#pragma once
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __APPLE__
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#endif
#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
extern "C" {
  extern const unsigned char _binary_about_png_start[];
  extern const unsigned char _binary_about_png_end[];
}
#endif

class ImageLoader {
private:
  struct ByteVector {
    uint8_t* data;
    size_t length;
    size_t capacity;
  };

  static bool bytevector_init(ByteVector* vec, size_t initial_capacity) {
    vec->data = (uint8_t*)malloc(initial_capacity);
    if (!vec->data) return false;
    vec->length = 0;
    vec->capacity = initial_capacity;
    return true;
  }

  static bool bytevector_push(ByteVector* vec, uint8_t byte) {
    if (vec->length >= vec->capacity) {
      size_t new_capacity = vec->capacity * 2;
      if (new_capacity < 1024) new_capacity = 1024;
      uint8_t* new_data = (uint8_t*)realloc(vec->data, new_capacity);
      if (!new_data) return false;
      vec->data = new_data;
      vec->capacity = new_capacity;
    }
    vec->data[vec->length++] = byte;
    return true;
  }

  static void bytevector_free(ByteVector* vec) {
    if (vec->data) {
      free(vec->data);
      vec->data = nullptr;
    }
    vec->length = 0;
    vec->capacity = 0;
  }

  struct BitStream {
    const uint8_t* data;
    size_t size;
    size_t byte_pos;
    uint32_t bit_buffer;
    int bits_in_buffer;
  };

  static void bitstream_init(BitStream* bs, const uint8_t* data, size_t size) {
    bs->data = data;
    bs->size = size;
    bs->byte_pos = 0;
    bs->bit_buffer = 0;
    bs->bits_in_buffer = 0;
  }

  static int bitstream_read_bits(BitStream* bs, int n) {
    while (bs->bits_in_buffer < n) {
      if (bs->byte_pos >= bs->size) return -1;
      bs->bit_buffer |= (uint32_t)bs->data[bs->byte_pos++] << bs->bits_in_buffer;
      bs->bits_in_buffer += 8;
    }
    int result = bs->bit_buffer & ((1 << n) - 1);
    bs->bit_buffer >>= n;
    bs->bits_in_buffer -= n;
    return result;
  }

  static int bitstream_read_byte_aligned(BitStream* bs) {
    bs->bit_buffer = 0;
    bs->bits_in_buffer = 0;
    if (bs->byte_pos >= bs->size) return -1;
    return bs->data[bs->byte_pos++];
  }

  static void bitstream_align_to_byte(BitStream* bs) {
    bs->bit_buffer = 0;
    bs->bits_in_buffer = 0;
  }

  struct HuffmanTable {
    int max_code[16];
    int offset[16];
    uint16_t symbols[288];
  };

  static void huffman_build(HuffmanTable* table, const uint8_t* lengths, int n) {
    int bl_count[16] = { 0 };
    for (int i = 0; i < n; i++) {
      if (lengths[i] > 0 && lengths[i] < 16) bl_count[lengths[i]]++;
    }

    int code = 0;
    bl_count[0] = 0;
    int next_code[16] = { 0 };
    for (int bits = 1; bits < 16; bits++) {
      code = (code + bl_count[bits - 1]) << 1;
      next_code[bits] = code;
    }

    for (int bits = 0; bits < 16; bits++) {
      table->max_code[bits] = -1;
      table->offset[bits] = 0;
    }

    int sym_idx = 0;
    for (int bits = 1; bits < 16; bits++) {
      table->offset[bits] = sym_idx - next_code[bits];
      for (int i = 0; i < n; i++) {
        if (lengths[i] == bits) {
          table->symbols[sym_idx++] = i;
          table->max_code[bits] = next_code[bits]++;
        }
      }
    }
  }

  static int huffman_decode(const HuffmanTable* table, BitStream* bs) {
    int code = 0;
    for (int len = 1; len < 16; len++) {
      int bit = bitstream_read_bits(bs, 1);
      if (bit < 0) return -1;
      code = (code << 1) | bit;
      if (code <= table->max_code[len] && table->max_code[len] >= 0) {
        return table->symbols[table->offset[len] + code];
      }
    }
    return -1;
  }

  static uint32_t adler32(const uint8_t* data, size_t len) {
    uint32_t a = 1, b = 0;
    const uint32_t MOD_ADLER = 65521;
    for (size_t i = 0; i < len; i++) {
      a = (a + data[i]) % MOD_ADLER;
      b = (b + a) % MOD_ADLER;
    }
    return (b << 16) | a;
  }

  static constexpr uint16_t length_base[29] = {
    3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,
    35,43,51,59,67,83,99,115,131,163,195,227,258
  };
  static constexpr uint8_t length_extra[29] = {
    0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,
    3,3,3,3,4,4,4,4,5,5,5,5,0
  };
  static constexpr uint16_t dist_base[30] = {
    1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
    257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577
  };
  static constexpr uint8_t dist_extra[30] = {
    0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,
    7,7,8,8,9,9,10,10,11,11,12,12,13,13
  };

  static void init_fixed_tables(uint8_t* lit_lengths, uint8_t* dist_lengths) {
    for (int i = 0; i <= 143; i++) lit_lengths[i] = 8;
    for (int i = 144; i <= 255; i++) lit_lengths[i] = 9;
    for (int i = 256; i <= 279; i++) lit_lengths[i] = 7;
    for (int i = 280; i <= 287; i++) lit_lengths[i] = 8;
    for (int i = 0; i < 32; i++) dist_lengths[i] = 5;
  }

  static int decode_dynamic_tables(BitStream* bs, uint8_t* lit_lengths, uint8_t* dist_lengths) {
    int hlit = bitstream_read_bits(bs, 5);
    if (hlit < 0) return -1; hlit += 257;
    int hdist = bitstream_read_bits(bs, 5);
    if (hdist < 0) return -1; hdist += 1;
    int hclen = bitstream_read_bits(bs, 4);
    if (hclen < 0) return -1; hclen += 4;

    const uint8_t cl_order[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
    uint8_t cl_lengths[19] = { 0 };
    for (int i = 0; i < hclen; i++) {
      int len = bitstream_read_bits(bs, 3);
      if (len < 0) return -1;
      cl_lengths[cl_order[i]] = (uint8_t)len;
    }

    HuffmanTable cl_table;
    huffman_build(&cl_table, cl_lengths, 19);

    uint8_t all_lengths[320] = { 0 };
    int idx = 0, total = hlit + hdist;

    while (idx < total) {
      int sym = huffman_decode(&cl_table, bs);
      if (sym < 0) return -1;
      if (sym < 16) all_lengths[idx++] = (uint8_t)sym;
      else if (sym == 16) {
        if (idx == 0) return -1;
        int repeat = bitstream_read_bits(bs, 2);
        if (repeat < 0) return -1;
        repeat += 3;
        uint8_t val = all_lengths[idx - 1];
        while (repeat-- > 0 && idx < total) all_lengths[idx++] = val;
      }
      else if (sym == 17) {
        int repeat = bitstream_read_bits(bs, 3);
        if (repeat < 0) return -1;
        repeat += 3;
        while (repeat-- > 0 && idx < total) all_lengths[idx++] = 0;
      }
      else if (sym == 18) {
        int repeat = bitstream_read_bits(bs, 7);
        if (repeat < 0) return -1;
        repeat += 11;
        while (repeat-- > 0 && idx < total) all_lengths[idx++] = 0;
      }
      else return -1;
    }

    for (int i = 0; i < hlit; i++) lit_lengths[i] = all_lengths[i];
    for (int i = 0; i < hdist; i++) dist_lengths[i] = all_lengths[hlit + i];

    return 0;
  }

  static int decode_block(BitStream* bs, ByteVector* output, const uint8_t* lit_lengths, const uint8_t* dist_lengths) {
    HuffmanTable lit_table, dist_table;
    huffman_build(&lit_table, lit_lengths, 288);
    huffman_build(&dist_table, dist_lengths, 32);

    while (1) {
      int symbol = huffman_decode(&lit_table, bs);
      if (symbol < 0) return -1;
      if (symbol < 256) {
        if (!bytevector_push(output, (uint8_t)symbol)) return -1;
      }
      else if (symbol == 256) return 0;
      else if (symbol <= 285) {
        int len_code = symbol - 257;
        if (len_code >= 29) return -1;

        int length = length_base[len_code];
        int extra_bits = length_extra[len_code];
        if (extra_bits > 0) {
          int extra = bitstream_read_bits(bs, extra_bits);
          if (extra < 0) return -1;
          length += extra;
        }

        int dist_code = huffman_decode(&dist_table, bs);
        if (dist_code < 0 || dist_code >= 30) return -1;
        int distance = dist_base[dist_code];
        extra_bits = dist_extra[dist_code];
        if (extra_bits > 0) {
          int extra = bitstream_read_bits(bs, extra_bits);
          if (extra < 0) return -1;
          distance += extra;
        }

        if (distance > (int)output->length || distance <= 0) return -1;
        size_t copy_pos = output->length - distance;
        for (int i = 0; i < length; i++) {
          if (!bytevector_push(output, output->data[copy_pos + i])) return -1;
        }
      }
      else return -1;
    }
  }

  static int zlib_decompress_deflate(const uint8_t* input, size_t size,
    ByteVector* output, size_t* bytes_consumed) {
    BitStream bs;
    bitstream_init(&bs, input, size);
    int is_final = 0;

    while (!is_final) {
      is_final = bitstream_read_bits(&bs, 1);
      if (is_final < 0) return -1;

      int type = bitstream_read_bits(&bs, 2);
      if (type < 0) return -1;

      if (type == 0) {
        bitstream_align_to_byte(&bs);
        int len_lo = bitstream_read_byte_aligned(&bs);
        int len_hi = bitstream_read_byte_aligned(&bs);
        int nlen_lo = bitstream_read_byte_aligned(&bs);
        int nlen_hi = bitstream_read_byte_aligned(&bs);
        if (len_lo < 0 || len_hi < 0 || nlen_lo < 0 || nlen_hi < 0) return -1;
        uint16_t len = (uint16_t)(len_lo | (len_hi << 8));
        uint16_t nlen = (uint16_t)(nlen_lo | (nlen_hi << 8));
        if (len != (uint16_t)~nlen) return -1;
        for (int i = 0; i < len; i++) {
          int b = bitstream_read_byte_aligned(&bs);
          if (b < 0) return -1;
          if (!bytevector_push(output, (uint8_t)b)) return -1;
        }
      }
      else if (type == 1) {
        uint8_t lit[288], dist[32];
        init_fixed_tables(lit, dist);
        if (decode_block(&bs, output, lit, dist) < 0) return -1;
      }
      else if (type == 2) {
        uint8_t lit[288] = { 0 }, dist[32] = { 0 };
        if (decode_dynamic_tables(&bs, lit, dist) < 0) return -1;
        if (decode_block(&bs, output, lit, dist) < 0) return -1;
      }
      else {
        return -1;
      }
    }

    if (bytes_consumed) *bytes_consumed = bs.byte_pos;
    return 0;
  }

  static int zlib_decompress(const uint8_t* input, size_t input_size, ByteVector* output) {
    if (input_size < 6) return -1;
    uint8_t cmf = input[0], flg = input[1];
    if ((cmf & 0x0F) != 8) return -1;
    if (((cmf << 8) + flg) % 31 != 0) return -1;
    if (flg & 0x20) return -1;

    if (input_size < 6) return -1;
    const uint8_t* deflate_data = input + 2;
    size_t deflate_size = input_size - 6;
    size_t consumed = 0;

    if (zlib_decompress_deflate(deflate_data, deflate_size, output, &consumed) < 0)
      return -1;

    if (input_size >= 6 + 4) {
      uint32_t stored_checksum =
        ((uint32_t)input[input_size - 4] << 24) |
        ((uint32_t)input[input_size - 3] << 16) |
        ((uint32_t)input[input_size - 2] << 8) |
        ((uint32_t)input[input_size - 1]);
      uint32_t computed = adler32(output->data, output->length);
      if (stored_checksum != computed) return -1;
    }

    return 0;
  }

  static uint32_t ReadU32BE(const std::vector<uint8_t>& data, size_t pos) {
    return (uint32_t(data[pos]) << 24) | (uint32_t(data[pos + 1]) << 16) |
      (uint32_t(data[pos + 2]) << 8) | uint32_t(data[pos + 3]);
  }

  static bool InflateZlib(const std::vector<uint8_t>& compressed, std::vector<uint8_t>& out) {
    ByteVector output;
    if (!bytevector_init(&output, compressed.size() * 4)) return false;

    int result = zlib_decompress(compressed.data(), compressed.size(), &output);

    if (result == 0) {
      out.assign(output.data, output.data + output.length);
    }

    bytevector_free(&output);
    return (result == 0);
  }

  static uint8_t PaethPredictor(uint8_t a, uint8_t b, uint8_t c) {
    int p = a + b - c;
    int pa = abs(p - a);
    int pb = abs(p - b);
    int pc = abs(p - c);
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
  }

  static bool UnfilterAndConvert(const std::vector<uint8_t>& data, std::vector<uint8_t>& rgba,
    uint32_t width, uint32_t height, uint8_t colorType, uint8_t bitDepth) {
    if (bitDepth != 8) return false;
    uint32_t bytesPerPixel = 0;
    bool hasAlpha = false;

    switch (colorType) {
    case 0: bytesPerPixel = 1; break;
    case 2: bytesPerPixel = 3; break;
    case 4: bytesPerPixel = 2; hasAlpha = true; break;
    case 6: bytesPerPixel = 4; hasAlpha = true; break;
    default: return false;
    }

    uint32_t stride = width * bytesPerPixel + 1;
    if (data.size() < stride * height) return false;

    rgba.resize(width * height * 4);
    std::vector<uint8_t> prevRow(width * bytesPerPixel, 0);

    for (uint32_t y = 0; y < height; y++) {
      uint8_t filter = data[y * stride];
      const uint8_t* row = &data[y * stride + 1];
      std::vector<uint8_t> currRow(width * bytesPerPixel);

      for (uint32_t x = 0; x < width * bytesPerPixel; x++) {
        uint8_t a = (x >= bytesPerPixel) ? currRow[x - bytesPerPixel] : 0;
        uint8_t b = prevRow[x];
        uint8_t c = (x >= bytesPerPixel) ? prevRow[x - bytesPerPixel] : 0;

        switch (filter) {
        case 0: currRow[x] = row[x]; break;
        case 1: currRow[x] = row[x] + a; break;
        case 2: currRow[x] = row[x] + b; break;
        case 3: currRow[x] = row[x] + ((a + b) >> 1); break;
        case 4: currRow[x] = row[x] + PaethPredictor(a, b, c); break;
        default: return false;
        }
      }

      for (uint32_t x = 0; x < width; x++) {
        uint32_t outPos = (y * width + x) * 4;
        uint32_t inPos = x * bytesPerPixel;

        if (colorType == 0) {
          rgba[outPos] = rgba[outPos + 1] = rgba[outPos + 2] = currRow[inPos];
          rgba[outPos + 3] = 255;
        }
        else if (colorType == 2) {
          rgba[outPos] = currRow[inPos];
          rgba[outPos + 1] = currRow[inPos + 1];
          rgba[outPos + 2] = currRow[inPos + 2];
          rgba[outPos + 3] = 255;
        }
        else if (colorType == 4) {
          rgba[outPos] = rgba[outPos + 1] = rgba[outPos + 2] = currRow[inPos];
          rgba[outPos + 3] = currRow[inPos + 1];
        }
        else if (colorType == 6) {
          rgba[outPos] = currRow[inPos];
          rgba[outPos + 1] = currRow[inPos + 1];
          rgba[outPos + 2] = currRow[inPos + 2];
          rgba[outPos + 3] = currRow[inPos + 3];
        }
      }

      prevRow = currRow;
    }

    return true;
  }

  static bool DecodePNG(const std::vector<uint8_t>& pngData, std::vector<uint8_t>& rgba,
    uint32_t& width, uint32_t& height) {
    if (pngData.size() < 8) return false;

    const uint8_t sig[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
    if (memcmp(pngData.data(), sig, 8) != 0) return false;

    size_t pos = 8;
    std::vector<uint8_t> idat;
    uint8_t colorType = 0;
    uint8_t bitDepth = 0;
    width = height = 0;

    while (pos + 12 <= pngData.size()) {
      uint32_t len = ReadU32BE(pngData, pos);
      pos += 4;
      if (pos + 4 + len + 4 > pngData.size()) break;

      std::string type(reinterpret_cast<const char*>(&pngData[pos]), 4);
      pos += 4;

      if (type == "IHDR" && len >= 13) {
        width = ReadU32BE(pngData, pos);
        height = ReadU32BE(pngData, pos + 4);
        bitDepth = pngData[pos + 8];
        colorType = pngData[pos + 9];
      }
      else if (type == "IDAT") {
        idat.insert(idat.end(), pngData.begin() + pos, pngData.begin() + pos + len);
      }
      else if (type == "IEND") {
        break;
      }

      pos += len + 4; // Skip data + CRC
    }

    if (width == 0 || height == 0 || idat.empty()) return false;

    std::vector<uint8_t> decompressed;
    if (!InflateZlib(idat, decompressed)) return false;

    return UnfilterAndConvert(decompressed, rgba, width, height, colorType, bitDepth);
  }

public:
  static bool LoadPNG(const char* name, std::vector<uint8_t>& outData) {
    outData.clear();
#ifdef _WIN32
    HMODULE hModule = GetModuleHandle(nullptr);
    if (!hModule) return false;

    bool isIntResource = (reinterpret_cast<uintptr_t>(name) < 0x10000);

    HRSRC hRes = nullptr;
    if (isIntResource) {
      hRes = FindResourceW(hModule, reinterpret_cast<LPCWSTR>(name), RT_RCDATA);
    }
    else {
      hRes = FindResourceA(hModule, name, "PNG");
    }

    if (!hRes) return false;
    HGLOBAL hMem = LoadResource(hModule, hRes);
    if (!hMem) return false;
    DWORD size = SizeofResource(hModule, hRes);
    void* ptr = LockResource(hMem);
    if (!ptr || size == 0) return false;
    outData.assign(static_cast<uint8_t*>(ptr), static_cast<uint8_t*>(ptr) + size);
    return true;
#endif
#ifdef __APPLE__
    @autoreleasepool{
      NSString * nsName = [NSString stringWithUTF8String : name];
      NSString* base = [nsName stringByDeletingPathExtension];
      NSString* ext = [nsName pathExtension];
      NSBundle* bundle = [NSBundle mainBundle];
      NSString* path = [bundle pathForResource : base ofType : ext];
      if (!path) return false;
      NSData* data = [NSData dataWithContentsOfFile : path];
      if (!data) return false;
      outData.resize([data length]);
      memcpy(outData.data(),[data bytes],[data length]);
      return true;
    }
#endif
#ifdef __linux__
    size_t size = _binary_about_png_end - _binary_about_png_start;

    if (size > 0) {
      outData.assign(_binary_about_png_start, _binary_about_png_end);
      return true;
    }
    return false;
#endif
  }

  static bool LoadPNGDecoded(const char* name, std::vector<uint8_t>& rgbaData,
    int& width, int& height) {
    uint32_t w, h;
    std::vector<uint8_t> pngData;
    if (!LoadPNG(name, pngData)) return false;
    if (!DecodePNG(pngData, rgbaData, w, h)) return false;
    width = static_cast<int>(w);
    height = static_cast<int>(h);
    return true;
  }

  static bool LoadPNGDecoded(const char* name, std::vector<uint8_t>& rgbaData,
    uint32_t& width, uint32_t& height) {
    std::vector<uint8_t> pngData;
    if (!LoadPNG(name, pngData)) return false;
    return DecodePNG(pngData, rgbaData, width, height);
  }

#ifdef _WIN32
  static HBITMAP CreateHBITMAP(const std::vector<uint8_t>& rgbaData,
    int width, int height, bool premultiplyAlpha = true) {
    if (width <= 0 || height <= 0) return nullptr;
    if (rgbaData.size() != static_cast<size_t>(width * height * 4)) return nullptr;

    BITMAPV5HEADER bmi = {};
    bmi.bV5Size = sizeof(BITMAPV5HEADER);
    bmi.bV5Width = width;
    bmi.bV5Height = -height; // top-down
    bmi.bV5Planes = 1;
    bmi.bV5BitCount = 32;
    bmi.bV5Compression = BI_BITFIELDS;
    bmi.bV5RedMask = 0x00FF0000;
    bmi.bV5GreenMask = 0x0000FF00;
    bmi.bV5BlueMask = 0x000000FF;
    bmi.bV5AlphaMask = 0xFF000000;

    void* bits = nullptr;
    HDC hdc = GetDC(nullptr);
    HBITMAP hBitmap = CreateDIBSection(hdc, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, hdc);

    if (hBitmap && bits) {
      uint8_t* dest = static_cast<uint8_t*>(bits);

      for (size_t i = 0; i < rgbaData.size(); i += 4) {
        uint8_t r = rgbaData[i];
        uint8_t g = rgbaData[i + 1];
        uint8_t b = rgbaData[i + 2];
        uint8_t a = rgbaData[i + 3];

        if (premultiplyAlpha && a < 255) {
          r = (r * a) / 255;
          g = (g * a) / 255;
          b = (b * a) / 255;
        }

        dest[i] = b;
        dest[i + 1] = g;
        dest[i + 2] = r;
        dest[i + 3] = a;
      }
    }

    return hBitmap;
  }

  static void DrawTransparentBitmap(HDC hdc, HBITMAP hBitmap, int x, int y, int width, int height) {
    BITMAP bm;
    GetObject(hBitmap, sizeof(BITMAP), &bm);

    HDC hdcSrc = CreateCompatibleDC(hdc);
    HDC hdcDst = CreateCompatibleDC(hdc);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    uint32_t* pDstBits = nullptr;
    HBITMAP hDstBitmap = CreateDIBSection(hdcDst, &bmi, DIB_RGB_COLORS, (void**)&pDstBits, nullptr, 0);

    uint32_t* pSrcBits = nullptr;
    HBITMAP hSrcBitmap = CreateDIBSection(hdcSrc, &bmi, DIB_RGB_COLORS, (void**)&pSrcBits, nullptr, 0);

    if (!pDstBits || !pSrcBits) {
      if (hDstBitmap) DeleteObject(hDstBitmap);
      if (hSrcBitmap) DeleteObject(hSrcBitmap);
      DeleteDC(hdcSrc);
      DeleteDC(hdcDst);
      return;
    }

    HBITMAP hOldSrc = (HBITMAP)SelectObject(hdcSrc, hSrcBitmap);
    HBITMAP hOldDst = (HBITMAP)SelectObject(hdcDst, hDstBitmap);

    HDC hdcTemp = CreateCompatibleDC(hdc);
    HBITMAP hOldTemp = (HBITMAP)SelectObject(hdcTemp, hBitmap);
    StretchBlt(hdcSrc, 0, 0, width, height, hdcTemp, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
    SelectObject(hdcTemp, hOldTemp);
    DeleteDC(hdcTemp);

    BitBlt(hdcDst, 0, 0, width, height, hdc, x, y, SRCCOPY);

    for (int py = 0; py < height; py++) {
      for (int px = 0; px < width; px++) {
        int idx = py * width + px;

        uint32_t src = pSrcBits[idx];
        uint32_t dst = pDstBits[idx];

        uint8_t srcA = (src >> 24) & 0xFF;
        uint8_t srcR = (src >> 16) & 0xFF;
        uint8_t srcG = (src >> 8) & 0xFF;
        uint8_t srcB = src & 0xFF;

        uint8_t dstR = (dst >> 16) & 0xFF;
        uint8_t dstG = (dst >> 8) & 0xFF;
        uint8_t dstB = dst & 0xFF;

        uint8_t outR = (srcR * srcA + dstR * (255 - srcA)) / 255;
        uint8_t outG = (srcG * srcA + dstG * (255 - srcA)) / 255;
        uint8_t outB = (srcB * srcA + dstB * (255 - srcA)) / 255;

        pDstBits[idx] = (0xFF << 24) | (outR << 16) | (outG << 8) | outB;
      }
    }

    BitBlt(hdc, x, y, width, height, hdcDst, 0, 0, SRCCOPY);

    SelectObject(hdcSrc, hOldSrc);
    SelectObject(hdcDst, hOldDst);
    DeleteObject(hSrcBitmap);
    DeleteObject(hDstBitmap);
    DeleteDC(hdcSrc);
    DeleteDC(hdcDst);
  }
#endif
};

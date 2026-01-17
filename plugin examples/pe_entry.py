def get_info():
    """
    HexViewer plugin metadata.
    Your C++ side calls this via GetPythonPluginInfo().
    """
    return {
        "name": "PE Entry Point Bookmark",
        "version": "1.0",
        "author": "HexViewer",
        "description": "Bookmarks the PE executable entry point"
    }


def generate_bookmarks(data: bytes, size: int):
    """
    HexViewer calls this via ExecutePluginBookmarks().
    Must return a list of dicts:
        { "offset": int, "label": str, "description": str }
    """
    bookmarks = []

    # --- Basic validation ---
    if not data or size < 0x100:
        return bookmarks

    # DOS header: "MZ"
    if data[0:2] != b"MZ":
        return bookmarks

    # e_lfanew: offset to PE header
    if 0x40 > size:
        return bookmarks
    pe_offset = int.from_bytes(data[0x3C:0x40], "little")

    # PE header must fit
    if pe_offset + 0x18 > size:
        return bookmarks

    # PE signature: "PE\0\0"
    if data[pe_offset:pe_offset + 4] != b"PE\x00\x00":
        return bookmarks

    # Number of sections (WORD at +6)
    num_sections = int.from_bytes(data[pe_offset + 6:pe_offset + 8], "little")

    # Optional header size (WORD at +20)
    opt_header_size = int.from_bytes(data[pe_offset + 20:pe_offset + 22], "little")

    # Optional header starts at PE + 24
    optional_header_offset = pe_offset + 24

    if optional_header_offset + opt_header_size > size:
        return bookmarks

    # AddressOfEntryPoint is at offset 0x10 in Optional Header
    entry_rva_offset = optional_header_offset + 0x10
    if entry_rva_offset + 4 > size:
        return bookmarks

    entry_rva = int.from_bytes(data[entry_rva_offset:entry_rva_offset + 4], "little")

    # Section table starts after optional header
    section_table_offset = optional_header_offset + opt_header_size

    # Each IMAGE_SECTION_HEADER is 40 bytes
    entry_file_offset = None

    for i in range(num_sections):
        sec = section_table_offset + i * 40
        if sec + 40 > size:
            break

        virtual_address = int.from_bytes(data[sec + 12:sec + 16], "little")
        raw_size        = int.from_bytes(data[sec + 16:sec + 20], "little")
        raw_offset      = int.from_bytes(data[sec + 20:sec + 24], "little")

        # Simple RVA-in-section check
        if virtual_address <= entry_rva < virtual_address + raw_size:
            entry_file_offset = raw_offset + (entry_rva - virtual_address)
            break

    if entry_file_offset is None:
        return bookmarks

    # Make sure offset is in file range
    if not (0 <= entry_file_offset < size):
        return bookmarks

    # --- Add bookmark ---
    bookmarks.append({
        "label": "Entry Point",
        "offset": entry_file_offset,
        "description": f"entry point (RVA 0x{entry_rva:X})"
    })

    return bookmarks


# Optional: tiny self-test when run directly
if __name__ == "__main__":
    # This won’t be used by HexViewer, but lets you quickly sanity-check.
    import sys
    if len(sys.argv) != 2:
        print("Usage: python pe_entry.py <pe-file>")
        sys.exit(1)

    path = sys.argv[1]
    with open(path, "rb") as f:
        data = f.read()

    bms = generate_bookmarks(data, len(data))
    for bm in bms:
        print(f"Bookmark at file offset 0x{bm['offset']:X}: {bm['label']} - {bm['description']}")
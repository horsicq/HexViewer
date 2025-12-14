#ifndef HEXDATA_H
#define HEXDATA_H
#include <vector>
#include <string>
#include <cstdint>
#include <capstone/capstone.h>

class HexData {
public:
  HexData();
  ~HexData();
  bool loadFile(const char* filepath);
  bool saveFile(const char* filepath);
  void clear();
  const std::vector<std::string>& getHexLines() const { return hexLines; }
  const std::vector<std::string>& getDisassemblyLines() const { return disassemblyLines; }
  std::string getHeaderLine() const { return headerLine; }
  size_t getFileSize() const { return fileData.size(); }
  bool isEmpty() const { return fileData.empty(); }
  int getCurrentBytesPerLine() const { return currentBytesPerLine; }
  bool editByte(size_t offset, uint8_t newValue);
  uint8_t getByte(size_t offset) const;
  uint8_t readByte(size_t offset) const { return getByte(offset); } 
  bool isModified() const { return modified; }
  void setModified(bool mod) { modified = mod; }
  void regenerateHexLines(int bytesPerLine);
  void setArchitecture(cs_arch arch, cs_mode mode);

private:
  void convertDataToHex(int bytesPerLine);
  void generateHeader(int bytesPerLine);
  void generateDisassembly(int bytesPerLine);
  std::string disassembleInstruction(size_t offset, int& instructionLength);
  bool initializeCapstone();
  void cleanupCapstone();

  std::vector<uint8_t> fileData;
  std::vector<std::string> hexLines;
  std::vector<std::string> disassemblyLines;
  std::string headerLine;
  std::string filename;
  int currentBytesPerLine;
  bool modified;
  csh csHandle;
  bool capstoneInitialized;
  cs_arch currentArch;
  cs_mode currentMode;
};
#endif // HEXDATA_H

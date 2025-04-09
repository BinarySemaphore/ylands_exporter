/* AI Generated */
#include "bmp.hpp"

#include <fstream>

#pragma pack(push, 1) // Ensure no padding in struct
struct BMPFileHeader {
    uint16_t fileType{0x4D42}; // "BM"
    uint32_t fileSize{0};
    uint16_t reserved1{0};
    uint16_t reserved2{0};
    uint32_t dataOffset{0};
};

struct BMPInfoHeader {
    uint32_t headerSize{0};
    int32_t width{0};
    int32_t height{0};
    uint16_t planes{1};
    uint16_t bitsPerPixel{24};
    uint32_t compression{0};
    uint32_t imageSize{0};
    int32_t xPixelsPerMeter{0};
    int32_t yPixelsPerMeter{0};
    uint32_t colorsUsed{0};
    uint32_t importantColors{0};
};
#pragma pack(pop)

bool write_bmp(const std::string& filename, int width, int height, const std::vector<uint8_t>& data) {
    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;
    
    infoHeader.headerSize = sizeof(BMPInfoHeader);
    infoHeader.width = width;
    infoHeader.height = height;
    infoHeader.imageSize = width * height * 3;

    fileHeader.dataOffset = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
    fileHeader.fileSize = fileHeader.dataOffset + infoHeader.imageSize;

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    file.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
    file.write(reinterpret_cast<const char*>(data.data()), data.size());

    file.close();
    return true;
}
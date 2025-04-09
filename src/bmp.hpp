/* AI Generated */
#ifndef BMP_H
#define BMP_H

#include <vector>

struct BMPFileHeader;
struct BMPInfoHeader;

bool write_bmp(const std::string& filename, int width, int height, const std::vector<uint8_t>& data);

#endif // BMP_H
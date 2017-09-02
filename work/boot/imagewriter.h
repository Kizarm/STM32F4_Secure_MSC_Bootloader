#ifndef IMAGEWRITER_H
#define IMAGEWRITER_H
#include <stdint.h>

class ImageWriter {
  public:
    ImageWriter();
    void eraseBlocks (uint32_t from, uint32_t len);
    void writeBytes  (uint8_t * data, uint32_t from, uint32_t len);
    void  readBytes  (uint8_t * data, uint32_t from, uint32_t len);
};

#endif // IMAGEWRITER_H

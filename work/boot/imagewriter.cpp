#include <string.h>
#include "debug.h"
#include "imagewriter.h"
#include "keys.h"
#include "tea.h"

extern "C" char          image_ctxt [];
extern "C" unsigned long image_size;

static const unsigned maxsectors = 12;
static const uint8_t  SectorSizes [] = {       // v KiB, celkem 1MiB, nulty zde budeme vynechavat = bootloader
  16, 16, 16, 16, 64, 128, 128, 128, 128, 128, 128, 128
};

ImageWriter::ImageWriter() {
}
void ImageWriter::eraseBlocks (uint32_t from, uint32_t len) {
  debug ("ERASE    from %3d => %d blocks\n", from, len);
}
// len je vzdy delitelna 8 (overeno, 64 bytu, prenasi se _vzdy_ po blocich 512)
void ImageWriter::readBytes (uint8_t * data, uint32_t from, uint32_t len) {
  uint8_t tmp [len]__attribute__((aligned (4)));  // Tady uz je problem, ze puvodni data jsou "nekde"
  memcpy (tmp, image_ctxt + from, len);           // a je lepe je mit zarovnana, takze je to kostrbate
  encrypt_block (tmp, len, GeneratedKeys.loader);
  memcpy (data, tmp, len);
}
void ImageWriter::writeBytes (uint8_t * data, uint32_t from, uint32_t len) {
  uint8_t tmp [len]__attribute__((aligned (4)));
  memcpy (tmp, data, len);
  decrypt_block (tmp, len, GeneratedKeys.loader);
  memcpy (image_ctxt + from, tmp, len); // flash writer
}

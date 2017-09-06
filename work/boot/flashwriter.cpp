#include <string.h>
#include "imagewriter.h"
#include "keys.h"
#include "tea.h"
#include "flash.h"

extern "C" char          image_ctxt [];
extern "C" unsigned long image_size;

static const unsigned maxsectors = 12;
static const uint8_t  SectorSizes [] = {       // v KiB, celkem 1MiB, nulty zde budeme vynechavat = bootloader
  16, 16, 16, 16, 64, 128, 128, 128, 128, 128, 128, 128
};
static unsigned char  isErased [maxsectors];
// SECTOR SIZES:32,64,96,128,256,512,768,1024,1280,1536,1792,2048 block
// SECTOR SIZES: 0,32,64,96, 224,480,736,992, 1248,1504,1760,2016


ImageWriter::ImageWriter() {
  for (unsigned n=0; n<maxsectors; n++) isErased [n] = 0;
}
void ImageWriter::eraseBlocks (uint32_t from, uint32_t len) {
  FLASHClass::Unlock();
  len += from;
  unsigned ers = 0;
  for (unsigned k=1; k<maxsectors; k++) {   // zaciname od 1
    if (ers < len) {
      if (!isErased[k]) {
        isErased[k] = 1;
        FLASHClass::EraseSector (k);
      }
    } else break;
    ers += SectorSizes[k] << 1;
  }
}
// len je vzdy delitelna 8 (overeno, 64 bytu, prenasi se _vzdy_ po blocich 512)
void ImageWriter::readBytes (uint8_t * data, uint32_t from, uint32_t len) {
  uint8_t tmp [len]__attribute__ ( (aligned (4))); // Tady uz je problem, ze puvodni data jsou "nekde"
  memcpy (tmp, image_ctxt + from, len);           // a je lepe je mit zarovnana, takze je to kostrbate
  encrypt_block (tmp, len, GeneratedKeys.loader);
  memcpy (data, tmp, len);
}
void ImageWriter::writeBytes (uint8_t * data, uint32_t from, uint32_t len) {
  uint8_t tmp [len]__attribute__ ( (aligned (4)));
  memcpy (tmp, data, len);
  decrypt_block (tmp, len, GeneratedKeys.loader);
  // memcpy (image_ctxt + from, tmp, len); // flash writer :

  if (FLASHClass::isLocked()) return; // chyba, ignoruj, ale nebude zapsano
  const uint32_t halfs = len >> 1;
  uint32_t address = (uint32_t) image_ctxt + from;
  uint16_t * towrite = (uint16_t*) tmp;
  for (unsigned n=0; n<halfs; n++) {
    FLASHClass::ProgramHalfWord (address, towrite[n]);
    address += 2;
  }
}

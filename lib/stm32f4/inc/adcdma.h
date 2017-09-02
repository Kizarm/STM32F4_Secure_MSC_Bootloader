#ifndef ADCDMA_H
#define ADCDMA_H

#include "stdint.h"

typedef void (*dataCallBack) (uint16_t* data, uint32_t len);

const uint32_t DmaBufLen = 120;

class AdcDma {
  public:
    AdcDma  (uint32_t us);
    void    setCallback (dataCallBack cb);
    static  uint16_t* getBuf (void) {return bufer;};
  protected:
    static void irq (void);
  private:
    static dataCallBack proces;
    static uint16_t*    pbuf0;
    static uint16_t*    pbuf1;
    static uint16_t     bufer [2 * DmaBufLen];
};

#endif // ADCDMA_H

#ifndef INERFACE_H
#define INERFACE_H

#define FPM_ARM

#include "mad.h"
#include "audiodma.h"
#include "gpio.h"

// Optimalizace pro Cortex-M4
static inline int16_t scale (mad_fixed_t sample) {
  int16_t __result;
  asm("ssat %0, %2, %1, asr %3"
     : "=&r" (__result)
     : "r" (sample),
       "M" (16), "M" (MAD_F_FRACBITS + 1 - 16));
  return __result;
}
// Jen pro fixní délku paketu 571 (1142) vzorků !!!
class Interface : public OneWay {
  public:
    Interface () : OneWay(), ledb(GpioPortD, 15) { ready = false; };
    // Voláno z přerušení od DMA zvuku (I2S)
    uint32_t Up (Sample * data, uint32_t len) {
      for (unsigned i=0; i<len; i++) {
        // Upravit a zkopírovat data
        Sample s;
        s.ss.l = scale (l_ch[i]);
        s.ss.r = scale (r_ch[i]);
        data [i] = s;
      }
      ready = true;     // můžou být další
      return len;
    }
    void pass (mad_fixed_t const * left, mad_fixed_t const * right, unsigned len) {
      l_ch  = left;     // jen nastav pointer na data
      r_ch  = right;
      ready = false;
      while (!ready) {  // a čekej až se data odeberou metodou Up()
      };
    }
    void blink (unsigned nchannels, unsigned brate, unsigned nsamples) {
      ~ledb;
    }
  private:
    mad_fixed_t const * l_ch;
    mad_fixed_t const * r_ch;
    volatile bool       ready;
    GpioClass           ledb;
};
extern void PlatformExit (void);

#endif //  INERFACE_H

#ifndef INERFACE_H
#define INERFACE_H

#define FPM_DEFAULT

# include "mad.h"
# include "audiodma.h"

# include <stdio.h>

static inline signed int scale (mad_fixed_t sample) {
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}
class Interface {
  public:
    Interface () { dac = 0; dlen = 0; };
    void operator += (AudioDma & a) {
      dac = & a;
    }
    void pass (mad_fixed_t const * left_ch, unsigned nsamples) {
      Sample tmp [nsamples];
      for (unsigned n=0; n<nsamples; n++) {
        Sample s;
        s.ss.l = scale (left_ch[n]);
        s.ss.r = scale (left_ch[n]);
        tmp [n] = s;
      }
      dac->Up (tmp, nsamples);
    }
    void blink (unsigned nchannels, unsigned brate, unsigned nsamples) {
      if (nsamples != dlen) {
        printf ("%d Channel(s), BitRate = %d, Data Size = %d\n", nchannels, brate, nsamples);
      }
      dlen = nsamples;
    }
  private:
    unsigned dlen;
    AudioDma * dac;
};
void PlatformExit (void) {
}

#endif //  INERFACE_H

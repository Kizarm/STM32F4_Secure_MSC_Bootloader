#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gsm.h"
#include "gpio.h"
#include "fifo.h"
#include "audio.h"
/** @file
 * 
 * Toto bylo přidáno pro vyzkoušení bootloaderu pro větší soubory.
 * Pro STM32F4 Discovery se to dá přeložit jen když je použit flash.patch,
 * do RAM se to prostě nevejde. Ale jde to vyzkoušet na Linuxu, pokud
 * v Makefile přepneme TARGET na unix. Chce to knihovnu libasound-dev.
 * Proto jsou tu ta divná ifdef.
 * */
#ifndef __arm__
#include <unistd.h>
  void __WFI () {usleep(1000);};
#endif // !__arm__

extern "C" const unsigned char DataSource[];
extern "C" const unsigned int  DataLenght;
volatile   int                 gblMutex=0;

static GsmStatic         GSM;

static GpioClass         ledg (GpioPortD, 12);
static GpioClass         ledo (GpioPortD, 13);
static Audio             aud;
#ifdef __arm__
static Fifo<256, short>  fdc;
#else
// Linux přepíná mezi thready, což trvá déle než režie přerušení na ARM, takže vyžaduje delší frontu.
static Fifo<2560,short>  fdc;
#endif
static volatile unsigned tim_cnt = 0;

static int16_t mySample (void) {
  short s = 0;
  if (fdc.Read (s)) {
#ifdef __arm__
    s >>= 2;      // jinak to strasne rve
#endif
  }
  tim_cnt += 1;
  return s;
}
// Voláno s frekvencí 44100 Hz
static int16_t SampleDriver (void) {
  static int16_t  s  = 0;
  static unsigned lr = 0, max = 0;
  // Tenhle divný algoritmus způsobí, že volání SampleDriver() s frekvencí 44000 Hz
  // se rozseká tak, že mySample() se volá s frekvencí 8kHz, těch 100Hz se zanedbá.
  if (!max) {
    s = mySample();     // 8kHz
    if (lr) max = 5;
    else    max = 4;
    lr ^= 1u;
  } else {
    max -= 1;
  }
  return s;             // 44 kHz
}

static void wait (unsigned ms) {
  ms <<= 3; tim_cnt = 0;
  while (tim_cnt < ms) {
    __WFI ();
  }
}

static void fillbuf (short * buf, unsigned len) {
  for (unsigned n=0; n<len; n++) {
    short h = buf[n];
    while (!fdc.Write(h)) {
      __WFI();
    }
  }
}
static void play (void) {
  const unsigned step = sizeof(gsm_frame), raw = 160;
  gsm_frame  s;
  gsm_signal d [raw];
  
  unsigned n = 0;
  while (true) {
    memcpy (s, DataSource + n, step);
    n += step;
#ifdef __arm__
    +ledo;
#endif //__arm__
    GSM.decode (s, d);
#ifdef __arm__
    -ledo;
#endif //__arm__
    fillbuf    (d, raw);
    if (n >= DataLenght) {
      break;
    }
  }
}
/* Zabere  1240 B stacku !!!
 * */
int main (void) {
  aud.SetDriver (SampleDriver);
  aud.IrqEnable (true);
  while (true) {
    +ledg;
    play ();
    -ledg;
    wait (3000u);
  }
  return 0;
}

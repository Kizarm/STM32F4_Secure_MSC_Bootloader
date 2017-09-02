#ifndef EFFECT_H
#define EFFECT_H
#include <stdint.h>
#include "gpio.h"

/** Používá modrou ledku na F0 Discovery.
 * */
static const uint32_t NoOfChannels = 4;

class Efect {
  public:
    Efect();
    void    start (bool on);
  private:
    GpioClass led;
};

#endif // EFFECT_H

#include "gpio.h"
#include "efect.h"
#include "leds.h"

Efect::Efect() : led (LEDGREEN) {
}
void Efect::start (bool on) {
  if (on) +led;
  else    -led;
}

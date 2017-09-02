#include "gpio.h"

GpioClass::GpioClass (GpioPortNum const port, const uint32_t no, const GPIOMode_TypeDef type) : pos(1<<no), num(no) {
}



#include "gpio.h"

struct UserDescriptor {
  uint32_t magic;
  uint32_t size;
  uint32_t check;
};
const uint32_t DescriptorOffset = 0x188;
const uint32_t UserMagicNumber  = 0x12345678;
extern "C" unsigned long image_size;
extern "C" uint8_t     * user_code_begin; // from jump.s

unsigned long image_size=0;

static bool UserCheck (void) {
  image_size = 0;
  UserDescriptor * ud = reinterpret_cast<UserDescriptor*>(user_code_begin + DescriptorOffset);
  if (ud->magic != UserMagicNumber) return false;
  uint32_t   len  = ud->size >> 2, cs = 0u;
  if (!len)                         return false;
  uint32_t * wptr = reinterpret_cast<uint32_t*>(user_code_begin);
  for (unsigned n=0; n<len; n++) cs += wptr [n];
  if (cs)                           return false;
  image_size = ud->size;
  return true;
}
/* C++ button needed */
extern "C" uint32_t check_user (void) {
  GpioClass butt (GpioPortA,  0, GPIO_Mode_IN);
  UserCheck (); // nastaveni image_size
  if (butt.get())  return 1;
  if (!image_size) return 1;
  return 0;
}

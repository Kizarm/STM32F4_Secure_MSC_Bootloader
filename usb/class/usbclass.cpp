#include <stdio.h>
#include <stdlib.h>
#include "usbclass.h"

UsbClass::UsbClass () {
  next = NULL;
}

void UsbClass::append (UsbClass * p) {
  if (next) next->append(p);
  else next = p;
}
void UsbClass::addIface (UsbInterface * p) {
  if (list) list->append(p);
  else list = p;
}
/** ************************************/
#ifdef DSCBLD
#include <string.h>

void UsbClass::blddsc (void) {
  for (UsbInterface * i = list; i; i = i->next) {
    i->blddsc();
  }
}
unsigned int UsbClass::enumerate (unsigned int in) {
  unsigned int res = in;
  for (UsbInterface * i = list; i; i = i->next) {
    res = i->enumerate (res);
  }
  return res;
}
#endif // DSCBLD

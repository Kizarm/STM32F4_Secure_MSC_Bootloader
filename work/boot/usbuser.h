#ifndef USBUSER_H
#define USBUSER_H
#include "usbdevice.h"
#include "cdclass.h"
#include "msclass.h"

extern  UsbDevice usb;
//extern  CdClass   vcom;
//extern  CdClass   test;
extern  MsClass   disc;
  
  
extern void BuildDevice (void);

#endif // USBUSER_H

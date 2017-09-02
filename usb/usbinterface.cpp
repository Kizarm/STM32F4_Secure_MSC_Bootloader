#include <stdlib.h>
#include "usbinterface.h"
#include "dscbld.h"
#include "usbd.h"
#include "debug.h"

UsbInterface::UsbInterface () : ep (this) {
  next = NULL;
}

void UsbInterface::append (UsbInterface * p) {
  if (next) next->append(p);
  else next = p;
}
void UsbInterface::Handler (EndpointCallbackEvents e) {
  error ("UsbInterface\n");
}
IfacePostRunAction UsbInterface::SetupHandler (USB_SETUP_PACKET * p) {
  error ("UsbInterface\n");
  return IfacePostRunActionStall;
}
unsigned int UsbInterface::CommandHandler (USB_SETUP_PACKET * p, uint8_t * buf) {
  error ("UsbInterface\n");
  return 0;
}

/** *******************************************/
#ifdef DSCBLD
#include <string.h>

void UsbInterface::blddsc (void) {
  StructOut (&dsc, sizeof (usb_interface_descriptor));
  ep.blddsc ();
}
unsigned int UsbInterface::enumerate (unsigned int in) {
  memset (&dsc, 0, sizeof (usb_interface_descriptor));
  dsc.bLength          = sizeof (usb_interface_descriptor);
  dsc.bDescriptorType  = USB_DT_INTERFACE;
  dsc.bInterfaceNumber = IfCount;
  // printf ("IfCount=%d\n", IfCount);
  
  unsigned int res = in;
  res += sizeof (usb_interface_descriptor);
  res  = ep.enumerate (res);
  
  IfCount += 1;
  return res;
}
#endif // DSCBLD

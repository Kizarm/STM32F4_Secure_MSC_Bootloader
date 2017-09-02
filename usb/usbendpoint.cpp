#include "dscbld.h"
#include "usbendpoint.h"
#include "usbinterface.h"
#include "usbd.h"
#include "usbhw.h"
#include "debug.h"
#include "usbdevice.h"


UsbEndpoint::UsbEndpoint (UsbInterface * p) {
  addr   = 0;
  parent = p;
  type   = EndpointTypeBulk;
}

extern  UsbHw * UsbHwInstance;

void UsbEndpoint::Handler (EndpointCallbackEvents e) {
  // Obsluz Interface.
  if (parent) parent->Handler (e);
  else error ("UsbEndpoint\n");
}
/*
uint32_t UsbEndpoint::Read (uint8_t * pData) {
  uint32_t EpNum = addr;
  return UsbHwInstance->ReadEP (EpNum, pData);
}
uint32_t UsbEndpoint::Write (uint8_t * pData, uint32_t cnt) {
  uint32_t EpNum = addr | 0x80;
  return UsbHwInstance->WriteEP (EpNum, pData, cnt);
}
void UsbEndpoint::SetStall (UsbEndpointDir dir) {
  uint32_t EpNum = addr;
  if (dir == EndpointIn) EpNum |= 0x80;
  UsbHwInstance->SetStallEP (EpNum);
}
*/

/** ************************************/
#ifdef DSCBLD
#include <string.h>

void UsbEndpoint::blddsc (void) {
  //printf ("Trace %s\n", __FUNCTION__);
  switch (type) {
    case EndpointTypeCtrl: break;
    case EndpointTypeIso:  setIso  (); break;
    case EndpointTypeBulk: setBulk (); break;
    case EndpointTypeIntr: setIntr (); break;
  }
}
void UsbEndpoint::setBulk (void) {
  dsc.wMaxPacketSize = 64;
  StructOut (&dsc, sizeof (usb_endpoint_descriptor));
  dsc.bEndpointAddress.b.dir = EndpointIn;
  StructOut (&dsc, sizeof (usb_endpoint_descriptor));
}
void UsbEndpoint::setIntr (void) {
  dsc.wMaxPacketSize = 16;
  dsc.bInterval      = 2;
  dsc.bEndpointAddress.b.dir = EndpointIn;
  StructOut (&dsc, sizeof (usb_endpoint_descriptor));
}
void UsbEndpoint::setIso (void) {
  // TODO
}
unsigned int UsbEndpoint::enumerate (unsigned int in) {
  memset (&dsc, 0, sizeof (usb_endpoint_descriptor));
  dsc.bLength    = sizeof (usb_endpoint_descriptor);
  dsc.bDescriptorType        = USB_DT_ENDPOINT;
  dsc.bEndpointAddress.b.adr = addr;
  dsc.bmAttributes.b.trans   = type;
  dsc.bEndpointAddress.b.dir = EndpointOut;
  unsigned int res = in;
  //printf ("UsbEndpoint::enumerate in =%d\n", res);
  res += sizeof (usb_endpoint_descriptor);
  if ((type == EndpointTypeBulk) || (type == EndpointTypeIso))
    res += sizeof (usb_endpoint_descriptor);
  //printf ("UsbEndpoint::enumerate res=%d\n", res);
  return res;
}
#endif // DSCBLD

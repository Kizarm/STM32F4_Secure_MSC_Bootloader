#ifndef USBINTERFACE_H
#define USBINTERFACE_H
#include "usbstr.h"
#include "usbendpoint.h"

class UsbInterface {
  public:
    UsbInterface ();
    void append  (UsbInterface * p);
    virtual void     Handler        (EndpointCallbackEvents e);
    virtual IfacePostRunAction SetupHandler   (USB_SETUP_PACKET * p);
    virtual unsigned           CommandHandler (USB_SETUP_PACKET * p, uint8_t * buf);
  private:
  public:
    UsbInterface * next;
    UsbEndpoint    ep;
/** ************************************/
#ifdef DSCBLD
  public:
    virtual void     blddsc    (void);
    virtual unsigned enumerate (unsigned in);
    usb_interface_descriptor dsc;
#endif // DSCBLD
/** ************************************/
};

#endif // USBINTERFACE_H

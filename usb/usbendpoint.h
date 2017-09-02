#ifndef USBENDPOINT_H
#define USBENDPOINT_H
#include "usbstr.h"

enum EndpointCallbackEvents {
 USB_EVT_SETUP    =  1,  /* Setup Packet */
 USB_EVT_OUT,            /* OUT Packet */ 
 USB_EVT_IN,             /*  IN Packet */
 USB_EVT_OUT_NAK,        /* OUT Packet - Not Acknowledged */
 USB_EVT_IN_NAK,         /*  IN Packet - Not Acknowledged */
 USB_EVT_OUT_STALL,      /* OUT Packet - Stalled */
 USB_EVT_IN_STALL,       /*  IN Packet - Stalled */
};

class UsbInterface;

class UsbEndpoint {
  public:
    UsbEndpoint (UsbInterface * p);
    void set (UsbEndpointType t) {type = t;   };
    UsbEndpointType get (void)   {return type;};
    void Handler (EndpointCallbackEvents e);
    /** Veškeré operace nad endpointy se týkají hardware.
     * Proto jsou definovány v usbhw.cpp i když v deklarace
     * jsou zde. Je otázka, zda je to správně.
     * */
    uint32_t Read    (uint8_t *pData);
    uint32_t Write   (uint8_t *pData, uint32_t cnt);
    
    void EnableEP      (UsbEndpointDir dir);
    void DisableEP     (UsbEndpointDir dir);
    void ResetEP       (UsbEndpointDir dir);
    void SetStallEP    (UsbEndpointDir dir);
    void ClrStallEP    (UsbEndpointDir dir);    
    void DeactivateEP  (UsbEndpointDir dir);
    
  private:
    UsbInterface  * parent;
    UsbEndpointType type;
  public:
    uint8_t addr;
/** ************************************/
#ifdef DSCBLD
  public:
    void     blddsc    (void);
    unsigned enumerate (unsigned in);
    usb_endpoint_descriptor dsc;
  protected:
    void setBulk (void);
    void setIso  (void);
    void setIntr (void);
#endif // DSCBLD
/** ************************************/
};

#endif // USBENDPOINT_H

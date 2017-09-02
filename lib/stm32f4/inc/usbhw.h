#ifndef USBHW_H
#define USBHW_H

#include "usbendpoint.h"

struct EP_LIST {
  uint32_t  buf_ptr;
  uint32_t  buf_length;
};

const unsigned MaxPhyEps  = 4;
const unsigned USB_EP_NUM = MaxPhyEps * 2;

class  UsbDevice;

class UsbHw {
  public:
    UsbHw (UsbDevice * p);
    void Init          (void);
    void Connect       (uint32_t con);
    int  MainLoop      (void);
    void Handler       (void);
    //uint32_t ReadEP    (uint32_t EPNum, uint8_t *pData);
    //uint32_t WriteEP   (uint32_t EPNum, uint8_t *pData, uint32_t cnt);
    UsbDevice * GetParent (void) {
      return parent;
    }
    
    uint32_t ReadSetupEP (/*uint32_t EPNum, */uint8_t *pData);
    void IOClkConfig   (void);
    void Reset         (void);
    void WakeUp        (void);
    void SetAddress    (uint32_t adr);
    void SetHwAddress  (uint32_t adr) {
      SetAddress (adr); /// pro jine targety nedela nic
    }
    void Ep0OutHandler (uint32_t len);
    
    uint32_t GetFrame  (void);
    void ConfigEP      (usb_endpoint_descriptor * pEPD){};
    
    /// zde nic nedela
  protected:
    void Cleanup        (void);
    void Configure      (uint32_t cfg);
    void USB_EPInit     (void);
  private:
    UsbDevice       * parent;
    
  public:
    unsigned       useeps;
    UsbEndpoint  * eps [MaxPhyEps];
    

};

#endif // USBHW_H

#ifndef USBHW_H
#define USBHW_H

#include <pthread.h>
#include "gadgetfs.h"
#include "usbendpoint.h"

const unsigned MaxPhyEps = 6;

class  UsbDevice;
/** ****************************************************************************************/
#define USB_BUFSIZE (1024)
/**
Buffery pro jednotlivé EP
*/
struct USB_DataBuffer {
  uint8_t  buf[USB_BUFSIZE];      //!< místo pro data
  volatile uint32_t len;                   //!< délka
  volatile uint32_t ofs;                   //!< ofset
};
/// Fyzický endpoint - neukládá adresu EP, bude řízena indexem v poli
struct physicalEndpoint {
  usb_endpoint_descriptor * pD;   //!< ukazatel na příslušný deskriptor
  char                    * name; //!< název v /dev/gadget (úplná cesta)
  int                       fd;   //!< a jemu příslušný filedeskriptor
  pthread_t                 thr;  //!< každý EP spouští thread
  USB_DataBuffer            buf;  //!< a má vlastní buffer
};
/// Logický endpoint - kopíruje se vlastně struktura řadiče LPC1343
typedef struct _logicalEndpoint {
  int                 id;             //!< ID je v podstatě adresa,
                                      ///  odkazy pro thread by neměly být na zásobníku
  physicalEndpoint    out;            //!< jeden EP OUT
  physicalEndpoint    in;             //!< a jeden EP IN
  pthread_mutex_t     mut;            //!< mutex
  pthread_cond_t      flg;            //!< a podmínka zastavení threadu jsou společné
} logicalEndpoint_t;
/** ****************************************************************************************/
class UsbHw {
  public:
    UsbHw (UsbDevice * p);
    void Init          (void);
    void Connect       (uint32_t con);
    int  MainLoop      (void);
    void     ConfigEP  (usb_endpoint_descriptor * pEPD);
    uint32_t ReadEPHW  (uint32_t EPNum, uint8_t *pData);
    uint32_t WriteEPHW (uint32_t EPNum, uint8_t *pData, uint32_t cnt);
    void SetStallEPHW  (uint32_t EPNum){};
    UsbDevice * GetParent (void) {
      return parent;
    }
    
    /// zde nic nedela
    void IOClkConfig   (void){};
    void Reset         (void){};
    void Suspend       (void){};
    void Resume        (void){};
    void WakeUp        (void){};
    void WakeUpCfg     (uint32_t cfg){};
    void SetAddress    (uint32_t adr){};
    void SetHwAddress  (uint32_t adr){};
    /*
    void DirCtrlEP     (uint32_t dir){};
    void EnableEP      (uint32_t EPNum){};
    void DisableEP     (uint32_t EPNum){};
    void ResetEP       (uint32_t EPNum){};
    void ClrStallEP    (uint32_t EPNum){};
    */
    /// wrappers of static funct.
    void *wep0_thread         (void * param);
    void *winp_thread_handler (void * param);
    void *wout_thread_handler (void * param);
    
  protected:
    void Cleanup       (void);
    void Configure     (uint32_t cfg);
    void stop_io       (void);
    //void close_fd      (void *n_ptr);
    int  autoconfig    (void);
    char *build_config (char *cp);
    int  ep_config     (int n, int dir, const char *label);
    void handle_control(struct usb_ctrlrequest *setup);
    
  private:
    UsbDevice * parent;
    
  public:
    unsigned       useeps;
    UsbEndpoint  * eps [MaxPhyEps];
    
/** ****************************************************************************************/
    int    verbose;                     //!< úroveň výpisu debug informací
    struct usb_device_descriptor          *device_desc;       //!< ukazatel na usb_device_descriptor
    struct usb_config_descriptor          *config;            //!< ukazatel na usb_config_descriptor
    
    enum usb_device_speed                 current_speed;      //!< rychlost zařízení (má být full)
    volatile int                          loop;               //!< podmínka hlavní smyčky
    volatile int                          connected;          //!< stav USB zařízení
    int                                   noack;
    logicalEndpoint_t                     ep[MaxPhyEps];      //!< 5 logických EP (adresa = index)
/** ****************************************************************************************/

};

#endif // USBHW_H

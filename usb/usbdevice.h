#ifndef USBDEVICE_H
#define USBDEVICE_H
#include "usbclass.h"
#include "usbhw.h"
#include "usbstr.h"

/** @file
 * @brief Kořen stromu USB DEVICE stacku.
 * 
 * Záměr byl postavit strom zařízení z jednotlivých standardních tříd USB kde může být teoreticky
 * i víc instancí jedné třídy - což je i prakticky využito pro CDC. Omezení hardware na poměrně
 * malý počet fyzických endpointů toto úsilí dost maří. Vytvoření deskriptorů je záležitost programu,
 * standardní třídy mají standardní deskriptory, jen je nutné u některý položek zohlednit celkovou
 * strukturu zařízení. Dalo by se to dělat za chodu a držet deskriptory v RAM, celkem to není moc
 * dat, zde byl zvolen trochu jiný způsob.
 * Protože je možné celé zařízení emulovat v Linuxu pomocí gadgetfs, pak není problém oddělit část
 * vytvářející deskriptory, udělat z toho program pro Linux, ten pak spustit a tím vytvořit soubor
 * v němž jsou všechny potřebné deskriptory v konstantní formě a tento soubor pak lze použít i pro
 * ostatní platformy. Viz Makefile v adresářích ./bld.
 * 
 * Celé je to odvozeno od stacku pro LPCxxx firmy NXP napsaného fy. Keil. Ten kód byl poněkud nepřehledný,
 * takže možná to zaneslo do výsledku chyby, ale je to tak složité, že vrtat se v tom znamená zničit
 * to úplně. Oni to asi původně dělali přesně podle specifikace a tak některé věci jsou tam asi dost zbytečně.
 * 
 * Zatím jsou celkem hotové třídy CDC a MSC, vůbec není řešen izochronní přenos.
 * STM32F4 platforma má ten UsbHw dost divný, ale celkem až na detaily funkční.
 * */

/** ************************************/
// const unsigned MaxPhyEps = 5; // v usbhw.h - platform specific
const unsigned MaxIfaces       =  5;
const unsigned USB_MAX_PACKET0 = 64;
const unsigned USB_POWER       =  0;


/** USB Endpoint Data Structure */
struct USB_EP_DATA {
  uint8_t * pData;
  uint16_t  Count;
};
/** Divna vec, cert vi kdo to vymyslel */
union u8x64u16x32 {
  uint8_t  b [USB_MAX_PACKET0];
  uint16_t w [USB_MAX_PACKET0 >> 1];
};
    
/** ************************************/

class UsbDevice : public UsbHw {
  public:
    UsbDevice ();
    void addClass (UsbClass * p);
    void buildDev (void);
    void Start    (void);
    /// Metody původního stacku
    void ResetCore      (void);
    void SetupStage     (void);
    void DataInStage    (void);
    void DataOutStage   (void);
    void StatusInStage  (void);
    void StatusOutStage (void);
  protected:
    /// Metody původního stacku nepoužité zvenku
    uint32_t ReqGetStatus         (void);
    uint32_t ReqSetClrFeature     (uint32_t sc);
    uint32_t ReqSetAddress        (void);
    uint32_t ReqGetDescriptor     (void);
    uint32_t ReqGetConfiguration  (void);
    uint32_t ReqSetConfiguration  (void);
    uint32_t ReqGetInterface      (void);
    uint32_t ReqSetInterface      (void);
  public:  
    /// Metody původního stacku - inline
    void SetStallMask     (uint32_t mask) {
      USB_EndPointStall |= mask;
    }
    void ClrStallMask     (uint32_t mask) {
      USB_EndPointStall  = mask;
    }
    void SetStallHaltMask (uint32_t mask) {
      USB_EndPointStall |= mask;
      USB_EndPointHalt  |= mask;
    }
    /// Obsluha Endpointu 0
    void Ep0Handler (EndpointCallbackEvents e);
  protected:
    /// Rozsekání původní špagety.
    void Ep0EventSetup (void);
    void Ep0EventOut   (void);
    void Ep0EventIn    (void);
    
    void Ep0SetupClass (void);
    void Ep0OutClass   (void);
  private:
    UsbInterface   if0;			//!< Základní virtuální interface
    UsbClass     * list;		//!< Device obsahuje několik Class, zde je kořen
    unsigned       useifs;		//!< Počet použitých Interfaces
    UsbInterface * ifs [MaxIfaces];	//!< pole ukazatelů na jednotlivé Interface
  public:
    /// Tyhle divné proměnné používá původní stack jako globální
    uint16_t USB_DeviceStatus;
    uint8_t  USB_DeviceAddress;
    uint8_t  USB_Configuration;
    uint32_t USB_EndPointMask;
    uint8_t  USB_AltSetting [MaxIfaces];
    uint32_t USB_EndPointHalt;
    uint32_t USB_EndPointStall;
  private:  
    USB_SETUP_PACKET SetupPacket;	//!< SetupPacket
    u8x64u16x32      EP0Buf;		//!< zde jsou data pro Endpoint 0
    USB_EP_DATA      EP0Data;		//!< a struktura pro manipulaci s nimi
    
/** ************************************/
#ifdef DSCBLD
  public:
    void     blddsc     (void);
    unsigned enumerate  (unsigned in);
    void     setStrings (const char * Manufacturer, const char * Product, const char * SerialNumber);
  protected:
    usb_device_descriptor dsc;
    usb_config_descriptor cfg;
    char const * sManufacturer, * sProduct, * sSerialNumber; 
  protected:
    void blddev (void);
    void bldcfg (void);
    void bldstr (void);
#endif // DSCBLD
/** ************************************/
};

#endif // USBDEVICE_H

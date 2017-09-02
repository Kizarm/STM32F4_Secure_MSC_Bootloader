#include "gpio.h"
#include "usbhw.h"
#include "usbdevice.h"
#include "dscbld.h"
#include "debug.h"
extern "C" {
#include "stm32f4xx.h"
#include "usb_regs.h"
}

UsbHw * UsbHwInstance = 0;

#define USB_OTG_FS              ((USB_OTG_GREGS *) USB_OTG_FS_PERIPH_BASE)

#define USBx                    USB_OTG_FS
// #define CFG_DEV_ENDPOINTS       4
// definitions from stm32f4xx_ll_usb.h
#define USBx_PCGCCTL            *(__IO uint32_t *)((uint32_t)USBx + USB_OTG_PCGCCTL_BASE)
#define USBx_DEVICE             ((USB_OTG_DREGS *)((uint32_t )USBx + USB_OTG_DEVICE_BASE))
#define USBx_INEP(i)            ((USB_OTG_INEPREGS *)((uint32_t)USBx + USB_OTG_IN_ENDPOINT_BASE + (i)*USB_OTG_EP_REG_SIZE))
#define USBx_OUTEP(i)           ((USB_OTG_OUTEPREGS *)((uint32_t)USBx + USB_OTG_OUT_ENDPOINT_BASE + (i)*USB_OTG_EP_REG_SIZE))
#define USBx_DFIFO(i)           *(__IO uint32_t *)((uint32_t)USBx + USB_OTG_FIFO_BASE + (i) * USB_OTG_FIFO_SIZE)

#define USB_OTG_SPEED_FULL      (USB_OTG_DCFG_DSPD_1 | USB_OTG_DCFG_DSPD_0)

#define STS_GOUT_NAK            USB_OTG_PKTSTS_0
#define STS_DATA_UPDT           USB_OTG_PKTSTS_1
#define STS_XFER_COMP           (USB_OTG_PKTSTS_1 | USB_OTG_PKTSTS_0)
#define STS_SETUP_COMP          USB_OTG_PKTSTS_2
#define STS_SETUP_UPDT          (USB_OTG_PKTSTS_2 | USB_OTG_PKTSTS_1)

#define EPTYP_CONTROL           0
#define EPTYP_ISO               USB_OTG_DIEPCTL_EPTYP_0
#define EPTYP_BULK              USB_OTG_DIEPCTL_EPTYP_1
#define EPTYP_INT               (USB_OTG_DIEPCTL_EPTYP_0 | USB_OTG_DIEPCTL_EPTYP_1)

#define USB_OTG_DAINTMSK_OEPM_BIT       16
#define USB_OTG_DCFG_DAD_BIT            4
#define USB_OTG_FIFO_SIZE_BIT           16
#define USB_OTG_DOEPTSIZ_PKTCNT_BIT     19
#define USB_OTG_DIEPTSIZ_PKTCNT_BIT     19
#define USB_OTG_DIEPTCTL_TXFNUM_BIT     22
#define USB_OTG_GRSTCTL_TXFNUM_BIT      6
#define USB_OTG_GUSBCFG_TRDR_BIT        10

#define USB_CTRL_PACKET_SIZE 64
#define EPoutput(e)     ((e) << 1)
#define EPinput(e)      (((e) << 1) | 1)

// ====================================
// USB signals
// ------------------------------------
#define USB_PORT        GPIOA
#define USB_DM_BIT      11
#define USB_DP_BIT      12
#define USB_VBUS_PORT   USB_PORT
#define USB_VBUS_BIT    9 // I/O function

#define GPIO_MASK2(m,p) ((m) << (2*(p)))
#define GPIO_MASK4(m,p) ((m) << (4*((p)&0x07)))

enum {CLK_PLLCLK, CLK_USBCLK, CLK_SYSCLK, CLK_FCLK, CLK_HCLK,
      CLK_APB1, CLK_APB2, CLK_APB1TIM, CLK_APB2TIM, CLK_ADCCLK,
      CLK_RTCCLK, CLK_IWDGCLK, CLK_MCO
     };

/** ********************************************************/
static const unsigned Ep0MaxBuffers = 4;
struct Ep0PartBuf {
  uint32_t len;
  uint8_t  buf [64];
};
struct Ep0Buffer {
  volatile uint32_t    ri,wi,le;
  Ep0PartBuf  pb[Ep0MaxBuffers];
};
struct EndpointBuffer {
  uint32_t  len;
};
struct EndPointHepler {
  //EndpointBuffer setup;
  EndpointBuffer eps [MaxPhyEps];
};
static volatile EndPointHepler EpList;
static          Ep0Buffer      Ep0Buf;

static const uint32_t StmEpTypes [] = {
  EPTYP_CONTROL, EPTYP_ISO, EPTYP_BULK, EPTYP_INT
};
/// debug
/*
static GpioClass ledg (GpioPortD, 12);
static GpioClass ledo (GpioPortD, 13);
static GpioClass ledr (GpioPortD, 14);
static GpioClass ledb (GpioPortD, 15);

extern "C" void mdebug (const char * fmt, ...);
extern "C" void mdinit (void);
*/
/*
 *  USB Get EP Command/Status Pointer Function
 *    EPNum.0..3: Address
 *    EPNum.7:    Dir
 *    Parameters:      Logical EP with direction bit
 *    Return Value:    Physical Memory location for EP list command/status
 */
extern "C" void * memcpy (void * dest, const void * src, unsigned n);
// ====================================
// USB wait for AHB master IDLE state
// ------------------------------------

static void usb_wait0 (unsigned mask) {
  volatile unsigned w;
  // wait for AHB master IDLE state
  for (w = 0; ! (USBx->GRSTCTL & mask) && w < 200000; w++);
}

static void usb_wait (unsigned mask) {
  volatile unsigned w;
  // wait for operation finished
  for (w = 0; (USBx->GRSTCTL & mask) && w < 200000; w++);
}

// ====================================
// Endpoint configure / disable
// ------------------------------------
static void usb_setEP (unsigned char epi, unsigned size, unsigned type) {
  if (size) {
    if (! (epi & 1)) {
      // output EP
      epi >>= 1;
      // set FIFO size
      if (!epi) {
        // common receive FIFO; 16..256 words
        USBx->GRXFSIZ = size >> 2;
      }
      // configure EP
      // enable endpoint interrupt
      USBx_DEVICE->DAINTMSK |= (1 << (epi + USB_OTG_DAINTMSK_OEPM_BIT));
      if (!epi) {
#if USB_CTRL_PACKET_SIZE == 8
        type |= 3;
#elif USB_CTRL_PACKET_SIZE == 16
        type |= 2;
#elif USB_CTRL_PACKET_SIZE == 32
        type |= 1;
#elif USB_CTRL_PACKET_SIZE == 64
        type |= 0;
#else
#error "Unsupported control endpoint size!"
#endif
        // set EP type & max packet size
        USBx_OUTEP (0)->DOEPCTL = type;
        // set packet count & transfer size
        USBx_OUTEP (0)->DOEPTSIZ = USB_OTG_DOEPTSIZ_STUPCNT_0 | (1UL << USB_OTG_DOEPTSIZ_PKTCNT_BIT) | USB_CTRL_PACKET_SIZE;
      } else {
        // set EP type & max packet size
        USBx_OUTEP (epi)->DOEPCTL = type | size;
        // set packet count & transfer size
        USBx_OUTEP (epi)->DOEPTSIZ = (1UL << USB_OTG_DOEPTSIZ_PKTCNT_BIT) | size;
        // check if bulk or interrupt EP
        if (type > EPTYP_ISO)
          // EP0 doesn't have this bit
          USBx_OUTEP (epi)->DOEPCTL |= USB_OTG_DOEPCTL_SD0PID_SEVNFRM;
      }
      // enable EP
      // activate EP, enable EP, clear NAK
      USBx_OUTEP (epi)->DOEPCTL |= USB_OTG_DOEPCTL_USBAEP | USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK;
    } else {
      // input EP
      epi >>= 1;
      if (!epi) {
        // EP0 transmit fifo
        USBx->DIEPTXF0_HNPTXFSIZ = ( (size >> 2) << USB_OTG_FIFO_SIZE_BIT) | USBx->GRXFSIZ;
        // EP type & max packet size
#if USB_CTRL_PACKET_SIZE == 8
        type |= 3;
#elif USB_CTRL_PACKET_SIZE == 16
        type |= 2;
#elif USB_CTRL_PACKET_SIZE == 32
        type |= 1;
#elif USB_CTRL_PACKET_SIZE == 64
        type |= 0;
#else
#error "Unsupported control endpoint size!"
#endif
        USBx_INEP (0)->DIEPCTL = (epi << USB_OTG_DIEPTCTL_TXFNUM_BIT) | type;
      } else {
        unsigned ep, i;
        unsigned offs = (USBx->DIEPTXF0_HNPTXFSIZ & USB_OTG_DIEPTXF_INEPTXSA) + (USBx->DIEPTXF0_HNPTXFSIZ >> USB_OTG_FIFO_SIZE_BIT);
        ep = epi - 1;
        for (i = 0; i < ep; i++) {
          offs += (USBx->DIEPTXF[i] >> USB_OTG_FIFO_SIZE_BIT);
        }
        USBx->DIEPTXF[ep] = ( (size >> 2) << USB_OTG_FIFO_SIZE_BIT) | offs;
        // EP type & max packet size
        USBx_INEP (epi)->DIEPCTL = (epi << USB_OTG_DIEPTCTL_TXFNUM_BIT) | type | size;
        // check if bulk or interrupt EP
        if (type > EPTYP_ISO)
          // EP0 doesn't have this bit
          USBx_INEP (epi)->DIEPCTL |= USB_OTG_DIEPCTL_SD0PID_SEVNFRM;
      }
      // configure EP
      // enable endpoint interrupt
      USBx_DEVICE->DAINTMSK |= (1 << epi);
      // enable EP
      // activate EP, enable NAK
      USBx_INEP (epi)->DIEPCTL |= USB_OTG_DIEPCTL_USBAEP | USB_OTG_DIEPCTL_SNAK;
      // if EP is enabled, disable it
      if (USBx_INEP (epi)->DIEPCTL &  USB_OTG_DIEPCTL_EPENA)
          USBx_INEP (epi)->DIEPCTL |= USB_OTG_DIEPCTL_EPDIS;
      // reset EP - flush FIFO (TXFNUM 0x10 for flushing all FIFOs)
      USBx->GRSTCTL = (USBx->GRSTCTL & ~USB_OTG_GRSTCTL_TXFNUM) | (epi << USB_OTG_GRSTCTL_TXFNUM_BIT) | USB_OTG_GRSTCTL_TXFFLSH;
      usb_wait (USB_OTG_GRSTCTL_TXFFLSH);
    }
  } else {
    // disable endpoint
    if (epi & 1) {
      // input EP
      epi >>= 1;
      // if enabled, disable EP
      if (USBx_INEP (epi)->DIEPCTL & USB_OTG_DIEPCTL_EPENA)
        USBx_INEP (epi)->DIEPCTL |= USB_OTG_DIEPCTL_EPDIS;
      // enable NAK
      USBx_INEP (epi)->DIEPCTL |= USB_OTG_DIEPCTL_SNAK;
      // deactivate EP
      USBx_INEP (epi)->DIEPCTL &= ~USB_OTG_DIEPCTL_USBAEP;
    } else {
      // output EP
      epi >>= 1;
      // if enabled, disable EP
      if (USBx_OUTEP (epi)->DOEPCTL & USB_OTG_DOEPCTL_EPENA)
        USBx_OUTEP (epi)->DOEPCTL |= USB_OTG_DOEPCTL_EPDIS;
      // enable NAK
      USBx_OUTEP (epi)->DOEPCTL |= USB_OTG_DOEPCTL_SNAK;
      // deactivate EP
      USBx_OUTEP (epi)->DOEPCTL &= ~USB_OTG_DOEPCTL_USBAEP;
    }
  }
}
// ====================================
// USB endpoint data functions
// ====================================
// read endpoint data
// ------------------------------------
static int usb_readEP (unsigned char epi, unsigned char *buf, const unsigned len, unsigned max) {
  unsigned data = 0;
  for (unsigned i = 0; i < len; i++) {
    if (!(i&3))  data   = USBx_DFIFO (0);
    if (i < max) buf[i] = data & 0xFF;
    data >>= 8;
  }
  // release endpoint fot further communication
  USBx_OUTEP (epi) -> DOEPCTL |= USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK;
  return len;
}
/** ********************************************************/

UsbHw::UsbHw (UsbDevice * p) {
  UsbHwInstance = this;
  parent = p;

  Init ();

}
/// LPC inicializuje RAM pro EP
void UsbHw::USB_EPInit (void) {
}

void UsbHw::IOClkConfig (void) {
  return;
}

/**
Inicializace systému.
*/
void UsbHw::Init (void) {

  //EpList.setup.len = 0;
  for (unsigned i=0; i<MaxPhyEps; i++) {
    EpList.eps[i].len = 0;
  }
  Ep0Buf.ri = 0;
  Ep0Buf.wi = 0;
  Ep0Buf.le = 0;
  /* Initialize clock and I/O pins for USB. */
  IOClkConfig(); // preneseno do Connect ???
  
  //mdinit();

  return;
}
/**
 * Spojení.
@param con 1 = connect
*/
void UsbHw::Connect (uint32_t state) {
  if (state) {
    RCC->AHB1ENR |= RCC_AHB1Periph_GPIOA;
    // DM & DP pins: high-speed, AF10
    USB_PORT->AFR[1] &= ~ (GPIO_MASK4 (15, USB_DM_BIT) | GPIO_MASK4 (15, USB_DP_BIT));
    USB_PORT->AFR[1] |= GPIO_MASK4 (10, USB_DM_BIT) | GPIO_MASK4 (10, USB_DP_BIT);
    USB_PORT->MODER &= ~ (GPIO_MASK2 (3, USB_DM_BIT) | GPIO_MASK2 (3, USB_DP_BIT));
    USB_PORT->MODER |= GPIO_MASK2 (2, USB_DM_BIT) | GPIO_MASK2 (2, USB_DP_BIT);
    //USB_PORT->OSPEEDR &= ~(GPIO_MASK2 (3, USB_DM_BIT) | GPIO_MASK2 (3, USB_DP_BIT));
    USB_PORT->OSPEEDR |= GPIO_MASK2 (3, USB_DM_BIT) | GPIO_MASK2 (3, USB_DP_BIT);
    USB_PORT->PUPDR &= ~ (GPIO_MASK2 (3, USB_DM_BIT) | GPIO_MASK2 (3, USB_DP_BIT));
    // Vbus pin: input
    USB_PORT->MODER &= ~ (GPIO_MASK2 (3, USB_VBUS_BIT));
    //USB_PORT->MODER |= GPIO_MASK2 (0, USB_VBUS_BIT);
    //USB_PORT->OSPEEDR &= ~(GPIO_MASK2 (3, USB_VBUS_BIT));
    //USB_PORT->OSPEEDR |= GPIO_MASK2 (3, USB_VBUS_BIT);
    USB_PORT->PUPDR &= ~ (GPIO_MASK2 (3, USB_VBUS_BIT));
#ifdef USB_ID_BIT
    // ID line: high-speed, pull-up, AF10
    USB_PORT->MODER &= ~ (GPIO_MASK2 (3, USB_ID_BIT));
    USB_PORT->MODER |= GPIO_MASK2 (2, USB_ID_BIT);
    //USB_PORT->OSPEEDR &= ~(GPIO_MASK2 (3, USB_ID_BIT));
    USB_PORT->OSPEEDR |= GPIO_MASK2 (3, USB_ID_BIT);
    USB_PORT->PUPDR &= ~ (GPIO_MASK2 (3, USB_ID_BIT));
    USB_PORT->PUPDR |= GPIO_MASK2 (1, USB_ID_BIT);
    USB_PORT->AFR[1] &= ~ (GPIO_MASK4 (15, USB_ID_BIT));
    USB_PORT->AFR[1] |= GPIO_MASK4 (10, USB_ID_BIT);
#endif
    // enable USB FS clocks
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
    // USB periphery reset
    RCC->AHB2RSTR  |=  RCC_AHB2RSTR_OTGFSRST;
    RCC->AHB2RSTR  &= ~RCC_AHB2RSTR_OTGFSRST;

    // USB core soft reset
    usb_wait0 (USB_OTG_GRSTCTL_AHBIDL);
    USBx->GRSTCTL |= USB_OTG_GRSTCTL_CSRST;
    usb_wait (USB_OTG_GRSTCTL_CSRST);

    // set USB Device mode, select internal PHY, turn-around time
    USBx->GUSBCFG = USB_OTG_GUSBCFG_FDMOD | USB_OTG_GUSBCFG_PHYSEL |
                    (5 << USB_OTG_GUSBCFG_TRDR_BIT);
    //((((get_clk (CLK_HCLK) * 4) / get_clk (CLK_USBCLK)) + 1) << USB_OTG_GUSBCFG_TRDR_BIT);
    usb_wait0 (USB_OTG_GRSTCTL_AHBIDL);

    // set USB interrupt priority
    NVIC->IP[OTG_FS_IRQn] = 0x80;
    // enable USB interrupt channel
    NVIC->ISER[OTG_FS_IRQn >> 5] = (1 << (OTG_FS_IRQn & 0x1F));
    // select TX FIFO empty interrupt; global USB interrupt enable
    USBx->GAHBCFG |= USB_OTG_GAHBCFG_GINT | USB_OTG_GAHBCFG_TXFELVL;
    // select full-speed
    USBx_DEVICE->DCFG |= USB_OTG_SPEED_FULL;
    // enable USB module clocks
    USBx_PCGCCTL = 0;
    // enable USB interrupts
    USBx->GINTMSK = USB_OTG_GINTMSK_USBSUSPM | // suspend
                    USB_OTG_GINTMSK_USBRST   | // reset
                    USB_OTG_GINTMSK_ENUMDNEM | // enumeration done
                    USB_OTG_GINTMSK_RXFLVLM  | // receive fifo non-empty
                    USB_OTG_GINTMSK_IEPINT   | // IN EP
                    USB_OTG_GINTMSK_OEPINT   | // OUT EP
                    USB_OTG_GINTMSK_WUIM;      // resume

    // set VBUS sensing; deactivate powerdown mode
    USBx->GCCFG = USB_OTG_GCCFG_NOVBUSSENS | USB_OTG_GCCFG_PWRDWN;
    // disable soft-disconnect
    USBx_DEVICE->DCTL &= ~USB_OTG_DCTL_SDIS;

    //USB_connect (0)
    // soft disconnect enabled
    //USBx_DEVICE->DCTL |= USB_OTG_DCTL_SDIS;
  } else {
    // USB stop
    // enable soft-disconnect
    USBx_DEVICE->DCTL |= USB_OTG_DCTL_SDIS;
    // reset VBUS sensing; activate powerdown mode
    USBx->GCCFG = 0;
    // disable USB interrupts
    USBx->GINTMSK = 0;
    // disable USB module clocks
    USBx_PCGCCTL = USB_OTG_PCGCCTL_STOPCLK | USB_OTG_PCGCCTL_GATECLK;
    // select TX FIFO empty interrupt; global USB interrupt enable
    USBx->GAHBCFG = 0;
    // disable USB interrupt channel
    NVIC->ICER[OTG_FS_IRQn >> 5] = (1 << (OTG_FS_IRQn & 0x1F));
    // reset USB Device mode
    USBx->GUSBCFG = 0;
    // USB core soft reset
    USBx->GRSTCTL  |= USB_OTG_GRSTCTL_CSRST;
    // USB periphery reset
    RCC->AHB2RSTR  |=  RCC_AHB2RSTR_OTGFSRST;
    RCC->AHB2RSTR  &= ~RCC_AHB2RSTR_OTGFSRST;
    // disable USB FS clocks
    RCC->AHB2ENR &= ~RCC_AHB2ENR_OTGFSEN;
#ifdef USB_ID_BIT
    // ID line: high-speed, pull-up, AF10
    USB_PORT->MODER &= ~ (GPIO_MASK2 (3, USB_ID_BIT));
    USB_PORT->PUPDR &= ~ (GPIO_MASK2 (3, USB_ID_BIT));
    USB_PORT->PUPDR |=   GPIO_MASK2 (2, USB_ID_BIT); // pull-down
#endif
    USB_PORT->MODER &= ~ (GPIO_MASK2 (3, USB_DM_BIT) | GPIO_MASK2 (3, USB_DP_BIT));
    USB_PORT->PUPDR &= ~ (GPIO_MASK2 (3, USB_DM_BIT) | GPIO_MASK2 (3, USB_DP_BIT));
    USB_PORT->PUPDR |= (GPIO_MASK2 (2, USB_DM_BIT) | GPIO_MASK2 (2, USB_DP_BIT));  // pull-down
    // Vbus pin: input
    USB_PORT->MODER &= ~ (GPIO_MASK2 (3, USB_VBUS_BIT));
    USB_PORT->PUPDR &= ~ (GPIO_MASK2 (3, USB_VBUS_BIT));
  }
  return;
}
void UsbHw::Reset (void) {
  unsigned long i;
  // disable all endpoints & enable NAK reply
  for (i = 0; i < MaxPhyEps; i++) {
    if (USBx_OUTEP (i)->DOEPCTL & USB_OTG_DOEPCTL_EPENA)
      USBx_OUTEP (i)->DOEPCTL = USB_OTG_DOEPCTL_EPDIS | USB_OTG_DOEPCTL_SNAK;
    if (USBx_INEP (i)->DIEPCTL & USB_OTG_DIEPCTL_EPENA)
      USBx_INEP (i)->DIEPCTL = USB_OTG_DIEPCTL_EPDIS | USB_OTG_DIEPCTL_SNAK;
  }
  // reset USB address
  SetAddress (0);
  // enable interrupts for output EPs
  USBx_DEVICE->DOEPMSK = USB_OTG_DOEPMSK_STUPM | USB_OTG_DOEPMSK_XFRCM;
  // enable interrupts for input EPs
  USBx_DEVICE->DIEPMSK = USB_OTG_DIEPMSK_XFRCM;

  // configure control endpoints
  usb_setEP (EPoutput (0), 512, EPTYP_CONTROL);  // size for RX FIFO
  usb_setEP (EPinput  (0), 64,  EPTYP_CONTROL);  // max control packet size
}
void UsbHw::SetAddress (uint32_t adr) {
  USBx_DEVICE->DCFG = (USBx_DEVICE->DCFG & ~USB_OTG_DCFG_DAD) | (adr << USB_OTG_DCFG_DAD_BIT);
}

void UsbHw::Configure (uint32_t cfg) {
}
uint32_t UsbHw::ReadSetupEP (uint8_t * pData) {
  uint32_t len  = sizeof (USB_SETUP_PACKET);
  // setup size must be 8
  usb_readEP (0, pData, len, USB_CTRL_PACKET_SIZE);
  // EpList.setup.len -= len;
  return len;
}

void UsbHw::WakeUp (void) {
}
void UsbHw::Cleanup (void) {
}
int UsbHw::MainLoop (void) {
  return 0;
}
/** ********************************************************************/

void UsbHw::Ep0OutHandler (uint32_t len) {
  uint8_t * pData = Ep0Buf.pb[Ep0Buf.wi].buf;
  len = usb_readEP (0, pData, len, USB_CTRL_PACKET_SIZE);
  if (len) {
    Ep0Buf.pb[Ep0Buf.wi++].len = len;
    Ep0Buf.wi %= Ep0MaxBuffers;
    Ep0Buf.le += 1;
  }
  parent->Ep0Handler(USB_EVT_OUT);
  
  if (Ep0Buf.le) {
    eps[0]->Read(pData);
  }
}
/** ********************************************************/
void UsbEndpoint::DeactivateEP (UsbEndpointDir dir) {
}
void UsbEndpoint::ResetEP (UsbEndpointDir dir) {
}
void UsbEndpoint::EnableEP (UsbEndpointDir dir) {
  // mdebug("EnableEp %d-%d-%d ", addr, dir, type);
  if (!addr) return;
  uint32_t size = 0x40;
  switch (type) {
    case EndpointTypeCtrl:
      break;
    case EndpointTypeIntr:
      size = 0x10;
      break;
//  case EndpointTypeIso : size = 0x200; break; // ???
    default:
      break;
  }
  usb_setEP ( ( (addr<<1) | (uint32_t) dir), size, StmEpTypes[type]);
}
void UsbEndpoint::DisableEP (UsbEndpointDir dir) {
  if (!addr) return;
  usb_setEP ( ( (addr<<1) | (uint32_t) dir), 0, 0);
}
void UsbEndpoint::SetStallEP (UsbEndpointDir dir) {
  /// asm volatile ("bkpt 0"); // TODO pro MSC asi potreba
  if (dir == EndpointIn) {	// in EP
    USBx_INEP  (addr)->DIEPCTL |= USB_OTG_DIEPCTL_STALL;
  } else {			// out EP
    USBx_OUTEP (addr)->DOEPCTL |= USB_OTG_DOEPCTL_STALL;
  }
}
void UsbEndpoint::ClrStallEP (UsbEndpointDir dir) {
  /// asm volatile ("bkpt 0"); // TODO
  if (dir == EndpointIn) {	// in EP
    USBx_INEP  (addr)->DIEPCTL &= ~ USB_OTG_DIEPCTL_STALL;
  } else {			// out EP
    USBx_OUTEP (addr)->DOEPCTL &= ~ USB_OTG_DOEPCTL_STALL;
  }
}

/** ********************************************************************/
uint32_t UsbEndpoint::Read (uint8_t * pData) {
  if (addr) {
    uint32_t len  = EpList.eps[addr].len;
    if (!len) return 0;
    usb_readEP ((addr), pData, len, USB_CTRL_PACKET_SIZE);
    EpList.eps[addr].len -= len;
    return len;
  } else { // Endpoint 0 to bude muset mit nejak neprimo
    if (!Ep0Buf.le) return 0; // neni co cist
    uint8_t * tbuf = Ep0Buf.pb[Ep0Buf.ri  ].buf;
    uint32_t  len  = Ep0Buf.pb[Ep0Buf.ri++].len;
    Ep0Buf.ri %= Ep0MaxBuffers;
    Ep0Buf.le -= 1;
    //for (uint32_t i=0; i<len; i++) mdebug ("[%02X]", tbuf[i]);
    memcpy (pData, tbuf, len);
    return len;
  }
}
uint32_t UsbEndpoint::Write (uint8_t * pData, uint32_t cnt) {
  __IO uint32_t * fifo;
  uint32_t res = cnt;
  USBx_INEP (addr) -> DIEPTSIZ = (USBx_INEP (addr) -> DIEPTSIZ & ~ (USB_OTG_DIEPTSIZ_XFRSIZ
                                  |  USB_OTG_DIEPTSIZ_PKTCNT)) | (1 << USB_OTG_DIEPTSIZ_PKTCNT_BIT) | cnt;
  USBx_INEP (addr) -> DIEPCTL |= USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_CNAK;
  fifo = &USBx_DFIFO (addr);
  while (cnt) {
    int i;
    unsigned long data = 0;
    for (i = 0; i < 32 && cnt; i+=8, cnt--) {
      data |= (*pData++) << i;
    }
    *fifo = data;
  }
  return res;
}
/** *****************************************************************************/

uint32_t UsbHw::GetFrame (void) {
  return 0;
}


extern "C" void OTG_FS_IRQHandler (void) {
  // Wraper pro C++ aby nebylo nutno patlat se statickými daty.
  UsbHwInstance->Handler ();
}

void UsbHw::Handler (void) {
  // check interrupt flags
  if ((USBx->GINTSTS & USBx->GINTMSK & USB_OTG_GINTSTS_MMIS)) {
    // mode mismatch interrupt
    USBx->GINTSTS = USB_OTG_GINTSTS_MMIS;
  }
  while ((USBx->GINTSTS & USBx->GINTMSK & USB_OTG_GINTSTS_RXFLVL)) {
    // read Rx FIFO data - OUT endpoints only...
    unsigned w  = USBx->GRXSTSP;
    unsigned ll = (w & USB_OTG_GRXSTSP_BCNT) >> 4;
    if ((w & USB_OTG_GRXSTSP_PKTSTS) == (STS_SETUP_UPDT)) {
      // SETUP packet
      // if (ll != sizeof(USB_SETUP_PACKET)) +ledr;
      // EpList.setup.len += ll;
      parent->Ep0Handler (USB_EVT_SETUP);
      //usb_setup0 (ll);
      continue;
    } else if ((w & USB_OTG_GRXSTSP_PKTSTS) == (STS_DATA_UPDT)) {
      // DATA packet
      uint32_t ep = w & USB_OTG_GRXSTSP_EPNUM;
      EpList.eps[ep].len += ll;
      if (ep) eps[ep]->Handler  (USB_EVT_OUT);
      else       Ep0OutHandler  (ll);
      // usb_output (EPoutput (w & USB_OTG_GRXSTSP_EPNUM), ll/*, &rq*/);
    }
  }

  if ((USBx->GINTSTS & USBx->GINTMSK & USB_OTG_GINTSTS_OEPINT)) {
    // output endpoint interrupt
    unsigned w;
    unsigned char ep;
    w = (USBx_DEVICE->DAINT & USBx_DEVICE->DAINTMSK) >> USB_OTG_DAINTMSK_OEPM_BIT;
    for (ep = 0; w; ep++, w >>= 1) {
      unsigned u;
      u = USBx_OUTEP (ep)->DOEPINT & USBx_DEVICE->DOEPMSK;
      // ZIT tady ma zpracovani dat EP0 nactenych pri RXFLVL
      /// if (!ep) ~ledg; /// neco tady je
      // if (!ep)  parent->Ep0Handler(USB_EVT_OUT); /// ???
      // zkusim jen odblokovat preruseni...
      if (u & USB_OTG_DOEPINT_XFRC)
        USBx_OUTEP (ep) -> DOEPINT = USB_OTG_DOEPINT_XFRC;
      if (u & USB_OTG_DOEPINT_STUP)
        USBx_OUTEP (ep) -> DOEPINT = USB_OTG_DOEPINT_STUP;
      if (u & USB_OTG_DOEPINT_OTEPDIS)
        USBx_OUTEP (ep) -> DOEPINT = USB_OTG_DOEPINT_OTEPDIS;
      // ZIT zapisuje do DOEPTSIZ a DOEPCTL...
      //USBx_OUTEP (ep) -> DOEPCTL |= USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK;
      // -> to uz je ve funkci usb_readEP ()
    }
  }
  if ((USBx->GINTSTS & USBx->GINTMSK & USB_OTG_GINTSTS_IEPINT)) {
    // input endpoint interrupt
    unsigned w;
    unsigned char ep;
    w = (USBx_DEVICE->DAINT & USBx_DEVICE->DAINTMSK) & 0xFFFF;
    for (ep = 0; w; ep++, w >>= 1) {
      unsigned u;
      u = USBx_INEP (ep)->DIEPINT & USBx_DEVICE->DIEPMSK;
      if (u & USB_OTG_DIEPINT_XFRC) {
        if (!ep) parent->Ep0Handler(USB_EVT_IN);
          //usb_input0 ();
        else    eps[ep]->Handler   (USB_EVT_IN);
        //  usb_input (EPinput (ep));
        USBx_INEP (ep)->DIEPINT = USB_OTG_DIEPINT_XFRC;
      }
    }
  }
  if ((USBx->GINTSTS & USBx->GINTMSK & USB_OTG_GINTSTS_WKUINT)) {
    // Resume interrupt
    // clear remote wake-up signaling
    USBx_DEVICE->DCTL &= ~USB_OTG_DCTL_RWUSIG;
    USBx->GINTSTS = USB_OTG_GINTSTS_WKUINT;
  }
  if ((USBx->GINTSTS & USBx->GINTMSK & USB_OTG_GINTSTS_USBSUSP)) {
    // Suspend interrupt
    if (USBx_DEVICE->DSTS & USB_OTG_DSTS_SUSPSTS)
      ;
    USBx->GINTSTS = USB_OTG_GINTSTS_USBSUSP;
  }
  if ((USBx->GINTSTS & USBx->GINTMSK & USB_OTG_GINTSTS_USBRST)) {
    // Reset interrupt
    Reset ();
    USBx->GINTSTS = USB_OTG_GINTSTS_USBRST;
  }
  if ((USBx->GINTSTS & USBx->GINTMSK & USB_OTG_GINTSTS_ENUMDNE)) {
    // Enumeration done interrupt
    // USB_ActivateSetup
    USBx_INEP (0)->DIEPCTL &= ~USB_OTG_DIEPCTL_MPSIZ;
    USBx_DEVICE->DCTL |= USB_OTG_DCTL_CGINAK;
    USBx_DEVICE->DCTL |= USB_OTG_DCTL_CGONAK;
    //
    USBx->GINTSTS = USB_OTG_GINTSTS_ENUMDNE;
  }
  if ((USBx->GINTSTS & USBx->GINTMSK & USB_OTG_GINTSTS_SOF)) {
    // SOF interrupt
    USBx->GINTSTS = USB_OTG_GINTSTS_SOF;
  }
  if ((USBx->GINTSTS & USBx->GINTMSK & USB_OTG_GINTSTS_IISOIXFR)) {
    // Incomplete ISO IN interrupt
    USBx->GINTSTS = USB_OTG_GINTSTS_IISOIXFR;
  }
  if ((USBx->GINTSTS & USBx->GINTMSK & USB_OTG_GINTSTS_PXFR_INCOMPISOOUT)) {
    // Incomplete ISO OUT interrupt
    USBx->GINTSTS = USB_OTG_GINTSTS_PXFR_INCOMPISOOUT;
  }
  if ((USBx->GINTSTS & USBx->GINTMSK & USB_OTG_GINTSTS_SRQINT)) {
    // Connection event interrupt
    USBx->GINTSTS = USB_OTG_GINTSTS_SRQINT;
  }
  if ((USBx->GINTSTS & USBx->GINTMSK & USB_OTG_GINTSTS_OTGINT)) {
    // Disconnection event interrupt
    unsigned w;
    w = USBx->GOTGINT;
    if (w & USB_OTG_GOTGINT_SEDET)
      // disconnect & class de-init callback
      ;
    USBx->GOTGINT = w;
  }
}

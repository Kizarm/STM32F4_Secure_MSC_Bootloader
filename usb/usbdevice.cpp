#include <stdio.h>
#include <stdlib.h>
#include "usbdevice.h"
#include "dscbld.h"
#include "debug.h"
/*
static void printstruct (const char * id, const void * s, unsigned l) {
  const unsigned char * p = (const unsigned char *) s;
  printf ("%s {%d} : ", id, l);
  for (unsigned i=0; i<l; i++) {
    printf ("[%02X]", p[i]);
  }
  printf ("\n");
}
*/
extern "C" void mdebug (const char * fmt, ...);

    volatile int UsbIsConfigured = 0;

UsbDevice::UsbDevice() : UsbHw (this), if0 () {
  if0.ep.set (EndpointTypeCtrl);
  eps [0] = & if0.ep;
  useeps  = 1;
  useifs  = 0;
  list = NULL;
}
void UsbDevice::Start (void) {
  if (UsbIsConfigured) return;
  Connect (1);
  while (!UsbIsConfigured);
}

void UsbDevice::addClass (UsbClass * p) {
  if (list) list->append (p);
  else list = p;
}
void UsbDevice::buildDev (void) {
  unsigned k = useeps, l = useifs;
  for (UsbClass * n = list; n; n = n->next) {
    for (UsbInterface * i = n->list; i; i = i->next) {
      eps [k] = & i->ep;
      i->ep.addr = k++;
      ifs [l++]  = i;
    }
  }
  useeps = k;
  useifs = l;
}
void UsbDevice::ResetCore (void) {
  USB_DeviceStatus  = USB_POWER;
  USB_DeviceAddress = 0;
  USB_Configuration = 0;
  USB_EndPointMask  = 0x00010001;
  USB_EndPointHalt  = 0x00000000;
  USB_EndPointStall = 0x00000000;
}
void UsbDevice::SetupStage (void) {
#ifdef __arm__
  ReadSetupEP ((uint8_t *) &SetupPacket);
#else
  if0.ep.Read ((uint8_t *) &SetupPacket);
#endif
}
void UsbDevice::DataInStage (void) {
  uint32_t cnt;

  if (EP0Data.Count > USB_MAX_PACKET0) {
    cnt = USB_MAX_PACKET0;
  } else {
    cnt = EP0Data.Count;
  }
  if (!cnt) return;
  cnt = if0.ep.Write (EP0Data.pData, cnt);
  EP0Data.pData += cnt;
  EP0Data.Count -= cnt;
}
void UsbDevice::DataOutStage (void) {
  uint32_t cnt;

  cnt = if0.ep.Read (EP0Data.pData);
  EP0Data.pData += cnt;
  EP0Data.Count -= cnt;
}
void UsbDevice::StatusInStage (void) {
  if0.ep.Write (NULL, 0);
}
void UsbDevice::StatusOutStage (void) {
  if0.ep.Read (EP0Buf.b);
}
uint32_t UsbDevice::ReqGetStatus (void) {
  uint32_t n, m;

  switch (SetupPacket.bmRequestType.BM.Recipient) {
    case REQUEST_TO_DEVICE:
      EP0Data.pData = (uint8_t *) &USB_DeviceStatus;
      break;
    case REQUEST_TO_INTERFACE:
      if ( (USB_Configuration != 0) && (SetupPacket.wIndex.WB.L < useifs)) {
        *EP0Buf.w = 0;
        EP0Data.pData = EP0Buf.b;
      } else {
        return (FALSE);
      }
      break;
    case REQUEST_TO_ENDPOINT:
      n = SetupPacket.wIndex.WB.L & 0x8F;
      m = (n & 0x80) ? ( (1 << 16) << (n & 0x0F)) : (1 << n);
      if ( ( (USB_Configuration != 0) || ( (n & 0x0F) == 0)) && (USB_EndPointMask & m)) {
        *EP0Buf.w = (USB_EndPointHalt & m) ? 1 : 0;
        EP0Data.pData = EP0Buf.b;
      } else {
        return (FALSE);
      }
      break;
    default:
      return (FALSE);
  }
  return (TRUE);

}
uint32_t UsbDevice::ReqSetClrFeature (uint32_t sc) {
  uint32_t n, m;

  switch (SetupPacket.bmRequestType.BM.Recipient) {
    case REQUEST_TO_DEVICE:
      if (SetupPacket.wValue.W == USB_FEATURE_REMOTE_WAKEUP) {
        if (sc) {
          //WakeUpCfg (TRUE);
          USB_DeviceStatus |=  USB_GETSTATUS_REMOTE_WAKEUP;
        } else {
          //WakeUpCfg (FALSE);
          USB_DeviceStatus &= ~USB_GETSTATUS_REMOTE_WAKEUP;
        }
      } else {
        return (FALSE);
      }
      break;
    case REQUEST_TO_INTERFACE:
      return (FALSE);
    case REQUEST_TO_ENDPOINT:
      n = SetupPacket.wIndex.WB.L & 0x8F;
      m = (n & 0x80) ? ( (1 << 16) << (n & 0x0F)) : (1 << n);
      if ( (USB_Configuration != 0) && ( (n & 0x0F) != 0) && (USB_EndPointMask & m)) {
        if (SetupPacket.wValue.W == USB_FEATURE_ENDPOINT_STALL) {
	  UsbEndpointDir dir;
	  if (n & 0x80)  dir = EndpointIn;
	  else           dir = EndpointOut;
	  n &= 0x0F;
          if (sc) {
            eps[n]->SetStallEP (dir);
            USB_EndPointHalt |=  m;
          } else {
            if ( (USB_EndPointStall & m) != 0) {
              return (TRUE);
            }
            eps[n]->ClrStallEP (dir);
            USB_EndPointHalt &= ~m;
          }
        } else {
          return (FALSE);
        }
      } else {
        return (FALSE);
      }
      break;
    default:
      return (FALSE);
  }
  return (TRUE);
}
uint32_t UsbDevice::ReqSetAddress (void) {
  switch (SetupPacket.bmRequestType.BM.Recipient) {
    case REQUEST_TO_DEVICE:
      USB_DeviceAddress = 0x80 | SetupPacket.wValue.WB.L;
      SetHwAddress (SetupPacket.wValue.WB.L);
      break;
    default:
      return (FALSE);
  }
  return (TRUE);
}
uint32_t UsbDevice::ReqGetDescriptor (void) {
  const uint8_t * pD;
  uint32_t len, n;

  // if (SetupPacket.wLength != 64) asm volatile ("bkpt 0");
  switch (SetupPacket.bmRequestType.BM.Recipient) {
    case REQUEST_TO_DEVICE:
      switch (SetupPacket.wValue.WB.H) {
        case USB_DEVICE_DESCRIPTOR_TYPE:
          EP0Data.pData = (uint8_t *) UsbDeviceDescriptor;
          len = UsbDeviceDescriptorLen;
          break;
        case USB_CONFIGURATION_DESCRIPTOR_TYPE:
          pD =  UsbConfigDescriptor;
          for (n = 0; n != SetupPacket.wValue.WB.L; n++) {
            if ( ( (usb_config_descriptor *) pD)->bLength != 0) {
              pD += ( (usb_config_descriptor *) pD)->wTotalLength.get();
            }
          }
          if ( ( (usb_config_descriptor *) pD)->bLength == 0) {
            return (FALSE);
          }
          EP0Data.pData = (uint8_t *) pD;
          len = ( (usb_config_descriptor *) pD)->wTotalLength.get();
          break;
        case USB_STRING_DESCRIPTOR_TYPE:
          pD =  UsbStringDescriptor;
	  print ("String n=%d\n",SetupPacket.wValue.WB.L);
          for (n = 0; n != SetupPacket.wValue.WB.L; n++) {
            if ( ( (USB_STRING_DESCRIPTOR *) pD)->bLength != 0) {
              pD += ( (USB_STRING_DESCRIPTOR *) pD)->bLength;
            }
          }
          if ( ( (USB_STRING_DESCRIPTOR *) pD)->bLength == 0) {
            return (FALSE);
          }
          EP0Data.pData = (uint8_t *) pD;
          len = ( (USB_STRING_DESCRIPTOR *) EP0Data.pData)->bLength;
          break;
        case USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE:
          /* USB Chapter 9. page 9.6.2 */
          /* This is a tricky one again. For USBCV compliance test,
          if we support LPM, bcdUSB should be 0x0201, the USBCV will ask
          for device qualifier info. If the device doesn't reply(return false),
          USBCV will fail. However, if the device replies, on the WIN XP system,
          the host would think it's a HS device attached. */
          pD = UsbDeviceDescriptor;
          //EP0Data.pData = (uint8_t *) USB_DeviceQualifier;    // TODO !!!
          len = sizeof (USB_DEVICE_QUALIFIER_DESCRIPTOR);
          break;
        default:
          return (FALSE);
      }
      break;
    case REQUEST_TO_INTERFACE:
      switch (SetupPacket.wValue.WB.H) {
        default:
          return (FALSE);
      }
      break;
    default:
      return (FALSE);
  }

  if (EP0Data.Count > len) {
    EP0Data.Count = len;
  }

  return (TRUE);
}
uint32_t UsbDevice::ReqGetConfiguration (void) {
  switch (SetupPacket.bmRequestType.BM.Recipient) {
    case REQUEST_TO_DEVICE:
      EP0Data.pData = &USB_Configuration;
      break;
    default:
      return (FALSE);
  }
  return (TRUE);
}
static inline void * AddOffset (const void * p, uint32_t o) {
  uint8_t * op = (uint8_t *) p;
  op += o;
  return (void *) op;
}
uint32_t UsbDevice::ReqSetConfiguration (void) {
  usb_config_descriptor * pD;
  uint32_t alt = 0;
  uint32_t n, m;

  switch (SetupPacket.bmRequestType.BM.Recipient) {
    case REQUEST_TO_DEVICE:

      if (SetupPacket.wValue.WB.L) {

        pD = (usb_config_descriptor *) UsbConfigDescriptor;

        while (pD->bLength) {
          switch (pD->bDescriptorType) {
            case USB_CONFIGURATION_DESCRIPTOR_TYPE:

              if (pD->bConfigurationValue == SetupPacket.wValue.WB.L) {
                USB_Configuration = SetupPacket.wValue.WB.L;
                useifs = pD->bNumInterfaces;
                for (n = 0; n < MaxIfaces; n++) {
                  USB_AltSetting[n] = 0;
                }
                for (n = 1; n < MaxPhyEps; n++) {
                  if (USB_EndPointMask & (1 << n)) {
                    eps[n]->DisableEP (EndpointOut);
                  }
                  if (USB_EndPointMask & ( (1 << 16) << n)) {
                    eps[n]->DisableEP (EndpointIn);
                  }
                }
                USB_EndPointMask = 0x00010001;
                USB_EndPointHalt = 0x00000000;
                USB_EndPointStall= 0x00000000;
                Configure (TRUE);
                UsbIsConfigured = 1;
                if (pD->bmAttributes & USB_CONFIG_POWERED_MASK) {
                  USB_DeviceStatus |=  USB_GETSTATUS_SELF_POWERED;
                } else {
                  USB_DeviceStatus &= ~USB_GETSTATUS_SELF_POWERED;
                }
              } else {
                pD = (usb_config_descriptor *) AddOffset (pD, pD->wTotalLength.get());
                continue;
              }
              break;
            case USB_INTERFACE_DESCRIPTOR_TYPE:
              alt = ( (usb_interface_descriptor *) pD)->bAlternateSetting;
              break;
            case USB_ENDPOINT_DESCRIPTOR_TYPE:
              if (alt == 0) {
                n = ( (usb_endpoint_descriptor *) pD)->bEndpointAddress.w & 0x8F;
                m = (n & 0x80) ? ( (1 << 16) << (n & 0x0F)) : (1 << n);
                USB_EndPointMask |= m;
                ConfigEP ( (usb_endpoint_descriptor *) pD);
		UsbEndpointDir dir;
		if (n & 0x80) dir = EndpointIn;
		else          dir = EndpointOut; 
		n &= 0x0F;
                eps[n]->EnableEP (dir);
                eps[n]->ResetEP  (dir);
              }
              break;
          }
          //(uint8_t *) pD += pD->bLength;
          pD = (usb_config_descriptor *) AddOffset (pD, pD->bLength);
        }
      } else {
        USB_Configuration = 0;
        for (n = 1; n < MaxPhyEps; n++) {
          if (USB_EndPointMask & (1 << n)) {
            eps[n]->DisableEP (EndpointOut);
          }
          if (USB_EndPointMask & ( (1 << 16) << n)) {
            eps[n]->DisableEP (EndpointIn);
          }
        }
        USB_EndPointMask  = 0x00010001;
        USB_EndPointHalt  = 0x00000000;
        USB_EndPointStall = 0x00000000;
        Configure (FALSE);
      }

      if (USB_Configuration != SetupPacket.wValue.WB.L) {
        return (FALSE);
      }
      break;
    default:
      return (FALSE);
  }
  return (TRUE);
}
uint32_t UsbDevice::ReqGetInterface (void) {
  switch (SetupPacket.bmRequestType.BM.Recipient) {
    case REQUEST_TO_INTERFACE:
      if ( (USB_Configuration != 0) && (SetupPacket.wIndex.WB.L < useifs)) {
        EP0Data.pData = USB_AltSetting + SetupPacket.wIndex.WB.L;
      } else {
        return (FALSE);
      }
      break;
    default:
      return (FALSE);
  }
  return (TRUE);
}
uint32_t UsbDevice::ReqSetInterface (void) {
  USB_COMMON_DESCRIPTOR * pD;
  uint32_t ifn = 0, alt = 0, old = 0, msk = 0;
  uint32_t n, m;
  uint32_t set;

  switch (SetupPacket.bmRequestType.BM.Recipient) {
    case REQUEST_TO_INTERFACE:
      if (USB_Configuration == 0) return (FALSE);
      set = FALSE;
      pD  = (USB_COMMON_DESCRIPTOR *) UsbConfigDescriptor;
      while (pD->bLength) {
        switch (pD->bDescriptorType) {
          case USB_CONFIGURATION_DESCRIPTOR_TYPE:
            if ( ( (usb_config_descriptor *) pD)->bConfigurationValue != USB_Configuration) {
              pD = (USB_COMMON_DESCRIPTOR *) AddOffset (pD, ( (usb_config_descriptor *) pD)->wTotalLength.get());
              continue;
            }
            break;
          case USB_INTERFACE_DESCRIPTOR_TYPE:
            ifn = ( (usb_interface_descriptor *) pD)->bInterfaceNumber;
            alt = ( (usb_interface_descriptor *) pD)->bAlternateSetting;
            msk = 0;
            if ( (ifn == SetupPacket.wIndex.WB.L) && (alt == SetupPacket.wValue.WB.L)) {
              set = TRUE;
              old = USB_AltSetting[ifn];
              USB_AltSetting[ifn] = (uint8_t) alt;
            }
            break;
          case USB_ENDPOINT_DESCRIPTOR_TYPE:
            if (ifn == SetupPacket.wIndex.WB.L) {
              n = ( (usb_endpoint_descriptor *) pD)->bEndpointAddress.w & 0x8F;
              m = (n & 0x80) ? ( (1 << 16) << (n & 0x0F)) : (1 << n);
	      UsbEndpointDir dir;
	      if (n & 0x80) dir = EndpointIn;
	      else          dir = EndpointOut; 
	      n &= 0x0F;
              if (alt == SetupPacket.wValue.WB.L) {
                USB_EndPointMask |=  m;
                USB_EndPointHalt &= ~m;
                ConfigEP ( (usb_endpoint_descriptor *) pD);
                eps[n]->EnableEP (dir);
                eps[n]->ResetEP  (dir);
                msk |= m;
              } else if ( (alt == old) && ( (msk & m) == 0)) {
                USB_EndPointMask &= ~m;
                USB_EndPointHalt &= ~m;
                eps[n]->DisableEP (dir);
              }
            }
            break;
        }
        //(uint8_t *) pD += pD->bLength;
        pD = (USB_COMMON_DESCRIPTOR *) AddOffset (pD, pD->bLength);
      }
      break;
    default:
      return (FALSE);
  }

  return (set);
}

/** ************************************************************************/

void UsbDevice::Ep0Handler (EndpointCallbackEvents event) {
  switch (event) {
    case USB_EVT_SETUP:
      Ep0EventSetup ();
      break;  /* end case USB_EVT_SETUP */

    case USB_EVT_OUT:
      Ep0EventOut ();
      break;  /* end case USB_EVT_OUT */

    case USB_EVT_IN :
      Ep0EventIn ();
      break;  /* end case USB_EVT_IN */

    case USB_EVT_OUT_STALL:
      if0.ep.ClrStallEP (EndpointOut);
      break;

    case USB_EVT_IN_STALL:
      if0.ep.ClrStallEP (EndpointIn);
      break;
    default:
      break;

  }
}

void UsbDevice::Ep0EventSetup (void) {
  SetupStage();
  // DirCtrlEP (SetupPacket.bmRequestType.BM.Dir);
  EP0Data.Count = SetupPacket.wLength;     /* Number of bytes to transfer */
  switch (SetupPacket.bmRequestType.BM.Type) {

    case REQUEST_STANDARD:
      switch (SetupPacket.bRequest) {
        case USB_REQUEST_GET_STATUS:
          if (!ReqGetStatus())       break;
          DataInStage();
          return;

        case USB_REQUEST_CLEAR_FEATURE:
          if (!ReqSetClrFeature (0)) break;
          StatusInStage();
          return;

        case USB_REQUEST_SET_FEATURE:
          if (!ReqSetClrFeature (1)) break;
          StatusInStage();
          return;

        case USB_REQUEST_SET_ADDRESS:
          if (!ReqSetAddress())      break;
          StatusInStage();
          return;

        case USB_REQUEST_GET_DESCRIPTOR:
          if (!ReqGetDescriptor())   break;
          DataInStage();
          return;

        case USB_REQUEST_SET_DESCRIPTOR:
          break;

        case USB_REQUEST_GET_CONFIGURATION:
          if (!ReqGetConfiguration()) break;
          DataInStage();
          return;

        case USB_REQUEST_SET_CONFIGURATION:
          if (!ReqSetConfiguration()) break;
          StatusInStage();
          return;

        case USB_REQUEST_GET_INTERFACE:
          if (!ReqGetInterface())    break;
          DataInStage();
          return;

        case USB_REQUEST_SET_INTERFACE:
          if (!ReqSetInterface())    break;
          StatusInStage();
          return;

        default:
          break;
      }
      break;  /* end case REQUEST_STANDARD */
    case REQUEST_CLASS:
      Ep0SetupClass ();
      return;  /* end case REQUEST_CLASS */


    default:
      break;
  }

  if0.ep.SetStallEP (EndpointIn);
  EP0Data.Count = 0;
}
void UsbDevice::Ep0SetupClass (void) {
  unsigned tmp;
  IfacePostRunAction act;
  switch (SetupPacket.bmRequestType.BM.Recipient) {

    case REQUEST_TO_DEVICE:
      break;

    case REQUEST_TO_INTERFACE:
      tmp = SetupPacket.wIndex.WB.L;
      act = ifs[tmp]->SetupHandler (& SetupPacket);
      switch (act) {
	case IfacePostRunActionDoNothing:
	  return;			// nedelej nic
        case IfacePostRunActionStatusIn:
          StatusInStage ();
          return;			// Status In
        case IfacePostRunActionSetData:
          EP0Data.pData = EP0Buf.b;	// Set Data Only
          return;
	case IfacePostRunActionMscLun:
	  EP0Buf.w[0] = 0;
        case IfacePostRunActionSetDataIn:
          EP0Data.pData = EP0Buf.b;	// Set Data + In
          DataInStage ();
          return;
        case IfacePostRunActionStall:	// stall
        default:
          break;
      }
      break;

    case REQUEST_TO_ENDPOINT:
    default:
      break;
  }
  if0.ep.SetStallEP (EndpointIn);
  EP0Data.Count = 0;
}

void UsbDevice::Ep0EventOut (void) {
  if (SetupPacket.bmRequestType.BM.Dir == REQUEST_HOST_TO_DEVICE) {
    if (EP0Data.Count) { /* still data to receive ? */
      DataOutStage();                                                /* receive data */
      if (EP0Data.Count == 0) {                                      /* data complete ? */
        switch (SetupPacket.bmRequestType.BM.Type) {

          case REQUEST_STANDARD:
            break;                                            /* not supported */

          case REQUEST_CLASS:
            Ep0OutClass ();
            return;

          default:
            break;
        }
        if0.ep.SetStallEP (EndpointIn);
        EP0Data.Count = 0;
      }
    }
  } else {
    StatusOutStage();                                            /* receive Acknowledge */
  }
  return;

}
void UsbDevice::Ep0OutClass (void) {
  unsigned tmp;
  switch (SetupPacket.bmRequestType.BM.Recipient) {
    case REQUEST_TO_DEVICE:
      break;                                        /* not supported */

    case REQUEST_TO_INTERFACE:
      tmp = SetupPacket.wIndex.WB.L;
      ifs[tmp]->CommandHandler (& SetupPacket, EP0Buf.b);
      StatusInStage ();
      return;

    case REQUEST_TO_ENDPOINT:

    default:
      break;
  }
  if0.ep.SetStallEP (EndpointIn);
  EP0Data.Count = 0;
}

void UsbDevice::Ep0EventIn (void) {
  if (SetupPacket.bmRequestType.BM.Dir == REQUEST_DEVICE_TO_HOST) {
    DataInStage();                                               /* send data */
  } else {
    if (USB_DeviceAddress & 0x80) {
      USB_DeviceAddress &= 0x7F;
      SetAddress (USB_DeviceAddress);
    }
  }
}

/** ************************************/
#ifdef DSCBLD

#include <string.h>
#include "dscbld.h"
#include "usbd.h"

unsigned int UsbDevice::enumerate (unsigned int in) {
  IfCount = 0;
  unsigned int res = in;
  res += sizeof (usb_config_descriptor);
  for (UsbClass * n = list; n; n = n->next) res = n->enumerate (res);
  return res;
}

void UsbDevice::blddsc (void) {
  printf ("PhysEndPoints=%d, Interfaces=%d\n", useeps, useifs);

  for (unsigned i=0; i<useeps; i++) {
    UsbEndpoint * p = eps [i];
    if (p->get() == EndpointTypeIntr) {
      printf ("(%d) %dEPI%02X\n",         i, p->get(), p->addr | 0x80);
    } else {
      printf ("(%d) %dEPI%02X,EPO%02X\n", i, p->get(), p->addr | 0x80, p->addr);
    }
  }

  cDesc = fopen ("dscbld.c", "w");
  fprintf (cDesc, "// Generated file, DO NOT EDIT !!!\n");
  blddev ();
  bldcfg ();
  bldstr ();
  fprintf (cDesc, "const unsigned UsbDeviceDescriptorLen = sizeof (UsbDeviceDescriptor);\n");
  fprintf (cDesc, "const unsigned UsbConfigDescriptorLen = sizeof (UsbConfigDescriptor);\n");
  fprintf (cDesc, "const unsigned UsbStringDescriptorLen = sizeof (UsbStringDescriptor);\n");
  fclose (cDesc);
}

static const char * AlignAttribute = "__attribute__((aligned(4)))";
void UsbDevice::blddev (void) {
  memset (&dsc, 0, sizeof (usb_device_descriptor));
  memset (&cfg, 0, sizeof (usb_config_descriptor));
  memset (StringField, 0, sizeof (StringField));

  dsc.bLength         = sizeof (usb_device_descriptor);
  dsc.bDescriptorType = USB_DT_DEVICE;

  dsc.bcdUSB          = 0x200;
//dsc.bcdUSB          = 0x110;
  /// funguje oboji (nuly jsou asi spravne)
  /*
  dsc.bDeviceClass    = 239;	// Misc. device
  dsc.bDeviceSubClass = 2;
  dsc.bDeviceProtocol = 1;		// Interface Association
  */
  dsc.bMaxPacketSize0 = 64;
  // -- dsc.bMaxPacketSize0 = 8; // jen test

  dsc.idVendor  = 0x0483;	// STMicroelectronics
  dsc.idProduct = 0x5740;	// STM32F407
  /*
  dsc.idVendor  = 0x16c0; // FREE
  dsc.idProduct = 0x05e1; // VCOM
  
  dsc.idVendor  = 0x1FC9;	// NXP LPC1343
  dsc.idProduct = 0x0003;	// VCOM
  */
  dsc.bcdDevice = 0x0100;

  StrCount = 0;
  dsc.iManufacturer   = AddString (sManufacturer, StrCount);
  dsc.iProduct        = AddString (sProduct,      StrCount);
  dsc.iSerialNumber   = AddString (sSerialNumber, StrCount);

  dsc.bNumConfigurations = 1;

  fprintf (cDesc, "const unsigned char UsbDeviceDescriptor [] %s = {\n", AlignAttribute);
  StructOut (&dsc, sizeof (usb_device_descriptor));
  fprintf (cDesc, "};\n");
}
void UsbDevice::bldcfg (void) {
  unsigned int len = enumerate (0);
  printf ("Total len = %d\n", len);

  cfg.bLength         = sizeof (usb_config_descriptor);
  cfg.bDescriptorType = USB_DT_CONFIG;
  cfg.bNumInterfaces  = useifs;
  cfg.bConfigurationValue = 1;
  cfg.iConfiguration  = 0;
  cfg.bmAttributes    = 0x80;	// Bus Powered
  cfg.bMaxPower       = 100	/*mA*/ >> 1;

  cfg.wTotalLength = len;

  fprintf (cDesc, "const unsigned char UsbConfigDescriptor [] %s = {\n", AlignAttribute);
  StructOut (&cfg, sizeof (usb_config_descriptor));
  for (UsbClass * n = list; n; n = n->next)  n->blddsc();
  fprintf (cDesc, "};\n");
}
#define USB_STRING_DESCRIPTOR_TYPE 3
void UsbDevice::bldstr (void) {
  fprintf (cDesc, "const unsigned char UsbStringDescriptor [] %s = {\n", AlignAttribute);
  fprintf (cDesc, "0x04,0x03,0x09,0x04,\n"); // fixne - US ASCII

  for (unsigned i=0; i<MaxUsbStrings; i++) {
    if (!StringField[i].index) continue;
    char const * str = StringField[i].string;
    unsigned j,l;
    l = strlen (str);
    fprintf (cDesc, "0x%02X,", 2* (l+1));	// bLength
    fprintf (cDesc, "0x%02X,", USB_STRING_DESCRIPTOR_TYPE);
    for (j=0; ; j++) {
      const unsigned char c = str[j];
      if (!c) break;
      fprintf (cDesc,"0x%02X,0,", c);
    }
    fprintf (cDesc, "\n");
  }

  fprintf (cDesc, "};\n");
}
void UsbDevice::setStrings (const char * Manufacturer, const char * Product, const char * SerialNumber) {
  sManufacturer = Manufacturer;
  sProduct      = Product;
  sSerialNumber = SerialNumber;
}

/** Pomocna funkce a data */

FILE  *  cDesc;
unsigned IfCount, StrCount;
StringNumbering StringField [MaxUsbStrings];

void StructOut (const void * p, unsigned l) {
  const uint8_t * s = (const uint8_t *) p;
  for (unsigned i=0; i<l; i++)
    fprintf (cDesc, "0x%02X,", s[i]);
  fprintf (cDesc, "\n");
}
unsigned AddString (char const * str, unsigned & index) {
  if (str) {
    StringField [index].index  = index + 1; 
    StringField [index].string = str;
    return ++index;
  }
  return 0;
}
//#warning "???"
#endif // DSCBLD
/** ************************************/

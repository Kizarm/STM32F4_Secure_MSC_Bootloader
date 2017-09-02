#include "dscbld.h"
#include "cdclass.h"
#include "usbd.h"
#include "debug.h"

CdClass::CdClass() : UsbClass(), BaseLayer (),
  ctrl (this), data (this) {
  ctrl.ep.set (EndpointTypeIntr);
  addIface (& ctrl);
  addIface (& data);
  
  usbtx_rdy   = 1;
  send_enable = 0;
  pusart      = NULL;
}
void CdClass::EventHandler (EndpointCallbackEvents event) {
  print ("CdClass: event=%d\n", event);
  unsigned  rlen;
  switch (event) {
    case USB_EVT_OUT:
      rlen = data.ep.Read (rxBuf);
      Recv   (rlen);
      break;
    case USB_EVT_IN:
      usbtx_rdy = 1;
      Send       ();
      break;
    default: break;
  }

}

uint32_t CdClass::Down (const char * buf, uint32_t len) {
  /// Pokud neni nastaveno DTR, zahodit data.
  if (!send_enable) return len;
  
  uint32_t n;
  for (n=0; n<len; n++) {
    if (!tx.Write(buf[n])) break;
  }
  Send ();
  return n;
}
void CdClass::Send (void) {
  char* ptr = (char*) txBuf;
  uint32_t n;
  
  if  (!usbtx_rdy) return;
  for (n=0; n<USB_MAX_NON_ISO_SIZE; n++) {
    if (!tx.Read(ptr[n])) break;
  }
  if  (!n) return;
  // print ("Chci zapsat %d\n", n);
  n = data.ep.Write (txBuf, n);
  usbtx_rdy = 0;
}
void CdClass::Recv (uint32_t len) {
  uint32_t  n, ofs = 0;
  while (len) {
    n = Up ((char*) rxBuf + ofs, len);
    if (!n) return;
    len -= n;
    ofs += n;
  }
}
//extern "C" void mdebug (const char * fmt, ...);
#ifdef DSCBLD
// aby nebylo nutne delat slozite include, zde zjednodusime
bool CdClass::SetCtrlLineState (uint16_t state) {
  return false;
}
bool CdClass::SetLineCode (uint8_t * d) {
  return false;
}
#else // DSCBLD

#include "usart.h"

bool CdClass::SetCtrlLineState (uint16_t state) {
  print  ("CdClass : 0x%04X\n", state);
  if (state & 1) send_enable = 1;
  else           send_enable = 0;
  return true;
}
bool CdClass::SetLineCode (uint8_t * d) {
  CDC_LINE_CODING * line_coding = (CDC_LINE_CODING *) d;
  uint32_t baud = 9600;
  uint8_t  lcr  = 0x3;               /* 8 bits, no Parity, 1 Stop bit */

  if (line_coding->bCharFormat)
    lcr |= (1 << 2);                 /* Number of stop bits */

  if (line_coding->bParityType) {    /* Parity bit type */
    lcr |= (1 << 3);
    lcr |= ( ( (line_coding->bParityType - 1) & 0x3) << 4);
  }
  if (line_coding->bDataBits) {      /* Number of data bits */
    lcr |= ( (line_coding->bDataBits - 5) & 0x3);
  } else {
    lcr |= 0x3;
  }
  baud = line_coding->dwDTERate.get();
  print  ("CdClass, Baud = %d\n", baud);
  if (pusart)
    pusart->SetLineParams (baud, lcr);
  return true;
}
#endif // DSCBLD
/** ************************************/
void CtrlIface::Handler (EndpointCallbackEvents event) {
  // Tady asi nebude muset byt nic
  error ("CtrlIface\n");
}
IfacePostRunAction CtrlIface::SetupHandler (USB_SETUP_PACKET * p) {
  switch (p->bRequest) {
    case CDC_SEND_ENCAPSULATED_COMMAND:
    case CDC_SET_COMM_FEATURE:
    case CDC_SET_LINE_CODING:
      return IfacePostRunActionSetData;

    case CDC_GET_ENCAPSULATED_RESPONSE:
      //if (CDC_GetEncapsulatedResponse()) {
        return IfacePostRunActionSetDataIn;
      //}
      break;
    case CDC_GET_COMM_FEATURE:
      //if (CDC_GetCommFeature (SetupPacket.wValue.W)) {
        return IfacePostRunActionSetDataIn;
      //}
      break;
    case CDC_CLEAR_COMM_FEATURE:
      //if (CDC_ClearCommFeature (SetupPacket.wValue.W)) {
        return IfacePostRunActionStatusIn;
      //}
      break;
    case CDC_GET_LINE_CODING:
      //if (CDC_GetLineCoding()) {
        return IfacePostRunActionSetDataIn;
      //}
      break;
    case CDC_SET_CONTROL_LINE_STATE:
      if (parent->SetCtrlLineState (p->wValue.W)) {
        return IfacePostRunActionStatusIn;
      }
      break;
    case CDC_SEND_BREAK:
      //if (CDC_SendBreak (SetupPacket.wValue.W)) {
        return IfacePostRunActionStatusIn;
      //}
      break;
  }
  return IfacePostRunActionDoNothing;
}
unsigned int CtrlIface::CommandHandler (USB_SETUP_PACKET * p, uint8_t * buf) {
  switch (p->bRequest) {
    case CDC_SEND_ENCAPSULATED_COMMAND:
      //if (CDC_SendEncapsulatedCommand()) {
        return 0;
      //}
      break;
    case CDC_SET_COMM_FEATURE:
      //if (CDC_SetCommFeature (SetupPacket.wValue.W)) {
        return 0;
      //}
      break;
    case CDC_SET_LINE_CODING:
      if (parent->SetLineCode (buf)) {
        return 0;
      }
      break;
  }
  return 1;
}

/** ************************************/

void DataIface::Handler (EndpointCallbackEvents event) {
  parent->EventHandler (event);
}

/** ************************************/
#ifdef DSCBLD
#include <string.h>

void CdClass::blddsc (void) {
  StructOut (&asoc, sizeof (usb_interface_assoc_descriptor));
  UsbClass::blddsc();
}
unsigned int CdClass::enumerate (unsigned int in) {
  memset (&asoc, 0, sizeof (usb_interface_assoc_descriptor));
  asoc.bLength = sizeof (usb_interface_assoc_descriptor);
  asoc.bDescriptorType   = USB_DT_INTERFACE_ASSOCIATION;
  asoc.bFirstInterface   = IfCount;
  asoc.bInterfaceCount   = 2;

  asoc.bFunctionClass    = CDC_COMMUNICATION_INTERFACE_CLASS;
  asoc.bFunctionSubClass = CDC_ABSTRACT_CONTROL_MODEL;
  asoc.bFunctionProtocol = 0;

  asoc.iFunction = AddString (StrDesc, StrCount); // Index pro string
  // printf ("CdClass::enumerate\n");
  unsigned int res = in;
  res += sizeof (usb_interface_assoc_descriptor);
  return UsbClass::enumerate (res);
}

void CtrlIface::blddsc (void) {
  fprintf (cDesc, "// Control Interface %d\n", dsc.bInterfaceNumber);
  StructOut (&dsc, sizeof (usb_interface_descriptor));
  StructOut (&acd, sizeof (usb_cdc_aditional_descriptors));
  // UsbInterface::blddsc();
  ep.blddsc();
}
unsigned int CtrlIface::enumerate (unsigned int in) {
  memset (&acd, 0, sizeof (usb_cdc_aditional_descriptors));
  acd.headfd.bLength         = sizeof (CDC_s1);
  acd.callfd.bFunctionLength = sizeof (CDC_s2);
  acd.ctrlfd.bFunctionLength = sizeof (CDC_s3);
  acd.uniofd.bFunctionLength = sizeof (CDC_s4);
  acd.headfd.bDescriptorType = acd.callfd.bDescriptorType = acd.ctrlfd.bDescriptorType
                             = acd.uniofd.bDescriptorType = CDC_CS_INTERFACE;

  acd.headfd.bDescriptorSubtype = CDC_HEADER;
  acd.callfd.bDescriptorSubtype = CDC_CALL_MANAGEMENT;
  acd.ctrlfd.bDescriptorSubtype = CDC_ABSTRACT_CONTROL_MANAGEMENT;
  acd.uniofd.bDescriptorSubtype = CDC_UNION;

  acd.headfd.bcdCDC             = CDC_V1_10;
//acd.callfd.bmCapabilities     = 1;            // ???
  acd.ctrlfd.bmCapabilities     = CDC_ABSTRACT_CONTROL_MANAGEMENT;

  acd.callfd.bDataInterface     = IfCount + 1;
  acd.uniofd.bMasterInterface   = IfCount;
  acd.uniofd.bSlaveInterface0   = IfCount + 1;

  unsigned int res = in;
  res += sizeof (usb_cdc_aditional_descriptors);
  res  = UsbInterface::enumerate (res);

  dsc.bNumEndpoints      = 1;
  dsc.bInterfaceClass    = CDC_COMMUNICATION_INTERFACE_CLASS;
  dsc.bInterfaceSubClass = CDC_ABSTRACT_CONTROL_MODEL;
  dsc.iInterface = 0; // Index Stringu - TODO

  return res;
}

void DataIface::blddsc (void) {
  fprintf (cDesc, "// Data    Interface %d\n", dsc.bInterfaceNumber);
  UsbInterface::blddsc();
}
unsigned int DataIface::enumerate (unsigned int in) {
  unsigned int res = in;
  res = UsbInterface::enumerate (res);

  dsc.bNumEndpoints      = 2;
  dsc.bInterfaceClass    = CDC_DATA_INTERFACE_CLASS;
  dsc.bInterfaceSubClass = 0;
  dsc.iInterface = 0; // Index Stringu - TODO

  return res;
}
#endif // DSCBLD

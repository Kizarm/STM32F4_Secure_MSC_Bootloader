#ifndef CDCLASS_H
#define CDCLASS_H

#include "usbclass.h"
#include "usbinterface.h"
#include "baselayer.h"
#include "mendian.h"
#include "fifo.h"

// Communication device class specification version 1.10
#define CDC_V1_10                               0x0110

// Communication interface class code
// (usbcdc11.pdf, 4.2, Table 15)
#define CDC_COMMUNICATION_INTERFACE_CLASS       0x02

enum CommunicationInterfaceClassSubclassCodes {
// (usbcdc11.pdf, 4.3, Table 16)
 CDC_DIRECT_LINE_CONTROL_MODEL         =  0x01,
 CDC_ABSTRACT_CONTROL_MODEL            =  0x02,
 CDC_TELEPHONE_CONTROL_MODEL           =  0x03,
 CDC_MULTI_CHANNEL_CONTROL_MODEL       =  0x04,
 CDC_CAPI_CONTROL_MODEL                =  0x05,
 CDC_ETHERNET_NETWORKING_CONTROL_MODEL =  0x06,
 CDC_ATM_NETWORKING_CONTROL_MODEL      =  0x07
};
// Communication interface class control protocol codes
// (usbcdc11.pdf, 4.4, Table 17)
#define CDC_PROTOCOL_COMMON_AT_COMMANDS         0x01

// Data interface class code
// (usbcdc11.pdf, 4.5, Table 18)
#define CDC_DATA_INTERFACE_CLASS                0x0A

enum DataInterfaceClassProtocolCodes {
// (usbcdc11.pdf, 4.7, Table 19)
 CDC_PROTOCOL_ISDN_BRI                =   0x30,
 CDC_PROTOCOL_HDLC                    =   0x31,
 CDC_PROTOCOL_TRANSPARENT             =   0x32,
 CDC_PROTOCOL_Q921_MANAGEMENT         =   0x50,
 CDC_PROTOCOL_Q921_DATA_LINK          =   0x51,
 CDC_PROTOCOL_Q921_MULTIPLEXOR        =   0x52,
 CDC_PROTOCOL_V42                     =   0x90,
 CDC_PROTOCOL_EURO_ISDN               =   0x91,
 CDC_PROTOCOL_V24_RATE_ADAPTATION     =   0x92,
 CDC_PROTOCOL_CAPI                    =   0x93,
 CDC_PROTOCOL_HOST_BASED_DRIVER       =   0xFD,
 CDC_PROTOCOL_DESCRIBED_IN_PUFD       =   0xFE
};
// Type values for bDescriptorType field of functional descriptors
// (usbcdc11.pdf, 5.2.3, Table 24)
#define CDC_CS_INTERFACE                        0x24
#define CDC_CS_ENDPOINT                         0x25

enum TypeValuesForDescriptorSubtypeFieldOfFunctionalDescriptors {
// (usbcdc11.pdf, 5.2.3, Table 25)
 CDC_HEADER                           =   0x00,
 CDC_CALL_MANAGEMENT                  =   0x01,
 CDC_ABSTRACT_CONTROL_MANAGEMENT      =   0x02,
 CDC_DIRECT_LINE_MANAGEMENT           =   0x03,
 CDC_TELEPHONE_RINGER                 =   0x04,
 CDC_REPORTING_CAPABILITIES           =   0x05,
 CDC_UNION                            =   0x06,
 CDC_COUNTRY_SELECTION                =   0x07,
 CDC_TELEPHONE_OPERATIONAL_MODES      =   0x08,
 CDC_USB_TERMINAL                     =   0x09,
 CDC_NETWORK_CHANNEL                  =   0x0A,
 CDC_PROTOCOL_UNIT                    =   0x0B,
 CDC_EXTENSION_UNIT                   =   0x0C,
 CDC_MULTI_CHANNEL_MANAGEMENT         =   0x0D,
 CDC_CAPI_CONTROL_MANAGEMENT          =   0x0E,
 CDC_ETHERNET_NETWORKING              =   0x0F,
 CDC_ATM_NETWORKING                   =   0x10
};
enum CDC_class_specific_request_codes {
// (usbcdc11.pdf, 6.2, Table 46)
// see Table 45 for info about the specific requests.
 CDC_SEND_ENCAPSULATED_COMMAND        =   0x00,
 CDC_GET_ENCAPSULATED_RESPONSE        =   0x01,
 CDC_SET_COMM_FEATURE                 =   0x02,
 CDC_GET_COMM_FEATURE                 =   0x03,
 CDC_CLEAR_COMM_FEATURE               =   0x04,
 CDC_SET_AUX_LINE_STATE               =   0x10,
 CDC_SET_HOOK_STATE                   =   0x11,
 CDC_PULSE_SETUP                      =   0x12,
 CDC_SEND_PULSE                       =   0x13,
 CDC_SET_PULSE_TIME                   =   0x14,
 CDC_RING_AUX_JACK                    =   0x15,
 CDC_SET_LINE_CODING                  =   0x20,
 CDC_GET_LINE_CODING                  =   0x21,
 CDC_SET_CONTROL_LINE_STATE           =   0x22,
 CDC_SEND_BREAK                       =   0x23,
 CDC_SET_RINGER_PARMS                 =   0x30,
 CDC_GET_RINGER_PARMS                 =   0x31,
 CDC_SET_OPERATION_PARMS              =   0x32,
 CDC_GET_OPERATION_PARMS              =   0x33,
 CDC_SET_LINE_PARMS                   =   0x34,
 CDC_GET_LINE_PARMS                   =   0x35,
 CDC_DIAL_DIGITS                      =   0x36,
 CDC_SET_UNIT_PARAMETER               =   0x37,
 CDC_GET_UNIT_PARAMETER               =   0x38,
 CDC_CLEAR_UNIT_PARAMETER             =   0x39,
 CDC_GET_PROFILE                      =   0x3A,
 CDC_SET_ETHERNET_MULTICAST_FILTERS   =   0x40,
 CDC_SET_ETHERNET_PMP_FILTER          =   0x41,
 CDC_GET_ETHERNET_PMP_FILTER          =   0x42,
 CDC_SET_ETHERNET_PACKET_FILTER       =   0x43,
 CDC_GET_ETHERNET_STATISTIC           =   0x44,
 CDC_SET_ATM_DATA_FORMAT              =   0x50,
 CDC_GET_ATM_DEVICE_STATISTICS        =   0x51,
 CDC_SET_ATM_DEFAULT_VC               =   0x52,
 CDC_GET_ATM_VC_STATISTICS            =   0x53
};
// Communication feature selector codes
// (usbcdc11.pdf, 6.2.2..6.2.4, Table 47)
#define CDC_ABSTRACT_STATE                      0x01
#define CDC_COUNTRY_SETTING                     0x02

// Feature Status returned for ABSTRACT_STATE Selector
// (usbcdc11.pdf, 6.2.3, Table 48)
#define CDC_IDLE_SETTING                        (1 << 0)
#define CDC_DATA_MULTPLEXED_STATE               (1 << 1)


// Control signal bitmap values for the SetControlLineState request
// (usbcdc11.pdf, 6.2.14, Table 51)
#define CDC_DTE_PRESENT                         (1 << 0)
#define CDC_ACTIVATE_CARRIER                    (1 << 1)

enum CDC_class_specific_notification_codes {
// (usbcdc11.pdf, 6.3, Table 68)
// see Table 67 for Info about class-specific notifications
 CDC_NOTIFICATION_NETWORK_CONNECTION   =  0x00,
 CDC_RESPONSE_AVAILABLE                =  0x01,
 CDC_AUX_JACK_HOOK_STATE               =  0x08,
 CDC_RING_DETECT                       =  0x09,
 CDC_NOTIFICATION_SERIAL_STATE         =  0x20,
 CDC_CALL_STATE_CHANGE                 =  0x28,
 CDC_LINE_STATE_CHANGE                 =  0x29,
 CDC_CONNECTION_SPEED_CHANGE           =  0x2A
};
// UART state bitmap values (Serial state notification).
// (usbcdc11.pdf, 6.3.5, Table 69)
#define CDC_SERIAL_STATE_OVERRUN                (1 << 6)  // receive data overrun error has occurred
#define CDC_SERIAL_STATE_PARITY                 (1 << 5)  // parity error has occurred
#define CDC_SERIAL_STATE_FRAMING                (1 << 4)  // framing error has occurred
#define CDC_SERIAL_STATE_RING                   (1 << 3)  // state of ring signal detection
#define CDC_SERIAL_STATE_BREAK                  (1 << 2)  // state of break detection
#define CDC_SERIAL_STATE_TX_CARRIER             (1 << 1)  // state of transmission carrier
#define CDC_SERIAL_STATE_RX_CARRIER             (1 << 0)  // state of receiver carrier



/** USB_DT_INTERFACE_ASSOCIATION: groups interfaces */
struct usb_interface_assoc_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;

	__u8  bFirstInterface;
	__u8  bInterfaceCount;
	__u8  bFunctionClass;
	__u8  bFunctionSubClass;
	__u8  bFunctionProtocol;
	__u8  iFunction;
} __attribute__ ((packed));

/**Header Functional Descriptor*/
struct CDC_s1 {
  __u8   bLength;
  __u8   bDescriptorType;
  __u8   bDescriptorSubtype;
  __le16 bcdCDC;
} __attribute__ ((packed));
/**Call Management Functional Descriptor*/
struct CDC_s2 {
  __u8 bFunctionLength;
  __u8 bDescriptorType;
  __u8 bDescriptorSubtype;
  __u8 bmCapabilities;
  __u8 bDataInterface;
} __attribute__ ((packed));
/**Abstract Control Management Functional Descriptor*/
struct CDC_s3 {
  __u8 bFunctionLength;
  __u8 bDescriptorType;
  __u8 bDescriptorSubtype;
  __u8 bmCapabilities;
} __attribute__ ((packed));
/**Union Functional Descriptor*/
struct CDC_s4 {
  __u8 bFunctionLength;
  __u8 bDescriptorType;
  __u8 bDescriptorSubtype;
  __u8 bMasterInterface;
  __u8 bSlaveInterface0;
} __attribute__ ((packed));

struct usb_cdc_aditional_descriptors {
  CDC_s1 headfd;
  CDC_s2 callfd;
  CDC_s3 ctrlfd;
  CDC_s4 uniofd;
} __attribute__ ((packed));
///  Line coding structure
//  Format of the data returned when a GetLineCoding request is received
// (usbcdc11.pdf, 6.2.13)
struct CDC_LINE_CODING {
  __le32 dwDTERate;                          // Data terminal rate in bits per second
  __u8   bCharFormat;                        // Number of stop bits
  __u8   bParityType;                        // Parity bit type
  __u8   bDataBits;                          // Number of data bits
} __attribute__ ((packed));

class CdClass;

class CtrlIface : public UsbInterface {
  public:
    CtrlIface (CdClass * p) : UsbInterface() {parent = p;};
    void Handler (EndpointCallbackEvents e);
    IfacePostRunAction SetupHandler   (USB_SETUP_PACKET * p);
    unsigned           CommandHandler (USB_SETUP_PACKET * p, uint8_t * buf);
    
    CdClass * parent;
/** ************************************/
#ifdef DSCBLD
  public:
    void     blddsc    (void);
    unsigned enumerate (unsigned in);
    usb_cdc_aditional_descriptors acd;
#endif // DSCBLD
/** ************************************/
};
class DataIface : public UsbInterface {
  public:
    DataIface (CdClass * p) : UsbInterface() {parent = p;};
    void Handler (EndpointCallbackEvents e);
    
    CdClass * parent;
/** ************************************/
#ifdef DSCBLD
  public:
    void     blddsc    (void);
    unsigned enumerate (unsigned in);
#endif // DSCBLD
/** ************************************/
};
#define USB_MAX_NON_ISO_SIZE 64
class UsartClass;

class CdClass : public UsbClass, public BaseLayer {
  public:
    CdClass();
   ~CdClass() {};
    uint32_t Down (const char* buf, uint32_t len);
    /// for optional SetLineCode handling
    void AttachUsart      (UsartClass & usart) {
      pusart = & usart;
    }
    bool SetCtrlLineState (uint16_t state);
    bool SetLineCode      (uint8_t * data);
    void EventHandler     (EndpointCallbackEvents e);
  protected:
    void Recv             (uint32_t len);
    void Send             (void);
  private:
    CtrlIface         ctrl;
    DataIface         data;
    // CDC_LINE_CODING   clc;
    UsartClass      * pusart;
    
    uint8_t           rxBuf [USB_MAX_NON_ISO_SIZE];
    uint8_t           txBuf [USB_MAX_NON_ISO_SIZE];
    volatile uint32_t usbtx_rdy;     //!< semafor pro čekání na kompletní výstup dat
    volatile uint32_t send_enable;   //!< je dobre blokovat data dokud neni DTR==1
    Fifo<64, char>    tx;
    
/** ************************************/
#ifdef DSCBLD
  public:
    void     blddsc    (void);
    unsigned enumerate (unsigned in);
    void     setString (char const * str) {
      StrDesc = str;
    }
    usb_interface_assoc_descriptor asoc;
    char const * StrDesc;
#endif // DSCBLD
/** ************************************/
};

#endif // CDCLASS_H

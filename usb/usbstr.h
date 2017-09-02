#ifndef _USB_STR_H
#define _USB_STR_H

#include "mendian.h"
#include <stdint.h>

#define FALSE 0
#define TRUE  1

enum bmRequestTypeDir {
 REQUEST_HOST_TO_DEVICE = 0,
 REQUEST_DEVICE_TO_HOST
};
enum bmRequestTypeType {
 REQUEST_STANDARD       =  0,
 REQUEST_CLASS,
 REQUEST_VENDOR,
 REQUEST_RESERVED
};
enum bmRequestTypeRecipient {
 REQUEST_TO_DEVICE      =  0,
 REQUEST_TO_INTERFACE,
 REQUEST_TO_ENDPOINT,
 REQUEST_TO_OTHER,
};
enum USBStandardFeatureselectors {
 USB_FEATURE_ENDPOINT_STALL      =      0,
 USB_FEATURE_REMOTE_WAKEUP
};
enum USBStandardRequestCodes {
 USB_REQUEST_GET_STATUS          =      0,
 USB_REQUEST_CLEAR_FEATURE       =      1,
 USB_REQUEST_SET_FEATURE         =      3,
 USB_REQUEST_SET_ADDRESS         =      5,
 USB_REQUEST_GET_DESCRIPTOR      =      6,
 USB_REQUEST_SET_DESCRIPTOR      =      7,
 USB_REQUEST_GET_CONFIGURATION   =      8,
 USB_REQUEST_SET_CONFIGURATION   =      9,
 USB_REQUEST_GET_INTERFACE       =      10,
 USB_REQUEST_SET_INTERFACE       =      11,
 USB_REQUEST_SYNC_FRAME          =      12
};
enum USBDescriptorTypes {
 USB_DEVICE_DESCRIPTOR_TYPE              =   1,
 USB_CONFIGURATION_DESCRIPTOR_TYPE,
 USB_STRING_DESCRIPTOR_TYPE,
 USB_INTERFACE_DESCRIPTOR_TYPE,
 USB_ENDPOINT_DESCRIPTOR_TYPE,
 USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE,
 USB_OTHER_SPEED_CONFIG_DESCRIPTOR_TYPE,
 USB_INTERFACE_POWER_DESCRIPTOR_TYPE,
 USB_OTG_DESCRIPTOR_TYPE,
 USB_DEBUG_DESCRIPTOR_TYPE,
 USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE,
 /* USB extension Descriptor Type. */
 USB_SECURITY_TYPE,
 USB_KEY_TYPE,
 USB_ENCRIPTION_TYPE,
 USB_BOS_TYPE,
 USB_DEVICE_CAPABILITY_TYPE,
 USB_WIRELESS_ENDPOINT_COMPANION_TYPE
};
/* USB GET_STATUS Bit Values */
#define USB_GETSTATUS_SELF_POWERED             0x01
#define USB_GETSTATUS_REMOTE_WAKEUP            0x02
#define USB_GETSTATUS_ENDPOINT_STALL           0x01
/* bmAttributes in Configuration Descriptor */
#define USB_CONFIG_POWERED_MASK                0x40
#define USB_CONFIG_BUS_POWERED                 0x80
#define USB_CONFIG_SELF_POWERED                0xC0
#define USB_CONFIG_REMOTE_WAKEUP               0x20
enum IfacePostRunAction {
  IfacePostRunActionStall = 0,
  IfacePostRunActionDoNothing,
  IfacePostRunActionStatusIn,
  IfacePostRunActionSetData,
  IfacePostRunActionSetDataIn,
  
  IfacePostRunActionMscLun
};
/* USB_DT_DEVICE: Device descriptor */
struct usb_device_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;

	__le16 bcdUSB;
	__u8  bDeviceClass;
	__u8  bDeviceSubClass;
	__u8  bDeviceProtocol;
	__u8  bMaxPacketSize0;
	__le16 idVendor;
	__le16 idProduct;
	__le16 bcdDevice;
	__u8  iManufacturer;
	__u8  iProduct;
	__u8  iSerialNumber;
	__u8  bNumConfigurations;
} __attribute__ ((packed));

struct usb_config_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;

	__le16 wTotalLength;
	__u8  bNumInterfaces;
	__u8  bConfigurationValue;
	__u8  iConfiguration;
	__u8  bmAttributes;
	__u8  bMaxPower;
} __attribute__ ((packed));
/* USB String Descriptor */
struct USB_STRING_DESCRIPTOR {
  __u8  bLength;
  __u8  bDescriptorType;
  __le16  bString/*[]*/;
} __attribute__ ((packed));
struct USB_COMMON_DESCRIPTOR {
  __u8  bLength;
  __u8  bDescriptorType;
} __attribute__ ((packed));

/* USB 2.0 Device Qualifier Descriptor */
struct USB_DEVICE_QUALIFIER_DESCRIPTOR {
  __u8  bLength;
  __u8  bDescriptorType;
  __le16  bcdUSB;
  __u8  bDeviceClass;
  __u8  bDeviceSubClass;
  __u8  bDeviceProtocol;
  __u8  bMaxPacketSize0;
  __u8  bNumConfigurations;
  __u8  bReserved;
} __attribute__ ((packed));

union WORD_BYTE {
  uint16_t W;
  struct {
    __u8 L;
    __u8 H;
  } __attribute__ ((packed)) WB;
} __attribute__ ((packed));

/* bmRequestType Definition */
union REQUEST_TYPE {
  struct _BM {
    __u8 Recipient : 5;
    __u8 Type      : 2;
    __u8 Dir       : 1;
  }  __attribute__ ((packed)) BM;
  __u8 B;
} __attribute__ ((packed));

/* USB Default Control Pipe Setup Packet */
struct USB_SETUP_PACKET {
  REQUEST_TYPE bmRequestType;
  __u8      bRequest;
  WORD_BYTE    wValue;
  WORD_BYTE    wIndex;
  uint16_t     wLength;
} __attribute__ ((packed));

/* USB_DT_INTERFACE: Interface descriptor */
struct usb_interface_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;

	__u8  bInterfaceNumber;
	__u8  bAlternateSetting;
	__u8  bNumEndpoints;
	__u8  bInterfaceClass;
	__u8  bInterfaceSubClass;
	__u8  bInterfaceProtocol;
	__u8  iInterface;
} __attribute__ ((packed));

enum UsbEndpointType {
  EndpointTypeCtrl=0,
  EndpointTypeIso,
  EndpointTypeBulk,
  EndpointTypeIntr
};
enum UsbEndpointDir {
  EndpointOut=0,
  EndpointIn
};
enum UsbSyncType {
  SyncTypeNosync=0,
  SyncTypeAsync,
  SyncTypeAdapt,
  SyncTypeSync
};
enum UsbUsageType {
  UsageTypeData=0,
  UsageTypeFeddback,
  UsageTypeIfd
};
/* USB_DT_ENDPOINT: Endpoint descriptor */
struct usb_endpoint_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;

	union {
	  __u8 w;
	  struct {
	    __u8 adr    : 4;
	    __u8 unused : 3;
	    __u8 dir    : 1;
	  } b;
	} bEndpointAddress;
	union {
	  __u8 w;
	  struct {
	    __u8 trans  : 2;
	    __u8 sync   : 2;
	    __u8 usage  : 2;
	    __u8 unused : 2;
	  } b;
	} bmAttributes;
	
	__le16 wMaxPacketSize;
	__u8  bInterval;

	/* NOTE:  these two are _only_ in audio endpoints.
	use USB_DT_ENDPOINT*_SIZE in bLength, not sizeof.
	__u8  bRefresh;
	__u8  bSynchAddress; */
} __attribute__ ((packed));

#endif // _USB_STR_H

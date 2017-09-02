#ifndef MSCLASS_H
#define MSCLASS_H

#include "usbclass.h"
#include "storagebase.h"

/**
@file
@brief Obsluha bulk in a out endpointů pro USB mass-storage class.

Stavový automat pro MSC_BulkStage (přibližně)
@dot
digraph finite_state_machine {
  rankdir=LR;
  size="10,5"
  node [ shape = doublecircle ]; MSC_BS_CBW;
  node [ shape=ellipse, style=filled, color=yellow ];
  MSC_BS_CBW -> MSC_BS_ERROR [ label = "error - CBW invalid" ];
  MSC_BS_CBW -> MSC_BS_CSW   [ label = "fail" ];
  MSC_BS_CSW -> MSC_BS_CBW   [ label = "" ];
  MSC_BS_CBW -> MSC_BS_DATA_IN  [ label = "request read" ];
  MSC_BS_DATA_IN  -> MSC_BS_DATA_IN  [ label = "data" ];
  MSC_BS_DATA_OUT -> MSC_BS_DATA_OUT [ label = "data" ];
  MSC_BS_CBW -> MSC_BS_DATA_OUT [ label = "request write, verify" ];
  MSC_BS_DATA_IN  -> MSC_BS_DATA_IN_LAST_STALL [ label = "out of disk" ];
  MSC_BS_DATA_IN  -> MSC_BS_DATA_IN_LAST       [ label = "read end" ];
  MSC_BS_DATA_OUT -> MSC_BS_CSW [ label = "write, verify end" ];
  MSC_BS_DATA_IN_LAST ->       MSC_BS_CSW [ label = "read end" ];
  MSC_BS_DATA_IN_LAST_STALL -> MSC_BS_CSW [ label = "read error" ];
}
@enddot
  Původní verze od Keil je celkem slušně napsaná, ale je to opravdu jen minimální verze,
  která sice funguje, ale moc použít se v reálu nedá. Bylo tedy nutné přidat obsluhu chyb pomocí Sense Data.
  RWSetup byl rozdělen na Read a Write, což umožňuje z těchto metod volat přímo příkaz pro čtení
  a zápis na paměťovou kartu, takže tyto operace se nemusí provádět blokově, uspoří to paměť a mělo by být
  i rychlejší (nepoužívají se však cache).

*/


/*! MSC Subclass Codes */
#define MSC_SUBCLASS_RBC                0x01
#define MSC_SUBCLASS_SFF8020I_MMC2      0x02
#define MSC_SUBCLASS_QIC157             0x03
#define MSC_SUBCLASS_UFI                0x04
#define MSC_SUBCLASS_SFF8070I           0x05
#define MSC_SUBCLASS_SCSI               0x06

/*! MSC Protocol Codes */
#define MSC_PROTOCOL_CBI_INT            0x00
#define MSC_PROTOCOL_CBI_NOINT          0x01
#define MSC_PROTOCOL_BULK_ONLY          0x50


/*! MSC Request Codes */
#define MSC_REQUEST_RESET               0xFF
#define MSC_REQUEST_GET_MAX_LUN         0xFE


/*! MSC Bulk-only Stage */
enum MSC_BulkStages {
  MSC_BS_CBW                    =  0,         //!< Command Block Wrapper
  MSC_BS_DATA_OUT,                            //!< Data Out Phase
  MSC_BS_DATA_IN,                             //!< Data In Phase
  MSC_BS_DATA_IN_LAST,                        //!< Data In Last Phase
  MSC_BS_DATA_IN_LAST_STALL,                  //!< Data In Last Phase with Stall
  MSC_BS_CSW,                                 //!< Command Status Wrapper
  MSC_BS_ERROR                                //!< Error
};

/*! Bulk-only Command Block Wrapper */
struct MSC_CBW {
  uint32_t dSignature;
  uint32_t dTag;
  uint32_t dDataLength;
  uint8_t  bmFlags;
  uint8_t  bLUN;
  uint8_t  bCBLength;
  uint8_t  CB[16];
} __attribute__((packed));

/*! Bulk-only Command Status Wrapper */
struct MSC_CSW {
  uint32_t dSignature;
  uint32_t dTag;
  uint32_t dDataResidue;
  uint8_t  bStatus;
} __attribute__((packed));

#define MSC_CBW_Signature               0x43425355
#define MSC_CSW_Signature               0x53425355


/*! CSW Status Definitions */
#define CSW_CMD_PASSED                  0x00
#define CSW_CMD_FAILED                  0x01
#define CSW_PHASE_ERROR                 0x02

/* Max In/Out Packet Size */
#define MSC_MAX_PACKET  64
/* Mass Storage Memory Layout */
#define MSC_BlockSize   512

/* SCSI Sense Key/Additional Sense Code/ASC Qualifier values */
#define SS_NO_SENSE                           0x000000  // to je sice blbost, nicmene alespon neco to dela
#define SS_COMMUNICATION_FAILURE              0x040800
#define SS_INVALID_COMMAND                    0x052000
#define SS_INVALID_FIELD_IN_CDB               0x052400
#define SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE 0x052100
#define SS_LOGICAL_UNIT_NOT_SUPPORTED         0x052500
#define SS_MEDIUM_NOT_PRESENT                 0x023a00
#define SS_MEDIUM_REMOVAL_PREVENTED           0x055302
#define SS_NOT_READY_TO_READY_TRANSITION      0x062800
#define SS_RESET_OCCURRED                     0x062900
#define SS_SAVING_PARAMETERS_NOT_SUPPORTED    0x053900
#define SS_UNRECOVERED_READ_ERROR             0x031100
#define SS_WRITE_ERROR                        0x030c02
#define SS_WRITE_PROTECTED                    0x072700

#define SK(x)     ((uint8_t) ((x) >> 16))  /* Sense Key byte, etc. */
#define ASC(x)    ((uint8_t) ((x) >> 8 ))
#define ASCQ(x)   ((uint8_t) (x))

enum SCSI_Commands {
 SCSI_TEST_UNIT_READY          =  0x00,
 SCSI_REQUEST_SENSE            =  0x03,
 SCSI_FORMAT_UNIT              =  0x04,
 SCSI_INQUIRY                  =  0x12,
 SCSI_MODE_SELECT6             =  0x15,
 SCSI_MODE_SENSE6              =  0x1A,
 SCSI_START_STOP_UNIT          =  0x1B,
 SCSI_MEDIA_REMOVAL            =  0x1E,
 SCSI_READ_FORMAT_CAPACITIES   =  0x23,
 SCSI_READ_CAPACITY            =  0x25,
 SCSI_READ10                   =  0x28,
 SCSI_WRITE10                  =  0x2A,
 SCSI_VERIFY10                 =  0x2F,
 SCSI_READ12                   =  0xA8,
 SCSI_WRITE12                  =  0xAA,
 SCSI_MODE_SELECT10            =  0x55,
 SCSI_MODE_SENSE10             =  0x5A
};
/// Sense data pro report chybových stavů SS_XXX
struct MSC_SenseData {
  uint32_t data;                        //!< Sense Key, Additional Sense Code, ASC Qualifier
  uint32_t info;                        //!< číslo bloku, kde se vyskytla chyba
  uint8_t  valid;                       //!< 0x80, pokud je info nenulový, jinak 0
};

class MsClass;
class StorageBase;
/// @brief MSC má jediný interface s bulk IN a OUT endpointem.
class MscIface : public UsbInterface {
  public:
    /// Konstruktor
    MscIface (MsClass * p) : UsbInterface() {parent=p;};
    /// Obsluha Bulk endpointů
    void Handler (EndpointCallbackEvents e);
    /// Obsluha Setup - Reset a GetMaxLUN
    IfacePostRunAction SetupHandler   (USB_SETUP_PACKET * p);
    /// pro přístup na MsClass
    MsClass * parent;
/** ************************************/
#ifdef DSCBLD
  public:
    void     blddsc    (void);
    unsigned enumerate (unsigned in);
#endif // DSCBLD
/** ************************************/
};
/// @brief Vlastní třída pro Mass Storage s podporou SCSI.
class MsClass : public UsbClass {
  public:
    /// Konstruktor
    MsClass();
    /// Nutno připojit nějaké zařízení na uložení dat.
    void Attach (StorageBase & base, bool readonly = false) {
      mmc = & base;
      base.parent = this;
      ReadOnly = readonly;
    }
    /// Zbytek už jsou celkem nezajímavé vnitřní metody, některé ale musí být přístupné zvenčí
    bool Reset             (void);
    bool GetMaxLUN         (void);
    void MSC_RewriteCSW    (void);

    /** MSC Bulk Callback Functions */
    void BulkIn (void);
    void BulkOut(void);
    void SetSenseData (uint32_t t, uint32_t i, uint8_t v);
    void DecResidue   (uint32_t n);
  protected:
    /// Následující metody přístup zvenčí už nepotřebují.
    void MSC_SetStallEP         (UsbEndpointDir dir);
    void MSC_SetStallEPStall    (UsbEndpointDir dir);
    void MSC_SetCSW             (void);
    void MSC_GetCBW             (void);
    void MSC_MemoryVerify       (void);
    void MSC_MemoryWrite        (void);
    void MSC_MemoryRead         (void);
    void MSC_CommandParser      (SCSI_Commands p);
    void MSC_ReadCapacity       (void);
    void MSC_ReadFormatCapacity (void);
    
    bool MSC_ReadSetup (void);
    bool MSC_WriteSetup (void);
    bool DataInFormat (void);
    void DataInTransfer (void);
    void MSC_TestUnitReady (void);
    void MSC_Inquiry (void);
    void MSC_RequestSense (void);
    void MSC_ModeSense6 (void);
    void MSC_ModeSense10 (void);
  private:
    MscIface      msif;
    StorageBase * mmc;
    
    uint32_t MemOK;                            //!< Memory OK
    volatile MSC_BulkStages  BulkStage;        //!< Bulk Stage

    uint8_t  BulkBuf  [MSC_MAX_PACKET];        //!< Bulk In/Out Buffer
    uint8_t  BulkLen;                          //!< Bulk In/Out Length

    MSC_CBW CBW;                               //!< Command Block Wrapper
    MSC_CSW CSW;                               //!< Command Status Wrapper
    /// Počáteční hodnota značí, že zařízení je po resetu
    MSC_SenseData SenseData;// = {SS_RESET_OCCURRED, 0, 0};
    
    bool ReadOnly;
    
/** ************************************/
#ifdef DSCBLD
  public:
    void     blddsc    (void);
    unsigned enumerate (unsigned in);
    void     setString (char const * str) {
      StrDesc = str;
    }
    char const * StrDesc;
  private:
#endif // DSCBLD
/** ************************************/
};

#endif // MSCLASS_H

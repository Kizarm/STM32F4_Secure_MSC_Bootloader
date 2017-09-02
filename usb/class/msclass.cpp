#include "mendian.h"
#include "usbdevice.h"
#include "msclass.h"
#include "debug.h"

static inline void bytecopy (void * dst, const void * src, uint32_t len) {
  const char * s = (const char *) src;
  char       * d = (char       *) dst;
  for (uint32_t i=0; i<len; i++)  d[i] = s[i];
}
/** **************************************************/
MsClass::MsClass() : msif (this) {
  addIface (& msif);
  BulkStage      = MSC_BS_CBW;
  SenseData.data = SS_RESET_OCCURRED;
  ReadOnly = false;
}

void MsClass::BulkIn (void) {
  // print ("  stage = %d\n", BulkStage);
  switch (BulkStage) {
    case MSC_BS_DATA_IN:
      switch (CBW.CB[0]) {
        case SCSI_READ10:
        case SCSI_READ12:
          MSC_MemoryRead();
          break;
      }
      break;
    case MSC_BS_DATA_IN_LAST:
      MSC_SetCSW();
      break;
    case MSC_BS_DATA_IN_LAST_STALL:
      MSC_SetStallEP (EndpointIn);
      MSC_SetCSW();
      break;
    case MSC_BS_CSW:
      BulkStage = MSC_BS_CBW;
      break;
    default: break;
  }
}
void MsClass::BulkOut (void) {
  // print (" stage = %d\n", BulkStage);
  BulkLen = msif.ep.Read (BulkBuf);
  switch (BulkStage) {
    case MSC_BS_CBW:
      MSC_GetCBW();
      break;
    case MSC_BS_DATA_OUT:
      switch (CBW.CB[0]) {
        case SCSI_WRITE10:
        case SCSI_WRITE12:
          MSC_MemoryWrite();
          break;
        case SCSI_VERIFY10:
          MSC_MemoryVerify();
          break;
      }
      break;
    default:
      MSC_SetStallEP(EndpointOut);
      error("PHASE ERR\n");
      CSW.bStatus = CSW_PHASE_ERROR;
      MSC_SetCSW();
      break;
  }
}
void MsClass::DecResidue (uint32_t n) {
  if (SenseData.data == SS_NO_SENSE)
    CSW.dDataResidue -= n;
}
void MsClass::MSC_RewriteCSW (void) {
  dbg();
  if (CSW.dSignature == MSC_CSW_Signature) {
    msif.ep.Write ((uint8_t *)&CSW, sizeof(CSW));
  }
}
void MsClass::SetSenseData (uint32_t t, uint32_t i, uint8_t v) {
  SenseData.data  = t;
  SenseData.info  = i;
  if (v) SenseData.valid = 0x80;
  else   SenseData.valid = 0x00;
}

bool MsClass::GetMaxLUN (void) {
  dbg();                           /* No LUN associated with this device */
  return true;
}
void MsClass::MSC_SetCSW (void) {
  // dbg();
  CSW.dSignature = MSC_CSW_Signature;
  msif.ep.Write ((uint8_t *)&CSW, sizeof(CSW));
  BulkStage = MSC_BS_CSW;
}
void MsClass::MSC_GetCBW (void) {
//  dbg();
  uint32_t n;

  for (n = 0; n < BulkLen; n++) {
    *((uint8_t *)&CBW + n) = BulkBuf[n];
  }
  if ((BulkLen == sizeof(CBW)) && (CBW.dSignature == MSC_CBW_Signature)) {
    /* Valid CBW */
    CSW.dTag = CBW.dTag;
    CSW.dDataResidue = CBW.dDataLength;
    if ((CBW.bLUN      != 0) || 
        (CBW.bCBLength <  1) || 
        (CBW.bCBLength > 16)   ) {
      error("BAD LUN OR LENGHT\n");
      CSW.bStatus = CSW_CMD_FAILED;
      SetSenseData (SS_LOGICAL_UNIT_NOT_SUPPORTED, 0, 0);
      MSC_SetCSW();
    } else {                              // CBW OK
      MSC_CommandParser ((SCSI_Commands) CBW.CB[0]);
    }
  } else {
    error("Invalid CBW\n");
    /* Invalid CBW */
    MSC_SetStallEPStall (EndpointIn);
                                           /* set EP to stay stalled */
    MSC_SetStallEPStall (EndpointOut);
                                           /* set EP to stay stalled */
    BulkStage = MSC_BS_ERROR;
  }
}
void MsClass::MSC_CommandParser (SCSI_Commands p) {
  switch (p) {
    case SCSI_TEST_UNIT_READY:
      MSC_TestUnitReady();
      break;
    case SCSI_REQUEST_SENSE:
      MSC_RequestSense();
      break;
    case SCSI_INQUIRY:
      MSC_Inquiry();
      break;
    case SCSI_MODE_SENSE6:
      MSC_ModeSense6();
      break;
    case SCSI_MODE_SENSE10:
      MSC_ModeSense10();
      break;
    case SCSI_READ_FORMAT_CAPACITIES:
      MSC_ReadFormatCapacity();
      break;
    case SCSI_READ_CAPACITY:
      MSC_ReadCapacity();
      break;
    case SCSI_READ10:
    case SCSI_READ12:
      if (MSC_ReadSetup()) {
        if ((CBW.bmFlags & 0x80) != 0) {
          BulkStage = MSC_BS_DATA_IN;
          MSC_MemoryRead();
        } else {                       /* direction mismatch */
          MSC_SetStallEP(EndpointOut);
          error("READ DIR ERR\n");
          CSW.bStatus = CSW_PHASE_ERROR;
          MSC_SetCSW();
        }
      }
      break;
    case SCSI_WRITE10:
    case SCSI_WRITE12:
      if (MSC_WriteSetup()) {
        if ((CBW.bmFlags & 0x80) == 0) {
          BulkStage = MSC_BS_DATA_OUT;
        } else {                       /* direction mismatch */
          MSC_SetStallEP(EndpointIn);
          error("WRITE DIR ERR\n");
          CSW.bStatus = CSW_PHASE_ERROR;
          MSC_SetCSW();
        }
      }
      break;
    case SCSI_VERIFY10:
      if ((CBW.CB[1] & 0x02) == 0) {
        // BYTCHK = 0 -> CRC Check (not implemented)
        CSW.bStatus = CSW_CMD_PASSED;
        MSC_SetCSW();
        break;
      }
      if (MSC_ReadSetup()) {
        if ((CBW.bmFlags & 0x80) == 0) {
          BulkStage = MSC_BS_DATA_OUT;
          MemOK = TRUE;
        } else {
          MSC_SetStallEP(EndpointIn);
          error("VERIFY DIR ERR\n");
          CSW.bStatus = CSW_PHASE_ERROR;
          MSC_SetCSW();
        }
      }
      break;
    case SCSI_START_STOP_UNIT:
    case SCSI_MEDIA_REMOVAL:
    case SCSI_MODE_SELECT6:
    case SCSI_MODE_SELECT10:
    case SCSI_FORMAT_UNIT:
    default:                        // FAIL - Unsuported cmd
      error("Unsupported cmd [0x%02X]\n", p);
      CSW.bStatus = CSW_CMD_FAILED;
      SetSenseData (SS_INVALID_FIELD_IN_CDB, 0, 0);
      MSC_SetCSW();
      break;
  }
}

void MsClass::MSC_MemoryRead (void) {
//  dbg();
  uint32_t n;
  mmcIOStates st;


  n = MSC_MAX_PACKET;
  
  st = mmc->Read (BulkBuf, n);
  msif.ep.Write  (BulkBuf, n);
  
  if (st == mmcIOPEN) return;

  BulkStage = MSC_BS_DATA_IN_LAST;
  if (SenseData.data == SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE) {
    BulkStage = MSC_BS_DATA_IN_LAST_STALL;
  }

  if (BulkStage != MSC_BS_DATA_IN) {
    // print("res=%d\n", CSW.dDataResidue);
    if (SenseData.data == SS_NO_SENSE)
      CSW.bStatus = CSW_CMD_PASSED;
    else
      CSW.bStatus = CSW_CMD_FAILED;
  }
}
void MsClass::MSC_MemoryVerify (void) {
  uint32_t n;
  mmcIOStates   st;
  static uint8_t  tmp[MSC_MAX_PACKET];
  
//  dbg();
  st = mmc->Read(tmp, BulkLen);
  for (n = 0; n < BulkLen; n++) {
    if (tmp[n] != BulkBuf[n]) {
      MemOK = FALSE;
      break;
    }
  }
  if (st == mmcIOPEN) return;
  
  BulkStage = MSC_BS_CSW;
  if (SenseData.data == SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE) {
    MSC_SetStallEP(EndpointOut);
  }
  if (SenseData.data != SS_NO_SENSE) MemOK = 0;
  CSW.bStatus = (MemOK) ? CSW_CMD_PASSED : CSW_CMD_FAILED;
  MSC_SetCSW();
}
void MsClass::MSC_MemoryWrite (void) {
//  dbg();
  mmcIOStates st;
  
  st = mmc->Write (BulkBuf, BulkLen);
  if (st == mmcIOPEN) return;
  
  BulkStage = MSC_BS_CSW;
  if (SenseData.data == SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE) {
    MSC_SetStallEP(EndpointOut);
  }

  if (SenseData.data == SS_NO_SENSE)
    CSW.bStatus = CSW_CMD_PASSED;
  else
    CSW.bStatus = CSW_CMD_FAILED;
  MSC_SetCSW();
  // print("res=%d\n", CSW.dDataResidue);
}
bool MsClass::DataInFormat (void) {
//  dbg();
  if (CBW.dDataLength == 0) {
    CSW.bStatus = CSW_PHASE_ERROR;
    error("ZERO LEN REQ\n");
    MSC_SetCSW();
    return (FALSE);
  }
  if ((CBW.bmFlags & 0x80) == 0) {
    MSC_SetStallEP(EndpointOut);
    CSW.bStatus = CSW_PHASE_ERROR;
    error("DIR ERR\n");
    MSC_SetCSW();
    return false;
  }
  return true;
}
void MsClass::DataInTransfer (void) {
//  dbg();
/* puvodne - pri MSC_ModeSense6 nastal STALL na 0x82
  if (BulkLen >= CBW.dDataLength) {
    BulkLen = CBW.dDataLength;
    BulkStage = MSC_BS_DATA_IN_LAST;
  }
  else {
    BulkStage = MSC_BS_DATA_IN_LAST_STALL; // short or zero packet
  }
*/
  if (BulkLen >= CBW.dDataLength)
    BulkLen = CBW.dDataLength;
  BulkStage = MSC_BS_DATA_IN_LAST;
  
  msif.ep.Write (BulkBuf, BulkLen);

  CSW.dDataResidue -= BulkLen;
  CSW.bStatus = CSW_CMD_PASSED;
}
void MsClass::MSC_Inquiry (void) {
  // dbg();

  if (!DataInFormat()) return;
  print("req=%d\n", CSW.dDataResidue);
  
  static const unsigned char hdr [] = {
    0,          // Direct Access Device
    0x80,       // RMB = 1: Removable Medium
    0,          // Version: No conformance claim to standard
    0x01,
    
    36-4,       // Additional Length
    0x80,       // SCCS = 1: Storage Controller Component
    0,
    0
  };
  uint32_t i = 0,j = 8;
  bytecopy (BulkBuf + i, hdr, j);
  i += j;
  j  = 28;
  bytecopy (BulkBuf + i, "Mrazik. SDHC Card on USB 1.0", j);
  i += j;
  BulkBuf [4] = i - 4;
  BulkLen     = i;
  
  DataInTransfer();

}
void MsClass::MSC_ModeSense10 (void) {
  dbg();
  if (!DataInFormat()) return;
  static const unsigned char hdr [] = {
    0,0x6,0,0, 0,0,0,0
  };
  bytecopy (BulkBuf, hdr, 8);

  BulkLen = 8;
  DataInTransfer();
}
void MsClass::MSC_ModeSense6 (void) {
  // dbg();
  if (!DataInFormat()) return;
  print("req=%d\n", CSW.dDataResidue);
  //print("DBD=%02x,PC=%02X,SC=%02X,AL=%02X,CO=%02X\n",
  //      CBW.CB[1], CBW.CB[2], CBW.CB[3], CBW.CB[4], CBW.CB[5]);
  if (!DataInFormat()) return;
  static const unsigned char hdr [] = {
    0x03,0,0 /** 0x80 read-only */ ,0
  };
  bytecopy (BulkBuf, hdr, 4);
  
  if (ReadOnly) BulkBuf [2] = 0x80;

  BulkLen = 4;
  DataInTransfer();
}
void MsClass::MSC_ReadCapacity (void) {
  dbg();

  if (!DataInFormat()) return;
  
  // Last Logical Block
  __be32 BlockCount = mmc->GetCapacity() - 1;
  bytecopy (BulkBuf, & BlockCount, 4);
  
  print ("BlockCount=%d\n", BlockCount.get());
  
  // Block Length
  __be32 BlockSize = MSC_BlockSize;
  bytecopy (BulkBuf + 4, & BlockSize, 4);

  BulkLen = 8;
  DataInTransfer();
}
void MsClass::MSC_ReadFormatCapacity (void) {
  dbg();

  if (!DataInFormat()) return;
  
  __be32 BlockCount = mmc->GetCapacity();

  BulkBuf[ 0] = 0x00;
  BulkBuf[ 1] = 0x00;
  BulkBuf[ 2] = 0x00;
  BulkBuf[ 3] = 0x08;          /* Capacity List Length */

  /* Block Count */
  bytecopy (BulkBuf + 4, & BlockCount, 4);

  /* Block Length */
  BulkBuf[ 8] = 0x02;          /* Descriptor Code: Formatted Media */
  BulkBuf[ 9] = (MSC_BlockSize >> 16) & 0xFF;
  BulkBuf[10] = (MSC_BlockSize >>  8) & 0xFF;
  BulkBuf[11] = (MSC_BlockSize >>  0) & 0xFF;

  BulkLen = 12;
  DataInTransfer();
}
bool MsClass::MSC_ReadSetup (void) {
  uint32_t m, n=0;
  // dbg();
  /* Logical Block Address of First Block */
  m = (CBW.CB[2] << 24) |
      (CBW.CB[3] << 16) |
      (CBW.CB[4] <<  8) |
      (CBW.CB[5] <<  0);

  /* Number of Blocks to transfer */
  switch (CBW.CB[0]) {
    case SCSI_READ10:
    case SCSI_VERIFY10:
      n = (CBW.CB[7] <<  8) |
          (CBW.CB[8] <<  0);
      break;

    case SCSI_READ12:
      n = (CBW.CB[6] << 24) |
          (CBW.CB[7] << 16) |
          (CBW.CB[8] <<  8) |
          (CBW.CB[9] <<  0);
      break;
  }

  if (CBW.dDataLength == 0) {              /* host requests no data */
    error("HOST REQ NO DATA\n");
    CSW.bStatus = CSW_CMD_FAILED;
    MSC_SetCSW();
    return (FALSE);
  }

  if (CBW.dDataLength != n * MSC_BlockSize) {
    if ((CBW.bmFlags & 0x80) != 0) {       /* stall appropriate EP */ 
      MSC_SetStallEP(EndpointIn);
    } else {
      MSC_SetStallEP(EndpointOut);
    }
    error("BAD REQ LEN %d\n", n);
    CSW.bStatus = CSW_CMD_FAILED;
    MSC_SetCSW();

    return (FALSE);
  }
  
  mmc->CmdRead (m, n);
  return true;
}
void MsClass::MSC_RequestSense (void) {
  uint32_t n;
  print("Sense Key [0x%02X] Asc [0x%02X]\n", SK(SenseData.data), ASC(SenseData.data));
  if (!DataInFormat()) return;
  for (n=0; n<18; n++) BulkBuf[n] = 0;                      // memset
  BulkBuf[0] = SenseData.valid | 0x70;                  /* Valid, current error     */
  BulkBuf[2] = SK(SenseData.data);
  BulkBuf[3] = (uint8_t) ((SenseData.info >> 24) & 0xFF);  /* Sense information MSB */
  BulkBuf[4] = (uint8_t) ((SenseData.info >> 16) & 0xFF);  /* Sense information     */
  BulkBuf[5] = (uint8_t) ((SenseData.info >>  8) & 0xFF);  /* Sense information     */
  BulkBuf[6] = (uint8_t)  (SenseData.info        & 0xFF);  /* Sense information LSB */
  BulkBuf[7] = 18 - 8;                                      /* Additional sense length  */
  BulkBuf[12] =  ASC(SenseData.data);
  BulkBuf[13] = ASCQ(SenseData.data);
  
  SetSenseData (SS_NO_SENSE, 0, 0);                     // Clear sense data
  BulkLen = 18;
  DataInTransfer();

}
void MsClass::MSC_TestUnitReady (void) {
  // dbg();

  if (CBW.dDataLength != 0) {
    if ((CBW.bmFlags & 0x80) != 0) {
      MSC_SetStallEP(EndpointIn);
    } else {
      MSC_SetStallEP(EndpointOut);
    }
    error("DIR ERR\n");
  }
  // Na začátku fail - možná je to blbost, ale jiná zařízení to tak dělají
  if (SenseData.data == SS_RESET_OCCURRED)
    CSW.bStatus = CSW_CMD_FAILED;
  else
    CSW.bStatus = CSW_CMD_PASSED;
  MSC_SetCSW();
}
bool MsClass::MSC_WriteSetup (void) {
  uint32_t m, n=0;
  // dbg();
  /* Logical Block Address of First Block */
  m = (CBW.CB[2] << 24) |
      (CBW.CB[3] << 16) |
      (CBW.CB[4] <<  8) |
      (CBW.CB[5] <<  0);

  /* Number of Blocks to transfer */
  switch (CBW.CB[0]) {
    case SCSI_WRITE10:
      n = (CBW.CB[7] <<  8) |
          (CBW.CB[8] <<  0);
      break;

    case SCSI_WRITE12:
      n = (CBW.CB[6] << 24) |
          (CBW.CB[7] << 16) |
          (CBW.CB[8] <<  8) |
          (CBW.CB[9] <<  0);
      break;
  }

  if (CBW.dDataLength == 0) {              /* host requests no data */
    error("HOST REQ NO DATA\n");
    CSW.bStatus = CSW_CMD_FAILED;
    MSC_SetCSW();
    return (FALSE);
  }

  if (CBW.dDataLength != n * MSC_BlockSize) {
    if ((CBW.bmFlags & 0x80) != 0) {       /* stall appropriate EP */ 
      MSC_SetStallEP(EndpointIn);
    } else {
      MSC_SetStallEP(EndpointOut);
    }
    error("BAD REQ LEN %d\n", n);
    CSW.bStatus = CSW_CMD_FAILED;
    MSC_SetCSW();

    return (FALSE);
  }
  
  mmc->CmdWrite (m, n);
  return true;
}
extern  UsbHw * UsbHwInstance;

bool MsClass::Reset (void) {
  dbg();
  UsbDevice * dev = UsbHwInstance->GetParent();
  dev->ClrStallMask (0x00000000);          /* EP must stay stalled */
  CSW.dSignature = 0;                      /* invalid signature */

  BulkStage = MSC_BS_CBW;

  return true;
}
void MsClass::MSC_SetStallEP (UsbEndpointDir dir) {
  dbg();
  uint32_t EPNum = msif.ep.addr | 0x0F;
  msif.ep.SetStallEP  (dir);
  uint32_t   mask = (dir) ? ((1 << 16) << EPNum) : (1 << EPNum);
  UsbDevice * dev = UsbHwInstance->GetParent();
  dev->SetStallMask (mask);
}
void MsClass::MSC_SetStallEPStall (UsbEndpointDir dir) {  
  dbg();
  uint32_t EPNum = msif.ep.addr | 0x0F;
  msif.ep.SetStallEP  (dir);
  uint32_t mask   = (dir) ? ((1 << 16) << EPNum) : (1 << EPNum);
  UsbDevice * dev = UsbHwInstance->GetParent();
  dev->SetStallHaltMask (mask);
}

/** ***********************************************/

void MscIface::Handler (EndpointCallbackEvents e) {
  switch (e) {
    case USB_EVT_IN:
      parent->BulkIn ();
      break;
    case USB_EVT_OUT:
      parent->BulkOut ();
      break;
    default:
      error ("Bad Event\n");
      break;
  }
}
IfacePostRunAction MscIface::SetupHandler (USB_SETUP_PACKET * p) {
  switch (p->bRequest) {
    case MSC_REQUEST_RESET:
      if ( (p->wValue.W == 0) &&                  /* RESET with invalid parameters -> STALL */
           (p->wLength  == 0)) {
        if (parent->Reset ()) {
          return IfacePostRunActionStatusIn;
        }
      }
      break;
    case MSC_REQUEST_GET_MAX_LUN:
      if ( (p->wValue.W == 0) &&                  /* GET_MAX_LUN with invalid parameters -> STALL */
           (p->wLength  == 1)) {
        if (parent->GetMaxLUN ()) {
	  return IfacePostRunActionMscLun;
        }
      }
      break;
  }
  return IfacePostRunActionStall;
}

/** ***********************************************/

#ifdef DSCBLD
#include "dscbld.h"

unsigned int MsClass::enumerate (unsigned int in) {
  return UsbClass::enumerate (in);
}
void MsClass::blddsc (void) {
  UsbClass::blddsc();
}


unsigned int MscIface::enumerate (unsigned int in) {
  unsigned int res = in;
  res  = UsbInterface::enumerate (res);

  dsc.bNumEndpoints      = 2;
  dsc.bInterfaceClass    = 0x08;
  dsc.bInterfaceSubClass = 0x06;	// SCSI transparent command set
  dsc.bInterfaceProtocol = 0x50;	// Bulk-Only Transport
  dsc.iInterface = AddString (parent->StrDesc, StrCount); // Index Stringu

  return res;
}
void MscIface::blddsc (void) {
  fprintf (cDesc, "// Msc     Interface %d\n", dsc.bInterfaceNumber);
  UsbInterface::blddsc();
}
#endif // DSCBLD

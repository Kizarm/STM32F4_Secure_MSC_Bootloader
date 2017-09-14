#include "storagemmc.h"
#include "debug.h"
#include <msclass.h>

#include "storagemmc.h"
/// pro pouziti ve spolupraci k MMC kartou
static StorageMmc * MmcInstance; 
extern "C" void SysTick_Handler (void);

void SysTick_Handler (void) {
  if (MmcInstance)
    MmcInstance->disk_timerproc();
}
//!///////////////////////////////////////


/*! Results of Disk Functions */
typedef enum {
  RES_OK = 0,   /*!< 0: Successful */
  RES_ERROR,    /*!< 1: R/W Error */
  RES_WRPRT,    /*!< 2: Write Protected */
  RES_NOTRDY,   /*!< 3: Not Ready */
  RES_PARERR    /*!< 4: Invalid Parameter */
} DRESULT;

#define STA_NOINIT    0x01  /*!< Drive not initialized */
#define STA_NODISK    0x02  /*!< No medium in the drive */
#define STA_PROTECT   0x04  /*!< Write protected */

/*! Card type flags (CardType) */
#define CT_MMC        0x01  /*!< MMC ver 3 */
#define CT_SD1        0x02  /*!< SD ver 1 */
#define CT_SD2        0x04  /*!< SD ver 2 */
#define CT_SDC        (CT_SD1|CT_SD2) /*!< SD */
#define CT_BLOCK      0x08  /*!< Block addressing */


/** Definitions for MMC/SDC command */
#define CMD0   (0x40+0)  // GO_IDLE_STATE
#define CMD1   (0x40+1)  // SEND_OP_COND (MMC)
#define ACMD41 (0xC0+41) // SEND_OP_COND (SDC)
#define CMD8   (0x40+8)  // SEND_IF_COND
#define CMD9   (0x40+9)  // SEND_CSD
#define CMD10  (0x40+10) // SEND_CI
#define CMD12  (0x40+12) // STOP_TRANSMISSION
#define ACMD13 (0xC0+13) // SD_STATUS (SDC)
#define CMD16  (0x40+16) // SET_BLOCKLEN
#define CMD17  (0x40+17) // READ_SINGLE_BLOCK
#define CMD18  (0x40+18) // READ_MULTIPLE_BLOCK
#define CMD23  (0x40+23) // SET_BLOCK_COUNT (MMC)
#define ACMD23 (0xC0+23) // SET_WR_BLK_ERASE_COUNT (SDC)
#define CMD24  (0x40+24) // WRITE_BLOCK
#define CMD25  (0x40+25) // WRITE_MULTIPLE_BLOCK
#define CMD55  (0x40+55) // APP_CMD
#define CMD58  (0x40+58) // READ_OCR
/*
static const uint32_t FreqLow  = 7; // ~ 160 kHz
static const uint32_t FreqHigh = 2; // ~ 2.6 MHz
*/

StorageMmc::StorageMmc () : spi(), block(512) {
  MmcInstance    = this;
  DiskTimerDelay = 0;
  Open ();
}
void StorageMmc::Open (void) {
  capacity = 0;
  ofs      = 0;
  len      = 0;
  
  uint8_t n, cmd, ty, ocr[4];

  Stat = STA_NOINIT;
  // Init SSP (clock low between frames, transition on leading edge)
  spi.Init ();
  // Wait 200ms for card detect to stabilise
  dmsDelay (20);
  spi.Clock(SpiClockLow);
  for (n = 100; n; n--) spi.Read(); // rcvr_spi();                   /* 80 dummy clocks */

  ty = 0;
  if (send_cmd (CMD0, 0) == 1) {                      /* Enter Idle state */
    Timer1 = 100;                                 /* Initialization timeout of 1000 msec */
    if (send_cmd (CMD8, 0x1AA) == 1) {                /* SDHC */
      for (n = 0; n < 4; n++) ocr[n] = spi.Read();    /* Get trailing return value of R7 resp */
      if (ocr[2] == 0x01 && ocr[3] == 0xAA) {         /* The card can work at vdd range of 2.7-3.6V */
        while (Timer1 && send_cmd (ACMD41, 1UL << 30)); /* Wait for leaving idle state (ACMD41 with HCS bit) */
        if (Timer1 && send_cmd (CMD58, 0) == 0) { /* Check CCS bit in the OCR */
          for (n = 0; n < 4; n++) ocr[n] = spi.Read();
          ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2; /* SDv2 */
        }
      }
    } else {                                          /* SDSC or MMC */
      if (send_cmd (ACMD41, 0) <= 1)  {
        ty = CT_SD1;
        cmd = ACMD41;                                 /* SDv1 */
      } else {
        ty = CT_MMC;
        cmd = CMD1;                                   /* MMCv3 */
      }
      while (Timer1 && send_cmd (cmd, 0));        /* Wait for leaving idle state */
      if (!Timer1 || send_cmd (CMD16, 512) != 0)  /* Set R/W block length to 512 */
        ty = 0;
    }
  }
  CardType = ty;
  deselect();

  if (ty) {                                       /* Initialization succeded */
    Stat &= ~STA_NOINIT;                          /* Clear STA_NOINIT */
    spi.Clock(SpiClockHigh);
  }
  print("Card type = %02X\n", CardType);
  if (!Stat) {
    capacity = get_disk_capacity();
    print("Capacity  = %d sect.\n", capacity);
  }

}

mmcIOStates StorageMmc::Read (uint8_t * buf, uint32_t lenght) {
  uint8_t token = 0xFF;
  
  if (capacity < ofs) {
    parent->SetSenseData(SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE, ofs, 1);
  }
  if (ok) spi.Recv (buf, lenght);
  // print("len=%d\n", len);
  len -= lenght;
  if (len) {
    if (mbk) {
      bct -= lenght;
      if (!bct) {           // block received
        bct  = block;
        ofs += 1;
        spi.Read();                           /* Discard CRC */
        spi.Read();
        // print("ofs=%d\n", ofs);
        // new read prepare
        Timer1 = 20;
        do {                                  /* Wait for data packet in timeout of 200ms */
          token = spi.Read();
        } while ((token == 0xFF) && Timer1);
        if (token != 0xFE) {
          error("Bad Token 0x%02X\n", token);
          parent->SetSenseData(SS_UNRECOVERED_READ_ERROR, ofs, 1);
          return mmcIOPEN;                     /* If not valid data token, retutn with error */
        }
        parent->DecResidue(block);
      }
    }
  }
  else {    // end of read
    spi.Read();                                 /* Discard CRC */
    spi.Read();
    if (mbk) {
       token = send_cmd (CMD12, 0);             /* STOP_TRANSMISSION */
    }
    deselect();
    parent->DecResidue(block);
    // print("End, token=0x%02X\n", token);
    return mmcIOEND;
  }
  return mmcIOPEN;
}
mmcIOStates StorageMmc::Write (uint8_t * buf, uint32_t lenght) {
  uint8_t resp;
  
  if (capacity < ofs) {
    parent->SetSenseData(SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE, ofs, 1);
  }
  if (ok) spi.Send (buf, lenght);
  
  len -= lenght;
  if (len) {
    if (mbk) {
      bct -= lenght;
      if (!bct) {           // block transmited
        bct  = block;
        ofs += 1;
        spi.Write (0xFF);                        /* CRC (Dummy) */
        spi.Write (0xFF);
        resp = spi.Read();                      /* Reveive data response */
        if ((resp & 0x1F) != 0x05) {            /* If not accepted, return with error */
          error("Not accepted at %d\n", ofs);
          parent->SetSenseData(SS_WRITE_ERROR, ofs, 1);
        }
        // new write prepare
        if (wait_ready() != 0xFF) {             // Timeout
          error("Timeout\n");
          parent->SetSenseData(SS_NOT_READY_TO_READY_TRANSITION, ofs, 1);
          return mmcIOPEN;
        }
        spi.Write (0xFC);                        /* Xmit data token */
        parent->DecResidue(block);
      }
    }
  }
  else {    // end of write
    spi.Write (0xFF);                            /* CRC (Dummy) */
    spi.Write (0xFF);
    resp = spi.Read();                      /* Reveive data response */
    if ((resp & 0x1F) != 0x05) {            /* If not accepted, return with error */
      error("Not accepted at end %d\n", ofs);
      parent->SetSenseData(SS_WRITE_ERROR, ofs, 1);
      return mmcIOEND;
    }
    if (mbk) {
      if (wait_ready() != 0xFF) {
        error("End Timeout\n");
        parent->SetSenseData(SS_NOT_READY_TO_READY_TRANSITION, ofs, 1);
        return mmcIOEND;
      }
      spi.Write (0xFD);
    }
    parent->DecResidue(block);
    deselect();
    return mmcIOEND;
  }
  return mmcIOPEN;
}
static uint32_t ReadCount = 0;
void StorageMmc::CmdRead (uint32_t offset, uint32_t lenght) {
  uint8_t token;
  ReadCount++;
  
  ofs = offset;
  len = lenght * block;
  ok  = 0;
  
  if (!lenght) {
    parent->SetSenseData(SS_COMMUNICATION_FAILURE, ofs, 1);
    return;
  }
  if (Stat & STA_NOINIT) {
    parent->SetSenseData(SS_MEDIUM_NOT_PRESENT, ofs, 1);
    return;
  }

  if (! (CardType & CT_BLOCK)) offset *= block; /* Convert to byte address if needed */

  mbk = 0;
  bct = block;
  if (lenght == 1) {                              /* Single block read */
    token = send_cmd (CMD17, offset);             /* READ_SINGLE_BLOCK */
  } else {                                        /* Multiple block read */
    mbk = 1;
    token = send_cmd (CMD18, offset);             /* READ_MULTIPLE_BLOCK */
  }                                               // token = cmd response, 0=OK.
  
  print(" ofs=%6d, len=%d, token=0x%02X\n", offset, lenght, token);
  Timer1 = 20;
  do {                                            /* Wait for data packet in timeout of 200ms */
    token = spi.Read();
  } while ((token == 0xFF) && Timer1);
  if (token != 0xFE) {
    asm volatile ("bkpt 0");
    error("Bad Token 0x%02X\n", token);
    parent->SetSenseData(SS_UNRECOVERED_READ_ERROR, ofs, 1);
    return ;                                      /* If not valid data token, retutn with error */
  }
  parent->SetSenseData(SS_NO_SENSE, 0, 0);
  ok = 1;

}

void StorageMmc::CmdWrite (uint32_t offset, uint32_t lenght) {
  uint8_t token;
  
  ofs = offset;
  len = lenght * block;
  ok  = 0;
  
  if (Stat & STA_PROTECT) return;

  if (!lenght) {
    parent->SetSenseData(SS_COMMUNICATION_FAILURE, ofs, 1);
    return;
  }
  if (Stat & STA_NOINIT) {
    parent->SetSenseData(SS_MEDIUM_NOT_PRESENT, ofs, 1);
    return;
  }
  if (Stat & STA_PROTECT) {
    parent->SetSenseData(SS_WRITE_PROTECTED, ofs, 1);
    return;
  }
  if (!(CardType & CT_BLOCK)) offset *= 512;  /* Convert to byte address if needed */

  mbk = 0;
  bct = block;
  if (lenght == 1) {                              /* Single block write */
    token = send_cmd (CMD24, offset);             /* WRITE_BLOCK */
  } else {                                        /* Multiple block write */
    mbk = 1;
    if (CardType & CT_SDC) send_cmd (ACMD23, lenght);
    token = send_cmd (CMD25, offset);             /* WRITE_MULTIPLE_BLOCK */
  }
  (void) token;
  print("ofs=%6d, len=%d, token=0x%02X\n", offset, lenght, token);
  if (wait_ready() != 0xFF) {           // Timeout
    parent->SetSenseData(SS_NOT_READY_TO_READY_TRANSITION, ofs, 1);
    error("Timeout\n");
    return;
  }

  if (mbk)
    spi.Write (0xFC);                    /* Xmit data token */
  else
    spi.Write (0xFE);                    /* Xmit data token */
    
  parent->SetSenseData(SS_NO_SENSE, 0, 0);
  ok = 1;

}
uint32_t StorageMmc::GetCapacity (void) {
  return capacity;
}

/** ************************************************/
uint8_t StorageMmc::wait_ready (void) {
  uint8_t res;


  Timer2 = 50; /* Wait for ready in timeout of 500ms */
  spi.Read();
  do
    res = spi.Read();
  while ( (res != 0xFF) && Timer2);

  return res;

}
void StorageMmc::deselect (void) {
  spi.Select(false);
  spi.Read();
}
bool StorageMmc::select (void) {
  spi.Select(true);
  if (wait_ready() != 0xFF) {
    deselect();
    return false;
  }
  return true;

}
bool StorageMmc::rcvr_datablock (uint8_t * buff, uint32_t btr) {
  uint8_t token;


  Timer1 = 20;
  do {       /* Wait for data packet in timeout of 200ms */
    token = spi.Read();
  } while ( (token == 0xFF) && Timer1);
  if (token != 0xFE) return FALSE; /* If not valid data token, retutn with error */

  do {       /* Receive the data block into buffer */
    *buff++ = spi.Read ();
    *buff++ = spi.Read ();
    *buff++ = spi.Read ();
    *buff++ = spi.Read ();
  } while (btr -= 4);
  spi.Read();      /* Discard CRC */
  spi.Read();

  return true;     /* Return with success */

}
uint8_t StorageMmc::send_cmd (uint8_t cmd, uint32_t arg) {
  uint8_t n, res;
  uint8_t com[6];


  if (cmd & 0x80) { /* ACMD<n> is the command sequense of CMD55-CMD<n> */
    cmd &= 0x7F;
    res = send_cmd (CMD55, 0);
    if (res > 1) return res;
  }

  /* Select the card and wait for ready */
  deselect();
  if (!select()) return 0xFF;

  com[0] = cmd;
  com[1] = (uint8_t) (arg >> 24);
  com[2] = (uint8_t) (arg >> 16);
  com[3] = (uint8_t) (arg >>  8);
  com[4] = (uint8_t) (arg);
  /* Send command packet */
  n = 0x01;                    /* Dummy CRC + Stop */
  if (cmd == CMD0) n = 0x95;   /* Valid CRC for CMD0(0) */
  if (cmd == CMD8) n = 0x87;   /* Valid CRC for CMD8(0x1AA) */

  com[5] = n;
  spi.Send (com, 6);
  /* Receive command response */
  if (cmd == CMD12) spi.Read();  /* Skip a stuff byte when stop reading */
  n = 10;        /* Wait for a valid response in timeout of 10 attempts */
  do
    res = spi.Read();
  while ( (res & 0x80) && --n);

  return res;   /* Return with the response value */
}
uint32_t StorageMmc::get_disk_capacity (void) {
  uint8_t  n, csd[16];
  uint16_t  csize;
  uint32_t rc = 0;
  
  if ( (send_cmd (CMD9, 0) == 0) && rcvr_datablock (csd, 16)) {
    if ( (csd[0] >> 6) == 1) { /* SDC ver 2.00 */
      csize = csd[9] + ( (uint16_t) csd[8] << 8) + 1;
      rc = (uint32_t) csize << 10;
    } else {     /* SDC ver 1.XX or MMC*/
      n = (csd[5] & 15) + ( (csd[10] & 128) >> 7) + ( (csd[9] & 3) << 1) + 2;
      csize = (csd[8] >> 6) + ( (uint16_t) csd[7] << 2) + ( (uint16_t) (csd[6] & 3) << 10) + 1;
      rc = (uint32_t) csize << (n - 9);
    }
  }
  return rc;
}

void StorageMmc::dmsDelay (int dms) {
  DiskTimerDelay = dms;
  while (DiskTimerDelay);
}
void StorageMmc::disk_timerproc (void) {
  // dbg();
  uint8_t n;
  if (DiskTimerDelay) DiskTimerDelay--;

  n = Timer1;      /* 100Hz decrement timer */
  if (n) Timer1 = --n;
  n = Timer2;
  if (n) Timer2 = --n;
}

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "msclass.h"
#include "storagefile.h"
#include "debug.h"

void StorageFile::Open (void) {
  int ret;
  struct stat st;
  mmccapacity = 0;
  mmcblock = 512;
  mmclen   = 0;
  mmcofs   = 0;
  mmcfd    = open (file, O_RDWR);
  if (mmcfd < 0) {
    fprintf (stderr, "Cannot open image file %s\n", file);
    return;
  }
  ret = fstat (mmcfd, &st);
  if (ret) {
    fprintf(stderr, "Cannot stat %s, %s\n", file, strerror(errno));
    return;
  }
  mmccapacity = (st.st_size / mmcblock);
}
void StorageFile::Close (void) {
  print ("StorageFile \"%s\"\n", file);
  close (mmcfd);
}


mmcIOStates StorageFile::Write (uint8_t * buf, uint32_t len) {
  size_t rc;
  if ((mmcblock * mmccapacity) < (mmcofs + len)) {
    parent->SetSenseData (SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE, mmcofs / mmcblock, 1);
    // return mmcIOEND;  // tt. případ reálně nenastává, možná bude nutno veškerý zápis dokončit
                      // jinak se poruší sekvence CBW CSW.
  }
  rc = write (mmcfd, buf, len);
  if (rc != len) {
    fprintf(stderr, "Write error\n");
    parent->SetSenseData (SS_WRITE_ERROR, mmcofs / mmcblock, 1);
  }
  mmcofs += len;
  mmclen -= len;
  /*
  // SS_WRITE_ERROR není reportováno systémem, host provede retry
  if (((mmcofs / 512) == 41) && (!xerr)) {    // simulace chyby na tt. adrese
    xerr = 1;
    parent->SetSenseData(SS_WRITE_ERROR, mmcofs / mmcblock, 1);
  }
  */
  parent->DecResidue(len);
  if (!mmclen) {
    return mmcIOEND;
  }
  return mmcIOPEN;
}

mmcIOStates StorageFile::Read (uint8_t * buf, uint32_t len) {
  size_t rc;
  if ((mmcblock * mmccapacity) < (mmcofs + len)) {
    parent->SetSenseData(SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE, mmcofs / mmcblock, 1);
    // return mmcIOEND;  // tt. případ reálně nenastává, možná bude nutno veškeré čtení dokončit
                         // jinak se poruší sekvence CBW CSW.
  }
  rc = read (mmcfd, buf, len);
  if (rc != len) {
    fprintf(stderr, "Read error\n");
    parent->SetSenseData(SS_UNRECOVERED_READ_ERROR, mmcofs / mmcblock, 1);
  }
  mmcofs += len;
  mmclen -= len;
  /*
  // Při read je chyba 0x03 reportována systémem, ale host neopakuje pokus o nové čtení
  if (((mmcofs / 512) == 250) && (!xerr)) {    // simulace chyby na tt. adrese
    xerr = 1;
    parent->SetSenseData(SS_UNRECOVERED_READ_ERROR, mmcofs / mmcblock, 1);
  }
  */
  parent->DecResidue(len);
  if (!mmclen) {
    return mmcIOEND;
  }
  return mmcIOPEN;
}

void StorageFile::CmdWrite (uint32_t offset, uint32_t lenght) {
  off_t rc;
  mmcofs = mmcblock * offset;
  mmclen = mmcblock * lenght;
  rc = lseek(mmcfd, mmcofs, SEEK_SET);
  if (rc != mmcofs) fprintf(stderr, "Seek error\n");
  print("ofs=%6d, len=%d\n", offset, lenght);
  parent->SetSenseData (SS_NO_SENSE, 0, 0);  
}

void StorageFile::CmdRead (uint32_t offset, uint32_t lenght) {
  off_t rc;
  mmcofs = mmcblock * offset;
  mmclen = mmcblock * lenght;
  rc = lseek(mmcfd, mmcofs, SEEK_SET);
  if (rc != mmcofs) fprintf(stderr, "Seek error\n");
  print(" ofs=%6d, len=%d\n", offset, lenght);
  parent->SetSenseData (SS_NO_SENSE, 0, 0);  
}

uint32_t StorageFile::GetCapacity (void) {
  return mmccapacity;

}

#include <string.h>
#include "msclass.h"
#include "storagememory.h"
#include "debug.h"

extern "C" {
  // image.s
  extern uint8_t  image_disc[];
  extern uint32_t image_size;
};

StorageMemory::StorageMemory () : mmcblock(512), image(image_disc) {
  mmclen   = 0;
  mmcofs   = 0;
  mmccapacity = (image_size / mmcblock);
}


mmcIOStates StorageMemory::Write (uint8_t * buf, uint32_t len) {
  if ((mmcblock * mmccapacity) < (mmcofs + len)) {
    parent->SetSenseData (SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE, mmcofs / mmcblock, 1);
    // return mmcIOEND;  // tt. případ reálně nenastává, možná bude nutno veškerý zápis dokončit
                         // jinak se poruší sekvence CBW CSW.
  }
  memcpy (image + mmcofs, buf, len);
  mmcofs   += len;
  mmclen   -= len;
  parent->DecResidue(len);
  if (!mmclen) {
    return mmcIOEND;
  }
  return mmcIOPEN;
}

mmcIOStates StorageMemory::Read (uint8_t * buf, uint32_t len) {
  if ((mmcblock * mmccapacity) < (mmcofs + len)) {
    parent->SetSenseData(SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE, mmcofs / mmcblock, 1);
    // return mmcIOEND;  // tt. případ reálně nenastává, možná bude nutno veškeré čtení dokončit
                         // jinak se poruší sekvence CBW CSW.
  }
  memcpy (buf, image + mmcofs, len);
  mmcofs   += len;
  mmclen   -= len;
  parent->DecResidue(len);
  if (!mmclen) {
    return mmcIOEND;
  }
  return mmcIOPEN;
}

void StorageMemory::CmdWrite (uint32_t offset, uint32_t lenght) {
  mmcofs = mmcblock * offset;
  mmclen = mmcblock * lenght;
  print("ofs=%6d, len=%d\n", offset, lenght);
  parent->SetSenseData (SS_NO_SENSE, 0, 0);  
}

void StorageMemory::CmdRead (uint32_t offset, uint32_t lenght) {
  mmcofs = mmcblock * offset;
  mmclen = mmcblock * lenght;
  print(" ofs=%6d, len=%d\n", offset, lenght);
  parent->SetSenseData (SS_NO_SENSE, 0, 0);  
}

uint32_t StorageMemory::GetCapacity (void) {
  return mmccapacity;
}

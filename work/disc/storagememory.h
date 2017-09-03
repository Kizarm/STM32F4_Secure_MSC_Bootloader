#ifndef STORAGEMEMORY_H
#define STORAGEMEMORY_H

#include "storagebase.h"

class StorageMemory : public StorageBase {
  public:
    StorageMemory ();
    mmcIOStates Write (uint8_t * buf, uint32_t len);
    mmcIOStates Read  (uint8_t * buf, uint32_t len);
    void CmdWrite     (uint32_t offset, uint32_t lenght);
    void CmdRead      (uint32_t offset, uint32_t lenght);
    uint32_t GetCapacity (void);
  private:
    const uint32_t mmcblock;            //!< 512, délka bloku
    uint32_t       mmccapacity;         //!< kapacita disku = délka souboru
    uint32_t       mmcofs;              //!< offset
    uint32_t       mmclen;              //!< délka

    uint8_t   *    image;
};

#endif // STORAGEMEMORY_H

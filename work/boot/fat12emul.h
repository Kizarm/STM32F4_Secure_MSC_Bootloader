#ifndef FAT12EMUL_H
#define FAT12EMUL_H
#include <stdint.h>
#include "storagebase.h"
#include "imagewriter.h"

class Fat12Emul : public StorageBase {
  public:
    Fat12Emul  ();
    mmcIOStates Write (uint8_t * buf, uint32_t len);
    mmcIOStates Read  (uint8_t * buf, uint32_t len);
    void CmdWrite     (uint32_t offset, uint32_t lenght);
    void CmdRead      (uint32_t offset, uint32_t lenght);
    uint32_t GetCapacity (void);
    void Prepare  (void);
  protected:
    void systRead     (uint8_t * buf, uint32_t len);
    void zeroRead     (uint8_t * buf, uint32_t len);
    void indxRead     (uint8_t * buf, uint32_t len);
    void fileRead     (uint8_t * buf, uint32_t len);
    
    void systWrite     (uint8_t * buf, uint32_t len);
    void zeroWrite     (uint8_t * buf, uint32_t len);
    void indxWrite     (uint8_t * buf, uint32_t len);
    void fileWrite     (uint8_t * buf, uint32_t len);
  private:
    ImageWriter    memory;
    const uint32_t block;            //!< 512, délka bloku
    uint32_t       capacity;         //!< kapacita disku = délka souboru + hlavicky
    uint32_t       rwofs;            //!< offset
    uint32_t       rwlen;            //!< délka
    uint32_t       pt0, pt1, pt2;    //!< záchytné body
    uint32_t       write_size;
    
    struct {    // 2 KiB data
      uint8_t boot [512];   //!< boot sector
      uint8_t fat  [512];   //!< FAT tables block
      uint8_t dir  [512];   //!< DIRECTORY  block
      uint8_t data [512];   //!< pro univerzalni pouziti (boot, data)
    } IO;
};

#endif // FAT12EMUL_H

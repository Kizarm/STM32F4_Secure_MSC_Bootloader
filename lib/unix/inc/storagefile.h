#ifndef STORAGEFILE_H
#define STORAGEFILE_H

#include "storagebase.h"

class StorageFile : public StorageBase {
  public:
    StorageFile (const char * name) : StorageBase(), file (name) {};
    void     Open         (void);
    void     Close        (void);
    
    mmcIOStates Write       (uint8_t * buf, uint32_t len);
    mmcIOStates Read        (uint8_t * buf, uint32_t len);
    void        CmdWrite    (uint32_t offset, uint32_t lenght);
    void        CmdRead     (uint32_t offset, uint32_t lenght);
    uint32_t    GetCapacity (void);
  private:
    const char * file;

    uint32_t    mmccapacity;         //!< kapacita disku = délka souboru
    uint32_t    mmcblock;            //!< 512, délka bloku
    int         mmcfd;               //!< filedeskriptor použitého souboru
    uint32_t    mmcofs;              //!< offset
    uint32_t    mmclen;              //!< délka
    
};

#endif // STORAGEFILE_H

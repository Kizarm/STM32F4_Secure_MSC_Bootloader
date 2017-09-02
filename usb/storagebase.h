#ifndef STORAGEBASE_H
#define STORAGEBASE_H

#include <stdint.h>

enum mmcIOStates {
  mmcIOPEN = 0,             //!< IO pending
  mmcIOEND                  //!< IO end
};

class MsClass;

class StorageBase {
  public:
    StorageBase () {};
    // virtual void     Open         (void)=0;
    // virtual void     Close        (void)=0;
    virtual uint32_t GetCapacity  (void)=0;
    virtual void     CmdRead      (uint32_t offset, uint32_t lenght)=0;
    virtual void     CmdWrite     (uint32_t offset, uint32_t lenght)=0;
    virtual mmcIOStates  Read   (uint8_t* buf, uint32_t len)=0;
    virtual mmcIOStates  Write  (uint8_t* buf, uint32_t len)=0;
  public:
    MsClass * parent;
  
};

#endif // STORAGEBASE_H

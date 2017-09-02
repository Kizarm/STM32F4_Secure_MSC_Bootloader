#ifndef FLASH_H
#define FLASH_H

  typedef enum {
    FLASH_BUSY = 1,
    FLASH_ERROR_PGS,
    FLASH_ERROR_PGP,
    FLASH_ERROR_PGA,
    FLASH_ERROR_WRP,
    FLASH_ERROR_PROGRAM,
    FLASH_ERROR_OPERATION,
    FLASH_COMPLETE
  } FLASH_Status;
  

class FLASHClass {
  public:
    static void         Unlock               (void);
    static bool         isLocked             (void);
    static FLASH_Status GetStatus            (void);
    static FLASH_Status WaitForLastOperation (void);
    static FLASH_Status EraseSector          (uint32_t FLASH_Sector);
    static FLASH_Status ProgramHalfWord      (uint32_t Address, uint16_t Data);
};

#endif // FLASH_H

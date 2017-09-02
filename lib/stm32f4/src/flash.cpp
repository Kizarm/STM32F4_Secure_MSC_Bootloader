#include <stdint.h>
#include "flash.h"

  typedef struct {
    volatile uint32_t ACR;      /*!< FLASH access control register, Address offset: 0x00 */
    volatile uint32_t KEYR;     /*!< FLASH key register,            Address offset: 0x04 */
    volatile uint32_t OPTKEYR;  /*!< FLASH option key register,     Address offset: 0x08 */
    volatile uint32_t SR;       /*!< FLASH status register,         Address offset: 0x0C */
    volatile uint32_t CR;       /*!< FLASH control register,        Address offset: 0x10 */
    volatile uint32_t OPTCR;    /*!< FLASH option control register, Address offset: 0x14 */
  } FLASH_TypeDef;

  #define FLASH_FLAG_EOP           ((uint32_t)0x00000001)  /*!< FLASH End of Operation flag */
  #define FLASH_FLAG_OPERR         ((uint32_t)0x00000002)  /*!< FLASH operation Error flag */
  #define FLASH_FLAG_WRPERR        ((uint32_t)0x00000010)  /*!< FLASH Write protected error flag */
  #define FLASH_FLAG_PGAERR        ((uint32_t)0x00000020)  /*!< FLASH Programming Alignment error flag */
  #define FLASH_FLAG_PGPERR        ((uint32_t)0x00000040)  /*!< FLASH Programming Parallelism error flag  */
  #define FLASH_FLAG_PGSERR        ((uint32_t)0x00000080)  /*!< FLASH Programming Sequence error flag  */
  #define FLASH_FLAG_BSY           ((uint32_t)0x00010000)  /*!< FLASH Busy flag */
  #define FLASH_PSIZE_BYTE         ((uint32_t)0x00000000)
  #define FLASH_PSIZE_HALF_WORD    ((uint32_t)0x00000100)
  #define FLASH_PSIZE_WORD         ((uint32_t)0x00000200)
  #define FLASH_PSIZE_DOUBLE_WORD  ((uint32_t)0x00000300)
  #define CR_PSIZE_MASK            ((uint32_t)0xFFFFFCFF)
  #define SECTOR_MASK              ((uint32_t)0xFFFFFF07)
  /*******************  Bits definition for FLASH_SR register  ******************/
  #define FLASH_SR_EOP                         ((uint32_t)0x00000001)
  #define FLASH_SR_SOP                         ((uint32_t)0x00000002)
  #define FLASH_SR_WRPERR                      ((uint32_t)0x00000010)
  #define FLASH_SR_PGAERR                      ((uint32_t)0x00000020)
  #define FLASH_SR_PGPERR                      ((uint32_t)0x00000040)
  #define FLASH_SR_PGSERR                      ((uint32_t)0x00000080)
  #define FLASH_SR_BSY                         ((uint32_t)0x00010000)

  /*******************  Bits definition for FLASH_CR register  ******************/
  #define FLASH_CR_PG                          ((uint32_t)0x00000001)
  #define FLASH_CR_SER                         ((uint32_t)0x00000002)
  #define FLASH_CR_MER                         ((uint32_t)0x00000004)
  #define FLASH_CR_SNB_0                       ((uint32_t)0x00000008)
  #define FLASH_CR_SNB_1                       ((uint32_t)0x00000010)
  #define FLASH_CR_SNB_2                       ((uint32_t)0x00000020)
  #define FLASH_CR_SNB_3                       ((uint32_t)0x00000040)
  #define FLASH_CR_PSIZE_0                     ((uint32_t)0x00000100)
  #define FLASH_CR_PSIZE_1                     ((uint32_t)0x00000200)
  #define FLASH_CR_STRT                        ((uint32_t)0x00010000)
  #define FLASH_CR_EOPIE                       ((uint32_t)0x01000000)
  #define FLASH_CR_LOCK                        ((uint32_t)0x80000000)

  static const uint32_t FLASH_KEY1 = 0x45670123u;
  static const uint32_t FLASH_KEY2 = 0xCDEF89ABu;
  static FLASH_TypeDef * const FLASH = reinterpret_cast<FLASH_TypeDef * const> (0x40023C00u);

  void FLASHClass::Unlock (void) {
    if (FLASH->CR & FLASH_CR_LOCK) {
      /* Authorize the FLASH Registers access */
      FLASH->KEYR = FLASH_KEY1;
      FLASH->KEYR = FLASH_KEY2;
    }
  }
  bool FLASHClass::isLocked (void) {
    if (FLASH->CR & FLASH_CR_LOCK) return true;
    else                           return false;
  }
  FLASH_Status FLASHClass::GetStatus (void) {
    if ( (FLASH->SR & FLASH_FLAG_BSY) == FLASH_FLAG_BSY) {
      return FLASH_BUSY;
    } else  if ( (FLASH->SR & FLASH_FLAG_WRPERR) != (uint32_t) 0x00) {
      return FLASH_ERROR_WRP;
    } else if ( (FLASH->SR & (uint32_t) 0xEF)  != (uint32_t) 0x00) {
      return FLASH_ERROR_PROGRAM;
    } else if ( (FLASH->SR & FLASH_FLAG_OPERR) != (uint32_t) 0x00) {
      return FLASH_ERROR_OPERATION;
    } else {
      return FLASH_COMPLETE;
    }
  }
  FLASH_Status FLASHClass::WaitForLastOperation (void) {
    volatile FLASH_Status status = FLASH_COMPLETE;
    /* Check for the FLASH Status */
    status = GetStatus();
    /* Wait for the FLASH operation to complete by polling on BUSY flag to be reset.
      Even if the FLASH operation fails, the BUSY flag will be reset and an error
      flag will be set */
    while (status == FLASH_BUSY) {
      status = GetStatus();
    }
    /* Return the operation status */
    return status;
  }
  // FLASH_Sector 1 .. 11 (0. vynechavat)
  FLASH_Status FLASHClass::EraseSector (uint32_t FLASH_Sector) {
    volatile FLASH_Status status = FLASH_COMPLETE;
    if (isLocked()) return status;
    /* Wait for last operation to be completed */
    status = WaitForLastOperation();
    if (status == FLASH_COMPLETE) {
      //print << "Erase sect. " << FLASH_Sector << EOL;
      /* if the previous operation is completed, proceed to erase the sector */
      FLASH->CR &= CR_PSIZE_MASK;
      FLASH->CR |= FLASH_PSIZE_HALF_WORD;
      FLASH->CR &= SECTOR_MASK;
      FLASH->CR |= FLASH_CR_SER | (FLASH_Sector << 3);
      FLASH->CR |= FLASH_CR_STRT;
      /* Wait for last operation to be completed */
      status = WaitForLastOperation();
      // asm volatile ("bkpt 0");  // FLASH_COMPLETE !!!
      /* if the erase operation is completed, disable the SER Bit */
      FLASH->CR &= (~FLASH_CR_SER);
      FLASH->CR &= SECTOR_MASK;
    }
    /* Return the Erase Status */
    return status;
  }
  FLASH_Status FLASHClass::ProgramHalfWord (uint32_t Address, uint16_t Data) {
    FLASH_Status status = FLASH_COMPLETE;
    /* Wait for last operation to be completed */
    status = WaitForLastOperation();
    if (status == FLASH_COMPLETE) {
      /* if the previous operation is completed, proceed to program the new data */
      FLASH->CR &= CR_PSIZE_MASK;
      FLASH->CR |= FLASH_PSIZE_HALF_WORD;
      FLASH->CR |= FLASH_CR_PG;

      * (volatile uint16_t*) Address = Data;
      /* Wait for last operation to be completed */
      status = WaitForLastOperation();
      /* if the program operation is completed, disable the PG Bit */
      FLASH->CR &= (~FLASH_CR_PG);
    }
    /* Return the Program Status */
    return status;
  }

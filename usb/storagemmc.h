#ifndef STORAGEMMC_H
#define STORAGEMMC_H
#include "storagebase.h"
#include "spisimple.h"

/** @file
 * @brief MMC / SDHC karta připojená přes SPI.
 * 
 * Celkově převzato od elm-chan, chodí celkem bez problémů.
 * Potřebuje kromě SPI ještě časovač pro disk_timerproc(),
 * což je zde řešeno v modulu SpiSimple.
 * */

/*! Status of Disk Functions */
typedef uint8_t  DSTATUS;

class StorageMmc : public StorageBase {
  public:
    StorageMmc();
    void        Open  (void);
    mmcIOStates Write (uint8_t * buf, uint32_t len);
    mmcIOStates Read  (uint8_t * buf, uint32_t len);
    void CmdWrite     (uint32_t offset, uint32_t lenght);
    void CmdRead      (uint32_t offset, uint32_t lenght);
    uint32_t GetCapacity (void);
    /// volat v systick po 100 ms.
    void disk_timerproc (void);
    /*
  public:
    StorageMmc * instance;
    */
  protected:
    uint8_t  wait_ready        (void);
    void     deselect          (void);
    bool     select            (void);
    bool     rcvr_datablock    (uint8_t *buff, uint32_t btr);
    uint8_t  send_cmd          (uint8_t cmd, uint32_t arg);
    uint32_t get_disk_capacity (void);
    
    void dmsDelay       (int dms);
  private:
    SpiSimple	spi;
    
    volatile int DiskTimerDelay;
    
    uint32_t    capacity;         //!< kapacita karty v blocích
    const uint32_t block;         //!< délka bloku (512)
    uint32_t    ofs;              //!< ofset je v blocích
    uint32_t    len;              //!< délka požadavku je v bytech
    uint32_t    bct;              //!< čítač bytů v rámci bloku
    uint32_t    mbk;              //!< příznak multiblokového požadavku
    
    DSTATUS           Stat;       //!< stav karty
    volatile uint8_t  Timer1;     //!< čítač pro stavovení timeout
    volatile uint8_t  Timer2;     //!< čítač pro stavovení timeout
    uint8_t           CardType;   //!< typ karty
    uint8_t           ok;         //!< příznak úspěšnosti operace
};

#endif // STORAGEMMC_H

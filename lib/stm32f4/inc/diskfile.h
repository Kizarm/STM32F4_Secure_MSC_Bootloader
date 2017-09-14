#ifndef DISKFILE_H
#define DISKFILE_H
#include "diskbase.h"
#include <stdio.h>
#include "spisimple.h"

typedef uint8_t  DSTATUS;

class DiskFile : public DiskBase {
  public:
    DiskFile();
    bool Open  (const char * name = 0);
    void Close (void);
    bool Read  (char * data, unsigned block);
    void disk_timerproc (void);
  protected:
    uint8_t  wait_ready        (void);
    void     deselect          (void);
    bool     select            (void);
    bool     rcvr_datablock    (uint8_t *buff, uint32_t btr);
    uint8_t  send_cmd          (uint8_t cmd, uint32_t arg);
    uint32_t get_disk_capacity (void);
    
    void dmsDelay       (int dms);
  private:
    SpiSimple spi;
    
    volatile int DiskTimerDelay;
    
    uint32_t    capacity;         //!< kapacita karty v blocích
    const uint32_t block;         //!< délka bloku (512)
    
    DSTATUS           Stat;       //!< stav karty
    volatile uint8_t  Timer1;     //!< čítač pro stavovení timeout
    volatile uint8_t  Timer2;     //!< čítač pro stavovení timeout
    uint8_t           CardType;   //!< typ karty
    uint8_t           ok;         //!< příznak úspěšnosti operace
};

#endif // DISKFILE_H

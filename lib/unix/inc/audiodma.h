#ifndef AUDIODMA_H
#define AUDIODMA_H

#define Audio8000HzSettings 256,5,12,1
#define Audio16000HzSettings 213,2,13,0
#define Audio32000HzSettings 213,2,6,1
#define Audio48000HzSettings 258,3,3,1
#define Audio96000HzSettings 344,2,3,1
#define Audio22050HzSettings 429,4,9,1
#define Audio44100HzSettings 271,2,6,0
#define AudioVGAHSyncSettings 419,2,13,0 // 31475.3606. Actual VGA timer is 31472.4616.

#include <stdint.h>
#include "oneway.h"

static const unsigned DmaBufLen = 576;
//static const uint32_t DmaBufLen = 2000;

class AudioDma {

  public:
    AudioDma       ();
    void   SetVolume (int volume);
    void   Turn      (bool on);
    void   Up (Sample * buf, unsigned len);
    static void sig_handler (int n);
    static void exit (void);
  protected:
  private:
};

#endif // AUDIODMA_H

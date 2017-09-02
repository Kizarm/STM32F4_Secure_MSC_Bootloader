#ifndef AUDIO_H
#define AUDIO_H

#define Audio8000HzSettings 256,5,12,1
#define Audio16000HzSettings 213,2,13,0
#define Audio32000HzSettings 213,2,6,1
#define Audio48000HzSettings 258,3,3,1
#define Audio96000HzSettings 344,2,3,1
#define Audio22050HzSettings 429,4,9,1
#define Audio44100HzSettings 271,2,6,0
#define AudioVGAHSyncSettings 419,2,13,0 // 31475.3606. Actual VGA timer is 31472.4616.

#include <stdint.h>

typedef int16_t (*irqCallback) (void);

class Audio {

  public:
    Audio          ();
    void SetVolume (int volume);
    void Turn      (bool on);
    void SampleOut (int16_t sample);
    void IrqEnable (bool on);
    void SetDriver (irqCallback cb);
  protected:
    void WriteRegister (uint8_t address, uint8_t value);
    static void irq    (void);
  private:
    static uint32_t    LR;
    static irqCallback sampleCallback;
};

static const double   AudioSampleRate = 44100.0;
static const unsigned AudioMidiDelay  = 44; // ~ AudioSampleRate / 1000
/// Počet generátorů.
static const unsigned int  maxGens  = 16;
/// Kladné maximum vzorku.
static const int           maxValue =  8191;
/// Záporné maximum vzorku.
static const int           minValue = -8192;
///
static const unsigned int  maxAmplt = (1U<<25);

#endif // AUDIO_H

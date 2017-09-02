#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

typedef int16_t (*irqCallback) (void);

class Audio {

  public:
    Audio          ();
    void SetVolume (int volume){};
    void Turn      (bool on){};
    void SampleOut (int16_t sample){};
    void IrqEnable (bool on){};
    void SetDriver (irqCallback cb);
    static irqCallback sampleCallback;
  protected:
    static void exit (void);
    static void sig_handler (int signum);
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
static const unsigned int  maxAmplt = (1U<<27);

#endif // AUDIO_H

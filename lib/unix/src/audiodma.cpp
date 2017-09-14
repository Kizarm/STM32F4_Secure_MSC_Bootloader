#include "audiodma.h"
#include "gpio.h"
#include <stdio.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include <signal.h>

static const char *device = "default";
static snd_pcm_t  *handle;

void PlatformExit (void) {
}


static int open_alsa_device (int channels, int srate) {
  int err;
  if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    printf("Playback open error: %s\n", snd_strerror(err));
    return 0;
  }
  if ((err = snd_pcm_set_params(handle,
                                SND_PCM_FORMAT_S16_LE,
                                SND_PCM_ACCESS_RW_INTERLEAVED,
                                channels,
                                srate,
                                1,
                                500000)) < 0) { /* 0.5sec */
    printf("Playback open error: %s\n", snd_strerror(err));
    return 0;
  }
  printf ("Alsa Open (%d)Ch, Speed = %d s/s OK.\n", channels, srate);
  return 1;
}
int alsa_write (const void* buf, int len) {
  snd_pcm_sframes_t frames;
  int err = 0;
  frames = snd_pcm_writei(handle, buf, len);
  if (frames < 0)
    frames = snd_pcm_recover(handle, frames, 0);
  if (frames < 0) {
    printf("snd_pcm_writei failed: %s\n", snd_strerror(err));
    return 0;
  }
  if (frames > 0 && frames < (long)sizeof(buf))
    printf("Short write (expected %i, wrote %li)\n", len, frames);
  return len;
}
void AudioDma::sig_handler (int signum) {
  printf(" - Received signal %d\n", signum);
  exit();
  ::exit (0);
}

void AudioDma::Up (Sample * data, unsigned len) {
  alsa_write (data, len);
}

AudioDma::AudioDma () {
  signal (SIGINT, sig_handler);
  // int res = open_alsa_device (2, 22050);
  int res = open_alsa_device (2, 44100);
  if (!res) {
    printf ("Nejde otevrit zvuk\n");
    return;
  }
}
void AudioDma::exit (void) {
  printf ("Alsa Close\n");
  snd_pcm_close (handle);
}
void AudioDma::SetVolume (int) {
}
void AudioDma::Turn (bool b) {
  if (!b) exit();
}

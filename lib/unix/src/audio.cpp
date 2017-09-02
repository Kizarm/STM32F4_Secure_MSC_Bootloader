#include "audio.h"
#include "gpio.h"
#include <stdio.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include <signal.h>

struct Sample {
  short l;
  short r;
}__attribute__((packed));

static const unsigned BufLen = 0x400;

static const char *device = "default";
static snd_pcm_t  *handle;

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
void Audio::sig_handler (int signum) {
  printf(" - Received signal %d\n", signum);
  exit();
  ::exit (0);
}

irqCallback Audio::sampleCallback = 0;

static pthread_t rc;

void * WriteHandler (void * data) {
  printf ("Start thread\n");
  Audio * pA = (Audio *) data;
  Sample buf [BufLen];
  static unsigned short s = 0;
  for (;;) {
    // printf ("pass\n");
    for (unsigned i=0; i<BufLen; i++) {
      if (pA->sampleCallback) s = pA->sampleCallback ();
      buf[i].l = s; buf[i].r = s;
    }
    alsa_write (buf, BufLen);
  }
  return NULL;
}

Audio::Audio () {
  signal (SIGINT, sig_handler);
  int res = open_alsa_device (2,44100);
  if (!res) {
    printf ("Nejde otevrit zvuk\n");
    return;
  }
  pthread_create (&rc, NULL, WriteHandler,  this);
}
void Audio::exit (void) {
  printf ("exit\n");
  pthread_cancel (rc);
  snd_pcm_close (handle);
}

void Audio::SetDriver (irqCallback cb) {
  sampleCallback = cb;
}
bool PlatformMainLoop (void) {
  usleep (10000);
  return true;
}

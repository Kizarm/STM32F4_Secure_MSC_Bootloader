#include <stdio.h>
#include <string.h>
#include "minimad.h"
#include "diskfile.h"
#include "fat32ro.h"

#ifdef __arm__
#include "gpio.h"
static GpioClass but (GpioPortA, 0u, GPIO_Mode_IN);
static bool skip_file () {
  static bool old_state = false;
         bool new_state = but.get();
  if (old_state != new_state) {
    old_state = new_state;
    if (new_state) return true;
  }
  return false;
}
#else
static inline bool skip_file () { return false; }
#endif

/*
 * This is a private message structure. A generic pointer to this structure
 * is passed to each of the callback functions. Put here any data you need
 * to access from within the callbacks.
 */
FAT32RO * MiniMad::fat;

MiniMad::MiniMad () {
}

struct buffer {
  unsigned char const * start;
  unsigned long         length;
};
// rozsekani vstupu na kousky (treba ze souboru)
static const unsigned chunk = 512;
static unsigned char  StreamBuff [chunk];
static unsigned       counter   = 0;
static unsigned       total_len = 0;

/*
 * This is the input callback. The purpose of this callback is to (re)fill
 * the stream buffer which is to be decoded. In this example, an entire file
 * has been mapped into memory, so we just call mad_stream_buffer() with the
 * address and length of the mapping. When this callback is called a second
 * time, we are finished decoding.
 */

enum mad_flow MiniMad::input (void * data,
                              struct mad_stream * stream) {
  
/* Takhle by to melo fungovat napr. ze souboru ->
 * musi to navazovat - tj. zjisit, kde to skoncilo (stream->next_frame) */
  unsigned flen = 0;
  if (stream->next_frame) flen = stream->next_frame - StreamBuff;
  if (flen > chunk) flen = chunk; // pro jistotu
  counter += flen;
  //printf ("%p:%p - %d\r", StreamBuff, stream->this_frame, flen);
  //fflush (stdout);
  if (counter < total_len) {
    //memcpy (StreamBuff, mpeg->mel + counter, chunk);
    fat->get_mpg ((char*)StreamBuff, counter, chunk);
    mad_stream_buffer (stream, StreamBuff, chunk);
    if (skip_file()) return MAD_FLOW_STOP;
    else             return MAD_FLOW_CONTINUE;
  } else
    return MAD_FLOW_STOP;
}

/*
 * This is the output callback function. It is called after each frame of
 * MPEG audio data has been completely decoded. The purpose of this callback
 * is to output (or play) the decoded PCM audio.
 */
extern Interface ifc;

enum mad_flow MiniMad::output (void * data,
                              struct mad_header const * header,
                              struct mad_pcm * pcm) {
  
  unsigned nsamples;
  unsigned nchannels, brate;
  mad_fixed_t const * left_ch;
  mad_fixed_t const * right_ch;

  // pcm->samplerate contains the sampling frequency

  brate     = pcm->samplerate;
  nchannels = pcm->channels;
  nsamples  = pcm->length;
  left_ch   = pcm->samples[0];
  if (nchannels == 1) {           // mono
    right_ch  = left_ch;
  } else {                        // stereo
    right_ch  = pcm->samples[1];
  }
  ifc.blink (nchannels, brate, nsamples);
  // ten vystup mad je divne proti vstupu alsa.
  ifc.pass (left_ch, right_ch, nsamples);

  return MAD_FLOW_CONTINUE;
}

/*
 * This is the error callback function. It is called whenever a decoding
 * error occurs. The error is indicated by stream->error; the list of
 * possible MAD_ERROR_* errors can be found in the mad.h (or stream.h)
 * header file.
 */

enum mad_flow MiniMad::error (void * data,
                              struct mad_stream * stream,
                              struct mad_frame * frame) {
  return MAD_FLOW_CONTINUE;
}

/*
 * This is the function called by main() above to perform all the decoding.
 * It instantiates a decoder object and configures it with the input,
 * output, and error callback functions above. A single call to
 * mad_decoder_run() continues until a callback function returns
 * MAD_FLOW_STOP (to stop decoding) or MAD_FLOW_BREAK (to stop decoding and
 * signal an error).
 */

int MiniMad::decode (const unsigned len) {
  struct buffer buffer;
  struct mad_decoder decoder;
  volatile int result;

  counter   = 0;
  total_len = len;
  /* initialize our private message structure */
  buffer.start  = StreamBuff;
  buffer.length = 0;
  /* configure input, output, and error functions */
  mad_decoder_init (&decoder, &buffer,
                    input, 0 /* header */, 0 /* filter */, output,
                    error, 0 /* message */);
  /* start decoding */
  result = mad_decoder_run (&decoder, MAD_DECODER_MODE_SYNC);
  /* release the decoder */
  mad_decoder_finish (&decoder);
  return result;
}

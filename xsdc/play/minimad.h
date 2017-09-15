#ifndef MINIMAD_H
#define MINIMAD_H

# include "iface.h"
# include "gpio.h"

/**
 * @file
 * @brief Hlavička pro MP3 player.
 * */
class FAT32RO;

class MiniMad {
public:
  MiniMad();
  static void setParent (FAT32RO * f) {fat=f;};
  static int  decode    (const unsigned len);
  static enum mad_flow input  (void * data, struct mad_stream * stream);
  static enum mad_flow output (void * data,
                       struct mad_header const * header,
                       struct mad_pcm * pcm);
  static enum mad_flow error  (void * data,
                       struct mad_stream * stream,
                       struct mad_frame * frame);
  static FAT32RO * fat;
};


#endif //  MINIMAD_H

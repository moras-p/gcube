#ifndef __AUDIO_H
#define __AUDIO_H 1


#include <string.h>
#include "general.h"
#include "mem.h"
#include "gdebug.h"


#define FREQ_32KHZ		64000
#define FREQ_48KHZ		48000


int audio_init (unsigned int freq, unsigned int samples);
void audio_close (void);
void audio_play (__u8 *buff, unsigned int len);
void audio_stop (void);
void audio_set_freq (unsigned int freq);
int audio_dump_stream (char *filename, char *stream, unsigned int len);


#endif // __AUDIO_H

/*
 *  gcube
 *  Copyright (c) 2004 monk
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 *
 *      
 *         
 */

#include <SDL/SDL.h>
#include <SDL/SDL_audio.h>

#include "audio.h"


SDL_AudioSpec spec;

unsigned char *buffer;
int buffer_len = 0, buffer_pos = 0;

__u8 xbuff1[409600], xbuff2[409600], *xbuff = NULL;
int xbuff_end = 0, xbuff_running = 0;

ADPStream adpstream;
__u32 stream_ofs;
int stream_len;
int stream_playing;

#define BLOCK_SIZE					32
#define SAMPLES_PER_BLOCK		28


static float iir1[2], iir2[2];


void adp_decode_init (void)
{
	iir1[0] = iir1[1] = iir2[0] = iir2[1] = 0;
}


__s16 adp_decode_sample (int bits, int q, int ch)
{
	static const float coef[] = { 0.86f, 1.8f, 0.82f, 1.53f };
	float iir_filter = ((short) (bits << 12) >> (q & 15));


	switch (q >> 4)
	{
		case 1:
			iir_filter += iir1[ch] * coef[0];
			break;
		
		case 2:
			iir_filter += iir1[ch] * coef[1] - iir2[ch] * coef[2];
			break;
		
		case 3:
			iir_filter += iir1[ch] * coef[3] - iir2[ch] * coef[0];
			break;
	}
	
	iir2[ch] = iir1[ch];
	iir1[ch] = CLAMP (iir_filter, -32768.5, 32767.5);
	
	return (__s16) iir1[ch];
}


void adp_decode_block (__s16 *pcm, __u8 *adpcm, int stereo)
{
	__u8 *adpcm2 = &adpcm[BLOCK_SIZE - SAMPLES_PER_BLOCK];
	int i;


	for (i = 0; i < SAMPLES_PER_BLOCK; i++)
	{
		*pcm++ = adp_decode_sample (*adpcm2 & 15, adpcm[0], 0);
		if (stereo)
			*pcm++ = adp_decode_sample (*adpcm2 >> 4, adpcm[1], 1);
		adpcm2++;
	}
}


void adp_stream_next (void)
{
	DEBUG (EVENT_LOG, ".adp: play adp stream %.8x, length %.8x",
				 adpstream.offset << 2, adpstream.length);

	stream_ofs = adpstream.offset << 2;
	stream_len = adpstream.length;

printf ("stream playing: %d\n", (stream_len > 0));
	stream_playing = (stream_len > 0);
}


// current stream offset
__u32 adp_stream_ofs (void)
{
	return stream_ofs;
}


void memcpy_swap16 (char *dest, char *src, int len)
{
	int i;


	for (i = 0; i < len/2; i++)
		((__u16 *)dest)[i] = BSWAP16 (((__u16 *)src)[i]);
}


// very thread unsafe
void adp_stream_decode (char *dst, int len)
{
	static __u8 buff[BLOCK_SIZE], buff2[SAMPLES_PER_BLOCK * 4];
	static int leftovers = 0;
	int ch = spec.channels;
	int n = 0;


	if (stream_playing)
	{
#if ENABLE_SOUND
		if (leftovers)
		{
			memcpy (dst, &buff2[SAMPLES_PER_BLOCK * 2 * ch - leftovers], leftovers);
			dst += leftovers;
			len -= leftovers;
		}
		while ((len >= SAMPLES_PER_BLOCK * 2 * ch) && stream_len)
		{
			di_read_stream (buff, stream_ofs, BLOCK_SIZE);
			adp_decode_block ((__s16 *) dst, buff, ch - 1);
			n++;
			stream_ofs += BLOCK_SIZE;
			stream_len -= BLOCK_SIZE;
			if (stream_len <= 0)
				adp_stream_next ();

			dst += SAMPLES_PER_BLOCK * 2 * ch;
			len -= SAMPLES_PER_BLOCK * 2 * ch;
		}

		if (stream_len)
		{		
			di_read_stream (buff, stream_ofs, BLOCK_SIZE);
			adp_decode_block ((__s16 *) buff2, buff, ch - 1);
			n++;
			stream_ofs += BLOCK_SIZE;
			stream_len -= BLOCK_SIZE;
			if (stream_len <= 0)
				adp_stream_next ();

			// leftovers
			memcpy (dst, buff2, len);
			leftovers = SAMPLES_PER_BLOCK * 2 * ch - len;
		}
#endif
		ai_counter_inc (SAMPLES_PER_BLOCK * n);
		return;
	}
	else
		ai_counter_inc (SAMPLES_PER_BLOCK * 0xff);
}


void audio_copy (void *dst, void *src, int len)
{
	if (spec.format == AUDIO_S16MSB)
		memcpy (dst, src, len);
	else
		memcpy_swap16 ((char *) dst, (char *) src, len);
}


void audio_fill_stream_callback (void *userdata, unsigned char *stream, int len)
{
	if (buffer_len)
	{
#if ENABLE_SOUND
//	printf ("buffer len = %d, pos %d\n", buffer_len, buffer_pos);
//memset (stream, 0, len);
		if (len > buffer_len)
		{
			audio_copy (stream, &buffer[buffer_pos], buffer_len);
			memset (stream + buffer_len, 0, len - buffer_len);
			buffer_len = 0;
			buffer_pos = 0;
		}
		else
		{
			audio_copy (stream, &buffer[buffer_pos], len);
			buffer_len -= len;
			buffer_pos += len;
		}
#endif

		STREAM_BLOCKS_LEFT = buffer_len >> 5;
	}
	else
	{
//		dsp_finished ();
		SDL_PauseAudio (TRUE);
		xbuff_running = FALSE;
		if (buffer == xbuff1)
			buffer = xbuff2;
		else
			buffer = xbuff1;
	}
}


int audio_init (unsigned int freq, unsigned int samples)
{
	SDL_AudioSpec desired, obtained;


	if (!samples)
		return FALSE;

	DEBUG (EVENT_INFO, ".audio: initialized freq %d, samples %d", freq, samples);
	memset (&desired, 0, sizeof (SDL_AudioSpec));
	memset (&obtained, 0, sizeof (SDL_AudioSpec));
	
	desired.freq = freq;
	desired.samples = samples;

//	desired.format = AUDIO_S16MSB;
	desired.format = AUDIO_S16LSB;
	desired.callback = audio_fill_stream_callback;
//	desired.channels = 1;
	desired.channels = 2;

	if (0 > SDL_OpenAudio (&desired, &obtained))
	{
		DEBUG (EVENT_EMAJOR, ".audio: couldn't open audio device: %s", SDL_GetError ());
		return FALSE;
	}
	
	memcpy (&spec, &obtained, sizeof (SDL_AudioSpec));

	buffer = xbuff = xbuff1;
	
	return TRUE;
}


void audio_close (void)
{
	SDL_CloseAudio ();
}

#define XBUF_DELAY			48000*3*2
//#define XBUF_DELAY			48000
void audio_play (unsigned char *buff, unsigned int len)
{
	//file_cat ("dump.sw", buff, len);
	SDL_LockAudio ();

//	DEBUG (EVENT_LOG, ".audio: play %d samples", len/2);
#if 0
	STREAM_BLOCKS_LEFT = len >> 5;
	buffer_len = len;
	buffer_pos = 0;
	buffer = buff;


	SDL_UnlockAudio ();

	SDL_PauseAudio (FALSE);
#else

	if (!xbuff)
		return;

	memcpy (&xbuff[xbuff_end], buff, len);
	xbuff_end += len;

	if (((xbuff_end + len) > sizeof (xbuff1)) || (xbuff_end >= XBUF_DELAY))
	{
		if (xbuff == xbuff1)
			xbuff = xbuff2;
		else
			xbuff = xbuff1;

		buffer_len = xbuff_end;
		xbuff_running = TRUE;
		xbuff_end = 0;
	STREAM_BLOCKS_LEFT = 0;

		SDL_UnlockAudio ();
		SDL_PauseAudio (FALSE);
	}
	else
		SDL_UnlockAudio ();

#endif
}


void audio_cancel_last (void)
{
	xbuff_end -= 640;
}


void audio_stop (void)
{
	SDL_PauseAudio (TRUE);
}


void audio_set_freq (int freq)
{
	if (spec.samples && (spec.freq != freq))
	{
		audio_close ();
		audio_init (freq, spec.samples);
	}
}


int audio_dump_stream (char *filename, char *stream, unsigned int len)
{
	FILE *f;
	__u8 d[2];
	unsigned int i;


	f = fopen (filename, "wb");
	if (!f)
	{
		char buff[1024], fname[1024];
		
		get_filename (fname, filename);
		sprintf (buff, "%s/%s", get_home_dir (), fname);
		f = fopen (buff, "wb");

		if (!f)
			return FALSE;
	}
	
	
	for (i = 0; i < len/2; i += 2)
	{
		// swap
		d[1] = stream[i];
		d[0] = stream[i+1];
		fwrite (d, 1, 2, f);
	}

	fclose (f);
	return TRUE;
}


void adp_stream_set (ADPStream *s)
{
	memcpy (&adpstream, s, sizeof (ADPStream));

	if (AI_STREAMING)
	{
		if (!s->offset && !s->length)
			printf ("stopping after current track\n");
		else
			printf ("starting track: %.8x of size %.8x\n", s->offset << 2, s->length);
	}
	else
	{
		if (!s->offset && !s->length)
			printf ("stopping current track\n");
		else
			printf ("queued track: %.8x of size %.8x\n", s->offset << 2, s->length);
	}
}


void adp_play (void)
{
	SDL_LockAudio ();
	adp_stream_next ();
	SDL_UnlockAudio ();

	SDL_PauseAudio (FALSE);
}


void adp_stream_start (void)
{
	printf ("start streaming\n");
	audio_set_freq (FREQ_48KHZ);
	adp_decode_init ();
	adp_play ();
}


void adp_stream_stop (void)
{
	printf ("stop streaming\n");
	audio_stop ();
}

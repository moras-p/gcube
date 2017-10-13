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
int buffer_len, buffer_pos;


void memcpy_swap16 (char *dest, char *src, int len)
{
	int i;


	for (i = 0; i < len/2; i++)
		((__u16 *)dest)[i] = BSWAP16 (((__u16 *)src)[i]);
}


void fill_stream_callback (void *userdata, unsigned char *stream, int len)
{
	if (buffer_len)
	{
		if (spec.format == AUDIO_S16MSB)
			memcpy (stream, &buffer[buffer_pos], len);
		else
		// need to swap data
			memcpy_swap16 (stream, &buffer[buffer_pos], len);

		if (len > buffer_len)
		{
			memset (stream + buffer_len, 0, len - buffer_len);
			buffer_len = 0;
			buffer_pos = 0;
		}
		else
		{
			buffer_len -= len;
			buffer_pos += len;
		}
		
		STREAM_BLOCKS_LEFT = buffer_len >> 5;
	}
	else
	{
		dsp_finished ();
		SDL_PauseAudio (TRUE);
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
	
	desired.format = AUDIO_S16MSB;
	desired.callback = fill_stream_callback;
	desired.channels = 1;

	if (0 > SDL_OpenAudio (&desired, &obtained))
	{
		DEBUG (EVENT_EMAJOR, ".audio: couldn't open audio device: %s", SDL_GetError ());
		return FALSE;
	}
	
	memcpy (&spec, &obtained, sizeof (SDL_AudioSpec));
	
	return TRUE;
}


void audio_close (void)
{
	SDL_CloseAudio ();
}


void audio_play (unsigned char *buff, unsigned int len)
{
	SDL_LockAudio ();

	DEBUG (EVENT_LOG, ".audio: play %d samples", len/2);
	
	STREAM_BLOCKS_LEFT = len >> 5;
	buffer_len = len;
	buffer_pos = 0;
	buffer = buff;

	SDL_UnlockAudio ();

	SDL_PauseAudio (FALSE);
}


void audio_stop (void)
{
	SDL_PauseAudio (TRUE);
}


void audio_set_freq (unsigned int freq)
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
	int i;


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

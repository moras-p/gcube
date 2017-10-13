/*
 *  thpview
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
 *  very simple thp video player
 *  - slow
 *  - loads whole file to memory
 */

// TODO: make patches for jdhuff.c and jdcolor.c for libjpeg-6b
// needs changed file jdhuff.c to recognize the marker

// jdcolor.c is borked as well
// jpeg-mmx under windows - remove simd file compilation from makefile.cfg
//   probably looses all speed improvement to libjpeg-6b then (check that maybe)

#include "thpview.h"


char filename[256] = "\0";
int quit_requested = FALSE;
SDL_Surface *screen, *output = NULL;
SDL_Rect rect;
unsigned int color_space = 1;
int verbose = FALSE;
int looping = FALSE;
int nolimit = FALSE;
int mute = FALSE;
int benchmark = FALSE;
SDL_AudioSpec spec;
AudioBuffer audio_buffer[2];
int audio_buffer_current, audio_buffer_filling;

__u8 video_buff[1024*1024*4];


__u32 find_marker_any (__u8 *data)
{
	__u32 i = 0;
	
	
	while (data[i++] != 0xff)
		;
	
	return i;
}


__u32 find_marker (__u8 *data, __u8 marker)
{
	__u32 i = 0;


	do
		i += find_marker_any (&data[i]);
	while (data[i] != marker);

	return i;
}


void thp_set_jpeg_frame_header (JPEGFrame *jframe, __u16 width, __u16 height)
{
	JPEGHeader *jpg = (JPEGHeader *) (jframe->data + 4);
	__u16 *markers = (__u16 *) jframe->data;


	markers[0] = BSWAP16 (JPEG_SOI_MARKER);
	markers[1] = BSWAP16 (JPEG_APP0_MARKER);

	strcpy (jpg->magic, "JFIF");
	jpg->length = BSWAP16 (sizeof (JPEGHeader));
	jpg->version = 0x0101;
	jpg->Xdensity = BSWAP16 (width);
	jpg->Ydensity = BSWAP16 (height);
	jpg->units = 0;
	jpg->Xthumbnail = jpg->Ythumbnail = 0;
	
	jframe->end_of_header = jframe->data + 4 + sizeof (JPEGHeader);
	jframe->end_of_header[0] = 0xff;
	jframe->end_of_header[1] = EOI_MARKER;
}


void thp_extract_jpeg_frame (JPEGFrame *jframe, __u8 *frame, int with_sound)
{
	THPFrameHeader *fhdr = (THPFrameHeader *) frame;
	__u8 *p, *data;
	int l = 0;


	// move frame pointer after SOI marker
	frame += 12 + 2 + ((with_sound) ? 4 : 0);
	
	// get the EOI marker
	p = frame - 2 + BSWAP32 (fhdr->video_data_size);
	while (!*p)
		p--;

	data = jframe->end_of_header;

	// copy everything up to Start Of Scan marker
	l = find_marker (frame, SOS_MARKER) + 1;
	memcpy (data, frame, l);
	data += l;
	frame += l;

	// convert any 0xff found before EOI to 0xff 0x00 combination
	// (jpeglib will search for markers inside image data)
	while (1)
	{
		l = find_marker_any (frame);

		memcpy (data, frame, l);
		data += l;

		if (frame + l >= p)
		{
			*data++ = EOI_MARKER;
			break;
		}

		*data++ = 0;
		frame += l;
	}

	jframe->size = data - jframe->data;
}


void thp_extract_jpeg_frame_new (JPEGFrame *jframe, __u8 *frame, int with_sound)
{
	THPFrameHeader *fhdr = (THPFrameHeader *) frame;


	frame += 12 + ((with_sound) ? 4 : 0);
	jframe->data = frame;
	jframe->size = BSWAP32 (fhdr->video_data_size);
}


void thp_write_jpeg_frame (JPEGFrame *jframe, char *fname)
{
	FILE *f;


	f = fopen (fname, "wb");
	fwrite (jframe->data, 1, jframe->size, f);
	fclose (f);
}


void thp_write_jpeg_frame_cat (FILE *f, JPEGFrame *jframe)
{
	fwrite (jframe->data, 1, jframe->size, f);
}


void thp_extract_audio_frame (AUDIOFrame *aframe, __u8 *frame)
{
	THPFrameHeader *fhdr = (THPFrameHeader *) frame;


	aframe->data = frame + 12 + 2 + 4 + BSWAP32 (fhdr->video_data_size);
	aframe->size = BSWAP32 (fhdr->audio_data_size);
}


// decode into raw signed short
// dst->data must be big enough
void decode_audio_frame_mono (AUDIOFrame *dst, AUDIOFrame *src)
{
	AUDIOHeader *hdr = (AUDIOHeader *) src->data;
	float prev2, prev1, curr, factor1, factor2;
	int factor3, nsamples;
	int index, i, j, d;
	__s16 *dstdata;
	__u8 *data;


	nsamples = BSWAP32 (hdr->num_samples);

	dst->size = nsamples * 2;

	prev1 = BSWAPS16 (hdr->c0_prev1);
	prev2 = BSWAPS16 (hdr->c0_prev2);

	data = src->data + sizeof (AUDIOHeader);
	dstdata = (__s16 *) dst->data;
	for (i = 0; i < nsamples;)
	{
		index = (*data >> 4) & 0x07;

		factor1 = (float) BSWAPS16 (hdr->c0_table[2*index + 0]) / 2048;
		factor2 = (float) BSWAPS16 (hdr->c0_table[2*index + 1]) / 2048;
		factor3 = 1 << (*data & 0x0f);

		data++;
		for (j = 0; j < 7; j++)
		{
			d = EXTS (4, *data >> 4);
			curr = prev1 * factor1 + prev2 * factor2 + d * factor3;
			prev2 = prev1;
			prev1 = curr;

			dstdata[i++] = curr;

			d = EXTS (4, *data & 0x0f);
			curr = prev1 * factor1 + prev2 * factor2 + d * factor3;
			prev2 = prev1;
			prev1 = curr;

			dstdata[i++] = curr;
			
			data++;
		}
	}
}


void decode_audio_frame_stereo (AUDIOFrame *dst, AUDIOFrame *src)
{
	AUDIOHeader *hdr = (AUDIOHeader *) src->data;
	float curr, lprev2, lprev1, lfactor1, lfactor2;
	float rprev2, rprev1, rfactor1, rfactor2;
	int lfactor3, rfactor3, lindex, rindex, nsamples;
	int i, j, d;
	__s16 *dstdata;
	__u8 *ldata, *rdata;


	nsamples = BSWAP32 (hdr->num_samples);

	dst->size = nsamples * 2 * 2;

	lprev1 = BSWAPS16 (hdr->c0_prev1);
	lprev2 = BSWAPS16 (hdr->c0_prev2);

	rprev1 = BSWAPS16 (hdr->c1_prev1);
	rprev2 = BSWAPS16 (hdr->c1_prev2);

	ldata = src->data + sizeof (AUDIOHeader);
	rdata = src->data + sizeof (AUDIOHeader) + BSWAP16 (hdr->channel_size);
	dstdata = (__s16 *) dst->data;
	for (i = 0; i < nsamples * 2;)
	{
		lindex = (*ldata >> 4) & 0x07;

		lfactor1 = (float) BSWAPS16 (hdr->c0_table[2*lindex + 0]) / 2048;
		lfactor2 = (float) BSWAPS16 (hdr->c0_table[2*lindex + 1]) / 2048;
		lfactor3 = 1 << (*ldata & 0x0f);

		ldata++;

		rindex = (*rdata >> 4) & 0x07;

		rfactor1 = (float) BSWAPS16 (hdr->c1_table[2*rindex + 0]) / 2048;
		rfactor2 = (float) BSWAPS16 (hdr->c1_table[2*rindex + 1]) / 2048;
		rfactor3 = 1 << (*rdata & 0x0f);

		rdata++;
		for (j = 0; j < 7; j++)
		{
			d = EXTS (4, *ldata >> 4);
			curr = lprev1 * lfactor1 + lprev2 * lfactor2 + d * lfactor3;
			lprev2 = lprev1;
			lprev1 = curr;

			dstdata[i++] = curr;

			d = EXTS (4, *rdata >> 4);
			curr = rprev1 * rfactor1 + rprev2 * rfactor2 + d * rfactor3;
			rprev2 = rprev1;
			rprev1 = curr;

			dstdata[i++] = curr;

			d = EXTS (4, *ldata & 0x0f);
			curr = lprev1 * lfactor1 + lprev2 * lfactor2 + d * lfactor3;
			lprev2 = lprev1;
			lprev1 = curr;

			dstdata[i++] = curr;

			d = EXTS (4, *rdata & 0x0f);
			curr = rprev1 * rfactor1 + rprev2 * rfactor2 + d * rfactor3;
			rprev2 = rprev1;
			rprev1 = curr;

			dstdata[i++] = curr;
			
			ldata++;
			rdata++;
		}
	}
}


void decode_audio_frame (AUDIOFrame *dst, AUDIOFrame *src, int nchannels)
{
	switch (nchannels)
	{
		case 2:
			decode_audio_frame_stereo (dst, src);
			break;

		default:
			decode_audio_frame_mono (dst, src);
	}
}


void thp_write_audio_frame (FILE *f, AUDIOFrame *aframe, int stereo)
{
	// buffer for decoded samples
	__u8 buff[0x10000];
	AUDIOFrame decoded;


	decoded.data = buff;

	if (stereo)
		decode_audio_frame_stereo (&decoded, aframe);
	else
		decode_audio_frame_mono (&decoded, aframe);

	fwrite (decoded.data, 1, decoded.size, f);
}


__u8 *thp_get_frame (__u8 *buff, int n)
{
	THPHeader *hdr = THP_HEADER (buff);
	THPAudioInfo *ainfo = THP_AUDIO_INFO (buff);
	THPFrameHeader *fhdr;
	__u8 *frame;


	frame = buff + BSWAP32 (hdr->first_frame_offset);
	while (n--)
	{
		fhdr = (THPFrameHeader *) frame;
		frame = frame + BSWAP32 (fhdr->video_data_size) + 12;
		if (ainfo)
		{
			frame += 4;
			if (THP_VERSION (buff) == 0x00010000)
				frame += BSWAP32 (fhdr->audio_data_size);
			else
				frame += BSWAP32 (fhdr->audio_data_size) * BSWAP32 (ainfo->num_data);
		}
		
		while (!*((__u32 *) frame))
			frame += 4;
	}

	return frame;
}


__u8 *thp_get_next_frame (__u8 *buff, __u8 *frame)
{
	THPAudioInfo *ainfo = THP_AUDIO_INFO (buff);
	THPFrameHeader *fhdr;


	fhdr = (THPFrameHeader *) frame;
	frame = frame + BSWAP32 (fhdr->video_data_size) + 12;
	if (ainfo)
	{
		frame += 4;
		if (THP_VERSION (buff) == 0x00010000)
			frame += BSWAP32 (fhdr->audio_data_size);
		else
			frame += BSWAP32 (fhdr->audio_data_size) * BSWAP32 (ainfo->num_data);
	}
		
	while (!*((__u32 *) frame))
		frame += 4;

	return frame;
}


int event_filter (const SDL_Event *event)
{
	switch (event->type)
	{
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			return FALSE;

		default:
			return TRUE;
	}
}


int window_open (int width, int height)
{
	if (0 > SDL_Init (SDL_INIT_VIDEO))
	{
		fprintf (stderr, "SDL_Init failed: %s\n", SDL_GetError ());
		return FALSE;
	}

	screen = SDL_SetVideoMode (width, height, 0, SDL_DOUBLEBUF | SDL_HWSURFACE);
	if (!screen)
	{
		fprintf (stderr, "SDL_SetVideoMode failed: %s\n", SDL_GetError ());
		return FALSE;
	}

	rect.x = rect.y = 0;
	rect.w = width;
	rect.h = height;

	if (color_space == CSPACE_GRAYSCALE)
	{
		SDL_Color grayscale[256];
		int i;

		for (i = 0; i < 256; i++)
			grayscale[i].r = grayscale[i].g = grayscale[i].b = i;

		output = SDL_AllocSurface (SDL_HWSURFACE, width, height, 8,
															 0xff, 0x0ff, 0xff, 0);

		if (output)
			SDL_SetPalette (output, SDL_LOGPAL | SDL_PHYSPAL, grayscale, 0, 256);
	}
	else if (color_space == CSPACE_RGB)
	{
		output = SDL_AllocSurface (SDL_HWSURFACE, width, height, 24,
#ifdef LIL_ENDIAN
							0x000000ff, 0x0000ff00, 0x00ff0000,
#else
							0x00ff0000, 0x0000ff00, 0x000000ff,
#endif
							0);
	}

	if (!output)
	{
		fprintf (stderr, "SDL_CreateRGBSurface failed: %s\n", SDL_GetError ());
		return FALSE;
	}

	SDL_SetEventFilter (event_filter);
	
	return TRUE;
}


void window_close (void)
{
	if (output)
		SDL_FreeSurface (output);
	SDL_Quit ();
}


void window_check_input (void)
{
	SDL_Event event;


	if (SDL_PollEvent (&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				quit_requested = TRUE;
				break;
			
			case SDL_KEYUP:
				switch (event.key.keysym.sym)
				{
					case SDLK_ESCAPE:
						quit_requested = TRUE;
						break;
					
					default:
						;
				}
			
			default:
				;
		}
	}
}


void window_display_frame (float progress)
{
	static int title_refresh = 0;
	char buff[256];
	float fps = timer_count_fps ();


	SDL_BlitSurface (output, NULL, screen, &rect);
	SDL_Flip (screen);

	if (!title_refresh)
	{
		sprintf (buff, "thpview playing [%s] @ fps: %2.2f (%2d%%)", filename, fps, (int) (100 * progress));
		SDL_WM_SetCaption (buff, NULL);
		title_refresh = 10;
	}

	title_refresh--;
	
	window_check_input ();
}


void benchmark_stats (float progress)
{
	static int refresh = 0;
	float fps = timer_count_fps ();


	if (!refresh)
	{
		if (fps > 0.1)
			printf ("fps: %2.2f (%2d%%)\n", fps, (int) (100 * progress));
		refresh = 100;
	}

	refresh--;
}


static void memcpy_swap16 (char *dest, char *src, int len)
{
	int i;


	for (i = 0; i < len/2; i++)
		((__u16 *)dest)[i] = BSWAP16 (((__u16 *)src)[i]);
}


void audio_memcpy (void *dest, void *src, int len)
{
	if (spec.format == AUDIO_S16LSB)
		memcpy (dest, src, len);
	else
	// need to swap data
		memcpy_swap16 (dest, src, len);
}


int audio_buffer_copy (void *dst, int len)
{
	AudioBuffer *ab_cur = &audio_buffer[audio_buffer_current];


	if (len < ab_cur->len)
	{
		audio_memcpy (dst, &ab_cur->data[ab_cur->pos], len);
		ab_cur->len -= len;
		ab_cur->pos += len;
		
		return len;
	}
	else
	{
		int ncopied = ab_cur->len;

		// low on buff or equal
		// copy the rest, reset buffer
		audio_memcpy (dst, &ab_cur->data[ab_cur->pos], ab_cur->len);
		ab_cur->len = 0;
		ab_cur->pos = 0;
		ab_cur->loaded = FALSE;

		return ncopied;
	}
}


void thp_audio_fill_stream_callback (void *userdata, unsigned char *stream, int len)
{
	AudioBuffer *ab_cur = &audio_buffer[audio_buffer_current];


	if (ab_cur->loaded)
	{
		int clen = audio_buffer_copy (stream, len);

		// buffer emptied
		if (!ab_cur->loaded)
		{
			len -= clen;
			stream += clen;

			// swap current buffer
			audio_buffer_current = 1 - audio_buffer_current;
			ab_cur = &audio_buffer[audio_buffer_current];

			if (ab_cur->loaded)
			{
				clen = audio_buffer_copy (stream, len);
				if (!ab_cur->loaded)
				{
					len -= clen;
					stream += clen;
					// fill the rest with silence
					memset (stream, 0, len);
				}
			}
			else
			{
				// fill the rest with silence
				memset (stream, 0, len);
			}
		}
	}
	else
	{
		SDL_PauseAudio (TRUE);
	}
}


int thp_audio_init (unsigned int freq, unsigned int samples, unsigned int channels)
{
	SDL_AudioSpec desired, obtained;


	memset (&desired, 0, sizeof (SDL_AudioSpec));
	memset (&obtained, 0, sizeof (SDL_AudioSpec));
	
	desired.freq = freq;
	desired.samples = samples;

	desired.format = AUDIO_S16LSB;
	desired.callback = thp_audio_fill_stream_callback;
	desired.channels = channels;

	if (0 > SDL_OpenAudio (&desired, &obtained))
	{
		printf ("Couldn't open audio device: %s", SDL_GetError ());
		return FALSE;
	}
	
	memcpy (&spec, &obtained, sizeof (SDL_AudioSpec));
	memset (audio_buffer, 0, sizeof (audio_buffer));
	
	return TRUE;
}


void thp_audio_close (void)
{
	SDL_CloseAudio ();
}


void thp_audio_play (void)
{
	if (SDL_GetAudioStatus () == SDL_AUDIO_PLAYING)
		return;

	SDL_PauseAudio (FALSE);
}


void thp_audio_stop (void)
{
	SDL_PauseAudio (TRUE);
}


void thp_play_movie (__u8 *buff, int audio)
{
	THPVideoInfo *vinfo = THP_VIDEO_INFO (buff);
	THPAudioInfo *ainfo = THP_AUDIO_INFO (buff);
	__u8 *frame;
	int i = 0, nchannels = 0, nframes;
	JPEGFrame jframe;
	AUDIOFrame aframe, decoded;
	void *outbuff;
	int pitch;


	if (audio)
		audio = THP_AUDIO_PRESENT (buff);

	if (mute)
		audio = FALSE;

	// max possible jpeg frame size
	jframe.data = malloc (BSWAP32 (THP_HEADER (buff)->max_buffer_size * 2 + 16));
	thp_set_jpeg_frame_header (&jframe, BSWAP32 (vinfo->width), BSWAP32 (vinfo->height));

	if (benchmark)
	{
		pitch = BSWAP32 (vinfo->width) * ((color_space == CSPACE_RGB) ? 3 : 1);
		outbuff = malloc (pitch * BSWAP32 (vinfo->height));
	}
	else
	{
		window_open (BSWAP32 (vinfo->width), BSWAP32 (vinfo->height));
		pitch = output->pitch;
		outbuff = output->pixels;
	}

	if (audio)
	{
		nchannels = BSWAP32 (ainfo->num_channels);
		thp_audio_init (BSWAP32 (ainfo->frequency), 512, nchannels);

		audio_buffer_current = 0;
	}

	jpeg_decompress_start (jframe.data, JFRAME_HEADER_SIZE);

	nframes = BSWAP32 (THP_HEADER (buff)->num_frames);
	do
	{
		frame = buff + BSWAP32 (THP_HEADER (buff)->first_frame_offset);
		for (i = 0; i < nframes; i++)
		{
			if (audio)
			{
				thp_extract_audio_frame (&aframe, frame);

				while (audio_buffer[audio_buffer_filling].loaded)
					timer_delay (5);

				decoded.data = audio_buffer[audio_buffer_filling].data;
				decode_audio_frame (&decoded, &aframe, nchannels);
				audio_buffer[audio_buffer_filling].len = decoded.size;
				audio_buffer[audio_buffer_filling].loaded = TRUE;
				audio_buffer_filling = 1 - audio_buffer_filling;

				thp_audio_play ();
			}
			else if (!nolimit)
				timer_update ();

			thp_extract_jpeg_frame_new (&jframe, frame, THP_AUDIO_PRESENT (buff));
			jpeg_decompress_continue (jframe.data, jframe.size, outbuff, pitch, color_space);

			if (benchmark)
				benchmark_stats ((float) i / (nframes - 1));
			else
				window_display_frame ((float) i / (nframes - 1));

			if (i < nframes - 1)
				frame = thp_get_next_frame (buff, frame);
		
			if (quit_requested)
				break;

			if (!audio && !nolimit)
				timer_delay_diff ((int) (1000.0 / BSWAPF (THP_HEADER (buff)->framerate)));
		}
	} while (looping && !quit_requested);

	// wait till sound finishes
	while (audio_buffer[audio_buffer_current].loaded)
		timer_delay (5);

	if (benchmark)
		free (outbuff);
	else
		window_close ();

//	free (jframe.data);
}


void thp_extract_data (__u8 *buff, int extract_video, int extract_audio)
{
	THPVideoInfo *vinfo = THP_VIDEO_INFO (buff);
	__u8 *frame;
	char fname[256];
	int i = 0;
	JPEGFrame jframe;
	AUDIOFrame aframe;
	FILE *faudio = NULL;
	int stereo = FALSE;


	if (extract_video)
	{
		// max possible jpeg frame size
		jframe.data = malloc (BSWAP32 (THP_HEADER (buff)->max_buffer_size * 2 + 16));
		thp_set_jpeg_frame_header (&jframe, BSWAP32 (vinfo->width), BSWAP32 (vinfo->height));
	}
	
	if (extract_audio && THP_AUDIO_PRESENT (buff))
	{
		// write wave header
		WAVHeader whdr;
		THPAudioInfo *ainfo = THP_AUDIO_INFO (buff);
		int nsamples = BSWAP32 (ainfo->num_samples);
		
		sprintf (fname, "%s.wav", filename);
		faudio = fopen (fname, "wb");

		strncpy (whdr.ChunkID, "RIFF", 4);
		strncpy (whdr.Format, "WAVE", 4);
		strncpy (whdr.SubChunk1ID, "fmt ", 4);
		strncpy (whdr.SubChunk2ID, "data", 4);

		whdr.SubChunk1Size = 16;
		whdr.AudioFormat = 1;
		whdr.NumChannels = BSWAP32 (ainfo->num_channels);
		whdr.SampleRate = BSWAP32 (ainfo->frequency);
		whdr.BitsPerSample = 16;
		whdr.ByteRate = whdr.SampleRate * whdr.NumChannels * whdr.BitsPerSample/8;
		whdr.BlockAlign = whdr.NumChannels * whdr.BitsPerSample/8;

		whdr.SubChunk2Size = nsamples * whdr.NumChannels * whdr.BitsPerSample/8;
		whdr.ChunkSize = 36 + whdr.SubChunk2Size;
		fwrite (&whdr, 1, sizeof (WAVHeader), faudio);
		
		stereo = (whdr.NumChannels == 2);
	}

	for (i = 0; i < BSWAP32 (THP_HEADER (buff)->num_frames); i++)
	{
		frame = thp_get_frame (buff, i);
		
		if (verbose)
			printf ("processing frame at offset 0x%.8x: %d\n", frame - buff, i);

		if (extract_video)
		{
			sprintf (fname, "%s-%.4d.jpg", filename, i);
			thp_extract_jpeg_frame (&jframe, frame, THP_AUDIO_PRESENT (buff));
			thp_write_jpeg_frame (&jframe, fname);

		}

		if (extract_audio && THP_AUDIO_PRESENT (buff))
		{
			thp_extract_audio_frame (&aframe, frame);
			thp_write_audio_frame (faudio, &aframe, stereo);
		}
	}

	if (extract_audio && THP_AUDIO_PRESENT (buff))
		fclose (faudio);

	if (extract_video)
		free (jframe.data);
}


void thp_show_info (char *buff)
{
	THPHeader *hdr = THP_HEADER (buff);
	THPComponentData *comp = THP_COMPONENT_DATA (buff);
	void *info = THP_COMPONENTS_INFO (buff);
	int i;


	printf ("              version: %.8x\n", BSWAP32 (hdr->version));
	printf ("            framerate: %f\n", BSWAPF (hdr->framerate));
	printf ("          frame count: %d\n", BSWAP32 (hdr->num_frames));

	printf (" number of components: %d\n", BSWAP32 (comp->num_components));

	for (i = 0; i < BSWAP32 (comp->num_components); i++)
	{
		switch (comp->components[i])
		{
			case COMPONENT_VIDEO_ID:
				{
					THPVideoInfo *vinfo = (THPVideoInfo *) info;

					printf ("        component[%2.2d]: video\n", i);
					printf ("                width: %d\n", BSWAP32 (vinfo->width));
					printf ("               height: %d\n", BSWAP32 (vinfo->height));
					
					info += COMPONENT_VIDEO_SIZE (buff);
				}
				break;
			
			case COMPONENT_AUDIO_ID:
				{
					THPAudioInfo *ainfo = (THPAudioInfo *) info;

					printf ("        component[%2.2d]: audio\n", i);
					printf ("             channels: %d\n", BSWAP32 (ainfo->num_channels));
					printf ("            frequency: %d\n", BSWAP32 (ainfo->frequency));
					printf ("    number of samples: %d\n", BSWAP32 (ainfo->num_samples));
					
					info += COMPONENT_AUDIO_SIZE (buff);
				}
				break;
				
			case COMPONENT_NONE_ID:
				break;
			
			default:
					printf ("        component[%2.2d]: unknown\n", i);
		}
	}
}


int is_thp (char *buff)
{
	return (BSWAP32 (THP_HEADER (buff)->magic) == THP_MAGIC);
}


#ifdef WINDOWSs
int thpview_main (int argc, char **argv)
#else
#undef main
int main (int argc, char **argv)
#endif
{
	int option_index = 0, c;
	struct option long_options[] =
	{
		{"help", 0, 0, 'h'},
		{"info", 0, 0, 'i'},
		{"verbose", 0, 0, 'v'},
		{"loop", 0, 0, 'l'},
		{"no-fps-limit", 0, 0, 's'},
		{"mute", 0, 0, 'm'},
		{"extract-data", 0, 0, 'x'},
		{"color-space", 1, 0, 'c'},
		{0, 0, 0, 0},
	};
	int show_info = FALSE, extract_data = FALSE;
	FILE *f;
	char *buff;
	int size;
	

	opterr = 0;
	while (-1 != (c = getopt_long (argc, argv, "hivlsmbxc:", long_options, &option_index)))
	{
		switch (c)
		{
		case 'h':
				printf ("thpview %s - Simple THP video player\n", THPVIEW_VERSION);
				printf ("Usage: thpview [OPTIONS] file.thp\n");
				printf ("  -i, --info                       show file information\n");
				printf ("  -v, --verbose                    verbose output\n");
				printf ("  -l, --loop                       loop playback\n");
				printf ("  -s, --no-fps-limit               play as fast as possible\n");
				printf ("  -m, --mute                       disable sound\n");
				printf ("  -b, --benchmark                  disable video\n");
				printf ("  -x, --extract-data               extract data to files\n");
				printf ("  -c, --color-space=c              0 - grayscale, 1 - rgb\n");
				return FALSE;

			case 'i':
				show_info = TRUE;
				break;
			
			case 'v':
				verbose = TRUE;
				break;
			
			case 'l':
				looping = TRUE;
				break;
			
			case 's':
				nolimit = TRUE;
				break;
			
			case 'm':
				mute = TRUE;
				break;
			
			case 'b':
				benchmark = TRUE;
				break;
			
			case 'x':
				extract_data = TRUE;
				break;

			case 'c':
				sscanf (optarg, "%d", &color_space);
				if (color_space > CSPACE_RGB)
					color_space = CSPACE_RGB;
				break;
		}
	}

	if (optind >= argc)
	{
		printf ("Usage: thpview [OPTIONS] file.thp\n");
		printf ("Try 'thpview --help' for more information.\n");
		return FALSE;
	}

	f = fopen (argv[optind], "rb");
	if (!f)
	{
		printf ("couldn't open file %s\n", argv[optind]);
		perror ("");
		return TRUE;
	}
	
	get_filename (filename, argv[optind]);
	filename[strlen (filename) - 4] = '\0';

	fseek (f, 0, SEEK_END);
	size = ftell (f);
	fseek (f, 0, SEEK_SET);

	buff = malloc (size);
	fread (buff, 1, size, f);
	fclose (f);

	if (is_thp (buff))
	{
		if (show_info)
		{
			thp_show_info (buff);
			return FALSE;
		}
		
		if (extract_data)
		{
			thp_extract_data (buff, TRUE, TRUE);
			return FALSE;
		}

		if (benchmark)
			printf ("playing in benchmark mode, no video output...\n");
		thp_play_movie (buff, 1);
	}
	else
		printf ("not a thp file\n");

	return FALSE;
}

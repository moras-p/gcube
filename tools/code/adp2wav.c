/*
 *  adp2wav
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
 *  This code is based on
 *  Nintendo Gamecube ADPCM Decoder Core Class
 *  by Shinji Chiba
 *
 */


#include "general.h"
#define __GNU_SOURCE
#include <getopt.h>
#include <fcntl.h>

typedef struct
{
	__u8 ChunkID[4];
	__u32 ChunkSize;
	__u8 Format[4];

	// fmt subchunk
	__u8 SubChunk1ID[4];
	__u32 SubChunk1Size;
	__u16 AudioFormat;
	__u16 NumChannels;
	__u32 SampleRate;
	__u32 ByteRate;
	__u16 BlockAlign;
	__u16 BitsPerSample;

	// data subchunk
	__u8 SubChunk2ID[4];
	__u32 SubChunk2Size;
} WAVHeader;


#define ADP2WAV_VERSION			"0.1"

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
	
	return iir1[ch];
}


void adp_decode_block (__s16 *pcm, __u8 *adpcm, int stereo)
{
	__u8 *adpcm2 = &adpcm[BLOCK_SIZE - SAMPLES_PER_BLOCK];
	int i;


	for (i = 0; i < SAMPLES_PER_BLOCK; i++)
	{
		*pcm++ = adp_decode_sample(*adpcm2 & 15, adpcm[0], 0);
		if (stereo)
			*pcm++ = adp_decode_sample (*adpcm2 >> 4, adpcm[1], 1);

		adpcm2++;
	}
}


int adp_decode_file_f (FILE *out, FILE *in, int ch, int rate, int raw)
{
	__u8 adpcm[BLOCK_SIZE];
	__s16 pcm[SAMPLES_PER_BLOCK * 2];
	int l, nsamples;


	fseek (in, 0, SEEK_END);
	nsamples = ftell (in);
	fseek (in, 0, SEEK_SET);
	nsamples = nsamples / BLOCK_SIZE * SAMPLES_PER_BLOCK;

	if (!raw)
	{
		// write wave header
		WAVHeader whdr;

		strncpy (whdr.ChunkID, "RIFF", 4);
		strncpy (whdr.Format, "WAVE", 4);
		strncpy (whdr.SubChunk1ID, "fmt ", 4);
		strncpy (whdr.SubChunk2ID, "data", 4);

		whdr.NumChannels = ch;
		whdr.SampleRate = rate;
		whdr.SubChunk1Size = 16;
		whdr.AudioFormat = 1;
		whdr.BitsPerSample = 16;
		whdr.ByteRate = whdr.SampleRate * whdr.NumChannels * whdr.BitsPerSample/8;
		whdr.BlockAlign = whdr.NumChannels * whdr.BitsPerSample/8;

		whdr.SubChunk2Size = nsamples * whdr.NumChannels * whdr.BitsPerSample/8;
		whdr.ChunkSize = 36 + whdr.SubChunk2Size;

		fwrite (&whdr, 1, sizeof (WAVHeader), out);
	}

	adp_decode_init ();

	while (1)
	{
		if (BLOCK_SIZE > (l = fread (adpcm, 1, BLOCK_SIZE, in)))
			break;

		adp_decode_block (pcm, adpcm, ch - 1);
		fwrite (pcm, 2, SAMPLES_PER_BLOCK * ch, out);
	}
	
	return TRUE;
}


int adp_decode_file (char *ofilename, char *ifilename, int ch, int rate, int raw)
{
	FILE *in, *out;


	in = fopen (ifilename, "rb");
	if (!in)
	{
		printf ("error: couldn't open file %s\n", ifilename);
		perror ("");
		return FALSE;
	}

	if (!ofilename)
		out = stdout;
	else
	{
		out = fopen (ofilename, "wb+");
		if (!out)
		{
			printf ("error: couldn't write to file %s\n", ofilename);
			perror ("");
			fclose (in);
			return FALSE;
		}
	}

	adp_decode_file_f (out, in, ch, rate, raw);

	if (out != stdout)
		fclose (out);
	fclose (in);
	
	return TRUE;
}


int main (int argc, char **argv)
{
	int option_index = 0, c;
	struct option long_options[] =
	{
		{"help", 0, 0, 'h'},
		{"dump-raw", 0, 0, 'd'},
		{"rate", 1, 0, 'r'},
		{"channels", 1, 0, 'c'},
		{0, 0, 0, 0},
	};
	char *inf = NULL, *outf = NULL;
	int channels = 2, rate = 48000, raw = FALSE;


	opterr = 0;
	while (-1 != (c = getopt_long (argc, argv, "hdr:c:", long_options, &option_index)))
	{
		switch (c)
		{
			case 'h':
				printf ("adp2wav %s - Simple ADP file decoder\n", ADP2WAV_VERSION);
				printf ("Usage: adp2wav [OPTIONS] in.adp [out.wav]\n");
				printf ("  -h, --help                       show this information\n");
				printf ("  -r, --rate=RATE                  set sampling rate\n");
				printf ("  -c, --channels=N                 1 - mono, 2 - stereo (default)\n");
				printf ("  -d, --decode-raw                 dump raw data\n");
				return FALSE;

			case 'd':
				raw = TRUE;
				break;
			
			case 'c':
				sscanf (optarg, "%d", &channels);
				if (channels <= 1)
					channels = 1;
				else
					channels = 2;
				break;

			case 'r':
				sscanf (optarg, "%d", &rate);
				break;
		}
	}

	if (optind >= argc)
	{
		printf ("Usage: adp2wav [OPTIONS] in.adp [out.wav]\n");
		printf ("Try 'adp2wav --help' for more information.\n");
		return FALSE;
	}

	inf = argv[optind];
	if (argc > (optind + 1))
		outf = argv[optind + 1];

	adp_decode_file (outf, inf, channels, rate, raw);

	return FALSE;
}

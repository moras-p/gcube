#ifndef __THPVIEW_H
#define __THPVIEW_H 1


#include <stdio.h>
#include <errno.h>
#include <SDL/SDL.h>

#include <jpeg_tools.h>
#include <general.h>
#include "mem.h"
#include "timer.h"

#define __GNU_SOURCE
#include <getopt.h>
#include <fcntl.h>

#define THPVIEW_VERSION					"0.2"

#define EXTS(n,X)					((__s32)((X) << (32 - (n))) >> (32 - (n)))


#define THP_MAGIC 0x54485000
#define COMPONENT_VIDEO_ID			0
#define COMPONENT_AUDIO_ID			1
#define COMPONENT_NONE_ID				255

#define COMPONENT_VIDEO_SIZE(X)		((THP_VERSION (X) == 0x00010000) ? 8 : 12)
#define COMPONENT_AUDIO_SIZE(X)		((THP_VERSION (X) == 0x00010000) ? 12 : 16)

#define THP_HEADER(X)							((THPHeader *) X)
#define THP_AUDIO_PRESENT(X)			(THP_HEADER (X)->max_audio_samples)
#define THP_VERSION(X)						(BSWAP32 (THP_HEADER (X)->version))
#define THP_COMPONENT_DATA(X)			((THPComponentData *) (X + BSWAP32 (THP_HEADER (X)->component_data_offset)))
#define THP_COMPONENTS_INFO(X)		((void *) (sizeof (THPComponentData) + (void *) THP_COMPONENT_DATA (X)))
#define THP_VIDEO_INFO(X)					((THPVideoInfo *) (sizeof (THPComponentData) + (void *) THP_COMPONENT_DATA (X)))
#define THP_AUDIO_INFO(X)					((THPAudioInfo *) (THP_AUDIO_PRESENT (X) ? (((THP_VERSION (X) == 0x00010000) ? 8 : 12) + (void *) THP_VIDEO_INFO (X)) : NULL))

#define JPEG_SOI_MARKER 0xffd8
#define JPEG_APP0_MARKER 0xffe0
#define JPEG_SOS_MARKER 0xffda
#define JPEG_EOI_MARKER 0xffd9

#define SOI_MARKER 0xd8
#define SOS_MARKER 0xda
#define EOI_MARKER 0xd9

#define CSPACE_GRAYSCALE 0
#define CSPACE_RGB       1

typedef struct
{
	__u32 magic;
	__u32 version;
	__u32 max_buffer_size;
	__u32 max_audio_samples;
	float framerate;
	__u32 num_frames;
	__u32 first_frame_size;
	__u32 movie_data_size;
	__u32 component_data_offset;
	__u32 offsets_data_offset;
	__u32 first_frame_offset;
	__u32 last_frame_offset;
} THPHeader;


typedef struct
{
	__u32 num_components;
	__u8 components[16];
} THPComponentData;


typedef struct
{
	__u32 width;
	__u32 height;
	__u32 reserved; // for version 1.1
} THPVideoInfo;


typedef struct
{
	__u32 num_channels;
	__u32 frequency;
	__u32 num_samples;
	__u32 num_data;	// for version 1.1
} THPAudioInfo;


typedef struct
{
	__u32 last_frame_size;
	__u32 next_frame_size;
	__u32 video_data_size;
	__u32 audio_data_size;
	// other components sizes
} THPFrameHeader;


#pragma pack(1)
struct JPEGHeaderTag
{
	__u16 length;
	char magic[5];
	__u16 version;
	__u8 units;
	__u16 Xdensity, Ydensity;
	__u8 Xthumbnail, Ythumbnail;
} __attribute__ ((packed));
typedef struct JPEGHeaderTag JPEGHeader;

// includes eoi marker
#define JFRAME_HEADER_SIZE (sizeof (JPEGHeader) + 4 + 2)

#pragma pack(1)
typedef struct
{
	__u8 *data, *end_of_header;
	__u32 size;
} JPEGFrame;

#pragma pack(1)
typedef struct
{
	__u8 *data;
	__u32 size;
} AUDIOFrame;

#pragma pack(1)
struct AUDIOHeaderTag
{
//	__u32 channel_size;
	__u16 channel_size;
	__u32 num_samples;
	__u16 c0_table[16];
	__u16 c1_table[16];
	__s16 c0_prev1, c0_prev2;
	__s16 c1_prev1, c1_prev2;
} __attribute__ ((packed));
typedef struct AUDIOHeaderTag AUDIOHeader;

typedef struct
{
	unsigned int len, pos, loaded;
	unsigned char data[0x10000];
} AudioBuffer;

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


#endif // __THPVIEW_H

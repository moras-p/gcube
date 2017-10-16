/*
 *  tplx
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
 *  simple TPL texture extractor
 *
 *
 */

#include <stdio.h>
#include <errno.h>

#include "general.h"
#include "gx_cunpacks.h"

#define TPL_MAGIC		0x0020af30

#ifdef LIL_ENDIAN
# define MASK_ALPHA		0xff000000
#else
# define MASK_ALPHA		0xff
#endif

# define BSWAP_16(B)\
	({\
			__u16 xB = (B);\
			(((__u16) ((xB) << 8) | ((xB) >> 8)));\
	})
# define BSWAP_32(B)\
	({\
			__u32 xB = (B);\
			(((__u32) (xB << 24) | ((xB << 8) & 0xff0000) | ((xB >> 8) & 0xff00) | (xB >> 24)));\
	})

#ifdef LIL_ENDIAN
# ifndef BSWAP16
#  define BSWAP16(B)  	(BSWAP_16 (B))
# endif

# ifndef BSWAP32
#  define BSWAP32(B)		(BSWAP_32 (B))
# endif

# define BIG_BSWAP16(B)	(B)
# define BIG_BSWAP32(B)	(B)

#else

# define BSWAP16(B) 	(B)
# define BSWAP32(B)		(B)

# define BIG_BSWAP16(B)  	(BSWAP_16 (B))
# define BIG_BSWAP32(B)		(BSWAP_32 (B))

#endif // LIL_ENDIAN

#define BOTH_BSWAP16(B)		(BSWAP_16 (B))
#define BOTH_BSWAP32(B)		(BSWAP_32 (B))

typedef struct
{
	__u32 texture_header_offset;
	__u32 palette_header_offset;
} TPLTexture;

typedef struct
{
	__u16 heigth, width;
	__u32 format;
	__u32 offset;

	__u32 wrap_s, wrap_t;
	__u32 min_filter, mag_filter;

	float lod_bias;

	__u8 edge_lod;
	__u8 min_lod, max_lod;
	__u8 unpacked;
} TPLTextureHeader;

typedef struct
{
	__u16 nitems;
	__u8 unpacked;
	__u8 pad;

	__u32 format;
	__u32 offset;
} TPLPaletteHeader;


#define TPLHeader(n) \
			struct\
			{\
				__u32 magic;\
				__u32 ntextures;\
				__u32 texture_size;\
				TPLTexture textures[n];\
			}\


__u8 texbuff[1024*1024*4];


inline __u32 color_unpack_rgb565 (__u32 X)
{
#ifdef LIL_ENDIAN
	return (((X & 0xf800) >>  8)  | ((X & 0xe000) >> 13)  |
				  ((X & 0x07e0) <<  5)  | ((X & 0x0600) >>  1)  |
					((X & 0x001f) << 19)  | ((X & 0x001c) << 14)) |
					MASK_ALPHA;
#else
	return (((X & 0xf800) << 16)  | ((X & 0xe000) << 11)  |
				  ((X & 0x07e0) << 13)  | ((X & 0x0600) <<  7)  |
					((X & 0x001f) << 11)  | ((X & 0x001c) <<  6)) |
					MASK_ALPHA;
#endif
}


inline __u32 color_unpack_rgba4 (__u32 X)
{
	__u32 xX = X;


#ifdef LIL_ENDIAN
	xX = ((X & 0x0f00) <<  0) | ((X & 0x00f0) << 12) | ((X & 0x000f) << 24) | ((X & 0xf000) >> 12);
#else
	xX = ((X & 0x0f00) <<  8) | ((X & 0x00f0) <<  4) | ((X & 0x000f) <<  0) | ((X & 0xf000) << 12);
#endif
	return (xX | (xX << 4));
}


inline __u32 color_unpack_rgba6 (__u32 X)
{
#ifdef LIL_ENDIAN
	return ((X & 0xfc0000) >> 16) | ((X & 0xc00000) >> 22) |
				 ((X & 0x03f000) >>  2) | ((X & 0x030000) >>  8) |
				 ((X & 0x000fc0) << 12) | ((X & 0x000c00) <<  6) |
				 ((X & 0x00003f) << 26) | ((X & 0x000030) << 20);
#else
	return ((X & 0xfc0000) <<  8) | ((X & 0xc00000) <<  2) |
				 ((X & 0x03f000) <<  6) | ((X & 0x030000) <<  0) |
				 ((X & 0x000fc0) <<  4) | ((X & 0x000c00) >>  2) |
				 ((X & 0x00003f) <<  2) | ((X & 0x000030) >>  4);
#endif
}

// RGB5 1rrr rrgg gggb bbbb
inline __u32 color_unpack_rgb555 (__u32 X)
{
#ifdef LIL_ENDIAN
	return ((X & 0x7c00) >>  7) | ((X & 0x7000) >> 12) |
				 ((X & 0x03e0) <<  6) | ((X & 0x0380) <<  1) |
				 ((X & 0x001f) << 19) | ((X & 0x001c) << 14);
#else
	return ((X & 0x7c00) << 17) | ((X & 0x7000) << 12) |
				 ((X & 0x03e0) << 14) | ((X & 0x0380) <<  9) |
				 ((X & 0x001f) << 11) | ((X & 0x001c) <<  6);
#endif
}


// RGB4A3  0aaa rrrr gggg bbbb
inline __u32 color_unpack_rgb4a3 (__u32 X)
{
#ifdef LIL_ENDIAN
	return ((X & 0x0f00) >>  4) | ((X & 0x0f00) >>  8) |
				 ((X & 0x00f0) <<  8) | ((X & 0x00f0) <<  4) |
				 ((X & 0x000f) << 20) | ((X & 0x000f) << 16) |
				 ((X & 0x7000) << 17) | ((X & 0x7000) << 13);
#else
	return ((X & 0x0f00) << 20) | ((X & 0x0f00) << 16) |
				 ((X & 0x00f0) << 16) | ((X & 0x00f0) << 12) |
				 ((X & 0x000f) << 12) | ((X & 0x000f) <<  8) |
				 ((X & 0x7000) >>  7) | ((X & 0x7000) >>  8);
#endif
}


inline __u32 color_unpack_rgb5a3 (__u32 X)
{
	if (X & 0x8000)
		return (color_unpack_rgb555 (X) | MASK_ALPHA);
	else
		return color_unpack_rgb4a3 (X);
}


inline __u32 color_unpack_i4 (__u32 X)
{
	X |= X << 4;
	X |= X << 8;
	return (X | (X << 16));
}


inline __u32 color_unpack_i8 (__u32 X)
{
	X |= X << 8;
	return (X | (X << 16));
}


inline __u32 color_unpack_ia4 (__u32 X)
{
	__u32 a = X & 0xf0;


	X &= 0x0f;
	X |= X << 4;
	X |= X << 8;
#ifdef LIL_ENDIAN
	return (X | (X << 8) | (a << 24) | (a << 20));
#else
	return ((X << 16) | (X << 8) | (a >> 4) | a);
#endif
}


inline __u32 color_unpack_ia8 (__u32 X)
{
	__u32 a = X & 0xff00;


	X &= 0x00ff;

#ifdef LIL_ENDIAN
	return (X | (X << 8) | (X << 16) | (a << 16));
#else
	return ((X << 8) | (X << 16) | (X << 24) | (a >> 8));
#endif
}



#define TEX_I4_TILE_WIDTH				8
#define TEX_I4_TILE_HEIGHT			8
// i4 -> rgba
// 8/8/4
void gx_convert_texture_i4_rgba (__u8 *src, int width, int height, __u16 *tlut, int format)
{
	__u32 *dst = (__u32 *) texbuff;
	int i, j, k, l, ofs, maxl;


	for (i = 0; i < height; i += TEX_I4_TILE_HEIGHT)
	{
		for (j = 0; j < width; j += TEX_I4_TILE_WIDTH)
		{
			for (k = 0, ofs = 0; k < TEX_I4_TILE_HEIGHT; k++)
			{
				maxl = MIN (width - j, TEX_I4_TILE_WIDTH);

				for (l = 0; l < maxl; l += 2)
				{
					dst[ofs + l + 0] = color_unpack_i4 ((*src & 0xf0) >> 4);
					if (l + 1 < maxl)
						dst[ofs + l + 1] = color_unpack_i4 ((*src & 0x0f) >> 0);
					src++;
				}

				ofs += width;
				src += (TEX_I4_TILE_WIDTH - maxl)/2;
			}

			dst += maxl;
		}

		dst += width * (TEX_I4_TILE_HEIGHT - 1);
	}
}


#define TEX_I8_TILE_WIDTH				8
#define TEX_I8_TILE_HEIGHT			4
// i8 -> rgba
// 8/4/8
void gx_convert_texture_i8_rgba (__u8 *src, int width, int height, __u16 *tlut, int format)
{
	__u32 *dst = (__u32 *) texbuff;
	int i, j, k, l, ofs, maxl;


	for (i = 0; i < height; i += TEX_I8_TILE_HEIGHT)
	{
		for (j = 0; j < width; j += TEX_I8_TILE_WIDTH)
		{
			for (k = 0, ofs = 0; k < TEX_I8_TILE_HEIGHT; k++)
			{
				maxl = MIN (width - j, TEX_I8_TILE_WIDTH);

				for (l = 0; l < maxl; l++)
				{
					dst[ofs + l] = color_unpack_i8 (*src) | MASK_ALPHA;
					src++;
				}

				ofs += width;
				src += TEX_I8_TILE_WIDTH - maxl;
			}

			dst += maxl;
		}

		dst += width * (TEX_I8_TILE_HEIGHT - 1);
	}
}


#define TEX_IA4_TILE_WIDTH				8
#define TEX_IA4_TILE_HEIGHT				4
// ia4 -> rgba
// 8/4/8
void gx_convert_texture_ia4_rgba (__u8 *src, int width, int height, __u16 *tlut, int format)
{
	__u32 *dst = (__u32 *) texbuff;
	int i, j, k, l, ofs, maxl;


	for (i = 0; i < height; i += TEX_IA4_TILE_HEIGHT)
	{
		for (j = 0; j < width; j += TEX_IA4_TILE_WIDTH)
		{
			for (k = 0, ofs = 0; k < TEX_IA4_TILE_HEIGHT; k++)
			{
				maxl = MIN (width - j, TEX_IA4_TILE_WIDTH);

				for (l = 0; l < maxl; l++)
				{
					dst[ofs + l + 0] = color_unpack_ia4 (*src);
					src++;
				}

				ofs += width;
				src += TEX_IA4_TILE_WIDTH - maxl;
			}

			dst += maxl;
		}

		dst += width * (TEX_IA4_TILE_HEIGHT - 1);
	}
}


#define TEX_IA8_TILE_WIDTH				4
#define TEX_IA8_TILE_HEIGHT				4
// ia8 -> rgba
// 4/4/16
void gx_convert_texture_ia8_rgba (__u8 *data, int width, int height, __u16 *tlut, int format)
{
	__u32 *dst = (__u32 *) texbuff;
	__u16 *src = (__u16 *) data;
	int i, j, k, l, ofs, maxl;


	for (i = 0; i < height; i += TEX_IA8_TILE_HEIGHT)
	{
		for (j = 0; j < width; j += TEX_IA8_TILE_WIDTH)
		{
			for (k = 0, ofs = 0; k < TEX_IA8_TILE_HEIGHT; k++)
			{
				maxl = MIN (width - j, TEX_IA8_TILE_WIDTH);

				for (l = 0; l < maxl; l++)
				{
					dst[ofs + l] = color_unpack_ia8 (BSWAP16 (*src));
					src++;
				}

				ofs += width;
				src += TEX_IA8_TILE_WIDTH - maxl;
			}

			dst += maxl;
		}

		dst += width * (TEX_IA8_TILE_HEIGHT - 1);
	}
}


#define TEX_RGB565_TILE_WIDTH				4
#define TEX_RGB565_TILE_HEIGHT			4
// rgb565 -> rgba
// 4/4/16
void gx_convert_texture_rgb565_rgba (__u8 *data, int width, int height, __u16 *tlut, int format)
{
	__u32 *dst = (__u32 *) texbuff;
	__u16 *src = (__u16 *) data;
	int i, j, k, l, ofs, maxl;


	for (i = 0; i < height; i += TEX_RGB565_TILE_HEIGHT)
	{
		for (j = 0; j < width; j += TEX_RGB565_TILE_WIDTH)
		{
			for (k = 0, ofs = 0; k < TEX_RGB565_TILE_HEIGHT; k++)
			{
				maxl = MIN (width - j, TEX_RGB565_TILE_WIDTH);

				for (l = 0; l < maxl; l++)
				{
					dst[ofs + l] = color_unpack_rgb565 (BSWAP16 (*src));
					src++;
				}

				ofs += width;
				src += TEX_RGB565_TILE_WIDTH - maxl;
			}

			dst += maxl;
		}

		dst += width * (TEX_RGB565_TILE_HEIGHT - 1);
	}
}


#define TEX_RGB5A3_TILE_WIDTH				4
#define TEX_RGB5A3_TILE_HEIGHT			4
// rgb5a3 -> rgba
// 4/4/16
void gx_convert_texture_rgb5a3_rgba (__u8 *data, int width, int height, __u16 *tlut, int format)
{
	__u32 *dst = (__u32 *) texbuff;
	__u16 *src = (__u16 *) data;
	int i, j, k, l, ofs, maxl;


	for (i = 0; i < height; i += TEX_RGB5A3_TILE_HEIGHT)
	{
		for (j = 0; j < width; j += TEX_RGB5A3_TILE_WIDTH)
		{
			for (k = 0, ofs = 0; k < TEX_RGB5A3_TILE_HEIGHT; k++)
			{
				maxl = MIN (width - j, TEX_RGB5A3_TILE_WIDTH);

				for (l = 0; l < maxl; l++)
				{
					dst[ofs + l] = color_unpack_rgb5a3 (BSWAP16 (*src));
					src++;
				}

				ofs += width;
				src += TEX_RGB5A3_TILE_WIDTH - maxl;
			}

			dst += maxl;
		}

		dst += width * (TEX_RGB5A3_TILE_HEIGHT - 1);
	}
}


#define TEX_RGBA8_TILE_WIDTH			4
#define TEX_RGBA8_TILE_HEIGHT			4
// rgba8 -> rgba
// 4/4/32 (two cache lines)
void gx_convert_texture_rgba8_rgba (__u8 *data, int width, int height, __u16 *tlut, int format)
{
	__u32 *dst = (__u32 *) texbuff;
	__u16 *src = (__u16 *) data;
	int i, j, k, l, ofs, maxl;
	__u32 t;


	for (i = 0; i < height; i += TEX_RGBA8_TILE_HEIGHT)
	{
		for (j = 0; j < width; j += TEX_RGBA8_TILE_WIDTH)
		{
			for (k = 0, ofs = 0; k < TEX_RGBA8_TILE_HEIGHT; k++)
			{
				maxl = MIN (width - j, TEX_RGBA8_TILE_WIDTH);

				for (l = 0; l < maxl; l++)
				{
					t = (BSWAP16 (src[0]) << 16) | BSWAP16 (src[16]);
					dst[ofs + l] = BSWAP32 ((t << 8) | ((t >> 24) & 0xff));
					src++;
				}

				ofs += width;
				src += TEX_RGBA8_TILE_WIDTH - maxl;
			}

			dst += maxl;
			src += 16;
		}

		dst += width * (TEX_RGBA8_TILE_HEIGHT - 1);
	}
}


__u32 (*tlut_unpack[]) (__u32 c) =
{
	color_unpack_ia8,
	color_unpack_rgb565,
	color_unpack_rgb5a3,
};

#define TEX_CI4_TILE_WIDTH				8
#define TEX_CI4_TILE_HEIGHT				8
// ci4 -> rgba
// 8/8/4
void gx_convert_texture_ci4_rgba (__u8 *src, int width, int height, __u16 *tlut, int format)
{
	__u32 *dst = (__u32 *) texbuff;
	int i, j, k, l, ofs, maxl;
	__u32 (*color_unpack) (__u32 c) = tlut_unpack[format & 3];


	tlut += 0xf0;

	for (i = 0; i < height; i += TEX_CI4_TILE_HEIGHT)
	{
		for (j = 0; j < width; j += TEX_CI4_TILE_WIDTH)
		{
			for (k = 0, ofs = 0; k < TEX_CI4_TILE_HEIGHT; k++)
			{
				maxl = MIN (width - j, TEX_CI4_TILE_WIDTH);

				for (l = 0; l < maxl; l += 2)
				{
					dst[ofs + l + 0] = color_unpack (BSWAP16 (tlut[(*src & 0xf0) >> 4]));
					if (l + 1 < maxl)
						dst[ofs + l + 1] = color_unpack (BSWAP16 (tlut[(*src & 0x0f) >> 0]));
					src++;
				}

				ofs += width;
				src += (TEX_CI4_TILE_WIDTH - maxl)/2;
			}

			dst += maxl;
		}

		dst += width * (TEX_CI4_TILE_HEIGHT - 1);
	}
}


#define TEX_CI8_TILE_WIDTH				8
#define TEX_CI8_TILE_HEIGHT				4
// ci8 -> rgba
// 8/4/8
void gx_convert_texture_ci8_rgba (__u8 *src, int width, int height, __u16 *tlut, int format)
{
	__u32 *dst = (__u32 *) texbuff;
	int i, j, k, l, ofs, maxl;
	__u32 (*color_unpack) (__u32 c) = tlut_unpack[format & 3];


	for (i = 0; i < height; i += TEX_CI8_TILE_HEIGHT)
	{
		for (j = 0; j < width; j += TEX_CI8_TILE_WIDTH)
		{
			for (k = 0, ofs = 0; k < TEX_CI8_TILE_HEIGHT; k++)
			{
				maxl = MIN (width - j, TEX_CI8_TILE_WIDTH);

				for (l = 0; l < maxl; l++)
				{
					dst[ofs + l] = color_unpack (BSWAP16 (tlut[*src]));
					src++;
				}

				ofs += width;
				src += TEX_CI8_TILE_WIDTH - maxl;
			}

			dst += maxl;
		}

		dst += width * (TEX_CI8_TILE_HEIGHT - 1);
	}
}


#define TEX_CI14X2_TILE_WIDTH					4
#define TEX_CI14X2_TILE_HEIGHT				4
// ci14x2 -> rgba
// 4/4/16
void gx_convert_texture_ci14x2_rgba (__u8 *data, int width, int height, __u16 *tlut, int format)
{
	__u32 *dst = (__u32 *) texbuff;
	__u16 *src = (__u16 *) data;
	int i, j, k, l, ofs, maxl;
	__u32 (*color_unpack) (__u32 c) = tlut_unpack[format & 3];


	for (i = 0; i < height; i += TEX_CI14X2_TILE_HEIGHT)
	{
		for (j = 0; j < width; j += TEX_CI14X2_TILE_WIDTH)
		{
			for (k = 0, ofs = 0; k < TEX_CI14X2_TILE_HEIGHT; k++)
			{
				maxl = MIN (width - j, TEX_CI14X2_TILE_WIDTH);

				for (l = 0; l < maxl; l++)
				{
					dst[ofs + l] = color_unpack (BSWAP16 (tlut[BSWAP16 (*src) & 0x3fff]));
					src++;
				}

				ofs += width;
				src += TEX_CI14X2_TILE_WIDTH - maxl;
			}

			dst += maxl;
		}

		dst += width * (TEX_CI14X2_TILE_HEIGHT - 1);
	}
}


__u32 icolor (__u32 a, __u32 b, float fa, float fb, float fc)
{
	__u8 *aa = (__u8 *) &a;
	__u8 *bb = (__u8 *) &b;
	__u8 cc[4];
	int i;


	for (i = 0; i < 4; i++)
		cc[i] = ((__u32) aa[i]*fa + (__u32) bb[i]*fb) / fc;

	return (*(__u32 *) cc);
}


#define TEX_CMP_TILE_WIDTH			8
#define TEX_CMP_TILE_HEIGHT			8
// cmp -> rgba
// 8/8/16 (2x2 blocks of 4x4 texels)
void gx_convert_texture_cmp_rgba (__u8 *data, int width, int height, __u16 *tlut, int format)
{
	__u32 *dst = (__u32 *) texbuff;
	__u16 *src = (__u16 *) data;
	int i, j, k, l, n, ofs, maxw;
	__u32 rgb[4];
	__u8 *cm;


	for (i = 0; i < height; i += TEX_CMP_TILE_HEIGHT)
	{
		for (j = 0; j < width; j += TEX_CMP_TILE_WIDTH)
		{
			maxw = MIN (width - j, TEX_CMP_TILE_WIDTH);

			for (k = 0; k < 2; k++)
			{
				for (l = 0; l < 2; l++)
				{
					rgb[0] = color_unpack_rgb565 (BSWAP16 (src[0]));
					rgb[1] = color_unpack_rgb565 (BSWAP16 (src[1]));

					if (BSWAP16 (src[0]) > BSWAP16 (src[1]))
					{
						rgb[2] = icolor (rgb[0], rgb[1], 2, 1, 3) | MASK_ALPHA;
						rgb[3] = icolor (rgb[1], rgb[0], 2, 1, 3) | MASK_ALPHA;
					}
					else
					{
						rgb[2] = icolor (rgb[0], rgb[1], 0.5, 0.5, 1) | MASK_ALPHA;
						rgb[3] = icolor (rgb[1], rgb[0], 2, 1, 3) &~ MASK_ALPHA;
					}

					// color selection (00, 01, 10, 11)
					cm = (__u8 *) &src[2];
					for (n = 0, ofs = l*4; n < 4; n++)
					{
						// one row (4 texels)
						if (maxw > 0 + l*4)
							dst[ofs + 0] = rgb[(cm[n] & 0xc0) >> 6];
						if (maxw > 1 + l*4)
							dst[ofs + 1] = rgb[(cm[n] & 0x30) >> 4];
						if (maxw > 2 + l*4)
							dst[ofs + 2] = rgb[(cm[n] & 0x0c) >> 2];
						if (maxw > 3 + l*4)
							dst[ofs + 3] = rgb[(cm[n] & 0x03) >> 0];

						ofs += width;
					}

					src += 4;
				}

				dst += width * 4;
			}

			dst += maxw - width * 8;
		}

		dst += width * (TEX_CMP_TILE_HEIGHT - 1);
	}
}


void (*texconvert_rgba[]) (__u8 *src, int width, int height, __u16 *tlut, int format) =
{
	gx_convert_texture_i4_rgba,
	gx_convert_texture_i8_rgba,
	gx_convert_texture_ia4_rgba,
	gx_convert_texture_ia8_rgba,
	gx_convert_texture_rgb565_rgba,
	gx_convert_texture_rgb5a3_rgba,
	gx_convert_texture_rgba8_rgba,
	NULL,
	gx_convert_texture_ci4_rgba,
	gx_convert_texture_ci8_rgba,
	gx_convert_texture_ci14x2_rgba,
	NULL,
	NULL,
	NULL,
	gx_convert_texture_cmp_rgba,
	NULL,
	
	NULL,
};


char *gx_convert_tex (void *data, int w, int h, int format, __u16 *tlut, int tlut_format)
{
	if (!texconvert_rgba[format])
		return NULL;
	
	texconvert_rgba[format] (data, w, h, tlut, tlut_format);

	return texbuff;
}

#pragma pack(1)
struct TGAHeaderTag
{
	__u8 idlen;
	__u8 cmaptype;
	__u8 datatype;
	__u16 cmaporigin;
	__u16 cmaplen;
	__u8 cmapdepth;
	__u16 xorigin, yorigin;
	__u16 width, height;
	__u8 bpp;
	__u8 imgdescriptor;
} __attribute__ ((packed));
typedef struct TGAHeaderTag TGAHeader;


void tplx_save_tga (char *filename, __u32 *data, int w, int h)
{
	TGAHeader hdr;
	const char *id = "Created with TPLX";
	FILE *f;
	int i, j;
	__u32 d;


	memset (&hdr, 0, sizeof (hdr));
	hdr.datatype = 2;							// uncompressed RGBA
	hdr.width = BIG_BSWAP16 (w);
	hdr.height = BIG_BSWAP16 (h);
	hdr.bpp = 32;
	hdr.idlen = strlen (id);

	f = fopen (filename, "wb");
	if (!f)
	{
		printf ("error: Can't create file %s\n", filename);
		return;
	}

	fwrite (&hdr, 1, sizeof (hdr), f);
	fwrite (id, 1, hdr.idlen, f);

	// ABGR -> ARGB
	for (i = h - 1; i >= 0; i--)
		for (j = 0; j < w; j++)
		{
			d = (data[i*w + j] & 0xff000000) | BOTH_BSWAP32 (data[i*w + j] << 8);
			d = BIG_BSWAP32 (d);

			fwrite (&d, 1, 4, f);
		}

	fclose (f);
}


void video_dump_tex (char *filename, void *data, int w, int h, int format, __u16 *pal, int pal_format)
{
	char *buff = gx_convert_tex (data, w, h, format, pal, pal_format);


	if (!buff)
	{
		printf ("error: unknown texture format %d\n", format);
		return;
	}

	tplx_save_tga (filename, (__u32 *) buff, w, h);
}


int extract_textures (char *buff)
{
	char temp[256];
	int i;
	TPLHeader (1) *hdr = (void *)buff;


	if (BSWAP32 (hdr->magic) != TPL_MAGIC)
	{
		printf ("error: unknown TPL format %.8x\n", BSWAP32 (hdr->magic));

		return TRUE;
	}
	else
	{
		int n = BSWAP32 (((__u32 *)buff)[1]);
		// using variable length array
		TPLHeader (n) *hdr = (void *) buff;
		TPLTextureHeader *texhdr = (void *) &buff[BSWAP32 (hdr->textures[0].texture_header_offset)];
		TPLPaletteHeader *palhdr;

	
		printf ("File contains %d textures:\n", n);
		for (i = 0; i < n; i++)
		{
			printf ("[%.2d] texture format %d, width %d, height %d\n",
							i,
							BSWAP32 (texhdr[i].format),
							BSWAP16 (texhdr[i].width),
							BSWAP16 (texhdr[i].heigth));

			if (hdr->textures[i].palette_header_offset)
			{
				palhdr = (void *) &buff[BSWAP32 (hdr->textures[i].palette_header_offset)];
				printf ("     palette format %d\n", BSWAP32 (palhdr->format));

				sprintf (temp, "tex#%.2d_%.2d+%d.tga",
									i, BSWAP32 (texhdr[i].format), BSWAP32 (palhdr->format));
				video_dump_tex (temp, &buff[BSWAP32 (texhdr[i].offset)],
														BSWAP16 (texhdr[i].width), BSWAP16 (texhdr[i].heigth),
														BSWAP32 (texhdr[i].format),
														(__u16 *)&buff[BSWAP32 (palhdr->offset)],
														BSWAP32 (palhdr->format));
			}
			else
			{
				sprintf (temp, "tex#%.2d_%.2d.tga", i, BSWAP32 (texhdr[i].format));
				video_dump_tex (temp, &buff[BSWAP32 (texhdr[i].offset)],
														BSWAP16 (texhdr[i].width), BSWAP16 (texhdr[i].heigth),
														BSWAP32 (texhdr[i].format),
														NULL, 0);
			}
		}
	}

	return FALSE;
}


int main (int argc, char **argv)
{
	FILE *f;
	char *buff;
	int size, r;


	if (argc < 2)
	{
		printf ("extracts textures from tpl file\nusage: tplx infile.tpl\n");
		return TRUE;
	}

	f = fopen (argv[1], "rb");
	if (!f)
	{
		printf ("couldn't open file %s\n", argv[1]);
		perror ("");
		return TRUE;
	}

	fseek (f, 0, SEEK_END);
	size = ftell (f);
	fseek (f, 0, SEEK_SET);
	buff = malloc (size);
	fread (buff, 1, size, f);
	fclose (f);


	r = extract_textures (buff);
	free (buff);

	return r;
}

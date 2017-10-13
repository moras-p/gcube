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
 *  GX
 *  will be split into few smaller files
 *         
 */

#include "hw_gx.h"
#include <SDL/SDL_opengl.h>

#include "gl_ext.h"

TextureCache texcache;
TextureTag *texactive[8] = {0};
int texenabled[8];

TextureTag tag_render_target;


// for texture conversion
__u8 texbuff[1024*1024*4];


#define TEX_I4_TILE_WIDTH				8
#define TEX_I4_TILE_HEIGHT			8
// i4 -> rgba
// 8/8/4
void gx_convert_texture_i4_rgba (__u8 *src, int width, int height, __u16 *tlut, int format, TexFormat *tf)
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

	tf->format = GL_RGBA;
	tf->type = GL_UNSIGNED_BYTE;
	tf->internal_format = GL_RGBA;
}


// doesn't work
void gx_convert_texture_i4 (__u8 *src, int width, int height, __u16 *tlut, int format, TexFormat *tf)
{
	__u8 *dst = (__u8 *) texbuff;
	int i, j, k, l, w4, ofs, maxl;


	if (is_power_of_two (width))
		w4 = 0;
	else
		w4 = (4 - (width & 3)) & 3;

	for (i = 0; i < height; i += TEX_I4_TILE_HEIGHT)
	{
		for (j = 0; j < width; j += TEX_I4_TILE_WIDTH)
		{
			for (k = 0, ofs = 0; k < TEX_I4_TILE_HEIGHT; k++)
			{
				maxl = MIN (width - j, TEX_I4_TILE_WIDTH);

				for (l = 0; l < maxl; l += 2)
				{
					dst[ofs + l + 0]= (*src & 0xf0);
					dst[ofs + l + 1]= (*src & 0x0f) << 4;
					src++;
				}

				ofs += width + w4;
				src += (TEX_I4_TILE_WIDTH - maxl)/2;
			}

			dst += maxl;
		}

		dst += (width + w4) * (TEX_I4_TILE_HEIGHT - 1) + w4;
	}

	tf->format = GL_INTENSITY;
	tf->type = GL_UNSIGNED_BYTE;
	tf->internal_format = GL_INTENSITY4;
}


#define TEX_I8_TILE_WIDTH				8
#define TEX_I8_TILE_HEIGHT			4
// i8 -> rgba
// 8/4/8
void gx_convert_texture_i8_rgba (__u8 *src, int width, int height, __u16 *tlut, int format, TexFormat *tf)
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

	tf->format = GL_RGBA;
	tf->type = GL_UNSIGNED_BYTE;
	tf->internal_format = GL_RGBA;
}


// doesn't work
void gx_convert_texture_i8 (__u8 *src, int width, int height, __u16 *tlut, int format, TexFormat *tf)
{
	__u8 *dst = (__u8 *) texbuff;
	int i, j, k, w4, ofs, maxl;


	if (is_power_of_two (width))
		w4 = 0;
	else
		w4 = (4 - (width & 3)) & 3;

	for (i = 0; i < height; i += TEX_I8_TILE_HEIGHT)
	{
		for (j = 0; j < width; j += TEX_I8_TILE_WIDTH)
		{
			for (k = 0, ofs = 0; k < TEX_I8_TILE_HEIGHT; k++)
			{
				maxl = MIN (width - j, TEX_I8_TILE_WIDTH);

				memcpy (&dst[ofs], src, maxl);
				src += TEX_I8_TILE_WIDTH;

				ofs += width + w4;
			}

			dst += maxl;
		}

		dst += (width + w4) * (TEX_I8_TILE_HEIGHT - 1) + w4;
	}

	tf->format = GL_INTENSITY;
	tf->type = GL_UNSIGNED_BYTE;
	tf->internal_format = GL_INTENSITY8;
}


#define TEX_IA4_TILE_WIDTH				8
#define TEX_IA4_TILE_HEIGHT				4
// ia4 -> rgba
// 8/4/8
void gx_convert_texture_ia4_rgba (__u8 *src, int width, int height, __u16 *tlut, int format, TexFormat *tf)
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

	tf->format = GL_RGBA;
	tf->type = GL_UNSIGNED_BYTE;
	tf->internal_format = GL_RGBA;
}


// GL_LUMINANCE4_ALPHA4 is not working as it should (no GL_UNSIGNED_BYTE_4_4 ?)
void gx_convert_texture_ia4 (__u8 *src, int width, int height, __u16 *tlut, int format, TexFormat *tf)
{
	__u16 *dst = (__u16 *) texbuff;
	int i, j, k, l, w4, ofs, maxl;


	if (is_power_of_two (width))
		w4 = 0;
	else
		w4 = (2 - (width & 1)) & 1;

	for (i = 0; i < height; i += TEX_IA4_TILE_HEIGHT)
	{
		for (j = 0; j < width; j += TEX_IA4_TILE_WIDTH)
		{
			for (k = 0, ofs = 0; k < TEX_IA4_TILE_HEIGHT; k++)
			{
				maxl = MIN (width - j, TEX_IA4_TILE_WIDTH);

				for (l = 0; l < maxl; l++)
				{
					dst[ofs + l] = ((*src & 0x0f) << 4) | ((*src & 0xf0) <<  8);
					src++;
				}

				ofs += width + w4;
				src += TEX_IA4_TILE_WIDTH - maxl;
			}

			dst += maxl;
		}

		dst += (width + w4) * (TEX_IA4_TILE_HEIGHT - 1) + w4;
	}

	tf->format = GL_LUMINANCE_ALPHA;
	tf->type = GL_UNSIGNED_BYTE;
	tf->internal_format = GL_LUMINANCE4_ALPHA4;
}


#define TEX_IA8_TILE_WIDTH				4
#define TEX_IA8_TILE_HEIGHT				4
// ia8 -> rgba
// 4/4/16
void gx_convert_texture_ia8_rgba (__u8 *data, int width, int height, __u16 *tlut, int format, TexFormat *tf)
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

	tf->format = GL_RGBA;
	tf->type = GL_UNSIGNED_BYTE;
	tf->internal_format = GL_RGBA;
}


void gx_convert_texture_ia8 (__u8 *data, int width, int height, __u16 *tlut, int format, TexFormat *tf)
{
	__u16 *dst = (__u16 *) texbuff;
	__u16 *src = (__u16 *) data;
	int i, j, k, l, w4, ofs, maxl;


	if (is_power_of_two (width))
		w4 = 0;
	else
		w4 = (2 - (width & 1)) & 1;

	for (i = 0; i < height; i += TEX_IA8_TILE_HEIGHT)
	{
		for (j = 0; j < width; j += TEX_IA8_TILE_WIDTH)
		{
			for (k = 0, ofs = 0; k < TEX_IA8_TILE_HEIGHT; k++)
			{
				maxl = MIN (width - j, TEX_IA8_TILE_WIDTH);

				for (l = 0; l < maxl; l++)
				{
					dst[ofs + l] = BSWAP16 (*src);
					src++;
				}

				ofs += width + w4;
				src += TEX_IA8_TILE_WIDTH - maxl;
			}

			dst += maxl;
		}

		dst += (width + w4) * (TEX_IA8_TILE_HEIGHT - 1) + w4;
	}

	tf->format = GL_LUMINANCE_ALPHA;
	tf->type = GL_UNSIGNED_BYTE;
	tf->internal_format = GL_LUMINANCE8_ALPHA8;
}


#define TEX_RGB565_TILE_WIDTH				4
#define TEX_RGB565_TILE_HEIGHT			4
// rgb565 -> rgba
// 4/4/16
void gx_convert_texture_rgb565_rgba (__u8 *data, int width, int height, __u16 *tlut, int format, TexFormat *tf)
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

	tf->format = GL_RGBA;
	tf->type = GL_UNSIGNED_BYTE;
	tf->internal_format = GL_RGBA;
}


void gx_convert_texture_rgb565 (__u8 *data, int width, int height, __u16 *tlut, int format, TexFormat *tf)
{
	__u16 *dst = (__u16 *) texbuff;
	__u16 *src = (__u16 *) data;
	int i, j, k, l, w4, ofs, maxl;


	if (is_power_of_two (width))
		w4 = 0;
	else
		w4 = (2 - (width & 1)) & 1;

	for (i = 0; i < height; i += TEX_RGB565_TILE_HEIGHT)
	{
		for (j = 0; j < width; j += TEX_RGB565_TILE_WIDTH)
		{
			for (k = 0, ofs = 0; k < TEX_RGB565_TILE_HEIGHT; k++)
			{
				maxl = MIN (width - j, TEX_RGB565_TILE_WIDTH);

				for (l = 0; l < maxl; l++)
				{
					dst[ofs + l] = BSWAP16 (*src);
					src++;
				}

				ofs += width + w4;
				src += TEX_RGB565_TILE_WIDTH - maxl;
			}

			dst += maxl;
		}

		dst += (width + w4) * (TEX_RGB565_TILE_HEIGHT - 1) + w4;
	}

	tf->format = GL_RGB;
	tf->type = GL_UNSIGNED_SHORT_5_6_5;
	tf->internal_format = GL_RGB8;
}


#define TEX_RGB5A3_TILE_WIDTH				4
#define TEX_RGB5A3_TILE_HEIGHT			4
// rgb5a3 -> rgba
// 4/4/16
void gx_convert_texture_rgb5a3_rgba (__u8 *data, int width, int height, __u16 *tlut, int format, TexFormat *tf)
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

	tf->format = GL_RGBA;
	tf->type = GL_UNSIGNED_BYTE;
	tf->internal_format = GL_RGBA;
}


#define TEX_RGBA8_TILE_WIDTH			4
#define TEX_RGBA8_TILE_HEIGHT			4
// rgba8 -> rgba
// 4/4/32 (two cache lines)
void gx_convert_texture_rgba8_rgba (__u8 *data, int width, int height, __u16 *tlut, int format, TexFormat *tf)
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

	tf->format = GL_RGBA;
	tf->type = GL_UNSIGNED_INT_8_8_8_8_REV;
	tf->internal_format = GL_RGBA8;
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
void gx_convert_texture_ci4_rgba (__u8 *src, int width, int height, __u16 *tlut, int format, TexFormat *tf)
{
	__u32 *dst = (__u32 *) texbuff;
	int i, j, k, l, ofs, maxl;
	__u32 (*color_unpack) (__u32 c) = tlut_unpack[format & 3];


//	tlut += 0xf0;

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

	tf->format = GL_RGBA;
	tf->type = GL_UNSIGNED_BYTE;
	tf->internal_format = GL_RGBA;
}


#define TEX_CI8_TILE_WIDTH				8
#define TEX_CI8_TILE_HEIGHT				4
// ci8 -> rgba
// 8/4/8
void gx_convert_texture_ci8_rgba (__u8 *src, int width, int height, __u16 *tlut, int format, TexFormat *tf)
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

	tf->format = GL_RGBA;
	tf->type = GL_UNSIGNED_BYTE;
	tf->internal_format = GL_RGBA;
}


#define TEX_CI14X2_TILE_WIDTH				4
#define TEX_CI14X2_TILE_HEIGHT				4
// ci14x2 -> rgba
// 4/4/16
void gx_convert_texture_ci14x2_rgba (__u8 *data, int width, int height, __u16 *tlut, int format, TexFormat *tf)
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

	tf->format = GL_RGBA;
	tf->type = GL_UNSIGNED_BYTE;
	tf->internal_format = GL_RGBA;
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
void gx_convert_texture_cmp_rgba (__u8 *data, int width, int height, __u16 *tlut, int format, TexFormat *tf)
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

	tf->format = GL_RGBA;
	tf->type = GL_UNSIGNED_BYTE;
	tf->internal_format = GL_RGBA;
}


void (*texconvert_rgba[]) (__u8 *src, int width, int height, __u16 *tlut, int format, TexFormat *tf) =
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


void (*texconvert[]) (__u8 *src, int width, int height, __u16 *tlut, int format, TexFormat *tf) =
{
// sumthings wrong here
//	gx_convert_texture_i4,
//	gx_convert_texture_i8,
	gx_convert_texture_i4_rgba,
	gx_convert_texture_i8_rgba,

	gx_convert_texture_ia4,
	gx_convert_texture_ia8,
	gx_convert_texture_rgb565,
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


void gx_enable_texture (unsigned int index, int enable)
{
#ifdef NO_GL_EXT
	if (index != 0)
		return;
#else
	glActiveTextureARB (GL_TEXTURE0_ARB + index);
#endif
	switch (enable)
	{
		case TEX_DISABLED:
			if (texenabled[index])
			{
				if (texenabled[index] == TEX_ENABLED_P2)
					glDisable (GL_TEXTURE_2D);
				else
					glDisable (GL_TEXTURE_RECTANGLE);
				
				texenabled[index] = TEX_DISABLED;
			}
			break;

		case GL_TEXTURE_2D:	
		case TEX_ENABLED_P2:
			if (texenabled[index] == TEX_ENABLED)
				glDisable (GL_TEXTURE_RECTANGLE);
			
			glEnable (GL_TEXTURE_2D);
			texenabled[index] = TEX_ENABLED_P2;
			break;

		default:
			if (texenabled[index] == TEX_ENABLED_P2)
				glDisable (GL_TEXTURE_2D);
			
			glEnable (GL_TEXTURE_RECTANGLE);
			texenabled[index] = TEX_ENABLED;
			break;
	}
}


void gx_bind_texture (unsigned int index, TextureTag *tag)
{
	texactive[index] = tag;
	if (tag)
		glBindTexture (tag->type, tag->tex);
}


char *gx_convert_texture (__u8 *address, int width, int height, int format,
													__s16 *tlut_address, int tlut_format)
{
	TexFormat tf;


	if (!texconvert_rgba[format])
		return NULL;

	texconvert_rgba[format] (address, width, height, tlut_address, tlut_format, &tf);

	return texbuff;
}


__u32 gl_texture_calculate_size (__u32 npixels, unsigned int gl_internal_format)
{
	switch (gl_internal_format)
	{
		case GL_RGBA:
		case GL_RGB8:
		case GL_RGBA8:
			return npixels * 4;

		case GL_INTENSITY4:
		case GL_INTENSITY8:
		case GL_LUMINANCE4_ALPHA4:
		case GL_LUMINANCE8_ALPHA8:
		default:
			return npixels;
	}
}


// not sure if this is correct. check it.
__u32 gx_texture_calculate_size (__u32 width, __u32 height, unsigned int format)
{
	__u32 cacheline = 0;
	__u32 size = 0, align = 32;


	switch (format)
	{
		case TEX_FORMAT_I4:
		case TEX_FORMAT_CI4:
		case TEX_FORMAT_CMP:
			cacheline = 1;
			break;

		case TEX_FORMAT_I8:
		case TEX_FORMAT_IA4:
		case TEX_FORMAT_CI8:
			cacheline = 2;
			break;

		case TEX_FORMAT_IA8:
		case TEX_FORMAT_RGB565:
		case TEX_FORMAT_CI14X2:
		case TEX_FORMAT_RGB5A3:
			cacheline = 4;
			break;

		case TEX_FORMAT_RGBA8:
			cacheline = 8;
			align = 64;
			break;

		default:
			// shouldn't happen
			return 0;
	}

	size = width * height * cacheline / 2;
	if (size & (align - 1))
		size += align - (size & (align - 1));
	
	return size;
}


void gx_dump_active_texture (int index, int lod)
{
	char fname[256];
	TexFormat tf;
	__u32 width, height, address;
	int i;


	if (!texactive[index])
		return;

	width = texactive[index]->width;
	height = texactive[index]->height;
	address = texactive[index]->address;

	for (i = 0; i < lod; i++)
	{
		address += gx_texture_calculate_size (width, height, texactive[index]->format);
		if (width > 1)
			width >>= 1;
		if (height > 1)
			height >>= 1;
	}

	if (texactive[index]->format >= 8 && texactive[index]->format <= 10)
		sprintf (fname, "tex@%.8x_%.2d+%d_%.8x.tga",
							address | 0x80000000, texactive[index]->format,
							texactive[index]->tlut_format, texactive[index]->tlut_address);
	else
		sprintf (fname, "tex@%.8x_%.2d.tga",
						 address | 0x80000000, texactive[index]->format);

	texconvert_rgba[texactive[index]->format] (
													MEM_ADDRESS (address), width, height,
													(__s16 *) TMEM_ADDRESS (texactive[index]->tlut_address),
													texactive[index]->tlut_format,
													&tf);

	video_dump_texture (fname, texbuff, width, height);
}


void texcache_tlut_reload (__u32 tlut_address)
{
	int i;


	for (i = 0; i < texcache.ntags; i++)
		if (texcache.tags[i].tlut_address == tlut_address)
			texcache.tags[i].reload = TRUE;
}


int texcache_tag_valid (TextureTag *tag)
{
	return (MEM32 (tag->marker_address) == TEXCACHE_MAGIC);
}


void texcache_tag_validate_fast (TextureTag *tag)
{
	MEM32 (tag->marker_address) = TEXCACHE_MAGIC;
}


void texcache_tag_validate (TextureTag *tag)
{
	// magic used to reload modified textures
	tag->marker_address = tag->address;
	tag->marker_save = MEM32 (tag->marker_address);
	MEM32 (tag->marker_address) = TEXCACHE_MAGIC;
}


void texcache_tag_invalidate (TextureTag *tag)
{
	if (texcache_tag_valid (tag))
		MEM32 (tag->marker_address) = tag->marker_save;
}


void texcache_invalidate_all (void)
{
	int i;
	

	for (i = 0; i < texcache.ntags; i++)
		texcache_tag_invalidate (&texcache.tags[i]);
}


void texcache_remove_tag (TextureTag *tag)
{
	glDeleteTextures (1, &tag->tex);
	texcache.memory_used -= tag->size;	

	texcache_tag_invalidate (tag);

	// copy last cache entry overwriting this one
	if (tag != &texcache.tags[texcache.ntags - 1])
		memcpy (tag, &texcache.tags[texcache.ntags - 1], sizeof (TextureTag));

	texcache.ntags--;
}


void texcache_remove_all (void)
{
	while (texcache.ntags)
		texcache_remove_tag (&texcache.tags[texcache.ntags - 1]);
}


void texcache_remove_unused (void)
{
	int i;
	__u32 max_misses = 0, victim = 0;


	for (i = 0; i < texcache.ntags; i++)
	{
		if (max_misses < texcache.tags[i].misses)
		{
			max_misses = texcache.tags[i].misses;
			victim = i;
		}
	}

	texcache_remove_tag (&texcache.tags[victim]);
}


TextureTag *texcache_add_tag (__u32 address, __u32 tlut_address,
															unsigned int tex, unsigned int type,
															int width, int height,
															int format, int tlut_format,
															unsigned int gl_internal_format,
															int mipmap, int min_lod, int max_lod,
															__u32 even_lod)
{
	__u32 texsize = 0;
	int i;


	if (mipmap)
	{
		// calculate size including all lods
		for (i = min_lod; i <= max_lod; i++)
			texsize += gl_texture_calculate_size ((width * height) >> (2 * i),
																						gl_internal_format);
	}
	else
		texsize = gl_texture_calculate_size (width * height, gl_internal_format);

	while (texcache.memory_used + texsize > (config.texcache_size * 1024 * 1024))
		texcache_remove_unused ();

	texcache.memory_used += texsize;

	texcache.tags[texcache.ntags].size = texsize;

	texcache.tags[texcache.ntags].address = address;
	texcache.tags[texcache.ntags].tlut_address = tlut_address;
	texcache.tags[texcache.ntags].tex = tex;
	texcache.tags[texcache.ntags].type = type;
	
	texcache.tags[texcache.ntags].width = width;
	texcache.tags[texcache.ntags].height = height;

	texcache.tags[texcache.ntags].format = format;
	texcache.tags[texcache.ntags].tlut_format = tlut_format;
	
	texcache.tags[texcache.ntags].mipmap = mipmap;
	texcache.tags[texcache.ntags].mipmaps_loaded = FALSE;
	texcache.tags[texcache.ntags].even_lod = even_lod;

	texcache.tags[texcache.ntags].reload = FALSE;

	texcache_tag_validate (&texcache.tags[texcache.ntags]);

	if (type == GL_TEXTURE_2D)
		texcache.tags[texcache.ntags].p2 = TRUE;
	else
		texcache.tags[texcache.ntags].p2 = FALSE;

	texcache.tags[texcache.ntags].misses = 0;

	return &texcache.tags[texcache.ntags++];
}


TextureTag *texcache_fetch (__u32 address, int width, int height)
{
	int i;


	for (i = 0; i < texcache.ntags; i++)
	{
		if (texcache.tags[i].address == address)
		{
			if ((texcache.tags[i].width == width) && (texcache.tags[i].height == height))
			{
				if (texcache_tag_valid (&texcache.tags[i]))
					// texture is still valid
					return &texcache.tags[i];

				// texture has changed, size is the same
				// can use old texture
				texcache.tags[i].reload = TRUE;
				return &texcache.tags[i];
			}
			else
			{
				// must create another texture
				texcache_remove_tag (&texcache.tags[i]);
				return NULL;
			}
		}
		else
			texcache.tags[i].misses++;
	}

	return NULL;
}


// this function is way too long. divide it into smaller ones.
void gx_load_texture (unsigned int index)
{
	__u32 address = TEX_IMAGE_BASE (index);
	__u32 tlut_address = TLUT_TMEM_BASE (index);
	int format = TEX_FORMAT (index);
	int width = TEX_WIDTH (index);
	int height = TEX_HEIGHT (index);
	int tlut_format = TLUT_FORMAT (index);
	float min_lod = ((float) TEX_MODE_MIN_LOD (index)) / 16;
	float max_lod = ((float) TEX_MODE_MAX_LOD (index)) / 16;
	unsigned int tex, type;
	TexFormat tf;
	TextureTag *tag;
	int i;


	// non power of two textures should only be able to use GX_WRAP
	// but it seems it is not so (Paper Mario RPG)
	if ((TEX_MODE_WRAP_S (index) != 0) || (TEX_MODE_WRAP_T (index) != 0))
	{
		DEBUG (EVENT_LOG_GX, "....  NP2 texture uses invalid wrap mode!");
		// only possible with p2 textures
		// enlarge the texture. there will be trash on new textures.
		// hopefully, it won't be displayed
		if (!is_power_of_two (width))
			width = closest_upper_power_of_two (width);

		if (!is_power_of_two (height))
			height = closest_upper_power_of_two (height);
	}

	if (MEM32 (address) == XFB_MAGIC)
	{
#if 1
		gx_enable_texture (index, tag_render_target.type);
		gx_bind_texture (index, &tag_render_target);
#else
		// don't use render target
		gx_enable_texture (index, 0);
		gx_bind_texture (index, NULL);
#endif
		return;
	}

	tag = texcache_fetch (address, width, height);

	// mipmap flag might have changed since last load
	if (tag && !tag->mipmaps_loaded && TEX_IS_MIPMAPPED (index))
	{
		texcache_remove_tag (tag);
		tag = NULL;
	}

	if (tag)
	{
		if (tag->tlut_address != tlut_address)
		{
			tag->reload = TRUE;
			tag->tlut_address = tlut_address;
		}
	
		gx_enable_texture (index, tag->type);
		if (tag != texactive[index])
			gx_bind_texture (index, tag);

		gx_set_texture_mode0 (index);
		gx_set_texture_mode1 (index);

		// yet another hack
		if (gxswitches.tex_reload && (tag->even_lod != TEX_EVEN_TMEM (index)))
		{
			tag->reload = TRUE;
			tag->even_lod = TEX_EVEN_TMEM (index);
		}
		
		if (tag->reload)
		{
			// if tag valid -> tlut was changed, but texture is still the same
			texcache_tag_invalidate (tag);

			// convert base image
			texconvert[format] (MEM_ADDRESS (address),
													width, height,
													(__s16 *) TMEM_ADDRESS (tlut_address),
													tlut_format,
													&tf);
	
			glTexSubImage2D (tag->type, min_lod, 0, 0, width, height,
											 tf.format, tf.type, texbuff); 

			if (tag->mipmap && !gxswitches.use_gl_mipmaps)
			{
				// convert all mipmaps
				i = min_lod;

				while (!((width == 1) && (height == 1)))
				{
					i++;

					address += gx_texture_calculate_size (width, height, format);
					if (width > 1)
						width >>= 1;
					if (height > 1)
						height >>= 1;

					texconvert[format] (MEM_ADDRESS (address),
															width, height,
															(__s16 *) TMEM_ADDRESS (tlut_address),
															tlut_format,
															&tf);

					glTexSubImage2D (tag->type, i, 0, 0, width, height,
													 tf.format, tf.type, texbuff); 
				}

				texactive[index]->mipmaps_loaded = TRUE;
			}

			tag->reload = FALSE;
			texcache_tag_validate_fast (tag);
		}

		return;
	}

	if (!texconvert[format] || !address)
	{
		texactive[index] = NULL;
		DEBUG (EVENT_LOG_GX, "....  TEX couldn't convert texture %.8x", address);
		return;
	}

	DEBUG (EVENT_LOG_GX, "....  load tex from %.8x W %d H %d F %d tlut %.8x F %d",
				 address | 0x80000000, width, height, format, tlut_address, tlut_format);

	if (is_power_of_two (width) && is_power_of_two (height))
		type = GL_TEXTURE_2D;
	else
		type = GL_TEXTURE_RECTANGLE;

	// convert base image
	texconvert[format] (MEM_ADDRESS (address),
											width, height,
											(__s16 *) TMEM_ADDRESS (tlut_address),
											tlut_format,
											&tf);
	// load texture
	gx_enable_texture (index, type);
	glGenTextures (1, &tex);

	// add tag
	texactive[index] = texcache_add_tag (address, tlut_address, tex, type,
																			 width, height, format, tlut_format,
																			 tf.internal_format,
																			 TEX_IS_MIPMAPPED (index),
																			 (int) min_lod, (int) max_lod,
																			 TEX_EVEN_TMEM (index));

	gx_bind_texture (index, texactive[index]);

	gx_set_texture_mode0 (index);
	gx_set_texture_mode1 (index);
	
	glTexImage2D (type, min_lod, tf.internal_format, width, height,
								0, tf.format, tf.type, texbuff);

	if (gxswitches.dump_textures)
		gx_dump_active_texture (index, min_lod);

	if (texactive[index]->mipmap && !gxswitches.use_gl_mipmaps)
	{
		// convert all mipmaps
		i = min_lod;

		while (!((width == 1) && (height == 1)))
		{
			i++;
			if (i > max_lod)
				break;
			address += gx_texture_calculate_size (width, height, format);
			if (width > 1)
				width >>= 1;
			if (height > 1)
				height >>= 1;

			texconvert[format] (MEM_ADDRESS (address),
													width, height,
													(__s16 *) TMEM_ADDRESS (tlut_address),
													tlut_format,
													&tf);

			glTexImage2D (type, i, tf.internal_format, width, height,
										0, tf.format, tf.type, texbuff);

			if (gxswitches.dump_textures)
				gx_dump_active_texture (index, i);
		}
	}

	if (texactive[index]->mipmap)
		texactive[index]->mipmaps_loaded = TRUE;
}



void gx_create_render_target (void)
{
	memset (&tag_render_target, 0, sizeof (tag_render_target));

	glGenTextures (1, &tag_render_target.tex);
	glBindTexture (GL_TEXTURE_RECTANGLE, tag_render_target.tex);
	glTexParameteri (GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D (GL_TEXTURE_RECTANGLE, 0, GL_RGBA, screen_width, screen_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	tag_render_target.type = GL_TEXTURE_RECTANGLE;
	tag_render_target.width = screen_width;
	tag_render_target.height = screen_height;
	tag_render_target.p2 = FALSE;
}


void gx_render_to_texture (__u32 address, unsigned int x, unsigned int y,
													 unsigned int w, unsigned int h, int mipmap)
{
		if (!tag_render_target.tex)
			gx_create_render_target ();

		glBindTexture (GL_TEXTURE_RECTANGLE, tag_render_target.tex);
		glReadBuffer (GL_BACK);
		glCopyTexImage2D (GL_TEXTURE_RECTANGLE, 0, GL_RGBA, x, screen_height - (y + h), w, h, 0);

		MEM32 (XFB_ADDRESS) = XFB_MAGIC;

		tag_render_target.xfb_mipmap = mipmap;
		tag_render_target.p2 = FALSE;
		tag_render_target.type = GL_TEXTURE_RECTANGLE;

		tag_render_target.width = w;
		tag_render_target.height = h;
		tag_render_target.address = address;
}

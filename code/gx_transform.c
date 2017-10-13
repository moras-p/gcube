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


// pushed matrix indices
int pn_matrix_index;
int tex_matrix_index[8];
// dequantization factor
float vdq, ndq, tdq[8];



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
	__u32 _X = X;


#ifdef LIL_ENDIAN
	_X = ((X & 0x0f00) <<  0) | ((X & 0x00f0) << 12) | ((X & 0x000f) << 24) | ((X & 0xf000) >> 12);
#else
	_X = ((X & 0x0f00) <<  8) | ((X & 0x00f0) <<  4) | ((X & 0x000f) <<  0) | ((X & 0xf000) << 12);
#endif
	return (_X | (_X << 4));
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


// send position / normal matrix index
unsigned int gx_send_pmidx (__u32 mem)
{
	pn_matrix_index = MEM (mem);
	
	return 1;
}


// send texture matrix index
unsigned int gx_send_t0midx (__u32 mem)
{
	tex_matrix_index[0] = MEM (mem);
	
	return 1;
}


unsigned int gx_send_t1midx (__u32 mem)
{
	tex_matrix_index[1] = MEM (mem);
	
	return 1;
}


unsigned int gx_send_t2midx (__u32 mem)
{
	tex_matrix_index[2] = MEM (mem);
	
	return 1;
}


unsigned int gx_send_t3midx (__u32 mem)
{
	tex_matrix_index[3] = MEM (mem);
	
	return 1;
}


unsigned int gx_send_t4midx (__u32 mem)
{
	tex_matrix_index[4] = MEM (mem);
	
	return 1;
}


unsigned int gx_send_t5midx (__u32 mem)
{
	tex_matrix_index[5] = MEM (mem);
	
	return 1;
}


unsigned int gx_send_t6midx (__u32 mem)
{
	tex_matrix_index[6] = MEM (mem);
	
	return 1;
}


unsigned int gx_send_t7midx (__u32 mem)
{
	tex_matrix_index[7] = MEM (mem);
	
	return 1;
}


// direct position
unsigned int gx_send_pu8_xy (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (mem);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", p[0], p[1]);

	glVertex2s (p[0], p[1]);

	return 2;
}


unsigned int gx_send_pu8_xyz (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (mem);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", p[0], p[1], p[2]);

	glVertex3s (p[0], p[1], p[2]);

	return 3;
}


unsigned int gx_send_ps8_xy (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (mem);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", p[0], p[1]);

	glVertex2s (p[0], p[1]);

	return 2;
}


unsigned int gx_send_ps8_xyz (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (mem);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", p[0], p[1], p[2]);

	glVertex3s (p[0], p[1], p[2]);

	return 3;
}


unsigned int gx_send_pu16_xy (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", MEMR16 (mem), MEMR16 (mem + 2));

	glVertex2i (MEMR16 (mem), MEMR16 (mem + 2));

	return 4;
}


unsigned int gx_send_pu16_xyz (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", MEMR16 (mem), MEMR16 (mem + 2), MEMR16 (mem + 4));

	glVertex3i (MEMR16 (mem), MEMR16 (mem + 2), MEMR16 (mem + 4));

	return 6;
}


unsigned int gx_send_ps16_xy (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", MEMSR16 (mem), MEMSR16 (mem + 2));

	glVertex2s (MEMSR16 (mem), MEMSR16 (mem + 2));

	return 4;
}


unsigned int gx_send_ps16_xyz (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", MEMSR16 (mem), MEMSR16 (mem + 2), MEMSR16 (mem + 4));

	glVertex3s (MEMSR16 (mem), MEMSR16 (mem + 2), MEMSR16 (mem + 4));

	return 6;
}


unsigned int gx_send_pf_xy (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  pos   (%.2f, %.2f)", MEMRF (mem), MEMRF (mem + 4));

	glVertex2f (MEMRF (mem), MEMRF (mem + 4));

	return 8;
}


unsigned int gx_send_pf_xyz (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  pos   (%.2f, %.2f, %.2f)", MEMRF (mem), MEMRF (mem + 4), MEMRF (mem + 8));

	glVertex3f (MEMRF (mem), MEMRF (mem + 4), MEMRF (mem + 8));

	return 12;
}


// 8 bit indexed position
unsigned int gx_send_pi8u8_xy (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", p[0], p[1]);

	glVertex2s (p[0], p[1]);

	return 1;
}


unsigned int gx_send_pi8u8_xyz (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", p[0], p[1], p[2]);

	glVertex3s (p[0], p[1], p[2]);

	return 1;
}


unsigned int gx_send_pi8s8_xy (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", p[0], p[1]);

	glVertex2s (p[0], p[1]);

	return 1;
}


unsigned int gx_send_pi8s8_xyz (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", p[0], p[1], p[2]);

	glVertex3s (p[0], p[1], p[2]);

	return 1;
}


unsigned int gx_send_pi8u16_xy (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", BSWAP16 (p[0]), BSWAP16 (p[1]));

	glVertex2i (BSWAP16 (p[0]), BSWAP16 (p[1]));

	return 1;
}


unsigned int gx_send_pi8u16_xyz (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", BSWAP16 (p[0]), BSWAP16 (p[1]), BSWAP16 (p[2]));

	glVertex3i (BSWAP16 (p[0]), BSWAP16 (p[1]), BSWAP16 (p[2]));

	return 1;
}


unsigned int gx_send_pi8s16_xy (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", BSWAPS16 (p[0]), BSWAPS16 (p[1]));

	glVertex2s (BSWAPS16 (p[0]), BSWAPS16 (p[1]));

	return 1;
}


unsigned int gx_send_pi8s16_xyz (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", BSWAPS16 (p[0]), BSWAPS16 (p[1]), BSWAPS16 (p[2]));

	glVertex3s (BSWAPS16 (p[0]), BSWAPS16 (p[1]), BSWAPS16 (p[2]));

	return 1;
}


unsigned int gx_send_pi8f_xy (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%.2f, %.2f)", BSWAPF (p[0]), BSWAPF (p[1]));

	glVertex2f (BSWAPF (p[0]), BSWAPF (p[1]));

	return 1;
}


unsigned int gx_send_pi8f_xyz (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%.2f, %.2f, %.2f)", BSWAPF (p[0]), BSWAPF (p[1]), BSWAPF (p[2]));

	glVertex3f (BSWAPF (p[0]), BSWAPF (p[1]), BSWAPF (p[2]));

	return 1;
}


// 16 bit indexed position
unsigned int gx_send_pi16u8_xy (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", p[0], p[1]);

	glVertex2s (p[0], p[1]);

	return 2;
}


unsigned int gx_send_pi16u8_xyz (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", p[0], p[1], p[2]);

	glVertex3s (p[0], p[1], p[2]);

	return 2;
}


unsigned int gx_send_pi16s8_xy (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", p[0], p[1]);

	glVertex2s (p[0], p[1]);

	return 2;
}


unsigned int gx_send_pi16s8_xyz (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", p[0], p[1], p[2]);

	glVertex3s (p[0], p[1], p[2]);

	return 2;
}


unsigned int gx_send_pi16u16_xy (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", BSWAP16 (p[0]), BSWAP16 (p[1]));

	glVertex2i (BSWAP16 (p[0]), BSWAP16 (p[1]));

	return 2;
}


unsigned int gx_send_pi16u16_xyz (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", BSWAP16 (p[0]), BSWAP16 (p[1]), BSWAP16 (p[2]));

	glVertex3i (BSWAP16 (p[0]), BSWAP16 (p[1]), BSWAP16 (p[2]));

	return 2;
}


unsigned int gx_send_pi16s16_xy (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", BSWAPS16 (p[0]), BSWAPS16 (p[1]));

	glVertex2s (BSWAPS16 (p[0]), BSWAPS16 (p[1]));

	return 2;
}


unsigned int gx_send_pi16s16_xyz (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", BSWAPS16 (p[0]), BSWAPS16 (p[1]), BSWAPS16 (p[2]));

	glVertex3s (BSWAPS16 (p[0]), BSWAPS16 (p[1]), BSWAPS16 (p[2]));

	return 2;
}


unsigned int gx_send_pi16f_xy (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%.2f, %.2f)", BSWAPF (p[0]), BSWAPF (p[1]));

	glVertex2f (BSWAPF (p[0]), BSWAPF (p[1]));

	return 2;
}


unsigned int gx_send_pi16f_xyz (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%.2f, %.2f, %.2f)", BSWAPF (p[0]), BSWAPF (p[1]), BSWAPF (p[2]));

	glVertex3f (BSWAPF (p[0]), BSWAPF (p[1]), BSWAPF (p[2]));

	return 2;
}


static unsigned int (*gxpos[]) (__u32 mem) =
{
	NULL,

	gx_send_pu8_xy,
	gx_send_ps8_xy,
	gx_send_pu16_xy,
	gx_send_ps16_xy,
	gx_send_pf_xy,
	NULL, NULL, NULL,

	gx_send_pu8_xyz,
	gx_send_ps8_xyz,
	gx_send_pu16_xyz,
	gx_send_ps16_xyz,
	gx_send_pf_xyz,
	NULL, NULL, NULL,

	gx_send_pi8u8_xy,
	gx_send_pi8s8_xy,
	gx_send_pi8u16_xy,
	gx_send_pi8s16_xy,
	gx_send_pi8f_xy,
	NULL, NULL, NULL,

	gx_send_pi8u8_xyz,
	gx_send_pi8s8_xyz,
	gx_send_pi8u16_xyz,
	gx_send_pi8s16_xyz,
	gx_send_pi8f_xyz,
	NULL, NULL, NULL,

	gx_send_pi16u8_xy,
	gx_send_pi16s8_xy,
	gx_send_pi16u16_xy,
	gx_send_pi16s16_xy,
	gx_send_pi16f_xy,
	NULL, NULL, NULL,

	gx_send_pi16u8_xyz,
	gx_send_pi16s8_xyz,
	gx_send_pi16u16_xyz,
	gx_send_pi16s16_xyz,
	gx_send_pi16f_xyz,
	NULL, NULL, NULL,
};


static unsigned int gxpos_size[] =
{
	0,
	2, 2, 4, 4, 8, 0, 0, 0,
	3, 3, 6, 6, 12, 0, 0, 0,
	1, 1, 1, 1, 1, 0, 0, 0,
	1, 1, 1, 1, 1, 0, 0, 0,
	2, 2, 2, 2, 2, 0, 0, 0,
	2, 2, 2, 2, 2, 0, 0, 0,
};


inline void transform_position_xy (float d[3], float v[2], int index)
{
	float *m = &XFMAT_GEO (index);


	d[0] = m[0]*v[0] + m[1]*v[1] + m[ 3];
	d[1] = m[4]*v[0] + m[5]*v[1] + m[ 7];
	d[2] = m[8]*v[0] + m[9]*v[1] + m[11];
}


inline void transform_position_xyz (float d[3], float v[3], int index)
{
	float *m = &XFMAT_GEO (index);


	d[0] = m[0]*v[0] + m[1]*v[1] + m[ 2]*v[2] + m[ 3];
	d[1] = m[4]*v[0] + m[5]*v[1] + m[ 6]*v[2] + m[ 7];
	d[2] = m[8]*v[0] + m[9]*v[1] + m[10]*v[2] + m[11];
}


// direct position transformed by matrix
unsigned int gx_send_pu8_xy_midx (__u32 mem)
{
	float d[3], v[2] =
	{
		vdq * MEM (mem), vdq * MEM (mem + 1),
	};


	transform_position_xy (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 2;
}


unsigned int gx_send_pu8_xyz_midx (__u32 mem)
{
	float d[3], v[3] =
	{
		vdq * MEM (mem), vdq * MEM (mem + 1), vdq * MEM (mem + 2),
	};


	transform_position_xyz (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 3;
}


unsigned int gx_send_ps8_xy_midx (__u32 mem)
{
	float d[3], v[2] =
	{
		vdq * MEMS (mem), vdq * MEMS (mem + 1),
	};


	transform_position_xy (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 2;
}


unsigned int gx_send_ps8_xyz_midx (__u32 mem)
{
	float d[3], v[3] =
	{
		vdq * MEMS (mem), vdq * MEMS (mem + 1), vdq * MEMS (mem + 2),
	};


	transform_position_xyz (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 3;
}


unsigned int gx_send_pu16_xy_midx (__u32 mem)
{
	float d[3], v[2] =
	{
		vdq * MEMR16 (mem), vdq * MEMR16 (mem + 2),
	};


	transform_position_xy (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 4;
}


unsigned int gx_send_pu16_xyz_midx (__u32 mem)
{
	float d[3], v[3] =
	{
		vdq * MEMR16 (mem), vdq * MEMR16 (mem + 2), vdq * MEMR16 (mem + 4),
	};


	transform_position_xyz (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 6;
}


unsigned int gx_send_ps16_xy_midx (__u32 mem)
{
	float d[3], v[2] =
	{
		vdq * MEMSR16 (mem), vdq * MEMSR16 (mem + 2),
	};


	transform_position_xy (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 4;
}


unsigned int gx_send_ps16_xyz_midx (__u32 mem)
{
	float d[3], v[3] =
	{
		vdq * MEMSR16 (mem), vdq * MEMSR16 (mem + 2), vdq * MEMSR16 (mem + 4),
	};


	transform_position_xyz (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 6;
}


unsigned int gx_send_pf_xy_midx (__u32 mem)
{
	float d[3], v[2] =
	{
		MEMRF (mem), MEMRF (mem + 4),
	};


	transform_position_xy (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 8;
}


unsigned int gx_send_pf_xyz_midx (__u32 mem)
{
	float d[3], v[3] =
	{
		MEMRF (mem), MEMRF (mem + 4), MEMRF (mem + 8),
	};


	transform_position_xyz (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 12;
}


// 8 bit indexed position
unsigned int gx_send_pi8u8_xy_midx (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);
	float d[3], v[2] =
	{
		vdq * p[0], vdq * p[1],
	};


	transform_position_xy (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 1;
}


unsigned int gx_send_pi8u8_xyz_midx (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);
	float d[3], v[3] =
	{
		vdq * p[0], vdq * p[1], vdq * p[2],
	};


	transform_position_xyz (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 1;
}


unsigned int gx_send_pi8s8_xy_midx (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);
	float d[3], v[2] =
	{
		vdq * p[0], vdq * p[1],
	};


	transform_position_xy (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 1;
}


unsigned int gx_send_pi8s8_xyz_midx (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);
	float d[3], v[3] =
	{
		vdq * p[0], vdq * p[1], vdq * p[2],
	};


	transform_position_xyz (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 1;
}


unsigned int gx_send_pi8u16_xy_midx (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);
	float d[3], v[2] =
	{
		vdq * BSWAP16 (p[0]), vdq * BSWAP16 (p[1]),
	};


	transform_position_xy (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 1;
}


unsigned int gx_send_pi8u16_xyz_midx (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);
	float d[3], v[3] =
	{
		vdq * BSWAP16 (p[0]), vdq * BSWAP16 (p[1]), vdq * BSWAP16 (p[2]),
	};


	transform_position_xyz (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 1;
}


unsigned int gx_send_pi8s16_xy_midx (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);
	float d[3], v[2] =
	{
		vdq * BSWAPS16 (p[0]), vdq * BSWAPS16 (p[1]),
	};


	transform_position_xy (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 1;
}


unsigned int gx_send_pi8s16_xyz_midx (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);
	float d[3], v[3] =
	{
		vdq * BSWAPS16 (p[0]), vdq * BSWAPS16 (p[1]), vdq * BSWAPS16 (p[2]),
	};


	transform_position_xyz (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 1;
}


unsigned int gx_send_pi8f_xy_midx (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);
	float d[3], v[2] =
	{
		BSWAPF (p[0]), BSWAPF (p[1]),
	};


	transform_position_xy (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 1;
}


unsigned int gx_send_pi8f_xyz_midx (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);
	float d[3], v[3] =
	{
		BSWAPF (p[0]), BSWAPF (p[1]), BSWAPF (p[2]),
	};


	transform_position_xyz (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 1;
}


// 16 bit indexed position
unsigned int gx_send_pi16u8_xy_midx (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);
	float d[3], v[2] =
	{
		vdq * p[0], vdq * p[1],
	};


	transform_position_xy (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 2;
}


unsigned int gx_send_pi16u8_xyz_midx (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);
	float d[3], v[3] =
	{
		vdq * p[0], vdq * p[1], vdq * p[2],
	};


	transform_position_xyz (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 2;
}


unsigned int gx_send_pi16s8_xy_midx (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);
	float d[3], v[2] =
	{
		vdq * p[0], vdq * p[1],
	};


	transform_position_xy (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 2;
}


unsigned int gx_send_pi16s8_xyz_midx (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);
	float d[3], v[3] =
	{
		vdq * p[0], vdq * p[1], vdq * p[2],
	};


	transform_position_xyz (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 2;
}


unsigned int gx_send_pi16u16_xy_midx (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);
	float d[3], v[2] =
	{
		vdq * BSWAP16 (p[0]), vdq * BSWAP16 (p[1]),
	};


	transform_position_xy (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 2;
}


unsigned int gx_send_pi16u16_xyz_midx (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);
	float d[3], v[3] =
	{
		vdq * BSWAP16 (p[0]), vdq * BSWAP16 (p[1]), vdq * BSWAP16 (p[2]),
	};


	transform_position_xyz (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 2;
}


unsigned int gx_send_pi16s16_xy_midx (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);
	float d[3], v[2] =
	{
		vdq * BSWAPS16 (p[0]), vdq * BSWAPS16 (p[1]),
	};


	transform_position_xy (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 2;
}


unsigned int gx_send_pi16s16_xyz_midx (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);
	float d[3], v[3] =
	{
		vdq * BSWAPS16 (p[0]), vdq * BSWAPS16 (p[1]), vdq * BSWAPS16 (p[2]),
	};


	transform_position_xyz (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 2;
}


unsigned int gx_send_pi16f_xy_midx (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);
	float d[3], v[2] =
	{
		BSWAPF (p[0]), BSWAPF (p[1]),
	};


	transform_position_xy (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 2;
}


unsigned int gx_send_pi16f_xyz_midx (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);
	float d[3], v[3] =
	{
		BSWAPF (p[0]), BSWAPF (p[1]), BSWAPF (p[2]),
	};


	transform_position_xyz (d, v, pn_matrix_index);
	DEBUG (EVENT_LOG_GX, "....  pos T (%.2f, %.2f, %.2f)", d[0], d[1], d[2]);
	glVertex3fv (d);

	return 2;
}


static unsigned int (*gxpos_midx[]) (__u32 mem) =
{
	NULL,

	gx_send_pu8_xy_midx,
	gx_send_ps8_xy_midx,
	gx_send_pu16_xy_midx,
	gx_send_ps16_xy_midx,
	gx_send_pf_xy_midx,
	NULL, NULL, NULL,

	gx_send_pu8_xyz_midx,
	gx_send_ps8_xyz_midx,
	gx_send_pu16_xyz_midx,
	gx_send_ps16_xyz_midx,
	gx_send_pf_xyz_midx,
	NULL, NULL, NULL,

	gx_send_pi8u8_xy_midx,
	gx_send_pi8s8_xy_midx,
	gx_send_pi8u16_xy_midx,
	gx_send_pi8s16_xy_midx,
	gx_send_pi8f_xy_midx,
	NULL, NULL, NULL,

	gx_send_pi8u8_xyz_midx,
	gx_send_pi8s8_xyz_midx,
	gx_send_pi8u16_xyz_midx,
	gx_send_pi8s16_xyz_midx,
	gx_send_pi8f_xyz_midx,
	NULL, NULL, NULL,

	gx_send_pi16u8_xy_midx,
	gx_send_pi16s8_xy_midx,
	gx_send_pi16u16_xy_midx,
	gx_send_pi16s16_xy_midx,
	gx_send_pi16f_xy_midx,
	NULL, NULL, NULL,

	gx_send_pi16u8_xyz_midx,
	gx_send_pi16s8_xyz_midx,
	gx_send_pi16u16_xyz_midx,
	gx_send_pi16s16_xyz_midx,
	gx_send_pi16f_xyz_midx,
	NULL, NULL, NULL,
};


// direct normal
unsigned int gx_send_ns8_xyz (__u32 mem)
{
	return 3;
}


unsigned int gx_send_ns16_xyz (__u32 mem)
{
	return 6;
}


unsigned int gx_send_nf_xyz (__u32 mem)
{
	return 12;
}


unsigned int gx_send_ns8_nbt (__u32 mem)
{
	return 9;
}


unsigned int gx_send_ns16_nbt (__u32 mem)
{
	return 18;
}


unsigned int gx_send_nf_nbt (__u32 mem)
{
	return 36;
}


// 8 bit indexed normal
unsigned int gx_send_ni8s8_xyz (__u32 mem)
{
	return 1;
}


unsigned int gx_send_ni8s16_xyz (__u32 mem)
{
	return 1;
}


unsigned int gx_send_ni8f_xyz (__u32 mem)
{
	return 1;
}


unsigned int gx_send_ni8s8_nbt (__u32 mem)
{
	return 1;
}


unsigned int gx_send_ni8s16_nbt (__u32 mem)
{
	return 1;
}


unsigned int gx_send_ni8f_nbt (__u32 mem)
{
	return 1;
}


unsigned int gx_send_ni8s8_nbt3 (__u32 mem)
{
	return 3;
}


unsigned int gx_send_ni8s16_nbt3 (__u32 mem)
{
	return 3;
}


unsigned int gx_send_ni8f_nbt3 (__u32 mem)
{
	return 3;
}


// 16 bit indexed normal
unsigned int gx_send_ni16s8_xyz (__u32 mem)
{
	return 2;
}


unsigned int gx_send_ni16s16_xyz (__u32 mem)
{
	return 2;
}


unsigned int gx_send_ni16f_xyz (__u32 mem)
{
	return 2;
}


unsigned int gx_send_ni16s8_nbt (__u32 mem)
{
	return 2;
}


unsigned int gx_send_ni16s16_nbt (__u32 mem)
{
	return 2;
}


unsigned int gx_send_ni16f_nbt (__u32 mem)
{
	return 2;
}


unsigned int gx_send_ni16s8_nbt3 (__u32 mem)
{
	return 6;
}


unsigned int gx_send_ni16s16_nbt3 (__u32 mem)
{
	return 6;
}


unsigned int gx_send_ni16f_nbt3 (__u32 mem)
{
	return 6;
}


static unsigned int (*gxnormal[]) (__u32 mem) =
{
	NULL,

	NULL,
	gx_send_ns8_xyz,
	NULL,
	gx_send_ns16_xyz,
	gx_send_nf_xyz,
	NULL, NULL, NULL,

	NULL,
	gx_send_ns8_nbt,
	NULL,
	gx_send_ns16_nbt,
	gx_send_nf_nbt,
	NULL, NULL, NULL,

	NULL,
	gx_send_ni8s8_xyz,
	NULL,
	gx_send_ni8s16_xyz,
	gx_send_ni8f_xyz,
	NULL, NULL, NULL,

	NULL,
	gx_send_ni8s8_nbt,
	NULL,
	gx_send_ni8s16_nbt,
	gx_send_ni8f_nbt,
	NULL, NULL, NULL,

	NULL,
	gx_send_ni16s8_xyz,
	NULL,
	gx_send_ni16s16_xyz,
	gx_send_ni16f_xyz,
	NULL, NULL, NULL,

	NULL,
	gx_send_ni16s8_nbt,
	NULL,
	gx_send_ni16s16_nbt,
	gx_send_ni16f_nbt,
	NULL, NULL, NULL,

	NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL,

	NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL,

	NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL,

	NULL,
	gx_send_ni8s8_nbt3,
	NULL,
	gx_send_ni8s16_nbt3,
	gx_send_ni8f_nbt3,
	NULL, NULL, NULL,

	NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL,

	NULL,
	gx_send_ni16s8_nbt3,
	NULL,
	gx_send_ni16s16_nbt3,
	gx_send_ni16f_nbt3,
	NULL, NULL, NULL,
};


static unsigned int gxnormal_size[] =
{
	0,

	0, 3, 0,  6, 12, 0, 0, 0,
	0, 9, 0, 18, 36, 0, 0, 0,

	0, 1, 0, 1, 1, 0, 0, 0,
	0, 1, 0, 1, 1, 0, 0, 0,

	0, 2, 0, 2, 2, 0, 0, 0,
	0, 2, 0, 2, 2, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,

	0, 3, 0, 3, 3, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 6, 0, 6, 6, 0, 0, 0,
};


// direct color
unsigned int gx_send_c_rgb565 (__u32 mem)
{
	__u32 c = color_unpack_rgb565 (MEMR16 (mem));


	DEBUG (EVENT_LOG_GX, "....  color (%6.6x)", c);

	glColor3ubv ((__u8 *) &c);

	return 2;
}


unsigned int gx_send_c_rgb8 (__u32 mem)
{
	__u32 c = MEM32 (mem);


	DEBUG (EVENT_LOG_GX, "....  color (%6.6x)", c);

	glColor3ubv ((__u8 *) &c);

	return 3;
}


unsigned int gx_send_c_rgbx8 (__u32 mem)
{
	__u32 c = MEM32 (mem);


	DEBUG (EVENT_LOG_GX, "....  color (%6.6x)", c);

	glColor3ubv ((__u8 *) &c);

	return 4;
}


unsigned int gx_send_c_rgba4 (__u32 mem)
{
	__u32 c = color_unpack_rgba4 (MEMR32 (mem) >> 16);


	DEBUG (EVENT_LOG_GX, "....  color (%8.8x)", c);

	glColor4ubv ((__u8 *) &c);


	return 2;
}


unsigned int gx_send_c_rgba6 (__u32 mem)
{
	__u32 c = color_unpack_rgba6 (MEMR32 (mem) >> 8);


	DEBUG (EVENT_LOG_GX, "....  color (%8.8x)", c);

	glColor4ubv ((__u8 *) &c);

	return 3;
}


unsigned int gx_send_c_rgba8 (__u32 mem)
{
	__u32 c = MEM32 (mem);


	DEBUG (EVENT_LOG_GX, "....  color (%8.8x)", c);

	glColor4ubv ((__u8 *) &c);

	return 4;
}


// 8 bit indexed color
unsigned int gx_send_ci8_rgb565 (__u32 mem)
{
	__u32 c = color_unpack_rgb565 (MEMR16 (CP_COLOR0_ARRAY_BASE + MEM (mem) * CP_COLOR0_ARRAY_STRIDE));


	DEBUG (EVENT_LOG_GX, "....  color (%6.6x)", c);

	glColor3ubv ((__u8 *) &c);

	return 1;
}


unsigned int gx_send_ci8_rgb8 (__u32 mem)
{
	__u32 c = MEM32 (CP_COLOR0_ARRAY_BASE + MEM (mem) * CP_COLOR0_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  color (%6.6x)", c);

	glColor3ubv ((__u8 *) &c);

	return 1;
}


unsigned int gx_send_ci8_rgbx8 (__u32 mem)
{
	__u32 c = MEM32 (CP_COLOR0_ARRAY_BASE + MEM (mem) * CP_COLOR0_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  color (%6.6x)", c);

	glColor3ubv ((__u8 *) &c);

	return 1;
}


unsigned int gx_send_ci8_rgba4 (__u32 mem)
{
	__u32 c = color_unpack_rgba4 (MEMR32 (CP_COLOR0_ARRAY_BASE + MEM (mem) * CP_COLOR0_ARRAY_STRIDE) >> 16);


	DEBUG (EVENT_LOG_GX, "....  color (%8.8x)", c);

	glColor4ubv ((__u8 *) &c);


	return 1;
}


unsigned int gx_send_ci8_rgba6 (__u32 mem)
{
	__u32 c = color_unpack_rgba6 (MEMR32 (CP_COLOR0_ARRAY_BASE + MEM (mem) * CP_COLOR0_ARRAY_STRIDE) >> 8);


	DEBUG (EVENT_LOG_GX, "....  color (%8.8x)", c);

	glColor4ubv ((__u8 *) &c);

	return 1;
}


unsigned int gx_send_ci8_rgba8 (__u32 mem)
{
	__u32 c = MEM32 (CP_COLOR0_ARRAY_BASE + MEM (mem) * CP_COLOR0_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  color (%8.8x)", c);

	glColor4ubv ((__u8 *) &c);

	return 1;
}


// 16 bit indexed color
unsigned int gx_send_ci16_rgb565 (__u32 mem)
{
	__u32 c = color_unpack_rgb565 (MEMR16 (CP_COLOR0_ARRAY_BASE + MEMR16 (mem) * CP_COLOR0_ARRAY_STRIDE));


	DEBUG (EVENT_LOG_GX, "....  color (%6.6x)", c);

	glColor3ubv ((__u8 *) &c);

	return 2;
}


unsigned int gx_send_ci16_rgb8 (__u32 mem)
{
	__u32 c = MEM32 (CP_COLOR0_ARRAY_BASE + MEMR16 (mem) * CP_COLOR0_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  color (%6.6x)", c);

	glColor3ubv ((__u8 *) &c);

	return 2;
}


unsigned int gx_send_ci16_rgbx8 (__u32 mem)
{
	__u32 c = MEM32 (CP_COLOR0_ARRAY_BASE + MEMR16 (mem) * CP_COLOR0_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  color (%6.6x)", c);

	glColor3ubv ((__u8 *) &c);

	return 2;
}


unsigned int gx_send_ci16_rgba4 (__u32 mem)
{
	__u32 c = color_unpack_rgba4 (MEMR32 (CP_COLOR0_ARRAY_BASE + MEMR16 (mem) * CP_COLOR0_ARRAY_STRIDE) >> 16);


	DEBUG (EVENT_LOG_GX, "....  color (%8.8x)", c);

	glColor4ubv ((__u8 *) &c);


	return 2;
}


unsigned int gx_send_ci16_rgba6 (__u32 mem)
{
	__u32 c = color_unpack_rgba6 (MEMR32 (CP_COLOR0_ARRAY_BASE + MEMR16 (mem) * CP_COLOR0_ARRAY_STRIDE) >> 8);


	DEBUG (EVENT_LOG_GX, "....  color (%8.8x)", c);

	glColor4ubv ((__u8 *) &c);

	return 2;
}


unsigned int gx_send_ci16_rgba8 (__u32 mem)
{
	__u32 c = MEM32 (CP_COLOR0_ARRAY_BASE + MEMR16 (mem) * CP_COLOR0_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  color (%8.8x)", c);

	glColor4ubv ((__u8 *) &c);

	return 2;
}


static unsigned int (*gxcol[]) (__u32 mem) =
{
	NULL,

	gx_send_c_rgb565,
	gx_send_c_rgb8,
	gx_send_c_rgbx8,
	gx_send_c_rgba4, gx_send_c_rgba6, gx_send_c_rgba8, NULL, NULL,

	gx_send_c_rgb565, gx_send_c_rgb8, gx_send_c_rgbx8,
	gx_send_c_rgba4,
	gx_send_c_rgba6,
	gx_send_c_rgba8,
	NULL, NULL,

	gx_send_ci8_rgb565,
	gx_send_ci8_rgb8,
	gx_send_ci8_rgbx8,
	gx_send_ci8_rgba4, gx_send_ci8_rgba6, gx_send_ci8_rgba8, NULL, NULL,

	gx_send_ci8_rgb565, gx_send_ci8_rgb8, gx_send_ci8_rgbx8,
	gx_send_ci8_rgba4,
	gx_send_ci8_rgba6,
	gx_send_ci8_rgba8,
	NULL, NULL,

	gx_send_ci16_rgb565,
	gx_send_ci16_rgb8,
	gx_send_ci16_rgbx8,
	gx_send_ci16_rgba4, gx_send_ci16_rgba6, gx_send_ci16_rgba8, NULL, NULL,

	gx_send_ci16_rgb565, gx_send_ci16_rgb8, gx_send_ci16_rgbx8,
	gx_send_ci16_rgba4,
	gx_send_ci16_rgba6,
	gx_send_ci16_rgba8,
	NULL, NULL,
};


static unsigned int gxcol_size[] =
{
	0,

	2, 3, 4, 2, 3, 4, 0, 0,
	2, 3, 4, 2, 3, 4, 0, 0,

	1, 1, 1, 1, 1, 1, 0, 0,
	1, 1, 1, 1, 1, 1, 0, 0,

	2, 2, 2, 2, 2, 2, 0, 0,
	2, 2, 2, 2, 2, 2, 0, 0,
};


// fake color
unsigned int gx_send_fake_c_rgb565 (__u32 mem)
{
	LOG_FAKE (EVENT_LOG_GX, "....  color~(%6.6x)", color_unpack_rgb565 (MEMR16 (mem)));

	return 2;
}


unsigned int gx_send_fake_c_rgb8 (__u32 mem)
{
	LOG_FAKE (EVENT_LOG_GX, "....  color~(%6.6x)", MEM32 (mem));

	return 3;
}


unsigned int gx_send_fake_c_rgbx8 (__u32 mem)
{
	LOG_FAKE (EVENT_LOG_GX, "....  color~(%6.6x)", MEM32 (mem));

	return 4;
}


unsigned int gx_send_fake_c_rgba4 (__u32 mem)
{
	LOG_FAKE (EVENT_LOG_GX, "....  color~(%8.8x)", color_unpack_rgba4 (MEMR32 (mem) >> 16));

	return 2;
}


unsigned int gx_send_fake_c_rgba6 (__u32 mem)
{
	LOG_FAKE (EVENT_LOG_GX, "....  color~(%8.8x)", color_unpack_rgba6 (MEMR32 (mem) >> 8));

	return 3;
}


unsigned int gx_send_fake_c_rgba8 (__u32 mem)
{
	LOG_FAKE (EVENT_LOG_GX, "....  color~(%8.8x)", MEM32 (mem));

	return 4;
}


// 8 bit indexed color
unsigned int gx_send_fake_ci8_rgb565 (__u32 mem)
{
	LOG_FAKE (EVENT_LOG_GX, "....  color~(%6.6x)", color_unpack_rgb565 (MEMR16 (CP_COLOR0_ARRAY_BASE + MEM (mem) * CP_COLOR0_ARRAY_STRIDE)));

	return 1;
}


unsigned int gx_send_fake_ci8_rgb8 (__u32 mem)
{
	LOG_FAKE (EVENT_LOG_GX, "....  color~(%6.6x)", MEM32 (CP_COLOR0_ARRAY_BASE + MEM (mem) * CP_COLOR0_ARRAY_STRIDE));

	return 1;
}


unsigned int gx_send_fake_ci8_rgbx8 (__u32 mem)
{
	LOG_FAKE (EVENT_LOG_GX, "....  color~(%6.6x)", MEM32 (CP_COLOR0_ARRAY_BASE + MEM (mem) * CP_COLOR0_ARRAY_STRIDE));

	return 1;
}


unsigned int gx_send_fake_ci8_rgba4 (__u32 mem)
{
	LOG_FAKE (EVENT_LOG_GX, "....  color~(%8.8x)", color_unpack_rgba4 (MEMR32 (CP_COLOR0_ARRAY_BASE + MEM (mem) * CP_COLOR0_ARRAY_STRIDE) >> 16));

	return 1;
}


unsigned int gx_send_fake_ci8_rgba6 (__u32 mem)
{
	LOG_FAKE (EVENT_LOG_GX, "....  color~(%8.8x)", color_unpack_rgba6 (MEMR32 (CP_COLOR0_ARRAY_BASE + MEM (mem) * CP_COLOR0_ARRAY_STRIDE) >> 8));

	return 1;
}


unsigned int gx_send_fake_ci8_rgba8 (__u32 mem)
{
	LOG_FAKE (EVENT_LOG_GX, "....  color~(%8.8x)", MEM32 (CP_COLOR0_ARRAY_BASE + MEM (mem) * CP_COLOR0_ARRAY_STRIDE));

	return 1;
}


// 16 bit indexed color
unsigned int gx_send_fake_ci16_rgb565 (__u32 mem)
{
	LOG_FAKE (EVENT_LOG_GX, "....  color~(%6.6x)", color_unpack_rgb565 (MEMR16 (CP_COLOR0_ARRAY_BASE + MEMR16 (mem) * CP_COLOR0_ARRAY_STRIDE)));

	return 2;
}


unsigned int gx_send_fake_ci16_rgb8 (__u32 mem)
{
	LOG_FAKE (EVENT_LOG_GX, "....  color~(%6.6x)", MEM32 (CP_COLOR0_ARRAY_BASE + MEMR16 (mem) * CP_COLOR0_ARRAY_STRIDE));

	return 2;
}


unsigned int gx_send_fake_ci16_rgbx8 (__u32 mem)
{
	LOG_FAKE (EVENT_LOG_GX, "....  color~(%6.6x)", MEM32 (CP_COLOR0_ARRAY_BASE + MEMR16 (mem) * CP_COLOR0_ARRAY_STRIDE));

	return 2;
}


unsigned int gx_send_fake_ci16_rgba4 (__u32 mem)
{
	LOG_FAKE (EVENT_LOG_GX, "....  color~(%8.8x)", color_unpack_rgba4 (MEMR32 (CP_COLOR0_ARRAY_BASE + MEMR16 (mem) * CP_COLOR0_ARRAY_STRIDE) >> 16));

	return 2;
}


unsigned int gx_send_fake_ci16_rgba6 (__u32 mem)
{
	LOG_FAKE (EVENT_LOG_GX, "....  color~(%8.8x)", color_unpack_rgba6 (MEMR32 (CP_COLOR0_ARRAY_BASE + MEMR16 (mem) * CP_COLOR0_ARRAY_STRIDE) >> 8));

	return 2;
}


unsigned int gx_send_fake_ci16_rgba8 (__u32 mem)
{
	LOG_FAKE (EVENT_LOG_GX, "....  color~(%8.8x)", MEM32 (CP_COLOR0_ARRAY_BASE + MEMR16 (mem) * CP_COLOR0_ARRAY_STRIDE));

	return 2;
}



static unsigned int (*gxcol_fake[]) (__u32 mem) =
{
	NULL,

	gx_send_fake_c_rgb565,
	gx_send_fake_c_rgb8,
	gx_send_fake_c_rgbx8,
	gx_send_fake_c_rgba4, gx_send_fake_c_rgba6, gx_send_fake_c_rgba8, NULL, NULL,

	gx_send_fake_c_rgb565, gx_send_fake_c_rgb8, gx_send_fake_c_rgbx8,
	gx_send_fake_c_rgba4,
	gx_send_fake_c_rgba6,
	gx_send_fake_c_rgba8,
	NULL, NULL,

	gx_send_fake_ci8_rgb565,
	gx_send_fake_ci8_rgb8,
	gx_send_fake_ci8_rgbx8,
	gx_send_fake_ci8_rgba4, gx_send_fake_ci8_rgba6, gx_send_fake_ci8_rgba8, NULL, NULL,

	gx_send_fake_ci8_rgb565, gx_send_fake_ci8_rgb8, gx_send_fake_ci8_rgbx8,
	gx_send_fake_ci8_rgba4,
	gx_send_fake_ci8_rgba6,
	gx_send_fake_ci8_rgba8,
	NULL, NULL,

	gx_send_fake_ci16_rgb565,
	gx_send_fake_ci16_rgb8,
	gx_send_fake_ci16_rgbx8,
	gx_send_fake_ci16_rgba4, gx_send_fake_ci16_rgba6, gx_send_fake_ci16_rgba8, NULL, NULL,

	gx_send_fake_ci16_rgb565, gx_send_fake_ci16_rgb8, gx_send_fake_ci16_rgbx8,
	gx_send_fake_ci16_rgba4,
	gx_send_fake_ci16_rgba6,
	gx_send_fake_ci16_rgba8,
	NULL, NULL,
};


// direct texture
unsigned int gx_send_tu8_s (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", MEM (mem));

	glTexCoord1s (MEM (mem));

	return 1;
}


unsigned int gx_send_tu8_st (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", MEM (mem), MEM (mem + 1));

	glTexCoord2s (MEM (mem), MEM (mem + 1));

	return 2;
}


unsigned int gx_send_ts8_s (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", MEMS (mem));

	glTexCoord1s (MEMS (mem));

	return 1;
}


unsigned int gx_send_ts8_st (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", MEMS (mem), MEMS (mem + 1));

	glTexCoord2s (MEMS (mem), MEMS (mem + 1));

	return 2;
}


unsigned int gx_send_tu16_s (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", MEMR16 (mem));

	glTexCoord1i (MEMR16 (mem));

	return 2;
}


unsigned int gx_send_tu16_st (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", MEMR16 (mem), MEMR16 (mem + 2));

	glTexCoord2i (MEMR16 (mem), MEMR16 (mem + 2));

	return 4;
}


unsigned int gx_send_ts16_s (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", MEMSR16 (mem));

	glTexCoord1s (MEMSR16 (mem));

	return 2;
}


unsigned int gx_send_ts16_st (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", MEMSR16 (mem), MEMSR16 (mem + 2));

	glTexCoord2s (MEMSR16 (mem), MEMSR16 (mem + 2));

	return 4;
}


unsigned int gx_send_tf_s (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", MEMRF (mem));

	glTexCoord1f (MEMRF (mem));

	return 4;
}


unsigned int gx_send_tf_st (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", MEMRF (mem), MEMRF (mem + 4));

	glTexCoord2f (MEMRF (mem), MEMRF (mem + 4));

	return 8;
}


// 8 bit indexed texture
unsigned int gx_send_ti8u8_s (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", p[0]);

	glTexCoord1s (p[0]);

	return 1;
}


unsigned int gx_send_ti8u8_st (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", p[0], p[1]);

	glTexCoord2s (p[0], p[1]);

	return 1;
}


unsigned int gx_send_ti8s8_s (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", p[0]);

	glTexCoord1s (p[0]);

	return 1;
}


unsigned int gx_send_ti8s8_st (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", p[0], p[1]);

	glTexCoord2s (p[0], p[1]);

	return 1;
}


unsigned int gx_send_ti8u16_s (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", BSWAP16 (p[0]));

	glTexCoord1i (BSWAP16 (p[0]));

	return 1;
}


unsigned int gx_send_ti8u16_st (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", BSWAP16 (p[0]), BSWAP16 (p[1]));

	glTexCoord2i (BSWAP16 (p[0]), BSWAP16 (p[1]));

	return 1;
}


unsigned int gx_send_ti8s16_s (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", BSWAPS16 (p[0]));

	glTexCoord1i (BSWAPS16 (p[0]));

	return 1;
}


unsigned int gx_send_ti8s16_st (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", BSWAPS16 (p[0]), BSWAPS16 (p[1]));

	glTexCoord2i (BSWAPS16 (p[0]), BSWAPS16 (p[1]));

	return 1;
}


unsigned int gx_send_ti8f_s (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", BSWAPF (p[0]));

	glTexCoord1f (BSWAPF (p[0]));

	return 1;
}


unsigned int gx_send_ti8f_st (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", BSWAPF (p[0]), BSWAPF (p[1]));

	glTexCoord2f (BSWAPF (p[0]), BSWAPF (p[1]));

	return 1;
}


// 16 bit indexed texture
unsigned int gx_send_ti16u8_s (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", p[0]);

	glTexCoord1s (p[0]);

	return 2;
}


unsigned int gx_send_ti16u8_st (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", p[0], p[1]);

	glTexCoord2s (p[0], p[1]);

	return 2;
}


unsigned int gx_send_ti16s8_s (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", p[0]);

	glTexCoord1s (p[0]);

	return 2;
}


unsigned int gx_send_ti16s8_st (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", p[0], p[1]);

	glTexCoord2s (p[0], p[1]);

	return 2;
}


unsigned int gx_send_ti16u16_s (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", BSWAP16 (p[0]));

	glTexCoord1i (BSWAP16 (p[0]));

	return 2;
}


unsigned int gx_send_ti16u16_st (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", BSWAP16 (p[0]), BSWAP16 (p[1]));

	glTexCoord2i (BSWAP16 (p[0]), BSWAP16 (p[1]));

	return 2;
}


unsigned int gx_send_ti16s16_s (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", BSWAPS16 (p[0]));

	glTexCoord1i (BSWAPS16 (p[0]));

	return 2;
}


unsigned int gx_send_ti16s16_st (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", BSWAPS16 (p[0]), BSWAPS16 (p[1]));

	glTexCoord2i (BSWAPS16 (p[0]), BSWAPS16 (p[1]));

	return 2;
}


unsigned int gx_send_ti16f_s (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", BSWAPF (p[0]));

	glTexCoord1f (BSWAPF (p[0]));

	return 2;
}


unsigned int gx_send_ti16f_st (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", BSWAPF (p[0]), BSWAPF (p[1]));

	glTexCoord2f (BSWAPF (p[0]), BSWAPF (p[1]));

	return 2;
}


static unsigned int (*gxtex[]) (__u32 mem) =
{
	NULL,

	gx_send_tu8_s,
	gx_send_ts8_s,
	gx_send_tu16_s,
	gx_send_ts16_s,
	gx_send_tf_s,
	NULL, NULL, NULL,

	gx_send_tu8_st,
	gx_send_ts8_st,
	gx_send_tu16_st,
	gx_send_ts16_st,
	gx_send_tf_st,
	NULL, NULL, NULL,

	gx_send_ti8u8_s,
	gx_send_ti8s8_s,
	gx_send_ti8u16_s,
	gx_send_ti8s16_s,
	gx_send_ti8f_s,
	NULL, NULL, NULL,

	gx_send_ti8u8_st,
	gx_send_ti8s8_st,
	gx_send_ti8u16_st,
	gx_send_ti8s16_st,
	gx_send_ti8f_st,
	NULL, NULL, NULL,

	gx_send_ti16u8_s,
	gx_send_ti16s8_s,
	gx_send_ti16u16_s,
	gx_send_ti16s16_s,
	gx_send_ti16f_s,
	NULL, NULL, NULL,

	gx_send_ti16u8_st,
	gx_send_ti16s8_st,
	gx_send_ti16u16_st,
	gx_send_ti16s16_st,
	gx_send_ti16f_st,
	NULL, NULL, NULL,
};


static unsigned int gxtex_size[] =
{
	0,

	1, 1, 2, 2, 4, 0, 0, 0,
	2, 2, 4, 4, 8, 0, 0, 0,

	1, 1, 1, 1, 1, 0, 0, 0,
	1, 1, 1, 1, 1, 0, 0, 0,

	2, 2, 2, 2, 2, 0, 0, 0,
	2, 2, 2, 2, 2, 0, 0, 0,
};


inline void transform_texcoord_s (float d[1], float v[1], int index)
{
	float *m = &XFMAT_TEX (index);


	d[0] = m[0]*v[0] + m[ 3];
}


inline void transform_texcoord_st (float d[2], float v[2], int index)
{
	float *m = &XFMAT_TEX (index);


	d[0] = m[0]*v[0] + m[1]*v[1] + m[ 3];
	d[1] = m[4]*v[0] + m[5]*v[1] + m[ 7];
}


inline void transform_texcoord_stq (float d[3], float v[3], int index)
{
	float *m = &XFMAT_TEX (index);


	d[0] = m[0]*v[0] + m[1]*v[1] + m[ 2]*v[2] + m[ 3];
	d[1] = m[4]*v[0] + m[5]*v[1] + m[ 6]*v[2] + m[ 7];
	d[2] = m[8]*v[0] + m[9]*v[1] + m[10]*v[2] + m[11];
}


inline void transform_texcoord_stq_dual (float d[3], float v[3], int index)
{
	float *m = &XFMAT_DUALTEX (index);


	d[0] = m[0]*v[0] + m[1]*v[1] + m[ 2]*v[2] + m[ 3];
	d[1] = m[4]*v[0] + m[5]*v[1] + m[ 6]*v[2] + m[ 7];
	d[2] = m[8]*v[0] + m[9]*v[1] + m[10]*v[2] + m[11];
}


// direct texture transformed by matrix
unsigned int gx_send_tu8_s_midx (__u32 mem)
{
	float d[1], v[1] =
	{
		tdq[0] * MEM (mem),
	};


	transform_texcoord_s (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", d[0]);

	glTexCoord1fv (d);

	return 1;
}


unsigned int gx_send_tu8_st_midx (__u32 mem)
{
	float d[2], v[2] =
	{
		tdq[0] * MEM (mem), tdq[0] * MEM (mem + 1),
	};


	transform_texcoord_st (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", d[0], d[1]);

	glTexCoord2fv (d);

	return 2;
}


unsigned int gx_send_ts8_s_midx (__u32 mem)
{
	float d[1], v[1] =
	{
		tdq[0] * MEMS (mem),
	};


	transform_texcoord_s (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", d[0]);

	glTexCoord1fv (d);

	return 1;
}


unsigned int gx_send_ts8_st_midx (__u32 mem)
{
	float d[2], v[2] =
	{
		tdq[0] * MEMS (mem), tdq[0] * MEMS (mem + 1),
	};


	transform_texcoord_st (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", d[0], d[1]);

	glTexCoord2fv (d);

	return 2;
}


unsigned int gx_send_tu16_s_midx (__u32 mem)
{
	float d[1], v[1] =
	{
		tdq[0] * MEMR16 (mem),
	};


	transform_texcoord_s (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", d[0]);

	glTexCoord1fv (d);

	return 2;
}


unsigned int gx_send_tu16_st_midx (__u32 mem)
{
	float d[2], v[2] =
	{
		tdq[0] * MEMR16 (mem), tdq[0] * MEMR16 (mem + 2),
	};


	transform_texcoord_st (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", d[0], d[1]);

	glTexCoord2fv (d);

	return 4;
}


unsigned int gx_send_ts16_s_midx (__u32 mem)
{
	float d[1], v[1] =
	{
		tdq[0] * MEMSR16 (mem),
	};


	transform_texcoord_s (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", d[0]);

	glTexCoord1fv (d);

	return 2;
}


unsigned int gx_send_ts16_st_midx (__u32 mem)
{
	float d[2], v[2] =
	{
		tdq[0] * MEMSR16 (mem), tdq[0] * MEMSR16 (mem + 2),
	};


	transform_texcoord_st (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", d[0], d[1]);

	glTexCoord2fv (d);

	return 4;
}


unsigned int gx_send_tf_s_midx (__u32 mem)
{
	float d[1], v[1] =
	{
		tdq[0] * MEMRF (mem),
	};


	transform_texcoord_s (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", d[0]);

	glTexCoord1fv (d);

	return 4;
}


unsigned int gx_send_tf_st_midx (__u32 mem)
{
	float d[2], v[2] =
	{
		tdq[0] * MEMRF (mem), tdq[0] * MEMRF (mem + 4),
	};


	transform_texcoord_st (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", d[0], d[1]);

	glTexCoord2fv (d);

	return 8;
}


// 8 bit indexed texture
unsigned int gx_send_ti8u8_s_midx (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));
	float d[1], v[1] =
	{
		tdq[0] * p[0],
	};


	transform_texcoord_s (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", d[0]);

	glTexCoord1fv (d);

	return 1;
}


unsigned int gx_send_ti8u8_st_midx (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));
	float d[2], v[2] =
	{
		tdq[0] * p[0], tdq[0] * p[1],
	};


	transform_texcoord_st (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", d[0], d[1]);

	glTexCoord2fv (d);

	return 1;
}


unsigned int gx_send_ti8s8_s_midx (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));
	float d[1], v[1] =
	{
		tdq[0] * p[0],
	};


	transform_texcoord_s (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", d[0]);

	glTexCoord1fv (d);

	return 1;
}


unsigned int gx_send_ti8s8_st_midx (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));
	float d[2], v[2] =
	{
		tdq[0] * p[0], tdq[0] * p[1],
	};


	transform_texcoord_st (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", d[0], d[1]);

	glTexCoord2fv (d);

	return 1;
}


unsigned int gx_send_ti8u16_s_midx (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));
	float d[1], v[1] =
	{
		tdq[0] * BSWAP16 (p[0]),
	};


	transform_texcoord_s (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", d[0]);

	glTexCoord1fv (d);

	return 1;
}


unsigned int gx_send_ti8u16_st_midx (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));
	float d[2], v[2] =
	{
		tdq[0] * BSWAP16 (p[0]), tdq[0] * BSWAP16 (p[1]),
	};


	transform_texcoord_st (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", d[0], d[1]);

	glTexCoord2fv (d);

	return 1;
}


unsigned int gx_send_ti8s16_s_midx (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));
	float d[1], v[1] =
	{
		tdq[0] * BSWAPS16 (p[0]),
	};


	transform_texcoord_s (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", d[0]);

	glTexCoord1fv (d);

	return 1;
}


unsigned int gx_send_ti8s16_st_midx (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));
	float d[2], v[2] =
	{
		tdq[0] * BSWAPS16 (p[0]), tdq[0] * BSWAPS16 (p[1]),
	};


	transform_texcoord_st (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", d[0], d[1]);

	glTexCoord2fv (d);

	return 1;
}


unsigned int gx_send_ti8f_s_midx (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));
	float d[1], v[1] =
	{
		tdq[0] * BSWAPF (p[0]),
	};


	transform_texcoord_s (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", d[0]);

	glTexCoord1fv (d);

	return 1;
}


unsigned int gx_send_ti8f_st_midx (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));
	float d[2], v[2] =
	{
		tdq[0] * BSWAPF (p[0]), tdq[0] * BSWAPF (p[1]),
	};


	transform_texcoord_st (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", d[0], d[1]);

	glTexCoord2fv (d);

	return 1;
}


// 16 bit indexed texture
unsigned int gx_send_ti16u8_s_midx (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));
	float d[1], v[1] =
	{
		tdq[0] * p[0],
	};


	transform_texcoord_s (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", d[0]);

	glTexCoord1fv (d);

	return 2;
}


unsigned int gx_send_ti16u8_st_midx (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));
	float d[2], v[2] =
	{
		tdq[0] * p[0], tdq[0] * p[1],
	};


	transform_texcoord_st (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", d[0], d[1]);

	glTexCoord2fv (d);

	return 2;
}


unsigned int gx_send_ti16s8_s_midx (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));
	float d[1], v[1] =
	{
		tdq[0] * p[0],
	};


	transform_texcoord_s (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", d[0]);

	glTexCoord1fv (d);

	return 2;
}


unsigned int gx_send_ti16s8_st_midx (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));
	float d[2], v[2] =
	{
		tdq[0] * p[0], tdq[0] * p[1],
	};


	transform_texcoord_st (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", d[0], d[1]);

	glTexCoord2fv (d);

	return 2;
}


unsigned int gx_send_ti16u16_s_midx (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));
	float d[1], v[1] =
	{
		tdq[0] * BSWAP16 (p[0]),
	};


	transform_texcoord_s (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", d[0]);

	glTexCoord1fv (d);

	return 2;
}


unsigned int gx_send_ti16u16_st_midx (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));
	float d[2], v[2] =
	{
		tdq[0] * BSWAP16 (p[0]), tdq[0] * BSWAP16 (p[1]),
	};


	transform_texcoord_st (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", d[0], d[1]);

	glTexCoord2fv (d);

	return 2;
}


unsigned int gx_send_ti16s16_s_midx (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));
	float d[1], v[1] =
	{
		tdq[0] * BSWAPS16 (p[0]),
	};


	transform_texcoord_s (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", d[0]);

	glTexCoord1fv (d);

	return 2;
}


unsigned int gx_send_ti16s16_st_midx (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));
	float d[2], v[2] =
	{
		tdq[0] * BSWAPS16 (p[0]), tdq[0] * BSWAPS16 (p[1]),
	};


	transform_texcoord_st (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", d[0], d[1]);

	glTexCoord2fv (d);

	return 2;
}


unsigned int gx_send_ti16f_s_midx (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));
	float d[1], v[1] =
	{
		tdq[0] * BSWAPF (p[0]),
	};


	transform_texcoord_s (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", d[0]);

	glTexCoord1fv (d);

	return 2;
}


unsigned int gx_send_ti16f_st_midx (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));
	float d[2], v[2] =
	{
		tdq[0] * BSWAPF (p[0]), tdq[0] * BSWAPF (p[1]),
	};


	transform_texcoord_st (d, v, tex_matrix_index[0]);
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", d[0], d[1]);

	glTexCoord2fv (d);

	return 2;
}


static unsigned int (*gxtex_midx[]) (__u32 mem) =
{
	NULL,

	gx_send_tu8_s_midx,
	gx_send_ts8_s_midx,
	gx_send_tu16_s_midx,
	gx_send_ts16_s_midx,
	gx_send_tf_s_midx,
	NULL, NULL, NULL,

	gx_send_tu8_st_midx,
	gx_send_ts8_st_midx,
	gx_send_tu16_st_midx,
	gx_send_ts16_st_midx,
	gx_send_tf_st_midx,
	NULL, NULL, NULL,

	gx_send_ti8u8_s_midx,
	gx_send_ti8s8_s_midx,
	gx_send_ti8u16_s_midx,
	gx_send_ti8s16_s_midx,
	gx_send_ti8f_s_midx,
	NULL, NULL, NULL,

	gx_send_ti8u8_st_midx,
	gx_send_ti8s8_st_midx,
	gx_send_ti8u16_st_midx,
	gx_send_ti8s16_st_midx,
	gx_send_ti8f_st_midx,
	NULL, NULL, NULL,

	gx_send_ti16u8_s_midx,
	gx_send_ti16s8_s_midx,
	gx_send_ti16u16_s_midx,
	gx_send_ti16s16_s_midx,
	gx_send_ti16f_s_midx,
	NULL, NULL, NULL,

	gx_send_ti16u8_st_midx,
	gx_send_ti16s8_st_midx,
	gx_send_ti16u16_st_midx,
	gx_send_ti16s16_st_midx,
	gx_send_ti16f_st_midx,
	NULL, NULL, NULL,
};


// fake texture
unsigned int gx_send_fake_tu8_s (__u32 mem)
{
	return 1;
}


unsigned int gx_send_fake_tu8_st (__u32 mem)
{
	return 2;
}


unsigned int gx_send_fake_ts8_s (__u32 mem)
{
	return 1;
}


unsigned int gx_send_fake_ts8_st (__u32 mem)
{
	return 2;
}


unsigned int gx_send_fake_tu16_s (__u32 mem)
{
	return 2;
}


unsigned int gx_send_fake_tu16_st (__u32 mem)
{
	return 4;
}


unsigned int gx_send_fake_ts16_s (__u32 mem)
{
	return 2;
}


unsigned int gx_send_fake_ts16_st (__u32 mem)
{
	return 4;
}


unsigned int gx_send_fake_tf_s (__u32 mem)
{
	return 4;
}


unsigned int gx_send_fake_tf_st (__u32 mem)
{
	return 8;
}


// 8 bit indexed texture
unsigned int gx_send_fake_ti8u8_s (__u32 mem)
{
	return 1;
}


unsigned int gx_send_fake_ti8u8_st (__u32 mem)
{
	return 1;
}


unsigned int gx_send_fake_ti8s8_s (__u32 mem)
{
	return 1;
}


unsigned int gx_send_fake_ti8s8_st (__u32 mem)
{
	return 1;
}


unsigned int gx_send_fake_ti8u16_s (__u32 mem)
{
	return 1;
}


unsigned int gx_send_fake_ti8u16_st (__u32 mem)
{
	return 1;
}


unsigned int gx_send_fake_ti8s16_s (__u32 mem)
{
	return 1;
}


unsigned int gx_send_fake_ti8s16_st (__u32 mem)
{
	return 1;
}


unsigned int gx_send_fake_ti8f_s (__u32 mem)
{
	return 1;
}


unsigned int gx_send_fake_ti8f_st (__u32 mem)
{
	return 1;
}


// 16 bit indexed texture
unsigned int gx_send_fake_ti16u8_s (__u32 mem)
{
	return 2;
}


unsigned int gx_send_fake_ti16u8_st (__u32 mem)
{
	return 2;
}


unsigned int gx_send_fake_ti16s8_s (__u32 mem)
{
	return 2;
}


unsigned int gx_send_fake_ti16s8_st (__u32 mem)
{
	return 2;
}


unsigned int gx_send_fake_ti16u16_s (__u32 mem)
{
	return 2;
}


unsigned int gx_send_fake_ti16u16_st (__u32 mem)
{
	return 2;
}


unsigned int gx_send_fake_ti16s16_s (__u32 mem)
{
	return 2;
}


unsigned int gx_send_fake_ti16s16_st (__u32 mem)
{
	return 2;
}


unsigned int gx_send_fake_ti16f_s (__u32 mem)
{
	return 2;
}


unsigned int gx_send_fake_ti16f_st (__u32 mem)
{
	return 2;
}


static unsigned int (*gxtex_fake[]) (__u32 mem) =
{
	NULL,

	gx_send_fake_tu8_s,
	gx_send_fake_ts8_s,
	gx_send_fake_tu16_s,
	gx_send_fake_ts16_s,
	gx_send_fake_tf_s,
	NULL, NULL, NULL,

	gx_send_fake_tu8_st,
	gx_send_fake_ts8_st,
	gx_send_fake_tu16_st,
	gx_send_fake_ts16_st,
	gx_send_fake_tf_st,
	NULL, NULL, NULL,

	gx_send_fake_ti8u8_s,
	gx_send_fake_ti8s8_s,
	gx_send_fake_ti8u16_s,
	gx_send_fake_ti8s16_s,
	gx_send_fake_ti8f_s,
	NULL, NULL, NULL,

	gx_send_fake_ti8u8_st,
	gx_send_fake_ti8s8_st,
	gx_send_fake_ti8u16_st,
	gx_send_fake_ti8s16_st,
	gx_send_fake_ti8f_st,
	NULL, NULL, NULL,

	gx_send_fake_ti16u8_s,
	gx_send_fake_ti16s8_s,
	gx_send_fake_ti16u16_s,
	gx_send_fake_ti16s16_s,
	gx_send_fake_ti16f_s,
	NULL, NULL, NULL,

	gx_send_fake_ti16u8_st,
	gx_send_fake_ti16s8_st,
	gx_send_fake_ti16u16_st,
	gx_send_fake_ti16s16_st,
	gx_send_fake_ti16f_st,
	NULL, NULL, NULL,
};


// 4x3
void gx_load_geometry_matrix (unsigned int index)
{
	float *xm = &XFMAT_GEO (index), m[16];
	int i;


	// must be transposed
	m[0*4 + 0] = xm[0*4 + 0];
	m[1*4 + 0] = xm[0*4 + 1];
	m[2*4 + 0] = xm[0*4 + 2];
	m[3*4 + 0] = xm[0*4 + 3];

	m[0*4 + 1] = xm[1*4 + 0];
	m[1*4 + 1] = xm[1*4 + 1];
	m[2*4 + 1] = xm[1*4 + 2];
	m[3*4 + 1] = xm[1*4 + 3];

	m[0*4 + 2] = xm[2*4 + 0];
	m[1*4 + 2] = xm[2*4 + 1];
	m[2*4 + 2] = xm[2*4 + 2];
	m[3*4 + 2] = xm[2*4 + 3];

	m[0*4 + 3] = 0;
	m[1*4 + 3] = 0;
	m[2*4 + 3] = 0;
	m[3*4 + 3] = 1;

	DEBUG (EVENT_LOG_GX, "....  loading geometry matrix");
	for (i = 0; i < 3; i++)
		DEBUG (EVENT_LOG_GX, "....  %8.2f   %8.2f   %8.2f   %8.2f", m[0*4 + i], m[1*4 + i], m[2*4 + i], m[3*4 + i]);

	glLoadMatrixf (m);
}


// 4x2
void gx_load_texture_matrix (unsigned int index, int p2)
{
	float *xm = &XFMAT_TEX (index), m[16];
	int i;


	// must be transposed
	m[0*4 + 0] = xm[0*4 + 0];
	m[1*4 + 0] = xm[0*4 + 1];
	m[2*4 + 0] = xm[0*4 + 2];
	m[3*4 + 0] = xm[0*4 + 3];

	m[0*4 + 1] = xm[1*4 + 0];
	m[1*4 + 1] = xm[1*4 + 1];
	m[2*4 + 1] = xm[1*4 + 2];
	m[3*4 + 1] = xm[1*4 + 3];

	m[0*4 + 2] = 0;
	m[1*4 + 2] = 0;
	m[2*4 + 2] = 1;
	m[3*4 + 2] = 0;

	m[0*4 + 3] = 0;
	m[1*4 + 3] = 0;
	m[2*4 + 3] = 0;
	m[3*4 + 3] = 1;

	if (!p2)
	{
		m[12] *= texactive[0]->width;
		m[13] *= texactive[0]->height;
	}

	DEBUG (EVENT_LOG_GX, "....  loading texture matrix");
	for (i = 0; i < 2; i++)
		DEBUG (EVENT_LOG_GX, "....  %8.2f   %8.2f   %8.2f   %8.2f", m[0*4 + i], m[1*4 + i], m[2*4 + i], m[3*4 + i]);

	glLoadMatrixf (m);
}


unsigned int gx_draw_old (__u32 mem, int prim, int n, int vat)
{
	unsigned int (*midx[16]) (__u32 mem);
	unsigned int (*vdata[16]) (__u32 mem);
	unsigned int (*pos) (__u32 mem);
	int nm = 0, nv = 0, psize, i, j;
	__u32 start = mem, mp = mem, list_size = 0, color;


	if (!n)
		return 0;

	// matrix indices
	if (VCD_MIDX)
	{
		if (VCD_PMIDX)
			midx[nm++] = gx_send_pmidx;

		if (VCD_T0MIDX)
			midx[nm++] = gx_send_t0midx;

		if (VCD_T1MIDX)
			midx[nm++] = gx_send_t1midx;
		if (VCD_T2MIDX)
			midx[nm++] = gx_send_t2midx;
		if (VCD_T3MIDX)
			midx[nm++] = gx_send_t3midx;
		if (VCD_T4MIDX)
			midx[nm++] = gx_send_t4midx;
		if (VCD_T5MIDX)
			midx[nm++] = gx_send_t5midx;
		if (VCD_T6MIDX)
			midx[nm++] = gx_send_t6midx;
		if (VCD_T7MIDX)
			midx[nm++] = gx_send_t7midx;
	}

	list_size += nm;
	list_size += gxnormal_size[GXIDX (NORMAL, vat)];
	list_size += gxcol_size[GXIDX (COL0, vat)];
	list_size += gxcol_size[GXIDX (COL1, vat)];
	list_size += gxtex_size[GXIDX (TEX0, vat)];
	list_size += gxtex_size[GXIDX (TEX1, vat)];
	list_size += gxtex_size[GXIDX (TEX2, vat)];
	list_size += gxtex_size[GXIDX (TEX3, vat)];
	list_size += gxtex_size[GXIDX (TEX4, vat)];
	list_size += gxtex_size[GXIDX (TEX5, vat)];
	list_size += gxtex_size[GXIDX (TEX6, vat)];
	list_size += gxtex_size[GXIDX (TEX7, vat)];
	list_size += gxpos_size[GXIDX (POS, vat)];
	if (EXCEEDES_LIST_BOUNDARY (start, list_size * n + 3))
	// draw commands take 3 bytes and return 3 + gx_draw
		return -(list_size * n + 6);

	if (CP_MATRIX_INDEX_0 != XF_MATRIX_INDEX_0)
		DEBUG (EVENT_STOP, ".gx:  CP_MATRIX_INDEX_0 is different than XF_MATRIX_INDEX_0");

	DEBUG (EVENT_LOG_GX, "....  VCD MIDX %2.2x POS %d NORMAL %d COL %d|%d TEX %d|%d|%d|%d|%d|%d|%d|%d",
											 VCD_MIDX, VCD_POS, VCD_NORMAL, VCD_COL0, VCD_COL1,
											 VCD_TEX0, VCD_TEX1, VCD_TEX2, VCD_TEX3, VCD_TEX4, VCD_TEX5, VCD_TEX6, VCD_TEX7);

	DEBUG (EVENT_LOG_GX, "....  VAT DQ %d POS %d|%d|%d NORMAL %d|%d COL0 %d|%d COL1 %d|%d",
											 VAT_BYTE_DEQUANT (vat),
											 VAT_POS_CNT (vat), VAT_POS_FMT (vat), VAT_POS_SHFT (vat),
											 VAT_NORMAL_CNT (vat), VAT_NORMAL_FMT (vat),
											 VAT_COL0_CNT (vat), VAT_COL0_FMT (vat), VAT_COL1_CNT (vat), VAT_COL1_FMT (vat));

	DEBUG (EVENT_LOG_GX, "....      TEX 0 %d|%d|%d 1 %d|%d|%d 2 %d|%d|%d 3 %d|%d|%d 4 %d|%d|%d 5 %d|%d|%d 6 %d|%d|%d 7 %d|%d|%d",
											 VAT_TEX0_CNT (vat), VAT_TEX0_FMT (vat), VAT_TEX0_SHFT (vat),
											 VAT_TEX1_CNT (vat), VAT_TEX1_FMT (vat), VAT_TEX1_SHFT (vat),
											 VAT_TEX2_CNT (vat), VAT_TEX2_FMT (vat), VAT_TEX2_SHFT (vat),
											 VAT_TEX3_CNT (vat), VAT_TEX3_FMT (vat), VAT_TEX3_SHFT (vat),
											 VAT_TEX4_CNT (vat), VAT_TEX4_FMT (vat), VAT_TEX4_SHFT (vat),
											 VAT_TEX5_CNT (vat), VAT_TEX5_FMT (vat), VAT_TEX5_SHFT (vat),
											 VAT_TEX6_CNT (vat), VAT_TEX6_FMT (vat), VAT_TEX6_SHFT (vat),
											 VAT_TEX7_CNT (vat), VAT_TEX7_FMT (vat), VAT_TEX7_SHFT (vat));

	DEBUG (EVENT_LOG_GX, "....  ARRAY POS %.8x|%d COL %.8x|%d TEX %.8x|%d",
											 CP_VERTEX_ARRAY_BASE | 0x80000000, CP_VERTEX_ARRAY_STRIDE,
											 CP_COLOR0_ARRAY_BASE | 0x80000000, CP_COLOR0_ARRAY_STRIDE,
											 CP_TEXTURE_ARRAY_BASE (0) | 0x80000000, CP_TEXTURE_ARRAY_STRIDE (0));

	DEBUG (EVENT_LOG_GX, "....  HOST COLORS %d NORMALS %d TEXTURES %d",
											 HOST_COLORS, HOST_NORMALS, HOST_TEXTURES);

	if (gxswitches.dont_draw)
	{
		gcube_perf_vertices (n);
		return list_size * n;
	}

	// normal (faked)
	vdata[nv] = gxnormal[GXIDX (NORMAL, vat)];
	if (vdata[nv])
		nv++;
	
	if (gxswitches.wireframe || gxswitches.tex_transparent)
	{
		// color 0
		vdata[nv] = gxcol_fake[GXIDX (COL0, vat)];
		if (vdata[nv])
			nv++;
	}
	else
	{
		// color 0
		vdata[nv] = gxcol[GXIDX (COL0, vat)];
		if (vdata[nv])
			nv++;
	}

	// color 1 (faked)
	vdata[nv] = gxcol_fake[GXIDX (COL1, vat)];
	if (vdata[nv])
		nv++;

	if (HOST_TEXTURES)
	{
		if (gxswitches.wireframe)
		{
			// texture 0
			vdata[nv] = gxtex_fake[GXIDX (TEX0, vat)];
			if (vdata[nv])
				nv++;
		}
		else
		{
			// texture 0
			if (VCD_T0MIDX)
				vdata[nv] = gxtex_midx[GXIDX (TEX0, vat)];
			else
				vdata[nv] = gxtex[GXIDX (TEX0, vat)];
			if (vdata[nv])
				nv++;
		}

		// other textures are faked
		// texture 1
		vdata[nv] = gxtex_fake[GXIDX (TEX1, vat)];
		if (vdata[nv])
			nv++;

		// texture 2
		vdata[nv] = gxtex_fake[GXIDX (TEX2, vat)];
		if (vdata[nv])
			nv++;

		// texture 3
		vdata[nv] = gxtex_fake[GXIDX (TEX3, vat)];
		if (vdata[nv])
			nv++;

		// texture 4
		vdata[nv] = gxtex_fake[GXIDX (TEX4, vat)];
		if (vdata[nv])
			nv++;

		// texture 5
		vdata[nv] = gxtex_fake[GXIDX (TEX5, vat)];
		if (vdata[nv])
			nv++;

		// texture 6
		vdata[nv] = gxtex_fake[GXIDX (TEX6, vat)];
		if (vdata[nv])
			nv++;

		// texture 7
		vdata[nv] = gxtex_fake[GXIDX (TEX7, vat)];
		if (vdata[nv])
			nv++;

		if (gxswitches.wireframe)
			gx_enable_texture (0, 0);
		else
			gx_load_texture (0);

		if (texactive[0])
		{
			DEBUG (EVENT_LOG_GX, "....  TEXTURE %.8x %dx%d",
						 texactive[0]->address, texactive[0]->width, texactive[0]->height);

			glMatrixMode (GL_TEXTURE);
	
			// dequantize
			tdq[0] = 1.0 / (1 << VAT_TEX0_SHFT (vat));
			if (VCD_T0MIDX)
			{
				glLoadIdentity ();
				if (!texactive[0]->p2)
					glScalef (TEX_SSIZE (0), TEX_TSIZE (0), 1);
			}
			else
			{
				gx_load_texture_matrix (MIDX_TEX0, texactive[0]->p2);

				if (texactive[0]->p2)
					glScalef (tdq[0], tdq[0], tdq[0]);
				else
					glScalef (tdq[0] * TEX_SSIZE (0), tdq[0] * TEX_TSIZE (0), tdq[0]);
			}

			if (texactive[0] == &tag_render_target)
			{
				// need to flip the render target
				if (tag_render_target.xfb_mipmap)
				{
					glTranslatef (0.5 / tdq[0] , 0.5 / tdq[0], 0);
					glScalef (2, -2, 1);
					glTranslatef (-0.5 * 0.5 / tdq[0] , -0.5 * 1.5 / tdq[0], 0);
				}
				else
				{
					glTranslatef (0.5 / tdq[0] , 0.5 / tdq[0], 0);
					glScalef (1, -1, 1);
					glTranslatef (-0.5 / tdq[0] , -0.5 / tdq[0], 0);
				}
			}

			glMatrixMode (GL_MODELVIEW);
		}
		else
			gx_enable_texture (0, 0);
	}
	else if (texactive[0])
		gx_enable_texture (0, 0);

	// position
	i = GXIDX (POS, vat);
	psize = gxpos_size[i];
	pos = gxpos[i];

	// load geometry matrix
	vdq = 1.0 / (1 << VAT_POS_SHFT (vat));
	if (VCD_PMIDX)
	{
		pos = gxpos_midx[GXIDX (POS, vat)];
		glLoadIdentity ();
	}
	else
	{
		gx_load_geometry_matrix (MIDX_GEO);
		glScalef (vdq, vdq, vdq);
	}

	if (gxswitches.wireframe)
		glColor4f (0, 0, 0, 1);
	else
	{
		if (gxswitches.tex_transparent)
			glColor4f (1, 1, 1, 0.5);
		else if (gxswitches.sw_fullbright)
			glColor4f (1, 1, 1, 1);
		else
		{
			if ((XF (0x1009)))
			{
				color = BSWAP32 (XF_MATERIAL0);

				if (XF (0x100e) & 1)
					color |= 0xffffff00;
			
				if (XF (0x1010) & 1)
					color |= 0xff;

				glColor4ubv ((unsigned char *) &color);
			}
			else	
				glColor4f (1, 1, 1, 1);
		}
	}

	glBegin (prim);

		for (j = 0; j < n; j++)
		{
			// matrix indices
			for (i = 0; i < nm; i++)
				mem += midx[i] (mem);

			// position must be sent last
			mp = mem;
			mem += psize;

			// normals, colors and texture coords
			for (i = 0; i < nv; i++)
				mem += vdata[i] (mem);

			// position
			pos (mp);
		}

	glEnd ();

	gcube_perf_vertices (n);

	if (list_size * n != (mem - start))
		DEBUG (EVENT_STOP, ".gx: incorrect calculation of list_size");

	return (mem - start);
}




// below this point everything belongs to the new engine
// only first texture stage is implemented.

typedef struct
{
	// current vertex data
	float pos[3];
	float tex[8][3];
	float color[2][4];
	float normal[3][3];
} GXTFVertex;

GXTFVertex varray[0x0fffff];
int cvertex;
GXTFVertex *cv;

float material[2][4], ambient[2][4];
int ncolors, ntextures;


#define GX_TEX_COORD_1(S)				({cv->tex[0][0] = S; cv->tex[0][1] = 0;})
#define GX_TEX_COORD_2(S,T)			({cv->tex[0][0] = S; cv->tex[0][1] = T;})
#define GX_POS_COORD_2(X,Y)			({cv->pos[0] = X; cv->pos[1] = Y; cv->pos[2] = 0;})
#define GX_POS_COORD_3(X,Y,Z)		({cv->pos[0] = X; cv->pos[1] = Y; cv->pos[2] = Z;})
#define GX_COLOR0_3V(V)					({cv->color[0][0] = (float) (V)[0] / 0xff; cv->color[0][1] = (float) (V)[1] / 0xff; cv->color[0][2] = (float) (V)[2] / 0xff; cv->color[0][3] = 1;})
#define GX_COLOR0_4V(V)					({cv->color[0][0] = (float) (V)[0] / 0xff; cv->color[0][1] = (float) (V)[1] / 0xff; cv->color[0][2] = (float) (V)[2] / 0xff; cv->color[0][3] = (float) (V)[3] / 0xff;})
#define GX_COLOR1_3V(V)					({cv->color[1][0] = (float) (V)[0] / 0xff; cv->color[1][1] = (float) (V)[1] / 0xff; cv->color[1][2] = (float) (V)[2] / 0xff; cv->color[1][3] = 1;})
#define GX_COLOR1_4V(V)					({cv->color[1][0] = (float) (V)[0] / 0xff; cv->color[1][1] = (float) (V)[1] / 0xff; cv->color[1][2] = (float) (V)[2] / 0xff; cv->color[1][3] = (float) (V)[3] / 0xff;})
#define GX_NORMAL(X,Y,Z)				({cv->normal[0][0] = X; cv->normal[0][1] = Y; cv->normal[0][2] = Z;})
#define GX_NORMAL_V(V)					({cv->normal[0][0] = (V)[0]; cv->normal[0][1] = (V)[1]; cv->normal[0][2] = (V)[2];})
#define GX_NORMAL_N(X,Y,Z)			(GX_NORMAL (X,Y,Z))
#define GX_NORMAL_B(X,Y,Z)			({cv->normal[1][0] = X; cv->normal[1][1] = Y; cv->normal[1][2] = Z;})
#define GX_NORMAL_T(X,Y,Z)			({cv->normal[2][0] = X; cv->normal[2][1] = Y; cv->normal[2][2] = Z;})
#define GX_NORMAL_N_V(V)				(GX_NORMAL_V (V))
#define GX_NORMAL_B_V(V)				({cv->normal[1][0] = (V)[0]; cv->normal[1][1] = (V)[1]; cv->normal[1][2] = (V)[2];})
#define GX_NORMAL_T_V(V)				({cv->normal[2][0] = (V)[0]; cv->normal[2][1] = (V)[1]; cv->normal[2][2] = (V)[2];})
#define GX_NORMAL_NBT_V(V)			({GX_NORMAL_N_V (&(V)[0]); GX_NORMAL_B_V (&(V)[3]); GX_NORMAL_T_V (&(V)[6]);})


//////////////////////////////////////////////////////////////////////////////
// direct position (transformed)
/////////////////////////////////////////////////////////////////////////////
unsigned int gx_tf_send_pu8_xy (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (mem);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", p[0], p[1]);

	GX_POS_COORD_2 (p[0], p[1]);

	return 2;
}


unsigned int gx_tf_send_pu8_xyz (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (mem);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", p[0], p[1], p[2]);

	GX_POS_COORD_3 (p[0], p[1], p[2]);

	return 3;
}


unsigned int gx_tf_send_ps8_xy (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (mem);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", p[0], p[1]);

	GX_POS_COORD_2 (p[0], p[1]);

	return 2;
}


unsigned int gx_tf_send_ps8_xyz (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (mem);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", p[0], p[1], p[2]);

	GX_POS_COORD_3 (p[0], p[1], p[2]);

	return 3;
}


unsigned int gx_tf_send_pu16_xy (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", MEMR16 (mem), MEMR16 (mem + 2));

	GX_POS_COORD_2 (MEMR16 (mem), MEMR16 (mem + 2));

	return 4;
}


unsigned int gx_tf_send_pu16_xyz (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", MEMR16 (mem), MEMR16 (mem + 2), MEMR16 (mem + 4));

	GX_POS_COORD_3 (MEMR16 (mem), MEMR16 (mem + 2), MEMR16 (mem + 4));

	return 6;
}


unsigned int gx_tf_send_ps16_xy (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", MEMSR16 (mem), MEMSR16 (mem + 2));

	GX_POS_COORD_2 (MEMSR16 (mem), MEMSR16 (mem + 2));

	return 4;
}


unsigned int gx_tf_send_ps16_xyz (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", MEMSR16 (mem), MEMSR16 (mem + 2), MEMSR16 (mem + 4));

	GX_POS_COORD_3 (MEMSR16 (mem), MEMSR16 (mem + 2), MEMSR16 (mem + 4));

	return 6;
}


unsigned int gx_tf_send_pf_xy (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  pos   (%.2f, %.2f)", MEMRF (mem), MEMRF (mem + 4));

	GX_POS_COORD_2 (MEMRF (mem), MEMRF (mem + 4));

	return 8;
}


unsigned int gx_tf_send_pf_xyz (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  pos   (%.2f, %.2f, %.2f)", MEMRF (mem), MEMRF (mem + 4), MEMRF (mem + 8));

	GX_POS_COORD_3 (MEMRF (mem), MEMRF (mem + 4), MEMRF (mem + 8));

	return 12;
}


// 8 bit indexed position
unsigned int gx_tf_send_pi8u8_xy (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", p[0], p[1]);

	GX_POS_COORD_2 (p[0], p[1]);

	return 1;
}


unsigned int gx_tf_send_pi8u8_xyz (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", p[0], p[1], p[2]);

	GX_POS_COORD_3 (p[0], p[1], p[2]);

	return 1;
}


unsigned int gx_tf_send_pi8s8_xy (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", p[0], p[1]);

	GX_POS_COORD_2 (p[0], p[1]);

	return 1;
}


unsigned int gx_tf_send_pi8s8_xyz (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", p[0], p[1], p[2]);

	GX_POS_COORD_3 (p[0], p[1], p[2]);

	return 1;
}


unsigned int gx_tf_send_pi8u16_xy (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", BSWAP16 (p[0]), BSWAP16 (p[1]));

	GX_POS_COORD_2 (BSWAP16 (p[0]), BSWAP16 (p[1]));

	return 1;
}


unsigned int gx_tf_send_pi8u16_xyz (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", BSWAP16 (p[0]), BSWAP16 (p[1]), BSWAP16 (p[2]));

	GX_POS_COORD_3 (BSWAP16 (p[0]), BSWAP16 (p[1]), BSWAP16 (p[2]));

	return 1;
}


unsigned int gx_tf_send_pi8s16_xy (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", BSWAPS16 (p[0]), BSWAPS16 (p[1]));

	GX_POS_COORD_2 (BSWAPS16 (p[0]), BSWAPS16 (p[1]));

	return 1;
}


unsigned int gx_tf_send_pi8s16_xyz (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", BSWAPS16 (p[0]), BSWAPS16 (p[1]), BSWAPS16 (p[2]));

	GX_POS_COORD_3 (BSWAPS16 (p[0]), BSWAPS16 (p[1]), BSWAPS16 (p[2]));

	return 1;
}


unsigned int gx_tf_send_pi8f_xy (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%.2f, %.2f)", BSWAPF (p[0]), BSWAPF (p[1]));

	GX_POS_COORD_2 (BSWAPF (p[0]), BSWAPF (p[1]));

	return 1;
}


unsigned int gx_tf_send_pi8f_xyz (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEM (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%.2f, %.2f, %.2f)", BSWAPF (p[0]), BSWAPF (p[1]), BSWAPF (p[2]));

	GX_POS_COORD_3 (BSWAPF (p[0]), BSWAPF (p[1]), BSWAPF (p[2]));

	return 1;
}


// 16 bit indexed position
unsigned int gx_tf_send_pi16u8_xy (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", p[0], p[1]);

	GX_POS_COORD_2 (p[0], p[1]);

	return 2;
}


unsigned int gx_tf_send_pi16u8_xyz (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", p[0], p[1], p[2]);

	GX_POS_COORD_3 (p[0], p[1], p[2]);

	return 2;
}


unsigned int gx_tf_send_pi16s8_xy (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", p[0], p[1]);

	GX_POS_COORD_2 (p[0], p[1]);

	return 2;
}


unsigned int gx_tf_send_pi16s8_xyz (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", p[0], p[1], p[2]);

	GX_POS_COORD_3 (p[0], p[1], p[2]);

	return 2;
}


unsigned int gx_tf_send_pi16u16_xy (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", BSWAP16 (p[0]), BSWAP16 (p[1]));

	GX_POS_COORD_2 (BSWAP16 (p[0]), BSWAP16 (p[1]));

	return 2;
}


unsigned int gx_tf_send_pi16u16_xyz (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", BSWAP16 (p[0]), BSWAP16 (p[1]), BSWAP16 (p[2]));

	GX_POS_COORD_3 (BSWAP16 (p[0]), BSWAP16 (p[1]), BSWAP16 (p[2]));

	return 2;
}


unsigned int gx_tf_send_pi16s16_xy (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d)", BSWAPS16 (p[0]), BSWAPS16 (p[1]));

	GX_POS_COORD_2 (BSWAPS16 (p[0]), BSWAPS16 (p[1]));

	return 2;
}


unsigned int gx_tf_send_pi16s16_xyz (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%d, %d, %d)", BSWAPS16 (p[0]), BSWAPS16 (p[1]), BSWAPS16 (p[2]));

	GX_POS_COORD_3 (BSWAPS16 (p[0]), BSWAPS16 (p[1]), BSWAPS16 (p[2]));

	return 2;
}


unsigned int gx_tf_send_pi16f_xy (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%.2f, %.2f)", BSWAPF (p[0]), BSWAPF (p[1]));

	GX_POS_COORD_2 (BSWAPF (p[0]), BSWAPF (p[1]));

	return 2;
}


unsigned int gx_tf_send_pi16f_xyz (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_VERTEX_ARRAY_BASE + MEMR16 (mem) * CP_VERTEX_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  pos   (%.2f, %.2f, %.2f)", BSWAPF (p[0]), BSWAPF (p[1]), BSWAPF (p[2]));

	GX_POS_COORD_3 (BSWAPF (p[0]), BSWAPF (p[1]), BSWAPF (p[2]));

	return 2;
}


static unsigned int (*gxpos_tf[]) (__u32 mem) =
{
	NULL,

	gx_tf_send_pu8_xy,
	gx_tf_send_ps8_xy,
	gx_tf_send_pu16_xy,
	gx_tf_send_ps16_xy,
	gx_tf_send_pf_xy,
	NULL, NULL, NULL,

	gx_tf_send_pu8_xyz,
	gx_tf_send_ps8_xyz,
	gx_tf_send_pu16_xyz,
	gx_tf_send_ps16_xyz,
	gx_tf_send_pf_xyz,
	NULL, NULL, NULL,

	gx_tf_send_pi8u8_xy,
	gx_tf_send_pi8s8_xy,
	gx_tf_send_pi8u16_xy,
	gx_tf_send_pi8s16_xy,
	gx_tf_send_pi8f_xy,
	NULL, NULL, NULL,

	gx_tf_send_pi8u8_xyz,
	gx_tf_send_pi8s8_xyz,
	gx_tf_send_pi8u16_xyz,
	gx_tf_send_pi8s16_xyz,
	gx_tf_send_pi8f_xyz,
	NULL, NULL, NULL,

	gx_tf_send_pi16u8_xy,
	gx_tf_send_pi16s8_xy,
	gx_tf_send_pi16u16_xy,
	gx_tf_send_pi16s16_xy,
	gx_tf_send_pi16f_xy,
	NULL, NULL, NULL,

	gx_tf_send_pi16u8_xyz,
	gx_tf_send_pi16s8_xyz,
	gx_tf_send_pi16u16_xyz,
	gx_tf_send_pi16s16_xyz,
	gx_tf_send_pi16f_xyz,
	NULL, NULL, NULL,
};


//////////////////////////////////////////////////////////////////////////////
// direct texture (transformed)
/////////////////////////////////////////////////////////////////////////////
unsigned int gx_tf_send_tu8_s (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", MEM (mem));

	GX_TEX_COORD_1 (MEM (mem));

	return 1;
}


unsigned int gx_tf_send_tu8_st (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", MEM (mem), MEM (mem + 1));

	GX_TEX_COORD_2 (MEM (mem), MEM (mem + 1));

	return 2;
}


unsigned int gx_tf_send_ts8_s (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", MEMS (mem));

	GX_TEX_COORD_1 (MEMS (mem));

	return 1;
}


unsigned int gx_tf_send_ts8_st (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", MEMS (mem), MEMS (mem + 1));

	GX_TEX_COORD_2 (MEMS (mem), MEMS (mem + 1));

	return 2;
}


unsigned int gx_tf_send_tu16_s (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", MEMR16 (mem));

	GX_TEX_COORD_1 (MEMR16 (mem));

	return 2;
}


unsigned int gx_tf_send_tu16_st (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", MEMR16 (mem), MEMR16 (mem + 2));

	GX_TEX_COORD_2 (MEMR16 (mem), MEMR16 (mem + 2));

	return 4;
}


unsigned int gx_tf_send_ts16_s (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", MEMSR16 (mem));

	GX_TEX_COORD_1 (MEMSR16 (mem));

	return 2;
}


unsigned int gx_tf_send_ts16_st (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", MEMSR16 (mem), MEMSR16 (mem + 2));

	GX_TEX_COORD_2 (MEMSR16 (mem), MEMSR16 (mem + 2));

	return 4;
}


unsigned int gx_tf_send_tf_s (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", MEMRF (mem));

	GX_TEX_COORD_1 (MEMRF (mem));

	return 4;
}


unsigned int gx_tf_send_tf_st (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", MEMRF (mem), MEMRF (mem + 4));

	GX_TEX_COORD_2 (MEMRF (mem), MEMRF (mem + 4));

	return 8;
}


// 8 bit indexed texture
unsigned int gx_tf_send_ti8u8_s (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", p[0]);

	GX_TEX_COORD_1 (p[0]);

	return 1;
}


unsigned int gx_tf_send_ti8u8_st (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", p[0], p[1]);

	GX_TEX_COORD_2 (p[0], p[1]);

	return 1;
}


unsigned int gx_tf_send_ti8s8_s (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", p[0]);

	GX_TEX_COORD_1 (p[0]);

	return 1;
}


unsigned int gx_tf_send_ti8s8_st (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", p[0], p[1]);

	GX_TEX_COORD_2 (p[0], p[1]);

	return 1;
}


unsigned int gx_tf_send_ti8u16_s (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", BSWAP16 (p[0]));

	GX_TEX_COORD_1 (BSWAP16 (p[0]));

	return 1;
}


unsigned int gx_tf_send_ti8u16_st (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", BSWAP16 (p[0]), BSWAP16 (p[1]));

	GX_TEX_COORD_2 (BSWAP16 (p[0]), BSWAP16 (p[1]));

	return 1;
}


unsigned int gx_tf_send_ti8s16_s (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", BSWAPS16 (p[0]));

	GX_TEX_COORD_1 (BSWAPS16 (p[0]));

	return 1;
}


unsigned int gx_tf_send_ti8s16_st (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", BSWAPS16 (p[0]), BSWAPS16 (p[1]));

	GX_TEX_COORD_2 (BSWAPS16 (p[0]), BSWAPS16 (p[1]));

	return 1;
}


unsigned int gx_tf_send_ti8f_s (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", BSWAPF (p[0]));

	GX_TEX_COORD_1 (BSWAPF (p[0]));

	return 1;
}


unsigned int gx_tf_send_ti8f_st (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEM (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", BSWAPF (p[0]), BSWAPF (p[1]));

	GX_TEX_COORD_2 (BSWAPF (p[0]), BSWAPF (p[1]));

	return 1;
}


// 16 bit indexed texture
unsigned int gx_tf_send_ti16u8_s (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", p[0]);

	GX_TEX_COORD_1 (p[0]);

	return 2;
}


unsigned int gx_tf_send_ti16u8_st (__u32 mem)
{
	__u8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", p[0], p[1]);

	GX_TEX_COORD_2 (p[0], p[1]);

	return 2;
}


unsigned int gx_tf_send_ti16s8_s (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", p[0]);

	GX_TEX_COORD_1 (p[0]);

	return 2;
}


unsigned int gx_tf_send_ti16s8_st (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", p[0], p[1]);

	GX_TEX_COORD_2 (p[0], p[1]);

	return 2;
}


unsigned int gx_tf_send_ti16u16_s (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", BSWAP16 (p[0]));

	GX_TEX_COORD_1 (BSWAP16 (p[0]));

	return 2;
}


unsigned int gx_tf_send_ti16u16_st (__u32 mem)
{
	__u16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", BSWAP16 (p[0]), BSWAP16 (p[1]));

	GX_TEX_COORD_2 (BSWAP16 (p[0]), BSWAP16 (p[1]));

	return 2;
}


unsigned int gx_tf_send_ti16s16_s (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d)", BSWAPS16 (p[0]));

	GX_TEX_COORD_1 (BSWAPS16 (p[0]));

	return 2;
}


unsigned int gx_tf_send_ti16s16_st (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%d, %d)", BSWAPS16 (p[0]), BSWAPS16 (p[1]));

	GX_TEX_COORD_2 (BSWAPS16 (p[0]), BSWAPS16 (p[1]));

	return 2;
}


unsigned int gx_tf_send_ti16f_s (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f)", BSWAPF (p[0]));

	GX_TEX_COORD_1 (BSWAPF (p[0]));

	return 2;
}


unsigned int gx_tf_send_ti16f_st (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_TEXTURE_ARRAY_BASE (0) + MEMR16 (mem) * CP_TEXTURE_ARRAY_STRIDE (0));


	DEBUG (EVENT_LOG_GX, "....  tex   (%.2f, %.2f)", BSWAPF (p[0]), BSWAPF (p[1]));

	GX_TEX_COORD_2 (BSWAPF (p[0]), BSWAPF (p[1]));

	return 2;
}


static unsigned int (*gxtex_tf[]) (__u32 mem) =
{
	NULL,

	gx_tf_send_tu8_s,
	gx_tf_send_ts8_s,
	gx_tf_send_tu16_s,
	gx_tf_send_ts16_s,
	gx_tf_send_tf_s,
	NULL, NULL, NULL,

	gx_tf_send_tu8_st,
	gx_tf_send_ts8_st,
	gx_tf_send_tu16_st,
	gx_tf_send_ts16_st,
	gx_tf_send_tf_st,
	NULL, NULL, NULL,

	gx_tf_send_ti8u8_s,
	gx_tf_send_ti8s8_s,
	gx_tf_send_ti8u16_s,
	gx_tf_send_ti8s16_s,
	gx_tf_send_ti8f_s,
	NULL, NULL, NULL,

	gx_tf_send_ti8u8_st,
	gx_tf_send_ti8s8_st,
	gx_tf_send_ti8u16_st,
	gx_tf_send_ti8s16_st,
	gx_tf_send_ti8f_st,
	NULL, NULL, NULL,

	gx_tf_send_ti16u8_s,
	gx_tf_send_ti16s8_s,
	gx_tf_send_ti16u16_s,
	gx_tf_send_ti16s16_s,
	gx_tf_send_ti16f_s,
	NULL, NULL, NULL,

	gx_tf_send_ti16u8_st,
	gx_tf_send_ti16s8_st,
	gx_tf_send_ti16u16_st,
	gx_tf_send_ti16s16_st,
	gx_tf_send_ti16f_st,
	NULL, NULL, NULL,
};


//////////////////////////////////////////////////////////////////////////////
// direct color 0 (transformed)
//////////////////////////////////////////////////////////////////////////////
unsigned int gx_tf_send_c0_rgb565 (__u32 mem)
{
	__u32 c = color_unpack_rgb565 (MEMR16 (mem));


	GX_COLOR0_3V ((__u8 *) &c);

	return 2;
}


unsigned int gx_tf_send_c0_rgb8 (__u32 mem)
{
	__u32 c = MEM32 (mem);


	GX_COLOR0_3V ((__u8 *) &c);

	return 3;
}


unsigned int gx_tf_send_c0_rgbx8 (__u32 mem)
{
	__u32 c = MEM32 (mem);


	GX_COLOR0_3V ((__u8 *) &c);

	return 4;
}


unsigned int gx_tf_send_c0_rgba4 (__u32 mem)
{
	__u32 c = color_unpack_rgba4 (MEMR32 (mem) >> 16);


	GX_COLOR0_4V ((__u8 *) &c);

	return 2;
}


unsigned int gx_tf_send_c0_rgba6 (__u32 mem)
{
	__u32 c = color_unpack_rgba6 (MEMR32 (mem) >> 8);


	GX_COLOR0_4V ((__u8 *) &c);

	return 3;
}


unsigned int gx_tf_send_c0_rgba8 (__u32 mem)
{
	__u32 c = MEM32 (mem);


	GX_COLOR0_4V ((__u8 *) &c);

	return 4;
}


// 8 bit indexed color
unsigned int gx_tf_send_c0i8_rgb565 (__u32 mem)
{
	__u32 c = color_unpack_rgb565 (MEMR16 (CP_COLOR0_ARRAY_BASE + MEM (mem) * CP_COLOR0_ARRAY_STRIDE));


	GX_COLOR0_3V ((__u8 *) &c);

	return 1;
}


unsigned int gx_tf_send_c0i8_rgb8 (__u32 mem)
{
	__u32 c = MEM32 (CP_COLOR0_ARRAY_BASE + MEM (mem) * CP_COLOR0_ARRAY_STRIDE);


	GX_COLOR0_3V ((__u8 *) &c);

	return 1;
}


unsigned int gx_tf_send_c0i8_rgbx8 (__u32 mem)
{
	__u32 c = MEM32 (CP_COLOR0_ARRAY_BASE + MEM (mem) * CP_COLOR0_ARRAY_STRIDE);


	GX_COLOR0_3V ((__u8 *) &c);

	return 1;
}


unsigned int gx_tf_send_c0i8_rgba4 (__u32 mem)
{
	__u32 c = color_unpack_rgba4 (MEMR32 (CP_COLOR0_ARRAY_BASE + MEM (mem) * CP_COLOR0_ARRAY_STRIDE) >> 16);


	GX_COLOR0_4V ((__u8 *) &c);

	return 1;
}


unsigned int gx_tf_send_c0i8_rgba6 (__u32 mem)
{
	__u32 c = color_unpack_rgba6 (MEMR32 (CP_COLOR0_ARRAY_BASE + MEM (mem) * CP_COLOR0_ARRAY_STRIDE) >> 8);


	GX_COLOR0_4V ((__u8 *) &c);

	return 1;
}


unsigned int gx_tf_send_c0i8_rgba8 (__u32 mem)
{
	__u32 c = MEM32 (CP_COLOR0_ARRAY_BASE + MEM (mem) * CP_COLOR0_ARRAY_STRIDE);


	GX_COLOR0_4V ((__u8 *) &c);

	return 1;
}


// 16 bit indexed color
unsigned int gx_tf_send_c0i16_rgb565 (__u32 mem)
{
	__u32 c = color_unpack_rgb565 (MEMR16 (CP_COLOR0_ARRAY_BASE + MEMR16 (mem) * CP_COLOR0_ARRAY_STRIDE));


	GX_COLOR0_3V ((__u8 *) &c);

	return 2;
}


unsigned int gx_tf_send_c0i16_rgb8 (__u32 mem)
{
	__u32 c = MEM32 (CP_COLOR0_ARRAY_BASE + MEMR16 (mem) * CP_COLOR0_ARRAY_STRIDE);


	GX_COLOR0_3V ((__u8 *) &c);

	return 2;
}


unsigned int gx_tf_send_c0i16_rgbx8 (__u32 mem)
{
	__u32 c = MEM32 (CP_COLOR0_ARRAY_BASE + MEMR16 (mem) * CP_COLOR0_ARRAY_STRIDE);


	GX_COLOR0_3V ((__u8 *) &c);

	return 2;
}


unsigned int gx_tf_send_c0i16_rgba4 (__u32 mem)
{
	__u32 c = color_unpack_rgba4 (MEMR32 (CP_COLOR0_ARRAY_BASE + MEMR16 (mem) * CP_COLOR0_ARRAY_STRIDE) >> 16);


	GX_COLOR0_4V ((__u8 *) &c);

	return 2;
}


unsigned int gx_tf_send_c0i16_rgba6 (__u32 mem)
{
	__u32 c = color_unpack_rgba6 (MEMR32 (CP_COLOR0_ARRAY_BASE + MEMR16 (mem) * CP_COLOR0_ARRAY_STRIDE) >> 8);


	GX_COLOR0_4V ((__u8 *) &c);

	return 2;
}


unsigned int gx_tf_send_c0i16_rgba8 (__u32 mem)
{
	__u32 c = MEM32 (CP_COLOR0_ARRAY_BASE + MEMR16 (mem) * CP_COLOR0_ARRAY_STRIDE);


	GX_COLOR0_4V ((__u8 *) &c);

	return 2;
}


static unsigned int (*gxcol0_tf[]) (__u32 mem) =
{
	NULL,

	gx_tf_send_c0_rgb565,
	gx_tf_send_c0_rgb8,
	gx_tf_send_c0_rgbx8,
	gx_tf_send_c0_rgba4, gx_tf_send_c0_rgba6, gx_tf_send_c0_rgba8, NULL, NULL,

	gx_tf_send_c0_rgb565, gx_tf_send_c0_rgb8, gx_tf_send_c0_rgbx8,
	gx_tf_send_c0_rgba4,
	gx_tf_send_c0_rgba6,
	gx_tf_send_c0_rgba8,
	NULL, NULL,

	gx_tf_send_c0i8_rgb565,
	gx_tf_send_c0i8_rgb8,
	gx_tf_send_c0i8_rgbx8,
	gx_tf_send_c0i8_rgba4, gx_tf_send_c0i8_rgba6, gx_tf_send_c0i8_rgba8, NULL, NULL,

	gx_tf_send_c0i8_rgb565, gx_tf_send_c0i8_rgb8, gx_tf_send_c0i8_rgbx8,
	gx_tf_send_c0i8_rgba4,
	gx_tf_send_c0i8_rgba6,
	gx_tf_send_c0i8_rgba8,
	NULL, NULL,

	gx_tf_send_c0i16_rgb565,
	gx_tf_send_c0i16_rgb8,
	gx_tf_send_c0i16_rgbx8,
	gx_tf_send_c0i16_rgba4, gx_tf_send_c0i16_rgba6, gx_tf_send_c0i16_rgba8, NULL, NULL,

	gx_tf_send_c0i16_rgb565, gx_tf_send_c0i16_rgb8, gx_tf_send_c0i16_rgbx8,
	gx_tf_send_c0i16_rgba4,
	gx_tf_send_c0i16_rgba6,
	gx_tf_send_c0i16_rgba8,
	NULL, NULL,
};


//////////////////////////////////////////////////////////////////////////////
// direct color 1 (transformed)
//////////////////////////////////////////////////////////////////////////////
unsigned int gx_tf_send_c1_rgb565 (__u32 mem)
{
	__u32 c = color_unpack_rgb565 (MEMR16 (mem));


	GX_COLOR1_3V ((__u8 *) &c);

	return 2;
}


unsigned int gx_tf_send_c1_rgb8 (__u32 mem)
{
	__u32 c = MEM32 (mem);


	GX_COLOR1_3V ((__u8 *) &c);

	return 3;
}


unsigned int gx_tf_send_c1_rgbx8 (__u32 mem)
{
	__u32 c = MEM32 (mem);


	GX_COLOR1_3V ((__u8 *) &c);

	return 4;
}


unsigned int gx_tf_send_c1_rgba4 (__u32 mem)
{
	__u32 c = color_unpack_rgba4 (MEMR32 (mem) >> 16);


	GX_COLOR1_4V ((__u8 *) &c);

	return 2;
}


unsigned int gx_tf_send_c1_rgba6 (__u32 mem)
{
	__u32 c = color_unpack_rgba6 (MEMR32 (mem) >> 8);


	GX_COLOR1_4V ((__u8 *) &c);

	return 3;
}


unsigned int gx_tf_send_c1_rgba8 (__u32 mem)
{
	__u32 c = MEM32 (mem);


	GX_COLOR1_4V ((__u8 *) &c);

	return 4;
}


// 8 bit indexed color
unsigned int gx_tf_send_c1i8_rgb565 (__u32 mem)
{
	__u32 c = color_unpack_rgb565 (MEMR16 (CP_COLOR1_ARRAY_BASE + MEM (mem) * CP_COLOR1_ARRAY_STRIDE));


	GX_COLOR1_3V ((__u8 *) &c);

	return 1;
}


unsigned int gx_tf_send_c1i8_rgb8 (__u32 mem)
{
	__u32 c = MEM32 (CP_COLOR1_ARRAY_BASE + MEM (mem) * CP_COLOR1_ARRAY_STRIDE);


	GX_COLOR1_3V ((__u8 *) &c);

	return 1;
}


unsigned int gx_tf_send_c1i8_rgbx8 (__u32 mem)
{
	__u32 c = MEM32 (CP_COLOR1_ARRAY_BASE + MEM (mem) * CP_COLOR1_ARRAY_STRIDE);


	GX_COLOR1_3V ((__u8 *) &c);

	return 1;
}


unsigned int gx_tf_send_c1i8_rgba4 (__u32 mem)
{
	__u32 c = color_unpack_rgba4 (MEMR32 (CP_COLOR1_ARRAY_BASE + MEM (mem) * CP_COLOR1_ARRAY_STRIDE) >> 16);


	GX_COLOR1_4V ((__u8 *) &c);

	return 1;
}


unsigned int gx_tf_send_c1i8_rgba6 (__u32 mem)
{
	__u32 c = color_unpack_rgba6 (MEMR32 (CP_COLOR1_ARRAY_BASE + MEM (mem) * CP_COLOR1_ARRAY_STRIDE) >> 8);


	GX_COLOR1_4V ((__u8 *) &c);

	return 1;
}


unsigned int gx_tf_send_c1i8_rgba8 (__u32 mem)
{
	__u32 c = MEM32 (CP_COLOR1_ARRAY_BASE + MEM (mem) * CP_COLOR1_ARRAY_STRIDE);


	GX_COLOR1_4V ((__u8 *) &c);

	return 1;
}


// 16 bit indexed color
unsigned int gx_tf_send_c1i16_rgb565 (__u32 mem)
{
	__u32 c = color_unpack_rgb565 (MEMR16 (CP_COLOR1_ARRAY_BASE + MEMR16 (mem) * CP_COLOR1_ARRAY_STRIDE));


	GX_COLOR1_3V ((__u8 *) &c);

	return 2;
}


unsigned int gx_tf_send_c1i16_rgb8 (__u32 mem)
{
	__u32 c = MEM32 (CP_COLOR1_ARRAY_BASE + MEMR16 (mem) * CP_COLOR1_ARRAY_STRIDE);


	GX_COLOR1_3V ((__u8 *) &c);

	return 2;
}


unsigned int gx_tf_send_c1i16_rgbx8 (__u32 mem)
{
	__u32 c = MEM32 (CP_COLOR1_ARRAY_BASE + MEMR16 (mem) * CP_COLOR1_ARRAY_STRIDE);


	GX_COLOR1_3V ((__u8 *) &c);

	return 2;
}


unsigned int gx_tf_send_c1i16_rgba4 (__u32 mem)
{
	__u32 c = color_unpack_rgba4 (MEMR32 (CP_COLOR1_ARRAY_BASE + MEMR16 (mem) * CP_COLOR1_ARRAY_STRIDE) >> 16);


	GX_COLOR1_4V ((__u8 *) &c);

	return 2;
}


unsigned int gx_tf_send_c1i16_rgba6 (__u32 mem)
{
	__u32 c = color_unpack_rgba6 (MEMR32 (CP_COLOR1_ARRAY_BASE + MEMR16 (mem) * CP_COLOR1_ARRAY_STRIDE) >> 8);


	GX_COLOR1_4V ((__u8 *) &c);

	return 2;
}


unsigned int gx_tf_send_c1i16_rgba8 (__u32 mem)
{
	__u32 c = MEM32 (CP_COLOR1_ARRAY_BASE + MEMR16 (mem) * CP_COLOR1_ARRAY_STRIDE);


	GX_COLOR1_4V ((__u8 *) &c);

	return 2;
}


static unsigned int (*gxcol1_tf[]) (__u32 mem) =
{
	NULL,

	gx_tf_send_c1_rgb565,
	gx_tf_send_c1_rgb8,
	gx_tf_send_c1_rgbx8,
	gx_tf_send_c1_rgba4, gx_tf_send_c1_rgba6, gx_tf_send_c1_rgba8, NULL, NULL,

	gx_tf_send_c1_rgb565, gx_tf_send_c1_rgb8, gx_tf_send_c1_rgbx8,
	gx_tf_send_c1_rgba4,
	gx_tf_send_c1_rgba6,
	gx_tf_send_c1_rgba8,
	NULL, NULL,

	gx_tf_send_c1i8_rgb565,
	gx_tf_send_c1i8_rgb8,
	gx_tf_send_c1i8_rgbx8,
	gx_tf_send_c1i8_rgba4, gx_tf_send_c1i8_rgba6, gx_tf_send_c1i8_rgba8, NULL, NULL,

	gx_tf_send_c1i8_rgb565, gx_tf_send_c1i8_rgb8, gx_tf_send_c1i8_rgbx8,
	gx_tf_send_c1i8_rgba4,
	gx_tf_send_c1i8_rgba6,
	gx_tf_send_c1i8_rgba8,
	NULL, NULL,

	gx_tf_send_c1i16_rgb565,
	gx_tf_send_c1i16_rgb8,
	gx_tf_send_c1i16_rgbx8,
	gx_tf_send_c1i16_rgba4, gx_tf_send_c1i16_rgba6, gx_tf_send_c1i16_rgba8, NULL, NULL,

	gx_tf_send_c1i16_rgb565, gx_tf_send_c1i16_rgb8, gx_tf_send_c1i16_rgbx8,
	gx_tf_send_c1i16_rgba4,
	gx_tf_send_c1i16_rgba6,
	gx_tf_send_c1i16_rgba8,
	NULL, NULL,
};


//////////////////////////////////////////////////////////////////////////////
// direct normal (transformed)
//////////////////////////////////////////////////////////////////////////////
unsigned int gx_tf_send_ns8_xyz (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (mem);


	DEBUG (EVENT_LOG_GX, "....  normal (%d, %d, %d)", p[0], p[1], p[2]);

	GX_NORMAL_V (p);

	return 3;
}


unsigned int gx_tf_send_ns16_xyz (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  normal (%d, %d, %d)", MEMSR16 (mem), MEMSR16 (mem + 2), MEMSR16 (mem + 4));

	GX_NORMAL (MEMSR16 (mem), MEMSR16 (mem + 2), MEMSR16 (mem + 4));

	return 6;
}


unsigned int gx_tf_send_nf_xyz (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  normal (%.2f, %.2f, %.2f)", MEMRF (mem), MEMRF (mem + 4), MEMRF (mem + 8));

	GX_NORMAL (MEMRF (mem), MEMRF (mem + 4), MEMRF (mem + 8));

	return 12;
}


unsigned int gx_tf_send_ns8_nbt (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (mem);


	DEBUG (EVENT_LOG_GX, "....  normal (%d, %d, %d)", p[0], p[1], p[2]);
	DEBUG (EVENT_LOG_GX, "....  binormal (%d, %d, %d)", p[3], p[4], p[5]);
	DEBUG (EVENT_LOG_GX, "....  tangent (%d, %d, %d)", p[6], p[7], p[8]);

	GX_NORMAL_NBT_V (p);

	return 9;
}


unsigned int gx_tf_send_ns16_nbt (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  normal (%d, %d, %d)", MEMSR16 (mem), MEMSR16 (mem + 2), MEMSR16 (mem + 4));
	DEBUG (EVENT_LOG_GX, "....  binormal (%d, %d, %d)", MEMSR16 (mem + 6), MEMSR16 (mem + 8), MEMSR16 (mem + 10));
	DEBUG (EVENT_LOG_GX, "....  tangent (%d, %d, %d)", MEMSR16 (mem + 12), MEMSR16 (mem + 14), MEMSR16 (mem + 16));

	GX_NORMAL_N (MEMSR16 (mem +  0), MEMSR16 (mem +  2), MEMSR16 (mem +  4));
	GX_NORMAL_B (MEMSR16 (mem +  6), MEMSR16 (mem +  8), MEMSR16 (mem + 10));
	GX_NORMAL_T (MEMSR16 (mem + 12), MEMSR16 (mem + 14), MEMSR16 (mem + 16));

	return 18;
}


unsigned int gx_tf_send_nf_nbt (__u32 mem)
{
	DEBUG (EVENT_LOG_GX, "....  normal (%.2f, %.2f, %.2f)", MEMRF (mem), MEMRF (mem + 4), MEMRF (mem + 8));
	DEBUG (EVENT_LOG_GX, "....  binormal (%.2f, %.2f, %.2f)", MEMRF (mem + 12), MEMRF (mem + 16), MEMRF (mem + 20));
	DEBUG (EVENT_LOG_GX, "....  tangent (%.2f, %.2f, %.2f)", MEMRF (mem + 24), MEMRF (mem + 28), MEMRF (mem + 32));

	GX_NORMAL_N (MEMRF (mem +  0), MEMRF (mem +  4), MEMRF (mem +  8));
	GX_NORMAL_B (MEMRF (mem + 12), MEMRF (mem + 16), MEMRF (mem + 20));
	GX_NORMAL_T (MEMRF (mem + 24), MEMRF (mem + 28), MEMRF (mem + 32));

	return 36;
}


// 8 bit indexed normal
unsigned int gx_tf_send_ni8s8_xyz (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEM (mem) * CP_NORMAL_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  normal (%d, %d, %d)", p[0], p[1], p[2]);

	GX_NORMAL_V (p);

	return 1;
}


unsigned int gx_tf_send_ni8s16_xyz (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEM (mem) * CP_NORMAL_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  normal (%d, %d, %d)", BSWAPS16 (p[0]), BSWAPS16 (p[1]), BSWAPS16 (p[2]));

	GX_NORMAL (BSWAPS16 (p[0]), BSWAPS16 (p[1]), BSWAPS16 (p[2]));

	return 1;
}


unsigned int gx_tf_send_ni8f_xyz (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEM (mem) * CP_NORMAL_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  normal (%.2f, %.2f, %.2f)", BSWAPF (p[0]), BSWAPF (p[1]), BSWAPF (p[2]));

	GX_NORMAL (BSWAPF (p[0]), BSWAPF (p[1]), BSWAPF (p[2]));

	return 1;
}


unsigned int gx_tf_send_ni8s8_nbt (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEM (mem) * CP_NORMAL_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  normal (%d, %d, %d)", p[0], p[1], p[2]);
	DEBUG (EVENT_LOG_GX, "....  binormal (%d, %d, %d)", p[3], p[4], p[5]);
	DEBUG (EVENT_LOG_GX, "....  tangent (%d, %d, %d)", p[6], p[7], p[8]);

	GX_NORMAL_NBT_V (p);

	return 1;
}


unsigned int gx_tf_send_ni8s16_nbt (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEM (mem) * CP_NORMAL_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  normal (%d, %d, %d)", BSWAPS16 (p[0]), BSWAPS16 (p[1]), BSWAPS16 (p[2]));
	DEBUG (EVENT_LOG_GX, "....  binormal (%d, %d, %d)", BSWAPS16 (p[3]), BSWAPS16 (p[4]), BSWAPS16 (p[5]));
	DEBUG (EVENT_LOG_GX, "....  tangent (%d, %d, %d)", BSWAPS16 (p[6]), BSWAPS16 (p[7]), BSWAPS16 (p[8]));

	GX_NORMAL_N (BSWAPS16 (p[0]), BSWAPS16 (p[1]), BSWAPS16 (p[2]));
	GX_NORMAL_B (BSWAPS16 (p[3]), BSWAPS16 (p[4]), BSWAPS16 (p[5]));
	GX_NORMAL_T (BSWAPS16 (p[6]), BSWAPS16 (p[7]), BSWAPS16 (p[8]));

	return 1;
}


unsigned int gx_tf_send_ni8f_nbt (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEM (mem) * CP_NORMAL_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  normal (%.2f, %.2f, %.2f)", BSWAPF (p[0]), BSWAPF (p[1]), BSWAPF (p[2]));
	DEBUG (EVENT_LOG_GX, "....  binormal (%.2f, %.2f, %.2f)", BSWAPF (p[3]), BSWAPF (p[4]), BSWAPF (p[5]));
	DEBUG (EVENT_LOG_GX, "....  tangent (%.2f, %.2f, %.2f)", BSWAPF (p[6]), BSWAPF (p[7]), BSWAPF (p[8]));

	GX_NORMAL_N (BSWAPF (p[0]), BSWAPF (p[1]), BSWAPF (p[2]));
	GX_NORMAL_B (BSWAPF (p[3]), BSWAPF (p[4]), BSWAPF (p[5]));
	GX_NORMAL_T (BSWAPF (p[6]), BSWAPF (p[7]), BSWAPF (p[8]));

	return 1;
}


unsigned int gx_tf_send_ni8s8_nbt3 (__u32 mem)
{
	__s8 *n = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEM (mem + 0) * CP_NORMAL_ARRAY_STRIDE);
	__s8 *b = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEM (mem + 1) * CP_NORMAL_ARRAY_STRIDE);
	__s8 *t = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEM (mem + 2) * CP_NORMAL_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  normal (%d, %d, %d)", n[0], n[1], n[2]);
	DEBUG (EVENT_LOG_GX, "....  binormal (%d, %d, %d)", b[0], b[1], b[2]);
	DEBUG (EVENT_LOG_GX, "....  tangent (%d, %d, %d)", t[0], t[1], t[2]);

	GX_NORMAL_N_V (n);
	GX_NORMAL_B_V (b);
	GX_NORMAL_T_V (t);

	return 3;
}


unsigned int gx_tf_send_ni8s16_nbt3 (__u32 mem)
{
	__s16 *n = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEM (mem + 0) * CP_NORMAL_ARRAY_STRIDE);
	__s16 *b = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEM (mem + 1) * CP_NORMAL_ARRAY_STRIDE);
	__s16 *t = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEM (mem + 2) * CP_NORMAL_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  normal (%d, %d, %d)", BSWAPS16 (n[0]), BSWAPS16 (n[1]), BSWAPS16 (n[2]));
	DEBUG (EVENT_LOG_GX, "....  binormal (%d, %d, %d)", BSWAPS16 (b[0]), BSWAPS16 (b[1]), BSWAPS16 (b[2]));
	DEBUG (EVENT_LOG_GX, "....  tangent (%d, %d, %d)", BSWAPS16 (t[0]), BSWAPS16 (t[1]), BSWAPS16 (t[2]));

	GX_NORMAL_N (BSWAPS16 (n[0]), BSWAPS16 (n[1]), BSWAPS16 (n[2]));
	GX_NORMAL_B (BSWAPS16 (b[0]), BSWAPS16 (b[1]), BSWAPS16 (b[2]));
	GX_NORMAL_T (BSWAPS16 (t[0]), BSWAPS16 (t[1]), BSWAPS16 (t[2]));

	return 3;
}


unsigned int gx_tf_send_ni8f_nbt3 (__u32 mem)
{
	float *n = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEM (mem + 0) * CP_NORMAL_ARRAY_STRIDE);
	float *b = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEM (mem + 1) * CP_NORMAL_ARRAY_STRIDE);
	float *t = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEM (mem + 2) * CP_NORMAL_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  normal (%.2f, %.2f, %.2f)", BSWAPF (n[0]), BSWAPF (n[1]), BSWAPF (n[2]));
	DEBUG (EVENT_LOG_GX, "....  binormal (%.2f, %.2f, %.2f)", BSWAPF (b[0]), BSWAPF (b[1]), BSWAPF (b[2]));
	DEBUG (EVENT_LOG_GX, "....  tangent (%.2f, %.2f, %.2f)", BSWAPF (t[0]), BSWAPF (t[1]), BSWAPF (t[2]));

	GX_NORMAL_N (BSWAPF (n[0]), BSWAPF (n[1]), BSWAPF (n[2]));
	GX_NORMAL_B (BSWAPF (b[0]), BSWAPF (b[1]), BSWAPF (b[2]));
	GX_NORMAL_T (BSWAPF (t[0]), BSWAPF (t[1]), BSWAPF (t[2]));

	return 3;
}


// 16 bit indexed normal
unsigned int gx_tf_send_ni16s8_xyz (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEMR16 (mem) * CP_NORMAL_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  normal (%d, %d, %d)", p[0], p[1], p[2]);

	GX_NORMAL_V (p);

	return 2;
}


unsigned int gx_tf_send_ni16s16_xyz (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEMR16 (mem) * CP_NORMAL_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  normal (%d, %d, %d)", BSWAPS16 (p[0]), BSWAPS16 (p[1]), BSWAPS16 (p[2]));

	GX_NORMAL (BSWAPS16 (p[0]), BSWAPS16 (p[1]), BSWAPS16 (p[2]));

	return 2;
}


unsigned int gx_tf_send_ni16f_xyz (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEMR16 (mem) * CP_NORMAL_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  normal (%.2f, %.2f, %.2f)", BSWAPF (p[0]), BSWAPF (p[1]), BSWAPF (p[2]));

	GX_NORMAL (BSWAPF (p[0]), BSWAPF (p[1]), BSWAPF (p[2]));

	return 2;
}


unsigned int gx_tf_send_ni16s8_nbt (__u32 mem)
{
	__s8 *p = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEMR16 (mem) * CP_NORMAL_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  normal (%d, %d, %d)", p[0], p[1], p[2]);
	DEBUG (EVENT_LOG_GX, "....  binormal (%d, %d, %d)", p[3], p[4], p[5]);
	DEBUG (EVENT_LOG_GX, "....  tangent (%d, %d, %d)", p[6], p[7], p[8]);

	GX_NORMAL_NBT_V (p);

	return 2;
}


unsigned int gx_tf_send_ni16s16_nbt (__u32 mem)
{
	__s16 *p = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEMR16 (mem) * CP_NORMAL_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  normal (%d, %d, %d)", BSWAPS16 (p[0]), BSWAPS16 (p[1]), BSWAPS16 (p[2]));
	DEBUG (EVENT_LOG_GX, "....  binormal (%d, %d, %d)", BSWAPS16 (p[3]), BSWAPS16 (p[4]), BSWAPS16 (p[5]));
	DEBUG (EVENT_LOG_GX, "....  tangent (%d, %d, %d)", BSWAPS16 (p[6]), BSWAPS16 (p[7]), BSWAPS16 (p[8]));

	GX_NORMAL_N (BSWAPS16 (p[0]), BSWAPS16 (p[1]), BSWAPS16 (p[2]));
	GX_NORMAL_B (BSWAPS16 (p[3]), BSWAPS16 (p[4]), BSWAPS16 (p[5]));
	GX_NORMAL_T (BSWAPS16 (p[6]), BSWAPS16 (p[7]), BSWAPS16 (p[8]));

	return 2;
}


unsigned int gx_tf_send_ni16f_nbt (__u32 mem)
{
	float *p = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEMR16 (mem) * CP_NORMAL_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  normal (%.2f, %.2f, %.2f)", BSWAPF (p[0]), BSWAPF (p[1]), BSWAPF (p[2]));
	DEBUG (EVENT_LOG_GX, "....  binormal (%.2f, %.2f, %.2f)", BSWAPF (p[3]), BSWAPF (p[4]), BSWAPF (p[5]));
	DEBUG (EVENT_LOG_GX, "....  tangent (%.2f, %.2f, %.2f)", BSWAPF (p[6]), BSWAPF (p[7]), BSWAPF (p[8]));

	GX_NORMAL_N (BSWAPF (p[0]), BSWAPF (p[1]), BSWAPF (p[2]));
	GX_NORMAL_B (BSWAPF (p[3]), BSWAPF (p[4]), BSWAPF (p[5]));
	GX_NORMAL_T (BSWAPF (p[6]), BSWAPF (p[7]), BSWAPF (p[8]));

	return 2;
}


unsigned int gx_tf_send_ni16s8_nbt3 (__u32 mem)
{
	__s8 *n = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEMR16 (mem + 0) * CP_NORMAL_ARRAY_STRIDE);
	__s8 *b = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEMR16 (mem + 2) * CP_NORMAL_ARRAY_STRIDE);
	__s8 *t = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEMR16 (mem + 4) * CP_NORMAL_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  normal (%d, %d, %d)", n[0], n[1], n[2]);
	DEBUG (EVENT_LOG_GX, "....  binormal (%d, %d, %d)", b[0], b[1], b[2]);
	DEBUG (EVENT_LOG_GX, "....  tangent (%d, %d, %d)", t[0], t[1], t[2]);

	GX_NORMAL_N_V (n);
	GX_NORMAL_B_V (b);
	GX_NORMAL_T_V (t);

	return 6;
}


unsigned int gx_tf_send_ni16s16_nbt3 (__u32 mem)
{
	__s16 *n = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEMR16 (mem + 0) * CP_NORMAL_ARRAY_STRIDE);
	__s16 *b = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEMR16 (mem + 2) * CP_NORMAL_ARRAY_STRIDE);
	__s16 *t = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEMR16 (mem + 4) * CP_NORMAL_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  normal (%d, %d, %d)", BSWAPS16 (n[0]), BSWAPS16 (n[1]), BSWAPS16 (n[2]));
	DEBUG (EVENT_LOG_GX, "....  binormal (%d, %d, %d)", BSWAPS16 (b[0]), BSWAPS16 (b[1]), BSWAPS16 (b[2]));
	DEBUG (EVENT_LOG_GX, "....  tangent (%d, %d, %d)", BSWAPS16 (t[0]), BSWAPS16 (t[1]), BSWAPS16 (t[2]));

	GX_NORMAL_N (BSWAPS16 (n[0]), BSWAPS16 (n[1]), BSWAPS16 (n[2]));
	GX_NORMAL_B (BSWAPS16 (b[0]), BSWAPS16 (b[1]), BSWAPS16 (b[2]));
	GX_NORMAL_T (BSWAPS16 (t[0]), BSWAPS16 (t[1]), BSWAPS16 (t[2]));

	return 6;
}


unsigned int gx_tf_send_ni16f_nbt3 (__u32 mem)
{
	float *n = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEMR16 (mem + 0) * CP_NORMAL_ARRAY_STRIDE);
	float *b = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEMR16 (mem + 2) * CP_NORMAL_ARRAY_STRIDE);
	float *t = MEM_ADDRESS (CP_NORMAL_ARRAY_BASE + MEMR16 (mem + 4) * CP_NORMAL_ARRAY_STRIDE);


	DEBUG (EVENT_LOG_GX, "....  normal (%.2f, %.2f, %.2f)", BSWAPF (n[0]), BSWAPF (n[1]), BSWAPF (n[2]));
	DEBUG (EVENT_LOG_GX, "....  binormal (%.2f, %.2f, %.2f)", BSWAPF (b[0]), BSWAPF (b[1]), BSWAPF (b[2]));
	DEBUG (EVENT_LOG_GX, "....  tangent (%.2f, %.2f, %.2f)", BSWAPF (t[0]), BSWAPF (t[1]), BSWAPF (t[2]));

	GX_NORMAL_N (BSWAPF (n[0]), BSWAPF (n[1]), BSWAPF (n[2]));
	GX_NORMAL_B (BSWAPF (b[0]), BSWAPF (b[1]), BSWAPF (b[2]));
	GX_NORMAL_T (BSWAPF (t[0]), BSWAPF (t[1]), BSWAPF (t[2]));

	return 6;
}


static unsigned int (*gxnormal_tf[]) (__u32 mem) =
{
	NULL,

	NULL,
	gx_tf_send_ns8_xyz,
	NULL,
	gx_tf_send_ns16_xyz,
	gx_tf_send_nf_xyz,
	NULL, NULL, NULL,

	NULL,
	gx_tf_send_ns8_nbt,
	NULL,
	gx_tf_send_ns16_nbt,
	gx_tf_send_nf_nbt,
	NULL, NULL, NULL,

	NULL,
	gx_tf_send_ni8s8_xyz,
	NULL,
	gx_tf_send_ni8s16_xyz,
	gx_tf_send_ni8f_xyz,
	NULL, NULL, NULL,

	NULL,
	gx_tf_send_ni8s8_nbt,
	NULL,
	gx_tf_send_ni8s16_nbt,
	gx_tf_send_ni8f_nbt,
	NULL, NULL, NULL,

	NULL,
	gx_tf_send_ni16s8_xyz,
	NULL,
	gx_tf_send_ni16s16_xyz,
	gx_tf_send_ni16f_xyz,
	NULL, NULL, NULL,

	NULL,
	gx_tf_send_ni16s8_nbt,
	NULL,
	gx_tf_send_ni16s16_nbt,
	gx_tf_send_ni16f_nbt,
	NULL, NULL, NULL,

	NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL,

	NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL,

	NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL,

	NULL,
	gx_tf_send_ni8s8_nbt3,
	NULL,
	gx_tf_send_ni8s16_nbt3,
	gx_tf_send_ni8f_nbt3,
	NULL, NULL, NULL,

	NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL,

	NULL,
	gx_tf_send_ni16s8_nbt3,
	NULL,
	gx_tf_send_ni16s16_nbt3,
	gx_tf_send_ni16f_nbt3,
	NULL, NULL, NULL,
};


inline void vector_scale (float dst[3], float src[3], float f)
{
	dst[0] = src[0] * f;
	dst[1] = src[1] * f;
	dst[2] = src[2] * f;
}


inline void vector_scale_d (double dst[3], double src[3], double f)
{
	dst[0] = src[0] * f;
	dst[1] = src[1] * f;
	dst[2] = src[2] * f;
}


inline void vector2_scale (float dst[2], float src[2], float f)
{
	dst[0] = src[0] * f;
	dst[1] = src[1] * f;
}


inline void vector_copy (float dst[3], float src[3])
{
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
}


inline void vector2_copy (float dst[2], float src[2])
{
	dst[0] = src[0];
	dst[1] = src[1];
}


inline void vector_clear (float dst[3])
{
	dst[0] = 0;
	dst[1] = 0;
	dst[2] = 0;
}


inline void vector_set (float dst[3], float t)
{
	dst[0] = t;
	dst[1] = t;
	dst[2] = t;
}


inline void vector_add (float dst[3], float a[3], float b[3])
{
	dst[0] = a[0] + b[0];
	dst[1] = a[1] + b[1];
	dst[2] = a[2] + b[2];
}


inline void vector_add_f (float dst[3], float a[3], float b)
{
	dst[0] = a[0] + b;
	dst[1] = a[1] + b;
	dst[2] = a[2] + b;
}


inline void vector_sub (float dst[3], float a[3], float b[3])
{
	dst[0] = a[0] - b[0];
	dst[1] = a[1] - b[1];
	dst[2] = a[2] - b[2];
}


inline float vector_dot_product (float a[3], float b[3])
{
	return (a[0]*b[0] + a[1]*b[1] + a[2]*b[2]);
}


inline float vector_square_magnitude (float src[3])
{
	return vector_dot_product (src, src);
}


inline float vector_magnitude (float src[3])
{
	return sqrt (vector_square_magnitude (src));
}


inline void vector_normalize (float dst[3], float src[3])
{
	float mag = vector_magnitude (src);


	if (!FLOAT_EQ (mag, 0))
		mag = 1.0 / mag;

	vector_scale (dst, src, mag);
}


inline void vector_normalize_fd (float dst[3], double src[3])
{
	double mag = sqrt (src[0]*src[0] + src[1]*src[1] + src[2]*src[2]);
	double t[3];


	if (!FLOAT_EQ (mag, 0))
		mag = 1.0 / mag;

	vector_scale_d (t, src, mag);
	dst[0] = t[0];
	dst[1] = t[1];
	dst[2] = t[2];
}


inline void transform_normal (float d[3], float v[3], int index)
{
	float *m = &XFMAT_NORMAL (index);


	d[0] = m[0]*v[0] + m[1]*v[1] + m[ 2]*v[2];
	d[1] = m[3]*v[0] + m[4]*v[1] + m[ 5]*v[2];
	d[2] = m[6]*v[0] + m[7]*v[1] + m[ 8]*v[2];
}


inline void transform_normal_d (double d[3], double v[3], int index)
{
	float *m = &XFMAT_NORMAL (index);


	d[0] = v[0]*m[0] + v[1]*m[1] + v[2]*m[ 2];
	d[1] = v[0]*m[3] + v[1]*m[4] + v[2]*m[ 5];
	d[2] = v[0]*m[6] + v[1]*m[7] + v[2]*m[ 8];
}


inline void color_u32_to_float (float dst[4], __u32 src)
{
	dst[0] = (float) ((src >> 24)       ) / 0xff;
	dst[1] = (float) ((src >> 16) & 0xff) / 0xff;
	dst[2] = (float) ((src >>  8) & 0xff) / 0xff;
	dst[3] = (float) ((src >>  0) & 0xff) / 0xff;
}


float calc_light_intensity (GXLight *light, __u32 ctrl, float pos[3], float normal[3])
{
	float intensity = 1;


	if (CHN_CTRL_ATTEN_ENABLE (ctrl))
	{
		float d, aattn, spot, dist;


		if (CHN_CTRL_ATTEN_SELECT (ctrl))
		{
			// spotlight
			float ldir[3];


			// vector from the light position to the vertex		
			vector_sub (ldir, light->pos, pos);
			// distance
			d = vector_magnitude (ldir);
			// normalize
			vector_scale (ldir, ldir, 1/d);

			// cosine of the angle between the light direction
			// and the vector from the light position to the vertex
			aattn = vector_dot_product (light->dir, ldir);
		}
		else
		{
			// specular
			d = aattn = vector_dot_product (light->half_angle, normal);
		}

		// angular attenuation
		spot = light->a[0] + light->a[1] * aattn + light->a[2] * aattn * aattn;
		if (spot < 0)
			spot = 0;

		// distance attenuation
		dist = 1.0 / (light->k[0] + light->k[1] * d + light->k[2] * d * d);
	
		intensity = spot * dist;
	}
	else
	{
		float ldir[3];

		vector_sub (ldir, light->pos, pos);
		vector_normalize (ldir, ldir);
		intensity = vector_dot_product (ldir, normal);
	}

	// clamp
	if (CHN_CTRL_DIFFUSE_ATTEN (ctrl) == 2)
		if (intensity < 0)
			intensity = 0;

	return intensity;
}


inline GXLight *xf_get_light (int i)
{
	return XF_LIGHT (i);
}


inline void calc_color (float outcolor[4], float incolor[4], int chn, float pos[3], float normal[3])
{
	float lightsum[4], lcolor[4], tcolor[4];
	__u32 lmask;
	int l;


	// select base color source
	if (CHN_CTRL_USE_MAT0 (XF_COLOR_CTRL (chn)))
		vector_copy (tcolor, material[chn]);
	else if (chn < ncolors)
		vector_copy (tcolor, incolor);
	else
		vector_set (tcolor, 1);

	if (CHN_CTRL_USE_LIGHTING (XF_COLOR_CTRL (chn)))
	{
		// select ambient color source
		if (CHN_CTRL_USE_AMB0 (XF_COLOR_CTRL (chn)))
			vector_copy (lightsum, ambient[chn]);
		else if (chn < ncolors)
			vector_copy (lightsum, incolor);
		else
			vector_clear (lightsum);

		lmask = CHN_CTRL_LIGHT_MASK (XF_COLOR_CTRL (chn));
		l = 0;

		// sum light intensity from all light sources
		while (lmask)
		{
			if (lmask & 1)
			{
				color_u32_to_float (lcolor, XF_LIGHT (l)->color);

				// modulate light color by its intensity
				vector_scale (lcolor, lcolor,
						calc_light_intensity (XF_LIGHT (l), XF_COLOR_CTRL (chn), pos, normal));

				vector_add (lightsum, lightsum, lcolor);
			}

			l++;
			lmask >>= 1;
		}
	}
	else
		vector_set (lightsum, 1);

	// same for alpha
	if (CHN_CTRL_USE_MAT0 (XF_ALPHA_CTRL (chn)))
		tcolor[3] = material[chn][3];
	else if (chn < ncolors)
		tcolor[3] = incolor[3];
	else
		tcolor[3] = 1;

	if (CHN_CTRL_USE_LIGHTING (XF_ALPHA_CTRL (chn)))
	{
		if (CHN_CTRL_USE_AMB0 (XF_ALPHA_CTRL (chn)))
			lightsum[3] = ambient[chn][3];
		else if (chn < ncolors)
			lightsum[3] = incolor[3];
		else
			lightsum[3] = 0;

		lmask = CHN_CTRL_LIGHT_MASK (XF_ALPHA_CTRL (chn));
		l = 0;

		while (lmask)
		{
			if (lmask & 1)
			{
				color_u32_to_float (lcolor, XF_LIGHT (l)->color);

				lightsum[3] += lcolor[3] *
						calc_light_intensity (XF_LIGHT (l), XF_COLOR_CTRL (chn), pos, normal);
			}

			l++;
			lmask >>= 1;
		}
	}
	else
		lightsum[3] = 1;

	outcolor[0] = CLAMP (tcolor[0] * lightsum[0], 0, 1);
	outcolor[1] = CLAMP (tcolor[1] * lightsum[1], 0, 1);
	outcolor[2] = CLAMP (tcolor[2] * lightsum[2], 0, 1);
	outcolor[3] = CLAMP (tcolor[3] * lightsum[3], 0, 1);
}


// incorrect, only few effects work right
void tex_gen (GXTFVertex *v, int s, float pos[3])
{
	int n = TS_COORD (s);
	int row = TC_SOURCE_ROW (n);
	float t[3], out[3];


	switch (row)
	{
		case INROW_GEOM:
			vector_copy (t, pos);
			break;

		case INROW_NORMAL:
			vector_copy (t, cv->normal[0]);
			break;

		case INROW_COLORS:
			// used with color texgen
			break;

		case INROW_BINORMAL_T:
			vector_copy (t, cv->normal[2]);
			break;

		case INROW_BINORMAL_B:
			vector_copy (t, cv->normal[1]);
			break;

		// TEX0 - TEX7
		default:
			if (row - INROW_TEX0 < ntextures)
			{
				vector2_copy (t, v->tex[row - INROW_TEX0]);
				t[2] = 1;
			}
	}
	
	switch (TC_TEXGEN_TYPE (n))
	{
		case TEXGEN_REGULAR:
			if (TC_PROJECTION (n))
			// stq
				transform_texcoord_stq (out, t, tex_matrix_index[n]);
			else
			// st
			{
				transform_texcoord_st (out, t, tex_matrix_index[n]);
				out[2] = 1;
			}
			break;

		case TEXGEN_EMBOSS_MAP:
			break;
		
		case TEXGEN_COLOR_STRGBC0:
			out[0] = cv->color[0][0];
			out[1] = cv->color[0][1];
			out[2] = 1.0;
			break;

		case TEXGEN_COLOR_STRGBC1:
			out[0] = cv->color[1][0];
			out[1] = cv->color[1][1];
			out[2] = 1.0;
			break;
	}

	if (TC_NORMALIZE (n))
		vector_normalize (out, out);

	transform_texcoord_stq_dual (v->tex[s], out, TC_DUALMTX (n));

	// divide by q
	v->tex[s][0] /= v->tex[s][2];
	v->tex[s][1] /= v->tex[s][2];
}


// ignore vertex if index is max (0xff for 8bit index, 0xffff for 16bit index)
void gx_send_current_vertex (void)
{
	float tpos[3], t[3];


	// dequantize
	vector_scale (tpos, cv->pos, vdq);
	transform_position_xyz (cv->pos, tpos, pn_matrix_index);

	if (HOST_NORMALS)
	{
	// dequantize
//		vector_scale (t, cv->normal[0], ndq);
		transform_normal (t, cv->normal[0], pn_matrix_index);
		vector_normalize (cv->normal[0], t);
	}

	if (XF_COLORS)
	{
		// calculate color
		calc_color (cv->color[0], cv->color[0], 0, cv->pos, cv->normal[0]);

		if (XF_COLORS == 2)
		{
			calc_color (cv->color[1], cv->color[1], 1, cv->pos, cv->normal[0]);
#if 0
			// specular only
			vector_copy (cv->color[0], cv->color[1]);
#else
			// diffuse + specular
			vector_add (cv->color[0], cv->color[0], cv->color[1]);
#endif
		}
	}

	if (XF_TEXTURES)
	{
		// calculate texture coordinates
		vector2_scale (cv->tex[0], cv->tex[0], tdq[0]);
		tex_gen (cv, 0, tpos);
	}

	if (XF_TEXTURES)
		glTexCoord2fv (cv->tex[0]);

	if (XF_COLORS && !(gxswitches.wireframe + gxswitches.tex_transparent + gxswitches.sw_fullbright))
		glColor4fv (cv->color[0]);

	glVertex3fv (cv->pos);
}


#define DO_TRANSFORM 1
unsigned int gx_draw_new (__u32 mem, int prim, int n, int vat)
{
	unsigned int (*midx[16]) (__u32 mem);
	unsigned int (*vdata[16]) (__u32 mem);
	unsigned int (*pos) (__u32 mem);
	int nm = 0, nv = 0, psize, i, j;
	__u32 start = mem, mp = mem, list_size = 0, color;


	if (!n)
		return 0;

	// matrix indices
	if (VCD_MIDX)
	{
		if (VCD_PMIDX)
			midx[nm++] = gx_send_pmidx;

		if (VCD_T0MIDX)
			midx[nm++] = gx_send_t0midx;

		if (VCD_T1MIDX)
			midx[nm++] = gx_send_t1midx;
		if (VCD_T2MIDX)
			midx[nm++] = gx_send_t2midx;
		if (VCD_T3MIDX)
			midx[nm++] = gx_send_t3midx;
		if (VCD_T4MIDX)
			midx[nm++] = gx_send_t4midx;
		if (VCD_T5MIDX)
			midx[nm++] = gx_send_t5midx;
		if (VCD_T6MIDX)
			midx[nm++] = gx_send_t6midx;
		if (VCD_T7MIDX)
			midx[nm++] = gx_send_t7midx;
	}

	list_size += nm;
	list_size += gxnormal_size[GXIDX (NORMAL, vat)];
	list_size += gxcol_size[GXIDX (COL0, vat)];
	list_size += gxcol_size[GXIDX (COL1, vat)];
	list_size += gxtex_size[GXIDX (TEX0, vat)];
	list_size += gxtex_size[GXIDX (TEX1, vat)];
	list_size += gxtex_size[GXIDX (TEX2, vat)];
	list_size += gxtex_size[GXIDX (TEX3, vat)];
	list_size += gxtex_size[GXIDX (TEX4, vat)];
	list_size += gxtex_size[GXIDX (TEX5, vat)];
	list_size += gxtex_size[GXIDX (TEX6, vat)];
	list_size += gxtex_size[GXIDX (TEX7, vat)];
	list_size += gxpos_size[GXIDX (POS, vat)];
	if (EXCEEDES_LIST_BOUNDARY (start, list_size * n + 3))
	// draw commands take 3 bytes and return 3 + gx_draw
		return -(list_size * n + 6);

	if (CP_MATRIX_INDEX_0 != XF_MATRIX_INDEX_0)
		DEBUG (EVENT_STOP, ".gx:  CP_MATRIX_INDEX_0 is different than XF_MATRIX_INDEX_0");

	DEBUG (EVENT_LOG_GX, "....  VCD MIDX %2.2x POS %d NORMAL %d COL %d|%d TEX %d|%d|%d|%d|%d|%d|%d|%d",
											 VCD_MIDX, VCD_POS, VCD_NORMAL, VCD_COL0, VCD_COL1,
											 VCD_TEX0, VCD_TEX1, VCD_TEX2, VCD_TEX3, VCD_TEX4, VCD_TEX5, VCD_TEX6, VCD_TEX7);

	DEBUG (EVENT_LOG_GX, "....  VAT DQ %d POS %d|%d|%d NORMAL %d|%d COL0 %d|%d COL1 %d|%d",
											 VAT_BYTE_DEQUANT (vat),
											 VAT_POS_CNT (vat), VAT_POS_FMT (vat), VAT_POS_SHFT (vat),
											 VAT_NORMAL_CNT (vat), VAT_NORMAL_FMT (vat),
											 VAT_COL0_CNT (vat), VAT_COL0_FMT (vat), VAT_COL1_CNT (vat), VAT_COL1_FMT (vat));

	DEBUG (EVENT_LOG_GX, "....      TEX 0 %d|%d|%d 1 %d|%d|%d 2 %d|%d|%d 3 %d|%d|%d 4 %d|%d|%d 5 %d|%d|%d 6 %d|%d|%d 7 %d|%d|%d",
											 VAT_TEX0_CNT (vat), VAT_TEX0_FMT (vat), VAT_TEX0_SHFT (vat),
											 VAT_TEX1_CNT (vat), VAT_TEX1_FMT (vat), VAT_TEX1_SHFT (vat),
											 VAT_TEX2_CNT (vat), VAT_TEX2_FMT (vat), VAT_TEX2_SHFT (vat),
											 VAT_TEX3_CNT (vat), VAT_TEX3_FMT (vat), VAT_TEX3_SHFT (vat),
											 VAT_TEX4_CNT (vat), VAT_TEX4_FMT (vat), VAT_TEX4_SHFT (vat),
											 VAT_TEX5_CNT (vat), VAT_TEX5_FMT (vat), VAT_TEX5_SHFT (vat),
											 VAT_TEX6_CNT (vat), VAT_TEX6_FMT (vat), VAT_TEX6_SHFT (vat),
											 VAT_TEX7_CNT (vat), VAT_TEX7_FMT (vat), VAT_TEX7_SHFT (vat));

	DEBUG (EVENT_LOG_GX, "....  ARRAY POS %.8x|%d COL %.8x|%d TEX %.8x|%d",
											 CP_VERTEX_ARRAY_BASE | 0x80000000, CP_VERTEX_ARRAY_STRIDE,
											 CP_COLOR0_ARRAY_BASE | 0x80000000, CP_COLOR0_ARRAY_STRIDE,
											 CP_TEXTURE_ARRAY_BASE (0) | 0x80000000, CP_TEXTURE_ARRAY_STRIDE (0));

	DEBUG (EVENT_LOG_GX, "....  HOST COLORS %d NORMALS %d TEXTURES %d",
											 HOST_COLORS, HOST_NORMALS, HOST_TEXTURES);

	if (gxswitches.dont_draw)
	{
		gcube_perf_vertices (n);
		return list_size * n;
	}

	// normal (faked)
#if DO_TRANSFORM
	vdata[nv] = gxnormal_tf[GXIDX (NORMAL, vat)];
#else
	vdata[nv] = gxnormal[GXIDX (NORMAL, vat)];
#endif
	if (vdata[nv])
	{
		nv++;
		
		switch (VAT_NORMAL_FMT (vat))
		{
			case 1:
				ndq = 1.0 / (1 << 6);
				break;
			
			case 3:
				ndq = 1.0 / (1 << 14);
				break;
			
			default:
				ndq = 1;
		}
	}

	if (gxswitches.wireframe || gxswitches.tex_transparent)
	{
		// color 0
		vdata[nv] = gxcol_fake[GXIDX (COL0, vat)];
		if (vdata[nv])
			nv++;

		// color 1
		vdata[nv] = gxcol_fake[GXIDX (COL1, vat)];
		if (vdata[nv])
			nv++;
	}
	else
	{
		ncolors = 0;

		// color 0
#if DO_TRANSFORM
		vdata[nv] = gxcol0_tf[GXIDX (COL0, vat)];
#else
		vdata[nv] = gxcol[GXIDX (COL0, vat)];
#endif
		if (vdata[nv])
		{
			nv++;
			ncolors++;
		}

		// color 1
#if DO_TRANSFORM
		vdata[nv] = gxcol1_tf[GXIDX (COL1, vat)];
#else
		vdata[nv] = gxcol_fake[GXIDX (COL1, vat)];
#endif
		if (vdata[nv])
		{
			nv++;
			ncolors++;
		}
	}

	ntextures = 0;
#if DO_TRANSFORM
	if (HOST_TEXTURES || XF_TEXTURES)
#else
	if (HOST_TEXTURES)
#endif
	{
		if (gxswitches.wireframe)
		{
			// texture 0
			vdata[nv] = gxtex_fake[GXIDX (TEX0, vat)];
			if (vdata[nv])
				nv++;
		}
		else
		{
			// texture 0
#if DO_TRANSFORM
				vdata[nv] = gxtex_tf[GXIDX (TEX0, vat)];
#else
			if (VCD_T0MIDX)
				vdata[nv] = gxtex_midx[GXIDX (TEX0, vat)];
			else
				vdata[nv] = gxtex[GXIDX (TEX0, vat)];
#endif
			if (vdata[nv])
			{
				nv++;
				ntextures++;
			}
		}

		// other textures are faked
		// texture 1
		vdata[nv] = gxtex_fake[GXIDX (TEX1, vat)];
		if (vdata[nv])
			nv++;

		// texture 2
		vdata[nv] = gxtex_fake[GXIDX (TEX2, vat)];
		if (vdata[nv])
			nv++;

		// texture 3
		vdata[nv] = gxtex_fake[GXIDX (TEX3, vat)];
		if (vdata[nv])
			nv++;

		// texture 4
		vdata[nv] = gxtex_fake[GXIDX (TEX4, vat)];
		if (vdata[nv])
			nv++;

		// texture 5
		vdata[nv] = gxtex_fake[GXIDX (TEX5, vat)];
		if (vdata[nv])
			nv++;

		// texture 6
		vdata[nv] = gxtex_fake[GXIDX (TEX6, vat)];
		if (vdata[nv])
			nv++;

		// texture 7
		vdata[nv] = gxtex_fake[GXIDX (TEX7, vat)];
		if (vdata[nv])
			nv++;

		if (gxswitches.wireframe)
			gx_enable_texture (0, 0);
		else
		{
			gx_load_texture (0);
		}

		if (texactive[0])
		{
			DEBUG (EVENT_LOG_GX, "....  TEXTURE %.8x %dx%d",
						 texactive[0]->address, texactive[0]->width, texactive[0]->height);

			glMatrixMode (GL_TEXTURE);
	
			// dequantize
			tdq[0] = 1.0 / (1 << VAT_TEX0_SHFT (vat));
#if !DO_TRANSFORM
			if (VCD_T0MIDX)
			{
				glLoadIdentity ();
				if (!texactive[0]->p2)
					glScalef (TEX_SSIZE (0), TEX_TSIZE (0), 1);
			}
			else
			{
				gx_load_texture_matrix (MIDX_TEX0, texactive[0]->p2);

				if (texactive[0]->p2)
					glScalef (tdq[0], tdq[0], tdq[0]);
				else
					glScalef (tdq[0] * TEX_SSIZE (0), tdq[0] * TEX_TSIZE (0), tdq[0]);
			}

			if (texactive[0] == &tag_render_target)
			{
				// need to flip the render target
				if (tag_render_target.xfb_mipmap)
				{
					glTranslatef (0.5 / tdq[0] , 0.5 / tdq[0], 0);
					glScalef (2, -2, 1);
					glTranslatef (-0.5 * 0.5 / tdq[0] , -0.5 * 1.5 / tdq[0], 0);
				}
				else
				{
					glTranslatef (0.5 / tdq[0] , 0.5 / tdq[0], 0);
					glScalef (1, -1, 1);
					glTranslatef (-0.5 / tdq[0] , -0.5 / tdq[0], 0);
				}
			}
#else
			glLoadIdentity ();

			if (!texactive[0]->p2)
				glScalef (TEX_SSIZE (0), TEX_TSIZE (0), 1);

			if (texactive[0] == &tag_render_target)
			{
				// need to flip the render target
				if (tag_render_target.xfb_mipmap)
				{
					glTranslatef (0.5 , 0.5, 0);
					glScalef (2, -2, 1);
					glTranslatef (-0.5 * 0.5 , -0.5 * 1.5, 0);
				}
				else
				{
					glTranslatef (0.5 , 0.5, 0);
					glScalef (1, -1, 1);
					glTranslatef (-0.5 , -0.5, 0);
				}
			}
#endif

			glMatrixMode (GL_MODELVIEW);
		}
		else
			gx_enable_texture (0, 0);
	}
	else if (texactive[0])
		gx_enable_texture (0, 0);

	// position
	i = GXIDX (POS, vat);
	psize = gxpos_size[i];
#if DO_TRANSFORM
	pos = gxpos_tf[i];
#else
	pos = gxpos[i];
#endif

	// load geometry matrix
	vdq = 1.0 / (1 << VAT_POS_SHFT (vat));
#if !DO_TRANSFORM
	if (VCD_PMIDX)
	{
		pos = gxpos_midx[GXIDX (POS, vat)];
		glLoadIdentity ();
	}
	else
	{
		gx_load_geometry_matrix (MIDX_GEO);
		glScalef (vdq, vdq, vdq);
	}
#else
	glLoadIdentity ();
#endif

	if (gxswitches.wireframe)
		glColor4f (0, 0, 0, 1);
	else
	{
		if (gxswitches.tex_transparent)
			glColor4f (1, 1, 1, 0.5);
		else if (gxswitches.sw_fullbright)
			glColor4f (1, 1, 1, 1);
		else
		{
			if (XF_COLORS)
			{
				color = BSWAP32 (XF_MATERIAL0);

				if (XF (0x100e) & 1)
					color |= 0xffffff00;
			
				if (XF (0x1010) & 1)
					color |= 0xff;

				glColor4ubv ((unsigned char *) &color);
			}
			else	
				glColor4f (1, 1, 1, 1);
		}
	}

#if DO_TRANSFORM
	cvertex = 0;
	cv = varray;

	color_u32_to_float (ambient[0], XF_AMBIENT0);
	color_u32_to_float (ambient[1], XF_AMBIENT1);
	color_u32_to_float (material[0], XF_MATERIAL0);
	color_u32_to_float (material[1], XF_MATERIAL1);

	pn_matrix_index = MIDX_GEO;
	tex_matrix_index[0] = MIDX_TEX0;
	
	if (!XF_COLORS)
		glColor4f (1, 1, 1, 1);
#endif

	glBegin (prim);

		for (j = 0; j < n; j++)
		{
			// matrix indices
			for (i = 0; i < nm; i++)
				mem += midx[i] (mem);

			// position must be sent last
			mp = mem;
			mem += psize;

			// normals, colors and texture coords
			for (i = 0; i < nv; i++)
				mem += vdata[i] (mem);

			// position
			pos (mp);
#if DO_TRANSFORM			
			// transform and send
			gx_send_current_vertex ();
			cv = &varray[++cvertex];
#endif
		}

	glEnd ();

	gcube_perf_vertices (n);

	if (list_size * n != (mem - start))
		DEBUG (EVENT_STOP, ".gx: incorrect calculation of list_size");

	return (mem - start);
}


unsigned int gx_draw (__u32 mem, int prim, int n, int vat)
{
	if (gxswitches.new_engine)
		return gx_draw_new (mem, prim, n, vat);
	else
		return gx_draw_old (mem, prim, n, vat);
}

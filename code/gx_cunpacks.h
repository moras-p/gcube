#ifndef __GX_CUNPACKS_H
#define __GX_CUNPACKS_H 1


#include "general.h"

#ifdef LIL_ENDIAN
# define MASK_ALPHA		0xff000000
#else
# define MASK_ALPHA		0xff
#endif

#ifndef __cplusplus

inline __u32 color_unpack_rgb565 (__u32 X);
inline __u32 color_unpack_rgba4 (__u32 X);
inline __u32 color_unpack_rgba6 (__u32 X);
inline __u32 color_unpack_rgb555 (__u32 X);
inline __u32 color_unpack_rgb4a3 (__u32 X);
inline __u32 color_unpack_rgb5a3 (__u32 X);
inline __u32 color_unpack_i4 (__u32 X);
inline __u32 color_unpack_i8 (__u32 X);
inline __u32 color_unpack_ia4 (__u32 X);
inline __u32 color_unpack_ia8 (__u32 X);

#else

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

#endif // __cplusplus

#endif // __GX_CUNPACKS_H

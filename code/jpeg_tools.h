#ifndef __JPEG_TOOLS_H
#define __JPEG_TOOLS_H 1


#include "general.h"


void jpeg_decompress (__u8 *src, __u32 size, char *dst, int pitch);
void jpeg_decompress_start (__u8 *src, __u32 size);
void jpeg_decompress_continue (__u8 *src, __u32 size, char *dst, int pitch, int color_space);

#endif // __JPEG_TOOLS_H

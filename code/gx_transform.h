#ifndef __GX_TRANSFORM_H
#define __GX_TRANSFORM_H 1



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

unsigned int gx_draw (__u32 mem, int prim, int n, int vat);


#endif // __GX_TRANSFORM_H

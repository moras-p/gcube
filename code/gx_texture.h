#ifndef __GX_TEXTURE_H
#define __GX_TEXTURE_H 1


#define TEXCACHE_MAGIC				BSWAP32 (0xdeadbeef)
#define XFB_MAGIC							BSWAP32 (0xbabe1ee5)
#define MAX_TEXCACHE_TAGS				0x1000


typedef struct
{
	unsigned int format, type, internal_format;
} TexFormat;


typedef struct
{
	__u32 address;
	__u32 tlut_address;
	unsigned int tex;
	unsigned int type;

	unsigned int p2;
	int reload;
	
	int width, height;
	int format;
	int tlut_format;
	
	__u32 marker_address;
	__u32 marker_save;

	__u32 misses;
	unsigned int size;
	
	int mipmap;
	int mipmaps_loaded;
	__u32 even_lod;
	
	// for render targets
	int xfb_mipmap;
	int dont_use;
} TextureTag;


typedef struct
{
	TextureTag tags[MAX_TEXCACHE_TAGS];
	unsigned int ntags;

	__u32 memory_used;
} TextureCache;


void gx_convert_texture_i4_rgba (__u8 *src, int width, int height, __u16 *tlut, int format, TexFormat *tf);
void gx_convert_texture_i4 (__u8 *src, int width, int height, __u16 *tlut, int format, TexFormat *tf);
void gx_convert_texture_i8_rgba (__u8 *src, int width, int height, __u16 *tlut, int format, TexFormat *tf);
void gx_convert_texture_i8 (__u8 *src, int width, int height, __u16 *tlut, int format, TexFormat *tf);
void gx_convert_texture_ia4_rgba (__u8 *src, int width, int height, __u16 *tlut, int format, TexFormat *tf);
void gx_convert_texture_ia4 (__u8 *src, int width, int height, __u16 *tlut, int format, TexFormat *tf);
void gx_convert_texture_ia8_rgba (__u8 *data, int width, int height, __u16 *tlut, int format, TexFormat *tf);
void gx_convert_texture_ia8 (__u8 *data, int width, int height, __u16 *tlut, int format, TexFormat *tf);
void gx_convert_texture_rgb565_rgba (__u8 *data, int width, int height, __u16 *tlut, int format, TexFormat *tf);
void gx_convert_texture_rgb565 (__u8 *data, int width, int height, __u16 *tlut, int format, TexFormat *tf);
void gx_convert_texture_rgb5a3_rgba (__u8 *data, int width, int height, __u16 *tlut, int format, TexFormat *tf);
void gx_convert_texture_rgba8_rgba (__u8 *data, int width, int height, __u16 *tlut, int format, TexFormat *tf);
void gx_convert_texture_ci4_rgba (__u8 *src, int width, int height, __u16 *tlut, int format, TexFormat *tf);
void gx_convert_texture_ci8_rgba (__u8 *src, int width, int height, __u16 *tlut, int format, TexFormat *tf);
void gx_convert_texture_ci14x2_rgba (__u8 *data, int width, int height, __u16 *tlut, int format, TexFormat *tf);
void gx_convert_texture_cmp_rgba (__u8 *data, int width, int height, __u16 *tlut, int format, TexFormat *tf);
void gx_convert_texture_i4_rgba (__u8 *src, int width, int height, __u16 *tlut, int format, TexFormat *tf);
char *gx_convert_texture (__u8 *src, int width, int height, int format, __s16 *tlut, int tlut_format);

__u32 gl_texture_calculate_size (__u32 npixels, unsigned int gl_internal_format);
__u32 gx_texture_calculate_size (__u32 width, __u32 height, unsigned int format);

void texcache_tlut_reload (__u32 tlut_address);
int texcache_tag_valid (TextureTag *tag);
void texcache_tag_validate_fast (TextureTag *tag);
void texcache_tag_validate (TextureTag *tag);
void texcache_tag_invalidate (TextureTag *tag);
void texcache_invalidate_all (void);
void texcache_remove_tag (TextureTag *tag);
void texcache_remove_all (void);
void texcache_remove_unused (void);
TextureTag *texcache_add_tag (__u32 address, __u32 tlut_address,
		unsigned int tex, unsigned int type, int width, int height,
		int format, int tlut_format, unsigned int gl_internal_format,
		int mipmap, int min_lod, int max_lod,	__u32 even_lod);
TextureTag *texcache_fetch (__u32 address, int width, int height);

void gx_enable_texture (unsigned int index, int enable);
void gx_load_texture (unsigned int index);
void gx_dump_active_texture (int index, int lod);
void gx_create_render_target (void);
void gx_render_to_texture (__u32 address, unsigned int x, unsigned int y,
													 unsigned int w, unsigned int h, int mipmap);


#endif // __GX_TEXTURE_H

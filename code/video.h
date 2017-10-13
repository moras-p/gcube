#ifndef __VIDEO_H
#define __VIDEO_H 1


#include <stdlib.h>
#include <string.h>

#include "cg.h"
#include "mem.h"
#include "hw_si.h"
#include "audio.h"
#include "config.h"


// GL_TEXTURE_RECTANGLE_EXT / NV
#define GL_TEXTURE_RECT 		0x84f5
//#define GL_TEXTURE_RECT 	GL_TEXTURE_2D
#define VID_BPP						32

#define VIDEO_WIDTH				(ewidth ? ewidth : 640)
#define VIDEO_HEIGHT			(eheight ? eheight : 480)

extern int screen_width, screen_height;


int video_init (int w, int h);
void video_close (void);
void video_refresh (void);
void video_refresh_nofb (void);
void video_set_framebuffer (unsigned char *addr);
int video_dump_texture (char *filename, char *data, int w, int h);
void video_set_gamma (float g);
void video_fb_reinit (void);

int input_gamepads_connected (void);
void video_set_fullscreen (int fs);
void video_input_check (void);

int save_tga (char *filename, __u32 *data, int w, int h, int invert, int convert);
int save_tga_fast (char *filename, __u8 *data, int w, int h);


#endif // __VIDEO_H

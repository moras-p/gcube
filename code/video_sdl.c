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
 *
 *      
 *         
 */

#include <SDL/SDL_opengl.h>
#include <SDL/SDL.h>
#include "gl_ext.h"

#include "video.h"


SDL_Surface *offscreen = NULL;
unsigned int fbtex = 0, fbdl = 0;

SDL_Surface *screen = NULL;
SDL_Overlay *yuv = NULL;
SDL_Rect r;
SDL_Joystick *gamepad[4] = {0};
int ngamepads = 0;
char window_title[256] = {0};

unsigned int display_mode = 0;

int disable_fb = FALSE;
int fb_set = FALSE;
int fullscreen = FALSE;
int catch_frames = FALSE;
int save_screenshot = FALSE;
int screen_width = 0, screen_height = 0;
extern int use_textures, use_colors;
int fsaa = 0;

// buffer for screenshots
__u32 sbuff[640 * 528];

#include "icon.c"
void video_set_icon (void)
{
	SDL_Surface *ic;


	ic = SDL_CreateRGBSurfaceFrom ((void *)icon.pixel_data, icon.width,
				icon.height, icon.bytes_per_pixel*8, icon.width * icon.bytes_per_pixel,
				0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
											
	SDL_WM_SetIcon (ic, NULL);
}


int video_event_filter (const SDL_Event *event)
{
	switch (event->type)
	{
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			return FALSE;

		default:
			return TRUE;
	}
}


int video_init_fb (int w, int h)
{
	if (offscreen)
		SDL_FreeSurface (offscreen);
	offscreen = SDL_CreateRGBSurface (SDL_HWSURFACE, w, h, VID_BPP,
																		0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
	if (!offscreen)
	{
		fprintf (stderr, "Couldn't create offscreen surface: %s\n", SDL_GetError ());
		return FALSE;
	}

	if (yuv)
	{
		void *pixels = yuv->pixels[0];

		SDL_FreeYUVOverlay (yuv);
		yuv = SDL_CreateYUVOverlay (w, h, SDL_YUY2_OVERLAY, offscreen);
		yuv->pixels[0] = pixels;
	}
	else
		yuv = SDL_CreateYUVOverlay (w, h, SDL_YUY2_OVERLAY, offscreen);

	if (!yuv)
	{
		fprintf (stderr, "Couldn't create yuv overlay: %s\n", SDL_GetError ());
		return FALSE;
	}

	if (GL_TEXTURE_RECT == GL_TEXTURE_2D)
	{
		glMatrixMode (GL_TEXTURE);
		glLoadIdentity ();
		glScalef (1.0/w, 1.0/h, 1);
	}
	
	if (fbtex)
		glDeleteTextures (1, &fbtex);
	glGenTextures (1, &fbtex);
	glBindTexture (GL_TEXTURE_RECT, fbtex);
	glTexParameteri (GL_TEXTURE_RECT, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri (GL_TEXTURE_RECT, GL_TEXTURE_WRAP_T, GL_CLAMP);
	// those two are necessery
	glTexParameteri (GL_TEXTURE_RECT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_RECT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D (GL_TEXTURE_RECT, 0, GL_RGB, offscreen->w, offscreen->h, 0, GL_RGB,
								GL_UNSIGNED_BYTE, offscreen->pixels);

	if (fbdl)
		glDeleteLists (fbdl, 1);
	fbdl = glGenLists (1);
	glNewList (fbdl, GL_COMPILE);

		glEnable (GL_TEXTURE_RECT);
		glBegin (GL_QUADS);
			glTexCoord2f (0, h);
			glVertex2f (0, h);

			glTexCoord2f (w, h);
			glVertex2f (w, h);

			glTexCoord2f (w, 0);
			glVertex2f (w, 0);

			glTexCoord2f (0, 0);
			glVertex2f (0, 0);
		glEnd ();
		glDisable (GL_TEXTURE_RECT);

	glEndList ();

	r.x = r.y = 0;
	r.w = w;
	r.h = h;
	
	return TRUE;
}


// reinitialize if screen is not NULL
int video_init (int w, int h)
{
	float max_anisotropy = 1;
	int max_tex_units;


	if ((screen && ((screen->w == w) && (screen->h == h))))
		return FALSE;

	if (!screen)
	{
		if (0 > SDL_Init (SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO))
		{
			fprintf (stderr, "SDL initialization failed: %s\n", SDL_GetError ());
			return FALSE;
		}
	}

	SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute (SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);

	if (fsaa)
	{
		SDL_GL_SetAttribute (SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute (SDL_GL_MULTISAMPLESAMPLES, fsaa);
	}

	video_set_icon ();
	if (fullscreen)
		screen = SDL_SetVideoMode (w, h, VID_BPP, SDL_HWSURFACE | SDL_OPENGL | SDL_FULLSCREEN);
	else
		screen = SDL_SetVideoMode (w, h, VID_BPP, SDL_HWSURFACE | SDL_OPENGL);
	if (!screen)
	{
		fprintf (stderr, "SDL_SetVideoMode failed: %s\n", SDL_GetError ());
		return FALSE;
	}
	SDL_ShowCursor (!fullscreen);

	// check for extensions
	if (!strstr (glGetString (GL_EXTENSIONS), "_texture_rectangle"))
		gcube_quit ("Textured rectangle extension is not supported by Your video card!");

#ifdef WINDOWS
	gl_load_ext ();
#endif

	glGetFloatv (GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy);
	gx_set_max_anisotropy (max_anisotropy);

	glGetIntegerv (GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &max_tex_units);

	cg_init ();

	if (fsaa)
		glEnable (GL_MULTISAMPLE_ARB);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	glViewport (0, 0, w, h);
	glOrtho (0, w, h, 0, -1, 1);
	glClearColor (0, 0, 0, 0);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glFrontFace (GL_CW);
	glDisable (GL_CULL_FACE);
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

	glDepthFunc (GL_LEQUAL);
	glEnable (GL_DEPTH_TEST);

	if (!disable_fb)
		if (!video_init_fb (w, h))
			return FALSE;

	DEBUG (EVENT_INFO, ".input: %d gamepad(s) connected", SDL_NumJoysticks ());
	if (SDL_NumJoysticks ())
	{
		int i;
	
		for (i = 0; i < 4; i++)
			if (gamepad[i])
				SDL_JoystickClose (gamepad[i]);

		SDL_JoystickEventState (SDL_ENABLE);

		ngamepads = SDL_NumJoysticks ();
		if (ngamepads > 4)
			ngamepads = 4;		

		for (i = 0; i < ngamepads; i++)
		{
			gamepad[i] = SDL_JoystickOpen (i);
		
			DEBUG (EVENT_INFO, "        [%s] with %d buttons, %d axes, %d hats and %d balls",
							SDL_JoystickName (i), SDL_JoystickNumButtons (gamepad[i]),
							SDL_JoystickNumAxes (gamepad[i]), SDL_JoystickNumHats (gamepad[i]),
							SDL_JoystickNumBalls (gamepad[i]));
		}
	}
	
	SDL_WM_SetCaption (GCUBE_DESCRIPTION, NULL);
	SDL_SetEventFilter (video_event_filter);

	audio_init (FREQ_32KHZ, CONFIG_AUDIO_BUFFER_SIZE);

	screen_width = w;
	screen_height = h;

	return TRUE;
}


void video_set_title (char *title)
{
	char buff[256];


	sprintf (buff, "%s %s", GCUBE_DESCRIPTION, title);
	SDL_WM_SetCaption (buff, NULL);
}


void video_close (void)
{
	int i;
	

	if (!screen)
		return;

	if (yuv)
	{
		SDL_FreeYUVOverlay (yuv);
		yuv = NULL;
	}

	for (i = 0; i < 4; i++)
		if (gamepad[i])
		{
			SDL_JoystickClose (gamepad[i]);
			gamepad[i] = NULL;
		}

	glDeleteTextures (1, &fbtex);
	glDeleteLists (fbdl, 1);

	screen = NULL;

	cg_cleanup ();

	SDL_Quit ();
}


void video_save_screenshot (void)
{
	char filename[1024], path[1024] = ".";
	unsigned int i = 0;


//	if (!path_writeable (path))
		sprintf (path, "%s/screenshots", get_home_dir ());

	while (TRUE)
	{
		sprintf (filename, "%s/screenshot-%.3d.tga", path, i);
		if (!file_exists (filename))
			break;

		i++;
	}

	glReadPixels (0, 0, screen->w, screen->h, GL_BGR, GL_UNSIGNED_BYTE, sbuff);
	if (save_tga_fast (filename, (__u8 *) sbuff, screen->w, screen->h))
		printf ("%s written\n", filename);
	else
		printf ("couldn't write %s\n", filename);
	
	save_screenshot = FALSE;
}


void video_save_frame (unsigned int n)
{
	char filename[1024];


	sprintf (filename, "%s/videos/frame-%.5d.tga", get_home_dir (), n);

//	glPixelZoom (1, -1);
	glReadPixels (0, 0, screen->w, screen->h, GL_BGR, GL_UNSIGNED_BYTE, sbuff);
	glPixelZoom (1, 1);
	if (save_tga_fast (filename, (__u8 *) sbuff, screen->w, screen->h))
		printf ("%s written\n", filename);
	else
		printf ("couldn't write %s\n", filename);
}


void video_fb_reinit (void)
{
	if (disable_fb)
		return;

	glViewport (0, 0, yuv->w, yuv->h);
	glDepthRange (0, 1);

	glClearDepth (1);
	glClearColor (0, 0, 0, 0);
	
	glDisable (GL_DEPTH_TEST);
	glDisable (GL_SCISSOR_TEST);
	glDisable (GL_BLEND);
	glDisable (GL_COLOR_LOGIC_OP);
	glDisable (GL_CULL_FACE);
	
	glDepthMask (GL_TRUE);
	glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	glOrtho (0, yuv->w, yuv->h, 0, -1, 1);
}


float count_fps (void)
{
	static int swaps = 0, last = 0;
	static float fps = 0;
	int t = SDL_GetTicks ();

	
	swaps++;
		
	if (t - last > 1000)
	{
		fps = (float) swaps / (0.001 * (t - last));

		swaps = 0;
		last = t;
	}
	
	return fps;	
}


void video_draw_fb (void)
{
	if (!yuv)
		return;

	if (SDL_DisplayYUVOverlay (yuv, &r))
		printf ("couldn't display YUV overlay: %s\n", SDL_GetError ());

	glBindTexture (GL_TEXTURE_RECT, fbtex);
	glTexSubImage2D (GL_TEXTURE_RECT, 0, 0, 0, offscreen->w, offscreen->h,
									 GL_BGR, GL_UNSIGNED_BYTE, offscreen->pixels);

	glCallList (fbdl);
}


void video_draw (void)
{
	char buff[256];


	SDL_GL_SwapBuffers ();
	if (catch_frames)
		video_save_frame (catch_frames++);
	if (save_screenshot)
		video_save_screenshot ();

	sprintf (buff, "fps: %.2f", count_fps ());
	video_set_title (buff);
}


void video_draw_efb (void)
{
	save_tga ("efbdump.tga", (__u32 *) EFB, EFB_WIDTH, screen_height, FALSE, FALSE);
	glPixelZoom (1, (float) -EFB_HEIGHT / screen_height);
#ifndef NO_GL_EXT
	glWindowPos2i (0, screen_height);
#endif
	glDrawPixels (EFB_WIDTH, EFB_HEIGHT, GL_BGRA, GL_UNSIGNED_BYTE, (void *) EFB);
	SDL_GL_SwapBuffers ();
	glPixelZoom (1, 1);
}


void input_check (void)
{
	SDL_Event event;
	Configuration *config = config_get ();
	static const int buttons[] =
	{
		BUTTON_START, BUTTON_A, BUTTON_B, BUTTON_L, BUTTON_R,
		BUTTON_X, BUTTON_Y, BUTTON_Z, BUTTON_UP, BUTTON_DOWN,
		BUTTON_LEFT, BUTTON_RIGHT, ACTION_AXIS0M, ACTION_AXIS0P,
		ACTION_AXIS1M, ACTION_AXIS1P, ACTION_AXIS2M, ACTION_AXIS2P,
		ACTION_AXIS3M, ACTION_AXIS3P,
	};


	if (SDL_PollEvent (&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				gcube_quit ("quit");
				break;

			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
					// fix for games recreating textures on the fly
					// (snk vs capcom)
					case SDLK_F1:
						gx_switch (GX_TOGGLE_TEX_RELOAD);
						break;
				
					case SDLK_F2:
						gx_switch (GX_TOGGLE_WIREFRAME);
						break;

					// switches between new and old rendering engine
					case SDLK_F3:
						gx_switch (GX_TOGGLE_ENGINE);
						break;
					
					// enable / disable fog (not yet correct)
					case SDLK_F4:
						gx_switch (GX_TOGGLE_FOG);
						break;

					// fix for nintendo puzzle collection
					// (the puzzle choice isn't visible without this)
					case SDLK_F5:
						gx_switch (GX_TOGGLE_TEX_TRANSPARENT);
						break;

					// speedup for cards with slow logic ops
					// (aggressive inline works faster with no difference in
					//  visual quality)
					case SDLK_F6:
						gx_switch (GX_TOGGLE_NO_LOGIC_OPS);
						break;

					// switches between mipmaps generated by the emulated program
					// and opengl generated mipmaps
					case SDLK_F7:
						gx_switch (GX_TOGGLE_USE_GL_MIPMAPS);
						break;

					case SDLK_F8:
						catch_frames = !catch_frames;
						break;

					case SDLK_F9:
						save_screenshot = TRUE;
						break;

					case SDLK_F10:
						fullscreen = !fullscreen;
						SDL_WM_ToggleFullScreen (screen);
						SDL_ShowCursor (!fullscreen);
						break;

					case SDLK_F11:
						gcube_save_state ();
						break;

					case SDLK_F12:
						gcube_load_state ();
						break;

					case SDLK_c:
						if (event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL))
							DEBUG (EVENT_STOP, ".video: user interrupt");
						break;

					case SDLK_BACKSPACE:
						gx_switch (GX_TOGGLE_DRAW);
						break;

					case SDLK_BACKQUOTE:
						gxswitches.use_shaders = !gxswitches.use_shaders;
						cg_enable (gxswitches.use_shaders);
						break;

					case SDLK_1:
						cg_fpcache_clear ();
						break;
						
					case SDLK_2:
						texcache_remove_all ();
						texcache_rt_remove_all ();
						break;

					case SDLK_BACKSLASH:
						video_draw_efb ();
						break;

					default:
						break;
				}
				// fall through
				
			case SDL_KEYUP:
				switch (event.key.keysym.sym)
				{
					case SDLK_ESCAPE:
						gcube_quit ("quit");
						break;

					case SDLK_BACKSPACE:
						if (event.type == SDL_KEYUP)
							gx_switch (GX_TOGGLE_DRAW);
						break;

					default:
						{
							int i, j;
						
							for (j = 0; j < 4; j++)
								if (config->keyboard[j])
									for (i = 0; i < sizeof (buttons) / sizeof (*buttons); i++)
										if (event.key.keysym.sym == config->kbmap[j][i])
										{
											pad_action (config->keyboard[j] - 1,
																	(event.type == SDL_KEYDOWN), buttons[i]);
										}
						}
						break;
				}
				break;

			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				{
					int i;
						
					if (config->gamepad[event.jbutton.which])
					{
						// only 8 buttons for gamepad (no action_axis)
						for (i = 0; i < 8; i++)
							if (config->padbmap[event.jbutton.which][i] == event.jbutton.button + 1)
								pad_action (config->gamepad[event.jbutton.which] - 1,
														(event.type == SDL_JOYBUTTONDOWN), buttons[i]);
					}
				}
				break;
				
			case SDL_JOYHATMOTION:
				{
					unsigned int buttons = 0;

					if (!config->gamepad[event.jbutton.which])
						break;

					if (event.jhat.value & SDL_HAT_UP)
						buttons |= BUTTON_UP;

					if (event.jhat.value & SDL_HAT_DOWN)
						buttons |= BUTTON_DOWN;

					if (event.jhat.value & SDL_HAT_LEFT)
						buttons |= BUTTON_LEFT;

					if (event.jhat.value & SDL_HAT_RIGHT)
						buttons |= BUTTON_RIGHT;

					pad_action_buttons_masked (config->gamepad[event.jbutton.which] - 1,
																		TRUE,	buttons,
																		BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT);
				}
				break;
				
			case SDL_JOYAXISMOTION:
				if (config->gamepad[event.jaxis.which])
				{
					int i;
				
					for (i = 0; i < 4; i++)
						if (config->padamap[event.jaxis.which][i] == event.jaxis.axis + 1)
							pad_action_axis (config->gamepad[event.jaxis.which] - 1,
															 i, event.jaxis.value >> 8);
				}
				break;
		}
	}
}


void video_refresh (void)
{
	if (screen)
	{
		video_draw ();

		if (fb_set && !disable_fb)
			video_draw_fb ();

		input_check ();
	}
}


void video_refresh_nofb (void)
{
	if (screen)
	{
		video_draw ();
		input_check ();
	}
}


void video_input_check (void)
{
	if (screen)
		input_check ();
}


void video_set_framebuffer (unsigned char *addr)
{
	if (disable_fb)
	{
		if (!screen)
			video_init (VIDEO_WIDTH, VIDEO_HEIGHT);

		return;
	}
	else
	{
		if (!yuv && !video_init (VIDEO_WIDTH, VIDEO_HEIGHT))
			return;
	}

	SDL_LockYUVOverlay (yuv);
	yuv->pixels[0] = addr;
	SDL_UnlockYUVOverlay (yuv);

	fb_set = TRUE;
}


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


int save_tga (char *filename, __u32 *data, int w, int h, int invert, int convert)
{
	TGAHeader hdr;
	const char *id = "Dumped by gcube";
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
		char path[1024];
		
		sprintf (path, "%s/%s", get_home_dir (), filename);
		f = fopen (path, "wb");

		if (!f)		
			return FALSE;
	}

	fwrite (&hdr, 1, sizeof (hdr), f);
	fwrite (id, 1, hdr.idlen, f);

	// ABGR -> ARGB
	if (invert)
	{
		for (i = 0; i < h; i++)
			for (j = 0; j < w; j++)
			{
				if (convert)
					d = (data[i*w + j] & 0xff000000) | BOTH_BSWAP32 (data[i*w + j] << 8);
				else
					d = data[i*w + j];
				d = BIG_BSWAP32 (d);

				fwrite (&d, 1, 4, f);
			}
	}
	else
	{
		for (i = h - 1; i >= 0; i--)
			for (j = 0; j < w; j++)
			{
				if (convert)
					d = (data[i*w + j] & 0xff000000) | BOTH_BSWAP32 (data[i*w + j] << 8);
				else
					d = data[i*w + j];
				d = BIG_BSWAP32 (data[i*w + j]);

				fwrite (&d, 1, 4, f);
			}
	}

	fclose (f);
	return TRUE;
}


// 24 bit
int save_tga_fast (char *filename, __u8 *data, int w, int h)
{
	TGAHeader hdr;
	const char *id = "Dumped by gcube";
	FILE *f;


	memset (&hdr, 0, sizeof (hdr));
	hdr.datatype = 2;							// uncompressed RGB
	hdr.width = BIG_BSWAP16 (w);
	hdr.height = BIG_BSWAP16 (h);
	hdr.bpp = 24;
	hdr.idlen = strlen (id);

	f = fopen (filename, "wb");
	if (!f)
	{
		char path[1024];
		
		sprintf (path, "%s/%s", get_home_dir (), filename);
		f = fopen (path, "wb");

		if (!f)		
			return FALSE;
	}

	fwrite (&hdr, 1, sizeof (hdr), f);
	fwrite (id, 1, hdr.idlen, f);
	fwrite (data, 3, w*h, f);
	fclose (f);

	return TRUE;
}


int video_dump_texture (char *filename, char *data, int w, int h)
{
	return save_tga (filename, (__u32 *) data, w, h, FALSE, TRUE);
}


int input_gamepads_connected (void)
{
	return ngamepads;
}


void video_set_fullscreen (int fs)
{
	if (fs != fullscreen)
	{
		fullscreen = !fullscreen;

		if (screen)
		{
			SDL_WM_ToggleFullScreen (screen);
			SDL_ShowCursor (!fullscreen);
		}
	}
}


void video_set_gamma (float g)
{
	static float last_gamma = 0;


	if (!FLOAT_EQ (g, last_gamma))
	{
//		SDL_SetGamma (g, g, g);
		last_gamma = g;
	}
}

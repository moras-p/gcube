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

float max_anisotropy = 1;



void gx_set_max_anisotropy (float a)
{
	max_anisotropy = a;
}


int gx_get_real_height (void)
{
	// just trying to guess the number of lines
	return (FLOAT_EQ (-XF_VIEWPORT_B * 2, 448) ? 448 : 480);
}


int gx_get_real_width (void)
{
	return (((XF_VIEWPORT_A * 2) < RVI16 (0x2070)) ? RVI16 (0x2070) : (XF_VIEWPORT_A * 2));
}


void gx_set_texture_mode0 (int index)
{
	unsigned int wrap[] =
	{
		GL_CLAMP, GL_REPEAT, GL_MIRRORED_REPEAT_IBM,
	};
	unsigned int minfilter[] =
	{
		GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, 0,
		GL_LINEAR, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR, 0,
	};
	unsigned int magfilter[] =
	{
		GL_NEAREST, GL_LINEAR,
	};
	float maxaniso[] =
	{
		// last entry is unused/reserved, but anyhow
		1, 2, 4, 8,
	};
	float aniso;
	

	DEBUG (EVENT_LOG_GX, "....  TEX WRAP %d/%d FILTER %d/%d ANISO %d LOD BIAS %.1f DIAGLOAD %d CLAMP %d",
				 TEX_MODE_WRAP_S (index), TEX_MODE_WRAP_T (index),
				 TEX_MODE_MAG_FILTER (index), TEX_MODE_MIN_FILTER (index),
				 TEX_MODE_MAX_ANISO (index),
				 ((float) (__s8) TEX_MODE_LOD_BIAS (index)) / 32,
				 TEX_MODE_DIAGLOAD (index), TEX_MODE_LOD_CLAMP (index));

	if (!texactive[index])
		return;

	aniso = maxaniso[TEX_MODE_MAX_ANISO (index)];
	if (aniso > max_anisotropy)
		aniso = max_anisotropy;

	if (TEX_IS_MIPMAPPED (index))
	{
		texactive[index]->mipmap = TRUE;
		if (gxswitches.force_max_aniso)
			aniso = max_anisotropy;
	}
	else
			texactive[index]->mipmap = FALSE;

	glTexEnvf (GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, 
						 ((float) (__s8) TEX_MODE_LOD_BIAS (index)) / 32);

	glTexParameterf (texactive[index]->type,
									 GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);

	if (gxswitches.use_gl_mipmaps && texactive[index]->mipmap)
		glTexParameterf (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	else
		glTexParameterf (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);

	glTexParameteri (texactive[index]->type,
										GL_TEXTURE_WRAP_S, wrap[TEX_MODE_WRAP_S (index)]);
	glTexParameteri (texactive[index]->type,
										GL_TEXTURE_WRAP_T, wrap[TEX_MODE_WRAP_T (index)]);

	if (texactive[index]->xfb_mipmap)
	{
		// apply filter to render target
		glTexParameteri (texactive[index]->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri (texactive[index]->type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		glTexParameteri (texactive[index]->type,
										 GL_TEXTURE_MIN_FILTER, minfilter[TEX_MODE_MIN_FILTER (index)]);
		glTexParameteri (texactive[index]->type,
										 GL_TEXTURE_MAG_FILTER, magfilter[TEX_MODE_MAG_FILTER (index)]);
	}
}


void gx_set_texture_mode1 (int index)
{
	DEBUG (EVENT_LOG_GX, "....  TEX LOD MIN %.1f MAX %.1f",
				 ((float) TEX_MODE_MIN_LOD (index)) / 16,
				 ((float) TEX_MODE_MAX_LOD (index)) / 16);

	if (!texactive[index] || !texactive[index]->p2)
		return;

	glTexParameterf (texactive[index]->type, GL_TEXTURE_MIN_LOD,
									 ((float) TEX_MODE_MIN_LOD (index)) / 16);

	glTexParameterf (texactive[index]->type, GL_TEXTURE_MAX_LOD,
									 ((float) TEX_MODE_MAX_LOD (index)) / 16);

	glTexParameterf (texactive[index]->type, GL_TEXTURE_BASE_LEVEL,
									 ((float) TEX_MODE_MIN_LOD (index)) / 16);

	glTexParameterf (texactive[index]->type, GL_TEXTURE_MAX_LEVEL,
									 ((float) TEX_MODE_MAX_LOD (index)) / 16);
}


void gx_set_projection (void)
{
	float m[16] = {0};


	glMatrixMode (GL_PROJECTION);

	if (XF_PROJECTION_ORTHOGRAPHIC)
	{
		DEBUG (EVENT_LOG_GX, "....  setting orthographic projection matrix");

		m[ 0] = XF_PROJECTION_A;
		m[12] = XF_PROJECTION_B;
		m[ 5] = XF_PROJECTION_C;
		m[13] = XF_PROJECTION_D;
		m[10] = XF_PROJECTION_E;
		m[14] = XF_PROJECTION_F;
		m[15] = 1;

		glLoadMatrixf (m);
	}
	else
	{
		DEBUG (EVENT_LOG_GX, "....  setting perspective projection matrix");

		m[ 0] = XF_PROJECTION_A;
		m[ 8] = XF_PROJECTION_B;
		m[ 5] = XF_PROJECTION_C;
		m[ 9] = XF_PROJECTION_D;
		m[10] = XF_PROJECTION_E;
		m[14] = XF_PROJECTION_F;
		m[11] = -1;
		
		glLoadMatrixf (m);
	}

	glMatrixMode (GL_MODELVIEW);
}


void gx_set_lpsize (void)
{
	DEBUG (EVENT_LOG_GX, "....  SIZE point %d line %d OFFSET point %d line %d", POINT_SIZE, LINE_SIZE, POINT_TEX_OFFSET, LINE_TEX_OFFSET);

	glPointSize ((float) POINT_SIZE / 6);
	glLineWidth ((float) LINE_SIZE / 6);
}


void gx_load_tlut (void)
{
	__u32 size = ((BP_LOADTLUT1 >> 10) & 0x3ff) << 5;
	__u32 src = (BP_LOADTLUT0 & 0x1fffff) << 5;
	__u32 dst = (BP_LOADTLUT1 & 0x3ff) << 5;


	DEBUG (EVENT_LOG_GX, "....  TLUT %d MEM[%.8x] -> TMEM[%.8x]",
				 size, src | 0x80000000, dst);


	if (0 != memcmp (TMEM_ADDRESS (dst), MEM_ADDRESS (src), size))
	{
		memcpy (TMEM_ADDRESS (dst), MEM_ADDRESS (src), size);

		// doesn't always give any visual difference, but slows down considerably.
		if (gxswitches.tex_reload)
			texcache_tlut_reload (dst);
	}
}


void gx_set_cull_mode (void)
{
	unsigned int cull[] =
	{
		// glFrontFace is set to GL_CW at video initialization
		0, GL_BACK, GL_FRONT, GL_FRONT_AND_BACK,
	};


	DEBUG (EVENT_LOG_GX, "....  CULL MODE %d", CULL_MODE);

	if (gxswitches.wireframe)
		return;

	if (CULL_MODE == 0)
		glDisable (GL_CULL_FACE);
	else
	{
		glEnable (GL_CULL_FACE);
		glCullFace (cull[CULL_MODE]);
	}
}


void gx_set_cmode0 (void)
{
	unsigned int sfactor[] =
	{
		GL_ZERO,
		GL_ONE,
		GL_DST_COLOR,
		GL_ONE_MINUS_DST_COLOR,
		GL_SRC_ALPHA,
		GL_ONE_MINUS_SRC_ALPHA,
		GL_DST_ALPHA,
		GL_ONE_MINUS_DST_ALPHA,
	};
	unsigned int dfactor[] =
	{
		GL_ZERO,
		GL_ONE,
		GL_SRC_COLOR,
		GL_ONE_MINUS_SRC_COLOR,
		GL_SRC_ALPHA,
		GL_ONE_MINUS_SRC_ALPHA,
		GL_DST_ALPHA,
		GL_ONE_MINUS_DST_ALPHA,
	};
	// aggressive inline doesn't work well with the proper sfactor/dfactor
	// this is a temporary workaround
	unsigned int sfactor_fix[] =
	{
		GL_ZERO,
		GL_ONE,
		GL_DST_ALPHA,
		GL_ONE_MINUS_DST_ALPHA,
		GL_SRC_ALPHA,
		GL_ONE_MINUS_SRC_ALPHA,
		GL_DST_COLOR,
		GL_ONE_MINUS_DST_COLOR,
	};
	unsigned int dfactor_fix[] =
	{
		GL_ZERO,
		GL_ONE,
		GL_DST_ALPHA,
		GL_ONE_MINUS_DST_ALPHA,
		GL_SRC_ALPHA,
		GL_ONE_MINUS_SRC_ALPHA,
		GL_SRC_COLOR,
		GL_ONE_MINUS_SRC_COLOR,
	};
	unsigned int logicop[] =
	{
		GL_CLEAR,
		GL_AND,
		GL_AND_REVERSE,
		GL_COPY,
		GL_AND_INVERTED,
		GL_NOOP,
		GL_XOR,
		GL_OR,
		GL_NOR,
		GL_EQUIV,
		GL_INVERT,
		GL_OR_REVERSE,
		GL_COPY_INVERTED,
		GL_OR_INVERTED,
		GL_NAND,
		GL_SET,
	};


	DEBUG (EVENT_LOG_GX,"....  CMODE0 ENABLE B%d/D%d/L%d UPDATE C%d/A%d FACTOR S%d/D%d LOGICOP %d",
				 CMODE_BLEND_ENABLED>0, CMODE_DITHER_ENABLED>0, CMODE_LOGICOP_ENABLED>0,
				 CMODE_COLOR_ENABLED>0, CMODE_ALPHA_ENABLED>0,
				 CMODE_SFACTOR, CMODE_DFACTOR, CMODE_LOGICOP);

	if (gxswitches.wireframe)
		return;

	if (CMODE_DITHER_ENABLED)
		glEnable (GL_DITHER);
	else
		glDisable (GL_DITHER);

	if (CMODE_BLEND_ENABLED)
	{
		glEnable (GL_BLEND);
		if (gxswitches.blending_fix)
			glBlendFunc (sfactor_fix[CMODE_SFACTOR], dfactor_fix[CMODE_DFACTOR]);
		else
			glBlendFunc (sfactor[CMODE_SFACTOR], dfactor[CMODE_DFACTOR]);
		// this one needs GL_ARB_imaging
		// lame windows need the extensions to be loaded manually
		// comment this line to compile it
#ifndef NO_GL_EXT
		glBlendEquation (CMODE_SUBTRACT ? GL_FUNC_REVERSE_SUBTRACT : GL_FUNC_ADD);
#endif
	}
	else
		glDisable (GL_BLEND);

	if (CMODE_LOGICOP_ENABLED && !gxswitches.no_logic_ops)
	{
		glEnable (GL_COLOR_LOGIC_OP);
		glLogicOp (logicop[CMODE_LOGICOP]);
	}
	else
		glDisable (GL_COLOR_LOGIC_OP);

	if (CMODE_COLOR_ENABLED)
		glColorMask (GL_TRUE, GL_TRUE, GL_TRUE,
								 CMODE_ALPHA_ENABLED ? GL_TRUE : GL_FALSE);
	else
		glColorMask (GL_FALSE, GL_FALSE, GL_FALSE,
								 CMODE_ALPHA_ENABLED ? GL_TRUE : GL_FALSE);
}


void gx_set_zmode (void)
{
	unsigned int func[] =
	{
		GL_NEVER,
		GL_LESS,
		GL_EQUAL,
		GL_LEQUAL,
		GL_GREATER,
		GL_NOTEQUAL,
		GL_GEQUAL,
		GL_ALWAYS,
	};


	DEBUG (EVENT_LOG_GX, "....  ZMODE ENABLE %d FUNC %d UPDATE %d",
				 ZMODE_ENABLED>0, ZMODE_FUNC, ZMODE_UPDATE_ENABLED>0);

	if (gxswitches.wireframe)
		return;

	if (ZMODE_ENABLED)
	{
		glEnable (GL_DEPTH_TEST);
		glDepthFunc (func[ZMODE_FUNC]);
	}
	else
		glDisable (GL_DEPTH_TEST);
	
	if (ZMODE_UPDATE_ENABLED)
		glDepthMask (GL_TRUE);
	else
		glDepthMask (GL_FALSE);
}


#define GX_NEVER				0
#define GX_ALWAYS				7
#define GX_AOP_AND			0
#define GX_AOP_OR				1
#define GX_AOP_XOR			2
#define GX_AOP_XNOR			3

// todo: use shaders for full emulation (both first and second operation)
void gx_set_alphafunc (void)
{
	unsigned int alphafunc[] =
	{
		GL_NEVER,
		GL_LESS,
		GL_EQUAL,
		GL_LEQUAL,
		GL_GREATER,
		GL_NOTEQUAL,
		GL_GEQUAL,
		GL_ALWAYS,
	};
	int en;


	DEBUG (EVENT_LOG_GX, "....  TEV_ALPHAFUNC OP0 %d/%.3f LOGIC %d OP1 %d/%.3f",
				 TEV_ALPHAFUNC_OP0, (float) TEV_ALPHAFUNC_A0 / 0xff,
				 TEV_ALPHAFUNC_LOGIC,
				 TEV_ALPHAFUNC_OP1, (float) TEV_ALPHAFUNC_A1 / 0xff);

	if (gxswitches.wireframe)
		return;

	switch (TEV_ALPHAFUNC_LOGIC)
	{
		case GX_AOP_AND:
			en = (TEV_ALPHAFUNC_OP0 != GX_ALWAYS) && (TEV_ALPHAFUNC_OP1 == GX_ALWAYS);
			break;
		
		case GX_AOP_OR:
			en = (TEV_ALPHAFUNC_OP0 != GX_ALWAYS) && (TEV_ALPHAFUNC_OP1 == GX_NEVER);
			break;
		
		default:
			en = FALSE;
	}

	if (en)
		glEnable (GL_ALPHA_TEST);
	else
		glDisable (GL_ALPHA_TEST);

	glAlphaFunc (alphafunc[TEV_ALPHAFUNC_OP0], (float) TEV_ALPHAFUNC_A0 / 0xff);
}


void gx_set_fog_param3 (void)
{
/*
	// calculate fog_startz and fog_endz
	if (TEV_FOG_FSEL == GX_FOG_LINEAR)
	{
		glEnable (GL_FOG);

		glFogi (GL_FOG_MODE, GL_LINEAR);
		glFogf (GL_FOG_START, fog_startz);
		glFogf (GL_FOG_END, fog_endz);
	}
	else
		glDisable (GL_FOG);
*/
}


void gx_set_fog_color (void)
{
	float color[4] =
	{
		(float) TEV_FOG_COLOR_R / 0xff,
		(float) TEV_FOG_COLOR_B / 0xff,
		(float) TEV_FOG_COLOR_G / 0xff,
		1,
	};
	
	
	glFogfv (GL_FOG_COLOR, color);
}


void gx_set_copy_clear_color (void)
{
	DEBUG (EVENT_LOG_GX, "....  COPY CLEAR COLOR %f %f %f %f",
				 (float) COPY_CLEAR_R / 0xff,
				 (float) COPY_CLEAR_G / 0xff,
				 (float) COPY_CLEAR_B / 0xff,
				 (float) COPY_CLEAR_A / 0xff);


	if (gxswitches.wireframe)
		return;

	glClearColor ((float) COPY_CLEAR_R / 0xff,
								(float) COPY_CLEAR_G / 0xff,
								(float) COPY_CLEAR_B / 0xff,
								(float) COPY_CLEAR_A / 0xff);
}


void gx_set_copy_clear_z (void)
{
	DEBUG (EVENT_LOG_GX, "....  COPY CLEAR Z %f",
				 (float) COPY_CLEAR_Z / 0xffffff);

	glClearDepth ((float) COPY_CLEAR_Z / 0xffffff);
}

// billy hatcher battle mode seems to be working well with the offsets
// but also needs the viewport set the same as scissor box??
#define BILLY_FIX	0
void gx_set_scissors (void)
{
	float xscale, yscale;
	int x, y, w, h, xofs, yofs;


	x = SCISSORS_LEFT - 342;
	y = SCISSORS_TOP - 342;
	w = SCISSORS_RIGHT - SCISSORS_LEFT;
	h = SCISSORS_BOTTOM - SCISSORS_TOP;
	xofs = (SCISSORS_OFFSET_X << 1) - 342;
	yofs = (SCISSORS_OFFSET_Y << 1) - 342;

	w += 1;
	h += 1;

	DEBUG (EVENT_LOG_GX, "....  SCISSORS %d %d %d %d %d %d", x, y, w, h, xofs, yofs);

	glEnable (GL_SCISSOR_TEST);

#if BILLY_FIX
	x -= xofs;
	y -= yofs;
#endif

	xscale = (float) screen_width / gx_get_real_width ();
	yscale = (float) screen_height / gx_get_real_height ();

	h *= yscale;
	y *= yscale;
	w *= xscale;
	x *= xscale;

#if BILLY_FIX
	glViewport (x, screen_height - (y + h), w, h);
#endif

	glScissor (x, screen_height - (y + h), w, h);
}


void gx_set_viewport (void)
{
	float x, y, w, h, n, f, xscale, yscale;


	w = XF_VIEWPORT_A * 2;
	h = -XF_VIEWPORT_B * 2;
	x = XF_VIEWPORT_D - (342 + XF_VIEWPORT_A);
	y = XF_VIEWPORT_E - (342 - XF_VIEWPORT_B);
	
	f = XF_VIEWPORT_F / 16777215;
	n = (XF_VIEWPORT_F - XF_VIEWPORT_C) / 16777215;

	DEBUG (EVENT_LOG_GX, "....  VIEWPORT %.2f %.2f %.2f %.2f %.2f %.2f",
				 x, y, w, h, n, f);

	xscale = (float) screen_width / gx_get_real_width ();
	yscale = (float) screen_height / gx_get_real_height ();

	// double scale -> egg mania fix
	// yscale *= 2;

	h *= yscale;
	y *= yscale;
	w *= xscale;
	x *= xscale;
	glViewport (x, screen_height - (y + h), w, h);
	glDepthRange (n, f);
}


void gx_set_gamma (void)
{
	static const float rgb_gamma[] = { 1.0, 1.7, 2.2 };


	video_set_gamma (rgb_gamma[COPY_EXECUTE_GAMMA]);
}


void gx_copy_efb (void)
{
	if (COPY_EXECUTE_TO_XFB)
	{
		if (gxswitches.flicker_fix)
		{
			if (CMODE_COLOR_ENABLED)
				gcube_pe_refresh ();
		}
		else
			gcube_pe_refresh ();

		if (COPY_EXECUTE_CLEAR_EFB || gxswitches.wireframe)
		{
			glPushAttrib (GL_VIEWPORT_BIT | GL_SCISSOR_BIT);

			glViewport (0, 0, screen_width, screen_height);
			glScissor (0, 0, screen_width, screen_height);
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			glPopAttrib ();
		}
	}
	else
	{
		float x, y, w, h, xscale, yscale;

		xscale = (float) screen_width / gx_get_real_width ();
		yscale = (float) screen_height / gx_get_real_height ();

		w = xscale * (EFB_SRC_WIDTH + 1);
		h = yscale * (EFB_SRC_HEIGHT + 1);
		x = xscale * EFB_SRC_LEFT;
		y = yscale * EFB_SRC_TOP;

		// efb to texture
		DEBUG (EVENT_LOG_GX, "....  copy color buffer to texture");

		gx_render_to_texture (XFB_ADDRESS, x, y, w, h, COPY_EXECUTE_MIPMAP);

		if (COPY_EXECUTE_CLEAR_EFB)
		{
			glPushAttrib (GL_VIEWPORT_BIT | GL_SCISSOR_BIT);

			// clear copied rectangle
			glViewport (x, screen_height - (y + h), w, h);
			glScissor (x, screen_height - (y + h), w, h);
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glPopAttrib ();
		}
	}
	
	// fix this
	force_refresh ();
}

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
 *
 *         
 */

#include <SDL/SDL_opengl.h>

#include "hw_gx.h"


__s32 (*gpop[0x100]) (__u32 mem);

GXState gxs;
GXSwitches gxswitches;
static int ignore_bad_displaylist = TRUE;


// needs 'end' passed to gpopcodes
/*
#ifdef GDEBUG
# define GPOPCODE_SIZE(X)\
	({\
			if ((X) + mem > end)\
			{\
				DEBUG (EVENT_LOG_GX, "....  command needs %d bytes, only %d provided", X, end - mem);\
				return -(X);\
			}\
	})
#else
# define GPOPCODE_SIZE(X)
#endif
*/
# define GPOPCODE_SIZE(X)

int gx_parse_list (__u32 start, __u32 length)
{
	__u32 end = start + length;
	int n;


	DEBUG (EVENT_LOG_GX_IMM, "..gx: parsing list (start = %.8x, length = %.8x)", start | 0x80000000, length);

	while (start < end)
	{
		n = GPOP (MEM (start)) (start);
		if (n < 0)
		{
			n = -n;
			if (!((n == 9) && (start + n < end)))
				return -(start - (end - length));
			// otherwise display list problem, continue
		}

		start += n;
	}

	return (start - (end - length));
}


GPOPCODE (INVALID)
{
	DEBUG (EVENT_EMAJOR, "..gx: invalid command %2.2x at %.8x", MEM (mem), mem | 0x80000000);

	return -1;
}


GPOPCODE (NOP)
{
	GPOPCODE_SIZE (1);

	DEBUG (EVENT_LOG_GX, "....  nop");

	return 1;
}


GPOPCODE (LOAD_CP)
{
	GPOPCODE_SIZE (6);

	DEBUG (EVENT_LOG_GX, "....  cp[%.2x] = %.8x", FIFO_U8 (mem + 1), FIFO_U32 (mem + 2));

	CP (FIFO_U8 (mem + 1)) = FIFO_U32 (mem + 2);

	return 6;
}


GPOPCODE (LOAD_XF)
{
	__u32 n = FIFO_U16 (mem += 1) + 1;
	__u32 a = FIFO_U16 (mem += 2);
	unsigned int i;

//#if 1
	// primary check
	GPOPCODE_SIZE (3);
	// secondary check (with valid n)
	GPOPCODE_SIZE (5 + n*4);
//#else
	if (EXCEEDES_LIST_BOUNDARY (mem, 5 + n*4))
		return -(5 + n*4);
//#endif

	mem += 2;
	if (a < 0x600)
	{
		// load matrix
		for (i = 0; i < n; i++, mem += 4)
		{
			DEBUG (EVENT_LOG_GX, "....  xfmat[%.4x] = %.2f", a + i, FIFO_F (mem));

			XFMAT (a + i) = FIFO_F (mem);
		}
	}
	else if (a < 0x800)
	{
		// light memory
		for (i = 0; i < n; i++, mem += 4)
		{
			DEBUG (EVENT_LOG_GX, "....  xfmat[%.4x] = %.8x", a + i, FIFO_U32 (mem));

			XFMATB (a + i) = FIFO_U32 (mem);
		}
		{
			int num = XFMAT_LIGHT_NUM (a);
			GXLight *light = XF_LIGHT (num);
			
			DEBUG (EVENT_LOG_GX, "....  light %d color %.8x pos (%.2f %.2f %.2f)",
								num, light->color, light->pos[0], light->pos[1], light->pos[2]);
		}
	}
	else
	{
		for (i = 0; i < n; i++, mem += 4)
		{
			DEBUG (EVENT_LOG_GX, "....  xf[%.4x] = %.8x", a + i, FIFO_U32 (mem));

			XF (a + i) = FIFO_U32 (mem);
		}
		
		// set matrices
		switch (a)
		{
			// viewport
			case 0x101a:
			case 0x101b:
			case 0x101d:
			case 0x101e:
				gx_set_viewport ();
				break;

			// projection
			case 0x1020:
				gx_set_projection ();
				break;
		}
	}

	return (5 + n*4);
}


// invalidate vertex cache
GPOPCODE (INVALIDATE_VCACHE)
{
	GPOPCODE_SIZE (1);

	DEBUG (EVENT_LOG_GX, "....  invalidate vertex cache");

	return 1;
}


GPOPCODE (LOAD_BP)
{
	__u32 mask = BP_MASK;
	__u8 index = FIFO_U8 (mem + 1);


	GPOPCODE_SIZE (5);

	DEBUG (EVENT_LOG_GX, "....  bp[%.2x] = %.6x", index, FIFO_U32 (mem + 1) & 0x00ffffff);

	// reset mask
//	if (BP_MASK != 0x00ffffff)
		BP_MASK = 0x00ffffff;

	BP (index) = (BP (index) &~ mask) | (FIFO_U32 (mem + 1) & mask);

	switch (index)
	{
		// GEN_MODE
		case 0x00:
			gx_set_cull_mode ();
			break;
		
		// IND_MTX
		case 0x06:
		case 0x09:
		case 0x0c:
			gx_set_tev_indm_ab ((index - 6) / 3);
			gx_set_tev_indscale ((index - 6) / 3);
			break;

		case 0x07:
		case 0x0a:
		case 0x0d:
			gx_set_tev_indm_cd ((index - 7) / 3);
			gx_set_tev_indscale ((index - 7) / 3);
			break;

		case 0x08:
		case 0x0b:
		case 0x0e:
			gx_set_tev_indm_ef ((index - 8) / 3);
			gx_set_tev_indscale ((index - 8) / 3);
			break;

		case 0x20:
		case 0x21:
			gx_set_scissors ();
			break;

		// point / line size and texture offsets
		case 0x22:
			gx_set_lpsize ();
			break;

		// PE_ZMODE
		case 0x40:
			gx_set_zmode ();
			break;

		// PE_CMODE0
		case 0x41:
			gx_set_cmode0 ();
			break;

		// PE_CMODE1
		// use shaders for constant alpha (??)
		case 0x42:
			break;

		// PE_DONE
		case 0x45:
			DEBUG (EVENT_LOG_GX, "PE_DONE");
			if (BP (0x45) & 1)
				DEBUG (EVENT_STOP, "bit 0 in BP (0x45) set");

			if (BP (0x45) & 2)
				pe_generate_interrupt (PE_INTERRUPT_FINISH);
			break;

		// PE_TOKEN
		case 0x47:
			pe_set_token (BP (0x47) & 0xffff);
			break;
		
		// PE_TOKEN_INT
		case 0x48:
			pe_set_token (BP (0x48) & 0xffff);
			pe_generate_interrupt (PE_INTERRUPT_TOKEN);
			break;

		case 0x4f:
		case 0x50:
			gx_set_copy_clear_color ();
			break;

		case 0x51:
			gx_set_copy_clear_z ();
			break;

		// PE_COPY
		case 0x52:
			gx_set_gamma ();
			gx_copy_efb ();
			break;
		
		// scissorbox offset
		case 0x59:
			gx_set_scissors ();
			break;

		// TX_LOADTLUT1
		case 0x65:
			gx_load_tlut ();
			break;

		// TX_INVALIDATE
		case 0x66:
			DEBUG (EVENT_LOG_GX, "....  invalidate texture cache");
			if (gxswitches.tc_invalidate_enabled)
				texcache_invalidate_all ();
			break;
	
		// TX_SETMODE0
		case 0x80:
		case 0x81:
		case 0x82:
		case 0x83:
			gx_set_texture_mode0 (FIFO_U8 (mem + 1) & 3);
			break;

		// TX_SETMODE1
		case 0x84:
		case 0x85:
		case 0x86:
		case 0x87:
			gx_set_texture_mode1 ((FIFO_U8 (mem + 1) >> 1) & 3);
			break;

		case 0xa0:
		case 0xa1:
		case 0xa2:
		case 0xa3:
			gx_set_texture_mode0 ((FIFO_U8 (mem + 1) & 3) + 4);
			break;

		case 0xa4:
		case 0xa5:
		case 0xa6:
		case 0xa7:
			gx_set_texture_mode1 (((FIFO_U8 (mem + 1) >> 1) & 3) + 4);
			break;

		// TEV_REGL
		case 0xe0:
		case 0xe2:
		case 0xe4:
		case 0xe6:
			gx_set_tev_konst_ra ((index & 7) >> 1);
			break;

		// TEV_REGH
		case 0xe1:
		case 0xe3:
		case 0xe5:
		case 0xe7:
			gx_set_tev_konst_bg ((index & 7) >> 1);
			break;

		case 0xee:
		case 0xef:
		case 0xf0:
		case 0xf1:
			gx_set_fog ();
			break;

		// TEV_FOG_COLOR
		case 0xf2:
			gx_set_fog_color ();
			break;

		// TEV_ALPHAFUNC
		case 0xf3:
			gx_set_alphafunc ();
			break;
		
		// BP MASK
		// MaxPlay Classic Games Volume 1 has a strange version of GXSetCoPlanar
		// other games execute it as two distinct bp register loads:
		// 0x61, 0xfe080000 -> set bp mask register
		// 0x61, (0x00000001 | (zfreeze_enabled << 19)) -> set gen mode
		// but this game uses one bp load to fill 2 registers (?!):
		// 0x61, 0xfe080000, (0x00000001 | (zfreeze_enabled << 19))
		// this will generate invalid gx command, so here's a hack
		case 0xfe:
			{
				__u32 next = FIFO_U32 (mem + 5);

				if ((next &~ 0x00080000) == 0x00000001)
				{
					DEBUG (EVENT_LOG_GX, "..gx: odd GXSetCoPlanar hack");
					BP (0x00) = (BP (0x00) &~ BP_MASK) | (next & BP_MASK);
					BP_MASK = 0x00ffffff;
					
					return 9;
				}
			}
			break;
	}

	return 5;
}


GPOPCODE (LOAD_INDEX)
{
	__u32 t = FIFO_U32 (mem + 1);
	// index = t >> 16;
	__u32 n = ((t >> 12) & 0x0f) + 1;
	__u32 a = t & 0x0fff;
	unsigned int i = ((FIFO_U8 (mem) >> 3) & 3) + 0x0c;		// array index


	GPOPCODE_SIZE (5);

	DEBUG (EVENT_LOG_GX, "....  load xf indexed %c: index %d size %d address %.4x", 'a' + i, t >> 16, n, a);

	mem = CP_ARRAY_BASE (i) + CP_ARRAY_STRIDE (i) * (t >> 16);
	for (i = 0; i < n; i++, mem += 4)
	{
		XFMATB (a + i) = FIFO_U32 (mem);

		DEBUG (EVENT_LOG_GX, "....   xfmat[%.4x] = %.2f", a + i, XFMAT (a + i));
	}

	return 5;
}


GPOPCODE (CALL_DISPLAYLIST)
{
	__u32 addr = FIFO_U32 (mem + 1), size = FIFO_U32 (mem + 5);


	GPOPCODE_SIZE (9);

	DEBUG (EVENT_LOG_GX, "....  call displaylist %.8x of size %.8x from %.8x", addr, size, mem);

	if (((addr & MEM_MASK) > MEM_SIZE) || (addr & 31) || (size & 31))
	{
		DEBUG (EVENT_EFATAL, "..gx: trying to parse invalid list %.8x of size %.8x",
						addr, size);

		if (ignore_bad_displaylist)
			return 9;
		else
			return -9;
	}

	if (gx_parse_list (addr, size) < 0)
	{
		if (ignore_bad_displaylist)
		{
			DEBUG (EVENT_LOG_GX, "..gx: problem with displaylist %.8x %.8x", addr, size);
			return 9;
		}
		else
		{
			DEBUG (EVENT_EFATAL, "..gx: problem with displaylist %.8x %.8x", addr, size);
			return -9;
		}
	}
	else
	{
		DEBUG (EVENT_LOG_GX, "....  end of displaylist %.8x", addr);
		return 9;
	}
}


GPOPCODE (DRAW_QUADS)
{
	GPOPCODE_SIZE (3 + gx_list_size (FIFO_U16 (mem + 1), FIFO_U8 (mem) & 7));

	DEBUG (EVENT_LOG_GX, "....  draw quads, %d verts", FIFO_U16 (mem + 1));

	return 3 + gx_draw (mem + 3, GX_QUADS, FIFO_U16 (mem + 1), FIFO_U8 (mem) & 7);
}


GPOPCODE (DRAW_TRIANGLES)
{
	GPOPCODE_SIZE (3 + gx_list_size (FIFO_U16 (mem + 1), FIFO_U8 (mem) & 7));

	DEBUG (EVENT_LOG_GX, "....  draw triangles, %d verts", FIFO_U16 (mem + 1));

	return 3 + gx_draw (mem + 3, GX_TRIANGLES, FIFO_U16 (mem + 1), FIFO_U8 (mem) & 7);
}


GPOPCODE (DRAW_TRIANGLE_STRIP)
{
	GPOPCODE_SIZE (3 + gx_list_size (FIFO_U16 (mem + 1), FIFO_U8 (mem) & 7));

	DEBUG (EVENT_LOG_GX, "....  draw triangle strip, %d verts", FIFO_U16 (mem + 1));

	return 3 + gx_draw (mem + 3, GX_TRIANGLE_STRIP, FIFO_U16 (mem + 1), FIFO_U8 (mem) & 7);
}


GPOPCODE (DRAW_TRIANGLE_FAN)
{
	GPOPCODE_SIZE (3 + gx_list_size (FIFO_U16 (mem + 1), FIFO_U8 (mem) & 7));

	DEBUG (EVENT_LOG_GX, "....  draw triangle fan, %d verts", FIFO_U16 (mem + 1));

	return 3 + gx_draw (mem + 3, GX_TRIANGLE_FAN, FIFO_U16 (mem + 1), FIFO_U8 (mem) & 7);
}


GPOPCODE (DRAW_LINES)
{
	GPOPCODE_SIZE (3 + gx_list_size (FIFO_U16 (mem + 1), FIFO_U8 (mem) & 7));

	DEBUG (EVENT_LOG_GX, "....  draw lines, %d verts", FIFO_U16 (mem + 1));

	return 3 + gx_draw (mem + 3, GX_LINES, FIFO_U16 (mem + 1), FIFO_U8 (mem) & 7);
}


GPOPCODE (DRAW_LINE_STRIP)
{
	GPOPCODE_SIZE (3 + gx_list_size (FIFO_U16 (mem + 1), FIFO_U8 (mem) & 7));

	DEBUG (EVENT_LOG_GX, "....  draw line strip, %d verts", FIFO_U16 (mem + 1));

	return 3 + gx_draw (mem + 3, GX_LINE_STRIP, FIFO_U16 (mem + 1), FIFO_U8 (mem) & 7);
}


GPOPCODE (DRAW_POINTS)
{
	GPOPCODE_SIZE (3 + gx_list_size (FIFO_U16 (mem + 1), FIFO_U8 (mem) & 7));

	DEBUG (EVENT_LOG_GX, "....  draw points, %d verts", FIFO_U16 (mem + 1));

	return 3 + gx_draw (mem + 3, GX_POINTS, FIFO_U16 (mem + 1), FIFO_U8 (mem) & 7);
}


int gx_switch (int sw)
{
	switch (sw)
	{
		case GX_TOGGLE_WIREFRAME:
			{
				gxswitches.wireframe = !gxswitches.wireframe;

				DEBUG (EVENT_LOG_GX, "..gx: toggle wireframe");
				if (gxswitches.wireframe)
				{
					glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);

					glClearColor (1, 1, 1, 0);

					glDisable (GL_BLEND);
					glDisable (GL_COLOR_LOGIC_OP);
					glDisable (GL_DEPTH_TEST);
					glDisable (GL_ALPHA_TEST);
					glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

					if (gxswitches.wireframe_culling)
						glEnable (GL_CULL_FACE);
					else
						glDisable (GL_CULL_FACE);
				}
				else
				{
					glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
			
					gx_set_cull_mode ();
					gx_set_copy_clear_color ();
					gx_set_cmode0 ();
					gx_set_zmode ();
					
					gxswitches.wireframe_culling = !gxswitches.wireframe_culling;
				}
			}
			return gxswitches.wireframe;
		
		case GX_TOGGLE_TEX_RELOAD:
			gxswitches.tex_reload = !gxswitches.tex_reload;
			return gxswitches.tex_reload;
		
		case GX_TOGGLE_TEX_DUMP:
			gxswitches.dump_textures = !gxswitches.dump_textures;
			return gxswitches.dump_textures;

		case GX_TOGGLE_FORCE_LINEAR:
			gxswitches.force_linear = !gxswitches.force_linear;
			return gxswitches.force_linear;

		case GX_TOGGLE_TEX_TRANSPARENT:
			gxswitches.tex_transparent = !gxswitches.tex_transparent;
			return gxswitches.tex_transparent;

		case GX_TOGGLE_DRAW:
			gxswitches.dont_draw = !gxswitches.dont_draw;
			return gxswitches.dont_draw;

		case GX_TOGGLE_NO_LOGIC_OPS:
			gxswitches.no_logic_ops = !gxswitches.no_logic_ops;
			return gxswitches.no_logic_ops;

		case GX_TOGGLE_FULLBRIGHT:
			gxswitches.sw_fullbright = !gxswitches.sw_fullbright;
			return gxswitches.sw_fullbright;

		case GX_TOGGLE_USE_GL_MIPMAPS:
			gxswitches.use_gl_mipmaps = !gxswitches.use_gl_mipmaps;
			texcache_remove_all ();
			return gxswitches.use_gl_mipmaps;

		case GX_TOGGLE_FORCE_MAX_ANISO:
			gxswitches.force_max_aniso = !gxswitches.force_max_aniso;
			texcache_remove_all ();
			return gxswitches.force_max_aniso;

		case GX_TOGGLE_TC_INVALIDATE_ENABLED:
			gxswitches.tc_invalidate_enabled = !gxswitches.tc_invalidate_enabled;
			return gxswitches.tc_invalidate_enabled;

		case GX_TOGGLE_ENGINE:
			gxswitches.new_engine = !gxswitches.new_engine;
			gxswitches.use_shaders = !gxswitches.use_shaders;
			cg_enable (gxswitches.use_shaders);
			texcache_remove_all ();
			return gxswitches.new_engine;

		case GX_TOGGLE_FOG:
			gxswitches.fog_enabled = !gxswitches.fog_enabled;
			return gxswitches.fog_enabled;

		case GX_TOGGLE_FIX_FLICKERING:
			gxswitches.flicker_fix = !gxswitches.flicker_fix;
			return gxswitches.flicker_fix;

		case GX_TOGGLE_FIX_BLENDING:
			gxswitches.blending_fix = !gxswitches.blending_fix;
			return gxswitches.blending_fix;

		default:
			return -1;
	}
}


void gx_reinit (void)
{
	gx_set_viewport ();
	gx_set_scissors ();
	gx_set_copy_clear_z ();
	gx_set_copy_clear_color ();
	gx_set_zmode ();
	gx_set_cmode0 ();
	gx_set_cull_mode ();
	gx_set_lpsize ();
	gx_set_projection ();
	gx_set_fog_color ();
	gx_set_alphafunc ();
}


void gx_init (void)
{
	int i;


	memset (&texcache, 0, sizeof (texcache));
	memset (&texcache_rt, 0, sizeof (texcache));
//	memset (&gxswitches, 0, sizeof (gxswitches));
	gxswitches.new_engine = gxswitches.use_shaders = TRUE;
//	gxswitches.fog_enabled = TRUE;
	// less texture converts
//	gxswitches.use_gl_mipmaps = TRUE;

	for (i = 0; i < 0x100; i++)
		GPOP (i) = INVALID;

	GPOP (0x00) = NOP;
	GPOP (0x08) = LOAD_CP;
	GPOP (0x10) = LOAD_XF;
	GPOP (0x20) = LOAD_INDEX;
	GPOP (0x28) = LOAD_INDEX;
	GPOP (0x30) = LOAD_INDEX;
	GPOP (0x38) = LOAD_INDEX;
	GPOP (0x40) = CALL_DISPLAYLIST;
	GPOP (0x48) = INVALIDATE_VCACHE;
	GPOP (0x61) = LOAD_BP;

	GPOPR (0x80, 0x87, DRAW_QUADS);
	GPOPR (0x90, 0x97, DRAW_TRIANGLES);
	GPOPR (0x98, 0x9f, DRAW_TRIANGLE_STRIP);
	GPOPR (0xa0, 0xa7, DRAW_TRIANGLE_FAN);
	GPOPR (0xa8, 0xaf, DRAW_LINES);
	GPOPR (0xb0, 0xb7, DRAW_LINE_STRIP);
	GPOPR (0xb8, 0xbf, DRAW_POINTS);

	BP_MASK = 0x00ffffff;
}

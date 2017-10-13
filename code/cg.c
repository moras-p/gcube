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

#include "cg.h"
#include "hw_gx.h"

#include "cg_vertex.c"

#define CG_DUMP_F				0
#define CG_LOAD_F				0

CGcontext cg_context = NULL;
CGprofile cg_profile_v, cg_profile_f;
CGprogram cg_program_v, cg_program_f = NULL;

// those need to be saved
float s_cconst[4][4];
float s_kconst[4][4];
float s_indm[3][6];
float s_indscale[3];
int s_aref[2];
int s_tus[8][2];


typedef struct
{
	MagicNum magic;
	CGprogram cg_program;
} FpTag;

FpTag fpcache[2048];
unsigned int nfptags = 0;




void cg_error (void)
{
	CGerror error = cgGetError ();
	
	
	if (error)
	{
		const char *listing = cgGetLastListing (cg_context);
		
		if (listing)
			fprintf (stderr, "%s\n%s", cgGetErrorString (error), cgGetLastListing (cg_context));
		else
			fprintf (stderr, "%s\n", cgGetErrorString (error));
	}
}


void cg_enable (int sw)
{
	if (sw)
	{
		if (cg_program_v)
		{
			cgGLEnableProfile (cg_profile_v);
			cgGLBindProgram (cg_program_v);
		}

		// getting incorrect results without this (?)
		glAlphaFunc (GL_ALWAYS, 0);
		glEnable (GL_ALPHA_TEST);
	}
	else
	{
		cgGLDisableProfile (cg_profile_v);
		cgGLDisableProfile (cg_profile_f);
	}
}


void cg_cleanup (void)
{
	if (cg_context)
		cgDestroyContext (cg_context);
}


int cg_init (void)
{
	if (cg_context)
		cg_cleanup ();

	memset (fpcache, 0, sizeof (fpcache));

	cgSetErrorCallback (cg_error);
	cg_context = cgCreateContext ();

//	cg_profile_v = cgGLGetLatestProfile (CG_GL_VERTEX);
	cg_profile_v = cgGetProfile ("arbvp1");
	cgGLSetOptimalOptions (cg_profile_v);
//	cg_profile_f = cgGLGetLatestProfile (CG_GL_FRAGMENT);
	cg_profile_f = cgGetProfile ("arbfp1");
	cgGLSetOptimalOptions (cg_profile_f);

	cg_program_v = cgCreateProgram (cg_context, CG_SOURCE,
									cg_vertex_str, cg_profile_v, "main", NULL);

	if (cg_program_v)
		cgGLLoadProgram (cg_program_v);
	else
	{
		fprintf (stderr, "CG: Error loading Vertex Program:\n%s", cg_vertex_str);
		return FALSE;
	}

	cg_enable (gxswitches.use_shaders);

	return TRUE;
}


void cg_params_update_const (int n)
{
	static const char *name[] =
	{
		"cconst[0]",
		"cconst[1]",
		"cconst[2]",
		"cconst[3]",
	};
	
	
	if (!gxswitches.use_shaders || !cg_program_f)
		return;

	cgSetParameter4fv (cgGetNamedParameter (cg_program_f, name[n]), s_cconst[n]);
}


void cg_params_update_konst (int n)
{
	static const char *name[] =
	{
		"kconst[0]",
		"kconst[1]",
		"kconst[2]",
		"kconst[3]",
	};
	
	
	if (!gxswitches.use_shaders || !cg_program_f)
		return;

	cgSetParameter4fv (cgGetNamedParameter (cg_program_f, name[n]), s_kconst[n]);
}


void cg_params_update_indm (int n)
{
	static const char *name[] =
	{
		"indm[0]",
		"indm[1]",
		"indm[2]",
	};
	
	
	if (!gxswitches.use_shaders || !cg_program_f)
		return;

	cgSetMatrixParameterfr (cgGetNamedParameter (cg_program_f, name[n]), s_indm[n]);
}


void cg_params_update_indscale (int n)
{
	static const char *name[] =
	{
		"indscale[0]",
		"indscale[1]",
		"indscale[2]",
	};
	
	
	if (!gxswitches.use_shaders || !cg_program_f)
		return;

	cgSetParameter1f (cgGetNamedParameter (cg_program_f, name[n]), s_indscale[n]);
}


void cg_params_update_aref (void)
{
	if (!gxswitches.use_shaders || !cg_program_f)
		return;

	cgSetParameter1i (cgGetNamedParameter (cg_program_f, "aref[0]"), s_aref[0]);
	cgSetParameter1i (cgGetNamedParameter (cg_program_f, "aref[1]"), s_aref[1]);
}


void cg_params_update_tus (int n)
{
	static const char *name[] =
	{
		"TUS[0]",
		"TUS[1]",
		"TUS[2]",
		"TUS[3]",
		"TUS[4]",
		"TUS[5]",
		"TUS[6]",
		"TUS[7]",
	};


	if (!gxswitches.use_shaders || !cg_program_f)
		return;

	cgSetParameter2iv (cgGetNamedParameter (cg_program_f, name[n]), s_tus[n]);
}


void cg_params_update_f (void)
{
	int i;


	for (i = 0; i < 8; i++)
		cg_params_update_tus (i);

	for (i = 0; i < 4; i++)
	{
		cg_params_update_const (i);
		cg_params_update_konst (i);
	}

	for (i = 0; i < 3; i++)
	{
		cg_params_update_indm (i);
		cg_params_update_indscale (i);
	}
	
	cg_params_update_aref ();
}


void cg_params_set_const_ra (int n, float c[2])
{
	s_cconst[n][0] = c[0];
	s_cconst[n][3] = c[1];

	cg_params_update_const (n);
}


void cg_params_set_konst_ra (int n, float c[2])
{
	s_kconst[n][0] = c[0];
	s_kconst[n][3] = c[1];
	
	cg_params_update_konst (n);
}


void cg_params_set_const_bg (int n, float c[2])
{
	s_cconst[n][2] = c[0];
	s_cconst[n][1] = c[1];

	cg_params_update_const (n);
}


void cg_params_set_konst_bg (int n, float c[2])
{
	s_kconst[n][2] = c[0];
	s_kconst[n][1] = c[1];
	
	cg_params_update_konst (n);
}


void cg_params_set_indm_ab (int n, float c1, float c2)
{
	s_indm[n][0] = c1;
	s_indm[n][3] = c2;
	
	cg_params_update_indm (n);
}


void cg_params_set_indm_cd (int n, float c1, float c2)
{
	s_indm[n][1] = c1;
	s_indm[n][4] = c2;
	
	cg_params_update_indm (n);
}


void cg_params_set_indm_ef (int n, float c1, float c2)
{
	s_indm[n][2] = c1;
	s_indm[n][5] = c2;
	
	cg_params_update_indm (n);
}


void cg_params_set_indscale (int n, float c)
{
	s_indscale[n] = c;

	cg_params_update_indscale (n);
}


void cg_params_set_aref (int c1, int c2)
{
	s_aref[0] = c1;
	s_aref[1] = c2;

	cg_params_update_aref ();
}


void cg_params_set_tus (int n, unsigned int c1, unsigned int c2)
{
	s_tus[n][0] = c1;
	s_tus[n][1] = c2;

	cg_params_update_tus (n);
}


void cg_get_tev_magic (MagicNum *m)
{
	int i, stc, stt;
	

	magic_num_reset (m);
	magic_num_acc (m, BP_GENMODE & 0x00073dff, 19);

	for (i = 0; i <= BP_TEVSTAGES; i++)
	{
		magic_num_acc (m, BP_TS (i) & 0x3ff, 10);

		stc = C_RSWAP (i);
		stt = C_TSWAP (i);
		magic_num_acc (m, stc, 2);
		magic_num_acc (m, stt, 2);

		magic_num_acc (m, KSEL_XRG (stc) & 0xf, 4);
		magic_num_acc (m, KSEL_XBA (stc) & 0xf, 4);
		magic_num_acc (m, KSEL_XRG (stt) & 0xf, 4);
		magic_num_acc (m, KSEL_XBA (stt) & 0xf, 4);

		magic_num_acc (m, BP_COLOR_ENV (i) & 0xffffff, 24);
		magic_num_acc (m, BP_ALPHA_ENV (i) & 0xffffff, 24);
	}

	magic_num_acc (m, (BP_TEV_ALPHAFUNC >> 16) & 0xff, 8);

	magic_num_acc (m, TEV_Z_ENV0 & 0xffffff, 24);
	magic_num_acc (m, TEV_Z_ENV1 & 0xf, 4);

	magic_num_acc (m, gxswitches.fog_enabled ? TEV_FOG_FSEL : 0, 3);
	
	for (i = 0; i < 8; i++)
		magic_num_acc (m, gx_is_tex_p2 (i), 1);
}


// for debugging purposes
void cg_break (void)
{
}


void cg_break_on_shader (__u32 xmagic)
{
	MagicNum magic;


	cg_get_tev_magic (&magic);
	if (xmagic == magic_num_xmagic (&magic))
		cg_break ();
}


void cg_fpcache_clear (void)
{
	while (nfptags)
		cgDestroyProgram (fpcache[--nfptags].cg_program);
	
	nfptags = 0;
	cg_program_f = 0;
}


int cg_fpcache_fetch (MagicNum *magic)
{
	int i;


	for (i = 0; i < nfptags; i++)
	{
		if (magic_num_eq (magic, &fpcache[i].magic))
			return i;
	}
	
	return -1;
}


void cg_bind_program_f (int n)
{
	if (gxswitches.use_shaders)
	{
		cgGLEnableProfile (cg_profile_f);
		if (cg_program_f != fpcache[n].cg_program)
		{
			cgGLBindProgram (fpcache[n].cg_program);
			cg_program_f = fpcache[n].cg_program;
			cg_params_update_f ();
		}
	}
}


void cg_unbind_f (void)
{
	cgGLDisableProfile (cg_profile_f);
}


#if MAGICNUM_SAFETY
void cg_fpcache_xmagic_check (int n)
{
	int i;


	for (i = 0; i < nfptags; i++)
	{
		if (i == n)
			continue;
		
		if (fpcache[i].magic.xmagic == fpcache[n].magic.xmagic)
		{
			printf ("fpcache[%d] xmagic invalid!\n", n);
			return;
		}
	}
}


void cg_fpcache_xmagic_check_all (void)
{
	int i;


	for (i = 0; i < nfptags; i++)
		cg_cache_xmagic_check (i);
}
#endif


CGprogram cg_from_file_f (MagicNum *magic)
{
	char filename[1024];


	sprintf (filename, "cg_%8.8x.fp", magic_num_xmagic (magic));
	if (file_exists (filename))
	{
		return cgCreateProgramFromFile (cg_context, CG_SOURCE,
								filename, cg_profile_f, "main", NULL);
	}
	else
		return 0;
}


CGprogram cg_from_str_f (char *prg_str)
{
	return cgCreateProgram (cg_context, CG_SOURCE, prg_str, cg_profile_f, "main", NULL);
}


void cg_dump_program (__u32 xmagic, const char *str, const char *ext)
{
	char filename[1024];
	FILE *f;


	sprintf (filename, "./cg_%8.8x.%s", xmagic, ext);
	f = fopen (filename, "w");
	if (f)
	{
		fprintf (f, str);
		fclose (f);
	}
}


void cg_dump_program_f (int n, int compiled)
{
	cg_dump_program (magic_num_xmagic (&fpcache[n].magic),
									 cgGetProgramString (fpcache[n].cg_program, CG_PROGRAM_SOURCE),
									 "fp");

	if (compiled)
		cg_dump_program (magic_num_xmagic (&fpcache[n].magic),
										 cgGetProgramString (fpcache[n].cg_program, CG_COMPILED_PROGRAM),
										 "fpc");
}


////////////////////////////////////////////////
// shader generator

const char *tev_bias[] =
{
	"+ 0.0",
	"+ 0.5",
	"- 0.5",
};


const char *tev_scale[] =
{
	"1.0",
	"2.0",
	"4.0",
	"0.5",
};


const char *tev_cinput[] =
{
	"CPREV",
	"APREV",
	"C0",
	"A0",
	"C1",
	"A1",
	"C2",
	"A2",
	"TEXC",
	"TEXA",
	"RASC",
	"RASA",
	"CONE",
	"CHALF",
	"CKONST", // QUARTER for HW1
	"CZERO",
};


const char *tev_cinput_cmp[] =
{
	"PREV",
	"PREV.aaaa",
	"REG0",
	"REG0.aaaa",
	"REG1",
	"REG1.aaaa",
	"REG2",
	"REG2.aaaa",
	"TEXCA",
	"TEXCA.aaaa",
	"RASCA",
	"RASCA.aaaa",
	"ONE",
	"HALF",
	"KONSTCA", // QUARTER for HW1
	"ZERO",
};


const char *tev_ainput[] =
{
	"APREV",
	"A0",
	"A1",
	"A2",
	"TEXA",
	"RASA",
	"AKONST", // ONE for HW1
	"AZERO",
};


const char *tev_ainput_cmp[] =
{
	"PREV",
	"REG0",
	"REG1",
	"REG2",
	"TEXCA",
	"RASCA",
	"KONSTCA", // ONE for HW1
	"ZERO",
};


const char *tev_cdest[] =
{
	"CPREV",
	"C0",
	"C1",
	"C2",
};


const char *tev_adest[] =
{
	"APREV",
	"A0",
	"A1",
	"A2",
};


const char *tev_reg_names[] =
{
	"PREV",
	"REG0",
	"REG1",
	"REG2",
};


const char *tev_kcsel[] =
{
	"1.0", "7.0/8", "3.0/4", "5.0/8", "1.0/2", "3.0/8", "1.0/4", "1.0/8",
	"ERROR", "ERROR", "ERROR", "ERROR",
	"K0.rgb", "K1.rgb", "K2.rgb", "K3.rgb",
	"K0.rrr", "K1.rrr", "K2.rrr", "K3.rrr",
	"K0.ggg", "K1.ggg", "K2.ggg", "K3.ggg",
	"K0.bbb", "K1.bbb", "K2.bbb", "K3.bbb",
	"K0.aaa", "K1.aaa", "K2.aaa", "K3.aaa",
};


const char *tev_kasel[] =
{
	"1.0", "7.0/8", "3.0/4", "5.0/8", "1.0/2", "3.0/8", "1.0/4", "1.0/8",
	"ERROR", "ERROR", "ERROR", "ERROR",
	"ERROR", "ERROR", "ERROR", "ERROR",
	"K0.r",	"K1.r", "K2.r",	"K3.r",
	"K0.g", "K1.g", "K2.g", "K3.g",
	"K0.b", "K1.b", "K2.b", "K3.b",
	"K0.a", "K1.a", "K2.a", "K3.a",
};


const char *tev_alphafunc0[] =
{
	"false",
	"A < aref[0]",
	"A == aref[0]",
	"A <= aref[0]",
	"A > aref[0]",
	"A != aref[0]",
	"A >= aref[0]",
	"true",
};


const char *tev_alphafunc1[] =
{
	"false",
	"A < aref[1]",
	"A == aref[1]",
	"A <= aref[1]",
	"A > aref[1]",
	"A != aref[1]",
	"A >= aref[1]",
	"true",
};


const char *tev_alphalogic[] =
{
	"&&",
	"||",
	"!=",
	"==",
};


const char *tev_comp_op[] =
{
	"GT",
	"EQ",
};


const char *tev_comp_scale[] =
{
	"R8",
	"GR16",
	"BGR24",
	"RGB8",
};


const char *tev_comp_ascale[] =
{
	"R8",
	"GR16",
	"BGR24",
	"A8",
};


const char *tev_fog[] =
{
	"ERROR",
	"ERROR",
	"LINEAR",
	"ERROR",
	"EXP",
	"EXP2",
	"BEXP",
	"BEXP2",
};


const char *tev_ind_bias[] =
{
	"NONE",
	"x",
	"y",
	"xy",
	"z",
	"xz",
	"yz",
	"xyz",
};


const char *tev_ind_bump[] =
{
	"OFF",
	"x",
	"y",
	"z",
};


const char *tev_ras[] =
{
	"COLOR (0)",
	"COLOR (1)",
	"ERROR", "ERROR", "ERROR",
	"BUMP",
	"BUMP * 255.0 / 248", // normalized bump
	"ZERO",
};


const char *tev_rgba = "rgba";


const char *tev_stwrap[] =
{
	"OFF", "256", "128", "64", "32", "16", "0", "ERROR",
};


const int tev_fmt_bits[] =
{
	256, 32, 16, 8,
};


static char sbuff[0xffff];
char *cg_inc = NULL;

#define CG_START									(sbuff[0] = '\0')
#define CG_PRINT(s)								(strcat (sbuff, s))
#define CG_PRINTF(fmt,args...)		(sprintf (sbuff + strlen (sbuff), fmt, ## args))
#define CG_END										return sbuff

// for debugging purposes
void cg_fp_print_config (void)
{
	int i;


	CG_PRINTF ("\t// tev stages %d ind stages %d colors %d texgens %d\n",
						 BP_TEVSTAGES + 1, BP_INDSTAGES, BP_COLORS, BP_TEXGENS);

	for (i = 0; i < 8; i++)
		if (texactive[i])
			CG_PRINTF ("\t// tex%d: gl %d\n", i, texactive[i]->tex);

	CG_PRINTF ("\t// AREF0 = %d; AREF1 = %d;\n",
							TEV_ALPHAFUNC_A0, TEV_ALPHAFUNC_A1);

	if (BP_INDSTAGES)
		for (i = 0; i < 3; i++)
			CG_PRINTF ("\t// m[%d] = float2x3 (%f, %f, %f, %f, %f, %f) * %f;\n", i,
							FLOAT_S11 (IND_MA(i)), FLOAT_S11 (IND_MC (i)), FLOAT_S11 (IND_ME (i)),
							FLOAT_S11 (IND_MB(i)), FLOAT_S11 (IND_MD (i)), FLOAT_S11 (IND_MF (i)),
							IND_MSF (i));

	for (i = 0; i < 4; i++)
	{
		CG_PRINTF ("\t//%s = float4 (%1.2f, %1.2f, %1.2f, %1.2f);\n",
						tev_reg_names[i],
						s_cconst[i][0], s_cconst[i][1], s_cconst[i][2], s_cconst[i][3]);

		CG_PRINTF ("\t//K%d = float4 (%1.2f, %1.2f, %1.2f, %1.2f);\n",
						i, s_kconst[i][0], s_kconst[i][1], s_kconst[i][2], s_kconst[i][3]);
	}
	
	for (i = 0; i < 8; i++)
	{
		if (texactive[i])
		{
			CG_PRINTF ("\t//TUS[%d] = int2 (%d, %d);\n", i,
								 texactive[i]->width,
								 texactive[i]->height);
		}
	}
}


char *cg_gen_fp (void)
{
	int i, stc, stt, mi;


	if (!cg_inc)
		cg_inc = f2str ("/big/dev/c/gcube/release/0.5c/cg_fragment.cg");

	CG_START;
	CG_PRINT (cg_inc);
	CG_PRINT ("\nfpout main (vpout IN)\n{\n\tfpout OUT;\n\n");

//	cg_fp_print_config ();

	if (BP_INDSTAGES)
	{
		CG_PRINT ("\n\t// indirect stages follow\n");

		// 4 indirect stages
		for (i = 0; i < BP_INDSTAGES; i++)
			CG_PRINTF ("\tiST[%d] = TTEX%s (%d, TC (%d).stq * float3 (1.0/%d, 1.0/%d, 1)).abg;\n",
								i,
								gx_is_tex_p2 (RAS_BI (i)) ? "_P2" : "",
								RAS_BI (i), RAS_CI (i),
								1 << RAS_SS (i), 1 << RAS_TS (i));
	}

	CG_PRINTF ("\n");

	for (i = 0; i < 4; i++)
		CG_PRINTF ("\t%s = cconst[%d];\n", tev_reg_names[i], i);

	CG_PRINT ("\n\t// tev stages follow\n");

	for (i = 0; i <= BP_TEVSTAGES; i++)
	{
		CG_PRINTF ("\n\t// stage %d\n", i);

		// rswap / tswap -> swap table id (from 0 to 3)
		stc = C_RSWAP (i);
		stt = C_TSWAP (i);

		if (TEV_STAGE_DIRECT (i) && RAS_TE (i))
		{
			CG_PRINTF ("\tTEXCA = TEX%s (%d, %d).%c%c%c%c;\n",
							gx_is_tex_p2 (RAS_TI (i)) ? "_P2" : "",
							RAS_TI (i), RAS_TC (i),
							tev_rgba[KSEL_SWAP_R (stt)],
							tev_rgba[KSEL_SWAP_G (stt)],
							tev_rgba[KSEL_SWAP_B (stt)],
							tev_rgba[KSEL_SWAP_A (stt)]);
		}
		else
		{
			CG_PRINTF ("\t// ind stage %d BS %d FMT %d BIAS %d M %d\n",
							IND_BT (i), IND_BS (i), IND_FMT (i), IND_BIAS (i), IND_M (i));

			// regular tex coords
			CG_PRINTF ("\tST = TC (%d).stq;\n", RAS_TC (i));

			// format
			CG_PRINTF ("\tSTsh = TC_FORMAT (iST[%d], %d);\n",
							 IND_BT (i), tev_fmt_bits[IND_FMT (i)]);

			// bump alpha select
			if (IND_BS (i))
				CG_PRINTF ("\tBUMP.a = STsh.%s;\n", tev_ind_bump[IND_BS (i)]);

			// bias
			if (IND_BIAS (i))
				CG_PRINTF ("\tSTsh.%s += 1.0 / %d;\n",
								tev_ind_bias[IND_BIAS (i)],
								IND_FMT (i) ? tev_fmt_bits[IND_FMT (i)] : -2);

			// times 4 gets a more visible bump offset
			// scale by matrix
			if (IND_M (i) >= 9)
			{
				// T type dynamic matrix
				CG_PRINTF ("\tSTsh = float3 (STsh.t * ST.s, STsh.t * ST.t, 0) * indscale[%d] * %d / 256;\n",
									 IND_M (i) - 9, tev_fmt_bits[IND_FMT (i)]);
			}
			else if (IND_M (i) >= 5)
			{
				// S type dynamic matrix
				CG_PRINTF ("\tSTsh = float3 (STsh.s * ST.s, STsh.s * ST.t, 0) * indscale[%d] * %d / 256;\n",
									 IND_M (i) - 5, tev_fmt_bits[IND_FMT (i)]);
			}
			else if (IND_M (i) >= 1)
			{
				mi = IND_M (i) - 1;
				CG_PRINTF ("\tSTsh = float3 (mul (indm[%d] * indscale[%d], STsh), 0) * %d;\n",
									 mi, mi, tev_fmt_bits[IND_FMT (i)]);
			}

			// bump with optional feedback
			CG_PRINTF ("\tBUMP.xyz %s STsh.xyz;\n", IND_FB (i) ? "+=" : "=");

			if (RAS_TE (i))
			{
				// wrapping (eliminate case 0)
				// wrap s
				if (IND_SW (i))
					CG_PRINTF ("\tST.s = TC_WRAP_X (ST.s, %s);\n", tev_stwrap[IND_SW (i)]);

				// wrap t
				if (IND_TW (i))
					CG_PRINTF ("\tST.t = TC_WRAP_X (ST.t, %s);\n", tev_stwrap[IND_TW (i)]);

				CG_PRINTF ("\tTEXCA = TTEX_SH%s (%d, ST, BUMP.xyz).%c%c%c%c;\n",
								gx_is_tex_p2 (RAS_TI (i)) ? "_P2" : "", RAS_TI (i),
								tev_rgba[KSEL_SWAP_R (stt)],
								tev_rgba[KSEL_SWAP_G (stt)],
								tev_rgba[KSEL_SWAP_B (stt)],
								tev_rgba[KSEL_SWAP_A (stt)]);
			}
		}

		CG_PRINTF ("\tKONSTCA = F31 (%s, %s);\n",
							tev_kcsel[KSEL_C (i)], tev_kasel[KSEL_A (i)]);

		CG_PRINTF ("\tRASCA = %s.%c%c%c%c;\n",
						tev_ras[RAS_CC (i)],
						tev_rgba[KSEL_SWAP_R (stc)],
						tev_rgba[KSEL_SWAP_G (stc)],
						tev_rgba[KSEL_SWAP_B (stc)],
						tev_rgba[KSEL_SWAP_A (stc)]);

		// color
		if (C_COLOR_BIAS (i) == GX_BIAS_COMPARE)
		{
			// d + ((a op b) ? c : 0)
			// this additional testing for true shouldn't be here, but otherwise
			//   the results are incorrect
			CG_PRINTF ("\t%s = clamp%s (%s + ((true == CMP_%s_%s (%s, %s)) ? %s.rgb : 0));\n",
							tev_cdest [C_COLOR_DEST (i)],
							C_COLOR_CLAMP (i) ? "u8" : "s10",
							tev_cinput [C_COLOR_D (i)],
							tev_comp_scale [C_COLOR_SCALE (i)],
							tev_comp_op [C_COLOR_OP (i)],
							tev_cinput_cmp [C_COLOR_A (i)],
							tev_cinput_cmp [C_COLOR_B (i)],
							tev_cinput_cmp [C_COLOR_C (i)]);
		}
		else
		{
			// (d op (a * (1 - c) + b*c) + bias) * scale
			CG_PRINTF ("\t%s = clamp%s ((%s %c lerp3 (%s, %s, %s) %s) * %s);\n",
							tev_cdest [C_COLOR_DEST (i)],
							C_COLOR_CLAMP (i) ? "u8" : "s10",
							tev_cinput [C_COLOR_D (i)],
							C_COLOR_OP (i) ? '-' : '+',
							tev_cinput [C_COLOR_A (i)],
							tev_cinput [C_COLOR_B (i)],
							tev_cinput [C_COLOR_C (i)],
							tev_bias [C_COLOR_BIAS (i)],
							tev_scale [C_COLOR_SCALE (i)]);
		}

		// alpha
		if (C_ALPHA_BIAS (i) == GX_BIAS_COMPARE)
		{
			// d + ((a op b) ? c : 0)
			CG_PRINTF ("\t%s = clamp%s (%s + ((true == CMP_%s_%s (%s, %s)) ? %s.a : 0));\n",
							tev_adest [C_ALPHA_DEST (i)],
							C_ALPHA_CLAMP (i) ? "u8" : "s10",
							tev_ainput [C_ALPHA_D (i)],
							tev_comp_ascale [C_ALPHA_SCALE (i)],
							tev_comp_op [C_ALPHA_OP (i)],
							tev_ainput_cmp [C_ALPHA_A (i)],
							tev_ainput_cmp [C_ALPHA_B (i)],
							tev_ainput_cmp [C_ALPHA_C (i)]);
		}
		else
		{
			// (d op (a * (1 - c) + b*c) + bias) * scale
			CG_PRINTF ("\t%s = clamp%s ((%s %c lerp1 (%s, %s, %s) %s) * %s);\n",
							tev_adest [C_ALPHA_DEST (i)],
							C_ALPHA_CLAMP (i) ? "u8" : "s10",
							tev_ainput [C_ALPHA_D (i)],
							C_ALPHA_OP (i) ? '-' : '+',
							tev_ainput [C_ALPHA_A (i)],
							tev_ainput [C_ALPHA_B (i)],
							tev_ainput [C_ALPHA_C (i)],
							tev_bias [C_ALPHA_BIAS (i)],
							tev_scale [C_ALPHA_SCALE (i)]);
		}
	}

	// alpha_pass = (alpha_src (comp0) ref0) (op) (alpha_src (comp1) ref1)
	// usuall case, no testing
	if (!((TEV_ALPHAFUNC_OP0 == TEV_ALPHAFUNC_OP1) &&
			(TEV_ALPHAFUNC_OP0 == ALPHAOP_TRUE) && (TEV_ALPHAFUNC_LOGIC != 2)))
	{
		CG_PRINT ("\n\t// alpha test\n");
		CG_PRINT ("\tA = round (APREV * 255);\n");
		CG_PRINTF ("\tif (! ((%s) %s (%s)) ) discard;\n",
								tev_alphafunc0[TEV_ALPHAFUNC_OP0],
								tev_alphalogic[TEV_ALPHAFUNC_LOGIC],
								tev_alphafunc1[TEV_ALPHAFUNC_OP1]);
	}

	if (TEV_Z_OP)
		CG_PRINTF ("\n\tDEPTH_%s (%d.0 / 0x%x);",
						 (TEV_Z_OP == Z_OP_ADD) ? "ADD" : "REPLACE",
						 TEV_Z_BIAS, 0xffffff >> ((2 - TEV_Z_FORMAT) * 8));

	if (gxswitches.fog_enabled)
	if (!XF_PROJECTION_ORTHOGRAPHIC)
	if (TEV_FOG_FSEL)
		CG_PRINTF ("\n\tFOG_%s;\n", tev_fog[TEV_FOG_FSEL]);

	CG_PRINTF ("\n\treturn OUT;\n}\n");

	CG_END;	
}


int cg_get_program_f (void)
{
	MagicNum *magic = &fpcache[nfptags].magic;
	char *prg_str = NULL;
	int n;


	if (!cg_context)
		return 0;

	cg_get_tev_magic (magic);
	n = cg_fpcache_fetch (magic);
	if (n == -1)
	{
		n = nfptags++;

#if MAGICNUM_SAFETY
		cg_fpcache_xmagic_check (n);
#endif

#if CG_LOAD_F
		fpcache[n].cg_program = cg_from_file_f (magic);
#else
		fpcache[n].cg_program = 0;
#endif
		if (!fpcache[n].cg_program)
		{
			prg_str = cg_gen_fp ();
			fpcache[n].cg_program = cg_from_str_f (prg_str);
		}

		if (fpcache[n].cg_program)
		{
#if CG_DUMP_F
			cg_dump_program_f (n, FALSE);
#endif
			cgGLLoadProgram (fpcache[n].cg_program);

			printf ("fpcache[%d] (%8.8x)%s%s%s%s\n", nfptags, magic_num_xmagic (magic),
							prg_str ? "" : ", loaded from disk",
							BP_INDSTAGES ? ", indirect stages" : "",
							TEV_Z_OP ? ", z texture" : "",
							TEV_FOG_FSEL ? ", fog" : "");
		}
		else
		{
			if (prg_str)
			{
				printf ("CG: Error compiling program (%8.8x)\n%s",
								magic_num_xmagic (magic), prg_str);
#if CG_DUMP_F
				cg_dump_program (magic_num_xmagic (magic), prg_str, "fp");
#endif
			}
			else
				printf ("CG: Error compiling program loaded from disk (%8.8x)\n",
								magic_num_xmagic (magic));

			n = 0;
		}
	}

	return n;
}

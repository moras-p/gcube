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
 *  cleanup needed
 *         
 */

#include "gcube.h"
//#include "scandir.h"

#include "mapdb.c"


int gcube_running = FALSE;
char *quit_message = NULL;
Map *map = NULL, *mdb = NULL;
char mount_dir[256] = {0};
int do_save_state = FALSE;
int hle_enabled = FALSE;
char gcbin_filename[256] = {0};

extern int disable_fb, display_fb;

void generate_map (void);
void enable_ignore_movies (int level);
void enable_hle_input (int enable);
void enable_hle (int enable);

#ifndef GDEBUG

int emu_state = GCUBE_RUN;

#else

static DebuggerState debugger;
# ifdef GCUBE_BY_DEFAULT
int emu_state = GCUBE_RUN;
# else
int emu_state = GDEBUG_RUN;
# endif
int gdebug_running = FALSE;

int command_find (char *cmd);
void debugger_parse_command (char *command);
void debugger_execute_command (void);
void debugger_cmd_step_into (char *args);
void debugger_cmd_detach (char *args);
void debugger_cmd_quit (char *args);
void debugger_cmd_help (char *args);

void mem_show (Window *win, int lines, unsigned int addr);
void code_show (Window *win, int lines, unsigned int addr);
void regs_show (Window *win, int sw);
void output_show (Window *win, OutputWin *out, int nlines, int pos);
void output_append (OutputWin *out, char *str);
void output_immediate_append (OutputWin *out, char *str);
void output_vappend (OutputWin *out, va_list ap, const char *format);
void output_immediate_vappend (OutputWin *out, va_list ap, const char *format);
void output_appendf (OutputWin *out, char *format, ...);
void output_immediate_appendf (OutputWin *out, char *format, ...);
void output_destroy (OutputWin *out);
void command_history_append (CommandWin *cmd, char *str);
void command_history_destroy (CommandWin *cmd);

#define debug_print(X)		(output_append (&debugger.output, X))
#define debug_printf(X, args...)		(output_appendf (&debugger.output, X, ## args))
#define debug_immprintf(X, args...)		(output_immediate_appendf (&debugger.output, X, ## args))

void cwin_change_mode (int mode);
void gdebug_create_map (void);


void sprint_intmask (char *buff, __u32 b);

typedef enum
{
	DEBUG_PE_INT_FIN, DEBUG_PE_INT_TOK,
	DEBUG_VI_INT_1, DEBUG_VI_INT_2, DEBUG_VI_INT_3, DEBUG_VI_INT_4,
	DEBUG_DSP_INT_DSP, DEBUG_DSP_INT_AR, DEBUG_DSP_INT_AID,
	DEBUG_AI_INT_AI,
	DEBUG_EXI_C0_INT_TC, DEBUG_EXI_C0_INT_EXT, DEBUG_EXI_C0_INT_EXI,
	DEBUG_EXI_C1_INT_TC, DEBUG_EXI_C1_INT_EXT, DEBUG_EXI_C1_INT_EXI,
	DEBUG_EXI_C2_INT_TC, DEBUG_EXI_C2_INT_EXT, DEBUG_EXI_C2_INT_EXI,
	DEBUG_EXI_C3_INT_TC, DEBUG_EXI_C3_INT_EXT, DEBUG_EXI_C3_INT_EXI,
	DEBUG_SI_INT_TC, DEBUG_SI_INT_RDST,
	DEBUG_DI_INT_BRK, DEBUG_DI_INT_TC, DEBUG_DI_INT_DE, DEBUG_DI_INT_CVR,
} DebugInterrupt;


const char *reg_names[] =
{
	"R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "R8", "R9", "R10", "R11",
	"R12", "R13", "R14", "R15", "R16", "R17", "R18", "R19", "R20", "R21", "R22",
	"R23", "R24", "R25", "R26", "R27", "R28", "R29", "R30", "R31",

	"SR0", "SR1", "SR2", "SR3", "SR4", "SR5", "SR6", "SR7", "SR8", "SR9", "SR10",
	"SR11", "SR12", "SR13", "SR14", "SR15",
	"CR", "MSR", "FPSCR", "TBL", "TBU", "PC", "IC", "SP", "XER", "LR", "CTR",
	"DEC", "SRR0", "SRR1", "HID0", "HID1", "HID2",
	"SDA1", "SDA2", "WPAR", "DMAU", "DMAL",

	"F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7",
	"F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15",
	"F16", "F17", "F18", "F19", "F20", "F21", "F22", "F23",
	"F24", "F25", "F26", "F27", "F28", "F29", "F30", "F31",

	"PS00", "PS01", "PS02", "PS03", "PS04", "PS05", "PS06", "PS07",
	"PS08", "PS09", "PS010", "PS011", "PS012", "PS013", "PS014", "PS015",
	"PS016", "PS017", "PS018", "PS019", "PS020", "PS021", "PS022", "PS023",
	"PS024", "PS025", "PS026", "PS027", "PS028", "PS029", "PS030", "PS031",

	"PS10", "PS11", "PS12", "PS13", "PS14", "PS15", "PS16", "PS17",
	"PS18", "PS19", "PS110", "PS111", "PS112", "PS113", "PS114", "PS115",
	"PS116", "PS117", "PS118", "PS119", "PS120", "PS121", "PS122", "PS123",
	"PS124", "PS125", "PS126", "PS127", "PS128", "PS129", "PS130", "PS131",
};

#define I_FPR						0x10000000
#define I_PS0						0x20000000
#define I_PS1						0x30000000

const int reg_indices[] =
{
	I_GPR+0, I_GPR+1, I_GPR+2, I_GPR+3, I_GPR+4, I_GPR+5, I_GPR+6, I_GPR+7, 
	I_GPR+8, I_GPR+9, I_GPR+10, I_GPR+11, I_GPR+12, I_GPR+13, I_GPR+14,
	I_GPR+15, I_GPR+16, I_GPR+17, I_GPR+18, I_GPR+19, I_GPR+20, I_GPR+21,
	I_GPR+22, I_GPR+23, I_GPR+24, I_GPR+25, I_GPR+26, I_GPR+27, I_GPR+28,
	I_GPR+29, I_GPR+30, I_GPR+31,

	I_SR+0, I_SR+1, I_SR+2, I_SR+3, I_SR+4, I_SR+5, I_SR+6, I_SR+7, I_SR+8,
	I_SR+9, I_SR+10, I_SR+11, I_SR+12, I_SR+13, I_SR+14, I_SR+15,
	I_CR, I_MSR, I_FPSCR, I_TBL, I_TBU, I_PC, I_IC, I_SP, I_XER, I_LR, I_CTR,
	I_DEC, I_SRR0, I_SRR1, I_HID0, I_HID1, I_HID2,
	I_SDA1, I_SDA2, I_WPAR, I_DMAU, I_DMAL,

	I_FPR+0, I_FPR+1, I_FPR+2, I_FPR+3, I_FPR+4, I_FPR+5, I_FPR+6, I_FPR+7,
	I_FPR+8, I_FPR+9, I_FPR+10, I_FPR+11, I_FPR+12, I_FPR+13, I_FPR+14, I_FPR+15,
	I_FPR+16, I_FPR+17, I_FPR+18, I_FPR+19, I_FPR+20, I_FPR+21, I_FPR+22, I_FPR+23,
	I_FPR+24, I_FPR+25, I_FPR+26, I_FPR+27, I_FPR+28, I_FPR+29, I_FPR+30, I_FPR+31,

	I_PS0+0, I_PS0+1, I_PS0+2, I_PS0+3, I_PS0+4, I_PS0+5, I_PS0+6, I_PS0+7,
	I_PS0+8, I_PS0+9, I_PS0+10, I_PS0+11, I_PS0+12, I_PS0+13, I_PS0+14, I_PS0+15,
	I_PS0+16, I_PS0+17, I_PS0+18, I_PS0+19, I_PS0+20, I_PS0+21, I_PS0+22, I_PS0+23,
	I_PS0+24, I_PS0+25, I_PS0+26, I_PS0+27, I_PS0+28, I_PS0+29, I_PS0+30, I_PS0+31,

	I_PS1+0, I_PS1+1, I_PS1+2, I_PS1+3, I_PS1+4, I_PS1+5, I_PS1+6, I_PS1+7,
	I_PS1+8, I_PS1+9, I_PS1+10, I_PS1+11, I_PS1+12, I_PS1+13, I_PS1+14, I_PS1+15,
	I_PS1+16, I_PS1+17, I_PS1+18, I_PS1+19, I_PS1+20, I_PS1+21, I_PS1+22, I_PS1+23,
	I_PS1+24, I_PS1+25, I_PS1+26, I_PS1+27, I_PS1+28, I_PS1+29, I_PS1+30, I_PS1+31,
};


UIDef constants[] =
{
	{ "on", 1 }, { "off", 0 },
	{ "true", 1}, { "false", 0 },

	{ "nop", 0x60000000 },

	{ "exc_system_reset", EXCEPTION_SYSTEM_RESET },
	{ "exc_machine_check", EXCEPTION_MACHINE_CHECK },
	{ "exc_dsi", EXCEPTION_DSI },
	{ "exc_isi", EXCEPTION_ISI },
	{ "exc_external", EXCEPTION_EXTERNAL },
	{ "exc_alignment", EXCEPTION_ALIGNMENT },
	{ "exc_program", EXCEPTION_PROGRAM },
	{ "exc_fp_unavailable", EXCEPTION_FP_UNAVAILABLE },
	{ "exc_decrementer", EXCEPTION_DECREMENTER },
	{ "exc_system_call", EXCEPTION_SYSTEM_CALL },
	{ "exc_trace", EXCEPTION_TRACE },
	{ "exc_performance_monitor", EXCEPTION_PERFORMANCE_MONITOR },
	{ "exc_iabr", EXCEPTION_IABR },
	{ "exc_reserved", EXCEPTION_RESERVED },
	{ "exc_thermal", EXCEPTION_THERMAL },

	{ "int_hsp", INTERRUPT_HSP },
	{ "int_debug", INTERRUPT_DEBUG },
	{ "int_cp", INTERRUPT_CP },
	{ "int_pe_finish", INTERRUPT_PE_FINISH },
	{ "int_pe_token", INTERRUPT_PE_TOKEN },
	{ "int_vi", INTERRUPT_VI },
	{ "int_mem", INTERRUPT_MEM },
	{ "int_dsp", INTERRUPT_DSP },
	{ "int_ai", INTERRUPT_AI },
	{ "int_exi", INTERRUPT_EXI },
	{ "int_si", INTERRUPT_SI },
	{ "int_di", INTERRUPT_DI },
	{ "int_rsw", INTERRUPT_RSW },
	{ "int_error", INTERRUPT_ERROR },
	
	{ "int_pe_fin", DEBUG_PE_INT_FIN },
	{ "int_pe_tok", DEBUG_PE_INT_TOK },
	{ "int_vi_1", DEBUG_VI_INT_1 },
	{ "int_vi_2", DEBUG_VI_INT_2 },
	{ "int_vi_3", DEBUG_VI_INT_3 },
	{ "int_vi_4", DEBUG_VI_INT_4 },
	{ "int_dsp_dsp", DEBUG_DSP_INT_DSP },
	{ "int_dsp_ar", DEBUG_DSP_INT_AR },
	{ "int_dsp_aid", DEBUG_DSP_INT_AID },
	{ "int_ai_ai", DEBUG_AI_INT_AI },
	{ "int_exi_c0_tc", DEBUG_EXI_C0_INT_TC },
	{ "int_exi_c0_ext", DEBUG_EXI_C0_INT_EXT },
	{ "int_exi_c0_exi", DEBUG_EXI_C0_INT_EXI },
	{ "int_exi_c1_tc", DEBUG_EXI_C1_INT_TC },
	{ "int_exi_c1_ext", DEBUG_EXI_C1_INT_EXT },
	{ "int_exi_c1_exi", DEBUG_EXI_C1_INT_EXI },
	{ "int_exi_c2_tc", DEBUG_EXI_C2_INT_TC },
	{ "int_exi_c2_ext", DEBUG_EXI_C2_INT_EXT },
	{ "int_exi_c2_exi", DEBUG_EXI_C2_INT_EXI },
	{ "int_exi_c3_tc", DEBUG_EXI_C3_INT_TC },
	{ "int_exi_c3_ext", DEBUG_EXI_C3_INT_EXT },
	{ "int_exi_c3_exi", DEBUG_EXI_C3_INT_EXI },
	{ "int_si_tc", DEBUG_SI_INT_TC },
	{ "int_si_rdst", DEBUG_SI_INT_RDST },
	{ "int_di_brk", DEBUG_DI_INT_BRK },
	{ "int_di_tc", DEBUG_DI_INT_TC },
	{ "int_di_de", DEBUG_DI_INT_DE },
	{ "int_di_cvr", DEBUG_DI_INT_CVR },

	{ "log_int", LOG_ENABLE_INT },
	{ "log_ai", LOG_ENABLE_AI },
	{ "log_dsp", LOG_ENABLE_DSP },
	{ "log_cp", LOG_ENABLE_CP },
	{ "log_di", LOG_ENABLE_DI },
	{ "log_exi", LOG_ENABLE_EXI },
	{ "log_mi", LOG_ENABLE_MI },
	{ "log_pe", LOG_ENABLE_PE },
	{ "log_pi", LOG_ENABLE_PI },
	{ "log_si", LOG_ENABLE_SI },
	{ "log_vi", LOG_ENABLE_VI },
	
	{ "log_gx", LOG_ENABLE_GX },
	{ "log_hle", LOG_ENABLE_HLE },
	{ "log_calls", LOG_ENABLE_CALLS },
	
	{ "gxt_wireframe", GX_TOGGLE_WIREFRAME },
	{ "gxt_tex_reload", GX_TOGGLE_TEX_RELOAD },
	{ "gxt_tex_dump", GX_TOGGLE_TEX_DUMP },
	{ "gxt_force_linear", GX_TOGGLE_FORCE_LINEAR },
	{ "gxt_tex_transparent", GX_TOGGLE_TEX_TRANSPARENT },
	{ "gxt_draw", GX_TOGGLE_DRAW },
	{ "gxt_use_gl_mipmaps", GX_TOGGLE_USE_GL_MIPMAPS },
	{ "gxt_force_max_aniso", GX_TOGGLE_FORCE_MAX_ANISO },
	{ "gxt_fix_flickering", GX_TOGGLE_FIX_FLICKERING },
	{ "gxt_fix_blending", GX_TOGGLE_FIX_BLENDING },
};


UIPDef variables[] =
{
	{ "INTSR", &INTSR },
	{ "INTMR", &INTMR },

	{ "code", &debugger.code_address },
	{ "mem", &debugger.mem_address },
	{ "log_enable", &debugger.log_enable },
	{ "refresh_rate", &debugger.refresh_rate },
	{ "refresh_delay", &debugger.refresh_delay },
	{ "min_refresh", &debugger.min_refresh },

	{ "last_pc", &debugger.last_pc },
	{ "output_code_lines", &debugger.output_code_lines },
	{ "result", &debugger.result },
};


// file browser
char *path_compact (char *path, char *rel)
{
	char *buff, *s;


	if (path && (0 == strcmp (rel, "..")) && (path[strlen (path) - 1] != '.'))
	{
		s = strrchr (path, '/');
		if (s)
		{
			buff = calloc (1, strlen (path) - strlen (s) + 1);
			strncpy (buff, path, strlen (path) - strlen (s));
			return buff;
		}
	}

	if (path)
	{
		buff = calloc (1, strlen (path) + strlen (rel) + 2);
		sprintf (buff, "%s/%s", path, rel);
	}
	else
		buff = strdup (rel);

	return buff;
}


void browser_change_path (Browser *b, char *rel)
{
	struct dirent **namelist;
	struct stat attribs;
	char *new_path, buff[1024];
	int i, n;


	new_path = path_compact (b->dir.path, rel);
	n = scandir (new_path, &namelist, NULL, alphasort);
	if (n <= 0)
	{
		if (n == 0)
			free (namelist);
		
		free (new_path);
		return;
	}

	// cleanup first
	if (b->dir.path)
	{
		free (b->dir.path);
		b->dir.path = NULL;
	}

	if (b->dir.items)
	{
		for (i = 0; i < b->dir.nitems; i++)
			free (b->dir.items[i].name);

		free (b->dir.items);
		b->dir.items = NULL;
		b->dir.nitems = 0;
	}

	b->dir.path = new_path;
	b->nitems = b->dir.nitems = --n;
	b->dir.items = calloc (b->nitems, sizeof (DirectoryItem));

	// no '.' entry
	// first time, only directories
	for (i = 0, n = 0; i < b->nitems; i++)
	{
		sprintf (buff, "%s/%s", b->dir.path, namelist[i + 1]->d_name);
		if (!((0 == stat (buff, &attribs)) && S_ISDIR (attribs.st_mode)))
			continue;

		b->dir.items[n].name = strdup (namelist[i + 1]->d_name);
		b->dir.items[n].type = DIR_ITEM_DIRECTORY;
		n++;
	}

	for (i = 0; i < b->nitems; i++)
	{
		sprintf (buff, "%s/%s", b->dir.path, namelist[i + 1]->d_name);
		if (!((0 == stat (buff, &attribs)) && !S_ISDIR (attribs.st_mode)))
			continue;

		b->dir.items[n].name = strdup (namelist[i + 1]->d_name);

		get_extension (buff, b->dir.items[n].name);
		if ((0 == strcasecmp (buff, "dol")) ||
				(0 == strcasecmp (buff, "elf")) ||
				(0 == strcasecmp (buff, "bin")))
			b->dir.items[n].type = DIR_ITEM_EXECUTABLE;
		else if ((0 == strcasecmp (buff, "gcm")) ||
						 (0 == strcasecmp (buff, "imp")) ||
						 (0 == strcasecmp (&b->dir.items[n].name[strlen (b->dir.items[n].name) - 6], "gcm.gz")))
			b->dir.items[n].type = DIR_ITEM_GCM;
		else
			b->dir.items[n].type = DIR_ITEM_FILE;
		n++;
		
		free (namelist[i + 1]);
	}
	free (namelist[0]);
	free (namelist);
}


void browser_show (Window *win, Browser *b, int nlines)
{
	int i, pos = b->start;
	char buff[256];


	if (b->dvd_dir)
	{
		DirItem *p = dir_get_item (b->current_dir, pos);

		for (i = win->nlines - nlines; i < win->nlines; i++)
		{
			window_clear_line (win, i);
			if (!p)
				continue;

			if (pos == b->pos)
				window_line_set_attribs (win, i, 0, 80, FG_MAGENTA, ATTRIB_STANDOUT);
			else if (p->type == DIR_ITEM_DIRECTORY)
				window_line_set_attribs (win, i, 0, 80, FG_GREEN, 0);

			window_printf (win, 3, i, p->name);
			
			pos++;
			p = p->next;
		}

		window_title_clear (win, 10, 80);
		window_title_printf (win, 10, "BROWSING DVD");
	}
	else
	{
		for (i = win->nlines - nlines; i < win->nlines; i++)
		{
			window_clear_line (win, i);
			if (pos >= b->nitems)
				continue;

			if (pos == b->pos)
				window_line_set_attribs (win, i, 0, 80, FG_MAGENTA, ATTRIB_STANDOUT);
			else if (b->dir.items[pos].type == DIR_ITEM_DIRECTORY)
				window_line_set_attribs (win, i, 0, 80, FG_GREEN, 0);
			else if (b->dir.items[pos].type == DIR_ITEM_EXECUTABLE)
				window_line_set_attribs (win, i, 0, 80, FG_RED, 0);

			window_printf (win, 3, i, b->dir.items[pos++].name);
		}

		window_title_clear (win, 10, 80);

		get_filename (buff, b->dir.path);
		window_title_printf (win, 10, "%s", buff);
	}
}


int on_browser_keypress (Window *win, int key, int modifiers, void *data)
{
	switch (key)
	{
		case 'i':
			if (!debugger.browser.dvd_dir &&
					debugger.browser.dir.items[debugger.browser.pos].type != DIR_ITEM_DIRECTORY)
			{
				char buff[1024];
				sprintf (buff, "%s/%s", debugger.browser.dir.path,
										debugger.browser.dir.items[debugger.browser.pos].name);
				if (mdvd_open (buff))
					debug_print ("dvd inserted");
				else
					debug_printf ("error inserting %s", buff);

				screen_redraw (&debugger.screen);
			}
			else if (!debugger.browser.dvd_dir)
			{
				char buff[1024];
				sprintf (buff, "%s/%s", debugger.browser.dir.path,
										debugger.browser.dir.items[debugger.browser.pos].name);
				if (vdvd_open (buff))
					debug_print ("dvd root mounted");
				else
					debug_printf ("error mounting %s", buff);

				screen_redraw (&debugger.screen);
			}
			return TRUE;

		case 'd':
			if (debugger.browser.dvd_dir)
			{
				DirItem *p = dir_get_item (debugger.browser.current_dir, debugger.browser.pos);

				if (p->type == DIR_ITEM_FILE)
				{
					char filename[1024];
					FILE *f;

					strcpy (filename, p->name);
					f = fopen (filename, "wb");
					if (!f)
					{
						sprintf (filename, "%s/%s", get_home_dir (), p->name);
						f = fopen (filename, "wb");
					}
					
					if (f)
					{
						char *buff = malloc (DUMP_BUFFER_SIZE);
						__u32 size = p->size;

						file_seek (debugger.browser.file, p->offset, SEEK_SET);
						while (size > DUMP_BUFFER_SIZE)
						{
							file_read (debugger.browser.file, buff, DUMP_BUFFER_SIZE);
							fwrite (buff, 1, DUMP_BUFFER_SIZE, f);
							size -= DUMP_BUFFER_SIZE;
						}

						file_read (debugger.browser.file, buff, size);
						fwrite (buff, 1, size, f);

						fclose (f);
						free (buff);

						debug_printf ("dumped file %s", filename);
					}
					else
						debug_printf ("error while dumping file %s: %s", p->name, strerror (errno));

					screen_redraw (&debugger.screen);
				}
			}
			return TRUE;
	
		// enter
		case 0x0d:
			if (debugger.browser.dvd_dir)
			{
				DirItem *p = dir_get_item (debugger.browser.current_dir,
																		debugger.browser.pos);

				if (p->type == DIR_ITEM_DIRECTORY)
				{
					if (p->sub)
					{
						debugger.browser.current_dir->last_pos = debugger.browser.pos;
						debugger.browser.current_dir = p->sub;
					}
					else
						debugger.browser.current_dir = p->parent;

					if (debugger.browser.current_dir)
					{
						debugger.browser.nitems = debugger.browser.current_dir->nitems;
						debugger.browser.pos = debugger.browser.start = 
														debugger.browser.current_dir->last_pos;
						debugger.browser.start = debugger.browser.pos -
																			debugger.code_win->nlines + 1;
						if (debugger.browser.start < 0)
							debugger.browser.start = 0;
					}
					else
					{
						dir_destroy (debugger.browser.dvd_dir);
						debugger.browser.dvd_dir = NULL;
						debugger.browser.pos = debugger.browser.start = 0;
						debugger.browser.nitems = debugger.browser.dir.nitems;
						
						file_close (debugger.browser.file);
						debugger.browser.file = NULL;
					}

					screen_redraw_window (&debugger.screen, win);
				}
			}
			else switch (debugger.browser.dir.items[debugger.browser.pos].type)
			{
				case DIR_ITEM_DIRECTORY:
					browser_change_path (&debugger.browser,
														debugger.browser.dir.items[debugger.browser.pos].name);
					debugger.browser.pos = debugger.browser.start = 0;
					screen_redraw_window (&debugger.screen, win);
					break;
				
				case DIR_ITEM_EXECUTABLE:
					{
						char buff[1024];

						sprintf (buff, "%s/%s", debugger.browser.dir.path,
											debugger.browser.dir.items[debugger.browser.pos].name);
						
						if (gcube_load_file (buff))
							debugger_cmd_detach (NULL);

						screen_redraw (&debugger.screen);
					}
					break;
				
				case DIR_ITEM_GCM:
					{
						char buff[1024];
						Dir *dir;

						sprintf (buff, "%s/%s", debugger.browser.dir.path,
											debugger.browser.dir.items[debugger.browser.pos].name);

						if ((dir = mdvd_load_directory (buff)))
						{
							debugger.browser.current_dir = debugger.browser.dvd_dir = dir;
							debugger.browser.pos = debugger.browser.start = 0;
							debugger.browser.nitems = debugger.browser.dvd_dir->nitems;
							debugger.browser.file = file_open (buff);
							
							screen_redraw_window (&debugger.screen, win);
						}
					}
					break;
			}
			return TRUE;
		
		case KEY_DOWN:
			if (debugger.browser.pos < debugger.browser.nitems - 1)
			{
				debugger.browser.pos++;
				if (debugger.browser.pos - debugger.browser.start  + 1 > win->nlines)
					debugger.browser.start++;
				screen_redraw_window (&debugger.screen, win);
			}
			return TRUE;

		case KEY_UP:
			if (debugger.browser.pos > 0)
			{
				debugger.browser.pos--;
				if (debugger.browser.pos < debugger.browser.start)
					debugger.browser.start--;
				screen_redraw_window (&debugger.screen, win);
			}
			return TRUE;

		case KEY_NPAGE:
			if (debugger.browser.pos < debugger.browser.nitems - 1)
			{
				debugger.browser.pos += win->nlines - 1;
				if (debugger.browser.pos >= debugger.browser.nitems - 1)
					debugger.browser.pos = debugger.browser.nitems - 1;

				debugger.browser.start = debugger.browser.pos - win->nlines + 1;
				if (debugger.browser.start < 0)
					debugger.browser.start = 0;

				screen_redraw_window (&debugger.screen, win);
			}
			return TRUE;

		case KEY_PPAGE:
			if (debugger.browser.pos > 0)
			{
				debugger.browser.pos -= win->nlines - 1;
				if (debugger.browser.pos < 0)
					debugger.browser.pos = 0;
				if (debugger.browser.pos < debugger.browser.start)
					debugger.browser.start = debugger.browser.pos;
				screen_redraw_window (&debugger.screen, win);
			}
			return TRUE;

		case KEY_HOME:
			if (debugger.browser.pos > 0)
			{
				debugger.browser.pos = debugger.browser.start = 0;
				screen_redraw_window (&debugger.screen, win);
			}
			return TRUE;
		
		case KEY_END:
			if (debugger.browser.pos < debugger.browser.nitems - 1)
			{
				debugger.browser.pos = debugger.browser.nitems - 1;

				debugger.browser.start = debugger.browser.pos - win->nlines + 1;
				if (debugger.browser.start < 0)
					debugger.browser.start = 0;
				screen_redraw_window (&debugger.screen, win);
			}
			return TRUE;

		case KEY_RIGHT:
			cwin_change_mode (debugger.cmode + 1);
			return TRUE;

		case KEY_LEFT:
			cwin_change_mode (debugger.cmode - 1);
			return TRUE;

		default:
			return FALSE;
	}
}


void on_browser_resize (Window *win, int nlines, void *data)
{
	if (nlines < debugger.browser.pos - debugger.browser.start + 1)
	{
		debugger.browser.pos -=
							(debugger.browser.pos - debugger.browser.start + 1) - nlines;

		if (debugger.browser.pos < 0)
			debugger.browser.pos = 0;
		if (debugger.browser.pos < debugger.browser.start)
			debugger.browser.start = debugger.browser.pos;
	}

	browser_show (win, &debugger.browser, win->nlines);
}


void on_browser_refresh (Window *win, void *data)
{
	browser_show (win, &debugger.browser, win->nlines);
}


unsigned int bin_to_dec (char *s)
{
	unsigned int i, len, res = 0;


	len = strlen (s);
	for (i = 0; i < len; i++)
		res += (s[i] - '0') * (1 << (len - i - 1));

	return res;
}


// returns real index (after wrapping)
int output_item_index (OutputWin *out, int n)
{
	int nn;


	nn = n + out->nfirst_item;
	if (nn >= out->nitems)
		nn -= out->nitems;
	
	return nn;
}


// returns number of characters extracted
int args_extract_uint (char *args, unsigned int *d)
{
	char buff[LINE_WIDTH + 1];
	int n = 0;


	// try hex
	if (1 <= sscanf (args, " 0x%x%n", d, &n))
		return n + 1;

	// try bin
	if (1 <= sscanf (args, " 0b%s%n", buff, &n))
	{
		*d = bin_to_dec (buff);
		return n + 1;
	}
	
	// try dec
	if (1 <= sscanf (args, " %d%n", d, &n))
		return n + 1;

	return 0;
}


int args_extract_int (char *args, int *d)
{
	char buff[LINE_WIDTH + 1];
	int n = 0;


	// try hex
	if (1 <= sscanf (args, " 0x%x%n", d, &n))
		return n + 1;

	// try bin
	if (1 <= sscanf (args, " 0b%s%n", buff, &n))
	{
		*d = bin_to_dec (buff);
		return n + 1;
	}
	
	// try dec
	if (1 <= sscanf (args, " %d%n", d, &n))
		return n + 1;

	return 0;
}

// hex value will be trated as hex representation of double value
int args_extract_double (char *args, double *d)
{
	char buff[LINE_WIDTH + 1];
	int n = 0;
	__u64 a;


	// try hex
	if (1 <= sscanf (args, " 0x%llx%n", &a, &n))
	{
		*d = ((Double64) a).fp;
		return n + 1;
	}

	// try bin
	if (1 <= sscanf (args, " 0b%s%n", buff, &n))
	{
		*d = bin_to_dec (buff);
		return n + 1;
	}
	
	// try float
	if (1 <= sscanf (args, " %lf%n", d, &n))
		return n + 1;

	return 0;
}


int args_extract_str (char *args, char *d)
{
	int n = 0;


	if (1 <= sscanf (args, "%s%n", d, &n))
		return n + 1;

	return 0;
}


int args_extract_constant (char *args, unsigned int *c)
{
	char r[LINE_WIDTH + 1] = {0};
	int i, n;


	if ((n = args_extract_str (args, r)))
		for (i = 0; i < sizeof (constants) / sizeof (UIDef); i++)
			if (0 == strcasecmp (r, constants[i].s))
			{
				*c = constants[i].ui;
				return n;
			}

	return 0;
}


// gives the index of the variable
int args_extract_variable (char *args, unsigned int *index)
{
	char r[LINE_WIDTH + 1] = {0};
	int i, n;


	if ((n = args_extract_str (args, r)))
		for (i = 0; i < sizeof (variables) / sizeof (UIPDef); i++)
			if (0 == strcasecmp (r, variables[i].s))
			{
				*index = i;
				return n;
			}

	return 0;
}


// gives the index of the register (in the reg_names table)
int args_extract_reg (char *args, unsigned int *regindex)
{
	char r[LINE_WIDTH + 1] = {0};
	int i, n;


	if ((n = args_extract_str (args, r)))
		for (i = 0; i < sizeof (reg_names) / sizeof (char *); i++)
			if (0 == strcasecmp (r, reg_names[i]))
			{
				*regindex = i;
				return n;
			}

	return 0;
}


int args_extract_map_item (char *args, MapItem **mi)
{
	char r[LINE_WIDTH + 1];
	int n;
	MapItem *a;


	if ((n = args_extract_str (args, r)))
	{
		a = map_get_item_by_name (map, r);
		if (a)
		{
			*mi = a;
			return n;
		}
	}

	return 0;
}


// try to parse the string and extract uint value
int args_parse_uint (char *args, unsigned int *d)
{
	unsigned int n;
	MapItem *mi;


	if ((n = args_extract_reg (args, d)))
	{
		if (reg_indices[*d] < I_FPR)
			*d = REGS (reg_indices[*d]);
		else if (reg_indices[*d] < I_PS0)
			*d = FPREGS (reg_indices[*d] - I_FPR);
		else if (reg_indices[*d] < I_PS1)
			*d = FPREGS_PS0 (reg_indices[*d] - I_PS0);
		else
			*d = FPREGS_PS1 (reg_indices[*d] - I_PS1);

		return n;
	}
	else if ((n = args_extract_constant (args, d)))
		return n;
	else if ((n = args_extract_variable (args, d)))
	{
		*d = *variables[*d].uip;
		return n;
	}
	else if ((n = args_extract_uint (args, d)))
		return n;
	else if ((n = args_extract_map_item (args, &mi)))
	{
		*d = mi->address;
		return n;
	}

	return 0;
}


// try to parse the string and extract int value
int args_parse_int (char *args, int *d)
{
	unsigned int n;
	MapItem *mi;


	if ((n = args_extract_reg (args, d)))
	{
		if (reg_indices[*d] < I_FPR)
			*d = REGS (reg_indices[*d]);
		else if (reg_indices[*d] < I_PS0)
			*d = FPREGS (reg_indices[*d] - I_FPR);
		else if (reg_indices[*d] < I_PS1)
			*d = FPREGS_PS0 (reg_indices[*d] - I_PS0);
		else
			*d = FPREGS_PS1 (reg_indices[*d] - I_PS1);

		return n;
	}
	else if ((n = args_extract_constant (args, d)))
		return n;
	else if ((n = args_extract_variable (args, d)))
	{
		*d = *variables[*d].uip;
		return n;
	}
	else if ((n = args_extract_int (args, d)))
		return n;
	else if ((n = args_extract_map_item (args, &mi)))
	{
		*d = mi->address;
		return n;
	}

	return 0;
}


// try to parse the string and extract double value
int args_parse_double (char *args, double *d)
{
	unsigned int n, k;
	MapItem *mi;


	if ((n = args_extract_reg (args, &k)))
	{
		if (reg_indices[k] < I_FPR)
			*d = REGS (reg_indices[k]);
		else if (reg_indices[k] < I_PS0)
			*d = FPREGS (reg_indices[k] - I_FPR);
		else if (reg_indices[k] < I_PS1)
			*d = FPREGS_PS0 (reg_indices[k] - I_PS0);
		else
			*d = FPREGS_PS1 (reg_indices[k] - I_PS1);

		return n;
	}
	else if ((n = args_extract_constant (args, &k)))
	{
		*d = k;
		return n;
	}
	else if ((n = args_extract_variable (args, &k)))
	{
		*d = *variables[k].uip;
		return n;
	}
	else if ((n = args_extract_double (args, d)))
		return n;
	else if ((n = args_extract_map_item (args, &mi)))
	{
		*d = mi->address;
		return n;
	}

	return 0;
}


int args_parse_subexpression (char *args, unsigned int *d)
{
	char buff[LINE_WIDTH + 1];
	char op[LINE_WIDTH + 1];
	int n = 0, k = 0, b = 0;


	sscanf (args, " %[^*/+^%&~|<> ]%n", buff, &n);
	if (!n || !args_parse_uint (buff, d))
		return 0;

	while (1)
	{
		k = 0;
		*op = '\0';
		sscanf (args + n, " %[*/-+^%&~|<>]%n", op, &k);
		if (!k)
			return n;
		
		n += k;
		k = 0;
		sscanf (args + n, " %[^*/-+^%&~|<> ]%n", buff, &k);
		if (!k || !args_parse_uint (buff, &b))
			return n;

		switch (*op)
		{
			case '*':
				*d *= b;
				break;
			
			case '/':
				*d /= b;
				break;

			case '^':
				*d ^= b;
				break;
		
			case '%':
				*d %= b;
				break;
		
			case '&':
				if (op[1] == '~')
					*d = *d &~ b;
				else
					*d &= b;
				break;
		
			case '|':
				*d |= b;
				break;
		
			case '<':
				if (op[1] == '<')
					*d <<= b;
				break;
		
			case '>':
				if (op[1] == '>')
					*d >>= b;
				break;
		
			default:
				return n - 1;
		}

		n += k;
	}
}


int args_parse_expression_uint (char *args, unsigned int *d)
{
	int a = 0, n = 0, k = 0;


	*d = 0;
	if (!(n = args_parse_subexpression (args, d)))
		return 0;

	while (1)
	{
		if (!(k = args_parse_subexpression (args + n + 1, &a)))
			return n + 1;

		switch (args[n])
		{
			case '+':
				n++;
				*d += a;
				break;
			
			case '-':
				n++;
				*d -= a;
				break;

			default:
				return n + 1;
		}

		n += k;
	}
}


int args_parse_subexpression_int (char *args, int *d)
{
	char buff[LINE_WIDTH + 1];
	char op[LINE_WIDTH + 1];
	int n = 0, k = 0, b = 0;


	sscanf (args, " %[^*/+^%&~|<> ]%n", buff, &n);
	if (!n || !args_parse_int (buff, d))
		return 0;

	while (1)
	{
		k = 0;
		*op = '\0';
		sscanf (args + n, " %[*/-+^%&~|<>]%n", op, &k);
		if (!k)
			return n;
		
		n += k;
		k = 0;
		sscanf (args + n, " %[^*/-+^%&~|<> ]%n", buff, &k);
		if (!k || !args_parse_int (buff, &b))
			return n;

		switch (*op)
		{
			case '*':
				*d *= b;
				break;
			
			case '/':
				*d /= b;
				break;

			case '^':
				*d ^= b;
				break;
		
			case '%':
				*d %= b;
				break;
		
			case '&':
				if (op[1] == '~')
					*d = *d &~ b;
				else
					*d &= b;
				break;
		
			case '|':
				*d |= b;
				break;
		
			case '<':
				if (op[1] == '<')
					*d <<= b;
				break;
		
			case '>':
				if (op[1] == '>')
					*d >>= b;
				break;
		
			default:
				return n - 1;
		}

		n += k;
	}
}


int args_parse_expression_int (char *args, int *d)
{
	int a = 0, n = 0, k = 0;


	*d = 0;
	if (!(n = args_parse_subexpression_int (args, d)))
		return 0;

	while (1)
	{
		if (!(k = args_parse_subexpression_int (args + n + 1, &a)))
			return n + 1;

		switch (args[n])
		{
			case '+':
				n++;
				*d += a;
				break;
			
			case '-':
				n++;
				*d -= a;
				break;

			default:
				return n + 1;
		}

		n += k;
	}
}


int args_parse_subexpression_double (char *args, double *d)
{
	char buff[LINE_WIDTH + 1];
	char op[LINE_WIDTH + 1];
	int n = 0, k = 0;
	double b;


	sscanf (args, " %[^*/+^&| ]%n", buff, &n);
	if (!n || !args_parse_double (buff, d))
		return 0;

	while (1)
	{
		k = 0;
		*op = '\0';
		sscanf (args + n, " %[*/-+^&|]%n", op, &k);
		if (!k)
			return n;
		
		n += k;
		k = 0;
		sscanf (args + n, " %[^*/-+^&| ]%n", buff, &k);
		if (!k || !args_parse_double (buff, &b))
			return n;

		switch (*op)
		{
			case '*':
				*d *= b;
				break;
			
			case '/':
				*d /= b;
				break;

			case '^':
				*d = ((Double64)((((Double64) *d).bin) ^ (((Double64) b).bin))).fp;
				break;
		
			case '&':
				*d = ((Double64)((((Double64) *d).bin) & (((Double64) b).bin))).fp;
				break;
		
			case '|':
				*d = ((Double64)((((Double64) *d).bin) | (((Double64) b).bin))).fp;
				break;
		
			default:
				return n - 1;
		}

		n += k;
	}
}


int args_parse_expression_double (char *args, double *d)
{
	int n = 0, k = 0;
	double a;


	*d = 0;
	if (!(n = args_parse_subexpression_double (args, d)))
		return 0;

	while (1)
	{
		if (!(k = args_parse_subexpression_double (args + n + 1, &a)))
			return n + 1;

		switch (args[n])
		{
			case '+':
				n++;
				*d += a;
				break;
			
			case '-':
				n++;
				*d -= a;
				break;

			default:
				return n + 1;
		}

		n += k;
	}
}


// check if code breakpoint has been reached
inline Breakpoint *breakpoint_reached (__u32 address)
{
	Breakpoint *bp = debugger.breakpoints[(address & MEM_MASK) >> 2];


	if (bp && (bp->type == BP_CODE) && bp->enabled)
	{
		if (bp->flags & BP_FLAGS_CMP_R3)
		{
			if (GPR[3] == bp->val)
				return bp;
			else
				return NULL;
		}

		return bp;
	}
	else
		return NULL;
}


void output_code_line (__u32 addr)
{
	char opcode[128], operand[128], buff[LINE_WIDTH + 1];
	__u32 op = MEM32_SAFE (addr), t;
	MapItem *mi;


	disassemble (opcode, operand, op, addr, &t);

	strcpy (buff, "            ");
	strncpy (buff, opcode, strlen (opcode));

	if ((t != addr + 4) && (mi = map_get_item_by_address (map, t)))
		debug_printf ("%.8x  %.8x     %s%s", addr, op, buff, mi->name);
	else
		debug_printf ("%.8x  %.8x     %s%s", addr, op, buff, operand);

	if (0 == strncmp (opcode, "blr", 3))
		debug_print ("");
}


void debugger_cmd_run (char *args)
{
	int i = 0;
	MapItem *mi;
	Breakpoint *bp;


	gdebug_running = TRUE;

	while (gdebug_running)
	{
		if ((mi = map_get_item_by_address (map, PC)))
			DEBUG (EVENT_LOG_CALLS, "<%.8x> %s", LR, mi->name);

		if (debugger.output_code_lines)
			output_code_line (PC);

		cpu_execute ();

		if (i++ > debugger.refresh_rate)
		{
			debugger.code_address = PC;
			screen_redraw (&debugger.screen);
			i = 0;
		}
		
		if ((bp = breakpoint_reached (PC)))
		{
			debug_printf ("breakpoint %d reached", bp->num);
			break;
		}
	}

	debugger.code_address = PC;
	
	if (do_save_state)
	{
		do_save_state = FALSE;
		gcube_save_state_delayed ();
		debugger_cmd_run (NULL);
	}
}


// run the code until program conunter reaches next instruction
// used to execute functions
void debugger_cmd_step_over (char *args)
{
	__u32 next = PC + 4;


	gdebug_running = TRUE;

	while (gdebug_running)
	{
		cpu_execute ();

		if (PC == next)
			break;
	}

	gdebug_running = FALSE;	
	debugger.code_address = PC;
}


void debugger_cmd_mem (char *args)
{
	__u32 n, address, data;


	if ((n = args_parse_expression_uint (args, &address)))
	{
		if (args_parse_expression_uint (args + n, &data))
		{
			switch (address % 4)
			{
				case 0:
					MEM_WWORD (address, data);
					break;
				
				case 2:
					MEM_WHALF (address, data);
					break;

				case 1:
				case 3:
					MEM_WBYTE (address, data);
					break;
			}
		}

		debugger.mem_address = address;
	}
	else
		debug_print ("* usage: mem address [data]");
}


void debugger_cmd_mfill (char *args)
{
	__u32 n, data, address, size;


	if ((n = args_parse_expression_uint (args, &address)))
	{
		if ((n += args_parse_expression_uint (args + n, &size)))
		{
			if (args_parse_expression_uint (args + n, &data))
				memset (MEM_ADDRESS (address), (__u8) data, size);
			else
				memset (MEM_ADDRESS (address), 0, size);

			debugger.mem_address = address;
			return;
		}
	}

	debug_print ("* usage: mfill address size [data]");
}


void debugger_cmd_hww (char *args)
{
	__u32 n, address, data;


	if ((n = args_parse_expression_uint (args, &address)))
	{
		if (args_parse_expression_uint (args + n, &data))
			hw_wword (address, data);

		if (hw_rword (address, &data))
			debug_printf ("hw[%.4x] = %.8x", address & 0xffff, data);
		else
			debug_print ("can't access this register by 32 bits");
	}
	else
		debug_print ("* usage: hww address [data]");
}


void debugger_cmd_hwh (char *args)
{
	__u32 n, address, data_32;
	__u16 data;


	if ((n = args_parse_expression_uint (args, &address)))
	{
		if (args_parse_expression_uint (args + n, &data_32))
			hw_whalf (address, (__u16) data_32);

		if (hw_rhalf (address, &data))
			debug_printf ("hw[%.4x] = %.4x", address & 0xffff, data);
		else
			debug_print ("can't access this register by 16 bits");
	}
	else
		debug_print ("* usage: hwh address [data]");
}


void debugger_cmd_code (char *args)
{
	__u32 d;


	if (args_parse_expression_uint (args, &d))
		debugger.code_address = d;
	else
		debug_print ("* usage: code address");
}


void debugger_cmd_reload (char *args)
{
	hle (map, FALSE);

	gcube_load_file (gcbin_filename);

	if (PC == 0)
	{
		debug_printf ("error: can't load file %s", gcbin_filename);
		PC = debugger.code_address = debugger.starting_pc;
		hle_enabled = FALSE;
	}
	else
		debugger.code_address = debugger.starting_pc = PC;
}


void debugger_cmd_load (char *args)
{
	char buff[256];


	if (!args_extract_str (args, buff))
	{
		debug_print ("* usage: load file");
		return;
	}
	
	if (!file_exists (buff))
	{
		debug_printf ("error: file %s does not exist", buff);
		return;
	}

	strcpy (gcbin_filename, buff);
	debugger_cmd_reload (args);
}


void debugger_print_msr (void)
{
	__u32 msr_bitmasks[] =
	{
		MSR_POW, MSR_ILE, MSR_EE, MSR_PR, MSR_FP, MSR_ME, MSR_FE0, MSR_SE, MSR_BE,
		MSR_FE1, MSR_IP, MSR_IR, MSR_DR, MSR_RI, MSR_LE,
	};
	char *msr_bitnames[] = 
	{
		"POW", "ILE", "EE", "PR", "FP", "ME", "FE0", "SE", "BE",
		"FE1", "IP", "IR", "DR", "RI", "LE",
	};
	char buff[LINE_WIDTH + 1] = {0};
	int i;
	
	
	for (i = 0; i < sizeof (msr_bitmasks) / 4; i++)
		if (MSR & msr_bitmasks[i])
		{
			strcat (buff, msr_bitnames[i]);
			strcat (buff, " ");
		}

	if (strlen (buff))
		debug_printf ("MSR = 0x%.8x -> %s", MSR, buff);
	else
		debug_printf ("MSR = 0x%.8x", MSR);
}


void debugger_print_cr (void)
{
	__u32 bitmasks[] =
	{
		CR_LT, CR_GT, CR_EQ, CR_SO,
	};
	char *bitnames[] = 
	{
		"L", "G", "E", "O",
	};
	char buff[2*LINE_WIDTH] = {0};
	char temp1[LINE_WIDTH + 1] = {0};
	char temp2[16];
	int i, j, k;


	for (i = 0; i < 8; i++)
	{
		k = GET_FIELD (CR, i);
		*temp2 = '\0';

		for (j = 0; j < 4; j++)
		{
			if (k & bitmasks[j])
			{
				strcat (temp2, bitnames[j]);
				strcat (temp2, " ");
			}
		}

		if (strlen (temp2))
		{
			temp2[strlen (temp2) - 1] = '\0';
			sprintf (temp1, "%d(%s) ", i, temp2);
			strcat (buff, temp1);
		}
	}

	if (strlen (buff))
		debug_printf ("CR = 0x%.8x -> %s", CR, buff);
	else
		debug_printf ("CR = 0x%.8x", CR);
}


void debugger_print_intmask (__u32 b, int index)
{
	char buff[LINE_WIDTH + 1] = {0};


	sprint_intmask (buff, b);
	if (strlen (buff))
		debug_printf ("%s = 0x%.8x -> %s", variables[index].s, b, buff);
	else
		debug_printf ("%s = 0x%.8x", variables[index].s, b);
}


void debugger_cmd_reg (char *args)
{
	unsigned int n, val, index, ri;
	double fval;


	if ((n = args_extract_reg (args, &index)))
	{
		ri = reg_indices[index];
		
		if (ri < I_FPR)
		{
			if ((n = args_parse_expression_uint (args + n, &val)))
				REGS (ri) = val;
		}
		else if (ri < I_PS0)
		{
			if ((n = args_parse_expression_double (args + n, &fval)))
				FPREGS (ri - I_FPR) = fval;
		}
		else if (ri < I_PS1)
		{
			if ((n = args_parse_expression_double (args + n, &fval)))
				FPREGS_PS0 (ri - I_PS0) = fval;
		}
		else
		{
			if ((n = args_parse_expression_double (args + n, &fval)))
				FPREGS_PS1 (ri - I_PS1) = fval;
		}

		switch (ri)
		{
			case I_MSR:
				debugger_print_msr ();
				break;

			case I_CR:
				debugger_print_cr ();
				break;

			case I_PC:
				if (n)
					debugger.code_address = PC;
				else
					debug_printf ("%s = 0x%.8x", reg_names[index], REGS (ri));
				break;

			default:
				if (ri < I_FPR)
					debug_printf ("%s = 0x%.8x", reg_names[index], REGS (ri));
				else if (ri < I_PS0)
					debug_printf ("%s = %f", reg_names[index], FPREGS (ri - I_FPR));
				else if (ri < I_PS1)
					debug_printf ("%s = %f", reg_names[index], FPREGS_PS0 (ri - I_PS0));
				else
					debug_printf ("%s = %f", reg_names[index], FPREGS_PS1 (ri - I_PS1));
		}
	}
	else if ((n = args_extract_variable (args, &ri)))
	{
		if ((n = args_parse_expression_uint (args + n, &val)))
			*variables[ri].uip = val;

		switch (ri)
		{
			case VAR_INTSR:
				debugger_print_intmask (INTSR, ri);
				break;
				
			case VAR_INTMR:
				debugger_print_intmask (INTMR, ri);
				break;
		
			default:
				debug_printf ("%s = 0x%.8x", variables[ri].s, *variables[ri].uip);
		}
	}
	else
		debug_print ("* usage: r reg [value]");
}


void debugger_cmd_exception (char *args)
{
	__u32 d;


	if (args_parse_expression_uint (args, &d))
	{
		cpu_exception (d + 4);
		debugger.code_address = PC;
	}
	else
		debug_print ("* usage: exception cpu_exception");
}

#include "hw_ai_dsp.h"
void debugger_cmd_interrupt (char *args)
{
	__u32 d;


	if (args_parse_expression_uint (args, &d))
	{
		MEM32 (EXCEPTION_EXTERNAL - 4) = BSWAP32 (0x60000000);

		switch (d)
		{
			case DEBUG_PE_INT_FIN:
				pe_generate_interrupt (PE_INTERRUPT_FINISH);
				break;

			case DEBUG_PE_INT_TOK:
				pe_generate_interrupt (PE_INTERRUPT_TOKEN);
				break;
			
			case DEBUG_VI_INT_1:
				vi_generate_interrupt (VI_INTERRUPT_1);
				break;

			case DEBUG_VI_INT_2:
				vi_generate_interrupt (VI_INTERRUPT_2);
				break;

			case DEBUG_VI_INT_3:
				vi_generate_interrupt (VI_INTERRUPT_3);
				break;

			case DEBUG_VI_INT_4:
				vi_generate_interrupt (VI_INTERRUPT_4);
				break;

			case DEBUG_DSP_INT_DSP:
				dsp_mail_push (0xdcd10001);
				dsp_generate_interrupt (DSP_INTERRUPT_DSP);
				break;

			case DEBUG_DSP_INT_AR:
				dsp_generate_interrupt (DSP_INTERRUPT_AR);
				break;

			case DEBUG_DSP_INT_AID:
				dsp_generate_interrupt (DSP_INTERRUPT_AID);
				break;

			case DEBUG_AI_INT_AI:
				ai_generate_interrupt ();
				break;

			case DEBUG_EXI_C0_INT_TC:
				exi_generate_interrupt (EXI_INTERRUPT_TC, 0);
				break;

			case DEBUG_EXI_C0_INT_EXT:
				exi_generate_interrupt (EXI_INTERRUPT_EXT, 0);
				break;

			case DEBUG_EXI_C0_INT_EXI:
				exi_generate_interrupt (EXI_INTERRUPT_EXI, 0);
				break;

			case DEBUG_EXI_C1_INT_TC:
				exi_generate_interrupt (EXI_INTERRUPT_TC, 0);
				break;

			case DEBUG_EXI_C1_INT_EXT:
				exi_generate_interrupt (EXI_INTERRUPT_EXT, 0);
				break;

			case DEBUG_EXI_C1_INT_EXI:
				exi_generate_interrupt (EXI_INTERRUPT_EXI, 0);
				break;

			case DEBUG_EXI_C2_INT_TC:
				exi_generate_interrupt (EXI_INTERRUPT_TC, 0);
				break;

			case DEBUG_EXI_C2_INT_EXT:
				exi_generate_interrupt (EXI_INTERRUPT_EXT, 0);
				break;

			case DEBUG_EXI_C2_INT_EXI:
				exi_generate_interrupt (EXI_INTERRUPT_EXI, 0);
				break;

			case DEBUG_EXI_C3_INT_TC:
				exi_generate_interrupt (EXI_INTERRUPT_TC, 0);
				break;

			case DEBUG_EXI_C3_INT_EXT:
				exi_generate_interrupt (EXI_INTERRUPT_EXT, 0);
				break;

			case DEBUG_EXI_C3_INT_EXI:
				exi_generate_interrupt (EXI_INTERRUPT_EXI, 0);
				break;

			case DEBUG_SI_INT_TC:
				si_generate_interrupt (SI_INTERRUPT_TC);
				break;

			case DEBUG_SI_INT_RDST:
				si_generate_interrupt (SI_INTERRUPT_RDST);
				break;

			case DEBUG_DI_INT_BRK:
				di_generate_interrupt (DI_INTERRUPT_BRK);
				break;

			case DEBUG_DI_INT_TC:
				di_generate_interrupt (DI_INTERRUPT_TC);
				break;

			case DEBUG_DI_INT_DE:
				di_generate_interrupt (DI_INTERRUPT_DE);
				break;

			case DEBUG_DI_INT_CVR:
				di_generate_interrupt (DI_INTERRUPT_CVR);
				break;
		}

		debugger.code_address = PC;
	}
	else
		debug_print ("* usage: interrupt interrupt_mask");
}


void sprint_binary (char *buff, unsigned int a)
{
	int i, j;


	buff[0] = '\0';
	for (j = 0; j < 8; j++)
	{
		for (i = 0; i < 4; i++, a <<= 1)
			strcat (buff, (a & 0x80000000) ? "1" : "0");

		if (j < 7)
			strcat (buff, "-");
	}
}


void debugger_cmd_print (char *args)
{
	unsigned int n;
	__u32 k = 0;
	char buff[LINE_WIDTH + 1];


	if ((n = args_parse_expression_uint (args, &k)))
	{
		sprint_binary (buff, k);
		debug_printf ("0x%.8x\t0b%s\t%d", k, buff, (__s32) k);
		debugger.result = k;
	}
	else
		debug_print ("* usage: print expression");
}


void debugger_cmd_print_double (char *args)
{
	unsigned int n;
	double d;


	if ((n = args_parse_expression_double (args, &d)))
	{
		debug_printf ("0x%.16llx\t%lf\t%d", ((Double64) d).bin, d, (int) d);
		debugger.result = (__u32) d;
	}
	else
		debug_print ("* usage: print expression");
}


void debugger_cmd_set (char *args)
{
	__u32 n, k, l;


	if ((n = args_extract_variable (args, &k)))
	{
		if ((n = args_parse_expression_uint (args + n, &l)))
		{
			*variables[k].uip = l;

			debugger_cmd_print (args);
			return;
		}
		else
		{
			debugger_cmd_print (args);
			return;
		}
	}

	debug_print ("* usage: set variable [value]");
}


void debugger_cmd_clear (char *args)
{
	output_destroy (&debugger.output);
	debugger.output.pos = 0;
	debugger.output.nfirst_item = 0;

	window_clear (debugger.output_win);
	screen_redraw_window (&debugger.screen, debugger.output_win);
}


Breakpoint *breakpoint_get (unsigned int num, int with_hidden)
{
	if ((num < MAX_BREAKPOINTS) && debugger.bpbynum[num] &&
			(!debugger.bpbynum[num]->hidden ||
			(debugger.bpbynum[num]->hidden && with_hidden)))
		return debugger.bpbynum[num];
	else
		return NULL;
}


Breakpoint *breakpoint_get_by_address (__u32 address, int with_hidden)
{
	unsigned int i;
	Breakpoint *bp;


	for (i = 0; i < debugger.nbreakpoints + debugger.nhwbreakpoints; i++)
	{
		if ((bp = debugger.bpbynum[i]) && (bp->address == address))
		{
			if (!bp->hidden || (bp->hidden && with_hidden))
				return bp;
			else
				return NULL;
		}
	}
	
	return NULL;
}


Breakpoint *debugger_add_breakpoint (__u32 address, int type)
{
	Breakpoint *bp;
	MapItem *mi;


	if (address >= 0xcc000000)
	{
		// check hw breakpoints
		if ((bp = debugger.hwbreakpoints[address & 0xffff]))
		{
			bp->type |= type;
			bp->enabled = TRUE;
			bp->hidden = FALSE;
		}
		else
		{
			bp = calloc (1, sizeof (Breakpoint));
			bp->type = type;
			bp->address = address;
			bp->enabled = TRUE;
			bp->num = debugger.nhwbreakpoints++ + BREAKPOINT_FIRST_HW;
			debugger.hwbreakpoints[address & 0xffff] = bp;
		}
	}
	else
	{
		if ((bp = debugger.breakpoints[(address & MEM_MASK) >> 2]))
		{
			bp->type |= type;
			bp->enabled = TRUE;
			bp->hidden = FALSE;
		}
		else
		{
			bp = calloc (1, sizeof (Breakpoint));
			bp->type = type;
			bp->address = address;
			bp->enabled = TRUE;
			bp->num = debugger.nbreakpoints++;
			debugger.breakpoints[(address & MEM_MASK) >> 2] = bp;
		}
	}
	
	debugger.bpbynum[bp->num] = bp;
	if ((mi = map_get_item_by_address (map, bp->address)))
		debug_printf ("breakpoint %d set at %s",
											bp->num, mi->name);
	else
		debug_printf ("breakpoint %d set at 0x%.8x",
											bp->num, address);

	return bp;
}


void debugger_remove_breakpoint (Breakpoint *bp)
{
	if (bp)
	{
		bp->hidden = TRUE;
		bp->enabled = FALSE;
		bp->type = 0;
	}
}


void debugger_breakpoints_destroy (void)
{
	int i;
	Breakpoint *bp;


	for (i = 0; i < debugger.nbreakpoints + debugger.nhwbreakpoints; i++)
	{
		if (i >= debugger.nbreakpoints)
		{
			bp = breakpoint_get (i - debugger.nbreakpoints + BREAKPOINT_FIRST_HW, TRUE);
			debugger.hwbreakpoints[bp->address & 0xffff] = NULL;
		}
		else
		{
			bp = breakpoint_get (i, TRUE);
			debugger.breakpoints[(bp->address & MEM_MASK) >> 2] = NULL;
		}

		debugger.bpbynum[bp->num] = NULL;
		free (bp);
	}
	
	debugger.nbreakpoints = 0;
	debugger.nhwbreakpoints = 0;
}


// add code breakpoint
void debugger_cmd_bp (char *args)
{
	__u32 b;
	Breakpoint *bp;
	int n;


	if (!(n = args_parse_expression_uint (args, &b)))
		return debug_print ("* usage: bp code_address [desired_r3_value]");

	bp = debugger_add_breakpoint (b, BP_CODE);
	if (args_parse_expression_uint (args + n, &b))
	{
		bp->val = b;
		bp->flags |= BP_FLAGS_CMP_R3;
	}
}


// add mem read breakpoint
void debugger_cmd_bpmr (char *args)
{
	__u32 b;


	if (!args_parse_expression_uint (args, &b))
		return debug_print ("* usage: bpmr mem_address");

	debugger_add_breakpoint (b, BP_MEM_READ);
}


// add mem write breakpoint
void debugger_cmd_bpmw (char *args)
{
	__u32 b;


	if (!args_parse_expression_uint (args, &b))
		return debug_print ("* usage: bpmw mem_address");

	debugger_add_breakpoint (b, BP_MEM_WRITE);
}


// add mem change breakpoint
void debugger_cmd_bpmc (char *args)
{
	__u32 b;
	Breakpoint *bp;


	if (!args_parse_expression_uint (args, &b))
		return debug_print ("* usage: bpmc mem_address");

	bp = debugger_add_breakpoint (b, BP_MEM_CHANGE);
	bp->val = MEM32 (b);
}


// add mem read/write breakpoint
void debugger_cmd_bpm (char *args)
{
	__u32 b;


	if (!args_parse_expression_uint (args, &b))
		return debug_print ("* usage: bpm mem_address");

	debugger_add_breakpoint (b, BP_MEM_READ | BP_MEM_WRITE);
}


// clear breakpoint
void debugger_cmd_bc (char *args)
{
	unsigned int b;
	Breakpoint *bp;
	char buff[LINE_WIDTH + 1];


	if (!args_parse_expression_uint (args, &b))
	{
		if (args_extract_str (args, buff) && (*buff == '*'))
		{
			debugger_breakpoints_destroy ();
			return debug_print ("cleared all breakpoints");
		}
		else
			return debug_print ("* usage: bc breakpoint_number");
	}

	bp = breakpoint_get (b, FALSE);
	if (!bp)
		bp = breakpoint_get_by_address (b, FALSE);

	if (bp)
	{
		debug_printf ("breakpoint %d cleared", bp->num);

		debugger_remove_breakpoint (bp);
	}
}


// disable breakpoint
void debugger_cmd_bd (char *args)
{
	unsigned int b;
	Breakpoint *bp;
	char buff[LINE_WIDTH + 1];


	if (!args_parse_expression_uint (args, &b))
	{
		if (args_extract_str (args, buff) && (*buff == '*'))
		{
			// disable all breakpoints
			for (b = 0; b < debugger.nbreakpoints + debugger.nhwbreakpoints; b++)
			{
				bp = breakpoint_get (b, FALSE);
				if (bp)
					bp->enabled = FALSE;
			}

			if (b)
				debug_print ("disabled all breakpoints");
			return;
		}
		else
			return debug_print ("* usage: bd breakpoint_number");
	}

	bp = breakpoint_get (b, FALSE);
	if (!bp)
		bp = breakpoint_get_by_address (b, FALSE);

	if (bp)
	{
		debug_printf ("breakpoint %d disabled", bp->num);

		bp->enabled = FALSE;
	}
}


// enable breakpoint
void debugger_cmd_be (char *args)
{
	unsigned int b;
	Breakpoint *bp;
	char buff[LINE_WIDTH + 1];


	if (!args_parse_expression_uint (args, &b))
	{
		if (args_extract_str (args, buff) && (*buff == '*'))
		{
			// enable all breakpoints
			for (b = 0; b < debugger.nbreakpoints + debugger.nhwbreakpoints; b++)
			{
				bp = breakpoint_get (b, FALSE);
				if (bp)
					bp->enabled = TRUE;
			}

			if (b)
				debug_print ("enabled all breakpoints");
			return;
		}
		else
			return debug_print ("* usage: be breakpoint_number");
	}

	bp = breakpoint_get (b, FALSE);
	if (!bp)
		bp = breakpoint_get_by_address (b, FALSE);

	if (bp)
	{
		debug_printf ("breakpoint %d enabled", bp->num);

		bp->enabled = TRUE;
	}
}


// list all breakpoints
void debugger_cmd_bl (char *args)
{
	int i;
	Breakpoint *bp;
	char *bp_names[] =
	{
		"code", "mem read", "mem write", "mem read write", "mem change",
		"mem change & code", "mem read & change", "mem read write", "mem read write"
	};
	MapItem *mi;


	for (i = 0; i < debugger.nbreakpoints + debugger.nhwbreakpoints; i++)
	{
		if (i >= debugger.nbreakpoints)
			bp = breakpoint_get (i - debugger.nbreakpoints + BREAKPOINT_FIRST_HW, TRUE);
		else
			bp = breakpoint_get (i, TRUE);

		if (bp && !bp->hidden)
		{
			if ((mi = map_get_item_by_address (map, bp->address)))
				debug_printf ("%d: %s breakpoint at %s%s",
												bp->num, bp_names[bp->type], mi->name,
												bp->enabled ? "" : " (disabled)");
			else
				debug_printf ("%d: %s breakpoint at 0x%.8x%s",
												bp->num, bp_names[bp->type], bp->address,
												bp->enabled ? "" : " (disabled)");
		}
	}

	if (!i)
		debug_print ("no breakpoints set");
}


void debugger_cmd_codedump (char *args)
{
	unsigned int n, k;
	char filename[LINE_WIDTH + 1];
	__u32 start = 0, end = 0, t;
	char operand[64], opcode[64], buff[64];
	__u32 op;
	FILE *f;
	MapItem *mi;

	
	
	if ((n = args_extract_str (args, filename)))
	{
		if ((k = args_parse_expression_uint (args + n, &start)))
		{
			n += k;
			if ((k = args_parse_expression_uint (args + n, &end)))
			{
				if (start > end)
				{
					t = start;
					start = end;
					end = start;
				}
				
				f = fopen (filename, "w+");
				if (!f)
				{
					char path[1024];
					
					sprintf (path, "%s/%s", get_home_dir (), filename);
					strcpy (filename, path);
					f = fopen (filename, "w+");
				}

				if (f)
				{
					while (start < end)
					{
						op = MEM32_SAFE (start);
						disassemble (opcode, operand, op, start, &t);

						strcpy (buff, "            ");
						strncpy (buff, opcode, strlen (opcode));

						if ((mi = map_get_item_by_address (map, start)))
							fprintf (f, "%s:\n", mi->name);
					
						if ((t != start + 4) && (mi = map_get_item_by_address (map, t)))
							fprintf (f, "%.8x  %.8x     %s%s\n", start, op, buff, mi->name);
						else
							fprintf (f, "%.8x  %.8x     %s%s\n", start, op, buff, operand);

						if (0 == strncmp (opcode, "blr", 3))
							fprintf (f, "\n");

						start += 4;
					}

					fclose (f);
					debug_printf ("code dumped to file %s", filename);
				}
				else
					debug_printf ("error while dumping code: ", strerror (errno));

				return;
			}
		}
	}
	
	debug_print ("* usage: codedump filename start end");
}


void debugger_cmd_bpex (char *args)
{
	debugger.break_on_exception = !debugger.break_on_exception;
	debug_printf ("break on exception %s",
									(debugger.break_on_exception) ? "on" : "off");
}


void debugger_cmd_bpuhw (char *args)
{
	unsigned int n, k;
	char str[LINE_WIDTH + 1];

	
	
	if ((n = args_extract_str (args, str)))
	{
		if (!args_parse_expression_uint (args + n, &k))
			k = -1;

		if (0 == strcasecmp (str, "read"))
		{
			if (k != -1)
				debugger.break_on_uhw_read = k;
			else
				debugger.break_on_uhw_read = !debugger.break_on_uhw_read;
			debug_printf ("break on unhandled hw read: %s",
											(debugger.break_on_uhw_read) ? "on" : "off");
		}
		else if (0 == strcasecmp (str, "write"))
		{
			if (k != -1)
				debugger.break_on_uhw_write = k;
			else
				debugger.break_on_uhw_write = !debugger.break_on_uhw_write;
			debug_printf ("break on unhandled hw write: %s",
											(debugger.break_on_uhw_write) ? "on" : "off");
		}
	}
	else
		debug_print ("* usage: bphw read|write (on|off)");
}


void debugger_cmd_xfb (char *args)
{
	__u32 fb;


	if (args_parse_expression_uint (args, &fb))
	{
		video_set_framebuffer (MEM_ADDRESS (fb));
		debug_printf ("xfb set to 0x%.8x", fb);
	}
	else
		debug_print ("* usage: xfb address");
}


void debugger_cmd_outdump (char *args)
{
	char filename[LINE_WIDTH + 1];
	int i;
	FILE *f;


	if (!args_extract_str (args, filename))
		strcpy (filename, "output.log");

	f = fopen (filename, "w+");
	if (!f)
	{
		char path[1024];
		
		sprintf (path, "%s/%s", get_home_dir (), filename);
		strcpy (filename, path);
		f = fopen (filename, "w+");
	}

	if (f)
	{
		for (i = 0; i < debugger.output.nitems; i++)
			fprintf (f, "%s\n", debugger.output.items[output_item_index (&debugger.output, i)]);

		fclose (f);
		debug_printf ("output dumped to file %s", filename);
	}
	else
		debug_printf ("error dumping output: %s", strerror (errno));
}


// load binary file to memory
void debugger_cmd_dload (char *args)
{
	char filename[LINE_WIDTH + 1];
	__u32 address, size;
	int n;
	FILE *f;


	if ((n = args_extract_str (args, filename)))
	{
		if (!args_parse_expression_uint (args + n, &address))
			address = debugger.mem_address;

		size = MEM_SIZE - (address & MEM_MASK);

		f = fopen (filename, "rb");
		if (f)
		{
			// loads at most 'size' bytes
			n = fread (MEM_ADDRESS (address), 1, size, f);
			fclose (f);
		
			debug_printf ("loaded %d bytes from file %s at %.8x",
											n, filename, address);
		}
		else
			debug_printf ("error opening file %s: %s", filename, strerror (errno));
	}
	else
		debug_print ("* usage: dload filename [address]");
}


void debugger_cmd_map (char *args)
{
	MapItem *mi;
	int i, k = 0;


	if (!map)
		debug_print ("map doesn't contain any entries");
	else
	{
		debug_print ("Address  Size   \tName");
		for (i = 0; i < map->nitems; i++)
			if ((mi = map_get_item_by_num (map, i)))
			{
				debug_printf ("%.8x %.8x\t%.60s", mi->address, mi->size, mi->name);
				k++;
			}

		debug_printf ("map file contains %d entries", k);
	}
}


void debugger_cmd_mapadd (char *args)
{
	__u32 address, size = 0;
	char name[LINE_WIDTH + 1];
	int n, k;
	MapItem *mi;


	if ((n = args_parse_expression_uint (args, &address)) &&
			(k = args_extract_str (args + n, name)))
	{
		if (!map)
			gdebug_create_map ();

		address &= ~3;

		args_parse_expression_uint (args + k + n, &size);

		if ((mi = map_get_item_by_address (map, address)))
		{
			if (size)
				map_item_modify (mi, address, size, name);
			else
			{
				map_item_modify (mi, address, 0, name);
				map_item_generate_db (mi);
			}
		}
		else
			map_item_generate_db (map_add_item (map, address, size, name));

		if (size)
			debug_printf ("added: %.8x %.8x\t%.60s", address, size, name);
		else
			debug_printf ("added: %.8x\t%.60s", address, name);
	}
	else
		debug_print ("* usage: mapadd address name [size]");
}


void debugger_cmd_save (char *args)
{
	char filename[LINE_WIDTH + 1];
	int n, compress;


	if ((n = args_extract_str (args, filename)))
	{
		if (!args_parse_expression_uint (args + n, &compress))
			compress = debugger.compress_states;
	}
	else
	{
		strcpy (filename, gcbin_filename);
		kill_extension (filename);
		strcat (filename, ".gcs");
		
		compress = debugger.compress_states;
	}

	// detach all hle functions
	if (hle_enabled)
		hle (map, FALSE);

	save_state (filename, compress);

	if (hle_enabled)
		hle (map, TRUE);
}


void debugger_cmd_mdbinternal (char *args)
{
	output_immediate_append (&debugger.output, "preparing the database...");
	
	mdb = map_extract_db (mapdb);
	debug_print ("done. internal database will be used for map generation.");
}


// save data that can be used as a reference for map generation
void debugger_cmd_mdbsave (char *args)
{
	char filename[LINE_WIDTH + 1];
	int n;


	if (!map)
		return debug_print ("map file must be loaded");

	if (!(n = args_extract_str (args, filename)))
	{
		strcpy (filename, gcbin_filename);
		kill_extension (filename);
		strcat (filename, ".mdb");
	}

	output_immediate_append (&debugger.output, "creating database...");
	if (map_save_db (map, filename))
		debug_printf ("saved file %s", filename);
	else
		debug_printf ("error saving mdb file: %s", strerror (errno));
}


// load data that will be used as a reference for map generation
void debugger_cmd_mdbload (char *args)
{
	char filename[LINE_WIDTH + 1];
	int n;


	if (!(n = args_extract_str (args, filename)))
		return debug_print ("* usage: mdbload filename");

	if (mdb)
		map_destroy (mdb);

	mdb = map_load_db (filename);
	if (mdb)
		debug_printf ("loaded file %s with %d items", filename, mdb->nitems);
	else
		debug_printf ("can't load file %s: %s", filename, strerror (errno));
}


void debugger_cmd_mapgen (char *args)
{
	if (!mdb)
	{
		output_immediate_append (&debugger.output, "using internal database...");
	
		mdb = map_extract_db (mapdb);
	}

	if (map)
		map_destroy (map);

	output_immediate_append (&debugger.output, "searching for functions...");
	map = map_generate_nameless ();
	output_immediate_append (&debugger.output, "guessing function names...");
	map_generate_names (map, mdb);
	debug_print ("finished");
}


void debugger_cmd_mapsave (char *args)
{
	char filename[LINE_WIDTH + 1];
	int n;


	if (!map)
		return debug_print ("map must be first loaded or generated");

	if (!(n = args_extract_str (args, filename)))
	{
		strcpy (filename, gcbin_filename);
		kill_extension (filename);
		strcat (filename, ".map");
	}

	map->modified = TRUE;
	if (map_save (map, filename))
		debug_printf ("saved file %s", filename);
	else
		debug_printf ("error saving map file: %s", strerror (errno));
}


void debugger_cmd_mapload (char *args)
{
	char filename[LINE_WIDTH + 1];
	int n;
	Map *newmap;


	if (!(n = args_extract_str (args, filename)))
	{
		strcpy (filename, gcbin_filename);
		kill_extension (filename);
		strcat (filename, ".map");
	}

	if ((newmap = map_load (filename)))
	{
		debug_printf ("loaded file %s", filename);

		if (map)
			map_destroy (map);

		map = newmap;
	}
	else
		debug_printf ("error loading map file %s: %s", filename, strerror (errno));
}


void debugger_cmd_dumpstream (char *args)
{
	char filename[LINE_WIDTH + 1];
	__u32 address, length;
	int n, k;


	if ((n = args_extract_str (args, filename)))
	{
		if ((k = args_parse_expression_uint (args + n, &address)))
		{
			n += k;
			if ((k = args_parse_expression_uint (args + n, &length)))
			{
				if (audio_dump_stream (filename, MEM_ADDRESS (address), length))
					debug_printf ("stream dumped to %s", filename);
				else
					debug_printf ("error dumping stream: %s", strerror (errno));
				return;
			}
		}
	}

	debug_printf ("* usage: dumpstream filename address length", filename);
}


void debugger_cmd_dumptexture (char *args)
{
	char filename[LINE_WIDTH + 1];
	__u32 address, format, width, height;
	int n, k;
	char *buff;


	if ((n = args_extract_str (args, filename)))
	{
		if ((k = args_parse_expression_uint (args + n, &address)))
		{
			n += k;
			if ((k = args_parse_expression_uint (args + n, &width)))
			{
				n += k;
				if ((k = args_parse_expression_uint (args + n, &height)))
				{
					n += k;
					if ((k = args_parse_expression_uint (args + n, &format)))
					{
						buff = gx_convert_texture (MEM_ADDRESS (address), width, height, format, NULL, 0);
						if (buff)
						{
							if (video_dump_texture (filename, buff, width, height))
								debug_printf ("texture dumped to %s", filename);
							else
								debug_printf ("error dumping texture: %s", strerror (errno));
						}
						else
							debug_printf ("unknown texture format");
						return;
					}
				}
			}
		}
	}

	debug_printf ("* usage: dumptexture filename address width height format", filename);
}


void debugger_cmd_dvdeject (char *args)
{
	mdvd_close ();
}


void debugger_cmd_logenable (char *args)
{
	__u32 l;


	if (args_parse_expression_uint (args, &l))
	{
		debugger.log_enable |= l;
		debug_printf ("log_enable = 0x%.8x", debugger.log_enable);
	}
	else
		debug_print ("* usage: logenable log_gx | log_ai | log_dsp ...");
}


void debugger_cmd_logdisable (char *args)
{
	__u32 l;


	if (args_parse_expression_uint (args, &l))
	{
		debugger.log_enable &= ~l;
		debug_printf ("log_enable = 0x%.8x", debugger.log_enable);
	}
	else
		debug_print ("* usage: logdisable log_gx | log_ai | log_dsp ...");
}


void debugger_cmd_hlelist (char *args)
{
	int n = 0;


	debug_printf ("High Level Emulated functions:");
	while (hle_functions[n].name)
		debug_printf ("  %s", hle_functions[n++].name);
}


void debugger_cmd_hle (char *args)
{
	unsigned int n;
	char buff[LINE_WIDTH + 1];
	const char *estat[] =
	{
		"LLE   ", "HLE   ", "IGNORE",
	};
	MapItem *mi;


	if (!map)
		generate_map ();

	n = args_extract_str (args, buff);
	if (!n)
	{
		for (n = 0; n < map->nitems; n++)
			if ((mi = map->items_by_num[n]))
				debug_printf ("  %s %s",
												estat[mi->emulation_status], mi->name);
	}
	else
	{
		if ((mi = map_get_item_by_name (map, buff)))
		{
			if (args_extract_str (args + n, buff))
			{
				if (0 == strcmp (buff, "lle"))
					map_item_emulation (mi, EMULATION_LLE, 0);
				else if (0 == strcmp (buff, "hle"))
				{
					if (!map_item_emulation_hle (mi))
						debug_printf ("hle_function %s not implemented", mi->name);
				}
				else if (0 == strcmp (buff, "ignore"))
					map_item_emulation (mi, EMULATION_IGNORE, 0);
				else
					debug_print ("* usage: hle [function] [state]");

				debug_printf ("  %s %s",
												estat[mi->emulation_status], mi->name);
			}
			else
				debug_printf ("  %s %s",
												estat[mi->emulation_status], mi->name);
		}
		else
			debug_printf ("function %s not found", buff);
	}
}


void debugger_cmd_hleattach (char *args)
{
	if (!hle_enabled)
	{
		hle (map, TRUE);
		debug_print ("hle system enabled");
		hle_enabled = TRUE;
	}
	else
		debug_print ("hle system is already enabled");
}


void debugger_cmd_hledetach (char *args)
{
	if (hle_enabled)
	{
		hle (map, FALSE);
		debug_print ("hle system disabled");
		hle_enabled = FALSE;
	}
	else
		debug_print ("hle system is already disabled");
}


void debugger_cmd_hlewith (char *args)
{
	unsigned int n, k;
	char name1[LINE_WIDTH + 1], name2[LINE_WIDTH + 1];
	MapItem *mi;


	if (!map)
	{
		debug_print ("no map file. hle cannot be used unless map file will be generated.");
		return;
	}

	if (((n = args_extract_str (args, name1))) &&
			((k = args_extract_str (args + n, name2))))
	{
		if ((mi = map_get_item_by_name (map, name1)))
		{
			if (map_item_hle_with (mi, name2))
				debug_printf ("function %s will be now emulated with %s",
												name1, name2);
			else
				debug_printf ("function %s not found", name2);
		}
		else
			debug_printf ("function %s not mapped", name2);
	}
	else
		debug_printf ("* usage: hlewith mapped_function hled_function", name2);
}


void debugger_cmd_movieignore (char *args)
{
	unsigned int sw;


	if (args_parse_expression_uint (args, &sw))
		enable_ignore_movies (sw);
	else
		debug_print ("* usage: movieignore 0|1|2");
}


void debugger_cmd_texdump (char *args)
{
	int sw;


	sw = gx_switch (GX_TOGGLE_TEX_DUMP);
	debug_printf ("texture dumping %s", (sw) ? "enabled" : "disabled");
}


void debugger_cmd_gxtoggle (char *args)
{
	unsigned int sw, k = 0;


	if (args_parse_expression_uint (args, &sw))
	{
		k = gx_switch (sw);
		debug_printf ("%s = %d", args, k);
	}
	else
		debug_print ("* usage: gxtoggle GXT_WIREFRAME | GXT_FORCE_LINEAR | ...");
}


void debugger_cmd_showregs (char *args)
{
	int i;
	int indices[] =
	{
		I_PC, I_LR, I_CR, I_FPSCR, I_MSR, I_IC, I_XER, I_CTR, I_DEC, I_SDA1, I_SDA2, 
		I_SRR0, I_SRR1, I_HID0, I_HID1, I_HID2, I_WPAR, I_TBL, I_TBU, I_DMAL, I_DMAU,
	};
	const char *names[] =
	{
		"PC", "LR", "CR", "FPSCR", "MSR", "IC", "XER", "CTR", "DEC", "SDA1", "SDA2",
		"SRR0", "SRR1", "HID0", "HID1", "HID2", "WPAR", "TBL", "TBU", "DMAL", "DMAU",
	};


	for (i = 0; i < sizeof (indices) / sizeof (*indices); i++)
		debug_printf ("%7s = %.8x", names[i], regs[indices[i]]);
	
	for (i = 0; i < 32; i++)
		debug_printf ("GPR[%2d] = %.8x", i, GPR[i]);

	for (i = 0; i < 16; i++)
		debug_printf (" SR[%2d] = %.8x", i, SR[i]);

	for (i = 0; i < 8; i++)
		debug_printf ("GQR[%2d] = %.8x", i, GQR[i]);

	for (i = 0; i < 32; i++)
		debug_printf ("PS0[%2d] = %.2f", i, FPR[i]);

	for (i = 0; i < 32; i++)
		debug_printf ("PS1[%2d] = %.2f", i, PS1[i]);
}


void debugger_cmd_fifo (char *args)
{
	debug_printf ("Current FIFO stats:");
	if (CPCR & CP_CR_GPLINK)
	{
		debug_printf ("  FIFO attached to: cpu and gpu");
		debug_printf ("      Base address: %.8x", CP_FIFO_BASE | 0x80000000);
		debug_printf ("              Size: %.8x", CP_FIFO_END - CP_FIFO_BASE);
		debug_printf ("     Write pointer: %.8x", CP_FIFO_WPOINTER | 0x80000000);
	}
	else
	{
		debug_printf ("  FIFO attached to: cpu");
		debug_printf ("      Base address: %.8x", PI_FIFO_BASE | 0x80000000);
		debug_printf ("              Size: %.8x", PI_FIFO_END - PI_FIFO_BASE);
		debug_printf ("     Write pointer: %.8x", PI_FIFO_WPOINTER | 0x80000000);
	}
}


Command commands[] =
{
	{ "help", "list of commands", debugger_cmd_help },
	{ "x", "detach from debugger", debugger_cmd_detach },
	{ "quit", "quit", debugger_cmd_quit },
	{ "n", "step into", debugger_cmd_step_into },
	{ "s", "step over", debugger_cmd_step_over },
	{ "mem", "show memory at given address and write data", debugger_cmd_mem },
	{ "mfill", "fill memory with a constant byte", debugger_cmd_mfill },
	{ "code", "disassemble code at given address", debugger_cmd_code },
	{ "r", "show / change register", debugger_cmd_reg },
	{ "run", "run program", debugger_cmd_run },
	{ "load", "load executable", debugger_cmd_load },
	{ "save", "save state", debugger_cmd_save },
	{ "reload", "reload executable", debugger_cmd_reload },
	{ "print", "print the value of an expression", debugger_cmd_print },
	{ "pf", "print the value of an expression as floating point", debugger_cmd_print_double },
	{ "set", "set variable", debugger_cmd_set },
	{ "exception", "initiate cpu exception", debugger_cmd_exception },
	{ "interrupt", "initiate cpu interrupt", debugger_cmd_interrupt },
	{ "clear", "clear output window", debugger_cmd_clear },
	{ "bp", "set code breakpoint", debugger_cmd_bp },
	{ "bpm", "set breakpoint on memory read/write", debugger_cmd_bpm },
	{ "bpmr", "set breakpoint on memory read", debugger_cmd_bpmr },
	{ "bpmw", "set breakpoint on memory write", debugger_cmd_bpmw },
	{ "bpmc", "set breakpoint on memory change", debugger_cmd_bpmc },
	{ "bpuhw", "break on unhandled hardware access", debugger_cmd_bpuhw },
	{ "bc", "clear breakpoint", debugger_cmd_bc },
	{ "bd", "disable breakpoint", debugger_cmd_bd },
	{ "be", "enable breakpoint", debugger_cmd_be },
	{ "bl", "list breakpoints", debugger_cmd_bl },
	{ "codedump", "dump code to file", debugger_cmd_codedump },
	{ "outdump", "dump output to file", debugger_cmd_outdump },
	{ "dload", "load data file at memory address", debugger_cmd_dload },
	{ "dumpstream", "dump audio stream to file", debugger_cmd_dumpstream },
	{ "dumptexture", "dump texture to file", debugger_cmd_dumptexture },
	{ "xfb", "set framebuffer address", debugger_cmd_xfb },
	{ "bpex", "break on exception", debugger_cmd_bpex },
	{ "dvdeject", "eject dvd", debugger_cmd_dvdeject },
	{ "logenable", "enable logs", debugger_cmd_logenable },
	{ "logdisable", "disable logs", debugger_cmd_logdisable },
	{ "hww", "hardware read / write word", debugger_cmd_hww },
	{ "hwh", "hardware read / write half", debugger_cmd_hwh },
	{ "mdbsave", "generate and save map database", debugger_cmd_mdbsave },
	{ "mdbload", "load map database", debugger_cmd_mdbload },
	{ "mdbinternal", "use internal map database", debugger_cmd_mdbinternal },
	{ "map", "show memory map entries", debugger_cmd_map },
	{ "mapadd", "add memory map entry", debugger_cmd_mapadd },
	{ "mapgen", "generate map file using previously loaded mdb file", debugger_cmd_mapgen },
	{ "mapsave", "save map file", debugger_cmd_mapsave },
	{ "mapload", "load map file", debugger_cmd_mapload },
	{ "hle", "display and change hle status of functions", debugger_cmd_hle },
	{ "hlelist", "show implemented hle functions", debugger_cmd_hlelist },
	{ "hleattach", "enable hle", debugger_cmd_hleattach },
	{ "hledetach", "disable hle", debugger_cmd_hledetach },
	{ "hlewith", "emulate one function using another", debugger_cmd_hlewith },
	{ "movieignore", "disable loading movies (THP, H4M, STR)", debugger_cmd_movieignore },
	{ "texdump", "enable / disable dumping textures", debugger_cmd_texdump },
	{ "gxtoggle", "toggle gx switch", debugger_cmd_gxtoggle },
	{ "showregs", "display the contents of registers", debugger_cmd_showregs },
	{ "fifo", "show current fifo informations", debugger_cmd_fifo },
};


void gdebug_event (int event, const char *format, ...)
{
	va_list ap;
	
	va_start (ap, format);
	switch (event)
	{
		case EVENT_QUIT:
			gcube_running = FALSE;
			gdebug_running = FALSE;
			emu_state = EMU_EXIT | GDEBUG_EXIT;
			output_vappend (&debugger.output, ap, format);
			break;
		
		case EVENT_EFATAL:
		case EVENT_STOP:
			if (gcube_running)
			{
				gcube_running = FALSE;
				emu_state = GDEBUG_RUN;
			}
			else if (gdebug_running)
			{
				gdebug_running = FALSE;
			}
			output_vappend (&debugger.output, ap, format);
			break;
		
		case EVENT_EMAJOR:
			gdebug_running = FALSE;
			output_vappend (&debugger.output, ap, format);
			break;
		
		case EVENT_INFO:
		case EVENT_LOG:
		case EVENT_EMINOR:
			output_vappend (&debugger.output, ap, format);
			break;

		case EVENT_OSREPORT:
			debug_printf ("OSReport: %s", format);
			break;

		case EVENT_EXCEPTION:
			if (debugger.break_on_exception)
		case EVENT_BREAKPOINT:
			{
				output_vappend (&debugger.output, ap, format);
				gdebug_running = FALSE;
			}
			break;

		case EVENT_UHW_READ:
			output_vappend (&debugger.output, ap, format);
			if (debugger.break_on_uhw_read)
				gdebug_running = FALSE;
			break;

		case EVENT_UHW_WRITE:
			output_vappend (&debugger.output, ap, format);
			if (debugger.break_on_uhw_write)
				gdebug_running = FALSE;
			break;

		case EVENT_TRAP:
			output_vappend (&debugger.output, ap, format);
			gdebug_running = FALSE;
			break;

		case EVENT_LOG_INT:
			if (debugger.log_enable & LOG_ENABLE_INT)
			{
				char buff[256] = {0};

				sprint_intmask (buff, INTSR);
				debug_printf ("%s%s", format, buff);
			}
			break;

		case EVENT_LOG_CALLS:
			if (debugger.log_enable & LOG_ENABLE_CALLS)
				output_vappend (&debugger.output, ap, format);
			break;

		case EVENT_LOG_AI:
			if (debugger.log_enable & LOG_ENABLE_AI)
				output_vappend (&debugger.output, ap, format);
			break;

		case EVENT_LOG_DSP:
			if (debugger.log_enable & LOG_ENABLE_DSP)
				output_vappend (&debugger.output, ap, format);
			break;

		case EVENT_LOG_CP:
			if (debugger.log_enable & LOG_ENABLE_CP)
				output_vappend (&debugger.output, ap, format);
			break;

		case EVENT_LOG_DI:
			if (debugger.log_enable & LOG_ENABLE_DI)
				output_vappend (&debugger.output, ap, format);
			break;

		case EVENT_LOG_EXI:
			if (debugger.log_enable & LOG_ENABLE_EXI)
				output_vappend (&debugger.output, ap, format);
			break;

		case EVENT_LOG_MI:
			if (debugger.log_enable & LOG_ENABLE_MI)
				output_vappend (&debugger.output, ap, format);
			break;

		case EVENT_LOG_PE:
			if (debugger.log_enable & LOG_ENABLE_PE)
				output_vappend (&debugger.output, ap, format);
			break;

		case EVENT_LOG_PI:
			if (debugger.log_enable & LOG_ENABLE_PI)
				output_vappend (&debugger.output, ap, format);
			break;

		case EVENT_LOG_SI:
			if (debugger.log_enable & LOG_ENABLE_SI)
				output_vappend (&debugger.output, ap, format);
			break;

		case EVENT_LOG_VI:
			if (debugger.log_enable & LOG_ENABLE_VI)
				output_vappend (&debugger.output, ap, format);
			break;

		case EVENT_LOG_GX:
		case EVENT_LOG_GX_IMM:
			if (debugger.log_enable & LOG_ENABLE_GX)
			{
				if (event == EVENT_LOG_GX_IMM)
					output_immediate_vappend (&debugger.output, ap, format);
				else
					output_vappend (&debugger.output, ap, format);
			}
			break;

		case EVENT_LOG_HLE:
			if (debugger.log_enable & LOG_ENABLE_HLE)
				output_vappend (&debugger.output, ap, format);
			break;

		case EVENT_DEBUG_MSG:
			if (!gcube_running)
				output_immediate_vappend (&debugger.output, ap, format);
			break;
	}

	va_end (ap);
}


// 'control + c' handler
void sig_ctrl_c (int data)
{
	if (gcube_running)
	{
		gcube_running = FALSE;
		emu_state = GDEBUG_RUN;
	}
	else if (gdebug_running)
	{
		gdebug_running = FALSE;
	}
}


// debugger functions

void debugger_cmd_step_into (char *args)
{
	debugger.last_pc = PC;
	cpu_execute ();
	debugger.code_address = PC;
}


void debugger_cmd_detach (char *args)
{
	emu_state = GDEBUG_EXIT | GCUBE_RUN;
}


void debugger_cmd_quit (char *args)
{
	emu_state = EMU_EXIT | GDEBUG_EXIT;
}


int command_find (char *cmd)
{
	int i;


	for (i = 0; i < (sizeof (commands) / sizeof (Command)); i++)
		if (0 == strncmp (cmd, commands[i].name, strlen (cmd)))
			break;
	
	if (i == (sizeof (commands) / sizeof (Command)))
		return -1;
	else
		return i;
}


void debugger_cmd_help (char *args)
{
	int i;
	char buff[LINE_WIDTH + 1];
	

	debug_printf ("");
	debug_printf ("available commands:");
	for (i = 0; i < (sizeof (commands) / sizeof (Command)); i++)
	{
		memset (buff, 0, sizeof (buff));
		memset (buff, ' ', 6);
		strcat (buff, commands[i].name);
		memset (buff + strlen (buff), ' ', 30 - 6 - strlen (buff));
		strcat (buff, commands[i].description);
		debug_print (buff);
	}
}


void debugger_parse_command (char *command)
{
	char cmd[LINE_WIDTH + 1] = {0}, args[LINE_WIDTH + 1] = {0};
	int i = 0;


	sscanf (command, "%s %[^\n]", cmd, args);

	i = command_find (cmd);
	if (i < 0)
		debug_printf ("unknown command: %s", cmd);
	else
		commands[i].action (args);

	screen_redraw (&debugger.screen);
}


void mem_show (Window *win, int lines, unsigned int addr)
{
	int i, j, c;
	float f;


	for (i = win->nlines - lines; i < win->nlines; i++)
	{
		switch (debugger.bmode)
		{
			case BWIN_MODE_FLOAT:
				f = MEMRF_SAFE (addr);
				window_clear_line (win, i);
				window_cprintf (win, 0, i, FG_RED, "%.8x", addr);
				window_cprintf (win, 10, i, FG_GREEN, "%40f", f);
				addr += 4;
				break;

			case BWIN_MODE_HEX:
			default:
				window_cprintf (win, 0, i, FG_RED, "%.8x", addr);
				window_line_set_attribs (win, i, 10, 60, FG_GREEN, 0);
				window_line_set_attribs (win, i, 60, 80, FG_BLACK, 0);

				for (j = 0; j < 0x10; j++)
				{
					c = MEM_SAFE (addr + j);
					window_printf (win, 10 + j*3 + j/4, i, "%.2x", c);
					window_printf (win, 63 + j, i, "%c", (c >= 0x20 && c < 0x7f) ? c : '.');
				}
				addr += 0x10;
				break;
		}
	}
}


void code_show (Window *win, int lines, unsigned int addr)
{
	__u32 target;
	char operand[64], opcode[64];
	unsigned int op;
	int i;
	Breakpoint *bp;
	MapItem *mi;


	addr -= (win->nlines/2) * 4;
	for (i = win->nlines - lines; i < win->nlines; i++)
	{
		op = MEM32_SAFE (addr);
		disassemble (opcode, operand, op, addr, &target);
		window_clear_line (win, i);
		
		if ((bp = debugger.breakpoints[(addr & MEM_MASK) >> 2]) && bp->enabled)
			window_line_set_attribs (win, i, 0, 80, FG_YELLOW, ATTRIB_STANDOUT);
		else if (addr == PC)
			window_line_set_attribs (win, i, 0, 80, FG_MAGENTA, ATTRIB_STANDOUT);
		else if (addr == debugger.code_address)
			window_line_set_attribs (win, i, 0, 80, FG_WHITE, ATTRIB_INVERT);
		else
		{
			window_line_set_attribs (win, i, 0, 8, FG_RED, 0);
			window_line_set_attribs (win, i, 10, 18, FG_GREEN, 0);
			window_line_set_attribs (win, i, 20, 60, FG_BLUE, 0);
			window_line_set_attribs (win, i, 60, 80, FG_BLACK, ATTRIB_BOLD);
		}
		window_printf (win, 0, i, "%.8x  %.8x   %s", addr, op, opcode);

		if ((target != addr + 4) && (mi = map_get_item_by_address (map, target)))
			window_printf (win, 32, i, "%s", mi->name);
		else
		{
			window_printf (win, 32, i, "%s", operand);
			if (target != addr + 4)
				window_printf (win, 19, i, "%c", (target > addr) ? 'v' : ((target == addr) ? '<' : '^'));
		}

		if ((mi = map_get_item_by_address (map, addr)))
			window_printf (win, 60, i, "%s", mi->name);

		addr += 4;
	}

	if (debugger.code_cmd_input)
	{
		window_title_clear (win, 7, LINE_WIDTH);
		window_title_printf (win, CODE_CMD_CURSOR_OFFSET, debugger.code_cmd);
	}
	else
	{
		window_title_clear (win, 7, LINE_WIDTH);

		if ((mi = map_get_item_containing_address (map, debugger.code_address)))
			window_title_printf (win, 7, mi->name);
	}
}


// redraw line at current address
void code_refresh_current_line (Window *win)
{
	__u32 target, addr;
	char operand[64], opcode[64];
	unsigned int op;
	int i;
	MapItem *mi;


	i = win->nlines / 2;
	addr = debugger.code_address;

	op = MEM32_SAFE (addr);
	disassemble (opcode, operand, op, addr, &target);
	window_clear_line (win, i);
		
	window_line_set_attribs (win, i, 0, 80, FG_MAGENTA, ATTRIB_STANDOUT);
	window_printf (win, 0, i, "%.8x  %.8x  %s", addr, op, opcode);

	if ((target != addr + 4) && (mi = map_get_item_by_address (map, target)))
		window_printf (win, 32, i, "%s", mi->name);
	else
		window_printf (win, 32, i, "%s", operand);

	if ((mi = map_get_item_by_address (map, addr)))
		window_printf (win, 60, i, "%s", mi->name);
}


char *format_double (char *buff, double d)
{
	char temp1[1024], temp2[1024], format[64];
	double a, b;
	int len, k, negative = FALSE;


	a = trunc (d);
	b = d - a;
	negative = (d < 0) ? 1 : 0;

	sprintf (temp1, "%.0lf", a);
	if (negative)
		sprintf (temp2, "%.6lf", b);
	else
		sprintf (temp2, "%.6lf", b);

	len = strlen (temp1);
	if ((a > 999999) || (a < -99999))
	{
		if (len <= 8)
			sprintf (buff, "%8.0lf", a);
		else
		{
			k = len - 8 + 3 + negative;
			if (k >= 100)
				sprintf (buff, "%.3se+%d", temp1, k);
			else if (k >= 10)
				sprintf (buff, "%.4se+%d", temp1, k);
			else
				sprintf (buff, "%.5se+%d", temp1, k);
		}
	}
	else if (a > -0.1 && a < 0.1)
	{
		strcpy (buff, temp2);
	}
	else
	{
		sprintf (format, "%s.%%.%ds", temp1, 7 - len);
		sprintf (buff, format, &temp2[2 + negative]);
	}

	return buff;
}


void regs_show (Window *win, int sw)
{
	static __u32 oldregs[4096] = {0};
	static double oldfpregs[32] = {0};
	static double oldps0regs[32] = {0};
	static double oldps1regs[32] = {0};
	static __u32 row1[] = { I_CR, I_FPSCR, I_XER, I_MSR, I_CTR, I_HID0, I_HID1, I_HID2 };
	static int fgc[] = { FG_BLACK, FG_MAGENTA };
	int i, j;
	char buff[256];


	switch (debugger.regs_mode)
	{
		case REGS_MODE_GPR:
			window_cprintf (win, 0, 0, FG_BLUE, "gpr");
			for (j = 0; j < 4; j++)
			{
				for (i = 0; i < 8; i++)
				{
					window_cprintf (win, 8 + i*9, j, fgc[oldregs[j*8 + i] != GPR[j*8 + i]],
													"%.8x", GPR[j*8 + i]);
					oldregs[j*8 + i] = GPR[j*8 + i];
				}
			}
			break;
		
		case REGS_MODE_FPR:
			window_cprintf (win, 0, 0, FG_BLUE, "fpr");
			for (j = 0; j < 4; j++)
			{
				for (i = 0; i < 8; i++)
				{
					format_double (buff, FPR[j*8 + i]);
					window_cprintf (win, 8 + i*9, j, fgc[oldfpregs[j*8 + i] != FPR[j*8 + i]],
													"%s", buff);
					oldfpregs[j*8 + i] = FPR[j*8 + i];
				}
			}
			break;

		case REGS_MODE_PS0:
			window_cprintf (win, 0, 0, FG_BLUE, "ps0");
			for (j = 0; j < 4; j++)
			{
				for (i = 0; i < 8; i++)
				{
					format_double (buff, PS0[j*8 + i]);
					window_cprintf (win, 8 + i*9, j, fgc[oldps0regs[j*8 + i] != PS0[j*8 + i]],
													"%s", buff);
					oldps0regs[j*8 + i] = PS0[j*8 + i];
				}
			}
			break;

		case REGS_MODE_PS1:
			window_cprintf (win, 0, 0, FG_BLUE, "ps1");
			for (j = 0; j < 4; j++)
			{
				for (i = 0; i < 8; i++)
				{
					format_double (buff, PS1[j*8 + i]);
					window_cprintf (win, 8 + i*9, j, fgc[oldps1regs[j*8 + i] != PS1[j*8 + i]],
													"%s", buff);
					oldps1regs[j*8 + i] = PS1[j*8 + i];
				}
			}
			break;
	}

	window_cprintf (win, 8, 4, FG_BLUE, "cr       fpscr    xer      msr      ctr      hid0     hid1     hid2");
	window_printf (win, 15, 4, (GET_CR0 & CR_EQ) ? "=" : (GET_CR0 & CR_LT) ? "<" : (GET_CR0 & CR_GT) ? ">" : "");
	for (i = 0; i < 8; i++)
	{
		window_cprintf (win, 8 + i*9, 5, fgc[oldregs[row1[i]] != REGS (row1[i])],
										"%.8x", REGS (row1[i]));
		oldregs[row1[i]] = REGS (row1[i]);
	}

	window_cprintf (win, 8, 6, FG_BLUE, "dec      ic                                           lr       pc");
	window_printf (win, 8, 7, "%.8x %.8x                                              %.8x", DEC, IC, PC);
	window_cprintf (win, 62, 7, fgc[oldregs[I_LR] != REGS (I_LR)],
									"%.8x", REGS (I_LR));
	oldregs[I_LR] = REGS (I_LR);
}


void output_show (Window *win, OutputWin *out, int nlines, int pos)
{
	int i;
	float t;


	for (i = win->nlines - nlines; i < win->nlines; i++)
	{
		if (pos >= out->nitems)
			break;

		window_clear_line (win, i);
		window_printf (win, 3, i, out->items[output_item_index (out, pos++)]);
	}
	
	t = (float) 100.0 * (out->pos + win->nlines) / out->nitems;
	if (t > 100.0)
		t = 100.0;

	window_title_printf (win, LINE_WIDTH - 7, " %3.0f %% ", t);
}


void output_append (OutputWin *out, char *str)
{
	if (out->nitems >= MAX_OUTPUT_LINES)
	{
		free (out->items[output_item_index (out, --out->nitems)]);
		out->nfirst_item++;
		if (out->nfirst_item >= MAX_OUTPUT_LINES)
			out->nfirst_item = 0;
	}

	out->items[output_item_index (out, out->nitems++)] = strdup (str);

	if (out->visitems)
		out->pos = out->nitems - out->visitems;
	if (out->pos < 0)
		out->pos = 0;
}


// refresh output after appending
void output_immediate_append (OutputWin *out, char *str)
{
	if (out->nitems >= MAX_OUTPUT_LINES)
	{
		free (out->items[output_item_index (out, --out->nitems)]);
		out->nfirst_item++;
		if (out->nfirst_item >= MAX_OUTPUT_LINES)
			out->nfirst_item = 0;
	}

	out->items[output_item_index (out, out->nitems++)] = strdup (str);

	if (out->visitems)
		out->pos = out->nitems - out->visitems;
	if (out->pos < 0)
		out->pos = 0;

	if (gdebug_running)
	{
		output_show (debugger.output_win, out, debugger.output_win->nlines, out->pos);
		screen_refresh (&debugger.screen);
	}
}


void output_vappend (OutputWin *out, va_list ap, const char *format)
{
	char buff[LINE_WIDTH + 1];
	
	
	vsnprintf (buff, sizeof (buff), format, ap);
	output_appendf (out, buff);
}


void output_immediate_vappend (OutputWin *out, va_list ap, const char *format)
{
	char buff[LINE_WIDTH + 1];
	
	
	vsnprintf (buff, sizeof (buff), format, ap);
	output_immediate_appendf (out, buff);
}


void output_appendf (OutputWin *out, char *format, ...)
{
	char str[LINE_WIDTH + 1];
	va_list ap;


	va_start (ap, format);
	vsnprintf (str, sizeof (str), format, ap);
	va_end (ap);

	output_append (out, str);
}


void output_immediate_appendf (OutputWin *out, char *format, ...)
{
	char str[LINE_WIDTH + 1];
	va_list ap;


	va_start (ap, format);
	vsnprintf (str, sizeof (str), format, ap);
	va_end (ap);

	output_immediate_append (out, str);
}


void output_destroy (OutputWin *out)
{
	while (out->nitems)
		free (out->items[--out->nitems]);
}


void command_history_append (CommandWin *cmd, char *str)
{
	if (cmd->hlines < MAX_HISTORY_LINES)
		if (!cmd->hlines || (cmd->hlines && strcmp (cmd->history[cmd->hlines-1], str)))
			cmd->history[cmd->hlines++] = strdup (str);
}


void command_history_destroy (CommandWin *cmd)
{
	while (cmd->hlines)
		free (cmd->history[--cmd->hlines]);
}


void debugger_execute_command (void)
{
	if (debugger.console.hpos)
	{
		// execute history command
		debugger_parse_command (debugger.console.history[debugger.console.hlines - debugger.console.hpos]);
		debugger.console.hpos = 0;
	}
	else if (strlen (debugger.console.buff) > 0)
	{
		// execute buffer command
		command_history_append (&debugger.console, debugger.console.buff);
		debugger_parse_command (debugger.console.buff);
	}
	else if (debugger.console.hlines)
		// execute last command
		debugger_parse_command (debugger.console.history[debugger.console.hlines - 1]);

	memset (debugger.console.buff, 0, CONSOLE_MAX_CHARS);
	debugger.console.pos = 0;
	window_refresh (debugger.console_win);
}



// window handling functions

void on_mem_resize (Window *win, int nlines, void *data)
{
	mem_show (win, win->nlines, debugger.mem_address);
	
	debugger.mem_lines = nlines;
}


void on_mem_refresh (Window *win, void *data)
{
	mem_show (win, win->nlines, debugger.mem_address);
}


void on_mem_scroll (Window *win, int nlines, int direction, void *data)
{
	unsigned int addr;
	int i;
	int size[] =
	{
		0x10, 4,
	};


	if (direction == DIR_DOWN)
	{
		addr = debugger.mem_address + win->nlines * size[debugger.bmode];
		debugger.mem_address += size[debugger.bmode] * nlines;
	}
	else
	{
		addr = debugger.mem_address - size[debugger.bmode];
		debugger.mem_address -= size[debugger.bmode] * nlines;
	}

	for (i = 0; i < nlines; i++)
	{
		if (direction == DIR_DOWN)
		{
			window_roll (win, 1, direction);
			mem_show (win, 1, addr);
			addr += size[debugger.bmode];
		}
		else
		{
			mem_show (win, 1, addr);
			window_roll (win, 1, direction);
			addr -= size[debugger.bmode];
		}
	}
}


int on_mem_keypress (Window *win, int key, int modifiers, void *data)
{
	switch (key)
	{
		case KEY_RIGHT:
			if (debugger.bmode < BWIN_NMODES - 1)
			{
				debugger.bmode++;
				window_clear (debugger.mem_win);
				screen_redraw_window (&debugger.screen, debugger.mem_win);
			}
			return TRUE;

		case KEY_LEFT:
			if (debugger.bmode > 0)
			{
				debugger.bmode--;
				window_clear (debugger.mem_win);
				screen_redraw_window (&debugger.screen, debugger.mem_win);
			}
			return TRUE;

		default:
			return FALSE;
	}
}


int buff_keystroke (char *buff, int *pos, int maxh, int key)
{
	int i;


	if (key >= 0x20 && key < 0x7f)
	{
		if (*pos < maxh)
			buff[(*pos)++] = key;

		return TRUE;
	}
	else switch (key)
	{
		case KEY_BACKSPACE:
			if (*pos > 0)
			{
				for (i = --(*pos); i < strlen (buff); i++)
					buff[i] = buff[i + 1];
				buff[i] = '\0';
			}
			return TRUE;
			
		case KEY_DC:
				for (i = *pos; i < strlen (buff); i++)
					buff[i] = buff[i + 1];
				buff[i] = '\0';
			return TRUE;

		case KEY_LEFT:
			if (*pos > 0)
				(*pos)--;
			return TRUE;

		case KEY_RIGHT:
			if (*pos < strlen (buff))
				(*pos)++;
			return TRUE;

		case KEY_HOME:
			*pos = 0;
			return TRUE;

		case KEY_END:
			*pos = strlen (buff);
			return TRUE;

		default:
			return FALSE;
	}
}


int on_code_keypress (Window *win, int key, int modifiers, void *data)
{
	if (!debugger.code_cmd_input)
	{
		switch (key)
		{
			case ':':
				// label
				debugger.code_cmd_input = CODE_CMD_SET_LABEL;
				screen_set_cursor_relative (&debugger.screen, win, debugger.hcodepos + CODE_CMD_CURSOR_OFFSET, 0);
				return TRUE;

			case ';':
				// label
				debugger.code_cmd_input = CODE_CMD_SET_COMMENT;
				screen_set_cursor_relative (&debugger.screen, win, debugger.hcodepos + CODE_CMD_CURSOR_OFFSET, 0);
				return TRUE;
			
			case 'b':
				{
					Breakpoint *bp;

					if ((bp = debugger.breakpoints[(debugger.code_address & MEM_MASK) >> 2]) &&
							bp->enabled)
					{
						debug_printf ("code breakpoint %d cleared", bp->num);

						debugger_remove_breakpoint (bp);
					}
					else
						debugger_add_breakpoint (debugger.code_address, BP_CODE);

					screen_redraw (&debugger.screen);
				}
				return TRUE;

			case 'j':
				PC = debugger.code_address;
				screen_redraw (&debugger.screen);
				return TRUE;

			case KEY_RIGHT:
				cwin_change_mode (debugger.cmode + 1);
				return TRUE;

			case KEY_LEFT:
				cwin_change_mode (debugger.cmode - 1);
				return TRUE;

			default:
				return FALSE;
		}
	}
	else switch (key)
	{
		// enter
		case 0x0d:
			switch (debugger.code_cmd_input)
			{
				case CODE_CMD_SET_LABEL:
					{
						MapItem *mi = map_get_item_by_address (map, debugger.code_address);

						if ((strlen (debugger.code_cmd) == 0) && mi)
							map_remove_item (map, mi);
						else if (mi)
						{
							map_item_modify (mi, debugger.code_address, mi->size, debugger.code_cmd);
							map->modified = TRUE;
						}
						else
						{
							if (!map)
								gdebug_create_map ();

							mi = map_add_item (map, debugger.code_address, 0, debugger.code_cmd);
							map_item_generate_db (mi);
						}
					}
					break;

				case CODE_CMD_SET_COMMENT:
					{
						MapItem *mi = map_get_item_by_address (map, debugger.code_address);

						if ((strlen (debugger.code_cmd) == 0) && mi)
							map_remove_item (map, mi);
						else if (mi)
						{
							map_item_modify (mi, debugger.code_address, mi->size, debugger.code_cmd);
							map->modified = TRUE;
						}
						else
						{
							if (!map)
								gdebug_create_map ();

							mi = map_add_item (map, debugger.code_address, 4, debugger.code_cmd);
						}
					}
					break;
			}
			
			memset (debugger.code_cmd, 0, strlen (debugger.code_cmd));
			debugger.code_cmd_input = 0;
			debugger.hcodepos = 0;

			window_title_clear (win, 7, LINE_WIDTH);
			screen_redraw_window (&debugger.screen, win);
			screen_set_cursor_relative (&debugger.screen, win, 0, 0);
			return TRUE;

		default:
			if (buff_keystroke (debugger.code_cmd, &debugger.hcodepos, CODE_CMD_MAX_CHARS, key))
			{
				screen_redraw_window (&debugger.screen, win);
				screen_set_cursor_relative (&debugger.screen, win, debugger.hcodepos + CODE_CMD_CURSOR_OFFSET, 0);
				return TRUE;
			}
			else
				return FALSE;
	}

	return FALSE;
}


void on_code_resize (Window *win, int nlines, void *data)
{
	code_show (win, win->nlines, debugger.code_address);
	
	debugger.code_lines = nlines;
}


void on_code_refresh (Window *win, void *data)
{
	code_show (win, win->nlines, debugger.code_address);
}


void on_code_scroll (Window *win, int nlines, int direction, void *data)
{
	unsigned int addr;
	int i;


	if (direction == DIR_DOWN)
	{
		addr = debugger.code_address + win->nlines * 4;
		debugger.code_address += 4 * nlines;
	}
	else
	{
		addr = debugger.code_address - 4;
		debugger.code_address -= 4 * nlines;
	}
	
	// current address will be highlighted
	// refresh the whole window to remove highlight from other lines
	if (nlines == 1)
	{
		window_roll (win, 1, direction);
		on_code_refresh (win, NULL);

		return;
	}

	for (i = 0; i < nlines; i++)
	{
		if (direction == DIR_DOWN)
		{
			window_roll (win, 1, direction);
			code_show (win, 1, addr);
			addr += 4;
		}
		else
		{
			code_show (win, 1, addr);
			window_roll (win, 1, direction);
			addr -= 4;
		}
	}
}


void on_regs_refresh (Window *win, void *data)
{
	regs_show (win, (int) data);
}


int on_regs_keypress (Window *win, int key, int modifiers, void *data)
{
	switch (key)
	{
		case KEY_RIGHT:
			if (++debugger.regs_mode > REGS_MODE_LAST)
				debugger.regs_mode = REGS_MODE_LAST;
			screen_redraw (&debugger.screen);
			return TRUE;

		case KEY_LEFT:
			if (--debugger.regs_mode < 0)
				debugger.regs_mode = 0;
			screen_redraw (&debugger.screen);
			return TRUE;

		default:
			return FALSE;
	}

	return FALSE;
}



int on_keypress (Screen *win, int key, int modifiers, void *data)
{
	switch (key)
	{
		case KEY_F(5):
			debugger_cmd_run (NULL);
			screen_redraw (&debugger.screen);
			return TRUE;

		case KEY_F(7):
			PC += 4;
			debugger.code_address = PC;
			screen_redraw (&debugger.screen);
			return TRUE;

		case KEY_F(8):
		case 'n':
			debugger_cmd_step_into (NULL);
			screen_redraw (&debugger.screen);
			return TRUE;
		
		case KEY_F(9):
		case 's':
			debugger_cmd_step_over (NULL);
			screen_redraw (&debugger.screen);
			return TRUE;
		
		case KEY_F(11):
		case 'x':
			debugger_cmd_detach (NULL);
			screen_redraw (&debugger.screen);
			return TRUE;

		case KEY_F(12):
		case 'q':
			debugger_cmd_quit (NULL);
			return TRUE;
		
		default:
			return FALSE;
	}
}


int on_unhandled_keypress (Screen *win, int key, int modifiers, void *data)
{
	screen_select_window (&debugger.screen, SELECT_N0 + 4);
	return window_handle_keypress (debugger.console_win, key, modifiers);
}


void on_output_resize (Window *win, int nlines, void *data)
{
	if (nlines > debugger.output.nitems - debugger.output.pos)
	{
		debugger.output.pos -= nlines - (debugger.output.nitems - debugger.output.pos);

		if (debugger.output.pos < 0)
			debugger.output.pos = 0;
	}

	output_show (win, &debugger.output, nlines, debugger.output.pos);
	debugger.output.visitems = win->nlines;
	
	debugger.output_lines = nlines;
}


void on_output_refresh (Window *win, void *data)
{
	output_show (win, &debugger.output, win->nlines, debugger.output.pos);
}


void on_output_scroll (Window *win, int nlines, int direction, void *data)
{
	int pos, i;


	if (!debugger.output.nitems || debugger.output.nitems < win->nlines)
		return;

	if (direction == DIR_FIRST)
	{
		debugger.output.pos = 0;
		output_show (win, &debugger.output, win->nlines, debugger.output.pos);
	}
	else if (direction == DIR_LAST)
	{
		debugger.output.pos = debugger.output.nitems - win->nlines;
		output_show (win, &debugger.output, win->nlines, debugger.output.pos);
	}
	else if (direction == DIR_DOWN)
	{
		pos = debugger.output.pos + win->nlines;
		if (nlines >= debugger.output.nitems - pos)
			nlines = debugger.output.nitems - pos;

		debugger.output.pos += nlines;

		for (i = 0; i < nlines; i++)
		{
			window_roll (win, 1, direction);
			output_show (win, &debugger.output, 1, pos);
			pos++;
		}
	}
	else
	{
		pos = debugger.output.pos - 1;
		if (nlines > debugger.output.pos)
			nlines = debugger.output.pos;

		debugger.output.pos -= nlines;

		for (i = 0; i < nlines; i++)
		{
			output_show (win, &debugger.output, 1, pos);
			window_roll (win, 1, direction);
			pos--;
		}
	}
}


void on_command_refresh (Window *win, void *data)
{
	window_clear_line (win, 0);
	if (debugger.console.hpos)
		window_printf (win, 0, 0, ".command: %s", debugger.console.history[debugger.console.hlines - debugger.console.hpos]);
	else
		window_printf (win, 0, 0, ".command: %s", debugger.console.buff);
}


void on_command_enter (Window *win, void *data)
{
	screen_set_cursor_relative (&debugger.screen, win, debugger.console.pos + CONSOLE_CURSOR_OFFSET, 0);
}


int on_command_keypress (Window *win, int key, int modifiers, void *data)
{
	// if looking at history buffer entry, copy it to edit buffer
	if (debugger.console.hpos &&
			((key >= 0x20 && key < 0x7f) || (key == KEY_BACKSPACE) || (key == KEY_DC)))
	{
		strcpy (debugger.console.buff, debugger.console.history[debugger.console.hlines - debugger.console.hpos]);
		debugger.console.hpos = 0;
	}

	switch (key)
	{
		// enter
		case 0x0d:
			debugger_execute_command ();
			screen_redraw_window (&debugger.screen, win);
			screen_set_cursor_relative (&debugger.screen, win, debugger.console.pos + CONSOLE_CURSOR_OFFSET, 0);
			return TRUE;

		case KEY_DOWN:
			if (debugger.console.hpos > 0)
			{
				debugger.console.hpos--;
				if (debugger.console.hpos > 0)
					debugger.console.pos = strlen (debugger.console.history[debugger.console.hlines - debugger.console.hpos]);
				else
					debugger.console.pos = strlen (debugger.console.buff);
				screen_redraw_window (&debugger.screen, win);
				screen_set_cursor_relative (&debugger.screen, win, debugger.console.pos + CONSOLE_CURSOR_OFFSET, 0);
			}
			return TRUE;
			
		case KEY_UP:
			if (debugger.console.hpos < debugger.console.hlines)
			{
				debugger.console.hpos++;
				debugger.console.pos = strlen (debugger.console.history[debugger.console.hlines - debugger.console.hpos]);

				screen_redraw_window (&debugger.screen, win);
				screen_set_cursor_relative (&debugger.screen, win, debugger.console.pos + CONSOLE_CURSOR_OFFSET, 0);
			}
			return TRUE;

		case KEY_NPAGE:
		case KEY_PPAGE:
			if (!debugger.output_win->hidden && debugger.output_win->nlines > 1)
			{
				window_scroll (debugger.output_win, debugger.output_win->nlines - 1,
											 (key == KEY_NPAGE) ? DIR_DOWN : DIR_UP);
				screen_refresh (&debugger.screen);
				return TRUE;
			}
			else
				return FALSE;

		case KEY_END:
			if (debugger.console.hpos)
				debugger.console.pos = strlen (debugger.console.history[debugger.console.hlines - debugger.console.hpos]);
			else
				debugger.console.pos = strlen (debugger.console.buff);
			screen_redraw_window (&debugger.screen, win);
			screen_set_cursor_relative (&debugger.screen, win, debugger.console.pos + CONSOLE_CURSOR_OFFSET, 0);
			return TRUE;

		case KEY_RIGHT:
			if (debugger.console.hpos)
			{
				if (debugger.console.pos < strlen (debugger.console.history[debugger.console.hlines - debugger.console.hpos]))
				{
					debugger.console.pos++;
					screen_redraw_window (&debugger.screen, win);
					screen_set_cursor_relative (&debugger.screen, win, debugger.console.pos + CONSOLE_CURSOR_OFFSET, 0);
				}
				
				return TRUE;
			}
			// fall through
		default:
			if (buff_keystroke (debugger.console.buff, &debugger.console.pos, CONSOLE_MAX_CHARS, key))
			{
				screen_redraw_window (&debugger.screen, win);
				screen_set_cursor_relative (&debugger.screen, win, debugger.console.pos + CONSOLE_CURSOR_OFFSET, 0);
				return TRUE;
			}
			else
				return FALSE;
	}
}

typedef struct
{
	char *title;
	
	// callbacks
	void *keypress;
	void *refresh;
	void *resize;
	void *scroll;
} WinMode;

WinMode cwmodes[] =
{
	{ ".code:", on_code_keypress, on_code_refresh, on_code_resize, on_code_scroll },
	{ ".browser:", on_browser_keypress, on_browser_refresh, on_browser_resize, NULL },
};


void set_window_mode (Window *win, WinMode *mode)
{
	window_clear (win);
	window_set_title (win, mode->title);
		
	window_add_keypress_callback (win, mode->keypress, NULL);
	window_add_refresh_callback (win, mode->refresh, NULL);
	window_add_size_change_callback (win, mode->resize, NULL);
	window_add_scroll_callback (win, mode->scroll, NULL);
}


void cwin_change_mode (int mode)
{
	if (mode >= 0 && mode < CODE_WINDOW_NMODES && mode != debugger.cmode)
	{
		debugger.cmode = mode;
		
		set_window_mode (debugger.code_win, &cwmodes[mode]);

		window_refresh (debugger.code_win);
		screen_refresh (&debugger.screen);
	}
}


//
void sprint_intmask (char *buff, __u32 b)
{
	__u32 bitmasks[] =
	{
		INTERRUPT_HSP, INTERRUPT_DEBUG, INTERRUPT_CP, INTERRUPT_PE_FINISH,
		INTERRUPT_PE_TOKEN, INTERRUPT_VI, INTERRUPT_MEM, INTERRUPT_DSP,
		INTERRUPT_AI, INTERRUPT_EXI, INTERRUPT_SI, INTERRUPT_DI,
		INTERRUPT_RSW, INTERRUPT_ERROR,
	};
	char *bitnames[] = 
	{
		"HSP", "DBG", "CP", "PFIN", "PTOK", "VI", "MEM", "DSP", "AI",
		"EXI", "SI", "DI", "RSW", "ERROR",
	};
	int i;


	for (i = 0; i < sizeof (bitmasks) / 4; i++)
		if (b & bitmasks[i])
		{
			strcat (buff, bitnames[i]);
			strcat (buff, " ");
		}

	i = strlen (buff);
	// kill last space
	if (i)
		buff[i - 1] = '\0';
}


void gdebug_print_intmask (__u32 mask, char *msg)
{
	char buff[256] = {0};


	sprint_intmask (buff, INTSR);
	DEBUG (EVENT_LOG_PI, "%s%s", msg, buff);
}


void gdebug_mem_read (__u32 addr)
{
	Breakpoint *bp;


	if (!gdebug_running)
		return;

	if ((bp = debugger.breakpoints[(addr & MEM_MASK) >> 2]) && bp->enabled &&
			(bp->type & BP_MEM_READ))
		DEBUG (EVENT_BREAKPOINT, "breakpoint on memory read at %.8x", bp->address);
}


void gdebug_mem_write (__u32 addr)
{
	Breakpoint *bp;


	if (!gdebug_running)
		return;

	addr &= ~3;

	if ((bp = debugger.breakpoints[(addr & MEM_MASK) >> 2]) && bp->enabled)
	{
		if (bp->type & BP_MEM_WRITE)
			DEBUG (EVENT_BREAKPOINT, "breakpoint on memory write at %.8x", bp->address);

		if ((bp->type & BP_MEM_CHANGE) && (MEM32 (addr) != bp->val))
		{
			DEBUG (EVENT_BREAKPOINT, "breakpoint on memory change at %.8x", bp->address);
			bp->val = MEM32 (addr);
		}
	}
}


void gdebug_hw_read (__u32 addr)
{
	Breakpoint *bp;


	if (!gdebug_running)
		return;

	addr &= ~3;

	if ((bp = debugger.hwbreakpoints[addr & 0xffff]) &&
			bp->enabled && (bp->type & BP_MEM_READ))
		DEBUG (EVENT_BREAKPOINT, "breakpoint on hw read at %.8x", bp->address);
}


void gdebug_hw_write (__u32 addr)
{
	Breakpoint *bp;


	if (!gdebug_running)
		return;

	if ((bp = debugger.hwbreakpoints[addr & 0xffff]) &&
			bp->enabled && (bp->type & BP_MEM_WRITE))
		DEBUG (EVENT_BREAKPOINT, "breakpoint on hw write at %.8x", bp->address);
}


void gdebug_create_map (void)
{
	char buff[256];


	if (map)
		return;

	strcpy (buff, gcbin_filename);
	kill_extension (buff);
	strcat (buff, ".map");
	map = map_create (buff);
}


void gdebug_run (__u32 pc)
{
	Window *win;


	debugger.code_address = pc;

	screen_init (&debugger.screen, debugger.colors);

	if (!debugger.initialized)
	{
		debugger.mem_lines = 3;
		debugger.code_lines = 5;
		debugger.output_lines = 3;

		debugger.refresh_rate = 100000;
		debugger.compress_states = TRUE;

		debugger.initialized = TRUE;
		debugger.log_enable = 0;

		debugger.refresh_delay = ref_delay;
		
		debugger.output_code_lines = FALSE;
		debugger.min_refresh = 1;
	}

	debugger.regs_win = win = window_create (0, 0, 8, ".regs:");
	if (!win)
	{
		screen_cleanup (&debugger.screen);
		printf ("textmode error: window couldn't be created");
		exit (1);
	}
	
	win->fixed_size = TRUE;
	if (debugger.regs_hidden)
		win->hidden = TRUE;
	window_add_refresh_callback (win, on_regs_refresh, NULL);
	window_add_keypress_callback (win, on_regs_keypress, NULL);
	screen_add_window (&debugger.screen, win);

	debugger.mem_win = win = window_create (0, 0, debugger.mem_lines, ".mem:");
	if (debugger.mem_hidden)
		win->hidden = TRUE;
	window_add_refresh_callback (win, on_mem_refresh, NULL);
	window_add_size_change_callback (win, on_mem_resize, NULL);
	window_add_scroll_callback (win, on_mem_scroll, NULL);
	window_add_keypress_callback (win, on_mem_keypress, NULL);
	screen_add_window (&debugger.screen, win);

	debugger.code_win = win = window_create (0, 0, debugger.code_lines, ".code:");
	if (debugger.code_hidden)
		win->hidden = TRUE;
	set_window_mode (debugger.code_win, &cwmodes[debugger.cmode]);
	screen_add_window (&debugger.screen, win);

	debugger.output_win = win = window_create (0, 0, debugger.output_lines, ".output:");
	debugger.output.visitems = debugger.output_lines;
	if (debugger.output_hidden)
		win->hidden = TRUE;
	window_add_refresh_callback (win, on_output_refresh, NULL);
	window_add_size_change_callback (win, on_output_resize, NULL);
	window_add_scroll_callback (win, on_output_scroll, NULL);
	screen_add_window (&debugger.screen, win);

	debugger.console_win = win = window_create (0, 0, 1, ".command:");
	win->no_title = TRUE;
	win->fixed_size = TRUE;
	win->always_visible = TRUE;
	window_add_keypress_callback (win, on_command_keypress, NULL);
	window_add_enter_callback (win, on_command_enter, NULL);
	window_add_refresh_callback (win, on_command_refresh, NULL);
	screen_add_window (&debugger.screen, win);

	screen_collapse (&debugger.screen, 0);
	screen_expand (&debugger.screen, 0);

	screen_add_keypress_callback (&debugger.screen, on_keypress, NULL);
	screen_add_unhandled_keypress_callback (&debugger.screen, on_unhandled_keypress, NULL);
	screen_redraw (&debugger.screen);
	screen_select_window (&debugger.screen, SELECT_LAST);

	while (!(emu_state & GDEBUG_EXIT))
		screen_handle_input (&debugger.screen, screen_getkey (&debugger.screen));

	debugger.regs_hidden = debugger.regs_win->hidden;
	debugger.mem_hidden = debugger.mem_win->hidden;
	debugger.code_hidden = debugger.code_win->hidden;
	debugger.output_hidden = debugger.output_win->hidden;
	screen_cleanup (&debugger.screen);

	emu_state &= ~GDEBUG_EXIT;
	gdebug_running = FALSE;
}


void gdebug_cleanup (void)
{
	output_destroy (&debugger.output);
	command_history_destroy (&debugger.console);

	debugger_breakpoints_destroy ();	
}

#endif // GDEBUG


void gcube_save_map (void)
{
	if (!map)
		return;

	map_save (map, NULL);
	DEBUG (EVENT_DEBUG_MSG, "saved map to file %s", map->filename);
}


void generate_map (void)
{
	if (!mdb)
	{
		DEBUG (EVENT_DEBUG_MSG, "preparing function database...");
		mdb = map_extract_db (mapdb);
	}

	if (map)
		map_destroy (map);

	DEBUG (EVENT_DEBUG_MSG, "searching for functions...");
	map = map_generate_nameless ();
	DEBUG (EVENT_DEBUG_MSG, "guessing function names...");
	map_generate_names (map, mdb);
	DEBUG (EVENT_DEBUG_MSG, "function map generated");
}


void enable_ignore_movies (int level)
{
	MapItem *mi;


	if (!map)
		generate_map ();

	if (level)
	{
		if ((mi = map_get_item_by_name (map, "THPVideoDecode")))
			map_item_hle_with (mi, "ignore_true");
		else
			DEBUG (EVENT_DEBUG_MSG, "function THPVideoDecode not mapped.");

		if ((mi = map_get_item_by_name (map, "THPPlayerGetState")))
			map_item_hle_with (mi, "THPPlayerGetState_ignore_movies");
		else
			DEBUG (EVENT_DEBUG_MSG, "function THPPlayerGetState not mapped.");

		if (level > 1)
		{
			if ((mi = map_get_item_by_name (map, "DVDOpen")))
				map_item_hle_with (mi, "DVDOpen_ignore_movies");
			else
				DEBUG (EVENT_DEBUG_MSG, "function DVDOpen not mapped.");

			if ((mi = map_get_item_by_name (map, "DVDConvertPathToEntrynum")))
				map_item_hle_with (mi, "DVDConvertPathToEntrynum_ignore_movies");
			else
				DEBUG (EVENT_DEBUG_MSG, "function DVDConvertPathToEntrynum not mapped.");
		}

		DEBUG (EVENT_DEBUG_MSG, "movieignore is on");
	}
	else
	{
		if ((mi = map_get_item_by_name (map, "THPVideoDecode")))
			map_item_emulation (mi, EMULATION_LLE, 0);

		if ((mi = map_get_item_by_name (map, "DVDOpen")))
			map_item_emulation (mi, EMULATION_LLE, 0);

		if ((mi = map_get_item_by_name (map, "DVDConvertPathToEntrynum")))
			map_item_emulation (mi, EMULATION_LLE, 0);

		DEBUG (EVENT_DEBUG_MSG, "movieignore is off");
	}
}


void enable_hle_input (int enable)
{
	if (!map)
		generate_map ();

	if (enable)
	{
		if (hle_function (map, "PADRead", TRUE))
			DEBUG (EVENT_DEBUG_MSG, "function PADRead not mapped.");

		DEBUG (EVENT_DEBUG_MSG, "hle input is on");
	}
	else
	{
		hle_function (map, "PADRead", FALSE);
		DEBUG (EVENT_DEBUG_MSG, "hle input is off");
	}
}


void enable_hle (int enable)
{
	if (!map)
		generate_map ();

	if (enable)
	{
		hle (map, TRUE);
#ifdef GDEBUG
		hle_enabled = TRUE;
#endif

		DEBUG (EVENT_DEBUG_MSG, "hle is on");
	}
	else
	{
		hle (map, FALSE);
#ifdef GDEBUG
		hle_enabled = FALSE;
#endif

		DEBUG (EVENT_DEBUG_MSG, "hle is off");
	}
}


void gcube_save_state (void)
{
	do_save_state = TRUE;
	gcube_running = FALSE;

#ifdef GDEBUG
	gdebug_running = FALSE;
#endif
}


void gcube_save_state_delayed (void)
{
	char filename[1024];


	strcpy (filename, gcbin_filename);
	kill_extension (filename);
	strcat (filename, ".gcs");
		
	// detach all hle functions
	if (hle_enabled)
		hle (map, FALSE);

	save_state (filename, FALSE);

	if (hle_enabled)
		hle (map, TRUE);
}


void gcube_load_state (void)
{
	char filename[1024];


	strcpy (filename, gcbin_filename);
	kill_extension (filename);
	strcat (filename, ".gcs");

	// detach all hle functions
	if (hle_enabled)
		hle (map, FALSE);

	load_state (filename);

	if (hle_enabled)
		hle (map, TRUE);
}


void gcube_run (void)
{
	gcube_running = TRUE;
	while (gcube_running)
		cpu_execute ();
	
	// savestates have to be created outside cpu_execute
	if (do_save_state)
	{
		do_save_state = FALSE;
		gcube_save_state_delayed ();
		gcube_run ();
	}
}


void gcube_load_map (char *filename)
{
	char buff[256];


	sprintf (buff, "%s.map", filename);
	if (!(map = map_load (buff)))
	{
		strcpy (buff, filename);
		kill_extension (buff);
		strcat (buff, ".map");
	}

	map = map_load (buff);

	if (map)
		DEBUG (EVENT_LOG, "loaded map file %s with %d entries", buff, map->nitems);
}


int gcube_load_file (char *filename)
{
	if (!file_exists (filename))
		return FALSE;

	strcpy (gcbin_filename, filename);

	if (mdvd_inserted ())
		mdvd_close ();

	mem_reset ();
	cpu_init ();
	hw_init ();

	// try disk image
	if (is_gcm (filename))
	{
		// set gcube_running to enable BS2Report
		__u32 last_gcube_running = gcube_running;
	
		mdvd_open (filename);
		hw_set_video_mode (mdvd_get_country_code ());
		gcube_running = TRUE;
		boot_apploader ();
		gcube_running = last_gcube_running;
		gcube_load_map (filename);
		return PC;
	}

	PC = load_gc (filename);
	if (PC == GC_FILE_ERROR)
	{
		PC = 0;
		return FALSE;
	}
	else if (PC == 0)
	{
		gcube_load_map (filename);
		return TRUE;
	}

	if (is_gcs (filename))
	{
		hw_reinit ();
		
		// reinit fb, only for homebrew
		if (!mdvd_inserted ())
			video_fb_reinit ();
	}
	else
	// hw_init must be called after the file is loaded
		hw_init ();

	gcube_load_map (filename);

	if (!mdvd_inserted ())
	{
	  if (strlen (mount_dir))
  		vdvd_open (mount_dir);
	  else
	  {
		  get_path (mount_dir, filename);
		  strcat (mount_dir, "dvdroot");
		  vdvd_open (mount_dir);
	  }
	}

//	hw_set_video_mode ('J');

	return TRUE;
}


void gcube_os_report (char *msg, int newline)
{
	DEBUG (EVENT_OSREPORT, msg);

	if (gcube_running)
		printf ("%s%s", msg, (newline) ? "\n" : "");
}


void gcube_quit (char *msg)
{
#ifdef GDEBUG
	gdebug_running = FALSE;
#endif
	gcube_running = FALSE;
	emu_state = EMU_EXIT | GDEBUG_EXIT;

	quit_message = msg;
}


int main (int argc, char **argv)
{
	int option_index = 0, c;
	struct option long_options[] =
	{
		{"help", 0, 0, 'h'},
		{"debug", 0, 0, 'd'},
		{"fullscreen", 0, 0, 'f'},
		{"mount", 1, 0, 'm'},
		{"color-mode", 1, 0, 'c'},
		{"ignore-movies", 1, 0, 'i'},
		{"refresh-delay", 1, 0, 'r'},
		{"hle", 0, 0, 'l'},
		{"hle-input", 0, 0, 'p'},
		{"save-maps", 1, 0, 's'},
		{"fix-flickering", 0, 0, '0'},
		{"fix-blending", 0, 0, '1'},
		{0, 0, 0, 0},
	};
#ifdef GDEBUG
	int colors = DEFAULT_COLOR_MODE;
#endif
	int fullscreen = FALSE;
	int ignore_movies = 0;
	int hle_input = FALSE, use_hle = FALSE, save_gen_map = FALSE;


	opterr = 0;
	while (-1 != (c = getopt_long (argc, argv, "hdf01m:c:i:r:pls", long_options, &option_index)))
	{
		switch (c)
		{
			case 'h':
				printf ("%s: Open source gamecube emulator\n", GCUBE_DESCRIPTION);
				printf ("Usage: gcube [OPTIONS] file\n");
				printf ("  -d, --debug                      run debugger at start\n");
				printf ("  -f, --fullscreen                 run in fullscreen\n");
				printf ("  -m, --mount=directory            mount virtual dvd root directory\n");
#ifdef GDEBUG
				printf ("  -c, --color-mode=MODE            debugger color mode:\n");
				printf ("                                    0 - black and white\n");
				printf ("                                    1 - default with transparent background\n");
				printf ("                                    2 - default with white background\n");
#endif
				printf ("  -i, --ignore-movies=MODE         tries to disable movie playback:\n");
				printf ("                                    0 - disabled (movie playback is on)\n");
				printf ("                                    1 - enabled, disables THPVideoDecode\n");
				printf ("                                    2 - enabled, disables loading of\n");
				printf ("                                        THP, H4M and STR files\n");
				printf ("  -r, --refresh-delay=VALUE        number of instructions executed before\n");
				printf ("                                   the video interrupt occurs\n");
				printf ("  -l, --hle                        use high level emulation\n");
				printf ("  -p, --hle-input                  use only high level input emulation\n");
				printf ("  -s, --save-maps                  save generated maps\n");
				printf ("      --fix-flickering             fix flickering (sonic mega collection)\n");
				printf ("      --fix-blending               fix blending (aggressive inline)\n");
				printf ("  -h, --help                       display this help and exit\n");
				return FALSE;

#ifdef GDEBUG
			case 'd':
				emu_state = GDEBUG_RUN;
				break;

			case 'c':
				sscanf (optarg, "%d", &colors);
				break;
#endif

			case 'm':
				strcpy (mount_dir, optarg);
				break;
			
			case 'f':
				fullscreen = TRUE;
				break;
			
			case 'r':
				sscanf (optarg, "%d", &ref_delay);
				break;
			
			case 'i':
				sscanf (optarg, "%d", &ignore_movies);
				break;

			case 'l':
				use_hle = TRUE;
				break;

			case 'p':
				hle_input = TRUE;
				break;

			case 's':
				save_gen_map = TRUE;
				break;
			
			case '0':
				gx_switch (GX_TOGGLE_FIX_FLICKERING);
				break;
			
			case '1':
				gx_switch (GX_TOGGLE_FIX_BLENDING);
				break;
		}
	}

	if (optind >= argc)
	{
		printf ("Usage: gcube [OPTIONS] file\n");
		printf ("Try 'gcube --help' for more information.\n");
		return FALSE;
	}

	create_dir_tree (".gcube-" GCUBE_VERSION);

	// fullscreen only if debugger isn't running
	if (emu_state == GCUBE_RUN)
		video_set_fullscreen (fullscreen);

	DEBUG (EVENT_INFO, "%s (c) 2004 by monk", GCUBE_DESCRIPTION);
	if (!gcube_load_file (argv[optind]))
	{
		printf ("can't open file %s\n", argv[optind]);
		return FALSE;
	}

	config_load ("gcuberc");

#ifdef OPEN_WINDOW_ON_START
	video_init (640, 480);
#endif

	if (!map && (ignore_movies || hle_input || use_hle))
	{
		generate_map ();
		if (save_gen_map)
		{
			char filename[1024];
			
			strcpy (filename, argv[optind]);
			kill_extension (filename);
			strcat (filename, ".map");

			map->filename = strdup (filename);
			map->modified = TRUE;
		}
	}

	if (ignore_movies)
		enable_ignore_movies (ignore_movies);
		
	if (use_hle)
		enable_hle (use_hle);
	else if (hle_input)
		enable_hle_input (hle_input);

#ifdef GDEBUG
	debugger.colors = colors;
	debugger.refresh_delay = ref_delay;
	debugger.min_refresh = 1;

	browser_change_path (&debugger.browser, ".");

	debugger.starting_pc = PC;
	signal (SIGINT, sig_ctrl_c);

	while (!(emu_state & EMU_EXIT))
	{
		emu_state |= EMU_EXIT;

		if (emu_state & GCUBE_RUN)
			gcube_run ();

		if (emu_state & GDEBUG_RUN)
			gdebug_run (PC);
	}

	gcube_save_map ();

#else
	gcube_run ();
	
	if (save_gen_map)
		gcube_save_map ();
#endif // GDEBUG

	map_destroy (map);
	map_destroy (mdb);

	if (quit_message)
		printf ("%s\n", quit_message);

	config_save ("gcuberc");

	return FALSE;
}

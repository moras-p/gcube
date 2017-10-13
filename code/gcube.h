#ifndef __GCUBE_H
#define __GCUBE_H 1



#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>

#include "general.h"
#include "config.h"

#include "ppc_disasm.h"
#include "cpu.h"
#include "diskio.h"
#include "hw.h"

#include "txlib.h"
#include "hw_di.h"
#include "hle.h"
#include "gdebug.h"


#define GDEBUG_RUN				0x001
#define GDEBUG_RUNNING		0x002
#define GDEBUG_EXIT				0x004
#define GCUBE_RUN					0x010
#define GCUBE_RUNNING			0x020
#define GCUBE_EXIT				0x040
#define EMU_EXIT					0x100

#define CONSOLE_CURSOR_OFFSET			10
#define CONSOLE_MAX_CHARS					(LINE_WIDTH - CONSOLE_CURSOR_OFFSET - 1)
#define CODE_CMD_CURSOR_OFFSET		7
#define CODE_CMD_MAX_CHARS				(LINE_WIDTH - CODE_CMD_CURSOR_OFFSET - 1)

#define MAX_HISTORY_LINES					4096
#define MAX_OUTPUT_LINES					800000
#define MAX_BREAKPOINTS						1024
#define BREAKPOINT_FIRST_HW				512

#define CODE_CMD_SET_LABEL				1
#define CODE_CMD_SET_COMMENT			2

#define VAR_INTSR				0
#define VAR_INTMR				1


#define CODE_WINDOW_NMODES				2

#define REGS_MODE_GPR							0
#define REGS_MODE_FPR							1
#define REGS_MODE_PS0							2
#define REGS_MODE_PS1							3
#define REGS_MODE_LAST						3


#define LOG_ENABLE_INT						0x00000001
#define LOG_ENABLE_AI							0x00000100
#define LOG_ENABLE_DSP						0x00000200
#define LOG_ENABLE_CP							0x00000400
#define LOG_ENABLE_DI							0x00000800
#define LOG_ENABLE_EXI						0x00001000
#define LOG_ENABLE_MI							0x00002000
#define LOG_ENABLE_PE							0x00004000
#define LOG_ENABLE_PI							0x00008000
#define LOG_ENABLE_SI							0x00010000
#define LOG_ENABLE_VI							0x00020000
#define LOG_ENABLE_GX							0x00040000
#define LOG_ENABLE_HLE						0x00080000
#define LOG_ENABLE_CALLS					0x00100000

#define LOG_ENABLE_ALL						(LOG_ENABLE_INT | LOG_ENABLE_AI | LOG_ENABLE_DSP | LOG_ENABLE_CP | LOG_ENABLE_DI | LOG_ENABLE_EXI | LOG_ENABLE_MI | LOG_ENABLE_PE | LOG_ENABLE_PI | LOG_ENABLE_SI | LOG_ENABLE_VI | LOG_ENABLE_GX)

#define BWIN_MODE_HEX							0
#define BWIN_MODE_FLOAT						1

#define BWIN_NMODES								2

#define DUMP_BUFFER_SIZE					65536

typedef struct
{
	char buff[LINE_WIDTH+1];
	int pos;
	char *history[MAX_HISTORY_LINES+1];
	int hlines;
	int hpos;
} CommandWin;


typedef struct
{
	char *items[MAX_OUTPUT_LINES];
	int nitems;
	int visitems;
	int pos;
	
	int nfirst_item;
} OutputWin;


#define DIR_ITEM_FILE					0
#define DIR_ITEM_DIRECTORY		1
#define DIR_ITEM_EXECUTABLE		2
#define DIR_ITEM_GCM					3

typedef struct
{
	char *name;
	int type;
} DirectoryItem;

typedef struct
{
	char *path;
	int nitems;
	DirectoryItem *items;
} Directory;

typedef struct
{
	int pos, start, nitems;
	Directory dir;
	Dir *dvd_dir, *current_dir;
	File *file;
} Browser;


#define BP_CODE				0
#define BP_MEM_READ		1
#define BP_MEM_WRITE	2
#define BP_MEM_CHANGE	4

#define BP_FLAGS_CMP_R3		1

typedef struct
{
	__u32 address;
	int enabled;
	int type;
	int num;
	int hidden;
	__u32 val;
	// hidden should be a flag
	int flags;
} Breakpoint;


typedef struct
{
	Screen screen;
	CommandWin console;
	OutputWin output;
	Browser browser;
	Window *regs_win, *mem_win, *code_win, *output_win, *console_win;

	__u32 mem_address;
	__u32 code_address;

	int initialized;
	unsigned int mem_lines, code_lines, output_lines;
	int regs_hidden, mem_hidden, code_hidden, output_hidden;

	int regs_mode;
	int bmode, cmode;

	__u32 starting_pc;
	__u32 last_pc;

	int novis;
	__u32 refresh_rate;
	int compress_states;
	unsigned int refresh_delay;
	unsigned int min_refresh;
	int output_code_lines;
	__u32 result;
	
	Breakpoint *breakpoints[0x05ffffff];
	int nbreakpoints;
	int break_on_exception;
	Breakpoint *hwbreakpoints[0xffff];
	int nhwbreakpoints;
	int break_on_uhw_read;
	int break_on_uhw_write;
	Breakpoint *bpbynum[MAX_BREAKPOINTS];

	char filename[256];

	int code_cmd_input;
	char code_cmd[LINE_WIDTH];
	int hcodepos;
	
	int colors;

	int log_enable;
} DebuggerState;


typedef struct
{
	char *name;
	char *description;
	void (*action) (char *args);
} Command;


typedef struct
{
	const char *s;
	unsigned int ui;
} UIDef;


typedef struct
{
	const char *s;
	unsigned int *uip;
} UIPDef;


void gcube_run (void);
int gcube_load_file (char *filename);
void gcube_save_state_delayed (void);


#endif // __GCUBE_H

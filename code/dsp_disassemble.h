//#pragma once

#include "dsp_types.h"
#include <stdio.h>

typedef struct gd_globals_t
{
	bool	print_tabs;
	bool	show_hex;
	bool	show_pc;
	bool	decode_names;
	bool	decode_registers;

	uint16	*binbuf;
	uint16	pc, isize;
	char	*buffer;
	uint16	buffer_size;
	char	ext_separator;
} gd_globals_t;


bool disassemble(char *name, FILE *output);


extern uint8	decode_names;
extern uint8	decode_registers;
extern uint8	show_hex;

gd_globals_t *gd_init(void);
char *gd_dis_opcode(gd_globals_t *gdg);
uint16 gd_dis_get_opcode_size(gd_globals_t *gdg);
bool gd_dis_file(gd_globals_t *gdg, char *name, FILE *output);
void gd_dis_close_unkop(void);
void gd_dis_open_unkop(void);
void gd_dis_mem (gd_globals_t *gdg, uint16 *mem, int size);
void gd_dis_mem_fast (uint16 *mem, int size);

int dsp_disassemble (char **buff, uint16 *mem);

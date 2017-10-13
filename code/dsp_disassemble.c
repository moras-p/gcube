/*====================================================================

filename:     disassemble.cpp
project:      GameCube DSP Tool (gcdsp)
created:      2005.03.04
mail:		  duddie@walla.com

Copyright (c) 2005 Duddie

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

====================================================================*/

#include <stdio.h>
//#include <tchar.h>
#include <memory.h>
#include <stdlib.h>

#include "dsp_types.h"
#include "dsp_opcodes.h"
#include "dsp_disassemble.h"
//#include "gdsp_tool.h"

//#pragma warning(disable:4996)

uint32 unk_opcodes[0x10000];

inline uint16 swap16(uint16 x)
{
	return (x >> 8) | (x << 8);
}

// predefined labels
typedef struct
{
	uint16	addr;
	char	*name;
	char	*description;
} pdlabel_t; //type: pdlabels_t instead of pdlabel_t

pdlabel_t pdlabels[] = 
{
	{ 0xffc9, "DSCR", "DSP DMA Control Reg", },
	{ 0xffcb, "DSBL", "DSP DMA Block Length", },
	{ 0xffcd, "DSPA", "DSP DMA DMEM Address", },
	{ 0xffce, "DSMAH", "DSP DMA Mem Address H", },
	{ 0xffcf, "DSMAL", "DSP DMA Mem Address L", },
	{ 0xffd4, "ACSAH", "", },
	{ 0xffd5, "ACSAL", "", },
	{ 0xffd6, "ACEAH", "", },
	{ 0xffd7, "ACEAL", "", },
	{ 0xffd8, "ACCAH", "", },
	{ 0xffd9, "ACCAL", "", },
	{ 0xffef, "AMDM", "ARAM DMA Request Mask", },
	{ 0xfffb, "DIRQ", "DSP Irq Request", },
	{ 0xfffc, "DMBH", "DSP Mailbox H", },
	{ 0xfffd, "DMBL", "DSP Mailbox L", },
	{ 0xfffe, "CMBH", "CPU Mailbox H", },
	{ 0xffff, "CMBL", "CPU Mailbox L", },
};

pdlabel_t regnames[] =
{
	{ 0x00, "R00", "Register 00", },
	{ 0x01, "R01", "Register 00", },
	{ 0x02, "R02", "Register 00", },
	{ 0x03, "R03", "Register 00", },
	{ 0x04, "R04", "Register 00", },
	{ 0x05, "R05", "Register 00", },
	{ 0x06, "R06", "Register 00", },
	{ 0x07, "R07", "Register 00", },
	{ 0x08, "R08", "Register 00", },
	{ 0x09, "R09", "Register 00", },
	{ 0x0a, "R0a", "Register 00", },
	{ 0x0b, "R0b", "Register 00", },
	{ 0x0c, "R0c", "Register 00", },
	{ 0x0d, "R0d", "Register 00", },
	{ 0x0e, "R0e", "Register 00", },
	{ 0x0f, "R0f", "Register 00", },
	{ 0x00, "ACH0", "Accumulator High 0", },
	{ 0x11, "ACH1", "Accumulator High 1", },
	{ 0x12, "R12", "Register 00", },
	{ 0x13, "R13", "Register 00", },
	{ 0x14, "R14", "Register 00", },
	{ 0x15, "R15", "Register 00", },
	{ 0x16, "R16", "Register 00", },
	{ 0x17, "R17", "Register 00", },
	{ 0x18, "R18", "Register 00", },
	{ 0x19, "R19", "Register 00", },
	{ 0x1a, "R1a", "Register 00", },
	{ 0x1b, "R1b", "Register 00", },
	{ 0x1c, "R1c", "Register 00", },
	{ 0x1d, "R1d", "Register 00", },
	{ 0x1e, "ACL0", "Register 00", },
	{ 0x1f, "ACL1", "Register 00", },
};

char tmpstr[12];
char * pdname(uint16 val)
{
	int i;
	for(i = 0 ; i < sizeof(pdlabels)/sizeof(pdlabel_t) ; i++)
	{
		if (pdlabels[i].addr == val)
			return pdlabels[i].name;
	}
	sprintf(tmpstr, "0x%04x", val);
	return tmpstr;
}

char *gd_dis_params(gd_globals_t *gdg, opc_t *opc, uint16 op1, uint16 op2, char *strbuf)
{
	char *buf = strbuf;
	uint32 val;
	int j;

	for (j = 0 ; j < opc->param_count ; j++)
	{
		if (j > 0)
		{
			sprintf(buf, ", ");
			buf += strlen(buf);
		}
		if (opc->params[j].loc >= 1)
			val = op2;
		else
			val = op1;

		val &= opc->params[j].mask;

		if (opc->params[j].lshift < 0)
			val = val << (-opc->params[j].lshift);
		else
			val = val >> opc->params[j].lshift;

		uint32 type;
		type = opc->params[j].type;
		if (type & P_REG)
		{

			if (type == P_ACCD)
				val = (~val & 0x1) | ((type & P_REGS_MASK) >> 8);
			else
				val |= (type & P_REGS_MASK) >> 8;
			type &= ~P_REGS_MASK;
		}

		switch(type)
		{
		case P_REG:
			if (gdg->decode_registers) sprintf(buf, "$%s", regnames[val].name);
			else sprintf(buf, "$%d", val);
			break;
		case P_PRG:
			if (gdg->decode_registers) sprintf(buf, "@$%s", regnames[val].name);
			else sprintf(buf, "@$%d", val);
			break;
		case P_VAL:
			sprintf(buf, "0x%04x", val);
			break;
		case P_IMM:
			if (opc->params[j].size != 2)
				sprintf(buf, "#0x%02x", val);
			else
				sprintf(buf, "#0x%04x", val);
			break;
		case P_MEM:
			if (opc->params[j].size != 2)
				val = (uint16)(sint8)val;
			if (gdg->decode_names)
				sprintf(buf, "@%s", pdname(val));
			else
				sprintf(buf, "@0x%04x", val);
			break;
		default:
			fprintf(stderr, "Unknown parameter type: %x\n", opc->params[j].type);
			exit(-1);
			break;
		}
		buf += strlen(buf);
	}
	return strbuf;
}


gd_globals_t *gd_init(void)
{
	gd_globals_t *gdg;

	gdg = (gd_globals_t *)malloc(sizeof(gd_globals_t));
	memset(gdg, 0, sizeof(gd_globals_t));
	return gdg;
}

uint16 gd_dis_get_opcode_size(gd_globals_t *gdg)
{
	uint32 j;
	opc_t *opc = NULL;
	opc_t	*opc_ext = NULL;
	uint32	op1;
	bool	extended;

	if ((gdg->pc & 0x7fff) >= 0x1000)
	{
		return 1;
	}
	op1 = swap16(gdg->binbuf[gdg->pc & 0x0fff]);

	for(j = 0 ; j < opcodes_size ; j++)
	{
		uint16 mask;
		if (opcodes[j].size & P_EXT)
			mask = opcodes[j].opcode_mask & 0xff00;
		else
			mask = opcodes[j].opcode_mask;
		if ((op1 & mask) == opcodes[j].opcode)
		{
			opc = &opcodes[j];
			break;
		}
	}
	if (opc->size & P_EXT && op1 & 0x00ff)
		extended = true;
	else
		extended = false;

	if (extended)
	{
		// opcode has an extension
		// find opcode
		for(j = 0 ; j < opcodes_ext_size ; j++)
		{
			if ((op1 & opcodes_ext[j].opcode_mask) == opcodes_ext[j].opcode)
			{
				opc_ext = &opcodes_ext[j];
				break;
			}
		}
		return opc_ext->size;
	}

	return opc->size & ~P_EXT;
}
char *gd_dis_opcode(gd_globals_t *gdg)
{
	uint32	j;
	uint32	op1, op2;
	opc_t	*opc = NULL;
	opc_t	*opc_ext = NULL;
	uint16	pc;
	char	*buf = gdg->buffer;
	bool	extended;

	pc = gdg->pc;
	*buf = '\0';
	if ((pc & 0x7fff) >= 0x1000)
	{
		gdg->pc++;
		return gdg->buffer;
	}
	pc &= 0x0fff;
	op1 = swap16(gdg->binbuf[pc]);

	// find opcode
	for(j = 0 ; j < opcodes_size ; j++)
	{
		uint16 mask;
		if (opcodes[j].size & P_EXT)
			mask = opcodes[j].opcode_mask & 0xff00;
		else
			mask = opcodes[j].opcode_mask;
		if ((op1 & mask) == opcodes[j].opcode)
		{
			opc = &opcodes[j];
			break;
		}
	}

	if (opc->size & P_EXT && op1 & 0x00ff)
		extended = true;
	else
		extended = false;

	if (extended)
	{
		// opcode has an extension
		// find opcode
		for(j = 0 ; j < opcodes_ext_size ; j++)
		{
			if ((op1 & opcodes_ext[j].opcode_mask) == opcodes_ext[j].opcode)
			{
				opc_ext = &opcodes_ext[j];
				break;
			}
		}
	}

	// printing

	if (gdg->show_pc) sprintf(buf, "%04x ", gdg->pc);
	buf += strlen(buf);

	if ((opc->size & ~P_EXT) == 2)
	{
		op2 = swap16(gdg->binbuf[pc + 1]);
		if (gdg->show_hex) sprintf(buf, "%04x %04x ", op1, op2);
	}
	else
	{
		op2 = 0;
		if (gdg->show_hex) sprintf(buf, "%04x      ", op1);
	}
	buf += strlen(buf);

	char tmpbuf[20];

	if (extended)
		sprintf(tmpbuf, "%s%c%s", opc->name, gdg->ext_separator, opc_ext->name);
	else
		sprintf(tmpbuf, "%s", opc->name);

	if (gdg->print_tabs)
		sprintf(buf, "%s\t", tmpbuf);
	else
		sprintf(buf, "%-12s", tmpbuf);
	buf += strlen(buf);
	if (opc->param_count > 0)
	{
		gd_dis_params(gdg, opc, op1, op2, buf);
	}
	buf += strlen(buf);
	
	if(extended)
	{
		if (opc->param_count > 0)
			sprintf(buf, " ");
		buf += strlen(buf);

		sprintf(buf, ": ");
		buf += strlen(buf);

		if (opc_ext->param_count > 0)
		{
			gd_dis_params(gdg, opc_ext, op1, op2, buf);
		}
		buf += strlen(buf);
	}

	if (opc->opcode_mask == 0)
	{
		// unknown opcode
		unk_opcodes[op1]++;
		sprintf(buf, "\t\t; *** UNKNOWN OPCODE ***");
	}
	if (extended)
		gdg->pc += opc_ext->size;
	else
		gdg->pc += opc->size & ~P_EXT;

	return gdg->buffer;
}

bool gd_dis_file(gd_globals_t *gdg, char *name, FILE *output)
{
	FILE *in;
	uint32 size;

	in = fopen(name, "rb");
	if (in == NULL)
		return false;
	fseek(in, 0, SEEK_END);
	size = ftell(in);
	fseek(in, 0, SEEK_SET);
	gdg->binbuf = (uint16 *)malloc(size);
	fread(gdg->binbuf, 1, size, in);

	gdg->buffer = (char *)malloc(256);
	gdg->buffer_size = 256;

	for (gdg->pc = 0 ; gdg->pc < (size/2) ;)
	{
		fprintf(output, "%s\n", gd_dis_opcode(gdg));
	}
	fclose(in);
	
	free(gdg->binbuf);
	gdg->binbuf = NULL;

	free(gdg->buffer);
	gdg->buffer = NULL;
	gdg->buffer_size = 0;

	return true;
}

void gd_dis_close_unkop(void)
{
	FILE *uo;
	int i, j;
	uint32 count = 0;

	uo = fopen("uo.bin", "wb");
	if (uo)
	{
		fwrite(unk_opcodes, 1, sizeof(unk_opcodes), uo);
		fclose(uo);
	}
	uo = fopen("unkopc.txt", "w");

	if (uo)
	{
		for(i = 0 ; i < 0x10000 ; i++)
		{
			if (unk_opcodes[i])
			{
				count++;
				fprintf(uo, "OP%04x\t%d", i, unk_opcodes[i]);
				for(j = 15 ; j >= 0 ; j--)
				{
					if ((j & 0x3) == 3)
						fprintf(uo, "\tb");
					fprintf(uo, "%d", (i >> j) & 0x1);
				}
				fprintf(uo, "\n");
			}
		}
		fprintf(uo, "Unknown opcodes count: %d\n", count);
		fclose(uo);
	}
}
void gd_dis_open_unkop(void)
{
	FILE *uo;

	uo = fopen("uo.bin", "rb");
	if (uo)
	{
		fread(unk_opcodes, 1, sizeof(unk_opcodes), uo);
		fclose(uo);
	}
	else
	{
		int i;
		for(i = 0 ; i < 0x10000 ; i++)
			unk_opcodes[i] = 0;
	}
}

void gd_dis_mem (gd_globals_t *gdg, uint16 *mem, int size)
{
	gdg->binbuf = mem;

	for (gdg->pc = 0; gdg->pc < size / 2;)
		printf ("%s\n", gd_dis_opcode (gdg));
}


void gd_dis_mem_fast (uint16 *mem, int size)
{
	gd_globals_t *gdg = gd_init ();

	gdg->decode_registers = false;
	gdg->decode_names = false;
	gdg->show_hex = true;
	gdg->show_pc = true;
	gdg->print_tabs = true;
	gdg->ext_separator = '\'';

	gdg->buffer = (char *) malloc (256);
	gdg->buffer_size = 256;
	
	gd_dis_mem (gdg, mem, size);

	free (gdg->buffer);
	gdg->buffer_size = 0;
}


int dsp_disassemble (char **buff, uint16 *mem)
{
	static int initialized = 0;
	static gd_globals_t *gdg;


	if (!initialized)
	{
		gdg = gd_init ();

		gdg->decode_registers = false;
		gdg->decode_names = false;
		gdg->show_hex = true;
		gdg->show_pc = false;
		gdg->print_tabs = true;
		gdg->ext_separator = '\'';

		gdg->buffer = (char *) malloc (256);
		gdg->buffer_size = 256;
		
		initialized = 1;

//	free (gdg->buffer);
//	gdg->buffer_size = 0;
	}
	
	gdg->binbuf = mem;
	gdg->pc = 0;
	*buff = gd_dis_opcode (gdg);
	
	return gdg->isize;
}

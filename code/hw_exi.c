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
 *  EXI - external interface (0xcc006800)
 *      
 *         
 */


#include "hw_exi.h"

#include "font_sjis.c"
#include "font_ansi.c"


__u8 rexi[REXI_SIZE];

__u8 fonts[0x50000];
__u8 uart[0x100];
SRAMS sram;
Memcard memc[2];


int imm_mx_mode = 0;
// sram offset currently written to might be kept in madr
int sram_ofs = 0;

void (*dma_transfer [64]) (__u32 address, __u32 length, __u32 offset);
void (*imm_transfer [64]) (int size, __u32 data);
void exi_dma_transfer (int channel, int device, int write, __u32 data, __u32 length, __u32 address);
void exi_imm_transfer (int channel, int device, int write, int size, __u32 data);


void dma_hook (int write, int channel, int device, void (* hook)(__u32, __u32, __u32))
{
	dma_transfer[(write << 5) | (channel << 2) | device] = hook;
}


void dma_hook_r (int channel, int device, void (* hook)(__u32, __u32, __u32))
{
	dma_hook (0, channel, device, hook);
}


void dma_hook_w (int channel, int device, void (* hook)(__u32, __u32, __u32))
{
	dma_hook (1, channel, device, hook);
}


void imm_hook (int write, int channel, int device, void (* hook)(int, __u32))
{
	imm_transfer[(write << 5) | (channel << 2) | device] = hook;
}


void imm_hook_r (int channel, int device, void (* hook)(int, __u32))
{
	imm_hook (0, channel, device, hook);
}


void imm_hook_w (int channel, int device, void (* hook)(int, __u32))
{
	imm_hook (1, channel, device, hook);
}


void dma_transfer_unknown (__u32 address, __u32 length, __u32 offset)
{
	DEBUG (EVENT_LOG_EXI, ".exi: unknown dma transfer ADDRESS %.8x LENGTH %.8x OFFSET %.8x",
					address, length, offset);
}


void imm_transfer_unknown (int size, __u32 data)
{
	DEBUG (EVENT_LOG_EXI, ".exi: unknown imm transfer SIZE %d DATA %.8x",
					size, data);
}


void read_to_u32 (__u32 *dst, __u8 *src, int size)
{
	*dst = 0;
	if (size >= 1)
		*dst |= *src++ << 24;
	if (size >= 2)
		*dst |= *src++ << 16;
	if (size >= 3)
		*dst |= *src++ <<  8;
	if (size >= 4)
		*dst |= *src++;
}


void mc_calc_checksum (int n, int start, int len, int ofs1, int ofs2)
{
	__u16 check1 = 0, check2 = 0;


	len /= 2;
	while (len--)
	{
		check1 += MEMC_R16 (n, start);
		check2 += ~MEMC_R16 (n, start);
		start += 2;
	}
	
	if (check1 == 0xffff)
		check1 = 0;
	if (check2 == 0xffff)
		check2 = 0;
	
	MEMC_W16 (n, ofs1, check1);
	MEMC_W16 (n, ofs2, check2);
}


void mc_format (int n)
{
	if (!memc[n].gmc)
	{
		DEBUG (EVENT_LOG_MC, "..mc: format failed, card in slot %d not opened", n);
		return;
	}

	memset (memc[n].gmc, 0, MC_BLOCK_SIZE * 5);

	// font encoding should be handled too

	memc[n].gmc->hdr.size = BSWAP16 (memc[n].size);

	MEMC_W32 (n, 0x00, 0x3bdc5da6); // checksum word 1
	MEMC_W32 (n, 0x04, 0x47d565fc); // checksum word 2
	MEMC_W32 (n, 0x08, 0x949bbc90); // checksum word 3

#if BREAK_MC_INIT
	MEMC_W32 (n, 0x10, 0x00ceb2f8); // format time lo
#endif

	memset (&MEMC (n, 0x0026), 0xff, 0x01fc - 0x0026); // unused
	// word at 0x0200 is used as offset in reads further than 0x0200 (?)
	MEMC_W32 (n, 0x0200, 0xffffffff);
	memset (&MEMC (n, 0x0204), 0xff, 0x2000 - 0x0204); // unused

	mc_calc_checksum (n, 0, 0x01fc, 0x01fc, 0x01fe);

	memset (&MEMC (n, 0x2000), 0xff, 0x1ffa); // unused
	memset (&MEMC (n, 0x4000), 0xff, 0x1ffa); // unused

	mc_calc_checksum (n, 0x2000, 0x1ffc, 0x2000 + 0x1ffc, 0x2000 + 0x1ffe);
	mc_calc_checksum (n, 0x4000, 0x1ffc, 0x4000 + 0x1ffc, 0x4000 + 0x1ffe);

	MEMC_W16 (n, 0x6006, memc[n].size * 16 - 5); // free blocks
	MEMC_W16 (n, 0x6008, 4); // last allocated block
	mc_calc_checksum (n, 0x6004, 0x1ffc, 0x6000, 0x6002);

	MEMC_W16 (n, 0x8006, memc[n].size * 16 - 5);
	MEMC_W16 (n, 0x8008, 4); // last allocated block
	mc_calc_checksum (n, 0x8004, 0x1ffc, 0x8000, 0x8002);
}


void mc_open (int n, int size)
{
	memset (&memc[n], 0, sizeof (Memcard));

	memc[n].status = MC_STATUS_READY;
	memc[n].hid = MC_HID;
	memc[n].size = size;
	memc[n].gmc = (GMC *) malloc (MC_BLOCK_SIZE * memc[n].size * 16);

	mc_format (n);
}


void mc_close (int n)
{
	if (memc[n].gmc)
		free (memc[n].gmc);
}


int mc_save (int n, const char *filename)
{
	FILE *f;


	if (!memc[n].gmc)
	{
		DEBUG (EVENT_LOG_MC, "..mc: save failed, card in slot %d not opened", n);
		return FALSE;
	}

	f = fopen (filename, "w+");
	if (!f)
	{
		DEBUG (EVENT_LOG_MC, "..mc: saving card %d to %s failed", n, filename);
		return FALSE;
	}
	
	DEBUG (EVENT_LOG_MC, "..mc: saving card %d to %s", n, filename);

	fwrite (&MEMC (n, 0), MEMC_SIZE (n), 1, f);
	fclose (f);

	return TRUE;
}


int mc_load (int n, const char *filename)
{
	FILE *f;


	if (!memc[n].gmc)
	{
		DEBUG (EVENT_LOG_MC, "..mc: load failed, card in slot %d not opened", n);
		return FALSE;
	}

	f = fopen (filename, "r");
	if (!f)
	{
		DEBUG (EVENT_LOG_MC, "..mc: loading %s into slot %d failed", filename, n);
		return FALSE;
	}

	DEBUG (EVENT_LOG_MC, "..mc: loading %s into slot %d, size %d", filename, n, memc[n].size);
	fread (&MEMC (n, 0), MEMC_SIZE (n), 1, f);
	fclose (f);

	MEMC_W32 (n, 0x0c, 0); MEMC_W32 (n, 0x10, 0); // format time

	return TRUE;
}


void mc_init (void)
{
	char buff[1024];


	sprintf (buff, "%s/%s", get_home_dir (), config.memcard_a_name);
	mc_open (0, config.memcard_a_size);
	mc_load (0, buff);

	sprintf (buff, "%s/%s", get_home_dir (), config.memcard_b_name);
	mc_open (1, config.memcard_b_size);
	mc_load (1, buff);
}


void mc_cleanup (void)
{
	char buff[1024];


	sprintf (buff, "%s/%s", get_home_dir (), config.memcard_a_name);
	mc_save (0, buff);
	mc_close (0);

	sprintf (buff, "%s/%s", get_home_dir (), config.memcard_b_name);
	mc_save (1, buff);
	mc_close (1);
}


// simple one for now, just place it as the first file
int mc_import_file (int n, const char *filename)
{
	// fst at 0x2000 and 0x4000 + 0x40 * position
	// data to 0xa000 + offset
	FILE *f;
	GMCDirEntry item;
	int i, size;


	f = fopen (filename, "r");
	if (!f)
		return FALSE;

	fread (&item, sizeof (item), 1, f);

	size = BSWAP16 (item.size);
	item.firstblock = BSWAP16 (5);

	memcpy (&MEMC (n, 0x2000), &item, 0x40);
	memcpy (&MEMC (n, 0x4000), &item, 0x40);
	mc_calc_checksum (n, 0x2000, 0x1ffc, 0x2000 + 0x1ffc, 0x2000 + 0x1ffe);
	mc_calc_checksum (n, 0x4000, 0x1ffc, 0x4000 + 0x1ffc, 0x4000 + 0x1ffe);

	// free blocks
	MEMC_W16 (n, 0x6006, memc[n].size * 16 - 5 - size);
	MEMC_W16 (n, 0x8006, memc[n].size * 16 - 5 - size);
	// last allocated block
	MEMC_W16 (n, 0x6008, BSWAP16 (item.firstblock) + size - 1);
	MEMC_W16 (n, 0x8008, BSWAP16 (item.firstblock) + size - 1);
	for (i = 0; i < size - 1; i++)
	{
		MEMC_W16 (n, 0x600a + i*2, BSWAP16 (item.firstblock) + i + 1);
		MEMC_W16 (n, 0x800a + i*2, BSWAP16 (item.firstblock) + i + 1);
	}

	MEMC_W16 (n, 0x600a + i*2, 0xffff);
	MEMC_W16 (n, 0x800a + i*2, 0xffff);

	mc_calc_checksum (n, 0x6004, 0x1ffc, 0x6000, 0x6002);
	mc_calc_checksum (n, 0x8004, 0x1ffc, 0x8000, 0x8002);

	fread (&MEMC (n, 0xa000), MC_BLOCK_SIZE, BSWAP16 (item.size), f);
	fclose (f);

	return TRUE;
}


void mc_attach (int n)
{
	exi_attach (n);
}


void mc_fix_id (void)
{
	// flash checksum needed too?
	memcpy (&MEMC (0, 0), &sram.flashid[0], 12);
	mc_calc_checksum (0, 0, 0x01fc, 0x01fc, 0x01fe);
	memcpy (&MEMC (1, 0), &sram.flashid[3], 12);
	mc_calc_checksum (1, 0, 0x01fc, 0x01fc, 0x01fe);

	DEBUG (EVENT_LOG_MC, "..mc: fixed ids");
}


void mc_execute (int n)
{
	switch (memc[n].cmd[0])
	{
		case MC_CMD_ERASEBLOCK:
			{
				__u32 bl = (memc[n].cmd[1] << 8) | memc[n].cmd[2];

				DEBUG (EVENT_LOG_MC, "..mc: CMD_ERASEBLOCK (%.4x)", bl);
				if ((MEMC_CMD_OFFS (n) + MC_BLOCK_SIZE) < MEMC_SIZE (n))
					memset (&MEMC (n, bl << 9), MC_ERASE_BYTE, MC_BLOCK_SIZE);
				exi_generate_interrupt (EXI_INTERRUPT_EXI, n);
			}
			break;

		case MC_CMD_WRITEBLOCK:
			if (memc[n].cmdsize < 5)
			{
				memc[n].bleft = 1;
				return; // cmdsize must be preserved
			}
			else
			{
				// next command should be dma write of 0x80 bytes
				// writing block using imm mode is not supported
				DEBUG (EVENT_LOG_MC, "..mc: CMD_WRITEBLOCK (offset %.8x)", MEMC_CMD_OFFS (n));
				exi_generate_interrupt (EXI_INTERRUPT_EXI, n);
			}
			break;

		case MC_CMD_INTENABLE:
			DEBUG (EVENT_LOG_MC, "..mc: CMD_INTENABLE (%d)", memc[n].cmd[1]);
			break;

		case MC_CMD_CLEARSTATUS:
		// clear some status bits
			DEBUG (EVENT_LOG_MC, "..mc: CMD_CLEARSTATUS");
			break;

		// action on mc_read

		case MC_CMD_READBLOCK:
			if (memc[n].cmdsize < 9)
			{
				memc[n].bleft = 5;
				return; // cmdsize must be preserved
			}
			break;

		case MC_CMD_GETEXIID:
		case MC_CMD_GETSTATUS:
		case MC_CMD_READID:
			break;

		default:
			DEBUG (EVENT_STOP, "..mc: unknown write 0x%.2x", memc[n].cmd[0]);
			break;
	}

	memc[n].cmdsize = 0; // command finished
	memc[n].rofs = 0;
}


void mc_write (int n, int size, __u32 data)
{
	DEBUG (EVENT_LOG_MC, "..mc: write %.8x (%d)", data, size);

	if (memc[n].bleft)
		memc[n].bleft -= size;

	while (size--)
	{
		memc[n].cmd[memc[n].cmdsize++] = data >> 24;
		data <<= 8;
	}

	if (!memc[n].bleft)
		mc_execute (n);
}


void mc_read (int n, int size, __u32 data)
{ 
	switch (memc[n].cmd[0])
	{
		case MC_CMD_GETEXIID:
			REXI_DATA (n) = memc[n].size;
			DEBUG (EVENT_LOG_MC, "..mc: CMD_GETEXIID (%d)", memc[n].size);
			break;

		case MC_CMD_GETSTATUS:
			REXI_DATA (n) = (memc[n].status << 24);
			DEBUG (EVENT_LOG_MC, "..mc: CMD_GETSTATUS (%.8x)", REXI_DATA (n));
			break;

		case MC_CMD_READBLOCK:
			// byte is 7 bits
			// page is 3 bits
			// sector is 16 bits
			DEBUG (EVENT_LOG_MC, "..mc: CMD_READBLOCK (sector %.4x page %.2x byte %.2x) ofs %.8x size %d",
						 (memc[n].cmd[1] << 8) | memc[n].cmd[2], memc[n].cmd[3], memc[n].cmd[4],
							MEMC_CMD_OFFS (n), size);
			if ((MEMC_CMD_OFFS (n) + size) < MEMC_SIZE (n))
				read_to_u32 (&REXI_DATA (n), &MEMC (n, MEMC_CMD_OFFS (n)), size);
			memc[n].status |= 0x40;
			break;

		case MC_CMD_READID:
			REXI_DATA (n) = memc[n].hid << 16;
			DEBUG (EVENT_LOG_MC, "..mc: CMD_READID (%.4x)", memc[n].hid);
			break;

		default:
			DEBUG (EVENT_STOP, "..mc: unknown read 0x%.2x", memc[n].cmd[0]);
	}

	memc[n].rofs += size;
}


void mc_dma_read (int n, __u32 address, __u32 length, __u32 offset)
{
//	if (offset)
//		DEBUG (EVENT_STOP, "mc dma read offset non zero (%.8x)", offset);

	if ((MEMC_CMD_OFFS (n) + length) >= MEMC_SIZE (n))
		DEBUG (EVENT_STOP, "mc dma read out of bounds");

	DEBUG (EVENT_LOG_MC, "..mc: dma MC%d[%.8x] -> MEM[%.8x] %.8x bytes",
				n, MEMC_CMD_OFFS (n), address | 0x80000000, length);

	memcpy (MEM_ADDRESS (address), &MEMC (n, MEMC_CMD_OFFS (n)), length);
}


void mc_dma_write (int n, __u32 address, __u32 length, __u32 offset)
{
//	if (offset)
//		DEBUG (EVENT_STOP, "mc dma write offset non zero (%.8x)", offset);

	if ((MEMC_CMD_OFFS (n) + length) >= MEMC_SIZE (n))
		DEBUG (EVENT_STOP, "mc dma write out of bounds");

	DEBUG (EVENT_LOG_MC, "..mc: dma MEM[%.8x] -> MC%d[%.8x] %.8x bytes",
				address | 0x80000000, n, MEMC_CMD_OFFS (n), length);

	memcpy (&MEMC (n, MEMC_CMD_OFFS (n)), MEM_ADDRESS (address), length);
}


void dma_mca_read (__u32 address, __u32 length, __u32 offset)
{
	mc_dma_read (0, address, length, offset);
}


void dma_mca_write (__u32 address, __u32 length, __u32 offset)
{
	mc_dma_write (0, address, length, offset);
}


void imm_mca_read (int size, __u32 data)
{ 
	mc_read (0, size, data);
}


void imm_mca_write (int size, __u32 data)
{
	mc_write (0, size, data);
}


void dma_mcb_read (__u32 address, __u32 length, __u32 offset)
{
	mc_dma_read (1, address, length, offset);
}


void dma_mcb_write (__u32 address, __u32 length, __u32 offset)
{
	mc_dma_write (1, address, length, offset);
}


void imm_mcb_read (int size, __u32 data)
{ 
	mc_read (1, size, data);
}


void imm_mcb_write (int size, __u32 data)
{
	mc_write (1, size, data);
}


void dma_mx_read (__u32 address, __u32 length, __u32 offset)
{
	if (offset == EXI_OFS_SRAM)
	{
		DEBUG (EVENT_LOG_EXI, "....  dma SRAM[%.2x] -> MEM[%.8x] %d bytes",
					 SRAM_OFFSET (offset), address | 0x80000000, length);
		if ((SRAM_OFFSET (offset) + length) > EXI_SRAM_SIZE)
			DEBUG (EVENT_STOP, "sram read out of bounds");
		memcpy (MEM_ADDRESS (address), SRAM_ADDRESS (SRAM_OFFSET (offset)), length);
	}
	else if ((offset >= ROM_FONTS_START) && (offset < ROM_FONTS_END))
	{
		__u32 p = (offset - ROM_FONTS_START) >> 6;

		DEBUG (EVENT_LOG_EXI, "....  FONT read [%.8x] -> MEM[%.8x]", p, address | 0x80000000);
		memcpy (MEM_ADDRESS (address), &fonts[p], length);
	}
	else
		DEBUG (EVENT_STOP, "dma mx reading uknown %.8x", offset);
}


void dma_mx_write (__u32 address, __u32 length, __u32 offset)
{
	DEBUG (EVENT_STOP, "dma mx write not supported");
}


void imm_mx_read (int size, __u32 data)
{
	if (data == EXI_OFS_RTC)
	{
		DEBUG (EVENT_STOP, "RTC read not supported");
		REXI32 (EXI_0DATA) = 0;
	}
	else if ((data >= EXI_OFS_SRAM) && (data < (EXI_OFS_SRAM + (EXI_SRAM_SIZE << 6))))
	{
		DEBUG (EVENT_STOP, "SRAM imm read not supported");
	}
	else if (data == EXI_OFS_UART)
	{
		DEBUG (EVENT_LOG_EXI, "....  UART read %d bytes", size);

		// 0 to 4
		REXI32 (EXI_0DATA) = 0x03000000;
	}
	else
		DEBUG (EVENT_STOP, "mx imm read unknown");
}


void imm_mx_write (int size, __u32 data)
{
	switch (imm_mx_mode)
	{
		case MX_UART_WRITE:
			{
				__u32 s = BSWAP32 (data);

				strncat ((char *) uart, (char *) &s, size);
				DEBUG (EVENT_LOG_EXI, "....  UART write %d bytes", size);
			}
			break;
		
		case MX_SRAM_WRITE:
			DEBUG (EVENT_LOG_EXI, "....  SRAM[%.2x] = %.8x (%d bytes)", sram_ofs, data, size);
			while (size--)
			{
				SRAM (sram_ofs++) = data >> 24;
				data <<= 8;
			}
			// update flash id on memcards
			if ((sram_ofs >= 0x14) && (sram_ofs <= 0x2c))
				mc_fix_id ();
			break;
		
		default:
			{
				if ((data >= (EXI_OFS_SRAM | EXI_CMD_WRITE))
					&& (data < ((EXI_OFS_SRAM + (EXI_SRAM_SIZE << 6)) | EXI_CMD_WRITE)))
				{
					imm_mx_mode = MX_SRAM_WRITE;
					sram_ofs = ((data - 0x100) >> 6) & 0x7f;
				}
				else if (data == (EXI_OFS_UART | EXI_CMD_WRITE))
					imm_mx_mode = MX_UART_WRITE;
				else if ((data > EXI_OFS_RTC) &&
								 (data != EXI_OFS_UART) && (data != EXI_OFS_SRAM))
					DEBUG (EVENT_STOP, "mx imm write unknown");
			}
	}
}


void imm_ad16_read (int size, __u32 data)
{ 
	DEBUG (EVENT_STOP, "ad16 imm read unsupported");
}


void imm_ad16_write (int size, __u32 data)
{
	DEBUG (EVENT_STOP, "ad16 imm write unsupported");
}


void os_report (unsigned char *str)
{
	char buff[1024];
	const char *p = (const char *) str;
	unsigned int i;


	// every line ends with \r
	while (1)
	{
		if (*p == '\r')
		{
			gcube_os_report ("", TRUE);
			p++;
		}
		
		while (*p < 0)
		{
			// ignore control characters
			p += 2;
		}

		if (*p == '\r')
			p++;

		if ((1 != sscanf (p, "%[^\r]\r", buff)) ||
				(p[strlen ((const char *) buff)] != '\r'))
			break;

		p += strlen ((const char *) buff) + 1;
		
		// clean the buffer from control characters
		for (i = 0; i < strlen ((const char *) buff);)
		{
			if (buff[i] < 0)
				memmove (&buff[i], &buff[i+2], strlen ((const char *) &buff[i+2]) + 1);
			else
				i++;
		}
	
		if (*buff != '\r')
			gcube_os_report (buff, TRUE);
	}

	memmove (str, p, strlen (p) + 1);
}


void exi_select (int chn, int dev)
{
	DEBUG (EVENT_LOG_EXI, "...   EXISelect %d", dev);
}


void exi_deselect (int chn, int dev)
{
	DEBUG (EVENT_LOG_EXI, "...   EXIDeselect %d", dev);

	if (imm_mx_mode == MX_UART_WRITE)
	{
		os_report (uart);
		imm_mx_mode = 0;
	}
	else if (imm_mx_mode != 0)
		imm_mx_mode = 0;
}


void exi_dma_transfer (int channel, int device, int type, __u32 data, __u32 length, __u32 address)
{
	DEBUG (EVENT_LOG_EXI, "....  DMA %s CHN%d DEV%d DATA %.8x LENGTH %.8x ADDRESS %.8x",
				 type ? "WRITE" : "READ", channel, device, data, length, address);

	// device == 0 means nothing is attached

	dma_transfer [(type << 5) | (channel << 2) | device] (address, length, data);
}


void exi_imm_transfer (int channel, int device, int type, int size, __u32 data)
{
	DEBUG (EVENT_LOG_EXI, "....  IMM %s CHN%d DEV%d DATA %.8x SIZE %d",
				 type ? "WRITE" : "READ", channel, device, data, size);

	// device == 0 means nothing is attached

	imm_transfer [(type << 5) | (channel << 2) | device] (size, data);
}


// registers
__u32 exi_r32_direct (__u32 addr)
{
	DEBUG (EVENT_LOG_EXI, ".exi: read  [%.4x] (%.8x)", addr & 0xffff, REXI32 (addr));

	return REXI32 (addr);
}


void exi_w32_direct (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_EXI, ".exi: write [%.4x] (%.8x) = %.8x", addr & 0xffff, REXI32 (addr), data);

	REXI32 (addr) = data;
}


void exi_update_interrupts (int n)
{
	pi_interrupt (INTERRUPT_EXI,
			((REXI_CSR (0) & EXI_CSR_TCINT)  && (REXI_CSR (0) & EXI_CSR_TCINTMSK))  ||
			((REXI_CSR (0) & EXI_CSR_EXTINT) && (REXI_CSR (0) & EXI_CSR_EXTINTMSK)) ||
			((REXI_CSR (0) & EXI_CSR_EXIINT) && (REXI_CSR (0) & EXI_CSR_EXIINTMSK)) ||
			((REXI_CSR (1) & EXI_CSR_TCINT)  && (REXI_CSR (1) & EXI_CSR_TCINTMSK))  ||
			((REXI_CSR (1) & EXI_CSR_EXTINT) && (REXI_CSR (1) & EXI_CSR_EXTINTMSK)) ||
			((REXI_CSR (1) & EXI_CSR_EXIINT) && (REXI_CSR (1) & EXI_CSR_EXIINTMSK)) ||
			((REXI_CSR (2) & EXI_CSR_TCINT)  && (REXI_CSR (2) & EXI_CSR_TCINTMSK))  ||
			((REXI_CSR (2) & EXI_CSR_EXTINT) && (REXI_CSR (2) & EXI_CSR_EXTINTMSK)) ||
			((REXI_CSR (2) & EXI_CSR_EXIINT) && (REXI_CSR (2) & EXI_CSR_EXIINTMSK)));
}


void exi_generate_interrupt (__u32 mask, int channel)
{
	if (mask & EXI_INTERRUPT_TC)
		REXI_CSR (channel) |= EXI_CSR_TCINT;

	if (mask & EXI_INTERRUPT_EXT)
		REXI_CSR (channel) |= EXI_CSR_EXTINT;

	if (mask & EXI_INTERRUPT_EXI)
		REXI_CSR (channel) |= EXI_CSR_EXIINT;

	exi_update_interrupts (channel);
}


void exi_attach (int n)
{
//	REXI_CSR (n) |= EXI_CSR_EXT | EXI_CSR_EXTINT;
	REXI_CSR (n) |= EXI_CSR_EXT;
	exi_update_interrupts (n);
}


// control status register
void exi_w32_csr (__u32 addr, __u32 data)
{
	int chn = (addr & 0xff) / 0x14, dev = ((data >> 7) & 7) / 2;


	DEBUG (EVENT_LOG_EXI, ".exi: CSR%d | DEV %d%d%d | CLK %d | EXT %d | INT %d%d%d/%d%d%d ack %d%d%d",
				 chn,	(data >> 9) & 1, (data >> 8) & 1, (data >> 7) & 1, (data >> 4) & 7, (data >> 12) & 1,
				 (REXI32 (addr) & EXI_CSR_EXTINT)>0, (REXI32 (addr) & EXI_CSR_TCINT)>0, (REXI32 (addr) & EXI_CSR_EXIINT)>0,
				 (data & EXI_CSR_EXTINTMSK)>0, (data & EXI_CSR_TCINTMSK)>0, (data & EXI_CSR_EXIINTMSK)>0,
				 (data & EXI_CSR_EXTINT)>0, (data & EXI_CSR_TCINT)>0, (data & EXI_CSR_EXIINT)>0);

	if ((((data >> 7) & 7) == 0) && ((REXI_CSR (chn) >> 7) & 7))
		exi_deselect (chn, ((REXI_CSR (chn) >> 7) & 7) / 2);
	else if (((data >> 7) & 7) != (((REXI_CSR (chn) >> 7) & 7)))
		exi_select (chn, dev);

	// acknowledge interrupts
	REXI32 (addr) &= ~(data & (EXI_CSR_EXIINT | EXI_CSR_TCINT | EXI_CSR_EXTINT));

	REXI32 (addr) = (REXI32 (addr) & (EXI_CSR_EXIINT | EXI_CSR_TCINT | EXI_CSR_EXTINT | EXI_CSR_EXT)) |
									(data & ~(EXI_CSR_EXIINT | EXI_CSR_TCINT | EXI_CSR_EXTINT | EXI_CSR_EXT));

	exi_update_interrupts (chn);
}


void exi_w32_cr (__u32 addr, __u32 data)
{
	int chn = ((addr & 0xff) - 0x0c) / 0x14, dev = ((REXI_CSR (chn) >> 7) & 7) / 2;
	int type = (data >> 2) & 3;


	DEBUG (EVENT_LOG_EXI, ".exi: CR%d | TLEN %d | RW %d | DMA %d | START %d",
				 chn, (data >> 4) & 3, (data >> 2) & 3, (data >> 1) & 1, data & 1);

	REXI32 (addr) = data;

	if (data & EXI_CR_TSTART)
	{
		// do transfer

		// operation:
		//  0 - read
		//  1 - write
		//  2 - read/write (invalid for dma)
		if (REXI32 (addr) & EXI_CR_DMA)
		{
			// dma transfer

			if (type == 2)
				DEBUG (EVENT_EMINOR, ".exi: dma read/write operation invalid");
			else
				exi_dma_transfer (chn, dev, type,
													REXI32 (addr + 4), REXI32 (addr - 4), REXI32 (addr - 8));
		}
		else
		{
			// immediate transfer
			if (type == 2)
				DEBUG (EVENT_STOP, "exi imm read/write operation unsupported");

			exi_imm_transfer (chn, dev, type,
												((data >> 4) & 3) + 1, REXI32 (addr + 4));
		}	

		REXI32 (addr) &= ~EXI_CR_TSTART;

		// csr, transfer complete interrupt
		REXI32 (addr - 0x0c) |= EXI_CSR_TCINT;
	}

	exi_update_interrupts (chn);
}


void exi_reinit (void)
{
}


void exi_init (void)
{
	int i;


	memset (rexi, 0, sizeof (rexi));
	memset (uart, 0, sizeof (uart));
	memset (&sram, 0, sizeof (sram));

	mem_hwr_hook (32, EXI_0CSR, exi_r32_direct);
	mem_hww_hook (32, EXI_0CSR, exi_w32_csr);
	mem_hwr_hook (32, EXI_0MAR, exi_r32_direct);
	mem_hww_hook (32, EXI_0MAR, exi_w32_direct);
	mem_hwr_hook (32, EXI_0LEN, exi_r32_direct);
	mem_hww_hook (32, EXI_0LEN, exi_w32_direct);
	mem_hww_hook (32, EXI_0CR, exi_w32_cr);
	mem_hwr_hook (32, EXI_0CR, exi_r32_direct);
	mem_hwr_hook (32, EXI_0DATA, exi_r32_direct);
	mem_hww_hook (32, EXI_0DATA, exi_w32_direct);

	mem_hwr_hook (32, EXI_1CSR, exi_r32_direct);
	mem_hww_hook (32, EXI_1CSR, exi_w32_csr);
	mem_hwr_hook (32, EXI_1MAR, exi_r32_direct);
	mem_hww_hook (32, EXI_1MAR, exi_w32_direct);
	mem_hwr_hook (32, EXI_1LEN, exi_r32_direct);
	mem_hww_hook (32, EXI_1LEN, exi_w32_direct);
	mem_hww_hook (32, EXI_1CR, exi_w32_cr);
	mem_hwr_hook (32, EXI_1CR, exi_r32_direct);
	mem_hwr_hook (32, EXI_1DATA, exi_r32_direct);
	mem_hww_hook (32, EXI_1DATA, exi_w32_direct);

	mem_hwr_hook (32, EXI_2CSR, exi_r32_direct);
	mem_hww_hook (32, EXI_2CSR, exi_w32_csr);
	mem_hwr_hook (32, EXI_2MAR, exi_r32_direct);
	mem_hww_hook (32, EXI_2MAR, exi_w32_direct);
	mem_hwr_hook (32, EXI_2LEN, exi_r32_direct);
	mem_hww_hook (32, EXI_2LEN, exi_w32_direct);
	mem_hww_hook (32, EXI_2CR, exi_w32_cr);
	mem_hwr_hook (32, EXI_2CR, exi_r32_direct);
	mem_hwr_hook (32, EXI_2DATA, exi_r32_direct);
	mem_hww_hook (32, EXI_2DATA, exi_w32_direct);

	memcpy (fonts, font_sjis, sizeof (font_sjis));
	memcpy (&fonts[0x4d000], font_ansi, sizeof (font_ansi));

	for (i = 0; i < 64; i++)
	{
		dma_transfer[i] = dma_transfer_unknown;
		imm_transfer[i] = imm_transfer_unknown;
	}
	
	dma_hook_r (0, 1, dma_mx_read);
	dma_hook_w (0, 1, dma_mx_write);
	imm_hook_r (0, 1, imm_mx_read);
	imm_hook_w (0, 1, imm_mx_write);

	dma_hook_r (0, 0, dma_mca_read);
	dma_hook_w (0, 0, dma_mca_write);
	imm_hook_r (0, 0, imm_mca_read);
	imm_hook_w (0, 0, imm_mca_write);

	dma_hook_r (1, 0, dma_mcb_read);
	dma_hook_w (1, 0, dma_mcb_write);
	imm_hook_r (1, 0, imm_mcb_read);
	imm_hook_w (1, 0, imm_mcb_write);

/*
	imm_hook_r (2, 0, imm_ad16_read);
	imm_hook_w (2, 0, imm_ad16_write);
*/
}

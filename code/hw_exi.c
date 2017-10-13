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

#define MX_UART_WRITE			1
int imm_mx_mode = 0;


void (*dma_transfer [64]) (__u32 address, __u32 length, __u32 offset);
void (*imm_transfer [64]) (int size, __u32 data);
void exi_dma_transfer (int channel, int device, int write, __u32 data, __u32 length, __u32 address);
void exi_imm_transfer (int channel, int device, int write, int size, __u32 data);


void dma_hook (int write, int channel, int device, void *hook)
{
	dma_transfer[(write << 5) | (channel << 2) | device] = hook;
}


void dma_hook_r (int channel, int device, void *hook)
{
	dma_hook (0, channel, device, hook);
}


void dma_hook_w (int channel, int device, void *hook)
{
	dma_hook (1, channel, device, hook);
}


void imm_hook (int write, int channel, int device, void *hook)
{
	imm_transfer[(write << 5) | (channel << 2) | device] = hook;
}


void imm_hook_r (int channel, int device, void *hook)
{
	imm_hook (0, channel, device, hook);
}


void imm_hook_w (int channel, int device, void *hook)
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


void dma_mx_read (__u32 address, __u32 length, __u32 offset)
{
	if (offset == 0x20000100)
		DEBUG (EVENT_LOG_EXI, ".... SRAM read -> MEM[%.8x]", address | 0x80000000);
	// fonts are from 0x1aff00 to 0x1fff00 (<<6)
	else if (offset >= (0x1aff00 << 6) && offset < (0x1fff00 << 6))
	{
		__u32 p = (offset >> 6) - 0x1aff00;

		DEBUG (EVENT_LOG_EXI, ".... FONT read [%.8x] -> MEM[%.8x]", p, address | 0x80000000);
		memcpy (MEM_ADDRESS (address), &fonts[p], length);
	}
}


void imm_mx_read (int size, __u32 data)
{
	// UART read
	if (data == 0x20010000)
	{
		DEBUG (EVENT_LOG_EXI, ".... UART read %d bytes", size);

		// 0 to 4
		REXI32 (EXI_0DATA) = 0x03000000;
	}
}


void imm_mx_write (int size, __u32 data)
{
	// UART write
	if (data == 0xa0010000)
		imm_mx_mode = MX_UART_WRITE;
	else if (imm_mx_mode == MX_UART_WRITE)
	{
		__u32 s = BSWAP32 (data);

		strncat (uart, (char *) &s, size);

		DEBUG (EVENT_LOG_EXI, ".... UART write %d bytes", size);
	}
}


void os_report (unsigned char *str)
{
	unsigned char buff[1024], *p = str;
	int i;


	// every line ends with \r
	while (1)
	{
		if (*p == '\r')
		{
			gcube_os_report ("", TRUE);
			p++;
		}
		
		while (*p > 0x7f)
		{
			// ignore control characters
			p += 2;
		}

		if (*p == '\r')
			p++;

		if ((1 != sscanf (p, "%[^\r]\r", buff)) ||
				(p[strlen (buff)] != '\r'))
			break;

		p += strlen (buff) + 1;
		
		// clean the buffer from control characters
		for (i = 0; i < strlen (buff);)
		{
			if (buff[i] > 0x7f)
				memmove (&buff[i], &buff[i+2], strlen (&buff[i+2]) + 1);
			else
				i++;
		}
	
		if (*buff != '\r')
			gcube_os_report (buff, TRUE);
	}

	memmove (str, p, strlen (p) + 1);
}


void exi_select (int dev)
{
	if (dev)
		DEBUG (EVENT_LOG_EXI, "...  EXISelect %d", dev);
	else
	{
		DEBUG (EVENT_LOG_EXI, "...  EXIDeselect %d", dev);

		if (imm_mx_mode == MX_UART_WRITE)
		{
			os_report (uart);
			imm_mx_mode = 0;
		}
	}
}


void exi_dma_transfer (int channel, int device, int write, __u32 data, __u32 length, __u32 address)
{
	DEBUG (EVENT_LOG_EXI, "....    DMA %s C%d D%d DATA %.8x LENGTH %.8x ADDRESS %.8x",
				 write ? "WRITE" : "READ", channel, device/2, data, length, address);

	// device == 0 means nothing is attached

	dma_transfer [(write << 5) | (channel << 2) | (device/2)] (address, length, data);
}


void exi_imm_transfer (int channel, int device, int write, int size, __u32 data)
{
	DEBUG (EVENT_LOG_EXI, "....    IMM %s C%d D%d DATA %.8x SIZE %d",
				 write ? "WRITE" : "READ", channel, device/2, data, size);

	// device == 0 means nothing is attached

	imm_transfer [(write << 5) | (channel << 2) | (device/2)] (size, data);
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


void exi_update_interrupts (int channel)
{
	__u32 addr = channel * 20 + 0x0c;


	pi_interrupt (INTERRUPT_EXI,
			((REXI32 (addr) & EXI_CSR_TCINT) && (REXI32 (addr) & EXI_CSR_TCINTMSK)) ||
			((REXI32 (addr) & EXI_CSR_EXTINT) && (REXI32 (addr) & EXI_CSR_EXTINTMSK)) ||
			((REXI32 (addr) & EXI_CSR_EXIINT) && (REXI32 (addr) & EXI_CSR_EXIINTMSK)));
}


void exi_generate_interrupt (__u32 mask, int channel)
{
	__u32 addr = channel * 20 + 0x0c;


	if (mask & EXI_INTERRUPT_TC)
		REXI32 (addr) |= EXI_CSR_TCINT;

	if (mask & EXI_INTERRUPT_EXT)
		REXI32 (addr) |= EXI_CSR_EXTINT;

	if (mask & EXI_INTERRUPT_EXI)
		REXI32 (addr) |= EXI_CSR_EXIINT;
	
	exi_update_interrupts (channel);
}


// control status register
void exi_w32_csr (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_EXI, ".exi: CSR%d | CLK %d%d%d | DEV %d%d%d | EXT %d | INT %d%d%d%d%d%d",
				 (addr & 0xff) / 0x14,	(data & 0x40)>0, (data & 0x20)>0, (data & 0x10)>0,
				 (data >> 9) & 1, (data >> 8) & 1, (data >> 7) & 1, (data >> 12) & 1,
				 (data & EXI_CSR_EXTINT)>0, (data & EXI_CSR_EXTINTMSK)>0, (data & EXI_CSR_TCINT)>0, (data & EXI_CSR_TCINTMSK)>0, (data & EXI_CSR_EXIINT)>0, (data & EXI_CSR_EXIINTMSK)>0);

	// acknowledge interrupts
	REXI32 (addr) &= ~(data & (EXI_CSR_EXIINT | EXI_CSR_TCINT | EXI_CSR_EXTINT));

	REXI32 (addr) = (REXI32 (addr) & (EXI_CSR_EXIINT | EXI_CSR_TCINT | EXI_CSR_EXTINT | EXI_CSR_EXT)) |
									(data & ~(EXI_CSR_EXIINT | EXI_CSR_TCINT | EXI_CSR_EXTINT | EXI_CSR_EXT));

	exi_select ((data >> 7) & 7);
	
	exi_update_interrupts ((addr & 0xff)/20);
}


void exi_w32_cr (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_EXI, ".exi: CR%d | TLEN %d%d | RW %d%d | DMA %d | START %d", (addr & 0xff) / 0x14,
				 (data >> 5) & 1, (data >> 4) & 1, (data >> 3) & 1, (data >> 2) & 1, (data >> 1) & 1, data & 1);

	REXI32 (addr) = data;

	if (data & EXI_CR_TSTART)
	{
		// do transfer
		DEBUG (EVENT_LOG_EXI, ".... EXI TRANSFER: C%d | DEV %d",
						((addr & 0xff) - 0x0c)/20, ((REXI32 (addr - 0x0c) >> 7) & 7)/2);

		if (REXI32 (addr) & 2)
		// dma transfer
		{
			exi_dma_transfer (((addr & 0xff) - 0x0c)/20, (REXI32 (addr - 0x0c) >> 7) & 7, (REXI32 (addr) & 4)>0,
												REXI32 (addr + 4), REXI32 (addr - 4), REXI32 (addr - 8));
		}
		else
		// immediate transfer
		{
			exi_imm_transfer (((addr & 0xff) - 0x0c)/20, (REXI32 (addr - 0x0c) >> 7) & 7, (REXI32 (addr) & 4)>0,
												((REXI32 (addr) >> 4) & 3) + 1, REXI32 (addr + 4));
		}
	
		REXI32 (addr) &= ~EXI_CR_TSTART;

		// csr, transfer complete interrupt
		REXI32 (addr - 0x0c) |= EXI_CSR_TCINT;
	}

	exi_update_interrupts (((addr & 0xff) - 0x0c)/20);
}


void exi_reinit (void)
{
}


void exi_init (void)
{
	int i;


	memset (rexi, 0, sizeof (rexi));
	memset (uart, 0, sizeof (uart));

	mem_hwr_hook (32, EXI_0CSR, exi_r32_direct);
	mem_hww_hook (32, EXI_0CSR, exi_w32_csr);
	mem_hwr_hook (32, EXI_0MAR, exi_r32_direct);
	mem_hww_hook (32, EXI_0MAR, exi_w32_direct);
	mem_hwr_hook (32, EXI_0LENGTH, exi_r32_direct);
	mem_hww_hook (32, EXI_0LENGTH, exi_w32_direct);
	mem_hww_hook (32, EXI_0CR, exi_w32_cr);
	mem_hwr_hook (32, EXI_0CR, exi_r32_direct);
	mem_hwr_hook (32, EXI_0DATA, exi_r32_direct);
	mem_hww_hook (32, EXI_0DATA, exi_w32_direct);

	mem_hwr_hook (32, EXI_1CSR, exi_r32_direct);
	mem_hww_hook (32, EXI_1CSR, exi_w32_csr);
	mem_hwr_hook (32, EXI_1MAR, exi_r32_direct);
	mem_hww_hook (32, EXI_1MAR, exi_w32_direct);
	mem_hwr_hook (32, EXI_1LENGTH, exi_r32_direct);
	mem_hww_hook (32, EXI_1LENGTH, exi_w32_direct);
	mem_hww_hook (32, EXI_1CR, exi_w32_cr);
	mem_hwr_hook (32, EXI_1CR, exi_r32_direct);
	mem_hwr_hook (32, EXI_1DATA, exi_r32_direct);
	mem_hww_hook (32, EXI_1DATA, exi_w32_direct);

	mem_hwr_hook (32, EXI_2CSR, exi_r32_direct);
	mem_hww_hook (32, EXI_2CSR, exi_w32_csr);
	mem_hwr_hook (32, EXI_2MAR, exi_r32_direct);
	mem_hww_hook (32, EXI_2MAR, exi_w32_direct);
	mem_hwr_hook (32, EXI_2LENGTH, exi_r32_direct);
	mem_hww_hook (32, EXI_2LENGTH, exi_w32_direct);
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
	imm_hook_r (0, 1, imm_mx_read);
	imm_hook_w (0, 1, imm_mx_write);
}

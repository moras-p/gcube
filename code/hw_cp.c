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
 *  CP - commad processor (0xcc000000)
 *      
 *         
 */

#include "hw_cp.h"


__u8 rcp[RCP_SIZE];

__u32 wpar_address = 0;


__u16 cp_r16_direct (__u32 addr)
{
	DEBUG (EVENT_LOG_CP, "..cp: read  [%.4x] (%.4x)", addr & 0xffff, RCP16 (addr));
	return RCP16 (addr);
}


void cp_w16_direct (__u32 addr, __u16 data)
{
	DEBUG (EVENT_LOG_CP, "..cp: write [%.4x] (%.4x) = %.4x", addr & 0xffff, RCP16 (addr), data);
	RCP16 (addr) = data;
}


void cp_w8_wpar_cpu (__u32 addr, __u8 data)
{
	DEBUG (EVENT_LOG_CP, "..cp: (%.8x) [%.8x] = %2.2x", PC, PI_FIFO_WPOINTER | 0x80000000, data);

	MEMW (PI_FIFO_WPOINTER, data);
	PI_FIFO_WPOINTER++;
}


void cp_w16_wpar_cpu (__u32 addr, __u16 data)
{
	DEBUG (EVENT_LOG_CP, "..cp: (%.8x) [%.8x] = %4.4x", PC, PI_FIFO_WPOINTER | 0x80000000, data);

	MEMWR16 (PI_FIFO_WPOINTER, data);
	PI_FIFO_WPOINTER += 2;
}


void cp_w32_wpar_cpu (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_CP, "..cp: (%.8x) [%.8x] = %8.8x", PC, PI_FIFO_WPOINTER | 0x80000000, data);

	MEMWR32 (PI_FIFO_WPOINTER, data);
	PI_FIFO_WPOINTER += 4;
}


inline void cp_check_marks (void)
{
	if (CP_FIFO_WPOINTER >= CP_FIFO_END)
	{
		__u32 b;
		int n;

		b = CP_FIFO_WPOINTER - CP_FIFO_BASE;
		if (b > 32)
			b -= 32;
		n = gx_parse_list (CP_FIFO_BASE, b);
		if (n < 0)
			n = -n;
		b = CP_FIFO_WPOINTER - CP_FIFO_BASE - n;

		if (b)
			memmove (MEM_ADDRESS (CP_FIFO_BASE), MEM_ADDRESS (CP_FIFO_BASE + n), b);

		CP_FIFO_WPOINTER = CP_FIFO_BASE + b;
	}

	CP_FIFO_RW_DIST = CP_FIFO_WPOINTER - CP_FIFO_RPOINTER;
/*
	if (CP_FIFO_RW_DIST >= CP_FIFO_HI_WMARK)
	{
		CPSR |= CP_SR_OFINT;
		if (CPCR & CP_CR_OFINTMSK)
		{
			DEBUG (EVENT_LOG, "hi watermark interrupt");
			pi_interrupt (INTERRUPT_CP);
		}
	}	

	if (CP_FIFO_RW_DIST <= CP_FIFO_LO_WMARK)
	{
		CPSR |= CP_SR_UFINT;
		if (CPCR & CP_CR_UFINTMSK)
		{
			DEBUG (EVENT_LOG, "lo watermark interrupt");
			pi_interrupt (INTERRUPT_CP);
		}
	}	
*/
}


void cp_w8_wpar_gp (__u32 addr, __u8 data)
{
	DEBUG (EVENT_LOG_CP, "..cp: (%.8x) [%.8x] = %2.2x", PC, CP_FIFO_WPOINTER | 0x80000000, data);
	MEMW (CP_FIFO_WPOINTER, data);
	CP_FIFO_WPOINTER++;
	
	cp_check_marks ();
}


void cp_w16_wpar_gp (__u32 addr, __u16 data)
{
	DEBUG (EVENT_LOG_CP, "..cp: (%.8x) [%.8x] = %4.4x", PC, CP_FIFO_WPOINTER | 0x80000000, data);
	MEMWR16 (CP_FIFO_WPOINTER, data);
	CP_FIFO_WPOINTER += 2;

	cp_check_marks ();
}


void cp_w32_wpar_gp (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_CP, "..cp: (%.8x) [%.8x] = %8.8x", PC, CP_FIFO_WPOINTER | 0x80000000, data);
	MEMWR32 (CP_FIFO_WPOINTER, data);

	CP_FIFO_WPOINTER += 4;

	if ((MEMR16 (CP_FIFO_WPOINTER - 4) == 0x4700) &&
			(MEMR16 (CP_FIFO_WPOINTER - 9) == 0x4800))
	{
		gx_parse_list (CP_FIFO_BASE, CP_FIFO_WPOINTER - CP_FIFO_BASE);
		CP_FIFO_WPOINTER = CP_FIFO_BASE;
	}
	else if ((data == 0x45000002) && (MEM (CP_FIFO_WPOINTER - 5) == 0x61))
	{
		gx_parse_list (CP_FIFO_BASE, CP_FIFO_WPOINTER - CP_FIFO_BASE);
		CP_FIFO_WPOINTER = CP_FIFO_BASE;
	}

	cp_check_marks ();
}


void cp_attach_fifo_to_cpu (void)
{
	DEBUG (EVENT_LOG_CP, "..cp: FIFO attached to CPU, address %.8x", wpar_address | 0xc0000000);

	mem_hww_hook (8, wpar_address + 0, cp_w8_wpar_cpu);
	mem_hww_hook (8, wpar_address + 1, cp_w8_wpar_cpu);
	mem_hww_hook (8, wpar_address + 2, cp_w8_wpar_cpu);
	mem_hww_hook (8, wpar_address + 3, cp_w8_wpar_cpu);
	mem_hww_hook (16, wpar_address + 0, cp_w16_wpar_cpu);
	mem_hww_hook (16, wpar_address + 2, cp_w16_wpar_cpu);
	mem_hww_hook (32, wpar_address, cp_w32_wpar_cpu);

	mem_hww_hook (8, wpar_address + 4, cp_w8_wpar_cpu);
	mem_hww_hook (8, wpar_address + 5, cp_w8_wpar_cpu);
	mem_hww_hook (8, wpar_address + 6, cp_w8_wpar_cpu);
	mem_hww_hook (8, wpar_address + 7, cp_w8_wpar_cpu);
	mem_hww_hook (16, wpar_address + 4, cp_w16_wpar_cpu);
	mem_hww_hook (16, wpar_address + 6, cp_w16_wpar_cpu);
	mem_hww_hook (32, wpar_address + 4, cp_w32_wpar_cpu);
}


void cp_attach_fifo_to_gp (void)
{
	DEBUG (EVENT_LOG_CP, "..cp: FIFO attached to GP, address %.8x", wpar_address | 0xc0000000);

	mem_hww_hook (8, wpar_address + 0, cp_w8_wpar_gp);
	mem_hww_hook (8, wpar_address + 1, cp_w8_wpar_gp);
	mem_hww_hook (8, wpar_address + 2, cp_w8_wpar_gp);
	mem_hww_hook (8, wpar_address + 3, cp_w8_wpar_gp);
	mem_hww_hook (16, wpar_address + 0, cp_w16_wpar_gp);
	mem_hww_hook (16, wpar_address + 2, cp_w16_wpar_gp);
	mem_hww_hook (32, wpar_address, cp_w32_wpar_gp);

	mem_hww_hook (8, wpar_address + 4, cp_w8_wpar_gp);
	mem_hww_hook (8, wpar_address + 5, cp_w8_wpar_gp);
	mem_hww_hook (8, wpar_address + 6, cp_w8_wpar_gp);
	mem_hww_hook (8, wpar_address + 7, cp_w8_wpar_gp);
	mem_hww_hook (16, wpar_address + 4, cp_w16_wpar_gp);
	mem_hww_hook (16, wpar_address + 6, cp_w16_wpar_gp);
	mem_hww_hook (32, wpar_address + 4, cp_w32_wpar_gp);
}


void cp_w16_cr (__u32 addr, __u16 data)
{
	DEBUG (EVENT_LOG_CP, "..cp: CR %.4x", data);

	if ((data & CP_CR_GPLINK) && !(RCP16 (addr) & CP_CR_GPLINK))
		cp_attach_fifo_to_gp ();
	else if (!(data & CP_CR_GPLINK) && (RCP16 (addr) & CP_CR_GPLINK))
		cp_attach_fifo_to_cpu ();

	RCP16 (addr) = data &~ CP_CR_CLEAR_BP;

	if (data & CP_CR_CLEAR_BP)
		RCP16 (CP_SR) &= ~CP_SR_BPINT;
}


void cp_w16_sr (__u32 addr, __u16 data)
{
	DEBUG (EVENT_LOG_CP, "..cp: SR %.4x", data);

	CPSR = CP_SR_READY | CP_SR_IDLE;
	if (data & CP_SR_BPINT)
		DEBUG (EVENT_EFATAL, "....  CP BREAKPOINT");
}


void cp_w16_clear (__u32 addr, __u16 data)
{
	DEBUG (EVENT_LOG_CP, "..cp: clear FIFO UF %d | clear FIFO OF %d",
					(data & CP_CLEAR_UF)>0, data & CP_CLEAR_OF);

	if (data & CP_CLEAR_UF)
		RCP16 (CP_SR) &= ~CP_SR_UFINT;

	if (data & CP_CLEAR_OF)
		RCP16 (CP_SR) &= ~CP_SR_OFINT;
}


__u16 cp_r16_rwdist (__u32 addr)
{
	DEBUG (EVENT_LOG_CP, "..cp: read  [%.4x] (%.4x)", addr & 0xffff, RCP16 (addr));

	CP_FIFO_RW_DIST = CP_FIFO_WPOINTER - CP_FIFO_RPOINTER;

	return RCP16 (addr);
}


void cp_wpar_redirect (__u32 address)
{
	if (wpar_address != address)
	{
		wpar_address = address;

		if (CPCR & CP_CR_GPLINK)
			cp_attach_fifo_to_gp ();
		else
			cp_attach_fifo_to_cpu ();
	}
}


void cp_reinit (void)
{
	if (CPCR & CP_CR_GPLINK)
		cp_attach_fifo_to_gp ();
	else
		cp_attach_fifo_to_cpu ();
}


void cp_init (void)
{
	memset (rcp, 0, sizeof (rcp));

	mem_hwr_hook (16, CP_CR, cp_r16_direct);
	mem_hww_hook (16, CP_CR, cp_w16_cr);

	RCP16 (CP_SR) = CP_SR_READY | CP_SR_IDLE;
	mem_hwr_hook (16, CP_SR, cp_r16_direct);
	mem_hww_hook (16, CP_SR, cp_w16_sr);

	RCP16 (CP_BB_RIGHT) = 640;
	RCP16 (CP_BB_BOTTOM) = 480;
	
	mem_hwr_hook (16, CP_CLEAR, cp_r16_direct);
	mem_hwr_hook (16, CP_TOKEN, cp_r16_direct);
	mem_hwr_hook (16, CP_BB_LEFT, cp_r16_direct);
	mem_hwr_hook (16, CP_BB_RIGHT, cp_r16_direct);
	mem_hwr_hook (16, CP_BB_TOP, cp_r16_direct);
	mem_hwr_hook (16, CP_BB_BOTTOM, cp_r16_direct);
	mem_hwr_hook (16, CP_FIFO_BASE_LO, cp_r16_direct);
	mem_hwr_hook (16, CP_FIFO_BASE_HI, cp_r16_direct);
	mem_hwr_hook (16, CP_FIFO_END_LO, cp_r16_direct);
	mem_hwr_hook (16, CP_FIFO_END_HI, cp_r16_direct);
	mem_hwr_hook (16, CP_FIFO_HWMARK_LO, cp_r16_direct);
	mem_hwr_hook (16, CP_FIFO_HWMARK_HI, cp_r16_direct);
	mem_hwr_hook (16, CP_FIFO_LWMARK_LO, cp_r16_direct);
	mem_hwr_hook (16, CP_FIFO_LWMARK_HI, cp_r16_direct);
	mem_hwr_hook (16, CP_FIFO_RW_DIST_LO, cp_r16_rwdist);
	mem_hwr_hook (16, CP_FIFO_RW_DIST_HI, cp_r16_rwdist);
	mem_hwr_hook (16, CP_FIFO_WPOINTER_LO, cp_r16_direct);
	mem_hwr_hook (16, CP_FIFO_WPOINTER_HI, cp_r16_direct);
	mem_hwr_hook (16, CP_FIFO_RPOINTER_LO, cp_r16_direct);
	mem_hwr_hook (16, CP_FIFO_RPOINTER_HI, cp_r16_direct);
	mem_hwr_hook (16, CP_FIFO_BP_LO, cp_r16_direct);
	mem_hwr_hook (16, CP_FIFO_BP_HI, cp_r16_direct);

	mem_hww_hook (16, CP_CLEAR, cp_w16_clear);
	mem_hww_hook (16, CP_TOKEN, cp_w16_direct);
	mem_hww_hook (16, CP_BB_LEFT, cp_w16_direct);
	mem_hww_hook (16, CP_BB_RIGHT, cp_w16_direct);
	mem_hww_hook (16, CP_BB_TOP, cp_w16_direct);
	mem_hww_hook (16, CP_BB_BOTTOM, cp_w16_direct);
	mem_hww_hook (16, CP_FIFO_BASE_LO, cp_w16_direct);
	mem_hww_hook (16, CP_FIFO_BASE_HI, cp_w16_direct);
	mem_hww_hook (16, CP_FIFO_END_LO, cp_w16_direct);
	mem_hww_hook (16, CP_FIFO_END_HI, cp_w16_direct);
	mem_hww_hook (16, CP_FIFO_HWMARK_LO, cp_w16_direct);
	mem_hww_hook (16, CP_FIFO_HWMARK_HI, cp_w16_direct);
	mem_hww_hook (16, CP_FIFO_LWMARK_LO, cp_w16_direct);
	mem_hww_hook (16, CP_FIFO_LWMARK_HI, cp_w16_direct);
	mem_hww_hook (16, CP_FIFO_RW_DIST_LO, cp_w16_direct);
	mem_hww_hook (16, CP_FIFO_RW_DIST_HI, cp_w16_direct);
	mem_hww_hook (16, CP_FIFO_WPOINTER_LO, cp_w16_direct);
	mem_hww_hook (16, CP_FIFO_WPOINTER_HI, cp_w16_direct);
	mem_hww_hook (16, CP_FIFO_RPOINTER_LO, cp_w16_direct);
	mem_hww_hook (16, CP_FIFO_RPOINTER_HI, cp_w16_direct);
	mem_hww_hook (16, CP_FIFO_BP_LO, cp_w16_direct);
	mem_hww_hook (16, CP_FIFO_BP_HI, cp_w16_direct);

	CPCR |= CP_CR_GPLINK;
	cp_wpar_redirect (0x0c008000);

	// unknown
	mem_hwr_hook (16, 0x0040, cp_r16_direct);
	mem_hwr_hook (16, 0x0042, cp_r16_direct);
	mem_hwr_hook (16, 0x0044, cp_r16_direct);
	mem_hwr_hook (16, 0x0046, cp_r16_direct);
	mem_hwr_hook (16, 0x0048, cp_r16_direct);
	mem_hwr_hook (16, 0x004a, cp_r16_direct);
	mem_hwr_hook (16, 0x004c, cp_r16_direct);
	mem_hwr_hook (16, 0x004e, cp_r16_direct);
}

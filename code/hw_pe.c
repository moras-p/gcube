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
 *  PE - pixel engine (0xcc001000)
 *      
 *         
 */

#include "hw_pe.h"


__u8 rpe[RPE_SIZE];



__u16 pe_r16_direct (__u16 addr)
{
	DEBUG (EVENT_LOG_PE, "..pe: read  [%.4x] (%.4x)", addr & 0xffff, RPE16 (addr));
	return RPE16 (addr);
}


void pe_w16_direct (__u32 addr, __u16 data)
{
	DEBUG (EVENT_LOG_PE, "..pe: write [%.4x] (%.4x) = %.4x", addr & 0xffff, RPE16 (addr), data);
	RPE16 (addr) = data;
}


void pe_update_interrupts (void)
{
	pi_interrupt (INTERRUPT_PE_FINISH,
					(RPE16 (PE_INTSR) & PE_INTSR_INT_FINISH) &&
					(RPE16 (PE_INTSR) & PE_INTSR_MSK_FINISH));

	pi_interrupt (INTERRUPT_PE_TOKEN,
					(RPE16 (PE_INTSR) & PE_INTSR_INT_TOKEN) &&
					(RPE16 (PE_INTSR) & PE_INTSR_MSK_TOKEN));
}


void pe_generate_interrupt (__u32 mask)
{
	if (mask & PE_INTERRUPT_FINISH)
		RPE16 (PE_INTSR) |= PE_INTSR_INT_FINISH;

	if (mask & PE_INTERRUPT_TOKEN)
		RPE16 (PE_INTSR) |= PE_INTSR_INT_TOKEN;
	
	pe_update_interrupts ();
}


void pe_w16_intsr (__u32 addr, __u16 data)
{
	if (data & PE_INTSR_INT_FINISH)
		RPE16 (addr) &= ~PE_INTSR_INT_FINISH;

	if (data & PE_INTSR_INT_TOKEN)
		RPE16 (addr) &= ~PE_INTSR_INT_TOKEN;

	RPE16 (addr) = (RPE16 (addr) &~ (PE_INTSR_MSK_FINISH | PE_INTSR_MSK_TOKEN)) |
								 (data & (PE_INTSR_MSK_FINISH | PE_INTSR_MSK_TOKEN));

	pe_update_interrupts ();
}


void pe_set_token (__u16 token)
{
	DEBUG (EVENT_LOG_PE, "..pe: token %.4x", token);
	RPE16 (PE_TOKEN) = token;
}


void pe_reinit (void)
{
}


void pe_init (void)
{
	memset (rpe, 0, sizeof (rpe));
	
	mem_hwr_hook (16, PE_TOKEN, pe_r16_direct);
	mem_hww_hook (16, PE_TOKEN, pe_w16_direct);
	mem_hww_hook (16, PE_INTSR, pe_w16_intsr);
	mem_hwr_hook (16, PE_INTSR, pe_r16_direct);
	
	// z config
	mem_hwr_hook (16, 0x1000, pe_r16_direct);
	mem_hww_hook (16, 0x1000, pe_w16_direct);

	// alpha
	mem_hwr_hook (16, 0x1002, pe_r16_direct);
	mem_hww_hook (16, 0x1002, pe_w16_direct);
	mem_hwr_hook (16, 0x1004, pe_r16_direct);
	mem_hww_hook (16, 0x1004, pe_w16_direct);
	mem_hwr_hook (16, 0x1006, pe_r16_direct);
	mem_hww_hook (16, 0x1006, pe_w16_direct);
	mem_hwr_hook (16, 0x1008, pe_r16_direct);
	mem_hww_hook (16, 0x1008, pe_w16_direct);
}

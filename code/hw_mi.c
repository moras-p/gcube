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
 *  MI - memory interface (0xcc004000)
 *      
 *         
 */

#include "hw_mi.h"


__u8 rmi[RMI_SIZE];



__u16 mi_r16_direct (__u32 addr)
{
	DEBUG (EVENT_LOG_MI, "..mi: read  [%.4x] (%.4x)", addr & 0xffff, RMI16 (addr));
	return RMI16 (addr);
}


void mi_w16_direct (__u32 addr, __u16 data)
{
	DEBUG (EVENT_LOG_MI, "..mi: write [%.4x] (%.4x) = %.4x", addr & 0xffff, RMI16 (addr), data);
	RMI16 (addr) = data;
}


void mi_update_interrupts (void)
{
/*
	pi_interrupt (INTERRUPT_MEM,
			((RMI16 (MI_INT) & MI_INTMSK_MEM0) && (RMI16 (MI_INTMSK) & MI_INTMSK_MEM0)) ||
			((RMI16 (MI_INT) & MI_INTMSK_MEM1) && (RMI16 (MI_INTMSK) & MI_INTMSK_MEM1)) ||
			((RMI16 (MI_INT) & MI_INTMSK_MEM2) && (RMI16 (MI_INTMSK) & MI_INTMSK_MEM2)) ||
			((RMI16 (MI_INT) & MI_INTMSK_MEM3) && (RMI16 (MI_INTMSK) & MI_INTMSK_MEM3)));
*/
}


void mi_w16_intmsk (__u32 addr, __u16 data)
{
	DEBUG (EVENT_LOG_MI, "..mi: write [%.4x] (%.4x) = %.4x", addr & 0xffff, RMI16 (addr), data);
	if (data & MI_INTMSK_ALL)
		data |= MI_INTMSK_MEM0 | MI_INTMSK_MEM1 | MI_INTMSK_MEM2 | MI_INTMSK_MEM3;

	RMI16 (addr) = data;
	
	mi_update_interrupts ();
}


void mi_w16_int (__u32 addr, __u16 data)
{
	DEBUG (EVENT_LOG_MI, "..mi: write [%.4x] (%.4x) = %.4x", addr & 0xffff, RMI16 (addr), data);

	if (data & MI_INTMSK_ALL)
		RMI16 (MI_INT) &= ~(MI_INTMSK_ALL | MI_INTMSK_MEM0 | MI_INTMSK_MEM1 | MI_INTMSK_MEM2 | MI_INTMSK_MEM3);

	if (data & MI_INTMSK_MEM0)
		RMI16 (MI_INT) &= ~MI_INTMSK_MEM0;

	if (data & MI_INTMSK_MEM1)
		RMI16 (MI_INT) &= ~MI_INTMSK_MEM1;

	if (data & MI_INTMSK_MEM2)
		RMI16 (MI_INT) &= ~MI_INTMSK_MEM2;

	if (data & MI_INTMSK_MEM3)
		RMI16 (MI_INT) &= ~MI_INTMSK_MEM3;

	mi_update_interrupts ();
}


void mi_reinit (void)
{
}


void mi_init (void)
{
	memset (rmi, 0, sizeof (rmi));

	// protected regions
	mem_hwr_hook (16, 0x4000, mi_r16_direct);
	mem_hww_hook (16, 0x4000, mi_w16_direct);
	mem_hwr_hook (16, 0x4002, mi_r16_direct);
	mem_hww_hook (16, 0x4002, mi_w16_direct);
	mem_hwr_hook (16, 0x4004, mi_r16_direct);
	mem_hww_hook (16, 0x4004, mi_w16_direct);
	mem_hwr_hook (16, 0x4006, mi_r16_direct);
	mem_hww_hook (16, 0x4006, mi_w16_direct);
	mem_hwr_hook (16, 0x4008, mi_r16_direct);
	mem_hww_hook (16, 0x4008, mi_w16_direct);
	mem_hwr_hook (16, 0x400a, mi_r16_direct);
	mem_hww_hook (16, 0x400a, mi_w16_direct);
	mem_hwr_hook (16, 0x400c, mi_r16_direct);
	mem_hww_hook (16, 0x400c, mi_w16_direct);
	mem_hwr_hook (16, 0x400e, mi_r16_direct);
	mem_hww_hook (16, 0x400e, mi_w16_direct);

	// protection type
	mem_hwr_hook (16, 0x4010, mi_r16_direct);
	mem_hww_hook (16, 0x4010, mi_w16_direct);

	// interrupt mask
	mem_hwr_hook (16, 0x401c, mi_r16_direct);
	mem_hww_hook (16, 0x401c, mi_w16_intmsk);

	// interrupt cause
	mem_hwr_hook (16, 0x401e, mi_r16_direct);
	mem_hww_hook (16, 0x401e, mi_w16_int);
}

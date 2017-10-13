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
 *  PI - processor interface (0xcc003000)
 *      
 *         
 */

#include "hw_pi.h"


__u8 rpi[RPI_SIZE];


__u32 pi_r32_direct (__u32 addr)
{
	DEBUG (EVENT_LOG_PI, "..pi: read  [%.4x] (%.8x)", addr & 0xffff, RPI32 (addr));
	return RPI32 (addr);
}


void pi_w32_direct (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_PI, "..pi: write [%.4x] (%.8x) = %.8x", addr & 0xffff, RPI32 (addr), data);
	RPI32 (addr) = data;
}


void pi_w32_intsr (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_PI, "..pi: writing to INTSR %.8x", data);
	
	INTSR &= ~data;
	pi_check_for_interrupts ();
}


void pi_w32_intmskr (__u32 addr, __u32 data)
{
	gdebug_print_intmask (data, "..pi: INTMASK ");

	INTMR = data;
	pi_check_for_interrupts ();
}


__u32 pi_r32_wpointer (__u32 addr)
{
	if (PI_FIFO_WPOINTER & 31)
	{
		// align write pointer to 32 byte boundary
		while ((PI_FIFO_WPOINTER & 31) && (0 == MEM (PI_FIFO_WPOINTER - 1)))
			PI_FIFO_WPOINTER--;
	}

	DEBUG (EVENT_LOG_PI, "..pi: read wpointer (%.8x)", RPI32 (addr));
	return RPI32 (addr);
}


int delayed_ints[16] = {-1};

void delayed_interrupt_set (int num, int delay)
{
	if (delayed_ints[num] > 0)
	{
			INTSR |= INTERRUPT_DI;
			pi_check_for_interrupts ();
	}

	delayed_ints[num] = delay;
}


void delayed_interrupt_check (void)
{
	// only INTERRUPT_DI
	if (delayed_ints[2] > 0)
	{
		if (!--delayed_ints[2])
		{
			delayed_ints[2] = -1;
			INTSR |= INTERRUPT_DI;
		}
	}
}


void pi_check_for_interrupts (void)
{
	delayed_interrupt_check ();
	if ((INTSR & INTMR) && (MSR & MSR_EE))
	{
		DEBUG (EVENT_LOG_INT, "INT: ");
		cpu_exception (EXCEPTION_EXTERNAL);
	}
}


void pi_interrupt (__u32 mask, int set)
{
	if (set && (mask == INTERRUPT_DI))
	{
		delayed_interrupt_set (2, 2000);
		return;
	}

	if (set)
		INTSR |= mask;
	else
		INTSR &= ~mask;

	pi_check_for_interrupts ();
}


void pi_interrupt_ex (__u32 mask, int set)
{
	if (set)
	{
		if (!(INTSR & mask))
			printf ("ex interrupt set: %.8x\n", mask);
	}
	else
	{
		if (INTSR & mask)
			printf ("ex interrupt cleared: %.8x\n", mask);
	}

	if (set)
		INTSR |= mask;
	else
		INTSR &= ~mask;

	if (MSR & MSR_EE)
	{
		PC -= 4;
		cpu_exception (EXCEPTION_EXTERNAL + 4);
	}
}


void pi_reinit (void)
{
}


void pi_init (void)
{
	memset (rpi, 0, sizeof (rpi));


	INTSR |= INTERRUPT_RESET_DEPRESSED;

	// console type (retail3)
//	RPI32 (0x302c) = 0x20000000;
	mem_hwr_hook (32, 0x302c, pi_r32_direct);

	// interrupt status register
	mem_hwr_hook (32, PI_INTSR, pi_r32_direct);
	mem_hww_hook (32, PI_INTSR, pi_w32_intsr);

	// interrupt mask register
	mem_hwr_hook (32, PI_INTMSKR, pi_r32_direct);
	mem_hww_hook (32, PI_INTMSKR, pi_w32_intmskr);

	// fifo base start / end / write pointer
	mem_hwr_hook (32, 0x300c, pi_r32_direct);
	mem_hww_hook (32, 0x300c, pi_w32_direct);
	mem_hwr_hook (32, 0x3010, pi_r32_direct);
	mem_hww_hook (32, 0x3010, pi_w32_direct);
	mem_hwr_hook (32, 0x3014, pi_r32_wpointer);
	mem_hww_hook (32, 0x3014, pi_w32_direct);
	
	// reset code
	RPI32 (0x3024) = 0x80000000;
	mem_hwr_hook (32, 0x3024, pi_r32_direct);
}

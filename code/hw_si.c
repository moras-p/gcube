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
 *  SI - serial interface (0xcc006400, 0x40)
 *      
 *         
 */

#include "hw_si.h"


__u8 rsi[RSI_SIZE];


// config.c will set it to standard device
int controller_type[4];
PADStatus pads[4];

void pad_set_buttons (int n, __u16 buttons)
{
	// additionally set analog l & r triggers and a & b buttons
	pads[n].buttons = buttons;
	pads[n].ltrigger = (pads[n].buttons & BUTTON_L) ? 255 : 0;
	pads[n].rtrigger = (pads[n].buttons & BUTTON_R) ? 255 : 0;
	pads[n].aanalog = (pads[n].buttons & BUTTON_A) ? 255 : 0;
	pads[n].banalog = (pads[n].buttons & BUTTON_B) ? 255 : 0;
}


void pad_action_stick (int n, __s8 x, __s8 y)
{
	pads[n].xstick = x;
	pads[n].ystick = y;
}


void pad_action_substick (int n, __s8 x, __s8 y)
{
	pads[n].xsubstick = x;
	pads[n].ysubstick = y;
}


void pad_set_action (int n, __u16 buttons, __s8 xstick, __s8 ystick,
										 __s8 xsubstick, __s8 ysubstick)
{
	pad_set_buttons (n, buttons);
	pads[n].xstick = xstick;
	pads[n].ystick = ystick;
	pads[n].xsubstick = xsubstick;
	pads[n].ysubstick = ysubstick;
}


void pad_action_buttons (int n, int pressed, __u16 buttons)
{
	if (pressed)
		pad_set_buttons (n, pads[n].buttons | buttons);
	else
		pad_set_buttons (n, pads[n].buttons &~ buttons);
}


void pad_action_buttons_masked (int n, int pressed, __u16 buttons, int mask)
{
	pad_set_buttons (n, pads[n].buttons &~ mask);
	pad_action_buttons (n, pressed, buttons);
}


void pad_action_axis (int n, int axis, __s8 value)
{
	switch (axis)
	{
		// analog y
		case 0:
			pads[n].ystick = value;
			break;

		// analog x
		case 1:
			pads[n].xstick = value;
			break;

		// canalog y
		case 2:
			pads[n].ysubstick = value;
			break;
		
		// canalog x
		case 3:
			pads[n].xsubstick = value;
			break;

		default:
			break;
	}
}


// for keyboard mapping
void pad_action (int n, int pressed, int action)
{
	if (action < ACTION_AXIS)
		pad_action_buttons (n, pressed, action);
	else
	{
		action -= ACTION_AXIS;
		// axis0-, axis0+, ...
		if (pressed)
		{
		// use only half of the max range of analog
			if (action & 2)
				pad_action_axis (n, action / 2, (action & 1) ? 127/1 : -128/1);
			else
				pad_action_axis (n, action / 2, (action & 1) ? -128/1 : 127/1);
		}
		else
			pad_action_axis (n, action / 2, 0);
	}
}


void si_transfer_data (int channel, __u32 *buff, int length)
{
	if (controller_type[channel])
	{
		// do transfer
		DEBUG (EVENT_LOG_SI, "..... COMMAND %.2x on channel %d", buff[0] >> 24, channel);
		switch (buff[0] >> 24)
		{
			// reset, get device type and status
			case 0x00:
				*buff = controller_type[channel];
				break;
			
			// read origins
			case 0x41:
			// recalibrate
			case 0x42:
				// cmd, analog sticks origin
				buff[0] = 0x41008080;
				// analog substick origin, l and r triggers
				buff[1] = 0x80800000;
				// a and b buttons, ???
				buff[2] = 0x00000000;
				break;

			default:
				DEBUG (EVENT_LOG_SI, "..si: unknown command %.2x", buff[0] >> 24);
		}
	}
	else
		*buff = 0;
}		


void si_send_command (int channel, __u32 command)
{
}


int si_get_data (int n, __u32 *hi, __u32 *lo)
{
	if (controller_type[n])
	{
		__u32 new_hi, new_lo;
	
		new_hi = SI_CHH (pads[n].buttons, pads[n].xstick, pads[n].ystick);
		new_lo = SI_CHL (pads[n].xsubstick, pads[n].ysubstick, pads[n].ltrigger, pads[n].rtrigger);

		// use origin ?
		new_hi |= 0x00800000;

		*hi = new_hi;
		*lo = new_lo;
		return TRUE;
	}
	else
		return FALSE;
}


__u32 si_r32_direct (__u32 addr)
{
	DEBUG (EVENT_LOG_SI, "..si: read  [%.4x] (%.8x) (%.8x)", addr & 0xffff, RSI32 (addr), PC);
	return RSI32 (addr);
}


void si_w32_direct (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_SI, "..si: write [%.4x] (%.8x)", addr & 0xffff, data);
	RSI32 (addr) = data;
}


void si_update_interrupts (void)
{
	pi_interrupt (INTERRUPT_SI,
			((SICSR & SI_COMMCSR_TCINT) && (SICSR & SI_COMMCSR_TCINTMSK)) ||
			((SICSR & SI_COMMCSR_RDSTINT) && (SICSR & SI_COMMCSR_RDSTINTMSK)));
}


void si_generate_interrupt (__u32 mask)
{
	if (mask & SI_INTERRUPT_TC)
		SICSR |= SI_COMMCSR_TCINT;

	if (mask & SI_INTERRUPT_RDST)
		SICSR |= SI_COMMCSR_RDSTINT;
		
	si_update_interrupts ();
}


__u32 si_r32_input (__u32 addr)
{
	int port = ((addr - SI_CH0_INH) & 0xff) / 0x0c;
	int rdst[] = 
	{
		SI_SR_RDST0, SI_SR_RDST1, SI_SR_RDST2, SI_SR_RDST3,
	};
	

	SISR &= ~rdst[port];

	if (!(SISR & (SI_SR_RDST0 | SI_SR_RDST1 | SI_SR_RDST2 | SI_SR_RDST3)))
		SICSR &= ~SI_COMMCSR_RDSTINT;

	si_update_interrupts ();

	return RSI32 (addr);
}


void si_w32_commcsr (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_SI, "..si: write [%.4x] (%.8x) CSR", addr & 0xffff, data);

	SICSR &= SI_COMMCSR_TCINT | SI_COMMCSR_RDSTINT;
	SICSR |= data & (SI_COMMCSR_TCINTMSK | SI_COMMCSR_RDSTINTMSK |
									 SI_COMMCSR_OUTLNGTH | SI_COMMCSR_INLNGTH | SI_COMMCSR_CHANNEL);

	if (data & SI_COMMCSR_TCINT)
		SICSR &= ~SI_COMMCSR_TCINT;

	if (data & SI_COMMCSR_RDSTINT)
		SICSR &= ~SI_COMMCSR_RDSTINT;

	if (data & SI_COMMCSR_TSTART)
	{
		// length
		int len = SI_INLENGTH;

		if (len == 0)
			len = 128;
		
		// start transfer
		si_transfer_data (SI_CHANNEL, &RSI32 (SI_IO_BUFF), len);

		// transfer complete interrupt
		SICSR |= SI_COMMCSR_TCINT;
	}

	si_update_interrupts ();
}


void si_w32_sr (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_SI, "..si: write [%.4x] (%.8x) SR", addr & 0xffff, data);

	SISR = (SISR &~ SI_SR_ERROR_BITS) | (data & SI_SR_ERROR_BITS);
	if (data & SI_SR_WR)
	{
		// copy all buffers
		si_send_command (0, RSI32 (SI_CH0_CMD));
		si_send_command (1, RSI32 (SI_CH1_CMD));
		si_send_command (2, RSI32 (SI_CH2_CMD));
		si_send_command (3, RSI32 (SI_CH3_CMD));

		SISR &= ~(SI_SR_WRST0 | SI_SR_WRST1 | SI_SR_WRST2 | SI_SR_WRST3);
	}
}


void si_w32_sipoll (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_SI, "..si: write [%.4x] (%.8x) SIPOLL", addr & 0xffff, data);

	if (data & SI_POLL_EN0)
		RSI32 (SI_SR) |= SI_SR_RDST0;

	if (data & SI_POLL_EN1)
		RSI32 (SI_SR) |= SI_SR_RDST1;

	if (data & SI_POLL_EN2)
		RSI32 (SI_SR) |= SI_SR_RDST2;

	if (data & SI_POLL_EN3)
		RSI32 (SI_SR) |= SI_SR_RDST3;

	RSI32 (addr) = data;
}


void si_update_devices (void)
{
	if (si_get_data (0, &RSI32 (SI_CH0_INH), &RSI32 (SI_CH0_INL)))
		SISR |= SI_SR_RDST0;
	else
		SISR &= ~SI_SR_RDST0;

	if (si_get_data (1, &RSI32 (SI_CH1_INH), &RSI32 (SI_CH1_INL)))
		SISR |= SI_SR_RDST1;
	else
		SISR &= ~SI_SR_RDST1;

	if (si_get_data (2, &RSI32 (SI_CH2_INH), &RSI32 (SI_CH2_INL)))
		SISR |= SI_SR_RDST2;
	else
		SISR &= ~SI_SR_RDST2;

	if (si_get_data (3, &RSI32 (SI_CH3_INH), &RSI32 (SI_CH3_INL)))
		SISR |= SI_SR_RDST3;
	else
		SISR &= ~SI_SR_RDST3;

	if (SISR & (SI_SR_RDST0 | SI_SR_RDST1 | SI_SR_RDST2 | SI_SR_RDST3))
		SICSR |= SI_COMMCSR_RDSTINT;

	si_update_interrupts ();
}


void si_poll (void)
{
	si_update_devices ();
}


void si_reinit (void)
{
}


void si_init (void)
{
	int i;


	memset (rsi, 0, sizeof (rsi));

	// initialize pads
	for (i = 0; i < 4; i++)
	{
		memset (&pads[i], 0, sizeof (PADStatus));
	
		// pad command
		RSI32 (SI_CHN_CMD (i)) = 0x00400300;		// ?

		// center analogs
		mem_hwr_hook (32, SI_CHN_CMD (i), si_r32_direct);
		mem_hww_hook (32, SI_CHN_CMD (i), si_w32_direct);

		// clear rdst on read
		mem_hwr_hook (32, SI_CHN_INH (i), si_r32_input);
		mem_hww_hook (32, SI_CHN_INH (i), si_w32_direct);
		mem_hwr_hook (32, SI_CHN_INL (i), si_r32_input);
		mem_hww_hook (32, SI_CHN_INL (i), si_w32_direct);
	}

	// sipoll
	mem_hwr_hook (32, SI_POLL, si_r32_direct);	
	mem_hww_hook (32, SI_POLL, si_w32_direct);

	// sicommcsr
	mem_hwr_hook (32, SI_COMMCSR, si_r32_direct);	
	mem_hww_hook (32, SI_COMMCSR, si_w32_commcsr);	

	// sisr - si status register
	mem_hwr_hook (32, SI_SR, si_r32_direct);	
	mem_hww_hook (32, SI_SR, si_w32_sr);	
	
	// siexilk - si exi clock lock
	mem_hwr_hook (32, SI_EXILK, si_r32_direct);	
	mem_hww_hook (32, SI_EXILK, si_w32_direct);	

	// si io buffer
	for (i = 0; i < 128; i += 4)
	{
		mem_hww_hook (32, SI_IO_BUFF + i, si_w32_direct);
		mem_hwr_hook (32, SI_IO_BUFF + i, si_r32_direct);
	}
}

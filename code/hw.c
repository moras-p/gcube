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
 *
 *      
 *         
 */

#include "hw.h"



//////////////////////////////////////////////////////////////////////////////
// this is a mess, needs cleaning up and fixing, but for now it works
#define CPUREGS_PRIV 512
#define refresh_delay			(CPUREGS (CPUREGS_PRIV + 2))
#define pe_refresh				(CPUREGS (CPUREGS_PRIV + 3))
#define vcount						(CPUREGS (CPUREGS_PRIV + 4))
#define do_postretrace		(CPUREGS (CPUREGS_PRIV + 5))
#define retrace_count			(CPUREGS (CPUREGS_PRIV + 6))
#define fifo_count				(CPUREGS (CPUREGS_PRIV + 7))
#define audio_count				(CPUREGS (CPUREGS_PRIV + 8))
#define AUDIO_COUNT 			0x2ffff
#define FIFO_COUNT 				0xaffff
#define RETRACE_COUNT 		0xfffff

int ref_delay = 110000;


void zero_state (void)
{
	refresh_delay = 640*480*3;
	retrace_count = RETRACE_COUNT;
	fifo_count = FIFO_COUNT;
	audio_count = AUDIO_COUNT;
}


void gcube_perf_vertices (int count)
{
	vcount += count;
}


void gcube_pe_refresh (void)
{
	pe_refresh = TRUE;

	// refresh only if something was drawn
	if (vcount)
	{
		refresh_delay = ref_delay;

		vcount = 0;
		video_refresh_nofb ();
	}
	else
		video_input_check ();
}


void hw_force_vi_refresh (void)
{
	IC = refresh_delay;
}


void do_refresh (void)
{
	if (IC > refresh_delay)
	{
		IC = 0;
		vi_refresh ();
		if (!pe_refresh)
			video_refresh ();
	}
}


void (*vid_refresh) (void) = do_refresh;


void refresh_manual (void)
{
	if (IC > 640*480*30)
	{
		refresh_delay = 640*480*5;
		vid_refresh = do_refresh;
	}
}


void gcube_refresh_manual (void)
{
	vid_refresh = refresh_manual;
	// don't call vi interrupt too often
	if (IC > 0xfff)
	{
		IC = 0;

		if (!pe_refresh)
			video_refresh ();

		vi_refresh ();
	}
}


void hw_update (void)
{
		if (!audio_count--)
		{
			audio_count = AUDIO_COUNT;
//			dsp_generate_interrupt (DSP_INTERRUPT_AID);
			dsp_update ();
		}
		
		if (!fifo_count--)
		{
			fifo_count = FIFO_COUNT;

			video_input_check ();
			si_update_devices ();
//			dsp_update ();
		}

#if 0
		if (!retrace_count--)
		{
			retrace_count = RETRACE_COUNT;
# if 1
			if (do_postretrace)
			{
				video_postretrace ();
				do_postretrace = FALSE;
			}
			else
			{
				video_preretrace ();
				do_postretrace = TRUE;
			}
# else
			video_preretrace ();
# endif
		}
#else
	vid_refresh ();
#endif
}
//////////////////////////////////////////////////////////////////////////////


int hw_rword (__u32 address, __u32 *data)
{
	address &= 0xfffc;
	
	if (address < 0x1000)
	{
	// CP
		*data = RCP32 (address);
		return TRUE;
	}
	else if (address < 0x2000)
	// PE, only as 16b
		return FALSE;
	else if (address < 0x3000)
	{
	// VI
		*data = RVI32 (address);
		return TRUE;
	}
	else if (address < 0x4000)
	{
	// PI, only as 32b
		*data = RPI32 (address);
		return TRUE;
	}
	else if (address < 0x5000)
	// MI, only as 16b
		return FALSE;
	else if (address < 0x6000)
	{
	// DSP
		*data = RDSP32 (address);
		return TRUE;
	}
	else if (address < 0x6400)
	{
	// DI, only as 32b
		*data = RDI32 (address);
		return TRUE;
	}
	else if (address < 0x6800)
	{
	// SI, only as 32b
		*data = RSI32 (address);
		return TRUE;
	}
	else if (address < 0x6c00)
	{
	// EXI, only as 32b
		*data = REXI32 (address);
		return TRUE;
	}
	else if (address < 0x6c10)
	{
	// AI, only as 32b
		*data = RAI32 (address);
		return TRUE;
	}
	
	return FALSE;
}


int hw_rhalf (__u32 address, __u16 *data)
{
	address &= 0xfffe;
	
	if (address < 0x1000)
	{
	// CP
		*data = RCP16 (address);
		return TRUE;
	}
	else if (address < 0x2000)
	{
	// PE, only as 16b
		*data = RPE16 (address);
		return TRUE;
	}
	else if (address < 0x3000)
	{
	// VI
		*data = RVI16 (address);
		return TRUE;
	}
	else if (address < 0x4000)
	// PI, only as 32b
		return FALSE;
	else if (address < 0x5000)
	{
	// MI, only as 16b
		*data = RMI16 (address);
		return TRUE;
	}
	else if (address < 0x6000)
	{
	// DSP
		*data = RDSP16 (address);
		return TRUE;
	}
	else if (address < 0x6400)
	// DI, only as 32b
		return FALSE;
	else if (address < 0x6800)
	// SI, only as 32b
		return FALSE;
	else if (address < 0x6c00)
	// EXI, only as 32b
		return FALSE;
	else if (address < 0x6c10)
	// AI, only as 32b
		return FALSE;
	
	return FALSE;
}


int hw_wword (__u32 address, __u32 data)
{
	address &= 0xfffc;
	
	if (address < 0x1000)
	{
	// CP
		RCP32 (address) = data;
		return TRUE;
	}
	else if (address < 0x2000)
	// PE, only as 16b
		return FALSE;
	else if (address < 0x3000)
	{
	// VI
		RVI32 (address) = data;
		return TRUE;
	}
	else if (address < 0x4000)
	{
	// PI, only as 32b
		RPI32 (address) = data;
		return TRUE;
	}
	else if (address < 0x5000)
	// MI, only as 16b
		return FALSE;
	else if (address < 0x6000)
	{
	// DSP
		RDSP32 (address) = data;
		return TRUE;
	}
	else if (address < 0x6400)
	{
	// DI, only as 32b
		RDI32 (address) = data;
		return TRUE;
	}
	else if (address < 0x6800)
	{
	// SI, only as 32b
		RSI32 (address) = data;
		return TRUE;
	}
	else if (address < 0x6c00)
	{
	// EXI, only as 32b
		REXI32 (address) = data;
		return TRUE;
	}
	else if (address < 0x6c10)
	{
	// AI, only as 32b
		RAI32 (address) = data;
		return TRUE;
	}
	
	return FALSE;
}


int hw_whalf (__u32 address, __u16 data)
{
	address &= 0xfffe;
	
	if (address < 0x1000)
	{
	// CP
		RCP16 (address) = data;
		return TRUE;
	}
	else if (address < 0x2000)
	{
	// PE, only as 16b
		RPE16 (address) = data;
		return TRUE;
	}
	else if (address < 0x3000)
	{
	// VI
		RVI16 (address) = data;
		return TRUE;
	}
	else if (address < 0x4000)
	// PI, only as 32b
		return FALSE;
	else if (address < 0x5000)
	{
	// MI, only as 16b
		RMI16 (address) = data;
		return TRUE;
	}
	else if (address < 0x6000)
	{
	// DSP
		RDSP16 (address) = data;
		return TRUE;
	}
	else if (address < 0x6400)
	// DI, only as 32b
		return FALSE;
	else if (address < 0x6800)
	// SI, only as 32b
		return FALSE;
	else if (address < 0x6c00)
	// EXI, only as 32b
		return FALSE;
	else if (address < 0x6c10)
	// AI, only as 32b
		return FALSE;
	
	return FALSE;
}


void install_exception_handlers (void)
{
	__u32 reset_handler[] =
	{
	// disabled now
		0x38600000,		// li  r3, 0
		0x3d20cc00,		// lis r9, 0xcc00
		0x90693024,		// stw r3, 0x3024 (r9) -> will cause reset
	};
	__u32 decrementer_handler[] =
	{
		// hack to get testdemo4 working
		0x3bff0001,		// addi r31, r31, 1
		0x4c000064,		// rfi
	};
	__u32 empty_handler[] =
	{
		0x3d20cc00,		// lis r9, 0xcc00
		0x61293000,		// ori r9, r9, 0x3000
		0x80090000,		// lwz r0, 0 (r9)
		0x4c000064,		// rfi
	};
	unsigned int i;


	for (i = 0; i < sizeof (reset_handler) / 4; i++)
	{
		MEMWR32 (EXCEPTION_SYSTEM_RESET + i*4, reset_handler[i]);
		// dol reload
		MEMWR32 (0x80001800 + i*4, reset_handler[i]);
	}

	for (i = 0; i < sizeof (decrementer_handler) / 4; i++)
		MEMWR32 (EXCEPTION_DECREMENTER + i*4, decrementer_handler[i]);

	for (i = 0; i < sizeof (empty_handler) / 4; i++)
	{
		MEMWR32 (EXCEPTION_SYSTEM_CALL + i*4, empty_handler[i]);
		MEMWR32 (EXCEPTION_EXTERNAL + i*4, empty_handler[i]);
	}
}


void hw_set_video_mode (int country_code)
{
	if (country_code == 'E' || country_code == 'J')
	{
		MEMWR32 (MEM_TV_MODE, TV_MODE_NTSC);
		vi_set_video_mode (TV_MODE_NTSC);
	}
	else
	{
		MEMWR32 (MEM_TV_MODE, TV_MODE_PAL);
		vi_set_video_mode (TV_MODE_PAL);
	}

	vi_set_country_code (country_code);
}


void hw_check_interrupts (void)
{
	dsp_check_interrupts ();
}


// after loading states
void hw_reinit (void)
{
	dsp_reinit ();
	cp_reinit ();
	di_reinit ();
	exi_reinit ();
	mi_reinit ();
	pe_reinit ();
	pi_reinit ();
	si_reinit ();
	vi_reinit ();

	gx_reinit ();
}


void hw_init (void)
{
	zero_state ();

	dsp_init ();
	cp_init ();
	di_init ();
	exi_init ();
	mi_init ();
	pe_init ();
	pi_init ();
	si_init ();
	vi_init ();

	gx_init ();

//	MEMWR32 (MEM_CONSOLE_TYPE, CONSOLE_TYPE_LATEST_PB);
	MEMWR32 (MEM_CONSOLE_TYPE, CONSOLE_TYPE_LATEST_DEVKIT);

	MEMWR32 (MEM_HEAP_BOTTOM, ARENA_LO);
	MEMWR32 (MEM_HEAP_TOP, ARENA_HI);
	
	MEMWR32 (MEM_MEM_SIZE, MEM_SIZE);
	MEMWR32 (MEM_SIM_MEM_SIZE, MEM_SIZE);

	MEMWR32 (MEM_BUS_CLOCK_SPEED, BUS_CLOCK_SPEED);
	MEMWR32 (MEM_CPU_CLOCK_SPEED, CPU_CLOCK_SPEED);

	MEMWR32 (MEM_BOOT_MAGIC, MEM_BOOT_MAGIC_NORMAL);
	
//	install_exception_handlers ();
	// default exception handlers (rfi)
	MEMWR32 (EXCEPTION_SYSTEM_RESET, 0x4c000064);
	MEMWR32 (EXCEPTION_DSI, 0x4c000064);
	MEMWR32 (EXCEPTION_EXTERNAL, 0x4c000064);
	MEMWR32 (EXCEPTION_DECREMENTER, 0x4c000064);
	MEMWR32 (EXCEPTION_SYSTEM_CALL, 0x4c000064);
}

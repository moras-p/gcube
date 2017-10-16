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
 *  VI - video interface (0xcc002000)
 *      
 *         
 */

#include "hw_vi.h"


const __u16 hardwired_ntsc[] =
{
	0x0f06,	0x0001,	0x4769, 0x01ad,	0x02ea, 0x5140,	0x0003, 0x0018,
	0x0002, 0x0019,	0x410c, 0x410c,	0x40ed, 0x40ed,	0x0043, 0x5a4e,
	0x0000, 0x0000,	0x0043, 0x5a4e,	0x0000, 0x0000,	0x0000, 0x0000,
	0x1107, 0x01ae,	0x1001, 0x0001,	0x0001, 0x0001,	0x0001, 0x0001,
	0x0000, 0x0000,	0x0000, 0x0000,	0x2850, 0x0100,	0x1ae7, 0x71f0,
	0x0db4, 0xa574,	0x00c1, 0x188e,	0xc4c0, 0xcbe2,	0xfcec, 0xdecf,
	0x1313, 0x0f08,	0x0008, 0x0c0f,	0x00ff, 0x0000,	0x0000, 0x0000,
	0x0280, 0x0000,	0x0000, 0x00ff,	0x00ff, 0x00ff,	0x00ff, 0x00ff,
};

const __u16 hardwired_pal[] =
{
	0x11f5,	0x0101,	0x4b6a, 0x01b0,	0x02f8, 0x5640,	0x0001, 0x0023,
	0x0000, 0x0024,	0x4d2b, 0x4d6d,	0x4d8a, 0x4d4c,	0x0043, 0x5a4e,
	0x0000, 0x0000,	0x0043, 0x5a4e,	0x0000, 0x0000,	0x013c, 0x0144,
	0x1139, 0x01b1,	0x1001, 0x0001,	0x0001, 0x0001,	0x0001, 0x0001,
	0x0000, 0x0000,	0x0000, 0x0000,	0x2850, 0x0100,	0x1ae7, 0x71f0,
	0x0db4, 0xa574,	0x00c1, 0x188e,	0xc4c0, 0xcbe2,	0xfcec, 0xdecf,
	0x1313, 0x0f08,	0x0008, 0x0c0f,	0x00ff, 0x0000,	0x0000, 0x0000,
	0x0280, 0x0000,	0x0000, 0x00ff,	0x00ff, 0x00ff,	0x00ff, 0x00ff,
};


__u8 rvi[RVI_SIZE];


__u32 vi_r32_direct (__u32 addr)
{
	DEBUG (EVENT_LOG_VI, "..vi: read  [%.4x] (%.8x)", addr & 0xffff, RVI32 (addr));
	return RVI32 (addr);
}


void vi_w32_direct (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_VI, "..vi: write [%.4x] (%.8x) = %.8x", addr & 0xffff, RVI32 (addr), data);
	RVI32 (addr) = data;
}


__u16 vi_r16_direct (__u32 addr)
{
	DEBUG (EVENT_LOG_VI, "..vi: read  [%.4x] (%.4x)", addr & 0xffff, RVI16 (addr));
	return RVI16 (addr);
}


void vi_w16_direct (__u32 addr, __u16 data)
{
	DEBUG (EVENT_LOG_VI, "..vi: write [%.4x] (%.4x) = %.4x", addr & 0xffff, RVI16 (addr), data);
	RVI16 (addr) = data;
}


__u8 vi_r8_direct (__u32 addr)
{
	DEBUG (EVENT_LOG_VI, "..vi: read  [%.4x] (%.2x)", addr & 0xffff, RVI8 (addr));
	return RVI8 (addr);
}


void vi_update_interrupts (void)
{
	pi_interrupt (INTERRUPT_VI,
			((RVI32 (VI_DINT0) & VI_DI_INT) && (RVI32 (VI_DINT0) & VI_DI_INTEN)) ||
			((RVI32 (VI_DINT1) & VI_DI_INT) && (RVI32 (VI_DINT1) & VI_DI_INTEN)) ||
			((RVI32 (VI_DINT2) & VI_DI_INT) && (RVI32 (VI_DINT2) & VI_DI_INTEN)) ||
			((RVI32 (VI_DINT3) & VI_DI_INT) && (RVI32 (VI_DINT3) & VI_DI_INTEN)));
}


void vi_generate_interrupt (__u32 mask)
{
	if (mask & VI_INTERRUPT_1)
		RVI32 (VI_DINT0) |= VI_DI_INT;

	if (mask & VI_INTERRUPT_2)
		RVI32 (VI_DINT1) |= VI_DI_INT;

	if (mask & VI_INTERRUPT_3)
		RVI32 (VI_DINT2) |= VI_DI_INT;

	if (mask & VI_INTERRUPT_4)
		RVI32 (VI_DINT3) |= VI_DI_INT;

	vi_update_interrupts ();
}


void vi_w32_int (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_VI, "..vi: write [%.4x] (%.8x) = %.8x", addr & 0xffff, RVI32 (addr), data);

	if (data & VI_DI_INT)
		data &= ~VI_DI_INT;

	RVI32 (addr) = data;
	
	vi_update_interrupts ();
}


void vi_w16_int (__u32 addr, __u16 data)
{
	DEBUG (EVENT_LOG_VI, "..vi: write [%.4x] (%.4x) = %.4x", addr & 0xffff, RVI16 (addr), data);

	if (data & (VI_DI_INT >> 16))
		data &= ~(VI_DI_INT >> 16);

	RVI16 (addr) = data;
	
	vi_update_interrupts ();
}


// todo: set correct num of lines
void vi_w16_mode (__u32 addr, __u16 data)
{
	DEBUG (EVENT_LOG_VI, "..vi: setting resolution %d / %d (%4.4x)", VIDEO_WIDTH, VIDEO_HEIGHT, data);
	video_init (VIDEO_WIDTH, VIDEO_HEIGHT);

	RVI16 (addr) = data;
}


void vi_w32_mode (__u32 addr, __u32 data)
{
	RVI32 (addr) = data;
	vi_w16_mode (addr, (__u16) data);
}


// external framebuffer half one
void vi_w32_xfb1 (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_VI, "..vi: xfb 1 address set to 0x%.8x", data);

	video_set_framebuffer ((__u8 *) MEM_ADDRESS (data));
	
	RVI32 (addr) = data;
}


// external framebuffer half two
void vi_w32_xfb2 (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_VI, "..vi: xfb 2 address set to 0x%.8x", data);
	RVI32 (addr) = data;
}


void vi_w16_xfb1 (__u32 addr, __u16 data)
{
	RVI16 (addr) = data;
	DEBUG (EVENT_LOG_VI, "..vi: xfb 1 address set to 0x%.8x", RVI32 (0x201c));

	video_set_framebuffer ((__u8 *) MEM_ADDRESS (RVI32 (0x201c)));
}


void vi_w16_xfb2 (__u32 addr, __u16 data)
{
	RVI16 (addr) = data;
	DEBUG (EVENT_LOG_VI, "..vi: xfb 2 address set to 0x%.8x", RVI32 (0x2024));
}


/*
// VIGetCurrentLine
// vct and hct are always masked with 0x7ff
1. wait until (this_vct == last_vct)
// modes magic values
mheight = 429
mwidth  = 525
2. vpos = ((vct - 1) << 1) + ((hct - 1) / mheight)
3. if (vpos > mwidth) vpos -= mwidth;
4. ret (vpos >> 1)
*/

// get current vertical position
/*
__u16 vi_r16_vct (__u32 addr)
{
	// fake vsync
	if (RVI16 (VI_VCT) == 0)
	{
		RVI16 (VI_VCT) = 1;
		return 0;
	}
	else if (RVI16 (VI_VCT) == 1)
		return (RVI16 (VI_VCT) = 200);
	else if (RVI16 (VI_VCT) == 200)
		return (RVI16 (VI_VCT) = 522);
	else if (RVI16 (VI_VCT) == 522)
		return (RVI16 (VI_VCT) = 574);
	else if (RVI16 (VI_VCT) == 574)
	{
		RVI16 (VI_VCT) = 575;
		return 574;
	}
	else
	{
		gcube_refresh_manual ();
		return (RVI16 (VI_VCT) = 0);
	}
}


// current horizontal position of the beam
__u16 vi_r16_hct (__u32 addr)
{
	return (RVI16 (VI_HCT) ^= 639);
}
*/
__u16 vi_r16_vct (__u32 addr)
{
	// fake vsync
	if (RVI16 (VI_VCT) == 0)
		return (RVI16 (VI_VCT) = 1);
	else if (RVI16 (VI_VCT) == 1)
	{
		RVI16 (VI_VCT) = 100;
		return 1;
	}
	else if (RVI16 (VI_VCT) == 100)
		return (RVI16 (VI_VCT) = 200);
	else if (RVI16 (VI_VCT) == 200)
		return (RVI16 (VI_VCT) = 522);
	else if (RVI16 (VI_VCT) == 522)
		return (RVI16 (VI_VCT) = 574);
	else if (RVI16 (VI_VCT) == 574)
	{
		RVI16 (VI_VCT) = 575;
		return 574;
	}
	else
	{
		gcube_refresh_manual ();
		return (RVI16 (VI_VCT) = 1);
	}
}


// current horizontal position of the beam
__u16 vi_r16_hct (__u32 addr)
{
//	return (RVI16 (VI_HCT) ^= 639);
	return ((RVI16 (VI_HCT) ^= 638) + 1);
}

__u32 vi_r32_vcthct (__u32 addr)
{
	vi_r16_vct (addr);
	vi_r16_hct (addr + 2);
	
	return RVI32 (VI_VCT);
}
// check 0x2002 NIN - interlace selector


void vi_set_video_mode (int mode)
{
	unsigned int i;


	switch (mode)
	{
		// pal50
		case TV_MODE_PAL:
			for (i = 0; i < sizeof (hardwired_pal) / 2; i++)
				RVI16 (0x2000 + i*2) = hardwired_pal[i];
			break;

		// ntsc
		case TV_MODE_NTSC:
			for (i = 0; i < sizeof (hardwired_ntsc) / 2; i++)
				RVI16 (0x2000 + i*2) = hardwired_ntsc[i];
			break;
	}

	RVI16 (0x2002) = 0x0001 | (mode << 8);
}


void vi_refresh (void)
{
#if 0
	if (RVI32 (VI_DINT0) & VI_DI_INTEN)
		RVI32 (VI_DINT0) |= VI_DI_INT;
	if (RVI32 (VI_DINT1) & VI_DI_INTEN)
		RVI32 (VI_DINT1) |= VI_DI_INT;
	if (RVI32 (VI_DINT2) & VI_DI_INTEN)
		RVI32 (VI_DINT2) |= VI_DI_INT;
	if (RVI32 (VI_DINT3) & VI_DI_INTEN)
		RVI32 (VI_DINT3) |= VI_DI_INT;
#else
	RVI32 (VI_DINT0) |= VI_DI_INT;
#endif
	si_poll ();

	vi_update_interrupts ();
}


void vi_set_country_code (unsigned char code)
{
	RVI16 (0x206e) = (__u16) code;
}


void vi_reinit (void)
{
	// video init
	if (RVI32 (VI_XFB1))
		vi_w32_xfb1 (VI_XFB1, RVI32 (VI_XFB1));
	else if (RVI32 (VI_MODE))
		vi_w32_mode (VI_MODE, RVI32 (VI_MODE));
}


void vi_init (void)
{
	memset (rvi, 0, sizeof (rvi));

	// video mode
	mem_hwr_hook ( 8, 0x2000, vi_r8_direct);
	mem_hwr_hook ( 8, 0x2001, vi_r8_direct);
	mem_hwr_hook (16, 0x2000, vi_r16_direct);
	mem_hww_hook (16, 0x2000, vi_w16_mode);
	mem_hwr_hook (16, 0x2002, vi_r16_direct);
	mem_hww_hook (16, 0x2002, vi_w16_mode);
	mem_hwr_hook (32, 0x2000, vi_r32_direct);
	mem_hww_hook (32, 0x2000, vi_w32_mode);

	// horizontal timing
	mem_hww_hook (16, 0x2004, vi_w16_direct);
	mem_hww_hook (16, 0x2006, vi_w16_direct);
	mem_hwr_hook (32, 0x2004, vi_r32_direct);
	mem_hww_hook (32, 0x2004, vi_w32_direct);
	mem_hww_hook (16, 0x2008, vi_w16_direct);
	mem_hww_hook (16, 0x200a, vi_w16_direct);
	mem_hwr_hook (32, 0x2008, vi_r32_direct);
	mem_hww_hook (32, 0x2008, vi_w32_direct);

	// odd / even field vertical timing
	mem_hww_hook (16, 0x200c, vi_w16_direct);
	mem_hww_hook (16, 0x200e, vi_w16_direct);
	mem_hwr_hook (32, 0x200c, vi_r32_direct);
	mem_hww_hook (32, 0x200c, vi_w32_direct);
	mem_hww_hook (16, 0x2010, vi_w16_direct);
	mem_hww_hook (16, 0x2012, vi_w16_direct);
	mem_hwr_hook (32, 0x2010, vi_r32_direct);
	mem_hww_hook (32, 0x2010, vi_w32_direct);

	// odd / even burst blanking internal
	mem_hww_hook (16, 0x2014, vi_w16_direct);
	mem_hww_hook (16, 0x2016, vi_w16_direct);
	mem_hwr_hook (32, 0x2014, vi_r32_direct);
	mem_hww_hook (32, 0x2014, vi_w32_direct);
	mem_hww_hook (16, 0x2018, vi_w16_direct);
	mem_hww_hook (16, 0x201a, vi_w16_direct);
	mem_hwr_hook (32, 0x2018, vi_r32_direct);
	mem_hww_hook (32, 0x2018, vi_w32_direct);

	// framebuffer
	mem_hww_hook (16, 0x201c, vi_w16_direct);
	mem_hww_hook (16, 0x201e, vi_w16_xfb1);
	mem_hwr_hook (32, 0x201c, vi_r32_direct);
	mem_hww_hook (32, 0x201c, vi_w32_xfb1);
	mem_hwr_hook (32, 0x2020, vi_r32_direct);
	mem_hww_hook (32, 0x2020, vi_w32_direct);
	mem_hww_hook (16, 0x2024, vi_w16_direct);
	mem_hww_hook (16, 0x2026, vi_w16_xfb2);
	mem_hwr_hook (32, 0x2024, vi_r32_direct);
	mem_hww_hook (32, 0x2024, vi_w32_xfb2);
	mem_hwr_hook (32, 0x2028, vi_r32_direct);
	mem_hww_hook (32, 0x2028, vi_w32_direct);

	// current position of the beam
	mem_hwr_hook (16, 0x202c, vi_r16_vct);
	mem_hwr_hook (16, 0x202e, vi_r16_hct);
	mem_hwr_hook (32, 0x202c, vi_r32_vcthct);
	mem_hww_hook (32, 0x202c, mem_fake_w32);

	// display interrupt 0 1 2 3
	mem_hwr_hook (32, 0x2030, vi_r32_direct);
	mem_hwr_hook (32, 0x2034, vi_r32_direct);
	mem_hwr_hook (32, 0x2038, vi_r32_direct);
	mem_hwr_hook (32, 0x203c, vi_r32_direct);
	mem_hww_hook (32, 0x2030, vi_w32_int);
	mem_hww_hook (32, 0x2034, vi_w32_int);
	mem_hww_hook (32, 0x2038, vi_w32_int);
	mem_hww_hook (32, 0x203c, vi_w32_int);

	mem_hwr_hook (16, 0x2030, vi_r16_direct);
	mem_hwr_hook (16, 0x2032, vi_r16_direct);
	mem_hwr_hook (16, 0x2034, vi_r16_direct);
	mem_hwr_hook (16, 0x2036, vi_r16_direct);
	mem_hwr_hook (16, 0x2038, vi_r16_direct);
	mem_hwr_hook (16, 0x203a, vi_r16_direct);
	mem_hwr_hook (16, 0x203c, vi_r16_direct);
	mem_hwr_hook (16, 0x203e, vi_r16_direct);

	mem_hww_hook (16, 0x2030, vi_w16_int);
	mem_hww_hook (16, 0x2032, vi_w16_direct);
	mem_hww_hook (16, 0x2034, vi_w16_direct);
	mem_hww_hook (16, 0x2036, vi_w16_direct);
	mem_hww_hook (16, 0x2038, vi_w16_direct);
	mem_hww_hook (16, 0x203a, vi_w16_direct);
	mem_hww_hook (16, 0x203c, vi_w16_direct);
	mem_hww_hook (16, 0x203e, vi_w16_direct);

	// display latch
	mem_hwr_hook (32, 0x2040, vi_r32_direct);
	mem_hww_hook (32, 0x2040, mem_fake_w32);
	mem_hwr_hook (32, 0x2044, vi_r32_direct);
	mem_hww_hook (32, 0x2044, mem_fake_w32);

	// scaling width
	mem_hww_hook (16, 0x2048, vi_w16_direct);
	mem_hww_hook (16, 0x204a, vi_w16_direct);
	mem_hwr_hook (32, 0x2048, vi_r32_direct);
	mem_hww_hook (32, 0x2048, vi_w32_direct);

	// filter coefficient tables
	mem_hww_hook (16, 0x204c, vi_w16_direct);
	mem_hww_hook (16, 0x204e, vi_w16_direct);
	mem_hwr_hook (32, 0x204c, vi_r32_direct);
	mem_hww_hook (32, 0x204c, vi_w32_direct);
	mem_hww_hook (16, 0x2050, vi_w16_direct);
	mem_hww_hook (16, 0x2052, vi_w16_direct);
	mem_hwr_hook (32, 0x2050, vi_r32_direct);
	mem_hww_hook (32, 0x2050, vi_w32_direct);
	mem_hww_hook (16, 0x2054, vi_w16_direct);
	mem_hww_hook (16, 0x2056, vi_w16_direct);
	mem_hwr_hook (32, 0x2054, vi_r32_direct);
	mem_hww_hook (32, 0x2054, vi_w32_direct);
	mem_hww_hook (16, 0x2058, vi_w16_direct);
	mem_hww_hook (16, 0x205a, vi_w16_direct);
	mem_hwr_hook (32, 0x2058, vi_r32_direct);
	mem_hww_hook (32, 0x2058, vi_w32_direct);
	mem_hww_hook (16, 0x205c, vi_w16_direct);
	mem_hww_hook (16, 0x205e, vi_w16_direct);
	mem_hwr_hook (32, 0x205c, vi_r32_direct);
	mem_hww_hook (32, 0x205c, vi_w32_direct);
	mem_hww_hook (16, 0x2060, vi_w16_direct);
	mem_hww_hook (16, 0x2062, vi_w16_direct);
	mem_hwr_hook (32, 0x2060, vi_r32_direct);
	mem_hww_hook (32, 0x2060, vi_w32_direct);
	mem_hww_hook (16, 0x2064, vi_w16_direct);
	mem_hww_hook (16, 0x2066, vi_w16_direct);
	mem_hwr_hook (32, 0x2064, vi_r32_direct);
	mem_hww_hook (32, 0x2064, vi_w32_direct);
	mem_hww_hook (16, 0x2068, vi_w16_direct);
	mem_hww_hook (16, 0x206a, vi_w16_direct);
	mem_hwr_hook (32, 0x2068, vi_r32_direct);
	mem_hww_hook (32, 0x2068, vi_w32_direct);

	// vi clock select / dtv status register
	// 0x206e is used by OSGetFontEncode to select encoding
	mem_hwr_hook (16, 0x206c, vi_r16_direct);
	mem_hww_hook (16, 0x206c, vi_w16_direct);
	mem_hwr_hook (16, 0x206e, vi_r16_direct);
	mem_hww_hook (16, 0x206e, vi_w16_direct);
	mem_hwr_hook (32, 0x206c, vi_r32_direct);
	mem_hww_hook (32, 0x206c, vi_w32_direct);

	// border ?
	mem_hww_hook (32, 0x2070, vi_w32_direct);
	mem_hwr_hook (32, 0x2070, vi_r32_direct);
	mem_hwr_hook (16, 0x2070, vi_r16_direct);
	mem_hww_hook (16, 0x2070, vi_w16_direct);
	mem_hwr_hook (16, 0x2072, vi_r16_direct);
	mem_hww_hook (16, 0x2072, vi_w16_direct);

	mem_hwr_hook ( 8, 0x2074, vi_r8_direct);
	mem_hwr_hook ( 8, 0x2075, vi_r8_direct);
	mem_hww_hook (16, 0x2074, vi_w16_direct);
	mem_hww_hook (16, 0x2076, vi_w16_direct);
	mem_hww_hook (32, 0x2074, vi_w32_direct);
	mem_hww_hook (32, 0x2078, vi_w32_direct);
	mem_hww_hook (32, 0x207c, vi_w32_direct);

	// init registers
	RVI16 (0x2000) = 0x0006;
	RVI16 (0x2004) = 0x4769;
	RVI16 (0x2006) = 0x01ad;
	RVI16 (0x2008) = 0x02ea;
	RVI16 (0x200a) = 0x5140;
	RVI16 (0x200c) = 0x0005;
	RVI16 (0x200e) = 0x01f6;
	RVI16 (0x2010) = 0x0004;
	RVI16 (0x2012) = 0x01f7;
	RVI16 (0x2014) = 0x410c;
	RVI16 (0x2016) = 0x410c;
	RVI16 (0x2018) = 0x40ed;
	RVI16 (0x201a) = 0x40ed;
	RVI16 (0x2030) = 0x1107;
	RVI16 (0x2032) = 0x01ae;
	RVI16 (0x2034) = 0x1001;
	RVI16 (0x2036) = 0x0001;
	RVI16 (0x2048) = 0x2828;
	RVI16 (0x206c) = 0x0000;
}


void video_preretrace (void)
{
	RVI32 (VI_DINT0) |= VI_DI_INT;
	
	vi_update_interrupts ();
}


void video_postretrace (void)
{
	RVI32 (VI_DINT1) |= VI_DI_INT;
	
	vi_update_interrupts ();
}

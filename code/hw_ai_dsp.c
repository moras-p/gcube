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
 *  DSP - digital signal processor interface (0xcc005000)
 *  AI  - audio interface (0xcc006c00)    
 *         
 */


#include "hw_ai_dsp.h"


__u8 rdsp[RDSP_SIZE];
__u8 rai[RAI_SIZE];

__u8 ARAM[ARAM_SIZE];

DSPState dspstate;

#define DSP_INIT			0xdcd10000
#define DSP_RESUME		0xdcd10001
#define DSP_YIELD			0xdcd10002
#define DSP_DONE			0xdcd10003
#define DSP_SYNC			0xdcd10004

void dsp_mail_push (__u32 mail)
{
	*dspstate.mail_queue_top++ = mail;
	
	if (dspstate.mail_queue_top == &dspstate.mail_queue[MAIL_QUEUE_SIZE])
		dspstate.mail_queue_top = dspstate.mail_queue;

	if (dspstate.mail_queue_top == dspstate.mail_queue_bottom)
		DEBUG (EVENT_EFATAL, ".dsp: mail queue is too small (queue_top reached queue_bottom)");
}


__u32 dsp_mail_pop (void)
{
	__u32 mail;
	
	
	if (dspstate.mail_queue_bottom == dspstate.mail_queue_top)
		mail = 0;
	else
		mail = *dspstate.mail_queue_bottom++;
	
	if (dspstate.mail_queue_bottom == &dspstate.mail_queue[MAIL_QUEUE_SIZE])
		dspstate.mail_queue_bottom = dspstate.mail_queue;

	return mail;
}


void dsp_mail_reset (void)
{
	dspstate.mail_queue_top = dspstate.mail_queue_bottom = dspstate.mail_queue;

	dspstate.mail_valid = FALSE;
	dspstate.mail_read = 0;
}


int dsp_mail_empty (void)
{
	return (dspstate.mail_queue_bottom == dspstate.mail_queue_top);
}


void dsp_mail_init (void)
{
	dspstate.mail_queue_top = dspstate.mail_queue_bottom = dspstate.mail_queue;

	dsp_mail_reset ();

	dsp_mail_push (0);
	dsp_mail_push (0x80544348);
	dsp_mail_push (0x8071FEED);
}


void dsp_update (void)
{
	if (!dspstate.booted)
		return;

	if (!dsp_mail_empty ())
		dsp_generate_interrupt (DSP_INTERRUPT_DSP);
}


// decode mail
// this still causes problems
// pokemon colosseum battle mode stops at loading and need
// 'int int_dsp_dsp' from debugger to go further
void dsp_send_mail (__u32 mail)
{
	static __u32 buff[256];
	static int nparams = 0, step = 0, resume = FALSE;
	static int test = FALSE;


	dspstate.mail_valid = FALSE;

	if (nparams == 0)
	{
		if (mail == 0)
		{
			dsp_generate_interrupt (DSP_INTERRUPT_DSP);
		}
		else if ((mail >> 16) == 0)
		{
			DEBUG (EVENT_LOG_DSP, ".dsp: mail -> incoming command with %d params", mail);
			nparams = mail;
		}
		else if ((mail >> 16) == 0xBABE)
		{
			DEBUG (EVENT_LOG_DSP, ".dsp: mail -> 0xBABE %.4x", mail & 0xffff);
			nparams = 1;
			resume = TRUE;
		}
		else if ((mail >> 16) == 0xcdd1)
		{
			// 10 params incoming
			nparams = 10;
			test = TRUE;
		}
		else switch (mail & 0xffff)
		{
			// mail size not given. check command and determine size.
			case 0:
				DEBUG (EVENT_LOG_DSP, "DSP MAIL: release halt");
				dsp_mail_push (DSP_RESUME);
				dsp_generate_interrupt (DSP_INTERRUPT_DSP);
				break;

			// DSetDolbyDelay				
			case 0x0a:
				DEBUG (EVENT_LOG_DSP, ".dsp: mail -> DSetDolbyDelay");
				nparams = 2;
				buff[step++] = mail;
				break;
			
			// DSetupTable
			case 0x40:
				DEBUG (EVENT_LOG_DSP, ".dsp: mail -> DSetupTable");
				buff[step++] = mail;
				nparams = 4;
				resume = TRUE;
				break;

			// DSyncFrame
			case 0x3800:
			case 0x5999:
			case 0x5ffb:
			// UpdateDSPChannel
			case 0x2000:
			case 0x4000:
				buff[step++] = mail;
				nparams = 2;
				break;

			// IRAM MMEM ADDR
			case 0xa001:
			// IRAM DSP ADDR
			case 0xc002:
			// IRAM LENGTH
			case 0xa002:
			// DRAM MMEM ADDR
			case 0xb002:
			// Start Vector
			case 0xd001:
				buff[0] = mail;
				nparams = -1;
				break;
			
			default:
				printf (".dsp: unknown mail %.8x\n", mail);
		}
	}
	else if (nparams < 0)
	{
		if ((buff[0] & 0xffff) == 0xd001)
		{
			// start vector set. boot task.
			dspstate.booted = TRUE;
			dsp_mail_reset ();
			dsp_mail_push (DSP_INIT);
			dsp_mail_push (DSP_RESUME);
		}
		nparams = 0;
	}
	else
	{
		buff[step++] = mail;
		if (!--nparams)
		{
			step = 0;

			if (test)
			{
				// DSP_DONE, DSP_YIELD
//				dsp_mail_push (DSP_DONE);
				test = FALSE;
				return;
			}

			if (resume)
				dsp_mail_push (DSP_RESUME);
			else
			{
				dsp_mail_push (DSP_SYNC);
				dsp_mail_push (0xf3550000 | (buff[0] >> 16));
			}
		}
	}
}


__u32 dsp_read_mail (void)
{
	dspstate.mail_valid = TRUE;

	return dsp_mail_pop ();
}


__u16 dsp_read_mail_hi (void)
{
	if (!dspstate.mail_valid)
		dspstate.mail_read = dsp_read_mail ();

	return (dspstate.mail_read >> 16);
}


// lo is read after hi
__u16 dsp_read_mail_lo (void)
{
	if (!dspstate.mail_valid)
		dspstate.mail_read = dsp_read_mail ();

	dspstate.mail_valid = FALSE;
	return (dspstate.mail_read & 0xffff);
}


void dsp_w16_mailbox (__u32 addr, __u16 data)
{
	RDSP16 (addr) = data;

	if (addr & 2)
	{
		dsp_send_mail (RDSP32 (DSP_MBOX_H));
		RDSP32 (DSP_MBOX_H) = 0;
	}
}


__u32 ai_r32_direct (__u32 addr)
{
	DEBUG (EVENT_LOG_AI, "..ai: read  [%.4x] (%.8x)", addr & 0xffff, RAI32 (addr));
	return RAI32 (addr);
}


void ai_w32_direct (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_AI, "..ai: write [%.4x] (%.8x) = %.8x", addr & 0xffff, RAI32 (addr), data);
	RAI32 (addr) = data;
}


void ai_update_interrupts (void)
{
	pi_interrupt (INTERRUPT_AI,
		(AICR & AICR_AIINT) && (AICR & AICR_AIINTMSK));
}


void ai_generate_interrupt (void)
{
	AICR |= AICR_AIINT;

	ai_update_interrupts ();
}


void ai_w32_cr (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_AI, "..ai: AICR | PSTAT %d | FREQ %d kHz | AIINT %d/%d | AIINTVLD %d | SCRESET %d",
				 data & AICR_PSTAT, (data & AICR_AFR) ? 48 : 32, (data & AICR_AIINTMSK)>0, (data & AICR_AIINT)>0, (data & AICR_AIINTVLD)>0, (data & AICR_SCRESET)>0);

	if ((AICR & AICR_PSTAT) != (data & AICR_PSTAT))
		DEBUG (EVENT_LOG_AI, "....  streaming %s", (data & AICR_PSTAT) ? "started" : "stopped");

	if (data & AICR_SCRESET)
	{
		AISCNT = 0;
		DEBUG (EVENT_LOG_AI, "....  resetting sample counter");
	}

	if (data & AICR_AIINT)
		RAI32 (addr) &= ~AICR_AIINT;

	RAI32 (addr) = (RAI32 (addr) &~ (AICR_PSTAT | AICR_AFR | AICR_AIINTMSK | AICR_AIINTVLD)) |
								 (data & (AICR_PSTAT | AICR_AFR | AICR_AIINTMSK | AICR_AIINTVLD));

	audio_set_freq ((AICR & AICR_AFR) ? FREQ_48KHZ : FREQ_32KHZ);
	
	ai_update_interrupts ();
}


// sample counter
__u32 ai_r32_aiscnt (__u32 addr)
{
	DEBUG (EVENT_LOG_AI, "..ai: AISCNT read %.8x", RAI32 (addr));

	// fake it
	if (AICR & AICR_PSTAT)
		RAI32 (addr)++;

	return RAI32 (addr);
}


void ai_w32_aivr (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_AI, "..ai: VOLUME %d / %d", data & 0x00ff, (data & 0xff00) >> 8);
	RAI32 (addr) = data;
}


__u16 dsp_r16_direct (__u32 addr)
{
	DEBUG (EVENT_LOG_DSP, ".dsp: read  [%.4x] (%.4x)", addr & 0xffff, RDSP16 (addr));

	return RDSP16 (addr);
}


void dsp_w16_direct (__u32 addr, __u16 data)
{
	DEBUG (EVENT_LOG_DSP, ".dsp: write [%.4x] (%.4x) = %.4x", addr & 0xffff, RDSP16 (addr), data);
	RDSP16 (addr) = data;
}


__u32 dsp_r32_direct (__u32 addr)
{
	DEBUG (EVENT_LOG_DSP, ".dsp: read  [%.4x] (%.8x)", addr & 0xffff, RDSP32 (addr));

	return RDSP32 (addr);
}


void dsp_w32_direct (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_DSP, ".dsp: write [%.4x] (%.8x) = %.8x", addr & 0xffff, RDSP32 (addr), data);
	RDSP32 (addr) = data;
}


void dsp_update_interrupts (void)
{
	pi_interrupt (INTERRUPT_DSP,
			((DSPCSR & DSP_CSR_DSPINT) && (DSPCSR & DSP_CSR_DSPINTMSK)) ||
			((DSPCSR & DSP_CSR_ARINT) && (DSPCSR & DSP_CSR_ARINTMSK)) ||
			((DSPCSR & DSP_CSR_AIDINT) && (DSPCSR & DSP_CSR_AIDINTMSK)));
}


void dsp_generate_interrupt (__u32 mask)
{
	if (mask & DSP_INTERRUPT_DSP)
		DSPCSR |= DSP_CSR_DSPINT;

	if (mask & DSP_INTERRUPT_AR)
		DSPCSR |= DSP_CSR_ARINT;

	if (mask & DSP_INTERRUPT_AID)
		DSPCSR |= DSP_CSR_AIDINT;
	
	dsp_update_interrupts ();
}


void dsp_w16_csr (__u32 addr, __u16 data)
{
	DEBUG (EVENT_LOG_DSP, ".dsp: (%.8x) CSR = %.4x %s %s", PC, data,
				 (data & DSP_CSR_RES) ? "reset" : "", (data & DSP_CSR_HALT) ? "halt" : "");

	if (data & DSP_CSR_DSPINT)
		RDSP16 (DSP_CSR) &= ~DSP_CSR_DSPINT;

	if (data & DSP_CSR_ARINT)
		RDSP16 (DSP_CSR) &= ~DSP_CSR_ARINT;

	if (data & DSP_CSR_AIDINT)
		RDSP16 (DSP_CSR) &= ~DSP_CSR_AIDINT;

	// copy all but dsp_reset and dsp_dma_int_status flags
	RDSP16 (DSP_CSR) = (RDSP16 (DSP_CSR) & (DSP_CSR_DSPINT | DSP_CSR_ARINT | DSP_CSR_AIDINT)) |
		 (data &~ (DSP_CSR_DSPINT | DSP_CSR_ARINT | DSP_CSR_AIDINT | DSP_CSR_RES | DSP_CSR_DMAINTST));

	dsp_update_interrupts ();
}


void dsp_w16_dmacr (__u32 addr, __u16 data)
{
	RDSP16 (addr) = data;

	RDSP16 (DMA_BLEFT) = (RDSP16 (DSP_DMACR) & 0x7fff) << 5;
	if (data & DMACR_PLAY)
	{
		DEBUG (EVENT_LOG_DSP, ".dsp: DMACR start %.8x length %d",
					 RDSP32 (DSP_DMA_START_H) | 0x80000000,
					 (RDSP16 (DSP_DMACR) & 0x7fff) << 5);
#if ENABLE_SOUND
		if (data & 0x7fff)
		audio_play (MEM_ADDRESS (RDSP32 (DSP_DMA_START_H)),
								(data & 0x7fff) << 5);
#endif
	}
	else
	{
#if ENABLE_SOUND
		audio_stop ();
#endif
		DEBUG (EVENT_LOG_DSP, ".dsp: DMACR stop, length %d", (data & 0x7fff) << 5);
	}

#if !ENABLE_SOUND
	// fake it
	RDSP16 (DMA_BLEFT) = 0;
	RDSP16 (DSP_DMACR) &= ~DMACR_PLAY;
#endif
}


void dsp_aram_transfer (void)
{
	__u32 size;


	if (RDSP32 (ARDMA_CNT_H) & ARCNT_READ)
	{
		RDSP32 (ARDMA_CNT_H) &= ~ARCNT_READ;

		// transfer from aram
		DEBUG (EVENT_LOG_DSP, ".dsp: read %.8x MEM[%.8x] <- ARAM[%.8x]",
					 RDSP32 (ARDMA_CNT_H), RDSP32 (ARDMA_MMADDR_H) | 0x80000000, RDSP32 (ARDMA_ARADDR_H));

		// ARCheckSize tries reading/writing outside of aram boundary
		if (RDSP32 (ARDMA_ARADDR_H) < ARAM_SIZE)
		{
			size = RDSP32 (ARDMA_CNT_H);
			if (RDSP32 (ARDMA_ARADDR_H) + size > ARAM_SIZE)
				size = ARAM_SIZE - RDSP32 (ARDMA_ARADDR_H);
		
			MEM_COPY_FROM_PTR (RDSP32 (ARDMA_MMADDR_H), ARAM_ADDRESS (RDSP32 (ARDMA_ARADDR_H)), size);
			MEM_SET (RDSP32 (ARDMA_MMADDR_H) + size, 0x05, RDSP32 (ARDMA_CNT_H) - size);
		}
	}
	else
	{
		// transfer to aram
		DEBUG (EVENT_LOG_DSP, ".dsp: write %.8x MEM[%.8x] -> ARAM[%.8x]",
					 RDSP32 (ARDMA_CNT_H), RDSP32 (ARDMA_MMADDR_H) | 0x80000000, RDSP32 (ARDMA_ARADDR_H));

		if (RDSP32 (ARDMA_ARADDR_H) < ARAM_SIZE)
		{
			size = RDSP32 (ARDMA_CNT_H);
			if (RDSP32 (ARDMA_ARADDR_H) + size > ARAM_SIZE)
				size = ARAM_SIZE - RDSP32 (ARDMA_ARADDR_H);

			MEM_COPY_TO_PTR (ARAM_ADDRESS (RDSP32 (ARDMA_ARADDR_H)), RDSP32 (ARDMA_MMADDR_H), size);
		}
	}

	RDSP32 (ARDMA_CNT_H) = 0;

	RDSP16 (DSP_CSR) |= DSP_CSR_ARINT;
	dsp_update_interrupts ();
}


void dsp_w32_arcnt (__u32 addr, __u32 data)
{
	RDSP32 (addr) = data &~ ARCNT_READ;

	dsp_aram_transfer ();
}


// is it safe to assume hi / lo order?
void dsp_w16_arcnt (__u32 addr, __u16 data)
{
	RDSP16 (addr) = data;

	if (addr & 2)
		dsp_aram_transfer ();
}



void dsp_finished (void)
{
	DEBUG (EVENT_LOG_DSP, ".dsp: finished");
	RDSP16 (DSP_DMACR) &= ~DMACR_PLAY;
}


void dsp_reinit (void)
{
}


void dsp_init (void)
{
	memset (rdsp, 0, sizeof (rdsp));
	memset (&dspstate, 0, sizeof (dspstate));


	// control status register
	RDSP16 (DSP_CSR) |= DSP_CSR_HALT;
	mem_hwr_hook (16, DSP_CSR, dsp_r16_direct);
	mem_hww_hook (16, DSP_CSR, dsp_w16_csr);

	// dsp mailbox (to dsp)
	mem_hwr_hook (16, DSP_MBOX_H, dsp_r16_direct);
	mem_hwr_hook (16, DSP_MBOX_L, dsp_r16_direct);
	mem_hww_hook (16, DSP_MBOX_H, dsp_w16_mailbox);
	mem_hww_hook (16, DSP_MBOX_L, dsp_w16_mailbox);

	// cpu mailbox (from dsp)
	mem_hwr_hook (16, DSP_CPU_MBOX_H, dsp_read_mail_hi);
	mem_hwr_hook (16, DSP_CPU_MBOX_L, dsp_read_mail_lo);

	// ARAM size / mode ?
	RDSP16 (0x5012) = 0x43;
	mem_hwr_hook (16, 0x5012, dsp_r16_direct);
	mem_hww_hook (16, 0x5012, dsp_w16_direct);

	// ARAM ready?
	RDSP16 (0x5016) = 1;
	mem_hwr_hook (16, 0x5016, dsp_r16_direct);
	
	mem_hwr_hook (32, ARDMA_MMADDR_H, dsp_r32_direct);
	mem_hww_hook (32, ARDMA_MMADDR_H, dsp_w32_direct);
	mem_hwr_hook (16, ARDMA_MMADDR_H, dsp_r16_direct);
	mem_hww_hook (16, ARDMA_MMADDR_H, dsp_w16_direct);
	mem_hwr_hook (16, ARDMA_MMADDR_L, dsp_r16_direct);
	mem_hww_hook (16, ARDMA_MMADDR_L, dsp_w16_direct);

	mem_hwr_hook (32, ARDMA_ARADDR_H, dsp_r32_direct);
	mem_hww_hook (32, ARDMA_ARADDR_H, dsp_w32_direct);
	mem_hwr_hook (16, ARDMA_ARADDR_H, dsp_r16_direct);
	mem_hww_hook (16, ARDMA_ARADDR_H, dsp_w16_direct);
	mem_hwr_hook (16, ARDMA_ARADDR_L, dsp_r16_direct);
	mem_hww_hook (16, ARDMA_ARADDR_L, dsp_w16_direct);

	// ARDMA control / length
	mem_hwr_hook (32, ARDMA_CNT_H, dsp_r32_direct);
	mem_hww_hook (32, ARDMA_CNT_H, dsp_w32_arcnt);
	mem_hwr_hook (16, ARDMA_CNT_H, dsp_r16_direct);
	mem_hww_hook (16, ARDMA_CNT_H, dsp_w16_arcnt);
	mem_hwr_hook (16, ARDMA_CNT_L, dsp_r16_direct);
	mem_hww_hook (16, ARDMA_CNT_L, dsp_w16_arcnt);
	
	// DMA start address
	mem_hwr_hook (16, DMA_START_H, dsp_r16_direct);
	mem_hww_hook (16, DMA_START_H, dsp_w16_direct);
	mem_hwr_hook (16, DMA_START_L, dsp_r16_direct);
	mem_hww_hook (16, DMA_START_L, dsp_w16_direct);
	
	// DMA control / length
	mem_hwr_hook (16, DMA_CR_LEN, dsp_r16_direct);
	mem_hww_hook (16, DMA_CR_LEN, dsp_w16_dmacr);
	
	// DMA bytes left
	mem_hwr_hook (16, DMA_BLEFT,	dsp_r16_direct);


	memset (rai, 0, sizeof (rai));


	AICR |= AICR_AFR;

	// ai control register
	mem_hwr_hook (32, AI_AICR, ai_r32_direct);
	mem_hww_hook (32, AI_AICR, ai_w32_cr);
	
	// volume
	mem_hwr_hook (32, AI_VR, ai_r32_direct);
	mem_hww_hook (32, AI_VR, ai_w32_aivr);

	// ai sample counter (read only)
	mem_hwr_hook (32, AI_AISCNT, ai_r32_aiscnt);

	// ai interrupt timing
	mem_hwr_hook (32, AI_IT, ai_r32_direct);
	mem_hww_hook (32, AI_IT, ai_w32_direct);

	dsp_mail_init ();
}

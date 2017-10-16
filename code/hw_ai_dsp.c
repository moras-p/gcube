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

__u8 aram[ARAM_SIZE];

DSPState dspstate;

int dspdmago = FALSE;
int yield2_supported = FALSE;
int genaidint = FALSE;
int dspatyield = FALSE;

int aigen = FALSE;

#define DSPDEBUG			0

#define DSP_INIT			0xdcd10000
#define DSP_RESUME		0xdcd10001
#define DSP_YIELD			0xdcd10002
#define DSP_DONE			0xdcd10003
#define DSP_SYNC			0xdcd10004
#define DSP_YIELD2		0xdcd10005

#define DSP_NEW_INTS	1
#define DSP_LOG_RW		0

void dsp_mail_push (__u32 mail)
{
	dspstate.mail_queue[dspstate.mail_queue_top++] = mail;
	
	if (dspstate.mail_queue_top == MAIL_QUEUE_SIZE)
		dspstate.mail_queue_top = 0;

	if (dspstate.mail_queue_top == dspstate.mail_queue_bottom)
		DEBUG (EVENT_EFATAL, ".dsp: mail queue is too small (queue_top reached queue_bottom)");

	DEBUG (EVENT_LOG_DSP, ".dsp: mail push %.8x (%d %d)", mail,
				dspstate.mail_queue_top, dspstate.mail_queue_bottom);

	dspstate.mail_valid = FALSE;
}


__u32 dsp_mail_pop (void)
{
	__u32 mail;
	
	
	if (dspstate.mail_queue_bottom == dspstate.mail_queue_top)
//		mail = 0x8071feed;
		mail = 0;
	else
		mail = dspstate.mail_queue[dspstate.mail_queue_bottom++];
	
	if (dspstate.mail_queue_bottom == MAIL_QUEUE_SIZE)
		dspstate.mail_queue_bottom = 0;

	DEBUG (EVENT_LOG_DSP, ".dsp: mail pop  %.8x", mail);
//	DEBUG (EVENT_STOP, ".dsp: mail pop  %.8x", mail);

	// sync mail is popped off the stack without an int
	if (mail != DSP_SYNC)
	if (dspstate.mail_queue_bottom != dspstate.mail_queue_top)
	// still some mail in there
	{
#if DSP_NEW_INTS
//		dsp_generate_interrupt (DSP_INTERRUPT_DSP);
#endif
		DEBUG (EVENT_LOG_DSP, ".dsp: more mail...");
	}

	return mail;
}


void dsp_mail_reset (void)
{
	dspstate.mail_queue_top = dspstate.mail_queue_bottom = 0;

	dspstate.mail_valid = FALSE;
	dspstate.mail_read = 0;
}


int dsp_mail_empty (void)
{
	return (dspstate.mail_queue_bottom == dspstate.mail_queue_top);
}


void dsp_mail_init (void)
{
	dspstate.mail_queue_top = dspstate.mail_queue_bottom = 0;

	dsp_mail_reset ();

	dsp_mail_push (0); // an error?
	dsp_mail_push (0x80544348);
	dsp_mail_push (0x8071FEED);
}


__u32 dsp_pending_ints[3];


void dsp_update_interrupts (void)
{
	pi_interrupt (INTERRUPT_DSP,
			((DSPCSR & DSP_CSR_DSPINT) && (DSPCSR & DSP_CSR_DSPINTMSK)) ||
			((DSPCSR & DSP_CSR_ARINT) && (DSPCSR & DSP_CSR_ARINTMSK)) ||
			((DSPCSR & DSP_CSR_AIDINT) && (DSPCSR & DSP_CSR_AIDINTMSK)));
}


inline void dsp_assert_interrupt (__u32 mask)
{
	DSPCSR |= mask;

	dsp_update_interrupts ();
DEBUG (EVENT_LOG_DSP, ".dsp: delayed interrupt asserted %.4x", mask);
}


void dsp_generate_interrupt_delayed (__u32 mask, __u32 delay)
{
	if (mask & DSP_INTERRUPT_DSP)
	{
		if (dsp_pending_ints[0])
			dsp_assert_interrupt (DSP_CSR_DSPINT);
		dsp_pending_ints[0] = delay;
DEBUG (EVENT_LOG_DSP, ".dsp: delayed dsp interrupt");
	}

	if (mask & DSP_INTERRUPT_AR)
	{
		if (dsp_pending_ints[1])
			dsp_assert_interrupt (DSP_CSR_ARINT);
		dsp_pending_ints[1] = delay/10;
		cnt_ar++;
		// this might slow it down, as well as so frequend dsp int checks
		// fix that
DEBUG (EVENT_LOG_DSP, ".dsp: delayed ar interrupt");
	}

	if (mask & DSP_INTERRUPT_AID)
	{
		if (dsp_pending_ints[2])
			dsp_assert_interrupt (DSP_CSR_AIDINT);
		dsp_pending_ints[2] = delay;
DEBUG (EVENT_LOG_DSP, ".dsp: delayed aid interrupt");
	}
}


__u32 mail_check_pending = 0;
void dsp_check_mails (void)
{
	if (mail_check_pending)
		if (!--mail_check_pending)
			if (!dsp_mail_empty ())
				dsp_assert_interrupt (DSP_CSR_DSPINT);
}


void dsp_new_mail (void)
{
	mail_check_pending = 0x2ffff;
}


void dsp_check_interrupts (void)
{
#if DSP_NEW_INTS
	if (dsp_pending_ints[0])
		if (!--dsp_pending_ints[0])
			dsp_assert_interrupt (DSP_CSR_DSPINT);

	if (dsp_pending_ints[1])
		if (!--dsp_pending_ints[1])
			dsp_assert_interrupt (DSP_CSR_ARINT);

	if (dsp_pending_ints[2])
		if (!--dsp_pending_ints[2])
			dsp_assert_interrupt (DSP_CSR_AIDINT);

	dsp_check_mails ();
#endif
}

void dsp_update (void)
{
//	dsp_check_interrupts ();

if (aigen)
{
	ai_generate_interrupt ();
	aigen = FALSE;
}

	if (dspdmago)
		dsp_generate_interrupt (DSP_INTERRUPT_AID);

	if (yield2_supported)
	{
		static int yield_cnt = 10;
		
		if (!--yield_cnt)
		{
			yield_cnt = 10;
			dsp_mail_push (DSP_YIELD2);
			dsp_generate_interrupt (DSP_INTERRUPT_DSP);
		}
	}

	if (!dspstate.booted)
		return;

	if (!dsp_mail_empty ())
		dsp_generate_interrupt (DSP_INTERRUPT_DSP);
}


#include "ax.h"

__s16 axbuff[4096];
__u32 ax_outsbuff = 0, ax_pbaddr = 0;


// invalid to pass header address here
// address is nibble address
__s16 dsp_adpcm_decode_sample (__u32 addr, AXPB *pb)
{
	int yn1, yn2, scale, ci, c0, c1, y, i;
	__u8 *adpcm;


	yn1 = BSWAPS16 (pb->adpcm.yn1);
	yn2 = BSWAPS16 (pb->adpcm.yn2);

	adpcm = ARAM_ADDRESS ((addr &~ 15) >> 1);
	i = (addr & 15) - 2;

		scale = 1 << (*adpcm & 15) << 11;
		ci = *adpcm >> 4;
		adpcm++;
		
		adpcm += i / 2;
		
		c0 = BSWAPS16 (pb->adpcm.a[ci][0]);
		c1 = BSWAPS16 (pb->adpcm.a[ci][1]);

			y = EXTS (4, (i & 1) ? (*adpcm++ & 15) : (*adpcm >> 4));
			y = (1024 + c0 * yn1 + c1 * yn2 + scale * y) >> 11;
			y = CLAMP (y, -32768, 32767);
			
			yn2 = yn1;
			yn1 = y;

	pb->adpcm.yn1 = BSWAP16 (yn1);
	pb->adpcm.yn2 = BSWAP16 (yn2);
	
	return y;
}


// generate interrupt only if state changed from run to stop
// needs resampling to be applied when ratio is diff from 1.0
// also multiply ratio by (current_dsp_freq / current_sdl_freq)
void ax_process (__u32 pbs)
{
	int i, j, k, sample, left, right;
	float fleft, fright;
	__u32 pbaddr, pos, lstart, lend, frac, ratio;
	__s16 *buff = (__s16 *) MEM_ADDRESS (ax_outsbuff);


	// size assumed to be 640, 2 is for stereo
	// 160 samples, stereo * sizeof (s16) * num_samples = 640
	for (j = 0; j < AX_SAMPLES_PER_FRAME; j++)
	{
		pbaddr = pbs;
		left = right = 0;
		fleft = fright = 0;

		// 64 voices
		for (i = 0; i < AX_MAX_VOICES; i++)
		{
			AXPB *pb = (AXPB *) MEM_ADDRESS (pbaddr);

			pbaddr = MEMR32 (pbaddr);

			if (BSWAP16 (pb->state) != AXPB_STATE_RUN)
				continue;

			pos = (BSWAP16 (pb->addr.cur_address_hi) << 16) | BSWAP16 (pb->addr.cur_address_lo);
			lstart = (BSWAP16 (pb->addr.loop_address_hi) << 16) | BSWAP16 (pb->addr.loop_address_lo);
			lend = (BSWAP16 (pb->addr.end_address_hi) << 16) | BSWAP16 (pb->addr.end_address_lo);

			frac = BSWAP16 (pb->src.cur_address_frac);
			ratio = ((BSWAP16 (pb->src.ratio_hi) << 16) | BSWAP16 (pb->src.ratio_lo));
			// ratio can be invalid in ax demos, set it to 1.0
			if (!ratio)
				ratio = 0x10000;

			switch (BSWAP16 (pb->addr.format))
			{
				case AXPB_FORMAT_PCM8:
					// 8 bit addressing
					sample = ARAM_R8S (pos) << 8;
					break;
				
				case AXPB_FORMAT_PCM16:
					// 16 bit addressing
					sample = ARAM_R16S (pos << 1);
					break;
				
				case AXPB_FORMAT_ADPCM:
					// 4 bit addressing

					// skip header
					if ((pos & 15) < 2)
						pos += 2 - (pos & 15);

					switch ((ratio + frac) >> 16)
					{
						case 0:
							sample = BSWAPS16 (pb->src.last_samples[3]);
							break;
						
						case 1:
							sample = dsp_adpcm_decode_sample (pos, pb);
							break;
						
						case 2:
							sample = (float) 0.5 * dsp_adpcm_decode_sample (pos, pb) +
				         0.5 * dsp_adpcm_decode_sample (pos + ((((pos + 1) & 15) == 0) ? 3 : 1), pb);
							break;

						default:
							printf ("big jump decode sample %d\n", pos);
					}

//					sample = dsp_adpcm_decode_sample (pos, pb);
					break;

				default:
					sample = 0;
			}

			for (k = 0; k < 3; k++)
				pb->src.last_samples[k] = pb->src.last_samples[k + 1];
			pb->src.last_samples[3] = BSWAP16 (sample);

			sample = sample * BSWAP16 (pb->ve.cur_vol) / 0x7fff;
			
			left += sample * BSWAPS16 (pb->mix.left) >> 15;
			right += sample * BSWAPS16 (pb->mix.right) >> 15;

			pos += (ratio + frac) >> 16;
			if (pos >= lend)
			{
				if (pb->addr.is_looping)
					pos = lstart;
				else
					pb->state = BSWAP16 (AXPB_STATE_STOP);
			}

			if (j == 0)
			DEBUG (EVENT_LOG_DSP, "      V[%2.2d] %.4x/%.4x addr %.8x/%.8x/%.8x v %.4x/%.4x",
								 i, pb->src_type, pb->addr.format,
								 (BSWAP16 (pb->addr.cur_address_hi) << 16) | BSWAP16 (pb->addr.cur_address_lo),
								 (BSWAP16 (pb->addr.loop_address_hi) << 16) | BSWAP16 (pb->addr.loop_address_lo),
								 (BSWAP16 (pb->addr.end_address_hi) << 16) | BSWAP16 (pb->addr.end_address_lo),
								 BSWAP16 (pb->ve.cur_vol), BSWAP16 (pb->ve.cur_delta));

			pb->src.cur_address_frac = BSWAP16 ((__u16) (frac + ratio));

			// volume change
			pb->ve.cur_vol = BSWAP16 (BSWAP16 (pb->ve.cur_vol) + BSWAPS16 (pb->ve.cur_delta));

			pb->addr.cur_address_hi = BSWAP16 (pos >> 16);
			pb->addr.cur_address_lo = BSWAP16 ((__u16) pos);
		}

		*buff++ = BSWAP16 (left);
		*buff++ = BSWAP16 (right);
	}
}


int ax_parse_cmd (__u32 addr)
{
	switch (MEMR16 (addr))
	{
		case 0x0000:
//			DEBUG (EVENT_LOG_DSP, "      studio address: %.8x", MEMR32 (addr + 2));
			return 6;

		case 0x0002:
//			DEBUG (EVENT_LOG_DSP, "      PB address: %.8x", MEMR32 (addr + 2));
			ax_pbaddr = MEMR32 (addr + 2);
			return 6;

		case 0x0007:
//			DEBUG (EVENT_LOG_DSP, "      OutSBuffer address: %.8x", MEMR32 (addr + 2));
			return 6;

		case 0x000a:
//			DEBUG (EVENT_LOG_DSP, "      CompressorTable address: %.8x", MEMR32 (addr + 2));
			return 6;

		case 0x000e:
//			DEBUG (EVENT_LOG_DSP, "      OutSBuffer2 addresses: %.8x %.8x", MEMR32 (addr + 2), MEMR32 (addr + 6));
			ax_outsbuff = MEMR32 (addr + 6);
			return 10;

		case 0x000f:
//			DEBUG (EVENT_LOG_DSP, "      end of list");
			ax_process (ax_pbaddr);
			return 0;
		
		default:
//			DEBUG (EVENT_LOG_DSP, "      unknown command %.4x", MEMR16 (addr));
			return 2;
	}
}


void ax_list (__u32 addr)
{
	int n;


//	DEBUG (EVENT_LOG_DSP, ".dsp: processing ax task %.8x", addr);

	while ((n = ax_parse_cmd (addr)))
		addr += n;

//	RDSP16 (DSP_DMACR) &= DMACR_PLAY;
//	RDSP16 (DSP_DMACR) = 0;
}



// decode mail
// this still causes problems
// pokemon colosseum battle mode stops at loading and need
// 'int int_dsp_dsp' from debugger to go further
void dsp_send_mail (__u32 mail)
{
	static __u32 buff[256];
	static int nparams = 0, step = 0, resume = FALSE, babe_mail = FALSE;
	static int test = FALSE, dspinit = FALSE, dspyield = FALSE;


	dspstate.mail_valid = FALSE;

	DEBUG (EVENT_LOG_DSP, ".dsp: got mail  %.8x", mail);

	if (nparams == 0)
	{
		if (mail == 0)
		{
//!!			DEBUG (EVENT_LOG_DSP, ".dsp: 0 mail, not good, assuming two more mails");
//!!			nparams = 2; // 2 more params for dsp release halt
	
//			dsp_new_mail ();	
//			dsp_generate_interrupt (DSP_INTERRUPT_DSP);
//		if (!dsp_pending_ints[0])
//			dsp_generate_interrupt_delayed (DSP_INTERRUPT_DSP, 0x2fffff);

//			nparams = 9; // 9 more params for exec_task
		}
		else if (mail == 0xff000000)
		{
			buff[step++] = mail;
			nparams = 1;
		}
		else if ((mail >> 16) == 0)
		{
			DEBUG (EVENT_LOG_DSP, ".dsp: mail -> incoming command with %d params", mail);
			nparams = mail;

			resume = TRUE;
		}
		else if ((mail >> 16) == 0xBABE)
		{
			DEBUG (EVENT_LOG_DSP, ".dsp: mail -> 0xBABE %.4x", mail & 0xffff);
			nparams = 1;
//			resume = TRUE;

			babe_mail = TRUE;
		}
		else if ((mail >> 16) == 0xcdd1)
		{
			if ((mail & 0xffff) == 0x0001)
			{
				// exec task incoming
				nparams = 10;
				dspinit = TRUE;
				dspatyield = !dspatyield;
			}
			// cdd10003
			else if ((mail & 0xffff) == 0x0003)
			{
				nparams = 0;
				dspyield = TRUE;
			}
			else if ((mail & 0xffff) == 0x0002)
			{
				nparams = 0;

//				dsp_mail_push (DSP_YIELD);
//DEBUG (EVENT_STOP, "break here");
//				dsp_mail_push (0x8071feed);
			}
			else
			{
				// 10 params incoming
				nparams = 10;
				test = TRUE;
			}
		}
		else switch (mail & 0xffff)
		{
			// mail size not given. check command and determine size.
			case 0:
				DEBUG (EVENT_LOG_DSP, ".dsp: mail -> release halt");
				dsp_mail_push (DSP_RESUME);
//!!disable this
//				dsp_generate_interrupt (DSP_INTERRUPT_DSP);
			dsp_new_mail ();	

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
				DEBUG (EVENT_LOG_DSP, ".dsp: mail -> DSyncFrame");
				buff[step++] = mail;
				nparams = 2;
				break;

			// UpdateDSPChannel
			case 0x2000:
			case 0x4000:
				DEBUG (EVENT_LOG_DSP, ".dsp: mail -> UpdateDSPChannel");
				buff[step++] = mail;
				nparams = 2;

				resume = TRUE;
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
				DEBUG (EVENT_LOG_DSP, ".dsp: unknown mail %.8x %.4x", mail, mail & 0xffff);
		}
	}
	else if (nparams < 0)
	{
		if ((buff[0] & 0xffff) == 0xd001)
		{
			// start vector set. boot task.
#if !DSP_NEW_INTS
			dspstate.booted = TRUE;
#endif
			dsp_mail_reset ();
			dsp_mail_push (DSP_INIT);
			// this is needed
//			dsp_mail_push (DSP_RESUME);

//			dsp_mail_push (DSP_DONE);
#if DSP_NEW_INTS
//				dsp_generate_interrupt (DSP_INTERRUPT_DSP);
			dsp_new_mail ();	

#endif
#if DSPDEBUG
DEBUG (EVENT_STOP, "init done");
#endif
		}
		nparams = 0;
	}
	else
	{
		buff[step++] = mail;
		if (!--nparams)
		{
			step = 0;

			if (buff[0] == 0xff000000)
			{
				DEBUG (EVENT_LOG_DSP, ".dsp: memcard unlock");
				dsp_mail_push (DSP_DONE);
//				dsp_generate_interrupt (DSP_INTERRUPT_DSP);
			dsp_new_mail ();	

			}
			else if (((buff[0] & 0xffff) == 0x0040) || // setup table
							 ((buff[0] & 0xffff) == 0x3800)) // sync frame
			{
//!!	yield2_supported = TRUE;
				dsp_mail_push (DSP_SYNC);
				dsp_mail_push (0xf3550000 | (buff[0] >> 16));
//				dsp_mail_push (DSP_RESUME);
//!! disable this
//				dsp_generate_interrupt (DSP_INTERRUPT_DSP);
			dsp_new_mail ();	

			}
			else if (babe_mail)
			{
				ax_list (buff[0]);
				babe_mail = FALSE;
//				dsp_mail_push (DSP_RESUME);
				dsp_mail_push (DSP_YIELD); // to switch to next task if present
//!				dsp_generate_interrupt (DSP_INTERRUPT_AID);
//				genaidint = TRUE;
//				dsp_generate_interrupt (DSP_INTERRUPT_DSP);
			dsp_new_mail ();	

			}
			else if (resume)
			{
				dsp_mail_push (DSP_RESUME);
//!!disable this
//				dsp_generate_interrupt (DSP_INTERRUPT_DSP);
			dsp_new_mail ();	

			}
			else if (dspinit)
			{
				if (buff[0])
				dsp_mail_push (DSP_INIT);
				else
				dsp_mail_push (DSP_RESUME);
//				dsp_generate_interrupt (DSP_INTERRUPT_DSP);
			dsp_new_mail ();	

			}
			else if (dspyield)
			{
				dsp_mail_push (DSP_RESUME);
//				dsp_generate_interrupt (DSP_INTERRUPT_DSP);
			dsp_new_mail ();	

			}
			else
			{
/*
DEBUG (EVENT_STOP, "shouldn't reach here");
				dsp_mail_push (DSP_SYNC);
				dsp_mail_push (0xf3550000 | (buff[0] >> 16));
*/
			}

		
			resume = FALSE;
			dspinit = FALSE;
			dspyield = FALSE;
		}
	}
}


__u32 dsp_read_mail (void)
{
	dspstate.mail_valid = TRUE;

	return dsp_mail_pop ();
}


// sunshine read mail from dsp -> hi/lo

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
	pi_interrupt (INTERRUPT_AI, (AICR & AICR_AIINT) && (AICR & AICR_AIINTMSK));
}


void ai_generate_interrupt (void)
{
	AICR |= AICR_AIINT;

	ai_update_interrupts ();
}


void ai_counter_inc (__u32 samples)
{
	if (!(AICR & AICR_AIINTVLD))
		if ((AISCNT < AIIT) && ((AISCNT + samples/2) >= AIIT))
		{
//			ai_generate_interrupt ();
			aigen = TRUE;
//			printf ("ai interrupt\n");
		}
	
	AISCNT += samples/2;
}


void ai_w32_cr (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_AI, "..ai: AICR | PSTAT %d | SCRESET %d | FREQ %d/%d kHz | AIINT %d/%d/%d ack %d",
				 data & AICR_PSTAT, (data & AICR_SCRESET)>0,
				 (data & AICR_AFR) ? 48 : 32, (data & AICR_DSPRATE) ? 48 : 32,
				 (data & AICR_AIINTMSK)>0, (data & AICR_AIINTVLD)>0,
				 (AICR & AICR_AIINT)>0, (data & AICR_AIINT)>0);

	if ((AICR & AICR_PSTAT) != (data & AICR_PSTAT))
	{
		DEBUG (EVENT_LOG_AI, "....  streaming %s", (data & AICR_PSTAT) ? "started" : "stopped");
		if (data & AICR_PSTAT)
			adp_stream_start ();
		else
			adp_stream_stop ();
	}

	if (data & AICR_SCRESET)
	{
		AISCNT = 0;
		DEBUG (EVENT_LOG_AI, "....  resetting sample counter");
	}

	if (data & AICR_AIINT)
		RAI32 (addr) &= ~AICR_AIINT;

	RAI32 (addr) = (RAI32 (addr) &~ (AICR_PSTAT | AICR_AFR | AICR_AIINTMSK | AICR_AIINTVLD | AICR_DSPRATE)) |
								 (data & (AICR_PSTAT | AICR_AFR | AICR_AIINTMSK | AICR_AIINTVLD | AICR_DSPRATE));

//	audio_set_freq ((AICR & AICR_AFR) ? FREQ_48KHZ : FREQ_32KHZ);
	audio_set_freq ((AICR & AICR_DSPRATE) ? FREQ_32KHZ : FREQ_48KHZ);
	
	ai_update_interrupts ();
}


// sample counter
__u32 ai_r32_aiscnt (__u32 addr)
{
	DEBUG (EVENT_LOG_AI, "..ai: AISCNT read %.8x", RAI32 (addr));
/*
	// fake it
	if (AICR & AICR_PSTAT)
		RAI32 (addr)++;
*/
	return RAI32 (addr);
}


void ai_w32_aivr (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_AI, "..ai: VOLUME %d / %d", data & 0x00ff, (data & 0xff00) >> 8);
	RAI32 (addr) = data;
}


__u16 dsp_r16_direct (__u32 addr)
{
#if DSP_LOG_RW
	DEBUG (EVENT_LOG_DSP, ".dsp: read  [%.4x] (%.4x)", addr & 0xffff, RDSP16 (addr));
#endif

	return RDSP16 (addr);
}


void dsp_w16_direct (__u32 addr, __u16 data)
{
#if DSP_LOG_RW
	DEBUG (EVENT_LOG_DSP, ".dsp: write [%.4x] (%.4x) = %.4x", addr & 0xffff, RDSP16 (addr), data);
#endif
	RDSP16 (addr) = data;
}


__u32 dsp_r32_direct (__u32 addr)
{
#if DSP_LOG_RW
	DEBUG (EVENT_LOG_DSP, ".dsp: read  [%.4x] (%.8x)", addr & 0xffff, RDSP32 (addr));
#endif

	return RDSP32 (addr);
}


void dsp_w32_direct (__u32 addr, __u32 data)
{
#if DSP_LOG_RW
	DEBUG (EVENT_LOG_DSP, ".dsp: write [%.4x] (%.8x) = %.8x", addr & 0xffff, RDSP32 (addr), data);
#endif
	RDSP32 (addr) = data;
}


void dsp_generate_interrupt (__u32 mask)
{
#if !DSP_NEW_INTS
	if (mask & DSP_INTERRUPT_DSP)
		DSPCSR |= DSP_CSR_DSPINT;

	if (mask & DSP_INTERRUPT_AR)
		DSPCSR |= DSP_CSR_ARINT;

	if (mask & DSP_INTERRUPT_AID)
		DSPCSR |= DSP_CSR_AIDINT;
#else
	dsp_generate_interrupt_delayed (mask, 0x2ffff);
#endif

	dsp_update_interrupts ();
}


// dsp has two more ints: dsp_dma_int and piint

void dsp_w16_csr (__u32 addr, __u16 data)
{
	// int order: DSP AR AID
	DEBUG (EVENT_LOG_DSP, ".dsp: (%.8x) CSR = %.4x | INT %d%d%d/%d%d%d ack %d%d%d + %d%d |%s%s%s", PC, data,
				 (RDSP16 (DSP_CSR) & DSP_CSR_DSPINT) > 0,
				 (RDSP16 (DSP_CSR) & DSP_CSR_ARINT) > 0,
				 (RDSP16 (DSP_CSR) & DSP_CSR_AIDINT) > 0,
				 (data & DSP_CSR_DSPINTMSK) > 0,
				 (data & DSP_CSR_ARINTMSK) > 0,
				 (data & DSP_CSR_AIDINTMSK) > 0,
				 (data & DSP_CSR_DSPINT) > 0,
				 (data & DSP_CSR_ARINT) > 0,
				 (data & DSP_CSR_AIDINT) > 0,
				 (data & DSP_CSR_DMAINTST) > 0,
				 (data & DSP_CSR_PIINT) > 0,
				 (data & DSP_CSR_DSPINIT) ? " init" : "",
				 (data & DSP_CSR_RES) ? " reset" : "",
				 (data & DSP_CSR_HALT) ? " halt" : "");

//	if ((data & DSP_CSR_AIDINT) && (RDSP16 (DSP_CSR) & DSP_CSR_AIDINT))
//		DEBUG (EVENT_STOP, "ack aid interrupt");

	if (data & DSP_CSR_PIINT)
	{
//				dsp_assert_interrupt (DSP_CSR_DSPINT);
/*
		if (mail_check_pending)
		{
			mail_check_pending = 1;
			dsp_check_mails ();
		}
*/
//		data &= ~DSP_CSR_PIINT;
	}

	if (data & DSP_CSR_DSPINT)
		RDSP16 (DSP_CSR) &= ~DSP_CSR_DSPINT;

	if (data & DSP_CSR_ARINT)
		RDSP16 (DSP_CSR) &= ~DSP_CSR_ARINT;

	if (data & DSP_CSR_AIDINT)
	{
		RDSP16 (DSP_CSR) &= ~DSP_CSR_AIDINT;

		if (!dsp_mail_empty ())
		{
//		if (dspatyield)
//			dsp_mail_push (DSP_RESUME);
//			dsp_generate_interrupt (DSP_INTERRUPT_DSP);
		}
	}

	if (data & DSP_CSR_RES)
	{
		dsp_pending_ints[0] = 0;
		dsp_pending_ints[1] = 0;
		dsp_pending_ints[2] = 0;
	}

	// copy all but dsp_reset and dsp_dma_int_status flags
	RDSP16 (DSP_CSR) = (RDSP16 (DSP_CSR) & (DSP_CSR_DSPINT | DSP_CSR_ARINT | DSP_CSR_AIDINT)) |
		 (data &~ (DSP_CSR_DSPINT | DSP_CSR_ARINT | DSP_CSR_AIDINT | DSP_CSR_RES | DSP_CSR_DMAINTST));

	dsp_update_interrupts ();
}


void audio_cancel_last (void);
void ai_dma (int start)
{
	// this should start a delayed transaction
	// if its called again, before the transaction finishes, it justs
	// sets new parameters, and not asserts a new interrupt

	if (dsp_pending_ints[2])
	{
		audio_cancel_last ();
		audio_play ((__u8 *) MEM_ADDRESS (RDSP32 (DSP_DMA_START_H)),
								(RDSP16 (DSP_DMACR) & 0x7fff) << 5);
		return;
	}

	audio_play ((__u8 *) MEM_ADDRESS (RDSP32 (DSP_DMA_START_H)),
							(RDSP16 (DSP_DMACR) & 0x7fff) << 5);
	RDSP16 (DMA_BLEFT) = 0;

	if (start)
	{
//	dsp_generate_interrupt_delayed (DSP_INTERRUPT_AID, 0x29000);
//	dspdmago = TRUE;
//	dsp_generate_interrupt_delayed (DSP_INTERRUPT_AID, 1);
	dsp_generate_interrupt_delayed (DSP_INTERRUPT_AID, 0x2ffff*23);
	}
	else
	dsp_generate_interrupt (DSP_INTERRUPT_AID);
}


void dsp_w16_dmacr (__u32 addr, __u16 data)
{
	if ((!(RDSP16 (DSP_DMACR) & DMACR_DMA_EN)) && (data & DMACR_DMA_EN))
	{
		DEBUG (EVENT_LOG_DSP, ".dsp: DMA start %.8x, length %d",
					 RDSP32 (DSP_DMA_START_H) | 0x80000000, (data & 0x7fff) << 5);

		ai_dma (1);
	}
	else if ((RDSP16 (DSP_DMACR) & DMACR_DMA_EN) && (!(data & DMACR_DMA_EN)))
		DEBUG (EVENT_LOG_DSP, ".dsp: DMA stop");
	else if (!(RDSP16 (DSP_DMACR) & DMACR_DMA_EN))
		DEBUG (EVENT_LOG_DSP, ".dsp: DMA init %.8x, length %d",
					 RDSP32 (DSP_DMA_START_H) | 0x80000000, (data & 0x7fff) << 5);
	else
	{
		DEBUG (EVENT_LOG_DSP, ".dsp: DMA next %.8x, length %d",
					 RDSP32 (DSP_DMA_START_H) | 0x80000000, (data & 0x7fff) << 5);

		ai_dma (0);
	}

	RDSP16 (addr) = data;
}


void dsp_w16_dmacrx (__u32 addr, __u16 data)
{
	// bleft is the number of bytes remaining in the current DMA
	RDSP16 (DMA_BLEFT) = (RDSP16 (DSP_DMACR) & 0x7fff) << 5;
	// this is the AI-FIFO DMA_Enable flag
	// the rest is num of bytes programmed for the next DMA transaction
	// AIDINT is the AI-FIFO DMA Interrupt
	// so:
	// - first AIInitDMA is called to set the parameters of next dma
	// - next AIStartDMA is called
	//   current parameters are taken, and dma transfer is started
	//   aidint is asserted to inform that the params have been accepted
	//   the interrupt handler is responsible to set up new parameters
	//   whenever new params are accepted, aid int is asserted
	// ! if AIStartDMA is called when transaction is in progress,
	//   will cause to start a new dma transaction with new params
	if (data & DMACR_PLAY)
	{
//		DEBUG (EVENT_STOP, ".dsp: DMACR start %.8x length %d",
		DEBUG (EVENT_LOG_DSP, ".dsp: DMACR start %.8x length %d",
					 RDSP32 (DSP_DMA_START_H) | 0x80000000,
					 (data & 0x7fff) << 5);
#if ENABLE_SOUND

//		if (((RDSP32 (DSP_DMA_START_H) & 0x0fffffff) == 0x41440) ||
//				((RDSP32 (DSP_DMA_START_H) & 0x0fffffff) == 0x416c0))
		if (data & 0x7fff)
		{
		audio_play ((__u8 *) MEM_ADDRESS (RDSP32 (DSP_DMA_START_H)),
								(data & 0x7fff) << 5);
		
			// instant dma
//			data &= ~0x7fff;
			data = 0;
			RDSP16 (DMA_BLEFT) = 0;
		}

#endif

	// change from stop to start
		if (!(RDSP16 (addr) & DMACR_PLAY))
		{
//			dsp_generate_interrupt (DSP_INTERRUPT_AID);
//			dspdmago = TRUE;
		}
	}
	else
	{
#if ENABLE_SOUND
//		audio_stop ();
#endif
		DEBUG (EVENT_LOG_DSP, ".dsp: DMACR stop  %.8x length %d", RDSP32 (DSP_DMA_START_H) | 0x80000000, (data & 0x7fff) << 5);
	}

/*
	if (data & 0x7fff)
		audio_play (MEM_ADDRESS (RDSP32 (DSP_DMA_START_H)), (data & 0x7fff) << 5);
*/
	RDSP16 (addr) = data;

/*
#if !ENABLE_SOUND
	// fake it
	RDSP16 (DMA_BLEFT) = 0;
	RDSP16 (DSP_DMACR) &= ~DMACR_PLAY;
#endif
*/
}


void dsp_aram_transfer (void)
{
	__u32 size;


	if (RDSP32 (ARDMA_CNT_H) & ARCNT_READ)
	{
		RDSP32 (ARDMA_CNT_H) &= ~ARCNT_READ;

		// transfer from aram
		DEBUG (EVENT_LOG_DSP, ".dsp: read %.8x MEM[%.8x] <- ARAM[%.8x]%s",
					 RDSP32 (ARDMA_CNT_H), RDSP32 (ARDMA_MMADDR_H) | 0x80000000, RDSP32 (ARDMA_ARADDR_H),
					 (RDSP32 (ARDMA_ARADDR_H) >= ARAM_SIZE) ? " | out of bounds" : "");

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
		DEBUG (EVENT_LOG_DSP, ".dsp: write %.8x MEM[%.8x] -> ARAM[%.8x]%s",
					 RDSP32 (ARDMA_CNT_H), RDSP32 (ARDMA_MMADDR_H) | 0x80000000, RDSP32 (ARDMA_ARADDR_H),
					 (RDSP32 (ARDMA_ARADDR_H) >= ARAM_SIZE) ? " | out of bounds" : "");

		if (RDSP32 (ARDMA_ARADDR_H) < ARAM_SIZE)
		{
			size = RDSP32 (ARDMA_CNT_H);
			if (RDSP32 (ARDMA_ARADDR_H) + size > ARAM_SIZE)
				size = ARAM_SIZE - RDSP32 (ARDMA_ARADDR_H);

			MEM_COPY_TO_PTR (ARAM_ADDRESS (RDSP32 (ARDMA_ARADDR_H)), RDSP32 (ARDMA_MMADDR_H), size);
		}
	}

	RDSP32 (ARDMA_CNT_H) = 0;

	dsp_generate_interrupt (DSP_INTERRUPT_AR);
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

	// sampling rate
	AICR |= AICR_AFR;
	AICR |= AICR_DSPRATE;

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

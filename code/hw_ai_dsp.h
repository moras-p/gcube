#ifndef __HW_DSP_H
#define __HW_DSP_H 1


#include "general.h"
#include "mem.h"

#define RDSP_SIZE					0x200
#define RDSP_MASK					0x1ff

//#define ARAM_SIZE					0x02000000
//#define ARAM_MASK					0x01ffffff
#define ARAM_SIZE					0x01000000
#define	ARAM_MASK					0x00ffffff

#define ARAM_ADDRESS(X)		(&aram [X])

#ifdef LIL_ENDIAN
# define RDSP16(X)					(*((__u16 *) &rdsp[RDSP_SIZE - ((X) & RDSP_MASK) - 2]))
# define RDSP32(X)					(*((__u32 *) &rdsp[RDSP_SIZE - ((X) & RDSP_MASK) - 4]))
#else
# define RDSP16(X)					(*((__u16 *) &rdsp[(X) & RDSP_MASK]))
# define RDSP32(X)					(*((__u32 *) &rdsp[(X) & RDSP_MASK]))
#endif

#define RAI_SIZE					0x200
#define RAI_MASK					0x1ff

#ifdef LIL_ENDIAN
# define RAI32(X)					(*((__u32 *) &rai[RAI_SIZE - ((X) & RAI_MASK) - 4]))
#else
# define RAI32(X)					(*((__u32 *) &rai[(X) & RAI_MASK]))
#endif


#define ARAM(X)						(aram[(X) & ARAM_MASK])
#define ARAM_R8(X)				(ARAM(X))
#define ARAM_R8S(X)				((__s8) ARAM(X))
#define ARAM_R16(X)				(BSWAP16 (*((__u16 *) &ARAM (X))))
#define ARAM_R16S(X)			((__s16) ARAM_R16 (X))

extern __u8 rdsp[RDSP_SIZE];
extern __u8 rai[RAI_SIZE];
extern __u8 aram[ARAM_SIZE];


#define DSP_CSR							0x500a
#define DSPCSR							(RDSP16 (DSP_CSR))
#define DSP_CSR_RES					(1 << 0)
#define DSP_CSR_PIINT				(1 << 1)
#define DSP_CSR_HALT				(1 << 2)
#define DSP_CSR_AIDINT			(1 << 3)
#define DSP_CSR_AIDINTMSK		(1 << 4)
#define DSP_CSR_ARINT				(1 << 5)
#define DSP_CSR_ARINTMSK		(1 << 6)
#define DSP_CSR_DSPINT			(1 << 7)
#define DSP_CSR_DSPINTMSK		(1 << 8)
#define DSP_CSR_DMAINTST		(1 << 9)
#define DSP_CSR_DSPINIT			(1 << 11)

#define AI_AICR							0x6c00
#define AICR								(RAI32 (AI_AICR))
#define AICR_DSPRATE				(1 << 6)
#define AICR_SCRESET				(1 << 5)
#define AICR_AIINTVLD				(1 << 4)
#define AICR_AIINT					(1 << 3)
#define AICR_AIINTMSK				(1 << 2)
#define AICR_AFR						(1 << 1)
#define AICR_PSTAT					(1 << 0)

#define AI_STREAMING				(AICR & AICR_PSTAT)

// volume
#define AI_VR								0x6c04

#define AI_AISCNT						0x6c08
#define AISCNT							(RAI32 (AI_AISCNT))
#define AI_IT								0x6c0c
#define AIIT								(RAI32 (AI_IT))

#define ARCNT_READ					0x80000000

#define ARDMA_MMADDR_H			0x5020
#define ARDMA_MMADDR_L			0x5022
#define ARDMA_ARADDR_H			0x5024
#define ARDMA_ARADDR_L			0x5026

#define DSP_DMA_START_H			0x5030
#define DSP_DMA_START_L			0x5032
#define DSP_DMACR						0x5036
#define DSP_DMA_BLOCKS_LEFT	0x503a
#define DMACR_PLAY					(0x8000)
#define DMACR_DMA_EN				(0x8000)

// len >> 5
#define STREAM_BLOCKS_LEFT		(RDSP16 (DSP_DMA_BLOCKS_LEFT))

// to dsp / from dsp
#define DSP_MBOX_H						0x5000
#define DSP_MBOX_L						0x5002
#define DSP_CPU_MBOX_H				0x5004
#define DSP_CPU_MBOX_L				0x5006

#define ARDMA_CNT_H						0x5028
#define ARDMA_CNT_L						0x502a

#define DMA_START_H						0x5030
#define DMA_START_L						0x5032

#define DMA_CR_LEN						0x5036
#define DMA_BLEFT							0x503a


#define DSP_INTERRUPT_DSP			(1 << 0)
#define DSP_INTERRUPT_AR			(1 << 1)
#define DSP_INTERRUPT_AID			(1 << 2)

#define MAIL_QUEUE_SIZE				(1024*1000)

typedef struct
{
	int mail_valid;
	__u32 mail_read;
	int booted;
	__u32 mail_queue[MAIL_QUEUE_SIZE];
	int mail_queue_top, mail_queue_bottom;
} DSPState;

extern DSPState dspstate;


void dsp_init (void);
void dsp_reinit (void);
void dsp_finished (void);

void ai_generate_interrupt (void);
void dsp_generate_interrupt (__u32 mask);

void dsp_mail_push (__u32 mail);
void dsp_update (void);

void ai_counter_inc (__u32 samples);

void dsp_check_interrupts (void);


#endif // __HW_DSP_H

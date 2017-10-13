#ifndef __HW_CP_H
#define __HW_CP_H 1


#include "general.h"
#include "mem.h"

#define RCP_SIZE					0x100
#define RCP_MASK					0x0ff

//#define RCP16(X)					(*((__u16 *) &rcp[RCP_SIZE - (X & RCP_MASK) - 2]))
//#define RCP32(X)					(*((__u32 *) &rcp[RCP_SIZE - (X & RCP_MASK) - 4]))
#define RCP16(X)					(*((__u16 *) &rcp[(X) & RCP_MASK]))
#define RCP32(X)					(*((__u32 *) &rcp[(X) & RCP_MASK]))

extern __u8 rcp[RCP_SIZE];


#define CP_SR							(0x0000)
#define CP_CR							(0x0002)
#define CP_CLEAR					(0x0004)
#define CP_TOKEN					(0x000e)
// bounding box
#define CP_BB_LEFT				(0x0010)
#define CP_BB_RIGHT				(0x0012)
#define CP_BB_TOP 				(0x0014)
#define CP_BB_BOTTOM			(0x0016)
#define CP_FIFO_BASE_LO		(0x0020)
#define CP_FIFO_BASE_HI		(0x0022)
#define CP_FIFO_END_LO		(0x0024)
#define CP_FIFO_END_HI		(0x0026)
// watermark
#define CP_FIFO_HWMARK_LO	(0x0028)
#define CP_FIFO_HWMARK_HI	(0x002a)
#define CP_FIFO_LWMARK_LO	(0x002c)
#define CP_FIFO_LWMARK_HI	(0x002e)

#define CP_FIFO_RW_DIST_LO	(0x0030)
#define CP_FIFO_RW_DIST_HI	(0x0032)
#define CP_FIFO_WPOINTER_LO	(0x0034)
#define CP_FIFO_WPOINTER_HI	(0x0036)
#define CP_FIFO_RPOINTER_LO	(0x0038)
#define CP_FIFO_RPOINTER_HI	(0x003a)

#define CP_FIFO_BP_LO				(0x003c)
#define CP_FIFO_BP_HI				(0x003e)

#define CP_FIFO_BASE				(RCP32 (CP_FIFO_BASE_LO))
#define CP_FIFO_END					(RCP32 (CP_FIFO_END_LO))
#define CP_FIFO_WPOINTER		(RCP32 (CP_FIFO_WPOINTER_LO))
#define CP_FIFO_RPOINTER		(RCP32 (CP_FIFO_RPOINTER_LO))
#define CP_FIFO_RW_DIST			(RCP32 (CP_FIFO_RW_DIST_LO))

#define CP_FIFO_BREAKPOINT	(RCP32 (CP_FIFO_BP_LO))
#define CP_FIFO_HI_WMARK		(RCP32 (CP_FIFO_HWMARK_LO))
#define CP_FIFO_LO_WMARK		(RCP32 (CP_FIFO_LWMARK_LO))


#define CP_SR_BPINT					(1 << 4)
#define CP_SR_IDLE					(1 << 3)
#define CP_SR_READY					(1 << 2)
#define CP_SR_UFINT					(1 << 1)
#define CP_SR_OFINT					(1 << 0)

#define CPSR								(RCP16 (CP_SR))
#define CPCR								(RCP16 (CP_CR))

#define CP_CR_BPINTMSK			(1 << 5)
#define CP_CR_GPLINK				(1 << 4)
#define CP_CR_UFINTMSK			(1 << 3)
#define CP_CR_OFINTMSK			(1 << 2)
#define CP_CR_CLEAR_BP			(1 << 1)
#define CP_CR_RDEN					(1 << 0)

#define CP_CLEAR_UF					(1 << 1)
#define CP_CLEAR_OF					(1 << 0)


void cp_init (void);
void cp_reinit (void);

void cp_wpar_redirect (__u32 address);


#endif // __HW_CP_H

#ifndef __HW_EXI_H
#define __HW_EXI_H 1


#include "general.h"
#include "mem.h"


#define REXI_SIZE						0x100
#define REXI_MASK						0x0ff

#define REXI32(X)						(*((__u32 *)(&rexi[(X) & REXI_MASK])))

extern __u8 rexi[REXI_SIZE];


#define EXI_BASE						0x6800
#define EXI_0CSR						(EXI_BASE + 0x00)
#define EXI_0MAR						(EXI_BASE + 0x04)
#define EXI_0LENGTH					(EXI_BASE + 0x08)
#define EXI_0CR							(EXI_BASE + 0x0c)
#define EXI_0DATA						(EXI_BASE + 0x10)
#define EXI_1CSR						(EXI_BASE + 0x14)
#define EXI_1MAR						(EXI_BASE + 0x18)
#define EXI_1LENGTH					(EXI_BASE + 0x1c)
#define EXI_1CR							(EXI_BASE + 0x20)
#define EXI_1DATA						(EXI_BASE + 0x24)
#define EXI_2CSR						(EXI_BASE + 0x28)
#define EXI_2MAR						(EXI_BASE + 0x2c)
#define EXI_2LENGTH					(EXI_BASE + 0x30)
#define EXI_2CR							(EXI_BASE + 0x34)
#define EXI_2DATA						(EXI_BASE + 0x38)

#define EXI_CSR_ROMDIS			(1 << 13)
// external device attached status / interrupt / mask
#define EXI_CSR_EXT					(1 << 12)
#define EXI_CSR_EXTINT			(1 << 11)
#define EXI_CSR_EXTINTMSK		(1 << 10)
#define EXI_CSR_CS2					(1 <<  9)
#define EXI_CSR_CS1					(1 <<  8)
#define EXI_CSR_CS0					(1 <<  7)
// transfer complete interrupt
#define EXI_CSR_TCINT				(1 <<  3)
#define EXI_CSR_TCINTMSK		(1 <<  2)
// external device detached interrupt / mask
#define EXI_CSR_EXIINT			(1 <<  1)
#define EXI_CSR_EXIINTMSK		(1 <<  0)

#define EXI_CR_TSTART				(1 << 0)
#define EXI_CR_DMA					(1 << 1)


#define EXI_INTERRUPT_TC			(1 << 0)
#define EXI_INTERRUPT_EXT			(1 << 1)
#define EXI_INTERRUPT_EXI			(1 << 2)


void exi_init (void);
void exi_reinit (void);

void exi_generate_interrupt (__u32 mask, int channel);


#endif // __HW_EXI_H

#ifndef __HW_PI_H
#define __HW_PI_H 1


#include "general.h"
#include "mem.h"


#define RPI_SIZE					0x100
#define RPI_MASK					0x0ff

#define RPI32(X)					(*((__u32 *)(&rpi[(X) & RPI_MASK])))

extern __u8 rpi[RPI_SIZE];


#define PI_BASE							0x3000
#define PI_INTSR						(PI_BASE + 0x00)
#define PI_INTMSKR					(PI_BASE + 0x04)

#define INTERRUPT_RESET_DEPRESSED			(1 << 16)
#define INTERRUPT_HSP				(1 << 13)
#define INTERRUPT_DEBUG			(1 << 12)
#define INTERRUPT_CP				(1 << 11)
#define INTERRUPT_PE_FINISH	(1 << 10)
#define INTERRUPT_PE_TOKEN	(1 <<  9)
#define INTERRUPT_VI				(1 <<  8)
#define INTERRUPT_MEM				(1 <<  7)
#define INTERRUPT_DSP				(1 <<  6)
#define INTERRUPT_AI				(1 <<  5)
#define INTERRUPT_EXI				(1 <<  4)
#define INTERRUPT_SI				(1 <<  3)
#define INTERRUPT_DI				(1 <<  2)
#define INTERRUPT_RSW				(1 <<  1)
#define INTERRUPT_ERROR			(1 <<  0)

#define INTSR								(RPI32 (PI_INTSR))
#define INTMR								(RPI32 (PI_INTMSKR))

#define PI_FIFO_BASE_P			0x300c
#define PI_FIFO_END_P				0x3010
#define PI_FIFO_WPOINTER_P	0x3014

#define PI_FIFO_BASE				(RPI32 (PI_FIFO_BASE_P))
#define PI_FIFO_END					(RPI32 (PI_FIFO_END_P))
#define PI_FIFO_WPOINTER		(RPI32 (PI_FIFO_WPOINTER_P))

#define PI_WRPTR_WRAP				(1 << 26)

void pi_init (void);
void pi_reinit (void);
void pi_check_for_interrupts (void);
void pi_interrupt (__u32 mask, int set);
void pi_interrupt_ex (__u32 mask, int set);


#endif // __HW_PI_H

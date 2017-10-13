#ifndef __HW_PE_H
#define __HW_PE_H 1


#include "general.h"
#include "mem.h"


#define RPE_SIZE					0x100
#define RPE_MASK					0x0ff

#define RPE16(X)					(*(__u16 *) (&rpe[(X) & RPE_MASK]))

extern __u8 rpe[RPE_SIZE];

#define PE_INTSR					(0x100a)
#define PE_TOKEN					(0x100e)

#define PE_INTSR_INT_FINISH			(1 << 0)
#define PE_INTSR_INT_TOKEN			(1 << 1)
#define PE_INTSR_MSK_FINISH			(1 << 2)
#define PE_INTSR_MSK_TOKEN			(1 << 3)


#define PE_INTERRUPT_FINISH			(1 << 0)
#define PE_INTERRUPT_TOKEN			(1 << 1)


void pe_init (void);
void pe_reinit (void);
void pe_set_token (__u16 token);

void pe_generate_interrupt (__u32 mask);


#endif // __HW_PE_H

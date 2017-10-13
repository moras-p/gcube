#ifndef __HW_MI_H
#define __HW_MI_H 1


#include "general.h"
#include "mem.h"


#define RMI_SIZE							0x100
#define RMI_MASK							0x0ff

#define RMI16(X)							(*((__u16 *) &rmi[(X) & RMI_MASK]))

extern __u8 rmi[RMI_SIZE];


#define MI_INTMSK							0x401c
#define MI_INT								0x401e

#define MI_INTMSK_ALL					(1 << 15)
#define MI_INTMSK_MEM3				(1 << 14)
#define MI_INTMSK_MEM2				(1 << 13)
#define MI_INTMSK_MEM1				(1 << 12)
#define MI_INTMSK_MEM0				(1 << 11)


void mi_init (void);
void mi_reinit (void);



#endif // __HW_MI_H

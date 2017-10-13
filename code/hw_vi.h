#ifndef __HW_VI_H
#define __HW_VI_H 1


#include "general.h"
#include "mem.h"
#include "video.h"

#define RVI_SIZE			0x100
#define RVI_MASK			0x0ff

// reversed addressing space
#ifdef LIL_ENDIAN
# define RVI16(X)			(*((__u16 *) &rvi[RVI_SIZE - ((X) & RVI_MASK) - 2]))
# define RVI32(X)			(*((__u32 *) &rvi[RVI_SIZE - ((X) & RVI_MASK) - 4]))
#else
# define RVI16(X)			(*((__u16 *) &rvi[(X) & RVI_MASK]))
# define RVI32(X)			(*((__u32 *) &rvi[(X) & RVI_MASK]))
#endif

extern __u8 rvi[RVI_SIZE];


#define VI_BASE				(0x2000)
#define VI_VCT				(VI_BASE + 0x2c)
#define VI_HCT				(VI_BASE + 0x2e)

#define VI_DINT0			(VI_BASE + 0x30)
#define VI_DINT1			(VI_BASE + 0x34)
#define VI_DINT2			(VI_BASE + 0x38)
#define VI_DINT3			(VI_BASE + 0x3c)

#define VI_DI_INT			(1 << 31)
#define VI_DI_INTEN		(1 << 28)

#define VI_MODE				(0x2000)
#define VI_XFB1				(0x201c)
#define VI_XFB2				(0x2024)


#define VI_INTERRUPT_1		(1 << 0)
#define VI_INTERRUPT_2		(1 << 1)
#define VI_INTERRUPT_3		(1 << 2)
#define VI_INTERRUPT_4		(1 << 3)


void vi_init (void);
void vi_reinit (void);
void vi_refresh (void);
void vi_set_video_mode (int mode);
void vi_set_country_code (unsigned char code);

void vi_generate_interrupt (__u32 mask);


#endif // __HW_VI_H

#ifndef __HW_SI_H
#define __HW_SI_H 1


#include "general.h"
#include "mem.h"

#define RSI_SIZE				0x100
#define RSI_MASK				0xff

/*
// reversed addressing space
#ifdef LIL_ENDIAN
# define RSI8(X)					(*((__u8 *) &rsi[RSI_SIZE - ((X) & RSI_MASK) - 1]))
# define RSI32(X)					(*((__u32 *) &rsi[RSI_SIZE - ((X) & RSI_MASK) - 4]))
#else
# define RSI8(X)					(*((__u8 *) &rsi[(X) & RSI_MASK]))
# define RSI32(X)					(*((__u32 *) &rsi[(X) & RSI_MASK]))
#endif
*/
#define RSI32(X)					(*((__u32 *) &rsi[(X) & RSI_MASK]))


extern __u8 rsi[RSI_SIZE];
extern int controller_type[4];

#define SI_CONTROLLER_TYPE(n)			(controller_type[n])

#define SI_CH0_CMD			0x6400
#define SI_CH0_INH			0x6404
#define SI_CH0_INL			0x6408
#define SI_CH1_CMD			0x640c
#define SI_CH1_INH			0x6410
#define SI_CH1_INL			0x6414
#define SI_CH2_CMD			0x6418
#define SI_CH2_INH			0x641c
#define SI_CH2_INL			0x6420
#define SI_CH3_CMD			0x6424
#define SI_CH3_INH			0x6428
#define SI_CH3_INL			0x642c

#define SI_CHN_CMD(n)		(0x6400 + n*12)
#define SI_CHN_INH(n)		(0x6404 + n*12)
#define SI_CHN_INL(n)		(0x6408 + n*12)

#define SI_CHH(B,X,Y)			((B << 16) | ((__u8) (0x80+X) << 8) | ((__u8) (0x80+Y)))
#define SI_CHL(X,Y,L,R)		(((__u8) (0x80+X) << 24) | ((__u8) (0x80+Y) << 16) | (R << 8) | L)

#define SI_POLL									0x6430
#define SI_COMMCSR							0x6434
#define SI_SR										0x6438
#define SI_EXILK								0x643c
#define SI_IO_BUFF							0x6480

#define SI_POLL_EN0							(1 << 7)
#define SI_POLL_EN1							(1 << 6)
#define SI_POLL_EN2							(1 << 5)
#define SI_POLL_EN3							(1 << 4)

#define SICSR										(RSI32 (SI_COMMCSR))
#define SISR										(RSI32 (SI_SR))

#define SI_COMMCSR_TCINT				(1 << 31)
#define SI_COMMCSR_TCINTMSK			(1 << 30)
#define SI_COMMCSR_COMERR				(1 << 29)
#define SI_COMMCSR_RDSTINT			(1 << 28)
#define SI_COMMCSR_RDSTINTMSK		(1 << 27)
#define SI_COMMCSR_OUTLNGTH			(0x7f << 16)
#define SI_COMMCSR_INLNGTH			(0x7f <<  8)
#define SI_COMMCSR_CHANNEL			(   3 <<  1)
#define SI_COMMCSR_TSTART				(1 <<  0)

#define SI_OUTLENGTH						((SICSR >> 16) & 0x7f)
#define SI_INLENGTH							((SICSR >>  8) & 0x7f)
#define SI_CHANNEL							((SICSR >>  1) & 3)


#define SI_SR_WR								(1 << 31)
// write status
#define SI_SR_WRST0							(1 << 28)
#define SI_SR_WRST1							(1 << 20)
#define SI_SR_WRST2							(1 << 12)
#define SI_SR_WRST3							(1 <<  4)
// read status
#define SI_SR_RDST0							(1 << 29)
#define SI_SR_WRST0							(1 << 28)
#define SI_SR_NOREP0						(1 << 27)
#define SI_SR_COLL0							(1 << 26)
#define SI_SR_OVRUN0						(1 << 25)
#define SI_SR_UNRUN0						(1 << 24)
#define SI_SR_RDST1							(1 << 21)
#define SI_SR_WRST1							(1 << 20)
#define SI_SR_NOREP1						(1 << 19)
#define SI_SR_COLL1							(1 << 18)
#define SI_SR_OVRUN1						(1 << 17)
#define SI_SR_UNRUN1						(1 << 16)
#define SI_SR_RDST2							(1 << 13)
#define SI_SR_WRST2							(1 << 12)
#define SI_SR_NOREP2						(1 << 11)
#define SI_SR_COLL2							(1 << 10)
#define SI_SR_OVRUN2						(1 <<  9)
#define SI_SR_UNRUN2						(1 <<  8)
#define SI_SR_RDST3							(1 <<  5)
#define SI_SR_WRST3							(1 <<  4)
#define SI_SR_NOREP3						(1 <<  3)
#define SI_SR_COLL3							(1 <<  2)
#define SI_SR_OVRUN3						(1 <<  1)
#define SI_SR_UNRUN3						(1 <<  0)

#define SI_SR_ERROR_BITS				(SI_SR_NOREP0 | SI_SR_COLL0 | SI_SR_OVRUN0 | SI_SR_UNRUN0 |\
																 SI_SR_NOREP1 | SI_SR_COLL1 | SI_SR_OVRUN1 | SI_SR_UNRUN1 |\
																 SI_SR_NOREP2 | SI_SR_COLL2 | SI_SR_OVRUN2 | SI_SR_UNRUN2 |\
																 SI_SR_NOREP3 | SI_SR_COLL3 | SI_SR_OVRUN3 | SI_SR_UNRUN3)

#define BUTTON_LEFT			0x0001
#define BUTTON_RIGHT		0x0002
#define BUTTON_DOWN			0x0004
#define BUTTON_UP				0x0008
#define BUTTON_Z				0x0010
#define BUTTON_L				0x0020
#define BUTTON_R				0x0040
#define BUTTON_A				0x0100
#define BUTTON_B				0x0200
#define BUTTON_X				0x0400
#define BUTTON_Y				0x0800
#define BUTTON_START		0x1000

#define ACTION_BUTTON		0x00000000
#define ACTION_AXIS			0x00002000
#define ACTION_AXIS0M		(ACTION_AXIS + 0)
#define ACTION_AXIS0P		(ACTION_AXIS + 1)
#define ACTION_AXIS1M		(ACTION_AXIS + 2)
#define ACTION_AXIS1P		(ACTION_AXIS + 3)
#define ACTION_AXIS2M		(ACTION_AXIS + 4)
#define ACTION_AXIS2P		(ACTION_AXIS + 5)
#define ACTION_AXIS3M		(ACTION_AXIS + 6)
#define ACTION_AXIS3P		(ACTION_AXIS + 7)


#define SIDEV_GC_CONTROLLER			0x09000000
#define SIDEV_GC_RECEIVER				0x88000000
#define SIDEV_GC_WAVEBIRD				0x8b100000
#define SIDEV_GC_KEYBOARD				0x08200000
#define SIDEV_N64_CONTROLLER		0x05000000
#define SIDEV_N64_MIC						0x00010000
#define SIDEV_N64_KEYBOARD			0x00020000
#define SIDEV_N64_MOUSE					0x02000000
#define SIDEV_GBA								0x00040000


#define SI_INTERRUPT_TC			(1 << 0)
#define SI_INTERRUPT_RDST		(1 << 1)

#pragma pack(1)
struct PADStatusTag
{
	__u16 buttons;
	__s8 xstick, ystick;
	__s8 xsubstick, ysubstick;
	__u8 ltrigger, rtrigger;
	__u8 aanalog, banalog;
	__s8 err;
} __attribute__ ((packed));
typedef struct PADStatusTag PADStatus;


#define PAD_ERR_NONE						0
#define PAD_ERR_NO_CONTROLLER		-1
#define PAD_ERR_NOT_READY				-2


void si_init (void);
void si_reinit (void);
void si_poll (void);

void si_generate_interrupt (__u32 mask);
void si_update_devices (void);

void pad_set_action (int n, __u16 buttons, __s8 xstick, __s8 ystick,
										 __s8 xsubstick, __s8 ysubstick);
void pad_action_buttons (int n, int pressed, __u16 buttons);
void pad_action_buttons_masked (int n, int pressed, __u16 buttons, int mask);
void pad_action_stick (int n, __s8 x, __s8 y);
void pad_action_substick (int n, __s8 x, __s8 y);
void pad_action_axis (int n, int axis, __s8 value);
void pad_action (int n, int pressed, int action);


#endif // __HW_SI_H

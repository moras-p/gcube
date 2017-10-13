#ifndef __CPU_H
#define __CPU_H 1


#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <math.h>

// missing from math.h
double round (double);
double trunc (double);

#include "general.h"
#include "mem.h"
#include "gdebug.h"


// registers
extern __u32 cpuregs[4096];
extern __u64 ps0[32];
extern __u64 ps1[32];

//#define BUS_CLOCK_SPEED					0x09a7ec80
#define BUS_CLOCK_SPEED					162000000
// 486 MHz
//#define CPU_CLOCK_SPEED					0x1cf7c580
#define CPU_CLOCK_SPEED					486000000


#define TBMAGIC									((__u64) 23 * 365 * 24 * 60 * 60 + 7 * 366 * 24 * 60 * 60)
#define T2TICKS(t)							((t) * CPU_CLOCK_SPEED / 12)
#define CALC_TBL								(TBL = T2TICKS ((__u64) time (NULL) - TBMAGIC))
#define CALC_TBU								(TBU = T2TICKS ((__u64) time (NULL) - TBMAGIC) >> 32)


// opcodes
#define OPCODE(X)			 		static void X (__u32 opcode)
#define OP4(X,O,I,R)			(op##X [(O << 10) | (I << 1) | R])
#define OP3(X,I,R)				(op##X [(I << 1) | R])
#define OP1(X)						(op0 [X])
#define OPR(X,I,R,m,n,o)\
	({\
		int i;\
		for (i = 0; i < (1 << (n - m)); i++)\
			op##X [(i << m) | (I << 1) | R] = o;\
	})


// bit operations
#if USE_BITOPS_H
# include <asm/bitops.h>
# define BIT(X,m,n)					(BIT_IS_SET(X,m,n) ? 1 : 0)
# define BIT_IS_SET(X,m,n)	(test_bit ((m)-(n)-1, &X))
# define BIT32(X,n)					(BIT (X, 32, n))
# define SET_BIT(X,n)				(set_bit (n, &X))
# define CLEAR_BIT(X,n)			(clear_bit (n, &X))
#else
# define BIT(X,m,n)					(((X) >> ((m)-(n)-1)) & 1)
# define BIT_IS_SET(X,m,n)	((X) & (1 << ((m)-(n)-1)))
# define BIT32(X,n)					(BIT (X, 32, n))
# define SET_BIT(X,n)				((X) |= (1 << (31-(n))))
# define CLEAR_BIT(X,n)			((X) &= ~(1 << (31-(n))))
#endif

//#define CHANGE_BIT(X,n,b)	((b) ? set_bit (n, &X) : clear_bit (n, &X))
#define CHANGE_BIT(X,n,b) 	((X) = ((X) &~ (1 << (31-(n)))) | (((b) & 1) << (31-(n))))


#define CPUREGS(x)			(cpuregs[x])
#define FPUREGS(x)			(((double *)ps0)[x])
#define FPUREGS_PS0(x)	(((double *)ps0)[x])
#define FPUREGS_PS1(x)	(((double *)ps1)[x])
// general purpose registers (r0-r31)
#define I_GPR		0
#define GPR 		(&CPUREGS (I_GPR))
// segment registers (sr0-sr15)
#define I_SR		32
#define SR			(&CPUREGS (I_SR))
// condition register
#define I_CR		48
#define CR			(CPUREGS (I_CR))
// machine state register
#define I_MSR		49
#define MSR			(CPUREGS (I_MSR))
// floating point status and control register
#define I_FPSCR	50
#define FPSCR		(CPUREGS (I_FPSCR))
// time base facility (lower and upper)
#define I_TBL		51
#define I_TBU		52
#define TBL			(CPUREGS (I_TBL))
#define TBU			(CPUREGS (I_TBU))
// special purpose registers (spr0-spr1023)
#define I_SPR		1024
#define SPR			(&CPUREGS (I_SPR))
#define I_DSISR	(I_SPR + 18)
#define DSISR		(CPUREGS (I_DSISR))
#define I_DAR		(I_SPR + 19)
#define DAR			(CPUREGS (I_DAR))
// for paired singles
#define I_GQR		(I_SPR + 912)
#define GQR			(&CPUREGS (I_GQR))
#define GQR0		(GQR[0])
#define GQR1		(GQR[1])
#define GQR2		(GQR[2])
#define GQR3		(GQR[3])
#define GQR4		(GQR[4])
#define GQR5		(GQR[5])
#define GQR6		(GQR[6])
#define GQR7		(GQR[7])
// program counter
#define I_PC		4095
#define PC			(CPUREGS (I_PC))
// instruction counter
#define I_IC		4094
#define IC			(CPUREGS (I_IC))
// direct memory access registers
#define I_DMAU	(I_SPR + 922)
#define DMAU		(CPUREGS (I_DMAU))
#define I_DMAL	(I_SPR + 923)
#define DMAL		(CPUREGS (I_DMAL))
#define LCE			(HID2 & HID2_LCE)
#define DMA_MEM_ADDR	(DMAU &~ 0x1f)
#define DMA_LC_ADDR		(DMAL &~ 0x1f)
#define DMA_LD				(DMAL & 0x10)
#define DMA_T					(DMAL & 2)
#define DMA_F					(DMAL & 1)
#define DMA_LEN				(((DMAU & 0x1f) << 2) | ((DMAL >> 2) & 3))

// stackpointer
#define I_SP		1
#define RSP			(CPUREGS (I_SP))
#define I_SDA1	2
#define I_SDA2	13
#define SDA1		(CPUREGS (I_SDA1))
#define SDA2		(CPUREGS (I_SDA2))
// register specifing carries and overflows for integer operations
#define I_XER		(I_SPR + 1)
#define XER			(CPUREGS (I_XER))
// link register
#define I_LR		(I_SPR + 8)
#define LR			(CPUREGS (I_LR))
// count register
#define I_CTR		(I_SPR + 9)
#define CTR			(CPUREGS (I_CTR))
// decrement register
#define I_DEC		(I_SPR + 22)
#define DEC			(CPUREGS (I_DEC))
// machine status save/restore register 0 and 1
#define I_SRR0	(I_SPR + 26)
#define I_SRR1	(I_SPR + 27)
#define SRR0		(CPUREGS (I_SRR0))
#define SRR1		(CPUREGS (I_SRR1))
// hardware implementation registers
#define I_HID2	(I_SPR + 920)
#define I_HID0	(I_SPR + 1008)
#define I_HID1	(I_SPR + 1009)
#define HID2		(CPUREGS (I_HID2))
#define HID0		(CPUREGS (I_HID0))
#define HID1		(CPUREGS (I_HID1))
#define I_SDR1		(I_SPR + 25)
#define SDR1			(CPUREGS (I_SDR1))
#define I_PVR			(I_SPR + 287)
#define PVR				(CPUREGS (I_PVR))
#define I_IBAT0U	(I_SPR + 528)
#define I_IBAT0L	(I_SPR + 529)
#define I_IBAT1U	(I_SPR + 530)
#define I_IBAT1L	(I_SPR + 531)
#define I_IBAT2U	(I_SPR + 532)
#define I_IBAT2L	(I_SPR + 533)
#define I_IBAT3U	(I_SPR + 534)
#define I_IBAT3L	(I_SPR + 535)
#define IBATU(X)	(CPUREGS (I_IBAT0U + 2*X))
#define IBATL(X)	(CPUREGS (I_IBAT0L + 2*X))
#define I_DBAT0U	(I_SPR + 536)
#define I_DBAT0L	(I_SPR + 537)
#define I_DBAT1U	(I_SPR + 538)
#define I_DBAT1L	(I_SPR + 539)
#define I_DBAT2U	(I_SPR + 540)
#define I_DBAT2L	(I_SPR + 541)
#define I_DBAT3U	(I_SPR + 542)
#define I_DBAT3L	(I_SPR + 543)
#define DBATU(X)	(CPUREGS (I_DBAT0U + 2*X))
#define DBATL(X)	(CPUREGS (I_DBAT0L + 2*X))

#define HID2_LSQE		(1 << 31)
#define HID2_WPE		(1 << 30)
#define HID2_PSE		(1 << 29)
#define HID2_LCE		(1 << 28)
#define HID2_DMAQL	((HID2 >> 4) & 0xf)

#define I_WPAR	(I_SPR + 921)
#define WPAR		(CPUREGS (I_WPAR))

// floating point registers (access as fp and binary)
#define FPR			((double *)ps0)
#define FBR			((__u64  *)ps0)

// opcode decoding
#define OPC		 	opcode
#define RD			((OPC >> 21) &       0x1f)
#define RS			RD
#define RA			((OPC >> 16) &       0x1f)
#define RB			((OPC >> 11) &       0x1f)
#define RC			((OPC >>  6) &       0x1f)
#define UIMM		((__u16) OPC)
#define SIMM		((__s16) OPC)
#define OE			((OPC >> 10) &       0x01)
#define Rc			(OPC         &       0x01)
#define LK			Rc
#define AA			((OPC >>  1) &       0x01)
#define LI			(OPC         &  0x3fffffc)
#define BO			((OPC >> 21) &       0x1f)
#define BI			((OPC >> 16) &       0x1f)
#define BD			((__s16) (OPC         &     0xfffc))
#define CRFD		((OPC >> 23) &       0x07)
#define L				((OPC >> 21) &       0x01)
#define CRBD		RD
#define CRBA		RA
#define CRBB		RB
#define CRFS		((OPC >> 18) &       0x07)
#define CRM			((OPC >> 12) &       0xff)
#define SH			RB
#define MB			((OPC >>  6) &       0x1f)
#define ME			((OPC >>  1) &       0x1f)
#define FM			((OPC >> 17) &       0xff)
#define IMM			((OPC >> 12) &       0x0f)

// quick register access
#define RRD			(GPR[RD])
#define RRS			(GPR[RS])
#define RRA			(GPR[RA])
#define RRB			(GPR[RB])
#define RRC			(GPR[RC])
#define NB			RB
#define RNB			(GPR[NB])
#define FRD			(FPR[RD])
#define FRS			(FPR[RS])
#define FRA			(FPR[RA])
#define FRB			(FPR[RB])
#define FRC			(FPR[RC])
#define FBD			(FBR[RD])
#define FBS			(FBR[RS])
#define FBA 		(FBR[RA])
#define FBB 		(FBR[RB])
#define FBC 		(FBR[RC])

// paired singles
#define PS0						((double *)ps0)
#define PS1						((double *)ps1)
#define PSB0					((__u64 *)ps0)
#define PSB1					((__u64 *)ps1)
#define PS0D					(PS0[RD])
#define PS1D					(PS1[RD])
#define PS0A					(PS0[RA])
#define PS1A					(PS1[RA])
#define PS0B					(PS0[RB])
#define PS1B					(PS1[RB])
#define PS0C					(PS0[RC])
#define PS1C					(PS1[RC])
#define PSB0D					(PSB0[RD])
#define PSB1D					(PSB1[RD])
#define PSB0A					(PSB0[RA])
#define PSB1A					(PSB1[RA])
#define PSB0B					(PSB0[RB])
#define PSB1B					(PSB1[RB])
#define PSB0C					(PSB0[RC])
#define PSB1C					(PSB1[RC])

#define PSE						(HID2 & HID2_PSE)
#define PSIMM					(EXTS (12, ((OPC) & 0xfff)))
#define PSI						((OPC >> 12) & 7)
#define PSXI					((OPC >>  7) & 7)
#define PSW						(OPC & 0x8000)
#define PSXW					(OPC & 0x0400)
#define LD_SCALE(n)		((GQR[n] >> 24) & 0x3f) 
#define LD_TYPE(n)		((GQR[n] >> 16) & 0x07)
#define ST_SCALE(n)		((GQR[n] >>  8) & 0x3f)
#define ST_TYPE(n)		((GQR[n]      ) & 0x07)

#define BRANCH_CTR_OK(bo)		(BIT_IS_SET ((bo), 5, 2) || (((CTR != 0) ^ BIT ((bo), 5, 3))))
#define BRANCH_COND_OK(bo) 	(BIT_IS_SET ((bo), 5, 0) || (BIT (CR, 32, BI) == BIT ((bo), 5, 1)))


// condition register bits
// lower than, greater than, equal and summary overflow (copy of XER_SO)
#define CR_LT				(1 << 3)
#define CR_GT				(1 << 2)
#define CR_EQ				(1 << 1)
#define CR_SO				(1 << 0)


#define SET_FIELD(X,n,Y)\
	({\
			__u32 xn = (n);\
			(X = (X &~ (0x0f << ((7-(xn)) << 2))) | ((Y) << ((7-(xn)) << 2)));\
	})
#define CLEAR_FIELD(X,n)	(X = (X &~ (0x0f << ((7-(n)) << 2))))
#define GET_FIELD(X,n)		((X >> (32 - (((n) + 1) << 2))) & 0x0f)

// signed
#define CALC_CR(n,X,Y)\
	({\
			__s32 xX = (X);\
			__s32 xY = (Y);\
			SET_FIELD (CR, n, ((IS_XER_SO) ? CR_SO : 0) | ((xX < xY) ? CR_LT : 0) | ((xX > xY) ? CR_GT : 0) | ((xX == xY) ? CR_EQ : 0));\
	})
// unsigned
#define CALC_CRL(n,X,Y)\
	({\
			__u32 xX = (X);\
			__u32 xY = (Y);\
			SET_FIELD (CR, n, ((IS_XER_SO) ? CR_SO : 0) | ((xX < xY) ? CR_LT : 0) | ((xX > xY) ? CR_GT : 0) | ((xX == xY) ? CR_EQ : 0));\
	})

// rewrite in asm
#define CALC_CR0(X)				(CALC_CR (0,X,0))
#define SET_CR0(X)				(SET_FIELD (CR, 0, X))
#define GET_CR0						(GET_FIELD (CR, 0))

#define IS_NAN(X)					((((X) & 0x000fffffffffffffULL) != 0) && (((X) & 0x7ff0000000000000ULL) == 0x7ff0000000000000ULL))
#define IS_QNAN(X)				((((X) & 0x000fffffffffffffULL) != 0) && (((X) & 0x7ff8000000000000ULL) == 0x7ff8000000000000ULL))
#define IS_SNAN(X)				((((X) & 0x000fffffffffffffULL) != 0) && (((X) & 0x7ff8000000000000ULL) == 0x7ff0000000000000ULL))
#define CALC_CRF(n,X,Y)\
	({\
			double xX = (X);\
			double xY = (Y);\
			if (xX < xY)\
			{\
				SET_FIELD (CR, n, 8);\
				FPSCR |= 8 << FPSCR_FPCC;\
			}\
			else if (xX > xY)\
			{\
				SET_FIELD (CR, n, 4);\
				FPSCR |= 4 << FPSCR_FPCC;\
			}\
			else\
			{\
				SET_FIELD (CR, n, 2);\
				FPSCR |= 2 << FPSCR_FPCC;\
			}\
	})
// ps are defined as doubles
#define IS_NANS						IS_NAN
#define CALC_CRFS(n,Xf,Xb,Yf,Yb)\
	({\
			double xX = (Xf);\
			double xY = (Yf);\
			FPSCR &= ~(0x1f << FPSCR_FPCC);\
			if (IS_NAN (Xb) || IS_NAN (Yb))\
			{\
				SET_FIELD (CR, n, 1);\
				FPSCR |= 1 << FPSCR_FPCC;\
				FPSCR |= FPSCR_VXSNAN;\
			}\
			else if (xX < xY)\
			{\
				SET_FIELD (CR, n, 8);\
				FPSCR |= 8 << FPSCR_FPCC;\
			}\
			else if (xX > xY)\
			{\
				SET_FIELD (CR, n, 4);\
				FPSCR |= 4 << FPSCR_FPCC;\
			}\
			else\
			{\
				SET_FIELD (CR, n, 2);\
				FPSCR |= 2 << FPSCR_FPCC;\
			}\
	})

#define SQRT(X)						(sqrt (X))

// XER bits
#define XER_SO						(1 << 31)
#define XER_OV						(1 << 30)
#define XER_CA						(1 << 29)
// XER byte count
#define XER_BC						(XER & 0x7f)

#define IS_XER_CA		 			((XER & XER_CA) > 0)
#define SET_XER_CA				(XER |=  XER_CA)
#define CLEAR_XER_CA			(XER &= ~XER_CA)
#define CHANGE_XER_CA(X)	((X)? SET_XER_CA : CLEAR_XER_CA)

#define IS_XER_OV		 			((XER & XER_OV) > 0)
#define SET_XER_OV				(XER |=  XER_OV)
#define CLEAR_XER_OV			(XER &= ~XER_OV)

#define IS_XER_SO		 			((XER & XER_SO) > 0)
#define SET_XER_SO				(XER |=  XER_SO)
#define CLEAR_XER_SO			(XER &= ~XER_SO)

#define SET_XER_OV_SO			(XER |=  XER_OV | XER_SO)

// carry
#define CALC_XER_CA(X,Y)			(((X) + (Y) < X) ? SET_XER_CA : CLEAR_XER_CA)
#define CALC_XER_CA_E(X,Y,c)	((CARRY3 (X,Y,c)) ? SET_XER_CA : CLEAR_XER_CA)
// overflow
//#define CALC_XER_OV(X,Y)			((OVERFLOW (X,Y)) ? SET_XER_OV_SO : CLEAR_XER_OV)
#define CALC_XER_OV(X,Y)			((CALC_OVERFLOW (X,Y)) ? SET_XER_OV_SO : CLEAR_XER_OV)
#define CALC_XER_OV_E(X,Y,c)	((CALC_OVERFLOW3 (X,Y,c)) ? SET_XER_OV_SO : CLEAR_XER_OV)
//#define CALC_XER_OV_E(X,Y,c)	((((X) > 0) && ((Y) > INT_MAX - (X))) ? SET_XER_OV_SO : ((c) ? CALC_XER_OV ((X)+(Y), c) : CLEAR_XER_OV))
// underflow
#define CALC_XER_OV_U(X,Y)		((CALC_UNDERFLOW (X,Y)) ? SET_XER_OV_SO : CLEAR_XER_OV)
#define CALC_XER_OV_UE(X,Y,c)	((CALC_UNDERFLOW3 (X,Y,c)) ? SET_XER_OV_SO : CLEAR_XER_OV)
//#define CALC_XER_OV_UE(X,Y,c)	((((X) < 0) && ((Y) < INT_MIN - (X))) ? SET_XER_OV_SO : ((c) ? CALC_XER_OV ((X)+(Y), c) : CLEAR_XER_OV))

// 0-3 bits of XER
#define GET_XER_03				((XER >> 28) & 0x0f)
#define SET_XER_03(X)			(XER = (XER &~ (0x0f << 28)) | ((X) << 28))

#define GENMASK(X,Y)\
	({\
			__u32 xX = (X);\
			__u32 xY = (Y);\
			((xX <= xY) ? \
			((0xffffffff >> (xX)) ^ (((xY) < 31)? (0xffffffff >> ((xY) + 1)) : 0)) :\
			(~((0xffffffff >> (xX)) ^ (((xY) < 31)? (0xffffffff >> ((xY) + 1)) : 0)))) ;\
	})
#define MASK(X,Y)			(mask[X][Y])


#define EXTS(n,X)					((__s32)((X) << (32 - (n))) >> (32 - (n)))
#define EXTS8(X)					((__u32)(__s32)(__s8)(__u8) X)
#define EXTS16(X)					((__s16) X)

#define FPSCR_FX					(1 << 31)
#define FPSCR_FEX					(1 << 30)
#define FPSCR_VX					(1 << 29)
#define FPSCR_OX					(1 << 28)
#define FPSCR_UX					(1 << 27)
#define FPSCR_ZX					(1 << 26)
#define FPSCR_XX					(1 << 25)
#define FPSCR_VXSNAN			(1 << 24)
#define FPSCR_VXISI				(1 << 23)
#define FPSCR_VXIDI				(1 << 22)
#define FPSCR_VXZDZ				(1 << 21)
#define FPSCR_VXIMZ				(1 << 20)
#define FPSCR_VXVC				(1 << 19)
#define FPSCR_FR					(1 << 18)
#define FPSCR_FI					(1 << 17)
#define FPSCR_VXSOFT			(1 << 10)
#define FPSCR_VXSQRT			(1 <<  9)
#define FPSCR_VXCVI				(1 <<  8)
#define FPSCR_VE					(1 <<  7)
#define FPSCR_OE					(1 <<  6)
#define FPSCR_UE					(1 <<  5)
#define FPSCR_ZE					(1 <<  4)
#define FPSCR_XE					(1 <<  3)
#define FPSCR_NI					(1 <<  2)
#define FPSCR_RN					(FPSCR & 3)
// condition code shift
#define FPSCR_FPCC				12

#define ROUND_NEAREST			0
#define ROUND_TWZERO			1
#define ROUND_TWPINF			2
#define ROUND_TWMINF			3

#define MSR_POW						(1 << 18)
#define MSR_ILE						(1 << 16)
#define MSR_EE						(1 << 15)
#define MSR_PR						(1 << 14)
#define MSR_FP						(1 << 13)
#define MSR_ME						(1 << 12)
#define MSR_FE0						(1 << 11)
#define MSR_SE						(1 << 10)
#define MSR_BE						(1 <<  9)
#define MSR_FE1						(1 <<  8)
#define MSR_IP						(1 <<  6)
#define MSR_IR						(1 <<  5)
#define MSR_DR						(1 <<  4)
#define MSR_PM						(1 <<  2)
#define MSR_RI						(1 <<  1)
#define MSR_LE						(1 <<  0)

#define BATU_BEPI(X)			(X >> 17)
#define BATU_BL(X)				((X >> 2) & 0x7ff)
#define BATU_VS						(1 <<  1)
#define BATU_VP						(1 <<  0)

#define BATL_BRPN(X)			(X >> 17)
#define BATL_WIMG(X)			((X >> 3) & 0x0f)
#define BATL_W						(1 <<  6)
#define BATL_I						(1 <<  5)
#define BATL_M						(1 <<  4)
#define BATL_G						(1 <<  3)
#define BATL_PP(X)				(X & 3)

#define SR_T							(1 << 31)
#define SR_KS							(1 << 30)
#define SR_KP							(1 << 29)
#define SR_N							(1 << 28)
#define SR_VSID(X)				(X & 0x00ffffff)

#define SDR1_HTABORG			(SDR1 >> 16)
#define SDR1_HTABMASK			(SDR1 & 0x000001ff)

#define EA_SR(X)					(X >> 28)
#define EA_API(X)					((X >> 22) & 0x3f)
#define EA_PI(X)					((X >> 12) & 0xffff)
#define EA_OFFSET(X)			(X & 0xfff)

#define PTE1_V						(1 << 31)
#define PTE1_VSID(X)			((X >> 7) & 0x00ffffff)
#define PTE1_H						(1 <<  6)
#define PTE1_API(X)				(X & 0x3f)

#define PTE2_RPN(X)				(X >> 12)
#define PTE2_R						(1 << 8)
#define PTE2_C						(1 << 7)
#define PTE2_W						(1 << 6)
#define PTE2_I						(1 << 5)
#define PTE2_M						(1 << 4)
#define PTE2_G						(1 << 3)
#define PTE2_PP(X)				(X & 3)

// exceptions
#define EXCEPTION_SYSTEM_RESET					0x80000100
#define EXCEPTION_MACHINE_CHECK					0x80000200
#define EXCEPTION_DSI										0x80000300
#define EXCEPTION_ISI										0x80000400
#define EXCEPTION_EXTERNAL							0x80000500
#define EXCEPTION_ALIGNMENT							0x80000600
#define EXCEPTION_PROGRAM								0x80000700
#define EXCEPTION_FP_UNAVAILABLE				0x80000800
#define EXCEPTION_DECREMENTER						0x80000900
#define EXCEPTION_SYSTEM_CALL						0x80000c00
#define EXCEPTION_TRACE									0x80000d00
#define EXCEPTION_PERFORMANCE_MONITOR		0x80000f00
#define EXCEPTION_IABR									0x80001300
#define EXCEPTION_RESERVED							0x80001400
#define EXCEPTION_THERMAL								0x80001700

#define DSISR_PAGE									(1 << 30)
#define DSISR_PROT									(1 << 27)
#define DSISR_STORE									(1 << 25)

#define SRR1_PAGE										(1 << 30)
#define SRR1_GUARD									(1 << 28)

// processor version number
#define PVR_GEKKO										0x7000
#define PVR_GEKKO_EARLY_HW					0x0008
#define PVR_REVISION_FIRST					0x0100
#define PVR_REVISION								0xbabe

extern int ref_delay;

void hle_execute (__u32 op);
void hle_reattach (__u32 op);


void cpu_init (void);
void cpu_execute (void);
void cpu_exception (__u32 ex);


#endif // __CPU_H

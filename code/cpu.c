/*
 *  Gekko interpreter
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
 *  missing:
 *    proper handling of FPSCR
 *  known bugs:
 *    lwarx and stwcx. not emulated correctly
 */


#include "cpu.h"


// registers
__u32 cpuregs[4096];
__u64 ps0[32], ps1[32];

// opcodes
void (*op0 [64]) (__u32);
void (*op4 [2048]) (__u32);
void (*op19[2048]) (__u32);
void (*op31[2048]) (__u32);
void (*op59[64]) (__u32);
void (*op63[2048]) (__u32);

// precalculated masks
__u32 mask[32][32];

//double (*fp_round[]) (double) = { round, trunc, ceil, floor };
double(*fp_round[]) (double) = { rint, rint, ceil, floor };
#define FP_ROUND(D)		(fp_round[FPSCR_RN] (D))

#if 0
# include <fenv.h>
int fp_round_mode[] = { FE_TONEAREST, FE_TOWARDZERO, FE_UPWARD, FE_DOWNWARD };
# define SET_ROUNDING_MODE				(fesetround (fp_round_mode[FPSCR_RN]))
#else
# define SET_ROUNDING_MODE
#endif


#define CHECK_FOR_FPU	do { if (!(MSR & MSR_FP)) return cpu_exception_dont_advance (EXCEPTION_FP_UNAVAILABLE); } while (0)

// how much to increase TB / decrease DEC every op
#define TBINC			8


// for lwarx and stwcx.
//int RESERVE = 0;
//__u32 RESERVE_ADDR = 0;
// use some unused cpu regs so they will get written into savestates
#define CPUREGS_PRIV			512
#define RESERVE 					(CPUREGS (CPUREGS_PRIV + 0))
#define RESERVE_ADDR			(CPUREGS (CPUREGS_PRIV + 1))


void cpu_exception (__u32 ex)
{
	// recoverable
	MSR |= MSR_RI;

	SRR0 = PC + 4;
	SRR1 = MSR;
	MSR &= ~(MSR_POW | MSR_EE | MSR_PR | MSR_FP | MSR_FE0 | MSR_SE |
					 MSR_BE | MSR_FE1 | MSR_IR | MSR_DR | MSR_PM | MSR_RI);
	
	PC = (MSR & MSR_IP) ? (ex | 0xfff00000) : ex;
	// PC will advance in cpu_execute
	PC -= 4;

	DEBUG (EVENT_EXCEPTION, "cpu exception %.8x", ex);
}


void cpu_exception_dont_advance (__u32 ex)
{
	cpu_exception (ex);
	// don't advance to next instruction
	SRR0 -= 4;
}


void decrementer_tick (void)
{
	if (DEC < TBINC)
	{
		if (MSR & MSR_EE)
		{
			cpu_exception (EXCEPTION_DECREMENTER);
			DEBUG (EVENT_EXCEPTION, "decrementer exception");
		}
		
		// zelda wind waker fix
		DEC = 0x00100000;
//		DEC = 0xffffffff;
	}

	DEC -= TBINC;
	TB += TBINC;
}


void cpu_execute (void)
{
	__u32 opcode;


	if (!mem_fetch (PC, &opcode))
	{
//		DEBUG (EVENT_EFATAL, ".cpu: unable to fetch opcode from PC %8.8x", PC);
		PC += 4;
		return;
	}

	op0[opcode >> 26] (opcode);

	decrementer_tick ();

	hw_update ();
	pi_check_for_interrupts ();

	IC++;
	PC += 4;
}


// not implemented
OPCODE (NI)
{
	DEBUG (EVENT_EMAJOR, ".cpu: %.8x -> unknown opcode %.8x: (%u, %u)",
					PC, OPC, OPC >> 26, (OPC & 0x7ff) >> 1);
}


// error occured
OPCODE (INVALID)
{
#ifdef GDEBUG
	DEBUG (EVENT_EFATAL, ".cpu: invalid opcode (%.8x), PC = %.8x", OPC, PC);
#endif
}

/*
// debugger trap
OPCODE (DEBUG_TRAP)
{
	DEBUG (EVENT_TRAP, ". debugger trap found");
}
*/

// empty
OPCODE (EMPTY)
{
}


// add
OPCODE (ADD)
{
	RRD = RRA + RRB;
}


// add (overflow)
OPCODE (ADDO)
{
	__u32 rra = RRA, rrb = RRB;


	CALC_XER_OV (rra, rrb);
	RRD = rra + rrb;
}


// add (record)
OPCODE (ADDD)
{
	CALC_CR0 ((RRD = RRA + RRB));
}


// add (overflow, record)
OPCODE (ADDOD)
{
	__u32 rra = RRA, rrb = RRB;


	CALC_XER_OV (rra, rrb);
	CALC_CR0 ((RRD = rra + rrb));
}


// add carrying
OPCODE (ADDC)
{
	__u32 rra = RRA, rrb = RRB;


	CALC_XER_CA (rra, rrb);
	RRD = rra + rrb;
}


// add carrying (overflow)
OPCODE (ADDCO)
{
	__u32 rra = RRA, rrb = RRB;


	CALC_XER_CA (rra, rrb);
	CALC_XER_OV (rra, rrb);
	RRD = rra + rrb;
}


// add carrying (record)
OPCODE (ADDCD)
{
	__u32 rra = RRA, rrb = RRB;


	CALC_XER_CA (rra, rrb);
	CALC_CR0 ((RRD = rra + rrb));
}


// add carrying (overflow, record)
OPCODE (ADDCOD)
{
	__u32 rra = RRA, rrb = RRB;


	CALC_XER_CA (rra, rrb);
	CALC_XER_OV (rra, rrb);
	CALC_CR0 ((RRD = rra + rrb));
}


// add extended
OPCODE (ADDE)
{
	__u32 rra = RRA, rrb = RRB, c = IS_XER_CA;


	CALC_XER_CA_E (rra, rrb, c);
	RRD = rra + rrb + c;
}


// add extended (overflow)
OPCODE (ADDEO)
{
	__u32 rra = RRA, rrb = RRB, c = IS_XER_CA;


	CALC_XER_CA_E (rra, rrb, c);
	CALC_XER_OV_E (rra, rrb, c);
	RRD = rra + rrb + c;
}


// add extended (record)
OPCODE (ADDED)
{
	__u32 rra = RRA, rrb = RRB, c = IS_XER_CA;


	CALC_XER_CA_E (rra, rrb, c);
	CALC_CR0 ((RRD = rra + rrb + c));
}


// add extended (overflow, record)
OPCODE (ADDEOD)
{
	__u32 rra = RRA, rrb = RRB, c = IS_XER_CA;


	CALC_XER_CA_E (rra, rrb, c);
	CALC_XER_OV_E (rra, rrb, c);
	CALC_CR0 ((RRD = rra + rrb + c));
}


// add immediate
OPCODE (ADDI)
{
	unsigned int ra = RA;


	RRD = (ra) ? (GPR[ra] + SIMM) : SIMM;
}


// add immediate carrying
OPCODE (ADDIC)
{
	__u32 rra = RRA;
	__s16 simm = SIMM;


	CALC_XER_CA (rra, simm);
	RRD = rra + simm;
}


// add immediate carrying and record
OPCODE (ADDICD)
{
	__u32 rra = RRA;
	__s16 simm = SIMM;


	CALC_XER_CA (rra, simm);
	CALC_CR0 ((RRD = rra + simm));
}


// add immediate shifted
OPCODE (ADDIS)
{
	unsigned int ra = RA;


	RRD = (ra) ? GPR[ra] + (SIMM << 16) : (SIMM << 16);
}


// add to minus one extended
OPCODE (ADDME)
{
	__u32 rra = RRA;


	if (IS_XER_CA)
	{
		CLEAR_XER_CA;
		RRD = rra;
	}
	else
	{
		if (rra != 0)
			SET_XER_CA;
		RRD = rra - 1;
	}
}


// add to minus one extended (overflow)
OPCODE (ADDMEO)
{
	__u32 rra = RRA;


	if (IS_XER_CA)
	{
		CLEAR_XER_CA;
		CLEAR_XER_OV;
		RRD = rra;
	}
	else
	{
		if (rra != 0)
			SET_XER_CA;

		if (rra == 0x80000000)
			SET_XER_OV_SO;
		else
			CLEAR_XER_OV;

		RRD = rra - 1;
	}
}


// add to minus one extended (record)
OPCODE (ADDMED)
{
	__u32 rra = RRA;


	if (IS_XER_CA)
	{
		CLEAR_XER_CA;
		CALC_CR0 ((RRD = rra));
	}
	else
	{
		if (rra != 0)
			SET_XER_CA;
		CALC_CR0 ((RRD = rra - 1));
	}
}


// add to minus one extended (overflow, record)
OPCODE (ADDMEOD)
{
	__u32 rra = RRA;


	if (IS_XER_CA)
	{
		CLEAR_XER_CA;
		CLEAR_XER_OV;
		CALC_CR0 ((RRD = rra));
	}
	else
	{
		if (rra != 0)
			SET_XER_CA;

		if (rra == 0x80000000)
			SET_XER_OV_SO;
		else
			CLEAR_XER_OV;

		CALC_CR0 ((RRD = rra - 1));
	}
}


// add to zero extended
OPCODE (ADDZE)
{
	__u32 rra = RRA;


	if (IS_XER_CA)
	{
		if (rra != 0xffffffff)
			CLEAR_XER_CA;
		RRD = rra + 1;
	}
	else
		RRD = rra;
}


// add to zero extended (overflow)
OPCODE (ADDZEO)
{
	__u32 rra = RRA;


	if (IS_XER_CA)
	{
		if (rra != 0xffffffff)
			CLEAR_XER_CA;
		
		if (rra == 0x7fffffff)
			SET_XER_OV_SO;
		else
			CLEAR_XER_OV;

		RRD = rra + 1;
	}
	else
	{
		CLEAR_XER_OV;
		RRD = rra;
	}
}


// add to zero extended (record)
OPCODE (ADDZED)
{
	__u32 rra = RRA;


	if (IS_XER_CA)
	{
		if (rra != 0xffffffff)
			CLEAR_XER_CA;
		CALC_CR0 ((RRD = rra + 1));
	}
	else
		CALC_CR0 ((RRD = rra));
}


// add to zero extended (overflow, record)
OPCODE (ADDZEOD)
{
	__u32 rra = RRA;


	if (IS_XER_CA)
	{
		if (rra != 0xffffffff)
			CLEAR_XER_CA;

		if (rra == 0x7fffffff)
			SET_XER_OV_SO;
		else
			CLEAR_XER_OV;

		CALC_CR0 ((RRD = rra + 1));
	}
	else
	{
		CLEAR_XER_OV;
		CALC_CR0 ((RRD = rra));
	}
}


// and
OPCODE (AND)
{
	RRA = RRS & RRB;
}


// and (record)
OPCODE (ANDD)
{
	CALC_CR0 ((RRA = RRS & RRB));
}


// and with complement
OPCODE (ANDC)
{
	RRA = RRS & ~RRB;
}


// and with complement (record)
OPCODE (ANDCD)
{
	CALC_CR0 ((RRA = RRS & ~RRB));
}


// and immediate (record)
OPCODE (ANDID)
{
	CALC_CR0 ((RRA = RRS & UIMM));
}


// and immediate shifted (record)
OPCODE (ANDISD)
{
	CALC_CR0 ((RRA = RRS & (UIMM << 16)));
}


// branch (AA = 0, LK = 0)
OPCODE (B)
{
	PC += EXTS (26, LI) - 4;
}


// branch (AA = 1, LK = 0)
OPCODE (BA)
{
	PC = EXTS (26, LI) - 4;
}


// branch (AA = 0, LK = 1)
OPCODE (BL)
{
	LR = PC + 4;
	PC += EXTS (26, LI) - 4;
}


// branch (AA = 1, LK = 1)
OPCODE (BLA)
{
	LR = PC + 4;
	PC = EXTS (26, LI) - 4;
}


OPCODE (BX)
{
	OPCODE ((*b[4])) = {B, BL, BA, BLA};


	b[OPC & 3] (OPC);
}


// branch conditional (AA = 0, LK = 0)
OPCODE (BC)
{
	__u32 bo = BO;


	if (!BIT_IS_SET (bo, 5, 2))
		CTR--;

	if (BRANCH_CTR_OK (bo) && BRANCH_COND_OK (bo))
		PC += EXTS (16, BD) - 4;
}


// branch conditional (AA = 1, LK = 0)
OPCODE (BCA)
{
	__u32 bo = BO;


	if (!BIT_IS_SET (bo, 5, 2))
		CTR--;

	if (BRANCH_CTR_OK (bo) && BRANCH_COND_OK (bo))
		PC = BD - 4;
}


// branch conditional (AA = 0, LK = 1)
OPCODE (BCL)
{
	__u32 bo = BO;


	if (!BIT_IS_SET (bo, 5, 2))
		CTR--;

	if (BRANCH_CTR_OK (bo) && BRANCH_COND_OK (bo))
	{
		LR = PC + 4;
		PC += BD - 4;
	}
}


// branch conditional (AA = 1, LK = 1)
OPCODE (BCLA)
{
	__u32 bo = BO;


	if (!BIT_IS_SET (bo, 5, 2))
		CTR--;

	if (BRANCH_CTR_OK (bo) && BRANCH_COND_OK (bo))
	{
		LR = PC + 4;
		PC = BD - 4;
	}
}


OPCODE (BCX)
{
	OPCODE ((*b[4])) = {BC, BCL, BCA, BCLA};


	b[OPC & 3] (OPC);
}


// branch conditional to count register
OPCODE (BCCTR)
{
	__u32 bo = BO;
	

	if (BRANCH_COND_OK (bo))
		PC = (CTR &~ 3) - 4;
}


// branch conditional to count register
OPCODE (BCCTRL)
{
	__u32 bo = BO;


	if (BRANCH_COND_OK (bo))
	{
		LR = PC + 4;
		PC = (CTR &~ 3) - 4;
	}
}


// branch conditional to link register
OPCODE (BCLR)
{
	__u32 bo = BO;


	if (!BIT_IS_SET (bo, 5, 2))
		CTR--;

	if (BRANCH_CTR_OK (bo) && BRANCH_COND_OK (bo))
		PC = (LR &~ 3) - 4;
}


// branch conditional to link register
OPCODE (BCLRL)
{
	__u32 bo = BO, lr = LR;


	if (!BIT_IS_SET (bo, 5, 2))
		CTR--;

	if (BRANCH_CTR_OK (bo) && BRANCH_COND_OK (bo))
	{
		LR = PC + 4;
		PC = (lr &~ 3) - 4;
	}
}


// compare
OPCODE (CMP)
{
	CALC_CR (CRFD, ((__s32) RRA), ((__s32) RRB));
}


// compare immediate
OPCODE (CMPI)
{
	CALC_CR (CRFD, ((__s32) RRA), SIMM);
}


// compare logical
OPCODE (CMPL)
{
	CALC_CRL (CRFD, RRA, RRB);
}


// compare logical immediate
OPCODE (CMPLI)
{
	CALC_CRL (CRFD, RRA, UIMM);
}


// count leading zeros word
OPCODE (CNTLZW)
{
	RRA = BITSCAN (RRS);
}


// count leading zeros word (record)
OPCODE (CNTLZWD)
{
	CALC_CR0 ((RRA = BITSCAN (RRS)));
}


// condition register and
OPCODE (CRAND)
{
	CHANGE_BIT (CR, CRBD, BIT32 (CR, CRBA) & BIT32 (CR, CRBB));
}


// condition register and with complement
OPCODE (CRANDC)
{
	CHANGE_BIT (CR, CRBD, BIT32 (CR, CRBA) &~ BIT32 (CR, CRBB));
}


// condition register equivalent
OPCODE (CREQV)
{
	CHANGE_BIT (CR, CRBD, (BIT32 (CR, CRBA) == BIT32 (CR, CRBB)));
}


// condition register nand
OPCODE (CRNAND)
{
	CHANGE_BIT (CR, CRBD, ~(BIT32 (CR, CRBA) & BIT32 (CR, CRBB)));
}


// condition register nor
OPCODE (CRNOR)
{
	CHANGE_BIT (CR, CRBD, ~(BIT32 (CR, CRBA) | BIT32 (CR, CRBB)));
}


// condition register or
OPCODE (CROR)
{
	CHANGE_BIT (CR, CRBD, BIT32 (CR, CRBA) | BIT32 (CR, CRBB));
}


// condition register or with complement
OPCODE (CRORC)
{
	CHANGE_BIT (CR, CRBD, BIT32 (CR, CRBA) |~ BIT32 (CR, CRBB));
}


// condition register xor
OPCODE (CRXOR)
{
	CHANGE_BIT (CR, CRBD, BIT32 (CR, CRBA) ^ BIT32 (CR, CRBB));
}


// data cache clear to zero
OPCODE (DCBZ)
{
	__u32 x = RA;


	if (x)
		x = GPR[x] + RRB;
	else
		x = RRB;

	mem_block_zero (x);
}


// divide word
OPCODE (DIVW)
{
	__s32 rrb = RRB;


	if (rrb)
		RRD = ((__s32) RRA) / rrb;
}


// divide word (overflow)
OPCODE (DIVWO)
{
	__s32 rra = RRA, rrb = RRB;


	if (!rrb)
		SET_XER_OV_SO;
	else
	{
		if ((rra == (__s32) 0x80000000) && (rrb == 0x7fffffff))
			SET_XER_OV_SO;
		else
			CLEAR_XER_OV;

		RRD = rra / rrb;
	}
}


// divide word (record)
OPCODE (DIVWD)
{
	__s32 rrb = RRB;


	if (rrb)
		CALC_CR0 ((RRD = (__s32) RRA / rrb));
}


// divide word (overflow, record)
OPCODE (DIVWOD)
{
	__s32 rra = RRA, rrb = RRB;


	if (!rrb)
		SET_XER_OV_SO;
	else
	{
		if ((rra == (__s32) 0x80000000) && (rrb == 0x7fffffff))
			SET_XER_OV_SO;
		else
			CLEAR_XER_OV;

		CALC_CR0 ((RRD = rra / rrb));
	}
}


// divide word unsigned
OPCODE (DIVWU)
{
	__u32 rrb = RRB;


	if (rrb)
		RRD = RRA / rrb;
}


// divide word unsigned (overflow)
OPCODE (DIVWUO)
{
	__u32 rra = RRA, rrb = RRB;


	if (!rrb)
		SET_XER_OV_SO;
	else
	{
		if (rra == 0x80000000 && rrb == 0x7fffffff)
			SET_XER_OV_SO;
		else
			CLEAR_XER_OV;

		RRD = rra / rrb;
	}
}


// divide word unsigned (record)
OPCODE (DIVWUD)
{
	__u32 rrb = RRB;


	if (rrb)
		CALC_CR0 ((RRD = RRA / rrb));
}


// divide word unsigned (overflow, record)
OPCODE (DIVWUOD)
{
	__u32 rra = RRA, rrb = RRB;


	if (!rrb)
		SET_XER_OV_SO;
	else
	{
		if (rra == 0x80000000 && rrb == 0x7fffffff)
			SET_XER_OV_SO;
		else
			CLEAR_XER_OV;

		CALC_CR0 ((RRD = rra / rrb));
	}
}

//
// equivalent
OPCODE (EQV)
{
	RRA = ~(RRS ^ RRB);
}


// equivalent (record)
OPCODE (EQVD)
{
	CALC_CR0 ((RRA = ~(RRS ^ RRB)));
}


// extend sign byte
OPCODE (EXTSB)
{
	RRA = EXTS8 (RRS);
}


// extend sign byte (record)
OPCODE (EXTSBD)
{
	CALC_CR0 ((RRA = EXTS8 (RRS)));
}


// extend sign half word
OPCODE (EXTSH)
{
	RRA = EXTS16 (RRS);
}


// extend sign half word (record)
OPCODE (EXTSHD)
{
	CALC_CR0 ((RRA = EXTS16 (RRS)));
}


// load byte and zero
OPCODE (LBZ)
{
	unsigned int ra = RA;


	if (MEM_RBYTE (&RRD, (ra) ? (GPR[ra] + SIMM) : SIMM))
		RRD = (__u8) RRD;
}


// load byte and zero with update
OPCODE (LBZU)
{
	__u32 addr = RRA + SIMM;


	if (MEM_RBYTE (&RRD, addr))
	{
		RRA = addr;
		RRD = (__u8) RRD;
	}
}


// load byte and zero with update indexed
OPCODE (LBZUX)
{
	__u32 addr = RRA + RRB;


	if (MEM_RBYTE (&RRD, addr))
	{
		RRA = addr;
		RRD = (__u8) RRD;
	}
}


// load byte and zero indexed
OPCODE (LBZX)
{
	unsigned int ra = RA;


	if (MEM_RBYTE (&RRD, (ra) ? (GPR[ra] + RRB) : RRB))
		RRD = (__u8) RRD;
}


// load half word algebraic
OPCODE (LHA)
{
	unsigned int ra = RA;


	if (MEM_RHALF_S (&RRD, (ra) ? (GPR[ra] + SIMM) : SIMM))
		RRD = (__s16) RRD;
}


// load half word algebraic with update
OPCODE (LHAU)
{
	__u32 addr = RRA + SIMM;


	if (MEM_RHALF_S (&RRD, addr))
	{
		RRA = addr;
		RRD = (__s16) RRD;
	}
}


// load half word algebraic with update indexed
OPCODE (LHAUX)
{
	__u32 addr = RRA + RRB;


	if (MEM_RHALF_S (&RRD, addr))
	{
		RRA = addr;
		RRD = (__s16) RRD;
	}
}


// load half word algebraic indexed
OPCODE (LHAX)
{
	unsigned int ra = RA;


	if (MEM_RHALF_S (&RRD, (ra) ? (GPR[ra] + RRB) : RRB))
		RRD = (__s16) RRD;
}


// load half word byte-reverse indexed
OPCODE (LHBRX)
{
	unsigned int ra = RA;


	if (MEM_RHALF_SR (&RRD, (ra) ? (GPR[ra] + RRB) : RRB))
		RRD = (__s16) RRD;
}


// load half word and zero
OPCODE (LHZ)
{
	unsigned int ra = RA;


	if (MEM_RHALF (&RRD, (ra) ? (GPR[ra] + SIMM) : SIMM))
		RRD = (__u16) RRD;
}


// load half word and zero with update
OPCODE (LHZU)
{
	__u32 addr = RRA + SIMM;


	if (MEM_RHALF (&RRD, addr))
	{
		RRA = addr;
		RRD = (__u16) RRD;
	}
}


// load half word and zero with update indexed
OPCODE (LHZUX)
{
	__u32 addr = RRA + RRB;


	if (MEM_RHALF (&RRD, addr))
	{
		RRA = addr;
		RRD = (__u16) RRD;
	}
}


// load half word and zero indexed
OPCODE (LHZX)
{
	unsigned int ra = RA;


	if (MEM_RHALF (&RRD, (ra) ? (GPR[ra] + RRB) : RRB))
		RRD = (__u16) RRD;
}


// load multiple word
OPCODE (LMW)
{
	unsigned int ea = RA;
	int r;


	// effective address in RA
	ea = (ea) ? GPR[ea] + SIMM : SIMM;

	for (r = RD; r < 32; r++, ea += 4)
		if (!MEM_RWORD (&GPR[r], ea))
			return;
}


// load string word immediate
OPCODE (LSWI)
{
	unsigned int i, n = NB, ea = RA, rd = RD;


	// number of bytes to load in NB
	n = (n) ? n : 32;
	// effective address in RA
	ea = (ea) ? GPR[ea] : 0;

	for (i = 0; i < n/4; i++, rd++, ea += 4)
		if (!MEM_RWORD (&GPR[rd & 0x1f], ea))
			return;

	// load (n%4) bytes into the register and clear other 4-(n%4) bytes
	if (n & 3)
	{
		rd &= 0x1f;
		if (MEM_RWORD (&GPR[rd], ea))
			GPR[rd] &= ~(0xffffffff << ((n & 3) << 3));
	}
}


// load string word indexed
OPCODE (LSWX)
{
	unsigned int i, n = XER_BC, ea = RA, rd = RD;


	// effective address in RA
	ea = (ea) ? (GPR[ea] + RRB) : RRB;

	for (i = 0; i < n/4; i++, rd++, ea += 4)
		if (!MEM_RWORD (&GPR[rd & 0x1f], ea))
			return;

	// load (n%4) bytes into the register and clear other 4-(n%4) bytes
	if (n & 3)
	{
		rd &= 0x1f;
		if (MEM_RWORD (&GPR[rd], ea))
			GPR[rd] &= ~(0xffffffff << ((n & 3) << 3));
	}
}


// load word and reserve indexed
OPCODE (LWARX)
{
	unsigned int ra = RA;
	__u32 ea;


	ea = (ra) ? (GPR[ra] + RRB) : RRB;
	MEM_RWORD (&RRD, ea);

	RESERVE = 1;
	RESERVE_ADDR = ea;
}


// load word byte-reverse indexed
OPCODE (LWBRX)
{
	unsigned int ra = RA;


	MEM_RWORD_R (&RRD, (ra) ? (GPR[ra] + RRB) : RRB);
}


// load word and zero
OPCODE (LWZ)
{
	unsigned int ra = RA;


	MEM_RWORD (&RRD, (ra) ? (GPR[ra] + SIMM) : SIMM);
}


// load word and zero with update
OPCODE (LWZU)
{
	__u32 addr = RRA + SIMM;


	if (MEM_RWORD (&RRD, addr))
		RRA = addr;
}


// load word and zero with update indexed
OPCODE (LWZUX)
{
	__u32 addr = RRA + RRB;


	if (MEM_RWORD (&RRD, addr))
		RRA = addr;
}


// load word and zero indexed
OPCODE (LWZX)
{
	unsigned int ra = RA;


	MEM_RWORD (&RRD, (ra) ? (GPR[ra] + RRB) : RRB);
}


// move condition register field
OPCODE (MCRF)
{
	SET_FIELD (CR, CRFD, GET_FIELD (CR, CRFS));
}


// move to condition register from FPSCR
OPCODE (MCRFS)
{
	__u32 fpscr = FPSCR, crfs = CRFS;


	SET_FIELD (CR, CRFD, GET_FIELD (FPSCR, crfs));
	CLEAR_FIELD (FPSCR, crfs);
	FPSCR |= fpscr & (FPSCR_FEX | FPSCR_VX);
}


// move to condition register from XER
OPCODE (MCRXR)
{
	SET_FIELD (CR, CRFD, GET_XER_03);
	SET_XER_03 (0);
}


// move from condition register
OPCODE (MFCR)
{
	RRD = CR;
}


// move from machine state register
OPCODE (MFMSR)
{
	RRD = MSR;
}


// move from special-purpose register
OPCODE (MFSPR)
{
	RRD = SPR[(RB << 5) | RA];
}


// move from segment register
OPCODE (MFSR)
{
	RRD = SR[RA & 0x0f];
}


// move from segment register indirect
OPCODE (MFSRIN)
{
	RRD = SR[RRB >> 28];
}


// move from time base
OPCODE (MFTB)
{
	int n = (RB << 5) | RA;


	if (n == 268)
		RRD = TBL;
	else if (n == 269)
		RRD = TBU;
}


// move to condition register fields
OPCODE (MTCRF)
{
	__u32 crm = CRM;
	unsigned int mask = 0, i;


	for (i = 0; i < 8; i++, crm >>= 1)
	{
		mask >>= 4;

		if (crm & 1)
			mask |= 0xf0000000;
	}

	CR = (RRS & mask) | (CR &~ mask);
}


// move to machine state register
OPCODE (MTMSR)
{
	MSR = RRS;

//	pi_check_for_interrupts ();
}


// move to special-purpose register
OPCODE (MTSPR)
{
	__u32 spr = (RB << 5) | RA;


	SPR[spr] = RRS;

	switch (spr)
	{
		// XER
		case 1:
		// LR
		case 8:
		// CTR
		case 9:
		// DEC
		case 22:
		// SRR0
		case 26:
		// SRR1
		case 27:
		// SPRG0
		case 272:
		// SPRG1
		case 273:
		// SPRG2
		case 274:
		// SPRG3
		case 275:
		// GQR0 - GQR7
		case 912:
		case 913:
		case 914:
		case 915:
		case 916:
		case 917:
		case 918:
		case 919:
		// HID2
		case 920:
		// DMAU
		case 922:
		// MMCR0 - monitor mode control
		case 952:
		// PMC1 - performance monitor counter
		case 953:
		// PMC2
		case 954:
		// SIA - sampled instruction address
		case 955:
		// MMCR1
		case 956:
		// PMC3
		case 957:
		// PMC4
		case 958:
		// SDA *
		case 959:
		// HID0
		case 1008:
		// HID1
		case 1009:
		// L2CR
		case 1017:
			break;

		// IBAT0U - instruction block address translation
		case 528:
		// IBAT0L
		case 529:
		// IBAT1U
		case 530:
		// IBAT1L
		case 531:
		// IBAT2U
		case 532:
		// IBAT2L
		case 533:
		// IBAT3U
		case 534:
		// IBAT3L
		case 535:
		// DBAT0U - data block address translation
		case 536:
		// DBAT0L
		case 537:
		// DBAT1U
		case 538:
		// DBAT1L
		case 539:
		// DBAT2U
		case 540:
		// DBAT2L
		case 541:
		// DBAT3U
		case 542:
		// DBAT3L
		case 543:
			break;

		
		// SDR1
		case 25:
			break;
	
		// WPAR
		case 921:
			cp_wpar_redirect (RRS);
			break;
	
		// DMAL
		case 923:
			if (LCE && DMA_T)
			{
				__u32 len = (DMA_LEN) ? DMA_LEN : 128;

				if (DMA_LD)
					// transfer from MEM to LC
					memcpy (L2C_ADDRESS (DMA_LC_ADDR), MEM_ADDRESS (DMA_MEM_ADDR), len << 5);
				else
					// transfer from LC to MEM
					memcpy (MEM_ADDRESS (DMA_MEM_ADDR), L2C_ADDRESS (DMA_LC_ADDR), len << 5);
			}
			break;
		
		// MSR
		case 49:
//			pi_check_for_interrupts ();
			break;
		
		default:
			DEBUG (EVENT_EMAJOR, ".cpu: unemulated mtspr %d", spr);
			break;
	}
}


// move to segment register
OPCODE (MTSR)
{
	SR[RA & 0x0f] = RRS;
}


// move to segment register indirect
OPCODE (MTSRIN)
{
	SR[RRB >> 28] = RRS;
}


// multiply high word
OPCODE (MULHW)
{
	RRD = (__s32) (((__s64) ((__s32) RRA) * ((__s32) RRB)) >> 32);
}


// multiply high word (record)
OPCODE (MULHWD)
{
	CALC_CR0 ((RRD = (__s32) (((__s64) ((__s32) RRA) * ((__s32) RRB)) >> 32)));
}


// multiply high word unsigned
OPCODE (MULHWU)
{
	RRD = (__u32) (((__u64) RRA * RRB) >> 32);
}


// multiply high word unsigned (record)
OPCODE (MULHWUD)
{
	CALC_CR0 ((RRD = (__u32) (((__u64) RRA * RRB) >> 32)));
}


// multiply low immediate
OPCODE (MULLI)
{
	RRD = ((__s32) RRA) * SIMM;
}


// multiply low word
OPCODE (MULLW)
{
	RRD = ((__s32) RRA) * ((__s32) RRB);
}


// multiply low word (overflow)
OPCODE (MULLWO)
{
	__u64 res = ((__s32) RRA) * ((__s32) RRB);
	

	if (res & 0xffffffff00000000ULL)
		SET_XER_OV_SO;
	else
		CLEAR_XER_OV;

	RRD = (__u32) res;
}


// multiply low word (record)
OPCODE (MULLWD)
{
	CALC_CR0 ((RRD = ((__s32) RRA) * ((__s32) RRB)));
}


// multiply low word (overflow, record)
OPCODE (MULLWOD)
{
	__u64 res =  ((__s32) RRA) * ((__s32) RRB);
	

	if (res & 0xffffffff00000000ULL)
		SET_XER_OV_SO;
	else
		CLEAR_XER_OV;

	CALC_CR0 ((RRD = (__u32) res));
}


// nand
OPCODE (NAND)
{
	RRA = ~(RRS & RRB);
}


// nand (record)
OPCODE (NANDD)
{
	CALC_CR0 ((RRA = ~(RRS & RRB)));
}


// negate
OPCODE (NEG)
{
	RRD = -RRA;
}


// negate (overflow)
OPCODE (NEGO)
{
	__s32 rra = RRA;


	if (rra == (__s32) 0x80000000)
		SET_XER_OV_SO;
	else
		CLEAR_XER_OV;

	RRD = -rra;
}


// negate (record)
OPCODE (NEGD)
{
	CALC_CR0 ((RRD = -RRA));
}


// negate (overflow, record)
OPCODE (NEGOD)
{
	__s32 rra = RRA;


	if (rra == (__s32) 0x80000000)
		SET_XER_OV_SO;
	else
		CLEAR_XER_OV;

	CALC_CR0 ((RRD = -rra));
}


// nor
OPCODE (NOR)
{
	RRA = ~(RRS | RRB);
}


// nor (record)
OPCODE (NORD)
{
	CALC_CR0 ((RRA = ~(RRS | RRB)));
}


// or
OPCODE (OR)
{
	RRA = RRS | RRB;
}


// or (record)
OPCODE (ORD)
{
	CALC_CR0 ((RRA = RRS | RRB));
}


// or with complement
OPCODE (ORC)
{
	RRA = RRS | ~RRB;
}


// or with complement (record)
OPCODE (ORCD)
{
	CALC_CR0 ((RRA = RRS | ~RRB));
}


// or immediate
OPCODE (ORI)
{
	RRA = RRS | UIMM;
}


// or immediate shifted
OPCODE (ORIS)
{
	RRA = RRS | (UIMM << 16);
}


// return from interrupt
#define RFI_MSR_SAVE_MASK		0x87c0ff73
OPCODE (RFI)
{
	MSR = (MSR &~ RFI_MSR_SAVE_MASK) | (SRR1 & RFI_MSR_SAVE_MASK);
	CLEAR_BIT (MSR, 13);
	PC = (SRR0 &~ 3) - 4;
	
//	pi_check_for_interrupts ();
}


// rotate left word immediate then mask insert (+record)
OPCODE (RLWIMIX)
{
	__u32 ra = RA;
	unsigned int m;


	m = MASK (MB, ME);
	GPR[ra] = (ROTL (RRS, SH) & m) | (GPR[ra] &~ m);

	if (OPC & 1)
		CALC_CR0 (GPR[ra]);
}


// rotate left word immediate then and with mask (+record)
OPCODE (RLWINMX)
{
	RRA = ROTL (RRS, SH) & MASK (MB, ME);

	if (OPC & 1)
		CALC_CR0 (RRA);
}


// rotate left word then and with mask (+record)
OPCODE (RLWNMX)
{
	RRA = ROTL (RRS, RRB) & MASK (MB, ME);

	if (OPC & 1)
		CALC_CR0 (RRA);
}


// system call
OPCODE (SC)
{
	cpu_exception (EXCEPTION_SYSTEM_CALL);
}


// shift left word
OPCODE (SLW)
{
	unsigned int n = RRB;


	RRA = (n < 32) ? RRS << n : 0;
}


// shift left word (record)
OPCODE (SLWD)
{
	unsigned int n = RRB;


	if (n < 32)
		CALC_CR0 ((RRA = RRS << n));
	else
		CALC_CR0 ((RRA = 0));
}


// shift right algebraic word
OPCODE (SRAW)
{
	unsigned int n = RRB & 0x3f;
	__s32 rrs = RRS;


	if (n == 0)
	{
		RRA = rrs;
		CLEAR_XER_CA;
	}
	else if (n < 32)
	{
		RRA = rrs >> n;
		CHANGE_XER_CA (((rrs < 0) && (rrs << (32 - n))));
	}
	else
	{
		if (rrs < 0)
		{
			RRA = 0xffffffff;
			CHANGE_XER_CA ((rrs & 0x7fffffff));
		}
		else
		{
			RRA = 0;
			CLEAR_XER_CA;
		}
	}
}


// shift right algebraic word (record)
OPCODE (SRAWD)
{
	unsigned int n = RRB;
	__s32 rrs = RRS;


	if (n == 0)
	{
		CALC_CR0 ((RRA = rrs));
		CLEAR_XER_CA;
	}
	else if (n < 32)
	{
		CALC_CR0 ((RRA = rrs >> n));
		CHANGE_XER_CA (((rrs < 0) && (rrs << (32 - n))));
	}
	else
	{
		if (rrs < 0)
		{
			CALC_CR0 ((RRA = 0xffffffff));
			CHANGE_XER_CA ((rrs & 0x7fffffff));
		}
		else
		{
			CALC_CR0 ((RRA = 0));
			CLEAR_XER_CA;
		}
	}
}


// shift right algebraic word immediate
OPCODE (SRAWI)
{
	unsigned int n = SH;
	__s32 rrs = RRS;


	if (n == 0)
	{
		RRA = rrs;
		CLEAR_XER_CA;
	}
	else
	{
		RRA = rrs >> n;
		CHANGE_XER_CA (((rrs < 0) && (rrs << (32 - n))));
	}
}


// shift right algebraic word immediate (record)
OPCODE (SRAWID)
{
	unsigned int n = SH;
	__s32 rrs = RRS;


	if (n == 0)
	{
		CALC_CR0 ((RRA = rrs));
		CLEAR_XER_CA;
	}
	else
	{
		CALC_CR0 ((RRA = rrs >> n));
		CHANGE_XER_CA (((rrs < 0) && (rrs << (32 - n))));
	}
}


// shift right word
OPCODE (SRW)
{
	unsigned int n = RRB;


	RRA = (n < 32) ? RRS >> n : 0;
}


// shift right word (record)
OPCODE (SRWD)
{
	unsigned int n = RRB;


	if (n < 32)
		CALC_CR0 ((RRA = RRS >> n));
	else
		CALC_CR0 ((RRA = 0));
}


// store byte
OPCODE (STB)
{
	unsigned int ra = RA;


	MEM_WBYTE ((ra) ? (GPR[ra] + SIMM) : SIMM, RRS);
}


// store byte with update
OPCODE (STBU)
{
	__u32 addr = RRA + SIMM;


	if (MEM_WBYTE (addr, RRS))
		RRA = addr;
}


// store byte with update indexed
OPCODE (STBUX)
{
	__u32 addr = RRA + RRB;


	if (MEM_WBYTE (addr, RRS))
		RRA = addr;
}


// store byte indexed
OPCODE (STBX)
{
	unsigned int ra = RA;


	MEM_WBYTE ((ra) ? (GPR[ra] + RRB) : RRB, RRS);
}


// store half word
OPCODE (STH)
{
	unsigned int ra = RA;


	MEM_WHALF ((ra) ? (GPR[ra] + SIMM) : SIMM, RRS);
}


// store half word byte-reverse indexed
OPCODE (STHBRX)
{
	unsigned int ra = RA;


	MEM_WHALF_R ((ra) ? (GPR[ra] + RRB) : RRB, RRS);
}


// store half word with update
OPCODE (STHU)
{
	__u32 addr = RRA + SIMM;


	if (MEM_WHALF (addr, RRS))
		RRA = addr;
}


// store half word with update indexed
OPCODE (STHUX)
{
	__u32 addr = RRA + RRB;


	if (MEM_WHALF (addr, RRS))
		RRA = addr;
}


// store half word indexed
OPCODE (STHX)
{
	unsigned int ra = RA;


	MEM_WHALF ((ra) ? (GPR[ra] + RRB) : RRB, RRS);
}


// store multiple word
OPCODE (STMW)
{
	unsigned int ea = RA;
	int r;


	// effective address in RA
	ea = (ea) ? GPR[ea] + SIMM : SIMM;

	for (r = RS; r < 32; r++, ea += 4)
		if (!MEM_WWORD (ea, GPR[r]))
			return;
}


// store string word immediate
OPCODE (STSWI)
{
	unsigned int i, n = NB, ea = RA, rd = RD;
	char *cr;


	// number of bytes to store in NB
	n = (n) ? n : 32;
	// effective address in RA
	ea = (ea) ? GPR[ea] : 0;

	for (i = 0; i < n/4; i++, rd++, ea += 4)
		if (!MEM_WWORD (ea, GPR[rd & 0x1f]))
			return;

	cr = (char *) &GPR[rd & 0x1f];
	for (i = 0; i < (n & 3); i++, ea++, cr++)
		if (!MEM_WBYTE (ea, *cr))
			return;
}


// store string word indexed
OPCODE (STSWX)
{
	unsigned int i, n = XER_BC, ea = RA, rd = RD;
	char *cr;


	// effective address in RA
	ea = (ea) ? (GPR[ea] + RRB) : RRB;

	for (i = 0; i < n/4; i++, rd++, ea += 4)
		if (!MEM_WWORD (ea, GPR[rd & 0x1f]))
			return;

	cr = (char *) &GPR[rd & 0x1f];
	for (i = 0; i < (n & 3); i++, ea++, cr++)
		if (!MEM_WBYTE (ea, *cr))
			return;
}


// store word
OPCODE (STW)
{
	unsigned int ra = RA;


	MEM_WWORD ((ra) ? (GPR[ra] + SIMM) : SIMM, RRS);
}


// store word byte-reverse indexed
OPCODE (STWBRX)
{
	unsigned int ra = RA;


	MEM_WWORD_R ((ra) ? (GPR[ra] + RRB) : RRB, RRS);
}


// store word conditional indexed
OPCODE (STWCXD)
{
	unsigned int ra = RA;
	__u32 ea;


	ea = (ra) ? (GPR[ra] + RRB) : RRB;
	if (RESERVE && RESERVE_ADDR == ea)
	{
		if (!MEM_WWORD (ea, RRS))
			return;

		SET_CR0 ((RESERVE << 1) | IS_XER_SO);
		RESERVE = 0;
	}
	else
		SET_CR0 (IS_XER_SO);
}


// store word with update
OPCODE (STWU)
{
	__u32 addr = RRA + SIMM;


	if (MEM_WWORD (addr, RRS))
		RRA = addr;
}


// store word with update indexed
OPCODE (STWUX)
{
	__u32 addr = RRA + RRB;


	if (MEM_WWORD (addr, RRS))
		RRA = addr;
}


// store word indexed
OPCODE (STWX)
{
	unsigned int ra = RA;


	MEM_WWORD ((ra) ? (GPR[ra] + RRB) : RRB, RRS);
}


// subtract from
OPCODE (SUBF)
{
	RRD = RRB - RRA;
}


// subtract from (overflow)
OPCODE (SUBFO)
{
	__u32 rra = RRA, rrb = RRB;


	CALC_XER_OV_U (rrb, rra);
	RRD = rrb - rra;
}


// subtract from (record)
OPCODE (SUBFD)
{
	CALC_CR0 ((RRD = RRB - RRA));
}


// subtract from (overflow, record)
OPCODE (SUBFOD)
{
	__u32 rra = RRA, rrb = RRB;


	CALC_XER_OV_U (rrb, rra);
	CALC_CR0 ((RRD = rrb - rra));
}


// subtract from carrying
OPCODE (SUBFC)
{
	__u32 rra = RRA, rrb = RRB;


	if (rra)
		CALC_XER_CA (rrb, -rra);
	else
		SET_XER_CA;

	RRD = rrb - rra;
}


// subtract from carrying (overflow)
OPCODE (SUBFCO)
{
	__u32 rra = RRA, rrb = RRB;


	if (rra)
		CALC_XER_CA (rrb, -rra);
	else
		SET_XER_CA;

	CALC_XER_OV_U (rrb, rra);
	RRD = rrb - rra;
}


// subtract from carrying (record)
OPCODE (SUBFCD)
{
	__u32 rra = RRA, rrb = RRB;


	if (rra)
		CALC_XER_CA (rrb, -rra);
	else
		SET_XER_CA;

	CALC_CR0 ((RRD = rrb - rra));
}


// subtract from carrying (overflow, record)
OPCODE (SUBFCOD)
{
	__u32 rra = RRA, rrb = RRB;


	if (rra)
		CALC_XER_CA (rrb, -rra);
	else
		SET_XER_CA;

	CALC_XER_OV_U (rrb, rra);
	CALC_CR0 ((RRD = rrb - rra));
}


// subtract from extended
OPCODE (SUBFE)
{
	__u32 rra = ~RRA, rrb = RRB, c = IS_XER_CA;


	CALC_XER_CA_E (rra, rrb, c);
	RRD = rra + rrb + c;
}


// subtract from extended (overflow)
OPCODE (SUBFEO)
{
	__u32 rra = ~RRA, rrb = RRB, c = IS_XER_CA;


	CALC_XER_CA_E (rrb, rra, c);
	CALC_XER_OV_E (rrb, rra, c);
	RRD = rra + rrb + c;
}


// subtract from extended (record)
OPCODE (SUBFED)
{
	__u32 rra = ~RRA, rrb = RRB, c = IS_XER_CA;


	CALC_XER_CA_E (rra, rrb, c);
	CALC_CR0 ((RRD = rra + rrb + c));
}


// subtract from extended (overflow, record)
OPCODE (SUBFEOD)
{
	__u32 rra = ~RRA, rrb = RRB, c = IS_XER_CA;


	CALC_XER_CA_E (rrb, rra, c);
	CALC_XER_OV_E (rrb, rra, c);
	CALC_CR0 ((RRD = rra + rrb + c));
}


// subtract from immediate carrying
OPCODE (SUBFIC)
{
	__u32 rra = ~RRA;
	__s32 simm = SIMM;


	CALC_XER_CA_E (rra, simm, 1);
	RRD = rra + simm + 1;
}


// subtract from minus one extended
OPCODE (SUBFME)
{
	__u32 rra = ~RRA;


	if (IS_XER_CA)
	{
		CLEAR_XER_CA;
		RRD = rra;
	}
	else
	{
		if (rra != 0)
			SET_XER_CA;
		RRD = rra - 1;
	}
}


// subtract from minus one extended (overflow)
OPCODE (SUBFMEO)
{
	__u32 rra = ~RRA;


	if (IS_XER_CA)
	{
		CLEAR_XER_CA;
		CLEAR_XER_OV;
		RRD = rra;
	}
	else
	{
		if (rra != 0)
			SET_XER_CA;

		if (rra == 0x80000000)
			SET_XER_OV_SO;
		else
			CLEAR_XER_OV;

		RRD = rra - 1;
	}
}


// subtract from minus one extended (record)
OPCODE (SUBFMED)
{
	__u32 rra = ~RRA;


	if (IS_XER_CA)
	{
		CLEAR_XER_CA;
		CALC_CR0 ((RRD = rra));
	}
	else
	{
		if (rra != 0)
			SET_XER_CA;
		CALC_CR0 ((RRD = rra - 1));
	}
}


// subtract from minus one extended (overflow, record)
OPCODE (SUBFMEOD)
{
	__u32 rra = ~RRA;


	if (IS_XER_CA)
	{
		CLEAR_XER_CA;
		CLEAR_XER_OV;
		CALC_CR0 ((RRD = rra));
	}
	else
	{
		if (rra != 0)
			SET_XER_CA;

		if (rra == 0x80000000)
			SET_XER_OV_SO;
		else
			CLEAR_XER_OV;

		CALC_CR0 ((RRD = rra - 1));
	}
}


// subtract from zero extended
OPCODE (SUBFZE)
{
	__u32 rra = ~RRA;


	if (IS_XER_CA)
	{
		if (rra != 0xffffffff)
			CLEAR_XER_CA;
		RRD = rra + 1;
	}
	else
		RRD = rra;
}


// subtract from zero extended (overflow)
OPCODE (SUBFZEO)
{
	__u32 rra = ~RRA;


	if (IS_XER_CA)
	{
		if (rra != 0xffffffff)
			CLEAR_XER_CA;

		if (rra == 0x7fffffff)
			SET_XER_OV_SO;
		else
			CLEAR_XER_OV;

		RRD = rra + 1;
	}
	else
	{
		CLEAR_XER_OV;
		RRD = rra;
	}
}


// subtract from zero extended (record)
OPCODE (SUBFZED)
{
	__u32 rra = ~RRA;


	if (IS_XER_CA)
	{
		if (rra != 0xffffffff)
			CLEAR_XER_CA;
		CALC_CR0 ((RRD = rra + 1));
	}
	else
		CALC_CR0 ((RRD = rra));
}


// subtract from zero extended (overflow, record)
OPCODE (SUBFZEOD)
{
	__u32 rra = ~RRA;


	if (IS_XER_CA)
	{
		if (rra != 0xffffffff)
			CLEAR_XER_CA;

		if (rra == 0x7fffffff)
			SET_XER_OV_SO;
		else
			CLEAR_XER_OV;

		CALC_CR0 ((RRD = rra + 1));
	}
	else
	{
		CLEAR_XER_OV;
		CALC_CR0 ((RRD = rra));
	}
}


// xor
OPCODE (XOR)
{
	RRA = RRS ^ RRB;
}


// xor (record)
OPCODE (XORD)
{
	CALC_CR0 ((RRA = RRS ^ RRB));
}


// xor immediate
OPCODE (XORI)
{
	RRA = RRS ^ UIMM;
}


// xor immediate shifted
OPCODE (XORIS)
{
	RRA = RRS ^ (UIMM << 16);
}



// floating point instructions


// floating absolute value
OPCODE (FABS)
{
	FBD = FBB &~ (1ULL << 63);
}


// floating add (double-precision)
OPCODE (FADD)
{
	FRD = FRA + FRB;
}


// floating add single
OPCODE (FADDS)
{
	if (PSE)
		PS0D = PS1D = (float) (PS0A + PS0B);
	else
		FRD = (float) (FRA + FRB);
}


// floating compare ordered
OPCODE (FCMPO)
{
	FPSCR &= ~(0x1f << FPSCR_FPCC);

	if (IS_NAN (FBA) || IS_NAN (FBB))
	{
		SET_FIELD (CR, CRFD, 1);
		FPSCR |= 1 << FPSCR_FPCC;

		if (IS_SNAN (FBA) || IS_SNAN (FBB))
			FPSCR |= FPSCR_VXSNAN;
		else if (!(FPSCR & FPSCR_VE) || IS_QNAN (FBA) || IS_QNAN (FBB))
			FPSCR |= FPSCR_VXVC;
	}
	else
		CALC_CRF (CRFD, FRA, FRB);
}


// floating compare unordered
OPCODE (FCMPU)
{
	FPSCR &= ~(0x1f << FPSCR_FPCC);

	if (IS_NAN (FBA) || IS_NAN (FBB))
	{
		SET_FIELD (CR, CRFD, 1);
		FPSCR |= 1 << FPSCR_FPCC;

		if (IS_SNAN (FBA) || IS_SNAN (FBB))
			FPSCR |= FPSCR_VXSNAN;
	}
	else
		CALC_CRF (CRFD, FRA, FRB);
}


// floating convert to integer word
OPCODE (FCTIW)
{
	double frb = FRB;


	if (frb >= MAX_S32)
		FBD = (__u32) MAX_S32;
	else if (frb < MIN_S32)
		FBD = (__u64) (__u32) MIN_S32;
	else
//		FBD = (__u64) (__u32) (__s32) FP_ROUND (frb);
		FBD = (__u64) (__u32) (__s32) frb;
}


// floating convert to integer word with round toward zero
OPCODE (FCTIWZ)
{
/*
	double frb = FRB;


	if (frb >= MAX_S32)
		FBD = (__u32) MAX_S32;
	else if (frb < MIN_S32)
		FBD = (__u32) MIN_S32;
	else
//		FBD = (__u32) (__s32) trunc (frb);
		FBD = (__u32) (__s32) frb;
*/
	FBD = (__u64)(__u32)(__s32) FRB;
}


// floating divide (double-precision)
OPCODE (FDIV)
{
	FRD = FRA / FRB;
}


// floating divide single
OPCODE (FDIVS)
{
	if (PSE)
		PS0D = PS1D = (float) (PS0A / PS0B);
	else
		FRD = (float) (FRA / FRB);
}


// floating multiply-add (double-precision)
OPCODE (FMADD)
{
	FRD = FRA * FRC + FRB;
}


// floating multiply-add single
OPCODE (FMADDS)
{
	if (PSE)
		PS0D = PS1D = (float) (PS0A * PS0C + PS0B);
	else
		FRD = (float) (FRA * FRC + FRB);
}


// floating move register
OPCODE (FMR)
{
	FBD = FBB;
}


// floating multiply-subtract (double-precision)
OPCODE (FMSUB)
{
	FRD = (FRA * FRC) - FRB;
}


// floating multiply-subtract single
OPCODE (FMSUBS)
{
	if (PSE)
		PS0D = PS1D = (float) (PS0A * PS0C - PS0B);
	else
		FRD = (float) (FRA * FRC - FRB);
}


// floating multiply (double-precision)
OPCODE (FMUL)
{
	FRD = FRA * FRC;
}


// floating multiply single
OPCODE (FMULS)
{
	if (PSE)
		PS0D = PS1D = (float) (PS0A * PS0C);
	else
		FRD = (float) (FRA * FRC);
}


// floating negative absolute value
OPCODE (FNABS)
{
	FBD = FBB | (1ULL << 63);
}


// floating negate
OPCODE (FNEG)
{
	FBD = FBB ^ (1ULL << 63);
}


// floating negative multiply-add (double-precision)
OPCODE (FNMADD)
{
	FRD = -(FRA * FRC + FRB);
}


// floating negative multiply-add single
OPCODE (FNMADDS)
{
	if (PSE)
		PS0D = PS1D = (float) -(PS0A * PS0C + PS0B);
	else
		FRD = (float) -(FRA * FRC + FRB);
}


// floating negative multiply-subtract (double-precision)
OPCODE (FNMSUB)
{
	FRD = -(FRA * FRC - FRB);
}


// floating negative multiply-subtract single
OPCODE (FNMSUBS)
{
	if (PSE)
		PS0D = PS1D = (float) -(PS0A * PS0C - PS0B);
	else
		FRD = (float) -(FRA * FRC - FRB);
}


// floating reciprocal estimate single
OPCODE (FRES)
{
	if (PSE)
		PS0D = PS1D = (float) (1.0 / PS0B);
	else
		FRD = (float) (1.0 / FRB);
}


// floating round to single
OPCODE (FRSP)
{
	FRD = (float) FRB;
	
	// fix from dolphin sources
	PS1D = PS0D;
}


// floating reciprocal square root estimate
OPCODE (FRSRTE)
{
	FRD = 1.0 / SQRT (FRB);
}


// floating select
OPCODE (FSEL)
{
	FRD = (FRA >= 0.0) ? FRC : FRB;
}


// floating square root (double-precision)
OPCODE (FSQRT)
{
	FRD = SQRT (FRB);
}


// floating square root single
OPCODE (FSQRTS)
{
	FRD = (float) SQRT (FRB);
}


// floating subtract (double-precision)
OPCODE (FSUB)
{
	FRD = FRA - FRB;
}


// floating subtract single
OPCODE (FSUBS)
{
	if (PSE)
		PS0D = PS1D = (float) (PS0A - PS0B);
	else
		FRD = (float) (FRA - FRB);
}


// load floating-point double
OPCODE (LFD)
{
	unsigned int ra = RA;


	CHECK_FOR_FPU;
	MEM_RDOUBLE (&FRD, (ra) ? (GPR[ra] + SIMM) : SIMM);
}


// load floating-point double with update
OPCODE (LFDU)
{
	__u32 addr = RRA + SIMM;


	CHECK_FOR_FPU;
	if (MEM_RDOUBLE (&FRD, addr))
		RRA = addr;
}


// load floating-point double with update indexed
OPCODE (LFDUX)
{
	__u32 addr = RRA + RRB;


	CHECK_FOR_FPU;
	if (MEM_RDOUBLE (&FRD, addr))
		RRA = addr;
}


// load floating-point double indexed
OPCODE (LFDX)
{
	unsigned int ra = RA;


	CHECK_FOR_FPU;
	MEM_RDOUBLE (&FRD, (ra) ? (GPR[ra] + RRB) : RRB);
}


// load floating-point single
OPCODE (LFS)
{
	unsigned int ra = RA;


	CHECK_FOR_FPU;
	if (PSE)
	{
		MEM_RSINGLE (&PS0D, (ra) ? (GPR[ra] + SIMM) : SIMM);
		PS1D = PS0D;
	}
	else
		MEM_RSINGLE (&FRD, (ra) ? (GPR[ra] + SIMM) : SIMM);
}


// load floating-point single with update
OPCODE (LFSU)
{
	__u32 addr = RRA + SIMM;


	CHECK_FOR_FPU;
	if (PSE)
	{
		if (MEM_RSINGLE (&PS0D, addr))
		{
			RRA = addr;
			PS1D = PS0D;
		}
	}
	else
	{
		if (MEM_RSINGLE (&FRD, addr))
			RRA = addr;
	}
}


// load floating-point single with update indexed
OPCODE (LFSUX)
{
	__u32 addr = RRA + RRB;


	CHECK_FOR_FPU;
	if (PSE)
	{
		if (MEM_RSINGLE (&PS0D, addr))
		{
			PS1D = PS0D;
			RRA = addr;
		}
	}
	else
	{
		if (MEM_RSINGLE (&FRD, addr))
			RRA = addr;
	}
}


// load floating-point single indexed
OPCODE (LFSX)
{
	unsigned int ra = RA;


	CHECK_FOR_FPU;
	if (PSE)
	{
		MEM_RSINGLE (&PS0D, (ra) ? (GPR[ra] + RRB) : RRB);
		PS1D = PS0D;
	}
	else
		MEM_RSINGLE (&FRD, (ra) ? (GPR[ra] + RRB) : RRB);
}


// store floating-point double
OPCODE (STFD)
{
	unsigned int ra = RA;


	CHECK_FOR_FPU;
	MEM_WDOUBLE ((ra) ? (GPR[ra] + SIMM) : SIMM, FRS);
}


// store floating-point double with update
OPCODE (STFDU)
{
	__u32 addr = RRA + SIMM;


	CHECK_FOR_FPU;
	if (MEM_WDOUBLE (addr, FRS))
		RRA = addr;
}


// store floating-point double with update indexed
OPCODE (STFDUX)
{
	__u32 addr = RRA + RRB;


	CHECK_FOR_FPU;
	if (MEM_WDOUBLE (addr, FRS))
		RRA = addr;
}


// store floating-point double indexed
OPCODE (STFDX)
{
	unsigned int ra = RA;


	CHECK_FOR_FPU;
	MEM_WDOUBLE ((ra) ? (GPR[ra] + RRB) : RRB, FRS);
}


// store floating-point as integer word indexed
OPCODE (STFIWX)
{
	unsigned int ra = RA;


	CHECK_FOR_FPU;
	MEM_WWORD ((ra) ? (GPR[ra] + RRB) : RRB, (__u32) FBS);
}


// store floating-point single
OPCODE (STFS)
{
	unsigned int ra = RA;


	CHECK_FOR_FPU;
	MEM_WSINGLE ((ra) ? (GPR[ra] + SIMM) : SIMM, (float) FRS);
}


// store floating-point single with update
OPCODE (STFSU)
{
	__u32 addr = RRA + SIMM;


	CHECK_FOR_FPU;
	if (MEM_WSINGLE (addr, (float) FRS))
		RRA = addr;
}


// store floating-point single with update indexed
OPCODE (STFSUX)
{
	__u32 addr = RRA + RRB;


	CHECK_FOR_FPU;
	if (MEM_WSINGLE (addr, (float) FRS))
		RRA = addr;
}


// store floating-point single indexed
OPCODE (STFSX)
{
	unsigned int ra = RA;


	CHECK_FOR_FPU;
	MEM_WSINGLE ((ra) ? (GPR[ra] + RRB) : RRB, (float) FRS);
}


// move from FPSCR
OPCODE (MFFS)
{
	FBD = FPSCR;
}


// move to FPSCR fields
OPCODE (MTFSF)
{
	__u32 fm = FM;
	unsigned int mask = 0, i;


	for (i = 0; i < 8; i++, fm >>= 1)
	{
		mask >>= 4;

		if (fm & 1)
			mask |= 0xf0000000;
	}

	FPSCR = (FBB & mask) | (FPSCR &~ mask);
	SET_ROUNDING_MODE;
}


// move to FPSCR field immediate
OPCODE (MTFSFI)
{
	SET_FIELD (FPSCR, CRFD, IMM);
	SET_ROUNDING_MODE;
}


// move to FPSCR bit 0
OPCODE (MTFSB0)
{
	CLEAR_BIT (FPSCR, CRBD);
	SET_ROUNDING_MODE;
}


// move to FPSCR bit 1
OPCODE (MTFSB1)
{
	SET_BIT (FPSCR, CRBD);
	SET_ROUNDING_MODE;
}


// paired singles


// paired singles absolute value
OPCODE (PS_ABS)
{
	PSB0D = PSB0B &~ (1ULL << 63);
	PSB1D = PSB1B &~ (1ULL << 63);
}


// paired singles add
OPCODE (PS_ADD)
{
	PS0D = PS0A + PS0B;
	PS1D = PS1A + PS1B;
}


// paired singles compare ordered high
OPCODE (PS_CMPO0)
{
	CALC_CRFS (CRFD, PS0A, PSB0A, PS0B, PSB0B);
}


// paired singles compare ordered low
OPCODE (PS_CMPO1)
{
	CALC_CRFS (CRFD, PS1A, PSB1A, PS1B, PSB1B);
}


// paired singles compare unordered high
OPCODE (PS_CMPU0)
{
	CALC_CRFS (CRFD, PS0A, PSB0A, PS0B, PSB0B);
}


// paired singles compare unordered low
OPCODE (PS_CMPU1)
{
	CALC_CRFS (CRFD, PS1A, PSB1A, PS1B, PSB1B);
}


// paired singles divide
OPCODE (PS_DIV)
{
	PS0D = PS0A / PS0B;
	PS1D = PS1A / PS1B;
}


// paired singles multiply-add
OPCODE (PS_MADD)
{
	PS0D = PS0A * PS0C + PS0B;
	PS1D = PS1A * PS1C + PS1B;
}


// paired singles multiply-add scalar high
OPCODE (PS_MADDS0)
{
	PS1D = PS1A * PS0C + PS1B;
	PS0D = PS0A * PS0C + PS0B;
}


// paired singles multiply-add scalar low
OPCODE (PS_MADDS1)
{
	PS0D = PS0A * PS1C + PS0B;
	PS1D = PS1A * PS1C + PS1B;
}


// paired singles merge high
OPCODE (PS_MERGE00)
{
	PS1D = PS0B;
	PS0D = PS0A;
}


// paired singles merge direct
OPCODE (PS_MERGE01)
{
	PS0D = PS0A;
	PS1D = PS1B;
}


// paired singles merge swapped
OPCODE (PS_MERGE10)
{
	double ps0b = PS0B;


	PS0D = PS1A;
	PS1D = ps0b;
}


// paired singles merge low
OPCODE (PS_MERGE11)
{
	PS0D = PS1A;
	PS1D = PS1B;
}


// paired singles move register
OPCODE (PS_MR)
{
	PS0D = PS0B;
	PS1D = PS1B;
}


// paired singles multiply-subtract
OPCODE (PS_MSUB)
{
	PS0D = PS0A * PS0C - PS0B;
	PS1D = PS1A * PS1C - PS1B;
}


// paired singles multiply
OPCODE (PS_MUL)
{
	PS0D = PS0A * PS0C;
	PS1D = PS1A * PS1C;
}


// paired singles multiply scalar high
OPCODE (PS_MULS0)
{
	PS1D = PS1A * PS0C;
	PS0D = PS0A * PS0C;
}


// paired singles multiply scalar low
OPCODE (PS_MULS1)
{
	PS0D = PS0A * PS1C;
	PS1D = PS1A * PS1C;
}


// paired singles negate absolute value
OPCODE (PS_NABS)
{
	PSB0D = PSB0B | (1ULL << 63);
	PSB1D = PSB1B | (1ULL << 63);
}


// paired singles negate
OPCODE (PS_NEG)
{
	PSB0D = PSB0B ^ (1ULL << 63);
	PSB1D = PSB1B ^ (1ULL << 63);
}


// paired singles negative multiply-add
OPCODE (PS_NMADD)
{
	PS0D = -(PS0A * PS0C + PS0B);
	PS1D = -(PS1A * PS1C + PS1B);
}


// paired singles negative multiply-subtract
OPCODE (PS_NMSUB)
{
	PS0D = -(PS0A * PS0C - PS0B);
	PS1D = -(PS1A * PS1C - PS1B);
}


// paired singles reciprocal estimate
OPCODE (PS_RES)
{
	PS0D = 1.0 / PS0B;
	PS1D = 1.0 / PS1B;
}


// paired singles reciprocal square root estimate
OPCODE (PS_RSQRTE)
{
	PS0D = 1.0 / SQRT (PS0B);
	PS1D = 1.0 / SQRT (PS1B);
}


// paired singles select
OPCODE (PS_SEL)
{
	PS0D = (PS0A >= 0.0) ? PS0C : PS0B;
	PS1D = (PS1A >= 0.0) ? PS1C : PS1B;
}


// paired singles subtract
OPCODE (PS_SUB)
{
	PS0D = PS0A - PS0B;
	PS1D = PS1A - PS1B;
}


// paired singles sum high
OPCODE (PS_SUM0)
{
	PS0D = PS0A + PS1B;
	PS1D = PS1C;
}


// paired singles sum low
OPCODE (PS_SUM1)
{
	PS1D = PS0A + PS1B;
	PS0D = PS0C;
}


// dequantization factor
static const float dq_factor[] =
{
	1.0/(1 <<  0),
	1.0/(1 <<  1),
	1.0/(1 <<  2),
	1.0/(1 <<  3),
	1.0/(1 <<  4),
	1.0/(1 <<  5),
	1.0/(1 <<  6),
	1.0/(1 <<  7),
	1.0/(1 <<  8),
	1.0/(1 <<  9),
	1.0/(1 << 10),
	1.0/(1 << 11),
	1.0/(1 << 12),
	1.0/(1 << 13),
	1.0/(1 << 14),
	1.0/(1 << 15),
	1.0/(1 << 16),
	1.0/(1 << 17),
	1.0/(1 << 18),
	1.0/(1 << 19),
	1.0/(1 << 20),
	1.0/(1 << 21),
	1.0/(1 << 22),
	1.0/(1 << 23),
	1.0/(1 << 24),
	1.0/(1 << 25),
	1.0/(1 << 26),
	1.0/(1 << 27),
	1.0/(1 << 28),
	1.0/(1 << 29),
	1.0/(1 << 30),
	1.0/(1 << 31),

	(1ULL << 32),
	(1 << 31),
	(1 << 30),
	(1 << 29),
	(1 << 28),
	(1 << 27),
	(1 << 26),
	(1 << 25),
	(1 << 24),
	(1 << 23),
	(1 << 22),
	(1 << 21),
	(1 << 20),
	(1 << 19),
	(1 << 18),
	(1 << 17),
	(1 << 16),
	(1 << 15),
	(1 << 14),
	(1 << 13),
	(1 << 12),
	(1 << 11),
	(1 << 10),
	(1 <<  9),
	(1 <<  8),
	(1 <<  7),
	(1 <<  6),
	(1 <<  5),
	(1 <<  4),
	(1 <<  3),
	(1 <<  2),
	(1 <<  1),
};

#define QLOAD(ea,offset,type,scale,dest)		\
	({\
		int ret = 0;\
		switch (type)\
		{\
			case 0:\
				ret = MEM_RSINGLE (dest, ea + (offset << 2));\
				break;\
			case 4:\
				{\
					__u8 data;\
					ret = MEM_RBYTE (&data, ea + offset);\
					*(dest) = dq_factor[scale] * data;\
				}\
				break;\
			case 5:\
				{\
					__u16 data;\
					ret = MEM_RHALF (&data, ea + (offset << 1));\
					*(dest) = dq_factor[scale] * data;\
				}\
				break;\
			case 6:\
				{\
					__s8 data;\
					ret = MEM_RBYTE (&data, ea + offset);\
					*(dest) = dq_factor[scale] * data;\
				}\
				break;\
			case 7:\
				{\
					__s16 data;\
					ret = MEM_RHALF (&data, ea + (offset << 1));\
					*(dest) = dq_factor[scale] * data;\
				}\
				break;\
			default:\
				DEBUG (EVENT_EMAJOR, ".cpu: unhandled qload: (%d,%d)", type, scale);\
		}\
	ret;\
	})


// paired singles quantized load
OPCODE (PSQ_L)
{
	unsigned int ra;
	int type, scale;
	__u32 ea;


	CHECK_FOR_FPU;

	type = LD_TYPE (PSI);
	scale = LD_SCALE (PSI);
	ra = RA;
	ea = (ra) ? (GPR[ra] + PSIMM) : PSIMM;

	if (PSW)
	{
		QLOAD (ea, 0, type, scale, &PS0D);
		PS1D = 1.0f;
	}
	else
	{
		if (QLOAD (ea, 0, type, scale, &PS0D))
			QLOAD (ea, 1, type, scale, &PS1D);
	}
}


// paired singles quantized load with update
OPCODE (PSQ_LU)
{
	int type, scale;
	__u32 ea;


	CHECK_FOR_FPU;

	type = LD_TYPE (PSI);
	scale = LD_SCALE (PSI);
	ea = RRA + PSIMM;

	if (PSW)
	{
		if (QLOAD (ea, 0, type, scale, &PS0D))
		{
			PS1D = 1.0f;
			RRA = ea;
		}
	}
	else
	{
		if (QLOAD (ea, 0, type, scale, &PS0D))
		{
			QLOAD (ea, 1, type, scale, &PS1D);
			RRA = ea;
		}
	}
}


// paired singles quantized load with update indexed
OPCODE (PSQ_LUX)
{
	int type = LD_TYPE (PSXI);
	int scale = LD_SCALE (PSXI);
	__u32 ea = RRA + RRB;

		
	if (PSXW)
	{
		if (QLOAD (ea, 0, type, scale, &PS0D))
		{
			PS1D = 1.0f;
			RRA = ea;
		}
	}
	else
	{
		if (QLOAD (ea, 0, type, scale, &PS0D))
		{
			QLOAD (ea, 1, type, scale, &PS1D);
			RRA = ea;
		}
	}
}


// paired singles quantized load indexed
OPCODE (PSQ_LX)
{
	unsigned int ra = RA;
	int type = LD_TYPE (PSXI);
	int scale = LD_SCALE (PSXI);
	__u32 ea;


	ea = (ra) ? (GPR[ra] + RRB) : RRB;
		
	if (PSXW)
	{
		QLOAD (ea, 0, type, scale, &PS0D);
		PS1D = 1.0f;
	}
	else
	{
		if (QLOAD (ea, 0, type, scale, &PS0D))
			QLOAD (ea, 1, type, scale, &PS1D);
	}
}


// quantization factor
static const float q_factor[] =
{
	(1 <<  0),
	(1 <<  1),
	(1 <<  2),
	(1 <<  3),
	(1 <<  4),
	(1 <<  5),
	(1 <<  6),
	(1 <<  7),
	(1 <<  8),
	(1 <<  9),

	(1 << 10),
	(1 << 11),
	(1 << 12),
	(1 << 13),
	(1 << 14),
	(1 << 15),
	(1 << 16),
	(1 << 17),
	(1 << 18),
	(1 << 19),

	(1 << 20),
	(1 << 21),
	(1 << 22),
	(1 << 23),
	(1 << 24),
	(1 << 25),
	(1 << 26),
	(1 << 27),
	(1 << 28),
	(1 << 29),
	(1 << 30),
	(1 << 31),

	1.0/(1ULL << 32),
	1.0/(1 << 31),
	1.0/(1 << 30),

	1.0/(1 << 29),
	1.0/(1 << 28),
	1.0/(1 << 27),
	1.0/(1 << 26),
	1.0/(1 << 25),
	1.0/(1 << 24),
	1.0/(1 << 23),
	1.0/(1 << 22),
	1.0/(1 << 21),
	1.0/(1 << 20),

	1.0/(1 << 19),
	1.0/(1 << 18),
	1.0/(1 << 17),
	1.0/(1 << 16),
	1.0/(1 << 15),
	1.0/(1 << 14),
	1.0/(1 << 13),
	1.0/(1 << 12),
	1.0/(1 << 11),
	1.0/(1 << 10),

	1.0/(1 <<  9),
	1.0/(1 <<  8),
	1.0/(1 <<  7),
	1.0/(1 <<  6),
	1.0/(1 <<  5),
	1.0/(1 <<  4),
	1.0/(1 <<  3),
	1.0/(1 <<  2),
	1.0/(1 <<  1),
};


#define QSTORE(ea,offset,type,s,dest)		\
	({\
		int ret = 0;\
		switch (type)\
		{\
			case 0:\
				ret = MEM_WSINGLE (ea + (offset << 2), (float) dest);\
				break;\
			case 4:\
				ret = MEM_WBYTE (ea + offset, (__u8) CLAMP (q_factor[s] * dest, 0, 0xff));\
				break;\
			case 5:\
				ret = MEM_WHALF (ea + (offset << 1), (__u16) CLAMP (q_factor[s] * dest, 0, 0xffff));\
				break;\
			case 6:\
				ret = MEM_WBYTE (ea + offset, (__s8) CLAMP (q_factor[s] * dest, MIN_S8, MAX_S8));\
				break;\
			case 7:\
				ret = MEM_WHALF (ea + (offset << 1), (__s16) CLAMP (q_factor[s] * dest, MIN_S16, MAX_S16));\
				break;\
			default:\
				DEBUG (EVENT_EMAJOR, ".cpu: unhandled qstore: (%d,%d)", type, scale);\
		}\
		ret;\
	})


// paired singles quantized store
OPCODE (PSQ_ST)
{
	unsigned int ra;
	int type, scale;
	__u32 ea;


	CHECK_FOR_FPU;

	type = LD_TYPE (PSI);
	scale = LD_SCALE (PSI);
	ra = RA;
	ea = (ra) ? (GPR[ra] + PSIMM) : PSIMM;

	if (PSW)
		QSTORE (ea, 0, type, scale, PS0D);
	else
	{
		if (QSTORE (ea, 0, type, scale, PS0D))
			QSTORE (ea, 1, type, scale, PS1D);
	}
}


// paired singles quantized store with update
OPCODE (PSQ_STU)
{
	int type, scale;
	__u32 ea;


	CHECK_FOR_FPU;

	type = LD_TYPE (PSI);
	scale = LD_SCALE (PSI);
	ea = RRA + PSIMM;

	if (PSW)
	{
		if (QSTORE (ea, 0, type, scale, PS0D))
			RRA = ea;
	}
	else
	{
		if (QSTORE (ea, 0, type, scale, PS0D))
		{
			QSTORE (ea, 1, type, scale, PS1D);
			RRA = ea;
		}
	}
}


// paired singles quantized store with update indexed
OPCODE (PSQ_STUX)
{
	int type = LD_TYPE (PSXI);
	int scale = LD_SCALE (PSXI);
	__u32 ea = RRA + RRB;


	if (PSXW)
	{
		if (QSTORE (ea, 0, type, scale, PS0D))
			RRA = ea;
	}
	else
	{
		if (QSTORE (ea, 0, type, scale, PS0D))
		{
			QSTORE (ea, 1, type, scale, PS1D);
			RRA = ea;
		}
	}
}


// paired singles quantized store indexed
OPCODE (PSQ_STX)
{
	unsigned int ra = RA;
	int type = LD_TYPE (PSXI);
	int scale = LD_SCALE (PSXI);
	__u32 ea;


	ea = (ra) ? (GPR[ra] + RRB) : RRB;
		
	if (PSXW)
		QSTORE (ea, 0, type, scale, PS0D);
	else
	{
		if (QSTORE (ea, 0, type, scale, PS0D))
			QSTORE (ea, 1, type, scale, PS1D);
	}
}


/////////////////////////////////////////////////////////////


// extension of "19"
OPCODE (OPC19)
{
	op19[opcode & 0x7ff] (opcode);
}


// extension of "31"
OPCODE (OPC31)
{
	op31[opcode & 0x7ff] (opcode);
}


// extension of "59"
OPCODE (OPC59)
{
	CHECK_FOR_FPU;
	op59[opcode & 0x3f] (opcode);
}


// extension of "63"
OPCODE (OPC63)
{
	CHECK_FOR_FPU;
	op63[opcode & 0x7ff] (opcode);
}


// extension of "4"
OPCODE (OPC4)
{
	CHECK_FOR_FPU;
	op4[opcode & 0x7ff] (opcode);
}


void cpu_init (void)
{
	int i, j;


	memset (cpuregs, 0, sizeof (cpuregs));
	memset (ps0, 0, sizeof (ps0));
	memset (ps1, 0, sizeof (ps1));

	TB = CALC_TB;
	RSP = 0x816ffff0;
	LR = EXCEPTION_SYSTEM_RESET;
	MSR = MSR_FP;

	// Dolphin OS setup (standard setup after Config24MB)
	IBATU (0) = DBATU (0) = 0x800001ff;
	IBATL (0) = DBATL (0) = 0x00000002;
	IBATU (2) = DBATU (2) = 0x810000ff;
	IBATL (2) = DBATL (2) = 0x01000002;

	IBATU (1) = DBATU (1) = 0;
	IBATL (1) = DBATL (1) = 0;
	IBATU (3) = DBATU (3) = 0;
	IBATL (3) = DBATL (3) = 0;

	// adds
	IBATU (1) = DBATU (1) = 0xc0001fff;
	IBATL (1) = DBATL (1) = 0x00000002;
	IBATU (3) = DBATU (3) = 0x000000ff;
	IBATL (3) = DBATL (3) = 0x00000002;

	PVR = (PVR_GEKKO << 16) | PVR_REVISION;

	for (i = 0; i < 16; i++)
		SR[i] = 0x2aa * i;

	// precalc masks
	for (i = 0; i < 32; i++)
		for (j = 0; j < 32; j++)
			mask[i][j] = GENMASK (i, j);

	for (i = 0; i < 64; i++)
		op0[i] = op59[i] = NI;

	for (i = 0; i < 2048; i++)
		op4[i] = op19[i] = op31[i] = op63[i] = NI;

	OP1 ( 1) = hle_execute;
	OP1 ( 2) = hle_reattach;

	OP1 ( 0) =  INVALID;
	OP1 ( 3) =  EMPTY;		// TWI - trap word immediate
	OP1 ( 4) = OPC4;
	OP1 ( 7) = MULLI;
	OP1 ( 8) = SUBFIC;
	OP1 (10) = CMPLI;
	OP1 (11) = CMPI;
	OP1 (12) = ADDIC;
	OP1 (13) = ADDICD;
	OP1 (14) = ADDI;
	OP1 (15) = ADDIS;
	OP1 (16) = BCX;
//	OP1 (17) = SC;
	OP1 (17) = EMPTY;
	OP1 (18) = BX;
	OP1 (19) =   OPC19;
	OP1 (20) = RLWIMIX;
	OP1 (21) = RLWINMX;
	OP1 (23) = RLWNMX;
	OP1 (24) = ORI;
	OP1 (25) = ORIS;
	OP1 (26) = XORI;
	OP1 (27) = XORIS;
	OP1 (28) = ANDID;
	OP1 (29) = ANDISD;
	OP1 (31) =   OPC31;
	OP1 (32) = LWZ;
	OP1 (33) = LWZU;
	OP1 (34) = LBZ;
	OP1 (35) = LBZU;
	OP1 (36) = STW;
	OP1 (37) = STWU;
	OP1 (38) = STB;
	OP1 (39) = STBU;
	OP1 (40) = LHZ;
	OP1 (41) = LHZU;
	OP1 (42) = LHA;
	OP1 (43) = LHAU;
	OP1 (44) = STH;
	OP1 (45) = STHU;
	OP1 (46) = LMW;
	OP1 (47) = STMW;
	OP1 (48) = LFS;
	OP1 (49) = LFSU;
	OP1 (50) = LFD;
	OP1 (51) = LFDU;
	OP1 (52) = STFS;
	OP1 (53) = STFSU;
	OP1 (54) = STFD;
	OP1 (55) = STFDU;
	OP1 (59) =   OPC59;
	OP1 (63) =   OPC63;

	OP3 (19,   0, 0) = MCRF;
	OP3 (19,  16, 0) = BCLR;
	OP3 (19,  16, 1) = BCLRL;
	OP3 (19,  33, 0) = CRNOR;
	OP3 (19,  50, 0) = RFI;
	OP3 (19, 129, 0) = CRANDC;
	OP3 (19, 150, 0) =  EMPTY;		// ISYNC - instruction synchronize
	OP3 (19, 193, 0) = CRXOR;
	OP3 (19, 225, 0) = CRNAND;
	OP3 (19, 257, 0) = CRAND;
	OP3 (19, 289, 0) = CREQV;
	OP3 (19, 417, 0) = CRORC;
	OP3 (19, 449, 0) = CROR;
	OP3 (19, 528, 0) = BCCTR;
	OP3 (19, 528, 1) = BCCTRL;
	
	OP3 (31,       0, 0) = CMP;
	OP3 (31,       4, 0) =  EMPTY;			// TW - trap word
	OP4 (31,   0,  8, 0) = SUBFC;
	OP4 (31,   0,  8, 1) = SUBFCD;
	OP4 (31,   1,  8, 0) = SUBFCO;
	OP4 (31,   1,  8, 1) = SUBFCOD;
	OP4 (31,   0, 10, 0) = ADDC;
	OP4 (31,   0, 10, 1) = ADDCD;
	OP4 (31,   1, 10, 0) = ADDCO;
	OP4 (31,   1, 10, 1) = ADDCOD;
	OP3 (31,      11, 0) = MULHWU;
	OP3 (31,      11, 1) = MULHWUD;
	OP3 (31,      19, 0) = MFCR;
	OP3 (31,      20, 0) = LWARX;
	OP3 (31,      23, 0) = LWZX;
	OP3 (31,      24, 0) = SLW;
	OP3 (31,      24, 1) = SLWD;
	OP3 (31,      26, 0) = CNTLZW;
	OP3 (31,      26, 1) = CNTLZWD;
	OP3 (31,      28, 0) = AND;
	OP3 (31,      28, 1) = ANDD;
	OP3 (31,      32, 0) = CMPL;
	OP4 (31,   0, 40, 0) = SUBF;
	OP4 (31,   0, 40, 1) = SUBFD;
	OP4 (31,   1, 40, 0) = SUBFO;
	OP4 (31,   1, 40, 1) = SUBFOD;
	OP3 (31,      54, 0) =  EMPTY;		// DCBST - data cache block store
	OP3 (31,      55, 0) = LWZUX;
	OP3 (31,      60, 0) = ANDC;
	OP3 (31,      60, 1) = ANDCD;
	OP3 (31,      75, 0) = MULHW;
	OP3 (31,      75, 1) = MULHWD;
	OP3 (31,      83, 0) = MFMSR;
	OP3 (31,      86, 0) =  EMPTY;		// DCBF - data cache block flush
	OP3 (31,      87, 0) = LBZX;
	OP4 (31,  0, 104, 0) = NEG;
	OP4 (31,  0, 104, 1) = NEGD;
	OP4 (31,  1, 104, 0) = NEGO;
	OP4 (31,  1, 104, 1) = NEGOD;
	OP3 (31,     124, 0) = NOR;
	OP3 (31,     124, 1) = NORD;
	OP3 (31,     119, 0) = LBZUX;
	OP4 (31,  0, 136, 0) = SUBFE;
	OP4 (31,  0, 136, 1) = SUBFED;
	OP4 (31,  1, 136, 0) = SUBFEO;
	OP4 (31,  1, 136, 1) = SUBFEOD;
	OP4 (31,  0, 138, 0) = ADDE;
	OP4 (31,  0, 138, 1) = ADDED;
	OP4 (31,  1, 138, 0) = ADDEO;
	OP4 (31,  1, 138, 1) = ADDEOD;
	OP3 (31,     144, 0) = MTCRF;
	OP3 (31,     146, 0) = MTMSR;
	OP3 (31,     150, 1) = STWCXD;
	OP3 (31,     151, 0) = STWX;
	OP3 (31,     183, 0) = STWUX;
	OP4 (31,  0, 200, 0) = SUBFZE;
	OP4 (31,  0, 200, 1) = SUBFZED;
	OP4 (31,  1, 200, 0) = SUBFZEO;
	OP4 (31,  1, 200, 1) = SUBFZEOD;
	OP4 (31,  0, 202, 0) = ADDZE;
	OP4 (31,  0, 202, 1) = ADDZED;
	OP4 (31,  1, 202, 0) = ADDZEO;
	OP4 (31,  1, 202, 1) = ADDZEOD;
	OP3 (31,     210, 0) = MTSR;
	OP3 (31,     215, 0) = STBX;
	OP4 (31,  0, 232, 0) = SUBFME;
	OP4 (31,  0, 232, 1) = SUBFMED;
	OP4 (31,  1, 232, 0) = SUBFMEO;
	OP4 (31,  1, 232, 1) = SUBFMEOD;
	OP4 (31,  0, 234, 0) = ADDME;
	OP4 (31,  0, 234, 1) = ADDMED;	
	OP4 (31,  1, 234, 0) = ADDMEO;
	OP4 (31,  1, 234, 1) = ADDMEOD;	
	OP4 (31,  0, 235, 0) = MULLW;
	OP4 (31,  0, 235, 1) = MULLWD;
	OP4 (31,  1, 235, 0) = MULLWO;
	OP4 (31,  1, 235, 1) = MULLWOD;
	OP3 (31,     242, 0) = MTSRIN;

	OP3 (31,     246, 0) =  EMPTY;		// DCBTST - data cache block touch for store
	OP3 (31,     247, 0) = STBUX;

	OP4 (31,  0, 266, 0) = ADD;
	OP4 (31,  0, 266, 1) = ADDD;
	OP4 (31,  1, 266, 0) = ADDO;
	OP4 (31,  1, 266, 1) = ADDOD;
	OP3 (31,     278, 0) =  EMPTY;		// DCBT - data cache block touch
	OP3 (31,     279, 0) = LHZX;
	OP3 (31,     284, 0) = EQV;
	OP3 (31,     284, 1) = EQVD;
	OP3 (31,     306, 0) =  EMPTY;		// TLBIE - translation lookaside buffer invalidate entry
	OP3 (31,     311, 0) = LHZUX;
	OP3 (31,     316, 0) = XOR;
	OP3 (31,     316, 1) = XORD;
	OP3 (31,     339, 0) = MFSPR;
	OP3 (31,     343, 0) = LHAX;
	OP3 (31,     370, 0) =  EMPTY;		// TLBIA - translation lookaside buffer invalidate all
	OP3 (31,     371, 0) = MFTB;
	OP3 (31,     375, 0) = LHAUX;
	OP3 (31,     407, 0) = STHX;
	OP3 (31,     412, 0) = ORC;
	OP3 (31,     412, 1) = ORCD;
	OP3 (31,     439, 0) = STHUX;
	OP3 (31,     444, 0) = OR;
	OP3 (31,     444, 1) = ORD;
	OP3 (31,     467, 0) = MTSPR;
	OP3 (31,     476, 0) = NAND;
	OP3 (31,     476, 1) = NANDD;
	OP4 (31,  0, 491, 0) = DIVW;
	OP4 (31,  0, 491, 1) = DIVWD;
	OP4 (31,  1, 491, 0) = DIVWO;
	OP4 (31,  1, 491, 1) = DIVWOD;
	OP4 (31,  0, 459, 0) = DIVWU;
	OP4 (31,  0, 459, 1) = DIVWUD;
	OP4 (31,  1, 459, 0) = DIVWUO;
	OP4 (31,  1, 459, 1) = DIVWUOD;
	OP3 (31,     470, 0) =  EMPTY;		// DCBI - data cache block invalidate
	OP3 (31,     566, 0) =  EMPTY;		// TLBSYNC - TLB synchronize
	OP3 (31,     512, 0) = MCRXR;
	OP3 (31,     533, 0) = LSWX;
	OP3 (31,     534, 0) = LWBRX;
	OP3 (31,     535, 0) = LFSX;
	OP3 (31,     536, 0) = SRW;
	OP3 (31,     536, 1) = SRWD;
	OP3 (31,     567, 0) = LFSUX;
	OP3 (31,     595, 0) = MFSR;
	OP3 (31,     597, 0) = LSWI;
	OP3 (31,     598, 0) =  EMPTY;		// SYNC - synchronize
	OP3 (31,     599, 0) = LFDX;
	OP3 (31,     631, 0) = LFDUX;
	OP3 (31,     659, 0) = MFSRIN;
	OP3 (31,     661, 0) = STSWX;
	OP3 (31,     662, 0) = STWBRX;
	OP3 (31,     663, 0) = STFSX;
	OP3 (31,     695, 0) = STFSUX;
	OP3 (31,     725, 0) = STSWI;
	OP3 (31,     727, 0) = STFDX;
	OP3 (31,     758, 0) =  EMPTY;		// DCBA - data cache block allocate
	OP3 (31,     759, 0) = STFDUX;
	OP3 (31,     790, 0) = LHBRX;
	OP3 (31,     792, 0) = SRAW;
	OP3 (31,     792, 1) = SRAWD;
	OP3 (31,     824, 0) = SRAWI;
	OP3 (31,     824, 1) = SRAWID;
	OP3 (31,     854, 0) =  EMPTY;		// EIEIO - enforce in-order execution of i/o
	OP3 (31,     918, 0) = STHBRX;
	OP3 (31,     922, 0) = EXTSH;
	OP3 (31,     922, 1) = EXTSHD;
	OP3 (31,     954, 0) = EXTSB;
	OP3 (31,     954, 1) = EXTSBD;
	OP3 (31,     982, 0) =  EMPTY;		// ICBI - instruction cache block invalidate
	OP3 (31,     983, 0) = STFIWX;
	OP3 (31,    1014, 0) = DCBZ;

	OP3 (59,  18, 0) = FDIVS;
	OP3 (59,  20, 0) = FSUBS;
	OP3 (59,  21, 0) = FADDS;
	OP3 (59,  22, 0) = FSQRTS;
	OP3 (59,  24, 0) = FRES;
	OP3 (59,  25, 0) = FMULS;
	OP3 (59,  28, 0) = FMSUBS;
	OP3 (59,  29, 0) = FMADDS;
	OP3 (59,  30, 0) = FNMSUBS;
	OP3 (59,  31, 0) = FNMADDS;

	OP3 (63,   0, 0) = FCMPU;
	OP3 (63,  12, 0) = FRSP;
	OP3 (63,  14, 0) = FCTIW;
	OP3 (63,  15, 0) = FCTIWZ;
	OPR (63,  18, 0, 6, 11, FDIV);
	OP3 (63,  20, 0) = FSUB;
	OPR (63,  21, 0, 6, 11, FADD);
	OP3 (63,  22, 0) = FSQRT;
	OPR (63,  23, 0, 6, 11, FSEL);
	OPR (63,  25, 0, 6, 11, FMUL);
	OP3 (63,  26, 0) = FRSRTE;
	OPR (63,  28, 0, 6, 11, FMSUB);
	OPR (63,  29, 0, 6, 11, FMADD);
	OPR (63,  30, 0, 6, 11, FNMSUB);
	OPR (63,  31, 0, 6, 11, FNMADD);
	OP3 (63,  32, 0) = FCMPO;
	OP3 (63,  38, 0) = MTFSB1;

	OP3 (63,  40, 0) = FNEG;
	OP3 (63,  64, 0) = MCRFS;
	OP3 (63,  70, 0) = MTFSB0;
	OP3 (63,  72, 0) = FMR;
	OP3 (63, 134, 0) = MTFSFI;
	OP3 (63, 136, 0) = FNABS;
	OP3 (63, 264, 0) = FABS;
	OP3 (63, 583, 0) = MFFS;
	OP3 (63, 711, 0) = MTFSF;

	// additional gekko instructions
	OP1 (56) = PSQ_L;
	OP1 (57) = PSQ_LU;
	OP1 (60) = PSQ_ST;
	OP1 (61) = PSQ_STU;

	OP3 ( 4,   0, 0) = PS_CMPU0;
	OPR ( 4,   6, 0, 7, 11, PSQ_LX);
	OPR ( 4,   7, 0, 7, 11, PSQ_STX);
	OPR ( 4,  10, 0, 6, 11, PS_SUM0);
	OPR ( 4,  11, 0, 6, 11, PS_SUM1);
	OPR ( 4,  12, 0, 6, 11, PS_MULS0);
	OPR ( 4,  13, 0, 6, 11, PS_MULS1);
	OPR ( 4,  14, 0, 6, 11, PS_MADDS0);
	OPR ( 4,  15, 0, 6, 11, PS_MADDS1);
	OP3 ( 4,  18, 0) = PS_DIV;
	OP3 ( 4,  20, 0) = PS_SUB;
	OP3 ( 4,  21, 0) = PS_ADD;
	OPR ( 4,  23, 0, 6, 11, PS_SEL);
	OP3 ( 4,  24, 0) = PS_RES;
	OPR ( 4,  25, 0, 6, 11, PS_MUL);
	OP3 ( 4,  26, 0) = PS_RSQRTE;
	OPR ( 4,  28, 0, 6, 11, PS_MSUB);
	OPR ( 4,  29, 0, 6, 11, PS_MADD);
	OPR ( 4,  30, 0, 6, 11, PS_NMSUB);
	OPR ( 4,  31, 0, 6, 11, PS_NMADD);
	OP3 ( 4,  32, 0) = PS_CMPO0;
	OPR ( 4,  38, 0, 7, 11, PSQ_LUX);
	OPR ( 4,  39, 0, 7, 11, PSQ_STUX);
	OP3 ( 4,  40, 0) = PS_NEG;
	OP3 ( 4,  64, 0) = PS_CMPU1;
	OP3 ( 4,  72, 0) = PS_MR;
	OP3 ( 4,  96, 0) = PS_CMPO1;
	OP3 ( 4, 136, 0) = PS_NABS;
	OP3 ( 4, 264, 0) = PS_ABS;
	OP3 ( 4, 528, 0) = PS_MERGE00;
	OP3 ( 4, 560, 0) = PS_MERGE01;
	OP3 ( 4, 592, 0) = PS_MERGE10;
	OP3 ( 4, 624, 0) = PS_MERGE11;
	OP3 ( 4,1014, 0) = EMPTY;						// data cache block set to zero locked
}

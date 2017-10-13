#ifndef __TOOLS_X86_H
#define __TOOLS_X86_H 1


// GNU AS inline routines

static inline __attribute__ ((const)) __u16 BSWAP_16 (__u16 X)
{
	asm
	(
		"xchgb %b0, %h0"
		: "=q" (X)
		: "0" (X)
	);
	
	return X;
}


static inline __attribute__ ((const)) __u32 BSWAP_32 (__u32 X)
{
	asm
	(
		"bswap %0"
		: "=r" (X)
		: "0" (X)
	);
	
	return X;
}


static inline __attribute__ ((const)) __u64 BSWAP_64 (__u64 X)
{
	return (((__u64)(BSWAP_32 (X)) << 32) | (BSWAP_32 (X >> 32)));
}


static inline __attribute__ ((const)) __u32 ROTL (__u32 X, int n)
{
	asm
	(
		"roll %b2, %0"
		: "=g" (X)
		: "0" (X), "cI" (n)
	);

	return X;
}


static inline __attribute__ ((const)) int CARRY (__u32 A, __u32 B)
{
	register __u8 carry;


	asm
	(
		"addl %2, %1"
		"setc %0"
		: "=&q" (carry), "+&r" (A)
		: "g" (B)
	);
	
	return carry;
}


static inline __attribute__ ((const)) int CARRY3 (__u32 A, __u32 B, __u32 C)
{
	register __u8 carry;


	asm
	(
		"addl %2, %1\n"
		"setc %0\n"
		"addl %3, %1\n"
		"adcb $0, %0"
		: "=&q" (carry), "+&r" (A)
		: "g" (B), "g" (C)
	);
	
	return carry;
}


static inline __attribute__ ((const)) int BITSCAN (__u32 X)
{
	asm
	(
		"bsr %0,%0"
		: "=r" (X)
		: "0" (X)
	);
	
	return 31 - X;
}


#endif // __TOOLS_X86_H

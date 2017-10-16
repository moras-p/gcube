#ifndef __MEM_H
#define __MEM_H 1


#include <stdio.h>
#include <string.h>

#include "general.h"
#include "gdebug.h"
#include "cpu.h"
#include "hw_ai_dsp.h"
#include "hw_cp.h"
#include "hw_di.h"
#include "hw_exi.h"
#include "hw_gx.h"
#include "hw_mi.h"
#include "hw_pe.h"
#include "hw_pi.h"
#include "hw_si.h"
#include "hw_vi.h"

// memory
#define MEM_SIZE 0x01800000
#define MEM_MASK 0x01ffffff

// some programs will set the fb too low and overwrite the code
// this temporary hack gets around that
//#define MEMHACK

#ifdef MEMHACK
extern __u8 RAM[2][MEM_SIZE];
# define MEM(X)					(RAM[((X) >> 30) & 1][(X) & MEM_MASK])
#else
extern __u8 RAM[MEM_SIZE];
# define MEM(X)					(RAM[(X) & MEM_MASK])
#endif

#define MEM_SAFE(A)			(mem_read8_safe (A))
#define MEM32_SAFE(A)		(BSWAP32 (mem_read32_safe (A)))
#define MEMRF_SAFE(A)		(BSWAPF (mem_read32_safe (A)))
#define MEM_ADDRESS(X)	((void *) &MEM (X))

#define MEM_L2C_ADDRESS(X)	(((X) >= L2C_BASE) ? L2C_ADDRESS (X) : MEM_ADDRESS (X))

// level 2 cache
#define L2C_SIZE					0x4000
#define L2C_MASK					0x3fff
extern __u8 L2C[L2C_SIZE];

#define L2C(X)						(L2C[(X) & L2C_MASK])
#define L2C_ADDRESS(X)		(&L2C (X))
#define L2C_BASE					0xe0000000

#define L2C16(A)			(*((__u16 *) &L2C (A)))
#define L2C32(A)			(*((__u32 *) &L2C (A)))
#define L2C64(A)			(*((__u64 *) &L2C (A)))

#define EFB_SIZE			0x0020fa00
extern __u8 EFB[EFB_SIZE];

#define EFB_WIDTH				640
#define EFB_HEIGHT			528
#define EFB(X)					(EFB[((((X) & 0xfff)>>2) + ((((X) & 0x3ff000) >> 12)*EFB_WIDTH))*4])
#define EFB16(X)				(*((__u16 *) &EFB (X)))
#define EFB32(X)				(*((__u32 *) &EFB (X)))

//#define MMU_ENABLED			

#define MMU_OK					TRUE
#define MMU_ERROR				FALSE

#ifdef MMU_ENABLED
# define EA_MASK						0x3fffffff
# define MEM_EFB_BASE				0x08000000
# define MEM_HW_BASE				0x0c000000
# define MEM_LC_BASE				0x20000000
#else
# define EA_MASK						0xffffffff
# define MEM_EFB_BASE				0xc8000000
# define MEM_HW_BASE				0xcc000000
# define MEM_LC_BASE				0xe0000000
#endif


typedef union
{
	__u64 bin;
	double fp;
} Double64;


typedef union
{
	__u32 bin;
	float fp;
} Single32;

static inline float U32TOSINGLE (__u32 a)
{
	Single32 d;
	d.bin = a;
	return d.fp;
}

static inline __u32 SINGLETOU32 (float a)
{
	Single32 d;
	d.fp = a;
	return d.bin;
}

static inline double U64TODOUBLE (__u64 a)
{
	Double64 d;
	d.bin = a;
	return d.fp;
}

static inline __u64 DOUBLETOU64 (double a)
{
	Double64 d;
	d.fp = a;
	return d.bin;
}

static inline __u64 U32TODOUBLE64 (__u32 a)
{
	Double64 d;
	d.fp = U32TOSINGLE (a);
	return d.bin;
}



#define MEMS(A)				((__s8) MEM (A))

#define MEM16(A)			(*((__u16 *) &MEM (A)))
#define MEM32(A)			(*((__u32 *) &MEM (A)))
#define MEM64(A)			(*((__u64 *) &MEM (A)))

#define MEMS16(A)			(*((__s16 *) &MEM (A)))
#define MEMS32(A)			(*((__s32 *) &MEM (A)))
#define MEMS64(A)			(*((__s64 *) &MEM (A)))

#define MEMF(A)				(*((float *) &MEM (A)))
#define MEMRF(A)			(U32TOSINGLE (MEMR32 (A)))

#define MEMR16(A)			(BSWAP16 (MEM16 (A)))
#define MEMR32(A)			(BSWAP32 (MEM32 (A)))
#define MEMR64(A) 		(BSWAP64 (MEM64 (A)))

#define MEMSR16(A)		((__s16) BSWAP16 (MEM16 (A)))
#define MEMSR32(A)		((__s32) BSWAP32 (MEM32 (A)))
#define MEMSR64(A)		((__s64) BSWAP64 (MEM64 (A)))

#define MEMW(A,d)			(MEM (A) = d)
#define MEMWR16(A,d)	(MEM16 (A) = BSWAP16 (d))
#define MEMWR32(A,d)	(MEM32 (A) = BSWAP32 (d))


// byte swap
#ifdef LIL_ENDIAN
# ifndef BSWAP16
#  define BSWAP16(B)  	(BSWAP_16 (B))
# endif

# ifndef BSWAPS16
#  define BSWAPS16(B)		((__s16) BSWAP16(B))
# endif

# ifndef BSWAP32
#  define BSWAP32(B)		(BSWAP_32 (B))
# endif

# ifndef BSWAP64
#  define BSWAP64(B)		(BSWAP_64 (B))
# endif

# ifndef BSWAPF
#  define BSWAPF(B)		U32TOSINGLE (BSWAP32 (SINGLETOU32 (B)))
# endif

# define BIG_BSWAP16(B)	(B)
# define BIG_BSWAP32(B)	(B)
# define BIG_BSWAP64(B)	(B)

#else

# define BSWAP16(B) 	(B)
# define BSWAPS16(B) 	(B)
# define BSWAP32(B)		(B)
# define BSWAP64(B)		(B)
# define BSWAPF(B)		(B)

# define BIG_BSWAP16(B)  	(BSWAP_16 (B))
# define BIG_BSWAP32(B)		(BSWAP_32 (B))
# define BIG_BSWAP64(B)		(BSWAP_64 (B))

#endif // LIL_ENDIAN

#define BOTH_BSWAP16(B)		(BSWAP_16 (B))
#define BOTH_BSWAP32(B)		(BSWAP_32 (B))


#define MEM_SET							mem_set
#define MEM_COPY_TO_PTR			mem_copy_to_ptr
#define MEM_COPY_FROM_PTR		mem_copy_from_ptr

// reads
#define MEM_RBYTE(D,A)				(read_byte ((__u8 *) D, (A)))
#define MEM_RHALF(D,A)				(read_half_word ((__u16 *) D, (A)))
#define MEM_RHALF_S(D,A)			(read_half_word ((__u16 *) D, (A)))
#define MEM_RHALF_SR(D,A	)		(read_half_word_r ((__u16 *) D, (A)))
#define MEM_RWORD(D,A)				(read_word ((__u32 *) D, (A)))
#define MEM_RWORD_R(D,A)			(read_word_r ((__u32 *) D, (A)))
#define MEM_RSINGLE(D,A)			(read_single ((__u64 *) D, (A)))
#define MEM_RDOUBLE(D,A)			(read_double ((__u64 *) D, (A)))

// writes
#define MEM_WBYTE(A,D)				(write_byte (A, (__u8) D))
#define MEM_WHALF(A,D)				(write_half_word (A, (__u16) D))
#define MEM_WHALF_R(A,D)			(write_half_word_r (A, (__u16) D))
#define MEM_WWORD(A,D)				(write_word (A, (__u32) D))
#define MEM_WWORD_R(A,D)			(write_word_r (A, (__u32) D))
#define MEM_WSINGLE(A,D)			(write_word (A, SINGLETOU32 (D)))
#define MEM_WDOUBLE(A,D)			(write_double (A, DOUBLETOU64 (D)))

// memory map
#define MEM_BOOT_MAGIC					0x80000020
#define MEM_VERSION							0x80000024
#define MEM_MEM_SIZE						0x80000028
#define MEM_CONSOLE_TYPE				0x8000002c
#define MEM_HEAP_BOTTOM					0x80000030
#define MEM_HEAP_TOP						0x80000034
#define MEM_FST									0x80000038
#define MEM_FST_SIZE						0x8000003c
#define MEM_DEBUG								0x80000040
#define MEM_SIM_MEM_SIZE				0x800000f0
#define MEM_BUS_CLOCK_SPEED			0x800000f8
#define MEM_CPU_CLOCK_SPEED			0x800000fc

#define CONSOLE_TYPE_RETAIL1				0x00000001
#define CONSOLE_TYPE_HW2_PB					0x00000002
#define CONSOLE_TYPE_LATEST_PB			0x00000003
#define CONSOLE_TYPE_LATEST_DEVKIT	0x10000006
#define CONSOLE_TYPE_LATEST_TDEV		0x20000006

#define MEM_BOOT_MAGIC_NORMAL		0x0D15EA5E
#define MEM_BOOT_MAGIC_JTAG			0xE5207C22
#define ARENA_LO								0x00000000
#define ARENA_HI								0x81700000

#define MEM_TV_MODE							0x800000cc
#define TV_MODE_NTSC						0
#define TV_MODE_PAL							1
#define TV_MODE_DEBUG						2


void mem_reset (void);

int read_byte (__u8 *data, __u32 addr);
int read_half_word_r (__u16 *data, __u32 addr);
int read_half_word (__u16 *data, __u32 addr);
int read_word_r (__u32 *data, __u32 addr);
int read_word (__u32 *data, __u32 addr);
int read_single (__u64 *data, __u32 addr);
int read_double (__u64 *data, __u32 addr);
int write_byte (__u32 addr, __u8 data);
int write_half_word_r (__u32 addr, __u16 data);
int write_half_word (__u32 addr, __u16 data);
int write_word_r (__u32 addr, __u32 data);
int write_word (__u32 addr, __u32 data);
int write_double (__u32 addr, __u64 data);

__u8 mem_read8_safe (__u32 addr);
__u32 mem_read32_safe (__u32 addr);

void mem_fake_w32 (__u32 addr, __u32 data);
__u32 mem_fake_r32 (__u32 addr);
void mem_fake_w16 (__u32 addr, __u16 data);
__u16 mem_fake_r16 (__u32 addr);

void mem_hwr_hookx (int bits, __u32 addr, void *ptr);
void mem_hww_hookx (int bits, __u32 addr, void *ptr);
#define mem_hwr_hook(b,a,p) (mem_hwr_hookx (b, a, (void *) p))
#define mem_hww_hook(b,a,p) (mem_hww_hookx (b, a, (void *) p))

int mem_set (__u32 addr, __u8 fill, __u32 size);
int mem_copy_to_ptr (__u8 *dst, __u32 address, __u32 size);
int mem_copy_from_ptr (__u32 address, __u8 *src, __u32 size);
int mem_dump (const char *filename, __u32 address, __u32 length);

int mem_fetch (__u32 pc, __u32 *op);
int mem_block_zero (__u32 addr);

#ifdef MMU_ENABLED
int mmu_effective_to_physical (__u32 *pa, __u32 ea, int write, int code);
int mmu_effective_to_physical_debug (__u32 *pa, __u32 ea, int code);
#else
# define mmu_effective_to_physical(pa,ea,w,c)	(*pa = ea & EA_MASK)
# define mmu_effective_to_physical_debug(pa,ea,c)	(*pa = ea & EA_MASK)
#endif

void mmu_print_setup (void);


#endif // __MEM_H

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


#define MEMS(A)				((__s8) MEM (A))

#define MEM16(A)			(*((__u16 *) &MEM (A)))
#define MEM32(A)			(*((__u32 *) &MEM (A)))
#define MEM64(A)			(*((__u64 *) &MEM (A)))

#define MEMS16(A)			(*((__s16 *) &MEM (A)))
#define MEMS32(A)			(*((__s32 *) &MEM (A)))
#define MEMS64(A)			(*((__s64 *) &MEM (A)))

#define MEMF(A)				(*((float *) &MEM (A)))
#define MEMRF(A)			(((Single32) MEMR32 (A)).fp)

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
#  define BSWAPF(B)\
	({\
			((Single32) BSWAP32 (((Single32) (B)).bin)).fp;\
	})
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
#define MEM_RBYTE				read_byte
#define MEM_RHALF				read_half_word
#define MEM_RHALF_S			(__s16) read_half_word
#define MEM_RHALF_SR		(__s16) read_half_word_r
#define MEM_RWORD				read_word
#define MEM_RWORD_R			read_word_r

// writes
#define MEM_WBYTE				write_byte
#define MEM_WHALF				write_half_word
#define MEM_WHALF_R			write_half_word_r
#define MEM_WWORD				write_word
#define MEM_WWORD_R			write_word_r

#define MEM_WSINGLE(A,d)			(write_word (A, ((Single32) (d)).bin))
#define MEM_RSINGLE_F(A)			(((Single32) read_word (A)).fp)
#define MEM_RSINGLE_B(A)			(((Single32) read_word (A)).bin)

#define MEM_WDOUBLE(A,d)			(write_double (A, ((Double64) (d)).bin))
#define MEM_RDOUBLE_F(A)			(((Double64) read_double (A)).fp)
#define MEM_RDOUBLE_B(A)			(((Double64) read_double (A)).bin)


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

__u8 read_byte (__u32 addr);
__u16 read_half_word_r (__u32 addr);
__u16 read_half_word (__u32 addr);
__u32 read_word_r (__u32 addr);
__u32 read_word (__u32 addr);
__u64 read_double (__u32 addr);
void write_byte (__u32 addr, __u8 data);
void write_half_word_r (__u32 addr, __u16 data);
void write_half_word (__u32 addr, __u16 data);
void write_word_r (__u32 addr, __u32 data);
void write_word (__u32 addr, __u32 data);
void write_double (__u32 addr, __u64 data);

inline __u8 mem_read8_safe (__u32 addr);
inline __u32 mem_read32_safe (__u32 addr);

void mem_fake_w32 (__u32 addr, __u32 data);
__u32 mem_fake_r32 (__u32 addr);
void mem_fake_w16 (__u32 addr, __u16 data);
__u16 mem_fake_r16 (__u32 addr);

void mem_hwr_hook (int bits, __u32 addr, void *ptr);
void mem_hww_hook (int bits, __u32 addr, void *ptr);

void mem_set (__u32 addr, __u8 fill, __u32 size);
void mem_copy_to_ptr (__u8 *dst, __u32 address, __u32 size);
void mem_copy_from_ptr (__u32 address, __u8 *src, __u32 size);


#endif // __MEM_H

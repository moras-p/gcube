/*
 *  gcube
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
 *
 *
 *      
 *         
 */

#include "mem.h"


#ifdef GDEBUG
# define CHECK_BOUNDS(X)\
	({\
			if ((X & MEM_MASK) > MEM_SIZE)\
				DEBUG (EVENT_EFATAL, ".mem: access outside of mem boundary 0x%.8x", X & MEM_MASK);\
	})
/*
# define CHECK_ALIGNMENT(X,n)\
	({\
			if (X & (n-1))\
				DEBUG (EVENT_EFATAL, ".mem: unaligned memory access 0x%.8x of size %d", X, n);\
	})
*/
#define CHECK_ALIGNMENT(X,n)
# define UNMAPPED_MEMORY_ACCESS(X)\
	({\
		DEBUG (EVENT_EFATAL, ".mem: access to unmapped memory 0x%.8x", X);\
	})
# define DEBUG_READ(X)				(gdebug_mem_read (X))
# define DEBUG_WRITE(X)				(gdebug_mem_write (X))
# define DEBUG_HW_READ(X)			(gdebug_hw_read (X))
# define DEBUG_HW_WRITE(X)		(gdebug_hw_write (X))
#else
# define DEBUG_READ(X)
# define DEBUG_WRITE(X)
# define CHECK_BOUNDS(X)
# define CHECK_ALIGNMENT(X,n)
# define UNMAPPED_MEMORY_ACCESS(X)
# define DEBUG_HW_READ(X)
# define DEBUG_HW_WRITE(X)
#endif

#define MEMORY_READ(X,n)	({ CHECK_BOUNDS (X); CHECK_ALIGNMENT (X, n); DEBUG_READ (X); })
#define MEMORY_WRITE(X,n)	({ CHECK_BOUNDS (X); CHECK_ALIGNMENT (X, n); })

#define HW_READ(X)				({ DEBUG_HW_READ (X); })
#define HW_WRITE(X)				({ DEBUG_HW_WRITE (X); })

#if 0
# define EFB_READ(X,n)		(DEBUG (EVENT_EFATAL, "EFB READ"))
# define EFB_WRITE(X,n)		(DEBUG (EVENT_EFATAL, "EFB WRITE"))
#else
# define EFB_READ(X,n)
# define EFB_WRITE(X,n)
#endif

# define MEM_EFB_BASE				0xc8000000
# define MEM_HW_BASE				0xcc000000
# define MEM_LC_BASE				0xe0000000


// memory
#ifdef MEMHACK
__u8 RAM[2][MEM_SIZE];
#else
__u8 RAM[MEM_SIZE];
#endif

// level 2 cache
__u8 L2C[L2C_SIZE];

// efb
__u8 EFB[EFB_SIZE];


// hardware registers hooks
__u8  (*hwread8   [0xffff]) (__u32 addr);
__u16 (*hwread16  [0xffff]) (__u32 addr);
__u32 (*hwread32  [0xffff]) (__u32 addr);
void  (*hwwrite8  [0xffff]) (__u32 addr, __u8 data);
void  (*hwwrite16 [0xffff]) (__u32 addr, __u16 data);
void  (*hwwrite32 [0xffff]) (__u32 addr, __u32 data);




// reads

inline __u8 read_byte (__u32 addr)
{
	if (addr < MEM_EFB_BASE)
	{
		MEMORY_READ (addr, 1);
		return (MEM (addr));
	}

	if (addr < MEM_HW_BASE)
	{
		// efb access
		EFB_READ (addr - MEM_EFB_BASE, 1);
		return 0;
	}

	if (addr < MEM_LC_BASE)
	{
		HW_READ (addr);
		return hwread8 [addr & 0xffff] (addr);
	}

	if (addr < (MEM_LC_BASE + L2C_SIZE))
	{
		return (L2C (addr));
	}
	
	UNMAPPED_MEMORY_ACCESS (addr);
	return 0;
}


inline __u16 read_half_word_r (__u32 addr)
{
	if (addr < MEM_EFB_BASE)
	{
		MEMORY_READ (addr, 2);
		return BIG_BSWAP16 (MEM16 (addr));
	}

	if (addr < MEM_HW_BASE)
	{
		// efb access
		EFB_READ (addr - MEM_EFB_BASE, 2);
		return 0;
	}

	if (addr < MEM_LC_BASE)
	{
		HW_READ (addr);
		return BOTH_BSWAP16 (hwread16 [addr & 0xfffe] (addr));
	}

	if (addr < (MEM_LC_BASE + L2C_SIZE))
	{
		return BIG_BSWAP16 (L2C16 (addr));
	}
	
	UNMAPPED_MEMORY_ACCESS (addr);
	return 0;
}


inline __u16 read_half_word (__u32 addr)
{
	if (addr < MEM_EFB_BASE)
	{
		MEMORY_READ (addr, 2);
		return BSWAP16 (MEM16 (addr));
	}

	if (addr < MEM_HW_BASE)
	{
		// efb access
		EFB_READ (addr - MEM_EFB_BASE, 2);
		return 0;
	}

	if (addr < MEM_LC_BASE)
	{
		HW_READ (addr);
		return hwread16 [addr & 0xfffe] (addr);
	}

	if (addr < (MEM_LC_BASE + L2C_SIZE))
	{
		return BSWAP16 (L2C16 (addr));
	}
	
	UNMAPPED_MEMORY_ACCESS (addr);
	return 0;
}


inline __u32 read_word_r (__u32 addr)
{
	if (addr < MEM_EFB_BASE)
	{
		MEMORY_READ (addr, 4);
		return BIG_BSWAP32 (MEM32 (addr));
	}

	if (addr < MEM_HW_BASE)
	{
		// efb access
		EFB_READ (addr - MEM_EFB_BASE, 4);
		return 0;
	}

	if (addr < MEM_LC_BASE)
	{
		HW_READ (addr);
		return BOTH_BSWAP32 (hwread32 [addr & 0xfffc] (addr));
	}

	if (addr < (MEM_LC_BASE + L2C_SIZE))
	{
		return BIG_BSWAP32 (L2C32 (addr));
	}
	
	UNMAPPED_MEMORY_ACCESS (addr);
	return 0;
}


inline __u32 read_word (__u32 addr)
{
	if (addr < MEM_EFB_BASE)
	{
		MEMORY_READ (addr, 4);
		return BSWAP32 (MEM32 (addr));
	}

	if (addr < MEM_HW_BASE)
	{
		// efb access
		EFB_READ (addr - MEM_EFB_BASE, 4);
		return 0;
	}

	if (addr < MEM_LC_BASE)
	{
		HW_READ (addr);
		return hwread32 [addr & 0xfffc] (addr);
	}

	if (addr < (MEM_LC_BASE + L2C_SIZE))
	{
		return BSWAP32 (L2C32 (addr));
	}
	
	UNMAPPED_MEMORY_ACCESS (addr);
	return 0;
}


inline __u64 read_double_r (__u32 addr)
{
	if (addr < MEM_EFB_BASE)
	{
		MEMORY_READ (addr, 8);
		return BIG_BSWAP64 (MEM64 (addr));
	}

	if (addr < (MEM_LC_BASE + L2C_SIZE))
	{
		return BIG_BSWAP64 (L2C64 (addr));
	}
	
	UNMAPPED_MEMORY_ACCESS (addr);
	return 0;
}


inline __u64 read_double (__u32 addr)
{
	if (addr < MEM_EFB_BASE)
	{
		MEMORY_READ (addr, 8);
		return BSWAP64 (MEM64 (addr));
	}

	if (addr < (MEM_LC_BASE + L2C_SIZE))
	{
		return BSWAP64 (L2C64 (addr));
	}
	
	UNMAPPED_MEMORY_ACCESS (addr);
	return 0;
}


// writes

inline int write_byte (__u32 addr, __u8 data)
{
	if (addr < MEM_EFB_BASE)
	{
		MEMORY_WRITE (addr, 1);
		MEM (addr) = data;
		DEBUG_WRITE (addr);
		return MMU_OK;
	}
	
	if (addr < MEM_HW_BASE)
	{
		// efb access
		EFB_WRITE (addr - MEM_EFB_BASE, 1);
		EFB (addr - MEM_EFB_BASE) = data;
		return MMU_OK;
	}

	if (addr < MEM_LC_BASE)
	{
		HW_WRITE (addr);
		hwwrite8 [addr & 0xffff] (addr, data);
		return MMU_OK;
	}

	if (addr < (MEM_LC_BASE + L2C_SIZE))
	{
		L2C (addr) = data;
		return MMU_OK;
	}
	
	UNMAPPED_MEMORY_ACCESS (addr);
	return MMU_ERROR;
}


inline int write_half_word_r (__u32 addr, __u16 data)
{
	if (addr < MEM_EFB_BASE)
	{
		MEMORY_WRITE (addr, 2);
		MEM16 (addr) = BIG_BSWAP16 (data);
		DEBUG_WRITE (addr);
		return MMU_OK;
	}

	if (addr < MEM_HW_BASE)
	{
		// efb access
		EFB_WRITE (addr - MEM_EFB_BASE, 2);
		EFB16 (addr - MEM_EFB_BASE) = BIG_BSWAP16 (data);
		return MMU_OK;
	}

	if (addr < MEM_LC_BASE)
	{
		HW_WRITE (addr);
		hwwrite16 [addr & 0xfffe] (addr, BOTH_BSWAP16 (data));
		return MMU_OK;
	}
	
	if (addr < (MEM_LC_BASE + L2C_SIZE))
	{
		L2C16 (addr) = BIG_BSWAP16 (data);
		return MMU_OK;
	}
	
	UNMAPPED_MEMORY_ACCESS (addr);
	return MMU_ERROR;
}


inline int write_half_word (__u32 addr, __u16 data)
{
	if (addr < MEM_EFB_BASE)
	{
		MEMORY_WRITE (addr, 2);
		MEM16 (addr) = BSWAP16 (data);
		DEBUG_WRITE (addr);
		return MMU_OK;
	}

	if (addr < MEM_HW_BASE)
	{
		// efb access
		EFB_WRITE (addr - MEM_EFB_BASE, 2);
		EFB16 (addr - MEM_EFB_BASE) = BSWAP16 (data);
		return MMU_OK;
	}

	if (addr < MEM_LC_BASE)
	{
		HW_WRITE (addr);
		hwwrite16 [addr & 0xfffe] (addr, data);
		return MMU_OK;
	}

	if (addr < (MEM_LC_BASE + L2C_SIZE))
	{
		L2C16 (addr) = BSWAP16 (data);
		return MMU_OK;
	}
	
	UNMAPPED_MEMORY_ACCESS (addr);
	return MMU_ERROR;
}


inline int write_word_r (__u32 addr, __u32 data)
{
	if (addr < MEM_EFB_BASE)
	{
		MEMORY_WRITE (addr, 4);
		MEM32 (addr) = BIG_BSWAP32 (data);
		DEBUG_WRITE (addr);
		return MMU_OK;
	}

	if (addr < MEM_HW_BASE)
	{
		// efb access
		EFB_WRITE (addr - MEM_EFB_BASE, 4);
		EFB32 (addr - MEM_EFB_BASE) = BSWAP32 (data);
		return MMU_OK;
	}

	if (addr < MEM_LC_BASE)
	{
		HW_WRITE (addr);
		hwwrite32 [addr & 0xfffc] (addr, BOTH_BSWAP32 (data));
		return MMU_OK;
	}

	if (addr < (MEM_LC_BASE + L2C_SIZE))
	{
		L2C32 (addr) = BIG_BSWAP32 (data);
		return MMU_OK;
	}
	
	UNMAPPED_MEMORY_ACCESS (addr);
	return MMU_ERROR;
}


inline int write_word (__u32 addr, __u32 data)
{
	if (addr < MEM_EFB_BASE)
	{
		MEMORY_WRITE (addr, 4);
		MEM32 (addr) = BSWAP32 (data);
		DEBUG_WRITE (addr);
		return MMU_OK;
	}

	if (addr < MEM_HW_BASE)
	{
		// efb access
		EFB_WRITE (addr - MEM_EFB_BASE, 4);
		EFB32 (addr - MEM_EFB_BASE) = BIG_BSWAP32 (data);
		return MMU_OK;
	}

	if (addr < MEM_LC_BASE)
	{
		HW_WRITE (addr);
		hwwrite32 [addr & 0xfffc] (addr, data);
		return MMU_OK;
	}

	if (addr < (MEM_LC_BASE + L2C_SIZE))
	{
		L2C32 (addr) = BSWAP32 (data);
		return MMU_OK;
	}
	
	UNMAPPED_MEMORY_ACCESS (addr);
	return MMU_ERROR;
}


inline int write_double (__u32 addr, __u64 data)
{
	if (addr < MEM_EFB_BASE)
	{
		MEMORY_WRITE (addr, 8);
		MEM64 (addr) = BSWAP64 (data);
		DEBUG_WRITE (addr);
		return MMU_OK;
	}

	if (addr < (MEM_LC_BASE + L2C_SIZE))
	{
		L2C64 (addr) = BSWAP64 (data);
		return MMU_OK;
	}
	
	UNMAPPED_MEMORY_ACCESS (addr);
	return MMU_OK;
}


// hw registers handling

void mem_fake_w32 (__u32 addr, __u32 data)
{
}


__u32 mem_fake_r32 (__u32 addr)
{
	return 0;
}


void mem_fake_w16 (__u32 addr, __u16 data)
{
}


__u16 mem_fake_r16 (__u32 addr)
{
	return 0;
}


// hooks

void mem_hw_hook (int read, int bits, __u32 addr, void *ptr)
{
	switch (bits)
	{
		case 8:
			if (read)
				hwread8[addr & 0xffff] = ptr;
			else
				hwwrite8[addr & 0xffff] = ptr;
			break;

		case 16:
			if (read)
				hwread16[addr & 0xffff] = ptr;
			else
				hwwrite16[addr & 0xffff] = ptr;
			break;

		case 32:
			if (read)
				hwread32[addr & 0xffff] = ptr;
			else
				hwwrite32[addr & 0xffff] = ptr;
			break;
	}
}


// hardware read hook
void mem_hwr_hook (int bits, __u32 addr, void *ptr)
{
	mem_hw_hook (TRUE, bits, addr, ptr);
}


// hardware write hook
void mem_hww_hook (int bits, __u32 addr, void *ptr)
{
	mem_hw_hook (FALSE, bits, addr, ptr);
}


__u8 hook_unhandled_read8 (__u32 addr)
{
	DEBUG (EVENT_UHW_READ, ".mem: unhandled hw read8 (0x%.4x)", addr & 0xffff);
	return 0;
}


void hook_unhandled_write8 (__u32 addr, __u8 data)
{
	DEBUG (EVENT_UHW_WRITE, ".mem: unhandled hw write8 (0x%.4x) = %.2x", addr & 0xffff, data);
}


__u16 hook_unhandled_read16 (__u32 addr)
{
	DEBUG (EVENT_UHW_READ, ".mem: unhandled hw read16 (0x%.4x)", addr & 0xffff);
	return 0;
}


void hook_unhandled_write16 (__u32 addr, __u16 data)
{
	DEBUG (EVENT_UHW_WRITE, ".mem: unhandled hw write16 (0x%.4x) = %.4x", addr & 0xffff, data);
}


__u32 hook_unhandled_read32 (__u32 addr)
{
	DEBUG (EVENT_UHW_READ, ".mem: unhandled hw read32 (0x%.4x)", addr & 0xffff);
	return 0;
}


void hook_unhandled_write32 (__u32 addr, __u32 data)
{
	DEBUG (EVENT_UHW_WRITE, ".mem: unhandled hw write32 (0x%.4x) = %.8x", addr & 0xffff, data);
}


// for debugger
inline __u8 mem_read8_safe (__u32 addr)
{
	if (addr >= L2C_BASE + L2C_SIZE)
		return 0;
	else if (addr >= L2C_BASE)
		return L2C (addr);
	else if ((addr & 0x3fffffff) < MEM_SIZE)
		return MEM (addr);
	else
		return 0;
}


inline __u32 mem_read32_safe (__u32 addr)
{
	if (addr >= L2C_BASE + L2C_SIZE)
		return 0;
	else if (addr >= L2C_BASE)
		return *((__u32 *) &L2C (addr));
	else if ((addr & 0x3fffffff) < MEM_SIZE)
		return *((__u32 *) &MEM (addr));
	else
		return 0;
}


void mem_set (__u32 address, __u8 fill, __u32 size)
{
	while (size--)
		write_byte (address++, fill);
}


void mem_copy_to_ptr (__u8 *dst, __u32 address, __u32 size)
{
	while (size--)
		*dst++ = read_byte (address++);
}


void mem_copy_from_ptr (__u32 address, __u8 *src, __u32 size)
{
	while (size--)
		write_byte (address++, *src++);
}


int mem_dump (const char *filename, __u32 address, __u32 length)
{
	FILE *f;


	f = fopen (filename, "wb");
	if (!f)
	{
		char buff[1024], fname[1024];
		
		get_filename (fname, filename);
		sprintf (buff, "%s/%s", get_home_dir (), fname);
		f = fopen (buff, "wb");

		if (!f)
			return FALSE;
	}

	fwrite (MEM_ADDRESS (address), 1, length, f);

	fclose (f);
	return TRUE;
}


void mem_reset (void)
{
	int i;


	memset (RAM, 0, MEM_SIZE);
	memset (L2C, 0, L2C_SIZE);
	memset (EFB, 0, EFB_SIZE);

	for (i = 0; i < 0xffff; i++)
	{
		mem_hwr_hook (8,  i, hook_unhandled_read8);
		mem_hwr_hook (16, i, hook_unhandled_read16);
		mem_hwr_hook (32, i, hook_unhandled_read32);
		mem_hww_hook (8,  i, hook_unhandled_write8);
		mem_hww_hook (16, i, hook_unhandled_write16);
		mem_hww_hook (32, i, hook_unhandled_write32);
	}

	DEBUG (EVENT_INFO, ".mem: reinitialized");
}

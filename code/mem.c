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


#define inline 

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


// mmu
void mmu_print_setup (void)
{
	unsigned int i;
	__u32 batu, batl;
	const char *pp_desc[] =
	{
		"No access", "Read only", "Read / Write", "Read only",
	};


	// dbats
	for (i = 0; i < 4; i++)
	{
		batu = DBATU (i);
		batl = DBATL (i);

		DEBUG (EVENT_LOG, "DBAT %d: 0x%.8x - 0x%.8x %s%s %c%c%c%c \"%s\"", i,
					 BATU_BEPI (batu) << 17,
					 ((BATU_BEPI (batu) | BATU_BL (batu)) << 17) | 0x1ffff,
					 (batu & BATU_VS) ? "VS" : "  ",
					 (batu & BATU_VP) ? "VP" : "  ",
					 (batl & BATL_W) ? 'W' : '*',
					 (batl & BATL_I) ? 'I' : '*',
					 (batl & BATL_M) ? 'M' : '*',
					 (batl & BATL_G) ? 'G' : '*',
					 pp_desc[BATL_PP (batl)]);
	}

	// ibats
	for (i = 0; i < 4; i++)
	{
		batu = IBATU (i);
		batl = IBATL (i);

		DEBUG (EVENT_LOG, "IBAT %d: 0x%.8x - 0x%.8x %s%s  %c%c  \"%s\"", i,
					 BATU_BEPI (batu) << 17,
					 ((BATU_BEPI (batu) | BATU_BL (batu)) << 17) | 0x1ffff,
					 (batu & BATU_VS) ? "VS" : "  ",
					 (batu & BATU_VP) ? "VP" : "  ",
					 (batl & BATL_I) ? 'I' : '*',
					 (batl & BATL_M) ? 'M' : '*',
					 pp_desc[BATL_PP (batl)]);
	}

	if (SDR1)
	{
		__u32 pt = SDR1_HTABORG << 16;
		__u32 size;
		
		
		i = SDR1_HTABMASK;
		size = 1 << 16;
		while ((i >>= 1))
			size <<= 1;

		DEBUG (EVENT_LOG, "PAGETABLE: 0x%.8x bytes at 0x%.8x", size, pt);
		DEBUG (EVENT_LOG, " address v vsid h api rpn r c wimg pp | effective -> physical");
		for (i = 0; i < size/8; i++)
		{
			if (MEM32 (pt))
			{
				__u32 pte1 = MEMR32 (pt);
				__u32 pte2 = MEMR32 (pt + 4);

				__u32 hash = (pt >> 6) & ((SDR1_HTABMASK << 10) | 0x3ff);
				__u32 pi = hash ^ PTE1_VSID (pte1);

				DEBUG (EVENT_LOG, "%.8x %d  %.3x %d  %.2x %.3x %d %d %d%d%d%d %d%d |  %.8x -> %.8x", pt,
							 (pte1 & PTE1_V)>0, PTE1_VSID (pte1), (pte1 & PTE1_H)>0, PTE1_API (pte1),
							 PTE2_RPN (pte2), (pte2 & PTE2_R)>0, (pte2 & PTE2_C)>0, (pte2 & PTE2_W)>0, (pte2 & PTE2_I)>0, (pte2 & PTE2_M)>0, (pte2 & PTE2_G)>0, (PTE2_PP (pte2) & 2)>0, PTE2_PP (pte2) & 1,
							 (PTE1_API (pte1) << 22) | (pi << 12), PTE2_RPN (pte2) << 12);
			}
			
			pt += 8;
		}
	}
}


// data read
#define MMU_EFFECTIVE_TO_PHYSICAL_R(pa,ea) (mmu_effective_to_physical (pa, ea, FALSE, FALSE))
// data write
#define MMU_EFFECTIVE_TO_PHYSICAL_W(pa,ea) (mmu_effective_to_physical (pa, ea, TRUE, FALSE))
// code fetch
#define MMU_EFFECTIVE_TO_PHYSICAL_F(pa,ea) (mmu_effective_to_physical (pa, ea, FALSE, TRUE))

#ifdef MMU_ENABLED
int mmu_effective_to_physical (__u32 *pa, __u32 ea, int write, int code)
{
	int i;


	// block address translation
	if (code)
	{
		if (!(MSR & MSR_IR))
		{
			*pa = ea;
			*pa &= 0x3fffffff;
			return MMU_OK;
		}

		for (i = 0; i < 4; i++)
		{
			__u32 bl = BATU_BL (IBATU (i));

			if (BATU_BEPI (IBATU (i)) == ((ea >> 17) &~ bl))
			{
				if (((IBATU (i) & BATU_VS) && !(MSR & MSR_PR)) ||
						((IBATU (i) & BATU_VP) &&  (MSR & MSR_PR)))
				{
					// page
					*pa = (BATL_BRPN (IBATL (i)) | ((ea >> 17) & bl)) << 17;
					// offset
					*pa |= (ea & 0x0001ffff);
					// necessary?
					*pa &= 0x3fffffff;
					return MMU_OK;
				}
			}
		}
	}
	else
	{
		if (!(MSR & MSR_DR))
		{
			*pa = ea;
			*pa &= 0x3fffffff;
			return MMU_OK;
		}

		// bepi -> block effective page index
		// bl -> block length
		// Vs -> supervisor mode valid bit
		// Vp -> user mode valid bit

		for (i = 0; i < 4; i++)
		{
			// bepi -> block effective page index
			// bl -> block length
			__u32 bl = BATU_BL (DBATU (i));

			if (BATU_BEPI (DBATU (i)) == ((ea >> 17) &~ bl))
			{
				if (((DBATU (i) & BATU_VS) && !(MSR & MSR_PR)) ||
						((DBATU (i) & BATU_VP) &&  (MSR & MSR_PR)))
				{
					// bat entry valid
					// todo: check access rights				
					// brpn -> physical block number
					// rpn -> real page number

					// page
					*pa = (BATL_BRPN (DBATL (i)) | ((ea >> 17) & bl)) << 17;
					// offset
					*pa |= (ea & 0x0001ffff);

					// necessary?
					*pa &= 0x3fffffff;
					return MMU_OK;
				}
			}
		}
	}


	// page address translation
	{
		__u32 sr = SR[EA_SR (ea)];
		__u32 offset = EA_OFFSET (ea);
		__u32 pi = EA_PI (ea); // 16 bit
		__u32 api = EA_API (ea);
		__u32 vsid = SR_VSID (sr); // 24 bit
		__u32 htaborg = SDR1_HTABORG; // 16 bit
		__u32 htabmask = SDR1_HTABMASK; // 9 bit
		__u32 hash, pteg_addr, pte;


		if (code && (sr & SR_N))
		// nonexecutable
		{
			cpu_exception_dont_advance (EXCEPTION_ISI);
			SRR1 |= SRR1_GUARD;
			return MMU_ERROR;
		}

		// primary hash
		hash = vsid ^ pi;
		pteg_addr = ((hash & ((htabmask << 10) | 0x3ff)) << 6) | (htaborg << 16);

		// check pteg
		for (i = 0; i < 8; i++, pteg_addr += 8)
		{
			pte = MEMR32 (pteg_addr);
			// v -> pte valid
			// h -> if set - secondary hash
			if ((pte & PTE1_V) && !(pte & PTE1_H) &&
					(vsid == PTE1_VSID (pte)) &&
					(api == PTE1_API (pte)))
			{
				// todo: check access mode

				pte = MEMR32 (pteg_addr + 4);
				*pa = (PTE2_RPN (pte) << 12) | offset;
					
				// update access bits
				pte |= write ? (PTE2_C | PTE2_R) : PTE2_R;
				MEMWR32 (pteg_addr + 4, pte);

				// necessary?
				*pa &= 0x3fffffff;
				return MMU_OK;
			}
		}
			
		// secondary hash
		hash = ~hash;
		pteg_addr = ((hash & ((htabmask << 10) | 0x3ff)) << 6) | (htaborg << 16);
			
		// check pteg
		for (i = 0; i < 8; i++, pteg_addr += 8)
		{
			pte = MEMR32 (pteg_addr);
			// v -> pte valid
			// h -> if set - secondary hash
			if ((pte & PTE1_V) && (pte & PTE1_H) &&
					(vsid == PTE1_VSID (pte)) &&
					(api == PTE1_API (pte)))
			{
				// check access mode
					
				pte = MEMR32 (pteg_addr + 4);
				*pa = (PTE2_RPN (pte) << 12) | offset;
					
				// update access bits
				pte |= write ? (PTE2_C | PTE2_R) : PTE2_R;
				MEMWR32 (pteg_addr + 4, pte);

				// necessary?
				*pa &= 0x3fffffff;
				return MMU_OK;
			}
		}
	}

	if (code)
	{
		cpu_exception_dont_advance (EXCEPTION_ISI);
		SRR1 |= SRR1_PAGE;
	}
	else
	{
		DSISR = DSISR_PAGE;
		if (write)
			DSISR |= DSISR_STORE;
		DAR = ea;

		cpu_exception_dont_advance (EXCEPTION_DSI);
	}

	return MMU_ERROR;
}


// same as above, just no exceptions
int mmu_effective_to_physical_debug (__u32 *pa, __u32 ea, int code)
{
	int i;


	// block address translation
	if (code)
	{
		if (!(MSR & MSR_IR))
		{
			*pa = ea;
//			*pa &= 0x3fffffff;
			return MMU_OK;
		}

		for (i = 0; i < 4; i++)
		{
			__u32 bl = BATU_BL (IBATU (i));

			if (BATU_BEPI (IBATU (i)) == ((ea >> 17) &~ bl))
			{
				if (((IBATU (i) & BATU_VS) && !(MSR & MSR_PR)) ||
						((IBATU (i) & BATU_VP) &&  (MSR & MSR_PR)))
				{
					// page
					*pa = (BATL_BRPN (IBATL (i)) | ((ea >> 17) & bl)) << 17;
					// offset
					*pa |= (ea & 0x0001ffff);
					// necessary?
					*pa &= 0x3fffffff;
					return MMU_OK;
				}
			}
		}
	}
	else
	{
		if (!(MSR & MSR_DR))
		{
			*pa = ea;
//			*pa &= 0x3fffffff;
			return MMU_OK;
		}

		// bepi -> block effective page index
		// bl -> block length
		// Vs -> supervisor mode valid bit
		// Vp -> user mode valid bit

		for (i = 0; i < 4; i++)
		{
			// bepi -> block effective page index
			// bl -> block length
			__u32 bl = BATU_BL (DBATU (i));

			if (BATU_BEPI (DBATU (i)) == ((ea >> 17) &~ bl))
			{
				if (((DBATU (i) & BATU_VS) && !(MSR & MSR_PR)) ||
						((DBATU (i) & BATU_VP) &&  (MSR & MSR_PR)))
				{
					// bat entry valid
					// todo: check access rights				
					// brpn -> physical block number
					// rpn -> real page number

					// page
					*pa = (BATL_BRPN (DBATL (i)) | ((ea >> 17) & bl)) << 17;
					// offset
					*pa |= (ea & 0x0001ffff);

					// necessary?
					*pa &= 0x3fffffff;
					return MMU_OK;
				}
			}
		}
	}


	// page address translation
	{
		__u32 sr = SR[EA_SR (ea)];
		__u32 offset = EA_OFFSET (ea);
		__u32 pi = EA_PI (ea); // 16 bit
		__u32 api = EA_API (ea);
		__u32 vsid = SR_VSID (sr); // 24 bit
		__u32 htaborg = SDR1_HTABORG; // 16 bit
		__u32 htabmask = SDR1_HTABMASK; // 9 bit
		__u32 hash, pteg_addr, pte;


		if (code && (sr & SR_N))
		// nonexecutable
			return MMU_ERROR;

		// primary hash
		hash = vsid ^ pi;
		pteg_addr = ((hash & ((htabmask << 10) | 0x3ff)) << 6) | (htaborg << 16);

		// check pteg
		for (i = 0; i < 8; i++, pteg_addr += 8)
		{
			pte = MEMR32 (pteg_addr);
			// v -> pte valid
			// h -> if set - secondary hash
			if ((pte & PTE1_V) && !(pte & PTE1_H) &&
					(vsid == PTE1_VSID (pte)) &&
					(api == PTE1_API (pte)))
			{
				// todo: check access mode

				pte = MEMR32 (pteg_addr + 4);
				*pa = (PTE2_RPN (pte) << 12) | offset;
					
				// necessary?
				*pa &= 0x3fffffff;
				return MMU_OK;
			}
		}
			
		// secondary hash
		hash = ~hash;
		pteg_addr = ((hash & ((htabmask << 10) | 0x3ff)) << 6) | (htaborg << 16);
			
		// check pteg
		for (i = 0; i < 8; i++, pteg_addr += 8)
		{
			pte = MEMR32 (pteg_addr);
			// v -> pte valid
			// h -> if set - secondary hash
			if ((pte & PTE1_V) && (pte & PTE1_H) &&
					(vsid == PTE1_VSID (pte)) &&
					(api == PTE1_API (pte)))
			{
				// check access mode
					
				pte = MEMR32 (pteg_addr + 4);
				*pa = (PTE2_RPN (pte) << 12) | offset;
					
				// necessary?
				*pa &= 0x3fffffff;
				return MMU_OK;
			}
		}
	}

	return MMU_ERROR;
}

#endif


// reads

inline int read_byte (__u8 *data, __u32 addr)
{
	if (!MMU_EFFECTIVE_TO_PHYSICAL_R (&addr, addr))
		return MMU_ERROR;

	if (addr < MEM_EFB_BASE)
	{
		MEMORY_READ (addr, 1);
		*data = MEM (addr);
		return MMU_OK;
	}

	if (addr < MEM_HW_BASE)
	{
		// efb access
		EFB_READ (addr - MEM_EFB_BASE, 1);
		*data = EFB (addr - MEM_EFB_BASE);
		return MMU_OK;
	}

	if (addr < MEM_LC_BASE)
	{
		HW_READ (addr);
		*data = hwread8 [addr & 0xffff] (addr);
		return MMU_OK;
	}

	if (addr < (MEM_LC_BASE + L2C_SIZE))
	{
		*data = L2C (addr);
		return MMU_OK;
	}
	
	UNMAPPED_MEMORY_ACCESS (addr);
	return MMU_ERROR;
}


inline int read_half_word_r (__u16 *data, __u32 addr)
{
	if (!MMU_EFFECTIVE_TO_PHYSICAL_R (&addr, addr))
		return MMU_ERROR;

	if (addr < MEM_EFB_BASE)
	{
		MEMORY_READ (addr, 2);
		*data = BIG_BSWAP16 (MEM16 (addr));
		return MMU_OK;
	}

	if (addr < MEM_HW_BASE)
	{
		// efb access
		EFB_READ (addr - MEM_EFB_BASE, 2);
		*data = BIG_BSWAP16 (EFB16 (addr - MEM_EFB_BASE));
		return MMU_OK;
	}

	if (addr < MEM_LC_BASE)
	{
		HW_READ (addr);
		*data = BOTH_BSWAP16 (hwread16 [addr & 0xfffe] (addr));
		return MMU_OK;
	}

	if (addr < (MEM_LC_BASE + L2C_SIZE))
	{
		*data = BIG_BSWAP16 (L2C16 (addr));
		return MMU_OK;
	}
	
	UNMAPPED_MEMORY_ACCESS (addr);
	return MMU_ERROR;
}


inline int read_half_word (__u16 *data, __u32 addr)
{
	if (!MMU_EFFECTIVE_TO_PHYSICAL_R (&addr, addr))
		return MMU_ERROR;

	if (addr < MEM_EFB_BASE)
	{
		MEMORY_READ (addr, 2);
		*data = BSWAP16 (MEM16 (addr));
		return MMU_OK;
	}

	if (addr < MEM_HW_BASE)
	{
		// efb access
		EFB_READ (addr - MEM_EFB_BASE, 2);
		*data = BSWAP16 (EFB16 (addr - MEM_EFB_BASE));
		return MMU_OK;
	}

	if (addr < MEM_LC_BASE)
	{
		HW_READ (addr);
		*data = hwread16 [addr & 0xfffe] (addr);
		return MMU_OK;
	}

	if (addr < (MEM_LC_BASE + L2C_SIZE))
	{
		*data = BSWAP16 (L2C16 (addr));
		return MMU_OK;
	}
	
	UNMAPPED_MEMORY_ACCESS (addr);
	return MMU_OK;
}


inline int read_word_r (__u32 *data, __u32 addr)
{
	if (!MMU_EFFECTIVE_TO_PHYSICAL_R (&addr, addr))
		return MMU_ERROR;

	if (addr < MEM_EFB_BASE)
	{
		MEMORY_READ (addr, 4);
		*data = BIG_BSWAP32 (MEM32 (addr));
		return MMU_OK;
	}

	if (addr < MEM_HW_BASE)
	{
		// efb access
		EFB_READ (addr - MEM_EFB_BASE, 4);
		*data = BIG_BSWAP32 (EFB32 (addr - MEM_EFB_BASE));
		return MMU_OK;
	}

	if (addr < MEM_LC_BASE)
	{
		HW_READ (addr);
		*data = BOTH_BSWAP32 (hwread32 [addr & 0xfffc] (addr));
		return MMU_OK;
	}

	if (addr < (MEM_LC_BASE + L2C_SIZE))
	{
		*data = BIG_BSWAP32 (L2C32 (addr));
		return MMU_OK;
	}
	
	UNMAPPED_MEMORY_ACCESS (addr);
	return MMU_ERROR;
}


inline int read_word (__u32 *data, __u32 addr)
{
	if (!MMU_EFFECTIVE_TO_PHYSICAL_R (&addr, addr))
		return MMU_ERROR;

	if (addr < MEM_EFB_BASE)
	{
		MEMORY_READ (addr, 4);
		*data = BSWAP32 (MEM32 (addr));
		return MMU_OK;
	}

	if (addr < MEM_HW_BASE)
	{
		// efb access
		EFB_READ (addr - MEM_EFB_BASE, 4);
		*data = BSWAP32 (EFB32 (addr - MEM_EFB_BASE));
		return MMU_OK;
	}

	if (addr < MEM_LC_BASE)
	{
		HW_READ (addr);
		*data = hwread32 [addr & 0xfffc] (addr);
		return MMU_OK;
	}

	if (addr < (MEM_LC_BASE + L2C_SIZE))
	{
		*data = BSWAP32 (L2C32 (addr));
		return MMU_OK;
	}
	
	UNMAPPED_MEMORY_ACCESS (addr);
	return MMU_ERROR;
}


inline int read_single (__u64 *data, __u32 addr)
{
	if (!MMU_EFFECTIVE_TO_PHYSICAL_R (&addr, addr))
		return MMU_ERROR;

	if (addr < MEM_EFB_BASE)
	{
		MEMORY_READ (addr, 4);
		*data = U32TODOUBLE64 (BSWAP32 (MEM32 (addr)));
		return MMU_OK;
	}

	if (addr < (MEM_LC_BASE + L2C_SIZE))
	{
		*data = U32TODOUBLE64 (BSWAP32 (L2C32 (addr)));
		return MMU_OK;
	}
	
	UNMAPPED_MEMORY_ACCESS (addr);
	return MMU_ERROR;
}


inline int read_double (__u64 *data, __u32 addr)
{
	if (!MMU_EFFECTIVE_TO_PHYSICAL_R (&addr, addr))
		return MMU_ERROR;

	if (addr < MEM_EFB_BASE)
	{
		MEMORY_READ (addr, 8);
		*data = BSWAP64 (MEM64 (addr));
		return MMU_OK;
	}

	if (addr < (MEM_LC_BASE + L2C_SIZE))
	{
		*data = BSWAP64 (L2C64 (addr));
		return MMU_OK;
	}
	
	UNMAPPED_MEMORY_ACCESS (addr);
	return MMU_ERROR;
}


// writes

inline int write_byte (__u32 addr, __u8 data)
{
	if (!MMU_EFFECTIVE_TO_PHYSICAL_W (&addr, addr))
		return MMU_ERROR;

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
	if (!MMU_EFFECTIVE_TO_PHYSICAL_W (&addr, addr))
		return MMU_ERROR;

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
	if (!MMU_EFFECTIVE_TO_PHYSICAL_W (&addr, addr))
		return MMU_ERROR;

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
	if (!MMU_EFFECTIVE_TO_PHYSICAL_W (&addr, addr))
		return MMU_ERROR;

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
	if (!MMU_EFFECTIVE_TO_PHYSICAL_W (&addr, addr))
		return MMU_ERROR;

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
	if (!MMU_EFFECTIVE_TO_PHYSICAL_W (&addr, addr))
		return MMU_ERROR;

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
				hwread8[addr & 0xffff] = (__u8 (*) (__u32)) ptr;
			else
				hwwrite8[addr & 0xffff] = (void (*) (__u32, __u8)) ptr;
			break;

		case 16:
			if (read)
				hwread16[addr & 0xffff] = (__u16 (*) (__u32)) ptr;
			else
				hwwrite16[addr & 0xffff] = (void (*) (__u32, __u16)) ptr;
			break;

		case 32:
			if (read)
				hwread32[addr & 0xffff] = (__u32 (*) (__u32)) ptr;
			else
				hwwrite32[addr & 0xffff] = (void (*) (__u32, __u32)) ptr;
			break;
	}
}


// hardware read hook
void mem_hwr_hookx (int bits, __u32 addr, void *ptr)
{
	mem_hw_hook (TRUE, bits, addr, ptr);
}


// hardware write hook
void mem_hww_hookx (int bits, __u32 addr, void *ptr)
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


int mem_set (__u32 address, __u8 fill, __u32 size)
{
	while (size--)
		if (!write_byte (address++, fill))
			return MMU_ERROR;

	return MMU_OK;
}


int mem_copy_to_ptr (__u8 *dst, __u32 address, __u32 size)
{
	while (size--)
		if (!read_byte (dst++, address++))
			return MMU_ERROR;

	return MMU_OK;
}


int mem_copy_from_ptr (__u32 address, __u8 *src, __u32 size)
{
	while (size--)
		if (!write_byte (address++, *src++))
			return MMU_ERROR;

	return MMU_OK;
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


inline int mem_fetch (__u32 pc, __u32 *op)
{
	if (MMU_EFFECTIVE_TO_PHYSICAL_F (&pc, pc))
	{
		*op = MEMR32 (pc);
		return MMU_OK;
	}
	else
		return MMU_ERROR;
}

/*
inline int mem_block_zero (__u32 addr)
{
	if (!MEM_WDOUBLE (addr +  0, (__u64) 0))
		return MMU_ERROR;

	MEM_WDOUBLE (addr +  8, (__u64) 0);
	MEM_WDOUBLE (addr + 16, (__u64) 0);
	MEM_WDOUBLE (addr + 24, (__u64) 0);

	return MMU_OK;
}
*/


inline int mem_block_zero (__u32 addr)
{
	if (!MMU_EFFECTIVE_TO_PHYSICAL_W (&addr, addr))
		return MMU_ERROR;

	MEM64 (addr +  0) = 0;
	MEM64 (addr +  8) = 0;
	MEM64 (addr + 16) = 0;
	MEM64 (addr + 24) = 0;

	return MMU_OK;
}

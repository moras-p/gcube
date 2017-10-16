#ifndef __HW_EXI_H
#define __HW_EXI_H 1


#include "general.h"
#include "mem.h"


#define REXI_SIZE						0x100
#define REXI_MASK						0x0ff

#define REXI32(X)						(*((__u32 *)(&rexi[(X) & REXI_MASK])))

extern __u8 rexi[REXI_SIZE];

#define SRAM(A)					(((__u8 *) &sram)[A])


#define EXI_BASE						0x6800
#define EXI_0CSR						(EXI_BASE + 0x00)
#define EXI_0MAR						(EXI_BASE + 0x04)
#define EXI_0LEN						(EXI_BASE + 0x08)
#define EXI_0CR							(EXI_BASE + 0x0c)
#define EXI_0DATA						(EXI_BASE + 0x10)
#define EXI_1CSR						(EXI_BASE + 0x14)
#define EXI_1MAR						(EXI_BASE + 0x18)
#define EXI_1LEN						(EXI_BASE + 0x1c)
#define EXI_1CR							(EXI_BASE + 0x20)
#define EXI_1DATA						(EXI_BASE + 0x24)
#define EXI_2CSR						(EXI_BASE + 0x28)
#define EXI_2MAR						(EXI_BASE + 0x2c)
#define EXI_2LEN						(EXI_BASE + 0x30)
#define EXI_2CR							(EXI_BASE + 0x34)
#define EXI_2DATA						(EXI_BASE + 0x38)

#define REXI_CSR(n)					REXI32 (EXI_0CSR  + 0x14 * n)
#define REXI_MAR(n)					REXI32 (EXI_0MAR  + 0x14 * n)
#define REXI_LEN(n)					REXI32 (EXI_0LEN  + 0x14 * n)
#define REXI_CR(n)					REXI32 (EXI_0CR   + 0x14 * n)
#define REXI_DATA(n)				REXI32 (EXI_0DATA + 0x14 * n)

#define EXI_CSR_ROMDIS			(1 << 13)
// external device attached status / interrupt / mask
#define EXI_CSR_EXT					(1 << 12)
#define EXI_CSR_EXTINT			(1 << 11)
#define EXI_CSR_EXTINTMSK		(1 << 10)
#define EXI_CSR_CS2					(1 <<  9)
#define EXI_CSR_CS1					(1 <<  8)
#define EXI_CSR_CS0					(1 <<  7)
// transfer complete interrupt
#define EXI_CSR_TCINT				(1 <<  3)
#define EXI_CSR_TCINTMSK		(1 <<  2)
// external device detached interrupt / mask
#define EXI_CSR_EXIINT			(1 <<  1)
#define EXI_CSR_EXIINTMSK		(1 <<  0)

#define EXI_CR_TSTART				(1 << 0)
#define EXI_CR_DMA					(1 << 1)


#define EXI_INTERRUPT_TC			(1 << 0)
#define EXI_INTERRUPT_EXT			(1 << 1)
#define EXI_INTERRUPT_EXI			(1 << 2)


#define EXI_OFS_ROM						0x00000000
#define EXI_OFS_RTC						0x20000000
#define EXI_OFS_SRAM					0x20000100
#define EXI_OFS_UART					0x20010000

#define ROM_FONTS_START				(0x001aff00 << 6)
#define ROM_FONTS_END					(0x001fff00 << 6)

#define EXI_CMD_WRITE					0x80000000

#define EXI_SRAM_SIZE					64

#define SRAM_ADDRESS(a)			(&((__u8 *) &sram)[a])
#define SRAM_OFFSET(a)			(((a - 0x100) >> 6) & 0x7f)

#define MC_STATUS_READY						1

#define MX_UART_WRITE			1
#define MX_SRAM_WRITE			2

#define MC_BLOCK_SIZE		0x2000
#define MC_ERASE_BYTE		0x00
#define MC_HID					0xc242

#define MEMC(n,A)					(((__u8 *) memc[n].gmc)[A])
#define MEMC16(n,A)				(*((__u16 *) &MEMC (n, A)))
#define MEMC32(n,A)				(*((__u32 *) &MEMC (n, A)))
#define MEMC_R16(n,A)			(BSWAP16 (MEMC16 (n, A)))
#define MEMC_R32(n,A)			(BSWAP32 (MEMC32 (n, A)))
#define MEMC_W16(n,A,d)		(MEMC16 (n, A) = BSWAP16 (d))
#define MEMC_W32(n,A,d)		(MEMC32 (n, A) = BSWAP32 (d))

#define MEMC_SIZE(n)			((memc[n].size << 4) * MC_BLOCK_SIZE)
#define MEMC_CMD_OFFS(n)  ((memc[n].cmd[1] << 17) | (memc[n].cmd[2] << 9) | (memc[n].cmd[3] << 7) | (memc[n].cmd[4] & 0x7f))

#define MC_CMD_GETEXIID						0x00
#define MC_CMD_READBLOCK					0x52
#define MC_CMD_INTENABLE					0x81
#define MC_CMD_GETSTATUS					0x83
#define MC_CMD_READID							0x85
#define MC_CMD_CLEARSTATUS				0x89
#define MC_CMD_ERASEBLOCK					0xf1
#define MC_CMD_WRITEBLOCK					0xf2


typedef struct
{
	__u16 checksum1, checksum2;
	__u32 ead0, ead1;
	__u32 cntbias;
	__s8  dispofsh;
	__u8  ntd;
	__u8  language;  // english/german/french/spanish/italian/dutch
	__u8  flags;
	__u32 flashid[6];
	__u32 wkbdid;
	__u32 wpadid[2];
	__u8  lastdvderror;
	__u8  reserved1;
	__u16 flashid_checksum[2];
	__u16 reserved2;
} SRAMS;

typedef struct
{
	__u32 unk[3];
	__u32 time[2];
	__u32 cardid[3];
	__u16 pad1;
	__u16 size;
	__u16 encoding;
	__u8 unused1[0x1d4];
	__u16 cnt;
	__u16 chksum1, chksum2;
	__u8 unused2[0x1e00];
} GMCHeader;

typedef struct
{
	__u32 gamecode;
	__u16 makercode;
	__u8 unused1;
	__u8 gfxi;
	__u8 filename[0x20];
	__u32 mtime;
	__u32 gfxofs;
	__u16 gfxfmt;
	__u16 animspeed;
	__u8 permission;
	__u8 copycnt;
	__u16 firstblock;
	__u16 size;
	__u16 unused2;
	__u32 commentofs;
} GMCDirEntry;

typedef struct
{
	GMCDirEntry items[127];
	__u8 pad[0x3a];
	__u16 cnt;
	__u16 chksum1, chksum2;
} GMCDirectory;

typedef struct
{
	__u16 chksum1, chksum2;
	__u16 cnt;
	__u16 bfree;
	__u16 blast;
	__u8 map[0x1ff6];
} GMCFAT;

typedef struct
{
	GMCHeader hdr;
	GMCDirectory dir, dirback;
	GMCFAT fat, fatback;
	// data will be larger than this
	__u8 data[MC_BLOCK_SIZE];
} GMC;

typedef struct
{
	__u8 cmd[16];
	__u32 bleft, ncmd, cmdsize, rofs;

	__u8 status;
	__u16 hid;

	GMC *gmc;
	__u32 size;
} Memcard;




void exi_init (void);
void exi_reinit (void);
void exi_attach (int n);

void exi_generate_interrupt (__u32 mask, int channel);


void mc_format (int n);
void mc_open (int n, int size);
void mc_close (int n);
int mc_save (int n, const char *filename);
int mc_load (int n, const char *filename);
void mc_init (void);
void mc_cleanup (void);
int mc_import_file (int n, const char *filename);
void mc_attach (int n);


#endif // __HW_EXI_H

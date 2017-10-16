#ifndef __HW_DI_H
#define __HW_DI_H 1


#include "general.h"
#include "diskio.h"

#define RDI_SIZE						0x100
#define RDI_MASK						0x0ff

#define RDI32(X)						(* (__u32 *) (&rdi[(X) & RDI_MASK]))

extern __u8 rdi[RDI_SIZE];


#define DI_SR								0x6000
#define DISR_BRKINT					(1 << 6)
#define DISR_BRKINTMSK			(1 << 5)
#define DISR_TCINT					(1 << 4)
#define DISR_TCINTMSK				(1 << 3)
#define DISR_DEINT					(1 << 2)
#define DISR_DEINTMSK				(1 << 1)
#define DISR_BRK						(1 << 0)

#define DI_CVR							0x6004
#define DICVR_CVRINT				(1 << 2)
#define DICVR_CVRINTMSK			(1 << 1)
#define DICVR_OPENED				(1 << 0)

#define DI_CMDBUF0					0x6008
#define DI_CMDBUF1					0x600c
#define DI_CMDBUF2					0x6010
#define DI_MAR							0x6014
#define DI_LENGTH						0x6018

#define DI_CR								0x601c
#define DICR_RW							(1 << 2)
#define DICR_DMA						(1 << 1)
#define DICR_TSTART					(1 << 0)

#define DI_IMMBUF						0x6020

#define DI_CFG							0x6024
#define DI_CFG_CONFIG				0xff

#define DVD_DOL_OFFSET			0x0420
#define DVD_FST_OFFSET			0x0424
#define DVD_FST_SIZE				0x0428
#define DVD_FST_MAX_SIZE		0x042c

#define DIR_ITEM_FILE					0
#define DIR_ITEM_DIRECTORY		1
#define DIR_ITEM_EXECUTABLE		2
#define DIR_ITEM_GCM					3

#define DVD_HEADER_SIZE				0x2440
#define DATA_BASE_OFFSET			0x3000

#define DISR								(RDI32 (DI_SR))
#define DICR								(RDI32 (DI_CR))
#define DICVR								(RDI32 (DI_CVR))


typedef struct DirItemTag
{
	int type;
	char *name;
	
	unsigned int offset;
	unsigned int size;

	struct DirItemTag *next;
	void *sub, *parent;
} DirItem;

typedef struct
{
	int nitems;
	DirItem *items;
	char *path;

	unsigned int last_pos;
} Dir;

typedef struct
{
	char *filename;
	void *f;

	unsigned int pos;

	Dir *root;

	// virtual dvd
	int vdvd_inserted;
	char *header;
	char *fst;
	unsigned int fst_size;
} MiniDVDImage;

extern MiniDVDImage mdvd;


#define DI_INTERRUPT_BRK			(1 << 0)
#define DI_INTERRUPT_TC				(1 << 1)
#define DI_INTERRUPT_DE				(1 << 2)
#define DI_INTERRUPT_CVR			(1 << 3)


void di_init (void);
void di_reinit (void);

void di_generate_interrupt (__u32 mask);


DirItem *dir_get_item (Dir *dir, int n);
int mdvd_open (char *filename);
int mdvd_open_fast (char *filename);
void mdvd_close (void);
unsigned int mdvd_seek (unsigned int offset);
unsigned int mdvd_seek_whence (unsigned int offset, int whence);
unsigned int mdvd_read (void *dest, unsigned int size);
int mdvd_inserted (void);

int vdvd_open (char *filename);

void dir_destroy (Dir *dir);
Dir *mdvd_load_directory (char *filename);

void boot_apploader (void);
int is_gcm (char *filename);

int mdvd_get_country_code (void);

void di_read_stream (void *dst, __u32 offset, __u32 size);


#endif // __HW_DI_H

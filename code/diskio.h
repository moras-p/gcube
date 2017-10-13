#ifndef __DISKIO_H
#define __DISKIO_H 1


#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include "elf.h"

#include "general.h"
#include "gdebug.h"
#include "mem.h"

#define DOL_NTEXT 7
#define DOL_NDATA 11

#define GCS_COMPRESSED_GZ				0x01

#define GC_FILE_ERROR						0xffffffff

// text - code (read-only)
// data - data (read-write)
// bss - uninitialized data (read-write)

typedef struct
{
	__u32 text_offset[7];
	__u32 data_offset[11];

	__u32 text_address[7];
	__u32 data_address[11];

	__u32 text_size[7];
	__u32 data_size[11];

	__u32 bss_address;
	__u32 bss_size;

	__u32 entry_point;

	__u32 padd[7];
} DOLHeader;


typedef struct
{
	__u8  magic[4];
	__u32 version;
	__u32 flags;
	__u32 reserved[15];

	// filename of the gcm (if any)
	// must be in the same dir as savestate
	char filename[256];
	// yet to come
	__u32 screenshot_size;
	__u32 screenshot_offset;

	// block size and position in file
	__u32 mem_size;
	__u32 mem_offset;

	__u32 l2c_size;
	__u32 l2c_offset;

	__u32 aram_size;
	__u32 aram_offset;
	
	__u32 regs_size;
	__u32 regs_offset;
	__u32 ps0regs_size;
	__u32 ps0regs_offset;
	__u32 ps1regs_size;
	__u32 ps1regs_offset;

	__u32 dspregs_size;
	__u32 dspregs_offset;

	__u32 airegs_size;
	__u32 airegs_offset;

	__u32 cpregs_size;
	__u32 cpregs_offset;

	__u32 diregs_size;
	__u32 diregs_offset;

	__u32 eiregs_size;
	__u32 eiregs_offset;

	__u32 miregs_size;
	__u32 miregs_offset;

	__u32 peregs_size;
	__u32 peregs_offset;

	__u32 piregs_size;
	__u32 piregs_offset;

	__u32 siregs_size;
	__u32 siregs_offset;

	__u32 viregs_size;
	__u32 viregs_offset;
	
	__u32 gxstate_size;
	__u32 gxstate_offset;

	__u32 dspstate_size;
	__u32 dspstate_offset;
} GCSHeader;


#define IMP_MAGIC						"\0x7fIMP"

typedef struct IMPMarkTag
{
	__u32 offset;
	__u32 length;

	struct IMPMarkTag *next;
} IMPMark;

typedef struct
{
	char magic[4];
	__u32 version;
	__u32 nitems;
	__u8 reserved[4];
} IMPHeader;

#define FILE_IMP				1

typedef struct
{
	FILE *f;

	IMPMark *marks, *current_mark;

	unsigned int zero_offset;
	int compressed_offset;
	unsigned int pos;

	__u8 data;
} IMPFile;

typedef struct
{
	char *filename;
	int type;

	void *f;
} File;


__u32 load_bin (char *filename);
__u32 load_dol (char *filename);
int is_elf (char *filename);
__u32 load_elf (char *filename);

int is_gcs (char *filename);
void save_state (char *filename, int compressed);
__u32 load_state (char *filename);

__u32 load_gc (char *filename);

int is_imp (char *fname);
void imp_close (IMPFile *file);
IMPFile *imp_open (char *fname);
int imp_seek (IMPFile *file, int offset, int whence);
int imp_read (IMPFile *file, char *buff, unsigned int size);

File *file_open (char *filename);
void file_close (File *file);
int file_seek (File *file, int offset, int whence);
int file_tell (File *file);
int file_read (File *file, void *buff, unsigned int size);
__u32 file_read_w (File *file);


#endif // __DISKIO_H

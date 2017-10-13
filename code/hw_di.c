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
 *  DI - dvd interface (0xcc006000)
 *      
 *         
 */

#include "hw_di.h"
#include "mem.h"
#include "hle.h"

#include <dirent.h>
#include <sys/stat.h>


__u8 rdi[RDI_SIZE];


MiniDVDImage mdvd;

unsigned int mdvd_read_fixed (void *dest, unsigned int size);


__u32 di_r32_direct (__u32 addr)
{
	DEBUG (EVENT_LOG_DI, "..di: read  [%.4x] (%.8x)", addr & 0xffff, RDI32 (addr));
	return RDI32 (addr);
}


void di_w32_direct (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_DI, "..di: write [%.4x] (%.8x) = %.8x", addr & 0xffff, RDI32 (addr), data);
	RDI32 (addr) = data;
}


void di_update_interrupts (void)
{
	pi_interrupt (INTERRUPT_DI,
		((DISR & DISR_BRKINT) && (DISR & DISR_BRKINTMSK)) ||
		((DISR & DISR_TCINT) && (DISR & DISR_TCINTMSK)) ||
		((DISR & DISR_DEINT) && (DISR & DISR_DEINTMSK)) ||
		((DICVR & DICVR_CVRINT) && (DICVR & DICVR_CVRINTMSK)));
}


void di_generate_interrupt (__u32 mask)
{
	if (mask & DI_INTERRUPT_BRK)
		DISR |= DISR_BRKINT;

	if (mask & DI_INTERRUPT_TC)
		DISR |= DISR_TCINT;

	if (mask & DI_INTERRUPT_DE)
		DISR |= DISR_DEINT;

	if (mask & DI_INTERRUPT_CVR)
		DICVR |= DICVR_CVRINT;
	
	di_update_interrupts ();
}


void di_w32_disr (__u32 addr, __u32 data)
{
	// acknowledge interrupts
	RDI32 (addr) &= ~(data & (DISR_DEINT | DISR_TCINT | DISR_BRKINT));
	
	RDI32 (addr) = (RDI32 (addr) &~ (DISR_DEINTMSK | DISR_TCINTMSK | DISR_BRKINTMSK))
									| (data & (DISR_DEINTMSK | DISR_TCINTMSK | DISR_BRKINTMSK));

	if (data & DISR_BRK)
	{
/*
		DEBUG (EVENT_LOG_DI, "..di: DI break");
	
		RDI32 (addr) |= DISR_BRKINT;
		if (RDI32 (addr) & DISR_BRKINTMSK)
			pi_interrupt (INTERRUPT_DI);
*/
		DEBUG (EVENT_EFATAL, "..di: DI break");
	}
	else
		DEBUG (EVENT_LOG_DI, "..di: DISR %.8x", data);

	di_update_interrupts ();
}


void di_w32_cover (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_DI, "..di: COVER %.8x", data);
	
	// clear interrupt
	if (data & DICVR_CVRINT)
		RDI32 (addr) &= ~DICVR_CVRINT;

	RDI32 (addr) = (RDI32 (addr) &~ DICVR_CVRINTMSK) | (data & DICVR_CVRINTMSK);
	
	di_update_interrupts ();
}


void di_w32_cfg (__u32 addr, __u32 data)
{
	DEBUG (EVENT_LOG_DI, "..di: CONFIG %.8x", data);
	
	RDI32 (addr) = data & DI_CFG_CONFIG;
}


void di_execute_command (void)
{
	switch (RDI32 (DI_CMDBUF0))
	{
		// drive info
		case 0x12000000:
			DEBUG (EVENT_LOG_DI, "....  CMD drive info");

			MEM_SET (RDI32 (DI_MAR) & 0x03ffffff, 0, RDI32 (DI_LENGTH) & 0x03ffffff);
			RDI32 (DI_LENGTH) = 0;
			break;
		
		// read disk id
		case 0xa8000040:
			DEBUG (EVENT_LOG_DI, "....  CMD read disk id");
			// fall through

		// read sector
		case 0xa8000000:
			DEBUG (EVENT_LOG_DI, "....  CMD read %.8x bytes DVD[%.8x] -> MEM[%.8x]",
							RDI32 (DI_LENGTH), RDI32 (DI_CMDBUF1) << 2, RDI32 (DI_MAR));

			mdvd_seek (RDI32 (DI_CMDBUF1) << 2);
//			mdvd_read (MEM_ADDRESS (RDI32 (DI_MAR) & 0x03ffffff), RDI32 (DI_LENGTH) & 0x03ffffff);
			mdvd_read_fixed (MEM_ADDRESS (RDI32 (DI_MAR) & 0x03ffffff), RDI32 (DI_LENGTH) & 0x03ffffff);
			RDI32 (DI_LENGTH) = 0;
			break;

		// seek
		case 0xab000000:
			DEBUG (EVENT_LOG_DI, "....  CMD seek to %8.x", RDI32 (DI_CMDBUF1) << 2);
			mdvd_seek (RDI32 (DI_CMDBUF1) << 2);
			break;

		// request error
		case 0xe0000000:
			DEBUG (EVENT_LOG_DI, "....  CMD request error");
			break;
		
		// play audio stream
		case 0xe1000000:
			DEBUG (EVENT_LOG_DI, "....  CMD play audio stream");
			break;

		// request audio status
		case 0xe2000000:
			DEBUG (EVENT_LOG_DI, "....  CMD request audio status");
			break;

		// stop motor
		case 0xe3000000:
			DEBUG (EVENT_LOG_DI, "....  CMD stop motor");
			break;
		
		// dvd audio disable
		case 0xe4000000:
			DEBUG (EVENT_LOG_DI, "....  CMD dvd audio disable");
			break;
		
		// dvd audio enable
		case 0xe4010000:
			DEBUG (EVENT_LOG_DI, "....  CMD dvd audio enable");
			break;
		
		default:
			DEBUG (EVENT_LOG_DI, "....  CMD unknown command (%.8x)", RDI32 (DI_CMDBUF0));
			break;
	}
/*
	if (RDI32 (DI_CR) & DICR_DMA)
		RDI32 (DI_LENGTH) = 0;
*/
}


void di_w32_dicr (__u32 addr, __u32 data)
{
	RDI32 (addr) = data;

	if (data & DICR_TSTART)
	{
		DEBUG (EVENT_LOG_DI, "..di: DI COMMAND EXECUTE");
		RDI32 (addr) &= ~DICR_TSTART;

		di_execute_command ();

		// transfer complete interrupt
		RDI32 (DI_SR) |= DISR_TCINT;
		di_update_interrupts ();
	}
	else
		DEBUG (EVENT_LOG_DI, "..di: DICR %.8x", data);
}


void di_open_cover (void)
{
	if (!(RDI32 (DI_CVR) & DICVR_OPENED))
	{
		RDI32 (DI_CVR) |= DICVR_OPENED;

		// cover interrupt
		RDI32 (DI_CVR) |= DICVR_CVRINT;
		di_update_interrupts ();
	}
}


void di_close_cover (void)
{
	if (RDI32 (DI_CVR) & DICVR_OPENED)
	{
		RDI32 (DI_CVR) &= ~DICVR_OPENED;
	
		// cover interrupt
		RDI32 (DI_CVR) |= DICVR_CVRINT;
		di_update_interrupts ();
	}
}


// dir
DirItem *dir_get_item (Dir *dir, int n)
{
	DirItem *p = dir->items;
	int i;


	for (i = 0; i < n; i++)
		if (!(p = p->next))
			break;

	return p;
}


DirItem *dir_find_item_by_offset (Dir *dir, __u32 offset, Dir **parent)
{
	DirItem *p, *k;


	if (!dir)
		return NULL;

	p = dir->items;
	while (p)
	{
		if (p->type == DIR_ITEM_DIRECTORY && (k = dir_find_item_by_offset (p->sub, offset, parent)))
				return k;
		else if (offset >= p->offset && offset < p->offset + p->size)
		{
			if (parent)
				*parent = dir;
			return p;
		}
		else
			p = p->next;
	}
	
	return NULL;
}


DirItem *dir_get_item_by_path (Dir *dir, const char *filename)
{
	DirItem *p = dir->items;
	char *t;
	int len;


	if (!dir)
		return NULL;

	t = strchr (filename, '/');
	if (t)
		len = t - filename;
	else
		len = strlen (filename);

	while (p)
	{
		if (0 == strncmp (filename, p->name, len))
		{
			if (p->type == DIR_ITEM_DIRECTORY)
				return dir_get_item_by_path (p->sub, filename + len + 1);
			else
				return p;
		}

		p = p->next;
	}

	return NULL;
}


DirItem *dir_find_item_by_offset_aligned (Dir *dir, __u32 offset, Dir **parent)
{
	DirItem *p, *k;


	if (!dir)
		return NULL;

	p = dir->items;
	while (p)
	{
		if (p->type == DIR_ITEM_DIRECTORY && (k = dir_find_item_by_offset_aligned (p->sub, offset, parent)))
				return k;
		else if (offset >= (p->offset &~ 3) && offset < (p->offset &~ 3) + p->size)
		{
			if (parent)
				*parent = dir;
			return p;
		}
		else
			p = p->next;
	}
	
	return NULL;
}


void dir_add_item (Dir *dir, DirItem *item)
{
	DirItem *n = dir->items;


	if (n)
	{
		while (n->next)
			n = n->next;

		n->next = item;
	}
	else
		dir->items = item;

	dir->nitems++;
}


void dir_sort (Dir *dir)
{
	DirItem *l, *k, *p;
	int i;


	// directories first
	i = 1;
	while (i)
	{
		i = 0;
		l = p = dir->items;
		while (p)
		{
			if ((p->type == DIR_ITEM_FILE) && p->next && (p->next->type == DIR_ITEM_DIRECTORY))
			{
				k = p->next;
				// switch p and k
				l->next = k;
				p->next = k->next;
				k->next = p;

				l = k;
				i++;
			}
			else
			{
				l = p;
				p = p->next;
			}
		}
	}
}


int dir_add_items (Dir *dir, __u32 *fst, char *st, int offset, int last_offset)
{
	DirItem *item;
	int nitems = 0;


	while (offset < last_offset)
	{
		item = calloc (1, sizeof (DirItem));

		item->name = strdup (&st[BSWAP32 (fst[offset*3]) & 0x00ffffff]);
		if (*item->name < 0x20 || *item->name > 0x7f)
		{
			free (item->name);
			offset++;

			continue;
		}
		item->type = BSWAP32 (fst[offset*3]) >> 24;
		item->offset = BSWAP32 (fst[offset*3 + 1]);
		item->size = BSWAP32 (fst[offset*3 + 2]);

		DEBUG (EVENT_LOG_DI, "FST %d %.8x %.8x %s", item->type, item->offset, item->size, item->name);

		dir_add_item (dir, item);
		offset++;

		if (item->type == DIR_ITEM_DIRECTORY)
		{
			// first entry
			DirItem *first = calloc (1, sizeof (DirItem));

			first->type = DIR_ITEM_DIRECTORY;
			first->name = strdup ("..");
			first->sub = NULL;
			first->parent = dir;

			item->parent = dir;
			item->sub = calloc (1, sizeof (Dir));
			
			dir_add_item (item->sub, first);
			dir_add_items (item->sub, fst, st, offset, item->size);
			offset = item->size;
		}
	}

	dir_sort (dir);	
	return nitems;
}


DirItem *dir_item_create (char *name, int type, unsigned int offset, unsigned int size)
{
	DirItem *item = calloc (1, sizeof (DirItem));


	item->name = strdup (name);
	item->type = type;
	item->offset = offset;
	item->size = size;

	return item;
}


Dir *dir_create (char *path, Dir *parent)
{
	struct dirent **namelist;
	struct stat attribs;
	char buff[1024];
	DirItem *item;
	Dir *dir, *sub;
	int i, n, nitems;


	n = scandir (path, &namelist, NULL, alphasort);
	if (n <= 0)
	{
		if (n == 0)
			free (namelist);

		return 0;
	}

	dir = calloc (1, sizeof (Dir));
	dir->path = strdup (path);
	nitems = n - 2;

	item = dir_item_create (strdup (".."), DIR_ITEM_DIRECTORY, 0, 0);
	item->parent = parent;
	dir_add_item (dir, item);

	// no '.' and '..' entry
	// first time, only directories
	for (i = 0, n = 0; i < nitems; i++)
	{
		sprintf (buff, "%s/%s", path, namelist[i + 2]->d_name);
		if (!((0 == stat (buff, &attribs)) && S_ISDIR (attribs.st_mode)))
			continue;

		sub = dir_create (buff, dir);
		if (sub)
		{
			item = dir_item_create (strdup (namelist[i + 2]->d_name),
													  	 DIR_ITEM_DIRECTORY, 0, 0);
			item->parent = dir;
			item->sub = sub;
			dir_add_item (dir, item);
		}

		n++;
	}

	for (i = 0; i < nitems; i++)
	{
		sprintf (buff, "%s/%s", path, namelist[i + 2]->d_name);
		if (!((0 == stat (buff, &attribs)) && !S_ISDIR (attribs.st_mode)))
			continue;

		item = dir_item_create (strdup (namelist[i + 2]->d_name),
												  	 DIR_ITEM_DIRECTORY, 0, 0);
		item->size = file_size (buff);

		get_extension (buff, item->name);
		if ((0 == strcasecmp (buff, "dol")) ||
				(0 == strcasecmp (buff, "elf")))
			item->type = DIR_ITEM_EXECUTABLE;
		else if ((0 == strcasecmp (buff, "gcm")) ||
						 (0 == strcasecmp (buff, "imp")) ||
						 (0 == strcasecmp (&item->name[strlen (item->name) - 6], "gcm.gz")))
			item->type = DIR_ITEM_GCM;
		else
			item->type = DIR_ITEM_FILE;

		dir_add_item (dir, item);
		
		n++;
		free (namelist[i + 2]);
	}

	free (namelist[1]);
	free (namelist[0]);
	free (namelist);

	return dir;
}


void dir_destroy (Dir *dir)
{
	DirItem *p = dir->items, *n = NULL;


	while (p)
	{
		n = p->next;
	
		if (p->type == DIR_ITEM_DIRECTORY && p->sub)
			dir_destroy (p->sub);

		free (p->name);
		free (p);

		p = n;
	}
}


int is_gcm (char *filename)
{
	// only by extension
	if ((0 == strcasecmp (&filename[strlen (filename) - 3], "gcm")) ||
			(0 == strcasecmp (&filename[strlen (filename) - 6], "gcm.gz")) ||
			(0 == strcasecmp (&filename[strlen (filename) - 3], "imp")))
		return TRUE;
	else
		return FALSE;
}


// fst

void fst_add_dir (__u32 *fst, int *fst_offset, char *st, int *st_offset, Dir *dir)
{
	DirItem *item = dir->items;
	Dir *sub;
	int parent_offset = *fst_offset - 1;


	while (item)
	{
		if (item->type == DIR_ITEM_DIRECTORY)
		{
			sub = item->sub;

			if (sub)
			{
				fst[*fst_offset*3 + 0] = BSWAP32 (*st_offset | 0x01000000);
				fst[*fst_offset*3 + 1] = BSWAP32 (parent_offset);
				fst[*fst_offset*3 + 2] = BSWAP32 (item->size);
				*fst_offset += 1;
			
				strcpy (&st[*st_offset], item->name);
				*st_offset += strlen (item->name) + 1;

				DEBUG (EVENT_LOG_DI, "FST OFFSET %.8x SIZE %.8x DIR  %s",
								parent_offset, item->size, item->name);
				fst_add_dir (fst, fst_offset, st, st_offset, sub);
			}
		}
		else
		{
			fst[*fst_offset*3 + 0] = BSWAP32 (*st_offset);
			fst[*fst_offset*3 + 1] = BSWAP32 (item->offset);
			fst[*fst_offset*3 + 2] = BSWAP32 (item->size);
			*fst_offset += 1;

			strcpy (&st[*st_offset], item->name);
			*st_offset += strlen (item->name) + 1;
			DEBUG (EVENT_LOG_DI, "FST OFFSET %.8x SIZE %.8x FILE %s",
								item->offset, item->size, item->name);
		}

		item = item->next;
	}
}


// calc fst_size and memory size of all items
// set offsets of the items
// return number of items
int fst_setup (Dir *dir, int *fst_size, int *size, int *nitems)
{
	DirItem *item = dir->items;


	while (item)
	{
		*fst_size += 3*4 + strlen (item->name) + 1;
		if (item->type == DIR_ITEM_DIRECTORY)
		{
			if (item->sub)
			{
				fst_setup (item->sub, fst_size, size, nitems);
				item->size = *nitems + 1;
			}
		}
		else
		{
			item->offset = *size + DATA_BASE_OFFSET;
			
			if (item->size & 0x7fff)
			{
				item->size += (0x8000 - (item->size & 0x7fff));
			}
			
			*size += item->size;
			// align to 32KB
			if (item->offset & 0x7fff)
			{
				item->offset += (0x8000 - (item->offset & 0x7fff));
				*size += (0x8000 - (item->offset & 0x7fff));
			}
		}
		
		item = item->next;
		if (item)
			*nitems += 1;
	}

	return *nitems;
}


__u32 *fst_from_dir (Dir *dir, unsigned int *fst_size)
{
	unsigned int size = 0, nitems = 0;
	__u32 *fst;
	int st_offset = 0, fst_offset = 1;


	*fst_size = 0;
	nitems = fst_setup (dir, fst_size, &size, &nitems) + 1;
	// root entry size
	*fst_size += 3*4;
	fst = calloc (1, *fst_size);

	// root
	fst[0] = BSWAP32 (0x01000000);
	fst[2] = BSWAP32 (nitems + 0);

	fst_add_dir (fst, &fst_offset, (char *) &fst[nitems*3], &st_offset, dir);

	return fst;
}


// virtual dvd

void vdvd_close (void)
{
	if (mdvd.vdvd_inserted)
	{
		free (mdvd.fst);
		free (mdvd.header);
		free (mdvd.filename);
		mdvd.vdvd_inserted = FALSE;
	}
	
	di_open_cover ();
}


int vdvd_open (char *path)
{
	Dir *dir;
	unsigned int fst_size;


	if (mdvd.vdvd_inserted)
		vdvd_close ();

	if ((dir = dir_create (path, NULL)))
	{
		mdvd.filename = strdup (path);
		mdvd.pos = 0;

		mdvd.root = dir;
		mdvd.header = calloc (1, DVD_HEADER_SIZE);
		mdvd.fst = (char *) fst_from_dir (dir, &fst_size);
		mdvd.fst_size = fst_size;

		*(__u32 *) &mdvd.header[DVD_FST_OFFSET] = BSWAP32 (DVD_HEADER_SIZE);
		*(__u32 *) &mdvd.header[DVD_FST_SIZE] = BSWAP32 (fst_size);
		*(__u32 *) &mdvd.header[DVD_FST_MAX_SIZE] = BSWAP32 (fst_size);
		// where to load fst
		MEM_WWORD (MEM_HEAP_TOP, round_up (MEM_RWORD (MEM_HEAP_TOP) - fst_size, 32));
		MEM_WWORD (MEM_FST, MEM_RWORD (MEM_HEAP_TOP));
		MEM_WWORD (MEM_FST_SIZE, fst_size);
		*(__u32 *) &mdvd.header[0x430] = BSWAP32 (MEM_RWORD (MEM_FST));
		
		// info
		strcpy (&mdvd.header[0], "VDVD");
		*(__u32 *) &mdvd.header[4] = BSWAP32 (0x30300000);
		strcpy (&mdvd.header[0x20], "Virtual DVD");
		
		
		mdvd.vdvd_inserted = TRUE;
		di_close_cover ();

		DEBUG (EVENT_LOG_DI, "..di: virtual dvd contains %d files",
						BSWAP32 (*(__u32 *) &mdvd.fst[8]));

		MEM_WWORD (MEM_BOOT_MAGIC, MEM_BOOT_MAGIC_JTAG);
		memcpy (MEM_ADDRESS (MEM_RWORD (MEM_FST)), mdvd.fst, fst_size);

		return TRUE;
	}
	else
		return FALSE;
}


unsigned int vdvd_read (void *dest, unsigned int size)
{
	if (mdvd.pos < DVD_HEADER_SIZE)
	{
		// size must be less than mdvd.pos + DVD_HEADER_SIZE

		memcpy (dest, &mdvd.header[mdvd.pos], size);
		mdvd.pos += size;
	}
	else if (mdvd.pos < DVD_HEADER_SIZE + mdvd.fst_size)
	{
		memcpy (dest, &mdvd.fst[mdvd.pos - DVD_HEADER_SIZE], size);
		mdvd.pos += size;
	}
	else
	{
		DirItem *p = NULL;
		Dir *parent;
		FILE *f;
		char buff[1024];
		__u32 offset;

		p = dir_find_item_by_offset (mdvd.root, mdvd.pos, &parent);

		if (p)
		{
			offset = mdvd.pos - p->offset;
			sprintf (buff, "%s/%s", parent->path, p->name);
			DEBUG (EVENT_LOG_DI, "....  vdvd | reading file %s (offset %x)", p->name, offset);

			f = fopen (buff, "r");
			if (!f)
				return 0;

			fseek (f, offset, SEEK_SET);
			fread (dest, 1, size, f);
			fclose (f);
		}
		else
		{
			memset (dest, 0, size);
			DEBUG (EVENT_STOP, "!!di: virtual reading gone too far");
		}
	}
	
	return size;
}


void mdvd_close (void)
{
	if (mdvd.vdvd_inserted)
		return vdvd_close ();

	if (mdvd.f)
	{
		file_close (mdvd.f);
		mdvd.f = NULL;
		free (mdvd.filename);

		dir_destroy (mdvd.root);
		mdvd.root = NULL;
	}
	
	di_open_cover ();
}


Dir *mdvd_load_directory (char *path)
{
	__u32 *buff;
	unsigned int size, max_size;
	Dir *root;
	DirItem *first;
	File *file;
	

	file = file_open (path);
	if (!file)
		return NULL;

	// load FST
	file_seek (file, DVD_FST_SIZE, SEEK_SET);
	size = BSWAP32 (file_read_w (file));
	max_size = BSWAP32 (file_read_w (file));

	file_seek (file, DVD_FST_OFFSET, SEEK_SET);
	file_seek (file, BSWAP32 (file_read_w (file)), SEEK_SET);
	buff = malloc (size);
	file_read (file, buff, size);
	file_close (file);

	root = calloc (1, sizeof (Dir));
	first = calloc (1, sizeof (DirItem));
	first->type = DIR_ITEM_DIRECTORY;
	first->name = strdup ("..");

	dir_add_item (root, first);
	dir_add_items (root, buff, (char *) &buff[BSWAP32 (buff[2])*3], 1, BSWAP32 (buff[2]));
//	mdvd.root = root;

	free (buff);
	
	return root;
}


unsigned int mdvd_read (void *dest, unsigned int size)
{
	DirItem *p = NULL;


	if (!mdvd.f && !mdvd.vdvd_inserted)
	{
		DEBUG (EVENT_EFATAL, "dvd read executed with no dvd in drive");
		return 0;
	}
	
	if (mdvd.vdvd_inserted)
		return vdvd_read (dest, size);

	if (mdvd.pos)
	{
		p = dir_find_item_by_offset (mdvd.root, mdvd.pos, NULL);

		if (p)
			DEBUG (EVENT_LOG_DI, "....  reading file %s", p->name);
	}

	mdvd.pos += size;
	return file_read (mdvd.f, dest, size);
}


unsigned int mdvd_read_fixed (void *dest, unsigned int size)
{
	DirItem *p = NULL;


	if (!mdvd.f && !mdvd.vdvd_inserted)
	{
		DEBUG (EVENT_EFATAL, "dvd read executed with no dvd in drive");
		return 0;
	}
	
	if (mdvd.vdvd_inserted)
		return vdvd_read (dest, size);

	if (mdvd.pos)
	{
		p = dir_find_item_by_offset_aligned (mdvd.root, mdvd.pos, NULL);

		if (p)
		{
			DEBUG (EVENT_LOG_DI, "....  reading file %s with fix %d", p->name, p->offset & 3);
			mdvd_seek_whence (p->offset & 3, SEEK_CUR);
		}
	}

	mdvd.pos += size;
	return file_read (mdvd.f, dest, size);
}


__u32 mdvd_read_w (void)
{
	__u32 w = 0;


	mdvd_read (&w, 4);
	return w;
}


// reads fst to mem
Dir *mdvd_read_fst (int modify_mem)
{
	__u32 *buff;
	unsigned int size, max_size;
	Dir *root;
	DirItem *first;


	// load FST
	mdvd_seek (DVD_FST_SIZE);
	size = BSWAP32 (mdvd_read_w ());
	max_size = BSWAP32 (mdvd_read_w ());

	mdvd_seek (DVD_FST_OFFSET);
	mdvd_seek (BSWAP32 (mdvd_read_w ()));

	if (modify_mem)
	{
		MEM_WWORD (MEM_HEAP_TOP, MEM_RWORD (MEM_HEAP_TOP) - max_size);
		MEM_WWORD (MEM_FST, MEM_RWORD (MEM_HEAP_TOP));
		MEM_WWORD (MEM_FST_SIZE, max_size);

		buff = (void *) MEM_ADDRESS (MEM_RWORD (MEM_FST));
	}
	else
		buff = malloc (size);

	mdvd_read (buff, size);
	mdvd_seek (0);

	root = calloc (1, sizeof (Dir));
	first = calloc (1, sizeof (DirItem));
	first->type = DIR_ITEM_DIRECTORY;
	first->name = strdup ("..");
	dir_add_item (root, first);
	dir_add_items (root, buff, (char *) &buff[BSWAP32 (buff[2])*3], 1, BSWAP32 (buff[2]));
	mdvd.root = root;
	
	if (!modify_mem)
		free (buff);

	return root;
}


int mdvd_open (char *filename)
{
	if (!file_exists (filename) || !is_gcm (filename))
		return FALSE;

	if (mdvd.f)
		mdvd_close ();

	mdvd.filename = strdup (filename);
	mdvd.f = file_open (filename);
	mdvd.pos = 0;

	mdvd.root = mdvd_read_fst (TRUE);

	di_close_cover ();

	return TRUE;
}


// don't modify memory or registers
// used after loading savestate
int mdvd_open_fast (char *filename)
{
	if (!file_exists (filename))
		return FALSE;

	if (mdvd.f)
		mdvd_close ();

	mdvd.filename = strdup (filename);
	mdvd.f = file_open (filename);
	mdvd.pos = 0;

	mdvd.root = mdvd_read_fst (FALSE);

	return TRUE;
}


int mdvd_inserted (void)
{
	if (mdvd.f || mdvd.vdvd_inserted)
		return TRUE;
	else
		return FALSE;
}


unsigned int mdvd_seek_whence (unsigned int offset, int whence)
{
	if (!mdvd.f && !mdvd.vdvd_inserted)
		return 0;

	if (mdvd.vdvd_inserted)
	{
		switch (whence)
		{
			case SEEK_SET:
				mdvd.pos = offset;
				break;
			
			case SEEK_CUR:
				mdvd.pos += offset;
				break;
		}
	}
	else
		mdvd.pos = file_seek (mdvd.f, offset, whence);
	
	return mdvd.pos;
}


unsigned int mdvd_seek (unsigned int offset)
{
	if (!mdvd.f && !mdvd.vdvd_inserted)
		return 0;
	
	mdvd.pos = offset;
	if (mdvd.vdvd_inserted)
		return offset;
	else
		return file_seek (mdvd.f, offset, SEEK_SET);
}


void boot_apploader (void)
{
	__u32 buff[0x20], bs2report;
	__u32 loader_prolog, loader_main, loader_epilog;
	__u32 msr = MSR;


	MSR &= ~MSR_EE;

	// BS2Report
	bs2report = BSWAP32 (hle_opcode ("BS2Report"));
	if (bs2report)
		MEM32 (0x81300000) = BSWAP32 (hle_opcode ("BS2Report"));
	else
		MEM32 (0x81300000) = BSWAP32 (0x4e800020);		// blr
	
	GPR[3] = 0x81300004;		// prolog
	GPR[4] = 0x81300008;		// main
	GPR[5] = 0x8130000c;		// epilog

	// disk info
	mdvd_seek (0);
	mdvd_read (MEM_ADDRESS (0), 32);

	// apploader
	mdvd_seek (0x2440);
	mdvd_read (buff, 32);
	// 5 - size
	mdvd_read (MEM_ADDRESS (0x81200000), BSWAP32 (buff[5]));

	// 4 - entrypoint
	PC = BSWAP32 (buff[4]);
	
	LR = 0;
	while (PC)
		cpu_execute ();

	loader_prolog = MEMR32 (0x81300004);
	loader_main   = MEMR32 (0x81300008);
	loader_epilog = MEMR32 (0x8130000c);

	GPR[3] = 0x81300000;

	LR = 0;
	PC = loader_prolog;
	// execute prolog
	while (PC)
		cpu_execute ();

	// execute main
	do
	{
		GPR[3] = 0x81300004;		// address
		GPR[4] = 0x81300008;		// size
		GPR[5] = 0x8130000c;		// disk offset

		LR = 0;
		PC = loader_main;
		while (PC)
			cpu_execute ();

		if (MEMR32 (0x81300008))
		{
			mdvd_seek (MEMR32 (0x8130000c));
			mdvd_read (MEM_ADDRESS (MEMR32 (0x81300004)), MEMR32 (0x81300008));
		}
	} while (GPR[3]);

	// execute epilog
	LR = 0;
	PC = loader_epilog;
	while (PC)
		cpu_execute ();

	MSR = msr;
	PC = GPR[3];
}


int mdvd_get_country_code (void)
{
	__u8 id;
	__u32 last_pos = mdvd.pos;


	mdvd_seek (3);
	mdvd_read (&id, 1);
	mdvd_seek (last_pos);

	return id;
}


void di_reinit (void)
{
}


void di_init (void)
{
	memset (rdi, 0, sizeof (rdi));

	di_open_cover ();

	// dicfg
	mem_hwr_hook (32, DI_SR, di_r32_direct);
	mem_hww_hook (32, DI_SR, di_w32_disr);

	mem_hwr_hook (32, DI_CVR, di_r32_direct);
	mem_hww_hook (32, DI_CVR, di_w32_cover);

	mem_hwr_hook (32, DI_CMDBUF0, di_r32_direct);
	mem_hww_hook (32, DI_CMDBUF0, di_w32_direct);
	mem_hwr_hook (32, DI_CMDBUF1, di_r32_direct);
	mem_hww_hook (32, DI_CMDBUF1, di_w32_direct);
	mem_hwr_hook (32, DI_CMDBUF2, di_r32_direct);
	mem_hww_hook (32, DI_CMDBUF2, di_w32_direct);

	mem_hwr_hook (32, DI_MAR, di_r32_direct);
	mem_hww_hook (32, DI_MAR, di_w32_direct);

	mem_hwr_hook (32, DI_LENGTH, di_r32_direct);
	mem_hww_hook (32, DI_LENGTH, di_w32_direct);

	mem_hwr_hook (32, DI_CR, di_r32_direct);
	mem_hww_hook (32, DI_CR, di_w32_dicr);

	mem_hwr_hook (32, DI_IMMBUF, di_r32_direct);
	mem_hww_hook (32, DI_IMMBUF, di_w32_direct);

	mem_hwr_hook (32, DI_CFG, di_r32_direct);
	mem_hww_hook (32, DI_CFG, di_w32_cfg);
}

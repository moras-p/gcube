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

#include "diskio.h"
#include "mem.h"


 // gzseek is buggy on windows, need to call gzrewind first
z_off_t my_gzseek(gzFile file, z_off_t offset, int whence)
{
	if (whence == SEEK_CUR)
		offset += gztell(file);

	gzrewind(file);
	return gzseek(file, offset, whence);
}
#ifdef WINDOWS
#define gzseek my_gzseek
#endif


int gzseekend (void *file)
{
	char buff[0x1000];
	int len = 0, t = 0x1000;


	while (t == sizeof (buff))
		len += (t = gzread (file, buff, sizeof (buff)));
	
	return len;
}


int is_imp (char *fname)
{
	IMPHeader gcph;
	FILE *f;


	f = fopen (fname, "rb");
	if (!f)
		return FALSE;

	fread (&gcph, 1, sizeof (IMPHeader), f);
	fclose (f);
	
	if (gcph.magic[0] == 0x7f &&
			gcph.magic[1] == 'I' &&
			gcph.magic[2] == 'M' &&
			gcph.magic[3] == 'P')
		return TRUE;
	else
		return FALSE;
}


void imp_close (IMPFile *file)
{
	IMPMark *a, *b;

	if (!file->f)
		return;

	fclose (file->f);

	a = file->marks;
	while (a)
	{
		b = a->next;
		free (a);
		a = b;
	}
	free (file);
}


IMPFile *imp_open (char *fname)
{
	IMPFile *file;
	IMPHeader gcph;
	IMPMark *m, *ml;
	unsigned int i;
	__u32 d;


	if (!is_imp (fname))
		return NULL;

	file = (IMPFile *) calloc (1, sizeof (IMPFile));
	file->f = fopen (fname, "rb");
	fread (&gcph, 1, sizeof (IMPHeader), file->f);

	// read items
	ml = m = file->current_mark = file->marks = (IMPMark *) calloc (1, sizeof (IMPMark));
	for (i = 0; i < BSWAP32 (gcph.nitems); i++)
	{
		ml = m;

		fread (&d, 1, 4, file->f);
		m->offset = BSWAP32 (d);

		fread (&d, 1, 4, file->f);
		m->length = BSWAP32 (d);

		m->next = (IMPMark *) calloc (1, sizeof (IMPMark));
		m = m->next;
	}

	// last offset (disk image size)
	m->offset = 1459978240;
	m->length = m->offset - ml->offset;
	file->zero_offset = ftell (file->f);
	file->compressed_offset = -1;

	return file;
}


int imp_seek (IMPFile *file, unsigned int offset, int whence)
{
	IMPMark *m = file->marks;
	unsigned int voffset;


	if (whence == SEEK_CUR)
		offset += file->pos;

	voffset = offset;
	while (offset > m->offset + m->length)
	{
		voffset -= m->length - 1;
		m = m->next;
	}
	
	if (offset > m->offset && offset <= m->offset + m->length)
	{
		voffset -= offset - m->offset;
		file->compressed_offset = offset - m->offset;
	}
	else
		file->compressed_offset = -1;

	fseek (file->f, voffset + file->zero_offset, SEEK_SET);

	if (file->compressed_offset > 0)
		fread (&file->data, 1, 1, file->f);

	file->pos = offset;
	file->current_mark = m;
	return offset;
}


int imp_tell (IMPFile *file)
{
	return file->pos;
}


int imp_read (IMPFile *file, char *buff, unsigned int size)
{
	int len, read_len, bcount = 0;


	while (size && file->current_mark->length)
	{
		if (file->compressed_offset == -1)
		{
			// read uncompressed data
			read_len = MIN (size, file->current_mark->offset - file->pos);
			len = fread (buff, 1, read_len, file->f);
			if (len != read_len)
			{
				file->pos += len;
				return (bcount + len);
			}
			
			size -= read_len;
			if (size)
				file->compressed_offset = 0;
		}
		else
		{
			// read compressed data
			if (file->compressed_offset == 0)
				fread (&file->data, 1, 1, file->f);
			
			read_len = MIN (size, file->current_mark->length - file->compressed_offset);
			memset (buff, file->data, read_len);
			size -= read_len;
			if (size)
			{
				file->current_mark = file->current_mark->next;
				file->compressed_offset = -1;
			}
			else
				file->compressed_offset += read_len;
		}
		
		buff += read_len;
		bcount += read_len;
		file->pos += read_len;
	}

	return bcount;
}

// read-only binary files
File *file_open (char *filename)
{
	File *file;


	if (!file_exists (filename))
		return NULL;


	file = (File *) calloc (1, sizeof (File));
	file->filename = strdup (filename);

	if (is_imp(filename))
	{
		file->f = imp_open(filename);
		file->type = FILE_IMP;
	}
	else
		file->f = gzopen (filename, "rb");

	return file;
}


void file_close (File *file)
{
	if (file->type == FILE_IMP)
		imp_close ((IMPFile *) file->f);
	else
		gzclose (file->f);

	free (file->filename);
	free (file);
}


int file_seek (File *file, int offset, int whence)
{
	if (file->type == FILE_IMP)
		return imp_seek ((IMPFile *) file->f, offset, whence);
	else
	{
		if (whence == SEEK_END)
			return gzseekend ((gzFile *) file->f);
		else
			return gzseek (file->f, offset, whence);
	}
}


int file_tell (File *file)
{
	if (file->type == FILE_IMP)
		return imp_tell ((IMPFile *) file->f);
	else
		return gztell (file->f);
}


int file_read(File *file, void *buff, unsigned int size)
{
	if (file->type == FILE_IMP)
		return imp_read((IMPFile *)file->f, (char *)buff, size);
	else
		return gzread(file->f, buff, size);
}


__u32 file_read_w (File *file)
{
	__u32 w = 0;


	file_read (file, &w, 4);
	return w;
}


__u32 load_dol (char *filename)
{
	File *dol;
	DOLHeader dh;
	int i;


	dol = file_open (filename);
	if (!dol)
	{
		DEBUG (EVENT_EFATAL, ".dol: couldn't open %s", filename);
		return GC_FILE_ERROR;
	}


	file_read (dol, &dh, sizeof (DOLHeader));
	
	for (i = 0; i < DOL_NTEXT; i++)
	{
		if (dh.text_offset[i])
		{
			file_seek (dol, BSWAP32 (dh.text_offset[i]), SEEK_SET);
			file_read (dol, MEM_ADDRESS (BSWAP32 (dh.text_address[i])), BSWAP32 (dh.text_size[i]));
			
			DEBUG (EVENT_INFO, ".dol: TEXT with size 0x%x loaded at 0x%x", BSWAP32 (dh.text_size[i]), BSWAP32 (dh.text_address[i]));
		}
	}
	
	for (i = 0; i < DOL_NDATA; i++)
	{
		if (dh.data_offset[i])
		{
			file_seek (dol, BSWAP32 (dh.data_offset[i]), SEEK_SET);
			file_read (dol, MEM_ADDRESS (BSWAP32 (dh.data_address[i])), BSWAP32 (dh.data_size[i]));

			DEBUG (EVENT_INFO, ".dol: DATA with size 0x%x loaded at 0x%x", BSWAP32 (dh.data_size[i]), BSWAP32 (dh.data_address[i]));
		}
	}
	
	file_close (dol);
	
	DEBUG (EVENT_INFO, ".dol: opened %s with entrypoint at 0x%x", filename, BSWAP32 (dh.entry_point));

	return BSWAP32 (dh.entry_point);
}


__u32 load_bin (char *filename)
{
	File *bin;
	int i;


	bin = file_open (filename);
	if (!bin)
	{
		DEBUG (EVENT_EFATAL, ".bin: couldn't open %s", filename);
		return GC_FILE_ERROR;
	}

	file_seek (bin, 0, SEEK_END);
	i = file_tell (bin);
	file_seek (bin, 0, SEEK_SET);
	file_read (bin, MEM_ADDRESS (0x80003100), i);

	DEBUG (EVENT_INFO, ".bin: file %s, loaded 0x%x bytes at 0x80003100", filename, i);

	file_close (bin);

	return 0x80003100;
}


int is_elf (char *filename)
{
	File *f;
	Elf32_Ehdr hdr;


	f = file_open (filename);
	if (!f)
		return FALSE;

	file_read (f, &hdr, sizeof (hdr));
	file_close (f);

	if ((hdr.e_ident[EI_MAG0] == 0x7f  &&
			 hdr.e_ident[EI_MAG1] == 'E'   &&
			 hdr.e_ident[EI_MAG2] == 'L'   &&
			 hdr.e_ident[EI_MAG3] == 'F')  &&
			 hdr.e_ident[EI_CLASS] == ELFCLASS32)
	{
		if ((BSWAP16 (hdr.e_type) != ET_EXEC) && (BSWAP16 (hdr.e_type) != ET_REL))
		{
			DEBUG (EVENT_EFATAL, ".elf: file %s is not executable (%d)", filename, BSWAP16 (hdr.e_type));
			return FALSE;
		}
		
		if (hdr.e_ident[EI_DATA] != ELFDATA2MSB)
		{
			DEBUG (EVENT_EFATAL, ".elf: only MSB encoding supported (%d)", hdr.e_ident[EI_DATA]);
			return FALSE;
		}

		return TRUE;
	}
	else
		return FALSE;
}


__u32 load_elf (char *filename)
{
	File *elf;
	Elf32_Ehdr hdr;
	Elf32_Phdr phdr;
	Elf32_Shdr shdr;
	int i;


	if (!is_elf (filename))
		return GC_FILE_ERROR;

	elf = file_open (filename);
	file_read (elf, &hdr, sizeof (hdr));

	hdr.e_phoff = BSWAP32 (hdr.e_phoff);
	hdr.e_phnum = BSWAP16 (hdr.e_phnum);

	for (i = 0; i < hdr.e_phnum; i++)
	{
		file_seek (elf, hdr.e_phoff + i*sizeof (phdr), SEEK_SET);
		file_read (elf, &phdr, sizeof (phdr));

		if (BSWAP32 (phdr.p_type) == PT_LOAD && phdr.p_filesz != 0)
		{
			file_seek (elf, BSWAP32 (phdr.p_offset), SEEK_SET);
			file_read (elf, MEM_ADDRESS (BSWAP32 (phdr.p_vaddr)), BSWAP32 (phdr.p_filesz));

			DEBUG (EVENT_INFO, ".elf: section with size 0x%x loaded at 0x%x", BSWAP32 (phdr.p_filesz), BSWAP32 (phdr.p_vaddr));
		}
	}

	if (BSWAP16 (hdr.e_type) == ET_REL)
	{
		hdr.e_shoff = BSWAP32 (hdr.e_shoff);
		hdr.e_shnum = BSWAP16 (hdr.e_shnum);

		for (i = 0; i < hdr.e_shnum; i++)
		{
			file_seek (elf, hdr.e_shoff + i*sizeof (shdr), SEEK_SET);
			file_read (elf, &shdr, sizeof (shdr));

			if ((BSWAP32 (shdr.sh_flags) & SHF_EXECINSTR) && (shdr.sh_size != 0))
			{
				file_seek (elf, BSWAP32 (shdr.sh_offset), SEEK_SET);
				file_read (elf, MEM_ADDRESS (BSWAP32 (shdr.sh_addr)), BSWAP32 (shdr.sh_size));

				DEBUG (EVENT_INFO, ".elf: section with size 0x%x loaded at 0x%x", BSWAP32 (shdr.sh_size), BSWAP32 (shdr.sh_addr));
			}
		}
	}

	file_close (elf);

	DEBUG (EVENT_INFO, ".elf: opened %s with entrypoint at 0x%x", filename, BSWAP32 (hdr.e_entry));

	return BSWAP32 (hdr.e_entry);
}


int is_gcs (char *filename)
{
	File *f;
	GCSHeader hdr;


	f = file_open (filename);
	if (!f)
		return FALSE;

	file_read (f, &hdr, sizeof (hdr));
	file_close (f);

	if ((hdr.magic[0] == 0x7f) &&
			(hdr.magic[1] == 'G')  &&
			(hdr.magic[2] == 'C')  &&
			(hdr.magic[3] == 'S'))
		return TRUE;
	else
		return FALSE;
}


void gcswrite (void *f, void *buff, unsigned int size, int compressed, int reverse)
{
	char *temp, *p = (char *) buff;
	unsigned int i;


#ifdef LIL_ENDIAN
	if (reverse)
	{
		temp = (char *) malloc (size);
		for (i = 0; i < size; i++)
			temp[i] = p[size - i - 1];
	}
	else
		temp = p;
#else
	temp = p;
#endif

	if (compressed)
		gzwrite (f, temp, size);
	else
		fwrite (temp, 1, size, (FILE *) f);

	if (reverse)
		free (temp);
}


void gcsread (void *f, void *buff, unsigned int size, int reverse)
{
	char *temp, *p = (char *) buff;
	unsigned int i;


#ifdef LIL_ENDIAN
	if (reverse)
	{
		temp = (char *) malloc (size);

		gzread (f, temp, size);
		for (i = 0; i < size; i++)
			p[i] = temp[size - i - 1];
		
		free (temp);
	}
	else
		gzread (f, buff, size);
#else
	gzread (f, buff, size);
#endif
}


void save_state (char *filename, int compressed)
{
	FILE *f;
	gzFile gf = NULL;
	void *gcs;
	GCSHeader hdr;


	f = fopen (filename, "wb");
	if (!f)
	{
		DEBUG (EVENT_EMINOR, "..io: error writing savestate");
		return;
	}

	// todo: header shouldn't be compressed
	if (compressed)
	{
		int pos = ftell (f);
		
		gf = gzdopen (fileno (f), "wb");
		gzseek (gf, pos, SEEK_SET);

		gcs = gf;
	}
	else
		gcs = f;


	memset (&hdr, 0, sizeof (hdr));

	hdr.magic[0] = 0x7f;
	hdr.magic[1] = 'G';
	hdr.magic[2] = 'C';
	hdr.magic[3] = 'S';

	if (compressed)
		hdr.flags |= GCS_COMPRESSED_GZ;

	if (mdvd.f)
		// no path (it's relative anyway)
		get_filename (hdr.filename, mdvd.filename);

	// texture caching mechanism writes markers to memory
	texcache_invalidate_all ();

	hdr.mem_size = MEM_SIZE;
	hdr.mem_offset = sizeof (hdr);
	hdr.l2c_size = L2C_SIZE;
	hdr.l2c_offset = hdr.mem_offset + hdr.mem_size;
	hdr.aram_size = ARAM_SIZE;
	hdr.aram_offset = hdr.l2c_offset + hdr.l2c_size;
	hdr.regs_size = 4*4096;
	hdr.regs_offset = hdr.aram_offset + hdr.aram_size;
	hdr.ps0regs_size = 8*32;
	hdr.ps0regs_offset = hdr.regs_offset + hdr.regs_size;
	hdr.ps1regs_size = 8*32;
	hdr.ps1regs_offset = hdr.ps0regs_offset + hdr.ps0regs_size;
	hdr.dspregs_size = RDSP_SIZE;
	hdr.dspregs_offset = hdr.ps1regs_offset + hdr.ps1regs_size;
	hdr.airegs_size = RAI_SIZE;
	hdr.airegs_offset = hdr.dspregs_offset + hdr.dspregs_size;
	hdr.cpregs_size = RCP_SIZE;
	hdr.cpregs_offset = hdr.airegs_offset + hdr.airegs_size;
	hdr.diregs_size = RDI_SIZE;
	hdr.diregs_offset = hdr.cpregs_offset + hdr.cpregs_size;
	hdr.eiregs_size = REXI_SIZE;
	hdr.eiregs_offset = hdr.diregs_offset + hdr.diregs_size;
	hdr.miregs_size = RMI_SIZE;
	hdr.miregs_offset = hdr.eiregs_offset + hdr.eiregs_size;
	hdr.peregs_size = RPE_SIZE;
	hdr.peregs_offset = hdr.miregs_offset + hdr.miregs_size;
	hdr.piregs_size = RPI_SIZE;
	hdr.piregs_offset = hdr.peregs_offset + hdr.peregs_size;
	hdr.siregs_size = RSI_SIZE;
	hdr.siregs_offset = hdr.piregs_offset + hdr.piregs_size;
	hdr.viregs_size = RVI_SIZE;
	hdr.viregs_offset = hdr.siregs_offset + hdr.siregs_size;
	hdr.gxstate_size = sizeof (GXState);
	hdr.gxstate_offset = hdr.viregs_offset + hdr.viregs_size;
	hdr.dspstate_size = sizeof (DSPState);
	hdr.dspstate_offset = hdr.gxstate_offset + hdr.gxstate_size;

	gcswrite (gcs, &hdr, sizeof (hdr), compressed, FALSE);

	// memory
	gcswrite (gcs, MEM_ADDRESS (0), hdr.mem_size, compressed, FALSE);
	// level 2 cache
	gcswrite (gcs, L2C_ADDRESS (0), hdr.l2c_size, compressed, FALSE);
	// auxiliary ram
	gcswrite (gcs, ARAM_ADDRESS (0), hdr.aram_size, compressed, FALSE);
	// cpu
	gcswrite (gcs, &CPUREGS (0), hdr.regs_size, compressed, FALSE);
	gcswrite (gcs, &FPUREGS_PS0 (0), hdr.ps0regs_size, compressed, FALSE);
	gcswrite (gcs, &FPUREGS_PS1 (0), hdr.ps1regs_size, compressed, FALSE);
	// hw
	gcswrite (gcs, rdsp, hdr.dspregs_size, compressed, TRUE);
	gcswrite (gcs, rai, hdr.airegs_size, compressed, TRUE);
	gcswrite (gcs, rcp, hdr.cpregs_size, compressed, FALSE);
	gcswrite (gcs, rdi, hdr.diregs_size, compressed, FALSE);
	gcswrite (gcs, rexi, hdr.eiregs_size, compressed, FALSE);
	gcswrite (gcs, rmi, hdr.miregs_size, compressed, FALSE);
	gcswrite (gcs, rpe, hdr.peregs_size, compressed, FALSE);
	gcswrite (gcs, rpi, hdr.piregs_size, compressed, FALSE);
	gcswrite (gcs, rsi, hdr.siregs_size, compressed, FALSE);
	gcswrite (gcs, rvi, hdr.viregs_size, compressed, TRUE);

	gcswrite (gcs, &gxs, hdr.gxstate_size, compressed, FALSE);
	gcswrite (gcs, &dspstate, hdr.dspstate_size, compressed, FALSE);

	if (compressed)
		gzclose (gf);
	else
		fclose (f);

	DEBUG (EVENT_INFO, "..io: %ssavestate written to file %s",
				 (compressed) ? "compressed " : "", filename);
}


__u32 load_state (char *filename)
{
	FILE *f;
	gzFile gf;
	GCSHeader hdr;


	if (!is_gcs (filename))
		return GC_FILE_ERROR;

	f = fopen (filename, "rb");
	gf = gzdopen (fileno (f), "rb");

	gcsread (gf, &hdr, sizeof (hdr), FALSE);
	
	// mem
	gzseek (gf, hdr.mem_offset, SEEK_SET);
	gcsread (gf, MEM_ADDRESS (0), hdr.mem_size, FALSE);
	// level 2 cache
	gzseek (gf, hdr.l2c_offset, SEEK_SET);
	gcsread (gf, L2C_ADDRESS (0), hdr.l2c_size, FALSE);
	// auxiliary ram
	gzseek (gf, hdr.aram_offset, SEEK_SET);
	gcsread (gf, ARAM_ADDRESS (0), hdr.aram_size, FALSE);
	// cpu
	gzseek (gf, hdr.regs_offset, SEEK_SET);
	gcsread (gf, &CPUREGS (0), hdr.regs_size, FALSE);
	gzseek (gf, hdr.ps0regs_offset, SEEK_SET);
	gcsread (gf, &FPUREGS_PS0 (0), hdr.ps0regs_size, FALSE);
	gzseek (gf, hdr.ps1regs_offset, SEEK_SET);
	gcsread (gf, &FPUREGS_PS1 (0), hdr.ps1regs_size, FALSE);
	// hw
	gzseek (gf, hdr.dspregs_offset, SEEK_SET);
	gcsread (gf, rdsp, hdr.dspregs_size, TRUE);
	gzseek (gf, hdr.airegs_offset, SEEK_SET);
	gcsread (gf, rai, hdr.airegs_size, TRUE);
	gzseek (gf, hdr.cpregs_offset, SEEK_SET);
	gcsread (gf, rcp, hdr.cpregs_size, FALSE);
	gzseek (gf, hdr.diregs_offset, SEEK_SET);
	gcsread (gf, rdi, hdr.diregs_size, FALSE);
	gzseek (gf, hdr.eiregs_offset, SEEK_SET);
	gcsread (gf, rexi, hdr.eiregs_size, FALSE);
	gzseek (gf, hdr.miregs_offset, SEEK_SET);
	gcsread (gf, rmi, hdr.miregs_size, FALSE);
	gzseek (gf, hdr.peregs_offset, SEEK_SET);
	gcsread (gf, rpe, hdr.peregs_size, FALSE);
	gzseek (gf, hdr.piregs_offset, SEEK_SET);
	gcsread (gf, rpi, hdr.piregs_size, FALSE);
	gzseek (gf, hdr.siregs_offset, SEEK_SET);
	gcsread (gf, rsi, hdr.siregs_size, FALSE);
	gzseek (gf, hdr.viregs_offset, SEEK_SET);
	gcsread (gf, rvi, hdr.viregs_size, TRUE);

	gzseek (gf, hdr.gxstate_offset, SEEK_SET);
	gcsread (gf, &gxs, hdr.gxstate_size, FALSE);
	gzseek (gf, hdr.dspstate_offset, SEEK_SET);
	gcsread (gf, &dspstate, hdr.dspstate_size, FALSE);
	

	gzclose (gf);

	if (*hdr.filename)
	{
		char buff[1024];
		
		get_path (buff, filename);
		strcat (buff, hdr.filename);
		mdvd_open_fast (hdr.filename);
	}

	DEBUG (EVENT_INFO, "..io: savestate %s loaded", filename);
	return PC;
}


__u32 load_gc (char *filename)
{
	char ext[64];
	
	
	get_extension (ext, filename);
	if (0 == strcasecmp (ext, "bin"))
		return load_bin (filename);
	else if (0 == strcasecmp (ext, "dol"))
		return load_dol (filename);
	else if (is_elf (filename))
		return load_elf (filename);
	else if (is_gcs (filename))
		return load_state (filename);
	else
	{
		DEBUG (EVENT_EFATAL, "..io: unknown type of file %s", filename);
		return GC_FILE_ERROR;
	}
}

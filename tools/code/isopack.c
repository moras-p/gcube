/*
 *  isopack
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
 *  simple compression utility for cd/dvd images
 *  - search for a block of repeating bytes (at least MIN_LENGTH bytes)
 *  - leave just one byte and write offset and size of the block in the header
 *
 */

#include <stdio.h>
#include <errno.h>

#include "general.h"


#define BUFF_SIZE						0xffff
int MIN_LENGTH = 1024;


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
	__u32 nmarks;
	__u8 reserved[4];
} IMPHeader;


void error (char *str)
{
	perror (str);
	exit (1);
}


void destroy_marks (IMPMark *marks)
{
	IMPMark *next;


	while (marks)
	{
		next = marks->next;
		free (marks);
		marks = next;
	}
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


int imp_decompress (char *infname, char *outfname)
{
	FILE *in, *out;
	IMPHeader gcph;
	IMPMark *m, *first;
	unsigned char buff[BUFF_SIZE];
	unsigned int i, offset = 0, n;
	__u32 d;


	in = fopen (infname, "rb");
	out = fopen (outfname, "wb");

	if (!in || !out)
	{
		if (in)
			fclose (in);
			
		printf ("error decompressing file %s to %s\n", infname, outfname);
		return FALSE;
	}

	fread (&gcph, 1, sizeof (IMPHeader), in);
	
	printf ("decompressing file %s to %s\n", infname, outfname);

	// read marks
	m = first = calloc (1, sizeof (IMPMark));
	for (i = 0; i < BSWAP_32 (gcph.nmarks); i++)
	{
		fread (&d, 1, 4, in);
		m->offset = BSWAP_32 (d) + 1;

		fread (&d, 1, 4, in);
		m->length = BSWAP_32 (d);
		
		m->next = calloc (1, sizeof (IMPMark));
		m = m->next;
	}
	
	// decompress
	m = first;
	while (m->length)
	{
		printf ("offset %.8x length %.8x\n", m->offset, m->length);

		while (m->offset - offset > BUFF_SIZE)
		{
			offset += fread (buff, 1, BUFF_SIZE, in);
			if (BUFF_SIZE != fwrite (buff, 1, BUFF_SIZE, out))
				error ("Error occured while writing file");
		}

		n = fread (buff, 1, m->offset - offset, in);
		if (m->offset - offset != fwrite (buff, 1, m->offset - offset, out))
			error ("Error occured while writing file");
		offset += n;

		fread (&d, 1, 1, in);
		memset (buff, d, BUFF_SIZE);
		while (m->length > BUFF_SIZE)
		{
			if (BUFF_SIZE != fwrite (buff, 1, BUFF_SIZE, out))
				error ("Error occured while writing file");
			m->length -= BUFF_SIZE;
			offset += BUFF_SIZE;
		}

		for (i = 0; i < m->length; i++)
			if (1 != fwrite (&d, 1, 1, out))
				error ("Error occured while writing file");
		offset += m->length;
		
		m = m->next;
	}

	n = BUFF_SIZE;
	while (n == BUFF_SIZE)
	{
		n = fread (buff, 1, BUFF_SIZE, in);
		if (n != fwrite (buff, 1, n, out))
			error ("Error occured while writing file");
	}

	destroy_marks (first);

	fclose (out);
	fclose (in);
	
	return TRUE;
}


int get_compression_data (FILE *file, IMPMark **data)
{
	unsigned char buff[BUFF_SIZE], b = 0;
	unsigned int i, k = 0, offset = 0, nmarks = 0, n = BUFF_SIZE;
	IMPMark *first, *m;


	m = first = calloc (1, sizeof (IMPMark));
	while (n == BUFF_SIZE)
	{
		n = fread (buff, 1, BUFF_SIZE, file);

		i = 0;
		while (i < n)
		{
			if (b == buff[i])
				k++;
			else
			{
				if (k >= MIN_LENGTH)
				{
					m->offset = offset;
					m->length = k;
					m->next = calloc (1, sizeof (IMPMark));
					m = m->next;
					nmarks++;
				}
			
				b = buff[i];
				offset += k + 1;
				k = 0;
			}
			i++;
		}
	}

	if (k >= MIN_LENGTH)
	{
		m->offset = offset;
		m->length = k;
		m->next = calloc (1, sizeof (IMPMark));
		m = m->next;
		nmarks++;
	}

	*data = first;
	return nmarks;
}


void imp_compress (char *infname, char *outfname)
{
	unsigned int n, nmarks, offset = 0;
	char buff[BUFF_SIZE];
	FILE *in, *out;
	IMPMark *first, *m;
	IMPHeader gcph;
	__u32 d;


	in = fopen (infname, "rb");
	if (!in)
	{
		printf ("error opening file %s\n", infname);
		return;
	}
	printf ("compressing file %s, min_length = %d\n", infname, MIN_LENGTH);

	out = fopen (outfname, "wb");
	if (!out)
	{
		fclose (in);
		printf ("error writing to file %s\n", outfname);
		return;
	}
	printf ("writing file %s\n", outfname);

	nmarks = get_compression_data (in, &first);


	memset (&gcph, 0, sizeof (IMPHeader));
	gcph.magic[0] = (char) 0x7f;
	gcph.magic[1] = 'I';
	gcph.magic[2] = 'M';
	gcph.magic[3] = 'P';
	gcph.nmarks = BSWAP_32 (nmarks);
	
	// write header
	if (sizeof (IMPHeader) != fwrite (&gcph, 1, sizeof (IMPHeader), out))
		error ("Error occured while writing file");
	m = first;
	while (m->next)
	{
		d = BSWAP_32 (m->offset - 1);
		if (4 != fwrite (&d, 1, 4, out))
			error ("Error occured while writing file");

		d = BSWAP_32 (m->length);
		if (4 != fwrite (&d, 1, 4, out))
			error ("Error occured while writing file");
		
		m = m->next;
	}

	// write compressed file contents
	m = first;
	rewind (in);
	while (m->length)
	{
		printf ("offset %.8x length %.8x\n", m->offset, m->length);
		while (m->offset - offset > BUFF_SIZE)
		{
			offset += fread (buff, 1, BUFF_SIZE, in);
			if (BUFF_SIZE != fwrite (buff, 1, BUFF_SIZE, out))
				error ("Error occured while writing file");
		}

		n = fread (buff, 1, m->offset - offset + 1, in);
		if (m->offset - offset + 1 != fwrite (buff, 1, m->offset - offset + 1, out))
			error ("Error occured while writing file");
		offset += n;
		
		fseek (in, m->length - 1, SEEK_CUR);
		offset += m->length - 1;
		
		m = m->next;
	}

	n = BUFF_SIZE;
	while (n == BUFF_SIZE)
	{
		n = fread (buff, 1, BUFF_SIZE, in);
		if (n != fwrite (buff, 1, n, out))
			error ("Error occured while writing file");
	}

	destroy_marks (first);

	fclose (out);
	fclose (in);
}



int main (int argc, char **argv)
{
	char temp[256];
	int decompress, shift = 0, d;


	if (argc > 1)
	{
		if (1 == sscanf (argv[1], "%d", &d))
		{
			MIN_LENGTH = d;
			shift = 1;
		}
		else
			shift = 0;
	}

	if (argc < 2 + shift)
	{
		printf ("simple iso compression utility\n");
		printf ("usage: isopack [MIN_LENGTH] infile [outfile]\n");
		printf ("if the infile is compressed, it will be decompressed\n");
		printf ("MIN_LENGTH defaults to 1024\n");
		return TRUE;
	}

	if (!file_exists (argv[1 + shift]))
	{
		printf ("couldn't open file %s\n", argv[1 + shift]);
		perror ("");
		return TRUE;
	}

	decompress = is_imp (argv[1 + shift]);

	if (argc < 3 + shift)
	{
		strcpy (temp, argv[1 + shift]);
		kill_extension (temp);
		if (decompress)
			strcat (temp, ".iso");
		else
			strcat (temp, ".imp");
	}
	else
		strcpy (temp, argv[2 + shift]);

	if (decompress)
		imp_decompress (argv[1 + shift], temp);
	else
		imp_compress (argv[1 + shift], temp);

	return FALSE;
}

/*
 *  bin2dol
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

#include <stdio.h>
#include <errno.h>

#include "general.h"
#include "diskio.h"


int main (int argc, char **argv)
{
	FILE *f;
	char *buff, temp[256];
	DOLHeader dh;
	int size;


	if (argc < 2)
	{
		printf ("usage: bin2dol infile.bin [outfile.dol]\n");
		return TRUE;
	}

	f = fopen (argv[1], "rb");
	if (!f)
	{
		printf ("couldn't open file %s\n", argv[1]);
		perror ("");
		return TRUE;
	}

	fseek (f, 0, SEEK_END);
	size = ftell (f);
	fseek (f, 0, SEEK_SET);
	buff = malloc (size);
	fread (buff, 1, size, f);
	fclose (f);

	if (argc < 3)
	{
		strcpy (temp, argv[1]);
		kill_extension (temp);
		strcat (temp, ".dol");
	}
	else
		strcpy (temp, argv[2]);

	f = fopen (temp, "wb");

	memset (&dh, 0, sizeof (DOLHeader));
	fwrite (&dh, 1, sizeof (DOLHeader), f);

	dh.text_offset[0] = BSWAP32 (ftell (f));
	dh.text_address[0] = BSWAP32 (0x80003100);
	dh.text_size[0] = BSWAP32 (size);
	dh.entry_point = BSWAP32 (0x80003100);

	fwrite (buff, 1, size, f);
	
	fseek (f, 0, SEEK_SET);
	fwrite (&dh, 1, sizeof (DOLHeader), f);

	fclose (f);
	free (buff);

	return FALSE;
}

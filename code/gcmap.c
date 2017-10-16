/*
 *  gcmap
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
 *  uses readelf output to construct map file
 *      
 *         
 */

#include <stdio.h>
#include <errno.h>

#include "general.h"


int main (int argc, char **argv)
{
	FILE *f, *out;
	__u32 address, size;
	char s[256];
	

	if (argc < 2)
	{
		printf ("usage: gcmap infile.elf [outfile.map]\n");
		return TRUE;
	}

	sprintf (s, "readelf -Ws %s", argv[1]);
	f = popen (s, "r");
	if (!f)
	{
		perror ("error:");
		return TRUE;
	}

	if (argc < 3)
		out = stdout;
	else
		out = fopen (argv[2], "w");
	
	while (!feof (f))
	{
		if (3 == fscanf (f, " %*d: %x %d FUNC %*s %*s %*d %s\n", &address, &size, s))
			fprintf (out, "%.8x %.8x %s\n", address | 0x80000000, size, s);
		else
			fscanf (f, "%*[^\n]\n");
	}

	if (out != stdout)
		fclose (out);
	fclose (f);

	return FALSE;
}

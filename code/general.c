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
 *  utility functions
 *      
 *         
 */

#include "general.h"


char home_dir[1024] = {0};


unsigned int file_exists (char *filename)
{
	FILE *f;


	f = fopen (filename, "r");
	if (!f)
		return FALSE;
	else
	{
		fclose (f);
		return TRUE;
	}
}


unsigned int file_size (char *filename)
{
	int size;
	FILE *f;


	f = fopen (filename, "r");
	if (!f)
		return 0;

	fseek (f, 0, SEEK_END);
	size = ftell (f);
	fclose (f);

	return size;
}


FILE *flog = NULL;

void log_open (char *filename)
{
	if (!flog)
		flog = fopen (filename, "w+");
}


void log_printf (char *fmt, ...)
{
	va_list ap;


	va_start (ap, fmt);
	vfprintf (flog, fmt, ap);
	va_end (ap);
}


void log_close (void)
{
	printf (".log: close\n");
	if (flog)
	{
		fclose (flog);
		flog = NULL;
	}
}


char *get_filename (char *name, const char *path)
{
	char *c;


	c = strrchr (path, '/');
	if (c)
		strcpy (name, c + 1);
	else
	{
		c = strrchr (path, '\\');
		if (c)
			strcpy (name, c + 1);
		else
			// path is filename
			strcpy (name, path);
	}

	return name;
}


char *get_path (char *path, const char *name)
{
	char *c;


	c = strrchr (name, '/');
	if (c)
	{
		strcpy (path, name);
		path[strlen (name) - strlen (c) + 1] = '\0';
	}
	else
	{
		c = strrchr (name, '\\');
		if (c)
		{
			strcpy (path, name);
			path[strlen (name) - strlen (c) + 1] = '\0';
		}
		else
			*path = '\0';
	}

	return path;
}


char *get_extension (char *ext, const char *filename)
{
	char str[256], *c;


	get_filename (str, filename);

	c = strrchr (str, '.');
	if (c)
		strcpy (ext, c + 1);
	else
		*ext = '\0';
	
	return ext;
}


char *kill_extension (char *filename)
{
	char *c;


	c = strrchr (filename, '.');
	if (c)
		*c = '\0';
	
	return filename;
}


char *get_home_dir (void)
{
	return home_dir;
}


__u32 round_up (__u32 a, __u32 b)
{
	if (a & (b - 1))
		a += b - (a & (b - 1));

	return a;
}


inline int is_power_of_two (__u32 a)
{
	int i = 0;


	while (a)
	{
		i += a & 1;
		a >>= 1;
	}
	
	if (i > 1)
		return FALSE;
	else
		return TRUE;
}


int closest_upper_power_of_two (__u32 a)
{
	int i = 1;


	while (i < a)
		i <<= 1;
	
	return i;
}


__u32 crc_setup (unsigned int bits)
{
	__u32 crc = 0, i;


	for (i = 0; i < bits; i++)
		crc |= 1 << i;

	return crc;
}


__u32 crc_iterate (__u32 crc, __u8 d)
{
	int j;


	crc ^= d;

	for (j = 0; j < 8; j++)
	{
		if (crc & 1)
		{
			crc >>= 1;
			crc ^= CRC_POLY;
		}
		else
			crc >>= 1;
	}
	
	return crc;
}


void create_dir (char *name)
{
	char buff[1024];


	sprintf (buff, "%s/%s", home_dir, name);
#ifdef WINDOWS
	mkdir (buff);
#else
	mkdir (buff, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
#endif
}


void create_dir_tree (char *parent)
{
	char *str = getenv ("HOME");
	// first try HOME (linux)
	// second try HOMEDRIVE + HOMEPATH (windows)s
	// otherwise try current working directory
	if (str)
	{
		// use home directory
		strcpy (home_dir, str);

		create_dir (parent);
		strcat (home_dir, "/");
		strcat (home_dir, parent);
	}
	else
	{
		// use current directory
		str = getcwd (NULL, 0);
		if (str)
		{
			strcpy (home_dir, str);
			free (str);
		}
		else
			// nothing will be saved
			return;
	}

	create_dir ("maps");
	create_dir ("screenshots");
	create_dir ("videos");
}


int path_writeable (char *path)
{
	FILE *f;
	char buff[1024];


	sprintf (buff, "%s/.tmpwrite", path);
	f = fopen (buff, "w+");
	if (!f)
		return FALSE;
	
	fclose (f);
	remove (buff);
	return TRUE;
}

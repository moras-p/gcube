/*
 * scandir() for win32
 * this tool should make life easier for people writing for both unix
and wintel
 * written by Tom Torfs, 2002/10/31
 * donated to the public domain; use this code for anything you like
as long as
 * it is understood there are absolutely *NO* warranties of any kind,
even implied
 */

#include "scandir.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int scandir (const char *dirname,
			 struct mydirent ***namelist, int (*select) (const struct mydirent *), int (*compar) (const struct mydirent **, const struct mydirent **))
{
	WIN32_FIND_DATA wfd;
	HANDLE hf;
	struct mydirent **plist, **newlist;
	struct mydirent d;
	int numentries = 0;
	int allocentries = 255;
	int i;
	char path[FILENAME_MAX];

	i = strlen (dirname);

	if (i > sizeof path - 5)
		return -1;

	strcpy (path, dirname);
	if (i > 0 && dirname[i - 1] != '\\' && dirname[i - 1] != '/')
		strcat (path, "\\");
	strcat (path, "*.*");

	hf = FindFirstFile (path, &wfd);
	if (hf == INVALID_HANDLE_VALUE)
		return -1;

	plist = malloc (sizeof *plist * allocentries);
	if (plist == NULL)
	{
		FindClose (hf);
		return -1;
	}

	do
	{
		if (numentries == allocentries)
		{
			allocentries *= 2;
			newlist = realloc (plist, sizeof *plist * allocentries);
			if (newlist == NULL)
			{
				for (i = 0; i < numentries; i++)
					free (plist[i]);
				free (plist);
				FindClose (hf);
				return -1;
			}
			plist = newlist;
		}

		strncpy (d.d_name, wfd.cFileName, sizeof d.d_name);
		d.d_ino = 0;
		d.d_namlen = strlen (wfd.cFileName);
		d.d_reclen = sizeof d;

		if (select == NULL || select (&d))
		{
			plist[numentries] = malloc (sizeof d);
			if (plist[numentries] == NULL)
			{
				for (i = 0; i < numentries; i++)
					free (plist[i]);
				free (plist);
				FindClose (hf);
				return -1;
			};
			memcpy (plist[numentries], &d, sizeof d);
			numentries++;
		}
	}
	while (FindNextFile (hf, &wfd));

	FindClose (hf);

	if (numentries == 0)
	{
		free (plist);
		*namelist = NULL;
	}
	else
	{
		newlist = realloc (plist, sizeof *plist * numentries);
		if (newlist != NULL)
			plist = newlist;

		if (compar != NULL)
			qsort (plist, numentries, sizeof *plist,
            (int (*)(const void *, const void *)) compar);

		*namelist = plist;
	}

	return numentries;
}

int alphasort (const struct mydirent **d1, const struct mydirent **d2)
{

	return strcasecmp ((*d1)->d_name, (*d2)->d_name);
}

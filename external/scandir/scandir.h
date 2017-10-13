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

#include <stdio.h>
#include <dirent.h>

#define MAXNAMLEN FILENAME_MAX

// directory entry structure
/*
struct mydirent
{
	char d_name[MAXNAMLEN + 1];	// name of directory entry (0 terminated) 
	unsigned long d_ino;		// file serial number -- will be 0 for win32 
	short d_namlen;				// length of string in d_name
	short d_reclen;				// length of this record
};
*/

#define mydirent dirent

/* the scandir() function */
int scandir (const char *dirname, struct mydirent ***namelist,
			 int (*select) (const struct mydirent *), int (*compar) (const struct mydirent **, const struct mydirent **));

/* compare function for scandir() for alphabetic sort
   (case-insensitive on Win32) */
int alphasort (const struct mydirent **d1, const struct mydirent **d2);

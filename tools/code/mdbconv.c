
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main (int argc, char **argv)
{
	FILE *f;
	unsigned int size, crc;
	//char *name;
	char name[1024];
	

	if (argc < 2)
	{
		printf ("mdbconv mapdbfile\n");
		return 0;
	}	

	f = fopen (argv[1], "r");

	if (f)
	{
		printf ("\n#include \"mapdb.h\"\n");
		printf ("\nMapDBItem mapdb[] = \n{\n");
		while (!feof (f))
		{
			if (3 == fscanf (f, "%x %x %s\n", &size, &crc, name))
				printf ("\tMDBI (0x%x, 0x%x, \"%s\"),\n", size, crc, name);
		}
		printf ("\tMDBI (0, 0, NULL),\n");
		printf ("};\n");

		fclose (f);
	}

	return 0;
}

#ifndef __MAPDB_H
#define __MAPDB_H 1


#include "general.h"

typedef struct
{
	__u32 size, crc;
	const char *name;
} MapDBItem;

#define MDBI(s,c,n)			{s, c, n}


#endif // __MAPDB_H

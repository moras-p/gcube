#ifndef __DSPTYPES_H
#define __DSPTYPES_H 1

typedef unsigned char	uint8;
typedef unsigned short	uint16;
typedef unsigned int	uint32;
typedef unsigned long long uint64;
//typedef unsigned int	uint;

typedef signed char	sint8;
typedef signed short	sint16;
typedef signed int	sint32;
typedef signed long long sint64;

typedef const uint32 cuint32;

#define false 0
#define true !false

#ifndef __cplusplus
typedef unsigned int bool;
#endif

#endif

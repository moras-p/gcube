#ifndef __GENERAL_H
#define __GENERAL_H 1


#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

#include "types.h"

#ifndef FALSE
# define FALSE 0
#endif

#ifndef TRUE
# define TRUE !FALSE
#endif

#include <SDL/SDL_byteorder.h>
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
# define LIL_ENDIAN 1
#endif

#ifdef ASM_X86
# include "tools_x86.h"
#else
# include "tools_c.h"
#endif

#ifdef NEED_SCANDIR
# include "scandir.h"
#endif

#define MAX_S32			((__s32) 0x7fffffff)
#define MIN_S32			((__s32) 0x80000000)

#define MAX_S16			((__s16) 0x7fff)
#define MIN_S16			((__s16) 0x8000)

#define MAX_S8			((__s8)  0x7f)
#define MIN_S8			((__s8)  0x80)


#define MIN(a,b)					(((a) < (b)) ? (a) : (b))
#define CLAMP(X,min,max)	(((X) > max) ? max : (((X) < min) ? min : (X)))
#define EPSI							0.0001
#define ABS(X)						(((X) > 0) ? (X) : -(X))
#define FLOAT_EQ(a,b)			(ABS ((a) - (b)) < EPSI)

#define CRC_POLY		0xA0000001


static inline int CALC_OVERFLOW (__s32 a, __s32 b)
{
	return ((a > 0) && (b > 0) && (INT_MAX - a < b)) || ((a < 0) && (b < 0) && (-INT_MAX - a > b));
}

#define CALC_OVERFLOW3(a,b,c) 		(CALC_OVERFLOW (a,b) || CALC_OVERFLOW (a+b,c))


static inline int CALC_UNDERFLOW (__s32 a, __s32 b)
{
	return ((a > 0) && (b < 0) && (INT_MAX + b < a)) || ((a < 0) && (b > 0) && (-INT_MAX + b > a));
}

#define CALC_UNDERFLOW3(a,b,c)		(CALC_UNDERFLOW (a,b) || CALC_UNDERFLOW (a+b,c))


unsigned int file_exists (char *filename);
unsigned int file_size (char *filename);

void log_open (char *filename);
void log_printf (char *fmt, ...);
void log_close (void);

char *get_filename (char *name, const char *path);
char *get_path (char *path, const char *name);
char *get_extension (char *ext, const char *filename);
char *kill_extension (char *filename);

char *get_home_dir (void);

__u32 round_up (__u32 a, __u32 b);
inline int is_power_of_two (__u32 a);
int closest_upper_power_of_two (__u32 a);

__u32 crc_setup (unsigned int bits);
__u32 crc_iterate (__u32 crc, __u8 d);

void create_dir (char *name);
void create_dir_tree (char *parent);

int path_writeable (char *path);


#endif // __GENERAL_H

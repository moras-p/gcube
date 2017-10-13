#ifndef __TOOLS_C_H
#define __TOOLS_C_H 1



#if USE_BYTESWAP_H
# include <byteswap.h>
# define BSWAP_16(B) 	(bswap_16 (B))
# define BSWAP_32(B)	(bswap_32 (B))
# define BSWAP_64(B)	(bswap_64 (B))
#else
# define BSWAP_16(B)\
	({\
			__u16 xB = (B);\
			(((__u16) ((xB) << 8) | ((xB) >> 8)));\
	})
# define BSWAP_32(B)\
	({\
			__u32 xB = (B);\
			(((__u32) (xB << 24) | ((xB << 8) & 0xff0000) | ((xB >> 8) & 0xff00) | (xB >> 24)));\
	})
# define BSWAP_64(B)\
	({\
			__u64 xB = (B);\
			__u32 *xB32 = (__u32 *) &xB;\
			(((__u64) BSWAP32 (xB32[0])) << 32) | BSWAP32 (xB32[1]);\
	})
#endif


#define ROTL(X,n)\
	({\
			__u32 xX = (X);\
			__u32 xn = (n);\
			((xX << xn) | (xX >> (32 - xn)));\
	})


#define CARRY3(a,b,c)				(((a) + (b) < a) || ((a) + (b) + (c) < c))


static inline int BITSCAN (__u32 a)
{
	int n = 0;


	for (n = 0; n < 32; n++, a <<= 1)
		if (a & (1 << 31))
			break;

	return n;
}


#endif // __TOOLS_C_H


#include "hle.h"

// for simplification
#define RF(X)										(BSWAPF ((float)(X)))
#define RF_1										(RF (1.0f))
#define RF_M1										(RF (-1.0f))
#define VRF(X)									{ RF (X[0]), RF (X[1]), RF (X[2]) }
#define QRF(X)									{ RF (X[0]), RF (X[1]), RF (X[2]), RF (X[3]) }
#define MRF(X)\
	{\
	 	RF (X[0]), RF (X[1]), RF (X[ 2]), RF (X[ 3]),\
	 	RF (X[4]), RF (X[5]), RF (X[ 6]), RF (X[ 7]),\
	 	RF (X[8]), RF (X[9]), RF (X[10]), RF (X[11]),\
	}
#define M44RF(X)\
	{\
	 	RF (X[ 0]), RF (X[ 1]), RF (X[ 2]), RF (X[ 3]),\
	 	RF (X[ 4]), RF (X[ 5]), RF (X[ 6]), RF (X[ 7]),\
	 	RF (X[ 8]), RF (X[ 9]), RF (X[10]), RF (X[11]),\
	 	RF (X[12]), RF (X[13]), RF (X[14]), RF (X[15]),\
	}

// standard matrices are 3 rows / 4 columns
// _rnr - don't reverse the result
// _inr - don't reverse the inputs

#define MTXDegToRad(d)					((d) * 0.01745329252f)

inline float det_3x3 (float *m)
{
	return	 RF (m[1])*RF (m[6])*RF (m[ 8])
				 - RF (m[2])*RF (m[5])*RF (m[ 8])
				 + RF (m[2])*RF (m[4])*RF (m[ 9])
				 - RF (m[0])*RF (m[6])*RF (m[ 9])
				 + RF (m[0])*RF (m[5])*RF (m[10])
				 - RF (m[1])*RF (m[4])*RF (m[10]);
}


inline void MTXCopy (void *src, void *dst)
{
	memcpy (dst, src, 4 * 12);
}


inline void MTX44Copy (void *src, void *dst)
{
	memcpy (dst, src, 4 * 16);
}


inline void MTXRotTrig (float *m, char axis, float sa, float ca)
{
	switch (axis)
	{
		case 'x':
		case 'X':
			m[ 0] = RF_1;
			m[ 1] = 0;
			m[ 2] = 0;
			m[ 3] = 0;
				
			m[ 4] = 0;
			m[ 5] = RF ( ca);
			m[ 6] = RF (-sa);
			m[ 7] = 0;
				
			m[ 8] = 0;
			m[ 9] = RF (sa);
			m[10] = RF (ca);
			m[11] = 0;
			break;

		case 'y':                       
		case 'Y':                       
			m[ 0] = RF (ca);
			m[ 1] = 0;
			m[ 2] = RF (sa);
			m[ 3] = 0;

			m[ 4] = 0;
			m[ 5] = RF_1;
			m[ 6] = 0;
			m[ 7] = 0;

			m[ 8] = RF (-sa);
			m[ 9] = 0;
			m[10] = RF ( ca);
			m[11] = 0;
			break;

    case 'z':
    case 'Z':
			m[ 0] = RF ( ca);
			m[ 1] = RF (-sa);
			m[ 2] = 0;
			m[ 3] = 0;
				
			m[ 4] = RF (sa);
			m[ 5] = RF (ca);
			m[ 6] = 0;
			m[ 7] = 0;
				
			m[ 8] = 0;
			m[ 9] = 0;
			m[10] = RF_1;
			m[11] = 0;
		break;

		default:
			break;
	}
}




inline void MTX44RotTrig (float *m, char axis, float sa, float ca)
{
	switch (axis)
	{
		case 'x':
		case 'X':
			m[ 0] = RF_1;
			m[ 1] = 0;
			m[ 2] = 0;
			m[ 3] = 0;
				
			m[ 4] = 0;
			m[ 5] = RF ( ca);
			m[ 6] = RF (-sa);
			m[ 7] = 0;
				
			m[ 8] = 0;
			m[ 9] = RF (sa);
			m[10] = RF (ca);
			m[11] = 0;

			m[12] = 0;
			m[13] = 0;
			m[14] = 0;
			m[15] = RF_1;
			break;

		case 'y':                       
		case 'Y':                       
			m[ 0] = RF (ca);
			m[ 1] = 0;
			m[ 2] = RF (sa);
			m[ 3] = 0;

			m[ 4] = 0;
			m[ 5] = RF_1;
			m[ 6] = 0;
			m[ 7] = 0;

			m[ 8] = RF (-sa);
			m[ 9] = 0;
			m[10] = RF ( ca);
			m[11] = 0;

			m[12] = 0;
			m[13] = 0;
			m[14] = 0;
			m[15] = RF_1;
			break;

    case 'z':
    case 'Z':
			m[ 0] = RF ( ca);
			m[ 1] = RF (-sa);
			m[ 2] = 0;
			m[ 3] = 0;
				
			m[ 4] = RF (sa);
			m[ 5] = RF (ca);
			m[ 6] = 0;
			m[ 7] = 0;
				
			m[ 8] = 0;
			m[ 9] = 0;
			m[10] = RF_1;
			m[11] = 0;

			m[12] = 0;
			m[13] = 0;
			m[14] = 0;
			m[15] = RF_1;
		break;

		default:
			break;
	}
}


inline void VECCopy (void *src, void *dst)
{
	memcpy (dst, src, 4 * 3);
}


inline float VECDotProduct (float *a, float *b)
{
	return RF (a[0]) * RF (b[0]) + RF (a[1]) * RF (b[1]) + RF (a[2]) * RF (b[2]);
}


inline void VECCrossProduct (float *a, float *b, float *axb)
{
	float t[3], *d;


	d = ((axb == a) || (axb == b)) ? t : axb;

	d[0] = RF (RF (a[1])*RF (b[2]) - RF (a[2])*RF (b[1]));
	d[1] = RF (RF (a[2])*RF (b[0]) - RF (a[0])*RF (b[2]));
	d[2] = RF (RF (a[0])*RF (b[1]) - RF (a[1])*RF (b[0]));
	
	if (d == t)
		VECCopy (t, axb);
}


inline void VECCrossProduct_rnr (float *a, float *b, float *axb)
{
	float t[3], *d;


	d = ((axb == a) || (axb == b)) ? t : axb;

	d[0] = RF (a[1])*RF (b[2]) - RF (a[2])*RF (b[1]);
	d[1] = RF (a[2])*RF (b[0]) - RF (a[0])*RF (b[2]);
	d[2] = RF (a[0])*RF (b[1]) - RF (a[1])*RF (b[0]);
	
	if (d == t)
		VECCopy (t, axb);
}


inline void VECCrossProduct_rnr_inr (float *a, float *b, float *axb)
{
	float t[3], *d;


	d = ((axb == a) || (axb == b)) ? t : axb;

	d[0] = a[1]*b[2] - a[2]*b[1];
	d[1] = a[2]*b[0] - a[0]*b[2];
	d[2] = a[0]*b[1] - a[1]*b[0];
	
	if (d == t)
		VECCopy (t, axb);
}


inline void VECScale (float *src, float *dst, float s)
{
	dst[0] = RF (RF (src[0]) * s);
	dst[1] = RF (RF (src[1]) * s);
	dst[2] = RF (RF (src[2]) * s);
}


inline float VECSquareMag (float *v)
{
	return VECDotProduct (v, v);
}


inline float VECMag (float *v)
{
	return sqrt (VECSquareMag (v));
}


inline void VECNormalize (float *v, float *nv)
{
	VECScale (v, nv, 1.0f / VECMag (v));
}


inline void VECNormalize_rnr_inr (float *v, float *nv)
{
	float s;


	s = 1.0f / sqrt (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

	nv[0] = v[0] * s;
	nv[1] = v[1] * s;
	nv[2] = v[2] * s;
}


inline void VECNormalize_rnr (float *v, float *nv)
{
#ifdef LIL_ENDIAN
	float t[3] = VRF (v);
	v = t;
#endif

	VECNormalize_rnr_inr (v, nv);
}


inline void VECSubtract (float *a, float *b, float *a_b)
{
	a_b[0] = RF (RF (a[0]) - RF (b[0]));
	a_b[1] = RF (RF (a[1]) - RF (b[1]));
	a_b[2] = RF (RF (a[2]) - RF (b[2]));
}


inline void VECSubtract_rnr (float *a, float *b, float *a_b)
{
	a_b[0] = RF (a[0]) - RF (b[0]);
	a_b[1] = RF (a[1]) - RF (b[1]);
	a_b[2] = RF (a[2]) - RF (b[2]);
}


inline void VECSubtract_rnr_inr (float *a, float *b, float *a_b)
{
	a_b[0] = a[0] - b[0];
	a_b[1] = a[1] - b[1];
	a_b[2] = a[2] - b[2];
}


inline void VECAdd (float *a, float *b, float *a_b)
{
	a_b[0] = RF (RF (a[0]) + RF (b[0]));
	a_b[1] = RF (RF (a[1]) + RF (b[1]));
	a_b[2] = RF (RF (a[2]) + RF (b[2]));
}


// vector

HLE (PSVECDotProduct)
{
	HLE_RETURN_F (VECDotProduct (HLE_PARAM_1_PTR_F, HLE_PARAM_2_PTR_F));
}


HLE (PSVECCrossProduct)
{
	VECCrossProduct (HLE_PARAM_1_PTR_F, HLE_PARAM_2_PTR_F, HLE_PARAM_3_PTR_F);
}


HLE (PSVECScale)
{
	VECScale (HLE_PARAM_1_PTR_F, HLE_PARAM_2_PTR_F, HLE_PARAM_F1);
}


HLE (PSVECSquareMag)
{
	HLE_RETURN_F (VECSquareMag (HLE_PARAM_1_PTR_F));
}


HLE (PSVECMag)
{
	HLE_RETURN_F (VECMag (HLE_PARAM_1_PTR_F));
}


HLE (PSVECNormalize)
{
	VECNormalize (HLE_PARAM_1_PTR_F, HLE_PARAM_2_PTR_F);
}


HLE (PSVECSubtract)
{
	VECSubtract (HLE_PARAM_1_PTR_F, HLE_PARAM_2_PTR_F, HLE_PARAM_3_PTR_F);
}


HLE (PSVECAdd)
{
	VECAdd (HLE_PARAM_1_PTR_F, HLE_PARAM_2_PTR_F, HLE_PARAM_3_PTR_F);
}


// matrix

HLE (PSMTXIdentity)
{
	float *m = HLE_PARAM_1_PTR_F;

	
	m[ 0] = RF_1;
	m[ 1] = 0;
	m[ 2] = 0;
	m[ 3] = 0;

	m[ 4] = 0;
	m[ 5] = RF_1;
	m[ 6] = 0;
	m[ 7] = 0;

	m[ 8] = 0;
	m[ 9] = 0;
	m[10] = RF_1;
	m[11] = 0;
}


HLE (PSMTXCopy)
{
	if (HLE_PARAM_1 != HLE_PARAM_2)
		MTXCopy (HLE_PARAM_1_PTR_F, HLE_PARAM_2_PTR_F);
}


HLE (PSMTXConcat)
{
	float *a = HLE_PARAM_1_PTR_F, *b = HLE_PARAM_2_PTR_F;
	float *m, *ab = HLE_PARAM_3_PTR_F;
	float t[12];

#ifdef LIL_ENDIAN
	float ta[12] = MRF (a);
	float tb[12] = MRF (b);
	a = ta;
	b = tb;
#endif

	m = ((ab == a) || (ab == b)) ? t : ab;

	// a x b
	m[ 0] = RF (a[0]*b[0] + a[1]*b[4] + a[2]*b[ 8]);
	m[ 1] = RF (a[0]*b[1] + a[1]*b[5] + a[2]*b[ 9]);
	m[ 2] = RF (a[0]*b[2] + a[1]*b[6] + a[2]*b[10]);
	m[ 3] = RF (a[0]*b[3] + a[1]*b[7] + a[2]*b[11] + a[ 3]);

	m[ 4] = RF (a[4]*b[0] + a[5]*b[4] + a[6]*b[ 8]);
	m[ 5] = RF (a[4]*b[1] + a[5]*b[5] + a[6]*b[ 9]);
	m[ 6] = RF (a[4]*b[2] + a[5]*b[6] + a[6]*b[10]);
	m[ 7] = RF (a[4]*b[3] + a[5]*b[7] + a[6]*b[11] + a[ 7]);

	m[ 8] = RF (a[8]*b[0] + a[9]*b[4] + a[10]*b[ 8]);
	m[ 9] = RF (a[8]*b[1] + a[9]*b[5] + a[10]*b[ 9]);
	m[10] = RF (a[8]*b[2] + a[9]*b[6] + a[10]*b[10]);
	m[11] = RF (a[8]*b[3] + a[9]*b[7] + a[10]*b[11] + a[11]);

	if (m == t)
		MTXCopy (t, ab);
}


HLE (PSMTXTranspose)
{
	float *m;
	float *src = HLE_PARAM_1_PTR_F, *dst = HLE_PARAM_2_PTR_F;
	float t[12];


	m = (src == dst) ? t : dst;

	m[ 0] = src[ 0];
	m[ 1] = src[ 4];
	m[ 2] = src[ 8];
	m[ 3] = 0;

	m[ 4] = src[ 1];
	m[ 5] = src[ 5];
	m[ 6] = src[ 9];
	m[ 7] = 0;

	m[ 8] = src[ 2];
	m[ 9] = src[ 6];
	m[10] = src[10];
	m[11] = 0;

	if (m == t)
		MTXCopy (t, dst);
}


HLE (PSMTXInverse)
{
	float *m;
	float *src = HLE_PARAM_1_PTR_F, *dst = HLE_PARAM_2_PTR_F;
	float t[12], det;


	m = (src == dst) ? t : dst;

	if ((det = det_3x3 (src)) == 0.0f)
		HLE_RETURN (0);
	else
		det = 1.0f / det;

	m[ 0] =  (RF (src[5])*RF (src[10]) - RF (src[9])*RF (src[6])) * det;
	m[ 1] = -(RF (src[1])*RF (src[10]) - RF (src[9])*RF (src[2])) * det;
	m[ 2] =  (RF (src[1])*RF (src[ 6]) - RF (src[5])*RF (src[2])) * det;

	m[ 4] = -(RF (src[4])*RF (src[10]) - RF (src[8])*RF (src[6])) * det;
	m[ 5] =  (RF (src[0])*RF (src[10]) - RF (src[8])*RF (src[2])) * det;
	m[ 6] = -(RF (src[0])*RF (src[ 6]) - RF (src[4])*RF (src[2])) * det;

	m[ 8] =  (RF (src[4])*RF (src[ 9]) - RF (src[8])*RF (src[5])) * det;
	m[ 9] = -(RF (src[0])*RF (src[ 9]) - RF (src[8])*RF (src[1])) * det;
	m[10] =  (RF (src[0])*RF (src[ 5]) - RF (src[4])*RF (src[1])) * det;

	m[ 3] = -m[ 0]*RF (src[3]) - m[ 1]*RF (src[7]) - m[ 2]*RF (src[11]);
	m[ 7] = -m[ 4]*RF (src[3]) - m[ 5]*RF (src[7]) - m[ 6]*RF (src[11]);
	m[11] = -m[ 8]*RF (src[3]) - m[ 9]*RF (src[7]) - m[10]*RF (src[11]);

#ifdef LIL_ENDIAN
	m[ 0] = RF (m[ 0]);
	m[ 1] = RF (m[ 1]);
	m[ 2] = RF (m[ 2]);
	m[ 3] = RF (m[ 3]);

	m[ 4] = RF (m[ 4]);
	m[ 5] = RF (m[ 5]);
	m[ 6] = RF (m[ 6]);
	m[ 7] = RF (m[ 7]);

	m[ 8] = RF (m[ 8]);
	m[ 9] = RF (m[ 9]);
	m[10] = RF (m[10]);
	m[11] = RF (m[11]);
#endif

	if (m == t)
		MTXCopy (t, dst);

	HLE_RETURN (1);
}


// inverse transpose
HLE (PSMTXInvXpose)
{
	float *m;
	float *src = HLE_PARAM_1_PTR_F, *dst = HLE_PARAM_2_PTR_F;
	float t[12], det;


	m = (src == dst) ? t : dst;

	if ((det = det_3x3 (src)) == 0.0f)
		HLE_RETURN (0);
	else
		det = 1.0f / det;

	m[ 0] = RF ( (RF (src[5])*RF (src[10]) - RF (src[9])*RF (src[6])) * det);
	m[ 1] = RF (-(RF (src[4])*RF (src[10]) - RF (src[8])*RF (src[6])) * det);
	m[ 2] = RF ( (RF (src[4])*RF (src[ 9]) - RF (src[8])*RF (src[5])) * det);

	m[ 4] = RF (-(RF (src[1])*RF (src[10]) - RF (src[9])*RF (src[2])) * det);
	m[ 5] = RF ( (RF (src[0])*RF (src[10]) - RF (src[8])*RF (src[2])) * det);
	m[ 6] = RF (-(RF (src[0])*RF (src[ 9]) - RF (src[8])*RF (src[1])) * det);

	m[ 8] = RF ( (RF (src[1])*RF (src[ 6]) - RF (src[5])*RF (src[2])) * det);
	m[ 9] = RF (-(RF (src[0])*RF (src[ 6]) - RF (src[4])*RF (src[2])) * det);
	m[10] = RF ( (RF (src[0])*RF (src[ 5]) - RF (src[4])*RF (src[1])) * det);

	m[ 3] = 0;
	m[ 7] = 0;
	m[11] = 0;

	if (m == t)
		MTXCopy (t, dst);

	HLE_RETURN (1);
}


HLE (PSMTXRotRad)
{
	MTXRotTrig (HLE_PARAM_1_PTR_F, HLE_PARAM_2,
							sin (HLE_PARAM_F1), cos (HLE_PARAM_F1));
}


HLE (PSMTXRotTrig)
{
	MTXRotTrig (HLE_PARAM_1_PTR_F, HLE_PARAM_2, HLE_PARAM_F1, HLE_PARAM_F2);
}


HLE (PSMTXRotAxisRad)
{
	float *m = HLE_PARAM_1_PTR_F, *v = HLE_PARAM_2_PTR_F;
	float nv[3], s, c, t;


	VECNormalize_rnr (v, nv);

	s = sin (HLE_PARAM_F1);
	c = cos (HLE_PARAM_F1);
	t = 1.0f - c;

	m[ 0] = RF ((t * nv[0] * nv[0]) + c);
	m[ 1] = RF ((t * nv[0] * nv[1]) - (s * nv[2]));
	m[ 2] = RF ((t * nv[0] * nv[2]) + (s * nv[1]));
	m[ 3] = 0;

	m[ 4] = RF ((t * nv[0] * nv[1]) + (s * nv[2]));
	m[ 5] = RF ((t * nv[1] * nv[1]) + c);
	m[ 6] = RF ((t * nv[1] * nv[2]) - (s * nv[0]));
	m[ 7] = 0;

	m[ 8] = RF ((t * nv[0] * nv[2]) - (s * nv[1]));
	m[ 9] = RF ((t * nv[1] * nv[2]) + (s * nv[0]));
	m[10] = RF ((t * nv[2] * nv[2]) + c);
	m[11] = 0;
}


HLE (PSMTXTrans)
{
	float *m = HLE_PARAM_1_PTR_F;


	m[ 0] = RF_1;
	m[ 1] = 0;
	m[ 2] = 0;
	m[ 3] = RF_HLE_PARAM_F1;
	
	m[ 4] = 0;
	m[ 5] = RF_1;
	m[ 6] = 0;
	m[ 7] = RF_HLE_PARAM_F2;
	
	m[ 8] = 0;
	m[ 9] = 0;
	m[10] = RF_1;
	m[11] = RF_HLE_PARAM_F3;
}


HLE (PSMTXTransApply)
{
	float *src = HLE_PARAM_1_PTR_F, *dst = HLE_PARAM_2_PTR_F;


	if (src != dst)
	{
		dst[ 0] = src[ 0];
		dst[ 1] = src[ 1];
		dst[ 2] = src[ 2];

		dst[ 4] = src[ 4];
		dst[ 5] = src[ 5];
		dst[ 6] = src[ 6];

		dst[ 8] = src[ 8];
		dst[ 9] = src[ 9];
		dst[10] = src[10];
	}

	dst[ 3] = RF (RF (src[ 3]) + HLE_PARAM_F1);
	dst[ 7] = RF (RF (src[ 7]) + HLE_PARAM_F2);
	dst[11] = RF (RF (src[11]) + HLE_PARAM_F3);
}


HLE (PSMTXScale)
{
	float *m = HLE_PARAM_1_PTR_F;


	m[ 0] = RF_HLE_PARAM_F1;
	m[ 1] = 0;
	m[ 2] = 0;
	m[ 3] = 0;

	m[ 4] = 0;
	m[ 5] = RF_HLE_PARAM_F2;
	m[ 6] = 0;
	m[ 7] = 0;

	m[ 8] = 0;
	m[ 9] = 0;
	m[10] = RF_HLE_PARAM_F3;
	m[11] = 0;
}


HLE (PSMTXScaleApply)
{
	float *src = HLE_PARAM_1_PTR_F, *dst = HLE_PARAM_2_PTR_F;


	dst[ 0] = RF (RF (src[ 0]) * HLE_PARAM_F1);
	dst[ 1] = RF (RF (src[ 1]) * HLE_PARAM_F1);
	dst[ 2] = RF (RF (src[ 2]) * HLE_PARAM_F1);
	dst[ 3] = RF (RF (src[ 3]) * HLE_PARAM_F1);

	dst[ 4] = RF (RF (src[ 4]) * HLE_PARAM_F2);
	dst[ 5] = RF (RF (src[ 5]) * HLE_PARAM_F2);
	dst[ 6] = RF (RF (src[ 6]) * HLE_PARAM_F2);
	dst[ 7] = RF (RF (src[ 7]) * HLE_PARAM_F2);

	dst[ 8] = RF (RF (src[ 8]) * HLE_PARAM_F3);
	dst[ 9] = RF (RF (src[ 9]) * HLE_PARAM_F3);
	dst[10] = RF (RF (src[10]) * HLE_PARAM_F3);
	dst[11] = RF (RF (src[11]) * HLE_PARAM_F3);
}


HLE (PSMTXQuat)
{
	float *m = HLE_PARAM_1_PTR_F, *q = HLE_PARAM_2_PTR_F;
	float s, xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;


#ifdef LIL_ENDIAN
	float t[4] = QRF (q);
	q = t;
#endif

	s = 2.0f / ((q[0] * q[0]) + (q[1] * q[1]) + (q[2] * q[2]) + (q[3] * q[3]));

	xs = q[0] * s;
	ys = q[1] * s;
	zs = q[2] * s;
	wx = q[3] * xs;
	wy = q[3] * ys;
	wz = q[3] * zs;
	xx = q[0] * xs;
	xy = q[0] * ys;
	xz = q[0] * zs;
	yy = q[1] * ys;
	yz = q[1] * zs;
	zz = q[2] * zs;

	m[ 0] = RF (1.0f - (yy + zz));
	m[ 1] = RF (xy - wz);
	m[ 2] = RF (xz + wy);
	m[ 3] = 0;

	m[ 4] = RF (xy + wz);
	m[ 5] = RF (1.0f - (xx + zz));
	m[ 6] = RF (yz - wx);
	m[ 7] = 0;

	m[ 8] = RF (xz - wy);
	m[ 9] = RF (yz + wx);
	m[10] = RF (1.0f - (xx + yy));
	m[11] = 0;
}


HLE (PSMTXReflect)
{
	float *m = HLE_PARAM_1_PTR_F, *p = HLE_PARAM_2_PTR_F, *n = HLE_PARAM_3_PTR_F;
	float vxy, vxz, vyz, pdotn;

#ifdef LIL_ENDIAN
	float rn[3] = VRF (n);
	n = rn;
#endif

	vxy = -2.0f * n[0] * n[1];
	vxz = -2.0f * n[0] * n[2];
	vyz = -2.0f * n[1] * n[2];
	pdotn = 2.0f * (RF (p[0]) * n[0] + RF (p[1]) * n[1] + RF (p[2]) * n[2]);

	m[ 0] = RF (1.0f - 2.0f * n[0] * n[0]);
	m[ 1] = RF (vxy);
	m[ 2] = RF (vxz);
	m[ 3] = RF (pdotn * n[0]);

	m[ 4] = RF (vxy);
	m[ 5] = RF (1.0f - 2.0f * n[1] * n[1]);
	m[ 6] = RF (vyz);
	m[ 7] = RF (pdotn * n[1]);

	m[ 8] = RF (vxz);
	m[ 9] = RF (vyz);
	m[10] = RF (1.0f - 2.0f * n[2] * n[2]);
	m[11] = RF (pdotn * n[2]);
}



HLE (PSMTXLookAt)
{
	float *m = HLE_PARAM_1_PTR_F;
	float *cam = HLE_PARAM_2_PTR_F, *camup = HLE_PARAM_3_PTR_F, *dest = HLE_PARAM_4_PTR_F;
	float look[3], right[3], up[3];

#ifdef LIL_ENDIAN
	float tcam[3] = VRF (cam);
	float tcamup[3] = VRF (camup);
	float tdest[3] = VRF (dest);
	cam = tcam;
	camup = tcamup;
	dest = tdest;
#endif

	VECSubtract_rnr_inr (cam, dest, look);
	VECNormalize_rnr_inr (look, look);

	// right = camup x look
	VECCrossProduct_rnr_inr (camup, look, right);
	VECNormalize_rnr_inr (right, right);

	// up = look x right
	VECCrossProduct_rnr_inr (look, right, up);

	m[ 0] = RF (right[0]);
	m[ 1] = RF (right[1]);
	m[ 2] = RF (right[2]);
	m[ 3] = RF (-(cam[0] * right[0] + cam[1] * right[1] + cam[2] * right[2]));

	m[ 4] = RF (up[0]);
	m[ 5] = RF (up[1]);
	m[ 6] = RF (up[2]);
	m[ 7] = RF (-(cam[0] * up[0] + cam[1] * up[1] + cam[2] * up[2]));

	m[ 8] = RF (look[0]);
	m[ 9] = RF (look[1]);
	m[10] = RF (look[2]);
	m[11] = RF (-(cam[0] * look[0] + cam[1] * look[1] + cam[2] * look[2]));
}


HLE (PSMTXLightFrustum)
{
	float *m = HLE_PARAM_1_PTR_F, t;


	t = 1.0f / (HLE_PARAM_F4 - HLE_PARAM_F3);
	m[ 0] = RF (2.0f * t * HLE_PARAM_F5 * HLE_PARAM_F6);
	m[ 1] = 0;
	m[ 2] = RF ((HLE_PARAM_F4 + HLE_PARAM_F3) * t * HLE_PARAM_F6 - HLE_PARAM_F8);
	m[ 3] = 0;

	t = 1.0f / (HLE_PARAM_F1 - HLE_PARAM_F2);
	m[ 4] = 0;
	m[ 5] = RF (2.0f * t * HLE_PARAM_F5 * HLE_PARAM_F7);
	m[ 6] = RF ((HLE_PARAM_F1 + HLE_PARAM_F2) * t * HLE_PARAM_F7 - HLE_PARAM_F9);
	m[ 7] = 0;

	m[ 8] = 0;
	m[ 9] = 0;
	m[10] = RF_M1;
	m[11] = 0;
}


HLE (PSMTXLightPerspective)
{
	float *m = HLE_PARAM_1_PTR_F, t;


	t = 1.0f / tan (MTXDegToRad (HLE_PARAM_F1 / 2));

	m[ 0] = RF ((t / HLE_PARAM_F2) * HLE_PARAM_F3);
	m[ 1] = 0;
	m[ 2] = RF (-1.0f * HLE_PARAM_F5);
	m[ 3] = 0;

	m[ 4] = 0;
	m[ 5] = RF (t * HLE_PARAM_F4);
	m[ 6] = RF (-1.0f * HLE_PARAM_F6);
	m[ 7] = 0;

	m[ 8] = 0;
	m[ 9] = 0;
	m[10] = RF_M1;
	m[11] = 0;
}


HLE (PSMTXLightOrtho)
{
	float *m = HLE_PARAM_1_PTR_F, t;


	t = 1.0f / (HLE_PARAM_F4 - HLE_PARAM_F3);
	m[0] = RF (2.0f * t * HLE_PARAM_F5);
	m[1] = 0;
	m[2] = 0;
	m[3] = RF (-t * (HLE_PARAM_F4 + HLE_PARAM_F3) * HLE_PARAM_F5 + HLE_PARAM_F7);

	t = 1.0f / (HLE_PARAM_F1 - HLE_PARAM_F2);
	m[ 4] = 0;
	m[ 5] = RF (2.0f * t * HLE_PARAM_F6);
	m[ 6] = 0;
	m[ 7] = RF (-t * (HLE_PARAM_F1 + HLE_PARAM_F2) * HLE_PARAM_F6 + HLE_PARAM_F8);

	m[ 8] = 0;
	m[ 9] = 0;
	m[10] = 0;
	m[11] = RF_1;
}


HLE (PSMTXMultVec)
{
	float *m = HLE_PARAM_1_PTR_F, *src = HLE_PARAM_2_PTR_F, *dst = HLE_PARAM_3_PTR_F;
	float *d, t[3];

#ifdef LIL_ENDIAN
	float tsrc[3] = VRF (src);
	src = tsrc;
#endif

	d = (src == dst) ? t : dst;

	d[0] = RF (RF (m[0])*src[0] + RF (m[1])*src[1] + RF (m[ 2])*src[2] + RF (m[ 3]));
	d[1] = RF (RF (m[4])*src[0] + RF (m[5])*src[1] + RF (m[ 6])*src[2] + RF (m[ 7]));
	d[2] = RF (RF (m[8])*src[0] + RF (m[9])*src[1] + RF (m[10])*src[2] + RF (m[11]));

	if (d == t)
		VECCopy (t, dst);
}


HLE (PSMTXMultVecArray)
{
	float *m = HLE_PARAM_1_PTR_F, *src = HLE_PARAM_2_PTR_F, *dst = HLE_PARAM_3_PTR_F;
	__u32 i = HLE_PARAM_4;
	float *d, *s, t[3];

#ifdef LIL_ENDIAN
	float tm[12] = MRF (m);
	m = tm;
#endif


	s = src;
	d = (src == dst) ? t : dst;
	while (i--)
	{
#ifdef LIL_ENDIAN
		float tsrc[3] = VRF (s);
		s = tsrc;
#endif
	
		d[0] = RF (m[0]*s[0] + m[1]*s[1] + m[ 2]*s[2] + m[ 3]);
		d[1] = RF (m[4]*s[0] + m[5]*s[1] + m[ 6]*s[2] + m[ 7]);
		d[2] = RF (m[8]*s[0] + m[9]*s[1] + m[10]*s[2] + m[11]);

		if (src == dst)
		{
			VECCopy (t, dst);
			dst += 3;
		}
		else
			d += 3;

		s = (src += 3);
	}
}


HLE (PSMTXMultVecSR)
{
	float *m = HLE_PARAM_1_PTR_F, *src = HLE_PARAM_2_PTR_F, *dst = HLE_PARAM_3_PTR_F;
	float *d, t[3];

#ifdef LIL_ENDIAN
	float tsrc[3] = VRF (src);
	src = tsrc;
#endif

	d = (src == dst) ? t : dst;

	d[0] = RF (RF (m[0])*src[0] + RF (m[1])*src[1] + RF (m[ 2])*src[2]);
	d[1] = RF (RF (m[4])*src[0] + RF (m[5])*src[1] + RF (m[ 6])*src[2]);
	d[2] = RF (RF (m[8])*src[0] + RF (m[9])*src[1] + RF (m[10])*src[2]);

	if (d == t)
		VECCopy (t, dst);
}


HLE (PSMTXMultVecArraySR)
{
	float *m = HLE_PARAM_1_PTR_F, *src = HLE_PARAM_2_PTR_F, *dst = HLE_PARAM_3_PTR_F;
	__u32 i = HLE_PARAM_4;
	float *d, *s, t[3];

#ifdef LIL_ENDIAN
	float tm[12] = MRF (m);
	m = tm;
#endif

	s = src;
	d = (src == dst) ? t : dst;
	while (i--)
	{
#ifdef LIL_ENDIAN
		float tsrc[3] = VRF (s);
		s = tsrc;
#endif
	
		d[0] = RF (m[0]*s[0] + m[1]*s[1] + m[ 2]*s[2]);
		d[1] = RF (m[4]*s[0] + m[5]*s[1] + m[ 6]*s[2]);
		d[2] = RF (m[8]*s[0] + m[9]*s[1] + m[10]*s[2]);

		if (src == dst)
		{
			VECCopy (t, dst);
			dst += 3;
		}
		else
			d += 3;

		s = (src += 3);
	}
}


HLE (PSMTX44MultVec)
{
	float *m = HLE_PARAM_1_PTR_F, *src = HLE_PARAM_2_PTR_F, *dst = HLE_PARAM_3_PTR_F;
	float *d, t[3], w;

#ifdef LIL_ENDIAN
	float tsrc[3] = VRF (src);
	src = tsrc;
#endif

	d = (src == dst) ? t : dst;

	w    = 1.0f / (RF (m[12])*src[0] + RF (m[13])*src[1] + RF (m[14])*src[2] + RF (m[15]));
	d[0] = RF ((RF (m[0])*src[0] + RF (m[1])*src[1] + RF (m[ 2])*src[2] + RF (m[ 3])) * w);
	d[1] = RF ((RF (m[4])*src[0] + RF (m[5])*src[1] + RF (m[ 6])*src[2] + RF (m[ 7])) * w);
	d[2] = RF ((RF (m[8])*src[0] + RF (m[9])*src[1] + RF (m[10])*src[2] + RF (m[11])) * w);

	if (d == t)
		VECCopy (t, dst);
}


HLE (PSMTX44MultVecArray)
{
	float *m = HLE_PARAM_1_PTR_F, *src = HLE_PARAM_2_PTR_F, *dst = HLE_PARAM_3_PTR_F;
	__u32 i = HLE_PARAM_4;
	float *d, t[3], w;

#ifdef LIL_ENDIAN
	float tm[16] = M44RF (m);
	m = tm;
#endif


	d = (src == dst) ? t : dst;
	while (i--)
	{
#ifdef LIL_ENDIAN
		float tsrc[3] = VRF (src);
		src = tsrc;
#endif
	
		w = 1.0f / (m[12]*src[0] + m[13]*src[1] + m[14]*src[2] + m[15]);
		d[0] = RF ((m[0]*src[0] + m[1]*src[1] + m[ 2]*src[2] + m[ 3]) * w);
		d[1] = RF ((m[4]*src[0] + m[5]*src[1] + m[ 6]*src[2] + m[ 7]) * w);
		d[2] = RF ((m[8]*src[0] + m[9]*src[1] + m[10]*src[2] + m[11]) * w);

		if (src == dst)
		{
			VECCopy (t, dst);
			dst += 3;
		}
		else
			d += 3;

		src += 3;
	}
}


HLE (PSMTXFrustum)
{
	float *m = HLE_PARAM_1_PTR_F, t;


	t = 1.0f / (HLE_PARAM_F4 - HLE_PARAM_F3);
	m[ 0] = RF (2.0f * t * HLE_PARAM_F5);
	m[ 1] = 0;
	m[ 2] = RF ((HLE_PARAM_F4 + HLE_PARAM_F3) * t);
	m[ 3] = 0;

	t = 1.0f / (HLE_PARAM_F1 - HLE_PARAM_F2);
	m[ 4] = 0;
	m[ 5] = RF (2.0f * t * HLE_PARAM_F5);
	m[ 6] = RF ((HLE_PARAM_F1 + HLE_PARAM_F2) * t);
	m[ 7] = 0;

	t = 1.0f / (HLE_PARAM_F6 - HLE_PARAM_F5);
	m[ 8] = 0;
	m[ 9] = 0;
	m[10] = RF (-HLE_PARAM_F5 * t);
	m[11] = RF (-HLE_PARAM_F6 * HLE_PARAM_F5 * t);

	m[12] = 0;
	m[13] = 0;
	m[14] = RF_M1;
	m[15] = 0;
}


HLE (PSMTXPerspective)
{
	float *m = HLE_PARAM_1_PTR_F, t;


	t = 1.0f / tan (MTXDegToRad (HLE_PARAM_F1 / 2));
	m[ 0] = RF (t / HLE_PARAM_F2);
	m[ 1] = 0;
	m[ 2] = 0;
	m[ 3] = 0;

	m[ 4] = 0;
	m[ 5] = RF (t);
	m[ 6] = 0;
	m[ 7] = 0;

	t = 1.0f / (HLE_PARAM_F4 - HLE_PARAM_F3);
	m[ 8] = 0;
	m[ 9] = 0;
	m[10] = RF (-t * HLE_PARAM_F3);
	m[11] = RF (-t * HLE_PARAM_F4 * HLE_PARAM_F3);

	m[12] = 0;
	m[13] = 0;
	m[14] = RF_M1;
	m[15] = 0;
}


HLE (PSMTXOrtho)
{
	float *m = HLE_PARAM_1_PTR_F, t;


	t = 1.0f / (HLE_PARAM_F4 - HLE_PARAM_F3);
	m[0] = RF (2.0f * t);
	m[1] = 0;
	m[2] = 0;
	m[3] = RF (-t * (HLE_PARAM_F4 + HLE_PARAM_F3));

	t = 1.0f / (HLE_PARAM_F1 - HLE_PARAM_F2);
	m[ 4] = 0;
	m[ 5] = RF (2.0f * t);
	m[ 6] = 0;
	m[ 7] = RF (-t * (HLE_PARAM_F1 + HLE_PARAM_F2));

	t = 1.0f / (HLE_PARAM_F6 - HLE_PARAM_F5);
	m[ 8] = 0;
	m[ 9] = 0;
	m[10] = RF (-t);
	m[11] = RF (-t * HLE_PARAM_F6);

	m[12] = 0;
	m[13] = 0;
	m[14] = 0;
	m[15] = RF_1;
}


HLE (PSMTX44Identity)
{
	float *m = HLE_PARAM_1_PTR_F;

	
	m[ 0] = RF_1;
	m[ 1] = 0;
	m[ 2] = 0;
	m[ 3] = 0;

	m[ 4] = 0;
	m[ 5] = RF_1;
	m[ 6] = 0;
	m[ 7] = 0;

	m[ 8] = 0;
	m[ 9] = 0;
	m[10] = RF_1;
	m[11] = 0;

	m[12] = 0;
	m[13] = 0;
	m[14] = 0;
	m[15] = RF_1;
}


HLE (PSMTX44Copy)
{
	if (HLE_PARAM_1 != HLE_PARAM_2)
		MTX44Copy (HLE_PARAM_1_PTR_F, HLE_PARAM_2_PTR_F);
}


HLE (PSMTX44Concat)
{
	float *a = HLE_PARAM_1_PTR_F, *b = HLE_PARAM_2_PTR_F;
	float *m, *ab = HLE_PARAM_3_PTR_F;
	float t[16];

#ifdef LIL_ENDIAN
	float ta[16] = M44RF (a);
	float tb[16] = M44RF (b);
	a = ta;
	b = tb;
#endif

	m = ((ab == a) || (ab == b)) ? t : ab;

	// a x b
	m[ 0] = RF (a[0]*b[0] + a[1]*b[4] + a[2]*b[ 8] + a[3]*b[12]);
	m[ 1] = RF (a[0]*b[1] + a[1]*b[5] + a[2]*b[ 9] + a[3]*b[13]);
	m[ 2] = RF (a[0]*b[2] + a[1]*b[6] + a[2]*b[10] + a[3]*b[14]);
	m[ 3] = RF (a[0]*b[3] + a[1]*b[7] + a[2]*b[11] + a[3]*b[15]);

	m[ 4] = RF (a[4]*b[0] + a[5]*b[4] + a[6]*b[ 8] + a[7]*b[12]);
	m[ 5] = RF (a[4]*b[1] + a[5]*b[5] + a[6]*b[ 9] + a[7]*b[13]);
	m[ 6] = RF (a[4]*b[2] + a[5]*b[6] + a[6]*b[10] + a[7]*b[14]);
	m[ 7] = RF (a[4]*b[3] + a[5]*b[7] + a[6]*b[11] + a[7]*b[15]);

	m[ 8] = RF (a[8]*b[0] + a[9]*b[4] + a[10]*b[ 8] + a[11]*b[12]);
	m[ 9] = RF (a[8]*b[1] + a[9]*b[5] + a[10]*b[ 9] + a[11]*b[13]);
	m[10] = RF (a[8]*b[2] + a[9]*b[6] + a[10]*b[10] + a[11]*b[14]);
	m[11] = RF (a[8]*b[3] + a[9]*b[7] + a[10]*b[11] + a[11]*b[15]);

	m[12] = RF (a[12]*b[0] + a[13]*b[4] + a[14]*b[ 8] + a[15]*b[12]);
	m[13] = RF (a[12]*b[1] + a[13]*b[5] + a[14]*b[ 9] + a[15]*b[13]);
	m[14] = RF (a[12]*b[2] + a[13]*b[6] + a[14]*b[10] + a[15]*b[14]);
	m[15] = RF (a[12]*b[3] + a[13]*b[7] + a[14]*b[11] + a[15]*b[15]);

	if (m == t)
		MTX44Copy (t, ab);
}


HLE (PSMTX44Transpose)
{
	float *m;
	float *src = HLE_PARAM_1_PTR_F, *dst = HLE_PARAM_2_PTR_F;
	float t[16];


	m = (src == dst) ? t : dst;

	m[ 0] = src[ 0];
	m[ 1] = src[ 4];
	m[ 2] = src[ 8];
	m[ 3] = src[12];

	m[ 4] = src[ 1];
	m[ 5] = src[ 5];
	m[ 6] = src[ 9];
	m[ 7] = src[13];

	m[ 8] = src[ 2];
	m[ 9] = src[ 6];
	m[10] = src[10];
	m[11] = src[14];

	m[12] = src[ 3];
	m[13] = src[ 7];
	m[14] = src[11];
	m[15] = src[15];

	if (m == t)
		MTX44Copy (t, dst);
}


// add here MTX44Inverse


HLE (PSMTX44RotRad)
{
	MTX44RotTrig (HLE_PARAM_1_PTR_F, HLE_PARAM_2,
							sin (HLE_PARAM_F1), cos (HLE_PARAM_F1));
}


HLE (PSMTX44RotTrig)
{
	MTX44RotTrig (HLE_PARAM_1_PTR_F, HLE_PARAM_2, HLE_PARAM_F1, HLE_PARAM_F2);
}


HLE (PSMTX44RotAxisRad)
{
	float *m = HLE_PARAM_1_PTR_F, *v = HLE_PARAM_2_PTR_F;
	float nv[3], s, c, t;


	VECNormalize_rnr (v, nv);

	s = sin (HLE_PARAM_F1);
	c = cos (HLE_PARAM_F1);
	t = 1.0f - c;

	m[ 0] = RF ((t * nv[0] * nv[0]) + c);
	m[ 1] = RF ((t * nv[0] * nv[1]) - (s * nv[2]));
	m[ 2] = RF ((t * nv[0] * nv[2]) + (s * nv[1]));
	m[ 3] = 0;

	m[ 4] = RF ((t * nv[0] * nv[1]) + (s * nv[2]));
	m[ 5] = RF ((t * nv[1] * nv[1]) + c);
	m[ 6] = RF ((t * nv[1] * nv[2]) - (s * nv[0]));
	m[ 7] = 0;

	m[ 8] = RF ((t * nv[0] * nv[2]) - (s * nv[1]));
	m[ 9] = RF ((t * nv[1] * nv[2]) + (s * nv[0]));
	m[10] = RF ((t * nv[2] * nv[2]) + c);
	m[11] = 0;

	m[12] = 0;
	m[13] = 0;
	m[14] = 0;
	m[15] = RF_1;
}


HLE (PSMTX44Trans)
{
	float *m = HLE_PARAM_1_PTR_F;


	m[ 0] = RF_1;
	m[ 1] = 0;
	m[ 2] = 0;
	m[ 3] = RF_HLE_PARAM_F1;
	
	m[ 4] = 0;
	m[ 5] = RF_1;
	m[ 6] = 0;
	m[ 7] = RF_HLE_PARAM_F2;
	
	m[ 8] = 0;
	m[ 9] = 0;
	m[10] = RF_1;
	m[11] = RF_HLE_PARAM_F3;

	m[12] = 0;
	m[13] = 0;
	m[14] = 0;
	m[15] = RF_1;
}


HLE (PSMTX44TransApply)
{
	float *src = HLE_PARAM_1_PTR_F, *dst = HLE_PARAM_2_PTR_F;


	if (src != dst)
	{
		dst[ 0] = src[ 0];
		dst[ 1] = src[ 1];
		dst[ 2] = src[ 2];

		dst[ 4] = src[ 4];
		dst[ 5] = src[ 5];
		dst[ 6] = src[ 6];

		dst[ 8] = src[ 8];
		dst[ 9] = src[ 9];
		dst[10] = src[10];

		dst[12] = src[12];
		dst[13] = src[13];
		dst[14] = src[14];
		dst[15] = src[15];
	}

	dst[ 3] = RF (RF (src[ 3]) + HLE_PARAM_F1);
	dst[ 7] = RF (RF (src[ 7]) + HLE_PARAM_F2);
	dst[11] = RF (RF (src[11]) + HLE_PARAM_F3);
}


HLE (PSMTX44Scale)
{
	float *m = HLE_PARAM_1_PTR_F;


	m[ 0] = RF_HLE_PARAM_F1;
	m[ 1] = 0;
	m[ 2] = 0;
	m[ 3] = 0;

	m[ 4] = 0;
	m[ 5] = RF_HLE_PARAM_F2;
	m[ 6] = 0;
	m[ 7] = 0;

	m[ 8] = 0;
	m[ 9] = 0;
	m[10] = RF_HLE_PARAM_F3;
	m[11] = 0;

	m[12] = 0;
	m[13] = 0;
	m[14] = 0;
	m[15] = RF_1;
}


HLE (PSMTX44ScaleApply)
{
	float *src = HLE_PARAM_1_PTR_F, *dst = HLE_PARAM_2_PTR_F;


	dst[ 0] = RF (RF (src[ 0]) * HLE_PARAM_F1);
	dst[ 1] = RF (RF (src[ 1]) * HLE_PARAM_F1);
	dst[ 2] = RF (RF (src[ 2]) * HLE_PARAM_F1);
	dst[ 3] = RF (RF (src[ 3]) * HLE_PARAM_F1);

	dst[ 4] = RF (RF (src[ 4]) * HLE_PARAM_F2);
	dst[ 5] = RF (RF (src[ 5]) * HLE_PARAM_F2);
	dst[ 6] = RF (RF (src[ 6]) * HLE_PARAM_F2);
	dst[ 7] = RF (RF (src[ 7]) * HLE_PARAM_F2);

	dst[ 8] = RF (RF (src[ 8]) * HLE_PARAM_F3);
	dst[ 9] = RF (RF (src[ 9]) * HLE_PARAM_F3);
	dst[10] = RF (RF (src[10]) * HLE_PARAM_F3);
	dst[11] = RF (RF (src[11]) * HLE_PARAM_F3);

	dst[12] = src[12];
	dst[13] = src[13];
	dst[14] = src[14];
	dst[15] = src[15];
}


HLE (sinf)
{
	HLE_RETURN_F (sin (HLE_PARAM_F1));
}


HLE (cosf)
{
	HLE_RETURN_F (cos (HLE_PARAM_F1));
}


HLE (tanf)
{
	HLE_RETURN_F (tan (HLE_PARAM_F1));
}


HLE (asinf)
{
	HLE_RETURN_F (asin (HLE_PARAM_F1));
}


HLE (acosf)
{
	HLE_RETURN_F (acos (HLE_PARAM_F1));
}


HLE (atanf)
{
	HLE_RETURN_F (atan (HLE_PARAM_F1));
}

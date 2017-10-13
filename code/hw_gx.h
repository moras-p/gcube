#ifndef __HW_GX_H
#define __HW_GX_H 1


#include "general.h"
#include "mem.h"
#include "hw_cp.h"
#include "gx.h"
#include "gx_texture.h"
#include "gx_transform.h"

//#define LOG_FAKE_SENDS							1

#ifdef LOG_FAKE_SENDS
# define LOG_FAKE										DEBUG
#else
# define LOG_FAKE(X,format,...)			({})
#endif

#define GPOPCODE(X)										static __s32 X (__u32 mem)

#define GX_QUADS											GL_QUADS
#define GX_TRIANGLES									GL_TRIANGLES
#define GX_TRIANGLE_STRIP							GL_TRIANGLE_STRIP
#define GX_TRIANGLE_FAN								GL_TRIANGLE_FAN
#define GX_LINES											GL_LINES
#define GX_LINE_STRIP									GL_LINE_STRIP
#define GX_POINTS											GL_POINTS

#define CP_VCD_LO											(CP (0x50))
#define CP_VCD_HI											(CP (0x60))
#define CP_VAT_A(X)										(CP (0x70 + X))
#define CP_VAT_B(X)										(CP (0x80 + X))
#define CP_VAT_C(X)										(CP (0x90 + X))
					
#define VCD_COL1											((CP_VCD_LO >> 15) & 3)
#define VCD_COL0											((CP_VCD_LO >> 13) & 3)
#define VCD_NORMAL										((CP_VCD_LO >> 11) & 3)
#define VCD_POS												((CP_VCD_LO >>  9) & 3)
#define VCD_MIDX											(CP_VCD_LO & 0x01ff)

#define VCD_TEX7											((CP_VCD_HI >> 14) & 3)
#define VCD_TEX6											((CP_VCD_HI >> 12) & 3)
#define VCD_TEX5											((CP_VCD_HI >> 10) & 3)
#define VCD_TEX4											((CP_VCD_HI >>  8) & 3)
#define VCD_TEX3											((CP_VCD_HI >>  6) & 3)
#define VCD_TEX2											((CP_VCD_HI >>  4) & 3)
#define VCD_TEX1											((CP_VCD_HI >>  2) & 3)
#define VCD_TEX0											((CP_VCD_HI >>  0) & 3)

#define VCD_T7MIDX										(CP_VCD_LO & 0x100)
#define VCD_T6MIDX										(CP_VCD_LO & 0x080)
#define VCD_T5MIDX										(CP_VCD_LO & 0x040)
#define VCD_T4MIDX										(CP_VCD_LO & 0x020)
#define VCD_T3MIDX										(CP_VCD_LO & 0x010)
#define VCD_T2MIDX										(CP_VCD_LO & 0x008)
#define VCD_T1MIDX										(CP_VCD_LO & 0x004)
#define VCD_T0MIDX										(CP_VCD_LO & 0x002)
#define VCD_PMIDX											(CP_VCD_LO & 0x001)

#define VAT_BYTE_DEQUANT(X)						((CP_VAT_A(X) >> 30) & 0x01)
#define VAT_TEX0_SHFT(X)							((CP_VAT_A(X) >> 25) & 0x1f)
#define VAT_TEX0_FMT(X)								((CP_VAT_A(X) >> 22) & 0x07)
#define VAT_TEX0_CNT(X)								((CP_VAT_A(X) >> 21) & 0x01)
#define VAT_COL1_FMT(X)								((CP_VAT_A(X) >> 18) & 0x07)
#define VAT_COL1_CNT(X)								((CP_VAT_A(X) >> 17) & 0x01)
#define VAT_COL0_FMT(X)								((CP_VAT_A(X) >> 14) & 0x07)
#define VAT_COL0_CNT(X)								((CP_VAT_A(X) >> 13) & 0x01)
#define VAT_NORMAL_FMT(X)							((CP_VAT_A(X) >> 10) & 0x07)
#define VAT_NORMAL_CNT(X)							((CP_VAT_A(X) >>  9) & 0x01)
#define VAT_POS_SHFT(X)								((CP_VAT_A(X) >>  4) & 0x1f)
#define VAT_POS_FMT(X)								((CP_VAT_A(X) >>  1) & 0x07)
#define VAT_POS_CNT(X)								((CP_VAT_A(X) >>  0) & 0x01)

#define VAT_TEX4_FMT(X)								((CP_VAT_B(X) >> 28) & 0x07)
#define VAT_TEX4_CNT(X)								((CP_VAT_B(X) >> 27) & 0x01)
#define VAT_TEX3_SHFT(X)							((CP_VAT_B(X) >> 22) & 0x1f)
#define VAT_TEX3_FMT(X)								((CP_VAT_B(X) >> 19) & 0x07)
#define VAT_TEX3_CNT(X)								((CP_VAT_B(X) >> 18) & 0x01)
#define VAT_TEX2_SHFT(X)							((CP_VAT_B(X) >> 13) & 0x1f)
#define VAT_TEX2_FMT(X)								((CP_VAT_B(X) >> 10) & 0x07)
#define VAT_TEX2_CNT(X)								((CP_VAT_B(X) >>  9) & 0x01)
#define VAT_TEX1_SHFT(X)							((CP_VAT_B(X) >>  4) & 0x1f)
#define VAT_TEX1_FMT(X)								((CP_VAT_B(X) >>  1) & 0x07)
#define VAT_TEX1_CNT(X)								((CP_VAT_B(X) >>  0) & 0x01)

#define VAT_TEX7_SHFT(X)							((CP_VAT_C(X) >> 27) & 0x1f)
#define VAT_TEX7_FMT(X)								((CP_VAT_C(X) >> 24) & 0x07)
#define VAT_TEX7_CNT(X)								((CP_VAT_C(X) >> 23) & 0x01)
#define VAT_TEX6_SHFT(X)							((CP_VAT_C(X) >> 18) & 0x1f)
#define VAT_TEX6_FMT(X)								((CP_VAT_C(X) >> 15) & 0x07)
#define VAT_TEX6_CNT(X)								((CP_VAT_C(X) >> 14) & 0x01)
#define VAT_TEX5_SHFT(X)							((CP_VAT_C(X) >>  9) & 0x1f)
#define VAT_TEX5_FMT(X)								((CP_VAT_C(X) >>  6) & 0x07)
#define VAT_TEX5_CNT(X)								((CP_VAT_C(X) >>  5) & 0x01)
#define VAT_TEX4_SHFT(X)							((CP_VAT_C(X) >>  0) & 0x1f)

#define XF_PROJECTION_A								(FXF (0x1020))
#define XF_PROJECTION_B								(FXF (0x1021))
#define XF_PROJECTION_C								(FXF (0x1022))
#define XF_PROJECTION_D								(FXF (0x1023))
#define XF_PROJECTION_E								(FXF (0x1024))
#define XF_PROJECTION_F								(FXF (0x1025))
#define XF_PROJECTION_ORTHOGRAPHIC		(XF  (0x1026))

#define XF_VIEWPORT_A									(FXF (0x101a))
#define XF_VIEWPORT_B									(FXF (0x101b))
#define XF_VIEWPORT_C									(FXF (0x101c))
#define XF_VIEWPORT_D									(FXF (0x101d))
#define XF_VIEWPORT_E									(FXF (0x101e))
#define XF_VIEWPORT_F									(FXF (0x101f))

#define XF_INVTXSPEC									(XF  (0x1008))
#define HOST_TEXTURES									((XF_INVTXSPEC >> 4) & 15)
#define HOST_NORMALS									((XF_INVTXSPEC >> 2) &  3)
#define HOST_COLORS										((XF_INVTXSPEC >> 0) &  3)

#define XF_COLORS											(XF (0x1009))
#define XF_TEXTURES										(XF (0x103f))
#define XF_DUALTEXTRANS								(XF (0x1012))

#define XF_AMBIENT0										(XF (0x100a))
#define XF_AMBIENT1										(XF (0x100b))
#define XF_MATERIAL0									(XF (0x100c))
#define XF_MATERIAL1									(XF (0x100d))

#define XF_COLOR_CTRL(X)							(XF (0x100e + X))
#define XF_ALPHA_CTRL(X)							(XF (0x1010 + X))

#define CHN_CTRL_LIGHT_MASK(X)				(((X >> 2) & 0x0f) | ((X >> 7) & 0xf0))
#define CHN_CTRL_DIFFUSE_ATTEN(X)			((X >>  7) & 3)
#define CHN_CTRL_ATTEN_SELECT(X)			((X >> 10) & 1)
#define CHN_CTRL_ATTEN_ENABLE(X)			(X & (1 << 9))
#define CHN_CTRL_USE_LIGHTING(X)			(X & (1 << 1))
#define CHN_CTRL_USE_AMB0(X)					(!(X & (1 << 6)))
#define CHN_CTRL_USE_MAT0(X)					(!(X & (1 << 0)))

#define XF_MATRIX_INDEX_0							(XF  (0x1018))
#define XF_MATRIX_INDEX_1							(XF  (0x1019))

#define XF_TEX(X)											(XF  (0x1040 + X))
#define TC_SOURCE_ROW(X)							((XF_TEX (X) >> 7) & 0x1f)
#define TC_TEXGEN_TYPE(X)							((XF_TEX (X) >> 4) & 7)
#define TC_PROJECTION(X)							(XF_TEX (X) & 2)

// only four in yagcd??
#define XF_DUALTEX(X)									(XF (0x1050 + X))
#define TC_DUALMTX(X)									(XF_DUALTEX (X) & 0x3f)
#define TC_NORMALIZE(X)								(XF_DUALTEX (X) & (1 << 8))

#define CP_MATRIX_INDEX_0							(CP (0x30))
#define CP_MATRIX_INDEX_1							(CP (0x40))

#define MIDX_TEX7											((CP_MATRIX_INDEX_1 >> 18) & 0x3f)
#define MIDX_TEX6											((CP_MATRIX_INDEX_1 >> 12) & 0x3f)
#define MIDX_TEX5											((CP_MATRIX_INDEX_1 >>  6) & 0x3f)
#define MIDX_TEX4											((CP_MATRIX_INDEX_1 >>  0) & 0x3f)
#define MIDX_TEX3											((CP_MATRIX_INDEX_0 >> 24) & 0x3f)
#define MIDX_TEX2											((CP_MATRIX_INDEX_0 >> 18) & 0x3f)
#define MIDX_TEX1											((CP_MATRIX_INDEX_0 >> 12) & 0x3f)
#define MIDX_TEX0											((CP_MATRIX_INDEX_0 >>  6) & 0x3f)
#define MIDX_GEO											((CP_MATRIX_INDEX_0 >>  0) & 0x3f)


#define CP_ARRAY_BASE(X)							(CP (0xa0 + X))
#define CP_ARRAY_STRIDE(X)						(CP (0xb0 + X))
#define CP_VERTEX_ARRAY_BASE					(CP_ARRAY_BASE (0))
#define CP_NORMAL_ARRAY_BASE					(CP_ARRAY_BASE (1))
#define CP_COLOR0_ARRAY_BASE					(CP_ARRAY_BASE (2))
#define CP_COLOR1_ARRAY_BASE					(CP_ARRAY_BASE (3))
#define CP_TEXTURE_ARRAY_BASE(n)			(CP_ARRAY_BASE (4 + n))
#define CP_VERTEX_ARRAY_STRIDE				(CP_ARRAY_STRIDE (0))
#define CP_NORMAL_ARRAY_STRIDE				(CP_ARRAY_STRIDE (1))
#define CP_COLOR0_ARRAY_STRIDE				(CP_ARRAY_STRIDE (2))
#define CP_COLOR1_ARRAY_STRIDE				(CP_ARRAY_STRIDE (3))
#define CP_TEXTURE_ARRAY_STRIDE(n)		(CP_ARRAY_STRIDE (4 + n))

#define BP_LPSIZE											(BP (0x22))
#define LINE_ASPECT										((BP_LPSIZE >> 22) &    1)
#define POINT_TEX_OFFSET							((BP_LPSIZE >> 19) & 0x0f)
#define LINE_TEX_OFFSET								((BP_LPSIZE >> 16) & 0x0f)
#define POINT_SIZE										((BP_LPSIZE >>  8) & 0xff)
#define LINE_SIZE											((BP_LPSIZE >>  0) & 0xff)

#define BP_SSIZE(X)										(BP (0x30 + X*2 + 0))
#define BP_TSIZE(X)										(BP (0x30 + X*2 + 1))
#define TEX_SSIZE(X)									((BP_SSIZE(X) & 0xffff) + 1)
#define TEX_TSIZE(X)									((BP_TSIZE(X) & 0xffff) + 1)

#define BP_SETMODE0(X)								(BP (0x80 | (X & 3) | ((X & 4) << 3)))
#define BP_SETMODE1(X)								(BP (0x84 | (X & 3) | ((X & 4) << 3)))
#define BP_SETIMAGE0(X)								(BP (0x88 | (X & 3) | ((X & 4) << 3)))
#define BP_SETIMAGE1(X)								(BP (0x8c | (X & 3) | ((X & 4) << 3)))
#define BP_SETIMAGE2(X)								(BP (0x90 | (X & 3) | ((X & 4) << 3)))
#define BP_SETIMAGE3(X)								(BP (0x94 | (X & 3) | ((X & 4) << 3)))
#define BP_SETTLUT(X)									(BP (0x98 | (X & 3) | ((X & 4) << 3)))

#define BP_LOADTLUT0									(BP (0x64))
#define BP_LOADTLUT1									(BP (0x65))

#define BP_GENMODE										(BP (0x0))
#define BP_ZMODE											(BP (0x40))
#define BP_CMODE0											(BP (0x41))
#define BP_CMODE1											(BP (0x42))
#define BP_PE_CONTROL									(BP (0x43))

#define BP_EFB_SRC_TOP_LEFT						(BP (0x49))
#define BP_EFB_SRC_WIDTH_HEIGHT				(BP (0x4a))
#define BP_XFB_ADDRESS								(BP (0x4b))

#define BP_COPY_CLEAR_AR							(BP (0x4f))
#define BP_COPY_CLEAR_GB							(BP (0x50))
#define BP_COPY_CLEAR_Z								(BP (0x51))
#define BP_COPY_EXECUTE								(BP (0x52))

#define BP_SCISSORS_TOP_LEFT					(BP (0x20))
#define BP_SCISSORS_BOTTOM_RIGHT			(BP (0x21))
#define BP_SCISSORS_OFFSET						(BP (0x59))

#define BP_TEV_FOG_COLOR							(BP (0xf2))
#define BP_TEV_ALPHAFUNC							(BP (0xf3))

#define BP_MASK												(BP (0xfe))

#define BP_TREF(X)										(BP (0x28 + X))
#define TS_COORD(X)										((BP (0x28 + (X/2)) >> (((X & 1) * 12) + 3)) & 7)

#define CULL_MODE											((BP_GENMODE >> 14) & 3)

#define EFB_SRC_TOP										((BP_EFB_SRC_TOP_LEFT >> 10) & 0x3ff)
#define EFB_SRC_LEFT									((BP_EFB_SRC_TOP_LEFT >>  0) & 0x3ff)
#define EFB_SRC_HEIGHT								((BP_EFB_SRC_WIDTH_HEIGHT >> 10) & 0x3ff)
#define EFB_SRC_WIDTH									((BP_EFB_SRC_WIDTH_HEIGHT >>  0) & 0x3ff)
#define XFB_ADDRESS										(BP_XFB_ADDRESS << 5)

#define ZMODE_UPDATE_ENABLED					(BP_ZMODE & 16)
#define ZMODE_FUNC										((BP_ZMODE >> 1) & 7)
#define ZMODE_ENABLED									(BP_ZMODE & 1)

#define CMODE_CONSTANT_ALPHA_ENABLED	(BP_CMODE1 & 256)
#define CMODE_CONSTANT_ALPHA					((BP_CMODE1 >>  0) & 0xff)
#define CMODE_LOGICOP									((BP_CMODE0 >> 12) & 0xf)
#define CMODE_SUBTRACT								(BP_CMODE0 & (1 << 11))
#define CMODE_SFACTOR									((BP_CMODE0 >>  8) & 7)
#define CMODE_DFACTOR									((BP_CMODE0 >>  5) & 7)
#define CMODE_ALPHA_ENABLED						(BP_CMODE0 & 16)
#define CMODE_COLOR_ENABLED						(BP_CMODE0 &  8)
#define CMODE_DITHER_ENABLED					(BP_CMODE0 &  4)
#define CMODE_LOGICOP_ENABLED					(BP_CMODE0 &  2)
#define CMODE_BLEND_ENABLED						(BP_CMODE0 &  1)

#define TEV_FOG_COLOR_R								((BP_TEV_FOG_COLOR >> 16) & 0xff)
#define TEV_FOG_COLOR_G								((BP_TEV_FOG_COLOR >>  8) & 0xff)
#define TEV_FOG_COLOR_B								((BP_TEV_FOG_COLOR >>  0) & 0xff)

#define TEV_ALPHAFUNC_LOGIC						((BP_TEV_ALPHAFUNC >> 22) & 0x03)
#define TEV_ALPHAFUNC_OP1							((BP_TEV_ALPHAFUNC >> 19) & 0x07)
#define TEV_ALPHAFUNC_OP0							((BP_TEV_ALPHAFUNC >> 16) & 0x07)
#define TEV_ALPHAFUNC_A1							((BP_TEV_ALPHAFUNC >>  8) & 0xff)
#define TEV_ALPHAFUNC_A0							((BP_TEV_ALPHAFUNC >>  0) & 0xff)

#define COPY_CLEAR_A									((BP_COPY_CLEAR_AR >> 8) & 0xff)
#define COPY_CLEAR_R									((BP_COPY_CLEAR_AR >> 0) & 0xff)
#define COPY_CLEAR_G									((BP_COPY_CLEAR_GB >> 8) & 0xff)
#define COPY_CLEAR_B									((BP_COPY_CLEAR_GB >> 0) & 0xff)
#define COPY_CLEAR_Z									(BP_COPY_CLEAR_Z)

#define COPY_EXECUTE_TO_XFB						(BP_COPY_EXECUTE & 0x4000)
#define COPY_EXECUTE_CLEAR_EFB				(BP_COPY_EXECUTE & 0x0800)
#define COPY_EXECUTE_MIPMAP						(BP_COPY_EXECUTE & 0x0200)
#define COPY_EXECUTE_GAMMA						((BP_COPY_EXECUTE >> 7) & 3)
#define COPY_XFB_FORMAT_INTENSITY			(BP_COPY_EXECUTE  & 0x8000)
#define COPY_XFB_FORMAT								((BP_COPY_EXECUTE >> 3) & 0x0f)

#define COPY_Z_BUFFER									(!(BP_PE_CONTROL & (1 << 6)))

#define GX_CTF_R4											(0 << 1)
#define GX_CTF_RA4										(2 << 1)
#define GX_CTF_RA8										(3 << 1)
#define GX_CTF_RGB565									(4 << 1)
#define GX_CTF_RGB5A3									(5 << 1)
#define GX_CTF_RGBA8									(6 << 1)
#define GX_CTF_A8											(7 << 1)

#define GX_CTF_R8											((0 << 1) | 1)
#define GX_CTF_G8											((1 << 1) | 1)
#define GX_CTF_B8											((2 << 1) | 1)
#define GX_CTF_RG8										((3 << 1) | 1)
#define GX_CTF_GB8										((4 << 1) | 1)

// if COPY_XFB_INTENSITY
#define GX_CTF_I4											(0 << 1)
#define GX_CTF_I8											(1 << 1)
#define GX_CTF_IA4										(2 << 1)
#define GX_CTF_IA8										(3 << 1)

// if COPY_Z_BUFFER
#define GX_CTF_Z4											(0 << 1)
#define GX_CTF_Z8											(1 << 1)
#define GX_CTF_Z8M										((1 << 1) | 1)
#define GX_CTF_Z8L										((2 << 1) | 1)
#define GX_CTF_Z16										((3 << 1) | 1)
#define GX_CTF_Z16L										((4 << 1) | 1)

#define INROW_GEOM										0
#define INROW_NORMAL									1
#define INROW_COLORS									2
#define INROW_BINORMAL_T							3
#define INROW_BINORMAL_B							4
#define INROW_TEX0										5
#define INROW_TEX1										6
#define INROW_TEX2										7
#define INROW_TEX3										8
#define INROW_TEX4										9
#define INROW_TEX5										10
#define INROW_TEX6										11
#define INROW_TEX7										12

#define TEXGEN_REGULAR								0
#define TEXGEN_EMBOSS_MAP							1
#define TEXGEN_COLOR_STRGBC0					2
#define TEXGEN_COLOR_STRGBC1					3

#define TEX_FORMAT(X)									(((BP_SETIMAGE0(X) >> 20) & 0x00f))
#define TEX_HEIGHT(X)									(((BP_SETIMAGE0(X) >> 10) & 0x3ff) + 1)
#define TEX_WIDTH(X)									(((BP_SETIMAGE0(X) >>  0) & 0x3ff) + 1)
#define TEX_IMAGE_BASE(X)							((BP_SETIMAGE3(X) & 0x00ffffff) << 5)
#define TEX_ODD_CACHE_HEIGHT(X)				((BP_SETIMAGE2(X) >> 18) & 3)
#define TEX_ODD_CACHE_WIDTH(X)				((BP_SETIMAGE2(X) >> 15) & 3)
#define TEX_ODD_TMEM(X)								((BP_SETIMAGE2(X) & 0x0000ffff) << 5)
#define TEX_IMAGE_TYPE(X)							((BP_SETIMAGE1(X) >> 21) & 1)
#define TEX_EVEN_CACHE_HEIGHT(X)			((BP_SETIMAGE1(X) >> 18) & 3)
#define TEX_EVEN_CACHE_WIDTH(X)				((BP_SETIMAGE1(X) >> 15) & 3)
#define TEX_EVEN_TMEM(X)							((BP_SETIMAGE1(X) & 0x0000ffff) << 5)

#define TLUT_FORMAT(X)								((BP_SETTLUT(X) >> 10) & 0x003)
#define TLUT_TMEM_BASE(X)							(((BP_SETTLUT(X) >>  0) & 0x3ff) << 5)

#define TEX_MODE_MAX_LOD(X)						((BP_SETMODE1(X) >>  8) & 0x0ff)
#define TEX_MODE_MIN_LOD(X)						((BP_SETMODE1(X) >>  0) & 0x0ff)
#define TEX_MODE_LOD_CLAMP(X)					((BP_SETMODE0(X) >> 21) & 0x007)
#define TEX_MODE_MAX_ANISO(X)					((BP_SETMODE0(X) >> 19) & 0x003)
#define TEX_MODE_LOD_BIAS(X)					((BP_SETMODE0(X) >>  9) & 0x3ff)
#define TEX_MODE_DIAGLOAD(X)					((BP_SETMODE0(X) >>  8) & 0x001)
#define TEX_MODE_MIN_FILTER(X)				((BP_SETMODE0(X) >>  5) & 0x007)
#define TEX_MODE_MAG_FILTER(X)				((BP_SETMODE0(X) >>  4) & 0x001)
#define TEX_MODE_WRAP_T(X)						((BP_SETMODE0(X) >>  2) & 0x003)
#define TEX_MODE_WRAP_S(X)						((BP_SETMODE0(X) >>  0) & 0x003)

#define SCISSORS_LEFT									((BP_SCISSORS_TOP_LEFT >> 12) & 0xfff)
#define SCISSORS_TOP									((BP_SCISSORS_TOP_LEFT >>  0) & 0xfff)
#define SCISSORS_RIGHT								((BP_SCISSORS_BOTTOM_RIGHT >> 12) & 0xfff)
#define SCISSORS_BOTTOM								((BP_SCISSORS_BOTTOM_RIGHT >>  0) & 0xfff)
#define SCISSORS_OFFSET_Y							((BP_SCISSORS_OFFSET >> 10) & 0x3ff)
#define SCISSORS_OFFSET_X							((BP_SCISSORS_OFFSET >>  0) & 0x3ff)

// direct formats
#define TEX_FORMAT_I4									0
#define TEX_FORMAT_I8									1
#define TEX_FORMAT_IA4								2
#define TEX_FORMAT_IA8								3
#define TEX_FORMAT_RGB565							4
#define TEX_FORMAT_RGB5A3							5
#define TEX_FORMAT_RGBA8							6
#define TEX_FORMAT_CMP								14

// indexed formats
#define TEX_FORMAT_CI4								8
#define TEX_FORMAT_CI8								9
#define TEX_FORMAT_CI14X2							10

#define TLUT_FORMAT_IA8								0
#define TLUT_FORMAT_RGB565						1
#define TLUT_FORMAT_RGB5A3						2

#define GL_TEXTURE_RECTANGLE					GL_TEXTURE_RECTANGLE_NV


// 0, DIRECT_U8, DIRECT_S8, ..., I8_U8, I8_S8, ..., I16_U8, I16_S8, ...
// tt c fff
#define GXIDX(X,v) ((VCD_##X == 0) ? 0 : \
 ((((VCD_##X - 1) << 4) | (VAT_##X##_CNT(v) << 3) | VAT_##X##_FMT(v)) + 1))

// VCD:
//   0 - no data
//   1 - direct
//   2 - 8 bit index
//   3 - 16 bit index

#define GPOP(X) 						(gpop[X])
#define GPOPR(m,n,X)\
	({\
		int i;\
		for (i = m; i <= n; i++)\
			GPOP (i) = X;\
	})


#define TMEM_SIZE						0x100000
#define TMEM_MASK						0x0fffff
#define TMEM_ADDRESS(X)			(&TMEM ((X) & TMEM_MASK))

#define BP(X)								(gxs.bp[X])
#define CP(X)								(gxs.cp[X])
#define XF(X)								(gxs.xf[(X) & 0xff])
#define FXF(X)							(((float *) gxs.xf)[(X) & 0xff])

#define XFMAT(X)						(((float *) gxs.xfmat)[X])
#define XFMATB(X)						(gxs.xfmat[X])
#define XFMAT_GEO(X)				(((float *) gxs.xfmat)[0x0000 + (X *  4)])
#define XFMAT_TEX(X)				(((float *) gxs.xfmat)[0x0000 + (X *  4)])
#define XFMAT_NORMAL(X)			(((float *) gxs.xfmat)[0x0400 + (X *  3)])
#define XFMAT_DUALTEX(X)		(((float *) gxs.xfmat)[0x0500 + (X *  4)])
#define XFMAT_LIGHT(X)			(((float *) gxs.xfmat)[0x0600 + (X * 16)])
#define TMEM(X)							(gxs.tmem[X])

#define XFMAT_LIGHT_NUM(X)	((X >> 4) & 0x0f)
#define XF_LIGHT(X)					((GXLight *) &XFMAT_LIGHT (X))

#define TEX_DISABLED					0
#define TEX_ENABLED_P2				1
#define TEX_ENABLED						2

#define GX_TOGGLE_WIREFRAME								0
#define GX_TOGGLE_TEX_RELOAD							1
#define GX_TOGGLE_TEX_DUMP								2
#define GX_TOGGLE_FORCE_LINEAR						3
#define GX_TOGGLE_TEX_TRANSPARENT					4
#define GX_TOGGLE_DRAW										5
#define GX_TOGGLE_NO_LOGIC_OPS						6
#define GX_TOGGLE_FULLBRIGHT							7
#define GX_TOGGLE_USE_GL_MIPMAPS					8
#define GX_TOGGLE_FORCE_MAX_ANISO					9
#define GX_TOGGLE_TC_INVALIDATE_ENABLED		10
#define GX_TOGGLE_ENGINE									11
#define GX_TOGGLE_FIX_FLICKERING					20
#define GX_TOGGLE_FIX_BLENDING						21

#define GX_NEAR									0
#define GX_NEAR_MIP_NEAR				1
#define GX_NEAR_MIP_LIN					2
#define GX_LINEAR								4
#define GX_LIN_MIP_NEAR					5
#define GX_LIN_MIP_LIN					6

#define TEV_FOG_PARAM3				(BP (0xf1))
#define TEV_FOG_FSEL					((TEV_FOG_PARAM3 >> 21) & 7)
#define GX_FOG_LINEAR					2

// GX_NEAR is 0, GX_LINEAR is 4, the rest is mipmapped
#define TEX_IS_MIPMAPPED(X)			(TEX_MODE_MIN_FILTER (index) & 3)

// check only main fifo, not pregenerated display lists
#define EXCEEDES_LIST_BOUNDARY(m,s)		((m >= CP_FIFO_END) ? FALSE : ((m & MEM_MASK) + (s) >= CP_FIFO_END))

#ifdef LIL_ENDIAN
# define MASK_ALPHA		0xff000000
#else
# define MASK_ALPHA		0xff
#endif


typedef struct
{
	__u32 reserved[3];
	__u32 color;
	float a[3];
	float k[3];
	union
	{
		struct
		{
			float pos[3];
			float dir[3];
		};
		struct
		{
			// direction and half angle ??
			float sdir[3];
			float half_angle[3];
		};
	};
} GXLight;

typedef struct
{
	__u32 cp[0x100];
	__u32 xf[0x100];
	__u32 bp[0x100];

	// matrix memory
	__u32 xfmat[0x800];

	// texture memory (used for tluts)
	__u8 tmem[TMEM_SIZE];
} GXState;


typedef struct
{
	int wireframe;
	int wireframe_culling;
	int tex_reload;
	int dump_textures;
	int force_linear;
	int tex_transparent;
	int dont_draw;
	int no_logic_ops;
	int sw_fullbright;
	int force_max_aniso;
	int use_gl_mipmaps;
	int tc_invalidate_enabled;
	int new_engine;
	
	// only from command line
	int flicker_fix;
	int blending_fix;
} GXSwitches;


extern GXState gxs;
extern GXSwitches gxswitches;
extern TextureCache texcache;
extern TextureTag *texactive[8];
extern TextureTag tag_render_target;

void gx_init (void);
void gx_reinit (void);
void gx_set_max_anisotropy (float a);
int gx_parse_list (__u32 address, __u32 length);
int gx_switch (int sw);


#endif // __HW_GX_H

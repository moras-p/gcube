#ifndef __HLE_H
#define __HLE_H 1


#include "general.h"
#include "mem.h"
#include "math.h"


#define MAX_MAP_ITEMS				20480

#define HLE(name)									void HLE_##name (MapItem *mi)
#define HLE_FUNCTION(name)				{ #name, HLE_##name, NULL }
#define HLE_FUNCTION_EQ(name,eq)	{ #name, HLE_##eq, NULL }
#define HLE_FUNCTION_NULL					{ NULL, NULL, NULL }

#define EMULATION_LLE				0
#define EMULATION_HLE				1
#define EMULATION_IGNORE		2

#define HLE_PARAM(X)				(GPR[3 + X])
#define HLE_PARAM_PTR(X)		(MEM_L2C_ADDRESS (GPR[3 + X]))

#define HLE_PARAM_1					GPR[3]
#define HLE_PARAM_2					GPR[4]
#define HLE_PARAM_3					GPR[5]
#define HLE_PARAM_4					GPR[6]
#define HLE_PARAM_5					GPR[7]
#define HLE_PARAM_6					GPR[8]
#define HLE_PARAM_7					GPR[9]

#define HLE_PARAM_F1				FPR[ 1]
#define HLE_PARAM_F2				FPR[ 2]
#define HLE_PARAM_F3				FPR[ 3]
#define HLE_PARAM_F4				FPR[ 4]
#define HLE_PARAM_F5				FPR[ 5]
#define HLE_PARAM_F6				FPR[ 6]
#define HLE_PARAM_F7				FPR[ 7]
#define HLE_PARAM_F8				FPR[ 8]
#define HLE_PARAM_F9				FPR[ 9]
#define HLE_PARAM_F10				FPR[10]

#define RF_HLE_PARAM_F1			RF ((float) FPR[ 1])
#define RF_HLE_PARAM_F2			RF ((float) FPR[ 2])
#define RF_HLE_PARAM_F3			RF ((float) FPR[ 3])
#define RF_HLE_PARAM_F4			RF ((float) FPR[ 4])
#define RF_HLE_PARAM_F5			RF ((float) FPR[ 5])
#define RF_HLE_PARAM_F6			RF ((float) FPR[ 6])
#define RF_HLE_PARAM_F7			RF ((float) FPR[ 7])
#define RF_HLE_PARAM_F8			RF ((float) FPR[ 8])
#define RF_HLE_PARAM_F9			RF ((float) FPR[ 9])
#define RF_HLE_PARAM_F10		RF ((float) FPR[10])

#define HLE_PARAM_1_PTR			(MEM_L2C_ADDRESS (GPR[3]))
#define HLE_PARAM_2_PTR			(MEM_L2C_ADDRESS (GPR[4]))
#define HLE_PARAM_3_PTR			(MEM_L2C_ADDRESS (GPR[5]))
#define HLE_PARAM_4_PTR			(MEM_L2C_ADDRESS (GPR[6]))
#define HLE_PARAM_5_PTR			(MEM_L2C_ADDRESS (GPR[7]))
#define HLE_PARAM_6_PTR			(MEM_L2C_ADDRESS (GPR[8]))
#define HLE_PARAM_7_PTR			(MEM_L2C_ADDRESS (GPR[9]))

#define HLE_RETURN(X)				({GPR[3] = (X); return;})
#define HLE_RETURN_F(X)			({FPR[1] = (X); return;})

#define HLE_OPCODE(X)						((1 << 26) | X)
#define HLE_REATACH_OPCODE(X)		((2 << 26) | X)

#define OPCODE_BLR			0x4e800020
#define OPCODE_RFI			0x4c000064
#define OPCODE_NOP			0x60000000

#define HLE_EXECUTE_LLE \
	{\
		int n = MEMR32 (mi->address) & 0x03ffffff;\
		MEM32 (mi->address) = mi->op;\
		MEM32 (mi->address + mi->size - 4) = BSWAP32 (HLE_REATACH_OPCODE (n));\
		PC = mi->address - 4;\
	}


typedef struct MapItemTag
{
	char *name;
	__u32 address, size;
	unsigned int num;
	
	// lle, hle or ignore
	int emulation_status;

	// calculated
	__u32 crc;
	
	// replaced by the HL opcode
	__u32 op;
} MapItem;

typedef struct
{
	char *filename;
	int modified, db_generated;

	// by memory address
	MapItem *items_by_mem[MEM_SIZE >> 2];

	// by number
	MapItem *items_by_num[MAX_MAP_ITEMS];
	int nitems;
} Map;


typedef struct
{
	const char *name;
	void (*execute) (MapItem *mi);
	MapItem *mi;
} HLEFunction;

extern HLEFunction hle_functions[];


// testing
HLE (VIGetCurrentLine);
HLE (OSReport_crippled);

HLE (THPPlayerGetState_ignore_movies);
HLE (DVDOpen_ignore_movies);
HLE (DVDConvertPathToEntrynum_ignore_movies);

HLE (PADRead);
HLE (SIProbe);

// todo: put those into respective include files

// general
HLE (memset);
HLE (memcpy);
HLE (memmove);
HLE (bzero);
HLE (strlen);
HLE (strncpy);
HLE (strcpy);

// math
HLE (sinf);
HLE (cosf);
HLE (tanf);
HLE (asinf);
HLE (acosf);
HLE (atanf);

HLE (PSVECDotProduct);
HLE (PSVECCrossProduct);
HLE (PSVECScale);
HLE (PSVECSquareMag);
HLE (PSVECMag);
HLE (PSVECNormalize);
HLE (PSVECSubtract);
HLE (PSVECAdd);
HLE (PSMTXIdentity);
HLE (PSMTXCopy);
HLE (PSMTXConcat);
HLE (PSMTXTranspose);
HLE (PSMTXInverse);
HLE (PSMTXInvXpose);
HLE (PSMTXRotRad);
HLE (PSMTXRotTrig);
HLE (PSMTXRotAxisRad);
HLE (PSMTXTrans);
HLE (PSMTXTransApply);
HLE (PSMTXScale);
HLE (PSMTXScaleApply);
HLE (PSMTXQuat);
HLE (PSMTXReflect);
HLE (PSMTXLookAt);
HLE (PSMTXLightFrustum);
HLE (PSMTXLightPerspective);
HLE (PSMTXLightOrtho);
HLE (PSMTXMultVec);
HLE (PSMTXMultVecArray);
HLE (PSMTXMultVecSR);
HLE (PSMTXMultVecArraySR);
HLE (PSMTX44MultVec);
HLE (PSMTX44MultVecArray);
HLE (PSMTXFrustum);
HLE (PSMTXPerspective);
HLE (PSMTXOrtho);
HLE (PSMTX44Identity);
HLE (PSMTX44Copy);
HLE (PSMTX44Concat);
HLE (PSMTX44Transpose);
//HLE (PSMTX44Inverse);
HLE (PSMTX44RotRad);
HLE (PSMTX44RotTrig);
HLE (PSMTX44RotAxisRad);
HLE (PSMTX44Trans);
HLE (PSMTX44TransApply);
HLE (PSMTX44Scale);
HLE (PSMTX44ScaleApply);


MapItem *map_item_create (__u32 address, __u32 size, char *name);
void map_item_destroy (MapItem *mi);
MapItem *map_add_item (Map *map, __u32 address, __u32 size, char *name);
void map_remove_item (Map *map, MapItem *mi);
void map_item_modify (MapItem *mi, __u32 address, __u32 size, char *name);
Map *map_create (char *filename);
void map_destroy (Map *map);
Map *map_load_full (char *filename);
Map *map_load (char *filename);
Map *map_extract_db (const char *m);
int map_save (Map *map, char *filename);
MapItem *map_get_item_by_name (Map *map, char *name);
MapItem *map_get_item_by_address (Map *map, __u32 address);
MapItem *map_get_item_containing_address (Map *map, __u32 address);
MapItem *map_get_item_by_num (Map *map, unsigned int num);
MapItem *map_find_item_by_crc (Map *map, __u32 crc, MapItem *start);
void map_item_generate_db (MapItem *mi);
void map_generate_db (Map *map);
int map_save_db (Map *map, char *filename);
Map *map_load_db (char *filename);

Map *map_generate_nameless (void);
void map_generate_names (Map *map, Map *db);

void map_item_emulation (MapItem *mi, int stat, unsigned int hle_num);
int map_item_emulation_hle (MapItem *mi);
int map_item_hle_with (MapItem *mi, const char *name);
void hle (Map *map, int attach);
void hle_execute (__u32 opcode);
void hle_reattach (__u32 opcode);
int hle_function (Map *map, const char *name, int attach);

__u32 hle_opcode (const char *name);


#endif // __HLE_H

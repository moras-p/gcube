
#include "hle.h"


// masking off all immediate values
#if 1
__u32 opcode_mask[64] =
{
	0xffffffff, 0xffffffff, 0xffffffff, 0xffff0000, 0xffffffff, 0xffffffff,
	0xffffffff, 0xffff0000, 0xffff0000, 0xffffffff, 0xffff0000, 0xffff0000,

	0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffffffff, 0xffffffff,
	0xfc000003, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,

	0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
	0xffffffff, 0xffffffff, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,

	0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
	0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,

	0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
	0xffff0000, 0xffff0000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,

	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
};
#else
__u32 opcode_mask[64] =
{
	0xffffffff, 0xffffffff, 0xffffffff, 0xffff0000, 0xffffffff, 0xffffffff,
	0xffffffff, 0xffff0000, 0xffff0000, 0xffffffff, 0xffff0000, 0xffff0000,

	0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffffffff, 0xffffffff,
	0xfc000003, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,

	0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
	0xffffffff, 0xffffffff, 0xffe00000, 0xffe00000, 0xffe00000, 0xffe00000,

	0xffe00000, 0xffe00000, 0xffe00000, 0xffe00000, 0xffe00000, 0xffe00000,
	0xffe00000, 0xffe00000, 0xffe00000, 0xffe00000, 0xffe00000, 0xffe00000,

	0xffe00000, 0xffe00000, 0xffe00000, 0xffe00000, 0xffe00000, 0xffe00000,
	0xffe00000, 0xffe00000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,

	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
};
#endif

HLE (empty)
{
}


HLE (ignore_true)
{
	HLE_RETURN (TRUE);
}


HLE (ignore_false)
{
	HLE_RETURN (FALSE);
}


HLEFunction hle_functions[] =
{
	HLE_FUNCTION (empty),
	HLE_FUNCTION (ignore_true),
	HLE_FUNCTION (ignore_false),

	// testing

	HLE_FUNCTION (VIGetCurrentLine),
	
	HLE_FUNCTION (OSReport_crippled),
	HLE_FUNCTION_EQ (BS2Report, OSReport_crippled),
	HLE_FUNCTION_EQ (DBPrintf, OSReport_crippled),
	HLE_FUNCTION_EQ (__DSP_debug_printf, OSReport_crippled),

	HLE_FUNCTION (DVDOpen_ignore_movies),
	HLE_FUNCTION (DVDConvertPathToEntrynum_ignore_movies),
	HLE_FUNCTION (THPPlayerGetState_ignore_movies),

	HLE_FUNCTION (PADRead),
	HLE_FUNCTION_EQ (PADInit, empty),
	HLE_FUNCTION_EQ (PADReset, ignore_true),
	HLE_FUNCTION_EQ (PADRecalibrate, ignore_true),
	HLE_FUNCTION (SIProbe),

	HLE_FUNCTION_EQ (OSProtectRange, empty),
	HLE_FUNCTION_EQ (DCFlushRange, empty),
	HLE_FUNCTION_EQ (PPCSync, empty),
	
	HLE_FUNCTION_EQ (OSGetResetButtonState, ignore_false),
	HLE_FUNCTION_EQ (OSGetResetCode, ignore_false),
	
	HLE_FUNCTION_EQ (DsetDolbyDelay__FUlUs, empty),
	HLE_FUNCTION_EQ (THPAudioDecode, ignore_false),

	// general
	HLE_FUNCTION (memset),
	HLE_FUNCTION (memcpy),
	HLE_FUNCTION (memmove),
	HLE_FUNCTION (bzero),
	HLE_FUNCTION (strlen),
	HLE_FUNCTION (strncpy),
	HLE_FUNCTION (strcpy),

	// math
	HLE_FUNCTION (sinf),
	HLE_FUNCTION (cosf),
	HLE_FUNCTION (tanf),
	HLE_FUNCTION (asinf),
	HLE_FUNCTION (acosf),
	HLE_FUNCTION (atanf),

	HLE_FUNCTION (PSVECDotProduct),
	HLE_FUNCTION (PSVECCrossProduct),
	HLE_FUNCTION (PSVECScale),
	HLE_FUNCTION (PSVECSquareMag),
	HLE_FUNCTION (PSVECMag),
	HLE_FUNCTION (PSVECNormalize),
	HLE_FUNCTION (PSVECSubtract),
	HLE_FUNCTION (PSVECAdd),

	HLE_FUNCTION (PSMTXIdentity),
	HLE_FUNCTION (PSMTXCopy),
	HLE_FUNCTION (PSMTXConcat),
	HLE_FUNCTION (PSMTXTranspose),
	HLE_FUNCTION (PSMTXInverse),
	HLE_FUNCTION (PSMTXInvXpose),

	HLE_FUNCTION (PSMTXRotRad),
	HLE_FUNCTION_EQ (MTXRotRad, PSMTXRotRad),
	HLE_FUNCTION (PSMTXRotTrig),
	HLE_FUNCTION (PSMTXRotAxisRad),
	HLE_FUNCTION (PSMTXTrans),
	HLE_FUNCTION (PSMTXTransApply),
	HLE_FUNCTION (PSMTXScale),
	HLE_FUNCTION (PSMTXScaleApply),
	HLE_FUNCTION (PSMTXQuat),
	HLE_FUNCTION (PSMTXReflect),
	HLE_FUNCTION (PSMTXLookAt),
	HLE_FUNCTION_EQ (C_MTXLookAt, PSMTXLookAt),
	HLE_FUNCTION (PSMTXLightFrustum),

	HLE_FUNCTION_EQ (C_MTXLightFrustum, PSMTXLightFrustum),
	HLE_FUNCTION (PSMTXLightPerspective),
	HLE_FUNCTION_EQ (C_MTXLightPerspective, PSMTXLightPerspective),
	HLE_FUNCTION (PSMTXLightOrtho),
	HLE_FUNCTION_EQ (C_MTXLightOrtho, PSMTXLightOrtho),

	HLE_FUNCTION (PSMTXMultVec),
	HLE_FUNCTION (PSMTXMultVecArray),
	HLE_FUNCTION (PSMTXMultVecSR),
	HLE_FUNCTION (PSMTXMultVecArraySR),
	HLE_FUNCTION (PSMTX44MultVec),
	HLE_FUNCTION (PSMTX44MultVecArray),
	HLE_FUNCTION_EQ (PSMTX44MultVecSR, PSMTXMultVecSR),
	HLE_FUNCTION_EQ (PSMTX44MultVecArraySR, PSMTXMultVecArraySR),
	HLE_FUNCTION (PSMTXFrustum),
	HLE_FUNCTION_EQ (C_MTXFrustum, PSMTXFrustum),
	HLE_FUNCTION (PSMTXPerspective),
	HLE_FUNCTION_EQ (MTXPerspective, PSMTXPerspective),
	HLE_FUNCTION (PSMTXOrtho),
	HLE_FUNCTION_EQ (C_MTXOrtho, PSMTXOrtho),

	HLE_FUNCTION (PSMTX44Identity),
	HLE_FUNCTION (PSMTX44Copy),
	HLE_FUNCTION (PSMTX44Concat),
	HLE_FUNCTION (PSMTX44Transpose),
//	HLE_FUNCTION (PSMTX44Inverse),
	HLE_FUNCTION (PSMTX44RotRad),
	HLE_FUNCTION (PSMTX44RotTrig),
	HLE_FUNCTION (PSMTX44RotAxisRad),
	HLE_FUNCTION (PSMTX44Trans),
	HLE_FUNCTION (PSMTX44TransApply),
	HLE_FUNCTION (PSMTX44Scale),
	HLE_FUNCTION (PSMTX44ScaleApply),

	HLE_FUNCTION_NULL,
};


MapItem *map_item_create (__u32 address, __u32 size, char *name)
{
	MapItem *mi;


	mi = calloc (1, sizeof (MapItem));
	mi->address = address;
	mi->size = size;

	if (name)
		mi->name = strdup (name);
	
	return mi;	
}


void map_item_destroy (MapItem *mi)
{
	if (!mi)
		return;

	if (mi->name)
		free (mi->name);
	free (mi);
}


MapItem *map_add_item (Map *map, __u32 address, __u32 size, char *name)
{
	MapItem *mi;


	if (!map || (map->nitems == MAX_MAP_ITEMS))
		return NULL;

	mi = map_item_create (address, size, name);
	mi->num = map->nitems++;

	map->items_by_num[mi->num] = mi;
	map->items_by_mem[(address & MEM_MASK) >> 2] = mi;
	
	map->modified = TRUE;
	
	return mi;
}


void map_remove_item (Map *map, MapItem *mi)
{
	if (!map || !mi)
		return;

	map->items_by_num[mi->num] = NULL;
	map->items_by_mem[(mi->address & MEM_MASK) >> 2] = NULL;

	map_item_destroy (mi);

	map->modified = TRUE;
}


void map_item_modify (MapItem *mi, __u32 address, __u32 size, char *name)
{
	if (!mi)
		return;

	mi->address = address;
	mi->size = size;
	free (mi->name);
	mi->name = strdup (name);
}


Map *map_create (char *filename)
{
	Map *map = calloc (1, sizeof (Map));


	if (filename)
		map->filename = strdup (filename);

	return map;
}


void map_destroy (Map *map)
{
	int i;


	if (!map)
		return;

	for (i = 0; i < map->nitems; i++)
		map_item_destroy (map->items_by_num[i]);

	free (map->filename);
	free (map);
}


Map *map_load_full (char *filename)
{
	FILE *f;
	char buff[4096], path[4096], line[4096] = {0};
	__u32 address, size;
	int i = 0;
	Map *map;


	if (!file_exists (filename))
	{
		get_filename (buff, filename);
		sprintf (path, "%s/maps/%s", get_home_dir (), buff);
		if (!file_exists (path))
			return NULL;
		
		filename = path;
	}

	map = map_create (filename);
	f = fopen (filename, "r");

	while (!feof (f) && !strlen (line))
		fscanf (f, " %4096[^\n]\n", line);

	if (!((0 == strncmp (line, ".init section layout", 20)) ||
		  	(0 == strncmp (line, "Link map", 8))))
		return NULL;

	// process only .init and .text sections
	for (i = 0; i < 2; i++)
	{
		while ((0 != strncmp (line, ".init", 5)) &&
					 (0 != strncmp (line, ".text", 5)) && !feof (f))
			fscanf (f, " %4096[^\n]\n", line);

		while (!feof (f))
		{
			fscanf (f, " %4096[^\n]\n", line);

			// next section
			if (*line == '.' || (*line == 'e' && (0 == strncmp (line, "extab", 5))))
				break;

			if ((3 == sscanf (line, " %*x %x %x %*d %4096s %*[^\n]\n", &size, &address, buff)))
				if ((*buff != '.') && (*buff != '@'))
					map_add_item (map, address, size, buff);
		}
	}
	
	fclose (f);

	if (!map->nitems)
	{
		map_destroy (map);
		return NULL;
	}
	
	map->modified = FALSE;
	return map;
}


Map *map_load (char *filename)
{
	FILE *f;
	char buff[4096], line[4096], path[4096];
	__u32 address, size;
	Map *map;
	int gendb = FALSE;


	if ((map = map_load_full (filename)))
		return map;

	if (!file_exists (filename))
	{
		get_filename (buff, filename);
		sprintf (path, "%s/maps/%s", get_home_dir (), buff);
		if (!file_exists (path))
			return NULL;
		
		filename = path;
	}

	map = map_create (filename);
	f = fopen (filename, "r");
	
	while (!feof (f))
	{
		fscanf (f, " %4096[^\n]\n", line);

		// if 17th char is not space, assume its address / name
		if (strlen (line) > 17 && line[17] == ' ')
		{
			if (3 == sscanf (line, "%x %x %4096s\n", &address, &size, buff))
				// sanity check
				if (address >= 0x80000000)
					map_add_item (map, address, size, buff);
		}
		else
		{
			if (2 == sscanf (line, "%x %4096s\n", &address, buff))
				// sanity check
				if (address >= 0x80000000)
					map_add_item (map, address, 0, buff);
				
			gendb = TRUE;
		}
	}
	
	fclose (f);
	
	if (!map->nitems)
	{
		map_destroy (map);
		return NULL;
	}
	
	if (gendb)
		map_generate_db (map);
	else
		map->modified = FALSE;

	return map;
}


int map_save (Map *map, char *filename)
{
	FILE *f;
	MapItem *mi;
	int i;


	if (!map)
		return FALSE;
	else if (!map->modified)
		return TRUE;

	if (!filename)
		filename = map->filename;

	if (!filename)
		return FALSE;

	f = fopen (filename, "w");
	if (!f)
	{
		char path[4096], buff[1024];
		
		get_filename (buff, filename);
		sprintf (path, "%s/maps/%s", get_home_dir (), buff);
		f = fopen (path, "w");
		if (!f)
			return FALSE;

		filename = path;
	}

	for (i = 0; i < map->nitems; i++)
	{
		if ((mi = map->items_by_num[i]))
		{
			if (mi->size)
				fprintf (f, "%.8x %.8x %s\n", mi->address, mi->size, mi->name);
			else
				fprintf (f, "%.8x %s\n", mi->address, mi->name);
		}
	}

	fclose (f);

	map->modified = FALSE;

	return TRUE;
}


MapItem *map_get_item_by_name (Map *map, char *name)
{
	int i;


	if (!map)
		return NULL;

	for (i = 0; i < map->nitems; i++)
		if (map->items_by_num[i] && map->items_by_num[i]->name &&
				(0 == strcmp (map->items_by_num[i]->name, name)))
			return map->items_by_num[i];

	return NULL;
}


MapItem *map_get_item_containing_address (Map *map, __u32 address)
{
	MapItem *mi;
	int i;


	address &= ~0x80000000;

	if (!map || ((address & MEM_MASK) > MEM_SIZE))
		return NULL;

	i = (address & MEM_MASK) >> 2;
	while (!(mi = map->items_by_mem[i--]) && (i >= 0))
		;

	if (mi && (address < (mi->address &~ 0x80000000) + mi->size))
		return mi;
	else
		return NULL;
}


MapItem *map_get_item_by_address (Map *map, __u32 address)
{
	if (map && ((address & MEM_MASK) < MEM_SIZE))
		return map->items_by_mem[(address & MEM_MASK) >> 2];
	else
		return NULL;
}


MapItem *map_get_item_by_num (Map *map, unsigned int num)
{
	if (map)
		return map->items_by_num[num];
	else
		return NULL;
}


MapItem *map_find_item_by_crc (Map *map, __u32 crc, MapItem *start)
{
	int i = 0;


	if (!map)
		return NULL;

	if (start)
		i = start->num + 1;
	
	for (; i < map->nitems; i++)
	{
		if ((map->items_by_num[i]) && (map->items_by_num[i]->crc == crc))
			return map->items_by_num[i];
	}

	return NULL;
}


MapItem *map_find_item_by_crc_and_size (Map *map, __u32 crc, __u32 size, MapItem *start)
{
	int i = 0;


	if (!map)
		return NULL;

	if (start)
		i = start->num + 1;
	
	for (; i < map->nitems; i++)
	{
		if ((map->items_by_num[i]) && (map->items_by_num[i]->crc == crc)
				&& (map->items_by_num[i]->size == size))
			return map->items_by_num[i];
	}

	return NULL;
}


__u32 calc_function_size (__u32 addr)
{
	__u32 start = addr, min_end = 0, t, op;


	while ((op = MEMR32 (addr)))
	{
		// on branch check if destination is outside (func_start, first blr)
		if ((op & 0xfc000000) == 0x40000000)
		{
			if (op & 2)
				t = (__s16) (op & 0xfffc);
			else
				t = addr + (__s16) (op & 0xfffc);

			if (min_end < t)
				min_end = t;
		}
		else if (((op == OPCODE_BLR) || (op == OPCODE_RFI)) && (addr >= min_end))
			break;

		addr += 4;
	}
	
	// odd. nop after rfi is counted as belonging to function.
	if ((op == OPCODE_RFI) && (MEMR32 (addr + 4) == OPCODE_NOP))
		addr += 4;

	return (addr - start + 4);
}


void map_item_generate_db (MapItem *mi)
{
	__u32 addr, op;
	int i, k;


	if (!mi)
		return;
		
	if (!mi->size)
		mi->size = calc_function_size (mi->address);

	if (mi->size > 4)
	{
		mi->crc = crc_setup (32);
		addr = mi->address;

		for (i = 0; i < mi->size; i += 4)
		{
			op = MEMR32 (addr);

			op &= opcode_mask[op >> 26];

			for (k = 0; k < 4; k++, op >>= 8)
				if (op & 0xff)
					mi->crc = crc_iterate (mi->crc, op & 0xff);

			addr += 4;
		}
	}
}


void map_generate_db (Map *map)
{
	int i;


	if (!map)
		return;

	for (i = 0; i < map->nitems; i++)
		map_item_generate_db (map->items_by_num[i]);

	map->db_generated = TRUE;
}


int map_save_db (Map *map, char *filename)
{
	int i;
	MapItem *mi, *t;
	FILE *f;
	char buff[512], path[4096];


	if (!map)
		return FALSE;

	if (!map->db_generated)
		map_generate_db (map);

	if (!filename)
	{
		strcpy (buff, map->filename);
		kill_extension (buff);
		strcat (buff, ".mdb");
		
		filename = buff;
	}

	f = fopen (filename, "w");
	if (!f)
	{
		char fname[1024];
		
		get_filename (fname, filename);
		sprintf (path, "%s/maps/%s", get_home_dir (), fname);
		f = fopen (path, "w");
		if (!f)
			return FALSE;
		
		filename = path;
	}

	for (i = 0; i < map->nitems; i++)
	{
		if (!(mi = map->items_by_num[i]))
			continue;
		
		if (mi->size > 4)
		{
			t = map_find_item_by_crc (map, mi->crc, NULL);
			if (t == mi)
				t = map_find_item_by_crc (map, mi->crc, mi);

			if (!t)
				fprintf (f, "%.8x %.8x %s\n", mi->size, mi->crc, mi->name);
		}
	}
	
	fclose (f);
	return TRUE;
}


Map *map_load_db (char *filename)
{
	FILE *f;
	char name[256], line[1024];
	__u32 size, crc;
	Map *map;
	MapItem *mi;


	if (!file_exists (filename))
		return NULL;

	map = map_create (filename);
	f = fopen (filename, "r");
	if (!f)
	{
		char path[4096], buff[1024];
	
		get_filename (buff, filename);
		sprintf (path, "%s/maps/%s", get_home_dir (), buff);
		f = fopen (path, "r");
		if (!f)
			return NULL;
		
		filename = path;
	}
	
	while (!feof (f))
	{
		fscanf (f, " %[^\n]\n", line);

		if (3 == sscanf (line, "%x %x %s\n", &size, &crc, name))
		{
			mi = map_add_item (map, 0, size, name);
			mi->crc = crc;
		}
	}
	
	fclose (f);
	
	map->modified = FALSE;
	return map;
}


// extract database from buffer
Map *map_extract_db (const char *m)
{
	char name[256], line[1024];
	__u32 size, crc;
	Map *map;
	MapItem *mi;
	unsigned int p = 0, t, len;


	if (!m)
		return NULL;

	map = map_create (NULL);
	
	len = strlen (m);
	while (p < len)
	{
		sscanf (&m[p], " %[^\n]\n%n", line, &t);
		p += t;

		if (3 == sscanf (line, "%x %x %s\n", &size, &crc, name))
		{
			mi = map_add_item (map, 0, size, name);
			mi->crc = crc;
		}
	}
	
	map->modified = FALSE;
	map->db_generated = TRUE;
	return map;
}


__u32 find_function (__u32 mem_start, __u32 mem_end, __u32 *start, __u32 *end)
{
	__u32 addr = mem_start;


	while ((addr < mem_end) &&
				 (MEMR32 (addr) != OPCODE_BLR) && (MEMR32 (addr) != OPCODE_RFI))
		addr += 4;
	*end = addr + 4;

	while (MEMR32 (addr - 4) && (addr > mem_start))
		addr -= 4;
	*start = addr;

	if ((*end - *start == 4) && (*end >= mem_end))
		return 0;
	else
		return (*end - *start);
}


Map *map_generate_nameless (void)
{
	__u32 start, end, size;
	Map *map;


	map = map_create (NULL);

	if ((find_function (0x00003100, MEM_SIZE, &start, &end)))
	{
		size = calc_function_size (start);
		map_add_item (map, start | 0x80000000, size, NULL);
		end = start + size;
	}
	else
		return NULL;

	while ((find_function (end, MEM_SIZE, &start, &end)))
	{
		size = calc_function_size (start);
		map_add_item (map, start | 0x80000000, size, NULL);
		end = start + size;
		DEBUG (EVENT_LOG, "found function: %.8x %.8x", start | 0x80000000, size);
	}

	map_generate_db (map);
	return map;
}


MapItem *map_find_item_by_crc_unnamed (Map *map, __u32 crc, MapItem *start)
{
	MapItem *mf;


	do
	{
		mf = map_find_item_by_crc (map, crc, start);
		start = mf;
	} while (mf && mf->name);

	return mf;
}


void map_generate_names (Map *map, Map *db)
{
	int i;
	MapItem *mi, *mf;


	// remove doubled
	for (i = 0; i < map->nitems; i++)
	{
		if ((mi = map->items_by_num[i]) && !mi->name)
		{
			mf = map_find_item_by_crc_unnamed (map, mi->crc, NULL);
			if (mf == mi)
				mf = map_find_item_by_crc_unnamed (map, mi->crc, mi);

			if (mf)
			{
				while (mf)
				{
					map_remove_item (map, mf);

					mf = map_find_item_by_crc_unnamed (map, mi->crc, NULL);
					if (mf == mi)
						mf = map_find_item_by_crc_unnamed (map, mi->crc, mi);
				}
				
				map_remove_item (map, mi);
			}
		}
	}

	for (i = 0; i < map->nitems; i++)
		if ((mi = map->items_by_num[i]) && !mi->name)
			if ((mf = map_find_item_by_crc_and_size (db, mi->crc, mi->size, NULL)))
			{
				map_item_modify (mi, mi->address, mi->size, mf->name);
				DEBUG (EVENT_DEBUG_MSG, "found: %.8x %.8x %s", mi->address, mi->size, mi->name);
			}
	
	for (i = 0; i < map->nitems; i++)
		if ((mi = map->items_by_num[i]))
		{
			if (!mi->name)
				map_remove_item (map, mi);
		}

	map->modified = TRUE;
}


void map_item_emulation (MapItem *mi, int stat, unsigned int hle_num)
{
	if (!mi)
		return;

	switch (stat)
	{
		case EMULATION_LLE:
			if (mi->emulation_status != EMULATION_LLE)
			{
				if (mi->op)
				{
					MEM32 (mi->address) = mi->op;
					mi->op = 0;
				}
				mi->emulation_status = EMULATION_LLE;
			}				
			break;

		case EMULATION_HLE:
			if (mi->emulation_status != EMULATION_HLE)
			{
				if (!mi->op)
					mi->op = MEM32 (mi->address);
				MEM32 (mi->address) = BSWAP32 (HLE_OPCODE (hle_num));
				mi->emulation_status = EMULATION_HLE;
			}				
			break;

		case EMULATION_IGNORE:
			if (mi->emulation_status != EMULATION_IGNORE)
			{
				if (!mi->op)
					mi->op = MEM32 (mi->address);
				MEM32 (mi->address) = BSWAP32 (OPCODE_BLR);
				mi->emulation_status = EMULATION_IGNORE;
			}				
			break;
	}
}


int hle_find_by_name (const char *name)
{
	int i = 0;


	while (hle_functions[i].name)
	{
		if (0 == strcmp (hle_functions[i].name, name))
			return i;

		i++;
	}

	return -1;
}


// hle one function using other
int map_item_hle_with (MapItem *mi, const char *name)
{
	int hle_num;


	if (!mi)
		return FALSE;

	hle_num = hle_find_by_name (name);
	if (hle_num == -1)
		return FALSE;

	if (!mi->op)
		mi->op = MEM32 (mi->address);
	MEM32 (mi->address) = BSWAP32 (HLE_OPCODE (hle_num));
	mi->emulation_status = EMULATION_HLE;
	hle_functions[hle_num].mi = mi;

	return TRUE;
}


int map_item_emulation_hle (MapItem *mi)
{
	int i = 0;
	
	
	if (!mi)
		return FALSE;
	
	while (hle_functions[i].name)
	{
		if (0 == strcmp (hle_functions[i].name, mi->name))
		{
			map_item_emulation (mi, EMULATION_HLE, i);
			hle_functions[i].mi = mi;
			return TRUE;
		}

		i++;
	}
	
	return FALSE;
}


void hle (Map *map, int attach)
{
	int i = 0;
	MapItem *mi;
	
	
	while (hle_functions[i].name)
	{
		if ((mi = map_get_item_by_name (map, (char *) hle_functions[i].name)))
		{
			map_item_emulation (mi, attach ? EMULATION_HLE : EMULATION_LLE, i);
			hle_functions[i].mi = mi;
		}

		i++;
	}
}


int hle_function (Map *map, const char *name, int attach)
{
	int i;
	MapItem *mi;
	
	
	mi = map_get_item_by_name (map, (char *) name);
	i = hle_find_by_name (name);
	if (!mi || !i)
		return FALSE;

	map_item_emulation (mi, attach ? EMULATION_HLE : EMULATION_LLE, i);
	hle_functions[i].mi = mi;
	return TRUE;
}


void hle_execute (__u32 opcode)
{
	int n = opcode & 0x03ffffff;


	if (n < sizeof (hle_functions) / sizeof (HLEFunction))
	{
		PC = (LR &~ 3) - 4;
		hle_functions[n].execute (hle_functions[n].mi);
	}
}


void hle_reattach (__u32 opcode)
{
	int n = opcode & 0x03ffffff;


	if (n < sizeof (hle_functions) / sizeof (HLEFunction))
	{
		MEM32 (hle_functions[n].mi->address) = BSWAP32 (HLE_OPCODE (n));
		MEM32 (PC) = BSWAP32 (OPCODE_BLR);

		PC = (LR &~ 3) - 4;
	}
}


__u32 hle_opcode (const char *name)
{
	int hle_num;


	hle_num = hle_find_by_name (name);
	if (hle_num == -1)
		return 0;

	return HLE_OPCODE (hle_num);
}

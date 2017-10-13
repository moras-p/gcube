/*
 *  gcube
 *  Copyright (c) 2004 monk
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 *  layout of the configuration file will change
 *  this is a quick write just to get it working    
 *         
 */

#include "config.h"


Configuration config;


char *config_read_string (const char *cf, const char *key, const char *def)
{
	char *str = NULL, *p;
	char buff[256] = {0};


	p = strstr (cf, key);
	if (p)
	{
		p += strlen (key);
		sscanf (p, " = %255[^\n]\n", buff);
		str = strdup (buff);
	}
	else if (def)
		str = strdup (def);

	return str;
}


int config_read_int (const char *cf, const char *key, int def)
{
	char *str = config_read_string (cf, key, NULL);
	int d;


	if (str)
	{
		if (1 != sscanf (str, "%i", &d))
			d = def;

		free (str);
		return d;
	}
	else
		return def;
}


const char *constants_str[] =
{
	"off",
	"keyboard1", "keyboard2", "keyboard3", "keyboard4",
	"gamepad1", "gamepad2", "gamepad3", "gamepad4",
};

const int constants_int[] =
{
	INPUT_OFF,
	INPUT_KEYBOARD1, INPUT_KEYBOARD2, INPUT_KEYBOARD3, INPUT_KEYBOARD4,
	INPUT_GAMEPAD1, INPUT_GAMEPAD2, INPUT_GAMEPAD3,	INPUT_GAMEPAD4,
};


int config_parse_int (const char *cf, const char *key, int def)
{
	int d, i;
	char *str;


	str = config_read_string (cf, key, NULL);
	if (!str)
		return def;

	for (i = 0; i < sizeof (constants_int) / sizeof (*constants_int); i++)
		if (0 == strncmp (constants_str[i], str, strlen (constants_str[i])))
		{
			free (str);
			return constants_int[i];
		}

	if (1 == sscanf (str, "%i", &d))
	{
		free (str);
		return d;
	}

	return def;
}


void config_read_int_array (const char *cf, const char *key, int *a, int n)
{
	char *str = config_read_string (cf, key, NULL);
	int k, d, i;
	char *p;


	if (str)
	{
		p = str;
		for (i = 0; i < n; i ++)
		{
			if (0 == sscanf (p, " %i,%n", &d, &k))
				break;
			
			a[i] = d;
			p += k;
		}
		
		free (str);
	}
}


void config_defaults (void)
{
	int default_kbmap[] =
	{
		IKEY_RETURN, IKEY_q, IKEY_w, IKEY_s, IKEY_a, IKEY_z, IKEY_x, IKEY_c,
		IKEY_UP, IKEY_DOWN, IKEY_LEFT, IKEY_RIGHT,
		IKEY_KP8, IKEY_KP5, IKEY_KP4, IKEY_KP6,
		IKEY_HOME, IKEY_END, IKEY_DELETE, IKEY_PAGEDOWN,
	};
	int default_padbmap[] =
	{
		10, 1, 2, 8, 7, 5, 6, 9
	};
	int default_padamap[] =
	{
		2, 1, 4, 3
	};
	int i, n;


	config.device[0] = INPUT_KEYBOARD1;
	memcpy (config.kbmap[0], default_kbmap, KBMAP_SIZE * sizeof (int));
	config.keyboard[0] = 1;

	n = input_gamepads_connected ();
	if (n > 3)
		n = 3;

	for (i = 0; i < n + 1; i++)
	{
		config.device[i + 1] = INPUT_GAMEPAD1 + i;
		config.gamepad[i] = i + 2;
		memcpy (config.padbmap[i], default_padbmap, PADBMAP_SIZE * sizeof (int));
		memcpy (config.padamap[i], default_padamap, PADAMAP_SIZE * sizeof (int));
	}

	config.buffer_size = DEFAULT_BUFFER_SIZE;
	config.texcache_size = DEFAULT_TEXCACHE_SIZE;
}


Configuration *config_load (const char *filename)
{
	char buff[1024];
	char *cf;
	int size, i;
	FILE *f;


	memset (&config, 0, sizeof (Configuration));

	config_defaults ();

	sprintf (buff, "%s/%s", get_home_dir (), filename);
	
	f = fopen (buff, "r");
	if (!f)
	{
		// try current directory
		f = fopen (filename, "r");
		if (!f)
			return &config;
	}
	
	fseek (f, 0, SEEK_END);
	size = ftell (f);
	fseek (f, 0, SEEK_SET);
	
	cf = calloc (1, size + 1);
	fread (cf, 1, size, f);
	fclose (f);

	config.device[0] = config_parse_int (cf, "p1_layout", INPUT_KEYBOARD1);
	config.device[1] = config_parse_int (cf, "p2_layout", INPUT_OFF);
	config.device[2] = config_parse_int (cf, "p3_layout", INPUT_OFF);
	config.device[3] = config_parse_int (cf, "p4_layout", INPUT_OFF);

	for (i = 0; i < 4; i++)
		if (config.device[i])
			SI_CONTROLLER_TYPE (i) = SIDEV_GC_CONTROLLER;

	for (i = 0; i < 4; i++)
		if (config.device[i] >= INPUT_GAMEPAD1 && config.device[i] <= INPUT_GAMEPAD4)
			config.gamepad[config.device[i] - INPUT_GAMEPAD1] = i + 1;

	for (i = 0; i < 4; i++)
		if (config.device[i] >= INPUT_KEYBOARD1 && config.device[i] <= INPUT_KEYBOARD4)
			config.keyboard[config.device[i] - INPUT_KEYBOARD1] = i + 1;

	// keyboard keys mapping:
	// start, a, b, l, r, x, y, z, up, down, left, right, a0-, a0+, a1-, a1+
	config_read_int_array (cf, "keyboard1_map", config.kbmap[0], KBMAP_SIZE);
	config_read_int_array (cf, "keyboard2_map", config.kbmap[1], KBMAP_SIZE);
	config_read_int_array (cf, "keyboard3_map", config.kbmap[2], KBMAP_SIZE);
	config_read_int_array (cf, "keyboard4_map", config.kbmap[3], KBMAP_SIZE);

	// pad button mapping:
	// start, a, b, l, r, x, y, z
	config_read_int_array (cf, "gamepad1_bmap", config.padbmap[0], PADBMAP_SIZE);
	config_read_int_array (cf, "gamepad2_bmap", config.padbmap[1], PADBMAP_SIZE);
	config_read_int_array (cf, "gamepad3_bmap", config.padbmap[2], PADBMAP_SIZE);
	config_read_int_array (cf, "gamepad4_bmap", config.padbmap[3], PADBMAP_SIZE);

	// pad axis mapping:
	// analog1 x, analog1 y, analog2 x, analog2 y
	config_read_int_array (cf, "gamepad1_amap", config.padamap[0], PADAMAP_SIZE);
	config_read_int_array (cf, "gamepad2_amap", config.padamap[1], PADAMAP_SIZE);
	config_read_int_array (cf, "gamepad3_amap", config.padamap[2], PADAMAP_SIZE);
	config_read_int_array (cf, "gamepad4_amap", config.padamap[3], PADAMAP_SIZE);

	// audio
	config.buffer_size = config_read_int (cf, "buffer_size", DEFAULT_BUFFER_SIZE);

	// max memory used for texture cache
	config.texcache_size = config_read_int (cf, "texcache_size", DEFAULT_TEXCACHE_SIZE);

	free (cf);
	
	return &config;
}


void config_write_string (FILE *f, const char *key, const char *value)
{
	fprintf (f, "%s = %s\n", key, value);
}


void config_write_int (FILE *f, const char *key, int value)
{
	fprintf (f, "%s = %d\n", key, value);
}


void config_write_int_array (FILE *f, const char *key, int *array, int size)
{
	int i;


	fprintf (f, "%s = ", key);
	for (i = 0; i < size - 1; i++)
		fprintf (f, "%d, ", array[i]);

	fprintf (f, "%d\n", array[i]);
}


void config_save (const char *filename)
{
	char buff[1024];
	FILE *f;


	sprintf (buff, "%s/%s", get_home_dir (), filename);
	
	f = fopen (buff, "w");
	if (!f)
	{
		perror ("Failed to save config");
		return;
	}

	fprintf (f, "// there are four different layouts for keyboard and gamepad\n\n");
	fprintf (f, "// valid layouts are: off keyboard1-4 gamepad1-4\n");
	
	config_write_string (f, "p1_layout", constants_str[config.device[0]]);
	config_write_string (f, "p2_layout", constants_str[config.device[1]]);
	config_write_string (f, "p3_layout", constants_str[config.device[2]]);
	config_write_string (f, "p4_layout", constants_str[config.device[3]]);

	fprintf (f,	"\n// start, a, b, l, r, x, y, z, up, down, left, right, a0-, a0+, a1-, a1+\n");
	config_write_int_array (f, "keyboard1_map", config.kbmap[0], KBMAP_SIZE);
	config_write_int_array (f, "keyboard2_map", config.kbmap[1], KBMAP_SIZE);
	config_write_int_array (f, "keyboard3_map", config.kbmap[2], KBMAP_SIZE);
	config_write_int_array (f, "keyboard4_map", config.kbmap[3], KBMAP_SIZE);

	fprintf (f,	"\n// start, a, b, l, r, x, y, z\n");
	config_write_int_array (f, "gamepad1_bmap", config.padbmap[0], PADBMAP_SIZE);
	config_write_int_array (f, "gamepad2_bmap", config.padbmap[1], PADBMAP_SIZE);
	config_write_int_array (f, "gamepad3_bmap", config.padbmap[2], PADBMAP_SIZE);
	config_write_int_array (f, "gamepad4_bmap", config.padbmap[3], PADBMAP_SIZE);

	fprintf (f,	"\n// analog1 y, analog1 x, analog2 y, analog2 x\n");
	config_write_int_array (f, "gamepad1_amap", config.padamap[0], PADAMAP_SIZE);
	config_write_int_array (f, "gamepad2_amap", config.padamap[1], PADAMAP_SIZE);
	config_write_int_array (f, "gamepad3_amap", config.padamap[2], PADAMAP_SIZE);
	config_write_int_array (f, "gamepad4_amap", config.padamap[3], PADAMAP_SIZE);

	fprintf (f,	"\n// audio buffer size from 512 to 8192\n");
	fprintf (f,	"// good values: linux 512, windows 8192 (smaller = faster response)\n");
	config_write_int (f, "buffer_size", config.buffer_size);

	fprintf (f,	"\n// max memory used for texture cache (MB)\n");
	config_write_int (f, "texcache_size", config.texcache_size);
	
	fclose (f);
}


Configuration *config_get (void)
{
	return &config;
}

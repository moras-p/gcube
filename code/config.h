#ifndef __CONFIG_H
#define __CONFIG_H


#include <stdio.h>
#include <string.h>

#include "general.h"
#include "keys_sdl.h"
#include "video.h"


#ifndef DEFAULT_BUFFER_SIZE
# define DEFAULT_BUFFER_SIZE 512
#endif

#define DEFAULT_TEXCACHE_SIZE				112

#define DEFAULT_MEMCARD_SIZE				16
#define DEFAULT_MEMCARD_A_NAME			"memcard_a.gmc"
#define DEFAULT_MEMCARD_B_NAME			"memcard_b.gmc"

#define INPUT_OFF						0
#define INPUT_KEYBOARD1			1
#define INPUT_KEYBOARD2			2
#define INPUT_KEYBOARD3			3
#define INPUT_KEYBOARD4			4
#define INPUT_GAMEPAD1			5
#define INPUT_GAMEPAD2			6
#define INPUT_GAMEPAD3			7
#define INPUT_GAMEPAD4			8

#define KBMAP_SIZE					20
#define PADBMAP_SIZE				8
#define PADAMAP_SIZE				4


// for gamepads, digital pad is bound to hat

typedef struct
{
	// input
	int device[4];

	// keyboard mapping
	int kbmap[4][KBMAP_SIZE];
	
	// gamepad button mapping
	int padbmap[4][PADBMAP_SIZE];
	
	// gamepad axis mapping
	int padamap[4][PADAMAP_SIZE];

	// memory cards
	int memcard_a_size;
	int memcard_b_size;
	char *memcard_a_name;
	char *memcard_b_name;

	// audio
	int buffer_size;
	
	// texcache size
	unsigned int texcache_size;

	// map keyboard / gamepad index to player (0 means not mapped)
	int keyboard[4];
	int gamepad[4];
} Configuration;


extern Configuration config;

#define CONFIG_AUDIO_BUFFER_SIZE				(config.buffer_size)


Configuration *config_load (const char *filename);
void config_save (const char *filename);
Configuration *config_get (void);


#endif // __CONFIG_H

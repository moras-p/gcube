
#include "hle.h"

HLE (memset)
{
	memset (HLE_PARAM_1_PTR, HLE_PARAM_2, HLE_PARAM_3);
	HLE_RETURN (HLE_PARAM_1);
}


HLE (memcpy)
{
	memcpy (HLE_PARAM_1_PTR, HLE_PARAM_2_PTR, HLE_PARAM_3);
	HLE_RETURN (HLE_PARAM_1);
}


HLE (memmove)
{
	memmove (HLE_PARAM_1_PTR, HLE_PARAM_2_PTR, HLE_PARAM_3);
	HLE_RETURN (HLE_PARAM_1);
}


HLE (bzero)
{
	memset (HLE_PARAM_1_PTR, 0, HLE_PARAM_2);
}


HLE (strlen)
{
	HLE_RETURN (strlen ((const char *) HLE_PARAM_1_PTR));
}


HLE (strncpy)
{
	strncpy ((char *) HLE_PARAM_1_PTR, (const char *) HLE_PARAM_2_PTR, HLE_PARAM_3);
	HLE_RETURN (HLE_PARAM_1);
}


HLE (strcpy)
{
	strcpy ((char *) HLE_PARAM_1_PTR, (const char *) HLE_PARAM_2_PTR);
	HLE_RETURN (HLE_PARAM_1);
}


#include "hw_si.h"
extern PADStatus pads[4];
HLE (PADRead)
{
	PADStatus *status = (PADStatus *) HLE_PARAM_1_PTR;
	int i;


	for (i = 0; i < 4; i++)
	{
		if (SI_CONTROLLER_TYPE (i) == SIDEV_GC_CONTROLLER)
		{
			memcpy (&status[i], &pads[i], sizeof (PADStatus));
			status[i].buttons = BSWAP16 (pads[i].buttons);
			status[i].err = PAD_ERR_NONE;
		}
		else
		{
			memset (&status[i], 0, sizeof (PADStatus));
			status[i].err = PAD_ERR_NO_CONTROLLER;
		}
	}

	// is motor present
	HLE_RETURN (FALSE);
}


HLE (SIProbe)
{
	HLE_RETURN (SI_CONTROLLER_TYPE (HLE_PARAM_1));
}


const char *movie_ext[] =
{
	"thp", "h4m", "str", "bik",
};


int is_movie_file (const char *name)
{
	unsigned int i, len = strlen (name);


	if (len > 4)
		for (i = 0; i < sizeof (movie_ext) / sizeof (*movie_ext); i++)
			if (0 == strcasecmp (movie_ext[i], &name[len - 3]))
				return TRUE;
	
	return FALSE;
}


HLE (DVDOpen_ignore_movies)
{
	if (is_movie_file ((const char *) HLE_PARAM_1_PTR))
		HLE_RETURN (0);
	
	HLE_EXECUTE_LLE;
}


HLE (DVDConvertPathToEntrynum_ignore_movies)
{
	if (is_movie_file ((const char *) HLE_PARAM_1_PTR))
		HLE_RETURN ((__u32) -1);

	HLE_EXECUTE_LLE;
}


HLE (THPPlayerGetState_ignore_movies)
{
	// movie ended
	HLE_RETURN (3);
}


// hack for games with crippled OSReport
// not all params will be passed. won't work with fp params.
HLE (OSReport_crippled)
{
// problems with 64 bit code ((void *) is 64 bit)
	static char buff[4096];
	const char *format = (const char *) HLE_PARAM_1_PTR, *p;
	int len, i = 0;
	__u32 params[128];


	len = strlen (format);
	p = format;

	params[i++] = (__u32) HLE_PARAM_1_PTR;
	while (p < format + len)
	{
		if (p[0] == '%')
		{
			if (p[1] == 's')
				params[i] = (__u32) HLE_PARAM_PTR (i);
			else
				params[i] = HLE_PARAM (i);
			
			i++;
		}

		p++;
	}

	sprintf (buff, (char *) params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7], params[8], params[9], params[10], params[11]);
	gcube_os_report (buff, FALSE);
}


HLE (VIGetCurrentLine)
{
	static float current_line = 0;


	current_line += 50;
	if (current_line >= 575)
		current_line = 0;

	HLE_RETURN ((__u32) current_line);
}

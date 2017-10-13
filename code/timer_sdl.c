
#include "timer.h"
#include <SDL/SDL.h>


unsigned int gtimer_ms = 0;
// for fps calcs
unsigned int gtimer_swaps = 0, gtimer_last_swap_ms = 0;
float gtimer_fps = 0;

void timer_update (void)
{
	gtimer_ms = SDL_GetTicks ();
}


void timer_delay (unsigned int ms)
{
	SDL_Delay (ms);
}


// delay ms miliseconds since last update
void timer_delay_diff (unsigned int ms)
{
	unsigned int diff = SDL_GetTicks () - gtimer_ms;


	if (diff < ms)
	{
		ms -= diff;
		SDL_Delay (ms);
	}
}


float timer_count_fps (void)
{
	unsigned int t = SDL_GetTicks ();

	
	gtimer_swaps++;
	if (t - gtimer_last_swap_ms > 1000)
	{
		gtimer_fps = (float) gtimer_swaps / (0.001 * (t - gtimer_last_swap_ms));

		gtimer_swaps = 0;
		gtimer_last_swap_ms = t;
	}

	return gtimer_fps;
}



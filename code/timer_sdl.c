
#include "timer.h"
#include <SDL/SDL.h>


unsigned int gtimer_ms = 0;

// for fps calcs
unsigned int gtimer_swaps = 0, gtimer_last_swap_ms = 0;
float gtimer_fps = 0;

unsigned int timer_ticks (void)
{
	return SDL_GetTicks ();
}


void timer_delay (unsigned int ms)
{
	SDL_Delay (ms);
}


void timer_update (void)
{
	gtimer_ms = timer_ticks ();
}


// delay ms miliseconds since last update
void timer_delay_diff (unsigned int ms)
{
	unsigned int diff = timer_ticks () - gtimer_ms;


	if (diff < ms)
	{
		ms -= diff;
		timer_delay (ms);
	}
}


float timer_count_fps (void)
{
	unsigned int t = timer_ticks ();


	gtimer_swaps++;
	if (t - gtimer_last_swap_ms > 1000)
	{
		gtimer_fps = (float) gtimer_swaps / (0.001 * (t - gtimer_last_swap_ms));

		gtimer_swaps = 0;
		gtimer_last_swap_ms = t;
	}

	return gtimer_fps;
}

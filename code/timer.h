#ifndef __TIMER_H
#define __TIMER_H 1


unsigned int timer_ticks (void);
void timer_delay (unsigned int ms);
void timer_update (void);
void timer_delay_diff (unsigned int ms);
float timer_count_fps (void);


#endif // __TIMER_H

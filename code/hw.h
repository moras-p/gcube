#ifndef __HW_H
#define __HW_H 1


#include <stdio.h>
#include <string.h>

#include "general.h"
#include "hw_ai_dsp.h"
#include "hw_cp.h"
#include "hw_di.h"
#include "hw_exi.h"
#include "hw_gx.h"
#include "hw_mi.h"
#include "hw_pe.h"
#include "hw_pi.h"
#include "hw_si.h"
#include "hw_vi.h"

#include "mem.h"

void hw_init (void);
void hw_reinit (void);
void hw_check_interrupts ();
void hw_set_video_mode (int country_code);

int hw_rword (__u32 address, __u32 *data);
int hw_rhalf (__u32 address, __u16 *data);
int hw_wword (__u32 address, __u32 data);
int hw_whalf (__u32 address, __u16 data);

extern int ref_delay;
void hw_update (void);
void hw_force_vi_refresh (void);


#endif // __HW_H

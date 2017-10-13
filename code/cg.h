#ifndef __CG_H
#define __CG_H 1


#include <Cg/cgGL.h>
#include "general.h"



void cg_enable (int sw);
void cg_cleanup (void);
int cg_init (void);
void cg_fpcache_clear (void);

void cg_params_set_const_ra (int n, float c[2]);
void cg_params_set_const_bg (int n, float c[2]);
void cg_params_set_konst_ra (int n, float c[2]);
void cg_params_set_konst_bg (int n, float c[2]);

void cg_params_set_indm_ab (int n, float c1, float c2);
void cg_params_set_indm_cd (int n, float c1, float c2);
void cg_params_set_indm_ef (int n, float c1, float c2);
void cg_params_set_indscale (int n, float c);
void cg_params_set_aref (int c1, int c2);

void cg_params_set_tus (int n, unsigned int width, unsigned int height);
void cg_bind_program_f (int n);
void cg_unbind_f (void);
int cg_get_program_f (void);


#endif

#ifndef __GX_H
#define __GX_H 1


void gx_set_max_anisotropy (float a);
int gx_get_real_width (void);
int gx_get_real_height (void);
void gx_set_texture_mode0 (int index);
void gx_set_texture_mode1 (int index);
void gx_set_projection (void);
void gx_set_lpsize (void);
void gx_load_tlut (void);
void gx_set_cull_mode (void);
void gx_set_cmode0 (void);
void gx_set_zmode (void);
void gx_set_alphafunc (void);
void gx_set_fog_param3 (void);
void gx_set_fog_color (void);
void gx_set_copy_clear_color (void);
void gx_set_copy_clear_z (void);
void gx_set_scissors (void);
void gx_set_viewport (void);
void gx_set_gamma (void);
void gx_copy_efb (void);
void gx_set_fog (void);
void gx_set_tev_konst_ra (int index);
void gx_set_tev_konst_bg (int index);
void gx_set_tev_indm_ab (int index);
void gx_set_tev_indm_cd (int index);
void gx_set_tev_indm_ef (int index);
void gx_set_tev_indscale (int index);


#endif // __GX_H

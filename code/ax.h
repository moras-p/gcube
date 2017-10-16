#ifndef __AX_H
#define __AX_H 1

#define AXPB_STATE_STOP        0
#define AXPB_STATE_RUN         1

#define AXPB_FORMAT_ADPCM      0
#define AXPB_FORMAT_PCM16      10
#define AXPB_FORMAT_PCM8       25

#define AX_MAX_VOICES               64

#define AX_MS_PER_FRAME          5
#define AX_SAMPLES_PER_MS        32
#define AX_SAMPLES_PER_SEC       (AX_SAMPLES_PER_MS * 1000)
#define AX_SAMPLES_PER_FRAME     (AX_SAMPLES_PER_MS * AX_MS_PER_FRAME)


typedef struct
{                 
	__u16 is_looping;
	__u16 format;
	__u16 loop_address_hi;
	__u16 loop_address_lo;
	__u16 end_address_hi;
	__u16 end_address_lo;
	__u16 cur_address_hi;
	__u16 cur_address_lo;
} AXPBAddress;

typedef struct
{
	__u16 a[8][2];
	__u16 gain;
	__u16 pred_scale;
	__u16 yn1;
	__u16 yn2;
} AXPBAdpcm;


typedef struct
{
	__u16 ratio_hi;
	__u16 ratio_lo;
	__u16 cur_address_frac;
	__u16 last_samples[4];
} AXPBSampleRateConv;

typedef struct
{
	__u16 cur_vol;
	__s16 cur_delta;
} AXPBVolumeEnv;

typedef struct
{
	__u16 left;
	__u16 left_delta;
	__u16 right;
	__u16 right_delta;
	__u16 aux_a_left;
	__u16 aux_a_left_delta;
	__u16 aux_a_right;
	__u16 aux_a_right_delta;
	__u16 aux_b_left;
	__u16 aux_b_left_delta;
	__u16 aux_b_right;
	__u16 aux_b_right_delta;
	__u16 aux_b_surround;
	__u16 aux_b_surround_delta;
	__u16 surround;
	__u16 surround_delta;
	__u16 aux_a_surround;
	__u16 aux_a_surround_delta;
} AXPBMixer;

typedef struct
{
	__u16 enabled;
	__u16 addr_hi;
	__u16 addr_lo;
	__u16 shift_left;
	__u16 shift_right;
	__u16 target_left;
	__u16 target_right;
} AXPBInitialTimeDelay;

typedef struct
{
	__u16 n_updates[5];
	__u16 addr_hi;
	__u16 addr_lo;
} AXPBUpdate;

typedef struct
{
	__s16 left;
	__s16 aux_a_left;
	__s16 aux_b_left;
	__s16 right;
	__s16 aux_a_right;
	__s16 aux_b_right;
	__s16 surround;
	__s16 aux_a_surround;
	__s16 aux_b_surround;
} AXPBDpop;

typedef struct
{
	__u16 pred_scale;
	__u16 yn1;
	__u16 yn2;
} AXPBLoop;

typedef struct
{
	__u16 next_addr_hi;
	__u16 next_addr_lo;
	__u16 cur_addr_hi;
	__u16 cur_addr_lo;
	__u16 src_type;
	__u16 coef_select;
	__u16 mixer_ctrl;
	__u16 state;
	__u16 type;

	AXPBMixer mix;
	AXPBInitialTimeDelay itd;
	AXPBUpdate update;
	AXPBDpop dpop;
	AXPBVolumeEnv ve;
	
	__u16 unused[3];
	
	AXPBAddress addr;
	AXPBAdpcm adpcm;
	AXPBSampleRateConv src;
	AXPBLoop adpcmLoop;
	
	__u16 pad[3];
} AXPB;


#endif // __AX_H

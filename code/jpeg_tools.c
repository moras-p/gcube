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
 *  functions used by thpview for jpeg decompression
 *
 *
 */

#include <stdio.h>
#include <errno.h>
#include <jpeglib.h>
#include <jerror.h>

#define SIZEOF(X) 				((size_t) sizeof (X))

#include "jpeg_tools.h"


// needed for jpeg in-memory decompression
METHODDEF (void) mjpg_empty (j_decompress_ptr cinfo)
{
}


METHODDEF (boolean) mjpg_fill_input_buffer (j_decompress_ptr cinfo)
{
	ERREXIT (cinfo, 0);
	return FALSE;
}


METHODDEF (void) mjpg_skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	if (num_bytes > cinfo->src->bytes_in_buffer)
		ERREXIT (cinfo, 0);
	
	cinfo->src->next_input_byte += num_bytes;
	cinfo->src->bytes_in_buffer -= num_bytes;
}


typedef struct
{
	struct jpeg_source_mgr pub;
} my_source_mgr;


void jpeg_memory_src (j_decompress_ptr cinfo, __u8 *buff, __u32 size)
{
	struct jpeg_source_mgr *smgr;


	if (!cinfo->src)
	// first time initialization
		cinfo->src = (struct jpeg_source_mgr *) (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT, SIZEOF (my_source_mgr));
	
	smgr = &((my_source_mgr *) cinfo->src)->pub;

	smgr->init_source = smgr->term_source = mjpg_empty;
	smgr->fill_input_buffer = mjpg_fill_input_buffer;
	smgr->skip_input_data = mjpg_skip_input_data;
	smgr->resync_to_restart = jpeg_resync_to_restart;

	smgr->next_input_byte = buff;
	smgr->bytes_in_buffer = size;
}


void jpeg_decompress (__u8 *src, __u32 size, char *dst, int pitch)
{
	JSAMPROW row[1];
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;


	cinfo.err = jpeg_std_error (&jerr);
	jpeg_create_decompress (&cinfo);

	jpeg_memory_src (&cinfo, src, size);
	jpeg_read_header (&cinfo, TRUE);
	cinfo.out_color_space = JCS_RGB;
//	cinfo.out_color_space = JCS_YCbCr;
	cinfo.quantize_colors = FALSE;

#if 0
// for faster loading
	cinfo.scale_num = 1;
	cinfo.scale_denom = 1;
	cinfo.dct_method = JDCT_FASTEST;
	cinfo.do_fancy_upsampling = FALSE;
	cinfo.do_fancy_smoothing = FALSE;
#endif

	jpeg_start_decompress (&cinfo);
	while (cinfo.output_scanline < cinfo.output_height)
	{
		*row = (JSAMPROW) ((void *) dst + cinfo.output_scanline * pitch);
		jpeg_read_scanlines (&cinfo, row, 1);
	}
	jpeg_finish_decompress (&cinfo);

	jpeg_destroy_decompress (&cinfo);
}


struct jpeg_decompress_struct cinfo;
struct jpeg_error_mgr jerr;
void jpeg_decompress_start (__u8 *src, __u32 size)
{
	cinfo.err = jpeg_std_error (&jerr);
	jpeg_create_decompress (&cinfo);

	jpeg_memory_src (&cinfo, src, size);
	jpeg_read_header (&cinfo, FALSE);
	cinfo.image_width = 512;
	cinfo.image_height = 416;
}


void jpeg_memory_src_reset (j_decompress_ptr cinfo, __u8 *buff, __u32 size)
{
	struct jpeg_source_mgr *smgr;


	smgr = &((my_source_mgr *) cinfo->src)->pub;

	smgr->next_input_byte = buff;
	smgr->bytes_in_buffer = size;
}


void jpeg_decompress_continue (__u8 *src, __u32 size, char *dst, int pitch, int color_space)
{
	JSAMPROW row[1];
	J_COLOR_SPACE cspace[] =
	{
		JCS_GRAYSCALE, JCS_RGB, JCS_YCbCr,
	};

	jpeg_memory_src_reset (&cinfo, src, size);
	jpeg_read_header (&cinfo, TRUE);

	cinfo.out_color_space = cspace[color_space];

#if 1
// for faster loading
	cinfo.dct_method = JDCT_FASTEST;
	cinfo.do_fancy_upsampling = FALSE;
	cinfo.do_block_smoothing = FALSE;
#endif


	jpeg_start_decompress (&cinfo);
	while (cinfo.output_scanline < cinfo.output_height)
	{
		row[0] = (JSAMPROW) ((void *) dst + cinfo.output_scanline * pitch);
		jpeg_read_scanlines (&cinfo, row, 1);
	}
	jpeg_finish_decompress (&cinfo);
}

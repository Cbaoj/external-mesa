
/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * \brief  Public interface into the drawing module.
 */

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */


#ifndef DRAW_CONTEXT_H
#define DRAW_CONTEXT_H


#include "pipe/p_state.h"


struct pipe_context;
struct vertex_buffer;
struct vertex_info;
struct draw_context;
struct draw_stage;
struct draw_vertex_shader;


/**
 * Clipmask flags
 */
/*@{*/
#define CLIP_RIGHT_BIT   0x01
#define CLIP_LEFT_BIT    0x02
#define CLIP_TOP_BIT     0x04
#define CLIP_BOTTOM_BIT  0x08
#define CLIP_NEAR_BIT    0x10
#define CLIP_FAR_BIT     0x20
/*@}*/

/**
 * Bitshift for each clip flag
 */
/*@{*/
#define CLIP_RIGHT_SHIFT 	0
#define CLIP_LEFT_SHIFT 	1
#define CLIP_TOP_SHIFT  	2
#define CLIP_BOTTOM_SHIFT       3
#define CLIP_NEAR_SHIFT  	4
#define CLIP_FAR_SHIFT  	5
/*@}*/


struct draw_context *draw_create( void );

void draw_destroy( struct draw_context *draw );

void draw_set_viewport_state( struct draw_context *draw,
                              const struct pipe_viewport_state *viewport );

void draw_set_clip_state( struct draw_context *pipe,
                          const struct pipe_clip_state *clip );

void draw_set_rasterizer_state( struct draw_context *draw,
                                const struct pipe_rasterizer_state *raster );

void draw_set_rasterize_stage( struct draw_context *draw,
                               struct draw_stage *stage );

void draw_wide_point_threshold(struct draw_context *draw, float threshold);

void draw_convert_wide_points(struct draw_context *draw, boolean enable);

void draw_convert_wide_lines(struct draw_context *draw, boolean enable);

boolean draw_use_sse(struct draw_context *draw);

void
draw_install_aaline_stage(struct draw_context *draw, struct pipe_context *pipe);

void
draw_install_aapoint_stage(struct draw_context *draw, struct pipe_context *pipe);

void
draw_install_pstipple_stage(struct draw_context *draw, struct pipe_context *pipe);


int
draw_find_vs_output(struct draw_context *draw,
                    uint semantic_name, uint semantic_index);

uint
draw_num_vs_outputs(struct draw_context *draw);



/*
 * Vertex shader functions
 */

struct draw_vertex_shader *
draw_create_vertex_shader(struct draw_context *draw,
                          const struct pipe_shader_state *shader);
void draw_bind_vertex_shader(struct draw_context *draw,
                             struct draw_vertex_shader *dvs);
void draw_delete_vertex_shader(struct draw_context *draw,
                               struct draw_vertex_shader *dvs);



/*
 * Vertex data functions
 */

void draw_set_vertex_buffer(struct draw_context *draw,
			    unsigned attr,
			    const struct pipe_vertex_buffer *buffer);

void draw_set_vertex_element(struct draw_context *draw,
			     unsigned attr,
			     const struct pipe_vertex_element *element);

void draw_set_mapped_element_buffer( struct draw_context *draw,
                                     unsigned eltSize, void *elements );

void draw_set_mapped_vertex_buffer(struct draw_context *draw,
                                   unsigned attr, const void *buffer);

void draw_set_mapped_constant_buffer(struct draw_context *draw,
                                     const void *buffer);


/***********************************************************************
 * draw_prim.c 
 */

void draw_arrays(struct draw_context *draw, unsigned prim,
		 unsigned start, unsigned count);

void draw_flush(struct draw_context *draw);

/***********************************************************************
 * draw_debug.c 
 */
boolean draw_validate_prim( unsigned prim, unsigned length );
unsigned draw_trim_prim( unsigned mode, unsigned count );



#endif /* DRAW_CONTEXT_H */

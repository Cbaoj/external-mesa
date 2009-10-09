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
#ifndef LP_SETUP_H
#define LP_SETUP_H

#include "pipe/p_compiler.h"

enum lp_interp {
   LP_INTERP_CONSTANT,
   LP_INTERP_LINEAR,
   LP_INTERP_PERSPECTIVE,
   LP_INTERP_POSITION,
   LP_INTERP_FACING
};

/* Describes how to generate all the fragment shader inputs from the
 * the vertices passed into our triangle/line/point functions.
 *
 * Vertices are treated as an array of float[4] values, indexed by
 * src_index.
 */
struct lp_shader_input {
   enum lp_interp interp;       /* how to interpolate values */
   unsigned src_index;          /* where to find values in incoming vertices */
};

struct pipe_texture;
struct pipe_surface;
struct setup_context;
struct lp_jit_context;

struct setup_context *
lp_setup_create( void );

void
lp_setup_clear(struct setup_context *setup,
               const float *clear_color,
               double clear_depth,
               unsigned clear_stencil,
               unsigned flags);

void
lp_setup_tri(struct setup_context *setup,
             const float (*v0)[4],
             const float (*v1)[4],
             const float (*v2)[4]);

void
lp_setup_line(struct setup_context *setup,
              const float (*v0)[4],
              const float (*v1)[4]);

void
lp_setup_point( struct setup_context *setup,
                const float (*v0)[4] );


void
lp_setup_flush( struct setup_context *setup,
                unsigned flags );


void
lp_setup_bind_framebuffer( struct setup_context *setup,
                           struct pipe_surface *color,
                           struct pipe_surface *zstencil );

void 
lp_setup_set_triangle_state( struct setup_context *setup,
                             unsigned cullmode,
                             boolean front_is_ccw );

void
lp_setup_set_fs_inputs( struct setup_context *setup,
                        const struct lp_shader_input *interp,
                        unsigned nr );

void
lp_setup_set_shader_state( struct setup_context *setup,
                           const struct lp_jit_context *jc );

boolean
lp_setup_is_texture_referenced( struct setup_context *setup,
                                const struct pipe_texture *texture );


void 
lp_setup_destroy( struct setup_context *setup );

#endif

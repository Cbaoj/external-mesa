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

#include "pipe/p_util.h"
#include "cell_context.h"
#include "cell_state.h"
#include "cell_state_emit.h"
#include "cell_batch.h"



static void
emit_state_cmd(struct cell_context *cell, uint cmd,
               const void *state, uint state_size)
{
   uint *dst = (uint *) cell_batch_alloc(cell, sizeof(uint) + state_size);
   *dst = cmd;
   memcpy(dst + 1, state, state_size);
}



void
cell_emit_state(struct cell_context *cell)
{
   if (cell->dirty & CELL_NEW_FRAMEBUFFER) {
      struct pipe_surface *cbuf = cell->framebuffer.cbufs[0];
      struct pipe_surface *zbuf = cell->framebuffer.zsbuf;
      struct cell_command_framebuffer *fb
         = cell_batch_alloc(cell, sizeof(*fb));
      fb->opcode = CELL_CMD_STATE_FRAMEBUFFER;
      fb->color_start = cell->cbuf_map[0];
      fb->color_format = cbuf->format;
      fb->depth_start = cell->zsbuf_map;
      fb->depth_format = zbuf ? zbuf->format : PIPE_FORMAT_NONE;
      fb->width = cell->framebuffer.cbufs[0]->width;
      fb->height = cell->framebuffer.cbufs[0]->height;
   }

   if (cell->dirty & CELL_NEW_DEPTH_STENCIL) {
      emit_state_cmd(cell, CELL_CMD_STATE_DEPTH_STENCIL,
                     cell->depth_stencil,
                     sizeof(struct pipe_depth_stencil_alpha_state));
   }

   if (cell->dirty & CELL_NEW_SAMPLER) {
      emit_state_cmd(cell, CELL_CMD_STATE_SAMPLER,
                     cell->sampler[0], sizeof(struct pipe_sampler_state));
   }

   if (cell->dirty & CELL_NEW_VERTEX_INFO) {
      emit_state_cmd(cell, CELL_CMD_STATE_VERTEX_INFO,
                     &cell->vertex_info, sizeof(struct vertex_info));
   }
}

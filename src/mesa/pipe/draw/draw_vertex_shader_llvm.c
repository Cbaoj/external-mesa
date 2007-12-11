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

 /*
  * Authors:
  *   Zack Rusin zack@tungstengraphics.com
  */

#include "pipe/p_util.h"
#include "draw_private.h"
#include "draw_context.h"
#include "draw_vertex.h"

#ifdef MESA_LLVM

#include "pipe/llvm/gallivm.h"
#include "pipe/p_shader_tokens.h"

#define DBG 0

static INLINE void
fetch_attrib4(const void *ptr, enum pipe_format format, float attrib[4])
{
   /* defaults */
   attrib[1] = 0.0;
   attrib[2] = 0.0;
   attrib[3] = 1.0;
   switch (format) {
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
      attrib[3] = ((float *) ptr)[3];
      /* fall-through */
   case PIPE_FORMAT_R32G32B32_FLOAT:
      attrib[2] = ((float *) ptr)[2];
      /* fall-through */
   case PIPE_FORMAT_R32G32_FLOAT:
      attrib[1] = ((float *) ptr)[1];
      /* fall-through */
   case PIPE_FORMAT_R32_FLOAT:
      attrib[0] = ((float *) ptr)[0];
      break;
   default:
      assert(0);
   }
}


/**
 * Fetch vertex attributes for 'count' vertices.
 */
static INLINE
void vertex_fetch(struct draw_context *draw,
                  const unsigned elt,
                  float (*inputs)[4])
{
   uint attr;

   /* loop over vertex attributes (vertex shader inputs) */
   for (attr = 0; attr < draw->vertex_shader->state->num_inputs; attr++) {

      unsigned buf = draw->vertex_element[attr].vertex_buffer_index;
      const void *src
         = (const void *) ((const ubyte *) draw->user.vbuffer[buf]
                           + draw->vertex_buffer[buf].buffer_offset
                           + draw->vertex_element[attr].src_offset
                           + elt * draw->vertex_buffer[buf].pitch);
      fetch_attrib4(src, draw->vertex_element[attr].src_format, inputs[attr]);
   }
}

static INLINE unsigned
compute_clipmask(const float *clip, const float (*plane)[4], unsigned nr)
{
   unsigned mask = 0;
   unsigned i;

   for (i = 0; i < nr; i++) {
      if (dot4(clip, plane[i]) < 0)
         mask |= (1<<i);
   }

   return mask;
}


/**
 * Called by the draw module when the vertx cache needs to be flushed.
 * This involves running the vertex shader.
 */
void draw_vertex_shader_queue_flush_llvm(struct draw_context *draw)
{
   unsigned i;

   struct vertex_header *dests[VS_QUEUE_LENGTH];
   float                 inputs[VS_QUEUE_LENGTH][PIPE_MAX_SHADER_INPUTS][4] ALIGN16_ATTRIB;
   float                 outputs[VS_QUEUE_LENGTH][PIPE_MAX_SHADER_INPUTS][4] ALIGN16_ATTRIB;
   float (*consts)[4]          = (float (*)[4]) draw->user.constants;
   struct gallivm_prog  *prog  = draw->vertex_shader->llvm_prog;
   const float          *scale = draw->viewport.scale;
   const float          *trans = draw->viewport.translate;
   /* fetch the inputs */
   for (i = 0; i < draw->vs.queue_nr; ++i) {
      unsigned elt = draw->vs.queue[i].elt;
      dests[i] = draw->vs.queue[i].dest;
      vertex_fetch(draw, elt, inputs[i]);
   }

   /* batch execute the shaders on all the vertices */
   gallivm_prog_exec(prog, inputs, outputs, consts,
                     draw->vs.queue_nr,
                     draw->vertex_shader->state->num_inputs,
                     draw->vertex_info.num_attribs - 2);


   /* store machine results */
   for (int i = 0; i < draw->vs.queue_nr; ++i) {
      unsigned slot;
      float x, y, z, w;
      struct vertex_header *vOut = draw->vs.queue[i].dest;
      float (*dests)[4] = outputs[i];

      /* Handle attr[0] (position) specially:
       *
       * XXX: Computing the clipmask should be done in the vertex
       * program as a set of DP4 instructions appended to the
       * user-provided code.
       */
      x = vOut->clip[0] = dests[0][0];
      y = vOut->clip[1] = dests[0][1];
      z = vOut->clip[2] = dests[0][2];
      w = vOut->clip[3] = dests[0][3];
#if DBG
      printf("output %d: %f %f %f %f\n", 0, x, y, z, w);
#endif

      vOut->clipmask = compute_clipmask(vOut->clip, draw->plane, draw->nr_planes);
      vOut->edgeflag = 1;
      /* divide by w */
      w = 1.0f / w;
      x *= w;
      y *= w;
      z *= w;

      /* Viewport mapping */
      vOut->data[0][0] = x * scale[0] + trans[0];
      vOut->data[0][1] = y * scale[1] + trans[1];
      vOut->data[0][2] = z * scale[2] + trans[2];
      vOut->data[0][3] = w;

      /* Remaining attributes are packed into sequential post-transform
       * vertex attrib slots.
       */
      for (slot = 1; slot < draw->vertex_info.num_attribs; slot++) {
         vOut->data[slot][0] = dests[slot][0];
         vOut->data[slot][1] = dests[slot][1];
         vOut->data[slot][2] = dests[slot][2];
         vOut->data[slot][3] = dests[slot][3];

#if DBG
         printf("output %d: %f %f %f %f\n", slot,
                vOut->data[slot][0],
                vOut->data[slot][1],
                vOut->data[slot][2],
                vOut->data[slot][3]);
#endif
      }
   } /* loop over vertices */

   draw->vs.queue_nr = 0;
}

#endif /* MESA_LLVM */

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
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Brian Paul
  */

#include "pipe/p_util.h"
#include "pipe/p_shader_tokens.h"
#if defined(__i386__) || defined(__386__)
#include "pipe/tgsi/exec/tgsi_sse2.h"
#endif
#include "draw_private.h"
#include "draw_context.h"

#include "x86/rtasm/x86sse.h"
#include "pipe/llvm/gallivm.h"


#define DBG_VS 0


static INLINE unsigned
compute_clipmask(const float *clip, /*const*/ float plane[][4], unsigned nr)
{
   unsigned mask = 0;
   unsigned i;

   /* Do the hardwired planes first:
    */
   if (-clip[0] + clip[3] < 0) mask |= CLIP_RIGHT_BIT;
   if ( clip[0] + clip[3] < 0) mask |= CLIP_LEFT_BIT;
   if (-clip[1] + clip[3] < 0) mask |= CLIP_TOP_BIT;
   if ( clip[1] + clip[3] < 0) mask |= CLIP_BOTTOM_BIT;
   if (-clip[2] + clip[3] < 0) mask |= CLIP_FAR_BIT;
   if ( clip[2] + clip[3] < 0) mask |= CLIP_NEAR_BIT;

   /* Followed by any remaining ones:
    */
   for (i = 6; i < nr; i++) {
      if (dot4(clip, plane[i]) < 0) 
         mask |= (1<<i);
   }

   return mask;
}


typedef void (XSTDCALL *codegen_function) (
   const struct tgsi_exec_vector *input,
   struct tgsi_exec_vector *output,
   float (*constant)[4],
   struct tgsi_exec_vector *temporary );


/**
 * Transform vertices with the current vertex program/shader
 * Up to four vertices can be shaded at a time.
 * \param vbuffer  the input vertex data
 * \param elts  indexes of four input vertices
 * \param count  number of vertices to shade [1..4]
 * \param vOut  array of pointers to four output vertices
 */
static void
run_vertex_program(struct draw_context *draw,
                   unsigned elts[4], unsigned count,
                   struct vertex_header *vOut[])
{
   struct tgsi_exec_machine *machine = &draw->machine;
   unsigned int j;

   ALIGN16_DECL(struct tgsi_exec_vector, inputs, PIPE_ATTRIB_MAX);
   ALIGN16_DECL(struct tgsi_exec_vector, outputs, PIPE_ATTRIB_MAX);
   const float *scale = draw->viewport.scale;
   const float *trans = draw->viewport.translate;

   assert(count <= 4);
   assert(draw->vertex_shader->state->output_semantic_name[0]
          == TGSI_SEMANTIC_POSITION);

   /* Consts does not require 16 byte alignment. */
   machine->Consts = (float (*)[4]) draw->user.constants;

   machine->Inputs = ALIGN16_ASSIGN(inputs);
   machine->Outputs = ALIGN16_ASSIGN(outputs);

   draw->vertex_fetch.fetch_func( draw, machine, elts, count );

   /* run shader */
#if defined(__i386__) || defined(__386__)
   if (draw->use_sse) {
      /* SSE */
      /* cast away const */
      struct draw_vertex_shader *shader
         = (struct draw_vertex_shader *)draw->vertex_shader;
      codegen_function func
         = (codegen_function) x86_get_func( &shader->sse2_program );
      func(
         machine->Inputs,
         machine->Outputs,
         machine->Consts,
         machine->Temps );
   }
   else
#endif
   {
      /* interpreter */
      tgsi_exec_machine_run( machine );
   }

   /* store machine results */
   for (j = 0; j < count; j++) {
      unsigned slot;
      float x, y, z, w;

      /* Handle attr[0] (position) specially:
       *
       * XXX: Computing the clipmask should be done in the vertex
       * program as a set of DP4 instructions appended to the
       * user-provided code.
       */
      x = vOut[j]->clip[0] = machine->Outputs[0].xyzw[0].f[j];
      y = vOut[j]->clip[1] = machine->Outputs[0].xyzw[1].f[j];
      z = vOut[j]->clip[2] = machine->Outputs[0].xyzw[2].f[j];
      w = vOut[j]->clip[3] = machine->Outputs[0].xyzw[3].f[j];

      vOut[j]->clipmask = compute_clipmask(vOut[j]->clip, draw->plane, draw->nr_planes);
      vOut[j]->edgeflag = 1;

      /* divide by w */
      w = 1.0f / w;
      x *= w;
      y *= w;
      z *= w;

      /* Viewport mapping */
      vOut[j]->data[0][0] = x * scale[0] + trans[0];
      vOut[j]->data[0][1] = y * scale[1] + trans[1];
      vOut[j]->data[0][2] = z * scale[2] + trans[2];
      vOut[j]->data[0][3] = w;

#if DBG_VS
      printf("output[%d]win: %f %f %f %f\n", j,
             vOut[j]->data[0][0],
             vOut[j]->data[0][1],
             vOut[j]->data[0][2],
             vOut[j]->data[0][3]);
#endif
      /* Remaining attributes are packed into sequential post-transform
       * vertex attrib slots.
       */
      for (slot = 1; slot < draw->num_vs_outputs; slot++) {
         vOut[j]->data[slot][0] = machine->Outputs[slot].xyzw[0].f[j];
         vOut[j]->data[slot][1] = machine->Outputs[slot].xyzw[1].f[j];
         vOut[j]->data[slot][2] = machine->Outputs[slot].xyzw[2].f[j];
         vOut[j]->data[slot][3] = machine->Outputs[slot].xyzw[3].f[j];
#if DBG_VS
         printf("output[%d][%d]: %f %f %f %f\n", j, slot,
                vOut[j]->data[slot][0],
                vOut[j]->data[slot][1],
                vOut[j]->data[slot][2],
                vOut[j]->data[slot][3]);
#endif
      }
   } /* loop over vertices */
}


/**
 * Run the vertex shader on all vertices in the vertex queue.
 * Called by the draw module when the vertx cache needs to be flushed.
 */
void
draw_vertex_shader_queue_flush(struct draw_context *draw)
{
   unsigned i, j;

   assert(draw->vs.queue_nr != 0);

   /* XXX: do this on statechange: 
    */
   draw_update_vertex_fetch( draw );

//   fprintf(stderr, " q(%d) ", draw->vs.queue_nr );
#ifdef MESA_LLVM
   if (draw->vertex_shader->llvm_prog) {
      draw_vertex_shader_queue_flush_llvm(draw);
      return;
   }
#endif

   /* run vertex shader on vertex cache entries, four per invokation */
   for (i = 0; i < draw->vs.queue_nr; i += 4) {
      struct vertex_header *dests[4];
      unsigned elts[4];
      int n = MIN2(4, draw->vs.queue_nr - i);

      for (j = 0; j < n; j++) {
         elts[j] = draw->vs.queue[i + j].elt;
         dests[j] = draw->vs.queue[i + j].dest;
      }

      for ( ; j < 4; j++) {
	 elts[j] = elts[0];
	 dests[j] = dests[0];
      }

      assert(n > 0);
      assert(n <= 4);

      run_vertex_program(draw, elts, n, dests);
   }

   draw->vs.queue_nr = 0;
}


struct draw_vertex_shader *
draw_create_vertex_shader(struct draw_context *draw,
                          const struct pipe_shader_state *shader)
{
   struct draw_vertex_shader *vs;

   vs = CALLOC_STRUCT( draw_vertex_shader );
   if (vs == NULL) {
      return NULL;
   }

   vs->state = shader;

#ifdef MESA_LLVM
   vs->llvm_prog = gallivm_from_tgsi(shader->tokens, GALLIVM_VS);
   draw->engine = gallivm_global_cpu_engine();
   if (!draw->engine) {
      draw->engine = gallivm_cpu_engine_create(vs->llvm_prog);
   }
   else {
      gallivm_cpu_jit_compile(draw->engine, vs->llvm_prog);
   }
#elif defined(__i386__) || defined(__386__)
   if (draw->use_sse) {
      /* cast-away const */
      struct pipe_shader_state *sh = (struct pipe_shader_state *) shader;

      x86_init_func( &vs->sse2_program );
      tgsi_emit_sse2( (struct tgsi_token *) sh->tokens, &vs->sse2_program );
   }
#endif

   return vs;
}


void
draw_bind_vertex_shader(struct draw_context *draw,
                        struct draw_vertex_shader *dvs)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );

   draw->vertex_shader = dvs;
   draw->num_vs_outputs = dvs->state->num_outputs;

   /* specify the fragment program to interpret/execute */
   tgsi_exec_machine_init(&draw->machine,
                          draw->vertex_shader->state->tokens,
                          PIPE_MAX_SAMPLERS,
                          NULL /*samplers*/ );
}


void
draw_delete_vertex_shader(struct draw_context *draw,
                          struct draw_vertex_shader *dvs)
{
#if defined(__i386__) || defined(__386__)
   x86_release_func( (struct x86_function *) &dvs->sse2_program );
#endif

   FREE( dvs );
}

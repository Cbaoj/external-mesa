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

/* Vertices are just an array of floats, with all the attributes
 * packed.  We currently assume a layout like:
 *
 * attr[0][0..3] - window position
 * attr[1..n][0..3] - remaining attributes.
 *
 * Attributes are assumed to be 4 floats wide but are packed so that
 * all the enabled attributes run contiguously.
 */

#include "pipe/p_util.h"
#include "pipe/p_defines.h"

#include "x86/rtasm/x86sse.h"

#include "pipe/llvm/gallivm.h"

#include "sp_context.h"
#include "sp_state.h"
#include "sp_headers.h"
#include "sp_quad.h"
#include "sp_tex_sample.h"


struct quad_shade_stage
{
   struct quad_stage stage;
   struct tgsi_sampler samplers[PIPE_MAX_SAMPLERS];
   struct tgsi_exec_machine machine;
   struct tgsi_exec_vector *inputs, *outputs;
   int colorOutSlot, depthOutSlot;
   struct gallivm_prog *llvm_prog;
};


/** cast wrapper */
static INLINE struct quad_shade_stage *
quad_shade_stage(struct quad_stage *qs)
{
   return (struct quad_shade_stage *) qs;
}


typedef void (XSTDCALL *codegen_function)(
   const struct tgsi_exec_vector *input,
   struct tgsi_exec_vector *output,
   float (*constant)[4],
   struct tgsi_exec_vector *temporary,
   const struct tgsi_interp_coef *coef );

/* This should be done by the fragment shader execution unit (code
 * generated from the decl instructions).  Do it here for now.
 */
static void
shade_quad(
   struct quad_stage *qs,
   struct quad_header *quad )
{
   struct quad_shade_stage *qss = quad_shade_stage( qs );
   struct softpipe_context *softpipe = qs->softpipe;
   const float fx = (float) quad->x0;
   const float fy = (float) quad->y0;
   struct tgsi_exec_machine *machine = &qss->machine;

   /* Consts does not require 16 byte alignment. */
   machine->Consts = softpipe->mapped_constants[PIPE_SHADER_FRAGMENT];

   machine->SamplerUnits = softpipe->sampler_units;
   machine->InterpCoefs = quad->coef;

   machine->Inputs[0].xyzw[0].f[0] = fx;
   machine->Inputs[0].xyzw[0].f[1] = fx + 1.0f;
   machine->Inputs[0].xyzw[0].f[2] = fx;
   machine->Inputs[0].xyzw[0].f[3] = fx + 1.0f;

   machine->Inputs[0].xyzw[1].f[0] = fy;
   machine->Inputs[0].xyzw[1].f[1] = fy;
   machine->Inputs[0].xyzw[1].f[2] = fy + 1.0f;
   machine->Inputs[0].xyzw[1].f[3] = fy + 1.0f;

   /* run shader */
#if defined(__i386__) || defined(__386__)
   if( softpipe->use_sse ) {
      codegen_function func = (codegen_function) x86_get_func( &softpipe->fs->sse2_program );
      func(
         machine->Inputs,
         machine->Outputs,
         machine->Consts,
         machine->Temps,
         machine->InterpCoefs );
      quad->mask &= ~(machine->Temps[TGSI_EXEC_TEMP_KILMASK_I].xyzw[TGSI_EXEC_TEMP_KILMASK_C].u[0]);
   }
   else
#endif
   {
#ifdef MESA_LLVM
      /*ga_llvm_prog_exec(softpipe->fs->llvm_prog);*/
#endif
      quad->mask &= tgsi_exec_machine_run( machine );
   }

   /* store result color */
   if (qss->colorOutSlot >= 0) {
      /* XXX need to handle multiple color outputs someday */
      assert(qss->stage.softpipe->fs->shader.output_semantic_name[qss->colorOutSlot]
             == TGSI_SEMANTIC_COLOR);
      memcpy(
             quad->outputs.color,
             &machine->Outputs[qss->colorOutSlot].xyzw[0].f[0],
             sizeof( quad->outputs.color ) );
   }

   /* store result Z */
   if (qss->depthOutSlot >= 0) {
      /* output[slot] is new Z */
      uint i;
      for (i = 0; i < 4; i++) {
         quad->outputs.depth[i] = machine->Outputs[0].xyzw[2].f[i];
      }
   }
   else {
      /* copy input Z (which was interpolated by the executor) to output Z */
      uint i;
      for (i = 0; i < 4; i++) {
         quad->outputs.depth[i] = machine->Inputs[0].xyzw[2].f[i];
      }
   }

   /* shader may cull fragments */
   if( quad->mask ) {
      qs->next->run( qs->next, quad );
   }
}

static void
shade_quad_llvm(struct quad_stage *qs,
                struct quad_header *quad)
{
   struct quad_shade_stage *qss = quad_shade_stage(qs);
   struct softpipe_context *softpipe = qs->softpipe;
   float dests[4][16][4];
   const float fx = (float) quad->x0;
   const float fy = (float) quad->y0;
   struct gallivm_prog *llvm = qss->llvm_prog;
   float inputs[4][16][4];
   memset(inputs, 0, sizeof(inputs));

   inputs[0][0][0] = fx;
   inputs[1][0][0] = fx + 1.0f;
   inputs[2][0][0] = fx;
   inputs[3][0][0] = fx + 1.0f;

   inputs[0][0][1] = fy;
   inputs[1][0][1] = fy;
   inputs[2][0][1] = fy + 1.0f;
   inputs[3][0][1] = fy + 1.0f;
   printf("MASK = %d\n", quad->mask);
   gallivm_prog_inputs_interpolate(llvm, inputs, quad->coef);
   for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 2; ++j) {
         printf("IN(%d,%d) [%f %f %f %f]\n", i, j, 
                inputs[i][j][0], inputs[i][j][1], inputs[i][j][2], inputs[i][j][3]);
      }
   }

   /*quad->mask &=*/
      gallivm_fragment_shader_exec(llvm, fx, fy, dests, inputs,
                                   softpipe->mapped_constants[PIPE_SHADER_FRAGMENT],
                                   qss->samplers, softpipe->sampler_units);

   printf("OUT LLVM = 1[%f %f %f %f], 2[%f %f %f %f]\n",
          dests[0][0][0], dests[0][0][1], dests[0][0][2], dests[0][0][3], 
          dests[0][1][0], dests[0][1][1], dests[0][1][2], dests[0][1][3]);

   /* store result color */
   if (qss->colorOutSlot >= 0) {
      unsigned i;
      /* XXX need to handle multiple color outputs someday */
      assert(qss->stage.softpipe->fs->shader.output_semantic_name[qss->colorOutSlot]
             == TGSI_SEMANTIC_COLOR);
      for (i = 0; i < QUAD_SIZE; ++i) {
         quad->outputs.color[0][i] = dests[i][qss->colorOutSlot][0];
         quad->outputs.color[1][i] = dests[i][qss->colorOutSlot][1];
         quad->outputs.color[2][i] = dests[i][qss->colorOutSlot][2];
         quad->outputs.color[3][i] = dests[i][qss->colorOutSlot][3];
      }
   }
   for (int i = 0; i < QUAD_SIZE; ++i) {
      printf("Q%d(%d) [%f, %f, %f, %f]\n", i, qss->colorOutSlot,
             quad->outputs.color[0][i],
             quad->outputs.color[1][i],
             quad->outputs.color[2][i],
             quad->outputs.color[3][i]);
   }

   /* store result Z */
   if (qss->depthOutSlot >= 0) {
      /* output[slot] is new Z */
      uint i;
      for (i = 0; i < 4; i++) {
         quad->outputs.depth[i] = dests[i][0][2];
      }
   }
   else {
      /* copy input Z (which was interpolated by the executor) to output Z */
      uint i;
      for (i = 0; i < 4; i++) {
         quad->outputs.depth[i] = inputs[i][0][2];
      }
   }
   printf("D [%f, %f, %f, %f] mask = %d\n",
             quad->outputs.depth[0],
             quad->outputs.depth[1],
             quad->outputs.depth[2],
             quad->outputs.depth[3], quad->mask);

   /* shader may cull fragments */
   if( quad->mask ) {
      qs->next->run( qs->next, quad );
   }
}

/**
 * Per-primitive (or per-begin?) setup
 */
static void shade_begin(struct quad_stage *qs)
{
   struct quad_shade_stage *qss = quad_shade_stage(qs);
   struct softpipe_context *softpipe = qs->softpipe;
   unsigned i;

   /* set TGSI sampler state that varies */
   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      qss->samplers[i].state = softpipe->sampler[i];
      qss->samplers[i].texture = softpipe->texture[i];
   }

   qss->llvm_prog = softpipe->fs->llvm_prog;
   /* XXX only do this if the fragment shader changes... */
   tgsi_exec_machine_init(&qss->machine,
                          softpipe->fs->shader.tokens,
                          PIPE_MAX_SAMPLERS,
                          qss->samplers );

   /* find output slots for depth, color */
   qss->colorOutSlot = -1;
   qss->depthOutSlot = -1;
   for (i = 0; i < qss->stage.softpipe->fs->shader.num_outputs; i++) {
      switch (qss->stage.softpipe->fs->shader.output_semantic_name[i]) {
      case TGSI_SEMANTIC_POSITION:
         qss->depthOutSlot = i;
         break;
      case TGSI_SEMANTIC_COLOR:
         qss->colorOutSlot = i;
         break;
      }
   }

   if (qs->next)
      qs->next->begin(qs->next);
}


static void shade_destroy(struct quad_stage *qs)
{
   struct quad_shade_stage *qss = (struct quad_shade_stage *) qs;

   FREE( qss->inputs );
   FREE( qss->outputs );
   FREE( qs );
}


struct quad_stage *sp_quad_shade_stage( struct softpipe_context *softpipe )
{
   struct quad_shade_stage *qss = CALLOC_STRUCT(quad_shade_stage);
   uint i;

   /* allocate storage for program inputs/outputs, aligned to 16 bytes */
   qss->inputs = MALLOC(PIPE_ATTRIB_MAX * sizeof(*qss->inputs) + 16);
   qss->outputs = MALLOC(PIPE_ATTRIB_MAX * sizeof(*qss->outputs) + 16);
   qss->machine.Inputs = align16(qss->inputs);
   qss->machine.Outputs = align16(qss->outputs);

   qss->stage.softpipe = softpipe;
   qss->stage.begin = shade_begin;
#ifdef MESA_LLVM
   qss->stage.run = shade_quad_llvm;
#else
   qss->stage.run = shade_quad;
#endif
   qss->stage.destroy = shade_destroy;

   /* set TGSI sampler state that's constant */
   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      assert(softpipe->tex_cache[i]);
      qss->samplers[i].get_samples = sp_get_samples;
      qss->samplers[i].pipe = &softpipe->pipe;
      qss->samplers[i].cache = softpipe->tex_cache[i];
   }

   return &qss->stage;
}

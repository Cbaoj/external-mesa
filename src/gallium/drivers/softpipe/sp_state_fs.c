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

#include "sp_context.h"
#include "sp_state.h"

#include "pipe/p_defines.h"
#include "pipe/p_util.h"
#include "pipe/p_inlines.h"
#include "pipe/p_winsys.h"
#include "draw/draw_context.h"
#include "pipe/p_shader_tokens.h"
#include "llvm/gallivm.h"
#include "tgsi/util/tgsi_dump.h"
#include "tgsi/exec/tgsi_sse2.h"


void *
softpipe_create_fs_state(struct pipe_context *pipe,
                         const struct pipe_shader_state *templ)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   struct sp_fragment_shader_state *state;

   /* Decide whether we'll be codegenerating this shader and if so do
    * that now.
    */

   state = CALLOC_STRUCT(sp_fragment_shader_state);
   if (!state)
      return NULL;

   state->shader = *templ;

   if (softpipe->dump_fs) {
      tgsi_dump(state->shader.tokens, 0);
   }

#ifdef MESA_LLVM
   state->llvm_prog = 0;

#if 0
   if (!gallivm_global_cpu_engine()) {
      gallivm_cpu_engine_create(state->llvm_prog);
   }
   else
      gallivm_cpu_jit_compile(gallivm_global_cpu_engine(), state->llvm_prog);
#endif

#elif defined(__i386__) || defined(__386__)
   if (softpipe->use_sse) {
      x86_init_func( &state->sse2_program );
      tgsi_emit_sse2_fs( state->shader.tokens, &state->sse2_program );
   }
#endif

   return state;
}


void
softpipe_bind_fs_state(struct pipe_context *pipe, void *fs)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   softpipe->fs = (struct sp_fragment_shader_state *) fs;

   softpipe->dirty |= SP_NEW_FS;
}


void
softpipe_delete_fs_state(struct pipe_context *pipe, void *fs)
{
   struct sp_fragment_shader_state *state = fs;

#if defined(__i386__) || defined(__386__)
   x86_release_func( &state->sse2_program );
#endif

   FREE( state );
}


void *
softpipe_create_vs_state(struct pipe_context *pipe,
                         const struct pipe_shader_state *templ)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   struct sp_vertex_shader_state *state;

   state = CALLOC_STRUCT(sp_vertex_shader_state);
   if (state == NULL ) {
      return NULL;
   }

   state->shader = *templ;

   state->draw_data = draw_create_vertex_shader(softpipe->draw,
                                                &state->shader);
   if (state->draw_data == NULL) {
      FREE( state );
      return NULL;
   }

   return state;
}


void
softpipe_bind_vs_state(struct pipe_context *pipe, void *vs)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   softpipe->vs = (const struct sp_vertex_shader_state *)vs;

   draw_bind_vertex_shader(softpipe->draw, softpipe->vs->draw_data);

   softpipe->dirty |= SP_NEW_VS;
}


void
softpipe_delete_vs_state(struct pipe_context *pipe, void *vs)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   struct sp_vertex_shader_state *state =
      (struct sp_vertex_shader_state *)vs;

   draw_delete_vertex_shader(softpipe->draw, state->draw_data);
   FREE( state );
}



void
softpipe_set_constant_buffer(struct pipe_context *pipe,
                             uint shader, uint index,
                             const struct pipe_constant_buffer *buf)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   struct pipe_winsys *ws = pipe->winsys;

   assert(shader < PIPE_SHADER_TYPES);
   assert(index == 0);

   /* note: reference counting */
   pipe_buffer_reference(ws,
			 &softpipe->constants[shader].buffer,
			 buf->buffer);
   softpipe->constants[shader].size = buf->size;

   softpipe->dirty |= SP_NEW_CONSTANTS;
}

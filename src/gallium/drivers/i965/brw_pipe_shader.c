/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

#include "util/u_memory.h"
  
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_scan.h"

#include "brw_context.h"
#include "brw_util.h"
#include "brw_wm.h"


/**
 * Determine if the given shader uses complex features such as flow
 * conditionals, loops, subroutines.
 */
GLboolean brw_wm_has_flow_control(const struct brw_fragment_shader *fp)
{
    return (fp->info.opcode_count[TGSI_OPCODE_ARL] > 0 ||
	    fp->info.opcode_count[TGSI_OPCODE_IF] > 0 ||
	    fp->info.opcode_count[TGSI_OPCODE_ENDIF] > 0 || /* redundant - IF */
	    fp->info.opcode_count[TGSI_OPCODE_CAL] > 0 ||
	    fp->info.opcode_count[TGSI_OPCODE_BRK] > 0 ||   /* redundant - BGNLOOP */
	    fp->info.opcode_count[TGSI_OPCODE_RET] > 0 ||   /* redundant - CAL */
	    fp->info.opcode_count[TGSI_OPCODE_BGNLOOP] > 0);
}



static void brw_bind_fs_state( struct pipe_context *pipe, void *prog )
{
   struct brw_context *brw = brw_context(pipe);

   brw->curr.fragment_shader = (struct brw_fragment_shader *)prog;
   brw->state.dirty.mesa |= PIPE_NEW_FRAGMENT_SHADER;
}

static void brw_bind_vs_state( struct pipe_context *pipe, void *prog )
{
   struct brw_context *brw = brw_context(pipe);

   brw->curr.vertex_shader = (struct brw_vertex_shader *)prog;
   brw->state.dirty.mesa |= PIPE_NEW_VERTEX_SHADER;
}



static void *brw_create_fs_state( struct pipe_context *pipe,
				  const struct pipe_shader_state *shader )
{
   struct brw_context *brw = brw_context(pipe);
   struct brw_fragment_shader *fs;
   int i;

   fs = CALLOC_STRUCT(brw_fragment_shader);
   if (fs == NULL)
      return NULL;

   /* Duplicate tokens, scan shader
    */
   fs->id = brw->program_id++;
   fs->has_flow_control = brw_wm_has_flow_control(fs);

   fs->tokens = tgsi_dup_tokens(shader->tokens);
   if (fs->tokens == NULL)
      goto fail;

   tgsi_scan_shader(fs->tokens, &fs->info);

   for (i = 0; i < fs->info.num_inputs; i++)
      if (fs->info.input_semantic_name[i] == TGSI_SEMANTIC_POSITION)
	 fs->uses_depth = 1;

   if (fs->info.uses_kill)
      fs->iz_lookup |= IZ_PS_KILL_ALPHATEST_BIT;

   if (fs->info.writes_z)
      fs->iz_lookup |= IZ_PS_COMPUTES_DEPTH_BIT;

   return (void *)fs;

fail:
   FREE(fs);
   return NULL;
}


static void *brw_create_vs_state( struct pipe_context *pipe,
				  const struct pipe_shader_state *shader )
{
   struct brw_context *brw = brw_context(pipe);

   struct brw_vertex_shader *vs = CALLOC_STRUCT(brw_vertex_shader);
   if (vs == NULL)
      return NULL;

   /* Duplicate tokens, scan shader
    */
   vs->id = brw->program_id++;
   //vs->has_flow_control = brw_wm_has_flow_control(vs);

   vs->tokens = tgsi_dup_tokens(shader->tokens);
   if (vs->tokens == NULL)
      goto fail;

   tgsi_scan_shader(vs->tokens, &vs->info);
   
   /* Done:
    */
   return (void *)vs;

fail:
   FREE(vs);
   return NULL;
}


static void brw_delete_fs_state( struct pipe_context *pipe, void *prog )
{
   struct brw_context *brw = brw_context(pipe);
   struct brw_fragment_shader *fs = (struct brw_fragment_shader *)prog;

   brw->sws->bo_unreference(fs->const_buffer);
   FREE( (void *)fs->tokens );
   FREE( fs );
}


static void brw_delete_vs_state( struct pipe_context *pipe, void *prog )
{
   struct brw_fragment_shader *vs = (struct brw_fragment_shader *)prog;

   /* Delete draw shader
    */
   FREE( (void *)vs->tokens );
   FREE( vs );
}


static void brw_set_constant_buffer(struct pipe_context *pipe,
                                     uint shader, uint index,
                                     const struct pipe_constant_buffer *buf)
{
   struct brw_context *brw = brw_context(pipe);

   assert(index == 0);

   if (shader == PIPE_SHADER_FRAGMENT) {
      pipe_buffer_reference( &brw->curr.fragment_constants,
                             buf->buffer );

      brw->state.dirty.mesa |= PIPE_NEW_FRAGMENT_CONSTANTS;
   }
   else {
      pipe_buffer_reference( &brw->curr.vertex_constants,
                             buf->buffer );

      brw->state.dirty.mesa |= PIPE_NEW_VERTEX_CONSTANTS;
   }
}


void brw_pipe_shader_init( struct brw_context *brw )
{
   brw->base.set_constant_buffer = brw_set_constant_buffer;

   brw->base.create_vs_state = brw_create_vs_state;
   brw->base.bind_vs_state = brw_bind_vs_state;
   brw->base.delete_vs_state = brw_delete_vs_state;

   brw->base.create_fs_state = brw_create_fs_state;
   brw->base.bind_fs_state = brw_bind_fs_state;
   brw->base.delete_fs_state = brw_delete_fs_state;
}

void brw_pipe_shader_cleanup( struct brw_context *brw )
{
   pipe_buffer_reference( &brw->curr.fragment_constants, NULL );
   pipe_buffer_reference( &brw->curr.vertex_constants, NULL );
}

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

#include "main/imports.h"
#include "main/context.h"
#include "main/extensions.h"
#include "vbo/vbo.h"
#include "shader/shader_api.h"
#include "st_public.h"
#include "st_context.h"
#include "st_cb_accum.h"
#include "st_cb_bufferobjects.h"
#include "st_cb_clear.h"
#include "st_cb_drawpixels.h"
#include "st_cb_fbo.h"
#include "st_cb_feedback.h"
#include "st_cb_queryobj.h"
#include "st_cb_rasterpos.h"
#include "st_cb_readpixels.h"
#include "st_cb_texture.h"
#include "st_cb_flush.h"
#include "st_cb_strings.h"
#include "st_atom.h"
#include "st_draw.h"
#include "st_extensions.h"
#include "st_program.h"
#include "pipe/p_context.h"
#include "pipe/draw/draw_context.h"
#include "pipe/cso_cache/cso_cache.h"


/**
 * Called via ctx->Driver.UpdateState()
 */
void st_invalidate_state(GLcontext * ctx, GLuint new_state)
{
   struct st_context *st = st_context(ctx);

   st->dirty.mesa |= new_state;
   st->dirty.st |= ST_NEW_MESA;

   /* This is the only core Mesa module we depend upon.
    * No longer use swrast, swsetup, tnl.
    */
   _vbo_InvalidateState(ctx, new_state);
}


static struct st_context *
st_create_context_priv( GLcontext *ctx, struct pipe_context *pipe )
{
   struct st_context *st = CALLOC_STRUCT( st_context );
   
   ctx->st = st;

   st->ctx = ctx;
   st->pipe = pipe;

   /* state tracker needs the VBO module */
   _vbo_CreateContext(ctx);

   st->draw = draw_create(); /* for selection/feedback */

   st->dirty.mesa = ~0;
   st->dirty.st = ~0;

   st->cache = cso_cache_create();

   st_init_atoms( st );
   st_init_draw( st );

   /* we want all vertex data to be placed in buffer objects */
   vbo_use_buffer_objects(ctx);

   /* Need these flags:
    */
   st->ctx->FragmentProgram._MaintainTexEnvProgram = GL_TRUE;
   st->ctx->FragmentProgram._UseTexEnvProgram = GL_TRUE;

   st->ctx->VertexProgram._MaintainTnlProgram = GL_TRUE;

   st->haveFramebufferSurfaces = GL_TRUE;

   st->pixel_xfer.cache = _mesa_new_program_cache();

   /* GL limits and extensions */
   st_init_limits(st);
   st_init_extensions(st);

   return st;
}


struct st_context *st_create_context(struct pipe_context *pipe,
                                     const __GLcontextModes *visual,
                                     struct st_context *share)
{
   GLcontext *ctx;
   GLcontext *shareCtx = share ? share->ctx : NULL;
   struct dd_function_table funcs;

   memset(&funcs, 0, sizeof(funcs));
   st_init_driver_functions(&funcs);

   ctx = _mesa_create_context(visual, shareCtx, &funcs, NULL);

   return st_create_context_priv(ctx, pipe);
}


static void st_destroy_context_priv( struct st_context *st )
{
   draw_destroy(st->draw);
   st_destroy_atoms( st );
   st_destroy_draw( st );

   _vbo_DestroyContext(st->ctx);

   cso_cache_delete( st->cache );

   _mesa_delete_program_cache(st->ctx, st->pixel_xfer.cache);

   st->pipe->destroy( st->pipe );
   free( st );
}

 
void st_destroy_context( struct st_context *st )
{
   GLcontext *ctx = st->ctx;
   _mesa_free_context_data(ctx);
   st_destroy_context_priv(st);
   free(ctx);
}


void st_make_current(struct st_context *st,
                     struct st_framebuffer *draw,
                     struct st_framebuffer *read)
{
   if (st) {
      _mesa_make_current(st->ctx, &draw->Base, &read->Base);
   }
   else {
      _mesa_make_current(NULL, NULL, NULL);
   }
}


void st_copy_context_state(struct st_context *dst,
                           struct st_context *src,
                           uint mask)
{
   _mesa_copy_context(dst->ctx, src->ctx, mask);
}


void st_init_driver_functions(struct dd_function_table *functions)
{
   _mesa_init_glsl_driver_functions(functions);

   st_init_accum_functions(functions);
   st_init_bufferobject_functions(functions);
   st_init_clear_functions(functions);
   st_init_drawpixels_functions(functions);
   st_init_fbo_functions(functions);
   st_init_feedback_functions(functions);
   st_init_program_functions(functions);
   st_init_query_functions(functions);
   st_init_rasterpos_functions(functions);
   st_init_readpixels_functions(functions);
   st_init_texture_functions(functions);
   st_init_flush_functions(functions);
   st_init_string_functions(functions);

   functions->UpdateState = st_invalidate_state;
}

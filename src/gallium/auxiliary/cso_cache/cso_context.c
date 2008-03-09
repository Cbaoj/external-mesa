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

 /* Wrap the cso cache & hash mechanisms in a simplified
  * pipe-driver-specific interface.
  *
  * Authors:
  *   Zack Rusin <zack@tungstengraphics.com>
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

#include "pipe/p_state.h"
#include "pipe/p_util.h"

#include "cso_cache/cso_context.h"
#include "cso_cache/cso_cache.h"
#include "cso_cache/cso_hash.h"

struct cso_context {
   struct pipe_context *pipe;
   struct cso_cache *cache;

   struct {
      void *samplers[PIPE_MAX_SAMPLERS];
      unsigned nr_samplers;
   } hw;

   void *samplers[PIPE_MAX_SAMPLERS];
   unsigned nr_samplers;

   void *blend;
   void *depth_stencil;
   void *rasterizer;
   void *fragment_shader;
   void *vertex_shader;
};


struct cso_context *cso_create_context( struct pipe_context *pipe )
{
   struct cso_context *ctx = CALLOC_STRUCT(cso_context);
   if (ctx == NULL)
      goto out;

   ctx->cache = cso_cache_create();
   if (ctx->cache == NULL)
      goto out;

   ctx->pipe = pipe;

   return ctx;

out:
   cso_destroy_context( ctx );      
   return NULL;
}


void cso_destroy_context( struct cso_context *ctx )
{
   if (ctx == NULL)
      return;
   
/*
   if (ctx->pipe) 
      ctx->pipe->flush( ctx->pipe, PIPE_FLUSH_UNBIND_ALL );
*/

   if (ctx->cache)
      cso_cache_delete( ctx->cache );
   
   FREE( ctx );
}


/* Those function will either find the state of the given template
 * in the cache or they will create a new state from the given
 * template, insert it in the cache and return it.
 */

/*
 * If the driver returns 0 from the create method then they will assign
 * the data member of the cso to be the template itself.
 */

void cso_set_blend(struct cso_context *ctx,
                   const struct pipe_blend_state *templ)
{
   unsigned hash_key = cso_construct_key((void*)templ, sizeof(struct pipe_blend_state));
   struct cso_hash_iter iter = cso_find_state_template(ctx->cache,
                                                       hash_key, CSO_BLEND,
                                                       (void*)templ);
   void *handle;

   if (cso_hash_iter_is_null(iter)) {
      struct cso_blend *cso = MALLOC(sizeof(struct cso_blend));

      cso->state = *templ;
      cso->data = ctx->pipe->create_blend_state(ctx->pipe, &cso->state);
      cso->delete_state = (cso_state_callback)ctx->pipe->delete_blend_state;
      cso->context = ctx->pipe;

      iter = cso_insert_state(ctx->cache, hash_key, CSO_BLEND, cso);
      handle = cso->data;
   }
   else {
      handle = ((struct cso_blend *)cso_hash_iter_data(iter))->data;
   }

   if (ctx->blend != handle) {
      ctx->blend = handle;
      ctx->pipe->bind_blend_state(ctx->pipe, handle);
   }
}

void cso_single_sampler(struct cso_context *ctx,
                        unsigned idx,
                        const struct pipe_sampler_state *templ)
{
   void *handle = NULL;
   
   if (templ != NULL) {
      unsigned hash_key = cso_construct_key((void*)templ, sizeof(struct pipe_sampler_state));
      struct cso_hash_iter iter = cso_find_state_template(ctx->cache,
                                                          hash_key, CSO_SAMPLER,
                                                          (void*)templ);

      if (cso_hash_iter_is_null(iter)) {
         struct cso_sampler *cso = MALLOC(sizeof(struct cso_sampler));
         
         cso->state = *templ;
         cso->data = ctx->pipe->create_sampler_state(ctx->pipe, &cso->state);
         cso->delete_state = (cso_state_callback)ctx->pipe->delete_sampler_state;
         cso->context = ctx->pipe;

         iter = cso_insert_state(ctx->cache, hash_key, CSO_SAMPLER, cso);
         handle = cso->data;
      }
      else {
         handle = ((struct cso_sampler *)cso_hash_iter_data(iter))->data;
      }
   }

   ctx->samplers[idx] = handle;
}

void cso_single_sampler_done( struct cso_context *ctx )
{
   unsigned i; 

   for (i = 0; i < 8; i++)
      if (ctx->samplers[i] == NULL)
         break;

   ctx->nr_samplers = i;

   if (ctx->hw.nr_samplers != ctx->nr_samplers ||
       memcmp(ctx->hw.samplers, 
              ctx->samplers, 
              ctx->nr_samplers * sizeof(void *)) != 0) 
   {
      memcpy(ctx->hw.samplers, ctx->samplers, ctx->nr_samplers * sizeof(void *));
      ctx->hw.nr_samplers = ctx->nr_samplers;

      ctx->pipe->bind_sampler_states(ctx->pipe, ctx->nr_samplers, ctx->samplers);
   }
}

void cso_set_samplers( struct cso_context *ctx,
                       unsigned nr,
                       const struct pipe_sampler_state **templates )
{
   unsigned i;
   
   /* TODO: fastpath
    */

   for (i = 0; i < nr; i++)
      cso_single_sampler( ctx, i, templates[i] );

   for ( ; i < ctx->nr_samplers; i++)
      cso_single_sampler( ctx, i, NULL );
   
   cso_single_sampler_done( ctx );
}

void cso_set_depth_stencil_alpha(struct cso_context *ctx,
                                 const struct pipe_depth_stencil_alpha_state *templ)
{
   unsigned hash_key = cso_construct_key((void*)templ,
                                         sizeof(struct pipe_depth_stencil_alpha_state));
   struct cso_hash_iter iter = cso_find_state_template(ctx->cache,
                                                       hash_key, 
						       CSO_DEPTH_STENCIL_ALPHA,
                                                       (void*)templ);
   void *handle;

   if (cso_hash_iter_is_null(iter)) {
      struct cso_depth_stencil_alpha *cso = MALLOC(sizeof(struct cso_depth_stencil_alpha));

      cso->state = *templ;
      cso->data = ctx->pipe->create_depth_stencil_alpha_state(ctx->pipe, &cso->state);
      cso->delete_state = (cso_state_callback)ctx->pipe->delete_depth_stencil_alpha_state;
      cso->context = ctx->pipe;

      cso_insert_state(ctx->cache, hash_key, CSO_DEPTH_STENCIL_ALPHA, cso);
      handle = cso->data;
   }
   else {
      handle = ((struct cso_depth_stencil_alpha *)cso_hash_iter_data(iter))->data;
   }

   if (ctx->depth_stencil != handle) {
      ctx->depth_stencil = handle;
      ctx->pipe->bind_depth_stencil_alpha_state(ctx->pipe, handle);
   }
}



void cso_set_rasterizer(struct cso_context *ctx,
                              const struct pipe_rasterizer_state *templ)
{
   unsigned hash_key = cso_construct_key((void*)templ,
                                         sizeof(struct pipe_rasterizer_state));
   struct cso_hash_iter iter = cso_find_state_template(ctx->cache,
                                                       hash_key, CSO_RASTERIZER,
                                                       (void*)templ);
   void *handle = NULL;

   if (cso_hash_iter_is_null(iter)) {
      struct cso_rasterizer *cso = MALLOC(sizeof(struct cso_rasterizer));

      cso->state = *templ;
      cso->data = ctx->pipe->create_rasterizer_state(ctx->pipe, &cso->state);
      cso->delete_state = (cso_state_callback)ctx->pipe->delete_rasterizer_state;
      cso->context = ctx->pipe;

      cso_insert_state(ctx->cache, hash_key, CSO_RASTERIZER, cso);
      handle = cso->data;
   }
   else {
      handle = ((struct cso_rasterizer *)cso_hash_iter_data(iter))->data;
   }

   if (ctx->rasterizer != handle) {
      ctx->rasterizer = handle;
      ctx->pipe->bind_rasterizer_state(ctx->pipe, handle);
   }
}





void cso_set_fragment_shader(struct cso_context *ctx,
                             const struct pipe_shader_state *templ)
{
   unsigned hash_key = cso_construct_key((void*)templ,
                                         sizeof(struct pipe_shader_state));
   struct cso_hash_iter iter = cso_find_state_template(ctx->cache,
                                                       hash_key, CSO_FRAGMENT_SHADER,
                                                       (void*)templ);
   void *handle = NULL;

   if (cso_hash_iter_is_null(iter)) {
      struct cso_fragment_shader *cso = MALLOC(sizeof(struct cso_fragment_shader));

      cso->state = *templ;
      cso->data = ctx->pipe->create_fs_state(ctx->pipe, &cso->state);
      cso->delete_state = (cso_state_callback)ctx->pipe->delete_fs_state;
      cso->context = ctx->pipe;

      iter = cso_insert_state(ctx->cache, hash_key, CSO_FRAGMENT_SHADER, cso);
      handle = cso->data;
   }
   else {
      handle = ((struct cso_fragment_shader *)cso_hash_iter_data(iter))->data;
   }

   if (ctx->fragment_shader != handle) {
      ctx->fragment_shader = handle;
      ctx->pipe->bind_fs_state(ctx->pipe, handle);
   }
}

void cso_set_vertex_shader(struct cso_context *ctx,
                           const struct pipe_shader_state *templ)
{
   unsigned hash_key = cso_construct_key((void*)templ,
                                         sizeof(struct pipe_shader_state));
   struct cso_hash_iter iter = cso_find_state_template(ctx->cache,
                                                       hash_key, CSO_VERTEX_SHADER,
                                                       (void*)templ);
   void *handle = NULL;

   if (cso_hash_iter_is_null(iter)) {
      struct cso_vertex_shader *cso = MALLOC(sizeof(struct cso_vertex_shader));

      cso->state = *templ;
      cso->data = ctx->pipe->create_vs_state(ctx->pipe, &cso->state);
      cso->delete_state = (cso_state_callback)ctx->pipe->delete_vs_state;
      cso->context = ctx->pipe;

      iter = cso_insert_state(ctx->cache, hash_key, CSO_VERTEX_SHADER, cso);
      handle = cso->data;
   }
   else {
      handle = ((struct cso_vertex_shader *)cso_hash_iter_data(iter))->data;
   }

   if (ctx->vertex_shader != handle) {
      ctx->vertex_shader = handle;
      ctx->pipe->bind_fs_state(ctx->pipe, handle);
   }
}

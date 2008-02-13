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
 

#include "st_context.h"
#include "st_cache.h"
#include "st_atom.h"
#include "st_program.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"


/**
 * Convert GLenum texcoord wrap tokens to pipe tokens.
 */
static GLuint
gl_wrap_to_sp(GLenum wrap)
{
   switch (wrap) {
   case GL_REPEAT:
      return PIPE_TEX_WRAP_REPEAT;
   case GL_CLAMP:
      return PIPE_TEX_WRAP_CLAMP;
   case GL_CLAMP_TO_EDGE:
      return PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   case GL_CLAMP_TO_BORDER:
      return PIPE_TEX_WRAP_CLAMP_TO_BORDER;
   case GL_MIRRORED_REPEAT:
      return PIPE_TEX_WRAP_MIRROR_REPEAT;
   case GL_MIRROR_CLAMP_EXT:
      return PIPE_TEX_WRAP_MIRROR_CLAMP;
   case GL_MIRROR_CLAMP_TO_EDGE_EXT:
      return PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE;
   case GL_MIRROR_CLAMP_TO_BORDER_EXT:
      return PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER;
   default:
      abort();
      return 0;
   }
}


static GLuint
gl_filter_to_mip_filter(GLenum filter)
{
   switch (filter) {
   case GL_NEAREST:
   case GL_LINEAR:
      return PIPE_TEX_MIPFILTER_NONE;

   case GL_NEAREST_MIPMAP_NEAREST:
   case GL_LINEAR_MIPMAP_NEAREST:
      return PIPE_TEX_MIPFILTER_NEAREST;

   case GL_NEAREST_MIPMAP_LINEAR:
   case GL_LINEAR_MIPMAP_LINEAR:
      return PIPE_TEX_MIPFILTER_LINEAR;

   default:
      assert(0);
      return PIPE_TEX_MIPFILTER_NONE;
   }
}


static GLuint
gl_filter_to_img_filter(GLenum filter)
{
   switch (filter) {
   case GL_NEAREST:
   case GL_NEAREST_MIPMAP_NEAREST:
   case GL_NEAREST_MIPMAP_LINEAR:
      return PIPE_TEX_FILTER_NEAREST;

   case GL_LINEAR:
   case GL_LINEAR_MIPMAP_NEAREST:
   case GL_LINEAR_MIPMAP_LINEAR:
      return PIPE_TEX_FILTER_LINEAR;

   default:
      assert(0);
      return PIPE_TEX_FILTER_NEAREST;
   }
}


static void 
update_samplers(struct st_context *st)
{
   const struct st_fragment_program *fs = st->fp;
   GLuint su;

   /* loop over sampler units (aka tex image units) */
   for (su = 0; su < st->ctx->Const.MaxTextureImageUnits; su++) {
      struct pipe_sampler_state sampler;
      const struct cso_sampler *cso;

      memset(&sampler, 0, sizeof(sampler));

      if (fs->Base.Base.SamplersUsed & (1 << su)) {
         GLuint texUnit = fs->Base.Base.SamplerUnits[su];
         const struct gl_texture_object *texobj
            = st->ctx->Texture.Unit[texUnit]._Current;

         assert(texobj);

         sampler.wrap_s = gl_wrap_to_sp(texobj->WrapS);
         sampler.wrap_t = gl_wrap_to_sp(texobj->WrapT);
         sampler.wrap_r = gl_wrap_to_sp(texobj->WrapR);

         sampler.min_img_filter = gl_filter_to_img_filter(texobj->MinFilter);
         sampler.min_mip_filter = gl_filter_to_mip_filter(texobj->MinFilter);
         sampler.mag_img_filter = gl_filter_to_img_filter(texobj->MagFilter);

         if (texobj->Target != GL_TEXTURE_RECTANGLE_ARB)
            sampler.normalized_coords = 1;

         sampler.lod_bias = st->ctx->Texture.Unit[su].LodBias;
#if 1
         sampler.min_lod = (texobj->MinLod) < 0.0 ? 0.0 : texobj->MinLod;
         sampler.max_lod = texobj->MaxLod;
#else
         /* min/max lod should really be as follows (untested).
          * Also, calculate_first_last_level() needs to be overhauled
          * since today's hardware had real support for LOD clamping.
          */
         sampler.min_lod = MAX2(texobj->BaseLevel, texobj->MinLod);
         sampler.max_lod = MIN2(texobj->MaxLevel, texobj->MaxLod);
#endif

	 sampler.max_anisotropy = texobj->MaxAnisotropy;

         /* only care about ARB_shadow, not SGI shadow */
         if (texobj->CompareMode == GL_COMPARE_R_TO_TEXTURE) {
            sampler.compare = 1;
            sampler.compare_mode = PIPE_TEX_COMPARE_R_TO_TEXTURE;
            sampler.compare_func
               = st_compare_func_to_pipe(texobj->CompareFunc);
         }

         /* XXX more sampler state here */
      }

      cso = st_cached_sampler_state(st, &sampler);

      if (cso != st->state.sampler[su]) {
         /* state has changed */
         st->state.sampler[su] = cso;
         st->pipe->bind_sampler_state(st->pipe, su, cso->data);
      }
   }
}


const struct st_tracked_state st_update_sampler = {
   .name = "st_update_sampler",
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .st  = 0,
   },
   .update = update_samplers
};






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


/**
 * Framebuffer/renderbuffer functions.
 *
 * \author Brian Paul
 */


#include "main/imports.h"
#include "main/context.h"
#include "main/fbobject.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "pipe/p_winsys.h"
#include "st_context.h"
#include "st_cb_fbo.h"
#include "st_cb_texture.h"
#include "st_format.h"
#include "st_public.h"



/**
 * Compute the renderbuffer's Red/Green/EtcBit fields from the pipe format.
 */
static int
init_renderbuffer_bits(struct st_renderbuffer *strb, uint pipeFormat)
{
   struct pipe_format_info info;

   if (!st_get_format_info( pipeFormat, &info )) {
      assert( 0 );
   }

   strb->Base._ActualFormat = info.base_format;
   strb->Base.RedBits = info.red_bits;
   strb->Base.GreenBits = info.green_bits;
   strb->Base.BlueBits = info.blue_bits;
   strb->Base.AlphaBits = info.alpha_bits;
   strb->Base.DepthBits = info.depth_bits;
   strb->Base.StencilBits = info.stencil_bits;
   strb->Base.DataType = st_format_datatype(pipeFormat);

   return info.size;
}


/**
 * gl_renderbuffer::AllocStorage()
 */
static GLboolean
st_renderbuffer_alloc_storage(GLcontext * ctx, struct gl_renderbuffer *rb,
                              GLenum internalFormat,
                              GLuint width, GLuint height)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   const uint pipeFormat
      = st_choose_pipe_format(pipe, internalFormat, GL_NONE, GL_NONE);
   GLuint cpp;
   GLbitfield flags = PIPE_SURFACE_FLAG_RENDER; /* want to render to surface */

   cpp = init_renderbuffer_bits(strb, pipeFormat);

   if (strb->surface && strb->surface->format != pipeFormat) {
      /* need to change surface types, free this surface */
      pipe_surface_reference(&strb->surface, NULL);
      assert(strb->surface == NULL);
   }

   if (!strb->surface) {
      strb->surface = pipe->winsys->surface_alloc(pipe->winsys, pipeFormat);
      assert(strb->surface);
      if (!strb->surface)
         return GL_FALSE;
      strb->surface->cpp = cpp;
      strb->surface->pitch = pipe->winsys->surface_pitch(pipe->winsys, cpp,
							 width, flags);
   }

   /* free old region */
   if (strb->surface->region) {
      /* loop here since mapping is refcounted */
      struct pipe_region *r = strb->surface->region;
      while (r->map)
         pipe->region_unmap(pipe, r);
      pipe->winsys->region_release(pipe->winsys, &strb->surface->region);
   }

   strb->surface->region = pipe->winsys->region_alloc(pipe->winsys,
						      strb->surface->pitch *
						      cpp * height, flags);
   if (!strb->surface->region)
      return GL_FALSE; /* out of memory, try s/w buffer? */

   ASSERT(strb->surface->region->buffer);
   ASSERT(strb->surface->format);

   strb->Base.Width  = strb->surface->width  = width;
   strb->Base.Height = strb->surface->height = height;

   return GL_TRUE;
}


/**
 * gl_renderbuffer::Delete()
 */
static void
st_renderbuffer_delete(struct gl_renderbuffer *rb)
{
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   ASSERT(strb);
   if (strb->surface) {
      struct pipe_winsys *ws = strb->surface->winsys;
      ws->surface_release(ws, &strb->surface);
   }
   free(strb);
}


/**
 * gl_renderbuffer::GetPointer()
 */
static void *
null_get_pointer(GLcontext * ctx, struct gl_renderbuffer *rb,
                 GLint x, GLint y)
{
   /* By returning NULL we force all software rendering to go through
    * the span routines.
    */
#if 0
   assert(0);  /* Should never get called with softpipe */
#endif
   return NULL;
}


/**
 * Called via ctx->Driver.NewFramebuffer()
 */
static struct gl_framebuffer *
st_new_framebuffer(GLcontext *ctx, GLuint name)
{
   /* XXX not sure we need to subclass gl_framebuffer for pipe */
   return _mesa_new_framebuffer(ctx, name);
}


/**
 * Called via ctx->Driver.NewRenderbuffer()
 */
static struct gl_renderbuffer *
st_new_renderbuffer(GLcontext *ctx, GLuint name)
{
   struct st_renderbuffer *strb = CALLOC_STRUCT(st_renderbuffer);
   if (strb) {
      _mesa_init_renderbuffer(&strb->Base, name);
      strb->Base.Delete = st_renderbuffer_delete;
      strb->Base.AllocStorage = st_renderbuffer_alloc_storage;
      strb->Base.GetPointer = null_get_pointer;
      return &strb->Base;
   }
   return NULL;
}


/**
 * Allocate a renderbuffer for a an on-screen window (not a user-created
 * renderbuffer).  The window system code determines the internal format.
 */
struct gl_renderbuffer *
st_new_renderbuffer_fb(GLenum intFormat)
{
   struct st_renderbuffer *strb;

   strb = CALLOC_STRUCT(st_renderbuffer);
   if (!strb) {
      _mesa_error(NULL, GL_OUT_OF_MEMORY, "creating renderbuffer");
      return NULL;
   }

   _mesa_init_renderbuffer(&strb->Base, 0);
   strb->Base.ClassID = 0x4242; /* just a unique value */
   strb->Base.InternalFormat = intFormat;

   switch (intFormat) {
   case GL_RGB5:
   case GL_RGBA8:
   case GL_RGBA16:
      strb->Base._BaseFormat = GL_RGBA;
      break;
   case GL_DEPTH_COMPONENT16:
   case GL_DEPTH_COMPONENT32:
      strb->Base._BaseFormat = GL_DEPTH_COMPONENT;
      break;
   case GL_DEPTH24_STENCIL8_EXT:
      strb->Base._BaseFormat = GL_DEPTH_STENCIL_EXT;
      break;
   case GL_STENCIL_INDEX8_EXT:
      strb->Base._BaseFormat = GL_STENCIL_INDEX;
      break;
   default:
      _mesa_problem(NULL,
		    "Unexpected intFormat in st_new_renderbuffer");
      return NULL;
   }

   /* st-specific methods */
   strb->Base.Delete = st_renderbuffer_delete;
   strb->Base.AllocStorage = st_renderbuffer_alloc_storage;
   strb->Base.GetPointer = null_get_pointer;

   /* surface is allocated in st_renderbuffer_alloc_storage() */
   strb->surface = NULL;

   return &strb->Base;
}



/**
 * Called via ctx->Driver.BindFramebufferEXT().
 */
static void
st_bind_framebuffer(GLcontext *ctx, GLenum target,
                    struct gl_framebuffer *fb, struct gl_framebuffer *fbread)
{

}

/**
 * Called by ctx->Driver.FramebufferRenderbuffer
 */
static void
st_framebuffer_renderbuffer(GLcontext *ctx, 
                            struct gl_framebuffer *fb,
                            GLenum attachment,
                            struct gl_renderbuffer *rb)
{
   /* XXX no need for derivation? */
   _mesa_framebuffer_renderbuffer(ctx, fb, attachment, rb);
}


/**
 * Called by ctx->Driver.RenderTexture
 */
static void
st_render_texture(GLcontext *ctx,
                  struct gl_framebuffer *fb,
                  struct gl_renderbuffer_attachment *att)
{
   struct st_context *st = ctx->st;
   struct st_renderbuffer *strb;
   struct gl_renderbuffer *rb;
   struct pipe_context *pipe = st->pipe;
   struct pipe_texture *pt;

   assert(!att->Renderbuffer);

   /* create new renderbuffer which wraps the texture image */
   rb = st_new_renderbuffer(ctx, 0);
   if (!rb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glFramebufferTexture()");
      return;
   }

   _mesa_reference_renderbuffer(&att->Renderbuffer, rb);
   assert(rb->RefCount == 1);
   rb->AllocStorage = NULL; /* should not get called */
   strb = st_renderbuffer(rb);

   /* get the texture for the texture object */
   pt = st_get_texobj_texture(att->Texture);
   assert(pt);
   assert(pt->width[att->TextureLevel]);

   rb->Width = pt->width[att->TextureLevel];
   rb->Height = pt->height[att->TextureLevel];

   /* the renderbuffer's surface is inside the texture */
   strb->surface = pipe->get_tex_surface(pipe, pt,
                                         att->CubeMapFace,
                                         att->TextureLevel,
                                         att->Zoffset);
   assert(strb->surface);

   init_renderbuffer_bits(strb, pt->format);

   /*
   printf("RENDER TO TEXTURE obj=%p pt=%p surf=%p  %d x %d\n",
          att->Texture, pt, strb->surface, rb->Width, rb->Height);
   */

   /* Invalidate buffer state so that the pipe's framebuffer state
    * gets updated.
    * That's where the new renderbuffer (which we just created) gets
    * passed to the pipe as a (color/depth) render target.
    */
   st_invalidate_state(ctx, _NEW_BUFFERS);
}


/**
 * Called via ctx->Driver.FinishRenderTexture.
 */
static void
st_finish_render_texture(GLcontext *ctx,
                         struct gl_renderbuffer_attachment *att)
{
   struct st_renderbuffer *strb = st_renderbuffer(att->Renderbuffer);

   assert(strb);

   ctx->st->pipe->flush(ctx->st->pipe, 0x0);

   /*
   printf("FINISH RENDER TO TEXTURE surf=%p\n", strb->surface);
   */

   pipe_surface_reference(&strb->surface, NULL);

   _mesa_reference_renderbuffer(&att->Renderbuffer, NULL);

   /* restore previous framebuffer state */
   st_invalidate_state(ctx, _NEW_BUFFERS);
}



void st_init_fbo_functions(struct dd_function_table *functions)
{
   functions->NewFramebuffer = st_new_framebuffer;
   functions->NewRenderbuffer = st_new_renderbuffer;
   functions->BindFramebuffer = st_bind_framebuffer;
   functions->FramebufferRenderbuffer = st_framebuffer_renderbuffer;
   functions->RenderTexture = st_render_texture;
   functions->FinishRenderTexture = st_finish_render_texture;
   /* no longer needed by core Mesa, drivers handle resizes...
   functions->ResizeBuffers = st_resize_buffers;
   */
}

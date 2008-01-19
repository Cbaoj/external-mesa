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

#ifndef ST_PUBLIC_H
#define ST_PUBLIC_H

#include "GL/gl.h"
#include "GL/internal/glcore.h"  /* for __GLcontextModes */

#include "pipe/p_compiler.h"
#include "pipe/p_format.h"


#define ST_SURFACE_FRONT_LEFT   0
#define ST_SURFACE_BACK_LEFT    1
#define ST_SURFACE_FRONT_RIGHT  2
#define ST_SURFACE_BACK_RIGHT   3
#define ST_SURFACE_DEPTH        8


struct st_context;
struct st_framebuffer;
struct pipe_context;
struct pipe_surface;


struct st_context *st_create_context(struct pipe_context *pipe,
                                     const __GLcontextModes *visual,
                                     struct st_context *share);

void st_destroy_context( struct st_context *st );

void st_copy_context_state(struct st_context *dst, struct st_context *src,
                           uint mask);

struct st_framebuffer *st_create_framebuffer( const __GLcontextModes *visual,
                                              enum pipe_format colorFormat,
                                              enum pipe_format depthFormat,
                                              enum pipe_format stencilFormat,
                                              uint width, uint height,
                                              void *privateData);

void st_resize_framebuffer( struct st_framebuffer *stfb,
                            uint width, uint height );

struct pipe_surface *st_get_framebuffer_surface(struct st_framebuffer *stfb,
                                                uint surfIndex);

void *st_framebuffer_private( struct st_framebuffer *stfb );

void st_unreference_framebuffer( struct st_framebuffer **stfb );

void st_make_current(struct st_context *st,
                     struct st_framebuffer *draw,
                     struct st_framebuffer *read);

void st_flush( struct st_context *st, uint pipeFlushFlags );

void st_notify_swapbuffers(struct st_framebuffer *stfb);

#endif

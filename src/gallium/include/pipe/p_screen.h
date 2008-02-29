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
 * Screen, Adapter or GPU
 *
 * These are driver functions/facilities that are context independent.
 */


#ifndef P_SCREEN_H
#define P_SCREEN_H


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"



#ifdef __cplusplus
extern "C" {
#endif



/**
 * Gallium screen/adapter context.  Basically everything
 * hardware-specific that doesn't actually require a rendering
 * context.
 */
struct pipe_screen {
   struct pipe_winsys *winsys;

   void (*destroy)( struct pipe_screen * );


   /* 
    * Capability queries
    */
   const char *(*get_name)( struct pipe_screen * );

   const char *(*get_vendor)( struct pipe_screen * );

   int (*get_param)( struct pipe_screen *, int param );

   float (*get_paramf)( struct pipe_screen *, int param );

   boolean (*is_format_supported)( struct pipe_screen *,
                                   enum pipe_format format, 
                                   uint type );


   /*
    * Texture functions
    */
   struct pipe_texture * (*texture_create)(struct pipe_screen *,
                                           const struct pipe_texture *templat);

   void (*texture_release)(struct pipe_screen *,
                           struct pipe_texture **pt);

   /** Get a surface which is a "view" into a texture */
   struct pipe_surface *(*get_tex_surface)(struct pipe_screen *,
                                           struct pipe_texture *texture,
                                           unsigned face, unsigned level,
                                           unsigned zslice);
};


#ifdef __cplusplus
}
#endif

#endif /* P_SCREEN_H */

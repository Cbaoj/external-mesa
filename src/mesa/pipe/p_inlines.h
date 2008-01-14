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

#ifndef P_INLINES_H
#define P_INLINES_H

#include "p_context.h"
#include "p_defines.h"
#include "p_winsys.h"


static INLINE void *
pipe_surface_map(struct pipe_surface *surface)
{
   return (char *)surface->winsys->buffer_map( surface->winsys, surface->buffer,
					       PIPE_BUFFER_FLAG_WRITE |
					       PIPE_BUFFER_FLAG_READ )
      + surface->offset;
}

static INLINE void
pipe_surface_unmap(struct pipe_surface *surface)
{
   surface->winsys->buffer_unmap( surface->winsys, surface->buffer );
}

/**
 * Set 'ptr' to point to 'surf' and update reference counting.
 * The old thing pointed to, if any, will be unreferenced first.
 * 'surf' may be NULL.
 */
static INLINE void
pipe_surface_reference(struct pipe_surface **ptr, struct pipe_surface *surf)
{
   assert(ptr);
   if (*ptr) {
      struct pipe_winsys *winsys = (*ptr)->winsys;
      winsys->surface_release(winsys, ptr);
      assert(!*ptr);
   }
   if (surf) {
      /* reference the new thing */
      surf->refcount++;
      *ptr = surf;
   }
}


/**
 * \sa pipe_surface_reference
 */
static INLINE void
pipe_texture_reference(struct pipe_context *pipe, struct pipe_texture **ptr,
		       struct pipe_texture *pt)
{
   assert(ptr);
   if (*ptr) {
      pipe->texture_release(pipe, ptr);
      assert(!*ptr);
   }
   if (pt) {
      /* reference the new thing */
      pt->refcount++;
      *ptr = pt;
   }
}


#endif /* P_INLINES_H */

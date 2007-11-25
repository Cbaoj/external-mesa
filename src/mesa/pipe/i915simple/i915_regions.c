/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/* Provide additional functionality on top of bufmgr buffers:
 *   - 2d semantics and blit operations (XXX: remove/simplify blits??)
 *   - refcounting of buffers for multiple images in a buffer.
 *   - refcounting of buffer mappings.
 */

#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "i915_context.h"
#include "i915_blit.h"




static ubyte *
i915_region_map(struct pipe_context *pipe, struct pipe_region *region)
{
   struct i915_context *i915 = i915_context( pipe );

   if (!region->map_refcount++) {
      region->map = i915->pipe.winsys->buffer_map( i915->pipe.winsys,
						   region->buffer,
						   PIPE_BUFFER_FLAG_WRITE | 
						   PIPE_BUFFER_FLAG_READ);
   }

   return region->map;
}

static void
i915_region_unmap(struct pipe_context *pipe, struct pipe_region *region)
{
   struct i915_context *i915 = i915_context( pipe );

   if (region->map_refcount > 0) {
      assert(region->map);
      if (!--region->map_refcount) {
         i915->pipe.winsys->buffer_unmap( i915->pipe.winsys,
                                          region->buffer );
         region->map = NULL;
      }
   }
}


/*
 * XXX Move this into core Mesa?
 */
static void
_mesa_copy_rect(ubyte * dst,
                unsigned cpp,
                unsigned dst_pitch,
                unsigned dst_x,
                unsigned dst_y,
                unsigned width,
                unsigned height,
                const ubyte * src,
                unsigned src_pitch,
		unsigned src_x, 
		unsigned src_y)
{
   unsigned i;

   dst_pitch *= cpp;
   src_pitch *= cpp;
   dst += dst_x * cpp;
   src += src_x * cpp;
   dst += dst_y * dst_pitch;
   src += src_y * dst_pitch;
   width *= cpp;

   if (width == dst_pitch && width == src_pitch)
      memcpy(dst, src, height * width);
   else {
      for (i = 0; i < height; i++) {
         memcpy(dst, src, width);
         dst += dst_pitch;
         src += src_pitch;
      }
   }
}


/* Upload data to a rectangular sub-region.  Lots of choices how to do this:
 *
 * - memcpy by span to current destination
 * - upload data as new buffer and blit
 *
 * Currently always memcpy.
 */
static void
i915_region_data(struct pipe_context *pipe,
	       struct pipe_region *dst,
	       unsigned dst_offset,
	       unsigned dstx, unsigned dsty,
	       const void *src, unsigned src_pitch,
	       unsigned srcx, unsigned srcy, unsigned width, unsigned height)
{
   _mesa_copy_rect(pipe->region_map(pipe, dst) + dst_offset,
                   dst->cpp,
                   dst->pitch,
                   dstx, dsty, width, height, src, src_pitch, srcx, srcy);

   pipe->region_unmap(pipe, dst);
}


/* Assumes all values are within bounds -- no checking at this level -
 * do it higher up if required.
 */
static void
i915_region_copy(struct pipe_context *pipe,
	       struct pipe_region *dst,
	       unsigned dst_offset,
	       unsigned dstx, unsigned dsty,
	       struct pipe_region *src,
	       unsigned src_offset,
	       unsigned srcx, unsigned srcy, unsigned width, unsigned height)
{
   assert( dst != src );
   assert( dst->cpp == src->cpp );

   if (0) {
      _mesa_copy_rect(pipe->region_map(pipe, dst) + dst_offset,
		      dst->cpp,
		      dst->pitch,
		      dstx, dsty, 
		      width, height, 
		      pipe->region_map(pipe, src) + src_offset, 
		      src->pitch, 
		      srcx, srcy);

      pipe->region_unmap(pipe, src);
      pipe->region_unmap(pipe, dst);
   }
   else {
      i915_copy_blit( i915_context(pipe),
		      dst->cpp,
		      (short) src->pitch, src->buffer, src_offset,
		      (short) dst->pitch, dst->buffer, dst_offset,
		      (short) srcx, (short) srcy, (short) dstx, (short) dsty, (short) width, (short) height );
   }
}

/* Fill a rectangular sub-region.  Need better logic about when to
 * push buffers into AGP - will currently do so whenever possible.
 */
static ubyte *
get_pointer(struct pipe_region *dst, unsigned x, unsigned y)
{
   return dst->map + (y * dst->pitch + x) * dst->cpp;
}


static void
i915_region_fill(struct pipe_context *pipe,
               struct pipe_region *dst,
               unsigned dst_offset,
               unsigned dstx, unsigned dsty,
               unsigned width, unsigned height, unsigned value)
{
   if (0) {
      unsigned i, j;

      (void)pipe->region_map(pipe, dst);

      switch (dst->cpp) {
      case 1: {
	 ubyte *row = get_pointer(dst, dstx, dsty);
	 for (i = 0; i < height; i++) {
	    memset(row, value, width);
	    row += dst->pitch;
	 }
      }
	 break;
      case 2: {
	 ushort *row = (ushort *) get_pointer(dst, dstx, dsty);
	 for (i = 0; i < height; i++) {
	    for (j = 0; j < width; j++)
	       row[j] = (ushort) value;
	    row += dst->pitch;
	 }
      }
	 break;
      case 4: {
	 unsigned *row = (unsigned *) get_pointer(dst, dstx, dsty);
	 for (i = 0; i < height; i++) {
	    for (j = 0; j < width; j++)
	       row[j] = value;
	    row += dst->pitch;
	 }
      }
	 break;
      default:
	 assert(0);
	 break;
      }
   }
   else {
      i915_fill_blit( i915_context(pipe),
		      dst->cpp,
		      (short) dst->pitch, 
		      dst->buffer, dst_offset,
		      (short) dstx, (short) dsty, 
		      (short) width, (short) height, 
		      value );
   }
}





void
i915_init_region_functions(struct i915_context *i915)
{
   i915->pipe.region_map = i915_region_map;
   i915->pipe.region_unmap = i915_region_unmap;
   i915->pipe.region_data = i915_region_data;
   i915->pipe.region_copy = i915_region_copy;
   i915->pipe.region_fill = i915_region_fill;
}


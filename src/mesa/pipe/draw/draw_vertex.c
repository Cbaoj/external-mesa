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
 * Functions for specifying the post-transformation vertex layout.
 *
 * Author:
 *    Brian Paul
 *    Keith Whitwell
 */


#include "pipe/draw/draw_private.h"
#include "pipe/draw/draw_vertex.h"


/**
 * Compute the size of a vertex, in dwords/floats, to update the
 * vinfo->size field.
 */
void
draw_compute_vertex_size(struct vertex_info *vinfo)
{
   uint i;

   vinfo->size = 0;
   for (i = 0; i < vinfo->num_attribs; i++) {
      switch (vinfo->format[i]) {
      case FORMAT_OMIT:
         break;
      case FORMAT_4UB:
         /* fall-through */
      case FORMAT_1F_PSIZE:
         /* fall-through */
      case FORMAT_1F:
         vinfo->size += 1;
         break;
      case FORMAT_2F:
         vinfo->size += 2;
         break;
      case FORMAT_3F:
         vinfo->size += 3;
         break;
      case FORMAT_4F:
         vinfo->size += 4;
         break;
      default:
         assert(0);
      }
   }

   assert(vinfo->size * 4 <= MAX_VERTEX_SIZE);
}

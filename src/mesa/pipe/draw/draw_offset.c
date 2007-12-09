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
 * \brief  polygon offset state
 *
 * \author  Keith Whitwell <keith@tungstengraphics.com>
 * \author  Brian Paul
 */

#include "pipe/p_util.h"
#include "draw_private.h"



struct offset_stage {
   struct draw_stage stage;

   float scale;
   float units;
};



static INLINE struct offset_stage *offset_stage( struct draw_stage *stage )
{
   return (struct offset_stage *) stage;
}


static void offset_begin( struct draw_stage *stage )
{
   struct offset_stage *offset = offset_stage(stage);
   float mrd = 1.0f / 65535.0f; /* XXX this depends on depthbuffer bits! */

   offset->units = stage->draw->rasterizer->offset_units * mrd;
   offset->scale = stage->draw->rasterizer->offset_scale;

   stage->next->begin( stage->next );
}


/**
 * Offset tri Z.  Some hardware can handle this, but not usually when
 * doing unfilled rendering.
 */
static void do_offset_tri( struct draw_stage *stage,
			   struct prim_header *header )
{
   struct offset_stage *offset = offset_stage(stage);   
   float inv_det = 1.0f / header->det;

   /* Window coords:
    */
   float *v0 = header->v[0]->data[0];
   float *v1 = header->v[1]->data[0];
   float *v2 = header->v[2]->data[0];

   /* edge vectors e = v0 - v2, f = v1 - v2 */
   float ex = v0[0] - v2[0];
   float ey = v0[1] - v2[1];
   float ez = v0[2] - v2[2];
   float fx = v1[0] - v2[0];
   float fy = v1[1] - v2[1];
   float fz = v1[2] - v2[2];

   /* (a,b) = cross(e,f).xy */
   float a = ey*fz - ez*fy;
   float b = ez*fx - ex*fz;

   float dzdx = FABSF(a * inv_det);
   float dzdy = FABSF(b * inv_det);

   float zoffset = offset->units + MAX2(dzdx, dzdy) * offset->scale;

   /*
    * Note: we're applying the offset and clamping per-vertex.
    * Ideally, the offset is applied per-fragment prior to fragment shading.
    */
   v0[2] = CLAMP(v0[2] + zoffset, 0.0f, 1.0f);
   v1[2] = CLAMP(v1[2] + zoffset, 0.0f, 1.0f);
   v2[2] = CLAMP(v2[2] + zoffset, 0.0f, 1.0f);

   stage->next->tri( stage->next, header );
}


static void offset_tri( struct draw_stage *stage,
			struct prim_header *header )
{
   struct prim_header tmp;

   tmp.det = header->det;
   tmp.edgeflags = header->edgeflags;
   tmp.v[0] = dup_vert(stage, header->v[0], 0);
   tmp.v[1] = dup_vert(stage, header->v[1], 1);
   tmp.v[2] = dup_vert(stage, header->v[2], 2);

   do_offset_tri( stage, &tmp );
}



static void offset_line( struct draw_stage *stage,
		       struct prim_header *header )
{
   stage->next->line( stage->next, header );
}


static void offset_point( struct draw_stage *stage,
			struct prim_header *header )
{
   stage->next->point( stage->next, header );
}


static void offset_end( struct draw_stage *stage )
{
   stage->next->end( stage->next );
}


static void offset_reset_stipple_counter( struct draw_stage *stage )
{
   stage->next->reset_stipple_counter( stage->next );
}


static void offset_destroy( struct draw_stage *stage )
{
   draw_free_tmps( stage );
   FREE( stage );
}


/**
 * Create polygon offset drawing stage.
 */
struct draw_stage *draw_offset_stage( struct draw_context *draw )
{
   struct offset_stage *offset = CALLOC_STRUCT(offset_stage);

   draw_alloc_tmps( &offset->stage, 3 );

   offset->stage.draw = draw;
   offset->stage.next = NULL;
   offset->stage.begin = offset_begin;
   offset->stage.point = offset_point;
   offset->stage.line = offset_line;
   offset->stage.tri = offset_tri;
   offset->stage.end = offset_end;
   offset->stage.reset_stipple_counter = offset_reset_stipple_counter;
   offset->stage.destroy = offset_destroy;

   return &offset->stage;
}

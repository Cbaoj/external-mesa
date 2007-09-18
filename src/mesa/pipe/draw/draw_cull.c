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
 * \brief  Drawing stage for polygon culling
 */

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */


#include "pipe/p_util.h"
#include "pipe/p_defines.h"
#include "draw_private.h"


struct cull_stage {
   struct draw_stage stage;
   unsigned winding;  /**< which winding(s) to cull (one of PIPE_WINDING_x) */
};


static INLINE struct cull_stage *cull_stage( struct draw_stage *stage )
{
   return (struct cull_stage *)stage;
}


static void cull_begin( struct draw_stage *stage )
{
   struct cull_stage *cull = cull_stage(stage);

   cull->winding = stage->draw->rasterizer->cull_mode;

   stage->next->begin( stage->next );
}


static void cull_tri( struct draw_stage *stage,
		      struct prim_header *header )
{
   /* Window coords: */
   const float *v0 = header->v[0]->data[0];
   const float *v1 = header->v[1]->data[0];
   const float *v2 = header->v[2]->data[0];

   /* edge vectors e = v0 - v2, f = v1 - v2 */
   const float ex = v0[0] - v2[0];
   const float ey = v0[1] - v2[1];
   const float fx = v1[0] - v2[0];
   const float fy = v1[1] - v2[1];
   
   /* det = cross(e,f).z */
   header->det = ex * fy - ey * fx;

   if (header->det != 0) {
      /* if (det < 0 then Z points toward camera and triangle is 
       * counter-clockwise winding.
       */
      unsigned winding = (header->det < 0) ? PIPE_WINDING_CCW : PIPE_WINDING_CW;

      if ((winding & cull_stage(stage)->winding) == 0) {
         /* triangle is not culled, pass to next stage */
	 stage->next->tri( stage->next, header );
      }
   }
}


static void cull_line( struct draw_stage *stage,
		       struct prim_header *header )
{
   stage->next->line( stage->next, header );
}


static void cull_point( struct draw_stage *stage,
			struct prim_header *header )
{
   stage->next->point( stage->next, header );
}


static void cull_end( struct draw_stage *stage )
{
   stage->next->end( stage->next );
}


static void cull_reset_stipple_counter( struct draw_stage *stage )
{
   stage->next->reset_stipple_counter( stage->next );
}

/**
 * Create a new polygon culling stage.
 */
struct draw_stage *draw_cull_stage( struct draw_context *draw )
{
   struct cull_stage *cull = CALLOC_STRUCT(cull_stage);

   draw_alloc_tmps( &cull->stage, 0 );

   cull->stage.draw = draw;
   cull->stage.next = NULL;
   cull->stage.begin = cull_begin;
   cull->stage.point = cull_point;
   cull->stage.line = cull_line;
   cull->stage.tri = cull_tri;
   cull->stage.end = cull_end;
   cull->stage.reset_stipple_counter = cull_reset_stipple_counter;

   return &cull->stage;
}

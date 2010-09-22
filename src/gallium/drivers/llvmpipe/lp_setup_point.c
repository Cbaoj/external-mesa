/**************************************************************************
 *
 * Copyright 2010, VMware Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/*
 * Binning code for points
 */

#include "lp_setup_context.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "lp_perf.h"
#include "lp_setup_context.h"
#include "lp_rast.h"
#include "lp_state_fs.h"
#include "tgsi/tgsi_scan.h"

#define NUM_CHANNELS 4

struct point_info {
   /* x,y deltas */
   int dy01, dy12;
   int dx01, dx12;

   const float (*v0)[4];
};   


/**
 * Compute a0 for a constant-valued coefficient (GL_FLAT shading).
 */
static void
constant_coef(struct lp_setup_context *setup,
              struct lp_rast_triangle *point,
              unsigned slot,
              const float value,
              unsigned i)
{
   point->inputs.a0[slot][i] = value;
   point->inputs.dadx[slot][i] = 0.0f;
   point->inputs.dady[slot][i] = 0.0f;
}


/**
 * Setup automatic texcoord coefficients (for sprite rendering).
 * \param slot  the vertex attribute slot to setup
 * \param i  the attribute channel in [0,3]
 * \param sprite_coord_origin  one of PIPE_SPRITE_COORD_x
 * \param perspective_proj  will the TEX instruction do a divide by Q?
 */
static void
texcoord_coef(struct lp_setup_context *setup,
              struct lp_rast_triangle *point,
              const struct point_info *info,
              unsigned slot,
              unsigned i,
              unsigned sprite_coord_origin,
              boolean perspective_proj)
{
   assert(i < 4);

   if (i == 0) {
      float dadx = FIXED_ONE / (float)info->dx12;
      float dady =  0.0f;
      float x0 = info->v0[0][0] - setup->pixel_offset;
      float y0 = info->v0[0][1] - setup->pixel_offset;

      point->inputs.dadx[slot][0] = dadx;
      point->inputs.dady[slot][0] = dady;
      point->inputs.a0[slot][0] = 0.5 - (dadx * x0 + dady * y0);

      if (!perspective_proj) {
         /* Divide coefficients by vertex.w here.
          *
          * It would be clearer to always multiply by w0 above and
          * then divide it out for perspective projection here, but
          * doing it this way involves less algebra.
          */
         float w0 = info->v0[0][3];
         point->inputs.dadx[slot][0] *= w0;
         point->inputs.dady[slot][0] *= w0;
         point->inputs.a0[slot][0] *= w0;
      }
   }
   else if (i == 1) {
      float dadx = 0.0f;
      float dady = FIXED_ONE / (float)info->dx12;
      float x0 = info->v0[0][0] - setup->pixel_offset;
      float y0 = info->v0[0][1] - setup->pixel_offset;

      if (sprite_coord_origin == PIPE_SPRITE_COORD_LOWER_LEFT) {
         dady = -dady;
      }

      point->inputs.dadx[slot][1] = dadx;
      point->inputs.dady[slot][1] = dady;
      point->inputs.a0[slot][1] = 0.5 - (dadx * x0 + dady * y0);

      if (!perspective_proj) {
         float w0 = info->v0[0][3];
         point->inputs.dadx[slot][1] *= w0;
         point->inputs.dady[slot][1] *= w0;
         point->inputs.a0[slot][1] *= w0;
      }
   }
   else if (i == 2) {
      point->inputs.a0[slot][2] = 0.0f;
      point->inputs.dadx[slot][2] = 0.0f;
      point->inputs.dady[slot][2] = 0.0f;
   }
   else {
      point->inputs.a0[slot][3] = 1.0f;
      point->inputs.dadx[slot][3] = 0.0f;
      point->inputs.dady[slot][3] = 0.0f;
   }
}


/**
 * Special coefficient setup for gl_FragCoord.
 * X and Y are trivial
 * Z and W are copied from position_coef which should have already been computed.
 * We could do a bit less work if we'd examine gl_FragCoord's swizzle mask.
 */
static void
setup_point_fragcoord_coef(struct lp_setup_context *setup,
                           struct lp_rast_triangle *point,
                           const struct point_info *info,
                           unsigned slot,
                           unsigned usage_mask)
{
   /*X*/
   if (usage_mask & TGSI_WRITEMASK_X) {
      point->inputs.a0[slot][0] = 0.0;
      point->inputs.dadx[slot][0] = 1.0;
      point->inputs.dady[slot][0] = 0.0;
   }

   /*Y*/
   if (usage_mask & TGSI_WRITEMASK_Y) {
      point->inputs.a0[slot][1] = 0.0;
      point->inputs.dadx[slot][1] = 0.0;
      point->inputs.dady[slot][1] = 1.0;
   }

   /*Z*/
   if (usage_mask & TGSI_WRITEMASK_Z) {
      constant_coef(setup, point, slot, info->v0[0][2], 2);
   }

   /*W*/
   if (usage_mask & TGSI_WRITEMASK_W) {
      constant_coef(setup, point, slot, info->v0[0][3], 3);
   }
}


/**
 * Compute the point->coef[] array dadx, dady, a0 values.
 */
static void   
setup_point_coefficients( struct lp_setup_context *setup,
                          struct lp_rast_triangle *point,
                          const struct point_info *info)
{
   const struct lp_fragment_shader *shader = setup->fs.current.variant->shader;
   unsigned fragcoord_usage_mask = TGSI_WRITEMASK_XYZ;
   unsigned slot;

   /* setup interpolation for all the remaining attributes:
    */
   for (slot = 0; slot < setup->fs.nr_inputs; slot++) {
      unsigned vert_attr = setup->fs.input[slot].src_index;
      unsigned usage_mask = setup->fs.input[slot].usage_mask;
      unsigned i;
      
      switch (setup->fs.input[slot].interp) {
      case LP_INTERP_POSITION:
         /*
          * The generated pixel interpolators will pick up the coeffs from
          * slot 0, so all need to ensure that the usage mask is covers all
          * usages.
          */
         fragcoord_usage_mask |= usage_mask;
         break;

      case LP_INTERP_LINEAR:
         /* Sprite tex coords may use linear interpolation someday */
         /* fall-through */

      case LP_INTERP_PERSPECTIVE:
         /* check if the sprite coord flag is set for this attribute.
          * If so, set it up so it up so x and y vary from 0 to 1.
          */
         if (shader->info.input_semantic_name[slot] == TGSI_SEMANTIC_GENERIC) {
            const int index = shader->info.input_semantic_index[slot];
            /* Note that sprite_coord enable is a bitfield of
             * PIPE_MAX_SHADER_OUTPUTS bits.
             */
            if (index < PIPE_MAX_SHADER_OUTPUTS &&
                (setup->sprite_coord_enable & (1 << index))) {
               for (i = 0; i < NUM_CHANNELS; i++)
                  if (usage_mask & (1 << i))
                     texcoord_coef(setup, point, info, slot + 1, i,
                                   setup->sprite_coord_origin,
                                   (usage_mask & TGSI_WRITEMASK_W));
               fragcoord_usage_mask |= TGSI_WRITEMASK_W;
               break;                     
            }
         }

         /* Otherwise fallthrough */
      default:
         for (i = 0; i < NUM_CHANNELS; i++) {
            if (usage_mask & (1 << i))
               constant_coef(setup, point, slot+1, info->v0[vert_attr][i], i);
         }
      }
   }

   /* The internal position input is in slot zero:
    */
   setup_point_fragcoord_coef(setup, point, info, 0,
                              fragcoord_usage_mask);
}


static INLINE int
subpixel_snap(float a)
{
   return util_iround(FIXED_ONE * a);
}


static boolean
try_setup_point( struct lp_setup_context *setup,
                 const float (*v0)[4] )
{
   /* x/y positions in fixed point */
   const int sizeAttr = setup->psize;
   const float size
      = (setup->point_size_per_vertex && sizeAttr > 0) ? v0[sizeAttr][0]
      : setup->point_size;
   
   /* Point size as fixed point integer, remove rounding errors 
    * and gives minimum width for very small points
    */
   int fixed_width = MAX2(FIXED_ONE,
                          (subpixel_snap(size) + FIXED_ONE/2 - 1) & ~(FIXED_ONE-1));

   const int x0 = subpixel_snap(v0[0][0] - setup->pixel_offset) - fixed_width/2;
   const int y0 = subpixel_snap(v0[0][1] - setup->pixel_offset) - fixed_width/2;
     
   struct lp_scene *scene = setup->scene;
   struct lp_rast_triangle *point;
   unsigned bytes;
   struct u_rect bbox;
   unsigned nr_planes = 4;
   struct point_info info;


   /* Bounding rectangle (in pixels) */
   {
      /* Yes this is necessary to accurately calculate bounding boxes
       * with the two fill-conventions we support.  GL (normally) ends
       * up needing a bottom-left fill convention, which requires
       * slightly different rounding.
       */
      int adj = (setup->pixel_offset != 0) ? 1 : 0;

      bbox.x0 = (x0 + (FIXED_ONE-1) + adj) >> FIXED_ORDER;
      bbox.x1 = (x0 + fixed_width + (FIXED_ONE-1) + adj) >> FIXED_ORDER;
      bbox.y0 = (y0 + (FIXED_ONE-1)) >> FIXED_ORDER;
      bbox.y1 = (y0 + fixed_width + (FIXED_ONE-1)) >> FIXED_ORDER;

      /* Inclusive coordinates:
       */
      bbox.x1--;
      bbox.y1--;
   }
   
   if (!u_rect_test_intersection(&setup->draw_region, &bbox)) {
      if (0) debug_printf("offscreen\n");
      LP_COUNT(nr_culled_tris);
      return TRUE;
   }

   u_rect_find_intersection(&setup->draw_region, &bbox);

   point = lp_setup_alloc_triangle(scene,
                                   setup->fs.nr_inputs,
                                   nr_planes,
                                   &bytes);
   if (!point)
      return FALSE;

#ifdef DEBUG
   point->v[0][0] = v0[0][0];
   point->v[0][1] = v0[0][1];
#endif

   info.v0 = v0;
   info.dx01 = 0;
   info.dx12 = fixed_width;
   info.dy01 = fixed_width;
   info.dy12 = 0;
   
   /* Setup parameter interpolants:
    */
   setup_point_coefficients(setup, point, &info);

   point->inputs.facing = 1.0F;
   point->inputs.state = setup->fs.stored;
   point->inputs.disable = FALSE;
   point->inputs.opaque = FALSE;

   {
      point->plane[0].dcdx = -1;
      point->plane[0].dcdy = 0;
      point->plane[0].c = 1-bbox.x0;
      point->plane[0].ei = 0;
      point->plane[0].eo = 1;

      point->plane[1].dcdx = 1;
      point->plane[1].dcdy = 0;
      point->plane[1].c = bbox.x1+1;
      point->plane[1].ei = -1;
      point->plane[1].eo = 0;

      point->plane[2].dcdx = 0;
      point->plane[2].dcdy = 1;
      point->plane[2].c = 1-bbox.y0;
      point->plane[2].ei = 0;
      point->plane[2].eo = 1;

      point->plane[3].dcdx = 0;
      point->plane[3].dcdy = -1;
      point->plane[3].c = bbox.y1+1;
      point->plane[3].ei = -1;
      point->plane[3].eo = 0;
   }

   return lp_setup_bin_triangle(setup, point, &bbox, nr_planes);
}


static void 
lp_setup_point(struct lp_setup_context *setup,
               const float (*v0)[4])
{
   if (!try_setup_point( setup, v0 ))
   {
      lp_setup_flush_and_restart(setup);

      if (!try_setup_point( setup, v0 ))
         assert(0);
   }
}


void 
lp_setup_choose_point( struct lp_setup_context *setup )
{
   setup->point = lp_setup_point;
}



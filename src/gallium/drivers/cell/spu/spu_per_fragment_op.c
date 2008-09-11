/*
 * (C) Copyright IBM Corporation 2008
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * AUTHORS, COPYRIGHT HOLDERS, AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file spu_per_fragment_op.c
 * SPU implementation various per-fragment operations.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include "pipe/p_format.h"
#include "spu_main.h"
#include "spu_per_fragment_op.h"

#define ZERO 0x80


/**
 * Get a "quad" of four fragment Z/stencil values from the given tile.
 * \param tile  the tile of Z/stencil values
 * \param x, y  location of the quad in the tile, in pixels
 * \param depth_format  format of the tile's data
 * \param detph  returns four depth values
 * \param stencil  returns four stencil values
 */
static void
read_ds_quad(tile_t *tile, unsigned x, unsigned y,
             enum pipe_format depth_format, qword *depth,
             qword *stencil)
{
   const int ix = x / 2;
   const int iy = y / 2;

   switch (depth_format) {
   case PIPE_FORMAT_Z16_UNORM: {
      qword *ptr = (qword *) &tile->us8[iy][ix / 2];

      const qword shuf_vec = (qword) {
         ZERO, ZERO, 0, 1, ZERO, ZERO, 2, 3,
         ZERO, ZERO, 4, 5, ZERO, ZERO, 6, 7
      };

      /* At even X values we want the first 4 shorts, and at odd X values we
       * want the second 4 shorts.
       */
      qword bias = (qword) spu_splats((unsigned char) ((ix & 0x01) << 3));
      qword bias_mask = si_fsmbi(0x3333);
      qword sv = si_a(shuf_vec, si_and(bias_mask, bias));

      *depth = si_shufb(*ptr, *ptr, sv);
      *stencil = si_il(0);
      break;
   }

   case PIPE_FORMAT_Z32_UNORM: {
      qword *ptr = (qword *) &tile->ui4[iy][ix];

      *depth = *ptr;
      *stencil = si_il(0);
      break;
   }

   case PIPE_FORMAT_Z24S8_UNORM: {
      qword *ptr = (qword *) &tile->ui4[iy][ix];
      qword mask = si_fsmbi(0xEEEE);

      *depth = si_rotmai(si_and(*ptr, mask), -8);
      *stencil = si_andc(*ptr, mask);
      break;
   }

   case PIPE_FORMAT_S8Z24_UNORM: {
      qword *ptr = (qword *) &tile->ui4[iy][ix];

      *depth = si_and(*ptr, si_fsmbi(0x7777));
      *stencil = si_andi(si_roti(*ptr, 8), 0x0ff);
      break;
   }

   default:
      ASSERT(0);
      break;
   }
}


/**
 * Put a quad of Z/stencil values into a tile.
 * \param tile  the tile of Z/stencil values to write into
 * \param x, y  location of the quad in the tile, in pixels
 * \param depth_format  format of the tile's data
 * \param detph  depth values to store
 * \param stencil  stencil values to store
 */
static void
write_ds_quad(tile_t *buffer, unsigned x, unsigned y,
              enum pipe_format depth_format,
              qword depth, qword stencil)
{
   const int ix = x / 2;
   const int iy = y / 2;

   (void) stencil;

   switch (depth_format) {
   case PIPE_FORMAT_Z16_UNORM: {
      qword *ptr = (qword *) &buffer->us8[iy][ix / 2];

      qword sv = ((ix & 0x01) == 0) 
          ? (qword) { 2, 3, 6, 7, 10, 11, 14, 15,
                      24, 25, 26, 27, 28, 29, 30, 31 }
          : (qword) { 16, 17, 18, 19, 20 , 21, 22, 23,
                      2, 3, 6, 7, 10, 11, 14, 15 };
      *ptr = si_shufb(depth, *ptr, sv);
      break;
   }

   case PIPE_FORMAT_Z32_UNORM: {
      qword *ptr = (qword *) &buffer->ui4[iy][ix];
      *ptr = depth;
      break;
   }

   case PIPE_FORMAT_Z24S8_UNORM: {
      qword *ptr = (qword *) &buffer->ui4[iy][ix];
      /* form select mask = 1110,1110,1110,1110 */
      qword mask = si_fsmbi(0xEEEE);
      /* depth[i] = depth[i] << 8 */
      depth = si_shli(depth, 8);
      /* *ptr[i] = depth[i][31:8] | stencil[i][7:0] */
      *ptr = si_selb(stencil, depth, mask);
      break;
   }

   case PIPE_FORMAT_S8Z24_UNORM: {
      qword *ptr = (qword *) &buffer->ui4[iy][ix];
      /* form select mask = 0111,0111,0111,0111 */
      qword mask = si_fsmbi(0x7777);
      /* stencil[i] = stencil[i] << 24 */
      stencil = si_shli(stencil, 24);
      /* *ptr[i] = stencil[i][31:24] | depth[i][23:0] */
      *ptr = si_selb(stencil, depth, mask);
      break;
   }

   default:
      ASSERT(0);
      break;
   }
}


/**
 * Do depth/stencil/alpha test for a "quad" of 4 fragments.
 * \param x,y  location of quad within tile
 * \param frag_mask  indicates which fragments are "alive"
 * \param frag_depth  four fragment depth values
 * \param frag_alpha  four fragment alpha values
 * \param facing  front/back facing for four fragments (1=front, 0=back)
 */
qword
spu_do_depth_stencil(int x, int y,
                     qword frag_mask, qword frag_depth, qword frag_alpha,
                     qword facing)
{
   struct spu_frag_test_results  result;
   qword pixel_depth;
   qword pixel_stencil;

   /* All of this preable code (everthing before the call to frag_test) should
    * be generated on the PPU and upload to the SPU.
    */
   if (spu.read_depth || spu.read_stencil) {
      read_ds_quad(&spu.ztile, x, y, spu.fb.depth_format,
                   &pixel_depth, &pixel_stencil);
   }

   /* convert floating point Z values to 32-bit uint */

   /* frag_depth *= spu.fb.zscale */
   frag_depth = si_fm(frag_depth, (qword)spu_splats(spu.fb.zscale));
   /* frag_depth = uint(frag_depth) */
   frag_depth = si_cfltu(frag_depth, 0);

   result = (*spu.frag_test)(frag_mask, pixel_depth, pixel_stencil,
                             frag_depth, frag_alpha, facing);


   /* This code (everthing after the call to frag_test) should
    * be generated on the PPU and upload to the SPU.
    */
   if (spu.read_depth || spu.read_stencil) {
      write_ds_quad(&spu.ztile, x, y, spu.fb.depth_format,
                    result.depth, result.stencil);
   }

   return result.mask;
}

/* clang --emit-llvm llvm_builtins.c |llvm-as |opt -std-compile-opts |llvm2cpp -for=Shader -gen-module -funcname=createBaseShader */
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
  * Authors:
  *   Zack Rusin zack@tungstengraphics.com
  */

/* clang --emit-llvm llvm_builtins.c |llvm-as |opt -std-compile-opts |llvm-dis */
typedef __attribute__(( ocu_vector_type(4) )) float float4;

#if 0
//clang doesn't suppoer "struct->member" notation yet
struct vertex_header {
   unsigned clipmask:12;
   unsigned edgeflag:1;
   unsigned pad:3;
   unsigned vertex_id:16;

   float clip[4];

   float data[][4];
};

inline float
dot4(float4 a, float4 b)
{
   float4 c = a*b;
   return c.x + c.y + c.z + c.w;
}

inline unsigned
compute_clipmask(float4 clip, float4 (*plane), unsigned nr)
{
   unsigned mask = 0;
   unsigned i;

   for (i = 0; i < nr; i++) {
      if (dot4(clip, plane[i]) < 0)
         mask |= (1<<i);
   }

   return mask;
}

inline void collect_results(float4 *results, struct vertex_header *vOut,
                            float4 *planes, int nr_planes,
                            float4 scale, float4 trans,
                            int num_attribs)
{
   /* store results */
   unsigned slot;
   float x, y, z, w;

   /* Handle attr[0] (position) specially:
    */
   float4 res0 = results[0];
   float *clip = vOut->clip;
   x = clip[0] = res0.x;
   y = clip[1] = res0.y;
   z = clip[2] = res0.z;
   w = clip[3] = res0.w;

   vOut->clipmask = compute_clipmask(res0, planes, nr_planes);
   vOut->edgeflag = 1;

   /* divide by w */
   w = 1.0f / w;
   x *= w;
   y *= w;
   z *= w;
   res0.x = x; res0.y = y; res0.z = z; res0.w = 1;

   /* Viewport mapping */
   res0 = res0 * scale + trans;
   vOut->data[0][0] = res0.x;
   vOut->data[0][1] = res0.y;
   vOut->data[0][2] = res0.z;
   vOut->data[0][3] = w;

   /* Remaining attributes are packed into sequential post-transform
    * vertex attrib slots.
    * Skip 0 since we just did it above.
    * Subtract two because of the VERTEX_HEADER, CLIP_POS attribs.
    */
   for (slot = 1; slot < num_attribs - 2; slot++) {
      float4 vec = results[slot];
      vOut->data[slot][0] = vec.x;
      vOut->data[slot][1] = vec.y;
      vOut->data[slot][2] = vec.z;
      vOut->data[slot][3] = vec.w;

      printf("output %d: %f %f %f %f\n", slot,
             vOut->data[slot][0],
             vOut->data[slot][1],
             vOut->data[slot][2],
             vOut->data[slot][3]);
   }
}
#endif

void from_array(float4 (*res)[16], float (*ainputs)[16][4],
                int count, int num_attribs)
{
   for (int i = 0; i < count; ++i) {
      for (int j = 0; j < num_attribs; ++j) {
         float4 vec;
         vec.x = ainputs[i][j][0];
         vec.y = ainputs[i][j][1];
         vec.z = ainputs[i][j][2];
         vec.w = ainputs[i][j][3];
         res[i][j] = vec;
      }
   }
}

void from_consts(float4 *res, float (*ainputs)[4],
                 int count)
{
   for (int i = 0; i < count; ++i) {
      float4 vec;
      vec.x = ainputs[i][0];
      vec.y = ainputs[i][1];
      vec.z = ainputs[i][2];
      vec.w = ainputs[i][3];
      res[i] = vec;
   }
}

void to_array(float (*dests)[4], float4 *in, int num_attribs)
{
   for (int i = 0; i < num_attribs; ++i) {
      float  *rd = dests[i];
      float4  ri = in[i];
      rd[0] = ri.x;
      rd[1] = ri.y;
      rd[2] = ri.z;
      rd[3] = ri.w;
   }
}

extern void execute_shader(float4 dests[16], float4 inputs[16],
                           float4 consts[32], float4 temps[128]);

void run_vertex_shader(float (*ainputs)[16][4],
                       float (*dests)[16][4],
                       float (*aconsts)[4],
                       int num_vertices,
                       int num_inputs,
                       int num_attribs,
                       int num_consts)
{
   float4  inputs[16*32*4][16];
   float4  consts[32];
   float4  results[16*32*4][16];
   float4  temps[128];//MAX_PROGRAM_TEMPS

   /*printf("XXX LLVM run_vertex_shader vertices = %d, inputs = %d, attribs = %d, consts = %d\n",
     num_vertices, num_inputs, num_attribs, num_consts);*/
   from_array(inputs, ainputs, num_vertices, num_inputs);
   from_consts(consts, aconsts, num_consts);
   for (int i = 0; i < num_vertices; ++i) {
      float4 *in  = inputs[i];
      float4 *res = results[i];
      execute_shader(res, in, consts, temps);
      to_array(dests[i], res, num_attribs);
   }
}


struct pipe_sampler_state;
struct pipe_mipmap_tree;
struct softpipe_tile_cache;

#define NUM_CHANNELS 4  /* R,G,B,A */
#define QUAD_SIZE    4  /* 4 pixel/quad */

struct tgsi_sampler
{
   const struct pipe_sampler_state *state;
   struct pipe_mipmap_tree *texture;
   /** Get samples for four fragments in a quad */
   void (*get_samples)(struct tgsi_sampler *sampler,
                       const float s[QUAD_SIZE],
                       const float t[QUAD_SIZE],
                       const float p[QUAD_SIZE],
                       float lodbias,
                       float rgba[NUM_CHANNELS][QUAD_SIZE]);
   void *pipe; /*XXX temporary*/
   struct softpipe_tile_cache *cache;
};

struct tgsi_interp_coef
{
   float a0[NUM_CHANNELS];	/* in an xyzw layout */
   float dadx[NUM_CHANNELS];
   float dady[NUM_CHANNELS];
};

int run_fragment_shader(float x, float y,
                        float (*dests)[32][4],
                        struct tgsi_interp_coef *coef,
                        float (*consts)[4],
                        int num_consts,
                        struct tgsi_sampler *samplers,
                        int num_samplers)
{
   float4  inputs[4][16];
   float4  consts[32];
   float4  results[4][16];
   float4  temps[128];//MAX_PROGRAM_TEMPS

   /*printf("XXX LLVM run_vertex_shader vertices = %d, inputs = %d, attribs = %d, consts = %d\n",
     num_vertices, num_inputs, num_attribs, num_consts);*/
   //from_array(inputs, ainputs, num_vertices, num_inputs);
   from_consts(consts, aconsts, num_consts);
   printf("AAAAAAAAAAAAAAAAAAAAAAA FRAGMENT SHADER %f %f\n", x, y);
   for (int i = 0; i < 4; ++i) {
      float4 *in  = inputs[i];
      float4 *res = results[i];
      //execute_shader(res, in, consts, temps);
      to_array(dests[i], res, num_attribs);
   }
}


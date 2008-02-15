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

#if !defined TGSI_EXEC_H
#define TGSI_EXEC_H

#include "pipe/p_compiler.h"

#if defined __cplusplus
extern "C" {
#endif

#define NUM_CHANNELS 4  /* R,G,B,A */
#define QUAD_SIZE    4  /* 4 pixel/quad */

/**
  * Registers may be treated as float, signed int or unsigned int.
  */
union tgsi_exec_channel
{
   float    f[QUAD_SIZE];
   int      i[QUAD_SIZE];
   unsigned u[QUAD_SIZE];
};

/**
  * A vector[RGBA] of channels[4 pixels]
  */
struct tgsi_exec_vector
{
   union tgsi_exec_channel xyzw[NUM_CHANNELS];
};

/**
 * For fragment programs, information for computing fragment input
 * values from plane equation of the triangle/line.
 */
struct tgsi_interp_coef
{
   float a0[NUM_CHANNELS];	/* in an xyzw layout */
   float dadx[NUM_CHANNELS];
   float dady[NUM_CHANNELS];
};


struct softpipe_tile_cache;  /**< Opaque to TGSI */

/**
 * Information for sampling textures, which must be implemented
 * by code outside the TGSI executor.
 */
struct tgsi_sampler
{
   const struct pipe_sampler_state *state;
   struct pipe_texture *texture;
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

/**
 * For branching/calling subroutines.
 */
struct tgsi_exec_labels
{
   unsigned labels[128][2];
   unsigned count;
};

/*
 * Locations of various utility registers (_I = Index, _C = Channel)
 */
#define TGSI_EXEC_TEMP_00000000_I   32
#define TGSI_EXEC_TEMP_00000000_C   0

#define TGSI_EXEC_TEMP_7FFFFFFF_I   32
#define TGSI_EXEC_TEMP_7FFFFFFF_C   1

#define TGSI_EXEC_TEMP_80000000_I   32
#define TGSI_EXEC_TEMP_80000000_C   2

#define TGSI_EXEC_TEMP_FFFFFFFF_I   32
#define TGSI_EXEC_TEMP_FFFFFFFF_C   3

#define TGSI_EXEC_TEMP_ONE_I        33
#define TGSI_EXEC_TEMP_ONE_C        0

#define TGSI_EXEC_TEMP_TWO_I        33
#define TGSI_EXEC_TEMP_TWO_C        1

#define TGSI_EXEC_TEMP_128_I        33
#define TGSI_EXEC_TEMP_128_C        2

#define TGSI_EXEC_TEMP_MINUS_128_I  33
#define TGSI_EXEC_TEMP_MINUS_128_C  3

#define TGSI_EXEC_TEMP_KILMASK_I    34
#define TGSI_EXEC_TEMP_KILMASK_C    0

#define TGSI_EXEC_TEMP_OUTPUT_I     34
#define TGSI_EXEC_TEMP_OUTPUT_C     1

#define TGSI_EXEC_TEMP_PRIMITIVE_I  34
#define TGSI_EXEC_TEMP_PRIMITIVE_C  2

#define TGSI_EXEC_TEMP_R0           35

#define TGSI_EXEC_NUM_TEMPS   (32 + 4)
#define TGSI_EXEC_NUM_ADDRS   1
#define TGSI_EXEC_NUM_IMMEDIATES  256

#define TGSI_EXEC_MAX_COND_NESTING  10
#define TGSI_EXEC_MAX_LOOP_NESTING  10
#define TGSI_EXEC_MAX_CALL_NESTING  10

/**
 * Run-time virtual machine state for executing TGSI shader.
 */
struct tgsi_exec_machine
{
   /*
    * 32 program temporaries
    * 4  internal temporaries
    * 1  address
    * 1  temporary of padding to align to 16 bytes
    */
   struct tgsi_exec_vector       _Temps[TGSI_EXEC_NUM_TEMPS + TGSI_EXEC_NUM_ADDRS + 1];

   /*
    * This will point to _Temps after aligning to 16B boundary.
    */
   struct tgsi_exec_vector       *Temps;
   struct tgsi_exec_vector       *Addrs;

   struct tgsi_sampler           *Samplers;

   float                         Imms[TGSI_EXEC_NUM_IMMEDIATES][4];
   unsigned                      ImmLimit;
   float                         (*Consts)[4];
   struct tgsi_exec_vector       *Inputs;
   struct tgsi_exec_vector       *Outputs;
   const struct tgsi_token       *Tokens;
   unsigned                      Processor;

   /* GEOMETRY processor only. */
   unsigned                      *Primitives;

   /* FRAGMENT processor only. */
   const struct tgsi_interp_coef *InterpCoefs;
   struct tgsi_exec_vector       QuadPos;

   /* Conditional execution masks */
   uint CondMask;  /**< For IF/ELSE/ENDIF */
   uint LoopMask;  /**< For BGNLOOP/ENDLOOP */
   uint ContMask;  /**< For loop CONT statements */
   uint FuncMask;  /**< For function calls */
   uint ExecMask;  /**< = CondMask & LoopMask */

   /** Condition mask stack (for nested conditionals) */
   uint CondStack[TGSI_EXEC_MAX_COND_NESTING];
   int CondStackTop;

   /** Loop mask stack (for nested loops) */
   uint LoopStack[TGSI_EXEC_MAX_LOOP_NESTING];
   int LoopStackTop;

   /** Loop continue mask stack (see comments in tgsi_exec.c) */
   uint ContStack[TGSI_EXEC_MAX_LOOP_NESTING];
   int ContStackTop;

   /** Function execution mask stack (for executing subroutine code) */
   uint FuncStack[TGSI_EXEC_MAX_CALL_NESTING];
   int FuncStackTop;

   /** Function call stack for saving/restoring the program counter */
   uint CallStack[TGSI_EXEC_MAX_CALL_NESTING];
   int CallStackTop;

   struct tgsi_full_instruction *Instructions;
   uint NumInstructions;

   struct tgsi_full_declaration *Declarations;
   uint NumDeclarations;

   struct tgsi_exec_labels Labels;
};

void
tgsi_exec_machine_init(
   struct tgsi_exec_machine *mach );


void 
tgsi_exec_machine_bind_shader(
   struct tgsi_exec_machine *mach,
   const struct tgsi_token *tokens,
   uint numSamplers,
   struct tgsi_sampler *samplers);

uint
tgsi_exec_machine_run(
   struct tgsi_exec_machine *mach );


void
tgsi_exec_machine_free_data(struct tgsi_exec_machine *mach);


#if defined __cplusplus
} /* extern "C" */
#endif

#endif /* TGSI_EXEC_H */

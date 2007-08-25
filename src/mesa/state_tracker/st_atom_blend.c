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
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Brian Paul
  */
 

#include "st_context.h"
#include "st_atom.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"


/**
 * Convert GLenum blend tokens to pipe tokens.
 * Both blend factors and blend funcs are accepted.
 */
static GLuint
gl_blend_to_sp(GLenum blend)
{
   switch (blend) {
   /* blend functions */
   case GL_FUNC_ADD:
      return PIPE_BLEND_ADD;
   case GL_FUNC_SUBTRACT:
      return PIPE_BLEND_SUBTRACT;
   case GL_FUNC_REVERSE_SUBTRACT:
      return PIPE_BLEND_REVERSE_SUBTRACT;
   case GL_MIN:
      return PIPE_BLEND_MIN;
   case GL_MAX:
      return PIPE_BLEND_MAX;

   /* blend factors */
   case GL_ONE:
      return PIPE_BLENDFACTOR_ONE;
   case GL_SRC_COLOR:
      return PIPE_BLENDFACTOR_SRC_COLOR;
   case GL_SRC_ALPHA:
      return PIPE_BLENDFACTOR_SRC_ALPHA;
   case GL_DST_ALPHA:
      return PIPE_BLENDFACTOR_DST_ALPHA;
   case GL_DST_COLOR:
      return PIPE_BLENDFACTOR_DST_COLOR;
   case GL_SRC_ALPHA_SATURATE:
      return PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE;
   case GL_CONSTANT_COLOR:
      return PIPE_BLENDFACTOR_CONST_COLOR;
   case GL_CONSTANT_ALPHA:
      return PIPE_BLENDFACTOR_CONST_ALPHA;
      /*
      return PIPE_BLENDFACTOR_SRC1_COLOR;
      return PIPE_BLENDFACTOR_SRC1_ALPHA;
      */
   case GL_ZERO:
      return PIPE_BLENDFACTOR_ZERO;
   case GL_ONE_MINUS_SRC_COLOR:
      return PIPE_BLENDFACTOR_INV_SRC_COLOR;
   case GL_ONE_MINUS_SRC_ALPHA:
      return PIPE_BLENDFACTOR_INV_SRC_ALPHA;
   case GL_ONE_MINUS_DST_COLOR:
      return PIPE_BLENDFACTOR_INV_DST_ALPHA;
   case GL_ONE_MINUS_DST_ALPHA:
      return PIPE_BLENDFACTOR_INV_DST_COLOR;
   case GL_ONE_MINUS_CONSTANT_COLOR:
      return PIPE_BLENDFACTOR_INV_CONST_COLOR;
   case GL_ONE_MINUS_CONSTANT_ALPHA:
      return PIPE_BLENDFACTOR_INV_CONST_ALPHA;
      /*
      return PIPE_BLENDFACTOR_INV_SRC1_COLOR;
      return PIPE_BLENDFACTOR_INV_SRC1_ALPHA;
      */
   default:
      assert("invalid GL token in gl_blend_to_sp()" == NULL);
      return 0;
   }
}


/**
 * Convert GLenum logicop tokens to pipe tokens.
 */
static GLuint
gl_logicop_to_sp(GLenum logicop)
{
   switch (logicop) {
   case GL_CLEAR:
      return PIPE_LOGICOP_CLEAR;
   case GL_NOR:
      return PIPE_LOGICOP_NOR;
   case GL_AND_INVERTED:
      return PIPE_LOGICOP_AND_INVERTED;
   case GL_COPY_INVERTED:
      return PIPE_LOGICOP_COPY_INVERTED;
   case GL_AND_REVERSE:
      return PIPE_LOGICOP_AND_REVERSE;
   case GL_INVERT:
      return PIPE_LOGICOP_INVERT;
   case GL_XOR:
      return PIPE_LOGICOP_XOR;
   case GL_NAND:
      return PIPE_LOGICOP_NAND;
   case GL_AND:
      return PIPE_LOGICOP_AND;
   case GL_EQUIV:
      return PIPE_LOGICOP_EQUIV;
   case GL_NOOP:
      return PIPE_LOGICOP_NOOP;
   case GL_OR_INVERTED:
      return PIPE_LOGICOP_OR_INVERTED;
   case GL_COPY:
      return PIPE_LOGICOP_COPY;
   case GL_OR_REVERSE:
      return PIPE_LOGICOP_OR_REVERSE;
   case GL_OR:
      return PIPE_LOGICOP_OR;
   case GL_SET:
      return PIPE_LOGICOP_SET;
   default:
      assert("invalid GL token in gl_logicop_to_sp()" == NULL);
      return 0;
   }
}


static void 
update_blend( struct st_context *st )
{
   struct pipe_blend_state blend;

   memset(&blend, 0, sizeof(blend));

   if (st->ctx->Color.ColorLogicOpEnabled ||
       (st->ctx->Color.BlendEnabled &&
        st->ctx->Color.BlendEquationRGB == GL_LOGIC_OP)) {
      /* logicop enabled */
      blend.logicop_enable = 1;
      blend.logicop_func = gl_logicop_to_sp(st->ctx->Color.LogicOp);
   }
   else if (st->ctx->Color.BlendEnabled) {
      /* blending enabled */
      blend.blend_enable = 1;

      blend.rgb_func = gl_blend_to_sp(st->ctx->Color.BlendEquationRGB);
      if (st->ctx->Color.BlendEquationRGB == GL_MIN ||
          st->ctx->Color.BlendEquationRGB == GL_MAX) {
         /* Min/max are special */
         blend.rgb_src_factor = PIPE_BLENDFACTOR_ONE;
         blend.rgb_dst_factor = PIPE_BLENDFACTOR_ONE;
      }
      else {
         blend.rgb_src_factor = gl_blend_to_sp(st->ctx->Color.BlendSrcRGB);
         blend.rgb_dst_factor = gl_blend_to_sp(st->ctx->Color.BlendDstRGB);
      }

      blend.alpha_func = gl_blend_to_sp(st->ctx->Color.BlendEquationA);
      if (st->ctx->Color.BlendEquationA == GL_MIN ||
          st->ctx->Color.BlendEquationA == GL_MAX) {
         /* Min/max are special */
         blend.alpha_src_factor = PIPE_BLENDFACTOR_ONE;
         blend.alpha_dst_factor = PIPE_BLENDFACTOR_ONE;
      }
      else {
         blend.alpha_src_factor = gl_blend_to_sp(st->ctx->Color.BlendSrcA);
         blend.alpha_dst_factor = gl_blend_to_sp(st->ctx->Color.BlendDstA);
      }
   }
   else {
      /* no blending / logicop */
   }

   /* Colormask - maybe reverse these bits? */
   if (st->ctx->Color.ColorMask[0])
      blend.colormask |= PIPE_MASK_R;
   if (st->ctx->Color.ColorMask[1])
      blend.colormask |= PIPE_MASK_G;
   if (st->ctx->Color.ColorMask[2])
      blend.colormask |= PIPE_MASK_B;
   if (st->ctx->Color.ColorMask[3])
      blend.colormask |= PIPE_MASK_A;

   if (st->ctx->Color.DitherFlag)
      blend.dither = 1;

   if (memcmp(&blend, &st->state.blend, sizeof(blend)) != 0) {
      /* state has changed */
      st->state.blend = blend;  /* struct copy */
      st->pipe->set_blend_state(st->pipe, &blend); /* set new state */
   }

   if (memcmp(st->ctx->Color.BlendColor, &st->state.blend_color, 4 * sizeof(GLfloat)) != 0) {
      /* state has changed */
      st->state.blend_color.color[0] = st->ctx->Color.BlendColor[0];
      st->state.blend_color.color[1] = st->ctx->Color.BlendColor[1];
      st->state.blend_color.color[2] = st->ctx->Color.BlendColor[2];
      st->state.blend_color.color[3] = st->ctx->Color.BlendColor[3];
      st->pipe->set_blend_color(st->pipe, (struct pipe_blend_color *) st->ctx->Color.BlendColor);
   }
}


const struct st_tracked_state st_update_blend = {
   .name = "st_update_blend",
   .dirty = {
      .mesa = (_NEW_COLOR),  /* XXX _NEW_BLEND someday? */
      .st  = 0,
   },
   .update = update_blend
};






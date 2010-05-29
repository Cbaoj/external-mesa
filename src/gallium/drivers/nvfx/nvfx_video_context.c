/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
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

#include "nvfx_video_context.h"
#include <softpipe/sp_video_context.h>

struct pipe_video_context *
nvfx_video_create(struct pipe_screen *screen, enum pipe_video_profile profile,
                  enum pipe_video_chroma_format chroma_format,
                  unsigned width, unsigned height, void *priv)
{
   struct pipe_context *pipe;

   assert(screen);

   pipe = screen->context_create(screen, priv);
   if (!pipe)
      return NULL;

   return sp_video_create_ex(pipe, profile, chroma_format, width, height,
                             VL_MPEG12_MC_RENDERER_BUFFER_PICTURE,
                             VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_ONE,
                             true);
}

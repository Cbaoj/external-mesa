/*
 * Copyright 2008 Ben Skeggs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "nv50_context.h"
#include "pipe/p_defines.h"
#include "pipe/internal/p_winsys_screen.h"
#include "pipe/p_inlines.h"

#include "util/u_tile.h"

static void
nv50_surface_copy(struct pipe_context *pipe, boolean flip,
		  struct pipe_surface *dest, unsigned destx, unsigned desty,
		  struct pipe_surface *src, unsigned srcx, unsigned srcy,
		  unsigned width, unsigned height)
{
	struct nv50_context *nv50 = (struct nv50_context *)pipe;
	struct nouveau_winsys *nvws = nv50->screen->nvws;

	if (flip) {
		desty += height;
		while (height--) {
			nvws->surface_copy(nvws, dest, destx, desty--, src,
					   srcx, srcy++, width, 1);
		}
	} else {
		nvws->surface_copy(nvws, dest, destx, desty, src, srcx, srcy,
				   width, height);
	}
}

static void
nv50_surface_fill(struct pipe_context *pipe, struct pipe_surface *dest,
		  unsigned destx, unsigned desty, unsigned width,
		  unsigned height, unsigned value)
{
	struct nv50_context *nv50 = (struct nv50_context *)pipe;
	struct nouveau_winsys *nvws = nv50->screen->nvws;

	nvws->surface_fill(nvws, dest, destx, desty, width, height, value);
}

static void *
nv50_surface_map(struct pipe_screen *screen, struct pipe_surface *ps,
		 unsigned flags )
{
	struct pipe_winsys *ws = screen->winsys;

	return ws->buffer_map(ws, nv50_surface_buffer(ps), flags);
}

static void
nv50_surface_unmap(struct pipe_screen *pscreen, struct pipe_surface *ps)
{
	struct pipe_winsys *ws = pscreen->winsys;

	ws->buffer_unmap(ws, nv50_surface_buffer(ps));
}

void
nv50_init_surface_functions(struct nv50_context *nv50)
{
	nv50->pipe.surface_copy = nv50_surface_copy;
	nv50->pipe.surface_fill = nv50_surface_fill;
}

void
nv50_surface_init_screen_functions(struct pipe_screen *pscreen)
{
	pscreen->surface_map = nv50_surface_map;
	pscreen->surface_unmap = nv50_surface_unmap;
}


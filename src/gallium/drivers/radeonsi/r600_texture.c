/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *      Jerome Glisse
 *      Corbin Simpson
 */
#include <errno.h>
#include "pipe/p_screen.h"
#include "util/u_format.h"
#include "util/u_format_s3tc.h"
#include "util/u_math.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "pipebuffer/pb_buffer.h"
#include "radeonsi_pipe.h"
#include "r600_resource.h"
#include "sid.h"

/* Copy from a full GPU texture to a transfer's staging one. */
static void r600_copy_to_staging_texture(struct pipe_context *ctx, struct r600_transfer *rtransfer)
{
	struct pipe_transfer *transfer = (struct pipe_transfer*)rtransfer;
	struct pipe_resource *texture = transfer->resource;

	ctx->resource_copy_region(ctx, rtransfer->staging_texture,
				0, 0, 0, 0, texture, transfer->level,
				&transfer->box);
}


/* Copy from a transfer's staging texture to a full GPU one. */
static void r600_copy_from_staging_texture(struct pipe_context *ctx, struct r600_transfer *rtransfer)
{
	struct pipe_transfer *transfer = (struct pipe_transfer*)rtransfer;
	struct pipe_resource *texture = transfer->resource;
	struct pipe_box sbox;

	sbox.x = sbox.y = sbox.z = 0;
	sbox.width = transfer->box.width;
	sbox.height = transfer->box.height;
	/* XXX that might be wrong */
	sbox.depth = 1;
	ctx->resource_copy_region(ctx, texture, transfer->level,
				  transfer->box.x, transfer->box.y, transfer->box.z,
				  rtransfer->staging_texture,
				  0, &sbox);
}

unsigned r600_texture_get_offset(struct r600_resource_texture *rtex,
					unsigned level, unsigned layer)
{
	unsigned offset = rtex->offset[level];

	switch (rtex->resource.b.b.target) {
	case PIPE_TEXTURE_3D:
	case PIPE_TEXTURE_CUBE:
	default:
		return offset + layer * rtex->layer_size[level];
	}
}

static unsigned r600_get_block_alignment(struct pipe_screen *screen,
					 enum pipe_format format,
					 unsigned array_mode)
{
	struct r600_screen* rscreen = (struct r600_screen *)screen;
	unsigned pixsize = util_format_get_blocksize(format);
	int p_align;

	switch(array_mode) {
#if 0
	case V_038000_ARRAY_1D_TILED_THIN1:
		p_align = MAX2(8,
			       ((rscreen->tiling_info.group_bytes / 8 / pixsize)));
		break;
	case V_038000_ARRAY_2D_TILED_THIN1:
		p_align = MAX2(rscreen->tiling_info.num_banks,
			       (((rscreen->tiling_info.group_bytes / 8 / pixsize)) *
				rscreen->tiling_info.num_banks)) * 8;
		break;
	case V_038000_ARRAY_LINEAR_ALIGNED:
		p_align = MAX2(64, rscreen->tiling_info.group_bytes / pixsize);
		break;
	case V_038000_ARRAY_LINEAR_GENERAL:
#endif
	default:
		p_align = rscreen->tiling_info.group_bytes / pixsize;
		break;
	}
	return p_align;
}

static unsigned r600_get_height_alignment(struct pipe_screen *screen,
					  unsigned array_mode)
{
	struct r600_screen* rscreen = (struct r600_screen *)screen;
	int h_align;

	switch (array_mode) {
#if 0
	case V_038000_ARRAY_2D_TILED_THIN1:
		h_align = rscreen->tiling_info.num_channels * 8;
		break;
	case V_038000_ARRAY_1D_TILED_THIN1:
	case V_038000_ARRAY_LINEAR_ALIGNED:
		h_align = 8;
		break;
	case V_038000_ARRAY_LINEAR_GENERAL:
#endif
	default:
		h_align = 1;
		break;
	}
	return h_align;
}

static unsigned r600_get_base_alignment(struct pipe_screen *screen,
					enum pipe_format format,
					unsigned array_mode)
{
	struct r600_screen* rscreen = (struct r600_screen *)screen;
	unsigned pixsize = util_format_get_blocksize(format);
	int p_align = r600_get_block_alignment(screen, format, array_mode);
	int h_align = r600_get_height_alignment(screen, array_mode);
	int b_align;

	switch (array_mode) {
#if 0
	case V_038000_ARRAY_2D_TILED_THIN1:
		b_align = MAX2(rscreen->tiling_info.num_banks * rscreen->tiling_info.num_channels * 8 * 8 * pixsize,
			       p_align * pixsize * h_align);
		break;
	case V_038000_ARRAY_1D_TILED_THIN1:
	case V_038000_ARRAY_LINEAR_ALIGNED:
	case V_038000_ARRAY_LINEAR_GENERAL:
#endif
	default:
		b_align = rscreen->tiling_info.group_bytes;
		break;
	}
	return b_align;
}

static unsigned mip_minify(unsigned size, unsigned level)
{
	unsigned val;
	val = u_minify(size, level);
	if (level > 0)
		val = util_next_power_of_two(val);
	return val;
}

static unsigned r600_texture_get_nblocksx(struct pipe_screen *screen,
					  struct r600_resource_texture *rtex,
					  unsigned level)
{
	struct pipe_resource *ptex = &rtex->resource.b.b;
	unsigned nblocksx, block_align, width;
	unsigned blocksize = util_format_get_blocksize(rtex->real_format);

	if (rtex->pitch_override)
		return rtex->pitch_override / blocksize;

	width = mip_minify(ptex->width0, level);
	nblocksx = util_format_get_nblocksx(rtex->real_format, width);

	block_align = r600_get_block_alignment(screen, rtex->real_format,
					      rtex->array_mode[level]);
	nblocksx = align(nblocksx, block_align);
	return nblocksx;
}

static unsigned r600_texture_get_nblocksy(struct pipe_screen *screen,
					  struct r600_resource_texture *rtex,
					  unsigned level)
{
	struct pipe_resource *ptex = &rtex->resource.b.b;
	unsigned height, tile_height;

	height = mip_minify(ptex->height0, level);
	height = util_format_get_nblocksy(rtex->real_format, height);
	tile_height = r600_get_height_alignment(screen,
						rtex->array_mode[level]);

	/* XXX Hack around an alignment issue. Less tests fail with this.
	 *
	 * The thing is depth-stencil buffers should be tiled, i.e.
	 * the alignment should be >=8. If I make them tiled, stencil starts
	 * working because it no longer overlaps with the depth buffer
	 * in memory, but texturing like drawpix-stencil breaks. */
	if (util_format_is_depth_or_stencil(rtex->real_format) && tile_height < 8)
		tile_height = 8;

	height = align(height, tile_height);
	return height;
}

static void r600_texture_set_array_mode(struct pipe_screen *screen,
					struct r600_resource_texture *rtex,
					unsigned level, unsigned array_mode)
{
	struct pipe_resource *ptex = &rtex->resource.b.b;

	switch (array_mode) {
#if 0
	case V_0280A0_ARRAY_LINEAR_GENERAL:
	case V_0280A0_ARRAY_LINEAR_ALIGNED:
	case V_0280A0_ARRAY_1D_TILED_THIN1:
#endif
	default:
		rtex->array_mode[level] = array_mode;
		break;
#if 0
	case V_0280A0_ARRAY_2D_TILED_THIN1:
	{
		unsigned w, h, tile_height, tile_width;

		tile_height = r600_get_height_alignment(screen, array_mode);
		tile_width = r600_get_block_alignment(screen, rtex->real_format, array_mode);

		w = mip_minify(ptex->width0, level);
		h = mip_minify(ptex->height0, level);
		if (w <= tile_width || h <= tile_height)
			rtex->array_mode[level] = V_0280A0_ARRAY_1D_TILED_THIN1;
		else
			rtex->array_mode[level] = array_mode;
	}
	break;
#endif
	}
}

static void r600_setup_miptree(struct pipe_screen *screen,
			       struct r600_resource_texture *rtex,
			       unsigned array_mode)
{
	struct pipe_resource *ptex = &rtex->resource.b.b;
	enum chip_class chipc = ((struct r600_screen*)screen)->chip_class;
	unsigned size, layer_size, i, offset;
	unsigned nblocksx, nblocksy;

	for (i = 0, offset = 0; i <= ptex->last_level; i++) {
		unsigned blocksize = util_format_get_blocksize(rtex->real_format);
		unsigned base_align = r600_get_base_alignment(screen, rtex->real_format, array_mode);

		r600_texture_set_array_mode(screen, rtex, i, array_mode);

		nblocksx = r600_texture_get_nblocksx(screen, rtex, i);
		nblocksy = r600_texture_get_nblocksy(screen, rtex, i);

		if (chipc >= CAYMAN /*&& array_mode == V_038000_ARRAY_LINEAR_GENERAL*/)
			layer_size = align(nblocksx, 64) * nblocksy * blocksize;
		else
			layer_size = nblocksx * nblocksy * blocksize;

		if (ptex->target == PIPE_TEXTURE_CUBE) {
			if (chipc >= CAYMAN)
				size = layer_size * 8;
		}
		else if (ptex->target == PIPE_TEXTURE_3D)
			size = layer_size * u_minify(ptex->depth0, i);
		else
			size = layer_size * ptex->array_size;

		/* align base image and start of miptree */
		if ((i == 0) || (i == 1))
			offset = align(offset, base_align);
		rtex->offset[i] = offset;
		rtex->layer_size[i] = layer_size;
		rtex->pitch_in_blocks[i] = nblocksx; /* CB talks in elements */
		rtex->pitch_in_bytes[i] = nblocksx * blocksize;

		offset += size;
	}
	rtex->size = offset;
}

/* Figure out whether u_blitter will fallback to a transfer operation.
 * If so, don't use a staging resource.
 */
static boolean permit_hardware_blit(struct pipe_screen *screen,
					const struct pipe_resource *res)
{
	unsigned bind;

	if (util_format_is_depth_or_stencil(res->format))
		bind = PIPE_BIND_DEPTH_STENCIL;
	else
		bind = PIPE_BIND_RENDER_TARGET;

	/* hackaround for S3TC */
	if (util_format_is_compressed(res->format))
		return TRUE;
	    
	if (!screen->is_format_supported(screen,
				res->format,
				res->target,
				res->nr_samples,
                                bind))
		return FALSE;

	if (!screen->is_format_supported(screen,
				res->format,
				res->target,
				res->nr_samples,
                                PIPE_BIND_SAMPLER_VIEW))
		return FALSE;

	switch (res->usage) {
	case PIPE_USAGE_STREAM:
	case PIPE_USAGE_STAGING:
		return FALSE;

	default:
		return TRUE;
	}
}

static boolean r600_texture_get_handle(struct pipe_screen* screen,
					struct pipe_resource *ptex,
					struct winsys_handle *whandle)
{
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)ptex;
	struct r600_resource *resource = &rtex->resource;
	struct r600_screen *rscreen = (struct r600_screen*)screen;

	return rscreen->ws->buffer_get_handle(resource->buf,
					      rtex->pitch_in_bytes[0], whandle);
}

static void r600_texture_destroy(struct pipe_screen *screen,
				 struct pipe_resource *ptex)
{
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)ptex;
	struct r600_resource *resource = &rtex->resource;

	if (rtex->flushed_depth_texture)
		pipe_resource_reference((struct pipe_resource **)&rtex->flushed_depth_texture, NULL);

	if (rtex->stencil)
		pipe_resource_reference((struct pipe_resource **)&rtex->stencil, NULL);

	pb_reference(&resource->buf, NULL);
	FREE(rtex);
}

static const struct u_resource_vtbl r600_texture_vtbl =
{
	r600_texture_get_handle,	/* get_handle */
	r600_texture_destroy,		/* resource_destroy */
	r600_texture_get_transfer,	/* get_transfer */
	r600_texture_transfer_destroy,	/* transfer_destroy */
	r600_texture_transfer_map,	/* transfer_map */
	u_default_transfer_flush_region,/* transfer_flush_region */
	r600_texture_transfer_unmap,	/* transfer_unmap */
	NULL	/* transfer_inline_write */
};

static struct r600_resource_texture *
r600_texture_create_object(struct pipe_screen *screen,
			   const struct pipe_resource *base,
			   unsigned array_mode,
			   unsigned pitch_in_bytes_override,
			   unsigned max_buffer_size,
			   struct pb_buffer *buf,
			   boolean alloc_bo)
{
	struct r600_resource_texture *rtex;
	struct r600_resource *resource;
	struct r600_screen *rscreen = (struct r600_screen*)screen;

	rtex = CALLOC_STRUCT(r600_resource_texture);
	if (rtex == NULL)
		return NULL;

	resource = &rtex->resource;
	resource->b.b = *base;
	resource->b.vtbl = &r600_texture_vtbl;
	pipe_reference_init(&resource->b.b.reference, 1);
	resource->b.b.screen = screen;
	rtex->pitch_override = pitch_in_bytes_override;
	rtex->real_format = base->format;

	/* We must split depth and stencil into two separate buffers on Evergreen. */
	if (!(base->flags & R600_RESOURCE_FLAG_TRANSFER) &&
	    ((struct r600_screen*)screen)->chip_class >= CAYMAN &&
	    util_format_is_depth_and_stencil(base->format)) {
		struct pipe_resource stencil;
		unsigned stencil_pitch_override = 0;

		switch (base->format) {
		case PIPE_FORMAT_Z24_UNORM_S8_UINT:
			rtex->real_format = PIPE_FORMAT_Z24X8_UNORM;
			break;
		case PIPE_FORMAT_S8_UINT_Z24_UNORM:
			rtex->real_format = PIPE_FORMAT_X8Z24_UNORM;
			break;
		case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
			rtex->real_format = PIPE_FORMAT_Z32_FLOAT;
			break;
		default:
			assert(0);
			FREE(rtex);
			return NULL;
		}

		/* Divide the pitch in bytes by 4 for stencil, because it has a smaller pixel size. */
		if (pitch_in_bytes_override) {
			assert(base->format == PIPE_FORMAT_Z24_UNORM_S8_UINT ||
			       base->format == PIPE_FORMAT_S8_UINT_Z24_UNORM);
			stencil_pitch_override = pitch_in_bytes_override / 4;
		}

		/* Allocate the stencil buffer. */
		stencil = *base;
		stencil.format = PIPE_FORMAT_S8_UINT;
		rtex->stencil = r600_texture_create_object(screen, &stencil, array_mode,
							   stencil_pitch_override,
							   max_buffer_size, NULL, FALSE);
		if (!rtex->stencil) {
			FREE(rtex);
			return NULL;
		}
		/* Proceed in creating the depth buffer. */
	}

	/* only mark depth textures the HW can hit as depth textures */
	if (util_format_is_depth_or_stencil(rtex->real_format) && permit_hardware_blit(screen, base))
		rtex->depth = 1;

	r600_setup_miptree(screen, rtex, array_mode);

	/* If we initialized separate stencil for Evergreen. place it after depth. */
	if (rtex->stencil) {
		unsigned stencil_align, stencil_offset;

		stencil_align = r600_get_base_alignment(screen, rtex->stencil->real_format, array_mode);
		stencil_offset = align(rtex->size, stencil_align);

		for (unsigned i = 0; i <= rtex->stencil->resource.b.b.last_level; i++)
			rtex->stencil->offset[i] += stencil_offset;

		rtex->size = stencil_offset + rtex->stencil->size;
	}

	/* Now create the backing buffer. */
	if (!buf && alloc_bo) {
		struct pipe_resource *ptex = &rtex->resource.b.b;
		unsigned base_align = r600_get_base_alignment(screen, ptex->format, array_mode);

		if (!r600_init_resource(rscreen, resource, rtex->size, base_align, base->bind, base->usage)) {
			pipe_resource_reference((struct pipe_resource**)&rtex->stencil, NULL);
			FREE(rtex);
			return NULL;
		}
	} else if (buf) {
		resource->buf = buf;
		resource->cs_buf = rscreen->ws->buffer_get_cs_handle(buf);
		resource->domains = RADEON_DOMAIN_GTT | RADEON_DOMAIN_VRAM;
	}

	if (rtex->stencil) {
		pb_reference(&rtex->stencil->resource.buf, rtex->resource.buf);
		rtex->stencil->resource.cs_buf = rtex->resource.cs_buf;
		rtex->stencil->resource.domains = rtex->resource.domains;
	}
	return rtex;
}

DEBUG_GET_ONCE_BOOL_OPTION(tiling_enabled, "R600_TILING", FALSE);

struct pipe_resource *r600_texture_create(struct pipe_screen *screen,
						const struct pipe_resource *templ)
{
	struct r600_screen *rscreen = (struct r600_screen*)screen;
	unsigned array_mode = 0;

	if (!(templ->flags & R600_RESOURCE_FLAG_TRANSFER) &&
	    !(templ->bind & PIPE_BIND_SCANOUT)) {
#if 0
		if (util_format_is_compressed(templ->format)) {
			array_mode = V_038000_ARRAY_1D_TILED_THIN1;
		}
		else if (debug_get_option_tiling_enabled() &&
			 rscreen->info.drm_minor >= 9 &&
			 permit_hardware_blit(screen, templ)) {
			array_mode = V_038000_ARRAY_2D_TILED_THIN1;
		}
#endif
	}

	return (struct pipe_resource *)r600_texture_create_object(screen, templ, array_mode,
								  0, 0, NULL, TRUE);
}

static struct pipe_surface *r600_create_surface(struct pipe_context *pipe,
						struct pipe_resource *texture,
						const struct pipe_surface *surf_tmpl)
{
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)texture;
	struct r600_surface *surface = CALLOC_STRUCT(r600_surface);
	unsigned level = surf_tmpl->u.tex.level;

	assert(surf_tmpl->u.tex.first_layer == surf_tmpl->u.tex.last_layer);
	if (surface == NULL)
		return NULL;
	/* XXX no offset */
/*	offset = r600_texture_get_offset(rtex, level, surf_tmpl->u.tex.first_layer);*/
	pipe_reference_init(&surface->base.reference, 1);
	pipe_resource_reference(&surface->base.texture, texture);
	surface->base.context = pipe;
	surface->base.format = surf_tmpl->format;
	surface->base.width = mip_minify(texture->width0, level);
	surface->base.height = mip_minify(texture->height0, level);
	surface->base.usage = surf_tmpl->usage;
	surface->base.texture = texture;
	surface->base.u.tex.first_layer = surf_tmpl->u.tex.first_layer;
	surface->base.u.tex.last_layer = surf_tmpl->u.tex.last_layer;
	surface->base.u.tex.level = level;

	surface->aligned_height = r600_texture_get_nblocksy(pipe->screen,
							    rtex, level);
	return &surface->base;
}

static void r600_surface_destroy(struct pipe_context *pipe,
				 struct pipe_surface *surface)
{
	pipe_resource_reference(&surface->texture, NULL);
	FREE(surface);
}

struct pipe_resource *r600_texture_from_handle(struct pipe_screen *screen,
					       const struct pipe_resource *templ,
					       struct winsys_handle *whandle)
{
	struct r600_screen *rscreen = (struct r600_screen*)screen;
	struct pb_buffer *buf = NULL;
	unsigned stride = 0;
	unsigned array_mode = 0;
	enum radeon_bo_layout micro, macro;

	/* Support only 2D textures without mipmaps */
	if ((templ->target != PIPE_TEXTURE_2D && templ->target != PIPE_TEXTURE_RECT) ||
	      templ->depth0 != 1 || templ->last_level != 0)
		return NULL;

	buf = rscreen->ws->buffer_from_handle(rscreen->ws, whandle, &stride);
	if (!buf)
		return NULL;

	rscreen->ws->buffer_get_tiling(buf, &micro, &macro, NULL, NULL, NULL, NULL, NULL);

#if 0
	if (macro == RADEON_LAYOUT_TILED)
		array_mode = V_0280A0_ARRAY_2D_TILED_THIN1;
	else if (micro == RADEON_LAYOUT_TILED)
		array_mode = V_0280A0_ARRAY_1D_TILED_THIN1;
	else
#endif
		array_mode = 0;

	return (struct pipe_resource *)r600_texture_create_object(screen, templ, array_mode,
								  stride, 0, buf, FALSE);
}

int r600_texture_depth_flush(struct pipe_context *ctx,
			     struct pipe_resource *texture, boolean just_create)
{
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)texture;
	struct pipe_resource resource;

	if (rtex->flushed_depth_texture)
		goto out;

	resource.target = texture->target;
	resource.format = texture->format;
	resource.width0 = texture->width0;
	resource.height0 = texture->height0;
	resource.depth0 = texture->depth0;
	resource.array_size = texture->array_size;
	resource.last_level = texture->last_level;
	resource.nr_samples = texture->nr_samples;
	resource.usage = PIPE_USAGE_DYNAMIC;
	resource.bind = texture->bind | PIPE_BIND_DEPTH_STENCIL;
	resource.flags = R600_RESOURCE_FLAG_TRANSFER | texture->flags;

	rtex->flushed_depth_texture = (struct r600_resource_texture *)ctx->screen->resource_create(ctx->screen, &resource);
	if (rtex->flushed_depth_texture == NULL) {
		R600_ERR("failed to create temporary texture to hold untiled copy\n");
		return -ENOMEM;
	}

	((struct r600_resource_texture *)rtex->flushed_depth_texture)->is_flushing_texture = TRUE;
out:
	if (just_create)
		return 0;

	/* XXX: only do this if the depth texture has actually changed:
	 */
	r600_blit_uncompress_depth(ctx, rtex);
	return 0;
}

/* Needs adjustment for pixelformat:
 */
static INLINE unsigned u_box_volume( const struct pipe_box *box )
{
	return box->width * box->depth * box->height;
};

struct pipe_transfer* r600_texture_get_transfer(struct pipe_context *ctx,
						struct pipe_resource *texture,
						unsigned level,
						unsigned usage,
						const struct pipe_box *box)
{
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)texture;
	struct pipe_resource resource;
	struct r600_transfer *trans;
	int r;
	boolean use_staging_texture = FALSE;

#if 0
	/* We cannot map a tiled texture directly because the data is
	 * in a different order, therefore we do detiling using a blit.
	 *
	 * Also, use a temporary in GTT memory for read transfers, as
	 * the CPU is much happier reading out of cached system memory
	 * than uncached VRAM.
	 */
	if (R600_TEX_IS_TILED(rtex, level))
		use_staging_texture = TRUE;
#endif

	if ((usage & PIPE_TRANSFER_READ) && u_box_volume(box) > 1024)
		use_staging_texture = TRUE;

	/* XXX: Use a staging texture for uploads if the underlying BO
	 * is busy.  No interface for checking that currently? so do
	 * it eagerly whenever the transfer doesn't require a readback
	 * and might block.
	 */
	if ((usage & PIPE_TRANSFER_WRITE) &&
			!(usage & (PIPE_TRANSFER_READ |
					PIPE_TRANSFER_DONTBLOCK |
					PIPE_TRANSFER_UNSYNCHRONIZED)))
		use_staging_texture = TRUE;

	if (!permit_hardware_blit(ctx->screen, texture) ||
		(texture->flags & R600_RESOURCE_FLAG_TRANSFER))
		use_staging_texture = FALSE;

	if (use_staging_texture && (usage & PIPE_TRANSFER_MAP_DIRECTLY))
		return NULL;

	trans = CALLOC_STRUCT(r600_transfer);
	if (trans == NULL)
		return NULL;
	pipe_resource_reference(&trans->transfer.resource, texture);
	trans->transfer.level = level;
	trans->transfer.usage = usage;
	trans->transfer.box = *box;
	if (rtex->depth) {
		/* XXX: only readback the rectangle which is being mapped?
		*/
		/* XXX: when discard is true, no need to read back from depth texture
		*/
		r = r600_texture_depth_flush(ctx, texture, FALSE);
		if (r < 0) {
			R600_ERR("failed to create temporary texture to hold untiled copy\n");
			pipe_resource_reference(&trans->transfer.resource, NULL);
			FREE(trans);
			return NULL;
		}
		trans->transfer.stride = rtex->flushed_depth_texture->pitch_in_bytes[level];
		trans->offset = r600_texture_get_offset(rtex->flushed_depth_texture, level, box->z);
		return &trans->transfer;
	} else if (use_staging_texture) {
		resource.target = PIPE_TEXTURE_2D;
		resource.format = texture->format;
		resource.width0 = box->width;
		resource.height0 = box->height;
		resource.depth0 = 1;
		resource.array_size = 1;
		resource.last_level = 0;
		resource.nr_samples = 0;
		resource.usage = PIPE_USAGE_STAGING;
		resource.bind = 0;
		resource.flags = R600_RESOURCE_FLAG_TRANSFER;
		/* For texture reading, the temporary (detiled) texture is used as
		 * a render target when blitting from a tiled texture. */
		if (usage & PIPE_TRANSFER_READ) {
			resource.bind |= PIPE_BIND_RENDER_TARGET;
		}
		/* For texture writing, the temporary texture is used as a sampler
		 * when blitting into a tiled texture. */
		if (usage & PIPE_TRANSFER_WRITE) {
			resource.bind |= PIPE_BIND_SAMPLER_VIEW;
		}
		/* Create the temporary texture. */
		trans->staging_texture = ctx->screen->resource_create(ctx->screen, &resource);
		if (trans->staging_texture == NULL) {
			R600_ERR("failed to create temporary texture to hold untiled copy\n");
			pipe_resource_reference(&trans->transfer.resource, NULL);
			FREE(trans);
			return NULL;
		}

		trans->transfer.stride =
			((struct r600_resource_texture *)trans->staging_texture)->pitch_in_bytes[0];
		if (usage & PIPE_TRANSFER_READ) {
			r600_copy_to_staging_texture(ctx, trans);
			/* Always referenced in the blit. */
			radeonsi_flush(ctx, NULL, 0);
		}
		return &trans->transfer;
	}
	trans->transfer.stride = rtex->pitch_in_bytes[level];
	trans->transfer.layer_stride = rtex->layer_size[level];
	trans->offset = r600_texture_get_offset(rtex, level, box->z);
	return &trans->transfer;
}

void r600_texture_transfer_destroy(struct pipe_context *ctx,
				   struct pipe_transfer *transfer)
{
	struct r600_transfer *rtransfer = (struct r600_transfer*)transfer;
	struct pipe_resource *texture = transfer->resource;
	struct r600_resource_texture *rtex = (struct r600_resource_texture*)texture;

	if (rtransfer->staging_texture) {
		if (transfer->usage & PIPE_TRANSFER_WRITE) {
			r600_copy_from_staging_texture(ctx, rtransfer);
		}
		pipe_resource_reference(&rtransfer->staging_texture, NULL);
	}

	if (rtex->depth && !rtex->is_flushing_texture) {
		if ((transfer->usage & PIPE_TRANSFER_WRITE) && rtex->flushed_depth_texture)
			r600_blit_push_depth(ctx, rtex);
	}

	pipe_resource_reference(&transfer->resource, NULL);
	FREE(transfer);
}

void* r600_texture_transfer_map(struct pipe_context *ctx,
				struct pipe_transfer* transfer)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_transfer *rtransfer = (struct r600_transfer*)transfer;
	struct radeon_winsys_cs_handle *buf;
	enum pipe_format format = transfer->resource->format;
	unsigned offset = 0;
	char *map;

	if (rtransfer->staging_texture) {
		buf = ((struct r600_resource *)rtransfer->staging_texture)->cs_buf;
	} else {
		struct r600_resource_texture *rtex = (struct r600_resource_texture*)transfer->resource;

		if (rtex->flushed_depth_texture)
			buf = ((struct r600_resource *)rtex->flushed_depth_texture)->cs_buf;
		else
			buf = ((struct r600_resource *)transfer->resource)->cs_buf;

		offset = rtransfer->offset +
			transfer->box.y / util_format_get_blockheight(format) * transfer->stride +
			transfer->box.x / util_format_get_blockwidth(format) * util_format_get_blocksize(format);
	}

	if (!(map = rctx->ws->buffer_map(buf, rctx->cs, transfer->usage))) {
		return NULL;
	}

	return map + offset;
}

void r600_texture_transfer_unmap(struct pipe_context *ctx,
				 struct pipe_transfer* transfer)
{
	struct r600_transfer *rtransfer = (struct r600_transfer*)transfer;
	struct r600_context *rctx = (struct r600_context*)ctx;
	struct radeon_winsys_cs_handle *buf;

	if (rtransfer->staging_texture) {
		buf = ((struct r600_resource *)rtransfer->staging_texture)->cs_buf;
	} else {
		struct r600_resource_texture *rtex = (struct r600_resource_texture*)transfer->resource;

		if (rtex->flushed_depth_texture) {
			buf = ((struct r600_resource *)rtex->flushed_depth_texture)->cs_buf;
		} else {
			buf = ((struct r600_resource *)transfer->resource)->cs_buf;
		}
	}
	rctx->ws->buffer_unmap(buf);
}

void r600_init_surface_functions(struct r600_context *r600)
{
	r600->context.create_surface = r600_create_surface;
	r600->context.surface_destroy = r600_surface_destroy;
}

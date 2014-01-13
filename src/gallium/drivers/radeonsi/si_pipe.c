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
 */
#include <stdio.h>
#include <errno.h>
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_context.h"
#include "tgsi/tgsi_scan.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "util/u_blitter.h"
#include "util/u_double_list.h"
#include "util/u_format.h"
#include "util/u_transfer.h"
#include "util/u_surface.h"
#include "util/u_pack_color.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_simple_shaders.h"
#include "util/u_upload_mgr.h"
#include "vl/vl_decoder.h"
#include "vl/vl_video_buffer.h"
#include "os/os_time.h"
#include "pipebuffer/pb_buffer.h"
#include "si_pipe.h"
#include "radeon/radeon_uvd.h"
#include "si.h"
#include "sid.h"
#include "si_resource.h"
#include "si_pipe.h"
#include "si_state.h"
#include "../radeon/r600_cs.h"

/*
 * pipe_context
 */
void si_flush(struct pipe_context *ctx, struct pipe_fence_handle **fence,
	      unsigned flags)
{
	struct si_context *sctx = (struct si_context *)ctx;
	struct pipe_query *render_cond = NULL;
	boolean render_cond_cond = FALSE;
	unsigned render_cond_mode = 0;

	if (fence) {
		*fence = sctx->b.ws->cs_create_fence(sctx->b.rings.gfx.cs);
	}

	/* Disable render condition. */
	if (sctx->current_render_cond) {
		render_cond = sctx->current_render_cond;
		render_cond_cond = sctx->current_render_cond_cond;
		render_cond_mode = sctx->current_render_cond_mode;
		ctx->render_condition(ctx, NULL, FALSE, 0);
	}

	si_context_flush(sctx, flags);

	/* Re-enable render condition. */
	if (render_cond) {
		ctx->render_condition(ctx, render_cond, render_cond_cond, render_cond_mode);
	}
}

static void si_flush_from_st(struct pipe_context *ctx,
			     struct pipe_fence_handle **fence,
			     unsigned flags)
{
	si_flush(ctx, fence,
		 flags & PIPE_FLUSH_END_OF_FRAME ? RADEON_FLUSH_END_OF_FRAME : 0);
}

static void si_flush_from_winsys(void *ctx, unsigned flags)
{
	si_flush((struct pipe_context*)ctx, NULL, flags);
}

static void si_destroy_context(struct pipe_context *context)
{
	struct si_context *sctx = (struct si_context *)context;

	si_release_all_descriptors(sctx);

	pipe_resource_reference(&sctx->null_const_buf.buffer, NULL);
	r600_resource_reference(&sctx->border_color_table, NULL);

	if (sctx->dummy_pixel_shader) {
		sctx->b.b.delete_fs_state(&sctx->b.b, sctx->dummy_pixel_shader);
	}
	for (int i = 0; i < 8; i++) {
		sctx->b.b.delete_depth_stencil_alpha_state(&sctx->b.b, sctx->custom_dsa_flush_depth_stencil[i]);
		sctx->b.b.delete_depth_stencil_alpha_state(&sctx->b.b, sctx->custom_dsa_flush_depth[i]);
		sctx->b.b.delete_depth_stencil_alpha_state(&sctx->b.b, sctx->custom_dsa_flush_stencil[i]);
	}
	sctx->b.b.delete_depth_stencil_alpha_state(&sctx->b.b, sctx->custom_dsa_flush_inplace);
	sctx->b.b.delete_blend_state(&sctx->b.b, sctx->custom_blend_resolve);
	sctx->b.b.delete_blend_state(&sctx->b.b, sctx->custom_blend_decompress);
	util_unreference_framebuffer_state(&sctx->framebuffer);

	util_blitter_destroy(sctx->blitter);

	r600_common_context_cleanup(&sctx->b);
	FREE(sctx);
}

static struct pipe_context *si_create_context(struct pipe_screen *screen, void *priv)
{
	struct si_context *sctx = CALLOC_STRUCT(si_context);
	struct si_screen* sscreen = (struct si_screen *)screen;
	int shader, i;

	if (sctx == NULL)
		return NULL;

	if (!r600_common_context_init(&sctx->b, &sscreen->b))
		goto fail;

	sctx->b.b.screen = screen;
	sctx->b.b.priv = priv;
	sctx->b.b.destroy = si_destroy_context;
	sctx->b.b.flush = si_flush_from_st;

	/* Easy accessing of screen/winsys. */
	sctx->screen = sscreen;

	si_init_blit_functions(sctx);
	si_init_query_functions(sctx);
	si_init_context_resource_functions(sctx);
	si_init_compute_functions(sctx);

	if (sscreen->b.info.has_uvd) {
		sctx->b.b.create_video_codec = si_uvd_create_decoder;
		sctx->b.b.create_video_buffer = si_video_buffer_create;
	} else {
		sctx->b.b.create_video_codec = vl_create_decoder;
		sctx->b.b.create_video_buffer = vl_video_buffer_create;
	}

	sctx->b.rings.gfx.cs = sctx->b.ws->cs_create(sctx->b.ws, RING_GFX, NULL);
	sctx->b.rings.gfx.flush = si_flush_from_winsys;

	si_init_all_descriptors(sctx);

	/* Initialize cache_flush. */
	sctx->cache_flush = si_atom_cache_flush;
	sctx->atoms.cache_flush = &sctx->cache_flush;

	sctx->atoms.streamout_begin = &sctx->b.streamout.begin_atom;

	switch (sctx->b.chip_class) {
	case SI:
	case CIK:
		si_init_state_functions(sctx);
		LIST_INITHEAD(&sctx->active_nontimer_query_list);
		sctx->max_db = 8;
		si_init_config(sctx);
		break;
	default:
		R600_ERR("Unsupported chip class %d.\n", sctx->b.chip_class);
		goto fail;
	}

	sctx->b.ws->cs_set_flush_callback(sctx->b.rings.gfx.cs, si_flush_from_winsys, sctx);

	sctx->blitter = util_blitter_create(&sctx->b.b);
	if (sctx->blitter == NULL)
		goto fail;

	sctx->dummy_pixel_shader =
		util_make_fragment_cloneinput_shader(&sctx->b.b, 0,
						     TGSI_SEMANTIC_GENERIC,
						     TGSI_INTERPOLATE_CONSTANT);
	sctx->b.b.bind_fs_state(&sctx->b.b, sctx->dummy_pixel_shader);

	/* these must be last */
	si_begin_new_cs(sctx);
	si_get_backend_mask(sctx);

	/* CIK cannot unbind a constant buffer (S_BUFFER_LOAD is buggy
	 * with a NULL buffer). We need to use a dummy buffer instead. */
	if (sctx->b.chip_class == CIK) {
		sctx->null_const_buf.buffer = pipe_buffer_create(screen, PIPE_BIND_CONSTANT_BUFFER,
								 PIPE_USAGE_STATIC, 16);
		sctx->null_const_buf.buffer_size = sctx->null_const_buf.buffer->width0;

		for (shader = 0; shader < SI_NUM_SHADERS; shader++) {
			for (i = 0; i < NUM_CONST_BUFFERS; i++) {
				sctx->b.b.set_constant_buffer(&sctx->b.b, shader, i,
							      &sctx->null_const_buf);
			}
		}

		/* Clear the NULL constant buffer, because loads should return zeros. */
		sctx->b.clear_buffer(&sctx->b.b, sctx->null_const_buf.buffer, 0,
				     sctx->null_const_buf.buffer->width0, 0);
	}

	return &sctx->b.b;
fail:
	si_destroy_context(&sctx->b.b);
	return NULL;
}

/*
 * pipe_screen
 */
static const char* si_get_vendor(struct pipe_screen* pscreen)
{
	return "X.Org";
}

const char *si_get_llvm_processor_name(enum radeon_family family)
{
	switch (family) {
		case CHIP_TAHITI: return "tahiti";
		case CHIP_PITCAIRN: return "pitcairn";
		case CHIP_VERDE: return "verde";
		case CHIP_OLAND: return "oland";
#if HAVE_LLVM <= 0x0303
		default: return "SI";
#else
		case CHIP_HAINAN: return "hainan";
		case CHIP_BONAIRE: return "bonaire";
		case CHIP_KABINI: return "kabini";
		case CHIP_KAVERI: return "kaveri";
		case CHIP_HAWAII: return "hawaii";
		default: return "";
#endif
	}
}

static const char *si_get_family_name(enum radeon_family family)
{
	switch(family) {
	case CHIP_TAHITI: return "AMD TAHITI";
	case CHIP_PITCAIRN: return "AMD PITCAIRN";
	case CHIP_VERDE: return "AMD CAPE VERDE";
	case CHIP_OLAND: return "AMD OLAND";
	case CHIP_HAINAN: return "AMD HAINAN";
	case CHIP_BONAIRE: return "AMD BONAIRE";
	case CHIP_KAVERI: return "AMD KAVERI";
	case CHIP_KABINI: return "AMD KABINI";
	case CHIP_HAWAII: return "AMD HAWAII";
	default: return "AMD unknown";
	}
}

static const char* si_get_name(struct pipe_screen* pscreen)
{
	struct si_screen *sscreen = (struct si_screen *)pscreen;

	return si_get_family_name(sscreen->b.family);
}

static int si_get_param(struct pipe_screen* pscreen, enum pipe_cap param)
{
	struct si_screen *sscreen = (struct si_screen *)pscreen;

	switch (param) {
	/* Supported features (boolean caps). */
	case PIPE_CAP_TWO_SIDED_STENCIL:
	case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
	case PIPE_CAP_ANISOTROPIC_FILTER:
	case PIPE_CAP_POINT_SPRITE:
	case PIPE_CAP_OCCLUSION_QUERY:
	case PIPE_CAP_TEXTURE_SHADOW_MAP:
	case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
	case PIPE_CAP_BLEND_EQUATION_SEPARATE:
	case PIPE_CAP_TEXTURE_SWIZZLE:
	case PIPE_CAP_DEPTH_CLIP_DISABLE:
	case PIPE_CAP_SHADER_STENCIL_EXPORT:
	case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
	case PIPE_CAP_MIXED_COLORBUFFER_FORMATS:
	case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
	case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
	case PIPE_CAP_SM3:
	case PIPE_CAP_SEAMLESS_CUBE_MAP:
	case PIPE_CAP_PRIMITIVE_RESTART:
	case PIPE_CAP_CONDITIONAL_RENDER:
	case PIPE_CAP_TEXTURE_BARRIER:
	case PIPE_CAP_INDEP_BLEND_ENABLE:
	case PIPE_CAP_INDEP_BLEND_FUNC:
	case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
	case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
	case PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY:
	case PIPE_CAP_VERTEX_BUFFER_STRIDE_4BYTE_ALIGNED_ONLY:
	case PIPE_CAP_VERTEX_ELEMENT_SRC_OFFSET_4BYTE_ALIGNED_ONLY:
	case PIPE_CAP_USER_INDEX_BUFFERS:
	case PIPE_CAP_USER_CONSTANT_BUFFERS:
	case PIPE_CAP_START_INSTANCE:
	case PIPE_CAP_NPOT_TEXTURES:
	case PIPE_CAP_MIXED_FRAMEBUFFER_SIZES:
        case PIPE_CAP_PREFER_BLIT_BASED_TEXTURE_TRANSFER:
	case PIPE_CAP_TGSI_INSTANCEID:
	case PIPE_CAP_COMPUTE:
	case PIPE_CAP_TEXTURE_BUFFER_OBJECTS:
        case PIPE_CAP_TGSI_VS_LAYER:
		return 1;

	case PIPE_CAP_TEXTURE_MULTISAMPLE:
		/* 2D tiling on CIK is supported since DRM 2.35.0 */
		return HAVE_LLVM >= 0x0304 && (sscreen->b.chip_class < CIK ||
					       sscreen->b.info.drm_minor >= 35);

	case PIPE_CAP_TGSI_TEXCOORD:
		return 0;

        case PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT:
                return 64;

	case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
		return 256;

	case PIPE_CAP_GLSL_FEATURE_LEVEL:
		return 140;

	case PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT:
		return 1;
	case PIPE_CAP_MAX_TEXTURE_BUFFER_SIZE:
		return MIN2(sscreen->b.info.vram_size, 0xFFFFFFFF);

	/* Unsupported features. */
	case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
	case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
	case PIPE_CAP_SCALED_RESOLVE:
	case PIPE_CAP_TGSI_CAN_COMPACT_CONSTANTS:
	case PIPE_CAP_FRAGMENT_COLOR_CLAMPED:
	case PIPE_CAP_VERTEX_COLOR_CLAMPED:
	case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
	case PIPE_CAP_USER_VERTEX_BUFFERS:
	case PIPE_CAP_QUERY_PIPELINE_STATISTICS:
	case PIPE_CAP_CUBE_MAP_ARRAY:
		return 0;

	case PIPE_CAP_TEXTURE_BORDER_COLOR_QUIRK:
		return PIPE_QUIRK_TEXTURE_BORDER_COLOR_SWIZZLE_R600;

	/* Stream output. */
	case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
		return sscreen->b.has_streamout ? 4 : 0;
	case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
		return sscreen->b.has_streamout ? 1 : 0;
	case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
	case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
		return sscreen->b.has_streamout ? 32*4 : 0;

	/* Texturing. */
	case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
	case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
	case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
			return 15;
	case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
		return 16384;
	case PIPE_CAP_MAX_COMBINED_SAMPLERS:
		return 32;

	/* Render targets. */
	case PIPE_CAP_MAX_RENDER_TARGETS:
		return 8;

	case PIPE_CAP_MAX_VIEWPORTS:
		return 1;

	/* Timer queries, present when the clock frequency is non zero. */
	case PIPE_CAP_QUERY_TIMESTAMP:
	case PIPE_CAP_QUERY_TIME_ELAPSED:
		return sscreen->b.info.r600_clock_crystal_freq != 0;

	case PIPE_CAP_MIN_TEXEL_OFFSET:
		return -8;

	case PIPE_CAP_MAX_TEXEL_OFFSET:
		return 7;
	case PIPE_CAP_ENDIANNESS:
		return PIPE_ENDIAN_LITTLE;
	}
	return 0;
}

static float si_get_paramf(struct pipe_screen* pscreen,
			     enum pipe_capf param)
{
	switch (param) {
	case PIPE_CAPF_MAX_LINE_WIDTH:
	case PIPE_CAPF_MAX_LINE_WIDTH_AA:
	case PIPE_CAPF_MAX_POINT_WIDTH:
	case PIPE_CAPF_MAX_POINT_WIDTH_AA:
		return 16384.0f;
	case PIPE_CAPF_MAX_TEXTURE_ANISOTROPY:
		return 16.0f;
	case PIPE_CAPF_MAX_TEXTURE_LOD_BIAS:
		return 16.0f;
	case PIPE_CAPF_GUARD_BAND_LEFT:
	case PIPE_CAPF_GUARD_BAND_TOP:
	case PIPE_CAPF_GUARD_BAND_RIGHT:
	case PIPE_CAPF_GUARD_BAND_BOTTOM:
		return 0.0f;
	}
	return 0.0f;
}

static int si_get_shader_param(struct pipe_screen* pscreen, unsigned shader, enum pipe_shader_cap param)
{
	switch(shader)
	{
	case PIPE_SHADER_FRAGMENT:
	case PIPE_SHADER_VERTEX:
		break;
	case PIPE_SHADER_GEOMETRY:
		/* TODO: support and enable geometry programs */
		return 0;
	case PIPE_SHADER_COMPUTE:
		switch (param) {
		case PIPE_SHADER_CAP_PREFERRED_IR:
			return PIPE_SHADER_IR_LLVM;
		default:
			return 0;
		}
	default:
		/* TODO: support tessellation */
		return 0;
	}

	switch (param) {
	case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
	case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
	case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
	case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
		return 16384;
	case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
		return 32;
	case PIPE_SHADER_CAP_MAX_INPUTS:
		return 32;
	case PIPE_SHADER_CAP_MAX_TEMPS:
		return 256; /* Max native temporaries. */
	case PIPE_SHADER_CAP_MAX_ADDRS:
		/* FIXME Isn't this equal to TEMPS? */
		return 1; /* Max native address registers */
	case PIPE_SHADER_CAP_MAX_CONSTS:
		return 4096; /* actually only memory limits this */
	case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
		return NUM_PIPE_CONST_BUFFERS;
	case PIPE_SHADER_CAP_MAX_PREDS:
		return 0; /* FIXME */
	case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
		return 1;
	case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
		return 0;
	case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
	case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
	case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
	case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
		return 1;
	case PIPE_SHADER_CAP_INTEGERS:
		return 1;
	case PIPE_SHADER_CAP_SUBROUTINES:
		return 0;
	case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
	case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
		return 16;
	case PIPE_SHADER_CAP_PREFERRED_IR:
		return PIPE_SHADER_IR_TGSI;
	}
	return 0;
}

static int si_get_video_param(struct pipe_screen *screen,
			      enum pipe_video_profile profile,
			      enum pipe_video_entrypoint entrypoint,
			      enum pipe_video_cap param)
{
	switch (param) {
	case PIPE_VIDEO_CAP_SUPPORTED:
		return vl_profile_supported(screen, profile, entrypoint);
	case PIPE_VIDEO_CAP_NPOT_TEXTURES:
		return 1;
	case PIPE_VIDEO_CAP_MAX_WIDTH:
	case PIPE_VIDEO_CAP_MAX_HEIGHT:
		return vl_video_buffer_max_size(screen);
	case PIPE_VIDEO_CAP_PREFERED_FORMAT:
		return PIPE_FORMAT_NV12;
	case PIPE_VIDEO_CAP_MAX_LEVEL:
		return vl_level_supported(screen, profile);
	default:
		return 0;
	}
}

static int si_get_compute_param(struct pipe_screen *screen,
        enum pipe_compute_cap param,
        void *ret)
{
	struct si_screen *sscreen = (struct si_screen *)screen;
	//TODO: select these params by asic
	switch (param) {
	case PIPE_COMPUTE_CAP_IR_TARGET: {
		const char *gpu = si_get_llvm_processor_name(sscreen->b.family);
	        if (ret) {
			sprintf(ret, "%s-r600--", gpu);
		}
		return (8 + strlen(gpu)) * sizeof(char);
	}
	case PIPE_COMPUTE_CAP_GRID_DIMENSION:
		if (ret) {
			uint64_t * grid_dimension = ret;
			grid_dimension[0] = 3;
		}
		return 1 * sizeof(uint64_t);
	case PIPE_COMPUTE_CAP_MAX_GRID_SIZE:
		if (ret) {
			uint64_t * grid_size = ret;
			grid_size[0] = 65535;
			grid_size[1] = 65535;
			grid_size[2] = 1;
		}
		return 3 * sizeof(uint64_t) ;

	case PIPE_COMPUTE_CAP_MAX_BLOCK_SIZE:
		if (ret) {
			uint64_t * block_size = ret;
			block_size[0] = 256;
			block_size[1] = 256;
			block_size[2] = 256;
		}
		return 3 * sizeof(uint64_t);
	case PIPE_COMPUTE_CAP_MAX_THREADS_PER_BLOCK:
		if (ret) {
			uint64_t * max_threads_per_block = ret;
			*max_threads_per_block = 256;
		}
		return sizeof(uint64_t);

	case PIPE_COMPUTE_CAP_MAX_GLOBAL_SIZE:
		if (ret) {
			uint64_t *max_global_size = ret;
			/* XXX: Not sure what to put here. */
			*max_global_size = 2000000000;
		}
		return sizeof(uint64_t);
	case PIPE_COMPUTE_CAP_MAX_LOCAL_SIZE:
		if (ret) {
			uint64_t *max_local_size = ret;
			/* Value reported by the closed source driver. */
			*max_local_size = 32768;
		}
		return sizeof(uint64_t);
	case PIPE_COMPUTE_CAP_MAX_INPUT_SIZE:
		if (ret) {
			uint64_t *max_input_size = ret;
			/* Value reported by the closed source driver. */
			*max_input_size = 1024;
		}
		return sizeof(uint64_t);
	case PIPE_COMPUTE_CAP_MAX_MEM_ALLOC_SIZE:
		if (ret) {
			uint64_t max_global_size;
			uint64_t *max_mem_alloc_size = ret;
			si_get_compute_param(screen, PIPE_COMPUTE_CAP_MAX_GLOBAL_SIZE, &max_global_size);
			*max_mem_alloc_size = max_global_size / 4;
		}
		return sizeof(uint64_t);
	default:
		fprintf(stderr, "unknown PIPE_COMPUTE_CAP %d\n", param);
		return 0;
	}
}

static void si_destroy_screen(struct pipe_screen* pscreen)
{
	struct si_screen *sscreen = (struct si_screen *)pscreen;

	if (sscreen == NULL)
		return;

	if (!radeon_winsys_unref(sscreen->b.ws))
		return;

	r600_common_screen_cleanup(&sscreen->b);

#if SI_TRACE_CS
	if (sscreen->trace_bo) {
		sscreen->ws->buffer_unmap(sscreen->trace_bo->cs_buf);
		pipe_resource_reference((struct pipe_resource**)&sscreen->trace_bo, NULL);
	}
#endif

	sscreen->b.ws->destroy(sscreen->b.ws);
	FREE(sscreen);
}

static uint64_t si_get_timestamp(struct pipe_screen *screen)
{
	struct si_screen *sscreen = (struct si_screen*)screen;

	return 1000000 * sscreen->b.ws->query_value(sscreen->b.ws, RADEON_TIMESTAMP) /
		sscreen->b.info.r600_clock_crystal_freq;
}

struct pipe_screen *radeonsi_screen_create(struct radeon_winsys *ws)
{
	struct si_screen *sscreen = CALLOC_STRUCT(si_screen);
	if (sscreen == NULL) {
		return NULL;
	}

	ws->query_info(ws, &sscreen->b.info);

	/* Set functions first. */
	sscreen->b.b.context_create = si_create_context;
	sscreen->b.b.destroy = si_destroy_screen;
	sscreen->b.b.get_name = si_get_name;
	sscreen->b.b.get_vendor = si_get_vendor;
	sscreen->b.b.get_param = si_get_param;
	sscreen->b.b.get_shader_param = si_get_shader_param;
	sscreen->b.b.get_paramf = si_get_paramf;
	sscreen->b.b.get_compute_param = si_get_compute_param;
	sscreen->b.b.get_timestamp = si_get_timestamp;
	sscreen->b.b.is_format_supported = si_is_format_supported;
	if (sscreen->b.info.has_uvd) {
		sscreen->b.b.get_video_param = ruvd_get_video_param;
		sscreen->b.b.is_video_format_supported = ruvd_is_format_supported;
	} else {
		sscreen->b.b.get_video_param = si_get_video_param;
		sscreen->b.b.is_video_format_supported = vl_video_buffer_is_format_supported;
	}
	si_init_screen_resource_functions(&sscreen->b.b);

	if (!r600_common_screen_init(&sscreen->b, ws)) {
		FREE(sscreen);
		return NULL;
	}

	sscreen->b.has_cp_dma = true;
	sscreen->b.has_streamout = HAVE_LLVM >= 0x0304;

	if (debug_get_bool_option("RADEON_DUMP_SHADERS", FALSE))
		sscreen->b.debug_flags |= DBG_FS | DBG_VS | DBG_GS | DBG_PS | DBG_CS;

#if SI_TRACE_CS
	sscreen->cs_count = 0;
	if (sscreen->info.drm_minor >= 28) {
		sscreen->trace_bo = (struct r600_resource*)pipe_buffer_create(&sscreen->screen,
										PIPE_BIND_CUSTOM,
										PIPE_USAGE_STAGING,
										4096);
		if (sscreen->trace_bo) {
			sscreen->trace_ptr = sscreen->ws->buffer_map(sscreen->trace_bo->cs_buf, NULL,
									PIPE_TRANSFER_UNSYNCHRONIZED);
		}
	}
#endif

	/* Create the auxiliary context. This must be done last. */
	sscreen->b.aux_context = sscreen->b.b.context_create(&sscreen->b.b, NULL);

	return &sscreen->b.b;
}

/*
 * Copyright © 2015 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "anv_private.h"

struct anv_image_view_info {
   uint8_t surface_type; /**< RENDER_SURFACE_STATE.SurfaceType */
   bool is_array:1; /**< RENDER_SURFACE_STATE.SurfaceArray */
   bool is_cube:1; /**< RENDER_SURFACE_STATE.CubeFaceEnable* */
};

static const uint8_t anv_halign[] = {
    [4] = HALIGN4,
    [8] = HALIGN8,
    [16] = HALIGN16,
};

static const uint8_t anv_valign[] = {
    [4] = VALIGN4,
    [8] = VALIGN8,
    [16] = VALIGN16,
};

static const uint8_t anv_surf_type_from_image_type[] = {
   [VK_IMAGE_TYPE_1D] = SURFTYPE_1D,
   [VK_IMAGE_TYPE_2D] = SURFTYPE_2D,
   [VK_IMAGE_TYPE_3D] = SURFTYPE_3D,

};

static const struct anv_image_view_info
anv_image_view_info_table[] = {
   #define INFO(s, ...) { .surface_type = s, __VA_ARGS__ }
   [VK_IMAGE_VIEW_TYPE_1D]          = INFO(SURFTYPE_1D),
   [VK_IMAGE_VIEW_TYPE_2D]          = INFO(SURFTYPE_2D),
   [VK_IMAGE_VIEW_TYPE_3D]          = INFO(SURFTYPE_3D),
   [VK_IMAGE_VIEW_TYPE_CUBE]        = INFO(SURFTYPE_CUBE,                  .is_cube = 1),
   [VK_IMAGE_VIEW_TYPE_1D_ARRAY]    = INFO(SURFTYPE_1D,     .is_array = 1),
   [VK_IMAGE_VIEW_TYPE_2D_ARRAY]    = INFO(SURFTYPE_2D,     .is_array = 1),
   [VK_IMAGE_VIEW_TYPE_CUBE_ARRAY]  = INFO(SURFTYPE_CUBE,   .is_array = 1, .is_cube = 1),
   #undef INFO
};

static const struct anv_surf_type_limits {
   int32_t width;
   int32_t height;
   int32_t depth;
} anv_surf_type_limits[] = {
   [SURFTYPE_1D]     = {16384,       0,   2048},
   [SURFTYPE_2D]     = {16384,   16384,   2048},
   [SURFTYPE_3D]     = {2048,     2048,   2048},
   [SURFTYPE_CUBE]   = {16384,   16384,    340},
   [SURFTYPE_BUFFER] = {128,     16384,     64},
   [SURFTYPE_STRBUF] = {128,     16384,     64},
};

static const struct anv_tile_info {
   uint32_t width;
   uint32_t height;

   /**
    * Alignment for RENDER_SURFACE_STATE.SurfaceBaseAddress.
    *
    * To simplify calculations, the alignments defined in the table are
    * sometimes larger than required.  For example, Skylake requires that X and
    * Y tiled buffers be aligned to 4K, but Broadwell permits smaller
    * alignment. We choose 4K to accomodate both chipsets.  The alignment of
    * a linear buffer depends on its element type and usage. Linear depth
    * buffers have the largest alignment, 64B, so we choose that for all linear
    * buffers.
    */
   uint32_t surface_alignment;
} anv_tile_info_table[] = {
   [LINEAR] = {   1,  1,   64 },
   [XMAJOR] = { 512,  8, 4096 },
   [YMAJOR] = { 128, 32, 4096 },
   [WMAJOR] = { 128, 32, 4096 },
};

static uint32_t
anv_image_choose_tile_mode(const struct anv_image_create_info *anv_info)
{
   if (anv_info->force_tile_mode)
      return anv_info->tile_mode;

   if (anv_info->vk_info->format == VK_FORMAT_S8_UINT)
      return WMAJOR;

   switch (anv_info->vk_info->tiling) {
   case VK_IMAGE_TILING_LINEAR:
      return LINEAR;
   case VK_IMAGE_TILING_OPTIMAL:
      return YMAJOR;
   default:
      assert(!"bad VKImageTiling");
      return LINEAR;
   }
}

static VkResult
anv_image_make_surface(const struct anv_image_create_info *create_info,
                       uint64_t *inout_image_size,
                       uint32_t *inout_image_alignment,
                       struct anv_surface *out_surface)
{
   /* See RENDER_SURFACE_STATE.SurfaceQPitch */
   static const uint16_t min_qpitch UNUSED = 0x4;
   static const uint16_t max_qpitch UNUSED = 0x1ffc;

   const VkExtent3D *restrict extent = &create_info->vk_info->extent;
   const uint32_t levels = create_info->vk_info->mipLevels;
   const uint32_t array_size = create_info->vk_info->arraySize;

   const uint8_t tile_mode = anv_image_choose_tile_mode(create_info);

   const struct anv_tile_info *tile_info =
       &anv_tile_info_table[tile_mode];

   const struct anv_format *format_info =
      anv_format_for_vk_format(create_info->vk_info->format);

   const uint32_t i = 4; /* FINISHME: Stop hardcoding subimage alignment */
   const uint32_t j = 4; /* FINISHME: Stop hardcoding subimage alignment */
   const uint32_t w0 = align_u32(extent->width, i);
   const uint32_t h0 = align_u32(extent->height, j);

   uint16_t qpitch;
   uint32_t mt_width;
   uint32_t mt_height;

   if (levels == 1 && array_size == 1) {
      qpitch = min_qpitch;
      mt_width = w0;
      mt_height = h0;
   } else {
      uint32_t w1 = align_u32(anv_minify(extent->width, 1), i);
      uint32_t h1 = align_u32(anv_minify(extent->height, 1), j);
      uint32_t w2 = align_u32(anv_minify(extent->width, 2), i);

      qpitch = h0 + h1 + 11 * j;
      mt_width = MAX(w0, w1 + w2);
      mt_height = array_size * qpitch;
   }

   assert(qpitch >= min_qpitch);
   if (qpitch > max_qpitch) {
      anv_loge("image qpitch > 0x%x\n", max_qpitch);
      return vk_error(VK_ERROR_OUT_OF_DEVICE_MEMORY);
   }

   /* From the Broadwell PRM, RENDER_SURFACE_STATE.SurfaceQpitch:
    *
    *   This field must be set an integer multiple of the Surface Vertical
    *   Alignment.
    */
   assert(anv_is_aligned(qpitch, j));

   uint32_t stride = align_u32(mt_width * format_info->cpp, tile_info->width);
   if (create_info->stride > 0)
      stride = create_info->stride;

   const uint32_t size = stride * align_u32(mt_height, tile_info->height);
   const uint32_t offset = align_u32(*inout_image_size,
                                     tile_info->surface_alignment);

   *inout_image_size = offset + size;
   *inout_image_alignment = MAX(*inout_image_alignment,
                                tile_info->surface_alignment);

   *out_surface = (struct anv_surface) {
      .offset = offset,
      .stride = stride,
      .tile_mode = tile_mode,
      .qpitch = qpitch,
      .h_align = i,
      .v_align = j,
   };

   return VK_SUCCESS;
}

VkResult
anv_image_create(VkDevice _device,
                 const struct anv_image_create_info *create_info,
                 VkImage *pImage)
{
   ANV_FROM_HANDLE(anv_device, device, _device);
   const VkImageCreateInfo *pCreateInfo = create_info->vk_info;
   const VkExtent3D *restrict extent = &pCreateInfo->extent;
   struct anv_image *image = NULL;
   VkResult r;

   assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);

   /* XXX: We don't handle any of these */
   anv_assert(pCreateInfo->imageType == VK_IMAGE_TYPE_2D);
   anv_assert(pCreateInfo->mipLevels > 0);
   anv_assert(pCreateInfo->arraySize > 0);
   anv_assert(pCreateInfo->samples == 1);
   anv_assert(pCreateInfo->extent.width > 0);
   anv_assert(pCreateInfo->extent.height > 0);
   anv_assert(pCreateInfo->extent.depth > 0);

   /* TODO(chadv): How should we validate inputs? */
   const uint8_t surf_type =
      anv_surf_type_from_image_type[pCreateInfo->imageType];

   const struct anv_surf_type_limits *limits =
      &anv_surf_type_limits[surf_type];

   if (extent->width > limits->width ||
       extent->height > limits->height ||
       extent->depth > limits->depth) {
      /* TODO(chadv): What is the correct error? */
      anv_loge("image extent is too large");
      return vk_error(VK_ERROR_INVALID_MEMORY_SIZE);
   }

   image = anv_device_alloc(device, sizeof(*image), 8,
                            VK_SYSTEM_ALLOC_TYPE_API_OBJECT);
   if (!image)
      return vk_error(VK_ERROR_OUT_OF_HOST_MEMORY);

   memset(image, 0, sizeof(*image));
   image->type = pCreateInfo->imageType;
   image->extent = pCreateInfo->extent;
   image->format = anv_format_for_vk_format(pCreateInfo->format);
   image->levels = pCreateInfo->mipLevels;
   image->array_size = pCreateInfo->arraySize;
   image->surf_type = surf_type;

   if (likely(!image->format->has_stencil || image->format->depth_format)) {
      /* The image's primary surface is a color or depth surface. */
      r = anv_image_make_surface(create_info, &image->size, &image->alignment,
                                 &image->primary_surface);
      if (r != VK_SUCCESS)
         goto fail;
   }

   if (image->format->has_stencil) {
      /* From the GPU's perspective, the depth buffer and stencil buffer are
       * separate buffers.  From Vulkan's perspective, though, depth and
       * stencil reside in the same image.  To satisfy Vulkan and the GPU, we
       * place the depth and stencil buffers in the same bo.
       */
      VkImageCreateInfo stencil_info = *pCreateInfo;
      stencil_info.format = VK_FORMAT_S8_UINT;

      r = anv_image_make_surface(
            &(struct anv_image_create_info) {
               .vk_info = &stencil_info,
            },
            &image->size, &image->alignment, &image->stencil_surface);

      if (r != VK_SUCCESS)
         goto fail;
   }

   *pImage = anv_image_to_handle(image);

   return VK_SUCCESS;

fail:
   if (image)
      anv_device_free(device, image);

   return r;
}

VkResult
anv_CreateImage(VkDevice device,
                const VkImageCreateInfo *pCreateInfo,
                VkImage *pImage)
{
   return anv_image_create(device,
      &(struct anv_image_create_info) {
         .vk_info = pCreateInfo,
      },
      pImage);
}

VkResult
anv_DestroyImage(VkDevice _device, VkImage _image)
{
   ANV_FROM_HANDLE(anv_device, device, _device);

   anv_device_free(device, anv_image_from_handle(_image));

   return VK_SUCCESS;
}

VkResult anv_GetImageSubresourceLayout(
    VkDevice                                    device,
    VkImage                                     image,
    const VkImageSubresource*                   pSubresource,
    VkSubresourceLayout*                        pLayout)
{
   stub_return(VK_UNSUPPORTED);
}

void
anv_surface_view_fini(struct anv_device *device,
                      struct anv_surface_view *view)
{
   anv_state_pool_free(&device->surface_state_pool, view->surface_state);
}

void
anv_image_view_init(struct anv_image_view *iview,
                    struct anv_device *device,
                    const VkImageViewCreateInfo* pCreateInfo,
                    struct anv_cmd_buffer *cmd_buffer)
{
   ANV_FROM_HANDLE(anv_image, image, pCreateInfo->image);

   const VkImageSubresourceRange *range = &pCreateInfo->subresourceRange;
   struct anv_surface_view *view = &iview->view;
   struct anv_surface *surface;

   const struct anv_format *format_info =
      anv_format_for_vk_format(pCreateInfo->format);

   const struct anv_image_view_info *view_type_info
      = &anv_image_view_info_table[pCreateInfo->viewType];

   if (pCreateInfo->viewType != VK_IMAGE_VIEW_TYPE_2D)
      anv_finishme("non-2D image views");

   switch (pCreateInfo->subresourceRange.aspect) {
   case VK_IMAGE_ASPECT_STENCIL:
      anv_finishme("stencil image views");
      abort();
      break;
   case VK_IMAGE_ASPECT_DEPTH:
   case VK_IMAGE_ASPECT_COLOR:
      view->offset = image->offset;
      surface = &image->primary_surface;
      break;
   default:
      unreachable("");
      break;
   }

   view->bo = image->bo;
   view->offset = image->offset + surface->offset;
   view->format = pCreateInfo->format;

   iview->extent = (VkExtent3D) {
      .width = anv_minify(image->extent.width, range->baseMipLevel),
      .height = anv_minify(image->extent.height, range->baseMipLevel),
      .depth = anv_minify(image->extent.depth, range->baseMipLevel),
   };

   uint32_t depth = 1;
   if (range->arraySize > 1) {
      depth = range->arraySize;
   } else if (image->extent.depth > 1) {
      depth = image->extent.depth;
   }

   static const uint32_t vk_to_gen_swizzle[] = {
      [VK_CHANNEL_SWIZZLE_ZERO]                 = SCS_ZERO,
      [VK_CHANNEL_SWIZZLE_ONE]                  = SCS_ONE,
      [VK_CHANNEL_SWIZZLE_R]                    = SCS_RED,
      [VK_CHANNEL_SWIZZLE_G]                    = SCS_GREEN,
      [VK_CHANNEL_SWIZZLE_B]                    = SCS_BLUE,
      [VK_CHANNEL_SWIZZLE_A]                    = SCS_ALPHA
   };

   struct GEN8_RENDER_SURFACE_STATE surface_state = {
      .SurfaceType = view_type_info->surface_type,
      .SurfaceArray = image->array_size > 1,
      .SurfaceFormat = format_info->surface_format,
      .SurfaceVerticalAlignment = anv_valign[surface->v_align],
      .SurfaceHorizontalAlignment = anv_halign[surface->h_align],
      .TileMode = surface->tile_mode,
      .VerticalLineStride = 0,
      .VerticalLineStrideOffset = 0,
      .SamplerL2BypassModeDisable = true,
      .RenderCacheReadWriteMode = WriteOnlyCache,
      .MemoryObjectControlState = GEN8_MOCS,

      /* The driver sets BaseMipLevel in SAMPLER_STATE, not here in
       * RENDER_SURFACE_STATE. The Broadwell PRM says "it is illegal to have
       * both Base Mip Level fields nonzero".
       */
      .BaseMipLevel = 0.0,

      .SurfaceQPitch = surface->qpitch >> 2,
      .Height = image->extent.height - 1,
      .Width = image->extent.width - 1,
      .Depth = depth - 1,
      .SurfacePitch = surface->stride - 1,
      .MinimumArrayElement = range->baseArraySlice,
      .NumberofMultisamples = MULTISAMPLECOUNT_1,
      .XOffset = 0,
      .YOffset = 0,

      /* For sampler surfaces, the hardware interprets field MIPCount/LOD as
       * MIPCount.  The range of levels accessible by the sampler engine is
       * [SurfaceMinLOD, SurfaceMinLOD + MIPCountLOD].
       */
      .MIPCountLOD = range->mipLevels - 1,
      .SurfaceMinLOD = range->baseMipLevel,

      .AuxiliarySurfaceMode = AUX_NONE,
      .RedClearColor = 0,
      .GreenClearColor = 0,
      .BlueClearColor = 0,
      .AlphaClearColor = 0,
      .ShaderChannelSelectRed = vk_to_gen_swizzle[pCreateInfo->channels.r],
      .ShaderChannelSelectGreen = vk_to_gen_swizzle[pCreateInfo->channels.g],
      .ShaderChannelSelectBlue = vk_to_gen_swizzle[pCreateInfo->channels.b],
      .ShaderChannelSelectAlpha = vk_to_gen_swizzle[pCreateInfo->channels.a],
      .ResourceMinLOD = 0.0,
      .SurfaceBaseAddress = { NULL, view->offset },
   };

   if (cmd_buffer) {
      view->surface_state =
         anv_state_stream_alloc(&cmd_buffer->surface_state_stream, 64, 64);
   } else {
      view->surface_state =
         anv_state_pool_alloc(&device->surface_state_pool, 64, 64);
   }

   GEN8_RENDER_SURFACE_STATE_pack(NULL, view->surface_state.map, &surface_state);
}

VkResult
anv_validate_CreateImageView(VkDevice _device,
                             const VkImageViewCreateInfo *pCreateInfo,
                             VkImageView *pView)
{
   ANV_FROM_HANDLE(anv_image, image, pCreateInfo->image);
   const VkImageSubresourceRange *subresource;
   const struct anv_image_view_info *view_info;
   const struct anv_format *view_format_info;

   /* Validate structure type before dereferencing it. */
   assert(pCreateInfo);
   assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
   subresource = &pCreateInfo->subresourceRange;

   /* Validate viewType is in range before using it. */
   assert(pCreateInfo->viewType >= VK_IMAGE_VIEW_TYPE_BEGIN_RANGE);
   assert(pCreateInfo->viewType <= VK_IMAGE_VIEW_TYPE_END_RANGE);
   view_info = &anv_image_view_info_table[pCreateInfo->viewType];

   /* Validate format is in range before using it. */
   assert(pCreateInfo->format >= VK_FORMAT_BEGIN_RANGE);
   assert(pCreateInfo->format <= VK_FORMAT_END_RANGE);
   view_format_info = anv_format_for_vk_format(pCreateInfo->format);

   /* Validate channel swizzles. */
   assert(pCreateInfo->channels.r >= VK_CHANNEL_SWIZZLE_BEGIN_RANGE);
   assert(pCreateInfo->channels.r <= VK_CHANNEL_SWIZZLE_END_RANGE);
   assert(pCreateInfo->channels.g >= VK_CHANNEL_SWIZZLE_BEGIN_RANGE);
   assert(pCreateInfo->channels.g <= VK_CHANNEL_SWIZZLE_END_RANGE);
   assert(pCreateInfo->channels.b >= VK_CHANNEL_SWIZZLE_BEGIN_RANGE);
   assert(pCreateInfo->channels.b <= VK_CHANNEL_SWIZZLE_END_RANGE);
   assert(pCreateInfo->channels.a >= VK_CHANNEL_SWIZZLE_BEGIN_RANGE);
   assert(pCreateInfo->channels.a <= VK_CHANNEL_SWIZZLE_END_RANGE);

   /* Validate subresource. */
   assert(subresource->aspect >= VK_IMAGE_ASPECT_BEGIN_RANGE);
   assert(subresource->aspect <= VK_IMAGE_ASPECT_END_RANGE);
   assert(subresource->mipLevels > 0);
   assert(subresource->arraySize > 0);
   assert(subresource->baseMipLevel < image->levels);
   assert(subresource->baseMipLevel + subresource->mipLevels <= image->levels);
   assert(subresource->baseArraySlice < image->array_size);
   assert(subresource->baseArraySlice + subresource->arraySize <= image->array_size);
   assert(pView);

   if (view_info->is_cube) {
      assert(subresource->baseArraySlice % 6 == 0);
      assert(subresource->arraySize % 6 == 0);
   }

   /* Validate format. */
   switch (subresource->aspect) {
   case VK_IMAGE_ASPECT_COLOR:
      assert(!image->format->depth_format);
      assert(!image->format->has_stencil);
      assert(!view_format_info->depth_format);
      assert(!view_format_info->has_stencil);
      assert(view_format_info->cpp == image->format->cpp);
      break;
   case VK_IMAGE_ASPECT_DEPTH:
      assert(image->format->depth_format);
      assert(view_format_info->depth_format);
      assert(view_format_info->cpp == image->format->cpp);
      break;
   case VK_IMAGE_ASPECT_STENCIL:
      /* FINISHME: Is it legal to have an R8 view of S8? */
      assert(image->format->has_stencil);
      assert(view_format_info->has_stencil);
      break;
   default:
      assert(!"bad VkImageAspect");
      break;
   }

   return anv_CreateImageView(_device, pCreateInfo, pView);
}

VkResult
anv_CreateImageView(VkDevice _device,
                    const VkImageViewCreateInfo *pCreateInfo,
                    VkImageView *pView)
{
   ANV_FROM_HANDLE(anv_device, device, _device);
   struct anv_image_view *view;

   view = anv_device_alloc(device, sizeof(*view), 8,
                           VK_SYSTEM_ALLOC_TYPE_API_OBJECT);
   if (view == NULL)
      return vk_error(VK_ERROR_OUT_OF_HOST_MEMORY);

   anv_image_view_init(view, device, pCreateInfo, NULL);

   *pView = anv_image_view_to_handle(view);

   return VK_SUCCESS;
}

VkResult
anv_DestroyImageView(VkDevice _device, VkImageView _iview)
{
   ANV_FROM_HANDLE(anv_device, device, _device);
   ANV_FROM_HANDLE(anv_image_view, iview, _iview);

   anv_surface_view_fini(device, &iview->view);
   anv_device_free(device, iview);

   return VK_SUCCESS;
}

void
anv_color_attachment_view_init(struct anv_color_attachment_view *aview,
                               struct anv_device *device,
                               const VkAttachmentViewCreateInfo* pCreateInfo,
                               struct anv_cmd_buffer *cmd_buffer)
{
   ANV_FROM_HANDLE(anv_image, image, pCreateInfo->image);
   struct anv_surface_view *view = &aview->view;
   struct anv_surface *surface = &image->primary_surface;
   const struct anv_format *format_info =
      anv_format_for_vk_format(pCreateInfo->format);

   aview->base.attachment_type = ANV_ATTACHMENT_VIEW_TYPE_COLOR;

   anv_assert(pCreateInfo->arraySize > 0);
   anv_assert(pCreateInfo->mipLevel < image->levels);
   anv_assert(pCreateInfo->baseArraySlice + pCreateInfo->arraySize <= image->array_size);

   view->bo = image->bo;
   view->offset = image->offset + surface->offset;
   view->format = pCreateInfo->format;

   aview->base.extent = (VkExtent3D) {
      .width = anv_minify(image->extent.width, pCreateInfo->mipLevel),
      .height = anv_minify(image->extent.height, pCreateInfo->mipLevel),
      .depth = anv_minify(image->extent.depth, pCreateInfo->mipLevel),
   };

   uint32_t depth = 1;
   if (pCreateInfo->arraySize > 1) {
      depth = pCreateInfo->arraySize;
   } else if (image->extent.depth > 1) {
      depth = image->extent.depth;
   }

   if (cmd_buffer) {
      view->surface_state =
         anv_state_stream_alloc(&cmd_buffer->surface_state_stream, 64, 64);
   } else {
      view->surface_state =
         anv_state_pool_alloc(&device->surface_state_pool, 64, 64);
   }

   struct GEN8_RENDER_SURFACE_STATE surface_state = {
      .SurfaceType = SURFTYPE_2D,
      .SurfaceArray = image->array_size > 1,
      .SurfaceFormat = format_info->surface_format,
      .SurfaceVerticalAlignment = anv_valign[surface->v_align],
      .SurfaceHorizontalAlignment = anv_halign[surface->h_align],
      .TileMode = surface->tile_mode,
      .VerticalLineStride = 0,
      .VerticalLineStrideOffset = 0,
      .SamplerL2BypassModeDisable = true,
      .RenderCacheReadWriteMode = WriteOnlyCache,
      .MemoryObjectControlState = GEN8_MOCS,

      /* The driver sets BaseMipLevel in SAMPLER_STATE, not here in
       * RENDER_SURFACE_STATE. The Broadwell PRM says "it is illegal to have
       * both Base Mip Level fields nonzero".
       */
      .BaseMipLevel = 0.0,

      .SurfaceQPitch = surface->qpitch >> 2,
      .Height = image->extent.height - 1,
      .Width = image->extent.width - 1,
      .Depth = depth - 1,
      .SurfacePitch = surface->stride - 1,
      .MinimumArrayElement = pCreateInfo->baseArraySlice,
      .NumberofMultisamples = MULTISAMPLECOUNT_1,
      .XOffset = 0,
      .YOffset = 0,

      /* For render target surfaces, the hardware interprets field MIPCount/LOD as
       * LOD. The Broadwell PRM says:
       *
       *    MIPCountLOD defines the LOD that will be rendered into.
       *    SurfaceMinLOD is ignored.
       */
      .SurfaceMinLOD = 0,
      .MIPCountLOD = pCreateInfo->mipLevel,

      .AuxiliarySurfaceMode = AUX_NONE,
      .RedClearColor = 0,
      .GreenClearColor = 0,
      .BlueClearColor = 0,
      .AlphaClearColor = 0,
      .ShaderChannelSelectRed = SCS_RED,
      .ShaderChannelSelectGreen = SCS_GREEN,
      .ShaderChannelSelectBlue = SCS_BLUE,
      .ShaderChannelSelectAlpha = SCS_ALPHA,
      .ResourceMinLOD = 0.0,
      .SurfaceBaseAddress = { NULL, view->offset },
   };

   GEN8_RENDER_SURFACE_STATE_pack(NULL, view->surface_state.map, &surface_state);
}

static void
anv_depth_stencil_view_init(struct anv_depth_stencil_view *view,
                            const VkAttachmentViewCreateInfo *pCreateInfo)
{
   ANV_FROM_HANDLE(anv_image, image, pCreateInfo->image);
   struct anv_surface *depth_surface = &image->primary_surface;
   struct anv_surface *stencil_surface = &image->stencil_surface;

   view->base.attachment_type = ANV_ATTACHMENT_VIEW_TYPE_DEPTH_STENCIL;

   /* XXX: We don't handle any of these */
   anv_assert(pCreateInfo->mipLevel == 0);
   anv_assert(pCreateInfo->baseArraySlice == 0);
   anv_assert(pCreateInfo->arraySize == 1);

   view->bo = image->bo;

   view->depth_stride = depth_surface->stride;
   view->depth_offset = image->offset + depth_surface->offset;
   view->depth_format = image->format->depth_format;
   view->depth_qpitch = 0; /* FINISHME: QPitch */

   view->stencil_stride = stencil_surface->stride;
   view->stencil_offset = image->offset + stencil_surface->offset;
   view->stencil_qpitch = 0; /* FINISHME: QPitch */
}

VkResult
anv_CreateAttachmentView(VkDevice _device,
                         const VkAttachmentViewCreateInfo *pCreateInfo,
                         VkAttachmentView *pView)
{
   ANV_FROM_HANDLE(anv_device, device, _device);

   assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_ATTACHMENT_VIEW_CREATE_INFO);

   if (anv_is_vk_format_depth_or_stencil(pCreateInfo->format)) {
      struct anv_depth_stencil_view *view =
         anv_device_alloc(device, sizeof(*view), 8,
                          VK_SYSTEM_ALLOC_TYPE_API_OBJECT);
      if (view == NULL)
         return vk_error(VK_ERROR_OUT_OF_HOST_MEMORY);

      anv_depth_stencil_view_init(view, pCreateInfo);

      *pView = anv_attachment_view_to_handle(&view->base);
   } else {
      struct anv_color_attachment_view *view =
         anv_device_alloc(device, sizeof(*view), 8,
                          VK_SYSTEM_ALLOC_TYPE_API_OBJECT);
      if (view == NULL)
         return vk_error(VK_ERROR_OUT_OF_HOST_MEMORY);

      anv_color_attachment_view_init(view, device, pCreateInfo, NULL);

      *pView = anv_attachment_view_to_handle(&view->base);
   }

   return VK_SUCCESS;
}

VkResult
anv_DestroyAttachmentView(VkDevice _device, VkAttachmentView _view)
{
   ANV_FROM_HANDLE(anv_device, device, _device);
   ANV_FROM_HANDLE(anv_attachment_view, view, _view);

   if (view->attachment_type == ANV_ATTACHMENT_VIEW_TYPE_COLOR) {
      struct anv_color_attachment_view *aview =
         (struct anv_color_attachment_view *)view;

      anv_surface_view_fini(device, &aview->view);
   }

   anv_device_free(device, view);

   return VK_SUCCESS;
}

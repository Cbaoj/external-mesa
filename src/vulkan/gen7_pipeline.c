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

#include "gen7_pack.h"
#include "gen75_pack.h"

#include "genX_pipeline_util.h"

static void
gen7_emit_rs_state(struct anv_pipeline *pipeline,
                   const VkPipelineRasterizationStateCreateInfo *info,
                   const struct anv_graphics_pipeline_create_info *extra)
{
   struct GEN7_3DSTATE_SF sf = {
      GEN7_3DSTATE_SF_header,

      /* FIXME: Get this from pass info */
      .DepthBufferSurfaceFormat                 = D24_UNORM_X8_UINT,

      /* LegacyGlobalDepthBiasEnable */

      .StatisticsEnable                         = true,
      .FrontFaceFillMode                        = vk_to_gen_fillmode[info->polygonMode],
      .BackFaceFillMode                         = vk_to_gen_fillmode[info->polygonMode],
      .ViewTransformEnable                      = !(extra && extra->disable_viewport),
      .FrontWinding                             = vk_to_gen_front_face[info->frontFace],
      /* bool                                         AntiAliasingEnable; */

      .CullMode                                 = vk_to_gen_cullmode[info->cullMode],

      /* uint32_t                                     LineEndCapAntialiasingRegionWidth; */
      .ScissorRectangleEnable                   =  !(extra && extra->disable_scissor),

      /* uint32_t                                     MultisampleRasterizationMode; */
      /* bool                                         LastPixelEnable; */

      .TriangleStripListProvokingVertexSelect   = 0,
      .LineStripListProvokingVertexSelect       = 0,
      .TriangleFanProvokingVertexSelect         = 0,

      /* uint32_t                                     AALineDistanceMode; */
      /* uint32_t                                     VertexSubPixelPrecisionSelect; */
      .UsePointWidthState                       = !pipeline->writes_point_size,
      .PointWidth                               = 1.0,
   };

   GEN7_3DSTATE_SF_pack(NULL, &pipeline->gen7.sf, &sf);
}

static void
gen7_emit_ds_state(struct anv_pipeline *pipeline,
                   const VkPipelineDepthStencilStateCreateInfo *info)
{
   if (info == NULL) {
      /* We're going to OR this together with the dynamic state.  We need
       * to make sure it's initialized to something useful.
       */
      memset(pipeline->gen7.depth_stencil_state, 0,
             sizeof(pipeline->gen7.depth_stencil_state));
      return;
   }

   struct GEN7_DEPTH_STENCIL_STATE state = {
      .DepthTestEnable = info->depthTestEnable,
      .DepthBufferWriteEnable = info->depthWriteEnable,
      .DepthTestFunction = vk_to_gen_compare_op[info->depthCompareOp],
      .DoubleSidedStencilEnable = true,

      .StencilTestEnable = info->stencilTestEnable,
      .StencilFailOp = vk_to_gen_stencil_op[info->front.failOp],
      .StencilPassDepthPassOp = vk_to_gen_stencil_op[info->front.passOp],
      .StencilPassDepthFailOp = vk_to_gen_stencil_op[info->front.depthFailOp],
      .StencilTestFunction = vk_to_gen_compare_op[info->front.compareOp],

      .BackfaceStencilFailOp = vk_to_gen_stencil_op[info->back.failOp],
      .BackfaceStencilPassDepthPassOp = vk_to_gen_stencil_op[info->back.passOp],
      .BackfaceStencilPassDepthFailOp = vk_to_gen_stencil_op[info->back.depthFailOp],
      .BackFaceStencilTestFunction = vk_to_gen_compare_op[info->back.compareOp],
   };

   GEN7_DEPTH_STENCIL_STATE_pack(NULL, &pipeline->gen7.depth_stencil_state, &state);
}

static void
gen7_emit_cb_state(struct anv_pipeline *pipeline,
                   const VkPipelineColorBlendStateCreateInfo *info,
                   const VkPipelineMultisampleStateCreateInfo *ms_info)
{
   struct anv_device *device = pipeline->device;

   if (info->pAttachments == NULL) {
      pipeline->blend_state =
         anv_state_pool_emit(&device->dynamic_state_pool,
            GEN7_BLEND_STATE, 64,
            .ColorBufferBlendEnable = false,
            .WriteDisableAlpha = false,
            .WriteDisableRed = false,
            .WriteDisableGreen = false,
            .WriteDisableBlue = false);
   } else {
      /* FIXME-GEN7: All render targets share blend state settings on gen7, we
       * can't implement this.
       */
      const VkPipelineColorBlendAttachmentState *a = &info->pAttachments[0];
      pipeline->blend_state =
         anv_state_pool_emit(&device->dynamic_state_pool,
            GEN7_BLEND_STATE, 64,

            .ColorBufferBlendEnable = a->blendEnable,
            .IndependentAlphaBlendEnable = true, /* FIXME: yes? */
            .AlphaBlendFunction = vk_to_gen_blend_op[a->alphaBlendOp],

            .SourceAlphaBlendFactor = vk_to_gen_blend[a->srcAlphaBlendFactor],
            .DestinationAlphaBlendFactor = vk_to_gen_blend[a->dstAlphaBlendFactor],

            .ColorBlendFunction = vk_to_gen_blend_op[a->colorBlendOp],
            .SourceBlendFactor = vk_to_gen_blend[a->srcColorBlendFactor],
            .DestinationBlendFactor = vk_to_gen_blend[a->dstColorBlendFactor],
            .AlphaToCoverageEnable = ms_info && ms_info->alphaToCoverageEnable,

#     if 0
            bool                                AlphaToOneEnable;
            bool                                AlphaToCoverageDitherEnable;
#     endif

            .WriteDisableAlpha = !(a->colorWriteMask & VK_COLOR_COMPONENT_A_BIT),
            .WriteDisableRed = !(a->colorWriteMask & VK_COLOR_COMPONENT_R_BIT),
            .WriteDisableGreen = !(a->colorWriteMask & VK_COLOR_COMPONENT_G_BIT),
            .WriteDisableBlue = !(a->colorWriteMask & VK_COLOR_COMPONENT_B_BIT),

            .LogicOpEnable = info->logicOpEnable,
            .LogicOpFunction = vk_to_gen_logic_op[info->logicOp],

#     if 0
            bool                                AlphaTestEnable;
            uint32_t                            AlphaTestFunction;
            bool                                ColorDitherEnable;
            uint32_t                            XDitherOffset;
            uint32_t                            YDitherOffset;
            uint32_t                            ColorClampRange;
            bool                                PreBlendColorClampEnable;
            bool                                PostBlendColorClampEnable;
#     endif
            );
    }

   anv_batch_emit(&pipeline->batch, GEN7_3DSTATE_BLEND_STATE_POINTERS,
                  .BlendStatePointer = pipeline->blend_state.offset);
}

static inline uint32_t
scratch_space(const struct brw_stage_prog_data *prog_data)
{
   return ffs(prog_data->total_scratch / 1024);
}

GENX_FUNC(GEN7, GEN75) VkResult
genX(graphics_pipeline_create)(
    VkDevice                                    _device,
    struct anv_pipeline_cache *                 cache,
    const VkGraphicsPipelineCreateInfo*         pCreateInfo,
    const struct anv_graphics_pipeline_create_info *extra,
    const VkAllocationCallbacks*                pAllocator,
    VkPipeline*                                 pPipeline)
{
   ANV_FROM_HANDLE(anv_device, device, _device);
   struct anv_pipeline *pipeline;
   VkResult result;

   assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);
   
   pipeline = anv_alloc2(&device->alloc, pAllocator, sizeof(*pipeline), 8,
                         VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (pipeline == NULL)
      return vk_error(VK_ERROR_OUT_OF_HOST_MEMORY);

   result = anv_pipeline_init(pipeline, device, cache,
                              pCreateInfo, extra, pAllocator);
   if (result != VK_SUCCESS) {
      anv_free2(&device->alloc, pAllocator, pipeline);
      return result;
   }

   assert(pCreateInfo->pVertexInputState);
   emit_vertex_input(pipeline, pCreateInfo->pVertexInputState, extra);

   assert(pCreateInfo->pRasterizationState);
   gen7_emit_rs_state(pipeline, pCreateInfo->pRasterizationState, extra);

   gen7_emit_ds_state(pipeline, pCreateInfo->pDepthStencilState);

   gen7_emit_cb_state(pipeline, pCreateInfo->pColorBlendState,
                                pCreateInfo->pMultisampleState);

   anv_batch_emit(&pipeline->batch, GEN7_3DSTATE_VF_STATISTICS,
                   .StatisticsEnable = true);
   anv_batch_emit(&pipeline->batch, GEN7_3DSTATE_HS, .Enable = false);
   anv_batch_emit(&pipeline->batch, GEN7_3DSTATE_TE, .TEEnable = false);
   anv_batch_emit(&pipeline->batch, GEN7_3DSTATE_DS, .DSFunctionEnable = false);
   anv_batch_emit(&pipeline->batch, GEN7_3DSTATE_STREAMOUT, .SOFunctionEnable = false);

   /* From the IVB PRM Vol. 2, Part 1, Section 3.2.1:
    *
    *    "A PIPE_CONTROL with Post-Sync Operation set to 1h and a depth stall
    *    needs to be sent just prior to any 3DSTATE_VS, 3DSTATE_URB_VS,
    *    3DSTATE_CONSTANT_VS, 3DSTATE_BINDING_TABLE_POINTER_VS,
    *    3DSTATE_SAMPLER_STATE_POINTER_VS command.  Only one PIPE_CONTROL
    *    needs to be sent before any combination of VS associated 3DSTATE."
    */
   anv_batch_emit(&pipeline->batch, GEN7_PIPE_CONTROL,
                  .DepthStallEnable = true,
                  .PostSyncOperation = WriteImmediateData,
                  .Address = { &device->workaround_bo, 0 });

   anv_batch_emit(&pipeline->batch, GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_VS,
                  .ConstantBufferOffset = 0,
                  .ConstantBufferSize = 4);
   anv_batch_emit(&pipeline->batch, GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_GS,
                  .ConstantBufferOffset = 4,
                  .ConstantBufferSize = 4);
   anv_batch_emit(&pipeline->batch, GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_PS,
                  .ConstantBufferOffset = 8,
                  .ConstantBufferSize = 4);

   anv_batch_emit(&pipeline->batch, GEN7_3DSTATE_AA_LINE_PARAMETERS);

   const VkPipelineRasterizationStateCreateInfo *rs_info =
      pCreateInfo->pRasterizationState;

   anv_batch_emit(&pipeline->batch, GEN7_3DSTATE_CLIP,
      .FrontWinding                             = vk_to_gen_front_face[rs_info->frontFace],
      .CullMode                                 = vk_to_gen_cullmode[rs_info->cullMode],
      .ClipEnable                               = true,
      .APIMode                                  = APIMODE_OGL,
      .ViewportXYClipTestEnable                 = !(extra && extra->disable_viewport),
      .ClipMode                                 = CLIPMODE_NORMAL,
      .TriangleStripListProvokingVertexSelect   = 0,
      .LineStripListProvokingVertexSelect       = 0,
      .TriangleFanProvokingVertexSelect         = 0,
      .MinimumPointWidth                        = 0.125,
      .MaximumPointWidth                        = 255.875,
      .MaximumVPIndex = pCreateInfo->pViewportState->viewportCount - 1);

   if (pCreateInfo->pMultisampleState &&
       pCreateInfo->pMultisampleState->rasterizationSamples > 1)
      anv_finishme("VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO");

   uint32_t samples = 1;
   uint32_t log2_samples = __builtin_ffs(samples) - 1;

   anv_batch_emit(&pipeline->batch, GEN7_3DSTATE_MULTISAMPLE,
      .PixelLocation                            = PIXLOC_CENTER,
      .NumberofMultisamples                     = log2_samples);

   anv_batch_emit(&pipeline->batch, GEN7_3DSTATE_SAMPLE_MASK,
      .SampleMask                               = 0xff);

   anv_batch_emit(&pipeline->batch, GEN7_3DSTATE_URB_VS,
      .VSURBStartingAddress                     = pipeline->urb.vs_start,
      .VSURBEntryAllocationSize                 = pipeline->urb.vs_size - 1,
      .VSNumberofURBEntries                     = pipeline->urb.nr_vs_entries);

   anv_batch_emit(&pipeline->batch, GEN7_3DSTATE_URB_GS,
      .GSURBStartingAddress                     = pipeline->urb.gs_start,
      .GSURBEntryAllocationSize                 = pipeline->urb.gs_size - 1,
      .GSNumberofURBEntries                     = pipeline->urb.nr_gs_entries);

   anv_batch_emit(&pipeline->batch, GEN7_3DSTATE_URB_HS,
      .HSURBStartingAddress                     = pipeline->urb.vs_start,
      .HSURBEntryAllocationSize                 = 0,
      .HSNumberofURBEntries                     = 0);

   anv_batch_emit(&pipeline->batch, GEN7_3DSTATE_URB_DS,
      .DSURBStartingAddress                     = pipeline->urb.vs_start,
      .DSURBEntryAllocationSize                 = 0,
      .DSNumberofURBEntries                     = 0);

   const struct brw_vue_prog_data *vue_prog_data = &pipeline->vs_prog_data.base;
   /* The last geometry producing stage will set urb_offset and urb_length,
    * which we use in 3DSTATE_SBE. Skip the VUE header and position slots. */
   uint32_t urb_offset = 1;
   uint32_t urb_length = (vue_prog_data->vue_map.num_slots + 1) / 2 - urb_offset;

#if 0 
   /* From gen7_vs_state.c */

   /**
    * From Graphics BSpec: 3D-Media-GPGPU Engine > 3D Pipeline Stages >
    * Geometry > Geometry Shader > State:
    *
    *     "Note: Because of corruption in IVB:GT2, software needs to flush the
    *     whole fixed function pipeline when the GS enable changes value in
    *     the 3DSTATE_GS."
    *
    * The hardware architects have clarified that in this context "flush the
    * whole fixed function pipeline" means to emit a PIPE_CONTROL with the "CS
    * Stall" bit set.
    */
   if (!brw->is_haswell && !brw->is_baytrail)
      gen7_emit_vs_workaround_flush(brw);
#endif

   if (pipeline->vs_vec4 == NO_KERNEL || (extra && extra->disable_vs))
      anv_batch_emit(&pipeline->batch, GENX(3DSTATE_VS), .VSFunctionEnable = false);
   else
      anv_batch_emit(&pipeline->batch, GENX(3DSTATE_VS),
         .KernelStartPointer                    = pipeline->vs_vec4,
         .ScratchSpaceBaseOffset                = pipeline->scratch_start[MESA_SHADER_VERTEX],
         .PerThreadScratchSpace                 = scratch_space(&vue_prog_data->base),

         .DispatchGRFStartRegisterforURBData    =
            vue_prog_data->base.dispatch_grf_start_reg,
         .VertexURBEntryReadLength              = vue_prog_data->urb_read_length,
         .VertexURBEntryReadOffset              = 0,

         .MaximumNumberofThreads                = device->info.max_vs_threads - 1,
         .StatisticsEnable                      = true,
         .VSFunctionEnable                      = true);

   const struct brw_gs_prog_data *gs_prog_data = &pipeline->gs_prog_data;

   if (pipeline->gs_kernel == NO_KERNEL || (extra && extra->disable_vs)) {
      anv_batch_emit(&pipeline->batch, GENX(3DSTATE_GS), .GSEnable = false);
   } else {
      urb_offset = 1;
      urb_length = (gs_prog_data->base.vue_map.num_slots + 1) / 2 - urb_offset;

      anv_batch_emit(&pipeline->batch, GENX(3DSTATE_GS),
         .KernelStartPointer                    = pipeline->gs_kernel,
         .ScratchSpaceBasePointer               = pipeline->scratch_start[MESA_SHADER_GEOMETRY],
         .PerThreadScratchSpace                 = scratch_space(&gs_prog_data->base.base),

         .OutputVertexSize                      = gs_prog_data->output_vertex_size_hwords * 2 - 1,
         .OutputTopology                        = gs_prog_data->output_topology,
         .VertexURBEntryReadLength              = gs_prog_data->base.urb_read_length,
         .IncludeVertexHandles                  = gs_prog_data->base.include_vue_handles,
         .DispatchGRFStartRegisterforURBData    =
            gs_prog_data->base.base.dispatch_grf_start_reg,

         .MaximumNumberofThreads                = device->info.max_gs_threads - 1,
         /* This in the next dword on HSW. */
         .ControlDataFormat                     = gs_prog_data->control_data_format,
         .ControlDataHeaderSize                 = gs_prog_data->control_data_header_size_hwords,
         .InstanceControl                       = MAX2(gs_prog_data->invocations, 1) - 1,
         .DispatchMode                          = gs_prog_data->base.dispatch_mode,
         .GSStatisticsEnable                    = true,
         .IncludePrimitiveID                    = gs_prog_data->include_primitive_id,
#     if (ANV_IS_HASWELL)
         .ReorderMode                           = REORDER_TRAILING,
#     else
         .ReorderEnable                         = true,
#     endif
         .GSEnable                              = true);
   }

   const struct brw_wm_prog_data *wm_prog_data = &pipeline->wm_prog_data;
   if (wm_prog_data->urb_setup[VARYING_SLOT_BFC0] != -1 ||
       wm_prog_data->urb_setup[VARYING_SLOT_BFC1] != -1)
      anv_finishme("two-sided color needs sbe swizzling setup");
   if (wm_prog_data->urb_setup[VARYING_SLOT_PRIMITIVE_ID] != -1)
      anv_finishme("primitive_id needs sbe swizzling setup");

   /* FIXME: generated header doesn't emit attr swizzle fields */
   anv_batch_emit(&pipeline->batch, GEN7_3DSTATE_SBE,
      .NumberofSFOutputAttributes               = pipeline->wm_prog_data.num_varying_inputs,
      .VertexURBEntryReadLength                 = urb_length,
      .VertexURBEntryReadOffset                 = urb_offset,
      .PointSpriteTextureCoordinateOrigin       = UPPERLEFT);

   anv_batch_emit(&pipeline->batch, GENX(3DSTATE_PS),
      .KernelStartPointer0                      = pipeline->ps_ksp0,
      .ScratchSpaceBasePointer                  = pipeline->scratch_start[MESA_SHADER_FRAGMENT],
      .PerThreadScratchSpace                    = scratch_space(&wm_prog_data->base),
                  
      .MaximumNumberofThreads                   = device->info.max_wm_threads - 1,
      .PushConstantEnable                       = wm_prog_data->base.nr_params > 0,
      .AttributeEnable                          = wm_prog_data->num_varying_inputs > 0,
      .oMaskPresenttoRenderTarget               = wm_prog_data->uses_omask,

      .RenderTargetFastClearEnable              = false,
      .DualSourceBlendEnable                    = false,
      .RenderTargetResolveEnable                = false,

      .PositionXYOffsetSelect                   = wm_prog_data->uses_pos_offset ?
         POSOFFSET_SAMPLE : POSOFFSET_NONE,

      ._32PixelDispatchEnable                   = false,
      ._16PixelDispatchEnable                   = pipeline->ps_simd16 != NO_KERNEL,
      ._8PixelDispatchEnable                    = pipeline->ps_simd8 != NO_KERNEL,

      .DispatchGRFStartRegisterforConstantSetupData0 = pipeline->ps_grf_start0,
      .DispatchGRFStartRegisterforConstantSetupData1 = 0,
      .DispatchGRFStartRegisterforConstantSetupData2 = pipeline->ps_grf_start2,

#if 0
   /* Haswell requires the sample mask to be set in this packet as well as
    * in 3DSTATE_SAMPLE_MASK; the values should match. */
   /* _NEW_BUFFERS, _NEW_MULTISAMPLE */
#endif

      .KernelStartPointer1                      = 0,
      .KernelStartPointer2                      = pipeline->ps_ksp2);

   /* FIXME-GEN7: This needs a lot more work, cf gen7 upload_wm_state(). */
   anv_batch_emit(&pipeline->batch, GEN7_3DSTATE_WM,
      .StatisticsEnable                         = true,
      .ThreadDispatchEnable                     = true,
      .LineEndCapAntialiasingRegionWidth        = 0, /* 0.5 pixels */
      .LineAntialiasingRegionWidth              = 1, /* 1.0 pixels */
      .EarlyDepthStencilControl                 = EDSC_NORMAL,
      .PointRasterizationRule                   = RASTRULE_UPPER_RIGHT,
      .PixelShaderComputedDepthMode             = wm_prog_data->computed_depth_mode,
      .BarycentricInterpolationMode             = wm_prog_data->barycentric_interp_modes);

   *pPipeline = anv_pipeline_to_handle(pipeline);

   return VK_SUCCESS;
}

GENX_FUNC(GEN7, GEN75) VkResult
genX(compute_pipeline_create)(
    VkDevice                                    _device,
    struct anv_pipeline_cache *                 cache,
    const VkComputePipelineCreateInfo*          pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkPipeline*                                 pPipeline)
{
   anv_finishme("primitive_id needs sbe swizzling setup");
   abort();
}

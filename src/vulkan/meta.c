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

#include "private.h"
#include "glsl_helpers.h"

static void
anv_device_init_meta_clear_state(struct anv_device *device)
{
   VkPipelineIaStateCreateInfo ia_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
      .disableVertexReuse = false,
      .primitiveRestartEnable = false,
      .primitiveRestartIndex = 0
   };

   /* We don't use a vertex shader for clearing, but instead build and pass
    * the VUEs directly to the rasterization backend.
    */
   VkShader fs = GLSL_VK_SHADER(device, FRAGMENT,
      out vec4 f_color;
      flat in vec4 v_color;
      void main()
      {
         f_color = v_color;
      }
   );

   VkPipelineShaderStageCreateInfo fs_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = &ia_create_info,
      .shader = {
         .stage = VK_SHADER_STAGE_FRAGMENT,
         .shader = fs,
         .linkConstBufferCount = 0,
         .pLinkConstBufferInfo = NULL,
         .pSpecializationInfo = NULL
      }
   };

   /* We use instanced rendering to clear multiple render targets. We have two
    * vertex buffers: the first vertex buffer holds per-vertex data and
    * provides the vertices for the clear rectangle. The second one holds
    * per-instance data, which consists of the VUE header (which selects the
    * layer) and the color (Vulkan supports per-RT clear colors).
    */
   VkPipelineVertexInputCreateInfo vi_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO,
      .pNext = &fs_create_info,
      .bindingCount = 2,
      .pVertexBindingDescriptions = (VkVertexInputBindingDescription[]) {
         {
            .binding = 0,
            .strideInBytes = 8,
            .stepRate = VK_VERTEX_INPUT_STEP_RATE_VERTEX
         },
         {
            .binding = 1,
            .strideInBytes = 32,
            .stepRate = VK_VERTEX_INPUT_STEP_RATE_INSTANCE
         },
      },
      .attributeCount = 3,
      .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[]) {
         {
            /* VUE Header */
            .location = 0,
            .binding = 1,
            .format = VK_FORMAT_R32G32B32A32_UINT,
            .offsetInBytes = 0
         },
         {
            /* Position */
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offsetInBytes = 0
         },
         {
            /* Color */
            .location = 2,
            .binding = 1,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offsetInBytes = 16
         }
      }
   };

   VkPipelineRsStateCreateInfo rs_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO,
      .pNext = &vi_create_info,
      .depthClipEnable = true,
      .rasterizerDiscardEnable = false,
      .fillMode = VK_FILL_MODE_SOLID,
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_CCW
   };

   anv_pipeline_create((VkDevice) device,
      &(VkGraphicsPipelineCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
         .pNext = &rs_create_info,
         .flags = 0,
         .layout = 0
      },
      &(struct anv_pipeline_create_info) {
         .use_repclear = true,
         .disable_viewport = true,
         .use_rectlist = true
      },
      &device->clear_state.pipeline);

   anv_DestroyObject((VkDevice) device, VK_OBJECT_TYPE_SHADER, fs);

   anv_CreateDynamicRasterState((VkDevice) device,
      &(VkDynamicRsStateCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO,
      },
      &device->clear_state.rs_state);
}

#define NUM_VB_USED 2
struct anv_saved_state {
   struct anv_bindings bindings;
   struct anv_bindings *old_bindings;
   struct anv_pipeline *old_pipeline;
};

static void
anv_cmd_buffer_save(struct anv_cmd_buffer *cmd_buffer,
                    struct anv_saved_state *state)
{
   state->old_bindings = cmd_buffer->bindings;
   cmd_buffer->bindings = &state->bindings;
   state->old_pipeline = cmd_buffer->pipeline;
}

static void
anv_cmd_buffer_restore(struct anv_cmd_buffer *cmd_buffer,
                       const struct anv_saved_state *state)
{
   cmd_buffer->bindings = state->old_bindings;
   cmd_buffer->pipeline = state->old_pipeline;

   cmd_buffer->vb_dirty |= (1 << NUM_VB_USED) - 1;
   cmd_buffer->dirty |= ANV_CMD_BUFFER_PIPELINE_DIRTY |
                        ANV_CMD_BUFFER_DESCRIPTOR_SET_DIRTY;
}

struct vue_header {
   uint32_t Reserved;
   uint32_t RTAIndex;
   uint32_t ViewportIndex;
   float PointWidth;
};

void
anv_cmd_buffer_clear(struct anv_cmd_buffer *cmd_buffer,
                     struct anv_render_pass *pass)
{
   struct anv_device *device = cmd_buffer->device;
   struct anv_framebuffer *fb = cmd_buffer->framebuffer;
   struct anv_saved_state saved_state;
   struct anv_state state;
   uint32_t size;

   struct instance_data {
      struct vue_header vue_header;
      float color[4];
   } *instance_data;

   if (pass->num_clear_layers == 0)
      return;

   const float vertex_data[] = {
      /* Rect-list coordinates */
            0.0,        0.0,
      fb->width,        0.0,
      fb->width, fb->height,

      /* Align to 16 bytes */
            0.0,        0.0,
   };

   size = sizeof(vertex_data) + pass->num_clear_layers * sizeof(instance_data[0]);
   state = anv_state_stream_alloc(&cmd_buffer->surface_state_stream, size, 16);

   memcpy(state.map, vertex_data, sizeof(vertex_data));
   instance_data = state.map + sizeof(vertex_data);

   for (uint32_t i = 0; i < pass->num_layers; i++) {
      if (pass->layers[i].color_load_op == VK_ATTACHMENT_LOAD_OP_CLEAR) {
         *instance_data++ = (struct instance_data) {
            .vue_header = {
               .RTAIndex = i,
               .ViewportIndex = 0,
               .PointWidth = 0.0
            },
            .color = {
               pass->layers[i].clear_color.color.floatColor[0],
               pass->layers[i].clear_color.color.floatColor[1],
               pass->layers[i].clear_color.color.floatColor[2],
               pass->layers[i].clear_color.color.floatColor[3],
            }
         };
      }
   }

   struct anv_buffer vertex_buffer = {
      .device = cmd_buffer->device,
      .size = size,
      .bo = &device->surface_state_block_pool.bo,
      .offset = state.offset
   };

   anv_cmd_buffer_save(cmd_buffer, &saved_state);

   /* Initialize render targets for the meta bindings. */
   anv_cmd_buffer_fill_render_targets(cmd_buffer);

   anv_CmdBindVertexBuffers((VkCmdBuffer) cmd_buffer, 0, 2,
      (VkBuffer[]) {
         (VkBuffer) &vertex_buffer,
         (VkBuffer) &vertex_buffer
      },
      (VkDeviceSize[]) {
         0,
         sizeof(vertex_data)
      });

   if ((VkPipeline) cmd_buffer->pipeline != device->clear_state.pipeline)
      anv_CmdBindPipeline((VkCmdBuffer) cmd_buffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, device->clear_state.pipeline);

   /* We don't need anything here, only set if not already set. */
   if (cmd_buffer->rs_state == NULL)
      anv_CmdBindDynamicStateObject((VkCmdBuffer) cmd_buffer,
                                    VK_STATE_BIND_POINT_RASTER,
                                    device->clear_state.rs_state);

   if (cmd_buffer->vp_state == NULL)
      anv_CmdBindDynamicStateObject((VkCmdBuffer) cmd_buffer,
                                    VK_STATE_BIND_POINT_VIEWPORT,
                                    cmd_buffer->framebuffer->vp_state);

   anv_CmdDraw((VkCmdBuffer) cmd_buffer, 0, 3, 0, pass->num_clear_layers);

   /* Restore API state */
   anv_cmd_buffer_restore(cmd_buffer, &saved_state);

}

static void
anv_device_init_meta_blit_state(struct anv_device *device)
{
   VkPipelineIaStateCreateInfo ia_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
      .disableVertexReuse = false,
      .primitiveRestartEnable = false,
      .primitiveRestartIndex = 0
   };

   /* We don't use a vertex shader for clearing, but instead build and pass
    * the VUEs directly to the rasterization backend.  However, we do need
    * to provide GLSL source for the vertex shader so that the compiler
    * does not dead-code our inputs.
    */
   VkShader vs = GLSL_VK_SHADER(device, VERTEX,
      in vec2 a_pos;
      in vec2 a_tex_coord;
      out vec4 v_tex_coord;
      void main()
      {
         v_tex_coord = vec4(a_tex_coord, 0, 1);
         gl_Position = vec4(a_pos, 0, 1);
      }
   );

   VkShader fs = GLSL_VK_SHADER(device, FRAGMENT,
      out vec4 f_color;
      in vec4 v_tex_coord;
      layout(set = 0, binding = 0) uniform sampler2D u_tex;
      void main()
      {
         f_color = texture(u_tex, v_tex_coord.xy);
      }
   );

   VkPipelineShaderStageCreateInfo vs_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = &ia_create_info,
      .shader = {
         .stage = VK_SHADER_STAGE_VERTEX,
         .shader = vs,
         .linkConstBufferCount = 0,
         .pLinkConstBufferInfo = NULL,
         .pSpecializationInfo = NULL
      }
   };

   VkPipelineShaderStageCreateInfo fs_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = &vs_create_info,
      .shader = {
         .stage = VK_SHADER_STAGE_FRAGMENT,
         .shader = fs,
         .linkConstBufferCount = 0,
         .pLinkConstBufferInfo = NULL,
         .pSpecializationInfo = NULL
      }
   };

   VkPipelineVertexInputCreateInfo vi_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO,
      .pNext = &fs_create_info,
      .bindingCount = 2,
      .pVertexBindingDescriptions = (VkVertexInputBindingDescription[]) {
         {
            .binding = 0,
            .strideInBytes = 0,
            .stepRate = VK_VERTEX_INPUT_STEP_RATE_VERTEX
         },
         {
            .binding = 1,
            .strideInBytes = 16,
            .stepRate = VK_VERTEX_INPUT_STEP_RATE_VERTEX
         },
      },
      .attributeCount = 3,
      .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[]) {
         {
            /* VUE Header */
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_UINT,
            .offsetInBytes = 0
         },
         {
            /* Position */
            .location = 1,
            .binding = 1,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offsetInBytes = 0
         },
         {
            /* Texture Coordinate */
            .location = 2,
            .binding = 1,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offsetInBytes = 8
         }
      }
   };

   VkDescriptorSetLayoutCreateInfo ds_layout_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .count = 1,
      .pBinding = (VkDescriptorSetLayoutBinding[]) {
         {
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .count = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = NULL
         },
      }
   };
   anv_CreateDescriptorSetLayout((VkDevice) device, &ds_layout_info,
                                 &device->blit_state.ds_layout);

   VkPipelineLayoutCreateInfo pipeline_layout_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .descriptorSetCount = 1,
      .pSetLayouts = &device->blit_state.ds_layout,
   };

   VkPipelineLayout pipeline_layout;
   anv_CreatePipelineLayout((VkDevice) device, &pipeline_layout_info,
                            &pipeline_layout);

   VkPipelineRsStateCreateInfo rs_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO,
      .pNext = &vi_create_info,
      .depthClipEnable = true,
      .rasterizerDiscardEnable = false,
      .fillMode = VK_FILL_MODE_SOLID,
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_CCW
   };

   VkGraphicsPipelineCreateInfo pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &rs_create_info,
      .flags = 0,
      .layout = pipeline_layout,
   };

   anv_pipeline_create((VkDevice) device, &pipeline_info,
                       &(struct anv_pipeline_create_info) {
                          .use_repclear = false,
                          .disable_viewport = true,
                          .disable_scissor = true,
                          .disable_vs = true,
                          .use_rectlist = true
                       },
                       &device->blit_state.pipeline);

   anv_DestroyObject((VkDevice) device, VK_OBJECT_TYPE_SHADER, vs);
   anv_DestroyObject((VkDevice) device, VK_OBJECT_TYPE_SHADER, fs);

   anv_CreateDynamicRasterState((VkDevice) device,
      &(VkDynamicRsStateCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO,
       },
      &device->blit_state.rs_state);
}

static void
meta_prepare_blit(struct anv_cmd_buffer *cmd_buffer,
                  struct anv_saved_state *saved_state)
{
   struct anv_device *device = cmd_buffer->device;

   anv_cmd_buffer_save(cmd_buffer, saved_state);

   if ((VkPipeline) cmd_buffer->pipeline != device->blit_state.pipeline)
      anv_CmdBindPipeline((VkCmdBuffer) cmd_buffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          device->blit_state.pipeline);

   /* We don't need anything here, only set if not already set. */
   if (cmd_buffer->rs_state == NULL)
      anv_CmdBindDynamicStateObject((VkCmdBuffer) cmd_buffer,
                                    VK_STATE_BIND_POINT_RASTER,
                                    device->blit_state.rs_state);
}

struct blit_region {
   VkOffset3D src_offset;
   VkExtent3D src_extent;
   VkOffset3D dest_offset;
   VkExtent3D dest_extent;
};

static void
meta_emit_blit(struct anv_cmd_buffer *cmd_buffer,
               struct anv_surface_view *src,
               VkOffset3D src_offset,
               VkExtent3D src_extent,
               struct anv_surface_view *dest,
               VkOffset3D dest_offset,
               VkExtent3D dest_extent)
{
   struct anv_device *device = cmd_buffer->device;

   struct blit_vb_data {
      float pos[2];
      float tex_coord[2];
   } *vb_data;

   unsigned vb_size = sizeof(struct vue_header) + 3 * sizeof(*vb_data);

   struct anv_state vb_state =
      anv_state_stream_alloc(&cmd_buffer->surface_state_stream, vb_size, 16);
   memset(vb_state.map, 0, sizeof(struct vue_header));
   vb_data = vb_state.map + sizeof(struct vue_header);

   vb_data[0] = (struct blit_vb_data) {
      .pos = {
         dest_offset.x + dest_extent.width,
         dest_offset.y + dest_extent.height,
      },
      .tex_coord = {
         (float)(src_offset.x + src_extent.width) / (float)src->extent.width,
         (float)(src_offset.y + src_extent.height) / (float)src->extent.height,
      },
   };

   vb_data[1] = (struct blit_vb_data) {
      .pos = {
         dest_offset.x,
         dest_offset.y + dest_extent.height,
      },
      .tex_coord = {
         (float)src_offset.x / (float)src->extent.width,
         (float)(src_offset.y + src_extent.height) / (float)src->extent.height,
      },
   };

   vb_data[2] = (struct blit_vb_data) {
      .pos = {
         dest_offset.x,
         dest_offset.y,
      },
      .tex_coord = {
         (float)src_offset.x / (float)src->extent.width,
         (float)src_offset.y / (float)src->extent.height,
      },
   };

   struct anv_buffer vertex_buffer = {
      .device = device,
      .size = vb_size,
      .bo = &device->surface_state_block_pool.bo,
      .offset = vb_state.offset,
   };

   anv_CmdBindVertexBuffers((VkCmdBuffer) cmd_buffer, 0, 2,
      (VkBuffer[]) {
         (VkBuffer) &vertex_buffer,
         (VkBuffer) &vertex_buffer
      },
      (VkDeviceSize[]) {
         0,
         sizeof(struct vue_header),
      });

   uint32_t count;
   VkDescriptorSet set;
   anv_AllocDescriptorSets((VkDevice) device, 0 /* pool */,
                           VK_DESCRIPTOR_SET_USAGE_ONE_SHOT,
                           1, &device->blit_state.ds_layout, &set, &count);
   anv_UpdateDescriptors((VkDevice) device, set, 1,
      (const void * []) {
         &(VkUpdateImages) {
            .sType = VK_STRUCTURE_TYPE_UPDATE_IMAGES,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .binding = 0,
            .count = 1,
            .pImageViews = (VkImageViewAttachInfo[]) {
               {
                  .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO,
                  .view = (VkImageView) src,
                  .layout = VK_IMAGE_LAYOUT_GENERAL,
               }
            }
         }
      });

   struct anv_framebuffer *fb;
   anv_CreateFramebuffer((VkDevice) device,
      &(VkFramebufferCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
         .colorAttachmentCount = 1,
         .pColorAttachments = (VkColorAttachmentBindInfo[]) {
            {
               .view = (VkColorAttachmentView) dest,
               .layout = VK_IMAGE_LAYOUT_GENERAL
            }
         },
         .pDepthStencilAttachment = NULL,
         .sampleCount = 1,
         .width = dest->extent.width,
         .height = dest->extent.height,
         .layers = 1
      }, (VkFramebuffer *)&fb);


   VkRenderPass pass;
   anv_CreateRenderPass((VkDevice )device,
      &(VkRenderPassCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
         .renderArea = { { 0, 0 }, { dest->extent.width, dest->extent.height } },
         .colorAttachmentCount = 1,
         .extent = { 0, },
         .sampleCount = 1,
         .layers = 1,
         .pColorFormats = (VkFormat[]) { dest->format },
         .pColorLayouts = (VkImageLayout[]) { VK_IMAGE_LAYOUT_GENERAL },
         .pColorLoadOps = (VkAttachmentLoadOp[]) { VK_ATTACHMENT_LOAD_OP_LOAD },
         .pColorStoreOps = (VkAttachmentStoreOp[]) { VK_ATTACHMENT_STORE_OP_STORE },
         .pColorLoadClearValues = (VkClearColor[]) {
            { .color = { .floatColor = { 1.0, 0.0, 0.0, 1.0 } }, .useRawValue = false }
         },
         .depthStencilFormat = VK_FORMAT_UNDEFINED,
      }, &pass);

   anv_CmdBeginRenderPass((VkCmdBuffer) cmd_buffer,
      &(VkRenderPassBegin) {
         .renderPass = pass,
         .framebuffer = (VkFramebuffer) fb,
      });

   anv_CmdBindDynamicStateObject((VkCmdBuffer) cmd_buffer,
                                 VK_STATE_BIND_POINT_VIEWPORT, fb->vp_state);

   anv_CmdBindDescriptorSets((VkCmdBuffer) cmd_buffer,
                             VK_PIPELINE_BIND_POINT_GRAPHICS, 0, 1,
                             &set, 0, NULL);

   anv_CmdDraw((VkCmdBuffer) cmd_buffer, 0, 3, 0, 1);

   anv_CmdEndRenderPass((VkCmdBuffer) cmd_buffer, pass);
}

static void
meta_finish_blit(struct anv_cmd_buffer *cmd_buffer,
                 const struct anv_saved_state *saved_state)
{
   anv_cmd_buffer_restore(cmd_buffer, saved_state);
}

static VkFormat
vk_format_for_cpp(int cpp)
{
   switch (cpp) {
   case 1: return VK_FORMAT_R8_UINT;
   case 2: return VK_FORMAT_R8G8_UINT;
   case 3: return VK_FORMAT_R8G8B8_UINT;
   case 4: return VK_FORMAT_R8G8B8A8_UINT;
   case 6: return VK_FORMAT_R16G16B16_UINT;
   case 8: return VK_FORMAT_R16G16B16A16_UINT;
   case 12: return VK_FORMAT_R32G32B32_UINT;
   case 16: return VK_FORMAT_R32G32B32A32_UINT;
   default:
      unreachable("Invalid format cpp");
   }
}

void anv_CmdCopyBuffer(
    VkCmdBuffer                                 cmdBuffer,
    VkBuffer                                    srcBuffer,
    VkBuffer                                    destBuffer,
    uint32_t                                    regionCount,
    const VkBufferCopy*                         pRegions)
{
   struct anv_cmd_buffer *cmd_buffer = (struct anv_cmd_buffer *)cmdBuffer;
   VkDevice vk_device = (VkDevice) cmd_buffer->device;
   struct anv_buffer *src_buffer = (struct anv_buffer *)srcBuffer;
   struct anv_buffer *dest_buffer = (struct anv_buffer *)destBuffer;
   struct anv_saved_state saved_state;

   meta_prepare_blit(cmd_buffer, &saved_state);

   for (unsigned r = 0; r < regionCount; r++) {
      size_t src_offset = src_buffer->offset + pRegions[r].srcOffset;
      size_t dest_offset = dest_buffer->offset + pRegions[r].destOffset;

      /* First, we compute the biggest format that can be used with the
       * given offsets and size.
       */
      int cpp = 16;

      int fs = ffs(src_offset) - 1;
      if (fs != -1)
         cpp = MIN2(cpp, 1 << fs);
      assert(src_offset % cpp == 0);

      fs = ffs(dest_offset) - 1;
      if (fs != -1)
         cpp = MIN2(cpp, 1 << fs);
      assert(dest_offset % cpp == 0);

      fs = ffs(pRegions[r].copySize) - 1;
      if (fs != -1)
         cpp = MIN2(cpp, 1 << fs);
      assert(pRegions[r].copySize % cpp == 0);

      VkFormat copy_format = vk_format_for_cpp(cpp);

      VkImageCreateInfo image_info = {
         .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
         .imageType = VK_IMAGE_TYPE_2D,
         .format = copy_format,
         .extent = {
            .width = pRegions[r].copySize / cpp,
            .height = 1,
            .depth = 1,
         },
         .mipLevels = 1,
         .arraySize = 1,
         .samples = 1,
         .tiling = VK_IMAGE_TILING_LINEAR,
         .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
         .flags = 0,
      };

      struct anv_image *src_image, *dest_image;
      vkCreateImage(vk_device, &image_info, (VkImage *)&src_image);
      vkCreateImage(vk_device, &image_info, (VkImage *)&dest_image);

      /* We could use a vk call to bind memory, but that would require
       * creating a dummy memory object etc. so there's really no point.
       */
      src_image->bo = src_buffer->bo;
      src_image->offset = src_offset;
      dest_image->bo = dest_buffer->bo;
      dest_image->offset = dest_offset;

      struct anv_surface_view src_view;
      anv_image_view_init(&src_view, cmd_buffer->device,
         &(VkImageViewCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = (VkImage)src_image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = copy_format,
            .channels = {
               VK_CHANNEL_SWIZZLE_R,
               VK_CHANNEL_SWIZZLE_G,
               VK_CHANNEL_SWIZZLE_B,
               VK_CHANNEL_SWIZZLE_A
            },
            .subresourceRange = {
               .aspect = VK_IMAGE_ASPECT_COLOR,
               .baseMipLevel = 0,
               .mipLevels = 1,
               .baseArraySlice = 0,
               .arraySize = 1
            },
            .minLod = 0
         },
         cmd_buffer);

      struct anv_surface_view dest_view;
      anv_color_attachment_view_init(&dest_view, cmd_buffer->device,
         &(VkColorAttachmentViewCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO,
            .image = (VkImage)dest_image,
            .format = copy_format,
            .mipLevel = 0,
            .baseArraySlice = 0,
            .arraySize = 1,
         },
         cmd_buffer);

      meta_emit_blit(cmd_buffer,
                     &src_view,
                     (VkOffset3D) { 0, 0, 0 },
                     (VkExtent3D) { pRegions[r].copySize / cpp, 1, 1 },
                     &dest_view,
                     (VkOffset3D) { 0, 0, 0 },
                     (VkExtent3D) { pRegions[r].copySize / cpp, 1, 1 });
   }

   meta_finish_blit(cmd_buffer, &saved_state);
}

void anv_CmdCopyImage(
    VkCmdBuffer                                 cmdBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     destImage,
    VkImageLayout                               destImageLayout,
    uint32_t                                    regionCount,
    const VkImageCopy*                          pRegions)
{
   struct anv_cmd_buffer *cmd_buffer = (struct anv_cmd_buffer *)cmdBuffer;
   struct anv_image *src_image = (struct anv_image *)srcImage;
   struct anv_saved_state saved_state;

   meta_prepare_blit(cmd_buffer, &saved_state);

   for (unsigned r = 0; r < regionCount; r++) {
      struct anv_surface_view src_view;
      anv_image_view_init(&src_view, cmd_buffer->device,
         &(VkImageViewCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = srcImage,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = src_image->format,
            .channels = {
               VK_CHANNEL_SWIZZLE_R,
               VK_CHANNEL_SWIZZLE_G,
               VK_CHANNEL_SWIZZLE_B,
               VK_CHANNEL_SWIZZLE_A
            },
            .subresourceRange = {
               .aspect = pRegions[r].srcSubresource.aspect,
               .baseMipLevel = pRegions[r].srcSubresource.mipLevel,
               .mipLevels = 1,
               .baseArraySlice = pRegions[r].srcSubresource.arraySlice,
               .arraySize = 1
            },
            .minLod = 0
         },
         cmd_buffer);

      struct anv_surface_view dest_view;
      anv_color_attachment_view_init(&dest_view, cmd_buffer->device,
         &(VkColorAttachmentViewCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO,
            .image = destImage,
            .format = src_image->format,
            .mipLevel = pRegions[r].destSubresource.mipLevel,
            .baseArraySlice = pRegions[r].destSubresource.arraySlice,
            .arraySize = 1,
         },
         cmd_buffer);

      meta_emit_blit(cmd_buffer,
                     &src_view,
                     pRegions[r].srcOffset,
                     pRegions[r].extent,
                     &dest_view,
                     pRegions[r].destOffset,
                     pRegions[r].extent);
   }

   meta_finish_blit(cmd_buffer, &saved_state);
}

void anv_CmdBlitImage(
    VkCmdBuffer                                 cmdBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     destImage,
    VkImageLayout                               destImageLayout,
    uint32_t                                    regionCount,
    const VkImageBlit*                          pRegions)
{
   struct anv_cmd_buffer *cmd_buffer = (struct anv_cmd_buffer *)cmdBuffer;
   struct anv_image *src_image = (struct anv_image *)srcImage;
   struct anv_image *dest_image = (struct anv_image *)destImage;
   struct anv_saved_state saved_state;

   meta_prepare_blit(cmd_buffer, &saved_state);

   for (unsigned r = 0; r < regionCount; r++) {
      struct anv_surface_view src_view;
      anv_image_view_init(&src_view, cmd_buffer->device,
         &(VkImageViewCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = srcImage,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = src_image->format,
            .channels = {
               VK_CHANNEL_SWIZZLE_R,
               VK_CHANNEL_SWIZZLE_G,
               VK_CHANNEL_SWIZZLE_B,
               VK_CHANNEL_SWIZZLE_A
            },
            .subresourceRange = {
               .aspect = pRegions[r].srcSubresource.aspect,
               .baseMipLevel = pRegions[r].srcSubresource.mipLevel,
               .mipLevels = 1,
               .baseArraySlice = pRegions[r].srcSubresource.arraySlice,
               .arraySize = 1
            },
            .minLod = 0
         },
         cmd_buffer);

      struct anv_surface_view dest_view;
      anv_color_attachment_view_init(&dest_view, cmd_buffer->device,
         &(VkColorAttachmentViewCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO,
            .image = destImage,
            .format = dest_image->format,
            .mipLevel = pRegions[r].destSubresource.mipLevel,
            .baseArraySlice = pRegions[r].destSubresource.arraySlice,
            .arraySize = 1,
         },
         cmd_buffer);

      meta_emit_blit(cmd_buffer,
                     &src_view,
                     pRegions[r].srcOffset,
                     pRegions[r].srcExtent,
                     &dest_view,
                     pRegions[r].destOffset,
                     pRegions[r].destExtent);
   }

   meta_finish_blit(cmd_buffer, &saved_state);
}

void anv_CmdCopyBufferToImage(
    VkCmdBuffer                                 cmdBuffer,
    VkBuffer                                    srcBuffer,
    VkImage                                     destImage,
    VkImageLayout                               destImageLayout,
    uint32_t                                    regionCount,
    const VkBufferImageCopy*                    pRegions)
{
   struct anv_cmd_buffer *cmd_buffer = (struct anv_cmd_buffer *)cmdBuffer;
   VkDevice vk_device = (VkDevice) cmd_buffer->device;
   struct anv_buffer *src_buffer = (struct anv_buffer *)srcBuffer;
   struct anv_image *dest_image = (struct anv_image *)destImage;
   struct anv_saved_state saved_state;

   meta_prepare_blit(cmd_buffer, &saved_state);

   for (unsigned r = 0; r < regionCount; r++) {
      struct anv_image *src_image;
      anv_CreateImage(vk_device,
         &(VkImageCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = dest_image->format,
            .extent = {
               .width = pRegions[r].imageExtent.width,
               .height = pRegions[r].imageExtent.height,
               .depth = 1,
            },
            .mipLevels = 1,
            .arraySize = 1,
            .samples = 1,
            .tiling = VK_IMAGE_TILING_LINEAR,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
            .flags = 0,
         }, (VkImage *)&src_image);

      /* We could use a vk call to bind memory, but that would require
       * creating a dummy memory object etc. so there's really no point.
       */
      src_image->bo = src_buffer->bo;
      src_image->offset = src_buffer->offset + pRegions[r].bufferOffset;

      struct anv_surface_view src_view;
      anv_image_view_init(&src_view, cmd_buffer->device,
         &(VkImageViewCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = (VkImage)src_image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = dest_image->format,
            .channels = {
               VK_CHANNEL_SWIZZLE_R,
               VK_CHANNEL_SWIZZLE_G,
               VK_CHANNEL_SWIZZLE_B,
               VK_CHANNEL_SWIZZLE_A
            },
            .subresourceRange = {
               .aspect = pRegions[r].imageSubresource.aspect,
               .baseMipLevel = 0,
               .mipLevels = 1,
               .baseArraySlice = 0,
               .arraySize = 1
            },
            .minLod = 0
         },
         cmd_buffer);

      struct anv_surface_view dest_view;
      anv_color_attachment_view_init(&dest_view, cmd_buffer->device,
         &(VkColorAttachmentViewCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO,
            .image = (VkImage)dest_image,
            .format = dest_image->format,
            .mipLevel = pRegions[r].imageSubresource.mipLevel,
            .baseArraySlice = pRegions[r].imageSubresource.arraySlice,
            .arraySize = 1,
         },
         cmd_buffer);

      meta_emit_blit(cmd_buffer,
                     &src_view,
                     (VkOffset3D) { 0, 0, 0 },
                     pRegions[r].imageExtent,
                     &dest_view,
                     pRegions[r].imageOffset,
                     pRegions[r].imageExtent);
   }

   meta_finish_blit(cmd_buffer, &saved_state);
}

void anv_CmdCopyImageToBuffer(
    VkCmdBuffer                                 cmdBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkBuffer                                    destBuffer,
    uint32_t                                    regionCount,
    const VkBufferImageCopy*                    pRegions)
{
   struct anv_cmd_buffer *cmd_buffer = (struct anv_cmd_buffer *)cmdBuffer;
   VkDevice vk_device = (VkDevice) cmd_buffer->device;
   struct anv_image *src_image = (struct anv_image *)srcImage;
   struct anv_buffer *dest_buffer = (struct anv_buffer *)destBuffer;
   struct anv_saved_state saved_state;

   meta_prepare_blit(cmd_buffer, &saved_state);

   for (unsigned r = 0; r < regionCount; r++) {
      struct anv_surface_view src_view;
      anv_image_view_init(&src_view, cmd_buffer->device,
         &(VkImageViewCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = srcImage,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = src_image->format,
            .channels = {
               VK_CHANNEL_SWIZZLE_R,
               VK_CHANNEL_SWIZZLE_G,
               VK_CHANNEL_SWIZZLE_B,
               VK_CHANNEL_SWIZZLE_A
            },
            .subresourceRange = {
               .aspect = pRegions[r].imageSubresource.aspect,
               .baseMipLevel = pRegions[r].imageSubresource.mipLevel,
               .mipLevels = 1,
               .baseArraySlice = pRegions[r].imageSubresource.arraySlice,
               .arraySize = 1
            },
            .minLod = 0
         },
         cmd_buffer);

      struct anv_image *dest_image;
      anv_CreateImage(vk_device,
         &(VkImageCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = src_image->format,
            .extent = {
               .width = pRegions[r].imageExtent.width,
               .height = pRegions[r].imageExtent.height,
               .depth = 1,
            },
            .mipLevels = 1,
            .arraySize = 1,
            .samples = 1,
            .tiling = VK_IMAGE_TILING_LINEAR,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
            .flags = 0,
         }, (VkImage *)&dest_image);

      /* We could use a vk call to bind memory, but that would require
       * creating a dummy memory object etc. so there's really no point.
       */
      dest_image->bo = dest_buffer->bo;
      dest_image->offset = dest_buffer->offset + pRegions[r].bufferOffset;

      struct anv_surface_view dest_view;
      anv_color_attachment_view_init(&dest_view, cmd_buffer->device,
         &(VkColorAttachmentViewCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO,
            .image = (VkImage)dest_image,
            .format = src_image->format,
            .mipLevel = 0,
            .baseArraySlice = 0,
            .arraySize = 1,
         },
         cmd_buffer);

      meta_emit_blit(cmd_buffer,
                     &src_view,
                     pRegions[r].imageOffset,
                     pRegions[r].imageExtent,
                     &dest_view,
                     (VkOffset3D) { 0, 0, 0 },
                     pRegions[r].imageExtent);
   }

   meta_finish_blit(cmd_buffer, &saved_state);
}

void anv_CmdCloneImageData(
    VkCmdBuffer                                 cmdBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     destImage,
    VkImageLayout                               destImageLayout)
{
   stub();
}

void anv_CmdUpdateBuffer(
    VkCmdBuffer                                 cmdBuffer,
    VkBuffer                                    destBuffer,
    VkDeviceSize                                destOffset,
    VkDeviceSize                                dataSize,
    const uint32_t*                             pData)
{
   stub();
}

void anv_CmdFillBuffer(
    VkCmdBuffer                                 cmdBuffer,
    VkBuffer                                    destBuffer,
    VkDeviceSize                                destOffset,
    VkDeviceSize                                fillSize,
    uint32_t                                    data)
{
   stub();
}

void anv_CmdClearColorImage(
    VkCmdBuffer                                 cmdBuffer,
    VkImage                                     image,
    VkImageLayout                               imageLayout,
    const VkClearColor*                         color,
    uint32_t                                    rangeCount,
    const VkImageSubresourceRange*              pRanges)
{
   stub();
}

void anv_CmdClearDepthStencil(
    VkCmdBuffer                                 cmdBuffer,
    VkImage                                     image,
    VkImageLayout                               imageLayout,
    float                                       depth,
    uint32_t                                    stencil,
    uint32_t                                    rangeCount,
    const VkImageSubresourceRange*              pRanges)
{
   stub();
}

void anv_CmdResolveImage(
    VkCmdBuffer                                 cmdBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     destImage,
    VkImageLayout                               destImageLayout,
    uint32_t                                    regionCount,
    const VkImageResolve*                       pRegions)
{
   stub();
}

void
anv_device_init_meta(struct anv_device *device)
{
   anv_device_init_meta_clear_state(device);
   anv_device_init_meta_blit_state(device);
}

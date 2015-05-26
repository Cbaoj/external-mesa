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

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <i915_drm.h>

#include "brw_device_info.h"
#include "util/macros.h"

#define VK_PROTOTYPES
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_intel.h>
#include <vulkan/vk_wsi_lunarg.h>

#include "entrypoints.h"

#include "brw_context.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline uint32_t
ALIGN_U32(uint32_t v, uint32_t a)
{
   return (v + a - 1) & ~(a - 1);
}

static inline int32_t
ALIGN_I32(int32_t v, int32_t a)
{
   return (v + a - 1) & ~(a - 1);
}

#define for_each_bit(b, dword)                          \
   for (uint32_t __dword = (dword);                     \
        (b) = __builtin_ffs(__dword) - 1, __dword;      \
        __dword &= ~(1 << (b)))

/* Define no kernel as 1, since that's an illegal offset for a kernel */
#define NO_KERNEL 1

struct anv_common {
    VkStructureType                             sType;
    const void*                                 pNext;
};

/* Whenever we generate an error, pass it through this function. Useful for
 * debugging, where we can break on it. Only call at error site, not when
 * propagating errors. Might be useful to plug in a stack trace here.
 */

static inline VkResult
vk_error(VkResult error)
{
#ifdef DEBUG
   fprintf(stderr, "vk_error: %x\n", error);
#endif

   return error;
}

void __anv_finishme(const char *file, int line, const char *format, ...);

/**
 * Print a FINISHME message, including its source location.
 */
#define anv_finishme(format, ...) \
   __anv_finishme(__FILE__, __LINE__, format, ##__VA_ARGS__);

#define stub_return(v) \
   do { \
      anv_finishme("stub %s", __func__); \
      return (v); \
   } while (0)

#define stub(v) \
   do { \
      anv_finishme("stub %s", __func__); \
      return; \
   } while (0)

/**
 * A dynamically growable, circular buffer.  Elements are added at head and
 * removed from tail. head and tail are free-running uint32_t indices and we
 * only compute the modulo with size when accessing the array.  This way,
 * number of bytes in the queue is always head - tail, even in case of
 * wraparound.
 */

struct anv_vector {
   uint32_t head;
   uint32_t tail;
   uint32_t element_size;
   uint32_t size;
   void *data;
};

int anv_vector_init(struct anv_vector *queue, uint32_t element_size, uint32_t size);
void *anv_vector_add(struct anv_vector *queue);
void *anv_vector_remove(struct anv_vector *queue);

static inline int
anv_vector_length(struct anv_vector *queue)
{
   return (queue->head - queue->tail) / queue->element_size;
}

static inline void
anv_vector_finish(struct anv_vector *queue)
{
   free(queue->data);
}

#define anv_vector_foreach(elem, queue)                                  \
   static_assert(__builtin_types_compatible_p(__typeof__(queue), struct anv_vector *), ""); \
   for (uint32_t __anv_vector_offset = (queue)->tail;                                \
        elem = (queue)->data + (__anv_vector_offset & ((queue)->size - 1)), __anv_vector_offset < (queue)->head; \
        __anv_vector_offset += (queue)->element_size)

struct anv_bo {
   int gem_handle;
   uint32_t index;
   uint64_t offset;
   uint64_t size;

   /* This field is here for the benefit of the aub dumper.  It can (and for
    * userptr bos it must) be set to the cpu map of the buffer.  Destroying
    * the bo won't clean up the mmap, it's still the responsibility of the bo
    * user to do that. */
   void *map;
};

/* Represents a lock-free linked list of "free" things.  This is used by
 * both the block pool and the state pools.  Unfortunately, in order to
 * solve the ABA problem, we can't use a single uint32_t head.
 */
union anv_free_list {
   struct {
      uint32_t offset;

      /* A simple count that is incremented every time the head changes. */
      uint32_t count;
   };
   uint64_t u64;
};

#define ANV_FREE_LIST_EMPTY ((union anv_free_list) { { 1, 0 } })

struct anv_block_pool {
   struct anv_device *device;

   struct anv_bo bo;
   void *map;
   int fd;
   uint32_t size;

   /**
    * Array of mmaps and gem handles owned by the block pool, reclaimed when
    * the block pool is destroyed.
    */
   struct anv_vector mmap_cleanups;

   uint32_t block_size;

   uint32_t next_block;
   union anv_free_list free_list;
};

struct anv_block_state {
   union {
      struct {
         uint32_t next;
         uint32_t end;
      };
      uint64_t u64;
   };
};

struct anv_state {
   uint32_t offset;
   uint32_t alloc_size;
   void *map;
};

struct anv_fixed_size_state_pool {
   size_t state_size;
   union anv_free_list free_list;
   struct anv_block_state block;
};

#define ANV_MIN_STATE_SIZE_LOG2 6
#define ANV_MAX_STATE_SIZE_LOG2 10

#define ANV_STATE_BUCKETS (ANV_MAX_STATE_SIZE_LOG2 - ANV_MIN_STATE_SIZE_LOG2)

struct anv_state_pool {
   struct anv_block_pool *block_pool;
   struct anv_fixed_size_state_pool buckets[ANV_STATE_BUCKETS];
};

struct anv_state_stream {
   struct anv_block_pool *block_pool;
   uint32_t next;
   uint32_t current_block;
   uint32_t end;
};

void anv_block_pool_init(struct anv_block_pool *pool,
                         struct anv_device *device, uint32_t block_size);
void anv_block_pool_init_slave(struct anv_block_pool *pool,
                               struct anv_block_pool *master_pool,
                               uint32_t num_blocks);
void anv_block_pool_finish(struct anv_block_pool *pool);
uint32_t anv_block_pool_alloc(struct anv_block_pool *pool);
void anv_block_pool_free(struct anv_block_pool *pool, uint32_t offset);
void anv_state_pool_init(struct anv_state_pool *pool,
                         struct anv_block_pool *block_pool);
struct anv_state anv_state_pool_alloc(struct anv_state_pool *pool,
                                      size_t state_size, size_t alignment);
void anv_state_pool_free(struct anv_state_pool *pool, struct anv_state state);
void anv_state_stream_init(struct anv_state_stream *stream,
                           struct anv_block_pool *block_pool);
void anv_state_stream_finish(struct anv_state_stream *stream);
struct anv_state anv_state_stream_alloc(struct anv_state_stream *stream,
                                        uint32_t size, uint32_t alignment);

struct anv_object;
struct anv_device;

typedef void (*anv_object_destructor_cb)(struct anv_device *,
                                         struct anv_object *,
                                         VkObjectType);

struct anv_object {
   anv_object_destructor_cb                     destructor;
};

struct anv_physical_device {
    struct anv_instance *                       instance;
    uint32_t                                    chipset_id;
    bool                                        no_hw;
    const char *                                path;
    const char *                                name;
    const struct brw_device_info *              info;
};

struct anv_instance {
    void *                                      pAllocUserData;
    PFN_vkAllocFunction                         pfnAlloc;
    PFN_vkFreeFunction                          pfnFree;
    uint32_t                                    apiVersion;
    uint32_t                                    physicalDeviceCount;
    struct anv_physical_device                  physicalDevice;
};

struct anv_clear_state {
   VkPipeline                                   pipeline;
   VkDynamicRsState                             rs_state;
};

struct anv_blit_state {
   VkPipeline                                   pipeline;
   VkDynamicRsState                             rs_state;
   VkDescriptorSetLayout                        ds_layout;
};

struct anv_device {
    struct anv_instance *                       instance;
    uint32_t                                    chipset_id;
    struct brw_device_info                      info;
    int                                         context_id;
    int                                         fd;
    bool                                        no_hw;
    bool                                        dump_aub;

    struct anv_block_pool                       dynamic_state_block_pool;
    struct anv_state_pool                       dynamic_state_pool;

    struct anv_block_pool                       instruction_block_pool;
    struct anv_block_pool                       surface_state_block_pool;
    struct anv_block_pool                       binding_table_block_pool;
    struct anv_state_pool                       surface_state_pool;

    struct anv_clear_state                      clear_state;
    struct anv_blit_state                       blit_state;

    struct anv_compiler *                       compiler;
    struct anv_aub_writer *                     aub_writer;
    pthread_mutex_t                             mutex;
};

struct anv_queue {
    struct anv_device *                         device;

    struct anv_state_pool *                     pool;

    /**
     * Serial number of the most recently completed batch executed on the
     * engine.
     */
    struct anv_state                            completed_serial;

    /**
     * The next batch submitted to the engine will be assigned this serial
     * number.
     */
    uint32_t                                    next_serial;

    uint32_t                                    last_collected_serial;
};

void *
anv_device_alloc(struct anv_device *            device,
                 size_t                         size,
                 size_t                         alignment,
                 VkSystemAllocType              allocType);

void
anv_device_free(struct anv_device *             device,
                void *                          mem);

void* anv_gem_mmap(struct anv_device *device,
                   uint32_t gem_handle, uint64_t offset, uint64_t size);
void anv_gem_munmap(void *p, uint64_t size);
uint32_t anv_gem_create(struct anv_device *device, size_t size);
void anv_gem_close(struct anv_device *device, int gem_handle);
int anv_gem_userptr(struct anv_device *device, void *mem, size_t size);
int anv_gem_wait(struct anv_device *device, int gem_handle, int64_t *timeout_ns);
int anv_gem_execbuffer(struct anv_device *device,
                       struct drm_i915_gem_execbuffer2 *execbuf);
int anv_gem_set_tiling(struct anv_device *device, int gem_handle,
                       uint32_t stride, uint32_t tiling);
int anv_gem_create_context(struct anv_device *device);
int anv_gem_destroy_context(struct anv_device *device, int context);
int anv_gem_get_param(int fd, uint32_t param);
int anv_gem_get_aperture(struct anv_device *device, uint64_t *size);
int anv_gem_handle_to_fd(struct anv_device *device, int gem_handle);
int anv_gem_fd_to_handle(struct anv_device *device, int fd);
int anv_gem_userptr(struct anv_device *device, void *mem, size_t size);

VkResult anv_bo_init_new(struct anv_bo *bo, struct anv_device *device, uint64_t size);

/* TODO: Remove hardcoded reloc limit. */
#define ANV_BATCH_MAX_RELOCS 256

struct anv_reloc_list {
   size_t                                       num_relocs;
   struct drm_i915_gem_relocation_entry         relocs[ANV_BATCH_MAX_RELOCS];
   struct anv_bo *                              reloc_bos[ANV_BATCH_MAX_RELOCS];
};

struct anv_batch {
   struct anv_bo                                bo;
   void *                                       next;
   struct anv_reloc_list                        cmd_relocs;
};

VkResult anv_batch_init(struct anv_batch *batch, struct anv_device *device);
void anv_batch_finish(struct anv_batch *batch, struct anv_device *device);
void anv_batch_reset(struct anv_batch *batch);
void *anv_batch_emit_dwords(struct anv_batch *batch, int num_dwords);
void anv_batch_emit_batch(struct anv_batch *batch, struct anv_batch *other);
uint64_t anv_batch_emit_reloc(struct anv_batch *batch,
                              void *location, struct anv_bo *bo, uint32_t offset);

struct anv_address {
   struct anv_bo *bo;
   uint32_t offset;
};

#define __gen_address_type struct anv_address
#define __gen_user_data struct anv_batch

static inline uint64_t
__gen_combine_address(struct anv_batch *batch, void *location,
                      const struct anv_address address, uint32_t delta)
{   
   if (address.bo == NULL) {
      return delta;
   } else {
      assert(batch->bo.map <= location &&
             (char *) location < (char *) batch->bo.map + batch->bo.size);

      return anv_batch_emit_reloc(batch, location, address.bo, address.offset + delta);
   }
}
   
#include "gen7_pack.h"
#include "gen75_pack.h"
#undef GEN8_3DSTATE_MULTISAMPLE
#include "gen8_pack.h"

#define anv_batch_emit(batch, cmd, ...) do {                            \
      struct cmd __template = {                                         \
         cmd ## _header,                                                \
         __VA_ARGS__                                                    \
      };                                                                \
      void *__dst = anv_batch_emit_dwords(batch, cmd ## _length);       \
      cmd ## _pack(batch, __dst, &__template);                          \
   } while (0)

#define anv_batch_emitn(batch, n, cmd, ...) ({          \
      struct cmd __template = {                         \
         cmd ## _header,                                \
        .DwordLength = n - cmd ## _length_bias,         \
         __VA_ARGS__                                    \
      };                                                \
      void *__dst = anv_batch_emit_dwords(batch, n);    \
      cmd ## _pack(batch, __dst, &__template);          \
      __dst;                                            \
   })

#define anv_batch_emit_merge(batch, dwords0, dwords1)                   \
   do {                                                                 \
      uint32_t *dw;                                                     \
                                                                        \
      assert(ARRAY_SIZE(dwords0) == ARRAY_SIZE(dwords1));               \
      dw = anv_batch_emit_dwords((batch), ARRAY_SIZE(dwords0));         \
      for (uint32_t i = 0; i < ARRAY_SIZE(dwords0); i++)                \
         dw[i] = (dwords0)[i] | (dwords1)[i];                           \
   } while (0)

#define GEN8_MOCS {                                     \
      .MemoryTypeLLCeLLCCacheabilityControl = WB,       \
      .TargetCache = L3DefertoPATforLLCeLLCselection,   \
      .AgeforQUADLRU = 0                                \
   }

struct anv_device_memory {
   struct anv_bo                                bo;
   VkDeviceSize                                 map_size;
   void *                                       map;
};

struct anv_dynamic_vp_state {
   struct anv_object base;
   struct anv_state sf_clip_vp;
   struct anv_state cc_vp;
   struct anv_state scissor;
};

struct anv_dynamic_rs_state {
   uint32_t state_sf[GEN8_3DSTATE_SF_length];
};

struct anv_dynamic_cb_state {
   uint32_t blend_offset;
};

struct anv_query_pool_slot {
   uint64_t begin;
   uint64_t end;
   uint64_t available;
};

struct anv_query_pool {
   struct anv_object                            base;
   VkQueryType                                  type;
   uint32_t                                     slots;
   struct anv_bo                                bo;
};

struct anv_descriptor_set_layout {
   struct {
      uint32_t surface_count;
      uint32_t *surface_start;
      uint32_t sampler_count;
      uint32_t *sampler_start;
   } stage[VK_NUM_SHADER_STAGE];

   uint32_t count;
   uint32_t num_dynamic_buffers;
   uint32_t entries[0];
};

struct anv_descriptor {
   struct anv_sampler *sampler;
   struct anv_surface_view *view;
};

struct anv_descriptor_set {
   struct anv_descriptor descriptors[0];
};

#define MAX_VBS   32
#define MAX_SETS   8
#define MAX_RTS    8

struct anv_pipeline_layout {
   struct {
      struct anv_descriptor_set_layout *layout;
      uint32_t surface_start[VK_NUM_SHADER_STAGE];
      uint32_t sampler_start[VK_NUM_SHADER_STAGE];
   } set[MAX_SETS];

   uint32_t num_sets;

   struct {
      uint32_t surface_count;
      uint32_t sampler_count;
   } stage[VK_NUM_SHADER_STAGE];
};

struct anv_buffer {
   struct anv_device *                          device;
   VkDeviceSize                                 size;

   /* Set when bound */
   struct anv_bo *                              bo;
   VkDeviceSize                                 offset;   
};

#define ANV_CMD_BUFFER_PIPELINE_DIRTY           (1 << 0)
#define ANV_CMD_BUFFER_DESCRIPTOR_SET_DIRTY     (1 << 1)
#define ANV_CMD_BUFFER_RS_DIRTY                 (1 << 2)
   
struct anv_bindings {
   struct {
      struct anv_buffer *buffer;
      VkDeviceSize offset;
   }                                            vb[MAX_VBS];

   struct {
      uint32_t                                  surfaces[256];
      struct { uint32_t dwords[4]; }            samplers[16];
   }                                            descriptors[VK_NUM_SHADER_STAGE];
};

struct anv_cmd_buffer {
   struct anv_object                            base;
   struct anv_device *                          device;

   struct drm_i915_gem_execbuffer2              execbuf;
   struct drm_i915_gem_exec_object2 *           exec2_objects;
   struct anv_bo **                             exec2_bos;
   bool                                         need_reloc;
   uint32_t                                     serial;

   uint32_t                                     bo_count;
   struct anv_batch                             batch;
   struct anv_bo                                surface_bo;
   uint32_t                                     surface_next;
   struct anv_reloc_list                        surface_relocs;
   struct anv_state_stream                      binding_table_state_stream;
   struct anv_state_stream                      surface_state_stream;
   struct anv_state_stream                      dynamic_state_stream;

   /* State required while building cmd buffer */
   uint32_t                                     vb_dirty;
   uint32_t                                     dirty;
   struct anv_pipeline *                        pipeline;
   struct anv_framebuffer *                     framebuffer;
   struct anv_dynamic_rs_state *                rs_state;
   struct anv_dynamic_vp_state *                vp_state;
   struct anv_bindings *                        bindings;
   struct anv_bindings                          default_bindings;
};

void anv_cmd_buffer_dump(struct anv_cmd_buffer *cmd_buffer);
void anv_aub_writer_destroy(struct anv_aub_writer *writer);

struct anv_fence {
   struct anv_object base;
   struct anv_bo bo;
   struct drm_i915_gem_execbuffer2 execbuf;
   struct drm_i915_gem_exec_object2 exec2_objects[1];
   bool ready;
};

struct anv_shader {
   uint32_t size;
   char data[0];
};

struct anv_pipeline {
   struct anv_object                            base;
   struct anv_device *                          device;
   struct anv_batch                             batch;
   struct anv_shader *                          shaders[VK_NUM_SHADER_STAGE];
   struct anv_pipeline_layout *                 layout;
   bool                                         use_repclear;

   struct brw_vs_prog_data                      vs_prog_data;
   struct brw_wm_prog_data                      wm_prog_data;
   struct brw_gs_prog_data                      gs_prog_data;
   struct brw_stage_prog_data *                 prog_data[VK_NUM_SHADER_STAGE];
   struct {
      uint32_t                                  vs_start;
      uint32_t                                  vs_size;
      uint32_t                                  nr_vs_entries;
      uint32_t                                  gs_start;
      uint32_t                                  gs_size;
      uint32_t                                  nr_gs_entries;
   } urb;

   struct anv_bo                                vs_scratch_bo;
   struct anv_bo                                ps_scratch_bo;
   struct anv_bo                                gs_scratch_bo;

   uint32_t                                     active_stages;
   struct anv_state_stream                      program_stream;
   uint32_t                                     vs_simd8;
   uint32_t                                     ps_simd8;
   uint32_t                                     ps_simd16;
   uint32_t                                     gs_vec4;
   uint32_t                                     gs_vertex_count;

   uint32_t                                     vb_used;
   uint32_t                                     binding_stride[MAX_VBS];

   uint32_t                                     state_sf[GEN8_3DSTATE_SF_length];
   uint32_t                                     state_raster[GEN8_3DSTATE_RASTER_length];
};

struct anv_pipeline_create_info {
   bool                                         use_repclear;
   bool                                         disable_viewport;
   bool                                         disable_scissor;
   bool                                         disable_vs;
   bool                                         use_rectlist;
};

VkResult
anv_pipeline_create(VkDevice device,
                    const VkGraphicsPipelineCreateInfo *pCreateInfo,
                    const struct anv_pipeline_create_info *extra,
                    VkPipeline *pPipeline);

struct anv_compiler *anv_compiler_create(int fd);
void anv_compiler_destroy(struct anv_compiler *compiler);
int anv_compiler_run(struct anv_compiler *compiler, struct anv_pipeline *pipeline);
void anv_compiler_free(struct anv_pipeline *pipeline);

struct anv_format {
   uint16_t                                     format;
   uint8_t                                      cpp;
   uint8_t                                      channels;
   bool                                         has_stencil;
};

const struct anv_format *
anv_format_for_vk_format(VkFormat format);

struct anv_image {
   VkImageType                                  type;
   VkExtent3D                                   extent;
   VkFormat                                     format;
   uint32_t                                     tile_mode;
   VkDeviceSize                                 size;
   uint32_t                                     alignment;
   uint32_t                                     stride;

   uint32_t                                     stencil_offset;
   uint32_t                                     stencil_stride;

   /* Set when bound */
   struct anv_bo *                              bo;
   VkDeviceSize                                 offset;

   struct anv_swap_chain *                      swap_chain;
};

struct anv_surface_view {
   struct anv_state                             surface_state;
   struct anv_bo *                              bo;
   uint32_t                                     offset;
   VkExtent3D                                   extent;
   VkFormat                                     format;
};

struct anv_image_create_info {
   uint32_t                                     tile_mode;
};

VkResult anv_image_create(VkDevice _device,
                          const VkImageCreateInfo *pCreateInfo,
                          const struct anv_image_create_info *extra,
                          VkImage *pImage);

void anv_image_view_init(struct anv_surface_view *view,
                         struct anv_device *device,
                         const VkImageViewCreateInfo* pCreateInfo,
                         struct anv_cmd_buffer *cmd_buffer);

void anv_color_attachment_view_init(struct anv_surface_view *view,
                                    struct anv_device *device,
                                    const VkColorAttachmentViewCreateInfo* pCreateInfo,
                                    struct anv_cmd_buffer *cmd_buffer);

struct anv_sampler {
   uint32_t state[4];
};

struct anv_depth_stencil_view {
   struct anv_bo *                              bo;

   uint32_t                                     depth_offset;
   uint32_t                                     depth_stride;
   uint32_t                                     depth_format;

   uint32_t                                     stencil_offset;
   uint32_t                                     stencil_stride;
};

struct anv_framebuffer {
   struct anv_object                            base;
   uint32_t                                     color_attachment_count;
   const struct anv_surface_view *              color_attachments[MAX_RTS];
   const struct anv_depth_stencil_view *        depth_stencil;

   uint32_t                                     sample_count;
   uint32_t                                     width;
   uint32_t                                     height;
   uint32_t                                     layers;

   /* Viewport for clears */
   VkDynamicVpState                             vp_state;
};

struct anv_render_pass_layer {
   VkAttachmentLoadOp                           color_load_op;
   VkClearColor                                 clear_color;
};

struct anv_render_pass {
   VkRect                                       render_area;

   uint32_t                                     num_clear_layers;
   uint32_t                                     num_layers;
   struct anv_render_pass_layer                 layers[0];
};

void anv_device_init_meta(struct anv_device *device);

void
anv_cmd_buffer_clear(struct anv_cmd_buffer *cmd_buffer,
                     struct anv_render_pass *pass);

void
anv_cmd_buffer_fill_render_targets(struct anv_cmd_buffer *cmd_buffer);

void *
anv_lookup_entrypoint(const char *name);

#ifdef __cplusplus
}
#endif

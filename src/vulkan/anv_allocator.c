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

#define _DEFAULT_SOURCE

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <values.h>
#include <assert.h>
#include <linux/futex.h>
#include <linux/memfd.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/syscall.h>

#include "anv_private.h"

#ifdef HAVE_VALGRIND
#define VG_NOACCESS_READ(__ptr) ({                       \
   VALGRIND_MAKE_MEM_DEFINED((__ptr), sizeof(*(__ptr))); \
   __typeof(*(__ptr)) __val = *(__ptr);                  \
   VALGRIND_MAKE_MEM_NOACCESS((__ptr), sizeof(*(__ptr)));\
   __val;                                                \
})
#define VG_NOACCESS_WRITE(__ptr, __val) ({                  \
   VALGRIND_MAKE_MEM_UNDEFINED((__ptr), sizeof(*(__ptr)));  \
   *(__ptr) = (__val);                                      \
   VALGRIND_MAKE_MEM_NOACCESS((__ptr), sizeof(*(__ptr)));   \
})
#else
#define VG_NOACCESS_READ(__ptr) (*(__ptr))
#define VG_NOACCESS_WRITE(__ptr, __val) (*(__ptr) = (__val))
#endif

/* Design goals:
 *
 *  - Lock free (except when resizing underlying bos)
 *
 *  - Constant time allocation with typically only one atomic
 *
 *  - Multiple allocation sizes without fragmentation
 *
 *  - Can grow while keeping addresses and offset of contents stable
 *
 *  - All allocations within one bo so we can point one of the
 *    STATE_BASE_ADDRESS pointers at it.
 *
 * The overall design is a two-level allocator: top level is a fixed size, big
 * block (8k) allocator, which operates out of a bo.  Allocation is done by
 * either pulling a block from the free list or growing the used range of the
 * bo.  Growing the range may run out of space in the bo which we then need to
 * grow.  Growing the bo is tricky in a multi-threaded, lockless environment:
 * we need to keep all pointers and contents in the old map valid.  GEM bos in
 * general can't grow, but we use a trick: we create a memfd and use ftruncate
 * to grow it as necessary.  We mmap the new size and then create a gem bo for
 * it using the new gem userptr ioctl.  Without heavy-handed locking around
 * our allocation fast-path, there isn't really a way to munmap the old mmap,
 * so we just keep it around until garbage collection time.  While the block
 * allocator is lockless for normal operations, we block other threads trying
 * to allocate while we're growing the map.  It sholdn't happen often, and
 * growing is fast anyway.
 *
 * At the next level we can use various sub-allocators.  The state pool is a
 * pool of smaller, fixed size objects, which operates much like the block
 * pool.  It uses a free list for freeing objects, but when it runs out of
 * space it just allocates a new block from the block pool.  This allocator is
 * intended for longer lived state objects such as SURFACE_STATE and most
 * other persistent state objects in the API.  We may need to track more info
 * with these object and a pointer back to the CPU object (eg VkImage).  In
 * those cases we just allocate a slightly bigger object and put the extra
 * state after the GPU state object.
 *
 * The state stream allocator works similar to how the i965 DRI driver streams
 * all its state.  Even with Vulkan, we need to emit transient state (whether
 * surface state base or dynamic state base), and for that we can just get a
 * block and fill it up.  These cases are local to a command buffer and the
 * sub-allocator need not be thread safe.  The streaming allocator gets a new
 * block when it runs out of space and chains them together so they can be
 * easily freed.
 */

/* Allocations are always at least 64 byte aligned, so 1 is an invalid value.
 * We use it to indicate the free list is empty. */
#define EMPTY 1

struct anv_mmap_cleanup {
   void *map;
   size_t size;
   uint32_t gem_handle;
};

#define ANV_MMAP_CLEANUP_INIT ((struct anv_mmap_cleanup){0})

static inline long
sys_futex(void *addr1, int op, int val1,
          struct timespec *timeout, void *addr2, int val3)
{
   return syscall(SYS_futex, addr1, op, val1, timeout, addr2, val3);
}

static inline int
futex_wake(uint32_t *addr, int count)
{
   return sys_futex(addr, FUTEX_WAKE, count, NULL, NULL, 0);
}

static inline int
futex_wait(uint32_t *addr, int32_t value)
{
   return sys_futex(addr, FUTEX_WAIT, value, NULL, NULL, 0);
}

static inline int
memfd_create(const char *name, unsigned int flags)
{
   return syscall(SYS_memfd_create, name, flags);
}

static inline uint32_t
ilog2_round_up(uint32_t value)
{
   assert(value != 0);
   return 32 - __builtin_clz(value - 1);
}

static inline uint32_t
round_to_power_of_two(uint32_t value)
{
   return 1 << ilog2_round_up(value);
}

static bool
anv_free_list_pop(union anv_free_list *list, void **map, uint32_t *offset)
{
   union anv_free_list current, new, old;

   current.u64 = list->u64;
   while (current.offset != EMPTY) {
      /* We have to add a memory barrier here so that the list head (and
       * offset) gets read before we read the map pointer.  This way we
       * know that the map pointer is valid for the given offset at the
       * point where we read it.
       */
      __sync_synchronize();

      uint32_t *next_ptr = *map + current.offset;
      new.offset = VG_NOACCESS_READ(next_ptr);
      new.count = current.count + 1;
      old.u64 = __sync_val_compare_and_swap(&list->u64, current.u64, new.u64);
      if (old.u64 == current.u64) {
         *offset = current.offset;
         return true;
      }
      current = old;
   }

   return false;
}

static void
anv_free_list_push(union anv_free_list *list, void *map, uint32_t offset)
{
   union anv_free_list current, old, new;
   uint32_t *next_ptr = map + offset;

   old = *list;
   do {
      current = old;
      VG_NOACCESS_WRITE(next_ptr, current.offset);
      new.offset = offset;
      new.count = current.count + 1;
      old.u64 = __sync_val_compare_and_swap(&list->u64, current.u64, new.u64);
   } while (old.u64 != current.u64);
}

/* All pointers in the ptr_free_list are assumed to be page-aligned.  This
 * means that the bottom 12 bits should all be zero.
 */
#define PFL_COUNT(x) ((uintptr_t)(x) & 0xfff)
#define PFL_PTR(x) ((void *)((uintptr_t)(x) & ~0xfff))
#define PFL_PACK(ptr, count) ({           \
   assert(((uintptr_t)(ptr) & 0xfff) == 0); \
   (void *)((uintptr_t)(ptr) | (uintptr_t)((count) & 0xfff)); \
})

static bool
anv_ptr_free_list_pop(void **list, void **elem)
{
   void *current = *list;
   while (PFL_PTR(current) != NULL) {
      void **next_ptr = PFL_PTR(current);
      void *new_ptr = VG_NOACCESS_READ(next_ptr);
      unsigned new_count = PFL_COUNT(current) + 1;
      void *new = PFL_PACK(new_ptr, new_count);
      void *old = __sync_val_compare_and_swap(list, current, new);
      if (old == current) {
         *elem = PFL_PTR(current);
         return true;
      }
      current = old;
   }

   return false;
}

static void
anv_ptr_free_list_push(void **list, void *elem)
{
   void *old, *current;
   void **next_ptr = elem;

   old = *list;
   do {
      current = old;
      VG_NOACCESS_WRITE(next_ptr, PFL_PTR(current));
      unsigned new_count = PFL_COUNT(current) + 1;
      void *new = PFL_PACK(elem, new_count);
      old = __sync_val_compare_and_swap(list, current, new);
   } while (old != current);
}

static uint32_t
anv_block_pool_grow(struct anv_block_pool *pool, uint32_t old_size);

void
anv_block_pool_init(struct anv_block_pool *pool,
                    struct anv_device *device, uint32_t block_size)
{
   assert(util_is_power_of_two(block_size));

   pool->device = device;
   pool->bo.gem_handle = 0;
   pool->bo.offset = 0;
   pool->block_size = block_size;
   pool->free_list = ANV_FREE_LIST_EMPTY;
   anv_vector_init(&pool->mmap_cleanups,
                   round_to_power_of_two(sizeof(struct anv_mmap_cleanup)), 128);

   /* Immediately grow the pool so we'll have a backing bo. */
   pool->state.next = 0;
   pool->state.end = anv_block_pool_grow(pool, 0);
}

void
anv_block_pool_finish(struct anv_block_pool *pool)
{
   struct anv_mmap_cleanup *cleanup;

   anv_vector_foreach(cleanup, &pool->mmap_cleanups) {
      if (cleanup->map)
         munmap(cleanup->map, cleanup->size);
      if (cleanup->gem_handle)
         anv_gem_close(pool->device, cleanup->gem_handle);
   }

   anv_vector_finish(&pool->mmap_cleanups);

   close(pool->fd);
}

static uint32_t
anv_block_pool_grow(struct anv_block_pool *pool, uint32_t old_size)
{
   size_t size;
   void *map;
   int gem_handle;
   struct anv_mmap_cleanup *cleanup;

   if (old_size == 0) {
      size = 32 * pool->block_size;
   } else {
      size = old_size * 2;
   }

   cleanup = anv_vector_add(&pool->mmap_cleanups);
   if (!cleanup)
      return 0;
   *cleanup = ANV_MMAP_CLEANUP_INIT;

   if (old_size == 0)
      pool->fd = memfd_create("block pool", MFD_CLOEXEC);

   if (pool->fd == -1)
      return 0;

   if (ftruncate(pool->fd, size) == -1)
      return 0;

   /* First try to see if mremap can grow the map in place. */
   map = MAP_FAILED;
   if (old_size > 0)
      map = mremap(pool->map, old_size, size, 0);
   if (map == MAP_FAILED) {
      /* Just leak the old map until we destroy the pool.  We can't munmap it
       * without races or imposing locking on the block allocate fast path. On
       * the whole the leaked maps adds up to less than the size of the
       * current map.  MAP_POPULATE seems like the right thing to do, but we
       * should try to get some numbers.
       */
      map = mmap(NULL, size, PROT_READ | PROT_WRITE,
                 MAP_SHARED | MAP_POPULATE, pool->fd, 0);
      cleanup->map = map;
      cleanup->size = size;
   }
   if (map == MAP_FAILED)
      return 0;

   gem_handle = anv_gem_userptr(pool->device, map, size);
   if (gem_handle == 0)
      return 0;
   cleanup->gem_handle = gem_handle;

   /* Now that we successfull allocated everything, we can write the new
    * values back into pool. */
   pool->map = map;
   pool->bo.gem_handle = gem_handle;
   pool->bo.size = size;
   pool->bo.map = map;
   pool->bo.index = 0;

   return size;
}

uint32_t
anv_block_pool_alloc(struct anv_block_pool *pool)
{
   uint32_t offset;
   struct anv_block_state state, old, new;

   /* Try free list first. */
   if (anv_free_list_pop(&pool->free_list, &pool->map, &offset)) {
      assert(pool->map);
      return offset;
   }

 restart:
   state.u64 = __sync_fetch_and_add(&pool->state.u64, pool->block_size);
   if (state.next < state.end) {
      assert(pool->map);
      return state.next;
   } else if (state.next == state.end) {
      /* We allocated the first block outside the pool, we have to grow it.
       * pool->next_block acts a mutex: threads who try to allocate now will
       * get block indexes above the current limit and hit futex_wait
       * below. */
      new.next = state.next + pool->block_size;
      new.end = anv_block_pool_grow(pool, state.end);
      assert(new.end > 0);
      old.u64 = __sync_lock_test_and_set(&pool->state.u64, new.u64);
      if (old.next != state.next)
         futex_wake(&pool->state.end, INT_MAX);
      return state.next;
   } else {
      futex_wait(&pool->state.end, state.end);
      goto restart;
   }
}

void
anv_block_pool_free(struct anv_block_pool *pool, uint32_t offset)
{
   anv_free_list_push(&pool->free_list, pool->map, offset);
}

static void
anv_fixed_size_state_pool_init(struct anv_fixed_size_state_pool *pool,
                               size_t state_size)
{
   /* At least a cache line and must divide the block size. */
   assert(state_size >= 64 && util_is_power_of_two(state_size));

   pool->state_size = state_size;
   pool->free_list = ANV_FREE_LIST_EMPTY;
   pool->block.next = 0;
   pool->block.end = 0;
}

static uint32_t
anv_fixed_size_state_pool_alloc(struct anv_fixed_size_state_pool *pool,
                                struct anv_block_pool *block_pool)
{
   uint32_t offset;
   struct anv_block_state block, old, new;

   /* Try free list first. */
   if (anv_free_list_pop(&pool->free_list, &block_pool->map, &offset))
      return offset;

   /* If free list was empty (or somebody raced us and took the items) we
    * allocate a new item from the end of the block */
 restart:
   block.u64 = __sync_fetch_and_add(&pool->block.u64, pool->state_size);

   if (block.next < block.end) {
      return block.next;
   } else if (block.next == block.end) {
      offset = anv_block_pool_alloc(block_pool);
      new.next = offset + pool->state_size;
      new.end = offset + block_pool->block_size;
      old.u64 = __sync_lock_test_and_set(&pool->block.u64, new.u64);
      if (old.next != block.next)
         futex_wake(&pool->block.end, INT_MAX);
      return offset;
   } else {
      futex_wait(&pool->block.end, block.end);
      goto restart;
   }
}

static void
anv_fixed_size_state_pool_free(struct anv_fixed_size_state_pool *pool,
                               struct anv_block_pool *block_pool,
                               uint32_t offset)
{
   anv_free_list_push(&pool->free_list, block_pool->map, offset);
}

void
anv_state_pool_init(struct anv_state_pool *pool,
                    struct anv_block_pool *block_pool)
{
   pool->block_pool = block_pool;
   for (unsigned i = 0; i < ANV_STATE_BUCKETS; i++) {
      size_t size = 1 << (ANV_MIN_STATE_SIZE_LOG2 + i);
      anv_fixed_size_state_pool_init(&pool->buckets[i], size);
   }
   VG(VALGRIND_CREATE_MEMPOOL(pool, 0, false));
}

void
anv_state_pool_finish(struct anv_state_pool *pool)
{
   VG(VALGRIND_DESTROY_MEMPOOL(pool));
}

struct anv_state
anv_state_pool_alloc(struct anv_state_pool *pool, size_t size, size_t align)
{
   unsigned size_log2 = ilog2_round_up(size < align ? align : size);
   assert(size_log2 <= ANV_MAX_STATE_SIZE_LOG2);
   if (size_log2 < ANV_MIN_STATE_SIZE_LOG2)
      size_log2 = ANV_MIN_STATE_SIZE_LOG2;
   unsigned bucket = size_log2 - ANV_MIN_STATE_SIZE_LOG2;

   struct anv_state state;
   state.alloc_size = 1 << size_log2;
   state.offset = anv_fixed_size_state_pool_alloc(&pool->buckets[bucket],
                                                  pool->block_pool);
   state.map = pool->block_pool->map + state.offset;
   VG(VALGRIND_MEMPOOL_ALLOC(pool, state.map, size));
   return state;
}

void
anv_state_pool_free(struct anv_state_pool *pool, struct anv_state state)
{
   assert(util_is_power_of_two(state.alloc_size));
   unsigned size_log2 = ilog2_round_up(state.alloc_size);
   assert(size_log2 >= ANV_MIN_STATE_SIZE_LOG2 &&
          size_log2 <= ANV_MAX_STATE_SIZE_LOG2);
   unsigned bucket = size_log2 - ANV_MIN_STATE_SIZE_LOG2;

   VG(VALGRIND_MEMPOOL_FREE(pool, state.map));
   anv_fixed_size_state_pool_free(&pool->buckets[bucket],
                                  pool->block_pool, state.offset);
}

#define NULL_BLOCK 1
struct stream_block {
   uint32_t next;

   /* The map for the BO at the time the block was givne to us */
   void *current_map;

#ifdef HAVE_VALGRIND
   void *_vg_ptr;
#endif
};

/* The state stream allocator is a one-shot, single threaded allocator for
 * variable sized blocks.  We use it for allocating dynamic state.
 */
void
anv_state_stream_init(struct anv_state_stream *stream,
                      struct anv_block_pool *block_pool)
{
   stream->block_pool = block_pool;
   stream->next = 0;
   stream->end = 0;
   stream->current_block = NULL_BLOCK;

   VG(VALGRIND_CREATE_MEMPOOL(stream, 0, false));
}

void
anv_state_stream_finish(struct anv_state_stream *stream)
{
   struct stream_block *sb;
   uint32_t block, next_block;

   block = stream->current_block;
   while (block != NULL_BLOCK) {
      sb = stream->block_pool->map + block;
      next_block = VG_NOACCESS_READ(&sb->next);
      VG(VALGRIND_MEMPOOL_FREE(stream, VG_NOACCESS_READ(&sb->_vg_ptr)));
      anv_block_pool_free(stream->block_pool, block);
      block = next_block;
   }

   VG(VALGRIND_DESTROY_MEMPOOL(stream));
}

struct anv_state
anv_state_stream_alloc(struct anv_state_stream *stream,
                       uint32_t size, uint32_t alignment)
{
   struct stream_block *sb;
   struct anv_state state;
   uint32_t block;

   state.offset = align_u32(stream->next, alignment);
   if (state.offset + size > stream->end) {
      block = anv_block_pool_alloc(stream->block_pool);
      void *current_map = stream->block_pool->map;
      sb = current_map + block;
      VG_NOACCESS_WRITE(&sb->current_map, current_map);
      VG_NOACCESS_WRITE(&sb->next, stream->current_block);
      VG(VG_NOACCESS_WRITE(&sb->_vg_ptr, 0));
      stream->current_block = block;
      stream->next = block + sizeof(*sb);
      stream->end = block + stream->block_pool->block_size;
      state.offset = align_u32(stream->next, alignment);
      assert(state.offset + size <= stream->end);
   }

   sb = stream->block_pool->map + stream->current_block;
   void *current_map = VG_NOACCESS_READ(&sb->current_map);

   state.map = current_map + state.offset;
   state.alloc_size = size;

#ifdef HAVE_VALGRIND
   void *vg_ptr = VG_NOACCESS_READ(&sb->_vg_ptr);
   if (vg_ptr == NULL) {
      vg_ptr = state.map;
      VG_NOACCESS_WRITE(&sb->_vg_ptr, vg_ptr);
      VALGRIND_MEMPOOL_ALLOC(stream, vg_ptr, size);
   } else {
      ptrdiff_t vg_offset = vg_ptr - current_map;
      assert(vg_offset >= stream->current_block &&
             vg_offset < stream->end);
      VALGRIND_MEMPOOL_CHANGE(stream, vg_ptr, vg_ptr,
                              (state.offset + size) - vg_offset);
   }
#endif

   stream->next = state.offset + size;

   return state;
}

struct bo_pool_bo_link {
   struct bo_pool_bo_link *next;
   struct anv_bo bo;
};

void
anv_bo_pool_init(struct anv_bo_pool *pool,
                 struct anv_device *device, uint32_t bo_size)
{
   pool->device = device;
   pool->bo_size = bo_size;
   pool->free_list = NULL;

   VG(VALGRIND_CREATE_MEMPOOL(pool, 0, false));
}

void
anv_bo_pool_finish(struct anv_bo_pool *pool)
{
   struct bo_pool_bo_link *link = PFL_PTR(pool->free_list);
   while (link != NULL) {
      struct bo_pool_bo_link link_copy = VG_NOACCESS_READ(link);

      anv_gem_munmap(link_copy.bo.map, pool->bo_size);
      anv_gem_close(pool->device, link_copy.bo.gem_handle);
      link = link_copy.next;
   }

   VG(VALGRIND_DESTROY_MEMPOOL(pool));
}

VkResult
anv_bo_pool_alloc(struct anv_bo_pool *pool, struct anv_bo *bo)
{
   VkResult result;

   void *next_free_void;
   if (anv_ptr_free_list_pop(&pool->free_list, &next_free_void)) {
      struct bo_pool_bo_link *next_free = next_free_void;
      *bo = VG_NOACCESS_READ(&next_free->bo);
      assert(bo->map == next_free);
      assert(bo->size == pool->bo_size);

      VG(VALGRIND_MEMPOOL_ALLOC(pool, bo->map, pool->bo_size));

      return VK_SUCCESS;
   }

   struct anv_bo new_bo;

   result = anv_bo_init_new(&new_bo, pool->device, pool->bo_size);
   if (result != VK_SUCCESS)
      return result;

   assert(new_bo.size == pool->bo_size);

   new_bo.map = anv_gem_mmap(pool->device, new_bo.gem_handle, 0, pool->bo_size);
   if (new_bo.map == NULL) {
      anv_gem_close(pool->device, new_bo.gem_handle);
      return vk_error(VK_ERROR_MEMORY_MAP_FAILED);
   }

   *bo = new_bo;

   VG(VALGRIND_MEMPOOL_ALLOC(pool, bo->map, pool->bo_size));

   return VK_SUCCESS;
}

void
anv_bo_pool_free(struct anv_bo_pool *pool, const struct anv_bo *bo)
{
   struct bo_pool_bo_link *link = bo->map;
   link->bo = *bo;

   VG(VALGRIND_MEMPOOL_FREE(pool, bo->map));
   anv_ptr_free_list_push(&pool->free_list, link);
}

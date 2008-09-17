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


/* main() for Cell SPU code */


#include <stdio.h>
#include <libmisc.h>

#include "spu_main.h"
#include "spu_render.h"
#include "spu_per_fragment_op.h"
#include "spu_texture.h"
#include "spu_tile.h"
//#include "spu_test.h"
#include "spu_vertex_shader.h"
#include "spu_dcache.h"
#include "cell/common.h"
#include "pipe/p_defines.h"


/*
helpful headers:
/usr/lib/gcc/spu/4.1.1/include/spu_mfcio.h
/opt/cell/sdk/usr/include/libmisc.h
*/

/* Set to 0 to disable all extraneous debugging code */
#define DEBUG 1

#if DEBUG
boolean Debug = FALSE;
boolean force_fragment_ops_fallback = TRUE;

/* These debug macros use the unusual construction ", ##__VA_ARGS__"
 * which expands to the expected comma + args if variadic arguments
 * are supplied, but swallows the comma if there are no variadic
 * arguments (which avoids syntax errors that would otherwise occur).
 */
#define DEBUG_PRINTF(format,...) \
   if (Debug) \
      printf("SPU %u: " format, spu.init.id, ##__VA_ARGS__)
#define D_PRINTF(flag, format,...) \
   if (spu.init.debug_flags & (flag)) \
      printf("SPU %u: " format, spu.init.id, ##__VA_ARGS__)

#else

#define DEBUG_PRINTF(...)
#define D_PRINTF(...)

#endif

struct spu_global spu;

struct spu_vs_context draw;


/**
 * Buffers containing dynamically generated SPU code:
 */
static unsigned char attribute_fetch_code_buffer[136 * PIPE_MAX_ATTRIBS]
    ALIGN16_ATTRIB;



/**
 * Tell the PPU that this SPU has finished copying a buffer to
 * local store and that it may be reused by the PPU.
 * This is done by writting a 16-byte batch-buffer-status block back into
 * main memory (in cell_context->buffer_status[]).
 */
static void
release_buffer(uint buffer)
{
   /* Evidently, using less than a 16-byte status doesn't work reliably */
   static const uint status[4] ALIGN16_ATTRIB
      = {CELL_BUFFER_STATUS_FREE, 0, 0, 0};

   const uint index = 4 * (spu.init.id * CELL_NUM_BUFFERS + buffer);
   uint *dst = spu.init.buffer_status + index;

   ASSERT(buffer < CELL_NUM_BUFFERS);

   mfc_put((void *) &status,    /* src in local memory */
           (unsigned int) dst,  /* dst in main memory */
           sizeof(status),      /* size */
           TAG_MISC,            /* tag is unimportant */
           0, /* tid */
           0  /* rid */);
}


/**
 * For tiles whose status is TILE_STATUS_CLEAR, write solid-filled
 * tiles back to the main framebuffer.
 */
static void
really_clear_tiles(uint surfaceIndex)
{
   const uint num_tiles = spu.fb.width_tiles * spu.fb.height_tiles;
   uint i;

   if (surfaceIndex == 0) {
      clear_c_tile(&spu.ctile);

      for (i = spu.init.id; i < num_tiles; i += spu.init.num_spus) {
         uint tx = i % spu.fb.width_tiles;
         uint ty = i / spu.fb.width_tiles;
         if (spu.ctile_status[ty][tx] == TILE_STATUS_CLEAR) {
            put_tile(tx, ty, &spu.ctile, TAG_SURFACE_CLEAR, 0);
         }
      }
   }
   else {
      clear_z_tile(&spu.ztile);

      for (i = spu.init.id; i < num_tiles; i += spu.init.num_spus) {
         uint tx = i % spu.fb.width_tiles;
         uint ty = i / spu.fb.width_tiles;
         if (spu.ztile_status[ty][tx] == TILE_STATUS_CLEAR)
            put_tile(tx, ty, &spu.ctile, TAG_SURFACE_CLEAR, 1);
      }
   }

#if 0
   wait_on_mask(1 << TAG_SURFACE_CLEAR);
#endif
}


static void
cmd_clear_surface(const struct cell_command_clear_surface *clear)
{
   DEBUG_PRINTF("CLEAR SURF %u to 0x%08x\n", clear->surface, clear->value);

   if (clear->surface == 0) {
      spu.fb.color_clear_value = clear->value;
      if (spu.init.debug_flags & CELL_DEBUG_CHECKER) {
         uint x = (spu.init.id << 4) | (spu.init.id << 12) |
            (spu.init.id << 20) | (spu.init.id << 28);
         spu.fb.color_clear_value ^= x;
      }
   }
   else {
      spu.fb.depth_clear_value = clear->value;
   }

#define CLEAR_OPT 1
#if CLEAR_OPT

   /* Simply set all tiles' status to CLEAR.
    * When we actually begin rendering into a tile, we'll initialize it to
    * the clear value.  If any tiles go untouched during the frame,
    * really_clear_tiles() will set them to the clear value.
    */
   if (clear->surface == 0) {
      memset(spu.ctile_status, TILE_STATUS_CLEAR, sizeof(spu.ctile_status));
   }
   else {
      memset(spu.ztile_status, TILE_STATUS_CLEAR, sizeof(spu.ztile_status));
   }

#else

   /*
    * This path clears the whole framebuffer to the clear color right now.
    */

   /*
   printf("SPU: %s num=%d w=%d h=%d\n",
          __FUNCTION__, num_tiles, spu.fb.width_tiles, spu.fb.height_tiles);
   */

   /* init a single tile to the clear value */
   if (clear->surface == 0) {
      clear_c_tile(&spu.ctile);
   }
   else {
      clear_z_tile(&spu.ztile);
   }

   /* walk over my tiles, writing the 'clear' tile's data */
   {
      const uint num_tiles = spu.fb.width_tiles * spu.fb.height_tiles;
      uint i;
      for (i = spu.init.id; i < num_tiles; i += spu.init.num_spus) {
         uint tx = i % spu.fb.width_tiles;
         uint ty = i / spu.fb.width_tiles;
         if (clear->surface == 0)
            put_tile(tx, ty, &spu.ctile, TAG_SURFACE_CLEAR, 0);
         else
            put_tile(tx, ty, &spu.ztile, TAG_SURFACE_CLEAR, 1);
      }
   }

   if (spu.init.debug_flags & CELL_DEBUG_SYNC) {
      wait_on_mask(1 << TAG_SURFACE_CLEAR);
   }

#endif /* CLEAR_OPT */

   DEBUG_PRINTF("CLEAR SURF done\n");
}


static void
cmd_release_verts(const struct cell_command_release_verts *release)
{
   DEBUG_PRINTF("RELEASE VERTS %u\n", release->vertex_buf);
   ASSERT(release->vertex_buf != ~0U);
   release_buffer(release->vertex_buf);
}


/**
 * Process a CELL_CMD_STATE_FRAGMENT_OPS command.
 * This involves installing new fragment ops SPU code.
 * If this function is never called, we'll use a regular C fallback function
 * for fragment processing.
 */
static void
cmd_state_fragment_ops(const struct cell_command_fragment_ops *fops)
{
   DEBUG_PRINTF("CMD_STATE_FRAGMENT_OPS\n");
   /* Copy SPU code from batch buffer to spu buffer */
   memcpy(spu.fragment_ops_code, fops->code, SPU_MAX_FRAGMENT_OPS_INSTS * 4);
   /* Copy state info (for fallback case only) */
   memcpy(&spu.depth_stencil_alpha, &fops->dsa, sizeof(fops->dsa));
   memcpy(&spu.blend, &fops->blend, sizeof(fops->blend));

   /* Parity twist!  For now, always use the fallback code by default,
    * only switching to codegen when specifically requested.  This
    * allows us to develop freely without risking taking down the
    * branch.
    *
    * Later, the parity of this check will be reversed, so that
    * codegen is *always* used, unless we specifically indicate that
    * we don't want it.
    *
    * Eventually, the option will be removed completely, because in
    * final code we'll always use codegen and won't even provide the
    * raw state records that the fallback code requires.
    */
   if (spu.init.debug_flags & CELL_DEBUG_FRAGMENT_OP_FALLBACK) {
      spu.fragment_ops = (spu_fragment_ops_func) spu.fragment_ops_code;
   }
   /* otherwise, the default fallback code remains in place */

   spu.read_depth = spu.depth_stencil_alpha.depth.enabled;
   spu.read_stencil = spu.depth_stencil_alpha.stencil[0].enabled;
}


static void
cmd_state_fragment_program(const struct cell_command_fragment_program *fp)
{
   DEBUG_PRINTF("CMD_STATE_FRAGMENT_PROGRAM\n");
   /* Copy SPU code from batch buffer to spu buffer */
   memcpy(spu.fragment_program_code, fp->code,
          SPU_MAX_FRAGMENT_PROGRAM_INSTS * 4);
#if 01
   /* Point function pointer at new code */
   spu.fragment_program = (spu_fragment_program_func)spu.fragment_program_code;
#endif
}


static void
cmd_state_framebuffer(const struct cell_command_framebuffer *cmd)
{
   DEBUG_PRINTF("FRAMEBUFFER: %d x %d at %p, cformat 0x%x  zformat 0x%x\n",
             cmd->width,
             cmd->height,
             cmd->color_start,
             cmd->color_format,
             cmd->depth_format);

   ASSERT_ALIGN16(cmd->color_start);
   ASSERT_ALIGN16(cmd->depth_start);

   spu.fb.color_start = cmd->color_start;
   spu.fb.depth_start = cmd->depth_start;
   spu.fb.color_format = cmd->color_format;
   spu.fb.depth_format = cmd->depth_format;
   spu.fb.width = cmd->width;
   spu.fb.height = cmd->height;
   spu.fb.width_tiles = (spu.fb.width + TILE_SIZE - 1) / TILE_SIZE;
   spu.fb.height_tiles = (spu.fb.height + TILE_SIZE - 1) / TILE_SIZE;

   switch (spu.fb.depth_format) {
   case PIPE_FORMAT_Z32_UNORM:
      spu.fb.zsize = 4;
      spu.fb.zscale = (float) 0xffffffffu;
      break;
   case PIPE_FORMAT_Z24S8_UNORM:
   case PIPE_FORMAT_S8Z24_UNORM:
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
      spu.fb.zsize = 4;
      spu.fb.zscale = (float) 0x00ffffffu;
      break;
   case PIPE_FORMAT_Z16_UNORM:
      spu.fb.zsize = 2;
      spu.fb.zscale = (float) 0xffffu;
      break;
   default:
      spu.fb.zsize = 0;
      break;
   }
}


static void
cmd_state_sampler(const struct cell_command_sampler *sampler)
{
   DEBUG_PRINTF("SAMPLER [%u]\n", sampler->unit);

   spu.sampler[sampler->unit] = sampler->state;
   if (spu.sampler[sampler->unit].min_img_filter == PIPE_TEX_FILTER_LINEAR)
      spu.sample_texture[sampler->unit] = sample_texture_bilinear;
   else
      spu.sample_texture[sampler->unit] = sample_texture_nearest;
}


static void
cmd_state_texture(const struct cell_command_texture *texture)
{
   const uint unit = texture->unit;
   const uint width = texture->width;
   const uint height = texture->height;

   DEBUG_PRINTF("TEXTURE [%u] at %p  size %u x %u\n",
             texture->unit, texture->start,
             texture->width, texture->height);

   spu.texture[unit].start = texture->start;
   spu.texture[unit].width = width;
   spu.texture[unit].height = height;

   spu.texture[unit].tiles_per_row = width / TILE_SIZE;

   spu.texture[unit].tex_size = (vector float) { width, height, 0.0, 0.0};
   spu.texture[unit].tex_size_mask = (vector unsigned int)
         { width - 1, height - 1, 0, 0 };
   spu.texture[unit].tex_size_x_mask = spu_splats(width - 1);
   spu.texture[unit].tex_size_y_mask = spu_splats(height - 1);
}


static void
cmd_state_vertex_info(const struct vertex_info *vinfo)
{
   DEBUG_PRINTF("VERTEX_INFO num_attribs=%u\n", vinfo->num_attribs);
   ASSERT(vinfo->num_attribs >= 1);
   ASSERT(vinfo->num_attribs <= 8);
   memcpy(&spu.vertex_info, vinfo, sizeof(*vinfo));
}


static void
cmd_state_vs_array_info(const struct cell_array_info *vs_info)
{
   const unsigned attr = vs_info->attr;

   ASSERT(attr < PIPE_MAX_ATTRIBS);
   draw.vertex_fetch.src_ptr[attr] = vs_info->base;
   draw.vertex_fetch.pitch[attr] = vs_info->pitch;
   draw.vertex_fetch.size[attr] = vs_info->size;
   draw.vertex_fetch.code_offset[attr] = vs_info->function_offset;
   draw.vertex_fetch.dirty = 1;
}


static void
cmd_state_attrib_fetch(const struct cell_attribute_fetch_code *code)
{
   mfc_get(attribute_fetch_code_buffer,
           (unsigned int) code->base,  /* src */
           code->size,
           TAG_BATCH_BUFFER,
           0, /* tid */
           0  /* rid */);
   wait_on_mask(1 << TAG_BATCH_BUFFER);

   draw.vertex_fetch.code = attribute_fetch_code_buffer;
}


static void
cmd_finish(void)
{
   DEBUG_PRINTF("FINISH\n");
   really_clear_tiles(0);
   /* wait for all outstanding DMAs to finish */
   mfc_write_tag_mask(~0);
   mfc_read_tag_status_all();
   /* send mbox message to PPU */
   spu_write_out_mbox(CELL_CMD_FINISH);
}


/**
 * Execute a batch of commands which was sent to us by the PPU.
 * See the cell_emit_state.c code to see where the commands come from.
 *
 * The opcode param encodes the location of the buffer and its size.
 */
static void
cmd_batch(uint opcode)
{
   const uint buf = (opcode >> 8) & 0xff;
   uint size = (opcode >> 16);
   uint64_t buffer[CELL_BUFFER_SIZE / 8] ALIGN16_ATTRIB;
   const unsigned usize = size / sizeof(buffer[0]);
   uint pos;

   DEBUG_PRINTF("BATCH buffer %u, len %u, from %p\n",
             buf, size, spu.init.buffers[buf]);

   ASSERT((opcode & CELL_CMD_OPCODE_MASK) == CELL_CMD_BATCH);

   ASSERT_ALIGN16(spu.init.buffers[buf]);

   size = ROUNDUP16(size);

   ASSERT_ALIGN16(spu.init.buffers[buf]);

   mfc_get(buffer,  /* dest */
           (unsigned int) spu.init.buffers[buf],  /* src */
           size,
           TAG_BATCH_BUFFER,
           0, /* tid */
           0  /* rid */);
   wait_on_mask(1 << TAG_BATCH_BUFFER);

   /* Tell PPU we're done copying the buffer to local store */
   DEBUG_PRINTF("release batch buf %u\n", buf);
   release_buffer(buf);

   /*
    * Loop over commands in the batch buffer
    */
   for (pos = 0; pos < usize; /* no incr */) {
      switch (buffer[pos]) {
      /*
       * rendering commands
       */
      case CELL_CMD_CLEAR_SURFACE:
         {
            struct cell_command_clear_surface *clr
               = (struct cell_command_clear_surface *) &buffer[pos];
            cmd_clear_surface(clr);
            pos += sizeof(*clr) / 8;
         }
         break;
      case CELL_CMD_RENDER:
         {
            struct cell_command_render *render
               = (struct cell_command_render *) &buffer[pos];
            uint pos_incr;
            cmd_render(render, &pos_incr);
            pos += pos_incr;
         }
         break;
      /*
       * state-update commands
       */
      case CELL_CMD_STATE_FRAMEBUFFER:
         {
            struct cell_command_framebuffer *fb
               = (struct cell_command_framebuffer *) &buffer[pos];
            cmd_state_framebuffer(fb);
            pos += sizeof(*fb) / 8;
         }
         break;
      case CELL_CMD_STATE_FRAGMENT_OPS:
         {
            struct cell_command_fragment_ops *fops
               = (struct cell_command_fragment_ops *) &buffer[pos];
            cmd_state_fragment_ops(fops);
            pos += sizeof(*fops) / 8;
         }
         break;
      case CELL_CMD_STATE_FRAGMENT_PROGRAM:
         {
            struct cell_command_fragment_program *fp
               = (struct cell_command_fragment_program *) &buffer[pos];
            cmd_state_fragment_program(fp);
            pos += sizeof(*fp) / 8;
         }
         break;
      case CELL_CMD_STATE_SAMPLER:
         {
            struct cell_command_sampler *sampler
               = (struct cell_command_sampler *) &buffer[pos];
            cmd_state_sampler(sampler);
            pos += sizeof(*sampler) / 8;
         }
         break;
      case CELL_CMD_STATE_TEXTURE:
         {
            struct cell_command_texture *texture
               = (struct cell_command_texture *) &buffer[pos];
            cmd_state_texture(texture);
            pos += sizeof(*texture) / 8;
         }
         break;
      case CELL_CMD_STATE_VERTEX_INFO:
         cmd_state_vertex_info((struct vertex_info *) &buffer[pos+1]);
         pos += (1 + ROUNDUP8(sizeof(struct vertex_info)) / 8);
         break;
      case CELL_CMD_STATE_VIEWPORT:
         (void) memcpy(& draw.viewport, &buffer[pos+1],
                       sizeof(struct pipe_viewport_state));
         pos += (1 + ROUNDUP8(sizeof(struct pipe_viewport_state)) / 8);
         break;
      case CELL_CMD_STATE_UNIFORMS:
         draw.constants = (const float (*)[4]) (uintptr_t) buffer[pos + 1];
         pos += 2;
         break;
      case CELL_CMD_STATE_VS_ARRAY_INFO:
         cmd_state_vs_array_info((struct cell_array_info *) &buffer[pos+1]);
         pos += (1 + ROUNDUP8(sizeof(struct cell_array_info)) / 8);
         break;
      case CELL_CMD_STATE_BIND_VS:
#if 0
         spu_bind_vertex_shader(&draw,
                                (struct cell_shader_info *) &buffer[pos+1]);
#endif
         pos += (1 + ROUNDUP8(sizeof(struct cell_shader_info)) / 8);
         break;
      case CELL_CMD_STATE_ATTRIB_FETCH:
         cmd_state_attrib_fetch((struct cell_attribute_fetch_code *)
                                &buffer[pos+1]);
         pos += (1 + ROUNDUP8(sizeof(struct cell_attribute_fetch_code)) / 8);
         break;
      /*
       * misc commands
       */
      case CELL_CMD_FINISH:
         cmd_finish();
         pos += 1;
         break;
      case CELL_CMD_RELEASE_VERTS:
         {
            struct cell_command_release_verts *release
               = (struct cell_command_release_verts *) &buffer[pos];
            cmd_release_verts(release);
            pos += sizeof(*release) / 8;
         }
         break;
      case CELL_CMD_FLUSH_BUFFER_RANGE: {
	 struct cell_buffer_range *br = (struct cell_buffer_range *)
	     &buffer[pos+1];

	 spu_dcache_mark_dirty((unsigned) br->base, br->size);
         pos += (1 + ROUNDUP8(sizeof(struct cell_buffer_range)) / 8);
	 break;
      }
      default:
         printf("SPU %u: bad opcode: 0x%llx\n", spu.init.id, buffer[pos]);
         ASSERT(0);
         break;
      }
   }

   DEBUG_PRINTF("BATCH complete\n");
}


/**
 * Temporary/simple main loop for SPEs: Get a command, execute it, repeat.
 */
static void
main_loop(void)
{
   struct cell_command cmd;
   int exitFlag = 0;

   DEBUG_PRINTF("Enter main loop\n");

   ASSERT((sizeof(struct cell_command) & 0xf) == 0);
   ASSERT_ALIGN16(&cmd);

   while (!exitFlag) {
      unsigned opcode;
      int tag = 0;

      DEBUG_PRINTF("Wait for cmd...\n");

      /* read/wait from mailbox */
      opcode = (unsigned int) spu_read_in_mbox();

      DEBUG_PRINTF("got cmd 0x%x\n", opcode);

      /* command payload */
      mfc_get(&cmd,  /* dest */
              (unsigned int) spu.init.cmd, /* src */
              sizeof(struct cell_command), /* bytes */
              tag,
              0, /* tid */
              0  /* rid */);
      wait_on_mask( 1 << tag );

      /*
       * NOTE: most commands should be contained in a batch buffer
       */

      switch (opcode & CELL_CMD_OPCODE_MASK) {
      case CELL_CMD_EXIT:
         DEBUG_PRINTF("EXIT\n");
         exitFlag = 1;
         break;
      case CELL_CMD_VS_EXECUTE:
#if 0
         spu_execute_vertex_shader(&draw, &cmd.vs);
#endif
         break;
      case CELL_CMD_BATCH:
         cmd_batch(opcode);
         break;
      default:
         printf("Bad opcode 0x%x!\n", opcode & CELL_CMD_OPCODE_MASK);
      }

   }

   DEBUG_PRINTF("Exit main loop\n");

   spu_dcache_report();
}



static void
one_time_init(void)
{
   memset(spu.ctile_status, TILE_STATUS_DEFINED, sizeof(spu.ctile_status));
   memset(spu.ztile_status, TILE_STATUS_DEFINED, sizeof(spu.ztile_status));
   invalidate_tex_cache();

   /* Install default/fallback fragment processing function.
    * This will normally be overriden by a code-gen'd function
    * unless CELL_FORCE_FRAGMENT_OPS_FALLBACK is set.
    */
   spu.fragment_ops = spu_fallback_fragment_ops;
}



/* In some versions of the SDK the SPE main takes 'unsigned long' as a
 * parameter.  In others it takes 'unsigned long long'.  Use a define to
 * select between the two.
 */
#ifdef SPU_MAIN_PARAM_LONG_LONG
typedef unsigned long long main_param_t;
#else
typedef unsigned long main_param_t;
#endif

/**
 * SPE entrypoint.
 */
int
main(main_param_t speid, main_param_t argp)
{
   int tag = 0;

   (void) speid;

   ASSERT(sizeof(tile_t) == TILE_SIZE * TILE_SIZE * 4);
   ASSERT(sizeof(struct cell_command_render) % 8 == 0);

   one_time_init();

   DEBUG_PRINTF("main() speid=%lu\n", (unsigned long) speid);
   D_PRINTF(CELL_DEBUG_FRAGMENT_OP_FALLBACK, "using fragment op fallback\n");

   mfc_get(&spu.init,  /* dest */
           (unsigned int) argp, /* src */
           sizeof(struct cell_init_info), /* bytes */
           tag,
           0, /* tid */
           0  /* rid */);
   wait_on_mask( 1 << tag );

#if 0
   if (spu.init.id==0)
      spu_test_misc();
#endif

   main_loop();

   return 0;
}


#include "util/u_memory.h"
#include "util/u_math.h"

#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"

#include "brw_screen.h"
#include "brw_winsys.h"



static void *
brw_buffer_map( struct pipe_screen *screen,
                struct pipe_buffer *buffer,
                unsigned usage )
{
   struct brw_screen *bscreen = brw_screen(screen); 
   struct brw_winsys_screen *sws = bscreen->sws;
   struct brw_buffer *buf = brw_buffer( buffer );

   if (buf->user_buffer)
      return buf->user_buffer;

   return sws->bo_map( buf->bo, 
                       (usage & PIPE_BUFFER_USAGE_CPU_WRITE) ? TRUE : FALSE );
}

static void 
brw_buffer_unmap( struct pipe_screen *screen,
                   struct pipe_buffer *buffer )
{
   struct brw_screen *bscreen = brw_screen(screen); 
   struct brw_winsys_screen *sws = bscreen->sws;
   struct brw_buffer *buf = brw_buffer( buffer );
   
   if (buf->bo)
      sws->bo_unmap(buf->bo);
}

static void
brw_buffer_destroy( struct pipe_buffer *buffer )
{
   struct brw_screen *bscreen = brw_screen( buffer->screen );
   struct brw_winsys_screen *sws = bscreen->sws;
   struct brw_buffer *buf = brw_buffer( buffer );

   assert(!p_atomic_read(&buffer->reference.count));

   if (buf->bo)
      sws->bo_unreference(buf->bo);
   
   FREE(buf);
}


static struct pipe_buffer *
brw_buffer_create(struct pipe_screen *screen,
                   unsigned alignment,
                   unsigned usage,
                   unsigned size)
{
   struct brw_screen *bscreen = brw_screen(screen);
   struct brw_winsys_screen *sws = bscreen->sws;
   struct brw_buffer *buf;
   unsigned usage_type;
   
   buf = CALLOC_STRUCT(brw_buffer);
   if (!buf)
      return NULL;
      
   pipe_reference_init(&buf->base.reference, 1);
   buf->base.screen = screen;
   buf->base.alignment = alignment;
   buf->base.usage = usage;
   buf->base.size = size;

   switch (usage & (PIPE_BUFFER_USAGE_VERTEX |
                    PIPE_BUFFER_USAGE_INDEX |
                    PIPE_BUFFER_USAGE_PIXEL |
                    PIPE_BUFFER_USAGE_CONSTANT))
   {
   case PIPE_BUFFER_USAGE_VERTEX:
   case PIPE_BUFFER_USAGE_INDEX:
   case (PIPE_BUFFER_USAGE_VERTEX|PIPE_BUFFER_USAGE_INDEX):
      usage_type = BRW_BUFFER_TYPE_VERTEX;
      break;
      
   case PIPE_BUFFER_USAGE_PIXEL:
      usage_type = BRW_BUFFER_TYPE_PIXEL;
      break;

   case PIPE_BUFFER_USAGE_CONSTANT:
      usage_type = BRW_BUFFER_TYPE_SHADER_CONSTANTS;
      break;

   default:
      usage_type = BRW_BUFFER_TYPE_GENERIC;
      break;
   }
   
   buf->bo = sws->bo_alloc( sws,
                            usage_type,
                            size,
                            alignment );
      
   return &buf->base; 
}


static struct pipe_buffer *
brw_user_buffer_create(struct pipe_screen *screen,
                       void *ptr,
                       unsigned bytes)
{
   struct brw_buffer *buf;
   
   buf = CALLOC_STRUCT(brw_buffer);
   if (!buf)
      return NULL;
      
   buf->user_buffer = ptr;
   
   pipe_reference_init(&buf->base.reference, 1);
   buf->base.screen = screen;
   buf->base.alignment = 1;
   buf->base.usage = 0;
   buf->base.size = bytes;
   
   return &buf->base; 
}


boolean brw_is_buffer_referenced_by_bo( struct brw_screen *brw_screen,
                                     struct pipe_buffer *buffer,
                                     struct brw_winsys_buffer *bo )
{
   struct brw_buffer *buf = brw_buffer(buffer);
   if (buf->bo == NULL)
      return FALSE;

   return brw_screen->sws->bo_references( bo, buf->bo );
}

   
void brw_screen_buffer_init(struct brw_screen *brw_screen)
{
   brw_screen->base.buffer_create = brw_buffer_create;
   brw_screen->base.user_buffer_create = brw_user_buffer_create;
   brw_screen->base.buffer_map = brw_buffer_map;
   brw_screen->base.buffer_unmap = brw_buffer_unmap;
   brw_screen->base.buffer_destroy = brw_buffer_destroy;
}

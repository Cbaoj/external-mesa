/**************************************************************************
 *
 * Copyright 2016 Advanced Micro Devices, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include <fcntl.h>

#include <X11/Xlib-xcb.h>
#include <xcb/dri3.h>
#include <xcb/present.h>

#include "loader.h"

#include "pipe/p_screen.h"
#include "pipe-loader/pipe_loader.h"

#include "util/u_memory.h"
#include "vl/vl_winsys.h"

struct vl_dri3_screen
{
   struct vl_screen base;
   xcb_connection_t *conn;
   xcb_drawable_t drawable;

   uint32_t width, height, depth;

   xcb_special_event_t *special_event;
};

static void
dri3_handle_present_event(struct vl_dri3_screen *scrn,
                          xcb_present_generic_event_t *ge)
{
   switch (ge->evtype) {
   case XCB_PRESENT_CONFIGURE_NOTIFY: {
      /* TODO */
      break;
   }
   case XCB_PRESENT_COMPLETE_NOTIFY: {
      /* TODO */
      break;
   }
   case XCB_PRESENT_EVENT_IDLE_NOTIFY: {
      /* TODO */
      break;
   }
   }
   free(ge);
}

static void
dri3_flush_present_events(struct vl_dri3_screen *scrn)
{
   if (scrn->special_event) {
      xcb_generic_event_t *ev;
      while ((ev = xcb_poll_for_special_event(
                   scrn->conn, scrn->special_event)) != NULL)
         dri3_handle_present_event(scrn, (xcb_present_generic_event_t *)ev);
   }
}

static bool
dri3_set_drawable(struct vl_dri3_screen *scrn, Drawable drawable)
{
   xcb_get_geometry_cookie_t geom_cookie;
   xcb_get_geometry_reply_t *geom_reply;
   xcb_void_cookie_t cookie;
   xcb_generic_error_t *error;
   xcb_present_event_t peid;

   assert(drawable);

   if (scrn->drawable == drawable)
      return true;

   scrn->drawable = drawable;

   geom_cookie = xcb_get_geometry(scrn->conn, scrn->drawable);
   geom_reply = xcb_get_geometry_reply(scrn->conn, geom_cookie, NULL);
   if (!geom_reply)
      return false;

   scrn->width = geom_reply->width;
   scrn->height = geom_reply->height;
   scrn->depth = geom_reply->depth;
   free(geom_reply);

   if (scrn->special_event) {
      xcb_unregister_for_special_event(scrn->conn, scrn->special_event);
      scrn->special_event = NULL;
   }

   peid = xcb_generate_id(scrn->conn);
   cookie =
      xcb_present_select_input_checked(scrn->conn, peid, scrn->drawable,
                      XCB_PRESENT_EVENT_MASK_CONFIGURE_NOTIFY |
                      XCB_PRESENT_EVENT_MASK_COMPLETE_NOTIFY |
                      XCB_PRESENT_EVENT_MASK_IDLE_NOTIFY);

   error = xcb_request_check(scrn->conn, cookie);
   if (error) {
      free(error);
      return false;
   } else
      scrn->special_event =
         xcb_register_for_special_xge(scrn->conn, &xcb_present_id, peid, 0);

   dri3_flush_present_events(scrn);

   return true;
}

static void
vl_dri3_flush_frontbuffer(struct pipe_screen *screen,
                          struct pipe_resource *resource,
                          unsigned level, unsigned layer,
                          void *context_private, struct pipe_box *sub_box)
{
   /* TODO */
   return;
}

static struct pipe_resource *
vl_dri3_screen_texture_from_drawable(struct vl_screen *vscreen, void *drawable)
{
   struct vl_dri3_screen *scrn = (struct vl_dri3_screen *)vscreen;

   assert(scrn);

   if (!dri3_set_drawable(scrn, (Drawable)drawable))
      return NULL;

   /* TODO */
   return NULL;
}

static struct u_rect *
vl_dri3_screen_get_dirty_area(struct vl_screen *vscreen)
{
   /* TODO */
   return NULL;
}

static uint64_t
vl_dri3_screen_get_timestamp(struct vl_screen *vscreen, void *drawable)
{
   /* TODO */
   return 0;
}

static void
vl_dri3_screen_set_next_timestamp(struct vl_screen *vscreen, uint64_t stamp)
{
   /* TODO */
   return;
}

static void *
vl_dri3_screen_get_private(struct vl_screen *vscreen)
{
   return vscreen;
}

static void
vl_dri3_screen_destroy(struct vl_screen *vscreen)
{
   struct vl_dri3_screen *scrn = (struct vl_dri3_screen *)vscreen;

   assert(vscreen);

   dri3_flush_present_events(scrn);

   if (scrn->special_event)
      xcb_unregister_for_special_event(scrn->conn, scrn->special_event);
   scrn->base.pscreen->destroy(scrn->base.pscreen);
   pipe_loader_release(&scrn->base.dev, 1);
   FREE(scrn);

   return;
}

struct vl_screen *
vl_dri3_screen_create(Display *display, int screen)
{
   struct vl_dri3_screen *scrn;
   const xcb_query_extension_reply_t *extension;
   xcb_dri3_open_cookie_t open_cookie;
   xcb_dri3_open_reply_t *open_reply;
   xcb_get_geometry_cookie_t geom_cookie;
   xcb_get_geometry_reply_t *geom_reply;
   int is_different_gpu;
   int fd;

   assert(display);

   scrn = CALLOC_STRUCT(vl_dri3_screen);
   if (!scrn)
      return NULL;

   scrn->conn = XGetXCBConnection(display);
   if (!scrn->conn)
      goto free_screen;

   xcb_prefetch_extension_data(scrn->conn , &xcb_dri3_id);
   xcb_prefetch_extension_data(scrn->conn, &xcb_present_id);
   extension = xcb_get_extension_data(scrn->conn, &xcb_dri3_id);
   if (!(extension && extension->present))
      goto free_screen;
   extension = xcb_get_extension_data(scrn->conn, &xcb_present_id);
   if (!(extension && extension->present))
      goto free_screen;

   open_cookie = xcb_dri3_open(scrn->conn, RootWindow(display, screen), None);
   open_reply = xcb_dri3_open_reply(scrn->conn, open_cookie, NULL);
   if (!open_reply)
      goto free_screen;
   if (open_reply->nfd != 1) {
      free(open_reply);
      goto free_screen;
   }

   fd = xcb_dri3_open_reply_fds(scrn->conn, open_reply)[0];
   if (fd < 0) {
      free(open_reply);
      goto free_screen;
   }
   fcntl(fd, F_SETFD, FD_CLOEXEC);
   free(open_reply);

   fd = loader_get_user_preferred_fd(fd, &is_different_gpu);
   /* TODO support different GPU */
   if (is_different_gpu)
      goto close_fd;

   geom_cookie = xcb_get_geometry(scrn->conn, RootWindow(display, screen));
   geom_reply = xcb_get_geometry_reply(scrn->conn, geom_cookie, NULL);
   if (!geom_reply)
      goto close_fd;
   /* TODO support depth other than 24 */
   if (geom_reply->depth != 24) {
      free(geom_reply);
      goto close_fd;
   }
   free(geom_reply);

   if (pipe_loader_drm_probe_fd(&scrn->base.dev, fd))
      scrn->base.pscreen = pipe_loader_create_screen(scrn->base.dev);

   if (!scrn->base.pscreen)
      goto release_pipe;

   scrn->base.destroy = vl_dri3_screen_destroy;
   scrn->base.texture_from_drawable = vl_dri3_screen_texture_from_drawable;
   scrn->base.get_dirty_area = vl_dri3_screen_get_dirty_area;
   scrn->base.get_timestamp = vl_dri3_screen_get_timestamp;
   scrn->base.set_next_timestamp = vl_dri3_screen_set_next_timestamp;
   scrn->base.get_private = vl_dri3_screen_get_private;
   scrn->base.pscreen->flush_frontbuffer = vl_dri3_flush_frontbuffer;

   return &scrn->base;

release_pipe:
   if (scrn->base.dev) {
      pipe_loader_release(&scrn->base.dev, 1);
      fd = -1;
   }
close_fd:
   if (fd != -1)
      close(fd);
free_screen:
   FREE(scrn);
   return NULL;
}

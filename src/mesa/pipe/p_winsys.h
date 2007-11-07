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

#ifndef P_WINSYS_H
#define P_WINSYS_H


/**
 * \file
 * This is the interface that Gallium3D requires any window system
 * hosting it to implement.  This is the only include file in Gallium3D
 * which is public.
 */


/** Opaque type for a buffer */
struct pipe_buffer_handle;

/**
 * Gallium3D drivers are (meant to be!) independent of both GL and the
 * window system.  The window system provides a buffer manager and a
 * set of additional hooks for things like command buffer submission,
 * etc.
 *
 * There clearly has to be some agreement between the window system
 * driver and the hardware driver about the format of command buffers,
 * etc.
 */


struct pipe_region;
struct pipe_surface;

/** Opaque type */
struct pipe_buffer_handle;

struct pipe_winsys
{
   /** Returns name of this winsys interface */
   const char *(*get_name)( struct pipe_winsys *sws );

   /** Wait for any buffered rendering to finish */
   void (*wait_idle)( struct pipe_winsys *sws );

   /**
    * Do any special operations to ensure frontbuffer contents are
    * displayed, eg copy fake frontbuffer.
    */
   void (*flush_frontbuffer)( struct pipe_winsys *sws,
                              struct pipe_surface *surf );

   /** Debug output */
   void (*printf)( struct pipe_winsys *sws,
		   const char *, ... );	


   /**
    * flags is bitmask of PIPE_SURFACE_FLAG_RENDER, PIPE_SURFACE_FLAG_TEXTURE
    */
   struct pipe_region *(*region_alloc)(struct pipe_winsys *ws,
                                       unsigned cpp, unsigned width,
                                       unsigned height, unsigned flags);

   void (*region_release)(struct pipe_winsys *ws, struct pipe_region **r);


   /** allocate a new surface (no context dependency) */
   struct pipe_surface *(*surface_alloc)(struct pipe_winsys *ws,
                                         unsigned format);

   void (*surface_release)(struct pipe_winsys *ws, struct pipe_surface **s);

   /**
    * The buffer manager is modeled after the dri_bufmgr interface, which 
    * in turn is modeled after the ARB_vertex_buffer_object extension,  
    * but this is the subset that gallium cares about.  Remember that
    * gallium gets to choose the interface it needs, and the window
    * systems must then implement that interface (rather than the
    * other way around...).
    */
   struct pipe_buffer_handle *(*buffer_create)(struct pipe_winsys *sws, 
					       unsigned alignment );

   /** Create a buffer that wraps user-space data */
   struct pipe_buffer_handle *(*user_buffer_create)(struct pipe_winsys *sws, 
                                                    void *ptr,
                                                    unsigned bytes);


   /** 
    * Map the entire data store of a buffer object into the client's address.
    * flags is bitmask of PIPE_BUFFER_FLAG_READ/WRITE. 
    */
   void *(*buffer_map)( struct pipe_winsys *sws, 
			struct pipe_buffer_handle *buf,
			unsigned flags );
   
   void (*buffer_unmap)( struct pipe_winsys *sws, 
			 struct pipe_buffer_handle *buf );

   /** Set ptr = buf, with reference counting */
   void (*buffer_reference)( struct pipe_winsys *sws,
                             struct pipe_buffer_handle **ptr,
                             struct pipe_buffer_handle *buf );

   /** 
    * Create the data store of a buffer and optionally initialize it.
    * 
    * usage is a bitmask of PIPE_BUFFER_USAGE_PIXEL/VERTEX/INDEX/CONSTANT. This
    * usage argument is only an optimization hint, not a guarantee, therefore 
    * proper behavior must be observed in all circumstances.
    */
   void (*buffer_data)(struct pipe_winsys *sws, 
		       struct pipe_buffer_handle *buf,
		       unsigned size, const void *data,
		       unsigned usage);

   /** Modify some or all of the data contained in a buffer's data store */
   void (*buffer_subdata)(struct pipe_winsys *sws, 
			  struct pipe_buffer_handle *buf,
			  unsigned long offset, 
			  unsigned long size, 
			  const void *data);

   /** Query some or all of the data contained in a buffer's data store */
   void (*buffer_get_subdata)(struct pipe_winsys *sws, 
			      struct pipe_buffer_handle *buf,
			      unsigned long offset, 
			      unsigned long size, 
			      void *data);

};



#endif /* P_WINSYS_H */

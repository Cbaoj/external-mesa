#include "draw/draw_context.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"

#include "nv04_context.h"
#include "nv04_state.h"

#include "nouveau/nouveau_channel.h"
#include "nouveau/nouveau_pushbuf.h"

boolean nv04_draw_elements( struct pipe_context *pipe,
                    struct pipe_buffer *indexBuffer,
                    unsigned indexSize,
                    unsigned prim, unsigned start, unsigned count)
{
	struct nv04_context *nv04 = nv04_context( pipe );
	struct draw_context *draw = nv04->draw;
	unsigned i;

	/*
	 * Map vertex buffers
	 */
	for (i = 0; i < PIPE_MAX_ATTRIBS; i++) {
		if (nv04->vertex_buffer[i].buffer) {
			void *buf
				= pipe->winsys->_buffer_map(pipe->winsys,
						nv04->vertex_buffer[i].buffer,
						PIPE_BUFFER_USAGE_CPU_READ);
			draw_set_mapped_vertex_buffer(draw, i, buf);
		}
	}
	/* Map index buffer, if present */
	if (indexBuffer) {
		void *mapped_indexes
			= pipe->winsys->_buffer_map(pipe->winsys, indexBuffer,
					PIPE_BUFFER_USAGE_CPU_READ);
		draw_set_mapped_element_buffer(draw, indexSize, mapped_indexes);
	}
	else {
		/* no index/element buffer */
		draw_set_mapped_element_buffer(draw, 0, NULL);
	}

	/* draw! */
	draw_arrays(nv04->draw, prim, start, count);

	/*
	 * unmap vertex/index buffers
	 */
	for (i = 0; i < PIPE_MAX_ATTRIBS; i++) {
		if (nv04->vertex_buffer[i].buffer) {
			pipe->winsys->_buffer_unmap(pipe->winsys, nv04->vertex_buffer[i].buffer);
			draw_set_mapped_vertex_buffer(draw, i, NULL);
		}
	}
	if (indexBuffer) {
		pipe->winsys->_buffer_unmap(pipe->winsys, indexBuffer);
		draw_set_mapped_element_buffer(draw, 0, NULL);
	}

	return TRUE;
}

boolean nv04_draw_arrays( struct pipe_context *pipe,
				 unsigned prim, unsigned start, unsigned count)
{
	return nv04_draw_elements(pipe, NULL, 0, prim, start, count);
}




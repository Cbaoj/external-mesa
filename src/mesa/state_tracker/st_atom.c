/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#include "glheader.h"
#include "context.h"

#include "st_context.h"
#include "st_atom.h"

       

/* This is used to initialize st->atoms[].  We could use this list
 * directly except for a single atom, st_update_constants, which has a
 * .dirty value which changes according to the parameters of the
 * current fragment and vertex programs, and so cannot be a static
 * value.
 */
static const struct st_tracked_state *atoms[] =
{
   &st_update_framebuffer,
   &st_update_clear_color,
   &st_update_depth,
   &st_update_clip,
   &st_update_tnl,
   &st_update_vs,
   &st_update_fs,
   &st_update_setup,
   &st_update_polygon_stipple,
   &st_update_viewport,
   &st_update_scissor,
   &st_update_blend,
   &st_update_stencil,
   &st_update_sampler,
   &st_update_texture,
   /* will be patched out at runtime */
/*    &st_update_constants */
};


void st_init_atoms( struct st_context *st )
{
   GLuint i;

   st->atoms = malloc(sizeof(atoms));
   st->nr_atoms = sizeof(atoms)/sizeof(*atoms);
   memcpy(st->atoms, atoms, sizeof(atoms));

   /* Patch in a pointer to the dynamic state atom:
    */
   for (i = 0; i < st->nr_atoms; i++)
      if (st->atoms[i] == &st_update_constants)
	 st->atoms[i] = &st->constants.tracked_state;

   memcpy(&st->constants.tracked_state, 
          &st_update_constants,
          sizeof(st_update_constants));
}


void st_destroy_atoms( struct st_context *st )
{
   if (st->atoms) {
      free(st->atoms);
      st->atoms = NULL;
   }
}


/***********************************************************************
 */

static GLboolean check_state( const struct st_state_flags *a,
			      const struct st_state_flags *b )
{
   return ((a->mesa & b->mesa) ||
	   (a->st & b->st));
}

static void accumulate_state( struct st_state_flags *a,
			      const struct st_state_flags *b )
{
   a->mesa |= b->mesa;
   a->st |= b->st;
}


static void xor_states( struct st_state_flags *result,
			     const struct st_state_flags *a,
			      const struct st_state_flags *b )
{
   result->mesa = a->mesa ^ b->mesa;
   result->st = a->st ^ b->st;
}


/***********************************************************************
 * Update all derived state:
 */

void st_validate_state( struct st_context *st )
{
   struct st_state_flags *state = &st->dirty;
   GLuint i;

   if (state->st == 0)
      return;

   if (1) {
      /* Debug version which enforces various sanity checks on the
       * state flags which are generated and checked to help ensure
       * state atoms are ordered correctly in the list.
       */
      struct st_state_flags examined, prev;      
      memset(&examined, 0, sizeof(examined));
      prev = *state;

      for (i = 0; i < st->nr_atoms; i++) {	 
	 const struct st_tracked_state *atom = st->atoms[i];
	 struct st_state_flags generated;

	 assert(atom->dirty.mesa ||
		atom->dirty.st);
	 assert(atom->update);

	 if (check_state(state, &atom->dirty)) {
	    st->atoms[i]->update( st );
	 }

	 accumulate_state(&examined, &atom->dirty);

	 /* generated = (prev ^ state)
	  * if (examined & generated)
	  *     fail;
	  */
	 xor_states(&generated, &prev, state);
	 assert(!check_state(&examined, &generated));
	 prev = *state;
      }
   }
   else {
      const GLuint nr = st->nr_atoms;

      for (i = 0; i < nr; i++) {	 
	 if (check_state(state, &st->atoms[i]->dirty))
	    st->atoms[i]->update( st );
      }
   }

   memset(state, 0, sizeof(*state));
}




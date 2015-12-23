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

#include "nir.h"
#include "nir_builder.h"
#include "nir_control_flow.h"

struct inline_functions_state {
   nir_function_impl *impl;
   nir_builder builder;
   bool progress;
};

static bool
inline_functions_block(nir_block *block, void *void_state)
{
   struct inline_functions_state *state = void_state;

   nir_builder *b = &state->builder;

   /* This is tricky.  We're iterating over instructions in a block but, as
    * we go, the block and its instruction list are being split into
    * pieces.  However, this *should* be safe since foreach_safe always
    * stashes the next thing in the iteration.  That next thing will
    * properly get moved to the next block when it gets split, and we
    * continue iterating there.
    */
   nir_foreach_instr_safe(block, instr) {
      if (instr->type != nir_instr_type_call)
         continue;

      state->progress = true;

      nir_call_instr *call = nir_instr_as_call(instr);
      assert(call->callee->impl);

      nir_function_impl *callee_copy =
         nir_function_impl_clone(call->callee->impl);

      exec_list_append(&state->impl->locals, &callee_copy->locals);
      exec_list_append(&state->impl->registers, &callee_copy->registers);

      b->cursor = nir_before_instr(&call->instr);

      /* Add copies of all in parameters */
      assert(call->num_params == callee_copy->num_params);
      for (unsigned i = 0; i < callee_copy->num_params; i++) {
         /* Only in or inout parameters */
         if (call->callee->params[i].param_type == nir_parameter_out)
            continue;

         nir_copy_deref_var(b, nir_deref_var_create(b->shader,
                                                    callee_copy->params[i]),
                               call->params[i]);
      }

      /* Pluck the body out of the function and place it here */
      nir_cf_list body;
      nir_cf_list_extract(&body, &callee_copy->body);
      nir_cf_reinsert(&body, b->cursor);

      b->cursor = nir_before_instr(&call->instr);

      /* Add copies of all out parameters and the return */
      assert(call->num_params == callee_copy->num_params);
      for (unsigned i = 0; i < callee_copy->num_params; i++) {
         /* Only out or inout parameters */
         if (call->callee->params[i].param_type == nir_parameter_in)
            continue;

         nir_copy_deref_var(b, call->params[i],
                               nir_deref_var_create(b->shader,
                                                    callee_copy->params[i]));
      }
      if (!glsl_type_is_void(call->callee->return_type)) {
         nir_copy_deref_var(b, call->return_deref,
                               nir_deref_var_create(b->shader,
                                                    callee_copy->return_var));
      }

      nir_instr_remove(&call->instr);
   }

   return true;
}

bool
nir_inline_functions_impl(nir_function_impl *impl)
{
   struct inline_functions_state state;

   state.progress = false;
   state.impl = impl;
   nir_builder_init(&state.builder, impl);

   nir_foreach_block(impl, inline_functions_block, &state);

   /* SSA and register indices are completely messed up now */
   nir_index_ssa_defs(impl);
   nir_index_local_regs(impl);

   nir_metadata_preserve(impl, nir_metadata_none);

   return state.progress;
}

bool
nir_inline_functions(nir_shader *shader)
{
   bool progress = false;

   nir_foreach_overload(shader, overload) {
      if (overload->impl)
         progress = nir_inline_functions_impl(overload->impl) || progress;
   }

   return progress;
}

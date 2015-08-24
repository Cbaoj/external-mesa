/*
 * Copyright © 2014 Intel Corporation
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
 *
 * Authors:
 *    Connor Abbott (cwabbott0@gmail.com)
 *
 */

#include "nir_control_flow_private.h"

/**
 * \name Control flow modification
 *
 * These functions modify the control flow tree while keeping the control flow
 * graph up-to-date. The invariants respected are:
 * 1. Each then statement, else statement, or loop body must have at least one
 *    control flow node.
 * 2. Each if-statement and loop must have one basic block before it and one
 *    after.
 * 3. Two basic blocks cannot be directly next to each other.
 * 4. If a basic block has a jump instruction, there must be only one and it
 *    must be at the end of the block.
 * 5. The CFG must always be connected - this means that we must insert a fake
 *    CFG edge for loops with no break statement.
 *
 * The purpose of the second one is so that we have places to insert code during
 * GCM, as well as eliminating the possibility of critical edges.
 */
/*@{*/

static inline void
block_add_pred(nir_block *block, nir_block *pred)
{
   _mesa_set_add(block->predecessors, pred);
}

static void
link_blocks(nir_block *pred, nir_block *succ1, nir_block *succ2)
{
   pred->successors[0] = succ1;
   block_add_pred(succ1, pred);

   pred->successors[1] = succ2;
   if (succ2 != NULL)
      block_add_pred(succ2, pred);
}

static void
unlink_blocks(nir_block *pred, nir_block *succ)
{
   if (pred->successors[0] == succ) {
      pred->successors[0] = pred->successors[1];
      pred->successors[1] = NULL;
   } else {
      assert(pred->successors[1] == succ);
      pred->successors[1] = NULL;
   }

   struct set_entry *entry = _mesa_set_search(succ->predecessors, pred);

   assert(entry);

   _mesa_set_remove(succ->predecessors, entry);
}

static void
unlink_block_successors(nir_block *block)
{
   if (block->successors[0] != NULL)
      unlink_blocks(block, block->successors[0]);
   if (block->successors[1] != NULL)
      unlink_blocks(block, block->successors[1]);
}

static void
link_non_block_to_block(nir_cf_node *node, nir_block *block)
{
   if (node->type == nir_cf_node_if) {
      /*
       * We're trying to link an if to a block after it; this just means linking
       * the last block of the then and else branches.
       */

      nir_if *if_stmt = nir_cf_node_as_if(node);

      nir_cf_node *last_then = nir_if_last_then_node(if_stmt);
      assert(last_then->type == nir_cf_node_block);
      nir_block *last_then_block = nir_cf_node_as_block(last_then);

      nir_cf_node *last_else = nir_if_last_else_node(if_stmt);
      assert(last_else->type == nir_cf_node_block);
      nir_block *last_else_block = nir_cf_node_as_block(last_else);

      if (exec_list_is_empty(&last_then_block->instr_list) ||
          nir_block_last_instr(last_then_block)->type != nir_instr_type_jump) {
         unlink_block_successors(last_then_block);
         link_blocks(last_then_block, block, NULL);
      }

      if (exec_list_is_empty(&last_else_block->instr_list) ||
          nir_block_last_instr(last_else_block)->type != nir_instr_type_jump) {
         unlink_block_successors(last_else_block);
         link_blocks(last_else_block, block, NULL);
      }
   } else {
      assert(node->type == nir_cf_node_loop);

      /*
       * We can only get to this codepath if we're inserting a new loop, or
       * at least a loop with no break statements; we can't insert break
       * statements into a loop when we haven't inserted it into the CFG
       * because we wouldn't know which block comes after the loop
       * and therefore, which block should be the successor of the block with
       * the break). Therefore, we need to insert a fake edge (see invariant
       * #5).
       */

      nir_loop *loop = nir_cf_node_as_loop(node);

      nir_cf_node *last = nir_loop_last_cf_node(loop);
      assert(last->type == nir_cf_node_block);
      nir_block *last_block =  nir_cf_node_as_block(last);

      last_block->successors[1] = block;
      block_add_pred(block, last_block);
   }
}

static void
link_block_to_non_block(nir_block *block, nir_cf_node *node)
{
   if (node->type == nir_cf_node_if) {
      /*
       * We're trying to link a block to an if after it; this just means linking
       * the block to the first block of the then and else branches.
       */

      nir_if *if_stmt = nir_cf_node_as_if(node);

      nir_cf_node *first_then = nir_if_first_then_node(if_stmt);
      assert(first_then->type == nir_cf_node_block);
      nir_block *first_then_block = nir_cf_node_as_block(first_then);

      nir_cf_node *first_else = nir_if_first_else_node(if_stmt);
      assert(first_else->type == nir_cf_node_block);
      nir_block *first_else_block = nir_cf_node_as_block(first_else);

      unlink_block_successors(block);
      link_blocks(block, first_then_block, first_else_block);
   } else {
      /*
       * For similar reasons as the corresponding case in
       * link_non_block_to_block(), don't worry about if the loop header has
       * any predecessors that need to be unlinked.
       */

      assert(node->type == nir_cf_node_loop);

      nir_loop *loop = nir_cf_node_as_loop(node);

      nir_cf_node *loop_header = nir_loop_first_cf_node(loop);
      assert(loop_header->type == nir_cf_node_block);
      nir_block *loop_header_block = nir_cf_node_as_block(loop_header);

      unlink_block_successors(block);
      link_blocks(block, loop_header_block, NULL);
   }

}

/**
 * Takes a basic block and inserts a new empty basic block before it, making its
 * predecessors point to the new block. This essentially splits the block into
 * an empty header and a body so that another non-block CF node can be inserted
 * between the two. Note that this does *not* link the two basic blocks, so
 * some kind of cleanup *must* be performed after this call.
 */

static nir_block *
split_block_beginning(nir_block *block)
{
   nir_block *new_block = nir_block_create(ralloc_parent(block));
   new_block->cf_node.parent = block->cf_node.parent;
   exec_node_insert_node_before(&block->cf_node.node, &new_block->cf_node.node);

   struct set_entry *entry;
   set_foreach(block->predecessors, entry) {
      nir_block *pred = (nir_block *) entry->key;

      unlink_blocks(pred, block);
      link_blocks(pred, new_block, NULL);
   }

   /* Any phi nodes must stay part of the new block, or else their
    * sourcse will be messed up. This will reverse the order of the phi's, but
    * order shouldn't matter.
    */
   nir_foreach_instr_safe(block, instr) {
      if (instr->type != nir_instr_type_phi)
         break;

      exec_node_remove(&instr->node);
      instr->block = new_block;
      exec_list_push_head(&new_block->instr_list, &instr->node);
   }

   return new_block;
}

static void
rewrite_phi_preds(nir_block *block, nir_block *old_pred, nir_block *new_pred)
{
   nir_foreach_instr_safe(block, instr) {
      if (instr->type != nir_instr_type_phi)
         break;

      nir_phi_instr *phi = nir_instr_as_phi(instr);
      nir_foreach_phi_src(phi, src) {
         if (src->pred == old_pred) {
            src->pred = new_pred;
            break;
         }
      }
   }
}

static void
insert_phi_undef(nir_block *block, nir_block *pred)
{
   nir_function_impl *impl = nir_cf_node_get_function(&block->cf_node);
   nir_foreach_instr(block, instr) {
      if (instr->type != nir_instr_type_phi)
         break;

      nir_phi_instr *phi = nir_instr_as_phi(instr);
      nir_ssa_undef_instr *undef =
         nir_ssa_undef_instr_create(ralloc_parent(phi),
                                    phi->dest.ssa.num_components);
      nir_instr_insert_before_cf_list(&impl->body, &undef->instr);
      nir_phi_src *src = ralloc(phi, nir_phi_src);
      src->pred = pred;
      src->src.parent_instr = &phi->instr;
      src->src.is_ssa = true;
      src->src.ssa = &undef->def;

      list_addtail(&src->src.use_link, &undef->def.uses);

      exec_list_push_tail(&phi->srcs, &src->node);
   }
}

/**
 * Moves the successors of source to the successors of dest, leaving both
 * successors of source NULL.
 */

static void
move_successors(nir_block *source, nir_block *dest)
{
   nir_block *succ1 = source->successors[0];
   nir_block *succ2 = source->successors[1];

   if (succ1) {
      unlink_blocks(source, succ1);
      rewrite_phi_preds(succ1, source, dest);
   }

   if (succ2) {
      unlink_blocks(source, succ2);
      rewrite_phi_preds(succ2, source, dest);
   }

   unlink_block_successors(dest);
   link_blocks(dest, succ1, succ2);
}

static bool
block_ends_in_jump(nir_block *block)
{
   return !exec_list_is_empty(&block->instr_list) &&
          nir_block_last_instr(block)->type == nir_instr_type_jump;
}


/* Given a basic block with no successors that has been inserted into the
 * control flow tree, gives it the successors it would normally have assuming
 * it doesn't end in a jump instruction. Also inserts phi sources with undefs
 * if necessary.
 */
static void
block_add_normal_succs(nir_block *block)
{
   if (exec_node_is_tail_sentinel(block->cf_node.node.next)) {
      nir_cf_node *parent = block->cf_node.parent;
      if (parent->type == nir_cf_node_if) {
         nir_cf_node *next = nir_cf_node_next(parent);
         assert(next->type == nir_cf_node_block);
         nir_block *next_block = nir_cf_node_as_block(next);

         link_blocks(block, next_block, NULL);
      } else {
         assert(parent->type == nir_cf_node_loop);
         nir_loop *loop = nir_cf_node_as_loop(parent);

         nir_cf_node *head = nir_loop_first_cf_node(loop);
         assert(head->type == nir_cf_node_block);
         nir_block *head_block = nir_cf_node_as_block(head);

         link_blocks(block, head_block, NULL);
         insert_phi_undef(head_block, block);
      }
   } else {
      nir_cf_node *next = nir_cf_node_next(&block->cf_node);
      if (next->type == nir_cf_node_if) {
         nir_if *next_if = nir_cf_node_as_if(next);

         nir_cf_node *first_then = nir_if_first_then_node(next_if);
         assert(first_then->type == nir_cf_node_block);
         nir_block *first_then_block = nir_cf_node_as_block(first_then);

         nir_cf_node *first_else = nir_if_first_else_node(next_if);
         assert(first_else->type == nir_cf_node_block);
         nir_block *first_else_block = nir_cf_node_as_block(first_else);

         link_blocks(block, first_then_block, first_else_block);
      } else {
         assert(next->type == nir_cf_node_loop);
         nir_loop *next_loop = nir_cf_node_as_loop(next);

         nir_cf_node *first = nir_loop_first_cf_node(next_loop);
         assert(first->type == nir_cf_node_block);
         nir_block *first_block = nir_cf_node_as_block(first);

         link_blocks(block, first_block, NULL);
         insert_phi_undef(first_block, block);
      }
   }
}

static nir_block *
split_block_end(nir_block *block)
{
   nir_block *new_block = nir_block_create(ralloc_parent(block));
   new_block->cf_node.parent = block->cf_node.parent;
   exec_node_insert_after(&block->cf_node.node, &new_block->cf_node.node);

   if (block_ends_in_jump(block)) {
      /* Figure out what successor block would've had if it didn't have a jump
       * instruction, and make new_block have that successor.
       */
      block_add_normal_succs(new_block);
   } else {
      move_successors(block, new_block);
   }

   return new_block;
}

/**
 * Inserts a non-basic block between two basic blocks and links them together.
 */

static void
insert_non_block(nir_block *before, nir_cf_node *node, nir_block *after)
{
   node->parent = before->cf_node.parent;
   exec_node_insert_after(&before->cf_node.node, &node->node);
   link_block_to_non_block(before, node);
   link_non_block_to_block(node, after);
}

/**
 * Inserts a non-basic block before a basic block.
 */

static void
insert_non_block_before_block(nir_cf_node *node, nir_block *block)
{
   /* split off the beginning of block into new_block */
   nir_block *new_block = split_block_beginning(block);

   /* insert our node in between new_block and block */
   insert_non_block(new_block, node, block);
}

/* walk up the control flow tree to find the innermost enclosed loop */
static nir_loop *
nearest_loop(nir_cf_node *node)
{
   while (node->type != nir_cf_node_loop) {
      node = node->parent;
   }

   return nir_cf_node_as_loop(node);
}

/*
 * update the CFG after a jump instruction has been added to the end of a block
 */

void
nir_handle_add_jump(nir_block *block)
{
   nir_instr *instr = nir_block_last_instr(block);
   nir_jump_instr *jump_instr = nir_instr_as_jump(instr);

   unlink_block_successors(block);

   nir_function_impl *impl = nir_cf_node_get_function(&block->cf_node);
   nir_metadata_preserve(impl, nir_metadata_none);

   if (jump_instr->type == nir_jump_break ||
       jump_instr->type == nir_jump_continue) {
      nir_loop *loop = nearest_loop(&block->cf_node);

      if (jump_instr->type == nir_jump_continue) {
         nir_cf_node *first_node = nir_loop_first_cf_node(loop);
         assert(first_node->type == nir_cf_node_block);
         nir_block *first_block = nir_cf_node_as_block(first_node);
         link_blocks(block, first_block, NULL);
      } else {
         nir_cf_node *after = nir_cf_node_next(&loop->cf_node);
         assert(after->type == nir_cf_node_block);
         nir_block *after_block = nir_cf_node_as_block(after);
         link_blocks(block, after_block, NULL);

         /* If we inserted a fake link, remove it */
         nir_cf_node *last = nir_loop_last_cf_node(loop);
         assert(last->type == nir_cf_node_block);
         nir_block *last_block =  nir_cf_node_as_block(last);
         if (last_block->successors[1] != NULL)
            unlink_blocks(last_block, after_block);
      }
   } else {
      assert(jump_instr->type == nir_jump_return);
      link_blocks(block, impl->end_block, NULL);
   }
}

static void
remove_phi_src(nir_block *block, nir_block *pred)
{
   nir_foreach_instr(block, instr) {
      if (instr->type != nir_instr_type_phi)
         break;

      nir_phi_instr *phi = nir_instr_as_phi(instr);
      nir_foreach_phi_src_safe(phi, src) {
         if (src->pred == pred) {
            list_del(&src->src.use_link);
            exec_node_remove(&src->node);
         }
      }
   }
}

/* Removes the successor of a block with a jump, and inserts a fake edge for
 * infinite loops. Note that the jump to be eliminated may be free-floating.
 */

static
void unlink_jump(nir_block *block, nir_jump_type type)
{
   if (block->successors[0])
      remove_phi_src(block->successors[0], block);
   if (block->successors[1])
      remove_phi_src(block->successors[1], block);

   if (type == nir_jump_break) {
      nir_block *next = block->successors[0];

      if (next->predecessors->entries == 1) {
         nir_loop *loop =
            nir_cf_node_as_loop(nir_cf_node_prev(&next->cf_node));

         /* insert fake link */
         nir_cf_node *last = nir_loop_last_cf_node(loop);
         assert(last->type == nir_cf_node_block);
         nir_block *last_block = nir_cf_node_as_block(last);

         last_block->successors[1] = next;
         block_add_pred(next, last_block);
      }
   }

   unlink_block_successors(block);
}

void
nir_handle_remove_jump(nir_block *block, nir_jump_type type)
{
   unlink_jump(block, type);

   block_add_normal_succs(block);

   nir_function_impl *impl = nir_cf_node_get_function(&block->cf_node);
   nir_metadata_preserve(impl, nir_metadata_none);
}

static void
insert_non_block_after_block(nir_block *block, nir_cf_node *node)
{
   /* split off the end of block into new_block */
   nir_block *new_block = split_block_end(block);

   /* insert our node in between block and new_block */
   insert_non_block(block, node, new_block);
}

/**
 * Inserts a basic block before another by merging the instructions.
 *
 * @param block the target of the insertion
 * @param before the block to be inserted - must not have been inserted before
 * @param has_jump whether \before has a jump instruction at the end
 */

static void
insert_block_before_block(nir_block *block, nir_block *before, bool has_jump)
{
   assert(!has_jump || exec_list_is_empty(&block->instr_list));

   foreach_list_typed(nir_instr, instr, node, &before->instr_list) {
      instr->block = block;
   }

   exec_list_prepend(&block->instr_list, &before->instr_list);

   if (has_jump)
      nir_handle_add_jump(block);
}

/**
 * Inserts a basic block after another by merging the instructions.
 *
 * @param block the target of the insertion
 * @param after the block to be inserted - must not have been inserted before
 * @param has_jump whether \after has a jump instruction at the end
 */

static void
insert_block_after_block(nir_block *block, nir_block *after, bool has_jump)
{
   foreach_list_typed(nir_instr, instr, node, &after->instr_list) {
      instr->block = block;
   }

   exec_list_append(&block->instr_list, &after->instr_list);

   if (has_jump)
      nir_handle_add_jump(block);
}

static void
update_if_uses(nir_cf_node *node)
{
   if (node->type != nir_cf_node_if)
      return;

   nir_if *if_stmt = nir_cf_node_as_if(node);

   if_stmt->condition.parent_if = if_stmt;
   if (if_stmt->condition.is_ssa) {
      list_addtail(&if_stmt->condition.use_link,
                   &if_stmt->condition.ssa->if_uses);
   } else {
      list_addtail(&if_stmt->condition.use_link,
                   &if_stmt->condition.reg.reg->if_uses);
   }
}

void
nir_cf_node_insert_after(nir_cf_node *node, nir_cf_node *after)
{
   update_if_uses(after);

   if (after->type == nir_cf_node_block) {
      /*
       * either node or the one after it must be a basic block, by invariant #2;
       * in either case, just merge the blocks together.
       */
      nir_block *after_block = nir_cf_node_as_block(after);

      bool has_jump = !exec_list_is_empty(&after_block->instr_list) &&
         nir_block_last_instr(after_block)->type == nir_instr_type_jump;

      if (node->type == nir_cf_node_block) {
         insert_block_after_block(nir_cf_node_as_block(node), after_block,
                                  has_jump);
      } else {
         nir_cf_node *next = nir_cf_node_next(node);
         assert(next->type == nir_cf_node_block);
         nir_block *next_block = nir_cf_node_as_block(next);

         insert_block_before_block(next_block, after_block, has_jump);
      }
   } else {
      if (node->type == nir_cf_node_block) {
         insert_non_block_after_block(nir_cf_node_as_block(node), after);
      } else {
         /*
          * We have to insert a non-basic block after a non-basic block. Since
          * every non-basic block has a basic block after it, this is equivalent
          * to inserting a non-basic block before a basic block.
          */

         nir_cf_node *next = nir_cf_node_next(node);
         assert(next->type == nir_cf_node_block);
         nir_block *next_block = nir_cf_node_as_block(next);

         insert_non_block_before_block(after, next_block);
      }
   }

   nir_function_impl *impl = nir_cf_node_get_function(node);
   nir_metadata_preserve(impl, nir_metadata_none);
}

void
nir_cf_node_insert_before(nir_cf_node *node, nir_cf_node *before)
{
   update_if_uses(before);

   if (before->type == nir_cf_node_block) {
      nir_block *before_block = nir_cf_node_as_block(before);

      bool has_jump = !exec_list_is_empty(&before_block->instr_list) &&
         nir_block_last_instr(before_block)->type == nir_instr_type_jump;

      if (node->type == nir_cf_node_block) {
         insert_block_before_block(nir_cf_node_as_block(node), before_block,
                                   has_jump);
      } else {
         nir_cf_node *prev = nir_cf_node_prev(node);
         assert(prev->type == nir_cf_node_block);
         nir_block *prev_block = nir_cf_node_as_block(prev);

         insert_block_after_block(prev_block, before_block, has_jump);
      }
   } else {
      if (node->type == nir_cf_node_block) {
         insert_non_block_before_block(before, nir_cf_node_as_block(node));
      } else {
         /*
          * We have to insert a non-basic block before a non-basic block. This
          * is equivalent to inserting a non-basic block after a basic block.
          */

         nir_cf_node *prev_node = nir_cf_node_prev(node);
         assert(prev_node->type == nir_cf_node_block);
         nir_block *prev_block = nir_cf_node_as_block(prev_node);

         insert_non_block_after_block(prev_block, before);
      }
   }

   nir_function_impl *impl = nir_cf_node_get_function(node);
   nir_metadata_preserve(impl, nir_metadata_none);
}

void
nir_cf_node_insert_begin(struct exec_list *list, nir_cf_node *node)
{
   nir_cf_node *begin = exec_node_data(nir_cf_node, list->head, node);
   nir_cf_node_insert_before(begin, node);
}

void
nir_cf_node_insert_end(struct exec_list *list, nir_cf_node *node)
{
   nir_cf_node *end = exec_node_data(nir_cf_node, list->tail_pred, node);
   nir_cf_node_insert_after(end, node);
}

/**
 * Stitch two basic blocks together into one. The aggregate must have the same
 * predecessors as the first and the same successors as the second.
 */

static void
stitch_blocks(nir_block *before, nir_block *after)
{
   /*
    * We move after into before, so we have to deal with up to 2 successors vs.
    * possibly a large number of predecessors.
    *
    * TODO: special case when before is empty and after isn't?
    */

   move_successors(after, before);

   foreach_list_typed(nir_instr, instr, node, &after->instr_list) {
      instr->block = before;
   }

   exec_list_append(&before->instr_list, &after->instr_list);
   exec_node_remove(&after->cf_node.node);
}


static void
cleanup_cf_node(nir_cf_node *node)
{
   switch (node->type) {
   case nir_cf_node_block: {
      nir_block *block = nir_cf_node_as_block(node);
      /* We need to walk the instructions and clean up defs/uses */
      nir_foreach_instr_safe(block, instr)
         if (instr->type != nir_instr_type_jump)
            nir_instr_remove(instr);
      break;
   }

   case nir_cf_node_if: {
      nir_if *if_stmt = nir_cf_node_as_if(node);
      foreach_list_typed(nir_cf_node, child, node, &if_stmt->then_list)
         cleanup_cf_node(child);
      foreach_list_typed(nir_cf_node, child, node, &if_stmt->else_list)
         cleanup_cf_node(child);

      list_del(&if_stmt->condition.use_link);
      break;
   }

   case nir_cf_node_loop: {
      nir_loop *loop = nir_cf_node_as_loop(node);
      foreach_list_typed(nir_cf_node, child, node, &loop->body)
         cleanup_cf_node(child);
      break;
   }
   case nir_cf_node_function: {
      nir_function_impl *impl = nir_cf_node_as_function(node);
      foreach_list_typed(nir_cf_node, child, node, &impl->body)
         cleanup_cf_node(child);
      break;
   }
   default:
      unreachable("Invalid CF node type");
   }
}

void
nir_cf_node_remove(nir_cf_node *node)
{
   nir_function_impl *impl = nir_cf_node_get_function(node);
   nir_metadata_preserve(impl, nir_metadata_none);

   if (node->type == nir_cf_node_block) {
      /*
       * Basic blocks can't really be removed by themselves, since they act as
       * padding between the non-basic blocks. So all we do here is empty the
       * block of instructions.
       *
       * TODO: could we assert here?
       */
      exec_list_make_empty(&nir_cf_node_as_block(node)->instr_list);
   } else {
      nir_cf_node *before = nir_cf_node_prev(node);
      assert(before->type == nir_cf_node_block);
      nir_block *before_block = nir_cf_node_as_block(before);

      nir_cf_node *after = nir_cf_node_next(node);
      assert(after->type == nir_cf_node_block);
      nir_block *after_block = nir_cf_node_as_block(after);

      exec_node_remove(&node->node);
      stitch_blocks(before_block, after_block);
   }

   cleanup_cf_node(node);
}

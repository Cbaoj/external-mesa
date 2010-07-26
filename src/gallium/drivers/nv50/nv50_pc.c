/*
 * Copyright 2010 Christoph Bumiller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "nv50_pc.h"
#include "nv50_program.h"

#include <stdio.h>

/* returns TRUE if operands 0 and 1 can be swapped */
boolean
nv_op_commutative(uint opcode)
{
   switch (opcode) {
   case NV_OP_ADD:
   case NV_OP_MUL:
   case NV_OP_MAD:
   case NV_OP_AND:
   case NV_OP_OR:
   case NV_OP_XOR:
   case NV_OP_MIN:
   case NV_OP_MAX:
   case NV_OP_SAD:
     return TRUE;
   default:
     return FALSE;
   }
}

/* return operand to which the address register applies */
int
nv50_indirect_opnd(struct nv_instruction *i)
{
   if (!i->src[4])
      return -1;

   switch (i->opcode) {
   case NV_OP_MOV:
   case NV_OP_LDA:
      return 0;
   default:
      return 1;
   }
}

boolean
nv50_nvi_can_use_imm(struct nv_instruction *nvi, int s)
{
   if (nvi->flags_src || nvi->flags_def)
      return FALSE;

   switch (nvi->opcode) {
   case NV_OP_ADD:
   case NV_OP_MUL:
   case NV_OP_AND:
   case NV_OP_OR:
   case NV_OP_XOR:
   case NV_OP_SHL:
   case NV_OP_SHR:
      return (s == 1) && (nvi->def[0]->reg.file == NV_FILE_GPR);
   case NV_OP_MOV:
      assert(s == 0);
      return (nvi->def[0]->reg.file == NV_FILE_GPR);
   default:
      return FALSE;
   }
}

boolean
nv50_nvi_can_load(struct nv_instruction *nvi, int s, struct nv_value *value)
{
   switch (nvi->opcode) {
   case NV_OP_ABS:
   case NV_OP_ADD:
   case NV_OP_CEIL:
   case NV_OP_FLOOR:
   case NV_OP_TRUNC:
   case NV_OP_CVT:
   case NV_OP_MAD:
   case NV_OP_MUL:
   case NV_OP_SAT:
   case NV_OP_SUB:
   case NV_OP_MAX:
   case NV_OP_MIN:
      if (s == 0 && (value->reg.file == NV_FILE_MEM_S ||
                     value->reg.file == NV_FILE_MEM_P))
         return TRUE;
      if (s == 1 &&
          value->reg.file >= NV_FILE_MEM_C(0) &&
          value->reg.file <= NV_FILE_MEM_C(15))
         return TRUE;
      if (s == 2 && nvi->src[1]->value->reg.file == NV_FILE_GPR)
         return TRUE;
      return FALSE;
   case NV_OP_MOV:
      assert(s == 0);
      return TRUE;
   default:
      return FALSE;
   }
}

ubyte
nv50_supported_src_mods(uint opcode, int s)
{
   switch (opcode) {
   case NV_OP_ABS:
      return NV_MOD_NEG | NV_MOD_ABS; /* obviously */
   case NV_OP_ADD:
   case NV_OP_MUL:
   case NV_OP_MAD:
      return NV_MOD_NEG;
   case NV_OP_DFDX:
   case NV_OP_DFDY:
      assert(s == 0);
      return NV_MOD_NEG;
   case NV_OP_MAX:
   case NV_OP_MIN:
      return NV_MOD_ABS;
   case NV_OP_CVT:
   case NV_OP_LG2:
   case NV_OP_NEG:
   case NV_OP_PREEX2:
   case NV_OP_PRESIN:
   case NV_OP_RCP:
   case NV_OP_RSQ:
      return NV_MOD_ABS | NV_MOD_NEG;
   default:
      return 0;
   }
}

int
nv_nvi_refcount(struct nv_instruction *nvi)
{
   int i, rc;

   rc = nvi->flags_def ? nvi->flags_def->refc : 0;

   for (i = 0; i < 4; ++i) {
      if (!nvi->def[i])
         return rc;
      rc += nvi->def[i]->refc;
   }
   return rc;
}

int
nvcg_replace_value(struct nv_pc *pc, struct nv_value *old_val,
                   struct nv_value *new_val)
{
   int i, n;

   if (old_val == new_val)
      return old_val->refc;

   for (i = 0, n = 0; i < pc->num_refs; ++i) {
      if (pc->refs[i]->value == old_val) {
         ++n;
         nv_reference(pc, &pc->refs[i], new_val);
      }
   }
   return n;
}

static void
nv_pc_free_refs(struct nv_pc *pc)
{
   int i;
   for (i = 0; i < pc->num_refs; i += 64)
      FREE(pc->refs[i]);
}

void
nv_print_program(struct nv_basic_block *b)
{
   struct nv_instruction *i = b->phi;

   b->priv = 0;

   debug_printf("=== BB %i ", b->id);
   if (b->out[0])
      debug_printf("(--0> %i) ", b->out[0]->id);
   if (b->out[1])
      debug_printf("(--1> %i) ", b->out[1]->id);
   debug_printf("===\n");

   if (!i)
      i = b->entry;
   for (; i; i = i->next)
      nv_print_instruction(i);

   if (!b->out[0]) {
      debug_printf("END\n\n");
      return;
   }
   if (!b->out[1] && ++(b->out[0]->priv) != b->out[0]->num_in)
      return;

   if (b->out[0] != b)
      nv_print_program(b->out[0]);

   if (b->out[1] && b->out[1] != b)
      nv_print_program(b->out[1]);
}

static INLINE void
nvcg_show_bincode(struct nv_pc *pc)
{
   int i;

   for (i = 0; i < pc->bin_size / 4; ++i)
      debug_printf("0x%08x ", pc->emit[i]);
   debug_printf("\n");
}

static int
nv50_emit_program(struct nv_pc *pc)
{
   uint32_t *code = pc->emit;
   int n;

   debug_printf("emitting program: size = %u\n", pc->bin_size);

   for (n = 0; n < pc->num_blocks; ++n) {
      struct nv_instruction *i;
      struct nv_basic_block *b = pc->bb_list[n];

      for (i = b->entry; i; i = i->next) {
         nv50_emit_instruction(pc, i);

         pc->bin_pos += 1 + (pc->emit[0] & 1);
         pc->emit += 1 + (pc->emit[0] & 1);
      }
   }
   assert(pc->emit == &code[pc->bin_size / 4]);

   /* XXX: we can do better than this ... */
   if ((pc->emit[-1] & 3) == 3) {
      pc->emit[0] = 0xf0000001;
      pc->emit[1] = 0xe0000000;
      pc->bin_size += 8;
   }

   pc->emit = code;
   code[pc->bin_size / 4 - 1] |= 1;

   nvcg_show_bincode(pc);

   return 0;
}

int
nv50_generate_code(struct nv50_translation_info *ti)
{
   struct nv_pc *pc;
   int ret;

   pc = CALLOC_STRUCT(nv_pc);
   if (!pc)
      return 1;

   ret = nv50_tgsi_to_nc(pc, ti);
   if (ret)
      goto out;

   /* optimization */
   ret = nv_pc_exec_pass0(pc);
   if (ret)
      goto out;

   /* register allocation */
   ret = nv_pc_exec_pass1(pc);
   if (ret)
      goto out;

   /* prepare for emission */
   ret = nv_pc_exec_pass2(pc);
   if (ret)
      goto out;

   pc->emit = CALLOC(pc->bin_size / 4 + 2, 4);
   if (!pc->emit) {
      ret = 3;
      goto out;
   }
   ret = nv50_emit_program(pc);
   if (ret)
      goto out;

   ti->p->code_size = pc->bin_size;
   ti->p->code = pc->emit;

   ti->p->immd_size = pc->immd_count * 4;
   ti->p->immd = pc->immd_buf;

   ti->p->max_gpr = (pc->max_reg[NV_FILE_GPR] + 1) >> 1;
   ti->p->max_gpr++;

   ti->p->fixups = pc->fixups;
   ti->p->num_fixups = pc->num_fixups;

   debug_printf("SHADER TRANSLATION - %s\n", ret ? "failure" : "success");

out:
   nv_pc_free_refs(pc);
   if (ret) {
      if (pc->emit)
         free(pc->emit);
      if (pc->immd_buf)
         free(pc->immd_buf);
      if (pc->fixups)
         free(pc->fixups);
   }
   free(pc);

   return ret;
}

static void
nvbb_insert_phi(struct nv_basic_block *b, struct nv_instruction *i)
{
   if (!b->phi) {
      i->prev = NULL;
      b->phi = i;
      i->next = b->entry;
      if (b->entry) {
         assert(!b->entry->prev && b->exit);
         b->entry->prev = i;
      } else {
         b->entry = i;
	 b->exit = i;
      }
   } else {
      assert(b->entry);
      if (b->entry->opcode == NV_OP_PHI) { /* insert after entry */
	 assert(b->entry == b->exit);
         b->entry->next = i;
         i->prev = b->entry;
         b->entry = i;
	 b->exit = i;
      } else { /* insert before entry */
         assert(b->entry->prev && b->exit);
         i->next = b->entry;
         i->prev = b->entry->prev;
         b->entry->prev = i;
         i->prev->next = i;
      }
   }
}

void
nvbb_insert_tail(struct nv_basic_block *b, struct nv_instruction *i)
{
   if (i->opcode == NV_OP_PHI) {
      nvbb_insert_phi(b, i);
   } else {
      i->prev = b->exit;
      if (b->exit)
         b->exit->next = i;
      b->exit = i;
      if (!b->entry)
         b->entry = i;
      else
      if (i->prev && i->prev->opcode == NV_OP_PHI)
         b->entry = i;
   }

   i->bb = b;
   b->num_instructions++;
}

void
nv_nvi_delete(struct nv_instruction *nvi)
{
   struct nv_basic_block *b = nvi->bb;
   int j;

   debug_printf("REM: "); nv_print_instruction(nvi);

   for (j = 0; j < 4; ++j) {
      if (!nvi->src[j])
         break;
      --(nvi->src[j]->value->refc);
      nvi->src[j] = NULL;
   }	       

   if (nvi->next)
      nvi->next->prev = nvi->prev;
   else {
      assert(nvi == b->exit);
      b->exit = nvi->prev;
   }

   if (nvi->prev)
      nvi->prev->next = nvi->next;

   if (nvi == b->entry) {
      assert(nvi->opcode != NV_OP_PHI || !nvi->next);

      if (!nvi->next || (nvi->opcode == NV_OP_PHI))
         b->entry = nvi->prev;
      else
         b->entry = nvi->next;
   }

   if (nvi == b->phi) {
      assert(!nvi->prev);
      if (nvi->opcode != NV_OP_PHI)
         debug_printf("WARN: b->phi points to non-PHI instruction\n");

      if (!nvi->next || nvi->next->opcode != NV_OP_PHI)
         b->phi = NULL;
      else
         b->phi = nvi->next;
   }
}

void
nv_nvi_permute(struct nv_instruction *i1, struct nv_instruction *i2)
{
   struct nv_basic_block *b = i1->bb;

   assert(i1->opcode != NV_OP_PHI &&
          i2->opcode != NV_OP_PHI);
   assert(i1->next == i2);

   if (b->exit == i2)
      b->exit = i1;

   if (b->entry == i1)
      b->entry = i2;

   i2->prev = i1->prev;
   i1->next = i2->next;
   i2->next = i1;
   i1->prev = i2;

   if (i2->prev)
      i2->prev->next = i2;
   if (i1->next)
      i1->next->prev = i1;
}

void nvbb_attach_block(struct nv_basic_block *parent, struct nv_basic_block *b)
{
   if (parent->out[0]) {
      assert(!parent->out[1]);
      parent->out[1] = b;
   } else
      parent->out[0] = b;

   b->in[b->num_in++] = parent;
}

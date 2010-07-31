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

/* XXX: need to clean this up so we get the typecasting right more naturally */

#include <unistd.h>

#include "nv50_context.h"
#include "nv50_pc.h"

#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"

#include "util/u_simple_list.h"
#include "tgsi/tgsi_dump.h"

#define BLD_MAX_TEMPS 64
#define BLD_MAX_ADDRS 4
#define BLD_MAX_PREDS 4
#define BLD_MAX_IMMDS 128

#define BLD_MAX_COND_NESTING 4
#define BLD_MAX_LOOP_NESTING 4
#define BLD_MAX_CALL_NESTING 2

/* collects all values assigned to the same TGSI register */
struct bld_value_stack {
   struct nv_value *top;
   struct nv_value **body;
   unsigned size;
};

static INLINE void
bld_push_value(struct bld_value_stack *stk)
{
   assert(!stk->size || (stk->body[stk->size - 1] != stk->top));

   if (!(stk->size % 8)) {
      unsigned old_sz = (stk->size + 0) * sizeof(struct nv_value *);
      unsigned new_sz = (stk->size + 8) * sizeof(struct nv_value *);
      stk->body = (struct nv_value **)REALLOC(stk->body, old_sz, new_sz);
   }
   stk->body[stk->size++] = stk->top;
   stk->top = NULL;
}

static INLINE void
bld_push_values(struct bld_value_stack *stacks, int n)
{
   int i, c;

   for (i = 0; i < n; ++i)
      for (c = 0; c < 4; ++c)
         if (stacks[i * 4 + c].top)
            bld_push_value(&stacks[i * 4 + c]);
}

#define FETCH_TEMP(i, c)    (bld->tvs[i][c].top)
#define STORE_TEMP(i, c, v) (bld->tvs[i][c].top = (v))
#define FETCH_ADDR(i, c)    (bld->avs[i][c].top)
#define STORE_ADDR(i, c, v) (bld->avs[i][c].top = (v))
#define FETCH_PRED(i, c)    (bld->pvs[i][c].top)
#define STORE_PRED(i, c, v) (bld->pvs[i][c].top = (v))
#define FETCH_OUTR(i, c)    (bld->ovs[i][c].top)
#define STORE_OUTR(i, c, v)                                         \
   do {                                                             \
      bld->ovs[i][c].top = (v);                                     \
      bld->outputs_written[(i) / 8] |= 1 << (((i) * 4 + (c)) % 32); \
   } while (0)

struct bld_context {
   struct nv50_translation_info *ti;

   struct nv_pc *pc;
   struct nv_basic_block *b;

   struct tgsi_parse_context parse[BLD_MAX_CALL_NESTING];
   int call_lvl;

   struct nv_basic_block *cond_bb[BLD_MAX_COND_NESTING];
   struct nv_basic_block *join_bb[BLD_MAX_COND_NESTING];
   struct nv_basic_block *else_bb[BLD_MAX_COND_NESTING];
   int cond_lvl;
   struct nv_basic_block *loop_bb[BLD_MAX_LOOP_NESTING];
   int loop_lvl;

   struct bld_value_stack tvs[BLD_MAX_TEMPS][4]; /* TGSI_FILE_TEMPORARY */
   struct bld_value_stack avs[BLD_MAX_ADDRS][4]; /* TGSI_FILE_ADDRESS */
   struct bld_value_stack pvs[BLD_MAX_PREDS][4]; /* TGSI_FILE_PREDICATE */
   struct bld_value_stack ovs[PIPE_MAX_SHADER_OUTPUTS][4];

   uint32_t outputs_written[PIPE_MAX_SHADER_OUTPUTS / 32];

   struct nv_value *frgcrd[4];
   struct nv_value *sysval[4];

   /* wipe on new BB */
   struct nv_value *saved_addr[4][2];
   struct nv_value *saved_inputs[128];
   struct nv_value *saved_immd[BLD_MAX_IMMDS];
   uint num_immds;
};

static INLINE struct nv_value *
bld_def(struct nv_instruction *i, int c, struct nv_value *value)
{
   i->def[c] = value;
   value->insn = i;
   return value;
}

static INLINE struct nv_value *
find_by_bb(struct bld_value_stack *stack, struct nv_basic_block *b)
{
   int i;

   if (stack->top && stack->top->insn->bb == b)
      return stack->top;

   for (i = stack->size - 1; i >= 0; --i)
      if (stack->body[i]->insn->bb == b)
         return stack->body[i];
   return NULL;
}

/* fetch value from stack that was defined in the specified basic block,
 * or search for first definitions in all of its predecessors
 */
static void
fetch_by_bb(struct bld_value_stack *stack,
            struct nv_value **vals, int *n,
            struct nv_basic_block *b)
{
   int i;
   struct nv_value *val;

   assert(*n < 16); /* MAX_COND_NESTING */

   val = find_by_bb(stack, b);
   if (val) {
      for (i = 0; i < *n; ++i)
         if (vals[i] == val)
            return;
      vals[(*n)++] = val;
      return;
   }
   for (i = 0; i < b->num_in; ++i)
      fetch_by_bb(stack, vals, n, b->in[i]);
}

static struct nv_value *
bld_fetch_global(struct bld_context *bld, struct bld_value_stack *stack)
{
   struct nv_value *vals[16], *phi = NULL;
   int j, i = 0, n = 0;

   fetch_by_bb(stack, vals, &n, bld->pc->current_block);

   if (n == 0)
      return NULL;
   if (n == 1)
      return vals[0];

   debug_printf("phi required: %i candidates\n", n);

   while (i < n) {
      struct nv_instruction *insn = new_instruction(bld->pc, NV_OP_PHI);

      j = phi ? 1 : 0;
      if (phi)
         insn->src[0] = new_ref(bld->pc, phi);

      phi = new_value(bld->pc, vals[0]->reg.file, vals[0]->reg.type);

      bld_def(insn, 0, phi);

      for (; j < 4; ++j) {
         insn->src[j] = new_ref(bld->pc, vals[i++]);
         if (i == n)
            break;
      }
      debug_printf("new phi: %i, %i in\n", phi->n, j);
   }

   /* insert_at_head(list, phi) is done at end of block */
   return phi;
}

static INLINE struct nv_value *
bld_imm_u32(struct bld_context *bld, uint32_t u)
{
   int i;
   unsigned n = bld->num_immds;

   debug_printf("bld_imm_u32: 0x%08x\n", u);

   for (i = 0; i < n; ++i)
      if (bld->saved_immd[i]->reg.imm.u32 == u)
         return bld->saved_immd[i];
   assert(n < BLD_MAX_IMMDS);

   debug_printf("need new one\n");

   bld->num_immds++;

   bld->saved_immd[n] = new_value(bld->pc, NV_FILE_IMM, NV_TYPE_U32);
   bld->saved_immd[n]->reg.imm.u32 = u;
   return bld->saved_immd[n];
}

static INLINE struct nv_value *
bld_imm_f32(struct bld_context *bld, float f)
{
   return bld_imm_u32(bld, fui(f));
}

#define SET_TYPE(v, t) ((v)->reg.type = NV_TYPE_##t)

static struct nv_value *
bld_insn_1(struct bld_context *bld, uint opcode, struct nv_value *src0)
{
   struct nv_instruction *insn = new_instruction(bld->pc, opcode);
   assert(insn);

   nv_reference(bld->pc, &insn->src[0], src0); /* NOTE: new_ref would suffice */
   
   return bld_def(insn, 0, new_value(bld->pc, NV_FILE_GPR, src0->reg.type));
}

static struct nv_value *
bld_insn_2(struct bld_context *bld, uint opcode,
	      struct nv_value *src0, struct nv_value *src1)
{
   struct nv_instruction *insn = new_instruction(bld->pc, opcode);

   nv_reference(bld->pc, &insn->src[0], src0);
   nv_reference(bld->pc, &insn->src[1], src1);

   return bld_def(insn, 0, new_value(bld->pc, NV_FILE_GPR, src0->reg.type));
}

static struct nv_value *
bld_insn_3(struct bld_context *bld, uint opcode,
              struct nv_value *src0, struct nv_value *src1,
              struct nv_value *src2)
{
   struct nv_instruction *insn = new_instruction(bld->pc, opcode);

   nv_reference(bld->pc, &insn->src[0], src0);
   nv_reference(bld->pc, &insn->src[1], src1);
   nv_reference(bld->pc, &insn->src[2], src2);

   return bld_def(insn, 0, new_value(bld->pc, NV_FILE_GPR, src0->reg.type));
}

#define BLD_INSN_1_EX(d, op, dt, s0, s0t)           \
   do {                                             \
      (d) = bld_insn_1(bld, (NV_OP_##op), (s0));    \
      (d)->reg.type = NV_TYPE_##dt;                 \
      (d)->insn->src[0]->typecast = NV_TYPE_##s0t;  \
   } while(0)

#define BLD_INSN_2_EX(d, op, dt, s0, s0t, s1, s1t)       \
   do {                                                  \
      (d) = bld_insn_2(bld, (NV_OP_##op), (s0), (s1));   \
      (d)->reg.type = NV_TYPE_##dt;                      \
      (d)->insn->src[0]->typecast = NV_TYPE_##s0t;       \
      (d)->insn->src[1]->typecast = NV_TYPE_##s1t;       \
   } while(0)

static struct nv_value *
bld_pow(struct bld_context *bld, struct nv_value *x, struct nv_value *e)
{
   struct nv_value *val;

   BLD_INSN_1_EX(val, LG2, F32, x, F32);
   BLD_INSN_2_EX(val, MUL, F32, e, F32, val, F32);
   val = bld_insn_1(bld, NV_OP_PREEX2, val);
   val = bld_insn_1(bld, NV_OP_EX2, val);

   return val;
}

static INLINE struct nv_value *
bld_load_imm_f32(struct bld_context *bld, float f)
{
   return bld_insn_1(bld, NV_OP_MOV, bld_imm_f32(bld, f));
}

static INLINE struct nv_value *
bld_load_imm_u32(struct bld_context *bld, uint32_t u)
{
   return bld_insn_1(bld, NV_OP_MOV, bld_imm_u32(bld, u));
}

static struct nv_value *
bld_get_address(struct bld_context *bld, int id, struct nv_value *indirect)
{
   int i;
   struct nv_instruction *nvi;

   for (i = 0; i < 4; ++i) {
      if (!bld->saved_addr[i][0])
         break;
      if (bld->saved_addr[i][1] == indirect) {
         nvi = bld->saved_addr[i][0]->insn;
         if (nvi->src[0]->value->reg.imm.u32 == id)
            return bld->saved_addr[i][0];
      }
   }
   i &= 3;

   bld->saved_addr[i][0] = bld_load_imm_u32(bld, id);
   bld->saved_addr[i][0]->reg.file = NV_FILE_ADDR;
   bld->saved_addr[i][1] = indirect;
   return bld->saved_addr[i][0];
}


static struct nv_value *
bld_predicate(struct bld_context *bld, struct nv_value *src)
{
   struct nv_instruction *nvi = src->insn;

   if (nvi->opcode == NV_OP_LDA ||
       nvi->opcode == NV_OP_PHI ||
       nvi->bb != bld->pc->current_block) {
      nvi = new_instruction(bld->pc, NV_OP_CVT);
      nv_reference(bld->pc, &nvi->src[0], src);
   }

   if (!nvi->flags_def) {
      nvi->flags_def = new_value(bld->pc, NV_FILE_FLAGS, NV_TYPE_U16);
      nvi->flags_def->insn = nvi;
   }
   return nvi->flags_def;
}

static void
bld_kil(struct bld_context *bld, struct nv_value *src)
{
   struct nv_instruction *nvi;

   src = bld_predicate(bld, src);
   nvi = new_instruction(bld->pc, NV_OP_KIL);
   nvi->fixed = 1;
   nvi->flags_src = new_ref(bld->pc, src);
   nvi->cc = NV_CC_LT;
}

static void
bld_flow(struct bld_context *bld, uint opcode, ubyte cc,
         struct nv_value *src, boolean plan_reconverge)
{
   struct nv_instruction *nvi;

   if (plan_reconverge)
      new_instruction(bld->pc, NV_OP_JOINAT)->fixed = 1;

   nvi = new_instruction(bld->pc, opcode);
   nvi->is_terminator = 1;
   nvi->cc = cc;
   nvi->flags_src = new_ref(bld->pc, src);
}

static ubyte
translate_setcc(unsigned opcode)
{
   switch (opcode) {
   case TGSI_OPCODE_SLT: return NV_CC_LT;
   case TGSI_OPCODE_SGE: return NV_CC_GE;
   case TGSI_OPCODE_SEQ: return NV_CC_EQ;
   case TGSI_OPCODE_SGT: return NV_CC_GT;
   case TGSI_OPCODE_SLE: return NV_CC_LE;
   case TGSI_OPCODE_SNE: return NV_CC_NE | NV_CC_U;
   case TGSI_OPCODE_STR: return NV_CC_TR;
   case TGSI_OPCODE_SFL: return NV_CC_FL;

   case TGSI_OPCODE_ISLT: return NV_CC_LT;
   case TGSI_OPCODE_ISGE: return NV_CC_GE;
   case TGSI_OPCODE_USEQ: return NV_CC_EQ;
   case TGSI_OPCODE_USGE: return NV_CC_GE;
   case TGSI_OPCODE_USLT: return NV_CC_LT;
   case TGSI_OPCODE_USNE: return NV_CC_NE;
   default:
      assert(0);
      return NV_CC_FL;
   }
}

static uint
translate_opcode(uint opcode)
{
   switch (opcode) {
   case TGSI_OPCODE_ABS: return NV_OP_ABS;
   case TGSI_OPCODE_ADD:
   case TGSI_OPCODE_SUB:
   case TGSI_OPCODE_UADD: return NV_OP_ADD;
   case TGSI_OPCODE_AND: return NV_OP_AND;
   case TGSI_OPCODE_EX2: return NV_OP_EX2;
   case TGSI_OPCODE_CEIL: return NV_OP_CEIL;
   case TGSI_OPCODE_FLR: return NV_OP_FLOOR;
   case TGSI_OPCODE_TRUNC: return NV_OP_TRUNC;
   case TGSI_OPCODE_DDX: return NV_OP_DFDX;
   case TGSI_OPCODE_DDY: return NV_OP_DFDY;
   case TGSI_OPCODE_F2I:
   case TGSI_OPCODE_F2U:
   case TGSI_OPCODE_I2F:
   case TGSI_OPCODE_U2F: return NV_OP_CVT;
   case TGSI_OPCODE_INEG: return NV_OP_NEG;
   case TGSI_OPCODE_LG2: return NV_OP_LG2;
   case TGSI_OPCODE_ISHR:
   case TGSI_OPCODE_USHR: return NV_OP_SHR;
   case TGSI_OPCODE_MAD:
   case TGSI_OPCODE_UMAD: return NV_OP_MAD;
   case TGSI_OPCODE_MAX:
   case TGSI_OPCODE_IMAX:
   case TGSI_OPCODE_UMAX: return NV_OP_MAX;
   case TGSI_OPCODE_MIN:
   case TGSI_OPCODE_IMIN:
   case TGSI_OPCODE_UMIN: return NV_OP_MIN;
   case TGSI_OPCODE_MUL:
   case TGSI_OPCODE_UMUL: return NV_OP_MUL;
   case TGSI_OPCODE_OR: return NV_OP_OR;
   case TGSI_OPCODE_RCP: return NV_OP_RCP;
   case TGSI_OPCODE_RSQ: return NV_OP_RSQ;
   case TGSI_OPCODE_SAD: return NV_OP_SAD;
   case TGSI_OPCODE_SHL: return NV_OP_SHL;
   case TGSI_OPCODE_SLT:
   case TGSI_OPCODE_SGE:
   case TGSI_OPCODE_SEQ:
   case TGSI_OPCODE_SGT:
   case TGSI_OPCODE_SLE:
   case TGSI_OPCODE_SNE:
   case TGSI_OPCODE_ISLT:
   case TGSI_OPCODE_ISGE:
   case TGSI_OPCODE_USEQ:
   case TGSI_OPCODE_USGE:
   case TGSI_OPCODE_USLT:
   case TGSI_OPCODE_USNE: return NV_OP_SET;
   case TGSI_OPCODE_TEX: return NV_OP_TEX;
   case TGSI_OPCODE_TXP: return NV_OP_TEX;
   case TGSI_OPCODE_TXB: return NV_OP_TXB;
   case TGSI_OPCODE_TXL: return NV_OP_TXL;
   case TGSI_OPCODE_XOR: return NV_OP_XOR;
   default:
      return NV_OP_NOP;
   }
}

static ubyte
infer_src_type(unsigned opcode)
{
   switch (opcode) {
   case TGSI_OPCODE_MOV:
   case TGSI_OPCODE_AND:
   case TGSI_OPCODE_OR:
   case TGSI_OPCODE_XOR:
   case TGSI_OPCODE_SAD:
   case TGSI_OPCODE_U2F:
   case TGSI_OPCODE_UADD:
   case TGSI_OPCODE_UDIV:
   case TGSI_OPCODE_UMOD:
   case TGSI_OPCODE_UMAD:
   case TGSI_OPCODE_UMUL:
   case TGSI_OPCODE_UMAX:
   case TGSI_OPCODE_UMIN:
   case TGSI_OPCODE_USEQ:
   case TGSI_OPCODE_USGE:
   case TGSI_OPCODE_USLT:
   case TGSI_OPCODE_USNE:
   case TGSI_OPCODE_USHR:
      return NV_TYPE_U32;
   case TGSI_OPCODE_I2F:
   case TGSI_OPCODE_IDIV:
   case TGSI_OPCODE_IMAX:
   case TGSI_OPCODE_IMIN:
   case TGSI_OPCODE_INEG:
   case TGSI_OPCODE_ISGE:
   case TGSI_OPCODE_ISHR:
   case TGSI_OPCODE_ISLT:
      return NV_TYPE_S32;
   default:
      return NV_TYPE_F32;
   }
}

static ubyte
infer_dst_type(unsigned opcode)
{
   switch (opcode) {
   case TGSI_OPCODE_MOV:
   case TGSI_OPCODE_F2U:
   case TGSI_OPCODE_AND:
   case TGSI_OPCODE_OR:
   case TGSI_OPCODE_XOR:
   case TGSI_OPCODE_SAD:
   case TGSI_OPCODE_UADD:
   case TGSI_OPCODE_UDIV:
   case TGSI_OPCODE_UMOD:
   case TGSI_OPCODE_UMAD:
   case TGSI_OPCODE_UMUL:
   case TGSI_OPCODE_UMAX:
   case TGSI_OPCODE_UMIN:
   case TGSI_OPCODE_USEQ:
   case TGSI_OPCODE_USGE:
   case TGSI_OPCODE_USLT:
   case TGSI_OPCODE_USNE:
   case TGSI_OPCODE_USHR:
      return NV_TYPE_U32;
   case TGSI_OPCODE_F2I:
   case TGSI_OPCODE_IDIV:
   case TGSI_OPCODE_IMAX:
   case TGSI_OPCODE_IMIN:
   case TGSI_OPCODE_INEG:
   case TGSI_OPCODE_ISGE:
   case TGSI_OPCODE_ISHR:
   case TGSI_OPCODE_ISLT:
      return NV_TYPE_S32;
   default:
      return NV_TYPE_F32;
   }
}

static void
emit_store(struct bld_context *bld, const struct tgsi_full_instruction *inst,
	   unsigned chan, struct nv_value *value)
{
   const struct tgsi_full_dst_register *reg = &inst->Dst[0];

   assert(chan < 4);

   if (inst->Instruction.Opcode != TGSI_OPCODE_MOV)
      value->reg.type = infer_dst_type(inst->Instruction.Opcode);

   switch (inst->Instruction.Saturate) {
   case TGSI_SAT_NONE:
      break;
   case TGSI_SAT_ZERO_ONE:
      BLD_INSN_1_EX(value, SAT, F32, value, F32);
      break;
   case TGSI_SAT_MINUS_PLUS_ONE:
      value = bld_insn_2(bld, NV_OP_MAX, value, bld_load_imm_f32(bld, -1.0f));
      value = bld_insn_2(bld, NV_OP_MIN, value, bld_load_imm_f32(bld, 1.0f));
      value->reg.type = NV_TYPE_F32;
      break;
   }

   switch (reg->Register.File) {
   case TGSI_FILE_OUTPUT:
      value = bld_insn_1(bld, NV_OP_MOV, value);
      value->reg.file = bld->ti->output_file;

      if (bld->ti->p->type == PIPE_SHADER_FRAGMENT) {
         STORE_OUTR(reg->Register.Index, chan, value);
      } else {
         value->insn->fixed = 1;
         value->reg.id = bld->ti->output_map[reg->Register.Index][chan];
      }
      break;
   case TGSI_FILE_TEMPORARY:
      assert(reg->Register.Index < BLD_MAX_TEMPS);
      value->reg.file = NV_FILE_GPR;
      if (value->insn->bb != bld->pc->current_block)
         value = bld_insn_1(bld, NV_OP_MOV, value);
      STORE_TEMP(reg->Register.Index, chan, value);
      break;
   case TGSI_FILE_ADDRESS:
      assert(reg->Register.Index < BLD_MAX_ADDRS);
      value->reg.file = NV_FILE_ADDR;
      STORE_ADDR(reg->Register.Index, chan, value);
      break;
   }
}

static INLINE uint32_t
bld_is_output_written(struct bld_context *bld, int i, int c)
{
   if (c < 0)
      return bld->outputs_written[i / 8] & (0xf << ((i * 4) % 32));
   return bld->outputs_written[i / 8] & (1 << ((i * 4 + c) % 32));
}

static void
bld_export_outputs(struct bld_context *bld)
{
   struct nv_value *vals[4];
   struct nv_instruction *nvi;
   int i, c, n;

   bld_push_values(&bld->ovs[0][0], PIPE_MAX_SHADER_OUTPUTS);

   for (i = 0; i < PIPE_MAX_SHADER_OUTPUTS; ++i) {
      if (!bld_is_output_written(bld, i, -1))
         continue;
      for (n = 0, c = 0; c < 4; ++c) {
         if (!bld_is_output_written(bld, i, c))
            continue;
         vals[n] = bld_fetch_global(bld, &bld->ovs[i][c]);
         assert(vals[n]);
         vals[n] = bld_insn_1(bld, NV_OP_MOV, vals[n]);
         vals[n++]->reg.id = bld->ti->output_map[i][c];
      }
      assert(n);

      (nvi = new_instruction(bld->pc, NV_OP_EXPORT))->fixed = 1;

      for (c = 0; c < n; ++c)
         nvi->src[c] = new_ref(bld->pc, vals[c]);
   }
}

static void
bld_new_block(struct bld_context *bld, struct nv_basic_block *b)
{
   int i;

   bld_push_values(&bld->tvs[0][0], BLD_MAX_TEMPS);
   bld_push_values(&bld->avs[0][0], BLD_MAX_ADDRS);
   bld_push_values(&bld->pvs[0][0], BLD_MAX_PREDS);
   bld_push_values(&bld->ovs[0][0], PIPE_MAX_SHADER_OUTPUTS);

   bld->pc->current_block = b;

   for (i = 0; i < 4; ++i)
      bld->saved_addr[i][0] = NULL;
}

static struct nv_value *
bld_saved_input(struct bld_context *bld, unsigned i, unsigned c)
{
   unsigned idx = bld->ti->input_map[i][c];

   if (bld->ti->p->type != PIPE_SHADER_FRAGMENT)
      return NULL;
   if (bld->saved_inputs[idx])
      return bld->saved_inputs[idx];
   return NULL;
}

static struct nv_value *
bld_interpolate(struct bld_context *bld, unsigned mode, struct nv_value *val)
{
   if (mode & (NV50_INTERP_LINEAR | NV50_INTERP_FLAT))
      val = bld_insn_1(bld, NV_OP_LINTERP, val);
   else
      val = bld_insn_2(bld, NV_OP_PINTERP, val, bld->frgcrd[3]);

   val->insn->flat = (mode & NV50_INTERP_FLAT) ? 1 : 0;
   val->insn->centroid = (mode & NV50_INTERP_CENTROID) ? 1 : 0;
   return val;
}

static struct nv_value *
emit_fetch(struct bld_context *bld, const struct tgsi_full_instruction *insn,
           const unsigned s, const unsigned chan)
{
   const struct tgsi_full_src_register *src = &insn->Src[s];
   struct nv_value *res;
   unsigned idx, swz, dim_idx, ind_idx, ind_swz;
   ubyte type = infer_src_type(insn->Instruction.Opcode);

   idx = src->Register.Index;
   swz = tgsi_util_get_full_src_register_swizzle(src, chan);
   dim_idx = -1;
   ind_idx = -1;
   ind_swz = 0;

   if (src->Register.Indirect) {
      ind_idx = src->Indirect.Index;
      ind_swz = tgsi_util_get_src_register_swizzle(&src->Indirect, 0);
   }

   switch (src->Register.File) {
   case TGSI_FILE_CONSTANT:
      dim_idx = src->Dimension.Index ? src->Dimension.Index + 2 : 1;
      assert(dim_idx < 14);
      assert(dim_idx == 1); /* for now */

      res = new_value(bld->pc, NV_FILE_MEM_C(dim_idx), type);
      res->reg.type = type;
      res->reg.id = (idx * 4 + swz) & 127;
      res = bld_insn_1(bld, NV_OP_LDA, res);

      if (src->Register.Indirect)
         res->insn->src[4] = new_ref(bld->pc, FETCH_ADDR(ind_idx, ind_swz));
      if (idx >= (128 / 4))
         res->insn->src[4] =
            new_ref(bld->pc, bld_get_address(bld, (idx * 16) & ~0x1ff, NULL));
      break;
   case TGSI_FILE_IMMEDIATE:
      assert(idx < bld->ti->immd32_nr);
      res = bld_load_imm_u32(bld, bld->ti->immd32[idx * 4 + swz]);
      res->reg.type = type;
      break;
   case TGSI_FILE_INPUT:
      res = bld_saved_input(bld, idx, swz);
      if (res && (insn->Instruction.Opcode != TGSI_OPCODE_TXP))
         return res;

      res = new_value(bld->pc, bld->ti->input_file, type);
      res->reg.id = bld->ti->input_map[idx][swz];

      if (res->reg.file == NV_FILE_MEM_V) {
         res = bld_interpolate(bld, bld->ti->interp_mode[idx], res);
      } else {
         assert(src->Dimension.Dimension == 0);
         res = bld_insn_1(bld, NV_OP_LDA, res);
      }
      assert(res->reg.type == type);

      bld->saved_inputs[bld->ti->input_map[idx][swz]] = res;
      break;
   case TGSI_FILE_TEMPORARY:
      /* this should be load from l[], with reload elimination later on */
      res = bld_fetch_global(bld, &bld->tvs[idx][swz]);
      break;
   case TGSI_FILE_ADDRESS:
      res = bld_fetch_global(bld, &bld->avs[idx][swz]);
      break;
   case TGSI_FILE_PREDICATE:
      res = bld_fetch_global(bld, &bld->pvs[idx][swz]);
      break;
   default:
      NOUVEAU_ERR("illegal/unhandled src reg file: %d\n", src->Register.File);
      abort();
      break;	   
   }
   if (!res) {
      debug_printf("WARNING: undefined source value in TGSI instruction\n");
      return bld_load_imm_u32(bld, 0);
   }

   switch (tgsi_util_get_full_src_register_sign_mode(src, chan)) {
   case TGSI_UTIL_SIGN_KEEP:
      break;
   case TGSI_UTIL_SIGN_CLEAR:
      res = bld_insn_1(bld, NV_OP_ABS, res);
      break;
   case TGSI_UTIL_SIGN_TOGGLE:
      res = bld_insn_1(bld, NV_OP_NEG, res);
      break;
   case TGSI_UTIL_SIGN_SET:
      res = bld_insn_1(bld, NV_OP_ABS, res);
      res = bld_insn_1(bld, NV_OP_NEG, res);
      break;
   default:
      NOUVEAU_ERR("illegal/unhandled src reg sign mode\n");
      abort();
      break;
   }

   return res;
}

static void
bld_lit(struct bld_context *bld, struct nv_value *dst0[4],
        const struct tgsi_full_instruction *insn)
{
   struct nv_value *val0, *zero;
   unsigned mask = insn->Dst[0].Register.WriteMask;

   if (mask & ((1 << 0) | (1 << 3)))
      dst0[3] = dst0[0] = bld_load_imm_f32(bld, 1.0f);

   if (mask & (3 << 1)) {
      zero = bld_load_imm_f32(bld, 0.0f);
      val0 = bld_insn_2(bld, NV_OP_MAX, emit_fetch(bld, insn, 0, 0), zero);

      if (mask & (1 << 1))
         dst0[1] = val0;
   }

   if (mask & (1 << 2)) {
      struct nv_value *val1, *val3, *src1, *src3;
      struct nv_value *pos128 = bld_load_imm_f32(bld, 127.999999f);
      struct nv_value *neg128 = bld_load_imm_f32(bld, -127.999999f);

      src1 = emit_fetch(bld, insn, 0, 1);
      src3 = emit_fetch(bld, insn, 0, 3);

      val0->insn->flags_def = new_value(bld->pc, NV_FILE_FLAGS, NV_TYPE_U16);
      val0->insn->flags_def->insn = val0->insn;

      val1 = bld_insn_2(bld, NV_OP_MAX, src1, zero);
      val3 = bld_insn_2(bld, NV_OP_MAX, src3, neg128);
      val3 = bld_insn_2(bld, NV_OP_MIN, val3, pos128);
      val3 = bld_pow(bld, val1, val3);

      dst0[2] = bld_insn_1(bld, NV_OP_MOV, zero);
      dst0[2]->insn->cc = NV_CC_LE;
      dst0[2]->insn->flags_src = new_ref(bld->pc, val0->insn->flags_def);

      dst0[2] = bld_insn_2(bld, NV_OP_SELECT, val3, dst0[2]);
   }
}

static INLINE void
get_tex_dim(const struct tgsi_full_instruction *insn, int *dim, int *arg)
{
   switch (insn->Texture.Texture) {
   case TGSI_TEXTURE_1D:
      *arg = *dim = 1;
      break;
   case TGSI_TEXTURE_SHADOW1D:
      *dim = 1;
      *arg = 2;
      break;
   case TGSI_TEXTURE_UNKNOWN:
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
      *arg = *dim = 2;
      break;
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWRECT:
      *dim = 2;
      *arg = 3;
      break;
   case TGSI_TEXTURE_3D:
   case TGSI_TEXTURE_CUBE:
      *dim = *arg = 3;
      break;
   default:
      assert(0);
      break;
   }
}

static void
load_proj_tex_coords(struct bld_context *bld,
		     struct nv_value *t[4], int dim,
		     const struct tgsi_full_instruction *insn)
{
   int c, mask = 0;

   t[3] = emit_fetch(bld, insn, 0, 3);

   if (t[3]->insn->opcode == NV_OP_PINTERP) {
      t[3]->insn->opcode = NV_OP_LINTERP;
      nv_reference(bld->pc, &t[3]->insn->src[1], NULL);
   }

   t[3] = bld_insn_1(bld, NV_OP_RCP, t[3]);

   for (c = 0; c < dim; ++c) {
      t[c] = emit_fetch(bld, insn, 0, c);
      if (t[c]->insn->opcode == NV_OP_LINTERP)
         t[c]->insn->opcode = NV_OP_PINTERP;

      if (t[c]->insn->opcode == NV_OP_PINTERP)
         nv_reference(bld->pc, &t[c]->insn->src[1], t[3]);
      else
         mask |= 1 << c;
   }

   for (c = 0; mask; ++c, mask >>= 1) {
      if (!(mask & 1))
         continue;
      t[c] = bld_insn_2(bld, NV_OP_MUL, t[c], t[3]);
   }
}

static void
bld_tex(struct bld_context *bld, struct nv_value *dst0[4],
        const struct tgsi_full_instruction *insn)
{
   struct nv_value *t[4];
   struct nv_instruction *nvi;
   uint opcode = translate_opcode(insn->Instruction.Opcode);
   int arg, dim, c;

   get_tex_dim(insn, &dim, &arg);

   if (insn->Texture.Texture == TGSI_TEXTURE_CUBE) {
   }
   // else
   if (insn->Instruction.Opcode == TGSI_OPCODE_TXP) {
      load_proj_tex_coords(bld, t, dim, insn);
   } else
      for (c = 0; c < dim; ++c)
         t[c] = emit_fetch(bld, insn, 0, c);

   if (arg != dim)
      t[dim] = emit_fetch(bld, insn, 0, 2);

   if (insn->Instruction.Opcode == TGSI_OPCODE_TXB ||
       insn->Instruction.Opcode == TGSI_OPCODE_TXL) {
      t[arg++] = emit_fetch(bld, insn, 0, 3);
   }

   for (c = 0; c < arg; ++c) {
      t[c] = bld_insn_1(bld, NV_OP_MOV, t[c]);
      t[c]->reg.type = NV_TYPE_F32;
   }

   nvi = new_instruction(bld->pc, opcode);

   for (c = 0; c < 4; ++c) {
      nvi->def[c] = dst0[c] = new_value(bld->pc, NV_FILE_GPR, NV_TYPE_F32);
      nvi->def[c]->insn = nvi;
   }
   for (c = 0; c < arg; ++c)
      nvi->src[c] = new_ref(bld->pc, t[c]);

   nvi->tex_t = insn->Src[1].Register.Index;
   nvi->tex_s = 0;
   nvi->tex_mask = 0xf;
   nvi->tex_cube = (insn->Texture.Texture == TGSI_TEXTURE_CUBE) ? 1 : 0;
   nvi->tex_live = 0;
   nvi->tex_argc = arg;
}

#define FOR_EACH_DST0_ENABLED_CHANNEL(chan, inst) \
   for (chan = 0; chan < 4; ++chan)               \
      if ((inst)->Dst[0].Register.WriteMask & (1 << chan))

static void
bld_instruction(struct bld_context *bld,
                const struct tgsi_full_instruction *insn)
{
   struct nv_value *src0;
   struct nv_value *src1;
   struct nv_value *src2;
   struct nv_value *dst0[4];
   struct nv_value *temp;
   int c;
   uint opcode = translate_opcode(insn->Instruction.Opcode);

   tgsi_dump_instruction(insn, 1);
	
   switch (insn->Instruction.Opcode) {
   case TGSI_OPCODE_ADD:
   case TGSI_OPCODE_MAX:
   case TGSI_OPCODE_MIN:
   case TGSI_OPCODE_MUL:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         src0 = emit_fetch(bld, insn, 0, c);
         src1 = emit_fetch(bld, insn, 1, c);
         dst0[c] = bld_insn_2(bld, opcode, src0, src1);
      }
      break;
   case TGSI_OPCODE_CMP:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         src0 = emit_fetch(bld, insn, 0, c);
         src1 = emit_fetch(bld, insn, 1, c);
         src2 = emit_fetch(bld, insn, 2, c);
         src0 = bld_predicate(bld, src0);

         src1 = bld_insn_1(bld, NV_OP_MOV, src1);
         src1->insn->flags_src = new_ref(bld->pc, src0);
         src1->insn->cc = NV_CC_LT;

         src2 = bld_insn_1(bld, NV_OP_MOV, src2);
         src2->insn->flags_src = new_ref(bld->pc, src0);
         src2->insn->cc = NV_CC_GE;

         dst0[c] = bld_insn_2(bld, NV_OP_SELECT, src1, src2);
      }
      break;
   case TGSI_OPCODE_COS:
      src0 = emit_fetch(bld, insn, 0, 0);
      temp = bld_insn_1(bld, NV_OP_PRESIN, src0);
      if (insn->Dst[0].Register.WriteMask & 7)
         temp = bld_insn_1(bld, NV_OP_COS, temp);
      for (c = 0; c < 3; ++c)
         if (insn->Dst[0].Register.WriteMask & (1 << c))
            dst0[c] = temp;
      if (!(insn->Dst[0].Register.WriteMask & (1 << 3)))
         break;
      /* XXX: if src0.x is src0.w, don't emit new insns */
      src0 = emit_fetch(bld, insn, 0, 3);
      temp = bld_insn_1(bld, NV_OP_PRESIN, src0);
      dst0[3] = bld_insn_1(bld, NV_OP_COS, temp);
      break;
   case TGSI_OPCODE_DP3:
      src0 = emit_fetch(bld, insn, 0, 0);
      src1 = emit_fetch(bld, insn, 1, 0);
      temp = bld_insn_2(bld, NV_OP_MUL, src0, src1);
      for (c = 1; c < 3; ++c) {
         src0 = emit_fetch(bld, insn, 0, c);
         src1 = emit_fetch(bld, insn, 1, c);
         temp = bld_insn_3(bld, NV_OP_MAD, src0, src1, temp);
      }
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn)
         dst0[c] = temp;
      break;
   case TGSI_OPCODE_DP4:
      src0 = emit_fetch(bld, insn, 0, 0);
      src1 = emit_fetch(bld, insn, 1, 0);
      temp = bld_insn_2(bld, NV_OP_MUL, src0, src1);
      for (c = 1; c < 4; ++c) {
         src0 = emit_fetch(bld, insn, 0, c);
         src1 = emit_fetch(bld, insn, 1, c);
         temp = bld_insn_3(bld, NV_OP_MAD, src0, src1, temp);
      }
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn)
         dst0[c] = temp;
      break;
   case TGSI_OPCODE_EX2:
      src0 = emit_fetch(bld, insn, 0, 0);
      temp = bld_insn_1(bld, NV_OP_PREEX2, src0);
      temp = bld_insn_1(bld, NV_OP_EX2, temp);
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn)
         dst0[c] = temp;
      break;
   case TGSI_OPCODE_FRC:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         src0 = emit_fetch(bld, insn, 0, c);
         dst0[c] = bld_insn_1(bld, NV_OP_FLOOR, src0);
         dst0[c] = bld_insn_2(bld, NV_OP_SUB, src0, dst0[c]);
      }
      break;
   case TGSI_OPCODE_KIL:
      for (c = 0; c < 4; ++c) {
         src0 = emit_fetch(bld, insn, 0, c);
         bld_kil(bld, src0);
      }
      break;
   case TGSI_OPCODE_IF:
   {
      struct nv_basic_block *b = new_basic_block(bld->pc);

      nvbb_attach_block(bld->pc->current_block, b);

      bld->join_bb[bld->cond_lvl] = bld->pc->current_block;
      bld->cond_bb[bld->cond_lvl] = bld->pc->current_block;

      src1 = bld_predicate(bld, emit_fetch(bld, insn, 0, 0));

      bld_flow(bld, NV_OP_BRA, NV_CC_EQ, src1, FALSE);

      ++bld->cond_lvl;
      bld_new_block(bld, b);
   }
      break;
   case TGSI_OPCODE_ELSE:
   {
      struct nv_basic_block *b = new_basic_block(bld->pc);

      --bld->cond_lvl;
      nvbb_attach_block(bld->join_bb[bld->cond_lvl], b);

      bld->cond_bb[bld->cond_lvl]->exit->target = b;
      bld->cond_bb[bld->cond_lvl] = bld->pc->current_block;

      new_instruction(bld->pc, NV_OP_BRA)->is_terminator = 1;

      ++bld->cond_lvl;
      bld_new_block(bld, b);
   }
      break;
   case TGSI_OPCODE_ENDIF: /* XXX: deal with ENDIF; ENDIF; */
   {
      struct nv_basic_block *b = new_basic_block(bld->pc);

      --bld->cond_lvl;
      nvbb_attach_block(bld->pc->current_block, b);
      nvbb_attach_block(bld->cond_bb[bld->cond_lvl], b);

      bld->cond_bb[bld->cond_lvl]->exit->target = b;

      if (0 && bld->join_bb[bld->cond_lvl]) {
         bld->join_bb[bld->cond_lvl]->exit->prev->target = b;

         new_instruction(bld->pc, NV_OP_NOP)->is_join = TRUE;
      }

      bld_new_block(bld, b);
   }
      break;
   case TGSI_OPCODE_BGNLOOP:
      assert(0);
      break;
   case TGSI_OPCODE_BRK:
      assert(0);
      break;
   case TGSI_OPCODE_CONT:
      assert(0);
      break;
   case TGSI_OPCODE_ENDLOOP:
      assert(0);
      break;
   case TGSI_OPCODE_ABS:
   case TGSI_OPCODE_CEIL:
   case TGSI_OPCODE_FLR:
   case TGSI_OPCODE_TRUNC:
   case TGSI_OPCODE_DDX:
   case TGSI_OPCODE_DDY:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         src0 = emit_fetch(bld, insn, 0, c);
         dst0[c] = bld_insn_1(bld, opcode, src0);
      }	   
      break;
   case TGSI_OPCODE_LIT:
      bld_lit(bld, dst0, insn);
      break;
   case TGSI_OPCODE_LRP:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         src0 = emit_fetch(bld, insn, 0, c);
         src1 = emit_fetch(bld, insn, 1, c);
         src2 = emit_fetch(bld, insn, 2, c);
         dst0[c] = bld_insn_2(bld, NV_OP_SUB, src1, src2);
         dst0[c] = bld_insn_3(bld, NV_OP_MAD, dst0[c], src0, src2);
      }
      break;
   case TGSI_OPCODE_MOV:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn)
         dst0[c] = emit_fetch(bld, insn, 0, c);
      break;
   case TGSI_OPCODE_MAD:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         src0 = emit_fetch(bld, insn, 0, c);
         src1 = emit_fetch(bld, insn, 1, c);
         src2 = emit_fetch(bld, insn, 2, c);
         dst0[c] = bld_insn_3(bld, opcode, src0, src1, src2);
      }
      break;
   case TGSI_OPCODE_POW:
      src0 = emit_fetch(bld, insn, 0, 0);
      src1 = emit_fetch(bld, insn, 1, 0);
      temp = bld_pow(bld, src0, src1);
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn)
         dst0[c] = temp;
      break;
   case TGSI_OPCODE_RCP:
   case TGSI_OPCODE_LG2:
      src0 = emit_fetch(bld, insn, 0, 0);
      temp = bld_insn_1(bld, opcode, src0);
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn)
         dst0[c] = temp;
      break;
   case TGSI_OPCODE_RSQ:
      src0 = emit_fetch(bld, insn, 0, 0);
      temp = bld_insn_1(bld, NV_OP_ABS, src0);
      temp = bld_insn_1(bld, NV_OP_RSQ, temp);
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn)
         dst0[c] = temp;
      break;
   case TGSI_OPCODE_SLT:
   case TGSI_OPCODE_SGE:
   case TGSI_OPCODE_SEQ:
   case TGSI_OPCODE_SGT:
   case TGSI_OPCODE_SLE:
   case TGSI_OPCODE_SNE:
   case TGSI_OPCODE_ISLT:
   case TGSI_OPCODE_ISGE:
   case TGSI_OPCODE_USEQ:
   case TGSI_OPCODE_USGE:
   case TGSI_OPCODE_USLT:
   case TGSI_OPCODE_USNE:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         src0 = emit_fetch(bld, insn, 0, c);
         src1 = emit_fetch(bld, insn, 1, c);
         dst0[c] = bld_insn_2(bld, NV_OP_SET, src0, src1);
         dst0[c]->insn->set_cond = translate_setcc(insn->Instruction.Opcode);
         dst0[c]->reg.type = infer_dst_type(insn->Instruction.Opcode);

         dst0[c]->insn->src[0]->typecast =
         dst0[c]->insn->src[1]->typecast =
            infer_src_type(insn->Instruction.Opcode);

         if (dst0[c]->reg.type != NV_TYPE_F32)
            break;
         dst0[c] = bld_insn_1(bld, NV_OP_ABS, dst0[c]);
         dst0[c]->insn->src[0]->typecast = NV_TYPE_S32;
         dst0[c]->reg.type = NV_TYPE_S32;
         dst0[c] = bld_insn_1(bld, NV_OP_CVT, dst0[c]);
         dst0[c]->reg.type = NV_TYPE_F32;
      }
      break;
   case TGSI_OPCODE_SUB:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         src0 = emit_fetch(bld, insn, 0, c);
         src1 = emit_fetch(bld, insn, 1, c);
         dst0[c] = bld_insn_2(bld, NV_OP_ADD, src0, src1);
         dst0[c]->insn->src[1]->mod ^= NV_MOD_NEG;
      }
      break;
   case TGSI_OPCODE_TEX:
   case TGSI_OPCODE_TXB:
   case TGSI_OPCODE_TXL:
   case TGSI_OPCODE_TXP:
      bld_tex(bld, dst0, insn);
      break;
   case TGSI_OPCODE_XPD:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         if (c == 3) {
            dst0[3] = bld_imm_f32(bld, 1.0f);
            break;
         }
         src0 = emit_fetch(bld, insn, 0, (c + 1) % 3);
         src1 = emit_fetch(bld, insn, 1, (c + 2) % 3);
         dst0[c] = bld_insn_2(bld, NV_OP_MUL, src0, src1);

         src0 = emit_fetch(bld, insn, 0, (c + 2) % 3);
         src1 = emit_fetch(bld, insn, 1, (c + 1) % 3);
         dst0[c] = bld_insn_3(bld, NV_OP_MAD, src0, src1, dst0[c]);

         dst0[c]->insn->src[2]->mod ^= NV_MOD_NEG;
      }
      break;
   case TGSI_OPCODE_END:
      if (bld->ti->p->type == PIPE_SHADER_FRAGMENT)
         bld_export_outputs(bld);
      break;
   default:
      NOUVEAU_ERR("nv_bld: unhandled opcode %u\n", insn->Instruction.Opcode);
      abort();
      break;
   }

   FOR_EACH_DST0_ENABLED_CHANNEL(c, insn)
      emit_store(bld, insn, c, dst0[c]);
}

int
nv50_tgsi_to_nc(struct nv_pc *pc, struct nv50_translation_info *ti)
{
   struct bld_context *bld = CALLOC_STRUCT(bld_context);
   int c;

   pc->root = pc->current_block = new_basic_block(pc);

   bld->pc = pc;
   bld->ti = ti;

   pc->loop_nesting_bound = 1; /* XXX: should work with 0 */

   c = util_bitcount(bld->ti->p->fp.interp >> 24);
   if (c && ti->p->type == PIPE_SHADER_FRAGMENT) {
      bld->frgcrd[3] = new_value(pc, NV_FILE_MEM_V, NV_TYPE_F32);
      bld->frgcrd[3]->reg.id = c - 1;
      bld->frgcrd[3] = bld_insn_1(bld, NV_OP_LINTERP, bld->frgcrd[3]);
      bld->frgcrd[3] = bld_insn_1(bld, NV_OP_RCP, bld->frgcrd[3]);
   }

   tgsi_parse_init(&bld->parse[0], ti->p->pipe.tokens);

   while (!tgsi_parse_end_of_tokens(&bld->parse[bld->call_lvl])) {
      const union tgsi_full_token *tok = &bld->parse[bld->call_lvl].FullToken;

      tgsi_parse_token(&bld->parse[bld->call_lvl]);

      switch (tok->Token.Type) {
      case TGSI_TOKEN_TYPE_INSTRUCTION:
         bld_instruction(bld, &tok->FullInstruction);
         break;
      default:
         break;
      }
   }

   FREE(bld);
   return 0;
}

#if 0
/* If a variable is assigned in a loop, replace all references to the value
 * from outside the loop with a phi value.
 */
static void
bld_adjust_nv_refs(struct nv_pc *pc, struct nv_basic_block *b,
                   struct nv_value *old_val,
                   struct nv_value *new_val)
{
   struct nv_instruction *nvi;

   for (nvi = b->entry; nvi; nvi = nvi->next) {
      int s;
      for (s = 0; s < 5; ++s) {
         if (!nvi->src[s])
            continue;
         if (nvi->src[s]->value == old_val)
            nv_reference(pc, &nvi->src[s], new_val);
      }
      if (nvi->flags_src && nvi->flags_src->value == old_val)
         nv_reference(pc, &nvi->flags_src, new_val);
   }
   b->pass_seq = pc->pass_seq;

   if (b->out[0] && b->out[0]->pass_seq < pc->pass_seq)
      bld_adjust_nv_refs(pc, b, old_val, new_val);

   if (b->out[1] && b->out[1]->pass_seq < pc->pass_seq)
      bld_adjust_nv_refs(pc, b, old_val, new_val);
}
#endif

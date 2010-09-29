/*
 * Copyright © 2010 Intel Corporation
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
 *    Eric Anholt <eric@anholt.net>
 *
 */

extern "C" {

#include <sys/types.h>

#include "main/macros.h"
#include "main/shaderobj.h"
#include "main/uniforms.h"
#include "program/prog_parameter.h"
#include "program/prog_print.h"
#include "program/prog_optimize.h"
#include "program/register_allocate.h"
#include "program/sampler.h"
#include "program/hash_table.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_wm.h"
#include "talloc.h"
}
#include "../glsl/glsl_types.h"
#include "../glsl/ir_optimization.h"
#include "../glsl/ir_print_visitor.h"

enum register_file {
   ARF = BRW_ARCHITECTURE_REGISTER_FILE,
   GRF = BRW_GENERAL_REGISTER_FILE,
   MRF = BRW_MESSAGE_REGISTER_FILE,
   IMM = BRW_IMMEDIATE_VALUE,
   FIXED_HW_REG, /* a struct brw_reg */
   UNIFORM, /* prog_data->params[hw_reg] */
   BAD_FILE
};

enum fs_opcodes {
   FS_OPCODE_FB_WRITE = 256,
   FS_OPCODE_RCP,
   FS_OPCODE_RSQ,
   FS_OPCODE_SQRT,
   FS_OPCODE_EXP2,
   FS_OPCODE_LOG2,
   FS_OPCODE_POW,
   FS_OPCODE_SIN,
   FS_OPCODE_COS,
   FS_OPCODE_DDX,
   FS_OPCODE_DDY,
   FS_OPCODE_LINTERP,
   FS_OPCODE_TEX,
   FS_OPCODE_TXB,
   FS_OPCODE_TXL,
   FS_OPCODE_DISCARD,
};

static int using_new_fs = -1;
static struct brw_reg brw_reg_from_fs_reg(class fs_reg *reg);

struct gl_shader *
brw_new_shader(GLcontext *ctx, GLuint name, GLuint type)
{
   struct brw_shader *shader;

   shader = talloc_zero(NULL, struct brw_shader);
   if (shader) {
      shader->base.Type = type;
      shader->base.Name = name;
      _mesa_init_shader(ctx, &shader->base);
   }

   return &shader->base;
}

struct gl_shader_program *
brw_new_shader_program(GLcontext *ctx, GLuint name)
{
   struct brw_shader_program *prog;
   prog = talloc_zero(NULL, struct brw_shader_program);
   if (prog) {
      prog->base.Name = name;
      _mesa_init_shader_program(ctx, &prog->base);
   }
   return &prog->base;
}

GLboolean
brw_compile_shader(GLcontext *ctx, struct gl_shader *shader)
{
   if (!_mesa_ir_compile_shader(ctx, shader))
      return GL_FALSE;

   return GL_TRUE;
}

GLboolean
brw_link_shader(GLcontext *ctx, struct gl_shader_program *prog)
{
   if (using_new_fs == -1)
      using_new_fs = getenv("INTEL_NEW_FS") != NULL;

   for (unsigned i = 0; i < prog->_NumLinkedShaders; i++) {
      struct brw_shader *shader = (struct brw_shader *)prog->_LinkedShaders[i];

      if (using_new_fs && shader->base.Type == GL_FRAGMENT_SHADER) {
	 void *mem_ctx = talloc_new(NULL);
	 bool progress;

	 if (shader->ir)
	    talloc_free(shader->ir);
	 shader->ir = new(shader) exec_list;
	 clone_ir_list(mem_ctx, shader->ir, shader->base.ir);

	 do_mat_op_to_vec(shader->ir);
	 do_mod_to_fract(shader->ir);
	 do_div_to_mul_rcp(shader->ir);
	 do_sub_to_add_neg(shader->ir);
	 do_explog_to_explog2(shader->ir);

	 do {
	    progress = false;

	    brw_do_channel_expressions(shader->ir);
	    brw_do_vector_splitting(shader->ir);

	    progress = do_lower_jumps(shader->ir, true, true,
				      true, /* main return */
				      false, /* continue */
				      false /* loops */
				      ) || progress;

	    progress = do_common_optimization(shader->ir, true, 32) || progress;

	    progress = lower_noise(shader->ir) || progress;
	    progress =
	       lower_variable_index_to_cond_assign(shader->ir,
						   GL_TRUE, /* input */
						   GL_TRUE, /* output */
						   GL_TRUE, /* temp */
						   GL_TRUE /* uniform */
						   ) || progress;
	 } while (progress);

	 validate_ir_tree(shader->ir);

	 reparent_ir(shader->ir, shader->ir);
	 talloc_free(mem_ctx);
      }
   }

   if (!_mesa_ir_link_shader(ctx, prog))
      return GL_FALSE;

   return GL_TRUE;
}

static int
type_size(const struct glsl_type *type)
{
   unsigned int size, i;

   switch (type->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_BOOL:
      return type->components();
   case GLSL_TYPE_ARRAY:
      return type_size(type->fields.array) * type->length;
   case GLSL_TYPE_STRUCT:
      size = 0;
      for (i = 0; i < type->length; i++) {
	 size += type_size(type->fields.structure[i].type);
      }
      return size;
   case GLSL_TYPE_SAMPLER:
      /* Samplers take up no register space, since they're baked in at
       * link time.
       */
      return 0;
   default:
      assert(!"not reached");
      return 0;
   }
}

class fs_reg {
public:
   /* Callers of this talloc-based new need not call delete. It's
    * easier to just talloc_free 'ctx' (or any of its ancestors). */
   static void* operator new(size_t size, void *ctx)
   {
      void *node;

      node = talloc_size(ctx, size);
      assert(node != NULL);

      return node;
   }

   void init()
   {
      this->reg = 0;
      this->reg_offset = 0;
      this->negate = 0;
      this->abs = 0;
      this->hw_reg = -1;
   }

   /** Generic unset register constructor. */
   fs_reg()
   {
      init();
      this->file = BAD_FILE;
   }

   /** Immediate value constructor. */
   fs_reg(float f)
   {
      init();
      this->file = IMM;
      this->type = BRW_REGISTER_TYPE_F;
      this->imm.f = f;
   }

   /** Immediate value constructor. */
   fs_reg(int32_t i)
   {
      init();
      this->file = IMM;
      this->type = BRW_REGISTER_TYPE_D;
      this->imm.i = i;
   }

   /** Immediate value constructor. */
   fs_reg(uint32_t u)
   {
      init();
      this->file = IMM;
      this->type = BRW_REGISTER_TYPE_UD;
      this->imm.u = u;
   }

   /** Fixed brw_reg Immediate value constructor. */
   fs_reg(struct brw_reg fixed_hw_reg)
   {
      init();
      this->file = FIXED_HW_REG;
      this->fixed_hw_reg = fixed_hw_reg;
      this->type = fixed_hw_reg.type;
   }

   fs_reg(enum register_file file, int hw_reg);
   fs_reg(class fs_visitor *v, const struct glsl_type *type);

   /** Register file: ARF, GRF, MRF, IMM. */
   enum register_file file;
   /** virtual register number.  0 = fixed hw reg */
   int reg;
   /** Offset within the virtual register. */
   int reg_offset;
   /** HW register number.  Generally unset until register allocation. */
   int hw_reg;
   /** Register type.  BRW_REGISTER_TYPE_* */
   int type;
   bool negate;
   bool abs;
   struct brw_reg fixed_hw_reg;

   /** Value for file == BRW_IMMMEDIATE_FILE */
   union {
      int32_t i;
      uint32_t u;
      float f;
   } imm;
};

static const fs_reg reg_undef;
static const fs_reg reg_null(ARF, BRW_ARF_NULL);

class fs_inst : public exec_node {
public:
   /* Callers of this talloc-based new need not call delete. It's
    * easier to just talloc_free 'ctx' (or any of its ancestors). */
   static void* operator new(size_t size, void *ctx)
   {
      void *node;

      node = talloc_zero_size(ctx, size);
      assert(node != NULL);

      return node;
   }

   void init()
   {
      this->opcode = BRW_OPCODE_NOP;
      this->saturate = false;
      this->conditional_mod = BRW_CONDITIONAL_NONE;
      this->predicated = false;
      this->sampler = 0;
      this->target = 0;
      this->eot = false;
      this->shadow_compare = false;
   }

   fs_inst()
   {
      init();
   }

   fs_inst(int opcode)
   {
      init();
      this->opcode = opcode;
   }

   fs_inst(int opcode, fs_reg dst, fs_reg src0)
   {
      init();
      this->opcode = opcode;
      this->dst = dst;
      this->src[0] = src0;
   }

   fs_inst(int opcode, fs_reg dst, fs_reg src0, fs_reg src1)
   {
      init();
      this->opcode = opcode;
      this->dst = dst;
      this->src[0] = src0;
      this->src[1] = src1;
   }

   fs_inst(int opcode, fs_reg dst, fs_reg src0, fs_reg src1, fs_reg src2)
   {
      init();
      this->opcode = opcode;
      this->dst = dst;
      this->src[0] = src0;
      this->src[1] = src1;
      this->src[2] = src2;
   }

   int opcode; /* BRW_OPCODE_* or FS_OPCODE_* */
   fs_reg dst;
   fs_reg src[3];
   bool saturate;
   bool predicated;
   int conditional_mod; /**< BRW_CONDITIONAL_* */

   int mlen; /**< SEND message length */
   int sampler;
   int target; /**< MRT target. */
   bool eot;
   bool shadow_compare;

   /** @{
    * Annotation for the generated IR.  One of the two can be set.
    */
   ir_instruction *ir;
   const char *annotation;
   /** @} */
};

class fs_visitor : public ir_visitor
{
public:

   fs_visitor(struct brw_wm_compile *c, struct brw_shader *shader)
   {
      this->c = c;
      this->p = &c->func;
      this->brw = p->brw;
      this->fp = brw->fragment_program;
      this->intel = &brw->intel;
      this->ctx = &intel->ctx;
      this->mem_ctx = talloc_new(NULL);
      this->shader = shader;
      this->fail = false;
      this->variable_ht = hash_table_ctor(0,
					  hash_table_pointer_hash,
					  hash_table_pointer_compare);

      this->frag_color = NULL;
      this->frag_data = NULL;
      this->frag_depth = NULL;
      this->first_non_payload_grf = 0;

      this->current_annotation = NULL;
      this->annotation_string = NULL;
      this->annotation_ir = NULL;
      this->base_ir = NULL;

      this->virtual_grf_sizes = NULL;
      this->virtual_grf_next = 1;
      this->virtual_grf_array_size = 0;
   }
   ~fs_visitor()
   {
      talloc_free(this->mem_ctx);
      hash_table_dtor(this->variable_ht);
   }

   fs_reg *variable_storage(ir_variable *var);
   int virtual_grf_alloc(int size);

   void visit(ir_variable *ir);
   void visit(ir_assignment *ir);
   void visit(ir_dereference_variable *ir);
   void visit(ir_dereference_record *ir);
   void visit(ir_dereference_array *ir);
   void visit(ir_expression *ir);
   void visit(ir_texture *ir);
   void visit(ir_if *ir);
   void visit(ir_constant *ir);
   void visit(ir_swizzle *ir);
   void visit(ir_return *ir);
   void visit(ir_loop *ir);
   void visit(ir_loop_jump *ir);
   void visit(ir_discard *ir);
   void visit(ir_call *ir);
   void visit(ir_function *ir);
   void visit(ir_function_signature *ir);

   fs_inst *emit(fs_inst inst);
   void assign_curb_setup();
   void assign_urb_setup();
   void assign_regs();
   void assign_regs_trivial();
   void generate_code();
   void generate_fb_write(fs_inst *inst);
   void generate_linterp(fs_inst *inst, struct brw_reg dst,
			 struct brw_reg *src);
   void generate_tex(fs_inst *inst, struct brw_reg dst, struct brw_reg src);
   void generate_math(fs_inst *inst, struct brw_reg dst, struct brw_reg *src);
   void generate_discard(fs_inst *inst, struct brw_reg temp);
   void generate_ddx(fs_inst *inst, struct brw_reg dst, struct brw_reg src);
   void generate_ddy(fs_inst *inst, struct brw_reg dst, struct brw_reg src);

   void emit_dummy_fs();
   void emit_fragcoord_interpolation(ir_variable *ir);
   void emit_general_interpolation(ir_variable *ir);
   void emit_interpolation_setup();
   void emit_fb_writes();

   struct brw_reg interp_reg(int location, int channel);
   int setup_uniform_values(int loc, const glsl_type *type);
   void setup_builtin_uniform_values(ir_variable *ir);

   struct brw_context *brw;
   const struct gl_fragment_program *fp;
   struct intel_context *intel;
   GLcontext *ctx;
   struct brw_wm_compile *c;
   struct brw_compile *p;
   struct brw_shader *shader;
   void *mem_ctx;
   exec_list instructions;

   int *virtual_grf_sizes;
   int virtual_grf_next;
   int virtual_grf_array_size;

   struct hash_table *variable_ht;
   ir_variable *frag_color, *frag_data, *frag_depth;
   int first_non_payload_grf;

   /** @{ debug annotation info */
   const char *current_annotation;
   ir_instruction *base_ir;
   const char **annotation_string;
   ir_instruction **annotation_ir;
   /** @} */

   bool fail;

   /* Result of last visit() method. */
   fs_reg result;

   fs_reg pixel_x;
   fs_reg pixel_y;
   fs_reg wpos_w;
   fs_reg pixel_w;
   fs_reg delta_x;
   fs_reg delta_y;

   int grf_used;

};

int
fs_visitor::virtual_grf_alloc(int size)
{
   if (virtual_grf_array_size <= virtual_grf_next) {
      if (virtual_grf_array_size == 0)
	 virtual_grf_array_size = 16;
      else
	 virtual_grf_array_size *= 2;
      virtual_grf_sizes = talloc_realloc(mem_ctx, virtual_grf_sizes,
					 int, virtual_grf_array_size);

      /* This slot is always unused. */
      virtual_grf_sizes[0] = 0;
   }
   virtual_grf_sizes[virtual_grf_next] = size;
   return virtual_grf_next++;
}

/** Fixed HW reg constructor. */
fs_reg::fs_reg(enum register_file file, int hw_reg)
{
   init();
   this->file = file;
   this->hw_reg = hw_reg;
   this->type = BRW_REGISTER_TYPE_F;
}

int
brw_type_for_base_type(const struct glsl_type *type)
{
   switch (type->base_type) {
   case GLSL_TYPE_FLOAT:
      return BRW_REGISTER_TYPE_F;
   case GLSL_TYPE_INT:
   case GLSL_TYPE_BOOL:
      return BRW_REGISTER_TYPE_D;
   case GLSL_TYPE_UINT:
      return BRW_REGISTER_TYPE_UD;
   case GLSL_TYPE_ARRAY:
   case GLSL_TYPE_STRUCT:
      /* These should be overridden with the type of the member when
       * dereferenced into.  BRW_REGISTER_TYPE_UD seems like a likely
       * way to trip up if we don't.
       */
      return BRW_REGISTER_TYPE_UD;
   default:
      assert(!"not reached");
      return BRW_REGISTER_TYPE_F;
   }
}

/** Automatic reg constructor. */
fs_reg::fs_reg(class fs_visitor *v, const struct glsl_type *type)
{
   init();

   this->file = GRF;
   this->reg = v->virtual_grf_alloc(type_size(type));
   this->reg_offset = 0;
   this->type = brw_type_for_base_type(type);
}

fs_reg *
fs_visitor::variable_storage(ir_variable *var)
{
   return (fs_reg *)hash_table_find(this->variable_ht, var);
}

/* Our support for uniforms is piggy-backed on the struct
 * gl_fragment_program, because that's where the values actually
 * get stored, rather than in some global gl_shader_program uniform
 * store.
 */
int
fs_visitor::setup_uniform_values(int loc, const glsl_type *type)
{
   unsigned int offset = 0;
   float *vec_values;

   if (type->is_matrix()) {
      const glsl_type *column = glsl_type::get_instance(GLSL_TYPE_FLOAT,
							type->vector_elements,
							1);

      for (unsigned int i = 0; i < type->matrix_columns; i++) {
	 offset += setup_uniform_values(loc + offset, column);
      }

      return offset;
   }

   switch (type->base_type) {
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_BOOL:
      vec_values = fp->Base.Parameters->ParameterValues[loc];
      for (unsigned int i = 0; i < type->vector_elements; i++) {
	 c->prog_data.param[c->prog_data.nr_params++] = &vec_values[i];
      }
      return 1;

   case GLSL_TYPE_STRUCT:
      for (unsigned int i = 0; i < type->length; i++) {
	 offset += setup_uniform_values(loc + offset,
					type->fields.structure[i].type);
      }
      return offset;

   case GLSL_TYPE_ARRAY:
      for (unsigned int i = 0; i < type->length; i++) {
	 offset += setup_uniform_values(loc + offset, type->fields.array);
      }
      return offset;

   case GLSL_TYPE_SAMPLER:
      /* The sampler takes up a slot, but we don't use any values from it. */
      return 1;

   default:
      assert(!"not reached");
      return 0;
   }
}


/* Our support for builtin uniforms is even scarier than non-builtin.
 * It sits on top of the PROG_STATE_VAR parameters that are
 * automatically updated from GL context state.
 */
void
fs_visitor::setup_builtin_uniform_values(ir_variable *ir)
{
   const struct gl_builtin_uniform_desc *statevar = NULL;

   for (unsigned int i = 0; _mesa_builtin_uniform_desc[i].name; i++) {
      statevar = &_mesa_builtin_uniform_desc[i];
      if (strcmp(ir->name, _mesa_builtin_uniform_desc[i].name) == 0)
	 break;
   }

   if (!statevar->name) {
      this->fail = true;
      printf("Failed to find builtin uniform `%s'\n", ir->name);
      return;
   }

   int array_count;
   if (ir->type->is_array()) {
      array_count = ir->type->length;
   } else {
      array_count = 1;
   }

   for (int a = 0; a < array_count; a++) {
      for (unsigned int i = 0; i < statevar->num_elements; i++) {
	 struct gl_builtin_uniform_element *element = &statevar->elements[i];
	 int tokens[STATE_LENGTH];

	 memcpy(tokens, element->tokens, sizeof(element->tokens));
	 if (ir->type->is_array()) {
	    tokens[1] = a;
	 }

	 /* This state reference has already been setup by ir_to_mesa,
	  * but we'll get the same index back here.
	  */
	 int index = _mesa_add_state_reference(this->fp->Base.Parameters,
					       (gl_state_index *)tokens);
	 float *vec_values = this->fp->Base.Parameters->ParameterValues[index];

	 /* Add each of the unique swizzles of the element as a
	  * parameter.  This'll end up matching the expected layout of
	  * the array/matrix/structure we're trying to fill in.
	  */
	 int last_swiz = -1;
	 for (unsigned int i = 0; i < 4; i++) {
	    int this_swiz = GET_SWZ(element->swizzle, i);
	    if (this_swiz == last_swiz)
	       break;
	    last_swiz = this_swiz;

	    c->prog_data.param[c->prog_data.nr_params++] = &vec_values[i];
	 }
      }
   }
}

void
fs_visitor::emit_fragcoord_interpolation(ir_variable *ir)
{
   fs_reg *reg = new(this->mem_ctx) fs_reg(this, ir->type);
   fs_reg wpos = *reg;
   fs_reg neg_y = this->pixel_y;
   neg_y.negate = true;

   /* gl_FragCoord.x */
   if (ir->pixel_center_integer) {
      emit(fs_inst(BRW_OPCODE_MOV, wpos, this->pixel_x));
   } else {
      emit(fs_inst(BRW_OPCODE_ADD, wpos, this->pixel_x, fs_reg(0.5f)));
   }
   wpos.reg_offset++;

   /* gl_FragCoord.y */
   if (ir->origin_upper_left && ir->pixel_center_integer) {
      emit(fs_inst(BRW_OPCODE_MOV, wpos, this->pixel_y));
   } else {
      fs_reg pixel_y = this->pixel_y;
      float offset = (ir->pixel_center_integer ? 0.0 : 0.5);

      if (!ir->origin_upper_left) {
	 pixel_y.negate = true;
	 offset += c->key.drawable_height - 1.0;
      }

      emit(fs_inst(BRW_OPCODE_ADD, wpos, pixel_y, fs_reg(offset)));
   }
   wpos.reg_offset++;

   /* gl_FragCoord.z */
   emit(fs_inst(FS_OPCODE_LINTERP, wpos, this->delta_x, this->delta_y,
		interp_reg(FRAG_ATTRIB_WPOS, 2)));
   wpos.reg_offset++;

   /* gl_FragCoord.w: Already set up in emit_interpolation */
   emit(fs_inst(BRW_OPCODE_MOV, wpos, this->wpos_w));

   hash_table_insert(this->variable_ht, reg, ir);
}


void
fs_visitor::emit_general_interpolation(ir_variable *ir)
{
   fs_reg *reg = new(this->mem_ctx) fs_reg(this, ir->type);
   /* Interpolation is always in floating point regs. */
   reg->type = BRW_REGISTER_TYPE_F;
   fs_reg attr = *reg;

   unsigned int array_elements;
   const glsl_type *type;

   if (ir->type->is_array()) {
      array_elements = ir->type->length;
      if (array_elements == 0) {
	 this->fail = true;
      }
      type = ir->type->fields.array;
   } else {
      array_elements = 1;
      type = ir->type;
   }

   int location = ir->location;
   for (unsigned int i = 0; i < array_elements; i++) {
      for (unsigned int j = 0; j < type->matrix_columns; j++) {
	 if (!(fp->Base.InputsRead & BITFIELD64_BIT(location))) {
	    /* If there's no incoming setup data for this slot, don't
	     * emit interpolation for it (since it's not used, and
	     * we'd fall over later trying to find the setup data.
	     */
	    attr.reg_offset += type->vector_elements;
	    continue;
	 }

	 for (unsigned int c = 0; c < type->vector_elements; c++) {
	    struct brw_reg interp = interp_reg(location, c);
	    emit(fs_inst(FS_OPCODE_LINTERP,
			 attr,
			 this->delta_x,
			 this->delta_y,
			 fs_reg(interp)));
	    attr.reg_offset++;
	 }
	 attr.reg_offset -= type->vector_elements;

	 for (unsigned int c = 0; c < type->vector_elements; c++) {
	    emit(fs_inst(BRW_OPCODE_MUL,
			 attr,
			 attr,
			 this->pixel_w));
	    attr.reg_offset++;
	 }
	 location++;
      }
   }

   hash_table_insert(this->variable_ht, reg, ir);
}

void
fs_visitor::visit(ir_variable *ir)
{
   fs_reg *reg = NULL;

   if (variable_storage(ir))
      return;

   if (strcmp(ir->name, "gl_FragColor") == 0) {
      this->frag_color = ir;
   } else if (strcmp(ir->name, "gl_FragData") == 0) {
      this->frag_data = ir;
   } else if (strcmp(ir->name, "gl_FragDepth") == 0) {
      this->frag_depth = ir;
   }

   if (ir->mode == ir_var_in) {
      if (!strcmp(ir->name, "gl_FragCoord")) {
	 emit_fragcoord_interpolation(ir);
	 return;
      } else if (!strcmp(ir->name, "gl_FrontFacing")) {
	 reg = new(this->mem_ctx) fs_reg(this, ir->type);
	 struct brw_reg r1_6ud = retype(brw_vec1_grf(1, 6), BRW_REGISTER_TYPE_UD);
	 /* bit 31 is "primitive is back face", so checking < (1 << 31) gives
	  * us front face
	  */
	 fs_inst *inst = emit(fs_inst(BRW_OPCODE_CMP,
				      *reg,
				      fs_reg(r1_6ud),
				      fs_reg(1u << 31)));
	 inst->conditional_mod = BRW_CONDITIONAL_L;
	 emit(fs_inst(BRW_OPCODE_AND, *reg, *reg, fs_reg(1u)));
      } else {
	 emit_general_interpolation(ir);
	 return;
      }
   }

   if (ir->mode == ir_var_uniform) {
      int param_index = c->prog_data.nr_params;

      if (!strncmp(ir->name, "gl_", 3)) {
	 setup_builtin_uniform_values(ir);
      } else {
	 setup_uniform_values(ir->location, ir->type);
      }

      reg = new(this->mem_ctx) fs_reg(UNIFORM, param_index);
   }

   if (!reg)
      reg = new(this->mem_ctx) fs_reg(this, ir->type);

   hash_table_insert(this->variable_ht, reg, ir);
}

void
fs_visitor::visit(ir_dereference_variable *ir)
{
   fs_reg *reg = variable_storage(ir->var);
   this->result = *reg;
}

void
fs_visitor::visit(ir_dereference_record *ir)
{
   const glsl_type *struct_type = ir->record->type;

   ir->record->accept(this);

   unsigned int offset = 0;
   for (unsigned int i = 0; i < struct_type->length; i++) {
      if (strcmp(struct_type->fields.structure[i].name, ir->field) == 0)
	 break;
      offset += type_size(struct_type->fields.structure[i].type);
   }
   this->result.reg_offset += offset;
   this->result.type = brw_type_for_base_type(ir->type);
}

void
fs_visitor::visit(ir_dereference_array *ir)
{
   ir_constant *index;
   int element_size;

   ir->array->accept(this);
   index = ir->array_index->as_constant();

   element_size = type_size(ir->type);
   this->result.type = brw_type_for_base_type(ir->type);

   if (index) {
      assert(this->result.file == UNIFORM ||
	     (this->result.file == GRF &&
	      this->result.reg != 0));
      this->result.reg_offset += index->value.i[0] * element_size;
   } else {
      assert(!"FINISHME: non-constant array element");
   }
}

void
fs_visitor::visit(ir_expression *ir)
{
   unsigned int operand;
   fs_reg op[2], temp;
   fs_reg result;
   fs_inst *inst;

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      ir->operands[operand]->accept(this);
      if (this->result.file == BAD_FILE) {
	 ir_print_visitor v;
	 printf("Failed to get tree for expression operand:\n");
	 ir->operands[operand]->accept(&v);
	 this->fail = true;
      }
      op[operand] = this->result;

      /* Matrix expression operands should have been broken down to vector
       * operations already.
       */
      assert(!ir->operands[operand]->type->is_matrix());
      /* And then those vector operands should have been broken down to scalar.
       */
      assert(!ir->operands[operand]->type->is_vector());
   }

   /* Storage for our result.  If our result goes into an assignment, it will
    * just get copy-propagated out, so no worries.
    */
   this->result = fs_reg(this, ir->type);

   switch (ir->operation) {
   case ir_unop_logic_not:
      emit(fs_inst(BRW_OPCODE_ADD, this->result, op[0], fs_reg(-1)));
      break;
   case ir_unop_neg:
      op[0].negate = !op[0].negate;
      this->result = op[0];
      break;
   case ir_unop_abs:
      op[0].abs = true;
      this->result = op[0];
      break;
   case ir_unop_sign:
      temp = fs_reg(this, ir->type);

      emit(fs_inst(BRW_OPCODE_MOV, this->result, fs_reg(0.0f)));

      inst = emit(fs_inst(BRW_OPCODE_CMP, reg_null, op[0], fs_reg(0.0f)));
      inst->conditional_mod = BRW_CONDITIONAL_G;
      inst = emit(fs_inst(BRW_OPCODE_MOV, this->result, fs_reg(1.0f)));
      inst->predicated = true;

      inst = emit(fs_inst(BRW_OPCODE_CMP, reg_null, op[0], fs_reg(0.0f)));
      inst->conditional_mod = BRW_CONDITIONAL_L;
      inst = emit(fs_inst(BRW_OPCODE_MOV, this->result, fs_reg(-1.0f)));
      inst->predicated = true;

      break;
   case ir_unop_rcp:
      emit(fs_inst(FS_OPCODE_RCP, this->result, op[0]));
      break;

   case ir_unop_exp2:
      emit(fs_inst(FS_OPCODE_EXP2, this->result, op[0]));
      break;
   case ir_unop_log2:
      emit(fs_inst(FS_OPCODE_LOG2, this->result, op[0]));
      break;
   case ir_unop_exp:
   case ir_unop_log:
      assert(!"not reached: should be handled by ir_explog_to_explog2");
      break;
   case ir_unop_sin:
      emit(fs_inst(FS_OPCODE_SIN, this->result, op[0]));
      break;
   case ir_unop_cos:
      emit(fs_inst(FS_OPCODE_COS, this->result, op[0]));
      break;

   case ir_unop_dFdx:
      emit(fs_inst(FS_OPCODE_DDX, this->result, op[0]));
      break;
   case ir_unop_dFdy:
      emit(fs_inst(FS_OPCODE_DDY, this->result, op[0]));
      break;

   case ir_binop_add:
      emit(fs_inst(BRW_OPCODE_ADD, this->result, op[0], op[1]));
      break;
   case ir_binop_sub:
      assert(!"not reached: should be handled by ir_sub_to_add_neg");
      break;

   case ir_binop_mul:
      emit(fs_inst(BRW_OPCODE_MUL, this->result, op[0], op[1]));
      break;
   case ir_binop_div:
      assert(!"not reached: should be handled by ir_div_to_mul_rcp");
      break;
   case ir_binop_mod:
      assert(!"ir_binop_mod should have been converted to b * fract(a/b)");
      break;

   case ir_binop_less:
      inst = emit(fs_inst(BRW_OPCODE_CMP, this->result, op[0], op[1]));
      inst->conditional_mod = BRW_CONDITIONAL_L;
      emit(fs_inst(BRW_OPCODE_AND, this->result, this->result, fs_reg(0x1)));
      break;
   case ir_binop_greater:
      inst = emit(fs_inst(BRW_OPCODE_CMP, this->result, op[0], op[1]));
      inst->conditional_mod = BRW_CONDITIONAL_G;
      emit(fs_inst(BRW_OPCODE_AND, this->result, this->result, fs_reg(0x1)));
      break;
   case ir_binop_lequal:
      inst = emit(fs_inst(BRW_OPCODE_CMP, this->result, op[0], op[1]));
      inst->conditional_mod = BRW_CONDITIONAL_LE;
      emit(fs_inst(BRW_OPCODE_AND, this->result, this->result, fs_reg(0x1)));
      break;
   case ir_binop_gequal:
      inst = emit(fs_inst(BRW_OPCODE_CMP, this->result, op[0], op[1]));
      inst->conditional_mod = BRW_CONDITIONAL_GE;
      emit(fs_inst(BRW_OPCODE_AND, this->result, this->result, fs_reg(0x1)));
      break;
   case ir_binop_equal:
   case ir_binop_all_equal: /* same as nequal for scalars */
      inst = emit(fs_inst(BRW_OPCODE_CMP, this->result, op[0], op[1]));
      inst->conditional_mod = BRW_CONDITIONAL_Z;
      emit(fs_inst(BRW_OPCODE_AND, this->result, this->result, fs_reg(0x1)));
      break;
   case ir_binop_nequal:
   case ir_binop_any_nequal: /* same as nequal for scalars */
      inst = emit(fs_inst(BRW_OPCODE_CMP, this->result, op[0], op[1]));
      inst->conditional_mod = BRW_CONDITIONAL_NZ;
      emit(fs_inst(BRW_OPCODE_AND, this->result, this->result, fs_reg(0x1)));
      break;

   case ir_binop_logic_xor:
      emit(fs_inst(BRW_OPCODE_XOR, this->result, op[0], op[1]));
      break;

   case ir_binop_logic_or:
      emit(fs_inst(BRW_OPCODE_OR, this->result, op[0], op[1]));
      break;

   case ir_binop_logic_and:
      emit(fs_inst(BRW_OPCODE_AND, this->result, op[0], op[1]));
      break;

   case ir_binop_dot:
   case ir_binop_cross:
   case ir_unop_any:
      assert(!"not reached: should be handled by brw_fs_channel_expressions");
      break;

   case ir_unop_noise:
      assert(!"not reached: should be handled by lower_noise");
      break;

   case ir_unop_sqrt:
      emit(fs_inst(FS_OPCODE_SQRT, this->result, op[0]));
      break;

   case ir_unop_rsq:
      emit(fs_inst(FS_OPCODE_RSQ, this->result, op[0]));
      break;

   case ir_unop_i2f:
   case ir_unop_b2f:
   case ir_unop_b2i:
      emit(fs_inst(BRW_OPCODE_MOV, this->result, op[0]));
      break;
   case ir_unop_f2i:
      emit(fs_inst(BRW_OPCODE_MOV, this->result, op[0]));
      break;
   case ir_unop_f2b:
   case ir_unop_i2b:
      inst = emit(fs_inst(BRW_OPCODE_CMP, this->result, op[0], fs_reg(0.0f)));
      inst->conditional_mod = BRW_CONDITIONAL_NZ;

   case ir_unop_trunc:
      emit(fs_inst(BRW_OPCODE_RNDD, this->result, op[0]));
      break;
   case ir_unop_ceil:
      op[0].negate = ~op[0].negate;
      inst = emit(fs_inst(BRW_OPCODE_RNDD, this->result, op[0]));
      this->result.negate = true;
      break;
   case ir_unop_floor:
      inst = emit(fs_inst(BRW_OPCODE_RNDD, this->result, op[0]));
      break;
   case ir_unop_fract:
      inst = emit(fs_inst(BRW_OPCODE_FRC, this->result, op[0]));
      break;

   case ir_binop_min:
      inst = emit(fs_inst(BRW_OPCODE_CMP, this->result, op[0], op[1]));
      inst->conditional_mod = BRW_CONDITIONAL_L;

      inst = emit(fs_inst(BRW_OPCODE_SEL, this->result, op[0], op[1]));
      inst->predicated = true;
      break;
   case ir_binop_max:
      inst = emit(fs_inst(BRW_OPCODE_CMP, this->result, op[0], op[1]));
      inst->conditional_mod = BRW_CONDITIONAL_G;

      inst = emit(fs_inst(BRW_OPCODE_SEL, this->result, op[0], op[1]));
      inst->predicated = true;
      break;

   case ir_binop_pow:
      inst = emit(fs_inst(FS_OPCODE_POW, this->result, op[0], op[1]));
      break;

   case ir_unop_bit_not:
   case ir_unop_u2f:
   case ir_binop_lshift:
   case ir_binop_rshift:
   case ir_binop_bit_and:
   case ir_binop_bit_xor:
   case ir_binop_bit_or:
      assert(!"GLSL 1.30 features unsupported");
      break;
   }
}

void
fs_visitor::visit(ir_assignment *ir)
{
   struct fs_reg l, r;
   int i;
   int write_mask;
   fs_inst *inst;

   /* FINISHME: arrays on the lhs */
   ir->lhs->accept(this);
   l = this->result;

   ir->rhs->accept(this);
   r = this->result;

   /* FINISHME: This should really set to the correct maximal writemask for each
    * FINISHME: component written (in the loops below).  This case can only
    * FINISHME: occur for matrices, arrays, and structures.
    */
   if (ir->write_mask == 0) {
      assert(!ir->lhs->type->is_scalar() && !ir->lhs->type->is_vector());
      write_mask = WRITEMASK_XYZW;
   } else {
      assert(ir->lhs->type->is_vector() || ir->lhs->type->is_scalar());
      write_mask = ir->write_mask;
   }

   assert(l.file != BAD_FILE);
   assert(r.file != BAD_FILE);

   if (ir->condition) {
      /* Get the condition bool into the predicate. */
      ir->condition->accept(this);
      inst = emit(fs_inst(BRW_OPCODE_CMP, reg_null, this->result, fs_reg(0)));
      inst->conditional_mod = BRW_CONDITIONAL_NZ;
   }

   for (i = 0; i < type_size(ir->lhs->type); i++) {
      if (i >= 4 || (write_mask & (1 << i))) {
	 inst = emit(fs_inst(BRW_OPCODE_MOV, l, r));
	 if (ir->condition)
	    inst->predicated = true;
	 r.reg_offset++;
      }
      l.reg_offset++;
   }
}

void
fs_visitor::visit(ir_texture *ir)
{
   int base_mrf = 2;
   fs_inst *inst = NULL;
   unsigned int mlen = 0;

   ir->coordinate->accept(this);
   fs_reg coordinate = this->result;

   if (ir->projector) {
      fs_reg inv_proj = fs_reg(this, glsl_type::float_type);

      ir->projector->accept(this);
      emit(fs_inst(FS_OPCODE_RCP, inv_proj, this->result));

      fs_reg proj_coordinate = fs_reg(this, ir->coordinate->type);
      for (unsigned int i = 0; i < ir->coordinate->type->vector_elements; i++) {
	 emit(fs_inst(BRW_OPCODE_MUL, proj_coordinate, coordinate, inv_proj));
	 coordinate.reg_offset++;
	 proj_coordinate.reg_offset++;
      }
      proj_coordinate.reg_offset = 0;

      coordinate = proj_coordinate;
   }

   for (mlen = 0; mlen < ir->coordinate->type->vector_elements; mlen++) {
      emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), coordinate));
      coordinate.reg_offset++;
   }

   /* Pre-Ironlake, the 8-wide sampler always took u,v,r. */
   if (intel->gen < 5)
      mlen = 3;

   if (ir->shadow_comparitor) {
      /* For shadow comparisons, we have to supply u,v,r. */
      mlen = 3;

      ir->shadow_comparitor->accept(this);
      emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), this->result));
      mlen++;
   }

   /* Do we ever want to handle writemasking on texture samples?  Is it
    * performance relevant?
    */
   fs_reg dst = fs_reg(this, glsl_type::vec4_type);

   switch (ir->op) {
   case ir_tex:
      inst = emit(fs_inst(FS_OPCODE_TEX, dst, fs_reg(MRF, base_mrf)));
      break;
   case ir_txb:
      ir->lod_info.bias->accept(this);
      emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), this->result));
      mlen++;

      inst = emit(fs_inst(FS_OPCODE_TXB, dst, fs_reg(MRF, base_mrf)));
      break;
   case ir_txl:
      ir->lod_info.lod->accept(this);
      emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, base_mrf + mlen), this->result));
      mlen++;

      inst = emit(fs_inst(FS_OPCODE_TXL, dst, fs_reg(MRF, base_mrf)));
      break;
   case ir_txd:
   case ir_txf:
      assert(!"GLSL 1.30 features unsupported");
      break;
   }

   inst->sampler =
      _mesa_get_sampler_uniform_value(ir->sampler,
				      ctx->Shader.CurrentProgram,
				      &brw->fragment_program->Base);
   inst->sampler = c->fp->program.Base.SamplerUnits[inst->sampler];

   this->result = dst;

   if (ir->shadow_comparitor)
      inst->shadow_compare = true;
   inst->mlen = mlen;
}

void
fs_visitor::visit(ir_swizzle *ir)
{
   ir->val->accept(this);
   fs_reg val = this->result;

   fs_reg result = fs_reg(this, ir->type);
   this->result = result;

   for (unsigned int i = 0; i < ir->type->vector_elements; i++) {
      fs_reg channel = val;
      int swiz = 0;

      switch (i) {
      case 0:
	 swiz = ir->mask.x;
	 break;
      case 1:
	 swiz = ir->mask.y;
	 break;
      case 2:
	 swiz = ir->mask.z;
	 break;
      case 3:
	 swiz = ir->mask.w;
	 break;
      }

      channel.reg_offset += swiz;
      emit(fs_inst(BRW_OPCODE_MOV, result, channel));
      result.reg_offset++;
   }
}

void
fs_visitor::visit(ir_discard *ir)
{
   fs_reg temp = fs_reg(this, glsl_type::uint_type);

   assert(ir->condition == NULL); /* FINISHME */

   emit(fs_inst(FS_OPCODE_DISCARD, temp, temp));
}

void
fs_visitor::visit(ir_constant *ir)
{
   fs_reg reg(this, ir->type);
   this->result = reg;

   for (unsigned int i = 0; i < ir->type->vector_elements; i++) {
      switch (ir->type->base_type) {
      case GLSL_TYPE_FLOAT:
	 emit(fs_inst(BRW_OPCODE_MOV, reg, fs_reg(ir->value.f[i])));
	 break;
      case GLSL_TYPE_UINT:
	 emit(fs_inst(BRW_OPCODE_MOV, reg, fs_reg(ir->value.u[i])));
	 break;
      case GLSL_TYPE_INT:
	 emit(fs_inst(BRW_OPCODE_MOV, reg, fs_reg(ir->value.i[i])));
	 break;
      case GLSL_TYPE_BOOL:
	 emit(fs_inst(BRW_OPCODE_MOV, reg, fs_reg((int)ir->value.b[i])));
	 break;
      default:
	 assert(!"Non-float/uint/int/bool constant");
      }
      reg.reg_offset++;
   }
}

void
fs_visitor::visit(ir_if *ir)
{
   fs_inst *inst;

   /* Don't point the annotation at the if statement, because then it plus
    * the then and else blocks get printed.
    */
   this->base_ir = ir->condition;

   /* Generate the condition into the condition code. */
   ir->condition->accept(this);
   inst = emit(fs_inst(BRW_OPCODE_MOV, fs_reg(brw_null_reg()), this->result));
   inst->conditional_mod = BRW_CONDITIONAL_NZ;

   inst = emit(fs_inst(BRW_OPCODE_IF));
   inst->predicated = true;

   foreach_iter(exec_list_iterator, iter, ir->then_instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      this->base_ir = ir;

      ir->accept(this);
   }

   if (!ir->else_instructions.is_empty()) {
      emit(fs_inst(BRW_OPCODE_ELSE));

      foreach_iter(exec_list_iterator, iter, ir->else_instructions) {
	 ir_instruction *ir = (ir_instruction *)iter.get();
	 this->base_ir = ir;

	 ir->accept(this);
      }
   }

   emit(fs_inst(BRW_OPCODE_ENDIF));
}

void
fs_visitor::visit(ir_loop *ir)
{
   fs_reg counter = reg_undef;

   if (ir->counter) {
      this->base_ir = ir->counter;
      ir->counter->accept(this);
      counter = *(variable_storage(ir->counter));

      if (ir->from) {
	 this->base_ir = ir->from;
	 ir->from->accept(this);

	 emit(fs_inst(BRW_OPCODE_MOV, counter, this->result));
      }
   }

   /* Start a safety counter.  If the user messed up their loop
    * counting, we don't want to hang the GPU.
    */
   fs_reg max_iter = fs_reg(this, glsl_type::int_type);
   emit(fs_inst(BRW_OPCODE_MOV, max_iter, fs_reg(10000)));

   emit(fs_inst(BRW_OPCODE_DO));

   if (ir->to) {
      this->base_ir = ir->to;
      ir->to->accept(this);

      fs_inst *inst = emit(fs_inst(BRW_OPCODE_CMP, reg_null,
				   counter, this->result));
      switch (ir->cmp) {
      case ir_binop_equal:
	 inst->conditional_mod = BRW_CONDITIONAL_Z;
	 break;
      case ir_binop_nequal:
	 inst->conditional_mod = BRW_CONDITIONAL_NZ;
	 break;
      case ir_binop_gequal:
	 inst->conditional_mod = BRW_CONDITIONAL_GE;
	 break;
      case ir_binop_lequal:
	 inst->conditional_mod = BRW_CONDITIONAL_LE;
	 break;
      case ir_binop_greater:
	 inst->conditional_mod = BRW_CONDITIONAL_G;
	 break;
      case ir_binop_less:
	 inst->conditional_mod = BRW_CONDITIONAL_L;
	 break;
      default:
	 assert(!"not reached: unknown loop condition");
	 this->fail = true;
	 break;
      }

      inst = emit(fs_inst(BRW_OPCODE_BREAK));
      inst->predicated = true;
   }

   foreach_iter(exec_list_iterator, iter, ir->body_instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      fs_inst *inst;

      this->base_ir = ir;
      ir->accept(this);

      /* Check the maximum loop iters counter. */
      inst = emit(fs_inst(BRW_OPCODE_ADD, max_iter, max_iter, fs_reg(-1)));
      inst->conditional_mod = BRW_CONDITIONAL_Z;

      inst = emit(fs_inst(BRW_OPCODE_BREAK));
      inst->predicated = true;
   }

   if (ir->increment) {
      this->base_ir = ir->increment;
      ir->increment->accept(this);
      emit(fs_inst(BRW_OPCODE_ADD, counter, counter, this->result));
   }

   emit(fs_inst(BRW_OPCODE_WHILE));
}

void
fs_visitor::visit(ir_loop_jump *ir)
{
   switch (ir->mode) {
   case ir_loop_jump::jump_break:
      emit(fs_inst(BRW_OPCODE_BREAK));
      break;
   case ir_loop_jump::jump_continue:
      emit(fs_inst(BRW_OPCODE_CONTINUE));
      break;
   }
}

void
fs_visitor::visit(ir_call *ir)
{
   assert(!"FINISHME");
}

void
fs_visitor::visit(ir_return *ir)
{
   assert(!"FINISHME");
}

void
fs_visitor::visit(ir_function *ir)
{
   /* Ignore function bodies other than main() -- we shouldn't see calls to
    * them since they should all be inlined before we get to ir_to_mesa.
    */
   if (strcmp(ir->name, "main") == 0) {
      const ir_function_signature *sig;
      exec_list empty;

      sig = ir->matching_signature(&empty);

      assert(sig);

      foreach_iter(exec_list_iterator, iter, sig->body) {
	 ir_instruction *ir = (ir_instruction *)iter.get();
	 this->base_ir = ir;

	 ir->accept(this);
      }
   }
}

void
fs_visitor::visit(ir_function_signature *ir)
{
   assert(!"not reached");
   (void)ir;
}

fs_inst *
fs_visitor::emit(fs_inst inst)
{
   fs_inst *list_inst = new(mem_ctx) fs_inst;
   *list_inst = inst;

   list_inst->annotation = this->current_annotation;
   list_inst->ir = this->base_ir;

   this->instructions.push_tail(list_inst);

   return list_inst;
}

/** Emits a dummy fragment shader consisting of magenta for bringup purposes. */
void
fs_visitor::emit_dummy_fs()
{
   /* Everyone's favorite color. */
   emit(fs_inst(BRW_OPCODE_MOV,
		fs_reg(MRF, 2),
		fs_reg(1.0f)));
   emit(fs_inst(BRW_OPCODE_MOV,
		fs_reg(MRF, 3),
		fs_reg(0.0f)));
   emit(fs_inst(BRW_OPCODE_MOV,
		fs_reg(MRF, 4),
		fs_reg(1.0f)));
   emit(fs_inst(BRW_OPCODE_MOV,
		fs_reg(MRF, 5),
		fs_reg(0.0f)));

   fs_inst *write;
   write = emit(fs_inst(FS_OPCODE_FB_WRITE,
			fs_reg(0),
			fs_reg(0)));
}

/* The register location here is relative to the start of the URB
 * data.  It will get adjusted to be a real location before
 * generate_code() time.
 */
struct brw_reg
fs_visitor::interp_reg(int location, int channel)
{
   int regnr = location * 2 + channel / 2;
   int stride = (channel & 1) * 4;

   return brw_vec1_grf(regnr, stride);
}

/** Emits the interpolation for the varying inputs. */
void
fs_visitor::emit_interpolation_setup()
{
   struct brw_reg g1_uw = retype(brw_vec1_grf(1, 0), BRW_REGISTER_TYPE_UW);

   this->current_annotation = "compute pixel centers";
   this->pixel_x = fs_reg(this, glsl_type::uint_type);
   this->pixel_y = fs_reg(this, glsl_type::uint_type);
   this->pixel_x.type = BRW_REGISTER_TYPE_UW;
   this->pixel_y.type = BRW_REGISTER_TYPE_UW;
   emit(fs_inst(BRW_OPCODE_ADD,
		this->pixel_x,
		fs_reg(stride(suboffset(g1_uw, 4), 2, 4, 0)),
		fs_reg(brw_imm_v(0x10101010))));
   emit(fs_inst(BRW_OPCODE_ADD,
		this->pixel_y,
		fs_reg(stride(suboffset(g1_uw, 5), 2, 4, 0)),
		fs_reg(brw_imm_v(0x11001100))));

   this->current_annotation = "compute pixel deltas from v0";
   this->delta_x = fs_reg(this, glsl_type::float_type);
   this->delta_y = fs_reg(this, glsl_type::float_type);
   emit(fs_inst(BRW_OPCODE_ADD,
		this->delta_x,
		this->pixel_x,
		fs_reg(negate(brw_vec1_grf(1, 0)))));
   emit(fs_inst(BRW_OPCODE_ADD,
		this->delta_y,
		this->pixel_y,
		fs_reg(negate(brw_vec1_grf(1, 1)))));

   this->current_annotation = "compute pos.w and 1/pos.w";
   /* Compute wpos.w.  It's always in our setup, since it's needed to
    * interpolate the other attributes.
    */
   this->wpos_w = fs_reg(this, glsl_type::float_type);
   emit(fs_inst(FS_OPCODE_LINTERP, wpos_w, this->delta_x, this->delta_y,
		interp_reg(FRAG_ATTRIB_WPOS, 3)));
   /* Compute the pixel 1/W value from wpos.w. */
   this->pixel_w = fs_reg(this, glsl_type::float_type);
   emit(fs_inst(FS_OPCODE_RCP, this->pixel_w, wpos_w));
   this->current_annotation = NULL;
}

void
fs_visitor::emit_fb_writes()
{
   this->current_annotation = "FB write header";
   int nr = 0;

   /* m0, m1 header */
   nr += 2;

   if (c->key.aa_dest_stencil_reg) {
      emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, nr++),
		   fs_reg(brw_vec8_grf(c->key.aa_dest_stencil_reg, 0))));
   }

   /* Reserve space for color. It'll be filled in per MRT below. */
   int color_mrf = nr;
   nr += 4;

   if (c->key.source_depth_to_render_target) {
      if (c->key.computes_depth) {
	 /* Hand over gl_FragDepth. */
	 assert(this->frag_depth);
	 fs_reg depth = *(variable_storage(this->frag_depth));

	 emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, nr++), depth));
      } else {
	 /* Pass through the payload depth. */
	 emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, nr++),
		      fs_reg(brw_vec8_grf(c->key.source_depth_reg, 0))));
      }
   }

   if (c->key.dest_depth_reg) {
      emit(fs_inst(BRW_OPCODE_MOV, fs_reg(MRF, nr++),
		   fs_reg(brw_vec8_grf(c->key.dest_depth_reg, 0))));
   }

   fs_reg color = reg_undef;
   if (this->frag_color)
      color = *(variable_storage(this->frag_color));
   else if (this->frag_data)
      color = *(variable_storage(this->frag_data));

   for (int target = 0; target < c->key.nr_color_regions; target++) {
      this->current_annotation = talloc_asprintf(this->mem_ctx,
						 "FB write target %d",
						 target);
      if (this->frag_color || this->frag_data) {
	 for (int i = 0; i < 4; i++) {
	    emit(fs_inst(BRW_OPCODE_MOV,
			 fs_reg(MRF, color_mrf + i),
			 color));
	    color.reg_offset++;
	 }
      }

      if (this->frag_color)
	 color.reg_offset -= 4;

      fs_inst *inst = emit(fs_inst(FS_OPCODE_FB_WRITE,
				   reg_undef, reg_undef));
      inst->target = target;
      inst->mlen = nr;
      if (target == c->key.nr_color_regions - 1)
	 inst->eot = true;
   }

   if (c->key.nr_color_regions == 0) {
      fs_inst *inst = emit(fs_inst(FS_OPCODE_FB_WRITE,
				   reg_undef, reg_undef));
      inst->mlen = nr;
      inst->eot = true;
   }

   this->current_annotation = NULL;
}

void
fs_visitor::generate_fb_write(fs_inst *inst)
{
   GLboolean eot = inst->eot;

   /* Header is 2 regs, g0 and g1 are the contents. g0 will be implied
    * move, here's g1.
    */
   brw_push_insn_state(p);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_MOV(p,
	   brw_message_reg(1),
	   brw_vec8_grf(1, 0));
   brw_pop_insn_state(p);

   brw_fb_WRITE(p,
		8, /* dispatch_width */
		retype(vec8(brw_null_reg()), BRW_REGISTER_TYPE_UW),
		0, /* base MRF */
		retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UW),
		inst->target,
		inst->mlen,
		0,
		eot);
}

void
fs_visitor::generate_linterp(fs_inst *inst,
			     struct brw_reg dst, struct brw_reg *src)
{
   struct brw_reg delta_x = src[0];
   struct brw_reg delta_y = src[1];
   struct brw_reg interp = src[2];

   if (brw->has_pln &&
       delta_y.nr == delta_x.nr + 1 &&
       (intel->gen >= 6 || (delta_x.nr & 1) == 0)) {
      brw_PLN(p, dst, interp, delta_x);
   } else {
      brw_LINE(p, brw_null_reg(), interp, delta_x);
      brw_MAC(p, dst, suboffset(interp, 1), delta_y);
   }
}

void
fs_visitor::generate_math(fs_inst *inst,
			  struct brw_reg dst, struct brw_reg *src)
{
   int op;

   switch (inst->opcode) {
   case FS_OPCODE_RCP:
      op = BRW_MATH_FUNCTION_INV;
      break;
   case FS_OPCODE_RSQ:
      op = BRW_MATH_FUNCTION_RSQ;
      break;
   case FS_OPCODE_SQRT:
      op = BRW_MATH_FUNCTION_SQRT;
      break;
   case FS_OPCODE_EXP2:
      op = BRW_MATH_FUNCTION_EXP;
      break;
   case FS_OPCODE_LOG2:
      op = BRW_MATH_FUNCTION_LOG;
      break;
   case FS_OPCODE_POW:
      op = BRW_MATH_FUNCTION_POW;
      break;
   case FS_OPCODE_SIN:
      op = BRW_MATH_FUNCTION_SIN;
      break;
   case FS_OPCODE_COS:
      op = BRW_MATH_FUNCTION_COS;
      break;
   default:
      assert(!"not reached: unknown math function");
      op = 0;
      break;
   }

   if (inst->opcode == FS_OPCODE_POW) {
      brw_MOV(p, brw_message_reg(3), src[1]);
   }

   brw_math(p, dst,
	    op,
	    inst->saturate ? BRW_MATH_SATURATE_SATURATE :
	    BRW_MATH_SATURATE_NONE,
	    2, src[0],
	    BRW_MATH_DATA_VECTOR,
	    BRW_MATH_PRECISION_FULL);
}

void
fs_visitor::generate_tex(fs_inst *inst, struct brw_reg dst, struct brw_reg src)
{
   int msg_type = -1;
   int rlen = 4;

   if (intel->gen == 5) {
      switch (inst->opcode) {
      case FS_OPCODE_TEX:
	 if (inst->shadow_compare) {
	    msg_type = BRW_SAMPLER_MESSAGE_SAMPLE_COMPARE_GEN5;
	 } else {
	    msg_type = BRW_SAMPLER_MESSAGE_SAMPLE_GEN5;
	 }
	 break;
      case FS_OPCODE_TXB:
	 if (inst->shadow_compare) {
	    msg_type = BRW_SAMPLER_MESSAGE_SAMPLE_BIAS_COMPARE_GEN5;
	 } else {
	    msg_type = BRW_SAMPLER_MESSAGE_SAMPLE_BIAS_GEN5;
	 }
	 break;
      }
   } else {
      switch (inst->opcode) {
      case FS_OPCODE_TEX:
	 /* Note that G45 and older determines shadow compare and dispatch width
	  * from message length for most messages.
	  */
	 if (inst->shadow_compare) {
	    msg_type = BRW_SAMPLER_MESSAGE_SIMD16_SAMPLE_COMPARE;
	 } else {
	    msg_type = BRW_SAMPLER_MESSAGE_SIMD16_SAMPLE;
	 }
      case FS_OPCODE_TXB:
	 if (inst->shadow_compare) {
	    assert(!"FINISHME: shadow compare with bias.");
	    msg_type = BRW_SAMPLER_MESSAGE_SIMD16_SAMPLE_BIAS;
	 } else {
	    msg_type = BRW_SAMPLER_MESSAGE_SIMD16_SAMPLE_BIAS;
	    rlen = 8;
	 }
	 break;
      }
   }
   assert(msg_type != -1);

   /* g0 header. */
   src.nr--;

   brw_SAMPLE(p,
	      retype(dst, BRW_REGISTER_TYPE_UW),
	      src.nr,
	      retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UW),
              SURF_INDEX_TEXTURE(inst->sampler),
	      inst->sampler,
	      WRITEMASK_XYZW,
	      msg_type,
	      rlen,
	      inst->mlen + 1,
	      0,
	      1,
	      BRW_SAMPLER_SIMD_MODE_SIMD8);
}


/* For OPCODE_DDX and OPCODE_DDY, per channel of output we've got input
 * looking like:
 *
 * arg0: ss0.tl ss0.tr ss0.bl ss0.br ss1.tl ss1.tr ss1.bl ss1.br
 *
 * and we're trying to produce:
 *
 *           DDX                     DDY
 * dst: (ss0.tr - ss0.tl)     (ss0.tl - ss0.bl)
 *      (ss0.tr - ss0.tl)     (ss0.tr - ss0.br)
 *      (ss0.br - ss0.bl)     (ss0.tl - ss0.bl)
 *      (ss0.br - ss0.bl)     (ss0.tr - ss0.br)
 *      (ss1.tr - ss1.tl)     (ss1.tl - ss1.bl)
 *      (ss1.tr - ss1.tl)     (ss1.tr - ss1.br)
 *      (ss1.br - ss1.bl)     (ss1.tl - ss1.bl)
 *      (ss1.br - ss1.bl)     (ss1.tr - ss1.br)
 *
 * and add another set of two more subspans if in 16-pixel dispatch mode.
 *
 * For DDX, it ends up being easy: width = 2, horiz=0 gets us the same result
 * for each pair, and vertstride = 2 jumps us 2 elements after processing a
 * pair. But for DDY, it's harder, as we want to produce the pairs swizzled
 * between each other.  We could probably do it like ddx and swizzle the right
 * order later, but bail for now and just produce
 * ((ss0.tl - ss0.bl)x4 (ss1.tl - ss1.bl)x4)
 */
void
fs_visitor::generate_ddx(fs_inst *inst, struct brw_reg dst, struct brw_reg src)
{
   struct brw_reg src0 = brw_reg(src.file, src.nr, 1,
				 BRW_REGISTER_TYPE_F,
				 BRW_VERTICAL_STRIDE_2,
				 BRW_WIDTH_2,
				 BRW_HORIZONTAL_STRIDE_0,
				 BRW_SWIZZLE_XYZW, WRITEMASK_XYZW);
   struct brw_reg src1 = brw_reg(src.file, src.nr, 0,
				 BRW_REGISTER_TYPE_F,
				 BRW_VERTICAL_STRIDE_2,
				 BRW_WIDTH_2,
				 BRW_HORIZONTAL_STRIDE_0,
				 BRW_SWIZZLE_XYZW, WRITEMASK_XYZW);
   brw_ADD(p, dst, src0, negate(src1));
}

void
fs_visitor::generate_ddy(fs_inst *inst, struct brw_reg dst, struct brw_reg src)
{
   struct brw_reg src0 = brw_reg(src.file, src.nr, 0,
				 BRW_REGISTER_TYPE_F,
				 BRW_VERTICAL_STRIDE_4,
				 BRW_WIDTH_4,
				 BRW_HORIZONTAL_STRIDE_0,
				 BRW_SWIZZLE_XYZW, WRITEMASK_XYZW);
   struct brw_reg src1 = brw_reg(src.file, src.nr, 2,
				 BRW_REGISTER_TYPE_F,
				 BRW_VERTICAL_STRIDE_4,
				 BRW_WIDTH_4,
				 BRW_HORIZONTAL_STRIDE_0,
				 BRW_SWIZZLE_XYZW, WRITEMASK_XYZW);
   brw_ADD(p, dst, src0, negate(src1));
}

void
fs_visitor::generate_discard(fs_inst *inst, struct brw_reg temp)
{
   struct brw_reg g0 = retype(brw_vec1_grf(0, 0), BRW_REGISTER_TYPE_UW);
   temp = brw_uw1_reg(temp.file, temp.nr, 0);

   brw_push_insn_state(p);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_NOT(p, temp, brw_mask_reg(1)); /* IMASK */
   brw_AND(p, g0, temp, g0);
   brw_pop_insn_state(p);
}

void
fs_visitor::assign_curb_setup()
{
   c->prog_data.first_curbe_grf = c->key.nr_payload_regs;
   c->prog_data.curb_read_length = ALIGN(c->prog_data.nr_params, 8) / 8;

   if (intel->gen == 5 && (c->prog_data.first_curbe_grf +
			   c->prog_data.curb_read_length) & 1) {
      /* Align the start of the interpolation coefficients so that we can use
       * the PLN instruction.
       */
      c->prog_data.first_curbe_grf++;
   }

   /* Map the offsets in the UNIFORM file to fixed HW regs. */
   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      for (unsigned int i = 0; i < 3; i++) {
	 if (inst->src[i].file == UNIFORM) {
	    int constant_nr = inst->src[i].hw_reg + inst->src[i].reg_offset;
	    struct brw_reg brw_reg = brw_vec1_grf(c->prog_data.first_curbe_grf +
						  constant_nr / 8,
						  constant_nr % 8);

	    inst->src[i].file = FIXED_HW_REG;
	    inst->src[i].fixed_hw_reg = brw_reg;
	 }
      }
   }
}

void
fs_visitor::assign_urb_setup()
{
   int urb_start = c->prog_data.first_curbe_grf + c->prog_data.curb_read_length;
   int interp_reg_nr[FRAG_ATTRIB_MAX];

   c->prog_data.urb_read_length = 0;

   /* Figure out where each of the incoming setup attributes lands. */
   for (unsigned int i = 0; i < FRAG_ATTRIB_MAX; i++) {
      interp_reg_nr[i] = -1;

      if (i != FRAG_ATTRIB_WPOS &&
	  !(brw->fragment_program->Base.InputsRead & BITFIELD64_BIT(i)))
	 continue;

      /* Each attribute is 4 setup channels, each of which is half a reg. */
      interp_reg_nr[i] = urb_start + c->prog_data.urb_read_length;
      c->prog_data.urb_read_length += 2;
   }

   /* Map the register numbers for FS_OPCODE_LINTERP so that it uses
    * the correct setup input.
    */
   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      if (inst->opcode != FS_OPCODE_LINTERP)
	 continue;

      assert(inst->src[2].file == FIXED_HW_REG);

      int location = inst->src[2].fixed_hw_reg.nr / 2;
      assert(interp_reg_nr[location] != -1);
      inst->src[2].fixed_hw_reg.nr = (interp_reg_nr[location] +
				      (inst->src[2].fixed_hw_reg.nr & 1));
   }

   this->first_non_payload_grf = urb_start + c->prog_data.urb_read_length;
}

static void
assign_reg(int *reg_hw_locations, fs_reg *reg)
{
   if (reg->file == GRF && reg->reg != 0) {
      reg->hw_reg = reg_hw_locations[reg->reg] + reg->reg_offset;
      reg->reg = 0;
   }
}

void
fs_visitor::assign_regs_trivial()
{
   int last_grf = 0;
   int hw_reg_mapping[this->virtual_grf_next];
   int i;

   hw_reg_mapping[0] = 0;
   hw_reg_mapping[1] = this->first_non_payload_grf;
   for (i = 2; i < this->virtual_grf_next; i++) {
      hw_reg_mapping[i] = (hw_reg_mapping[i - 1] +
			   this->virtual_grf_sizes[i - 1]);
   }
   last_grf = hw_reg_mapping[i - 1] + this->virtual_grf_sizes[i - 1];

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      assign_reg(hw_reg_mapping, &inst->dst);
      assign_reg(hw_reg_mapping, &inst->src[0]);
      assign_reg(hw_reg_mapping, &inst->src[1]);
   }

   this->grf_used = last_grf + 1;
}

void
fs_visitor::assign_regs()
{
   int last_grf = 0;
   int hw_reg_mapping[this->virtual_grf_next + 1];
   int base_reg_count = BRW_MAX_GRF - this->first_non_payload_grf;
   int class_sizes[base_reg_count];
   int class_count = 0;

   /* Set up the register classes.
    *
    * The base registers store a scalar value.  For texture samples,
    * we get virtual GRFs composed of 4 contiguous hw register.  For
    * structures and arrays, we store them as contiguous larger things
    * than that, though we should be able to do better most of the
    * time.
    */
   class_sizes[class_count++] = 1;
   for (int r = 1; r < this->virtual_grf_next; r++) {
      int i;

      for (i = 0; i < class_count; i++) {
	 if (class_sizes[i] == this->virtual_grf_sizes[r])
	    break;
      }
      if (i == class_count) {
	 class_sizes[class_count++] = this->virtual_grf_sizes[r];
      }
   }

   int ra_reg_count = 0;
   int class_base_reg[class_count];
   int class_reg_count[class_count];
   int classes[class_count];

   for (int i = 0; i < class_count; i++) {
      class_base_reg[i] = ra_reg_count;
      class_reg_count[i] = base_reg_count - (class_sizes[i] - 1);
      ra_reg_count += class_reg_count[i];
   }

   struct ra_regs *regs = ra_alloc_reg_set(ra_reg_count);
   for (int i = 0; i < class_count; i++) {
      classes[i] = ra_alloc_reg_class(regs);

      for (int i_r = 0; i_r < class_reg_count[i]; i_r++) {
	 ra_class_add_reg(regs, classes[i], class_base_reg[i] + i_r);
      }

      /* Add conflicts between our contiguous registers aliasing
       * base regs and other register classes' contiguous registers
       * that alias base regs, or the base regs themselves for classes[0].
       */
      for (int c = 0; c <= i; c++) {
	 for (int i_r = 0; i_r < class_reg_count[i] - 1; i_r++) {
	    for (int c_r = MAX2(0, i_r - (class_sizes[c] - 1));
		 c_r <= MIN2(class_reg_count[c] - 1, i_r + class_sizes[i] - 1);
		 c_r++) {

	       if (0) {
		  printf("%d/%d conflicts %d/%d\n",
			 class_sizes[i], i_r,
			 class_sizes[c], c_r);
	       }

	       ra_add_reg_conflict(regs,
				   class_base_reg[i] + i_r,
				   class_base_reg[c] + c_r);
	    }
	 }
      }
   }

   ra_set_finalize(regs);

   struct ra_graph *g = ra_alloc_interference_graph(regs,
						    this->virtual_grf_next);
   /* Node 0 is just a placeholder to keep virtual_grf[] mapping 1:1
    * with nodes.
    */
   ra_set_node_class(g, 0, classes[0]);

   /* FINISHME: Proper interference (live interval analysis) */
   for (int i = 1; i < this->virtual_grf_next; i++) {
      for (int c = 0; c < class_count; c++) {
	 if (class_sizes[c] == this->virtual_grf_sizes[i]) {
	    ra_set_node_class(g, i, classes[c]);
	    break;
	 }
      }

      for (int j = 1; j < i; j++) {
	 ra_add_node_interference(g, i, j);
      }
   }

   /* FINISHME: Handle spilling */
   if (!ra_allocate_no_spills(g)) {
      fprintf(stderr, "Failed to allocate registers.\n");
      this->fail = true;
      return;
   }

   /* Get the chosen virtual registers for each node, and map virtual
    * regs in the register classes back down to real hardware reg
    * numbers.
    */
   hw_reg_mapping[0] = 0; /* unused */
   for (int i = 1; i < this->virtual_grf_next; i++) {
      int reg = ra_get_node_reg(g, i);
      int hw_reg = -1;

      for (int c = 0; c < class_count; c++) {
	 if (reg >= class_base_reg[c] &&
	     reg < class_base_reg[c] + class_reg_count[c] - 1) {
	    hw_reg = reg - class_base_reg[c];
	    break;
	 }
      }

      assert(hw_reg != -1);
      hw_reg_mapping[i] = this->first_non_payload_grf + hw_reg;
      last_grf = MAX2(last_grf,
		      hw_reg_mapping[i] + this->virtual_grf_sizes[i] - 1);
   }

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      assign_reg(hw_reg_mapping, &inst->dst);
      assign_reg(hw_reg_mapping, &inst->src[0]);
      assign_reg(hw_reg_mapping, &inst->src[1]);
   }

   this->grf_used = last_grf + 1;

   talloc_free(g);
   talloc_free(regs);
}

static struct brw_reg brw_reg_from_fs_reg(fs_reg *reg)
{
   struct brw_reg brw_reg;

   switch (reg->file) {
   case GRF:
   case ARF:
   case MRF:
      brw_reg = brw_vec8_reg(reg->file,
			    reg->hw_reg, 0);
      brw_reg = retype(brw_reg, reg->type);
      break;
   case IMM:
      switch (reg->type) {
      case BRW_REGISTER_TYPE_F:
	 brw_reg = brw_imm_f(reg->imm.f);
	 break;
      case BRW_REGISTER_TYPE_D:
	 brw_reg = brw_imm_d(reg->imm.i);
	 break;
      case BRW_REGISTER_TYPE_UD:
	 brw_reg = brw_imm_ud(reg->imm.u);
	 break;
      default:
	 assert(!"not reached");
	 break;
      }
      break;
   case FIXED_HW_REG:
      brw_reg = reg->fixed_hw_reg;
      break;
   case BAD_FILE:
      /* Probably unused. */
      brw_reg = brw_null_reg();
      break;
   case UNIFORM:
      assert(!"not reached");
      brw_reg = brw_null_reg();
      break;
   }
   if (reg->abs)
      brw_reg = brw_abs(brw_reg);
   if (reg->negate)
      brw_reg = negate(brw_reg);

   return brw_reg;
}

void
fs_visitor::generate_code()
{
   unsigned int annotation_len = 0;
   int last_native_inst = 0;
   struct brw_instruction *if_stack[16], *loop_stack[16];
   int if_stack_depth = 0, loop_stack_depth = 0;
   int if_depth_in_loop[16];

   if_depth_in_loop[loop_stack_depth] = 0;

   memset(&if_stack, 0, sizeof(if_stack));
   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();
      struct brw_reg src[3], dst;

      for (unsigned int i = 0; i < 3; i++) {
	 src[i] = brw_reg_from_fs_reg(&inst->src[i]);
      }
      dst = brw_reg_from_fs_reg(&inst->dst);

      brw_set_conditionalmod(p, inst->conditional_mod);
      brw_set_predicate_control(p, inst->predicated);

      switch (inst->opcode) {
      case BRW_OPCODE_MOV:
	 brw_MOV(p, dst, src[0]);
	 break;
      case BRW_OPCODE_ADD:
	 brw_ADD(p, dst, src[0], src[1]);
	 break;
      case BRW_OPCODE_MUL:
	 brw_MUL(p, dst, src[0], src[1]);
	 break;

      case BRW_OPCODE_FRC:
	 brw_FRC(p, dst, src[0]);
	 break;
      case BRW_OPCODE_RNDD:
	 brw_RNDD(p, dst, src[0]);
	 break;
      case BRW_OPCODE_RNDZ:
	 brw_RNDZ(p, dst, src[0]);
	 break;

      case BRW_OPCODE_AND:
	 brw_AND(p, dst, src[0], src[1]);
	 break;
      case BRW_OPCODE_OR:
	 brw_OR(p, dst, src[0], src[1]);
	 break;
      case BRW_OPCODE_XOR:
	 brw_XOR(p, dst, src[0], src[1]);
	 break;

      case BRW_OPCODE_CMP:
	 brw_CMP(p, dst, inst->conditional_mod, src[0], src[1]);
	 break;
      case BRW_OPCODE_SEL:
	 brw_SEL(p, dst, src[0], src[1]);
	 break;

      case BRW_OPCODE_IF:
	 assert(if_stack_depth < 16);
	 if_stack[if_stack_depth] = brw_IF(p, BRW_EXECUTE_8);
	 if_depth_in_loop[loop_stack_depth]++;
	 if_stack_depth++;
	 break;
      case BRW_OPCODE_ELSE:
	 if_stack[if_stack_depth - 1] =
	    brw_ELSE(p, if_stack[if_stack_depth - 1]);
	 break;
      case BRW_OPCODE_ENDIF:
	 if_stack_depth--;
	 brw_ENDIF(p , if_stack[if_stack_depth]);
	 if_depth_in_loop[loop_stack_depth]--;
	 break;

      case BRW_OPCODE_DO:
	 loop_stack[loop_stack_depth++] = brw_DO(p, BRW_EXECUTE_8);
	 if_depth_in_loop[loop_stack_depth] = 0;
	 break;

      case BRW_OPCODE_BREAK:
	 brw_BREAK(p, if_depth_in_loop[loop_stack_depth]);
	 brw_set_predicate_control(p, BRW_PREDICATE_NONE);
	 break;
      case BRW_OPCODE_CONTINUE:
	 brw_CONT(p, if_depth_in_loop[loop_stack_depth]);
	 brw_set_predicate_control(p, BRW_PREDICATE_NONE);
	 break;

      case BRW_OPCODE_WHILE: {
	 struct brw_instruction *inst0, *inst1;
	 GLuint br = 1;

	 if (intel->gen == 5)
	    br = 2;

	 assert(loop_stack_depth > 0);
	 loop_stack_depth--;
	 inst0 = inst1 = brw_WHILE(p, loop_stack[loop_stack_depth]);
	 /* patch all the BREAK/CONT instructions from last BGNLOOP */
	 while (inst0 > loop_stack[loop_stack_depth]) {
	    inst0--;
	    if (inst0->header.opcode == BRW_OPCODE_BREAK &&
		inst0->bits3.if_else.jump_count == 0) {
	       inst0->bits3.if_else.jump_count = br * (inst1 - inst0 + 1);
	    }
	    else if (inst0->header.opcode == BRW_OPCODE_CONTINUE &&
		     inst0->bits3.if_else.jump_count == 0) {
	       inst0->bits3.if_else.jump_count = br * (inst1 - inst0);
	    }
	 }
      }
	 break;

      case FS_OPCODE_RCP:
      case FS_OPCODE_RSQ:
      case FS_OPCODE_SQRT:
      case FS_OPCODE_EXP2:
      case FS_OPCODE_LOG2:
      case FS_OPCODE_POW:
      case FS_OPCODE_SIN:
      case FS_OPCODE_COS:
	 generate_math(inst, dst, src);
	 break;
      case FS_OPCODE_LINTERP:
	 generate_linterp(inst, dst, src);
	 break;
      case FS_OPCODE_TEX:
      case FS_OPCODE_TXB:
      case FS_OPCODE_TXL:
	 generate_tex(inst, dst, src[0]);
	 break;
      case FS_OPCODE_DISCARD:
	 generate_discard(inst, dst /* src0 == dst */);
	 break;
      case FS_OPCODE_DDX:
	 generate_ddx(inst, dst, src[0]);
	 break;
      case FS_OPCODE_DDY:
	 generate_ddy(inst, dst, src[0]);
	 break;
      case FS_OPCODE_FB_WRITE:
	 generate_fb_write(inst);
	 break;
      default:
	 if (inst->opcode < (int)ARRAY_SIZE(brw_opcodes)) {
	    _mesa_problem(ctx, "Unsupported opcode `%s' in FS",
			  brw_opcodes[inst->opcode].name);
	 } else {
	    _mesa_problem(ctx, "Unsupported opcode %d in FS", inst->opcode);
	 }
	 this->fail = true;
      }

      if (annotation_len < p->nr_insn) {
	 annotation_len *= 2;
	 if (annotation_len < 16)
	    annotation_len = 16;

	 this->annotation_string = talloc_realloc(this->mem_ctx,
						  annotation_string,
						  const char *,
						  annotation_len);
	 this->annotation_ir = talloc_realloc(this->mem_ctx,
					      annotation_ir,
					      ir_instruction *,
					      annotation_len);
      }

      for (unsigned int i = last_native_inst; i < p->nr_insn; i++) {
	 this->annotation_string[i] = inst->annotation;
	 this->annotation_ir[i] = inst->ir;
      }
      last_native_inst = p->nr_insn;
   }
}

GLboolean
brw_wm_fs_emit(struct brw_context *brw, struct brw_wm_compile *c)
{
   struct brw_compile *p = &c->func;
   struct intel_context *intel = &brw->intel;
   GLcontext *ctx = &intel->ctx;
   struct brw_shader *shader = NULL;
   struct gl_shader_program *prog = ctx->Shader.CurrentProgram;

   if (!prog)
      return GL_FALSE;

   if (!using_new_fs)
      return GL_FALSE;

   for (unsigned int i = 0; i < prog->_NumLinkedShaders; i++) {
      if (prog->_LinkedShaders[i]->Type == GL_FRAGMENT_SHADER) {
	 shader = (struct brw_shader *)prog->_LinkedShaders[i];
	 break;
      }
   }
   if (!shader)
      return GL_FALSE;

   /* We always use 8-wide mode, at least for now.  For one, flow
    * control only works in 8-wide.  Also, when we're fragment shader
    * bound, we're almost always under register pressure as well, so
    * 8-wide would save us from the performance cliff of spilling
    * regs.
    */
   c->dispatch_width = 8;

   if (INTEL_DEBUG & DEBUG_WM) {
      printf("GLSL IR for native fragment shader %d:\n", prog->Name);
      _mesa_print_ir(shader->ir, NULL);
      printf("\n");
   }

   /* Now the main event: Visit the shader IR and generate our FS IR for it.
    */
   fs_visitor v(c, shader);

   if (0) {
      v.emit_dummy_fs();
   } else {
      v.emit_interpolation_setup();

      /* Generate FS IR for main().  (the visitor only descends into
       * functions called "main").
       */
      foreach_iter(exec_list_iterator, iter, *shader->ir) {
	 ir_instruction *ir = (ir_instruction *)iter.get();
	 v.base_ir = ir;
	 ir->accept(&v);
      }

      v.emit_fb_writes();
      v.assign_curb_setup();
      v.assign_urb_setup();
      if (0)
	 v.assign_regs_trivial();
      else
	 v.assign_regs();
   }

   v.generate_code();

   assert(!v.fail); /* FINISHME: Cleanly fail, tested at link time, etc. */

   if (v.fail)
      return GL_FALSE;

   if (INTEL_DEBUG & DEBUG_WM) {
      const char *last_annotation_string = NULL;
      ir_instruction *last_annotation_ir = NULL;

      printf("Native code for fragment shader %d:\n", prog->Name);
      for (unsigned int i = 0; i < p->nr_insn; i++) {
	 if (last_annotation_ir != v.annotation_ir[i]) {
	    last_annotation_ir = v.annotation_ir[i];
	    if (last_annotation_ir) {
	       printf("   ");
	       last_annotation_ir->print();
	       printf("\n");
	    }
	 }
	 if (last_annotation_string != v.annotation_string[i]) {
	    last_annotation_string = v.annotation_string[i];
	    if (last_annotation_string)
	       printf("   %s\n", last_annotation_string);
	 }
	 brw_disasm(stdout, &p->store[i], intel->gen);
      }
      printf("\n");
   }

   c->prog_data.total_grf = v.grf_used;
   c->prog_data.total_scratch = 0;

   return GL_TRUE;
}

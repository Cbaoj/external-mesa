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
 *
 * Authors:
 *    Jason Ekstrand (jason@jlekstrand.net)
 *
 */

#include "spirv_to_nir_private.h"
#include "nir_vla.h"

static struct vtn_ssa_value *
vtn_const_ssa_value(struct vtn_builder *b, nir_constant *constant,
                    const struct glsl_type *type)
{
   struct hash_entry *entry = _mesa_hash_table_search(b->const_table, constant);

   if (entry)
      return entry->data;

   struct vtn_ssa_value *val = rzalloc(b, struct vtn_ssa_value);
   val->type = type;

   switch (glsl_get_base_type(type)) {
   case GLSL_TYPE_INT:
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_BOOL:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_DOUBLE:
      if (glsl_type_is_vector_or_scalar(type)) {
         unsigned num_components = glsl_get_vector_elements(val->type);
         nir_load_const_instr *load =
            nir_load_const_instr_create(b->shader, num_components);

         for (unsigned i = 0; i < num_components; i++)
            load->value.u[i] = constant->value.u[i];

         nir_instr_insert_before_cf_list(&b->impl->body, &load->instr);
         val->def = &load->def;
      } else {
         assert(glsl_type_is_matrix(type));
         unsigned rows = glsl_get_vector_elements(val->type);
         unsigned columns = glsl_get_matrix_columns(val->type);
         val->elems = ralloc_array(b, struct vtn_ssa_value *, columns);

         for (unsigned i = 0; i < columns; i++) {
            struct vtn_ssa_value *col_val = rzalloc(b, struct vtn_ssa_value);
            col_val->type = glsl_get_column_type(val->type);
            nir_load_const_instr *load =
               nir_load_const_instr_create(b->shader, rows);

            for (unsigned j = 0; j < rows; j++)
               load->value.u[j] = constant->value.u[rows * i + j];

            nir_instr_insert_before_cf_list(&b->impl->body, &load->instr);
            col_val->def = &load->def;

            val->elems[i] = col_val;
         }
      }
      break;

   case GLSL_TYPE_ARRAY: {
      unsigned elems = glsl_get_length(val->type);
      val->elems = ralloc_array(b, struct vtn_ssa_value *, elems);
      const struct glsl_type *elem_type = glsl_get_array_element(val->type);
      for (unsigned i = 0; i < elems; i++)
         val->elems[i] = vtn_const_ssa_value(b, constant->elements[i],
                                             elem_type);
      break;
   }

   case GLSL_TYPE_STRUCT: {
      unsigned elems = glsl_get_length(val->type);
      val->elems = ralloc_array(b, struct vtn_ssa_value *, elems);
      for (unsigned i = 0; i < elems; i++) {
         const struct glsl_type *elem_type =
            glsl_get_struct_field(val->type, i);
         val->elems[i] = vtn_const_ssa_value(b, constant->elements[i],
                                             elem_type);
      }
      break;
   }

   default:
      unreachable("bad constant type");
   }

   return val;
}

struct vtn_ssa_value *
vtn_ssa_value(struct vtn_builder *b, uint32_t value_id)
{
   struct vtn_value *val = vtn_untyped_value(b, value_id);
   switch (val->value_type) {
   case vtn_value_type_constant:
      return vtn_const_ssa_value(b, val->constant, val->const_type);

   case vtn_value_type_ssa:
      return val->ssa;
   default:
      unreachable("Invalid type for an SSA value");
   }
}

static char *
vtn_string_literal(struct vtn_builder *b, const uint32_t *words,
                   unsigned word_count)
{
   return ralloc_strndup(b, (char *)words, word_count * sizeof(*words));
}

static const uint32_t *
vtn_foreach_instruction(struct vtn_builder *b, const uint32_t *start,
                        const uint32_t *end, vtn_instruction_handler handler)
{
   const uint32_t *w = start;
   while (w < end) {
      SpvOp opcode = w[0] & SpvOpCodeMask;
      unsigned count = w[0] >> SpvWordCountShift;
      assert(count >= 1 && w + count <= end);

      if (!handler(b, opcode, w, count))
         return w;

      w += count;
   }
   assert(w == end);
   return w;
}

static void
vtn_handle_extension(struct vtn_builder *b, SpvOp opcode,
                     const uint32_t *w, unsigned count)
{
   switch (opcode) {
   case SpvOpExtInstImport: {
      struct vtn_value *val = vtn_push_value(b, w[1], vtn_value_type_extension);
      if (strcmp((const char *)&w[2], "GLSL.std.450") == 0) {
         val->ext_handler = vtn_handle_glsl450_instruction;
      } else {
         assert(!"Unsupported extension");
      }
      break;
   }

   case SpvOpExtInst: {
      struct vtn_value *val = vtn_value(b, w[3], vtn_value_type_extension);
      bool handled = val->ext_handler(b, w[4], w, count);
      (void)handled;
      assert(handled);
      break;
   }

   default:
      unreachable("Unhandled opcode");
   }
}

static void
_foreach_decoration_helper(struct vtn_builder *b,
                           struct vtn_value *base_value,
                           int member,
                           struct vtn_value *value,
                           vtn_decoration_foreach_cb cb, void *data)
{
   for (struct vtn_decoration *dec = value->decoration; dec; dec = dec->next) {
      if (dec->member >= 0) {
         assert(member == -1);
         member = dec->member;
      }

      if (dec->group) {
         assert(dec->group->value_type == vtn_value_type_decoration_group);
         _foreach_decoration_helper(b, base_value, member, dec->group, cb, data);
      } else {
         cb(b, base_value, member, dec, data);
      }
   }
}

/** Iterates (recursively if needed) over all of the decorations on a value
 *
 * This function iterates over all of the decorations applied to a given
 * value.  If it encounters a decoration group, it recurses into the group
 * and iterates over all of those decorations as well.
 */
void
vtn_foreach_decoration(struct vtn_builder *b, struct vtn_value *value,
                       vtn_decoration_foreach_cb cb, void *data)
{
   _foreach_decoration_helper(b, value, -1, value, cb, data);
}

static void
vtn_handle_decoration(struct vtn_builder *b, SpvOp opcode,
                      const uint32_t *w, unsigned count)
{
   const uint32_t *w_end = w + count;
   const uint32_t target = w[1];
   w += 2;

   int member = -1;
   switch (opcode) {
   case SpvOpDecorationGroup:
      vtn_push_value(b, target, vtn_value_type_undef);
      break;

   case SpvOpMemberDecorate:
      member = *(w++);
      /* fallthrough */
   case SpvOpDecorate: {
      struct vtn_value *val = &b->values[target];

      struct vtn_decoration *dec = rzalloc(b, struct vtn_decoration);
      dec->member = member;
      dec->decoration = *(w++);
      dec->literals = w;

      /* Link into the list */
      dec->next = val->decoration;
      val->decoration = dec;
      break;
   }

   case SpvOpGroupMemberDecorate:
      member = *(w++);
      /* fallthrough */
   case SpvOpGroupDecorate: {
      struct vtn_value *group = &b->values[target];
      assert(group->value_type == vtn_value_type_decoration_group);

      for (; w < w_end; w++) {
         struct vtn_value *val = &b->values[*w];
         struct vtn_decoration *dec = rzalloc(b, struct vtn_decoration);
         dec->member = member;
         dec->group = group;

         /* Link into the list */
         dec->next = val->decoration;
         val->decoration = dec;
      }
      break;
   }

   default:
      unreachable("Unhandled opcode");
   }
}

static void
struct_member_decoration_cb(struct vtn_builder *b,
                            struct vtn_value *val, int member,
                            const struct vtn_decoration *dec, void *void_fields)
{
   struct glsl_struct_field *fields = void_fields;

   if (member < 0)
      return;

   switch (dec->decoration) {
   case SpvDecorationPrecisionLow:
   case SpvDecorationPrecisionMedium:
   case SpvDecorationPrecisionHigh:
      break; /* FIXME: Do nothing with these for now. */
   case SpvDecorationSmooth:
      fields[member].interpolation = INTERP_QUALIFIER_SMOOTH;
      break;
   case SpvDecorationNoperspective:
      fields[member].interpolation = INTERP_QUALIFIER_NOPERSPECTIVE;
      break;
   case SpvDecorationFlat:
      fields[member].interpolation = INTERP_QUALIFIER_FLAT;
      break;
   case SpvDecorationCentroid:
      fields[member].centroid = true;
      break;
   case SpvDecorationSample:
      fields[member].sample = true;
      break;
   case SpvDecorationLocation:
      fields[member].location = dec->literals[0];
      break;
   default:
      unreachable("Unhandled member decoration");
   }
}

static void
vtn_handle_type(struct vtn_builder *b, SpvOp opcode,
                const uint32_t *w, unsigned count)
{
   struct vtn_value *val = vtn_push_value(b, w[1], vtn_value_type_type);

   switch (opcode) {
   case SpvOpTypeVoid:
      val->type = glsl_void_type();
      return;
   case SpvOpTypeBool:
      val->type = glsl_bool_type();
      return;
   case SpvOpTypeInt:
      val->type = glsl_int_type();
      return;
   case SpvOpTypeFloat:
      val->type = glsl_float_type();
      return;

   case SpvOpTypeVector: {
      const struct glsl_type *base =
         vtn_value(b, w[2], vtn_value_type_type)->type;
      unsigned elems = w[3];

      assert(glsl_type_is_scalar(base));
      val->type = glsl_vector_type(glsl_get_base_type(base), elems);
      return;
   }

   case SpvOpTypeMatrix: {
      const struct glsl_type *base =
         vtn_value(b, w[2], vtn_value_type_type)->type;
      unsigned columns = w[3];

      assert(glsl_type_is_vector(base));
      val->type = glsl_matrix_type(glsl_get_base_type(base),
                                  glsl_get_vector_elements(base),
                                  columns);
      return;
   }

   case SpvOpTypeArray:
      val->type = glsl_array_type(b->values[w[2]].type, w[3]);
      return;

   case SpvOpTypeStruct: {
      NIR_VLA(struct glsl_struct_field, fields, count);
      for (unsigned i = 0; i < count - 2; i++) {
         /* TODO: Handle decorators */
         fields[i].type = vtn_value(b, w[i + 2], vtn_value_type_type)->type;
         fields[i].name = ralloc_asprintf(b, "field%d", i);
         fields[i].location = -1;
         fields[i].interpolation = 0;
         fields[i].centroid = 0;
         fields[i].sample = 0;
         fields[i].matrix_layout = 2;
         fields[i].stream = -1;
      }

      vtn_foreach_decoration(b, val, struct_member_decoration_cb, fields);

      const char *name = val->name ? val->name : "struct";

      val->type = glsl_struct_type(fields, count, name);
      return;
   }

   case SpvOpTypeFunction: {
      const struct glsl_type *return_type = b->values[w[2]].type;
      NIR_VLA(struct glsl_function_param, params, count - 3);
      for (unsigned i = 0; i < count - 3; i++) {
         params[i].type = vtn_value(b, w[i + 3], vtn_value_type_type)->type;

         /* FIXME: */
         params[i].in = true;
         params[i].out = true;
      }
      val->type = glsl_function_type(return_type, params, count - 3);
      return;
   }

   case SpvOpTypePointer:
      /* FIXME:  For now, we'll just do the really lame thing and return
       * the same type.  The validator should ensure that the proper number
       * of dereferences happen
       */
      val->type = vtn_value(b, w[3], vtn_value_type_type)->type;
      return;

   case SpvOpTypeSampler: {
      const struct glsl_type *sampled_type =
         vtn_value(b, w[2], vtn_value_type_type)->type;

      assert(glsl_type_is_vector_or_scalar(sampled_type));

      enum glsl_sampler_dim dim;
      switch ((SpvDim)w[3]) {
      case SpvDim1D:       dim = GLSL_SAMPLER_DIM_1D;    break;
      case SpvDim2D:       dim = GLSL_SAMPLER_DIM_2D;    break;
      case SpvDim3D:       dim = GLSL_SAMPLER_DIM_3D;    break;
      case SpvDimCube:     dim = GLSL_SAMPLER_DIM_CUBE;  break;
      case SpvDimRect:     dim = GLSL_SAMPLER_DIM_RECT;  break;
      case SpvDimBuffer:   dim = GLSL_SAMPLER_DIM_BUF;   break;
      default:
         unreachable("Invalid SPIR-V Sampler dimension");
      }

      /* TODO: Handle the various texture image/filter options */
      (void)w[4];

      bool is_array = w[5];
      bool is_shadow = w[6];

      assert(w[7] == 0 && "FIXME: Handl multi-sampled textures");

      val->type = glsl_sampler_type(dim, is_shadow, is_array,
                                    glsl_get_base_type(sampled_type));
      return;
   }

   case SpvOpTypeRuntimeArray:
   case SpvOpTypeOpaque:
   case SpvOpTypeEvent:
   case SpvOpTypeDeviceEvent:
   case SpvOpTypeReserveId:
   case SpvOpTypeQueue:
   case SpvOpTypePipe:
   default:
      unreachable("Unhandled opcode");
   }
}

static void
vtn_handle_constant(struct vtn_builder *b, SpvOp opcode,
                    const uint32_t *w, unsigned count)
{
   struct vtn_value *val = vtn_push_value(b, w[2], vtn_value_type_constant);
   val->const_type = vtn_value(b, w[1], vtn_value_type_type)->type;
   val->constant = ralloc(b, nir_constant);
   switch (opcode) {
   case SpvOpConstantTrue:
      assert(val->const_type == glsl_bool_type());
      val->constant->value.u[0] = NIR_TRUE;
      break;
   case SpvOpConstantFalse:
      assert(val->const_type == glsl_bool_type());
      val->constant->value.u[0] = NIR_FALSE;
      break;
   case SpvOpConstant:
      assert(glsl_type_is_scalar(val->const_type));
      val->constant->value.u[0] = w[3];
      break;
   case SpvOpConstantComposite: {
      unsigned elem_count = count - 3;
      nir_constant **elems = ralloc_array(b, nir_constant *, elem_count);
      for (unsigned i = 0; i < elem_count; i++)
         elems[i] = vtn_value(b, w[i + 3], vtn_value_type_constant)->constant;

      switch (glsl_get_base_type(val->const_type)) {
      case GLSL_TYPE_UINT:
      case GLSL_TYPE_INT:
      case GLSL_TYPE_FLOAT:
      case GLSL_TYPE_BOOL:
         if (glsl_type_is_matrix(val->const_type)) {
            unsigned rows = glsl_get_vector_elements(val->const_type);
            assert(glsl_get_matrix_columns(val->const_type) == elem_count);
            for (unsigned i = 0; i < elem_count; i++)
               for (unsigned j = 0; j < rows; j++)
                  val->constant->value.u[rows * i + j] = elems[i]->value.u[j];
         } else {
            assert(glsl_type_is_vector(val->const_type));
            assert(glsl_get_vector_elements(val->const_type) == elem_count);
            for (unsigned i = 0; i < elem_count; i++)
               val->constant->value.u[i] = elems[i]->value.u[0];
         }
         ralloc_free(elems);
         break;

      case GLSL_TYPE_STRUCT:
      case GLSL_TYPE_ARRAY:
         ralloc_steal(val->constant, elems);
         val->constant->elements = elems;
         break;

      default:
         unreachable("Unsupported type for constants");
      }
      break;
   }

   default:
      unreachable("Unhandled opcode");
   }
}

static void
var_decoration_cb(struct vtn_builder *b, struct vtn_value *val, int member,
                  const struct vtn_decoration *dec, void *void_var)
{
   assert(val->value_type == vtn_value_type_deref);
   assert(val->deref->deref.child == NULL);
   assert(val->deref->var == void_var);

   nir_variable *var = void_var;
   switch (dec->decoration) {
   case SpvDecorationPrecisionLow:
   case SpvDecorationPrecisionMedium:
   case SpvDecorationPrecisionHigh:
      break; /* FIXME: Do nothing with these for now. */
   case SpvDecorationSmooth:
      var->data.interpolation = INTERP_QUALIFIER_SMOOTH;
      break;
   case SpvDecorationNoperspective:
      var->data.interpolation = INTERP_QUALIFIER_NOPERSPECTIVE;
      break;
   case SpvDecorationFlat:
      var->data.interpolation = INTERP_QUALIFIER_FLAT;
      break;
   case SpvDecorationCentroid:
      var->data.centroid = true;
      break;
   case SpvDecorationSample:
      var->data.sample = true;
      break;
   case SpvDecorationInvariant:
      var->data.invariant = true;
      break;
   case SpvDecorationConstant:
      assert(var->constant_initializer != NULL);
      var->data.read_only = true;
      break;
   case SpvDecorationNonwritable:
      var->data.read_only = true;
      break;
   case SpvDecorationLocation:
      var->data.explicit_location = true;
      var->data.location = dec->literals[0];
      break;
   case SpvDecorationComponent:
      var->data.location_frac = dec->literals[0];
      break;
   case SpvDecorationIndex:
      var->data.explicit_index = true;
      var->data.index = dec->literals[0];
      break;
   case SpvDecorationBinding:
      var->data.explicit_binding = true;
      var->data.binding = dec->literals[0];
      break;
   case SpvDecorationDescriptorSet:
      var->data.descriptor_set = dec->literals[0];
      break;
   case SpvDecorationBuiltIn:
      var->data.mode = nir_var_system_value;
      var->data.read_only = true;
      switch ((SpvBuiltIn)dec->literals[0]) {
      case SpvBuiltInFrontFacing:
         var->data.location = SYSTEM_VALUE_FRONT_FACE;
         break;
      case SpvBuiltInVertexId:
         var->data.location = SYSTEM_VALUE_VERTEX_ID;
         break;
      case SpvBuiltInInstanceId:
         var->data.location = SYSTEM_VALUE_INSTANCE_ID;
         break;
      case SpvBuiltInSampleId:
         var->data.location = SYSTEM_VALUE_SAMPLE_ID;
         break;
      case SpvBuiltInSamplePosition:
         var->data.location = SYSTEM_VALUE_SAMPLE_POS;
         break;
      case SpvBuiltInSampleMask:
         var->data.location = SYSTEM_VALUE_SAMPLE_MASK_IN;
         break;
      case SpvBuiltInInvocationId:
         var->data.location = SYSTEM_VALUE_INVOCATION_ID;
         break;
      case SpvBuiltInPrimitiveId:
      case SpvBuiltInPosition:
      case SpvBuiltInPointSize:
      case SpvBuiltInClipVertex:
      case SpvBuiltInClipDistance:
      case SpvBuiltInCullDistance:
      case SpvBuiltInLayer:
      case SpvBuiltInViewportIndex:
      case SpvBuiltInTessLevelOuter:
      case SpvBuiltInTessLevelInner:
      case SpvBuiltInTessCoord:
      case SpvBuiltInPatchVertices:
      case SpvBuiltInFragCoord:
      case SpvBuiltInPointCoord:
      case SpvBuiltInFragColor:
      case SpvBuiltInFragDepth:
      case SpvBuiltInHelperInvocation:
      case SpvBuiltInNumWorkgroups:
      case SpvBuiltInWorkgroupSize:
      case SpvBuiltInWorkgroupId:
      case SpvBuiltInLocalInvocationId:
      case SpvBuiltInGlobalInvocationId:
      case SpvBuiltInLocalInvocationIndex:
      case SpvBuiltInWorkDim:
      case SpvBuiltInGlobalSize:
      case SpvBuiltInEnqueuedWorkgroupSize:
      case SpvBuiltInGlobalOffset:
      case SpvBuiltInGlobalLinearId:
      case SpvBuiltInWorkgroupLinearId:
      case SpvBuiltInSubgroupSize:
      case SpvBuiltInSubgroupMaxSize:
      case SpvBuiltInNumSubgroups:
      case SpvBuiltInNumEnqueuedSubgroups:
      case SpvBuiltInSubgroupId:
      case SpvBuiltInSubgroupLocalInvocationId:
         unreachable("Unhandled builtin enum");
      }
      break;
   case SpvDecorationNoStaticUse:
      /* This can safely be ignored */
      break;
   case SpvDecorationBlock:
   case SpvDecorationBufferBlock:
   case SpvDecorationRowMajor:
   case SpvDecorationColMajor:
   case SpvDecorationGLSLShared:
   case SpvDecorationGLSLStd140:
   case SpvDecorationGLSLStd430:
   case SpvDecorationGLSLPacked:
   case SpvDecorationPatch:
   case SpvDecorationRestrict:
   case SpvDecorationAliased:
   case SpvDecorationVolatile:
   case SpvDecorationCoherent:
   case SpvDecorationNonreadable:
   case SpvDecorationUniform:
      /* This is really nice but we have no use for it right now. */
   case SpvDecorationCPacked:
   case SpvDecorationSaturatedConversion:
   case SpvDecorationStream:
   case SpvDecorationOffset:
   case SpvDecorationAlignment:
   case SpvDecorationXfbBuffer:
   case SpvDecorationStride:
   case SpvDecorationFuncParamAttr:
   case SpvDecorationFPRoundingMode:
   case SpvDecorationFPFastMathMode:
   case SpvDecorationLinkageAttributes:
   case SpvDecorationSpecId:
      break;
   default:
      unreachable("Unhandled variable decoration");
   }
}

static struct vtn_ssa_value *
_vtn_variable_load(struct vtn_builder *b,
                   nir_deref_var *src_deref, nir_deref *src_deref_tail)
{
   struct vtn_ssa_value *val = rzalloc(b, struct vtn_ssa_value);
   val->type = src_deref_tail->type;

   /* The deref tail may contain a deref to select a component of a vector (in
    * other words, it might not be an actual tail) so we have to save it away
    * here since we overwrite it later.
    */
   nir_deref *old_child = src_deref_tail->child;

   if (glsl_type_is_vector_or_scalar(val->type)) {
      nir_intrinsic_instr *load =
         nir_intrinsic_instr_create(b->shader, nir_intrinsic_load_var);
      load->variables[0] =
         nir_deref_as_var(nir_copy_deref(load, &src_deref->deref));
      load->num_components = glsl_get_vector_elements(val->type);
      nir_ssa_dest_init(&load->instr, &load->dest, load->num_components, NULL);

      nir_builder_instr_insert(&b->nb, &load->instr);

      if (src_deref->var->data.mode == nir_var_uniform &&
          glsl_get_base_type(val->type) == GLSL_TYPE_BOOL) {
         /* Uniform boolean loads need to be fixed up since they're defined
          * to be zero/nonzero rather than NIR_FALSE/NIR_TRUE.
          */
         val->def = nir_ine(&b->nb, &load->dest.ssa, nir_imm_int(&b->nb, 0));
      } else {
         val->def = &load->dest.ssa;
      }
   } else if (glsl_get_base_type(val->type) == GLSL_TYPE_ARRAY ||
              glsl_type_is_matrix(val->type)) {
      unsigned elems = glsl_get_length(val->type);
      val->elems = ralloc_array(b, struct vtn_ssa_value *, elems);

      nir_deref_array *deref = nir_deref_array_create(b);
      deref->deref_array_type = nir_deref_array_type_direct;
      deref->deref.type = glsl_get_array_element(val->type);
      src_deref_tail->child = &deref->deref;
      for (unsigned i = 0; i < elems; i++) {
         deref->base_offset = i;
         val->elems[i] = _vtn_variable_load(b, src_deref, &deref->deref);
      }
   } else {
      assert(glsl_get_base_type(val->type) == GLSL_TYPE_STRUCT);
      unsigned elems = glsl_get_length(val->type);
      val->elems = ralloc_array(b, struct vtn_ssa_value *, elems);

      nir_deref_struct *deref = nir_deref_struct_create(b, 0);
      src_deref_tail->child = &deref->deref;
      for (unsigned i = 0; i < elems; i++) {
         deref->index = i;
         deref->deref.type = glsl_get_struct_field(val->type, i);
         val->elems[i] = _vtn_variable_load(b, src_deref, &deref->deref);
      }
   }

   src_deref_tail->child = old_child;

   return val;
}

static void
_vtn_variable_store(struct vtn_builder *b, nir_deref_var *dest_deref,
                    nir_deref *dest_deref_tail, struct vtn_ssa_value *src)
{
   nir_deref *old_child = dest_deref_tail->child;

   if (glsl_type_is_vector_or_scalar(src->type)) {
      nir_intrinsic_instr *store =
         nir_intrinsic_instr_create(b->shader, nir_intrinsic_store_var);
      store->variables[0] =
         nir_deref_as_var(nir_copy_deref(store, &dest_deref->deref));
      store->src[0] = nir_src_for_ssa(src->def);

      nir_builder_instr_insert(&b->nb, &store->instr);
   } else if (glsl_get_base_type(src->type) == GLSL_TYPE_ARRAY ||
              glsl_type_is_matrix(src->type)) {
      unsigned elems = glsl_get_length(src->type);

      nir_deref_array *deref = nir_deref_array_create(b);
      deref->deref_array_type = nir_deref_array_type_direct;
      deref->deref.type = glsl_get_array_element(src->type);
      dest_deref_tail->child = &deref->deref;
      for (unsigned i = 0; i < elems; i++) {
         deref->base_offset = i;
         _vtn_variable_store(b, dest_deref, &deref->deref, src->elems[i]);
      }
   } else {
      assert(glsl_get_base_type(src->type) == GLSL_TYPE_STRUCT);
      unsigned elems = glsl_get_length(src->type);

      nir_deref_struct *deref = nir_deref_struct_create(b, 0);
      dest_deref_tail->child = &deref->deref;
      for (unsigned i = 0; i < elems; i++) {
         deref->index = i;
         deref->deref.type = glsl_get_struct_field(src->type, i);
         _vtn_variable_store(b, dest_deref, &deref->deref, src->elems[i]);
      }
   }

   dest_deref_tail->child = old_child;
}

/*
 * Gets the NIR-level deref tail, which may have as a child an array deref
 * selecting which component due to OpAccessChain supporting per-component
 * indexing in SPIR-V.
 */

static nir_deref *
get_deref_tail(nir_deref_var *deref)
{
   nir_deref *cur = &deref->deref;
   while (!glsl_type_is_vector_or_scalar(cur->type) && cur->child)
      cur = cur->child;

   return cur;
}

static nir_ssa_def *vtn_vector_extract(struct vtn_builder *b,
                                       nir_ssa_def *src, unsigned index);

static nir_ssa_def *vtn_vector_extract_dynamic(struct vtn_builder *b,
                                               nir_ssa_def *src,
                                               nir_ssa_def *index);

static struct vtn_ssa_value *
vtn_variable_load(struct vtn_builder *b, nir_deref_var *src)
{
   nir_deref *src_tail = get_deref_tail(src);
   struct vtn_ssa_value *val = _vtn_variable_load(b, src, src_tail);

   if (src_tail->child) {
      nir_deref_array *vec_deref = nir_deref_as_array(src_tail->child);
      assert(vec_deref->deref.child == NULL);
      val->type = vec_deref->deref.type;
      if (vec_deref->deref_array_type == nir_deref_array_type_direct)
         val->def = vtn_vector_extract(b, val->def, vec_deref->base_offset);
      else
         val->def = vtn_vector_extract_dynamic(b, val->def,
                                               vec_deref->indirect.ssa);
   }

   return val;
}

static nir_ssa_def * vtn_vector_insert(struct vtn_builder *b,
                                       nir_ssa_def *src, nir_ssa_def *insert,
                                       unsigned index);

static nir_ssa_def * vtn_vector_insert_dynamic(struct vtn_builder *b,
                                               nir_ssa_def *src,
                                               nir_ssa_def *insert,
                                               nir_ssa_def *index);
static void
vtn_variable_store(struct vtn_builder *b, struct vtn_ssa_value *src,
                   nir_deref_var *dest)
{
   nir_deref *dest_tail = get_deref_tail(dest);
   if (dest_tail->child) {
      struct vtn_ssa_value *val = _vtn_variable_load(b, dest, dest_tail);
      nir_deref_array *deref = nir_deref_as_array(dest_tail->child);
      assert(deref->deref.child == NULL);
      if (deref->deref_array_type == nir_deref_array_type_direct)
         val->def = vtn_vector_insert(b, val->def, src->def,
                                      deref->base_offset);
      else
         val->def = vtn_vector_insert_dynamic(b, val->def, src->def,
                                              deref->indirect.ssa);
      _vtn_variable_store(b, dest, dest_tail, val);
   } else {
      _vtn_variable_store(b, dest, dest_tail, src);
   }
}

static void
vtn_variable_copy(struct vtn_builder *b, nir_deref_var *src,
                  nir_deref_var *dest)
{
   nir_deref *src_tail = get_deref_tail(src);

   if (src_tail->child) {
      assert(get_deref_tail(dest)->child);
      struct vtn_ssa_value *val = vtn_variable_load(b, src);
      vtn_variable_store(b, val, dest);
   } else {
      nir_intrinsic_instr *copy =
         nir_intrinsic_instr_create(b->shader, nir_intrinsic_copy_var);
      copy->variables[0] = nir_deref_as_var(nir_copy_deref(copy, &dest->deref));
      copy->variables[1] = nir_deref_as_var(nir_copy_deref(copy, &src->deref));

      nir_builder_instr_insert(&b->nb, &copy->instr);
   }
}

static void
vtn_handle_variables(struct vtn_builder *b, SpvOp opcode,
                     const uint32_t *w, unsigned count)
{
   switch (opcode) {
   case SpvOpVariable: {
      const struct glsl_type *type =
         vtn_value(b, w[1], vtn_value_type_type)->type;
      struct vtn_value *val = vtn_push_value(b, w[2], vtn_value_type_deref);

      nir_variable *var = ralloc(b->shader, nir_variable);

      var->type = type;
      var->name = ralloc_strdup(var, val->name);

      switch ((SpvStorageClass)w[3]) {
      case SpvStorageClassUniform:
      case SpvStorageClassUniformConstant:
         var->data.mode = nir_var_uniform;
         var->data.read_only = true;
         var->interface_type = type;
         break;
      case SpvStorageClassInput:
         var->data.mode = nir_var_shader_in;
         var->data.read_only = true;
         break;
      case SpvStorageClassOutput:
         var->data.mode = nir_var_shader_out;
         break;
      case SpvStorageClassPrivateGlobal:
         var->data.mode = nir_var_global;
         break;
      case SpvStorageClassFunction:
         var->data.mode = nir_var_local;
         break;
      case SpvStorageClassWorkgroupLocal:
      case SpvStorageClassWorkgroupGlobal:
      case SpvStorageClassGeneric:
      case SpvStorageClassPrivate:
      case SpvStorageClassAtomicCounter:
      default:
         unreachable("Unhandled variable storage class");
      }

      if (count > 4) {
         assert(count == 5);
         var->constant_initializer =
            vtn_value(b, w[4], vtn_value_type_constant)->constant;
      }

      val->deref = nir_deref_var_create(b, var);

      vtn_foreach_decoration(b, val, var_decoration_cb, var);

      if (b->execution_model == SpvExecutionModelFragment &&
          var->data.mode == nir_var_shader_out) {
         var->data.location += FRAG_RESULT_DATA0;
      } else if (b->execution_model == SpvExecutionModelVertex &&
                 var->data.mode == nir_var_shader_in) {
         var->data.location += VERT_ATTRIB_GENERIC0;
      } else if (var->data.mode == nir_var_shader_in ||
                 var->data.mode == nir_var_shader_out) {
         var->data.location += VARYING_SLOT_VAR0;
      }

      switch (var->data.mode) {
      case nir_var_shader_in:
         exec_list_push_tail(&b->shader->inputs, &var->node);
         break;
      case nir_var_shader_out:
         exec_list_push_tail(&b->shader->outputs, &var->node);
         break;
      case nir_var_global:
         exec_list_push_tail(&b->shader->globals, &var->node);
         break;
      case nir_var_local:
         exec_list_push_tail(&b->impl->locals, &var->node);
         break;
      case nir_var_uniform:
         exec_list_push_tail(&b->shader->uniforms, &var->node);
         break;
      case nir_var_system_value:
         exec_list_push_tail(&b->shader->system_values, &var->node);
         break;
      }
      break;
   }

   case SpvOpAccessChain:
   case SpvOpInBoundsAccessChain: {
      struct vtn_value *val = vtn_push_value(b, w[2], vtn_value_type_deref);
      nir_deref_var *base = vtn_value(b, w[3], vtn_value_type_deref)->deref;
      val->deref = nir_deref_as_var(nir_copy_deref(b, &base->deref));

      nir_deref *tail = &val->deref->deref;
      while (tail->child)
         tail = tail->child;

      for (unsigned i = 0; i < count - 4; i++) {
         assert(w[i + 4] < b->value_id_bound);
         struct vtn_value *idx_val = &b->values[w[i + 4]];

         enum glsl_base_type base_type = glsl_get_base_type(tail->type);
         switch (base_type) {
         case GLSL_TYPE_UINT:
         case GLSL_TYPE_INT:
         case GLSL_TYPE_FLOAT:
         case GLSL_TYPE_DOUBLE:
         case GLSL_TYPE_BOOL:
         case GLSL_TYPE_ARRAY: {
            nir_deref_array *deref_arr = nir_deref_array_create(b);
            if (base_type == GLSL_TYPE_ARRAY) {
               deref_arr->deref.type = glsl_get_array_element(tail->type);
            } else if (glsl_type_is_matrix(tail->type)) {
               deref_arr->deref.type = glsl_get_column_type(tail->type);
            } else {
               assert(glsl_type_is_vector(tail->type));
               deref_arr->deref.type = glsl_scalar_type(base_type);
            }

            if (idx_val->value_type == vtn_value_type_constant) {
               unsigned idx = idx_val->constant->value.u[0];
               deref_arr->deref_array_type = nir_deref_array_type_direct;
               deref_arr->base_offset = idx;
            } else {
               assert(idx_val->value_type == vtn_value_type_ssa);
               deref_arr->deref_array_type = nir_deref_array_type_indirect;
               deref_arr->base_offset = 0;
               deref_arr->indirect =
                  nir_src_for_ssa(vtn_ssa_value(b, w[1])->def);
            }
            tail->child = &deref_arr->deref;
            break;
         }

         case GLSL_TYPE_STRUCT: {
            assert(idx_val->value_type == vtn_value_type_constant);
            unsigned idx = idx_val->constant->value.u[0];
            nir_deref_struct *deref_struct = nir_deref_struct_create(b, idx);
            deref_struct->deref.type = glsl_get_struct_field(tail->type, idx);
            tail->child = &deref_struct->deref;
            break;
         }
         default:
            unreachable("Invalid type for deref");
         }
         tail = tail->child;
      }
      break;
   }

   case SpvOpCopyMemory: {
      nir_deref_var *dest = vtn_value(b, w[1], vtn_value_type_deref)->deref;
      nir_deref_var *src = vtn_value(b, w[2], vtn_value_type_deref)->deref;

      vtn_variable_copy(b, src, dest);
      break;
   }

   case SpvOpLoad: {
      nir_deref_var *src = vtn_value(b, w[3], vtn_value_type_deref)->deref;
      const struct glsl_type *src_type = nir_deref_tail(&src->deref)->type;

      if (glsl_get_base_type(src_type) == GLSL_TYPE_SAMPLER) {
         vtn_push_value(b, w[2], vtn_value_type_deref)->deref = src;
         return;
      }

      struct vtn_value *val = vtn_push_value(b, w[2], vtn_value_type_ssa);
      val->ssa = vtn_variable_load(b, src);
      break;
   }

   case SpvOpStore: {
      nir_deref_var *dest = vtn_value(b, w[1], vtn_value_type_deref)->deref;
      struct vtn_ssa_value *src = vtn_ssa_value(b, w[2]);
      vtn_variable_store(b, src, dest);
      break;
   }

   case SpvOpVariableArray:
   case SpvOpCopyMemorySized:
   case SpvOpArrayLength:
   case SpvOpImagePointer:
   default:
      unreachable("Unhandled opcode");
   }
}

static void
vtn_handle_function_call(struct vtn_builder *b, SpvOp opcode,
                         const uint32_t *w, unsigned count)
{
   unreachable("Unhandled opcode");
}

static struct vtn_ssa_value *
vtn_create_ssa_value(struct vtn_builder *b, const struct glsl_type *type)
{
   struct vtn_ssa_value *val = rzalloc(b, struct vtn_ssa_value);
   val->type = type;
   
   if (!glsl_type_is_vector_or_scalar(type)) {
      unsigned elems = glsl_get_length(type);
      val->elems = ralloc_array(b, struct vtn_ssa_value *, elems);
      for (unsigned i = 0; i < elems; i++) {
         const struct glsl_type *child_type;

         switch (glsl_get_base_type(type)) {
         case GLSL_TYPE_INT:
         case GLSL_TYPE_UINT:
         case GLSL_TYPE_BOOL:
         case GLSL_TYPE_FLOAT:
         case GLSL_TYPE_DOUBLE:
            child_type = glsl_get_column_type(type);
            break;
         case GLSL_TYPE_ARRAY:
            child_type = glsl_get_array_element(type);
            break;
         case GLSL_TYPE_STRUCT:
            child_type = glsl_get_struct_field(type, i);
            break;
         default:
            unreachable("unkown base type");
         }

         val->elems[i] = vtn_create_ssa_value(b, child_type);
      }
   }

   return val;
}

static nir_tex_src
vtn_tex_src(struct vtn_builder *b, unsigned index, nir_tex_src_type type)
{
   nir_tex_src src;
   src.src = nir_src_for_ssa(vtn_value(b, index, vtn_value_type_ssa)->ssa->def);
   src.src_type = type;
   return src;
}

static void
vtn_handle_texture(struct vtn_builder *b, SpvOp opcode,
                   const uint32_t *w, unsigned count)
{
   struct vtn_value *val = vtn_push_value(b, w[2], vtn_value_type_ssa);
   nir_deref_var *sampler = vtn_value(b, w[3], vtn_value_type_deref)->deref;

   nir_tex_src srcs[8]; /* 8 should be enough */
   nir_tex_src *p = srcs;

   unsigned coord_components = 0;
   switch (opcode) {
   case SpvOpTextureSample:
   case SpvOpTextureSampleDref:
   case SpvOpTextureSampleLod:
   case SpvOpTextureSampleProj:
   case SpvOpTextureSampleGrad:
   case SpvOpTextureSampleOffset:
   case SpvOpTextureSampleProjLod:
   case SpvOpTextureSampleProjGrad:
   case SpvOpTextureSampleLodOffset:
   case SpvOpTextureSampleProjOffset:
   case SpvOpTextureSampleGradOffset:
   case SpvOpTextureSampleProjLodOffset:
   case SpvOpTextureSampleProjGradOffset:
   case SpvOpTextureFetchTexelLod:
   case SpvOpTextureFetchTexelOffset:
   case SpvOpTextureFetchSample:
   case SpvOpTextureFetchTexel:
   case SpvOpTextureGather:
   case SpvOpTextureGatherOffset:
   case SpvOpTextureGatherOffsets:
   case SpvOpTextureQueryLod: {
      /* All these types have the coordinate as their first real argument */
      struct vtn_ssa_value *coord = vtn_ssa_value(b, w[4]);
      coord_components = glsl_get_vector_elements(coord->type);
      p->src = nir_src_for_ssa(coord->def);
      p->src_type = nir_tex_src_coord;
      p++;
      break;
   }

   default:
      break;
   }

   nir_texop texop;
   switch (opcode) {
   case SpvOpTextureSample:
      texop = nir_texop_tex;

      if (count == 6) {
         texop = nir_texop_txb;
         *p++ = vtn_tex_src(b, w[5], nir_tex_src_bias);
      }
      break;

   case SpvOpTextureSampleDref:
   case SpvOpTextureSampleLod:
   case SpvOpTextureSampleProj:
   case SpvOpTextureSampleGrad:
   case SpvOpTextureSampleOffset:
   case SpvOpTextureSampleProjLod:
   case SpvOpTextureSampleProjGrad:
   case SpvOpTextureSampleLodOffset:
   case SpvOpTextureSampleProjOffset:
   case SpvOpTextureSampleGradOffset:
   case SpvOpTextureSampleProjLodOffset:
   case SpvOpTextureSampleProjGradOffset:
   case SpvOpTextureFetchTexelLod:
   case SpvOpTextureFetchTexelOffset:
   case SpvOpTextureFetchSample:
   case SpvOpTextureFetchTexel:
   case SpvOpTextureGather:
   case SpvOpTextureGatherOffset:
   case SpvOpTextureGatherOffsets:
   case SpvOpTextureQuerySizeLod:
   case SpvOpTextureQuerySize:
   case SpvOpTextureQueryLod:
   case SpvOpTextureQueryLevels:
   case SpvOpTextureQuerySamples:
   default:
      unreachable("Unhandled opcode");
   }

   nir_tex_instr *instr = nir_tex_instr_create(b->shader, p - srcs);

   const struct glsl_type *sampler_type = nir_deref_tail(&sampler->deref)->type;
   instr->sampler_dim = glsl_get_sampler_dim(sampler_type);

   switch (glsl_get_sampler_result_type(sampler_type)) {
   case GLSL_TYPE_FLOAT:   instr->dest_type = nir_type_float;     break;
   case GLSL_TYPE_INT:     instr->dest_type = nir_type_int;       break;
   case GLSL_TYPE_UINT:    instr->dest_type = nir_type_unsigned;  break;
   case GLSL_TYPE_BOOL:    instr->dest_type = nir_type_bool;      break;
   default:
      unreachable("Invalid base type for sampler result");
   }

   instr->op = texop;
   memcpy(instr->src, srcs, instr->num_srcs * sizeof(*instr->src));
   instr->coord_components = coord_components;
   instr->is_array = glsl_sampler_type_is_array(sampler_type);
   instr->is_shadow = glsl_sampler_type_is_shadow(sampler_type);

   instr->sampler = nir_deref_as_var(nir_copy_deref(instr, &sampler->deref));

   nir_ssa_dest_init(&instr->instr, &instr->dest, 4, NULL);
   val->ssa = vtn_create_ssa_value(b, glsl_vector_type(GLSL_TYPE_FLOAT, 4));
   val->ssa->def = &instr->dest.ssa;

   nir_builder_instr_insert(&b->nb, &instr->instr);
}


static nir_alu_instr *
create_vec(void *mem_ctx, unsigned num_components)
{
   nir_op op;
   switch (num_components) {
   case 1: op = nir_op_fmov; break;
   case 2: op = nir_op_vec2; break;
   case 3: op = nir_op_vec3; break;
   case 4: op = nir_op_vec4; break;
   default: unreachable("bad vector size");
   }

   nir_alu_instr *vec = nir_alu_instr_create(mem_ctx, op);
   nir_ssa_dest_init(&vec->instr, &vec->dest.dest, num_components, NULL);

   return vec;
}

static struct vtn_ssa_value *
vtn_transpose(struct vtn_builder *b, struct vtn_ssa_value *src)
{
   if (src->transposed)
      return src->transposed;

   struct vtn_ssa_value *dest =
      vtn_create_ssa_value(b, glsl_transposed_type(src->type));

   for (unsigned i = 0; i < glsl_get_matrix_columns(dest->type); i++) {
      nir_alu_instr *vec = create_vec(b, glsl_get_matrix_columns(src->type));
      if (glsl_type_is_vector_or_scalar(src->type)) {
          vec->src[0].src = nir_src_for_ssa(src->def);
          vec->src[0].swizzle[0] = i;
      } else {
         for (unsigned j = 0; j < glsl_get_matrix_columns(src->type); j++) {
            vec->src[j].src = nir_src_for_ssa(src->elems[j]->def);
            vec->src[j].swizzle[0] = i;
         }
      }
      nir_builder_instr_insert(&b->nb, &vec->instr);
      dest->elems[i]->def = &vec->dest.dest.ssa;
   }

   dest->transposed = src;

   return dest;
}

/*
 * Normally, column vectors in SPIR-V correspond to a single NIR SSA
 * definition. But for matrix multiplies, we want to do one routine for
 * multiplying a matrix by a matrix and then pretend that vectors are matrices
 * with one column. So we "wrap" these things, and unwrap the result before we
 * send it off.
 */

static struct vtn_ssa_value *
vtn_wrap_matrix(struct vtn_builder *b, struct vtn_ssa_value *val)
{
   if (val == NULL)
      return NULL;

   if (glsl_type_is_matrix(val->type))
      return val;

   struct vtn_ssa_value *dest = rzalloc(b, struct vtn_ssa_value);
   dest->type = val->type;
   dest->elems = ralloc_array(b, struct vtn_ssa_value *, 1);
   dest->elems[0] = val;

   return dest;
}

static struct vtn_ssa_value *
vtn_unwrap_matrix(struct vtn_ssa_value *val)
{
   if (glsl_type_is_matrix(val->type))
         return val;

   return val->elems[0];
}

static struct vtn_ssa_value *
vtn_matrix_multiply(struct vtn_builder *b,
                    struct vtn_ssa_value *_src0, struct vtn_ssa_value *_src1)
{

   struct vtn_ssa_value *src0 = vtn_wrap_matrix(b, _src0);
   struct vtn_ssa_value *src1 = vtn_wrap_matrix(b, _src1);
   struct vtn_ssa_value *src0_transpose = vtn_wrap_matrix(b, _src0->transposed);
   struct vtn_ssa_value *src1_transpose = vtn_wrap_matrix(b, _src1->transposed);

   unsigned src0_rows = glsl_get_vector_elements(src0->type);
   unsigned src0_columns = glsl_get_matrix_columns(src0->type);
   unsigned src1_columns = glsl_get_matrix_columns(src1->type);

   struct vtn_ssa_value *dest =
      vtn_create_ssa_value(b, glsl_matrix_type(glsl_get_base_type(src0->type),
                                               src0_rows, src1_columns));

   dest = vtn_wrap_matrix(b, dest);

   bool transpose_result = false;
   if (src0_transpose && src1_transpose) {
      /* transpose(A) * transpose(B) = transpose(B * A) */
      src1 = src0_transpose;
      src0 = src1_transpose;
      src0_transpose = NULL;
      src1_transpose = NULL;
      transpose_result = true;
   }

   if (src0_transpose && !src1_transpose &&
       glsl_get_base_type(src0->type) == GLSL_TYPE_FLOAT) {
      /* We already have the rows of src0 and the columns of src1 available,
       * so we can just take the dot product of each row with each column to
       * get the result.
       */

      for (unsigned i = 0; i < src1_columns; i++) {
         nir_alu_instr *vec = create_vec(b, src0_rows);
         for (unsigned j = 0; j < src0_rows; j++) {
            vec->src[j].src =
               nir_src_for_ssa(nir_fdot(&b->nb, src0_transpose->elems[j]->def,
                                        src1->elems[i]->def));
         }

         nir_builder_instr_insert(&b->nb, &vec->instr);
         dest->elems[i]->def = &vec->dest.dest.ssa;
      }
   } else {
      /* We don't handle the case where src1 is transposed but not src0, since
       * the general case only uses individual components of src1 so the
       * optimizer should chew through the transpose we emitted for src1.
       */

      for (unsigned i = 0; i < src1_columns; i++) {
         /* dest[i] = sum(src0[j] * src1[i][j] for all j) */
         dest->elems[i]->def =
            nir_fmul(&b->nb, src0->elems[0]->def,
                     vtn_vector_extract(b, src1->elems[i]->def, 0));
         for (unsigned j = 1; j < src0_columns; j++) {
            dest->elems[i]->def =
               nir_fadd(&b->nb, dest->elems[i]->def,
                        nir_fmul(&b->nb, src0->elems[j]->def,
                                 vtn_vector_extract(b,
                                                    src1->elems[i]->def, j)));
         }
      }
   }
   
   dest = vtn_unwrap_matrix(dest);

   if (transpose_result)
      dest = vtn_transpose(b, dest);

   return dest;
}

static struct vtn_ssa_value *
vtn_mat_times_scalar(struct vtn_builder *b,
                     struct vtn_ssa_value *mat,
                     nir_ssa_def *scalar)
{
   struct vtn_ssa_value *dest = vtn_create_ssa_value(b, mat->type);
   for (unsigned i = 0; i < glsl_get_matrix_columns(mat->type); i++) {
      if (glsl_get_base_type(mat->type) == GLSL_TYPE_FLOAT)
         dest->elems[i]->def = nir_fmul(&b->nb, mat->elems[i]->def, scalar);
      else
         dest->elems[i]->def = nir_imul(&b->nb, mat->elems[i]->def, scalar);
   }

   return dest;
}

static void
vtn_handle_matrix_alu(struct vtn_builder *b, SpvOp opcode,
                      const uint32_t *w, unsigned count)
{
   struct vtn_value *val = vtn_push_value(b, w[2], vtn_value_type_ssa);

   switch (opcode) {
   case SpvOpTranspose: {
      struct vtn_ssa_value *src = vtn_ssa_value(b, w[3]);
      val->ssa = vtn_transpose(b, src);
      break;
   }

   case SpvOpOuterProduct: {
      struct vtn_ssa_value *src0 = vtn_ssa_value(b, w[3]);
      struct vtn_ssa_value *src1 = vtn_ssa_value(b, w[4]);

      val->ssa = vtn_matrix_multiply(b, src0, vtn_transpose(b, src1));
      break;
   }

   case SpvOpMatrixTimesScalar: {
      struct vtn_ssa_value *mat = vtn_ssa_value(b, w[3]);
      struct vtn_ssa_value *scalar = vtn_ssa_value(b, w[4]);

      if (mat->transposed) {
         val->ssa = vtn_transpose(b, vtn_mat_times_scalar(b, mat->transposed,
                                                          scalar->def));
      } else {
         val->ssa = vtn_mat_times_scalar(b, mat, scalar->def);
      }
      break;
   }

   case SpvOpVectorTimesMatrix:
   case SpvOpMatrixTimesVector:
   case SpvOpMatrixTimesMatrix: {
      struct vtn_ssa_value *src0 = vtn_ssa_value(b, w[3]);
      struct vtn_ssa_value *src1 = vtn_ssa_value(b, w[4]);

      val->ssa = vtn_matrix_multiply(b, src0, src1);
      break;
   }

   default: unreachable("unknown matrix opcode");
   }
}

static void
vtn_handle_alu(struct vtn_builder *b, SpvOp opcode,
               const uint32_t *w, unsigned count)
{
   struct vtn_value *val = vtn_push_value(b, w[2], vtn_value_type_ssa);
   const struct glsl_type *type =
      vtn_value(b, w[1], vtn_value_type_type)->type;
   val->ssa = vtn_create_ssa_value(b, type);

   /* Collect the various SSA sources */
   unsigned num_inputs = count - 3;
   nir_ssa_def *src[4];
   for (unsigned i = 0; i < num_inputs; i++)
      src[i] = vtn_ssa_value(b, w[i + 3])->def;

   /* Indicates that the first two arguments should be swapped.  This is
    * used for implementing greater-than and less-than-or-equal.
    */
   bool swap = false;

   nir_op op;
   switch (opcode) {
   /* Basic ALU operations */
   case SpvOpSNegate:               op = nir_op_ineg;    break;
   case SpvOpFNegate:               op = nir_op_fneg;    break;
   case SpvOpNot:                   op = nir_op_inot;    break;

   case SpvOpAny:
      switch (src[0]->num_components) {
      case 1:  op = nir_op_imov;    break;
      case 2:  op = nir_op_bany2;   break;
      case 3:  op = nir_op_bany3;   break;
      case 4:  op = nir_op_bany4;   break;
      }
      break;

   case SpvOpAll:
      switch (src[0]->num_components) {
      case 1:  op = nir_op_imov;    break;
      case 2:  op = nir_op_ball2;   break;
      case 3:  op = nir_op_ball3;   break;
      case 4:  op = nir_op_ball4;   break;
      }
      break;

   case SpvOpIAdd:                  op = nir_op_iadd;    break;
   case SpvOpFAdd:                  op = nir_op_fadd;    break;
   case SpvOpISub:                  op = nir_op_isub;    break;
   case SpvOpFSub:                  op = nir_op_fsub;    break;
   case SpvOpIMul:                  op = nir_op_imul;    break;
   case SpvOpFMul:                  op = nir_op_fmul;    break;
   case SpvOpUDiv:                  op = nir_op_udiv;    break;
   case SpvOpSDiv:                  op = nir_op_idiv;    break;
   case SpvOpFDiv:                  op = nir_op_fdiv;    break;
   case SpvOpUMod:                  op = nir_op_umod;    break;
   case SpvOpSMod:                  op = nir_op_umod;    break; /* FIXME? */
   case SpvOpFMod:                  op = nir_op_fmod;    break;

   case SpvOpDot:
      assert(src[0]->num_components == src[1]->num_components);
      switch (src[0]->num_components) {
      case 1:  op = nir_op_fmul;    break;
      case 2:  op = nir_op_fdot2;   break;
      case 3:  op = nir_op_fdot3;   break;
      case 4:  op = nir_op_fdot4;   break;
      }
      break;

   case SpvOpShiftRightLogical:     op = nir_op_ushr;    break;
   case SpvOpShiftRightArithmetic:  op = nir_op_ishr;    break;
   case SpvOpShiftLeftLogical:      op = nir_op_ishl;    break;
   case SpvOpLogicalOr:             op = nir_op_ior;     break;
   case SpvOpLogicalXor:            op = nir_op_ixor;    break;
   case SpvOpLogicalAnd:            op = nir_op_iand;    break;
   case SpvOpBitwiseOr:             op = nir_op_ior;     break;
   case SpvOpBitwiseXor:            op = nir_op_ixor;    break;
   case SpvOpBitwiseAnd:            op = nir_op_iand;    break;
   case SpvOpSelect:                op = nir_op_bcsel;   break;
   case SpvOpIEqual:                op = nir_op_ieq;     break;

   /* Comparisons: (TODO: How do we want to handled ordered/unordered?) */
   case SpvOpFOrdEqual:             op = nir_op_feq;     break;
   case SpvOpFUnordEqual:           op = nir_op_feq;     break;
   case SpvOpINotEqual:             op = nir_op_ine;     break;
   case SpvOpFOrdNotEqual:          op = nir_op_fne;     break;
   case SpvOpFUnordNotEqual:        op = nir_op_fne;     break;
   case SpvOpULessThan:             op = nir_op_ult;     break;
   case SpvOpSLessThan:             op = nir_op_ilt;     break;
   case SpvOpFOrdLessThan:          op = nir_op_flt;     break;
   case SpvOpFUnordLessThan:        op = nir_op_flt;     break;
   case SpvOpUGreaterThan:          op = nir_op_ult;  swap = true;   break;
   case SpvOpSGreaterThan:          op = nir_op_ilt;  swap = true;   break;
   case SpvOpFOrdGreaterThan:       op = nir_op_flt;  swap = true;   break;
   case SpvOpFUnordGreaterThan:     op = nir_op_flt;  swap = true;   break;
   case SpvOpULessThanEqual:        op = nir_op_uge;  swap = true;   break;
   case SpvOpSLessThanEqual:        op = nir_op_ige;  swap = true;   break;
   case SpvOpFOrdLessThanEqual:     op = nir_op_fge;  swap = true;   break;
   case SpvOpFUnordLessThanEqual:   op = nir_op_fge;  swap = true;   break;
   case SpvOpUGreaterThanEqual:     op = nir_op_uge;     break;
   case SpvOpSGreaterThanEqual:     op = nir_op_ige;     break;
   case SpvOpFOrdGreaterThanEqual:  op = nir_op_fge;     break;
   case SpvOpFUnordGreaterThanEqual:op = nir_op_fge;     break;

   /* Conversions: */
   case SpvOpConvertFToU:           op = nir_op_f2u;     break;
   case SpvOpConvertFToS:           op = nir_op_f2i;     break;
   case SpvOpConvertSToF:           op = nir_op_i2f;     break;
   case SpvOpConvertUToF:           op = nir_op_u2f;     break;
   case SpvOpBitcast:               op = nir_op_imov;    break;
   case SpvOpUConvert:
   case SpvOpSConvert:
      op = nir_op_imov; /* TODO: NIR is 32-bit only; these are no-ops. */
      break;
   case SpvOpFConvert:
      op = nir_op_fmov;
      break;

   /* Derivatives: */
   case SpvOpDPdx:         op = nir_op_fddx;          break;
   case SpvOpDPdy:         op = nir_op_fddy;          break;
   case SpvOpDPdxFine:     op = nir_op_fddx_fine;     break;
   case SpvOpDPdyFine:     op = nir_op_fddy_fine;     break;
   case SpvOpDPdxCoarse:   op = nir_op_fddx_coarse;   break;
   case SpvOpDPdyCoarse:   op = nir_op_fddy_coarse;   break;
   case SpvOpFwidth:
      val->ssa->def = nir_fadd(&b->nb,
                               nir_fabs(&b->nb, nir_fddx(&b->nb, src[0])),
                               nir_fabs(&b->nb, nir_fddx(&b->nb, src[1])));
      return;
   case SpvOpFwidthFine:
      val->ssa->def = nir_fadd(&b->nb,
                               nir_fabs(&b->nb, nir_fddx_fine(&b->nb, src[0])),
                               nir_fabs(&b->nb, nir_fddx_fine(&b->nb, src[1])));
      return;
   case SpvOpFwidthCoarse:
      val->ssa->def = nir_fadd(&b->nb,
                               nir_fabs(&b->nb, nir_fddx_coarse(&b->nb, src[0])),
                               nir_fabs(&b->nb, nir_fddx_coarse(&b->nb, src[1])));
      return;

   case SpvOpVectorTimesScalar:
      /* The builder will take care of splatting for us. */
      val->ssa->def = nir_fmul(&b->nb, src[0], src[1]);
      return;

   case SpvOpSRem:
   case SpvOpFRem:
      unreachable("No NIR equivalent");

   case SpvOpIsNan:
   case SpvOpIsInf:
   case SpvOpIsFinite:
   case SpvOpIsNormal:
   case SpvOpSignBitSet:
   case SpvOpLessOrGreater:
   case SpvOpOrdered:
   case SpvOpUnordered:
   default:
      unreachable("Unhandled opcode");
   }

   if (swap) {
      nir_ssa_def *tmp = src[0];
      src[0] = src[1];
      src[1] = tmp;
   }

   nir_alu_instr *instr = nir_alu_instr_create(b->shader, op);
   nir_ssa_dest_init(&instr->instr, &instr->dest.dest,
                     glsl_get_vector_elements(type), val->name);
   val->ssa->def = &instr->dest.dest.ssa;

   for (unsigned i = 0; i < nir_op_infos[op].num_inputs; i++)
      instr->src[i].src = nir_src_for_ssa(src[i]);

   nir_builder_instr_insert(&b->nb, &instr->instr);
}

static nir_ssa_def *
vtn_vector_extract(struct vtn_builder *b, nir_ssa_def *src, unsigned index)
{
   unsigned swiz[4] = { index };
   return nir_swizzle(&b->nb, src, swiz, 1, true);
}


static nir_ssa_def *
vtn_vector_insert(struct vtn_builder *b, nir_ssa_def *src, nir_ssa_def *insert,
                  unsigned index)
{
   nir_alu_instr *vec = create_vec(b->shader, src->num_components);

   for (unsigned i = 0; i < src->num_components; i++) {
      if (i == index) {
         vec->src[i].src = nir_src_for_ssa(insert);
      } else {
         vec->src[i].src = nir_src_for_ssa(src);
         vec->src[i].swizzle[0] = i;
      }
   }

   nir_builder_instr_insert(&b->nb, &vec->instr);

   return &vec->dest.dest.ssa;
}

static nir_ssa_def *
vtn_vector_extract_dynamic(struct vtn_builder *b, nir_ssa_def *src,
                           nir_ssa_def *index)
{
   nir_ssa_def *dest = vtn_vector_extract(b, src, 0);
   for (unsigned i = 1; i < src->num_components; i++)
      dest = nir_bcsel(&b->nb, nir_ieq(&b->nb, index, nir_imm_int(&b->nb, i)),
                       vtn_vector_extract(b, src, i), dest);

   return dest;
}

static nir_ssa_def *
vtn_vector_insert_dynamic(struct vtn_builder *b, nir_ssa_def *src,
                          nir_ssa_def *insert, nir_ssa_def *index)
{
   nir_ssa_def *dest = vtn_vector_insert(b, src, insert, 0);
   for (unsigned i = 1; i < src->num_components; i++)
      dest = nir_bcsel(&b->nb, nir_ieq(&b->nb, index, nir_imm_int(&b->nb, i)),
                       vtn_vector_insert(b, src, insert, i), dest);

   return dest;
}

static nir_ssa_def *
vtn_vector_shuffle(struct vtn_builder *b, unsigned num_components,
                   nir_ssa_def *src0, nir_ssa_def *src1,
                   const uint32_t *indices)
{
   nir_alu_instr *vec = create_vec(b->shader, num_components);

   nir_ssa_undef_instr *undef = nir_ssa_undef_instr_create(b->shader, 1);
   nir_builder_instr_insert(&b->nb, &undef->instr);

   for (unsigned i = 0; i < num_components; i++) {
      uint32_t index = indices[i];
      if (index == 0xffffffff) {
         vec->src[i].src = nir_src_for_ssa(&undef->def);
      } else if (index < src0->num_components) {
         vec->src[i].src = nir_src_for_ssa(src0);
         vec->src[i].swizzle[0] = index;
      } else {
         vec->src[i].src = nir_src_for_ssa(src1);
         vec->src[i].swizzle[0] = index - src0->num_components;
      }
   }

   nir_builder_instr_insert(&b->nb, &vec->instr);

   return &vec->dest.dest.ssa;
}

/*
 * Concatentates a number of vectors/scalars together to produce a vector
 */
static nir_ssa_def *
vtn_vector_construct(struct vtn_builder *b, unsigned num_components,
                     unsigned num_srcs, nir_ssa_def **srcs)
{
   nir_alu_instr *vec = create_vec(b->shader, num_components);

   unsigned dest_idx = 0;
   for (unsigned i = 0; i < num_srcs; i++) {
      nir_ssa_def *src = srcs[i];
      for (unsigned j = 0; j < src->num_components; j++) {
         vec->src[dest_idx].src = nir_src_for_ssa(src);
         vec->src[dest_idx].swizzle[0] = j;
         dest_idx++;
      }
   }

   nir_builder_instr_insert(&b->nb, &vec->instr);

   return &vec->dest.dest.ssa;
}

static struct vtn_ssa_value *
vtn_composite_copy(void *mem_ctx, struct vtn_ssa_value *src)
{
   struct vtn_ssa_value *dest = rzalloc(mem_ctx, struct vtn_ssa_value);
   dest->type = src->type;

   if (glsl_type_is_vector_or_scalar(src->type)) {
      dest->def = src->def;
   } else {
      unsigned elems = glsl_get_length(src->type);

      dest->elems = ralloc_array(mem_ctx, struct vtn_ssa_value *, elems);
      for (unsigned i = 0; i < elems; i++)
         dest->elems[i] = vtn_composite_copy(mem_ctx, src->elems[i]);
   }

   return dest;
}

static struct vtn_ssa_value *
vtn_composite_insert(struct vtn_builder *b, struct vtn_ssa_value *src,
                     struct vtn_ssa_value *insert, const uint32_t *indices,
                     unsigned num_indices)
{
   struct vtn_ssa_value *dest = vtn_composite_copy(b, src);

   struct vtn_ssa_value *cur = dest;
   unsigned i;
   for (i = 0; i < num_indices - 1; i++) {
      cur = cur->elems[indices[i]];
   }

   if (glsl_type_is_vector_or_scalar(cur->type)) {
      /* According to the SPIR-V spec, OpCompositeInsert may work down to
       * the component granularity. In that case, the last index will be
       * the index to insert the scalar into the vector.
       */

      cur->def = vtn_vector_insert(b, cur->def, insert->def, indices[i]);
   } else {
      cur->elems[indices[i]] = insert;
   }

   return dest;
}

static struct vtn_ssa_value *
vtn_composite_extract(struct vtn_builder *b, struct vtn_ssa_value *src,
                      const uint32_t *indices, unsigned num_indices)
{
   struct vtn_ssa_value *cur = src;
   for (unsigned i = 0; i < num_indices; i++) {
      if (glsl_type_is_vector_or_scalar(cur->type)) {
         assert(i == num_indices - 1);
         /* According to the SPIR-V spec, OpCompositeExtract may work down to
          * the component granularity. The last index will be the index of the
          * vector to extract.
          */

         struct vtn_ssa_value *ret = rzalloc(b, struct vtn_ssa_value);
         ret->type = glsl_scalar_type(glsl_get_base_type(cur->type));
         ret->def = vtn_vector_extract(b, cur->def, indices[i]);
         return ret;
      }
   }

   return cur;
}

static void
vtn_handle_composite(struct vtn_builder *b, SpvOp opcode,
                     const uint32_t *w, unsigned count)
{
   struct vtn_value *val = vtn_push_value(b, w[2], vtn_value_type_ssa);
   const struct glsl_type *type = vtn_value(b, w[1], vtn_value_type_type)->type;
   val->ssa = vtn_create_ssa_value(b, type);

   switch (opcode) {
   case SpvOpVectorExtractDynamic:
      val->ssa->def = vtn_vector_extract_dynamic(b, vtn_ssa_value(b, w[3])->def,
                                                 vtn_ssa_value(b, w[4])->def);
      break;

   case SpvOpVectorInsertDynamic:
      val->ssa->def = vtn_vector_insert_dynamic(b, vtn_ssa_value(b, w[3])->def,
                                                vtn_ssa_value(b, w[4])->def,
                                                vtn_ssa_value(b, w[5])->def);
      break;

   case SpvOpVectorShuffle:
      val->ssa->def = vtn_vector_shuffle(b, glsl_get_vector_elements(type),
                                         vtn_ssa_value(b, w[3])->def,
                                         vtn_ssa_value(b, w[4])->def,
                                         w + 5);
      break;

   case SpvOpCompositeConstruct: {
      val->ssa = rzalloc(b, struct vtn_ssa_value);
      unsigned elems = count - 3;
      if (glsl_type_is_vector_or_scalar(type)) {
         nir_ssa_def *srcs[4];
         for (unsigned i = 0; i < elems; i++)
            srcs[i] = vtn_ssa_value(b, w[3 + i])->def;
         val->ssa->def =
            vtn_vector_construct(b, glsl_get_vector_elements(type),
                                 elems, srcs);
      } else {
         val->ssa->elems = ralloc_array(b, struct vtn_ssa_value *, elems);
         for (unsigned i = 0; i < elems; i++)
            val->ssa->elems[i] = vtn_ssa_value(b, w[3 + i]);
      }
      break;
   }
   case SpvOpCompositeExtract:
      val->ssa = vtn_composite_extract(b, vtn_ssa_value(b, w[3]),
                                       w + 4, count - 4);
      break;

   case SpvOpCompositeInsert:
      val->ssa = vtn_composite_insert(b, vtn_ssa_value(b, w[4]),
                                      vtn_ssa_value(b, w[3]),
                                      w + 5, count - 5);
      break;

   case SpvOpCopyObject:
      val->ssa = vtn_composite_copy(b, vtn_ssa_value(b, w[3]));
      break;

   default:
      unreachable("unknown composite operation");
   }
}

static void
vtn_phi_node_init(struct vtn_builder *b, struct vtn_ssa_value *val)
{
   if (glsl_type_is_vector_or_scalar(val->type)) {
      nir_phi_instr *phi = nir_phi_instr_create(b->shader);
      nir_ssa_dest_init(&phi->instr, &phi->dest,
                        glsl_get_vector_elements(val->type), NULL);
      exec_list_make_empty(&phi->srcs);
      nir_builder_instr_insert(&b->nb, &phi->instr);
      val->def = &phi->dest.ssa;
   } else {
      unsigned elems = glsl_get_length(val->type);
      for (unsigned i = 0; i < elems; i++)
         vtn_phi_node_init(b, val->elems[i]);
   }
}

static struct vtn_ssa_value *
vtn_phi_node_create(struct vtn_builder *b, const struct glsl_type *type)
{
   struct vtn_ssa_value *val = vtn_create_ssa_value(b, type);
   vtn_phi_node_init(b, val);
   return val;
}

static void
vtn_handle_phi_first_pass(struct vtn_builder *b, const uint32_t *w)
{
   struct vtn_value *val = vtn_push_value(b, w[2], vtn_value_type_ssa);
   const struct glsl_type *type = vtn_value(b, w[1], vtn_value_type_type)->type;
   val->ssa = vtn_phi_node_create(b, type);
}

static void
vtn_phi_node_add_src(struct vtn_ssa_value *phi, const nir_block *pred,
                     struct vtn_ssa_value *val)
{
   assert(phi->type == val->type);
   if (glsl_type_is_vector_or_scalar(phi->type)) {
      nir_phi_instr *phi_instr = nir_instr_as_phi(phi->def->parent_instr);
      nir_phi_src *src = ralloc(phi_instr, nir_phi_src);
      src->pred = (nir_block *) pred;
      src->src = nir_src_for_ssa(val->def);
      exec_list_push_tail(&phi_instr->srcs, &src->node);
   } else {
      unsigned elems = glsl_get_length(phi->type);
      for (unsigned i = 0; i < elems; i++)
         vtn_phi_node_add_src(phi->elems[i], pred, val->elems[i]);
   }
}

static struct vtn_ssa_value *
vtn_get_phi_node_src(struct vtn_builder *b, nir_block *block,
                     const struct glsl_type *type, const uint32_t *w,
                     unsigned count)
{
   struct hash_entry *entry = _mesa_hash_table_search(b->block_table, block);
   if (entry) {
      struct vtn_block *spv_block = entry->data;
      for (unsigned off = 4; off < count; off += 2) {
         if (spv_block == vtn_value(b, w[off], vtn_value_type_block)->block) {
            return vtn_ssa_value(b, w[off - 1]);
         }
      }
   }

   nir_builder_insert_before_block(&b->nb, block);
   struct vtn_ssa_value *phi = vtn_phi_node_create(b, type);

   struct set_entry *entry2;
   set_foreach(block->predecessors, entry2) {
      nir_block *pred = (nir_block *) entry2->key;
      struct vtn_ssa_value *val = vtn_get_phi_node_src(b, pred, type, w,
                                                       count);
      vtn_phi_node_add_src(phi, pred, val);
   }

   return phi;
}

static bool
vtn_handle_phi_second_pass(struct vtn_builder *b, SpvOp opcode,
                           const uint32_t *w, unsigned count)
{
   if (opcode == SpvOpLabel) {
      b->block = vtn_value(b, w[1], vtn_value_type_block)->block;
      return true;
   }

   if (opcode != SpvOpPhi)
      return true;

   struct vtn_ssa_value *phi = vtn_value(b, w[2], vtn_value_type_ssa)->ssa;

   struct set_entry *entry;
   set_foreach(b->block->block->predecessors, entry) {
      nir_block *pred = (nir_block *) entry->key;

      struct vtn_ssa_value *val = vtn_get_phi_node_src(b, pred, phi->type, w,
                                                       count);
      vtn_phi_node_add_src(phi, pred, val);
   }

   return true;
}

static bool
vtn_handle_preamble_instruction(struct vtn_builder *b, SpvOp opcode,
                                const uint32_t *w, unsigned count)
{
   switch (opcode) {
   case SpvOpSource:
   case SpvOpSourceExtension:
   case SpvOpCompileFlag:
   case SpvOpExtension:
      /* Unhandled, but these are for debug so that's ok. */
      break;

   case SpvOpExtInstImport:
      vtn_handle_extension(b, opcode, w, count);
      break;

   case SpvOpMemoryModel:
      assert(w[1] == SpvAddressingModelLogical);
      assert(w[2] == SpvMemoryModelGLSL450);
      break;

   case SpvOpEntryPoint:
      assert(b->entry_point == NULL);
      b->entry_point = &b->values[w[2]];
      b->execution_model = w[1];
      break;

   case SpvOpExecutionMode:
      unreachable("Execution modes not yet implemented");
      break;

   case SpvOpString:
      vtn_push_value(b, w[1], vtn_value_type_string)->str =
         vtn_string_literal(b, &w[2], count - 2);
      break;

   case SpvOpName:
      b->values[w[1]].name = vtn_string_literal(b, &w[2], count - 2);
      break;

   case SpvOpMemberName:
      /* TODO */
      break;

   case SpvOpLine:
      break; /* Ignored for now */

   case SpvOpDecorationGroup:
   case SpvOpDecorate:
   case SpvOpMemberDecorate:
   case SpvOpGroupDecorate:
   case SpvOpGroupMemberDecorate:
      vtn_handle_decoration(b, opcode, w, count);
      break;

   case SpvOpTypeVoid:
   case SpvOpTypeBool:
   case SpvOpTypeInt:
   case SpvOpTypeFloat:
   case SpvOpTypeVector:
   case SpvOpTypeMatrix:
   case SpvOpTypeSampler:
   case SpvOpTypeArray:
   case SpvOpTypeRuntimeArray:
   case SpvOpTypeStruct:
   case SpvOpTypeOpaque:
   case SpvOpTypePointer:
   case SpvOpTypeFunction:
   case SpvOpTypeEvent:
   case SpvOpTypeDeviceEvent:
   case SpvOpTypeReserveId:
   case SpvOpTypeQueue:
   case SpvOpTypePipe:
      vtn_handle_type(b, opcode, w, count);
      break;

   case SpvOpConstantTrue:
   case SpvOpConstantFalse:
   case SpvOpConstant:
   case SpvOpConstantComposite:
   case SpvOpConstantSampler:
   case SpvOpConstantNullPointer:
   case SpvOpConstantNullObject:
   case SpvOpSpecConstantTrue:
   case SpvOpSpecConstantFalse:
   case SpvOpSpecConstant:
   case SpvOpSpecConstantComposite:
      vtn_handle_constant(b, opcode, w, count);
      break;

   case SpvOpVariable:
      vtn_handle_variables(b, opcode, w, count);
      break;

   default:
      return false; /* End of preamble */
   }

   return true;
}

static bool
vtn_handle_first_cfg_pass_instruction(struct vtn_builder *b, SpvOp opcode,
                                      const uint32_t *w, unsigned count)
{
   switch (opcode) {
   case SpvOpFunction: {
      assert(b->func == NULL);
      b->func = rzalloc(b, struct vtn_function);

      const struct glsl_type *result_type =
         vtn_value(b, w[1], vtn_value_type_type)->type;
      struct vtn_value *val = vtn_push_value(b, w[2], vtn_value_type_function);
      const struct glsl_type *func_type =
         vtn_value(b, w[4], vtn_value_type_type)->type;

      assert(glsl_get_function_return_type(func_type) == result_type);

      nir_function *func =
         nir_function_create(b->shader, ralloc_strdup(b->shader, val->name));

      nir_function_overload *overload = nir_function_overload_create(func);
      overload->num_params = glsl_get_length(func_type);
      overload->params = ralloc_array(overload, nir_parameter,
                                      overload->num_params);
      for (unsigned i = 0; i < overload->num_params; i++) {
         const struct glsl_function_param *param =
            glsl_get_function_param(func_type, i);
         overload->params[i].type = param->type;
         if (param->in) {
            if (param->out) {
               overload->params[i].param_type = nir_parameter_inout;
            } else {
               overload->params[i].param_type = nir_parameter_in;
            }
         } else {
            if (param->out) {
               overload->params[i].param_type = nir_parameter_out;
            } else {
               assert(!"Parameter is neither in nor out");
            }
         }
      }
      b->func->overload = overload;
      break;
   }

   case SpvOpFunctionEnd:
      b->func->end = w;
      b->func = NULL;
      break;

   case SpvOpFunctionParameter:
      break; /* Does nothing */

   case SpvOpLabel: {
      assert(b->block == NULL);
      b->block = rzalloc(b, struct vtn_block);
      b->block->label = w;
      vtn_push_value(b, w[1], vtn_value_type_block)->block = b->block;

      if (b->func->start_block == NULL) {
         /* This is the first block encountered for this function.  In this
          * case, we set the start block and add it to the list of
          * implemented functions that we'll walk later.
          */
         b->func->start_block = b->block;
         exec_list_push_tail(&b->functions, &b->func->node);
      }
      break;
   }

   case SpvOpBranch:
   case SpvOpBranchConditional:
   case SpvOpSwitch:
   case SpvOpKill:
   case SpvOpReturn:
   case SpvOpReturnValue:
   case SpvOpUnreachable:
      assert(b->block);
      b->block->branch = w;
      b->block = NULL;
      break;

   case SpvOpSelectionMerge:
   case SpvOpLoopMerge:
      assert(b->block && b->block->merge_op == SpvOpNop);
      b->block->merge_op = opcode;
      b->block->merge_block_id = w[1];
      break;

   default:
      /* Continue on as per normal */
      return true;
   }

   return true;
}

static bool
vtn_handle_body_instruction(struct vtn_builder *b, SpvOp opcode,
                            const uint32_t *w, unsigned count)
{
   switch (opcode) {
   case SpvOpLabel: {
      struct vtn_block *block = vtn_value(b, w[1], vtn_value_type_block)->block;
      assert(block->block == NULL);

      struct exec_node *list_tail = exec_list_get_tail(b->nb.cf_node_list);
      nir_cf_node *tail_node = exec_node_data(nir_cf_node, list_tail, node);
      assert(tail_node->type == nir_cf_node_block);
      block->block = nir_cf_node_as_block(tail_node);
      break;
   }

   case SpvOpLoopMerge:
   case SpvOpSelectionMerge:
      /* This is handled by cfg pre-pass and walk_blocks */
      break;

   case SpvOpUndef:
      vtn_push_value(b, w[2], vtn_value_type_undef);
      break;

   case SpvOpExtInst:
      vtn_handle_extension(b, opcode, w, count);
      break;

   case SpvOpVariable:
   case SpvOpVariableArray:
   case SpvOpLoad:
   case SpvOpStore:
   case SpvOpCopyMemory:
   case SpvOpCopyMemorySized:
   case SpvOpAccessChain:
   case SpvOpInBoundsAccessChain:
   case SpvOpArrayLength:
   case SpvOpImagePointer:
      vtn_handle_variables(b, opcode, w, count);
      break;

   case SpvOpFunctionCall:
      vtn_handle_function_call(b, opcode, w, count);
      break;

   case SpvOpTextureSample:
   case SpvOpTextureSampleDref:
   case SpvOpTextureSampleLod:
   case SpvOpTextureSampleProj:
   case SpvOpTextureSampleGrad:
   case SpvOpTextureSampleOffset:
   case SpvOpTextureSampleProjLod:
   case SpvOpTextureSampleProjGrad:
   case SpvOpTextureSampleLodOffset:
   case SpvOpTextureSampleProjOffset:
   case SpvOpTextureSampleGradOffset:
   case SpvOpTextureSampleProjLodOffset:
   case SpvOpTextureSampleProjGradOffset:
   case SpvOpTextureFetchTexelLod:
   case SpvOpTextureFetchTexelOffset:
   case SpvOpTextureFetchSample:
   case SpvOpTextureFetchTexel:
   case SpvOpTextureGather:
   case SpvOpTextureGatherOffset:
   case SpvOpTextureGatherOffsets:
   case SpvOpTextureQuerySizeLod:
   case SpvOpTextureQuerySize:
   case SpvOpTextureQueryLod:
   case SpvOpTextureQueryLevels:
   case SpvOpTextureQuerySamples:
      vtn_handle_texture(b, opcode, w, count);
      break;

   case SpvOpSNegate:
   case SpvOpFNegate:
   case SpvOpNot:
   case SpvOpAny:
   case SpvOpAll:
   case SpvOpConvertFToU:
   case SpvOpConvertFToS:
   case SpvOpConvertSToF:
   case SpvOpConvertUToF:
   case SpvOpUConvert:
   case SpvOpSConvert:
   case SpvOpFConvert:
   case SpvOpConvertPtrToU:
   case SpvOpConvertUToPtr:
   case SpvOpPtrCastToGeneric:
   case SpvOpGenericCastToPtr:
   case SpvOpBitcast:
   case SpvOpIsNan:
   case SpvOpIsInf:
   case SpvOpIsFinite:
   case SpvOpIsNormal:
   case SpvOpSignBitSet:
   case SpvOpLessOrGreater:
   case SpvOpOrdered:
   case SpvOpUnordered:
   case SpvOpIAdd:
   case SpvOpFAdd:
   case SpvOpISub:
   case SpvOpFSub:
   case SpvOpIMul:
   case SpvOpFMul:
   case SpvOpUDiv:
   case SpvOpSDiv:
   case SpvOpFDiv:
   case SpvOpUMod:
   case SpvOpSRem:
   case SpvOpSMod:
   case SpvOpFRem:
   case SpvOpFMod:
   case SpvOpVectorTimesScalar:
   case SpvOpDot:
   case SpvOpShiftRightLogical:
   case SpvOpShiftRightArithmetic:
   case SpvOpShiftLeftLogical:
   case SpvOpLogicalOr:
   case SpvOpLogicalXor:
   case SpvOpLogicalAnd:
   case SpvOpBitwiseOr:
   case SpvOpBitwiseXor:
   case SpvOpBitwiseAnd:
   case SpvOpSelect:
   case SpvOpIEqual:
   case SpvOpFOrdEqual:
   case SpvOpFUnordEqual:
   case SpvOpINotEqual:
   case SpvOpFOrdNotEqual:
   case SpvOpFUnordNotEqual:
   case SpvOpULessThan:
   case SpvOpSLessThan:
   case SpvOpFOrdLessThan:
   case SpvOpFUnordLessThan:
   case SpvOpUGreaterThan:
   case SpvOpSGreaterThan:
   case SpvOpFOrdGreaterThan:
   case SpvOpFUnordGreaterThan:
   case SpvOpULessThanEqual:
   case SpvOpSLessThanEqual:
   case SpvOpFOrdLessThanEqual:
   case SpvOpFUnordLessThanEqual:
   case SpvOpUGreaterThanEqual:
   case SpvOpSGreaterThanEqual:
   case SpvOpFOrdGreaterThanEqual:
   case SpvOpFUnordGreaterThanEqual:
   case SpvOpDPdx:
   case SpvOpDPdy:
   case SpvOpFwidth:
   case SpvOpDPdxFine:
   case SpvOpDPdyFine:
   case SpvOpFwidthFine:
   case SpvOpDPdxCoarse:
   case SpvOpDPdyCoarse:
   case SpvOpFwidthCoarse:
      vtn_handle_alu(b, opcode, w, count);
      break;

   case SpvOpTranspose:
   case SpvOpOuterProduct:
   case SpvOpMatrixTimesScalar:
   case SpvOpVectorTimesMatrix:
   case SpvOpMatrixTimesVector:
   case SpvOpMatrixTimesMatrix:
      vtn_handle_matrix_alu(b, opcode, w, count);
      break;

   case SpvOpVectorExtractDynamic:
   case SpvOpVectorInsertDynamic:
   case SpvOpVectorShuffle:
   case SpvOpCompositeConstruct:
   case SpvOpCompositeExtract:
   case SpvOpCompositeInsert:
   case SpvOpCopyObject:
      vtn_handle_composite(b, opcode, w, count);
      break;

   case SpvOpPhi:
      vtn_handle_phi_first_pass(b, w);
      break;

   default:
      unreachable("Unhandled opcode");
   }

   return true;
}

static void
vtn_walk_blocks(struct vtn_builder *b, struct vtn_block *start,
                struct vtn_block *break_block, struct vtn_block *cont_block,
                struct vtn_block *end_block)
{
   struct vtn_block *block = start;
   while (block != end_block) {
      if (block->merge_op == SpvOpLoopMerge) {
         /* This is the jump into a loop. */
         struct vtn_block *new_cont_block = block;
         struct vtn_block *new_break_block =
            vtn_value(b, block->merge_block_id, vtn_value_type_block)->block;

         nir_loop *loop = nir_loop_create(b->shader);
         nir_cf_node_insert_end(b->nb.cf_node_list, &loop->cf_node);

         struct exec_list *old_list = b->nb.cf_node_list;

         /* Reset the merge_op to prerevent infinite recursion */
         block->merge_op = SpvOpNop;

         nir_builder_insert_after_cf_list(&b->nb, &loop->body);
         vtn_walk_blocks(b, block, new_break_block, new_cont_block, NULL);

         nir_builder_insert_after_cf_list(&b->nb, old_list);
         block = new_break_block;
         continue;
      }

      const uint32_t *w = block->branch;
      SpvOp branch_op = w[0] & SpvOpCodeMask;

      b->block = block;
      vtn_foreach_instruction(b, block->label, block->branch,
                              vtn_handle_body_instruction);

      nir_cf_node *cur_cf_node =
         exec_node_data(nir_cf_node, exec_list_get_tail(b->nb.cf_node_list),
                        node);
      nir_block *cur_block = nir_cf_node_as_block(cur_cf_node);
      _mesa_hash_table_insert(b->block_table, cur_block, block);

      switch (branch_op) {
      case SpvOpBranch: {
         struct vtn_block *branch_block =
            vtn_value(b, w[1], vtn_value_type_block)->block;

         if (branch_block == break_block) {
            nir_jump_instr *jump = nir_jump_instr_create(b->shader,
                                                         nir_jump_break);
            nir_builder_instr_insert(&b->nb, &jump->instr);

            return;
         } else if (branch_block == cont_block) {
            nir_jump_instr *jump = nir_jump_instr_create(b->shader,
                                                         nir_jump_continue);
            nir_builder_instr_insert(&b->nb, &jump->instr);

            return;
         } else if (branch_block == end_block) {
            /* We're branching to the merge block of an if, since for loops
             * and functions end_block == NULL, so we're done here.
             */
            return;
         } else {
            /* We're branching to another block, and according to the rules,
             * we can only branch to another block with one predecessor (so
             * we're the only one jumping to it) so we can just process it
             * next.
             */
            block = branch_block;
            continue;
         }
      }

      case SpvOpBranchConditional: {
         /* Gather up the branch blocks */
         struct vtn_block *then_block =
            vtn_value(b, w[2], vtn_value_type_block)->block;
         struct vtn_block *else_block =
            vtn_value(b, w[3], vtn_value_type_block)->block;

         nir_if *if_stmt = nir_if_create(b->shader);
         if_stmt->condition = nir_src_for_ssa(vtn_ssa_value(b, w[1])->def);
         nir_cf_node_insert_end(b->nb.cf_node_list, &if_stmt->cf_node);

         if (then_block == break_block) {
            nir_jump_instr *jump = nir_jump_instr_create(b->shader,
                                                         nir_jump_break);
            nir_instr_insert_after_cf_list(&if_stmt->then_list,
                                           &jump->instr);
            block = else_block;
         } else if (else_block == break_block) {
            nir_jump_instr *jump = nir_jump_instr_create(b->shader,
                                                         nir_jump_break);
            nir_instr_insert_after_cf_list(&if_stmt->else_list,
                                           &jump->instr);
            block = then_block;
         } else if (then_block == cont_block) {
            nir_jump_instr *jump = nir_jump_instr_create(b->shader,
                                                         nir_jump_continue);
            nir_instr_insert_after_cf_list(&if_stmt->then_list,
                                           &jump->instr);
            block = else_block;
         } else if (else_block == cont_block) {
            nir_jump_instr *jump = nir_jump_instr_create(b->shader,
                                                         nir_jump_continue);
            nir_instr_insert_after_cf_list(&if_stmt->else_list,
                                           &jump->instr);
            block = then_block;
         } else {
            /* According to the rules we're branching to two blocks that don't
             * have any other predecessors, so we can handle this as a
             * conventional if.
             */
            assert(block->merge_op == SpvOpSelectionMerge);
            struct vtn_block *merge_block =
               vtn_value(b, block->merge_block_id, vtn_value_type_block)->block;

            struct exec_list *old_list = b->nb.cf_node_list;

            nir_builder_insert_after_cf_list(&b->nb, &if_stmt->then_list);
            vtn_walk_blocks(b, then_block, break_block, cont_block, merge_block);

            nir_builder_insert_after_cf_list(&b->nb, &if_stmt->else_list);
            vtn_walk_blocks(b, else_block, break_block, cont_block, merge_block);

            nir_builder_insert_after_cf_list(&b->nb, old_list);
            block = merge_block;
            continue;
         }

         /* If we got here then we inserted a predicated break or continue
          * above and we need to handle the other case.  We already set
          * `block` above to indicate what block to visit after the
          * predicated break.
          */

         /* It's possible that the other branch is also a break/continue.
          * If it is, we handle that here.
          */
         if (block == break_block) {
            nir_jump_instr *jump = nir_jump_instr_create(b->shader,
                                                         nir_jump_break);
            nir_builder_instr_insert(&b->nb, &jump->instr);

            return;
         } else if (block == cont_block) {
            nir_jump_instr *jump = nir_jump_instr_create(b->shader,
                                                         nir_jump_continue);
            nir_builder_instr_insert(&b->nb, &jump->instr);

            return;
         }

         /* If we got here then there was a predicated break/continue but
          * the other half of the if has stuff in it.  `block` was already
          * set above so there is nothing left for us to do.
          */
         continue;
      }

      case SpvOpReturn: {
         nir_jump_instr *jump = nir_jump_instr_create(b->shader,
                                                      nir_jump_return);
         nir_builder_instr_insert(&b->nb, &jump->instr);
         return;
      }

      case SpvOpKill: {
         nir_intrinsic_instr *discard =
            nir_intrinsic_instr_create(b->shader, nir_intrinsic_discard);
         nir_builder_instr_insert(&b->nb, &discard->instr);
         return;
      }

      case SpvOpSwitch:
      case SpvOpReturnValue:
      case SpvOpUnreachable:
      default:
         unreachable("Unhandled opcode");
      }
   }
}

nir_shader *
spirv_to_nir(const uint32_t *words, size_t word_count,
             const nir_shader_compiler_options *options)
{
   const uint32_t *word_end = words + word_count;

   /* Handle the SPIR-V header (first 4 dwords)  */
   assert(word_count > 5);

   assert(words[0] == SpvMagicNumber);
   assert(words[1] == 99);
   /* words[2] == generator magic */
   unsigned value_id_bound = words[3];
   assert(words[4] == 0);

   words+= 5;

   nir_shader *shader = nir_shader_create(NULL, options);

   /* Initialize the stn_builder object */
   struct vtn_builder *b = rzalloc(NULL, struct vtn_builder);
   b->shader = shader;
   b->value_id_bound = value_id_bound;
   b->values = ralloc_array(b, struct vtn_value, value_id_bound);
   exec_list_make_empty(&b->functions);

   /* Handle all the preamble instructions */
   words = vtn_foreach_instruction(b, words, word_end,
                                   vtn_handle_preamble_instruction);

   /* Do a very quick CFG analysis pass */
   vtn_foreach_instruction(b, words, word_end,
                           vtn_handle_first_cfg_pass_instruction);

   foreach_list_typed(struct vtn_function, func, node, &b->functions) {
      b->impl = nir_function_impl_create(func->overload);
      b->const_table = _mesa_hash_table_create(b, _mesa_hash_pointer,
                                               _mesa_key_pointer_equal);
      b->block_table = _mesa_hash_table_create(b, _mesa_hash_pointer,
                                               _mesa_key_pointer_equal);
      nir_builder_init(&b->nb, b->impl);
      nir_builder_insert_after_cf_list(&b->nb, &b->impl->body);
      vtn_walk_blocks(b, func->start_block, NULL, NULL, NULL);
      vtn_foreach_instruction(b, func->start_block->label, func->end,
                              vtn_handle_phi_second_pass);
   }

   ralloc_free(b);

   return shader;
}

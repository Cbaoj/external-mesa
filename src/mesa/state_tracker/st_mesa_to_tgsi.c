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

/*
 * \author
 * Michal Krol
 */


#include "tgsi_platform.h"
#include "pipe/tgsi/exec/tgsi_core.h"
#include "st_mesa_to_tgsi.h"
#include "shader/prog_parameter.h"

#define TGSI_DEBUG 0


/*
 * Map mesa register file to TGSI register file.
 */
static GLuint
map_register_file(
   enum register_file file )
{
   switch( file ) {
   case PROGRAM_UNDEFINED:
      return TGSI_FILE_NULL;
   case PROGRAM_TEMPORARY:
      return TGSI_FILE_TEMPORARY;
   //case PROGRAM_LOCAL_PARAM:
   //case PROGRAM_ENV_PARAM:
   case PROGRAM_STATE_VAR:
   case PROGRAM_NAMED_PARAM:
   case PROGRAM_UNIFORM:
      return TGSI_FILE_CONSTANT;
   case PROGRAM_CONSTANT:
      return TGSI_FILE_IMMEDIATE;
   case PROGRAM_INPUT:
      return TGSI_FILE_INPUT;
   case PROGRAM_OUTPUT:
      return TGSI_FILE_OUTPUT;
   case PROGRAM_ADDRESS:
      return TGSI_FILE_ADDRESS;
   default:
      assert( 0 );
      return TGSI_FILE_NULL;
   }
}

/**
 * Map mesa register file index to TGSI index.
 * Take special care when processing input and output indices.
 * \param file  one of TGSI_FILE_x
 * \param index  the mesa register file index
 * \param inputMapping  maps Mesa input indexes to TGSI input indexes
 * \param outputMapping  maps Mesa output indexes to TGSI output indexes
 */
static GLuint
map_register_file_index(
   GLuint file,
   GLuint index,
   const GLuint inputMapping[],
   const GLuint outputMapping[],
   const GLuint immediateMapping[])
{
   switch( file ) {
   case TGSI_FILE_INPUT:
      /* inputs are mapped according to the user-defined map */
      return inputMapping[index];

   case TGSI_FILE_OUTPUT:
      return outputMapping[index];

   case TGSI_FILE_IMMEDIATE:
      return immediateMapping[index];

   default:
      return index;
   }
}

/*
 * Map mesa texture target to TGSI texture target.
 */
static GLuint
map_texture_target(
   GLuint textarget )
{
   switch( textarget ) {
   case TEXTURE_1D_INDEX:
      return TGSI_TEXTURE_1D;
   case TEXTURE_2D_INDEX:
      return TGSI_TEXTURE_2D;
   case TEXTURE_3D_INDEX:
      return TGSI_TEXTURE_3D;
   case TEXTURE_CUBE_INDEX:
      return TGSI_TEXTURE_CUBE;
   case TEXTURE_RECT_INDEX:
      return TGSI_TEXTURE_RECT;
   default:
      assert( 0 );
   }

   return TGSI_TEXTURE_1D;
}

static GLuint
convert_sat(
   GLuint sat )
{
   switch( sat ) {
   case SATURATE_OFF:
      return TGSI_SAT_NONE;
   case SATURATE_ZERO_ONE:
      return TGSI_SAT_ZERO_ONE;
   case SATURATE_PLUS_MINUS_ONE:
      return TGSI_SAT_MINUS_PLUS_ONE;
   default:
      assert( 0 );
      return TGSI_SAT_NONE;
   }
}

static GLuint
convert_writemask(
   GLuint writemask )
{
   assert( WRITEMASK_X == TGSI_WRITEMASK_X );
   assert( WRITEMASK_Y == TGSI_WRITEMASK_Y );
   assert( WRITEMASK_Z == TGSI_WRITEMASK_Z );
   assert( WRITEMASK_W == TGSI_WRITEMASK_W );
   assert( (writemask & ~TGSI_WRITEMASK_XYZW) == 0 );

   return writemask;
}

static struct tgsi_full_immediate
make_immediate(const float *value, uint size)
{
   struct tgsi_full_immediate imm;
   imm.Immediate.Type = TGSI_TOKEN_TYPE_IMMEDIATE;
   imm.Immediate.Size = 1 + size; /* one for the token itself */
   imm.Immediate.DataType = TGSI_IMM_FLOAT32;
   imm.u.ImmediateFloat32 = (struct tgsi_immediate_float32 *) value;
   return imm;
}

static void
compile_instruction(
   const struct prog_instruction *inst,
   struct tgsi_full_instruction *fullinst,
   const GLuint inputMapping[],
   const GLuint outputMapping[],
   const GLuint immediateMapping[],
   GLuint preamble_size,
   GLuint processor )
{
   GLuint i;
   struct tgsi_full_dst_register *fulldst;
   struct tgsi_full_src_register *fullsrc;

   *fullinst = tgsi_default_full_instruction();

   fullinst->Instruction.Saturate = convert_sat( inst->SaturateMode );
   fullinst->Instruction.NumDstRegs = _mesa_num_inst_dst_regs( inst->Opcode );
   fullinst->Instruction.NumSrcRegs = _mesa_num_inst_src_regs( inst->Opcode );

   fulldst = &fullinst->FullDstRegisters[0];
   fulldst->DstRegister.File = map_register_file( inst->DstReg.File );
   fulldst->DstRegister.Index = map_register_file_index(
      fulldst->DstRegister.File,
      inst->DstReg.Index,
      inputMapping,
      outputMapping,
      NULL
      );
   fulldst->DstRegister.WriteMask = convert_writemask( inst->DstReg.WriteMask );

   for( i = 0; i < fullinst->Instruction.NumSrcRegs; i++ ) {
      GLuint j;

      fullsrc = &fullinst->FullSrcRegisters[i];
      fullsrc->SrcRegister.File = map_register_file( inst->SrcReg[i].File );
      fullsrc->SrcRegister.Index = map_register_file_index(
         fullsrc->SrcRegister.File,
         inst->SrcReg[i].Index,
         inputMapping,
         outputMapping,
         immediateMapping);

      for( j = 0; j < 4; j++ ) {
         GLuint swz;

         swz = GET_SWZ( inst->SrcReg[i].Swizzle, j );
         if( swz > SWIZZLE_W ) {
            tgsi_util_set_src_register_extswizzle(
               &fullsrc->SrcRegisterExtSwz,
               swz,
               j );
         }
         else {
            tgsi_util_set_src_register_swizzle(
               &fullsrc->SrcRegister,
               swz,
               j );
         }
      }

      if( inst->SrcReg[i].NegateBase == NEGATE_XYZW ) {
         fullsrc->SrcRegister.Negate = 1;
      }
      else if( inst->SrcReg[i].NegateBase != NEGATE_NONE ) {
         if( inst->SrcReg[i].NegateBase & NEGATE_X ) {
            fullsrc->SrcRegisterExtSwz.NegateX = 1;
         }
         if( inst->SrcReg[i].NegateBase & NEGATE_Y ) {
            fullsrc->SrcRegisterExtSwz.NegateY = 1;
         }
         if( inst->SrcReg[i].NegateBase & NEGATE_Z ) {
            fullsrc->SrcRegisterExtSwz.NegateZ = 1;
         }
         if( inst->SrcReg[i].NegateBase & NEGATE_W ) {
            fullsrc->SrcRegisterExtSwz.NegateW = 1;
         }
      }

      if( inst->SrcReg[i].Abs ) {
         fullsrc->SrcRegisterExtMod.Absolute = 1;
      }

      if( inst->SrcReg[i].NegateAbs ) {
         fullsrc->SrcRegisterExtMod.Negate = 1;
      }

      if( inst->SrcReg[i].RelAddr ) {
         fullsrc->SrcRegister.Indirect = 1;

         fullsrc->SrcRegisterInd.File = TGSI_FILE_ADDRESS;
         fullsrc->SrcRegisterInd.Index = 0;
      }
   }

   switch( inst->Opcode ) {
   case OPCODE_ARL:
      fullinst->Instruction.Opcode = TGSI_OPCODE_ARL;
      break;
   case OPCODE_ABS:
      fullinst->Instruction.Opcode = TGSI_OPCODE_ABS;
      break;
   case OPCODE_ADD:
      fullinst->Instruction.Opcode = TGSI_OPCODE_ADD;
      break;
   case OPCODE_BGNLOOP:
      fullinst->Instruction.Opcode = TGSI_OPCODE_BGNLOOP2;
      fullinst->InstructionExtLabel.Label = inst->BranchTarget + preamble_size;
      break;
   case OPCODE_BGNSUB:
      fullinst->Instruction.Opcode = TGSI_OPCODE_BGNSUB;
      break;
   case OPCODE_BRA:
      fullinst->Instruction.Opcode = TGSI_OPCODE_BRA;
      break;
   case OPCODE_BRK:
      fullinst->Instruction.Opcode = TGSI_OPCODE_BRK;
      break;
   case OPCODE_CAL:
      fullinst->Instruction.Opcode = TGSI_OPCODE_CAL;
      fullinst->InstructionExtLabel.Label = inst->BranchTarget + preamble_size;
      break;
   case OPCODE_CMP:
      fullinst->Instruction.Opcode = TGSI_OPCODE_CMP;
      break;
   case OPCODE_CONT:
      fullinst->Instruction.Opcode = TGSI_OPCODE_CONT;
      break;
   case OPCODE_COS:
      fullinst->Instruction.Opcode = TGSI_OPCODE_COS;
      break;
   case OPCODE_DDX:
      fullinst->Instruction.Opcode = TGSI_OPCODE_DDX;
      break;
   case OPCODE_DDY:
      fullinst->Instruction.Opcode = TGSI_OPCODE_DDY;
      break;
   case OPCODE_DP3:
      fullinst->Instruction.Opcode = TGSI_OPCODE_DP3;
      break;
   case OPCODE_DP4:
      fullinst->Instruction.Opcode = TGSI_OPCODE_DP4;
      break;
   case OPCODE_DPH:
      fullinst->Instruction.Opcode = TGSI_OPCODE_DPH;
      break;
   case OPCODE_DST:
      fullinst->Instruction.Opcode = TGSI_OPCODE_DST;
      break;
   case OPCODE_ELSE:
      fullinst->Instruction.Opcode = TGSI_OPCODE_ELSE;
      fullinst->InstructionExtLabel.Label = inst->BranchTarget + preamble_size;
      break;
   case OPCODE_ENDIF:
      fullinst->Instruction.Opcode = TGSI_OPCODE_ENDIF;
      break;
   case OPCODE_ENDLOOP:
      fullinst->Instruction.Opcode = TGSI_OPCODE_ENDLOOP2;
      fullinst->InstructionExtLabel.Label = inst->BranchTarget + preamble_size;
      break;
   case OPCODE_ENDSUB:
      fullinst->Instruction.Opcode = TGSI_OPCODE_ENDSUB;
      break;
   case OPCODE_EX2:
      fullinst->Instruction.Opcode = TGSI_OPCODE_EX2;
      break;
   case OPCODE_EXP:
      fullinst->Instruction.Opcode = TGSI_OPCODE_EXP;
      break;
   case OPCODE_FLR:
      fullinst->Instruction.Opcode = TGSI_OPCODE_FLR;
      break;
   case OPCODE_FRC:
      fullinst->Instruction.Opcode = TGSI_OPCODE_FRC;
      break;
   case OPCODE_IF:
      fullinst->Instruction.Opcode = TGSI_OPCODE_IF;
      fullinst->InstructionExtLabel.Label = inst->BranchTarget + preamble_size;
      break;
   case OPCODE_INT:
      fullinst->Instruction.Opcode = TGSI_OPCODE_INT;
      break;
   case OPCODE_KIL:
      /* predicated w/ a register */
      fullinst->Instruction.Opcode = TGSI_OPCODE_KILP;
      break;
   case OPCODE_KIL_NV:
      /* unpredicated */
      assert(inst->DstReg.CondMask == COND_TR);
      fullinst->Instruction.Opcode = TGSI_OPCODE_KIL;
      break;
   case OPCODE_LG2:
      fullinst->Instruction.Opcode = TGSI_OPCODE_LG2;
      break;
   case OPCODE_LOG:
      fullinst->Instruction.Opcode = TGSI_OPCODE_LOG;
      break;
   case OPCODE_LIT:
      fullinst->Instruction.Opcode = TGSI_OPCODE_LIT;
      break;
   case OPCODE_LRP:
      fullinst->Instruction.Opcode = TGSI_OPCODE_LRP;
      break;
   case OPCODE_MAD:
      fullinst->Instruction.Opcode = TGSI_OPCODE_MAD;
      break;
   case OPCODE_MAX:
      fullinst->Instruction.Opcode = TGSI_OPCODE_MAX;
      break;
   case OPCODE_MIN:
      fullinst->Instruction.Opcode = TGSI_OPCODE_MIN;
      break;
   case OPCODE_MOV:
      fullinst->Instruction.Opcode = TGSI_OPCODE_MOV;
      break;
   case OPCODE_MUL:
      fullinst->Instruction.Opcode = TGSI_OPCODE_MUL;
      break;
   case OPCODE_NOISE1:
      fullinst->Instruction.Opcode = TGSI_OPCODE_NOISE1;
      break;
   case OPCODE_NOISE2:
      fullinst->Instruction.Opcode = TGSI_OPCODE_NOISE2;
      break;
   case OPCODE_NOISE3:
      fullinst->Instruction.Opcode = TGSI_OPCODE_NOISE3;
      break;
   case OPCODE_NOISE4:
      fullinst->Instruction.Opcode = TGSI_OPCODE_NOISE4;
      break;
   case OPCODE_NOP:
      fullinst->Instruction.Opcode = TGSI_OPCODE_NOP;
      break;
   case OPCODE_POW:
      fullinst->Instruction.Opcode = TGSI_OPCODE_POW;
      break;
   case OPCODE_RCP:
      fullinst->Instruction.Opcode = TGSI_OPCODE_RCP;
      break;
   case OPCODE_RET:
      fullinst->Instruction.Opcode = TGSI_OPCODE_RET;
      break;
   case OPCODE_RSQ:
      fullinst->Instruction.Opcode = TGSI_OPCODE_RSQ;
      tgsi_util_set_full_src_register_sign_mode(
         &fullinst->FullSrcRegisters[0],
         TGSI_UTIL_SIGN_CLEAR );
      break;
   case OPCODE_SCS:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SCS;
      fulldst->DstRegister.WriteMask &= TGSI_WRITEMASK_XY;
      break;
   case OPCODE_SEQ:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SEQ;
      break;
   case OPCODE_SGE:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SGE;
      break;
   case OPCODE_SGT:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SGT;
      break;
   case OPCODE_SIN:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SIN;
      break;
   case OPCODE_SLE:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SLE;
      break;
   case OPCODE_SLT:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SLT;
      break;
   case OPCODE_SNE:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SNE;
      break;
   case OPCODE_SUB:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SUB;
      break;
   case OPCODE_SWZ:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SWZ;
      break;
   case OPCODE_TEX:
      /* ordinary texture lookup */
      fullinst->Instruction.Opcode = TGSI_OPCODE_TEX;
      fullinst->Instruction.NumSrcRegs = 2;
      fullinst->InstructionExtTexture.Texture = map_texture_target( inst->TexSrcTarget );
      fullinst->FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_SAMPLER;
      fullinst->FullSrcRegisters[1].SrcRegister.Index = inst->TexSrcUnit;
      break;
   case OPCODE_TXB:
      /* texture lookup with LOD bias */
      fullinst->Instruction.Opcode = TGSI_OPCODE_TXB;
      fullinst->Instruction.NumSrcRegs = 2;
      fullinst->InstructionExtTexture.Texture = map_texture_target( inst->TexSrcTarget );
      fullinst->FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_SAMPLER;
      fullinst->FullSrcRegisters[1].SrcRegister.Index = inst->TexSrcUnit;
      break;
   case OPCODE_TXD:
      /* texture lookup with explicit partial derivatives */
      fullinst->Instruction.Opcode = TGSI_OPCODE_TXD;
      fullinst->Instruction.NumSrcRegs = 4;
      fullinst->InstructionExtTexture.Texture = map_texture_target( inst->TexSrcTarget );
      /* src[0] = coord, src[1] = d[strq]/dx, src[2] = d[strq]/dy */
      fullinst->FullSrcRegisters[3].SrcRegister.File = TGSI_FILE_SAMPLER;
      fullinst->FullSrcRegisters[3].SrcRegister.Index = inst->TexSrcUnit;
      break;
   case OPCODE_TXL:
      /* texture lookup with explicit LOD */
      fullinst->Instruction.Opcode = TGSI_OPCODE_TXL;
      fullinst->Instruction.NumSrcRegs = 2;
      fullinst->InstructionExtTexture.Texture = map_texture_target( inst->TexSrcTarget );
      fullinst->FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_SAMPLER;
      fullinst->FullSrcRegisters[1].SrcRegister.Index = inst->TexSrcUnit;
      break;
   case OPCODE_TXP:
      /* texture lookup with divide by Q component */
      /* convert to TEX w/ special flag for division */
      fullinst->Instruction.Opcode = TGSI_OPCODE_TEX;
      fullinst->Instruction.NumSrcRegs = 2;
      fullinst->InstructionExtTexture.Texture = map_texture_target( inst->TexSrcTarget );
      fullinst->FullSrcRegisters[0].SrcRegisterExtSwz.ExtDivide = TGSI_EXTSWIZZLE_W;
      fullinst->FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_SAMPLER;
      fullinst->FullSrcRegisters[1].SrcRegister.Index = inst->TexSrcUnit;
      break;
   case OPCODE_XPD:
      fullinst->Instruction.Opcode = TGSI_OPCODE_XPD;
      fulldst->DstRegister.WriteMask &= TGSI_WRITEMASK_XYZ;
      break;
   case OPCODE_END:
      fullinst->Instruction.Opcode = TGSI_OPCODE_RET;
      break;
   default:
      assert( 0 );
   }
}

/**
 * \param usage_mask  bitfield of TGSI_WRITEMASK_{XYZW} tokens
 */
static struct tgsi_full_declaration
make_input_decl(
   GLuint index,
   GLboolean interpolate_info,
   GLuint interpolate,
   GLuint usage_mask,
   GLboolean semantic_info,
   GLuint semantic_name,
   GLbitfield semantic_index )
{
   struct tgsi_full_declaration decl;

   assert(semantic_name < TGSI_SEMANTIC_COUNT);

   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_INPUT;
   decl.Declaration.Declare = TGSI_DECLARE_RANGE;
   decl.Declaration.UsageMask = usage_mask;
   decl.Declaration.Semantic = semantic_info;
   decl.u.DeclarationRange.First = index;
   decl.u.DeclarationRange.Last = index;
   if (semantic_info) {
      decl.Semantic.SemanticName = semantic_name;
      decl.Semantic.SemanticIndex = semantic_index;
   }
   if (interpolate_info) {
      decl.Declaration.Interpolate = 1;
      decl.Interpolation.Interpolate = interpolate;
   }

   return decl;
}

/**
 * \param usage_mask  bitfield of TGSI_WRITEMASK_{XYZW} tokens
 */
static struct tgsi_full_declaration
make_output_decl(
   GLuint index,
   GLuint semantic_name,
   GLuint semantic_index,
   GLbitfield usage_mask )
{
   struct tgsi_full_declaration decl;

   assert(semantic_name < TGSI_SEMANTIC_COUNT);

   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_OUTPUT;
   decl.Declaration.Declare = TGSI_DECLARE_RANGE;
   decl.Declaration.UsageMask = usage_mask;
   decl.Declaration.Semantic = 1;
   decl.u.DeclarationRange.First = index;
   decl.u.DeclarationRange.Last = index;
   decl.Semantic.SemanticName = semantic_name;
   decl.Semantic.SemanticIndex = semantic_index;

   return decl;
}


static struct tgsi_full_declaration
make_temp_decl(
   GLuint start_index,
   GLuint end_index )
{
   struct tgsi_full_declaration decl;
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_TEMPORARY;
   decl.Declaration.Declare = TGSI_DECLARE_RANGE;
   decl.u.DeclarationRange.First = start_index;
   decl.u.DeclarationRange.Last = end_index;
   return decl;
}


/**
 * Find the temporaries which are used in the given program.
 */
static void
find_temporaries(const struct gl_program *program,
                 GLboolean tempsUsed[MAX_PROGRAM_TEMPS])
{
   GLuint i, j;

   for (i = 0; i < MAX_PROGRAM_TEMPS; i++)
      tempsUsed[i] = GL_FALSE;

   for (i = 0; i < program->NumInstructions; i++) {
      const struct prog_instruction *inst = program->Instructions + i;
      const GLuint n = _mesa_num_inst_src_regs( inst->Opcode );
      for (j = 0; j < n; j++) {
         if (inst->SrcReg[j].File == PROGRAM_TEMPORARY)
            tempsUsed[inst->SrcReg[j].Index] = GL_TRUE;
         if (inst->DstReg.File == PROGRAM_TEMPORARY)
            tempsUsed[inst->DstReg.Index] = GL_TRUE;
      }
   }
}




/**
 * Translate Mesa program to TGSI format.
 * \param program  the program to translate
 * \param numInputs  number of input registers used
 * \param inputMapping  maps Mesa fragment program inputs to TGSI generic
 *                      input indexes
 * \param inputSemanticName  the TGSI_SEMANTIC flag for each input
 * \param inputSemanticIndex  the semantic index (ex: which texcoord) for each input
 * \param interpMode  the TGSI_INTERPOLATE_LINEAR/PERSP mode for each input

 * \param numOutputs  number of output registers used
 * \param outputMapping  maps Mesa fragment program outputs to TGSI
 *                       generic outputs
 * \param outputSemanticName  the TGSI_SEMANTIC flag for each output
 * \param outputSemanticIndex  the semantic index (ex: which texcoord) for each output
 * \param tokens  array to store translated tokens in
 * \param maxTokens  size of the tokens array
 *
 */
GLboolean
tgsi_translate_mesa_program(
   uint procType,
   const struct gl_program *program,
   GLuint numInputs,
   const GLuint inputMapping[],
   const ubyte inputSemanticName[],
   const ubyte inputSemanticIndex[],
   const GLuint interpMode[],
   GLuint numOutputs,
   const GLuint outputMapping[],
   const ubyte outputSemanticName[],
   const ubyte outputSemanticIndex[],
   struct tgsi_token *tokens,
   GLuint maxTokens )
{
   GLuint i;
   GLuint ti;  /* token index */
   struct tgsi_header *header;
   struct tgsi_processor *processor;
   struct tgsi_full_instruction fullinst;
   GLuint preamble_size = 0;
   GLuint immediates[1000];
   GLuint numImmediates = 0;

   assert(procType == TGSI_PROCESSOR_FRAGMENT ||
          procType == TGSI_PROCESSOR_VERTEX);

   *(struct tgsi_version *) &tokens[0] = tgsi_build_version();

   header = (struct tgsi_header *) &tokens[1];
   *header = tgsi_build_header();

   processor = (struct tgsi_processor *) &tokens[2];
   *processor = tgsi_build_processor( procType, header );

   ti = 3;

   /*
    * Declare input attributes.
    */
   if (procType == TGSI_PROCESSOR_FRAGMENT) {
      for (i = 0; i < numInputs; i++) {
         struct tgsi_full_declaration fulldecl;
         switch (inputSemanticName[i]) {
         case TGSI_SEMANTIC_POSITION:
            /* Fragment XY pos */
            fulldecl = make_input_decl(i,
                                       GL_TRUE, TGSI_INTERPOLATE_CONSTANT,
                                       TGSI_WRITEMASK_XY,
                                       GL_TRUE, TGSI_SEMANTIC_POSITION, 0 );
            ti += tgsi_build_full_declaration(
                                              &fulldecl,
                                              &tokens[ti],
                                              header,
                                              maxTokens - ti );
            /* Fragment ZW pos */
            fulldecl = make_input_decl(i,
                                       GL_TRUE, TGSI_INTERPOLATE_LINEAR,
                                       TGSI_WRITEMASK_ZW,
                                       GL_TRUE, TGSI_SEMANTIC_POSITION, 0 );
            ti += tgsi_build_full_declaration(&fulldecl,
                                              &tokens[ti],
                                              header,
                                              maxTokens - ti );
            break;
         default:
            fulldecl = make_input_decl(i,
                                       GL_TRUE, interpMode[i],
                                       TGSI_WRITEMASK_XYZW,
                                       GL_TRUE, inputSemanticName[i],
                                       inputSemanticIndex[i]);
            ti += tgsi_build_full_declaration(&fulldecl,
                                              &tokens[ti],
                                              header,
                                              maxTokens - ti );
            break;
         }
      }
   }
   else {
      /* vertex prog */
      for (i = 0; i < numInputs; i++) {
         struct tgsi_full_declaration fulldecl;
         fulldecl = make_input_decl(i,
                                    GL_FALSE, 0,
                                    TGSI_WRITEMASK_XYZW,
                                    GL_FALSE, 0, 0);
         ti += tgsi_build_full_declaration(&fulldecl,
                                           &tokens[ti],
                                           header,
                                           maxTokens - ti );
      }
   }

   /*
    * Declare output attributes.
    */
   if (procType == TGSI_PROCESSOR_FRAGMENT) {
      for (i = 0; i < numOutputs; i++) {
         struct tgsi_full_declaration fulldecl;
         switch (outputSemanticName[i]) {
         case TGSI_SEMANTIC_POSITION:
            fulldecl = make_output_decl(i,
                                        TGSI_SEMANTIC_POSITION, 0, /* Z / Depth */
                                        TGSI_WRITEMASK_Z );
            break;
         case TGSI_SEMANTIC_COLOR:
            fulldecl = make_output_decl(i,
                                        TGSI_SEMANTIC_COLOR, 0,
                                        TGSI_WRITEMASK_XYZW );
            break;
         default:
            abort();
         }
         ti += tgsi_build_full_declaration(&fulldecl,
                                           &tokens[ti],
                                           header,
                                           maxTokens - ti );
      }
   }
   else {
      /* vertex prog */
      for (i = 0; i < numOutputs; i++) {
         struct tgsi_full_declaration fulldecl;
         fulldecl = make_output_decl(i,
                                     outputSemanticName[i],
                                     outputSemanticIndex[i],
                                     TGSI_WRITEMASK_XYZW );
         ti += tgsi_build_full_declaration(&fulldecl,
                                           &tokens[ti],
                                           header,
                                           maxTokens - ti );
      }
   }

   /* temporary decls */
   {
      GLboolean tempsUsed[MAX_PROGRAM_TEMPS + 1];
      GLboolean inside_range = GL_FALSE;
      GLuint start_range;

      find_temporaries(program, tempsUsed);
      tempsUsed[MAX_PROGRAM_TEMPS] = GL_FALSE;
      for (i = 0; i < MAX_PROGRAM_TEMPS + 1; i++) {
         if (tempsUsed[i] && !inside_range) {
            inside_range = GL_TRUE;
            start_range = i;
         }
         else if (!tempsUsed[i] && inside_range) {
            struct tgsi_full_declaration fulldecl;

            inside_range = GL_FALSE;
            fulldecl = make_temp_decl( start_range, i - 1 );
            ti += tgsi_build_full_declaration(
                                              &fulldecl,
                                              &tokens[ti],
                                              header,
                                              maxTokens - ti );
         }
      }
   }

   /* immediates/literals */
   for (i = 0; i < program->Parameters->NumParameters; i++) {
      if (program->Parameters->Parameters[i].Type == PROGRAM_CONSTANT) {
         struct tgsi_full_immediate fullimm
            = make_immediate(program->Parameters->ParameterValues[i],
                             program->Parameters->Parameters[i].Size);
         ti += tgsi_build_full_immediate(&fullimm,
                                         &tokens[ti],
                                         header,
                                         maxTokens - ti);
         immediates[i] = numImmediates;
         numImmediates++;
      }
   }

   for( i = 0; i < program->NumInstructions; i++ ) {
      compile_instruction(
            &program->Instructions[i],
            &fullinst,
            inputMapping,
            outputMapping,
            immediates,
            preamble_size,
            procType );

      ti += tgsi_build_full_instruction(
         &fullinst,
         &tokens[ti],
         header,
         maxTokens - ti );
   }

   return GL_TRUE;
}


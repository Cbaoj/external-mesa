#include "tgsi_platform.h"
#include "tgsi_core.h"

struct text_dump
{
    FILE *file;
    GLuint tabs;
};

static void
text_dump_write(
   struct text_dump *dump,
   const void *buffer,
   GLuint size )
{
   fwrite( buffer, size, 1, dump->file );
}

static void
text_dump_str(
   struct text_dump *dump,
   const char *str )
{
   GLuint i;
   GLuint len = strlen( str );

   for( i = 0; i < len; i++ ) {
      text_dump_write( dump, &str[i], 1 );

      if( str[i] == '\n' ) {
         GLuint i;

         for( i = 0; i < dump->tabs; i++ ) {
            text_dump_write( dump, "    ", 4 );
         }
      }
   }
}

static void
text_dump_chr(
   struct text_dump *dump,
   const char chr )
{
   char str[2];

   str[0] = chr;
   str[1] = '\0';
   text_dump_str( dump, str );
}

static void
text_dump_uix(
   struct text_dump *dump,
   const GLuint ui)
{
   char str[36];

   sprintf( str, "0x%x", ui );
   text_dump_str( dump, str );
}

static void
text_dump_uid(
   struct text_dump *dump,
   const GLuint ui )
{
   char str[16];

   sprintf( str, "%u", ui );
   text_dump_str( dump, str );
}

static void
text_dump_sid(
   struct text_dump *dump,
   const GLint si )
{
   char str[16];

   sprintf( str, "%d", si );
   text_dump_str( dump, str );
}

static void
text_dump_flt(
   struct text_dump *dump,
   const GLfloat f )
{
   char str[48];

   sprintf( str, "%40.6f", f );
   text_dump_str( dump, str );
}

static void
text_dump_enum(
   struct text_dump *dump,
   const GLuint e,
   const char **enums,
   const GLuint enums_count )
{
   if( e >= enums_count ) {
      text_dump_uid( dump, e );
   }
   else {
      text_dump_str( dump, enums[e] );
   }
}

static void
text_dump_tab(
   struct text_dump *dump )
{
   dump->tabs++;
}

static void
text_dump_untab(
   struct text_dump *dump )
{
   assert( dump->tabs > 0 );

   --dump->tabs;
}

#define TXT(S)          text_dump_str( dump, S )
#define CHR(C)          text_dump_chr( dump, C )
#define UIX(I)          text_dump_uix( dump, I )
#define UID(I)          text_dump_uid( dump, I )
#define SID(I)          text_dump_sid( dump, I )
#define FLT(F)          text_dump_flt( dump, F )
#define TAB()           text_dump_tab( dump )
#define UNT()           text_dump_untab( dump )
#define ENM(E,ENUMS)    text_dump_enum( dump, E, ENUMS, sizeof( ENUMS ) / sizeof( *ENUMS ) )

static const char *TGSI_PROCESSOR_TYPES[] =
{
   "PROCESSOR_FRAGMENT",
   "PROCESSOR_VERTEX",
   "PROCESSOR_GEOMETRY"
};

static const char *TGSI_PROCESSOR_TYPES_SHORT[] =
{
   "FRAG",
   "VERT",
   "GEOM"
};

static const char *TGSI_TOKEN_TYPES[] =
{
   "TOKEN_TYPE_DECLARATION",
   "TOKEN_TYPE_IMMEDIATE",
   "TOKEN_TYPE_INSTRUCTION"
};

static const char *TGSI_FILES[] =
{
   "FILE_NULL",
   "FILE_CONSTANT",
   "FILE_INPUT",
   "FILE_OUTPUT",
   "FILE_TEMPORARY",
   "FILE_SAMPLER",
   "FILE_ADDRESS",
   "FILE_IMMEDIATE"
};

static const char *TGSI_FILES_SHORT[] =
{
   "NULL",
   "CONST",
   "IN",
   "OUT",
   "TEMP",
   "SAMP",
   "ADDR",
   "IMM"
};

static const char *TGSI_DECLARES[] =
{
   "DECLARE_RANGE",
   "DECLARE_MASK"
};

static const char *TGSI_INTERPOLATES[] =
{
   "INTERPOLATE_CONSTANT",
   "INTERPOLATE_LINEAR",
   "INTERPOLATE_PERSPECTIVE"
};

static const char *TGSI_INTERPOLATES_SHORT[] =
{
   "CONSTANT",
   "LINEAR",
   "PERSPECTIVE"
};

static const char *TGSI_SEMANTICS[] =
{
   "SEMANTIC_DEPTH",
   "SEMANTIC_COLOR"
};

static const char *TGSI_SEMANTICS_SHORT[] =
{
   "DEPTH",
   "COLOR"
};

static const char *TGSI_IMMS[] =
{
   "IMM_FLOAT32"
};

static const char *TGSI_IMMS_SHORT[] =
{
   "FLT32"
};

static const char *TGSI_OPCODES[] =
{
   "OPCODE_ARL",
   "OPCODE_MOV",
   "OPCODE_LIT",
   "OPCODE_RCP",
   "OPCODE_RSQ",
   "OPCODE_EXP",
   "OPCODE_LOG",
   "OPCODE_MUL",
   "OPCODE_ADD",
   "OPCODE_DP3",
   "OPCODE_DP4",
   "OPCODE_DST",
   "OPCODE_MIN",
   "OPCODE_MAX",
   "OPCODE_SLT",
   "OPCODE_SGE",
   "OPCODE_MAD",
   "OPCODE_SUB",
   "OPCODE_LERP",
   "OPCODE_CND",
   "OPCODE_CND0",
   "OPCODE_DOT2ADD",
   "OPCODE_INDEX",
   "OPCODE_NEGATE",
   "OPCODE_FRAC",
   "OPCODE_CLAMP",
   "OPCODE_FLOOR",
   "OPCODE_ROUND",
   "OPCODE_EXPBASE2",
   "OPCODE_LOGBASE2",
   "OPCODE_POWER",
   "OPCODE_CROSSPRODUCT",
   "OPCODE_MULTIPLYMATRIX",
   "OPCODE_ABS",
   "OPCODE_RCC",
   "OPCODE_DPH",
   "OPCODE_COS",
   "OPCODE_DDX",
   "OPCODE_DDY",
   "OPCODE_KIL",
   "OPCODE_PK2H",
   "OPCODE_PK2US",
   "OPCODE_PK4B",
   "OPCODE_PK4UB",
   "OPCODE_RFL",
   "OPCODE_SEQ",
   "OPCODE_SFL",
   "OPCODE_SGT",
   "OPCODE_SIN",
   "OPCODE_SLE",
   "OPCODE_SNE",
   "OPCODE_STR",
   "OPCODE_TEX",
   "OPCODE_TXD",
   "OPCODE_UP2H",
   "OPCODE_UP2US",
   "OPCODE_UP4B",
   "OPCODE_UP4UB",
   "OPCODE_X2D",
   "OPCODE_ARA",
   "OPCODE_ARR",
   "OPCODE_BRA",
   "OPCODE_CAL",
   "OPCODE_RET",
   "OPCODE_SSG",
   "OPCODE_CMP",
   "OPCODE_SCS",
   "OPCODE_TXB",
   "OPCODE_NRM",
   "OPCODE_DIV",
   "OPCODE_DP2",
   "OPCODE_TXL",
   "OPCODE_BRK",
   "OPCODE_IF",
   "OPCODE_LOOP",
   "OPCODE_REP",
   "OPCODE_ELSE",
   "OPCODE_ENDIF",
   "OPCODE_ENDLOOP",
   "OPCODE_ENDREP",
   "OPCODE_PUSHA",
   "OPCODE_POPA",
   "OPCODE_CEIL",
   "OPCODE_I2F",
   "OPCODE_NOT",
   "OPCODE_TRUNC",
   "OPCODE_SHL",
   "OPCODE_SHR",
   "OPCODE_AND",
   "OPCODE_OR",
   "OPCODE_MOD",
   "OPCODE_XOR",
   "OPCODE_SAD",
   "OPCODE_TXF",
   "OPCODE_TXQ",
   "OPCODE_CONT",
   "OPCODE_EMIT",
   "OPCODE_ENDPRIM",
   "OPCODE_BGNLOOP2",
   "OPCODE_BGNSUB",
   "OPCODE_ENDLOOP2",
   "OPCODE_ENDSUB",
   "OPCODE_NOISE1",
   "OPCODE_NOISE2",
   "OPCODE_NOISE3",
   "OPCODE_NOISE4",
   "OPCODE_NOP",
   "OPCODE_TEXBEM",
   "OPCODE_TEXBEML",
   "OPCODE_TEXREG2AR",
   "OPCODE_TEXM3X2PAD",
   "OPCODE_TEXM3X2TEX",
   "OPCODE_TEXM3X3PAD",
   "OPCODE_TEXM3X3TEX",
   "OPCODE_TEXM3X3SPEC",
   "OPCODE_TEXM3X3VSPEC",
   "OPCODE_TEXREG2GB",
   "OPCODE_TEXREG2RGB",
   "OPCODE_TEXDP3TEX",
   "OPCODE_TEXDP3",
   "OPCODE_TEXM3X3",
   "OPCODE_TEXM3X2DEPTH",
   "OPCODE_TEXDEPTH",
   "OPCODE_BEM",
   "OPCODE_M4X3",
   "OPCODE_M3X4",
   "OPCODE_M3X3",
   "OPCODE_M3X2",
   "OPCODE_NRM4",
   "OPCODE_CALLNZ",
   "OPCODE_IFC",
   "OPCODE_BREAKC",
   "OPCODE_TXP"
};

static const char *TGSI_OPCODES_SHORT[] =
{
   "ARL",
   "MOV",
   "LIT",
   "RCP",
   "RSQ",
   "EXP",
   "LOG",
   "MUL",
   "ADD",
   "DP3",
   "DP4",
   "DST",
   "MIN",
   "MAX",
   "SLT",
   "SGE",
   "MAD",
   "SUB",
   "LERP",
   "CND",
   "CND0",
   "DOT2ADD",
   "INDEX",
   "NEGATE",
   "FRAC",
   "CLAMP",
   "FLOOR",
   "ROUND",
   "EXPBASE2",
   "LOGBASE2",
   "POWER",
   "CROSSPRODUCT",
   "MULTIPLYMATRIX",
   "ABS",
   "RCC",
   "DPH",
   "COS",
   "DDX",
   "DDY",
   "KIL",
   "PK2H",
   "PK2US",
   "PK4B",
   "PK4UB",
   "RFL",
   "SEQ",
   "SFL",
   "SGT",
   "SIN",
   "SLE",
   "SNE",
   "STR",
   "TEX",
   "TXD",
   "UP2H",
   "UP2US",
   "UP4B",
   "UP4UB",
   "X2D",
   "ARA",
   "ARR",
   "BRA",
   "CAL",
   "RET",
   "SSG",
   "CMP",
   "SCS",
   "TXB",
   "NRM",
   "DIV",
   "DP2",
   "TXL",
   "BRK",
   "IF",
   "LOOP",
   "REP",
   "ELSE",
   "ENDIF",
   "ENDLOOP",
   "ENDREP",
   "PUSHA",
   "POPA",
   "CEIL",
   "I2F",
   "NOT",
   "TRUNC",
   "SHL",
   "SHR",
   "AND",
   "OR",
   "MOD",
   "XOR",
   "SAD",
   "TXF",
   "TXQ",
   "CONT",
   "EMIT",
   "ENDPRIM",
   "BGNLOOP2",
   "BGNSUB",
   "ENDLOOP2",
   "ENDSUB",
   "NOISE1",
   "NOISE2",
   "NOISE3",
   "NOISE4",
   "NOP",
   "TEXBEM",
   "TEXBEML",
   "TEXREG2AR",
   "TEXM3X2PAD",
   "TEXM3X2TEX",
   "TEXM3X3PAD",
   "TEXM3X3TEX",
   "TEXM3X3SPEC",
   "TEXM3X3VSPEC",
   "TEXREG2GB",
   "TEXREG2RGB",
   "TEXDP3TEX",
   "TEXDP3",
   "TEXM3X3",
   "TEXM3X2DEPTH",
   "TEXDEPTH",
   "BEM",
   "M4X3",
   "M3X4",
   "M3X3",
   "M3X2",
   "NRM4",
   "CALLNZ",
   "IFC",
   "BREAKC",
};

static const char *TGSI_SATS[] =
{
   "SAT_NONE",
   "SAT_ZERO_ONE",
   "SAT_MINUS_PLUS_ONE"
};

static const char *TGSI_INSTRUCTION_EXTS[] =
{
   "INSTRUCTION_EXT_TYPE_NV",
   "INSTRUCTION_EXT_TYPE_LABEL",
   "INSTRUCTION_EXT_TYPE_TEXTURE"
};

static const char *TGSI_PRECISIONS[] =
{
   "PRECISION_DEFAULT",
   "TGSI_PRECISION_FLOAT32",
   "TGSI_PRECISION_FLOAT16",
   "TGSI_PRECISION_FIXED12"
};

static const char *TGSI_CCS[] =
{
   "CC_GT",
   "CC_EQ",
   "CC_LT",
   "CC_UN",
   "CC_GE",
   "CC_LE",
   "CC_NE",
   "CC_TR",
   "CC_FL"
};

static const char *TGSI_SWIZZLES[] =
{
   "SWIZZLE_X",
   "SWIZZLE_Y",
   "SWIZZLE_Z",
   "SWIZZLE_W"
};

static const char *TGSI_SWIZZLES_SHORT[] =
{
   "x",
   "y",
   "z",
   "w"
};

static const char *TGSI_TEXTURES[] =
{
   "TEXTURE_UNKNOWN",
   "TEXTURE_1D",
   "TEXTURE_2D",
   "TEXTURE_3D",
   "TEXTURE_CUBE",
   "TEXTURE_RECT",
   "TEXTURE_SHADOW1D",
   "TEXTURE_SHADOW2D",
   "TEXTURE_SHADOWRECT"
};

static const char *TGSI_SRC_REGISTER_EXTS[] =
{
   "SRC_REGISTER_EXT_TYPE_SWZ",
   "SRC_REGISTER_EXT_TYPE_MOD"
};

static const char *TGSI_EXTSWIZZLES[] =
{
   "EXTSWIZZLE_X",
   "EXTSWIZZLE_Y",
   "EXTSWIZZLE_Z",
   "EXTSWIZZLE_W",
   "EXTSWIZZLE_ZERO",
   "EXTSWIZZLE_ONE"
};

static const char *TGSI_WRITEMASKS[] =
{
   "0",
   "WRITEMASK_X",
   "WRITEMASK_Y",
   "WRITEMASK_XY",
   "WRITEMASK_Z",
   "WRITEMASK_XZ",
   "WRITEMASK_YZ",
   "WRITEMASK_XYZ",
   "WRITEMASK_W",
   "WRITEMASK_XW",
   "WRITEMASK_YW",
   "WRITEMASK_XYW",
   "WRITEMASK_ZW",
   "WRITEMASK_XZW",
   "WRITEMASK_YZW",
   "WRITEMASK_XYZW"
};

static const char *TGSI_DST_REGISTER_EXTS[] =
{
   "DST_REGISTER_EXT_TYPE_CONDCODE",
   "DST_REGISTER_EXT_TYPE_MODULATE"
};

static const char *TGSI_MODULATES[] =
{
   "MODULATE_1X",
   "MODULATE_2X",
   "MODULATE_4X",
   "MODULATE_8X",
   "MODULATE_HALF",
   "MODULATE_QUARTER",
   "MODULATE_EIGHTH"
};

static void
dump_declaration_short(
   struct text_dump *dump,
   struct tgsi_full_declaration *decl )
{
   TXT( "\nDCL " );
   ENM( decl->Declaration.File, TGSI_FILES_SHORT );

   switch( decl->Declaration.Declare ) {
   case TGSI_DECLARE_RANGE:
      CHR( '[' );
      UID( decl->u.DeclarationRange.First );
      if( decl->u.DeclarationRange.First != decl->u.DeclarationRange.Last ) {
         TXT( ".." );
         UID( decl->u.DeclarationRange.Last );
      }
      CHR( ']' );
      break;
   default:
      assert( 0 );
   }

   if( decl->Declaration.Interpolate ) {
      TXT( ", " );
      ENM( decl->Interpolation.Interpolate, TGSI_INTERPOLATES_SHORT );
   }

   if( decl->Declaration.Semantic ) {
      TXT( ", " );
      ENM( decl->Semantic.SemanticName, TGSI_SEMANTICS_SHORT );
      CHR( '[' );
      UID( decl->Semantic.SemanticIndex );
      CHR( ']' );
   }
}

static void
dump_declaration_verbose(
   struct text_dump *dump,
   struct tgsi_full_declaration *decl,
   GLuint ignored,
   GLuint deflt,
   struct tgsi_full_declaration *fd )
{
   TXT( "\nFile       : " );
   ENM( decl->Declaration.File, TGSI_FILES );
   TXT( "\nDeclare    : " );
   ENM( decl->Declaration.Declare, TGSI_DECLARES );
   if( deflt || fd->Declaration.Interpolate != decl->Declaration.Interpolate ) {
      TXT( "\nInterpolate: " );
      UID( decl->Declaration.Interpolate );
   }
   if( deflt || fd->Declaration.Semantic != decl->Declaration.Semantic ) {
      TXT( "\nSemantic   : " );
      UID( decl->Declaration.Semantic );
   }
   if( ignored ) {
      TXT( "\nPadding    : " );
      UIX( decl->Declaration.Padding );
   }

   CHR( '\n' );
   switch( decl->Declaration.Declare ) {
   case TGSI_DECLARE_RANGE:
      TXT( "\nFirst: " );
      UID( decl->u.DeclarationRange.First );
      TXT( "\nLast : " );
      UID( decl->u.DeclarationRange.Last );
      break;

   case TGSI_DECLARE_MASK:
      TXT( "\nMask: " );
      UIX( decl->u.DeclarationMask.Mask );
      break;

   default:
      assert( 0 );
   }

   if( decl->Declaration.Interpolate ) {
      CHR( '\n' );
      TXT( "\nInterpolate: " );
      ENM( decl->Interpolation.Interpolate, TGSI_INTERPOLATES );
      if( ignored ) {
         TXT( "\nPadding    : " );
         UIX( decl->Interpolation.Padding );
      }
   }

   if( decl->Declaration.Semantic ) {
      CHR( '\n' );
      TXT( "\nSemanticName : " );
      ENM( decl->Semantic.SemanticName, TGSI_SEMANTICS );
      TXT( "\nSemanticIndex: " );
      UID( decl->Semantic.SemanticIndex );
      if( ignored ) {
         TXT( "\nPadding      : " );
         UIX( decl->Semantic.Padding );
      }
   }
}

static void
dump_immediate_short(
   struct text_dump *dump,
   struct tgsi_full_immediate *imm )
{
   GLuint i;

   TXT( "\nIMM " );
   ENM( imm->Immediate.DataType, TGSI_IMMS_SHORT );

   TXT( " { " );
   for( i = 0; i < imm->Immediate.Size - 1; i++ ) {
      switch( imm->Immediate.DataType ) {
      case TGSI_IMM_FLOAT32:
         FLT( imm->u.ImmediateFloat32[i].Float );
         break;

      default:
         assert( 0 );
      }

      if( i < imm->Immediate.Size - 2 ) {
         TXT( ", " );
      }
   }
   TXT( " }" );
}

static void
dump_immediate_verbose(
   struct text_dump *dump,
   struct tgsi_full_immediate *imm,
   GLuint ignored )
{
   GLuint i;

   TXT( "\nDataType   : " );
   ENM( imm->Immediate.DataType, TGSI_IMMS );
   if( ignored ) {
      TXT( "\nPadding    : " );
      UIX( imm->Immediate.Padding );
   }

   for( i = 0; i < imm->Immediate.Size - 1; i++ ) {
      CHR( '\n' );
      switch( imm->Immediate.DataType ) {
      case TGSI_IMM_FLOAT32:
         TXT( "\nFloat: " );
         FLT( imm->u.ImmediateFloat32[i].Float );
         break;

      default:
         assert( 0 );
      }
   }
}

static void
dump_instruction_short(
   struct text_dump *dump,
   struct tgsi_full_instruction *inst,
   GLuint instno )
{
   GLuint i;
   GLboolean first_reg = GL_TRUE;

   CHR( '\n' );
   UID( instno );
   CHR( ':' );
   ENM( inst->Instruction.Opcode, TGSI_OPCODES_SHORT );

   switch( inst->Instruction.Saturate ) {
   case TGSI_SAT_NONE:
      break;
   case TGSI_SAT_ZERO_ONE:
      TXT( "_SAT" );
      break;
   case TGSI_SAT_MINUS_PLUS_ONE:
      TXT( "_SAT[-1,1]" );
      break;
   default:
      assert( 0 );
   }

   for( i = 0; i < inst->Instruction.NumDstRegs; i++ ) {
      struct tgsi_full_dst_register *dst = &inst->FullDstRegisters[i];

      if( !first_reg ) {
         CHR( ',' );
      }
      CHR( ' ' );

      ENM( dst->DstRegister.File, TGSI_FILES_SHORT );

      CHR( '[' );
      SID( dst->DstRegister.Index );
      CHR( ']' );

      if( dst->DstRegister.WriteMask != TGSI_WRITEMASK_XYZW ) {
         CHR( '.' );
         if( dst->DstRegister.WriteMask & TGSI_WRITEMASK_X ) {
            CHR( 'x' );
         }
         if( dst->DstRegister.WriteMask & TGSI_WRITEMASK_Y ) {
            CHR( 'y' );
         }
         if( dst->DstRegister.WriteMask & TGSI_WRITEMASK_Z ) {
            CHR( 'z' );
         }
         if( dst->DstRegister.WriteMask & TGSI_WRITEMASK_W ) {
            CHR( 'w' );
         }
      }

      first_reg = GL_FALSE;
   }

   for( i = 0; i < inst->Instruction.NumSrcRegs; i++ ) {
      struct tgsi_full_src_register *src = &inst->FullSrcRegisters[i];

      if( !first_reg ) {
         CHR( ',' );
      }
      CHR( ' ' );

      if( src->SrcRegisterExtMod.Negate  ) {
         CHR( '-' );
      }
      if( src->SrcRegisterExtMod.Absolute ) {
         CHR( '|' );
      }
      if( src->SrcRegister.Negate ) {
         CHR( '-' );
      }

      ENM( src->SrcRegister.File, TGSI_FILES_SHORT );

      CHR( '[' );
      SID( src->SrcRegister.Index );
      CHR( ']' );

      if( src->SrcRegister.SwizzleX != TGSI_SWIZZLE_X ||
            src->SrcRegister.SwizzleY != TGSI_SWIZZLE_Y ||
            src->SrcRegister.SwizzleZ != TGSI_SWIZZLE_Z ||
            src->SrcRegister.SwizzleW != TGSI_SWIZZLE_W ) {
         CHR( '.' );
         ENM( src->SrcRegister.SwizzleX, TGSI_SWIZZLES_SHORT );
         ENM( src->SrcRegister.SwizzleY, TGSI_SWIZZLES_SHORT );
         ENM( src->SrcRegister.SwizzleZ, TGSI_SWIZZLES_SHORT );
         ENM( src->SrcRegister.SwizzleW, TGSI_SWIZZLES_SHORT );
      }

      if( src->SrcRegisterExtMod.Absolute ) {
         CHR( '|' );
      }

      first_reg = GL_FALSE;
   }

   switch( inst->Instruction.Opcode ) {
   case TGSI_OPCODE_IF:
   case TGSI_OPCODE_ELSE:
      TXT( " :" );
      UID( inst->InstructionExtLabel.Label );
      break;
   }
}

static void
dump_instruction_verbose(
   struct text_dump *dump,
   struct tgsi_full_instruction *inst,
   GLuint ignored,
   GLuint deflt,
   struct tgsi_full_instruction *fi )
{
   GLuint i;

   TXT( "\nOpcode     : " );
   ENM( inst->Instruction.Opcode, TGSI_OPCODES );
   if( deflt || fi->Instruction.Saturate != inst->Instruction.Saturate ) {
      TXT( "\nSaturate   : " );
      ENM( inst->Instruction.Saturate, TGSI_SATS );
   }
   if( deflt || fi->Instruction.NumDstRegs != inst->Instruction.NumDstRegs ) {
      TXT( "\nNumDstRegs : " );
      UID( inst->Instruction.NumDstRegs );
   }
   if( deflt || fi->Instruction.NumSrcRegs != inst->Instruction.NumSrcRegs ) {
      TXT( "\nNumSrcRegs : " );
      UID( inst->Instruction.NumSrcRegs );
   }
   if( ignored ) {
      TXT( "\nPadding    : " );
      UIX( inst->Instruction.Padding );
   }

   if( deflt || tgsi_compare_instruction_ext_nv( inst->InstructionExtNv, fi->InstructionExtNv ) ) {
      CHR( '\n' );
      TXT( "\nType          : " );
      ENM( inst->InstructionExtNv.Type, TGSI_INSTRUCTION_EXTS );
      if( deflt || fi->InstructionExtNv.Precision != inst->InstructionExtNv.Precision ) {
         TXT( "\nPrecision     : " );
         ENM( inst->InstructionExtNv.Precision, TGSI_PRECISIONS );
      }
      if( deflt || fi->InstructionExtNv.CondDstIndex != inst->InstructionExtNv.CondDstIndex ) {
         TXT( "\nCondDstIndex  : " );
         UID( inst->InstructionExtNv.CondDstIndex );
      }
      if( deflt || fi->InstructionExtNv.CondFlowIndex != inst->InstructionExtNv.CondFlowIndex ) {
         TXT( "\nCondFlowIndex : " );
         UID( inst->InstructionExtNv.CondFlowIndex );
      }
      if( deflt || fi->InstructionExtNv.CondMask != inst->InstructionExtNv.CondMask ) {
         TXT( "\nCondMask      : " );
         ENM( inst->InstructionExtNv.CondMask, TGSI_CCS );
      }
      if( deflt || fi->InstructionExtNv.CondSwizzleX != inst->InstructionExtNv.CondSwizzleX ) {
         TXT( "\nCondSwizzleX  : " );
         ENM( inst->InstructionExtNv.CondSwizzleX, TGSI_SWIZZLES );
      }
      if( deflt || fi->InstructionExtNv.CondSwizzleY != inst->InstructionExtNv.CondSwizzleY ) {
         TXT( "\nCondSwizzleY  : " );
         ENM( inst->InstructionExtNv.CondSwizzleY, TGSI_SWIZZLES );
      }
      if( deflt || fi->InstructionExtNv.CondSwizzleZ != inst->InstructionExtNv.CondSwizzleZ ) {
         TXT( "\nCondSwizzleZ  : " );
         ENM( inst->InstructionExtNv.CondSwizzleZ, TGSI_SWIZZLES );
      }
      if( deflt || fi->InstructionExtNv.CondSwizzleW != inst->InstructionExtNv.CondSwizzleW ) {
         TXT( "\nCondSwizzleW  : " );
         ENM( inst->InstructionExtNv.CondSwizzleW, TGSI_SWIZZLES );
      }
      if( deflt || fi->InstructionExtNv.CondDstUpdate != inst->InstructionExtNv.CondDstUpdate ) {
         TXT( "\nCondDstUpdate : " );
         UID( inst->InstructionExtNv.CondDstUpdate );
      }
      if( deflt || fi->InstructionExtNv.CondFlowEnable != inst->InstructionExtNv.CondFlowEnable ) {
         TXT( "\nCondFlowEnable: " );
         UID( inst->InstructionExtNv.CondFlowEnable );
      }
      if( ignored ) {
         TXT( "\nPadding       : " );
         UIX( inst->InstructionExtNv.Padding );
         if( deflt || fi->InstructionExtNv.Extended != inst->InstructionExtNv.Extended ) {
            TXT( "\nExtended      : " );
            UID( inst->InstructionExtNv.Extended );
         }
      }
   }

   if( deflt || tgsi_compare_instruction_ext_label( inst->InstructionExtLabel, fi->InstructionExtLabel ) ) {
      CHR( '\n' );
      TXT( "\nType    : " );
      ENM( inst->InstructionExtLabel.Type, TGSI_INSTRUCTION_EXTS );
      if( deflt || fi->InstructionExtLabel.Label != inst->InstructionExtLabel.Label ) {
         TXT( "\nLabel   : " );
         UID( inst->InstructionExtLabel.Label );
      }
      if( ignored ) {
         TXT( "\nPadding : " );
         UIX( inst->InstructionExtLabel.Padding );
         if( deflt || fi->InstructionExtLabel.Extended != inst->InstructionExtLabel.Extended ) {
            TXT( "\nExtended: " );
            UID( inst->InstructionExtLabel.Extended );
         }
      }
   }

   if( deflt || tgsi_compare_instruction_ext_texture( inst->InstructionExtTexture, fi->InstructionExtTexture ) ) {
      CHR( '\n' );
      TXT( "\nType    : " );
      ENM( inst->InstructionExtTexture.Type, TGSI_INSTRUCTION_EXTS );
      if( deflt || fi->InstructionExtTexture.Texture != inst->InstructionExtTexture.Texture ) {
         TXT( "\nTexture : " );
         ENM( inst->InstructionExtTexture.Texture, TGSI_TEXTURES );
      }
      if( ignored ) {
         TXT( "\nPadding : " );
         UIX( inst->InstructionExtTexture.Padding );
         if( deflt || fi->InstructionExtTexture.Extended != inst->InstructionExtTexture.Extended ) {
            TXT( "\nExtended: " );
            UID( inst->InstructionExtTexture.Extended );
         }
      }
   }

   for( i = 0; i < inst->Instruction.NumDstRegs; i++ ) {
      struct tgsi_full_dst_register *dst = &inst->FullDstRegisters[i];
      struct tgsi_full_dst_register *fd = &fi->FullDstRegisters[i];

      CHR( '\n' );
      TXT( "\nFile     : " );
      ENM( dst->DstRegister.File, TGSI_FILES );
      if( deflt || fd->DstRegister.WriteMask != dst->DstRegister.WriteMask ) {
         TXT( "\nWriteMask: " );
         ENM( dst->DstRegister.WriteMask, TGSI_WRITEMASKS );
      }
      if( ignored ) {
         if( deflt || fd->DstRegister.Indirect != dst->DstRegister.Indirect ) {
            TXT( "\nIndirect : " );
            UID( dst->DstRegister.Indirect );
         }
         if( deflt || fd->DstRegister.Dimension != dst->DstRegister.Dimension ) {
            TXT( "\nDimension: " );
            UID( dst->DstRegister.Dimension );
         }
      }
      if( deflt || fd->DstRegister.Index != dst->DstRegister.Index ) {
         TXT( "\nIndex    : " );
         SID( dst->DstRegister.Index );
      }
      if( ignored ) {
         TXT( "\nPadding  : " );
         UIX( dst->DstRegister.Padding );
         if( deflt || fd->DstRegister.Extended != dst->DstRegister.Extended ) {
            TXT( "\nExtended : " );
            UID( dst->DstRegister.Extended );
         }
      }

      if( deflt || tgsi_compare_dst_register_ext_concode( dst->DstRegisterExtConcode, fd->DstRegisterExtConcode ) ) {
         CHR( '\n' );
         TXT( "\nType        : " );
         ENM( dst->DstRegisterExtConcode.Type, TGSI_DST_REGISTER_EXTS );
         if( deflt || fd->DstRegisterExtConcode.CondMask != dst->DstRegisterExtConcode.CondMask ) {
            TXT( "\nCondMask    : " );
            ENM( dst->DstRegisterExtConcode.CondMask, TGSI_CCS );
         }
         if( deflt || fd->DstRegisterExtConcode.CondSwizzleX != dst->DstRegisterExtConcode.CondSwizzleX ) {
            TXT( "\nCondSwizzleX: " );
            ENM( dst->DstRegisterExtConcode.CondSwizzleX, TGSI_SWIZZLES );
         }
         if( deflt || fd->DstRegisterExtConcode.CondSwizzleY != dst->DstRegisterExtConcode.CondSwizzleY ) {
            TXT( "\nCondSwizzleY: " );
            ENM( dst->DstRegisterExtConcode.CondSwizzleY, TGSI_SWIZZLES );
         }
         if( deflt || fd->DstRegisterExtConcode.CondSwizzleZ != dst->DstRegisterExtConcode.CondSwizzleZ ) {
            TXT( "\nCondSwizzleZ: " );
            ENM( dst->DstRegisterExtConcode.CondSwizzleZ, TGSI_SWIZZLES );
         }
         if( deflt || fd->DstRegisterExtConcode.CondSwizzleW != dst->DstRegisterExtConcode.CondSwizzleW ) {
            TXT( "\nCondSwizzleW: " );
            ENM( dst->DstRegisterExtConcode.CondSwizzleW, TGSI_SWIZZLES );
         }
         if( deflt || fd->DstRegisterExtConcode.CondSrcIndex != dst->DstRegisterExtConcode.CondSrcIndex ) {
            TXT( "\nCondSrcIndex: " );
            UID( dst->DstRegisterExtConcode.CondSrcIndex );
         }
         if( ignored ) {
            TXT( "\nPadding     : " );
            UIX( dst->DstRegisterExtConcode.Padding );
            if( deflt || fd->DstRegisterExtConcode.Extended != dst->DstRegisterExtConcode.Extended ) {
               TXT( "\nExtended    : " );
               UID( dst->DstRegisterExtConcode.Extended );
            }
         }
      }

      if( deflt || tgsi_compare_dst_register_ext_modulate( dst->DstRegisterExtModulate, fd->DstRegisterExtModulate ) ) {
         CHR( '\n' );
         TXT( "\nType    : " );
         ENM( dst->DstRegisterExtModulate.Type, TGSI_DST_REGISTER_EXTS );
         if( deflt || fd->DstRegisterExtModulate.Modulate != dst->DstRegisterExtModulate.Modulate ) {
            TXT( "\nModulate: " );
            ENM( dst->DstRegisterExtModulate.Modulate, TGSI_MODULATES );
         }
         if( ignored ) {
            TXT( "\nPadding : " );
            UIX( dst->DstRegisterExtModulate.Padding );
            if( deflt || fd->DstRegisterExtModulate.Extended != dst->DstRegisterExtModulate.Extended ) {
               TXT( "\nExtended: " );
               UID( dst->DstRegisterExtModulate.Extended );
            }
         }
      }
   }

   for( i = 0; i < inst->Instruction.NumSrcRegs; i++ ) {
      struct tgsi_full_src_register *src = &inst->FullSrcRegisters[i];
      struct tgsi_full_src_register *fs = &fi->FullSrcRegisters[i];

      CHR( '\n' );
      TXT( "\nFile     : ");
      ENM( src->SrcRegister.File, TGSI_FILES );
      if( deflt || fs->SrcRegister.SwizzleX != src->SrcRegister.SwizzleX ) {
         TXT( "\nSwizzleX : " );
         ENM( src->SrcRegister.SwizzleX, TGSI_SWIZZLES );
      }
      if( deflt || fs->SrcRegister.SwizzleY != src->SrcRegister.SwizzleY ) {
         TXT( "\nSwizzleY : " );
         ENM( src->SrcRegister.SwizzleY, TGSI_SWIZZLES );
      }
      if( deflt || fs->SrcRegister.SwizzleZ != src->SrcRegister.SwizzleZ ) {
         TXT( "\nSwizzleZ : " );
         ENM( src->SrcRegister.SwizzleZ, TGSI_SWIZZLES );
      }
      if( deflt || fs->SrcRegister.SwizzleW != src->SrcRegister.SwizzleW ) {
         TXT( "\nSwizzleW : " );
         ENM( src->SrcRegister.SwizzleW, TGSI_SWIZZLES );
      }
      if( deflt || fs->SrcRegister.Negate != src->SrcRegister.Negate ) {
         TXT( "\nNegate   : " );
         UID( src->SrcRegister.Negate );
      }
      if( ignored ) {
         if( deflt || fs->SrcRegister.Indirect != src->SrcRegister.Indirect ) {
            TXT( "\nIndirect : " );
            UID( src->SrcRegister.Indirect );
         }
         if( deflt || fs->SrcRegister.Dimension != src->SrcRegister.Dimension ) {
            TXT( "\nDimension: " );
            UID( src->SrcRegister.Dimension );
         }
      }
      if( deflt || fs->SrcRegister.Index != src->SrcRegister.Index ) {
         TXT( "\nIndex    : " );
         SID( src->SrcRegister.Index );
      }
      if( ignored ) {
         if( deflt || fs->SrcRegister.Extended != src->SrcRegister.Extended ) {
            TXT( "\nExtended : " );
            UID( src->SrcRegister.Extended );
         }
      }

      if( deflt || tgsi_compare_src_register_ext_swz( src->SrcRegisterExtSwz, fs->SrcRegisterExtSwz ) ) {
         CHR( '\n' );
         TXT( "\nType       : " );
         ENM( src->SrcRegisterExtSwz.Type, TGSI_SRC_REGISTER_EXTS );
         if( deflt || fs->SrcRegisterExtSwz.ExtSwizzleX != src->SrcRegisterExtSwz.ExtSwizzleX ) {
            TXT( "\nExtSwizzleX: " );
            ENM( src->SrcRegisterExtSwz.ExtSwizzleX, TGSI_EXTSWIZZLES );
         }
         if( deflt || fs->SrcRegisterExtSwz.ExtSwizzleY != src->SrcRegisterExtSwz.ExtSwizzleY ) {
            TXT( "\nExtSwizzleY: " );
            ENM( src->SrcRegisterExtSwz.ExtSwizzleY, TGSI_EXTSWIZZLES );
         }
         if( deflt || fs->SrcRegisterExtSwz.ExtSwizzleZ != src->SrcRegisterExtSwz.ExtSwizzleZ ) {
            TXT( "\nExtSwizzleZ: " );
            ENM( src->SrcRegisterExtSwz.ExtSwizzleZ, TGSI_EXTSWIZZLES );
         }
         if( deflt || fs->SrcRegisterExtSwz.ExtSwizzleW != src->SrcRegisterExtSwz.ExtSwizzleW ) {
            TXT( "\nExtSwizzleW: " );
            ENM( src->SrcRegisterExtSwz.ExtSwizzleW, TGSI_EXTSWIZZLES );
         }
         if( deflt || fs->SrcRegisterExtSwz.NegateX != src->SrcRegisterExtSwz.NegateX ) {
            TXT( "\nNegateX   : " );
            UID( src->SrcRegisterExtSwz.NegateX );
         }
         if( deflt || fs->SrcRegisterExtSwz.NegateY != src->SrcRegisterExtSwz.NegateY ) {
            TXT( "\nNegateY   : " );
            UID( src->SrcRegisterExtSwz.NegateY );
         }
         if( deflt || fs->SrcRegisterExtSwz.NegateZ != src->SrcRegisterExtSwz.NegateZ ) {
            TXT( "\nNegateZ   : " );
            UID( src->SrcRegisterExtSwz.NegateZ );
         }
         if( deflt || fs->SrcRegisterExtSwz.NegateW != src->SrcRegisterExtSwz.NegateW ) {
            TXT( "\nNegateW   : " );
            UID( src->SrcRegisterExtSwz.NegateW );
         }
         if( deflt || fs->SrcRegisterExtSwz.ExtDivide != src->SrcRegisterExtSwz.ExtDivide ) {
            TXT( "\nExtDivide  : " );
            ENM( src->SrcRegisterExtSwz.ExtDivide, TGSI_EXTSWIZZLES );
         }
         if( ignored ) {
            TXT( "\nPadding   : " );
            UIX( src->SrcRegisterExtSwz.Padding );
            if( deflt || fs->SrcRegisterExtSwz.Extended != src->SrcRegisterExtSwz.Extended ) {
               TXT( "\nExtended   : " );
               UID( src->SrcRegisterExtSwz.Extended );
            }
         }
      }

      if( deflt || tgsi_compare_src_register_ext_mod( src->SrcRegisterExtMod, fs->SrcRegisterExtMod ) ) {
         CHR( '\n' );
         TXT( "\nType     : " );
         ENM( src->SrcRegisterExtMod.Type, TGSI_SRC_REGISTER_EXTS );
         if( deflt || fs->SrcRegisterExtMod.Complement != src->SrcRegisterExtMod.Complement ) {
            TXT( "\nComplement: " );
            UID( src->SrcRegisterExtMod.Complement );
         }
         if( deflt || fs->SrcRegisterExtMod.Bias != src->SrcRegisterExtMod.Bias ) {
            TXT( "\nBias     : " );
            UID( src->SrcRegisterExtMod.Bias );
         }
         if( deflt || fs->SrcRegisterExtMod.Scale2X != src->SrcRegisterExtMod.Scale2X ) {
            TXT( "\nScale2X   : " );
            UID( src->SrcRegisterExtMod.Scale2X );
         }
         if( deflt || fs->SrcRegisterExtMod.Absolute != src->SrcRegisterExtMod.Absolute ) {
            TXT( "\nAbsolute  : " );
            UID( src->SrcRegisterExtMod.Absolute );
         }
         if( deflt || fs->SrcRegisterExtMod.Negate != src->SrcRegisterExtMod.Negate ) {
            TXT( "\nNegate   : " );
            UID( src->SrcRegisterExtMod.Negate );
         }
         if( ignored ) {
            TXT( "\nPadding   : " );
            UIX( src->SrcRegisterExtMod.Padding );
            if( deflt || fs->SrcRegisterExtMod.Extended != src->SrcRegisterExtMod.Extended ) {
               TXT( "\nExtended  : " );
               UID( src->SrcRegisterExtMod.Extended );
            }
         }
      }
   }
}

void
tgsi_dump(
   const struct tgsi_token *tokens,
   GLuint flags )
{
   struct text_dump _dump;
   struct text_dump *dump = &_dump;
   struct tgsi_parse_context parse;
   struct tgsi_full_instruction fi;
   struct tgsi_full_declaration fd;
   GLuint verbose = flags & TGSI_DUMP_VERBOSE;
   GLuint ignored = !(flags & TGSI_DUMP_NO_IGNORED);
   GLuint deflt = !(flags & TGSI_DUMP_NO_DEFAULT);
   GLuint instno = 0;

   {
#if 0
      static GLuint counter = 0;
      char buffer[64];

      sprintf( buffer, "tgsi-dump-%.4u.txt", counter++ );
      dump->file = fopen( buffer, "wt" );
#else
      dump->file = stderr;
      dump->tabs = 0;
#endif
   }

   /* sanity check */
   assert(strcmp(TGSI_OPCODES[TGSI_OPCODE_CONT], "OPCODE_CONT") == 0);

   tgsi_parse_init( &parse, tokens );

   TXT( "tgsi-dump begin -----------------" );

   CHR( '\n' );
   ENM( parse.FullHeader.Processor.Processor, TGSI_PROCESSOR_TYPES_SHORT );
   CHR( ' ' );
   UID( parse.FullVersion.Version.MajorVersion );
   CHR( '.' );
   UID( parse.FullVersion.Version.MinorVersion );

   if( verbose ) {
      TXT( "\nMajorVersion: " );
      UID( parse.FullVersion.Version.MajorVersion );
      TXT( "\nMinorVersion: " );
      UID( parse.FullVersion.Version.MinorVersion );
      CHR( '\n' );

      TXT( "\nHeaderSize: " );
      UID( parse.FullHeader.Header.HeaderSize );
      TXT( "\nBodySize  : " );
      UID( parse.FullHeader.Header.BodySize );
      TXT( "\nProcessor : " );
      ENM( parse.FullHeader.Processor.Processor, TGSI_PROCESSOR_TYPES );
      CHR( '\n' );
   }

   fi = tgsi_default_full_instruction();
   fd = tgsi_default_full_declaration();

   while( !tgsi_parse_end_of_tokens( &parse ) ) {
      tgsi_parse_token( &parse );

      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         dump_declaration_short(
            dump,
            &parse.FullToken.FullDeclaration );
         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
         dump_immediate_short(
            dump,
            &parse.FullToken.FullImmediate );
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         dump_instruction_short(
            dump,
            &parse.FullToken.FullInstruction,
            instno );
         instno++;
         break;

      default:
         assert( 0 );
      }

      if( verbose ) {
         TXT( "\nType       : " );
         ENM( parse.FullToken.Token.Type, TGSI_TOKEN_TYPES );
         if( ignored ) {
            TXT( "\nSize       : " );
            UID( parse.FullToken.Token.Size );
            if( deflt || parse.FullToken.Token.Extended ) {
               TXT( "\nExtended   : " );
               UID( parse.FullToken.Token.Extended );
            }
         }

         switch( parse.FullToken.Token.Type ) {
         case TGSI_TOKEN_TYPE_DECLARATION:
            dump_declaration_verbose(
               dump,
               &parse.FullToken.FullDeclaration,
               ignored,
               deflt,
               &fd );
            break;

         case TGSI_TOKEN_TYPE_IMMEDIATE:
            dump_immediate_verbose(
               dump,
               &parse.FullToken.FullImmediate,
               ignored );
            break;

         case TGSI_TOKEN_TYPE_INSTRUCTION:
            dump_instruction_verbose(
               dump,
               &parse.FullToken.FullInstruction,
               ignored,
               deflt,
               &fi );
            break;

         default:
            assert( 0 );
         }

         CHR( '\n' );
      }
   }

   TXT( "\ntgsi-dump end -------------------\n" );

   tgsi_parse_free( &parse );

   if (dump->file != stderr &&
       dump->file != stdout)
      fclose( dump->file );
}


#if !defined TGSI_TOKEN_H
#define TGSI_TOKEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "p_compiler.h"

struct tgsi_version
{
   unsigned MajorVersion  : 8;
   unsigned MinorVersion  : 8;
   unsigned Padding       : 16;
};

struct tgsi_header
{
   unsigned HeaderSize : 8;
   unsigned BodySize   : 24;
};

#define TGSI_PROCESSOR_FRAGMENT  0
#define TGSI_PROCESSOR_VERTEX    1
#define TGSI_PROCESSOR_GEOMETRY  2

struct tgsi_processor
{
   unsigned Processor  : 4;  /* TGSI_PROCESSOR_ */
   unsigned Padding    : 28;
};

#define TGSI_TOKEN_TYPE_DECLARATION    0
#define TGSI_TOKEN_TYPE_IMMEDIATE      1
#define TGSI_TOKEN_TYPE_INSTRUCTION    2

struct tgsi_token
{
   unsigned Type       : 4;  /* TGSI_TOKEN_TYPE_ */
   unsigned Size       : 8;  /* UINT */
   unsigned Padding    : 19;
   unsigned Extended   : 1;  /* BOOL */
};

enum tgsi_file_type {
   TGSI_FILE_NULL        =0,
   TGSI_FILE_CONSTANT    =1,
   TGSI_FILE_INPUT       =2,
   TGSI_FILE_OUTPUT      =3,
   TGSI_FILE_TEMPORARY   =4,
   TGSI_FILE_SAMPLER     =5,
   TGSI_FILE_ADDRESS     =6,
   TGSI_FILE_IMMEDIATE   =7,
   TGSI_FILE_COUNT      /**< how many TGSI_FILE_ types */
};


#define TGSI_DECLARE_RANGE    0
#define TGSI_DECLARE_MASK     1

#define TGSI_WRITEMASK_NONE     0x00
#define TGSI_WRITEMASK_X        0x01
#define TGSI_WRITEMASK_Y        0x02
#define TGSI_WRITEMASK_XY       0x03
#define TGSI_WRITEMASK_Z        0x04
#define TGSI_WRITEMASK_XZ       0x05
#define TGSI_WRITEMASK_YZ       0x06
#define TGSI_WRITEMASK_XYZ      0x07
#define TGSI_WRITEMASK_W        0x08
#define TGSI_WRITEMASK_XW       0x09
#define TGSI_WRITEMASK_YW       0x0A
#define TGSI_WRITEMASK_XYW      0x0B
#define TGSI_WRITEMASK_ZW       0x0C
#define TGSI_WRITEMASK_XZW      0x0D
#define TGSI_WRITEMASK_YZW      0x0E
#define TGSI_WRITEMASK_XYZW     0x0F

struct tgsi_declaration
{
   unsigned Type        : 4;  /* TGSI_TOKEN_TYPE_DECLARATION */
   unsigned Size        : 8;  /* UINT */
   unsigned File        : 4;  /* one of TGSI_FILE_x */
   unsigned Declare     : 4;  /* one of TGSI_DECLARE_x */
   unsigned UsageMask   : 4;  /* bitmask of TGSI_WRITEMASK_x flags */
   unsigned Interpolate : 1;  /* BOOL, any interpolation info? */
   unsigned Semantic    : 1;  /* BOOL, any semantic info? */
   unsigned Padding     : 5;
   unsigned Extended    : 1;  /* BOOL */
};

struct tgsi_declaration_range
{
   unsigned First   : 16; /* UINT */
   unsigned Last    : 16; /* UINT */
};

struct tgsi_declaration_mask
{
   unsigned Mask : 32; /* UINT */
};

#define TGSI_INTERPOLATE_CONSTANT      0
#define TGSI_INTERPOLATE_LINEAR        1
#define TGSI_INTERPOLATE_PERSPECTIVE   2

struct tgsi_declaration_interpolation
{
   unsigned Interpolate   : 4;  /* TGSI_INTERPOLATE_ */
   unsigned Padding       : 28;
};

#define TGSI_SEMANTIC_POSITION 0
#define TGSI_SEMANTIC_COLOR    1
#define TGSI_SEMANTIC_BCOLOR   2 /**< back-face color */
#define TGSI_SEMANTIC_FOG      3
#define TGSI_SEMANTIC_PSIZE    4
#define TGSI_SEMANTIC_GENERIC  5
#define TGSI_SEMANTIC_NORMAL   6
#define TGSI_SEMANTIC_COUNT    7 /**< number of semantic values */

struct tgsi_declaration_semantic
{
   unsigned SemanticName   : 8;  /* one of TGSI_SEMANTIC_ */
   unsigned SemanticIndex  : 16; /* UINT */
   unsigned Padding        : 8;
};

#define TGSI_IMM_FLOAT32   0

struct tgsi_immediate
{
   unsigned Type       : 4;  /* TGSI_TOKEN_TYPE_IMMEDIATE */
   unsigned Size       : 8;  /* UINT */
   unsigned DataType   : 4;  /* TGSI_IMM_ */
   unsigned Padding    : 15;
   unsigned Extended   : 1;  /* BOOL */
};

struct tgsi_immediate_float32
{
   float Float;
};

/*
 * GL_NV_vertex_program
 */
#define TGSI_OPCODE_ARL                 0
#define TGSI_OPCODE_MOV                 1
#define TGSI_OPCODE_LIT                 2
#define TGSI_OPCODE_RCP                 3
#define TGSI_OPCODE_RSQ                 4
#define TGSI_OPCODE_EXP                 5
#define TGSI_OPCODE_LOG                 6
#define TGSI_OPCODE_MUL                 7
#define TGSI_OPCODE_ADD                 8
#define TGSI_OPCODE_DP3                 9
#define TGSI_OPCODE_DP4                 10
#define TGSI_OPCODE_DST                 11
#define TGSI_OPCODE_MIN                 12
#define TGSI_OPCODE_MAX                 13
#define TGSI_OPCODE_SLT                 14
#define TGSI_OPCODE_SGE                 15
#define TGSI_OPCODE_MAD                 16

/*
 * GL_ATI_fragment_shader
 */
#define TGSI_OPCODE_SUB                 17
#define TGSI_OPCODE_DOT3                TGSI_OPCODE_DP3
#define TGSI_OPCODE_DOT4                TGSI_OPCODE_DP4
#define TGSI_OPCODE_LERP                18
#define TGSI_OPCODE_CND                 19
#define TGSI_OPCODE_CND0                20
#define TGSI_OPCODE_DOT2ADD             21

/*
 * GL_EXT_vertex_shader
 */
#define TGSI_OPCODE_INDEX               22
#define TGSI_OPCODE_NEGATE              23
#define TGSI_OPCODE_MADD                TGSI_OPCODE_MAD
#define TGSI_OPCODE_FRAC                24
#define TGSI_OPCODE_SETGE               TGSI_OPCODE_SGE
#define TGSI_OPCODE_SETLT               TGSI_OPCODE_SLT
#define TGSI_OPCODE_CLAMP               25
#define TGSI_OPCODE_FLOOR               26
#define TGSI_OPCODE_ROUND               27
#define TGSI_OPCODE_EXPBASE2            28
#define TGSI_OPCODE_LOGBASE2            29
#define TGSI_OPCODE_POWER               30
#define TGSI_OPCODE_RECIP               TGSI_OPCODE_RCP
#define TGSI_OPCODE_RECIPSQRT           TGSI_OPCODE_RSQ
#define TGSI_OPCODE_CROSSPRODUCT        31
#define TGSI_OPCODE_MULTIPLYMATRIX      32

/*
 * GL_NV_vertex_program1_1
 */
#define TGSI_OPCODE_ABS                 33
#define TGSI_OPCODE_RCC                 34
#define TGSI_OPCODE_DPH                 35

/*
 * GL_NV_fragment_program
 */
#define TGSI_OPCODE_COS                 36
#define TGSI_OPCODE_DDX                 37
#define TGSI_OPCODE_DDY                 38
#define TGSI_OPCODE_EX2                 TGSI_OPCODE_EXPBASE2
#define TGSI_OPCODE_FLR                 TGSI_OPCODE_FLOOR
#define TGSI_OPCODE_FRC                 TGSI_OPCODE_FRAC
#define TGSI_OPCODE_KILP                39  /* predicated kill */
#define TGSI_OPCODE_LG2                 TGSI_OPCODE_LOGBASE2
#define TGSI_OPCODE_LRP                 TGSI_OPCODE_LERP
#define TGSI_OPCODE_PK2H                40
#define TGSI_OPCODE_PK2US               41
#define TGSI_OPCODE_PK4B                42
#define TGSI_OPCODE_PK4UB               43
#define TGSI_OPCODE_POW                 TGSI_OPCODE_POWER
#define TGSI_OPCODE_RFL                 44
#define TGSI_OPCODE_SEQ                 45
#define TGSI_OPCODE_SFL                 46
#define TGSI_OPCODE_SGT                 47
#define TGSI_OPCODE_SIN                 48
#define TGSI_OPCODE_SLE                 49
#define TGSI_OPCODE_SNE                 50
#define TGSI_OPCODE_STR                 51
#define TGSI_OPCODE_TEX                 52
#define TGSI_OPCODE_TXD                 53
#define TGSI_OPCODE_TXP                 134
#define TGSI_OPCODE_UP2H                54
#define TGSI_OPCODE_UP2US               55
#define TGSI_OPCODE_UP4B                56
#define TGSI_OPCODE_UP4UB               57
#define TGSI_OPCODE_X2D                 58

/*
 * GL_NV_vertex_program2
 */
#define TGSI_OPCODE_ARA                 59
#define TGSI_OPCODE_ARR                 60
#define TGSI_OPCODE_BRA                 61
#define TGSI_OPCODE_CAL                 62
#define TGSI_OPCODE_RET                 63
#define TGSI_OPCODE_SSG                 64

/*
 * GL_ARB_vertex_program
 */
#define TGSI_OPCODE_SWZ                 TGSI_OPCODE_MOV
#define TGSI_OPCODE_XPD                 TGSI_OPCODE_CROSSPRODUCT

/*
 * GL_ARB_fragment_program
 */
#define TGSI_OPCODE_CMP                 65
#define TGSI_OPCODE_SCS                 66
#define TGSI_OPCODE_TXB                 67

/*
 * GL_NV_fragment_program_option
 */
/* No new opcode */

/*
 * GL_NV_fragment_program2
 */
#define TGSI_OPCODE_NRM                 68
#define TGSI_OPCODE_DIV                 69
#define TGSI_OPCODE_DP2                 70
#define TGSI_OPCODE_DP2A                TGSI_OPCODE_DOT2ADD
#define TGSI_OPCODE_TXL                 71
#define TGSI_OPCODE_BRK                 72
#define TGSI_OPCODE_IF                  73
#define TGSI_OPCODE_LOOP                74
#define TGSI_OPCODE_REP                 75
#define TGSI_OPCODE_ELSE                76
#define TGSI_OPCODE_ENDIF               77
#define TGSI_OPCODE_ENDLOOP             78
#define TGSI_OPCODE_ENDREP              79

/*
 * GL_NV_vertex_program2_option
 */

/*
 * GL_NV_vertex_program3
 */
#define TGSI_OPCODE_PUSHA               80
#define TGSI_OPCODE_POPA                81

/*
 * GL_NV_gpu_program4
 */
#define TGSI_OPCODE_CEIL                82
#define TGSI_OPCODE_I2F                 83
#define TGSI_OPCODE_NOT                 84
#define TGSI_OPCODE_TRUNC               85
#define TGSI_OPCODE_SHL                 86
#define TGSI_OPCODE_SHR                 87
#define TGSI_OPCODE_AND                 88
#define TGSI_OPCODE_OR                  89
#define TGSI_OPCODE_MOD                 90
#define TGSI_OPCODE_XOR                 91
#define TGSI_OPCODE_SAD                 92
#define TGSI_OPCODE_TXF                 93
#define TGSI_OPCODE_TXQ                 94
#define TGSI_OPCODE_CONT                95

/*
 * GL_NV_vertex_program4
 */
/* Same as GL_NV_gpu_program4 */

/*
 * GL_NV_fragment_program4
 */
/* Same as GL_NV_gpu_program4 */

/*
 * GL_NV_geometry_program4
 */
/* Same as GL_NV_gpu_program4 */
#define TGSI_OPCODE_EMIT                96
#define TGSI_OPCODE_ENDPRIM             97

/*
 * GLSL
 */
#define TGSI_OPCODE_BGNLOOP2            98
#define TGSI_OPCODE_BGNSUB              99
#define TGSI_OPCODE_ENDLOOP2            100
#define TGSI_OPCODE_ENDSUB              101
#define TGSI_OPCODE_INT                 TGSI_OPCODE_TRUNC
#define TGSI_OPCODE_NOISE1              102
#define TGSI_OPCODE_NOISE2              103
#define TGSI_OPCODE_NOISE3              104
#define TGSI_OPCODE_NOISE4              105
#define TGSI_OPCODE_NOP                 106

/*
 * ps_1_1
 */
#define TGSI_OPCODE_TEXKILL             TGSI_OPCODE_KILP
#define TGSI_OPCODE_TEXBEM              107
#define TGSI_OPCODE_TEXBEML             108
#define TGSI_OPCODE_TEXREG2AR           109
#define TGSI_OPCODE_TEXM3X2PAD          110
#define TGSI_OPCODE_TEXM3X2TEX          111
#define TGSI_OPCODE_TEXM3X3PAD          112
#define TGSI_OPCODE_TEXM3X3TEX          113
#define TGSI_OPCODE_TEXM3X3SPEC         114
#define TGSI_OPCODE_TEXM3X3VSPEC        115

/*
 * ps_1_2
 */
#define TGSI_OPCODE_TEXREG2GB           116
#define TGSI_OPCODE_TEXREG2RGB          117
#define TGSI_OPCODE_TEXDP3TEX           118
#define TGSI_OPCODE_TEXDP3              119
#define TGSI_OPCODE_TEXM3X3             120
/* CMP - use TGSI_OPCODE_CND0 */

/*
 * ps_1_3
 */
#define TGSI_OPCODE_TEXM3X2DEPTH        121
/* CMP - use TGSI_OPCODE_CND0 */

/*
 * ps_1_4
 */
#define TGSI_OPCODE_TEXLD               TGSI_OPCODE_TEX
#define TGSI_OPCODE_TEXDEPTH            122
#define TGSI_OPCODE_BEM                 123

/*
 * ps_2_0
 */
#define TGSI_OPCODE_M4X4                TGSI_OPCODE_MULTIPLYMATRIX
#define TGSI_OPCODE_M4X3                124
#define TGSI_OPCODE_M3X4                125
#define TGSI_OPCODE_M3X3                126
#define TGSI_OPCODE_M3X2                127
#define TGSI_OPCODE_CRS                 TGSI_OPCODE_XPD
#define TGSI_OPCODE_NRM4                128
#define TGSI_OPCODE_SINCOS              TGSI_OPCODE_SCS
#define TGSI_OPCODE_TEXLDB              TGSI_OPCODE_TXB
#define TGSI_OPCODE_DP2ADD              TGSI_OPCODE_DP2A

/*
 * ps_2_x
 */
#define TGSI_OPCODE_CALL                TGSI_OPCODE_CAL
#define TGSI_OPCODE_CALLNZ              129
#define TGSI_OPCODE_IFC                 130
#define TGSI_OPCODE_BREAK               TGSI_OPCODE_BRK
#define TGSI_OPCODE_BREAKC              131
#define TGSI_OPCODE_DSX                 TGSI_OPCODE_DDX
#define TGSI_OPCODE_DSY                 TGSI_OPCODE_DDY
#define TGSI_OPCODE_TEXLDD              TGSI_OPCODE_TXD

/*
 * vs_1_1
 */
#define TGSI_OPCODE_EXPP                TGSI_OPCODE_EXP
#define TGSI_OPCODE_LOGP                TGSI_OPCODE_LG2

/*
 * vs_2_0
 */
#define TGSI_OPCODE_SGN                 TGSI_OPCODE_SSG
#define TGSI_OPCODE_MOVA                TGSI_OPCODE_ARR

/*
 * vs_2_x
 */

#define TGSI_OPCODE_KIL                 132  /* unpredicated kill */
#define TGSI_OPCODE_END                 133  /* aka HALT */

#define TGSI_OPCODE_LAST                135

#define TGSI_SAT_NONE            0  /* do not saturate */
#define TGSI_SAT_ZERO_ONE        1  /* clamp to [0,1] */
#define TGSI_SAT_MINUS_PLUS_ONE  2  /* clamp to [-1,1] */

/*
 * Opcode is the operation code to execute. A given operation defines the
 * semantics how the source registers (if any) are interpreted and what is
 * written to the destination registers (if any) as a result of execution.
 *
 * NumDstRegs and NumSrcRegs is the number of destination and source registers,
 * respectively. For a given operation code, those numbers are fixed and are
 * present here only for convenience.
 *
 * If Extended is TRUE, it is now executed.
 *
 * Saturate controls how are final results in destination registers modified.
 */

struct tgsi_instruction
{
   unsigned Type       : 4;  /* TGSI_TOKEN_TYPE_INSTRUCTION */
   unsigned Size       : 8;  /* UINT */
   unsigned Opcode     : 8;  /* TGSI_OPCODE_ */
   unsigned Saturate   : 2;  /* TGSI_SAT_ */
   unsigned NumDstRegs : 2;  /* UINT */
   unsigned NumSrcRegs : 4;  /* UINT */
   unsigned Padding    : 3;
   unsigned Extended   : 1;  /* BOOL */
};

/*
 * If tgsi_instruction::Extended is TRUE, tgsi_instruction_ext follows.
 * 
 * Then, tgsi_instruction::NumDstRegs of tgsi_dst_register follow.
 * 
 * Then, tgsi_instruction::NumSrcRegs of tgsi_src_register follow.
 *
 * tgsi_instruction::Size contains the total number of words that make the
 * instruction, including the instruction word.
 */

#define TGSI_INSTRUCTION_EXT_TYPE_NV        0
#define TGSI_INSTRUCTION_EXT_TYPE_LABEL     1
#define TGSI_INSTRUCTION_EXT_TYPE_TEXTURE   2
#define TGSI_INSTRUCTION_EXT_TYPE_PREDICATE 3

struct tgsi_instruction_ext
{
   unsigned Type       : 4;  /* TGSI_INSTRUCTION_EXT_TYPE_ */
   unsigned Padding    : 27;
   unsigned Extended   : 1;  /* BOOL */
};

/*
 * If tgsi_instruction_ext::Type is TGSI_INSTRUCTION_EXT_TYPE_NV, it should
 * be cast to tgsi_instruction_ext_nv.
 * 
 * If tgsi_instruction_ext::Type is TGSI_INSTRUCTION_EXT_TYPE_LABEL, it
 * should be cast to tgsi_instruction_ext_label.
 * 
 * If tgsi_instruction_ext::Type is TGSI_INSTRUCTION_EXT_TYPE_TEXTURE, it
 * should be cast to tgsi_instruction_ext_texture.
 * 
 * If tgsi_instruction_ext::Type is TGSI_INSTRUCTION_EXT_TYPE_PREDICATE, it
 * should be cast to tgsi_instruction_ext_predicate.
 * 
 * If tgsi_instruction_ext::Extended is TRUE, another tgsi_instruction_ext
 * follows.
 */

#define TGSI_PRECISION_DEFAULT      0
#define TGSI_PRECISION_FLOAT32      1
#define TGSI_PRECISION_FLOAT16      2
#define TGSI_PRECISION_FIXED12      3

#define TGSI_CC_GT      0
#define TGSI_CC_EQ      1
#define TGSI_CC_LT      2
#define TGSI_CC_UN      3
#define TGSI_CC_GE      4
#define TGSI_CC_LE      5
#define TGSI_CC_NE      6
#define TGSI_CC_TR      7
#define TGSI_CC_FL      8

#define TGSI_SWIZZLE_X      0
#define TGSI_SWIZZLE_Y      1
#define TGSI_SWIZZLE_Z      2
#define TGSI_SWIZZLE_W      3

/*
 * Precision controls the precision at which the operation should be executed.
 *
 * CondDstUpdate enables condition code register writes. When this field is
 * TRUE, CondDstIndex specifies the index of the condition code register to
 * update.
 *
 * CondFlowEnable enables conditional execution of the operation. When this
 * field is TRUE, CondFlowIndex specifies the index of the condition code
 * register to test against CondMask with component swizzle controled by
 * CondSwizzleX, CondSwizzleY, CondSwizzleZ and CondSwizzleW. If the test fails,
 * the operation is not executed.
 */

struct tgsi_instruction_ext_nv
{
   unsigned Type             : 4;    /* TGSI_INSTRUCTION_EXT_TYPE_NV */
   unsigned Precision        : 4;    /* TGSI_PRECISION_ */
   unsigned CondDstIndex     : 4;    /* UINT */
   unsigned CondFlowIndex    : 4;    /* UINT */
   unsigned CondMask         : 4;    /* TGSI_CC_ */
   unsigned CondSwizzleX     : 2;    /* TGSI_SWIZZLE_ */
   unsigned CondSwizzleY     : 2;    /* TGSI_SWIZZLE_ */
   unsigned CondSwizzleZ     : 2;    /* TGSI_SWIZZLE_ */
   unsigned CondSwizzleW     : 2;    /* TGSI_SWIZZLE_ */
   unsigned CondDstUpdate    : 1;    /* BOOL */
   unsigned CondFlowEnable   : 1;    /* BOOL */
   unsigned Padding          : 1;
   unsigned Extended         : 1;    /* BOOL */
};

struct tgsi_instruction_ext_label
{
   unsigned Type     : 4;    /* TGSI_INSTRUCTION_EXT_TYPE_LABEL */
   unsigned Label    : 24;   /* UINT */
   unsigned Padding  : 3;
   unsigned Extended : 1;    /* BOOL */
};

#define TGSI_TEXTURE_UNKNOWN        0
#define TGSI_TEXTURE_1D             1
#define TGSI_TEXTURE_2D             2
#define TGSI_TEXTURE_3D             3
#define TGSI_TEXTURE_CUBE           4
#define TGSI_TEXTURE_RECT           5
#define TGSI_TEXTURE_SHADOW1D       6
#define TGSI_TEXTURE_SHADOW2D       7
#define TGSI_TEXTURE_SHADOWRECT     8

struct tgsi_instruction_ext_texture
{
   unsigned Type     : 4;    /* TGSI_INSTRUCTION_EXT_TYPE_TEXTURE */
   unsigned Texture  : 8;    /* TGSI_TEXTURE_ */
   unsigned Padding  : 19;
   unsigned Extended : 1;    /* BOOL */
};

struct tgsi_instruction_ext_predicate
{
   unsigned Type             : 4;    /* TGSI_INSTRUCTION_EXT_TYPE_PREDICATE */
   unsigned PredDstIndex     : 4;    /* UINT */
   unsigned PredWriteMask    : 4;    /* TGSI_WRITEMASK_ */
   unsigned Padding          : 19;
   unsigned Extended         : 1;    /* BOOL */
};

/*
 * File specifies the register array to access.
 *
 * Index specifies the element number of a register in the register file.
 *
 * If Indirect is TRUE, Index should be offset by the X component of a source
 * register that follows. The register can be now fetched into local storage
 * for further processing.
 *
 * If Negate is TRUE, all components of the fetched register are negated.
 *
 * The fetched register components are swizzled according to SwizzleX, SwizzleY,
 * SwizzleZ and SwizzleW.
 *
 * If Extended is TRUE, any further modifications to the source register are
 * made to this temporary storage.
 */

struct tgsi_src_register
{
   unsigned File        : 4;  /* TGSI_FILE_ */
   unsigned SwizzleX    : 2;  /* TGSI_SWIZZLE_ */
   unsigned SwizzleY    : 2;  /* TGSI_SWIZZLE_ */
   unsigned SwizzleZ    : 2;  /* TGSI_SWIZZLE_ */
   unsigned SwizzleW    : 2;  /* TGSI_SWIZZLE_ */
   unsigned Negate      : 1;  /* BOOL */
   unsigned Indirect    : 1;  /* BOOL */
   unsigned Dimension   : 1;  /* BOOL */
   int      Index       : 16; /* SINT */
   unsigned Extended    : 1;  /* BOOL */
};

/*
 * If tgsi_src_register::Extended is TRUE, tgsi_src_register_ext follows.
 * 
 * Then, if tgsi_src_register::Indirect is TRUE, another tgsi_src_register
 * follows.
 *
 * Then, if tgsi_src_register::Dimension is TRUE, tgsi_dimension follows.
 */

#define TGSI_SRC_REGISTER_EXT_TYPE_SWZ      0
#define TGSI_SRC_REGISTER_EXT_TYPE_MOD      1

struct tgsi_src_register_ext
{
   unsigned Type     : 4;    /* TGSI_SRC_REGISTER_EXT_TYPE_ */
   unsigned Padding  : 27;
   unsigned Extended : 1;    /* BOOL */
};

/*
 * If tgsi_src_register_ext::Type is TGSI_SRC_REGISTER_EXT_TYPE_SWZ,
 * it should be cast to tgsi_src_register_ext_swz.
 * 
 * If tgsi_src_register_ext::Type is TGSI_SRC_REGISTER_EXT_TYPE_MOD,
 * it should be cast to tgsi_src_register_ext_mod.
 * 
 * If tgsi_dst_register_ext::Extended is TRUE, another tgsi_dst_register_ext
 * follows.
 */

#define TGSI_EXTSWIZZLE_X       TGSI_SWIZZLE_X
#define TGSI_EXTSWIZZLE_Y       TGSI_SWIZZLE_Y
#define TGSI_EXTSWIZZLE_Z       TGSI_SWIZZLE_Z
#define TGSI_EXTSWIZZLE_W       TGSI_SWIZZLE_W
#define TGSI_EXTSWIZZLE_ZERO    4
#define TGSI_EXTSWIZZLE_ONE     5

/*
 * ExtSwizzleX, ExtSwizzleY, ExtSwizzleZ and ExtSwizzleW swizzle the source
 * register in an extended manner.
 *
 * NegateX, NegateY, NegateZ and NegateW negate individual components of the
 * source register.
 */

struct tgsi_src_register_ext_swz
{
   unsigned Type         : 4;    /* TGSI_SRC_REGISTER_EXT_TYPE_SWZ */
   unsigned ExtSwizzleX  : 4;    /* TGSI_EXTSWIZZLE_ */
   unsigned ExtSwizzleY  : 4;    /* TGSI_EXTSWIZZLE_ */
   unsigned ExtSwizzleZ  : 4;    /* TGSI_EXTSWIZZLE_ */
   unsigned ExtSwizzleW  : 4;    /* TGSI_EXTSWIZZLE_ */
   unsigned NegateX      : 1;    /* BOOL */
   unsigned NegateY      : 1;    /* BOOL */
   unsigned NegateZ      : 1;    /* BOOL */
   unsigned NegateW      : 1;    /* BOOL */
   unsigned Padding      : 7;
   unsigned Extended     : 1;    /* BOOL */
};

/**
 * Extra src register modifiers
 *
 * If Complement is TRUE, the source register is modified by subtracting it
 * from 1.0.
 *
 * If Bias is TRUE, the source register is modified by subtracting 0.5 from it.
 *
 * If Scale2X is TRUE, the source register is modified by multiplying it by 2.0.
 *
 * If Absolute is TRUE, the source register is modified by removing the sign.
 *
 * If Negate is TRUE, the source register is modified by negating it.
 */

struct tgsi_src_register_ext_mod
{
   unsigned Type         : 4;    /* TGSI_SRC_REGISTER_EXT_TYPE_MOD */
   unsigned Complement   : 1;    /* BOOL */
   unsigned Bias         : 1;    /* BOOL */
   unsigned Scale2X      : 1;    /* BOOL */
   unsigned Absolute     : 1;    /* BOOL */
   unsigned Negate       : 1;    /* BOOL */
   unsigned Padding      : 22;
   unsigned Extended     : 1;    /* BOOL */
};

struct tgsi_dimension
{
   unsigned Indirect    : 1;  /* BOOL */
   unsigned Dimension   : 1;  /* BOOL */
   unsigned Padding     : 13;
   int      Index       : 16; /* SINT */
   unsigned Extended    : 1;  /* BOOL */
};

struct tgsi_dst_register
{
   unsigned File        : 4;  /* TGSI_FILE_ */
   unsigned WriteMask   : 4;  /* TGSI_WRITEMASK_ */
   unsigned Indirect    : 1;  /* BOOL */
   unsigned Dimension   : 1;  /* BOOL */
   int      Index       : 16; /* SINT */
   unsigned Padding     : 5;
   unsigned Extended    : 1;  /* BOOL */
};

/*
 * If tgsi_dst_register::Extended is TRUE, tgsi_dst_register_ext follows.
 * 
 * Then, if tgsi_dst_register::Indirect is TRUE, tgsi_src_register follows.
 */

#define TGSI_DST_REGISTER_EXT_TYPE_CONDCODE     0
#define TGSI_DST_REGISTER_EXT_TYPE_MODULATE     1
#define TGSI_DST_REGISTER_EXT_TYPE_PREDICATE    2

struct tgsi_dst_register_ext
{
   unsigned Type     : 4;    /* TGSI_DST_REGISTER_EXT_TYPE_ */
   unsigned Padding  : 27;
   unsigned Extended : 1;    /* BOOL */
};

/**
 * Extra destination register modifiers
 *
 * If tgsi_dst_register_ext::Type is TGSI_DST_REGISTER_EXT_TYPE_CONDCODE,
 * it should be cast to tgsi_dst_register_ext_condcode.
 * 
 * If tgsi_dst_register_ext::Type is TGSI_DST_REGISTER_EXT_TYPE_MODULATE,
 * it should be cast to tgsi_dst_register_ext_modulate.
 * 
 * If tgsi_dst_register_ext::Type is TGSI_DST_REGISTER_EXT_TYPE_PREDICATE,
 * it should be cast to tgsi_dst_register_ext_predicate.
 * 
 * If tgsi_dst_register_ext::Extended is TRUE, another tgsi_dst_register_ext
 * follows.
 */
struct tgsi_dst_register_ext_concode
{
   unsigned Type         : 4;    /* TGSI_DST_REGISTER_EXT_TYPE_CONDCODE */
   unsigned CondMask     : 4;    /* TGSI_CC_ */
   unsigned CondSwizzleX : 2;    /* TGSI_SWIZZLE_ */
   unsigned CondSwizzleY : 2;    /* TGSI_SWIZZLE_ */
   unsigned CondSwizzleZ : 2;    /* TGSI_SWIZZLE_ */
   unsigned CondSwizzleW : 2;    /* TGSI_SWIZZLE_ */
   unsigned CondSrcIndex : 4;    /* UINT */
   unsigned Padding      : 11;
   unsigned Extended     : 1;    /* BOOL */
};

#define TGSI_MODULATE_1X        0
#define TGSI_MODULATE_2X        1
#define TGSI_MODULATE_4X        2
#define TGSI_MODULATE_8X        3
#define TGSI_MODULATE_HALF      4
#define TGSI_MODULATE_QUARTER   5
#define TGSI_MODULATE_EIGHTH    6

struct tgsi_dst_register_ext_modulate
{
   unsigned Type     : 4;    /* TGSI_DST_REGISTER_EXT_TYPE_MODULATE */
   unsigned Modulate : 4;    /* TGSI_MODULATE_ */
   unsigned Padding  : 23;
   unsigned Extended : 1;    /* BOOL */
};

/*
 * Currently, the following constraints apply.
 *
 * - PredSwizzleXYZW is either set to identity or replicate.
 * - PredSrcIndex is 0.
 */

struct tgsi_dst_register_ext_predicate
{
   unsigned Type         : 4;    /* TGSI_DST_REGISTER_EXT_TYPE_PREDICATE */
   unsigned PredSwizzleX : 2;    /* TGSI_SWIZZLE_ */
   unsigned PredSwizzleY : 2;    /* TGSI_SWIZZLE_ */
   unsigned PredSwizzleZ : 2;    /* TGSI_SWIZZLE_ */
   unsigned PredSwizzleW : 2;    /* TGSI_SWIZZLE_ */
   unsigned PredSrcIndex : 4;    /* UINT */
   unsigned Negate       : 1;    /* BOOL */
   unsigned Padding      : 14;
   unsigned Extended     : 1;    /* BOOL */
};


#ifdef __cplusplus
}
#endif

#endif /* TGSI_TOKEN_H */


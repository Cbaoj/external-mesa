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


/* Instructions, enums and structures for IVB.
 *
 * This file has been generated, do not hand edit.
 */

#pragma once

#include <stdio.h>
#include <assert.h>

#ifndef __gen_field_functions
#define __gen_field_functions

union __gen_value {
   float f;
   uint32_t dw;
};

static inline uint64_t
__gen_field(uint64_t v, uint32_t start, uint32_t end)
{
#if DEBUG
   if (end - start + 1 < 64)
      assert(v < 1ul << (end - start + 1));
#endif

   return v << start;
}

static inline uint64_t
__gen_offset(uint64_t v, uint32_t start, uint32_t end)
{
#if DEBUG
   uint64_t mask = (~0ul >> (64 - (end - start + 1))) << start;

   assert((v & ~mask) == 0);
#endif

   return v;
}

static inline uint32_t
__gen_float(float v)
{
   return ((union __gen_value) { .f = (v) }).dw;
}

#ifndef __gen_address_type
#error #define __gen_address_type before including this file
#endif

#ifndef __gen_user_data
#error #define __gen_combine_address before including this file
#endif

#endif

#define GEN7_3DSTATE_URB_VS_length 0x00000002
#define GEN7_3DSTATE_URB_VS_length_bias 0x00000002
#define GEN7_3DSTATE_URB_VS_header              \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 48,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_URB_VS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     VSURBStartingAddress;
   uint32_t                                     VSURBEntryAllocationSize;
   uint32_t                                     VSNumberofURBEntries;
};

static inline void
GEN7_3DSTATE_URB_VS_pack(__gen_user_data *data, void * restrict dst,
                        const struct GEN7_3DSTATE_URB_VS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->VSURBStartingAddress, 25, 29) |
      __gen_field(values->VSURBEntryAllocationSize, 16, 24) |
      __gen_field(values->VSNumberofURBEntries, 0, 15) |
      0;

}

#define GEN7_MI_STORE_REGISTER_MEM_length 0x00000003
#define GEN7_MI_STORE_REGISTER_MEM_length_bias 0x00000002
#define GEN7_MI_STORE_REGISTER_MEM_header       \
   .CommandType          =  0,                  \
   .MICommandOpcode      = 36,                  \
   .DwordLength          =  1

struct GEN7_MI_STORE_REGISTER_MEM {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
   uint32_t                                     UseGlobalGTT;
   uint32_t                                     DwordLength;
   uint32_t                                     RegisterAddress;
   __gen_address_type                           MemoryAddress;
};

static inline void
GEN7_MI_STORE_REGISTER_MEM_pack(__gen_user_data *data, void * restrict dst,
                               const struct GEN7_MI_STORE_REGISTER_MEM * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      __gen_field(values->UseGlobalGTT, 22, 22) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->RegisterAddress, 2, 22) |
      0;

   uint32_t dw2 =
      0;

   dw[2] =
      __gen_combine_address(data, &dw[2], values->MemoryAddress, dw2);

}

#define GEN7_PIPELINE_SELECT_length 0x00000001
#define GEN7_PIPELINE_SELECT_length_bias 0x00000001
#define GEN7_PIPELINE_SELECT_header             \
   .CommandType          =  3,                  \
   .CommandSubType       =  1,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  =  4

struct GEN7_PIPELINE_SELECT {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
#define     _3D                                                0
#define     Media                                              1
#define     GPGPU                                              2
   uint32_t                                     PipelineSelection;
};

static inline void
GEN7_PIPELINE_SELECT_pack(__gen_user_data *data, void * restrict dst,
                         const struct GEN7_PIPELINE_SELECT * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->PipelineSelection, 0, 1) |
      0;

}

#define GEN7_STATE_BASE_ADDRESS_length 0x0000000a
#define GEN7_STATE_BASE_ADDRESS_length_bias 0x00000002
#define GEN7_STATE_BASE_ADDRESS_header          \
   .CommandType          =  3,                  \
   .CommandSubType       =  0,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  =  1,                  \
   .DwordLength          =  8

struct GEN7_MEMORY_OBJECT_CONTROL_STATE {
   uint32_t                                     GraphicsDataTypeGFDT;
   uint32_t                                     LLCCacheabilityControlLLCCC;
   uint32_t                                     L3CacheabilityControlL3CC;
};

static inline void
GEN7_MEMORY_OBJECT_CONTROL_STATE_pack(__gen_user_data *data, void * restrict dst,
                                     const struct GEN7_MEMORY_OBJECT_CONTROL_STATE * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->GraphicsDataTypeGFDT, 2, 2) |
      __gen_field(values->LLCCacheabilityControlLLCCC, 1, 1) |
      __gen_field(values->L3CacheabilityControlL3CC, 0, 0) |
      0;

}

struct GEN7_STATE_BASE_ADDRESS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   __gen_address_type                           GeneralStateBaseAddress;
   struct GEN7_MEMORY_OBJECT_CONTROL_STATE      GeneralStateMemoryObjectControlState;
   struct GEN7_MEMORY_OBJECT_CONTROL_STATE      StatelessDataPortAccessMemoryObjectControlState;
   uint32_t                                     StatelessDataPortAccessForceWriteThru;
   uint32_t                                     GeneralStateBaseAddressModifyEnable;
   __gen_address_type                           SurfaceStateBaseAddress;
   struct GEN7_MEMORY_OBJECT_CONTROL_STATE      SurfaceStateMemoryObjectControlState;
   uint32_t                                     SurfaceStateBaseAddressModifyEnable;
   __gen_address_type                           DynamicStateBaseAddress;
   struct GEN7_MEMORY_OBJECT_CONTROL_STATE      DynamicStateMemoryObjectControlState;
   uint32_t                                     DynamicStateBaseAddressModifyEnable;
   __gen_address_type                           IndirectObjectBaseAddress;
   struct GEN7_MEMORY_OBJECT_CONTROL_STATE      IndirectObjectMemoryObjectControlState;
   uint32_t                                     IndirectObjectBaseAddressModifyEnable;
   __gen_address_type                           InstructionBaseAddress;
   struct GEN7_MEMORY_OBJECT_CONTROL_STATE      InstructionMemoryObjectControlState;
   uint32_t                                     InstructionBaseAddressModifyEnable;
   __gen_address_type                           GeneralStateAccessUpperBound;
   uint32_t                                     GeneralStateAccessUpperBoundModifyEnable;
   __gen_address_type                           DynamicStateAccessUpperBound;
   uint32_t                                     DynamicStateAccessUpperBoundModifyEnable;
   __gen_address_type                           IndirectObjectAccessUpperBound;
   uint32_t                                     IndirectObjectAccessUpperBoundModifyEnable;
   __gen_address_type                           InstructionAccessUpperBound;
   uint32_t                                     InstructionAccessUpperBoundModifyEnable;
};

static inline void
GEN7_STATE_BASE_ADDRESS_pack(__gen_user_data *data, void * restrict dst,
                            const struct GEN7_STATE_BASE_ADDRESS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   uint32_t dw_GeneralStateMemoryObjectControlState;
   GEN7_MEMORY_OBJECT_CONTROL_STATE_pack(data, &dw_GeneralStateMemoryObjectControlState, &values->GeneralStateMemoryObjectControlState);
   uint32_t dw_StatelessDataPortAccessMemoryObjectControlState;
   GEN7_MEMORY_OBJECT_CONTROL_STATE_pack(data, &dw_StatelessDataPortAccessMemoryObjectControlState, &values->StatelessDataPortAccessMemoryObjectControlState);
   uint32_t dw1 =
      /* Struct  GeneralStateMemoryObjectControlState (8..11): */
      __gen_field(dw_GeneralStateMemoryObjectControlState, 8, 11) |
      /* Struct  StatelessDataPortAccessMemoryObjectControlState (4..7): */
      __gen_field(dw_StatelessDataPortAccessMemoryObjectControlState, 4, 7) |
      __gen_field(values->StatelessDataPortAccessForceWriteThru, 3, 3) |
      __gen_field(values->GeneralStateBaseAddressModifyEnable, 0, 0) |
      0;

   dw[1] =
      __gen_combine_address(data, &dw[1], values->GeneralStateBaseAddress, dw1);

   uint32_t dw_SurfaceStateMemoryObjectControlState;
   GEN7_MEMORY_OBJECT_CONTROL_STATE_pack(data, &dw_SurfaceStateMemoryObjectControlState, &values->SurfaceStateMemoryObjectControlState);
   uint32_t dw2 =
      /* Struct  SurfaceStateMemoryObjectControlState (8..11): */
      __gen_field(dw_SurfaceStateMemoryObjectControlState, 8, 11) |
      __gen_field(values->SurfaceStateBaseAddressModifyEnable, 0, 0) |
      0;

   dw[2] =
      __gen_combine_address(data, &dw[2], values->SurfaceStateBaseAddress, dw2);

   uint32_t dw_DynamicStateMemoryObjectControlState;
   GEN7_MEMORY_OBJECT_CONTROL_STATE_pack(data, &dw_DynamicStateMemoryObjectControlState, &values->DynamicStateMemoryObjectControlState);
   uint32_t dw3 =
      /* Struct  DynamicStateMemoryObjectControlState (8..11): */
      __gen_field(dw_DynamicStateMemoryObjectControlState, 8, 11) |
      __gen_field(values->DynamicStateBaseAddressModifyEnable, 0, 0) |
      0;

   dw[3] =
      __gen_combine_address(data, &dw[3], values->DynamicStateBaseAddress, dw3);

   uint32_t dw_IndirectObjectMemoryObjectControlState;
   GEN7_MEMORY_OBJECT_CONTROL_STATE_pack(data, &dw_IndirectObjectMemoryObjectControlState, &values->IndirectObjectMemoryObjectControlState);
   uint32_t dw4 =
      /* Struct  IndirectObjectMemoryObjectControlState (8..11): */
      __gen_field(dw_IndirectObjectMemoryObjectControlState, 8, 11) |
      __gen_field(values->IndirectObjectBaseAddressModifyEnable, 0, 0) |
      0;

   dw[4] =
      __gen_combine_address(data, &dw[4], values->IndirectObjectBaseAddress, dw4);

   uint32_t dw_InstructionMemoryObjectControlState;
   GEN7_MEMORY_OBJECT_CONTROL_STATE_pack(data, &dw_InstructionMemoryObjectControlState, &values->InstructionMemoryObjectControlState);
   uint32_t dw5 =
      /* Struct  InstructionMemoryObjectControlState (8..11): */
      __gen_field(dw_InstructionMemoryObjectControlState, 8, 11) |
      __gen_field(values->InstructionBaseAddressModifyEnable, 0, 0) |
      0;

   dw[5] =
      __gen_combine_address(data, &dw[5], values->InstructionBaseAddress, dw5);

   uint32_t dw6 =
      __gen_field(values->GeneralStateAccessUpperBoundModifyEnable, 0, 0) |
      0;

   dw[6] =
      __gen_combine_address(data, &dw[6], values->GeneralStateAccessUpperBound, dw6);

   uint32_t dw7 =
      __gen_field(values->DynamicStateAccessUpperBoundModifyEnable, 0, 0) |
      0;

   dw[7] =
      __gen_combine_address(data, &dw[7], values->DynamicStateAccessUpperBound, dw7);

   uint32_t dw8 =
      __gen_field(values->IndirectObjectAccessUpperBoundModifyEnable, 0, 0) |
      0;

   dw[8] =
      __gen_combine_address(data, &dw[8], values->IndirectObjectAccessUpperBound, dw8);

   uint32_t dw9 =
      __gen_field(values->InstructionAccessUpperBoundModifyEnable, 0, 0) |
      0;

   dw[9] =
      __gen_combine_address(data, &dw[9], values->InstructionAccessUpperBound, dw9);

}

#define GEN7_STATE_PREFETCH_length 0x00000002
#define GEN7_STATE_PREFETCH_length_bias 0x00000002
#define GEN7_STATE_PREFETCH_header              \
   .CommandType          =  3,                  \
   .CommandSubType       =  0,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  =  3,                  \
   .DwordLength          =  0

struct GEN7_STATE_PREFETCH {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   __gen_address_type                           PrefetchPointer;
   uint32_t                                     PrefetchCount;
};

static inline void
GEN7_STATE_PREFETCH_pack(__gen_user_data *data, void * restrict dst,
                        const struct GEN7_STATE_PREFETCH * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   uint32_t dw1 =
      __gen_field(values->PrefetchCount, 0, 2) |
      0;

   dw[1] =
      __gen_combine_address(data, &dw[1], values->PrefetchPointer, dw1);

}

#define GEN7_STATE_SIP_length 0x00000002
#define GEN7_STATE_SIP_length_bias 0x00000002
#define GEN7_STATE_SIP_header                   \
   .CommandType          =  3,                  \
   .CommandSubType       =  0,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  =  2,                  \
   .DwordLength          =  0

struct GEN7_STATE_SIP {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     SystemInstructionPointer;
};

static inline void
GEN7_STATE_SIP_pack(__gen_user_data *data, void * restrict dst,
                   const struct GEN7_STATE_SIP * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->SystemInstructionPointer, 4, 31) |
      0;

}

#define GEN7_SWTESS_BASE_ADDRESS_length 0x00000002
#define GEN7_SWTESS_BASE_ADDRESS_length_bias 0x00000002
#define GEN7_SWTESS_BASE_ADDRESS_header         \
   .CommandType          =  3,                  \
   .CommandSubType       =  0,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  =  3,                  \
   .DwordLength          =  0

struct GEN7_SWTESS_BASE_ADDRESS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   __gen_address_type                           SWTessellationBaseAddress;
   struct GEN7_MEMORY_OBJECT_CONTROL_STATE      SWTessellationMemoryObjectControlState;
};

static inline void
GEN7_SWTESS_BASE_ADDRESS_pack(__gen_user_data *data, void * restrict dst,
                             const struct GEN7_SWTESS_BASE_ADDRESS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   uint32_t dw_SWTessellationMemoryObjectControlState;
   GEN7_MEMORY_OBJECT_CONTROL_STATE_pack(data, &dw_SWTessellationMemoryObjectControlState, &values->SWTessellationMemoryObjectControlState);
   uint32_t dw1 =
      /* Struct  SWTessellationMemoryObjectControlState (8..11): */
      __gen_field(dw_SWTessellationMemoryObjectControlState, 8, 11) |
      0;

   dw[1] =
      __gen_combine_address(data, &dw[1], values->SWTessellationBaseAddress, dw1);

}

#define GEN7_3DPRIMITIVE_length 0x00000007
#define GEN7_3DPRIMITIVE_length_bias 0x00000002
#define GEN7_3DPRIMITIVE_header                 \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  3,                  \
   ._3DCommandSubOpcode  =  0,                  \
   .DwordLength          =  5

struct GEN7_3DPRIMITIVE {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     IndirectParameterEnable;
   uint32_t                                     PredicateEnable;
   uint32_t                                     DwordLength;
   uint32_t                                     EndOffsetEnable;
#define     SEQUENTIAL                                         0
#define     RANDOM                                             1
   uint32_t                                     VertexAccessType;
   uint32_t                                     PrimitiveTopologyType;
   uint32_t                                     VertexCountPerInstance;
   uint32_t                                     StartVertexLocation;
   uint32_t                                     InstanceCount;
   uint32_t                                     StartInstanceLocation;
   uint32_t                                     BaseVertexLocation;
};

static inline void
GEN7_3DPRIMITIVE_pack(__gen_user_data *data, void * restrict dst,
                     const struct GEN7_3DPRIMITIVE * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->IndirectParameterEnable, 10, 10) |
      __gen_field(values->PredicateEnable, 8, 8) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->EndOffsetEnable, 9, 9) |
      __gen_field(values->VertexAccessType, 8, 8) |
      __gen_field(values->PrimitiveTopologyType, 0, 5) |
      0;

   dw[2] =
      __gen_field(values->VertexCountPerInstance, 0, 31) |
      0;

   dw[3] =
      __gen_field(values->StartVertexLocation, 0, 31) |
      0;

   dw[4] =
      __gen_field(values->InstanceCount, 0, 31) |
      0;

   dw[5] =
      __gen_field(values->StartInstanceLocation, 0, 31) |
      0;

   dw[6] =
      __gen_field(values->BaseVertexLocation, 0, 31) |
      0;

}

#define GEN7_3DSTATE_AA_LINE_PARAMETERS_length 0x00000003
#define GEN7_3DSTATE_AA_LINE_PARAMETERS_length_bias 0x00000002
#define GEN7_3DSTATE_AA_LINE_PARAMETERS_header  \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  = 10,                  \
   .DwordLength          =  1

struct GEN7_3DSTATE_AA_LINE_PARAMETERS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     AACoverageBias;
   uint32_t                                     AACoverageSlope;
   uint32_t                                     AACoverageEndCapBias;
   uint32_t                                     AACoverageEndCapSlope;
};

static inline void
GEN7_3DSTATE_AA_LINE_PARAMETERS_pack(__gen_user_data *data, void * restrict dst,
                                    const struct GEN7_3DSTATE_AA_LINE_PARAMETERS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->AACoverageBias, 16, 23) |
      __gen_field(values->AACoverageSlope, 0, 7) |
      0;

   dw[2] =
      __gen_field(values->AACoverageEndCapBias, 16, 23) |
      __gen_field(values->AACoverageEndCapSlope, 0, 7) |
      0;

}

#define GEN7_3DSTATE_BINDING_TABLE_POINTERS_DS_length 0x00000002
#define GEN7_3DSTATE_BINDING_TABLE_POINTERS_DS_length_bias 0x00000002
#define GEN7_3DSTATE_BINDING_TABLE_POINTERS_DS_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 40,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_BINDING_TABLE_POINTERS_DS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     PointertoDSBindingTable;
};

static inline void
GEN7_3DSTATE_BINDING_TABLE_POINTERS_DS_pack(__gen_user_data *data, void * restrict dst,
                                           const struct GEN7_3DSTATE_BINDING_TABLE_POINTERS_DS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->PointertoDSBindingTable, 5, 15) |
      0;

}

#define GEN7_3DSTATE_BINDING_TABLE_POINTERS_GS_length 0x00000002
#define GEN7_3DSTATE_BINDING_TABLE_POINTERS_GS_length_bias 0x00000002
#define GEN7_3DSTATE_BINDING_TABLE_POINTERS_GS_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 41,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_BINDING_TABLE_POINTERS_GS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     PointertoGSBindingTable;
};

static inline void
GEN7_3DSTATE_BINDING_TABLE_POINTERS_GS_pack(__gen_user_data *data, void * restrict dst,
                                           const struct GEN7_3DSTATE_BINDING_TABLE_POINTERS_GS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->PointertoGSBindingTable, 5, 15) |
      0;

}

#define GEN7_3DSTATE_BINDING_TABLE_POINTERS_HS_length 0x00000002
#define GEN7_3DSTATE_BINDING_TABLE_POINTERS_HS_length_bias 0x00000002
#define GEN7_3DSTATE_BINDING_TABLE_POINTERS_HS_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 39,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_BINDING_TABLE_POINTERS_HS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     PointertoHSBindingTable;
};

static inline void
GEN7_3DSTATE_BINDING_TABLE_POINTERS_HS_pack(__gen_user_data *data, void * restrict dst,
                                           const struct GEN7_3DSTATE_BINDING_TABLE_POINTERS_HS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->PointertoHSBindingTable, 5, 15) |
      0;

}

#define GEN7_3DSTATE_BINDING_TABLE_POINTERS_PS_length 0x00000002
#define GEN7_3DSTATE_BINDING_TABLE_POINTERS_PS_length_bias 0x00000002
#define GEN7_3DSTATE_BINDING_TABLE_POINTERS_PS_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 42,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_BINDING_TABLE_POINTERS_PS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     PointertoPSBindingTable;
};

static inline void
GEN7_3DSTATE_BINDING_TABLE_POINTERS_PS_pack(__gen_user_data *data, void * restrict dst,
                                           const struct GEN7_3DSTATE_BINDING_TABLE_POINTERS_PS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->PointertoPSBindingTable, 5, 15) |
      0;

}

#define GEN7_3DSTATE_BINDING_TABLE_POINTERS_VS_length 0x00000002
#define GEN7_3DSTATE_BINDING_TABLE_POINTERS_VS_length_bias 0x00000002
#define GEN7_3DSTATE_BINDING_TABLE_POINTERS_VS_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 38,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_BINDING_TABLE_POINTERS_VS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     PointertoVSBindingTable;
};

static inline void
GEN7_3DSTATE_BINDING_TABLE_POINTERS_VS_pack(__gen_user_data *data, void * restrict dst,
                                           const struct GEN7_3DSTATE_BINDING_TABLE_POINTERS_VS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->PointertoVSBindingTable, 5, 15) |
      0;

}

#define GEN7_3DSTATE_BLEND_STATE_POINTERS_length 0x00000002
#define GEN7_3DSTATE_BLEND_STATE_POINTERS_length_bias 0x00000002
#define GEN7_3DSTATE_BLEND_STATE_POINTERS_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 36,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_BLEND_STATE_POINTERS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     BlendStatePointer;
};

static inline void
GEN7_3DSTATE_BLEND_STATE_POINTERS_pack(__gen_user_data *data, void * restrict dst,
                                      const struct GEN7_3DSTATE_BLEND_STATE_POINTERS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->BlendStatePointer, 6, 31) |
      0;

}

#define GEN7_3DSTATE_CC_STATE_POINTERS_length 0x00000002
#define GEN7_3DSTATE_CC_STATE_POINTERS_length_bias 0x00000002
#define GEN7_3DSTATE_CC_STATE_POINTERS_header   \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 14,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_CC_STATE_POINTERS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     ColorCalcStatePointer;
};

static inline void
GEN7_3DSTATE_CC_STATE_POINTERS_pack(__gen_user_data *data, void * restrict dst,
                                   const struct GEN7_3DSTATE_CC_STATE_POINTERS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->ColorCalcStatePointer, 6, 31) |
      0;

}

#define GEN7_3DSTATE_CHROMA_KEY_length 0x00000004
#define GEN7_3DSTATE_CHROMA_KEY_length_bias 0x00000002
#define GEN7_3DSTATE_CHROMA_KEY_header          \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  =  4,                  \
   .DwordLength          =  2

struct GEN7_3DSTATE_CHROMA_KEY {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     ChromaKeyTableIndex;
   uint32_t                                     ChromaKeyLowValue;
   uint32_t                                     ChromaKeyHighValue;
};

static inline void
GEN7_3DSTATE_CHROMA_KEY_pack(__gen_user_data *data, void * restrict dst,
                            const struct GEN7_3DSTATE_CHROMA_KEY * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->ChromaKeyTableIndex, 30, 31) |
      0;

   dw[2] =
      __gen_field(values->ChromaKeyLowValue, 0, 31) |
      0;

   dw[3] =
      __gen_field(values->ChromaKeyHighValue, 0, 31) |
      0;

}

#define GEN7_3DSTATE_CLEAR_PARAMS_length 0x00000003
#define GEN7_3DSTATE_CLEAR_PARAMS_length_bias 0x00000002
#define GEN7_3DSTATE_CLEAR_PARAMS_header        \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  =  4,                  \
   .DwordLength          =  1

struct GEN7_3DSTATE_CLEAR_PARAMS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     DepthClearValue;
   uint32_t                                     DepthClearValueValid;
};

static inline void
GEN7_3DSTATE_CLEAR_PARAMS_pack(__gen_user_data *data, void * restrict dst,
                              const struct GEN7_3DSTATE_CLEAR_PARAMS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->DepthClearValue, 0, 31) |
      0;

   dw[2] =
      __gen_field(values->DepthClearValueValid, 0, 0) |
      0;

}

#define GEN7_3DSTATE_CLIP_length 0x00000004
#define GEN7_3DSTATE_CLIP_length_bias 0x00000002
#define GEN7_3DSTATE_CLIP_header                \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 18,                  \
   .DwordLength          =  2

struct GEN7_3DSTATE_CLIP {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     FrontWinding;
   uint32_t                                     VertexSubPixelPrecisionSelect;
   uint32_t                                     EarlyCullEnable;
#define     CULLMODE_BOTH                                      0
#define     CULLMODE_NONE                                      1
#define     CULLMODE_FRONT                                     2
#define     CULLMODE_BACK                                      3
   uint32_t                                     CullMode;
   uint32_t                                     ClipperStatisticsEnable;
   uint32_t                                     UserClipDistanceCullTestEnableBitmask;
   uint32_t                                     ClipEnable;
#define     APIMODE_OGL                                        0
   uint32_t                                     APIMode;
   uint32_t                                     ViewportXYClipTestEnable;
   uint32_t                                     ViewportZClipTestEnable;
   uint32_t                                     GuardbandClipTestEnable;
   uint32_t                                     UserClipDistanceClipTestEnableBitmask;
#define     CLIPMODE_NORMAL                                    0
#define     CLIPMODE_REJECT_ALL                                3
#define     CLIPMODE_ACCEPT_ALL                                4
   uint32_t                                     ClipMode;
   uint32_t                                     PerspectiveDivideDisable;
   uint32_t                                     NonPerspectiveBarycentricEnable;
#define     Vertex0                                            0
#define     Vertex1                                            1
#define     Vertex2                                            2
   uint32_t                                     TriangleStripListProvokingVertexSelect;
#define     Vertex0                                            0
#define     Vertex1                                            1
   uint32_t                                     LineStripListProvokingVertexSelect;
#define     Vertex0                                            0
#define     Vertex1                                            1
#define     Vertex2                                            2
   uint32_t                                     TriangleFanProvokingVertexSelect;
   uint32_t                                     MinimumPointWidth;
   uint32_t                                     MaximumPointWidth;
   uint32_t                                     ForceZeroRTAIndexEnable;
   uint32_t                                     MaximumVPIndex;
};

static inline void
GEN7_3DSTATE_CLIP_pack(__gen_user_data *data, void * restrict dst,
                      const struct GEN7_3DSTATE_CLIP * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->FrontWinding, 20, 20) |
      __gen_field(values->VertexSubPixelPrecisionSelect, 19, 19) |
      __gen_field(values->EarlyCullEnable, 18, 18) |
      __gen_field(values->CullMode, 16, 17) |
      __gen_field(values->ClipperStatisticsEnable, 10, 10) |
      __gen_field(values->UserClipDistanceCullTestEnableBitmask, 0, 7) |
      0;

   dw[2] =
      __gen_field(values->ClipEnable, 31, 31) |
      __gen_field(values->APIMode, 30, 30) |
      __gen_field(values->ViewportXYClipTestEnable, 28, 28) |
      __gen_field(values->ViewportZClipTestEnable, 27, 27) |
      __gen_field(values->GuardbandClipTestEnable, 26, 26) |
      __gen_field(values->UserClipDistanceClipTestEnableBitmask, 16, 23) |
      __gen_field(values->ClipMode, 13, 15) |
      __gen_field(values->PerspectiveDivideDisable, 9, 9) |
      __gen_field(values->NonPerspectiveBarycentricEnable, 8, 8) |
      __gen_field(values->TriangleStripListProvokingVertexSelect, 4, 5) |
      __gen_field(values->LineStripListProvokingVertexSelect, 2, 3) |
      __gen_field(values->TriangleFanProvokingVertexSelect, 0, 1) |
      0;

   dw[3] =
      __gen_field(values->MinimumPointWidth, 17, 27) |
      __gen_field(values->MaximumPointWidth, 6, 16) |
      __gen_field(values->ForceZeroRTAIndexEnable, 5, 5) |
      __gen_field(values->MaximumVPIndex, 0, 3) |
      0;

}

#define GEN7_3DSTATE_CONSTANT_DS_length 0x00000007
#define GEN7_3DSTATE_CONSTANT_DS_length_bias 0x00000002
#define GEN7_3DSTATE_CONSTANT_DS_header         \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 26,                  \
   .DwordLength          =  5

struct GEN7_3DSTATE_CONSTANT_BODY {
   uint32_t                                     ConstantBuffer1ReadLength;
   uint32_t                                     ConstantBuffer0ReadLength;
   uint32_t                                     ConstantBuffer3ReadLength;
   uint32_t                                     ConstantBuffer2ReadLength;
   __gen_address_type                           PointerToConstantBuffer0;
   struct GEN7_MEMORY_OBJECT_CONTROL_STATE      ConstantBufferObjectControlState;
   __gen_address_type                           PointerToConstantBuffer1;
   __gen_address_type                           PointerToConstantBuffer2;
   __gen_address_type                           PointerToConstantBuffer3;
};

static inline void
GEN7_3DSTATE_CONSTANT_BODY_pack(__gen_user_data *data, void * restrict dst,
                               const struct GEN7_3DSTATE_CONSTANT_BODY * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->ConstantBuffer1ReadLength, 16, 31) |
      __gen_field(values->ConstantBuffer0ReadLength, 0, 15) |
      0;

   dw[1] =
      __gen_field(values->ConstantBuffer3ReadLength, 16, 31) |
      __gen_field(values->ConstantBuffer2ReadLength, 0, 15) |
      0;

   uint32_t dw_ConstantBufferObjectControlState;
   GEN7_MEMORY_OBJECT_CONTROL_STATE_pack(data, &dw_ConstantBufferObjectControlState, &values->ConstantBufferObjectControlState);
   uint32_t dw2 =
      /* Struct  ConstantBufferObjectControlState (0..4): */
      __gen_field(dw_ConstantBufferObjectControlState, 0, 4) |
      0;

   dw[2] =
      __gen_combine_address(data, &dw[2], values->PointerToConstantBuffer0, dw2);

   uint32_t dw3 =
      0;

   dw[3] =
      __gen_combine_address(data, &dw[3], values->PointerToConstantBuffer1, dw3);

   uint32_t dw4 =
      0;

   dw[4] =
      __gen_combine_address(data, &dw[4], values->PointerToConstantBuffer2, dw4);

   uint32_t dw5 =
      0;

   dw[5] =
      __gen_combine_address(data, &dw[5], values->PointerToConstantBuffer3, dw5);

}

struct GEN7_3DSTATE_CONSTANT_DS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   struct GEN7_3DSTATE_CONSTANT_BODY            ConstantBody;
};

static inline void
GEN7_3DSTATE_CONSTANT_DS_pack(__gen_user_data *data, void * restrict dst,
                             const struct GEN7_3DSTATE_CONSTANT_DS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   GEN7_3DSTATE_CONSTANT_BODY_pack(data, &dw[1], &values->ConstantBody);
}

#define GEN7_3DSTATE_CONSTANT_GS_length 0x00000007
#define GEN7_3DSTATE_CONSTANT_GS_length_bias 0x00000002
#define GEN7_3DSTATE_CONSTANT_GS_header         \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 22,                  \
   .DwordLength          =  5

struct GEN7_3DSTATE_CONSTANT_GS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   struct GEN7_3DSTATE_CONSTANT_BODY            ConstantBody;
};

static inline void
GEN7_3DSTATE_CONSTANT_GS_pack(__gen_user_data *data, void * restrict dst,
                             const struct GEN7_3DSTATE_CONSTANT_GS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   GEN7_3DSTATE_CONSTANT_BODY_pack(data, &dw[1], &values->ConstantBody);
}

#define GEN7_3DSTATE_CONSTANT_HS_length 0x00000007
#define GEN7_3DSTATE_CONSTANT_HS_length_bias 0x00000002
#define GEN7_3DSTATE_CONSTANT_HS_header         \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 25,                  \
   .DwordLength          =  5

struct GEN7_3DSTATE_CONSTANT_HS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   struct GEN7_3DSTATE_CONSTANT_BODY            ConstantBody;
};

static inline void
GEN7_3DSTATE_CONSTANT_HS_pack(__gen_user_data *data, void * restrict dst,
                             const struct GEN7_3DSTATE_CONSTANT_HS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   GEN7_3DSTATE_CONSTANT_BODY_pack(data, &dw[1], &values->ConstantBody);
}

#define GEN7_3DSTATE_CONSTANT_PS_length 0x00000007
#define GEN7_3DSTATE_CONSTANT_PS_length_bias 0x00000002
#define GEN7_3DSTATE_CONSTANT_PS_header         \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 23,                  \
   .DwordLength          =  5

struct GEN7_3DSTATE_CONSTANT_PS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   struct GEN7_3DSTATE_CONSTANT_BODY            ConstantBody;
};

static inline void
GEN7_3DSTATE_CONSTANT_PS_pack(__gen_user_data *data, void * restrict dst,
                             const struct GEN7_3DSTATE_CONSTANT_PS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   GEN7_3DSTATE_CONSTANT_BODY_pack(data, &dw[1], &values->ConstantBody);
}

#define GEN7_3DSTATE_CONSTANT_VS_length 0x00000007
#define GEN7_3DSTATE_CONSTANT_VS_length_bias 0x00000002
#define GEN7_3DSTATE_CONSTANT_VS_header         \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 21,                  \
   .DwordLength          =  5

struct GEN7_3DSTATE_CONSTANT_VS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   struct GEN7_3DSTATE_CONSTANT_BODY            ConstantBody;
};

static inline void
GEN7_3DSTATE_CONSTANT_VS_pack(__gen_user_data *data, void * restrict dst,
                             const struct GEN7_3DSTATE_CONSTANT_VS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   GEN7_3DSTATE_CONSTANT_BODY_pack(data, &dw[1], &values->ConstantBody);
}

#define GEN7_3DSTATE_DEPTH_BUFFER_length 0x00000007
#define GEN7_3DSTATE_DEPTH_BUFFER_length_bias 0x00000002
#define GEN7_3DSTATE_DEPTH_BUFFER_header        \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  =  5,                  \
   .DwordLength          =  5

struct GEN7_3DSTATE_DEPTH_BUFFER {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
#define     SURFTYPE_1D                                        0
#define     SURFTYPE_2D                                        1
#define     SURFTYPE_3D                                        2
#define     SURFTYPE_CUBE                                      3
#define     SURFTYPE_NULL                                      7
   uint32_t                                     SurfaceType;
   uint32_t                                     DepthWriteEnable;
   uint32_t                                     StencilWriteEnable;
   uint32_t                                     HierarchicalDepthBufferEnable;
#define     D32_FLOAT                                          1
#define     D24_UNORM_X8_UINT                                  3
#define     D16_UNORM                                          5
   uint32_t                                     SurfaceFormat;
   uint32_t                                     SurfacePitch;
   __gen_address_type                           SurfaceBaseAddress;
   uint32_t                                     Height;
   uint32_t                                     Width;
   uint32_t                                     LOD;
#define     SURFTYPE_CUBEmustbezero                            0
   uint32_t                                     Depth;
   uint32_t                                     MinimumArrayElement;
   struct GEN7_MEMORY_OBJECT_CONTROL_STATE      DepthBufferObjectControlState;
   uint32_t                                     DepthCoordinateOffsetY;
   uint32_t                                     DepthCoordinateOffsetX;
   uint32_t                                     RenderTargetViewExtent;
};

static inline void
GEN7_3DSTATE_DEPTH_BUFFER_pack(__gen_user_data *data, void * restrict dst,
                              const struct GEN7_3DSTATE_DEPTH_BUFFER * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->SurfaceType, 29, 31) |
      __gen_field(values->DepthWriteEnable, 28, 28) |
      __gen_field(values->StencilWriteEnable, 27, 27) |
      __gen_field(values->HierarchicalDepthBufferEnable, 22, 22) |
      __gen_field(values->SurfaceFormat, 18, 20) |
      __gen_field(values->SurfacePitch, 0, 17) |
      0;

   uint32_t dw2 =
      0;

   dw[2] =
      __gen_combine_address(data, &dw[2], values->SurfaceBaseAddress, dw2);

   dw[3] =
      __gen_field(values->Height, 18, 31) |
      __gen_field(values->Width, 4, 17) |
      __gen_field(values->LOD, 0, 3) |
      0;

   uint32_t dw_DepthBufferObjectControlState;
   GEN7_MEMORY_OBJECT_CONTROL_STATE_pack(data, &dw_DepthBufferObjectControlState, &values->DepthBufferObjectControlState);
   dw[4] =
      __gen_field(values->Depth, 21, 31) |
      __gen_field(values->MinimumArrayElement, 10, 20) |
      /* Struct  DepthBufferObjectControlState (0..3): */
      __gen_field(dw_DepthBufferObjectControlState, 0, 3) |
      0;

   dw[5] =
      __gen_field(values->DepthCoordinateOffsetY, 16, 31) |
      __gen_field(values->DepthCoordinateOffsetX, 0, 15) |
      0;

   dw[6] =
      __gen_field(values->RenderTargetViewExtent, 21, 31) |
      0;

}

#define GEN7_3DSTATE_DEPTH_STENCIL_STATE_POINTERS_length 0x00000002
#define GEN7_3DSTATE_DEPTH_STENCIL_STATE_POINTERS_length_bias 0x00000002
#define GEN7_3DSTATE_DEPTH_STENCIL_STATE_POINTERS_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 37,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_DEPTH_STENCIL_STATE_POINTERS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     PointertoDEPTH_STENCIL_STATE;
};

static inline void
GEN7_3DSTATE_DEPTH_STENCIL_STATE_POINTERS_pack(__gen_user_data *data, void * restrict dst,
                                              const struct GEN7_3DSTATE_DEPTH_STENCIL_STATE_POINTERS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->PointertoDEPTH_STENCIL_STATE, 6, 31) |
      0;

}

#define GEN7_3DSTATE_DRAWING_RECTANGLE_length 0x00000004
#define GEN7_3DSTATE_DRAWING_RECTANGLE_length_bias 0x00000002
#define GEN7_3DSTATE_DRAWING_RECTANGLE_header   \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  =  0,                  \
   .DwordLength          =  2

struct GEN7_3DSTATE_DRAWING_RECTANGLE {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     ClippedDrawingRectangleYMin;
   uint32_t                                     ClippedDrawingRectangleXMin;
   uint32_t                                     ClippedDrawingRectangleYMax;
   uint32_t                                     ClippedDrawingRectangleXMax;
   uint32_t                                     DrawingRectangleOriginY;
   uint32_t                                     DrawingRectangleOriginX;
};

static inline void
GEN7_3DSTATE_DRAWING_RECTANGLE_pack(__gen_user_data *data, void * restrict dst,
                                   const struct GEN7_3DSTATE_DRAWING_RECTANGLE * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->ClippedDrawingRectangleYMin, 16, 31) |
      __gen_field(values->ClippedDrawingRectangleXMin, 0, 15) |
      0;

   dw[2] =
      __gen_field(values->ClippedDrawingRectangleYMax, 16, 31) |
      __gen_field(values->ClippedDrawingRectangleXMax, 0, 15) |
      0;

   dw[3] =
      __gen_field(values->DrawingRectangleOriginY, 16, 31) |
      __gen_field(values->DrawingRectangleOriginX, 0, 15) |
      0;

}

#define GEN7_3DSTATE_DS_length 0x00000006
#define GEN7_3DSTATE_DS_length_bias 0x00000002
#define GEN7_3DSTATE_DS_header                  \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 29,                  \
   .DwordLength          =  4

struct GEN7_3DSTATE_DS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     KernelStartPointer;
#define     Multiple                                           0
#define     Single                                             1
   uint32_t                                     SingleDomainPointDispatch;
#define     Dmask                                              0
#define     Vmask                                              1
   uint32_t                                     VectorMaskEnable;
#define     NoSamplers                                         0
#define     _14Samplers                                        1
#define     _58Samplers                                        2
#define     _912Samplers                                       3
#define     _1316Samplers                                      4
   uint32_t                                     SamplerCount;
   uint32_t                                     BindingTableEntryCount;
#define     IEEE754                                            0
#define     Alternate                                          1
   uint32_t                                     FloatingPointMode;
   uint32_t                                     IllegalOpcodeExceptionEnable;
   uint32_t                                     SoftwareExceptionEnable;
   uint32_t                                     ScratchSpaceBasePointer;
   uint32_t                                     PerThreadScratchSpace;
   uint32_t                                     DispatchGRFStartRegisterForURBData;
   uint32_t                                     PatchURBEntryReadLength;
   uint32_t                                     PatchURBEntryReadOffset;
   uint32_t                                     MaximumNumberofThreads;
   uint32_t                                     StatisticsEnable;
   uint32_t                                     ComputeWCoordinateEnable;
   uint32_t                                     DSCacheDisable;
   uint32_t                                     DSFunctionEnable;
};

static inline void
GEN7_3DSTATE_DS_pack(__gen_user_data *data, void * restrict dst,
                    const struct GEN7_3DSTATE_DS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->KernelStartPointer, 6, 31) |
      0;

   dw[2] =
      __gen_field(values->SingleDomainPointDispatch, 31, 31) |
      __gen_field(values->VectorMaskEnable, 30, 30) |
      __gen_field(values->SamplerCount, 27, 29) |
      __gen_field(values->BindingTableEntryCount, 18, 25) |
      __gen_field(values->FloatingPointMode, 16, 16) |
      __gen_field(values->IllegalOpcodeExceptionEnable, 13, 13) |
      __gen_field(values->SoftwareExceptionEnable, 7, 7) |
      0;

   dw[3] =
      __gen_offset(values->ScratchSpaceBasePointer, 10, 31) |
      __gen_field(values->PerThreadScratchSpace, 0, 3) |
      0;

   dw[4] =
      __gen_field(values->DispatchGRFStartRegisterForURBData, 20, 24) |
      __gen_field(values->PatchURBEntryReadLength, 11, 17) |
      __gen_field(values->PatchURBEntryReadOffset, 4, 9) |
      0;

   dw[5] =
      __gen_field(values->MaximumNumberofThreads, 25, 31) |
      __gen_field(values->StatisticsEnable, 10, 10) |
      __gen_field(values->ComputeWCoordinateEnable, 2, 2) |
      __gen_field(values->DSCacheDisable, 1, 1) |
      __gen_field(values->DSFunctionEnable, 0, 0) |
      0;

}

#define GEN7_3DSTATE_GS_length 0x00000007
#define GEN7_3DSTATE_GS_length_bias 0x00000002
#define GEN7_3DSTATE_GS_header                  \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 17,                  \
   .DwordLength          =  5

struct GEN7_3DSTATE_GS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     KernelStartPointer;
   uint32_t                                     SingleProgramFlowSPF;
#define     Dmask                                              0
#define     Vmask                                              1
   uint32_t                                     VectorMaskEnableVME;
#define     NoSamplers                                         0
#define     _14Samplers                                        1
#define     _58Samplers                                        2
#define     _912Samplers                                       3
#define     _1316Samplers                                      4
   uint32_t                                     SamplerCount;
   uint32_t                                     BindingTableEntryCount;
#define     NormalPriority                                     0
#define     HighPriority                                       1
   uint32_t                                     ThreadPriority;
#define     IEEE754                                            0
#define     alternate                                          1
   uint32_t                                     FloatingPointMode;
   uint32_t                                     IllegalOpcodeExceptionEnable;
   uint32_t                                     MaskStackExceptionEnable;
   uint32_t                                     SoftwareExceptionEnable;
   uint32_t                                     ScratchSpaceBasePointer;
   uint32_t                                     PerThreadScratchSpace;
   uint32_t                                     OutputVertexSize;
   uint32_t                                     OutputTopology;
   uint32_t                                     VertexURBEntryReadLength;
   uint32_t                                     IncludeVertexHandles;
   uint32_t                                     VertexURBEntryReadOffset;
   uint32_t                                     DispatchGRFStartRegisterforURBData;
   uint32_t                                     MaximumNumberofThreads;
#define     GSCTL_CUT                                          0
#define     GSCTL_SID                                          1
   uint32_t                                     ControlDataFormat;
   uint32_t                                     ControlDataHeaderSize;
   uint32_t                                     InstanceControl;
   uint32_t                                     DefaultStreamID;
#define     SINGLE                                             0
#define     DUAL_INSTANCE                                      1
#define     DUAL_OBJECT                                        2
   uint32_t                                     DispatchMode;
   uint32_t                                     GSStatisticsEnable;
   uint32_t                                     GSInvocationsIncrementValue;
   uint32_t                                     IncludePrimitiveID;
   uint32_t                                     Hint;
   uint32_t                                     ReorderEnable;
   uint32_t                                     DiscardAdjacency;
   uint32_t                                     GSEnable;
   uint32_t                                     SemaphoreHandle;
};

static inline void
GEN7_3DSTATE_GS_pack(__gen_user_data *data, void * restrict dst,
                    const struct GEN7_3DSTATE_GS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->KernelStartPointer, 6, 31) |
      0;

   dw[2] =
      __gen_field(values->SingleProgramFlowSPF, 31, 31) |
      __gen_field(values->VectorMaskEnableVME, 30, 30) |
      __gen_field(values->SamplerCount, 27, 29) |
      __gen_field(values->BindingTableEntryCount, 18, 25) |
      __gen_field(values->ThreadPriority, 17, 17) |
      __gen_field(values->FloatingPointMode, 16, 16) |
      __gen_field(values->IllegalOpcodeExceptionEnable, 13, 13) |
      __gen_field(values->MaskStackExceptionEnable, 11, 11) |
      __gen_field(values->SoftwareExceptionEnable, 7, 7) |
      0;

   dw[3] =
      __gen_offset(values->ScratchSpaceBasePointer, 10, 31) |
      __gen_field(values->PerThreadScratchSpace, 0, 3) |
      0;

   dw[4] =
      __gen_field(values->OutputVertexSize, 23, 28) |
      __gen_field(values->OutputTopology, 17, 22) |
      __gen_field(values->VertexURBEntryReadLength, 11, 16) |
      __gen_field(values->IncludeVertexHandles, 10, 10) |
      __gen_field(values->VertexURBEntryReadOffset, 4, 9) |
      __gen_field(values->DispatchGRFStartRegisterforURBData, 0, 3) |
      0;

   dw[5] =
      __gen_field(values->MaximumNumberofThreads, 25, 31) |
      __gen_field(values->ControlDataFormat, 24, 24) |
      __gen_field(values->ControlDataHeaderSize, 20, 23) |
      __gen_field(values->InstanceControl, 15, 19) |
      __gen_field(values->DefaultStreamID, 13, 14) |
      __gen_field(values->DispatchMode, 11, 12) |
      __gen_field(values->GSStatisticsEnable, 10, 10) |
      __gen_field(values->GSInvocationsIncrementValue, 5, 9) |
      __gen_field(values->IncludePrimitiveID, 4, 4) |
      __gen_field(values->Hint, 3, 3) |
      __gen_field(values->ReorderEnable, 2, 2) |
      __gen_field(values->DiscardAdjacency, 1, 1) |
      __gen_field(values->GSEnable, 0, 0) |
      0;

   dw[6] =
      __gen_offset(values->SemaphoreHandle, 0, 11) |
      0;

}

#define GEN7_3DSTATE_HIER_DEPTH_BUFFER_length 0x00000003
#define GEN7_3DSTATE_HIER_DEPTH_BUFFER_length_bias 0x00000002
#define GEN7_3DSTATE_HIER_DEPTH_BUFFER_header   \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  =  7,                  \
   .DwordLength          =  1

struct GEN7_3DSTATE_HIER_DEPTH_BUFFER {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   struct GEN7_MEMORY_OBJECT_CONTROL_STATE      HierarchicalDepthBufferObjectControlState;
   uint32_t                                     SurfacePitch;
   __gen_address_type                           SurfaceBaseAddress;
};

static inline void
GEN7_3DSTATE_HIER_DEPTH_BUFFER_pack(__gen_user_data *data, void * restrict dst,
                                   const struct GEN7_3DSTATE_HIER_DEPTH_BUFFER * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   uint32_t dw_HierarchicalDepthBufferObjectControlState;
   GEN7_MEMORY_OBJECT_CONTROL_STATE_pack(data, &dw_HierarchicalDepthBufferObjectControlState, &values->HierarchicalDepthBufferObjectControlState);
   dw[1] =
      /* Struct  HierarchicalDepthBufferObjectControlState (25..28): */
      __gen_field(dw_HierarchicalDepthBufferObjectControlState, 25, 28) |
      __gen_field(values->SurfacePitch, 0, 16) |
      0;

   uint32_t dw2 =
      0;

   dw[2] =
      __gen_combine_address(data, &dw[2], values->SurfaceBaseAddress, dw2);

}

#define GEN7_3DSTATE_HS_length 0x00000007
#define GEN7_3DSTATE_HS_length_bias 0x00000002
#define GEN7_3DSTATE_HS_header                  \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 27,                  \
   .DwordLength          =  5

struct GEN7_3DSTATE_HS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
#define     NoSamplers                                         0
#define     _14Samplers                                        1
#define     _58Samplers                                        2
#define     _912Samplers                                       3
#define     _1316Samplers                                      4
   uint32_t                                     SamplerCount;
   uint32_t                                     BindingTableEntryCount;
#define     IEEE754                                            0
#define     alternate                                          1
   uint32_t                                     FloatingPointMode;
   uint32_t                                     IllegalOpcodeExceptionEnable;
   uint32_t                                     SoftwareExceptionEnable;
   uint32_t                                     MaximumNumberofThreads;
   uint32_t                                     Enable;
   uint32_t                                     StatisticsEnable;
   uint32_t                                     InstanceCount;
   uint32_t                                     KernelStartPointer;
   uint32_t                                     ScratchSpaceBasePointer;
   uint32_t                                     PerThreadScratchSpace;
   uint32_t                                     SingleProgramFlow;
#define     Dmask                                              0
#define     Vmask                                              1
   uint32_t                                     VectorMaskEnable;
   uint32_t                                     IncludeVertexHandles;
   uint32_t                                     DispatchGRFStartRegisterForURBData;
   uint32_t                                     VertexURBEntryReadLength;
   uint32_t                                     VertexURBEntryReadOffset;
   uint32_t                                     SemaphoreHandle;
};

static inline void
GEN7_3DSTATE_HS_pack(__gen_user_data *data, void * restrict dst,
                    const struct GEN7_3DSTATE_HS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->SamplerCount, 27, 29) |
      __gen_field(values->BindingTableEntryCount, 18, 25) |
      __gen_field(values->FloatingPointMode, 16, 16) |
      __gen_field(values->IllegalOpcodeExceptionEnable, 13, 13) |
      __gen_field(values->SoftwareExceptionEnable, 7, 7) |
      __gen_field(values->MaximumNumberofThreads, 0, 6) |
      0;

   dw[2] =
      __gen_field(values->Enable, 31, 31) |
      __gen_field(values->StatisticsEnable, 29, 29) |
      __gen_field(values->InstanceCount, 0, 3) |
      0;

   dw[3] =
      __gen_offset(values->KernelStartPointer, 6, 31) |
      0;

   dw[4] =
      __gen_offset(values->ScratchSpaceBasePointer, 10, 31) |
      __gen_field(values->PerThreadScratchSpace, 0, 3) |
      0;

   dw[5] =
      __gen_field(values->SingleProgramFlow, 27, 27) |
      __gen_field(values->VectorMaskEnable, 26, 26) |
      __gen_field(values->IncludeVertexHandles, 24, 24) |
      __gen_field(values->DispatchGRFStartRegisterForURBData, 19, 23) |
      __gen_field(values->VertexURBEntryReadLength, 11, 16) |
      __gen_field(values->VertexURBEntryReadOffset, 4, 9) |
      0;

   dw[6] =
      __gen_offset(values->SemaphoreHandle, 0, 11) |
      0;

}

#define GEN7_3DSTATE_INDEX_BUFFER_length 0x00000003
#define GEN7_3DSTATE_INDEX_BUFFER_length_bias 0x00000002
#define GEN7_3DSTATE_INDEX_BUFFER_header        \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 10,                  \
   .DwordLength          =  1

struct GEN7_3DSTATE_INDEX_BUFFER {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   struct GEN7_MEMORY_OBJECT_CONTROL_STATE      MemoryObjectControlState;
   uint32_t                                     CutIndexEnable;
#define     INDEX_BYTE                                         0
#define     INDEX_WORD                                         1
#define     INDEX_DWORD                                        2
   uint32_t                                     IndexFormat;
   uint32_t                                     DwordLength;
   __gen_address_type                           BufferStartingAddress;
   __gen_address_type                           BufferEndingAddress;
};

static inline void
GEN7_3DSTATE_INDEX_BUFFER_pack(__gen_user_data *data, void * restrict dst,
                              const struct GEN7_3DSTATE_INDEX_BUFFER * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   uint32_t dw_MemoryObjectControlState;
   GEN7_MEMORY_OBJECT_CONTROL_STATE_pack(data, &dw_MemoryObjectControlState, &values->MemoryObjectControlState);
   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      /* Struct  MemoryObjectControlState (12..15): */
      __gen_field(dw_MemoryObjectControlState, 12, 15) |
      __gen_field(values->CutIndexEnable, 10, 10) |
      __gen_field(values->IndexFormat, 8, 9) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   uint32_t dw1 =
      0;

   dw[1] =
      __gen_combine_address(data, &dw[1], values->BufferStartingAddress, dw1);

   uint32_t dw2 =
      0;

   dw[2] =
      __gen_combine_address(data, &dw[2], values->BufferEndingAddress, dw2);

}

#define GEN7_3DSTATE_LINE_STIPPLE_length 0x00000003
#define GEN7_3DSTATE_LINE_STIPPLE_length_bias 0x00000002
#define GEN7_3DSTATE_LINE_STIPPLE_header        \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  =  8,                  \
   .DwordLength          =  1

struct GEN7_3DSTATE_LINE_STIPPLE {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     ModifyEnableCurrentRepeatCounterCurrentStippleIndex;
   uint32_t                                     CurrentRepeatCounter;
   uint32_t                                     CurrentStippleIndex;
   uint32_t                                     LineStipplePattern;
   uint32_t                                     LineStippleInverseRepeatCount;
   uint32_t                                     LineStippleRepeatCount;
};

static inline void
GEN7_3DSTATE_LINE_STIPPLE_pack(__gen_user_data *data, void * restrict dst,
                              const struct GEN7_3DSTATE_LINE_STIPPLE * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->ModifyEnableCurrentRepeatCounterCurrentStippleIndex, 31, 31) |
      __gen_field(values->CurrentRepeatCounter, 21, 29) |
      __gen_field(values->CurrentStippleIndex, 16, 19) |
      __gen_field(values->LineStipplePattern, 0, 15) |
      0;

   dw[2] =
      __gen_field(values->LineStippleInverseRepeatCount, 15, 31) |
      __gen_field(values->LineStippleRepeatCount, 0, 8) |
      0;

}

#define GEN7_3DSTATE_MONOFILTER_SIZE_length 0x00000002
#define GEN7_3DSTATE_MONOFILTER_SIZE_length_bias 0x00000002
#define GEN7_3DSTATE_MONOFILTER_SIZE_header     \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  = 17,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_MONOFILTER_SIZE {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     MonochromeFilterWidth;
   uint32_t                                     MonochromeFilterHeight;
};

static inline void
GEN7_3DSTATE_MONOFILTER_SIZE_pack(__gen_user_data *data, void * restrict dst,
                                 const struct GEN7_3DSTATE_MONOFILTER_SIZE * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->MonochromeFilterWidth, 3, 5) |
      __gen_field(values->MonochromeFilterHeight, 0, 2) |
      0;

}

#define GEN7_3DSTATE_MULTISAMPLE_length 0x00000004
#define GEN7_3DSTATE_MULTISAMPLE_length_bias 0x00000002
#define GEN7_3DSTATE_MULTISAMPLE_header         \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  = 13,                  \
   .DwordLength          =  2

struct GEN7_3DSTATE_MULTISAMPLE {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
#define     PIXLOC_CENTER                                      0
#define     PIXLOC_UL_CORNER                                   1
   uint32_t                                     PixelLocation;
#define     NUMSAMPLES_1                                       0
#define     NUMSAMPLES_4                                       2
#define     NUMSAMPLES_8                                       3
   uint32_t                                     NumberofMultisamples;
   uint32_t                                     Sample3XOffset;
   uint32_t                                     Sample3YOffset;
   uint32_t                                     Sample2XOffset;
   uint32_t                                     Sample2YOffset;
   uint32_t                                     Sample1XOffset;
   uint32_t                                     Sample1YOffset;
   uint32_t                                     Sample0XOffset;
   uint32_t                                     Sample0YOffset;
   uint32_t                                     Sample7XOffset;
   uint32_t                                     Sample7YOffset;
   uint32_t                                     Sample6XOffset;
   uint32_t                                     Sample6YOffset;
   uint32_t                                     Sample5XOffset;
   uint32_t                                     Sample5YOffset;
   uint32_t                                     Sample4XOffset;
   uint32_t                                     Sample4YOffset;
};

static inline void
GEN7_3DSTATE_MULTISAMPLE_pack(__gen_user_data *data, void * restrict dst,
                             const struct GEN7_3DSTATE_MULTISAMPLE * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->PixelLocation, 4, 4) |
      __gen_field(values->NumberofMultisamples, 1, 3) |
      0;

   dw[2] =
      __gen_field(values->Sample3XOffset, 28, 31) |
      __gen_field(values->Sample3YOffset, 24, 27) |
      __gen_field(values->Sample2XOffset, 20, 23) |
      __gen_field(values->Sample2YOffset, 16, 19) |
      __gen_field(values->Sample1XOffset, 12, 15) |
      __gen_field(values->Sample1YOffset, 8, 11) |
      __gen_field(values->Sample0XOffset, 4, 7) |
      __gen_field(values->Sample0YOffset, 0, 3) |
      0;

   dw[3] =
      __gen_field(values->Sample7XOffset, 28, 31) |
      __gen_field(values->Sample7YOffset, 24, 27) |
      __gen_field(values->Sample6XOffset, 20, 23) |
      __gen_field(values->Sample6YOffset, 16, 19) |
      __gen_field(values->Sample5XOffset, 12, 15) |
      __gen_field(values->Sample5YOffset, 8, 11) |
      __gen_field(values->Sample4XOffset, 4, 7) |
      __gen_field(values->Sample4YOffset, 0, 3) |
      0;

}

#define GEN7_3DSTATE_POLY_STIPPLE_OFFSET_length 0x00000002
#define GEN7_3DSTATE_POLY_STIPPLE_OFFSET_length_bias 0x00000002
#define GEN7_3DSTATE_POLY_STIPPLE_OFFSET_header \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  =  6,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_POLY_STIPPLE_OFFSET {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     PolygonStippleXOffset;
   uint32_t                                     PolygonStippleYOffset;
};

static inline void
GEN7_3DSTATE_POLY_STIPPLE_OFFSET_pack(__gen_user_data *data, void * restrict dst,
                                     const struct GEN7_3DSTATE_POLY_STIPPLE_OFFSET * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->PolygonStippleXOffset, 8, 12) |
      __gen_field(values->PolygonStippleYOffset, 0, 4) |
      0;

}

#define GEN7_3DSTATE_POLY_STIPPLE_PATTERN_length 0x00000021
#define GEN7_3DSTATE_POLY_STIPPLE_PATTERN_length_bias 0x00000002
#define GEN7_3DSTATE_POLY_STIPPLE_PATTERN_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  =  7,                  \
   .DwordLength          = 31

struct GEN7_3DSTATE_POLY_STIPPLE_PATTERN {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     PatternRow;
};

static inline void
GEN7_3DSTATE_POLY_STIPPLE_PATTERN_pack(__gen_user_data *data, void * restrict dst,
                                      const struct GEN7_3DSTATE_POLY_STIPPLE_PATTERN * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->PatternRow, 0, 31) |
      0;

}

#define GEN7_3DSTATE_PS_length 0x00000008
#define GEN7_3DSTATE_PS_length_bias 0x00000002
#define GEN7_3DSTATE_PS_header                  \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 32,                  \
   .DwordLength          =  6

struct GEN7_3DSTATE_PS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     KernelStartPointer0;
#define     Multiple                                           0
#define     Single                                             1
   uint32_t                                     SingleProgramFlowSPF;
#define     Dmask                                              0
#define     Vmask                                              1
   uint32_t                                     VectorMaskEnableVME;
   uint32_t                                     SamplerCount;
#define     FTZ                                                0
#define     RET                                                1
   uint32_t                                     DenormalMode;
   uint32_t                                     BindingTableEntryCount;
#define     IEEE745                                            0
#define     Alt                                                1
   uint32_t                                     FloatingPointMode;
#define     RTNE                                               0
#define     RU                                                 1
#define     RD                                                 2
#define     RTZ                                                3
   uint32_t                                     RoundingMode;
   uint32_t                                     IllegalOpcodeExceptionEnable;
   uint32_t                                     MaskStackExceptionEnable;
   uint32_t                                     SoftwareExceptionEnable;
   uint32_t                                     ScratchSpaceBasePointer;
   uint32_t                                     PerThreadScratchSpace;
   uint32_t                                     MaximumNumberofThreads;
   uint32_t                                     PushConstantEnable;
   uint32_t                                     AttributeEnable;
   uint32_t                                     oMaskPresenttoRenderTarget;
   uint32_t                                     RenderTargetFastClearEnable;
   uint32_t                                     DualSourceBlendEnable;
   uint32_t                                     RenderTargetResolveEnable;
#define     POSOFFSET_NONE                                     0
#define     POSOFFSET_CENTROID                                 2
#define     POSOFFSET_SAMPLE                                   3
   uint32_t                                     PositionXYOffsetSelect;
   uint32_t                                     _32PixelDispatchEnable;
   uint32_t                                     _16PixelDispatchEnable;
   uint32_t                                     _8PixelDispatchEnable;
   uint32_t                                     DispatchGRFStartRegisterforConstantSetupData0;
   uint32_t                                     DispatchGRFStartRegisterforConstantSetupData1;
   uint32_t                                     DispatchGRFStartRegisterforConstantSetupData2;
   uint32_t                                     KernelStartPointer1;
   uint32_t                                     KernelStartPointer2;
};

static inline void
GEN7_3DSTATE_PS_pack(__gen_user_data *data, void * restrict dst,
                    const struct GEN7_3DSTATE_PS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->KernelStartPointer0, 6, 31) |
      0;

   dw[2] =
      __gen_field(values->SingleProgramFlowSPF, 31, 31) |
      __gen_field(values->VectorMaskEnableVME, 30, 30) |
      __gen_field(values->SamplerCount, 27, 29) |
      __gen_field(values->DenormalMode, 26, 26) |
      __gen_field(values->BindingTableEntryCount, 18, 25) |
      __gen_field(values->FloatingPointMode, 16, 16) |
      __gen_field(values->RoundingMode, 14, 15) |
      __gen_field(values->IllegalOpcodeExceptionEnable, 13, 13) |
      __gen_field(values->MaskStackExceptionEnable, 11, 11) |
      __gen_field(values->SoftwareExceptionEnable, 7, 7) |
      0;

   dw[3] =
      __gen_offset(values->ScratchSpaceBasePointer, 10, 31) |
      __gen_field(values->PerThreadScratchSpace, 0, 3) |
      0;

   dw[4] =
      __gen_field(values->MaximumNumberofThreads, 24, 31) |
      __gen_field(values->PushConstantEnable, 11, 11) |
      __gen_field(values->AttributeEnable, 10, 10) |
      __gen_field(values->oMaskPresenttoRenderTarget, 9, 9) |
      __gen_field(values->RenderTargetFastClearEnable, 8, 8) |
      __gen_field(values->DualSourceBlendEnable, 7, 7) |
      __gen_field(values->RenderTargetResolveEnable, 6, 6) |
      __gen_field(values->PositionXYOffsetSelect, 3, 4) |
      __gen_field(values->_32PixelDispatchEnable, 2, 2) |
      __gen_field(values->_16PixelDispatchEnable, 1, 1) |
      __gen_field(values->_8PixelDispatchEnable, 0, 0) |
      0;

   dw[5] =
      __gen_field(values->DispatchGRFStartRegisterforConstantSetupData0, 16, 22) |
      __gen_field(values->DispatchGRFStartRegisterforConstantSetupData1, 8, 14) |
      __gen_field(values->DispatchGRFStartRegisterforConstantSetupData2, 0, 6) |
      0;

   dw[6] =
      __gen_offset(values->KernelStartPointer1, 6, 31) |
      0;

   dw[7] =
      __gen_offset(values->KernelStartPointer2, 6, 31) |
      0;

}

#define GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_DS_length 0x00000002
#define GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_DS_length_bias 0x00000002
#define GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_DS_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  = 20,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_DS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
#define     _0KB                                               0
   uint32_t                                     ConstantBufferOffset;
#define     _0KB                                               0
   uint32_t                                     ConstantBufferSize;
};

static inline void
GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_DS_pack(__gen_user_data *data, void * restrict dst,
                                        const struct GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_DS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->ConstantBufferOffset, 16, 19) |
      __gen_field(values->ConstantBufferSize, 0, 4) |
      0;

}

#define GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_GS_length 0x00000002
#define GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_GS_length_bias 0x00000002
#define GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_GS_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  = 21,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_GS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
#define     _0KB                                               0
   uint32_t                                     ConstantBufferOffset;
#define     _0KB                                               0
   uint32_t                                     ConstantBufferSize;
};

static inline void
GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_GS_pack(__gen_user_data *data, void * restrict dst,
                                        const struct GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_GS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->ConstantBufferOffset, 16, 19) |
      __gen_field(values->ConstantBufferSize, 0, 4) |
      0;

}

#define GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_HS_length 0x00000002
#define GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_HS_length_bias 0x00000002
#define GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_HS_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  = 19,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_HS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
#define     _0KB                                               0
   uint32_t                                     ConstantBufferOffset;
#define     _0KB                                               0
   uint32_t                                     ConstantBufferSize;
};

static inline void
GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_HS_pack(__gen_user_data *data, void * restrict dst,
                                        const struct GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_HS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->ConstantBufferOffset, 16, 19) |
      __gen_field(values->ConstantBufferSize, 0, 4) |
      0;

}

#define GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_PS_length 0x00000002
#define GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_PS_length_bias 0x00000002
#define GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_PS_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  = 22,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_PS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
#define     _0KB                                               0
   uint32_t                                     ConstantBufferOffset;
#define     _0KB                                               0
   uint32_t                                     ConstantBufferSize;
};

static inline void
GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_PS_pack(__gen_user_data *data, void * restrict dst,
                                        const struct GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_PS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->ConstantBufferOffset, 16, 19) |
      __gen_field(values->ConstantBufferSize, 0, 4) |
      0;

}

#define GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_VS_length 0x00000002
#define GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_VS_length_bias 0x00000002
#define GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_VS_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  = 18,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_VS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
#define     _0KB                                               0
   uint32_t                                     ConstantBufferOffset;
#define     _0KB                                               0
   uint32_t                                     ConstantBufferSize;
};

static inline void
GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_VS_pack(__gen_user_data *data, void * restrict dst,
                                        const struct GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_VS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->ConstantBufferOffset, 16, 19) |
      __gen_field(values->ConstantBufferSize, 0, 4) |
      0;

}

#define GEN7_3DSTATE_SAMPLER_PALETTE_LOAD0_length_bias 0x00000002
#define GEN7_3DSTATE_SAMPLER_PALETTE_LOAD0_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  =  2

struct GEN7_PALETTE_ENTRY {
   uint32_t                                     Alpha;
   uint32_t                                     Red;
   uint32_t                                     Green;
   uint32_t                                     Blue;
};

static inline void
GEN7_PALETTE_ENTRY_pack(__gen_user_data *data, void * restrict dst,
                       const struct GEN7_PALETTE_ENTRY * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->Alpha, 24, 31) |
      __gen_field(values->Red, 16, 23) |
      __gen_field(values->Green, 8, 15) |
      __gen_field(values->Blue, 0, 7) |
      0;

}

struct GEN7_3DSTATE_SAMPLER_PALETTE_LOAD0 {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   /* variable length fields follow */
};

static inline void
GEN7_3DSTATE_SAMPLER_PALETTE_LOAD0_pack(__gen_user_data *data, void * restrict dst,
                                       const struct GEN7_3DSTATE_SAMPLER_PALETTE_LOAD0 * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   /* variable length fields follow */
}

#define GEN7_3DSTATE_SAMPLER_PALETTE_LOAD1_length_bias 0x00000002
#define GEN7_3DSTATE_SAMPLER_PALETTE_LOAD1_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  = 12

struct GEN7_3DSTATE_SAMPLER_PALETTE_LOAD1 {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   /* variable length fields follow */
};

static inline void
GEN7_3DSTATE_SAMPLER_PALETTE_LOAD1_pack(__gen_user_data *data, void * restrict dst,
                                       const struct GEN7_3DSTATE_SAMPLER_PALETTE_LOAD1 * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   /* variable length fields follow */
}

#define GEN7_3DSTATE_SAMPLER_STATE_POINTERS_DS_length 0x00000002
#define GEN7_3DSTATE_SAMPLER_STATE_POINTERS_DS_length_bias 0x00000002
#define GEN7_3DSTATE_SAMPLER_STATE_POINTERS_DS_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 45,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_SAMPLER_STATE_POINTERS_DS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     PointertoDSSamplerState;
};

static inline void
GEN7_3DSTATE_SAMPLER_STATE_POINTERS_DS_pack(__gen_user_data *data, void * restrict dst,
                                           const struct GEN7_3DSTATE_SAMPLER_STATE_POINTERS_DS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->PointertoDSSamplerState, 5, 31) |
      0;

}

#define GEN7_3DSTATE_SAMPLER_STATE_POINTERS_GS_length 0x00000002
#define GEN7_3DSTATE_SAMPLER_STATE_POINTERS_GS_length_bias 0x00000002
#define GEN7_3DSTATE_SAMPLER_STATE_POINTERS_GS_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 46,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_SAMPLER_STATE_POINTERS_GS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     PointertoGSSamplerState;
};

static inline void
GEN7_3DSTATE_SAMPLER_STATE_POINTERS_GS_pack(__gen_user_data *data, void * restrict dst,
                                           const struct GEN7_3DSTATE_SAMPLER_STATE_POINTERS_GS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->PointertoGSSamplerState, 5, 31) |
      0;

}

#define GEN7_3DSTATE_SAMPLER_STATE_POINTERS_HS_length 0x00000002
#define GEN7_3DSTATE_SAMPLER_STATE_POINTERS_HS_length_bias 0x00000002
#define GEN7_3DSTATE_SAMPLER_STATE_POINTERS_HS_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 44,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_SAMPLER_STATE_POINTERS_HS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     PointertoHSSamplerState;
};

static inline void
GEN7_3DSTATE_SAMPLER_STATE_POINTERS_HS_pack(__gen_user_data *data, void * restrict dst,
                                           const struct GEN7_3DSTATE_SAMPLER_STATE_POINTERS_HS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->PointertoHSSamplerState, 5, 31) |
      0;

}

#define GEN7_3DSTATE_SAMPLER_STATE_POINTERS_PS_length 0x00000002
#define GEN7_3DSTATE_SAMPLER_STATE_POINTERS_PS_length_bias 0x00000002
#define GEN7_3DSTATE_SAMPLER_STATE_POINTERS_PS_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 47,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_SAMPLER_STATE_POINTERS_PS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     PointertoPSSamplerState;
};

static inline void
GEN7_3DSTATE_SAMPLER_STATE_POINTERS_PS_pack(__gen_user_data *data, void * restrict dst,
                                           const struct GEN7_3DSTATE_SAMPLER_STATE_POINTERS_PS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->PointertoPSSamplerState, 5, 31) |
      0;

}

#define GEN7_3DSTATE_SAMPLER_STATE_POINTERS_VS_length 0x00000002
#define GEN7_3DSTATE_SAMPLER_STATE_POINTERS_VS_length_bias 0x00000002
#define GEN7_3DSTATE_SAMPLER_STATE_POINTERS_VS_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 43,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_SAMPLER_STATE_POINTERS_VS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     PointertoVSSamplerState;
};

static inline void
GEN7_3DSTATE_SAMPLER_STATE_POINTERS_VS_pack(__gen_user_data *data, void * restrict dst,
                                           const struct GEN7_3DSTATE_SAMPLER_STATE_POINTERS_VS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->PointertoVSSamplerState, 5, 31) |
      0;

}

#define GEN7_3DSTATE_SAMPLE_MASK_length 0x00000002
#define GEN7_3DSTATE_SAMPLE_MASK_length_bias 0x00000002
#define GEN7_3DSTATE_SAMPLE_MASK_header         \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 24,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_SAMPLE_MASK {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     SampleMask;
};

static inline void
GEN7_3DSTATE_SAMPLE_MASK_pack(__gen_user_data *data, void * restrict dst,
                             const struct GEN7_3DSTATE_SAMPLE_MASK * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->SampleMask, 0, 7) |
      0;

}

#define GEN7_3DSTATE_SBE_length 0x0000000e
#define GEN7_3DSTATE_SBE_length_bias 0x00000002
#define GEN7_3DSTATE_SBE_header                 \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 31,                  \
   .DwordLength          = 12

struct GEN7_3DSTATE_SBE {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
#define     SWIZ_0_15                                          0
#define     SWIZ_16_31                                         1
   uint32_t                                     AttributeSwizzleControlMode;
   uint32_t                                     NumberofSFOutputAttributes;
   uint32_t                                     AttributeSwizzleEnable;
#define     UPPERLEFT                                          0
#define     LOWERLEFT                                          1
   uint32_t                                     PointSpriteTextureCoordinateOrigin;
   uint32_t                                     VertexURBEntryReadLength;
   uint32_t                                     VertexURBEntryReadOffset;
   uint32_t                                     Attribute2n1ComponentOverrideW;
   uint32_t                                     Attribute2n1ComponentOverrideZ;
   uint32_t                                     Attribute2n1ComponentOverrideY;
   uint32_t                                     Attribute2n1ComponentOverrideX;
#define     CONST_0000                                         0
#define     CONST_0001_FLOAT                                   1
#define     CONST_1111_FLOAT                                   2
#define     PRIM_ID                                            3
   uint32_t                                     Attribute2n1ConstantSource;
#define     INPUTATTR                                          0
#define     INPUTATTR_FACING                                   1
#define     INPUTATTR_W                                        2
#define     INPUTATTR_FACING_W                                 3
   uint32_t                                     Attribute2n1SwizzleSelect;
   uint32_t                                     Attribute2n1SourceAttribute;
   uint32_t                                     Attribute2nComponentOverrideW;
   uint32_t                                     Attribute2nComponentOverrideZ;
   uint32_t                                     Attribute2nComponentOverrideY;
   uint32_t                                     Attribute2nComponentOverrideX;
#define     CONST_0000                                         0
#define     CONST_0001_FLOAT                                   1
#define     CONST_1111_FLOAT                                   2
#define     PRIM_ID                                            3
   uint32_t                                     Attribute2nConstantSource;
#define     INPUTATTR                                          0
#define     INPUTATTR_FACING                                   1
#define     INPUTATTR_W                                        2
#define     INPUTATTR_FACING_W                                 3
   uint32_t                                     Attribute2nSwizzleSelect;
   uint32_t                                     Attribute2nSourceAttribute;
   uint32_t                                     PointSpriteTextureCoordinateEnable;
   uint32_t                                     ConstantInterpolationEnable310;
   uint32_t                                     Attribute7WrapShortestEnables;
   uint32_t                                     Attribute6WrapShortestEnables;
   uint32_t                                     Attribute5WrapShortestEnables;
   uint32_t                                     Attribute4WrapShortestEnables;
   uint32_t                                     Attribute3WrapShortestEnables;
   uint32_t                                     Attribute2WrapShortestEnables;
   uint32_t                                     Attribute1WrapShortestEnables;
   uint32_t                                     Attribute0WrapShortestEnables;
   uint32_t                                     Attribute15WrapShortestEnables;
   uint32_t                                     Attribute14WrapShortestEnables;
   uint32_t                                     Attribute13WrapShortestEnables;
   uint32_t                                     Attribute12WrapShortestEnables;
   uint32_t                                     Attribute11WrapShortestEnables;
   uint32_t                                     Attribute10WrapShortestEnables;
   uint32_t                                     Attribute9WrapShortestEnables;
   uint32_t                                     Attribute8WrapShortestEnables;
};

static inline void
GEN7_3DSTATE_SBE_pack(__gen_user_data *data, void * restrict dst,
                     const struct GEN7_3DSTATE_SBE * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->AttributeSwizzleControlMode, 28, 28) |
      __gen_field(values->NumberofSFOutputAttributes, 22, 27) |
      __gen_field(values->AttributeSwizzleEnable, 21, 21) |
      __gen_field(values->PointSpriteTextureCoordinateOrigin, 20, 20) |
      __gen_field(values->VertexURBEntryReadLength, 11, 15) |
      __gen_field(values->VertexURBEntryReadOffset, 4, 9) |
      0;

   dw[2] =
      __gen_field(values->Attribute2n1ComponentOverrideW, 31, 31) |
      __gen_field(values->Attribute2n1ComponentOverrideZ, 30, 30) |
      __gen_field(values->Attribute2n1ComponentOverrideY, 29, 29) |
      __gen_field(values->Attribute2n1ComponentOverrideX, 28, 28) |
      __gen_field(values->Attribute2n1ConstantSource, 25, 26) |
      __gen_field(values->Attribute2n1SwizzleSelect, 22, 23) |
      __gen_field(values->Attribute2n1SourceAttribute, 16, 20) |
      __gen_field(values->Attribute2nComponentOverrideW, 15, 15) |
      __gen_field(values->Attribute2nComponentOverrideZ, 14, 14) |
      __gen_field(values->Attribute2nComponentOverrideY, 13, 13) |
      __gen_field(values->Attribute2nComponentOverrideX, 12, 12) |
      __gen_field(values->Attribute2nConstantSource, 9, 10) |
      __gen_field(values->Attribute2nSwizzleSelect, 6, 7) |
      __gen_field(values->Attribute2nSourceAttribute, 0, 4) |
      0;

   dw[10] =
      __gen_field(values->PointSpriteTextureCoordinateEnable, 0, 31) |
      0;

   dw[11] =
      __gen_field(values->ConstantInterpolationEnable310, 0, 31) |
      0;

   dw[12] =
      __gen_field(values->Attribute7WrapShortestEnables, 28, 31) |
      __gen_field(values->Attribute6WrapShortestEnables, 24, 27) |
      __gen_field(values->Attribute5WrapShortestEnables, 20, 23) |
      __gen_field(values->Attribute4WrapShortestEnables, 16, 19) |
      __gen_field(values->Attribute3WrapShortestEnables, 12, 15) |
      __gen_field(values->Attribute2WrapShortestEnables, 8, 11) |
      __gen_field(values->Attribute1WrapShortestEnables, 4, 7) |
      __gen_field(values->Attribute0WrapShortestEnables, 0, 3) |
      0;

   dw[13] =
      __gen_field(values->Attribute15WrapShortestEnables, 28, 31) |
      __gen_field(values->Attribute14WrapShortestEnables, 24, 27) |
      __gen_field(values->Attribute13WrapShortestEnables, 20, 23) |
      __gen_field(values->Attribute12WrapShortestEnables, 16, 19) |
      __gen_field(values->Attribute11WrapShortestEnables, 12, 15) |
      __gen_field(values->Attribute10WrapShortestEnables, 8, 11) |
      __gen_field(values->Attribute9WrapShortestEnables, 4, 7) |
      __gen_field(values->Attribute8WrapShortestEnables, 0, 3) |
      0;

}

#define GEN7_3DSTATE_SCISSOR_STATE_POINTERS_length 0x00000002
#define GEN7_3DSTATE_SCISSOR_STATE_POINTERS_length_bias 0x00000002
#define GEN7_3DSTATE_SCISSOR_STATE_POINTERS_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 15,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_SCISSOR_STATE_POINTERS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     ScissorRectPointer;
};

static inline void
GEN7_3DSTATE_SCISSOR_STATE_POINTERS_pack(__gen_user_data *data, void * restrict dst,
                                        const struct GEN7_3DSTATE_SCISSOR_STATE_POINTERS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->ScissorRectPointer, 5, 31) |
      0;

}

#define GEN7_3DSTATE_SF_length 0x00000007
#define GEN7_3DSTATE_SF_length_bias 0x00000002
#define GEN7_3DSTATE_SF_header                  \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 19,                  \
   .DwordLength          =  5

struct GEN7_3DSTATE_SF {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
#define     D32_FLOAT_S8X24_UINT                               0
#define     D32_FLOAT                                          1
#define     D24_UNORM_S8_UINT                                  2
#define     D24_UNORM_X8_UINT                                  3
#define     D16_UNORM                                          5
   uint32_t                                     DepthBufferSurfaceFormat;
   uint32_t                                     LegacyGlobalDepthBiasEnable;
   uint32_t                                     StatisticsEnable;
   uint32_t                                     GlobalDepthOffsetEnableSolid;
   uint32_t                                     GlobalDepthOffsetEnableWireframe;
   uint32_t                                     GlobalDepthOffsetEnablePoint;
#define     RASTER_SOLID                                       0
#define     RASTER_WIREFRAME                                   1
#define     RASTER_POINT                                       2
   uint32_t                                     FrontFaceFillMode;
#define     RASTER_SOLID                                       0
#define     RASTER_WIREFRAME                                   1
#define     RASTER_POINT                                       2
   uint32_t                                     BackFaceFillMode;
   uint32_t                                     ViewTransformEnable;
   uint32_t                                     FrontWinding;
   uint32_t                                     AntiAliasingEnable;
#define     CULLMODE_BOTH                                      0
#define     CULLMODE_NONE                                      1
#define     CULLMODE_FRONT                                     2
#define     CULLMODE_BACK                                      3
   uint32_t                                     CullMode;
   uint32_t                                     LineWidth;
   uint32_t                                     LineEndCapAntialiasingRegionWidth;
   uint32_t                                     ScissorRectangleEnable;
   uint32_t                                     MultisampleRasterizationMode;
   uint32_t                                     LastPixelEnable;
#define     Vertex0                                            0
#define     Vertex1                                            1
#define     Vertex2                                            2
   uint32_t                                     TriangleStripListProvokingVertexSelect;
   uint32_t                                     LineStripListProvokingVertexSelect;
#define     Vertex0                                            0
#define     Vertex1                                            1
#define     Vertex2                                            2
   uint32_t                                     TriangleFanProvokingVertexSelect;
#define     AALINEDISTANCE_TRUE                                1
   uint32_t                                     AALineDistanceMode;
   uint32_t                                     VertexSubPixelPrecisionSelect;
   uint32_t                                     UsePointWidthState;
   uint32_t                                     PointWidth;
   uint32_t                                     GlobalDepthOffsetConstant;
   uint32_t                                     GlobalDepthOffsetScale;
   uint32_t                                     GlobalDepthOffsetClamp;
};

static inline void
GEN7_3DSTATE_SF_pack(__gen_user_data *data, void * restrict dst,
                    const struct GEN7_3DSTATE_SF * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->DepthBufferSurfaceFormat, 12, 14) |
      __gen_field(values->LegacyGlobalDepthBiasEnable, 11, 11) |
      __gen_field(values->StatisticsEnable, 10, 10) |
      __gen_field(values->GlobalDepthOffsetEnableSolid, 9, 9) |
      __gen_field(values->GlobalDepthOffsetEnableWireframe, 8, 8) |
      __gen_field(values->GlobalDepthOffsetEnablePoint, 7, 7) |
      __gen_field(values->FrontFaceFillMode, 5, 6) |
      __gen_field(values->BackFaceFillMode, 3, 4) |
      __gen_field(values->ViewTransformEnable, 1, 1) |
      __gen_field(values->FrontWinding, 0, 0) |
      0;

   dw[2] =
      __gen_field(values->AntiAliasingEnable, 31, 31) |
      __gen_field(values->CullMode, 29, 30) |
      __gen_field(values->LineWidth, 18, 27) |
      __gen_field(values->LineEndCapAntialiasingRegionWidth, 16, 17) |
      __gen_field(values->ScissorRectangleEnable, 11, 11) |
      __gen_field(values->MultisampleRasterizationMode, 8, 9) |
      0;

   dw[3] =
      __gen_field(values->LastPixelEnable, 31, 31) |
      __gen_field(values->TriangleStripListProvokingVertexSelect, 29, 30) |
      __gen_field(values->LineStripListProvokingVertexSelect, 27, 28) |
      __gen_field(values->TriangleFanProvokingVertexSelect, 25, 26) |
      __gen_field(values->AALineDistanceMode, 14, 14) |
      __gen_field(values->VertexSubPixelPrecisionSelect, 12, 12) |
      __gen_field(values->UsePointWidthState, 11, 11) |
      __gen_field(values->PointWidth, 0, 10) |
      0;

   dw[4] =
      __gen_field(values->GlobalDepthOffsetConstant, 0, 31) |
      0;

   dw[5] =
      __gen_field(values->GlobalDepthOffsetScale, 0, 31) |
      0;

   dw[6] =
      __gen_field(values->GlobalDepthOffsetClamp, 0, 31) |
      0;

}

#define GEN7_3DSTATE_SO_BUFFER_length 0x00000004
#define GEN7_3DSTATE_SO_BUFFER_length_bias 0x00000002
#define GEN7_3DSTATE_SO_BUFFER_header           \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  = 24,                  \
   .DwordLength          =  2

struct GEN7_3DSTATE_SO_BUFFER {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     SOBufferIndex;
   struct GEN7_MEMORY_OBJECT_CONTROL_STATE      SOBufferObjectControlState;
   uint32_t                                     SurfacePitch;
   __gen_address_type                           SurfaceBaseAddress;
   __gen_address_type                           SurfaceEndAddress;
};

static inline void
GEN7_3DSTATE_SO_BUFFER_pack(__gen_user_data *data, void * restrict dst,
                           const struct GEN7_3DSTATE_SO_BUFFER * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   uint32_t dw_SOBufferObjectControlState;
   GEN7_MEMORY_OBJECT_CONTROL_STATE_pack(data, &dw_SOBufferObjectControlState, &values->SOBufferObjectControlState);
   dw[1] =
      __gen_field(values->SOBufferIndex, 29, 30) |
      /* Struct  SOBufferObjectControlState (25..28): */
      __gen_field(dw_SOBufferObjectControlState, 25, 28) |
      __gen_field(values->SurfacePitch, 0, 11) |
      0;

   uint32_t dw2 =
      0;

   dw[2] =
      __gen_combine_address(data, &dw[2], values->SurfaceBaseAddress, dw2);

   uint32_t dw3 =
      0;

   dw[3] =
      __gen_combine_address(data, &dw[3], values->SurfaceEndAddress, dw3);

}

#define GEN7_3DSTATE_SO_DECL_LIST_length_bias 0x00000002
#define GEN7_3DSTATE_SO_DECL_LIST_header        \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  1,                  \
   ._3DCommandSubOpcode  = 23

struct GEN7_SO_DECL {
   uint32_t                                     OutputBufferSlot;
   uint32_t                                     HoleFlag;
   uint32_t                                     RegisterIndex;
   uint32_t                                     ComponentMask;
};

static inline void
GEN7_SO_DECL_pack(__gen_user_data *data, void * restrict dst,
                 const struct GEN7_SO_DECL * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->OutputBufferSlot, 12, 13) |
      __gen_field(values->HoleFlag, 11, 11) |
      __gen_field(values->RegisterIndex, 4, 9) |
      __gen_field(values->ComponentMask, 0, 3) |
      0;

}

struct GEN7_SO_DECL_ENTRY {
   struct GEN7_SO_DECL                          Stream3Decl;
   struct GEN7_SO_DECL                          Stream2Decl;
   struct GEN7_SO_DECL                          Stream1Decl;
   struct GEN7_SO_DECL                          Stream0Decl;
};

static inline void
GEN7_SO_DECL_ENTRY_pack(__gen_user_data *data, void * restrict dst,
                       const struct GEN7_SO_DECL_ENTRY * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   uint32_t dw_Stream3Decl;
   GEN7_SO_DECL_pack(data, &dw_Stream3Decl, &values->Stream3Decl);
   uint32_t dw_Stream2Decl;
   GEN7_SO_DECL_pack(data, &dw_Stream2Decl, &values->Stream2Decl);
   uint32_t dw_Stream1Decl;
   GEN7_SO_DECL_pack(data, &dw_Stream1Decl, &values->Stream1Decl);
   uint32_t dw_Stream0Decl;
   GEN7_SO_DECL_pack(data, &dw_Stream0Decl, &values->Stream0Decl);
   dw[0] =
      /* Struct  Stream3Decl (48..63): */
      __gen_field(dw_Stream3Decl, 48, 63) |
      /* Struct  Stream2Decl (32..47): */
      __gen_field(dw_Stream2Decl, 32, 47) |
      /* Struct  Stream1Decl (16..31): */
      __gen_field(dw_Stream1Decl, 16, 31) |
      /* Struct  Stream0Decl (0..15): */
      __gen_field(dw_Stream0Decl, 0, 15) |
      0;

   GEN7_SO_DECL_pack(data, &dw[0], &values->Stream0Decl);
}

struct GEN7_3DSTATE_SO_DECL_LIST {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     StreamtoBufferSelects3;
   uint32_t                                     StreamtoBufferSelects2;
   uint32_t                                     StreamtoBufferSelects1;
   uint32_t                                     StreamtoBufferSelects0;
   uint32_t                                     NumEntries3;
   uint32_t                                     NumEntries2;
   uint32_t                                     NumEntries1;
   uint32_t                                     NumEntries0;
   /* variable length fields follow */
};

static inline void
GEN7_3DSTATE_SO_DECL_LIST_pack(__gen_user_data *data, void * restrict dst,
                              const struct GEN7_3DSTATE_SO_DECL_LIST * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 8) |
      0;

   dw[1] =
      __gen_field(values->StreamtoBufferSelects3, 12, 15) |
      __gen_field(values->StreamtoBufferSelects2, 8, 11) |
      __gen_field(values->StreamtoBufferSelects1, 4, 7) |
      __gen_field(values->StreamtoBufferSelects0, 0, 3) |
      0;

   dw[2] =
      __gen_field(values->NumEntries3, 24, 31) |
      __gen_field(values->NumEntries2, 16, 23) |
      __gen_field(values->NumEntries1, 8, 15) |
      __gen_field(values->NumEntries0, 0, 7) |
      0;

   /* variable length fields follow */
}

#define GEN7_3DSTATE_STENCIL_BUFFER_length 0x00000003
#define GEN7_3DSTATE_STENCIL_BUFFER_length_bias 0x00000002
#define GEN7_3DSTATE_STENCIL_BUFFER_header      \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  =  6,                  \
   .DwordLength          =  1

struct GEN7_3DSTATE_STENCIL_BUFFER {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   struct GEN7_MEMORY_OBJECT_CONTROL_STATE      StencilBufferObjectControlState;
   uint32_t                                     SurfacePitch;
   __gen_address_type                           SurfaceBaseAddress;
};

static inline void
GEN7_3DSTATE_STENCIL_BUFFER_pack(__gen_user_data *data, void * restrict dst,
                                const struct GEN7_3DSTATE_STENCIL_BUFFER * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   uint32_t dw_StencilBufferObjectControlState;
   GEN7_MEMORY_OBJECT_CONTROL_STATE_pack(data, &dw_StencilBufferObjectControlState, &values->StencilBufferObjectControlState);
   dw[1] =
      /* Struct  StencilBufferObjectControlState (25..28): */
      __gen_field(dw_StencilBufferObjectControlState, 25, 28) |
      __gen_field(values->SurfacePitch, 0, 16) |
      0;

   uint32_t dw2 =
      0;

   dw[2] =
      __gen_combine_address(data, &dw[2], values->SurfaceBaseAddress, dw2);

}

#define GEN7_3DSTATE_STREAMOUT_length 0x00000003
#define GEN7_3DSTATE_STREAMOUT_length_bias 0x00000002
#define GEN7_3DSTATE_STREAMOUT_header           \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 30,                  \
   .DwordLength          =  1

struct GEN7_3DSTATE_STREAMOUT {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     SOFunctionEnable;
   uint32_t                                     RenderingDisable;
   uint32_t                                     RenderStreamSelect;
#define     LEADING                                            0
#define     TRAILING                                           1
   uint32_t                                     ReorderMode;
   uint32_t                                     SOStatisticsEnable;
   uint32_t                                     SOBufferEnable3;
   uint32_t                                     SOBufferEnable2;
   uint32_t                                     SOBufferEnable1;
   uint32_t                                     SOBufferEnable0;
   uint32_t                                     Stream3VertexReadOffset;
   uint32_t                                     Stream3VertexReadLength;
   uint32_t                                     Stream2VertexReadOffset;
   uint32_t                                     Stream2VertexReadLength;
   uint32_t                                     Stream1VertexReadOffset;
   uint32_t                                     Stream1VertexReadLength;
   uint32_t                                     Stream0VertexReadOffset;
   uint32_t                                     Stream0VertexReadLength;
};

static inline void
GEN7_3DSTATE_STREAMOUT_pack(__gen_user_data *data, void * restrict dst,
                           const struct GEN7_3DSTATE_STREAMOUT * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->SOFunctionEnable, 31, 31) |
      __gen_field(values->RenderingDisable, 30, 30) |
      __gen_field(values->RenderStreamSelect, 27, 28) |
      __gen_field(values->ReorderMode, 26, 26) |
      __gen_field(values->SOStatisticsEnable, 25, 25) |
      __gen_field(values->SOBufferEnable3, 11, 11) |
      __gen_field(values->SOBufferEnable2, 10, 10) |
      __gen_field(values->SOBufferEnable1, 9, 9) |
      __gen_field(values->SOBufferEnable0, 8, 8) |
      0;

   dw[2] =
      __gen_field(values->Stream3VertexReadOffset, 29, 29) |
      __gen_field(values->Stream3VertexReadLength, 24, 28) |
      __gen_field(values->Stream2VertexReadOffset, 21, 21) |
      __gen_field(values->Stream2VertexReadLength, 16, 20) |
      __gen_field(values->Stream1VertexReadOffset, 13, 13) |
      __gen_field(values->Stream1VertexReadLength, 8, 12) |
      __gen_field(values->Stream0VertexReadOffset, 5, 5) |
      __gen_field(values->Stream0VertexReadLength, 0, 4) |
      0;

}

#define GEN7_3DSTATE_TE_length 0x00000004
#define GEN7_3DSTATE_TE_length_bias 0x00000002
#define GEN7_3DSTATE_TE_header                  \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 28,                  \
   .DwordLength          =  2

struct GEN7_3DSTATE_TE {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
#define     INTEGER                                            0
#define     ODD_FRACTIONAL                                     1
#define     EVEN_FRACTIONAL                                    2
   uint32_t                                     Partitioning;
#define     POINT                                              0
#define     LINE                                               1
#define     TRI_CW                                             2
#define     TRI_CCW                                            3
   uint32_t                                     OutputTopology;
#define     QUAD                                               0
#define     TRI                                                1
#define     ISOLINE                                            2
   uint32_t                                     TEDomain;
#define     HW_TESS                                            0
#define     SW_TESS                                            1
   uint32_t                                     TEMode;
   uint32_t                                     TEEnable;
   float                                        MaximumTessellationFactorOdd;
   float                                        MaximumTessellationFactorNotOdd;
};

static inline void
GEN7_3DSTATE_TE_pack(__gen_user_data *data, void * restrict dst,
                    const struct GEN7_3DSTATE_TE * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->Partitioning, 12, 13) |
      __gen_field(values->OutputTopology, 8, 9) |
      __gen_field(values->TEDomain, 4, 5) |
      __gen_field(values->TEMode, 1, 2) |
      __gen_field(values->TEEnable, 0, 0) |
      0;

   dw[2] =
      __gen_float(values->MaximumTessellationFactorOdd) |
      0;

   dw[3] =
      __gen_float(values->MaximumTessellationFactorNotOdd) |
      0;

}

#define GEN7_3DSTATE_URB_DS_length 0x00000002
#define GEN7_3DSTATE_URB_DS_length_bias 0x00000002
#define GEN7_3DSTATE_URB_DS_header              \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 50,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_URB_DS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     DSURBStartingAddress;
   uint32_t                                     DSURBEntryAllocationSize;
   uint32_t                                     DSNumberofURBEntries;
};

static inline void
GEN7_3DSTATE_URB_DS_pack(__gen_user_data *data, void * restrict dst,
                        const struct GEN7_3DSTATE_URB_DS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->DSURBStartingAddress, 25, 29) |
      __gen_field(values->DSURBEntryAllocationSize, 16, 24) |
      __gen_field(values->DSNumberofURBEntries, 0, 15) |
      0;

}

#define GEN7_3DSTATE_URB_GS_length 0x00000002
#define GEN7_3DSTATE_URB_GS_length_bias 0x00000002
#define GEN7_3DSTATE_URB_GS_header              \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 51,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_URB_GS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     GSURBStartingAddress;
   uint32_t                                     GSURBEntryAllocationSize;
   uint32_t                                     GSNumberofURBEntries;
};

static inline void
GEN7_3DSTATE_URB_GS_pack(__gen_user_data *data, void * restrict dst,
                        const struct GEN7_3DSTATE_URB_GS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->GSURBStartingAddress, 25, 29) |
      __gen_field(values->GSURBEntryAllocationSize, 16, 24) |
      __gen_field(values->GSNumberofURBEntries, 0, 15) |
      0;

}

#define GEN7_3DSTATE_URB_HS_length 0x00000002
#define GEN7_3DSTATE_URB_HS_length_bias 0x00000002
#define GEN7_3DSTATE_URB_HS_header              \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 49,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_URB_HS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     HSURBStartingAddress;
   uint32_t                                     HSURBEntryAllocationSize;
   uint32_t                                     HSNumberofURBEntries;
};

static inline void
GEN7_3DSTATE_URB_HS_pack(__gen_user_data *data, void * restrict dst,
                        const struct GEN7_3DSTATE_URB_HS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->HSURBStartingAddress, 25, 29) |
      __gen_field(values->HSURBEntryAllocationSize, 16, 24) |
      __gen_field(values->HSNumberofURBEntries, 0, 15) |
      0;

}

#define GEN7_3DSTATE_VERTEX_BUFFERS_length_bias 0x00000002
#define GEN7_3DSTATE_VERTEX_BUFFERS_header      \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  =  8

struct GEN7_VERTEX_BUFFER_STATE {
   uint32_t                                     VertexBufferIndex;
#define     VERTEXDATA                                         0
#define     INSTANCEDATA                                       1
   uint32_t                                     BufferAccessType;
   struct GEN7_MEMORY_OBJECT_CONTROL_STATE      VertexBufferMemoryObjectControlState;
   uint32_t                                     AddressModifyEnable;
   uint32_t                                     NullVertexBuffer;
   uint32_t                                     VertexFetchInvalidate;
   uint32_t                                     BufferPitch;
   __gen_address_type                           BufferStartingAddress;
   __gen_address_type                           EndAddress;
   uint32_t                                     InstanceDataStepRate;
};

static inline void
GEN7_VERTEX_BUFFER_STATE_pack(__gen_user_data *data, void * restrict dst,
                             const struct GEN7_VERTEX_BUFFER_STATE * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   uint32_t dw_VertexBufferMemoryObjectControlState;
   GEN7_MEMORY_OBJECT_CONTROL_STATE_pack(data, &dw_VertexBufferMemoryObjectControlState, &values->VertexBufferMemoryObjectControlState);
   dw[0] =
      __gen_field(values->VertexBufferIndex, 26, 31) |
      __gen_field(values->BufferAccessType, 20, 20) |
      /* Struct  VertexBufferMemoryObjectControlState (16..19): */
      __gen_field(dw_VertexBufferMemoryObjectControlState, 16, 19) |
      __gen_field(values->AddressModifyEnable, 14, 14) |
      __gen_field(values->NullVertexBuffer, 13, 13) |
      __gen_field(values->VertexFetchInvalidate, 12, 12) |
      __gen_field(values->BufferPitch, 0, 11) |
      0;

   uint32_t dw1 =
      0;

   dw[1] =
      __gen_combine_address(data, &dw[1], values->BufferStartingAddress, dw1);

   uint32_t dw2 =
      0;

   dw[2] =
      __gen_combine_address(data, &dw[2], values->EndAddress, dw2);

   dw[3] =
      __gen_field(values->InstanceDataStepRate, 0, 31) |
      0;

}

struct GEN7_3DSTATE_VERTEX_BUFFERS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   /* variable length fields follow */
};

static inline void
GEN7_3DSTATE_VERTEX_BUFFERS_pack(__gen_user_data *data, void * restrict dst,
                                const struct GEN7_3DSTATE_VERTEX_BUFFERS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   /* variable length fields follow */
}

#define GEN7_3DSTATE_VERTEX_ELEMENTS_length_bias 0x00000002
#define GEN7_3DSTATE_VERTEX_ELEMENTS_header     \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  =  9

struct GEN7_VERTEX_ELEMENT_STATE {
   uint32_t                                     VertexBufferIndex;
   uint32_t                                     Valid;
   uint32_t                                     SourceElementFormat;
   uint32_t                                     EdgeFlagEnable;
   uint32_t                                     SourceElementOffset;
   uint32_t                                     Component0Control;
   uint32_t                                     Component1Control;
   uint32_t                                     Component2Control;
   uint32_t                                     Component3Control;
};

static inline void
GEN7_VERTEX_ELEMENT_STATE_pack(__gen_user_data *data, void * restrict dst,
                              const struct GEN7_VERTEX_ELEMENT_STATE * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->VertexBufferIndex, 26, 31) |
      __gen_field(values->Valid, 25, 25) |
      __gen_field(values->SourceElementFormat, 16, 24) |
      __gen_field(values->EdgeFlagEnable, 15, 15) |
      __gen_field(values->SourceElementOffset, 0, 11) |
      0;

   dw[1] =
      __gen_field(values->Component0Control, 28, 30) |
      __gen_field(values->Component1Control, 24, 26) |
      __gen_field(values->Component2Control, 20, 22) |
      __gen_field(values->Component3Control, 16, 18) |
      0;

}

struct GEN7_3DSTATE_VERTEX_ELEMENTS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   /* variable length fields follow */
};

static inline void
GEN7_3DSTATE_VERTEX_ELEMENTS_pack(__gen_user_data *data, void * restrict dst,
                                 const struct GEN7_3DSTATE_VERTEX_ELEMENTS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   /* variable length fields follow */
}

#define GEN7_3DSTATE_VF_STATISTICS_length 0x00000001
#define GEN7_3DSTATE_VF_STATISTICS_length_bias 0x00000001
#define GEN7_3DSTATE_VF_STATISTICS_header       \
   .CommandType          =  3,                  \
   .CommandSubType       =  1,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 11

struct GEN7_3DSTATE_VF_STATISTICS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     StatisticsEnable;
};

static inline void
GEN7_3DSTATE_VF_STATISTICS_pack(__gen_user_data *data, void * restrict dst,
                               const struct GEN7_3DSTATE_VF_STATISTICS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->StatisticsEnable, 0, 0) |
      0;

}

#define GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_CC_length 0x00000002
#define GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_CC_length_bias 0x00000002
#define GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_CC_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 35,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_CC {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     CCViewportPointer;
};

static inline void
GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_CC_pack(__gen_user_data *data, void * restrict dst,
                                            const struct GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_CC * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->CCViewportPointer, 5, 31) |
      0;

}

#define GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP_length 0x00000002
#define GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP_length_bias 0x00000002
#define GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP_header\
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 33,                  \
   .DwordLength          =  0

struct GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     SFClipViewportPointer;
};

static inline void
GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP_pack(__gen_user_data *data, void * restrict dst,
                                                 const struct GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->SFClipViewportPointer, 6, 31) |
      0;

}

#define GEN7_3DSTATE_VS_length 0x00000006
#define GEN7_3DSTATE_VS_length_bias 0x00000002
#define GEN7_3DSTATE_VS_header                  \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 16,                  \
   .DwordLength          =  4

struct GEN7_3DSTATE_VS {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     KernelStartPointer;
#define     Multiple                                           0
#define     Single                                             1
   uint32_t                                     SingleVertexDispatch;
#define     Dmask                                              0
#define     Vmask                                              1
   uint32_t                                     VectorMaskEnableVME;
#define     NoSamplers                                         0
#define     _14Samplers                                        1
#define     _58Samplers                                        2
#define     _912Samplers                                       3
#define     _1316Samplers                                      4
   uint32_t                                     SamplerCount;
   uint32_t                                     BindingTableEntryCount;
#define     IEEE754                                            0
#define     Alternate                                          1
   uint32_t                                     FloatingPointMode;
   uint32_t                                     IllegalOpcodeExceptionEnable;
   uint32_t                                     SoftwareExceptionEnable;
   uint32_t                                     ScratchSpaceBaseOffset;
   uint32_t                                     PerThreadScratchSpace;
   uint32_t                                     DispatchGRFStartRegisterforURBData;
   uint32_t                                     VertexURBEntryReadLength;
   uint32_t                                     VertexURBEntryReadOffset;
   uint32_t                                     MaximumNumberofThreads;
   uint32_t                                     StatisticsEnable;
   uint32_t                                     VertexCacheDisable;
   uint32_t                                     VSFunctionEnable;
};

static inline void
GEN7_3DSTATE_VS_pack(__gen_user_data *data, void * restrict dst,
                    const struct GEN7_3DSTATE_VS * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->KernelStartPointer, 6, 31) |
      0;

   dw[2] =
      __gen_field(values->SingleVertexDispatch, 31, 31) |
      __gen_field(values->VectorMaskEnableVME, 30, 30) |
      __gen_field(values->SamplerCount, 27, 29) |
      __gen_field(values->BindingTableEntryCount, 18, 25) |
      __gen_field(values->FloatingPointMode, 16, 16) |
      __gen_field(values->IllegalOpcodeExceptionEnable, 13, 13) |
      __gen_field(values->SoftwareExceptionEnable, 7, 7) |
      0;

   dw[3] =
      __gen_offset(values->ScratchSpaceBaseOffset, 10, 31) |
      __gen_field(values->PerThreadScratchSpace, 0, 3) |
      0;

   dw[4] =
      __gen_field(values->DispatchGRFStartRegisterforURBData, 20, 24) |
      __gen_field(values->VertexURBEntryReadLength, 11, 16) |
      __gen_field(values->VertexURBEntryReadOffset, 4, 9) |
      0;

   dw[5] =
      __gen_field(values->MaximumNumberofThreads, 25, 31) |
      __gen_field(values->StatisticsEnable, 10, 10) |
      __gen_field(values->VertexCacheDisable, 1, 1) |
      __gen_field(values->VSFunctionEnable, 0, 0) |
      0;

}

#define GEN7_3DSTATE_WM_length 0x00000003
#define GEN7_3DSTATE_WM_length_bias 0x00000002
#define GEN7_3DSTATE_WM_header                  \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  0,                  \
   ._3DCommandSubOpcode  = 20,                  \
   .DwordLength          =  1

struct GEN7_3DSTATE_WM {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     StatisticsEnable;
   uint32_t                                     DepthBufferClear;
   uint32_t                                     ThreadDispatchEnable;
   uint32_t                                     DepthBufferResolveEnable;
   uint32_t                                     HierarchicalDepthBufferResolveEnable;
   uint32_t                                     LegacyDiamondLineRasterization;
   uint32_t                                     PixelShaderKillPixel;
#define     PSCDEPTH_OFF                                       0
#define     PSCDEPTH_ON                                        1
#define     PSCDEPTH_ON_GE                                     2
#define     PSCDEPTH_ON_LE                                     3
   uint32_t                                     PixelShaderComputedDepthMode;
#define     EDSC_NORMAL                                        0
#define     EDSC_PSEXEC                                        1
#define     EDSC_PREPS                                         2
   uint32_t                                     EarlyDepthStencilControl;
   uint32_t                                     PixelShaderUsesSourceDepth;
   uint32_t                                     PixelShaderUsesSourceW;
#define     INTERP_PIXEL                                       0
#define     INTERP_CENTROID                                    2
#define     INTERP_SAMPLE                                      3
   uint32_t                                     PositionZWInterpolationMode;
   uint32_t                                     BarycentricInterpolationMode;
   uint32_t                                     PixelShaderUsesInputCoverageMask;
   uint32_t                                     LineEndCapAntialiasingRegionWidth;
   uint32_t                                     LineAntialiasingRegionWidth;
   uint32_t                                     PolygonStippleEnable;
   uint32_t                                     LineStippleEnable;
#define     RASTRULE_UPPER_LEFT                                0
#define     RASTRULE_UPPER_RIGHT                               1
   uint32_t                                     PointRasterizationRule;
#define     MSRASTMODE_OFF_PIXEL                               0
#define     MSRASTMODE_OFF_PATTERN                             1
#define     MSRASTMODE_ON_PIXEL                                2
#define     MSRASTMODE_ON_PATTERN                              3
   uint32_t                                     MultisampleRasterizationMode;
#define     MSDISPMODE_PERSAMPLE                               0
#define     MSDISPMODE_PERPIXEL                                1
   uint32_t                                     MultisampleDispatchMode;
};

static inline void
GEN7_3DSTATE_WM_pack(__gen_user_data *data, void * restrict dst,
                    const struct GEN7_3DSTATE_WM * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->StatisticsEnable, 31, 31) |
      __gen_field(values->DepthBufferClear, 30, 30) |
      __gen_field(values->ThreadDispatchEnable, 29, 29) |
      __gen_field(values->DepthBufferResolveEnable, 28, 28) |
      __gen_field(values->HierarchicalDepthBufferResolveEnable, 27, 27) |
      __gen_field(values->LegacyDiamondLineRasterization, 26, 26) |
      __gen_field(values->PixelShaderKillPixel, 25, 25) |
      __gen_field(values->PixelShaderComputedDepthMode, 23, 24) |
      __gen_field(values->EarlyDepthStencilControl, 21, 22) |
      __gen_field(values->PixelShaderUsesSourceDepth, 20, 20) |
      __gen_field(values->PixelShaderUsesSourceW, 19, 19) |
      __gen_field(values->PositionZWInterpolationMode, 17, 18) |
      __gen_field(values->BarycentricInterpolationMode, 11, 16) |
      __gen_field(values->PixelShaderUsesInputCoverageMask, 10, 10) |
      __gen_field(values->LineEndCapAntialiasingRegionWidth, 8, 9) |
      __gen_field(values->LineAntialiasingRegionWidth, 6, 7) |
      __gen_field(values->PolygonStippleEnable, 4, 4) |
      __gen_field(values->LineStippleEnable, 3, 3) |
      __gen_field(values->PointRasterizationRule, 2, 2) |
      __gen_field(values->MultisampleRasterizationMode, 0, 1) |
      0;

   dw[2] =
      __gen_field(values->MultisampleDispatchMode, 31, 31) |
      0;

}

#define GEN7_GPGPU_OBJECT_length 0x00000008
#define GEN7_GPGPU_OBJECT_length_bias 0x00000002
#define GEN7_GPGPU_OBJECT_header                \
   .CommandType          =  3,                  \
   .Pipeline             =  2,                  \
   .MediaCommandOpcode   =  1,                  \
   .SubOpcode            =  4,                  \
   .DwordLength          =  6

struct GEN7_GPGPU_OBJECT {
   uint32_t                                     CommandType;
   uint32_t                                     Pipeline;
   uint32_t                                     MediaCommandOpcode;
   uint32_t                                     SubOpcode;
   uint32_t                                     PredicateEnable;
   uint32_t                                     DwordLength;
   uint32_t                                     SharedLocalMemoryFixedOffset;
   uint32_t                                     InterfaceDescriptorOffset;
   uint32_t                                     SharedLocalMemoryOffset;
   uint32_t                                     EndofThreadGroup;
#define     HalfSlice1                                         2
#define     HalfSlice0                                         1
#define     EitherHalfSlice                                    0
   uint32_t                                     HalfSliceDestinationSelect;
   uint32_t                                     IndirectDataLength;
   uint32_t                                     IndirectDataStartAddress;
   uint32_t                                     ThreadGroupIDX;
   uint32_t                                     ThreadGroupIDY;
   uint32_t                                     ThreadGroupIDZ;
   uint32_t                                     ExecutionMask;
};

static inline void
GEN7_GPGPU_OBJECT_pack(__gen_user_data *data, void * restrict dst,
                      const struct GEN7_GPGPU_OBJECT * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->Pipeline, 27, 28) |
      __gen_field(values->MediaCommandOpcode, 24, 26) |
      __gen_field(values->SubOpcode, 16, 23) |
      __gen_field(values->PredicateEnable, 8, 8) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->SharedLocalMemoryFixedOffset, 7, 7) |
      __gen_field(values->InterfaceDescriptorOffset, 0, 4) |
      0;

   dw[2] =
      __gen_field(values->SharedLocalMemoryOffset, 28, 31) |
      __gen_field(values->EndofThreadGroup, 24, 24) |
      __gen_field(values->HalfSliceDestinationSelect, 17, 18) |
      __gen_field(values->IndirectDataLength, 0, 16) |
      0;

   dw[3] =
      __gen_offset(values->IndirectDataStartAddress, 0, 31) |
      0;

   dw[4] =
      __gen_field(values->ThreadGroupIDX, 0, 31) |
      0;

   dw[5] =
      __gen_field(values->ThreadGroupIDY, 0, 31) |
      0;

   dw[6] =
      __gen_field(values->ThreadGroupIDZ, 0, 31) |
      0;

   dw[7] =
      __gen_field(values->ExecutionMask, 0, 31) |
      0;

}

#define GEN7_GPGPU_WALKER_length 0x0000000b
#define GEN7_GPGPU_WALKER_length_bias 0x00000002
#define GEN7_GPGPU_WALKER_header                \
   .CommandType          =  3,                  \
   .Pipeline             =  2,                  \
   .MediaCommandOpcode   =  1,                  \
   .SubOpcodeA           =  5,                  \
   .DwordLength          =  9

struct GEN7_GPGPU_WALKER {
   uint32_t                                     CommandType;
   uint32_t                                     Pipeline;
   uint32_t                                     MediaCommandOpcode;
   uint32_t                                     SubOpcodeA;
   uint32_t                                     IndirectParameterEnable;
   uint32_t                                     PredicateEnable;
   uint32_t                                     DwordLength;
   uint32_t                                     InterfaceDescriptorOffset;
#define     SIMD8                                              0
#define     SIMD16                                             1
#define     SIMD32                                             2
   uint32_t                                     SIMDSize;
   uint32_t                                     ThreadDepthCounterMaximum;
   uint32_t                                     ThreadHeightCounterMaximum;
   uint32_t                                     ThreadWidthCounterMaximum;
   uint32_t                                     ThreadGroupIDStartingX;
   uint32_t                                     ThreadGroupIDXDimension;
   uint32_t                                     ThreadGroupIDStartingY;
   uint32_t                                     ThreadGroupIDYDimension;
   uint32_t                                     ThreadGroupIDStartingZ;
   uint32_t                                     ThreadGroupIDZDimension;
   uint32_t                                     RightExecutionMask;
   uint32_t                                     BottomExecutionMask;
};

static inline void
GEN7_GPGPU_WALKER_pack(__gen_user_data *data, void * restrict dst,
                      const struct GEN7_GPGPU_WALKER * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->Pipeline, 27, 28) |
      __gen_field(values->MediaCommandOpcode, 24, 26) |
      __gen_field(values->SubOpcodeA, 16, 23) |
      __gen_field(values->IndirectParameterEnable, 10, 10) |
      __gen_field(values->PredicateEnable, 8, 8) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->InterfaceDescriptorOffset, 0, 4) |
      0;

   dw[2] =
      __gen_field(values->SIMDSize, 30, 31) |
      __gen_field(values->ThreadDepthCounterMaximum, 16, 21) |
      __gen_field(values->ThreadHeightCounterMaximum, 8, 13) |
      __gen_field(values->ThreadWidthCounterMaximum, 0, 5) |
      0;

   dw[3] =
      __gen_field(values->ThreadGroupIDStartingX, 0, 31) |
      0;

   dw[4] =
      __gen_field(values->ThreadGroupIDXDimension, 0, 31) |
      0;

   dw[5] =
      __gen_field(values->ThreadGroupIDStartingY, 0, 31) |
      0;

   dw[6] =
      __gen_field(values->ThreadGroupIDYDimension, 0, 31) |
      0;

   dw[7] =
      __gen_field(values->ThreadGroupIDStartingZ, 0, 31) |
      0;

   dw[8] =
      __gen_field(values->ThreadGroupIDZDimension, 0, 31) |
      0;

   dw[9] =
      __gen_field(values->RightExecutionMask, 0, 31) |
      0;

   dw[10] =
      __gen_field(values->BottomExecutionMask, 0, 31) |
      0;

}

#define GEN7_MEDIA_CURBE_LOAD_length 0x00000004
#define GEN7_MEDIA_CURBE_LOAD_length_bias 0x00000002
#define GEN7_MEDIA_CURBE_LOAD_header            \
   .CommandType          =  3,                  \
   .Pipeline             =  2,                  \
   .MediaCommandOpcode   =  0,                  \
   .SubOpcode            =  1,                  \
   .DwordLength          =  2

struct GEN7_MEDIA_CURBE_LOAD {
   uint32_t                                     CommandType;
   uint32_t                                     Pipeline;
   uint32_t                                     MediaCommandOpcode;
   uint32_t                                     SubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     CURBETotalDataLength;
   uint32_t                                     CURBEDataStartAddress;
};

static inline void
GEN7_MEDIA_CURBE_LOAD_pack(__gen_user_data *data, void * restrict dst,
                          const struct GEN7_MEDIA_CURBE_LOAD * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->Pipeline, 27, 28) |
      __gen_field(values->MediaCommandOpcode, 24, 26) |
      __gen_field(values->SubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 15) |
      0;

   dw[1] =
      0;

   dw[2] =
      __gen_field(values->CURBETotalDataLength, 0, 16) |
      0;

   dw[3] =
      __gen_field(values->CURBEDataStartAddress, 0, 31) |
      0;

}

#define GEN7_MEDIA_INTERFACE_DESCRIPTOR_LOAD_length 0x00000004
#define GEN7_MEDIA_INTERFACE_DESCRIPTOR_LOAD_length_bias 0x00000002
#define GEN7_MEDIA_INTERFACE_DESCRIPTOR_LOAD_header\
   .CommandType          =  3,                  \
   .Pipeline             =  2,                  \
   .MediaCommandOpcode   =  0,                  \
   .SubOpcode            =  2,                  \
   .DwordLength          =  2

struct GEN7_MEDIA_INTERFACE_DESCRIPTOR_LOAD {
   uint32_t                                     CommandType;
   uint32_t                                     Pipeline;
   uint32_t                                     MediaCommandOpcode;
   uint32_t                                     SubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     InterfaceDescriptorTotalLength;
   uint32_t                                     InterfaceDescriptorDataStartAddress;
};

static inline void
GEN7_MEDIA_INTERFACE_DESCRIPTOR_LOAD_pack(__gen_user_data *data, void * restrict dst,
                                         const struct GEN7_MEDIA_INTERFACE_DESCRIPTOR_LOAD * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->Pipeline, 27, 28) |
      __gen_field(values->MediaCommandOpcode, 24, 26) |
      __gen_field(values->SubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 15) |
      0;

   dw[1] =
      0;

   dw[2] =
      __gen_field(values->InterfaceDescriptorTotalLength, 0, 16) |
      0;

   dw[3] =
      __gen_offset(values->InterfaceDescriptorDataStartAddress, 0, 31) |
      0;

}

#define GEN7_MEDIA_OBJECT_length_bias 0x00000002
#define GEN7_MEDIA_OBJECT_header                \
   .CommandType          =  3,                  \
   .MediaCommandPipeline =  2,                  \
   .MediaCommandOpcode   =  1,                  \
   .MediaCommandSubOpcode =  0

struct GEN7_MEDIA_OBJECT {
   uint32_t                                     CommandType;
   uint32_t                                     MediaCommandPipeline;
   uint32_t                                     MediaCommandOpcode;
   uint32_t                                     MediaCommandSubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     InterfaceDescriptorOffset;
   uint32_t                                     ChildrenPresent;
#define     Nothreadsynchronization                            0
#define     Threaddispatchissynchronizedbythespawnrootthreadmessage       1
   uint32_t                                     ThreadSynchronization;
#define     Notusingscoreboard                                 0
#define     Usingscoreboard                                    1
   uint32_t                                     UseScoreboard;
#define     HalfSlice1                                         2
#define     HalfSlice0                                         1
#define     Eitherhalfslice                                    0
   uint32_t                                     HalfSliceDestinationSelect;
   uint32_t                                     IndirectDataLength;
   __gen_address_type                           IndirectDataStartAddress;
   uint32_t                                     ScoredboardY;
   uint32_t                                     ScoreboardX;
   uint32_t                                     ScoreboardColor;
   uint32_t                                     ScoreboardMask;
   /* variable length fields follow */
};

static inline void
GEN7_MEDIA_OBJECT_pack(__gen_user_data *data, void * restrict dst,
                      const struct GEN7_MEDIA_OBJECT * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MediaCommandPipeline, 27, 28) |
      __gen_field(values->MediaCommandOpcode, 24, 26) |
      __gen_field(values->MediaCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 15) |
      0;

   dw[1] =
      __gen_field(values->InterfaceDescriptorOffset, 0, 4) |
      0;

   dw[2] =
      __gen_field(values->ChildrenPresent, 31, 31) |
      __gen_field(values->ThreadSynchronization, 24, 24) |
      __gen_field(values->UseScoreboard, 21, 21) |
      __gen_field(values->HalfSliceDestinationSelect, 17, 18) |
      __gen_field(values->IndirectDataLength, 0, 16) |
      0;

   uint32_t dw3 =
      0;

   dw[3] =
      __gen_combine_address(data, &dw[3], values->IndirectDataStartAddress, dw3);

   dw[4] =
      __gen_field(values->ScoredboardY, 16, 24) |
      __gen_field(values->ScoreboardX, 0, 8) |
      0;

   dw[5] =
      __gen_field(values->ScoreboardColor, 16, 19) |
      __gen_field(values->ScoreboardMask, 0, 7) |
      0;

   /* variable length fields follow */
}

#define GEN7_MEDIA_OBJECT_PRT_length 0x00000010
#define GEN7_MEDIA_OBJECT_PRT_length_bias 0x00000002
#define GEN7_MEDIA_OBJECT_PRT_header            \
   .CommandType          =  3,                  \
   .Pipeline             =  2,                  \
   .MediaCommandOpcode   =  1,                  \
   .SubOpcode            =  2,                  \
   .DwordLength          = 14

struct GEN7_MEDIA_OBJECT_PRT {
   uint32_t                                     CommandType;
   uint32_t                                     Pipeline;
   uint32_t                                     MediaCommandOpcode;
   uint32_t                                     SubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     InterfaceDescriptorOffset;
   uint32_t                                     ChildrenPresent;
   uint32_t                                     PRT_FenceNeeded;
#define     Rootthreadqueue                                    0
#define     VFEstateflush                                      1
   uint32_t                                     PRT_FenceType;
   uint32_t                                     InlineData;
};

static inline void
GEN7_MEDIA_OBJECT_PRT_pack(__gen_user_data *data, void * restrict dst,
                          const struct GEN7_MEDIA_OBJECT_PRT * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->Pipeline, 27, 28) |
      __gen_field(values->MediaCommandOpcode, 24, 26) |
      __gen_field(values->SubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 15) |
      0;

   dw[1] =
      __gen_field(values->InterfaceDescriptorOffset, 0, 4) |
      0;

   dw[2] =
      __gen_field(values->ChildrenPresent, 31, 31) |
      __gen_field(values->PRT_FenceNeeded, 23, 23) |
      __gen_field(values->PRT_FenceType, 22, 22) |
      0;

   dw[3] =
      0;

   dw[4] =
      __gen_field(values->InlineData, 0, 31) |
      0;

}

#define GEN7_MEDIA_OBJECT_WALKER_length_bias 0x00000002
#define GEN7_MEDIA_OBJECT_WALKER_header         \
   .CommandType          =  3,                  \
   .Pipeline             =  2,                  \
   .MediaCommandOpcode   =  1,                  \
   .SubOpcode            =  3

struct GEN7_MEDIA_OBJECT_WALKER {
   uint32_t                                     CommandType;
   uint32_t                                     Pipeline;
   uint32_t                                     MediaCommandOpcode;
   uint32_t                                     SubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     InterfaceDescriptorOffset;
   uint32_t                                     ChildrenPresent;
#define     Nothreadsynchronization                            0
#define     Threaddispatchissynchronizedbythespawnrootthreadmessage       1
   uint32_t                                     ThreadSynchronization;
#define     Notusingscoreboard                                 0
#define     Usingscoreboard                                    1
   uint32_t                                     UseScoreboard;
   uint32_t                                     IndirectDataLength;
   uint32_t                                     IndirectDataStartAddress;
   uint32_t                                     ScoreboardMask;
   uint32_t                                     DualMode;
   uint32_t                                     Repel;
   uint32_t                                     ColorCountMinusOne;
   uint32_t                                     MiddleLoopExtraSteps;
   uint32_t                                     LocalMidLoopUnitY;
   uint32_t                                     MidLoopUnitX;
   uint32_t                                     GlobalLoopExecCount;
   uint32_t                                     LocalLoopExecCount;
   uint32_t                                     BlockResolutionY;
   uint32_t                                     BlockResolutionX;
   uint32_t                                     LocalStartY;
   uint32_t                                     LocalStartX;
   uint32_t                                     LocalEndY;
   uint32_t                                     LocalEndX;
   uint32_t                                     LocalOuterLoopStrideY;
   uint32_t                                     LocalOuterLoopStrideX;
   uint32_t                                     LocalInnerLoopUnitY;
   uint32_t                                     LocalInnerLoopUnitX;
   uint32_t                                     GlobalResolutionY;
   uint32_t                                     GlobalResolutionX;
   uint32_t                                     GlobalStartY;
   uint32_t                                     GlobalStartX;
   uint32_t                                     GlobalOuterLoopStrideY;
   uint32_t                                     GlobalOuterLoopStrideX;
   uint32_t                                     GlobalInnerLoopUnitY;
   uint32_t                                     GlobalInnerLoopUnitX;
   /* variable length fields follow */
};

static inline void
GEN7_MEDIA_OBJECT_WALKER_pack(__gen_user_data *data, void * restrict dst,
                             const struct GEN7_MEDIA_OBJECT_WALKER * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->Pipeline, 27, 28) |
      __gen_field(values->MediaCommandOpcode, 24, 26) |
      __gen_field(values->SubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 15) |
      0;

   dw[1] =
      __gen_field(values->InterfaceDescriptorOffset, 0, 4) |
      0;

   dw[2] =
      __gen_field(values->ChildrenPresent, 31, 31) |
      __gen_field(values->ThreadSynchronization, 24, 24) |
      __gen_field(values->UseScoreboard, 21, 21) |
      __gen_field(values->IndirectDataLength, 0, 16) |
      0;

   dw[3] =
      __gen_offset(values->IndirectDataStartAddress, 0, 31) |
      0;

   dw[4] =
      0;

   dw[5] =
      __gen_field(values->ScoreboardMask, 0, 7) |
      0;

   dw[6] =
      __gen_field(values->DualMode, 31, 31) |
      __gen_field(values->Repel, 30, 30) |
      __gen_field(values->ColorCountMinusOne, 24, 27) |
      __gen_field(values->MiddleLoopExtraSteps, 16, 20) |
      __gen_field(values->LocalMidLoopUnitY, 12, 13) |
      __gen_field(values->MidLoopUnitX, 8, 9) |
      0;

   dw[7] =
      __gen_field(values->GlobalLoopExecCount, 16, 25) |
      __gen_field(values->LocalLoopExecCount, 0, 9) |
      0;

   dw[8] =
      __gen_field(values->BlockResolutionY, 16, 24) |
      __gen_field(values->BlockResolutionX, 0, 8) |
      0;

   dw[9] =
      __gen_field(values->LocalStartY, 16, 24) |
      __gen_field(values->LocalStartX, 0, 8) |
      0;

   dw[10] =
      __gen_field(values->LocalEndY, 16, 24) |
      __gen_field(values->LocalEndX, 0, 8) |
      0;

   dw[11] =
      __gen_field(values->LocalOuterLoopStrideY, 16, 25) |
      __gen_field(values->LocalOuterLoopStrideX, 0, 9) |
      0;

   dw[12] =
      __gen_field(values->LocalInnerLoopUnitY, 16, 25) |
      __gen_field(values->LocalInnerLoopUnitX, 0, 9) |
      0;

   dw[13] =
      __gen_field(values->GlobalResolutionY, 16, 24) |
      __gen_field(values->GlobalResolutionX, 0, 8) |
      0;

   dw[14] =
      __gen_field(values->GlobalStartY, 16, 25) |
      __gen_field(values->GlobalStartX, 0, 9) |
      0;

   dw[15] =
      __gen_field(values->GlobalOuterLoopStrideY, 16, 25) |
      __gen_field(values->GlobalOuterLoopStrideX, 0, 9) |
      0;

   dw[16] =
      __gen_field(values->GlobalInnerLoopUnitY, 16, 25) |
      __gen_field(values->GlobalInnerLoopUnitX, 0, 9) |
      0;

   /* variable length fields follow */
}

#define GEN7_MEDIA_STATE_FLUSH_length 0x00000002
#define GEN7_MEDIA_STATE_FLUSH_length_bias 0x00000002
#define GEN7_MEDIA_STATE_FLUSH_header           \
   .CommandType          =  3,                  \
   .Pipeline             =  2,                  \
   .MediaCommandOpcode   =  0,                  \
   .SubOpcode            =  4,                  \
   .DwordLength          =  0

struct GEN7_MEDIA_STATE_FLUSH {
   uint32_t                                     CommandType;
   uint32_t                                     Pipeline;
   uint32_t                                     MediaCommandOpcode;
   uint32_t                                     SubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     WatermarkRequired;
   uint32_t                                     InterfaceDescriptorOffset;
};

static inline void
GEN7_MEDIA_STATE_FLUSH_pack(__gen_user_data *data, void * restrict dst,
                           const struct GEN7_MEDIA_STATE_FLUSH * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->Pipeline, 27, 28) |
      __gen_field(values->MediaCommandOpcode, 24, 26) |
      __gen_field(values->SubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 15) |
      0;

   dw[1] =
      __gen_field(values->WatermarkRequired, 6, 6) |
      __gen_field(values->InterfaceDescriptorOffset, 0, 5) |
      0;

}

#define GEN7_MEDIA_VFE_STATE_length 0x00000008
#define GEN7_MEDIA_VFE_STATE_length_bias 0x00000002
#define GEN7_MEDIA_VFE_STATE_header             \
   .CommandType          =  3,                  \
   .Pipeline             =  2,                  \
   .MediaCommandOpcode   =  0,                  \
   .SubOpcode            =  0,                  \
   .DwordLength          =  6

struct GEN7_MEDIA_VFE_STATE {
   uint32_t                                     CommandType;
   uint32_t                                     Pipeline;
   uint32_t                                     MediaCommandOpcode;
   uint32_t                                     SubOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     ScratchSpaceBasePointer;
   uint32_t                                     PerThreadScratchSpace;
   uint32_t                                     MaximumNumberofThreads;
   uint32_t                                     NumberofURBEntries;
#define     Maintainingtheexistingtimestampstate               0
#define     Resettingrelativetimerandlatchingtheglobaltimestamp       1
   uint32_t                                     ResetGatewayTimer;
#define     MaintainingOpenGatewayForwardMsgCloseGatewayprotocollegacymode       0
#define     BypassingOpenGatewayCloseGatewayprotocol           1
   uint32_t                                     BypassGatewayControl;
#define     NoMMIOreadwriteallowed                             0
#define     MMIOreadwritetoanyaddress                          2
   uint32_t                                     GatewayMMIOAccessControl;
   uint32_t                                     GPGPUMode;
   uint32_t                                     URBEntryAllocationSize;
   uint32_t                                     CURBEAllocationSize;
#define     Scoreboarddisabled                                 0
#define     Scoreboardenabled                                  1
   uint32_t                                     ScoreboardEnable;
#define     StallingScoreboard                                 0
#define     NonStallingScoreboard                              1
   uint32_t                                     ScoreboardType;
   uint32_t                                     ScoreboardMask;
   uint32_t                                     Scoreboard3DeltaY;
   uint32_t                                     Scoreboard3DeltaX;
   uint32_t                                     Scoreboard2DeltaY;
   uint32_t                                     Scoreboard2DeltaX;
   uint32_t                                     Scoreboard1DeltaY;
   uint32_t                                     Scoreboard1DeltaX;
   uint32_t                                     Scoreboard0DeltaY;
   uint32_t                                     Scoreboard0DeltaX;
   uint32_t                                     Scoreboard7DeltaY;
   uint32_t                                     Scoreboard7DeltaX;
   uint32_t                                     Scoreboard6DeltaY;
   uint32_t                                     Scoreboard6DeltaX;
   uint32_t                                     Scoreboard5DeltaY;
   uint32_t                                     Scoreboard5DeltaX;
   uint32_t                                     Scoreboard4DeltaY;
   uint32_t                                     Scoreboard4DeltaX;
};

static inline void
GEN7_MEDIA_VFE_STATE_pack(__gen_user_data *data, void * restrict dst,
                         const struct GEN7_MEDIA_VFE_STATE * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->Pipeline, 27, 28) |
      __gen_field(values->MediaCommandOpcode, 24, 26) |
      __gen_field(values->SubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 15) |
      0;

   dw[1] =
      __gen_offset(values->ScratchSpaceBasePointer, 10, 31) |
      __gen_field(values->PerThreadScratchSpace, 0, 3) |
      0;

   dw[2] =
      __gen_field(values->MaximumNumberofThreads, 16, 31) |
      __gen_field(values->NumberofURBEntries, 8, 15) |
      __gen_field(values->ResetGatewayTimer, 7, 7) |
      __gen_field(values->BypassGatewayControl, 6, 6) |
      __gen_field(values->GatewayMMIOAccessControl, 3, 4) |
      __gen_field(values->GPGPUMode, 2, 2) |
      0;

   dw[3] =
      0;

   dw[4] =
      __gen_field(values->URBEntryAllocationSize, 16, 31) |
      __gen_field(values->CURBEAllocationSize, 0, 15) |
      0;

   dw[5] =
      __gen_field(values->ScoreboardEnable, 31, 31) |
      __gen_field(values->ScoreboardType, 30, 30) |
      __gen_field(values->ScoreboardMask, 0, 7) |
      0;

   dw[6] =
      __gen_field(values->Scoreboard3DeltaY, 28, 31) |
      __gen_field(values->Scoreboard3DeltaX, 24, 27) |
      __gen_field(values->Scoreboard2DeltaY, 20, 23) |
      __gen_field(values->Scoreboard2DeltaX, 16, 19) |
      __gen_field(values->Scoreboard1DeltaY, 12, 15) |
      __gen_field(values->Scoreboard1DeltaX, 8, 11) |
      __gen_field(values->Scoreboard0DeltaY, 4, 7) |
      __gen_field(values->Scoreboard0DeltaX, 0, 3) |
      0;

   dw[7] =
      __gen_field(values->Scoreboard7DeltaY, 28, 31) |
      __gen_field(values->Scoreboard7DeltaX, 24, 27) |
      __gen_field(values->Scoreboard6DeltaY, 20, 23) |
      __gen_field(values->Scoreboard6DeltaX, 16, 19) |
      __gen_field(values->Scoreboard5DeltaY, 12, 15) |
      __gen_field(values->Scoreboard5DeltaX, 8, 11) |
      __gen_field(values->Scoreboard4DeltaY, 4, 7) |
      __gen_field(values->Scoreboard4DeltaX, 0, 3) |
      0;

}

#define GEN7_MI_ARB_CHECK_length 0x00000001
#define GEN7_MI_ARB_CHECK_length_bias 0x00000001
#define GEN7_MI_ARB_CHECK_header                \
   .CommandType          =  0,                  \
   .MICommandOpcode      =  5

struct GEN7_MI_ARB_CHECK {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
};

static inline void
GEN7_MI_ARB_CHECK_pack(__gen_user_data *data, void * restrict dst,
                      const struct GEN7_MI_ARB_CHECK * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      0;

}

#define GEN7_MI_ARB_ON_OFF_length 0x00000001
#define GEN7_MI_ARB_ON_OFF_length_bias 0x00000001
#define GEN7_MI_ARB_ON_OFF_header               \
   .CommandType          =  0,                  \
   .MICommandOpcode      =  8

struct GEN7_MI_ARB_ON_OFF {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
   uint32_t                                     ArbitrationEnable;
};

static inline void
GEN7_MI_ARB_ON_OFF_pack(__gen_user_data *data, void * restrict dst,
                       const struct GEN7_MI_ARB_ON_OFF * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      __gen_field(values->ArbitrationEnable, 0, 0) |
      0;

}

#define GEN7_MI_BATCH_BUFFER_END_length 0x00000001
#define GEN7_MI_BATCH_BUFFER_END_length_bias 0x00000001
#define GEN7_MI_BATCH_BUFFER_END_header         \
   .CommandType          =  0,                  \
   .MICommandOpcode      = 10

struct GEN7_MI_BATCH_BUFFER_END {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
};

static inline void
GEN7_MI_BATCH_BUFFER_END_pack(__gen_user_data *data, void * restrict dst,
                             const struct GEN7_MI_BATCH_BUFFER_END * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      0;

}

#define GEN7_MI_BATCH_BUFFER_START_length 0x00000002
#define GEN7_MI_BATCH_BUFFER_START_length_bias 0x00000002
#define GEN7_MI_BATCH_BUFFER_START_header       \
   .CommandType          =  0,                  \
   .MICommandOpcode      = 49,                  \
   .DwordLength          =  0

struct GEN7_MI_BATCH_BUFFER_START {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
   uint32_t                                     ClearCommandBufferEnable;
#define     ASI_GGTT                                           0
#define     ASI_PPGTT                                          1
   uint32_t                                     AddressSpaceIndicator;
   uint32_t                                     DwordLength;
   __gen_address_type                           BatchBufferStartAddress;
};

static inline void
GEN7_MI_BATCH_BUFFER_START_pack(__gen_user_data *data, void * restrict dst,
                               const struct GEN7_MI_BATCH_BUFFER_START * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      __gen_field(values->ClearCommandBufferEnable, 11, 11) |
      __gen_field(values->AddressSpaceIndicator, 8, 8) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   uint32_t dw1 =
      0;

   dw[1] =
      __gen_combine_address(data, &dw[1], values->BatchBufferStartAddress, dw1);

}

#define GEN7_MI_CLFLUSH_length_bias 0x00000002
#define GEN7_MI_CLFLUSH_header                  \
   .CommandType          =  0,                  \
   .MICommandOpcode      = 39

struct GEN7_MI_CLFLUSH {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
#define     PerProcessGraphicsAddress                          0
#define     GlobalGraphicsAddress                              1
   uint32_t                                     UseGlobalGTT;
   uint32_t                                     DwordLength;
   __gen_address_type                           PageBaseAddress;
   uint32_t                                     StartingCachelineOffset;
   __gen_address_type                           PageBaseAddressHigh;
   /* variable length fields follow */
};

static inline void
GEN7_MI_CLFLUSH_pack(__gen_user_data *data, void * restrict dst,
                    const struct GEN7_MI_CLFLUSH * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      __gen_field(values->UseGlobalGTT, 22, 22) |
      __gen_field(values->DwordLength, 0, 9) |
      0;

   uint32_t dw1 =
      __gen_field(values->StartingCachelineOffset, 6, 11) |
      0;

   dw[1] =
      __gen_combine_address(data, &dw[1], values->PageBaseAddress, dw1);

   uint32_t dw2 =
      0;

   dw[2] =
      __gen_combine_address(data, &dw[2], values->PageBaseAddressHigh, dw2);

   /* variable length fields follow */
}

#define GEN7_MI_CONDITIONAL_BATCH_BUFFER_END_length 0x00000002
#define GEN7_MI_CONDITIONAL_BATCH_BUFFER_END_length_bias 0x00000002
#define GEN7_MI_CONDITIONAL_BATCH_BUFFER_END_header\
   .CommandType          =  0,                  \
   .MICommandOpcode      = 54,                  \
   .UseGlobalGTT         =  0,                  \
   .CompareSemaphore     =  0,                  \
   .DwordLength          =  0

struct GEN7_MI_CONDITIONAL_BATCH_BUFFER_END {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
   uint32_t                                     UseGlobalGTT;
   uint32_t                                     CompareSemaphore;
   uint32_t                                     DwordLength;
   uint32_t                                     CompareDataDword;
   __gen_address_type                           CompareAddress;
};

static inline void
GEN7_MI_CONDITIONAL_BATCH_BUFFER_END_pack(__gen_user_data *data, void * restrict dst,
                                         const struct GEN7_MI_CONDITIONAL_BATCH_BUFFER_END * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      __gen_field(values->UseGlobalGTT, 22, 22) |
      __gen_field(values->CompareSemaphore, 21, 21) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->CompareDataDword, 0, 31) |
      0;

   uint32_t dw2 =
      0;

   dw[2] =
      __gen_combine_address(data, &dw[2], values->CompareAddress, dw2);

}

#define GEN7_MI_FLUSH_length 0x00000001
#define GEN7_MI_FLUSH_length_bias 0x00000001
#define GEN7_MI_FLUSH_header                    \
   .CommandType          =  0,                  \
   .MICommandOpcode      =  4

struct GEN7_MI_FLUSH {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
   uint32_t                                     IndirectStatePointersDisable;
   uint32_t                                     GenericMediaStateClear;
#define     DontReset                                          0
#define     Reset                                              1
   uint32_t                                     GlobalSnapshotCountReset;
#define     Flush                                              0
#define     DontFlush                                          1
   uint32_t                                     RenderCacheFlushInhibit;
#define     DontInvalidate                                     0
#define     Invalidate                                         1
   uint32_t                                     StateInstructionCacheInvalidate;
};

static inline void
GEN7_MI_FLUSH_pack(__gen_user_data *data, void * restrict dst,
                  const struct GEN7_MI_FLUSH * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      __gen_field(values->IndirectStatePointersDisable, 5, 5) |
      __gen_field(values->GenericMediaStateClear, 4, 4) |
      __gen_field(values->GlobalSnapshotCountReset, 3, 3) |
      __gen_field(values->RenderCacheFlushInhibit, 2, 2) |
      __gen_field(values->StateInstructionCacheInvalidate, 1, 1) |
      0;

}

#define GEN7_MI_LOAD_REGISTER_IMM_length 0x00000003
#define GEN7_MI_LOAD_REGISTER_IMM_length_bias 0x00000002
#define GEN7_MI_LOAD_REGISTER_IMM_header        \
   .CommandType          =  0,                  \
   .MICommandOpcode      = 34,                  \
   .DwordLength          =  1

struct GEN7_MI_LOAD_REGISTER_IMM {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
   uint32_t                                     ByteWriteDisables;
   uint32_t                                     DwordLength;
   uint32_t                                     RegisterOffset;
   uint32_t                                     DataDWord;
};

static inline void
GEN7_MI_LOAD_REGISTER_IMM_pack(__gen_user_data *data, void * restrict dst,
                              const struct GEN7_MI_LOAD_REGISTER_IMM * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      __gen_field(values->ByteWriteDisables, 8, 11) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->RegisterOffset, 2, 22) |
      0;

   dw[2] =
      __gen_field(values->DataDWord, 0, 31) |
      0;

}

#define GEN7_MI_LOAD_REGISTER_MEM_length 0x00000003
#define GEN7_MI_LOAD_REGISTER_MEM_length_bias 0x00000002
#define GEN7_MI_LOAD_REGISTER_MEM_header        \
   .CommandType          =  0,                  \
   .MICommandOpcode      = 41,                  \
   .DwordLength          =  1

struct GEN7_MI_LOAD_REGISTER_MEM {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
   uint32_t                                     UseGlobalGTT;
   uint32_t                                     AsyncModeEnable;
   uint32_t                                     DwordLength;
   uint32_t                                     RegisterAddress;
   __gen_address_type                           MemoryAddress;
};

static inline void
GEN7_MI_LOAD_REGISTER_MEM_pack(__gen_user_data *data, void * restrict dst,
                              const struct GEN7_MI_LOAD_REGISTER_MEM * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      __gen_field(values->UseGlobalGTT, 22, 22) |
      __gen_field(values->AsyncModeEnable, 21, 21) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_offset(values->RegisterAddress, 2, 22) |
      0;

   uint32_t dw2 =
      0;

   dw[2] =
      __gen_combine_address(data, &dw[2], values->MemoryAddress, dw2);

}

#define GEN7_MI_NOOP_length 0x00000001
#define GEN7_MI_NOOP_length_bias 0x00000001
#define GEN7_MI_NOOP_header                     \
   .CommandType          =  0,                  \
   .MICommandOpcode      =  0

struct GEN7_MI_NOOP {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
   uint32_t                                     IdentificationNumberRegisterWriteEnable;
   uint32_t                                     IdentificationNumber;
};

static inline void
GEN7_MI_NOOP_pack(__gen_user_data *data, void * restrict dst,
                 const struct GEN7_MI_NOOP * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      __gen_field(values->IdentificationNumberRegisterWriteEnable, 22, 22) |
      __gen_field(values->IdentificationNumber, 0, 21) |
      0;

}

#define GEN7_MI_PREDICATE_length 0x00000001
#define GEN7_MI_PREDICATE_length_bias 0x00000001
#define GEN7_MI_PREDICATE_header                \
   .CommandType          =  0,                  \
   .MICommandOpcode      = 12

struct GEN7_MI_PREDICATE {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
#define     KEEP                                               0
#define     LOAD                                               2
#define     LOADINV                                            3
   uint32_t                                     LoadOperation;
#define     COMBINE_SET                                        0
#define     COMBINE_AND                                        1
#define     COMBINE_OR                                         2
#define     COMBINE_XOR                                        3
   uint32_t                                     CombineOperation;
#define     COMPARE_SRCS_EQUAL                                 2
#define     COMPARE_DELTAS_EQUAL                               3
   uint32_t                                     CompareOperation;
};

static inline void
GEN7_MI_PREDICATE_pack(__gen_user_data *data, void * restrict dst,
                      const struct GEN7_MI_PREDICATE * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      __gen_field(values->LoadOperation, 6, 7) |
      __gen_field(values->CombineOperation, 3, 4) |
      __gen_field(values->CompareOperation, 0, 1) |
      0;

}

#define GEN7_MI_REPORT_HEAD_length 0x00000001
#define GEN7_MI_REPORT_HEAD_length_bias 0x00000001
#define GEN7_MI_REPORT_HEAD_header              \
   .CommandType          =  0,                  \
   .MICommandOpcode      =  7

struct GEN7_MI_REPORT_HEAD {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
};

static inline void
GEN7_MI_REPORT_HEAD_pack(__gen_user_data *data, void * restrict dst,
                        const struct GEN7_MI_REPORT_HEAD * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      0;

}

#define GEN7_MI_SEMAPHORE_MBOX_length 0x00000003
#define GEN7_MI_SEMAPHORE_MBOX_length_bias 0x00000002
#define GEN7_MI_SEMAPHORE_MBOX_header           \
   .CommandType          =  0,                  \
   .MICommandOpcode      = 22,                  \
   .DwordLength          =  1

struct GEN7_MI_SEMAPHORE_MBOX {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
#define     RVSYNC                                             0
#define     RBSYNC                                             2
#define     UseGeneralRegisterSelect                           3
   uint32_t                                     RegisterSelect;
   uint32_t                                     DwordLength;
   uint32_t                                     SemaphoreDataDword;
};

static inline void
GEN7_MI_SEMAPHORE_MBOX_pack(__gen_user_data *data, void * restrict dst,
                           const struct GEN7_MI_SEMAPHORE_MBOX * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      __gen_field(values->RegisterSelect, 16, 17) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->SemaphoreDataDword, 0, 31) |
      0;

   dw[2] =
      0;

}

#define GEN7_MI_SET_CONTEXT_length 0x00000002
#define GEN7_MI_SET_CONTEXT_length_bias 0x00000002
#define GEN7_MI_SET_CONTEXT_header              \
   .CommandType          =  0,                  \
   .MICommandOpcode      = 24,                  \
   .DwordLength          =  0

struct GEN7_MI_SET_CONTEXT {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
   uint32_t                                     DwordLength;
   __gen_address_type                           LogicalContextAddress;
   uint32_t                                     ReservedMustbe1;
   uint32_t                                     ExtendedStateSaveEnable;
   uint32_t                                     ExtendedStateRestoreEnable;
   uint32_t                                     ForceRestore;
   uint32_t                                     RestoreInhibit;
};

static inline void
GEN7_MI_SET_CONTEXT_pack(__gen_user_data *data, void * restrict dst,
                        const struct GEN7_MI_SET_CONTEXT * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   uint32_t dw1 =
      __gen_field(values->ReservedMustbe1, 8, 8) |
      __gen_field(values->ExtendedStateSaveEnable, 3, 3) |
      __gen_field(values->ExtendedStateRestoreEnable, 2, 2) |
      __gen_field(values->ForceRestore, 1, 1) |
      __gen_field(values->RestoreInhibit, 0, 0) |
      0;

   dw[1] =
      __gen_combine_address(data, &dw[1], values->LogicalContextAddress, dw1);

}

#define GEN7_MI_STORE_DATA_IMM_length 0x00000004
#define GEN7_MI_STORE_DATA_IMM_length_bias 0x00000002
#define GEN7_MI_STORE_DATA_IMM_header           \
   .CommandType          =  0,                  \
   .MICommandOpcode      = 32,                  \
   .DwordLength          =  2

struct GEN7_MI_STORE_DATA_IMM {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
   uint32_t                                     UseGlobalGTT;
   uint32_t                                     DwordLength;
   uint32_t                                     Address;
   uint32_t                                     CoreModeEnable;
   uint32_t                                     DataDWord0;
   uint32_t                                     DataDWord1;
};

static inline void
GEN7_MI_STORE_DATA_IMM_pack(__gen_user_data *data, void * restrict dst,
                           const struct GEN7_MI_STORE_DATA_IMM * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      __gen_field(values->UseGlobalGTT, 22, 22) |
      __gen_field(values->DwordLength, 0, 5) |
      0;

   dw[1] =
      0;

   dw[2] =
      __gen_field(values->Address, 2, 31) |
      __gen_field(values->CoreModeEnable, 0, 0) |
      0;

   dw[3] =
      __gen_field(values->DataDWord0, 0, 31) |
      0;

   dw[4] =
      __gen_field(values->DataDWord1, 0, 31) |
      0;

}

#define GEN7_MI_STORE_DATA_INDEX_length 0x00000003
#define GEN7_MI_STORE_DATA_INDEX_length_bias 0x00000002
#define GEN7_MI_STORE_DATA_INDEX_header         \
   .CommandType          =  0,                  \
   .MICommandOpcode      = 33,                  \
   .DwordLength          =  1

struct GEN7_MI_STORE_DATA_INDEX {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     Offset;
   uint32_t                                     DataDWord0;
   uint32_t                                     DataDWord1;
};

static inline void
GEN7_MI_STORE_DATA_INDEX_pack(__gen_user_data *data, void * restrict dst,
                             const struct GEN7_MI_STORE_DATA_INDEX * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->Offset, 2, 11) |
      0;

   dw[2] =
      __gen_field(values->DataDWord0, 0, 31) |
      0;

   dw[3] =
      __gen_field(values->DataDWord1, 0, 31) |
      0;

}

#define GEN7_MI_SUSPEND_FLUSH_length 0x00000001
#define GEN7_MI_SUSPEND_FLUSH_length_bias 0x00000001
#define GEN7_MI_SUSPEND_FLUSH_header            \
   .CommandType          =  0,                  \
   .MICommandOpcode      = 11

struct GEN7_MI_SUSPEND_FLUSH {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
   uint32_t                                     SuspendFlush;
};

static inline void
GEN7_MI_SUSPEND_FLUSH_pack(__gen_user_data *data, void * restrict dst,
                          const struct GEN7_MI_SUSPEND_FLUSH * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      __gen_field(values->SuspendFlush, 0, 0) |
      0;

}

#define GEN7_MI_TOPOLOGY_FILTER_length 0x00000001
#define GEN7_MI_TOPOLOGY_FILTER_length_bias 0x00000001
#define GEN7_MI_TOPOLOGY_FILTER_header          \
   .CommandType          =  0,                  \
   .MICommandOpcode      = 13

struct GEN7_MI_TOPOLOGY_FILTER {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
   uint32_t                                     TopologyFilterValue;
};

static inline void
GEN7_MI_TOPOLOGY_FILTER_pack(__gen_user_data *data, void * restrict dst,
                            const struct GEN7_MI_TOPOLOGY_FILTER * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      __gen_field(values->TopologyFilterValue, 0, 5) |
      0;

}

#define GEN7_MI_UPDATE_GTT_length_bias 0x00000002
#define GEN7_MI_UPDATE_GTT_header               \
   .CommandType          =  0,                  \
   .MICommandOpcode      = 35

struct GEN7_MI_UPDATE_GTT {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
#define     PerProcessGraphicsAddress                          0
#define     GlobalGraphicsAddress                              1
   uint32_t                                     UseGlobalGTT;
   uint32_t                                     DwordLength;
   __gen_address_type                           EntryAddress;
   /* variable length fields follow */
};

static inline void
GEN7_MI_UPDATE_GTT_pack(__gen_user_data *data, void * restrict dst,
                       const struct GEN7_MI_UPDATE_GTT * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      __gen_field(values->UseGlobalGTT, 22, 22) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   uint32_t dw1 =
      0;

   dw[1] =
      __gen_combine_address(data, &dw[1], values->EntryAddress, dw1);

   /* variable length fields follow */
}

#define GEN7_MI_URB_CLEAR_length 0x00000002
#define GEN7_MI_URB_CLEAR_length_bias 0x00000002
#define GEN7_MI_URB_CLEAR_header                \
   .CommandType          =  0,                  \
   .MICommandOpcode      = 25,                  \
   .DwordLength          =  0

struct GEN7_MI_URB_CLEAR {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
   uint32_t                                     DwordLength;
   uint32_t                                     URBClearLength;
   uint32_t                                     URBAddress;
};

static inline void
GEN7_MI_URB_CLEAR_pack(__gen_user_data *data, void * restrict dst,
                      const struct GEN7_MI_URB_CLEAR * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->URBClearLength, 16, 28) |
      __gen_offset(values->URBAddress, 0, 13) |
      0;

}

#define GEN7_MI_USER_INTERRUPT_length 0x00000001
#define GEN7_MI_USER_INTERRUPT_length_bias 0x00000001
#define GEN7_MI_USER_INTERRUPT_header           \
   .CommandType          =  0,                  \
   .MICommandOpcode      =  2

struct GEN7_MI_USER_INTERRUPT {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
};

static inline void
GEN7_MI_USER_INTERRUPT_pack(__gen_user_data *data, void * restrict dst,
                           const struct GEN7_MI_USER_INTERRUPT * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      0;

}

#define GEN7_MI_WAIT_FOR_EVENT_length 0x00000001
#define GEN7_MI_WAIT_FOR_EVENT_length_bias 0x00000001
#define GEN7_MI_WAIT_FOR_EVENT_header           \
   .CommandType          =  0,                  \
   .MICommandOpcode      =  3

struct GEN7_MI_WAIT_FOR_EVENT {
   uint32_t                                     CommandType;
   uint32_t                                     MICommandOpcode;
   uint32_t                                     DisplayPipeCHorizontalBlankWaitEnable;
   uint32_t                                     DisplayPipeCVerticalBlankWaitEnable;
   uint32_t                                     DisplaySpriteCFlipPendingWaitEnable;
#define     Notenabled                                         0
   uint32_t                                     ConditionCodeWaitSelect;
   uint32_t                                     DisplayPlaneCFlipPendingWaitEnable;
   uint32_t                                     DisplayPipeCScanLineWaitEnable;
   uint32_t                                     DisplayPipeBHorizontalBlankWaitEnable;
   uint32_t                                     DisplayPipeBVerticalBlankWaitEnable;
   uint32_t                                     DisplaySpriteBFlipPendingWaitEnable;
   uint32_t                                     DisplayPlaneBFlipPendingWaitEnable;
   uint32_t                                     DisplayPipeBScanLineWaitEnable;
   uint32_t                                     DisplayPipeAHorizontalBlankWaitEnable;
   uint32_t                                     DisplayPipeAVerticalBlankWaitEnable;
   uint32_t                                     DisplaySpriteAFlipPendingWaitEnable;
   uint32_t                                     DisplayPlaneAFlipPendingWaitEnable;
   uint32_t                                     DisplayPipeAScanLineWaitEnable;
};

static inline void
GEN7_MI_WAIT_FOR_EVENT_pack(__gen_user_data *data, void * restrict dst,
                           const struct GEN7_MI_WAIT_FOR_EVENT * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->MICommandOpcode, 23, 28) |
      __gen_field(values->DisplayPipeCHorizontalBlankWaitEnable, 22, 22) |
      __gen_field(values->DisplayPipeCVerticalBlankWaitEnable, 21, 21) |
      __gen_field(values->DisplaySpriteCFlipPendingWaitEnable, 20, 20) |
      __gen_field(values->ConditionCodeWaitSelect, 16, 19) |
      __gen_field(values->DisplayPlaneCFlipPendingWaitEnable, 15, 15) |
      __gen_field(values->DisplayPipeCScanLineWaitEnable, 14, 14) |
      __gen_field(values->DisplayPipeBHorizontalBlankWaitEnable, 13, 13) |
      __gen_field(values->DisplayPipeBVerticalBlankWaitEnable, 11, 11) |
      __gen_field(values->DisplaySpriteBFlipPendingWaitEnable, 10, 10) |
      __gen_field(values->DisplayPlaneBFlipPendingWaitEnable, 9, 9) |
      __gen_field(values->DisplayPipeBScanLineWaitEnable, 8, 8) |
      __gen_field(values->DisplayPipeAHorizontalBlankWaitEnable, 5, 5) |
      __gen_field(values->DisplayPipeAVerticalBlankWaitEnable, 3, 3) |
      __gen_field(values->DisplaySpriteAFlipPendingWaitEnable, 2, 2) |
      __gen_field(values->DisplayPlaneAFlipPendingWaitEnable, 1, 1) |
      __gen_field(values->DisplayPipeAScanLineWaitEnable, 0, 0) |
      0;

}

#define GEN7_PIPE_CONTROL_length 0x00000005
#define GEN7_PIPE_CONTROL_length_bias 0x00000002
#define GEN7_PIPE_CONTROL_header                \
   .CommandType          =  3,                  \
   .CommandSubType       =  3,                  \
   ._3DCommandOpcode     =  2,                  \
   ._3DCommandSubOpcode  =  0,                  \
   .DwordLength          =  3

struct GEN7_PIPE_CONTROL {
   uint32_t                                     CommandType;
   uint32_t                                     CommandSubType;
   uint32_t                                     _3DCommandOpcode;
   uint32_t                                     _3DCommandSubOpcode;
   uint32_t                                     DwordLength;
#define     DAT_PPGTT                                          0
#define     DAT_GGTT                                           1
   uint32_t                                     DestinationAddressType;
#define     NoLRIOperation                                     0
#define     MMIOWriteImmediateData                             1
   uint32_t                                     LRIPostSyncOperation;
   uint32_t                                     StoreDataIndex;
   uint32_t                                     CommandStreamerStallEnable;
#define     DontReset                                          0
#define     Reset                                              1
   uint32_t                                     GlobalSnapshotCountReset;
   uint32_t                                     TLBInvalidate;
   uint32_t                                     GenericMediaStateClear;
#define     NoWrite                                            0
#define     WriteImmediateData                                 1
#define     WritePSDepthCount                                  2
#define     WriteTimestamp                                     3
   uint32_t                                     PostSyncOperation;
   uint32_t                                     DepthStallEnable;
#define     DisableFlush                                       0
#define     EnableFlush                                        1
   uint32_t                                     RenderTargetCacheFlushEnable;
   uint32_t                                     InstructionCacheInvalidateEnable;
   uint32_t                                     TextureCacheInvalidationEnable;
   uint32_t                                     IndirectStatePointersDisable;
   uint32_t                                     NotifyEnable;
   uint32_t                                     PipeControlFlushEnable;
   uint32_t                                     DCFlushEnable;
   uint32_t                                     VFCacheInvalidationEnable;
   uint32_t                                     ConstantCacheInvalidationEnable;
   uint32_t                                     StateCacheInvalidationEnable;
   uint32_t                                     StallAtPixelScoreboard;
#define     FlushDisabled                                      0
#define     FlushEnabled                                       1
   uint32_t                                     DepthCacheFlushEnable;
   __gen_address_type                           Address;
   uint32_t                                     ImmediateData;
   uint32_t                                     ImmediateData0;
};

static inline void
GEN7_PIPE_CONTROL_pack(__gen_user_data *data, void * restrict dst,
                      const struct GEN7_PIPE_CONTROL * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->CommandType, 29, 31) |
      __gen_field(values->CommandSubType, 27, 28) |
      __gen_field(values->_3DCommandOpcode, 24, 26) |
      __gen_field(values->_3DCommandSubOpcode, 16, 23) |
      __gen_field(values->DwordLength, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->DestinationAddressType, 24, 24) |
      __gen_field(values->LRIPostSyncOperation, 23, 23) |
      __gen_field(values->StoreDataIndex, 21, 21) |
      __gen_field(values->CommandStreamerStallEnable, 20, 20) |
      __gen_field(values->GlobalSnapshotCountReset, 19, 19) |
      __gen_field(values->TLBInvalidate, 18, 18) |
      __gen_field(values->GenericMediaStateClear, 16, 16) |
      __gen_field(values->PostSyncOperation, 14, 15) |
      __gen_field(values->DepthStallEnable, 13, 13) |
      __gen_field(values->RenderTargetCacheFlushEnable, 12, 12) |
      __gen_field(values->InstructionCacheInvalidateEnable, 11, 11) |
      __gen_field(values->TextureCacheInvalidationEnable, 10, 10) |
      __gen_field(values->IndirectStatePointersDisable, 9, 9) |
      __gen_field(values->NotifyEnable, 8, 8) |
      __gen_field(values->PipeControlFlushEnable, 7, 7) |
      __gen_field(values->DCFlushEnable, 5, 5) |
      __gen_field(values->VFCacheInvalidationEnable, 4, 4) |
      __gen_field(values->ConstantCacheInvalidationEnable, 3, 3) |
      __gen_field(values->StateCacheInvalidationEnable, 2, 2) |
      __gen_field(values->StallAtPixelScoreboard, 1, 1) |
      __gen_field(values->DepthCacheFlushEnable, 0, 0) |
      0;

   uint32_t dw2 =
      0;

   dw[2] =
      __gen_combine_address(data, &dw[2], values->Address, dw2);

   dw[3] =
      __gen_field(values->ImmediateData, 0, 31) |
      0;

   dw[4] =
      __gen_field(values->ImmediateData, 0, 31) |
      0;

}

#define GEN7_3DSTATE_CONSTANT_BODY_length 0x00000006

#define GEN7_VERTEX_BUFFER_STATE_length 0x00000004

#define GEN7_VERTEX_ELEMENT_STATE_length 0x00000002

#define GEN7_SO_DECL_ENTRY_length 0x00000002

#define GEN7_SO_DECL_length 0x00000001

#define GEN7_SCISSOR_RECT_length 0x00000002

struct GEN7_SCISSOR_RECT {
   uint32_t                                     ScissorRectangleYMin;
   uint32_t                                     ScissorRectangleXMin;
   uint32_t                                     ScissorRectangleYMax;
   uint32_t                                     ScissorRectangleXMax;
};

static inline void
GEN7_SCISSOR_RECT_pack(__gen_user_data *data, void * restrict dst,
                      const struct GEN7_SCISSOR_RECT * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->ScissorRectangleYMin, 16, 31) |
      __gen_field(values->ScissorRectangleXMin, 0, 15) |
      0;

   dw[1] =
      __gen_field(values->ScissorRectangleYMax, 16, 31) |
      __gen_field(values->ScissorRectangleXMax, 0, 15) |
      0;

}

#define GEN7_SF_CLIP_VIEWPORT_length 0x00000010

struct GEN7_SF_CLIP_VIEWPORT {
   float                                        ViewportMatrixElementm00;
   float                                        ViewportMatrixElementm11;
   float                                        ViewportMatrixElementm22;
   float                                        ViewportMatrixElementm30;
   float                                        ViewportMatrixElementm31;
   float                                        ViewportMatrixElementm32;
   float                                        XMinClipGuardband;
   float                                        XMaxClipGuardband;
   float                                        YMinClipGuardband;
   float                                        YMaxClipGuardband;
};

static inline void
GEN7_SF_CLIP_VIEWPORT_pack(__gen_user_data *data, void * restrict dst,
                          const struct GEN7_SF_CLIP_VIEWPORT * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_float(values->ViewportMatrixElementm00) |
      0;

   dw[1] =
      __gen_float(values->ViewportMatrixElementm11) |
      0;

   dw[2] =
      __gen_float(values->ViewportMatrixElementm22) |
      0;

   dw[3] =
      __gen_float(values->ViewportMatrixElementm30) |
      0;

   dw[4] =
      __gen_float(values->ViewportMatrixElementm31) |
      0;

   dw[5] =
      __gen_float(values->ViewportMatrixElementm32) |
      0;

   dw[6] =
      0;

   dw[7] =
      0;

   dw[8] =
      __gen_float(values->XMinClipGuardband) |
      0;

   dw[9] =
      __gen_float(values->XMaxClipGuardband) |
      0;

   dw[10] =
      __gen_float(values->YMinClipGuardband) |
      0;

   dw[11] =
      __gen_float(values->YMaxClipGuardband) |
      0;

   dw[12] =
      0;

}

#define GEN7_BLEND_STATE_length 0x00000002

struct GEN7_BLEND_STATE {
   uint32_t                                     ColorBufferBlendEnable;
   uint32_t                                     IndependentAlphaBlendEnable;
#define     BLENDFUNCTION_ADD                                  0
#define     BLENDFUNCTION_SUBTRACT                             1
#define     BLENDFUNCTION_REVERSE_SUBTRACT                     2
#define     BLENDFUNCTION_MIN                                  3
#define     BLENDFUNCTION_MAX                                  4
   uint32_t                                     AlphaBlendFunction;
#define     BLENDFACTOR_ONE                                    1
#define     BLENDFACTOR_SRC_COLOR                              2
#define     BLENDFACTOR_SRC_ALPHA                              3
#define     BLENDFACTOR_DST_ALPHA                              4
#define     BLENDFACTOR_DST_COLOR                              5
#define     BLENDFACTOR_SRC_ALPHA_SATURATE                     6
#define     BLENDFACTOR_CONST_COLOR                            7
#define     BLENDFACTOR_CONST_ALPHA                            8
#define     BLENDFACTOR_SRC1_COLOR                             9
#define     BLENDFACTOR_SRC1_ALPHA                            10
#define     BLENDFACTOR_ZERO                                  17
#define     BLENDFACTOR_INV_SRC_COLOR                         18
#define     BLENDFACTOR_INV_SRC_ALPHA                         19
#define     BLENDFACTOR_INV_DST_ALPHA                         20
#define     BLENDFACTOR_INV_DST_COLOR                         21
#define     BLENDFACTOR_INV_CONST_COLOR                       23
#define     BLENDFACTOR_INV_CONST_ALPHA                       24
#define     BLENDFACTOR_INV_SRC1_COLOR                        25
#define     BLENDFACTOR_INV_SRC1_ALPHA                        26
   uint32_t                                     SourceAlphaBlendFactor;
   uint32_t                                     DestinationAlphaBlendFactor;
#define     BLENDFUNCTION_ADD                                  0
#define     BLENDFUNCTION_SUBTRACT                             1
#define     BLENDFUNCTION_REVERSE_SUBTRACT                     2
#define     BLENDFUNCTION_MIN                                  3
#define     BLENDFUNCTION_MAX                                  4
   uint32_t                                     ColorBlendFunction;
   uint32_t                                     SourceBlendFactor;
   uint32_t                                     DestinationBlendFactor;
   uint32_t                                     AlphaToCoverageEnable;
   uint32_t                                     AlphaToOneEnable;
   uint32_t                                     AlphaToCoverageDitherEnable;
   uint32_t                                     WriteDisableAlpha;
   uint32_t                                     WriteDisableRed;
   uint32_t                                     WriteDisableGreen;
   uint32_t                                     WriteDisableBlue;
   uint32_t                                     LogicOpEnable;
#define     LOGICOP_CLEAR                                      0
#define     LOGICOP_NOR                                        1
#define     LOGICOP_AND_INVERTED                               2
#define     LOGICOP_COPY_INVERTED                              3
#define     LOGICOP_AND_REVERSE                                4
#define     LOGICOP_INVERT                                     5
#define     LOGICOP_XOR                                        6
#define     LOGICOP_NAND                                       7
#define     LOGICOP_AND                                        8
#define     LOGICOP_EQUIV                                      9
#define     LOGICOP_NOOP                                      10
#define     LOGICOP_OR_INVERTED                               11
#define     LOGICOP_COPY                                      12
#define     LOGICOP_OR_REVERSE                                13
#define     LOGICOP_OR                                        14
#define     LOGICOP_SET                                       15
   uint32_t                                     LogicOpFunction;
   uint32_t                                     AlphaTestEnable;
#define     COMPAREFUNCTION_ALWAYS                             0
#define     COMPAREFUNCTION_NEVER                              1
#define     COMPAREFUNCTION_LESS                               2
#define     COMPAREFUNCTION_EQUAL                              3
#define     COMPAREFUNCTION_LEQUAL                             4
#define     COMPAREFUNCTION_GREATER                            5
#define     COMPAREFUNCTION_NOTEQUAL                           6
#define     COMPAREFUNCTION_GEQUAL                             7
   uint32_t                                     AlphaTestFunction;
   uint32_t                                     ColorDitherEnable;
   uint32_t                                     XDitherOffset;
   uint32_t                                     YDitherOffset;
#define     COLORCLAMP_UNORM                                   0
#define     COLORCLAMP_SNORM                                   1
#define     COLORCLAMP_RTFORMAT                                2
   uint32_t                                     ColorClampRange;
   uint32_t                                     PreBlendColorClampEnable;
   uint32_t                                     PostBlendColorClampEnable;
};

static inline void
GEN7_BLEND_STATE_pack(__gen_user_data *data, void * restrict dst,
                     const struct GEN7_BLEND_STATE * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->ColorBufferBlendEnable, 31, 31) |
      __gen_field(values->IndependentAlphaBlendEnable, 30, 30) |
      __gen_field(values->AlphaBlendFunction, 26, 28) |
      __gen_field(values->SourceAlphaBlendFactor, 20, 24) |
      __gen_field(values->DestinationAlphaBlendFactor, 15, 19) |
      __gen_field(values->ColorBlendFunction, 11, 13) |
      __gen_field(values->SourceBlendFactor, 5, 9) |
      __gen_field(values->DestinationBlendFactor, 0, 4) |
      0;

   dw[1] =
      __gen_field(values->AlphaToCoverageEnable, 31, 31) |
      __gen_field(values->AlphaToOneEnable, 30, 30) |
      __gen_field(values->AlphaToCoverageDitherEnable, 29, 29) |
      __gen_field(values->WriteDisableAlpha, 27, 27) |
      __gen_field(values->WriteDisableRed, 26, 26) |
      __gen_field(values->WriteDisableGreen, 25, 25) |
      __gen_field(values->WriteDisableBlue, 24, 24) |
      __gen_field(values->LogicOpEnable, 22, 22) |
      __gen_field(values->LogicOpFunction, 18, 21) |
      __gen_field(values->AlphaTestEnable, 16, 16) |
      __gen_field(values->AlphaTestFunction, 13, 15) |
      __gen_field(values->ColorDitherEnable, 12, 12) |
      __gen_field(values->XDitherOffset, 10, 11) |
      __gen_field(values->YDitherOffset, 8, 9) |
      __gen_field(values->ColorClampRange, 2, 3) |
      __gen_field(values->PreBlendColorClampEnable, 1, 1) |
      __gen_field(values->PostBlendColorClampEnable, 0, 0) |
      0;

}

#define GEN7_CC_VIEWPORT_length 0x00000002

struct GEN7_CC_VIEWPORT {
   float                                        MinimumDepth;
   float                                        MaximumDepth;
};

static inline void
GEN7_CC_VIEWPORT_pack(__gen_user_data *data, void * restrict dst,
                     const struct GEN7_CC_VIEWPORT * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_float(values->MinimumDepth) |
      0;

   dw[1] =
      __gen_float(values->MaximumDepth) |
      0;

}

#define GEN7_COLOR_CALC_STATE_length 0x00000006

struct GEN7_COLOR_CALC_STATE {
   uint32_t                                     StencilReferenceValue;
   uint32_t                                     BackFaceStencilReferenceValue;
#define     Cancelled                                          0
#define     NotCancelled                                       1
   uint32_t                                     RoundDisableFunctionDisable;
#define     ALPHATEST_UNORM8                                   0
#define     ALPHATEST_FLOAT32                                  1
   uint32_t                                     AlphaTestFormat;
   uint32_t                                     AlphaReferenceValueAsUNORM8;
   float                                        AlphaReferenceValueAsFLOAT32;
   float                                        BlendConstantColorRed;
   float                                        BlendConstantColorGreen;
   float                                        BlendConstantColorBlue;
   float                                        BlendConstantColorAlpha;
};

static inline void
GEN7_COLOR_CALC_STATE_pack(__gen_user_data *data, void * restrict dst,
                          const struct GEN7_COLOR_CALC_STATE * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->StencilReferenceValue, 24, 31) |
      __gen_field(values->BackFaceStencilReferenceValue, 16, 23) |
      __gen_field(values->RoundDisableFunctionDisable, 15, 15) |
      __gen_field(values->AlphaTestFormat, 0, 0) |
      0;

   dw[1] =
      __gen_field(values->AlphaReferenceValueAsUNORM8, 0, 31) |
      __gen_float(values->AlphaReferenceValueAsFLOAT32) |
      0;

   dw[2] =
      __gen_float(values->BlendConstantColorRed) |
      0;

   dw[3] =
      __gen_float(values->BlendConstantColorGreen) |
      0;

   dw[4] =
      __gen_float(values->BlendConstantColorBlue) |
      0;

   dw[5] =
      __gen_float(values->BlendConstantColorAlpha) |
      0;

}

#define GEN7_DEPTH_STENCIL_STATE_length 0x00000003

struct GEN7_DEPTH_STENCIL_STATE {
   uint32_t                                     StencilTestEnable;
#define     COMPAREFUNCTION_ALWAYS                             0
#define     COMPAREFUNCTION_NEVER                              1
#define     COMPAREFUNCTION_LESS                               2
#define     COMPAREFUNCTION_EQUAL                              3
#define     COMPAREFUNCTION_LEQUAL                             4
#define     COMPAREFUNCTION_GREATER                            5
#define     COMPAREFUNCTION_NOTEQUAL                           6
#define     COMPAREFUNCTION_GEQUAL                             7
   uint32_t                                     StencilTestFunction;
#define     STENCILOP_KEEP                                     0
#define     STENCILOP_ZERO                                     1
#define     STENCILOP_REPLACE                                  2
#define     STENCILOP_INCRSAT                                  3
#define     STENCILOP_DECRSAT                                  4
#define     STENCILOP_INCR                                     5
#define     STENCILOP_DECR                                     6
#define     STENCILOP_INVERT                                   7
   uint32_t                                     StencilFailOp;
   uint32_t                                     StencilPassDepthFailOp;
   uint32_t                                     StencilPassDepthPassOp;
   uint32_t                                     StencilBufferWriteEnable;
   uint32_t                                     DoubleSidedStencilEnable;
#define     COMPAREFUNCTION_ALWAYS                             0
#define     COMPAREFUNCTION_NEVER                              1
#define     COMPAREFUNCTION_LESS                               2
#define     COMPAREFUNCTION_EQUAL                              3
#define     COMPAREFUNCTION_LEQUAL                             4
#define     COMPAREFUNCTION_GREATER                            5
#define     COMPAREFUNCTION_NOTEQUAL                           6
#define     COMPAREFUNCTION_GEQUAL                             7
   uint32_t                                     BackFaceStencilTestFunction;
#define     STENCILOP_KEEP                                     0
#define     STENCILOP_ZERO                                     1
#define     STENCILOP_REPLACE                                  2
#define     STENCILOP_INCRSAT                                  3
#define     STENCILOP_DECRSAT                                  4
#define     STENCILOP_INCR                                     5
#define     STENCILOP_DECR                                     6
#define     STENCILOP_INVERT                                   7
   uint32_t                                     BackfaceStencilFailOp;
   uint32_t                                     BackfaceStencilPassDepthFailOp;
   uint32_t                                     BackfaceStencilPassDepthPassOp;
   uint32_t                                     StencilTestMask;
   uint32_t                                     StencilWriteMask;
   uint32_t                                     BackfaceStencilTestMask;
   uint32_t                                     BackfaceStencilWriteMask;
   uint32_t                                     DepthTestEnable;
#define     COMPAREFUNCTION_ALWAYS                             0
#define     COMPAREFUNCTION_NEVER                              1
#define     COMPAREFUNCTION_LESS                               2
#define     COMPAREFUNCTION_EQUAL                              3
#define     COMPAREFUNCTION_LEQUAL                             4
#define     COMPAREFUNCTION_GREATER                            5
#define     COMPAREFUNCTION_NOTEQUAL                           6
#define     COMPAREFUNCTION_GEQUAL                             7
   uint32_t                                     DepthTestFunction;
   uint32_t                                     DepthBufferWriteEnable;
};

static inline void
GEN7_DEPTH_STENCIL_STATE_pack(__gen_user_data *data, void * restrict dst,
                             const struct GEN7_DEPTH_STENCIL_STATE * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->StencilTestEnable, 31, 31) |
      __gen_field(values->StencilTestFunction, 28, 30) |
      __gen_field(values->StencilFailOp, 25, 27) |
      __gen_field(values->StencilPassDepthFailOp, 22, 24) |
      __gen_field(values->StencilPassDepthPassOp, 19, 21) |
      __gen_field(values->StencilBufferWriteEnable, 18, 18) |
      __gen_field(values->DoubleSidedStencilEnable, 15, 15) |
      __gen_field(values->BackFaceStencilTestFunction, 12, 14) |
      __gen_field(values->BackfaceStencilFailOp, 9, 11) |
      __gen_field(values->BackfaceStencilPassDepthFailOp, 6, 8) |
      __gen_field(values->BackfaceStencilPassDepthPassOp, 3, 5) |
      0;

   dw[1] =
      __gen_field(values->StencilTestMask, 24, 31) |
      __gen_field(values->StencilWriteMask, 16, 23) |
      __gen_field(values->BackfaceStencilTestMask, 8, 15) |
      __gen_field(values->BackfaceStencilWriteMask, 0, 7) |
      0;

   dw[2] =
      __gen_field(values->DepthTestEnable, 31, 31) |
      __gen_field(values->DepthTestFunction, 27, 29) |
      __gen_field(values->DepthBufferWriteEnable, 26, 26) |
      0;

}

#define GEN7_MEMORY_OBJECT_CONTROL_STATE_length 0x00000001

#define GEN7_INTERFACE_DESCRIPTOR_DATA_length 0x00000008

struct GEN7_INTERFACE_DESCRIPTOR_DATA {
   uint32_t                                     KernelStartPointer;
#define     Multiple                                           0
#define     Single                                             1
   uint32_t                                     SingleProgramFlow;
#define     NormalPriority                                     0
#define     HighPriority                                       1
   uint32_t                                     ThreadPriority;
#define     IEEE754                                            0
#define     Alternate                                          1
   uint32_t                                     FloatingPointMode;
   uint32_t                                     IllegalOpcodeExceptionEnable;
   uint32_t                                     MaskStackExceptionEnable;
   uint32_t                                     SoftwareExceptionEnable;
   uint32_t                                     SamplerStatePointer;
#define     Nosamplersused                                     0
#define     Between1and4samplersused                           1
#define     Between5and8samplersused                           2
#define     Between9and12samplersused                          3
#define     Between13and16samplersused                         4
   uint32_t                                     SamplerCount;
   uint32_t                                     BindingTablePointer;
   uint32_t                                     BindingTableEntryCount;
   uint32_t                                     ConstantURBEntryReadLength;
   uint32_t                                     ConstantURBEntryReadOffset;
#define     RTNE                                               0
#define     RU                                                 1
#define     RD                                                 2
#define     RTZ                                                3
   uint32_t                                     RoundingMode;
   uint32_t                                     BarrierEnable;
   uint32_t                                     SharedLocalMemorySize;
   uint32_t                                     NumberofThreadsinGPGPUThreadGroup;
};

static inline void
GEN7_INTERFACE_DESCRIPTOR_DATA_pack(__gen_user_data *data, void * restrict dst,
                                   const struct GEN7_INTERFACE_DESCRIPTOR_DATA * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_offset(values->KernelStartPointer, 6, 31) |
      0;

   dw[1] =
      __gen_field(values->SingleProgramFlow, 18, 18) |
      __gen_field(values->ThreadPriority, 17, 17) |
      __gen_field(values->FloatingPointMode, 16, 16) |
      __gen_field(values->IllegalOpcodeExceptionEnable, 13, 13) |
      __gen_field(values->MaskStackExceptionEnable, 11, 11) |
      __gen_field(values->SoftwareExceptionEnable, 7, 7) |
      0;

   dw[2] =
      __gen_offset(values->SamplerStatePointer, 5, 31) |
      __gen_field(values->SamplerCount, 2, 4) |
      0;

   dw[3] =
      __gen_offset(values->BindingTablePointer, 5, 15) |
      __gen_field(values->BindingTableEntryCount, 0, 4) |
      0;

   dw[4] =
      __gen_field(values->ConstantURBEntryReadLength, 16, 31) |
      __gen_field(values->ConstantURBEntryReadOffset, 0, 15) |
      0;

   dw[5] =
      __gen_field(values->RoundingMode, 22, 23) |
      __gen_field(values->BarrierEnable, 21, 21) |
      __gen_field(values->SharedLocalMemorySize, 16, 20) |
      __gen_field(values->NumberofThreadsinGPGPUThreadGroup, 0, 7) |
      0;

   dw[6] =
      0;

   dw[7] =
      0;

}

#define GEN7_PALETTE_ENTRY_length 0x00000001

#define GEN7_SAMPLER_BORDER_COLOR_STATE_length 0x00000004

struct GEN7_SAMPLER_BORDER_COLOR_STATE {
   uint32_t                                     BorderColorRedDX100GL;
   uint32_t                                     BorderColorAlpha;
   uint32_t                                     BorderColorBlue;
   uint32_t                                     BorderColorGreen;
   uint32_t                                     BorderColorRedDX9;
   uint32_t                                     BorderColorGreen0;
   uint32_t                                     BorderColorBlue0;
   uint32_t                                     BorderColorAlpha0;
};

static inline void
GEN7_SAMPLER_BORDER_COLOR_STATE_pack(__gen_user_data *data, void * restrict dst,
                                    const struct GEN7_SAMPLER_BORDER_COLOR_STATE * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->BorderColorRedDX100GL, 0, 31) |
      __gen_field(values->BorderColorAlpha, 24, 31) |
      __gen_field(values->BorderColorBlue, 16, 23) |
      __gen_field(values->BorderColorGreen, 8, 15) |
      __gen_field(values->BorderColorRedDX9, 0, 7) |
      0;

   dw[1] =
      __gen_field(values->BorderColorGreen, 0, 31) |
      0;

   dw[2] =
      __gen_field(values->BorderColorBlue, 0, 31) |
      0;

   dw[3] =
      __gen_field(values->BorderColorAlpha, 0, 31) |
      0;

}

#define GEN7_SAMPLER_STATE_length 0x00000004

struct GEN7_SAMPLER_STATE {
   uint32_t                                     SamplerDisable;
#define     DX10OGL                                            0
#define     DX9                                                1
   uint32_t                                     TextureBorderColorMode;
#define     OGL                                                1
   uint32_t                                     LODPreClampEnable;
   uint32_t                                     BaseMipLevel;
#define     MIPFILTER_NONE                                     0
#define     MIPFILTER_NEAREST                                  1
#define     MIPFILTER_LINEAR                                   3
   uint32_t                                     MipModeFilter;
#define     MAPFILTER_NEAREST                                  0
#define     MAPFILTER_LINEAR                                   1
#define     MAPFILTER_ANISOTROPIC                              2
#define     MAPFILTER_MONO                                     6
   uint32_t                                     MagModeFilter;
#define     MAPFILTER_NEAREST                                  0
#define     MAPFILTER_LINEAR                                   1
#define     MAPFILTER_ANISOTROPIC                              2
#define     MAPFILTER_MONO                                     6
   uint32_t                                     MinModeFilter;
   uint32_t                                     TextureLODBias;
#define     LEGACY                                             0
#define     EWAApproximation                                   1
   uint32_t                                     AnisotropicAlgorithm;
   uint32_t                                     MinLOD;
   uint32_t                                     MaxLOD;
#define     PREFILTEROPALWAYS                                  0
#define     PREFILTEROPNEVER                                   1
#define     PREFILTEROPLESS                                    2
#define     PREFILTEROPEQUAL                                   3
#define     PREFILTEROPLEQUAL                                  4
#define     PREFILTEROPGREATER                                 5
#define     PREFILTEROPNOTEQUAL                                6
#define     PREFILTEROPGEQUAL                                  7
   uint32_t                                     ShadowFunction;
#define     PROGRAMMED                                         0
#define     OVERRIDE                                           1
   uint32_t                                     CubeSurfaceControlMode;
   uint32_t                                     BorderColorPointer;
   uint32_t                                     ChromaKeyEnable;
   uint32_t                                     ChromaKeyIndex;
#define     KEYFILTER_KILL_ON_ANY_MATCH                        0
#define     KEYFILTER_REPLACE_BLACK                            1
   uint32_t                                     ChromaKeyMode;
#define     RATIO21                                            0
#define     RATIO41                                            1
#define     RATIO61                                            2
#define     RATIO81                                            3
#define     RATIO101                                           4
#define     RATIO121                                           5
#define     RATIO141                                           6
#define     RATIO161                                           7
   uint32_t                                     MaximumAnisotropy;
   uint32_t                                     RAddressMinFilterRoundingEnable;
   uint32_t                                     RAddressMagFilterRoundingEnable;
   uint32_t                                     VAddressMinFilterRoundingEnable;
   uint32_t                                     VAddressMagFilterRoundingEnable;
   uint32_t                                     UAddressMinFilterRoundingEnable;
   uint32_t                                     UAddressMagFilterRoundingEnable;
#define     FULL                                               0
#define     MED                                                2
#define     LOW                                                3
   uint32_t                                     TrilinearFilterQuality;
   uint32_t                                     NonnormalizedCoordinateEnable;
   uint32_t                                     TCXAddressControlMode;
   uint32_t                                     TCYAddressControlMode;
   uint32_t                                     TCZAddressControlMode;
};

static inline void
GEN7_SAMPLER_STATE_pack(__gen_user_data *data, void * restrict dst,
                       const struct GEN7_SAMPLER_STATE * restrict values)
{
   uint32_t *dw = (uint32_t * restrict) dst;

   dw[0] =
      __gen_field(values->SamplerDisable, 31, 31) |
      __gen_field(values->TextureBorderColorMode, 29, 29) |
      __gen_field(values->LODPreClampEnable, 28, 28) |
      __gen_field(values->BaseMipLevel, 22, 26) |
      __gen_field(values->MipModeFilter, 20, 21) |
      __gen_field(values->MagModeFilter, 17, 19) |
      __gen_field(values->MinModeFilter, 14, 16) |
      __gen_field(values->TextureLODBias, 1, 13) |
      __gen_field(values->AnisotropicAlgorithm, 0, 0) |
      0;

   dw[1] =
      __gen_field(values->MinLOD, 20, 31) |
      __gen_field(values->MaxLOD, 8, 19) |
      __gen_field(values->ShadowFunction, 1, 3) |
      __gen_field(values->CubeSurfaceControlMode, 0, 0) |
      0;

   dw[2] =
      __gen_offset(values->BorderColorPointer, 5, 31) |
      0;

   dw[3] =
      __gen_field(values->ChromaKeyEnable, 25, 25) |
      __gen_field(values->ChromaKeyIndex, 23, 24) |
      __gen_field(values->ChromaKeyMode, 22, 22) |
      __gen_field(values->MaximumAnisotropy, 19, 21) |
      __gen_field(values->RAddressMinFilterRoundingEnable, 13, 13) |
      __gen_field(values->RAddressMagFilterRoundingEnable, 14, 14) |
      __gen_field(values->VAddressMinFilterRoundingEnable, 15, 15) |
      __gen_field(values->VAddressMagFilterRoundingEnable, 16, 16) |
      __gen_field(values->UAddressMinFilterRoundingEnable, 17, 17) |
      __gen_field(values->UAddressMagFilterRoundingEnable, 18, 18) |
      __gen_field(values->TrilinearFilterQuality, 11, 12) |
      __gen_field(values->NonnormalizedCoordinateEnable, 10, 10) |
      __gen_field(values->TCXAddressControlMode, 6, 8) |
      __gen_field(values->TCYAddressControlMode, 3, 5) |
      __gen_field(values->TCZAddressControlMode, 0, 2) |
      0;

}

/* Enum 3D_Prim_Topo_Type */
#define     _3DPRIM_POINTLIST                                  1
#define     _3DPRIM_LINELIST                                   2
#define     _3DPRIM_LINESTRIP                                  3
#define     _3DPRIM_TRILIST                                    4
#define     _3DPRIM_TRISTRIP                                   5
#define     _3DPRIM_TRIFAN                                     6
#define     _3DPRIM_QUADLIST                                   7
#define     _3DPRIM_QUADSTRIP                                  8
#define     _3DPRIM_LINELIST_ADJ                               9
#define     _3DPRIM_LISTSTRIP_ADJ                             10
#define     _3DPRIM_TRILIST_ADJ                               11
#define     _3DPRIM_TRISTRIP_ADJ                              12
#define     _3DPRIM_TRISTRIP_REVERSE                          13
#define     _3DPRIM_POLYGON                                   14
#define     _3DPRIM_RECTLIST                                  15
#define     _3DPRIM_LINELOOP                                  16
#define     _3DPRIM_POINTLIST_BF                              17
#define     _3DPRIM_LINESTRIP_CONT                            18
#define     _3DPRIM_LINESTRIP_BF                              19
#define     _3DPRIM_LINESTRIP_CONT_BF                         20
#define     _3DPRIM_TRIFAN_NOSTIPPLE                          22
#define     _3DPRIM_PATCHLIST_1                               32
#define     _3DPRIM_PATCHLIST_2                               33
#define     _3DPRIM_PATCHLIST_3                               34
#define     _3DPRIM_PATCHLIST_4                               35
#define     _3DPRIM_PATCHLIST_5                               36
#define     _3DPRIM_PATCHLIST_6                               37
#define     _3DPRIM_PATCHLIST_7                               38
#define     _3DPRIM_PATCHLIST_8                               39
#define     _3DPRIM_PATCHLIST_9                               40
#define     _3DPRIM_PATCHLIST_10                              41
#define     _3DPRIM_PATCHLIST_11                              42
#define     _3DPRIM_PATCHLIST_12                              43
#define     _3DPRIM_PATCHLIST_13                              44
#define     _3DPRIM_PATCHLIST_14                              45
#define     _3DPRIM_PATCHLIST_15                              46
#define     _3DPRIM_PATCHLIST_16                              47
#define     _3DPRIM_PATCHLIST_17                              48
#define     _3DPRIM_PATCHLIST_18                              49
#define     _3DPRIM_PATCHLIST_19                              50
#define     _3DPRIM_PATCHLIST_20                              51
#define     _3DPRIM_PATCHLIST_21                              52
#define     _3DPRIM_PATCHLIST_22                              53
#define     _3DPRIM_PATCHLIST_23                              54
#define     _3DPRIM_PATCHLIST_24                              55
#define     _3DPRIM_PATCHLIST_25                              56
#define     _3DPRIM_PATCHLIST_26                              57
#define     _3DPRIM_PATCHLIST_27                              58
#define     _3DPRIM_PATCHLIST_28                              59
#define     _3DPRIM_PATCHLIST_29                              60
#define     _3DPRIM_PATCHLIST_30                              61
#define     _3DPRIM_PATCHLIST_31                              62
#define     _3DPRIM_PATCHLIST_32                              63

/* Enum 3D_Vertex_Component_Control */
#define     VFCOMP_NOSTORE                                     0
#define     VFCOMP_STORE_SRC                                   1
#define     VFCOMP_STORE_0                                     2
#define     VFCOMP_STORE_1_FP                                  3
#define     VFCOMP_STORE_1_INT                                 4
#define     VFCOMP_STORE_VID                                   5
#define     VFCOMP_STORE_IID                                   6
#define     VFCOMP_STORE_PID                                   7

/* Enum 3D_Compare_Function */
#define     COMPAREFUNCTION_ALWAYS                             0
#define     COMPAREFUNCTION_NEVER                              1
#define     COMPAREFUNCTION_LESS                               2
#define     COMPAREFUNCTION_EQUAL                              3
#define     COMPAREFUNCTION_LEQUAL                             4
#define     COMPAREFUNCTION_GREATER                            5
#define     COMPAREFUNCTION_NOTEQUAL                           6
#define     COMPAREFUNCTION_GEQUAL                             7

/* Enum SURFACE_FORMAT */
#define     R32G32B32A32_FLOAT                                 0
#define     R32G32B32A32_SINT                                  1
#define     R32G32B32A32_UINT                                  2
#define     R32G32B32A32_UNORM                                 3
#define     R32G32B32A32_SNORM                                 4
#define     R64G64_FLOAT                                       5
#define     R32G32B32X32_FLOAT                                 6
#define     R32G32B32A32_SSCALED                               7
#define     R32G32B32A32_USCALED                               8
#define     R32G32B32A32_SFIXED                               32
#define     R64G64_PASSTHRU                                   33
#define     R32G32B32_FLOAT                                   64
#define     R32G32B32_SINT                                    65
#define     R32G32B32_UINT                                    66
#define     R32G32B32_UNORM                                   67
#define     R32G32B32_SNORM                                   68
#define     R32G32B32_SSCALED                                 69
#define     R32G32B32_USCALED                                 70
#define     R32G32B32_SFIXED                                  80
#define     R16G16B16A16_UNORM                               128
#define     R16G16B16A16_SNORM                               129
#define     R16G16B16A16_SINT                                130
#define     R16G16B16A16_UINT                                131
#define     R16G16B16A16_FLOAT                               132
#define     R32G32_FLOAT                                     133
#define     R32G32_SINT                                      134
#define     R32G32_UINT                                      135
#define     R32_FLOAT_X8X24_TYPELESS                         136
#define     X32_TYPELESS_G8X24_UINT                          137
#define     L32A32_FLOAT                                     138
#define     R32G32_UNORM                                     139
#define     R32G32_SNORM                                     140
#define     R64_FLOAT                                        141
#define     R16G16B16X16_UNORM                               142
#define     R16G16B16X16_FLOAT                               143
#define     A32X32_FLOAT                                     144
#define     L32X32_FLOAT                                     145
#define     I32X32_FLOAT                                     146
#define     R16G16B16A16_SSCALED                             147
#define     R16G16B16A16_USCALED                             148
#define     R32G32_SSCALED                                   149
#define     R32G32_USCALED                                   150
#define     R32G32_SFIXED                                    160
#define     R64_PASSTHRU                                     161
#define     B8G8R8A8_UNORM                                   192
#define     B8G8R8A8_UNORM_SRGB                              193
#define     R10G10B10A2_UNORM                                194
#define     R10G10B10A2_UNORM_SRGB                           195
#define     R10G10B10A2_UINT                                 196
#define     R10G10B10_SNORM_A2_UNORM                         197
#define     R8G8B8A8_UNORM                                   199
#define     R8G8B8A8_UNORM_SRGB                              200
#define     R8G8B8A8_SNORM                                   201
#define     R8G8B8A8_SINT                                    202
#define     R8G8B8A8_UINT                                    203
#define     R16G16_UNORM                                     204
#define     R16G16_SNORM                                     205
#define     R16G16_SINT                                      206
#define     R16G16_UINT                                      207
#define     R16G16_FLOAT                                     208
#define     B10G10R10A2_UNORM                                209
#define     B10G10R10A2_UNORM_SRGB                           210
#define     R11G11B10_FLOAT                                  211
#define     R32_SINT                                         214
#define     R32_UINT                                         215
#define     R32_FLOAT                                        216
#define     R24_UNORM_X8_TYPELESS                            217
#define     X24_TYPELESS_G8_UINT                             218
#define     L32_UNORM                                        221
#define     A32_UNORM                                        222
#define     L16A16_UNORM                                     223
#define     I24X8_UNORM                                      224
#define     L24X8_UNORM                                      225
#define     A24X8_UNORM                                      226
#define     I32_FLOAT                                        227
#define     L32_FLOAT                                        228
#define     A32_FLOAT                                        229
#define     X8B8_UNORM_G8R8_SNORM                            230
#define     A8X8_UNORM_G8R8_SNORM                            231
#define     B8X8_UNORM_G8R8_SNORM                            232
#define     B8G8R8X8_UNORM                                   233
#define     B8G8R8X8_UNORM_SRGB                              234
#define     R8G8B8X8_UNORM                                   235
#define     R8G8B8X8_UNORM_SRGB                              236
#define     R9G9B9E5_SHAREDEXP                               237
#define     B10G10R10X2_UNORM                                238
#define     L16A16_FLOAT                                     240
#define     R32_UNORM                                        241
#define     R32_SNORM                                        242
#define     R10G10B10X2_USCALED                              243
#define     R8G8B8A8_SSCALED                                 244
#define     R8G8B8A8_USCALED                                 245
#define     R16G16_SSCALED                                   246
#define     R16G16_USCALED                                   247
#define     R32_SSCALED                                      248
#define     R32_USCALED                                      249
#define     B5G6R5_UNORM                                     256
#define     B5G6R5_UNORM_SRGB                                257
#define     B5G5R5A1_UNORM                                   258
#define     B5G5R5A1_UNORM_SRGB                              259
#define     B4G4R4A4_UNORM                                   260
#define     B4G4R4A4_UNORM_SRGB                              261
#define     R8G8_UNORM                                       262
#define     R8G8_SNORM                                       263
#define     R8G8_SINT                                        264
#define     R8G8_UINT                                        265
#define     R16_UNORM                                        266
#define     R16_SNORM                                        267
#define     R16_SINT                                         268
#define     R16_UINT                                         269
#define     R16_FLOAT                                        270
#define     A8P8_UNORM_PALETTE0                              271
#define     A8P8_UNORM_PALETTE1                              272
#define     I16_UNORM                                        273
#define     L16_UNORM                                        274
#define     A16_UNORM                                        275
#define     L8A8_UNORM                                       276
#define     I16_FLOAT                                        277
#define     L16_FLOAT                                        278
#define     A16_FLOAT                                        279
#define     L8A8_UNORM_SRGB                                  280
#define     R5G5_SNORM_B6_UNORM                              281
#define     B5G5R5X1_UNORM                                   282
#define     B5G5R5X1_UNORM_SRGB                              283
#define     R8G8_SSCALED                                     284
#define     R8G8_USCALED                                     285
#define     R16_SSCALED                                      286
#define     R16_USCALED                                      287
#define     P8A8_UNORM_PALETTE0                              290
#define     P8A8_UNORM_PALETTE1                              291
#define     A1B5G5R5_UNORM                                   292
#define     A4B4G4R4_UNORM                                   293
#define     L8A8_UINT                                        294
#define     L8A8_SINT                                        295
#define     R8_UNORM                                         320
#define     R8_SNORM                                         321
#define     R8_SINT                                          322
#define     R8_UINT                                          323
#define     A8_UNORM                                         324
#define     I8_UNORM                                         325
#define     L8_UNORM                                         326
#define     P4A4_UNORM_PALETTE0                              327
#define     A4P4_UNORM_PALETTE0                              328
#define     R8_SSCALED                                       329
#define     R8_USCALED                                       330
#define     P8_UNORM_PALETTE0                                331
#define     L8_UNORM_SRGB                                    332
#define     P8_UNORM_PALETTE1                                333
#define     P4A4_UNORM_PALETTE1                              334
#define     A4P4_UNORM_PALETTE1                              335
#define     Y8_UNORM                                         336
#define     L8_UINT                                          338
#define     L8_SINT                                          339
#define     I8_UINT                                          340
#define     I8_SINT                                          341
#define     DXT1_RGB_SRGB                                    384
#define     R1_UNORM                                         385
#define     YCRCB_NORMAL                                     386
#define     YCRCB_SWAPUVY                                    387
#define     P2_UNORM_PALETTE0                                388
#define     P2_UNORM_PALETTE1                                389
#define     BC1_UNORM                                        390
#define     BC2_UNORM                                        391
#define     BC3_UNORM                                        392
#define     BC4_UNORM                                        393
#define     BC5_UNORM                                        394
#define     BC1_UNORM_SRGB                                   395
#define     BC2_UNORM_SRGB                                   396
#define     BC3_UNORM_SRGB                                   397
#define     MONO8                                            398
#define     YCRCB_SWAPUV                                     399
#define     YCRCB_SWAPY                                      400
#define     DXT1_RGB                                         401
#define     FXT1                                             402
#define     R8G8B8_UNORM                                     403
#define     R8G8B8_SNORM                                     404
#define     R8G8B8_SSCALED                                   405
#define     R8G8B8_USCALED                                   406
#define     R64G64B64A64_FLOAT                               407
#define     R64G64B64_FLOAT                                  408
#define     BC4_SNORM                                        409
#define     BC5_SNORM                                        410
#define     R16G16B16_FLOAT                                  411
#define     R16G16B16_UNORM                                  412
#define     R16G16B16_SNORM                                  413
#define     R16G16B16_SSCALED                                414
#define     R16G16B16_USCALED                                415
#define     BC6H_SF16                                        417
#define     BC7_UNORM                                        418
#define     BC7_UNORM_SRGB                                   419
#define     BC6H_UF16                                        420
#define     PLANAR_420_8                                     421
#define     R8G8B8_UNORM_SRGB                                424
#define     ETC1_RGB8                                        425
#define     ETC2_RGB8                                        426
#define     EAC_R11                                          427
#define     EAC_RG11                                         428
#define     EAC_SIGNED_R11                                   429
#define     EAC_SIGNED_RG11                                  430
#define     ETC2_SRGB8                                       431
#define     R16G16B16_UINT                                   432
#define     R16G16B16_SINT                                   433
#define     R32_SFIXED                                       434
#define     R10G10B10A2_SNORM                                435
#define     R10G10B10A2_USCALED                              436
#define     R10G10B10A2_SSCALED                              437
#define     R10G10B10A2_SINT                                 438
#define     B10G10R10A2_SNORM                                439
#define     B10G10R10A2_USCALED                              440
#define     B10G10R10A2_SSCALED                              441
#define     B10G10R10A2_UINT                                 442
#define     B10G10R10A2_SINT                                 443
#define     R64G64B64A64_PASSTHRU                            444
#define     R64G64B64_PASSTHRU                               445
#define     ETC2_RGB8_PTA                                    448
#define     ETC2_SRGB8_PTA                                   449
#define     ETC2_EAC_RGBA8                                   450
#define     ETC2_EAC_SRGB8_A8                                451
#define     R8G8B8_UINT                                      456
#define     R8G8B8_SINT                                      457
#define     RAW                                              511

/* Enum Texture Coordinate Mode */
#define     TCM_WRAP                                           0
#define     TCM_MIRROR                                         1
#define     TCM_CLAMP                                          2
#define     TCM_CUBE                                           3
#define     TCM_CLAMP_BORDER                                   4
#define     TCM_MIRROR_ONCE                                    5


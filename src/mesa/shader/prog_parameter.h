/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file prog_parameter.c
 * Program parameter lists and functions.
 * \author Brian Paul
 */

#ifndef PROG_PARAMETER_H
#define PROG_PARAMETER_H

#include "mtypes.h"


/**
 * Named program parameters 
 * Used for NV_fragment_program "DEFINE"d constants and "DECLARE"d parameters,
 * and ARB_fragment_program global state references.  For the later, Name
 * might be "state.light[0].diffuse", for example.
 */
struct gl_program_parameter
{
   const char *Name;        /**< Null-terminated string */
   enum register_file Type; /**< PROGRAM_NAMED_PARAM, CONSTANT or STATE_VAR */
   GLuint Size;             /**< Number of components (1..4) */
   /**
    * A sequence of STATE_* tokens and integers to identify GL state.
    */
   GLuint StateIndexes[6];
};


/**
 * A list of the above program_parameter instances.
 */
struct gl_program_parameter_list
{
   GLuint Size;           /**< allocated size of Parameters, ParameterValues */
   GLuint NumParameters;  /**< number of parameters in arrays */
   struct gl_program_parameter *Parameters; /**< Array [Size] */
   GLfloat (*ParameterValues)[4];        /**< Array [Size] of GLfloat[4] */
   GLbitfield StateFlags; /**< _NEW_* flags indicating which state changes
                               might invalidate ParameterValues[] */
};


extern struct gl_program_parameter_list *
_mesa_new_parameter_list(void);

extern void
_mesa_free_parameter_list(struct gl_program_parameter_list *paramList);

extern struct gl_program_parameter_list *
_mesa_clone_parameter_list(const struct gl_program_parameter_list *list);

extern GLint
_mesa_add_parameter(struct gl_program_parameter_list *paramList,
                    const char *name, const GLfloat values[4], GLuint size,
                    enum register_file type);

extern GLint
_mesa_add_named_parameter(struct gl_program_parameter_list *paramList,
                          const char *name, const GLfloat values[4]);

extern GLint
_mesa_add_named_constant(struct gl_program_parameter_list *paramList,
                         const char *name, const GLfloat values[4],
                         GLuint size);

extern GLint
_mesa_add_unnamed_constant(struct gl_program_parameter_list *paramList,
                           const GLfloat values[4], GLuint size,
                           GLuint *swizzleOut);

extern GLint
_mesa_add_uniform(struct gl_program_parameter_list *paramList,
                  const char *name, GLuint size);

extern GLint
_mesa_add_varying(struct gl_program_parameter_list *paramList,
                  const char *name, GLuint size);

extern GLint
_mesa_add_state_reference(struct gl_program_parameter_list *paramList,
                          const GLint *stateTokens);

extern GLfloat *
_mesa_lookup_parameter_value(const struct gl_program_parameter_list *paramList,
                             GLsizei nameLen, const char *name);

extern GLint
_mesa_lookup_parameter_index(const struct gl_program_parameter_list *paramList,
                             GLsizei nameLen, const char *name);

extern GLboolean
_mesa_lookup_parameter_constant(const struct gl_program_parameter_list *paramList,
                                const GLfloat v[], GLsizei vSize,
                                GLint *posOut, GLuint *swizzleOut);


#endif /* PROG_PARAMETER_H */

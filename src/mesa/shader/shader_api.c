/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 2004-2007  Brian Paul   All Rights Reserved.
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
 * \file shader_api.c
 * Implementation of GLSL-related API functions
 * \author Brian Paul
 */

/**
 * XXX things to do:
 * 1. Check that the right error code is generated for all _mesa_error() calls.
 * 2. Insert FLUSH_VERTICES calls in various places
 */


#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "program.h"
#include "prog_parameter.h"
#include "prog_print.h"
#include "prog_statevars.h"
#include "shader_api.h"

#include "slang_compile.h"
#include "slang_link.h"



/**
 * Allocate a new gl_shader_program object, initialize it.
 */
struct gl_shader_program *
_mesa_new_shader_program(GLcontext *ctx, GLuint name)
{
   struct gl_shader_program *shProg;
   shProg = CALLOC_STRUCT(gl_shader_program);
   if (shProg) {
      shProg->Type = GL_SHADER_PROGRAM;
      shProg->Name = name;
      shProg->RefCount = 1;
      shProg->Attributes = _mesa_new_parameter_list();
   }
   return shProg;
}


void
_mesa_free_shader_program_data(GLcontext *ctx,
                               struct gl_shader_program *shProg)
{
   assert(shProg->Type == GL_SHADER_PROGRAM);

   if (shProg->VertexProgram) {
      if (shProg->VertexProgram->Base.Parameters == shProg->Uniforms) {
         /* to prevent a double-free in the next call */
         shProg->VertexProgram->Base.Parameters = NULL;
      }
      _mesa_delete_program(ctx, &shProg->VertexProgram->Base);
      shProg->VertexProgram = NULL;
   }

   if (shProg->FragmentProgram) {
      if (shProg->FragmentProgram->Base.Parameters == shProg->Uniforms) {
         /* to prevent a double-free in the next call */
         shProg->FragmentProgram->Base.Parameters = NULL;
      }
      _mesa_delete_program(ctx, &shProg->FragmentProgram->Base);
      shProg->FragmentProgram = NULL;
   }


   if (shProg->Uniforms) {
      _mesa_free_parameter_list(shProg->Uniforms);
      shProg->Uniforms = NULL;
   }

   if (shProg->Varying) {
      _mesa_free_parameter_list(shProg->Varying);
      shProg->Varying = NULL;
   }
}



void
_mesa_free_shader_program(GLcontext *ctx, struct gl_shader_program *shProg)
{
   _mesa_free_shader_program_data(ctx, shProg);
   _mesa_free(shProg);
}


/**
 * Lookup a GLSL program object.
 */
struct gl_shader_program *
_mesa_lookup_shader_program(GLcontext *ctx, GLuint name)
{
   struct gl_shader_program *shProg;
   if (name) {
      shProg = (struct gl_shader_program *)
         _mesa_HashLookup(ctx->Shared->ShaderObjects, name);
      /* Note that both gl_shader and gl_shader_program objects are kept
       * in the same hash table.  Check the object's type to be sure it's
       * what we're expecting.
       */
      if (shProg && shProg->Type != GL_SHADER_PROGRAM) {
         return NULL;
      }
      return shProg;
   }
   return NULL;
}


/**
 * Allocate a new gl_shader object, initialize it.
 */
struct gl_shader *
_mesa_new_shader(GLcontext *ctx, GLuint name, GLenum type)
{
   struct gl_shader *shader;
   assert(type == GL_FRAGMENT_SHADER || type == GL_VERTEX_SHADER);
   shader = CALLOC_STRUCT(gl_shader);
   if (shader) {
      shader->Type = type;
      shader->Name = name;
      shader->RefCount = 1;
   }
   return shader;
}


void
_mesa_free_shader(GLcontext *ctx, struct gl_shader *sh)
{
   GLuint i;
   if (sh->Source)
      _mesa_free((void *) sh->Source);
   if (sh->InfoLog)
      _mesa_free(sh->InfoLog);
   for (i = 0; i < sh->NumPrograms; i++) {
      assert(sh->Programs[i]);
      _mesa_delete_program(ctx, sh->Programs[i]);
   }
   if (sh->Programs)
      _mesa_free(sh->Programs);
   _mesa_free(sh);
}


/**
 * Lookup a GLSL shader object.
 */
struct gl_shader *
_mesa_lookup_shader(GLcontext *ctx, GLuint name)
{
   if (name) {
      struct gl_shader *sh = (struct gl_shader *)
         _mesa_HashLookup(ctx->Shared->ShaderObjects, name);
      /* Note that both gl_shader and gl_shader_program objects are kept
       * in the same hash table.  Check the object's type to be sure it's
       * what we're expecting.
       */
      if (sh && sh->Type == GL_SHADER_PROGRAM) {
         assert(sh->Type == GL_VERTEX_SHADER ||
                sh->Type == GL_FRAGMENT_SHADER);
         return NULL;
      }
      return sh;
   }
   return NULL;
}


void
_mesa_init_shader_state(GLcontext * ctx)
{
   ctx->Shader._FragmentShaderPresent = GL_FALSE;
   ctx->Shader._VertexShaderPresent = GL_FALSE;
}





/**
 * Copy string from <src> to <dst>, up to maxLength characters, returning
 * length of <dst> in <length>.
 * \param src  the strings source
 * \param maxLength  max chars to copy
 * \param length  returns number of chars copied
 * \param dst  the string destination
 */
static void
copy_string(GLchar *dst, GLsizei maxLength, GLsizei *length, const GLchar *src)
{
   GLsizei len;
   for (len = 0; len < maxLength - 1 && src && src[len]; len++)
      dst[len] = src[len];
   if (maxLength > 0)
      dst[len] = 0;
   if (length)
      *length = len;
}




/**
 * Called via ctx->Driver.AttachShader()
 */
void
_mesa_attach_shader(GLcontext *ctx, GLuint program, GLuint shader)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);
   struct gl_shader *sh = _mesa_lookup_shader(ctx, shader);
   const GLuint n = shProg->NumShaders;
   GLuint i;

   if (!shProg || !sh) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glAttachShader(bad program or shader name)");
      return;
   }

   for (i = 0; i < n; i++) {
      if (shProg->Shaders[i] == sh) {
         /* already attached */
         return;
      }
   }

   /* grow list */
   shProg->Shaders = (struct gl_shader **)
      _mesa_realloc(shProg->Shaders,
                    n * sizeof(struct gl_shader *),
                    (n + 1) * sizeof(struct gl_shader *));
   if (!shProg->Shaders) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glAttachShader");
      return;
   }

   /* append */
   shProg->Shaders[n] = sh;
   sh->RefCount++;
   shProg->NumShaders++;
}


void
_mesa_bind_attrib_location(GLcontext *ctx, GLuint program, GLuint index,
                           const GLchar *name)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);
   GLint i;
   GLint oldIndex;

   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindAttribLocation(program)");
      return;
   }

   if (!name)
      return;

   if (strncmp(name, "gl_", 3) == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindAttribLocation(illegal name)");
      return;
   }

   oldIndex = _mesa_get_attrib_location(ctx, program, name);

   /* this will replace the current value if it's already in the list */
   i = _mesa_add_attribute(shProg->Attributes, name, index);
   if (i < 0) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindAttribLocation");
   }

   if (shProg->VertexProgram && oldIndex >= 0) {
      _slang_remap_attribute(&shProg->VertexProgram->Base, oldIndex, index);
   }

   printf("===== post BindAttrib:\n");
   _mesa_print_program(&shProg->VertexProgram->Base);

}


GLuint
_mesa_create_shader(GLcontext *ctx, GLenum type)
{
   struct gl_shader *sh;
   GLuint name;

   name = _mesa_HashFindFreeKeyBlock(ctx->Shared->ShaderObjects, 1);

   switch (type) {
   case GL_FRAGMENT_SHADER:
   case GL_VERTEX_SHADER:
      sh = _mesa_new_shader(ctx, name, type);
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "CreateShader(type)");
      return 0;
   }

   _mesa_HashInsert(ctx->Shared->ShaderObjects, name, sh);

   return name;
}


GLuint 
_mesa_create_program(GLcontext *ctx)
{
   GLuint name;
   struct gl_shader_program *shProg;

   name = _mesa_HashFindFreeKeyBlock(ctx->Shared->ShaderObjects, 1);
   shProg = _mesa_new_shader_program(ctx, name);

   _mesa_HashInsert(ctx->Shared->ShaderObjects, name, shProg);

   return name;
}


void
_mesa_delete_program2(GLcontext *ctx, GLuint name)
{
   struct gl_shader_program *shProg;

   shProg = _mesa_lookup_shader_program(ctx, name);
   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteProgram(name)");
      return;
   }

   /* always remove from hash table */
   _mesa_HashRemove(ctx->Shared->ShaderObjects, name);

   shProg->DeletePending = GL_TRUE;

   /* decrement refcount, delete if zero */
   shProg->RefCount--;
   if (shProg->RefCount <= 0) {
      _mesa_free_shader_program(ctx, shProg);
   }
}


void
_mesa_delete_shader(GLcontext *ctx, GLuint shader)
{
   struct gl_shader *sh = _mesa_lookup_shader(ctx, shader);
   if (!sh) {
      return;
   }

   sh->DeletePending = GL_TRUE;
   sh->RefCount--;
   if (sh->RefCount <= 0) {
      _mesa_free_shader(ctx, sh);
   }
}


void
_mesa_detach_shader(GLcontext *ctx, GLuint program, GLuint shader)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);
   const GLuint n = shProg->NumShaders;
   GLuint i, j;

   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glDetachShader(bad program or shader name)");
      return;
   }

   for (i = 0; i < n; i++) {
      if (shProg->Shaders[i]->Name == shader) {
         struct gl_shader **newList;
         /* found it */

         shProg->Shaders[i]->RefCount--;

         /* alloc new, smaller array */
         newList = (struct gl_shader **)
            _mesa_malloc((n - 1) * sizeof(struct gl_shader *));
         if (!newList) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glDetachShader");
            return;
         }
         for (j = 0; j < i; j++) {
            newList[j] = shProg->Shaders[j];
         }
         while (++i < n)
            newList[j++] = shProg->Shaders[i];
         _mesa_free(shProg->Shaders);

         /* XXX refcounting! */

         shProg->Shaders = newList;
         return;
      }
   }

   /* not found */
   _mesa_error(ctx, GL_INVALID_VALUE,
               "glDetachShader(shader not found)");
}


void
_mesa_get_active_attrib(GLcontext *ctx, GLuint program, GLuint index,
                        GLsizei maxLength, GLsizei *length, GLint *size,
                        GLenum *type, GLchar *nameOut)
{
   static const GLenum vec_types[] = {
      GL_FLOAT, GL_FLOAT_VEC2, GL_FLOAT_VEC3, GL_FLOAT_VEC4
   };
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);
   GLint sz;

   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveUniform");
      return;
   }

   if (!shProg->Attributes || index >= shProg->Attributes->NumParameters) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveUniform(index)");
      return;
   }

   copy_string(nameOut, maxLength, length,
               shProg->Attributes->Parameters[index].Name);
   sz = shProg->Attributes->Parameters[index].Size;
   if (size)
      *size = sz;
   if (type)
      *type = vec_types[sz]; /* XXX this is a temporary hack */
}


/**
 * Called via ctx->Driver.GetActiveUniform().
 */
void
_mesa_get_active_uniform(GLcontext *ctx, GLuint program, GLuint index,
                         GLsizei maxLength, GLsizei *length, GLint *size,
                         GLenum *type, GLchar *nameOut)
{
   static const GLenum vec_types[] = {
      GL_FLOAT, GL_FLOAT_VEC2, GL_FLOAT_VEC3, GL_FLOAT_VEC4
   };
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);
   GLint sz;

   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveUniform");
      return;
   }

   if (!shProg->Uniforms || index >= shProg->Uniforms->NumParameters) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveUniform(index)");
      return;
   }

   copy_string(nameOut, maxLength, length,
               shProg->Uniforms->Parameters[index].Name);
   sz = shProg->Uniforms->Parameters[index].Size;
   if (size)
      *size = sz;
   if (type)
      *type = vec_types[sz]; /* XXX this is a temporary hack */
}


/**
 * Called via ctx->Driver.GetAttachedShaders().
 */
void
_mesa_get_attached_shaders(GLcontext *ctx, GLuint program, GLsizei maxCount,
                           GLsizei *count, GLuint *obj)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);
   if (shProg) {
      GLuint i;
      for (i = 0; i < maxCount && i < shProg->NumShaders; i++) {
         obj[i] = shProg->Shaders[i]->Name;
      }
      if (count)
         *count = i;
   }
   else {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetAttachedShaders");
   }
}


GLint
_mesa_get_attrib_location(GLcontext *ctx, GLuint program,
                          const GLchar *name)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);

   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetAttribLocation");
      return -1;
   }

   if (!shProg->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetAttribLocation(program not linked)");
      return -1;
   }

   if (!name)
      return -1;

   if (shProg->Attributes) {
      GLint i = _mesa_lookup_parameter_index(shProg->Attributes, -1, name);
      if (i >= 0) {
         return shProg->Attributes->Parameters[i].StateIndexes[1];
      }
   }
   return -1;
}


GLuint
_mesa_get_handle(GLcontext *ctx, GLenum pname)
{
#if 0
   GET_CURRENT_CONTEXT(ctx);

   switch (pname) {
   case GL_PROGRAM_OBJECT_ARB:
      {
         struct gl2_program_intf **pro = ctx->Shader.CurrentProgram;

         if (pro != NULL)
            return (**pro)._container._generic.
               GetName((struct gl2_generic_intf **) (pro));
      }
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetHandleARB");
   }
#endif
   return 0;
}


void
_mesa_get_programiv(GLcontext *ctx, GLuint program,
                    GLenum pname, GLint *params)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);

   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetProgramiv(program)");
      return;
   }

   switch (pname) {
   case GL_DELETE_STATUS:
      *params = shProg->DeletePending;
      break; 
   case GL_LINK_STATUS:
      *params = shProg->LinkStatus;
      break;
   case GL_VALIDATE_STATUS:
      *params = shProg->Validated;
      break;
   case GL_INFO_LOG_LENGTH:
      *params = shProg->InfoLog ? strlen(shProg->InfoLog) : 0;
      break;
   case GL_ATTACHED_SHADERS:
      *params = shProg->NumShaders;
      break;
   case GL_ACTIVE_ATTRIBUTES:
      *params = shProg->Attributes ? shProg->Attributes->NumParameters : 0;
      break;
   case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
      *params = _mesa_parameter_longest_name(shProg->Attributes);
      break;
   case GL_ACTIVE_UNIFORMS:
      *params = shProg->Uniforms ? shProg->Uniforms->NumParameters : 0;
      break;
   case GL_ACTIVE_UNIFORM_MAX_LENGTH:
      *params = _mesa_parameter_longest_name(shProg->Uniforms);
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramiv(pname)");
      return;
   }
}


void
_mesa_get_shaderiv(GLcontext *ctx, GLuint name, GLenum pname, GLint *params)
{
   struct gl_shader *shader = _mesa_lookup_shader(ctx, name);

   if (!shader) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetShaderiv(shader)");
      return;
   }

   switch (pname) {
   case GL_SHADER_TYPE:
      *params = shader->Type;
      break;
   case GL_DELETE_STATUS:
      *params = shader->DeletePending;
      break;
   case GL_COMPILE_STATUS:
      *params = shader->CompileStatus;
      break;
   case GL_INFO_LOG_LENGTH:
      *params = shader->InfoLog ? strlen(shader->InfoLog) : 0;
      break;
   case GL_SHADER_SOURCE_LENGTH:
      *params = shader->Source ? strlen((char *) shader->Source) : 0;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetShaderiv(pname)");
      return;
   }
}


void
_mesa_get_program_info_log(GLcontext *ctx, GLuint program, GLsizei bufSize,
                           GLsizei *length, GLchar *infoLog)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);
   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetProgramInfoLog(program)");
      return;
   }
   copy_string(infoLog, bufSize, length, shProg->InfoLog);
}


void
_mesa_get_shader_info_log(GLcontext *ctx, GLuint shader, GLsizei bufSize,
                          GLsizei *length, GLchar *infoLog)
{
   struct gl_shader *sh = _mesa_lookup_shader(ctx, shader);
   if (!sh) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetShaderInfoLog(shader)");
      return;
   }
   copy_string(infoLog, bufSize, length, sh->InfoLog);
}


/**
 * Called via ctx->Driver.GetShaderSource().
 */
void
_mesa_get_shader_source(GLcontext *ctx, GLuint shader, GLsizei maxLength,
                        GLsizei *length, GLchar *sourceOut)
{
   struct gl_shader *sh = _mesa_lookup_shader(ctx, shader);
   if (!sh) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetShaderSource(shader)");
      return;
   }
   copy_string(sourceOut, maxLength, length, sh->Source);
}


/**
 * Called via ctx->Driver.GetUniformfv().
 */
void
_mesa_get_uniformfv(GLcontext *ctx, GLuint program, GLint location,
                    GLfloat *params)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);
   if (shProg) {
      GLuint i;
      if (location >= 0 && location < shProg->Uniforms->NumParameters) {
         for (i = 0; i < shProg->Uniforms->Parameters[location].Size; i++) {
            params[i] = shProg->Uniforms->ParameterValues[location][i];
         }
      }
      else {
         _mesa_error(ctx, GL_INVALID_VALUE, "glGetUniformfv(location)");
      }
   }
   else {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetUniformfv(program)");
   }
}


/**
 * Called via ctx->Driver.GetUniformLocation().
 */
GLint
_mesa_get_uniform_location(GLcontext *ctx, GLuint program, const GLchar *name)
{
   if (ctx->Shader.CurrentProgram) {
      const struct gl_shader_program *shProg = ctx->Shader.CurrentProgram;
      GLuint loc;
      for (loc = 0; loc < shProg->Uniforms->NumParameters; loc++) {
         const struct gl_program_parameter *u
            = shProg->Uniforms->Parameters + loc;
         /* XXX this is a temporary simplification / short-cut.
          * We need to handle things like "e.c[0].b" as seen in the
          * GLSL orange book, page 189.
          */
         if ((u->Type == PROGRAM_UNIFORM ||
              u->Type == PROGRAM_SAMPLER) && !strcmp(u->Name, name)) {
            return loc;
         }
      }
   }
   return -1;

}


GLboolean
_mesa_is_program(GLcontext *ctx, GLuint name)
{
   struct gl_shader_program *shProg = _mesa_lookup_shader_program(ctx, name);
   return shProg ? GL_TRUE : GL_FALSE;
}


GLboolean
_mesa_is_shader(GLcontext *ctx, GLuint name)
{
   struct gl_shader *shader = _mesa_lookup_shader(ctx, name);
   return shader ? GL_TRUE : GL_FALSE;
}



/**
 * Called via ctx->Driver.ShaderSource()
 */
void
_mesa_shader_source(GLcontext *ctx, GLuint shader, const GLchar *source)
{
   struct gl_shader *sh = _mesa_lookup_shader(ctx, shader);
   if (!sh) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glShaderSource(shaderObj)");
      return;
   }

   /* free old shader source string and install new one */
   if (sh->Source) {
      _mesa_free((void *) sh->Source);
   }
   sh->Source = source;
   sh->CompileStatus = GL_FALSE;
}


/**
 * Called via ctx->Driver.CompileShader()
 */
void
_mesa_compile_shader(GLcontext *ctx, GLuint shaderObj)
{
   struct gl_shader *sh = _mesa_lookup_shader(ctx, shaderObj);

   if (!sh) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glCompileShader(shaderObj)");
      return;
   }

   sh->CompileStatus = _slang_compile(ctx, sh);
}


/**
 * Called via ctx->Driver.LinkProgram()
 */
void
_mesa_link_program(GLcontext *ctx, GLuint program)
{
   struct gl_shader_program *shProg;

   shProg = _mesa_lookup_shader_program(ctx, program);
   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glLinkProgram(program)");
      return;
   }

   _slang_link2(ctx, program, shProg);
}


/**
 * Called via ctx->Driver.UseProgram()
 */
void
_mesa_use_program(GLcontext *ctx, GLuint program)
{
   /* unbind old */
   if (ctx->Shader.CurrentProgram) {
      ctx->Shader.CurrentProgram->RefCount--;
      if (ctx->Shader.CurrentProgram->RefCount <= 0) {
         _mesa_free_shader_program(ctx, ctx->Shader.CurrentProgram);
      }
      ctx->Shader.CurrentProgram = NULL;
   }

   /* XXXX need to handle reference counting here! */
   if (program) {
      struct gl_shader_program *shProg;
      shProg = _mesa_lookup_shader_program(ctx, program);
      if (!shProg) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glUseProgramObjectARB(programObj)");
         return;
      }
      ctx->Shader.CurrentProgram = shProg;
      shProg->RefCount++;
   }
   else {
      /* don't use a shader program */
      ctx->Shader.CurrentProgram = NULL;
   }      
}


/**
 * Called via ctx->Driver.Uniform().
 */
void
_mesa_uniform(GLcontext *ctx, GLint location, GLsizei count,
              const GLvoid *values, GLenum type)
{
   struct gl_shader_program *shProg = ctx->Shader.CurrentProgram;
   GLfloat *uniformVal;

   if (!shProg || !shProg->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glUniform(program not linked)");
      return;
   }

   if (location < 0 || location >= shProg->Uniforms->NumParameters) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glUniform(location)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   uniformVal = shProg->Uniforms->ParameterValues[location];

   if (type == GL_INT ||
       type == GL_INT_VEC2 ||
       type == GL_INT_VEC3 ||
       type == GL_INT_VEC4) {
      const GLint *iValues = (const GLint *) values;
      switch (type) {
      case GL_INT_VEC4:
         uniformVal[3] = (GLfloat) iValues[3];
         /* fall-through */
      case GL_INT_VEC3:
         uniformVal[2] = (GLfloat) iValues[2];
         /* fall-through */
      case GL_INT_VEC2:
         uniformVal[1] = (GLfloat) iValues[1];
         /* fall-through */
      case GL_INT:
         uniformVal[0] = (GLfloat) iValues[0];
         break;
      default:
         _mesa_problem(ctx, "Invalid type in _mesa_uniform");
         return;
      }
   }
   else {
      const GLfloat *fValues = (const GLfloat *) values; /* XXX */
      switch (type) {
      case GL_FLOAT_VEC4:
         uniformVal[3] = fValues[3];
         /* fall-through */
      case GL_FLOAT_VEC3:
         uniformVal[2] = fValues[2];
         /* fall-through */
      case GL_FLOAT_VEC2:
         uniformVal[1] = fValues[1];
         /* fall-through */
      case GL_FLOAT:
         uniformVal[0] = fValues[0];
         break;
      default:
         _mesa_problem(ctx, "Invalid type in _mesa_uniform");
         return;
      }
   }

   if (shProg->Uniforms->Parameters[location].Type == PROGRAM_SAMPLER) {
      _slang_resolve_samplers(shProg, &shProg->VertexProgram->Base);
      _slang_resolve_samplers(shProg, &shProg->FragmentProgram->Base);
      FLUSH_VERTICES(ctx, _NEW_TEXTURE);
   }
}


/**
 * Called by ctx->Driver.UniformMatrix().
 */
void
_mesa_uniform_matrix(GLcontext *ctx, GLint cols, GLint rows,
                     GLenum matrixType, GLint location, GLsizei count,
                     GLboolean transpose, const GLfloat *values)
{
   struct gl_shader_program *shProg = ctx->Shader.CurrentProgram;
   if (!shProg || !shProg->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
         "glUniformMatrix(program not linked)");
      return;
   }
   if (location < 0 || location >= shProg->Uniforms->NumParameters) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glUniformMatrix(location)");
      return;
   }
   if (values == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glUniformMatrix");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   /*
    * Note: the _columns_ of a matrix are stored in program registers, not
    * the rows.
    */
   /* XXXX need to test 3x3 and 2x2 matrices... */
   if (transpose) {
      GLuint row, col;
      for (col = 0; col < cols; col++) {
         GLfloat *v = shProg->Uniforms->ParameterValues[location + col];
         for (row = 0; row < rows; row++) {
            v[row] = values[col * rows + row];
         }
      }
   }
   else {
      GLuint row, col;
      for (col = 0; col < cols; col++) {
         GLfloat *v = shProg->Uniforms->ParameterValues[location + col];
         for (row = 0; row < rows; row++) {
            v[row] = values[row * cols + col];
         }
      }
   }
}


void
_mesa_validate_program(GLcontext *ctx, GLuint program)
{
   struct gl_shader_program *shProg;
   shProg = _mesa_lookup_shader_program(ctx, program);
   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glValidateProgram(program)");
      return;
   }
   /* XXX temporary */
   shProg->Validated = GL_TRUE;

   /* From the GL spec:
     any two active samplers in the current program object are of
     different types, but refer to the same texture image unit,

     any active sampler in the current program object refers to a texture
     image unit where fixed-function fragment processing accesses a
     texture target that does not match the sampler type, or 

     the sum of the number of active samplers in the program and the
     number of texture image units enabled for fixed-function fragment
     processing exceeds the combined limit on the total number of texture
     image units allowed.
   */
}

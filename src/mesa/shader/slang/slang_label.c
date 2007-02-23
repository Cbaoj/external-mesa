

/**
 * Functions for managing instruction labels.
 * Basically, this is used to manage the problem of forward branches where
 * we have a branch instruciton but don't know the target address yet.
 */


#include "slang_label.h"


slang_label *
_slang_label_new(const char *name)
{
   slang_label *l = (slang_label *) _mesa_calloc(sizeof(slang_label));
   if (l) {
      l->Name = _mesa_strdup(name);
      l->Location = -1;
   }
   return l;
}

void
_slang_label_delete(slang_label *l)
{
   if (l->Name)
      _mesa_free(l->Name);
   if (l->References)
      _mesa_free(l->References);
   _mesa_free(l);
}


void
_slang_label_add_reference(slang_label *l, GLuint inst)
{
   const GLuint oldSize = l->NumReferences * sizeof(GLuint);
   assert(l->Location < 0);
   l->References = _mesa_realloc(l->References,
                                 oldSize, oldSize + sizeof(GLuint));
   if (l->References) {
      l->References[l->NumReferences] = inst;
      l->NumReferences++;
   }
}


GLint
_slang_label_get_location(const slang_label *l)
{
   return l->Location;
}


void
_slang_label_set_location(slang_label *l, GLint location,
                          struct gl_program *prog)
{
   GLuint i;

   assert(l->Location < 0);
   assert(location >= 0);

   l->Location = location;

   /* for the instructions that were waiting to learn the label's location: */
   for (i = 0; i < l->NumReferences; i++) {
      const GLuint j = l->References[i];
      prog->Instructions[j].BranchTarget = location;
   }

   if (l->References) {
      _mesa_free(l->References);
      l->References = NULL;
   }
}

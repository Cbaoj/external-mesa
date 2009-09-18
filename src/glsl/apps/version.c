/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../pp/sl_pp_public.h"


int
main(int argc,
     char *argv[])
{
   FILE *in;
   long size;
   char *inbuf;
   struct sl_pp_purify_options options;
   char *outbuf;
   struct sl_pp_context *context;
   struct sl_pp_token_info *tokens;
   unsigned int version;
   unsigned int tokens_eaten;
   FILE *out;

   if (argc != 3) {
      return 1;
   }

   in = fopen(argv[1], "rb");
   if (!in) {
      return 1;
   }

   fseek(in, 0, SEEK_END);
   size = ftell(in);
   fseek(in, 0, SEEK_SET);

   out = fopen(argv[2], "wb");
   if (!out) {
      fclose(in);
      return 1;
   }

   inbuf = malloc(size + 1);
   if (!inbuf) {
      fprintf(out, "$OOMERROR\n");

      fclose(out);
      fclose(in);
      return 1;
   }

   if (fread(inbuf, 1, size, in) != size) {
      fprintf(out, "$READERROR\n");

      free(inbuf);
      fclose(out);
      fclose(in);
      return 1;
   }
   inbuf[size] = '\0';

   fclose(in);

   memset(&options, 0, sizeof(options));

   if (sl_pp_purify(inbuf, &options, &outbuf)) {
      fprintf(out, "$PURIFYERROR\n");

      free(inbuf);
      fclose(out);
      return 1;
   }

   free(inbuf);

   context = sl_pp_context_create();
   if (!context) {
      fprintf(out, "$CONTEXERROR\n");

      free(outbuf);
      fclose(out);
      return 1;
   }

   if (sl_pp_tokenise(context, outbuf, &tokens)) {
      fprintf(out, "$ERROR: `%s'\n", sl_pp_context_error_message(context));

      sl_pp_context_destroy(context);
      free(outbuf);
      fclose(out);
      return 1;
   }

   free(outbuf);

   if (sl_pp_version(context, tokens, &version, &tokens_eaten)) {
      fprintf(out, "$ERROR: `%s'\n", sl_pp_context_error_message(context));

      sl_pp_context_destroy(context);
      free(tokens);
      fclose(out);
      return -1;
   }

   sl_pp_context_destroy(context);
   free(tokens);

   fprintf(out,
           "%u\n%u\n",
           version,
           tokens_eaten);

   fclose(out);

   return 0;
}

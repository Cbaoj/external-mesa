/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#include <stdarg.h>

#ifdef WIN32
#include <windows.h>
#include <winddi.h>
#else
#include <stdio.h>
#include <stdlib.h>
#endif

#include "pipe/p_debug.h" 
#include "pipe/p_compiler.h" 


#ifdef WIN32
static INLINE void 
rpl_EngDebugPrint(const char *format, ...)
{
   va_list ap;
   va_start(ap, format);
   EngDebugPrint("", (PCHAR)format, ap);
   va_end(ap);
}

int rpl_vsnprintf(char *, size_t, const char *, va_list);
#endif


void debug_vprintf(const char *format, va_list ap)
{
#ifdef WIN32
#ifndef WINCE
   /* EngDebugPrint does not handle float point arguments, so we need to use
    * our own vsnprintf implementation */
   char buf[512 + 1];
   rpl_vsnprintf(buf, sizeof(buf), format, ap);
   rpl_EngDebugPrint("%s", buf);
#else
   /* TODO: Implement debug print for WINCE */
#endif
#else
   vfprintf(stderr, format, ap);
#endif
}


void debug_printf(const char *format, ...)
{
   va_list ap;
   va_start(ap, format);
   debug_vprintf(format, ap);
   va_end(ap);
}


/* TODO: implement a debug_abort that calls EngBugCheckEx on WIN32 */


static INLINE void debug_break(void) 
{
#if (defined(__i386__) || defined(__386__)) && defined(__GNUC__)
   __asm("int3");
#elif (defined(__i386__) || defined(__386__)) && defined(__MSC__)
   _asm {int 3};
#elif defined(WIN32) && !defined(WINCE)
   EngDebugBreak();
#else
   abort();
#endif
}


void debug_assert_fail(const char *expr, const char *file, unsigned line) 
{
   debug_printf("%s:%i: Assertion `%s' failed.\n", file, line, expr);
   debug_break();
}

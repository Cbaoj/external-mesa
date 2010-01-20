/**
 * Functions for choosing and opening/loading device drivers.
 */


#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "eglconfig.h"
#include "eglcontext.h"
#include "egldefines.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglglobals.h"
#include "egllog.h"
#include "eglmisc.h"
#include "eglmode.h"
#include "eglscreen.h"
#include "eglstring.h"
#include "eglsurface.h"

#if defined(_EGL_PLATFORM_POSIX)
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#endif


/**
 * Wrappers for dlopen/dlclose()
 */
#if defined(_EGL_PLATFORM_WINDOWS)


/* XXX Need to decide how to do dynamic name lookup on Windows */
static const char DefaultDriverName[] = "TBD";

typedef HMODULE lib_handle;

static HMODULE
open_library(const char *filename)
{
   return LoadLibrary(filename);
}

static void
close_library(HMODULE lib)
{
   FreeLibrary(lib);
}


static const char *
library_suffix(void)
{
   return "dll";
}


#elif defined(_EGL_PLATFORM_POSIX)


static const char DefaultDriverName[] = "egl_softpipe";

typedef void * lib_handle;

static void *
open_library(const char *filename)
{
   return dlopen(filename, RTLD_LAZY);
}

static void
close_library(void *lib)
{
   dlclose(lib);
}


static const char *
library_suffix(void)
{
   return "so";
}


#else /* _EGL_PLATFORM_NO_OS */

static const char DefaultDriverName[] = "builtin";

typedef void *lib_handle;

static INLINE void *
open_library(const char *filename)
{
   return (void *) filename;
}

static INLINE void
close_library(void *lib)
{
}


static const char *
library_suffix(void)
{
   return NULL;
}


#endif


/**
 * Open the named driver and find its bootstrap function: _eglMain().
 */
static _EGLMain_t
_eglOpenLibrary(const char *driverPath, lib_handle *handle)
{
   lib_handle lib;
   _EGLMain_t mainFunc = NULL;
   const char *error = "unknown error";

   assert(driverPath);

   _eglLog(_EGL_DEBUG, "dlopen(%s)", driverPath);
   lib = open_library(driverPath);

#if defined(_EGL_PLATFORM_WINDOWS)
   /* XXX untested */
   if (lib)
      mainFunc = (_EGLMain_t) GetProcAddress(lib, "_eglMain");
#elif defined(_EGL_PLATFORM_POSIX)
   if (lib) {
      mainFunc = (_EGLMain_t) dlsym(lib, "_eglMain");
      if (!mainFunc)
         error = dlerror();
   }
   else {
      error = dlerror();
   }
#else /* _EGL_PLATFORM_NO_OS */
   /* must be the default driver name */
   if (strcmp(driverPath, DefaultDriverName) == 0)
      mainFunc = (_EGLMain_t) _eglMain;
   else
      error = "not builtin driver";
#endif

   if (!lib) {
      _eglLog(_EGL_WARNING, "Could not open driver %s (%s)",
              driverPath, error);
      if (!getenv("EGL_DRIVER"))
         _eglLog(_EGL_WARNING,
                 "The driver can be overridden by setting EGL_DRIVER");
      return NULL;
   }

   if (!mainFunc) {
      _eglLog(_EGL_WARNING, "_eglMain not found in %s (%s)",
              driverPath, error);
      if (lib)
         close_library(lib);
      return NULL;
   }

   *handle = lib;
   return mainFunc;
}


/**
 * Load the named driver.
 */
static _EGLDriver *
_eglLoadDriver(const char *path, const char *args)
{
   _EGLMain_t mainFunc;
   lib_handle lib;
   _EGLDriver *drv = NULL;

   mainFunc = _eglOpenLibrary(path, &lib);
   if (!mainFunc)
      return NULL;

   drv = mainFunc(args);
   if (!drv) {
      if (lib)
         close_library(lib);
      return NULL;
   }

   if (!drv->Name) {
      _eglLog(_EGL_WARNING, "Driver loaded from %s has no name", path);
      drv->Name = "UNNAMED";
   }

   drv->Path = _eglstrdup(path);
   drv->Args = (args) ? _eglstrdup(args) : NULL;
   if (!drv->Path || (args && !drv->Args)) {
      if (drv->Path)
         free((char *) drv->Path);
      if (drv->Args)
         free((char *) drv->Args);
      drv->Unload(drv);
      if (lib)
         close_library(lib);
      return NULL;
   }

   drv->LibHandle = lib;

   return drv;
}


/**
 * Match a display to a preloaded driver.
 *
 * The matching is done by finding the driver with the highest score.
 */
static _EGLDriver *
_eglMatchDriver(_EGLDisplay *dpy)
{
   _EGLDriver *best_drv = NULL;
   EGLint best_score = -1, i;

   for (i = 0; i < _eglGlobal.NumDrivers; i++) {
      _EGLDriver *drv = _eglGlobal.Drivers[i];
      EGLint score;

      score = (drv->Probe) ? drv->Probe(drv, dpy) : 0;
      if (score > best_score) {
         if (best_drv) {
            _eglLog(_EGL_DEBUG, "driver %s has higher score than %s",
                  drv->Name, best_drv->Name);
         }

         best_drv = drv;
         best_score = score;
         /* perfect match */
         if (score >= 100)
            break;
      }
   }

   return best_drv;
}


/**
 * Open a preloaded driver.
 */
_EGLDriver *
_eglOpenDriver(_EGLDisplay *dpy)
{
   _EGLDriver *drv = _eglMatchDriver(dpy);
   return drv;
}


/**
 * Close a preloaded driver.
 */
EGLBoolean
_eglCloseDriver(_EGLDriver *drv, _EGLDisplay *dpy)
{
   return EGL_TRUE;
}


/**
 * Preload a user driver.
 *
 * A user driver can be specified by EGL_DRIVER.
 */
static EGLBoolean
_eglPreloadUserDriver(void)
{
#if defined(_EGL_PLATFORM_POSIX) || defined(_EGL_PLATFORM_WINDOWS)
   _EGLDriver *drv;
   char *env, *path;
   const char *suffix, *p;

   env = getenv("EGL_DRIVER");
   if (!env)
      return EGL_FALSE;

   path = env;
   suffix = library_suffix();

   /* append suffix if there isn't */
   p = strrchr(path, '.');
   if (!p && suffix) {
      size_t len = strlen(path);
      char *tmp = malloc(len + strlen(suffix) + 2);
      if (tmp) {
         memcpy(tmp, path, len);
         tmp[len++] = '.';
         tmp[len] = '\0';
         strcat(tmp + len, suffix);

         path = tmp;
      }
   }

   drv = _eglLoadDriver(path, NULL);
   if (path != env)
      free(path);
   if (!drv)
      return EGL_FALSE;

   _eglGlobal.Drivers[_eglGlobal.NumDrivers++] = drv;

   return EGL_TRUE;
#else /* _EGL_PLATFORM_POSIX || _EGL_PLATFORM_WINDOWS */
   return EGL_FALSE;
#endif
}


/**
 * Preload display drivers.
 *
 * Display drivers are a set of drivers that support a certain display system.
 * The display system may be specified by EGL_DISPLAY.
 *
 * FIXME This makes libEGL a memory hog if an user driver is not specified and
 * there are many display drivers.
 */
static EGLBoolean
_eglPreloadDisplayDrivers(void)
{
#if defined(_EGL_PLATFORM_POSIX)
   const char *dpy, *suffix;
   char path[1024], prefix[32];
   DIR *dirp;
   struct dirent *dirent;

   dpy = getenv("EGL_DISPLAY");
   if (!dpy || !dpy[0])
      dpy = _EGL_DEFAULT_DISPLAY;
   if (!dpy || !dpy[0])
      return EGL_FALSE;

   snprintf(prefix, sizeof(prefix), "egl_%s_", dpy);
   suffix = library_suffix();

   dirp = opendir(_EGL_DRIVER_SEARCH_DIR);
   if (!dirp)
      return EGL_FALSE;

   while ((dirent = readdir(dirp))) {
      _EGLDriver *drv;
      const char *p;

      /* match the prefix */
      if (strncmp(dirent->d_name, prefix, strlen(prefix)) != 0)
         continue;

      /* match the suffix */
      p = strrchr(dirent->d_name, '.');
      if ((p && !suffix) || (!p && suffix))
         continue;
      else if (p && suffix && strcmp(p + 1, suffix) != 0)
         continue;

      snprintf(path, sizeof(path),
            _EGL_DRIVER_SEARCH_DIR"/%s", dirent->d_name);

      drv = _eglLoadDriver(path, NULL);
      if (drv)
         _eglGlobal.Drivers[_eglGlobal.NumDrivers++] = drv;
   }

   closedir(dirp);

   return (_eglGlobal.NumDrivers > 0);
#else /* _EGL_PLATFORM_POSIX */
   return EGL_FALSE;
#endif
}


/**
 * Preload the default driver.
 */
static EGLBoolean
_eglPreloadDefaultDriver(void)
{
   _EGLDriver *drv;
   char path[1024];
   const char *suffix = library_suffix();

   if (suffix)
      snprintf(path, sizeof(path), "%s.%s", DefaultDriverName, suffix);
   else
      snprintf(path, sizeof(path), DefaultDriverName);

   drv = _eglLoadDriver(path, NULL);
   if (!drv)
      return EGL_FALSE;

   _eglGlobal.Drivers[_eglGlobal.NumDrivers++] = drv;

   return EGL_TRUE;
}


/**
 * Preload drivers.
 *
 * This function loads the driver modules and creates the corresponding
 * _EGLDriver objects.
 */
EGLBoolean
_eglPreloadDrivers(void)
{
   EGLBoolean loaded;

   /* already preloaded */
   if (_eglGlobal.NumDrivers)
      return EGL_TRUE;

   loaded = (_eglPreloadUserDriver() ||
             _eglPreloadDisplayDrivers() ||
             _eglPreloadDefaultDriver());

   return loaded;
}


/**
 * Unload preloaded drivers.
 */
void
_eglUnloadDrivers(void)
{
   EGLint i;
   for (i = 0; i < _eglGlobal.NumDrivers; i++) {
      _EGLDriver *drv = _eglGlobal.Drivers[i];
      lib_handle handle = drv->LibHandle;

      if (drv->Path)
         free((char *) drv->Path);
      if (drv->Args)
         free((char *) drv->Args);

      /* destroy driver */
      if (drv->Unload)
         drv->Unload(drv);

      if (handle)
         close_library(handle);
      _eglGlobal.Drivers[i] = NULL;
   }

   _eglGlobal.NumDrivers = 0;
}


/**
 * Plug all the available fallback routines into the given driver's
 * dispatch table.
 */
void
_eglInitDriverFallbacks(_EGLDriver *drv)
{
   /* If a pointer is set to NULL, then the device driver _really_ has
    * to implement it.
    */
   drv->API.Initialize = NULL;
   drv->API.Terminate = NULL;

   drv->API.GetConfigs = _eglGetConfigs;
   drv->API.ChooseConfig = _eglChooseConfig;
   drv->API.GetConfigAttrib = _eglGetConfigAttrib;

   drv->API.CreateContext = _eglCreateContext;
   drv->API.DestroyContext = _eglDestroyContext;
   drv->API.MakeCurrent = _eglMakeCurrent;
   drv->API.QueryContext = _eglQueryContext;

   drv->API.CreateWindowSurface = _eglCreateWindowSurface;
   drv->API.CreatePixmapSurface = _eglCreatePixmapSurface;
   drv->API.CreatePbufferSurface = _eglCreatePbufferSurface;
   drv->API.DestroySurface = _eglDestroySurface;
   drv->API.QuerySurface = _eglQuerySurface;
   drv->API.SurfaceAttrib = _eglSurfaceAttrib;
   drv->API.BindTexImage = _eglBindTexImage;
   drv->API.ReleaseTexImage = _eglReleaseTexImage;
   drv->API.SwapInterval = _eglSwapInterval;
   drv->API.SwapBuffers = _eglSwapBuffers;
   drv->API.CopyBuffers = _eglCopyBuffers;

   drv->API.QueryString = _eglQueryString;
   drv->API.WaitClient = _eglWaitClient;
   drv->API.WaitNative = _eglWaitNative;

#ifdef EGL_MESA_screen_surface
   drv->API.ChooseModeMESA = _eglChooseModeMESA; 
   drv->API.GetModesMESA = _eglGetModesMESA;
   drv->API.GetModeAttribMESA = _eglGetModeAttribMESA;
   drv->API.GetScreensMESA = _eglGetScreensMESA;
   drv->API.CreateScreenSurfaceMESA = _eglCreateScreenSurfaceMESA;
   drv->API.ShowScreenSurfaceMESA = _eglShowScreenSurfaceMESA;
   drv->API.ScreenPositionMESA = _eglScreenPositionMESA;
   drv->API.QueryScreenMESA = _eglQueryScreenMESA;
   drv->API.QueryScreenSurfaceMESA = _eglQueryScreenSurfaceMESA;
   drv->API.QueryScreenModeMESA = _eglQueryScreenModeMESA;
   drv->API.QueryModeStringMESA = _eglQueryModeStringMESA;
#endif /* EGL_MESA_screen_surface */

#ifdef EGL_VERSION_1_2
   drv->API.CreatePbufferFromClientBuffer = _eglCreatePbufferFromClientBuffer;
#endif /* EGL_VERSION_1_2 */
}



/**
 * Try to determine which EGL APIs (OpenGL, OpenGL ES, OpenVG, etc)
 * are supported on the system by looking for standard library names.
 */
EGLint
_eglFindAPIs(void)
{
   EGLint mask = 0x0;
   lib_handle lib;
#if defined(_EGL_PLATFORM_WINDOWS)
   /* XXX not sure about these names */
   const char *es1_libname = "libGLESv1_CM.dll";
   const char *es2_libname = "libGLESv2.dll";
   const char *gl_libname = "OpenGL32.dll";
   const char *vg_libname = "libOpenVG.dll";
#elif defined(_EGL_PLATFORM_POSIX)
   const char *es1_libname = "libGLESv1_CM.so";
   const char *es2_libname = "libGLESv2.so";
   const char *gl_libname = "libGL.so";
   const char *vg_libname = "libOpenVG.so";
#else /* _EGL_PLATFORM_NO_OS */
   const char *es1_libname = NULL;
   const char *es2_libname = NULL;
   const char *gl_libname = NULL;
   const char *vg_libname = NULL;
#endif

   if ((lib = open_library(es1_libname))) {
      close_library(lib);
      mask |= EGL_OPENGL_ES_BIT;
   }

   if ((lib = open_library(es2_libname))) {
      close_library(lib);
      mask |= EGL_OPENGL_ES2_BIT;
   }

   if ((lib = open_library(gl_libname))) {
      close_library(lib);
      mask |= EGL_OPENGL_BIT;
   }

   if ((lib = open_library(vg_libname))) {
      close_library(lib);
      mask |= EGL_OPENVG_BIT;
   }

   return mask;
}

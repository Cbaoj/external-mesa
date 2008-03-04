/* $XFree86: xc/lib/GL/dri/dri_util.c,v 1.7 2003/04/28 17:01:25 dawes Exp $ */
/**
 * \file dri_util.c
 * DRI utility functions.
 *
 * This module acts as glue between GLX and the actual hardware driver.  A DRI
 * driver doesn't really \e have to use any of this - it's optional.  But, some
 * useful stuff is done here that otherwise would have to be duplicated in most
 * drivers.
 * 
 * Basically, these utility functions take care of some of the dirty details of
 * screen initialization, context creation, context binding, DRM setup, etc.
 *
 * These functions are compiled into each DRI driver so libGL.so knows nothing
 * about them.
 */


#include <assert.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <dlfcn.h>

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

#include "imports.h"
#define None 0

#include "dri_util.h"
#include "drm_sarea.h"

#ifndef GLX_OML_sync_control
typedef GLboolean ( * PFNGLXGETMSCRATEOMLPROC) (__DRIdrawable *drawable, int32_t *numerator, int32_t *denominator);
#endif

/**
 * This is just a token extension used to signal that the driver
 * supports setting a read drawable.
 */
const __DRIextension driReadDrawableExtension = {
    __DRI_READ_DRAWABLE, __DRI_READ_DRAWABLE_VERSION
};

static void *driCreateNewDrawable(__DRIscreen *screen,
				  const __GLcontextModes *modes,
                                  __DRIdrawable *pdraw,
				  drm_drawable_t hwDrawable,
                                  int renderType, const int *attrs);

static void driDestroyDrawable(__DRIdrawable *drawable);


/**
 * Print message to \c stderr if the \c LIBGL_DEBUG environment variable
 * is set. 
 * 
 * Is called from the drivers.
 * 
 * \param f \c printf like format string.
 */
void
__driUtilMessage(const char *f, ...)
{
    va_list args;

    if (getenv("LIBGL_DEBUG")) {
        fprintf(stderr, "libGL error: \n");
        va_start(args, f);
        vfprintf(stderr, f, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
}


/*****************************************************************/
/** \name Context (un)binding functions                          */
/*****************************************************************/
/*@{*/

/**
 * Unbind context.
 * 
 * \param scrn the screen.
 * \param gc context.
 *
 * \return \c GL_TRUE on success, or \c GL_FALSE on failure.
 * 
 * \internal
 * This function calls __DriverAPIRec::UnbindContext, and then decrements
 * __DRIdrawablePrivateRec::refcount which must be non-zero for a successful
 * return.
 * 
 * While casting the opaque private pointers associated with the parameters
 * into their respective real types it also assures they are not \c NULL. 
 */
static GLboolean driUnbindContext(__DRIcontext *ctx)
{
    __DRIcontextPrivate *pcp;
    __DRIscreenPrivate *psp;
    __DRIdrawablePrivate *pdp;
    __DRIdrawablePrivate *prp;

    /*
    ** Assume error checking is done properly in glXMakeCurrent before
    ** calling driUnbindContext.
    */

    if (ctx == NULL)
        return GL_FALSE;

    pcp = (__DRIcontextPrivate *)ctx->private;
    psp = (__DRIscreenPrivate *)pcp->driScreenPriv;
    pdp = (__DRIdrawablePrivate *)pcp->driDrawablePriv;
    prp = (__DRIdrawablePrivate *)pcp->driReadablePriv;

    /* Let driver unbind drawable from context */
    (*psp->DriverAPI.UnbindContext)(pcp);

    if (pdp->refcount == 0) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    pdp->refcount--;

    if (prp != pdp) {
        if (prp->refcount == 0) {
	    /* ERROR!!! */
	    return GL_FALSE;
	}

	prp->refcount--;
    }


    /* XXX this is disabled so that if we call SwapBuffers on an unbound
     * window we can determine the last context bound to the window and
     * use that context's lock. (BrianP, 2-Dec-2000)
     */
#if 0
    /* Unbind the drawable */
    pcp->driDrawablePriv = NULL;
    pdp->driContextPriv = &psp->dummyContextPriv;
#endif

    return GL_TRUE;
}


/**
 * This function takes both a read buffer and a draw buffer.  This is needed
 * for \c glXMakeCurrentReadSGI or GLX 1.3's \c glXMakeContextCurrent
 * function.
 */
static GLboolean driBindContext(__DRIcontext * ctx,
				__DRIdrawable *pdraw,
				__DRIdrawable *pread)
{
    __DRIdrawablePrivate *pdp;
    __DRIdrawablePrivate *prp;
    __DRIcontextPrivate * const pcp = ctx->private;
    __DRIscreenPrivate *psp = pcp->driScreenPriv;

    /*
    ** Assume error checking is done properly in glXMakeCurrent before
    ** calling driBindContext.
    */

    if (ctx == NULL || pdraw == None || pread == None)
	return GL_FALSE;

    pdp = (__DRIdrawablePrivate *) pdraw->private;
    prp = (__DRIdrawablePrivate *) pread->private;

    /* Bind the drawable to the context */
    pcp->driDrawablePriv = pdp;
    pcp->driReadablePriv = prp;
    pdp->driContextPriv = pcp;
    pdp->refcount++;
    if ( pdp != prp ) {
	prp->refcount++;
    }

    /*
    ** Now that we have a context associated with this drawable, we can
    ** initialize the drawable information if has not been done before.
    */

    if (psp->dri2.enabled) {
       __driParseEvents(psp, pdp);
       __driParseEvents(psp, prp);
    } else {
	if (!pdp->pStamp || *pdp->pStamp != pdp->lastStamp) {
	    DRM_SPINLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);
	    __driUtilUpdateDrawableInfo(pdp);
	    DRM_SPINUNLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);
	}
	
	if ((pdp != prp) && (!prp->pStamp || *prp->pStamp != prp->lastStamp)) {
	    DRM_SPINLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);
	    __driUtilUpdateDrawableInfo(prp);
	    DRM_SPINUNLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);
	}
    }

    /* Call device-specific MakeCurrent */
    (*psp->DriverAPI.MakeCurrent)(pcp, pdp, prp);

    return GL_TRUE;
}

/*@}*/


/*****************************************************************/
/** \name Drawable handling functions                            */
/*****************************************************************/
/*@{*/

/**
 * Update private drawable information.
 *
 * \param pdp pointer to the private drawable information to update.
 * 
 * This function basically updates the __DRIdrawablePrivate struct's
 * cliprect information by calling \c __DRIinterfaceMethods::getDrawableInfo.
 * This is usually called by the DRI_VALIDATE_DRAWABLE_INFO macro which
 * compares the __DRIdrwablePrivate pStamp and lastStamp values.  If
 * the values are different that means we have to update the clipping
 * info.
 */
void
__driUtilUpdateDrawableInfo(__DRIdrawablePrivate *pdp)
{
    __DRIscreenPrivate *psp = pdp->driScreenPriv;
    __DRIcontextPrivate *pcp = pdp->driContextPriv;
    
    if (!pcp 
	|| ((pdp != pcp->driDrawablePriv) && (pdp != pcp->driReadablePriv))) {
	/* ERROR!!! 
	 * ...but we must ignore it. There can be many contexts bound to a
	 * drawable.
	 */
    }

    if (pdp->pClipRects) {
	_mesa_free(pdp->pClipRects); 
	pdp->pClipRects = NULL;
    }

    if (pdp->pBackClipRects) {
	_mesa_free(pdp->pBackClipRects); 
	pdp->pBackClipRects = NULL;
    }

    DRM_SPINUNLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);

    if (! (*psp->getDrawableInfo->getDrawableInfo)(pdp->pdraw,
			  &pdp->index, &pdp->lastStamp,
			  &pdp->x, &pdp->y, &pdp->w, &pdp->h,
			  &pdp->numClipRects, &pdp->pClipRects,
			  &pdp->backX,
			  &pdp->backY,
			  &pdp->numBackClipRects,
			  &pdp->pBackClipRects )) {
	/* Error -- eg the window may have been destroyed.  Keep going
	 * with no cliprects.
	 */
        pdp->pStamp = &pdp->lastStamp; /* prevent endless loop */
	pdp->numClipRects = 0;
	pdp->pClipRects = NULL;
	pdp->numBackClipRects = 0;
	pdp->pBackClipRects = NULL;
    }
    else
       pdp->pStamp = &(psp->pSAREA->drawableTable[pdp->index].stamp);

    DRM_SPINLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);

}

int
__driParseEvents(__DRIscreenPrivate *psp, __DRIdrawablePrivate *pdp)
{
    __DRIDrawableConfigEvent *dc, *last_dc;
    __DRIBufferAttachEvent *ba, *last_ba;
    unsigned int tail, mask, *p, end, total, size, changed;
    unsigned char *data;
    size_t rect_size;
    __DRIcontextPrivate *pcp = pdp->driContextPriv;

    if (pcp == NULL)
	return 0;

    /* Check for wraparound. */
    if (psp->dri2.buffer->prealloc - pdp->dri2.tail > psp->dri2.buffer->size) {
       /* If prealloc overlaps into what we just parsed, the
	* server overwrote it and we have to reset our tail
	* pointer. */
	DRM_UNLOCK(psp->fd, psp->lock, pcp->hHWContext);
	(*psp->dri2.core->reemitDrawableInfo)(pdp->pdraw, &pdp->dri2.tail);
	DRM_LIGHT_LOCK(psp->fd, psp->lock, pcp->hHWContext);
    }

    total = psp->dri2.buffer->head - pdp->dri2.tail;
    mask = psp->dri2.buffer->size - 1;
    end = psp->dri2.buffer->head;
    data = psp->dri2.buffer->data;

    changed = 0;
    last_dc = NULL;
    last_ba = NULL;

    for (tail = pdp->dri2.tail; tail != end; tail += size) {
       p = (unsigned int *) (data + (tail & mask));
       size = DRI2_EVENT_SIZE(*p);
       if (size > total || (tail & mask) + size > psp->dri2.buffer->size) {
	  /* illegal data, bail out. */
	  fprintf(stderr, "illegal event size\n");
	  break;
       }

       switch (DRI2_EVENT_TYPE(*p)) {
       case DRI2_EVENT_DRAWABLE_CONFIG:
	  dc = (__DRIDrawableConfigEvent *) p;
	  if (dc->drawable == pdp->hHWDrawable)
	     last_dc = dc;
	  break;

       case DRI2_EVENT_BUFFER_ATTACH:
	  ba = (__DRIBufferAttachEvent *) p;
	  if (ba->drawable == pdp->hHWDrawable && 
	      ba->buffer.attachment == DRI_DRAWABLE_BUFFER_FRONT_LEFT)
	     last_ba = ba;
	  break;
       }
    }
	  
    if (last_dc) {
       if (pdp->w != last_dc->width || pdp->h != last_dc->height)
	  changed = 1;

       pdp->x = last_dc->x;
       pdp->y = last_dc->y;
       pdp->w = last_dc->width;
       pdp->h = last_dc->height;

       pdp->backX = 0;
       pdp->backY = 0;
       pdp->numBackClipRects = 1;
       pdp->pBackClipRects[0].x1 = 0;
       pdp->pBackClipRects[0].y1 = 0;
       pdp->pBackClipRects[0].x2 = pdp->w;
       pdp->pBackClipRects[0].y2 = pdp->h;

       pdp->numClipRects = last_dc->num_rects;
       _mesa_free(pdp->pClipRects);
       rect_size = last_dc->num_rects * sizeof last_dc->rects[0];
       pdp->pClipRects = _mesa_malloc(rect_size);
       memcpy(pdp->pClipRects, last_dc->rects, rect_size);

       if (changed)
	  (*psp->DriverAPI.UpdateBuffer)(pdp, (unsigned int *) last_dc);
    }

    /* Front buffer attachments are special, they typically mean that
     * we're rendering to a redirected window (or a child window of a
     * redirected window) and that it got resized.  Resizing the root
     * window on randr events is a special case of this.  Other causes
     * may be a window transitioning between redirected and
     * non-redirected, or a window getting reparented between parents
     * with different window pixmaps (eg two redirected windows).
     * These events are special in that the X server allocates the
     * buffer and that the buffer may be shared by other child
     * windows.  When our window share the window pixmap with its
     * parent, drawable config events doesn't affect the front buffer.
     * We only care about the last such event in the buffer; in fact,
     * older events will refer to invalid buffer objects.*/
    if (last_ba)
       (*psp->DriverAPI.UpdateBuffer)(pdp, (unsigned int *) last_ba);

    /* Like for buffer attachments, we only care about the most recent
     * drawable config. */
    if (last_dc)
       (*psp->DriverAPI.UpdateBuffer)(pdp, (unsigned int *) last_dc);

    /* If there was a drawable config event in the buffer and it
     * changed the size of the window, all buffer auxillary buffer
     * attachments prior to that are invalid (as opposed to the front
     * buffer case discussed above).  In that case we can start
     * looking for buffer attachment after the last drawable config
     * event.  If there is no drawable config event in this batch of
     * events, we have to assume that the last batch might have had
     * one and process all buffer attach events.*/
    if (last_dc && changed)
       tail = (unsigned char *) last_dc - data;
    else
       tail = pdp->dri2.tail;

    for ( ; tail != end; tail += size) {
       ba = (__DRIBufferAttachEvent *) (data + (tail & mask));
       size = DRI2_EVENT_SIZE(ba->event_header);

       if (DRI2_EVENT_TYPE(ba->event_header) != DRI2_EVENT_BUFFER_ATTACH)
	  continue;
       if (ba->drawable != pdp->hHWDrawable)
	  continue;
       if (last_ba == ba)
	  continue;

       (*psp->DriverAPI.UpdateBuffer)(pdp, (unsigned int *) ba);
    }

    pdp->dri2.tail = tail;

    /* FIXME: Return whether we changed anything.  This check always
     * returns true if we received events, but we could refine the
     * check to only return TRUE if the drawable actually changed.  */

    return total > 0;
}

/*@}*/

/*****************************************************************/
/** \name GLX callbacks                                          */
/*****************************************************************/
/*@{*/

/**
 * Swap buffers.
 *
 * \param drawablePrivate opaque pointer to the per-drawable private info.
 * 
 * \internal
 * This function calls __DRIdrawablePrivate::swapBuffers.
 * 
 * Is called directly from glXSwapBuffers().
 */
static void driSwapBuffers(__DRIdrawable *drawable)
{
    __DRIdrawablePrivate *dPriv = drawable->private;
    __DRIscreenPrivate *psp = dPriv->driScreenPriv;
    drm_clip_rect_t rect;

    if (!dPriv->numClipRects)
        return;

    dPriv->swapBuffers(dPriv);

    /* Check that we actually have the new damage report method */
    if (psp->damage == NULL)
	return;

    /* Assume it's affecting the whole drawable for now */
    rect.x1 = 0;
    rect.y1 = 0;
    rect.x2 = rect.x1 + dPriv->w;
    rect.y2 = rect.y1 + dPriv->h;

    /* Report the damage.  Currently, all our drivers draw directly to the
     * front buffer, so we report the damage there rather than to the backing
     * store (if any).
     */
    (*psp->damage->reportDamage)(dPriv->pdraw,
				 dPriv->x, dPriv->y, &rect, 1, GL_TRUE);
}

static int driDrawableGetMSC( __DRIscreen *screen, __DRIdrawable *drawable,
			      int64_t *msc )
{
    __DRIscreenPrivate *sPriv = screen->private;
    __DRIdrawablePrivate *dPriv = drawable->private;

    return sPriv->DriverAPI.GetDrawableMSC(sPriv, dPriv, msc);
}

static int driWaitForMSC(__DRIdrawable *drawable, int64_t target_msc,
			 int64_t divisor, int64_t remainder,
			 int64_t * msc, int64_t * sbc)
{
    __DRIdrawablePrivate *dPriv = drawable->private;
    __DRIswapInfo  sInfo;
    int  status;


    status = dPriv->driScreenPriv->DriverAPI.WaitForMSC( dPriv, target_msc,
                                                         divisor, remainder,
                                                         msc );

    /* GetSwapInfo() may not be provided by the driver if GLX_SGI_video_sync
     * is supported but GLX_OML_sync_control is not.  Therefore, don't return
     * an error value if GetSwapInfo() is not implemented.
    */
    if ( status == 0
         && dPriv->driScreenPriv->DriverAPI.GetSwapInfo ) {
        status = dPriv->driScreenPriv->DriverAPI.GetSwapInfo( dPriv, & sInfo );
        *sbc = sInfo.swap_count;
    }

    return status;
}

const __DRImediaStreamCounterExtension driMediaStreamCounterExtension = {
    { __DRI_MEDIA_STREAM_COUNTER, __DRI_MEDIA_STREAM_COUNTER_VERSION },
    driWaitForMSC,
    driDrawableGetMSC,
};

static void driCopySubBuffer(__DRIdrawable *drawable,
			      int x, int y, int w, int h)
{
    __DRIdrawablePrivate *dPriv = drawable->private;
    dPriv->driScreenPriv->DriverAPI.CopySubBuffer(dPriv, x, y, w, h);
}

const __DRIcopySubBufferExtension driCopySubBufferExtension = {
    { __DRI_COPY_SUB_BUFFER, __DRI_COPY_SUB_BUFFER_VERSION },
    driCopySubBuffer
};

static void driSetSwapInterval(__DRIdrawable *drawable, unsigned int interval)
{
    __DRIdrawablePrivate *dpriv = drawable->private;

    dpriv->swap_interval = interval;
}

static unsigned int driGetSwapInterval(__DRIdrawable *drawable)
{
    __DRIdrawablePrivate *dpriv = drawable->private;

    return dpriv->swap_interval;
}

const __DRIswapControlExtension driSwapControlExtension = {
    { __DRI_SWAP_CONTROL, __DRI_SWAP_CONTROL_VERSION },
    driSetSwapInterval,
    driGetSwapInterval
};


/**
 * This is called via __DRIscreenRec's createNewDrawable pointer.
 */
static void *driCreateNewDrawable(__DRIscreen *screen,
				  const __GLcontextModes *modes,
				  __DRIdrawable *pdraw,
				  drm_drawable_t hwDrawable,
				  int renderType,
				  const int *attrs)
{
    __DRIscreenPrivate *psp;
    __DRIdrawablePrivate *pdp;


    pdraw->private = NULL;

    /* Since pbuffers are not yet supported, no drawable attributes are
     * supported either.
     */
    (void) attrs;

    pdp = (__DRIdrawablePrivate *)_mesa_malloc(sizeof(__DRIdrawablePrivate));
    if (!pdp) {
	return NULL;
    }

    pdp->hHWDrawable = hwDrawable;
    pdp->pdraw = pdraw;
    pdp->refcount = 0;
    pdp->pStamp = NULL;
    pdp->lastStamp = 0;
    pdp->index = 0;
    pdp->x = 0;
    pdp->y = 0;
    pdp->w = 0;
    pdp->h = 0;
    pdp->numClipRects = 0;
    pdp->numBackClipRects = 0;
    pdp->pClipRects = NULL;
    pdp->pBackClipRects = NULL;
    pdp->vblSeq = 0;
    pdp->vblFlags = 0;

    psp = (__DRIscreenPrivate *)screen->private;
    pdp->driScreenPriv = psp;
    pdp->driContextPriv = &psp->dummyContextPriv;

    if (!(*psp->DriverAPI.CreateBuffer)(psp, pdp, modes,
					renderType == GLX_PIXMAP_BIT)) {
       _mesa_free(pdp);
       return NULL;
    }

    pdraw->private = pdp;
    pdraw->destroyDrawable = driDestroyDrawable;
    pdraw->swapBuffers = driSwapBuffers;  /* called by glXSwapBuffers() */
    pdp->msc_base = 0;

    /* This special default value is replaced with the configured
     * default value when the drawable is first bound to a direct
     * rendering context. 
     */
    pdp->swap_interval = (unsigned)-1;

    pdp->swapBuffers = psp->DriverAPI.SwapBuffers;

    if (psp->dri2.enabled) {
	pdp->dri2.tail = 0;
	pdp->pBackClipRects = _mesa_malloc(sizeof *pdp->pBackClipRects);
    }

   return (void *) pdp;
}

static void
driDestroyDrawable(__DRIdrawable *drawable)
{
    __DRIdrawablePrivate *pdp = drawable->private;
    __DRIscreenPrivate *psp;

    if (pdp) {
	psp = pdp->driScreenPriv;
        (*psp->DriverAPI.DestroyBuffer)(pdp);
	if (pdp->pClipRects) {
	    _mesa_free(pdp->pClipRects);
	    pdp->pClipRects = NULL;
	}
	if (pdp->pBackClipRects) {
	    _mesa_free(pdp->pBackClipRects);
	    pdp->pBackClipRects = NULL;
	}
	_mesa_free(pdp);
    }
}

/*@}*/


/*****************************************************************/
/** \name Context handling functions                             */
/*****************************************************************/
/*@{*/

/**
 * Destroy the per-context private information.
 * 
 * \param contextPrivate opaque pointer to the per-drawable private info.
 *
 * \internal
 * This function calls __DriverAPIRec::DestroyContext on \p contextPrivate, calls
 * drmDestroyContext(), and finally frees \p contextPrivate.
 */
static void
driDestroyContext(__DRIcontext *context)
{
    __DRIcontextPrivate  *pcp   = context->private;

    if (pcp) {
	(*pcp->driScreenPriv->DriverAPI.DestroyContext)(pcp);
	_mesa_free(pcp);
    }
}


/**
 * Create the per-drawable private driver information.
 * 
 * \param dpy           The display handle.
 * \param modes         Mode used to create the new context.
 * \param render_type   Type of rendering target.  \c GLX_RGBA is the only
 *                      type likely to ever be supported for direct-rendering.
 * \param shared        The shared context dependent methods or \c NULL if
 *                      non-existent.
 * \param pctx          DRI context to receive the context dependent methods.
 *
 * \returns An opaque pointer to the per-context private information on
 *          success, or \c NULL on failure.
 * 
 * \internal
 * This function allocates and fills a __DRIcontextPrivateRec structure.  It
 * performs some device independent initialization and passes all the
 * relevent information to __DriverAPIRec::CreateContext to create the
 * context.
 *
 */
static void *
driCreateNewContext(__DRIscreen *screen, const __GLcontextModes *modes,
		    int render_type, __DRIcontext *shared, 
		    drm_context_t hwContext, __DRIcontext *pctx)
{
    __DRIcontextPrivate *pcp;
    __DRIcontextPrivate *pshare = (shared != NULL) ? shared->private : NULL;
    __DRIscreenPrivate *psp;
    void * const shareCtx = (pshare != NULL) ? pshare->driverPrivate : NULL;

    psp = (__DRIscreenPrivate *)screen->private;

    pcp = (__DRIcontextPrivate *)_mesa_malloc(sizeof(__DRIcontextPrivate));
    if (!pcp) {
	return NULL;
    }

    pcp->hHWContext = hwContext;
    pcp->driScreenPriv = psp;
    pcp->driDrawablePriv = NULL;

    /* When the first context is created for a screen, initialize a "dummy"
     * context.
     */

    if (!psp->dri2.enabled && !psp->dummyContextPriv.driScreenPriv) {
        psp->dummyContextPriv.hHWContext = psp->pSAREA->dummy_context;
        psp->dummyContextPriv.driScreenPriv = psp;
        psp->dummyContextPriv.driDrawablePriv = NULL;
        psp->dummyContextPriv.driverPrivate = NULL;
	/* No other fields should be used! */
    }

    pctx->destroyContext = driDestroyContext;
    pctx->bindContext    = driBindContext;
    pctx->unbindContext  = driUnbindContext;

    if ( !(*psp->DriverAPI.CreateContext)(modes, pcp, shareCtx) ) {
        _mesa_free(pcp);
        return NULL;
    }

    return pcp;
}
/*@}*/


static const __DRIextension **
driGetExtensions(__DRIscreen *screen)
{
    __DRIscreenPrivate *psp = screen->private;

    return psp->extensions;
}

/*****************************************************************/
/** \name Screen handling functions                              */
/*****************************************************************/
/*@{*/

/**
 * Destroy the per-screen private information.
 * 
 * \param dpy the display handle.
 * \param scrn the screen number.
 * \param screenPrivate opaque pointer to the per-screen private information.
 *
 * \internal
 * This function calls __DriverAPIRec::DestroyScreen on \p screenPrivate, calls
 * drmClose(), and finally frees \p screenPrivate.
 */
static void driDestroyScreen(__DRIscreen *screen)
{
    __DRIscreenPrivate *psp = screen->private;

    if (psp) {
	/* No interaction with the X-server is possible at this point.  This
	 * routine is called after XCloseDisplay, so there is no protocol
	 * stream open to the X-server anymore.
	 */

	if (psp->DriverAPI.DestroyScreen)
	    (*psp->DriverAPI.DestroyScreen)(psp);

	if (psp->dri2.enabled) {
	    drmBOUnmap(psp->fd, &psp->dri2.sareaBO);
	    drmBOUnreference(psp->fd, &psp->dri2.sareaBO);
	} else {
	   (void)drmUnmap((drmAddress)psp->pSAREA, SAREA_MAX);
	   (void)drmUnmap((drmAddress)psp->pFB, psp->fbSize);
	   (void)drmCloseOnce(psp->fd);
	}

	_mesa_free(psp);
    }
}

static void
setupLoaderExtensions(__DRIscreenPrivate *psp,
		      const __DRIextension **extensions)
{
    int i;

    for (i = 0; extensions[i]; i++) {
	if (strcmp(extensions[i]->name, __DRI_CONTEXT_MODES) == 0)
	    psp->contextModes = (__DRIcontextModesExtension *) extensions[i];
	if (strcmp(extensions[i]->name, __DRI_GET_DRAWABLE_INFO) == 0)
	    psp->getDrawableInfo = (__DRIgetDrawableInfoExtension *) extensions[i];
	if (strcmp(extensions[i]->name, __DRI_DAMAGE) == 0)
	    psp->damage = (__DRIdamageExtension *) extensions[i];
	if (strcmp(extensions[i]->name, __DRI_SYSTEM_TIME) == 0)
	    psp->systemTime = (__DRIsystemTimeExtension *) extensions[i];
	if (strcmp(extensions[i]->name, __DRI_CORE_DRI2) == 0)
	    psp->dri2.core = (__DRIcoreDRI2Extension *) extensions[i];
    }
}

/**
 * This is the bootstrap function for the driver.  libGL supplies all of the
 * requisite information about the system, and the driver initializes itself.
 * This routine also fills in the linked list pointed to by \c driver_modes
 * with the \c __GLcontextModes that the driver can support for windows or
 * pbuffers.
 * 
 * \param scrn  Index of the screen
 * \param psc   DRI screen data (not driver private)
 * \param modes Linked list of known display modes.  This list is, at a
 *              minimum, a list of modes based on the current display mode.
 *              These roughly match the set of available X11 visuals, but it
 *              need not be limited to X11!  The calling libGL should create
 *              a list that will inform the driver of the current display
 *              mode (i.e., color buffer depth, depth buffer depth, etc.).
 * \param ddx_version Version of the 2D DDX.  This may not be meaningful for
 *                    all drivers.
 * \param dri_version Version of the "server-side" DRI.
 * \param drm_version Version of the kernel DRM.
 * \param frame_buffer Data describing the location and layout of the
 *                     framebuffer.
 * \param pSAREA       Pointer the the SAREA.
 * \param fd           Device handle for the DRM.
 * \param internal_api_version  Version of the internal interface between the
 *                              driver and libGL.
 * \param driverAPI Driver API functions used by other routines in dri_util.c.
 * 
 * \note There is no need to check the minimum API version in this
 * function.  Since the name of this function is versioned, it is
 * impossible for a loader that is too old to even load this driver.
 */
PUBLIC
void * __DRI_CREATE_NEW_SCREEN( int scrn, __DRIscreen *psc,
				const __DRIversion * ddx_version,
				const __DRIversion * dri_version,
				const __DRIversion * drm_version,
				const __DRIframebuffer * frame_buffer,
				drmAddress pSAREA, int fd, 
				const __DRIextension ** extensions,
				__GLcontextModes ** driver_modes )
			     
{
    __DRIscreenPrivate *psp;
    static const __DRIextension *emptyExtensionList[] = { NULL };

    psp = _mesa_malloc(sizeof(*psp));
    if (!psp)
	return NULL;

    psp->psc = psc;

    setupLoaderExtensions(psp, extensions);

    /*
    ** NOT_DONE: This is used by the X server to detect when the client
    ** has died while holding the drawable lock.  The client sets the
    ** drawable lock to this value.
    */
    psp->drawLockID = 1;

    psp->drm_version = *drm_version;
    psp->ddx_version = *ddx_version;
    psp->dri_version = *dri_version;

    psp->pSAREA = pSAREA;
    psp->lock = (drmLock *) &psp->pSAREA->lock;

    psp->pFB = frame_buffer->base;
    psp->fbSize = frame_buffer->size;
    psp->fbStride = frame_buffer->stride;
    psp->fbWidth = frame_buffer->width;
    psp->fbHeight = frame_buffer->height;
    psp->devPrivSize = frame_buffer->dev_priv_size;
    psp->pDevPriv = frame_buffer->dev_priv;
    psp->fbBPP = psp->fbStride * 8 / frame_buffer->width;

    psp->extensions = emptyExtensionList;
    psp->fd = fd;
    psp->myNum = scrn;
    psp->dri2.enabled = GL_FALSE;

    /*
    ** Do not init dummy context here; actual initialization will be
    ** done when the first DRI context is created.  Init screen priv ptr
    ** to NULL to let CreateContext routine that it needs to be inited.
    */
    psp->dummyContextPriv.driScreenPriv = NULL;

    psc->destroyScreen     = driDestroyScreen;
    psc->getExtensions     = driGetExtensions;
    psc->createNewDrawable = driCreateNewDrawable;
    psc->createNewContext  = driCreateNewContext;

    *driver_modes = __driDriverInitScreen(psp);
    if (*driver_modes == NULL) {
	_mesa_free(psp);
	return NULL;
    }

    return psp;
}

PUBLIC void *
__DRI2_CREATE_NEW_SCREEN(int scrn, __DRIscreen *psc,
			 int fd, unsigned int sarea_handle,
			 const __DRIextension **extensions,
			 __GLcontextModes **driver_modes)
{
    __DRIscreenPrivate *psp;
    static const __DRIextension *emptyExtensionList[] = { NULL };
    unsigned int *p;
    drmVersionPtr version;
    __GLcontextModes *(*initScreen)(__DRIscreenPrivate *psc);

    initScreen = dlsym(NULL, "__dri2DriverInitScreen");
    if (initScreen == NULL)
        return NULL;

    psp = _mesa_malloc(sizeof(*psp));
    if (!psp)
	return NULL;

    setupLoaderExtensions(psp, extensions);

    psp->psc = psc;

    version = drmGetVersion(fd);
    if (version) {
	psp->drm_version.major = version->version_major;
	psp->drm_version.minor = version->version_minor;
	psp->drm_version.patch = version->version_patchlevel;
	drmFreeVersion(version);
    }

    psp->extensions = emptyExtensionList;
    psp->fd = fd;
    psp->myNum = scrn;
    psp->dri2.enabled = GL_TRUE;

    if (drmBOReference(psp->fd, sarea_handle, &psp->dri2.sareaBO)) {
	fprintf(stderr, "Failed to reference DRI2 sarea BO\n");
	_mesa_free(psp);
	return NULL;
    }

    if (drmBOMap(psp->fd, &psp->dri2.sareaBO,
		 DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE, 0, &psp->dri2.sarea)) {
	drmBOUnreference(psp->fd, &psp->dri2.sareaBO);
	_mesa_free(psp);
	return NULL;
    }

    p = psp->dri2.sarea;
    while (DRI2_SAREA_BLOCK_TYPE(*p)) {
	switch (DRI2_SAREA_BLOCK_TYPE(*p)) {
	case DRI2_SAREA_BLOCK_LOCK:
	    psp->dri2.lock = (__DRILock *) p;
	    break;
	case DRI2_SAREA_BLOCK_EVENT_BUFFER:
	    psp->dri2.buffer = (__DRIEventBuffer *) p;
	    break;
	}
	p = DRI2_SAREA_BLOCK_NEXT(p);
    }

    psp->lock = (drmLock *) &psp->dri2.lock->lock;

    psc->destroyScreen     = driDestroyScreen;
    psc->getExtensions     = driGetExtensions;
    psc->createNewDrawable = driCreateNewDrawable;
    psc->createNewContext  = driCreateNewContext;

    *driver_modes = initScreen(psp);
    if (*driver_modes == NULL) {
	drmBOUnmap(psp->fd, &psp->dri2.sareaBO);
	drmBOUnreference(psp->fd, &psp->dri2.sareaBO);
	_mesa_free(psp);
	return NULL;
    }

    return psp;
}

static int
driFrameTracking(__DRIdrawable *drawable, GLboolean enable)
{
    return GLX_BAD_CONTEXT;
}

static int
driQueryFrameTracking(__DRIdrawable *drawable,
		      int64_t * sbc, int64_t * missedFrames,
		      float * lastMissedUsage, float * usage)
{
   __DRIswapInfo   sInfo;
   int             status;
   int64_t         ust;
   __DRIdrawablePrivate * dpriv = drawable->private;
   __DRIscreenPrivate *psp = dpriv->driScreenPriv;

   status = dpriv->driScreenPriv->DriverAPI.GetSwapInfo( dpriv, & sInfo );
   if ( status == 0 ) {
      *sbc = sInfo.swap_count;
      *missedFrames = sInfo.swap_missed_count;
      *lastMissedUsage = sInfo.swap_missed_usage;

      (*psp->systemTime->getUST)( & ust );
      *usage = driCalculateSwapUsage( dpriv, sInfo.swap_ust, ust );
   }

   return status;
}

const __DRIframeTrackingExtension driFrameTrackingExtension = {
    { __DRI_FRAME_TRACKING, __DRI_FRAME_TRACKING_VERSION },
    driFrameTracking,
    driQueryFrameTracking    
};

/**
 * Calculate amount of swap interval used between GLX buffer swaps.
 * 
 * The usage value, on the range [0,max], is the fraction of total swap
 * interval time used between GLX buffer swaps is calculated.
 *
 *            \f$p = t_d / (i * t_r)\f$
 * 
 * Where \f$t_d\f$ is the time since the last GLX buffer swap, \f$i\f$ is the
 * swap interval (as set by \c glXSwapIntervalSGI), and \f$t_r\f$ time
 * required for a single vertical refresh period (as returned by \c
 * glXGetMscRateOML).
 * 
 * See the documentation for the GLX_MESA_swap_frame_usage extension for more
 * details.
 *
 * \param   dPriv  Pointer to the private drawable structure.
 * \return  If less than a single swap interval time period was required
 *          between GLX buffer swaps, a number greater than 0 and less than
 *          1.0 is returned.  If exactly one swap interval time period is
 *          required, 1.0 is returned, and if more than one is required then
 *          a number greater than 1.0 will be returned.
 *
 * \sa glXSwapIntervalSGI glXGetMscRateOML
 * 
 * \todo Instead of caching the \c glXGetMscRateOML function pointer, would it
 *       be possible to cache the sync rate?
 */
float
driCalculateSwapUsage( __DRIdrawablePrivate *dPriv, int64_t last_swap_ust,
		       int64_t current_ust )
{
   int32_t   n;
   int32_t   d;
   int       interval;
   float     usage = 1.0;
   __DRIscreenPrivate *psp = dPriv->driScreenPriv;

   if ( (*psp->systemTime->getMSCRate)(dPriv->pdraw, &n, &d) ) {
      interval = (dPriv->swap_interval != 0) ? dPriv->swap_interval : 1;


      /* We want to calculate
       * (current_UST - last_swap_UST) / (interval * us_per_refresh).  We get
       * current_UST by calling __glXGetUST.  last_swap_UST is stored in
       * dPriv->swap_ust.  interval has already been calculated.
       *
       * The only tricky part is us_per_refresh.  us_per_refresh is
       * 1000000 / MSC_rate.  We know the MSC_rate is n / d.  We can flip it
       * around and say us_per_refresh = 1000000 * d / n.  Since this goes in
       * the denominator of the final calculation, we calculate
       * (interval * 1000000 * d) and move n into the numerator.
       */

      usage = (current_ust - last_swap_ust);
      usage *= n;
      usage /= (interval * d);
      usage /= 1000000.0;
   }
   
   return usage;
}

/*@}*/

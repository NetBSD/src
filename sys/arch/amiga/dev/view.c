/*
 * Copyright (c) 1994 Christian E. Hopps
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christian E. Hopps.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: view.c,v 1.2 1994/01/29 06:59:37 chopps Exp $
 */

/* The view major device is a placeholder device.  It serves
 * simply to map the semantics of a graphics dipslay to 
 * the semantics of a character block device.  In other
 * words the graphics system as currently built does not like to be
 * refered to by open/close/ioctl.  This device serves as
 * a interface to graphics. */

#include "param.h"
#include "proc.h"
#include "ioctl.h"
#include "file.h"
#include "malloc.h"

#include "device.h"

#include "machine/cpu.h"

#include "vm/vm.h"
#include "vm/vm_kern.h"
#include "vm/vm_page.h"
#include "vm/vm_pager.h"

#include "miscfs/specfs/specdev.h"
#include "vnode.h"
#include "mman.h"

#include "grf/grf_types.h"
#include "grf/grf_bitmap.h"
#include "grf/grf_view.h"
#include "grf/grf_mode.h"
#include "grf/grf_monitor.h"
#include "viewioctl.h"
#include "viewvar.h"
#if 0
#include "view.h"
#endif

/* just one possible right now with the grf kludge */
#undef NVIEW
#define NVIEW 1

int viewprobe ();

#if 0
/* not currently used as independant device */
struct driver view_driver = { viewprobe, "view" };
#endif

struct view_softc views[NVIEW];
static int inited;

int view_default_x;
int view_default_y;
int view_default_width = 640;
int view_default_height = 400;
int view_default_depth = 2;

/* 
 *  functions for probeing.
 */

/* this function is called early to set up a display. */
viewconfig ()
{
    viewprobe ();
}

viewprobe ()
{
    if (!inited) {
    	int i;

    	inited = 1;
    	for (i=0; i<NVIEW; i++) {
    	    views[i].view = NULL;
    	    views[i].flags = 0;
    	}
    }
    return (1);
}


/*
 *  Internal functions.
 */

static void
view_display (struct view_softc *vu)
{
    int s = spltty ();
    int i;

    if (vu == NULL) {
    	return;
    }
    /* mark views that share this monitor as not displaying */
    for (i=0; i<NVIEW; i++) {
        if (views[i].flags&VUF_DISPLAY && views[i].monitor == vu->monitor) {
            views[i].flags &= ~VUF_DISPLAY;
        }
    }

    vu->flags |= VUF_ADDED;
    if (vu->view) {
	vu->view->display.x = vu->size.x;
	vu->view->display.y = vu->size.y;
	grf_display_view (vu->view);
	vu->size.x = vu->view->display.x;
	vu->size.y = vu->view->display.y;
	vu->flags |= VUF_DISPLAY;
    }

    splx (s);
}

/* remove a view from our added list if it is marked as displaying */
/* switch to a new display. */
static void
view_remove (struct view_softc *vu)
{
    int i;

    if (!(vu->flags & VUF_ADDED)) {
    	return;
    }

    vu->flags &= ~VUF_ADDED;
    if (vu->flags & VUF_DISPLAY) {
	for (i=0; i<NVIEW; i++) {
	    if (views[i].flags & VUF_ADDED && &views[i] != vu && 
	        views[i].monitor == vu->monitor) {
		view_display (&views[i]);
		break;
	    }
    	}
    }
    vu->flags &= ~VUF_DISPLAY;
    grf_remove_view (vu->view);
}

static int
view_setsize (struct view_softc *vu, struct view_size *vs)
{
    view_t *new, *old;
    dimen_t ns;
    int co = 0, cs = 0;
    
    if (vs->x != vu->size.x ||
	vs->y != vu->size.y) {
	co = 1;
    }

    if (vs->width != vu->size.width ||
	vs->height != vu->size.height ||
	vs->depth != vu->size.depth) {
	cs = 1;
    }

    if (!cs && !co) {
	/* no change. */
	return (0);
    }
    
    ns.width = vs->width;
    ns.height = vs->height;

    new = grf_alloc_view (NULL, &ns, vs->depth);
    if (!new) {
	return (ENOMEM);	/* check this error. */
    }
	
    old = vu->view;
    vu->view = new;

    vu->size.x = new->display.x;
    vu->size.y = new->display.y;
    vu->size.width = new->display.width;
    vu->size.height = new->display.height;
    vu->size.depth = new->bitmap->depth;
    
    vu->mode = grf_get_display_mode (vu->view);
    vu->monitor = grf_get_monitor (vu->mode);

    vu->size.x = vs->x;
    vu->size.y = vs->y;
    
    /* we are all set to go everything is A-OK. */
    /* we need a custom remove here to avoid letting another view display */
    /* mark as not added or displayed */
    if (vu->flags & VUF_DISPLAY) {
	vu->flags &= ~(VUF_ADDED|VUF_DISPLAY);

	/* change view pointers */

	/* display new view first */ 
	view_display (vu);
    }

    /* remove old view from monitor */
    grf_free_view (old);
    
    return (0);
}

/*
 *  functions made available by conf.c
 */

/*ARGSUSED*/
viewopen (dev, flags)
  dev_t dev;
{
    dimen_t size;
    struct view_softc *vu = &views[minor(dev)];

    if (minor(dev) >= NVIEW)
        return EXDEV;

    if (vu->flags & VUF_OPEN) {
        return (EBUSY);
    }

    vu->size.x = view_default_x;
    vu->size.y = view_default_y;
    size.width = vu->size.width = view_default_width;
    size.height = vu->size.height = view_default_height;
    vu->size.depth = view_default_depth;

    vu->view = grf_alloc_view (NULL, &size, vu->size.depth);
    if (vu->view) {
	vu->size.x = vu->view->display.x;
	vu->size.y = vu->view->display.y;
	vu->size.width = vu->view->display.width;
	vu->size.height = vu->view->display.height;
	vu->size.depth = vu->view->bitmap->depth;
       	vu->flags |= VUF_OPEN;
    	vu->mode = grf_get_display_mode (vu->view);
    	vu->monitor = grf_get_monitor (vu->mode);
       	return (0);
    }
    return (ENOMEM);
}

/*ARGSUSED*/
viewclose (dev, flags)
  dev_t dev;
{
    struct view_softc *vu = &views[minor(dev)];

    if (vu->flags & VUF_OPEN) {
	view_remove (vu);
	grf_free_view (vu->view);
	vu->flags = 0;
	vu->view = NULL;
	vu->mode = NULL;
	vu->monitor = NULL;	
    }
}


/*ARGSUSED*/
viewioctl (dev, cmd, data, flag, p)
  dev_t dev;
  caddr_t data;
  struct proc *p;
{
    bmap_t *bm;
    colormap_t *cm;
    struct view_softc *vu = &views[minor(dev)];
    int error = 0;

    switch (cmd) {
      case VIEW_DISPLAY:
      	view_display (vu);
      	break;

      case VIEW_REMOVE:
      	view_remove (vu);
      	break;

      case VIEW_GETSIZE:
        bcopy (&vu->size, data, sizeof (struct view_size)); 
        break;

      case VIEW_SETSIZE:
        error = view_setsize (vu, (struct view_size *)data);
        break;

      case VIEW_GETBITMAP:
        bm = (bmap_t *)data;
        bcopy (vu->view->bitmap, bm, sizeof (bmap_t));
	if ((int)p != -1) {
	    bm->plane = 0;
	    bm->blit_temp = 0;
	    bm->hardware_address = 0;
	}
        break;
      case VIEW_GETCOLORMAP:
	error = view_get_colormap (vu, data);
	break;
      case VIEW_USECOLORMAP:
	error = view_use_colormap (vu, data);
	break;
      default:
	error = EINVAL;
	break;
    }
    return (error);
}

/*ARGSUSED*/
view_get_colormap (vu, ucm)
    struct view_softc *vu;
    colormap_t *ucm;
{
    int error = 0;
    u_long *cme = malloc(sizeof (u_long)*(ucm->size + 1),
			    M_IOCTLOPS, M_WAITOK); /* add one incase of zero, ick. */
    if (cme) {
	u_long *uep = ucm->entry;

	ucm->entry = cme;			  /* set entry to out alloc. */

	if (!vu->view || grf_get_colormap (vu->view, ucm))
	    error = EINVAL;
	else if (copyout (cme, uep, sizeof (u_long)*ucm->size)) 
	    error = EINVAL;
	
	ucm->entry = uep;			  /* set entry back to users. */
	free (cme, M_IOCTLOPS);
    } else {
	error = ENOMEM;
    }
    return (error);
}

/*ARGSUSED*/
view_use_colormap (vu, ucm)
    struct view_softc *vu;
    colormap_t *ucm;
{
    int error = 0;
    colormap_t *cm = malloc(sizeof (u_long)*ucm->size + sizeof (*cm),
			    M_IOCTLOPS, M_WAITOK);
    if (cm) {
	bcopy (ucm, cm, sizeof (colormap_t));
	cm->entry = &cm[1];			  /* table directly after. */
	if (copyin (ucm->entry, cm->entry, sizeof (u_long)*ucm->size)) 
	    error = EINVAL;
	else if (!vu->view || grf_use_colormap (vu->view, cm)) 
	    error = EINVAL;
	free (cm, M_IOCTLOPS);
    } else {
	error = ENOMEM;
    }
    return (error);
}

/*ARGSUSED*/
viewmap(dev, off, prot)
        dev_t dev;
{
    struct view_softc *vu = &views[minor(dev)];
    bmap_t *bm = vu->view->bitmap;
    u_byte *bmd_start = bm->hardware_address; 
    u_long  bmd_size = bm->bytes_per_row*bm->rows*bm->depth;

    if (off >= 0 && off < bmd_size) {
    	return(((u_int)bmd_start + off) >> PGSHIFT);
    }
    /* bogus */
    return(-1);
}

/*ARGSUSED*/
viewselect(dev, rw)
  dev_t dev;
{
    if (rw == FREAD)
	return(0);
    return(1);
}

void
viewattach() {}

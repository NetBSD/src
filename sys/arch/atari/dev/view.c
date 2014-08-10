/*	$NetBSD: view.c,v 1.33.2.1 2014/08/10 06:53:52 tls Exp $	*/

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
 */

/* The view major device is a placeholder device.  It serves
 * simply to map the semantics of a graphics dipslay to 
 * the semantics of a character block device.  In other
 * words the graphics system as currently built does not like to be
 * refered to by open/close/ioctl.  This device serves as
 * a interface to graphics. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: view.c,v 1.33.2.1 2014/08/10 06:53:52 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/conf.h>
#include <machine/cpu.h>
#include <atari/dev/grfabs_reg.h>
#include <atari/dev/viewioctl.h>
#include <atari/dev/viewvar.h>
#include "view.h"

static void view_display(struct view_softc *);
static void view_remove(struct view_softc *);
static int  view_setsize(struct view_softc *, struct view_size *);
static int  view_get_colormap(struct view_softc *, colormap_t *);
static int  view_set_colormap(struct view_softc *, colormap_t *);

struct view_softc views[NVIEW];
static int view_inited;

int view_default_x;
int view_default_y;
int view_default_width  = 640;
int view_default_height = 400;
int view_default_depth  = 1;

dev_type_open(viewopen);
dev_type_close(viewclose);
dev_type_ioctl(viewioctl);
dev_type_mmap(viewmmap);

const struct cdevsw view_cdevsw = {
	.d_open = viewopen,
	.d_close = viewclose,
	.d_read = nullread,
	.d_write = nullwrite,
	.d_ioctl = viewioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = viewmmap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

/* 
 *  functions for probeing.
 */
void	viewattach(int);

void
viewattach(int cnt)
{
	viewprobe();
	printf("%d view%s configured\n", NVIEW, NVIEW > 1 ? "s" : "");
}

/* this function is called early to set up a display. */
int
viewprobe(void)
{
    	int i;
	
	if (view_inited)
		return 1;

	view_inited = 1;

	for (i = 0; i < NVIEW; i++) {
		views[i].view = NULL;
		views[i].flags = 0;
	}
	return 1;
}


/*
 *  Internal functions.
 */

static void
view_display (struct view_softc *vu)
{
	int s, i;

	if (vu == NULL)
		return;
	
	s = spltty();

	/*
	 * mark views that share this monitor as not displaying 
	 */
	for (i = 0; i < NVIEW; i++) {
		if (views[i].flags & VUF_DISPLAY) {
			if (vu->view && (vu->view == views[i].view)) {
				splx(s);
				return;
			}
			if (views[i].view) {
				grf_save_view(views[i].view);
				views[i].view->flags &= ~VF_DISPLAY;
			}
			views[i].flags &= ~VUF_DISPLAY;
		}
	}

	vu->flags |= VUF_ADDED;
	if (vu->view) {
		vu->view->display.x = vu->size.x;
		vu->view->display.y = vu->size.y;

		grf_display_view(vu->view);
		vu->view->flags |= VF_DISPLAY;

		vu->size.x = vu->view->display.x;
		vu->size.y = vu->view->display.y;
		vu->flags |= VUF_DISPLAY;
	}
	splx(s);
}

/* 
 * remove a view from our added list if it is marked as displaying
 * switch to a new display.
 */
static void
view_remove(struct view_softc *vu)
{
	int i;

	if ((vu->flags & VUF_ADDED) == 0)
		return;

	vu->flags &= ~VUF_ADDED;
	if (vu->flags & VUF_DISPLAY) {
		for (i = 0; i < NVIEW; i++) {
			if ((views[i].flags & VUF_ADDED) && &views[i] != vu) {
				view_display(&views[i]);
				break;
			}
		}
	}
	vu->flags &= ~VUF_DISPLAY;
	grf_remove_view(vu->view);
}

static int
view_setsize(struct view_softc *vu, struct view_size *vs)
{
	view_t	*new, *old;
	dmode_t	*dmode;
	dimen_t ns;
	int	co, cs;
   
	co = 0;
	cs = 0;
	if (vs->x != vu->size.x || vs->y != vu->size.y)
		co = 1;

	if (vs->width != vu->size.width || vs->height != vu->size.height ||
	    vs->depth != vu->size.depth)
		cs = 1;

	if (cs == 0 && co == 0)
		return 0;
    
	ns.width  = vs->width;
	ns.height = vs->height;

	if ((dmode = grf_get_best_mode(&ns, vs->depth)) != NULL) {
		/*
		 * If we can't do better, leave it
		 */
		if (dmode == vu->view->mode)
			return 0;
	}
	new = grf_alloc_view(dmode, &ns, vs->depth);
	if (new == NULL)
		return ENOMEM;
	
	old = vu->view;
	vu->view = new;
	vu->size.x = new->display.x;
	vu->size.y = new->display.y;
	vu->size.width = new->display.width;
	vu->size.height = new->display.height;
	vu->size.depth = new->bitmap->depth;

	/* 
	 * we need a custom remove here to avoid letting 
	 * another view display mark as not added or displayed 
	 */
	if (vu->flags & VUF_DISPLAY) {
		vu->flags &= ~(VUF_ADDED|VUF_DISPLAY);
		view_display(vu);
	}
	grf_free_view(old);
	return 0;
}

static int
view_get_colormap (struct view_softc *vu, colormap_t *ucm)
{
	int	error;
	long	*cme;
	long	*uep;

	if (ucm->size > MAX_CENTRIES)
		return EINVAL;
		
	/* add one incase of zero, ick. */
	cme = malloc(sizeof(ucm->entry[0])*(ucm->size+1), M_TEMP,M_WAITOK);
	if (cme == NULL)
		return ENOMEM;

	error      = 0;	
	uep        = ucm->entry;
	ucm->entry = cme;	  /* set entry to out alloc. */
	if (vu->view == NULL || grf_get_colormap(vu->view, ucm))
		error = EINVAL;
	else
		error = copyout(cme, uep, sizeof(ucm->entry[0]) * ucm->size);
	ucm->entry = uep;	  /* set entry back to users. */
	free(cme, M_TEMP);
	return error;
}

static int
view_set_colormap(struct view_softc *vu, colormap_t *ucm)
{
	colormap_t	*cm;
	int		error = 0;

	if (ucm->size > MAX_CENTRIES)
		return EINVAL;
		
	cm = malloc(sizeof(ucm->entry[0])*ucm->size + sizeof(*cm),
	    M_TEMP, M_WAITOK);
	if (cm == NULL)
		return ENOMEM;

	memcpy(cm, ucm, sizeof(colormap_t));
	cm->entry = (long *)&cm[1];		 /* table directly after. */
	if (((error = 
	    copyin(ucm->entry,cm->entry,sizeof(ucm->entry[0])*ucm->size)) == 0)
	    && (vu->view == NULL || grf_use_colormap(vu->view, cm)))
		error = EINVAL;
	free(cm, M_TEMP);
	return error;
}

/*
 *  functions made available by conf.c
 */

/*ARGSUSED*/
int
viewopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	dimen_t			size;
	struct view_softc	*vu;

	vu = &views[minor(dev)];

	if (minor(dev) >= NVIEW)
		return EXDEV;
	if (vu->flags & VUF_OPEN)
		return EBUSY;

	vu->size.x = view_default_x;
	vu->size.y = view_default_y;
	size.width = vu->size.width = view_default_width;
	size.height = vu->size.height = view_default_height;
	vu->size.depth = view_default_depth;
	vu->view = grf_alloc_view(NULL, &size, vu->size.depth);
	if (vu->view == NULL)
		return ENOMEM;

	vu->size.x = vu->view->display.x;
	vu->size.y = vu->view->display.y;
	vu->size.width = vu->view->display.width;
	vu->size.height = vu->view->display.height;
	vu->size.depth = vu->view->bitmap->depth;
       	vu->flags |= VUF_OPEN;
       	return 0;
}

/*ARGSUSED*/
int
viewclose (dev_t dev, int flags, int mode, struct lwp *l)
{
	struct view_softc *vu;

	vu = &views[minor(dev)];

	if ((vu->flags & VUF_OPEN) == 0)
		return 0; /* XXX not open? */
	view_remove (vu);
	grf_free_view (vu->view);
	vu->flags = 0;
	vu->view = NULL;
	return 0;
}


/*ARGSUSED*/
int
viewioctl (dev_t dev, u_long cmd, void * data, int flag, struct lwp *l)
{
	struct view_softc	*vu;
	bmap_t			*bm;
	int			error;

	vu = &views[minor(dev)];
	error = 0;

	switch (cmd) {
	case VIOCDISPLAY:
		view_display(vu);
		break;
	case VIOCREMOVE:
		view_remove(vu);
		break;
	case VIOCGSIZE:
		memcpy(data, &vu->size, sizeof (struct view_size)); 
		break;
	case VIOCSSIZE:
		error = view_setsize(vu, (struct view_size *)data);
		break;
	case VIOCGBMAP:
		bm = (bmap_t *)data;
		memcpy(bm, vu->view->bitmap, sizeof(bmap_t));
		if (l != NOLWP) {
			bm->plane      = NULL;
			bm->hw_address = NULL;
			bm->regs       = NULL;
			bm->hw_regs    = NULL;
		}
		break;
	case VIOCGCMAP:
		error = view_get_colormap(vu, (colormap_t *)data);
		break;
	case VIOCSCMAP:
		error = view_set_colormap(vu, (colormap_t *)data);
		break;
	default:
		error = EPASSTHROUGH;
		break;
	}
	return error;
}

/*ARGSUSED*/
paddr_t
viewmmap(dev_t dev, off_t off, int prot)
{
	struct view_softc	*vu;
	bmap_t			*bm;
	u_char			*bmd_start;
	u_long			bmd_lin, bmd_vga; 

	vu = &views[minor(dev)];
	bm = vu->view->bitmap;
	bmd_start = bm->hw_address; 
	bmd_lin = bm->lin_base;
	bmd_vga = bm->vga_base;

	/* 
	 * control registers
	 */
	if (off >= 0 && off < bm->reg_size)
		return ((paddr_t)bm->hw_regs + off) >> PGSHIFT;

	/*
	 * VGA memory
	 */
	if (off >= bmd_vga && off < (bmd_vga + bm->vga_mappable))
		return ((paddr_t)bm->vga_address - bmd_vga + off) >> PGSHIFT;

	/*
	 * frame buffer
	 */
	if (off >= bmd_lin && off < (bmd_lin + bm->phys_mappable))
		return ((paddr_t)bmd_start - bmd_lin + off) >> PGSHIFT;

	return -1;
}

view_t	*
viewview(dev_t dev)
{

	return views[minor(dev)].view;
}

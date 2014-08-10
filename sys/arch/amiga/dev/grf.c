/*	$NetBSD: grf.c,v 1.61.2.1 2014/08/10 06:53:49 tls Exp $ */

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: grf.c 1.31 91/01/21$
 *
 *	@(#)grf.c	7.8 (Berkeley) 5/7/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: grf.c,v 1.61.2.1 2014/08/10 06:53:49 tls Exp $");

/*
 * Graphics display driver for the Amiga
 * This is the hardware-independent portion of the driver.
 * Hardware access is through the grf_softc->g_mode routine.
 */

#include "view.h"
#include "grf.h"
#include "kbd.h"
#include "wsdisplay.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/mman.h>
#include <sys/bus.h>
#include <sys/kauth.h>

#include <machine/cpu.h>

#include <dev/cons.h>
#include <dev/sun/fbio.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <amiga/amiga/color.h>	/* DEBUG */
#include <amiga/amiga/device.h>
#include <amiga/dev/grfioctl.h>
#include <amiga/dev/grfws.h>
#include <amiga/dev/grfvar.h>
#include <amiga/dev/itevar.h>
#include <amiga/dev/kbdvar.h>
#include <amiga/dev/viewioctl.h>

#include <sys/conf.h>

#if NGRF > 0
#include "ite.h"
#if NITE == 0
#define	ite_on(u,f)
#define	ite_off(u,f)
#define ite_reinit(d)
#endif

int grfon(dev_t);
int grfoff(dev_t);
int grfsinfo(dev_t, struct grfdyninfo *);

void grfattach(device_t, device_t, void *);
int grfmatch(device_t, cfdata_t, void *);
int grfprint(void *, const char *);
#ifdef DEBUG
void grfdebug(struct grf_softc *, const char *, ...);
#endif
/*
 * pointers to grf drivers device structs
 */
struct grf_softc *grfsp[NGRF];

CFATTACH_DECL_NEW(grf, 0,
    grfmatch, grfattach, NULL, NULL);

dev_type_open(grfopen);
dev_type_close(grfclose);
dev_type_ioctl(grfioctl);
dev_type_mmap(grfmmap);

const struct cdevsw grf_cdevsw = {
	.d_open = grfopen,
	.d_close = grfclose,
	.d_read = nullread,
	.d_write = nullwrite,
	.d_ioctl = grfioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = grfmmap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

/*
 * only used in console init.
 */
static cfdata_t cfdata;

#if NWSDISPLAY > 0
static struct vcons_screen console_vcons;

static void grf_init_screen(void *, struct vcons_screen *, int, long *);
static struct rasops_info *grf_setup_rasops(struct grf_softc *,
    struct vcons_screen *);
static paddr_t grf_wsmmap_md(off_t off);

cons_decl(grf);
#endif

/*
 * match if the unit of grf matches its perspective
 * low level board driver.
 */
int
grfmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct grf_softc *psc;

	psc = device_private(parent);
	if (cf->cf_unit != psc->g_unit)
		return(0);
	cfdata = cf;
	return(1);
}

/*
 * Attach.. plug pointer in and print some info.
 * Then try and attach a wsdisplay or ite to us.
 * Note: self is NULL durring console init.
 */
void
grfattach(device_t parent, device_t self, void *aux)
{
#if NWSDISPLAY > 0
	struct wsemuldisplaydev_attach_args wa;
	long defattr;
#endif
	struct grf_softc *gp;
	int maj;

	gp = device_private(parent);
	gp->g_device = self;
	grfsp[gp->g_unit] = gp;

	/*
	 * find our major device number
	 */
	maj = cdevsw_lookup_major(&grf_cdevsw);

	gp->g_grfdev = makedev(maj, gp->g_unit);
	if (self != NULL) {
		printf(": width %d height %d", gp->g_display.gd_dwidth,
		    gp->g_display.gd_dheight);
		if (gp->g_display.gd_colors == 2)
			printf(" monochrome\n");
		else
			printf(" colors %d\n", gp->g_display.gd_colors);
#if NWSDISPLAY > 0
		vcons_init(&gp->g_vd, gp, gp->g_screens[0], gp->g_accessops);
		gp->g_vd.init_screen = grf_init_screen;
		if (gp->g_flags & GF_CONSOLE) {
			console_vcons.scr_flags |= VCONS_SCREEN_IS_STATIC;
			vcons_init_screen(&gp->g_vd,
			    &console_vcons, 1, &defattr);
			gp->g_screens[0]->textops =
			    &console_vcons.scr_ri.ri_ops;
			wsdisplay_cnattach(gp->g_screens[0],
			    &console_vcons.scr_ri, 0, 0, defattr);
			vcons_replay_msgbuf(&console_vcons);
		}

		/* attach wsdisplay */
		wa.console = (gp->g_flags & GF_CONSOLE) != 0;
		wa.scrdata = &gp->g_screenlist;
		wa.accessops = gp->g_accessops;
		wa.accesscookie = &gp->g_vd;
		config_found(self, &wa, wsemuldisplaydevprint);
#endif  /* NWSDISPLAY > 0 */
	}

#if NWSDISPLAY == 0
	/*
	 * try and attach an ite
	 */
	amiga_config_found(cfdata, self, gp, grfprint);
#endif
}

int
grfprint(void *aux, const char *pnp)
{
	if (pnp)
		aprint_normal("ite at %s", pnp);
	return(UNCONF);
}

/*ARGSUSED*/
int
grfopen(dev_t dev, int flags, int devtype, struct lwp *l)
{
	struct grf_softc *gp;

	if (GRFUNIT(dev) >= NGRF || (gp = grfsp[GRFUNIT(dev)]) == NULL)
		return(ENXIO);

	if ((gp->g_flags & GF_ALIVE) == 0)
		return(ENXIO);

	if ((gp->g_flags & (GF_OPEN|GF_EXCLUDE)) == (GF_OPEN|GF_EXCLUDE))
		return(EBUSY);

	return(0);
}

/*ARGSUSED*/
int
grfclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct grf_softc *gp;

	gp = grfsp[GRFUNIT(dev)];
	(void)grfoff(dev);
	gp->g_flags &= GF_ALIVE;
	return(0);
}

/*ARGSUSED*/
int
grfioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct grf_softc *gp;
	int error;

	gp = grfsp[GRFUNIT(dev)];
	error = 0;

	switch (cmd) {
	case OGRFIOCGINFO:
	        /* argl.. no bank-member.. */
	  	memcpy(data, (void *)&gp->g_display, sizeof(struct grfinfo)-4);
		break;
	case GRFIOCGINFO:
		memcpy(data, (void *)&gp->g_display, sizeof(struct grfinfo));
		break;
	case GRFIOCON:
		error = grfon(dev);
		break;
	case GRFIOCOFF:
		error = grfoff(dev);
		break;
	case GRFIOCSINFO:
		error = grfsinfo(dev, (struct grfdyninfo *) data);
		break;
	case GRFGETVMODE:
		return(gp->g_mode(gp, GM_GRFGETVMODE, data, 0, 0));
	case GRFSETVMODE:
		error = gp->g_mode(gp, GM_GRFSETVMODE, data, 0, 0);
		if (error == 0 && gp->g_itedev && !(gp->g_flags & GF_GRFON))
			ite_reinit(gp->g_itedev);
		break;
	case GRFGETNUMVM:
		return(gp->g_mode(gp, GM_GRFGETNUMVM, data, 0, 0));
	/*
	 * these are all hardware dependent, and have to be resolved
	 * in the respective driver.
	 */
	case GRFIOCPUTCMAP:
	case GRFIOCGETCMAP:
	case GRFIOCSSPRITEPOS:
	case GRFIOCGSPRITEPOS:
	case GRFIOCSSPRITEINF:
	case GRFIOCGSPRITEINF:
	case GRFIOCGSPRITEMAX:
	case GRFIOCBITBLT:
    	case GRFIOCSETMON:
	case GRFTOGGLE: /* Toggles between Cirrus boards and native ECS on
                     Amiga. 15/11/94 ill */
		/*
		 * We need the minor dev number to get the overlay/image
		 * information for grf_ul.
		 */
		return(gp->g_mode(gp, GM_GRFIOCTL, data, cmd, dev));

	case GRFIOCBLANK:	/* blank ioctl, IOCON/OFF will turn ite on */
	case FBIOSVIDEO:
		error = gp->g_mode(gp, GM_GRFIOCTL, data, GRFIOCBLANK, dev);
		if (!error)
			gp->g_blank = *(int *)data;
		return (error);

	case FBIOGVIDEO:
		*(int *)data = gp->g_blank;
		return (0);

	default:
#if NVIEW > 0
		/*
		 * check to see whether it's a command recognized by the
		 * view code if the unit is 0
		 * XXX
		 */
		if (GRFUNIT(dev) == 0) {
			extern const struct cdevsw view_cdevsw;

			return((*view_cdevsw.d_ioctl)(dev, cmd, data, flag, l));
		}
#endif
		error = EPASSTHROUGH;
		break;

	}
	return(error);
}

/*
 * map the contents of a graphics display card into process'
 * memory space.
 */
paddr_t
grfmmap(dev_t dev, off_t off, int prot)
{
	struct grf_softc *gp;
	struct grfinfo *gi;

	gp = grfsp[GRFUNIT(dev)];
	gi = &gp->g_display;

	/*
	 * control registers
	 */
	if (off >= 0 && off < gi->gd_regsize)
		return(((paddr_t)gi->gd_regaddr + off) >> PGSHIFT);

	/*
	 * frame buffer
	 */
	if (off >= gi->gd_regsize && off < gi->gd_regsize+gi->gd_fbsize) {
		off -= gi->gd_regsize;
		return(((paddr_t)gi->gd_fbaddr + off) >> PGSHIFT);
	}
	/* bogus */
	return(-1);
}

int
grfon(dev_t dev)
{
	struct grf_softc *gp;

	gp = grfsp[GRFUNIT(dev)];

	if (gp->g_flags & GF_GRFON)
		return(0);

	gp->g_flags |= GF_GRFON;
	if (gp->g_itedev != NODEV)
		ite_off(gp->g_itedev, 3);

	return(gp->g_mode(gp, (dev & GRFOVDEV) ? GM_GRFOVON : GM_GRFON,
							NULL, 0, 0));
}

int
grfoff(dev_t dev)
{
	struct grf_softc *gp;
	int error;

	gp = grfsp[GRFUNIT(dev)];

	if ((gp->g_flags & GF_GRFON) == 0)
		return(0);

	gp->g_flags &= ~GF_GRFON;
	error = gp->g_mode(gp, (dev & GRFOVDEV) ? GM_GRFOVOFF : GM_GRFOFF,
							NULL, 0, 0);

	/*
	 * Closely tied together no X's
	 */
	if (gp->g_itedev != NODEV)
		ite_on(gp->g_itedev, 2);

	return(error);
}

int
grfsinfo(dev_t dev, struct grfdyninfo *dyninfo)
{
	struct grf_softc *gp;
	int error;

	gp = grfsp[GRFUNIT(dev)];
	error = gp->g_mode(gp, GM_GRFCONFIG, dyninfo, 0, 0);

	/*
	 * Closely tied together no X's
	 */
	if (gp->g_itedev != NODEV)
		ite_reinit(gp->g_itedev);
	return(error);
}

#if NWSDISPLAY > 0
void
grfcnprobe(struct consdev *cd)
{
	struct grf_softc *gp;
	int unit;

	/*
	 * Find the first working grf device for being console.
	 * Ignore unit 0 (grfcc), which should use amidisplaycc instead.
	 */
	for (unit = 1; unit < NGRF; unit++) {
		gp = grfsp[unit];
		if (gp != NULL && (gp->g_flags & GF_ALIVE)) {
			cd->cn_pri = CN_INTERNAL;
			cd->cn_dev = NODEV;  /* initialized later by wscons */
			return;
		}
	}

	/* no grf console alive */
	cd->cn_pri = CN_DEAD;
}

void
grfcninit(struct consdev *cd)
{
	struct grf_softc *gp;
	struct rasops_info *ri;
	long defattr;
	int unit;

	/* find console grf and set up wsdisplay for it */
	for (unit = 1; unit < NGRF; unit++) {
		gp = grfsp[unit];
		if (gp != NULL && (gp->g_flags & GF_ALIVE)) {
			gp->g_flags |= GF_CONSOLE;  /* we are console! */
			gp->g_screens[0]->ncols = gp->g_display.gd_fbwidth /
			    gp->g_screens[0]->fontwidth;
			gp->g_screens[0]->nrows = gp->g_display.gd_fbheight /
			    gp->g_screens[0]->fontheight;

			ri = grf_setup_rasops(gp, &console_vcons);
			console_vcons.scr_cookie = gp;
			defattr = 0;  /* XXX */

			wsdisplay_preattach(gp->g_screens[0], ri, 0, 0,
			    defattr);
#if NKBD > 0
			/* tell kbd device it is used as console keyboard */
			kbd_cnattach();
#endif
			return;
		}
	}
	panic("grfcninit: lost console");
}

static void
grf_init_screen(void *cookie, struct vcons_screen *scr, int existing,
    long *defattr)
{
	struct grf_softc *gp;
	struct rasops_info *ri __unused;

	gp = cookie;
	ri = grf_setup_rasops(gp, scr);
}

static struct rasops_info *
grf_setup_rasops(struct grf_softc *gp, struct vcons_screen *scr)
{
	struct rasops_info *ri;
	int i;

	ri = &scr->scr_ri;
	scr->scr_flags |= VCONS_DONT_READ;
	memset(ri, 0, sizeof(struct rasops_info));

	ri->ri_rows = gp->g_screens[0]->nrows;
	ri->ri_cols = gp->g_screens[0]->ncols;
	ri->ri_hw = scr;
	ri->ri_ops.cursor    = gp->g_emulops->cursor;
	ri->ri_ops.mapchar   = gp->g_emulops->mapchar;
	ri->ri_ops.copyrows  = gp->g_emulops->copyrows;
	ri->ri_ops.eraserows = gp->g_emulops->eraserows;
	ri->ri_ops.copycols  = gp->g_emulops->copycols;
	ri->ri_ops.erasecols = gp->g_emulops->erasecols;
	ri->ri_ops.putchar   = gp->g_emulops->putchar;
	ri->ri_ops.allocattr = gp->g_emulops->allocattr;

	/* multiplication table for row-offsets */
	for (i = 0; i < ri->ri_rows; i++)
		gp->g_rowoffset[i] = i * ri->ri_cols;

	return ri;
}

paddr_t
grf_wsmmap(void *v, void *vs, off_t off, int prot)
{
	struct vcons_data *vd;
	struct grf_softc *gp;
	struct grfinfo *gi;

	vd = v;
	gp = vd->cookie;
	gi = &gp->g_display;

	/* Normal fb mapping */
	if (off < gi->gd_fbsize)
		return grf_wsmmap_md(((bus_addr_t)gp->g_fbkva) + off);

	if (kauth_authorize_machdep(kauth_cred_get(), KAUTH_MACHDEP_UNMANAGEDMEM,
	    NULL, NULL, NULL, NULL) != 0) {
		aprint_normal("%s: permission to mmap denied.\n",
		    device_xname(gp->g_device));
		return -1;
	}

	if ((off >= (bus_addr_t)gp->g_fbkva ) &&
	    (off < ( (bus_addr_t)gp->g_fbkva + (size_t)gi->gd_fbsize)))
		return grf_wsmmap_md(off);

	/* Handle register mapping */
	if ((off >= (bus_addr_t)gi->gd_regaddr) &&
	    (off < ((bus_addr_t)gi->gd_regaddr + (size_t)gi->gd_regsize)))
		return grf_wsmmap_md(off);

	return -1;
}

static paddr_t
grf_wsmmap_md(off_t off) 
{
#if defined(__m68k__)
	return (paddr_t) m68k_btop(off);
#else
	return -1; /* FIXME */
#endif
}

int
grf_wsioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct vcons_data *vd;
	struct grf_softc *gp;

	vd = v;
	gp = vd->cookie;

	switch (cmd) {
	/* XXX: check if ptr to implementation is not null */
	case WSDISPLAYIO_GINFO:
		return gp->g_wsioctl->ginfo(gp, data);
	case WSDISPLAYIO_SMODE:
		return gp->g_wsioctl->smode(gp, data);
	case WSDISPLAYIO_GMODE:
		return gp->g_wsioctl->gmode(gp, data);
	case WSDISPLAYIO_GTYPE:
		return gp->g_wsioctl->gtype(gp, data);
	case WSDISPLAYIO_SVIDEO:
		return gp->g_wsioctl->svideo(gp, data);
	case WSDISPLAYIO_GVIDEO:
		return gp->g_wsioctl->gvideo(gp, data);
	case WSDISPLAYIO_GETCMAP:
		return gp->g_wsioctl->getcmap(gp, data);
	case WSDISPLAYIO_PUTCMAP:
		return gp->g_wsioctl->putcmap(gp, data);
	}

	return EPASSTHROUGH;
}

/* wsdisplay_accessops ioctls */

int 
grf_wsaogetcmap(void *c, void *data) 
{
	u_int index, count;
	struct grf_softc *gp;
	struct wsdisplay_cmap *cm __unused;

	cm = (struct wsdisplay_cmap*) data;
	gp = c;
	index = 0;
	count = 0;

	if (gp->g_wsmode == WSDISPLAYIO_MODE_EMUL)
		return EINVAL;

	if (index >= 255 || count > 256 || index + count > 256)
		return EINVAL;

	/* 
	 * TODO: copyout values for r, g, b. This function should be 
	 * driver-specific... 
	 */

	return 0;
}

int
grf_wsaoputcmap(void *c, void *data)
{
	/*
	 * We probably couldn't care less about color map in MODE_EMUL,
	 * I don't know about X11 yet. Also, these ioctls could be used by
	 * fullscreen console programs (think wsdisplay picture viewer, or
	 * the wsimgshow tool written by Yasushi Oshima).
	 */
	struct grf_softc *gp;

	gp = c;

	if (gp->g_wsmode == WSDISPLAYIO_MODE_EMUL)
		return EINVAL;
	/* ... */

	return 0;
}

int
grf_wsaosvideo(void *c, void *data)
{
#if 0
	struct grf_softc *gp;
	dev_t dev; 
	int rv;

	gp = c;
	dev = (dev_t) &gp->g_grfdev;

	if (*(u_int *)data == WSDISPLAYIO_VIDEO_OFF) {
		if ((gp->g_flags & GF_GRFON) == 0)
			rv = 0;
		else {
			gp->g_flags &= ~GF_GRFON;
			rv = gp->g_mode(gp, (dev & GRFOVDEV) ? 
			    GM_GRFOVOFF : GM_GRFOFF, NULL, 0, 0);
		}

	} else {
		if ((gp->g_flags & GF_GRFON))
        		rv = 0;
		else
			gp->g_flags |= GF_GRFON;
			rv = gp->g_mode(gp, (dev & GRFOVDEV) ? 
			    GM_GRFOVON : GM_GRFON, NULL, 0, 0);
	}

	return rv;
#endif
	return 0;
}

int
grf_wsaogvideo(void *c, void *data) 
{
	struct grf_softc *gp;

	gp = c;

	if(gp->g_flags & GF_GRFON) 
		*(u_int *)data = WSDISPLAYIO_VIDEO_ON;
	else
		*(u_int *)data = WSDISPLAYIO_VIDEO_OFF;

	return 0;
}

int
grf_wsaogtype(void *c, void *data)
{
	struct grf_softc *gp __unused;

	gp = c;

	*(u_int *)data = WSDISPLAY_TYPE_GRF;
	return 0;
}

int
grf_wsaogmode(void *c, void *data)
{
	struct grf_softc *gp;

	gp = c;

	*(u_int *)data = gp->g_wsmode;
	return 0;
}

int
grf_wsaosmode(void *c, void *data)
{
	/* XXX: should provide hw-dependent impl of this in grf_xxx driver? */
	struct grf_softc *gp;

	gp = c;

	if ((*(int*) data) != gp->g_wsmode) {
		gp->g_wsmode = (*(int*) data);
		if ((*(int*) data) == WSDISPLAYIO_MODE_EMUL) {
			//vcons_redraw_screen( active vcons screen );
		} 
	}
	return 0;
}

int
grf_wsaoginfo(void *c, void *data) 
{
	struct wsdisplay_fbinfo *fbinfo;
	struct grf_softc *gp;
	struct grfinfo *gi;

	gp = c;

	fbinfo = (struct wsdisplay_fbinfo *)data;
	gi = &gp->g_display;

	/*
	 * TODO: better sanity checking, it is possible that 
	 * wsdisplay is initialized, but no screen is opened
	 * (for example, device is not used).
	 */

	/*
	 * We shold return truth about current mode here because
	 * X11 wsfb driver denepds on this!
	 */
	fbinfo->height = gi->gd_fbheight;
	fbinfo->width = gi->gd_fbwidth;
	fbinfo->depth = gi->gd_planes;
	fbinfo->cmsize = gi->gd_colors;

	return 0;
}

#endif  /* NWSDISPLAY > 0 */

#ifdef DEBUG
void
grfdebug(struct grf_softc *gp, const char *fmt, ...)
{
	static int ccol = 0, crow = 1;
	volatile char *cp;
	char buf[256];
	va_list ap;
	int ncols;
	char *bp;

	va_start(ap, fmt);
	vsnprintf(buf, 256, fmt, ap);
	va_end(ap);

	cp = gp->g_fbkva;
	ncols = gp->g_display.gd_fbwidth / 8;
	cp += (crow * ncols + ccol) << 2;
	for (bp = buf; *bp != '\0'; bp++) {
		if (*bp == '\n') {
			ccol = 0;
			crow++;
			continue;
		}
		*cp++ = *bp;
		*cp = 0x0a;
		cp += 3;
		ccol++;
	}
}
#endif  /* DEBUG */

#endif	/* NGRF > 0 */

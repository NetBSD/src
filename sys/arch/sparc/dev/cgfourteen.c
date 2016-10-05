/*	$NetBSD: cgfourteen.c,v 1.79.4.3 2016/10/05 20:55:35 skrll Exp $ */

/*
 * Copyright (c) 1996
 *	The President and Fellows of Harvard College. All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by Harvard University.
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 *	This product includes software developed by Harvard University and
 *	its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *   Based on:
 *	NetBSD: cgthree.c,v 1.28 1996/05/31 09:59:22 pk Exp
 *	NetBSD: cgsix.c,v 1.25 1996/04/01 17:30:00 christos Exp
 */

/*
 * Driver for Campus-II on-board mbus-based video (cgfourteen).
 *
 * Does not handle interrupts, even though they can occur.
 *
 * XXX should defer colormap updates to vertical retrace interrupts
 */

/*
 * The following is for debugging only; it opens up a security hole
 * enabled by allowing any user to map the control registers for the
 * cg14 into their space.
 */
#undef CG14_MAP_REGS

#include "opt_wsemul.h"
#include "sx.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/mman.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <dev/pci/pciio.h>

#include <uvm/uvm_extern.h>

#include <dev/sun/fbio.h>
#include <machine/autoconf.h>
#include <machine/pmap.h>
#include <dev/sun/fbvar.h>
#include <machine/cpu.h>
#include <dev/sbus/sbusvar.h>

#include "wsdisplay.h"
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>

#include <dev/wscons/wsdisplay_vconsvar.h>
#include <dev/wscons/wsdisplay_glyphcachevar.h>

#include <sparc/sparc/asm.h>
#include <sparc/dev/cgfourteenreg.h>
#include <sparc/dev/cgfourteenvar.h>
#include <sparc/dev/sxreg.h>
#include <sparc/dev/sxvar.h>

/* autoconfiguration driver */
static int	cgfourteenmatch(device_t, struct cfdata *, void *);
static void	cgfourteenattach(device_t, device_t, void *);
static void	cgfourteenunblank(device_t);

CFATTACH_DECL_NEW(cgfourteen, sizeof(struct cgfourteen_softc),
    cgfourteenmatch, cgfourteenattach, NULL, NULL);

extern struct cfdriver cgfourteen_cd;

dev_type_open(cgfourteenopen);
dev_type_close(cgfourteenclose);
dev_type_ioctl(cgfourteenioctl);
dev_type_mmap(cgfourteenmmap);
dev_type_poll(cgfourteenpoll);

const struct cdevsw cgfourteen_cdevsw = {
        .d_open = cgfourteenopen,
	.d_close = cgfourteenclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = cgfourteenioctl,
        .d_stop = nostop,
	.d_tty = notty,
	.d_poll = cgfourteenpoll,
	.d_mmap = cgfourteenmmap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

/* frame buffer generic driver */
static struct fbdriver cgfourteenfbdriver = {
	cgfourteenunblank, cgfourteenopen, cgfourteenclose, cgfourteenioctl,
	cgfourteenpoll, cgfourteenmmap, nokqfilter
};

static void cg14_set_video(struct cgfourteen_softc *, int);
static int  cg14_get_video(struct cgfourteen_softc *);
static int  cg14_get_cmap(struct fbcmap *, union cg14cmap *, int);
static int  cg14_put_cmap(struct fbcmap *, union cg14cmap *, int);
static void cg14_load_hwcmap(struct cgfourteen_softc *, int, int);
static void cg14_init(struct cgfourteen_softc *);
static void cg14_reset(struct cgfourteen_softc *);

#if NWSDISPLAY > 0
static void cg14_setup_wsdisplay(struct cgfourteen_softc *, int);
static void cg14_init_cmap(struct cgfourteen_softc *);
static int  cg14_putcmap(struct cgfourteen_softc *, struct wsdisplay_cmap *);
static int  cg14_getcmap(struct cgfourteen_softc *, struct wsdisplay_cmap *);
static void cg14_set_depth(struct cgfourteen_softc *, int);
static void cg14_move_cursor(struct cgfourteen_softc *, int, int);
static int  cg14_do_cursor(struct cgfourteen_softc *,
                           struct wsdisplay_cursor *);

#if NSX > 0
static void cg14_wait_idle(struct cgfourteen_softc *);
static void cg14_rectfill(struct cgfourteen_softc *, int, int, int, int,
    uint32_t);
static void cg14_invert(struct cgfourteen_softc *, int, int, int, int);
static void cg14_bitblt(void *, int, int, int, int, int, int, int);
static void cg14_bitblt_gc(void *, int, int, int, int, int, int, int);

static void cg14_putchar_aa(void *, int, int, u_int, long);
static void cg14_cursor(void *, int, int, int);
static void cg14_putchar(void *, int, int, u_int, long);
static void cg14_copycols(void *, int, int, int, int);
static void cg14_erasecols(void *, int, int, int, long);
static void cg14_copyrows(void *, int, int, int);
static void cg14_eraserows(void *, int, int, long);
#endif /* NSX > 0 */

#endif

/*
 * Match a cgfourteen.
 */
int
cgfourteenmatch(device_t parent, struct cfdata *cf, void *aux)
{
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa = &uoba->uoba_sbus;

	/*
	 * The cgfourteen is a local-bus video adaptor, accessed directly
	 * via the processor, and not through device space or an external
	 * bus. Thus we look _only_ at the obio bus.
	 * Additionally, these things exist only on the Sun4m.
	 */

	if (uoba->uoba_isobio4 != 0 || !CPU_ISSUN4M)
		return (0);

	/* Check driver name */
	return (strcmp(cf->cf_name, sa->sa_name) == 0);
}

#if NWSDISPLAY > 0
static int	cg14_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t	cg14_mmap(void *, void *, off_t, int);
static void	cg14_init_screen(void *, struct vcons_screen *, int, long *);


struct wsdisplay_accessops cg14_accessops = {
	cg14_ioctl,
	cg14_mmap,
	NULL,	/* alloc_screen */
	NULL,	/* free_screen */
	NULL,	/* show_screen */
	NULL, 	/* load_font */
	NULL,	/* pollc */
	NULL	/* scroll */
};
#endif

/*
 * Attach a display.  We need to notice if it is the console, too.
 */
void
cgfourteenattach(device_t parent, device_t self, void *aux)
{
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa = &uoba->uoba_sbus;
	struct cgfourteen_softc *sc = device_private(self);
	struct fbdevice *fb = &sc->sc_fb;
	bus_space_handle_t bh;
	int node;
	volatile uint32_t *lut;
	int i, isconsole, items;
	uint32_t fbva[2] = {0, 0};
	uint32_t *ptr = fbva;
#if NSX > 0
	device_t dv;
	deviter_t di;
#endif

	sc->sc_dev = self;
	sc->sc_opens = 0;
	node = sa->sa_node;

	/* Remember cookies for cgfourteenmmap() */
	sc->sc_bustag = sa->sa_bustag;

	fb->fb_driver = &cgfourteenfbdriver;
	fb->fb_device = sc->sc_dev;
	/* Mask out invalid flags from the user. */
	fb->fb_flags = device_cfdata(sc->sc_dev)->cf_flags & FB_USERMASK;

	fb->fb_type.fb_type = FBTYPE_MDICOLOR;
	fb->fb_type.fb_depth = 32;

	fb_setsize_obp(fb, sc->sc_fb.fb_type.fb_depth, 1152, 900, node);

	fb->fb_type.fb_cmsize = CG14_CLUT_SIZE;

	if (sa->sa_nreg < 2) {
		printf("%s: only %d register sets\n",
			device_xname(self), sa->sa_nreg);
		return;
	}
	memcpy(sc->sc_physadr, sa->sa_reg,
	      sa->sa_nreg * sizeof(struct sbus_reg));

	sc->sc_vramsize = sc->sc_physadr[CG14_PXL_IDX].sbr_size;
	fb->fb_type.fb_size = sc->sc_vramsize;

	printf(": %d MB VRAM", (uint32_t)(sc->sc_vramsize >> 20));
	/*
	 * Now map in the 8 useful pages of registers
	 */
	if (sa->sa_size < 0x10000) {
#ifdef DIAGNOSTIC
		printf("warning: can't find all cgfourteen registers...\n");
#endif
		sa->sa_size = 0x10000;
	}
	if (sbus_bus_map(sa->sa_bustag, sa->sa_slot,
			 sa->sa_offset,
			 sa->sa_size,
			 0 /*BUS_SPACE_MAP_LINEAR*/,
			 &bh) != 0) {
		printf("%s: cannot map control registers\n",
		    device_xname(self));
		return;
	}
	sc->sc_regh = bh;
	sc->sc_regaddr = BUS_ADDR(sa->sa_slot, sa->sa_offset);
	sc->sc_fbaddr = BUS_ADDR(sc->sc_physadr[CG14_PXL_IDX].sbr_slot,
				sc->sc_physadr[CG14_PXL_IDX].sbr_offset);

	sc->sc_ctl   = (struct cg14ctl  *) (bh);
	sc->sc_hwc   = (struct cg14curs *) (bh + CG14_OFFSET_CURS);
	sc->sc_dac   = (struct cg14dac  *) (bh + CG14_OFFSET_DAC);
	sc->sc_xlut  = (struct cg14xlut *) (bh + CG14_OFFSET_XLUT);
	sc->sc_clut1 = (struct cg14clut *) (bh + CG14_OFFSET_CLUT1);
	sc->sc_clut2 = (struct cg14clut *) (bh + CG14_OFFSET_CLUT2);
	sc->sc_clut3 = (struct cg14clut *) (bh + CG14_OFFSET_CLUT3);
	sc->sc_clutincr =        (u_int *) (bh + CG14_OFFSET_CLUTINCR);

	/*
	 * Let the user know that we're here
	 */
	printf(": %dx%d",
		fb->fb_type.fb_width, fb->fb_type.fb_height);

	/*
	 * Enable the video.
	 */
	cg14_set_video(sc, 1);

	/*
	 * Grab the initial colormap
	 */
	lut = sc->sc_clut1->clut_lut;
	for (i = 0; i < CG14_CLUT_SIZE; i++)
		sc->sc_cmap.cm_chip[i] = lut[i];

	/* See if we're the console */
        isconsole = fb_is_console(node);

#if NWSDISPLAY > 0
	prom_getprop(sa->sa_node, "address", 4, &items, &ptr);
	if (fbva[1] == 0) {
		if (sbus_bus_map( sc->sc_bustag,
		    sc->sc_physadr[CG14_PXL_IDX].sbr_slot,
		    sc->sc_physadr[CG14_PXL_IDX].sbr_offset,
		    sc->sc_vramsize, BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_LARGE,
		    &bh) != 0) {
			printf("%s: cannot map pixels\n",
				device_xname(sc->sc_dev));
			return;
		}
		sc->sc_fb.fb_pixels = bus_space_vaddr(sc->sc_bustag, bh);
	} else {
		sc->sc_fb.fb_pixels = (void *)fbva[1];
	}

	if (isconsole)
		printf(" (console)\n");
	else
		printf("\n");

	sc->sc_depth = 8;

#if NSX > 0
	/* see if we've got an SX to help us */
	sc->sc_sx = NULL;
	for (dv = deviter_first(&di, DEVITER_F_ROOT_FIRST);
	    dv != NULL;
	    dv = deviter_next(&di)) {
		if (device_is_a(dv, "sx")) {
			sc->sc_sx = device_private(dv);
		}
	}
	deviter_release(&di);
	if (sc->sc_sx != NULL) {
		sc->sc_fb_paddr = bus_space_mmap(sc->sc_bustag,
		    sc->sc_fbaddr, 0, 0, 0) & 0xfffff000;
		aprint_normal_dev(sc->sc_dev, "using %s\n",
		    device_xname(sc->sc_sx->sc_dev));
		aprint_debug_dev(sc->sc_dev, "fb paddr: %08x\n",
		    sc->sc_fb_paddr);
		sx_write(sc->sc_sx, SX_PAGE_BOUND_LOWER, sc->sc_fb_paddr);
		sx_write(sc->sc_sx, SX_PAGE_BOUND_UPPER,
		    sc->sc_fb_paddr + 0x03ffffff);
	}
	cg14_wait_idle(sc);
#endif
	cg14_setup_wsdisplay(sc, isconsole);
#endif

	/* Attach to /dev/fb */
	fb_attach(&sc->sc_fb, isconsole);
}

/*
 * Keep track of the number of opens made. In the 24-bit driver, we need to
 * switch to 24-bit mode on the first open, and switch back to 8-bit on
 * the last close. This kind of nonsense is needed to give screenblank
 * a fighting chance of working.
 */

int
cgfourteenopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct cgfourteen_softc *sc;
	int oldopens;

	sc = device_lookup_private(&cgfourteen_cd, minor(dev));
	if (sc == NULL)
		return(ENXIO);
	oldopens = sc->sc_opens++;

	/* Setup the cg14 as we want it, and save the original PROM state */
	if (oldopens == 0)	/* first open only, to make screenblank work */
		cg14_init(sc);

	return (0);
}

int
cgfourteenclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct cgfourteen_softc *sc =
	    device_lookup_private(&cgfourteen_cd, minor(dev));
	int opens;

	opens = --sc->sc_opens;
	if (sc->sc_opens < 0)
		opens = sc->sc_opens = 0;

	/*
	 * Restore video state to make the PROM happy, on last close.
	 */
	if (opens == 0) {
		cg14_reset(sc);
#if NSX > 0
		if (sc->sc_sx)
			glyphcache_wipe(&sc->sc_gc);
#endif
	}
	return (0);
}

int
cgfourteenioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	struct cgfourteen_softc *sc =
	    device_lookup_private(&cgfourteen_cd, minor(dev));
	struct fbgattr *fba;
	int error;

	switch (cmd) {

	case FBIOGTYPE:
		*(struct fbtype *)data = sc->sc_fb.fb_type;
		break;

	case FBIOGATTR:
		fba = (struct fbgattr *)data;
		fba->real_type = FBTYPE_MDICOLOR;
		fba->owner = 0;		/* XXX ??? */
		fba->fbtype = sc->sc_fb.fb_type;
		fba->sattr.flags = 0;
		fba->sattr.emu_type = sc->sc_fb.fb_type.fb_type;
		fba->sattr.dev_specific[0] = -1;
		fba->emu_types[0] = sc->sc_fb.fb_type.fb_type;
		fba->emu_types[1] = -1;
		break;

	case FBIOGETCMAP:
		return(cg14_get_cmap((struct fbcmap *)data, &sc->sc_cmap,
				     CG14_CLUT_SIZE));

	case FBIOPUTCMAP:
		/* copy to software map */
#define p ((struct fbcmap *)data)
		error = cg14_put_cmap(p, &sc->sc_cmap, CG14_CLUT_SIZE);
		if (error)
			return (error);
		/* now blast them into the chip */
		/* XXX should use retrace interrupt */
		cg14_load_hwcmap(sc, p->index, p->count);
#undef p
		break;

	case FBIOGVIDEO:
		*(int *)data = cg14_get_video(sc);
		break;

	case FBIOSVIDEO:
		cg14_set_video(sc, *(int *)data);
		break;

	case CG14_SET_PIXELMODE: {
		int depth = *(int *)data;

		if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)
			return EINVAL;

		cg14_set_depth(sc, depth);
		}
		break;
	default:
		return (ENOTTY);
	}
	return (0);
}

/*
 * Undo the effect of an FBIOSVIDEO that turns the video off.
 */
static void
cgfourteenunblank(device_t dev)
{
	struct cgfourteen_softc *sc = device_private(dev);

	cg14_set_video(sc, 1);
#if NWSDISPLAY > 0
	if (sc->sc_mode != WSDISPLAYIO_MODE_EMUL) {
		cg14_set_depth(sc, 8);
		cg14_init_cmap(sc);
		vcons_redraw_screen(sc->sc_vd.active);
		sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	}
#endif
}

/*
 * Return the address that would map the given device at the given
 * offset, allowing for the given protection, or return -1 for error.
  */
paddr_t
cgfourteenmmap(dev_t dev, off_t off, int prot)
{
	struct cgfourteen_softc *sc =
	    device_lookup_private(&cgfourteen_cd, minor(dev));
	off_t offset = -1;

	if (off & PGOFSET)
		panic("cgfourteenmmap");

	if (off < 0)
		return (-1);

	if (off >= 0 && off < 0x10000) {
		offset = sc->sc_regaddr;
	} else if (off >= CG14_CURSOR_VOFF &&
		   off < (CG14_CURSOR_VOFF + 0x1000)) {
		offset = sc->sc_regaddr + CG14_OFFSET_CURS;
		off -= CG14_CURSOR_VOFF;
	} else if (off >= CG14_DIRECT_VOFF &&
		   off < (CG14_DIRECT_VOFF + sc->sc_vramsize)) {
		offset = sc->sc_fbaddr + CG14_FB_VRAM;
		off -= CG14_DIRECT_VOFF;
	} else if (off >= CG14_BGR_VOFF &&
		   off < (CG14_BGR_VOFF + sc->sc_vramsize)) {
		offset = sc->sc_fbaddr + CG14_FB_CBGR;
		off -= CG14_BGR_VOFF;
	} else if (off >= CG14_X32_VOFF &&
		   off < (CG14_X32_VOFF + (sc->sc_vramsize >> 2))) {
		offset = sc->sc_fbaddr + CG14_FB_PX32;
		off -= CG14_X32_VOFF;
	} else if (off >= CG14_B32_VOFF &&
		   off < (CG14_B32_VOFF + (sc->sc_vramsize >> 2))) {
		offset = sc->sc_fbaddr + CG14_FB_PB32;
		off -= CG14_B32_VOFF;
	} else if (off >= CG14_G32_VOFF &&
		   off < (CG14_G32_VOFF + (sc->sc_vramsize >> 2))) {
		offset = sc->sc_fbaddr + CG14_FB_PG32;
		off -= CG14_G32_VOFF;
	} else if (off >= CG14_R32_VOFF &&
		   off < CG14_R32_VOFF + (sc->sc_vramsize >> 2)) {
		offset = sc->sc_fbaddr + CG14_FB_PR32;
		off -= CG14_R32_VOFF;
#if NSX > 0
	/*
	 * for convenience we also map the SX ranges here:
	 * - one page userland registers
	 * - CG14-sized IO space at 0x800000000 ( not a typo, it's above 4GB )
	 */
	} else if (sc->sc_sx == NULL) {
		return -1;
	} else if (off >= CG14_SXREG_VOFF &&
		   off < (CG14_SXREG_VOFF + 0x400)) {
		return (bus_space_mmap(sc->sc_sx->sc_tag, sc->sc_sx->sc_uregs,
			0, prot, BUS_SPACE_MAP_LINEAR));
	} else if (off >= CG14_SXIO_VOFF &&
		   off < (CG14_SXIO_VOFF + 0x03ffffff)) {
		return (bus_space_mmap(sc->sc_sx->sc_tag, 0x800000000LL,
			sc->sc_fb_paddr + (off - CG14_SXIO_VOFF),
			prot, BUS_SPACE_MAP_LINEAR));
#endif
	} else
		return -1;

	return (bus_space_mmap(sc->sc_bustag, offset, off, prot,
		    BUS_SPACE_MAP_LINEAR));
}

int
cgfourteenpoll(dev_t dev, int events, struct lwp *l)
{

	return (seltrue(dev, events, l));
}

/*
 * Miscellaneous helper functions
 */

/* Initialize the framebuffer, storing away useful state for later reset */
static void
cg14_init(struct cgfourteen_softc *sc)
{
	cg14_set_depth(sc, 32);
}

static void
/* Restore the state saved on cg14_init */
cg14_reset(struct cgfourteen_softc *sc)
{
	cg14_set_depth(sc, 8);
}

/* Enable/disable video display; power down monitor if DPMS-capable */
static void
cg14_set_video(struct cgfourteen_softc *sc, int enable)
{
	/*
	 * We can only use DPMS to power down the display if the chip revision
	 * is greater than 0.
	 */
	if (enable) {
		if ((sc->sc_ctl->ctl_rsr & CG14_RSR_REVMASK) > 0)
			sc->sc_ctl->ctl_mctl |= (CG14_MCTL_ENABLEVID |
						 CG14_MCTL_POWERCTL);
		else
			sc->sc_ctl->ctl_mctl |= CG14_MCTL_ENABLEVID;
	} else {
		if ((sc->sc_ctl->ctl_rsr & CG14_RSR_REVMASK) > 0)
			sc->sc_ctl->ctl_mctl &= ~(CG14_MCTL_ENABLEVID |
						  CG14_MCTL_POWERCTL);
		else
			sc->sc_ctl->ctl_mctl &= ~CG14_MCTL_ENABLEVID;
	}
}

/* Get status of video display */
static int
cg14_get_video(struct cgfourteen_softc *sc)
{
	return ((sc->sc_ctl->ctl_mctl & CG14_MCTL_ENABLEVID) != 0);
}

/* Read the software shadow colormap */
static int
cg14_get_cmap(struct fbcmap *p, union cg14cmap *cm, int cmsize)
{
        u_int i, start, count;
        u_char *cp;
        int error;

        start = p->index;
        count = p->count;
        if (start >= cmsize || count > cmsize - start)
                return (EINVAL);

        for (cp = &cm->cm_map[start][0], i = 0; i < count; cp += 4, i++) {
                error = copyout(&cp[3], &p->red[i], 1);
                if (error)
                        return error;
                error = copyout(&cp[2], &p->green[i], 1);
                if (error)
                        return error;
                error = copyout(&cp[1], &p->blue[i], 1);
                if (error)
                        return error;
        }
        return (0);
}

/* Write the software shadow colormap */
static int
cg14_put_cmap(struct fbcmap *p, union cg14cmap *cm, int cmsize)
{
        u_int i, start, count;
        u_char *cp;
        u_char cmap[256][4];
        int error;

        start = p->index;
        count = p->count;
        if (start >= cmsize || count > cmsize - start)
                return (EINVAL);

        memcpy(&cmap, &cm->cm_map, sizeof cmap);
        for (cp = &cmap[start][0], i = 0; i < count; cp += 4, i++) {
                error = copyin(&p->red[i], &cp[3], 1);
                if (error)
                        return error;
                error = copyin(&p->green[i], &cp[2], 1);
                if (error)
                        return error;
                error = copyin(&p->blue[i], &cp[1], 1);
                if (error)
                        return error;
                cp[0] = 0;      /* no alpha channel */
        }
        memcpy(&cm->cm_map, &cmap, sizeof cmap);
        return (0);
}

static void
cg14_load_hwcmap(struct cgfourteen_softc *sc, int start, int ncolors)
{
	/* XXX switch to auto-increment, and on retrace intr */

	/* Setup pointers to source and dest */
	uint32_t *colp = &sc->sc_cmap.cm_chip[start];
	volatile uint32_t *lutp = &sc->sc_clut1->clut_lut[start];

	/* Copy by words */
	while (--ncolors >= 0)
		*lutp++ = *colp++;
}

static void
cg14_setup_wsdisplay(struct cgfourteen_softc *sc, int is_cons)
{
	struct wsemuldisplaydev_attach_args aa;
	struct rasops_info *ri;
	long defattr;

 	sc->sc_defaultscreen_descr = (struct wsscreen_descr){
		"default",
		0, 0,
		NULL,
		8, 16,
		WSSCREEN_WSCOLORS | WSSCREEN_HILIT,
		NULL
	};
	cg14_set_depth(sc, 8);
	sc->sc_screens[0] = &sc->sc_defaultscreen_descr;
	sc->sc_screenlist = (struct wsscreen_list){1, sc->sc_screens};
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	vcons_init(&sc->sc_vd, sc, &sc->sc_defaultscreen_descr,
	    &cg14_accessops);
	sc->sc_vd.init_screen = cg14_init_screen;

	ri = &sc->sc_console_screen.scr_ri;

	sc->sc_gc.gc_bitblt = cg14_bitblt_gc;
	sc->sc_gc.gc_blitcookie = sc;
	sc->sc_gc.gc_rop = 0xc;
	if (is_cons) {
		vcons_init_screen(&sc->sc_vd, &sc->sc_console_screen, 1,
		    &defattr);

		/* clear the screen with the default background colour */
		if (sc->sc_sx != NULL) {
			cg14_rectfill(sc, 0, 0, ri->ri_width, ri->ri_height,
				ri->ri_devcmap[(defattr >> 16) & 0xf]);
		} else {
			memset(sc->sc_fb.fb_pixels,
			       ri->ri_devcmap[(defattr >> 16) & 0xf],
			       ri->ri_stride * ri->ri_height);
		}
		sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

		sc->sc_defaultscreen_descr.textops = &ri->ri_ops;
		sc->sc_defaultscreen_descr.capabilities = ri->ri_caps;
		sc->sc_defaultscreen_descr.nrows = ri->ri_rows;
		sc->sc_defaultscreen_descr.ncols = ri->ri_cols;
		glyphcache_init(&sc->sc_gc, sc->sc_fb.fb_type.fb_height + 5,
			(sc->sc_vramsize / sc->sc_fb.fb_type.fb_width) -
			 sc->sc_fb.fb_type.fb_height - 5,
			sc->sc_fb.fb_type.fb_width,
			ri->ri_font->fontwidth,
			ri->ri_font->fontheight,
			defattr);
		wsdisplay_cnattach(&sc->sc_defaultscreen_descr, ri, 0, 0,
		    defattr);
		vcons_replay_msgbuf(&sc->sc_console_screen);
	} else {
		/*
		 * since we're not the console we can postpone the rest
		 * until someone actually allocates a screen for us
		 */
		glyphcache_init(&sc->sc_gc, sc->sc_fb.fb_type.fb_height + 5,
			(sc->sc_vramsize / sc->sc_fb.fb_type.fb_width) -
			 sc->sc_fb.fb_type.fb_height - 5,
			sc->sc_fb.fb_type.fb_width,
			ri->ri_font->fontwidth,
			ri->ri_font->fontheight,
			DEFATTR);
	}

	cg14_init_cmap(sc);

	aa.console = is_cons;
	aa.scrdata = &sc->sc_screenlist;
	aa.accessops = &cg14_accessops;
	aa.accesscookie = &sc->sc_vd;

	config_found(sc->sc_dev, &aa, wsemuldisplaydevprint);
}

static void
cg14_init_cmap(struct cgfourteen_softc *sc)
{
	struct rasops_info *ri = &sc->sc_console_screen.scr_ri;
	int i, j = 0;
	uint8_t cmap[768];

	rasops_get_cmap(ri, cmap, sizeof(cmap));

	for (i = 0; i < 256; i++) {

		sc->sc_cmap.cm_map[i][3] = cmap[j];
		sc->sc_cmap.cm_map[i][2] = cmap[j + 1];
		sc->sc_cmap.cm_map[i][1] = cmap[j + 2];
		j += 3;
	}
	cg14_load_hwcmap(sc, 0, 256);
}

static int
cg14_putcmap(struct cgfourteen_softc *sc, struct wsdisplay_cmap *cm)
{
	u_int index = cm->index;
	u_int count = cm->count;
	int i, error;
	u_char rbuf[256], gbuf[256], bbuf[256];

	if (cm->index >= 256 || cm->count > 256 ||
	    (cm->index + cm->count) > 256)
		return EINVAL;
	error = copyin(cm->red, &rbuf[index], count);
	if (error)
		return error;
	error = copyin(cm->green, &gbuf[index], count);
	if (error)
		return error;
	error = copyin(cm->blue, &bbuf[index], count);
	if (error)
		return error;

	for (i = 0; i < count; i++) {
		sc->sc_cmap.cm_map[index][3] = rbuf[index];
		sc->sc_cmap.cm_map[index][2] = gbuf[index];
		sc->sc_cmap.cm_map[index][1] = bbuf[index];

		index++;
	}
	cg14_load_hwcmap(sc, 0, 256);
	return 0;
}

static int
cg14_getcmap(struct cgfourteen_softc *sc, struct wsdisplay_cmap *cm)
{
	uint8_t rbuf[256], gbuf[256], bbuf[256];
	u_int index = cm->index;
	u_int count = cm->count;
	int error, i;

	if (index >= 255 || count > 256 || index + count > 256)
		return EINVAL;


	for (i = 0; i < count; i++) {
		rbuf[i] = sc->sc_cmap.cm_map[index][3];
		gbuf[i] = sc->sc_cmap.cm_map[index][2];
		bbuf[i] = sc->sc_cmap.cm_map[index][1];

		index++;
	}
	error = copyout(rbuf,   cm->red,   count);
	if (error)
		return error;
	error = copyout(gbuf, cm->green, count);
	if (error)
		return error;
	error = copyout(bbuf,  cm->blue,  count);
	if (error)
		return error;

	return 0;
}

static int
cg14_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct vcons_data *vd = v;
	struct cgfourteen_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;

	switch (cmd) {

		case WSDISPLAYIO_GTYPE:
			*(uint32_t *)data = WSDISPLAY_TYPE_SUNCG14;
			return 0;

		case WSDISPLAYIO_GINFO:
			wdf = (void *)data;
			wdf->height = ms->scr_ri.ri_height;
			wdf->width = ms->scr_ri.ri_width;
			wdf->depth = 32;
			wdf->cmsize = 256;
			return 0;

		case WSDISPLAYIO_GETCMAP:
			return cg14_getcmap(sc,
			    (struct wsdisplay_cmap *)data);

		case WSDISPLAYIO_PUTCMAP:
			return cg14_putcmap(sc,
			    (struct wsdisplay_cmap *)data);

		case WSDISPLAYIO_LINEBYTES:
			*(u_int *)data = ms->scr_ri.ri_stride << 2;
			return 0;

		case WSDISPLAYIO_SMODE:
			{
				int new_mode = *(int*)data;
				if (new_mode != sc->sc_mode) {
					sc->sc_mode = new_mode;
					if(new_mode == WSDISPLAYIO_MODE_EMUL) {
						bus_space_write_1(sc->sc_bustag,
						    sc->sc_regh,
						    CG14_CURSOR_CONTROL, 0);

						cg14_set_depth(sc, 8);
						cg14_init_cmap(sc);
#if NSX > 0
						if (sc->sc_sx)
							glyphcache_wipe(&sc->sc_gc);
#endif
						vcons_redraw_screen(ms);
					} else {

						cg14_set_depth(sc, 32);
					}
				}
			}
			return 0;
		case WSDISPLAYIO_SVIDEO:
			cg14_set_video(sc, *(int *)data);
			return 0;
		case WSDISPLAYIO_GVIDEO:
			return cg14_get_video(sc) ?
			    WSDISPLAYIO_VIDEO_ON : WSDISPLAYIO_VIDEO_OFF;
		case WSDISPLAYIO_GCURPOS:
			{
				struct wsdisplay_curpos *cp = (void *)data;

				cp->x = sc->sc_cursor.cc_pos.x;
				cp->y = sc->sc_cursor.cc_pos.y;
			}
			return 0;
		case WSDISPLAYIO_SCURPOS:
			{
				struct wsdisplay_curpos *cp = (void *)data;

				cg14_move_cursor(sc, cp->x, cp->y);
			}
			return 0;
		case WSDISPLAYIO_GCURMAX:
			{
				struct wsdisplay_curpos *cp = (void *)data;

				cp->x = 32;
				cp->y = 32;
			}
			return 0;
		case WSDISPLAYIO_SCURSOR:
			{
				struct wsdisplay_cursor *cursor = (void *)data;

				return cg14_do_cursor(sc, cursor);
			}
		case PCI_IOC_CFGREAD:
		case PCI_IOC_CFGWRITE:
			return EINVAL;

	}
	return EPASSTHROUGH;
}

static paddr_t
cg14_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct cgfourteen_softc *sc = vd->cookie;

	/* allow mmap()ing the full framebuffer, not just what we use */
	if (offset < sc->sc_vramsize)
		return bus_space_mmap(sc->sc_bustag,
		    BUS_ADDR(sc->sc_physadr[CG14_PXL_IDX].sbr_slot,
		      sc->sc_physadr[CG14_PXL_IDX].sbr_offset),
		    offset + CG14_FB_CBGR, prot, BUS_SPACE_MAP_LINEAR);

	return -1;
}

static void
cg14_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct cgfourteen_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = 8;
	ri->ri_width = sc->sc_fb.fb_type.fb_width;
	ri->ri_height = sc->sc_fb.fb_type.fb_height;
	ri->ri_stride = ri->ri_width;
	ri->ri_flg = RI_CENTER | RI_FULLCLEAR;

	ri->ri_bits = (char *)sc->sc_fb.fb_pixels;
#if NSX > 0
	ri->ri_flg |= RI_8BIT_IS_RGB | RI_ENABLE_ALPHA;

	/*
	 * unaligned copies with horizontal overlap are slow, so don't bother
	 * handling them in cg14_bitblt() and use putchar() instead
	 */
	if (sc->sc_sx != NULL) {
		scr->scr_flags |= VCONS_NO_COPYCOLS;
	} else
#endif
	scr->scr_flags |= VCONS_DONT_READ;

	if (existing) {
		ri->ri_flg |= RI_CLEAR;
	}

	rasops_init(ri, 0, 0);
	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri,
	    sc->sc_fb.fb_type.fb_height / ri->ri_font->fontheight,
	    sc->sc_fb.fb_type.fb_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
#if NSX > 0
	if (sc->sc_sx != NULL) {
		ri->ri_ops.copyrows = cg14_copyrows;
		ri->ri_ops.copycols = cg14_copycols;
		ri->ri_ops.eraserows = cg14_eraserows;
		ri->ri_ops.erasecols = cg14_erasecols;
		ri->ri_ops.cursor = cg14_cursor;
		if (FONT_IS_ALPHA(ri->ri_font)) {
			ri->ri_ops.putchar = cg14_putchar_aa;
		} else
			ri->ri_ops.putchar = cg14_putchar;
	}
#endif /* NSX > 0 */
}

static void
cg14_set_depth(struct cgfourteen_softc *sc, int depth)
{
	int i;

	if (sc->sc_depth == depth)
		return;

	switch (depth) {
		case 8:
			bus_space_write_1(sc->sc_bustag, sc->sc_regh,
			    CG14_MCTL, CG14_MCTL_ENABLEVID |
			    CG14_MCTL_PIXMODE_8 | CG14_MCTL_POWERCTL);
			sc->sc_depth = 8;
			/* everything is CLUT1 */
			for (i = 0; i < CG14_CLUT_SIZE; i++)
			     sc->sc_xlut->xlut_lut[i] = 0;
			break;
		case 32:
			bus_space_write_1(sc->sc_bustag, sc->sc_regh,
			    CG14_MCTL, CG14_MCTL_ENABLEVID |
			    CG14_MCTL_PIXMODE_32 | CG14_MCTL_POWERCTL);
			sc->sc_depth = 32;
			for (i = 0; i < CG14_CLUT_SIZE; i++)
			     sc->sc_xlut->xlut_lut[i] = 0;
			break;
		default:
			printf("%s: can't change to depth %d\n",
			    device_xname(sc->sc_dev), depth);
	}
}

static void
cg14_move_cursor(struct cgfourteen_softc *sc, int x, int y)
{
	uint32_t pos;

	sc->sc_cursor.cc_pos.x = x;
	sc->sc_cursor.cc_pos.y = y;
	pos = ((sc->sc_cursor.cc_pos.x - sc->sc_cursor.cc_hot.x ) << 16) |
	      ((sc->sc_cursor.cc_pos.y - sc->sc_cursor.cc_hot.y ) & 0xffff);
	bus_space_write_4(sc->sc_bustag, sc->sc_regh, CG14_CURSOR_X, pos);
}

static int
cg14_do_cursor(struct cgfourteen_softc *sc, struct wsdisplay_cursor *cur)
{
	if (cur->which & WSDISPLAY_CURSOR_DOCUR) {

		bus_space_write_1(sc->sc_bustag, sc->sc_regh,
		    CG14_CURSOR_CONTROL, cur->enable ? CG14_CRSR_ENABLE : 0);
	}
	if (cur->which & WSDISPLAY_CURSOR_DOHOT) {

		sc->sc_cursor.cc_hot.x = cur->hot.x;
		sc->sc_cursor.cc_hot.y = cur->hot.y;
		cur->which |= WSDISPLAY_CURSOR_DOPOS;
	}
	if (cur->which & WSDISPLAY_CURSOR_DOPOS) {

		cg14_move_cursor(sc, cur->pos.x, cur->pos.y);
	}
	if (cur->which & WSDISPLAY_CURSOR_DOCMAP) {
		int i;
		uint32_t val;

		for (i = 0; i < min(cur->cmap.count, 3); i++) {
			val = (cur->cmap.red[i] ) |
			      (cur->cmap.green[i] << 8) |
			      (cur->cmap.blue[i] << 16);
			bus_space_write_4(sc->sc_bustag, sc->sc_regh,
			    CG14_CURSOR_COLOR1 + ((i + cur->cmap.index) << 2),
			    val);
		}
	}
	if (cur->which & WSDISPLAY_CURSOR_DOSHAPE) {
		uint32_t buffer[32], latch, tmp;
		int i;

		copyin(cur->mask, buffer, 128);
		for (i = 0; i < 32; i++) {
			latch = 0;
			tmp = buffer[i] & 0x80808080;
			latch |= tmp >> 7;
			tmp = buffer[i] & 0x40404040;
			latch |= tmp >> 5;
			tmp = buffer[i] & 0x20202020;
			latch |= tmp >> 3;
			tmp = buffer[i] & 0x10101010;
			latch |= tmp >> 1;
			tmp = buffer[i] & 0x08080808;
			latch |= tmp << 1;
			tmp = buffer[i] & 0x04040404;
			latch |= tmp << 3;
			tmp = buffer[i] & 0x02020202;
			latch |= tmp << 5;
			tmp = buffer[i] & 0x01010101;
			latch |= tmp << 7;
			bus_space_write_4(sc->sc_bustag, sc->sc_regh,
			    CG14_CURSOR_PLANE0 + (i << 2), latch);
		}
		copyin(cur->image, buffer, 128);
		for (i = 0; i < 32; i++) {
			latch = 0;
			tmp = buffer[i] & 0x80808080;
			latch |= tmp >> 7;
			tmp = buffer[i] & 0x40404040;
			latch |= tmp >> 5;
			tmp = buffer[i] & 0x20202020;
			latch |= tmp >> 3;
			tmp = buffer[i] & 0x10101010;
			latch |= tmp >> 1;
			tmp = buffer[i] & 0x08080808;
			latch |= tmp << 1;
			tmp = buffer[i] & 0x04040404;
			latch |= tmp << 3;
			tmp = buffer[i] & 0x02020202;
			latch |= tmp << 5;
			tmp = buffer[i] & 0x01010101;
			latch |= tmp << 7;
			bus_space_write_4(sc->sc_bustag, sc->sc_regh,
			    CG14_CURSOR_PLANE1 + (i << 2), latch);
		}
	}
	return 0;
}

#if NSX > 0

static void
cg14_wait_idle(struct cgfourteen_softc *sc)
{
}

static void
cg14_rectfill(struct cgfourteen_softc *sc, int x, int y, int wi, int he,
     uint32_t colour)
{
	uint32_t addr, pptr;
	int line, cnt, pre, words;
	int stride = sc->sc_fb.fb_type.fb_width;

	addr = sc->sc_fb_paddr + x + stride * y;
	sx_write(sc->sc_sx, SX_QUEUED(8), colour);
	sx_write(sc->sc_sx, SX_QUEUED(9), colour);
	/*
	 * Calculate the number of pixels we need to do one by one
	 * until we're 32bit aligned, then do the rest in 32bit
	 * mode. Assumes that stride is always a multiple of 4.
	 */
	/* TODO: use 32bit writes with byte mask instead */
	pre = addr & 3;
	if (pre != 0) pre = 4 - pre;
	for (line = 0; line < he; line++) {
		pptr = addr;
		cnt = wi;
		if (pre) {
			sta(pptr & ~7, ASI_SX, SX_STBS(8, pre - 1, pptr & 7));
			pptr += pre;
			cnt -= pre;
		}
		/* now do the aligned pixels in 32bit chunks */
		while(cnt > 31) {
			words = min(32, cnt >> 2);
			sta(pptr & ~7, ASI_SX, SX_STS(8, words - 1, pptr & 7));
			pptr += words << 2;
			cnt -= words << 2;
		}
		/* do any remaining pixels byte-wise again */
		if (cnt > 0)
			sta(pptr & ~7, ASI_SX, SX_STBS(8, cnt - 1, pptr & 7));
		addr += stride;
	}
}

static void
cg14_invert(struct cgfourteen_softc *sc, int x, int y, int wi, int he)
{
	uint32_t addr, pptr;
	int line, cnt, pre, words;
	int stride = sc->sc_fb.fb_type.fb_width;

	addr = sc->sc_fb_paddr + x + stride * y;
	sx_write(sc->sc_sx, SX_ROP_CONTROL, 0x33); /* ~src a */
	/*
	 * Calculate the number of pixels we need to do one by one
	 * until we're 32bit aligned, then do the rest in 32bit
	 * mode. Assumes that stride is always a multiple of 4.
	 */
	/* TODO: use 32bit writes with byte mask instead */
	pre = addr & 3;
	if (pre != 0) pre = 4 - pre;
	for (line = 0; line < he; line++) {
		pptr = addr;
		cnt = wi;
		if (pre) {
			sta(pptr & ~7, ASI_SX, SX_LDB(8, pre - 1, pptr & 7));
			sx_write(sc->sc_sx, SX_INSTRUCTIONS,
			    SX_ROP(8, 8, 32, pre - 1));
			sta(pptr & ~7, ASI_SX, SX_STB(32, pre - 1, pptr & 7));
			pptr += pre;
			cnt -= pre;
		}
		/* now do the aligned pixels in 32bit chunks */
		while(cnt > 15) {
			words = min(16, cnt >> 2);
			sta(pptr & ~7, ASI_SX, SX_LD(8, words - 1, pptr & 7));
			sx_write(sc->sc_sx, SX_INSTRUCTIONS,
			    SX_ROP(8, 8, 32, words - 1));
			sta(pptr & ~7, ASI_SX, SX_ST(32, words - 1, pptr & 7));
			pptr += words << 2;
			cnt -= words << 2;
		}
		/* do any remaining pixels byte-wise again */
		if (cnt > 0)
			sta(pptr & ~7, ASI_SX, SX_LDB(8, cnt - 1, pptr & 7));
			sx_write(sc->sc_sx, SX_INSTRUCTIONS,
			    SX_ROP(8, 8, 32, cnt - 1));
			sta(pptr & ~7, ASI_SX, SX_STB(32, cnt - 1, pptr & 7));
		addr += stride;
	}
}

static inline void
cg14_slurp(int reg, uint32_t addr, int cnt)
{
	int num;
	while (cnt > 0) {
		num = min(32, cnt);
		sta(addr & ~7, ASI_SX, SX_LD(reg, num - 1, addr & 7));
		cnt -= num;
		reg += num;
		addr += (num << 2);
	}
}

static inline void
cg14_spit(int reg, uint32_t addr, int cnt)
{
	int num;
	while (cnt > 0) {
		num = min(32, cnt);
		sta(addr & ~7, ASI_SX, SX_ST(reg, num - 1, addr & 7));
		cnt -= num;
		reg += num;
		addr += (num << 2);
	}
}

static void
cg14_bitblt(void *cookie, int xs, int ys, int xd, int yd,
    int wi, int he, int rop)
{
	struct cgfourteen_softc *sc = cookie;
	uint32_t saddr, daddr, sptr, dptr;
	int line, cnt, stride = sc->sc_fb.fb_type.fb_width;
	int num, words, skip;

	if (ys < yd) {
		/* need to go bottom-up */
		saddr = sc->sc_fb_paddr + xs + stride * (ys + he - 1);
		daddr = sc->sc_fb_paddr + xd + stride * (yd + he - 1);
		skip = -stride;
	} else {
		saddr = sc->sc_fb_paddr + xs + stride * ys;
		daddr = sc->sc_fb_paddr + xd + stride * yd;
		skip = stride;
	}

	if ((saddr & 3) == (daddr & 3)) {
		int pre = saddr & 3;	/* pixels to copy byte-wise */
		if (pre != 0) pre = 4 - pre;
		for (line = 0; line < he; line++) {
			sptr = saddr;
			dptr = daddr;
			cnt = wi;
			if (pre > 0) {
				sta(sptr & ~7, ASI_SX,
				    SX_LDB(32, pre - 1, sptr & 7));
				sta(dptr & ~7, ASI_SX,
				    SX_STB(32, pre - 1, dptr & 7));
				cnt -= pre;
				sptr += pre;
				dptr += pre;
			}
			words = cnt >> 2;
			while(cnt > 3) {
				num = min(120, words);
				cg14_slurp(8, sptr, num);
				cg14_spit(8, dptr, num);
				sptr += num << 2;
				dptr += num << 2;
				cnt -= num << 2;
			}
			if (cnt > 0) {
				sta(sptr & ~7, ASI_SX,
				    SX_LDB(32, cnt - 1, sptr & 7));
				sta(dptr & ~7, ASI_SX,
				    SX_STB(32, cnt - 1, dptr & 7));
			}
			saddr += skip;
			daddr += skip;
		}
	} else {
		/* unaligned, have to use byte mode */
		/* funnel shifter & byte mask trickery? */
		for (line = 0; line < he; line++) {
			sptr = saddr;
			dptr = daddr;
			cnt = wi;
			while(cnt > 31) {
				sta(sptr & ~7, ASI_SX, SX_LDB(32, 31, sptr & 7));
				sta(dptr & ~7, ASI_SX, SX_STB(32, 31, dptr & 7));
				sptr += 32;
				dptr += 32;
				cnt -= 32;
			}
			if (cnt > 0) {
				sta(sptr & ~7, ASI_SX,
				    SX_LDB(32, cnt - 1, sptr & 7));
				sta(dptr & ~7, ASI_SX,
				    SX_STB(32, cnt - 1, dptr & 7));
			}
			saddr += skip;
			daddr += skip;
		}
	}
}

/*
 * for copying glyphs around
 * - uses all quads for reads
 * - uses quads for writes as far as possible
 * - limited by number of registers - won't do more than 120 wide
 * - doesn't handle overlaps
 */
static void
cg14_bitblt_gc(void *cookie, int xs, int ys, int xd, int yd,
    int wi, int he, int rop)
{
	struct cgfourteen_softc *sc = cookie;
	uint32_t saddr, daddr;
	int line, cnt = wi, stride = sc->sc_fb.fb_type.fb_width;
	int dreg = 8, swi = wi, dd;
	int in = 0, q = 0, out = 0, r;

	saddr = sc->sc_fb_paddr + xs + stride * ys;
	daddr = sc->sc_fb_paddr + xd + stride * yd;

	if (saddr & 3) {
		swi += saddr & 3;
		dreg += saddr & 3;
		saddr &= ~3;
	}
	swi = (swi + 3) >> 2;	/* round up, number of quads to read */

	if (daddr & 3) {
		in = 4 - (daddr & 3); /* pixels to write in byte mode */
		cnt -= in;
	}

	q = cnt >> 2;
	out = cnt & 3;

	for (line = 0; line < he; line++) {
		/* read source line, in all quads */
		sta(saddr & ~7, ASI_SX, SX_LDUQ0(8, swi - 1, saddr & 7));
		/* now write it out */
		dd = daddr;
		r = dreg;
		if (in > 0) {
			sta(dd & ~7, ASI_SX, SX_STB(r, in - 1, dd & 7));
			dd += in;
			r += in;
		}
		if (q > 0) {
			sta(dd & ~7, ASI_SX, SX_STUQ0(r, q - 1, dd & 7));
			r += q << 2;
			dd += q << 2;
		}
		if (out > 0) {
			sta(dd & ~7, ASI_SX, SX_STB(r, out - 1, dd & 7));
		}
		saddr += stride;
		daddr += stride;
	}
}

static void
cg14_putchar(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, c);
	struct vcons_screen *scr = ri->ri_hw;
	struct cgfourteen_softc *sc = scr->scr_cookie;
	void *data;
	uint32_t fg, bg;
	int i, x, y, wi, he;
	uint32_t addr;
	int stride = sc->sc_fb.fb_type.fb_width;

	if (sc->sc_mode != WSDISPLAYIO_MODE_EMUL)
		return;

	if (!CHAR_IN_FONT(c, font))
		return;

	wi = font->fontwidth;
	he = font->fontheight;

	bg = ri->ri_devcmap[(attr >> 16) & 0xf];
	fg = ri->ri_devcmap[(attr >> 24) & 0xf];
	sx_write(sc->sc_sx, SX_QUEUED(8), bg);
	sx_write(sc->sc_sx, SX_QUEUED(9), fg);

	x = ri->ri_xorigin + col * wi;
	y = ri->ri_yorigin + row * he;

	if (c == 0x20) {
		cg14_rectfill(sc, x, y, wi, he, bg);
		return;
	}

	data = WSFONT_GLYPH(c, font);
	addr = sc->sc_fb_paddr + x + stride * y;

	switch (font->stride) {
		case 1: {
			uint8_t *data8 = data;
			uint32_t reg;
			for (i = 0; i < he; i++) {
				reg = *data8;
				sx_write(sc->sc_sx, SX_QUEUED(R_MASK),
				    reg << 24);
				sta(addr & ~7, ASI_SX, SX_STBS(8, wi - 1, addr & 7));
				data8++;
				addr += stride;
			}
			break;
		}
		case 2: {
			uint16_t *data16 = data;
			uint32_t reg;
			for (i = 0; i < he; i++) {
				reg = *data16;
				sx_write(sc->sc_sx, SX_QUEUED(R_MASK),
				    reg << 16);
				sta(addr & ~7, ASI_SX, SX_STBS(8, wi - 1, addr & 7));
				data16++;
				addr += stride;
			}
			break;
		}
	}
}

static void
cg14_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct cgfourteen_softc *sc = scr->scr_cookie;
	int x, y, wi, he;

	wi = ri->ri_font->fontwidth;
	he = ri->ri_font->fontheight;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		x = ri->ri_ccol * wi + ri->ri_xorigin;
		y = ri->ri_crow * he + ri->ri_yorigin;
		if (ri->ri_flg & RI_CURSOR) {
			cg14_invert(sc, x, y, wi, he);
			ri->ri_flg &= ~RI_CURSOR;
		}
		ri->ri_crow = row;
		ri->ri_ccol = col;
		if (on) {
			x = ri->ri_ccol * wi + ri->ri_xorigin;
			y = ri->ri_crow * he + ri->ri_yorigin;
			cg14_invert(sc, x, y, wi, he);
			ri->ri_flg |= RI_CURSOR;
		}
	} else {
		scr->scr_ri.ri_crow = row;
		scr->scr_ri.ri_ccol = col;
		scr->scr_ri.ri_flg &= ~RI_CURSOR;
	}

}

static void
cg14_putchar_aa(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, c);
	struct vcons_screen *scr = ri->ri_hw;
	struct cgfourteen_softc *sc = scr->scr_cookie;
	int stride = sc->sc_fb.fb_type.fb_width;
	uint32_t bg, addr, bg8, fg8, pixel, in, q, next;
	int i, j, x, y, wi, he, r, g, b, aval, cnt, reg;
	int r1, g1, b1, r0, g0, b0, fgo, bgo, rv;
	uint8_t *data8;

	if (sc->sc_mode != WSDISPLAYIO_MODE_EMUL)
		return;

	if (!CHAR_IN_FONT(c, font))
		return;

	wi = font->fontwidth;
	he = font->fontheight;

	bg = ri->ri_devcmap[(attr >> 16) & 0xf];
	x = ri->ri_xorigin + col * wi;
	y = ri->ri_yorigin + row * he;
	if (c == 0x20) {
		cg14_rectfill(sc, x, y, wi, he, bg);
		return;
	}

	rv = glyphcache_try(&sc->sc_gc, c, x, y, attr);
	if (rv == GC_OK)
		return;

	addr = sc->sc_fb_paddr + x + stride * y;
	data8 = WSFONT_GLYPH(c, font);

	/*
	 * we need the RGB colours here, so get offsets into rasops_cmap
	 */
	fgo = ((attr >> 24) & 0xf) * 3;
	bgo = ((attr >> 16) & 0xf) * 3;

	r0 = rasops_cmap[bgo];
	r1 = rasops_cmap[fgo];
	g0 = rasops_cmap[bgo + 1];
	g1 = rasops_cmap[fgo + 1];
	b0 = rasops_cmap[bgo + 2];
	b1 = rasops_cmap[fgo + 2];
#define R3G3B2(r, g, b) ((r & 0xe0) | ((g >> 3) & 0x1c) | (b >> 6))
	bg8 = R3G3B2(r0, g0, b0);
	fg8 = R3G3B2(r1, g1, b1);

	for (i = 0; i < he; i++) {
		/* calculate one line of pixels */
		for (j = 0; j < wi; j++) {
			aval = *data8;
			if (aval == 0) {
				pixel = bg8;
			} else if (aval == 255) {
				pixel = fg8;
			} else {
				r = aval * r1 + (255 - aval) * r0;
				g = aval * g1 + (255 - aval) * g0;
				b = aval * b1 + (255 - aval) * b0;
				pixel = ((r & 0xe000) >> 8) |
					((g & 0xe000) >> 11) |
					((b & 0xc000) >> 14);
			}
			/*
			 * stick them into SX registers and hope we never have
			 * to deal with fonts more than 120 pixels wide
			 */
			sx_write(sc->sc_sx, SX_QUEUED(j + 8), pixel);
			data8++;
		}
		/* now write them into video memory */
		in = (addr & 3);
		next = addr;
		reg = 8;
		cnt = wi;
		if (in != 0) {
			in = 4 - in;	/* pixels to write until aligned */
			sta(next & ~7, ASI_SX, SX_STB(8, in - 1, next & 7));
			next += in;
			reg = 8 + in;
			cnt -= in;
		}
		q = cnt >> 2;	/* number of writes we can do in quads */
		if (q > 0) {
			sta(next & ~7, ASI_SX, SX_STUQ0(reg, q - 1, next & 7));
			next += (q << 2);
			cnt -= (q << 2);
			reg += (q << 2);
		}
		if (cnt > 0) {
			sta(next & ~7, ASI_SX, SX_STB(reg, cnt - 1, next & 7));
		}

		addr += stride;
	}

	if (rv == GC_ADD) {
		glyphcache_add(&sc->sc_gc, c, x, y);
	}
}

static void
cg14_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct cgfourteen_softc *sc = scr->scr_cookie;
	int32_t xs, xd, y, width, height;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		xs = ri->ri_xorigin + ri->ri_font->fontwidth * srccol;
		xd = ri->ri_xorigin + ri->ri_font->fontwidth * dstcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		cg14_bitblt(sc, xs, y, xd, y, width, height, 0x0c);
	}
}

static void
cg14_erasecols(void *cookie, int row, int startcol, int ncols, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct cgfourteen_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		x = ri->ri_xorigin + ri->ri_font->fontwidth * startcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);

		cg14_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);
	}
}

static void
cg14_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct cgfourteen_softc *sc = scr->scr_cookie;
	int32_t x, ys, yd, width, height;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		x = ri->ri_xorigin;
		ys = ri->ri_yorigin + ri->ri_font->fontheight * srcrow;
		yd = ri->ri_yorigin + ri->ri_font->fontheight * dstrow;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;
		cg14_bitblt(sc, x, ys, x, yd, width, height, 0x0c);
	}
}

static void
cg14_eraserows(void *cookie, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct cgfourteen_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		x = ri->ri_xorigin;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);

		cg14_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);
	}
}

#endif /* NSX > 0 */


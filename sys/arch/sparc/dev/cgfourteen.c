/*	$NetBSD: cgfourteen.c,v 1.52.20.6 2010/10/09 03:31:52 yamt Exp $ */

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
 * Provides minimum emulation of a Sun cgthree 8-bit framebuffer to
 * allow X to run.
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

#include <sparc/dev/cgfourteenreg.h>
#include <sparc/dev/cgfourteenvar.h>

#include "opt_wsemul.h"

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
        cgfourteenopen, cgfourteenclose, noread, nowrite, cgfourteenioctl,
        nostop, notty, cgfourteenpoll, cgfourteenmmap, nokqfilter,
};

/* frame buffer generic driver */
static struct fbdriver cgfourteenfbdriver = {
	cgfourteenunblank, cgfourteenopen, cgfourteenclose, cgfourteenioctl,
	cgfourteenpoll, cgfourteenmmap, nokqfilter
};

extern struct tty *fbconstty;

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
#endif

#if defined(RASTERCONSOLE) && (NWSDISPLAY > 0)
#error "You can't have it both ways - either RASTERCONSOLE or wsdisplay"
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

/*
 * Set COLOUR_OFFSET to the offset of the video RAM.  This is to provide
 *  space for faked overlay junk for the cg8 emulation.
 *
 * As it happens, this value is correct for both cg3 and cg8 emulation!
 */
#define COLOUR_OFFSET (256*1024)

#ifdef RASTERCONSOLE
static void cg14_set_rcons_luts(struct cgfourteen_softc *sc)
{
	int i;

	for (i=0;i<CG14_CLUT_SIZE;i++) sc->sc_xlut->xlut_lut[i] = 0x22;
	for (i=0;i<CG14_CLUT_SIZE;i++) sc->sc_clut2->clut_lut[i] = 0x00ffffff;
	sc->sc_clut2->clut_lut[0] = 0x00ffffff;
	sc->sc_clut2->clut_lut[255] = 0;
}
#endif /* RASTERCONSOLE */

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
	int node, ramsize;
	volatile uint32_t *lut;
	int i, isconsole, items;
	uint32_t fbva[2] = {0, 0};
	uint32_t *ptr = fbva;

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
	ramsize = roundup(fb->fb_type.fb_height * fb->fb_linebytes, NBPG);

	fb->fb_type.fb_cmsize = CG14_CLUT_SIZE;
	fb->fb_type.fb_size = ramsize + COLOUR_OFFSET;

	if (sa->sa_nreg < 2) {
		printf("%s: only %d register sets\n",
			self->dv_xname, sa->sa_nreg);
		return;
	}
	memcpy(sc->sc_physadr, sa->sa_reg,
	      sa->sa_nreg * sizeof(struct sbus_reg));

	sc->sc_vramsize = sc->sc_physadr[CG14_PXL_IDX].sbr_size;

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
		printf("%s: cannot map control registers\n", self->dv_xname);
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

#if defined(RASTERCONSOLE)
	if (isconsole) {
		printf(" (console)\n");
		/* *sbus*_bus_map?  but that's how we map the regs... */
		if (sbus_bus_map( sc->sc_bustag,
				  sc->sc_physadr[CG14_PXL_IDX].sbr_slot,
				  sc->sc_physadr[CG14_PXL_IDX].sbr_offset +
				    0x03800000,
				  1152 * 900, BUS_SPACE_MAP_LINEAR,
				  &bh) != 0) {
			printf("%s: cannot map pixels\n",
			    device_xname(sc->sc_dev));
			return;
		}
		sc->sc_rcfb = sc->sc_fb;
		sc->sc_rcfb.fb_type.fb_type = FBTYPE_SUN3COLOR;
		sc->sc_rcfb.fb_type.fb_depth = 8;
		sc->sc_rcfb.fb_linebytes = 1152;
		sc->sc_rcfb.fb_type.fb_size = roundup(1152*900,NBPG);
		sc->sc_rcfb.fb_pixels = (void *)bh;

		printf("vram at %p\n",(void *)bh);
		/* XXX should use actual screen size */

		for (i = 0; i < ramsize; i++)
		    ((unsigned char *)bh)[i] = 0;
		fbrcons_init(&sc->sc_rcfb);
		cg14_set_rcons_luts(sc);
		sc->sc_ctl->ctl_mctl = CG14_MCTL_ENABLEVID | 
		    CG14_MCTL_PIXMODE_32 | CG14_MCTL_POWERCTL;
	} else
		printf("\n");
#endif

#if NWSDISPLAY > 0
	prom_getprop(sa->sa_node, "address", 4, &items, &ptr);
	if (fbva[1] == 0) {
		if (sbus_bus_map( sc->sc_bustag,
		    sc->sc_physadr[CG14_PXL_IDX].sbr_slot,
		    sc->sc_physadr[CG14_PXL_IDX].sbr_offset,
		    ramsize, BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_LARGE,
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
	if (opens == 0)
		cg14_reset(sc);

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

		switch (depth) {
		case 8:
			bus_space_write_1(sc->sc_bustag, sc->sc_regh,
			    CG14_MCTL, CG14_MCTL_ENABLEVID | 
			    CG14_MCTL_PIXMODE_8 | CG14_MCTL_POWERCTL);
			break;
		case 32:
			bus_space_write_1(sc->sc_bustag, sc->sc_regh,
			    CG14_MCTL, CG14_MCTL_ENABLEVID | 
			    CG14_MCTL_PIXMODE_32 | CG14_MCTL_POWERCTL);
			break;
		default:
			return EINVAL;
		}
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
	volatile uint32_t *clut;
	volatile uint8_t  *xlut;
	int i;

	/*
	 * We stash away the following to restore on close:
	 *
	 * 	color look-up table 1 	(sc->sc_saveclut)
	 *	x look-up table		(sc->sc_savexlut)
	 *	control register	(sc->sc_savectl)
	 *	cursor control register (sc->sc_savehwc)
	 */
	sc->sc_savectl = sc->sc_ctl->ctl_mctl;
	sc->sc_savehwc = sc->sc_hwc->curs_ctl;

	clut = (volatile uint32_t *) sc->sc_clut1->clut_lut;
	xlut = (volatile uint8_t *) sc->sc_xlut->xlut_lut;
	for (i = 0; i < CG14_CLUT_SIZE; i++) {
		sc->sc_saveclut.cm_chip[i] = clut[i];
		sc->sc_savexlut[i] = xlut[i];
	}

	/*
	 * Enable the video and put it in 8 bit mode
	 */
	sc->sc_ctl->ctl_mctl = CG14_MCTL_ENABLEVID | CG14_MCTL_PIXMODE_8 |
		CG14_MCTL_POWERCTL;
}

static void
/* Restore the state saved on cg14_init */
cg14_reset(struct cgfourteen_softc *sc)
{
	volatile uint32_t *clut;
	volatile uint8_t  *xlut;
	int i;

	/*
	 * We restore the following, saved in cg14_init:
	 *
	 * 	color look-up table 1 	(sc->sc_saveclut)
	 *	x look-up table		(sc->sc_savexlut)
	 *	control register	(sc->sc_savectl)
	 *	cursor control register (sc->sc_savehwc)
	 *
	 * Note that we don't touch the video enable bits in the
	 * control register; otherwise, screenblank wouldn't work.
	 */
	sc->sc_ctl->ctl_mctl = (sc->sc_ctl->ctl_mctl & (CG14_MCTL_ENABLEVID |
							CG14_MCTL_POWERCTL)) |
				(sc->sc_savectl & ~(CG14_MCTL_ENABLEVID |
						    CG14_MCTL_POWERCTL));
	sc->sc_hwc->curs_ctl = sc->sc_savehwc;

	clut = sc->sc_clut1->clut_lut;
	xlut = sc->sc_xlut->xlut_lut;
	for (i = 0; i < CG14_CLUT_SIZE; i++) {
		clut[i] = sc->sc_saveclut.cm_chip[i];
		xlut[i] = sc->sc_savexlut[i];
	}
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

#if NWSDISPLAY > 0
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
	sc->sc_screens[0] = &sc->sc_defaultscreen_descr;
	sc->sc_screenlist = (struct wsscreen_list){1, sc->sc_screens};
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	vcons_init(&sc->sc_vd, sc, &sc->sc_defaultscreen_descr,
	    &cg14_accessops);
	sc->sc_vd.init_screen = cg14_init_screen;

	ri = &sc->sc_console_screen.scr_ri;

	if (is_cons) {
		vcons_init_screen(&sc->sc_vd, &sc->sc_console_screen, 1,
		    &defattr);

		/* clear the screen with the default background colour */
		memset(sc->sc_fb.fb_pixels,
		       (defattr >> 16) & 0xff,
		       ri->ri_stride * ri->ri_height);
		sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

		sc->sc_defaultscreen_descr.textops = &ri->ri_ops;
		sc->sc_defaultscreen_descr.capabilities = ri->ri_caps;
		sc->sc_defaultscreen_descr.nrows = ri->ri_rows;
		sc->sc_defaultscreen_descr.ncols = ri->ri_cols;
		wsdisplay_cnattach(&sc->sc_defaultscreen_descr, ri, 0, 0,
		    defattr);
		vcons_replay_msgbuf(&sc->sc_console_screen);
	} else {
		/*
		 * since we're not the console we can postpone the rest
		 * until someone actually allocates a screen for us
		 */
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
	int i, j = 0;

	for (i = 0; i < 256; i++) {

		sc->sc_cmap.cm_map[i][3] = rasops_cmap[j];
		sc->sc_cmap.cm_map[i][2] = rasops_cmap[j + 1];
		sc->sc_cmap.cm_map[i][1] = rasops_cmap[j + 2];
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
	scr->scr_flags |= VCONS_DONT_READ;

	if (existing) {
		ri->ri_flg |= RI_CLEAR;
	}

	rasops_init(ri, sc->sc_fb.fb_type.fb_height / 8,
	     sc->sc_fb.fb_type.fb_width / 8);
	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri,
	    sc->sc_fb.fb_type.fb_height / ri->ri_font->fontheight,
	    sc->sc_fb.fb_type.fb_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
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
#endif

/*	$NetBSD: cgfourteen.c,v 1.42 2006/01/17 04:22:08 christos Exp $ */

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
#define CG14_MAP_REGS

/*
 * The following enables 24-bit operation: when opened, the framebuffer
 * will switch to 24-bit mode (actually 32-bit mode), and provide a
 * simple cg8 emulation.
 */
#define CG14_CG8

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include <uvm/uvm_extern.h>

#include <dev/sun/fbio.h>
#include <machine/autoconf.h>
#include <machine/pmap.h>
#include <dev/sun/fbvar.h>
#include <machine/cpu.h>
#include <dev/sbus/sbusvar.h>

#include <sparc/dev/cgfourteenreg.h>
#include <sparc/dev/cgfourteenvar.h>

/* autoconfiguration driver */
static int	cgfourteenmatch(struct device *, struct cfdata *, void *);
static void	cgfourteenattach(struct device *, struct device *, void *);
static void	cgfourteenunblank(struct device *);

CFATTACH_DECL(cgfourteen, sizeof(struct cgfourteen_softc),
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

/*
 * Match a cgfourteen.
 */
int
cgfourteenmatch(struct device *parent, struct cfdata *cf, void *aux)
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

static void cg14_set_rcons_luts(struct cgfourteen_softc *sc)
{
	int i;

	for (i=0;i<CG14_CLUT_SIZE;i++) sc->sc_xlut->xlut_lut[i] = 0x22;
	for (i=0;i<CG14_CLUT_SIZE;i++) sc->sc_clut2->clut_lut[i] = 0x00ffffff;
	sc->sc_clut2->clut_lut[0] = 0x00ffffff;
	sc->sc_clut2->clut_lut[255] = 0;
}

/*
 * Attach a display.  We need to notice if it is the console, too.
 */
void
cgfourteenattach(struct device *parent, struct device *self, void *aux)
{
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa = &uoba->uoba_sbus;
	struct cgfourteen_softc *sc = (struct cgfourteen_softc *)self;
	struct fbdevice *fb = &sc->sc_fb;
	bus_space_handle_t bh;
	int node, ramsize;
	volatile uint32_t *lut;
	int i, isconsole;

	node = sa->sa_node;

	/* Remember cookies for cgfourteenmmap() */
	sc->sc_bustag = sa->sa_bustag;

	fb->fb_driver = &cgfourteenfbdriver;
	fb->fb_device = &sc->sc_dev;
	/* Mask out invalid flags from the user. */
	fb->fb_flags = sc->sc_dev.dv_cfdata->cf_flags & FB_USERMASK;

	/*
	 * We're emulating a cg3/8, so represent ourselves as one
	 */
#ifdef CG14_CG8
	fb->fb_type.fb_type = FBTYPE_MEMCOLOR;
	fb->fb_type.fb_depth = 32;
	fb_setsize_obp(fb, sc->sc_fb.fb_type.fb_depth, 1152, 900, node);
	ramsize = roundup(fb->fb_type.fb_height * 1152 * 4, NBPG);
#else
	fb->fb_type.fb_type = FBTYPE_SUN3COLOR;
	fb->fb_type.fb_depth = 8;
	fb_setsize_obp(fb, sc->sc_fb.fb_type.fb_depth, 1152, 900, node);
	ramsize = roundup(fb->fb_type.fb_height * fb->fb_linebytes, NBPG);
#endif
	fb->fb_type.fb_cmsize = CG14_CLUT_SIZE;
	fb->fb_type.fb_size = ramsize + COLOUR_OFFSET;

	if (sa->sa_nreg < 2) {
		printf("%s: only %d register sets\n",
			self->dv_xname, sa->sa_nreg);
		return;
	}
	bcopy(sa->sa_reg, sc->sc_physadr,
	      sa->sa_nreg * sizeof(struct sbus_reg));

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
			 BUS_SPACE_MAP_LINEAR,
			 &bh) != 0) {
		printf("%s: cannot map control registers\n", self->dv_xname);
		return;
	}

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
#ifdef CG14_CG8
	printf(": cgeight emulated at %dx%dx24bpp",
		fb->fb_type.fb_width, fb->fb_type.fb_height);
#else
	printf(": cgthree emulated at %dx%dx8bpp",
		fb->fb_type.fb_width, fb->fb_type.fb_height);
#endif
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

	if (isconsole) {
		printf(" (console)\n");
#ifdef RASTERCONSOLE
		/* *sbus*_bus_map?  but that's how we map the regs... */
		if (sbus_bus_map( sc->sc_bustag,
				  sc->sc_physadr[CG14_PXL_IDX].sbr_slot,
				  sc->sc_physadr[CG14_PXL_IDX].sbr_offset+0x03800000,
				  1152*900, BUS_SPACE_MAP_LINEAR,
				  &bh) != 0) {
			printf("%s: cannot map pixels\n",&sc->sc_dev.dv_xname[0]);
		} else {
			sc->sc_rcfb = sc->sc_fb;
			sc->sc_rcfb.fb_type.fb_type = FBTYPE_SUN3COLOR;
			sc->sc_rcfb.fb_type.fb_depth = 8;
			sc->sc_rcfb.fb_linebytes = 1152;
			sc->sc_rcfb.fb_type.fb_size = roundup(1152*900,NBPG);
			sc->sc_rcfb.fb_pixels = (void *)bh;
			printf("vram at %p\n",(void *)bh);
			for (i=0;i<1152*900;i++) ((unsigned char *)bh)[i] = 0;
			fbrcons_init(&sc->sc_rcfb);
			cg14_set_rcons_luts(sc);
			sc->sc_ctl->ctl_mctl = CG14_MCTL_ENABLEVID | CG14_MCTL_PIXMODE_32 | CG14_MCTL_POWERCTL;
		}
#endif
	} else
		printf("\n");

	/* Attach to /dev/fb */
	fb_attach(&sc->sc_fb, isconsole);
}

/*
 * Keep track of the number of opens made. In the 24-bit driver, we need to
 * switch to 24-bit mode on the first open, and switch back to 8-bit on
 * the last close. This kind of nonsense is needed to give screenblank
 * a fighting chance of working.
 */
static int cg14_opens = 0;

int
cgfourteenopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct cgfourteen_softc *sc;
	int unit;
	int s, oldopens;

	unit = minor(dev);
	if (unit >= cgfourteen_cd.cd_ndevs)
		return(ENXIO);
	sc = cgfourteen_cd.cd_devs[minor(dev)];
	if (sc == NULL)
		return(ENXIO);
	s = splhigh();
	oldopens = cg14_opens++;
	splx(s);

	/* Setup the cg14 as we want it, and save the original PROM state */
	if (oldopens == 0)	/* first open only, to make screenblank work */
		cg14_init(sc);

	return (0);
}

int
cgfourteenclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct cgfourteen_softc *sc = cgfourteen_cd.cd_devs[minor(dev)];
	int s, opens;

	s = splhigh();
	opens = --cg14_opens;
	if (cg14_opens < 0)
		opens = cg14_opens = 0;
	splx(s);

	/*
	 * Restore video state to make the PROM happy, on last close.
	 */
	if (opens == 0)
		cg14_reset(sc);

	return (0);
}

int
cgfourteenioctl(dev_t dev, u_long cmd, caddr_t data, int flags, struct lwp *l)
{
	struct cgfourteen_softc *sc = cgfourteen_cd.cd_devs[minor(dev)];
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
#ifdef CG14_CG8
		p->index &= 0xffffff;
#endif
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

	default:
		return (ENOTTY);
	}
	return (0);
}

/*
 * Undo the effect of an FBIOSVIDEO that turns the video off.
 */
static void
cgfourteenunblank(struct device *dev)
{

	cg14_set_video((struct cgfourteen_softc *)dev, 1);
}

/*
 * Return the address that would map the given device at the given
 * offset, allowing for the given protection, or return -1 for error.
 *
 * Since we're pretending to be a cg8, we put the main video RAM at the
 *  same place the cg8 does, at offset 256k.  The cg8 has an enable
 *  plane in the 256k space; our "enable" plane works differently.  We
 *  can't emulate the enable plane very well, but all X uses it for is
 *  to clear it at startup - so we map the first page of video RAM over
 *  and over to fill that 256k space.  We also map some other views of
 *  the video RAM space.
 *
 * Our memory map thus looks like
 *
 *	mmap range		space	base offset
 *	00000000-00040000	vram	0 (multi-mapped - see above)
 *	00040000-00434800	vram	00000000
 *	01000000-01400000	vram	01000000
 *	02000000-02200000	vram	02000000
 *	02800000-02a00000	vram	02800000
 *	03000000-03100000	vram	03000000
 *	03400000-03500000	vram	03400000
 *	03800000-03900000	vram	03800000
 *	03c00000-03d00000	vram	03c00000
 *	10000000-10010000	regs	00000000 (only if CG14_MAP_REGS)
 */
paddr_t
cgfourteenmmap(dev_t dev, off_t off, int prot)
{
	struct cgfourteen_softc *sc = cgfourteen_cd.cd_devs[minor(dev)];

	if (off & PGOFSET)
		panic("cgfourteenmmap");

	if (off < 0)
		return (-1);

#if defined(CG14_MAP_REGS) /* XXX: security hole */
	/*
	 * Map the control registers into user space. Should only be
	 * used for debugging!
	 */
	if ((u_int)off >= 0x10000000 && (u_int)off < 0x10000000 + 16*4096) {
		off -= 0x10000000;
		return (bus_space_mmap(sc->sc_bustag,
			BUS_ADDR(sc->sc_physadr[CG14_CTL_IDX].sbr_slot,
				   sc->sc_physadr[CG14_CTL_IDX].sbr_offset),
			off, prot, BUS_SPACE_MAP_LINEAR));
	}
#endif

	if (off < COLOUR_OFFSET)
		off = 0;
	else if (off < COLOUR_OFFSET+(1152*900*4))
		off -= COLOUR_OFFSET;
	else {
		switch (off >> 20) {
			case 0x010: case 0x011: case 0x012: case 0x013:
			case 0x020: case 0x021:
			case 0x028: case 0x029:
			case 0x030:
			case 0x034:
			case 0x038:
			case 0x03c:
				break;
			default:
				return(-1);
		}
	}

	return (bus_space_mmap(sc->sc_bustag,
		BUS_ADDR(sc->sc_physadr[CG14_PXL_IDX].sbr_slot,
			   sc->sc_physadr[CG14_PXL_IDX].sbr_offset),
		off, prot, BUS_SPACE_MAP_LINEAR));
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
	u_int32_t *clut;
	u_int8_t  *xlut;
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

	clut = (u_int32_t *) sc->sc_clut1->clut_lut;
	xlut = (u_int8_t *) sc->sc_xlut->xlut_lut;
	for (i = 0; i < CG14_CLUT_SIZE; i++) {
		sc->sc_saveclut.cm_chip[i] = clut[i];
		sc->sc_savexlut[i] = xlut[i];
	}

#ifdef CG14_CG8
	/*
	 * Enable the video, and put in 24 bit mode.
	 */
	sc->sc_ctl->ctl_mctl = CG14_MCTL_ENABLEVID | CG14_MCTL_PIXMODE_32 |
		CG14_MCTL_POWERCTL;

	/*
	 * Zero the xlut to enable direct-color mode
	 */
	for (i = 0; i < CG14_CLUT_SIZE; i++)
		sc->sc_xlut->xlut_lut[i] = 0;
#else
	/*
	 * Enable the video and put it in 8 bit mode
	 */
	sc->sc_ctl->ctl_mctl = CG14_MCTL_ENABLEVID | CG14_MCTL_PIXMODE_8 |
		CG14_MCTL_POWERCTL;
#endif
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

/*	$NetBSD: bwtwo.c,v 1.41 1999/05/19 08:31:42 pk Exp $ */

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
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
 *	@(#)bwtwo.c	8.1 (Berkeley) 6/11/93
 */

/*
 * black&white display (bwtwo) driver.
 *
 * Does not handle interrupts, even though they can occur.
 *
 * P4 and overlay plane support by Jason R. Thorpe <thorpej@NetBSD.ORG>.
 * Overlay plane handling hints and ideas provided by Brad Spencer.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include <vm/vm.h>

#include <machine/fbio.h>
#include <machine/autoconf.h>
#include <machine/pmap.h>
#include <machine/fbvar.h>
#include <machine/eeprom.h>
#include <machine/ctlreg.h>
#include <machine/conf.h>
#include <sparc/sparc/asm.h>

#include <sparc/dev/btreg.h>
#include <sparc/dev/bwtworeg.h>
#include <sparc/dev/sbusvar.h>
#include <sparc/dev/pfourreg.h>

/* per-display variables */
struct bwtwo_softc {
	struct device	sc_dev;		/* base device */
	struct sbusdev	sc_sd;		/* sbus device */
	struct fbdevice	sc_fb;		/* frame buffer device */
	bus_space_tag_t	sc_bustag;
	bus_type_t	sc_btype;	/* phys address description */
	bus_addr_t	sc_paddr;	/* for device mmap() */

	volatile struct fbcontrol *sc_reg;/* control registers */
	int		sc_pixeloffset;	/* offset to framebuffer */
	/*
	 * Additional overlay plane goo.
	 */
	int	sc_ovtype;		/* what kind of color fb? */
#define BWO_NONE	0x00
#define BWO_CGFOUR	0x01
#define BWO_CGEIGHT	0x02

	/* Video status */
	int	(*sc_get_video) __P((struct bwtwo_softc *));
	void	(*sc_set_video) __P((struct bwtwo_softc *, int));
};

/* autoconfiguration driver */
static void	bwtwoattach_sbus __P((struct device *, struct device *, void *));
static int	bwtwomatch_sbus __P((struct device *, struct cfdata *, void *));
static void	bwtwoattach_obio __P((struct device *, struct device *, void *));
static int	bwtwomatch_obio __P((struct device *, struct cfdata *, void *));

static void	bwtwoattach __P((struct bwtwo_softc *, char *, int, int));
static void	bwtwounblank __P((struct device *));
static int	bwtwo_get_video_sun4  __P((struct bwtwo_softc *));
static void	bwtwo_set_video_sun4 __P((struct bwtwo_softc *, int));
static int	bwtwo_get_video_sun4c  __P((struct bwtwo_softc *));
static void	bwtwo_set_video_sun4c __P((struct bwtwo_softc *, int));
static int	bwtwo_pfour_probe __P((void *, void *));


/* cdevsw prototypes */
cdev_decl(bwtwo);

struct cfattach bwtwo_sbus_ca = {
	sizeof(struct bwtwo_softc), bwtwomatch_sbus, bwtwoattach_sbus
};

struct cfattach bwtwo_obio_ca = {
	sizeof(struct bwtwo_softc), bwtwomatch_obio, bwtwoattach_obio
};

extern struct cfdriver bwtwo_cd;

/* XXX we do not handle frame buffer interrupts (do not know how) */

/* frame buffer generic driver */
static struct fbdriver bwtwofbdriver = {
	bwtwounblank, bwtwoopen, bwtwoclose, bwtwoioctl, bwtwopoll, bwtwommap
};

extern int fbnode;
extern struct tty *fbconstty;

/*
 * Match a bwtwo.
 */
int
bwtwomatch_sbus(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct sbus_attach_args *sa = aux;

	return (strcmp(cf->cf_driver->cd_name, sa->sa_name) == 0);
}

int
bwtwomatch_obio(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	if (uoba->uoba_isobio4 == 0)
		return (0);

	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag, 0, oba->oba_paddr,
				4,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				bwtwo_pfour_probe, cf));
}

int
bwtwo_pfour_probe(vaddr, arg)
	void *vaddr;
	void *arg;
{
	struct cfdata *cf = arg;

	switch (fb_pfour_id(vaddr)) {
	case PFOUR_ID_BW:
	case PFOUR_ID_COLOR8P1:		/* bwtwo in ... */
	case PFOUR_ID_COLOR24:		/* ...overlay plane */
		/* This is wrong; should be done in bwtwo_attach() */
		cf->cf_flags |= FB_PFOUR;
		/* FALLTHROUGH */
	case PFOUR_NOTPFOUR:
		return (1);
	}
	return (0);
}

/*
 * Attach a display.  We need to notice if it is the console, too.
 */
void
bwtwoattach_sbus(parent, self, args)
	struct device *parent, *self;
	void *args;
{
	struct bwtwo_softc *sc = (struct bwtwo_softc *)self;
	struct sbus_attach_args *sa = args;
	struct fbdevice *fb = &sc->sc_fb;
	bus_space_handle_t bh;
	int isconsole, node;
	char *name;
	extern struct tty *fbconstty;

	node = sa->sa_node;

	/* Remember cookies for bwtwo_mmap() */
	sc->sc_bustag = sa->sa_bustag;
	sc->sc_btype = (bus_type_t)sa->sa_slot;
	sc->sc_paddr = (bus_addr_t)sa->sa_offset;

	fb->fb_flags = sc->sc_dev.dv_cfdata->cf_flags;
	fb->fb_type.fb_depth = 1;
	fb_setsize_obp(fb, fb->fb_type.fb_depth, 1152, 900, node);

	/*
	 * When the ROM has mapped in a bwtwo display, the address
	 * maps only the video RAM, hence we always map the control
	 * registers ourselves.  We only need the video RAM if we are
	 * going to print characters via rconsole.
	 */
	if (sbus_bus_map(sa->sa_bustag, sa->sa_slot,
			 sa->sa_offset + BWREG_REG,
			 sizeof(struct fbcontrol),
			 BUS_SPACE_MAP_LINEAR,
			 0, &bh) != 0) {
		printf("%s: cannot map control registers\n", self->dv_xname);
		return;
	}
	sc->sc_reg = (struct fbcontrol *)bh;
	fb->fb_pfour = NULL;

	sc->sc_pixeloffset = BWREG_MEM;

	isconsole = node == fbnode && fbconstty != NULL;
	name = getpropstring(node, "model");

	/* Assume `bwtwo at sbus' only happens at sun4c's */
	sc->sc_get_video = bwtwo_get_video_sun4c;
	sc->sc_set_video = bwtwo_set_video_sun4c;

	if (sa->sa_npromvaddrs != 0)
		sc->sc_fb.fb_pixels = (caddr_t)sa->sa_promvaddrs[0];
	if (isconsole && sc->sc_fb.fb_pixels == NULL) {
		int ramsize = fb->fb_type.fb_height * fb->fb_linebytes;
		if (sbus_bus_map(sa->sa_bustag, sa->sa_slot,
				 sa->sa_offset + sc->sc_pixeloffset,
				 ramsize,
				 BUS_SPACE_MAP_LINEAR,
				 0, &bh) != 0) {
			printf("%s: cannot map pixels\n", self->dv_xname);
			return;
		}
		sc->sc_fb.fb_pixels = (char *)bh;
	}

	sbus_establish(&sc->sc_sd, &sc->sc_dev);
	bwtwoattach(sc, name, isconsole, node == fbnode);
}

void
bwtwoattach_obio(parent, self, uax)
	struct device *parent, *self;
	void *uax;
{
	struct bwtwo_softc *sc = (struct bwtwo_softc *)self;
	union obio_attach_args *uoba = uax;
	struct obio4_attach_args *oba;
	struct fbdevice *fb = &sc->sc_fb;
	struct eeprom *eep = (struct eeprom *)eeprom_va;
	bus_space_handle_t bh;
	int constype, isconsole;
	char *name;

	if (uoba->uoba_isobio4 == 0) {
		bwtwoattach_sbus(parent, self, &uoba->uoba_sbus);
		return;
	}

	oba = &uoba->uoba_oba4;

	/* Remember cookies for bwtwo_mmap() */
	sc->sc_bustag = oba->oba_bustag;
	sc->sc_btype = (bus_type_t)0;
	sc->sc_paddr = (bus_addr_t)oba->oba_paddr;

	fb->fb_flags = sc->sc_dev.dv_cfdata->cf_flags;
	fb->fb_type.fb_depth = 1;
	fb_setsize_eeprom(fb, fb->fb_type.fb_depth, 1152, 900);

	constype = (fb->fb_flags & FB_PFOUR) ? EE_CONS_P4OPT : EE_CONS_BW;
	if (eep == NULL || eep->eeConsole == constype)
		isconsole = (fbconstty != NULL);
	else
		isconsole = 0;

	if (fb->fb_flags & FB_PFOUR) {
		/*
		 * Map the pfour control register.
		 * Set pixel offset to appropriate overlay plane.
		 */
		name = "bwtwo/p4";

		if (obio_bus_map(oba->oba_bustag,
				 oba->oba_paddr,
				 0,
				 sizeof(u_int32_t),
				 BUS_SPACE_MAP_LINEAR,
				 0, &bh) != 0) {
			printf("%s: cannot map pfour register\n",
				self->dv_xname);
			return;
		}
		fb->fb_pfour = (u_int32_t *)bh;
		sc->sc_reg = NULL;

		/*
		 * Notice if this is an overlay plane on a color
		 * framebuffer.  Note that PFOUR_COLOR_OFF_OVERLAY
		 * is the same as PFOUR_BW_OFF, but we use the
		 * different names anyway.
		 */
		switch (PFOUR_ID(*fb->fb_pfour)) {
		case PFOUR_ID_COLOR8P1:
			sc->sc_ovtype = BWO_CGFOUR;
			sc->sc_pixeloffset = PFOUR_COLOR_OFF_OVERLAY;
			break;

		case PFOUR_ID_COLOR24:
			sc->sc_ovtype = BWO_CGEIGHT;
			sc->sc_pixeloffset = PFOUR_COLOR_OFF_OVERLAY;
			break;

		default:
			sc->sc_ovtype = BWO_NONE;
			sc->sc_pixeloffset = PFOUR_BW_OFF;
			break;
		}

	} else {
		/* A plain bwtwo */
		if (obio_bus_map(oba->oba_bustag,
				 oba->oba_paddr,
				 BWREG_REG,
				 sizeof(struct fbcontrol),
				 BUS_SPACE_MAP_LINEAR,
				 0, &bh) != 0) {
			printf("%s: cannot map control registers\n",
				self->dv_xname);
			return;
		}
		sc->sc_reg = (struct fbcontrol *)bh;
		fb->fb_pfour = NULL;

		name = "bwtwo";
		sc->sc_pixeloffset = BWREG_MEM;
	}
	sc->sc_get_video = bwtwo_get_video_sun4;
	sc->sc_set_video = bwtwo_set_video_sun4;

	if (isconsole) {
		int ramsize = fb->fb_type.fb_height * fb->fb_linebytes;
		if (obio_bus_map(oba->oba_bustag, oba->oba_paddr,
				 sc->sc_pixeloffset,
				 ramsize,
				 BUS_SPACE_MAP_LINEAR,
				 0, &bh) != 0) {
			printf("%s: cannot map pixels\n", self->dv_xname);
			return;
		}
		sc->sc_fb.fb_pixels = (char *)bh;
	}

	bwtwoattach(sc, name, isconsole, 1);
}

void
bwtwoattach(sc, name, isconsole, isfb)
	struct	bwtwo_softc *sc;
	char	*name;
	int	isconsole;
	int	isfb;
{
	struct fbdevice *fb = &sc->sc_fb;

	/* Fill in the remaining fbdevice values */
	fb->fb_driver = &bwtwofbdriver;
	fb->fb_device = &sc->sc_dev;
	fb->fb_type.fb_type = FBTYPE_SUN2BW;
	fb->fb_type.fb_cmsize = 0;
	fb->fb_type.fb_size = fb->fb_type.fb_height * fb->fb_linebytes;
	printf(": %s, %d x %d", name,
	       fb->fb_type.fb_width, fb->fb_type.fb_height);

	/* Insure video is enabled */
	sc->sc_set_video(sc, 1);

	if (isconsole) {
		printf(" (console)\n");
#ifdef RASTERCONSOLE
		/*
		 * XXX rcons doesn't seem to work properly on the overlay
		 * XXX plane.  This is a temporary kludge until someone
		 * XXX fixes it.
		 */
		if ((fb->fb_flags & FB_PFOUR) == 0 ||
		    (sc->sc_ovtype == BWO_NONE))
			fbrcons_init(fb);
#endif
	} else
		printf("\n");

	if ((fb->fb_flags & FB_PFOUR) && (sc->sc_ovtype != BWO_NONE)) {
		char *ovnam;

		switch (sc->sc_ovtype) {
		case BWO_CGFOUR:
			ovnam = "cgfour";
			break;

		case BWO_CGEIGHT:
			ovnam = "cgeight";
			break;

		default:
			ovnam = "unknown";
			break;
		}
		printf("%s: %s overlay plane\n", sc->sc_dev.dv_xname, ovnam);
	}

	if (isfb) {
		/*
		 * If we're on an overlay plane of a color framebuffer,
		 * then we don't force the issue in fb_attach() because
		 * we'd like the color framebuffer to actually be the
		 * "console framebuffer".  We're only around to speed
		 * up rconsole.
		 */
		if ((fb->fb_flags & FB_PFOUR) && (sc->sc_ovtype != BWO_NONE ))
			fb_attach(fb, 0);
		else
			fb_attach(fb, isconsole);
	}
}


int
bwtwoopen(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	int unit = minor(dev);

	if (unit >= bwtwo_cd.cd_ndevs || bwtwo_cd.cd_devs[unit] == NULL)
		return (ENXIO);

	return (0);
}

int
bwtwoclose(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{

	return (0);
}

int
bwtwoioctl(dev, cmd, data, flags, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flags;
	struct proc *p;
{
	struct bwtwo_softc *sc = bwtwo_cd.cd_devs[minor(dev)];

	switch (cmd) {

	case FBIOGTYPE:
		*(struct fbtype *)data = sc->sc_fb.fb_type;
		break;

	case FBIOGVIDEO:
		*(int *)data = sc->sc_get_video(sc);
		break;

	case FBIOSVIDEO:
		sc->sc_set_video(sc, (*(int *)data));
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}

static void
bwtwounblank(dev)
	struct device *dev;
{
	struct bwtwo_softc *sc = (struct bwtwo_softc *)dev;

	sc->sc_set_video(sc, 1);
}

int
bwtwopoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{

	return (seltrue(dev, events, p));
}

/*
 * Return the address that would map the given device at the given
 * offset, allowing for the given protection, or return -1 for error.
 */
int
bwtwommap(dev, off, prot)
	dev_t dev;
	int off, prot;
{
	struct bwtwo_softc *sc = bwtwo_cd.cd_devs[minor(dev)];
	bus_space_handle_t bh;

	if (off & PGOFSET)
		panic("bwtwommap");

	if ((unsigned)off >= sc->sc_fb.fb_type.fb_size)
		return (-1);

	if (bus_space_mmap(sc->sc_bustag,
			   sc->sc_btype,
			   sc->sc_paddr + sc->sc_pixeloffset + off,
			   BUS_SPACE_MAP_LINEAR, &bh))
		return (-1);

	return ((int)bh);
}

static int
bwtwo_get_video_sun4c(sc)
	struct bwtwo_softc *sc;
{

	return ((sc->sc_reg->fbc_ctrl & FBC_VENAB) != 0);
}

static int
bwtwo_get_video_sun4(sc)
	struct bwtwo_softc *sc;
{

	if (sc->sc_fb.fb_flags & FB_PFOUR) {
		/*
		 * This handles the overlay plane case, too.
		 */
		return (fb_pfour_get_video(&sc->sc_fb));
	} else
		return ((lduba(AC_SYSENABLE, ASI_CONTROL) & SYSEN_VIDEO) != 0);
}

static void
bwtwo_set_video_sun4c(sc, enable)
	struct bwtwo_softc *sc;
	int enable;
{

	if (enable)
		/*
		 * If the we're the console the PROM will have taken care
		 * of the FBC_TIMING bit and video control parameters. We
		 * simply turn on FBC_TIMING; in case other video parameters
		 * are necessary, here is a set read from an ELC:
		 * [0xbb,0x2b,0x4,0x14,0xae,0x3,0xa8,0x24,0x1,0x5,0xff,0x1]
		 */
		sc->sc_reg->fbc_ctrl |= FBC_VENAB | FBC_TIMING;
	else
		sc->sc_reg->fbc_ctrl &= ~FBC_VENAB;
}

static void
bwtwo_set_video_sun4(sc, enable)
	struct bwtwo_softc *sc;
	int enable;
{

	if (sc->sc_fb.fb_flags & FB_PFOUR) {
		/*
		 * This handles the overlay plane case, too.
		 */
		fb_pfour_set_video(&sc->sc_fb, enable);
		return;
	}
	if (enable)
		stba(AC_SYSENABLE, ASI_CONTROL,
		     lduba(AC_SYSENABLE, ASI_CONTROL) | SYSEN_VIDEO);
	else
		stba(AC_SYSENABLE, ASI_CONTROL,
		     lduba(AC_SYSENABLE, ASI_CONTROL) & ~SYSEN_VIDEO);

	return;
}

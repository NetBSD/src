/*	$NetBSD: cgthree.c,v 1.48 2000/07/09 20:38:34 pk Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
 *	@(#)cgthree.c	8.2 (Berkeley) 10/30/93
 */

/*
 * color display (cgthree) driver.
 *
 * Does not handle interrupts, even though they can occur.
 *
 * XXX should defer colormap updates to vertical retrace interrupts
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/fbio.h>
#include <machine/fbvar.h>

#include <sparc/dev/btreg.h>
#include <sparc/dev/btvar.h>
#include <sparc/dev/cgthreereg.h>
#include <sparc/dev/sbusvar.h>

#include <machine/conf.h>

/* per-display variables */
struct cgthree_softc {
	struct device	sc_dev;		/* base device */
	struct sbusdev	sc_sd;		/* sbus device */
	struct fbdevice	sc_fb;		/* frame buffer device */
	bus_space_tag_t	sc_bustag;
	bus_type_t	sc_btype;	/* phys address description */
	bus_addr_t	sc_paddr;	/* for device mmap() */

	volatile struct fbcontrol *sc_fbc;	/* Brooktree registers */
	union	bt_cmap sc_cmap;	/* Brooktree color map */
};

/* autoconfiguration driver */
static int	cgthreematch_sbus(struct device *, struct cfdata *, void *);
static int	cgthreematch_obio(struct device *, struct cfdata *, void *);
static void	cgthreeattach_sbus(struct device *, struct device *, void *);
static void	cgthreeattach_obio(struct device *, struct device *, void *);

static void	cgthreeunblank(struct device *);
static void	cgthreeattach __P((struct cgthree_softc *, char *, int));

/* cdevsw prototypes */
cdev_decl(cgthree);

struct cfattach cgthree_sbus_ca = {
	sizeof(struct cgthree_softc), cgthreematch_sbus, cgthreeattach_sbus
};

struct cfattach cgthree_obio_ca = {
	sizeof(struct cgthree_softc), cgthreematch_obio, cgthreeattach_obio
};

extern struct cfdriver cgthree_cd;

/* frame buffer generic driver */
static struct fbdriver cgthreefbdriver = {
	cgthreeunblank, cgthreeopen, cgthreeclose, cgthreeioctl, cgthreepoll,
	cgthreemmap
};

static void cgthreeloadcmap __P((struct cgthree_softc *, int, int));
static void cgthree_set_video __P((struct cgthree_softc *, int));
static int cgthree_get_video __P((struct cgthree_softc *));

/* Video control parameters */
struct cg3_videoctrl {
	unsigned char	sense;		/* Monitor sense value */
	unsigned char	vctrl[12];
} cg3_videoctrl[] = {
/* Missing entries: sense 0x10, 0x30, 0x50 */
	{ 0x40, /* this happens to be my 19'' 1152x900 gray-scale monitor */
	   {0xbb, 0x2b, 0x3, 0xb, 0xb3, 0x3, 0xaf, 0x2b, 0x2, 0xa, 0xff, 0x1}
	},
	{ 0x00, /* default? must be last */
	   {0xbb, 0x2b, 0x3, 0xb, 0xb3, 0x3, 0xaf, 0x2b, 0x2, 0xa, 0xff, 0x1}
	}
};

/*
 * Match a cgthree.
 */
int
cgthreematch_sbus(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct sbus_attach_args *sa = aux;

	return (strcmp(cf->cf_driver->cd_name, sa->sa_name) == 0);
}

int
cgthreematch_obio(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	if (uoba->uoba_isobio4 == 0)
		return (strcmp(cf->cf_driver->cd_name, uoba->uoba_sbus.sa_name) == 0);

	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag, 0, oba->oba_paddr,
				4,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				NULL, NULL));
}

/*
 * Attach a display.  We need to notice if it is the console, too.
 */
void
cgthreeattach_sbus(parent, self, args)
	struct device *parent, *self;
	void *args;
{
	struct cgthree_softc *sc = (struct cgthree_softc *)self;
	struct sbus_attach_args *sa = args;
	struct fbdevice *fb = &sc->sc_fb;
	int node = sa->sa_node;
	int isconsole;
	char *name;
	bus_space_handle_t bh;

	/* Remember cookies for cgthree_mmap() */
	sc->sc_bustag = sa->sa_bustag;
	sc->sc_btype = (bus_type_t)sa->sa_slot;
	sc->sc_paddr = (bus_addr_t)sa->sa_offset;

	fb->fb_driver = &cgthreefbdriver;
	fb->fb_device = &sc->sc_dev;
	fb->fb_flags = sc->sc_dev.dv_cfdata->cf_flags & FB_USERMASK;
	fb->fb_type.fb_type = FBTYPE_SUN3COLOR;

	fb->fb_type.fb_depth = 8;
	fb_setsize_obp(fb, fb->fb_type.fb_depth, 1152, 900, node);

	/*
	 * When the ROM has mapped in a cgthree display, the address
	 * maps only the video RAM, so in any case we have to map the
	 * registers ourselves.  We only need the video RAM if we are
	 * going to print characters via rconsole.
	 */
	if (sbus_bus_map(sa->sa_bustag, sa->sa_slot,
			 sa->sa_offset + CG3REG_REG,
			 sizeof(struct fbcontrol),
			 BUS_SPACE_MAP_LINEAR,
			 0,
			 &bh) != 0) {
		printf("%s: cannot map control registers\n", self->dv_xname);
		return;
	}
	sc->sc_fbc = (struct fbcontrol *)bh;

	isconsole = fb_is_console(node);
	name = getpropstring(node, "model");

	if (sa->sa_npromvaddrs != 0)
		fb->fb_pixels = (caddr_t)sa->sa_promvaddrs[0];
	if (isconsole && fb->fb_pixels == NULL) {
		int ramsize = fb->fb_type.fb_height * fb->fb_linebytes;
		if (sbus_bus_map(sa->sa_bustag, sa->sa_slot,
				 sa->sa_offset + CG3REG_MEM,
				 ramsize,
				 BUS_SPACE_MAP_LINEAR,
				 0,
				 &bh) != 0) {
			printf("%s: cannot map pixels\n", self->dv_xname);
			return;
		}
		fb->fb_pixels = (char *)bh;
	}

	sbus_establish(&sc->sc_sd, &sc->sc_dev);
	cgthreeattach(sc, name, isconsole);
}

void
cgthreeattach_obio(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cgthree_softc *sc = (struct cgthree_softc *)self;
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;
	struct fbdevice *fb = &sc->sc_fb;
	int isconsole;
	char *name;
	bus_space_handle_t bh;

	if (uoba->uoba_isobio4 == 0) {
		cgthreeattach_sbus(parent, self, aux);
		return;
	}

	oba = &uoba->uoba_oba4;

	/* Remember cookies for cgthree_mmap() */
	sc->sc_bustag = oba->oba_bustag;
	sc->sc_btype = (bus_type_t)0;
	sc->sc_paddr = (bus_addr_t)oba->oba_paddr;

	fb->fb_driver = &cgthreefbdriver;
	fb->fb_device = &sc->sc_dev;
	fb->fb_flags = sc->sc_dev.dv_cfdata->cf_flags & FB_USERMASK;
	fb->fb_type.fb_type = FBTYPE_SUN3COLOR;
	fb->fb_type.fb_depth = 8;

	fb_setsize_eeprom(fb, fb->fb_type.fb_depth, 1152, 900);

	if (obio_bus_map(oba->oba_bustag, oba->oba_paddr,
			 CG3REG_REG,
			 sizeof(struct fbcontrol),
			 BUS_SPACE_MAP_LINEAR,
			 0,
			 &bh) != 0) {
		printf("%s: cannot map control registers\n", self->dv_xname);
		return;
	}
	sc->sc_fbc = (struct fbcontrol *)bh;

	isconsole = fb_is_console(0);
	name = "cgthree";

	if (isconsole) {
		int ramsize = fb->fb_type.fb_height * fb->fb_linebytes;
		if (obio_bus_map(oba->oba_bustag, oba->oba_paddr,
				 CG3REG_MEM,
				 ramsize,
				 BUS_SPACE_MAP_LINEAR,
				 0,
				 &bh) != 0) {
			printf("%s: cannot map pixels\n", self->dv_xname);
			return;
		}
		sc->sc_fb.fb_pixels = (char *)bh;
	}

	cgthreeattach(sc, name, isconsole);
}

void
cgthreeattach(sc, name, isconsole)
	struct cgthree_softc *sc;
	char *name;
	int isconsole;
{
	int i;
	struct fbdevice *fb = &sc->sc_fb;
	volatile struct fbcontrol *fbc = sc->sc_fbc;
	volatile struct bt_regs *bt = &fbc->fbc_dac;

	fb->fb_type.fb_cmsize = 256;
	fb->fb_type.fb_size = fb->fb_type.fb_height * fb->fb_linebytes;
	printf(": %s, %d x %d", name,
		fb->fb_type.fb_width, fb->fb_type.fb_height);

	/* Transfer video magic to board, if it's not running */
	if ((fbc->fbc_ctrl & FBC_TIMING) == 0) {
		int sense = (fbc->fbc_status & FBS_MSENSE);
		/* Search table for video timings fitting this monitor */
		for (i = 0; i < sizeof(cg3_videoctrl)/sizeof(cg3_videoctrl[0]);
		     i++) {
			int j;
			if (sense != cg3_videoctrl[i].sense)
				continue;

			printf(" setting video ctrl");
			for (j = 0; j < 12; j++)
				fbc->fbc_vcontrol[j] =
					cg3_videoctrl[i].vctrl[j];
			fbc->fbc_ctrl |= FBC_TIMING;
			break;
		}
	}

	/* Initialize the default color map. */
	bt_initcmap(&sc->sc_cmap, 256);
	cgthreeloadcmap(sc, 0, 256);

	/* make sure we are not blanked */
	cgthree_set_video(sc, 1);
	BT_INIT(bt, 0);

	if (isconsole) {
		printf(" (console)\n");
#ifdef RASTERCONSOLE
		fbrcons_init(fb);
#endif
	} else
		printf("\n");

	fb_attach(fb, isconsole);
}


int
cgthreeopen(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	int unit = minor(dev);

	if (unit >= cgthree_cd.cd_ndevs || cgthree_cd.cd_devs[unit] == NULL)
		return (ENXIO);
	return (0);
}

int
cgthreeclose(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{

	return (0);
}

int
cgthreeioctl(dev, cmd, data, flags, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flags;
	struct proc *p;
{
	struct cgthree_softc *sc = cgthree_cd.cd_devs[minor(dev)];
	struct fbgattr *fba;
	int error;

	switch (cmd) {

	case FBIOGTYPE:
		*(struct fbtype *)data = sc->sc_fb.fb_type;
		break;

	case FBIOGATTR:
		fba = (struct fbgattr *)data;
		fba->real_type = sc->sc_fb.fb_type.fb_type;
		fba->owner = 0;		/* XXX ??? */
		fba->fbtype = sc->sc_fb.fb_type;
		fba->sattr.flags = 0;
		fba->sattr.emu_type = sc->sc_fb.fb_type.fb_type;
		fba->sattr.dev_specific[0] = -1;
		fba->emu_types[0] = sc->sc_fb.fb_type.fb_type;
		fba->emu_types[1] = -1;
		break;

	case FBIOGETCMAP:
#define p ((struct fbcmap *)data)
		return (bt_getcmap(p, &sc->sc_cmap, 256, 1));

	case FBIOPUTCMAP:
		/* copy to software map */
		error = bt_putcmap(p, &sc->sc_cmap, 256, 1);
		if (error)
			return (error);
		/* now blast them into the chip */
		/* XXX should use retrace interrupt */
		cgthreeloadcmap(sc, p->index, p->count);
#undef p
		break;

	case FBIOGVIDEO:
		*(int *)data = cgthree_get_video(sc);
		break;

	case FBIOSVIDEO:
		cgthree_set_video(sc, *(int *)data);
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}

int
cgthreepoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{

	return (seltrue(dev, events, p));
}

/*
 * Undo the effect of an FBIOSVIDEO that turns the video off.
 */
static void
cgthreeunblank(dev)
	struct device *dev;
{

	cgthree_set_video((struct cgthree_softc *)dev, 1);
}

static void
cgthree_set_video(sc, enable)
	struct cgthree_softc *sc;
	int enable;
{

	if (enable)
		sc->sc_fbc->fbc_ctrl |= FBC_VENAB;
	else
		sc->sc_fbc->fbc_ctrl &= ~FBC_VENAB;
}

static int
cgthree_get_video(sc)
	struct cgthree_softc *sc;
{

	return ((sc->sc_fbc->fbc_ctrl & FBC_VENAB) != 0);
}

/*
 * Load a subset of the current (new) colormap into the Brooktree DAC.
 */
static void
cgthreeloadcmap(sc, start, ncolors)
	struct cgthree_softc *sc;
	int start, ncolors;
{
	volatile struct bt_regs *bt;
	u_int *ip;
	int count;

	ip = &sc->sc_cmap.cm_chip[BT_D4M3(start)];	/* start/4 * 3 */
	count = BT_D4M3(start + ncolors - 1) - BT_D4M3(start) + 3;
	bt = &sc->sc_fbc->fbc_dac;
	bt->bt_addr = BT_D4M4(start);
	while (--count >= 0)
		bt->bt_cmap = *ip++;
}

/*
 * Return the address that would map the given device at the given
 * offset, allowing for the given protection, or return -1 for error.
 *
 * The cg3 is mapped starting at 256KB, for pseudo-compatibility with
 * the cg4 (which had an overlay plane in the first 128K and an enable
 * plane in the next 128K).  X11 uses only 256k+ region but tries to
 * map the whole thing, so we repeatedly map the first 256K to the
 * first page of the color screen.  If someone tries to use the overlay
 * and enable regions, they will get a surprise....
 *
 * As well, mapping at an offset of 0x04000000 causes the cg3 to be
 * mapped in flat mode without the cg4 emulation.
 */
paddr_t
cgthreemmap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
	struct cgthree_softc *sc = cgthree_cd.cd_devs[minor(dev)];
	bus_space_handle_t bh;

#define START		(128*1024 + 128*1024)
#define NOOVERLAY	(0x04000000)

	if (off & PGOFSET)
		panic("cgthreemmap");
	if (off < 0)
		return (-1);
	if ((u_int)off >= NOOVERLAY)
		off -= NOOVERLAY;
	else if ((u_int)off >= START)
		off -= START;
	else
		off = 0;

	if (off >= sc->sc_fb.fb_type.fb_size)
		return (-1);

	if (bus_space_mmap(sc->sc_bustag,
			   sc->sc_btype,
			   sc->sc_paddr + CG3REG_MEM + off,
			   BUS_SPACE_MAP_LINEAR, &bh))
		return (-1);

	return ((paddr_t)bh);
}

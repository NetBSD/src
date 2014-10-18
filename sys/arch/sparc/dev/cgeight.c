/*	$NetBSD: cgeight.c,v 1.50 2014/10/18 08:33:26 snj Exp $	*/

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
 *	from @(#)cgthree.c	8.2 (Berkeley) 10/30/93
 */

/*
 * Copyright (c) 1995 Theo de Raadt.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/*
 * color display (cgeight) driver.
 *
 * Does not handle interrupts, even though they can occur.
 *
 * XXX should defer colormap updates to vertical retrace interrupts
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cgeight.c,v 1.50 2014/10/18 08:33:26 snj Exp $");

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

#include <machine/autoconf.h>
#include <machine/eeprom.h>

#include <dev/sun/fbio.h>
#include <dev/sun/fbvar.h>
#include <dev/sun/btreg.h>
#include <dev/sun/btvar.h>
#include <dev/sun/pfourreg.h>

/* per-display variables */
struct cgeight_softc {
	struct fbdevice	sc_fb;		/* frame buffer device */
	bus_space_tag_t	sc_bustag;
	bus_addr_t	sc_paddr;	/* phys address for device mmap() */

	volatile struct fbcontrol *sc_fbc;	/* Brooktree registers */
	union bt_cmap	sc_cmap;	/* Brooktree color map */
};

/* autoconfiguration driver */
static void	cgeightattach(device_t, device_t, void *);
static int	cgeightmatch(device_t, cfdata_t, void *);
#if defined(SUN4)
static void	cgeightunblank(device_t);
#endif

static int	cg8_pfour_probe(void *, void *);

CFATTACH_DECL_NEW(cgeight, sizeof(struct cgeight_softc),
    cgeightmatch, cgeightattach, NULL, NULL);

extern struct cfdriver cgeight_cd;

dev_type_open(cgeightopen);
dev_type_ioctl(cgeightioctl);
dev_type_mmap(cgeightmmap);

const struct cdevsw cgeight_cdevsw = {
	.d_open = cgeightopen,
	.d_close = nullclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = cgeightioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = cgeightmmap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

#if defined(SUN4)
/* frame buffer generic driver */
static struct fbdriver cgeightfbdriver = {
	cgeightunblank, cgeightopen, nullclose, cgeightioctl,
	nopoll, cgeightmmap, nokqfilter
};

static void cgeightloadcmap(struct cgeight_softc *, int, int);
static int cgeight_get_video(struct cgeight_softc *);
static void cgeight_set_video(struct cgeight_softc *, int);
#endif

/*
 * Match a cgeight.
 */
static int
cgeightmatch(device_t parent, cfdata_t cf, void *aux)
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	if (uoba->uoba_isobio4 == 0)
		return (0);

	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag, oba->oba_paddr,
				4,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				cg8_pfour_probe, NULL));
}

static int
cg8_pfour_probe(void *vaddr, void *arg)
{

	return (fb_pfour_id(vaddr) == PFOUR_ID_COLOR24);
}

/*
 * Attach a display.  We need to notice if it is the console, too.
 */
static void
cgeightattach(device_t parent, device_t self, void *aux)
{
#if defined(SUN4)
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba = &uoba->uoba_oba4;
	struct cgeight_softc *sc = device_private(self);
	struct fbdevice *fb = &sc->sc_fb;
	bus_space_handle_t bh;
	volatile struct bt_regs *bt;
	int ramsize, i, isconsole;

	sc->sc_bustag = oba->oba_bustag;
	sc->sc_paddr = (bus_addr_t)oba->oba_paddr;

	/* Map the pfour register. */
	if (bus_space_map(oba->oba_bustag, oba->oba_paddr,
			  sizeof(uint32_t),
			  BUS_SPACE_MAP_LINEAR,
			  &bh) != 0) {
		printf("%s: cannot map pfour register\n",
			device_xname(self));
		return;
	}
	fb->fb_pfour = (volatile uint32_t *)bh;

	fb->fb_driver = &cgeightfbdriver;
	fb->fb_device = self;
	fb->fb_type.fb_type = FBTYPE_MEMCOLOR;
	fb->fb_flags = device_cfdata(self)->cf_flags & FB_USERMASK;
	fb->fb_flags |= FB_PFOUR;

	ramsize = PFOUR_COLOR_OFF_END - PFOUR_COLOR_OFF_OVERLAY;

	fb->fb_type.fb_depth = 24;
	fb_setsize_eeprom(fb, fb->fb_type.fb_depth, 1152, 900);

	sc->sc_fb.fb_type.fb_cmsize = 256;
	sc->sc_fb.fb_type.fb_size = ramsize;
	printf(": cgeight/p4, %d x %d",
		fb->fb_type.fb_width,
		fb->fb_type.fb_height);

	isconsole = 0;

	if (CPU_ISSUN4) {
		struct eeprom *eep = (struct eeprom *)eeprom_va;

		/*
		 * Assume this is the console if there's no eeprom info
		 * to be found.
		 */
		if (eep == NULL || eep->eeConsole == EE_CONS_P4OPT)
			isconsole = fb_is_console(0);
	}

#if 0
	/*
	 * We don't do any of the console handling here.  Instead,
	 * we let the bwtwo driver pick up the overlay plane and
	 * use it instead.  Rconsole should have better performance
	 * with the 1-bit depth.
	 *      -- Jason R. Thorpe <thorpej@NetBSD.org>
	 */

	/*
	 * When the ROM has mapped in a cgfour display, the address
	 * maps only the video RAM, so in any case we have to map the
	 * registers ourselves.  We only need the video RAM if we are
	 * going to print characters via rconsole.
	 */

	if (isconsole) {
		/* XXX this is kind of a waste */
		fb->fb_pixels = mapiodev(ca->ca_ra.ra_reg,
					 PFOUR_COLOR_OFF_OVERLAY, ramsize);
	}
#endif

	/* Map the Brooktree. */
	if (bus_space_map(oba->oba_bustag,
			  oba->oba_paddr + PFOUR_COLOR_OFF_CMAP,
			  sizeof(struct fbcontrol),
			  BUS_SPACE_MAP_LINEAR,
			  &bh) != 0) {
		printf("%s: cannot map control registers\n",
			device_xname(self));
		return;
	}
	sc->sc_fbc = (volatile struct fbcontrol *)bh;

#if 0	/* XXX thorpej ??? */
	/* tell the enable plane to look at the mono image */
	memset(ca->ca_ra.ra_vaddr, 0xff,
	    sc->sc_fb.fb_type.fb_width * sc->sc_fb.fb_type.fb_height / 8);
#endif

	/* grab initial (current) color map */
	bt = &sc->sc_fbc->fbc_dac;
	bt->bt_addr = 0;
	for (i = 0; i < 256 * 3 / 4; i++)
		sc->sc_cmap.cm_chip[i] = bt->bt_cmap;

	BT_INIT(bt, 0);

#if 0	/* see above */
	if (isconsole) {
		printf(" (console)\n");
#if defined(RASTERCONSOLE) && 0	/* XXX been told it doesn't work well. */
		fbrcons_init(fb);
#endif
	} else
#endif /* 0 */
		printf("\n");

	/*
	 * Even though we're not using rconsole, we'd still like
	 * to notice if we're the console framebuffer.
	 */
	fb_attach(&sc->sc_fb, isconsole);
#endif
}

int
cgeightopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	int unit = minor(dev);

	if (device_lookup(&cgeight_cd, unit) == NULL)
		return (ENXIO);
	return (0);
}

int
cgeightioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
#if defined(SUN4)
	struct cgeight_softc *sc = device_lookup_private(&cgeight_cd,
							 minor(dev));
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
		cgeightloadcmap(sc, p->index, p->count);
#undef p
		break;

	case FBIOGVIDEO:
		*(int *)data = cgeight_get_video(sc);
		break;

	case FBIOSVIDEO:
		cgeight_set_video(sc, *(int *)data);
		break;

	default:
		return (ENOTTY);
	}
#endif /* SUN4 */

	return (0);
}

/*
 * Return the address that would map the given device at the given
 * offset, allowing for the given protection, or return -1 for error.
 *
 * The cg8 maps its overlay plane at 0 for 128K, followed by the
 * enable plane for 128K, followed by the colour for as long as it
 * goes. Starting at 8MB, it maps the ramdac for PAGE_SIZE, then the p4
 * register for PAGE_SIZE, then the bootrom for 0x40000.
 */
paddr_t
cgeightmmap(dev_t dev, off_t off, int prot)
{
	struct cgeight_softc *sc = device_lookup_private(&cgeight_cd,
							 minor(dev));
	off_t poff;

#define START_ENABLE	(128*1024)
#define START_COLOR	((128*1024) + (128*1024))
#define COLOR_SIZE	(sc->sc_fb.fb_type.fb_width * \
			    sc->sc_fb.fb_type.fb_height * 3)
#define END_COLOR	(START_COLOR + COLOR_SIZE)
#define START_SPECIAL	0x800000
#define PROMSIZE	0x40000
#define NOOVERLAY	(0x04000000)

	if (off & PGOFSET)
		panic("cgeightmap");

	if (off < 0)
		return (-1);
	else if ((u_int)off >= NOOVERLAY) {
		off -= NOOVERLAY;

		/*
		 * X11 maps a huge chunk of the frame buffer; far more than
		 * there really is. We compensate by double-mapping the
		 * first page for as many other pages as it wants
		 */
		while ((u_int)off >= COLOR_SIZE)
			off -= COLOR_SIZE;	/* XXX thorpej ??? */

		poff = off + PFOUR_COLOR_OFF_COLOR;
	} else if ((u_int)off < START_ENABLE) {
		/*
		 * in overlay plane
		 */
		poff = PFOUR_COLOR_OFF_OVERLAY + off;
	} else if ((u_int)off < START_COLOR) {
		/*
		 * in enable plane
		 */
		poff = (off - START_ENABLE) + PFOUR_COLOR_OFF_ENABLE;
	} else if ((u_int)off < sc->sc_fb.fb_type.fb_size) {
		/*
		 * in colour plane
		 */
		poff = (off - START_COLOR) + PFOUR_COLOR_OFF_COLOR;
	} else if ((u_int)off < START_SPECIAL) {
		/*
		 * hole
		 */
		poff = 0;	/* XXX */
	} else if ((u_int)off == START_SPECIAL) {
		/*
		 * colour map (Brooktree)
		 */
		poff = PFOUR_COLOR_OFF_CMAP;
	} else if ((u_int)off == START_SPECIAL + PAGE_SIZE) {
		/*
		 * p4 register
		 */
		poff = 0;
	} else if ((u_int)off > (START_SPECIAL + (PAGE_SIZE * 2)) &&
	    (u_int) off < (START_SPECIAL + (PAGE_SIZE * 2) + PROMSIZE)) {
		/*
		 * rom
		 */
		poff = 0x8000 + (off - (START_SPECIAL + (PAGE_SIZE * 2)));
	} else
		return (-1);

	return (bus_space_mmap(sc->sc_bustag,
		sc->sc_paddr, poff,
		prot, BUS_SPACE_MAP_LINEAR));
}

#if defined(SUN4)
/*
 * Undo the effect of an FBIOSVIDEO that turns the video off.
 */
static void
cgeightunblank(device_t dev)
{

	cgeight_set_video(device_private(dev), 1);
}

static int
cgeight_get_video(struct cgeight_softc *sc)
{

	return (fb_pfour_get_video(&sc->sc_fb));
}

static void
cgeight_set_video(struct cgeight_softc *sc, int enable)
{

	fb_pfour_set_video(&sc->sc_fb, enable);
}

/*
 * Load a subset of the current (new) colormap into the Brooktree DAC.
 */
static void
cgeightloadcmap(struct cgeight_softc *sc, int start, int ncolors)
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
#endif /* SUN4 */

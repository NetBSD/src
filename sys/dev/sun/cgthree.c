/*	$NetBSD: cgthree.c,v 1.10 2003/08/25 17:50:30 uwe Exp $ */

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
 * color display (cgthree) driver.
 *
 * Does not handle interrupts, even though they can occur.
 *
 * XXX should defer colormap updates to vertical retrace interrupts
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cgthree.c,v 1.10 2003/08/25 17:50:30 uwe Exp $");

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

#include <dev/sun/fbio.h>
#include <dev/sun/fbvar.h>

#include <dev/sun/btreg.h>
#include <dev/sun/btvar.h>
#include <dev/sun/cgthreereg.h>
#include <dev/sun/cgthreevar.h>

static void	cgthreeunblank(struct device *);
static void	cgthreeloadcmap(struct cgthree_softc *, int, int);
static void	cgthree_set_video(struct cgthree_softc *, int);
static int	cgthree_get_video(struct cgthree_softc *);

extern struct cfdriver cgthree_cd;

dev_type_open(cgthreeopen);
dev_type_ioctl(cgthreeioctl);
dev_type_mmap(cgthreemmap);

const struct cdevsw cgthree_cdevsw = {
	cgthreeopen, nullclose, noread, nowrite, cgthreeioctl,
	nostop, notty, nopoll, cgthreemmap, nokqfilter
};

/* frame buffer generic driver */
static struct fbdriver cgthreefbdriver = {
	cgthreeunblank, cgthreeopen, nullclose, cgthreeioctl, nopoll,
	cgthreemmap, nokqfilter
};

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

	fb->fb_driver = &cgthreefbdriver;
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

	return (bus_space_mmap(sc->sc_bustag,
		sc->sc_paddr, CG3REG_MEM + off,
		prot, BUS_SPACE_MAP_LINEAR));
}

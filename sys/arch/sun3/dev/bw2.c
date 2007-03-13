/*	$NetBSD: bw2.c,v 1.31.2.1 2007/03/13 16:50:07 ad Exp $	*/

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
 *	@(#)bwtwo.c	8.1 (Berkeley) 6/11/93
 */

/*
 * black&white display (bw2) driver.
 *
 * Does not handle interrupts, even though they can occur.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bw2.c,v 1.31.2.1 2007/03/13 16:50:07 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/tty.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <dev/sun/fbio.h>
#include <machine/idprom.h>
#include <machine/pmap.h>

#include <sun3/dev/fbvar.h>
#include <sun3/dev/bw2reg.h>
#include <sun3/dev/p4reg.h>

/* per-display variables */
struct bw2_softc {
	struct	device sc_dev;		/* base device */
	struct	fbdevice sc_fb;		/* frame buffer device */
	int 	sc_phys;		/* display RAM (phys addr) */
	/* If using overlay plane of something, it is... */
	int	sc_ovtype;		/* ... this type. */
	int sc_video_on;
};

/* autoconfiguration driver */
static void	bw2attach(struct device *, struct device *, void *);
static int	bw2match(struct device *, struct cfdata *, void *);

CFATTACH_DECL(bwtwo, sizeof(struct bw2_softc),
    bw2match, bw2attach, NULL, NULL);

extern struct cfdriver bwtwo_cd;

dev_type_open(bw2open);
dev_type_ioctl(bw2ioctl);
dev_type_mmap(bw2mmap);

const struct cdevsw bwtwo_cdevsw = {
	bw2open, nullclose, noread, nowrite, bw2ioctl,
	nostop, notty, nopoll, bw2mmap, nokqfilter,
};

/* XXX we do not handle frame buffer interrupts */

static int bw2gvideo(struct fbdevice *, void *);
static int bw2svideo(struct fbdevice *, void *);

static struct fbdriver bw2fbdriver = {
	bw2open, nullclose, bw2mmap, nokqfilter,
	fb_noioctl,
	bw2gvideo, bw2svideo,
	fb_noioctl, fb_noioctl, };

static int 
bw2match(struct device *parent, struct cfdata *cf, void *args)
{
	struct confargs *ca = args;
	int mid, p4id, peekval;
	void *p4reg;

	/* No default address support. */
	if (ca->ca_paddr == -1)
		return (0);

	/*
	 * Slight hack here:  The low four bits of the
	 * config flags, if set, restrict the match to
	 * that machine "implementation" only.
	 */
	mid = cf->cf_flags & IDM_IMPL_MASK;
	if (mid && (mid != (cpu_machine_id & IDM_IMPL_MASK)))
		return (0);

	/*
	 * Make sure something is there, and if so,
	 * see if it looks like a P4 register.
	 */
	p4reg = bus_tmapin(ca->ca_bustype, ca->ca_paddr);
	peekval = peek_long(p4reg);
	p4id = (peekval == -1) ?
		P4_NOTFOUND : fb_pfour_id(p4reg);
	bus_tmapout(p4reg);
	if (peekval == -1)
		return (0);

	/*
	 * The config flag 0x40 if set means we should match
	 * only on a CG? overlay plane.  We can use only the
	 * CG4 and CG8, which both have a P4 register.
	 */
	if (cf->cf_flags & 0x40) {
		switch (p4id) {
		case P4_ID_COLOR8P1:
		case P4_ID_COLOR24:
			return (1);
		case P4_NOTFOUND:
		default:
			return (0);
		}
	}

	/*
	 * OK, we are expecting a plain old BW2, and
	 * there may or may not be a P4 register.
	 */
	switch (p4id) {
	case P4_ID_BW:
	case P4_NOTFOUND:
		return (1);
	default:
#ifdef	DEBUG
		printf("bwtwo at 0x%lx match p4id=0x%x fails\n",
			   ca->ca_paddr, p4id & 0xFF);
#endif
		break;
	}

	return (0);
}

/*
 * Attach a display.  We need to notice if it is the console, too.
 */
static void 
bw2attach(struct device *parent, struct device *self, void *args)
{
	struct bw2_softc *sc = (struct bw2_softc *)self;
	struct fbdevice *fb = &sc->sc_fb;
	struct confargs *ca = args;
	struct fbtype *fbt;
	void *p4reg;
	int p4id, tmp;
	int pixeloffset;		/* offset to framebuffer */

	fbt = &fb->fb_fbtype;
	fbt->fb_type = FBTYPE_SUN2BW;
	fbt->fb_width = 1152;	/* default - see below */
	fbt->fb_height = 900;	/* default - see below */
	fbt->fb_depth = 1;
	fbt->fb_cmsize = 0;
	fbt->fb_size = BW2_FBSIZE;	/* default - see below */
	fb->fb_driver = &bw2fbdriver;
	fb->fb_private = sc;
	fb->fb_name  = sc->sc_dev.dv_xname;
	fb->fb_flags = device_cfdata(&sc->sc_dev)->cf_flags;

	/* Set up default pixel offset.  May be changed below. */
	pixeloffset = 0;

	/* Does it have a P4 register? */
	p4reg = bus_mapin(ca->ca_bustype, ca->ca_paddr, 4);
	p4id = fb_pfour_id(p4reg);
	if (p4id != P4_NOTFOUND)
		fb->fb_pfour = p4reg;
	else
		bus_mapout(p4reg, 4);

	switch (p4id) {
	case P4_NOTFOUND:
		pixeloffset = 0;
		break;

	case P4_ID_BW:
		pixeloffset = P4_BW_OFF;
		break;

	default:
		printf("%s: bad p4id=0x%x\n", fb->fb_name, p4id);
		/* Must be some kinda color... */
		/* fall through */
	case P4_ID_COLOR8P1:
	case P4_ID_COLOR24:
		sc->sc_ovtype = p4id;
		pixeloffset = P4_COLOR_OFF_OVERLAY;
		break;
	}
	sc->sc_phys = ca->ca_paddr + pixeloffset;

	/*
	 * Determine width and height as follows:
	 * If it has a P4 register, use that;
	 * else if unit==0, use the EEPROM size,
	 * else make our best guess.
	 */
	if (fb->fb_pfour)
		fb_pfour_setsize(fb);
	/* XXX device_unit() abuse */
	else if (device_unit(&sc->sc_dev) == 0)
		fb_eeprom_setsize(fb);
	else {
		/* Guess based on machine ID. */
		switch (cpu_machine_id) {
#ifdef	_SUN3_
		case ID_SUN3_60:
			/*
			 * Only the model 60 can have hi-res.
			 * Look at the "resolution" jumper.
			 */
			tmp = bus_peek(BUS_OBMEM, BW2_CR_PADDR, 1);
			if ((tmp != -1) && (tmp & 0x80) == 0)
				goto high_res;
			break;

		case ID_SUN3_260:
			/* The Sun3/260 is ALWAYS high-resolution! */
			/* fall through */
		high_res:
			fbt->fb_width = 1600;
			fbt->fb_height = 1280;
			fbt->fb_size = BW2_FBSIZE_HIRES;
			break;
#endif	/* SUN3 */

		default:
			/* Leave the defaults set above. */
			break;
		}
	}
	printf(" (%dx%d)\n", fbt->fb_width, fbt->fb_height);

	/* Make sure video is on. */
	tmp = 1;
	bw2svideo(fb, &tmp);

	/* Let /dev/fb know we are here. */
	fb_attach(fb, 1);
}

int 
bw2open(dev_t dev, int flags, int mode, struct lwp *l)
{
	int unit = minor(dev);

	if (unit >= bwtwo_cd.cd_ndevs || bwtwo_cd.cd_devs[unit] == NULL)
		return (ENXIO);
	return (0);
}

int 
bw2ioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	struct bw2_softc *sc = bwtwo_cd.cd_devs[minor(dev)];

	return (fbioctlfb(&sc->sc_fb, cmd, data));
}

/*
 * Return the address that would map the given device at the given
 * offset, allowing for the given protection, or return -1 for error.
 */
paddr_t 
bw2mmap(dev_t dev, off_t off, int prot)
{
	struct bw2_softc *sc = bwtwo_cd.cd_devs[minor(dev)];
	int size = sc->sc_fb.fb_fbtype.fb_size;

	if (off & PGOFSET)
		panic("bw2mmap");

	if ((off < 0) || (off >= size))
		return (-1);

	/*
	 * I turned on PMAP_NC here to disable the cache as I was
	 * getting horribly broken behaviour without it.
	 */
	return ((sc->sc_phys + off) | PMAP_NC);
}

/* FBIOGVIDEO: */
static int 
bw2gvideo(struct fbdevice *fb, void *data)
{
	struct bw2_softc *sc = fb->fb_private;
	int *on = data;

	*on = sc->sc_video_on;
	return (0);
}

/* FBIOSVIDEO: */
static int 
bw2svideo(struct fbdevice *fb, void *data)
{
	struct bw2_softc *sc = fb->fb_private;
	int *on = data;

	if (sc->sc_video_on == *on)
		return (0);
	sc->sc_video_on = *on;

	if (fb->fb_pfour)
		fb_pfour_set_video(fb, sc->sc_video_on);
	else
		enable_video(sc->sc_video_on);

	return(0);
}


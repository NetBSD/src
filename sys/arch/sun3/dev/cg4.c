/*	$NetBSD: cg4.c,v 1.21.4.1 2001/09/13 01:14:48 thorpej Exp $	*/

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
 *	from: @(#)cgthree.c	8.2 (Berkeley) 10/30/93
 */

/*
 * color display (cg4) driver.
 *
 * Credits, history:
 * Gordon Ross created this driver based on the cg3 driver from
 * the sparc port as distributed in BSD 4.4 Lite, but included
 * support for only the "type B" adapter (Brooktree DACs).
 * Ezra Story added support for the "type A" (AMD DACs).
 *
 * Todo:
 * Make this driver handle video interrupts.
 * Defer colormap updates to vertical retrace interrupts.
 */

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
#include <machine/fbio.h>
#include <machine/idprom.h>
#include <machine/pmap.h>

#include <sun3/dev/fbvar.h>
#include <sun3/dev/btreg.h>
#include <sun3/dev/cg4reg.h>
#include <sun3/dev/p4reg.h>

union bt_cmap_u {
	u_char  btcm_char[256 * 3];		/* raw data */
	u_char  btcm_rgb[256][3];		/* 256 R/G/B entries */
	u_int   btcm_int[256 * 3 / 4];	/* the way the chip gets loaded */
};

#define CG4_TYPE_A 0	/* AMD DACs */
#define CG4_TYPE_B 1	/* Brooktree DACs */

cdev_decl(cg4);

#define	CG4_MMAP_SIZE (CG4_OVERLAY_SIZE + CG4_ENABLE_SIZE + CG4_PIXMAP_SIZE)

#define CMAP_SIZE 256
struct soft_cmap {
	u_char r[CMAP_SIZE];
	u_char g[CMAP_SIZE];
	u_char b[CMAP_SIZE];
};

/* per-display variables */
struct cg4_softc {
	struct	device sc_dev;		/* base device */
	struct	fbdevice sc_fb;		/* frame buffer device */
	int 	sc_cg4type;		/* A or B */
	int 	sc_pa_overlay;		/* phys. addr. of overlay plane */
	int 	sc_pa_enable;		/* phys. addr. of enable plane */
	int 	sc_pa_pixmap;		/* phys. addr. of color plane */
	int 	sc_video_on;		/* zero if blanked */
	void	*sc_va_cmap;		/* Colormap h/w (mapped KVA) */
	void	*sc_btcm;		/* Soft cmap, Brooktree format */
	void	(*sc_ldcmap) __P((struct cg4_softc *));
	struct soft_cmap sc_cmap;	/* Soft cmap, user format */
};

/* autoconfiguration driver */
static void	cg4attach __P((struct device *, struct device *, void *));
static int	cg4match __P((struct device *, struct cfdata *, void *));

struct cfattach cgfour_ca = {
	sizeof(struct cg4_softc), cg4match, cg4attach
};

extern struct cfdriver cgfour_cd;

static int	cg4gattr   __P((struct fbdevice *, void *));
static int	cg4gvideo  __P((struct fbdevice *, void *));
static int	cg4svideo  __P((struct fbdevice *, void *));
static int	cg4getcmap __P((struct fbdevice *, void *));
static int	cg4putcmap __P((struct fbdevice *, void *));

#ifdef	_SUN3_
static void	cg4a_init   __P((struct cg4_softc *));
static void	cg4a_ldcmap __P((struct cg4_softc *));
#endif	/* SUN3 */

static void	cg4b_init   __P((struct cg4_softc *));
static void	cg4b_ldcmap __P((struct cg4_softc *));

static struct fbdriver cg4_fbdriver = {
	cg4open, cg4close, cg4mmap, cg4gattr,
	cg4gvideo, cg4svideo,
	cg4getcmap, cg4putcmap };

/*
 * Match a cg4.
 */
static int
cg4match(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
	struct confargs *ca = args;
	int mid, p4id, peekval, tmp;
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
	 * The config flag 0x10 if set means we are
	 * looking for a Type A board (3/110).
	 */
	if (cf->cf_flags & 0x10) {
#ifdef	_SUN3_
		/* Type A: Check for AMD RAMDACs in control space. */
		if (bus_peek(BUS_OBIO, CG4A_OBIO_CMAP, 1) == -1)
			return (0);
		/* Check for the overlay plane. */
		tmp = ca->ca_paddr + CG4A_OFF_OVERLAY;
		if (bus_peek(ca->ca_bustype, tmp, 1) == -1)
			return (0);
		/* OK, it looks like a Type A. */
		return (1);
#else	/* SUN3 */
		/* Only the Sun3/110 ever has a type A. */
		return (0);
#endif	/* SUN3 */
	}

	/*
	 * From here on, it is a type B or nothing.
	 * The config flag 0x20 if set means there
	 * is no P4 register.  (bus error)
	 */
	if ((cf->cf_flags & 0x20) == 0) {
		p4reg = bus_tmapin(ca->ca_bustype, ca->ca_paddr);
		peekval = peek_long(p4reg);
		p4id = (peekval == -1) ?
			P4_NOTFOUND : fb_pfour_id(p4reg);
		bus_tmapout(p4reg);
		if (peekval == -1)
			return (0);
		if (p4id != P4_ID_COLOR8P1) {
#ifdef	DEBUG
			printf("cgfour at 0x%x match p4id=0x%x fails\n",
				   ca->ca_paddr, p4id & 0xFF);
#endif
			return (0);
		}
	}

	/*
	 * Check for CMAP hardware and overlay plane.
	 */
	tmp = ca->ca_paddr + CG4B_OFF_CMAP;
	if (bus_peek(ca->ca_bustype, tmp, 4) == -1)
		return (0);
	tmp = ca->ca_paddr + CG4B_OFF_OVERLAY;
	if (bus_peek(ca->ca_bustype, tmp, 1) == -1)
		return (0);

	return (1);
}

/*
 * Attach a display.  We need to notice if it is the console, too.
 */
static void
cg4attach(parent, self, args)
	struct device *parent, *self;
	void *args;
{
	struct cg4_softc *sc = (struct cg4_softc *)self;
	struct fbdevice *fb = &sc->sc_fb;
	struct confargs *ca = args;
	struct fbtype *fbt;
	int tmp;

	fbt = &fb->fb_fbtype;
	fbt->fb_type = FBTYPE_SUN4COLOR;
	fbt->fb_width = 1152;	/* default - see below */
	fbt->fb_height = 900;	/* default - see below */
	fbt->fb_depth = 8;
	fbt->fb_cmsize = 256;
	fbt->fb_size = CG4_MMAP_SIZE;
	fb->fb_driver = &cg4_fbdriver;
	fb->fb_private = sc;
	fb->fb_name  = sc->sc_dev.dv_xname;
	fb->fb_flags = sc->sc_dev.dv_cfdata->cf_flags;

	/*
	 * The config flag 0x10 if set means we are
	 * attaching a Type A (3/110) which has the
	 * AMD RAMDACs in control space, and no P4.
	 */
	if (fb->fb_flags & 0x10) {
#ifdef	_SUN3_
		sc->sc_cg4type = CG4_TYPE_A;
		sc->sc_ldcmap  = cg4a_ldcmap;
		sc->sc_pa_overlay = ca->ca_paddr + CG4A_OFF_OVERLAY;
		sc->sc_pa_enable  = ca->ca_paddr + CG4A_OFF_ENABLE;
		sc->sc_pa_pixmap  = ca->ca_paddr + CG4A_OFF_PIXMAP;
		sc->sc_va_cmap = bus_mapin(BUS_OBIO, CG4A_OBIO_CMAP,
		                           sizeof(struct amd_regs));
		cg4a_init(sc);
#else	/* SUN3 */
		panic("cgfour flags 0x10");
#endif	/* SUN3 */
	} else {
		sc->sc_cg4type = CG4_TYPE_B;
		sc->sc_ldcmap  = cg4b_ldcmap;
		sc->sc_pa_overlay = ca->ca_paddr + CG4B_OFF_OVERLAY;
		sc->sc_pa_enable  = ca->ca_paddr + CG4B_OFF_ENABLE;
		sc->sc_pa_pixmap  = ca->ca_paddr + CG4B_OFF_PIXMAP;
		tmp               = ca->ca_paddr + CG4B_OFF_CMAP;
		sc->sc_va_cmap = bus_mapin(ca->ca_bustype, tmp,
		                           sizeof(struct bt_regs));
		cg4b_init(sc);
	}

	if ((fb->fb_flags & 0x20) == 0) {
		/* It is supposed to have a P4 register. */
		fb->fb_pfour = bus_mapin(ca->ca_bustype, ca->ca_paddr, 4);
	}

	/*
	 * Determine width and height as follows:
	 * If it has a P4 register, use that;
	 * else if unit==0, use the EEPROM size,
	 * else make our best guess.
	 */
	if (fb->fb_pfour)
		fb_pfour_setsize(fb);
	else if (sc->sc_dev.dv_unit == 0)
		fb_eeprom_setsize(fb);
	else {
		/* Guess based on machine ID. */
		switch (cpu_machine_id) {
		default:
			/* Leave the defaults set above. */
			break;
		}
	}
	printf(" (%dx%d)\n", fbt->fb_width, fbt->fb_height);

	/*
	 * Make sure video is on.  This driver uses a
	 * black colormap to blank the screen, so if
	 * there is any global enable, set it here.
	 */
	tmp = 1;
	cg4svideo(fb, &tmp);
	if (fb->fb_pfour)
		fb_pfour_set_video(fb, 1);
	else
		enable_video(1);

	/* Let /dev/fb know we are here. */
	fb_attach(fb, 4);
}

int
cg4open(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	int unit = minor(dev);

	if (unit >= cgfour_cd.cd_ndevs || cgfour_cd.cd_devs[unit] == NULL)
		return (ENXIO);
	return (0);
}

int
cg4close(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{

	return (0);
}

int
cg4ioctl(dev, cmd, data, flags, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flags;
	struct proc *p;
{
	struct cg4_softc *sc = cgfour_cd.cd_devs[minor(dev)];

	return (fbioctlfb(&sc->sc_fb, cmd, data));
}

/*
 * Return the address that would map the given device at the given
 * offset, allowing for the given protection, or return -1 for error.
 *
 * X11 expects its mmap'd region to look like this:
 * 	128k overlay data memory
 * 	128k overlay enable bitmap
 * 	1024k color memory
 *
 * The hardware looks completely different.
 */
paddr_t
cg4mmap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
	struct cg4_softc *sc = cgfour_cd.cd_devs[minor(dev)];
	int physbase;

	if (off & PGOFSET)
		panic("cg4mmap");

	if ((off < 0) || (off >= CG4_MMAP_SIZE))
		return (-1);

	if (off < 0x40000) {
		if (off < 0x20000) {
			physbase = sc->sc_pa_overlay;
		} else {
			/* enable plane */
			off -= 0x20000;
			physbase = sc->sc_pa_enable;
		}
	} else {
		/* pixel map */
		off -= 0x40000;
		physbase = sc->sc_pa_pixmap;
	}

	/*
	 * I turned on PMAP_NC here to disable the cache as I was
	 * getting horribly broken behaviour without it.
	 */
	return ((physbase + off) | PMAP_NC);
}

/*
 * Internal ioctl functions.
 */

/* FBIOGATTR: */
static int  cg4gattr(fb, data)
	struct fbdevice *fb;
	void *data;
{
	struct fbgattr *fba = data;

	fba->real_type = fb->fb_fbtype.fb_type;
	fba->owner = 0;		/* XXX - TIOCCONS stuff? */
	fba->fbtype = fb->fb_fbtype;
	fba->sattr.flags = 0;
	fba->sattr.emu_type = fb->fb_fbtype.fb_type;
	fba->sattr.dev_specific[0] = -1;
	fba->emu_types[0] = fb->fb_fbtype.fb_type;
	fba->emu_types[1] = -1;
	return (0);
}

/* FBIOGVIDEO: */
static int  cg4gvideo(fb, data)
	struct fbdevice *fb;
	void *data;
{
	struct cg4_softc *sc = fb->fb_private;
	int *on = data;

	*on = sc->sc_video_on;
	return (0);
}

/* FBIOSVIDEO: */
static int cg4svideo(fb, data)
	struct fbdevice *fb;
	void *data;
{
	struct cg4_softc *sc = fb->fb_private;
	int *on = data;

	if (sc->sc_video_on == *on)
		return (0);
	sc->sc_video_on = *on;

	(*sc->sc_ldcmap)(sc);
	return (0);
}

/*
 * FBIOGETCMAP:
 * Copy current colormap out to user space.
 */
static int cg4getcmap(fb, data)
	struct fbdevice *fb;
	void *data;
{
	struct cg4_softc *sc = fb->fb_private;
	struct soft_cmap *cm = &sc->sc_cmap;
	struct fbcmap *fbcm = data;
	int error, start, count;

	start = fbcm->index;
	count = fbcm->count;
	if ((start < 0) || (start >= CMAP_SIZE) ||
	    (count < 0) || (start + count > CMAP_SIZE) )
		return (EINVAL);

	if ((error = copyout(&cm->r[start], fbcm->red, count)) != 0)
		return (error);

	if ((error = copyout(&cm->g[start], fbcm->green, count)) != 0)
		return (error);

	if ((error = copyout(&cm->b[start], fbcm->blue, count)) != 0)
		return (error);

	return (0);
}

/*
 * FBIOPUTCMAP:
 * Copy new colormap from user space and load.
 */
static int cg4putcmap(fb, data)
	struct fbdevice *fb;
	void *data;
{
	struct cg4_softc *sc = fb->fb_private;
	struct soft_cmap *cm = &sc->sc_cmap;
	struct fbcmap *fbcm = data;
	int error, start, count;

	start = fbcm->index;
	count = fbcm->count;
	if ((start < 0) || (start >= CMAP_SIZE) ||
	    (count < 0) || (start + count > CMAP_SIZE) )
		return (EINVAL);

	if ((error = copyin(fbcm->red, &cm->r[start], count)) != 0)
		return (error);

	if ((error = copyin(fbcm->green, &cm->g[start], count)) != 0)
		return (error);

	if ((error = copyin(fbcm->blue, &cm->b[start], count)) != 0)
		return (error);

	(*sc->sc_ldcmap)(sc);
	return (0);
}

/****************************************************************
 * Routines for the "Type A" hardware
 ****************************************************************/
#ifdef	_SUN3_

static void
cg4a_init(sc)
	struct cg4_softc *sc;
{
	volatile struct amd_regs *ar = sc->sc_va_cmap;
	struct soft_cmap *cm = &sc->sc_cmap;
	int i;

	/* Grab initial (current) color map. */
	for(i = 0; i < 256; i++) {
		cm->r[i] = ar->r[i];
		cm->g[i] = ar->g[i];
		cm->b[i] = ar->b[i];
	}
}

static void
cg4a_ldcmap(sc)
	struct cg4_softc *sc;
{
	volatile struct amd_regs *ar = sc->sc_va_cmap;
	struct soft_cmap *cm = &sc->sc_cmap;
	int i;

	/*
	 * Now blast them into the chip!
	 * XXX Should use retrace interrupt!
	 * Just set a "need load" bit and let the
	 * retrace interrupt handler do the work.
	 */
	if (sc->sc_video_on) {
		/* Update H/W colormap. */
		for (i = 0; i < 256; i++) {
			ar->r[i] = cm->r[i];
			ar->g[i] = cm->g[i];
			ar->b[i] = cm->b[i];
		}
	} else {
		/* Clear H/W colormap. */
		for (i = 0; i < 256; i++) {
			ar->r[i] = 0;
			ar->g[i] = 0;
			ar->b[i] = 0;
		}
	}
}
#endif	/* SUN3 */

/****************************************************************
 * Routines for the "Type B" hardware
 ****************************************************************/

static void
cg4b_init(sc)
	struct cg4_softc *sc;
{
	volatile struct bt_regs *bt = sc->sc_va_cmap;
	struct soft_cmap *cm = &sc->sc_cmap;
	union bt_cmap_u *btcm;
	int i;

	/* Need a buffer for colormap format translation. */
	btcm = malloc(sizeof(*btcm), M_DEVBUF, M_WAITOK);
	sc->sc_btcm = btcm;

	/*
	 * BT458 chip initialization as described in Brooktree's
	 * 1993 Graphics and Imaging Product Databook (DB004-1/93).
	 *
	 * It appears that the 3/60 uses the low byte, and the 3/80
	 * uses the high byte, while both ignore the other bytes.
	 * Writing same value to all bytes works on both.
	 */
	bt->bt_addr = 0x04040404;	/* select read mask register */
	bt->bt_ctrl = ~0;       	/* all planes on */
	bt->bt_addr = 0x05050505;	/* select blink mask register */
	bt->bt_ctrl = 0;        	/* all planes non-blinking */
	bt->bt_addr = 0x06060606;	/* select command register */
	bt->bt_ctrl = 0x43434343;	/* palette enabled, overlay planes enabled */
	bt->bt_addr = 0x07070707;	/* select test register */
	bt->bt_ctrl = 0;        	/* not test mode */

	/* grab initial (current) color map */
	bt->bt_addr = 0;
#ifdef	_SUN3_
	/* Sun3/60 wants 32-bit access, packed. */
	for (i = 0; i < (256 * 3 / 4); i++)
		btcm->btcm_int[i] = bt->bt_cmap;
#else	/* SUN3 */
	/* Sun3/80 wants 8-bits in the high byte. */
	for (i = 0; i < (256 * 3); i++)
		btcm->btcm_char[i] = bt->bt_cmap >> 24;
#endif	/* SUN3 */

	/* Transpose into H/W cmap into S/W form. */
	for (i = 0; i < 256; i++) {
		cm->r[i] = btcm->btcm_rgb[i][0];
		cm->g[i] = btcm->btcm_rgb[i][1];
		cm->b[i] = btcm->btcm_rgb[i][2];
	}
}

static void
cg4b_ldcmap(sc)
	struct cg4_softc *sc;
{
	volatile struct bt_regs *bt = sc->sc_va_cmap;
	struct soft_cmap *cm = &sc->sc_cmap;
	union bt_cmap_u *btcm = sc->sc_btcm;
	int i;

	/* Transpose S/W cmap into H/W form. */
	for (i = 0; i < 256; i++) {
		btcm->btcm_rgb[i][0] = cm->r[i];
		btcm->btcm_rgb[i][1] = cm->g[i];
		btcm->btcm_rgb[i][2] = cm->b[i];
	}

	/*
	 * Now blast them into the chip!
	 * XXX Should use retrace interrupt!
	 * Just set a "need load" bit and let the
	 * retrace interrupt handler do the work.
	 */
	bt->bt_addr = 0;

#ifdef	_SUN3_
	/* Sun3/60 wants 32-bit access, packed. */
	if (sc->sc_video_on) {
		/* Update H/W colormap. */
		for (i = 0; i < (256 * 3 / 4); i++)
			bt->bt_cmap = btcm->btcm_int[i];
	} else {
		/* Clear H/W colormap. */
		for (i = 0; i < (256 * 3 / 4); i++)
			bt->bt_cmap = 0;
	}
#else	/* SUN3 */
	/* Sun3/80 wants 8-bits in the high byte. */
	if (sc->sc_video_on) {
		/* Update H/W colormap. */
		for (i = 0; i < (256 * 3); i++)
			bt->bt_cmap = btcm->btcm_char[i] << 24;
	} else {
		/* Clear H/W colormap. */
		for (i = 0; i < (256 * 3); i++)
			bt->bt_cmap = 0;
	}
#endif	/* SUN3 */
}


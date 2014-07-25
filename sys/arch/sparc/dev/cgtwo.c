/*	$NetBSD: cgtwo.c,v 1.57 2014/07/25 08:10:34 dholland Exp $ */

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
 *	from: @(#)cgthree.c	8.2 (Berkeley) 10/30/93
 */

/*
 * color display (cgtwo) driver.
 *
 * Does not handle interrupts, even though they can occur.
 *
 * XXX should defer colormap updates to vertical retrace interrupts
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cgtwo.c,v 1.57 2014/07/25 08:10:34 dholland Exp $");

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

#include <dev/sun/fbio.h>
#include <dev/sun/fbvar.h>

#include <dev/vme/vmevar.h>

#include <machine/eeprom.h>
#include <machine/cgtworeg.h>


/* per-display variables */
struct cgtwo_softc {
	struct	fbdevice sc_fb;		/* frame buffer device */
	vme_addr_t		sc_paddr;
	vme_chipset_tag_t	sc_ct;
	bus_space_tag_t		sc_bt;
	volatile struct cg2statusreg *sc_reg;	/* CG2 control registers */
	volatile u_short *sc_cmap;
#define sc_redmap(sc)	((sc)->sc_cmap)
#define sc_greenmap(sc)	((sc)->sc_cmap + CG2_CMSIZE)
#define sc_bluemap(sc)	((sc)->sc_cmap + 2 * CG2_CMSIZE)
};

/* autoconfiguration driver */
static int	cgtwomatch(device_t, cfdata_t, void *);
static void	cgtwoattach(device_t, device_t, void *);
static void	cgtwounblank(device_t);
int		cgtwogetcmap(struct cgtwo_softc *, struct fbcmap *);
int		cgtwoputcmap(struct cgtwo_softc *, struct fbcmap *);

CFATTACH_DECL_NEW(cgtwo, sizeof(struct cgtwo_softc),
    cgtwomatch, cgtwoattach, NULL, NULL);

extern struct cfdriver cgtwo_cd;

dev_type_open(cgtwoopen);
dev_type_ioctl(cgtwoioctl);
dev_type_mmap(cgtwommap);

const struct cdevsw cgtwo_cdevsw = {
	.d_open = cgtwoopen,
	.d_close = nullclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = cgtwoioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = cgtwommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

/* frame buffer generic driver */
static struct fbdriver cgtwofbdriver = {
	cgtwounblank, cgtwoopen, nullclose, cgtwoioctl, nopoll, cgtwommap,
	nokqfilter
};

/*
 * Match a cgtwo.
 */
static int
cgtwomatch(device_t parent, cfdata_t cf, void *aux)
{
	struct vme_attach_args	*va = aux;
	vme_chipset_tag_t	ct = va->va_vct;
	vme_am_t		mod;

	/*
	 * Mask out invalid flags from the user.
	 */
	cf->cf_flags &= FB_USERMASK;

	mod = 0x3d; /* VME_AM_A24 | VME_AM_MBO | VME_AM_SUPER | VME_AM_DATA */
	if (vme_probe(ct, va->r[0].offset + CG2_CTLREG_OFF, 2, mod, VME_D16,
			  0, 0)) {
		return (0);
	}

	return (1);
}

/*
 * Attach a display.  We need to notice if it is the console, too.
 */
static void
cgtwoattach(device_t parent, device_t self, void *aux)
{
	struct vme_attach_args	*va = aux;
	vme_chipset_tag_t	ct = va->va_vct;
	bus_space_tag_t		bt;
	bus_space_handle_t	bh;
	vme_am_t		mod;
	vme_mapresc_t resc;
	struct cgtwo_softc *sc = device_private(self);
	struct fbdevice *fb = &sc->sc_fb;
	struct eeprom *eep = (struct eeprom *)eeprom_va;
	int isconsole = 0;

	sc->sc_ct = ct;
	fb->fb_driver = &cgtwofbdriver;
	fb->fb_device = self;
	fb->fb_type.fb_type = FBTYPE_SUN2COLOR;
	fb->fb_flags = device_cfdata(self)->cf_flags;

	fb->fb_type.fb_depth = 8;
	fb_setsize_eeprom(fb, fb->fb_type.fb_depth, 1152, 900);

	fb->fb_type.fb_cmsize = 256;
	fb->fb_type.fb_size = roundup(CG2_MAPPED_SIZE, PAGE_SIZE);
	printf(": cgtwo, %d x %d",
	       fb->fb_type.fb_width, fb->fb_type.fb_height);

	/*
	 * When the ROM has mapped in a cgtwo display, the address
	 * maps only the video RAM, so in any case we have to map the
	 * registers ourselves.  We only need the video RAM if we are
	 * going to print characters via rconsole.
	 */
	sc->sc_paddr = va->r[0].offset;
	mod = 0x3d; /* VME_AM_A24 | VME_AM_MBO | VME_AM_SUPER | VME_AM_DATA */

	if (vme_space_map(ct, sc->sc_paddr + CG2_ROPMEM_OFF +
				offsetof(struct cg2fb, status.reg),
			sizeof(struct cg2statusreg), mod, VME_D16, 0,
			&bt, &bh, &resc) != 0)
		panic("cgtwo: vme_map status");
	sc->sc_bt = bt;
	sc->sc_reg = (volatile struct cg2statusreg *)bh; /* XXX */

	if (vme_space_map(ct, sc->sc_paddr + CG2_ROPMEM_OFF +
				offsetof(struct cg2fb, redmap[0]),
			3 * CG2_CMSIZE, mod, VME_D16, 0,
			&bt, &bh, &resc) != 0)
		panic("cgtwo: vme_map cmap");
	sc->sc_cmap = (volatile u_short *)bh; /* XXX */

	/*
	 * Assume this is the console if there's no eeprom info
	 * to be found.
	 */
	if (eep == NULL || eep->eeConsole == EE_CONS_COLOR)
		isconsole = fb_is_console(0);
	else
		isconsole = 0;

	if (isconsole) {
		if (vme_space_map(ct, sc->sc_paddr + CG2_PIXMAP_OFF,
				CG2_PIXMAP_SIZE, mod, VME_D16, 0,
				  &bt, &bh, &resc) != 0)
			panic("cgtwo: vme_map pixels");

		fb->fb_pixels = (void *)bh; /* XXX */
		printf(" (console)\n");
#ifdef RASTERCONSOLE
		fbrcons_init(fb);
#endif
	} else
		printf("\n");

	fb_attach(fb, isconsole);
}

int
cgtwoopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	int unit = minor(dev);

	if (device_lookup(&cgtwo_cd, unit) == NULL)
		return (ENXIO);
	return (0);
}

int
cgtwoioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	register struct cgtwo_softc *sc = device_lookup_private(&cgtwo_cd,
								minor(dev));
	register struct fbgattr *fba;

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
		return cgtwogetcmap(sc, (struct fbcmap *) data);

	case FBIOPUTCMAP:
		return cgtwoputcmap(sc, (struct fbcmap *) data);

	case FBIOGVIDEO:
		*(int *)data = sc->sc_reg->video_enab;
		break;

	case FBIOSVIDEO:
		sc->sc_reg->video_enab = (*(int*)data) & 1;
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
cgtwounblank(device_t dev)
{
	struct cgtwo_softc *sc = device_private(dev);
	sc->sc_reg->video_enab = 1;
}

/*
 */
int
cgtwogetcmap(struct cgtwo_softc *sc, struct fbcmap *cmap)
{
	u_char red[CG2_CMSIZE], green[CG2_CMSIZE], blue[CG2_CMSIZE];
	int error, start, count, ecount;
	register u_int i;
	register volatile u_short *p;

	start = cmap->index;
	count = cmap->count;
	ecount = start + count;
	if (start >= CG2_CMSIZE || count > CG2_CMSIZE - start)
		return (EINVAL);

	/* XXX - Wait for retrace? */

	/* Copy hardware to local arrays. */
	p = &sc_redmap(sc)[start];
	for (i = start; i < ecount; i++)
		red[i] = *p++;
	p = &sc_greenmap(sc)[start];
	for (i = start; i < ecount; i++)
		green[i] = *p++;
	p = &sc_bluemap(sc)[start];
	for (i = start; i < ecount; i++)
		blue[i] = *p++;

	/* Copy local arrays to user space. */
	if ((error = copyout(red + start, cmap->red, count)) != 0)
		return (error);
	if ((error = copyout(green + start, cmap->green, count)) != 0)
		return (error);
	if ((error = copyout(blue + start, cmap->blue, count)) != 0)
		return (error);

	return (0);
}

/*
 */
int
cgtwoputcmap(struct cgtwo_softc *sc, struct fbcmap *cmap)
{
	u_char red[CG2_CMSIZE], green[CG2_CMSIZE], blue[CG2_CMSIZE];
	int error;
	u_int start, count, ecount;
	register u_int i;
	register volatile u_short *p;

	start = cmap->index;
	count = cmap->count;
	ecount = start + count;
	if (start >= CG2_CMSIZE || count > CG2_CMSIZE - start)
		return (EINVAL);

	/* Copy from user space to local arrays. */
	if ((error = copyin(cmap->red, red + start, count)) != 0)
		return (error);
	if ((error = copyin(cmap->green, green + start, count)) != 0)
		return (error);
	if ((error = copyin(cmap->blue, blue + start, count)) != 0)
		return (error);

	/* XXX - Wait for retrace? */

	/* Copy from local arrays to hardware. */
	p = &sc_redmap(sc)[start];
	for (i = start; i < ecount; i++)
		*p++ = red[i];
	p = &sc_greenmap(sc)[start];
	for (i = start; i < ecount; i++)
		*p++ = green[i];
	p = &sc_bluemap(sc)[start];
	for (i = start; i < ecount; i++)
		*p++ = blue[i];

	return (0);
}

/*
 * Return the address that would map the given device at the given
 * offset, allowing for the given protection, or return -1 for error.
 */
paddr_t
cgtwommap(dev_t dev, off_t off, int prot)
{
	extern int sparc_vme_mmap_cookie(vme_addr_t, vme_am_t,
					 bus_space_handle_t *);

	register struct cgtwo_softc *sc = device_lookup_private(&cgtwo_cd,
								minor(dev));
	vme_am_t mod;
	bus_space_handle_t bh;

	if (off & PGOFSET)
		panic("cgtwommap");

	if (off >= sc->sc_fb.fb_type.fb_size)
		return (-1);

	/* Apparently, the pixels are in 32-bit data space */
	mod = 0x3d; /* VME_AM_A24 | VME_AM_MBO | VME_AM_SUPER | VME_AM_DATA */

	if (sparc_vme_mmap_cookie(sc->sc_paddr + off, mod, &bh) != 0)
		panic("cgtwommap");

	return ((paddr_t)bh);
}

/*	$NetBSD: cgtwo.c,v 1.25 1998/01/28 02:27:16 thorpej Exp $ */

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
 * color display (cgtwo) driver.
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

#include <vm/vm.h>

#include <machine/fbio.h>
#include <machine/autoconf.h>
#include <machine/pmap.h>
#include <machine/fbvar.h>

#include <dev/vme/vmevar.h>

#if defined(SUN4)
#include <machine/eeprom.h>
#endif
#include <machine/conf.h>
#include <machine/cgtworeg.h>


/* per-display variables */
struct cgtwo_softc {
	struct	device sc_dev;		/* base device */
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
static void	cgtwoattach __P((struct device *, struct device *, void *));
static int	cgtwomatch __P((struct device *, struct cfdata *, void *));
static void	cgtwounblank __P((struct device *));
int		cgtwogetcmap __P((struct cgtwo_softc *, struct fbcmap *));
int		cgtwoputcmap __P((struct cgtwo_softc *, struct fbcmap *));

/* cdevsw prototypes */
cdev_decl(cgtwo);

struct cfattach cgtwo_ca = {
	sizeof(struct cgtwo_softc), cgtwomatch, cgtwoattach
};

extern struct cfdriver cgtwo_cd;

/* frame buffer generic driver */
static struct fbdriver cgtwofbdriver = {
	cgtwounblank, cgtwoopen, cgtwoclose, cgtwoioctl, cgtwopoll, cgtwommap
};

extern int fbnode;
extern struct tty *fbconstty;

/*
 * Match a cgtwo.
 */
int
cgtwomatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct vme_attach_args	*va = aux;
	vme_chipset_tag_t	ct = va->vma_chipset_tag;
	bus_space_tag_t		bt = va->vma_bustag;
	vme_mod_t		mod;

	/*
	 * Mask out invalid flags from the user.
	 */
	cf->cf_flags &= FB_USERMASK;

	mod = VMEMOD_A24 | VMEMOD_S | VMEMOD_D;
	if (vme_bus_probe(ct, bt, va->vma_reg[0] + CG2_CTLREG_OFF, 2, mod)) {
		return (1);
	}

	return (0);
}

/*
 * Attach a display.  We need to notice if it is the console, too.
 */
void
cgtwoattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct vme_attach_args	*va = aux;
	vme_chipset_tag_t	ct = va->vma_chipset_tag;
	bus_space_tag_t		bt = va->vma_bustag;
	bus_space_handle_t	bh;
	vme_mod_t		mod;
	register struct cgtwo_softc *sc = (struct cgtwo_softc *)self;
	register int node = 0;
	int isconsole = 0;
	char *nam = NULL;

	sc->sc_ct = ct;
	sc->sc_bt = bt;
	sc->sc_fb.fb_driver = &cgtwofbdriver;
	sc->sc_fb.fb_device = &sc->sc_dev;
	sc->sc_fb.fb_type.fb_type = FBTYPE_SUN2COLOR;
	sc->sc_fb.fb_flags = sc->sc_dev.dv_cfdata->cf_flags;

	sc->sc_fb.fb_type.fb_depth = 8;
	fb_setsize(&sc->sc_fb, sc->sc_fb.fb_type.fb_depth,
	    1152, 900, node, BUS_VME16 /* XXX*/);

	sc->sc_fb.fb_type.fb_cmsize = 256;
	sc->sc_fb.fb_type.fb_size = roundup(CG2_MAPPED_SIZE, NBPG);
	printf(": %s, %d x %d", nam,
	       sc->sc_fb.fb_type.fb_width, sc->sc_fb.fb_type.fb_height);

	/*
	 * When the ROM has mapped in a cgtwo display, the address
	 * maps only the video RAM, so in any case we have to map the
	 * registers ourselves.  We only need the video RAM if we are
	 * going to print characters via rconsole.
	 */
#if defined(SUN4)
	if (CPU_ISSUN4) {
		struct eeprom *eep = (struct eeprom *)eeprom_va;
		/*
		 * Assume this is the console if there's no eeprom info
		 * to be found.
		 */
		if (eep == NULL || eep->eeConsole == EE_CONS_COLOR)
			isconsole = (fbconstty != NULL);
		else
			isconsole = 0;
	}
#endif

	sc->sc_paddr = va->vma_reg[0];
	mod = VMEMOD_A24 | VMEMOD_S | VMEMOD_D;

	if (isconsole) {
		if (vme_bus_map(ct, sc->sc_paddr + CG2_PIXMAP_OFF,
				CG2_PIXMAP_SIZE, mod, bt, &bh) != 0)
			panic("cgtwo: vme_map pixels");

		sc->sc_fb.fb_pixels = (caddr_t)bh;
	}

	if (vme_bus_map(ct, sc->sc_paddr + CG2_ROPMEM_OFF +
				offsetof(struct cg2fb, status.reg),
			sizeof(struct cg2statusreg), mod, bt, &bh) != 0)
		panic("cgtwo: vme_map status");
	sc->sc_reg = (volatile struct cg2statusreg *)bh;

	if (vme_bus_map(ct, sc->sc_paddr + CG2_ROPMEM_OFF +
				offsetof(struct cg2fb, redmap[0]),
			3 * CG2_CMSIZE, mod, bt, &bh) != 0)
		panic("cgtwo: vme_map cmap");
	sc->sc_cmap = (volatile u_short *)bh;

	if (isconsole) {
		printf(" (console)\n");
#ifdef RASTERCONSOLE
		fbrcons_init(&sc->sc_fb);
#endif
	} else
		printf("\n");

	if (node == fbnode || CPU_ISSUN4)
		fb_attach(&sc->sc_fb, isconsole);
}

int
cgtwoopen(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	int unit = minor(dev);

	if (unit >= cgtwo_cd.cd_ndevs || cgtwo_cd.cd_devs[unit] == NULL)
		return (ENXIO);
	return (0);
}

int
cgtwoclose(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{

	return (0);
}

int
cgtwoioctl(dev, cmd, data, flags, p)
	dev_t dev;
	u_long cmd;
	register caddr_t data;
	int flags;
	struct proc *p;
{
	register struct cgtwo_softc *sc = cgtwo_cd.cd_devs[minor(dev)];
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

int
cgtwopoll(dev, events, p)
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
cgtwounblank(dev)
	struct device *dev;
{
	struct cgtwo_softc *sc = (struct cgtwo_softc *)dev;
	sc->sc_reg->video_enab = 1;
}

/*
 */
int
cgtwogetcmap(sc, cmap)
	register struct cgtwo_softc *sc;
	register struct fbcmap *cmap;
{
	u_char red[CG2_CMSIZE], green[CG2_CMSIZE], blue[CG2_CMSIZE];
	int error, start, count, ecount;
	register u_int i;
	register volatile u_short *p;

	start = cmap->index;
	count = cmap->count;
	ecount = start + count;
	if (start >= CG2_CMSIZE || ecount > CG2_CMSIZE)
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
cgtwoputcmap(sc, cmap)
	register struct cgtwo_softc *sc;
	register struct fbcmap *cmap;
{
	u_char red[CG2_CMSIZE], green[CG2_CMSIZE], blue[CG2_CMSIZE];
	int error, start, count, ecount;
	register u_int i;
	register volatile u_short *p;

	start = cmap->index;
	count = cmap->count;
	ecount = start + count;
	if (start >= CG2_CMSIZE || ecount > CG2_CMSIZE)
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
int
cgtwommap(dev, off, prot)
	dev_t dev;
	int off, prot;
{
	register struct cgtwo_softc *sc = cgtwo_cd.cd_devs[minor(dev)];
	vme_mod_t mod;
	int handle;

	if (off & PGOFSET)
		panic("cgtwommap");

	if ((unsigned)off >= sc->sc_fb.fb_type.fb_size)
		return (-1);

	/* Apparently, the pixels are in 32-bit data space */
	mod = VMEMOD_A24 | VMEMOD_S | VMEMOD_D | VMEMOD_D32;

	if (vme_bus_mmap_cookie(sc->sc_ct, sc->sc_paddr + off,
				mod, sc->sc_bt, &handle) != 0)
		panic("cgtwommap");

	return (handle);
}

/*	$NetBSD: et4000.c,v 1.5.2.1 2001/10/10 11:56:00 fvdl Exp $	*/
/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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
 * Thanks to:
 *	Leo Weppelman
 *	'Maximum Entropy'
 *	Thomas Gerner
 *	Juergen Orscheidt
 * for their help and for code that I could refer to when writing this driver.
 *
 * Defining DEBUG_ET4000 will cause the driver to *always* attach.  Use for
 * debugging register settings.
 */

/*
#define DEBUG_ET4000
*/

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/vnode.h>
#include <atari/vme/vmevar.h>

#include <machine/iomap.h>
#include <machine/video.h>
#include <machine/mfp.h>
#include <machine/cpu.h>
#include <atari/atari/device.h>
#include <atari/dev/grfioctl.h>
#include <atari/dev/grf_etreg.h>

/*
 * Allow a 8Kb io-region and a 1MB frame buffer to be mapped. This
 * is more or less required by the XFree server.  The X server also
 * requires that the frame buffer be mapped above 0x3fffff.
 */
#define REG_MAPPABLE	(8 * 1024)		/* 0x2000 */
#define FRAME_MAPPABLE	(1 * 1024 * 1024)	/* 0x100000 */
#define FRAME_BASE	(4 * 1024 * 1024)	/* 0x400000 */
#define VGA_MAPPABLE	(128 * 1024)		/* 0x20000 */
#define VGA_BASE	0xa0000

static int	et_vme_match __P((struct device *, struct cfdata *, void *));
static void	et_vme_attach __P((struct device *, struct device *, void *));
static int	et_probe_addresses __P((struct vme_attach_args *));
static void	et_start __P((bus_space_tag_t *, bus_space_handle_t *, int *,
		    u_char *));
static void	et_stop __P((bus_space_tag_t *, bus_space_handle_t *, int *,
		    u_char *));
static int	et_detect __P((bus_space_tag_t *, bus_space_tag_t *,
		    bus_space_handle_t *, bus_space_handle_t *, u_int));

dev_decl(et,open);
dev_decl(et,close);
dev_decl(et,read);
dev_decl(et,write);
dev_decl(et,ioctl);
dev_decl(et,mmap);

int		eton __P((dev_t));
int		etoff __P((dev_t));

/* Register and screen memory addresses for ET4000 based VME cards */
static struct et_addresses {
	u_long io_addr;
	u_long io_size;
	u_long mem_addr;
	u_long mem_size;
} etstd[] = {
	{ 0xfebf0000, REG_MAPPABLE, 0xfec00000, FRAME_MAPPABLE }, /* Crazy Dots VME & II */
	{ 0xfed00000, REG_MAPPABLE, 0xfec00000, FRAME_MAPPABLE }, /* Spektrum I & HC */
	{ 0xfed80000, REG_MAPPABLE, 0xfec00000, FRAME_MAPPABLE }  /* Spektrum TC */
};

#define NETSTD (sizeof(etstd) / sizeof(etstd[0]))

struct grfabs_et_priv {
	volatile caddr_t	regkva;
	volatile caddr_t	memkva;
	int			regsz;
	int			memsz;
} et_priv;

struct et_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_ioh;
	bus_space_handle_t sc_memh;
	int sc_flags;
	int sc_iobase;
	int sc_maddr;
	int sc_iosize;
	int sc_msize;
};

#define ET_SC_FLAGS_INUSE 1

struct cfattach et_ca = {
	sizeof(struct et_softc), et_vme_match, et_vme_attach
};

extern struct cfdriver et_cd;

/*
 * Look for a ET4000 (Crazy Dots) card on the VME bus.  We might
 * match Spektrum cards too (untested).
 */
int 
et_vme_match(pdp, cfp, auxp)
	struct device	*pdp;
	struct cfdata	*cfp;
	void		*auxp;
{
	struct vme_attach_args *va = auxp;

	return(et_probe_addresses(va));
}

static int
et_probe_addresses(va)
	struct vme_attach_args *va;
{
	int i, found = 0;
	bus_space_tag_t iot;
	bus_space_tag_t memt;
	bus_space_handle_t ioh;
	bus_space_handle_t memh;

	iot = va->va_iot;
	memt = va->va_memt;

/* Loop around our possible addresses looking for a match */
	for (i = 0; i < NETSTD; i++) {
		struct et_addresses *et_ap = &etstd[i];
		struct vme_attach_args vat = *va;

		if (vat.va_irq != VMECF_IRQ_DEFAULT) {
			printf("et probe: config error: no irq support\n");
			return(0);
		}
		if (vat.va_iobase == VMECF_IOPORT_DEFAULT)
			vat.va_iobase = et_ap->io_addr;
		if (vat.va_maddr == VMECF_MEM_DEFAULT)
			vat.va_maddr = et_ap->mem_addr;
		if (vat.va_iosize == VMECF_IOSIZE_DEFAULT)
			vat.va_iosize = et_ap->io_size;
		if (vat.va_msize == VMECF_MEMSIZ_DEFAULT)
			vat.va_msize = et_ap->mem_size;
		if (bus_space_map(iot, vat.va_iobase, vat.va_iosize, 0,
				  &ioh)) {
			printf("et probe: cannot map io area\n");
			return(0);
		}
		if (bus_space_map(memt, vat.va_maddr, vat.va_msize,
			  	  BUS_SPACE_MAP_LINEAR|BUS_SPACE_MAP_CACHEABLE,
			  	  &memh)) {
			bus_space_unmap(iot, ioh, vat.va_iosize);
			printf("et probe: cannot map memory area\n");
			return(0);
		}
		found = et_detect(&iot, &memt, &ioh, &memh, vat.va_msize);
		bus_space_unmap(iot, ioh, vat.va_iosize);
		bus_space_unmap(memt, memh, vat.va_msize);
		if (found) {
			*va = vat;
			return(1);
		}
	}
	return(0);
}

static void
et_start(iot, ioh, vgabase, saved)
	bus_space_tag_t *iot;
	bus_space_handle_t *ioh;
	int *vgabase;
	u_char *saved;
{
	/* Enable VGA */
	bus_space_write_1(*iot, *ioh, GREG_VIDEOSYSENABLE, 0x01);
	/* Check whether colour (base = 3d0) or mono (base = 3b0) mode */
	*vgabase = (bus_space_read_1(*iot, *ioh, GREG_MISC_OUTPUT_R) & 0x01)
	    ? 0x3d0 : 0x3b0;
	/* Enable 'Tseng Extensions' - writes to CRTC and ATC[16] */
	bus_space_write_1(*iot, *ioh, GREG_HERCULESCOMPAT, 0x03);
	bus_space_write_1(*iot, *ioh, *vgabase + 0x08, 0xa0);
	/* Set up 16 bit I/O, memory, Tseng addressing and linear mapping */
	bus_space_write_1(*iot, *ioh, *vgabase + 0x04, 0x36);
	bus_space_write_1(*iot, *ioh, *vgabase + 0x05, 0xf0);
	/* Enable writes to CRTC[0..7] */
	bus_space_write_1(*iot, *ioh, *vgabase + 0x04, 0x11);
	*saved = bus_space_read_1(*iot, *ioh, *vgabase + 0x05);
	bus_space_write_1(*iot, *ioh, *vgabase + 0x05, *saved & 0x7f);
	/* Map all memory for video modes */
	bus_space_write_1(*iot, *ioh, 0x3ce, 0x06);
	bus_space_write_1(*iot, *ioh, 0x3cf, 0x01);
}

static void
et_stop(iot, ioh, vgabase, saved)
	bus_space_tag_t *iot;
	bus_space_handle_t *ioh;
	int *vgabase;
	u_char *saved;
{
	/* Restore writes to CRTC[0..7] */
	bus_space_write_1(*iot, *ioh, *vgabase + 0x04, 0x11);
	*saved = bus_space_read_1(*iot, *ioh, *vgabase + 0x05);
	bus_space_write_1(*iot, *ioh, *vgabase + 0x05, *saved | 0x80);
	/* Disable 'Tseng Extensions' */
	bus_space_write_1(*iot, *ioh, *vgabase + 0x08, 0x00);
	bus_space_write_1(*iot, *ioh, GREG_DISPMODECONTROL, 0x29);
	bus_space_write_1(*iot, *ioh, GREG_HERCULESCOMPAT, 0x01);
}

static int
et_detect(iot, memt, ioh, memh, memsize)
	bus_space_tag_t *iot, *memt;
	bus_space_handle_t *ioh, *memh;
	u_int memsize;
{
	u_char orig, new, saved;
	int vgabase;

	/* Test accessibility of registers and memory */
	if(!bus_space_peek_1(*iot, *ioh, GREG_STATUS1_R))
		return(0);
	if(!bus_space_peek_1(*memt, *memh, 0))
		return(0);

	et_start(iot, ioh, &vgabase, &saved);

	/* Is the card a Tseng card?  Check read/write of ATC[16] */
	(void)bus_space_read_1(*iot, *ioh, vgabase + 0x0a);
	bus_space_write_1(*iot, *ioh, ACT_ADDRESS, 0x16 | 0x20);
	orig = bus_space_read_1(*iot, *ioh, ACT_ADDRESS_R);
	bus_space_write_1(*iot, *ioh, ACT_ADDRESS_W, (orig ^ 0x10));
	bus_space_write_1(*iot, *ioh, ACT_ADDRESS, 0x16 | 0x20);
	new = bus_space_read_1(*iot, *ioh, ACT_ADDRESS_R);
	bus_space_write_1(*iot, *ioh, ACT_ADDRESS, orig);
	if (new != (orig ^ 0x10)) {
#ifdef DEBUG_ET4000
		printf("et4000: ATC[16] failed (%x != %x)\n",
		    new, (orig ^ 0x10));
#else
		et_stop(iot, ioh, &vgabase, &saved);
		return(0);
#endif
	}
	/* Is the card and ET4000?  Check read/write of CRTC[33] */
	bus_space_write_1(*iot, *ioh, vgabase + 0x04, 0x33);
	orig = bus_space_read_1(*iot, *ioh, vgabase + 0x05);
	bus_space_write_1(*iot, *ioh, vgabase + 0x05, (orig ^ 0x0f));
	new = bus_space_read_1(*iot, *ioh, vgabase + 0x05);
	bus_space_write_1(*iot, *ioh, vgabase + 0x05, orig);
	if (new != (orig ^ 0x0f)) {
#ifdef DEBUG_ET4000
		printf("et4000: CRTC[33] failed (%x != %x)\n",
		    new, (orig ^ 0x0f));
#else
		et_stop(iot, ioh, &vgabase, &saved);
		return(0);
#endif
	}

	/* Set up video memory so we can read & write it */
	bus_space_write_1(*iot, *ioh, 0x3c4, 0x04);
	bus_space_write_1(*iot, *ioh, 0x3c5, 0x06);
	bus_space_write_1(*iot, *ioh, 0x3c4, 0x07);
	bus_space_write_1(*iot, *ioh, 0x3c5, 0xa8);
	bus_space_write_1(*iot, *ioh, 0x3ce, 0x01);
	bus_space_write_1(*iot, *ioh, 0x3cf, 0x00);
	bus_space_write_1(*iot, *ioh, 0x3ce, 0x03);
	bus_space_write_1(*iot, *ioh, 0x3cf, 0x00);
	bus_space_write_1(*iot, *ioh, 0x3ce, 0x05);
	bus_space_write_1(*iot, *ioh, 0x3cf, 0x40);

#define TEST_PATTERN 0xa5a5a5a5

	bus_space_write_4(*memt, *memh, 0x0, TEST_PATTERN);
	if (bus_space_read_4(*memt, *memh, 0x0) != TEST_PATTERN)
	{
#ifdef DEBUG_ET4000
		printf("et4000: Video base write/read failed\n");
#else
		et_stop(iot, ioh, &vgabase, &saved);
		return(0);
#endif
	}
	bus_space_write_4(*memt, *memh, memsize - 4, TEST_PATTERN);
	if (bus_space_read_4(*memt, *memh, memsize - 4) != TEST_PATTERN)
	{
#ifdef DEBUG_ET4000
		printf("et4000: Video top write/read failed\n");
#else
		et_stop(iot, ioh, &vgabase, &saved);
		return(0);
#endif
	}

	et_stop(iot, ioh, &vgabase, &saved);
	return(1);
}

static void
et_vme_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct et_softc *sc = (struct et_softc *)self;
	struct vme_attach_args *va = aux;
	bus_space_handle_t ioh;
	bus_space_handle_t memh;

	printf("\n");

	if (bus_space_map(va->va_iot, va->va_iobase, va->va_iosize, 0, &ioh))
		panic("et attach: cannot map io area\n");
	if (bus_space_map(va->va_memt, va->va_maddr, va->va_msize, 0, &memh))
		panic("et attach: cannot map mem area\n");

	sc->sc_iot = va->va_iot;
	sc->sc_ioh = ioh;
	sc->sc_memt = va->va_memt;
	sc->sc_memh = memh;
	sc->sc_flags = 0;
	sc->sc_iobase = va->va_iobase;
	sc->sc_maddr = va->va_maddr;
	sc->sc_iosize = va->va_iosize;
	sc->sc_msize = va->va_msize;

	et_priv.regkva = (volatile caddr_t)ioh;
	et_priv.memkva = (volatile caddr_t)memh;
	et_priv.regsz = va->va_iosize;
	et_priv.memsz = va->va_msize;
}

int
etopen(devvp, flags, devtype, p)
	struct vnode *devvp;
	int flags, devtype;
	struct proc *p;
{
	struct et_softc *sc;
	dev_t dev;

	dev = vdev_rdev(devvp);
	if (minor(dev) >= et_cd.cd_ndevs)
		return(ENXIO);
	sc = et_cd.cd_devs[minor(dev)];
	if (sc->sc_flags & ET_SC_FLAGS_INUSE)
		return(EBUSY);

	vdev_setprivdata(devvp, sc);

	sc->sc_flags |= ET_SC_FLAGS_INUSE;
	return(0);
}

int
etclose(devvp, flags, devtype, p)
	struct vnode *devvp;
	int flags, devtype;
	struct proc *p;
{
	struct et_softc *sc;

	/*
	 * XXX: Should we reset to a default mode?
	 */
	sc = vdev_privdata(devvp);
	sc->sc_flags &= ~ET_SC_FLAGS_INUSE;
	return(0);
}

int
etread(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	return(EINVAL);
}

int
etwrite(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	return(EINVAL);
}

int
etioctl(devvp, cmd, data, flags, p)
	struct vnode *devvp;
	u_long cmd;
	caddr_t data;
	int flags;
	struct proc *p;
{
	struct grfinfo g_display;
	struct et_softc *sc;

	sc = vdev_privdata(devvp);
	switch (cmd) {
	case GRFIOCON:
		return(0);
		break;
	case GRFIOCOFF:
		return(0);
		break;
	case GRFIOCGINFO:
		g_display.gd_fbaddr = (caddr_t) (sc->sc_maddr);
		g_display.gd_fbsize = sc->sc_msize;
		g_display.gd_linbase = FRAME_BASE;
		g_display.gd_regaddr = (caddr_t) (sc->sc_iobase);
		g_display.gd_regsize = sc->sc_iosize;
		g_display.gd_vgaaddr = (caddr_t) (sc->sc_maddr);
		g_display.gd_vgasize = VGA_MAPPABLE;
		g_display.gd_vgabase = VGA_BASE;
		g_display.gd_colors = 16;
		g_display.gd_planes = 4;
		g_display.gd_fbwidth = 640;	/* XXX: should be 'unknown' */
		g_display.gd_fbheight = 400;	/* XXX: should be 'unknown' */
		g_display.gd_fbx = 0;
		g_display.gd_fby = 0;
		g_display.gd_dwidth = 0;
		g_display.gd_dheight = 0;
		g_display.gd_dx = 0;
		g_display.gd_dy = 0;
		g_display.gd_bank_size = 0;
		bcopy((caddr_t)&g_display, data, sizeof(struct grfinfo));
		break;
	case GRFIOCMAP:
		return(EINVAL);
		break;
	case GRFIOCUNMAP:
		return(EINVAL);
		break;
	default:
		return(EINVAL);
		break;
	}
	return(0);
}

paddr_t
etmmap(devvp, offset, prot)
	struct vnode *devvp;
	off_t offset;
	int prot;
{
	struct et_softc *sc;

	sc = vdev_privdata(devvp);

	/* 
	 * control registers
	 * mapped from offset 0x0 to REG_MAPPABLE
	 */
	if (offset >= 0 && offset <= sc->sc_iosize)
		return(m68k_btop(sc->sc_iobase + offset));

	/*
	 * VGA memory
	 * mapped from offset 0xa0000 to 0xc0000
	 */
	if (offset >= VGA_BASE && offset < (VGA_MAPPABLE + VGA_BASE))
		return(m68k_btop(sc->sc_maddr + offset - VGA_BASE));

	/*
	 * frame buffer
	 * mapped from offset 0x400000 to 0x4fffff
	 */
	if (offset >= FRAME_BASE && offset < sc->sc_msize + FRAME_BASE)
		return(m68k_btop(sc->sc_maddr + offset - FRAME_BASE));

	return(-1);
}

int 
eton(dev)
	dev_t dev;
{
	struct et_softc *sc;

	if (minor(dev) >= et_cd.cd_ndevs)
		return(ENXIO);
	sc = et_cd.cd_devs[minor(dev)];
	if (!sc)
		return(ENXIO);
	return(0);
}

int 
etoff(dev)
	dev_t dev;
{
	return(0);
}


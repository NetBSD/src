/*	$NetBSD: et4000.c,v 1.24.12.1 2014/08/20 00:02:49 tls Exp $	*/
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: et4000.c,v 1.24.12.1 2014/08/20 00:02:49 tls Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/event.h>
#include <atari/vme/vmevar.h>

#include <machine/iomap.h>
#include <machine/video.h>
#include <machine/mfp.h>
#include <machine/cpu.h>
#include <atari/atari/device.h>
#include <atari/dev/grfioctl.h>
#include <atari/dev/grf_etreg.h>

#include "ioconf.h"

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

static int	et4k_vme_match(device_t, cfdata_t, void *);
static void	et4k_vme_attach(device_t, device_t, void *);
static int	et4k_probe_addresses(struct vme_attach_args *);
static void	et4k_start(bus_space_tag_t *, bus_space_handle_t *, int *,
		    u_char *);
static void	et4k_stop(bus_space_tag_t *, bus_space_handle_t *, int *,
		    u_char *);
static int	et4k_detect(bus_space_tag_t *, bus_space_tag_t *,
		    bus_space_handle_t *, bus_space_handle_t *, u_int);

int		et4kon(dev_t);
int		et4koff(dev_t);

/* Register and screen memory addresses for ET4000 based VME cards */
static struct et4k_addresses {
	u_long io_addr;
	u_long io_size;
	u_long mem_addr;
	u_long mem_size;
} et4kstd[] = {
	{ 0xfebf0000, REG_MAPPABLE, 0xfec00000, FRAME_MAPPABLE }, /* Crazy Dots VME & II */
	{ 0xfed00000, REG_MAPPABLE, 0xfec00000, FRAME_MAPPABLE }, /* Spektrum I & HC */
	{ 0xfed80000, REG_MAPPABLE, 0xfec00000, FRAME_MAPPABLE }  /* Spektrum TC */
};

#define NET4KSTD (sizeof(et4kstd) / sizeof(et4kstd[0]))

struct grfabs_et4k_priv {
	volatile void *	regkva;
	volatile void *	memkva;
	int			regsz;
	int			memsz;
} et4k_priv;

struct et4k_softc {
	device_t sc_dev;
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

CFATTACH_DECL_NEW(et4k, sizeof(struct et4k_softc),
    et4k_vme_match, et4k_vme_attach, NULL, NULL);

dev_type_open(et4kopen);
dev_type_close(et4kclose);
dev_type_read(et4kread);
dev_type_write(et4kwrite);
dev_type_ioctl(et4kioctl);
dev_type_mmap(et4kmmap);

const struct cdevsw et4k_cdevsw = {
	.d_open = et4kopen,
	.d_close = et4kclose,
	.d_read = et4kread,
	.d_write = et4kwrite,
	.d_ioctl = et4kioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = et4kmmap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

/*
 * Look for a ET4000 (Crazy Dots) card on the VME bus.  We might
 * match Spektrum cards too (untested).
 */
int 
et4k_vme_match(device_t parent, cfdata_t cf, void *aux)
{
	struct vme_attach_args *va = aux;

	return et4k_probe_addresses(va);
}

static int
et4k_probe_addresses(struct vme_attach_args *va)
{
	int i, found = 0;
	bus_space_tag_t iot;
	bus_space_tag_t memt;
	bus_space_handle_t ioh;
	bus_space_handle_t memh;

	iot = va->va_iot;
	memt = va->va_memt;

/* Loop around our possible addresses looking for a match */
	for (i = 0; i < NET4KSTD; i++) {
		struct et4k_addresses *et4k_ap = &et4kstd[i];
		struct vme_attach_args vat = *va;

		if (vat.va_irq != VMECF_IRQ_DEFAULT) {
			printf("%s: config error: no irq support\n", __func__);
			return 0;
		}
		if (vat.va_iobase == VMECF_IOPORT_DEFAULT)
			vat.va_iobase = et4k_ap->io_addr;
		if (vat.va_maddr == VMECF_MEM_DEFAULT)
			vat.va_maddr = et4k_ap->mem_addr;
		if (vat.va_iosize == VMECF_IOSIZE_DEFAULT)
			vat.va_iosize = et4k_ap->io_size;
		if (vat.va_msize == VMECF_MEMSIZ_DEFAULT)
			vat.va_msize = et4k_ap->mem_size;
		if (bus_space_map(iot, vat.va_iobase, vat.va_iosize, 0,
				  &ioh)) {
			printf("%s: cannot map io area\n", __func__);
			return 0;
		}
		if (bus_space_map(memt, vat.va_maddr, vat.va_msize,
			  	  BUS_SPACE_MAP_LINEAR|BUS_SPACE_MAP_CACHEABLE,
			  	  &memh)) {
			bus_space_unmap(iot, ioh, vat.va_iosize);
			printf("%s: cannot map memory area\n", __func__);
			return 0;
		}
		found = et4k_detect(&iot, &memt, &ioh, &memh, vat.va_msize);
		bus_space_unmap(iot, ioh, vat.va_iosize);
		bus_space_unmap(memt, memh, vat.va_msize);
		if (found) {
			*va = vat;
			return 1;
		}
	}
	return 0;
}

static void
et4k_start(bus_space_tag_t *iot, bus_space_handle_t *ioh, int *vgabase, u_char *saved)
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
et4k_stop(bus_space_tag_t *iot, bus_space_handle_t *ioh, int *vgabase, u_char *saved)
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
et4k_detect(bus_space_tag_t *iot, bus_space_tag_t *memt, bus_space_handle_t *ioh, bus_space_handle_t *memh, u_int memsize)
{
	u_char orig, new, saved;
	int vgabase;

	/* Test accessibility of registers and memory */
	if (!bus_space_peek_1(*iot, *ioh, GREG_STATUS1_R))
		return 0;
	if (!bus_space_peek_1(*memt, *memh, 0))
		return 0;

	et4k_start(iot, ioh, &vgabase, &saved);

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
		et4k_stop(iot, ioh, &vgabase, &saved);
		return 0;
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
		et4k_stop(iot, ioh, &vgabase, &saved);
		return 0;
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
		et4k_stop(iot, ioh, &vgabase, &saved);
		return 0;
#endif
	}
	bus_space_write_4(*memt, *memh, memsize - 4, TEST_PATTERN);
	if (bus_space_read_4(*memt, *memh, memsize - 4) != TEST_PATTERN)
	{
#ifdef DEBUG_ET4000
		printf("et4000: Video top write/read failed\n");
#else
		et4k_stop(iot, ioh, &vgabase, &saved);
		return 0;
#endif
	}

	et4k_stop(iot, ioh, &vgabase, &saved);
	return 1;
}

static void
et4k_vme_attach(device_t parent, device_t self, void *aux)
{
	struct et4k_softc *sc = device_private(self);
	struct vme_attach_args *va = aux;
	bus_space_handle_t ioh;
	bus_space_handle_t memh;

	sc->sc_dev = self;

	printf("\n");

	if (bus_space_map(va->va_iot, va->va_iobase, va->va_iosize, 0, &ioh))
		panic("%s: cannot map io area", __func__);
	if (bus_space_map(va->va_memt, va->va_maddr, va->va_msize, 0, &memh))
		panic("%s: cannot map mem area", __func__);

	sc->sc_iot = va->va_iot;
	sc->sc_ioh = ioh;
	sc->sc_memt = va->va_memt;
	sc->sc_memh = memh;
	sc->sc_flags = 0;
	sc->sc_iobase = va->va_iobase;
	sc->sc_maddr = va->va_maddr;
	sc->sc_iosize = va->va_iosize;
	sc->sc_msize = va->va_msize;

	et4k_priv.regkva = (volatile void *)ioh;
	et4k_priv.memkva = (volatile void *)memh;
	et4k_priv.regsz = va->va_iosize;
	et4k_priv.memsz = va->va_msize;
}

int
et4kopen(dev_t dev, int flags, int devtype, struct lwp *l)
{
	struct et4k_softc *sc;

	sc = device_lookup_private(&et4k_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;
	if (sc->sc_flags & ET_SC_FLAGS_INUSE)
		return EBUSY;
	sc->sc_flags |= ET_SC_FLAGS_INUSE;
	return 0;
}

int
et4kclose(dev_t dev, int flags, int devtype, struct lwp *l)
{
	struct et4k_softc *sc;

	/*
	 * XXX: Should we reset to a default mode?
	 */
	sc = device_lookup_private(&et4k_cd, minor(dev));
	sc->sc_flags &= ~ET_SC_FLAGS_INUSE;
	return 0;
}

int
et4kread(dev_t dev, struct uio *uio, int flags)
{

	return EINVAL;
}

int
et4kwrite(dev_t dev, struct uio *uio, int flags)
{

	return EINVAL;
}

int
et4kioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	struct grfinfo g_display;
	struct et4k_softc *sc;

	sc = device_lookup_private(&et4k_cd, minor(dev));
	switch (cmd) {
	case GRFIOCON:
		return 0;
		break;
	case GRFIOCOFF:
		return 0;
		break;
	case GRFIOCGINFO:
		g_display.gd_fbaddr = (void *) (sc->sc_maddr);
		g_display.gd_fbsize = sc->sc_msize;
		g_display.gd_linbase = FRAME_BASE;
		g_display.gd_regaddr = (void *) (sc->sc_iobase);
		g_display.gd_regsize = sc->sc_iosize;
		g_display.gd_vgaaddr = (void *) (sc->sc_maddr);
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
		memcpy(data, (void *)&g_display, sizeof(struct grfinfo));
		break;
	case GRFIOCMAP:
		return EINVAL;
		break;
	case GRFIOCUNMAP:
		return EINVAL;
		break;
	default:
		return EINVAL;
		break;
	}
	return 0;
}

paddr_t
et4kmmap(dev_t dev, off_t offset, int prot)
{
	struct et4k_softc *sc;

	sc = device_lookup_private(&et4k_cd, minor(dev));

	/* 
	 * control registers
	 * mapped from offset 0x0 to REG_MAPPABLE
	 */
	if (offset >= 0 && offset <= sc->sc_iosize)
		return m68k_btop(sc->sc_iobase + offset);

	/*
	 * VGA memory
	 * mapped from offset 0xa0000 to 0xc0000
	 */
	if (offset >= VGA_BASE && offset < (VGA_MAPPABLE + VGA_BASE))
		return m68k_btop(sc->sc_maddr + offset - VGA_BASE);

	/*
	 * frame buffer
	 * mapped from offset 0x400000 to 0x4fffff
	 */
	if (offset >= FRAME_BASE && offset < sc->sc_msize + FRAME_BASE)
		return m68k_btop(sc->sc_maddr + offset - FRAME_BASE);

	return -1;
}

int 
et4kon(dev_t dev)
{
	struct et4k_softc *sc;

	if (minor(dev) >= et4k_cd.cd_ndevs)
		return ENXIO;
	sc = device_lookup_private(&et4k_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;
	return 0;
}

int 
et4koff(dev_t dev)
{

	return 0;
}


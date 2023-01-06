/*	$NetBSD: leo.c,v 1.24 2023/01/06 10:28:28 tsutsui Exp $	*/

/*-
 * Copyright (c) 1997 maximum entropy <entropy@zippy.bernstein.com>
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 * Driver for the Circad Leonardo 1.2 from Lexicor, a 24-bit true color
 * VME graphics card based on the Texas Instruments TMS34061.
 *
 * Written by maximum entropy <entropy@zippy.bernstein.com>, December 5, 1997.
 *
 * This driver was written from scratch, but I referred to several other
 * drivers in the NetBSD distribution as examples.  The file I referred to
 * the most was /sys/arch/atari/vme/if_le_vme.c.  Due credits:
 * Copyright (c) 1997 Leo Weppelman.  All rights reserved.
 * Copyright (c) 1995 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * This code is derived from software contributed to Berkeley by
 *	Ralph Campbell and Rick Macklem.
 * This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: leo.c,v 1.24 2023/01/06 10:28:28 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <machine/cpu.h>
#include <sys/bus.h>
#include <machine/iomap.h>
#include <machine/scu.h>
#include <atari/vme/vmevar.h>
#include <atari/vme/leovar.h>
#include <atari/vme/leoioctl.h>

#include "ioconf.h"

static struct leo_addresses {
	u_long reg_addr;
	u_int reg_size;
	u_long mem_addr;
	u_int mem_size;
} leostd[] = {
	{ 0xfed90000, 0x100, 0xfec00000, 0x100000 }
};

#define NLEOSTD (sizeof(leostd) / sizeof(leostd[0]))

struct leo_softc {
	device_t sc_dev;		/* XXX what goes here? */
	bus_space_tag_t sc_iot;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_ioh;
	bus_space_handle_t sc_memh;
	int sc_flags;
	int sc_maddr;
	u_int sc_msize;
};

#define LEO_SC_FLAGS_INUSE 1

static int leo_match(device_t, cfdata_t, void *);
static void leo_attach(device_t, device_t, void *);
static int leo_probe(bus_space_tag_t *, bus_space_tag_t *,
			  bus_space_handle_t *, bus_space_handle_t *,
			  u_int, u_int);
static int leo_init(struct leo_softc *, int);
static int leo_scroll(struct leo_softc *, int);

CFATTACH_DECL_NEW(leo, sizeof(struct leo_softc),
    leo_match, leo_attach, NULL, NULL);

static dev_type_open(leoopen);
static dev_type_close(leoclose);
static dev_type_read(leomove);
static dev_type_ioctl(leoioctl);
static dev_type_mmap(leommap);

const struct cdevsw leo_cdevsw = {
	.d_open = leoopen,
	.d_close = leoclose,
	.d_read = leomove,
	.d_write = leomove,
	.d_ioctl = leoioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = leommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

static int
leo_match(device_t parent, cfdata_t cf, void *aux)
{
	struct vme_attach_args *va = aux;
	int i;
	bus_space_tag_t iot;
	bus_space_tag_t memt;
	bus_space_handle_t ioh;
	bus_space_handle_t memh;

	/*
	 * We are passed our configuration in the attachment arguments.
	 * The configuration information may be partially unspecified.
	 * For any unspecified configuration parameters, we fill in those
	 * parameters with data for a "standard" configuration.
	 * Once we have a fully specified configuration, we try to probe
	 * a card with that configuration.
	 * The Leonardo only has one configuration and it isn't likely
	 * to change, but this routine doesn't assume that's the case.
	 */
	iot = va->va_iot;
	memt = va->va_memt;
	for (i = 0; i < NLEOSTD; i++) {
		struct leo_addresses *leo_ap = &leostd[i];
		int found = 0;
		struct vme_attach_args vat = *va;

		if (vat.va_irq != VMECF_IRQ_DEFAULT) {
			printf("leo_match: config error: no irq support\n");
			return 0;
		}
		if (vat.va_iobase == VMECF_IOPORT_DEFAULT)
			vat.va_iobase = leo_ap->reg_addr;
		if (vat.va_maddr == VMECF_MEM_DEFAULT)
			vat.va_maddr = leo_ap->mem_addr;
		if (vat.va_iosize == VMECF_IOSIZE_DEFAULT)
			vat.va_iosize = leo_ap->reg_size;
		if (vat.va_msize == VMECF_MEMSIZ_DEFAULT)
			vat.va_msize = leo_ap->mem_size;
		if (bus_space_map(iot, vat.va_iobase, vat.va_iosize, 0, &ioh)) {
			printf("leo_match: cannot map io area\n");
			return 0;
		}
		if (bus_space_map(memt, vat.va_maddr, vat.va_msize,
				  BUS_SPACE_MAP_LINEAR|BUS_SPACE_MAP_CACHEABLE,
				  &memh)) {
			bus_space_unmap(iot, ioh, vat.va_iosize);
			printf("leo_match: cannot map memory area\n");
			return 0;
		}
		found = leo_probe(&iot, &memt, &ioh, &memh,
				  vat.va_iosize, vat.va_msize);
		bus_space_unmap(iot, ioh, vat.va_iosize);
		bus_space_unmap(memt, memh, vat.va_msize);
		if (found) {
			*va = vat;
			return 1;
		}
	}
	return 0;
}

static int
leo_probe(bus_space_tag_t *iot, bus_space_tag_t *memt, bus_space_handle_t *ioh, bus_space_handle_t *memh, u_int iosize, u_int msize)
{

	/* Test that our highest register is within the io range. */
	if (0xca > iosize) /* XXX */
		return 0;
	/* Test if we can peek each register. */
	if (!bus_space_peek_1(*iot, *ioh, LEO_REG_MSBSCROLL))
		return 0;
	if (!bus_space_peek_1(*iot, *ioh, LEO_REG_LSBSCROLL))
		return 0;
	/*
	 * Write a test pattern at the start and end of the memory region,
	 * and test if the pattern can be read back.  If so, the region is
	 * backed by memory (i.e. the card is present).
	 * On the Leonardo, the first byte of each longword isn't backed by
	 * physical memory, so we only compare the three low-order bytes
	 * with the test pattern.
	 */
	bus_space_write_4(*memt, *memh, 0, 0xa5a5a5a5);
	if ((bus_space_read_4(*memt, *memh, 0) & 0xffffff) != 0xa5a5a5)
		return 0;
	bus_space_write_4(*memt, *memh, msize - 4, 0xa5a5a5a5);
	if ((bus_space_read_4(*memt, *memh, msize - 4) & 0xffffff)
		!= 0xa5a5a5)
		return 0;
	return 1;
}

static void
leo_attach(device_t parent, device_t self, void *aux)
{
	struct leo_softc *sc = device_private(self);
	struct vme_attach_args *va = aux;
	bus_space_handle_t ioh;
	bus_space_handle_t memh;
#ifndef SET_REGION
	int i;
#endif

	sc->sc_dev = self;

	printf("\n");
	if (bus_space_map(va->va_iot, va->va_iobase, va->va_iosize, 0, &ioh))
		panic("leo_attach: cannot map io area");
	if (bus_space_map(va->va_memt, va->va_maddr, va->va_msize,
			  BUS_SPACE_MAP_LINEAR|BUS_SPACE_MAP_CACHEABLE, &memh))
		panic("leo_attach: cannot map memory area");
#ifdef SET_REGION /* XXX seems to be unimplemented on atari? */
	bus_space_set_region_4(va->va_memt, memh, 0, 0, va->va_msize >> 2);
#else
	for (i = 0; i < (va->va_msize >> 2); i++)
		bus_space_write_4(va->va_memt, memh, i << 2, 0);
#endif
	sc->sc_iot = va->va_iot;
	sc->sc_ioh = ioh;
	sc->sc_memt = va->va_memt;
	sc->sc_memh = memh;
	sc->sc_flags = 0;
	sc->sc_maddr = va->va_maddr;
	sc->sc_msize = va->va_msize;
	leo_init(sc, 512);
	leo_scroll(sc, 0);
}

int
leoopen(dev_t dev, int flags, int devtype, struct lwp *l)
{
	struct leo_softc *sc;
	int r;

	sc = device_lookup_private(&leo_cd, minor(dev));
	if (!sc)
		return ENXIO;
	if (sc->sc_flags & LEO_SC_FLAGS_INUSE)
		return EBUSY;
	r = leo_init(sc, 512);
	if (r != 0)
		return r;
	r = leo_scroll(sc, 0);
	if (r != 0)
		return r;
	sc->sc_flags |= LEO_SC_FLAGS_INUSE;
	return 0;
}

static int
leo_init(struct leo_softc *sc, int ysize)
{

	if ((ysize != 256) && (ysize != 384) && (ysize != 512))
		return EINVAL;
	/* XXX */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x00, 0x6);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x08, 0x0);
	if (ysize == 384)
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x10, 0x10);
	else
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x10, 0x11);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x18, 0x0);
	if (ysize == 384)
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x20, 0x50);
	else
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x20, 0x51);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x28, 0x0);
	if (ysize == 384)
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x30, 0x56);
	else
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x30, 0x57);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x38, 0x0);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x40, 0x6);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x48, 0x0);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x50, 0x25);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x58, 0x0);
	if (ysize == 256) {
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x60, 0x1f);
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x68, 0x1);
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x70, 0x29);
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x78, 0x1);
	} else if (ysize == 384) {
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x60, 0xa5);
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x68, 0x1);
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x70, 0xa7);
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x78, 0x1);
	} else {
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x60, 0x1d);
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x68, 0x2);
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x70, 0x27);
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x78, 0x2);
	}
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0xb8, 0x10);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0xb0, 0x10);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0x80, 0x4);
	if (ysize == 384)
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0xc8, 0x21);
	else
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0xc8, 0x20);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0xc0, 0x40);
	return 0;
}

static int
leo_scroll(struct leo_softc *sc, int scroll)
{

	if ((scroll < 0) || (scroll > 255))
		return EINVAL;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LEO_REG_MSBSCROLL,
			  (scroll >> 6) & 0xff);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LEO_REG_LSBSCROLL,
			  (scroll << 2) & 0xff);
	return 0;
}

int
leoclose(dev_t dev, int flags, int devtype, struct lwp *l)
{
	struct leo_softc *sc;

	sc = device_lookup_private(&leo_cd, minor(dev));
	sc->sc_flags &= ~LEO_SC_FLAGS_INUSE;
	return 0;
}

#define SMALLBSIZE      32

int
leomove(dev_t dev, struct uio *uio, int flags)
{
	struct leo_softc *sc;
	int length, size, error;
	u_int8_t smallbuf[SMALLBSIZE];
	off_t offset;

	sc = device_lookup_private(&leo_cd,minor(dev));
	if (uio->uio_offset > sc->sc_msize)
		return 0;
	length = sc->sc_msize - uio->uio_offset;
	if (length > uio->uio_resid)
		length = uio->uio_resid;
	while (length > 0) {
		size = length;
		if (size > SMALLBSIZE)
			size = SMALLBSIZE;
		length -= size;
		offset = uio->uio_offset;
		if (uio->uio_rw == UIO_READ)
			bus_space_read_region_1(sc->sc_memt, sc->sc_memh,
			    offset, smallbuf, size);
		if ((error = uiomove((void *)smallbuf, size, uio)))
			return (error);
		if (uio->uio_rw == UIO_WRITE)
			bus_space_write_region_1(sc->sc_memt, sc->sc_memh,
			    offset, smallbuf, size);
	}
	return 0;
}

int
leoioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	struct leo_softc *sc;

	sc = device_lookup_private(&leo_cd,minor(dev));
	switch (cmd) {
	case LIOCYRES:
		return leo_init(sc, *(int *)data);
		break;
	case LIOCSCRL:
		return leo_scroll(sc, *(int *)data);
		break;
	default:
		return EINVAL;
		break;
	}
}

paddr_t
leommap(dev_t dev, off_t offset, int prot)
{
	struct leo_softc *sc;

	sc = device_lookup_private(&leo_cd, minor(dev));
	if (offset >= 0 && offset < sc->sc_msize)
		return m68k_btop(sc->sc_maddr + offset);
	return -1;
}

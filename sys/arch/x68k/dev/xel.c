/*	$NetBSD: xel.c,v 1.1.2.2 1999/01/31 17:10:24 minoura Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Minoura Makoto.
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
 * TSR Xellent30 driver.
 * Detect Xellent30, and reserve its I/O area.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <arch/x68k/dev/intiovar.h>

static paddr_t xel_addr __P((struct device *, struct cfdata *,
			     struct intio_attach_args *));
static int xel_probe __P((paddr_t));
static int xel_match __P((struct device *, struct cfdata *, void *));
static void xel_attach __P((struct device *, struct device *, void *));

struct xel_softc {
	struct device dev;

	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bh;
};

struct cfattach xel_ca = {
	sizeof (struct xel_softc), xel_match, xel_attach
};

static paddr_t xel_addrs[] = { 0xec0000, 0xec4000, 0xec8000, 0xecc000 };

#define XEL_MODE_RAM_LOWER	0x00
#define XEL_MODE_RAM_HIGHER	0x01
#define XEL_MODE_UNMAP_RAM	0x00
#define XEL_MODE_MAP_RAM	0x02
#define XEL_MODE_MPU_000	0x00
#define XEL_MODE_MPU_030	0x04

#define XEL_RAM_ADDR_LOWER	0xbc0000
#define XEL_RAM_ADDR_HIGHER	0xfc0000


static paddr_t
xel_addr (parent, match, ia)
	struct device *parent;
	struct cfdata *match;
	struct intio_attach_args *ia;
{
	paddr_t addr = 0;

	if (match->cf_addr == INTIOCF_ADDR_DEFAULT) {
		int i;

		for (i=0; i<sizeof(xel_addrs)/sizeof(xel_addrs[0]); i++) {
			if (xel_probe(xel_addrs[i])) {
				addr = xel_addrs[i];
				break;
			}
		}
	} else {
		if (xel_probe((paddr_t) match->cf_addr))
			addr = (paddr_t) match->cf_addr;
	}

	if (addr) {
		/* found! */
		ia->ia_addr = (int) addr;
		ia->ia_size = 0x4000;
		if (intio_map_allocate_region (parent, ia, INTIO_MAP_TESTONLY)
		    < 0)
			return 0;
		else
			return addr;
	}

	return 0;
}

extern int *nofault;

static int
xel_probe(addr)
	paddr_t addr;
{
	u_int32_t b1, b2;
	u_int16_t *start = (void*) INTIO_ADDR(addr);
	label_t	faultbuf;
	volatile u_int32_t *sram = (void*) INTIO_ADDR(XEL_RAM_ADDR_HIGHER);

	if (badaddr(start))
		return 0;

	nofault = (int *) &faultbuf;
	if (setjmp(&faultbuf)) {
		nofault = (int *) 0;
		return 0;
	}

	b1 = sram[0];
	b2 = sram[1];
	/* Try to map the Xellent local memory. */
	start[0] = XEL_MODE_RAM_HIGHER | XEL_MODE_MAP_RAM | XEL_MODE_MPU_030;

#if 0
	/* the contents should be deferent. */
	if (b1 == sram[0] && b2 == sram[1]) {
		nofault = (int *) 0;
		return 0;
	}
#else
	/* Try to write to the local memory. */
	sram[0] = 0x55555555;
	sram[1] = 0xaaaaaaaa;
	if (sram[0] != 0x55555555 || sram[1] != 0xaaaaaaaa) {
		sram[0] = b1;
		sram[1] = b2;
		nofault = (int *) 0;
		return 0;
	}
	sram[0] = 0xaaaaaaaa;
	sram[1] = 0x55555555;
	if (sram[0] != 0xaaaaaaaa || sram[1] != 0x55555555) {
		sram[0] = b1;
		sram[1] = b2;
		nofault = (int *) 0;
		return 0;
	}
	sram[0] = b1;
	sram[1] = b2;
#endif

	/* Unmap. */
	start[0] = XEL_MODE_UNMAP_RAM | XEL_MODE_MPU_030;

	nofault = (int *) 0;
	return 1;
}

static int
xel_match (parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct intio_attach_args *ia = aux;

	if (strcmp (ia->ia_name, "xel") != 0)
		return 0;

	if (xel_addr(parent, match, ia)) {
#ifdef DIAGNOSTIC
		if (cputype != CPU_68030)
			panic ("Non-030 Xellent???");
#endif
		return 1;
	}
	return 0;
}

static void
xel_attach (parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct xel_softc *sc = (void*)self;
	struct intio_attach_args *ia = aux;
	struct cfdata *cf = self->dv_cfdata;
	paddr_t addr;
	int r;

	addr = xel_addr(parent, cf, aux);
	sc->sc_bst = ia->ia_bst;
	ia->ia_addr = (int) addr;
	ia->ia_size = 0x4000;
	r = intio_map_allocate_region (parent, ia, INTIO_MAP_ALLOCATE);
#ifdef DIAGNOSTIC
	if (r)
		panic ("IO map for Xellent30 corruption??");
#endif
	printf (": Xellent30 MPU Accelerator.\n");

	return;
}

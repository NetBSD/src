/*	$NetBSD: p5membar.c,v 1.2.4.2 2012/02/18 07:31:19 mrg Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * Handle autoconfigured PCI resources on some revisions of CyberVision PPC,
 * BlizzardVision PPC graphics cards and all G-REX PCI busboards. 
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/extent.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <m68k/bus_dma.h>
#include <amiga/dev/zbusvar.h>
#include <amiga/pci/p5pbreg.h>
#include <amiga/pci/p5pbvar.h>
#include <amiga/pci/p5membarvar.h>

/* Zorro IDs */
#define ZORRO_MANID_P5		8512
#define ZORRO_PRODID_P5PB	101		/* CVPPC/BVPPC/G-REX */

static int	p5membar_match(struct device *, struct cfdata *, void *);
static void	p5membar_attach(struct device *, struct device *, void *);

CFATTACH_DECL_NEW(p5membar, sizeof(struct p5membar_softc),
    p5membar_match, p5membar_attach, NULL, NULL);

static int
p5membar_match(device_t parent, cfdata_t cf, void *aux)
{
	struct zbus_args *zap;

	zap = aux;

	if (zap->manid != ZORRO_MANID_P5)
		return 0;

	if (zap->prodid != ZORRO_PRODID_P5PB)
		return 0;

	return 1;
}

static void
p5membar_attach(device_t parent, device_t self, void *aux)
{
	struct zbus_args *zap;
	struct p5membar_softc *sc;

	sc = device_private(self); 
	zap = aux;

	sc->sc_dev = self;
	sc->sc_base = zap->pa;
	sc->sc_size = zap->size;

	if ((bus_addr_t) zap->pa == P5BUS_PCI_CONF_BASE) {
		aprint_normal(": PCI config area, %d kB\n",
		    zap->size / 1024);
		sc->sc_type = P5MEMBAR_TYPE_INTERNAL;
	} else if ((bus_addr_t) zap->pa == P5BUS_PCI_IO_BASE) {
		aprint_normal(": PCI I/O area, %d kB\n",
		    zap->size / 1024);
		sc->sc_type = P5MEMBAR_TYPE_INTERNAL;
	} else if ((bus_addr_t) zap->pa == P5BUS_BRIDGE_BASE) {
		aprint_normal(": PCI bridge, %d kB\n",
		    zap->size / 1024);
		sc->sc_type = P5MEMBAR_TYPE_INTERNAL;
	} else {
		aprint_normal(": PCI memory BAR, %d kB\n",
		    zap->size / 1024);
		sc->sc_type = P5MEMBAR_TYPE_MEMORY;
	}

	/*
	 * Do nothing here, p5pb should find the p5membar devices
	 * and do the right(tm) thing. 
	 */

}


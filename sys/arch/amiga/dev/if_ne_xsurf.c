/*	$NetBSD: if_ne_xsurf.c,v 1.1.2.2 2012/05/23 10:07:40 yamt Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Bernd Ernesti.
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

#include <sys/cdefs.h>

/*
 * X-Surf driver, ne(4) attachment. 
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/bus.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <dev/ic/dp8390reg.h>
#include <dev/ic/dp8390var.h>

#include <dev/ic/ne2000reg.h>
#include <dev/ic/ne2000var.h>

#include <dev/ic/rtl80x9reg.h>
#include <dev/ic/rtl80x9var.h>

#include <amiga/amiga/device.h>
#include <amiga/amiga/isr.h>

#include <amiga/dev/xsurfvar.h>
#include <amiga/dev/zbusvar.h>

int	ne_xsurf_match(device_t, cfdata_t , void *);
void	ne_xsurf_attach(device_t, device_t, void *);

struct ne_xsurf_softc {
	struct ne2000_softc	sc_ne2000;
	struct bus_space_tag	sc_bst;
	struct isr		sc_isr;
};

CFATTACH_DECL_NEW(ne_xsurf, sizeof(struct ne_xsurf_softc),
    ne_xsurf_match, ne_xsurf_attach, NULL, NULL);

/*
 * The Amiga address are shifted by one bit to the ISA-Bus, but
 * this is handled by the bus_space functions.
 */
#define	NE_XSURF_NPORTS		0x20
#define	NE_XSURF_NICBASE	0x0300	/* 0x0600 */
#define	NE_XSURF_NICSIZE	0x10
#define	NE_XSURF_ASICBASE	0x0310	/* 0x0620 */
#define	NE_XSURF_ASICSIZE	0x10

#define XSURF_NE_OFFSET		0x8000

/*
 * Clockport offsets.
 */
#define XSURF_CP1_BASE		0xA001
#define XSURF_CP2_BASE		0xC000

/*
 * E3B Deneb firmware v11 creates fake X-Surf autoconfig entry.
 * Do not attach ne driver to this fake card, otherwise kernel panic
 * may occur.
 */
#define DENEB_XSURF_SERNO	0xC0FFEE01	/* Serial of the fake card */

int
ne_xsurf_match(device_t parent, cfdata_t cf, void *aux)
{
	struct xsurfbus_attach_args *xap = aux;

	if (strcmp(xap->xaa_name, "ne_xsurf") != 0)
		return 0;

	return 1;
}

/*
 * Install interface into kernel networking data structures.
 */
void
ne_xsurf_attach(device_t parent, device_t self, void *aux)
{
	struct ne_xsurf_softc *zsc = device_private(self);
	struct ne2000_softc *nsc = &zsc->sc_ne2000;
	struct dp8390_softc *dsc = &nsc->sc_dp8390;

	struct xsurfbus_attach_args *xap = aux;

	bus_space_tag_t nict = &zsc->sc_bst;
	bus_space_handle_t nich;
	bus_space_tag_t asict = nict;
	bus_space_handle_t asich;

	dsc->sc_dev = self;
	dsc->sc_mediachange = rtl80x9_mediachange;
	dsc->sc_mediastatus = rtl80x9_mediastatus;
	dsc->init_card = rtl80x9_init_card;
	dsc->sc_media_init = rtl80x9_media_init;

	zsc->sc_bst.base = xap->xaa_base;

	zsc->sc_bst.absm = &amiga_bus_stride_2;

	aprint_normal("\n");

	/* Map i/o space. */
	if (bus_space_map(nict, NE_XSURF_NICBASE, 
	    NE_XSURF_NPORTS, 0, &nich)) {
		aprint_error_dev(self, "can't map nic i/o space\n");
		return;
	}

	if (bus_space_subregion(nict, nich, NE2000_ASIC_OFFSET, 
	    NE_XSURF_ASICSIZE, &asich)) {
		aprint_error_dev(self, "can't map asic i/o space\n");
		return;
	}

	dsc->sc_regt = nict;
	dsc->sc_regh = nich;

	nsc->sc_asict = asict;
	nsc->sc_asich = asich;

	/* This interface is always enabled. */
	dsc->sc_enabled = 1;

	/*
	 * Do generic NE2000 attach.  This will read the station address
	 * from the EEPROM.
	 */
	ne2000_attach(nsc, NULL);

	zsc->sc_isr.isr_intr = dp8390_intr;
	zsc->sc_isr.isr_arg = dsc;
	zsc->sc_isr.isr_ipl = 2;
	add_isr(&zsc->sc_isr);
}

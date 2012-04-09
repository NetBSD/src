/*	$NetBSD: if_ne_zbus.c,v 1.15 2012/04/09 19:55:00 rkujawa Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: if_ne_zbus.c,v 1.15 2012/04/09 19:55:00 rkujawa Exp $");

/*
 * Thanks to Village Tronic for giving me a card.
 * Bernd Ernesti
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

#include <amiga/dev/zbusvar.h>

int	ne_zbus_match(device_t, cfdata_t , void *);
void	ne_zbus_attach(device_t, device_t, void *);

struct ne_zbus_softc {
	struct ne2000_softc	sc_ne2000;
	struct bus_space_tag	sc_bst;
	struct isr		sc_isr;
};

CFATTACH_DECL_NEW(ne_zbus, sizeof(struct ne_zbus_softc),
    ne_zbus_match, ne_zbus_attach, NULL, NULL);

/*
 * The Amiga address are shifted by one bit to the ISA-Bus, but
 * this is handled by the bus_space functions.
 */
#define	NE_ARIADNE_II_NPORTS	0x20
#define	NE_ARIADNE_II_NICBASE	0x0300	/* 0x0600 */
#define	NE_ARIADNE_II_NICSIZE	0x10
#define	NE_ARIADNE_II_ASICBASE	0x0310	/* 0x0620 */
#define	NE_ARIADNE_II_ASICSIZE	0x10

/*
 * E3B Deneb firmware v11 creates fake X-Surf autoconfig entry.
 * Do not attach ne driver to this fake card, otherwise kernel panic
 * may occur.
 */
#define DENEB_XSURF_SERNO	0xC0FFEE01	/* Serial of the fake card */

int
ne_zbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct zbus_args *zap = aux;

	/* Ariadne II ethernet card */
	if (zap->manid == 2167 && zap->prodid == 202)
		return (1);

	/* X-surf ethernet card */
	if (zap->manid == 4626 && zap->prodid == 23) {
		if (zap->serno != DENEB_XSURF_SERNO)
			return (1);
	}

	return (0);
}

/*
 * Install interface into kernel networking data structures
 */
void
ne_zbus_attach(device_t parent, device_t self, void *aux)
{
	struct ne_zbus_softc *zsc = device_private(self);
	struct ne2000_softc *nsc = &zsc->sc_ne2000;
	struct dp8390_softc *dsc = &nsc->sc_dp8390;
	struct zbus_args *zap = aux;
	bus_space_tag_t nict = &zsc->sc_bst;
	bus_space_handle_t nich;
	bus_space_tag_t asict = nict;
	bus_space_handle_t asich;

	dsc->sc_dev = self;
	dsc->sc_mediachange = rtl80x9_mediachange;
	dsc->sc_mediastatus = rtl80x9_mediastatus;
	dsc->init_card = rtl80x9_init_card;
	dsc->sc_media_init = rtl80x9_media_init;

	zsc->sc_bst.base = (u_long)zap->va + 0;
	if (zap->manid == 4626)
		 zsc->sc_bst.base += 0x8000;

	zsc->sc_bst.absm = &amiga_bus_stride_2;

	aprint_normal("\n");

	/* Map i/o space. */
	if (bus_space_map(nict, NE_ARIADNE_II_NICBASE, NE_ARIADNE_II_NPORTS, 0, &nich)) {
		aprint_error_dev(self, "can't map nic i/o space\n");
		return;
	}

	if (bus_space_subregion(nict, nich, NE2000_ASIC_OFFSET, NE_ARIADNE_II_ASICSIZE,
	    &asich)) {
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

/*	$NetBSD: if_ne_mainbus.c,v 1.1.8.2 2011/06/06 09:06:13 jruoho Exp $	*/
/*
 * Copyright (c) 2010 KIYOHARA Takashi
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ne_mainbus.c,v 1.1.8.2 2011/06/06 09:06:13 jruoho Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <machine/autoconf.h>
#include <machine/intr.h>
#include <machine/mmeye.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/ic/dp8390reg.h>
#include <dev/ic/dp8390var.h>

#include <dev/ic/ne2000reg.h>
#include <dev/ic/ne2000var.h>

#include "locators.h"

static int ne_mainbus_match(device_t, cfdata_t , void *);
static void ne_mainbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ne_mainbus, sizeof(struct ne2000_softc),
    ne_mainbus_match, ne_mainbus_attach, NULL, NULL);

static int
ne_mainbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	bus_space_tag_t nict, asict;
	bus_space_handle_t nich, asich;
	int rv = 0;

	if (strcmp(ma->ma_name, match->cf_name) != 0)
		return 0;

	/* Disallow wildcarded values. */
	if (ma->ma_addr1 == MAINBUSCF_ADDR1_DEFAULT ||
	    ma->ma_irq1 == MAINBUSCF_IRQ1_DEFAULT)
		return 0;

	/* Make sure this is a valid NE[12]000 i/o address. */
	if ((ma->ma_addr1 & 0x1f) != 0)
		return 0;

	/* Map i/o space. */
	nict = SH3_BUS_SPACE_PCMCIA_IO;
	if (bus_space_map(nict, ma->ma_addr1, NE2000_NPORTS, 0, &nich))
		return 0;

	if (bus_space_subregion(nict, nich, NE2000_ASIC_OFFSET,
	    NE2000_ASIC_NPORTS, &asich))
		goto out;
	asict = nict;

	/* Look for an NE2000-compatible card. */
	rv = ne2000_detect(nict, nich, asict, asich);

 out:
	bus_space_unmap(nict, nich, NE2000_NPORTS);
	return rv;
}

void
ne_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct ne2000_softc *sc = device_private(self);
	struct dp8390_softc *dsc = &sc->sc_dp8390;
	struct mainbus_attach_args *ma = aux;
	bus_space_tag_t nict, asict;
	bus_space_handle_t nich, asich;
	int netype;
	const char *typestr;
	void *ih;

	dsc->sc_dev = self;
	aprint_normal("\n");

	/* Map i/o space. */
	nict = SH3_BUS_SPACE_PCMCIA_IO;
	if (bus_space_map(nict, ma->ma_addr1, NE2000_NPORTS, 0, &nich)) {
		aprint_error_dev(self, "can't map i/o space\n");
		return;
	}

	if (bus_space_subregion(nict, nich, NE2000_ASIC_OFFSET,
	    NE2000_ASIC_NPORTS, &asich)) {
		aprint_error_dev(self, "can't subregion i/o space\n");
		return;
	}
	asict = nict;

	dsc->sc_regt = nict;
	dsc->sc_regh = nich;

	sc->sc_asict = asict;
	sc->sc_asich = asich;

	/*
	 * Detect it again, so we can print some information about the
	 * interface.
	 */
	netype = ne2000_detect(nict, nich, asict, asich);
	switch (netype) {
	case NE2000_TYPE_RTL8019:
		typestr = "NE2000 (RTL8019)";
		break;

	case NE2000_TYPE_NE1000:
	case NE2000_TYPE_NE2000:
	default:
		aprint_error_dev(self, "where did the card go?!\n");
		return;
	}

	aprint_normal_dev(self, "%s Ethernet\n", typestr);

	/* This interface is always enabled. */
	dsc->sc_enabled = 1;

	/*
	 * Do generic NE2000 attach.  This will read the station address
	 * from the EEPROM.
	 */
	ne2000_attach(sc, NULL);

	/* Establish the interrupt handler. */
	ih = mmeye_intr_establish(ma->ma_irq1, IST_LEVEL, IPL_NET,
	    dp8390_intr, dsc);
	if (ih == NULL)
		aprint_error_dev(self,
		    "couldn't establish interrupt handler\n");
}

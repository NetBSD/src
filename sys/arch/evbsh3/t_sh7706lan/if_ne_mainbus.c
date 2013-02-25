/*	$NetBSD: if_ne_mainbus.c,v 1.2.12.1 2013/02/25 00:28:40 tls Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
__KERNEL_RCSID(0, "$NetBSD: if_ne_mainbus.c,v 1.2.12.1 2013/02/25 00:28:40 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <sys/intr.h>

#include <dev/ic/dp8390reg.h>
#include <dev/ic/dp8390var.h>

#include <dev/ic/ne2000reg.h>
#include <dev/ic/ne2000var.h>

#include <machine/autoconf.h>

#include <sh3/exception.h>

static int ne_mainbus_match(device_t, cfdata_t, void *);
static void ne_mainbus_attach(device_t, device_t, void *);

struct ne_mainbus_softc {
	struct	ne2000_softc sc_ne2000;		/* real "ne2000" softc */

	void	*sc_ih;				/* interrupt cookie */
};

CFATTACH_DECL_NEW(ne_mainbus, sizeof(struct ne_mainbus_softc),
    ne_mainbus_match, ne_mainbus_attach, NULL, NULL);

extern struct _bus_space t_sh7706lan_bus_io;

static int
ne_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *maa = (struct mainbus_attach_args *)aux;
	bus_space_tag_t nict = &t_sh7706lan_bus_io;
	bus_space_handle_t nich;
	bus_space_tag_t asict;
	bus_space_handle_t asich;
	int rv = 0;

	if (strcmp(maa->ma_name, "ne") != 0)
		return 0;

	/* Map i/o space. */
	if (bus_space_map(nict, 0x10000300, NE2000_NPORTS, 0, &nich))
		return 0;

	asict = nict;
	if (bus_space_subregion(nict, nich, NE2000_ASIC_OFFSET,
	    NE2000_ASIC_NPORTS, &asich))
		goto out;

	/* Look for an NE2000-compatible card. */
	rv = ne2000_detect(nict, nich, asict, asich);

 out:
	bus_space_unmap(nict, nich, NE2000_NPORTS);
	return rv;
}

static void
ne_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct ne_mainbus_softc *msc = device_private(self);
	struct ne2000_softc *nsc = &msc->sc_ne2000;
	struct dp8390_softc *dsc = &nsc->sc_dp8390;
	bus_space_tag_t nict = &t_sh7706lan_bus_io;
	bus_space_handle_t nich;
	bus_space_tag_t asict = nict;
	bus_space_handle_t asich;
	const char *typestr;
	int netype;

	dsc->sc_dev = self;

	aprint_naive("\n");
	aprint_normal("\n");

	/* Map i/o space. */
	if (bus_space_map(nict, 0x10000300, NE2000_NPORTS, 0, &nich)){
		aprint_error_dev(self, "can't map i/o space\n");
		return;
	}

	if (bus_space_subregion(nict, nich, NE2000_ASIC_OFFSET,
	    NE2000_ASIC_NPORTS, &asich)) {
		aprint_error_dev(self, "can't subregion i/o space\n");
		bus_space_unmap(nict, nich, NE2000_NPORTS);
		return;
	}

	dsc->sc_regt = nict;
	dsc->sc_regh = nich;

	nsc->sc_asict = asict;
	nsc->sc_asich = asich;

	/*
	 * Detect it again, so we can print some information about the
	 * interface.
	 */
	netype = ne2000_detect(nict, nich, asict, asich);
	switch (netype) {
	case NE2000_TYPE_NE1000:
		typestr = "NE1000";
		break;

	case NE2000_TYPE_NE2000:
		typestr = "NE2000";
		break;

	case NE2000_TYPE_RTL8019:
		typestr = "NE2000 (RTL8019)";
 		break;

	default:
		aprint_error_dev(self, "where did the card go?!\n");
		bus_space_unmap(nict, nich, NE2000_NPORTS);
		return;
	}

	aprint_normal_dev(self, "%s Ethernet\n", typestr);

	/* This interface is always enabled. */
	dsc->sc_enabled = 1;

	/* force 8bit */
	nsc->sc_quirk |= NE2000_QUIRK_8BIT;

	/*
	 * Do generic NE2000 attach.  This will read the station address
	 * from the EEPROM.
	 */
	ne2000_attach(nsc, NULL);

	/* Establish the interrupt handler. */
	msc->sc_ih = intc_intr_establish(SH7709_INTEVT2_IRQ2, IST_LEVEL,
	    IPL_NET, dp8390_intr, dsc);
	if (msc->sc_ih == NULL)
		aprint_error_dev(self,
		    "couldn't establish interrupt handler\n");
}

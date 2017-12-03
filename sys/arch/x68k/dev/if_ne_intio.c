/*	$NetBSD: if_ne_intio.c,v 1.17.12.1 2017/12/03 11:36:48 jdolecek Exp $	*/

/*
 * Copyright (c) 2001 Tetsuya Isaki. All rights reserved.
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Ethernet part of Nereid Ethernet/USB/Memory board
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ne_intio.c,v 1.17.12.1 2017/12/03 11:36:48 jdolecek Exp $");

#include "opt_inet.h"
#include "opt_ns.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#endif

#if BPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/bus.h>
#include <machine/cpu.h>

#include <dev/ic/dp8390reg.h>
#include <dev/ic/dp8390var.h>
#include <dev/ic/ne2000reg.h>
#include <dev/ic/ne2000var.h>

#include <arch/x68k/dev/intiovar.h>

#define NE_INTIO_ADDR  (0xece300)
#define NE_INTIO_ADDR2 (0xeceb00)
#define NE_INTIO_INTR  (0xf9)
#define NE_INTIO_INTR2 (0xf8)

static int  ne_intio_match(device_t, cfdata_t, void *);
static void ne_intio_attach(device_t, device_t, void *);

#define ne_intio_softc ne2000_softc

CFATTACH_DECL_NEW(ne_intio, sizeof(struct ne_intio_softc),
    ne_intio_match, ne_intio_attach, NULL, NULL);

static int
ne_intio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct intio_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_bst;
	bus_space_handle_t ioh;
	bus_space_tag_t asict;
	bus_space_handle_t asich;
	int rv = 0;

	if (ia->ia_addr == INTIOCF_ADDR_DEFAULT)
		ia->ia_addr = NE_INTIO_ADDR;
	if (ia->ia_intr == INTIOCF_INTR_DEFAULT)
		ia->ia_intr = NE_INTIO_INTR;

	/* fixed parameters */
	if (!(ia->ia_addr == NE_INTIO_ADDR  && ia->ia_intr == NE_INTIO_INTR ) &&
	    !(ia->ia_addr == NE_INTIO_ADDR2 && ia->ia_intr == NE_INTIO_INTR2)  )
		return 0;

	/* Make sure this is a valid NE2000 I/O address */
	if ((ia->ia_addr & 0x1f) != 0)
		return 0;

	/* Check whether the board is inserted or not */
	if (badaddr((void *)IIOV(ia->ia_addr)))
		return 0;

	/* Map I/O space */
	if (bus_space_map(iot, ia->ia_addr, NE2000_NPORTS*2,
			BUS_SPACE_MAP_SHIFTED_EVEN, &ioh))
		return 0;

	asict = iot;
	if (bus_space_subregion(iot, ioh, NE2000_ASIC_OFFSET*2,
			NE2000_ASIC_NPORTS*2, &asich))
		goto out;

	/* Look for an NE2000 compatible card */
	rv = ne2000_detect(iot, ioh, asict, asich);

 out:
	bus_space_unmap(iot, ioh, NE2000_NPORTS);
	return (rv != 0) ? 1 : 0;
}

static void
ne_intio_attach(device_t parent, device_t self, void *aux)
{
	struct ne_intio_softc *sc = device_private(self);
	struct dp8390_softc *dsc = &sc->sc_dp8390;
	struct intio_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_bst;
	bus_space_handle_t ioh;
	bus_space_tag_t asict;
	bus_space_handle_t asich;
	const char *typestr;
	int netype;

	dsc->sc_dev = self;
	aprint_normal(": Nereid Ethernet\n");

	/* Map I/O space */
	if (bus_space_map(iot, ia->ia_addr, NE2000_NPORTS*2,
			BUS_SPACE_MAP_SHIFTED_EVEN, &ioh)){
		aprint_error_dev(self, "can't map I/O space\n");
		return;
	}

	asict = iot;
	if (bus_space_subregion(iot, ioh, NE2000_ASIC_OFFSET*2,
			NE2000_ASIC_NPORTS*2, &asich)) {
		aprint_error_dev(self, "can't subregion I/O space\n");
		return;
	}

	dsc->sc_regt = iot;
	dsc->sc_regh = ioh;

	sc->sc_asict = asict;
	sc->sc_asich = asich;

	/*
	 * detect it again, so we can print some information about
	 * the interface.
	 * XXX: Should I check NE1000 or NE2000 for Nereid?
	 */
	netype = ne2000_detect(iot, ioh, asict, asich);
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
		return;
	}

	aprint_normal_dev(self, "%s Ethernet\n", typestr);

	/* This interface is always enabled */
	dsc->sc_enabled = 1;

	/*
	 * Do generic NE2000 attach.
	 * This will read the mac address from the EEPROM.
	 */
	ne2000_attach(sc, NULL);

	/* Establish the interrupt handler */
	if (intio_intr_establish(ia->ia_intr, "ne", dp8390_intr, dsc))
		aprint_error_dev(self,
		    "couldn't establish interrupt handler\n");
}

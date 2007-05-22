/*	$NetBSD: if_ne_neptune.c,v 1.11.38.1 2007/05/22 17:27:43 matt Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ne_neptune.c,v 1.11.38.1 2007/05/22 17:27:43 matt Exp $");

#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
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

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/bus.h>

#include <dev/ic/dp8390reg.h>
#include <dev/ic/dp8390var.h>

#include <dev/ic/ne2000reg.h>
#include <dev/ic/ne2000var.h>

#include <dev/ic/rtl80x9reg.h>
#include <dev/ic/rtl80x9var.h>

#include <arch/x68k/dev/neptunevar.h>

static int ne_neptune_match(struct device *, struct cfdata *, void *);
static void ne_neptune_attach(struct device *, struct device *, void *);
static int ne_neptune_intr(void *);

#define ne_neptune_softc ne2000_softc

CFATTACH_DECL(ne_neptune, sizeof(struct ne_neptune_softc),
    ne_neptune_match, ne_neptune_attach, NULL, NULL);

int
ne_neptune_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct neptune_attach_args *na = aux;
	bus_space_tag_t nict = na->na_bst;
	bus_space_handle_t nich;
	bus_space_tag_t asict;
	bus_space_handle_t asich;
	int rv = 0;

	if (na->na_addr == NEPTUNECF_ADDR_DEFAULT)
		na->na_addr = 0x300;

	/* Make sure this is a valid NE[12]000 i/o address. */
	if ((na->na_addr & 0x1f) != 0)
		return (0);

	/* Map i/o space. */
	if (bus_space_map(nict, na->na_addr, NE2000_NPORTS*2, 0, &nich))
		return (0);

	asict = nict;
	if (bus_space_subregion(nict, nich, NE2000_ASIC_OFFSET,
	    NE2000_ASIC_NPORTS*2, &asich))
		goto out;

	/* Look for an NE2000-compatible card. */
	rv = ne2000_detect(nict, nich, asict, asich);

 out:
	bus_space_unmap(nict, nich, NE2000_NPORTS);
	return (rv);
}

void
ne_neptune_attach(struct device *parent, struct device *self, void *aux)
{
	struct ne_neptune_softc *nsc = (struct ne_neptune_softc *)self;
	struct dp8390_softc *dsc = &nsc->sc_dp8390;
	struct neptune_attach_args *na = aux;
	bus_space_tag_t nict = na->na_bst;
	bus_space_handle_t nich;
	bus_space_tag_t asict = nict;
	bus_space_handle_t asich;
	const char *typestr;
	int netype;

	printf("\n");

	/* Map i/o space. */
	if (bus_space_map(nict, na->na_addr, NE2000_NPORTS*2, 0, &nich)) {
		printf("%s: can't map i/o space\n", dsc->sc_dev.dv_xname);
		return;
	}

	if (bus_space_subregion(nict, nich, NE2000_ASIC_OFFSET,
	    NE2000_ASIC_NPORTS*2, &asich)) {
		printf("%s: can't subregion i/o space\n", dsc->sc_dev.dv_xname);
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
		/*
		 * Check for a Realtek 8019.
		 */
		bus_space_write_1(nict, nich, ED_P0_CR,
		    ED_CR_PAGE_0 | ED_CR_STP);
		if (bus_space_read_1(nict, nich, NERTL_RTL0_8019ID0) ==
								RTL0_8019ID0 &&
		    bus_space_read_1(nict, nich, NERTL_RTL0_8019ID1) ==
								RTL0_8019ID1) {
			typestr = "NE2000 (RTL8019)";
			dsc->sc_mediachange = rtl80x9_mediachange;
			dsc->sc_mediastatus = rtl80x9_mediastatus;
			dsc->init_card = rtl80x9_init_card;
			dsc->sc_media_init = rtl80x9_media_init;
		}
		break;

	default:
		printf("%s: where did the card go?!\n", dsc->sc_dev.dv_xname);
		return;
	}

	printf("%s: %s Ethernet\n", dsc->sc_dev.dv_xname, typestr);

	/* This interface is always enabled. */
	dsc->sc_enabled = 1;

	/*
	 * Do generic NE2000 attach.  This will read the station address
	 * from the EEPROM.
	 */
	ne2000_attach(nsc, NULL);

	/* Establish the interrupt handler. */
	if (neptune_intr_establish(na->na_intr, "ne", ne_neptune_intr, dsc))
		printf("%s: couldn't establish interrupt handler\n",
		    dsc->sc_dev.dv_xname);
}

static int
ne_neptune_intr(void *arg)
{
	spl4();			/* XXX */
	return dp8390_intr(arg);
}

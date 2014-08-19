/*	$NetBSD: if_ne_xsh.c,v 1.4.10.2 2014/08/20 00:02:43 tls Exp $ */

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>

/*
 * X-Surf 100 driver, ne(4) attachment. 
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/bus.h>

#include <machine/cpu.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <dev/ic/dp8390reg.h>
#include <dev/ic/dp8390var.h>

#include <dev/ic/ne2000reg.h>
#include <dev/ic/ne2000var.h>

#include <dev/ic/ax88190var.h>

#include <amiga/amiga/device.h>
#include <amiga/amiga/isr.h>

#include <amiga/dev/xshvar.h>
#include <amiga/dev/zbusvar.h>

/* #define NE_XSH_DEBUG 1 */

int	ne_xsh_match(device_t, cfdata_t , void *);
void	ne_xsh_attach(device_t, device_t, void *);
#ifdef NE_XSH_DEBUG
void	ne_xsh_debug_mapping(bus_space_tag_t, bus_space_handle_t, 
	    bus_space_handle_t);
#endif /* NE_XSH_DEBUG */

struct ne_xsh_softc {
	struct ne2000_softc	sc_ne2000;
	struct bus_space_tag	sc_bst;
	struct isr		sc_isr;
};

CFATTACH_DECL_NEW(ne_xsh, sizeof(struct ne_xsh_softc),
    ne_xsh_match, ne_xsh_attach, NULL, NULL);

int
ne_xsh_match(device_t parent, cfdata_t cf, void *aux)
{
	struct xshbus_attach_args *xap = aux;

	if (strcmp(xap->xaa_name, "ne_xsh") != 0)
		return 0;

	return 1;
}

void
ne_xsh_attach(device_t parent, device_t self, void *aux)
{
	struct ne_xsh_softc *zsc = device_private(self);
	struct ne2000_softc *nsc = &zsc->sc_ne2000;
	struct dp8390_softc *dsc = &nsc->sc_dp8390;

	struct xshbus_attach_args *xap = aux;

	bus_space_tag_t xsht;
	bus_space_handle_t nich;
	bus_space_handle_t asich;

	dsc->sc_dev = self;

	zsc->sc_bst.base = xap->xaa_base;
	zsc->sc_bst.absm = &amiga_bus_stride_4;

	aprint_normal("\n");

	xsht = &zsc->sc_bst;

	/* Map the NIC and ASIC spaces. */
	if (bus_space_map(xsht, NE2000_NIC_OFFSET, NE2000_NPORTS, 0, &nich)) {
		aprint_error_dev(self, "can't map nic i/o space\n");
		return;
	}
	if (bus_space_subregion(xsht, nich, NE2000_ASIC_OFFSET, 
	    NE2000_ASIC_NPORTS, &asich)) {
		aprint_error_dev(self, "can't map asic i/o space\n");
		return;
	}

#ifdef NE_XSH_DEBUG
	ne_xsh_debug_mapping(xsht, nich, asich);	
#endif /* NE_XSH_DEBUG */

	dsc->sc_regt = xsht;
	dsc->sc_regh = nich;

	nsc->sc_asict = xsht;
	nsc->sc_asich = asich;

	/* This interface is always enabled. */
	dsc->sc_enabled = 1;

	nsc->sc_type = NE2000_TYPE_AX88796;

	dsc->sc_mediachange = ax88190_mediachange;
	dsc->sc_mediastatus = ax88190_mediastatus;
	dsc->init_card = ax88190_init_card;
	dsc->stop_card = ax88190_stop_card;
	dsc->sc_media_init = ax88190_media_init;
	dsc->sc_media_fini = ax88190_media_fini;

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

#ifdef NE_XSH_DEBUG
void
ne_xsh_debug_mapping(bus_space_tag_t xsht, bus_space_handle_t nich, 
    bus_space_handle_t asich)
{
	bus_addr_t va;
	int i;

	aprint_normal("xsh: NIC mapping: ");
	for(va = nich, i = 0; i < NE2000_NIC_NPORTS; i++) {
		aprint_normal("%x:%x ", i, kvtop((void*) (va+i)));
	}
	aprint_normal("\n");

	aprint_normal("xsh: ASIC mapping: ");
	for(va = asich, i = 0; i < NE2000_ASIC_NPORTS; i++) {
		aprint_normal("%x:%x ", i, kvtop((void*) (va+i)));
	}
	aprint_normal("\n");
}
#endif /* NE_XSH_DEBUG */

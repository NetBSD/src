/* $NetBSD: dwctwo_plb.c,v 1.3 2014/09/02 14:55:56 skrll Exp $ */
/*
 * Copyright (c) 2013 KIYOHARA Takashi
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dwctwo_plb.c,v 1.3 2014/09/02 14:55:56 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/extent.h>

#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/dev/plbvar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dwc2/dwc2.h>
#include <dwc2/dwc2var.h>
#include "dwc2_core.h"

#include "locators.h"

#define DWCTWO_SIZE	0x40000

static int dwctwo_plb_match(device_t, cfdata_t, void *);
static void dwctwo_plb_attach(device_t, device_t, void *);

static void dwctwo_plb_deferred(device_t);

CFATTACH_DECL_NEW(dwctwo_plb, sizeof(struct dwc2_softc),
    dwctwo_plb_match, dwctwo_plb_attach, NULL, NULL);

static struct powerpc_bus_space dwctwo_tag = {
	_BUS_SPACE_LITTLE_ENDIAN | _BUS_SPACE_MEM_TYPE,
	0x00000000,
	0x00000000,
	DWCTWO_SIZE
};
static char ex_storage[EXTENT_FIXED_STORAGE_SIZE(8)]
    __attribute__((aligned(8)));


static int
dwctwo_plb_match(device_t parent, cfdata_t match, void *aux)
{
	struct plb_attach_args *paa = aux;

	if (strcmp(paa->plb_name, match->cf_name) != 0)
		return 0;

	if (match->cf_loc[PLBCF_ADDR] == PLBCF_ADDR_DEFAULT)
		panic("dwctwo_plb_match: wildcard addr not allowed");
	if (match->cf_loc[PLBCF_IRQ] == PLBCF_IRQ_DEFAULT)
		panic("dwctwo_plb_match: wildcard IRQ not allowed");

	paa->plb_addr = match->cf_loc[PLBCF_ADDR];
	paa->plb_irq = match->cf_loc[PLBCF_IRQ];
	return 1;
}

static void
dwctwo_plb_attach(device_t parent, device_t self, void *aux)
{
	struct dwc2_softc *sc = device_private(self);
	struct plb_attach_args *paa = aux;
	prop_dictionary_t dict = device_properties(self);
	uint32_t srst0;

	sc->sc_dev = self;

	/* get core parameters */
	if (!prop_dictionary_get_uint32(dict, "params",
	    (uint32_t *)&sc->sc_params)) {
		aprint_error("struct dwc2_core_params not found\n");
		return;
	}

	dwctwo_tag.pbs_base = paa->plb_addr;
	dwctwo_tag.pbs_limit += paa->plb_addr;
	if (bus_space_init(&dwctwo_tag, "dwctwotag", ex_storage,
	    sizeof(ex_storage)))
		panic("dwctwo_attach: Failed to initialise opb_tag");
	sc->sc_iot = &dwctwo_tag;
	bus_space_map(sc->sc_iot, paa->plb_addr, DWCTWO_SIZE, 0, &sc->sc_ioh);
	sc->sc_bus.dmatag = paa->plb_dmat;

	intr_establish(paa->plb_irq, IST_LEVEL, IPL_SCHED, dwc2_intr, sc);

	/* Enable the USB interface. */
	mtsdr(DCR_SDR0_PFC1, mfsdr(DCR_SDR0_PFC1) | SDR0_PFC1_USBEN);
	srst0 = mfsdr(DCR_SDR0_SRST0);
	mtsdr(DCR_SDR0_SRST0, srst0 | SDR0_SRST0_UPRST | SDR0_SRST0_AHB);
	delay(200 * 1000);	/* XXXX */
	mtsdr(DCR_SDR0_SRST0, srst0);

	config_defer(self, dwctwo_plb_deferred);
}

static void
dwctwo_plb_deferred(device_t self)
{
	struct dwc2_softc *sc = device_private(self);
	int error;

	error = dwc2_init(sc);
	if (error != 0) {
		aprint_error_dev(self, "couldn't initialize host, error=%d\n",
		    error);
		return;
	}
	sc->sc_child = config_found(sc->sc_dev, &sc->sc_bus, usbctlprint);
}

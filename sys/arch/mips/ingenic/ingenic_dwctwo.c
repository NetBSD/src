/*	$NetBSD: ingenic_dwctwo.c,v 1.1 2014/12/06 14:35:47 macallan Exp $ */

/*-
 * Copyright (c) 2014 Michael Lorenz
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
__KERNEL_RCSID(0, "$NetBSD: ingenic_dwctwo.c,v 1.1 2014/12/06 14:35:47 macallan Exp $");

/*
 * adapted from bcm2835_dwctwo.c
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/bus.h>
#include <sys/workqueue.h>

#include <mips/ingenic/ingenic_var.h>
#include <mips/ingenic/ingenic_regs.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dwc2/dwc2var.h>

#include <dwc2/dwc2.h>
#include "dwc2_core.h"

struct ingenic_dwc2_softc {
	struct dwc2_softc	sc_dwc2;

	void			*sc_ih;
};

static struct dwc2_core_params ingenic_dwc2_params = {
	.otg_cap			= 0,	/* HNP/SRP capable */
	.otg_ver			= 0,	/* 1.3 */
	.dma_enable			= 1,
	.dma_desc_enable		= 0,
	.speed				= 0,	/* High Speed */
	.enable_dynamic_fifo		= 1,
	.en_multiple_tx_fifo		= 1,
	.host_rx_fifo_size		= 774,	/* 774 DWORDs */
	.host_nperio_tx_fifo_size	= 256,	/* 256 DWORDs */
	.host_perio_tx_fifo_size	= 512,	/* 512 DWORDs */
	.max_transfer_size		= 65535,
	.max_packet_count		= 511,
	.host_channels			= 8,
	.phy_type			= 1,	/* UTMI */
	.phy_utmi_width			= 8,	/* 8 bits */
	.phy_ulpi_ddr			= 0,	/* Single */
	.phy_ulpi_ext_vbus		= 0,
	.i2c_enable			= 0,
	.ulpi_fs_ls			= 0,
	.host_support_fs_ls_low_power	= 0,
	.host_ls_low_power_phy_clk	= 0,	/* 48 MHz */
	.ts_dline			= 0,
	.reload_ctl			= 0,
	.ahbcfg				= 0x10,
	.uframe_sched			= 1,
};

static int ingenic_dwc2_match(device_t, struct cfdata *, void *);
static void ingenic_dwc2_attach(device_t, device_t, void *);
static void ingenic_dwc2_deferred(device_t);

CFATTACH_DECL_NEW(ingenic_dwctwo, sizeof(struct ingenic_dwc2_softc),
    ingenic_dwc2_match, ingenic_dwc2_attach, NULL, NULL);

/* ARGSUSED */
static int
ingenic_dwc2_match(device_t parent, struct cfdata *match, void *aux)
{
	struct apbus_attach_args *aa = aux;

	if (strcmp(aa->aa_name, "dwctwo") != 0)
		return 0;

	return 1;
}

/* ARGSUSED */
static void
ingenic_dwc2_attach(device_t parent, device_t self, void *aux)
{
	struct ingenic_dwc2_softc *sc = device_private(self);
	struct apbus_attach_args *aa = aux;
	int error;

	sc->sc_dwc2.sc_dev = self;

	sc->sc_dwc2.sc_iot = aa->aa_bst;
	sc->sc_dwc2.sc_bus.dmatag = aa->aa_dmat;
	sc->sc_dwc2.sc_params = &ingenic_dwc2_params;

	if (aa->aa_addr == 0)
		aa->aa_addr = 0x13500000;

	error = bus_space_map(aa->aa_bst, aa->aa_addr, 0x20000, 0,
	    &sc->sc_dwc2.sc_ioh);
	if (error) {
		aprint_error_dev(self,
		    "can't map registers for %s: %d\n", aa->aa_name, error);
		return;
	}

	aprint_naive(": USB controller\n");
	aprint_normal(": USB controller\n");

#if notyet
	sc->sc_ih = bcm2835_intr_establish(aaa->aaa_intr, IPL_SCHED,
	   dwc2_intr, &sc->sc_dwc2);

	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     aaa->aaa_intr);
		goto fail;
	}
#endif
	config_defer(self, ingenic_dwc2_deferred);

	return;

#if notyet
fail:
	if (sc->sc_ih) {
		intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}
#endif
	bus_space_unmap(sc->sc_dwc2.sc_iot, sc->sc_dwc2.sc_ioh, 0x20000);
}

static void
ingenic_dwc2_deferred(device_t self)
{
	struct ingenic_dwc2_softc *sc = device_private(self);
	int error;

	error = dwc2_init(&sc->sc_dwc2);
	if (error != 0) {
		aprint_error_dev(self, "couldn't initialize host, error=%d\n",
		    error);
		return;
	}
	sc->sc_dwc2.sc_child = config_found(sc->sc_dwc2.sc_dev,
	    &sc->sc_dwc2.sc_bus, usbctlprint);
}

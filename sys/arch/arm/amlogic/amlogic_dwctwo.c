/*	$NetBSD: amlogic_dwctwo.c,v 1.6.16.2 2017/12/03 11:35:51 jdolecek Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: amlogic_dwctwo.c,v 1.6.16.2 2017/12/03 11:35:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/bus.h>
#include <sys/workqueue.h>

#include <arm/amlogic/amlogic_reg.h>
#include <arm/amlogic/amlogic_crureg.h>
#include <arm/amlogic/amlogic_var.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dwc2/dwc2var.h>

#include <dwc2/dwc2.h>
#include "dwc2_core.h"

struct amlogic_dwctwo_softc {
	struct dwc2_softc	sc_dwc2;

	void			*sc_ih;
};

static struct dwc2_core_params amlogic_dwctwo_params = {
	.otg_cap			= 2,	/* 0 - HNP/SRP capable */
	.otg_ver			= 1,	/* 1 - 2.0 */
	.dma_enable			= -1,	/* 1 - DMA (default, if available) */
	.dma_desc_enable		= 0,	/* 1 - Descriptor DMA (default, if available) */
	.speed				= -1,	/* 0 - High Speed */
	.enable_dynamic_fifo		= 1,	/* 1 - Allow dynamic FIFO sizing (default, if available) */
	.en_multiple_tx_fifo		= -1,	/* Specifies whether dedicated per-endpoint transmit FIFOs are enabled */
	.host_rx_fifo_size		= 512,	/* 512 DWORDs */
	.host_nperio_tx_fifo_size	= 500,	/* 500 DWORDs */
	.host_perio_tx_fifo_size	= -1,	/* 256 DWORDs */
	.max_transfer_size		= -1,   /* 2047 to 65,535 */
	.max_packet_count		= -1,   /* 15 to 511 */
	.host_channels			= -1,	/* 1 to 16 */
	.phy_type			= 1, 	/* 1- UTMI+ Phy */
	.phy_utmi_width			= -1,	/* 8 bits */
	.phy_ulpi_ddr			= -1,	/* Single */
	.phy_ulpi_ext_vbus		= -1,
	.i2c_enable			= -1,
	.ulpi_fs_ls			= -1,	/* 0 - No (default) */
	.host_support_fs_ls_low_power	= -1,	/* 0 - Don't support low power mode (default) */
	.host_ls_low_power_phy_clk	= -1,	/* 1 - 48 MHz  default when phy_type is UTMI+ or ULPI*/
	.ts_dline			= -1,	/* 0 - No (default) */
	.reload_ctl			= -1,	/* 0 - No (default for core < 2.92a) */
	.ahbcfg				= 0x3,	/* INCR4 */
	.uframe_sched			= 1,	/* True to enable microframe scheduler */
	.external_id_pin_ctl		= -1,
	.hibernation			= -1,
};

static int amlogic_dwctwo_match(device_t, struct cfdata *, void *);
static void amlogic_dwctwo_attach(device_t, device_t, void *);
static void amlogic_dwctwo_deferred(device_t);

CFATTACH_DECL_NEW(amlogic_dwctwo, sizeof(struct amlogic_dwctwo_softc),
    amlogic_dwctwo_match, amlogic_dwctwo_attach, NULL, NULL);

static int
amlogic_dwctwo_match(device_t parent, struct cfdata *match, void *aux)
{
	return 1;
}

static void
amlogic_dwctwo_attach(device_t parent, device_t self, void *aux)
{
	struct amlogic_dwctwo_softc * const sc = device_private(self);
	struct amlogicio_attach_args * const aio = aux;
	const struct amlogic_locators * const loc = &aio->aio_loc;

	sc->sc_dwc2.sc_dev = self;

	sc->sc_dwc2.sc_iot = aio->aio_core_bst;
	bus_space_subregion(aio->aio_core_bst, aio->aio_bsh,
            loc->loc_offset, loc->loc_size, &sc->sc_dwc2.sc_ioh);
	sc->sc_dwc2.sc_bus.ub_dmatag = aio->aio_dmat;
	sc->sc_dwc2.sc_params = &amlogic_dwctwo_params;

	aprint_naive("\n");
	aprint_normal(": USB controller\n");

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_VM,
	   IST_LEVEL | IST_MPSAFE, dwc2_intr, &sc->sc_dwc2);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     loc->loc_intr);
		goto fail;
	}
	config_defer(self, amlogic_dwctwo_deferred);

	return;

fail:
	if (sc->sc_ih) {
		intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}
}

static void
amlogic_dwctwo_deferred(device_t self)
{
	struct amlogic_dwctwo_softc *sc = device_private(self);
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

/*	$NetBSD: bcm2835_dwctwo.c,v 1.8 2017/12/10 21:38:26 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_dwctwo.c,v 1.8 2017/12/10 21:38:26 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/bus.h>
#include <sys/workqueue.h>

#include <arm/broadcom/bcm2835reg.h>

#include <dev/fdt/fdtvar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dwc2/dwc2var.h>

#include <dwc2/dwc2.h>
#include "dwc2_core.h"

struct bcmdwc2_softc {
	struct dwc2_softc	sc_dwc2;

	void			*sc_ih;
	int			sc_phandle;
};

static struct dwc2_core_params bcmdwc2_params = {
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
	.external_id_pin_ctl		= -1,
	.hibernation			= -1,
};

static int bcmdwc2_match(device_t, struct cfdata *, void *);
static void bcmdwc2_attach(device_t, device_t, void *);
static void bcmdwc2_deferred(device_t);

CFATTACH_DECL_NEW(bcmdwctwo, sizeof(struct bcmdwc2_softc),
    bcmdwc2_match, bcmdwc2_attach, NULL, NULL);

/* ARGSUSED */
static int
bcmdwc2_match(device_t parent, struct cfdata *match, void *aux)
{
	const char * const compatible[] = {
	    "brcm,bcm2708-usb",
	    "brcm,bcm2835-usb",
	    NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

/* ARGSUSED */
static void
bcmdwc2_attach(device_t parent, device_t self, void *aux)
{
	struct bcmdwc2_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	aprint_naive(": USB controller\n");
	aprint_normal(": USB controller\n");

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_phandle = phandle;
	sc->sc_dwc2.sc_dev = self;
	sc->sc_dwc2.sc_iot = faa->faa_bst;
	sc->sc_dwc2.sc_bus.ub_dmatag = faa->faa_dmat;
	sc->sc_dwc2.sc_params = &bcmdwc2_params;

	error = bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_dwc2.sc_ioh);
	if (error) {
		aprint_error(": couldn't map device\n");
		return;
	}

	char intrstr[128];
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_VM, FDT_INTR_MPSAFE,
	    dwc2_intr, &sc->sc_dwc2);

	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %s\n",
		    intrstr);
		goto fail;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
	config_interrupts(self, bcmdwc2_deferred);

	return;

fail:
	if (sc->sc_ih) {
		fdtbus_intr_disestablish(sc->sc_phandle, sc->sc_ih);
		sc->sc_ih = NULL;
	}
	bus_space_unmap(sc->sc_dwc2.sc_iot, sc->sc_dwc2.sc_ioh, size);
}

static void
bcmdwc2_deferred(device_t self)
{
	struct bcmdwc2_softc *sc = device_private(self);
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

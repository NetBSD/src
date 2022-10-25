/*	$NetBSD: zynq7000_usb.c,v 1.3 2022/10/25 22:52:48 jmcneill Exp $	*/
/*-
 * Copyright (c) 2015  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
__KERNEL_RCSID(0, "$NetBSD: zynq7000_usb.c,v 1.3 2022/10/25 22:52:48 jmcneill Exp $");

#include "opt_soc.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

#include <arm/xilinx/zynq_usbvar.h>
#include <arm/xilinx/zynq_usbreg.h>

#include <dev/fdt/fdtvar.h>

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "xlnx,zynq-usb-2.20a" },
	DEVICE_COMPAT_EOL
};

static int zynqusb_match(device_t, struct cfdata *, void *);
static void zynqusb_attach(device_t, device_t, void *);

/* attach structures */
CFATTACH_DECL_NEW(zynqusb, sizeof(struct zynqehci_softc),
    zynqusb_match, zynqusb_attach, NULL, NULL);

static int
zynqusb_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
zynqusb_attach(device_t parent, device_t self, void *aux)
{
	struct fdt_attach_args * faa = aux;
	char intrstr[128];
	const int phandle = faa->faa_phandle;
	struct zynqehci_softc *sc = device_private(self);
	ehci_softc_t *hsc = &sc->sc_hsc;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	if (fdtbus_intr_establish(phandle, 0, IPL_USB, IST_LEVEL, ehci_intr,
				  hsc) == NULL) {
		aprint_error("failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}

	zynqusb_attach_common(parent, self, faa->faa_bst, faa->faa_dmat,
	    addr, size, 0, ZYNQUSBC_IF_ULPI, ZYNQUSB_HOST);
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
}

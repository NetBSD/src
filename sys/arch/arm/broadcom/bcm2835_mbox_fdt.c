/*	$NetBSD: bcm2835_mbox_fdt.c,v 1.3 2021/01/29 14:11:14 skrll Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_mbox_fdt.c,v 1.3 2021/01/29 14:11:14 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/timetc.h>
#include <sys/bus.h>
#include <sys/mutex.h>

#include <arm/broadcom/bcm2835_mbox.h>
#include <arm/broadcom/bcm2835_mboxreg.h>
#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835var.h>

#include <dev/fdt/fdtvar.h>

static int bcmmbox_fdt_match(device_t, cfdata_t, void *);
static void bcmmbox_fdt_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(bcmmbox_fdt, sizeof(struct bcm2835mbox_softc),
    bcmmbox_fdt_match, bcmmbox_fdt_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "brcm,bcm2835-mbox" },
	DEVICE_COMPAT_EOL
};

/* ARGSUSED */
static int
bcmmbox_fdt_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
bcmmbox_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct bcm2835mbox_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;
	sc->sc_dmat = faa->faa_dmat;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get register address\n");
		return;
	}

	if (bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_ioh) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": VC mailbox\n");

	sc->sc_intrh = fdtbus_intr_establish_xname(phandle, 0, IPL_VM, 0,
	    bcmmbox_intr, sc, device_xname(self));
	if (sc->sc_intrh == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	bcmmbox_attach(sc);
}

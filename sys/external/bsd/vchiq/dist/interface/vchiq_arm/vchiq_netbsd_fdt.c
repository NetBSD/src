/* $NetBSD: vchiq_netbsd_fdt.c,v 1.7 2021/08/07 16:19:18 thorpej Exp $ */

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared D. McNeill
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
__KERNEL_RCSID(0, "$NetBSD: vchiq_netbsd_fdt.c,v 1.7 2021/08/07 16:19:18 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/sysctl.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835_intr.h>

#include <dev/fdt/fdtvar.h>

#include "vchiq_arm.h"
#include "vchiq_2835.h"
#include "vchiq_netbsd.h"

struct vchiq_fdt_softc {
	struct vchiq_softc sc_vchiq;
	int sc_phandle;
};

static int vchiq_fdt_match(device_t, cfdata_t, void *);
static void vchiq_fdt_attach(device_t, device_t, void *);

static void vchiq_fdt_defer(device_t);

/* External functions */
int vchiq_init(void);


CFATTACH_DECL_NEW(vchiq_fdt, sizeof(struct vchiq_fdt_softc),
    vchiq_fdt_match, vchiq_fdt_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "brcm,bcm2835-vchiq" },
	DEVICE_COMPAT_EOL
};

static int
vchiq_fdt_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
vchiq_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct vchiq_fdt_softc *fsc = device_private(self);
	struct vchiq_softc *sc = &fsc->sc_vchiq;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	aprint_naive("\n");
	aprint_normal(": BCM2835 VCHIQ\n");

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;
	fsc->sc_phandle = phandle;

#if BYTE_ORDER == BIG_ENDIAN
	aprint_error_dev(sc->sc_dev, "not supported yet in big-endian mode\n");
	return;
#endif

	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error_dev(sc->sc_dev, "couldn't get register address\n");
		return;
	}

	if (bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_ioh) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	vchiq_platform_attach(faa->faa_dmat);

	vchiq_set_softc(sc);

	config_mountroot(self, vchiq_fdt_defer);
}

static void
vchiq_fdt_defer(device_t self)
{
	struct vchiq_attach_args vaa;
	struct vchiq_fdt_softc *fsc = device_private(self);
	struct vchiq_softc *sc = &fsc->sc_vchiq;
	const int phandle = fsc->sc_phandle;

	vchiq_core_initialize();

	char intrstr[128];
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0, IPL_VM,
	    FDT_INTR_MPSAFE, vchiq_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	vchiq_init();

	vaa.vaa_name = "AUDS";
	config_found(self, &vaa, vchiq_print, CFARGS_NONE);
}

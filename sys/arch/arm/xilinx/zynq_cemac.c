/*	$NetBSD: zynq_cemac.c,v 1.9 2024/08/25 07:25:00 skrll Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: zynq_cemac.c,v 1.9 2024/08/25 07:25:00 skrll Exp $");

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <dev/cadence/if_cemacvar.h>

#include <net/if_ether.h>

#include <dev/fdt/fdtvar.h>

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "cdns,zynq-gem" },
	DEVICE_COMPAT_EOL
};

static int
cemac_match(device_t parent, cfdata_t cfdata, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
cemac_attach(device_t parent, device_t self, void *aux)
{
	struct fdt_attach_args * const faa = aux;
	struct cemac_softc *sc = device_private(self);
	const int phandle = faa->faa_phandle;
	prop_dictionary_t prop = device_properties(self);
	char intrstr[128];
	const char *macaddr;
	bus_addr_t addr;
	bus_size_t size;
	int error, len;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	error = bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_ioh);
	if (error) {
		aprint_error(": failed to map register %#lx@%#lx: %d\n",
		    size, addr, error);
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	if (fdtbus_intr_establish(phandle, 0, IPL_NET, 0, cemac_intr,
				  device_private(self)) == NULL) {
		aprint_error(": failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}

	macaddr = fdtbus_get_prop(phandle, "local-mac-address", &len);
	if (macaddr != NULL && len == ETHER_ADDR_LEN) {
		prop_dictionary_set_data(prop, "mac-address", macaddr, len);
	}

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;
	sc->sc_dmat = faa->faa_dmat;
	sc->cemac_flags = CEMAC_FLAG_GEM;

	cemac_attach_common(sc);
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
}


CFATTACH_DECL_NEW(cemac, sizeof(struct cemac_softc),
    cemac_match, cemac_attach, NULL, NULL);

/* $NetBSD: ahcisata_fdt.c,v 1.1 2018/09/03 23:15:12 jmcneill Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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

__KERNEL_RCSID(0, "$NetBSD: ahcisata_fdt.c,v 1.1 2018/09/03 23:15:12 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <dev/ata/atavar.h>
#include <dev/ic/ahcisatavar.h>

#include <dev/fdt/fdtvar.h>

static const char * compatible[] = {
	"snps,dwc-ahci",
	"generic-ahci",
	NULL
};

static int
ahcisata_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
ahcisata_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct ahci_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct fdtbus_reset *rst;
	struct clk *clk;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	int i;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_atac.atac_dev = self;
	sc->sc_dmat = faa->faa_dmat;
	sc->sc_ahcit = faa->faa_bst;
	sc->sc_ahcis = size;
	if (bus_space_map(sc->sc_ahcit, addr, size, 0, &sc->sc_ahcih) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	if (of_getprop_uint32(phandle, "ports-implemented", &sc->sc_ahci_ports) != 0)
		sc->sc_save_init_data = true;

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	for (i = 0; (clk = fdtbus_clock_get_index(phandle, i)) != NULL; i++)
		if (clk_enable(clk) != 0) {
			aprint_error(": couldn't enable clock #%d\n", i);
			return;
		}
	for (i = 0; (rst = fdtbus_reset_get_index(phandle, i)) != NULL; i++)
		if (fdtbus_reset_deassert(rst) != 0) {
			aprint_error(": couldn't de-assert reset #%d\n", i);
			return;
		}

	aprint_naive("\n");
	aprint_normal(": AHCI SATA controller\n");

	if (fdtbus_intr_establish(phandle, 0, IPL_BIO, 0, ahci_intr, sc) == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n", intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	ahci_attach(sc);
}

CFATTACH_DECL_NEW(ahcisata_fdt, sizeof(struct ahci_softc),
	ahcisata_fdt_match, ahcisata_fdt_attach, NULL, NULL);

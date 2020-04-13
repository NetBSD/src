/*	$NetBSD: sni_emmc.c,v 1.6.4.2 2020/04/13 08:03:37 martin Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

/*
 * Socionext SC2A11 SynQuacer eMMC driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sni_emmc.c,v 1.6.4.2 2020/04/13 08:03:37 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/intr.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <machine/endian.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>
#include <dev/sdmmc/sdmmcvar.h>

#include <dev/fdt/fdtvar.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>

static int sniemmc_fdt_match(device_t, struct cfdata *, void *);
static void sniemmc_fdt_attach(device_t, device_t, void *);
static int sniemmc_acpi_match(device_t, struct cfdata *, void *);
static void sniemmc_acpi_attach(device_t, device_t, void *);

struct sniemmc_softc {
	struct sdhc_softc	sc;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_iob;
	bus_size_t		sc_ios;
	void			*sc_ih;
	struct sdhc_host	*sc_hosts[1];
	bus_dmamap_t		sc_dmamap;
	bus_dma_segment_t	sc_segs[1];
	kcondvar_t		sc_cv;
	int			sc_phandle;
};

CFATTACH_DECL_NEW(sniemmc_fdt, sizeof(struct sniemmc_softc),
    sniemmc_fdt_match, sniemmc_fdt_attach, NULL, NULL);

CFATTACH_DECL_NEW(sniemmc_acpi, sizeof(struct sniemmc_softc),
    sniemmc_acpi_match, sniemmc_acpi_attach, NULL, NULL);

static void sniemmc_attach_i(device_t);

static int
sniemmc_fdt_match(device_t parent, struct cfdata *match, void *aux)
{
	static const char * compatible[] = {
		"socionext,synquacer-sdhci",
		"fujitsu,mb86s70-sdhci-3.0",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sniemmc_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct sniemmc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	prop_dictionary_t dict = device_properties(self);
	const int phandle = faa->faa_phandle;
	bus_space_handle_t ioh;
	bus_addr_t addr;
	bus_size_t size;
	char intrstr[128];
	_Bool disable;

	prop_dictionary_get_bool(dict, "disable", &disable);
	if (disable) {
		aprint_naive(": disabled\n");
		aprint_normal(": disabled\n");
		return;
	}
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0
	    || bus_space_map(faa->faa_bst, addr, size, 0, &ioh) != 0) {
		aprint_error(": unable to map device\n");
		return;
	}
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		goto fail;
	}
	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_SDMMC, 0,
	    sdhc_intr, &sc->sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		goto fail;
	}

	aprint_naive("\n");
	aprint_normal_dev(self, "Socionext eMMC controller\n");
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	sc->sc.sc_dev = self;
	sc->sc.sc_dmat = faa->faa_dmat;
	sc->sc.sc_host = sc->sc_hosts;
	sc->sc_phandle = phandle;
	sc->sc_iot = faa->faa_bst;
	sc->sc_ioh = ioh;
	sc->sc_iob = addr;
	sc->sc_ios = size;

	config_defer(self, sniemmc_attach_i);
	return;
 fail:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
	return;
}

static int
sniemmc_acpi_match(device_t parent, struct cfdata *match, void *aux)
{
	static const char * compatible[] = {
		"SCX0002",
		NULL
	};
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;
	return acpi_match_hid(aa->aa_node->ad_devinfo, compatible);
}

static void
sniemmc_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct sniemmc_softc * const sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	bus_space_handle_t ioh;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	ACPI_STATUS rv;

	rv = acpi_resource_parse(self, aa->aa_node->ad_handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;
	mem = acpi_res_mem(&res, 0);
	irq = acpi_res_irq(&res, 0);
	if (mem == NULL || irq == NULL || mem->ar_length == 0) {
		aprint_error(": incomplete resources\n");
		return;
	}
	if (bus_space_map(aa->aa_memt, mem->ar_base, mem->ar_length, 0,
	    &ioh)) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_ih = acpi_intr_establish(self,
	    (uint64_t)(uintptr_t)aa->aa_node->ad_handle,
	    IPL_BIO, false, sdhc_intr, &sc->sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		goto fail;
	}

	aprint_naive("\n");
	aprint_normal_dev(self, "Socionext eMMC controller\n");

	sc->sc.sc_dev = self;
	sc->sc.sc_dmat = aa->aa_dmat;
	sc->sc.sc_host = sc->sc_hosts;
	sc->sc_iot = aa->aa_memt;
	sc->sc_ioh = ioh;
	sc->sc_ios = mem->ar_length;

	config_defer(self, sniemmc_attach_i);

	acpi_resource_cleanup(&res);
	return;
 fail:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
	acpi_resource_cleanup(&res);
	return;
}

static void
sniemmc_attach_i(device_t self)
{
	struct sniemmc_softc * const sc = device_private(self);
	int error;

	sc->sc.sc_flags = 0;
	sc->sc.sc_flags |= SDHC_FLAG_32BIT_ACCESS;
	sc->sc.sc_clkbase = 50000;	/* Default to 50MHz */

	aprint_normal_dev(sc->sc.sc_dev, "doing sdhc_host() ...\n");
#if 0
	error = sdhc_host_found(&sc->sc, sc->sc_iot, sc->sc_ioh, sc->sc_ios);
#endif
	error = 0;
	if (error) {
		aprint_error_dev(self, "couldn't intialize host, error=%d\n",				error);
		goto fail;
	}
	return;
 fail:
	if (sc->sc_phandle)
		fdtbus_intr_disestablish(sc->sc_phandle, sc->sc_ih);
	else
		acpi_intr_disestablish(sc->sc_ih);
	sc->sc_ih = NULL;
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
	return;
}

/* $NetBSD: genet_acpi.c,v 1.6 2024/09/15 08:30:01 skrll Exp $ */

/*-
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: genet_acpi.c,v 1.6 2024/09/15 08:30:01 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <sys/rndsource.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/miivar.h>

#include <dev/ic/bcmgenetvar.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "BCM6E4E" },	/* Broadcom GENET v5 */
	DEVICE_COMPAT_EOL
};

static int	genet_acpi_match(device_t, cfdata_t, void *);
static void	genet_acpi_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(genet_acpi, sizeof(struct genet_softc),
    genet_acpi_match, genet_acpi_attach, NULL, NULL);

static int
genet_acpi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

static void
genet_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct genet_softc * const sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	ACPI_HANDLE handle = aa->aa_node->ad_handle;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	char *phy_mode;
	ACPI_STATUS rv;
	int error;
	void *ih;

	sc->sc_dev = self;

	rv = acpi_resource_parse(sc->sc_dev, handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	mem = acpi_res_mem(&res, 0);
	if (mem == NULL) {
		aprint_error_dev(self, "couldn't find mem resource\n");
		goto done;
	}

	irq = acpi_res_irq(&res, 0);
	if (irq == NULL) {
		aprint_error_dev(self, "couldn't find irq resource\n");
		goto done;
	}

	sc->sc_bst = aa->aa_memt;
	error = bus_space_map(sc->sc_bst, mem->ar_base, mem->ar_length,
	    0, &sc->sc_bsh);
	if (error != 0) {
		aprint_error_dev(self, "couldn't map registers\n");
		goto done;
	}
	sc->sc_dmat = BUS_DMA_TAG_VALID(aa->aa_dmat64) ?
	    aa->aa_dmat64 : aa->aa_dmat;
	sc->sc_phy_id = MII_PHY_ANY;

	rv = acpi_dsd_string(handle, "phy-mode", &phy_mode);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self, "missing 'phy-mode' property\n");
		goto done;
	}

	if (strcmp(phy_mode, "rgmii-rxid") == 0)
		sc->sc_phy_mode = GENET_PHY_MODE_RGMII_RXID;
	else if (strcmp(phy_mode, "rgmii-txid") == 0)
		sc->sc_phy_mode = GENET_PHY_MODE_RGMII_TXID;
	else if (strcmp(phy_mode, "rgmii-id") == 0)
		sc->sc_phy_mode = GENET_PHY_MODE_RGMII_ID;
	else if (strcmp(phy_mode, "rgmii") == 0)
		sc->sc_phy_mode = GENET_PHY_MODE_RGMII;
	else {
		aprint_error(": unsupported phy-mode '%s'\n", phy_mode);
		goto done;
	}

	aprint_normal("%s", device_xname(self));
	if (genet_attach(sc) != 0)
		goto done;

        ih = acpi_intr_establish(self, (uint64_t)(uintptr_t)handle, IPL_NET,
	    true, genet_intr, sc, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		goto done;
	}

done:
	acpi_resource_cleanup(&res);
}

/*	$NetBSD: bcm2838_emmc2_acpi.c,v 1.3 2022/02/06 15:52:20 jmcneill Exp $	*/

/*
 * Copyright (c) 2021 Jared McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: bcm2838_emmc2_acpi.c,v 1.3 2022/02/06 15:52:20 jmcneill Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kmem.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>
#include <dev/sdmmc/sdmmcvar.h>

#define _COMPONENT	ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME	("bcmemmc2_acpi")

static int	bcmemmc2_acpi_match(device_t, cfdata_t, void *);
static void	bcmemmc2_acpi_attach(device_t, device_t, void *);
static void	bcmemmc2_acpi_attach1(device_t);

static const char * const compatible[] = {
	"BRCME88C",
	NULL
};

struct bcmemmc2_acpi_softc {
	struct sdhc_softc sc;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;
	bus_size_t sc_memsize;
	void *sc_ih;
	struct sdhc_host *sc_hosts[1];
};

CFATTACH_DECL_NEW(bcmemmc2_acpi, sizeof(struct bcmemmc2_acpi_softc),
    bcmemmc2_acpi_match, bcmemmc2_acpi_attach, NULL, NULL);

static int
bcmemmc2_acpi_match(device_t parent, cfdata_t match, void *opaque)
{
	struct acpi_attach_args *aa = opaque;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, compatible);
}

static void
bcmemmc2_acpi_attach(device_t parent, device_t self, void *opaque)
{
	struct bcmemmc2_acpi_softc *sc = device_private(self);
	struct acpi_attach_args *aa = opaque;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	ACPI_STATUS rv;

	sc->sc.sc_dev = self;
	sc->sc.sc_dmat = aa->aa_dmat;
	sc->sc.sc_host = sc->sc_hosts;
	sc->sc_memt = aa->aa_memt;

	rv = acpi_resource_parse(self, aa->aa_node->ad_handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	mem = acpi_res_mem(&res, 0);
	irq = acpi_res_irq(&res, 0);
	if (mem == NULL || irq == NULL) {
		aprint_error_dev(self, "incomplete resources\n");
		goto cleanup;
	}
	if (mem->ar_length == 0) {
		aprint_error_dev(self, "zero length memory resource\n");
		goto cleanup;
	}
	sc->sc_memsize = mem->ar_length;

	if (bus_space_map(sc->sc_memt, mem->ar_base, sc->sc_memsize, 0,
	    &sc->sc_memh)) {
		aprint_error_dev(self, "couldn't map registers\n");
		goto cleanup;
	}

	/*
	 * The controller doesn't honour standard SDHCI voltage registers, so
	 * disable UHS modes.
	 */
	sc->sc.sc_flags = SDHC_FLAG_32BIT_ACCESS |
#if notyet
			  SDHC_FLAG_USE_DMA |
#endif
			  SDHC_FLAG_NO_1_8_V;

	sc->sc_ih = acpi_intr_establish(self,
	    (uint64_t)(uintptr_t)aa->aa_node->ad_handle,
	    IPL_BIO, true, sdhc_intr, &sc->sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self,
		    "couldn't establish interrupt handler\n");
		goto unmap;
	}

	config_defer(self, bcmemmc2_acpi_attach1);

	acpi_resource_cleanup(&res);
	return;

unmap:
	bus_space_unmap(sc->sc_memt, sc->sc_memh, sc->sc_memsize);
	sc->sc_memsize = 0;
cleanup:
	acpi_resource_cleanup(&res);
}

static void
bcmemmc2_acpi_attach1(device_t self)
{
	struct bcmemmc2_acpi_softc *sc = device_private(self);

	if (sdhc_host_found(&sc->sc, sc->sc_memt, sc->sc_memh,
	    sc->sc_memsize) != 0) {
		aprint_error_dev(self, "couldn't initialize host\n");
		goto fail;
	}

	return;

fail:
	if (sc->sc_ih != NULL)
		acpi_intr_disestablish(sc->sc_ih);
	sc->sc_ih = NULL;
}

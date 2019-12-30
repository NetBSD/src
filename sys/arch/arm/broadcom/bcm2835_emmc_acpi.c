/*	$NetBSD: bcm2835_emmc_acpi.c,v 1.1 2019/12/30 18:53:34 jmcneill Exp $	*/

/*
 * Copyright (c) 2016 Kimihiro Nonaka <nonaka@NetBSD.org>
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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_emmc_acpi.c,v 1.1 2019/12/30 18:53:34 jmcneill Exp $");

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

#include <arm/broadcom/bcm2835var.h>
#include <arm/broadcom/bcm2835_mbox.h>

#include <evbarm/rpi/vcio.h>
#include <evbarm/rpi/vcpm.h>
#include <evbarm/rpi/vcprop.h>

#define _COMPONENT	ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME	("bcmemmc_acpi")

static int	bcmemmc_acpi_match(device_t, cfdata_t, void *);
static void	bcmemmc_acpi_attach(device_t, device_t, void *);
static void	bcmemmc_acpi_attach1(device_t);

static u_int	bcmemmc_acpi_emmc_clock_rate(void);

static const char * const compatible[] = {
	"BCM2847",
	NULL
};

static struct {
	struct vcprop_buffer_hdr	vb_hdr;
	struct vcprop_tag_clockrate	vbt_emmcclockrate;
	struct vcprop_tag end;
} vb_emmc __cacheline_aligned = {
	.vb_hdr = {
		.vpb_len = sizeof(vb_emmc),
		.vpb_rcode = VCPROP_PROCESS_REQUEST,
	},
	.vbt_emmcclockrate = {
		.tag = {
			.vpt_tag = VCPROPTAG_GET_CLOCKRATE,
			.vpt_len = VCPROPTAG_LEN(vb_emmc.vbt_emmcclockrate),
			.vpt_rcode = VCPROPTAG_REQUEST
		},
		.id = VCPROP_CLK_EMMC
	},
	.end = {
		.vpt_tag = VCPROPTAG_NULL
	}
};

struct bcmemmc_acpi_softc {
	struct sdhc_softc sc;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;
	bus_size_t sc_memsize;
	void *sc_ih;
	struct sdhc_host *sc_hosts[1];
};

CFATTACH_DECL_NEW(bcmemmc_acpi, sizeof(struct bcmemmc_acpi_softc),
    bcmemmc_acpi_match, bcmemmc_acpi_attach, NULL, NULL);

static int
bcmemmc_acpi_match(device_t parent, cfdata_t match, void *opaque)
{
	struct acpi_attach_args *aa = opaque;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, compatible);
}

static void
bcmemmc_acpi_attach(device_t parent, device_t self, void *opaque)
{
	struct bcmemmc_acpi_softc *sc = device_private(self);
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

	sc->sc.sc_flags = SDHC_FLAG_32BIT_ACCESS |
			  SDHC_FLAG_HOSTCAPS |
			  SDHC_FLAG_NO_HS_BIT;
	sc->sc.sc_caps = SDHC_VOLTAGE_SUPP_3_3V | SDHC_HIGH_SPEED_SUPP |
	    (SDHC_MAX_BLK_LEN_1024 << SDHC_MAX_BLK_LEN_SHIFT);

	sc->sc_ih = acpi_intr_establish(self,
	    (uint64_t)(uintptr_t)aa->aa_node->ad_handle,
	    IPL_BIO, true, sdhc_intr, &sc->sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self,
		    "couldn't establish interrupt handler\n");
		goto unmap;
	}

	config_defer(self, bcmemmc_acpi_attach1);

	acpi_resource_cleanup(&res);
	return;

unmap:
	bus_space_unmap(sc->sc_memt, sc->sc_memh, sc->sc_memsize);
	sc->sc_memsize = 0;
cleanup:
	acpi_resource_cleanup(&res);
}

static void
bcmemmc_acpi_attach1(device_t self)
{
	struct bcmemmc_acpi_softc *sc = device_private(self);

	sc->sc.sc_clkbase = bcmemmc_acpi_emmc_clock_rate() / 1000;

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

static u_int
bcmemmc_acpi_emmc_clock_rate(void)
{
	uint32_t res;
	int error;

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_emmc,
	    sizeof(vb_emmc), &res);
	if (error) {
		printf("%s: mbox request failed (%d)\n", __func__, error);
		return 0;
	}

        if (!vcprop_buffer_success_p(&vb_emmc.vb_hdr) ||
            !vcprop_tag_success_p(&vb_emmc.vbt_emmcclockrate.tag) ||
	    vb_emmc.vbt_emmcclockrate.rate < 0)
                return 0;

	return vb_emmc.vbt_emmcclockrate.rate;
}

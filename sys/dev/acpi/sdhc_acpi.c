/*	$NetBSD: sdhc_acpi.c,v 1.2.2.3 2016/10/05 20:55:40 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sdhc_acpi.c,v 1.2.2.3 2016/10/05 20:55:40 skrll Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kmem.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>
#include <dev/sdmmc/sdmmcvar.h>

#define _COMPONENT	ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME	("sdhc_acpi")

static int	sdhc_acpi_match(device_t, cfdata_t, void *);
static void	sdhc_acpi_attach(device_t, device_t, void *);
static int	sdhc_acpi_detach(device_t, int);
static bool	sdhc_acpi_resume(device_t, const pmf_qual_t *);

struct sdhc_acpi_softc {
	struct sdhc_softc sc;
	int sc_irq;

	ACPI_HANDLE sc_crs, sc_srs;
	ACPI_BUFFER sc_crs_buffer;
};

CFATTACH_DECL_NEW(sdhc_acpi, sizeof(struct sdhc_acpi_softc),
    sdhc_acpi_match, sdhc_acpi_attach, sdhc_acpi_detach, NULL);

static uint32_t sdhc_acpi_intr(void *);

static const char * const sdhc_acpi_ids[] = {
	"80860F14",
	"80860F16",
	NULL
};

static int
sdhc_acpi_match(device_t parent, cfdata_t match, void *opaque)
{
	struct acpi_attach_args *aa = opaque;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, sdhc_acpi_ids);
}

static void
sdhc_acpi_attach(device_t parent, device_t self, void *opaque)
{
	struct sdhc_acpi_softc *sc = device_private(self);
	struct acpi_attach_args *aa = opaque;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	bus_space_handle_t memh;
	ACPI_STATUS rv;

	sc->sc.sc_dev = self;
	sc->sc.sc_dmat = aa->aa_dmat;
	sc->sc.sc_host = NULL;
	sc->sc_irq = -1;

	rv = acpi_resource_parse(self, aa->aa_node->ad_handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	AcpiGetHandle(aa->aa_node->ad_handle, "_CRS", &sc->sc_crs);
	AcpiGetHandle(aa->aa_node->ad_handle, "_SRS", &sc->sc_srs);
	if (sc->sc_crs && sc->sc_srs) {
		/* XXX Why need this? */
		sc->sc_crs_buffer.Pointer = NULL;
		sc->sc_crs_buffer.Length = ACPI_ALLOCATE_LOCAL_BUFFER;
		rv = AcpiGetCurrentResources(sc->sc_crs, &sc->sc_crs_buffer);
		if (ACPI_FAILURE(rv))
			sc->sc_crs = sc->sc_srs = NULL;
	}

	mem = acpi_res_mem(&res, 0);
	irq = acpi_res_irq(&res, 0);
	if (mem == NULL || irq == NULL) {
		aprint_error_dev(self, "incomplete resources\n");
		goto cleanup;
	}

	if (bus_space_map(aa->aa_memt, mem->ar_base, mem->ar_length, 0,
	    &memh)) {
		aprint_error_dev(self, "couldn't map registers\n");
		goto cleanup;
	}

	/* XXX acpi_intr_establish? */
	rv = AcpiOsInstallInterruptHandler(irq->ar_irq, sdhc_acpi_intr, sc);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self,
		    "couldn't establish interrupt handler\n");
		goto cleanup;
	}
	sc->sc_irq = irq->ar_irq;

	sc->sc.sc_host = kmem_zalloc(sizeof(struct sdhc_host *), KM_NOSLEEP);
	if (sc->sc.sc_host == NULL) {
		aprint_error_dev(self, "couldn't alloc memory\n");
		goto cleanup;
	}

	/* Enable DMA transfer */
	sc->sc.sc_flags |= SDHC_FLAG_USE_DMA;

	if (sdhc_host_found(&sc->sc, aa->aa_memt, memh, mem->ar_length) != 0) {
		aprint_error_dev(self, "couldn't initialize host\n");
		goto cleanup;
	}

	if (!pmf_device_register1(self, sdhc_suspend, sdhc_acpi_resume,
	    sdhc_shutdown)) {
		aprint_error_dev(self, "couldn't establish powerhook\n");
	}

	acpi_resource_cleanup(&res);
	return;

cleanup:
	acpi_resource_cleanup(&res);
	if (sc->sc_crs_buffer.Pointer)
		ACPI_FREE(sc->sc_crs_buffer.Pointer);
	if (sc->sc_irq >= 0)
		/* XXX acpi_intr_disestablish? */
		AcpiOsRemoveInterruptHandler(sc->sc_irq, sdhc_acpi_intr);
	if (sc->sc.sc_host != NULL)
		kmem_free(sc->sc.sc_host, sizeof(struct sdhc_host *));
}

static int
sdhc_acpi_detach(device_t self, int flags)
{
	struct sdhc_acpi_softc *sc = device_private(self);
	int rv;

	pmf_device_deregister(self);

	if (sc->sc_crs_buffer.Pointer)
		ACPI_FREE(sc->sc_crs_buffer.Pointer);

	rv = sdhc_detach(&sc->sc, flags);
	if (rv)
		return rv;

	if (sc->sc_irq >= 0)
		/* XXX acpi_intr_disestablish? */
		AcpiOsRemoveInterruptHandler(sc->sc_irq, sdhc_acpi_intr);

	if (sc->sc.sc_host != NULL)
		kmem_free(sc->sc.sc_host, sizeof(struct sdhc_host *));

	return 0;
}

static bool
sdhc_acpi_resume(device_t self, const pmf_qual_t *qual)
{
	struct sdhc_acpi_softc *sc = device_private(self);
	ACPI_STATUS rv;

	if (sc->sc_crs && sc->sc_srs) {
		rv = AcpiSetCurrentResources(sc->sc_srs, &sc->sc_crs_buffer);
		if (ACPI_FAILURE(rv))
			printf("%s: _SRS failed: %s\n",
			    device_xname(self), AcpiFormatException(rv));
	}

	return sdhc_resume(self, qual);
}

static uint32_t
sdhc_acpi_intr(void *context)
{
	struct sdhc_acpi_softc *sc = context;

	if (!sdhc_intr(&sc->sc))
		return ACPI_INTERRUPT_NOT_HANDLED;
	return ACPI_INTERRUPT_HANDLED;
}

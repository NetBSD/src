/*	$NetBSD: sdhc_acpi.c,v 1.9.2.1 2020/02/29 20:19:07 ad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sdhc_acpi.c,v 1.9.2.1 2020/02/29 20:19:07 ad Exp $");

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

/* Freescale ESDHC */
#define	SDHC_ESDHC_FLAGS	\
    (SDHC_FLAG_HAVE_DVS|SDHC_FLAG_NO_PWR0|SDHC_FLAG_32BIT_ACCESS|SDHC_FLAG_ENHANCED)

#define _COMPONENT	ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME	("sdhc_acpi")

static int	sdhc_acpi_match(device_t, cfdata_t, void *);
static void	sdhc_acpi_attach(device_t, device_t, void *);
static int	sdhc_acpi_detach(device_t, int);
static bool	sdhc_acpi_resume(device_t, const pmf_qual_t *);

struct sdhc_acpi_softc {
	struct sdhc_softc sc;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;
	bus_size_t sc_memsize;
	void *sc_ih;

	ACPI_HANDLE sc_crs, sc_srs;
	ACPI_BUFFER sc_crs_buffer;
};

CFATTACH_DECL_NEW(sdhc_acpi, sizeof(struct sdhc_acpi_softc),
    sdhc_acpi_match, sdhc_acpi_attach, sdhc_acpi_detach, NULL);

static void	sdhc_acpi_intel_emmc_hw_reset(struct sdhc_softc *,
		    struct sdhc_host *);

static const struct sdhc_acpi_slot {
	const char *hid;
	const char *uid;
	int type;
#define	SLOT_TYPE_SD	0	/* SD or SDIO */
#define	SLOT_TYPE_EMMC	1	/* eMMC */
	uint32_t flags;
} sdhc_acpi_slot_map[] = {
	{ .hid = "80865ACA",		 .type = SLOT_TYPE_SD },
	{ .hid = "80865ACC",		 .type = SLOT_TYPE_EMMC },
	{ .hid = "80865AD0",		 .type = SLOT_TYPE_SD },
	{ .hid = "80860F14", .uid = "1", .type = SLOT_TYPE_EMMC },
	{ .hid = "80860F14", .uid = "3", .type = SLOT_TYPE_SD },
	{ .hid = "80860F16",   		 .type = SLOT_TYPE_SD },
	{ .hid = "INT33BB",  .uid = "2", .type = SLOT_TYPE_SD },
	{ .hid = "INT33BB",  .uid = "3", .type = SLOT_TYPE_SD },
	{ .hid = "INT33C6",		 .type = SLOT_TYPE_SD },
	{ .hid = "INT3436",		 .type = SLOT_TYPE_SD },
	{ .hid = "INT344D",		 .type = SLOT_TYPE_SD },
	{ .hid = "NXP0003",  .uid = "0", .type = SLOT_TYPE_SD,
					 .flags = SDHC_ESDHC_FLAGS },
	{ .hid = "NXP0003",  .uid = "1", .type = SLOT_TYPE_EMMC,
					 .flags = SDHC_ESDHC_FLAGS },

	/* Generic IDs last */
	{ .hid = "PNP0D40",		 .type = SLOT_TYPE_SD },
	{ .hid = "PNP0FFF",  .uid = "3", .type = SLOT_TYPE_SD },
};

static const struct sdhc_acpi_slot *
sdhc_acpi_find_slot(ACPI_DEVICE_INFO *ad)
{
	const struct sdhc_acpi_slot *slot;
	const char *hid, *uid;
	size_t i;

	hid = ad->HardwareId.String;
	uid = ad->UniqueId.String;

	if (!(ad->Valid & ACPI_VALID_HID) || hid == NULL)
		return NULL;

	for (i = 0; i < __arraycount(sdhc_acpi_slot_map); i++) {
		slot = &sdhc_acpi_slot_map[i];
		const char * const slot_id[] = { slot->hid, NULL };
		if (acpi_match_hid(ad, slot_id)) {
			if (slot->uid == NULL ||
			    ((ad->Valid & ACPI_VALID_UID) != 0 &&
			     uid != NULL &&
			     strcmp(uid, slot->uid) == 0))
				return slot;
		}
	}
	return NULL;
}

static int
sdhc_acpi_match(device_t parent, cfdata_t match, void *opaque)
{
	struct acpi_attach_args *aa = opaque;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return sdhc_acpi_find_slot(aa->aa_node->ad_devinfo) != NULL;
}

static void
sdhc_acpi_attach(device_t parent, device_t self, void *opaque)
{
	struct sdhc_acpi_softc *sc = device_private(self);
	struct acpi_attach_args *aa = opaque;
	const struct sdhc_acpi_slot *slot;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	ACPI_STATUS rv;
	ACPI_INTEGER clock_freq;

	sc->sc.sc_dev = self;
	sc->sc.sc_dmat = aa->aa_dmat;
	sc->sc.sc_host = NULL;
	sc->sc_memt = aa->aa_memt;

	slot = sdhc_acpi_find_slot(aa->aa_node->ad_devinfo);
	if (slot->type == SLOT_TYPE_EMMC)
		sc->sc.sc_vendor_hw_reset = sdhc_acpi_intel_emmc_hw_reset;

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

	sc->sc_ih = acpi_intr_establish(self,
	    (uint64_t)(uintptr_t)aa->aa_node->ad_handle,
	    IPL_BIO, false, sdhc_intr, &sc->sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self,
		    "couldn't establish interrupt handler\n");
		goto unmap;
	}

	sc->sc.sc_host = kmem_zalloc(sizeof(struct sdhc_host *), KM_SLEEP);

	sc->sc.sc_flags |= slot->flags;

	/* Enable DMA transfer */
	sc->sc.sc_flags |= SDHC_FLAG_USE_DMA;

	/* Read clock frequency from device properties */
	rv = acpi_dsd_integer(aa->aa_node->ad_handle, "clock-frequency",
	    &clock_freq);
	if (ACPI_SUCCESS(rv))
		sc->sc.sc_clkbase = clock_freq / 1000;

	if (sdhc_host_found(&sc->sc, sc->sc_memt, sc->sc_memh,
	    sc->sc_memsize) != 0) {
		aprint_error_dev(self, "couldn't initialize host\n");
		goto fail;
	}

	if (!pmf_device_register1(self, sdhc_suspend, sdhc_acpi_resume,
	    sdhc_shutdown)) {
		aprint_error_dev(self, "couldn't establish powerhook\n");
	}

	acpi_resource_cleanup(&res);
	return;

fail:
	if (sc->sc.sc_host != NULL)
		kmem_free(sc->sc.sc_host, sizeof(struct sdhc_host *));
	sc->sc.sc_host = NULL;
	if (sc->sc_ih != NULL)
		acpi_intr_disestablish(sc->sc_ih);
	sc->sc_ih = NULL;
unmap:
	bus_space_unmap(sc->sc_memt, sc->sc_memh, sc->sc_memsize);
	sc->sc_memsize = 0;
cleanup:
	if (sc->sc_crs_buffer.Pointer)
		ACPI_FREE(sc->sc_crs_buffer.Pointer);
	sc->sc_crs_buffer.Pointer = NULL;
	acpi_resource_cleanup(&res);
}

static int
sdhc_acpi_detach(device_t self, int flags)
{
	struct sdhc_acpi_softc *sc = device_private(self);
	int rv;

	pmf_device_deregister(self);

	rv = sdhc_detach(&sc->sc, flags);
	if (rv)
		return rv;

	if (sc->sc_ih != NULL)
		acpi_intr_disestablish(sc->sc_ih);

	if (sc->sc.sc_host != NULL)
		kmem_free(sc->sc.sc_host, sizeof(struct sdhc_host *));

	if (sc->sc_memsize > 0)
		bus_space_unmap(sc->sc_memt, sc->sc_memh, sc->sc_memsize);

	if (sc->sc_crs_buffer.Pointer)
		ACPI_FREE(sc->sc_crs_buffer.Pointer);

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

static void
sdhc_acpi_intel_emmc_hw_reset(struct sdhc_softc *sc, struct sdhc_host *hp)
{
	kmutex_t *plock = sdhc_host_lock(hp);
	uint8_t reg;

	mutex_enter(plock);

	reg = sdhc_host_read_1(hp, SDHC_POWER_CTL);
	reg |= 0x10;
	sdhc_host_write_1(hp, SDHC_POWER_CTL, reg);

	sdmmc_delay(10);

	reg &= ~0x10;
	sdhc_host_write_1(hp, SDHC_POWER_CTL, reg);

	sdmmc_delay(1000);

	mutex_exit(plock);
}

/* $NetBSD: itpm_acpi.c,v 1.1 2012/01/22 06:44:28 christos Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 * ACPI attachment for the Infineon SLD 9630 TT 1.1 and SLB 9635 TT 1.2
 * trusted platform module. See www.trustedcomputinggroup.org
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: itpm_acpi.c,v 1.1 2012/01/22 06:44:28 christos Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/acpi/itpm_acpireg.h>

#define _COMPONENT          ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME            ("itpm_acpi")

static int	itpm_acpi_match(device_t, cfdata_t, void *);
static void	itpm_acpi_attach(device_t, device_t, void *);

struct itpm_acpi_softc {
	struct itpm_softc sc_itpm;
	bus_space_handle_t sc_baseioh;
	struct acpi_devnode *sc_node;	/* ACPI devnode */
};

static int	itpm_acpi_enumerate(struct itpm_acpi_softc *);
static void	itpm_acpi_getknownfds(struct itpm_acpi_softc *);

static const struct fd_type *itpm_acpi_nvtotype(const char *, int, int);

CFATTACH_DECL_NEW(itpm_acpi, sizeof(struct itpm_acpi_softc), itpm_acpi_match,
    itpm_acpi_attach, NULL, NULL);

/*
 * Supported device IDs
 */

static const char * const itpm_acpi_ids[] = {
	"IFX0101",
	"IFX0102",
	NULL
};


#define ITPM_VENDOR_INFINEON	0x15d1
#define ITPM_PRODUCT_SLD_9630	0x0006
#define ITPM_PRODUCT_SLB_9635	0x000b

#define ITPM_CMD_VERSION_L		0x20
#define ITPM_CMD_VERSION_H		0x21
#define ITPM_CMD_DATA_ACTIVATION_REG	0x30
#define ITMP_CMD_RESET_LP_IRQC_DISABLE	0x41
#define ITPM_CMD_ENABLE_REG_PAIR	0x55
#define ITPM_CMD_IOLOC_H		0x60
#define ITPM_CMD_IOLOC_L		0x61
#define ITPM_CMD_DISABLE_REG_PAIR	0xaa
#define ITPM_CMD_VENDOR_ID_L		0xf1
#define ITPM_CMD_VENDOR_ID_H		0xf2
#define ITPM_CMD_PRODUCT_L		0xf3
#define ITPM_CMD_PRODUCT_H		0xf4

#define ITPM_REG_WRFIFO			0
#define ITPM_REG_RDFIFO			1
#define ITPM_REG_STAT			2
#define ITMP_REG_CMD			3

#define ITPM_INFO_ADDR	0x0
#define ITPM_INFO_DATA	0x1

static inline uint8_t
itpm_conf_read(struct itpm_softc *sc, uint8_t off)
{
	return bus_space_read_1(sc->sc_ciot, sc->sc_cioh, off);
}

static inline uint8_t
itpm_data_read(struct itpm_softc *sc, uint8_t off)
{
	return bus_space_read_1(sc->sc_diot, sc->sc_dioh, off);
}

static inline void
itpm_conf_write(struct itpm_softc *sc, uint8_t off, uint8_t val)
{
	bus_space_write_1(sc->sc_ciot, sc->sc_cioh, off, val);
}

static inline void
itpm_data_write(struct itpm_softc *sc, uint8_t off, uint8_t val)
{
	bus_space_write_1(sc->sc_diot, sc->sc_dioh, off, val);
}

static void
itpm_read2(struct itpm_softc *sc, uint8_t offh, uint8_t offl, uint16_t *id)
{
	itpm_conf_write(sc, offh, ITPM_INFO_ADDR);
	*id = itpm_conf_read(sc, ITPM_INFO_DATA) << 8;
	itpm_conf_write(sc, offl, ITPM_INFO_ADDR);
	*id |= itpm_conf_read(sc, ITPM_INFO_DATA);
}

static void
itpm_write2(struct itpm_softc *sc, uint8_t offh, uint8_t offl, uint61_t id)
{
	itpm_conf_write(sc, offh, ITPM_INFO_ADDR);
	itpm_conf_write(sc, (id >> 8) 0xff, ITPM_INFO_DATA);
	itpm_conf_write(sc, offl, ITPM_INFO_ADDR);
	itpm_conf_write(sc, id & 0xff, ITPM_INFO_DATA);
}

/*
 * itpm_acpi_match: autoconf(9) match routine
 */
static int
itpm_acpi_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, itpm_acpi_ids);
}

/*
 * itpm_acpi_attach: autoconf(9) attach routine
 */
static void
itpm_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct itpm_acpi_softc *asc = device_private(self);
	struct itpm_softc *sc = &asc->sc_itpm;
	struct acpi_attach_args *aa = aux;
	struct acpi_io *cio, *dio;
	struct acpi_resources res;
	ACPI_STATUS rv;
	uint16_t vendor, product, version, ioloc;

	sc->sc_dev = self;
	sc->sc_ic = aa->aa_ic;
	asc->sc_node = aa->aa_node;

	/* parse resources */
	rv = acpi_resource_parse(sc->sc_dev, aa->aa_node->ad_handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	/* find our configuration i/o registers */
	cio = acpi_res_io(&res, 0);
	if (cio == NULL || cio->ar_len < 2) {
		aprint_error_dev(sc->sc_dev,
		    "unable to find configuration i/o register resource\n");
		goto out;
	}

	dio = acpi_res_io(&res, 1);
	if (dio == NULL || dio->ar_length < 4) {
		aprint_error_dev(sc->sc_dev,
		    "unable to find data i/o register resource\n");
		goto out;
	}

	sc->sc_ciot = cio->ar_iot;
	if (bus_space_map(sc->sc_ciot, cio->ar_base, cio->ar_length,
	    0, &sc->sc_cioh)) {
		aprint_error_dev(sc->sc_dev, "unable to map ctl i/o space\n");
		goto out;
	}

	sc->sc_diot = dio->ar_iot;
	if (bus_space_map(sc->sc_diot, dio->ar_base, dio->ar_length,
	    0, &sc->sc_dioh)) {
		aprint_error_dev(sc->sc_dev, "unable to map data i/o space\n");
		goto out1;
	}

	itpm_conf_write(sc, ITPM_CMD_REG_ENABLE_PAIR, ITPM_INFO_ADDR);

	itpm_read2(sc, &vendor, ITPM_CMD_VENDOR_ID_H, ITPM_CMD_VENDOR_ID_L);
	if (vendor != ITPM_VENDOR_INFINEON) {
		aprint_error_dev(sc->sc_dev, "unknown itpm vendor 0x%x\n",
		    vendor);
		goto unconf;
	}

	itpm_read2(sc, &product, ITPM_CMD_PRODUCT_H, ITPM_CMD_PRODUCT_L);
	itpm_read2(sc, &version, ITPM_CMD_VERSION_H, ITPM_CMD_VERSION_L);

	switch (product) {
	case ITPM_PRODUCT_SLD_9630:
		aprint_normal_dev(self,
		    "Infineon SLD 9630 TT 1.1, version 0x%x"); version);
		break;
	case ITPM_PRODUCT_SLB_9635:
		aprint_normal_dev(self,
		    "Infineon SLB 9635 TT 1.2, version 0x%x", version);
		break;
	default:
		aprint_error_dev(self,
		    "Unknown Infineon product 0x%x, version 0x%x",
		    product, version);
		break;
	}
	/*
	 * Configure data range locations
	 */
	itpm_write2(sc, ITPM_CMD_IOLOC_H, ITPM_CMD_IOLOC_L, dio->ar_base);
	itpm_read2(sc, &ioloc, ITPM_CMD_IOLOC_H, ITPM_CMD_IOLOC_L);
	if (ioloc != dio->ar_base) {
		aprint_error_dev(self,
		    "Failed to set io location expected 0x%x, got 0x%x",
		    dio->ar_base, ioloc);
		goto out2;
	}
	/* Set the activation register */
	itmp_config_out(sc, ITPM_CMD_DATA_ACTIVATION_REG, ITPM_INFO_ADDR);
	itmp_config_out(sc, 1, ITPM_INFO_DATA);

	itpm_conf_write(sc, ITPM_CMD_REG_DISABLE_PAIR, ITPM_INFO_ADDR);

	itpm_data_write(sc, ITPM_CMD_RESET_LP_IRQC_DISABLE, ITPM_REG_CMD);

out2:
	bus_space_unmap(sc->sc_diot, sc->sc_dioh, dio->ar_length);
out1:
	bus_space_unmap(sc->sc_ciot, sc->sc_cioh, cio->ar_length);
out:
	acpi_resource_cleanup(&res);
}

static int
itpm_acpi_enumerate(struct itpm_acpi_softc *asc)
{
	struct itpm_softc *sc = &asc->sc_itpm;
	ACPI_OBJECT *fde;
	ACPI_BUFFER abuf;
	ACPI_STATUS rv;
	uint32_t *p;
	int i, drives = -1;

	rv = acpi_eval_struct(asc->sc_node->ad_handle, "_FDE", &abuf);

	if (ACPI_FAILURE(rv)) {
		aprint_normal_dev(sc->sc_dev, "failed to evaluate _FDE: %s\n",
		    AcpiFormatException(rv));
		return drives;
	}
	fde = abuf.Pointer;
	if (fde->Type != ACPI_TYPE_BUFFER) {
		aprint_error_dev(sc->sc_dev, "expected BUFFER, got %u\n",
		    fde->Type);
		goto out;
	}
	if (fde->Buffer.Length < 5 * sizeof(uint32_t)) {
		aprint_error_dev(sc->sc_dev,
		    "expected buffer len of %lu, got %u\n",
		    (unsigned long)(5 * sizeof(uint32_t)), fde->Buffer.Length);
		goto out;
	}

	p = (uint32_t *)fde->Buffer.Pointer;

	/*
	 * Indexes 0 through 3 are each uint32_t booleans. True if a drive
	 * is present.
	 */
	drives = 0;
	for (i = 0; i < 4; i++) {
		if (p[i]) drives |= (1 << i);
		aprint_normal_dev(sc->sc_dev, "drive %d %sattached\n", i,
		    p[i] ? "" : "not ");
	}

	/*
	 * p[4] reports tape presence. Possible values:
	 * 	0	- Unknown if device is present
	 *	1	- Device is present
	 *	2	- Device is never present
	 *	>2	- Reserved
	 *
	 * we don't currently use this.
	 */

out:
	ACPI_FREE(abuf.Pointer);
	return drives;
}


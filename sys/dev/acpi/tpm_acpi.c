/* $NetBSD: tpm_acpi.c,v 1.8 2019/06/22 12:57:40 maxv Exp $ */

/*
 * Copyright (c) 2012, 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas and Maxime Villard.
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
__KERNEL_RCSID(0, "$NetBSD: tpm_acpi.c,v 1.8 2019/06/22 12:57:40 maxv Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/pmf.h>

#include <dev/ic/tpmreg.h>
#include <dev/ic/tpmvar.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/isa/isavar.h>

#include "ioconf.h"

#define _COMPONENT	ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME	("tpm_acpi")

static int	tpm_acpi_match(device_t, cfdata_t, void *);
static void	tpm_acpi_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(tpm_acpi, sizeof(struct tpm_softc), tpm_acpi_match,
    tpm_acpi_attach, NULL, NULL);

/*
 * Supported TPM 2.0 devices.
 */
static const char * const tpm2_acpi_ids[] = {
	"MSFT0101",
	NULL
};

static int
tpm_acpi_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;
	ACPI_TABLE_TPM2 *tpm2;
	ACPI_STATUS rv;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	/* We support only one TPM. */
	if (tpm_cd.cd_devs && tpm_cd.cd_devs[0])
		return 0;

	if (!acpi_match_hid(aa->aa_node->ad_devinfo, tpm2_acpi_ids))
		return 0;

	/* Make sure it uses TIS, and not CRB. */
	rv = AcpiGetTable(ACPI_SIG_TPM2, 1, (ACPI_TABLE_HEADER **)&tpm2);
	if (ACPI_FAILURE(rv))
		return 0;
	if (tpm2->StartMethod != ACPI_TPM2_MEMORY_MAPPED)
		return 0;

	return 1;
}

static void
tpm_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct tpm_softc *sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	bus_addr_t base;
	bus_addr_t size;
	int rv, inum;

	sc->sc_dev = self;
	sc->sc_ver = TPM_2_0;

	rv = acpi_resource_parse(self, aa->aa_node->ad_handle, "_CRS", &res,
	    &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(sc->sc_dev, "cannot parse resources %d\n", rv);
		return;
	}

	mem = acpi_res_mem(&res, 0);
	if (mem == NULL) {
		aprint_error_dev(sc->sc_dev, "cannot find mem\n");
		goto out;
	}
	if (mem->ar_length != TPM_SPACE_SIZE) {
		aprint_error_dev(sc->sc_dev,
		    "wrong size mem %"PRIu64" != %u\n",
		    (uint64_t)mem->ar_length, TPM_SPACE_SIZE);
		goto out;
	}

	base = mem->ar_base;
	size = mem->ar_length;
	sc->sc_bt = aa->aa_memt;
	sc->sc_init = tpm_tis12_init;
	sc->sc_start = tpm_tis12_start;
	sc->sc_read = tpm_tis12_read;
	sc->sc_write = tpm_tis12_write;
	sc->sc_end = tpm_tis12_end;

	if (bus_space_map(sc->sc_bt, base, size, 0, &sc->sc_bh)) {
		aprint_error_dev(sc->sc_dev, "cannot map registers\n");
		goto out;
	}

	if (!tpm_tis12_probe(sc->sc_bt, sc->sc_bh)) {
		aprint_error_dev(sc->sc_dev, "TIS1.2 probe failed\n");
		goto out1;
	}

	irq = acpi_res_irq(&res, 0);
	if (irq == NULL)
		inum = -1;
	else
		inum = irq->ar_irq;

	if ((rv = (*sc->sc_init)(sc, inum)) != 0) {
		aprint_error_dev(sc->sc_dev, "cannot init device %d\n", rv);
		goto out1;
	}

	if (inum != -1 &&
	    (sc->sc_ih = isa_intr_establish(aa->aa_ic, irq->ar_irq,
	    IST_EDGE, IPL_TTY, tpm_intr, sc)) == NULL) {
		aprint_error_dev(sc->sc_dev, "cannot establish interrupt\n");
		goto out1;
	}

	acpi_resource_cleanup(&res);
	return;

out1:
	bus_space_unmap(sc->sc_bt, sc->sc_bh, size);
out:
	acpi_resource_cleanup(&res);
}

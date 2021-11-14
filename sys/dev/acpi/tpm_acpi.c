/* $NetBSD: tpm_acpi.c,v 1.14 2021/11/14 21:18:30 riastradh Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tpm_acpi.c,v 1.14 2021/11/14 21:18:30 riastradh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/pmf.h>

#include <dev/ic/tpmreg.h>
#include <dev/ic/tpmvar.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include "ioconf.h"

#define _COMPONENT	ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME	("tpm_acpi")

static int	tpm_acpi_match(device_t, cfdata_t, void *);
static void	tpm_acpi_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(tpm_acpi, sizeof(struct tpm_softc), tpm_acpi_match,
    tpm_acpi_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "PNP0C31",		.value = TPM_1_2 },
	{ .compat = "MSFT0101",		.value = TPM_2_0 },
	DEVICE_COMPAT_EOL
};

static int
tpm_acpi_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;
	const struct device_compatible_entry *dce;
	ACPI_TABLE_TPM2 *tpm2;
	ACPI_STATUS rv;
	int ret;

	/* We support only one TPM. */
	if (tpm_cd.cd_devs && tpm_cd.cd_devs[0])
		return 0;

	ret = acpi_compatible_match(aa, compat_data);
	if (ret == 0)
		return 0;

	dce = acpi_compatible_lookup(aa, compat_data);
	KASSERT(dce != NULL);

	if (dce->value == TPM_1_2) {
		/* XXX assume TPM 1.2 devices are memory-mapped. */
		return ret;
	}

	/* Make sure it uses TIS, and not CRB. */
	rv = AcpiGetTable(ACPI_SIG_TPM2, 1, (ACPI_TABLE_HEADER **)&tpm2);
	if (ACPI_FAILURE(rv))
		return 0;
	if (tpm2->StartMethod != ACPI_TPM2_MEMORY_MAPPED)
		return 0;

	return ret;
}

static void
tpm_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct tpm_softc *sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	const struct device_compatible_entry *dce;
	struct acpi_resources res;
	struct acpi_mem *mem;
	bus_addr_t base;
	bus_addr_t size;
	int rv;

	rv = acpi_resource_parse(self, aa->aa_node->ad_handle, "_CRS", &res,
	    &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self, "cannot parse resources %d\n", rv);
		return;
	}

	mem = acpi_res_mem(&res, 0);
	if (mem == NULL) {
		aprint_error_dev(self, "cannot find mem\n");
		goto out;
	}
	if (mem->ar_length < TPM_SPACE_SIZE) {
		aprint_error_dev(self, "wrong size mem %"PRIu64" < %u\n",
		    (uint64_t)mem->ar_length, TPM_SPACE_SIZE);
		goto out;
	}
	base = mem->ar_base;
	size = mem->ar_length;

	dce = acpi_compatible_lookup(aa, compat_data);
	KASSERT(dce != NULL);

	sc->sc_dev = self;
	sc->sc_ver = dce->value;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	sc->sc_busy = false;
	sc->sc_intf = &tpm_intf_tis12;
	sc->sc_bt = aa->aa_memt;
	if (bus_space_map(sc->sc_bt, base, size, 0, &sc->sc_bh)) {
		aprint_error_dev(sc->sc_dev, "cannot map registers\n");
		goto out;
	}

	if ((rv = (*sc->sc_intf->probe)(sc->sc_bt, sc->sc_bh)) != 0) {
		aprint_error_dev(sc->sc_dev, "probe failed, rv=%d\n", rv);
		goto out1;
	}
	if ((rv = (*sc->sc_intf->init)(sc)) != 0) {
		aprint_error_dev(sc->sc_dev, "cannot init device, rv=%d\n", rv);
		goto out1;
	}

	if (!pmf_device_register(self, tpm_suspend, tpm_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
	acpi_resource_cleanup(&res);
	return;

out1:
	bus_space_unmap(sc->sc_bt, sc->sc_bh, size);
out:
	acpi_resource_cleanup(&res);
}

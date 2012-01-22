/* $NetBSD: tpm_acpi.c,v 1.1 2012/01/22 06:44:28 christos Exp $ */

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
 * Copyright (c) 2008, 2009 Michael Shalayeff
 * Copyright (c) 2009, 2010 Hans-Jörg Höxer
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * ACPI attachment for the Infineon SLD 9630 TT 1.1 and SLB 9635 TT 1.2
 * trusted platform module. See www.trustedcomputinggroup.org
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tpm_acpi.c,v 1.1 2012/01/22 06:44:28 christos Exp $");

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

#define _COMPONENT          ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME            ("tpm_acpi")

static int	tpm_acpi_match(device_t, cfdata_t, void *);
static void	tpm_acpi_attach(device_t, device_t, void *);


CFATTACH_DECL_NEW(tpm_acpi, sizeof(struct tpm_softc), tpm_acpi_match,
    tpm_acpi_attach, NULL, NULL);

/*
 * Supported device IDs
 */

static const char * const tpm_acpi_ids[] = {
	"IFX0101",
	"IFX0102",
	NULL
};

extern struct cfdriver tpm_cd;

static int
tpm_acpi_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	/* There can be only one. */
	if (tpm_cd.cd_devs && tpm_cd.cd_devs[0])
		return 0;

#ifdef notyet
	// XXX: My sony can't find the IRQ
	return acpi_match_hid(aa->aa_node->ad_devinfo, tpm_acpi_ids);
#else
	return 0;
#endif
}

static void
tpm_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct tpm_softc *sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	int rv;

	sc->sc_dev = self;

        /* Parse our resources */
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

	if (mem->ar_length != TPM_SIZE) {
		aprint_error_dev(sc->sc_dev, "wrong size mem %u != %u\n",
		    mem->ar_length, TPM_SIZE);
		goto out;
	}

	sc->sc_bt = aa->aa_memt;
	sc->sc_init = tpm_tis12_init;
	sc->sc_start = tpm_tis12_start;
	sc->sc_read = tpm_tis12_read;
	sc->sc_write = tpm_tis12_write;
	sc->sc_end = tpm_tis12_end;

	if (bus_space_map(sc->sc_bt, mem->ar_base, mem->ar_length, 0,
	    &sc->sc_bh)) {
		aprint_error_dev(sc->sc_dev, "cannot map registers\n");
		goto out;
	}

	irq = acpi_res_irq(&res, 0);
	if (irq == NULL) {
		aprint_error_dev(sc->sc_dev, "cannot find irq\n");
		goto out1;
	}

	if ((rv = (*sc->sc_init)(sc, irq->ar_irq,
	    device_xname(sc->sc_dev))) != 0)
		aprint_error_dev(sc->sc_dev, "cannot init device %d\n", rv);
		goto out1;

	if ((sc->sc_ih = isa_intr_establish(aa->aa_ic, irq->ar_irq,
	    IST_EDGE, IPL_TTY, tpm_intr, sc)) == NULL) {
		aprint_error_dev(sc->sc_dev, "cannot establish interrupt\n");
		goto out1;
	}

	if (!pmf_device_register(sc->sc_dev, tpm_suspend, tpm_resume))
		aprint_error_dev(sc->sc_dev, "Cannot set power mgmt handler\n");
	return;
out1:
	bus_space_unmap(sc->sc_bt, sc->sc_bh, TPM_SIZE);
out:
	acpi_resource_cleanup(&res);
}

/*	$NetBSD: bcm2835_mbox_acpi.c,v 1.2 2020/02/22 22:09:07 jmcneill Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_mbox_acpi.c,v 1.2 2020/02/22 22:09:07 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/timetc.h>
#include <sys/bus.h>
#include <sys/mutex.h>

#include <arm/broadcom/bcm2835_mbox.h>
#include <arm/broadcom/bcm2835_mboxreg.h>
#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835var.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>
#include <arch/evbarm/rpi/vcprop.h>
#include <arch/evbarm/rpi/vcio.h>
#include <arch/evbarm/rpi/vcpm.h>

static int bcmmbox_acpi_match(device_t, cfdata_t, void *);
static void bcmmbox_acpi_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(bcmmbox_acpi, sizeof(struct bcm2835mbox_softc),
    bcmmbox_acpi_match, bcmmbox_acpi_attach, NULL, NULL);

static int
bcmmbox_acpi_match(device_t parent, cfdata_t match, void *aux)
{
	const char * const acpi_compatible[] = {
		"BCM2849",
		NULL
	};
	struct acpi_attach_args * const aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, acpi_compatible);
}

static void
bcmmbox_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct bcm2835mbox_softc *sc = device_private(self);
	struct acpi_attach_args * const aa = aux;
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
	if (mem == NULL || irq == NULL) {
		aprint_error_dev(self, "couldn't find resources\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_memt;
	sc->sc_dmat = aa->aa_dmat;

	if (bus_space_map(sc->sc_iot, mem->ar_base, mem->ar_length, 0, &sc->sc_ioh) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

#if notyet
	sc->sc_intrh = acpi_intr_establish(self, (uint64_t)aa->aa_node->ad_handle,
	    IPL_VM, false, bcmmbox_intr, sc, device_xname(self));
	if (sc->sc_intrh == NULL) {
		aprint_error_dev(self, "failed to establish interrupt\n");
		return;
	}
#endif

	bcmmbox_attach(sc);
}

/* $NetBSD: vchiq_netbsd_acpi.c,v 1.5 2021/08/07 16:19:18 thorpej Exp $ */

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared D. McNeill
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
__KERNEL_RCSID(0, "$NetBSD: vchiq_netbsd_acpi.c,v 1.5 2021/08/07 16:19:18 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/sysctl.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835_intr.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>

#include "vchiq_arm.h"
#include "vchiq_2835.h"
#include "vchiq_netbsd.h"

struct vchiq_acpi_softc {
	struct vchiq_softc sc_vchiq;
	ACPI_HANDLE sc_handle;
};

static int vchiq_acpi_match(device_t, cfdata_t, void *);
static void vchiq_acpi_attach(device_t, device_t, void *);

static void vchiq_acpi_defer(device_t);

/* External functions */
int vchiq_init(void);


CFATTACH_DECL_NEW(vchiq_acpi, sizeof(struct vchiq_acpi_softc),
    vchiq_acpi_match, vchiq_acpi_attach, NULL, NULL);

static int
vchiq_acpi_match(device_t parent, cfdata_t match, void *aux)
{
	const char * const compatible[] = { "BCM2835", NULL };
	struct acpi_attach_args * const aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, compatible);
}

static void
vchiq_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct vchiq_acpi_softc *asc = device_private(self);
	struct vchiq_softc *sc = &asc->sc_vchiq;
	struct acpi_attach_args * const aa = aux;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	ACPI_STATUS rv;

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_memt;
	asc->sc_handle = aa->aa_node->ad_handle;

#if BYTE_ORDER == BIG_ENDIAN
	aprint_error_dev(sc->sc_dev, "not supported yet in big-endian mode\n");
	return;
#endif

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

	if (bus_space_map(sc->sc_iot, mem->ar_base, mem->ar_length, 0,
	    &sc->sc_ioh) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	vchiq_platform_attach(aa->aa_dmat);

	vchiq_set_softc(sc);

	config_mountroot(self, vchiq_acpi_defer);
}

static void
vchiq_acpi_defer(device_t self)
{
	struct vchiq_attach_args vaa;
	struct vchiq_acpi_softc *asc = device_private(self);
	struct vchiq_softc *sc = &asc->sc_vchiq;
	ACPI_HANDLE handle = asc->sc_handle;

	vchiq_core_initialize();

	sc->sc_ih = acpi_intr_establish(sc->sc_dev, (uint64_t)handle,
	    IPL_VM, true, vchiq_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(sc->sc_dev, "failed to establish interrupt\n");
		return;
	}

	vchiq_init();

	vaa.vaa_name = "AUDS";
	config_found(self, &vaa, vchiq_print, CFARGS_NONE);
}

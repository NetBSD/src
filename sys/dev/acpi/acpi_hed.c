/*	$NetBSD: acpi_hed.c,v 1.1.4.2 2024/10/09 13:00:11 martin Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 * HED: Hardware Error Device, PNP0C33.
 *
 * This device serves only to receive notifications about hardware
 * errors, which we then dispatch to apei(4).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_hed.c,v 1.1.4.2 2024/10/09 13:00:11 martin Exp $");

#include <sys/device.h>
#include <sys/module.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/apei_hed.h>

#define	_COMPONENT		ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME		("acpi_hed")

struct acpihed_softc {
	device_t		sc_dev;
	struct acpi_devnode	*sc_node;
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "PNP0C33" },
	DEVICE_COMPAT_EOL
};

static int acpihed_match(device_t, cfdata_t, void *);
static void acpihed_attach(device_t, device_t, void *);
static int acpihed_detach(device_t, int);
static void acpihed_notify(ACPI_HANDLE, uint32_t, void *);

CFATTACH_DECL_NEW(acpihed, sizeof(struct acpihed_softc),
    acpihed_match, acpihed_attach, acpihed_detach, NULL);

static int
acpihed_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

static void
acpihed_attach(device_t parent, device_t self, void *aux)
{
	struct acpihed_softc *sc = device_private(self);
	struct acpi_attach_args *aa = aux;

	aprint_naive("\n");
	aprint_normal(": ACPI Hardware Error Device\n");

	pmf_device_register(self, NULL, NULL);

	sc->sc_dev = self;
	sc->sc_node = aa->aa_node;

	acpi_register_notify(sc->sc_node, acpihed_notify);
}

static int
acpihed_detach(device_t self, int flags)
{
	struct acpihed_softc *sc = device_private(self);
	int error;

	error = config_detach_children(self, flags);
	if (error)
		return error;

	acpi_deregister_notify(sc->sc_node);

	pmf_device_deregister(self);

	return 0;
}

static void
acpihed_notify(ACPI_HANDLE handle, uint32_t event, void *cookie)
{

	apei_hed_notify();
}

MODULE(MODULE_CLASS_DRIVER, acpihed, "apei");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
acpihed_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_acpihed,
		    cfattach_ioconf_acpihed, cfdata_ioconf_acpihed);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_acpihed,
		    cfattach_ioconf_acpihed, cfdata_ioconf_acpihed);
#endif
		return error;
	default:
		return ENOTTY;
	}
}

/*	$NetBSD: sni_i2c.c,v 1.2 2020/03/18 05:02:48 nisimura Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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
 * Socionext SC2A11 SynQuacer I2C driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sni_i2c.c,v 1.2 2020/03/18 05:02:48 nisimura Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/intr.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <machine/endian.h>

#include <dev/fdt/fdtvar.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

static int sniiic_fdt_match(device_t, struct cfdata *, void *);
static void sniiic_fdt_attach(device_t, device_t, void *);
static int sniiic_acpi_match(device_t, struct cfdata *, void *);
static void sniiic_acpi_attach(device_t, device_t, void *);

struct sniiic_softc {
	device_t sc_dev;
};

CFATTACH_DECL_NEW(sniiic_fdt, sizeof(struct sniiic_softc),
    sniiic_fdt_match, sniiic_fdt_attach, NULL, NULL);

CFATTACH_DECL_NEW(sniiic_acpi, sizeof(struct sniiic_softc),
    sniiic_acpi_match, sniiic_acpi_attach, NULL, NULL);

static int sniiic_attach_i(struct sniiic_softc *);

static int
sniiic_fdt_match(device_t parent, struct cfdata *match, void *aux)
{
	static const char * compatible[] = {
		"socionext,synquacer-i2c",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sniiic_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct sniiic_softc * const sc = device_private(self);
	int error;

	error = sniiic_attach_i(sc);
	(void)error;
}

static int
sniiic_acpi_match(device_t parent, struct cfdata *match, void *aux)
{
	static const char * compatible[] = {
		"SCX0003",
		NULL
	};
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;
	return acpi_match_hid(aa->aa_node->ad_devinfo, compatible);
}

static void
sniiic_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct sniiic_softc * const sc = device_private(self);
	int error;

	error = sniiic_attach_i(sc);
	(void)error;
}

static int
sniiic_attach_i(struct sniiic_softc *sc)
{
	return 0;
}

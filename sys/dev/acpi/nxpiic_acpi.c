/* $NetBSD: nxpiic_acpi.c,v 1.1 2021/01/24 18:02:51 jmcneill Exp $ */

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared McNeill <jmcneill@invisible.ca>.
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
__KERNEL_RCSID(0, "$NetBSD: nxpiic_acpi.c,v 1.1 2021/01/24 18:02:51 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>
#include <dev/acpi/acpi_i2c.h>

#include <dev/i2c/motoi2cvar.h>
#include <dev/i2c/motoi2creg.h>

struct nxpiic_softc {
	device_t		sc_dev;
	struct motoi2c_softc	sc_motoi2c;
};

static int	nxpiic_acpi_match(device_t, cfdata_t, void *);
static void	nxpiic_acpi_attach(device_t, device_t, void *);

static uint8_t	nxpiic_acpi_iord(struct motoi2c_softc *, bus_size_t);
static void	nxpiic_acpi_iowr(struct motoi2c_softc *, bus_size_t, uint8_t);

CFATTACH_DECL_NEW(nxpiic_acpi, sizeof(struct nxpiic_softc),
    nxpiic_acpi_match, nxpiic_acpi_attach, NULL, NULL);

static const char * const compatible[] = {
	"NXP0001",
	NULL
};

static int
nxpiic_acpi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, compatible);
}

static void
nxpiic_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct nxpiic_softc * const sc = device_private(self);
	struct motoi2c_softc * const msc = &sc->sc_motoi2c;
	struct acpi_attach_args *aa = aux;
	struct acpi_resources res;
	struct acpi_mem *mem;
	ACPI_INTEGER clock_freq;
	ACPI_STATUS rv;
	int error;

	sc->sc_dev = self;
	msc->sc_iot = aa->aa_memt;

	rv = acpi_resource_parse(sc->sc_dev, aa->aa_node->ad_handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	mem = acpi_res_mem(&res, 0);
	if (mem == NULL) {
		aprint_error_dev(self, "couldn't find mem resource\n");
		goto done;
	}

	rv = acpi_dsd_integer(aa->aa_node->ad_handle, "clock-frequency",
	    &clock_freq);
	if (ACPI_FAILURE(rv) || clock_freq == 0) {
		aprint_error_dev(self, "couldn't get clock frequency\n");
		goto done;
	}

	error = bus_space_map(msc->sc_iot, mem->ar_base, mem->ar_length, 0,
	    &msc->sc_ioh);
	if (error) {
		aprint_error_dev(self, "couldn't map registers\n");
		return;
	}

	msc->sc_iord = nxpiic_acpi_iord;
	msc->sc_iowr = nxpiic_acpi_iowr;
	msc->sc_child_devices = acpi_enter_i2c_devs(aa->aa_node);

	motoi2c_attach_common(self, msc, NULL);

done:
	acpi_resource_cleanup(&res);

}

static uint8_t
nxpiic_acpi_iord(struct motoi2c_softc *msc, bus_size_t off)
{
	KASSERT((off & 3) == 0);

	if (off >= I2CDFSRR)
		return 0;

	return bus_space_read_1(msc->sc_iot, msc->sc_ioh, off >> 2);
}

static void
nxpiic_acpi_iowr(struct motoi2c_softc *msc, bus_size_t off, uint8_t val)
{
	KASSERT((off & 3) == 0);

	if (off >= I2CDFSRR)
		return;

	bus_space_write_1(msc->sc_iot, msc->sc_ioh, off >> 2, val);
}

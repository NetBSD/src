/* $NetBSD: nxpiic_acpi.c,v 1.5 2022/07/22 23:43:23 thorpej Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: nxpiic_acpi.c,v 1.5 2022/07/22 23:43:23 thorpej Exp $");

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

#define	NXPIIC_SPEED_STD	100000

static const struct clk_div {
	int scl_div;
	uint8_t ibc;
} nxpiic_clk_div[] = {
	{ 20, 0x00 },	{ 22, 0x01 },	{ 24, 0x02 },	{ 26, 0x03 },
	{ 28, 0x04 },	{ 30, 0x05 },	{ 32, 0x09 },	{ 34, 0x06 },
	{ 36, 0x0a },	{ 40, 0x07 },	{ 44, 0x0c },	{ 48, 0x0d },
	{ 52, 0x43 },	{ 56, 0x0e },	{ 60, 0x45 },	{ 64, 0x12 },
	{ 68, 0x0f },	{ 72, 0x13 },	{ 80, 0x14 },	{ 88, 0x15 },
	{ 96, 0x19 },	{ 104, 0x16 },	{ 112, 0x1a },	{ 128, 0x17 },
	{ 136, 0x4f },	{ 144, 0x1c },	{ 160, 0x1d },	{ 176, 0x55 },
	{ 192, 0x1e },	{ 208, 0x56 },	{ 224, 0x22 },	{ 228, 0x24 },
	{ 240, 0x1f },	{ 256, 0x23 },	{ 288, 0x5c },	{ 320, 0x25 },
	{ 384, 0x26 },	{ 448, 0x2a },	{ 480, 0x27 },	{ 512, 0x2b },
	{ 576, 0x2c },	{ 640, 0x2d },	{ 768, 0x31 },	{ 896, 0x32 },
	{ 960, 0x2f },	{ 1024, 0x33 },	{ 1152, 0x34 },	{ 1280, 0x35 },
	{ 1536, 0x36 },	{ 1792, 0x3a },	{ 1920, 0x37 },	{ 2048, 0x3b },
	{ 2304, 0x3c },	{ 2560, 0x3d },	{ 3072, 0x3e },	{ 3584, 0x7a },
	{ 3840, 0x3f },	{ 4096, 0x7B },	{ 5120, 0x7d },	{ 6144, 0x7e },
};

static int	nxpiic_acpi_match(device_t, cfdata_t, void *);
static void	nxpiic_acpi_attach(device_t, device_t, void *);

static uint8_t	nxpiic_acpi_iord(struct motoi2c_softc *, bus_size_t);
static void	nxpiic_acpi_iowr(struct motoi2c_softc *, bus_size_t, uint8_t);

CFATTACH_DECL_NEW(nxpiic_acpi, sizeof(struct motoi2c_softc),
    nxpiic_acpi_match, nxpiic_acpi_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "NXP0001" },
	DEVICE_COMPAT_EOL
};

static int
nxpiic_acpi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

static void
nxpiic_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct motoi2c_softc * const sc = device_private(self);
	struct motoi2c_settings settings;
	struct acpi_attach_args *aa = aux;
	struct acpi_resources res;
	struct acpi_mem *mem;
	ACPI_INTEGER clock_freq;
	ACPI_STATUS rv;
	int error, n;

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_memt;

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
	aprint_debug_dev(self, "bus clock %u Hz\n", (u_int)clock_freq);

	error = bus_space_map(sc->sc_iot, mem->ar_base, mem->ar_length, 0,
	    &sc->sc_ioh);
	if (error) {
		aprint_error_dev(self, "couldn't map registers\n");
		return;
	}

	settings.i2c_adr = MOTOI2C_ADR_DEFAULT;
	settings.i2c_dfsrr = MOTOI2C_DFSRR_DEFAULT;
	for (n = 0; n < __arraycount(nxpiic_clk_div) - 1; n++) {
		if (clock_freq / nxpiic_clk_div[n].scl_div < NXPIIC_SPEED_STD)
			break;
	}
	settings.i2c_fdr = nxpiic_clk_div[n].ibc;

	sc->sc_flags |= MOTOI2C_F_ENABLE_INV | MOTOI2C_F_STATUS_W1C;
	sc->sc_iord = nxpiic_acpi_iord;
	sc->sc_iowr = nxpiic_acpi_iowr;
	sc->sc_child_devices = acpi_enter_i2c_devs(self, aa->aa_node);

	motoi2c_attach(sc, &settings);

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

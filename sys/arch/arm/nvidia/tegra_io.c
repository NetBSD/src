/* $NetBSD: tegra_io.c,v 1.14 2015/10/17 21:18:16 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "opt_tegra.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_io.c,v 1.14 2015/10/17 21:18:16 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <sys/bus.h>

#include <arm/mainbus/mainbus.h>
#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_var.h>

#include "locators.h"

static int	tegraio_match(device_t, cfdata_t, void *);
static void	tegraio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(tegra_io, 0,
    tegraio_match, tegraio_attach, NULL, NULL);

static int	tegraio_print(void *, const char *);
static int	tegraio_find(device_t, cfdata_t, const int *, void *);

static void	tegraio_scan(device_t, bus_space_handle_t,
			     const struct tegra_locators *, u_int);

static bool tegraio_found = false;

#define NOPORT	TEGRAIOCF_PORT_DEFAULT
#define NOINTR	TEGRAIO_INTR_DEFAULT

static const struct tegra_locators tegra_ppsb_locators[] = {
  { "tegracar",
    TEGRA_CAR_OFFSET, TEGRA_CAR_SIZE, NOPORT, NOINTR },
  { "tegragpio",
    TEGRA_GPIO_OFFSET, TEGRA_GPIO_SIZE, NOPORT, NOINTR },
  { "tegratimer",
    TEGRA_TIMER_OFFSET, TEGRA_TIMER_SIZE, NOPORT, NOINTR },
};

static const struct tegra_locators tegra_apb_locators[] = {
  { "tegramc",
    TEGRA_MC_OFFSET, TEGRA_MC_SIZE, NOPORT, NOINTR },
  { "tegrapmc",
    TEGRA_PMC_OFFSET, TEGRA_PMC_SIZE, NOPORT, NOINTR },
  { "tegraxusbpad",
    TEGRA_XUSB_PADCTL_OFFSET, TEGRA_XUSB_PADCTL_SIZE, NOPORT, NOINTR },
  { "tegrampio",
    TEGRA_MPIO_OFFSET, TEGRA_MPIO_SIZE, NOPORT, NOINTR },
  { "tegrai2c",
    TEGRA_I2C1_OFFSET, TEGRA_I2C1_SIZE, 0, TEGRA_INTR_I2C1 },
  { "tegrai2c",
    TEGRA_I2C2_OFFSET, TEGRA_I2C2_SIZE, 1, TEGRA_INTR_I2C2 },
  { "tegrai2c",
    TEGRA_I2C3_OFFSET, TEGRA_I2C3_SIZE, 2, TEGRA_INTR_I2C3 },
  { "tegrai2c",
    TEGRA_I2C4_OFFSET, TEGRA_I2C4_SIZE, 3, TEGRA_INTR_I2C4 },
  { "tegrai2c",
    TEGRA_I2C5_OFFSET, TEGRA_I2C5_SIZE, 4, TEGRA_INTR_I2C5 },
  { "tegrai2c",
    TEGRA_I2C6_OFFSET, TEGRA_I2C6_SIZE, 5, TEGRA_INTR_I2C6 },
  { "com",
    TEGRA_UARTA_OFFSET, TEGRA_UARTA_SIZE, 0, TEGRA_INTR_UARTA },
  { "com",
    TEGRA_UARTB_OFFSET, TEGRA_UARTB_SIZE, 1, TEGRA_INTR_UARTB },
  { "com",
    TEGRA_UARTC_OFFSET, TEGRA_UARTC_SIZE, 2, TEGRA_INTR_UARTC },
  { "com",
    TEGRA_UARTD_OFFSET, TEGRA_UARTD_SIZE, 3, TEGRA_INTR_UARTD },
  { "tegrartc",
    TEGRA_RTC_OFFSET, TEGRA_RTC_SIZE, NOPORT, NOINTR },
  { "sdhc",
    TEGRA_SDMMC1_OFFSET, TEGRA_SDMMC1_SIZE, 0, TEGRA_INTR_SDMMC1 },
  { "sdhc",
    TEGRA_SDMMC2_OFFSET, TEGRA_SDMMC2_SIZE, 1, TEGRA_INTR_SDMMC2 },
  { "sdhc",
    TEGRA_SDMMC3_OFFSET, TEGRA_SDMMC3_SIZE, 2, TEGRA_INTR_SDMMC3 },
  { "sdhc",
    TEGRA_SDMMC4_OFFSET, TEGRA_SDMMC4_SIZE, 3, TEGRA_INTR_SDMMC4 },
  { "ahcisata",
    TEGRA_SATA_OFFSET, TEGRA_SATA_SIZE, NOPORT, TEGRA_INTR_SATA },
  { "hdaudio",
    TEGRA_HDA_OFFSET, TEGRA_HDA_SIZE, NOPORT, TEGRA_INTR_HDA },
  { "tegracec",
    TEGRA_CEC_OFFSET, TEGRA_CEC_SIZE, NOPORT, TEGRA_INTR_CEC },
};

static const struct tegra_locators tegra_ahb_a2_locators[] = {
  { "ehci",
    TEGRA_USB1_OFFSET, TEGRA_USB1_SIZE, 0, TEGRA_INTR_USB1 },
  { "ehci",
    TEGRA_USB2_OFFSET, TEGRA_USB2_SIZE, 1, TEGRA_INTR_USB2 },
  { "ehci",
    TEGRA_USB3_OFFSET, TEGRA_USB3_SIZE, 2, TEGRA_INTR_USB3 },
};

static const struct tegra_locators tegra_pcie_locators[] = {
  { "tegrapcie",
    TEGRA_PCIE_OFFSET, TEGRA_PCIE_SIZE, NOPORT, TEGRA_INTR_PCIE_INT },
};

static const struct tegra_locators tegra_host1x_locators[] = {
  { "tegrahost1x", 0, TEGRA_HOST1X_SIZE, NOPORT, NOINTR }
};

static const struct tegra_locators tegra_ghost_locators[] = {
  { "tegradc",
    TEGRA_DISPLAYA_OFFSET, TEGRA_DISPLAYA_SIZE, 0, TEGRA_INTR_DISPLAYA },
  { "tegradc",
    TEGRA_DISPLAYB_OFFSET, TEGRA_DISPLAYB_SIZE, 1, TEGRA_INTR_DISPLAYB },
  { "tegrahdmi",
    TEGRA_HDMI_OFFSET, TEGRA_HDMI_SIZE, NOPORT, TEGRA_INTR_HDMI },
};

static const struct tegra_locators tegra_gpu_locators[] = {
  { "nouveau", 0, TEGRA_GPU_SIZE, NOPORT, NOINTR }
};

int
tegraio_match(device_t parent, cfdata_t cf, void *aux)
{
	if (tegraio_found)
		return 0;
	return 1;
}

void
tegraio_attach(device_t parent, device_t self, void *aux)
{
	tegraio_found = true;

	aprint_naive("\n");
	aprint_normal(": %s\n", tegra_chip_name());

	tegraio_scan(self, tegra_ppsb_bsh,
	    tegra_ppsb_locators, __arraycount(tegra_ppsb_locators));
	tegraio_scan(self, tegra_apb_bsh,
	    tegra_apb_locators, __arraycount(tegra_apb_locators));
	tegraio_scan(self, tegra_ahb_a2_bsh,
	    tegra_ahb_a2_locators, __arraycount(tegra_ahb_a2_locators));
	tegraio_scan(self, (bus_space_handle_t)NULL,
	    tegra_host1x_locators, __arraycount(tegra_host1x_locators));
	tegraio_scan(self, (bus_space_handle_t)NULL,
	    tegra_ghost_locators, __arraycount(tegra_ghost_locators));
	tegraio_scan(self, (bus_space_handle_t)NULL,
	    tegra_gpu_locators, __arraycount(tegra_gpu_locators));
	tegraio_scan(self, (bus_space_handle_t)NULL,
	    tegra_pcie_locators, __arraycount(tegra_pcie_locators));
}

static void
tegraio_scan(device_t self, bus_space_handle_t bsh,
    const struct tegra_locators *locators, u_int count)
{
	const struct tegra_locators * const eloc = locators + count;
	for (const struct tegra_locators *loc = locators; loc < eloc; loc++) {
		struct tegraio_attach_args tio = {
			.tio_loc = *loc,
			.tio_bst = &armv7_generic_bs_tag,
			.tio_a4x_bst = &armv7_generic_a4x_bs_tag,
			.tio_bsh = bsh,
			.tio_dmat = &tegra_dma_tag,
			.tio_coherent_dmat = &tegra_coherent_dma_tag,
		};
		cfdata_t cf = config_search_ia(tegraio_find, self,
		    "tegraio", &tio);
		if (cf != NULL)
			config_attach(self, cf, &tio, tegraio_print);
	}
}

int
tegraio_print(void *aux, const char *pnp)
{
	const struct tegraio_attach_args * const tio = aux;

	if (tio->tio_loc.loc_port != TEGRAIOCF_PORT_DEFAULT)
		aprint_normal(" port %d", tio->tio_loc.loc_port);

	return UNCONF;
}

static int
tegraio_find(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	const struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;
	const int port = cf->cf_loc[TEGRAIOCF_PORT];

	if (strcmp(cf->cf_name, loc->loc_name)
	    || (port != TEGRAIOCF_PORT_DEFAULT && port != loc->loc_port))
		return 0;

	return config_match(parent, cf, aux);
}

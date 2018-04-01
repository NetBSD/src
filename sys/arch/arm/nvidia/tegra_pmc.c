/* $NetBSD: tegra_pmc.c,v 1.12 2018/04/01 04:35:04 ryo Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_pmc.c,v 1.12 2018/04/01 04:35:04 ryo Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_pmcreg.h>
#include <arm/nvidia/tegra_var.h>

#include <dev/fdt/fdtvar.h>

static int	tegra_pmc_match(device_t, cfdata_t, void *);
static void	tegra_pmc_attach(device_t, device_t, void *);

struct tegra_pmc_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
};

static struct tegra_pmc_softc *pmc_softc = NULL;

CFATTACH_DECL_NEW(tegra_pmc, sizeof(struct tegra_pmc_softc),
	tegra_pmc_match, tegra_pmc_attach, NULL, NULL);

static int
tegra_pmc_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = {
		"nvidia,tegra210-pmc",
		"nvidia,tegra124-pmc",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_pmc_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_pmc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}

	KASSERT(pmc_softc == NULL);
	pmc_softc = sc;

	aprint_naive("\n");
	aprint_normal(": PMC\n");
}

static void
tegra_pmc_get_bs(bus_space_tag_t *pbst, bus_space_handle_t *pbsh)
{
	if (pmc_softc) {
		*pbst = pmc_softc->sc_bst;
		*pbsh = pmc_softc->sc_bsh;
	} else {
		extern struct bus_space arm_generic_bs_tag;

		*pbst = &arm_generic_bs_tag;

		bus_space_subregion(*pbst, tegra_apb_bsh,
		    TEGRA_PMC_OFFSET, TEGRA_PMC_SIZE, pbsh);
	}
}

void
tegra_pmc_reset(void)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	uint32_t cntrl;

	tegra_pmc_get_bs(&bst, &bsh);

	cntrl = bus_space_read_4(bst, bsh, PMC_CNTRL_0_REG);
	cntrl |= PMC_CNTRL_0_MAIN_RST;
	bus_space_write_4(bst, bsh, PMC_CNTRL_0_REG, cntrl);

	for (;;) {
		__asm("wfi");
	}
}

void
tegra_pmc_power(u_int partid, bool enable)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	uint32_t status, toggle;
	bool state;
	int retry = 10000;

	tegra_pmc_get_bs(&bst, &bsh);

	status = bus_space_read_4(bst, bsh, PMC_PWRGATE_STATUS_0_REG);
	state = !!(status & __BIT(partid));
	if (state == enable)
		return;

	while (--retry > 0) {
		toggle = bus_space_read_4(bst, bsh, PMC_PWRGATE_TOGGLE_0_REG);
		if ((toggle & PMC_PWRGATE_TOGGLE_0_START) == 0)
			break;
		delay(1);
	}
	if (retry == 0) {
		printf("ERROR: Couldn't enable PMC partition %#x\n", partid);
		return;
	}

	bus_space_write_4(bst, bsh, PMC_PWRGATE_TOGGLE_0_REG,
	    __SHIFTIN(partid, PMC_PWRGATE_TOGGLE_0_PARTID) |
	    PMC_PWRGATE_TOGGLE_0_START);
}

void
tegra_pmc_remove_clamping(u_int partid)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;

	tegra_pmc_get_bs(&bst, &bsh);

	if (partid == PMC_PARTID_TD) {
		/*
		 * On Tegra124 and later, the GPU power clamping is
		 * controlled by a separate register
		 */
		bus_space_write_4(bst, bsh, PMC_GPU_RG_CNTRL_REG, 0);
		return;
	}

	bus_space_write_4(bst, bsh, PMC_REMOVE_CLAMPING_CMD_0_REG,
	    __BIT(partid));
}

void
tegra_pmc_hdmi_enable(void)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;

	tegra_pmc_get_bs(&bst, &bsh);

	tegra_reg_set_clear(bst, bsh, PMC_IO_DPD_STATUS_REG,
	    0, PMC_IO_DPD_STATUS_HDMI);
	tegra_reg_set_clear(bst, bsh, PMC_IO_DPD2_STATUS_REG,
	    0, PMC_IO_DPD2_STATUS_HV);
}

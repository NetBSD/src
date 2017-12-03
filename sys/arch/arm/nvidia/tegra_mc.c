/* $NetBSD: tegra_mc.c,v 1.7.10.2 2017/12/03 11:35:54 jdolecek Exp $ */

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

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_mc.c,v 1.7.10.2 2017/12/03 11:35:54 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_mcreg.h>
#include <arm/nvidia/tegra_var.h>

#include <dev/fdt/fdtvar.h>

static int	tegra_mc_match(device_t, cfdata_t, void *);
static void	tegra_mc_attach(device_t, device_t, void *);

static int	tegra_mc_intr(void *);

struct tegra_mc_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	void			*sc_ih;
};

static struct tegra_mc_softc *mc_softc = NULL;

CFATTACH_DECL_NEW(tegra_mc, sizeof(struct tegra_mc_softc),
	tegra_mc_match, tegra_mc_attach, NULL, NULL);

#define MC_READ(sc, reg)		\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define MC_WRITE(sc, reg, val)		\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define MC_SET_CLEAR(sc, reg, set, clr)	\
    tegra_reg_set_clear((sc)->sc_bst, (sc)->sc_bsh, (reg), (set), (clr))

static int
tegra_mc_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "nvidia,tegra124-mc", NULL };
	struct fdt_attach_args * const faa = aux;
	
	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_mc_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_mc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	char intrstr[128];
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

	KASSERT(mc_softc == NULL);
	mc_softc = sc;

	aprint_naive("\n");
	aprint_normal(": MC\n");

	if (!fdtbus_intr_str(faa->faa_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(faa->faa_phandle, 0, IPL_VM,
	    FDT_INTR_MPSAFE, tegra_mc_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	MC_WRITE(sc, MC_INTSTATUS_REG, MC_INT__ALL);
	MC_WRITE(sc, MC_INTMASK_REG, MC_INT__ALL);
}

static int
tegra_mc_intr(void *v)
{
	struct tegra_mc_softc * const sc = v;

	const uint32_t status = MC_READ(sc, MC_INTSTATUS_REG);

	if (status == 0) {
		return 0;
	}

	const uint32_t err_status = MC_READ(sc, MC_ERR_STATUS_REG);
	const uint32_t err_adr = MC_READ(sc, MC_ERR_ADR_REG);

	device_printf(sc->sc_dev, "intrstatus %#x err %#x adr %#x\n",
	    status, err_status, err_adr);

	MC_WRITE(sc, MC_INTSTATUS_REG, status);

	return status;
}

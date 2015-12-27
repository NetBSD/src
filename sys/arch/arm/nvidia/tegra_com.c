/* $NetBSD: tegra_com.c,v 1.1.2.4 2015/12/27 12:09:31 skrll Exp $ */

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

__KERNEL_RCSID(1, "$NetBSD: tegra_com.c,v 1.1.2.4 2015/12/27 12:09:31 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/termios.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_var.h>

#include <dev/ic/comvar.h>

#include <dev/fdt/fdtvar.h>

static int tegra_com_match(device_t, cfdata_t, void *);
static void tegra_com_attach(device_t, device_t, void *);

struct tegra_com_softc {
	struct com_softc tsc_sc;
	void *tsc_ih;

	struct clk *tsc_clk;
	struct fdtbus_reset *tsc_rst;
};

CFATTACH_DECL_NEW(tegra_com, sizeof(struct tegra_com_softc),
	tegra_com_match, tegra_com_attach, NULL, NULL);

static int
tegra_com_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "nvidia,tegra124-uart", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_com_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_com_softc * const tsc = device_private(self);
	struct com_softc * const sc = &tsc->tsc_sc;
	struct fdt_attach_args * const faa = aux;
	bus_space_handle_t bsh;
	bus_space_tag_t bst;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	u_int reg_shift;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (of_getprop_uint32(faa->faa_phandle, "reg-shift", &reg_shift)) {
		/* missing or bad reg-shift property, assume 2 */
		bst = faa->faa_a4x_bst;
	} else {
		if (reg_shift == 2) {
			bst = faa->faa_a4x_bst;
		} else if (reg_shift == 0) {
			bst = faa->faa_bst;
		} else {
			aprint_error(": unsupported reg-shift value %d\n",
			    reg_shift);
			return;
		}
	}

	sc->sc_dev = self;

	tsc->tsc_clk = fdtbus_clock_get_index(faa->faa_phandle, 0);
	tsc->tsc_rst = fdtbus_reset_get(faa->faa_phandle, "serial");

	if (tsc->tsc_clk == NULL) {
		aprint_error(": couldn't get frequency\n");
		return;
	}

	sc->sc_frequency = clk_get_rate(tsc->tsc_clk);
	sc->sc_type = COM_TYPE_TEGRA;

	error = bus_space_map(bst, addr, size, 0, &bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}

	COM_INIT_REGS(sc->sc_regs, bst, bsh, addr);

	com_attach_subr(sc);
	aprint_naive("\n");

	if (!fdtbus_intr_str(faa->faa_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	tsc->tsc_ih = fdtbus_intr_establish(faa->faa_phandle, 0, IPL_SERIAL,
	    FDT_INTR_MPSAFE, comintr, sc);
	if (tsc->tsc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
}

/* $NetBSD: dwiic_fdt.c,v 1.1.2.2 2018/09/30 01:45:49 pgoyette Exp $ */

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Manuel Bouyer.
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
 * Synopsys DesignWare I2C controller, FDT front-end
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dwiic_fdt.c,v 1.1.2.2 2018/09/30 01:45:49 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>
#include <dev/ic/dwiic_var.h>

static const char * const compatible[] = {
	"snps,designware-i2c",
	NULL
};

struct dwiic_fdt_softc {
	struct dwiic_softc	sc_dwiic;
};

static int	dwiic_fdt_match(device_t, cfdata_t, void *);
static void	dwiic_fdt_attach(device_t, device_t, void *);
static i2c_tag_t dwiic_fdt_i2c_get_tag(device_t);

struct fdtbus_i2c_controller_func dwiic_fdt_i2c_funcs = {
	.get_tag = dwiic_fdt_i2c_get_tag,
};

CFATTACH_DECL_NEW(dwiic_fdt, sizeof(struct dwiic_fdt_softc),
    dwiic_fdt_match, dwiic_fdt_attach, dwiic_detach, NULL);

int
dwiic_fdt_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

void
dwiic_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct dwiic_fdt_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	char intrstr[128];

	sc->sc_dwiic.sc_dev = self;
	sc->sc_dwiic.sc_power = NULL;
	sc->sc_dwiic.sc_type = dwiic_type_generic;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dwiic.sc_iot = faa->faa_bst;
	if (bus_space_map(sc->sc_dwiic.sc_iot, addr, size, 0, &sc->sc_dwiic.sc_ioh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive(": I2C controller\n");
	aprint_normal(": I2C controller\n");

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	sc->sc_dwiic.sc_ih = fdtbus_intr_establish(phandle, 0, IPL_VM, 0,
		dwiic_intr, &sc->sc_dwiic);
	if (sc->sc_dwiic.sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		goto out;
	}

	dwiic_attach(&sc->sc_dwiic);

	pmf_device_register(self, dwiic_suspend, dwiic_resume);

	fdtbus_register_i2c_controller(self, phandle, &dwiic_fdt_i2c_funcs);

	fdtbus_attach_i2cbus(self, phandle, &sc->sc_dwiic.sc_i2c_tag, iicbus_print);

out:
	return;
}

static i2c_tag_t
dwiic_fdt_i2c_get_tag(device_t dev)
{
	struct dwiic_fdt_softc * const sc = device_private(dev);

	return &sc->sc_dwiic.sc_i2c_tag;
}

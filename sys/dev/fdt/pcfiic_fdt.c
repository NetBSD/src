/*	$NetBSD: pcfiic_fdt.c,v 1.1 2026/07/12 04:23:59 thorpej Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: pcfiic_fdt.c,v 1.1 2026/07/12 04:23:59 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <dev/fdt/fdtvar.h>

#include <dev/ofw/openfirm.h>

#include <dev/i2c/i2cvar.h>

#include <dev/ic/pcf8584var.h>
#include <dev/ic/pcf8584reg.h>

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "nxp,pcf8584" },
	DEVICE_COMPAT_EOL
};

struct pcfiic_fdt_softc {
	struct pcfiic_softc	sc_pcfdev;

	int			sc_phandle;
	void			*sc_ih;
};

static int
pcfiic_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
pcfiic_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct pcfiic_fdt_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;
	uint32_t frequency = 0;
	uint32_t own_addr;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_phandle = phandle;
	sc->sc_pcfdev.sc_iot = faa->faa_bst;
	error = bus_space_map(faa->faa_bst, addr, size, 0,
			      &sc->sc_pcfdev.sc_ioh);
	if (error) {
		aprint_error(": couldn't map registers (error=%d)\n",
		    error);
		return;
	}

	sc->sc_pcfdev.sc_dev = self;
	if (of_getprop_uint32(phandle, "clock-frequency", &frequency)) {
		clk = fdtbus_clock_get_index(phandle, 0);
		if (clk != NULL) {
			frequency = clk_get_rate(clk);
		}
	}
	if (frequency == 0) {
		aprint_error(": couldn't get frequency\n");
		return;
	}

	/*
	 * Compute the clock configuration values based on the external
	 * clock frequency.  In general, we want to run at full speed
	 * (90KHz, just shy of the standard I2C full-speed 100KHz).
	 *
	 * However, the speeds that we can configure (90KHz, 45KHz,
	 * 11KHz, and 1.5KHz, respectively) can only be achieved if
	 * we get an exact match on the external clock frequency (and
	 * can thus configure the prescalar properly).  Those frequencies
	 * are 3MHz, 4.43MHz, 6MHz, 8MHz, and 12MHz.
	 *
	 * XXX We could be a little more sophisticated here... like what
	 * if the external clock is 6.25MHz?
	 */
	uint8_t scl_val = PCF8584_SCL_90;
	uint8_t clk_val;

	if (frequency <= 3000000) {
		clk_val = PCF8584_CLK_3;
	} else if (frequency <= 4430000) {
		clk_val = PCF8584_CLK_4_43;
	} else if (frequency <= 6000000) {
		clk_val = PCF8584_CLK_6;
	} else if (frequency <= 8000000) {
		clk_val = PCF8584_CLK_8;
	} else if (frequency <= 12000000) {
		clk_val = PCF8584_CLK_12;
	} else {
		aprint_error(": frequency %u out of range\n", frequency);
		return;
	}

	if (of_getprop_uint32(phandle, "own-address", &own_addr)) {
		own_addr = 0xaa;
	} else if (own_addr == 0x00 || own_addr > 0xff) {
		aprint_error(": invalid own-address property\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		sc->sc_pcfdev.sc_poll = 1;
	}
	aprint_normal("\n");

	uint32_t reg_shift;
	if (of_getprop_uint32(phandle, "reg-shift", &reg_shift) < 0) {
		reg_shift = 0;
	}

	sc->sc_pcfdev.sc_regmap[PCF8584_S0] = PCF8584_S0 << reg_shift;
	sc->sc_pcfdev.sc_regmap[PCF8584_S1] = PCF8584_S1 << reg_shift;

	if (!sc->sc_pcfdev.sc_poll) {
		sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0,
		    IPL_SERIAL, 0, pcfiic_intr, &sc->sc_pcfdev,
		    device_xname(self));
		if (sc->sc_ih == NULL) {
			aprint_error_dev(self,
			    "failed to establish interrupt at %s\n", intrstr);
			sc->sc_pcfdev.sc_poll = 1;
		} else {
			aprint_normal_dev(self,
			    "interrupting at %s\n", intrstr);
		}
	}

	pcfiic_attach(&sc->sc_pcfdev,
	    (i2c_addr_t)(own_addr >> 1), scl_val | clk_val);
}

CFATTACH_DECL_NEW(pcfiic_fdt, sizeof(struct pcfiic_fdt_softc),
	pcfiic_fdt_match, pcfiic_fdt_attach, NULL, NULL);

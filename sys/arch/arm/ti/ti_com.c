/* $NetBSD: ti_com.c,v 1.3 2017/10/26 23:28:15 jmcneill Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: ti_com.c,v 1.3 2017/10/26 23:28:15 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/termios.h>

#include <dev/ic/comvar.h>

#include <dev/fdt/fdtvar.h>

#include <arch/arm/ti/ti_prcm.h>

static int ti_com_match(device_t, cfdata_t, void *);
static void ti_com_attach(device_t, device_t, void *);

static const char * const compatible[] = {
	"ti,am3352-uart",
	"ti,omap3-uart",
	NULL
};

struct ti_com_softc {
	struct com_softc ssc_sc;
	void *ssc_ih;
};

CFATTACH_DECL_NEW(ti_com, sizeof(struct ti_com_softc),
	ti_com_match, ti_com_attach, NULL, NULL);

static int
ti_com_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
ti_com_attach(device_t parent, device_t self, void *aux)
{
	struct ti_com_softc * const ssc = device_private(self);
	struct com_softc * const sc = &ssc->ssc_sc;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_space_handle_t bsh;
	bus_space_tag_t bst;
	char intrstr[128];
	struct clk *hwmod;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	bst = faa->faa_a4x_bst;

	sc->sc_dev = self;

	if (of_getprop_uint32(phandle, "clock-frequency", &sc->sc_frequency) != 0) {
		aprint_error(": missing 'clock-frequency' property\n");
		return;
	}

	sc->sc_type = COM_TYPE_NORMAL;

	error = bus_space_map(bst, addr, size, 0, &bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}

	hwmod = ti_prcm_get_hwmod(phandle, 0);
	KASSERT(hwmod != NULL);
	if (clk_enable(hwmod) != 0) {
		aprint_error(": couldn't enable module\n");
		return;
	}

	COM_INIT_REGS(sc->sc_regs, bst, bsh, addr);

	com_attach_subr(sc);
	aprint_naive("\n");

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	ssc->ssc_ih = fdtbus_intr_establish(phandle, 0, IPL_SERIAL,
	    FDT_INTR_MPSAFE, comintr, sc);
	if (ssc->ssc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
}

/*
 * Console support
 */

static int
ti_com_console_match(int phandle)
{
	return of_match_compatible(phandle, compatible);
}

static void
ti_com_console_consinit(struct fdt_attach_args *faa, u_int uart_freq)
{
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_a4x_bst;
	bus_addr_t addr;
	tcflag_t flags;
	int speed;

	fdtbus_get_reg(phandle, 0, &addr, NULL);
	speed = fdtbus_get_stdout_speed();
	if (speed < 0)
		speed = 115200;	/* default */
	flags = fdtbus_get_stdout_flags();

	if (comcnattach(bst, addr, speed, uart_freq, COM_TYPE_NORMAL, flags))
		panic("Cannot initialize ti com console");
}

static const struct fdt_console ti_com_console = {
	.match = ti_com_console_match,
	.consinit = ti_com_console_consinit,
};

FDT_CONSOLE(ti_com, &ti_com_console);

/* $NetBSD: apple_wdog.c,v 1.3 2022/04/05 05:04:04 skrll Exp $ */

/*-
 * Copyright (c) 2021 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: apple_wdog.c,v 1.3 2022/04/05 05:04:04 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/lwp.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#define	WDOG_CHIP_CTL		0x000c
#define	WDOG_SYS_TMR		0x0010
#define	WDOG_SYS_RST		0x0014
#define	WDOG_SYS_CTL		0x001c
#define	 WDOG_SYS_CTL_ENABLE	__BIT(2)

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "apple,wdt" },
	{ .compat = "apple,reboot-v0" },
	DEVICE_COMPAT_EOL
};

struct apple_wdog_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
};

#define WDOG_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WDOG_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static void
apple_wdog_reset(device_t dev)
{
	struct apple_wdog_softc * const sc = device_private(dev);

	WDOG_WRITE(sc, WDOG_SYS_RST, 1);
	WDOG_WRITE(sc, WDOG_SYS_CTL, WDOG_SYS_CTL_ENABLE);
	WDOG_WRITE(sc, WDOG_SYS_TMR, 0);
}

static struct fdtbus_power_controller_func apple_wdog_power_funcs = {
	.reset = apple_wdog_reset,
};

static int
apple_wdog_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
apple_wdog_attach(device_t parent, device_t self, void *aux)
{
	struct apple_wdog_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Apple Watchdog\n");

	WDOG_WRITE(sc, WDOG_CHIP_CTL, 0);
	WDOG_WRITE(sc, WDOG_SYS_CTL, 0);

	fdtbus_register_power_controller(self, phandle,
	    &apple_wdog_power_funcs);
}

CFATTACH_DECL_NEW(apple_wdog, sizeof(struct apple_wdog_softc),
	apple_wdog_match, apple_wdog_attach, NULL, NULL);

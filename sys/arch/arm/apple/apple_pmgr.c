/* $NetBSD: apple_pmgr.c,v 1.1 2022/04/27 07:55:42 skrll Exp $ */

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: apple_pmgr.c,v 1.1 2022/04/27 07:55:42 skrll Exp $");

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/queue.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

/*
 * Power manager registers
 */
#define PMGR_PS_TARGET_MASK	__BITS(3, 0)
#define PMGR_PS_ACTUAL_MASK	__BITS(7, 4)
#define  PMGR_PS_ACTIVE		0xf
#define  PMGR_PS_PWRGATE	0x0

struct apple_pmgr_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
};

#define PMGR_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	PMGR_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))


static const struct device_compatible_entry compat_data[] = {
	{ .compat = "apple,pmgr" },
	DEVICE_COMPAT_EOL
};

static void
apple_pmgr_enable(device_t dev, const uint32_t *data, bool enable)
{
	struct apple_pmgr_softc * const sc = device_private(dev);
	const uint32_t pstate = enable ? PMGR_PS_ACTIVE : PMGR_PS_PWRGATE;
	const int phandle = fdtbus_get_phandle_from_native(be32dec(data));

	fdtbus_powerdomain_enable(phandle);

	bus_size_t off;
	bus_size_t size;
	if (fdtbus_get_reg(phandle, 0, &off, &size) != 0 || size != 4) {
		aprint_error(": invalid 'reg' property\n");
		return;
	}

	uint32_t val = PMGR_READ(sc, off);

	val &= ~PMGR_PS_TARGET_MASK;
	val |= __SHIFTIN(pstate, PMGR_PS_TARGET_MASK);
	PMGR_WRITE(sc, off, val);

	for (int timo = 0; timo < 100; timo++) {
		val = PMGR_READ(sc, off);
		if (__SHIFTOUT(val, PMGR_PS_ACTUAL_MASK) == pstate)
			break;
		delay(1);
	}
}


static int
apple_pmgr_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}


static void
apple_pmgr_attach(device_t parent, device_t self, void *aux)
{
	struct apple_pmgr_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	bus_addr_t pmgr_addr;
	bus_size_t pmgr_size;

	int error = fdtbus_get_reg(phandle, 0, &pmgr_addr, &pmgr_size);
	if (error) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;

	error = bus_space_map(faa->faa_bst, pmgr_addr, pmgr_size, 0,
	    &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map registers: %d\n", error);
		return;
	}

	for (int node = OF_child(phandle); node; node = OF_peer(node)) {
		static const struct device_compatible_entry compat_ps[] = {
			{ .compat = "apple,pmgr-pwrstate" },
			DEVICE_COMPAT_EOL
		};

		if (!of_compatible_match(node, compat_ps))
			continue;

		static struct fdtbus_powerdomain_controller_func funcs = {
			.pdc_enable = apple_pmgr_enable,
		};

		fdtbus_register_powerdomain_controller(self, node, &funcs);
	}
}

CFATTACH_DECL_NEW(apple_pmgr, sizeof(struct apple_pmgr_softc),
    apple_pmgr_match, apple_pmgr_attach, NULL, NULL);

/* $NetBSD: sunxi_resets.c,v 1.1.8.2 2017/12/03 11:35:56 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_resets.c,v 1.1.8.2 2017/12/03 11:35:56 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

#include <dev/clk/clk_backend.h>

#define	RESET_REG(index)	(((index) / 32) * 4)
#define	RESET_MASK(index)	__BIT((index) % 32)

static const char * compatible[] = {
	"allwinner,sun6i-a31-clock-reset",
	NULL
};

struct sunxi_resets_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
};

#define	RESET_READ(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	RESET_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static void *
sunxi_resets_acquire(device_t dev, const void *data, size_t len)
{
	if (len != 4)
		return NULL;

	/* Specifier is an index. Just return it. */
	return (void *)(uintptr_t)be32dec(data);
}

static void
sunxi_resets_release(device_t dev, void *priv)
{
}

static int
sunxi_resets_assert(device_t dev, void *priv)
{
	struct sunxi_resets_softc * const sc = device_private(dev);
	const uintptr_t index = (uintptr_t)priv;

	const bus_size_t reset_reg = RESET_REG(index);
	const uint32_t reset_mask = RESET_MASK(index);

	const uint32_t val = RESET_READ(sc, reset_reg);
	RESET_WRITE(sc, reset_reg, val & ~reset_mask);

	return 0;
}

static int
sunxi_resets_deassert(device_t dev, void *priv)
{
	struct sunxi_resets_softc * const sc = device_private(dev);
	const uintptr_t index = (uintptr_t)priv;

	const bus_size_t reset_reg = RESET_REG(index);
	const uint32_t reset_mask = RESET_MASK(index);

	const uint32_t val = RESET_READ(sc, reset_reg);
	RESET_WRITE(sc, reset_reg, val | reset_mask);

	return 0;
}

static const struct fdtbus_reset_controller_func sunxi_fdtreset_funcs = {
	.acquire = sunxi_resets_acquire,
	.release = sunxi_resets_release,
	.reset_assert = sunxi_resets_assert,
	.reset_deassert = sunxi_resets_deassert,
};

static int
sunxi_resets_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sunxi_resets_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_resets_softc * const sc = device_private(self);
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
	aprint_normal("\n");

	fdtbus_register_reset_controller(sc->sc_dev, phandle,
	    &sunxi_fdtreset_funcs);
}

CFATTACH_DECL_NEW(sunxi_resets, sizeof(struct sunxi_resets_softc),
    sunxi_resets_match, sunxi_resets_attach, NULL, NULL);

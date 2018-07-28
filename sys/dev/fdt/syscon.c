/* $NetBSD: syscon.c,v 1.2.2.2 2018/07/28 04:37:44 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: syscon.c,v 1.2.2.2 2018/07/28 04:37:44 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

struct syscon_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	kmutex_t		sc_lock;

	struct syscon		sc_syscon;
};

static int	syscon_match(device_t, cfdata_t, void *);
static void	syscon_attach(device_t, device_t, void *);

static const char *compatible[] = {
	"syscon",
	NULL
};

CFATTACH_DECL_NEW(syscon, sizeof(struct syscon_softc),
	syscon_match, syscon_attach, NULL, NULL);

static void
syscon_generic_lock(void *priv)
{
	struct syscon_softc * const sc = priv;

	mutex_enter(&sc->sc_lock);
}

static void
syscon_generic_unlock(void *priv)
{
	struct syscon_softc * const sc = priv;

	mutex_exit(&sc->sc_lock);
}

static uint32_t
syscon_generic_read_4(void *priv, bus_size_t reg)
{
	struct syscon_softc * const sc = priv;

	KASSERT(mutex_owned(&sc->sc_lock));

	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, reg);
}

static void
syscon_generic_write_4(void *priv, bus_size_t reg, uint32_t val)
{
	struct syscon_softc * const sc = priv;

	KASSERT(mutex_owned(&sc->sc_lock));

	bus_space_write_4(sc->sc_bst, sc->sc_bsh, reg, val);
}

static int
syscon_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
syscon_attach(device_t parent, device_t self, void *aux)
{
	struct syscon_softc * const sc = device_private(self);
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
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);
	sc->sc_syscon.priv = sc;
	sc->sc_syscon.lock = syscon_generic_lock;
	sc->sc_syscon.unlock = syscon_generic_unlock;
	sc->sc_syscon.read_4 = syscon_generic_read_4;
	sc->sc_syscon.write_4 = syscon_generic_write_4;

	aprint_naive("\n");
	aprint_normal(": System Controller Registers\n");

	fdtbus_register_syscon(self, phandle, &sc->sc_syscon);

	fdt_add_bus(self, phandle, faa);
}

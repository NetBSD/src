/* $NetBSD: jh7110_syscon.c,v 1.1 2024/11/11 20:30:08 skrll Exp $ */

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: jh7110_syscon.c,v 1.1 2024/11/11 20:30:08 skrll Exp $");

#include <sys/param.h>

#include <sys/mutex.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

struct jh7110_syscon_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;

	kmutex_t		sc_lock;
	struct syscon		sc_syscon;
};

#define RD4(sc, reg)							       \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WR4(sc, reg, val)						       \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))


static void
jh7110_syscon_lock(void *priv)
{
	struct jh7110_syscon_softc * const sc = priv;

	mutex_enter(&sc->sc_lock);
}

static void
jh7110_syscon_unlock(void *priv)
{
	struct jh7110_syscon_softc * const sc = priv;

	mutex_exit(&sc->sc_lock);
}

static uint32_t
jh7110_syscon_read_4(void *priv, bus_size_t reg)
{
	struct jh7110_syscon_softc * const sc = priv;

	KASSERT(mutex_owned(&sc->sc_lock));

	return RD4(sc, reg);
}

static void
jh7110_syscon_write_4(void *priv, bus_size_t reg, uint32_t val)
{
	struct jh7110_syscon_softc * const sc = priv;

	KASSERT(mutex_owned(&sc->sc_lock));

	WR4(sc, reg, val);
}


/* Compat string(s) */
static const struct device_compatible_entry compat_data[] = {
	{ .compat = "starfive,jh7110-stg-syscon" },
	DEVICE_COMPAT_EOL
};

static int
jh7110_syscon_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
jh7110_syscon_attach(device_t parent, device_t self, void *aux)
{
	struct jh7110_syscon_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const bus_space_tag_t bst = faa->faa_bst;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = bst;

	error = fdtbus_get_reg(phandle, 0, &addr, &size);
	if (error) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	error = bus_space_map(bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#" PRIxBUSADDR ": %d", addr,
		    error);
		return;
	}

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	aprint_naive("\n");
	aprint_normal(": JH7110 STG system controller\n");

	sc->sc_syscon.priv = sc;
	sc->sc_syscon.lock = jh7110_syscon_lock;
	sc->sc_syscon.unlock = jh7110_syscon_unlock;
	sc->sc_syscon.read_4 = jh7110_syscon_read_4;
	sc->sc_syscon.write_4 = jh7110_syscon_write_4;
	fdtbus_register_syscon(self, phandle, &sc->sc_syscon);
}

CFATTACH_DECL_NEW(jh7110_syscon, sizeof(struct jh7110_syscon_softc),
	jh7110_syscon_match, jh7110_syscon_attach, NULL, NULL);

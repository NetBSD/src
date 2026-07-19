/*	$NetBSD: pgctrl.c,v 1.1 2026/07/19 01:48:24 thorpej Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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

/*
 * Flattened Device Tree glue for pg68k "control space".
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pgctrl.c,v 1.1 2026/07/19 01:48:24 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/mutex.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

#include <hb68k/pg68k/control.h>

struct pgctrl_softc {
	device_t	sc_dev;
	kmutex_t	sc_lock;
	struct syscon	sc_syscon;
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "pg68k,control-space" },
	DEVICE_COMPAT_EOL
};

static void
pgctrl_lock(void *v)
{
	struct pgctrl_softc * const sc = v;

	mutex_enter(&sc->sc_lock);
}

static void
pgctrl_unlock(void *v)
{
	struct pgctrl_softc * const sc = v;

	mutex_exit(&sc->sc_lock);
}

static uint32_t
pgctrl_read_4(void *v, bus_size_t reg)
{
	struct pgctrl_softc * const sc = v;

	KASSERT(mutex_owned(&sc->sc_lock));

	return control_inb(reg);
}

static void
pgctrl_write_4(void *v, bus_size_t reg, uint32_t val)
{
	struct pgctrl_softc * const sc = v;

	KASSERT(mutex_owned(&sc->sc_lock));

	control_outb(reg, (uint8_t)val);
}

static int
pgctrl_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
pgctrl_attach(device_t parent, device_t self, void *aux)
{
	struct pgctrl_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	sc->sc_syscon.priv = sc;
	sc->sc_syscon.lock = pgctrl_lock;
	sc->sc_syscon.unlock = pgctrl_unlock;
	sc->sc_syscon.read_4 = pgctrl_read_4;
	sc->sc_syscon.write_4 = pgctrl_write_4;

	fdtbus_register_syscon(self, phandle, &sc->sc_syscon);
}

CFATTACH_DECL_NEW(pgctrl, sizeof(struct pgctrl_softc),
    pgctrl_match, pgctrl_attach, NULL, NULL);

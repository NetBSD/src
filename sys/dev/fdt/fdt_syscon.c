/* $NetBSD: fdt_syscon.c,v 1.3.2.2 2018/07/28 04:37:44 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fdt_syscon.c,v 1.3.2.2 2018/07/28 04:37:44 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/queue.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

struct syscon;

struct fdtbus_syscon {
	device_t sc_dev;
	int sc_phandle;
	struct syscon *sc_syscon;

	LIST_ENTRY(fdtbus_syscon) sc_next;
};

static LIST_HEAD(, fdtbus_syscon) fdtbus_syscons =
    LIST_HEAD_INITIALIZER(fdtbus_syscons);

int
fdtbus_register_syscon(device_t dev, int phandle,
    struct syscon *syscon)
{
	struct fdtbus_syscon *sc;

	sc = kmem_alloc(sizeof(*sc), KM_SLEEP);
	sc->sc_dev = dev;
	sc->sc_phandle = phandle;
	sc->sc_syscon = syscon;

	LIST_INSERT_HEAD(&fdtbus_syscons, sc, sc_next);

	return 0;
}

static struct fdtbus_syscon *
fdtbus_get_syscon(int phandle)
{
	struct fdtbus_syscon *sc;

	LIST_FOREACH(sc, &fdtbus_syscons, sc_next) {
		if (sc->sc_phandle == phandle)
			return sc;
	}

	return NULL;
}

struct syscon *
fdtbus_syscon_acquire(int phandle, const char *prop)
{
	struct fdtbus_syscon *sc;
	int sc_phandle;

	sc_phandle = fdtbus_get_phandle(phandle, prop);
	if (sc_phandle < 0)
		return NULL;

	sc = fdtbus_get_syscon(sc_phandle);
	if (sc == NULL)
		return NULL;

	return sc->sc_syscon;
}

struct syscon *
fdtbus_syscon_lookup(int phandle)
{
	struct fdtbus_syscon *sc;

	sc = fdtbus_get_syscon(phandle);
	if (sc == NULL)
		return NULL;

	return sc->sc_syscon;
}

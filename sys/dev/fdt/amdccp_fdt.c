/* $NetBSD: amdccp_fdt.c,v 1.1.2.2 2018/10/20 06:58:31 pgoyette Exp $ */

/*
 * Copyright (c) 2018 Jonathan A. Kollasch
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: amdccp_fdt.c,v 1.1.2.2 2018/10/20 06:58:31 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <dev/fdt/fdtvar.h>

#include <dev/ic/amdccpvar.h>

struct amdccp_fdt_softc {
	struct amdccp_softc sc_sc;
};

static int amdccp_fdt_match(device_t, cfdata_t, void *);
static void amdccp_fdt_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(amdccp_fdt, sizeof(struct amdccp_fdt_softc),
    amdccp_fdt_match, amdccp_fdt_attach, NULL, NULL);

static const struct of_compat_data compat_data[] = {
	{ "amd,ccp-seattle-v1a", },
	{ NULL }
};

static int
amdccp_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
amdccp_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct amdccp_fdt_softc * const fsc = device_private(self);
	struct amdccp_softc * const sc = &fsc->sc_sc;
	const struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	fsc->sc_sc.sc_dev = self;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;		
	}

	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": AMD CCP\n");

	amdccp_common_attach(sc);
}

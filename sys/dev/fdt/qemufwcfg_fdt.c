/* $NetBSD: qemufwcfg_fdt.c,v 1.1.2.2 2018/06/25 07:25:49 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: qemufwcfg_fdt.c,v 1.1.2.2 2018/06/25 07:25:49 pgoyette Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/bus.h>

#include <dev/fdt/fdtvar.h>

#include <dev/ic/qemufwcfgvar.h>

static int	fwcfg_fdt_match(device_t, cfdata_t, void *);
static void	fwcfg_fdt_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(qemufwcfg_fdt, sizeof(struct fwcfg_softc),
    fwcfg_fdt_match,
    fwcfg_fdt_attach,
    NULL,
    NULL
);

static const char * const compatible[] = {
	"qemu,fw-cfg-mmio",
	NULL
};


static int
fwcfg_fdt_match(device_t parent, cfdata_t match, void *opaque)
{
	struct fdt_attach_args * const faa = opaque;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
fwcfg_fdt_attach(device_t parent, device_t self, void *opaque)
{
	struct fwcfg_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = opaque;
	bus_addr_t base;
	bus_size_t size;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &base, &size) != 0) {
		aprint_error_dev(self, "couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;

	if (bus_space_map(sc->sc_bst, base, size, 0, &sc->sc_bsh) != 0) {
		aprint_error_dev(self, "couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal("\n");

	fwcfg_attach(sc);

	pmf_device_register(self, NULL, NULL);
}

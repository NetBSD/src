/* $NetBSD: sunxi_sid.c,v 1.1 2017/10/03 23:42:17 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_sid.c,v 1.1 2017/10/03 23:42:17 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_sid.h>

#define	EFUSE_THERMAL_CALIB0	0x34
#define	EFUSE_THERMAL_CALIB1	0x38

struct sunxi_sid_config {
	bus_size_t	efuse_offset;
};

static const struct sunxi_sid_config sun4i_a10_sid_config = {
	.efuse_offset = 0,
};

static const struct sunxi_sid_config sun8i_h3_sid_config = {
	.efuse_offset = 0x200,
};

static const struct of_compat_data compat_data[] = {
	{ "allwinner,sun4i-a10-sid",	(uintptr_t)&sun4i_a10_sid_config },
	{ "allwinner,sun7i-a20-sid",	(uintptr_t)&sun4i_a10_sid_config },
	{ "allwinner,sun8i-h3-sid",	(uintptr_t)&sun8i_h3_sid_config },
	{ NULL }
};

struct sunxi_sid_softc {
	device_t			sc_dev;
	bus_space_tag_t			sc_bst;
	bus_space_handle_t		sc_bsh;
	const struct sunxi_sid_config	*sc_conf;
};

static struct sunxi_sid_softc *sid_softc = NULL;

#define EFUSE_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (sc)->sc_conf->efuse_offset + (reg))
#define EFUSE_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (sc)->sc_conf->efuse_offset + (reg), (val))

static int
sunxi_sid_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_sid_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_sid_softc * const sc = device_private(self);
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
	sc->sc_conf = (void *)of_search_compatible(phandle, compat_data)->data;

	aprint_naive("\n");
	aprint_normal(": Security ID EFUSE\n");

	KASSERT(sid_softc == NULL);
	sid_softc = sc;
}

CFATTACH_DECL_NEW(sunxi_sid, sizeof(struct sunxi_sid_softc),
	sunxi_sid_match, sunxi_sid_attach, NULL, NULL);

int
sunxi_sid_read_tscalib(uint32_t *calib)
{
	if (sid_softc == NULL)
		return ENXIO;

	calib[0] = EFUSE_READ(sid_softc, EFUSE_THERMAL_CALIB0);
	calib[1] = EFUSE_READ(sid_softc, EFUSE_THERMAL_CALIB1);
	return 0;
}

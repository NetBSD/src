/* $NetBSD: tegra_genfb.c,v 1.1.2.3 2015/09/22 12:05:38 skrll Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_genfb.c,v 1.1.2.3 2015/09/22 12:05:38 skrll Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#if defined(DDB)
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include <dev/wsfb/genfbvar.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_var.h>

static int	tegra_genfb_match(device_t, cfdata_t, void *);
static void	tegra_genfb_attach(device_t, device_t, void *);

struct tegra_genfb_softc {
	struct genfb_softc	sc_gen;
	bus_dma_tag_t		sc_dmat;
	bus_dmamap_t		sc_dmamap;
};

static int	tegra_genfb_ioctl(void *, void *, u_long, void *, int, lwp_t *);
static paddr_t	tegra_genfb_mmap(void *, void *, off_t, int);
static bool	tegra_genfb_shutdown(device_t, int);

#if defined(DDB)
static void	tegra_genfb_ddb_trap_callback(int);
static device_t tegra_genfb_consoledev = NULL;
#endif

CFATTACH_DECL_NEW(tegra_genfb, sizeof(struct tegra_genfb_softc),
	tegra_genfb_match, tegra_genfb_attach, NULL, NULL);

static int
tegra_genfb_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
tegra_genfb_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_genfb_softc * const sc = device_private(self);
	struct tegrafb_attach_args * const tfb = aux;
	prop_dictionary_t prop = device_properties(self);
	const bool is_console = tfb->tfb_console;
	struct genfb_ops ops;

	sc->sc_gen.sc_dev = self;
	sc->sc_dmat = tfb->tfb_dmat;
	sc->sc_dmamap = tfb->tfb_dmamap;

	prop_dictionary_set_bool(prop, "is_console", is_console);
	prop_dictionary_set_uint32(prop, "width", tfb->tfb_width);
	prop_dictionary_set_uint32(prop, "height", tfb->tfb_height);
	prop_dictionary_set_uint8(prop, "depth", tfb->tfb_depth);
	prop_dictionary_set_uint32(prop, "linebytes", tfb->tfb_stride);
	prop_dictionary_set_uint64(prop, "address", 0);
	prop_dictionary_set_uint64(prop, "virtual_address",
	    (uintptr_t)tfb->tfb_dmap);

	genfb_init(&sc->sc_gen);
	if (sc->sc_gen.sc_width == 0 || sc->sc_gen.sc_fbsize == 0) {
		aprint_error(": disabled\n");
		return;
	}

	pmf_device_register1(self, NULL, NULL, tegra_genfb_shutdown);

	aprint_naive("\n");
	if (is_console) {
		aprint_normal(": switching to framebuffer console\n");
	} else {
		aprint_normal("\n");
	}

	memset(&ops, 0, sizeof(ops));
	ops.genfb_ioctl = tegra_genfb_ioctl;
	ops.genfb_mmap = tegra_genfb_mmap;
	genfb_attach(&sc->sc_gen, &ops);

#if defined(DDB)
	if (is_console) {
		tegra_genfb_consoledev = self;
		db_trap_callback = tegra_genfb_ddb_trap_callback;
	}
#endif

}

static int
tegra_genfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct tegra_genfb_softc * const sc = v;
	struct wsdisplayio_bus_id *busid;
	struct wsdisplayio_fbinfo *fbi;
	struct rasops_info *ri = &sc->sc_gen.vd.active->scr_ri;
	int error;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_TEGRA;
		return 0;
	case WSDISPLAYIO_GET_BUSID:
		busid = data;
		busid->bus_type = WSDISPLAYIO_BUS_SOC;
		return 0;
	case WSDISPLAYIO_GET_FBINFO:
		fbi = data;
		error = wsdisplayio_get_fbinfo(ri, fbi);
		fbi->fbi_flags |= WSFB_VRAM_IS_RAM;
		return error;
	default:
		return EPASSTHROUGH;
	}
}

static paddr_t
tegra_genfb_mmap(void *v, void *vs, off_t off, int prot)
{
	struct tegra_genfb_softc * const sc = v;

	if (off < 0 || off >= sc->sc_dmamap->dm_segs[0].ds_len)
		return -1;

	return bus_dmamem_mmap(sc->sc_dmat, sc->sc_dmamap->dm_segs, 1,
	    off, prot, BUS_DMA_PREFETCHABLE);
}

static bool
tegra_genfb_shutdown(device_t self, int flags)
{
	genfb_enable_polling(self);
	return true;
}

#if defined(DDB)
static void
tegra_genfb_ddb_trap_callback(int where)
{
	if (tegra_genfb_consoledev == NULL)
		return;

	if (where) {
		genfb_enable_polling(tegra_genfb_consoledev);
	} else {
		genfb_disable_polling(tegra_genfb_consoledev);
	}
}
#endif

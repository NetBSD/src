/*	$NetBSD: imx_genfb.c,v 1.1.18.2 2017/12/03 11:35:53 jdolecek Exp $	*/

/*
 * Copyright (c) 2015 Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx_genfb.c,v 1.1.18.2 2017/12/03 11:35:53 jdolecek Exp $");

#include "opt_ddb.h"
#include "opt_genfb.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <arm/imx/imx51reg.h>
#include <arm/imx/imx51var.h>
#include <arm/imx/imx51_ipuv3var.h>

#if defined(DDB)
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include <dev/videomode/videomode.h>
#include <dev/wsfb/genfbvar.h>

struct imx_genfb_softc {
	struct genfb_softc sc_gen;

	bus_dma_tag_t sc_dmat;
	bus_dma_segment_t *sc_dmasegs;
	int sc_ndmasegs;
};

#if defined(DDB)
static void	imx_genfb_ddb_trap_callback(int);
static device_t	imx_genfb_consoledev = NULL;
#endif

static int	imx_genfb_match(device_t, cfdata_t, void *);
static void	imx_genfb_attach(device_t, device_t, void *);

static int	imx_genfb_ioctl(void *, void *, u_long, void *, int, lwp_t *);
static paddr_t	imx_genfb_mmap(void *, void *, off_t, int);
static bool	imx_genfb_shutdown(device_t, int);

CFATTACH_DECL_NEW(imx_genfb, sizeof(struct imx_genfb_softc),
	imx_genfb_match, imx_genfb_attach, NULL, NULL);

static int
imx_genfb_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
imx_genfb_attach(device_t parent, device_t self, void *aux)
{
	struct imx_genfb_softc *sc = device_private(self);
	struct imxfb_attach_args * const ifb = aux;
	prop_dictionary_t cfg = device_properties(self);
	struct genfb_ops ops;

	sc->sc_gen.sc_dev = self;
	sc->sc_dmat = ifb->ifb_dmat;
	sc->sc_dmasegs = ifb->ifb_dmasegs;
	sc->sc_ndmasegs = ifb->ifb_ndmasegs;

	prop_dictionary_set_uint32(cfg, "width", ifb->ifb_width);
	prop_dictionary_set_uint32(cfg, "height", ifb->ifb_height);
	prop_dictionary_set_uint8(cfg, "depth", ifb->ifb_depth);
	prop_dictionary_set_uint16(cfg, "linebytes", ifb->ifb_stride);
	prop_dictionary_set_uint32(cfg, "address", 0);
	prop_dictionary_set_uint32(cfg, "virtual_address",
	    (uintptr_t)ifb->ifb_fb);

	genfb_init(&sc->sc_gen);

	if (sc->sc_gen.sc_width == 0 || sc->sc_gen.sc_fbsize == 0) {
		aprint_normal(": disabled\n");
		return;
	}

	pmf_device_register1(self, NULL, NULL, imx_genfb_shutdown);

	memset(&ops, 0, sizeof(ops));
	ops.genfb_ioctl = imx_genfb_ioctl;
	ops.genfb_mmap = imx_genfb_mmap;

	aprint_naive("\n");

	bool is_console = false;
	prop_dictionary_get_bool(cfg, "is_console", &is_console);

	if (is_console)
		aprint_normal(": switching to framebuffer console\n");
	else
		aprint_normal("\n");

	genfb_attach(&sc->sc_gen, &ops);

#if defined(DDB)
	if (is_console) {
		imx_genfb_consoledev = self;
		db_trap_callback = imx_genfb_ddb_trap_callback;
	}
#endif
}

static int
imx_genfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct wsdisplayio_bus_id *busid;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_IMXIPU;
		return 0;
	case WSDISPLAYIO_GET_BUSID:
		busid = data;
		busid->bus_type = WSDISPLAYIO_BUS_SOC;
		return 0;
	default:
		return EPASSTHROUGH;
	}
}

static paddr_t
imx_genfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct imx_genfb_softc *sc = v;

	KASSERT(offset >= 0);
	KASSERT(offset < sc->sc_dmasegs[0].ds_len);

	return bus_dmamem_mmap(sc->sc_dmat, sc->sc_dmasegs, sc->sc_ndmasegs,
	    offset, prot, BUS_DMA_PREFETCHABLE);
}

static bool
imx_genfb_shutdown(device_t self, int flags)
{
	genfb_enable_polling(self);
	return true;
}

#if defined(DDB)
static void
imx_genfb_ddb_trap_callback(int where)
{
	if (imx_genfb_consoledev == NULL)
		return;

	if (where)
		genfb_enable_polling(imx_genfb_consoledev);
	else
		genfb_disable_polling(imx_genfb_consoledev);
}
#endif

void
imx_genfb_set_videomode(device_t dev, u_int width, u_int height)
{
	struct imx_genfb_softc *sc = device_private(dev);

	if (sc->sc_gen.sc_width != width || sc->sc_gen.sc_height != height) {
		device_printf(sc->sc_gen.sc_dev,
		    "mode switching not yet supported\n");
	}
}

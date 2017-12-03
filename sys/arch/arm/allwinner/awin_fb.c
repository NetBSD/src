/* $NetBSD: awin_fb.c,v 1.9.16.2 2017/12/03 11:35:51 jdolecek Exp $ */

/*-
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
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

#include "awin_mp.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: awin_fb.c,v 1.9.16.2 2017/12/03 11:35:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <dev/videomode/videomode.h>
#include <dev/wsfb/genfbvar.h>

struct awin_fb_softc {
	struct genfb_softc sc_gen;
	device_t sc_debedev;
	device_t sc_mpdev;

	bus_dma_tag_t sc_dmat;
	bus_dma_segment_t *sc_dmasegs;
	int sc_ndmasegs;
};

static device_t	awin_fb_consoledev = NULL;

static int	awin_fb_match(device_t, cfdata_t, void *);
static void	awin_fb_attach(device_t, device_t, void *);

static int	awin_fb_ioctl(void *, void *, u_long, void *, int, lwp_t *);
static paddr_t	awin_fb_mmap(void *, void *, off_t, int);
static bool	awin_fb_shutdown(device_t, int);

CFATTACH_DECL_NEW(awin_fb, sizeof(struct awin_fb_softc),
	awin_fb_match, awin_fb_attach, NULL, NULL);

static int
awin_fb_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
awin_fb_attach(device_t parent, device_t self, void *aux)
{
	struct awin_fb_softc *sc = device_private(self);
	struct awinfb_attach_args * const afb = aux;
	prop_dictionary_t cfg = device_properties(self);
	struct genfb_ops ops;

	if (awin_fb_consoledev == NULL)
		awin_fb_consoledev = self;

	sc->sc_gen.sc_dev = self;
	sc->sc_debedev = parent;
	sc->sc_dmat = afb->afb_dmat;
	sc->sc_dmasegs = afb->afb_dmasegs;
	sc->sc_ndmasegs = afb->afb_ndmasegs;
	sc->sc_mpdev = device_find_by_driver_unit("awinmp", 0);

	prop_dictionary_set_uint32(cfg, "width", afb->afb_width);
	prop_dictionary_set_uint32(cfg, "height", afb->afb_height);
	prop_dictionary_set_uint8(cfg, "depth", 32);
	prop_dictionary_set_uint16(cfg, "linebytes", afb->afb_width * 4);
	prop_dictionary_set_uint32(cfg, "address", 0);
	prop_dictionary_set_uint32(cfg, "virtual_address",
	    (uintptr_t)afb->afb_fb);

	genfb_init(&sc->sc_gen);

	if (sc->sc_gen.sc_width == 0 || sc->sc_gen.sc_fbsize == 0) {
		aprint_normal(": disabled\n");
		return;
	}

	pmf_device_register1(self, NULL, NULL, awin_fb_shutdown);

	memset(&ops, 0, sizeof(ops));
	ops.genfb_ioctl = awin_fb_ioctl;
	ops.genfb_mmap = awin_fb_mmap;

	aprint_naive("\n");

	bool is_console = false;
	prop_dictionary_get_bool(cfg, "is_console", &is_console);

	if (is_console)
		aprint_normal(": switching to framebuffer console\n");
	else
		aprint_normal("\n");

	genfb_attach(&sc->sc_gen, &ops);
}

static int
awin_fb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct awin_fb_softc *sc = v;
	struct wsdisplayio_bus_id *busid;
	struct wsdisplayio_fbinfo *fbi;
	struct rasops_info *ri;
	int error;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_ALLWINNER;
		return 0;
	case WSDISPLAYIO_GET_BUSID:
		busid = data;
		busid->bus_type = WSDISPLAYIO_BUS_SOC;
		return 0;
	case WSDISPLAYIO_GET_FBINFO:
		fbi = data;
		ri = &sc->sc_gen.vd.active->scr_ri;
		error = wsdisplayio_get_fbinfo(ri, fbi);
		if (error == 0) {
			fbi->fbi_flags |= WSFB_VRAM_IS_RAM;
			fbi->fbi_fbsize = sc->sc_dmasegs[0].ds_len;
#if NAWIN_MP > 0
			if (sc->sc_mpdev)
				fbi->fbi_flags |= WSFB_ACCEL;
#endif
		}
		return error;
	case WSDISPLAYIO_SVIDEO:
	case WSDISPLAYIO_GVIDEO:
	case WSDISPLAYIO_GCURPOS:
	case WSDISPLAYIO_SCURPOS:
	case WSDISPLAYIO_GCURMAX:
	case WSDISPLAYIO_SCURSOR:
		return awin_debe_ioctl(sc->sc_debedev, cmd, data);
#if NAWIN_MP > 0
	case WSDISPLAYIO_FILL:
	case WSDISPLAYIO_COPY:
	case WSDISPLAYIO_SYNC:
		if (sc->sc_mpdev == NULL)
			return EPASSTHROUGH;
		return awin_mp_ioctl(sc->sc_mpdev, cmd, data);
#endif
	default:
		return EPASSTHROUGH;
	}
}

static paddr_t
awin_fb_mmap(void *v, void *vs, off_t off, int prot)
{
	struct awin_fb_softc *sc = v;

	if (off < 0 || off >= sc->sc_dmasegs[0].ds_len)
		return -1;

	return bus_dmamem_mmap(sc->sc_dmat, sc->sc_dmasegs, sc->sc_ndmasegs,
	    off, prot, BUS_DMA_PREFETCHABLE);
}

static bool
awin_fb_shutdown(device_t self, int flags)
{
	genfb_enable_polling(self);
	return true;
}

void
awin_fb_ddb_trap_callback(int where)
{
	if (awin_fb_consoledev == NULL)
		return;

	if (where) {
		genfb_enable_polling(awin_fb_consoledev);
	} else {
		genfb_disable_polling(awin_fb_consoledev);
	}
}

void
awin_fb_set_videomode(device_t dev, u_int width, u_int height)
{
	struct awin_fb_softc *sc = device_private(dev);

	if (sc->sc_gen.sc_width != width || sc->sc_gen.sc_height != height) {
		device_printf(sc->sc_gen.sc_dev,
		    "mode switching not yet supported\n");
	}
}

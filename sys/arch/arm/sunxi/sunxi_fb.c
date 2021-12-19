/* $NetBSD: sunxi_fb.c,v 1.7 2021/12/19 12:28:20 riastradh Exp $ */

/*-
 * Copyright (c) 2015-2019 Jared McNeill <jmcneill@invisible.ca>
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

#include "opt_wsdisplay_compat.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunxi_fb.c,v 1.7 2021/12/19 12:28:20 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_drm.h>

#include <drm/drm_drv.h>
#include <drm/drmfb.h>

static int	sunxi_fb_match(device_t, cfdata_t, void *);
static void	sunxi_fb_attach(device_t, device_t, void *);

static void	sunxi_fb_init(struct sunxi_drm_task *);

static bool	sunxi_fb_shutdown(device_t, int);

struct sunxi_fb_softc {
	struct drmfb_softc	sc_drmfb;
	device_t		sc_dev;
	struct sunxi_drm_softc	*sc_drm;
	struct sunxi_drm_framebuffer *sc_fb;
	struct sunxi_drmfb_attach_args sc_sfa;
	struct sunxi_drm_task	sc_attach_task;
};

static paddr_t	sunxi_fb_mmapfb(struct drmfb_softc *, off_t, int);
static int	sunxi_fb_ioctl(struct drmfb_softc *, u_long, void *, int,
			       lwp_t *);

static const struct drmfb_params sunxifb_drmfb_params = {
	.dp_mmapfb = sunxi_fb_mmapfb,
	.dp_ioctl = sunxi_fb_ioctl,
	
};

CFATTACH_DECL_NEW(sunxi_fb, sizeof(struct sunxi_fb_softc),
	sunxi_fb_match, sunxi_fb_attach, NULL, NULL);

static int
sunxi_fb_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
sunxi_fb_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_fb_softc * const sc = device_private(self);
	struct sunxi_drm_softc * const drmsc = device_private(parent);
	struct sunxi_drmfb_attach_args * const sfa = aux;

	sc->sc_dev = self;
	sc->sc_drm = drmsc;
	sc->sc_sfa = *sfa;
	sc->sc_fb = to_sunxi_drm_framebuffer(sfa->sfa_fb_helper->fb);

	aprint_naive("\n");
	aprint_normal("\n");

#ifdef WSDISPLAY_MULTICONS
	prop_dictionary_t dict = device_properties(self);
	const bool is_console = true;
	prop_dictionary_set_bool(dict, "is_console", is_console);
#endif
	sunxi_task_init(&sc->sc_attach_task, &sunxi_fb_init);
	sunxi_task_schedule(parent, &sc->sc_attach_task);
}

static void
sunxi_fb_init(struct sunxi_drm_task *task)
{
	struct sunxi_fb_softc * const sc =
	    container_of(task, struct sunxi_fb_softc, sc_attach_task);
	device_t dev = sc->sc_dev;
	struct sunxi_drmfb_attach_args * const sfa = &sc->sc_sfa;
	int error;

	const struct drmfb_attach_args da = {
		.da_dev = dev,
		.da_fb_helper = sfa->sfa_fb_helper,
		.da_fb_sizes = &sfa->sfa_fb_sizes,
		.da_fb_vaddr = sc->sc_fb->obj->vaddr,
		.da_fb_linebytes = sfa->sfa_fb_linebytes,
		.da_params = &sunxifb_drmfb_params,
	};


	error = drmfb_attach(&sc->sc_drmfb, &da);
	if (error) {
		aprint_error_dev(dev, "failed to attach drmfb: %d\n", error);
		return;
	}

	pmf_device_register1(dev, NULL, NULL, sunxi_fb_shutdown);
}

static bool
sunxi_fb_shutdown(device_t self, int flags)
{
	struct sunxi_fb_softc * const sc = device_private(self);

	return drmfb_shutdown(&sc->sc_drmfb, flags);
}

static paddr_t
sunxi_fb_mmapfb(struct drmfb_softc *sc, off_t off, int prot)
{
	struct sunxi_fb_softc * const tfb_sc = (struct sunxi_fb_softc *)sc;
	struct drm_gem_cma_object *obj = tfb_sc->sc_fb->obj;

	KASSERT(off >= 0);
	KASSERT(off < obj->dmasize);

	return bus_dmamem_mmap(obj->dmat, obj->dmasegs, 1, off, prot,
	    BUS_DMA_PREFETCHABLE);
}

static int
sunxi_fb_ioctl(struct drmfb_softc *sc, u_long cmd, void *data, int flag,
    lwp_t *l)
{
	struct wsdisplayio_bus_id *busid;
	struct wsdisplayio_fbinfo *fbi;
	struct rasops_info *ri = &sc->sc_genfb.vd.active->scr_ri;
	int error;

	switch (cmd) {
	case WSDISPLAYIO_GET_BUSID:
		busid = data;
		busid->bus_type = WSDISPLAYIO_BUS_SOC;
		return 0;
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_GENFB;
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

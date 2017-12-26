/* $NetBSD: tegra_fb.c,v 1.4 2017/12/26 14:54:52 jmcneill Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_fb.c,v 1.4 2017/12/26 14:54:52 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <drm/drmP.h>
#include <drm/drmfb.h>

#include <arm/nvidia/tegra_var.h>
#include <arm/nvidia/tegra_drm.h>

static int	tegra_fb_match(device_t, cfdata_t, void *);
static void	tegra_fb_attach(device_t, device_t, void *);

static bool	tegra_fb_shutdown(device_t, int);

struct tegra_fb_softc {
	struct drmfb_softc	sc_drmfb;
	device_t		sc_dev;
	struct tegra_drm_softc	*sc_drm;
	struct tegra_drmfb_attach_args sc_tfa;
	struct tegra_framebuffer *sc_fb;
};

static paddr_t	tegra_fb_mmapfb(struct drmfb_softc *, off_t, int);
static int	tegra_fb_ioctl(struct drmfb_softc *, u_long, void *, int,
			       lwp_t *);


static const struct drmfb_params tegrafb_drmfb_params = {
	.dp_mmapfb = tegra_fb_mmapfb,
	.dp_ioctl = tegra_fb_ioctl,
	
};

CFATTACH_DECL_NEW(tegra_fb, sizeof(struct tegra_fb_softc),
	tegra_fb_match, tegra_fb_attach, NULL, NULL);

static int
tegra_fb_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
tegra_fb_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_fb_softc * const sc = device_private(self);
	struct tegra_drm_softc * const drmsc = device_private(parent);
	struct tegra_drmfb_attach_args * const tfa = aux;
	int error;

	sc->sc_dev = self;
	sc->sc_drm = drmsc;
	sc->sc_tfa = *tfa;
	sc->sc_fb = to_tegra_framebuffer(tfa->tfa_fb_helper->fb);

	aprint_naive("\n");
	aprint_normal("\n");

	const struct drmfb_attach_args da = {
		.da_dev = self,
		.da_fb_helper = tfa->tfa_fb_helper,
		.da_fb_sizes = &tfa->tfa_fb_sizes,
		.da_fb_vaddr = sc->sc_fb->obj->vaddr,
		.da_fb_linebytes = tfa->tfa_fb_linebytes,
		.da_params = &tegrafb_drmfb_params,
	};

	error = drmfb_attach(&sc->sc_drmfb, &da);
	if (error) {
		aprint_error_dev(self, "failed to attach drmfb: %d\n", error);
		return;
	}

	pmf_device_register1(self, NULL, NULL, tegra_fb_shutdown);
}

static bool
tegra_fb_shutdown(device_t self, int flags)
{
	struct tegra_fb_softc * const sc = device_private(self);

	return drmfb_shutdown(&sc->sc_drmfb, flags);
}

static paddr_t
tegra_fb_mmapfb(struct drmfb_softc *sc, off_t off, int prot)
{
	struct tegra_fb_softc * const tfb_sc = (struct tegra_fb_softc *)sc;
	struct drm_gem_cma_object *obj = tfb_sc->sc_fb->obj;

	KASSERT(off >= 0);
	KASSERT(off < obj->dmasize);

	return bus_dmamem_mmap(obj->dmat, obj->dmasegs, 1, off, prot,
	    BUS_DMA_PREFETCHABLE);
}

static int
tegra_fb_ioctl(struct drmfb_softc *sc, u_long cmd, void *data, int flag,
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
		*(u_int *)data = WSDISPLAY_TYPE_TEGRA;
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

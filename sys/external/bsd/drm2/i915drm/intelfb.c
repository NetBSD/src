/*	$NetBSD: intelfb.c,v 1.9.6.3 2017/12/03 11:37:59 jdolecek Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intelfb.c,v 1.9.6.3 2017/12/03 11:37:59 jdolecek Exp $");

#include <sys/types.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <drm/drmP.h>
#include <drm/drmfb.h>
#include <drm/drmfb_pci.h>

#include "i915_drv.h"
#include "i915_pci.h"
#include "intelfb.h"

static int	intelfb_match(device_t, cfdata_t, void *);
static void	intelfb_attach(device_t, device_t, void *);
static int	intelfb_detach(device_t, int);

static void	intelfb_attach_task(struct i915drmkms_task *);

static bool	intelfb_shutdown(device_t, int);

static paddr_t	intelfb_drmfb_mmapfb(struct drmfb_softc *, off_t, int);

struct intelfb_softc {
	struct drmfb_softc		sc_drmfb; /* XXX Must be first.  */
	device_t			sc_dev;
	struct intelfb_attach_args	sc_ifa;
	bus_space_handle_t		sc_fb_bsh;
	struct i915drmkms_task		sc_attach_task;
	bool				sc_mapped:1;
	bool				sc_scheduled:1;
	bool				sc_attached:1;
};

static const struct drmfb_params intelfb_drmfb_params = {
	.dp_mmapfb = intelfb_drmfb_mmapfb,
	.dp_mmap = drmfb_pci_mmap,
	.dp_ioctl = drmfb_pci_ioctl,
	.dp_is_vga_console = drmfb_pci_is_vga_console,
	.dp_disable_vga = i915_disable_vga,
};

CFATTACH_DECL_NEW(intelfb, sizeof(struct intelfb_softc),
    intelfb_match, intelfb_attach, intelfb_detach, NULL);

static int
intelfb_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

static void
intelfb_attach(device_t parent, device_t self, void *aux)
{
	struct intelfb_softc *const sc = device_private(self);
	const struct intelfb_attach_args *const ifa = aux;
	int error;

	sc->sc_dev = self;
	sc->sc_ifa = *ifa;
	sc->sc_mapped = false;
	sc->sc_scheduled = false;
	sc->sc_attached = false;

	aprint_naive("\n");
	aprint_normal("\n");

	/* XXX Defer this too?  */
	error = bus_space_map(ifa->ifa_fb_bst, ifa->ifa_fb_addr,
	    ifa->ifa_fb_size,
	    BUS_SPACE_MAP_LINEAR|BUS_SPACE_MAP_PREFETCHABLE,
	    &sc->sc_fb_bsh);
	if (error) {
		aprint_error_dev(self, "unable to map framebuffer: %d\n",
		    error);
		goto fail0;
	}
	sc->sc_mapped = true;

	i915drmkms_task_init(&sc->sc_attach_task, &intelfb_attach_task);
	error = i915drmkms_task_schedule(parent, &sc->sc_attach_task);
	if (error) {
		aprint_error_dev(self, "failed to schedule mode set: %d\n",
		    error);
		goto fail1;
	}
	sc->sc_scheduled = true;

	/* Success!  */
	return;

fail1:	bus_space_unmap(ifa->ifa_fb_bst, sc->sc_fb_bsh, ifa->ifa_fb_size);
	sc->sc_mapped = false;
fail0:	return;
}

static int
intelfb_detach(device_t self, int flags)
{
	struct intelfb_softc *const sc = device_private(self);
	int error;

	if (sc->sc_scheduled)
		return EBUSY;

	if (sc->sc_attached) {
		pmf_device_deregister(self);
		error = drmfb_detach(&sc->sc_drmfb, flags);
		if (error) {
			/* XXX Ugh.  */
			(void)pmf_device_register1(self, NULL, NULL,
			    &intelfb_shutdown);
			return error;
		}
		sc->sc_attached = false;
	}

	if (sc->sc_mapped) {
		bus_space_unmap(sc->sc_ifa.ifa_fb_bst, sc->sc_fb_bsh,
		    sc->sc_ifa.ifa_fb_size);
		sc->sc_mapped = false;
	}

	return 0;
}

static void
intelfb_attach_task(struct i915drmkms_task *task)
{
	struct intelfb_softc *const sc = container_of(task,
	    struct intelfb_softc, sc_attach_task);
	const struct intelfb_attach_args *const ifa = &sc->sc_ifa;
	const struct drm_fb_helper_surface_size *const sizes = &ifa->ifa_fb_sizes;
	const struct drmfb_attach_args da = {
		.da_dev = sc->sc_dev,
		.da_fb_helper = ifa->ifa_fb_helper,
		.da_fb_sizes = &ifa->ifa_fb_sizes,
		.da_fb_vaddr = bus_space_vaddr(ifa->ifa_fb_bst, sc->sc_fb_bsh),
		.da_fb_linebytes = roundup2((sizes->surface_width *
		    howmany(sizes->surface_bpp, 8)), 64),
		.da_params = &intelfb_drmfb_params,
	};
	int error;

	error = drmfb_attach(&sc->sc_drmfb, &da);
	if (error) {
		aprint_error_dev(sc->sc_dev, "failed to attach drmfb: %d\n",
		    error);
		return;
	}

	if (!pmf_device_register1(sc->sc_dev, NULL, NULL, &intelfb_shutdown))
		aprint_error_dev(sc->sc_dev,
		    "failed to register shutdown handler\n");

	sc->sc_attached = true;
}

static bool
intelfb_shutdown(device_t self, int flags)
{
	struct intelfb_softc *const sc = device_private(self);

	return drmfb_shutdown(&sc->sc_drmfb, flags);
}

static paddr_t
intelfb_drmfb_mmapfb(struct drmfb_softc *drmfb, off_t offset, int prot)
{
	struct intelfb_softc *const sc = container_of(drmfb,
	    struct intelfb_softc, sc_drmfb);
	struct drm_fb_helper *const helper = sc->sc_ifa.ifa_fb_helper;
	struct intel_fbdev *const fbdev = container_of(helper,
	    struct intel_fbdev, helper);
	struct drm_device *const dev = helper->dev;
	struct drm_i915_private *const dev_priv = dev->dev_private;

	KASSERT(0 <= offset);
	KASSERT(offset < fbdev->fb->obj->base.size);

	return bus_space_mmap(dev->bst, dev_priv->gtt.mappable_base,
	    i915_gem_obj_ggtt_offset(fbdev->fb->obj) + offset,
	    prot, BUS_SPACE_MAP_PREFETCHABLE);
}

/*	$NetBSD: radeondrmkmsfb.c,v 1.16 2022/07/18 23:33:53 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: radeondrmkmsfb.c,v 1.16 2022/07/18 23:33:53 riastradh Exp $");

#include <sys/types.h>
#include <sys/device.h>

#include <drm/drm_fb_helper.h>
#include <drm/drmfb.h>
#include <drm/drmfb_pci.h>

#include <radeon.h>
#include "radeon_drv.h"
#include "radeon_task.h"
#include "radeondrmkmsfb.h"

struct radeonfb_softc {
	struct drmfb_softc		sc_drmfb; /* XXX Must be first.  */
	device_t			sc_dev;
	struct radeonfb_attach_args	sc_rfa;
	struct radeon_task		sc_attach_task;
	bool				sc_attached:1;
};

static int	radeonfb_match(device_t, cfdata_t, void *);
static void	radeonfb_attach(device_t, device_t, void *);
static int	radeonfb_detach(device_t, int);

static void	radeonfb_attach_task(struct radeon_task *);

static paddr_t	radeonfb_drmfb_mmapfb(struct drmfb_softc *, off_t, int);
static bool	radeonfb_shutdown(device_t, int);

CFATTACH_DECL_NEW(radeondrmkmsfb, sizeof(struct radeonfb_softc),
    radeonfb_match, radeonfb_attach, radeonfb_detach, NULL);

static const struct drmfb_params radeonfb_drmfb_params = {
	.dp_mmapfb = radeonfb_drmfb_mmapfb,
	.dp_mmap = drmfb_pci_mmap,
	.dp_ioctl = drmfb_pci_ioctl,
	.dp_is_vga_console = drmfb_pci_is_vga_console,
};

static int
radeonfb_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

static void
radeonfb_attach(device_t parent, device_t self, void *aux)
{
	struct radeonfb_softc *const sc = device_private(self);
	const struct radeonfb_attach_args *const rfa = aux;
	int error;

	sc->sc_dev = self;
	sc->sc_rfa = *rfa;
	sc->sc_attached = false;

	aprint_naive("\n");
	aprint_normal("\n");

	radeon_task_init(&sc->sc_attach_task, &radeonfb_attach_task);
	error = radeon_task_schedule(parent, &sc->sc_attach_task);
	if (error) {
		aprint_error_dev(self, "failed to schedule mode set: %d\n",
		    error);
		return;
	}
	config_pending_incr(self);
}

static int
radeonfb_detach(device_t self, int flags)
{
	struct radeonfb_softc *const sc = device_private(self);
	int error;

	if (sc->sc_attached) {
		pmf_device_deregister(self);
		error = drmfb_detach(&sc->sc_drmfb, flags);
		if (error) {
			/* XXX Ugh.  */
			(void)pmf_device_register1(self, NULL, NULL,
			    &radeonfb_shutdown);
			return error;
		}
		sc->sc_attached = false;
	}

	return 0;
}

static void
radeonfb_attach_task(struct radeon_task *task)
{
	struct radeonfb_softc *const sc = container_of(task,
	    struct radeonfb_softc, sc_attach_task);
	const struct radeonfb_attach_args *const rfa = &sc->sc_rfa;
	const struct drmfb_attach_args da = {
		.da_dev = sc->sc_dev,
		.da_fb_helper = rfa->rfa_fb_helper,
		.da_fb_sizes = &rfa->rfa_fb_sizes,
		.da_fb_vaddr = __UNVOLATILE(rfa->rfa_fb_ptr),
		.da_fb_linebytes = rfa->rfa_fb_linebytes,
		.da_params = &radeonfb_drmfb_params,
	};
	int error;

	error = drmfb_attach(&sc->sc_drmfb, &da);
	if (error) {
		aprint_error_dev(sc->sc_dev, "failed to attach drmfb: %d\n",
		    error);
		goto out;
	}
	if (!pmf_device_register1(sc->sc_dev, NULL, NULL, &radeonfb_shutdown))
		aprint_error_dev(sc->sc_dev,
		    "failed to register shutdown handler\n");

	sc->sc_attached = true;
out:
	config_pending_decr(sc->sc_dev);
}

static bool
radeonfb_shutdown(device_t self, int flags)
{
	struct radeonfb_softc *const sc = device_private(self);

	return drmfb_shutdown(&sc->sc_drmfb, flags);
}

static paddr_t
radeonfb_drmfb_mmapfb(struct drmfb_softc *drmfb, off_t offset, int prot)
{
	struct radeonfb_softc *const sc = container_of(drmfb,
	    struct radeonfb_softc, sc_drmfb);
	struct drm_fb_helper *const helper = sc->sc_rfa.rfa_fb_helper;
	struct drm_framebuffer *const fb = helper->fb;
	struct drm_gem_object *const gobj = fb->obj[0];
	struct radeon_bo *const rbo = gem_to_radeon_bo(gobj);
	int flags = 0;

	if (offset < 0)
		return -1;

	const unsigned num_pages __diagused = rbo->tbo.num_pages;

	KASSERT(offset < (num_pages << PAGE_SHIFT));
	KASSERT(rbo->tbo.mem.bus.is_iomem);

	if (ISSET(rbo->tbo.mem.placement, TTM_PL_FLAG_WC))
		flags |= BUS_SPACE_MAP_PREFETCHABLE;

	return bus_space_mmap(rbo->tbo.bdev->memt,
	    rbo->tbo.mem.bus.base, rbo->tbo.mem.bus.offset + offset,
	    prot, flags);
}

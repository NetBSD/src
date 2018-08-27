/*	$NetBSD: amdgpufb.c,v 1.1 2018/08/27 14:02:32 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: amdgpufb.c,v 1.1 2018/08/27 14:02:32 riastradh Exp $");

#include <sys/types.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <drm/drmP.h>
#include <drm/drmfb.h>
#include <drm/drmfb_pci.h>

#include "amdgpu.h"
#include "amdgpu_task.h"
#include "amdgpufb.h"

static int	amdgpufb_match(device_t, cfdata_t, void *);
static void	amdgpufb_attach(device_t, device_t, void *);
static int	amdgpufb_detach(device_t, int);

static void	amdgpufb_attach_task(struct amdgpu_task *);

static bool	amdgpufb_shutdown(device_t, int);
static paddr_t	amdgpufb_drmfb_mmapfb(struct drmfb_softc *, off_t, int);

struct amdgpufb_softc {
	struct drmfb_softc		sc_drmfb; /* XXX Must be first.  */
	device_t			sc_dev;
	struct amdgpufb_attach_args	sc_afa;
	struct amdgpu_task		sc_attach_task;
	bool				sc_scheduled:1;
	bool				sc_attached:1;
};

static const struct drmfb_params amdgpufb_drmfb_params = {
	.dp_mmapfb = amdgpufb_drmfb_mmapfb,
	.dp_mmap = drmfb_pci_mmap,
	.dp_ioctl = drmfb_pci_ioctl,
	.dp_is_vga_console = drmfb_pci_is_vga_console,
};

CFATTACH_DECL_NEW(amdgpufb, sizeof(struct amdgpufb_softc),
    amdgpufb_match, amdgpufb_attach, amdgpufb_detach, NULL);

static int
amdgpufb_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

static void
amdgpufb_attach(device_t parent, device_t self, void *aux)
{
	struct amdgpufb_softc *const sc = device_private(self);
	const struct amdgpufb_attach_args *const afa = aux;
	int error;

	sc->sc_dev = self;
	sc->sc_afa = *afa;
	sc->sc_scheduled = false;
	sc->sc_attached = false;

	aprint_naive("\n");
	aprint_normal("\n");

	amdgpu_task_init(&sc->sc_attach_task, &amdgpufb_attach_task);
	error = amdgpu_task_schedule(parent, &sc->sc_attach_task);
	if (error) {
		aprint_error_dev(self, "failed to schedule mode set: %d\n",
		    error);
		goto fail0;
	}
	sc->sc_scheduled = true;

	/* Success!  */
	return;

fail0:	return;
}

static int
amdgpufb_detach(device_t self, int flags)
{
	struct amdgpufb_softc *const sc = device_private(self);
	int error;

	if (sc->sc_scheduled)
		return EBUSY;

	if (sc->sc_attached) {
		pmf_device_deregister(self);
		error = drmfb_detach(&sc->sc_drmfb, flags);
		if (error) {
			/* XXX Ugh.  */
			(void)pmf_device_register1(self, NULL, NULL,
			    &amdgpufb_shutdown);
			return error;
		}
		sc->sc_attached = false;
	}

	return 0;
}

static void
amdgpufb_attach_task(struct amdgpu_task *task)
{
	struct amdgpufb_softc *const sc = container_of(task,
	    struct amdgpufb_softc, sc_attach_task);
	const struct amdgpufb_attach_args *const afa = &sc->sc_afa;
	const struct drmfb_attach_args da = {
		.da_dev = sc->sc_dev,
		.da_fb_helper = afa->afa_fb_helper,
		.da_fb_sizes = &afa->afa_fb_sizes,
		.da_fb_vaddr = __UNVOLATILE(afa->afa_fb_ptr),
		.da_fb_linebytes = afa->afa_fb_linebytes,
		.da_params = &amdgpufb_drmfb_params,
	};
	int error;

	error = drmfb_attach(&sc->sc_drmfb, &da);
	if (error) {
		aprint_error_dev(sc->sc_dev, "failed to attach drmfb: %d\n",
		    error);
		return;
	}

	if (!pmf_device_register1(sc->sc_dev, NULL, NULL, &amdgpufb_shutdown))
		aprint_error_dev(sc->sc_dev,
		    "failed to register shutdown handler\n");

	sc->sc_attached = true;
}

static bool
amdgpufb_shutdown(device_t self, int flags)
{
	struct amdgpufb_softc *const sc = device_private(self);

	return drmfb_shutdown(&sc->sc_drmfb, flags);
}

static paddr_t
amdgpufb_drmfb_mmapfb(struct drmfb_softc *drmfb, off_t offset, int prot)
{
	struct amdgpufb_softc *const sc = container_of(drmfb,
	    struct amdgpufb_softc, sc_drmfb);
	struct drm_fb_helper *const helper = sc->sc_afa.afa_fb_helper;
	struct drm_framebuffer *const fb = helper->fb;
	struct amdgpu_framebuffer *const rfb = container_of(fb,
	    struct amdgpu_framebuffer, base);
	struct drm_gem_object *const gobj = rfb->obj;
	struct amdgpu_bo *const rbo = gem_to_amdgpu_bo(gobj);
	const unsigned num_pages __diagused = rbo->tbo.num_pages;
	int flags = 0;

	KASSERT(0 <= offset);
	KASSERT(offset < ((uintmax_t)num_pages << PAGE_SHIFT));
	KASSERT(rbo->tbo.mem.bus.is_iomem);

	if (ISSET(rbo->tbo.mem.placement, TTM_PL_FLAG_WC))
		flags |= BUS_SPACE_MAP_PREFETCHABLE;

	return bus_space_mmap(rbo->tbo.bdev->memt, rbo->tbo.mem.bus.base,
	    rbo->tbo.mem.bus.offset + offset, prot, flags);
}

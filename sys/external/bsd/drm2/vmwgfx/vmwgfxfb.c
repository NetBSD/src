/*	$NetBSD: vmwgfxfb.c,v 1.1 2022/02/17 01:21:03 riastradh Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: vmwgfxfb.c,v 1.1 2022/02/17 01:21:03 riastradh Exp $");

#include <sys/types.h>
#include <sys/device.h>

#include <drm/drm_fb_helper.h>
#include <drm/drmfb.h>
#include <drm/drmfb_pci.h>

#include "vmwgfx_drv.h"
#include "vmwgfx_task.h"
#include "vmwgfxfb.h"

struct vmwgfxfb_softc {
	struct drmfb_softc		sc_drmfb; /* XXX Must be first.  */
	device_t			sc_dev;
	struct vmwgfxfb_attach_args	sc_vfa;
	struct vmwgfx_task		sc_attach_task;
	bool				sc_scheduled:1;
	bool				sc_attached:1;
};

static int	vmwgfxfb_match(device_t, cfdata_t, void *);
static void	vmwgfxfb_attach(device_t, device_t, void *);
static int	vmwgfxfb_detach(device_t, int);

static void	vmwgfxfb_attach_task(struct vmwgfx_task *);

static paddr_t	vmwgfxfb_drmfb_mmapfb(struct drmfb_softc *, off_t, int);
static bool	vmwgfxfb_shutdown(device_t, int);

CFATTACH_DECL_NEW(vmwgfxfb, sizeof(struct vmwgfxfb_softc),
    vmwgfxfb_match, vmwgfxfb_attach, vmwgfxfb_detach, NULL);

static const struct drmfb_params vmwgfxfb_drmfb_params = {
	.dp_mmapfb = vmwgfxfb_drmfb_mmapfb,
	.dp_mmap = drmfb_pci_mmap,
	.dp_ioctl = drmfb_pci_ioctl,
	.dp_is_vga_console = drmfb_pci_is_vga_console,
};

static int
vmwgfxfb_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

static void
vmwgfxfb_attach(device_t parent, device_t self, void *aux)
{
	struct vmwgfxfb_softc *const sc = device_private(self);
	const struct vmwgfxfb_attach_args *const vfa = aux;
	int error;

	sc->sc_dev = self;
	sc->sc_vfa = *vfa;
	sc->sc_scheduled = false;
	sc->sc_attached = false;

	aprint_naive("\n");
	aprint_normal("\n");

	vmwgfx_task_init(&sc->sc_attach_task, &vmwgfxfb_attach_task);
	error = vmwgfx_task_schedule(parent, &sc->sc_attach_task);
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
vmwgfxfb_detach(device_t self, int flags)
{
	struct vmwgfxfb_softc *const sc = device_private(self);
	int error;

	if (sc->sc_scheduled)
		return EBUSY;

	if (sc->sc_attached) {
		pmf_device_deregister(self);
		error = drmfb_detach(&sc->sc_drmfb, flags);
		if (error) {
			/* XXX Ugh.  */
			(void)pmf_device_register1(self, NULL, NULL,
			    &vmwgfxfb_shutdown);
			return error;
		}
		sc->sc_attached = false;
	}

	return 0;
}

static void
vmwgfxfb_attach_task(struct vmwgfx_task *task)
{
	struct vmwgfxfb_softc *const sc = container_of(task,
	    struct vmwgfxfb_softc, sc_attach_task);
	const struct vmwgfxfb_attach_args *const vfa = &sc->sc_vfa;
	const struct drmfb_attach_args da = {
		.da_dev = sc->sc_dev,
		.da_fb_helper = vfa->vfa_fb_helper,
		.da_fb_sizes = &vfa->vfa_fb_sizes,
		.da_fb_vaddr = __UNVOLATILE(vfa->vfa_fb_ptr),
		.da_fb_linebytes = vfa->vfa_fb_linebytes,
		.da_params = &vmwgfxfb_drmfb_params,
	};
	device_t parent = device_parent(sc->sc_dev);
	bool is_console;
	int error;

	KASSERT(sc->sc_scheduled);

	/*
	 * MD device enumeration logic may choose the vmwgfxN PCI
	 * device as the console.  If so, propagate that down to the
	 * vmwgfxfbN device for genfb.
	 */
	if (prop_dictionary_get_bool(device_properties(parent),
		"is_console", &is_console) &&
	    !prop_dictionary_set_bool(device_properties(sc->sc_dev),
		"is_console", is_console)) {
		aprint_error_dev(sc->sc_dev, "failed to set is_console\n");
	}

	error = drmfb_attach(&sc->sc_drmfb, &da);
	if (error) {
		aprint_error_dev(sc->sc_dev, "failed to attach drmfb: %d\n",
		    error);
		return;
	}
	if (!pmf_device_register1(sc->sc_dev, NULL, NULL, &vmwgfxfb_shutdown))
		aprint_error_dev(sc->sc_dev,
		    "failed to register shutdown handler\n");

	sc->sc_attached = true;
}

static bool
vmwgfxfb_shutdown(device_t self, int flags)
{
	struct vmwgfxfb_softc *const sc = device_private(self);

	return drmfb_shutdown(&sc->sc_drmfb, flags);
}

static paddr_t
vmwgfxfb_drmfb_mmapfb(struct drmfb_softc *drmfb, off_t offset, int prot)
{
	struct vmwgfxfb_softc *const sc = container_of(drmfb,
	    struct vmwgfxfb_softc, sc_drmfb);
	struct drm_fb_helper *const helper = sc->sc_vfa.vfa_fb_helper;
	struct drm_framebuffer *const fb = helper->fb;
	struct vmw_buffer_object *const vbo = /*XXX MAGIC HERE*/;
	int flags = 0;

	if (offset < 0)
		return -1;

	const unsigned num_pages __diagused = vbo->base.num_pages;

	KASSERT(offset < (num_pages << PAGE_SHIFT));
	KASSERT(vbo->base.mem.bus.is_iomem);

	if (ISSET(vbo->base.mem.placement, TTM_PL_FLAG_WC))
		flags |= BUS_SPACE_MAP_PREFETCHABLE;

	return bus_space_mmap(vbo->base.bdev->memt,
	    vbo->base.mem.bus.base, vbo->base.mem.bus.offset + offset,
	    prot, flags);
}

/*	$NetBSD: nouveaufb.c,v 1.4.14.2 2017/12/03 11:38:00 jdolecek Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: nouveaufb.c,v 1.4.14.2 2017/12/03 11:38:00 jdolecek Exp $");

#include <sys/types.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <drm/drmP.h>
#include <drm/drmfb.h>
#include <drm/drmfb_pci.h>

#include "nouveau_bo.h"
#include "nouveau_drm.h"
#include "nouveau_fbcon.h"
#include "nouveau_pci.h"
#include "nouveaufb.h"

static int	nouveaufb_match(device_t, cfdata_t, void *);
static void	nouveaufb_attach(device_t, device_t, void *);
static int	nouveaufb_detach(device_t, int);

static void	nouveaufb_attach_task(struct nouveau_pci_task *);

static bool	nouveaufb_shutdown(device_t, int);
static paddr_t	nouveaufb_drmfb_mmapfb(struct drmfb_softc *, off_t, int);

struct nouveaufb_softc {
	struct drmfb_softc		sc_drmfb; /* XXX Must be first.  */
	device_t			sc_dev;
	struct nouveaufb_attach_args	sc_nfa;
	struct nouveau_pci_task		sc_attach_task;
	bool				sc_scheduled:1;
	bool				sc_attached:1;
};

static const struct drmfb_params nouveaufb_drmfb_params = {
	.dp_mmapfb = nouveaufb_drmfb_mmapfb,
	.dp_mmap = drmfb_pci_mmap,
	.dp_ioctl = drmfb_pci_ioctl,
	.dp_is_vga_console = drmfb_pci_is_vga_console,
};

CFATTACH_DECL_NEW(nouveaufb, sizeof(struct nouveaufb_softc),
    nouveaufb_match, nouveaufb_attach, nouveaufb_detach, NULL);

static int
nouveaufb_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

static void
nouveaufb_attach(device_t parent, device_t self, void *aux)
{
	struct nouveaufb_softc *const sc = device_private(self);
	const struct nouveaufb_attach_args *const nfa = aux;
	int error;

	sc->sc_dev = self;
	sc->sc_nfa = *nfa;
	sc->sc_scheduled = false;
	sc->sc_attached = false;

	aprint_naive("\n");
	aprint_normal("\n");

	nouveau_pci_task_init(&sc->sc_attach_task, &nouveaufb_attach_task);
	error = nouveau_pci_task_schedule(parent, &sc->sc_attach_task);
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
nouveaufb_detach(device_t self, int flags)
{
	struct nouveaufb_softc *const sc = device_private(self);
	int error;

	if (sc->sc_scheduled)
		return EBUSY;

	if (sc->sc_attached) {
		pmf_device_deregister(self);
		error = drmfb_detach(&sc->sc_drmfb, flags);
		if (error) {
			/* XXX Ugh.  */
			(void)pmf_device_register1(self, NULL, NULL,
			    &nouveaufb_shutdown);
			return error;
		}
		sc->sc_attached = false;
	}

	return 0;
}

static void
nouveaufb_attach_task(struct nouveau_pci_task *task)
{
	struct nouveaufb_softc *const sc = container_of(task,
	    struct nouveaufb_softc, sc_attach_task);
	const struct nouveaufb_attach_args *const nfa = &sc->sc_nfa;
	const struct drmfb_attach_args da = {
		.da_dev = sc->sc_dev,
		.da_fb_helper = nfa->nfa_fb_helper,
		.da_fb_sizes = &nfa->nfa_fb_sizes,
		.da_fb_vaddr = __UNVOLATILE(nfa->nfa_fb_ptr),
		.da_fb_linebytes = nfa->nfa_fb_linebytes,
		.da_params = &nouveaufb_drmfb_params,
	};
	int error;

	error = drmfb_attach(&sc->sc_drmfb, &da);
	if (error) {
		aprint_error_dev(sc->sc_dev, "failed to attach drmfb: %d\n",
		    error);
		return;
	}

	if (!pmf_device_register1(sc->sc_dev, NULL, NULL, &nouveaufb_shutdown))
		aprint_error_dev(sc->sc_dev,
		    "failed to register shutdown handler\n");

	sc->sc_attached = true;
}

static bool
nouveaufb_shutdown(device_t self, int flags)
{
	struct nouveaufb_softc *const sc = device_private(self);

	return drmfb_shutdown(&sc->sc_drmfb, flags);
}

static paddr_t
nouveaufb_drmfb_mmapfb(struct drmfb_softc *drmfb, off_t offset, int prot)
{
	struct nouveaufb_softc *const sc = container_of(drmfb,
	    struct nouveaufb_softc, sc_drmfb);
	struct drm_fb_helper *const helper = sc->sc_nfa.nfa_fb_helper;
	struct nouveau_fbdev *const fbdev = container_of(helper,
	    struct nouveau_fbdev, helper);
	struct nouveau_bo *const nvbo = fbdev->nouveau_fb.nvbo;
	const unsigned num_pages __diagused = nvbo->bo.num_pages;
	int flags = 0;

	KASSERT(0 <= offset);
	KASSERT(offset < (num_pages << PAGE_SHIFT));

	if (ISSET(nvbo->bo.mem.placement, TTM_PL_FLAG_WC))
		flags |= BUS_SPACE_MAP_PREFETCHABLE;

	return bus_space_mmap(nvbo->bo.bdev->memt, nvbo->bo.mem.bus.base,
	    nvbo->bo.mem.bus.offset + offset, prot, flags);
}

/*	$NetBSD: via_pci.c,v 1.3 2018/08/27 14:12:44 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: via_pci.c,v 1.3 2018/08/27 14:12:44 riastradh Exp $");

#include <sys/types.h>
#include <sys/device.h>
#include <sys/pmf.h>
#include <sys/systm.h>

#include <linux/pci.h>

#include <drm/drmP.h>
#include <drm/drm_pciids.h>
#include <drm/via_drm.h>

#include "via_drv.h"

struct viadrm_softc {
	device_t		sc_dev;
	struct pci_dev		sc_pci_dev;
	struct drm_device	*sc_drm_dev;
};

static int	viadrm_match(device_t, cfdata_t, void *);
static void	viadrm_attach(device_t, device_t, void *);
static int	viadrm_detach(device_t, int);

extern struct drm_driver *const via_drm_driver; /* XXX */

CFATTACH_DECL_NEW(viadrmums, sizeof(struct viadrm_softc),
    viadrm_match, viadrm_attach, viadrm_detach, NULL);

static const struct pci_device_id viadrm_pci_ids[] = {
	viadrv_PCI_IDS
};

static const unsigned long *
viadrm_lookup(const struct pci_attach_args *pa)
{
	unsigned i;

	for (i = 0; i < __arraycount(viadrm_pci_ids); i++) {
		KASSERT(viadrm_pci_ids[i].subvendor == PCI_ANY_ID);
		KASSERT(viadrm_pci_ids[i].subdevice == PCI_ANY_ID);
		KASSERT(viadrm_pci_ids[i].class == 0);
		KASSERT(viadrm_pci_ids[i].class_mask == 0);
		if (PCI_VENDOR(pa->pa_id) != viadrm_pci_ids[i].vendor)
			continue;
		if (PCI_PRODUCT(pa->pa_id) != viadrm_pci_ids[i].device)
			continue;
		return &viadrm_pci_ids[i].driver_data;
	}

	return NULL;
}

static int
viadrm_match(device_t parent, cfdata_t match, void *aux)
{
	extern int viadrm_guarantee_initialized(void);
	const struct pci_attach_args *const pa = aux;
	int error;

	error = viadrm_guarantee_initialized();
	if (error) {
		aprint_error("viadrm: failed to initialize: %d\n", error);
		return 0;
	}

	if (viadrm_lookup(pa) == NULL)
		return 0;

	return 1;
}

static void
viadrm_attach(device_t parent, device_t self, void *aux)
{
	struct viadrm_softc *const sc = device_private(self);
	const struct pci_attach_args *const pa = aux;
	const unsigned long *const cookiep = viadrm_lookup(pa);
	int error;

	KASSERT(cookiep != NULL);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	/* Initialize the Linux PCI device descriptor.  */
	linux_pci_dev_init(&sc->sc_pci_dev, self, device_parent(self), pa, 0);

	/* XXX errno Linux->NetBSD */
	error = -drm_pci_attach(self, pa, &sc->sc_pci_dev, via_drm_driver,
	    *cookiep, &sc->sc_drm_dev);
	if (error) {
		aprint_error_dev(self, "unable to attach drm: %d\n", error);
		return;
	}
}

static int
viadrm_detach(device_t self, int flags)
{
	struct viadrm_softc *const sc = device_private(self);
	int error;

	error = config_detach_children(self, flags);
	if (error)
		return error;
	if (sc->sc_drm_dev == NULL)
		goto out;
	/* XXX errno Linux->NetBSD */
	error = -drm_pci_detach(sc->sc_drm_dev, flags);
	if (error)
		return error;
	sc->sc_drm_dev = NULL;

out:	linux_pci_dev_destroy(&sc->sc_pci_dev);
	pmf_device_deregister(self);
	return 0;
}

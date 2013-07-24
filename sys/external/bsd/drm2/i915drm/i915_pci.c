/*	$NetBSD: i915_pci.c,v 1.1.2.2 2013/07/24 03:54:01 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: i915_pci.c,v 1.1.2.2 2013/07/24 03:54:01 riastradh Exp $");

#include <sys/types.h>
#include <sys/systm.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <drm/drmP.h>

#include "i915_drv.h"

struct i915drm_softc {
	struct drm_device	sc_drm_dev;
	struct pci_dev		sc_pci_dev;
};

static int	i915drm_match(device_t, cfdata_t, void *);
static void	i915drm_attach(device_t, device_t, void *);
static int	i915drm_detach(device_t, int);

CFATTACH_DECL_NEW(i915drm, sizeof(struct i915drm_softc),
    i915drm_match, i915drm_attach, i915drm_detach, NULL);

/* XXX Kludge to get these from i915_drv.c.  */
extern struct drm_driver *const i915_drm_driver;
extern const struct pci_device_id *const i915_device_ids;
extern const size_t i915_n_device_ids;

static const struct intel_device_info *
i915drm_pci_lookup(const struct pci_attach_args *pa)
{
	size_t i;

	/* Attach only at function 0 to work around Intel lossage.  */
	if (pa->pa_function != 0)
		return NULL;

	/* We're interested only in Intel products.  */
	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_INTEL)
		return NULL;

	/* We're interested only in Intel display devices.  */
	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_DISPLAY)
		return NULL;

	for (i = 0; i < i915_n_device_ids; i++)
		if (PCI_PRODUCT(pa->pa_id) == i915_device_ids[i].device)
			break;

	/* Did we find it?  */
	if (i == i915_n_device_ids)
		return NULL;

	const struct intel_device_info *const info =
	    (const void *)(uintptr_t)i915_device_ids[i].driver_data;

	/* XXX Whattakludge!  */
	if (info->is_valleyview) {
		printf("i915drm: preliminary hardware support disabled\n");
		return NULL;
	}

	return info;
}

static int
i915drm_match(device_t parent, cfdata_t match, void *aux)
{
	const struct pci_attach_args *const pa = aux;

	return (i915drm_pci_lookup(pa) != NULL);
}

static void
i915drm_attach(device_t parent, device_t self, void *aux)
{
	struct i915drm_softc *const sc = device_private(self);
	const struct pci_attach_args *const pa = aux;
	const struct intel_device_info *const info = i915drm_pci_lookup(pa);
	const unsigned long flags =
	    (unsigned long)(uintptr_t)(const void *)info;

	KASSERT(info != NULL);

	/* XXX Whattakludge!  */
	if (info->gen != 3) {
		i915_drm_driver->driver_features &=~ DRIVER_REQUIRE_AGP;
		i915_drm_driver->driver_features &=~ DRIVER_USE_AGP;
	}

	/* Initialize the drm pci driver state.  */
	sc->sc_drm_dev.driver = i915_drm_driver;
	drm_pci_attach(self, pa, &sc->sc_pci_dev, &sc->sc_drm_dev);

	/* Attach the drm driver.  */
	drm_config_found(self, i915_drm_driver, flags, &sc->sc_drm_dev);
}

static int
i915drm_detach(device_t self, int flags)
{
	struct i915drm_softc *const sc = device_private(self);
	int error;

	/* Detach the drm driver first.  */
	error = config_detach_children(self, flags);
	if (error)
		return error;

	/* drm driver is gone.  We can safely drop drm pci driver state.  */
	error = drm_pci_detach(&sc->sc_drm_dev, flags);
	if (error)
		return error;

	return 0;
}

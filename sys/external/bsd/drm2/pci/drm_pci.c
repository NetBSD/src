/*	$NetBSD: drm_pci.c,v 1.45 2021/12/19 11:09:48 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_pci.c,v 1.45 2021/12/19 11:09:48 riastradh Exp $");

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>

#include <linux/err.h>
#include <drm/drm_agpsupport.h>
#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <drm/drm_legacy.h>
#include <drm/drm_pci.h>

#include "../dist/drm/drm_internal.h"

struct drm_bus_irq_cookie {
	pci_intr_handle_t *intr_handles;
	void *ih_cookie;
};

static const struct pci_attach_args *
drm_pci_attach_args(struct drm_device *dev)
{
	return &dev->pdev->pd_pa;
}

int
drm_pci_attach(struct drm_device *dev, const struct pci_attach_args *pa,
    struct pci_dev *pdev)
{
	device_t self = dev->dev;
	unsigned int unit;
	int ret;

	/* Ensure the drm agp hooks are initialized.  */
	/* XXX errno NetBSD->Linux */
	ret = -drm_guarantee_initialized();
	if (ret)
		return ret;

	dev->pdev = pdev;

	/* XXX Set the power state to D0?  */

	/* Set up the bus space and bus DMA tags.  */
	dev->bst = pa->pa_memt;
	dev->bus_dmat = (pci_dma64_available(pa)? pa->pa_dmat64 : pa->pa_dmat);
	dev->bus_dmat32 = pa->pa_dmat;
	dev->dmat = dev->bus_dmat;
	dev->dmat_subregion_p = false;
	dev->dmat_subregion_min = 0;
	dev->dmat_subregion_max = __type_max(bus_addr_t);

	/* Find all the memory maps.  */
	CTASSERT(PCI_NUM_RESOURCES < (SIZE_MAX / sizeof(dev->bus_maps[0])));
	dev->bus_maps = kmem_zalloc(PCI_NUM_RESOURCES *
	    sizeof(dev->bus_maps[0]), KM_SLEEP);
	dev->bus_nmaps = PCI_NUM_RESOURCES;
	for (unit = 0; unit < PCI_NUM_RESOURCES; unit++) {
		struct drm_bus_map *const bm = &dev->bus_maps[unit];
		const int reg = PCI_BAR(unit);
		const pcireg_t type =
		    pci_mapreg_type(pa->pa_pc, pa->pa_tag, reg);

		/* Reject non-memory mappings.  */
		if ((type & PCI_MAPREG_TYPE_MEM) != PCI_MAPREG_TYPE_MEM) {
			aprint_debug_dev(self, "map %u has non-memory type:"
			    " 0x%"PRIxMAX"\n", unit, (uintmax_t)type);
			continue;
		}

		/*
		 * If it's a 64-bit mapping, don't interpret the second
		 * half of it as another BAR in the next iteration of
		 * the loop -- move on to the next unit.
		 */
		if (PCI_MAPREG_MEM_TYPE(type) == PCI_MAPREG_MEM_TYPE_64BIT)
			unit++;

		/* Inquire about it.  We'll map it in drm_legacy_ioremap.  */
		if (pci_mapreg_info(pa->pa_pc, pa->pa_tag, reg, type,
			&bm->bm_base, &bm->bm_size, &bm->bm_flags) != 0) {
			aprint_debug_dev(self, "map %u failed\n", unit);
			continue;
		}

		/* Assume since it is a memory mapping it can be linear.  */
		bm->bm_flags |= BUS_SPACE_MAP_LINEAR;
	}

	/* Set up AGP stuff if requested.  */
	if (drm_core_check_feature(dev, DRIVER_USE_AGP)) {
		if (pci_find_capability(dev->pdev, PCI_CAP_ID_AGP))
			dev->agp = drm_agp_init(dev);
		if (dev->agp)
			dev->agp->agp_mtrr = arch_phys_wc_add(dev->agp->base,
				dev->agp->agp_info.aki_info.ai_aperture_size);
	}

	/* Success!  */
	return 0;
}

void
drm_pci_detach(struct drm_device *dev)
{

	/* Tear down AGP stuff if necessary.  */
	drm_pci_agp_destroy(dev);

	/* Free the record of available bus space mappings.  */
	dev->bus_nmaps = 0;
	kmem_free(dev->bus_maps, PCI_NUM_RESOURCES * sizeof(dev->bus_maps[0]));

	/* Tear down bus space and bus DMA tags.  */
	if (dev->dmat_subregion_p) {
		bus_dmatag_destroy(dev->dmat);
	}
}

void
drm_pci_agp_destroy(struct drm_device *dev)
{

	if (dev->agp) {
		arch_phys_wc_del(dev->agp->agp_mtrr);
		drm_agp_fini(dev);
		KASSERT(dev->agp == NULL);
	}
}

int
drm_pci_request_irq(struct drm_device *dev, int flags)
{
	const char *const name = device_xname(dev->dev);
	int (*const handler)(void *) = dev->driver->irq_handler;
	const struct pci_attach_args *const pa = drm_pci_attach_args(dev);
	const char *intrstr;
	char intrbuf[PCI_INTRSTR_LEN];
	struct drm_bus_irq_cookie *irq_cookie;

	irq_cookie = kmem_alloc(sizeof(*irq_cookie), KM_SLEEP);

	if (dev->pdev->msi_enabled) {
		if (dev->pdev->pd_intr_handles == NULL) {
			if (pci_msi_alloc_exact(pa, &irq_cookie->intr_handles,
			    1)) {
				aprint_error_dev(dev->dev,
				    "couldn't allocate MSI (%s)\n", name);
				goto error;
			}
		} else {
			irq_cookie->intr_handles = dev->pdev->pd_intr_handles;
			dev->pdev->pd_intr_handles = NULL;
		}
	} else {
		if (pci_intx_alloc(pa, &irq_cookie->intr_handles)) {
			aprint_error_dev(dev->dev,
			    "couldn't allocate INTx interrupt (%s)\n", name);
			goto error;
		}
	}

	intrstr = pci_intr_string(pa->pa_pc, irq_cookie->intr_handles[0],
	    intrbuf, sizeof(intrbuf));
	irq_cookie->ih_cookie = pci_intr_establish_xname(pa->pa_pc,
	    irq_cookie->intr_handles[0], IPL_DRM, handler, dev, name);
	if (irq_cookie->ih_cookie == NULL) {
		aprint_error_dev(dev->dev,
		    "couldn't establish interrupt at %s (%s)\n", intrstr, name);
		pci_intr_release(pa->pa_pc, irq_cookie->intr_handles, 1);
		goto error;
	}

	aprint_normal_dev(dev->dev, "interrupting at %s (%s)\n", intrstr, name);
	dev->irq_cookie = irq_cookie;
	return 0;

error:
	kmem_free(irq_cookie, sizeof(*irq_cookie));
	return -ENOENT;
}

void
drm_pci_free_irq(struct drm_device *dev)
{
	struct drm_bus_irq_cookie *const cookie = dev->irq_cookie;
	const struct pci_attach_args *pa = drm_pci_attach_args(dev);

	pci_intr_disestablish(pa->pa_pc, cookie->ih_cookie);
	pci_intr_release(pa->pa_pc, cookie->intr_handles, 1);
	kmem_free(cookie, sizeof(*cookie));
	dev->irq_cookie = NULL;
}

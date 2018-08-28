/*	$NetBSD: drm_pci.c,v 1.30 2018/08/28 03:34:39 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_pci.c,v 1.30 2018/08/28 03:34:39 riastradh Exp $");

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>

#include <drm/drmP.h>
#include <drm/drm_internal.h>
#include <drm/drm_legacy.h>

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
drm_pci_attach(device_t self, const struct pci_attach_args *pa,
    struct pci_dev *pdev, struct drm_driver *driver, unsigned long cookie,
    struct drm_device **devp)
{
	struct drm_device *dev;
	unsigned int unit;
	int ret;

	/* Ensure the drm agp hooks are installed.  */
	/* XXX errno NetBSD->Linux */
	ret = -drmkms_pci_agp_guarantee_initialized();
	if (ret)
		goto fail0;

	/* Create a DRM device.  */
	dev = drm_dev_alloc(driver, self);
	if (dev == NULL) {
		ret = -ENOMEM;
		goto fail0;
	}

	dev->pdev = pdev;
	pdev->pd_drm_dev = dev;	/* XXX Nouveau kludge.  */

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
		if (drm_pci_device_is_agp(dev))
			dev->agp = drm_agp_init(dev);
		if (dev->agp)
			dev->agp->agp_mtrr = arch_phys_wc_add(dev->agp->base,
				dev->agp->agp_info.aki_info.ai_aperture_size);
	}

	/* Register the DRM device and do driver-specific initialization.  */
	ret = drm_dev_register(dev, cookie);
	if (ret)
		goto fail1;

	/* Success!  */
	*devp = dev;
	return 0;

fail2: __unused
	drm_dev_unregister(dev);
fail1:	drm_pci_agp_destroy(dev);
	dev->bus_nmaps = 0;
	kmem_free(dev->bus_maps, PCI_NUM_RESOURCES * sizeof(dev->bus_maps[0]));
	if (dev->dmat_subregion_p) {
		bus_dmatag_destroy(dev->dmat);
	}
	drm_dev_unref(dev);
fail0:	return ret;
}

int
drm_pci_detach(struct drm_device *dev, int flags __unused)
{

	/* Do driver-specific detachment and unregister the device.  */
	drm_dev_unregister(dev);

	/* Tear down AGP stuff if necessary.  */
	drm_pci_agp_destroy(dev);

	/* Free the record of available bus space mappings.  */
	dev->bus_nmaps = 0;
	kmem_free(dev->bus_maps, PCI_NUM_RESOURCES * sizeof(dev->bus_maps[0]));

	/* Tear down bus space and bus DMA tags.  */
	if (dev->dmat_subregion_p) {
		bus_dmatag_destroy(dev->dmat);
	}

	drm_dev_unref(dev);

	return 0;
}

void
drm_pci_agp_destroy(struct drm_device *dev)
{

	if (dev->agp) {
		arch_phys_wc_del(dev->agp->agp_mtrr);
		drm_agp_clear(dev);
		kfree(dev->agp); /* XXX Should go in drm_agp_clear...  */
		dev->agp = NULL;
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

int
drm_pci_set_busid(struct drm_device *dev, struct drm_master *master)
{
	const struct pci_attach_args *const pa = &dev->pdev->pd_pa;

	master->unique = kasprintf(GFP_KERNEL, "pci:%04x:%02x:%02x.%d",
	    device_unit(device_parent(dev->dev)),
	    pa->pa_bus, pa->pa_device, pa->pa_function);
	if (master->unique == NULL)
		return -ENOMEM;
	master->unique_len = strlen(master->unique);

	return 0;
}

int
drm_pci_set_unique_impl(struct drm_device *dev, struct drm_master *master,
    struct drm_unique *unique)
{
	char kbuf[64], ubuf[64];
	int ret;

	/* Reject excessively long unique strings.  */
	if (unique->unique_len > sizeof(ubuf) - 1)
		return -EINVAL;

	/* Copy in the alleged unique string, NUL-terminated.  */
	ret = -copyin(unique->unique, ubuf, unique->unique_len);
	if (ret)
		return ret;
	ubuf[unique->unique_len] = '\0';

	/* Make sure it matches what we expect.  */
	snprintf(kbuf, sizeof kbuf, "PCI:%d:%ld:%ld", dev->pdev->bus->number,
	    (long)PCI_SLOT(dev->pdev->devfn),
	    (long)PCI_FUNC(dev->pdev->devfn));
	if (strncmp(kbuf, ubuf, sizeof(kbuf)) != 0)
		return -EINVAL;

	/* Remember it.  */
	master->unique = kstrdup(ubuf, GFP_KERNEL);
	master->unique_len = strlen(master->unique);

	/* Success!  */
	return 0;
}

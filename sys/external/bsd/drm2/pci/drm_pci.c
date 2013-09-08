/*	$NetBSD: drm_pci.c,v 1.1.2.10 2013/09/08 16:15:17 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_pci.c,v 1.1.2.10 2013/09/08 16:15:17 riastradh Exp $");

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>

#include <drm/drmP.h>

static int	drm_pci_get_irq(struct drm_device *);
static int	drm_pci_irq_install(struct drm_device *,
		    irqreturn_t (*)(void *), int, const char *, void *,
		    struct drm_bus_irq_cookie **);
static void	drm_pci_irq_uninstall(struct drm_device *,
		    struct drm_bus_irq_cookie *);
static const char *
		drm_pci_get_name(struct drm_device *);
static int	drm_pci_set_busid(struct drm_device *, struct drm_master *);
static int	drm_pci_set_unique(struct drm_device *, struct drm_master *,
		    struct drm_unique *);
static int	drm_pci_irq_by_busid(struct drm_device *,
		    struct drm_irq_busid *);
static int	drm_pci_agp_init(struct drm_device *);

const struct drm_bus drm_pci_bus = {
	.bus_type = DRIVER_BUS_PCI,
	.get_irq = drm_pci_get_irq,
	.irq_install = drm_pci_irq_install,
	.irq_uninstall = drm_pci_irq_uninstall,
	.get_name = drm_pci_get_name,
	.set_busid = drm_pci_set_busid,
	.set_unique = drm_pci_set_unique,
	.irq_by_busid = drm_pci_irq_by_busid,
	.agp_init = drm_pci_agp_init,
};

static const struct pci_attach_args *
drm_pci_attach_args(struct drm_device *dev)
{
	return &dev->pdev->pd_pa;
}

int
drm_pci_init(struct drm_driver *driver, struct pci_driver *pdriver __unused)
{

	driver->bus = &drm_pci_bus;
	return 0;
}

void
drm_pci_exit(struct drm_driver *driver __unused,
    struct pci_driver *pdriver __unused)
{
}

/* XXX Should go elsewhere.  */
#define	PCI_NMAPREGS	((PCI_MAPREG_END - PCI_MAPREG_START) / 4)

void
drm_pci_attach(device_t self, const struct pci_attach_args *pa,
    struct pci_dev *pdev, struct drm_device *dev)
{
	unsigned int unit;

	linux_pci_dev_init(pdev, self, pa, 0);

	dev->pdev = pdev;
	dev->pci_vendor = pdev->vendor;
	dev->pci_device = pdev->device;

	/* XXX Set the power state to D0?  */

	dev->bst = pa->pa_memt;
	/* XXX Let the driver say something about 32-bit vs 64-bit DMA?  */
	dev->bus_dmat = (pci_dma64_available(pa)? pa->pa_dmat64 : pa->pa_dmat);
	dev->dmat = dev->bus_dmat;
	dev->dmat_subregion_p = false;

	CTASSERT(PCI_NMAPREGS < (SIZE_MAX / sizeof(dev->bus_maps[0])));
	dev->bus_nmaps = PCI_NMAPREGS;
	dev->bus_maps = kmem_zalloc((PCI_NMAPREGS * sizeof(dev->bus_maps[0])),
	    KM_SLEEP);
	for (unit = 0; unit < dev->bus_nmaps; unit++) {
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

		/* Inquire about it.  We'll map it in drm_ioremap.  */
		if (pci_mapreg_info(pa->pa_pc, pa->pa_tag, reg, type,
			&bm->bm_base, &bm->bm_size, &bm->bm_flags) != 0) {
			aprint_debug_dev(self, "map %u failed\n", unit);
			continue;
		}

		aprint_debug_dev(self, "map %u at %"PRIxMAX" size %"PRIxMAX
		    " flags 0x%"PRIxMAX"\n",
		    unit, (uintmax_t)bm->bm_base, (uintmax_t)bm->bm_size,
		    (uintmax_t)bm->bm_flags);

		/* Assume since it is a memory mapping it can be linear.  */
		bm->bm_flags |= BUS_SPACE_MAP_LINEAR;
	}

	/* All `agp' maps are just initialized to zero.  */
	CTASSERT(PCI_NMAPREGS < (SIZE_MAX / sizeof(dev->agp_maps[0])));
	dev->agp_nmaps = PCI_NMAPREGS;
	dev->agp_maps = kmem_zalloc((PCI_NMAPREGS * sizeof(dev->agp_maps[0])),
	    KM_SLEEP);
}

int
drm_pci_detach(struct drm_device *dev __unused, int flags __unused)
{

	kmem_free(dev->bus_maps, (PCI_NMAPREGS * sizeof(dev->bus_maps[0])));
	kmem_free(dev->agp_maps, (PCI_NMAPREGS * sizeof(dev->agp_maps[0])));

	/*
	 * XXX This is the wrong place!  Should be a routine in
	 * drm_memory.c.
	 */
	if (dev->dmat_subregion_p)
		bus_dmatag_destroy(dev->dmat);

	/* XXX Disestablish irqs or anything?  */
	return 0;
}

static int
drm_pci_get_irq(struct drm_device *dev)
{
	pci_intr_handle_t ih_pih;
	int ih_int;

	/*
	 * This is a compile-time assertion that the types match.  If
	 * this fails, we have to change a bunch of drm code that uses
	 * int for intr handles.
	 */
	KASSERT(&ih_pih != &ih_int);

	if (pci_intr_map(drm_pci_attach_args(dev), &ih_pih))
		return -1;	/* XXX Hope -1 is an invalid intr handle.  */

	ih_int = ih_pih;
	return ih_int;
}

static int
drm_pci_irq_install(struct drm_device *dev, irqreturn_t (*handler)(void *),
    int flags, const char *name, void *arg,
    struct drm_bus_irq_cookie **cookiep)
{
	const struct pci_attach_args *const pa = drm_pci_attach_args(dev);
	pci_intr_handle_t ih;
	const char *intrstr;
	void *ih_cookie;

	if (pci_intr_map(pa, &ih))
		return -ENOENT;

	intrstr = pci_intr_string(pa->pa_pc, ih);
	ih_cookie = pci_intr_establish(pa->pa_pc, ih, IPL_DRM, handler, arg);
	if (ih_cookie == NULL) {
		aprint_error_dev(dev->dev,
		    "couldn't establish interrupt at %s (%s)\n",
		    intrstr, name);
		return -ENOENT;
	}

	aprint_normal_dev(dev->dev, "interrupting at %s (%s)\n",
	    intrstr, name);
	*cookiep = (struct drm_bus_irq_cookie *)ih_cookie;
	return 0;
}

static void
drm_pci_irq_uninstall(struct drm_device *dev,
    struct drm_bus_irq_cookie *cookie)
{
	const struct pci_attach_args *pa = drm_pci_attach_args(dev);

	pci_intr_disestablish(pa->pa_pc, (void *)cookie);
}

static const char *
drm_pci_get_name(struct drm_device *dev)
{
	return "pci";		/* XXX PCI bus names?  */
}

static int
drm_pci_set_busid(struct drm_device *dev, struct drm_master *master)
{
	return -ENOSYS;		/* XXX PCI bus ids?  */
}

static int
drm_pci_set_unique(struct drm_device *dev, struct drm_master *master,
    struct drm_unique *unique)
{
	return -ENOSYS;		/* XXX */
}

static int
drm_pci_irq_by_busid(struct drm_device *dev, struct drm_irq_busid *busid)
{
	return -ENOSYS;		/* XXX */
}

static int
drm_pci_agp_init(struct drm_device *dev)
{
	return 0;		/* XXX */
}

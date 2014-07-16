/*	$NetBSD: radeon_pci.c,v 1.1 2014/07/16 20:59:58 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: radeon_pci.c,v 1.1 2014/07/16 20:59:58 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "vga.h"
#endif

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/workqueue.h>

#include <dev/pci/pciio.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/pci/wsdisplay_pci.h>
#include <dev/wsfb/genfbvar.h>

#if NVGA > 0
/*
 * XXX All we really need is vga_is_console from vgavar.h, but the
 * header files are missing their own dependencies, so we need to
 * explicitly drag in the other crap.
 */
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif

#include <drm/drmP.h>
#include <drm/drm_fb_helper.h>

#include <radeon.h>
#include "radeon_drv.h"

struct radeon_genfb_work;
SIMPLEQ_HEAD(radeon_genfb_work_head, radeon_genfb_work);

struct radeon_softc {
	device_t			sc_dev;
	struct workqueue		*sc_genfb_wq;
	struct radeon_genfb_work_head	sc_genfb_work;
	struct drm_device		*sc_drm_dev;
	struct pci_dev			sc_pci_dev;
};

struct radeon_genfb_work {
	struct drm_fb_helper	*rgw_fb_helper;
	union {
		SIMPLEQ_ENTRY(radeon_genfb_work)	queue;
		struct work				work;
	}			rgw_u;
};

static bool	radeon_pci_lookup(const struct pci_attach_args *,
		    unsigned long *);

static int	radeon_match(device_t, cfdata_t, void *);
static void	radeon_attach(device_t, device_t, void *);
static int	radeon_detach(device_t, int);

static void	radeon_genfb_defer_set_config(struct drm_fb_helper *);
static void	radeon_genfb_set_config_work(struct work *, void *);
static void	radeon_genfb_set_config(struct radeon_genfb_work *);
static int	radeon_genfb_ioctl(void *, void *, unsigned long, void *,
		    int, struct lwp *);
static paddr_t	radeon_genfb_mmap(void *, void *, off_t, int);

CFATTACH_DECL_NEW(radeondrmkms, sizeof(struct radeon_softc),
    radeon_match, radeon_attach, radeon_detach, NULL);

/* XXX Kludge to get these from radeon_drv.c.  */
extern struct drm_driver *const radeon_drm_driver;
extern const struct pci_device_id *const radeon_device_ids;
extern const size_t radeon_n_device_ids;

static bool
radeon_pci_lookup(const struct pci_attach_args *pa, unsigned long *flags)
{
	size_t i;

	for (i = 0; i < radeon_n_device_ids; i++) {
		if ((PCI_VENDOR(pa->pa_id) == radeon_device_ids[i].vendor) &&
		    (PCI_PRODUCT(pa->pa_id) == radeon_device_ids[i].device))
			break;
	}

	/* Did we find it?  */
	if (i == radeon_n_device_ids)
		return false;

	if (flags)
		*flags = radeon_device_ids[i].driver_data;
	return true;
}

static int
radeon_match(device_t parent, cfdata_t match, void *aux)
{
	extern int radeon_guarantee_initialized(void);
	const struct pci_attach_args *const pa = aux;
	int error;

	error = radeon_guarantee_initialized();
	if (error) {
		aprint_error("radeon: failed to initialize: %d\n", error);
		return 0;
	}

	if (!radeon_pci_lookup(pa, NULL))
		return 0;

	return 6;		/* XXX Beat genfb_pci...  */
}

static void
radeon_attach(device_t parent, device_t self, void *aux)
{
	struct radeon_softc *const sc = device_private(self);
	const struct pci_attach_args *const pa = aux;
	bool ok __diagused;
	unsigned long flags;
	int error;

	ok = radeon_pci_lookup(pa, &flags);
	KASSERT(ok);

	sc->sc_dev = self;

	pci_aprint_devinfo(pa, NULL);

	SIMPLEQ_INIT(&sc->sc_genfb_work);

	/* XXX errno Linux->NetBSD */
	error = -drm_pci_attach(self, pa, &sc->sc_pci_dev, radeon_drm_driver,
	    flags, &sc->sc_drm_dev);
	if (error) {
		aprint_error_dev(self, "unable to attach drm: %d\n", error);
		return;
	}

	while (!SIMPLEQ_EMPTY(&sc->sc_genfb_work)) {
		struct radeon_genfb_work *const work =
		    SIMPLEQ_FIRST(&sc->sc_genfb_work);

		SIMPLEQ_REMOVE_HEAD(&sc->sc_genfb_work, rgw_u.queue);
		radeon_genfb_set_config(work);
	}

	error = workqueue_create(&sc->sc_genfb_wq, "radeonfb",
	    &radeon_genfb_set_config_work, NULL, PRI_NONE, IPL_NONE,
	    WQ_MPSAFE);
	if (error) {
		aprint_error_dev(self, "unable to create workqueue: %d\n",
		    error);
		return;
	}
}

static int
radeon_detach(device_t self, int flags)
{
	struct radeon_softc *const sc = device_private(self);
	int error;

	/* XXX Check for in-use before tearing it all down...  */
	error = config_detach_children(self, flags);
	if (error)
		return error;

	if (sc->sc_genfb_wq == NULL)
		return 0;
	workqueue_destroy(sc->sc_genfb_wq);

	if (sc->sc_drm_dev == NULL)
		return 0;
	/* XXX errno Linux->NetBSD */
	error = -drm_pci_detach(sc->sc_drm_dev, flags);
	if (error)
		return error;

	return 0;
}

int
radeon_genfb_attach(struct drm_device *dev, struct drm_fb_helper *helper,
    const struct drm_fb_helper_surface_size *sizes, struct radeon_bo *rbo)
{
	struct radeon_softc *const sc = container_of(dev->pdev,
	    struct radeon_softc, sc_pci_dev);
	static const struct genfb_ops zero_genfb_ops;
	struct genfb_ops genfb_ops = zero_genfb_ops;
	const prop_dictionary_t dict = device_properties(sc->sc_dev);
	enum { CONS_VGA, CONS_GENFB, CONS_NONE } what_was_cons;
	int ret;

#if NVGA > 0
	if (vga_is_console(dev->pdev->pd_pa.pa_iot, -1) ||
	    vga_is_console(dev->pdev->pd_pa.pa_iot, -1)) {
		what_was_cons = CONS_VGA;
		prop_dictionary_set_bool(dict, "is_console", true);
		/*
		 * There is a window from here until genfb attaches in
		 * which kernel messages will go into a black hole,
		 * until genfb replays the console.  Whattakludge.
		 *
		 * wsdisplay_cndetach must come first, to clear cn_tab,
		 * so that nothing will use it; then vga_cndetach
		 * unmaps the bus space that it would have used.
		 */
		wsdisplay_cndetach();
		vga_cndetach();
	} else
#endif
	if (genfb_is_console() && genfb_is_enabled()) {
		what_was_cons = CONS_GENFB;
		prop_dictionary_set_bool(dict, "is_console", true);
	} else {
		what_was_cons = CONS_NONE;
		prop_dictionary_set_bool(dict, "is_console", false);
	}

	/* XXX Ugh...  Pass these parameters some other way!  */
	prop_dictionary_set_uint32(dict, "width", sizes->fb_width);
	prop_dictionary_set_uint32(dict, "height", sizes->fb_height);
	prop_dictionary_set_uint8(dict, "depth", sizes->surface_bpp);
	prop_dictionary_set_uint16(dict, "linebytes",
	    roundup2((sizes->fb_width * howmany(sizes->surface_bpp, 8)), 64));
	prop_dictionary_set_uint32(dict, "address", 0); /* XXX >32-bit */
	CTASSERT(sizeof(uintptr_t) <= sizeof(uint64_t));
	prop_dictionary_set_uint64(dict, "virtual_address",
	    (uint64_t)(uintptr_t)rbo->kptr);

	helper->genfb.sc_dev = sc->sc_dev;
	genfb_init(&helper->genfb);
	genfb_ops.genfb_ioctl = radeon_genfb_ioctl;
	genfb_ops.genfb_mmap = radeon_genfb_mmap;

	/* XXX errno NetBSD->Linux */
	ret = -genfb_attach(&helper->genfb, &genfb_ops);
	if (ret) {
		DRM_ERROR("failed to attach genfb: %d\n", ret);
		switch (what_was_cons) { /* XXX Restore console...  */
		case CONS_VGA: break;
		case CONS_GENFB: break;
		case CONS_NONE: break;
		default: break;
		}
		return ret;
	}

	radeon_genfb_defer_set_config(helper);

	return 0;
}

static void
radeon_genfb_defer_set_config(struct drm_fb_helper *helper)
{
	struct drm_device *const dev = helper->dev;
	struct radeon_softc *const sc = container_of(dev->pdev,
	    struct radeon_softc, sc_pci_dev);
	struct radeon_genfb_work *work;

	/* Really shouldn't sleep here...  */
	work = kmem_alloc(sizeof(*work), KM_SLEEP);
	work->rgw_fb_helper = helper;

	if (sc->sc_genfb_wq == NULL) /* during attachment */
		SIMPLEQ_INSERT_TAIL(&sc->sc_genfb_work, work, rgw_u.queue);
	else
		workqueue_enqueue(sc->sc_genfb_wq, &work->rgw_u.work, NULL);
}

static void
radeon_genfb_set_config_work(struct work *work, void *cookie __unused)
{

	radeon_genfb_set_config(container_of(work, struct radeon_genfb_work,
		rgw_u.work));
}

static void
radeon_genfb_set_config(struct radeon_genfb_work *work)
{

	drm_fb_helper_set_config(work->rgw_fb_helper);
	kmem_free(work, sizeof(*work));
}

static int
radeon_genfb_ioctl(void *v, void *vs, unsigned long cmd, void *data, int flag,
    struct lwp *l)
{
	struct genfb_softc *const genfb = v;
	struct drm_fb_helper *const helper = container_of(genfb,
	    struct drm_fb_helper, genfb);
	struct drm_device *const dev = helper->dev;
	const struct pci_attach_args *const pa = &dev->pdev->pd_pa;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(unsigned int *)data = WSDISPLAY_TYPE_PCIVGA;
		return 0;

	/* PCI config read/write passthrough.  */
	case PCI_IOC_CFGREAD:
	case PCI_IOC_CFGWRITE:
		return pci_devioctl(pa->pa_pc, pa->pa_tag, cmd, data, flag, l);

	case WSDISPLAYIO_GET_BUSID:
		return wsdisplayio_busid_pci(genfb->sc_dev,
		    pa->pa_pc, pa->pa_tag, data);

	default:
		return EPASSTHROUGH;
	}
}

static paddr_t
radeon_genfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct genfb_softc *const genfb = v;
	struct drm_fb_helper *const helper = container_of(genfb,
	    struct drm_fb_helper, genfb);
	struct drm_framebuffer *const fb = helper->fb;
	struct radeon_framebuffer *const rfb = container_of(fb,
	    struct radeon_framebuffer, base);
	struct drm_gem_object *const gobj = rfb->obj;
	struct radeon_bo *const rbo = gem_to_radeon_bo(gobj);
	struct drm_device *const dev = helper->dev;
	const struct pci_attach_args *const pa = &dev->pdev->pd_pa;
	unsigned int i;

	if (offset < 0)
		return -1;

	/* Treat low memory as the framebuffer itself.  */
	if (offset < genfb->sc_fbsize) {
		const unsigned num_pages __diagused = rbo->tbo.num_pages;
		bus_addr_t addr;
		int flags = 0;

		KASSERT(genfb->sc_fbsize == (num_pages << PAGE_SHIFT));
		KASSERT(num_pages == rbo->tbo.ttm->num_pages);
		addr = page_to_phys(rbo->tbo.ttm->pages[offset >> PAGE_SHIFT]);
		/* XXX CACHEABLE?  PREFETCHABLE?  WC?  WB?  */
		if (ISSET(rbo->tbo.mem.placement, TTM_PL_FLAG_CACHED))
			flags |= BUS_SPACE_MAP_PREFETCHABLE;
		/*
		 * XXX Urk.  We assume bus_space_mmap can cope with
		 * normal system RAM addresses.
		 */
		return bus_space_mmap(rbo->tbo.bdev->memt, addr, 0, prot,
		    flags);
	}

	/* XXX Cargo-culted from genfb_pci.  */
	if (kauth_authorize_machdep(kauth_cred_get(),
		KAUTH_MACHDEP_UNMANAGEDMEM, NULL, NULL, NULL, NULL) != 0) {
		aprint_normal_dev(dev->dev, "mmap at %"PRIxMAX" rejected\n",
		    (uintmax_t)offset);
		return -1;
	}

	for (i = 0; PCI_BAR(i) <= PCI_MAPREG_ROM; i++) {
		pcireg_t type;
		bus_addr_t addr;
		bus_size_t size;
		int flags;

		/* Interrogate the BAR.  */
		if (!pci_mapreg_probe(pa->pa_pc, pa->pa_tag, PCI_BAR(i),
			&type))
			continue;
		if (PCI_MAPREG_TYPE(type) != PCI_MAPREG_TYPE_MEM)
			continue;
		if (pci_mapreg_info(pa->pa_pc, pa->pa_tag, PCI_BAR(i), type,
			&addr, &size, &flags))
			continue;

		/* Try to map it if it's in range.  */
		if ((addr <= offset) && (offset < (addr + size)))
			return bus_space_mmap(pa->pa_memt, addr,
			    (offset - addr), prot, flags);

		/* Skip a slot if this was a 64-bit BAR.  */
		if ((PCI_MAPREG_TYPE(type) == PCI_MAPREG_TYPE_MEM) &&
		    (PCI_MAPREG_MEM_TYPE(type) == PCI_MAPREG_MEM_TYPE_64BIT))
			i += 1;
	}

	/* Failure!  */
	return -1;
}

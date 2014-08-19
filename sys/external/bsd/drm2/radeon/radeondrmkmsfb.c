/*	$NetBSD: radeondrmkmsfb.c,v 1.3.6.2 2014/08/20 00:04:22 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: radeondrmkmsfb.c,v 1.3.6.2 2014/08/20 00:04:22 tls Exp $");

#ifdef _KERNEL_OPT
#include "vga.h"
#endif

#include <sys/types.h>
#include <sys/device.h>

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
#include "radeon_task.h"
#include "radeondrmkmsfb.h"

struct radeonfb_softc {
	/* XXX genfb requires the genfb_softc to be first.  */
	struct genfb_softc		sc_genfb;
	device_t			sc_dev;
	struct radeonfb_attach_args	sc_rfa;
	struct radeon_task		sc_setconfig_task;
	bool				sc_scheduled:1;
	bool				sc_attached:1;
};

static int	radeonfb_match(device_t, cfdata_t, void *);
static void	radeonfb_attach(device_t, device_t, void *);
static int	radeonfb_detach(device_t, int);

static void	radeonfb_setconfig_task(struct radeon_task *);

static int	radeonfb_genfb_ioctl(void *, void *, unsigned long, void *,
		    int, struct lwp *);
static paddr_t	radeonfb_genfb_mmap(void *, void *, off_t, int);
static int	radeonfb_genfb_enable_polling(void *);
static int	radeonfb_genfb_disable_polling(void *);

CFATTACH_DECL_NEW(radeondrmkmsfb, sizeof(struct radeonfb_softc),
    radeonfb_match, radeonfb_attach, radeonfb_detach, NULL);

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
	sc->sc_scheduled = false;
	sc->sc_attached = false;

	aprint_naive("\n");
	aprint_normal("\n");

	radeon_task_init(&sc->sc_setconfig_task, &radeonfb_setconfig_task);
	error = radeon_task_schedule(parent, &sc->sc_setconfig_task);
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
radeonfb_detach(device_t self, int flags)
{
	struct radeonfb_softc *const sc = device_private(self);

	if (sc->sc_scheduled)
		return EBUSY;

	if (sc->sc_attached) {
		/* XXX genfb detach?  Help?  */
		sc->sc_attached = false;
	}

	return 0;
}

static void
radeonfb_setconfig_task(struct radeon_task *task)
{
	struct radeonfb_softc *const sc = container_of(task,
	    struct radeonfb_softc, sc_setconfig_task);
	const prop_dictionary_t dict = device_properties(sc->sc_dev);
	const struct radeonfb_attach_args *const rfa = &sc->sc_rfa;
	const struct drm_fb_helper_surface_size *const sizes =
	    &rfa->rfa_fb_sizes;
	enum { CONS_VGA, CONS_GENFB, CONS_NONE } what_was_cons;
	static const struct genfb_ops zero_genfb_ops;
	struct genfb_ops genfb_ops = zero_genfb_ops;
	int error;

	KASSERT(sc->sc_scheduled);

	/* XXX Ugh...  Pass these parameters some other way!  */
	prop_dictionary_set_uint32(dict, "width", sizes->fb_width);
	prop_dictionary_set_uint32(dict, "height", sizes->fb_height);
	prop_dictionary_set_uint8(dict, "depth", sizes->surface_bpp);
	prop_dictionary_set_uint16(dict, "linebytes",
	    roundup2((sizes->fb_width * howmany(sizes->surface_bpp, 8)), 64));
	prop_dictionary_set_uint32(dict, "address", 0); /* XXX >32-bit */
	CTASSERT(sizeof(uintptr_t) <= sizeof(uint64_t));
	prop_dictionary_set_uint64(dict, "virtual_address",
	    (uint64_t)(uintptr_t)rfa->rfa_fb_ptr);

	/* XXX Whattakludge!  */
#if NVGA > 0
	if (vga_is_console(rfa->rfa_fb_helper->dev->pdev->pd_pa.pa_iot, -1)) {
		what_was_cons = CONS_VGA;
		prop_dictionary_set_bool(dict, "is_console", true);
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

	sc->sc_genfb.sc_dev = sc->sc_dev;
	genfb_init(&sc->sc_genfb);
	genfb_ops.genfb_ioctl = radeonfb_genfb_ioctl;
	genfb_ops.genfb_mmap = radeonfb_genfb_mmap;
	genfb_ops.genfb_enable_polling = radeonfb_genfb_enable_polling;
	genfb_ops.genfb_disable_polling = radeonfb_genfb_disable_polling;

	error = genfb_attach(&sc->sc_genfb, &genfb_ops);
	if (error) {
		aprint_error_dev(sc->sc_dev, "failed to attach genfb: %d\n",
		    error);
		goto fail0;
	}
	sc->sc_attached = true;

	drm_fb_helper_set_config(sc->sc_rfa.rfa_fb_helper);

	/* Success!  */
	sc->sc_scheduled = false;
	return;

fail0:	/* XXX Restore console...  */
	switch (what_was_cons) {
	case CONS_VGA:
		break;
	case CONS_GENFB:
		break;
	case CONS_NONE:
		break;
	default:
		break;
	}
}

static int
radeonfb_genfb_ioctl(void *v, void *vs, unsigned long cmd, void *data, int flag,
    struct lwp *l)
{
	struct genfb_softc *const genfb = v;
	struct radeonfb_softc *const sc = container_of(genfb,
	    struct radeonfb_softc, sc_genfb);
	struct drm_device *const dev = sc->sc_rfa.rfa_fb_helper->dev;
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
		return wsdisplayio_busid_pci(dev->dev, pa->pa_pc, pa->pa_tag,
		    data);

	default:
		return EPASSTHROUGH;
	}
}

static paddr_t
radeonfb_genfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct genfb_softc *const genfb = v;
	struct radeonfb_softc *const sc = container_of(genfb,
	    struct radeonfb_softc, sc_genfb);
	struct drm_fb_helper *const helper = sc->sc_rfa.rfa_fb_helper;
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
		int flags = 0;

		KASSERT(genfb->sc_fbsize == (num_pages << PAGE_SHIFT));
		KASSERT(rbo->tbo.mem.bus.is_iomem);

		if (ISSET(rbo->tbo.mem.placement, TTM_PL_FLAG_WC))
			flags |= BUS_SPACE_MAP_PREFETCHABLE;

		return bus_space_mmap(rbo->tbo.bdev->memt,
		    rbo->tbo.mem.bus.base, rbo->tbo.mem.bus.offset + offset,
		    prot, flags);
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

static int
radeonfb_genfb_enable_polling(void *cookie)
{
	struct genfb_softc *const genfb = cookie;
	struct radeonfb_softc *const sc = container_of(genfb,
	    struct radeonfb_softc, sc_genfb);

	return drm_fb_helper_debug_enter_fb(sc->sc_rfa.rfa_fb_helper);
}

static int
radeonfb_genfb_disable_polling(void *cookie)
{
	struct genfb_softc *const genfb = cookie;
	struct radeonfb_softc *const sc = container_of(genfb,
	    struct radeonfb_softc, sc_genfb);

	return drm_fb_helper_debug_leave_fb(sc->sc_rfa.rfa_fb_helper);
}

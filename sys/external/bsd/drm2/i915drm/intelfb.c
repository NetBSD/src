/*	$NetBSD: intelfb.c,v 1.9.2.2 2014/08/10 06:55:39 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: intelfb.c,v 1.9.2.2 2014/08/10 06:55:39 tls Exp $");

#ifdef _KERNEL_OPT
#include "vga.h"
#endif

#include <sys/types.h>
#include <sys/bus.h>
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

#include "i915_drv.h"
#include "i915_pci.h"
#include "intelfb.h"

struct intelfb_softc {
	/* XXX genfb requires the genfb_softc to be first.  */
	struct genfb_softc		sc_genfb;
	device_t			sc_dev;
	struct intelfb_attach_args	sc_ifa;
	bus_space_handle_t		sc_fb_bsh;
	struct i915drmkms_task		sc_setconfig_task;
	bool				sc_mapped:1;
	bool				sc_scheduled:1;
	bool				sc_attached:1;
};

static int	intelfb_match(device_t, cfdata_t, void *);
static void	intelfb_attach(device_t, device_t, void *);
static int	intelfb_detach(device_t, int);

static void	intelfb_setconfig_task(struct i915drmkms_task *);

static int	intelfb_genfb_dpms(struct drm_device *, int);
static int	intelfb_genfb_ioctl(void *, void *, unsigned long, void *,
		    int, struct lwp *);
static paddr_t	intelfb_genfb_mmap(void *, void *, off_t, int);
static int	intelfb_genfb_enable_polling(void *);
static int	intelfb_genfb_disable_polling(void *);
static bool	intelfb_genfb_shutdown(device_t, int);
static bool	intelfb_genfb_setmode(struct genfb_softc *, int);

static const struct genfb_mode_callback intelfb_genfb_mode_callback = {
	.gmc_setmode = intelfb_genfb_setmode,
};

CFATTACH_DECL_NEW(intelfb, sizeof(struct intelfb_softc),
    intelfb_match, intelfb_attach, intelfb_detach, NULL);

static int
intelfb_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

static void
intelfb_attach(device_t parent, device_t self, void *aux)
{
	struct intelfb_softc *const sc = device_private(self);
	const struct intelfb_attach_args *const ifa = aux;
	int error;

	sc->sc_dev = self;
	sc->sc_ifa = *ifa;
	sc->sc_mapped = false;
	sc->sc_scheduled = false;
	sc->sc_attached = false;

	aprint_naive("\n");
	aprint_normal("\n");

	/* XXX Defer this too?  */
	error = bus_space_map(ifa->ifa_fb_bst, ifa->ifa_fb_addr,
	    ifa->ifa_fb_size,
	    (BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE),
	    &sc->sc_fb_bsh);
	if (error) {
		aprint_error_dev(self, "unable to map framebuffer: %d\n",
		    error);
		goto fail0;
	}
	sc->sc_mapped = true;

	i915drmkms_task_init(&sc->sc_setconfig_task, &intelfb_setconfig_task);
	error = i915drmkms_task_schedule(parent, &sc->sc_setconfig_task);
	if (error) {
		aprint_error_dev(self, "failed to schedule mode set: %d\n",
		    error);
		goto fail1;
	}
	sc->sc_scheduled = true;

	/* Success!  */
	return;

fail1:	bus_space_unmap(ifa->ifa_fb_bst, sc->sc_fb_bsh, ifa->ifa_fb_size);
	sc->sc_mapped = false;
fail0:	return;
}

static int
intelfb_detach(device_t self, int flags)
{
	struct intelfb_softc *const sc = device_private(self);

	if (sc->sc_scheduled)
		return EBUSY;

	if (sc->sc_attached) {
		/* XXX genfb detach?  Help?  */
		sc->sc_attached = false;
	}

	if (sc->sc_mapped) {
		bus_space_unmap(sc->sc_ifa.ifa_fb_bst, sc->sc_fb_bsh,
		    sc->sc_ifa.ifa_fb_size);
		sc->sc_mapped = false;
	}

	return 0;
}

static void
intelfb_setconfig_task(struct i915drmkms_task *task)
{
	struct intelfb_softc *const sc = container_of(task,
	    struct intelfb_softc, sc_setconfig_task);
	const prop_dictionary_t dict = device_properties(sc->sc_dev);
	const struct intelfb_attach_args *const ifa = &sc->sc_ifa;
	const struct drm_fb_helper_surface_size *const sizes =
	    &ifa->ifa_fb_sizes;
#if NVGA > 0			/* XXX no workie for modules */
	struct drm_device *const dev = sc->sc_ifa.ifa_drm_dev;
#endif
	enum { CONS_VGA, CONS_GENFB, CONS_NONE } what_was_cons;
	static const struct genfb_ops zero_genfb_ops;
	struct genfb_ops genfb_ops = zero_genfb_ops;
	int error;

	KASSERT(sc->sc_scheduled);

	if (ifa->ifa_fb_zero)
		bus_space_set_region_1(sc->sc_ifa.ifa_fb_bst, sc->sc_fb_bsh, 0,
		    0, sc->sc_ifa.ifa_fb_size);

	/* XXX Ugh...  Pass these parameters some other way!  */
	prop_dictionary_set_uint32(dict, "width", sizes->surface_width);
	prop_dictionary_set_uint32(dict, "height", sizes->surface_height);
	prop_dictionary_set_uint8(dict, "depth", sizes->surface_bpp);
	prop_dictionary_set_uint16(dict, "linebytes",
	    roundup2((sizes->surface_width * howmany(sizes->surface_bpp, 8)),
		64));
	prop_dictionary_set_uint32(dict, "address", 0); /* XXX >32-bit */
	CTASSERT(sizeof(uintptr_t) <= sizeof(uint64_t));
	prop_dictionary_set_uint64(dict, "virtual_address",
	    (uint64_t)(uintptr_t)bus_space_vaddr(sc->sc_ifa.ifa_fb_bst,
		sc->sc_fb_bsh));

	prop_dictionary_set_uint64(dict, "mode_callback",
	    (uint64_t)(uintptr_t)&intelfb_genfb_mode_callback);

	/* XXX Whattakludge!  */
#if NVGA > 0
	if (vga_is_console(dev->pdev->pd_pa.pa_iot, -1)) {
		what_was_cons = CONS_VGA;
		prop_dictionary_set_bool(dict, "is_console", true);
		i915_disable_vga(dev);
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
	genfb_ops.genfb_ioctl = intelfb_genfb_ioctl;
	genfb_ops.genfb_mmap = intelfb_genfb_mmap;
	genfb_ops.genfb_enable_polling = intelfb_genfb_enable_polling;
	genfb_ops.genfb_disable_polling = intelfb_genfb_disable_polling;

	error = genfb_attach(&sc->sc_genfb, &genfb_ops);
	if (error) {
		aprint_error_dev(sc->sc_dev, "failed to attach genfb: %d\n",
		    error);
		goto fail0;
	}
	sc->sc_attached = true;

	pmf_device_register1(sc->sc_dev, NULL, NULL,
	    intelfb_genfb_shutdown);

	drm_fb_helper_set_config(sc->sc_ifa.ifa_fb_helper);

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
intelfb_genfb_dpms(struct drm_device *dev, int dpms_mode)
{
	struct drm_i915_private *const dev_priv = dev->dev_private;
	/* XXX What guarantees dev_priv->fbdev stays around?  */
	struct drm_fb_helper *const fb_helper = &dev_priv->fbdev->helper;
	unsigned i;

	drm_modeset_lock_all(dev);
	for (i = 0; i < fb_helper->connector_count; i++) {
		struct drm_connector *const connector =
		    fb_helper->connector_info[i]->connector;
		(*connector->funcs->dpms)(connector, dpms_mode);
		drm_object_property_set_value(&connector->base,
		    dev->mode_config.dpms_property, dpms_mode);
	}
	drm_modeset_unlock_all(dev);

	return 0;
}

static int
intelfb_genfb_ioctl(void *v, void *vs, unsigned long cmd, void *data, int flag,
    struct lwp *l)
{
	struct genfb_softc *const genfb = v;
	struct intelfb_softc *const sc = container_of(genfb,
	    struct intelfb_softc, sc_genfb);
	struct drm_device *const dev = sc->sc_ifa.ifa_fb_helper->dev;
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

	/*
	 * Screen blanking ioctls.  Not to be confused with backlight
	 * (can be disabled while stuff is still drawn on the screen),
	 * brightness, or contrast (which we don't support).  Backlight
	 * and brightness are done through WSDISPLAYIO_{GET,SET}PARAM.
	 * This toggles between DPMS ON and DPMS OFF; backlight toggles
	 * between DPMS ON and DPMS SUSPEND.
	 */
	case WSDISPLAYIO_GVIDEO: {
		int *onp = (int *)data;

		/* XXX Can't really determine a single answer here.  */
		*onp = 1;
		return 0;
	}

	case WSDISPLAYIO_SVIDEO: {
		const int on = *(const int *)data;

		return intelfb_genfb_dpms(dev,
		    on? DRM_MODE_DPMS_ON : DRM_MODE_DPMS_OFF);
	}

	default:
		return EPASSTHROUGH;
	}
}

static paddr_t
intelfb_genfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct genfb_softc *const genfb = v;
	struct intelfb_softc *const sc = container_of(genfb,
	    struct intelfb_softc, sc_genfb);
	struct drm_fb_helper *const helper = sc->sc_ifa.ifa_fb_helper;
	struct intel_fbdev *const fbdev = container_of(helper,
	    struct intel_fbdev, helper);
	struct drm_device *const dev = helper->dev;
	struct drm_i915_private *const dev_priv = dev->dev_private;
	const struct pci_attach_args *const pa = &dev->pdev->pd_pa;
	unsigned int i;

	if (offset < 0)
		return -1;

	/* Treat low memory as the framebuffer itself.  */
	if (offset < genfb->sc_fbsize)
		return bus_space_mmap(dev->bst,
		    (dev_priv->gtt.mappable_base +
			i915_gem_obj_ggtt_offset(fbdev->fb->obj)),
		    offset, prot,
		    (BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE));

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
intelfb_genfb_enable_polling(void *cookie)
{
	struct genfb_softc *const genfb = cookie;
	struct intelfb_softc *const sc = container_of(genfb,
	    struct intelfb_softc, sc_genfb);

	return drm_fb_helper_debug_enter_fb(sc->sc_ifa.ifa_fb_helper);
}

static int
intelfb_genfb_disable_polling(void *cookie)
{
	struct genfb_softc *const genfb = cookie;
	struct intelfb_softc *const sc = container_of(genfb,
	    struct intelfb_softc, sc_genfb);

	return drm_fb_helper_debug_leave_fb(sc->sc_ifa.ifa_fb_helper);
}

static bool
intelfb_genfb_shutdown(device_t self, int flags)
{
	genfb_enable_polling(self);
	return true;
}

static bool
intelfb_genfb_setmode(struct genfb_softc *genfb, int mode)
{
	struct intelfb_softc *sc = (struct intelfb_softc *)genfb;

	if (mode == WSDISPLAYIO_MODE_EMUL) {
		drm_fb_helper_set_config(sc->sc_ifa.ifa_fb_helper);
	}

	return true;
}

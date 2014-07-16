/*	$NetBSD: i915_pci.c,v 1.11 2014/07/16 20:56:25 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: i915_pci.c,v 1.11 2014/07/16 20:56:25 riastradh Exp $");

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

#include "i915_drv.h"

struct intel_genfb_work;
SIMPLEQ_HEAD(intel_genfb_work_head, intel_genfb_work);

struct i915drmkms_softc {
	device_t			sc_dev;
	struct workqueue		*sc_genfb_wq;
	struct intel_genfb_work_head	sc_genfb_work;
	struct drm_device		*sc_drm_dev;
	struct pci_dev			sc_pci_dev;
#if 0				/* XXX backlight/brightness */
	struct genfb_parameter_callback	sc_genfb_backlight_callback;
	struct genfb_parameter_callback	sc_genfb_brightness_callback;
#endif
};

struct intel_genfb_work {
	struct drm_fb_helper	*igw_fb_helper;
	union {
		SIMPLEQ_ENTRY(intel_genfb_work)	queue;
		struct work			work;
	}			igw_u;
};

static const struct intel_device_info *
		i915drmkms_pci_lookup(const struct pci_attach_args *);

static int	i915drmkms_match(device_t, cfdata_t, void *);
static void	i915drmkms_attach(device_t, device_t, void *);
static int	i915drmkms_detach(device_t, int);

static void	intel_genfb_defer_set_config(struct drm_fb_helper *);
static void	intel_genfb_set_config_work(struct work *, void *);
static void	intel_genfb_set_config(struct intel_genfb_work *);
static int	intel_genfb_dpms(struct drm_device *, int);
static int	intel_genfb_ioctl(void *, void *, unsigned long, void *,
		    int, struct lwp *);
static paddr_t	intel_genfb_mmap(void *, void *, off_t, int);

#if 0				/* XXX backlight/brightness */
static int	intel_genfb_get_backlight(void *, int *);
static int	intel_genfb_set_backlight(void *, int);
static int	intel_genfb_upd_backlight(void *, int);
static int	intel_genfb_get_brightness(void *, int *);
static int	intel_genfb_set_brightness(void *, int);
static int	intel_genfb_upd_brightness(void *, int);
#endif

CFATTACH_DECL_NEW(i915drmkms, sizeof(struct i915drmkms_softc),
    i915drmkms_match, i915drmkms_attach, i915drmkms_detach, NULL);

/* XXX Kludge to get these from i915_drv.c.  */
extern struct drm_driver *const i915_drm_driver;
extern const struct pci_device_id *const i915_device_ids;
extern const size_t i915_n_device_ids;

static const struct intel_device_info *
i915drmkms_pci_lookup(const struct pci_attach_args *pa)
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

	if (IS_PRELIMINARY_HW(info)) {
		printf("i915drmkms: preliminary hardware support disabled\n");
		return NULL;
	}

	return info;
}

static int
i915drmkms_match(device_t parent, cfdata_t match, void *aux)
{
	extern int i915drmkms_guarantee_initialized(void);
	const struct pci_attach_args *const pa = aux;
	int error;

	error = i915drmkms_guarantee_initialized();
	if (error) {
		aprint_error("i915drmkms: failed to initialize: %d\n", error);
		return 0;
	}

	if (i915drmkms_pci_lookup(pa) == NULL)
		return 0;

	return 6;		/* XXX Beat genfb_pci...  */
}

static void
i915drmkms_attach(device_t parent, device_t self, void *aux)
{
	struct i915drmkms_softc *const sc = device_private(self);
	const struct pci_attach_args *const pa = aux;
	const struct intel_device_info *const info = i915drmkms_pci_lookup(pa);
	const unsigned long cookie =
	    (unsigned long)(uintptr_t)(const void *)info;
	int error;

	KASSERT(info != NULL);

	sc->sc_dev = self;

	pci_aprint_devinfo(pa, NULL);

	SIMPLEQ_INIT(&sc->sc_genfb_work);

	/* XXX errno Linux->NetBSD */
	error = -drm_pci_attach(self, pa, &sc->sc_pci_dev, i915_drm_driver,
	    cookie, &sc->sc_drm_dev);
	if (error) {
		aprint_error_dev(self, "unable to attach drm: %d\n", error);
		return;
	}

	while (!SIMPLEQ_EMPTY(&sc->sc_genfb_work)) {
		struct intel_genfb_work *const work =
		    SIMPLEQ_FIRST(&sc->sc_genfb_work);

		SIMPLEQ_REMOVE_HEAD(&sc->sc_genfb_work, igw_u.queue);
		intel_genfb_set_config(work);
	}

	error = workqueue_create(&sc->sc_genfb_wq, "intelfb",
	    &intel_genfb_set_config_work, NULL, PRI_NONE, IPL_NONE, WQ_MPSAFE);
	if (error) {
		aprint_error_dev(self, "unable to create workqueue: %d\n",
		    error);
		return;
	}
}

static int
i915drmkms_detach(device_t self, int flags)
{
	struct i915drmkms_softc *const sc = device_private(self);
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
intel_genfb_attach(struct drm_device *dev, struct drm_fb_helper *helper,
    const struct drm_fb_helper_surface_size *sizes)
{
	struct i915drmkms_softc *const sc = container_of(dev->pdev,
	    struct i915drmkms_softc, sc_pci_dev);
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
		 */
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
	    (uint64_t)(uintptr_t)bus_space_vaddr(helper->fb_bst,
		helper->fb_bsh));

#if 0				/* XXX backlight/brightness */
	/* XXX What happens with multi-head, multi-fb displays?  */
	sc->sc_genfb_backlight_callback = (struct genfb_parameter_callback) {
		.gpc_cookie = dev,
		.gpc_get_parameter = &intel_genfb_get_backlight,
		.gpc_set_parameter = &intel_genfb_set_backlight,
		.gpc_upd_parameter = &intel_genfb_upd_backlight,
	};
	prop_dictionary_set_uint64(dict, "backlight_callback",
	    (uint64_t)(uintptr_t)&sc->sc_genfb_backlight_callback);

	sc->sc_genfb_brightness_callback = (struct genfb_parameter_callback) {
		.gpc_cookie = dev,
		.gpc_get_parameter = &intel_genfb_get_brightness,
		.gpc_set_parameter = &intel_genfb_set_brightness,
		.gpc_upd_parameter = &intel_genfb_upd_brightness,
	};
	prop_dictionary_set_uint64(dict, "brightness_callback",
	    (uint64_t)(uintptr_t)&sc->sc_genfb_brightness_callback);
#endif

	helper->genfb.sc_dev = sc->sc_dev;
	genfb_init(&helper->genfb);
	genfb_ops.genfb_ioctl = intel_genfb_ioctl;
	genfb_ops.genfb_mmap = intel_genfb_mmap;

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

	intel_genfb_defer_set_config(helper);

	return 0;
}

static void
intel_genfb_defer_set_config(struct drm_fb_helper *helper)
{
	struct drm_device *const dev = helper->dev;
	struct i915drmkms_softc *const sc = container_of(dev->pdev,
	    struct i915drmkms_softc, sc_pci_dev);
	struct intel_genfb_work *work;

	/* Really shouldn't sleep here...  */
	work = kmem_alloc(sizeof(*work), KM_SLEEP);
	work->igw_fb_helper = helper;

	if (sc->sc_genfb_wq == NULL) /* during attachment */
		SIMPLEQ_INSERT_TAIL(&sc->sc_genfb_work, work, igw_u.queue);
	else
		workqueue_enqueue(sc->sc_genfb_wq, &work->igw_u.work, NULL);
}

static void
intel_genfb_set_config_work(struct work *work, void *cookie __unused)
{

	intel_genfb_set_config(container_of(work, struct intel_genfb_work,
		igw_u.work));
}

static void
intel_genfb_set_config(struct intel_genfb_work *work)
{

	drm_fb_helper_set_config(work->igw_fb_helper);
	kmem_free(work, sizeof(*work));
}

static int
intel_genfb_dpms(struct drm_device *dev, int dpms_mode)
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
intel_genfb_ioctl(void *v, void *vs, unsigned long cmd, void *data, int flag,
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

		return intel_genfb_dpms(dev,
		    on? DRM_MODE_DPMS_ON : DRM_MODE_DPMS_OFF);
	}

	default:
		return EPASSTHROUGH;
	}
}

static paddr_t
intel_genfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct genfb_softc *const genfb = v;
	struct drm_fb_helper *const helper = container_of(genfb,
	    struct drm_fb_helper, genfb);
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

#if 0				/* XXX backlight/brightness */
static int
intel_genfb_get_backlight(void *cookie, int *enablep)
{
	struct drm_device *const dev = cookie;
	struct drm_i915_private *const dev_priv = dev->dev_private;

	*enablep = dev_priv->backlight.present;

	return 0;
}

static int
intel_genfb_set_backlight(void *cookie, int enable)
{
	struct drm_device *const dev = cookie;

	return intel_genfb_dpms(dev,
	    enable? DRM_MODE_DPMS_ON : DRM_MODE_DPMS_SUSPEND);
}

static int
intel_genfb_upd_backlight(void *cookie __unused, int step __unused)
{

	panic("can't update i915drm backlight");
}

static int
intel_genfb_get_brightness(void *cookie, int *brightnessp)
{
	struct drm_device *const dev = cookie;
	struct drm_i915_private *const dev_priv = dev->dev_private;

	*brightnessp = dev_priv->backlight_level;

	return 0;
}

static int
intel_genfb_set_brightness(void *cookie, int brightness)
{
	struct drm_device *const dev = cookie;

	intel_panel_set_backlight(dev,
	    MIN(brightness, intel_panel_get_max_backlight(dev)));

	return 0;
}

static int
intel_genfb_upd_brightness(void *cookie, int delta)
{
	struct drm_device *const dev = cookie;
	struct drm_i915_private *const dev_priv = dev->dev_private;

	intel_panel_set_backlight(dev,
	    MIN(dev_priv->backlight_level + delta,
		intel_panel_get_max_backlight(dev)));

	return 0;
}
#endif

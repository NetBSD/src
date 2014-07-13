/*	$NetBSD: i915_pci.c,v 1.10 2014/07/13 01:17:15 mlelstv Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: i915_pci.c,v 1.10 2014/07/13 01:17:15 mlelstv Exp $");

#ifdef _KERNEL_OPT
#include "vga.h"
#endif

#include <sys/types.h>
#ifndef _MODULE
/* XXX Mega-kludge because modules are broken.  */
#include <sys/once.h>
#endif
#include <sys/systm.h>

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

struct i915drm_softc {
	device_t			sc_dev;
	struct drm_device		sc_drm_dev;
	struct pci_dev			sc_pci_dev;
	struct drm_i915_gem_object	*sc_fb_obj;
	bus_space_handle_t		sc_fb_bsh;
	struct genfb_softc		sc_genfb;
	struct genfb_parameter_callback	sc_genfb_backlight_callback;
	struct genfb_parameter_callback	sc_genfb_brightness_callback;
	struct list_head		sc_fb_list; /* XXX Kludge!  */
	bool				sc_console;
};

static int	i915drm_match(device_t, cfdata_t, void *);
static void	i915drm_attach(device_t, device_t, void *);
static int	i915drm_detach(device_t, int);

#ifndef _MODULE
/* XXX Mega-kludge because modules are broken.  */
static int	i915drm_init(void);
static ONCE_DECL(i915drm_init_once);
#endif

static void	i915drm_attach_framebuffer(device_t);
static int	i915drm_detach_framebuffer(device_t, int);

static int	i915drm_fb_probe(struct drm_fb_helper *,
		    struct drm_fb_helper_surface_size *);
static void	i915drm_fb_detach(struct drm_fb_helper *);
static int	i915drm_fb_create_handle(struct drm_framebuffer *,
		    struct drm_file *, unsigned int *);
static void	i915drm_fb_destroy(struct drm_framebuffer *);

static int	i915drm_fb_dpms(struct i915drm_softc *, int);

static int	i915drm_genfb_ioctl(void *, void *, unsigned long, void *,
		    int, struct lwp *);
static paddr_t	i915drm_genfb_mmap(void *, void *, off_t, int);

static int	i915drm_genfb_get_backlight(void *, int *);
static int	i915drm_genfb_set_backlight(void *, int);
static int	i915drm_genfb_upd_backlight(void *, int);
static int	i915drm_genfb_get_brightness(void *, int *);
static int	i915drm_genfb_set_brightness(void *, int);
static int	i915drm_genfb_upd_brightness(void *, int);

CFATTACH_DECL_NEW(i915drmkms, sizeof(struct i915drm_softc),
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

#ifndef _MODULE
	/* XXX Mega-kludge because modules are broken.  */
	if (RUN_ONCE(&i915drm_init_once, &i915drm_init) != 0)
		return 0;
#endif

	if (i915drm_pci_lookup(pa) == NULL)
		return 0;

	return 6;		/* XXX Beat genfb_pci...  */
}

static void
i915drm_attach(device_t parent, device_t self, void *aux)
{
	struct i915drm_softc *const sc = device_private(self);
	const struct pci_attach_args *const pa = aux;
	const struct intel_device_info *const info = i915drm_pci_lookup(pa);
	const unsigned long flags =
	    (unsigned long)(uintptr_t)(const void *)info;
	int error;

	KASSERT(info != NULL);

	sc->sc_dev = self;

	pci_aprint_devinfo(pa, NULL);

	/* XXX Whattakludge!  */
	if (info->gen != 3) {
		i915_drm_driver->driver_features &=~ DRIVER_REQUIRE_AGP;
		i915_drm_driver->driver_features &=~ DRIVER_USE_AGP;
	}

	/*
	 * Figure out whether to grab the console.
	 *
	 * XXX This is much too hairy!  Can we simplify it and
	 * x86/consinit.c?
	 */
#if NVGA > 0
	if (vga_is_console(pa->pa_iot, -1) ||
	    vga_is_console(pa->pa_memt, -1)) {
		sc->sc_console = true;
		/*
		 * There is a window from here until genfb attaches in
		 * which kernel messages will go into a black hole,
		 * until genfb replays the console.  Whattakludge.
		 *
		 * vga_cndetach detaches wscons and unmaps the bus space
		 * that it would have used.
		 */
		vga_cndetach();
	} else
#endif
	if (genfb_is_console() && genfb_is_enabled()) {
		sc->sc_console = true;
	} else {
		sc->sc_console = false;
	}

	/* Initialize the drm pci driver state.  */
	sc->sc_drm_dev.driver = i915_drm_driver;
	drm_pci_attach(self, pa, &sc->sc_pci_dev, &sc->sc_drm_dev);

	/* Attach the drm driver.  */
	error = drm_config_found(self, i915_drm_driver, flags,
	    &sc->sc_drm_dev);
	if (error) {
		aprint_error_dev(self, "unable to attach drm: %d\n", error);
		return;
	}

	/* Attach a framebuffer.  */
	i915drm_attach_framebuffer(self);
}

static int
i915drm_detach(device_t self, int flags)
{
	struct i915drm_softc *const sc = device_private(self);
	int error;

	/*
	 * XXX OK to do this first?  Detaching the drm driver runs
	 * i915_driver_unload, which frees all the i915 private data
	 * structures.
	 */
	error = i915drm_detach_framebuffer(self, flags);
	if (error)
		return error;

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

#ifndef _MODULE
/* XXX Mega-kludge because modules are broken.  See drm_init for details.  */
static int
i915drm_init(void)
{
	int error;

	i915_drm_driver->num_ioctls = i915_max_ioctl;
	i915_drm_driver->driver_features |= DRIVER_MODESET;

	error = drm_pci_init(i915_drm_driver, NULL);
	if (error) {
		aprint_error("i915drmkms: failed to init pci: %d\n",
		    error);
		return error;
	}

	return 0;
}
#endif

static struct drm_fb_helper_funcs i915drm_fb_helper_funcs = {
	.gamma_set = &intel_crtc_fb_gamma_set,
	.gamma_get = &intel_crtc_fb_gamma_get,
	.fb_probe = &i915drm_fb_probe,
};

static void
i915drm_attach_framebuffer(device_t self)
{
	struct i915drm_softc *const sc = device_private(self);
	struct drm_device *const dev = &sc->sc_drm_dev;
	struct drm_i915_private *const dev_priv = dev->dev_private;
	int ret;

	INIT_LIST_HEAD(&sc->sc_fb_list);

	KASSERT(dev_priv->fbdev == NULL);
	dev_priv->fbdev = kmem_zalloc(sizeof(*dev_priv->fbdev), KM_SLEEP);

	struct drm_fb_helper *const fb_helper = &dev_priv->fbdev->helper;

	fb_helper->funcs = &i915drm_fb_helper_funcs;
	ret = drm_fb_helper_init(dev, fb_helper, dev_priv->num_pipe,
	    INTELFB_CONN_LIMIT);
	if (ret) {
		aprint_error_dev(self, "unable to init drm fb helper: %d\n",
		    ret);
		goto fail0;
	}

	drm_fb_helper_single_add_all_connectors(fb_helper);
	drm_fb_helper_initial_config(fb_helper, 32 /* XXX ? */);

	/* Success!  */
	return;

fail1: __unused
	drm_fb_helper_fini(fb_helper);
fail0:	kmem_free(dev_priv->fbdev, sizeof(*dev_priv->fbdev));
	dev_priv->fbdev = NULL;
}

static int
i915drm_detach_framebuffer(device_t self, int flags)
{
	struct i915drm_softc *const sc = device_private(self);
	struct drm_device *const dev = &sc->sc_drm_dev;
	struct drm_i915_private *const dev_priv = dev->dev_private;
	struct intel_fbdev *const fbdev = dev_priv->fbdev;

	if (fbdev != NULL) {
		struct drm_fb_helper *const fb_helper = &fbdev->helper;

		if (fb_helper->fb != NULL)
			i915drm_fb_detach(fb_helper);
		KASSERT(fb_helper->fb == NULL);

		KASSERT(sc->sc_fb_obj == NULL);
		drm_fb_helper_fini(fb_helper);
		kmem_free(fbdev, sizeof(*fbdev));
		dev_priv->fbdev = NULL;
	}

	return 0;
}

static const struct drm_framebuffer_funcs i915drm_fb_funcs = {
	.create_handle = &i915drm_fb_create_handle,
	.destroy = &i915drm_fb_destroy,
};

static int
i915drm_fb_probe(struct drm_fb_helper *fb_helper,
    struct drm_fb_helper_surface_size *sizes)
{
	struct drm_device *const dev = fb_helper->dev;
	struct drm_i915_private *const dev_priv = dev->dev_private;
	struct i915drm_softc *const sc = container_of(dev,
	    struct i915drm_softc, sc_drm_dev);
	const prop_dictionary_t dict = device_properties(sc->sc_dev);
	static const struct genfb_ops zero_genfb_ops;
	struct genfb_ops genfb_ops = zero_genfb_ops;
	static const struct drm_mode_fb_cmd2 zero_mode_cmd;
	struct drm_mode_fb_cmd2 mode_cmd = zero_mode_cmd;
	bus_size_t size;
	int ret;

	aprint_debug_dev(sc->sc_dev, "probe framebuffer"
	    ": %"PRIu32" by %"PRIu32", bpp %"PRIu32" depth %"PRIu32"\n",
	    sizes->surface_width,
	    sizes->surface_height,
	    sizes->surface_bpp,
	    sizes->surface_depth);

	if (fb_helper->fb != NULL) {
#if notyet			/* XXX genfb detach */
		i915drm_fb_detach(fb_helper);
#else
		aprint_debug_dev(sc->sc_dev, "already have a framebuffer"
		    ": %p\n", fb_helper->fb);
		return 0;
#endif
	}

	/*
	 * XXX Cargo-culted from Linux.  Using sizes as an input/output
	 * parameter seems sketchy...
	 */
	if (sizes->surface_bpp == 24)
		sizes->surface_bpp = 32;

	mode_cmd.width = sizes->surface_width;
	mode_cmd.height = sizes->surface_height;
	KASSERT(sizes->surface_bpp <= (UINT32_MAX - 7));
	KASSERT(mode_cmd.width <=
	    ((UINT32_MAX / howmany(sizes->surface_bpp, 8)) - 63));
	mode_cmd.pitches[0] = round_up((mode_cmd.width *
		howmany(sizes->surface_bpp, 8)), 64);
	mode_cmd.pixel_format = drm_mode_legacy_fb_format(sizes->surface_bpp,
	    sizes->surface_depth);

	KASSERT(mode_cmd.pitches[0] <=
	    ((__type_max(bus_size_t) / mode_cmd.height) - 63));
	size = round_up((mode_cmd.pitches[0] * mode_cmd.height), PAGE_SIZE);

	sc->sc_fb_obj = i915_gem_alloc_object(dev, size);
	if (sc->sc_fb_obj == NULL) {
		aprint_error_dev(sc->sc_dev, "unable to create framebuffer\n");
		ret = -ENODEV; /* XXX ? */
		goto fail0;
	}

	mutex_lock(&dev->struct_mutex);
	ret = intel_pin_and_fence_fb_obj(dev, sc->sc_fb_obj, NULL);
	if (ret) {
		aprint_error_dev(sc->sc_dev, "unable to pin fb: %d\n", ret);
		goto fail1;
	}

	ret = intel_framebuffer_init(dev, &dev_priv->fbdev->ifb, &mode_cmd,
	    sc->sc_fb_obj);
	if (ret) {
		aprint_error_dev(sc->sc_dev, "unable to init framebuffer"
		    ": %d\n", ret);
		goto fail2;
	}

	/*
	 * XXX Kludge: drm_framebuffer_remove assumes that the
	 * framebuffer has been put on a userspace list by
	 * drm_mode_addfb and tries to list_del it.  This is not the
	 * case, so pretend we are on a list.
	 */
	list_add(&dev_priv->fbdev->ifb.base.filp_head, &sc->sc_fb_list);

	/*
	 * XXX Kludge: The intel_framebuffer abstraction sets up a
	 * destruction routine that frees the wrong pointer, under the
	 * assumption (which is invalid even upstream) that all
	 * intel_framebuffer structures are allocated in
	 * intel_framebuffer_create.
	 */
	dev_priv->fbdev->ifb.base.funcs = &i915drm_fb_funcs;

	fb_helper->fb = &dev_priv->fbdev->ifb.base;
	mutex_unlock(&dev->struct_mutex);

	/* XXX errno NetBSD->Linux */
	ret = -bus_space_map(dev->bst,
	    (dev_priv->mm.gtt_base_addr + sc->sc_fb_obj->gtt_offset),
	    size,
	    (BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE),
	    &sc->sc_fb_bsh);
	if (ret) {
		aprint_error_dev(sc->sc_dev, "unable to map framebuffer: %d\n",
		    ret);
		goto fail3;
	}

	prop_dictionary_set_bool(dict, "is_console", sc->sc_console);
	prop_dictionary_set_uint32(dict, "width", mode_cmd.width);
	prop_dictionary_set_uint32(dict, "height", mode_cmd.height);
	prop_dictionary_set_uint8(dict, "depth", sizes->surface_bpp);
	prop_dictionary_set_uint16(dict, "linebytes", mode_cmd.pitches[0]);
	prop_dictionary_set_uint32(dict, "address", 0); /* XXX >32-bit */
	CTASSERT(sizeof(uintptr_t) <= sizeof(uint64_t));
	prop_dictionary_set_uint64(dict, "virtual_address",
	    (uint64_t)(uintptr_t)bus_space_vaddr(dev->bst, sc->sc_fb_bsh));

	sc->sc_genfb_backlight_callback = (struct genfb_parameter_callback) {
		.gpc_cookie = sc,
		.gpc_get_parameter = &i915drm_genfb_get_backlight,
		.gpc_set_parameter = &i915drm_genfb_set_backlight,
		.gpc_upd_parameter = &i915drm_genfb_upd_backlight,
	};
	prop_dictionary_set_uint64(dict, "backlight_callback",
	    (uint64_t)(uintptr_t)&sc->sc_genfb_backlight_callback);

	sc->sc_genfb_brightness_callback = (struct genfb_parameter_callback) {
		.gpc_cookie = sc,
		.gpc_get_parameter = &i915drm_genfb_get_brightness,
		.gpc_set_parameter = &i915drm_genfb_set_brightness,
		.gpc_upd_parameter = &i915drm_genfb_upd_brightness,
	};
	prop_dictionary_set_uint64(dict, "brightness_callback",
	    (uint64_t)(uintptr_t)&sc->sc_genfb_brightness_callback);

	sc->sc_genfb.sc_dev = sc->sc_dev;
	genfb_init(&sc->sc_genfb);

	genfb_ops.genfb_ioctl = i915drm_genfb_ioctl;
	genfb_ops.genfb_mmap = i915drm_genfb_mmap;

	/* XXX errno NetBSD->Linux */
	ret = -genfb_attach(&sc->sc_genfb, &genfb_ops);
	if (ret) {
		aprint_error_dev(sc->sc_dev, "unable to attach genfb: %d\n",
		    ret);
		goto fail4;
	}

	/* Success!  */
	return 1;

fail4:	bus_space_unmap(dev->bst, sc->sc_fb_bsh, size);
	fb_helper->fb = NULL;
fail3:	drm_framebuffer_unreference(&dev_priv->fbdev->ifb.base);
fail2:	i915_gem_object_unpin(sc->sc_fb_obj);
fail1:	drm_gem_object_unreference_unlocked(&sc->sc_fb_obj->base);
	mutex_unlock(&dev->struct_mutex);
fail0:	KASSERT(ret < 0);
	return ret;
}

static int
i915drm_fb_dpms(struct i915drm_softc *sc, int dpms_mode)
{
	struct drm_device *const dev = &sc->sc_drm_dev;
	struct drm_i915_private *const dev_priv = dev->dev_private;
	/* XXX What guarantees dev_priv->fbdev stays around?  */
	struct drm_fb_helper *const fb_helper = &dev_priv->fbdev->helper;
	unsigned i;

	mutex_lock(&dev->mode_config.mutex);
	for (i = 0; i < fb_helper->connector_count; i++) {
		struct drm_connector *const connector =
		    fb_helper->connector_info[i]->connector;
		(*connector->funcs->dpms)(connector, dpms_mode);
		drm_object_property_set_value(&connector->base,
		    dev->mode_config.dpms_property, dpms_mode);
	}
	mutex_unlock(&dev->mode_config.mutex);

	return 0;
}

static void
i915drm_fb_detach(struct drm_fb_helper *fb_helper)
{
	struct drm_device *const dev = fb_helper->dev;
	struct i915drm_softc *const sc = container_of(dev,
	    struct i915drm_softc, sc_drm_dev);
	struct drm_i915_gem_object *const obj = sc->sc_fb_obj;

	/* XXX How to detach genfb?  */
	bus_space_unmap(dev->bst, sc->sc_fb_bsh, obj->base.size);
	drm_framebuffer_unreference(fb_helper->fb);
	fb_helper->fb = NULL;
	drm_gem_object_unreference_unlocked(&obj->base);
	sc->sc_fb_obj = NULL;
}

static void
i915drm_fb_destroy(struct drm_framebuffer *fb)
{

	drm_framebuffer_cleanup(fb);
}

static int
i915drm_fb_create_handle(struct drm_framebuffer *fb, struct drm_file *file,
    unsigned int *handle)
{

	return drm_gem_handle_create(file,
	    &to_intel_framebuffer(fb)->obj->base, handle);
}

static int
i915drm_genfb_ioctl(void *v, void *vs, unsigned long cmd, void *data, int flag,
    struct lwp *l)
{
	struct genfb_softc *const genfb = v;
	struct i915drm_softc *const sc = container_of(genfb,
	    struct i915drm_softc, sc_genfb);
	const struct pci_attach_args *const pa = &sc->sc_pci_dev.pd_pa;

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

		return i915drm_fb_dpms(sc,
		    on? DRM_MODE_DPMS_ON : DRM_MODE_DPMS_OFF);
	}

	default:
		return EPASSTHROUGH;
	}
}

static paddr_t
i915drm_genfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct genfb_softc *const genfb = v;
	struct i915drm_softc *const sc = container_of(genfb,
	    struct i915drm_softc, sc_genfb);
	struct drm_device *const dev = &sc->sc_drm_dev;
	struct drm_i915_private *const dev_priv = dev->dev_private;
	const struct pci_attach_args *const pa = &dev->pdev->pd_pa;
	unsigned int i;

	if (offset < 0)
		return -1;

	/* Treat low memory as the framebuffer itself.  */
	if (offset < genfb->sc_fbsize)
		return bus_space_mmap(dev->bst,
		    (dev_priv->mm.gtt_base_addr + sc->sc_fb_obj->gtt_offset),
		    offset,
		    prot, (BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE));

	/* XXX Cargo-culted from genfb_pci.  */
	if (kauth_authorize_machdep(kauth_cred_get(),
		KAUTH_MACHDEP_UNMANAGEDMEM, NULL, NULL, NULL, NULL) != 0) {
		aprint_normal_dev(sc->sc_dev, "mmap at %"PRIxMAX" rejected\n",
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
i915drm_genfb_get_backlight(void *cookie, int *enablep)
{
	struct i915drm_softc *const sc = cookie;
	struct drm_device *const dev = &sc->sc_drm_dev;
	struct drm_i915_private *const dev_priv = dev->dev_private;

	*enablep = dev_priv->backlight_enabled;

	return 0;
}

static int
i915drm_genfb_set_backlight(void *cookie, int enable)
{
	struct i915drm_softc *const sc = cookie;

	return i915drm_fb_dpms(sc,
	    enable? DRM_MODE_DPMS_ON : DRM_MODE_DPMS_SUSPEND);
}

static int
i915drm_genfb_upd_backlight(void *cookie __unused, int step __unused)
{

	panic("can't update i915drm backlight");
}

static int
i915drm_genfb_get_brightness(void *cookie, int *brightnessp)
{
	struct i915drm_softc *const sc = cookie;
	struct drm_device *const dev = &sc->sc_drm_dev;
	struct drm_i915_private *const dev_priv = dev->dev_private;

	*brightnessp = dev_priv->backlight_level;

	return 0;
}

static int
i915drm_genfb_set_brightness(void *cookie, int brightness)
{
	struct i915drm_softc *const sc = cookie;
	struct drm_device *const dev = &sc->sc_drm_dev;

	intel_panel_set_backlight(dev,
	    MIN(brightness, intel_panel_get_max_backlight(dev)));

	return 0;
}

static int
i915drm_genfb_upd_brightness(void *cookie, int delta)
{
	struct i915drm_softc *const sc = cookie;
	struct drm_device *const dev = &sc->sc_drm_dev;
	struct drm_i915_private *const dev_priv = dev->dev_private;

	intel_panel_set_backlight(dev,
	    MIN(dev_priv->backlight_level + delta,
		intel_panel_get_max_backlight(dev)));

	return 0;
}

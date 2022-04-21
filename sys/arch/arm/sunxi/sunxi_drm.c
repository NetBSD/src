/* $NetBSD: sunxi_drm.c,v 1.24 2022/04/21 21:22:25 andvar Exp $ */

/*-
 * Copyright (c) 2019 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunxi_drm.c,v 1.24 2022/04/21 21:22:25 andvar Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <uvm/uvm_device.h>
#include <uvm/uvm_extern.h>
#include <uvm/uvm_object.h>

#include <dev/fdt/fdt_port.h>
#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_drm.h>

#include <drm/drm_auth.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_vblank.h>

#define	SUNXI_DRM_MAX_WIDTH	3840
#define	SUNXI_DRM_MAX_HEIGHT	2160

/*
 * The DRM headers break trunc_page/round_page macros with a redefinition
 * of PAGE_MASK. Use our own macros instead.
 */
#define	SUNXI_PAGE_MASK		(PAGE_SIZE - 1)
#define	SUNXI_TRUNC_PAGE(x)	((x) & ~SUNXI_PAGE_MASK)
#define	SUNXI_ROUND_PAGE(x)	(((x) + SUNXI_PAGE_MASK) & ~SUNXI_PAGE_MASK)

static TAILQ_HEAD(, sunxi_drm_endpoint) sunxi_drm_endpoints =
    TAILQ_HEAD_INITIALIZER(sunxi_drm_endpoints);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun8i-h3-display-engine" },
	{ .compat = "allwinner,sun50i-a64-display-engine" },
	DEVICE_COMPAT_EOL
};

static const char * fb_compatible[] = {
	"allwinner,simple-framebuffer",
	NULL
};

static int	sunxi_drm_match(device_t, cfdata_t, void *);
static void	sunxi_drm_attach(device_t, device_t, void *);

static void	sunxi_drm_init(device_t);
static vmem_t	*sunxi_drm_alloc_cma_pool(struct drm_device *, size_t);

static uint32_t	sunxi_drm_get_vblank_counter(struct drm_device *, unsigned int);
static int	sunxi_drm_enable_vblank(struct drm_device *, unsigned int);
static void	sunxi_drm_disable_vblank(struct drm_device *, unsigned int);

static int	sunxi_drm_load(struct drm_device *, unsigned long);
static void	sunxi_drm_unload(struct drm_device *);

static void	sunxi_drm_task_work(struct work *, void *);

static struct drm_driver sunxi_drm_driver = {
	.driver_features = DRIVER_MODESET | DRIVER_GEM,
	.dev_priv_size = 0,
	.load = sunxi_drm_load,
	.unload = sunxi_drm_unload,

	.gem_free_object = drm_gem_cma_free_object,
	.mmap_object = drm_gem_or_legacy_mmap_object,
	.gem_uvm_ops = &drm_gem_cma_uvm_ops,

	.dumb_create = drm_gem_cma_dumb_create,
	.dumb_destroy = drm_gem_dumb_destroy,

	.get_vblank_counter = sunxi_drm_get_vblank_counter,
	.enable_vblank = sunxi_drm_enable_vblank,
	.disable_vblank = sunxi_drm_disable_vblank,

	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL,
};

CFATTACH_DECL_NEW(sunxi_drm, sizeof(struct sunxi_drm_softc),
	sunxi_drm_match, sunxi_drm_attach, NULL, NULL);

static int
sunxi_drm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sunxi_drm_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_drm_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	struct drm_driver * const driver = &sunxi_drm_driver;
	prop_dictionary_t dict = device_properties(self);
	bool is_disabled;

	aprint_naive("\n");

	if (prop_dictionary_get_bool(dict, "disabled", &is_disabled) &&
	    is_disabled) {
		aprint_normal(": Display Engine Pipeline (disabled)\n");
		return;
	}

	aprint_normal(": Display Engine Pipeline\n");

	sc->sc_dev = self;
	sc->sc_dmat = faa->faa_dmat;
	sc->sc_bst = faa->faa_bst;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_task_thread = NULL;
	SIMPLEQ_INIT(&sc->sc_tasks);
	if (workqueue_create(&sc->sc_task_wq, "sunxidrm",
		&sunxi_drm_task_work, NULL, PRI_NONE, IPL_NONE, WQ_MPSAFE)) {
		aprint_error_dev(self, "unable to create workqueue\n");
		sc->sc_task_wq = NULL;
		return;
	}

	sc->sc_ddev = drm_dev_alloc(driver, sc->sc_dev);
	if (IS_ERR(sc->sc_ddev)) {
		aprint_error_dev(self, "couldn't allocate DRM device\n");
		return;
	}
	sc->sc_ddev->dev_private = sc;
	sc->sc_ddev->bst = sc->sc_bst;
	sc->sc_ddev->bus_dmat = sc->sc_dmat;
	sc->sc_ddev->dmat = sc->sc_ddev->bus_dmat;
	sc->sc_ddev->dmat_subregion_p = false;

	fdt_remove_bycompat(fb_compatible);

	config_defer(self, sunxi_drm_init);
}

static void
sunxi_drm_init(device_t dev)
{
	struct sunxi_drm_softc * const sc = device_private(dev);
	struct drm_driver * const driver = &sunxi_drm_driver;
	int error;

	/*
	 * Cause any tasks issued synchronously during attach to be
	 * processed at the end of this function.
	 */
	sc->sc_task_thread = curlwp;

	error = -drm_dev_register(sc->sc_ddev, 0);
	if (error) {
		aprint_error_dev(dev, "couldn't register DRM device: %d\n",
		    error);
		goto out;
	}
	sc->sc_dev_registered = true;

	aprint_normal_dev(dev, "initialized %s %d.%d.%d %s on minor %d\n",
	    driver->name, driver->major, driver->minor, driver->patchlevel,
	    driver->date, sc->sc_ddev->primary->index);

	/*
	 * Process asynchronous tasks queued synchronously during
	 * attach.  This will be for display detection to attach a
	 * framebuffer, so we have the opportunity for a console device
	 * to attach before autoconf has completed, in time for init(8)
	 * to find that console without panicking.
	 */
	while (!SIMPLEQ_EMPTY(&sc->sc_tasks)) {
		struct sunxi_drm_task *const task =
		    SIMPLEQ_FIRST(&sc->sc_tasks);

		SIMPLEQ_REMOVE_HEAD(&sc->sc_tasks, sdt_u.queue);
		(*task->sdt_fn)(task);
	}

out:	/* Cause any subsequent tasks to be processed by the workqueue.  */
	atomic_store_relaxed(&sc->sc_task_thread, NULL);
}

static vmem_t *
sunxi_drm_alloc_cma_pool(struct drm_device *ddev, size_t cma_size)
{
	struct sunxi_drm_softc * const sc = sunxi_drm_private(ddev);
	bus_dma_segment_t segs[1];
	int nsegs;
	int error;

	error = bus_dmamem_alloc(sc->sc_dmat, cma_size, PAGE_SIZE, 0,
	    segs, 1, &nsegs, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't allocate CMA pool\n");
		return NULL;
	}

	return vmem_create("sunxidrm", segs[0].ds_addr, segs[0].ds_len,
	    PAGE_SIZE, NULL, NULL, NULL, 0, VM_SLEEP, IPL_NONE);
}

static int
sunxi_drm_fb_create_handle(struct drm_framebuffer *fb,
    struct drm_file *file, unsigned int *handle)
{
	struct sunxi_drm_framebuffer *sfb = to_sunxi_drm_framebuffer(fb);

	return drm_gem_handle_create(file, &sfb->obj->base, handle);
}

static void
sunxi_drm_fb_destroy(struct drm_framebuffer *fb)
{
	struct sunxi_drm_framebuffer *sfb = to_sunxi_drm_framebuffer(fb);

	drm_framebuffer_cleanup(fb);
	drm_gem_object_put_unlocked(&sfb->obj->base);
	kmem_free(sfb, sizeof(*sfb));
}

static const struct drm_framebuffer_funcs sunxi_drm_framebuffer_funcs = {
	.create_handle = sunxi_drm_fb_create_handle,
	.destroy = sunxi_drm_fb_destroy,
};

static struct drm_framebuffer *
sunxi_drm_fb_create(struct drm_device *ddev, struct drm_file *file,
    const struct drm_mode_fb_cmd2 *cmd)
{
	struct sunxi_drm_framebuffer *fb;
	struct drm_gem_object *gem_obj;
	int error;

	if (cmd->flags)
		return NULL;

	gem_obj = drm_gem_object_lookup(file, cmd->handles[0]);
	if (gem_obj == NULL)
		return NULL;

	fb = kmem_zalloc(sizeof(*fb), KM_SLEEP);
	fb->obj = to_drm_gem_cma_obj(gem_obj);
	drm_helper_mode_fill_fb_struct(ddev, &fb->base, cmd);

	error = drm_framebuffer_init(ddev, &fb->base, &sunxi_drm_framebuffer_funcs);
	if (error != 0)
		goto dealloc;

	return &fb->base;

dealloc:
	drm_framebuffer_cleanup(&fb->base);
	kmem_free(fb, sizeof(*fb));
	drm_gem_object_put_unlocked(gem_obj);

	return NULL;
}

static struct drm_mode_config_funcs sunxi_drm_mode_config_funcs = {
	.fb_create = sunxi_drm_fb_create,
};

static int
sunxi_drm_simplefb_lookup(bus_addr_t *paddr, bus_size_t *psize)
{
	static const struct device_compatible_entry simplefb_compat[] = {
		{ .compat = "simple-framebuffer" },
		DEVICE_COMPAT_EOL
	};
	int chosen, child, error;
	bus_addr_t addr_end;

	chosen = OF_finddevice("/chosen");
	if (chosen == -1)
		return ENOENT;

	for (child = OF_child(chosen); child; child = OF_peer(child)) {
		if (!fdtbus_status_okay(child))
			continue;
		if (!of_compatible_match(child, simplefb_compat))
			continue;
		error = fdtbus_get_reg(child, 0, paddr, psize);
		if (error != 0)
			return error;

		/* Reclaim entire pages used by the simplefb */
		addr_end = *paddr + *psize;
		*paddr = SUNXI_TRUNC_PAGE(*paddr);
		*psize = SUNXI_ROUND_PAGE(addr_end - *paddr);
		return 0;
	}

	return ENOENT;
}

static int
sunxi_drm_fb_probe(struct drm_fb_helper *helper, struct drm_fb_helper_surface_size *sizes)
{
	struct sunxi_drm_softc * const sc = sunxi_drm_private(helper->dev);
	struct drm_device *ddev = helper->dev;
	struct sunxi_drm_framebuffer *sfb = to_sunxi_drm_framebuffer(helper->fb);
	struct drm_framebuffer *fb = helper->fb;
	struct sunxi_drmfb_attach_args sfa;
	bus_addr_t sfb_addr;
	bus_size_t sfb_size;
	size_t cma_size;
	int error;

	const u_int width = sizes->surface_width;
	const u_int height = sizes->surface_height;
	const u_int pitch = width * (32 / 8);

	const size_t size = roundup(height * pitch, PAGE_SIZE);

	if (sunxi_drm_simplefb_lookup(&sfb_addr, &sfb_size) != 0)
		sfb_size = 0;

	/* Reserve enough memory for a 4K plane, rounded to 1MB */
	cma_size = (SUNXI_DRM_MAX_WIDTH * SUNXI_DRM_MAX_HEIGHT * 4);
	if (sfb_size == 0) {
		/* Add memory for FB console if we cannot reclaim bootloader memory */
		cma_size += size;
	}
	cma_size = roundup(cma_size, 1024 * 1024);
	sc->sc_ddev->cma_pool = sunxi_drm_alloc_cma_pool(sc->sc_ddev, cma_size);
	if (sc->sc_ddev->cma_pool != NULL) {
		if (sfb_size != 0) {
			error = vmem_add(sc->sc_ddev->cma_pool, sfb_addr,
			    sfb_size, VM_SLEEP);
			if (error != 0)
				sfb_size = 0;
		}
		aprint_normal_dev(sc->sc_dev, "reserved %u MB DRAM for CMA",
		    (u_int)((cma_size + sfb_size) / (1024 * 1024)));
		if (sfb_size != 0)
			aprint_normal(" (%u MB reclaimed from bootloader)",
			    (u_int)(sfb_size / (1024 * 1024)));
		aprint_normal("\n");
	}

	sfb->obj = drm_gem_cma_create(ddev, size);
	if (sfb->obj == NULL) {
		DRM_ERROR("failed to allocate memory for framebuffer\n");
		return -ENOMEM;
	}

	fb->pitches[0] = pitch;
	fb->offsets[0] = 0;
	fb->width = width;
	fb->height = height;
	fb->format = drm_format_info(DRM_FORMAT_XRGB8888);
	fb->dev = ddev;

	error = drm_framebuffer_init(ddev, fb, &sunxi_drm_framebuffer_funcs);
	if (error != 0) {
		DRM_ERROR("failed to initialize framebuffer\n");
		return error;
	}

	memset(&sfa, 0, sizeof(sfa));
	sfa.sfa_drm_dev = ddev;
	sfa.sfa_fb_helper = helper;
	sfa.sfa_fb_sizes = *sizes;
	sfa.sfa_fb_bst = sc->sc_bst;
	sfa.sfa_fb_dmat = sc->sc_dmat;
	sfa.sfa_fb_linebytes = helper->fb->pitches[0];

	helper->fbdev = config_found(ddev->dev, &sfa, NULL,
	    CFARGS(.iattr = "sunxifbbus"));
	if (helper->fbdev == NULL) {
		DRM_ERROR("unable to attach framebuffer\n");
		return -ENXIO;
	}

	return 0;
}

static struct drm_fb_helper_funcs sunxi_drm_fb_helper_funcs = {
	.fb_probe = sunxi_drm_fb_probe,
};

static int
sunxi_drm_load(struct drm_device *ddev, unsigned long flags)
{
	struct sunxi_drm_softc * const sc = sunxi_drm_private(ddev);
	struct sunxi_drm_endpoint *sep;
	struct sunxi_drm_fbdev *fbdev;
	const u_int *data;
	int datalen, error, num_crtc;

	drm_mode_config_init(ddev);
	ddev->mode_config.min_width = 0;
	ddev->mode_config.min_height = 0;
	ddev->mode_config.max_width = SUNXI_DRM_MAX_WIDTH;
	ddev->mode_config.max_height = SUNXI_DRM_MAX_HEIGHT;
	ddev->mode_config.funcs = &sunxi_drm_mode_config_funcs;

	num_crtc = 0;
	data = fdtbus_get_prop(sc->sc_phandle, "allwinner,pipelines", &datalen);
	while (datalen >= 4) {
		const int crtc_phandle = fdtbus_get_phandle_from_native(be32dec(data));

		TAILQ_FOREACH(sep, &sunxi_drm_endpoints, entries)
			if (sep->phandle == crtc_phandle && sep->ddev == NULL) {
				sep->ddev = ddev;
				error = fdt_endpoint_activate_direct(sep->ep, true);
				if (error != 0) {
					aprint_error_dev(sc->sc_dev, "failed to activate endpoint: %d\n",
					    error);
				}
				if (fdt_endpoint_type(sep->ep) == EP_DRM_CRTC)
					num_crtc++;
			}

		datalen -= 4;
		data++;
	}

	if (num_crtc == 0) {
		aprint_error_dev(sc->sc_dev, "no pipelines configured\n");
		error = ENXIO;
		goto drmerr;
	}

	fbdev = kmem_zalloc(sizeof(*fbdev), KM_SLEEP);

	drm_fb_helper_prepare(ddev, &fbdev->helper, &sunxi_drm_fb_helper_funcs);

	error = drm_fb_helper_init(ddev, &fbdev->helper, num_crtc);
	if (error)
		goto allocerr;

	fbdev->helper.fb = kmem_zalloc(sizeof(struct sunxi_drm_framebuffer), KM_SLEEP);

	drm_fb_helper_single_add_all_connectors(&fbdev->helper);

	drm_helper_disable_unused_functions(ddev);

	drm_fb_helper_initial_config(&fbdev->helper, 32);

	/* XXX */
	ddev->irq_enabled = true;
	drm_vblank_init(ddev, num_crtc);

	return 0;

allocerr:
	kmem_free(fbdev, sizeof(*fbdev));
drmerr:
	drm_mode_config_cleanup(ddev);

	return error;
}

static uint32_t
sunxi_drm_get_vblank_counter(struct drm_device *ddev, unsigned int crtc)
{
	struct sunxi_drm_softc * const sc = sunxi_drm_private(ddev);

	if (crtc >= __arraycount(sc->sc_vbl))
		return 0;

	if (sc->sc_vbl[crtc].get_vblank_counter == NULL)
		return 0;

	return sc->sc_vbl[crtc].get_vblank_counter(sc->sc_vbl[crtc].priv);
}

static int
sunxi_drm_enable_vblank(struct drm_device *ddev, unsigned int crtc)
{
	struct sunxi_drm_softc * const sc = sunxi_drm_private(ddev);

	if (crtc >= __arraycount(sc->sc_vbl))
		return 0;

	if (sc->sc_vbl[crtc].enable_vblank == NULL)
		return 0;

	sc->sc_vbl[crtc].enable_vblank(sc->sc_vbl[crtc].priv);

	return 0;
}

static void
sunxi_drm_disable_vblank(struct drm_device *ddev, unsigned int crtc)
{
	struct sunxi_drm_softc * const sc = sunxi_drm_private(ddev);

	if (crtc >= __arraycount(sc->sc_vbl))
		return;

	if (sc->sc_vbl[crtc].disable_vblank == NULL)
		return;

	sc->sc_vbl[crtc].disable_vblank(sc->sc_vbl[crtc].priv);
}

static void
sunxi_drm_unload(struct drm_device *ddev)
{
	drm_mode_config_cleanup(ddev);
}

int
sunxi_drm_register_endpoint(int phandle, struct fdt_endpoint *ep)
{
	struct sunxi_drm_endpoint *sep;

	sep = kmem_zalloc(sizeof(*sep), KM_SLEEP);
	sep->phandle = phandle;
	sep->ep = ep;
	sep->ddev = NULL;
	TAILQ_INSERT_TAIL(&sunxi_drm_endpoints, sep, entries);

	return 0;
}

struct drm_device *
sunxi_drm_endpoint_device(struct fdt_endpoint *ep)
{
	struct sunxi_drm_endpoint *sep;

	TAILQ_FOREACH(sep, &sunxi_drm_endpoints, entries)
		if (sep->ep == ep)
			return sep->ddev;

	return NULL;
}

static void
sunxi_drm_task_work(struct work *work, void *cookie)
{
	struct sunxi_drm_task *task = container_of(work, struct sunxi_drm_task,
	    sdt_u.work);

	(*task->sdt_fn)(task);
}

void
sunxi_task_init(struct sunxi_drm_task *task,
    void (*fn)(struct sunxi_drm_task *))
{

	task->sdt_fn = fn;
}

void
sunxi_task_schedule(device_t self, struct sunxi_drm_task *task)
{
	struct sunxi_drm_softc *sc = device_private(self);

	if (atomic_load_relaxed(&sc->sc_task_thread) == curlwp)
		SIMPLEQ_INSERT_TAIL(&sc->sc_tasks, task, sdt_u.queue);
	else
		workqueue_enqueue(sc->sc_task_wq, &task->sdt_u.work, NULL);
}

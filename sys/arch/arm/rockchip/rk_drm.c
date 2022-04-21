/* $NetBSD: rk_drm.c,v 1.19 2022/04/21 21:22:25 andvar Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: rk_drm.c,v 1.19 2022/04/21 21:22:25 andvar Exp $");

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

#include <arm/rockchip/rk_drm.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_auth.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_damage_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_vblank.h>

#define	RK_DRM_MAX_WIDTH	3840
#define	RK_DRM_MAX_HEIGHT	2160

static TAILQ_HEAD(, rk_drm_ports) rk_drm_ports =
    TAILQ_HEAD_INITIALIZER(rk_drm_ports);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "rockchip,display-subsystem" },
	DEVICE_COMPAT_EOL
};

static const char * fb_compatible[] = {
	"simple-framebuffer",
	NULL
};

static int	rk_drm_match(device_t, cfdata_t, void *);
static void	rk_drm_attach(device_t, device_t, void *);

static void	rk_drm_init(device_t);
static vmem_t	*rk_drm_alloc_cma_pool(struct drm_device *, size_t);

static int	rk_drm_load(struct drm_device *, unsigned long);
static void	rk_drm_unload(struct drm_device *);

static void	rk_drm_task_work(struct work *, void *);

static struct drm_driver rk_drm_driver = {
	.driver_features = DRIVER_MODESET | DRIVER_ATOMIC | DRIVER_GEM,
	.dev_priv_size = 0,
	.load = rk_drm_load,
	.unload = rk_drm_unload,

	.gem_free_object = drm_gem_cma_free_object,
	.mmap_object = drm_gem_or_legacy_mmap_object,
	.gem_uvm_ops = &drm_gem_cma_uvm_ops,

	.dumb_create = drm_gem_cma_dumb_create,
	.dumb_destroy = drm_gem_dumb_destroy,

	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL,
};

CFATTACH_DECL_NEW(rk_drm, sizeof(struct rk_drm_softc),
	rk_drm_match, rk_drm_attach, NULL, NULL);

static int
rk_drm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
rk_drm_attach(device_t parent, device_t self, void *aux)
{
	struct rk_drm_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	struct drm_driver * const driver = &rk_drm_driver;
	prop_dictionary_t dict = device_properties(self);
	bool is_disabled;

	aprint_naive("\n");

	if (prop_dictionary_get_bool(dict, "disabled", &is_disabled) &&
	    is_disabled) {
		aprint_normal(": (disabled)\n");
		return;
	}

	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_dmat = faa->faa_dmat;
	sc->sc_bst = faa->faa_bst;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_task_thread = NULL;
	SIMPLEQ_INIT(&sc->sc_tasks);
	if (workqueue_create(&sc->sc_task_wq, "rkdrm",
	    &rk_drm_task_work, NULL, PRI_NONE, IPL_NONE, WQ_MPSAFE)) {
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

	/*
	 * Wait until rk_vop is attached as a sibling to this device --
	 * we need that to actually display our framebuffer.
	 */
	config_defer(self, rk_drm_init);
}

static void
rk_drm_init(device_t dev)
{
	struct rk_drm_softc * const sc = device_private(dev);
	struct drm_driver * const driver = &rk_drm_driver;
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
		struct rk_drm_task *const task = SIMPLEQ_FIRST(&sc->sc_tasks);

		SIMPLEQ_REMOVE_HEAD(&sc->sc_tasks, rdt_u.queue);
		(*task->rdt_fn)(task);
	}

out:	/* Cause any subsequent tasks to be processed by the workqueue.  */
	atomic_store_relaxed(&sc->sc_task_thread, NULL);
}

static vmem_t *
rk_drm_alloc_cma_pool(struct drm_device *ddev, size_t cma_size)
{
	struct rk_drm_softc * const sc = rk_drm_private(ddev);
	bus_dma_segment_t segs[1];
	int nsegs;
	int error;

	error = bus_dmamem_alloc(sc->sc_dmat, cma_size, PAGE_SIZE, 0,
	    segs, 1, &nsegs, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't allocate CMA pool\n");
		return NULL;
	}

	return vmem_create("rkdrm", segs[0].ds_addr, segs[0].ds_len,
	    PAGE_SIZE, NULL, NULL, NULL, 0, VM_SLEEP, IPL_NONE);
}

static int
rk_drm_fb_create_handle(struct drm_framebuffer *fb,
    struct drm_file *file, unsigned int *handle)
{
	struct rk_drm_framebuffer *sfb = to_rk_drm_framebuffer(fb);

	return drm_gem_handle_create(file, &sfb->obj->base, handle);
}

static void
rk_drm_fb_destroy(struct drm_framebuffer *fb)
{
	struct rk_drm_framebuffer *sfb = to_rk_drm_framebuffer(fb);

	drm_framebuffer_cleanup(fb);
	drm_gem_object_put_unlocked(&sfb->obj->base);
	kmem_free(sfb, sizeof(*sfb));
}

static const struct drm_framebuffer_funcs rk_drm_framebuffer_funcs = {
	.create_handle = rk_drm_fb_create_handle,
	.destroy = rk_drm_fb_destroy,
	.dirty = drm_atomic_helper_dirtyfb,
};

static struct drm_framebuffer *
rk_drm_fb_create(struct drm_device *ddev, struct drm_file *file,
    const struct drm_mode_fb_cmd2 *cmd)
{
	struct rk_drm_framebuffer *fb;
	struct drm_gem_object *gem_obj;
	int error;

	if (cmd->flags)
		return NULL;

	gem_obj = drm_gem_object_lookup(file, cmd->handles[0]);
	if (gem_obj == NULL)
		return NULL;

	fb = kmem_zalloc(sizeof(*fb), KM_SLEEP);
	drm_helper_mode_fill_fb_struct(ddev, &fb->base, cmd);
	fb->obj = to_drm_gem_cma_obj(gem_obj);

	error = drm_framebuffer_init(ddev, &fb->base, &rk_drm_framebuffer_funcs);
	if (error != 0)
		goto dealloc;

	return &fb->base;

dealloc:
	drm_framebuffer_cleanup(&fb->base);
	kmem_free(fb, sizeof(*fb));
	drm_gem_object_put_unlocked(gem_obj);

	return NULL;
}

static struct drm_mode_config_funcs rk_drm_mode_config_funcs = {
	.fb_create = rk_drm_fb_create,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
};

static struct drm_mode_config_helper_funcs rk_drm_mode_config_helper_funcs = {
	.atomic_commit_tail = drm_atomic_helper_commit_tail_rpm,
};

static int
rk_drm_fb_probe(struct drm_fb_helper *helper, struct drm_fb_helper_surface_size *sizes)
{
	struct rk_drm_softc * const sc = rk_drm_private(helper->dev);
	struct drm_device *ddev = helper->dev;
	struct rk_drm_framebuffer *sfb = to_rk_drm_framebuffer(helper->fb);
	struct drm_framebuffer *fb = helper->fb;
	struct rk_drmfb_attach_args sfa;
	size_t cma_size;
	int error;

	const u_int width = sizes->surface_width;
	const u_int height = sizes->surface_height;
	const u_int pitch = width * (32 / 8);

	const size_t size = roundup(height * pitch, PAGE_SIZE);

	/* Reserve enough memory for the FB console plus a 4K plane, rounded to 1MB */
	cma_size = size;
	cma_size += (RK_DRM_MAX_WIDTH * RK_DRM_MAX_HEIGHT * 4);
	cma_size = roundup(cma_size, 1024 * 1024);
	sc->sc_ddev->cma_pool = rk_drm_alloc_cma_pool(sc->sc_ddev, cma_size);
	if (sc->sc_ddev->cma_pool != NULL)
		aprint_normal_dev(sc->sc_dev, "reserved %u MB DRAM for CMA\n",
		    (u_int)(cma_size / (1024 * 1024)));

	sfb->obj = drm_gem_cma_create(ddev, size);
	if (sfb->obj == NULL) {
		DRM_ERROR("failed to allocate memory for framebuffer\n");
		return -ENOMEM;
	}

	/* similar to drm_helper_mode_fill_fb_struct(), but we have no cmd */
	fb->pitches[0] = pitch;
	fb->offsets[0] = 0;
	fb->width = width;
	fb->height = height;
	fb->modifier = 0;
	fb->flags = 0;
#ifdef __ARM_BIG_ENDIAN
	fb->format = drm_format_info(DRM_FORMAT_BGRX8888);
#else
	fb->format = drm_format_info(DRM_FORMAT_XRGB8888);
#endif
	fb->dev = ddev;

	error = drm_framebuffer_init(ddev, fb, &rk_drm_framebuffer_funcs);
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
	    CFARGS(.iattr = "rkfbbus"));
	if (helper->fbdev == NULL) {
		DRM_ERROR("unable to attach framebuffer\n");
		return -ENXIO;
	}

	return 0;
}

static struct drm_fb_helper_funcs rk_drm_fb_helper_funcs = {
	.fb_probe = rk_drm_fb_probe,
};

static int
rk_drm_load(struct drm_device *ddev, unsigned long flags)
{
	struct rk_drm_softc * const sc = rk_drm_private(ddev);
	struct rk_drm_ports *sport;
	struct rk_drm_fbdev *fbdev;
	struct fdt_endpoint *ep;
	const u_int *data;
	int datalen, error, num_crtc, ep_index;

	drm_mode_config_init(ddev);
	ddev->mode_config.min_width = 0;
	ddev->mode_config.min_height = 0;
	ddev->mode_config.max_width = RK_DRM_MAX_WIDTH;
	ddev->mode_config.max_height = RK_DRM_MAX_HEIGHT;
	ddev->mode_config.funcs = &rk_drm_mode_config_funcs;
	ddev->mode_config.helper_private = &rk_drm_mode_config_helper_funcs;

	num_crtc = 0;
	data = fdtbus_get_prop(sc->sc_phandle, "ports", &datalen);
	while (datalen >= 4) {
		const int crtc_phandle = fdtbus_get_phandle_from_native(be32dec(data));

		TAILQ_FOREACH(sport, &rk_drm_ports, entries)
			if (sport->phandle == crtc_phandle && sport->ddev == NULL) {
				sport->ddev = ddev;
				for (ep_index = 0; (ep = fdt_endpoint_get_from_index(sport->port, 0, ep_index)) != NULL; ep_index++) {
					error = fdt_endpoint_activate_direct(ep, true);
					if (error != 0)
						aprint_debug_dev(sc->sc_dev,
						    "failed to activate endpoint %d: %d\n",
						    ep_index, error);
				}
				num_crtc++;
			}

		datalen -= 4;
		data++;
	}

	if (num_crtc == 0) {
		aprint_error_dev(sc->sc_dev, "no display interface ports configured\n");
		error = ENXIO;
		goto drmerr;
	}

	drm_mode_config_reset(ddev);

	fbdev = kmem_zalloc(sizeof(*fbdev), KM_SLEEP);

	drm_fb_helper_prepare(ddev, &fbdev->helper, &rk_drm_fb_helper_funcs);

	error = drm_fb_helper_init(ddev, &fbdev->helper, num_crtc);
	if (error)
		goto allocerr;

	fbdev->helper.fb = kmem_zalloc(sizeof(struct rk_drm_framebuffer), KM_SLEEP);

	drm_fb_helper_single_add_all_connectors(&fbdev->helper);

	drm_fb_helper_initial_config(&fbdev->helper, 32);

	/* XXX Delegate this to rk_vop.c?  */
	ddev->irq_enabled = true;
	drm_vblank_init(ddev, num_crtc);

	return 0;

allocerr:
	kmem_free(fbdev, sizeof(*fbdev));
drmerr:
	drm_mode_config_cleanup(ddev);

	return error;
}

static void
rk_drm_unload(struct drm_device *ddev)
{
	drm_mode_config_cleanup(ddev);
}

int
rk_drm_register_port(int phandle, struct fdt_device_ports *port)
{
	struct rk_drm_ports *sport;

	sport = kmem_zalloc(sizeof(*sport), KM_SLEEP);
	sport->phandle = phandle;
	sport->port = port;
	sport->ddev = NULL;
	TAILQ_INSERT_TAIL(&rk_drm_ports, sport, entries);

	return 0;
}

struct drm_device *
rk_drm_port_device(struct fdt_device_ports *port)
{
	struct rk_drm_ports *sport;

	TAILQ_FOREACH(sport, &rk_drm_ports, entries)
		if (sport->port == port)
			return sport->ddev;

	return NULL;
}

static void
rk_drm_task_work(struct work *work, void *cookie)
{
	struct rk_drm_task *task = container_of(work, struct rk_drm_task,
	    rdt_u.work);

	(*task->rdt_fn)(task);
}

void
rk_task_init(struct rk_drm_task *task,
    void (*fn)(struct rk_drm_task *))
{

	task->rdt_fn = fn;
}

void
rk_task_schedule(device_t self, struct rk_drm_task *task)
{
	struct rk_drm_softc *sc = device_private(self);

	if (atomic_load_relaxed(&sc->sc_task_thread) == curlwp)
		SIMPLEQ_INSERT_TAIL(&sc->sc_tasks, task, rdt_u.queue);
	else
		workqueue_enqueue(sc->sc_task_wq, &task->rdt_u.work, NULL);
}

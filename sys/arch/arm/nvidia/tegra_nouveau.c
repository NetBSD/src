/* $NetBSD: tegra_nouveau.c,v 1.3 2015/10/18 14:04:32 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_nouveau.c,v 1.3 2015/10/18 14:04:32 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_var.h>

#include <drm/drmP.h>
#include <engine/device.h>

extern char *nouveau_config;
extern char *nouveau_debug;
extern struct drm_driver *const nouveau_drm_driver;

static int	tegra_nouveau_match(device_t, cfdata_t, void *);
static void	tegra_nouveau_attach(device_t, device_t, void *);

struct tegra_nouveau_softc {
	device_t		sc_dev;
	bus_dma_tag_t		sc_dmat;
	struct drm_device	*sc_drm_dev;
	struct platform_device	sc_platform_dev;
	struct nouveau_device	*sc_nv_dev;
};

static int	tegra_nouveau_init(struct tegra_nouveau_softc *);

static int	tegra_nouveau_get_irq(struct drm_device *);
static const char *tegra_nouveau_get_name(struct drm_device *);
static int	tegra_nouveau_set_busid(struct drm_device *,
					struct drm_master *);
static int	tegra_nouveau_irq_install(struct drm_device *,
					  irqreturn_t (*)(void *),
					  int, const char *, void *,
					  struct drm_bus_irq_cookie **);
static void	tegra_nouveau_irq_uninstall(struct drm_device *,
					    struct drm_bus_irq_cookie *);

static struct drm_bus drm_tegra_nouveau_bus = {
	.bus_type = DRIVER_BUS_PLATFORM,
	.get_irq = tegra_nouveau_get_irq,
	.get_name = tegra_nouveau_get_name,
	.set_busid = tegra_nouveau_set_busid,
	.irq_install = tegra_nouveau_irq_install,
	.irq_uninstall = tegra_nouveau_irq_uninstall
};

CFATTACH_DECL_NEW(tegra_nouveau, sizeof(struct tegra_nouveau_softc),
	tegra_nouveau_match, tegra_nouveau_attach, NULL, NULL);

static int
tegra_nouveau_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
tegra_nouveau_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_nouveau_softc * const sc = device_private(self);
	struct tegraio_attach_args * const tio = aux;
#if notyet
	const struct tegra_locators * const loc = &tio->tio_loc;
#endif
	int error;

	sc->sc_dev = self;
	sc->sc_dmat = tio->tio_dmat;

	aprint_naive("\n");
	aprint_normal(": GPU\n");

	tegra_car_gpu_enable();

	error = -nouveau_device_create(&sc->sc_platform_dev,
	    NOUVEAU_BUS_PLATFORM, -1, device_xname(self),
	    nouveau_config, nouveau_debug, &sc->sc_nv_dev);
	if (error) {
		aprint_error_dev(self, "couldn't create nouveau device: %d\n",
		    error);
		return;
	}

	error = tegra_nouveau_init(sc);
	if (error) {
		aprint_error_dev(self, "couldn't attach drm: %d\n", error);
		return;
	}
}

static int
tegra_nouveau_init(struct tegra_nouveau_softc *sc)
{
	struct drm_driver * const driver = nouveau_drm_driver;
	struct drm_device *dev;
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	int error;

	driver->kdriver.platform_device = &sc->sc_platform_dev;
	driver->bus = &drm_tegra_nouveau_bus;

	dev = drm_dev_alloc(driver, sc->sc_dev);
	if (dev == NULL)
		return ENOMEM;
	dev->platformdev = &sc->sc_platform_dev;

	dev->platformdev->id = -1;
	dev->platformdev->dev = *sc->sc_dev;	/* XXX */
	dev->platformdev->dmat = sc->sc_dmat;
	dev->platformdev->nresource = 2;
	dev->platformdev->resource[0].tag = bst;
	dev->platformdev->resource[0].start = TEGRA_GPU_BASE;
	dev->platformdev->resource[0].len = 0x01000000;
	dev->platformdev->resource[1].tag = bst;
	dev->platformdev->resource[1].start = TEGRA_GPU_BASE +
	    dev->platformdev->resource[0].len;
	dev->platformdev->resource[1].len = 0x01000000;

	error = -drm_dev_register(dev, 0);
	if (error) {
		drm_dev_unref(dev);
		return error;
	}

	device_printf(sc->sc_dev, "initialized %s %d.%d.%d %s on minor %d\n",
	    driver->name, driver->major, driver->minor, driver->patchlevel,
	    driver->date, dev->primary->index);

	return 0;
}

static int
tegra_nouveau_get_irq(struct drm_device *dev)
{
	return TEGRA_INTR_GPU;
}

static const char *tegra_nouveau_get_name(struct drm_device *dev)
{
	return "tegra_nouveau";
}

static int
tegra_nouveau_set_busid(struct drm_device *dev, struct drm_master *master)
{
	int id;

	id = dev->platformdev->id;
	if (id < 0)
		id = 0;

	master->unique = kmem_asprintf("platform:tegra_nouveau:%02d", id);
	if (master->unique == NULL)
		return -ENOMEM;
	master->unique_len = strlen(master->unique);

	return 0;
}

static int
tegra_nouveau_irq_install(struct drm_device *dev,
    irqreturn_t (*handler)(void *), int flags, const char *name, void *arg,
    struct drm_bus_irq_cookie **cookiep)
{
	void *ih;
	int irq = TEGRA_INTR_GPU;

	ih = intr_establish(irq, IPL_DRM, IST_LEVEL, handler, arg);
	if (ih == NULL) {
		aprint_error_dev(dev->dev,
		    "couldn't establish interrupt (%s)\n", name);
		return -ENOENT;
	}

	aprint_normal_dev(dev->dev, "interrupting on irq %d (%s)\n",
	    irq, name);
	*cookiep = (struct drm_bus_irq_cookie *)ih;
	return 0;
}

static void
tegra_nouveau_irq_uninstall(struct drm_device *dev,
    struct drm_bus_irq_cookie *cookie)
{
	intr_disestablish(cookie);
}

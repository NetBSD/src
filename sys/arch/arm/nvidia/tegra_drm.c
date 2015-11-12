/* $NetBSD: tegra_drm.c,v 1.3 2015/11/12 00:43:52 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_drm.c,v 1.3 2015/11/12 00:43:52 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_device.h>

#include <drm/drmP.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_var.h>
#include <arm/nvidia/tegra_drm.h>

static int	tegra_drm_match(device_t, cfdata_t, void *);
static void	tegra_drm_attach(device_t, device_t, void *);

static const char *tegra_drm_get_name(struct drm_device *);
static int	tegra_drm_set_busid(struct drm_device *, struct drm_master *);

static int	tegra_drm_load(struct drm_device *, unsigned long);
static int	tegra_drm_unload(struct drm_device *);

static int	tegra_drm_dumb_create(struct drm_file *, struct drm_device *,
		    struct drm_mode_create_dumb *);
static int	tegra_drm_dumb_map_offset(struct drm_file *,
		    struct drm_device *, uint32_t, uint64_t *);

static const struct uvm_pagerops tegra_drm_gem_uvm_ops = {
	.pgo_reference = drm_gem_pager_reference,
	.pgo_detach = drm_gem_pager_detach,
	.pgo_fault = tegra_drm_gem_fault,
};

static struct drm_driver tegra_drm_driver = {
	.driver_features = DRIVER_MODESET | DRIVER_GEM,
	.dev_priv_size = 0,
	.load = tegra_drm_load,
	.unload = tegra_drm_unload,

	.gem_free_object = tegra_drm_gem_free_object,
	.mmap_object = drm_gem_or_legacy_mmap_object,
	.gem_uvm_ops = &tegra_drm_gem_uvm_ops,

	.dumb_create = tegra_drm_dumb_create,
	.dumb_map_offset = tegra_drm_dumb_map_offset,
	.dumb_destroy = drm_gem_dumb_destroy,

	.get_vblank_counter = tegra_drm_get_vblank_counter,
	.enable_vblank = tegra_drm_enable_vblank,
	.disable_vblank = tegra_drm_disable_vblank,

	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL 
};

static const struct drm_bus tegra_drm_bus = {
	.bus_type = DRIVER_BUS_PLATFORM,
	.get_name = tegra_drm_get_name,
	.set_busid = tegra_drm_set_busid
};

CFATTACH_DECL_NEW(tegra_drm, sizeof(struct tegra_drm_softc),
	tegra_drm_match, tegra_drm_attach, NULL, NULL);

static int
tegra_drm_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
tegra_drm_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_drm_softc * const sc = device_private(self);
	struct tegraio_attach_args * const tio = aux;
	prop_dictionary_t prop = device_properties(self);
	struct drm_driver * const driver = &tegra_drm_driver;
	const char *pin, *dev;
	int error;

	sc->sc_dev = self;
	sc->sc_dmat = tio->tio_dmat;
	sc->sc_bst = tio->tio_bst;

	if (prop_dictionary_get_cstring_nocopy(prop, "hpd-gpio", &pin)) {
		sc->sc_pin_hpd = tegra_gpio_acquire(pin, GPIO_PIN_INPUT);
	}
	if (prop_dictionary_get_cstring_nocopy(prop, "pll-gpio", &pin)) {
		sc->sc_pin_pll = tegra_gpio_acquire(pin, GPIO_PIN_OUTPUT);
		if (sc->sc_pin_pll) {
			tegra_gpio_write(sc->sc_pin_pll, 0);
		} else {
			panic("couldn't get pll-gpio pin");
		}
	}
	if (prop_dictionary_get_cstring_nocopy(prop, "power-gpio", &pin)) {
		sc->sc_pin_power = tegra_gpio_acquire(pin, GPIO_PIN_OUTPUT);
		if (sc->sc_pin_power) {
			tegra_gpio_write(sc->sc_pin_power, 1);
		}
	}
	if (prop_dictionary_get_cstring_nocopy(prop, "ddc-device", &dev)) {
		sc->sc_ddcdev = device_find_by_xname(dev);
	}
	prop_dictionary_get_bool(prop, "force-dvi", &sc->sc_force_dvi);

	aprint_naive("\n");
	aprint_normal("\n");

	driver->bus = &tegra_drm_bus;

	sc->sc_ddev = drm_dev_alloc(driver, sc->sc_dev);
	if (sc->sc_ddev == NULL) {
		aprint_error_dev(self, "couldn't allocate DRM device\n");
		return;
	}
	sc->sc_ddev->dev_private = sc;

	error = -drm_dev_register(sc->sc_ddev, 0);
	if (error) {
		drm_dev_unref(sc->sc_ddev);
		aprint_error_dev(self, "couldn't register DRM device: %d\n",
		    error);
		return;
	}

	aprint_normal_dev(self, "initialized %s %d.%d.%d %s on minor %d\n",
	    driver->name, driver->major, driver->minor, driver->patchlevel,
	    driver->date, sc->sc_ddev->primary->index);

	return;
}

static const char *
tegra_drm_get_name(struct drm_device *ddev)
{
	return DRIVER_NAME;
}

static int
tegra_drm_set_busid(struct drm_device *ddev, struct drm_master *master)
{
	const char *id = "platform:tegra:0";

	master->unique = kzalloc(strlen(id) + 1, GFP_KERNEL);
	if (master->unique == NULL)
		return -ENOMEM;
	strcpy(master->unique, id);
	master->unique_len = strlen(master->unique);

	return 0;
}


static int
tegra_drm_load(struct drm_device *ddev, unsigned long flags)
{
	char *devname;
	int error;

	devname = kzalloc(strlen(DRIVER_NAME) + 1, GFP_KERNEL);
	if (devname == NULL) {
		return -ENOMEM;
	}
	strcpy(devname, DRIVER_NAME);
	ddev->devname = devname;

	error = tegra_drm_mode_init(ddev);
	if (error)
		goto drmerr;

	error = tegra_drm_fb_init(ddev);
	if (error)
		goto drmerr;

	return 0;

drmerr:
	drm_mode_config_cleanup(ddev);

	return error;
}

static int
tegra_drm_unload(struct drm_device *ddev)
{
	drm_mode_config_cleanup(ddev);

	return 0;
}

static int
tegra_drm_dumb_create(struct drm_file *file_priv, struct drm_device *ddev,
    struct drm_mode_create_dumb *args)
{
	struct tegra_gem_object *obj;
	uint32_t handle;
	int error;

	args->pitch = args->width * ((args->bpp + 7) / 8);
	args->size = args->pitch * args->height;
	args->size = roundup(args->size, PAGE_SIZE);
	args->handle = 0;

	obj = tegra_drm_obj_alloc(ddev, args->size);
	if (obj == NULL)
		return -ENOMEM;

	error = drm_gem_handle_create(file_priv, &obj->base, &handle);
	drm_gem_object_unreference_unlocked(&obj->base);
	if (error) {
		tegra_drm_obj_free(obj);
		return error;
	}

	args->handle = handle;

	return 0;
}

static int
tegra_drm_dumb_map_offset(struct drm_file *file_priv,
    struct drm_device *ddev, uint32_t handle, uint64_t *offset)
{
	struct drm_gem_object *gem_obj;
	struct tegra_gem_object *obj;
	int error;

	gem_obj = drm_gem_object_lookup(ddev, file_priv, handle);
	if (gem_obj == NULL)
		return -ENOENT;

	obj = to_tegra_gem_obj(gem_obj);

	if (drm_vma_node_has_offset(&obj->base.vma_node) == 0) {
		error = drm_gem_create_mmap_offset(&obj->base);
		if (error)
			goto done;
	}

	*offset = drm_vma_node_offset_addr(&obj->base.vma_node);

done:
	drm_gem_object_unreference_unlocked(&obj->base);

	return error;
}

/* $NetBSD: tegra_drm.c,v 1.7.10.2 2017/12/03 11:35:54 jdolecek Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_drm.c,v 1.7.10.2 2017/12/03 11:35:54 jdolecek Exp $");

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

#include <dev/fdt/fdtvar.h>

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
	const char * compatible[] = { "nvidia,tegra124-host1x", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_drm_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_drm_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	struct drm_driver * const driver = &tegra_drm_driver;
	prop_dictionary_t prop = device_properties(self);
	int error, node, hdmi_phandle, ddc_phandle;
	const char * const hdmi_compat[] = { "nvidia,tegra124-hdmi", NULL };
	const char * const dc_compat[] = { "nvidia,tegra124-dc", NULL };
	const char * const hdmi_supplies[] = {
		"hdmi-supply", "pll-supply", "vdd-supply"
	};
	struct fdtbus_regulator *reg;
	u_int n, ndc;

	sc->sc_dev = self;
	sc->sc_dmat = faa->faa_dmat;
	sc->sc_bst = faa->faa_bst;
	sc->sc_phandle = faa->faa_phandle;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_clk_host1x = fdtbus_clock_get_index(faa->faa_phandle, 0);
	if (sc->sc_clk_host1x == NULL) {
		aprint_error_dev(self, "couldn't get clock host1x\n");
		return;
	}
	sc->sc_rst_host1x = fdtbus_reset_get(faa->faa_phandle, "host1x");
	if (sc->sc_clk_host1x == NULL || sc->sc_rst_host1x == NULL) {
		aprint_error_dev(self, "couldn't get reset host1x\n");
		return;
	}

	ndc = 0;
	hdmi_phandle = -1;
	for (node = OF_child(faa->faa_phandle); node; node = OF_peer(node)) {
		if (of_match_compatible(node, hdmi_compat)) {
			sc->sc_clk_hdmi = fdtbus_clock_get(node, "hdmi");
			sc->sc_clk_hdmi_parent = fdtbus_clock_get(node,
			    "parent");
			sc->sc_rst_hdmi = fdtbus_reset_get(node, "hdmi");
			hdmi_phandle = node;
		} else if (of_match_compatible(node, dc_compat) &&
			   ndc < __arraycount(sc->sc_clk_dc)) {
			sc->sc_clk_dc[ndc] = fdtbus_clock_get(node, "dc");
			sc->sc_clk_dc_parent[ndc] = fdtbus_clock_get(node,
			    "parent");
			sc->sc_rst_dc[ndc] = fdtbus_reset_get(node, "dc");
			++ndc;
		}
	}
	if (hdmi_phandle >= 0) {
		ddc_phandle = fdtbus_get_phandle(hdmi_phandle,
		    "nvidia,ddc-i2c-bus");
		if (ddc_phandle >= 0) {
			sc->sc_ddc = fdtbus_get_i2c_tag(ddc_phandle);
		}

		sc->sc_pin_hpd = fdtbus_gpio_acquire(hdmi_phandle,
		    "nvidia,hpd-gpio", GPIO_PIN_INPUT);

		for (n = 0; n < __arraycount(hdmi_supplies); n++) {
			const char *supply = hdmi_supplies[n];
			reg = fdtbus_regulator_acquire(hdmi_phandle, supply);
			if (reg == NULL) {
				aprint_error_dev(self, "couldn't acquire %s\n",
				    supply);
				continue;
			}
			if (fdtbus_regulator_enable(reg) != 0) {
				aprint_error_dev(self, "couldn't enable %s\n",
				    supply);
			}
			fdtbus_regulator_release(reg);
		}
	}

	fdtbus_reset_assert(sc->sc_rst_host1x);
	error = clk_enable(sc->sc_clk_host1x);
	if (error) {
		aprint_error_dev(self, "couldn't enable clock host1x: %d\n",
		    error);
		return;
	}
	fdtbus_reset_deassert(sc->sc_rst_host1x);

	prop_dictionary_get_bool(prop, "force-dvi", &sc->sc_force_dvi);

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
	} else {
		error = 0;
	}

	*offset = drm_vma_node_offset_addr(&obj->base.vma_node);

done:
	drm_gem_object_unreference_unlocked(&obj->base);

	return error;
}

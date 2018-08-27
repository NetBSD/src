/* $NetBSD: tegra_nouveau.c,v 1.11 2018/08/27 15:31:51 riastradh Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_nouveau.c,v 1.11 2018/08/27 15:31:51 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_pmcreg.h>
#include <arm/nvidia/tegra_var.h>
#include <arm/nvidia/tegra_intr.h>

#include <dev/fdt/fdtvar.h>

#include <drm/drmP.h>
#include <core/tegra.h>
#include <nvkm/engine/device/priv.h>
#include <nvkm/subdev/mc/priv.h>

extern char *nouveau_config;
extern char *nouveau_debug;
extern struct drm_driver *const nouveau_drm_driver_stub;     /* XXX */
extern struct drm_driver *const nouveau_drm_driver_platform; /* XXX */
static bool nouveau_driver_initialized = false;		     /* XXX */

static int	tegra_nouveau_match(device_t, cfdata_t, void *);
static void	tegra_nouveau_attach(device_t, device_t, void *);

struct tegra_nouveau_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	struct {
		bus_addr_t		addr;
		bus_size_t		size;
	}			sc_resources[2];
	bus_dma_tag_t		sc_dmat;
	int			sc_phandle;
	void			*sc_ih;
	struct clk		*sc_clk_gpu;
	struct clk		*sc_clk_pwr;
	struct fdtbus_reset	*sc_rst_gpu;
	struct drm_device	*sc_drm_dev;
	struct nvkm_device	sc_nv_dev;
};

static void	tegra_nouveau_init(device_t);

static int	tegra_nouveau_set_busid(struct drm_device *,
					struct drm_master *);

static int	tegra_nouveau_intr(void *);

static int	nvkm_device_tegra_init(struct nvkm_device *);
static void	nvkm_device_tegra_fini(struct nvkm_device *, bool);

static bus_space_tag_t
		nvkm_device_tegra_resource_tag(struct nvkm_device *, unsigned);
static bus_addr_t
		nvkm_device_tegra_resource_addr(struct nvkm_device *, unsigned);
static bus_size_t
		nvkm_device_tegra_resource_size(struct nvkm_device *, unsigned);

CFATTACH_DECL_NEW(tegra_nouveau, sizeof(struct tegra_nouveau_softc),
	tegra_nouveau_match, tegra_nouveau_attach, NULL, NULL);

static const struct nvkm_device_func nvkm_device_tegra_func = {
	.tegra = NULL,		/* XXX */
	.dtor = NULL,
	.init = nvkm_device_tegra_init,
	.fini = nvkm_device_tegra_fini,
	.resource_tag = nvkm_device_tegra_resource_tag,
	.resource_addr = nvkm_device_tegra_resource_addr,
	.resource_size = nvkm_device_tegra_resource_size,
	.cpu_coherent = false,
};

static const struct nvkm_device_tegra_func gk20a_platform_data = {
	.iommu_bit = 34,
};

static const struct of_compat_data compat_data[] = {
	{ "nvidia,gk20a", (uintptr_t)&gk20a_platform_data },
	{ "nvidia,gm20b", (uintptr_t)&gk20a_platform_data },
	{ NULL, 0 },
};

static int
tegra_nouveau_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
tegra_nouveau_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_nouveau_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	prop_dictionary_t prop = device_properties(self);
	const struct of_compat_data *data =
	    of_search_compatible(faa->faa_phandle, compat_data);
	const struct nvkm_device_tegra_func *tegra_func =
	    (const void *)data->data;
	int error;

	KASSERT(tegra_func != NULL);

	if (!nouveau_driver_initialized) {
		*nouveau_drm_driver_platform = *nouveau_drm_driver_stub;
		nouveau_drm_driver_platform->set_busid =
		    tegra_nouveau_set_busid;
		nouveau_driver_initialized = true;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_dmat = faa->faa_dmat;
	sc->sc_phandle = faa->faa_phandle;

	sc->sc_clk_gpu = fdtbus_clock_get(faa->faa_phandle, "gpu");
	if (sc->sc_clk_gpu == NULL) {
		aprint_error(": couldn't get clock gpu\n");
		return;
	}
	sc->sc_clk_pwr = fdtbus_clock_get(faa->faa_phandle, "pwr");
	if (sc->sc_clk_pwr == NULL) {
		aprint_error(": couldn't get clock pwr\n");
		return;
	}
	sc->sc_rst_gpu = fdtbus_reset_get(faa->faa_phandle, "gpu");
	if (sc->sc_rst_gpu == NULL) {
		aprint_error(": couldn't get reset gpu\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": GPU\n");

	prop_dictionary_get_cstring(prop, "debug", &nouveau_debug);
	prop_dictionary_get_cstring(prop, "config", &nouveau_config);

	fdtbus_reset_assert(sc->sc_rst_gpu);
	error = clk_set_rate(sc->sc_clk_pwr, 204000000);
	if (error) {
		aprint_error_dev(self, "couldn't set clock pwr frequency: %d\n",
		    error);
		return;
	}
	error = clk_enable(sc->sc_clk_pwr);
	if (error) {
		aprint_error_dev(self, "couldn't enable clock pwr: %d\n",
		    error);
		return;
	}
	error = clk_enable(sc->sc_clk_gpu);
	if (error) {
		aprint_error_dev(self, "couldn't enable clock gpu: %d\n",
		    error);
		return;
	}
	tegra_pmc_remove_clamping(PMC_PARTID_TD);
	fdtbus_reset_deassert(sc->sc_rst_gpu);

	error = -nvkm_device_ctor(&nvkm_device_tegra_func, NULL, self,
	    NVKM_DEVICE_TEGRA, -1, device_xname(self),
	    nouveau_config, nouveau_debug, true, true, ~0ULL, &sc->sc_nv_dev);
	if (error) {
		aprint_error_dev(self, "couldn't create nouveau device: %d\n",
		    error);
		return;
	}

	config_mountroot(self, tegra_nouveau_init);
}

static void
tegra_nouveau_init(device_t self)
{
	struct tegra_nouveau_softc * const sc = device_private(self);
	struct drm_driver * const driver = nouveau_drm_driver_platform;
	struct drm_device *dev;
	bus_addr_t addr[2], size[2];
	int error;

	if (fdtbus_get_reg(sc->sc_phandle, 0, &addr[0], &size[0]) != 0 ||
	    fdtbus_get_reg(sc->sc_phandle, 1, &addr[1], &size[1]) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	dev = drm_dev_alloc(driver, sc->sc_dev);
	if (dev == NULL) {
		aprint_error_dev(self, "couldn't allocate DRM device\n");
		return;
	}
	dev->bst = sc->sc_bst;
	dev->bus_dmat = sc->sc_dmat;
	dev->dmat = dev->bus_dmat;
	dev->dmat_subregion_p = false;

	sc->sc_resources[0].addr = addr[0];
	sc->sc_resources[0].size = size[0];
	sc->sc_resources[1].addr = addr[1];
	sc->sc_resources[1].size = size[1];

	error = -drm_dev_register(dev, 0);
	if (error) {
		drm_dev_unref(dev);
		aprint_error_dev(self, "couldn't register DRM device: %d\n",
		    error);
		return;
	}

	aprint_normal_dev(self, "initialized %s %d.%d.%d %s on minor %d\n",
	    driver->name, driver->major, driver->minor, driver->patchlevel,
	    driver->date, dev->primary->index);
}

static int
tegra_nouveau_set_busid(struct drm_device *dev, struct drm_master *master)
{
	int id = 0;		/* XXX device_unit(self)? */

	master->unique = kmem_asprintf("platform:tegra_nouveau:%02d", id);
	if (master->unique == NULL)
		return -ENOMEM;
	master->unique_len = strlen(master->unique);

	return 0;
}

static int
tegra_nouveau_intr(void *cookie)
{
	struct tegra_nouveau_softc *sc = cookie;
	struct nvkm_mc *mc = sc->sc_nv_dev.mc;
	bool handled = false;

	if (__predict_true(mc)) {
		nvkm_mc_intr_unarm(mc);
		nvkm_mc_intr(mc, &handled);
		nvkm_mc_intr_rearm(mc);
	}

	return handled;
}

static int
nvkm_device_tegra_init(struct nvkm_device *nvdev)
{
	struct tegra_nouveau_softc *sc =
	    container_of(nvdev, struct tegra_nouveau_softc, sc_nv_dev);
	device_t self = sc->sc_dev;
	char intrstr[128];

	if (!fdtbus_intr_str(sc->sc_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return -EIO;
	}

	sc->sc_ih = fdtbus_intr_establish(sc->sc_phandle, 0, IPL_VM,
	    FDT_INTR_MPSAFE, tegra_nouveau_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return -EIO;
	}

	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
	return 0;
}

static void
nvkm_device_tegra_fini(struct nvkm_device *nvdev, bool suspend)
{
	struct tegra_nouveau_softc *sc =
	    container_of(nvdev, struct tegra_nouveau_softc, sc_nv_dev);

	fdtbus_intr_disestablish(sc->sc_phandle, sc->sc_ih);
}

static bus_space_tag_t
nvkm_device_tegra_resource_tag(struct nvkm_device *dev, unsigned bar)
{
	struct tegra_nouveau_softc *sc =
	    container_of(dev, struct tegra_nouveau_softc, sc_nv_dev);

	KASSERT(bar < 2);
	return sc->sc_bst;
}

static bus_addr_t
nvkm_device_tegra_resource_addr(struct nvkm_device *dev, unsigned bar)
{
	struct tegra_nouveau_softc *sc =
	    container_of(dev, struct tegra_nouveau_softc, sc_nv_dev);

	KASSERT(bar < 2);
	return sc->sc_resources[bar].addr;
}

static bus_size_t
nvkm_device_tegra_resource_size(struct nvkm_device *dev, unsigned bar)
{
	struct tegra_nouveau_softc *sc =
	    container_of(dev, struct tegra_nouveau_softc, sc_nv_dev);

	KASSERT(bar < 2);
	return sc->sc_resources[bar].size;
}

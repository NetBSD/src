/* $NetBSD: tegra_nouveau.c,v 1.10.8.2 2017/12/03 11:35:54 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_nouveau.c,v 1.10.8.2 2017/12/03 11:35:54 jdolecek Exp $");

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
#include <engine/device.h>

extern char *nouveau_config;
extern char *nouveau_debug;
extern struct drm_driver *const nouveau_drm_driver;

static int	tegra_nouveau_match(device_t, cfdata_t, void *);
static void	tegra_nouveau_attach(device_t, device_t, void *);

struct tegra_nouveau_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_dma_tag_t		sc_dmat;
	int			sc_phandle;
	struct clk		*sc_clk_gpu;
	struct clk		*sc_clk_pwr;
	struct fdtbus_reset	*sc_rst_gpu;
	struct drm_device	*sc_drm_dev;
	struct platform_device	sc_platform_dev;
	struct nouveau_device	*sc_nv_dev;
};

static void	tegra_nouveau_init(device_t);

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
	const char * const compatible[] = { "nvidia,gk20a", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_nouveau_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_nouveau_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	prop_dictionary_t prop = device_properties(self);
	int error;

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

	error = -nouveau_device_create(&sc->sc_platform_dev,
	    NOUVEAU_BUS_PLATFORM, -1, device_xname(self),
	    nouveau_config, nouveau_debug, &sc->sc_nv_dev);
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
	struct drm_driver * const driver = nouveau_drm_driver;
	struct drm_device *dev;
	bus_addr_t addr[2], size[2];
	int error;

	if (fdtbus_get_reg(sc->sc_phandle, 0, &addr[0], &size[0]) != 0 ||
	    fdtbus_get_reg(sc->sc_phandle, 1, &addr[1], &size[1]) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	driver->kdriver.platform_device = &sc->sc_platform_dev;
	driver->bus = &drm_tegra_nouveau_bus;

	dev = drm_dev_alloc(driver, sc->sc_dev);
	if (dev == NULL) {
		aprint_error_dev(self, "couldn't allocate DRM device\n");
		return;
	}
	dev->bst = sc->sc_bst;
	dev->bus_dmat = sc->sc_dmat;
	dev->dmat = dev->bus_dmat;
	dev->dmat_subregion_p = false;
	dev->platformdev = &sc->sc_platform_dev;

	dev->platformdev->id = -1;
	dev->platformdev->pd_dev = sc->sc_dev;
	dev->platformdev->dmat = sc->sc_dmat;
	dev->platformdev->nresource = 2;
	dev->platformdev->resource[0].tag = sc->sc_bst;
	dev->platformdev->resource[0].start = addr[0];
	dev->platformdev->resource[0].len = size[0];
	dev->platformdev->resource[1].tag = sc->sc_bst;
	dev->platformdev->resource[1].start = addr[1];
	dev->platformdev->resource[1].len = size[1];

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
	struct tegra_nouveau_softc * const sc = device_private(dev->dev);
	char intrstr[128];
	char *inames, *p;
	u_int index;
	void *ih;
	int len, resid;

	len = OF_getproplen(sc->sc_phandle, "interrupt-names");
	if (len <= 0) {
		aprint_error_dev(dev->dev, "no interrupt-names property\n");
		return -EIO;
	}

	inames = kmem_alloc(len, KM_SLEEP);
	if (OF_getprop(sc->sc_phandle, "interrupt-names", inames, len) != len) {
		aprint_error_dev(dev->dev, "failed to get interrupt-names\n");
		kmem_free(inames, len);
		return -EIO;
	}
	p = inames;
	resid = len;
	index = 0;
	while (resid > 0) {
		if (strcmp(name, p) == 0)
			break;
		const int slen = strlen(p) + 1;
		p += slen;
		len -= slen;
		++index;
	}
	kmem_free(inames, len);
	if (len == 0) {
		aprint_error_dev(dev->dev, "unknown interrupt name '%s'\n",
		    name);
		return -EINVAL;
	}

	if (!fdtbus_intr_str(sc->sc_phandle, index, intrstr, sizeof(intrstr))) {
		aprint_error_dev(dev->dev, "failed to decode interrupt\n");
		return -ENXIO;
	}

	ih = fdtbus_intr_establish(sc->sc_phandle, index, IPL_DRM,
	    FDT_INTR_MPSAFE, handler, arg);
	if (ih == NULL) {
		aprint_error_dev(dev->dev,
		    "failed to establish interrupt on %s\n", intrstr);
		return -ENOENT;
	}

	aprint_normal_dev(dev->dev, "interrupting on %s\n", intrstr);

	*cookiep = (struct drm_bus_irq_cookie *)ih;
	return 0;
}

static void
tegra_nouveau_irq_uninstall(struct drm_device *dev,
    struct drm_bus_irq_cookie *cookie)
{
	struct tegra_nouveau_softc * const sc = device_private(dev->dev);

	fdtbus_intr_disestablish(sc->sc_phandle, cookie);
}

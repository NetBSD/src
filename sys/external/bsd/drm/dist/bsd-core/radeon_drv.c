/* radeon_drv.c -- ATI Radeon driver -*- linux-c -*-
 * Created: Wed Feb 14 17:10:04 2001 by gareth@valinux.com
 */
/*-
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Gareth Hughes <gareth@valinux.com>
 *
 */

#include "drmP.h"
#include "drm.h"
#include "radeon_drm.h"
#include "radeon_drv.h"
#include "drm_pciids.h"

int radeon_no_wb;

/* drv_PCI_IDs comes from drm_pciids.h, generated from drm_pciids.txt. */
static drm_pci_id_list_t radeon_pciidlist[] = {
	radeon_PCI_IDS
};

static void radeon_configure(struct drm_device *dev)
{
	dev->driver->driver_features =
	    DRIVER_USE_AGP | DRIVER_USE_MTRR | DRIVER_PCI_DMA |
	    DRIVER_SG | DRIVER_HAVE_DMA | DRIVER_HAVE_IRQ;

	dev->driver->buf_priv_size	= sizeof(drm_radeon_buf_priv_t);
	dev->driver->load		= radeon_driver_load;
	dev->driver->unload		= radeon_driver_unload;
	dev->driver->firstopen		= radeon_driver_firstopen;
	dev->driver->open		= radeon_driver_open;
	dev->driver->preclose		= radeon_driver_preclose;
	dev->driver->postclose		= radeon_driver_postclose;
	dev->driver->lastclose		= radeon_driver_lastclose;
	dev->driver->get_vblank_counter	= radeon_get_vblank_counter;
	dev->driver->enable_vblank	= radeon_enable_vblank;
	dev->driver->disable_vblank	= radeon_disable_vblank;
	dev->driver->irq_preinstall	= radeon_driver_irq_preinstall;
	dev->driver->irq_postinstall	= radeon_driver_irq_postinstall;
	dev->driver->irq_uninstall	= radeon_driver_irq_uninstall;
	dev->driver->irq_handler	= radeon_driver_irq_handler;
	dev->driver->dma_ioctl		= radeon_cp_buffers;

	dev->driver->ioctls		= radeon_ioctls;
	dev->driver->max_ioctl		= radeon_max_ioctl;

	dev->driver->name		= DRIVER_NAME;
	dev->driver->desc		= DRIVER_DESC;
	dev->driver->date		= DRIVER_DATE;
	dev->driver->major		= DRIVER_MAJOR;
	dev->driver->minor		= DRIVER_MINOR;
	dev->driver->patchlevel		= DRIVER_PATCHLEVEL;
}

#if defined(__FreeBSD__)
static int
radeon_probe(device_t kdev)
{
	return drm_probe(kdev, radeon_pciidlist);
}

static int
radeon_attach(device_t kdev)
{
	struct drm_device *dev = device_get_softc(kdev);

	dev->driver = malloc(sizeof(struct drm_driver_info), DRM_MEM_DRIVER,
	    M_WAITOK | M_ZERO);

	radeon_configure(dev);

	return drm_attach(kdev, radeon_pciidlist);
}

static int
radeon_detach(device_t kdev)
{
	struct drm_device *dev = device_get_softc(kdev);
	int ret;

	ret = drm_detach(kdev);

	free(dev->driver, DRM_MEM_DRIVER);

	return ret;
}

static device_method_t radeon_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,		radeon_probe),
	DEVMETHOD(device_attach,	radeon_attach),
	DEVMETHOD(device_detach,	radeon_detach),

	{ 0, 0 }
};

static driver_t radeon_driver = {
	"drm",
	radeon_methods,
	sizeof(struct drm_device)
};

extern devclass_t drm_devclass;
#if __FreeBSD_version >= 700010
DRIVER_MODULE(radeon, vgapci, radeon_driver, drm_devclass, 0, 0);
#else
DRIVER_MODULE(radeon, pci, radeon_driver, drm_devclass, 0, 0);
#endif
MODULE_DEPEND(radeon, drm, 1, 1, 1);

#elif   defined(__NetBSD__)

static bool
radeondrm_suspend(device_t self, pmf_qual_t qual)
{
	struct drm_device *rad_dev = device_private(self);
	drm_radeon_cp_stop_t stop_args;

	stop_args.flush = stop_args.idle = 0;
	if (radeon_cp_stop(rad_dev, &stop_args, rad_dev->lock.file_priv) != 0)
		return false;

	return true;
}

static bool
radeondrm_resume(device_t self, pmf_qual_t qual)
{
	struct drm_device *rad_dev = device_private(self);
	if (radeon_cp_resume(rad_dev, NULL, NULL) != 0)
		return false;
	
	return true;
}

static int
radeondrm_probe(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;
	return drm_probe(pa, radeon_pciidlist);
}

static void
radeondrm_attach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct drm_device *dev = device_private(self);

	dev->driver = malloc(sizeof(struct drm_driver_info), DRM_MEM_DRIVER,
	    M_WAITOK | M_ZERO);

	radeon_configure(dev);

	if (!pmf_device_register(self, radeondrm_suspend, radeondrm_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");

	drm_attach(self, pa, radeon_pciidlist);
}

static int
radeondrm_detach(device_t self, int flags)
{
	pmf_device_deregister(self);

	return drm_detach(self, flags);
}

CFATTACH_DECL_NEW(radeondrm, sizeof(struct drm_device),
    radeondrm_probe, radeondrm_attach, radeondrm_detach, NULL);

#ifdef _MODULE

MODULE(MODULE_CLASS_DRIVER, radeondrm, NULL);

CFDRIVER_DECL(radeondrm, DV_DULL, NULL);
extern struct cfattach radeondrm_ca;
static int drmloc[] = { -1 };
static struct cfparent drmparent = {
	"drm", "vga", DVUNIT_ANY
};
static struct cfdata radeondrm_cfdata[] = {
	{
		.cf_name = "radeondrm",
		.cf_atname = "radeondrm",
		.cf_unit = 0,
		.cf_fstate = FSTATE_STAR,
		.cf_loc = drmloc,
		.cf_flags = 0,
		.cf_pspec = &drmparent,
	},
	{ NULL }
};

static int
radeondrm_modcmd(modcmd_t cmd, void *arg)
{
	int err;

	switch (cmd) {
	case MODULE_CMD_INIT:
		err = config_cfdriver_attach(&radeondrm_cd);
		if (err)
			return err;
		err = config_cfattach_attach("radeondrm", &radeondrm_ca);
		if (err) {
			config_cfdriver_detach(&radeondrm_cd);
			return err;
		}
		err = config_cfdata_attach(radeondrm_cfdata, 1);
		if (err) {
			config_cfattach_detach("radeondrm", &radeondrm_ca);
			config_cfdriver_detach(&radeondrm_cd);
			return err;
		}
		return 0;
	case MODULE_CMD_FINI:
		err = config_cfdata_detach(radeondrm_cfdata);
		if (err)
			return err;
		config_cfattach_detach("radeondrm", &radeondrm_ca);
		config_cfdriver_detach(&radeondrm_cd);
		return 0;
	default:
		return ENOTTY;
	}
}
#endif /* _MODULE */

#endif

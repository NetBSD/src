/* sis.c -- sis driver -*- linux-c -*-
 */
/*-
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
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
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "drmP.h"
#include "sis_drm.h"
#include "sis_drv.h"
#include "drm_pciids.h"

/* drv_PCI_IDs comes from drm_pciids.h, generated from drm_pciids.txt. */
static drm_pci_id_list_t sis_pciidlist[] = {
	sis_PCI_IDS
};

static void sis_configure(struct drm_device *dev)
{
	dev->driver->driver_features =
	    DRIVER_USE_AGP | DRIVER_USE_MTRR;

	dev->driver->buf_priv_size	= 1; /* No dev_priv */
	dev->driver->context_ctor	= sis_init_context;
	dev->driver->context_dtor	= sis_final_context;

	dev->driver->ioctls		= sis_ioctls;
	dev->driver->max_ioctl		= sis_max_ioctl;

	dev->driver->name		= DRIVER_NAME;
	dev->driver->desc		= DRIVER_DESC;
	dev->driver->date		= DRIVER_DATE;
	dev->driver->major		= DRIVER_MAJOR;
	dev->driver->minor		= DRIVER_MINOR;
	dev->driver->patchlevel		= DRIVER_PATCHLEVEL;
}

#if defined(__FreeBSD__)

static int
sis_probe(device_t kdev)
{
	return drm_probe(kdev, sis_pciidlist);
}

static int
sis_attach(device_t kdev)
{
	struct drm_device *dev = device_get_softc(kdev);

	dev->driver = malloc(sizeof(struct drm_driver_info), DRM_MEM_DRIVER,
	    M_WAITOK | M_ZERO);

	sis_configure(dev);

	return drm_attach(kdev, sis_pciidlist);
}

static int
sis_detach(device_t kdev)
{
	struct drm_device *dev = device_get_softc(kdev);
	int ret;

	ret = drm_detach(kdev);

	free(dev->driver, DRM_MEM_DRIVER);

	return ret;
}

static device_method_t sis_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,		sis_probe),
	DEVMETHOD(device_attach,	sis_attach),
	DEVMETHOD(device_detach,	sis_detach),

	{ 0, 0 }
};

static driver_t sis_driver = {
	"drm",
	sis_methods,
	sizeof(struct drm_device)
};

extern devclass_t drm_devclass;
#if __FreeBSD_version >= 700010
DRIVER_MODULE(sisdrm, vgapci, sis_driver, drm_devclass, 0, 0);
#else
DRIVER_MODULE(sisdrm, pci, sis_driver, drm_devclass, 0, 0);
#endif
MODULE_DEPEND(sisdrm, drm, 1, 1, 1);

#elif   defined(__NetBSD__)

static int
sisdrm_probe(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	return drm_probe(pa, sis_pciidlist);
}

static void
sisdrm_attach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct drm_device *dev = device_private(self);

	dev->driver = malloc(sizeof(struct drm_driver_info), DRM_MEM_DRIVER,
	    M_WAITOK | M_ZERO);

	sis_configure(dev);

	drm_attach(self, pa, sis_pciidlist);
}

CFATTACH_DECL_NEW(sisdrm, sizeof(struct drm_device),
    sisdrm_probe, sisdrm_attach, drm_detach, NULL);

#ifdef _MODULE

MODULE(MODULE_CLASS_DRIVER, sisdrm, NULL);

CFDRIVER_DECL(sisdrm, DV_DULL, NULL);
extern struct cfattach sisdrm_ca;
static int drmloc[] = { -1 };
static struct cfparent drmparent = {
	"drm", "vga", DVUNIT_ANY
};
static struct cfdata sisdrm_cfdata[] = {
	{
		.cf_name = "sisdrm",
		.cf_atname = "sisdrm",
		.cf_unit = 0,
		.cf_fstate = FSTATE_STAR,
		.cf_loc = drmloc,
		.cf_flags = 0,
		.cf_pspec = &drmparent,
	},
	{ NULL }
};

static int
sisdrm_modcmd(modcmd_t cmd, void *arg)
{
	int err;

	switch (cmd) {
	case MODULE_CMD_INIT:
		err = config_cfdriver_attach(&sisdrm_cd);
		if (err)
			return err;
		err = config_cfattach_attach("sisdrm", &sisdrm_ca);
		if (err) {
			config_cfdriver_detach(&sisdrm_cd);
			return err;
		}
		err = config_cfdata_attach(sisdrm_cfdata, 1);
		if (err) {
			config_cfattach_detach("sisdrm", &sisdrm_ca);
			config_cfdriver_detach(&sisdrm_cd);
			return err;
		}
		return 0;
	case MODULE_CMD_FINI:
		err = config_cfdata_detach(sisdrm_cfdata);
		if (err)
			return err;
		config_cfattach_detach("sisdrm", &sisdrm_ca);
		config_cfdriver_detach(&sisdrm_cd);
		return 0;
	default:
		return ENOTTY;
	}
}
#endif /* _MODULE */

#endif

/*	$NetBSD: i915_drv.c,v 1.8.8.1 2009/05/13 17:21:08 jym Exp $	*/

/* i915_drv.c -- ATI Radeon driver -*- linux-c -*-
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i915_drv.c,v 1.8.8.1 2009/05/13 17:21:08 jym Exp $");
/*
__FBSDID("$FreeBSD: src/sys/dev/drm/i915_drv.c,v 1.5 2006/05/17 06:36:28 anholt Exp $");
*/

#ifdef _MODULE
#include <sys/module.h>
#endif

#include "drmP.h"
#include "drm.h"
#include "i915_drm.h"
#include "i915_drv.h"
#include "drm_pciids.h"

/* drv_PCI_IDs comes from drm_pciids.h, generated from drm_pciids.txt. */
static drm_pci_id_list_t i915_pciidlist[] = {
	i915_PCI_IDS
};

static void i915_configure(drm_device_t *dev)
{
	dev->driver.buf_priv_size	= 1;	/* No dev_priv */
	dev->driver.load		= i915_driver_load;
	dev->driver.preclose		= i915_driver_preclose;
	dev->driver.lastclose		= i915_driver_lastclose;
	dev->driver.device_is_agp	= i915_driver_device_is_agp,
	dev->driver.vblank_wait		= i915_driver_vblank_wait;
	dev->driver.irq_preinstall	= i915_driver_irq_preinstall;
	dev->driver.irq_postinstall	= i915_driver_irq_postinstall;
	dev->driver.irq_uninstall	= i915_driver_irq_uninstall;
	dev->driver.irq_handler		= i915_driver_irq_handler;

	dev->driver.ioctls		= i915_ioctls;
	dev->driver.max_ioctl		= i915_max_ioctl;

	dev->driver.name		= DRIVER_NAME;
	dev->driver.desc		= DRIVER_DESC;
	dev->driver.date		= DRIVER_DATE;
	dev->driver.major		= DRIVER_MAJOR;
	dev->driver.minor		= DRIVER_MINOR;
	dev->driver.patchlevel		= DRIVER_PATCHLEVEL;

	dev->driver.use_agp		= 1;
	dev->driver.require_agp		= 1;
	dev->driver.use_mtrr		= 1;
	dev->driver.use_irq		= 1;
	dev->driver.use_vbl_irq		= 1;
}

#ifdef __FreeBSD__
static int
i915_probe(device_t dev)
{
	return drm_probe(dev, i915_pciidlist);
}

static int
i915_attach(device_t nbdev)
{
	drm_device_t *dev = device_get_softc(nbdev);

	memset(dev, 0, sizeof(drm_device_t));
	i915_configure(dev);
	return drm_attach(nbdev, i915_pciidlist);
}

static device_method_t i915_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,		i915_probe),
	DEVMETHOD(device_attach,	i915_attach),
	DEVMETHOD(device_detach,	drm_detach),

	{ 0, 0 }
};

static driver_t i915_driver = {
#if __FreeBSD_version >= 700010
	"drm",
#else
	"drmsub",
#endif
	i915_methods,
	sizeof(drm_device_t)
};

extern devclass_t drm_devclass;
#if __FreeBSD_version >= 700010
DRIVER_MODULE(i915, vgapci, i915_driver, drm_devclass, 0, 0);
#else
DRIVER_MODULE(i915, agp, i915_driver, drm_devclass, 0, 0);
#endif
MODULE_DEPEND(i915, drm, 1, 1, 1);

#elif defined(__NetBSD__) || defined(__OpenBSD__)

static int
i915drm_probe(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;
	return drm_probe(pa, i915_pciidlist);
}

static void
i915drm_attach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;
	drm_device_t *dev = device_private(self);

	i915_configure(dev);

	pmf_device_register(self, NULL, NULL);

	drm_attach(self, pa, i915_pciidlist);
}

CFATTACH_DECL_NEW(i915drm, sizeof(drm_device_t), i915drm_probe, i915drm_attach,
	drm_detach, drm_activate);

#ifdef _MODULE

MODULE(MODULE_CLASS_DRIVER, i915drm, "drm");

CFDRIVER_DECL(i915drm, DV_DULL, NULL);
extern struct cfattach i915drm_ca;
static int drmloc[] = { -1 };
static struct cfparent drmparent = {
	"drm", "vga", DVUNIT_ANY
};
static struct cfdata i915drm_cfdata[] = {
	{
		.cf_name = "i915drm",
		.cf_atname = "i915drm",
		.cf_unit = 0,
		.cf_fstate = FSTATE_STAR,
		.cf_loc = drmloc,
		.cf_flags = 0,
		.cf_pspec = &drmparent,
	},
	{ NULL }
};

static int
i915drm_modcmd(modcmd_t cmd, void *arg)
{
	int err;

	switch (cmd) {
	case MODULE_CMD_INIT:
		err = config_cfdriver_attach(&i915drm_cd);
		if (err)
			return err;
		err = config_cfattach_attach("i915drm", &i915drm_ca);
		if (err) {
			config_cfdriver_detach(&i915drm_cd);
			return err;
		}
		err = config_cfdata_attach(i915drm_cfdata, 1);
		if (err) {
			config_cfattach_detach("i915drm", &i915drm_ca);
			config_cfdriver_detach(&i915drm_cd);
			return err;
		}
		return 0;
	case MODULE_CMD_FINI:
		err = config_cfdata_detach(i915drm_cfdata);
		if (err)
			return err;
		config_cfattach_detach("i915drm", &i915drm_ca);
		config_cfdriver_detach(&i915drm_cd);
		return 0;
	default:
		return ENOTTY;
	}
}

#endif

#endif

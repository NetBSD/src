/*	$NetBSD: nouveau_module.c,v 1.8 2018/08/27 15:31:27 riastradh Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nouveau_module.c,v 1.8 2018/08/27 15:31:27 riastradh Exp $");

#include <sys/types.h>
#include <sys/module.h>
#include <sys/systm.h>

#include <drm/drmP.h>
#include <drm/drm_sysctl.h>

#include <core/device.h>

#ifdef _KERNEL_OPT
#include "opt_drmkms_pci.h"
#endif

MODULE(MODULE_CLASS_DRIVER, nouveau, "drmkms"); /* XXX drmkms_i2c, drmkms_ttm */

#ifdef _MODULE
#include "ioconf.c"
#endif

struct drm_sysctl_def nouveau_def = DRM_SYSCTL_INIT();

#if NDRMKMS_PCI > 0
extern struct drm_driver *const nouveau_drm_driver_stub; /* XXX */
extern struct drm_driver *const nouveau_drm_driver_pci;	 /* XXX */
#endif

static int
nouveau_init(void)
{
#if NDRMKMS_PCI > 0
	int error;

	*nouveau_drm_driver_pci = *nouveau_drm_driver_stub;
	nouveau_drm_driver_pci->set_busid = drm_pci_set_busid;
	nouveau_drm_driver_pci->request_irq = drm_pci_request_irq;
	nouveau_drm_driver_pci->free_irq = drm_pci_free_irq;

	error = drm_pci_init(nouveau_drm_driver_pci, NULL);
	if (error)
		return error;
#endif

	nvkm_devices_init();
	drm_sysctl_init(&nouveau_def);

	return 0;
}

static void
nouveau_fini(void)
{

	drm_sysctl_fini(&nouveau_def);
	nvkm_devices_fini();
#if NDRMKMS_PCI > 0
	drm_pci_exit(nouveau_drm_driver_pci, NULL);
#endif
}

static int
nouveau_modcmd(modcmd_t cmd, void *arg __unused)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = nouveau_init();
		if (error) {
			aprint_error("nouveau: failed to initialize: %d\n",
			    error);
			return error;
		}
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_nouveau,
		    cfattach_ioconf_nouveau, cfdata_ioconf_nouveau);
		if (error) {
			aprint_error("nouveau: failed to init component"
			    ": %d\n", error);
			nouveau_fini();
			return error;
		}
#endif
		return 0;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_nouveau,
		    cfattach_ioconf_nouveau, cfdata_ioconf_nouveau);
		if (error) {
			aprint_error("nouveau: failed to fini component"
			    ": %d\n", error);
			return error;
		}
#endif
		nouveau_fini();
		return 0;

	default:
		return ENOTTY;
	}
}

/*	$NetBSD: radeon_module.c,v 1.2.6.2 2014/08/20 00:04:22 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: radeon_module.c,v 1.2.6.2 2014/08/20 00:04:22 tls Exp $");

#include <sys/types.h>
#include <sys/module.h>
#ifndef _MODULE
#include <sys/once.h>
#endif
#include <sys/systm.h>

#include <drm/drmP.h>

#include "radeon_drv.h"

MODULE(MODULE_CLASS_DRIVER, radeon, "drmkms,drmkms_pci"); /* XXX drmkms_i2c, drmkms_ttm */

#ifdef _MODULE
#include "ioconf.c"
#endif

/* XXX Kludge to get these from radeon_drv.c.  */
extern struct drm_driver *const radeon_drm_driver;
extern int radeon_max_kms_ioctl;

static int
radeon_init(void)
{
	extern int drm_guarantee_initialized(void);
	int error;

	error = drm_guarantee_initialized();
	if (error)
		return error;

	radeon_drm_driver->num_ioctls = radeon_max_kms_ioctl;
	radeon_drm_driver->driver_features |= DRIVER_MODESET;

	error = drm_pci_init(radeon_drm_driver, NULL);
	if (error) {
		aprint_error("radeon: failed to init pci: %d\n",
		    error);
		return error;
	}

	return 0;
}

int	radeon_guarantee_initialized(void); /* XXX */
int
radeon_guarantee_initialized(void)
{
#ifdef _MODULE
	return 0;
#else
	static ONCE_DECL(radeon_init_once);

	return RUN_ONCE(&radeon_init_once, &radeon_init);
#endif
}

static void
radeon_fini(void)
{

	drm_pci_exit(radeon_drm_driver, NULL);
}

static int
radeon_modcmd(modcmd_t cmd, void *arg __unused)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		/* XXX Kludge it up...  Must happen before attachment.  */
#ifdef _MODULE
		error = radeon_init();
#else
		error = radeon_guarantee_initialized();
#endif
		if (error) {
			aprint_error("radeon: failed to initialize: %d\n",
			    error);
			return error;
		}
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_radeon,
		    cfattach_ioconf_radeon, cfdata_ioconf_radeon);
		if (error) {
			aprint_error("radeon: failed to init component"
			    ": %d\n", error);
			radeon_fini();
			return error;
		}
#endif
		return 0;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_radeon,
		    cfattach_ioconf_radeon, cfdata_ioconf_radeon);
		if (error) {
			aprint_error("radeon: failed to fini component"
			    ": %d\n", error);
			return error;
		}
#endif
		radeon_fini();
		return 0;

	default:
		return ENOTTY;
	}
}

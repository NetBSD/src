/*	$NetBSD: i915_module.c,v 1.3 2014/07/16 20:56:25 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: i915_module.c,v 1.3 2014/07/16 20:56:25 riastradh Exp $");

#include <sys/types.h>
#include <sys/module.h>
#ifndef _MODULE
#include <sys/once.h>
#endif
#include <sys/systm.h>

#include <drm/drmP.h>

#include "i915_drv.h"

MODULE(MODULE_CLASS_DRIVER, i915drmkms, "drmkms,drmkms_pci"); /* XXX drmkms_i2c */

#ifdef _MODULE
#include "ioconf.c"
#endif

/* XXX Kludge to get these from i915_drv.c.  */
extern struct drm_driver *const i915_drm_driver;
extern const struct pci_device_id *const i915_device_ids;
extern const size_t i915_n_device_ids;

static int
i915drmkms_init(void)
{
	extern int drm_guarantee_initialized(void);
	int error;

	error = drm_guarantee_initialized();
	if (error)
		return error;

	i915_drm_driver->num_ioctls = i915_max_ioctl;
	i915_drm_driver->driver_features |= DRIVER_MODESET;
	i915_drm_driver->driver_features &= ~DRIVER_USE_AGP;

	error = drm_pci_init(i915_drm_driver, NULL);
	if (error) {
		aprint_error("i915drmkms: failed to init pci: %d\n",
		    error);
		return error;
	}

	return 0;
}

int	i915drmkms_guarantee_initialized(void); /* XXX */
int
i915drmkms_guarantee_initialized(void)
{
#ifdef _MODULE
	return 0;
#else
	static ONCE_DECL(i915drmkms_init_once);

	return RUN_ONCE(&i915drmkms_init_once, &i915drmkms_init);
#endif
}

static void
i915drmkms_fini(void)
{

	drm_pci_exit(i915_drm_driver, NULL);
}

static int
i915drmkms_modcmd(modcmd_t cmd, void *arg __unused)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		/* XXX Kludge it up...  Must happen before attachment.  */
#ifdef _MODULE
		error = i915drmkms_init();
#else
		error = i915drmkms_guarantee_initialized();
#endif
		if (error) {
			aprint_error("i915drmkms: failed to initialize: %d\n",
			    error);
			return error;
		}
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_i915drmkms,
		    cfattach_ioconf_i915drmkms, cfdata_ioconf_i915drmkms);
		if (error) {
			aprint_error("i915drmkms: failed to init component"
			    ": %d\n", error);
			i915drmkms_fini();
			return error;
		}
#endif
		return 0;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_i915drmkms,
		    cfattach_ioconf_i915drmkms, cfdata_ioconf_i915drmkms);
		if (error) {
			aprint_error("i915drmkms: failed to fini component"
			    ": %d\n", error);
			return error;
		}
#endif
		i915drmkms_fini();
		return 0;

	default:
		return ENOTTY;
	}
}

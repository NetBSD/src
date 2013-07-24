/*	$NetBSD: i915_module.c,v 1.1.2.7 2013/07/24 03:57:33 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: i915_module.c,v 1.1.2.7 2013/07/24 03:57:33 riastradh Exp $");

#include <sys/types.h>
#include <sys/module.h>
#include <sys/systm.h>

#include <drm/drmP.h>

#include "i915_drv.h"

MODULE(MODULE_CLASS_DRIVER, i915drm2, "drm2"); /* XXX drm2pci, drm2edid */

#ifdef _MODULE
#include "ioconf.c"
#endif

/* XXX Kludge.  */
extern struct drm_driver *const i915_drm_driver;

static int
i915drm2_modcmd(modcmd_t cmd, void *arg __unused)
{
#ifdef _MODULE
	int error;
#endif

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		/* XXX Kludge it up...  Must happen before attachment.  */
		i915_drm_driver->num_ioctls = i915_max_ioctl;
		i915_drm_driver->driver_features |= DRIVER_MODESET;

		error = drm_pci_init(i915_drm_driver, NULL);
		if (error) {
			aprint_error("i915drm: failed to init pci: %d\n",
			    error);
			return error;
		}
		error = config_init_component(cfdriver_ioconf_i915drm,
		    cfattach_ioconf_i915drm, cfdata_ioconf_i915drm);
		if (error) {
			aprint_error("i915drm: failed to init component: %d\n",
			    error);
			drm_pci_exit(i915_drm_driver, NULL);
			return error;
		}
#endif
		return 0;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_i915drm,
		    cfattach_ioconf_i915drm, cfdata_ioconf_i915drm);
		if (error) {
			aprint_error("i915drm: failed to fini component: %d\n",
			    error);
			return error;
		}
		drm_pci_exit(i915_drm_driver, NULL);
#endif
		return 0;

	default:
		return ENOTTY;
	}
}

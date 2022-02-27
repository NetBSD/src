/*	$NetBSD: i915_module.c,v 1.18 2022/02/27 21:22:01 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: i915_module.c,v 1.18 2022/02/27 21:22:01 riastradh Exp $");

#include <sys/types.h>
#include <sys/module.h>
#ifndef _MODULE
#include <sys/once.h>
#endif
#include <sys/systm.h>

#include <drm/drm_device.h>
#include <drm/drm_sysctl.h>

#include "i915_drv.h"
#include "i915_globals.h"
#include "gt/intel_rps.h"

MODULE(MODULE_CLASS_DRIVER, i915drmkms, "acpivga,drmkms,drmkms_pci"); /* XXX drmkms_i2c */

#ifdef _MODULE
#include "ioconf.c"
#endif

struct drm_sysctl_def i915_def = DRM_SYSCTL_INIT();

/* XXX use link sets for DEFINE_SPINLOCK */
extern spinlock_t i915_sw_fence_lock;
extern spinlock_t *const i915_schedule_lock;

int i915_global_buddy_init(void); /* XXX */

static int
i915drmkms_init(void)
{
	int error;

	error = drm_guarantee_initialized();
	if (error)
		return error;

	/* XXX errno Linux->NetBSD */
	error = -i915_globals_init();
	if (error)
		return error;

	drm_sysctl_init(&i915_def);
	spin_lock_init(&mchdev_lock);
	spin_lock_init(&i915_sw_fence_lock);
	spin_lock_init(i915_schedule_lock);

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

	spin_lock_destroy(i915_schedule_lock);
	spin_lock_destroy(&i915_sw_fence_lock);
	spin_lock_destroy(&mchdev_lock);
	drm_sysctl_fini(&i915_def);

	i915_globals_exit();
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

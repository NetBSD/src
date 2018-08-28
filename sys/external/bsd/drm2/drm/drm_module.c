/*	$NetBSD: drm_module.c,v 1.15 2018/08/28 03:41:39 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_module.c,v 1.15 2018/08/28 03:41:39 riastradh Exp $");

#include <sys/types.h>
#include <sys/condvar.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/once.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <linux/mutex.h>

#include <drm/drmP.h>
#include <drm/drm_encoder_slave.h>
#include <drm/drm_internal.h>
#include <drm/drm_sysctl.h>

/*
 * XXX This is stupid.
 *
 * 1. Builtin modules are broken: they don't get initialized before
 *    autoconf matches devices, but we need the initialization to be
 *    run in order to match and attach drmkms drivers.
 *
 * 2. The following dependencies are _not_ correct:
 *    - drmkms can't depend on agp because not all drmkms drivers run
 *      on platforms guaranteed to have pci, let alone agp
 *    - drmkms_pci can't depend on agp because not all _pci_ has agp
 *      (e.g., tegra)
 *    - radeon (e.g.) can't depend on agp because not all radeon
 *      devices are on platforms guaranteed to have agp
 *
 * 3. We need to register the agp hooks before we try to attach a
 *    device.
 *
 * 4. The only mechanism we have to force this is the
 *    mumblefrotz_guarantee_initialized kludge.
 *
 * 5. We don't know if we even _can_ call
 *    drmkms_agp_guarantee_initialized unless we know NAGP.
 *
 * 6. We don't know NAGP unless we include "agp.h".
 *
 * 7. We can't include "agp.h" if the platform has agp.
 *
 * 8. The way we determine whether we have agp is NAGP.
 *
 * 9. @!*#&^@&*@!&^#@
 */
#if defined(__powerpc__) || defined(__i386__) || defined(__x86_64__)
#include "agp.h"
#endif

/*
 * XXX I2C stuff should be moved to a separate drmkms_i2c module.
 */
MODULE(MODULE_CLASS_DRIVER, drmkms, "drmkms_linux");

struct mutex	drm_global_mutex;

struct drm_sysctl_def drm_def = DRM_SYSCTL_INIT();

static int
drm_init(void)
{
	extern int linux_guarantee_initialized(void);
	int error;

	error = linux_guarantee_initialized();
	if (error)
		return error;

	drm_agp_hooks_init();
#if NAGP > 0
	extern int drmkms_agp_guarantee_initialized(void);
	error = drmkms_agp_guarantee_initialized();
	if (error) {
		drm_agp_hooks_fini();
		return error;
	}
#endif

	if (ISSET(boothowto, AB_DEBUG))
		drm_debug = ~(unsigned int)0;

	spin_lock_init(&drm_minor_lock);
	idr_init(&drm_minors_idr);
	linux_mutex_init(&drm_global_mutex);
	drm_connector_ida_init();
	drm_global_init();
	drm_sysctl_init(&drm_def);
	drm_i2c_encoders_init();

	return 0;
}

int
drm_guarantee_initialized(void)
{
#ifdef _MODULE
	return 0;
#else
	static ONCE_DECL(drm_init_once);

	return RUN_ONCE(&drm_init_once, &drm_init);
#endif
}

static void
drm_fini(void)
{

	drm_i2c_encoders_fini();
	drm_sysctl_fini(&drm_def);
	drm_global_release();
	drm_connector_ida_destroy();
	linux_mutex_destroy(&drm_global_mutex);
	idr_destroy(&drm_minors_idr);
	spin_lock_destroy(&drm_minor_lock);
	drm_agp_hooks_fini();
}

int
drm_irq_by_busid(struct drm_device *dev, void *data, struct drm_file *file)
{

	return -ENODEV;
}

static int
drmkms_modcmd(modcmd_t cmd, void *arg __unused)
{
#ifdef _MODULE
	devmajor_t bmajor = NODEVMAJOR, cmajor = NODEVMAJOR;
#endif
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = drm_init();
#else
		error = drm_guarantee_initialized();
#endif
		if (error)
			return error;
#ifdef _MODULE
		error = devsw_attach("drm", NULL, &bmajor,
		    &drm_cdevsw, &cmajor);
		if (error) {
			aprint_error("drmkms: unable to attach devsw: %d\n",
			    error);
			return error;
		}
#endif
		return 0;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = devsw_detach(NULL, &drm_cdevsw);
		if (error)
			return error;
#endif
		drm_fini();
		return 0;

	default:
		return ENOTTY;
	}
}

/*	$NetBSD: vmwgfx_module.c,v 1.1 2022/02/17 01:21:03 riastradh Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: vmwgfx_module.c,v 1.1 2022/02/17 01:21:03 riastradh Exp $");

#include <sys/types.h>
#include <sys/module.h>
#ifndef _MODULE
#include <sys/once.h>
#endif
#include <sys/systm.h>

#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <drm/drm_sysctl.h>

#include "vmwgfx_drv.h"

MODULE(MODULE_CLASS_DRIVER, vmwgfx, "drmkms,drmkms_pci"); /* XXX drmkms_i2c, drmkms_ttm */

#ifdef _MODULE
#include "ioconf.c"
#endif

struct drm_sysctl_def vmwgfx_def = DRM_SYSCTL_INIT();

static int
vmwgfx_init(void)
{
	int error;

	error = drm_guarantee_initialized();
	if (error)
		return error;

	drm_sysctl_init(&vmwgfx_def);

	return 0;
}

int	vmwgfx_guarantee_initialized(void); /* XXX */
int
vmwgfx_guarantee_initialized(void)
{
#ifdef _MODULE
	return 0;
#else
	static ONCE_DECL(vmwgfx_init_once);

	return RUN_ONCE(&vmwgfx_init_once, &vmwgfx_init);
#endif
}

static void
vmwgfx_fini(void)
{

	drm_sysctl_fini(&vmwgfx_def);
}

static int
vmwgfx_modcmd(modcmd_t cmd, void *arg __unused)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		/* XXX Kludge it up...  Must happen before attachment.  */
#ifdef _MODULE
		error = vmwgfx_init();
#else
		error = vmwgfx_guarantee_initialized();
#endif
		if (error) {
			aprint_error("vmwgfx: failed to initialize: %d\n",
			    error);
			return error;
		}
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_vmwgfx,
		    cfattach_ioconf_vmwgfx, cfdata_ioconf_vmwgfx);
		if (error) {
			aprint_error("vmwgfx: failed to init component"
			    ": %d\n", error);
			vmwgfx_fini();
			return error;
		}
#endif
		return 0;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_vmwgfx,
		    cfattach_ioconf_vmwgfx, cfdata_ioconf_vmwgfx);
		if (error) {
			aprint_error("vmwgfx: failed to fini component"
			    ": %d\n", error);
			return error;
		}
#endif
		vmwgfx_fini();
		return 0;

	default:
		return ENOTTY;
	}
}

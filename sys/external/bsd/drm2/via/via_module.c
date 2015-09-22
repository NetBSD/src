/*	$NetBSD: via_module.c,v 1.1.2.3 2015/09/22 12:06:06 skrll Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: via_module.c,v 1.1.2.3 2015/09/22 12:06:06 skrll Exp $");

#include <sys/types.h>
#include <sys/module.h>
#include <sys/once.h>

#include <drm/drmP.h>
#include <drm/via_drm.h>

#include "via_drv.h"

MODULE(MODULE_CLASS_DRIVER, viadrmums, "drmkms,drmkms_pci");

#ifdef _MODULE
#include "ioconf.c"
#endif

extern struct drm_driver *const via_drm_driver; /* XXX */

static int
viadrm_init(void)
{
	extern int drm_guarantee_initialized(void);
	int error;

	via_init_command_verifier(); /* idempotent, no unwind needed */

	error = drm_guarantee_initialized();
	if (error)
		return error;

	error = drm_pci_init(via_drm_driver, NULL);
	if (error) {
		aprint_error("viadrmkms: failed to init pci: %d\n",
		    error);
		return error;
	}

	return 0;
}

int	viadrm_guarantee_initialized(void); /* XXX */
int
viadrm_guarantee_initialized(void)
{
#ifdef _MODULE
	return 0;
#else
	static ONCE_DECL(viadrm_init_once);

	return RUN_ONCE(&viadrm_init_once, &viadrm_init);
#endif
}

static void
viadrm_fini(void)
{

	drm_pci_exit(via_drm_driver, NULL);
}

static int
viadrmums_modcmd(modcmd_t cmd, void *arg __unused)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = viadrm_init();
#else
		error = viadrm_guarantee_initialized();
#endif
		if (error) {
			aprint_error("viadrmums: failed to initialize: %d\n",
			    error);
			return error;
		}
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_viadrmums,
		    cfattach_ioconf_viadrmums, cfdata_ioconf_viadrmums);
		if (error) {
			aprint_error("viadrmums: failed to init component"
			    ": %d\n", error);
			viadrm_fini();
			return error;
		}
#endif
		return 0;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_viadrmums,
		    cfattach_ioconf_viadrmums, cfdata_ioconf_viadrmums);
		if (error) {
			aprint_error("viadrmums: failed to fini component"
			    ": %d\n", error);
			return error;
		}
#endif
		viadrm_fini();
		return 0;

	default:
		return ENOTTY;
	}
}

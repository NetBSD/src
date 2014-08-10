/*	$NetBSD: nouveau_module.c,v 1.1.2.2 2014/08/10 06:55:40 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: nouveau_module.c,v 1.1.2.2 2014/08/10 06:55:40 tls Exp $");

#include <sys/types.h>
#include <sys/module.h>
#ifndef _MODULE
#include <sys/once.h>
#endif
#include <sys/systm.h>

#include <drm/drmP.h>

#include <core/object.h>

MODULE(MODULE_CLASS_DRIVER, nouveau, "drmkms,drmkms_pci"); /* XXX drmkms_i2c, drmkms_ttm */

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
nouveau_init(void)
{
	extern int drm_guarantee_initialized(void);
	int error;

	error = drm_guarantee_initialized();
	if (error)
		return error;

	error = drm_pci_init(nouveau_drm_driver, NULL);
	if (error) {
		aprint_error("nouveau: failed to init pci: %d\n", error);
		return error;
	}

	nouveau_objects_init();
#if 0				/* XXX nouveau acpi */
	nouveau_register_dsm_handler();
#endif
}

int	nouveau_guarantee_initialized(void); /* XXX */
int
nouveau_guarantee_initialized(void)
{
#ifdef _MODULE
	return 0;
#else
	static ONCE_DECL(nouveau_init_once);

	return RUN_ONCE(&nouveau_init_once, &nouveau_init);
#endif
}

static void
nouveau_fini(void)
{

#if 0				/* XXX nouveau acpi */
	nouveau_unregister_dsm_handler();
#endif
	nouveau_objects_fini();
	drm_pci_exit(nouveau_drm_driver, NULL);
}

static int
nouveau_modcmd(modcmd_t cmd, void *arg __unused)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = nouveau_init();
#else
		error = nouveau_guarantee_initialized();
#endif
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

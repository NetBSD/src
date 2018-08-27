/*	$NetBSD: drm_pci_module.c,v 1.6 2018/08/27 15:31:27 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_pci_module.c,v 1.6 2018/08/27 15:31:27 riastradh Exp $");

#include <sys/module.h>
#include <sys/once.h>

#include <drm/drmP.h>
#include <drm/drm_internal.h>

MODULE(MODULE_CLASS_MISC, drmkms_pci, "drmkms,pci");

#ifdef CONFIG_AGP
const struct drm_agp_hooks drmkms_pci_agp_hooks = {
	.agph_acquire_ioctl = &drm_agp_acquire_ioctl,
	.agph_release_ioctl = &drm_agp_release_ioctl,
	.agph_enable_ioctl = &drm_agp_enable_ioctl,
	.agph_info_ioctl = &drm_agp_info_ioctl,
	.agph_alloc_ioctl = &drm_agp_alloc_ioctl,
	.agph_free_ioctl = &drm_agp_free_ioctl,
	.agph_bind_ioctl = &drm_agp_bind_ioctl,
	.agph_unbind_ioctl = &drm_agp_unbind_ioctl,
	.agph_release = &drm_agp_release,
	.agph_clear = &drm_agp_clear,
};
#endif

static int (*drm_pci_set_unique_save)(struct drm_device *, struct drm_master *,
    struct drm_unique *);

static int
drmkms_pci_agp_init(void)
{
#ifdef CONFIG_AGP
	int error;

	error = drm_agp_register(&drmkms_pci_agp_hooks);
	if (error)
		return error;
#endif

	drm_pci_set_unique_save = drm_pci_set_unique_impl;
	drm_pci_set_unique_hook(&drm_pci_set_unique_save);

	return 0;
}

int
drmkms_pci_agp_guarantee_initialized(void)
{
#ifdef _MODULE
	return 0;
#else
	static ONCE_DECL(drmkms_pci_agp_init_once);

	return RUN_ONCE(&drmkms_pci_agp_init_once, &drmkms_pci_agp_init);
#endif
}

static void
drmkms_pci_agp_fini(void)
{

	drm_pci_set_unique_hook(&drm_pci_set_unique_save);
	KASSERT(drm_pci_set_unique_save == drm_pci_set_unique_impl);

#ifdef CONFIG_AGP
	drm_agp_deregister(&drmkms_pci_agp_hooks);
#endif
}

static int
drmkms_pci_modcmd(modcmd_t cmd, void *arg __unused)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = drmkms_pci_agp_init();
#else
		error = drmkms_pci_agp_guarantee_initialized();
#endif
		if (error)
			return error;
		return 0;

	case MODULE_CMD_FINI:
		drmkms_pci_agp_fini();
		return 0;

	default:
		return ENOTTY;
	}
}

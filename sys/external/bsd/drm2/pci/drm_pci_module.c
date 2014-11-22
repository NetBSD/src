/*	$NetBSD: drm_pci_module.c,v 1.3 2014/11/22 19:18:07 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_pci_module.c,v 1.3 2014/11/22 19:18:07 riastradh Exp $");

#include <sys/module.h>

#include <drm/drmP.h>

MODULE(MODULE_CLASS_MISC, drmkms_pci, "drmkms,pci");

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

static int
drmkms_pci_modcmd(modcmd_t cmd, void *arg __unused)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = drm_agp_register(&drmkms_pci_agp_hooks);
		if (error)
			return error;
		return 0;

	case MODULE_CMD_FINI:
		drm_agp_deregister(&drmkms_pci_agp_hooks);
		return 0;

	default:
		return ENOTTY;
	}
}

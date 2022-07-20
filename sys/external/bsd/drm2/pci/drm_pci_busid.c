/*	$NetBSD: drm_pci_busid.c,v 1.2 2022/07/20 01:20:20 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: drm_pci_busid.c,v 1.2 2022/07/20 01:20:20 riastradh Exp $");

#include <sys/types.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <lib/libkern/libkern.h>

#include <dev/pci/pcivar.h>

#include <linux/kernel.h>
#include <linux/pci.h>

#include <drm/drm_auth.h>
#include <drm/drm_device.h>
#include "../dist/drm/drm_internal.h"

int
drm_pci_set_busid(struct drm_device *dev, struct drm_master *master)
{
	const struct pci_attach_args *const pa = &dev->pdev->pd_pa;

	KASSERT(dev_is_pci(dev->dev));

	master->unique = kasprintf(GFP_KERNEL, "pci:%04x:%02x:%02x.%d",
	    pci_domain_nr(dev->pdev->bus),
	    pa->pa_bus, pa->pa_device, pa->pa_function);
	if (master->unique == NULL)
		return -ENOMEM;
	master->unique_len = strlen(master->unique);

	return 0;
}

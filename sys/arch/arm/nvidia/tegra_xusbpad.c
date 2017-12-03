/* $NetBSD: tegra_xusbpad.c,v 1.6.2.2 2017/12/03 11:35:54 jdolecek Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_xusbpad.c,v 1.6.2.2 2017/12/03 11:35:54 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

#include <arm/nvidia/tegra_xusbpad.h>

static const struct tegra_xusbpad_ops *xusbpad_ops = NULL;
static device_t xusbpad_device = NULL;

void
tegra_xusbpad_register(device_t dev, const struct tegra_xusbpad_ops *ops)
{
	KASSERT(dev != NULL && ops != NULL);
	KASSERT(xusbpad_ops == NULL);
	KASSERT(xusbpad_device == NULL);

	xusbpad_ops = ops;
	xusbpad_device = dev;
}

void
tegra_xusbpad_sata_enable(void)
{
	if (xusbpad_ops == NULL || xusbpad_ops->sata_enable == NULL) {
		aprint_error("%s: could not enable SATA\n", __func__);
		return;
	}
	xusbpad_ops->sata_enable(xusbpad_device);
}

void
tegra_xusbpad_xhci_enable(void)
{
	if (xusbpad_ops == NULL || xusbpad_ops->xhci_enable == NULL) {
		aprint_error("%s: could not enable XHCI\n", __func__);
		return;
	}
	xusbpad_ops->xhci_enable(xusbpad_device);
}

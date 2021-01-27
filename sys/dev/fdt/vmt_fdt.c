/*	$NetBSD: vmt_fdt.c,v 1.5 2021/01/27 03:10:21 thorpej Exp $ */

/*
 * Copyright (c) 2020 Ryo Shimizu <ryo@nerv.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vmt_fdt.c,v 1.5 2021/01/27 03:10:21 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>
#include <dev/vmt/vmtreg.h>
#include <dev/vmt/vmtvar.h>

static int vmt_fdt_match(device_t, cfdata_t, void *);
static void vmt_fdt_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(vmt_fdt, sizeof(struct vmt_softc),
    vmt_fdt_match, vmt_fdt_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "vmware" },
	DEVICE_COMPAT_EOL
};

static int
vmt_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct fdt_attach_args * const faa = aux;

	if (OF_finddevice("/hypervisor") != faa->faa_phandle)
		return 0;
	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
vmt_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct vmt_softc * const sc = device_private(self);

	aprint_naive("\n");
	aprint_normal(": VMware Tools driver\n");

	sc->sc_dev = self;
	vmt_common_attach(sc);
}

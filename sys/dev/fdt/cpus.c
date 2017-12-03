/* $NetBSD: cpus.c,v 1.2.10.2 2017/12/03 11:37:01 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: cpus.c,v 1.2.10.2 2017/12/03 11:37:01 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>

static int	cpus_match(device_t, cfdata_t, void *);
static void	cpus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpus, 0, cpus_match, cpus_attach, NULL, NULL);

static int
cpus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return OF_finddevice("/cpus") == faa->faa_phandle;
}

static void
cpus_attach(device_t parent, device_t self, void *aux)
{
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	int child;

	aprint_naive("\n");
	aprint_normal("\n");

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		if (!fdtbus_status_okay(child))
			continue;

		struct fdt_attach_args cfaa = *faa;
		cfaa.faa_phandle = child;
		cfaa.faa_name = "cpu";
		cfaa.faa_quiet = 0;

		device_t cpu = config_found(self, &cfaa, fdtbus_print);
		if (cpu)
			config_found(cpu, &cfaa, NULL);
	}
}

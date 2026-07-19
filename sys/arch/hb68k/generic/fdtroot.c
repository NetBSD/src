/*	$NetBSD: fdtroot.c,v 1.1 2026/07/19 01:48:20 thorpej Exp $	*/

/*-
 * Copyright (c) 2017 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: fdtroot.c,v 1.1 2026/07/19 01:48:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <dev/ofw/openfirm.h>

static int
fdtroot_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
fdtroot_attach(device_t parent, device_t self, void *aux)
{
	struct fdt_attach_args faa;

	aprint_naive("\n");
	aprint_normal("\n");

	machine_init_attach_args(&faa);
	faa.faa_name = "";
	faa.faa_phandle = OF_peer(0);

	config_found(self, &faa, NULL, CFARGS_NONE);
}

CFATTACH_DECL_NEW(fdtroot, 0,
    fdtroot_match, fdtroot_attach, NULL, NULL);

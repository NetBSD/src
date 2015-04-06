/* $NetBSD: xen_ucode.c,v 1.4.8.1 2015/04/06 15:18:04 skrll Exp $ */
/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christoph Egger.
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
__KERNEL_RCSID(0, "$NetBSD: xen_ucode.c,v 1.4.8.1 2015/04/06 15:18:04 skrll Exp $");

#include "opt_cpu_ucode.h"
#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/cpuio.h>
#include <sys/cpu.h>

#include <dev/firmload.h>

#include <machine/cpuvar.h>
#include <machine/cputypes.h>

#include <x86/cpu_ucode.h>

static struct cpu_ucode_softc ucode_softc;

int
cpu_ucode_get_version(struct cpu_ucode_version *data)
{

	switch (cpu_vendor) {
	case CPUVENDOR_AMD:
		return cpu_ucode_amd_get_version(data);
	case CPUVENDOR_INTEL:
		return cpu_ucode_intel_get_version(data);
	default:
		return EOPNOTSUPP;
	}

	return 0;
}

#ifdef COMPAT_60
int
compat6_cpu_ucode_get_version(struct compat6_cpu_ucode *data)
{

	switch (cpu_vendor) {
	case CPUVENDOR_AMD:
		return compat6_cpu_ucode_amd_get_version(data);
	default:
		return EOPNOTSUPP;
	}

	return 0;
}
#endif /* COMPAT_60 */

int
cpu_ucode_md_open(firmware_handle_t *fwh, int loader_version, const char *fwname)
{
	switch (cpu_vendor) {
	case CPUVENDOR_AMD:
		return cpu_ucode_amd_firmware_open(fwh, fwname);
	case CPUVENDOR_INTEL:
		return cpu_ucode_intel_firmware_open(fwh, fwname);
	default:
		return EOPNOTSUPP;
	}
}

int
cpu_ucode_apply(const struct cpu_ucode *data)
{
	struct cpu_ucode_softc *sc = &ucode_softc;
	struct xen_platform_op op;
	int error;

	/* Xen updates all??? */
	if (data->cpu_nr != CPU_UCODE_ALL_CPUS)
		return EOPNOTSUPP;

	sc->loader_version = data->loader_version;
	error = cpu_ucode_load(sc, data->fwname);
	if (error)
		return error;

	op.cmd = XENPF_microcode_update;
	set_xen_guest_handle(op.u.microcode.data, sc->sc_blob);
	op.u.microcode.length = sc->sc_blobsize;

	error = -HYPERVISOR_platform_op(&op);

	if (sc->sc_blob)
		firmware_free(sc->sc_blob, sc->sc_blobsize);
	sc->sc_blob = NULL;
	sc->sc_blobsize = 0;
	return error;
}

#ifdef COMPAT_60
int
compat6_cpu_ucode_apply(const struct compat6_cpu_ucode *data)
{
	struct cpu_ucode_softc *sc = &ucode_softc;
	struct xen_platform_op op;
	int error;

	sc->loader_version = CPU_UCODE_LOADER_AMD;
	error = cpu_ucode_load(sc, data->fwname);
	if (error)
		return error;

	op.cmd = XENPF_microcode_update;
	set_xen_guest_handle(op.u.microcode.data, sc->sc_blob);
	op.u.microcode.length = sc->sc_blobsize;

	error = -HYPERVISOR_platform_op(&op);

	if (sc->sc_blob != NULL)
		firmware_free(sc->sc_blob, sc->sc_blobsize);
	sc->sc_blob = NULL;
	sc->sc_blobsize = 0;
	return error;
}
#endif /* COMPAT_60 */

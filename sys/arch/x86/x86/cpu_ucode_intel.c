/* $NetBSD: cpu_ucode_intel.c,v 1.5 2014/03/26 08:04:19 christos Exp $ */
/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthias Drochner.
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
__KERNEL_RCSID(0, "$NetBSD: cpu_ucode_intel.c,v 1.5 2014/03/26 08:04:19 christos Exp $");

#include "opt_xen.h"
#include "opt_cpu_ucode.h"
#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/cpuio.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/xcall.h>

#include <machine/cpufunc.h>
#include <machine/specialreg.h>
#include <x86/cpu_ucode.h>

#define MSR_IA32_PLATFORM_ID 0x17
#define MSR_IA32_BIOS_UPDT_TRIGGER 0x79
#define MSR_IA32_BIOS_SIGN_ID 0x8b

static void
intel_getcurrentucode(uint32_t *ucodeversion, int *platformid)
{
	unsigned int unneeded_ids[4];
	uint64_t msr;

	kpreempt_disable();

	wrmsr(MSR_IA32_BIOS_SIGN_ID, 0);
	x86_cpuid(0, unneeded_ids);
	msr = rdmsr(MSR_IA32_BIOS_SIGN_ID);
	*ucodeversion = msr >> 32;

	kpreempt_enable();

	msr = rdmsr(MSR_IA32_PLATFORM_ID);
	*platformid = ((int)(msr >> 50)) & 7;
}

int
cpu_ucode_intel_get_version(struct cpu_ucode_version *ucode)
{
	struct cpu_info *ci = curcpu();
	struct cpu_ucode_version_intel1 data;

	if (ucode->loader_version != CPU_UCODE_LOADER_INTEL1 ||
	    CPUID_TO_FAMILY(ci->ci_signature) < 6)
		return EOPNOTSUPP;
	if (!ucode->data)
		return 0;

	intel_getcurrentucode(&data.ucodeversion, &data.platformid);

	return copyout(&data, ucode->data, sizeof(data));
}

int
cpu_ucode_intel_firmware_open(firmware_handle_t *fwh, const char *fwname)
{
	const char *fw_path = "cpu_x86_intel1";
	uint32_t ucodeversion, cpu_signature;
	int platformid;
	char cpuspec[11];

	if (fwname != NULL && fwname[0] != '\0')
		return firmware_open(fw_path, fwname, fwh);

	cpu_signature = curcpu()->ci_signature;
	if (CPUID_TO_FAMILY(cpu_signature) < 6)
		return EOPNOTSUPP;

	intel_getcurrentucode(&ucodeversion, &platformid);
	snprintf(cpuspec, sizeof(cpuspec), "%08x-%d", cpu_signature,
	    platformid);

	return firmware_open(fw_path, cpuspec, fwh);
}

#ifndef XEN
int
cpu_ucode_intel_apply(struct cpu_ucode_softc *sc, int cpuno)
{
	uint32_t ucodetarget, oucodeversion, nucodeversion;
	int platformid;
	struct intel1_ucode_header *uh;

	if (sc->loader_version != CPU_UCODE_LOADER_INTEL1
	    || cpuno != CPU_UCODE_CURRENT_CPU)
		return EINVAL;

	/* XXX relies on malloc alignment */
	if ((uintptr_t)(sc->sc_blob) & 15) {
		printf("ucode alignment bad\n");
		return EINVAL;
	}

	uh = (struct intel1_ucode_header *)(sc->sc_blob);
	if (uh->uh_header_ver != 1 || uh->uh_loader_rev != 1)
		return EINVAL;
	ucodetarget = uh->uh_rev;

	kpreempt_disable();

	intel_getcurrentucode(&oucodeversion, &platformid);
	if (oucodeversion >= ucodetarget) {
		kpreempt_enable();
		return EEXIST; /* ??? */
	}
	wrmsr(MSR_IA32_BIOS_UPDT_TRIGGER, (uintptr_t)(sc->sc_blob) + 48);
	intel_getcurrentucode(&nucodeversion, &platformid);

	kpreempt_enable();

	if (nucodeversion != ucodetarget)
		return EIO;

	printf("cpu %d: ucode 0x%x->0x%x\n", curcpu()->ci_index,
	       oucodeversion, nucodeversion);

	return 0;
}
#endif /* ! XEN */

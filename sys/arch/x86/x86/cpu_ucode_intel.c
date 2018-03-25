/* $NetBSD: cpu_ucode_intel.c,v 1.12.8.3 2018/03/25 08:49:12 pgoyette Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: cpu_ucode_intel.c,v 1.12.8.3 2018/03/25 08:49:12 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_xen.h"
#include "opt_cpu_ucode.h"
#endif

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/cpuio.h>
#include <sys/cpu.h>
#include <sys/kmem.h>

#include <machine/cpufunc.h>
#include <machine/specialreg.h>
#include <x86/cpu_ucode.h>

static void
intel_getcurrentucode(uint32_t *ucodeversion, int *platformid)
{
	unsigned int unneeded_ids[4];
	uint64_t msr;

	kpreempt_disable();

	wrmsr(MSR_BIOS_SIGN, 0);
	x86_cpuid(0, unneeded_ids);
	msr = rdmsr(MSR_BIOS_SIGN);
	*ucodeversion = msr >> 32;

	kpreempt_enable();

	msr = rdmsr(MSR_IA32_PLATFORM_ID);
	*platformid = ((int)(msr >> 50)) & 7;
}

int
cpu_ucode_intel_get_version(struct cpu_ucode_version *ucode,
    void *ptr, size_t len)
{
	struct cpu_info *ci = curcpu();
	struct cpu_ucode_version_intel1 *data = ptr;

	if (ucode->loader_version != CPU_UCODE_LOADER_INTEL1 ||
	    CPUID_TO_FAMILY(ci->ci_signature) < 6)
		return EOPNOTSUPP;

	if (len < sizeof(*data))
		return ENOSPC;

	intel_getcurrentucode(&data->ucodeversion, &data->platformid);
	return 0;
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
	int platformid, cpuid;
	struct intel1_ucode_header *uh;
	void *uha;
	size_t newbufsize = 0;
	int rv = 0;

	if (sc->loader_version != CPU_UCODE_LOADER_INTEL1
	    || cpuno != CPU_UCODE_CURRENT_CPU)
		return EINVAL;

	uh = (struct intel1_ucode_header *)(sc->sc_blob);
	if (uh->uh_header_ver != 1 || uh->uh_loader_rev != 1)
		return EINVAL;
	ucodetarget = uh->uh_rev;

	if ((uintptr_t)(sc->sc_blob) & 15) {
		/* Make the buffer 16 byte aligned */
		newbufsize = sc->sc_blobsize + 15;
		uha = kmem_alloc(newbufsize, KM_SLEEP);
		uh = (struct intel1_ucode_header *)roundup2((uintptr_t)uha, 16);
		/* Copy to the new area */
		memcpy(uh, sc->sc_blob, sc->sc_blobsize);
	}

	kpreempt_disable();

	intel_getcurrentucode(&oucodeversion, &platformid);
	if (oucodeversion >= ucodetarget) {
		kpreempt_enable();
		rv = EEXIST; /* ??? */
		goto out;
	}
	wrmsr(MSR_BIOS_UPDT_TRIG, (uintptr_t)uh + 48);
	intel_getcurrentucode(&nucodeversion, &platformid);
	cpuid = curcpu()->ci_index;

	kpreempt_enable();

	if (nucodeversion != ucodetarget) {
		rv = EIO;
		goto out;
	}

	printf("cpu %d: ucode 0x%x->0x%x\n", cpuid,
	       oucodeversion, nucodeversion);
out:
	if (newbufsize != 0)
		kmem_free(uha, newbufsize);
	return rv;
}
#endif /* ! XEN */

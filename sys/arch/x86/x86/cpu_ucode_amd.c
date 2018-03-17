/* $NetBSD: cpu_ucode_amd.c,v 1.8 2018/03/17 15:56:32 christos Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: cpu_ucode_amd.c,v 1.8 2018/03/17 15:56:32 christos Exp $");

#include "opt_xen.h"
#include "opt_cpu_ucode.h"

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/cpuio.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/xcall.h>

#include <machine/cpufunc.h>
#include <machine/specialreg.h>
#include <x86/cpu_ucode.h>

struct microcode_amd_header {
	uint32_t ah_data_code;
	uint32_t ah_patch_id;
	uint8_t ah_patch_data_id[2];
	uint8_t ah_patch_data_len;
	uint8_t ah_init_flag;
	uint32_t ah_patch_data_checksum;
	uint32_t ah_nb_dev_id;
	uint32_t ah_sb_dev_id;
	uint16_t ah_processor_rev_id;
	uint8_t ah_nb_rev_id;
	uint8_t ah_sb_rev_id;
	uint8_t ah_bios_api_rev;
	uint8_t ah_reserved[3];
	uint32_t ah_match_reg[8];
} __packed;

/* equivalence cpu table */
struct microcode_amd_equiv_cpu_table {
	uint32_t ect_installed_cpu;
	uint32_t ect_fixed_errata_mask;
	uint32_t ect_fixed_errata_compare;
	uint16_t ect_equiv_cpu;
	uint16_t ect_reserved;
};

#define UCODE_MAGIC	0x00414d44

struct microcode_amd {
	uint8_t *mpb; /* microcode patch block */
	size_t mpb_size;
	struct microcode_amd_equiv_cpu_table *ect;
	size_t ect_size;
};

struct mpbhdr {
#define UCODE_TYPE_EQUIV	0
#define UCODE_TYPE_PATCH	1
	uint32_t mpb_type;
	uint32_t mpb_len;
	uint32_t mpb_data[];
};

static uint32_t
amd_cpufamily(void)
{
	uint32_t family;
	struct cpu_info *ci = curcpu();

	family = CPUID_TO_FAMILY(ci->ci_signature);

	return family;
}

int
cpu_ucode_amd_get_version(struct cpu_ucode_version *ucode, void *ptr,
    size_t len)
{
	struct cpu_ucode_version_amd *data = ptr;

	if (ucode->loader_version != CPU_UCODE_LOADER_AMD
	    || amd_cpufamily() < 0x10)
		return EOPNOTSUPP;

	if (len < sizeof(*data))
		return ENOSPC;

	data->version = rdmsr(MSR_UCODE_AMD_PATCHLEVEL);
	return 0;
}

int
cpu_ucode_amd_firmware_open(firmware_handle_t *fwh, const char *fwname)
{
	const char *fw_path = "x86/amd";
	char _fwname[32];
	int error;

	if (fwname != NULL && fwname[0] != '\0')
		return firmware_open(fw_path, fwname, fwh);

	snprintf(_fwname, sizeof(_fwname), "microcode_amd_fam%xh.bin",
	    amd_cpufamily());

	error = firmware_open(fw_path, _fwname, fwh);
	if (error == 0)
		return 0;

	return firmware_open(fw_path, "microcode_amd.bin", fwh);
}

#ifndef XEN
struct mc_buf {
	uint8_t *mc_buf;
	uint32_t mc_equiv_cpuid;
	struct mpbhdr *mc_mpbuf;
	struct microcode_amd *mc_amd;
	int mc_error;
};

static void
cpu_apply_cb(void *arg0, void *arg1)
{
	int error = 0;
	const struct cpu_ucode_softc *sc = arg0;
	struct microcode_amd mc_amd;
	struct mc_buf mc;
	device_t dev;
	int s;

	memcpy(&mc, arg1, sizeof(mc));
	mc_amd.mpb = mc.mc_amd->mpb;
	mc_amd.mpb_size = mc.mc_amd->mpb_size;

	dev = curcpu()->ci_dev;
	s = splhigh();

	do {
		uint64_t patchlevel;
		struct microcode_amd_header *hdr;

		if (mc.mc_mpbuf->mpb_type != UCODE_TYPE_PATCH) {
			aprint_debug_dev(dev, "ucode: patch type expected\n");
			goto next;
		}

		hdr = (struct microcode_amd_header *)mc_amd.mpb;
		if (hdr->ah_processor_rev_id != mc.mc_equiv_cpuid) {
			aprint_debug_dev(dev, "ucode: patch does not "
			    "match this cpu "
			    "(patch is for cpu id %x, cpu id is %x)\n",
			    hdr->ah_processor_rev_id, mc.mc_equiv_cpuid);
			goto next;
		}

		patchlevel = rdmsr(MSR_UCODE_AMD_PATCHLEVEL);
		if (hdr->ah_patch_id <= patchlevel)
			goto next;

		/* found matching microcode update */
		wrmsr(MSR_UCODE_AMD_PATCHLOADER, (u_long)hdr);

		/* check current patch id and patch's id for match */
		if (patchlevel == rdmsr(MSR_UCODE_AMD_PATCHLEVEL)) {
			aprint_debug_dev(dev, "ucode: update from revision "
			    "0x%"PRIx64" to 0x%x failed\n",
			    patchlevel, hdr->ah_patch_id);
			error = EIO;
			goto out;
		} else {
			/* Success */
			error = 0;
			goto out;
		}

next:
		/* Check for race:
		 * When we booted with -x a cpu might already finished
		 * (why doesn't xc_wait() wait for *all* cpus?)
		 * and sc->sc_blob is already freed.
		 * In this case the calculation below touches
		 * non-allocated memory and results in a crash.
		 */
		if (sc->sc_blob == NULL)
			break;

		mc.mc_buf += mc.mc_mpbuf->mpb_len +
		    sizeof(mc.mc_mpbuf->mpb_type) +
		    sizeof(mc.mc_mpbuf->mpb_len);
		mc.mc_mpbuf = (struct mpbhdr *)mc.mc_buf;
		mc_amd.mpb = (uint8_t *)mc.mc_mpbuf->mpb_data;
		mc_amd.mpb_size = mc.mc_mpbuf->mpb_len;

	} while ((uintptr_t)((mc.mc_buf) - (uint8_t *)sc->sc_blob) < sc->sc_blobsize);
	aprint_error_dev(dev, "ucode: No newer patch available "
	    "for this cpu than is already installed.\n");
	error = ENOENT;

out:
	if (error)
		((struct mc_buf *)(arg1))->mc_error = error;
	splx(s);
}

int
cpu_ucode_amd_apply(struct cpu_ucode_softc *sc, int cpuno)
{
	int i, error = 0;
	uint32_t *magic;
	uint32_t cpu_signature;
	uint32_t equiv_cpuid = 0;
	struct mc_buf mc;
	int where;

	if (sc->loader_version != CPU_UCODE_LOADER_AMD
	    || cpuno != CPU_UCODE_ALL_CPUS)
		return EINVAL;

	cpu_signature = curcpu()->ci_signature;

	KASSERT(sc->sc_blob != NULL);
	magic = (uint32_t *)sc->sc_blob;
	if (*magic != UCODE_MAGIC) {
		aprint_error("ucode: wrong file magic\n");
		return EINVAL;
	}

	mc.mc_buf = &sc->sc_blob[sizeof(*magic)];
	mc.mc_mpbuf = (struct mpbhdr *)mc.mc_buf;

	/* equivalence table is expected to come first */
	if (mc.mc_mpbuf->mpb_type != UCODE_TYPE_EQUIV) {
		aprint_error("ucode: missing equivalence table\n");
		return EINVAL;
	}

	mc.mc_amd = kmem_zalloc(sizeof(*mc.mc_amd), KM_NOSLEEP);
	if (mc.mc_amd == NULL)
		return ENOMEM;

	mc.mc_amd->ect = kmem_alloc(mc.mc_mpbuf->mpb_len, KM_NOSLEEP);
	if (mc.mc_amd->ect == NULL) {
		error = ENOMEM;
		goto err0;
	}

	memcpy(mc.mc_amd->ect, mc.mc_mpbuf->mpb_data, mc.mc_mpbuf->mpb_len);
	mc.mc_amd->ect_size = mc.mc_mpbuf->mpb_len;

	/* check if there is a patch for this cpu present */
	for (i = 0; mc.mc_amd->ect[i].ect_installed_cpu != 0; i++) {
		if (cpu_signature == mc.mc_amd->ect[i].ect_installed_cpu) {
			/* keep extended family and extended model */
			equiv_cpuid = mc.mc_amd->ect[i].ect_equiv_cpu & 0xffff;
			break;
		}
	}
	if (equiv_cpuid == 0) {
		aprint_error("ucode: No patch available for this cpu\n");
		error = ENOENT;
		goto err1;
	}

	mc.mc_equiv_cpuid = equiv_cpuid;
	mc.mc_buf += mc.mc_mpbuf->mpb_len + sizeof(mc.mc_mpbuf->mpb_type) +
	    sizeof(mc.mc_mpbuf->mpb_len);
	mc.mc_mpbuf = (struct mpbhdr *)mc.mc_buf;
	mc.mc_amd->mpb = (uint8_t *)mc.mc_mpbuf->mpb_data;
	mc.mc_amd->mpb_size = mc.mc_mpbuf->mpb_len;

	/* Apply it on all cpus */
	mc.mc_error = 0;
	where = xc_broadcast(0, cpu_apply_cb, sc, &mc);

	/* Wait for completion */
	xc_wait(where);
	error = mc.mc_error;

err1:
	kmem_free(mc.mc_amd->ect, mc.mc_amd->ect_size);
err0:
	kmem_free(mc.mc_amd, sizeof(*mc.mc_amd));
	return error;
}
#endif /* ! XEN */

/*	$NetBSD: fdt_bootstrap.c,v 1.1 2026/08/05 01:32:56 thorpej Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: fdt_bootstrap.c,v 1.1 2026/08/05 01:32:56 thorpej Exp $");

#include "opt_hb68k_config.h"
#include "opt_m68k_arch.h"

#include <sys/types.h>
#include <sys/cpu.h>

#include <m68k/seglist.h>

#include <uvm/uvm_extern.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_memory.h>
#include <dev/fdt/fdt_platform.h>

#include <libfdt.h>

#include <dev/ofw/openfirm.h>

struct cpu_mmu_data {
	int	cpu_type;
	int	mmu_type;
};

#if defined(M68010)
static const struct cpu_mmu_data cpu_data10 = { CPU_68010, MMU_CUSTOM };
#endif

#if defined(M68020)
static const struct cpu_mmu_data cpu_data20 = { CPU_68020, MMU_UNKNOWN };
#endif

#if defined(M68030)
static const struct cpu_mmu_data cpu_data030 = { CPU_68030, MMU_68030 };
#endif

#if defined(M68040)
static const struct cpu_mmu_data cpu_data040 = { CPU_68040, MMU_68040 };
#endif

#if defined(M68060)
static const struct cpu_mmu_data cpu_data060 = { CPU_68060, MMU_68040 };
#endif

static const struct device_compatible_entry cpu_compat_data[] = {
#if defined(M68010)
	{ .compat = "motorola,68010",	.data = &cpu_data010 },
#endif
#if defined(M68020)
	{ .compat = "motorola,68020",	.data = &cpu_data020 },
#endif
#if defined(M68030)
	{ .compat = "motorola,68030",	.data = &cpu_data030 },
#endif
#if defined(M68040)
	{ .compat = "motorola,68040",	.data = &cpu_data040 },
	{ .compat = "motorola,680lc40",	.data = &cpu_data040 },
#endif
#if defined(M68060)
	{ .compat = "motorola,68060",	.data = &cpu_data060 },
	{ .compat = "motorola,680lc60",	.data = &cpu_data060 },
#endif
	DEVICE_COMPAT_EOL
};

static const struct device_compatible_entry mmu_compat_data[] = {
#if defined(M68K_MMU_68851)
	{ .compat = "motorola,68851",	.value = MMU_68851 },
#endif
	DEVICE_COMPAT_EOL
};

struct add_memory_physsegs_context {
	phys_seg_list_t *segs;
	unsigned int nsegs;
	unsigned int npgs;
};

static void
add_memory_physsegs(const struct fdt_memory *m, void *v)
{
	struct add_memory_physsegs_context *ctx = v;
	paddr_t first = m68k_round_page((paddr_t)m->start);
	paddr_t last = m68k_trunc_page((paddr_t)m->end);

	if (ctx->nsegs == VM_PHYSSEG_MAX) {
		return;
	}

	if (ctx->nsegs > 0 &&
	    ctx->segs[ctx->nsegs - 1].ps_end == first) {
		ctx->segs[ctx->nsegs - 1].ps_end =
		    ctx->segs[ctx->nsegs - 1].ps_avail_end = last;
	} else {
		ctx->segs[ctx->nsegs].ps_start =
		    ctx->segs[ctx->nsegs].ps_avail_start = first;
		ctx->segs[ctx->nsegs].ps_end =
		    ctx->segs[ctx->nsegs].ps_avail_end = last;
		ctx->nsegs++;
	}

	ctx->npgs += m68k_btop(last - first);
}

/*
 * fdt_bootstrap1 --
 *	Very early bootstrap routine; we extract some vital information
 *	out of the Device Tree necessary for pmap_bootstrap1().
 *
 *	N.B. Because we have to make lots of calls into other parts
 *	of the kernel, this whole thing really only works if reloff == 0,
 *	i.e. the kernel is going to run VA==PA or if the MMU is
 *	already enabled and we're already running at our linked
 *	address.
 */
paddr_t
fdt_bootstrap1(paddr_t nextpa, paddr_t reloff)
{
	const struct fdt_platform *plat;
	const struct device_compatible_entry *dce;
	int phandle;

	if (reloff != 0) {
		panic("%s: no hope...", __func__);
	}

#if defined(CONFIG_STATIC_DEVICE_TREE)
	extern uint8_t dt_blob_start[];
	if (! machine_fdt_init(dt_blob_start)) {
		panic("%s: bad static device tree", __func__);
	}
#else
	/*
	 * XXX Assume that the device tree immediately follows
	 * the static kernel image.
	 */
	void *fdt_data = (void *)nextpa;
	if (! machine_fdt_init(fdt_data)) {
		panic("%s: don't know where the device tree is", __func__);
	}
	/* nextpa will be updated below */
#endif /* CONFIG_STATIC_DEVICE_TREE */

	plat = machine_platform();
	if (plat == NULL) {
		panic("%s: kernel is not configured for this platform",
		    __func__);
	}

	/* Determine the CPU / MMU type. */
	phandle = OF_finddevice("/cpus/cpu@0");
	if (phandle == -1) {
		panic("%s: unable to find CPU node", __func__);
	}
	dce = of_compatible_lookup(phandle, cpu_compat_data);
	if (dce == NULL) {
		panic("%s: kernel is not configured for this CPU", __func__);
	}
	const struct cpu_mmu_data *cmd = dce->data;
	cputype = cmd->cpu_type;
	mmutype = cmd->mmu_type;

	if (mmutype == MMU_UNKNOWN) {
		char mmuprop[32];

		if (OF_getprop(phandle, "mmu-type",
			       mmuprop, sizeof(mmuprop)) >= 0) {
			dce = device_compatible_lookup_strlist(mmuprop,
			    strlen(mmuprop) + 1, mmu_compat_data);
			if (dce != NULL) {
				mmutype = dce->value;
			}
		}
		/*
		 * If it's still unknown at this point, the platform
		 * bootstrap routine will have a chance to weigh in.
		 */
	}

#if !defined(CONFIG_STATIC_DEVICE_TREE)
	/*
	 * We're done with any potential changes to the device tree.
	 * Compact it and advance past it.
	 */
	fdt_pack(fdt_data);
	nextpa = (nextpa + fdt_totalsize(fdt_data) + 3) & ~3U;
#endif

	/*
	 * Platform-specific bootstrap.  Any additional MMU configuration
	 * information, and a chance to patch up any /memory nodes, if
	 * necessary.
	 */
	if (plat->fp_bootstrap != NULL) {
		nextpa = (*plat->fp_bootstrap)(nextpa, reloff);
	}

	/* Fatal if we still don't know the MMU type. */
	if (mmutype == MMU_UNKNOWN) {
		panic("%s: don't know the MMU type", __func__);
	}

	uint64_t memory_start, memory_end;
	fdt_memory_get(&memory_start, &memory_end);
	fdt_memory_remove_reserved(memory_start, memory_end);

	struct add_memory_physsegs_context ctx = {
		.segs = phys_seg_list,
	};
	fdt_memory_foreach(add_memory_physsegs, &ctx);
	physmem = ctx.npgs;

	return nextpa;
}

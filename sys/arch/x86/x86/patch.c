/*	$NetBSD: patch.c,v 1.50 2022/04/09 12:07:00 riastradh Exp $	*/

/*-
 * Copyright (c) 2007, 2008, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*
 * Patch kernel code at boot time, depending on available CPU features.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: patch.c,v 1.50 2022/04/09 12:07:00 riastradh Exp $");

#include "opt_lockdebug.h"
#ifdef i386
#include "opt_spldebug.h"
#endif

#include <sys/types.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/specialreg.h>
#include <machine/frameasm.h>

#include <uvm/uvm.h>
#include <machine/pmap.h>

#include <x86/cpuvar.h>
#include <x86/cputypes.h>

__link_set_decl(x86_hotpatch_descriptors, struct x86_hotpatch_descriptor);

struct x86_hotpatch_destination {
	uint8_t name;
	uint8_t size;
	void *addr;
} __packed;

/* -------------------------------------------------------------------------- */

/* CLAC instruction, part of SMAP. */
extern uint8_t hp_clac, hp_clac_end;
static const struct x86_hotpatch_source hp_clac_source = {
	.saddr = &hp_clac,
	.eaddr = &hp_clac_end
};
static const struct x86_hotpatch_descriptor hp_clac_desc = {
	.name = HP_NAME_CLAC,
	.nsrc = 1,
	.srcs = { &hp_clac_source }
};
__link_set_add_rodata(x86_hotpatch_descriptors, hp_clac_desc);

/* STAC instruction, part of SMAP. */
extern uint8_t hp_stac, hp_stac_end;
static const struct x86_hotpatch_source hp_stac_source = {
	.saddr = &hp_stac,
	.eaddr = &hp_stac_end
};
static const struct x86_hotpatch_descriptor hp_stac_desc = {
	.name = HP_NAME_STAC,
	.nsrc = 1,
	.srcs = { &hp_stac_source }
};
__link_set_add_rodata(x86_hotpatch_descriptors, hp_stac_desc);

/* Errata on certain AMD CPUs. */
extern uint8_t hp_retfence, hp_retfence_end;
static const struct x86_hotpatch_source hp_retfence_source = {
	.saddr = &hp_retfence,
	.eaddr = &hp_retfence_end
};
static const struct x86_hotpatch_descriptor hp_retfence_desc = {
	.name = HP_NAME_RETFENCE,
	.nsrc = 1,
	.srcs = { &hp_retfence_source }
};
__link_set_add_rodata(x86_hotpatch_descriptors, hp_retfence_desc);

/* No lock when on a single processor. */
extern uint8_t hp_nolock, hp_nolock_end;
static const struct x86_hotpatch_source hp_nolock_source = {
	.saddr = &hp_nolock,
	.eaddr = &hp_nolock_end
};
static const struct x86_hotpatch_descriptor hp_nolock_desc = {
	.name = HP_NAME_NOLOCK,
	.nsrc = 1,
	.srcs = { &hp_nolock_source }
};
__link_set_add_rodata(x86_hotpatch_descriptors, hp_nolock_desc);

/* Use MFENCE if available, part of SSE2. */
extern uint8_t sse2_mfence, sse2_mfence_end;
static const struct x86_hotpatch_source hp_sse2_mfence_source = {
	.saddr = &sse2_mfence,
	.eaddr = &sse2_mfence_end
};
static const struct x86_hotpatch_descriptor hp_sse2_mfence_desc = {
	.name = HP_NAME_SSE2_MFENCE,
	.nsrc = 1,
	.srcs = { &hp_sse2_mfence_source }
};
__link_set_add_rodata(x86_hotpatch_descriptors, hp_sse2_mfence_desc);

#ifdef i386
/* CAS_64. */
extern uint8_t _atomic_cas_cx8, _atomic_cas_cx8_end;
static const struct x86_hotpatch_source hp_cas_cx8_source = {
	.saddr = &_atomic_cas_cx8,
	.eaddr = &_atomic_cas_cx8_end
};
static const struct x86_hotpatch_descriptor hp_cas_cx8_desc = {
	.name = HP_NAME_CAS_64,
	.nsrc = 1,
	.srcs = { &hp_cas_cx8_source }
};
__link_set_add_rodata(x86_hotpatch_descriptors, hp_cas_cx8_desc);

/* SPLLOWER. */
extern uint8_t cx8_spllower, cx8_spllower_end;
static const struct x86_hotpatch_source hp_cx8_spllower_source = {
	.saddr = &cx8_spllower,
	.eaddr = &cx8_spllower_end
};
static const struct x86_hotpatch_descriptor hp_cx8_spllower_desc = {
	.name = HP_NAME_SPLLOWER,
	.nsrc = 1,
	.srcs = { &hp_cx8_spllower_source }
};
__link_set_add_rodata(x86_hotpatch_descriptors, hp_cx8_spllower_desc);

/* MUTEX_EXIT. */
#ifndef LOCKDEBUG
extern uint8_t i686_mutex_spin_exit, i686_mutex_spin_exit_end;
static const struct x86_hotpatch_source hp_i686_mutex_spin_exit_source = {
	.saddr = &i686_mutex_spin_exit,
	.eaddr = &i686_mutex_spin_exit_end
};
static const struct x86_hotpatch_descriptor hp_i686_mutex_spin_exit_desc = {
	.name = HP_NAME_MUTEX_EXIT,
	.nsrc = 1,
	.srcs = { &hp_i686_mutex_spin_exit_source }
};
__link_set_add_rodata(x86_hotpatch_descriptors, hp_i686_mutex_spin_exit_desc);
#endif
#endif

/* -------------------------------------------------------------------------- */

static inline void __unused
patchbytes(void *addr, const uint8_t *bytes, size_t size)
{
	uint8_t *ptr = (uint8_t *)addr;
	size_t i;

	for (i = 0; i < size; i++) {
		ptr[i] = bytes[i];
	}
}

/*
 * Rules: each pointer accessed in this function MUST be read-only.
 *
 * Called from ASM only, prototype not public.
 */
int x86_hotpatch_apply(uint8_t, uint8_t);
int
__noubsan /* the local variables have unknown alignment to UBSan */
x86_hotpatch_apply(uint8_t name, uint8_t sel)
{
	struct x86_hotpatch_descriptor * const *iter;
	const struct x86_hotpatch_descriptor *desc;
	const struct x86_hotpatch_source *src;
	const struct x86_hotpatch_destination *hps, *hpe, *hp;
	extern char __rodata_hotpatch_start;
	extern char __rodata_hotpatch_end;
	const uint8_t *bytes;
	bool found = false;
	size_t size;

	/*
	 * Find the descriptor, and perform some sanity checks.
	 */
	__link_set_foreach(iter, x86_hotpatch_descriptors) {
		desc = *iter;
		if (desc->name == name) {
			found = true;
			break;
		}
	}
	if (!found)
		return -1;
	if (desc->nsrc > 2)
		return -1;
	if (sel >= desc->nsrc)
		return -1;

	/*
	 * Get the hotpatch source.
	 */
	src = desc->srcs[sel];
	bytes = src->saddr;
	size = (size_t)src->eaddr - (size_t)src->saddr;

	/*
	 * Apply the hotpatch on each registered destination.
	 */
	hps = (struct x86_hotpatch_destination *)&__rodata_hotpatch_start;
	hpe = (struct x86_hotpatch_destination *)&__rodata_hotpatch_end;
	for (hp = hps; hp < hpe; hp++) {
		if (hp->name != name) {
			continue;
		}
		if (hp->size != size) {
			return -1;
		}
		patchbytes(hp->addr, bytes, size);
	}

	return 0;
}

#ifdef __x86_64__
/*
 * The CPU added the D bit on the text pages while we were writing to them.
 * Remove that bit. Kinda annoying, but we can't avoid it.
 */
static void
remove_d_bit(void)
{
	extern struct bootspace bootspace;
	pt_entry_t pte;
	vaddr_t va;
	size_t i, n;

	for (i = 0; i < BTSPACE_NSEGS; i++) {
		if (bootspace.segs[i].type != BTSEG_TEXT)
			continue;
		va = bootspace.segs[i].va;
		n = 0;
		while (n < bootspace.segs[i].sz) {
			if (L2_BASE[pl2_i(va)] & PTE_PS) {
				pte = L2_BASE[pl2_i(va)] & ~PTE_D;
				pmap_pte_set(&L2_BASE[pl2_i(va)], pte);
				n += NBPD_L2;
				va += NBPD_L2;
			} else {
				pte = L1_BASE[pl1_i(va)] & ~PTE_D;
				pmap_pte_set(&L1_BASE[pl1_i(va)], pte);
				n += NBPD_L1;
				va += NBPD_L1;
			}
		}
	}

	tlbflushg();
}
#else
#define remove_d_bit()	__nothing
#endif

/*
 * Interrupts disabled here. Called from ASM only, prototype not public.
 */
void x86_hotpatch_cleanup(int);
void
x86_hotpatch_cleanup(int retval)
{
	if (retval != 0) {
		panic("x86_hotpatch_apply failed");
	}

	remove_d_bit();
}

/* -------------------------------------------------------------------------- */

void
x86_patch(bool early)
{
	static bool first, second;

	if (early) {
		if (first)
			return;
		first = true;
	} else {
		if (second)
			return;
		second = true;
	}

	if (!early && ncpu == 1) {
#ifndef LOCKDEBUG
		/*
		 * Uniprocessor: kill LOCK prefixes.
		 */
		x86_hotpatch(HP_NAME_NOLOCK, 0);
#endif
	}

	if (!early && (cpu_feature[0] & CPUID_SSE2) != 0) {
		/*
		 * Faster memory barriers.  The only barrier x86 ever
		 * requires for MI synchronization between CPUs is
		 * MFENCE for store-before-load ordering; all other
		 * ordering is guaranteed already -- every load is a
		 * load-acquire and every store is a store-release.
		 *
		 * LFENCE and SFENCE are relevant only for MD logic
		 * involving I/O devices or non-temporal stores.
		 */
		x86_hotpatch(HP_NAME_SSE2_MFENCE, 0);
	}

#ifdef i386
	/*
	 * Patch early and late.  Second time around the 'lock' prefix
	 * may be gone.
	 */
	if ((cpu_feature[0] & CPUID_CX8) != 0) {
		x86_hotpatch(HP_NAME_CAS_64, 0);
	}

#if !defined(SPLDEBUG)
	if (!early && (cpu_feature[0] & CPUID_CX8) != 0) {
		/* Faster splx(), mutex_spin_exit(). */
		x86_hotpatch(HP_NAME_SPLLOWER, 0);
#if !defined(LOCKDEBUG)
		x86_hotpatch(HP_NAME_MUTEX_EXIT, 0);
#endif
	}
#endif /* !SPLDEBUG */
#endif	/* i386 */

	/*
	 * On some Opteron revisions, locked operations erroneously
	 * allow memory references to be `bled' outside of critical
	 * sections.  Apply workaround.
	 */
	if (cpu_vendor == CPUVENDOR_AMD &&
	    (CPUID_TO_FAMILY(cpu_info_primary.ci_signature) == 0xe ||
	    (CPUID_TO_FAMILY(cpu_info_primary.ci_signature) == 0xf &&
	    CPUID_TO_EXTMODEL(cpu_info_primary.ci_signature) < 0x4))) {
		x86_hotpatch(HP_NAME_RETFENCE, 0);
	}

	/*
	 * If SMAP is present then patch the prepared holes with clac/stac
	 * instructions.
	 */
	if (!early && cpu_feature[5] & CPUID_SEF_SMAP) {
		KASSERT(rcr4() & CR4_SMAP);

		x86_hotpatch(HP_NAME_CLAC, 0);
		x86_hotpatch(HP_NAME_STAC, 0);
	}
}

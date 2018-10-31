/*	$NetBSD: asan.h,v 1.1 2018/10/31 06:26:26 maxv Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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

#include <sys/ksyms.h>

#include <amd64/pmap.h>
#include <amd64/vmparam.h>

#ifdef __HAVE_PCPU_AREA
#error "PCPU area not allowed with KASAN"
#endif
#ifdef __HAVE_DIRECT_MAP
#error "DMAP not allowed with KASAN"
#endif

#define __MD_VIRTUAL_SHIFT	47	/* 48bit address space, cut half */
#define __MD_CANONICAL_BASE	0xFFFF800000000000

#define __MD_SHADOW_SIZE	(1ULL << (__MD_VIRTUAL_SHIFT - KASAN_SHADOW_SCALE_SHIFT))
#define KASAN_MD_SHADOW_START	(VA_SIGN_NEG((L4_SLOT_KASAN * NBPD_L4)))
#define KASAN_MD_SHADOW_END	(KASAN_MD_SHADOW_START + __MD_SHADOW_SIZE)

static bool __md_early __read_mostly = true;
static uint8_t __md_earlypages[8 * PAGE_SIZE] __aligned(PAGE_SIZE);
static size_t __md_earlytaken = 0;

static inline int8_t *
kasan_md_addr_to_shad(const void *addr)
{
	vaddr_t va = (vaddr_t)addr;
	return (int8_t *)(KASAN_MD_SHADOW_START +
	    ((va - __MD_CANONICAL_BASE) >> KASAN_SHADOW_SCALE_SHIFT));
}

static inline bool
kasan_md_unsupported(vaddr_t addr)
{
	return (addr >= (vaddr_t)PTE_BASE &&
	    addr < ((vaddr_t)PTE_BASE + NBPD_L4));
}

static paddr_t
__md_early_palloc(void)
{
	paddr_t ret;

	KASSERT(__md_earlytaken < 8);

	ret = (paddr_t)(&__md_earlypages[0] + __md_earlytaken * PAGE_SIZE);
	__md_earlytaken++;

	ret -= KERNBASE;

	return ret;
}

static paddr_t
__md_palloc(void)
{
	paddr_t pa;

	if (__predict_false(__md_early))
		pa = __md_early_palloc();
	else
		pa = pmap_get_physpage();

	return pa;
}

static void
kasan_md_shadow_map_page(vaddr_t va)
{
	paddr_t pa;

	if (!pmap_valid_entry(L4_BASE[pl4_i(va)])) {
		pa = __md_palloc();
		L4_BASE[pl4_i(va)] = pa | PG_KW | pmap_pg_nx | PG_V;
	}
	if (!pmap_valid_entry(L3_BASE[pl3_i(va)])) {
		pa = __md_palloc();
		L3_BASE[pl3_i(va)] = pa | PG_KW | pmap_pg_nx | PG_V;
	}
	if (!pmap_valid_entry(L2_BASE[pl2_i(va)])) {
		pa = __md_palloc();
		L2_BASE[pl2_i(va)] = pa | PG_KW | pmap_pg_nx | PG_V;
	}
	if (!pmap_valid_entry(L1_BASE[pl1_i(va)])) {
		pa = __md_palloc();
		L1_BASE[pl1_i(va)] = pa | PG_KW | pmap_pg_g | pmap_pg_nx | PG_V;
	}
}

/*
 * Map only the current stack. We will map the rest in kasan_init.
 */
static void
kasan_md_early_init(void *stack)
{
	kasan_shadow_map(stack, USPACE);
	__md_early = false;
}

/*
 * Create the shadow mapping. We don't create the 'User' area, because we
 * exclude it from the monitoring. The 'Main' area is created dynamically
 * in pmap_growkernel.
 */
static void
kasan_md_init(void)
{
	extern struct bootspace bootspace;
	size_t i;

	CTASSERT((__MD_SHADOW_SIZE / NBPD_L4) == NL4_SLOT_KASAN);

	/* Kernel. */
	for (i = 0; i < BTSPACE_NSEGS; i++) {
		if (bootspace.segs[i].type == BTSEG_NONE) {
			continue;
		}
		kasan_shadow_map((void *)bootspace.segs[i].va,
		    bootspace.segs[i].sz);
	}

	/* Boot region. */
	kasan_shadow_map((void *)bootspace.boot.va, bootspace.boot.sz);

	/* Module map. */
	kasan_shadow_map((void *)bootspace.smodule,
	    (size_t)(bootspace.emodule - bootspace.smodule));

	/* The bootstrap spare va. */
	kasan_shadow_map((void *)bootspace.spareva, PAGE_SIZE);
}

static inline bool
__md_unwind_end(const char *name)
{
	if (!strcmp(name, "syscall") ||
	    !strcmp(name, "handle_syscall") ||
	    !strncmp(name, "Xintr", 5) ||
	    !strncmp(name, "Xhandle", 7) ||
	    !strncmp(name, "Xresume", 7) ||
	    !strncmp(name, "Xstray", 6) ||
	    !strncmp(name, "Xhold", 5) ||
	    !strncmp(name, "Xrecurse", 8) ||
	    !strcmp(name, "Xdoreti") ||
	    !strncmp(name, "Xsoft", 5)) {
		return true;
	}

	return false;
}

static void
kasan_md_unwind(void)
{
	uint64_t *rbp, rip;
	const char *mod;
	const char *sym;
	size_t nsym;
	int error;

	rbp = (uint64_t *)__builtin_frame_address(0);
	nsym = 0;

	while (1) {
		/* 8(%rbp) contains the saved %rip. */
		rip = *(rbp + 1);

		if (rip < KERNBASE) {
			break;
		}
		error = ksyms_getname(&mod, &sym, (vaddr_t)rip, KSYMS_PROC);
		if (error) {
			break;
		}
		printf("#%zu %p in %s <%s>\n", nsym, (void *)rip, sym, mod);
		if (__md_unwind_end(sym)) {
			break;
		}

		rbp = (uint64_t *)*(rbp);
		if (rbp == 0) {
			break;
		}
		nsym++;

		if (nsym >= 15) {
			break;
		}
	}
}

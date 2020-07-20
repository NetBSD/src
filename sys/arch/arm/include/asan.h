/*	$NetBSD: asan.h,v 1.3 2020/07/19 11:47:48 skrll Exp $	*/

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson.
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

#include <sys/atomic.h>
#include <sys/ksyms.h>

#include <arm/vmparam.h>
#include <arm/arm32/machdep.h>
#include <arm/arm32/pmap.h>

#define KASAN_MD_SHADOW_START	VM_KERNEL_KASAN_BASE
#define KASAN_MD_SHADOW_END	VM_KERNEL_KASAN_END
#define __MD_KERNMEM_BASE	KERNEL_BASE

static inline int8_t *
kasan_md_addr_to_shad(const void *addr)
{
	vaddr_t va = (vaddr_t)addr;
	return (int8_t *)(KASAN_MD_SHADOW_START +
	    ((va - __MD_KERNMEM_BASE) >> KASAN_SHADOW_SCALE_SHIFT));
}

static inline bool
kasan_md_unsupported(vaddr_t addr)
{
	return addr < VM_MIN_KERNEL_ADDRESS ||
	    addr >= KASAN_MD_SHADOW_START;
}

/* -------------------------------------------------------------------------- */

/*
 * Early mapping, used to map just the stack at boot time. We rely on the fact
 * that VA = PA + KERNEL_BASE.
 */

#define KASAN_NEARLYPAGES	3

static bool __md_early __read_mostly;
static size_t __md_nearlypages __attribute__((__section__(".data")));
static uint8_t __md_earlypages[KASAN_NEARLYPAGES * PAGE_SIZE]
    __aligned(PAGE_SIZE)  __attribute__((__section__(".data")));

static vaddr_t
__md_palloc(void)
{
	paddr_t pa;

	if (__predict_false(__md_early)) {
		KASSERTMSG(__md_nearlypages < KASAN_NEARLYPAGES,
		    "__md_nearlypages %zu", __md_nearlypages);

		vaddr_t va = (vaddr_t)(&__md_earlypages[0] + __md_nearlypages * PAGE_SIZE);
		__md_nearlypages++;
		__builtin_memset((void *)va, 0, PAGE_SIZE);

		return KERN_VTOPHYS(va);
	}

	if (!uvm.page_init_done) {
		if (uvm_page_physget(&pa) == false)
			panic("KASAN can't get a page");

		return pa;
	}

	struct vm_page *pg;
retry:
	pg = uvm_pagealloc(NULL, 0, NULL, 0);
	if (pg == NULL) {
		uvm_wait(__func__);
		goto retry;
	}
	pa = VM_PAGE_TO_PHYS(pg);

	return pa;
}

static void
kasan_md_shadow_map_page(vaddr_t va)
{
	const uint32_t mask = L1_TABLE_SIZE - 1;
	const paddr_t ttb = (paddr_t)(armreg_ttbr1_read() & ~mask);
	pd_entry_t * const pdep = (pd_entry_t *)KERN_PHYSTOV(ttb);

	const size_t l1slot = l1pte_index(va);
	vaddr_t l2ptva;

	KASSERT((va & PAGE_MASK) == 0);
	KASSERT(__md_early || l1pte_page_p(pdep[l1slot]));

	if (!l1pte_page_p(pdep[l1slot])) {
		KASSERT(__md_early);
		const paddr_t l2ptpa = __md_palloc();
		const vaddr_t segl2va = va & -L2_S_SEGSIZE;
		const size_t segl1slot = l1pte_index(segl2va);

		const pd_entry_t npde =
		    L1_C_PROTO | l2ptpa | L1_C_DOM(PMAP_DOMAIN_KERNEL);

		l1pte_set(pdep + segl1slot, npde);
		PDE_SYNC_RANGE(pdep, PAGE_SIZE / L2_T_SIZE);

		l2ptva = KERN_PHYSTOV(l1pte_pa(pdep[l1slot]));
	} else {
		/*
		 * The shadow map area L2PTs were allocated and mapped
		 * by arm32_kernel_vm_init.  Use the array of pv_addr_t
		 * to get the l2ptva.
		 */
		extern pv_addr_t kasan_l2pt[];
		const size_t off = va - KASAN_MD_SHADOW_START;
		const size_t segoff = off & (L2_S_SEGSIZE - 1);
		const size_t idx = off / L2_S_SEGSIZE;
		const vaddr_t segl2ptva = kasan_l2pt[idx].pv_va;
		l2ptva = segl2ptva + l1pte_index(segoff) * L2_TABLE_SIZE_REAL;
	}

	pt_entry_t * l2pt = (pt_entry_t *)l2ptva;
	pt_entry_t * const ptep = &l2pt[l2pte_index(va)];

	if (!l2pte_valid_p(*ptep)) {
		const int prot = VM_PROT_READ | VM_PROT_WRITE;
		const paddr_t pa = __md_palloc();
		pt_entry_t npte =
		    L2_S_PROTO |
		    pa |
		    pte_l2_s_cache_mode_pt |
		    L2_S_PROT(PTE_KERNEL, prot);

		l2pte_set(ptep, npte, 0);
		PTE_SYNC(ptep);
		__builtin_memset((void *)va, 0, PAGE_SIZE);
	}
}

/*
 * Map the init stacks of the BP and APs. We will map the rest in kasan_init.
 */
#define INIT_ARM_STACK_SHIFT	10
#define INIT_ARM_STACK_SIZE	(1 << INIT_ARM_STACK_SHIFT)

static void
kasan_md_early_init(void *stack)
{

	__md_early = true;
	__md_nearlypages = 0;
	kasan_shadow_map(stack, INIT_ARM_STACK_SIZE * MAXCPUS);
	__md_early = false;
}

static void
kasan_md_init(void)
{
	extern vaddr_t kasan_kernelstart;
	extern vaddr_t kasan_kernelsize;

	kasan_shadow_map((void *)kasan_kernelstart, kasan_kernelsize);

	/* The VAs we've created until now. */
	vaddr_t eva;

	eva = pmap_growkernel(VM_KERNEL_VM_BASE);
	kasan_shadow_map((void *)VM_KERNEL_VM_BASE, eva - VM_KERNEL_VM_BASE);
}


static inline bool
__md_unwind_end(const char *name)
{
	static const char * const vectors[] = {
		"undefined_entry",
		"swi_entry",
		"prefetch_abort_entry",
		"data_abort_entry",
		"address_exception_entry",
		"irq_entry",
		"fiqvector"
	};

	for (size_t i = 0; i < __arraycount(vectors); i++) {
		if (!strncmp(name, vectors[i], strlen(vectors[i])))
			return true;
	}

	return false;
}

static void
kasan_md_unwind(void)
{
	uint32_t lr, *fp;
	const char *mod;
	const char *sym;
	size_t nsym;
	int error;

	fp = (uint32_t *)__builtin_frame_address(0);
	nsym = 0;

	while (1) {
		/*
		 * normal frame
		 *  fp[ 0] saved code pointer
		 *  fp[-1] saved lr value
		 *  fp[-2] saved sp value
		 *  fp[-3] saved fp value
		 */
		lr = fp[-1];

		if (lr < VM_MIN_KERNEL_ADDRESS) {
			break;
		}
		error = ksyms_getname(&mod, &sym, (vaddr_t)lr, KSYMS_PROC);
		if (error) {
			break;
		}
		printf("#%zu %p in %s <%s>\n", nsym, (void *)lr, sym, mod);
		if (__md_unwind_end(sym)) {
			break;
		}

		fp = (uint32_t *)fp[-3];
		if (fp == NULL) {
			break;
		}
		nsym++;

		if (nsym >= 15) {
			break;
		}
	}
}

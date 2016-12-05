/*	$NetBSD: loadfile_machdep.c,v 1.13.4.3 2016/12/05 10:54:58 skrll Exp $	*/

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This work is based on the code contributed by Robert Drehmel to the
 * FreeBSD project.
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

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/ctlreg.h>
#include <machine/vmparam.h>
#include <machine/promlib.h>
#include <machine/hypervisor.h>

#include "boot.h"
#include "openfirm.h"


#define MAXSEGNUM	50
#define hi(val)		((uint32_t)(((val) >> 32) & (uint32_t)-1))
#define lo(val)		((uint32_t)((val) & (uint32_t)-1))

#define roundup2(x, y)	(((x)+((y)-1))&(~((y)-1)))


typedef int phandle_t;

extern void	itlb_enter(vaddr_t, uint32_t, uint32_t);
extern void	dtlb_enter(vaddr_t, uint32_t, uint32_t);
extern void	dtlb_replace(vaddr_t, uint32_t, uint32_t);
extern vaddr_t	itlb_va_to_pa(vaddr_t);
extern vaddr_t	dtlb_va_to_pa(vaddr_t);

static void	tlb_init(void);
static void	tlb_init_sun4u(void);
#ifdef SUN4V
static void	tlb_init_sun4v(void);
#endif
void	sparc64_finalize_tlb_sun4u(u_long);
#ifdef SUN4V
void	sparc64_finalize_tlb_sun4v(u_long);
#endif
static int	mmu_mapin(vaddr_t, vsize_t);
static int	mmu_mapin_sun4u(vaddr_t, vsize_t);
#ifdef SUN4V
static int	mmu_mapin_sun4v(vaddr_t, vsize_t);
#endif
static ssize_t	mmu_read(int, void *, size_t);
static void*	mmu_memcpy(void *, const void *, size_t);
static void*	mmu_memset(void *, int, size_t);
static void	mmu_freeall(void);

static int	ofw_mapin(vaddr_t, vsize_t);
static ssize_t	ofw_read(int, void *, size_t);
static void*	ofw_memcpy(void *, const void *, size_t);
static void*	ofw_memset(void *, int, size_t);
static void	ofw_freeall(void);

#if 0
static int	nop_mapin(vaddr_t, vsize_t);
#endif
static ssize_t	nop_read(int, void *, size_t);
static void*	nop_memcpy(void *, const void *, size_t);
static void*	nop_memset(void *, int, size_t);
static void	nop_freeall(void);


struct tlb_entry *dtlb_store = 0;
struct tlb_entry *itlb_store = 0;

int dtlb_slot;
int itlb_slot;
int dtlb_slot_max;
int itlb_slot_max;

static struct kvamap {
	uint64_t start;
	uint64_t end;
} kvamap[MAXSEGNUM];

static struct memsw {
	ssize_t	(* read)(int f, void *addr, size_t size);
	void*	(* memcpy)(void *dst, const void *src, size_t size);
	void*	(* memset)(void *dst, int c, size_t size);
	void	(* freeall)(void);
} memswa[] = {
	{ nop_read, nop_memcpy, nop_memset, nop_freeall },
	{ ofw_read, ofw_memcpy, ofw_memset, ofw_freeall },
	{ mmu_read, mmu_memcpy, mmu_memset, mmu_freeall }
};

static struct memsw *memsw = &memswa[0];

#ifdef SUN4V
static int sun4v = 0;
#endif

/*
 * Check if a memory region is already mapped. Return length and virtual
 * address of unmapped sub-region, if any.
 */
static uint64_t
kvamap_extract(vaddr_t va, vsize_t len, vaddr_t *new_va)
{
	int i;

	*new_va  = va;
	for (i = 0; (len > 0) && (i < MAXSEGNUM); i++) {
		if (kvamap[i].start == NULL)
			break;
		if ((kvamap[i].start <= va) && (va < kvamap[i].end)) {
			uint64_t va_len = kvamap[i].end - va;
			len = (va_len < len) ? len - va_len : 0;
			*new_va = kvamap[i].end;
		}
	}

	return len;
}

/*
 * Record new kernel mapping.
 */
static void
kvamap_enter(uint64_t va, uint64_t len)
{
	int i;

	DPRINTF(("kvamap_enter: %d@%p\n", (int)len, (void*)(u_long)va));
	for (i = 0; (len > 0) && (i < MAXSEGNUM); i++) {
		if (kvamap[i].start == NULL) {
			kvamap[i].start = va;
			kvamap[i].end = va + len;
			break;
		}
	}

	if (i == MAXSEGNUM) {
		panic("Too many allocations requested.");
	}
}

/*
 * Initialize TLB as required by MMU mapping functions.
 */
static void
tlb_init(void)
{
	phandle_t root;
#ifdef SUN4V
	char buf[128];
#endif

	if (dtlb_store != NULL) {
		return;
	}

	if ( (root = prom_findroot()) == -1) {
		panic("tlb_init: prom_findroot()");
	}
#ifdef SUN4V
	if (_prom_getprop(root, "compatible", buf, sizeof(buf)) > 0 &&
		    strcmp(buf, "sun4v") == 0) {
		tlb_init_sun4v();
		sun4v = 1;
	}
	else {
#endif
		tlb_init_sun4u();
#ifdef SUN4V
	}
#endif

	dtlb_store = alloc(dtlb_slot_max * sizeof(*dtlb_store));
	itlb_store = alloc(itlb_slot_max * sizeof(*itlb_store));
	if (dtlb_store == NULL || itlb_store == NULL) {
		panic("tlb_init: malloc");
	}

	dtlb_slot = itlb_slot = 0;
}

/*
 * Initialize TLB as required by MMU mapping functions - sun4u.
 */
static void
tlb_init_sun4u(void)
{
	phandle_t child;
	phandle_t root;
	char buf[128];
	bool foundcpu = false;
	u_int bootcpu;
	u_int cpu;

	bootcpu = get_cpuid();

	if ( (root = prom_findroot()) == -1) {
		panic("tlb_init: prom_findroot()");
	}

	for (child = prom_firstchild(root); child != 0;
			child = prom_nextsibling(child)) {
		if (child == -1) {
			panic("tlb_init: OF_child");
		}
		if (_prom_getprop(child, "device_type", buf, sizeof(buf)) > 0 &&
		    strcmp(buf, "cpu") == 0) {
			if (_prom_getprop(child, "upa-portid", &cpu,
			    sizeof(cpu)) == -1 && _prom_getprop(child, "portid",
			    &cpu, sizeof(cpu)) == -1)
				panic("tlb_init: prom_getprop");
			foundcpu = true;
			if (cpu == bootcpu)
				break;
		}
	}
	if (!foundcpu)
		panic("tlb_init: no cpu found!");
	if (cpu != bootcpu)
		panic("tlb_init: no node for bootcpu?!?!");
	if (_prom_getprop(child, "#dtlb-entries", &dtlb_slot_max,
	    sizeof(dtlb_slot_max)) == -1 ||
	    _prom_getprop(child, "#itlb-entries", &itlb_slot_max,
	    sizeof(itlb_slot_max)) == -1)
		panic("tlb_init: prom_getprop");
}

#ifdef SUN4V
/*
 * Initialize TLB as required by MMU mapping functions - sun4v.
 */
static void
tlb_init_sun4v(void)
{
	psize_t len;
	paddr_t pa;
	int64_t hv_rc;

	hv_mach_desc((paddr_t)NULL, &len); /* Trick to get actual length */
	if ( !len ) {
		panic("init_tlb: hv_mach_desc() failed");
	}
	pa = OF_alloc_phys(len, 16);
	if ( pa == -1 ) {
		panic("OF_alloc_phys() failed");
	}
	hv_rc = hv_mach_desc(pa, &len);
	if (hv_rc != H_EOK) {
		panic("hv_mach_desc() failed");
	}
	/* XXX dig out TLB node info - 64 is ok for loading the kernel */
	dtlb_slot_max = itlb_slot_max = 64;
}
#endif

/*
 * Map requested memory region with permanent 4MB pages.
 */
static int
mmu_mapin(vaddr_t rva, vsize_t len)
{
	len  = roundup2(len + (rva & PAGE_MASK_4M), PAGE_SIZE_4M);
	rva &= ~PAGE_MASK_4M;

	tlb_init();

#if SUN4V
	if ( sun4v )
		return mmu_mapin_sun4v(rva, len);
	else
#endif
		return mmu_mapin_sun4u(rva, len);
}

/*
 * Map requested memory region with permanent 4MB pages - sun4u.
 */
static int
mmu_mapin_sun4u(vaddr_t rva, vsize_t len)
{
	uint64_t data;
	paddr_t pa;
	vaddr_t va, mva;

	for (pa = (paddr_t)-1; len > 0; rva = va) {
		if ( (len = kvamap_extract(rva, len, &va)) == 0) {
			/* The rest is already mapped */
			break;
		}

		if (dtlb_va_to_pa(va) == (u_long)-1 ||
		    itlb_va_to_pa(va) == (u_long)-1) {
			/* Allocate a physical page, claim the virtual area */
			if (pa == (paddr_t)-1) {
				pa = OF_alloc_phys(PAGE_SIZE_4M, PAGE_SIZE_4M);
				if (pa == (paddr_t)-1)
					panic("out of memory");
				mva = OF_claim_virt(va, PAGE_SIZE_4M);
				if (mva != va) {
					panic("can't claim virtual page "
					    "(wanted %#lx, got %#lx)",
					    va, mva);
				}
				/* The mappings may have changed, be paranoid. */
				continue;
			}

			/*
			 * Actually, we can only allocate two pages less at
			 * most (depending on the kernel TSB size).
			 */
			if (dtlb_slot >= dtlb_slot_max)
				panic("mmu_mapin: out of dtlb_slots");
			if (itlb_slot >= itlb_slot_max)
				panic("mmu_mapin: out of itlb_slots");

			DPRINTF(("mmu_mapin: 0x%lx:0x%x.0x%x\n", va,
			    hi(pa), lo(pa)));

			data = SUN4U_TSB_DATA(0,	/* global */
					PGSZ_4M,	/* 4mb page */
					pa,		/* phys.address */
					1,		/* privileged */
					1,		/* write */
					1,		/* cache */
					1,		/* alias */
					1,		/* valid */
					0,		/* endianness */
					0		/* wc */
					);
			data |= SUN4U_TLB_L | SUN4U_TLB_CV; /* locked, virt.cache */

			dtlb_store[dtlb_slot].te_pa = pa;
			dtlb_store[dtlb_slot].te_va = va;
			dtlb_slot++;
			dtlb_enter(va, hi(data), lo(data));
			pa = (paddr_t)-1;
		}

		kvamap_enter(va, PAGE_SIZE_4M);

		len -= len > PAGE_SIZE_4M ? PAGE_SIZE_4M : len;
		va += PAGE_SIZE_4M;
	}

	if (pa != (paddr_t)-1) {
		OF_free_phys(pa, PAGE_SIZE_4M);
	}

	return (0);
}

#ifdef SUN4V
/*
 * Map requested memory region with permanent 4MB pages - sun4v.
 */
static int
mmu_mapin_sun4v(vaddr_t rva, vsize_t len)
{
	uint64_t data;
	paddr_t pa;
	vaddr_t va, mva;
	int64_t hv_rc;

	for (pa = (paddr_t)-1; len > 0; rva = va) {
		if ( (len = kvamap_extract(rva, len, &va)) == 0) {
			/* The rest is already mapped */
			break;
		}

		/* Allocate a physical page, claim the virtual area */
		if (pa == (paddr_t)-1) {
			pa = OF_alloc_phys(PAGE_SIZE_4M, PAGE_SIZE_4M);
			if (pa == (paddr_t)-1)
				panic("out of memory");
			mva = OF_claim_virt(va, PAGE_SIZE_4M);
			if (mva != va) {
				panic("can't claim virtual page "
				    "(wanted %#lx, got %#lx)",
				    va, mva);
			}
		}

		/*
		 * Actually, we can only allocate two pages less at
		 * most (depending on the kernel TSB size).
		 */
		if (dtlb_slot >= dtlb_slot_max)
			panic("mmu_mapin: out of dtlb_slots");
		if (itlb_slot >= itlb_slot_max)
			panic("mmu_mapin: out of itlb_slots");

		DPRINTF(("mmu_mapin: 0x%lx:0x%x.0x%x\n", va,
		    hi(pa), lo(pa)));

		data = SUN4V_TSB_DATA(
			0,		/* global */
			PGSZ_4M,	/* 4mb page */
			pa,		/* phys.address */
			1,		/* privileged */
			1,		/* write */
			1,		/* cache */
			1,		/* alias */
			1,		/* valid */
			0,		/* endianness */
			0		/* wc */
			);
		data |= SUN4V_TLB_CV; /* virt.cache */

		dtlb_store[dtlb_slot].te_pa = pa;
		dtlb_store[dtlb_slot].te_va = va;
		dtlb_slot++;
		hv_rc = hv_mmu_map_perm_addr(va, data, MAP_DTLB);
		if ( hv_rc != H_EOK ) {
			panic("hv_mmu_map_perm_addr() failed - rc = %ld", hv_rc);
		}

		kvamap_enter(va, PAGE_SIZE_4M);

		pa = (paddr_t)-1;

		len -= len > PAGE_SIZE_4M ? PAGE_SIZE_4M : len;
		va += PAGE_SIZE_4M;
	}

	if (pa != (paddr_t)-1) {
		OF_free_phys(pa, PAGE_SIZE_4M);
	}

	return (0);
}
#endif

static ssize_t
mmu_read(int f, void *addr, size_t size)
{
	mmu_mapin((vaddr_t)addr, size);
	return read(f, addr, size);
}

static void*
mmu_memcpy(void *dst, const void *src, size_t size)
{
	mmu_mapin((vaddr_t)dst, size);
	return memcpy(dst, src, size);
}

static void*
mmu_memset(void *dst, int c, size_t size)
{
	mmu_mapin((vaddr_t)dst, size);
	return memset(dst, c, size);
}

static void
mmu_freeall(void)
{
	int i;

	dtlb_slot = itlb_slot = 0;
	for (i = 0; i < MAXSEGNUM; i++) {
		/* XXX return all mappings to PROM and unmap the pages! */
		kvamap[i].start = kvamap[i].end = 0;
	}
}

/*
 * Claim requested memory region in OpenFirmware allocation pool.
 */
static int
ofw_mapin(vaddr_t rva, vsize_t len)
{
	vaddr_t va;

	len  = roundup2(len + (rva & PAGE_MASK_4M), PAGE_SIZE_4M);
	rva &= ~PAGE_MASK_4M;

	if ( (len = kvamap_extract(rva, len, &va)) != 0) {
		if (OF_claim((void *)(long)va, len, PAGE_SIZE_4M) == (void*)-1){
			panic("ofw_mapin: Cannot claim memory.");
		}
		kvamap_enter(va, len);
	}

	return (0);
}

static ssize_t
ofw_read(int f, void *addr, size_t size)
{
	ofw_mapin((vaddr_t)addr, size);
	return read(f, addr, size);
}

static void*
ofw_memcpy(void *dst, const void *src, size_t size)
{
	ofw_mapin((vaddr_t)dst, size);
	return memcpy(dst, src, size);
}

static void*
ofw_memset(void *dst, int c, size_t size)
{
	ofw_mapin((vaddr_t)dst, size);
	return memset(dst, c, size);
}

static void
ofw_freeall(void)
{
	int i;

	dtlb_slot = itlb_slot = 0;
	for (i = 0; i < MAXSEGNUM; i++) {
		OF_release((void*)(u_long)kvamap[i].start,
				(u_int)(kvamap[i].end - kvamap[i].start));
		kvamap[i].start = kvamap[i].end = 0;
	}
}

/*
 * NOP implementation exists solely for kernel header loading sake. Here
 * we use alloc() interface to allocate memory and avoid doing some dangerous
 * things.
 */
static ssize_t
nop_read(int f, void *addr, size_t size)
{
	return read(f, addr, size);
}

static void*
nop_memcpy(void *dst, const void *src, size_t size)
{
	/*
	 * Real NOP to make LOAD_HDR work: loadfile_elfXX copies ELF headers
	 * right after the highest kernel address which will not be mapped with
	 * nop_XXX operations.
	 */
	return (dst);
}

static void*
nop_memset(void *dst, int c, size_t size)
{
	return memset(dst, c, size);
}

static void
nop_freeall(void)
{ }

/*
 * loadfile() hooks.
 */
ssize_t
sparc64_read(int f, void *addr, size_t size)
{
	return (*memsw->read)(f, addr, size);
}

void*
sparc64_memcpy(void *dst, const void *src, size_t size)
{
	return (*memsw->memcpy)(dst, src, size);
}

void*
sparc64_memset(void *dst, int c, size_t size)
{
	return (*memsw->memset)(dst, c, size);
}

/*
 * Remove write permissions from text mappings in the dTLB.
 * Add entries in the iTLB.
 */
void
sparc64_finalize_tlb(u_long data_va)
{
#ifdef SUN4V
	if ( sun4v )
		sparc64_finalize_tlb_sun4v(data_va);
	else
#endif
		sparc64_finalize_tlb_sun4u(data_va);
}

/*
 * Remove write permissions from text mappings in the dTLB - sun4u.
 * Add entries in the iTLB.
 */
void
sparc64_finalize_tlb_sun4u(u_long data_va)
{
	int i;
	int64_t data;
	bool writable_text = false;

	for (i = 0; i < dtlb_slot; i++) {
		if (dtlb_store[i].te_va >= data_va) {
			/*
			 * If (for whatever reason) the start of the
			 * writable section is right at the start of
			 * the kernel, we need to map it into the ITLB
			 * nevertheless (and don't make it readonly).
			 */
			if (i == 0 && dtlb_store[i].te_va == data_va)
				writable_text = true;
			else
				continue;
		}

		data = SUN4U_TSB_DATA(0,	/* global */
				PGSZ_4M,	/* 4mb page */
				dtlb_store[i].te_pa,	/* phys.address */
				1,		/* privileged */
				0,		/* write */
				1,		/* cache */
				1,		/* alias */
				1,		/* valid */
				0,		/* endianness */
				0		/* wc */
				);
		data |= SUN4U_TLB_L | SUN4U_TLB_CV; /* locked, virt.cache */
		if (!writable_text)
			dtlb_replace(dtlb_store[i].te_va, hi(data), lo(data));
		itlb_store[itlb_slot] = dtlb_store[i];
		itlb_slot++;
		itlb_enter(dtlb_store[i].te_va, hi(data), lo(data));
	}
	if (writable_text)
		printf("WARNING: kernel text mapped writable!\n");

}

#ifdef SUN4V
/*
 * Remove write permissions from text mappings in the dTLB - sun4v.
 * Add entries in the iTLB.
 */
void
sparc64_finalize_tlb_sun4v(u_long data_va)
{
	int i;
	int64_t data;
	bool writable_text = false;
	int64_t hv_rc;

	for (i = 0; i < dtlb_slot; i++) {
		if (dtlb_store[i].te_va >= data_va) {
			/*
			 * If (for whatever reason) the start of the
			 * writable section is right at the start of
			 * the kernel, we need to map it into the ITLB
			 * nevertheless (and don't make it readonly).
			 */
			if (i == 0 && dtlb_store[i].te_va == data_va)
				writable_text = true;
			else
				continue;
		}

		data = SUN4V_TSB_DATA(
			0,		/* global */
			PGSZ_4M,	/* 4mb page */
			dtlb_store[i].te_pa,	/* phys.address */
			1,		/* privileged */
			0,		/* write */
			1,		/* cache */
			1,		/* alias */
			1,		/* valid */
			0,		/* endianness */
			0		/* wc */
			);
		data |= SUN4V_TLB_CV|SUN4V_TLB_X; /* virt.cache, executable */
		if (!writable_text) {
			hv_rc = hv_mmu_unmap_perm_addr(dtlb_store[i].te_va,
			                               MAP_DTLB);
			if ( hv_rc != H_EOK ) {
				panic("hv_mmu_unmap_perm_addr() failed - "
				      "rc = %ld", hv_rc);
			}
			hv_rc = hv_mmu_map_perm_addr(dtlb_store[i].te_va, data,
			                             MAP_DTLB);
			if ( hv_rc != H_EOK ) {
				panic("hv_mmu_map_perm_addr() failed - "
				      "rc = %ld", hv_rc);
			}
		}

		itlb_store[itlb_slot] = dtlb_store[i];
		itlb_slot++;
		hv_rc = hv_mmu_map_perm_addr(dtlb_store[i].te_va, data,
		                             MAP_ITLB);
		if ( hv_rc != H_EOK ) {
			panic("hv_mmu_map_perm_addr() failed - rc = %ld", hv_rc);
		}
	}
	if (writable_text)
		printf("WARNING: kernel text mapped writable!\n");
}
#endif

/*
 * Record kernel mappings in bootinfo structure.
 */
void
sparc64_bi_add(void)
{
	int i;
	int itlb_size, dtlb_size;
	struct btinfo_count bi_count;
	struct btinfo_tlb *bi_itlb, *bi_dtlb;

	bi_count.count = itlb_slot;
	bi_add(&bi_count, BTINFO_ITLB_SLOTS, sizeof(bi_count));
	bi_count.count = dtlb_slot;
	bi_add(&bi_count, BTINFO_DTLB_SLOTS, sizeof(bi_count));

	itlb_size = sizeof(*bi_itlb) + sizeof(struct tlb_entry) * itlb_slot;
	dtlb_size = sizeof(*bi_dtlb) + sizeof(struct tlb_entry) * dtlb_slot;

	bi_itlb = alloc(itlb_size);
	bi_dtlb = alloc(dtlb_size);

	if ((bi_itlb == NULL) || (bi_dtlb == NULL)) {
		panic("Out of memory in sparc64_bi_add.\n");
	}

	for (i = 0; i < itlb_slot; i++) {
		bi_itlb->tlb[i].te_va = itlb_store[i].te_va;
		bi_itlb->tlb[i].te_pa = itlb_store[i].te_pa;
	}
	bi_add(bi_itlb, BTINFO_ITLB, itlb_size);

	for (i = 0; i < dtlb_slot; i++) {
		bi_dtlb->tlb[i].te_va = dtlb_store[i].te_va;
		bi_dtlb->tlb[i].te_pa = dtlb_store[i].te_pa;
	}
	bi_add(bi_dtlb, BTINFO_DTLB, dtlb_size);
}

/*
 * Choose kernel image mapping strategy:
 *
 * LOADFILE_NOP_ALLOCATOR	To load kernel image headers
 * LOADFILE_OFW_ALLOCATOR	To map the kernel by OpenFirmware means
 * LOADFILE_MMU_ALLOCATOR	To use permanent 4MB mappings
 */
void
loadfile_set_allocator(int type)
{
	if (type >= (sizeof(memswa) / sizeof(struct memsw))) {
		panic("Bad allocator request.\n");
	}

	/*
	 * Release all memory claimed by previous allocator and schedule
	 * another allocator for succeeding memory allocation calls.
	 */
	(*memsw->freeall)();
	memsw = &memswa[type];
}

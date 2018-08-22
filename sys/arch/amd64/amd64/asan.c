/*	$NetBSD: asan.c,v 1.3 2018/08/22 12:07:42 maxv Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard, and Siddharth Muralee.
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
__KERNEL_RCSID(0, "$NetBSD: asan.c,v 1.3 2018/08/22 12:07:42 maxv Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/asan.h>

#include <uvm/uvm.h>
#include <amd64/pmap.h>
#include <amd64/vmparam.h>

#define VIRTUAL_SHIFT		47	/* 48bit address space, cut half */
#define CANONICAL_BASE		0xFFFF800000000000

#define KASAN_SHADOW_SCALE_SHIFT	3
#define KASAN_SHADOW_SCALE_SIZE		(1UL << KASAN_SHADOW_SCALE_SHIFT)
#define KASAN_SHADOW_MASK		(KASAN_SHADOW_SCALE_SIZE - 1)

#define KASAN_SHADOW_SIZE	(1ULL << (VIRTUAL_SHIFT - KASAN_SHADOW_SCALE_SHIFT))
#define KASAN_SHADOW_START	(VA_SIGN_NEG((L4_SLOT_KASAN * NBPD_L4)))
#define KASAN_SHADOW_END	(KASAN_SHADOW_START + KASAN_SHADOW_SIZE)

#define __RET_ADDR	(unsigned long)__builtin_return_address(0)

void kasan_shadow_map(void *, size_t);
void kasan_early_init(void);
void kasan_init(void);

static bool kasan_enabled __read_mostly = false;

static inline int8_t *kasan_addr_to_shad(const void *addr)
{
	vaddr_t va = (vaddr_t)addr;
	return (int8_t *)(KASAN_SHADOW_START +
	    ((va - CANONICAL_BASE) >> KASAN_SHADOW_SCALE_SHIFT));
}

static __always_inline bool
kasan_unsupported(vaddr_t addr)
{
	return (addr >= (vaddr_t)PTE_BASE &&
	    addr < ((vaddr_t)PTE_BASE + NBPD_L4));
}

/* -------------------------------------------------------------------------- */

static bool kasan_early __read_mostly = true;
static uint8_t earlypages[8 * PAGE_SIZE] __aligned(PAGE_SIZE);
static size_t earlytaken = 0;

static paddr_t
kasan_early_palloc(void)
{
	paddr_t ret;

	KASSERT(earlytaken < 8);

	ret = (paddr_t)(&earlypages[0] + earlytaken * PAGE_SIZE);
	earlytaken++;

	ret -= KERNBASE;

	return ret;
}

static paddr_t
kasan_palloc(void)
{
	paddr_t pa;

	if (__predict_false(kasan_early))
		pa = kasan_early_palloc();
	else
		pa = pmap_get_physpage();

	return pa;
}

static void
kasan_shadow_map_page(vaddr_t va)
{
	paddr_t pa;

	if (!pmap_valid_entry(L4_BASE[pl4_i(va)])) {
		pa = kasan_palloc();
		L4_BASE[pl4_i(va)] = pa | PG_KW | pmap_pg_nx | PG_V;
	}
	if (!pmap_valid_entry(L3_BASE[pl3_i(va)])) {
		pa = kasan_palloc();
		L3_BASE[pl3_i(va)] = pa | PG_KW | pmap_pg_nx | PG_V;
	}
	if (!pmap_valid_entry(L2_BASE[pl2_i(va)])) {
		pa = kasan_palloc();
		L2_BASE[pl2_i(va)] = pa | PG_KW | pmap_pg_nx | PG_V;
	}
	if (!pmap_valid_entry(L1_BASE[pl1_i(va)])) {
		pa = kasan_palloc();
		L1_BASE[pl1_i(va)] = pa | PG_KW | pmap_pg_g | pmap_pg_nx | PG_V;
	}
}

/*
 * Allocate the necessary stuff in the shadow, so that we can monitor the
 * passed area.
 */
void
kasan_shadow_map(void *addr, size_t size)
{
	size_t sz, npages, i;
	vaddr_t sva, eva;

	KASSERT((vaddr_t)addr % KASAN_SHADOW_SCALE_SIZE == 0);

	sz = roundup(size, KASAN_SHADOW_SCALE_SIZE) / KASAN_SHADOW_SCALE_SIZE;

	sva = (vaddr_t)kasan_addr_to_shad(addr);
	eva = (vaddr_t)kasan_addr_to_shad(addr) + sz;

	sva = rounddown(sva, PAGE_SIZE);
	eva = roundup(eva, PAGE_SIZE);

	npages = (eva - sva) / PAGE_SIZE;

	KASSERT(sva >= KASAN_SHADOW_START && eva < KASAN_SHADOW_END);

	for (i = 0; i < npages; i++) {
		kasan_shadow_map_page(sva + i * PAGE_SIZE);
	}
}

/* -------------------------------------------------------------------------- */

#ifdef __HAVE_PCPU_AREA
#error "PCPU area not allowed with KASAN"
#endif
#ifdef __HAVE_DIRECT_MAP
#error "DMAP not allowed with KASAN"
#endif

static void
kasan_ctors(void)
{
	extern uint64_t __CTOR_LIST__, __CTOR_END__;
	size_t nentries, i;
	uint64_t *ptr;

	nentries = ((size_t)&__CTOR_END__ - (size_t)&__CTOR_LIST__) /
	    sizeof(uintptr_t);

	ptr = &__CTOR_LIST__;
	for (i = 0; i < nentries; i++) {
		void (*func)(void);

		func = (void *)(*ptr);
		(*func)();

		ptr++;
	}
}

/*
 * Map only the current stack. We will map the rest in kasan_init.
 */
void
kasan_early_init(void)
{
	extern vaddr_t lwp0uarea;

	kasan_shadow_map((void *)lwp0uarea, USPACE);
	kasan_early = false;
}

/*
 * Create the shadow mapping. We don't create the 'User' area, because we
 * exclude it from the monitoring. The 'Main' area is created dynamically
 * in pmap_growkernel.
 */
void
kasan_init(void)
{
	extern struct bootspace bootspace;
	size_t i;

	CTASSERT((KASAN_SHADOW_SIZE / NBPD_L4) == NL4_SLOT_KASAN);

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

	kasan_enabled = true;

	/* Call the ASAN constructors. */
	kasan_ctors();
}

/* -------------------------------------------------------------------------- */

static void
kasan_report(unsigned long addr, size_t size, bool write, unsigned long rip)
{
	printf("kASan: Unauthorized Access In %p: Addr %p [%zu byte%s, %s]\n",
	    (void *)rip, (void *)addr, size, (size > 1 ? "s" : ""),
	    (write ? "write" : "read"));
}

/* -------------------------------------------------------------------------- */

/* Our redzone values. */
#define KASAN_GLOBAL_REDZONE	0xFA
#define KASAN_MEMORY_REDZONE	0xFB

/* Stack redzone shadow values. Part of the compiler ABI. */
#define KASAN_STACK_LEFT	0xF1
#define KASAN_STACK_MID		0xF2
#define KASAN_STACK_RIGHT	0xF3
#define KASAN_STACK_PARTIAL	0xF4
#define KASAN_USE_AFTER_SCOPE	0xF8

static void
kasan_shadow_fill(const void *addr, size_t size, uint8_t val)
{
	void *shad;

	if (__predict_false(!kasan_enabled))
		return;
	if (__predict_false(size == 0))
		return;
	if (__predict_false(kasan_unsupported((vaddr_t)addr)))
		return;

	KASSERT((vaddr_t)addr % KASAN_SHADOW_SCALE_SIZE == 0);
	KASSERT(size % KASAN_SHADOW_SCALE_SIZE == 0);

	shad = (void *)kasan_addr_to_shad(addr);
	size = size >> KASAN_SHADOW_SCALE_SHIFT;

	__builtin_memset(shad, val, size);
}

static __always_inline void
kasan_shadow_1byte_markvalid(unsigned long addr)
{
	int8_t *byte = kasan_addr_to_shad((void *)addr);
	int8_t last = (addr & KASAN_SHADOW_MASK) + 1;

	*byte = last;
}

void
kasan_add_redzone(size_t *size)
{
	*size = roundup(*size, KASAN_SHADOW_SCALE_SIZE);
	*size += KASAN_SHADOW_SCALE_SIZE;
}

static void
kasan_markmem(const void *addr, size_t size, bool valid)
{
	size_t i;

	KASSERT((vaddr_t)addr % KASAN_SHADOW_SCALE_SIZE == 0);

	if (valid) {
		for (i = 0; i < size; i++) {
			kasan_shadow_1byte_markvalid((unsigned long)addr+i);
		}
	} else {
		KASSERT(size % KASAN_SHADOW_SCALE_SIZE == 0);
		kasan_shadow_fill(addr, size, KASAN_MEMORY_REDZONE);
	}
}

void
kasan_alloc(const void *addr, size_t size, size_t sz_with_redz)
{
	kasan_markmem(addr, sz_with_redz, false);
	kasan_markmem(addr, size, true);
}

void
kasan_free(const void *addr, size_t sz_with_redz)
{
	kasan_markmem(addr, sz_with_redz, true);
}

/* -------------------------------------------------------------------------- */

#define ADDR_CROSSES_SCALE_BOUNDARY(addr, size) 		\
	(addr >> KASAN_SHADOW_SCALE_SHIFT) !=			\
	    ((addr + size - 1) >> KASAN_SHADOW_SCALE_SHIFT)

static __always_inline bool
kasan_shadow_1byte_isvalid(unsigned long addr)
{
	int8_t *byte = kasan_addr_to_shad((void *)addr);
	int8_t last = (addr & KASAN_SHADOW_MASK) + 1;

	return __predict_true(*byte == 0 || last <= *byte);
}

static __always_inline bool
kasan_shadow_2byte_isvalid(unsigned long addr)
{
	int8_t *byte, last;

	if (ADDR_CROSSES_SCALE_BOUNDARY(addr, 2)) {
		return (kasan_shadow_1byte_isvalid(addr) &&
		    kasan_shadow_1byte_isvalid(addr+1));
	}

	byte = kasan_addr_to_shad((void *)addr);
	last = ((addr + 1) & KASAN_SHADOW_MASK) + 1;

	return __predict_true(*byte == 0 || last <= *byte);
}

static __always_inline bool
kasan_shadow_4byte_isvalid(unsigned long addr)
{
	int8_t *byte, last;

	if (ADDR_CROSSES_SCALE_BOUNDARY(addr, 4)) {
		return (kasan_shadow_2byte_isvalid(addr) &&
		    kasan_shadow_2byte_isvalid(addr+2));
	}

	byte = kasan_addr_to_shad((void *)addr);
	last = ((addr + 3) & KASAN_SHADOW_MASK) + 1;

	return __predict_true(*byte == 0 || last <= *byte);
}

static __always_inline bool
kasan_shadow_8byte_isvalid(unsigned long addr)
{
	int8_t *byte, last;

	if (ADDR_CROSSES_SCALE_BOUNDARY(addr, 8)) {
		return (kasan_shadow_4byte_isvalid(addr) &&
		    kasan_shadow_4byte_isvalid(addr+4));
	}

	byte = kasan_addr_to_shad((void *)addr);
	last = ((addr + 7) & KASAN_SHADOW_MASK) + 1;

	return __predict_true(*byte == 0 || last <= *byte);
}

static __always_inline bool
kasan_shadow_Nbyte_isvalid(unsigned long addr, size_t size)
{
	size_t i;

	for (i = 0; i < size; i++) {
		if (!kasan_shadow_1byte_isvalid(addr+i))
			return false;
	}

	return true;
}

static __always_inline void
kasan_shadow_check(unsigned long addr, size_t size, bool write,
    unsigned long retaddr)
{
	bool valid;

	if (__predict_false(!kasan_enabled))
		return;
	if (__predict_false(size == 0))
		return;
	if (__predict_false(kasan_unsupported(addr)))
		return;

	if (__builtin_constant_p(size)) {
		switch (size) {
		case 1:
			valid = kasan_shadow_1byte_isvalid(addr);
			break;
		case 2:
			valid = kasan_shadow_2byte_isvalid(addr);
			break;
		case 4:
			valid = kasan_shadow_4byte_isvalid(addr);
			break;
		case 8:
			valid = kasan_shadow_8byte_isvalid(addr);
			break;
		default:
			valid = kasan_shadow_Nbyte_isvalid(addr, size);
			break;
		}
	} else {
		valid = kasan_shadow_Nbyte_isvalid(addr, size);
	}

	if (__predict_false(!valid)) {
		kasan_report(addr, size, write, retaddr);
	}
}

/* -------------------------------------------------------------------------- */

void *
kasan_memcpy(void *dst, const void *src, size_t len)
{
	kasan_shadow_check((unsigned long)src, len, false, __RET_ADDR);
	kasan_shadow_check((unsigned long)dst, len, true, __RET_ADDR);
	return __builtin_memcpy(dst, src, len);
}

int
kasan_memcmp(const void *b1, const void *b2, size_t len)
{
	kasan_shadow_check((unsigned long)b1, len, false, __RET_ADDR);
	kasan_shadow_check((unsigned long)b2, len, false, __RET_ADDR);
	return __builtin_memcmp(b1, b2, len);
}

void *
kasan_memset(void *b, int c, size_t len)
{
	kasan_shadow_check((unsigned long)b, len, true, __RET_ADDR);
	return __builtin_memset(b, c, len);
}

/* -------------------------------------------------------------------------- */

#if defined(__clang__) && (__clang_major__ - 0 >= 6)
#define ASAN_ABI_VERSION	8
#elif __GNUC_PREREQ__(7, 1) && !defined(__clang__)
#define ASAN_ABI_VERSION	8
#elif __GNUC_PREREQ__(6, 1) && !defined(__clang__)
#define ASAN_ABI_VERSION	6
#else
#error "Unsupported compiler version"
#endif

/*
 * Part of the compiler ABI.
 */
struct __asan_global_source_location {
	const char *filename;
	int line_no;
	int column_no;
};
struct __asan_global {
	const void *beg;		/* address of the global variable */
	size_t size;			/* size of the global variable */
	size_t size_with_redzone;	/* size with the redzone */
	const void *name;		/* name of the variable */
	const void *module_name;	/* name of the module where the var is declared */
	unsigned long has_dynamic_init;	/* the var has dyn initializer (c++) */
	struct __asan_global_source_location *location;
#if ASAN_ABI_VERSION >= 7
	uintptr_t odr_indicator;	/* the address of the ODR indicator symbol */
#endif
};

void __asan_register_globals(struct __asan_global *, size_t);
void __asan_unregister_globals(struct __asan_global *, size_t);

static void
kasan_register_global(struct __asan_global *global)
{
	size_t aligned_size = roundup(global->size, KASAN_SHADOW_SCALE_SIZE);

	/* Poison the redzone following the var. */
	kasan_shadow_fill((void *)((uintptr_t)global->beg + aligned_size),
	    global->size_with_redzone - aligned_size, KASAN_GLOBAL_REDZONE);
}

void
__asan_register_globals(struct __asan_global *globals, size_t size)
{
	size_t i;
	for (i = 0; i < size; i++) {
		kasan_register_global(&globals[i]);
	}
}

void
__asan_unregister_globals(struct __asan_global *globals, size_t size)
{
}

#define ASAN_LOAD_STORE(size)					\
	void __asan_load##size(unsigned long);			\
	void __asan_load##size(unsigned long addr)		\
	{							\
		kasan_shadow_check(addr, size, false, __RET_ADDR);\
	} 							\
	void __asan_load##size##_noabort(unsigned long);	\
	void __asan_load##size##_noabort(unsigned long addr)	\
	{							\
		kasan_shadow_check(addr, size, false, __RET_ADDR);\
	}							\
	void __asan_store##size(unsigned long);			\
	void __asan_store##size(unsigned long addr)		\
	{							\
		kasan_shadow_check(addr, size, true, __RET_ADDR);\
	}							\
	void __asan_store##size##_noabort(unsigned long);	\
	void __asan_store##size##_noabort(unsigned long addr)	\
	{							\
		kasan_shadow_check(addr, size, true, __RET_ADDR);\
	}

ASAN_LOAD_STORE(1);
ASAN_LOAD_STORE(2);
ASAN_LOAD_STORE(4);
ASAN_LOAD_STORE(8);
ASAN_LOAD_STORE(16);

void __asan_loadN(unsigned long, size_t);
void __asan_loadN_noabort(unsigned long, size_t);
void __asan_storeN(unsigned long, size_t);
void __asan_storeN_noabort(unsigned long, size_t);
void __asan_handle_no_return(void);
void __asan_poison_stack_memory(const void *, size_t);
void __asan_unpoison_stack_memory(const void *, size_t);
void __asan_alloca_poison(unsigned long, size_t);
void __asan_allocas_unpoison(const void *, const void *);

void
__asan_loadN(unsigned long addr, size_t size)
{
	kasan_shadow_check(addr, size, false, __RET_ADDR);
}

void
__asan_loadN_noabort(unsigned long addr, size_t size)
{
	kasan_shadow_check(addr, size, false, __RET_ADDR);
}

void
__asan_storeN(unsigned long addr, size_t size)
{
	kasan_shadow_check(addr, size, true, __RET_ADDR);
}

void
__asan_storeN_noabort(unsigned long addr, size_t size)
{
	kasan_shadow_check(addr, size, true, __RET_ADDR);
}

void
__asan_handle_no_return(void)
{
	/* nothing */
}

void
__asan_poison_stack_memory(const void *addr, size_t size)
{
	KASSERT((vaddr_t)addr % KASAN_SHADOW_SCALE_SIZE == 0);
	kasan_shadow_fill(addr, size, KASAN_USE_AFTER_SCOPE);
}

void
__asan_unpoison_stack_memory(const void *addr, size_t size)
{
	KASSERT((vaddr_t)addr % KASAN_SHADOW_SCALE_SIZE == 0);
	kasan_shadow_fill(addr, size, 0);
}

void
__asan_alloca_poison(unsigned long addr, size_t size)
{
	panic("%s: impossible!", __func__);
}

void
__asan_allocas_unpoison(const void *stack_top, const void *stack_bottom)
{
	panic("%s: impossible!", __func__);
}

#define ASAN_SET_SHADOW(byte) \
	void __asan_set_shadow_##byte(void *, size_t);			\
	void __asan_set_shadow_##byte(void *addr, size_t size)		\
	{								\
		__builtin_memset((void *)addr, 0x##byte, size);		\
	}

ASAN_SET_SHADOW(00);
ASAN_SET_SHADOW(f1);
ASAN_SET_SHADOW(f2);
ASAN_SET_SHADOW(f3);
ASAN_SET_SHADOW(f5);
ASAN_SET_SHADOW(f8);

/*	$NetBSD: subr_asan.c,v 1.5 2019/02/24 10:44:41 maxv Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_asan.c,v 1.5 2019/02/24 10:44:41 maxv Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/asan.h>

#include <uvm/uvm.h>

/* ASAN constants. Part of the compiler ABI. */
#define KASAN_SHADOW_SCALE_SHIFT	3
#define KASAN_SHADOW_SCALE_SIZE		(1UL << KASAN_SHADOW_SCALE_SHIFT)
#define KASAN_SHADOW_MASK		(KASAN_SHADOW_SCALE_SIZE - 1)

/* The MD code. */
#include <machine/asan.h>

/* Our redzone values. */
#define KASAN_MEMORY_FREED	0xFA
#define KASAN_MEMORY_REDZONE	0xFB

/* Stack redzone values. Part of the compiler ABI. */
#define KASAN_STACK_LEFT	0xF1
#define KASAN_STACK_MID		0xF2
#define KASAN_STACK_RIGHT	0xF3
#define KASAN_STACK_PARTIAL	0xF4
#define KASAN_USE_AFTER_SCOPE	0xF8

/* ASAN ABI version. */
#if defined(__clang__) && (__clang_major__ - 0 >= 6)
#define ASAN_ABI_VERSION	8
#elif __GNUC_PREREQ__(7, 1) && !defined(__clang__)
#define ASAN_ABI_VERSION	8
#elif __GNUC_PREREQ__(6, 1) && !defined(__clang__)
#define ASAN_ABI_VERSION	6
#else
#error "Unsupported compiler version"
#endif

#define __RET_ADDR	(unsigned long)__builtin_return_address(0)

/* Global variable descriptor. Part of the compiler ABI.  */
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

static bool kasan_enabled __read_mostly = false;

/* -------------------------------------------------------------------------- */

void
kasan_shadow_map(void *addr, size_t size)
{
	size_t sz, npages, i;
	vaddr_t sva, eva;

	KASSERT((vaddr_t)addr % KASAN_SHADOW_SCALE_SIZE == 0);

	sz = roundup(size, KASAN_SHADOW_SCALE_SIZE) / KASAN_SHADOW_SCALE_SIZE;

	sva = (vaddr_t)kasan_md_addr_to_shad(addr);
	eva = (vaddr_t)kasan_md_addr_to_shad(addr) + sz;

	sva = rounddown(sva, PAGE_SIZE);
	eva = roundup(eva, PAGE_SIZE);

	npages = (eva - sva) / PAGE_SIZE;

	KASSERT(sva >= KASAN_MD_SHADOW_START && eva < KASAN_MD_SHADOW_END);

	for (i = 0; i < npages; i++) {
		kasan_md_shadow_map_page(sva + i * PAGE_SIZE);
	}
}

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

void
kasan_early_init(void *stack)
{
	kasan_md_early_init(stack);
}

void
kasan_init(void)
{
	/* MD initialization. */
	kasan_md_init();

	/* Now officially enabled. */
	kasan_enabled = true;

	/* Call the ASAN constructors. */
	kasan_ctors();
}

static inline const char *
kasan_code_name(uint8_t code)
{
	switch (code) {
	case KASAN_MEMORY_FREED:
		return "UseAfterFree";
	case KASAN_MEMORY_REDZONE:
		return "RedZone";
	case 1 ... 7:
		return "RedZonePartial";
	case KASAN_STACK_LEFT:
		return "StackLeft";
	case KASAN_STACK_RIGHT:
		return "StackRight";
	case KASAN_STACK_PARTIAL:
		return "StackPartial";
	case KASAN_USE_AFTER_SCOPE:
		return "UseAfterScope";
	default:
		return "Unknown";
	}
}

static void
kasan_report(unsigned long addr, size_t size, bool write, unsigned long pc,
    uint8_t code)
{
	printf("ASan: Unauthorized Access In %p: Addr %p [%zu byte%s, %s,"
	    " %s]\n",
	    (void *)pc, (void *)addr, size, (size > 1 ? "s" : ""),
	    (write ? "write" : "read"), kasan_code_name(code));
	kasan_md_unwind();
}

static __always_inline void
kasan_shadow_1byte_markvalid(unsigned long addr)
{
	int8_t *byte = kasan_md_addr_to_shad((void *)addr);
	int8_t last = (addr & KASAN_SHADOW_MASK) + 1;

	*byte = last;
}

static __always_inline void
kasan_shadow_Nbyte_markvalid(const void *addr, size_t size)
{
	size_t i;

	for (i = 0; i < size; i++) {
		kasan_shadow_1byte_markvalid((unsigned long)addr+i);
	}
}

static __always_inline void
kasan_shadow_Nbyte_fill(const void *addr, size_t size, uint8_t val)
{
	void *shad;

	if (__predict_false(size == 0))
		return;
	if (__predict_false(kasan_md_unsupported((vaddr_t)addr)))
		return;

	KASSERT((vaddr_t)addr % KASAN_SHADOW_SCALE_SIZE == 0);
	KASSERT(size % KASAN_SHADOW_SCALE_SIZE == 0);

	shad = (void *)kasan_md_addr_to_shad(addr);
	size = size >> KASAN_SHADOW_SCALE_SHIFT;

	__builtin_memset(shad, val, size);
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
	KASSERT((vaddr_t)addr % KASAN_SHADOW_SCALE_SIZE == 0);
	if (valid) {
		kasan_shadow_Nbyte_markvalid(addr, size);
	} else {
		kasan_shadow_Nbyte_fill(addr, size, KASAN_MEMORY_REDZONE);
	}
}

void
kasan_softint(struct lwp *l)
{
	const void *stk = (const void *)uvm_lwp_getuarea(l);

	kasan_shadow_Nbyte_fill(stk, USPACE, 0);
}

/*
 * In an area of size 'sz_with_redz', mark the 'size' first bytes as valid,
 * and the rest as invalid. There are generally two use cases:
 *
 *  o kasan_mark(addr, origsize, size), with origsize < size. This marks the
 *    redzone at the end of the buffer as invalid.
 *
 *  o kasan_mark(addr, size, size). This marks the entire buffer as valid.
 */
void
kasan_mark(const void *addr, size_t size, size_t sz_with_redz)
{
	kasan_markmem(addr, sz_with_redz, false);
	kasan_markmem(addr, size, true);
}

/* -------------------------------------------------------------------------- */

#define ADDR_CROSSES_SCALE_BOUNDARY(addr, size) 		\
	(addr >> KASAN_SHADOW_SCALE_SHIFT) !=			\
	    ((addr + size - 1) >> KASAN_SHADOW_SCALE_SHIFT)

static __always_inline bool
kasan_shadow_1byte_isvalid(unsigned long addr, uint8_t *code)
{
	int8_t *byte = kasan_md_addr_to_shad((void *)addr);
	int8_t last = (addr & KASAN_SHADOW_MASK) + 1;

	if (__predict_true(*byte == 0 || last <= *byte)) {
		return true;
	}
	*code = *byte;
	return false;
}

static __always_inline bool
kasan_shadow_2byte_isvalid(unsigned long addr, uint8_t *code)
{
	int8_t *byte, last;

	if (ADDR_CROSSES_SCALE_BOUNDARY(addr, 2)) {
		return (kasan_shadow_1byte_isvalid(addr, code) &&
		    kasan_shadow_1byte_isvalid(addr+1, code));
	}

	byte = kasan_md_addr_to_shad((void *)addr);
	last = ((addr + 1) & KASAN_SHADOW_MASK) + 1;

	if (__predict_true(*byte == 0 || last <= *byte)) {
		return true;
	}
	*code = *byte;
	return false;
}

static __always_inline bool
kasan_shadow_4byte_isvalid(unsigned long addr, uint8_t *code)
{
	int8_t *byte, last;

	if (ADDR_CROSSES_SCALE_BOUNDARY(addr, 4)) {
		return (kasan_shadow_2byte_isvalid(addr, code) &&
		    kasan_shadow_2byte_isvalid(addr+2, code));
	}

	byte = kasan_md_addr_to_shad((void *)addr);
	last = ((addr + 3) & KASAN_SHADOW_MASK) + 1;

	if (__predict_true(*byte == 0 || last <= *byte)) {
		return true;
	}
	*code = *byte;
	return false;
}

static __always_inline bool
kasan_shadow_8byte_isvalid(unsigned long addr, uint8_t *code)
{
	int8_t *byte, last;

	if (ADDR_CROSSES_SCALE_BOUNDARY(addr, 8)) {
		return (kasan_shadow_4byte_isvalid(addr, code) &&
		    kasan_shadow_4byte_isvalid(addr+4, code));
	}

	byte = kasan_md_addr_to_shad((void *)addr);
	last = ((addr + 7) & KASAN_SHADOW_MASK) + 1;

	if (__predict_true(*byte == 0 || last <= *byte)) {
		return true;
	}
	*code = *byte;
	return false;
}

static __always_inline bool
kasan_shadow_Nbyte_isvalid(unsigned long addr, size_t size, uint8_t *code)
{
	size_t i;

	for (i = 0; i < size; i++) {
		if (!kasan_shadow_1byte_isvalid(addr+i, code))
			return false;
	}

	return true;
}

static __always_inline void
kasan_shadow_check(unsigned long addr, size_t size, bool write,
    unsigned long retaddr)
{
	uint8_t code;
	bool valid;

	if (__predict_false(!kasan_enabled))
		return;
	if (__predict_false(size == 0))
		return;
	if (__predict_false(kasan_md_unsupported(addr)))
		return;

	if (__builtin_constant_p(size)) {
		switch (size) {
		case 1:
			valid = kasan_shadow_1byte_isvalid(addr, &code);
			break;
		case 2:
			valid = kasan_shadow_2byte_isvalid(addr, &code);
			break;
		case 4:
			valid = kasan_shadow_4byte_isvalid(addr, &code);
			break;
		case 8:
			valid = kasan_shadow_8byte_isvalid(addr, &code);
			break;
		default:
			valid = kasan_shadow_Nbyte_isvalid(addr, size, &code);
			break;
		}
	} else {
		valid = kasan_shadow_Nbyte_isvalid(addr, size, &code);
	}

	if (__predict_false(!valid)) {
		kasan_report(addr, size, write, retaddr, code);
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

char *
kasan_strcpy(char *dst, const char *src)
{
	char *save = dst;

	while (1) {
		kasan_shadow_check((unsigned long)src, 1, false, __RET_ADDR);
		kasan_shadow_check((unsigned long)dst, 1, true, __RET_ADDR);
		*dst = *src;
		if (*src == '\0')
			break;
		src++, dst++;
	}

	return save;
}

int
kasan_strcmp(const char *s1, const char *s2)
{
	while (1) {
		kasan_shadow_check((unsigned long)s1, 1, false, __RET_ADDR);
		kasan_shadow_check((unsigned long)s2, 1, false, __RET_ADDR);
		if (*s1 != *s2)
			break;
		if (*s1 == '\0')
			return 0;
		s1++, s2++;
	}

	return (*(const unsigned char *)s1 - *(const unsigned char *)s2);
}

size_t
kasan_strlen(const char *str)
{
	const char *s;

	s = str;
	while (1) {
		kasan_shadow_check((unsigned long)s, 1, false, __RET_ADDR);
		if (*s == '\0')
			break;
		s++;
	}

	return (s - str);
}

/* -------------------------------------------------------------------------- */

void __asan_register_globals(struct __asan_global *, size_t);
void __asan_unregister_globals(struct __asan_global *, size_t);

void
__asan_register_globals(struct __asan_global *globals, size_t n)
{
	size_t i;

	for (i = 0; i < n; i++) {
		kasan_mark(globals[i].beg, globals[i].size,
		    globals[i].size_with_redzone);
	}
}

void
__asan_unregister_globals(struct __asan_global *globals, size_t n)
{
	/* never called */
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

void __asan_poison_stack_memory(const void *, size_t);
void __asan_unpoison_stack_memory(const void *, size_t);

void __asan_poison_stack_memory(const void *addr, size_t size)
{
	size = roundup(size, KASAN_SHADOW_SCALE_SIZE);
	kasan_shadow_Nbyte_fill(addr, size, KASAN_USE_AFTER_SCOPE);
}

void __asan_unpoison_stack_memory(const void *addr, size_t size)
{
	kasan_shadow_Nbyte_markvalid(addr, size);
}

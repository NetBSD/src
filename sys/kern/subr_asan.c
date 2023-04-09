/*	$NetBSD: subr_asan.c,v 1.28 2023/04/09 09:18:09 riastradh Exp $	*/

/*
 * Copyright (c) 2018-2020 Maxime Villard, m00nbsd.net
 * All rights reserved.
 *
 * This code is part of the KASAN subsystem of the NetBSD kernel.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_asan.c,v 1.28 2023/04/09 09:18:09 riastradh Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/asan.h>

#include <uvm/uvm_extern.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#ifdef KASAN_PANIC
#define REPORT panic
#else
#define REPORT printf
#endif

/* ASAN constants. Part of the compiler ABI. */
#define KASAN_SHADOW_SCALE_SIZE		(1UL << KASAN_SHADOW_SCALE_SHIFT)
#define KASAN_SHADOW_MASK		(KASAN_SHADOW_SCALE_SIZE - 1)
#define KASAN_ALLOCA_SCALE_SIZE		32

/* The MD code. */
#include <machine/asan.h>

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

	KASSERT(sva >= KASAN_MD_SHADOW_START);
	KASSERT(eva < KASAN_MD_SHADOW_END);

	for (i = 0; i < npages; i++) {
		kasan_md_shadow_map_page(sva + i * PAGE_SIZE);
	}
}

static void
kasan_ctors(void)
{
	extern Elf_Addr __CTOR_LIST__, __CTOR_END__;
	size_t nentries, i;
	Elf_Addr *ptr;

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
	case KASAN_GENERIC_REDZONE:
		return "GenericRedZone";
	case KASAN_MALLOC_REDZONE:
		return "MallocRedZone";
	case KASAN_KMEM_REDZONE:
		return "KmemRedZone";
	case KASAN_POOL_REDZONE:
		return "PoolRedZone";
	case KASAN_POOL_FREED:
		return "PoolUseAfterFree";
	case 1 ... 7:
		return "RedZonePartial";
	case KASAN_STACK_LEFT:
		return "StackLeft";
	case KASAN_STACK_MID:
		return "StackMiddle";
	case KASAN_STACK_RIGHT:
		return "StackRight";
	case KASAN_USE_AFTER_RET:
		return "UseAfterRet";
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
	REPORT("ASan: Unauthorized Access In %p: Addr %p [%zu byte%s, %s,"
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
kasan_shadow_Nbyte_fill(const void *addr, size_t size, uint8_t code)
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

	__builtin_memset(shad, code, size);
}

void
kasan_add_redzone(size_t *size)
{
	*size = roundup(*size, KASAN_SHADOW_SCALE_SIZE);
	*size += KASAN_SHADOW_SCALE_SIZE;
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
 *  o kasan_mark(addr, origsize, size, code), with origsize < size. This marks
 *    the redzone at the end of the buffer as invalid.
 *
 *  o kasan_mark(addr, size, size, 0). This marks the entire buffer as valid.
 */
void
kasan_mark(const void *addr, size_t size, size_t sz_with_redz, uint8_t code)
{
	size_t i, n, redz;
	int8_t *shad;

	KASSERT((vaddr_t)addr % KASAN_SHADOW_SCALE_SIZE == 0);
	redz = sz_with_redz - roundup(size, KASAN_SHADOW_SCALE_SIZE);
	KASSERT(redz % KASAN_SHADOW_SCALE_SIZE == 0);
	shad = kasan_md_addr_to_shad(addr);

	/* Chunks of 8 bytes, valid. */
	n = size / KASAN_SHADOW_SCALE_SIZE;
	for (i = 0; i < n; i++) {
		*shad++ = 0;
	}

	/* Possibly one chunk, mid. */
	if ((size & KASAN_SHADOW_MASK) != 0) {
		*shad++ = (size & KASAN_SHADOW_MASK);
	}

	/* Chunks of 8 bytes, invalid. */
	n = redz / KASAN_SHADOW_SCALE_SIZE;
	for (i = 0; i < n; i++) {
		*shad++ = code;
	}
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
#ifdef DDB
	if (__predict_false(db_recover != NULL))
		return;
#endif
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

void *
kasan_memmove(void *dst, const void *src, size_t len)
{
	kasan_shadow_check((unsigned long)src, len, false, __RET_ADDR);
	kasan_shadow_check((unsigned long)dst, len, true, __RET_ADDR);
	return __builtin_memmove(dst, src, len);
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

char *
kasan_strcat(char *dst, const char *src)
{
	size_t ldst, lsrc;

	ldst = __builtin_strlen(dst);
	lsrc = __builtin_strlen(src);
	kasan_shadow_check((unsigned long)dst, ldst + lsrc + 1, true,
	    __RET_ADDR);
	kasan_shadow_check((unsigned long)src, lsrc + 1, false,
	    __RET_ADDR);

	return __builtin_strcat(dst, src);
}

char *
kasan_strchr(const char *s, int c)
{
	kasan_shadow_check((unsigned long)s, __builtin_strlen(s) + 1, false,
	    __RET_ADDR);
	return __builtin_strchr(s, c);
}

char *
kasan_strrchr(const char *s, int c)
{
	kasan_shadow_check((unsigned long)s, __builtin_strlen(s) + 1, false,
	    __RET_ADDR);
	return __builtin_strrchr(s, c);
}

#undef kcopy
#undef copyinstr
#undef copyoutstr
#undef copyin

int	kasan_kcopy(const void *, void *, size_t);
int	kasan_copyinstr(const void *, void *, size_t, size_t *);
int	kasan_copyoutstr(const void *, void *, size_t, size_t *);
int	kasan_copyin(const void *, void *, size_t);
int	kcopy(const void *, void *, size_t);
int	copyinstr(const void *, void *, size_t, size_t *);
int	copyoutstr(const void *, void *, size_t, size_t *);
int	copyin(const void *, void *, size_t);

int
kasan_kcopy(const void *src, void *dst, size_t len)
{
	kasan_shadow_check((unsigned long)src, len, false, __RET_ADDR);
	kasan_shadow_check((unsigned long)dst, len, true, __RET_ADDR);
	return kcopy(src, dst, len);
}

int
kasan_copyin(const void *uaddr, void *kaddr, size_t len)
{
	kasan_shadow_check((unsigned long)kaddr, len, true, __RET_ADDR);
	return copyin(uaddr, kaddr, len);
}

int
kasan_copyinstr(const void *uaddr, void *kaddr, size_t len, size_t *done)
{
	kasan_shadow_check((unsigned long)kaddr, len, true, __RET_ADDR);
	return copyinstr(uaddr, kaddr, len, done);
}

int
kasan_copyoutstr(const void *kaddr, void *uaddr, size_t len, size_t *done)
{
	kasan_shadow_check((unsigned long)kaddr, len, false, __RET_ADDR);
	return copyoutstr(kaddr, uaddr, len, done);
}

/* -------------------------------------------------------------------------- */

#undef _ucas_32
#undef _ucas_32_mp
#undef _ucas_64
#undef _ucas_64_mp
#undef _ufetch_8
#undef _ufetch_16
#undef _ufetch_32
#undef _ufetch_64

int _ucas_32(volatile uint32_t *, uint32_t, uint32_t, uint32_t *);
int kasan__ucas_32(volatile uint32_t *, uint32_t, uint32_t, uint32_t *);
int
kasan__ucas_32(volatile uint32_t *uaddr, uint32_t old, uint32_t new,
    uint32_t *ret)
{
	kasan_shadow_check((unsigned long)ret, sizeof(*ret), true,
	    __RET_ADDR);
	return _ucas_32(uaddr, old, new, ret);
}

#ifdef __HAVE_UCAS_MP
int _ucas_32_mp(volatile uint32_t *, uint32_t, uint32_t, uint32_t *);
int kasan__ucas_32_mp(volatile uint32_t *, uint32_t, uint32_t, uint32_t *);
int
kasan__ucas_32_mp(volatile uint32_t *uaddr, uint32_t old, uint32_t new,
    uint32_t *ret)
{
	kasan_shadow_check((unsigned long)ret, sizeof(*ret), true,
	    __RET_ADDR);
	return _ucas_32_mp(uaddr, old, new, ret);
}
#endif

#ifdef _LP64
int _ucas_64(volatile uint64_t *, uint64_t, uint64_t, uint64_t *);
int kasan__ucas_64(volatile uint64_t *, uint64_t, uint64_t, uint64_t *);
int
kasan__ucas_64(volatile uint64_t *uaddr, uint64_t old, uint64_t new,
    uint64_t *ret)
{
	kasan_shadow_check((unsigned long)ret, sizeof(*ret), true,
	    __RET_ADDR);
	return _ucas_64(uaddr, old, new, ret);
}

#ifdef __HAVE_UCAS_MP
int _ucas_64_mp(volatile uint64_t *, uint64_t, uint64_t, uint64_t *);
int kasan__ucas_64_mp(volatile uint64_t *, uint64_t, uint64_t, uint64_t *);
int
kasan__ucas_64_mp(volatile uint64_t *uaddr, uint64_t old, uint64_t new,
    uint64_t *ret)
{
	kasan_shadow_check((unsigned long)ret, sizeof(*ret), true,
	    __RET_ADDR);
	return _ucas_64_mp(uaddr, old, new, ret);
}
#endif
#endif

int _ufetch_8(const uint8_t *, uint8_t *);
int kasan__ufetch_8(const uint8_t *, uint8_t *);
int
kasan__ufetch_8(const uint8_t *uaddr, uint8_t *valp)
{
	kasan_shadow_check((unsigned long)valp, sizeof(*valp), true,
	    __RET_ADDR);
	return _ufetch_8(uaddr, valp);
}

int _ufetch_16(const uint16_t *, uint16_t *);
int kasan__ufetch_16(const uint16_t *, uint16_t *);
int
kasan__ufetch_16(const uint16_t *uaddr, uint16_t *valp)
{
	kasan_shadow_check((unsigned long)valp, sizeof(*valp), true,
	    __RET_ADDR);
	return _ufetch_16(uaddr, valp);
}

int _ufetch_32(const uint32_t *, uint32_t *);
int kasan__ufetch_32(const uint32_t *, uint32_t *);
int
kasan__ufetch_32(const uint32_t *uaddr, uint32_t *valp)
{
	kasan_shadow_check((unsigned long)valp, sizeof(*valp), true,
	    __RET_ADDR);
	return _ufetch_32(uaddr, valp);
}

#ifdef _LP64
int _ufetch_64(const uint64_t *, uint64_t *);
int kasan__ufetch_64(const uint64_t *, uint64_t *);
int
kasan__ufetch_64(const uint64_t *uaddr, uint64_t *valp)
{
	kasan_shadow_check((unsigned long)valp, sizeof(*valp), true,
	    __RET_ADDR);
	return _ufetch_64(uaddr, valp);
}
#endif

/* -------------------------------------------------------------------------- */

#undef atomic_add_32
#undef atomic_add_int
#undef atomic_add_long
#undef atomic_add_ptr
#undef atomic_add_64
#undef atomic_add_32_nv
#undef atomic_add_int_nv
#undef atomic_add_long_nv
#undef atomic_add_ptr_nv
#undef atomic_add_64_nv
#undef atomic_and_32
#undef atomic_and_uint
#undef atomic_and_ulong
#undef atomic_and_64
#undef atomic_and_32_nv
#undef atomic_and_uint_nv
#undef atomic_and_ulong_nv
#undef atomic_and_64_nv
#undef atomic_or_32
#undef atomic_or_uint
#undef atomic_or_ulong
#undef atomic_or_64
#undef atomic_or_32_nv
#undef atomic_or_uint_nv
#undef atomic_or_ulong_nv
#undef atomic_or_64_nv
#undef atomic_cas_32
#undef atomic_cas_uint
#undef atomic_cas_ulong
#undef atomic_cas_ptr
#undef atomic_cas_64
#undef atomic_cas_32_ni
#undef atomic_cas_uint_ni
#undef atomic_cas_ulong_ni
#undef atomic_cas_ptr_ni
#undef atomic_cas_64_ni
#undef atomic_swap_32
#undef atomic_swap_uint
#undef atomic_swap_ulong
#undef atomic_swap_ptr
#undef atomic_swap_64
#undef atomic_dec_32
#undef atomic_dec_uint
#undef atomic_dec_ulong
#undef atomic_dec_ptr
#undef atomic_dec_64
#undef atomic_dec_32_nv
#undef atomic_dec_uint_nv
#undef atomic_dec_ulong_nv
#undef atomic_dec_ptr_nv
#undef atomic_dec_64_nv
#undef atomic_inc_32
#undef atomic_inc_uint
#undef atomic_inc_ulong
#undef atomic_inc_ptr
#undef atomic_inc_64
#undef atomic_inc_32_nv
#undef atomic_inc_uint_nv
#undef atomic_inc_ulong_nv
#undef atomic_inc_ptr_nv
#undef atomic_inc_64_nv

#define ASAN_ATOMIC_FUNC_ADD(name, tret, targ1, targ2) \
	void atomic_add_##name(volatile targ1 *, targ2); \
	void kasan_atomic_add_##name(volatile targ1 *, targ2); \
	void kasan_atomic_add_##name(volatile targ1 *ptr, targ2 val) \
	{ \
		kasan_shadow_check((uintptr_t)ptr, sizeof(tret), true, \
		    __RET_ADDR); \
		atomic_add_##name(ptr, val); \
	} \
	tret atomic_add_##name##_nv(volatile targ1 *, targ2); \
	tret kasan_atomic_add_##name##_nv(volatile targ1 *, targ2); \
	tret kasan_atomic_add_##name##_nv(volatile targ1 *ptr, targ2 val) \
	{ \
		kasan_shadow_check((uintptr_t)ptr, sizeof(tret), true, \
		    __RET_ADDR); \
		return atomic_add_##name##_nv(ptr, val); \
	}

#define ASAN_ATOMIC_FUNC_AND(name, tret, targ1, targ2) \
	void atomic_and_##name(volatile targ1 *, targ2); \
	void kasan_atomic_and_##name(volatile targ1 *, targ2); \
	void kasan_atomic_and_##name(volatile targ1 *ptr, targ2 val) \
	{ \
		kasan_shadow_check((uintptr_t)ptr, sizeof(tret), true, \
		    __RET_ADDR); \
		atomic_and_##name(ptr, val); \
	} \
	tret atomic_and_##name##_nv(volatile targ1 *, targ2); \
	tret kasan_atomic_and_##name##_nv(volatile targ1 *, targ2); \
	tret kasan_atomic_and_##name##_nv(volatile targ1 *ptr, targ2 val) \
	{ \
		kasan_shadow_check((uintptr_t)ptr, sizeof(tret), true, \
		    __RET_ADDR); \
		return atomic_and_##name##_nv(ptr, val); \
	}

#define ASAN_ATOMIC_FUNC_OR(name, tret, targ1, targ2) \
	void atomic_or_##name(volatile targ1 *, targ2); \
	void kasan_atomic_or_##name(volatile targ1 *, targ2); \
	void kasan_atomic_or_##name(volatile targ1 *ptr, targ2 val) \
	{ \
		kasan_shadow_check((uintptr_t)ptr, sizeof(tret), true, \
		    __RET_ADDR); \
		atomic_or_##name(ptr, val); \
	} \
	tret atomic_or_##name##_nv(volatile targ1 *, targ2); \
	tret kasan_atomic_or_##name##_nv(volatile targ1 *, targ2); \
	tret kasan_atomic_or_##name##_nv(volatile targ1 *ptr, targ2 val) \
	{ \
		kasan_shadow_check((uintptr_t)ptr, sizeof(tret), true, \
		    __RET_ADDR); \
		return atomic_or_##name##_nv(ptr, val); \
	}

#define ASAN_ATOMIC_FUNC_CAS(name, tret, targ1, targ2) \
	tret atomic_cas_##name(volatile targ1 *, targ2, targ2); \
	tret kasan_atomic_cas_##name(volatile targ1 *, targ2, targ2); \
	tret kasan_atomic_cas_##name(volatile targ1 *ptr, targ2 exp, targ2 new) \
	{ \
		kasan_shadow_check((uintptr_t)ptr, sizeof(tret), true, \
		    __RET_ADDR); \
		return atomic_cas_##name(ptr, exp, new); \
	} \
	tret atomic_cas_##name##_ni(volatile targ1 *, targ2, targ2); \
	tret kasan_atomic_cas_##name##_ni(volatile targ1 *, targ2, targ2); \
	tret kasan_atomic_cas_##name##_ni(volatile targ1 *ptr, targ2 exp, targ2 new) \
	{ \
		kasan_shadow_check((uintptr_t)ptr, sizeof(tret), true, \
		    __RET_ADDR); \
		return atomic_cas_##name##_ni(ptr, exp, new); \
	}

#define ASAN_ATOMIC_FUNC_SWAP(name, tret, targ1, targ2) \
	tret atomic_swap_##name(volatile targ1 *, targ2); \
	tret kasan_atomic_swap_##name(volatile targ1 *, targ2); \
	tret kasan_atomic_swap_##name(volatile targ1 *ptr, targ2 val) \
	{ \
		kasan_shadow_check((uintptr_t)ptr, sizeof(tret), true, \
		    __RET_ADDR); \
		return atomic_swap_##name(ptr, val); \
	}

#define ASAN_ATOMIC_FUNC_DEC(name, tret, targ1) \
	void atomic_dec_##name(volatile targ1 *); \
	void kasan_atomic_dec_##name(volatile targ1 *); \
	void kasan_atomic_dec_##name(volatile targ1 *ptr) \
	{ \
		kasan_shadow_check((uintptr_t)ptr, sizeof(tret), true, \
		    __RET_ADDR); \
		atomic_dec_##name(ptr); \
	} \
	tret atomic_dec_##name##_nv(volatile targ1 *); \
	tret kasan_atomic_dec_##name##_nv(volatile targ1 *); \
	tret kasan_atomic_dec_##name##_nv(volatile targ1 *ptr) \
	{ \
		kasan_shadow_check((uintptr_t)ptr, sizeof(tret), true, \
		    __RET_ADDR); \
		return atomic_dec_##name##_nv(ptr); \
	}

#define ASAN_ATOMIC_FUNC_INC(name, tret, targ1) \
	void atomic_inc_##name(volatile targ1 *); \
	void kasan_atomic_inc_##name(volatile targ1 *); \
	void kasan_atomic_inc_##name(volatile targ1 *ptr) \
	{ \
		kasan_shadow_check((uintptr_t)ptr, sizeof(tret), true, \
		    __RET_ADDR); \
		atomic_inc_##name(ptr); \
	} \
	tret atomic_inc_##name##_nv(volatile targ1 *); \
	tret kasan_atomic_inc_##name##_nv(volatile targ1 *); \
	tret kasan_atomic_inc_##name##_nv(volatile targ1 *ptr) \
	{ \
		kasan_shadow_check((uintptr_t)ptr, sizeof(tret), true, \
		    __RET_ADDR); \
		return atomic_inc_##name##_nv(ptr); \
	}

ASAN_ATOMIC_FUNC_ADD(32, uint32_t, uint32_t, int32_t);
ASAN_ATOMIC_FUNC_ADD(64, uint64_t, uint64_t, int64_t);
ASAN_ATOMIC_FUNC_ADD(int, unsigned int, unsigned int, int);
ASAN_ATOMIC_FUNC_ADD(long, unsigned long, unsigned long, long);
ASAN_ATOMIC_FUNC_ADD(ptr, void *, void, ssize_t);

ASAN_ATOMIC_FUNC_AND(32, uint32_t, uint32_t, uint32_t);
ASAN_ATOMIC_FUNC_AND(64, uint64_t, uint64_t, uint64_t);
ASAN_ATOMIC_FUNC_AND(uint, unsigned int, unsigned int, unsigned int);
ASAN_ATOMIC_FUNC_AND(ulong, unsigned long, unsigned long, unsigned long);

ASAN_ATOMIC_FUNC_OR(32, uint32_t, uint32_t, uint32_t);
ASAN_ATOMIC_FUNC_OR(64, uint64_t, uint64_t, uint64_t);
ASAN_ATOMIC_FUNC_OR(uint, unsigned int, unsigned int, unsigned int);
ASAN_ATOMIC_FUNC_OR(ulong, unsigned long, unsigned long, unsigned long);

ASAN_ATOMIC_FUNC_CAS(32, uint32_t, uint32_t, uint32_t);
ASAN_ATOMIC_FUNC_CAS(64, uint64_t, uint64_t, uint64_t);
ASAN_ATOMIC_FUNC_CAS(uint, unsigned int, unsigned int, unsigned int);
ASAN_ATOMIC_FUNC_CAS(ulong, unsigned long, unsigned long, unsigned long);
ASAN_ATOMIC_FUNC_CAS(ptr, void *, void, void *);

ASAN_ATOMIC_FUNC_SWAP(32, uint32_t, uint32_t, uint32_t);
ASAN_ATOMIC_FUNC_SWAP(64, uint64_t, uint64_t, uint64_t);
ASAN_ATOMIC_FUNC_SWAP(uint, unsigned int, unsigned int, unsigned int);
ASAN_ATOMIC_FUNC_SWAP(ulong, unsigned long, unsigned long, unsigned long);
ASAN_ATOMIC_FUNC_SWAP(ptr, void *, void, void *);

ASAN_ATOMIC_FUNC_DEC(32, uint32_t, uint32_t)
ASAN_ATOMIC_FUNC_DEC(64, uint64_t, uint64_t)
ASAN_ATOMIC_FUNC_DEC(uint, unsigned int, unsigned int);
ASAN_ATOMIC_FUNC_DEC(ulong, unsigned long, unsigned long);
ASAN_ATOMIC_FUNC_DEC(ptr, void *, void);

ASAN_ATOMIC_FUNC_INC(32, uint32_t, uint32_t)
ASAN_ATOMIC_FUNC_INC(64, uint64_t, uint64_t)
ASAN_ATOMIC_FUNC_INC(uint, unsigned int, unsigned int);
ASAN_ATOMIC_FUNC_INC(ulong, unsigned long, unsigned long);
ASAN_ATOMIC_FUNC_INC(ptr, void *, void);

/* -------------------------------------------------------------------------- */

#ifdef __HAVE_KASAN_INSTR_BUS

#include <sys/bus.h>

#undef bus_space_read_multi_1
#undef bus_space_read_multi_2
#undef bus_space_read_multi_4
#undef bus_space_read_multi_8
#undef bus_space_read_multi_stream_1
#undef bus_space_read_multi_stream_2
#undef bus_space_read_multi_stream_4
#undef bus_space_read_multi_stream_8
#undef bus_space_read_region_1
#undef bus_space_read_region_2
#undef bus_space_read_region_4
#undef bus_space_read_region_8
#undef bus_space_read_region_stream_1
#undef bus_space_read_region_stream_2
#undef bus_space_read_region_stream_4
#undef bus_space_read_region_stream_8
#undef bus_space_write_multi_1
#undef bus_space_write_multi_2
#undef bus_space_write_multi_4
#undef bus_space_write_multi_8
#undef bus_space_write_multi_stream_1
#undef bus_space_write_multi_stream_2
#undef bus_space_write_multi_stream_4
#undef bus_space_write_multi_stream_8
#undef bus_space_write_region_1
#undef bus_space_write_region_2
#undef bus_space_write_region_4
#undef bus_space_write_region_8
#undef bus_space_write_region_stream_1
#undef bus_space_write_region_stream_2
#undef bus_space_write_region_stream_4
#undef bus_space_write_region_stream_8

#define ASAN_BUS_READ_FUNC(bytes, bits) \
	void bus_space_read_multi_##bytes(bus_space_tag_t, bus_space_handle_t,	\
	    bus_size_t, uint##bits##_t *, bus_size_t);				\
	void kasan_bus_space_read_multi_##bytes(bus_space_tag_t,		\
	    bus_space_handle_t, bus_size_t, uint##bits##_t *, bus_size_t);	\
	void kasan_bus_space_read_multi_##bytes(bus_space_tag_t tag,		\
	    bus_space_handle_t hnd, bus_size_t size, uint##bits##_t *buf,	\
	    bus_size_t count)							\
	{									\
		kasan_shadow_check((uintptr_t)buf,				\
		    sizeof(uint##bits##_t) * count, false, __RET_ADDR);		\
		bus_space_read_multi_##bytes(tag, hnd, size, buf, count);	\
	}									\
	void bus_space_read_multi_stream_##bytes(bus_space_tag_t,		\
	    bus_space_handle_t, bus_size_t, uint##bits##_t *, bus_size_t);	\
	void kasan_bus_space_read_multi_stream_##bytes(bus_space_tag_t,		\
	    bus_space_handle_t, bus_size_t, uint##bits##_t *, bus_size_t);	\
	void kasan_bus_space_read_multi_stream_##bytes(bus_space_tag_t tag,	\
	    bus_space_handle_t hnd, bus_size_t size, uint##bits##_t *buf,	\
	    bus_size_t count)							\
	{									\
		kasan_shadow_check((uintptr_t)buf,				\
		    sizeof(uint##bits##_t) * count, false, __RET_ADDR);		\
		bus_space_read_multi_stream_##bytes(tag, hnd, size, buf, count);\
	}									\
	void bus_space_read_region_##bytes(bus_space_tag_t, bus_space_handle_t,	\
	    bus_size_t, uint##bits##_t *, bus_size_t);				\
	void kasan_bus_space_read_region_##bytes(bus_space_tag_t,		\
	    bus_space_handle_t, bus_size_t, uint##bits##_t *, bus_size_t);	\
	void kasan_bus_space_read_region_##bytes(bus_space_tag_t tag,		\
	    bus_space_handle_t hnd, bus_size_t size, uint##bits##_t *buf,	\
	    bus_size_t count)							\
	{									\
		kasan_shadow_check((uintptr_t)buf,				\
		    sizeof(uint##bits##_t) * count, false, __RET_ADDR);		\
		bus_space_read_region_##bytes(tag, hnd, size, buf, count);	\
	}									\
	void bus_space_read_region_stream_##bytes(bus_space_tag_t,		\
	    bus_space_handle_t, bus_size_t, uint##bits##_t *, bus_size_t);	\
	void kasan_bus_space_read_region_stream_##bytes(bus_space_tag_t,	\
	    bus_space_handle_t, bus_size_t, uint##bits##_t *, bus_size_t);	\
	void kasan_bus_space_read_region_stream_##bytes(bus_space_tag_t tag,	\
	    bus_space_handle_t hnd, bus_size_t size, uint##bits##_t *buf,	\
	    bus_size_t count)							\
	{									\
		kasan_shadow_check((uintptr_t)buf,				\
		    sizeof(uint##bits##_t) * count, false, __RET_ADDR);		\
		bus_space_read_region_stream_##bytes(tag, hnd, size, buf, count);\
	}

#define ASAN_BUS_WRITE_FUNC(bytes, bits) \
	void bus_space_write_multi_##bytes(bus_space_tag_t, bus_space_handle_t,	\
	    bus_size_t, const uint##bits##_t *, bus_size_t);			\
	void kasan_bus_space_write_multi_##bytes(bus_space_tag_t,		\
	    bus_space_handle_t, bus_size_t, const uint##bits##_t *, bus_size_t);\
	void kasan_bus_space_write_multi_##bytes(bus_space_tag_t tag,		\
	    bus_space_handle_t hnd, bus_size_t size, const uint##bits##_t *buf,	\
	    bus_size_t count)							\
	{									\
		kasan_shadow_check((uintptr_t)buf,				\
		    sizeof(uint##bits##_t) * count, true, __RET_ADDR);		\
		bus_space_write_multi_##bytes(tag, hnd, size, buf, count);	\
	}									\
	void bus_space_write_multi_stream_##bytes(bus_space_tag_t,		\
	    bus_space_handle_t, bus_size_t, const uint##bits##_t *, bus_size_t);\
	void kasan_bus_space_write_multi_stream_##bytes(bus_space_tag_t,	\
	    bus_space_handle_t, bus_size_t, const uint##bits##_t *, bus_size_t);\
	void kasan_bus_space_write_multi_stream_##bytes(bus_space_tag_t tag,	\
	    bus_space_handle_t hnd, bus_size_t size, const uint##bits##_t *buf,	\
	    bus_size_t count)							\
	{									\
		kasan_shadow_check((uintptr_t)buf,				\
		    sizeof(uint##bits##_t) * count, true, __RET_ADDR);		\
		bus_space_write_multi_stream_##bytes(tag, hnd, size, buf, count);\
	}									\
	void bus_space_write_region_##bytes(bus_space_tag_t, bus_space_handle_t,\
	    bus_size_t, const uint##bits##_t *, bus_size_t);			\
	void kasan_bus_space_write_region_##bytes(bus_space_tag_t,		\
	    bus_space_handle_t, bus_size_t, const uint##bits##_t *, bus_size_t);\
	void kasan_bus_space_write_region_##bytes(bus_space_tag_t tag,		\
	    bus_space_handle_t hnd, bus_size_t size, const uint##bits##_t *buf,	\
	    bus_size_t count)							\
	{									\
		kasan_shadow_check((uintptr_t)buf,				\
		    sizeof(uint##bits##_t) * count, true, __RET_ADDR);		\
		bus_space_write_region_##bytes(tag, hnd, size, buf, count);	\
	}									\
	void bus_space_write_region_stream_##bytes(bus_space_tag_t,		\
	    bus_space_handle_t, bus_size_t, const uint##bits##_t *, bus_size_t);\
	void kasan_bus_space_write_region_stream_##bytes(bus_space_tag_t,	\
	    bus_space_handle_t, bus_size_t, const uint##bits##_t *, bus_size_t);\
	void kasan_bus_space_write_region_stream_##bytes(bus_space_tag_t tag,	\
	    bus_space_handle_t hnd, bus_size_t size, const uint##bits##_t *buf,	\
	    bus_size_t count)							\
	{									\
		kasan_shadow_check((uintptr_t)buf,				\
		    sizeof(uint##bits##_t) * count, true, __RET_ADDR);		\
		bus_space_write_region_stream_##bytes(tag, hnd, size, buf, count);\
	}

ASAN_BUS_READ_FUNC(1, 8)
ASAN_BUS_READ_FUNC(2, 16)
ASAN_BUS_READ_FUNC(4, 32)
ASAN_BUS_READ_FUNC(8, 64)

ASAN_BUS_WRITE_FUNC(1, 8)
ASAN_BUS_WRITE_FUNC(2, 16)
ASAN_BUS_WRITE_FUNC(4, 32)
ASAN_BUS_WRITE_FUNC(8, 64)

#endif /* __HAVE_KASAN_INSTR_BUS */

/* -------------------------------------------------------------------------- */

#include <sys/mbuf.h>

static void
kasan_dma_sync_linear(uint8_t *buf, bus_addr_t offset, bus_size_t len,
    bool write, uintptr_t pc)
{
	kasan_shadow_check((uintptr_t)(buf + offset), len, write, pc);
}

static void
kasan_dma_sync_mbuf(struct mbuf *m, bus_addr_t offset, bus_size_t len,
    bool write, uintptr_t pc)
{
	bus_addr_t minlen;

	for (; m != NULL && len != 0; m = m->m_next) {
		kasan_shadow_check((uintptr_t)m, sizeof(*m), false, pc);

		if (offset >= m->m_len) {
			offset -= m->m_len;
			continue;
		}

		minlen = MIN(len, m->m_len - offset);
		kasan_shadow_check((uintptr_t)(mtod(m, char *) + offset),
		    minlen, write, pc);

		offset = 0;
		len -= minlen;
	}
}

static void
kasan_dma_sync_uio(struct uio *uio, bus_addr_t offset, bus_size_t len,
    bool write, uintptr_t pc)
{
	bus_size_t minlen, resid;
	struct iovec *iov;
	int i;

	kasan_shadow_check((uintptr_t)uio, sizeof(struct uio), false, pc);

	if (!VMSPACE_IS_KERNEL_P(uio->uio_vmspace))
		return;

	resid = uio->uio_resid;
	iov = uio->uio_iov;

	for (i = 0; i < uio->uio_iovcnt && resid != 0; i++) {
		kasan_shadow_check((uintptr_t)&iov[i], sizeof(iov[i]),
		    false, pc);
		minlen = MIN(resid, iov[i].iov_len);
		kasan_shadow_check((uintptr_t)iov[i].iov_base, minlen,
		    write, pc);
		resid -= minlen;
	}
}

void
kasan_dma_sync(bus_dmamap_t map, bus_addr_t offset, bus_size_t len, int ops)
{
	bool write = (ops & (BUS_DMASYNC_PREWRITE|BUS_DMASYNC_POSTWRITE)) != 0;

	switch (map->dm_buftype) {
	case KASAN_DMA_LINEAR:
		kasan_dma_sync_linear(map->dm_buf, offset, len, write,
		    __RET_ADDR);
		break;
	case KASAN_DMA_MBUF:
		kasan_dma_sync_mbuf(map->dm_buf, offset, len, write,
		    __RET_ADDR);
		break;
	case KASAN_DMA_UIO:
		kasan_dma_sync_uio(map->dm_buf, offset, len, write,
		    __RET_ADDR);
		break;
	case KASAN_DMA_RAW:
		break;
	default:
		panic("%s: impossible", __func__);
	}
}

void
kasan_dma_load(bus_dmamap_t map, void *buf, bus_size_t buflen, int type)
{
	map->dm_buf = buf;
	map->dm_buflen = buflen;
	map->dm_buftype = type;
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
		    globals[i].size_with_redzone, KASAN_GENERIC_REDZONE);
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

void
__asan_poison_stack_memory(const void *addr, size_t size)
{
	size = roundup(size, KASAN_SHADOW_SCALE_SIZE);
	kasan_shadow_Nbyte_fill(addr, size, KASAN_USE_AFTER_SCOPE);
}

void
__asan_unpoison_stack_memory(const void *addr, size_t size)
{
	kasan_shadow_Nbyte_markvalid(addr, size);
}

void __asan_alloca_poison(const void *, size_t);
void __asan_allocas_unpoison(const void *, const void *);

void __asan_alloca_poison(const void *addr, size_t size)
{
	const void *l, *r;

	KASSERT((vaddr_t)addr % KASAN_ALLOCA_SCALE_SIZE == 0);

	l = (const uint8_t *)addr - KASAN_ALLOCA_SCALE_SIZE;
	r = (const uint8_t *)addr + roundup(size, KASAN_ALLOCA_SCALE_SIZE);

	kasan_shadow_Nbyte_fill(l, KASAN_ALLOCA_SCALE_SIZE, KASAN_STACK_LEFT);
	kasan_mark(addr, size, roundup(size, KASAN_ALLOCA_SCALE_SIZE),
	    KASAN_STACK_MID);
	kasan_shadow_Nbyte_fill(r, KASAN_ALLOCA_SCALE_SIZE, KASAN_STACK_RIGHT);
}

void __asan_allocas_unpoison(const void *stkbegin, const void *stkend)
{
	size_t size;

	if (__predict_false(!stkbegin))
		return;
	if (__predict_false((uintptr_t)stkbegin > (uintptr_t)stkend))
		return;
	size = (uintptr_t)stkend - (uintptr_t)stkbegin;

	kasan_shadow_Nbyte_fill(stkbegin, size, 0);
}

/*	$NetBSD: subr_msan.c,v 1.19 2023/04/11 10:19:56 riastradh Exp $	*/

/*
 * Copyright (c) 2019-2020 Maxime Villard, m00nbsd.net
 * All rights reserved.
 *
 * This code is part of the KMSAN subsystem of the NetBSD kernel.
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
__KERNEL_RCSID(0, "$NetBSD: subr_msan.c,v 1.19 2023/04/11 10:19:56 riastradh Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/kprintf.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/cpu.h>
#include <sys/msan.h>

#include <ddb/db_active.h>

static void kmsan_printf(const char *, ...);

void kmsan_init_arg(size_t);
void kmsan_init_ret(size_t);

#ifdef KMSAN_PANIC
#define REPORT panic
#else
#define REPORT kmsan_printf
#endif

/* -------------------------------------------------------------------------- */

/*
 * Part of the compiler ABI.
 */

typedef uint32_t msan_orig_t;

typedef struct {
	uint8_t *shad;
	msan_orig_t *orig;
} msan_meta_t;

#define MSAN_PARAM_SIZE		800
#define MSAN_RETVAL_SIZE	800
typedef struct {
	uint8_t param[MSAN_PARAM_SIZE];
	uint8_t retval[MSAN_RETVAL_SIZE];
	uint8_t _va_arg[MSAN_PARAM_SIZE];
	uint8_t va_arg_origin[MSAN_PARAM_SIZE];
	uint64_t va_arg_overflow_size;
	msan_orig_t param_origin[MSAN_PARAM_SIZE];
	msan_orig_t retval_origin;
	msan_orig_t origin;
} msan_tls_t;

/* -------------------------------------------------------------------------- */

/* The MD code. */
#include <machine/msan.h>

/* -------------------------------------------------------------------------- */

#define __RET_ADDR	(uintptr_t)__builtin_return_address(0)
#define MSAN_NCONTEXT	16

typedef struct {
	size_t ctx;
	msan_tls_t tls[MSAN_NCONTEXT];
} msan_lwp_t;

static msan_tls_t dummy_tls;

static uint8_t msan_dummy_shad[PAGE_SIZE] __aligned(PAGE_SIZE);
static uint8_t msan_dummy_orig[PAGE_SIZE] __aligned(PAGE_SIZE);
static msan_lwp_t msan_lwp0;
static bool kmsan_enabled __read_mostly;

/* -------------------------------------------------------------------------- */

static bool kmsan_reporting = false;

static inline void
kmsan_printf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	kprintf(fmt, TOCONS, NULL, NULL, ap);
	va_end(ap);
}

static inline const char *
kmsan_orig_name(int type)
{
	switch (type) {
	case KMSAN_TYPE_STACK:
		return "Stack";
	case KMSAN_TYPE_KMEM:
		return "Kmem";
	case KMSAN_TYPE_MALLOC:
		return "Malloc";
	case KMSAN_TYPE_POOL:
		return "Pool";
	case KMSAN_TYPE_UVM:
		return "Uvm";
	default:
		return "Unknown";
	}
}

/*
 * The format of the string is: "----var@function". Parse it to display a nice
 * warning.
 */
static void
kmsan_report_hook(const void *addr, size_t size, size_t off, const char *hook)
{
	unsigned long symstart;
	const char *mod, *sym;
	msan_orig_t *orig;
	const char *typename;
	char *var, *fn;
	uintptr_t ptr;
	char buf[128];
	int type;
	int s;

	if (__predict_false(panicstr != NULL || db_active || kmsan_reporting))
		return;

	kmsan_reporting = true;
	__insn_barrier();

	orig = (msan_orig_t *)kmsan_md_addr_to_orig(addr);
	orig = (msan_orig_t *)((uintptr_t)orig & ~0x3);

	if (*orig == 0) {
		REPORT("MSan: Uninitialized Memory In %s At Offset "
		    "%zu\n", hook, off);
		goto out;
	}

	kmsan_md_orig_decode(*orig, &type, &ptr);
	typename = kmsan_orig_name(type);

	if (kmsan_md_is_pc(ptr)) {
		s = pserialize_read_enter();
		if (ksyms_getname(&mod, &sym, (vaddr_t)ptr, KSYMS_PROC) ||
		    ksyms_getval(mod, sym, &symstart, KSYMS_PROC)) {
			REPORT("MSan: Uninitialized %s Memory In %s "
			    "At Offset %zu/%zu, IP %p\n", typename, hook, off,
			    size, (void *)ptr);
		} else {
			char soff[16] = "";

			if ((vaddr_t)ptr < symstart) {
				snprintf(soff, sizeof(soff), "-0x%"PRIxVADDR,
				    symstart - (vaddr_t)ptr);
			} else if ((vaddr_t)ptr > symstart) {
				snprintf(soff, sizeof(soff), "+0x%"PRIxVADDR,
				    (vaddr_t)ptr - symstart);
			}
			REPORT("MSan: Uninitialized %s Memory In %s "
			    "At Offset %zu/%zu, From %s%s%lx\n",
			    typename, hook,
			    off, size, sym,
			    ((unsigned long)ptr < symstart ? "-" :
				(unsigned long)ptr > symstart ? "+" :
				""),
			    (unsigned long)ptr - symstart);
		}
		pserialize_read_exit(s);
	} else {
		var = (char *)ptr + 4;
		strlcpy(buf, var, sizeof(buf));
		var = buf;
		fn = __builtin_strchr(buf, '@');
		*fn++ = '\0';
		REPORT("MSan: Uninitialized %s Memory In %s At Offset "
		    "%zu, Variable '%s' From %s()\n", typename, hook, off,
		    var, fn);
	}

out:
	kmsan_md_unwind();
	__insn_barrier();
	kmsan_reporting = false;
}

static void
kmsan_report_inline(msan_orig_t orig, unsigned long pc)
{
	const char *mod, *sym;
	const char *typename;
	char *var, *fn;
	uintptr_t ptr;
	char buf[128];
	int type;
	int s;

	if (__predict_false(panicstr != NULL || db_active || kmsan_reporting))
		return;

	kmsan_reporting = true;
	__insn_barrier();

	if (orig == 0) {
		REPORT("MSan: Uninitialized Variable In %p\n",
		    (void *)pc);
		goto out;
	}

	kmsan_md_orig_decode(orig, &type, &ptr);
	typename = kmsan_orig_name(type);

	if (kmsan_md_is_pc(ptr)) {
		s = pserialize_read_enter();
		if (ksyms_getname(&mod, &sym, (vaddr_t)ptr, KSYMS_PROC)) {
			REPORT("MSan: Uninitialized %s Memory, "
			    "Origin %x\n", typename, orig);
		} else {
			REPORT("MSan: Uninitialized %s Memory "
			    "From %s()\n", typename, sym);
		}
		pserialize_read_exit(s);
	} else {
		var = (char *)ptr + 4;
		strlcpy(buf, var, sizeof(buf));
		var = buf;
		fn = __builtin_strchr(buf, '@');
		*fn++ = '\0';
		REPORT("MSan: Uninitialized Variable '%s' From %s()\n",
		    var, fn);
	}

out:
	kmsan_md_unwind();
	__insn_barrier();
	kmsan_reporting = false;
}

/* -------------------------------------------------------------------------- */

static inline msan_meta_t
kmsan_meta_get(void *addr, size_t size)
{
	msan_meta_t ret;

	if (__predict_false(!kmsan_enabled)) {
		ret.shad = msan_dummy_shad;
		ret.orig = (msan_orig_t *)msan_dummy_orig;
	} else if (__predict_false(kmsan_md_unsupported((vaddr_t)addr))) {
		ret.shad = msan_dummy_shad;
		ret.orig = (msan_orig_t *)msan_dummy_orig;
	} else {
		ret.shad = (void *)kmsan_md_addr_to_shad(addr);
		ret.orig = (msan_orig_t *)kmsan_md_addr_to_orig(addr);
		ret.orig = (msan_orig_t *)((uintptr_t)ret.orig & ~0x3);
	}

	return ret;
}

static inline void
kmsan_origin_fill(void *addr, msan_orig_t o, size_t size)
{
	msan_orig_t *orig;
	size_t i;

	if (__predict_false(!kmsan_enabled))
		return;
	if (__predict_false(kmsan_md_unsupported((vaddr_t)addr)))
		return;

	orig = (msan_orig_t *)kmsan_md_addr_to_orig(addr);
	size += ((uintptr_t)orig & 0x3);
	orig = (msan_orig_t *)((uintptr_t)orig & ~0x3);

	for (i = 0; i < size; i += 4) {
		orig[i / 4] = o;
	}
}

static inline void
kmsan_shadow_fill(void *addr, uint8_t c, size_t size)
{
	uint8_t *shad;

	if (__predict_false(!kmsan_enabled))
		return;
	if (__predict_false(kmsan_md_unsupported((vaddr_t)addr)))
		return;

	shad = kmsan_md_addr_to_shad(addr);
	__builtin_memset(shad, c, size);
}

static inline void
kmsan_meta_copy(void *dst, const void *src, size_t size)
{
	uint8_t *orig_src, *orig_dst;
	uint8_t *shad_src, *shad_dst;
	msan_orig_t *_src, *_dst;
	size_t i;

	if (__predict_false(!kmsan_enabled))
		return;
	if (__predict_false(kmsan_md_unsupported((vaddr_t)dst)))
		return;
	if (__predict_false(kmsan_md_unsupported((vaddr_t)src))) {
		kmsan_shadow_fill(dst, KMSAN_STATE_INITED, size);
		return;
	}

	shad_src = kmsan_md_addr_to_shad(src);
	shad_dst = kmsan_md_addr_to_shad(dst);
	__builtin_memmove(shad_dst, shad_src, size);

	orig_src = kmsan_md_addr_to_orig(src);
	orig_dst = kmsan_md_addr_to_orig(dst);
	for (i = 0; i < size; i++) {
		_src = (msan_orig_t *)((uintptr_t)orig_src & ~0x3);
		_dst = (msan_orig_t *)((uintptr_t)orig_dst & ~0x3);
		*_dst = *_src;
		orig_src++;
		orig_dst++;
	}
}

static inline void
kmsan_shadow_check(const void *addr, size_t size, const char *hook)
{
	uint8_t *shad;
	size_t i;

	if (__predict_false(!kmsan_enabled))
		return;
	if (__predict_false(kmsan_md_unsupported((vaddr_t)addr)))
		return;

	shad = kmsan_md_addr_to_shad(addr);
	for (i = 0; i < size; i++) {
		if (__predict_true(shad[i] == 0))
			continue;
		kmsan_report_hook((const char *)addr + i, size, i, hook);
		break;
	}
}

void
kmsan_init_arg(size_t n)
{
	msan_lwp_t *lwp;
	uint8_t *arg;

	if (__predict_false(!kmsan_enabled))
		return;
	lwp = curlwp->l_kmsan;
	arg = lwp->tls[lwp->ctx].param;
	__builtin_memset(arg, 0, n);
}

void
kmsan_init_ret(size_t n)
{
	msan_lwp_t *lwp;
	uint8_t *arg;

	if (__predict_false(!kmsan_enabled))
		return;
	lwp = curlwp->l_kmsan;
	arg = lwp->tls[lwp->ctx].retval;
	__builtin_memset(arg, 0, n);
}

static void
kmsan_check_arg(size_t size, const char *hook)
{
	msan_lwp_t *lwp;
	uint8_t *arg;
	size_t i;

	if (__predict_false(!kmsan_enabled))
		return;
	lwp = curlwp->l_kmsan;
	arg = lwp->tls[lwp->ctx].param;

	for (i = 0; i < size; i++) {
		if (__predict_true(arg[i] == 0))
			continue;
		kmsan_report_hook((const char *)arg + i, size, i, hook);
		break;
	}
}

void
kmsan_lwp_alloc(struct lwp *l)
{
	msan_lwp_t *lwp;

	kmsan_init_arg(sizeof(size_t) + sizeof(km_flag_t));
	lwp = kmem_zalloc(sizeof(msan_lwp_t), KM_SLEEP);
	lwp->ctx = 1;

	l->l_kmsan = lwp;
}

void
kmsan_lwp_free(struct lwp *l)
{
	kmsan_init_arg(sizeof(void *) + sizeof(size_t));
	kmem_free(l->l_kmsan, sizeof(msan_lwp_t));
}

void kmsan_intr_enter(void);
void kmsan_intr_leave(void);
void kmsan_softint(struct lwp *);

void
kmsan_intr_enter(void)
{
	msan_lwp_t *lwp;

	if (__predict_false(!kmsan_enabled))
		return;
	lwp = curlwp->l_kmsan;

	lwp->ctx++;
	if (__predict_false(lwp->ctx >= MSAN_NCONTEXT)) {
		kmsan_enabled = false;
		panic("%s: lwp->ctx = %zu", __func__, lwp->ctx);
	}

	kmsan_init_arg(sizeof(void *));
}

void
kmsan_intr_leave(void)
{
	msan_lwp_t *lwp;

	if (__predict_false(!kmsan_enabled))
		return;
	lwp = curlwp->l_kmsan;

	if (__predict_false(lwp->ctx == 0)) {
		kmsan_enabled = false;
		panic("%s: lwp->ctx = %zu", __func__, lwp->ctx);
	}
	lwp->ctx--;
}

void
kmsan_softint(struct lwp *l)
{
	kmsan_init_arg(sizeof(lwp_t *) + sizeof(int));
}

/* -------------------------------------------------------------------------- */

void
kmsan_shadow_map(void *addr, size_t size)
{
	size_t npages, i;
	vaddr_t va;

	KASSERT((vaddr_t)addr % PAGE_SIZE == 0);
	KASSERT(size % PAGE_SIZE == 0);

	npages = size / PAGE_SIZE;

	va = (vaddr_t)kmsan_md_addr_to_shad(addr);
	for (i = 0; i < npages; i++) {
		kmsan_md_shadow_map_page(va + i * PAGE_SIZE);
	}

	va = (vaddr_t)kmsan_md_addr_to_orig(addr);
	for (i = 0; i < npages; i++) {
		kmsan_md_shadow_map_page(va + i * PAGE_SIZE);
	}
}

void
kmsan_orig(void *addr, size_t size, int type, uintptr_t pc)
{
	msan_orig_t orig;

	orig = kmsan_md_orig_encode(type, pc);
	kmsan_origin_fill(addr, orig, size);
}

void
kmsan_mark(void *addr, size_t size, uint8_t c)
{
	kmsan_shadow_fill(addr, c, size);
}

void
kmsan_check_mbuf(void *buf)
{
	struct mbuf *m = buf;

	do {
		kmsan_shadow_check(mtod(m, void *), m->m_len, "MbufChain");
	} while ((m = m->m_next) != NULL);
}

void
kmsan_check_buf(void *buf)
{
	buf_t *bp = buf;

	kmsan_shadow_check(bp->b_data, bp->b_bcount, "bwrite()");
}

void
kmsan_init(void *stack)
{
	/* MD initialization. */
	kmsan_md_init();

	/* Map the stack. */
	kmsan_shadow_map(stack, USPACE);

	/* Initialize the TLS for curlwp. */
	msan_lwp0.ctx = 1;
	curlwp->l_kmsan = &msan_lwp0;

	/* Now officially enabled. */
	kmsan_enabled = true;
}

/* -------------------------------------------------------------------------- */

msan_meta_t __msan_metadata_ptr_for_load_n(void *, size_t);
msan_meta_t __msan_metadata_ptr_for_store_n(void *, size_t);

msan_meta_t __msan_metadata_ptr_for_load_n(void *addr, size_t size)
{
	return kmsan_meta_get(addr, size);
}

msan_meta_t __msan_metadata_ptr_for_store_n(void *addr, size_t size)
{
	return kmsan_meta_get(addr, size);
}

#define MSAN_META_FUNC(size)						\
	msan_meta_t __msan_metadata_ptr_for_load_##size(void *);	\
	msan_meta_t __msan_metadata_ptr_for_load_##size(void *addr)	\
	{								\
		return kmsan_meta_get(addr, size);			\
	}								\
	msan_meta_t __msan_metadata_ptr_for_store_##size(void *);	\
	msan_meta_t __msan_metadata_ptr_for_store_##size(void *addr)	\
	{								\
		return kmsan_meta_get(addr, size);			\
	}

MSAN_META_FUNC(1)
MSAN_META_FUNC(2)
MSAN_META_FUNC(4)
MSAN_META_FUNC(8)

void __msan_instrument_asm_store(void *, size_t);
msan_orig_t __msan_chain_origin(msan_orig_t);
void __msan_poison_alloca(void *, uint64_t, char *);
void __msan_unpoison_alloca(void *, uint64_t);
void __msan_warning(msan_orig_t);
msan_tls_t *__msan_get_context_state(void);

void __msan_instrument_asm_store(void *addr, size_t size)
{
	kmsan_shadow_fill(addr, KMSAN_STATE_INITED, size);
}

msan_orig_t __msan_chain_origin(msan_orig_t origin)
{
	return origin;
}

void __msan_poison_alloca(void *addr, uint64_t size, char *descr)
{
	msan_orig_t orig;

	orig = kmsan_md_orig_encode(KMSAN_TYPE_STACK, (uintptr_t)descr);
	kmsan_origin_fill(addr, orig, size);
	kmsan_shadow_fill(addr, KMSAN_STATE_UNINIT, size);
}

void __msan_unpoison_alloca(void *addr, uint64_t size)
{
	kmsan_shadow_fill(addr, KMSAN_STATE_INITED, size);
}

void __msan_warning(msan_orig_t origin)
{
	if (__predict_false(!kmsan_enabled))
		return;
	kmsan_report_inline(origin, __RET_ADDR);
}

msan_tls_t *__msan_get_context_state(void)
{
	msan_lwp_t *lwp;

	if (__predict_false(!kmsan_enabled))
		return &dummy_tls;
	lwp = curlwp->l_kmsan;

	return &lwp->tls[lwp->ctx];
}

/* -------------------------------------------------------------------------- */

/*
 * Function hooks. Mostly ASM functions which need KMSAN wrappers to handle
 * initialized areas properly.
 */

void *kmsan_memcpy(void *dst, const void *src, size_t len)
{
	/* No kmsan_check_arg, because inlined. */
	kmsan_init_ret(sizeof(void *));
	if (__predict_true(len != 0)) {
		kmsan_meta_copy(dst, src, len);
	}
	return __builtin_memcpy(dst, src, len);
}

int
kmsan_memcmp(const void *b1, const void *b2, size_t len)
{
	const uint8_t *_b1 = b1, *_b2 = b2;
	size_t i;

	kmsan_check_arg(sizeof(b1) + sizeof(b2) + sizeof(len),
	    "memcmp():args");
	kmsan_init_ret(sizeof(int));

	for (i = 0; i < len; i++) {
		if (*_b1 != *_b2) {
			kmsan_shadow_check(b1, i + 1, "memcmp():arg1");
			kmsan_shadow_check(b2, i + 1, "memcmp():arg2");
			return *_b1 - *_b2;
		}
		_b1++, _b2++;
	}

	return 0;
}

void *kmsan_memset(void *dst, int c, size_t len)
{
	/* No kmsan_check_arg, because inlined. */
	kmsan_shadow_fill(dst, KMSAN_STATE_INITED, len);
	kmsan_init_ret(sizeof(void *));
	return __builtin_memset(dst, c, len);
}

void *kmsan_memmove(void *dst, const void *src, size_t len)
{
	/* No kmsan_check_arg, because inlined. */
	if (__predict_true(len != 0)) {
		kmsan_meta_copy(dst, src, len);
	}
	kmsan_init_ret(sizeof(void *));
	return __builtin_memmove(dst, src, len);
}

__strong_alias(__msan_memcpy, kmsan_memcpy)
__strong_alias(__msan_memset, kmsan_memset)
__strong_alias(__msan_memmove, kmsan_memmove)

char *
kmsan_strcpy(char *dst, const char *src)
{
	const char *_src = src;
	char *_dst = dst;
	size_t len = 0;

	kmsan_check_arg(sizeof(dst) + sizeof(src), "strcpy():args");

	while (1) {
		len++;
		*dst = *src;
		if (*src == '\0')
			break;
		src++, dst++;
	}

	kmsan_shadow_check(_src, len, "strcpy():arg2");
	kmsan_shadow_fill(_dst, KMSAN_STATE_INITED, len);
	kmsan_init_ret(sizeof(char *));
	return _dst;
}

int
kmsan_strcmp(const char *s1, const char *s2)
{
	const char *_s1 = s1, *_s2 = s2;
	size_t len = 0;

	kmsan_check_arg(sizeof(s1) + sizeof(s2), "strcmp():args");
	kmsan_init_ret(sizeof(int));

	while (1) {
		len++;
		if (*s1 != *s2)
			break;
		if (*s1 == '\0') {
			kmsan_shadow_check(_s1, len, "strcmp():arg1");
			kmsan_shadow_check(_s2, len, "strcmp():arg2");
			return 0;
		}
		s1++, s2++;
	}

	kmsan_shadow_check(_s1, len, "strcmp():arg1");
	kmsan_shadow_check(_s2, len, "strcmp():arg2");

	return (*(const unsigned char *)s1 - *(const unsigned char *)s2);
}

size_t
kmsan_strlen(const char *str)
{
	const char *s;

	kmsan_check_arg(sizeof(str), "strlen():args");

	s = str;
	while (1) {
		if (*s == '\0')
			break;
		s++;
	}

	kmsan_shadow_check(str, (size_t)(s - str) + 1, "strlen():arg1");
	kmsan_init_ret(sizeof(size_t));
	return (s - str);
}

char *
kmsan_strcat(char *dst, const char *src)
{
	size_t ldst, lsrc;
	char *ret;

	kmsan_check_arg(sizeof(dst) + sizeof(src), "strcat():args");

	ldst = __builtin_strlen(dst);
	lsrc = __builtin_strlen(src);
	kmsan_shadow_check(dst, ldst + 1, "strcat():arg1");
	kmsan_shadow_check(src, lsrc + 1, "strcat():arg2");
	ret = __builtin_strcat(dst, src);
	kmsan_shadow_fill(dst, KMSAN_STATE_INITED, ldst + lsrc + 1);

	kmsan_init_ret(sizeof(char *));
	return ret;
}

char *
kmsan_strchr(const char *s, int c)
{
	char *ret;

	kmsan_check_arg(sizeof(s) + sizeof(c), "strchr():args");
	kmsan_shadow_check(s, __builtin_strlen(s) + 1, "strchr():arg1");
	ret = __builtin_strchr(s, c);

	kmsan_init_ret(sizeof(char *));
	return ret;
}

char *
kmsan_strrchr(const char *s, int c)
{
	char *ret;

	kmsan_check_arg(sizeof(s) + sizeof(c), "strrchr():args");
	kmsan_shadow_check(s, __builtin_strlen(s) + 1, "strrchr():arg1");
	ret = __builtin_strrchr(s, c);

	kmsan_init_ret(sizeof(char *));
	return ret;
}

#undef kcopy
#undef copyin
#undef copyout
#undef copyinstr
#undef copyoutstr

int	kmsan_kcopy(const void *, void *, size_t);
int	kmsan_copyin(const void *, void *, size_t);
int	kmsan_copyout(const void *, void *, size_t);
int	kmsan_copyinstr(const void *, void *, size_t, size_t *);
int	kmsan_copyoutstr(const void *, void *, size_t, size_t *);

int	kcopy(const void *, void *, size_t);
int	copyin(const void *, void *, size_t);
int	copyout(const void *, void *, size_t);
int	copyinstr(const void *, void *, size_t, size_t *);
int	copyoutstr(const void *, void *, size_t, size_t *);

int
kmsan_kcopy(const void *src, void *dst, size_t len)
{
	kmsan_check_arg(sizeof(src) + sizeof(dst) + sizeof(len),
	    "kcopy():args");
	if (__predict_true(len != 0)) {
		kmsan_meta_copy(dst, src, len);
	}
	kmsan_init_ret(sizeof(int));
	return kcopy(src, dst, len);
}

int
kmsan_copyin(const void *uaddr, void *kaddr, size_t len)
{
	int ret;

	kmsan_check_arg(sizeof(uaddr) + sizeof(kaddr) + sizeof(len),
	    "copyin():args");
	ret = copyin(uaddr, kaddr, len);
	if (ret == 0)
		kmsan_shadow_fill(kaddr, KMSAN_STATE_INITED, len);
	kmsan_init_ret(sizeof(int));

	return ret;
}

int
kmsan_copyout(const void *kaddr, void *uaddr, size_t len)
{
	kmsan_check_arg(sizeof(kaddr) + sizeof(uaddr) + sizeof(len),
	    "copyout():args");
	kmsan_shadow_check(kaddr, len, "copyout():arg1");
	kmsan_init_ret(sizeof(int));
	return copyout(kaddr, uaddr, len);
}

int
kmsan_copyinstr(const void *uaddr, void *kaddr, size_t len, size_t *done)
{
	size_t _done;
	int ret;

	kmsan_check_arg(sizeof(uaddr) + sizeof(kaddr) +
	    sizeof(len) + sizeof(done), "copyinstr():args");
	ret = copyinstr(uaddr, kaddr, len, &_done);
	if (ret == 0 || ret == ENAMETOOLONG)
		kmsan_shadow_fill(kaddr, KMSAN_STATE_INITED, _done);
	if (done != NULL) {
		*done = _done;
		kmsan_shadow_fill(done, KMSAN_STATE_INITED, sizeof(size_t));
	}
	kmsan_init_ret(sizeof(int));

	return ret;
}

int
kmsan_copyoutstr(const void *kaddr, void *uaddr, size_t len, size_t *done)
{
	size_t _done;
	int ret;

	kmsan_check_arg(sizeof(kaddr) + sizeof(uaddr) +
	    sizeof(len) + sizeof(done), "copyoutstr():args");
	ret = copyoutstr(kaddr, uaddr, len, &_done);
	kmsan_shadow_check(kaddr, _done, "copyoutstr():arg1");
	if (done != NULL) {
		*done = _done;
		kmsan_shadow_fill(done, KMSAN_STATE_INITED, sizeof(size_t));
	}
	kmsan_init_ret(sizeof(int));

	return ret;
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
#undef _ustore_8
#undef _ustore_16
#undef _ustore_32
#undef _ustore_64

int _ucas_32(volatile uint32_t *, uint32_t, uint32_t, uint32_t *);
int kmsan__ucas_32(volatile uint32_t *, uint32_t, uint32_t, uint32_t *);
int
kmsan__ucas_32(volatile uint32_t *uaddr, uint32_t old, uint32_t new,
    uint32_t *ret)
{
	int _ret;
	kmsan_check_arg(sizeof(uaddr) + sizeof(old) +
	    sizeof(new) + sizeof(ret), "ucas_32():args");
	_ret = _ucas_32(uaddr, old, new, ret);
	if (_ret == 0)
		kmsan_shadow_fill(ret, KMSAN_STATE_INITED, sizeof(*ret));
	kmsan_init_ret(sizeof(int));
	return _ret;
}

#ifdef __HAVE_UCAS_MP
int _ucas_32_mp(volatile uint32_t *, uint32_t, uint32_t, uint32_t *);
int kmsan__ucas_32_mp(volatile uint32_t *, uint32_t, uint32_t, uint32_t *);
int
kmsan__ucas_32_mp(volatile uint32_t *uaddr, uint32_t old, uint32_t new,
    uint32_t *ret)
{
	int _ret;
	kmsan_check_arg(sizeof(uaddr) + sizeof(old) +
	    sizeof(new) + sizeof(ret), "ucas_32_mp():args");
	_ret = _ucas_32_mp(uaddr, old, new, ret);
	if (_ret == 0)
		kmsan_shadow_fill(ret, KMSAN_STATE_INITED, sizeof(*ret));
	kmsan_init_ret(sizeof(int));
	return _ret;
}
#endif

#ifdef _LP64
int _ucas_64(volatile uint64_t *, uint64_t, uint64_t, uint64_t *);
int kmsan__ucas_64(volatile uint64_t *, uint64_t, uint64_t, uint64_t *);
int
kmsan__ucas_64(volatile uint64_t *uaddr, uint64_t old, uint64_t new,
    uint64_t *ret)
{
	int _ret;
	kmsan_check_arg(sizeof(uaddr) + sizeof(old) +
	    sizeof(new) + sizeof(ret), "ucas_64():args");
	_ret = _ucas_64(uaddr, old, new, ret);
	if (_ret == 0)
		kmsan_shadow_fill(ret, KMSAN_STATE_INITED, sizeof(*ret));
	kmsan_init_ret(sizeof(int));
	return _ret;
}

#ifdef __HAVE_UCAS_MP
int _ucas_64_mp(volatile uint64_t *, uint64_t, uint64_t, uint64_t *);
int kmsan__ucas_64_mp(volatile uint64_t *, uint64_t, uint64_t, uint64_t *);
int
kmsan__ucas_64_mp(volatile uint64_t *uaddr, uint64_t old, uint64_t new,
    uint64_t *ret)
{
	int _ret;
	kmsan_check_arg(sizeof(uaddr) + sizeof(old) +
	    sizeof(new) + sizeof(ret), "ucas_64_mp():args");
	_ret = _ucas_64_mp(uaddr, old, new, ret);
	if (_ret == 0)
		kmsan_shadow_fill(ret, KMSAN_STATE_INITED, sizeof(*ret));
	kmsan_init_ret(sizeof(int));
	return _ret;
}
#endif
#endif

int _ufetch_8(const uint8_t *, uint8_t *);
int kmsan__ufetch_8(const uint8_t *, uint8_t *);
int
kmsan__ufetch_8(const uint8_t *uaddr, uint8_t *valp)
{
	int _ret;
	kmsan_check_arg(sizeof(uaddr) + sizeof(valp), "ufetch_8():args");
	_ret = _ufetch_8(uaddr, valp);
	if (_ret == 0)
		kmsan_shadow_fill(valp, KMSAN_STATE_INITED, sizeof(*valp));
	kmsan_init_ret(sizeof(int));
	return _ret;
}

int _ufetch_16(const uint16_t *, uint16_t *);
int kmsan__ufetch_16(const uint16_t *, uint16_t *);
int
kmsan__ufetch_16(const uint16_t *uaddr, uint16_t *valp)
{
	int _ret;
	kmsan_check_arg(sizeof(uaddr) + sizeof(valp), "ufetch_16():args");
	_ret = _ufetch_16(uaddr, valp);
	if (_ret == 0)
		kmsan_shadow_fill(valp, KMSAN_STATE_INITED, sizeof(*valp));
	kmsan_init_ret(sizeof(int));
	return _ret;
}

int _ufetch_32(const uint32_t *, uint32_t *);
int kmsan__ufetch_32(const uint32_t *, uint32_t *);
int
kmsan__ufetch_32(const uint32_t *uaddr, uint32_t *valp)
{
	int _ret;
	kmsan_check_arg(sizeof(uaddr) + sizeof(valp), "ufetch_32():args");
	_ret = _ufetch_32(uaddr, valp);
	if (_ret == 0)
		kmsan_shadow_fill(valp, KMSAN_STATE_INITED, sizeof(*valp));
	kmsan_init_ret(sizeof(int));
	return _ret;
}

#ifdef _LP64
int _ufetch_64(const uint64_t *, uint64_t *);
int kmsan__ufetch_64(const uint64_t *, uint64_t *);
int
kmsan__ufetch_64(const uint64_t *uaddr, uint64_t *valp)
{
	int _ret;
	kmsan_check_arg(sizeof(uaddr) + sizeof(valp), "ufetch_64():args");
	_ret = _ufetch_64(uaddr, valp);
	if (_ret == 0)
		kmsan_shadow_fill(valp, KMSAN_STATE_INITED, sizeof(*valp));
	kmsan_init_ret(sizeof(int));
	return _ret;
}
#endif

int _ustore_8(uint8_t *, uint8_t);
int kmsan__ustore_8(uint8_t *, uint8_t);
int
kmsan__ustore_8(uint8_t *uaddr, uint8_t val)
{
	kmsan_check_arg(sizeof(uaddr) + sizeof(val), "ustore_8():args");
	kmsan_init_ret(sizeof(int));
	return _ustore_8(uaddr, val);
}

int _ustore_16(uint16_t *, uint16_t);
int kmsan__ustore_16(uint16_t *, uint16_t);
int
kmsan__ustore_16(uint16_t *uaddr, uint16_t val)
{
	kmsan_check_arg(sizeof(uaddr) + sizeof(val), "ustore_16():args");
	kmsan_init_ret(sizeof(int));
	return _ustore_16(uaddr, val);
}

int _ustore_32(uint32_t *, uint32_t);
int kmsan__ustore_32(uint32_t *, uint32_t);
int
kmsan__ustore_32(uint32_t *uaddr, uint32_t val)
{
	kmsan_check_arg(sizeof(uaddr) + sizeof(val), "ustore_32():args");
	kmsan_init_ret(sizeof(int));
	return _ustore_32(uaddr, val);
}

#ifdef _LP64
int _ustore_64(uint64_t *, uint64_t);
int kmsan__ustore_64(uint64_t *, uint64_t);
int
kmsan__ustore_64(uint64_t *uaddr, uint64_t val)
{
	kmsan_check_arg(sizeof(uaddr) + sizeof(val), "ustore_64():args");
	kmsan_init_ret(sizeof(int));
	return _ustore_64(uaddr, val);
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

#define MSAN_ATOMIC_FUNC_ADD(name, tret, targ1, targ2) \
	void atomic_add_##name(volatile targ1 *, targ2); \
	void kmsan_atomic_add_##name(volatile targ1 *, targ2); \
	void kmsan_atomic_add_##name(volatile targ1 *ptr, targ2 val) \
	{ \
		kmsan_check_arg(sizeof(ptr) + sizeof(val), \
		    "atomic_add_" #name "():args"); \
		kmsan_shadow_check((const void *)(uintptr_t)ptr, sizeof(tret), \
		    "atomic_add_" #name "():arg1"); \
		atomic_add_##name(ptr, val); \
	} \
	tret atomic_add_##name##_nv(volatile targ1 *, targ2); \
	tret kmsan_atomic_add_##name##_nv(volatile targ1 *, targ2); \
	tret kmsan_atomic_add_##name##_nv(volatile targ1 *ptr, targ2 val) \
	{ \
		kmsan_check_arg(sizeof(ptr) + sizeof(val), \
		    "atomic_add_" #name "_nv():args"); \
		kmsan_shadow_check((const void *)(uintptr_t)ptr, sizeof(tret), \
		    "atomic_add_" #name "_nv():arg1"); \
		kmsan_init_ret(sizeof(tret)); \
		return atomic_add_##name##_nv(ptr, val); \
	}

#define MSAN_ATOMIC_FUNC_AND(name, tret, targ1, targ2) \
	void atomic_and_##name(volatile targ1 *, targ2); \
	void kmsan_atomic_and_##name(volatile targ1 *, targ2); \
	void kmsan_atomic_and_##name(volatile targ1 *ptr, targ2 val) \
	{ \
		kmsan_check_arg(sizeof(ptr) + sizeof(val), \
		    "atomic_and_" #name "():args"); \
		kmsan_shadow_check((const void *)(uintptr_t)ptr, sizeof(tret), \
		    "atomic_and_" #name "():arg1"); \
		atomic_and_##name(ptr, val); \
	} \
	tret atomic_and_##name##_nv(volatile targ1 *, targ2); \
	tret kmsan_atomic_and_##name##_nv(volatile targ1 *, targ2); \
	tret kmsan_atomic_and_##name##_nv(volatile targ1 *ptr, targ2 val) \
	{ \
		kmsan_check_arg(sizeof(ptr) + sizeof(val), \
		    "atomic_and_" #name "_nv():args"); \
		kmsan_shadow_check((const void *)(uintptr_t)ptr, sizeof(tret), \
		    "atomic_and_" #name "_nv():arg1"); \
		kmsan_init_ret(sizeof(tret)); \
		return atomic_and_##name##_nv(ptr, val); \
	}

#define MSAN_ATOMIC_FUNC_OR(name, tret, targ1, targ2) \
	void atomic_or_##name(volatile targ1 *, targ2); \
	void kmsan_atomic_or_##name(volatile targ1 *, targ2); \
	void kmsan_atomic_or_##name(volatile targ1 *ptr, targ2 val) \
	{ \
		kmsan_check_arg(sizeof(ptr) + sizeof(val), \
		    "atomic_or_" #name "():args"); \
		kmsan_shadow_check((const void *)(uintptr_t)ptr, sizeof(tret), \
		    "atomic_or_" #name "():arg1"); \
		atomic_or_##name(ptr, val); \
	} \
	tret atomic_or_##name##_nv(volatile targ1 *, targ2); \
	tret kmsan_atomic_or_##name##_nv(volatile targ1 *, targ2); \
	tret kmsan_atomic_or_##name##_nv(volatile targ1 *ptr, targ2 val) \
	{ \
		kmsan_check_arg(sizeof(ptr) + sizeof(val), \
		    "atomic_or_" #name "_nv():args"); \
		kmsan_shadow_check((const void *)(uintptr_t)ptr, sizeof(tret), \
		    "atomic_or_" #name "_nv():arg1"); \
		kmsan_init_ret(sizeof(tret)); \
		return atomic_or_##name##_nv(ptr, val); \
	}

#define MSAN_ATOMIC_FUNC_CAS(name, tret, targ1, targ2) \
	tret atomic_cas_##name(volatile targ1 *, targ2, targ2); \
	tret kmsan_atomic_cas_##name(volatile targ1 *, targ2, targ2); \
	tret kmsan_atomic_cas_##name(volatile targ1 *ptr, targ2 exp, targ2 new) \
	{ \
		kmsan_check_arg(sizeof(ptr) + sizeof(exp) + sizeof(new), \
		    "atomic_cas_" #name "():args"); \
		kmsan_shadow_check((const void *)(uintptr_t)ptr, sizeof(tret), \
		    "atomic_cas_" #name "():arg1"); \
		kmsan_init_ret(sizeof(tret)); \
		return atomic_cas_##name(ptr, exp, new); \
	} \
	tret atomic_cas_##name##_ni(volatile targ1 *, targ2, targ2); \
	tret kmsan_atomic_cas_##name##_ni(volatile targ1 *, targ2, targ2); \
	tret kmsan_atomic_cas_##name##_ni(volatile targ1 *ptr, targ2 exp, targ2 new) \
	{ \
		kmsan_check_arg(sizeof(ptr) + sizeof(exp) + sizeof(new), \
		    "atomic_cas_" #name "_ni():args"); \
		kmsan_shadow_check((const void *)(uintptr_t)ptr, sizeof(tret), \
		    "atomic_cas_" #name "_ni():arg1"); \
		kmsan_init_ret(sizeof(tret)); \
		return atomic_cas_##name##_ni(ptr, exp, new); \
	}

#define MSAN_ATOMIC_FUNC_SWAP(name, tret, targ1, targ2) \
	tret atomic_swap_##name(volatile targ1 *, targ2); \
	tret kmsan_atomic_swap_##name(volatile targ1 *, targ2); \
	tret kmsan_atomic_swap_##name(volatile targ1 *ptr, targ2 val) \
	{ \
		kmsan_check_arg(sizeof(ptr) + sizeof(val), \
		    "atomic_swap_" #name "():args"); \
		kmsan_shadow_check((const void *)(uintptr_t)ptr, sizeof(tret), \
		    "atomic_swap_" #name "():arg1"); \
		kmsan_init_ret(sizeof(tret)); \
		return atomic_swap_##name(ptr, val); \
	}

#define MSAN_ATOMIC_FUNC_DEC(name, tret, targ1) \
	void atomic_dec_##name(volatile targ1 *); \
	void kmsan_atomic_dec_##name(volatile targ1 *); \
	void kmsan_atomic_dec_##name(volatile targ1 *ptr) \
	{ \
		kmsan_check_arg(sizeof(ptr), \
		    "atomic_dec_" #name "():args"); \
		kmsan_shadow_check((const void *)(uintptr_t)ptr, sizeof(tret), \
		    "atomic_dec_" #name "():arg1"); \
		atomic_dec_##name(ptr); \
	} \
	tret atomic_dec_##name##_nv(volatile targ1 *); \
	tret kmsan_atomic_dec_##name##_nv(volatile targ1 *); \
	tret kmsan_atomic_dec_##name##_nv(volatile targ1 *ptr) \
	{ \
		kmsan_check_arg(sizeof(ptr), \
		    "atomic_dec_" #name "_nv():args"); \
		kmsan_shadow_check((const void *)(uintptr_t)ptr, sizeof(tret), \
		    "atomic_dec_" #name "_nv():arg1"); \
		kmsan_init_ret(sizeof(tret)); \
		return atomic_dec_##name##_nv(ptr); \
	}

#define MSAN_ATOMIC_FUNC_INC(name, tret, targ1) \
	void atomic_inc_##name(volatile targ1 *); \
	void kmsan_atomic_inc_##name(volatile targ1 *); \
	void kmsan_atomic_inc_##name(volatile targ1 *ptr) \
	{ \
		kmsan_check_arg(sizeof(ptr), \
		    "atomic_inc_" #name "():args"); \
		kmsan_shadow_check((const void *)(uintptr_t)ptr, sizeof(tret), \
		    "atomic_inc_" #name "():arg1"); \
		atomic_inc_##name(ptr); \
	} \
	tret atomic_inc_##name##_nv(volatile targ1 *); \
	tret kmsan_atomic_inc_##name##_nv(volatile targ1 *); \
	tret kmsan_atomic_inc_##name##_nv(volatile targ1 *ptr) \
	{ \
		kmsan_check_arg(sizeof(ptr), \
		    "atomic_inc_" #name "_nv():args"); \
		kmsan_shadow_check((const void *)(uintptr_t)ptr, sizeof(tret), \
		    "atomic_inc_" #name "_nv():arg1"); \
		kmsan_init_ret(sizeof(tret)); \
		return atomic_inc_##name##_nv(ptr); \
	}

MSAN_ATOMIC_FUNC_ADD(32, uint32_t, uint32_t, int32_t);
MSAN_ATOMIC_FUNC_ADD(64, uint64_t, uint64_t, int64_t);
MSAN_ATOMIC_FUNC_ADD(int, unsigned int, unsigned int, int);
MSAN_ATOMIC_FUNC_ADD(long, unsigned long, unsigned long, long);
MSAN_ATOMIC_FUNC_ADD(ptr, void *, void, ssize_t);

MSAN_ATOMIC_FUNC_AND(32, uint32_t, uint32_t, uint32_t);
MSAN_ATOMIC_FUNC_AND(64, uint64_t, uint64_t, uint64_t);
MSAN_ATOMIC_FUNC_AND(uint, unsigned int, unsigned int, unsigned int);
MSAN_ATOMIC_FUNC_AND(ulong, unsigned long, unsigned long, unsigned long);

MSAN_ATOMIC_FUNC_OR(32, uint32_t, uint32_t, uint32_t);
MSAN_ATOMIC_FUNC_OR(64, uint64_t, uint64_t, uint64_t);
MSAN_ATOMIC_FUNC_OR(uint, unsigned int, unsigned int, unsigned int);
MSAN_ATOMIC_FUNC_OR(ulong, unsigned long, unsigned long, unsigned long);

MSAN_ATOMIC_FUNC_CAS(32, uint32_t, uint32_t, uint32_t);
MSAN_ATOMIC_FUNC_CAS(64, uint64_t, uint64_t, uint64_t);
MSAN_ATOMIC_FUNC_CAS(uint, unsigned int, unsigned int, unsigned int);
MSAN_ATOMIC_FUNC_CAS(ulong, unsigned long, unsigned long, unsigned long);
MSAN_ATOMIC_FUNC_CAS(ptr, void *, void, void *);

MSAN_ATOMIC_FUNC_SWAP(32, uint32_t, uint32_t, uint32_t);
MSAN_ATOMIC_FUNC_SWAP(64, uint64_t, uint64_t, uint64_t);
MSAN_ATOMIC_FUNC_SWAP(uint, unsigned int, unsigned int, unsigned int);
MSAN_ATOMIC_FUNC_SWAP(ulong, unsigned long, unsigned long, unsigned long);
MSAN_ATOMIC_FUNC_SWAP(ptr, void *, void, void *);

MSAN_ATOMIC_FUNC_DEC(32, uint32_t, uint32_t)
MSAN_ATOMIC_FUNC_DEC(64, uint64_t, uint64_t)
MSAN_ATOMIC_FUNC_DEC(uint, unsigned int, unsigned int);
MSAN_ATOMIC_FUNC_DEC(ulong, unsigned long, unsigned long);
MSAN_ATOMIC_FUNC_DEC(ptr, void *, void);

MSAN_ATOMIC_FUNC_INC(32, uint32_t, uint32_t)
MSAN_ATOMIC_FUNC_INC(64, uint64_t, uint64_t)
MSAN_ATOMIC_FUNC_INC(uint, unsigned int, unsigned int);
MSAN_ATOMIC_FUNC_INC(ulong, unsigned long, unsigned long);
MSAN_ATOMIC_FUNC_INC(ptr, void *, void);

/* -------------------------------------------------------------------------- */

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

#define MSAN_BUS_READ_FUNC(bytes, bits) \
	void bus_space_read_multi_##bytes(bus_space_tag_t, bus_space_handle_t,	\
	    bus_size_t, uint##bits##_t *, bus_size_t);				\
	void kmsan_bus_space_read_multi_##bytes(bus_space_tag_t,		\
	    bus_space_handle_t, bus_size_t, uint##bits##_t *, bus_size_t);	\
	void kmsan_bus_space_read_multi_##bytes(bus_space_tag_t tag,		\
	    bus_space_handle_t hnd, bus_size_t size, uint##bits##_t *buf,	\
	    bus_size_t count)							\
	{									\
		kmsan_shadow_fill(buf, KMSAN_STATE_INITED,			\
		    sizeof(uint##bits##_t) * count);				\
		bus_space_read_multi_##bytes(tag, hnd, size, buf, count);	\
	}									\
	void bus_space_read_multi_stream_##bytes(bus_space_tag_t,		\
	    bus_space_handle_t, bus_size_t, uint##bits##_t *, bus_size_t);	\
	void kmsan_bus_space_read_multi_stream_##bytes(bus_space_tag_t,		\
	    bus_space_handle_t, bus_size_t, uint##bits##_t *, bus_size_t);	\
	void kmsan_bus_space_read_multi_stream_##bytes(bus_space_tag_t tag,	\
	    bus_space_handle_t hnd, bus_size_t size, uint##bits##_t *buf,	\
	    bus_size_t count)							\
	{									\
		kmsan_shadow_fill(buf, KMSAN_STATE_INITED,			\
		    sizeof(uint##bits##_t) * count);				\
		bus_space_read_multi_stream_##bytes(tag, hnd, size, buf, count);\
	}									\
	void bus_space_read_region_##bytes(bus_space_tag_t, bus_space_handle_t,	\
	    bus_size_t, uint##bits##_t *, bus_size_t);				\
	void kmsan_bus_space_read_region_##bytes(bus_space_tag_t,		\
	    bus_space_handle_t, bus_size_t, uint##bits##_t *, bus_size_t);	\
	void kmsan_bus_space_read_region_##bytes(bus_space_tag_t tag,		\
	    bus_space_handle_t hnd, bus_size_t size, uint##bits##_t *buf,	\
	    bus_size_t count)							\
	{									\
		kmsan_shadow_fill(buf, KMSAN_STATE_INITED,			\
		    sizeof(uint##bits##_t) * count);				\
		bus_space_read_region_##bytes(tag, hnd, size, buf, count);	\
	}									\
	void bus_space_read_region_stream_##bytes(bus_space_tag_t,		\
	    bus_space_handle_t, bus_size_t, uint##bits##_t *, bus_size_t);	\
	void kmsan_bus_space_read_region_stream_##bytes(bus_space_tag_t,	\
	    bus_space_handle_t, bus_size_t, uint##bits##_t *, bus_size_t);	\
	void kmsan_bus_space_read_region_stream_##bytes(bus_space_tag_t tag,	\
	    bus_space_handle_t hnd, bus_size_t size, uint##bits##_t *buf,	\
	    bus_size_t count)							\
	{									\
		kmsan_shadow_fill(buf, KMSAN_STATE_INITED,			\
		    sizeof(uint##bits##_t) * count);				\
		bus_space_read_region_stream_##bytes(tag, hnd, size, buf, count);\
	}

MSAN_BUS_READ_FUNC(1, 8)
MSAN_BUS_READ_FUNC(2, 16)
MSAN_BUS_READ_FUNC(4, 32)
MSAN_BUS_READ_FUNC(8, 64)

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

#define MSAN_BUS_WRITE_FUNC(bytes, bits) \
	void bus_space_write_multi_##bytes(bus_space_tag_t, bus_space_handle_t,	\
	    bus_size_t, const uint##bits##_t *, bus_size_t);			\
	void kmsan_bus_space_write_multi_##bytes(bus_space_tag_t,		\
	    bus_space_handle_t, bus_size_t, const uint##bits##_t *, bus_size_t);\
	void kmsan_bus_space_write_multi_##bytes(bus_space_tag_t tag,		\
	    bus_space_handle_t hnd, bus_size_t size, const uint##bits##_t *buf,	\
	    bus_size_t count)							\
	{									\
		kmsan_shadow_check(buf, sizeof(uint##bits##_t) * count,		\
		    "bus_space_write()");					\
		bus_space_write_multi_##bytes(tag, hnd, size, buf, count);	\
	}									\
	void bus_space_write_multi_stream_##bytes(bus_space_tag_t,		\
	    bus_space_handle_t, bus_size_t, const uint##bits##_t *, bus_size_t);\
	void kmsan_bus_space_write_multi_stream_##bytes(bus_space_tag_t,	\
	    bus_space_handle_t, bus_size_t, const uint##bits##_t *, bus_size_t);\
	void kmsan_bus_space_write_multi_stream_##bytes(bus_space_tag_t tag,	\
	    bus_space_handle_t hnd, bus_size_t size, const uint##bits##_t *buf,	\
	    bus_size_t count)							\
	{									\
		kmsan_shadow_check(buf, sizeof(uint##bits##_t) * count,		\
		    "bus_space_write()");					\
		bus_space_write_multi_stream_##bytes(tag, hnd, size, buf, count);\
	}									\
	void bus_space_write_region_##bytes(bus_space_tag_t, bus_space_handle_t,\
	    bus_size_t, const uint##bits##_t *, bus_size_t);			\
	void kmsan_bus_space_write_region_##bytes(bus_space_tag_t,		\
	    bus_space_handle_t, bus_size_t, const uint##bits##_t *, bus_size_t);\
	void kmsan_bus_space_write_region_##bytes(bus_space_tag_t tag,		\
	    bus_space_handle_t hnd, bus_size_t size, const uint##bits##_t *buf,	\
	    bus_size_t count)							\
	{									\
		kmsan_shadow_check(buf, sizeof(uint##bits##_t) * count,		\
		    "bus_space_write()");					\
		bus_space_write_region_##bytes(tag, hnd, size, buf, count);	\
	}									\
	void bus_space_write_region_stream_##bytes(bus_space_tag_t,		\
	    bus_space_handle_t, bus_size_t, const uint##bits##_t *, bus_size_t);\
	void kmsan_bus_space_write_region_stream_##bytes(bus_space_tag_t,	\
	    bus_space_handle_t, bus_size_t, const uint##bits##_t *, bus_size_t);\
	void kmsan_bus_space_write_region_stream_##bytes(bus_space_tag_t tag,	\
	    bus_space_handle_t hnd, bus_size_t size, const uint##bits##_t *buf,	\
	    bus_size_t count)							\
	{									\
		kmsan_shadow_check(buf, sizeof(uint##bits##_t) * count,		\
		    "bus_space_write()");					\
		bus_space_write_region_stream_##bytes(tag, hnd, size, buf, count);\
	}

MSAN_BUS_WRITE_FUNC(1, 8)
MSAN_BUS_WRITE_FUNC(2, 16)
MSAN_BUS_WRITE_FUNC(4, 32)
MSAN_BUS_WRITE_FUNC(8, 64)

/* -------------------------------------------------------------------------- */

#include <sys/mbuf.h>

static void
kmsan_dma_sync_linear(uint8_t *buf, bus_addr_t offset, bus_size_t len,
    bool init, uintptr_t pc)
{
	if (init) {
		kmsan_shadow_fill(buf + offset, KMSAN_STATE_INITED, len);
	} else {
		kmsan_shadow_check(buf + offset, len, "LinearDmaSyncOp");
	}
}

static void
kmsan_dma_sync_mbuf(struct mbuf *m, bus_addr_t offset, bus_size_t len,
    bool init, uintptr_t pc)
{
	bus_addr_t minlen;

	for (; m != NULL && len != 0; m = m->m_next) {
		if (offset >= m->m_len) {
			offset -= m->m_len;
			continue;
		}

		minlen = MIN(len, m->m_len - offset);

		if (init) {
			kmsan_shadow_fill(mtod(m, char *) + offset,
			    KMSAN_STATE_INITED, minlen);
		} else {
			kmsan_shadow_check(mtod(m, char *) + offset,
			    minlen, "MbufDmaSyncOp");
		}

		offset = 0;
		len -= minlen;
	}
}

static void
kmsan_dma_sync_uio(struct uio *uio, bus_addr_t offset, bus_size_t len,
    bool init, uintptr_t pc)
{
	bus_size_t minlen, resid;
	struct iovec *iov;
	int i;

	if (!VMSPACE_IS_KERNEL_P(uio->uio_vmspace))
		return;

	resid = uio->uio_resid;
	iov = uio->uio_iov;

	for (i = 0; i < uio->uio_iovcnt && resid != 0; i++) {
		minlen = MIN(resid, iov[i].iov_len);

		if (init) {
			kmsan_shadow_fill(iov[i].iov_base,
			    KMSAN_STATE_INITED, minlen);
		} else {
			kmsan_shadow_check(iov[i].iov_base, minlen,
			    "UioDmaSyncOp");
		}

		resid -= minlen;
	}
}

void
kmsan_dma_sync(bus_dmamap_t map, bus_addr_t offset, bus_size_t len, int ops)
{
	bool init;

	if ((ops & (BUS_DMASYNC_PREWRITE|BUS_DMASYNC_POSTREAD)) == 0)
		return;
	init = (ops & BUS_DMASYNC_POSTREAD) != 0;

	switch (map->dm_buftype) {
	case KMSAN_DMA_LINEAR:
		kmsan_dma_sync_linear(map->dm_buf, offset, len, init,
		    __RET_ADDR);
		break;
	case KMSAN_DMA_MBUF:
		kmsan_dma_sync_mbuf(map->dm_buf, offset, len, init,
		    __RET_ADDR);
		break;
	case KMSAN_DMA_UIO:
		kmsan_dma_sync_uio(map->dm_buf, offset, len, init,
		    __RET_ADDR);
		break;
	case KMSAN_DMA_RAW:
		break;
	default:
		panic("%s: impossible", __func__);
	}
}

void
kmsan_dma_load(bus_dmamap_t map, void *buf, bus_size_t buflen, int type)
{
	map->dm_buf = buf;
	map->dm_buflen = buflen;
	map->dm_buftype = type;
}

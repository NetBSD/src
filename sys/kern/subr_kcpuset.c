/*	$NetBSD: subr_kcpuset.c,v 1.2 2011/08/07 21:13:05 rmind Exp $	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mindaugas Rasiukevicius.
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
 * Kernel CPU set implementation.
 *
 * Interface can be used by kernel subsystems as a unified dynamic CPU
 * bitset implementation handling many CPUs.  Facility also supports early
 * use by MD code on boot, as it fixups bitsets on further boot.
 *
 * TODO:
 * - Handle "reverse" bitset on fixup/grow.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_kcpuset.c,v 1.2 2011/08/07 21:13:05 rmind Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/sched.h>
#include <sys/kcpuset.h>
#include <sys/pool.h>

/* Number of CPUs to support. */
#define	KC_MAXCPUS		roundup2(MAXCPUS, 32)

/*
 * Structure of dynamic CPU set in the kernel.
 */
struct kcpuset {
	uint32_t		bits[0];
};

typedef struct kcpuset_impl {
	/* Reference count. */
	u_int			kc_refcnt;
	/* Next to free, if non-NULL (used when multiple references). */ 
	struct kcpuset *	kc_next;
	/* Actual variable-sized field of bits. */
	struct kcpuset		kc_field;
} kcpuset_impl_t;

#define	KC_BITS_OFF		(offsetof(struct kcpuset_impl, kc_field))
#define	KC_GETSTRUCT(b)		((kcpuset_impl_t *)((char *)(b) - KC_BITS_OFF))

/* Sizes of a single bitset. */
#define	KC_SHIFT		5
#define	KC_MASK			31

/* An array of noted early kcpuset creations and data. */
#define	KC_SAVE_NITEMS		8

/* Structures for early boot mechanism (must be statically initialised). */
static kcpuset_t **		kc_noted_early[KC_SAVE_NITEMS];
static uint32_t			kc_bits_early[KC_SAVE_NITEMS];
static int			kc_last_idx = 0;
static bool			kc_initialised = false;

#define	KC_BITSIZE_EARLY	sizeof(kc_bits_early[0])
#define	KC_NFIELDS_EARLY	(KC_BITSIZE_EARLY >> KC_SHIFT)

/*
 * The size of whole bitset fields and amount of fields.
 * The whole size must statically initialise for early case.
 */
static size_t			kc_bitsize __read_mostly = KC_BITSIZE_EARLY;
static size_t			kc_nfields __read_mostly = KC_NFIELDS_EARLY;

static pool_cache_t		kc_cache __read_mostly;

static kcpuset_t *		kcpuset_create_raw(void);

/*
 * kcpuset_sysinit: initialize the subsystem, transfer early boot cases
 * to dynamically allocated sets.
 */
void
kcpuset_sysinit(void)
{
	kcpuset_t *kc_dynamic[KC_SAVE_NITEMS], *kcp;
	int i, s;

	/* Set a kcpuset_t sizes. */
	kc_nfields = (KC_MAXCPUS >> KC_SHIFT);
	kc_bitsize = sizeof(uint32_t) * kc_nfields;

	kc_cache = pool_cache_init(sizeof(kcpuset_impl_t) + kc_bitsize,
	    coherency_unit, 0, 0, "kcpuset", NULL, IPL_NONE, NULL, NULL, NULL);

	/* First, pre-allocate kcpuset entries. */
	for (i = 0; i < kc_last_idx; i++) {
		kcp = kcpuset_create_raw();
		kcpuset_zero(kcp);
		kc_dynamic[i] = kcp;
	}

	/*
	 * Prepare to convert all early noted kcpuset uses to dynamic sets.
	 * All processors, except the one we are currently running (primary),
	 * must not be spinned yet.  Since MD facilities can use kcpuset,
	 * raise the IPL to high.
	 */
	KASSERT(mp_online == false);

	s = splhigh();
	for (i = 0; i < kc_last_idx; i++) {
		/*
		 * Transfer the bits from early static storage to the kcpuset.
		 */
		KASSERT(kc_bitsize >= KC_BITSIZE_EARLY);
		memcpy(kc_dynamic[i], &kc_bits_early[i], KC_BITSIZE_EARLY);

		/*
		 * Store the new pointer, pointing to the allocated kcpuset.
		 * Note: we are not in an interrupt context and it is the only
		 * CPU running - thus store is safe (e.g. no need for pointer
		 * variable to be volatile).
		 */
		*kc_noted_early[i] = kc_dynamic[i];
	}
	kc_initialised = true;
	kc_last_idx = 0;
	splx(s);
}

/*
 * kcpuset_early_ptr: note an early boot use by saving the pointer and
 * returning a pointer to a static, temporary bit field.
 */
static kcpuset_t *
kcpuset_early_ptr(kcpuset_t **kcptr)
{
	kcpuset_t *kcp;
	int s;

	s = splhigh();
	if (kc_last_idx < KC_SAVE_NITEMS) {
		/*
		 * Save the pointer, return pointer to static early field.
		 * Need to zero it out.
		 */
		kc_noted_early[kc_last_idx++] = kcptr;
		kcp = (kcpuset_t *)&kc_bits_early[kc_last_idx];
		memset(kcp, 0, KC_BITSIZE_EARLY);
		KASSERT(kc_bitsize == KC_BITSIZE_EARLY);
	} else {
		panic("kcpuset(9): all early-use entries exhausted; "
		    "increase KC_SAVE_NITEMS\n");
	}
	splx(s);

	return kcp;
}

/*
 * Routines to create or destroy the CPU set.
 * Early boot case is handled.
 */

static kcpuset_t *
kcpuset_create_raw(void)
{
	kcpuset_impl_t *kc;

	kc = pool_cache_get(kc_cache, PR_WAITOK);
	kc->kc_refcnt = 1;
	kc->kc_next = NULL;

	/* Note: return pointer to the actual field of bits. */
	KASSERT((uint8_t *)kc + KC_BITS_OFF == (uint8_t *)&kc->kc_field);
	return &kc->kc_field;
}

void
kcpuset_create(kcpuset_t **retkcp)
{

	if (__predict_false(!kc_initialised)) {
		/* Early boot use - special case. */
		*retkcp = kcpuset_early_ptr(retkcp);
		return;
	}
	*retkcp = kcpuset_create_raw();
}

void
kcpuset_destroy(kcpuset_t *kcp)
{
	kcpuset_impl_t *kc;

	KASSERT(kc_initialised);
	KASSERT(kcp != NULL);

	do {
		kc = KC_GETSTRUCT(kcp);
		kcp = kc->kc_next;
		pool_cache_put(kc_cache, kc);
	} while (kcp);
}

/*
 * Routines to copy or reference/unreference the CPU set.
 * Note: early boot case is not supported by these routines.
 */

void
kcpuset_copy(kcpuset_t *dkcp, kcpuset_t *skcp)
{

	KASSERT(kc_initialised);
	KASSERT(KC_GETSTRUCT(dkcp)->kc_refcnt == 1);
	memcpy(dkcp, skcp, kc_bitsize);
}

void
kcpuset_use(kcpuset_t *kcp)
{
	kcpuset_impl_t *kc = KC_GETSTRUCT(kcp);

	KASSERT(kc_initialised);
	atomic_inc_uint(&kc->kc_refcnt);
}

void
kcpuset_unuse(kcpuset_t *kcp, kcpuset_t **lst)
{
	kcpuset_impl_t *kc = KC_GETSTRUCT(kcp);

	KASSERT(kc_initialised);
	KASSERT(kc->kc_refcnt > 0);

	if (atomic_dec_uint_nv(&kc->kc_refcnt) != 0) {
		return;
	}
	KASSERT(kc->kc_next == NULL);
	if (lst == NULL) {
		kcpuset_destroy(kcp);
		return;
	}
	kc->kc_next = *lst;
	*lst = kcp;
}

/*
 * Routines to transfer the CPU set from / to userspace.
 * Note: early boot case is not supported by these routines.
 */

int
kcpuset_copyin(const cpuset_t *ucp, kcpuset_t *kcp, size_t len)
{
	kcpuset_impl_t *kc = KC_GETSTRUCT(kcp);

	KASSERT(kc_initialised);
	KASSERT(kc->kc_refcnt > 0);
	KASSERT(kc->kc_next == NULL);
	(void)kc;

	if (len != kc_bitsize) { /* XXX */
		return EINVAL;
	}
	return copyin(ucp, kcp, kc_bitsize);
}

int
kcpuset_copyout(kcpuset_t *kcp, cpuset_t *ucp, size_t len)
{
	kcpuset_impl_t *kc = KC_GETSTRUCT(kcp);

	KASSERT(kc_initialised);
	KASSERT(kc->kc_refcnt > 0);
	KASSERT(kc->kc_next == NULL);
	(void)kc;

	if (len != kc_bitsize) { /* XXX */
		return EINVAL;
	}
	return copyout(kcp, ucp, kc_bitsize);
}

/*
 * Routines to change bit field - zero, fill, set, unset, etc.
 */
 
void
kcpuset_zero(kcpuset_t *kcp)
{

	KASSERT(!kc_initialised || KC_GETSTRUCT(kcp)->kc_refcnt > 0);
	KASSERT(!kc_initialised || KC_GETSTRUCT(kcp)->kc_next == NULL);
	memset(kcp, 0, kc_bitsize);
}

void
kcpuset_fill(kcpuset_t *kcp)
{

	KASSERT(!kc_initialised || KC_GETSTRUCT(kcp)->kc_refcnt > 0);
	KASSERT(!kc_initialised || KC_GETSTRUCT(kcp)->kc_next == NULL);
	memset(kcp, ~0, kc_bitsize);
}

void
kcpuset_set(kcpuset_t *kcp, cpuid_t i)
{
	const size_t j = i >> KC_SHIFT;

	KASSERT(!kc_initialised || KC_GETSTRUCT(kcp)->kc_next == NULL);
	KASSERT(j < kc_nfields);

	kcp->bits[j] |= 1 << (i & KC_MASK);
}

void
kcpuset_clear(kcpuset_t *kcp, cpuid_t i)
{
	const size_t j = i >> KC_SHIFT;

	KASSERT(!kc_initialised || KC_GETSTRUCT(kcp)->kc_next == NULL);
	KASSERT(j < kc_nfields);

	kcp->bits[j] &= ~(1 << (i & KC_MASK));
}

int
kcpuset_isset(kcpuset_t *kcp, cpuid_t i)
{
	const size_t j = i >> KC_SHIFT;

	KASSERT(kcp != NULL);
	KASSERT(!kc_initialised || KC_GETSTRUCT(kcp)->kc_refcnt > 0);
	KASSERT(!kc_initialised || KC_GETSTRUCT(kcp)->kc_next == NULL);
	KASSERT(j < kc_nfields);

	return ((1 << (i & KC_MASK)) & kcp->bits[j]) != 0;
}

bool
kcpuset_iszero(kcpuset_t *kcp)
{

	for (size_t j = 0; j < kc_nfields; j++) {
		if (kcp->bits[j] != 0) {
			return false;
		}
	}
	return true;
}

bool
kcpuset_match(const kcpuset_t *kcp1, const kcpuset_t *kcp2)
{

	return memcmp(kcp1, kcp2, kc_bitsize) == 0;
}

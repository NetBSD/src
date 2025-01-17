// SPDX-FileCopyrightText: 2009 Novell Inc.
// SPDX-FileCopyrightText: 2010 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-only

#ifndef _URCU_REF_H
#define _URCU_REF_H

/*
 * Userspace RCU - Reference counting
 *
 * Author: Jan Blunck <jblunck@suse.de>
 */

#include <stdbool.h>
#include <limits.h>
#include <stdlib.h>
#include <urcu/assert.h>
#include <urcu/uatomic.h>

struct urcu_ref {
	long refcount; /* ATOMIC */
};

static inline void urcu_ref_set(struct urcu_ref *ref, long val)
{
	uatomic_set(&ref->refcount, val);
}

static inline void urcu_ref_init(struct urcu_ref *ref)
{
	urcu_ref_set(ref, 1);
}

static inline bool  __attribute__((warn_unused_result))
		urcu_ref_get_safe(struct urcu_ref *ref)
{
	long old, _new, res;

	old = uatomic_read(&ref->refcount);
	for (;;) {
		if (old == LONG_MAX) {
			return false;	/* Failure. */
		}
		_new = old + 1;
		res = uatomic_cmpxchg(&ref->refcount, old, _new);
		if (res == old) {
			return true;	/* Success. */
		}
		old = res;
	}
}

static inline void urcu_ref_get(struct urcu_ref *ref)
{
	if (!urcu_ref_get_safe(ref))
		abort();
}

static inline void urcu_ref_put(struct urcu_ref *ref,
				void (*release)(struct urcu_ref *))
{
	long res = uatomic_sub_return(&ref->refcount, 1);
	urcu_posix_assert(res >= 0);
	if (res == 0)
		release(ref);
}

/*
 * urcu_ref_get_unless_zero
 *
 * Allows getting a reference atomically if the reference count is not
 * zero. Returns true if the reference is taken, false otherwise. This
 * needs to be used in conjunction with another synchronization
 * technique (e.g.  RCU or mutex) to ensure existence of the reference
 * count. False is also returned in case incrementing the refcount would
 * result in an overflow.
 */
static inline bool urcu_ref_get_unless_zero(struct urcu_ref *ref)
{
	long old, _new, res;

	old = uatomic_read(&ref->refcount);
	for (;;) {
		if (old == 0 || old == LONG_MAX)
			return false;	/* Failure. */
		_new = old + 1;
		res = uatomic_cmpxchg(&ref->refcount, old, _new);
		if (res == old) {
			return true;	/* Success. */
		}
		old = res;
	}
}

#endif /* _URCU_REF_H */

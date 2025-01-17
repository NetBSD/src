// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
// SPDX-FileCopyrightText: 2009 Paul E. McKenney, IBM Corporation.
//
// SPDX-License-Identifier: LGPL-2.1-or-later

/*
 * library wrappers to be used by non-LGPL compatible source code.
 *
 * IBM's contributions to this file may be relicensed under LGPLv2 or later.
 */

#include <urcu/uatomic.h>

#include <urcu/static/pointer.h>
/* Do not #define _LGPL_SOURCE to ensure we can emit the wrapper symbols */
#include <urcu/pointer.h>

extern void synchronize_rcu(void);

void *rcu_dereference_sym(void *p)
{
	return _rcu_dereference(p);
}

void *rcu_set_pointer_sym(void **p, void *v)
{
	uatomic_store(p, v, CMM_RELEASE);
	return v;
}

void *rcu_xchg_pointer_sym(void **p, void *v)
{
	return uatomic_xchg_mo(p, v, CMM_SEQ_CST);
}

void *rcu_cmpxchg_pointer_sym(void **p, void *old, void *_new)
{
	return uatomic_cmpxchg_mo(p, old, _new, CMM_SEQ_CST, CMM_RELAXED);
}

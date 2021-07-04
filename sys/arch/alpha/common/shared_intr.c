/* $NetBSD: shared_intr.c,v 1.29 2021/07/04 22:36:43 thorpej Exp $ */

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * Copyright (c) 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Authors: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * Common shared-interrupt-line functionality.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: shared_intr.c,v 1.29 2021/07/04 22:36:43 thorpej Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/queue.h>
#include <sys/atomic.h>
#include <sys/intr.h>
#include <sys/xcall.h>

static const char *
intr_typename(int type)
{

	switch (type) {
	case IST_UNUSABLE:
		return ("disabled");
	case IST_NONE:
		return ("none");
	case IST_PULSE:
		return ("pulsed");
	case IST_EDGE:
		return ("edge-triggered");
	case IST_LEVEL:
		return ("level-triggered");
	}
	panic("intr_typename: unknown type %d", type);
}

struct alpha_shared_intr *
alpha_shared_intr_alloc(unsigned int n)
{
	struct alpha_shared_intr *intr;
	unsigned int i;

	KASSERT(n != 0);

	intr = kmem_alloc(n * sizeof(*intr), KM_SLEEP);
	for (i = 0; i < n; i++) {
		TAILQ_INIT(&intr[i].intr_q);
		intr[i].intr_sharetype = IST_NONE;
		intr[i].intr_dfltsharetype = IST_NONE;
		intr[i].intr_nstrays = 0;
		intr[i].intr_maxstrays = 0;
		intr[i].intr_private = NULL;
		intr[i].intr_cpu = NULL;
		intr[i].intr_string = kmem_asprintf("irq %u", i);
	}

	return (intr);
}

int
alpha_shared_intr_dispatch(struct alpha_shared_intr *intr, unsigned int num)
{
	struct alpha_shared_intrhand *ih;
	int rv, handled;

	atomic_add_long(&intr[num].intr_evcnt.ev_count, 1);

	ih = intr[num].intr_q.tqh_first;
	handled = 0;
	while (ih != NULL) {

		/*
		 * The handler returns one of three values:
		 *   0:	This interrupt wasn't for me.
		 *   1: This interrupt was for me.
		 *  -1: This interrupt might have been for me, but I can't say
		 *      for sure.
		 */

		rv = (*ih->ih_fn)(ih->ih_arg);

		handled = handled || (rv != 0);
		ih = ih->ih_q.tqe_next;
	}

	return (handled);
}

static int
alpha_shared_intr_wrapper(void * const arg)
{
	struct alpha_shared_intrhand * const ih = arg;
	int rv;

	KERNEL_LOCK(1, NULL);
	rv = (*ih->ih_real_fn)(ih->ih_real_arg);
	KERNEL_UNLOCK_ONE(NULL);

	return rv;
}

struct alpha_shared_intrhand *
alpha_shared_intr_alloc_intrhand(struct alpha_shared_intr *intr,
    unsigned int num, int type, int level, int flags,
    int (*fn)(void *), void *arg, const char *basename)
{
	struct alpha_shared_intrhand *ih;

	if (intr[num].intr_sharetype == IST_UNUSABLE) {
		printf("%s: %s irq %d: unusable\n", __func__,
		    basename, num);
		return NULL;
	}

	KASSERT(type != IST_NONE);

	ih = kmem_alloc(sizeof(*ih), KM_SLEEP);

	ih->ih_intrhead = intr;
	ih->ih_fn = ih->ih_real_fn = fn;
	ih->ih_arg = ih->ih_real_arg = arg;
	ih->ih_level = level;
	ih->ih_type = type;
	ih->ih_num = num;

	/*
	 * Non-MPSAFE interrupts get a wrapper that takes the
	 * KERNEL_LOCK.
	 */
	if ((flags & ALPHA_INTR_MPSAFE) == 0) {
		ih->ih_fn = alpha_shared_intr_wrapper;
		ih->ih_arg = ih;
	}

	return (ih);
}

void
alpha_shared_intr_free_intrhand(struct alpha_shared_intrhand *ih)
{

	kmem_free(ih, sizeof(*ih));
}

static void
alpha_shared_intr_link_unlink_xcall(void *arg1, void *arg2)
{
	struct alpha_shared_intrhand *ih = arg1;
	struct alpha_shared_intr *intr = ih->ih_intrhead;
	unsigned int num = ih->ih_num;

	struct cpu_info *ci = intr[num].intr_cpu;

	KASSERT(ci != NULL);
	KASSERT(ci == curcpu() || !mp_online);
	KASSERT(!cpu_intr_p());

	const unsigned long psl = alpha_pal_swpipl(ALPHA_PSL_IPL_HIGH);

	if (arg2 != NULL) {
		TAILQ_INSERT_TAIL(&intr[num].intr_q, ih, ih_q);
		ci->ci_nintrhand++;
	} else {
		TAILQ_REMOVE(&intr[num].intr_q, ih, ih_q);
		ci->ci_nintrhand--;
	}

	alpha_pal_swpipl(psl);
}

bool
alpha_shared_intr_link(struct alpha_shared_intr *intr,
    struct alpha_shared_intrhand *ih, const char *basename)
{
	int type = ih->ih_type;
	unsigned int num = ih->ih_num;

	KASSERT(mutex_owned(&cpu_lock));
	KASSERT(ih->ih_intrhead == intr);

	switch (intr[num].intr_sharetype) {
	case IST_EDGE:
	case IST_LEVEL:
		if (type == intr[num].intr_sharetype)
			break;
	case IST_PULSE:
		if (type != IST_NONE) {
			if (intr[num].intr_q.tqh_first == NULL) {
				printf("alpha_shared_intr_establish: %s irq %d: warning: using %s on %s\n",
				    basename, num, intr_typename(type),
				    intr_typename(intr[num].intr_sharetype));
				type = intr[num].intr_sharetype;
			} else {
				printf("alpha_shared_intr_establish: %s irq %d: can't share %s with %s\n",
				    basename, num, intr_typename(type),
				    intr_typename(intr[num].intr_sharetype));
				return (false);
			}
		}
		break;

	case IST_NONE:
		/* not currently used; safe */
		break;
	}

	intr[num].intr_sharetype = type;

	/*
	 * If a CPU hasn't been assigned yet, just give it to the
	 * primary.
	 */
	if (intr[num].intr_cpu == NULL) {
		intr[num].intr_cpu = &cpu_info_primary;
	}

	kpreempt_disable();
	if (intr[num].intr_cpu == curcpu() || !mp_online) {
		alpha_shared_intr_link_unlink_xcall(ih, ih);
	} else {
		uint64_t where = xc_unicast(XC_HIGHPRI,
		    alpha_shared_intr_link_unlink_xcall, ih, ih,
		    intr->intr_cpu);
		xc_wait(where);
	}
	kpreempt_enable();

	return (true);
}

void
alpha_shared_intr_unlink(struct alpha_shared_intr *intr,
    struct alpha_shared_intrhand *ih, const char *basename)
{
	unsigned int num = ih->ih_num;

	KASSERT(mutex_owned(&cpu_lock));

	kpreempt_disable();
	if (intr[num].intr_cpu == curcpu() || !mp_online) {
		alpha_shared_intr_link_unlink_xcall(ih, NULL);
	} else {
		uint64_t where = xc_unicast(XC_HIGHPRI,
		    alpha_shared_intr_link_unlink_xcall, ih, NULL,
		    intr->intr_cpu);
		xc_wait(where);
	}
	kpreempt_enable();
}

int
alpha_shared_intr_get_sharetype(struct alpha_shared_intr *intr,
    unsigned int num)
{

	return (intr[num].intr_sharetype);
}

int
alpha_shared_intr_isactive(struct alpha_shared_intr *intr, unsigned int num)
{

	return (intr[num].intr_q.tqh_first != NULL);
}

int
alpha_shared_intr_firstactive(struct alpha_shared_intr *intr, unsigned int num)
{

	return (intr[num].intr_q.tqh_first != NULL &&
		intr[num].intr_q.tqh_first->ih_q.tqe_next == NULL);
}

void
alpha_shared_intr_set_dfltsharetype(struct alpha_shared_intr *intr,
    unsigned int num, int newdfltsharetype)
{

#ifdef DIAGNOSTIC
	if (alpha_shared_intr_isactive(intr, num))
		panic("alpha_shared_intr_set_dfltsharetype on active intr");
#endif

	intr[num].intr_dfltsharetype = newdfltsharetype;
	intr[num].intr_sharetype = intr[num].intr_dfltsharetype;
}

void
alpha_shared_intr_set_maxstrays(struct alpha_shared_intr *intr,
    unsigned int num, int newmaxstrays)
{
	int s = splhigh();
	intr[num].intr_maxstrays = newmaxstrays;
	intr[num].intr_nstrays = 0;
	splx(s);
}

void
alpha_shared_intr_reset_strays(struct alpha_shared_intr *intr,
    unsigned int num)
{

	/*
	 * Don't bother blocking interrupts; this doesn't have to be
	 * precise, but it does need to be fast.
	 */
	intr[num].intr_nstrays = 0;
}

void
alpha_shared_intr_stray(struct alpha_shared_intr *intr, unsigned int num,
    const char *basename)
{

	intr[num].intr_nstrays++;

	if (intr[num].intr_maxstrays == 0)
		return;

	if (intr[num].intr_nstrays <= intr[num].intr_maxstrays)
		log(LOG_ERR, "stray %s irq %d%s\n", basename, num,
		    intr[num].intr_nstrays >= intr[num].intr_maxstrays ?
		      "; stopped logging" : "");
}

void
alpha_shared_intr_set_private(struct alpha_shared_intr *intr,
    unsigned int num, void *v)
{

	intr[num].intr_private = v;
}

void *
alpha_shared_intr_get_private(struct alpha_shared_intr *intr,
    unsigned int num)
{

	return (intr[num].intr_private);
}

static unsigned int
alpha_shared_intr_q_count_handlers(struct alpha_shared_intr *intr_q)
{
	unsigned int cnt = 0;
	struct alpha_shared_intrhand *ih;

	TAILQ_FOREACH(ih, &intr_q->intr_q, ih_q) {
		cnt++;
	}

	return cnt;
}

static void
alpha_shared_intr_set_cpu_xcall(void *arg1, void *arg2)
{
	struct alpha_shared_intr *intr_q = arg1;
	struct cpu_info *ci = arg2;
	unsigned int cnt = alpha_shared_intr_q_count_handlers(intr_q);

	KASSERT(ci == curcpu() || !mp_online);

	ci->ci_nintrhand += cnt;
	KASSERT(cnt <= ci->ci_nintrhand);
}

static void
alpha_shared_intr_unset_cpu_xcall(void *arg1, void *arg2)
{
	struct alpha_shared_intr *intr_q = arg1;
	struct cpu_info *ci = arg2;
	unsigned int cnt = alpha_shared_intr_q_count_handlers(intr_q);

	KASSERT(ci == curcpu() || !mp_online);

	KASSERT(cnt <= ci->ci_nintrhand);
	ci->ci_nintrhand -= cnt;
}

void
alpha_shared_intr_set_cpu(struct alpha_shared_intr *intr, unsigned int num,
    struct cpu_info *ci)
{
	struct cpu_info *old_ci;

	KASSERT(mutex_owned(&cpu_lock));

	old_ci = intr[num].intr_cpu;
	intr[num].intr_cpu = ci;

	if (old_ci != NULL && old_ci != ci) {
		kpreempt_disable();

		if (ci == curcpu() || !mp_online) {
			alpha_shared_intr_set_cpu_xcall(&intr[num], ci);
		} else {
			uint64_t where = xc_unicast(XC_HIGHPRI,
			    alpha_shared_intr_set_cpu_xcall, &intr[num],
			    ci, ci);
			xc_wait(where);
		}

		if (old_ci == curcpu() || !mp_online) {
			alpha_shared_intr_unset_cpu_xcall(&intr[num], old_ci);
		} else {
			uint64_t where = xc_unicast(XC_HIGHPRI,
			    alpha_shared_intr_unset_cpu_xcall, &intr[num],
			    old_ci, old_ci);
			xc_wait(where);
		}

		kpreempt_enable();
	}
}

struct cpu_info *
alpha_shared_intr_get_cpu(struct alpha_shared_intr *intr, unsigned int num)
{

	return (intr[num].intr_cpu);
}

struct evcnt *
alpha_shared_intr_evcnt(struct alpha_shared_intr *intr,
    unsigned int num)
{

	return (&intr[num].intr_evcnt);
}

void
alpha_shared_intr_set_string(struct alpha_shared_intr *intr,
    unsigned int num, char *str)
{
	char *ostr = intr[num].intr_string;
	intr[num].intr_string = str;
	kmem_strfree(ostr);
}

const char *
alpha_shared_intr_string(struct alpha_shared_intr *intr,
    unsigned int num)
{

	return (intr[num].intr_string);
}

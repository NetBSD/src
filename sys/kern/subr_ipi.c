/*	$NetBSD: subr_ipi.c,v 1.1 2014/05/19 22:47:54 rmind Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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
 * Inter-processor interrupt (IPI) interface with cross-call support.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_ipi.c,v 1.1 2014/05/19 22:47:54 rmind Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/evcnt.h>
#include <sys/cpu.h>
#include <sys/ipi.h>
#include <sys/kcpuset.h>
#include <sys/kmem.h>
#include <sys/lock.h>

/*
 * Per-CPU mailbox for IPI messages: it is a single cache line storing
 * up to IPI_MSG_MAX messages.
 */

#define	IPI_MSG_SLOTS	(CACHE_LINE_SIZE / sizeof(ipi_msg_t *))
#define	IPI_MSG_MAX	IPI_MSG_SLOTS

typedef struct {
	ipi_msg_t *	msg[IPI_MSG_SLOTS];
} ipi_mbox_t;

static ipi_mbox_t *	ipi_mboxes	__read_mostly;
static struct evcnt	ipi_mboxfull_ev	__cacheline_aligned;

#ifndef MULTIPROCESSOR
#define	cpu_ipi(ci)	KASSERT(ci == NULL)
#endif

void
ipi_sysinit(void)
{
	const size_t len = ncpu * sizeof(ipi_mbox_t);

	/* Allocate per-CPU IPI mailboxes. */
	ipi_mboxes = kmem_zalloc(len, KM_SLEEP);
	KASSERT(ipi_mboxes != NULL);

	evcnt_attach_dynamic(&ipi_mboxfull_ev, EVCNT_TYPE_MISC, NULL,
	   "ipi", "full");
}

/*
 * put_msg: insert message into the mailbox.
 */
static inline void
put_msg(ipi_mbox_t *mbox, ipi_msg_t *msg)
{
	int count = SPINLOCK_BACKOFF_MIN;
again:
	for (u_int i = 0; i < IPI_MSG_MAX; i++) {
		if (__predict_true(mbox->msg[i] == NULL) &&
		    atomic_cas_ptr(&mbox->msg[i], NULL, msg) == NULL) {
			return;
		}
	}

	/* All slots are full: we have to spin-wait. */
	ipi_mboxfull_ev.ev_count++;
	SPINLOCK_BACKOFF(count);
	goto again;
}

/*
 * ipi_cpu_handler: the IPI handler.
 */
void
ipi_cpu_handler(void)
{
	const struct cpu_info * const ci = curcpu();
	ipi_mbox_t *mbox = &ipi_mboxes[cpu_index(ci)];

	KASSERT(curcpu() == ci);

	for (u_int i = 0; i < IPI_MSG_MAX; i++) {
		ipi_msg_t *msg;

		/* Get the message. */
		if ((msg = mbox->msg[i]) == NULL) {
			continue;
		}
		mbox->msg[i] = NULL;

		/* Execute the handler. */
		KASSERT(msg->func);
		msg->func(msg->arg);

		/* Ack the request. */
		atomic_dec_uint(&msg->_pending);
	}
}

/*
 * ipi_unicast: send an IPI to a single CPU.
 *
 * => The CPU must be remote; must not be local.
 * => The caller must ipi_wait() on the message for completion.
 */
void
ipi_unicast(ipi_msg_t *msg, struct cpu_info *ci)
{
	const cpuid_t id = cpu_index(ci);

	KASSERT(msg->func != NULL);
	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != ci);

	msg->_pending = 1;
	membar_producer();

	put_msg(&ipi_mboxes[id], msg);
	cpu_ipi(ci);
}

/*
 * ipi_multicast: send an IPI to each CPU in the specified set.
 *
 * => The caller must ipi_wait() on the message for completion.
 */
void
ipi_multicast(ipi_msg_t *msg, const kcpuset_t *target)
{
	const struct cpu_info * const self = curcpu();
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	u_int local;

	KASSERT(msg->func != NULL);
	KASSERT(kpreempt_disabled());

	local = !!kcpuset_isset(target, cpu_index(self));
	msg->_pending = kcpuset_countset(target) - local;
	membar_producer();

	for (CPU_INFO_FOREACH(cii, ci)) {
		cpuid_t id;

		if (__predict_false(ci == self)) {
			continue;
		}
		id = cpu_index(ci);
		if (!kcpuset_isset(target, id)) {
			continue;
		}
		put_msg(&ipi_mboxes[id], msg);
		cpu_ipi(ci);
	}
	if (local) {
		msg->func(msg->arg);
	}
}

/*
 * ipi_broadcast: send an IPI to all CPUs.
 *
 * => The caller must ipi_wait() on the message for completion.
 */
void
ipi_broadcast(ipi_msg_t *msg)
{
	const struct cpu_info * const self = curcpu();
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	KASSERT(msg->func != NULL);
	KASSERT(kpreempt_disabled());

	msg->_pending = ncpu - 1;
	membar_producer();

	/* Broadcast IPIs for remote CPUs. */
	for (CPU_INFO_FOREACH(cii, ci)) {
		cpuid_t id;

		if (__predict_false(ci == self)) {
			continue;
		}
		id = cpu_index(ci);
		put_msg(&ipi_mboxes[id], msg);
	}
	cpu_ipi(NULL);

	/* Finally, execute locally. */
	msg->func(msg->arg);
}

/*
 * ipi_wait: spin-wait until the message is processed.
 */
void
ipi_wait(ipi_msg_t *msg)
{
	int count = SPINLOCK_BACKOFF_MIN;

	while (msg->_pending) {
		KASSERT(msg->_pending < ncpu);
		SPINLOCK_BACKOFF(count);
	}
}

/*	$NetBSD: linux_irq_work.c,v 1.1 2021/12/19 11:49:57 riastradh Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
__KERNEL_RCSID(0, "$NetBSD: linux_irq_work.c,v 1.1 2021/12/19 11:49:57 riastradh Exp $");

#include <sys/param.h>

#include <sys/atomic.h>
#include <sys/intr.h>
#include <sys/mutex.h>
#include <sys/percpu.h>
#include <sys/queue.h>

#include <linux/irq_work.h>

struct irq_work_cpu {
	kmutex_t			iwc_lock;
	SIMPLEQ_HEAD(, irq_work)	iwc_todo;
};

enum {
	IRQ_WORK_PENDING = 1,
};

static struct percpu *irq_work_percpu;
static void *irq_work_sih __read_mostly;

static void
irq_work_intr(void *cookie)
{
	struct irq_work_cpu *iwc;
	SIMPLEQ_HEAD(, irq_work) todo = SIMPLEQ_HEAD_INITIALIZER(todo);
	struct irq_work *iw, *next;

	iwc = percpu_getref(irq_work_percpu);
	mutex_spin_enter(&iwc->iwc_lock);
	SIMPLEQ_CONCAT(&todo, &iwc->iwc_todo);
	mutex_spin_exit(&iwc->iwc_lock);
	percpu_putref(irq_work_percpu);

	SIMPLEQ_FOREACH_SAFE(iw, &todo, iw_entry, next) {
		atomic_store_relaxed(&iw->iw_flags, 0);
		(*iw->func)(iw);
	}
}

static void
irq_work_cpu_init(void *ptr, void *cookie, struct cpu_info *ci)
{
	struct irq_work_cpu *iwc = ptr;

	mutex_init(&iwc->iwc_lock, MUTEX_DEFAULT, IPL_HIGH);
	SIMPLEQ_INIT(&iwc->iwc_todo);
}

static void
irq_work_cpu_fini(void *ptr, void *cookie, struct cpu_info *ci)
{
	struct irq_work_cpu *iwc __diagused = ptr;

	KASSERT(SIMPLEQ_EMPTY(&iwc->iwc_todo));
	mutex_destroy(&iwc->iwc_lock);
}

void
linux_irq_work_init(void)
{

	irq_work_percpu = percpu_create(sizeof(struct irq_work_cpu),
	    irq_work_cpu_init, irq_work_cpu_fini, NULL);
	irq_work_sih = softint_establish(SOFTINT_SERIAL|SOFTINT_MPSAFE,
	    irq_work_intr, NULL);
}

void
linux_irq_work_fini(void)
{

	softint_disestablish(irq_work_sih);
	percpu_free(irq_work_percpu, sizeof(struct irq_work_cpu));
}

void
init_irq_work(struct irq_work *iw, void (*func)(struct irq_work *))
{

	iw->iw_flags = 0;
	iw->func = func;
}

bool
irq_work_queue(struct irq_work *iw)
{
	struct irq_work_cpu *iwc;

	if (atomic_swap_uint(&iw->iw_flags, IRQ_WORK_PENDING)
	    & IRQ_WORK_PENDING)
		return false;

	iwc = percpu_getref(irq_work_percpu);
	mutex_spin_enter(&iwc->iwc_lock);
	SIMPLEQ_INSERT_TAIL(&iwc->iwc_todo, iw, iw_entry);
	mutex_spin_exit(&iwc->iwc_lock);
	softint_schedule(irq_work_sih);
	percpu_putref(irq_work_percpu);

	return true;
}

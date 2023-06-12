/*	$NetBSD: interrupt.c,v 1.2 2023/06/12 19:04:14 skrll Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Simon Burge and Nick Hudson.
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

#include "opt_multiprocessor.h"

#include <sys/cdefs.h>

__RCSID("$NetBSD: interrupt.c,v 1.2 2023/06/12 19:04:14 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/kernel.h>

#include <machine/locore.h>
#include <machine/machdep.h>
#include <machine/sbi.h>

#include <riscv/dev/plicvar.h>


static void
riscv_intr_default_handler(struct trapframe *frame, register_t epc,
    register_t status, register_t cause)
{
#if 1
	panic("not supposed to get here");
#else
	struct cpu_info * const ci = curcpu();
	const int code = CAUSE_CODE(cause);

	KASSERT(CAUSE_INTERRUPT_P(cause));

	ci->ci_intr_depth++;
	switch (code) {
	case IRQ_SUPERVISOR_SOFTWARE:
#ifdef MULTIPROCESSOR
		ipi_handler(tf);
#else
		panic("%s: SUPERVISOR SOFTWARE interrupt", __func__);
#endif
		break;
	case IRQ_SUPERVISOR_TIMER: {
		struct clockframe cf = {
			.cf_epc = epc,
			.cf_status = status,
			.cf_intr_depth = ci->ci_intr_depth
		};
		timer_handler(&cf);
		break;
	    }
	case IRQ_SUPERVISOR_EXTERNAL:
		extintr_handler(tf);
		break;
	default:
		panic("%s: unknown exception code %u", __func__, code);
	}
	ci->ci_intr_depth--;
#endif
}


static void (*_riscv_intr_handler)(struct trapframe *, register_t,
    register_t, register_t) = riscv_intr_default_handler;


void
riscv_intr_set_handler(void (*intr_handler)(struct trapframe *, register_t,
    register_t, register_t))
{
	KASSERT(_riscv_intr_handler == riscv_intr_default_handler ||
		_riscv_intr_handler == intr_handler);
	_riscv_intr_handler = intr_handler;
}


void
cpu_intr(struct trapframe *tf, register_t epc, register_t status,
    register_t cause)
{
	_riscv_intr_handler(tf, epc, status, cause);
}


static void *
intr_establish_xname(int irq, int ipl, int type, int (*func)(void *), void *arg,
    const char *xname)
{
	KASSERT(!cpu_intr_p());
	KASSERT(!cpu_softintr_p());

	return plic_intr_establish_xname(irq, ipl, type, func, arg, xname);
}

void *
intr_establish(int irq, int ipl, int type, int (*func)(void *), void *arg)
{
	return intr_establish_xname(irq, ipl, type, func, arg, NULL);
}

void
intr_disestablish(void *ih)
{
//	struct intrsource * const is = ih;

	KASSERT(!cpu_intr_p());
	KASSERT(!cpu_softintr_p());
}


#ifdef MULTIPROCESSOR
__CTASSERT(NIPIS < 16);

int
riscv_ipi_intr(void *arg)
{
	struct cpu_info * const ci = curcpu();
	membar_acquire();

	csr_sip_clear(SIP_SSIP);	/* clean pending interrupt status */

	unsigned long pending;
	while ((pending = atomic_swap_ulong(&ci->ci_request_ipis, 0)) != 0) {
		membar_acquire();
		atomic_or_ulong(&ci->ci_active_ipis, pending);

		ipi_process(ci, pending);

		atomic_and_ulong(&ci->ci_active_ipis, pending);
	}

	return 1;
}

int
cpu_send_ipi(struct cpu_info *ci, int req)
{
	KASSERT(req < NIPIS);
	if (ci == NULL) {
		CPU_INFO_ITERATOR cii;
		for (CPU_INFO_FOREACH(cii, ci)) {
			if (ci != curcpu()) {
				cpu_send_ipi(ci, req);
			}
		}
		return 0;
	}
	const uint32_t ipi_mask = __BIT(req);

	membar_release();
	atomic_or_ulong(&ci->ci_request_ipis, ipi_mask);

	membar_release();
	unsigned long hartmask = 0;
	const cpuid_t hartid = ci->ci_cpuid;
	KASSERT(hartid < sizeof(unsigned long) * NBBY);
	hartmask |= __BIT(hartid);
	struct sbiret sbiret = sbi_send_ipi(hartmask, 0);

	KASSERT(sbiret.error == SBI_SUCCESS);

	return 0;
}
#endif	/* MULTIPROCESSOR */

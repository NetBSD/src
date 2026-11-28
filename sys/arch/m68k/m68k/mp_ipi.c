/*	$NetBSD: mp_ipi.c,v 1.1 2026/11/28 01:35:36 thorpej Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2000, 2001, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: mp_ipi.c,v 1.1 2026/11/28 01:35:36 thorpej Exp $");

/*
 * Interprocessor interrupt handlers.
 *
 * These are generic routines that don't have any direct hardware
 * dependency.  All that we require from hardware is a doorbell
 * addressable by hardware CPU ID and for the doorbell handler to
 * call cpu_ipi_process() at spl7().
 */

#include <sys/param.h>
#include <sys/evcnt.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/ipi.h>
#include <sys/intr.h>
#include <sys/xcall.h>

#include <m68k/cacheops.h>

#include <uvm/uvm_extern.h>

typedef void (*ipifunc_t)(struct trapframe *);

static void	cpu_ipi_halt(struct trapframe *);
static void	cpu_ipi_ast(struct trapframe *);
static void	cpu_ipi_pause(struct trapframe *);
static void	cpu_ipi_xcall(struct trapframe *);
static void	cpu_ipi_generic(struct trapframe *);

struct ipi_impl {
	const char *name;
	ipifunc_t   handler;
};

/* Ordered highest-to-lowest priority. */
struct ipi_impl ipi_impl[] = {
	[_IPI_HALT] = {
		.name    = "halt ipi",
		.handler = cpu_ipi_halt,
	},
	[_IPI_PMAP_USER] = {
		.name    = "pmap (user) ipi",
		.handler = (ipifunc_t)pmap_message_user_process,
	},
	[_IPI_PMAP_INTR] = {
		.name    = "pmap (intr) ipi",
		.handler = (ipifunc_t)pmap_message_intr_process,
	},
	[_IPI_PMAP] = {
		.name    = "pmap ipi",
		.handler = (ipifunc_t)pmap_message_process,
	},
	[_IPI_AST] = {
		.name    = "ast ipi",
		.handler = cpu_ipi_ast,
	},
	[_IPI_PAUSE] = {
		.name    = "pause ipi",
		.handler = cpu_ipi_pause,
	},
	[_IPI_XCALL] = {
		.name    = "xcall ipi",
		.handler = cpu_ipi_xcall,
	},
	[_IPI_GENERIC] = {
		.name    = "generic ipi",
		.handler = cpu_ipi_generic,
	},
};

struct cpu_ipi_data {
	struct evcnt	cid_ipi_ev;
	struct evcnt	cid_ipi_which_ev[_NIPIS];
};

/*
 * Initialize IPI state for a CPU.  This must be called after
 * mi_cpu_attach().
 */
void
cpu_ipi_init(struct cpu_info *ci)
{
	struct cpu_ipi_data *cid = kmem_zalloc(sizeof(*cid), KM_SLEEP);
	int i;

	evcnt_attach_dynamic(&cid->cid_ipi_ev, EVCNT_TYPE_INTR,
	    NULL, ci->ci_cpuname, "ipi");

	for (i = 0; i < _NIPIS; i++) {
		evcnt_attach_dynamic(&cid->cid_ipi_which_ev[i],
		    EVCNT_TYPE_INTR, NULL, ci->ci_cpuname,
		    ipi_impl[i].name);
	}

	ci->ci_ipi_data = cid;
}

/*
 * Process IPIs for a CPU.
 */
void
cpu_ipi_process(struct trapframe *tf)
{
	struct cpu_ipi_data * const cid = cpuinfo.ci_ipi_data;
	u_int pending_ipis, i;

	KASSERT((getsr() & PSL_HIGHIPL) == PSL_HIGHIPL);
	KASSERT(cid != NULL);

	while ((pending_ipis = atomic_swap_uint(&cpuinfo.ci_ipis, 0)) != 0) {
		/* N.B all m68k loads are load-acquire. */

		cid->cid_ipi_ev.ev_count++;

		for (i = 0; i < _NIPIS; i++) {
			if (pending_ipis & (1UL << i)) {
				cid->cid_ipi_which_ev[i].ev_count++;
				(*ipi_impl[i].handler)(tf);
			}
		}
	}
}

/*
 * Send an interprocessor interrupt.
 */
void
cpu_send_ipi(u_int const cpu_id, int const which)
{

	KASSERT(cpu_info_tab != NULL);
	KASSERT(cpu_info_tab[cpu_id] != NULL);
	KASSERT(cpus_running & (1U << cpu_id));
	KASSERT(which >= 0);
	KASSERT(which <= 31);

	/* N.B. all m68k stores are store-release. */
	atomic_or_uint(&cpu_info_tab[cpu_id]->ci_ipis, (1U << which));

	/* Ring the doorbell. */
	machine_ipi_doorbell(cpu_id);
}

/*
 * Send an IPI to all in the list but ourselves.
 */
void
cpu_multicast_ipi(uint32_t cpumask, int const which)
{
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;

	cpumask &= cpus_running;
	cpumask &= ~(1U << cpu_number());
	if (cpumask == 0)
		return;

	for (CPU_INFO_FOREACH(cii, ci)) {
		if ((cpumask & (1UL << ci->ci_cpuid)) == 0) {
			continue;
		}
		cpu_send_ipi(ci->ci_cpuid, which);
	}
}

/*
 * Broadcast an IPI to all but ourselves.
 */
void
cpu_broadcast_ipi(int const which)
{
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;

	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci == cpuinfo.ci_self) {
			continue;
		}
		cpu_send_ipi(ci->ci_cpuid, which);
	}
}

static void
cpu_send_ipi_ci(struct cpu_info * const ci, int which)
{
	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != ci);

	if (ci) {
		/* Unicast: remote CPU. */
		cpu_send_ipi(ci->ci_cpuid, which);
	} else {
		/* Broadcast: all, but local CPU (caller will handle it). */
		cpu_broadcast_ipi(which);
	}
}

static void
cpu_ipi_halt(struct trapframe * const tf __unused)
{
	/* Disable interrupts. */
	(void) spl7();

	/* XXX XXX XXX */

	for (;;) {
		__nothing;
	}
}

static void
cpu_ipi_ast(struct trapframe * const framep __unused)
{
	if (cpuinfo.ci_onproc != cpuinfo.ci_data.cpu_idlelwp) {
		cpu_set_ast(cpuinfo.ci_onproc);
	}
}

static void
cpu_ipi_pause(struct trapframe * const tf)
{
	const u_int cpumask = (1U << cpuinfo.ci_cpuid);

	/* Point debuggers at our trapframe for register state. */
	cpuinfo.ci_db_regs = tf;
	atomic_or_uint(&cpuinfo.ci_flags, CPU_F_PAUSED);

	/* Spin with interrupts disabled until we're resumed. */
	while (cpus_paused & cpumask) {
		__nothing;
	}

	atomic_and_uint(&cpuinfo.ci_flags, (u_int)~CPU_F_PAUSED);
	cpuinfo.ci_db_regs = NULL;

	/*
	 * Invalidate ATC and I-cache on the way out in case
	 * things have changed.
	 */
	TBIA();
	ICIA();
}

/*
 * MD support for xcall(9) interface.
 */

static void
cpu_ipi_xcall(struct trapframe * const tf __unused)
{
	xc_ipi_handler();
}

void
xc_send_ipi(struct cpu_info * const ci)
{
	cpu_send_ipi_ci(ci, _IPI_XCALL);
}

/*
 * MD support for ipi(9) interface.
 */

static void
cpu_ipi_generic(struct trapframe * const tf __unused)
{
	ipi_cpu_handler();
}

void
cpu_ipi(struct cpu_info * const ci)
{
	cpu_send_ipi_ci(ci, _IPI_GENERIC);
}

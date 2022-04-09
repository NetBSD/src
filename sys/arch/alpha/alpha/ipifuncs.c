/* $NetBSD: ipifuncs.c,v 1.55 2022/04/09 23:42:56 riastradh Exp $ */

/*-
 * Copyright (c) 1998, 1999, 2000, 2001 The NetBSD Foundation, Inc.
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

__KERNEL_RCSID(0, "$NetBSD: ipifuncs.c,v 1.55 2022/04/09 23:42:56 riastradh Exp $");

/*
 * Interprocessor interrupt handlers.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/ipi.h>
#include <sys/intr.h>
#include <sys/xcall.h>
#include <sys/bitops.h>

#include <uvm/uvm_extern.h>

#include <machine/alpha_cpu.h>
#include <machine/alpha.h>
#include <machine/cpuvar.h>
#include <machine/rpb.h>
#include <machine/prom.h>

typedef void (*ipifunc_t)(struct cpu_info *, struct trapframe *);

static void	alpha_ipi_halt(struct cpu_info *, struct trapframe *);
static void	alpha_ipi_primary_cc(struct cpu_info *, struct trapframe *);
static void	alpha_ipi_ast(struct cpu_info *, struct trapframe *);
static void	alpha_ipi_pause(struct cpu_info *, struct trapframe *);
static void	alpha_ipi_xcall(struct cpu_info *, struct trapframe *);
static void	alpha_ipi_generic(struct cpu_info *, struct trapframe *);

const ipifunc_t ipifuncs[ALPHA_NIPIS] = {
	[ilog2(ALPHA_IPI_HALT)] =	alpha_ipi_halt,
	[ilog2(ALPHA_IPI_PRIMARY_CC)] =	alpha_ipi_primary_cc,
	[ilog2(ALPHA_IPI_SHOOTDOWN)] =	pmap_tlb_shootdown_ipi,
	[ilog2(ALPHA_IPI_AST)] =	alpha_ipi_ast,
	[ilog2(ALPHA_IPI_PAUSE)] =	alpha_ipi_pause,
	[ilog2(ALPHA_IPI_XCALL)] =	alpha_ipi_xcall,
	[ilog2(ALPHA_IPI_GENERIC)] =	alpha_ipi_generic
};

const char * const ipinames[ALPHA_NIPIS] = {
	[ilog2(ALPHA_IPI_HALT)] =	"halt ipi",
	[ilog2(ALPHA_IPI_PRIMARY_CC)] =	"primary cc ipi",
	[ilog2(ALPHA_IPI_SHOOTDOWN)] =	"shootdown ipi",
	[ilog2(ALPHA_IPI_AST)] =	"ast ipi",
	[ilog2(ALPHA_IPI_PAUSE)] =	"pause ipi",
	[ilog2(ALPHA_IPI_XCALL)] =	"xcall ipi",
	[ilog2(ALPHA_IPI_GENERIC)] =	"generic ipi",
};

/*
 * Initialize IPI state for a CPU.
 *
 * Note: the cpu_info softc pointer must be valid.
 */
void
alpha_ipi_init(struct cpu_info *ci)
{
	struct cpu_softc * const sc = ci->ci_softc;
	const char * const xname = device_xname(sc->sc_dev);
	int i;

	evcnt_attach_dynamic(&sc->sc_evcnt_ipi, EVCNT_TYPE_INTR,
	    NULL, xname, "ipi");

	for (i = 0; i < ALPHA_NIPIS; i++) {
		evcnt_attach_dynamic(&sc->sc_evcnt_which_ipi[i],
		    EVCNT_TYPE_INTR, NULL, xname, ipinames[i]);
	}
}

/*
 * Process IPIs for a CPU.
 */
void
alpha_ipi_process(struct cpu_info *ci, struct trapframe *framep)
{
	struct cpu_softc * const sc = ci->ci_softc;
	u_long pending_ipis, bit;

#ifdef DIAGNOSTIC
	if (sc == NULL) {
		/* XXX panic? */
		printf("WARNING: no softc for ID %lu\n", ci->ci_cpuid);
		return;
	}
#endif

	while ((pending_ipis = atomic_swap_ulong(&ci->ci_ipis, 0)) != 0) {
		/*
		 * Ensure everything prior to setting ci_ipis in
		 * alpha_send_ipi happens-before everything after
		 * reading ci_ipis here so we're not working on stale
		 * inputs.
		 */
		membar_acquire();

		sc->sc_evcnt_ipi.ev_count++;

		for (bit = 0; bit < ALPHA_NIPIS; bit++) {
			if (pending_ipis & (1UL << bit)) {
				sc->sc_evcnt_which_ipi[bit].ev_count++;
				(*ipifuncs[bit])(ci, framep);
			}
		}
	}
}

/*
 * Send an interprocessor interrupt.
 */
void
alpha_send_ipi(u_long const cpu_id, u_long const ipimask)
{

	KASSERT(cpu_id < hwrpb->rpb_pcs_cnt);
	KASSERT(cpu_info[cpu_id] != NULL);
	KASSERT(cpus_running & (1UL << cpu_id));

	/*
	 * Make sure all loads and stores prior to calling
	 * alpha_send_ipi() have completed before informing
	 * the CPU of the work we are asking it to do.
	 */
	membar_release();
	atomic_or_ulong(&cpu_info[cpu_id]->ci_ipis, ipimask);

	/*
	 * Ensure that the store of ipimask completes before actually
	 * writing to the IPIR.
	 *
	 * Note: we use MB rather than WMB because how the IPIR
	 * is implemented is not architecturally specified, and
	 * WMB is only guaranteed to provide ordering for stores
	 * to regions of the same memory-likeness.
	 */
	alpha_mb();
	alpha_pal_wripir(cpu_id);
}

/*
 * Broadcast an IPI to all but ourselves.
 */
void
alpha_broadcast_ipi(u_long const ipimask)
{
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;

	const u_long cpu_id = cpu_number();
	const u_long cpumask = cpus_running & ~(1UL << cpu_id);

	for (CPU_INFO_FOREACH(cii, ci)) {
		if ((cpumask & (1UL << ci->ci_cpuid)) == 0)
			continue;
		alpha_send_ipi(ci->ci_cpuid, ipimask);
	}
}

/*
 * Send an IPI to all in the list but ourselves.
 */
void
alpha_multicast_ipi(u_long cpumask, u_long const ipimask)
{
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;

	cpumask &= cpus_running;
	cpumask &= ~(1UL << cpu_number());
	if (cpumask == 0)
		return;

	for (CPU_INFO_FOREACH(cii, ci)) {
		if ((cpumask & (1UL << ci->ci_cpuid)) == 0)
			continue;
		alpha_send_ipi(ci->ci_cpuid, ipimask);
	}
}

static void
alpha_ipi_halt(struct cpu_info * const ci,
    struct trapframe * const framep __unused)
{
	const u_long cpu_id = ci->ci_cpuid;
	const u_long wait_mask = (1UL << cpu_id);

	/* Disable interrupts. */
	(void) splhigh();

	if (cpu_id != hwrpb->rpb_primary_cpu_id) {
		/*
		 * If we're not the primary, we just halt now.
		 */
		cpu_halt();
	}

	/*
	 * We're the primary.  We need to wait for all the other
	 * secondary CPUs to halt, then we can drop back to the
	 * console.
	 */
	alpha_mb();
	for (;;) {
		alpha_mb();
		if (cpus_running == wait_mask)
			break;
		delay(1000);
	}

	prom_halt(boothowto & RB_HALT);
	/* NOTREACHED */
}

static void
alpha_ipi_primary_cc(struct cpu_info * const ci __unused,
    struct trapframe * const framep __unused)
{
	int const s = splhigh();
	cc_primary_cc();
	splx(s);
}

static void
alpha_ipi_ast(struct cpu_info * const ci,
    struct trapframe * const framep __unused)
{

	if (ci->ci_onproc != ci->ci_data.cpu_idlelwp)
		aston(ci->ci_onproc);
}

static void
alpha_ipi_pause(struct cpu_info * const ci, struct trapframe * const framep)
{
	const u_long cpumask = (1UL << ci->ci_cpuid);
	int s;

	s = splhigh();

	/* Point debuggers at our trapframe for register state. */
	ci->ci_db_regs = framep;
	alpha_wmb();
	atomic_or_ulong(&ci->ci_flags, CPUF_PAUSED);

	/* Spin with interrupts disabled until we're resumed. */
	do {
		alpha_mb();
	} while (cpus_paused & cpumask);

	atomic_and_ulong(&ci->ci_flags, ~CPUF_PAUSED);
	alpha_wmb();
	ci->ci_db_regs = NULL;

	splx(s);

	/* Do a TBIA+IMB on the way out, in case things have changed. */
	ALPHA_TBIA();
	alpha_pal_imb();
}

/*
 * MD support for xcall(9) interface.
 */

static void
alpha_ipi_xcall(struct cpu_info * const ci __unused,
    struct trapframe * const framep __unused)
{
	xc_ipi_handler();
}

void
xc_send_ipi(struct cpu_info * const ci)
{
	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != ci);

	if (ci) {
		/* Unicast: remote CPU. */
		alpha_send_ipi(ci->ci_cpuid, ALPHA_IPI_XCALL);
	} else {
		/* Broadcast: all, but local CPU (caller will handle it). */
		alpha_broadcast_ipi(ALPHA_IPI_XCALL);
	}
}

static void
alpha_ipi_generic(struct cpu_info * const ci __unused,
    struct trapframe * const framep __unused)
{
	ipi_cpu_handler();
}

void
cpu_ipi(struct cpu_info * const ci)
{
	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != ci);

	if (ci) {
		/* Unicast: remote CPU. */
		alpha_send_ipi(ci->ci_cpuid, ALPHA_IPI_GENERIC);
	} else {
		/* Broadcast: all, but local CPU (caller will handle it). */
		alpha_broadcast_ipi(ALPHA_IPI_GENERIC);
	}
}

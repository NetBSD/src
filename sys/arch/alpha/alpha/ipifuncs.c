/* $NetBSD: ipifuncs.c,v 1.47.12.1 2014/08/20 00:02:41 tls Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: ipifuncs.c,v 1.47.12.1 2014/08/20 00:02:41 tls Exp $");

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
static void	alpha_ipi_microset(struct cpu_info *, struct trapframe *);
static void	alpha_ipi_imb(struct cpu_info *, struct trapframe *);
static void	alpha_ipi_ast(struct cpu_info *, struct trapframe *);
static void	alpha_ipi_pause(struct cpu_info *, struct trapframe *);
static void	alpha_ipi_xcall(struct cpu_info *, struct trapframe *);
static void	alpha_ipi_generic(struct cpu_info *, struct trapframe *);

/*
 * NOTE: This table must be kept in order with the bit definitions
 * in <machine/intr.h>.
 */
const ipifunc_t ipifuncs[ALPHA_NIPIS] = {
	[ilog2(ALPHA_IPI_HALT)] =	alpha_ipi_halt,
	[ilog2(ALPHA_IPI_MICROSET)] =	alpha_ipi_microset,
	[ilog2(ALPHA_IPI_SHOOTDOWN)] =	pmap_do_tlb_shootdown,
	[ilog2(ALPHA_IPI_IMB)] =	alpha_ipi_imb,
	[ilog2(ALPHA_IPI_AST)] =	alpha_ipi_ast,
	[ilog2(ALPHA_IPI_PAUSE)] =	alpha_ipi_pause,
	[ilog2(ALPHA_IPI_XCALL)] =	alpha_ipi_xcall,
	[ilog2(ALPHA_IPI_GENERIC)] =	alpha_ipi_generic
};

const char * const ipinames[ALPHA_NIPIS] = {
	[ilog2(ALPHA_IPI_HALT)] =	"halt ipi",
	[ilog2(ALPHA_IPI_MICROSET)] =	"microset ipi",
	[ilog2(ALPHA_IPI_SHOOTDOWN)] =	"shootdown ipi",
	[ilog2(ALPHA_IPI_IMB)] =	"imb ipi",
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

	pending_ipis = atomic_swap_ulong(&ci->ci_ipis, 0);

	/*
	 * For various reasons, it is possible to have spurious calls
	 * to this routine, so just bail out now if there are none
	 * pending.
	 */
	if (pending_ipis == 0)
		return;

	sc->sc_evcnt_ipi.ev_count++;

	for (bit = 0; bit < ALPHA_NIPIS; bit++) {
		if (pending_ipis & (1UL << bit)) {
			sc->sc_evcnt_which_ipi[bit].ev_count++;
			(*ipifuncs[bit])(ci, framep);
		}
	}
}

/*
 * Send an interprocessor interrupt.
 */
void
alpha_send_ipi(u_long cpu_id, u_long ipimask)
{

#ifdef DIAGNOSTIC
	if (cpu_id >= hwrpb->rpb_pcs_cnt ||
	    cpu_info[cpu_id] == NULL)
		panic("alpha_send_ipi: bogus cpu_id");
	if (((1UL << cpu_id) & cpus_running) == 0)
		panic("alpha_send_ipi: CPU %ld not running", cpu_id);
#endif

	atomic_or_ulong(&cpu_info[cpu_id]->ci_ipis, ipimask);
	alpha_pal_wripir(cpu_id);
}

/*
 * Broadcast an IPI to all but ourselves.
 */
void
alpha_broadcast_ipi(u_long ipimask)
{
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
	u_long cpu_id = cpu_number();
	u_long cpumask;

	cpumask = cpus_running & ~(1UL << cpu_id);

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
alpha_multicast_ipi(u_long cpumask, u_long ipimask)
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
alpha_ipi_halt(struct cpu_info *ci, struct trapframe *framep)
{
	u_long cpu_id = ci->ci_cpuid;
	u_long wait_mask = (1UL << cpu_id);

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
alpha_ipi_microset(struct cpu_info *ci, struct trapframe *framep)
{

	cc_calibrate_cpu(ci);
}

static void
alpha_ipi_imb(struct cpu_info *ci, struct trapframe *framep)
{

	alpha_pal_imb();
}

static void
alpha_ipi_ast(struct cpu_info *ci, struct trapframe *framep)
{

	if (ci->ci_curlwp != ci->ci_data.cpu_idlelwp)
		aston(ci->ci_curlwp);
}

static void
alpha_ipi_pause(struct cpu_info *ci, struct trapframe *framep)
{
	u_long cpumask = (1UL << ci->ci_cpuid);
	int s;

	s = splhigh();

	/* Point debuggers at our trapframe for register state. */
	ci->ci_db_regs = framep;

	atomic_or_ulong(&ci->ci_flags, CPUF_PAUSED);

	/* Spin with interrupts disabled until we're resumed. */
	do {
		alpha_mb();
	} while (cpus_paused & cpumask);

	atomic_and_ulong(&ci->ci_flags, ~CPUF_PAUSED);

	ci->ci_db_regs = NULL;

	splx(s);

	/* Do an IMB on the way out, in case the kernel text was changed. */
	alpha_pal_imb();
}

/*
 * MD support for xcall(9) interface.
 */

static void
alpha_ipi_xcall(struct cpu_info *ci, struct trapframe *framep)
{
	xc_ipi_handler();
}

void
xc_send_ipi(struct cpu_info *ci)
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
alpha_ipi_generic(struct cpu_info *ci, struct trapframe *framep)
{
	ipi_cpu_handler();
}

void
cpu_ipi(struct cpu_info *ci)
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

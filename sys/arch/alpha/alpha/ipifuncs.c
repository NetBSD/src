/* $NetBSD: ipifuncs.c,v 1.33 2003/02/05 12:16:42 nakayama Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

__KERNEL_RCSID(0, "$NetBSD: ipifuncs.c,v 1.33 2003/02/05 12:16:42 nakayama Exp $");

/*
 * Interprocessor interrupt handlers.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/reboot.h>

#include <uvm/uvm_extern.h>

#include <machine/atomic.h>
#include <machine/alpha_cpu.h>
#include <machine/alpha.h>
#include <machine/cpu.h>
#include <machine/cpuvar.h>
#include <machine/intr.h>
#include <machine/rpb.h>
#include <machine/prom.h>

typedef void (*ipifunc_t)(struct cpu_info *, struct trapframe *);

void	alpha_ipi_halt(struct cpu_info *, struct trapframe *);
void	alpha_ipi_microset(struct cpu_info *, struct trapframe *);
void	alpha_ipi_imb(struct cpu_info *, struct trapframe *);
void	alpha_ipi_ast(struct cpu_info *, struct trapframe *);
void	alpha_ipi_synch_fpu(struct cpu_info *, struct trapframe *);
void	alpha_ipi_discard_fpu(struct cpu_info *, struct trapframe *);
void	alpha_ipi_pause(struct cpu_info *, struct trapframe *);

/*
 * NOTE: This table must be kept in order with the bit definitions
 * in <machine/intr.h>.
 */
ipifunc_t ipifuncs[ALPHA_NIPIS] = {
	alpha_ipi_halt,
	alpha_ipi_microset,
	pmap_do_tlb_shootdown,
	alpha_ipi_imb,
	alpha_ipi_ast,
	alpha_ipi_synch_fpu,
	alpha_ipi_discard_fpu,
	alpha_ipi_pause,
	pmap_do_reactivate,
};

const char *ipinames[ALPHA_NIPIS] = {
	"halt ipi",
	"microset ipi",
	"shootdown ipi",
	"imb ipi",
	"ast ipi",
	"synch fpu ipi",
	"discard fpu ipi",
	"pause ipi",
	"pmap reactivate ipi",
};

/*
 * Initialize IPI state for a CPU.
 *
 * Note: the cpu_info softc pointer must be valid.
 */
void
alpha_ipi_init(struct cpu_info *ci)
{
	struct cpu_softc *sc = ci->ci_softc;
	int i;

	evcnt_attach_dynamic(&sc->sc_evcnt_ipi, EVCNT_TYPE_INTR,
	    NULL, sc->sc_dev.dv_xname, "ipi");

	for (i = 0; i < ALPHA_NIPIS; i++) {
		evcnt_attach_dynamic(&sc->sc_evcnt_which_ipi[i],
		    EVCNT_TYPE_INTR, NULL, sc->sc_dev.dv_xname,
		    ipinames[i]);
	}
}

/*
 * Process IPIs for a CPU.
 */
void
alpha_ipi_process(struct cpu_info *ci, struct trapframe *framep)
{
	struct cpu_softc *sc = ci->ci_softc;
	u_long pending_ipis, bit;

#ifdef DIAGNOSTIC
	if (sc == NULL) {
		/* XXX panic? */
		printf("WARNING: no softc for ID %lu\n", ci->ci_cpuid);
		return;
	}
#endif

	pending_ipis = atomic_loadlatch_ulong(&ci->ci_ipis, 0);

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

	atomic_setbits_ulong(&cpu_info[cpu_id]->ci_ipis, ipimask);
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

void
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
	printf("%s: waiting for secondary CPUs to halt...\n",
	    ci->ci_softc->sc_dev.dv_xname);
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

void
alpha_ipi_microset(struct cpu_info *ci, struct trapframe *framep)
{

	cc_microset(ci);
}

void
alpha_ipi_imb(struct cpu_info *ci, struct trapframe *framep)
{

	alpha_pal_imb();
}

void
alpha_ipi_ast(struct cpu_info *ci, struct trapframe *framep)
{

	if (ci->ci_curlwp != NULL)
		aston(ci->ci_curlwp->l_proc);
}

void
alpha_ipi_synch_fpu(struct cpu_info *ci, struct trapframe *framep)
{

	if (ci->ci_flags & CPUF_FPUSAVE)
		return;
	fpusave_cpu(ci, 1);
}

void
alpha_ipi_discard_fpu(struct cpu_info *ci, struct trapframe *framep)
{

	if (ci->ci_flags & CPUF_FPUSAVE)
		return;
	fpusave_cpu(ci, 0);
}

void
alpha_ipi_pause(struct cpu_info *ci, struct trapframe *framep)
{
	u_long cpumask = (1UL << ci->ci_cpuid);
	int s;

	s = splhigh();

	/* Point debuggers at our trapframe for register state. */
	ci->ci_db_regs = framep;

	atomic_setbits_ulong(&ci->ci_flags, CPUF_PAUSED);

	/* Spin with interrupts disabled until we're resumed. */
	do {
		alpha_mb();
	} while (cpus_paused & cpumask);

	atomic_clearbits_ulong(&ci->ci_flags, CPUF_PAUSED);

	ci->ci_db_regs = NULL;

	splx(s);

	/* Do an IMB on the way out, in case the kernel text was changed. */
	alpha_pal_imb();
}

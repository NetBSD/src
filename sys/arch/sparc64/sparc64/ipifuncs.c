/*	$NetBSD: ipifuncs.c,v 1.47.10.1 2014/08/10 06:54:09 tls Exp $ */

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: ipifuncs.c,v 1.47.10.1 2014/08/10 06:54:09 tls Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/xcall.h>
#include <sys/ipi.h>

#include <machine/db_machdep.h>

#include <machine/cpu.h>
#include <machine/cpu_counter.h>
#include <machine/ctlreg.h>
#include <machine/pmap.h>
#include <machine/sparc64.h>

#include <sparc64/sparc64/cache.h>

#if defined(DDB) || defined(KGDB)
#ifdef DDB
#include <ddb/db_command.h>
#include <ddb/db_output.h>
#endif
#endif

#ifdef SUN4V
#define SPARC64_IPI_RETRIES	10000
#endif

/* CPU sets containing halted, paused and resumed cpus */
static volatile sparc64_cpuset_t cpus_halted;
static volatile sparc64_cpuset_t cpus_spinning;
static volatile sparc64_cpuset_t cpus_paused;
static volatile sparc64_cpuset_t cpus_resumed;

/* IPI handlers. */
static int	sparc64_ipi_wait(sparc64_cpuset_t volatile *, sparc64_cpuset_t);
static void	sparc64_ipi_error(const char *, sparc64_cpuset_t, sparc64_cpuset_t);

/* Send IPI functions for supported platforms */
static void	sparc64_send_ipi_sun4u(int, ipifunc_t, uint64_t, uint64_t);
#ifdef SUN4V
static void	sparc64_send_ipi_sun4v(int, ipifunc_t, uint64_t, uint64_t);
#endif
 
/*
 * These are the "function" entry points in locore.s/mp_subr.s to handle IPI's.
 */
void	sparc64_ipi_halt(void *, void *);
void	sparc64_ipi_pause(void *, void *);
void	sparc64_ipi_flush_pte_us(void *, void *);
void	sparc64_ipi_flush_pte_usiii(void *, void *);
#ifdef SUN4V
void	sparc64_ipi_flush_pte_sun4v(void *, void *);
#endif
void	sparc64_ipi_dcache_flush_page_us(void *, void *);
void	sparc64_ipi_dcache_flush_page_usiii(void *, void *);
void	sparc64_ipi_blast_dcache(void *, void *);
void	sparc64_ipi_ccall(void *, void *);

/*
 * Process cpu stop-self event.
 */
void
sparc64_ipi_halt_thiscpu(void *arg, void *arg2)
{
	extern void prom_printf(const char *fmt, ...);

	printf("cpu%d: shutting down\n", cpu_number());
	if (prom_has_stop_other() || !prom_has_stopself()) {
		/*
		 * just loop here, the final cpu will stop us later
		 */
		CPUSET_ADD(cpus_spinning, cpu_number());
		CPUSET_ADD(cpus_halted, cpu_number());
		spl0();
		while (1)
			/* nothing */;
	} else {
		CPUSET_ADD(cpus_halted, cpu_number());
		prom_stopself();
	}
}

void
sparc64_do_pause(void)
{
#if defined(DDB)
	extern bool ddb_running_on_this_cpu(void);
	extern void db_resume_others(void);
#endif

	CPUSET_ADD(cpus_paused, cpu_number());

	do {
		membar_Sync();
	} while(CPUSET_HAS(cpus_paused, cpu_number()));
	membar_Sync();
	CPUSET_ADD(cpus_resumed, cpu_number());

#if defined(DDB)
	if (ddb_running_on_this_cpu()) {
		db_command_loop();
		db_resume_others();
	}
#endif
}

/*
 * Pause cpu.  This is called from locore.s after setting up a trapframe.
 */
void
sparc64_ipi_pause_thiscpu(void *arg)
{
	int s;
#if defined(DDB)
	extern void fill_ddb_regs_from_tf(struct trapframe64 *tf);
	extern void ddb_restore_state(void);
	
	if (arg)
		fill_ddb_regs_from_tf(arg);
#endif

	s = intr_disable();
	sparc64_do_pause();

#if defined(DDB)
	if (arg) {
		ddb_restore_state();
		curcpu()->ci_ddb_regs = NULL;
	}
#endif

	intr_restore(s);
}

/*
 * Initialize IPI machinery.
 */
void
sparc64_ipi_init(void)
{

	/* Clear all cpu sets. */
	CPUSET_CLEAR(cpus_halted);
	CPUSET_CLEAR(cpus_spinning);
	CPUSET_CLEAR(cpus_paused);
	CPUSET_CLEAR(cpus_resumed);
}

/*
 * Send an IPI to all in the list but ourselves.
 */
void
sparc64_multicast_ipi(sparc64_cpuset_t cpuset, ipifunc_t func, uint64_t arg1,
		      uint64_t arg2)
{
	struct cpu_info *ci;

	CPUSET_DEL(cpuset, cpu_number());
	if (CPUSET_EMPTY(cpuset))
		return;

	for (ci = cpus; ci != NULL; ci = ci->ci_next) {
		if (CPUSET_HAS(cpuset, ci->ci_index)) {
			CPUSET_DEL(cpuset, ci->ci_index);
			sparc64_send_ipi(ci->ci_cpuid, func, arg1, arg2);
		}
	}
}

/*
 * Broadcast an IPI to all but ourselves.
 */
void
sparc64_broadcast_ipi(ipifunc_t func, uint64_t arg1, uint64_t arg2)
{

	sparc64_multicast_ipi(CPUSET_EXCEPT(cpus_active, cpu_number()), func,
		arg1, arg2);
}

/*
 * Send an interprocessor interrupt.
 */
void
sparc64_send_ipi(int upaid, ipifunc_t func, uint64_t arg1, uint64_t arg2)
{
#ifdef SUN4V	
	if (CPU_ISSUN4V)
		sparc64_send_ipi_sun4v(upaid, func, arg1, arg2);
	else
#endif		
		sparc64_send_ipi_sun4u(upaid, func, arg1, arg2);
		    
}

/*
 * Send an interprocessor interrupt - sun4v.
 */
void
sparc64_send_ipi_sun4u(int upaid, ipifunc_t func, uint64_t arg1, uint64_t arg2)
{
	int i, ik, shift = 0;
	uint64_t intr_func;

	KASSERT(upaid != curcpu()->ci_cpuid);

	/*
	 * UltraSPARC-IIIi CPUs select the BUSY/NACK pair based on the
	 * lower two bits of the ITID.
	 */
	if (CPU_IS_USIIIi())
		shift = (upaid & 0x3) * 2;

	if (ldxa(0, ASI_IDSR) & (IDSR_BUSY << shift))
		panic("recursive IPI?");

	intr_func = (uint64_t)(u_long)func;

	/* Schedule an interrupt. */
	for (i = 0; i < 10000; i++) {
		int s = intr_disable();

		stxa(IDDR_0H, ASI_INTERRUPT_DISPATCH, intr_func);
		stxa(IDDR_1H, ASI_INTERRUPT_DISPATCH, arg1);
		stxa(IDDR_2H, ASI_INTERRUPT_DISPATCH, arg2);
		stxa(IDCR(upaid), ASI_INTERRUPT_DISPATCH, 0);
		membar_Sync();
		/* Workaround for SpitFire erratum #54, from FreeBSD */
		if (CPU_IS_SPITFIRE()) {
			(void)ldxa(P_DCR_0, ASI_INTERRUPT_RECEIVE_DATA);
			membar_Sync();
		}

		for (ik = 0; ik < 1000000; ik++) {
			if (ldxa(0, ASI_IDSR) & (IDSR_BUSY << shift))
				continue;
			else
				break;
		}
		intr_restore(s);

		if (ik == 1000000)
			break;

		if ((ldxa(0, ASI_IDSR) & (IDSR_NACK << shift)) == 0)
			return;
		/*
		 * Wait for a while with enabling interrupts to avoid
		 * deadlocks.  XXX - random value is better.
		 */
		DELAY(1);
	}

	if (panicstr == NULL)
		panic("cpu%d: ipi_send: couldn't send ipi to UPAID %u"
			" (tried %d times)", cpu_number(), upaid, i);
}

#ifdef SUN4V
/*
 * Send an interprocessor interrupt - sun4v.
 */
void
sparc64_send_ipi_sun4v(int cpuid, ipifunc_t func, uint64_t arg1, uint64_t arg2)
{
	struct cpu_info *ci = curcpu();
	int err, i;
	
	stha(ci->ci_cpuset, ASI_PHYS_CACHED, cpuid);
	stxa(ci->ci_mondo, ASI_PHYS_CACHED, (vaddr_t)func);
	stxa(ci->ci_mondo + 8, ASI_PHYS_CACHED, arg1);
	stxa(ci->ci_mondo + 16, ASI_PHYS_CACHED, arg2);
	
	for (i = 0; i < SPARC64_IPI_RETRIES; i++) {
		err = hv_cpu_mondo_send(1, ci->ci_cpuset, ci->ci_mondo);
		if (err != H_EWOULDBLOCK)
			break;
		delay(10);
	}
	if (err != H_EOK)
		panic("Unable to send mondo %lx to cpu %d: %d",
		    (long unsigned int)func, cpuid, err);
}
#endif

/*
 * Wait for IPI operation to complete.
 * Return 0 on success.
 */
int
sparc64_ipi_wait(sparc64_cpuset_t volatile *cpus_watchset, sparc64_cpuset_t cpus_mask)
{
	uint64_t limit = gettick() + cpu_frequency(curcpu());

	while (gettick() < limit) {
		membar_Sync();
		if (CPUSET_EQUAL(*cpus_watchset, cpus_mask))
			return 0;
	}
	return 1;
}

/*
 * Halt all cpus but ourselves.
 */
void
mp_halt_cpus(void)
{
	sparc64_cpuset_t cpumask, cpuset;
	struct cpu_info *ci;

	CPUSET_ASSIGN(cpuset, cpus_active);
	CPUSET_DEL(cpuset, cpu_number());
	CPUSET_ASSIGN(cpumask, cpuset);
	CPUSET_SUB(cpuset, cpus_halted);

	if (CPUSET_EMPTY(cpuset))
		return;

	CPUSET_CLEAR(cpus_spinning);
	sparc64_multicast_ipi(cpuset, sparc64_ipi_halt, 0, 0);
	if (sparc64_ipi_wait(&cpus_halted, cpumask))
		sparc64_ipi_error("halt", cpumask, cpus_halted);

	/*
	 * Depending on available firmware methods, other cpus will
	 * either shut down themselfs, or spin and wait for us to
	 * stop them.
	 */
	if (CPUSET_EMPTY(cpus_spinning)) {
		/* give other cpus a few cycles to actually power down */
		delay(10000);
		return;
	}
	/* there are cpus spinning - shut them down if we can */
	if (prom_has_stop_other()) {
		for (ci = cpus; ci != NULL; ci = ci->ci_next) {
			if (!CPUSET_HAS(cpus_spinning, ci->ci_index)) continue;
			prom_stop_other(ci->ci_cpuid);
		}
	}
}

/*
 * Pause all cpus but ourselves.
 */
void
mp_pause_cpus(void)
{
	int i = 3;
	sparc64_cpuset_t cpuset;

	CPUSET_ASSIGN(cpuset, cpus_active);
	CPUSET_DEL(cpuset, cpu_number());
	while (i-- > 0) {
		if (CPUSET_EMPTY(cpuset))
			return;

		sparc64_multicast_ipi(cpuset, sparc64_ipi_pause, 0, 0);
		if (!sparc64_ipi_wait(&cpus_paused, cpuset))
			return;
		CPUSET_SUB(cpuset, cpus_paused);
	}
	sparc64_ipi_error("pause", cpus_paused, cpuset);
}

/*
 * Resume a single cpu
 */
void
mp_resume_cpu(int cno)
{
	CPUSET_DEL(cpus_paused, cno);
	membar_Sync();
}

/*
 * Resume all paused cpus.
 */
void
mp_resume_cpus(void)
{
	int i = 3;
	sparc64_cpuset_t cpuset;

	CPUSET_CLEAR(cpuset);	/* XXX: gcc -Wuninitialized */

	while (i-- > 0) {
		CPUSET_CLEAR(cpus_resumed);
		CPUSET_ASSIGN(cpuset, cpus_paused);
		membar_Sync();
		CPUSET_CLEAR(cpus_paused);

		/* CPUs awake on cpus_paused clear */
		if (!sparc64_ipi_wait(&cpus_resumed, cpuset))
			return;
	}
	sparc64_ipi_error("resume", cpus_resumed, cpuset);
}

int
mp_cpu_is_paused(sparc64_cpuset_t cpunum)
{

	return CPUSET_HAS(cpus_paused, cpunum);
}

/*
 * Flush pte on all active processors.
 */
void
smp_tlb_flush_pte(vaddr_t va, struct pmap * pm)
{
	sparc64_cpuset_t cpuset;
	struct cpu_info *ci;
	int ctx;
	bool kpm = (pm == pmap_kernel());
	ipifunc_t func;
#ifdef SUN4V
	if (CPU_ISSUN4V)
		func = sparc64_ipi_flush_pte_sun4v;
	else if (CPU_IS_USIII_UP())
#else
	if (CPU_IS_USIII_UP())
#endif		
		func = sparc64_ipi_flush_pte_usiii;
	else
		func = sparc64_ipi_flush_pte_us;

	/* Flush our own TLB */
	ctx = pm->pm_ctx[cpu_number()];
	KASSERT(ctx >= 0);
	if (kpm || ctx > 0)
		sp_tlb_flush_pte(va, ctx);

	CPUSET_ASSIGN(cpuset, cpus_active);
	CPUSET_DEL(cpuset, cpu_number());
	if (CPUSET_EMPTY(cpuset))
		return;

	/* Flush others */
	for (ci = cpus; ci != NULL; ci = ci->ci_next) {
		if (CPUSET_HAS(cpuset, ci->ci_index)) {
			CPUSET_DEL(cpuset, ci->ci_index);
			ctx = pm->pm_ctx[ci->ci_index];
			KASSERT(ctx >= 0);
			if (!kpm && ctx == 0)
				continue;
			sparc64_send_ipi(ci->ci_cpuid, func, va, ctx);
		}
	}
}

/*
 * Make sure this page is flushed from all/some CPUs.
 */
void
smp_dcache_flush_page_cpuset(paddr_t pa, sparc64_cpuset_t activecpus)
{
	ipifunc_t func;

	if (CPU_IS_USIII_UP())
		func = sparc64_ipi_dcache_flush_page_usiii;
	else
		func = sparc64_ipi_dcache_flush_page_us;

	sparc64_multicast_ipi(activecpus, func, pa, dcache_line_size);
	sp_dcache_flush_page(pa);
}

void
smp_dcache_flush_page_allcpu(paddr_t pa)
{

	smp_dcache_flush_page_cpuset(pa, cpus_active);
}

/*
 * Flush the D$ on all CPUs.
 */
void
smp_blast_dcache(void)
{

	sparc64_multicast_ipi(cpus_active, sparc64_ipi_blast_dcache,
			      dcache_size, dcache_line_size);
	sp_blast_dcache(dcache_size, dcache_line_size);
}

/*
 * Print an error message.
 */
void
sparc64_ipi_error(const char *s, sparc64_cpuset_t cpus_succeeded,
	sparc64_cpuset_t cpus_expected)
{
	int cpuid;

	CPUSET_DEL(cpus_expected, cpus_succeeded);
	if (!CPUSET_EMPTY(cpus_expected)) {
		printf("Failed to %s:", s);
		do {
			cpuid = CPUSET_NEXT(cpus_expected);
			CPUSET_DEL(cpus_expected, cpuid);
			printf(" cpu%d", cpuid);
		} while(!CPUSET_EMPTY(cpus_expected));
	}

	printf("\n");
}

/*
 * MD support for xcall(9) interface.
 */

void
sparc64_generic_xcall(struct cpu_info *target, ipi_c_call_func_t func,
	void *arg)
{
	/* if target == NULL broadcast to everything but curcpu */
	if (target)
		sparc64_send_ipi(target->ci_cpuid, sparc64_ipi_ccall,
		    (uint64_t)(uintptr_t)func, (uint64_t)(uintptr_t)arg);
	else {
		
		sparc64_multicast_ipi(cpus_active, sparc64_ipi_ccall,
		    (uint64_t)(uintptr_t)func, (uint64_t)(uintptr_t)arg);
	}
}

void
xc_send_ipi(struct cpu_info *target)
{

	sparc64_generic_xcall(target, (ipi_c_call_func_t)xc_ipi_handler, NULL);
}

void
cpu_ipi(struct cpu_info *target)
{

	sparc64_generic_xcall(target, (ipi_c_call_func_t)ipi_cpu_handler, NULL);
}

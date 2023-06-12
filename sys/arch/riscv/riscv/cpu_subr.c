/*	$NetBSD: cpu_subr.c,v 1.3 2023/06/12 19:04:14 skrll Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#include "opt_ddb.h"
#include "opt_multiprocessor.h"
#include "opt_riscv_debug.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu_subr.c,v 1.3 2023/06/12 19:04:14 skrll Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/xcall.h>

#include <machine/db_machdep.h>
#include <machine/sbi.h>

#ifdef DDB
#include <ddb/db_output.h>
#endif

#ifdef VERBOSE_INIT_RISCV
#define VPRINTF(...)	printf(__VA_ARGS__)
#else
#define VPRINTF(...)	__nothing
#endif

#ifdef MULTIPROCESSOR
#define NCPUINFO	MAXCPUS
#else
#define NCPUINFO	1
#endif /* MULTIPROCESSOR */

unsigned long cpu_hartid[NCPUINFO] = {
	[0 ... NCPUINFO - 1] = ~0,
};

#ifdef MULTIPROCESSOR

kcpuset_t *cpus_halted;
kcpuset_t *cpus_hatched;
kcpuset_t *cpus_paused;
kcpuset_t *cpus_resumed;
kcpuset_t *cpus_running;

#define	CPUINDEX_DIVISOR	(sizeof(u_long) * NBBY)

#define N howmany(MAXCPUS, CPUINDEX_DIVISOR)

/* cpu_hatch_ipi needs fixing for > 1 */
CTASSERT(N == 1);
volatile u_long riscv_cpu_hatched[N] __cacheline_aligned = { };
volatile u_long riscv_cpu_mbox[N] __cacheline_aligned = { };
u_int riscv_cpu_max = 1;

/* IPI all APs to GO! */
static void
cpu_ipi_aps(void)
{
	unsigned long hartmask = 0;
	// BP is index 0
	for (size_t i = 1; i < ncpu; i++) {
		const cpuid_t hartid = cpu_hartid[i];
		KASSERT(hartid < sizeof(unsigned long) * NBBY);
		hartmask |= __BIT(hartid);
	}
	struct sbiret sbiret = sbi_send_ipi(hartmask, 0);

	KASSERT(sbiret.error == SBI_SUCCESS);
}

void
cpu_boot_secondary_processors(void)
{
	u_int cpuno;

	if ((boothowto & RB_MD1) != 0)
		return;

	VPRINTF("%s: starting secondary processors\n", __func__);

	/*
	 * send mbox to have secondary processors do cpu_hatch()
	 * store-release matches locore.S
	 */
	asm volatile("fence rw,w");
	for (size_t n = 0; n < __arraycount(riscv_cpu_mbox); n++)
		atomic_or_ulong(&riscv_cpu_mbox[n], riscv_cpu_hatched[n]);
	cpu_ipi_aps();

	/* wait for all cpus to have done cpu_hatch() */
	for (cpuno = 1; cpuno < ncpu; cpuno++) {
		if (!cpu_hatched_p(cpuno))
			continue;

		const size_t off = cpuno / CPUINDEX_DIVISOR;
		const u_long bit = __BIT(cpuno % CPUINDEX_DIVISOR);

		/* load-acquire matches cpu_clr_mbox */
		while (atomic_load_acquire(&riscv_cpu_mbox[off]) & bit) {
			/* spin - it shouldn't be long */
			;
		}
	}

	VPRINTF("%s: secondary processors hatched\n", __func__);
}

bool
cpu_hatched_p(u_int cpuindex)
{
	const u_int off = cpuindex / CPUINDEX_DIVISOR;
	const u_int bit = cpuindex % CPUINDEX_DIVISOR;

	/* load-acquire matches cpu_set_hatched */
	return (atomic_load_acquire(&riscv_cpu_hatched[off]) & __BIT(bit)) != 0;
}


void
cpu_set_hatched(int cpuindex)
{

	const size_t off = cpuindex / CPUINDEX_DIVISOR;
	const u_long bit = __BIT(cpuindex % CPUINDEX_DIVISOR);

	/* store-release matches cpu_hatched_p */
	asm volatile("fence rw, w" ::: "memory");
	atomic_or_ulong(&riscv_cpu_hatched[off], bit);

	asm volatile("fence w, rw" ::: "memory");
}

void
cpu_clr_mbox(int cpuindex)
{

	const size_t off = cpuindex / CPUINDEX_DIVISOR;
	const u_long bit = __BIT(cpuindex % CPUINDEX_DIVISOR);

	/* store-release matches locore.S */
	asm volatile("fence rw,w" ::: "memory");
	atomic_and_ulong(&riscv_cpu_mbox[off], ~bit);

	asm volatile("fence w, rw"  ::: "memory");
}


void
cpu_broadcast_ipi(int tag)
{

	/*
	 * No reason to remove ourselves since multicast_ipi will do that
	 * for us.
	 */
	cpu_multicast_ipi(cpus_running, tag);
}

void
cpu_multicast_ipi(const kcpuset_t *kcp, int tag)
{
	struct cpu_info * const ci = curcpu();
	kcpuset_t *kcp2 = ci->ci_multicastcpus;

	if (kcpuset_match(cpus_running, ci->ci_data.cpu_kcpuset))
		return;

	kcpuset_copy(kcp2, kcp);
	kcpuset_remove(kcp2, ci->ci_data.cpu_kcpuset);
	for (unsigned int cii; (cii = kcpuset_ffs(kcp2)) != 0; ) {
		kcpuset_clear(kcp2, --cii);
		(void)cpu_send_ipi(cpu_lookup(cii), tag);
	}
}

static void
cpu_ipi_wait(const char *s, const kcpuset_t *watchset, const kcpuset_t *wanted)
{
	bool done = false;
	struct cpu_info * const ci = curcpu();
	kcpuset_t *kcp = ci->ci_watchcpus;

	/* some finite amount of time */

	for (u_long limit = curcpu()->ci_cpu_freq / 10; !done && limit--; ) {
		kcpuset_copy(kcp, watchset);
		kcpuset_intersect(kcp, wanted);
		done = kcpuset_match(kcp, wanted);
	}

	if (!done) {
		cpuid_t cii;
		kcpuset_copy(kcp, wanted);
		kcpuset_remove(kcp, watchset);
		if ((cii = kcpuset_ffs(kcp)) != 0) {
			printf("Failed to %s:", s);
			do {
				kcpuset_clear(kcp, --cii);
				printf(" cpu%lu", cii);
			} while ((cii = kcpuset_ffs(kcp)) != 0);
			printf("\n");
		}
	}
}

/*
 * Halt this cpu
 */
void
cpu_halt(void)
{
	cpuid_t cii = cpu_index(curcpu());

	printf("cpu%lu: shutting down\n", cii);
	kcpuset_atomic_set(cpus_halted, cii);
	spl0();		/* allow interrupts e.g. further ipi ? */
	for (;;) ;	/* spin */

	/* NOTREACHED */
}

/*
 * Halt all running cpus, excluding current cpu.
 */
void
cpu_halt_others(void)
{
	kcpuset_t *kcp;

	// If we are the only CPU running, there's nothing to do.
	if (kcpuset_match(cpus_running, curcpu()->ci_data.cpu_kcpuset))
		return;

	// Get all running CPUs
	kcpuset_clone(&kcp, cpus_running);
	// Remove ourself
	kcpuset_remove(kcp, curcpu()->ci_data.cpu_kcpuset);
	// Remove any halted CPUs
	kcpuset_remove(kcp, cpus_halted);
	// If there are CPUs left, send the IPIs
	if (!kcpuset_iszero(kcp)) {
		cpu_multicast_ipi(kcp, IPI_HALT);
		cpu_ipi_wait("halt", cpus_halted, kcp);
	}
	kcpuset_destroy(kcp);

	/*
	 * TBD
	 * Depending on available firmware methods, other cpus will
	 * either shut down themselves, or spin and wait for us to
	 * stop them.
	 */
}

/*
 * Pause this cpu
 */
void
cpu_pause(void	)
{
	const int s = splhigh();
	cpuid_t cii = cpu_index(curcpu());

	if (__predict_false(cold)) {
		splx(s);
		return;
	}

	do {
		kcpuset_atomic_set(cpus_paused, cii);
		do {
			;
		} while (kcpuset_isset(cpus_paused, cii));
		kcpuset_atomic_set(cpus_resumed, cii);
#if defined(DDB)
		if (ddb_running_on_this_cpu_p())
			cpu_Debugger();
		if (ddb_running_on_any_cpu_p())
			continue;
#endif
	} while (false);

	splx(s);
}

/*
 * Pause all running cpus, excluding current cpu.
 */
void
cpu_pause_others(void)
{
	struct cpu_info * const ci = curcpu();

	if (cold || kcpuset_match(cpus_running, ci->ci_data.cpu_kcpuset))
		return;

	kcpuset_t *kcp = ci->ci_ddbcpus;

	kcpuset_copy(kcp, cpus_running);
	kcpuset_remove(kcp, ci->ci_data.cpu_kcpuset);
	kcpuset_remove(kcp, cpus_paused);

	cpu_broadcast_ipi(IPI_SUSPEND);
	cpu_ipi_wait("pause", cpus_paused, kcp);
}

/*
 * Resume a single cpu
 */
void
cpu_resume(cpuid_t cii)
{

	if (__predict_false(cold))
		return;

	struct cpu_info * const ci = curcpu();
	kcpuset_t *kcp = ci->ci_ddbcpus;

	kcpuset_set(kcp, cii);
	kcpuset_atomicly_remove(cpus_resumed, cpus_resumed);
	kcpuset_atomic_clear(cpus_paused, cii);

	cpu_ipi_wait("resume", cpus_resumed, kcp);
}

/*
 * Resume all paused cpus.
 */
void
cpu_resume_others(void)
{

	if (__predict_false(cold))
		return;

	struct cpu_info * const ci = curcpu();
	kcpuset_t *kcp = ci->ci_ddbcpus;

	kcpuset_atomicly_remove(cpus_resumed, cpus_resumed);
	kcpuset_copy(kcp, cpus_paused);
	kcpuset_atomicly_remove(cpus_paused, cpus_paused);

	/* CPUs awake on cpus_paused clear */
	cpu_ipi_wait("resume", cpus_resumed, kcp);
}

bool
cpu_is_paused(cpuid_t cii)
{

	return !cold && kcpuset_isset(cpus_paused, cii);
}

#ifdef DDB
void
cpu_debug_dump(void)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	char running, hatched, paused, resumed, halted;
	db_printf("CPU CPUID STATE CPUINFO            CPL INT MTX IPIS(A/R)\n");
	for (CPU_INFO_FOREACH(cii, ci)) {
		hatched = (kcpuset_isset(cpus_hatched, cpu_index(ci)) ? 'H' : '-');
		running = (kcpuset_isset(cpus_running, cpu_index(ci)) ? 'R' : '-');
		paused  = (kcpuset_isset(cpus_paused,  cpu_index(ci)) ? 'P' : '-');
		resumed = (kcpuset_isset(cpus_resumed, cpu_index(ci)) ? 'r' : '-');
		halted  = (kcpuset_isset(cpus_halted,  cpu_index(ci)) ? 'h' : '-');
		db_printf("%3d 0x%03lx %c%c%c%c%c %p "
			"%3d %3d %3d "
			"0x%02lx/0x%02lx\n",
			cpu_index(ci), ci->ci_cpuid,
			running, hatched, paused, resumed, halted,
			ci, ci->ci_cpl, ci->ci_intr_depth, ci->ci_mtx_count,
			ci->ci_active_ipis, ci->ci_request_ipis);
	}
}
#endif

void
xc_send_ipi(struct cpu_info *ci)
{
	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != ci);

	cpu_send_ipi(ci, IPI_XCALL);
}

void
cpu_ipi(struct cpu_info *ci)
{
	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != ci);

	cpu_send_ipi(ci, IPI_GENERIC);
}

#endif

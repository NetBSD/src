/*	$NetBSD: ipifuncs.c,v 1.3.6.1 2006/04/22 11:38:02 simonb Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ipifuncs.c,v 1.3.6.1 2006/04/22 11:38:02 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/ctlreg.h>
#include <machine/pte.h>
#include <machine/sparc64.h>


#define IPI_TLB_SHOOTDOWN	0

#define	SPARC64_IPI_HALT_NO	100
#define	SPARC64_IPI_RESUME_NO	101

#define SPARC64_IPI_RETRIES	100

#define	sparc64_ipi_sleep()	delay(1000)

extern int db_active;

typedef void (* ipifunc_t)(void *);

/* CPU sets containing halted, paused and resumed cpus */
static volatile cpuset_t cpus_halted;
static volatile cpuset_t cpus_paused;
static volatile cpuset_t cpus_resumed;

volatile struct ipi_tlb_args ipi_tlb_args;

/* IPI handlers. */
static int	sparc64_ipi_halt(void *);
static int	sparc64_ipi_pause(void *);
static int	sparc64_ipi_wait(cpuset_t volatile *, cpuset_t);
static void	sparc64_ipi_error(const char *, cpuset_t, cpuset_t);

void	sparc64_ipi_flush_pte(void *);
void	sparc64_ipi_flush_ctx(void *);
void	sparc64_ipi_flush_all(void *);

/* IPI handlers working at SOFTINT level. */
static struct intrhand ipi_halt_intr = {
	sparc64_ipi_halt, NULL, SPARC64_IPI_HALT_NO, IPL_HALT
};

static struct intrhand ipi_pause_intr = {
	sparc64_ipi_pause, NULL, SPARC64_IPI_RESUME_NO, IPL_PAUSE
};

static struct intrhand *ipihand[] = {
	&ipi_halt_intr,
	&ipi_pause_intr
};

/*
 * Fast IPI table. If function is null, the handler is looked up
 * in 'ipihand' table.
 */
static ipifunc_t ipifuncs[SPARC64_NIPIS] = {
	NULL,			/* ipi_halt_intr  */
	NULL,			/* ipi_pause_intr */
	sparc64_ipi_flush_pte,
	sparc64_ipi_flush_ctx,
	sparc64_ipi_flush_all
};


/*
 * Process cpu stop-self event.
 */
static int
sparc64_ipi_halt(void *arg)
{

	printf("cpu%d: shutting down\n", cpu_number());
	CPUSET_ADD(cpus_halted, cpu_number());
	prom_stopself();

	return(1);
}

/*
 * Pause cpu.
 */
static int
sparc64_ipi_pause(void *arg)
{
	int s;
	cpuid_t cpuid;

	cpuid = cpu_number();
	printf("cpu%ld paused.\n", cpuid);

	s = intr_disable();
	CPUSET_ADD(cpus_paused, cpuid);

	do {
		membar_sync();
	} while(CPUSET_HAS(cpus_paused, cpuid));
	membar_sync();

	CPUSET_ADD(cpus_resumed, cpuid);

	intr_restore(s);
	printf("cpu%ld resumed.\n", cpuid);
	return (1);
}

/*
 * Initialize IPI machinery.
 */
void
sparc64_ipi_init()
{

	/* Clear all cpu sets. */
	CPUSET_CLEAR(cpus_halted);
	CPUSET_CLEAR(cpus_paused);
	CPUSET_CLEAR(cpus_resumed);

	/* Install interrupt handlers. */
	intr_establish(ipi_halt_intr.ih_pil, &ipi_halt_intr);
	intr_establish(ipi_pause_intr.ih_pil, &ipi_pause_intr);
}

/*
 * Send an IPI to all in the list but ourselves.
 */
void
sparc64_multicast_ipi(cpuset_t cpuset, u_long ipimask)
{
	struct cpu_info *ci;

	CPUSET_DEL(cpuset, cpu_number());
	if (CPUSET_EMPTY(cpuset))
		return;

	for (ci = cpus; ci != NULL; ci = ci->ci_next) {
		if (CPUSET_HAS(cpuset, ci->ci_number)) {
			CPUSET_DEL(cpuset, ci->ci_number);
			sparc64_send_ipi(ci->ci_upaid, ipimask);
		}
	}
}

/*
 * Broadcast an IPI to all but ourselves.
 */
void
sparc64_broadcast_ipi(u_long ipimask)
{
	cpuset_t cpuset;

	CPUSET_ALL_BUT(cpuset, cpu_number());
	sparc64_multicast_ipi(cpuset, ipimask);
}

/*
 * Send an interprocessor interrupt.
 */
void
sparc64_send_ipi(int upaid, u_long ipimask)
{
	int i;
	uint64_t intr_number, intr_func, intr_arg;

	KASSERT(ipimask < (1UL << SPARC64_NIPIS));
	KASSERT((ldxa(0, ASR_IDSR) & IDSR_BUSY) == 0);

	/* Setup interrupt data. */
	i = ffs(ipimask) - 1;
	intr_func = (uint64_t)ipifuncs[i];
	if (intr_func) {
		/* fast trap */
		intr_number = 0;
		intr_arg = (uint64_t)&ipi_tlb_args;
	} else {
		/* softint trap */
		struct intrhand *ih = ipihand[i];
		intr_number = (uint64_t)ih->ih_number;
		intr_func = (uint64_t)ih->ih_fun;
		intr_arg = (uint64_t)ih->ih_arg;
	}

	/* Schedule an interrupt. */
	for (i = 0; i < SPARC64_IPI_RETRIES; i++) {
		int s = intr_disable();

		stxa(IDDR_0H, ASI_INTERRUPT_DISPATCH, intr_number);
		stxa(IDDR_1H, ASI_INTERRUPT_DISPATCH, intr_func);
		stxa(IDDR_2H, ASI_INTERRUPT_DISPATCH, intr_arg);
		stxa(IDCR(upaid), ASI_INTERRUPT_DISPATCH, 0);
		membar_sync();

		while (ldxa(0, ASR_IDSR) & IDSR_BUSY)
			;
		intr_restore(s);

		if ((ldxa(0, ASR_IDSR) & IDSR_NACK) == 0)
			return;
	}

	if (db_active || panicstr != NULL)
		printf("ipi_send: couldn't send ipi to module %u\n", upaid);
	else
		panic("ipi_send: couldn't send ipi");
}

/*
 * Wait for IPI operation to complete.
 */
int
sparc64_ipi_wait(cpuset_t volatile *cpus_watchset, cpuset_t cpus_mask)
{
	int i;

	for (i = 0; i < SPARC64_IPI_RETRIES; i++) {
		membar_sync();
		if (CPUSET_EQUAL(*cpus_watchset, cpus_mask))
			break;
		sparc64_ipi_sleep();
	}
	return (i == SPARC64_IPI_RETRIES);
}

/*
 * Halt all cpus but ourselves.
 */
void
sparc64_ipi_halt_cpus()
{
	cpuset_t cpumask, cpuset;

	CPUSET_ASSIGN(cpuset, cpus_active);
	CPUSET_DEL(cpuset, cpu_number());
	CPUSET_ASSIGN(cpumask, cpuset);
	CPUSET_SUB(cpuset, cpus_halted);

	if (CPUSET_EMPTY(cpuset))
		return;

	sparc64_multicast_ipi(cpuset, SPARC64_IPI_HALT);
	if (sparc64_ipi_wait(&cpus_halted, cpumask))
		sparc64_ipi_error("halt", cpumask, cpus_halted);
}

/*
 * Pause all cpus but ourselves.
 */
void
sparc64_ipi_pause_cpus()
{
	cpuset_t cpuset;

	CPUSET_ASSIGN(cpuset, cpus_active);
	CPUSET_DEL(cpuset, cpu_number());

	if (CPUSET_EMPTY(cpuset))
		return;

	sparc64_multicast_ipi(cpuset, SPARC64_IPI_PAUSE);
	if (sparc64_ipi_wait(&cpus_paused, cpuset))
		sparc64_ipi_error("pause", cpus_paused, cpuset);
}

/*
 * Resume all paused cpus.
 */
void
sparc64_ipi_resume_cpus()
{
	cpuset_t cpuset;

	CPUSET_CLEAR(cpus_resumed);
	CPUSET_ASSIGN(cpuset, cpus_paused);
	membar_sync();
	CPUSET_CLEAR(cpus_paused);

	/* CPUs awake on cpus_paused clear */
	if (sparc64_ipi_wait(&cpus_resumed, cpuset))
		sparc64_ipi_error("resume", cpus_resumed, cpuset);
}

/*
 * Flush pte on all active processors.
 */
void
smp_tlb_flush_pte(vaddr_t va, int ctx)
{
	/* Flush our own TLB */
	sp_tlb_flush_pte(va, ctx);

#if defined(IPI_TLB_SHOOTDOWN)
	/* Flush others */
	ipi_tlb_args.ita_vaddr = va;
	ipi_tlb_args.ita_ctx = ctx;

	sparc64_broadcast_ipi(SPARC64_IPI_FLUSH_PTE);
#endif
}

/*
 * Flush context on all active processors.
 */
void
smp_tlb_flush_ctx(int ctx)
{
	/* Flush our own TLB */
	sp_tlb_flush_ctx(ctx);

#if defined(IPI_TLB_SHOOTDOWN)
	/* Flush others */
	ipi_tlb_args.ita_vaddr = (vaddr_t)0;
	ipi_tlb_args.ita_ctx = ctx;

	sparc64_broadcast_ipi(SPARC64_IPI_FLUSH_CTX);
#endif
}

/*
 * Flush whole TLB on all active processors.
 */
void
smp_tlb_flush_all()
{
	/* Flush our own TLB */
	sp_tlb_flush_all();

#if defined(IPI_TLB_SHOOTDOWN)
	/* Flush others */
	sparc64_broadcast_ipi(SPARC64_IPI_FLUSH_ALL);
#endif
}

/*
 * Print an error message.
 */
void
sparc64_ipi_error(const char *s, cpuset_t cpus_succeeded,
	cpuset_t cpus_expected)
{
	int cpuid;

	CPUSET_DEL(cpus_expected, cpus_succeeded);
	printf("Failed to %s:", s);
	do {
		cpuid = CPUSET_NEXT(cpus_expected);
		CPUSET_DEL(cpus_expected, cpuid);
		printf(" cpu%d", cpuid);
	} while(!CPUSET_EMPTY(cpus_expected));
	printf("\n");
}

/*	$NetBSD: octeon_intr.c,v 1.27 2022/04/09 23:34:40 riastradh Exp $	*/
/*
 * Copyright 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Platform-specific interrupt support for the MIPS Malta.
 */

#include "opt_multiprocessor.h"

#include "cpunode.h"
#define __INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_intr.c,v 1.27 2022/04/09 23:34:40 riastradh Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/atomic.h>
#include <sys/xcall.h>

#include <lib/libkern/libkern.h>

#include <mips/locore.h>

#include <mips/cavium/dev/octeon_ciureg.h>
#include <mips/cavium/octeonvar.h>

/*
 * XXX:
 * Force all interrupts (except clock intrs and IPIs) to be routed
 * through cpu0 until MP on MIPS is more stable.
 */
#define	OCTEON_CPU0_INTERRUPTS


/*
 * This is a mask of bits to clear in the SR when we go to a
 * given hardware interrupt priority level.
 */
static const struct ipl_sr_map octeon_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE] =		0,
	[IPL_SOFTCLOCK] =	MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] =		MIPS_SOFT_INT_MASK,
	[IPL_VM] =		MIPS_SOFT_INT_MASK | MIPS_INT_MASK_0,
	[IPL_SCHED] =		MIPS_SOFT_INT_MASK | MIPS_INT_MASK_0
				    | MIPS_INT_MASK_1 | MIPS_INT_MASK_5,
	[IPL_DDB] =		MIPS_SOFT_INT_MASK | MIPS_INT_MASK_0
				    | MIPS_INT_MASK_1 | MIPS_INT_MASK_5,
	[IPL_HIGH] =		MIPS_INT_MASK,
    },
};

static const char * octeon_intrnames[NIRQS] = {
	"workq 0",
	"workq 1",
	"workq 2",
	"workq 3",
	"workq 4",
	"workq 5",
	"workq 6",
	"workq 7",
	"workq 8",
	"workq 9",
	"workq 10",
	"workq 11",
	"workq 12",
	"workq 13",
	"workq 14",
	"workq 15",
	"gpio 0",
	"gpio 1",
	"gpio 2",
	"gpio 3",
	"gpio 4",
	"gpio 5",
	"gpio 6",
	"gpio 7",
	"gpio 8",
	"gpio 9",
	"gpio 10",
	"gpio 11",
	"gpio 12",
	"gpio 13",
	"gpio 14",
	"gpio 15",
	"mbox 0-15",
	"mbox 16-31",
	"uart 0",
	"uart 1",
	"pci inta",
	"pci intb",
	"pci intc",
	"pci intd",
	"pci msi 0-15",
	"pci msi 16-31",
	"pci msi 32-47",
	"pci msi 48-63",
	"wdog summary",
	"twsi",
	"rml",
	"trace",
	"gmx drop",
	"reserved",
	"ipd drop",
	"reserved",
	"timer 0",
	"timer 1",
	"timer 2",
	"timer 3",
	"usb",
	"pcm/tdm",
	"mpi/spi",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
};

struct octeon_intrhand {
	int (*ih_func)(void *);
	void *ih_arg;
	int ih_irq;
	int ih_ipl;
};

#ifdef MULTIPROCESSOR
static int octeon_send_ipi(struct cpu_info *, int);
static int octeon_ipi_intr(void *);

static struct octeon_intrhand ipi_intrhands[2] = {
	[0] = {
		.ih_func = octeon_ipi_intr,
		.ih_arg = (void *)(uintptr_t)__BITS(15,0),
		.ih_irq = CIU_INT_MBOX_15_0,
		.ih_ipl = IPL_HIGH,
	},
	[1] = {
		.ih_func = octeon_ipi_intr,
		.ih_arg = (void *)(uintptr_t)__BITS(31,16),
		.ih_irq = CIU_INT_MBOX_31_16,
		.ih_ipl = IPL_SCHED,
	},
};

static int ipi_prio[NIPIS] = {
	[IPI_NOP] = IPL_HIGH,
	[IPI_AST] = IPL_HIGH,
	[IPI_SHOOTDOWN] = IPL_SCHED,
	[IPI_SYNCICACHE] = IPL_HIGH,
	[IPI_KPREEMPT] = IPL_HIGH,
	[IPI_SUSPEND] = IPL_HIGH,
	[IPI_HALT] = IPL_HIGH,
	[IPI_XCALL] = IPL_HIGH,
	[IPI_GENERIC] = IPL_HIGH,
	[IPI_WDOG] = IPL_HIGH,
};

#endif

static struct octeon_intrhand *octciu_intrs[NIRQS] = {
#ifdef MULTIPROCESSOR
	[CIU_INT_MBOX_15_0] = &ipi_intrhands[0],
	[CIU_INT_MBOX_31_16] = &ipi_intrhands[1],
#endif
};

static kmutex_t octeon_intr_lock;

#if defined(MULTIPROCESSOR)
#define	OCTEON_NCPU	MAXCPUS
#else
#define	OCTEON_NCPU	1
#endif

struct cpu_softc octeon_cpu_softc[OCTEON_NCPU];

static void
octeon_intr_setup(void)
{
	struct cpu_softc *cpu;
	int cpunum;

#define X(a)	MIPS_PHYS_TO_XKPHYS(OCTEON_CCA_NONE, (a))

	for (cpunum = 0; cpunum < OCTEON_NCPU; cpunum++) {
		cpu = &octeon_cpu_softc[cpunum];

		cpu->cpu_ip2_sum0 = X(CIU_IP2_SUM0(cpunum));
		cpu->cpu_ip3_sum0 = X(CIU_IP3_SUM0(cpunum));
		cpu->cpu_ip4_sum0 = X(CIU_IP4_SUM0(cpunum));

		cpu->cpu_int_sum1 = X(CIU_INT_SUM1);

		cpu->cpu_ip2_en[0] = X(CIU_IP2_EN0(cpunum));
		cpu->cpu_ip3_en[0] = X(CIU_IP3_EN0(cpunum));
		cpu->cpu_ip4_en[0] = X(CIU_IP4_EN0(cpunum));

		cpu->cpu_ip2_en[1] = X(CIU_IP2_EN1(cpunum));
		cpu->cpu_ip3_en[1] = X(CIU_IP3_EN1(cpunum));
		cpu->cpu_ip4_en[1] = X(CIU_IP4_EN1(cpunum));

		cpu->cpu_wdog = X(CIU_WDOG(cpunum));
		cpu->cpu_pp_poke = X(CIU_PP_POKE(cpunum));

#ifdef MULTIPROCESSOR
		cpu->cpu_mbox_set = X(CIU_MBOX_SET(cpunum));
		cpu->cpu_mbox_clr = X(CIU_MBOX_CLR(cpunum));
#endif
	}

#undef X

}

void
octeon_intr_init(struct cpu_info *ci)
{
	const int cpunum = cpu_index(ci);
	struct cpu_softc *cpu = &octeon_cpu_softc[cpunum];
	const char * const xname = cpu_name(ci);
	int bank;

	cpu->cpu_ci = ci;
	ci->ci_softc = cpu;

	KASSERT(cpunum == ci->ci_cpuid);

	if (ci->ci_cpuid == 0) {
		ipl_sr_map = octeon_ipl_sr_map;
		mutex_init(&octeon_intr_lock, MUTEX_DEFAULT, IPL_HIGH);
#ifdef MULTIPROCESSOR
		mips_locoresw.lsw_send_ipi = octeon_send_ipi;
#endif

		octeon_intr_setup();
	}

#ifdef MULTIPROCESSOR
	// Enable the IPIs
	cpu->cpu_ip4_enable[0] |= __BIT(CIU_INT_MBOX_15_0);
	cpu->cpu_ip3_enable[0] |= __BIT(CIU_INT_MBOX_31_16);
#endif

	if (ci->ci_dev) {
		for (bank = 0; bank < NBANKS; bank++) {
			aprint_verbose_dev(ci->ci_dev,
			    "enabling intr masks %u "
			    " %#"PRIx64"/%#"PRIx64"/%#"PRIx64"\n",
			    bank,
			    cpu->cpu_ip2_enable[bank],
			    cpu->cpu_ip3_enable[bank],
			    cpu->cpu_ip4_enable[bank]);
		}
	}

	for (bank = 0; bank < NBANKS; bank++) {
		mips3_sd(cpu->cpu_ip2_en[bank], cpu->cpu_ip2_enable[bank]);
		mips3_sd(cpu->cpu_ip3_en[bank], cpu->cpu_ip3_enable[bank]);
		mips3_sd(cpu->cpu_ip4_en[bank], cpu->cpu_ip4_enable[bank]);
	}

#ifdef MULTIPROCESSOR
	mips3_sd(cpu->cpu_mbox_clr, __BITS(31,0));
#endif

	for (int i = 0; i < NIRQS; i++) {
		if (octeon_intrnames[i] == NULL)
			octeon_intrnames[i] = kmem_asprintf("irq %d", i);
		evcnt_attach_dynamic(&cpu->cpu_intr_evs[i],
		    EVCNT_TYPE_INTR, NULL, xname, octeon_intrnames[i]);
	}
}

void
octeon_cal_timer(int corefreq)
{
	/* Compute the number of cycles per second. */
	curcpu()->ci_cpu_freq = corefreq;

	/* Compute the number of ticks for hz. */
	curcpu()->ci_cycles_per_hz = (curcpu()->ci_cpu_freq + hz / 2) / hz;

	/* Compute the delay divisor and reciprical. */
	curcpu()->ci_divisor_delay =
	    ((curcpu()->ci_cpu_freq + 500000) / 1000000);
#if 0
	MIPS_SET_CI_RECIPRICAL(curcpu());
#endif

	mips3_cp0_count_write(0);
	mips3_cp0_compare_write(0);
}

void *
octeon_intr_establish(int irq, int ipl, int (*func)(void *), void *arg)
{
	struct octeon_intrhand *ih;
	struct cpu_softc *cpu;
#ifndef OCTEON_CPU0_INTERRUPTS
	int cpunum;
#endif

	if (irq >= NIRQS)
		panic("octeon_intr_establish: bogus IRQ %d", irq);
	if (ipl < IPL_VM)
		panic("octeon_intr_establish: bogus IPL %d", ipl);

	ih = kmem_zalloc(sizeof(*ih), KM_NOSLEEP);
	if (ih == NULL)
		return (NULL);

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_irq = irq;
	ih->ih_ipl = ipl;

	mutex_enter(&octeon_intr_lock);

	/*
	 * First, make it known.
	 */
	KASSERTMSG(octciu_intrs[irq] == NULL, "irq %d in use! (%p)",
	    irq, octciu_intrs[irq]);

	atomic_store_release(&octciu_intrs[irq], ih);

	/*
	 * Now enable it.
	 */
	const int bank = irq / 64;
	const uint64_t irq_mask = __BIT(irq % 64);

	switch (ipl) {
	case IPL_VM:
		cpu = &octeon_cpu_softc[0];
		cpu->cpu_ip2_enable[bank] |= irq_mask;
		mips3_sd(cpu->cpu_ip2_en[bank], cpu->cpu_ip2_enable[bank]);
		break;

	case IPL_SCHED:
#ifdef OCTEON_CPU0_INTERRUPTS
		cpu = &octeon_cpu_softc[0];
		cpu->cpu_ip3_enable[bank] |= irq_mask;
		mips3_sd(cpu->cpu_ip3_en[bank], cpu->cpu_ip3_enable[bank]);
#else	/* OCTEON_CPU0_INTERRUPTS */
		for (cpunum = 0; cpunum < OCTEON_NCPU; cpunum++) {
			cpu = &octeon_cpu_softc[cpunum];
			if (cpu->cpu_ci == NULL)
				break;
			cpu->cpu_ip3_enable[bank] |= irq_mask;
			mips3_sd(cpu->cpu_ip3_en[bank], cpu->cpu_ip3_enable[bank]);
		}
#endif	/* OCTEON_CPU0_INTERRUPTS */
		break;

	case IPL_DDB:
	case IPL_HIGH:
#ifdef OCTEON_CPU0_INTERRUPTS
		cpu = &octeon_cpu_softc[0];
		cpu->cpu_ip4_enable[bank] |= irq_mask;
		mips3_sd(cpu->cpu_ip4_en[bank], cpu->cpu_ip4_enable[bank]);
#else	/* OCTEON_CPU0_INTERRUPTS */
		for (cpunum = 0; cpunum < OCTEON_NCPU; cpunum++) {
			cpu = &octeon_cpu_softc[cpunum];
			if (cpu->cpu_ci == NULL)
				break;
			cpu->cpu_ip4_enable[bank] |= irq_mask;
			mips3_sd(cpu->cpu_ip4_en[bank], cpu->cpu_ip4_enable[bank]);
		}
#endif	/* OCTEON_CPU0_INTERRUPTS */
		break;
	}

	mutex_exit(&octeon_intr_lock);

	return ih;
}

void
octeon_intr_disestablish(void *cookie)
{
	struct octeon_intrhand * const ih = cookie;
	struct cpu_softc *cpu;
	const int irq = ih->ih_irq & (NIRQS-1);
	const int ipl = ih->ih_ipl;
	int cpunum;

	mutex_enter(&octeon_intr_lock);

	/*
	 * First disable it.
	 */
	const int bank = irq / 64;
	const uint64_t irq_mask = ~__BIT(irq % 64);

	switch (ipl) {
	case IPL_VM:
		cpu = &octeon_cpu_softc[0];
		cpu->cpu_ip2_enable[bank] &= ~irq_mask;
		mips3_sd(cpu->cpu_ip2_en[bank], cpu->cpu_ip2_enable[bank]);
		break;

	case IPL_SCHED:
		for (cpunum = 0; cpunum < OCTEON_NCPU; cpunum++) {
			cpu = &octeon_cpu_softc[cpunum];
			if (cpu->cpu_ci == NULL)
				break;
			cpu->cpu_ip3_enable[bank] &= ~irq_mask;
			mips3_sd(cpu->cpu_ip3_en[bank], cpu->cpu_ip3_enable[bank]);
		}
		break;

	case IPL_DDB:
	case IPL_HIGH:
		for (cpunum = 0; cpunum < OCTEON_NCPU; cpunum++) {
			cpu = &octeon_cpu_softc[cpunum];
			if (cpu->cpu_ci == NULL)
				break;
			cpu->cpu_ip4_enable[bank] &= ~irq_mask;
			mips3_sd(cpu->cpu_ip4_en[bank], cpu->cpu_ip4_enable[bank]);
		}
		break;
	}

	atomic_store_relaxed(&octciu_intrs[irq], NULL);

	mutex_exit(&octeon_intr_lock);

	/*
	 * Wait until the interrupt handler is no longer running on all
	 * CPUs before freeing ih and returning.
	 */
	xc_barrier(0);
	kmem_free(ih, sizeof(*ih));
}

void
octeon_iointr(int ipl, vaddr_t pc, uint32_t ipending)
{
	struct cpu_info * const ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;
	int bank;

	KDASSERT(mips_cp0_status_read() & MIPS_SR_INT_IE);
	KASSERT((ipending & ~MIPS_INT_MASK) == 0);
	KASSERT(ipending & MIPS_HARD_INT_MASK);
	uint64_t hwpend[2] = { 0, 0 };

	const uint64_t sum1 = mips3_ld(cpu->cpu_int_sum1);

	if (ipending & MIPS_INT_MASK_2) {
		hwpend[0] = mips3_ld(cpu->cpu_ip4_sum0)
		    & cpu->cpu_ip4_enable[0];
		hwpend[1] = sum1 & cpu->cpu_ip4_enable[1];
	} else if (ipending & MIPS_INT_MASK_1) {
		hwpend[0] = mips3_ld(cpu->cpu_ip3_sum0)
		    & cpu->cpu_ip3_enable[0];
		hwpend[1] = sum1 & cpu->cpu_ip3_enable[1];
	} else if (ipending & MIPS_INT_MASK_0) {
		hwpend[0] = mips3_ld(cpu->cpu_ip2_sum0)
		    & cpu->cpu_ip2_enable[0];
		hwpend[1] = sum1 & cpu->cpu_ip2_enable[1];
	} else {
		panic("octeon_iointr: unexpected ipending %#x", ipending);
	}
	for (bank = 0; bank <= 1; bank++) {
		while (hwpend[bank] != 0) {
			const int bit = ffs64(hwpend[bank]) - 1;
			const int irq = (bank * 64) + bit;
			hwpend[bank] &= ~__BIT(bit);

			struct octeon_intrhand * const ih =
			    atomic_load_consume(&octciu_intrs[irq]);
			cpu->cpu_intr_evs[irq].ev_count++;
			if (__predict_true(ih != NULL)) {
#ifdef MULTIPROCESSOR
				if (ipl == IPL_VM) {
					KERNEL_LOCK(1, NULL);
#endif
					(*ih->ih_func)(ih->ih_arg);
#ifdef MULTIPROCESSOR
					KERNEL_UNLOCK_ONE(NULL);
				} else {
					(*ih->ih_func)(ih->ih_arg);
				}
#endif
				KDASSERT(mips_cp0_status_read() & MIPS_SR_INT_IE);
			}
		}
	}
	KDASSERT(mips_cp0_status_read() & MIPS_SR_INT_IE);
}

#ifdef MULTIPROCESSOR
__CTASSERT(NIPIS < 16);

int
octeon_ipi_intr(void *arg)
{
	struct cpu_info * const ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;
	const uint32_t mbox_mask = (uintptr_t) arg;
	uint32_t ipi_mask = mbox_mask;

	KASSERTMSG((mbox_mask & __BITS(31,16)) == 0 || ci->ci_cpl >= IPL_SCHED,
	    "mbox_mask %#"PRIx32" cpl %d", mbox_mask, ci->ci_cpl);

	ipi_mask &= mips3_ld(cpu->cpu_mbox_set);
	if (ipi_mask == 0)
		return 0;
	membar_acquire();

	mips3_sd(cpu->cpu_mbox_clr, ipi_mask);

	KASSERT(__SHIFTOUT(ipi_mask, mbox_mask) < __BIT(NIPIS));

#if NWDOG > 0
	// Handle WDOG requests ourselves.
	if (ipi_mask & __BIT(IPI_WDOG)) {
		softint_schedule(cpu->cpu_wdog_sih);
		atomic_and_64(&ci->ci_request_ipis, ~__BIT(IPI_WDOG));
		ipi_mask &= ~__BIT(IPI_WDOG);
		ci->ci_evcnt_per_ipi[IPI_WDOG].ev_count++;
		if (__predict_true(ipi_mask == 0))
			return 1;
	}
#endif

	/* if the request is clear, it was previously processed */
	if ((atomic_load_relaxed(&ci->ci_request_ipis) & ipi_mask) == 0)
		return 0;
	membar_acquire();

	atomic_or_64(&ci->ci_active_ipis, ipi_mask);
	atomic_and_64(&ci->ci_request_ipis, ~ipi_mask);

	ipi_process(ci, __SHIFTOUT(ipi_mask, mbox_mask));

	atomic_and_64(&ci->ci_active_ipis, ~ipi_mask);

	return 1;
}

int
octeon_send_ipi(struct cpu_info *ci, int req)
{
	KASSERT(req < NIPIS);
	if (ci == NULL) {
		CPU_INFO_ITERATOR cii;
		for (CPU_INFO_FOREACH(cii, ci)) {
			if (ci != curcpu()) {
				octeon_send_ipi(ci, req);
			}
		}
		return 0;
	}
	KASSERT(cold || ci->ci_softc != NULL);
	if (ci->ci_softc == NULL)
		return -1;

	struct cpu_softc * const cpu = ci->ci_softc;
	const u_int ipi_shift = ipi_prio[req] == IPL_SCHED ? 16 : 0;
	const uint32_t ipi_mask = __BIT(req + ipi_shift);

	membar_release();
	atomic_or_64(&ci->ci_request_ipis, ipi_mask);

	membar_release();
	mips3_sd(cpu->cpu_mbox_set, ipi_mask);

	return 0;
}
#endif	/* MULTIPROCESSOR */

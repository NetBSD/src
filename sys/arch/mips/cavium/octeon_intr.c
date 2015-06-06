/*	$NetBSD: octeon_intr.c,v 1.3.2.2 2015/06/06 14:40:01 skrll Exp $	*/
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

#include "opt_octeon.h"
#define __INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_intr.c,v 1.3.2.2 2015/06/06 14:40:01 skrll Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/atomic.h>

#include <lib/libkern/libkern.h>

#include <mips/locore.h>

#include <mips/cavium/dev/octeon_ciureg.h>
#include <mips/cavium/octeonvar.h>

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
				    | MIPS_INT_MASK_5,
	[IPL_DDB] =		MIPS_SOFT_INT_MASK | MIPS_INT_MASK_0
				    | MIPS_INT_MASK_1 | MIPS_INT_MASK_5,
	[IPL_HIGH] =		MIPS_INT_MASK,
    },
};

const char * const octeon_intrnames[NIRQS] = {
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

struct octeon_intrhand ipi_intrhands[2] = {
	[0] = {
		.ih_func = octeon_ipi_intr,
		.ih_arg = (void *)(uintptr_t)__BITS(15,0),
		.ih_irq = _CIU_INT_MBOX_15_0_SHIFT,
		.ih_ipl = IPL_SCHED,
	},
	[1] = {
		.ih_func = octeon_ipi_intr,
		.ih_arg = (void *)(uintptr_t)__BITS(31,16),
		.ih_irq = _CIU_INT_MBOX_31_16_SHIFT,
		.ih_ipl = IPL_HIGH,
	},
};
#endif

struct octeon_intrhand *octeon_ciu_intrs[NIRQS] = {
#ifdef MULTIPROCESSOR
	[_CIU_INT_MBOX_15_0_SHIFT] = &ipi_intrhands[0],
	[_CIU_INT_MBOX_31_16_SHIFT] = &ipi_intrhands[1],
#endif
};

kmutex_t octeon_intr_lock;

#define X(a)	MIPS_PHYS_TO_XKPHYS(OCTEON_CCA_NONE, (a))

struct cpu_softc octeon_cpu0_softc = {
	.cpu_ci = &cpu_info_store,
	.cpu_int0_sum0 = X(CIU_INT0_SUM0),
	.cpu_int1_sum0 = X(CIU_INT1_SUM0),
	.cpu_int2_sum0 = X(CIU_INT4_SUM0),

	.cpu_int0_en0 = X(CIU_INT0_EN0),
	.cpu_int1_en0 = X(CIU_INT1_EN0),
	.cpu_int2_en0 = X(CIU_INT4_EN00),

	.cpu_int0_en1 = X(CIU_INT0_EN1),
	.cpu_int1_en1 = X(CIU_INT1_EN1),
	.cpu_int2_en1 = X(CIU_INT4_EN01),

	.cpu_int32_en = X(CIU_INT32_EN0),

#ifdef MULTIPROCESSOR
	.cpu_mbox_set = X(CIU_MBOX_SET0),
	.cpu_mbox_clr = X(CIU_MBOX_CLR0),
#endif
};

#ifdef MULTIPROCESSOR
struct cpu_softc octeon_cpu1_softc = {
	.cpu_int0_sum0 = X(CIU_INT2_SUM0),
	.cpu_int1_sum0 = X(CIU_INT3_SUM0),
	.cpu_int2_sum0 = X(CIU_INT4_SUM1),

	.cpu_int0_en0 = X(CIU_INT2_EN0),
	.cpu_int1_en0 = X(CIU_INT3_EN0),
	.cpu_int2_en0 = X(CIU_INT4_EN10),

	.cpu_int0_en1 = X(CIU_INT2_EN1),
	.cpu_int1_en1 = X(CIU_INT3_EN1),
	.cpu_int2_en1 = X(CIU_INT4_EN11),

	.cpu_int32_en = X(CIU_INT32_EN1),

	.cpu_mbox_set = X(CIU_MBOX_SET1),
	.cpu_mbox_clr = X(CIU_MBOX_CLR1),
};
#endif

#undef X

void
octeon_intr_init(struct cpu_info *ci)
{
	const int cpunum = cpu_index(ci);
	const char * const xname = cpu_name(ci);
	struct cpu_softc *cpu;

	ipl_sr_map = octeon_ipl_sr_map;

	if (ci->ci_cpuid == 0) {
		mutex_init(&octeon_intr_lock, MUTEX_DEFAULT, IPL_HIGH);
		cpu = &octeon_cpu0_softc;
#ifdef MULTIPROCESSOR
		mips_locoresw.lsw_send_ipi = octeon_send_ipi;
#endif
	} else {
		KASSERT(cpunum == 1);
#ifdef MULTIPROCESSOR
		cpu = &octeon_cpu1_softc;
#else
		cpu = NULL;
#endif
	}
	ci->ci_softc = cpu;

#ifdef MULTIPROCESSOR
	// Enable the IPIs
	cpu->cpu_int0_enable0 |= __BIT(_CIU_INT_MBOX_15_0_SHIFT);
	cpu->cpu_int2_enable0 |= __BIT(_CIU_INT_MBOX_31_16_SHIFT);
#endif

	mips64_sd_a64(cpu->cpu_int0_en0, cpu->cpu_int0_enable0);
	mips64_sd_a64(cpu->cpu_int1_en0, cpu->cpu_int1_enable0);
	mips64_sd_a64(cpu->cpu_int2_en0, cpu->cpu_int2_enable0);

	mips64_sd_a64(cpu->cpu_int32_en, 0);

	mips64_sd_a64(cpu->cpu_int0_en1, 0);	// WDOG IPL2
	mips64_sd_a64(cpu->cpu_int1_en1, 0);	// WDOG IPL3
	mips64_sd_a64(cpu->cpu_int2_en1, 0);	// WDOG IPL4

#ifdef MULTIPROCESSOR
	mips64_sd_a64(cpu->cpu_mbox_clr, __BITS(31,0));
#endif

	for (size_t i = 0; i < NIRQS; i++) {
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
	KASSERTMSG(octeon_ciu_intrs[irq] == NULL, "irq %d in use! (%p)",
	    irq, octeon_ciu_intrs[irq]);

	octeon_ciu_intrs[irq] = ih;
	membar_producer();

	/*
	 * Now enable it.
	 */
	const uint64_t irq_mask = __BIT(irq);
	struct cpu_softc * const cpu0 = &octeon_cpu0_softc;
#if MULTIPROCESSOR
	struct cpu_softc * const cpu1 = &octeon_cpu1_softc;
#endif

	switch (ipl) {
	case IPL_VM:
		cpu0->cpu_int0_enable0 |= irq_mask;
		mips64_sd_a64(cpu0->cpu_int0_en0, cpu0->cpu_int0_enable0);
		break;

	case IPL_SCHED:
		cpu0->cpu_int1_enable0 |= irq_mask;
		mips64_sd_a64(cpu0->cpu_int1_en0, cpu0->cpu_int1_enable0);
#ifdef MULTIPROCESSOR
		cpu1->cpu_int1_enable0 = cpu0->cpu_int1_enable0;
		mips64_sd_a64(cpu1->cpu_int1_en0, cpu1->cpu_int1_enable0);
#endif
		break;

	case IPL_DDB:
	case IPL_HIGH:
		cpu0->cpu_int2_enable0 |= irq_mask;
		mips64_sd_a64(cpu0->cpu_int2_en0, cpu0->cpu_int2_enable0);
#ifdef MULTIPROCESSOR
		cpu1->cpu_int2_enable0 = cpu0->cpu_int2_enable0;
		mips64_sd_a64(cpu1->cpu_int2_en0, cpu1->cpu_int2_enable0);
#endif
		break;
	}

	mutex_exit(&octeon_intr_lock);

	return ih;
}

void
octeon_intr_disestablish(void *cookie)
{
	struct octeon_intrhand * const ih = cookie;
	const int irq = ih->ih_irq & (NIRQS-1);
	const int ipl = ih->ih_ipl;

	mutex_enter(&octeon_intr_lock);

	/*
	 * First disable it.
	 */
	const uint64_t irq_mask = ~__BIT(irq);
	struct cpu_softc * const cpu0 = &octeon_cpu0_softc;
#if MULTIPROCESSOR
	struct cpu_softc * const cpu1 = &octeon_cpu1_softc;
#endif

	switch (ipl) {
	case IPL_VM:
		cpu0->cpu_int0_enable0 &= ~irq_mask;
		mips64_sd_a64(cpu0->cpu_int0_en0, cpu0->cpu_int0_enable0);
		break;

	case IPL_SCHED:
		cpu0->cpu_int1_enable0 &= ~irq_mask;
		mips64_sd_a64(cpu0->cpu_int1_en0, cpu0->cpu_int1_enable0);
#ifdef MULTIPROCESSOR
		cpu1->cpu_int1_enable0 = cpu0->cpu_int1_enable0;
		mips64_sd_a64(cpu1->cpu_int1_en0, cpu1->cpu_int1_enable0);
#endif
		break;

	case IPL_DDB:
	case IPL_HIGH:
		cpu0->cpu_int2_enable0 &= ~irq_mask;
		mips64_sd_a64(cpu0->cpu_int2_en0, cpu0->cpu_int2_enable0);
#ifdef MULTIPROCESSOR
		cpu1->cpu_int2_enable0 = cpu0->cpu_int2_enable0;
		mips64_sd_a64(cpu1->cpu_int2_en0, cpu1->cpu_int2_enable0);
#endif
		break;
	}

	/*
	 * Now remove it since we shouldn't get interrupts for it.
	 */
	octeon_ciu_intrs[irq] = NULL;

	mutex_exit(&octeon_intr_lock);

	kmem_free(ih, sizeof(*ih));
}

void
octeon_iointr(int ipl, vaddr_t pc, uint32_t ipending)
{
	struct cpu_info * const ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;

	KASSERT((ipending & ~MIPS_INT_MASK) == 0);
	KASSERT(ipending & MIPS_HARD_INT_MASK);
	uint64_t hwpend = 0;

	if (ipending & MIPS_INT_MASK_2) {
		hwpend = mips64_ld_a64(cpu->cpu_int2_sum0)
		    & cpu->cpu_int2_enable0;
	} else if (ipending & MIPS_INT_MASK_1) {
		hwpend = mips64_ld_a64(cpu->cpu_int1_sum0)
		    & cpu->cpu_int1_enable0;
	} else if (ipending & MIPS_INT_MASK_0) {
		hwpend = mips64_ld_a64(cpu->cpu_int0_sum0)
		    & cpu->cpu_int0_enable0;
	} else {
		panic("octeon_iointr: unexpected ipending %#x", ipending);
	}
	while (hwpend != 0) {
		const int irq = ffs64(hwpend) - 1;
		hwpend &= ~__BIT(irq);
		
		struct octeon_intrhand * const ih = octeon_ciu_intrs[irq];
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
		}
	}
}

#ifdef MULTIPROCESSOR
__CTASSERT(NIPIS < 16);

int
octeon_ipi_intr(void *arg)
{
	struct cpu_info * const ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;
	uint64_t ipi_mask = (uintptr_t) arg;

	ipi_mask &= mips64_ld_a64(cpu->cpu_mbox_set);
	mips64_sd_a64(cpu->cpu_mbox_clr, ipi_mask);

	ipi_mask |= (ipi_mask >> 16);
	ipi_mask &= __BITS(15,0);

	KASSERT(ci->ci_cpl >= IPL_SCHED);
	KASSERT(ipi_mask < __BIT(NIPIS));

	/* if the request is clear, it was previously processed */
	if ((ci->ci_request_ipis & ipi_mask) == 0)
		return 0;

	atomic_or_64(&ci->ci_active_ipis, ipi_mask);
	atomic_and_64(&ci->ci_request_ipis, ~ipi_mask);

	ipi_process(ci, ipi_mask);

	atomic_and_64(&ci->ci_active_ipis, ~ipi_mask);

	return 1;
}

int
octeon_send_ipi(struct cpu_info *ci, int req)
{

	KASSERT(req < NIPIS);
	if (ci == NULL) {
		// only deals with 2 CPUs
		ci = cpuid_infos[(cpu_number() == 0) ? 1 : 0];
	}

	struct cpu_softc * const cpu = ci->ci_softc;
	uint64_t ipi_mask = __BIT(req);

	if (req == IPI_SUSPEND) {
		ipi_mask <<= 16;
	}

	mips64_sd_a64(cpu->cpu_mbox_set, ipi_mask);
	return 0;
}
#endif	/* MULTIPROCESSOR */

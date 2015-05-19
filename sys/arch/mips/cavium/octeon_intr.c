/*	$NetBSD: octeon_intr.c,v 1.2 2015/05/19 05:51:16 matt Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: octeon_intr.c,v 1.2 2015/05/19 05:51:16 matt Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

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
	[IPL_DDB] =		MIPS_INT_MASK,
	[IPL_HIGH] =		MIPS_INT_MASK,
    },
};

#define	NIRQS		64

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
	LIST_ENTRY(octeon_intrhand) ih_q;
	int (*ih_func)(void *);
	void *ih_arg;
	int ih_irq;
	int ih_ipl;
	int ih_intr;
};

struct octeon_intrhead {
	LIST_HEAD(, octeon_intrhand) intr_list;
	struct evcnt intr_count;
	int intr_refcnt;
};

struct octeon_intrhead octeon_ciu_intrtab[NIRQS];

struct octeon_cpuintr {
	LIST_HEAD(, octeon_intrhand) cintr_list;
	struct evcnt cintr_count;
};

#define	NINTRS		5	/* MIPS INT0 - INT4 */

struct octeon_cpuintr octeon_cpuintrs[NINTRS];
const char * const octeon_cpuintrnames[NINTRS] = {
	"int 0 (IP2)",
	"int 1 (IP3)",
	"int 2 ",
	"int 3 ",
	"int 4 ",
};


/* temporary interrupt enable bits */
uint64_t int0_enable0;
uint64_t int1_enable0;


#if 0
/*
 * NOTE: This routine must be called with interrupts disabled in the CPSR.
 */
static void
octeon_intr_calculate_masks(void)
{
	struct octeon_intrhand *ih;
	int level, ipl, irq;

	/* First, figure out which IPLs each INT has. */
	for (level = 0; level < NINTRS; level++) {
		int levels = 0;

		for (irq = 0; irq < NIRQS; irq++)
			LIST_FOREACH(ih, &octeon_intrtab[irq].intr_list, ih_q)
				if (ih->ih_intr == level)
					levels |= 1 << ih->ih_ipl;
		octeon_cpuintrs[level].cintr_levels = levels;
	}

	/* Next, figure out which INTs are used by each IPL. */
	for (ipl = 0; ipl < _IPL_N; ipl++) {
		int irqs = 0;

		for (level = 0; level < NINTRS; level++)
			if (1 << ipl & octeon_cpuintrs[level].cintr_levels)
				irqs |= MIPS_INT_MASK_0 << level;
		ipl_sr_bits[ipl] = irqs;
	}

	/*
	 * IPL_CLOCK should mask clock interrupt even if interrupt handler
	 * is not registered.
	 */
	ipl_sr_bits[IPL_CLOCK] |= MIPS_INT_MASK_5;

	/*
	 * IPL_NONE is used for hardware interrupts that are never blocked,
	 * and do not block anything else.
	 */
	ipl_sr_bits[IPL_NONE] = 0;

	/*
	 * Initialize the soft interrupt masks to block themselves.
	 */
	ipl_sr_bits[IPL_SOFTCLOCK] |= SI_TO_SRBIT(SI_SOFTCLOCK);
	ipl_sr_bits[IPL_SOFTNET] |= SI_TO_SRBIT(SI_SOFTNET);
	ipl_sr_bits[IPL_SOFTSERIAL] |= SI_TO_SRBIT(SI_SOFTSERIAL);

	/*
	 * Enforce a heirarchy that gives "slow" device (or devices with
	 * limited input buffer space/"real-time" requirements) a better
	 * chance at not dropping data.
	 */
	for (ipl = 1; ipl < _IPL_N; ipl++)
		ipl_sr_bits[ipl] |= ipl_sr_bits[ipl - 1];

	/*
	 * splhigh() must block "everything".
	 */
	ipl_sr_bits[IPL_HIGH] = MIPS_INT_MASK;

	/*
	 * Now compute which INTs must be blocked when servicing any
	 * given INT.
	 */
	for (level = 0; level < NINTRS; level++) {
		int irqs = (MIPS_INT_MASK_0 << level);

		for (irq = 0; irq < NIRQS; irq++)
			LIST_FOREACH(ih, &octeon_intrtab[irq].intr_list, ih_q)
				if (ih->ih_intr == level)
					irqs |= ipl_sr_bits[ih->ih_ipl];
		octeon_cpuintrs[level].cintr_mask = irqs;
	}

//	for (ipl = 0; ipl < _IPL_N; ipl++)
//		printf("debug: ipl_sr_bits[%.2d] = %.8x\n", ipl, ipl_sr_bits[ipl]);
}
#endif

void
octeon_intr_init(void)
{
	ipl_sr_map = octeon_ipl_sr_map;

	octeon_write_csr(CIU_INT0_EN0, 0);
	octeon_write_csr(CIU_INT1_EN0, 0);
	octeon_write_csr(CIU_INT32_EN0, 0);
	octeon_write_csr(CIU_INT0_EN1, 0);
	octeon_write_csr(CIU_INT1_EN1, 0);
	octeon_write_csr(CIU_INT32_EN1, 0);

	for (size_t i = 0; i < NINTRS; i++) {
		LIST_INIT(&octeon_cpuintrs[i].cintr_list);
		evcnt_attach_dynamic(&octeon_cpuintrs[i].cintr_count,
		    EVCNT_TYPE_INTR, NULL, "mips", octeon_cpuintrnames[i]);
	}

	for (size_t i = 0; i < NIRQS; i++) {
		LIST_INIT(&octeon_ciu_intrtab[i].intr_list);
		octeon_ciu_intrtab[i].intr_refcnt = 0;
		evcnt_attach_dynamic(&octeon_ciu_intrtab[i].intr_count,
		    EVCNT_TYPE_INTR, NULL, "octeon", octeon_intrnames[i]);
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
octeon_intr_establish(int irq, int req, int level,
    int (*func)(void *), void *arg)
{
	struct octeon_intrhand *ih;
	u_int64_t irq_mask;
	int s;

	if (irq >= NIRQS)
		panic("octeon_intr_establish: bogus IRQ %d", irq);
	if (req > 1)
		panic("octeon_intr_establish: bogus request %d", req);

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_irq = irq;
	ih->ih_ipl = level;
	ih->ih_intr = req;

	s = splhigh();

	/*
	 * First, link it into the tables.
	 * XXX do we want a separate list (really, should only be one item, not
	 *     a list anyway) per irq, not per CPU interrupt?
	 */
	LIST_INSERT_HEAD(&octeon_ciu_intrtab[irq].intr_list, ih, ih_q);

	/*
	 * Now enable it.
	 */
	if (octeon_ciu_intrtab[irq].intr_refcnt++ == 0) {
		irq_mask = 1ULL << irq;

		switch (req) {
		case 0:
			int0_enable0 |= irq_mask;
			octeon_write_csr(CIU_INT0_EN0, int0_enable0);
			break;

		case 1:
			int1_enable0 |= irq_mask;
			octeon_write_csr(CIU_INT1_EN0, int1_enable0);
			break;
		}
	}
	splx(s);

	return (ih);
}

void
octeon_intr_disestablish(void *cookie)
{
	struct octeon_intrhand *ih = cookie;
	u_int64_t irq_mask;
	int irq, req, s;

	irq = ih->ih_irq;
	req = ih->ih_intr;

	s = splhigh();

	/*
	 * First, remove it from the table.
	 */
	LIST_REMOVE(ih, ih_q);

	/*
	 * Now, disable it, if there is nothing remaining on the
	 * list.
	 */
	if (octeon_ciu_intrtab[irq].intr_refcnt-- == 1) {
		irq &= 63;	/* throw away high bit if set */
		req &= 1;	/* throw away high bit if set */
		irq_mask = ~(1ULL << irq);

		switch (req) {
		case 0:
			int0_enable0 &= irq_mask;
			octeon_write_csr(CIU_INT0_EN0, int0_enable0);
			break;

		case 1:
			int1_enable0 &= irq_mask;
			octeon_write_csr(CIU_INT1_EN0, int1_enable0);
			break;
		}
	}
	free(ih, M_DEVBUF);

	splx(s);
}

void
octeon_iointr(int ipl, vaddr_t pc, uint32_t ipending)
{
	struct octeon_intrhand *ih;
	int level, irq;
	uint64_t hwpend = 0;

	level = __builtin_clz(ipending);
	switch (level = 21 - level) {
	case 0: // MIPS_INT_MASK_0
		hwpend = octeon_read_csr(CIU_INT0_SUM0) & int0_enable0;
		break;

	case 1: // MIPS_INT_MASK_1
		hwpend = octeon_read_csr(CIU_INT1_SUM0) & int1_enable0;
		break;

	default:
		panic("octeon_iointr: illegal interrupt");
	}
	if ((irq = ffs64(hwpend) - 1) < 0)
		return;
	octeon_ciu_intrtab[irq].intr_count.ev_count++;
	LIST_FOREACH(ih, &octeon_ciu_intrtab[irq].intr_list, ih_q) {
		(*ih->ih_func)(ih->ih_arg);
	}
}

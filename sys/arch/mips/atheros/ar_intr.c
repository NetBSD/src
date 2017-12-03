/* $NetBSD: ar_intr.c,v 1.3.12.1 2017/12/03 11:36:26 jdolecek Exp $ */
/*
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * This code was written by Garrett D'Amore for the Champaign-Urbana
 * Community Wireless Network Project.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ar_intr.c,v 1.3.12.1 2017/12/03 11:36:26 jdolecek Exp $");

#define __INTR_PRIVATE

#include <sys/param.h>
#include <sys/intr.h>
#include <sys/cpu.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <mips/cpuregs.h>
#include <mips/locore.h>
#include <mips/atheros/include/platform.h>

#define	REGVAL(x)	*((volatile uint32_t *)(MIPS_PHYS_TO_KSEG1((x))))

/*
 * Only MISC interrupts are easily masked at the interrupt controller.
 * The others have to be masked at the source.
 */

#define	NINTRS	7	/* MIPS INT2-INT4 (7 is clock interrupt) */
#define	NIRQS	32	/* bits in Miscellaneous Interrupt Status Register */

struct atheros_intrhand {
	LIST_ENTRY(atheros_intrhand) ih_q;
	int (*ih_func)(void *);
	void *ih_arg;
	int ih_irq;
};

struct atheros_intr {
	LIST_HEAD(, atheros_intrhand) intr_qh;
	struct evcnt	intr_count;
};

static struct atheros_intr cpu_intrs[NINTRS];
static struct atheros_intr misc_intrs[NIRQS];

static uint32_t
misc_intstat_get(void)
{
	return REGVAL(platformsw->apsw_misc_intstat);
}

static void
misc_intstat_put(uint32_t v)
{
	REGVAL(platformsw->apsw_misc_intstat) = v;
}

static uint32_t
misc_intmask_get(void)
{
	return REGVAL(platformsw->apsw_misc_intmask);
}

static void
misc_intmask_put(uint32_t v)
{
	REGVAL(platformsw->apsw_misc_intmask) = v;
}


static void *
genath_cpu_intr_establish(int intr, int (*func)(void *), void *arg)
{
	struct atheros_intrhand	*ih;

	if ((ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT)) == NULL)
		return NULL;

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_irq = intr;

	const int s = splhigh();

	LIST_INSERT_HEAD(&cpu_intrs[intr].intr_qh, ih, ih_q);

	/*
	 * The MIPS CPU interrupts are enabled at boot time, so they
	 * should pretty much always be ready to go.
	 */

	splx(s);
	return (ih);
}

static void
genath_cpu_intr_disestablish(void *arg)
{
	struct atheros_intrhand	* const ih = arg;

	const int s = splhigh();

	LIST_REMOVE(ih, ih_q);

	splx(s);
	free(ih, M_DEVBUF);
}

static void *
genath_misc_intr_establish(int irq, int (*func)(void *), void *arg)
{
	struct atheros_intr * const intr = &misc_intrs[irq];
	struct atheros_intrhand	*ih;
	bool first;
	int s;


	if ((ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT)) == NULL)
		return NULL;

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_irq = irq;

	s = splhigh();

	first = LIST_EMPTY(&intr->intr_qh);

	LIST_INSERT_HEAD(&intr->intr_qh, ih, ih_q);

	if (first) {
		const uint32_t mask = misc_intmask_get() | __BIT(irq);
		misc_intmask_put(mask);
		(void) misc_intmask_get();	/* flush wbuffer */
	}

	splx(s);

	return ih;
}

static void
genath_misc_intr_disestablish(void *arg)
{
	struct atheros_intrhand	*ih = arg;
	struct atheros_intr * const intr = &misc_intrs[ih->ih_irq];

	const int s = splhigh();

	LIST_REMOVE(ih, ih_q);
	if (LIST_EMPTY(&intr->intr_qh)) {
		const uint32_t mask = misc_intmask_get() & ~__BIT(ih->ih_irq);
		misc_intmask_put(mask);
		(void) misc_intmask_get();	/* flush wbuffer */
	}

	splx(s);
	free(ih, M_DEVBUF);
}


static int
genath_misc_intr(void *arg)
{
	uint32_t		isr;
	uint32_t		mask;
	int			rv = 0;
	struct atheros_intr	*intr = arg;

	isr = misc_intstat_get();
	mask = misc_intmask_get();

	misc_intstat_put(isr & ~mask);

	isr &= mask;
	while (isr != 0) {
		struct atheros_intrhand	*ih;
		int index = 31 - __builtin_clz(isr & -isr); /* ffs */
		intr += index;

		intr->intr_count.ev_count++;
		LIST_FOREACH(ih, &intr->intr_qh, ih_q) {
			rv |= (*ih->ih_func)(ih->ih_arg);
		}
		isr >>= index + 1;
		intr++;
	}
	
	return rv;
}

static void
genath_iointr(int cpl, vaddr_t pc, uint32_t ipending)
{
	struct atheros_intr *intr = &cpu_intrs[NINTRS-1];

	/* move ipending to the most significant bits */
	ipending *= __BIT(31) / (MIPS_INT_MASK_0 << (NINTRS-1));
	while (ipending != 0) {
		struct atheros_intrhand	*ih;
		int index = __builtin_clz(ipending);

		intr -= index;
		ipending <<= index;
		KASSERT(ipending & __BIT(31));
		KASSERT(intr >= cpu_intrs);

		intr->intr_count.ev_count++;
		LIST_FOREACH(ih, &intr->intr_qh, ih_q) {
			(*ih->ih_func)(ih->ih_arg);
		}
		ipending <<= 1;
		intr--;
	}
}

static void
genath_intr_init(void)
{
	const struct atheros_platformsw * const apsw = platformsw;

	KASSERT(apsw->apsw_ipl_sr_map != NULL);
	ipl_sr_map = *apsw->apsw_ipl_sr_map;

	for (size_t i = 0; i < apsw->apsw_cpu_nintrs; i++) {
		if (apsw->apsw_cpu_intrnames[i] != NULL) {
			LIST_INIT(&cpu_intrs[i].intr_qh);
			evcnt_attach_dynamic(&cpu_intrs[i].intr_count,
			    EVCNT_TYPE_INTR, NULL, "cpu",
			    apsw->apsw_cpu_intrnames[i]);
		}
	}

	for (size_t i = 0; i < apsw->apsw_misc_nintrs; i++) {
		if (apsw->apsw_misc_intrnames[i] != NULL) {
			LIST_INIT(&misc_intrs[i].intr_qh);
			evcnt_attach_dynamic(&misc_intrs[i].intr_count,
			    EVCNT_TYPE_INTR, NULL, "misc",
			    apsw->apsw_misc_intrnames[i]);
		}
	}

	/* make sure we start without any misc interrupts enabled */
	(void) misc_intstat_get();
	misc_intmask_put(0);
	misc_intstat_put(0);

	/* make sure we register the MISC interrupt handler */
	genath_cpu_intr_establish(apsw->apsw_cpuirq_misc,
	    genath_misc_intr, misc_intrs);
}


const struct atheros_intrsw atheros_intrsw = {
	.aisw_init = genath_intr_init,
	.aisw_cpu_establish = genath_cpu_intr_establish,
	.aisw_cpu_disestablish = genath_cpu_intr_disestablish,
	.aisw_misc_establish = genath_misc_intr_establish,
	.aisw_misc_disestablish = genath_misc_intr_disestablish,
	.aisw_iointr = genath_iointr,
};


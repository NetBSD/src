/*	$NetBSD: rmixl_intr.c,v 1.1.2.9 2010/02/06 02:59:04 matt Exp $	*/

/*-
 * Copyright (c) 2007 Ruslan Ermilov and Vsevolod Lobko.
 * All rights reserved.
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
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Platform-specific interrupt support for the RMI XLP, XLR, XLS
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rmixl_intr.c,v 1.1.2.9 2010/02/06 02:59:04 matt Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <mips/cpu.h>
#include <mips/locore.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#ifdef IOINTR_DEBUG
int iointr_debug = IOINTR_DEBUG;
# define DPRINTF(x)	do { if (iointr_debug) printf x ; } while(0)
#else
# define DPRINTF(x)
#endif

#define RMIXL_PICREG_READ(off) \
	RMIXL_IOREG_READ(RMIXL_IO_DEV_PIC + (off))
#define RMIXL_PICREG_WRITE(off, val) \
	RMIXL_IOREG_WRITE(RMIXL_IO_DEV_PIC + (off), (val))
/*
 * This is a mask of bits to clear in the SR when we go to a
 * given hardware interrupt priority level.
 * _SR_BITS_DFLT bits are to be always clear (disabled)
 */
#define _SR_BITS_DFLT	(MIPS_INT_MASK_2|MIPS_INT_MASK_3|MIPS_INT_MASK_4)
const uint32_t ipl_sr_bits[_IPL_N] = {
	[IPL_NONE] = _SR_BITS_DFLT,
	[IPL_SOFTCLOCK] =
		_SR_BITS_DFLT
	      | MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] =
		_SR_BITS_DFLT
	      | MIPS_SOFT_INT_MASK_0
	      | MIPS_SOFT_INT_MASK_1,
	[IPL_VM] =
		_SR_BITS_DFLT
	      | MIPS_SOFT_INT_MASK_0
	      | MIPS_SOFT_INT_MASK_1
	      | MIPS_INT_MASK_0,
	[IPL_SCHED] =
		MIPS_INT_MASK,
};

/*
 * 'IRQs' here are indiividual interrupt sources
 * each has a slot in the Interrupt Redirection Table (IRT)
 * in the order listed
 *
 * NOTE: many irq sources depend on the chip family
 * XLS1xx vs. XLS2xx vs. XLS3xx vs. XLS6xx
 * use the right table for the CPU that's running.
 */

/*
 * rmixl_irqnames_xls1xx
 * - use for XLS1xx, XLS2xx, XLS4xx-Lite
 */
#define	NIRQS	32
static const char * const rmixl_irqnames_xls1xx[NIRQS] = {
	"int 0 (watchdog)",	/*  0 */
	"int 1 (timer0)",	/*  1 */
	"int 2 (timer1)",	/*  2 */
	"int 3 (timer2)",	/*  3 */
	"int 4 (timer3)",	/*  4 */
	"int 5 (timer4)",	/*  5 */
	"int 6 (timer5)",	/*  6 */
	"int 7 (timer6)",	/*  7 */
	"int 8 (timer7)",	/*  8 */
	"int 9 (uart0)",	/*  9 */
	"int 10 (uart1)",	/* 10 */
	"int 11 (i2c0)",	/* 11 */
	"int 12 (i2c1)",	/* 12 */
	"int 13 (pcmcia)",	/* 13 */
	"int 14 (gpio_a)",	/* 14 */
	"int 15 (irq15)",	/* 15 */
	"int 16 (bridge_tb)",	/* 16 */
	"int 17 (gmac0)",	/* 17 */
	"int 18 (gmac1)",	/* 18 */
	"int 19 (gmac2)",	/* 19 */
	"int 20 (gmac3)",	/* 20 */
	"int 21 (irq21)",	/* 21 */
	"int 22 (irq22)",	/* 22 */
	"int 23 (irq23)",	/* 23 */
	"int 24 (irq24)",	/* 24 */
	"int 25 (bridge_err)",	/* 25 */
	"int 26 (pcie_link0)",	/* 26 */
	"int 27 (pcie_link1)",	/* 27 */
	"int 28 (irq28)",	/* 28 */
	"int 29 (pcie_err)",	/* 29 */
	"int 30 (gpio_b)",	/* 30 */
	"int 31 (usb)",		/* 31 */
};

/*
 * rmixl_irqnames_xls4xx:
 * - use for XLS4xx, XLS6xx
 */
static const char * const rmixl_irqnames_xls4xx[NIRQS] = {
	"int 0 (watchdog)",	/*  0 */
	"int 1 (timer0)",	/*  1 */
	"int 2 (timer1)",	/*  2 */
	"int 3 (timer2)",	/*  3 */
	"int 4 (timer3)",	/*  4 */
	"int 5 (timer4)",	/*  5 */
	"int 6 (timer5)",	/*  6 */
	"int 7 (timer6)",	/*  7 */
	"int 8 (timer7)",	/*  8 */
	"int 9 (uart0)",	/*  9 */
	"int 10 (uart1)",	/* 10 */
	"int 11 (i2c0)",	/* 11 */
	"int 12 (i2c1)",	/* 12 */
	"int 13 (pcmcia)",	/* 13 */
	"int 14 (gpio_a)",	/* 14 */
	"int 15 (irq15)",	/* 15 */
	"int 16 (bridge_tb)",	/* 16 */
	"int 17 (gmac0)",	/* 17 */
	"int 18 (gmac1)",	/* 18 */
	"int 19 (gmac2)",	/* 19 */
	"int 20 (gmac3)",	/* 20 */
	"int 21 (irq21)",	/* 21 */
	"int 22 (irq22)",	/* 22 */
	"int 23 (irq23)",	/* 23 */
	"int 24 (irq24)",	/* 24 */
	"int 25 (bridge_err)",	/* 25 */
	"int 26 (pcie_link0)",	/* 26 */
	"int 27 (pcie_link1)",	/* 27 */
	"int 28 (pcie_link2)",	/* 28 */
	"int 29 (pcie_link3)",	/* 29 */
	"int 30 (gpio_b)",	/* 30 */
	"int 31 (usb)",		/* 31 */
};

/*
 * rmixl_irqnames_generic:
 * - use for unknown cpu implementation
 */
static const char * const rmixl_irqnames_generic[NIRQS] = {
	"int 0",	/*  0 */
	"int 1",	/*  1 */
	"int 2",	/*  2 */
	"int 3",	/*  3 */
	"int 4",	/*  4 */
	"int 5",	/*  5 */
	"int 6",	/*  6 */
	"int 7",	/*  7 */
	"int 8",	/*  8 */
	"int 9",	/*  9 */
	"int 10",	/* 10 */
	"int 11",	/* 11 */
	"int 12",	/* 12 */
	"int 13",	/* 13 */
	"int 14",	/* 14 */
	"int 15",	/* 15 */
	"int 16",	/* 16 */
	"int 17",	/* 17 */
	"int 18",	/* 18 */
	"int 19",	/* 19 */
	"int 20",	/* 20 */
	"int 21",	/* 21 */
	"int 22",	/* 22 */
	"int 23",	/* 23 */
	"int 24",	/* 24 */
	"int 25",	/* 25 */
	"int 26",	/* 26 */
	"int 27",	/* 27 */
	"int 28",	/* 28 */
	"int 29",	/* 29 */
	"int 30",	/* 30 */
	"int 31",	/* 31 */
};

/*
 * per-IRQ event stats
 */
struct rmixl_irqtab {
	struct evcnt irq_count;
	void *irq_ih;
};
static struct rmixl_irqtab rmixl_irqtab[NIRQS];


/*
 * 'vectors' here correspond to IRT Entry vector numbers
 * - IRT Entry vector# is bit# in EIRR
 * - note that EIRR[7:0] == CAUSE[15:8]
 * - we actually only use the first _IPL_N bits
 *   (less than 8)
 * 
 * each IRT entry gets routed to a vector
 * (if and when that interrupt is established)
 * the vectors are shared on a per-IPL basis
 * which simplifies dispatch
 *
 * XXX use of mips64 extended IRQs is TBD
 */
#define	NINTRVECS	_IPL_N

/*
 * translate IPL to vector number
 */
static const int rmixl_iplvec[_IPL_N] = {
	[IPL_NONE] = 		-1,	/* XXX */
	[IPL_SOFTCLOCK] =	 0,
	[IPL_SOFTNET] =		 1,
	[IPL_VM] =		 2,
	[IPL_SCHED] =		 3,
};

/*
 * list and ref count manage sharing of each vector
 */
struct rmixl_intrvec {
	LIST_HEAD(, evbmips_intrhand) iv_list;
	uint32_t iv_ack;
	rmixl_intr_trigger_t iv_trigger;
	rmixl_intr_polarity_t iv_polarity;
	u_int iv_refcnt;
};
static struct rmixl_intrvec rmixl_intrvec[NINTRVECS];

#ifdef DIAGNOSTIC
static int evbmips_intr_init_done;
#endif


static void rmixl_intr_irt_init(int);
static void rmixl_intr_irt_disestablish(int);
static void rmixl_intr_irt_establish(int, int, rmixl_intr_trigger_t,
		rmixl_intr_polarity_t, int);


static inline void
pic_irt_print(const char *s, const int n, u_int irq)
{
#ifdef IOINTR_DEBUG
	uint32_t c0, c1;

	c0 = RMIXL_PICREG_READ(RMIXL_PIC_IRTENTRYC0(irq));
	c1 = RMIXL_PICREG_READ(RMIXL_PIC_IRTENTRYC1(irq));
	printf("%s:%d: irq %d: c0 %#x, c1 %#x\n", s, n, irq, c0, c1);
#endif
}

void
evbmips_intr_init(void)
{
	uint32_t r;
	int i;

	KASSERT(cpu_rmixls(mips_options.mips_cpu));

#ifdef DIAGNOSTIC
	if (evbmips_intr_init_done != 0)
		panic("%s: evbmips_intr_init_done %d",
			__func__, evbmips_intr_init_done);
#endif

	for (i=0; i < NIRQS; i++) {
		evcnt_attach_dynamic(&rmixl_irqtab[i].irq_count,
			EVCNT_TYPE_INTR, NULL, "rmixl", rmixl_intr_string(i));
		rmixl_irqtab[i].irq_ih = NULL;
	}

	for (i=0; i < NINTRVECS; i++) {
		LIST_INIT(&rmixl_intrvec[i].iv_list);
		rmixl_intrvec[i].iv_ack = 0;
		rmixl_intrvec[i].iv_refcnt = 0;
	}

	/*
	 * disable watchdog NMI, timers
	 *
	 * XXX
	 *  WATCHDOG_ENB is preserved because clearing it causes
	 *  hang on the XLS616 (but not on the XLS408)
	 */
	r = RMIXL_PICREG_READ(RMIXL_PIC_CONTROL);
	r &= RMIXL_PIC_CONTROL_RESV|RMIXL_PIC_CONTROL_WATCHDOG_ENB;
	RMIXL_PICREG_WRITE(RMIXL_PIC_CONTROL, r);

	/*
	 * initialize all IRT Entries
	 */
	for (i=0; i < NIRQS; i++)
		rmixl_intr_irt_init(i);

	/*
	 * establish IRT entry for mips3 clock interrupt
	 */
	rmixl_intr_irt_establish(7, IPL_CLOCK, RMIXL_INTR_LEVEL,
		RMIXL_INTR_HIGH, rmixl_iplvec[IPL_CLOCK]);

#ifdef DIAGNOSTIC
	evbmips_intr_init_done = 1;
#endif
}

const char *
rmixl_intr_string(int irq)
{
	const char *name;

	if (irq < 0 || irq >= NIRQS)
		panic("%s: irq %d out of range, max %d",
			__func__, irq, NIRQS - 1);

	switch (MIPS_PRID_IMPL(mips_options.mips_cpu_id)) {
	case MIPS_XLS104:
	case MIPS_XLS108:
	case MIPS_XLS204:
	case MIPS_XLS208:
	case MIPS_XLS404LITE:
	case MIPS_XLS408LITE:
		name = rmixl_irqnames_xls1xx[irq];
		break;
	case MIPS_XLS404:
	case MIPS_XLS408:
	case MIPS_XLS416:
	case MIPS_XLS608:
	case MIPS_XLS616:
		name = rmixl_irqnames_xls4xx[irq];
		break;
	default:
		name = rmixl_irqnames_generic[irq];
		break;
	}

	return name;
}

/*
 * rmixl_intr_irt_init
 * - invalidate IRT Entry for irq
 * - unmask Thread#0 in low word (assume we only have 1 thread)
 */
static void
rmixl_intr_irt_init(int irq)
{
	uint32_t threads;

#if defined(MULTIPROCESSOR) && defined(NOTYET)
	/*
	 * XXX make sure the threads are ours?
	 */
	switch (MIPS_PRID_IMPL(mips_options.mips_cpu_id)) {
	case MIPS_XLS104:
	case MIPS_XLS204:
	case MIPS_XLS404:
	case MIPS_XLS404LITE:
		threads = __BITS(5,4) | __BITS(1,0);
		break;
	case MIPS_XLS108:
	case MIPS_XLS208:
	case MIPS_XLS408:
	case MIPS_XLS408LITE:
	case MIPS_XLS608:
		threads = __BITS(7,0);
		break;
	case MIPS_XLS416:
	case MIPS_XLS616:
		threads = __BITS(15,0);
		break;
	default:
		panic("%s: unknown cpu ID %#x\n", __func__,
			mips_options.mips_cpu_id);
	}
#else
	threads = 1;
#endif
	RMIXL_PICREG_WRITE(RMIXL_PIC_IRTENTRYC1(irq), 0);	/* high word */
	RMIXL_PICREG_WRITE(RMIXL_PIC_IRTENTRYC0(irq), threads);	/* low  word */
}

/*
 * rmixl_intr_irt_disestablish
 * - invalidate IRT Entry for irq
 * - writes to IRTENTRYC1 only; leave IRTENTRYC0 as-is
 */
static void
rmixl_intr_irt_disestablish(int irq)
{
	DPRINTF(("%s: irq %d, irtc1 %#x\n", __func__, irq, 0));
	RMIXL_PICREG_WRITE(RMIXL_PIC_IRTENTRYC1(irq), 0);	/* high word */
}

/*
 * rmixl_intr_irt_establish
 * - construct and IRT Entry for irq and write to PIC
 * - writes to IRTENTRYC1 only; assumes IRTENTRYC0 has been initialized
 */
static void
rmixl_intr_irt_establish(int irq, int ipl, rmixl_intr_trigger_t trigger,
	rmixl_intr_polarity_t polarity, int vec)
{
	uint32_t irtc1;

	irtc1  = RMIXL_PIC_IRTENTRYC1_VALID;
	irtc1 |= RMIXL_PIC_IRTENTRYC1_GL;	/* local */

	if (trigger == RMIXL_INTR_LEVEL)
		irtc1 |= RMIXL_PIC_IRTENTRYC1_TRG;

	if ((polarity == RMIXL_INTR_FALLING) || (polarity == RMIXL_INTR_LOW))
		irtc1 |= RMIXL_PIC_IRTENTRYC1_P;

	irtc1 |= vec;

	/*
	 * write IRT Entry to PIC (high word only)
	 */
	DPRINTF(("%s: irq %d, irtc1 %#x\n", __func__, irq, irtc1));
	RMIXL_PICREG_WRITE(RMIXL_PIC_IRTENTRYC1(irq), irtc1);
}

void *
rmixl_intr_establish(int irq, int ipl, rmixl_intr_trigger_t trigger,
	rmixl_intr_polarity_t polarity, int (*func)(void *), void *arg)
{
	struct evbmips_intrhand *ih;
	struct rmixl_intrvec *ivp;
	int vec;
	int s;

#ifdef DIAGNOSTIC
	if (evbmips_intr_init_done == 0)
		panic("%s: called before evbmips_intr_init", __func__);
#endif

	/*
	 * check args and assemble an IRT Entry
	 */
	if (irq < 0 || irq >= NIRQS)
		panic("%s: irq %d out of range, max %d",
			__func__, irq, NIRQS - 1);
	if (ipl <= 0 || ipl >= _IPL_N)
		panic("%s: ipl %d out of range, min %d, max %d",
			__func__, ipl, 1, _IPL_N - 1);
	if (rmixl_irqtab[irq].irq_ih != NULL)
		panic("%s: irq %d busy", __func__, irq);

	switch (trigger) {
	case RMIXL_INTR_EDGE:
	case RMIXL_INTR_LEVEL:
		break;
	default:
		panic("%s: bad trigger %d\n", __func__, trigger);
	}

	switch (polarity) {
	case RMIXL_INTR_RISING:
	case RMIXL_INTR_HIGH:
	case RMIXL_INTR_FALLING:
	case RMIXL_INTR_LOW:
		break;
	default:
		panic("%s: bad polarity %d\n", __func__, polarity);
	}

	/*
	 * ipl determines which vector to use
	 */
	vec = rmixl_iplvec[ipl];
	DPRINTF(("%s: irq %d, ipl %d, vec %d\n", __func__, irq, ipl, vec));
	KASSERT((vec & ~RMIXL_PIC_IRTENTRYC1_INTVEC) == 0);

	s = splhigh();

	ivp = &rmixl_intrvec[vec];
	if (ivp->iv_refcnt == 0) {
		ivp->iv_trigger = trigger;
		ivp->iv_polarity = polarity;
	} else {
		if (ivp->iv_trigger != trigger) {
#ifdef DIAGNOSTIC
			printf("%s: vec %d, irqs {", __func__, vec);
			LIST_FOREACH(ih, &ivp->iv_list, ih_q) {
				printf(" %d", ih->ih_irq);
			}
			printf(" } trigger type %d; irq %d wants type %d\n",
				ivp->iv_trigger, irq, trigger);
#endif
			panic("%s: trigger mismatch at vec %d\n",
				__func__, vec);
		}
		if (ivp->iv_polarity != polarity) {
#ifdef DIAGNOSTIC
			printf("%s: vec %d, irqs {", __func__, vec);
			LIST_FOREACH(ih, &ivp->iv_list, ih_q) {
				printf(" %d", ih->ih_irq);
			}
			printf(" } polarity type %d; irq %d wants type %d\n",
				ivp->iv_polarity, irq, polarity);
#endif
			panic("%s: polarity mismatch at vec %d\n",
				__func__, vec);
		}
	}
	ivp->iv_ack |= (1 << irq);

	/*
	 * allocate and initialize an interrupt handle
	 */
	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return NULL;

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_irq = irq;
	ih->ih_ipl = ipl;

	/*
	 * mark this irq as established, busy
	 */
	rmixl_irqtab[irq].irq_ih = ih;

	/*
	 * link this ih into the tables and bump reference count
	 */
	LIST_INSERT_HEAD(&ivp->iv_list, ih, ih_q);
	ivp->iv_refcnt++;

	/*
	 * establish IRT Entry
	 */
	rmixl_intr_irt_establish(irq, ipl, trigger, polarity, vec);

	splx(s);

	return ih;
}

void
rmixl_intr_disestablish(void *cookie)
{
	struct evbmips_intrhand *ih = cookie;
	struct rmixl_intrvec *ivp;
	int irq;
	int vec;
	int s;

	irq = ih->ih_irq;
	vec = rmixl_iplvec[ih->ih_ipl];
	ivp = &rmixl_intrvec[vec];

	s = splhigh();

	/*
	 * disable the IRT Entry (high word only)
	 */
	rmixl_intr_irt_disestablish(irq);

	/*
	 * remove from the table and adjust the reference count
	 */
	LIST_REMOVE(ih, ih_q);
	ivp->iv_refcnt--;
	ivp->iv_ack &= ~(1 << irq);

	/* 
	 * this irq now disestablished, not busy
	 */
	rmixl_irqtab[irq].irq_ih = NULL;

	splx(s);

	free(ih, M_DEVBUF);
}

void
evbmips_iointr(uint32_t status, uint32_t cause, uint32_t pc, uint32_t ipending)
{
	struct evbmips_intrhand *ih;
	struct rmixl_intrvec *ivp;
	int vec;
	uint64_t eirr;
#ifdef IOINTR_DEBUG
	uint64_t eimr;

	printf("%s: status %#x, cause %#x, pc %#x, ipending %#x\n",
		__func__, status, cause, pc, ipending); 

	asm volatile("dmfc0 %0, $9, 6;" : "=r"(eirr));
	asm volatile("dmfc0 %0, $9, 7;" : "=r"(eimr));
	printf("%s:%d: eirr %#lx, eimr %#lx\n", __func__, __LINE__, eirr, eimr);
#endif

	for (vec = NINTRVECS - 1; vec >= 2; vec--) {
		if ((ipending & (MIPS_SOFT_INT_MASK_0 << vec)) == 0)
			continue;

		ivp = &rmixl_intrvec[vec];

		eirr = 1ULL << vec;
		asm volatile("dmtc0 %0, $9, 6;" :: "r"(eirr));

#ifdef IOINTR_DEBUG
		printf("%s: interrupt at vec %d\n",
			__func__, vec); 
		if (LIST_EMPTY(&ivp->iv_list))
			printf("%s: unexpected interrupt at vec %d\n",
				__func__, vec); 
#endif
		LIST_FOREACH(ih, &ivp->iv_list, ih_q) {
			pic_irt_print(__func__, __LINE__, ih->ih_irq);
			RMIXL_PICREG_WRITE(RMIXL_PIC_INTRACK,
				(1 << ih->ih_irq));
			if ((*ih->ih_func)(ih->ih_arg) != 0) {
				rmixl_irqtab[ih->ih_irq].irq_count.ev_count++;
			}
		}

		cause &= ~(MIPS_SOFT_INT_MASK_0 << vec);
	}


	/* Re-enable anything that we have processed. */
	_splset(MIPS_SR_INT_IE | ((status & ~cause) & MIPS_HARD_INT_MASK));
}

#ifdef DEBUG
int rmixl_intrvec_print(void);
int
rmixl_intrvec_print(void)
{
	struct evbmips_intrhand *ih;
	struct rmixl_intrvec *ivp;
	int vec;

	ivp = &rmixl_intrvec[0];
	for (vec=0; vec < NINTRVECS ; vec++) {
		printf("vec %d, irqs {", vec);
		LIST_FOREACH(ih, &ivp->iv_list, ih_q)
			printf(" %d", ih->ih_irq);
		printf(" } trigger type %d\n", ivp->iv_trigger);
		ivp++;
	}
	return 0;
}
#endif

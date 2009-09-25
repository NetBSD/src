/*	$NetBSD: rmixl_intr.c,v 1.1.2.2 2009/09/25 22:22:09 cliff Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: rmixl_intr.c,v 1.1.2.2 2009/09/25 22:22:09 cliff Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <mips/locore.h>
#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given hardware interrupt priority level.
 */
const uint32_t ipl_sr_bits[_IPL_N] = {
	[IPL_NONE] = 0,
	[IPL_SOFTCLOCK] =
		MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] =
		MIPS_SOFT_INT_MASK_0
	      | MIPS_SOFT_INT_MASK_1,
	[IPL_VM] =
		MIPS_SOFT_INT_MASK_0
	      | MIPS_SOFT_INT_MASK_1
	      | MIPS_INT_MASK_0,
	[IPL_SCHED] =
		MIPS_SOFT_INT_MASK_0
	      | MIPS_SOFT_INT_MASK_1
	      | MIPS_INT_MASK_0
	      | MIPS_INT_MASK_1
	      | MIPS_INT_MASK_2
	      | MIPS_INT_MASK_3
	      | MIPS_INT_MASK_4
	      | MIPS_INT_MASK_5,
};

/*
 * 'IRQs' here are indiividual interrupt sources
 * each has a slot in the Interrupt Redirection Table (IRT)
 * in the order listed
 *
 * NOTE: many irq sources depend on the chip family
 * XLS1xx vs. XLS2xx vs. XLS3xx vs. XLS6xx
 * so just use generic names where they diverge
 */
#define	NIRQS	32
static const char *rmixl_irqnames[NIRQS] = {
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
	"int 29 (irq29)",	/* 29 */
	"int 30 (gpio_b)",	/* 30 */
	"int 31 (usb)",		/* 31 */
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
	u_int iv_refcnt;
};
static struct rmixl_intrvec rmixl_intrvec[NINTRVECS];


/*
 * register byte order is BIG ENDIAN regardless of code model
 */
#define REG_DEREF(o)					\
	*((volatile uint32_t *)MIPS_PHYS_TO_KSEG1( 	\
		rmixl_configuration.rc_io_pbase 	\
		+ RMIXL_IO_DEV_PIC + (o)))

#define REG_READ(o)	be32toh(REG_DEREF(o))
#define REG_WRITE(o,v)	REG_DEREF(o) = htobe32(v)

void
evbmips_intr_init(void)
{
	uint32_t r;
	int i;

	for (i=0; i < NIRQS; i++) {
		evcnt_attach_dynamic(&rmixl_irqtab[i].irq_count,
			EVCNT_TYPE_INTR, NULL, "rmixl", rmixl_irqnames[i]);
		rmixl_irqtab[i].irq_ih = NULL;
	}

	for (i=0; i < NINTRVECS; i++) {
		LIST_INIT(&rmixl_intrvec[i].iv_list);
		rmixl_intrvec[i].iv_refcnt = 0;
	}

	/*
	 * disable watchdog, watchdog NMI, timers
	 */
	r = REG_READ(RMIXL_PIC_CONTROL);
	r &= RMIXL_PIC_CONTROL_RESV;
	REG_WRITE(RMIXL_PIC_CONTROL, r);

	/*
	 * invalidate all IRT Entries
	 * permanently unmask Thread#0 in low word
	 * (assume we only have 1 thread)
	 */
	for (i=0; i < NIRQS; i++) {

		/* high word */
		r = REG_READ(RMIXL_PIC_IRTENTRYC1(i));
		r &= RMIXL_PIC_IRTENTRYC1_RESV;
		REG_WRITE(RMIXL_PIC_IRTENTRYC1(i), r);

		/* low word */
		r = REG_READ(RMIXL_PIC_IRTENTRYC0(i));
		r &= RMIXL_PIC_IRTENTRYC0_RESV;
		r |= 1;					/* Thread Mask */
		REG_WRITE(RMIXL_PIC_IRTENTRYC0(i), r);
	} 
}

void *
rmixl_intr_establish(int irq, int ipl, rmixl_intr_trigger_t trigger,
	rmixl_intr_polarity_t polarity, int (*func)(void *), void *arg)
{
	struct evbmips_intrhand *ih;
	uint32_t irtc1;
	int vec;
	int s;

	/*
	 * check args and assemble an IRT Entry
	 */
	if (irq < 0 || irq >= NIRQS)
		panic("%s: irq %d out of range, max %d",
			__func__, irq, NIRQS - 1);
	if (ipl < 0 || ipl >= _IPL_N)
		panic("%s: ipl %d out of range, max %d",
			__func__, ipl, _IPL_N - 1);
	if (rmixl_irqtab[irq].irq_ih != NULL)
		panic("%s: irq %d busy", __func__, irq);

	irtc1  = RMIXL_PIC_IRTENTRYC1_VALID;
	irtc1 |= RMIXL_PIC_IRTENTRYC1_GL;	/* local */

	switch (trigger) {
	case RMIXL_INTR_EDGE:
		break;
	case RMIXL_INTR_LEVEL:
		irtc1 |= RMIXL_PIC_IRTENTRYC1_TRG;
		break;
	default:
		panic("%s: bad trigger %d\n", __func__, trigger);
	}

	switch (polarity) {
	case RMIXL_INTR_RISING:
	case RMIXL_INTR_HIGH:
		break;
	case RMIXL_INTR_FALLING:
	case RMIXL_INTR_LOW:
		irtc1 |= RMIXL_PIC_IRTENTRYC1_P;
		break;
	default:
		panic("%s: bad polarity %d\n", __func__, polarity);
	}

	/*
	 * ipl determines which vector to use
	 */
	vec = rmixl_iplvec[ipl];
printf("%s: ipl=%d, vec=%d\n", __func__, ipl, vec);
	KASSERT((vec & ~RMIXL_PIC_IRTENTRYC1_INTVEC) == 0);
	irtc1 |= vec;

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

	s = splhigh();

	/*
	 * mark this irq as established, busy
	 */
	rmixl_irqtab[irq].irq_ih = ih;

	/*
	 * link this ih into the tables and bump reference count
	 */
	LIST_INSERT_HEAD(&rmixl_intrvec[vec].iv_list, ih, ih_q);
	rmixl_intrvec[vec].iv_refcnt++;

	/*
	 * establish IRT Entry (low word only)
	 */
	REG_WRITE(RMIXL_PIC_IRTENTRYC1(irq), irtc1);

	splx(s);

	return ih;
}

void
rmixl_intr_disestablish(void *cookie)
{
	struct evbmips_intrhand *ih = cookie;
	uint32_t r;
	int irq;
	int vec;
	int s;

	irq = ih->ih_irq;
	vec = rmixl_iplvec[ih->ih_ipl];

	s = splhigh();

	/*
	 * remove from the table and adjust the reference count
	 */
	LIST_REMOVE(ih, ih_q);
	rmixl_intrvec[vec].iv_refcnt--;

	/*
	 * disable the IRT Entry (low word only)
	 */
	r = REG_READ(RMIXL_PIC_IRTENTRYC1(irq));
	r &= RMIXL_PIC_IRTENTRYC1_RESV;
	REG_WRITE(RMIXL_PIC_IRTENTRYC1(irq), r);

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
	uint64_t eirr;
	uint64_t eimr;
	uint32_t sr;
	int vec;

	printf("\n%s: status: %#x, cause %#x\n", __func__, status, cause);
	asm volatile ("mfc0 %0, $9, 6;" :"=r"(sr));
	printf("%s:%d: SR: %#x\n", __func__, __LINE__, sr);
	asm volatile ("dmfc0 %0, $9, 7;" :"=r"(eimr));
	printf("%s: EIMR: %#lx\n", __func__, eimr);

	for (vec = NINTRVECS - 1; vec >= 0; vec--) {
		if ((ipending & (MIPS_SOFT_INT_MASK_0 << vec)) == 0)
			continue;

		/* ack this vec in the EIRR */
		eirr = (1 << vec);
		asm volatile ("dmtc0 %0, $9, 6;" :: "r"(eirr));

		LIST_FOREACH(ih, &rmixl_intrvec[vec].iv_list, ih_q) {
			if ((*ih->ih_func)(ih->ih_arg) != 0)
				rmixl_irqtab[ih->ih_irq].irq_count.ev_count++;
		}
		cause &= ~(MIPS_SOFT_INT_MASK_0 << vec);
	}

	/* Re-enable anything that we have processed. */
	printf("%s:%d: re-enable: %#x\n", __func__, __LINE__,
		MIPS_SR_INT_IE | ((status & ~cause) & MIPS_HARD_INT_MASK));
	_splset(MIPS_SR_INT_IE | ((status & ~cause) & MIPS_HARD_INT_MASK));

	asm volatile ("mfc0 %0, $9, 6;" :"=r"(sr));
	printf("%s: SR: %#x\n", __func__, sr);

	asm volatile ("dmfc0 %0, $9, 6;" :"=r"(eirr));
	printf("%s: EIRR: %#lx\n", __func__, eirr);

	asm volatile ("dmfc0 %0, $9, 7;" :"=r"(eimr));
	printf("%s: EIMR: %#lx\n", __func__, eimr);
}

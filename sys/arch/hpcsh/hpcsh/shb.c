/*	$NetBSD: shb.c,v 1.11 2002/02/28 01:57:00 uch Exp $	*/

/*-
 * Copyright (c) 1993, 1994 Charles Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "hd64465if.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <net/netisr.h>

#include <sh3/trapreg.h>
#include <sh3/intcreg.h>

#include <machine/shbvar.h>
#include <machine/autoconf.h>
#include <machine/debug.h>

#include <hpcsh/dev/hd64465/hd64465var.h>

int shbmatch(struct device *, struct cfdata *, void *);
void shbattach(struct device *, struct device *, void *);
int shbprint(void *, const char *);
int shbsearch(struct device *, struct cfdata *, void *);

void intr_calculatemasks(void);
int fakeintr(void *);
void *shb_intr_establish(int, int, int, int (*_fun)(void *), void *);
int intrhandler(int, int, int, int, struct trapframe);
int check_ipending(int, int, int, int, struct trapframe);
void Xsoftserial(void);
void Xsoftnet(void);
void Xsoftclock(void);

static int __nih;
static struct intrhand __intr_handler[ICU_LEN];
int intrmask[ICU_LEN], intrlevel[ICU_LEN];
struct intrhand *intrhand[ICU_LEN];

/* mask interrupt source */
void mask_irq_sh3(int);
void unmask_irq_sh3(int);
void mask_irq_sh4(int);
void unmask_irq_sh4(int);
#if defined(SH3) && defined(SH4)
static void (*mask_irq)(int);
static void (*unmask_irq)(int);
#elif defined (SH3)
#define mask_irq	mask_irq_sh3
#define unmask_irq	unmask_irq_sh3
#elif defined (SH4)
#define mask_irq	mask_irq_sh4
#define unmask_irq	unmask_irq_sh4
#endif

struct cfattach shb_ca = {
	sizeof(struct device), shbmatch, shbattach
};

int
shbmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, cf->cf_driver->cd_name))
		return (0);

        return (1);
}

void
shbattach(struct device *parent, struct device *self, void *aux)
{

	printf("\n");
#if defined(SH3) && defined(SH4)
	mask_irq = CPU_IS_SH3 ? mask_irq_sh3 : mask_irq_sh4;
	unmask_irq = CPU_IS_SH3 ? unmask_irq_sh3 : unmask_irq_sh4;
#endif

	config_search(shbsearch, self, NULL);

#include	"scif.h"
#include	"com.h"
#if (NSCIF > 0) || (NCOM > 0)
	shb_intr_establish(SIR_SERIAL, IST_LEVEL, IPL_SOFTSERIAL,
	    (int (*) (void *))Xsoftserial, NULL);
#endif
	shb_intr_establish(SIR_NET, IST_LEVEL, IPL_SOFTNET,
	    (int (*) (void *))Xsoftnet, NULL);

	shb_intr_establish(SIR_CLOCK, IST_LEVEL, IPL_SOFTCLOCK,
	    (int (*) (void *))Xsoftclock, NULL);
}

int
shbprint(void *aux, const char *isa)
{
	struct shb_attach_args *ia = aux;

	if (ia->ia_irq != IRQUNK)
		printf(" irq %d", ia->ia_irq);

	return (UNCONF);
}

int
shbsearch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct shb_attach_args ia;

	ia.ia_irq = cf->cf_irq;

	if ((*cf->cf_attach->ca_match)(parent, cf, &ia) > 0)
		config_attach(parent, cf, &ia, shbprint);

	return (0);
}

/*
 * Recalculate the interrupt masks from scratch.
 * We could code special registry and deregistry versions of this function that
 * would be faster, but the code would be nastier, and we don't expect this to
 * happen very much anyway.
 */
void
intr_calculatemasks()
{
	int irq, level;
	struct intrhand *q;

	/* First, figure out which levels each IRQ uses. */
	for (irq = 0; irq < ICU_LEN; irq++) {
		int levels = 0;
		for (q = intrhand[irq]; q; q = q->ih_next)
			levels |= 1 << q->ih_level;
		intrlevel[irq] = levels;
	}

	/* Then figure out which IRQs use each level. */
	for (level = 0; level < NIPL; level++) {
		int irqs = 0;
		for (irq = 0; irq < ICU_LEN; irq++)
			if (intrlevel[irq] & (1 << level))
				irqs |= 1 << irq;
		imask[level] = irqs;
	}

	/*
	 * Initialize soft interrupt masks to block themselves.
	 */
	imask[IPL_SOFTCLOCK] |= 1 << SIR_CLOCK;
	imask[IPL_SOFTNET] |= 1 << SIR_NET;
	imask[IPL_SOFTSERIAL] |= 1 << SIR_SERIAL;

	/*
	 * IPL_NONE is used for hardware interrupts that are never blocked,
	 * and do not block anything else.
	 */
	imask[IPL_NONE] = 0;

	/*
	 * Enforce a hierarchy that gives slow devices a better chance at not
	 * dropping data.
	 */
	imask[IPL_SOFTCLOCK] |= imask[IPL_NONE];
	imask[IPL_SOFTNET] |= imask[IPL_SOFTCLOCK];
	imask[IPL_BIO] |= imask[IPL_SOFTNET];
	imask[IPL_NET] |= imask[IPL_BIO];
	imask[IPL_SOFTSERIAL] |= imask[IPL_NET];
	imask[IPL_TTY] |= imask[IPL_SOFTSERIAL];

	/*
	 * There are tty, network and disk drivers that use free() at interrupt
	 * time, so imp > (tty | net | bio).
	 */
	imask[IPL_IMP] |= imask[IPL_TTY];

	imask[IPL_AUDIO] |= imask[IPL_IMP];

	/*
	 * Since run queues may be manipulated by both the statclock and tty,
	 * network, and disk drivers, clock > imp.
	 */
	imask[IPL_CLOCK] |= imask[IPL_AUDIO];

	/*
	 * IPL_HIGH must block everything that can manipulate a run queue.
	 */
	imask[IPL_HIGH] |= imask[IPL_CLOCK];

	/*
	 * We need serial drivers to run at the absolute highest priority to
	 * avoid overruns, so serial > high.
	 */
	imask[IPL_SERIAL] |= imask[IPL_HIGH];

	/* And eventually calculate the complete masks. */
	for (irq = 0; irq < ICU_LEN; irq++) {
		int irqs = 1 << irq;
		for (q = intrhand[irq]; q; q = q->ih_next)
			irqs |= imask[q->ih_level];
		intrmask[irq] = irqs;
	}
}

/*
 * Set up an interrupt handler to start being called.
 * XXX PRONE TO RACE CONDITIONS, UGLY, 'INTERESTING' INSERTION ALGORITHM.
 */
void *
shb_intr_establish(int irq, int type, int level, int (*ih_fun)(void *),
    void *ih_arg)
{
	struct intrhand **p, *q, *ih;
	int s;

	/* no point in sleeping unless someone can free memory. */
	if (__nih == ICU_LEN)
		panic("%s:: can't allocate handler info", __FUNCTION__);
	ih = &__intr_handler[__nih++];

	/*
	 * Figure out where to put the handler.
	 * This is O(N^2), but we want to preserve the order, and N is
	 * generally small.
	 */
	s = splhigh();
	for (p = &intrhand[irq]; (q = *p) != NULL; p = &q->ih_next)
		;

	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
	ih->ih_count = 0;
	ih->ih_next = NULL;
	ih->ih_level = level;
	ih->ih_irq = irq;
	*p = ih;

	intr_calculatemasks();

	/* unmask H/W interrupt mask register */
	if (irq < SHB_MAX_HARDINTR)
		unmask_irq(irq);
	splx(s);

	return (ih);
}

int
fakeintr(arg)
	void *arg;
{

	return 0;
}

/*ARGSUSED*/
int	/* 1 = check ipending on return, 0 = fast intr return */
intrhandler(int p1, int p2, int p3, int p4, /* dummy param */
    struct trapframe frame)
{
	unsigned int irl;
	struct intrhand *ih;
	unsigned int irq_num;
	int ocpl;

	irl = (unsigned int)frame.tf_trapno;

	if (irl >= INTEVT_SOFT) {
		/* This is software interrupt */
		irq_num = (irl - INTEVT_SOFT);
	} else if (irl == INTEVT_TMU0) {
		irq_num = TMU0_IRQ;
#ifdef SH4
	} else if (CPU_IS_SH4 && (irl & 0x0f00) == INTEVT_SCIF) {
		irq_num = SCIF_IRQ;
#endif
	} else {
		irq_num = (irl - 0x200) >> 5;
	}

	mask_irq(irq_num);

	if (cpl & (1 << irq_num)) {
		ipending |= (1 << irq_num);
		return (0);
	}

	ocpl = cpl;
	cpl |= intrmask[irq_num];
	ih = intrhand[irq_num];
	if (ih == NULL) {
		/* this is stray interrupt */
		cpl = ocpl;
		printf("stray interrupt. INTEVT=0x%x\n", frame.tf_trapno);
		return (1);
	}

	_cpu_intr_resume(0);
	while (ih) {
		if (ih->ih_arg)
			(*ih->ih_fun)(ih->ih_arg);
		else
			(*ih->ih_fun)(&frame);
		ih = ih->ih_next;
	}
	_cpu_intr_suspend();

	cpl = ocpl;

	unmask_irq(irq_num);

	return (1);
}

int	/* 1 = resume ihandler on return, 0 = go to fast intr return */
check_ipending(int p1, int p2, int p3, int p4, /* dummy param */
    struct trapframe frame)
{
	int ir, i, mask;

 restart:
	ir = (~cpl) & ipending;
	if (ir == 0)
		return (0);

	mask = 1 << IRQ_LOW;
	for (i = IRQ_LOW; i <= IRQ_HIGH; i++, mask <<= 1) {
		if (ir & mask)
			break;
	}
	if (IRQ_HIGH < i) {
		mask = 1 << SIR_LOW;
		for (i = SIR_LOW; i <= SIR_HIGH; i++, mask <<= 1) {
			if (ir & mask)
				break;
		}
	}

	if ((mask & ipending) == 0)
		goto restart;

	ipending &= ~mask;

	if (i < SHB_MAX_HARDINTR) {
		/* 
		 * set interrupt event register,
		 * this value is referenced in ihandler 
		 */
		_reg_write_4(SH_(INTEVT), (i << 5) + 0x200);
	} else {
		/* This is software interrupt */
		_reg_write_4(SH_(INTEVT), INTEVT_SOFT + i);
	}

	return (1);
}

#ifdef SH4
void
mask_irq_sh4(int irq)
{

	switch (irq) {
	case TMU0_IRQ:
		_reg_write_2(SH4_IPRA, _reg_read_2(SH4_IPRA) & ~((15) << 12));
		break;
	case SCIF_IRQ:
		_reg_write_2(SH4_IPRC, _reg_read_2(SH4_IPRC) & ~((15) << 4));
		break;
#if NHD64465IF > 0
	case 11:

		hd64465_intr_mask();
		break;
#endif /* NHD64465IF > 0 */
	default:
		if (irq < SHB_MAX_HARDINTR)
			printf("masked unknown irq(%d)!\n", irq);
	}
}

void
unmask_irq_sh4(int irq)
{

	switch (irq) {
	case TMU0_IRQ:
		_reg_write_2(SH4_IPRA,
		    _reg_read_2(SH4_IPRA) | ((15 - irq) << 12));
		break;
	case SCIF_IRQ:
		_reg_write_2(SH4_IPRC,
		    _reg_read_2(SH4_IPRC) | ((15 - irq) << 4));
		break;
#if NHD64465IF > 0
	case 11:
		hd64465_intr_unmask();
		break;
#endif /* NHD64465IF > 0 */
	default:
		if (irq < SHB_MAX_HARDINTR)
			printf("unmasked unknown irq(%d)!\n", irq);
	}
}
#endif /* SH4 */

#ifdef SH3
/*
 * work around for SH7709A broken IPR register is applied for SH7709.
 */
#define IPRA	0
#define IPRB	1
#define IPRC	2
#define IPRD	3
#define IPRE	4
static u_int16_t ipr[5];

void
mask_irq_sh3(int irq)
{

	switch (irq) {
	case TMU0_IRQ:
		ipr[IPRA] &= ~((15) << 12);
		_reg_write_2(SH3_IPRA, ipr[IPRA]);
		break;
	case SCIF_IRQ:
		ipr[IPRE] &= ~((15) << 4);
		_reg_write_2(SH7709_IPRE, ipr[IPRE]);
		break;
	case IRQ4_IRQ:
		ipr[IPRD] &= ~15;
		_reg_write_2(SH7709_IPRD, ipr[IPRD]);
		break;
	default:
		if (irq < SHB_MAX_HARDINTR)
			printf("masked unknown irq(%d)!\n", irq);
	}
}

void
unmask_irq_sh3(int irq)
{

	switch (irq) {
	case TMU0_IRQ:
		ipr[IPRA] |= ((15 - irq) << 12);
		_reg_write_2(SH3_IPRA, ipr[IPRA]);
		break;
	case SCIF_IRQ:
		ipr[IPRE] |= ((15 - irq) << 4);
		_reg_write_2(SH7709_IPRE, ipr[IPRE]);
		break;
	case IRQ4_IRQ:
		ipr[IPRD] |= (15 - irq);
		_reg_write_2(SH7709_IPRD, ipr[IPRD]);
		break;
	default:
		if (irq < SHB_MAX_HARDINTR)
			printf("unmasked unknown irq(%d)!\n", irq);
	}
}
#endif /* SH3 */

#if (NSCIF > 0) || (NCOM > 0)
void scifsoft(void *);
void comsoft(void *);

void
Xsoftserial(void)
{

#if (NSCIF > 0)
	scifsoft(NULL);
#endif
#if (NCOM > 0)
	comsoft(NULL);
#endif
}	
#endif /* NSCIF > 0 || NCOM > 0 */

void
Xsoftnet(void)
{
	int s, ni;

	s = splhigh();
	ni = netisr;
	netisr = 0;
	splx(s);

#define DONETISR(bit, fn) do {		\
	if (ni & (1 << bit))		\
		fn();			\
} while (0)

#include <net/netisr_dispatch.h>

#undef DONETISR
}

void
Xsoftclock(void)
{

        softclock(NULL);
}

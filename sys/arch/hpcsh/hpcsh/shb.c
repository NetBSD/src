/*	$NetBSD: shb.c,v 1.8 2002/02/11 17:32:36 uch Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <net/netisr.h>

#include <sh3/intcreg.h>
#include <sh3/trapreg.h>

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
void mask_irq(int);
void unmask_irq(int);
void Xsoftserial(void);
void Xsoftnet(void);
void Xsoftclock(void);
void __sh3_intr_debug_hook(void);
void __set_intr_level(u_int32_t);

static int __nih;
static struct intrhand __intr_handler[ICU_LEN];
int intrmask[ICU_LEN], intrlevel[ICU_LEN];
struct intrhand *intrhand[ICU_LEN];

struct cfattach shb_ca = {
	sizeof(struct device), shbmatch, shbattach
};

#define SH_SR_MD		0x40000000
#define SH_SR_RB		0x20000000
#define SH_SR_BL		0x10000000
#define SH_SR_FD		0x00008000
#define SH_SR_M			0x00000200
#define SH_SR_IMASK		0x000000f0
#define SH_SR_IMASK_SHIFT	4
#define SH_SR_S			0x00000002
#define SH_SR_T			0x00000001

void
__set_intr_level(u_int32_t m)
{
	u_int32_t sr;

	m &= SH_SR_IMASK;
	/* set new interrupt priority level */
	__asm__ __volatile__("stc	sr, %0" : "=r"(sr));
	sr &= ~SH_SR_IMASK;	
	sr |= m;
	__asm__ __volatile__("ldc	%0, sr" :: "r"(sr));
}

void
__sh3_intr_debug_hook()
{
	u_int32_t intevt = *(volatile u_int32_t *)0xffffffd8;
	u_int32_t intevt2 = *(volatile u_int32_t *)0xa4000000;
	if (intevt < 0xf00 && intevt != 0x420)
		printf("INTEVT=%08x INTEVT2=%08x\n", intevt, intevt2);
}

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

	config_search(shbsearch, self, NULL);

#include	"sci.h"
#include	"scif.h"
#include	"com.h"
#if (NSCI > 0) || (NSCIF > 0) || (NCOM > 0)
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
	} else if (irl == INTEVT_TMU1) {
		irq_num = TMU1_IRQ;
	} else if (IS_INTEVT_SCI0(irl)) {	/* XXX TOO DIRTY */
		irq_num = SCI_IRQ;
#ifdef SH4
	} else if ((irl & 0x0f00) == INTEVT_SCIF) {
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

	__set_intr_level(0);
	while (ih) {
		if (ih->ih_arg)
			(*ih->ih_fun)(ih->ih_arg);
		else
			(*ih->ih_fun)(&frame);
		ih = ih->ih_next;
	}
	__set_intr_level(15);

	cpl = ocpl;

	unmask_irq(irq_num);

	return (1);
}

int	/* 1 = resume ihandler on return, 0 = go to fast intr return */
check_ipending(int p1, int p2, int p3, int p4, /* dummy param */
    struct trapframe frame)
{
	int ir;
	int i;
	int mask;

 restart:
	ir = (~cpl) & ipending;
	if (ir == 0)
		return 0;

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
		/* set interrupt event register, this value is referenced in ihandler */
		SHREG_INTEVT = (i << 5) + 0x200;
	} else {
		/* This is software interrupt */
		SHREG_INTEVT = INTEVT_SOFT+i;
	}

	return 1;
}

#ifdef SH4
void
mask_irq(irq)
	int irq;
{
	switch (irq) {
	case TMU1_IRQ:
		SHREG_IPRA &= ~((15)<<8);
		break;
	case SCI_IRQ:
		SHREG_IPRB &= ~((15)<<4);
		break;
	case SCIF_IRQ:
		SHREG_IPRC &= ~((15)<<4);
		break;
	case 11:
		hd64465_intr_mask();
		break;
	default:
		if (irq < SHB_MAX_HARDINTR)
			printf("masked unknown irq(%d)!\n", irq);
	}
}

void
unmask_irq(irq)
	int irq;
{

	switch (irq) {
	case TMU1_IRQ:
		SHREG_IPRA |= ((15 - irq)<<8);
		break;
	case SCI_IRQ:
		SHREG_IPRB |= ((15 - irq)<<4);
		break;
	case SCIF_IRQ:
		SHREG_IPRC |= ((15 - irq)<<4);
		break;
	case 11:
		hd64465_intr_unmask();
		break;
	default:
		if (irq < SHB_MAX_HARDINTR)
			printf("unmasked unknown irq(%d)!\n", irq);
	}
}
#else /* SH4 */
#ifdef SH7709A_BROKEN_IPR	/* broken IPR patch */
#define IPRA	0
#define IPRB	1
#define IPRC	2
#define IPRD	3
#define IPRE	4
static unsigned short ipr[ 5 ];
#endif /* SH7709A_BROKEN_IPR */

void
mask_irq(int irq)
{
	switch (irq) {
	case TMU1_IRQ:
#ifdef SH7709A_BROKEN_IPR
		ipr[IPRA] &= ~((15)<<8);
		SHREG_IPRA = ipr[IPRA];
#else
		SHREG_IPRA &= ~((15)<<8);
#endif
		break;
	case SCI_IRQ:
#ifdef SH7709A_BROKEN_IPR
		ipr[IPRB] &= ~((15)<<4);
		SHREG_IPRB = ipr[IPRB];
#else
		SHREG_IPRB &= ~((15)<<4);
#endif
		break;
#if defined(SH7709) || defined(SH7709A) || defined(SH7729)
	case SCIF_IRQ:
#ifdef SH7709A_BROKEN_IPR
		ipr[IPRE] &= ~((15)<<4);
		SHREG_IPRE = ipr[IPRE];
#else
		SHREG_IPRE &= ~((15)<<4);
#endif
		break;
#endif
	case IRQ4_IRQ:
#ifdef SH7709A_BROKEN_IPR
		ipr[IPRD] &= ~15;
		SHREG_IPRD = ipr[IPRD];
#else
		SHREG_IPRD &= ~0xf;
#endif
		break;
	default:
		if (irq < SHB_MAX_HARDINTR)
			printf("masked unknown irq(%d)!\n", irq);
	}
}

void
unmask_irq(irq)
	int irq;
{

	switch (irq) {
	case TMU1_IRQ:
#ifdef SH7709A_BROKEN_IPR
		ipr[ IPRA ] |= ((15 - irq)<<8);
		SHREG_IPRA = ipr[ IPRA ];
#else
		SHREG_IPRA |= ((15 - irq)<<8);
#endif
		break;
	case SCI_IRQ:
#ifdef SH7709A_BROKEN_IPR
		ipr[IPRB] |= ((15 - irq)<<4);
		SHREG_IPRB = ipr[IPRB];
#else
		SHREG_IPRB |= ((15 - irq)<<4);
#endif
		break;
#if defined(SH7709) || defined(SH7709A) || defined(SH7729)
	case SCIF_IRQ:
#ifdef SH7709A_BROKEN_IPR
		ipr[ IPRE ] |= ((15 - irq)<<4);
		SHREG_IPRE = ipr[ IPRE ];
#else
		SHREG_IPRE |= ((15 - irq)<<4);
#endif
		break;
#endif
	case IRQ4_IRQ:
#ifdef SH7709A_BROKEN_IPR
		ipr[IPRD] |= (15 - irq);
		SHREG_IPRD = ipr[IPRD];
#else
		SHREG_IPRD |= (15 - irq);
#endif
		break;
	default:
		if (irq < SHB_MAX_HARDINTR)
			printf("unmasked unknown irq(%d)!\n", irq);
	}
}
#endif /* SH4 */

#if (NSCI > 0) || (NSCIF > 0) || (NCOM > 0)
void scisoft(void *);
void scifsoft(void *);
void comsoft(void *);

void
Xsoftserial(void)
{

#if (NSCI > 0)
	scisoft(NULL);
#endif
#if (NSCIF > 0)
	scifsoft(NULL);
#endif
#if (NCOM > 0)
	comsoft(NULL);
#endif
}	
#endif /* NSCI > 0 || NSCIF > 0 || NCOM > 0 */

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

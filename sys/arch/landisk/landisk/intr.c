/*	$NetBSD: intr.c,v 1.4.8.1 2012/04/17 00:06:34 yamt Exp $	*/

/*-
 * Copyright (C) 2005 NONAKA Kimihiro <nonaka@netbsd.org>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.4.8.1 2012/04/17 00:06:34 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <sh3/exception.h>

#include <machine/intr.h>

#define	_N_EXTINTR		8

#define	LANDISK_INTEN		0xb0000005
#define	INTEN_ALL_MASK		0x00

struct intrhand {
	int	(*ih_fun)(void *);
	void	*ih_arg;
	struct	intrhand *ih_next;
	int	ih_enable;
	int	ih_level;
	int	ih_irq;
	struct evcnt ih_evcnt;
};

struct extintr_handler {
	int		(*eih_func)(void *eih_arg);
	void		*eih_arg;
	struct intrhand	*eih_ih;
	int		eih_nih;
};

static struct extintr_handler extintr_handler[_N_EXTINTR];

static const char *extintr_names[_N_EXTINTR] = {
	"irq5", "irq6", "irq7", "irq8",
	"irq9", "irq10", "irq11", "irq12"
};

static int fakeintr(void *arg);
static int extintr_intr_handler(void *arg);

void
intc_intr(int ssr, int spc, int ssp)
{
	struct intc_intrhand *ih;
	struct clockframe cf;
	int evtcode;

	curcpu()->ci_data.cpu_nintr++;

	evtcode = _reg_read_4(SH4_INTEVT);
	ih = EVTCODE_IH(evtcode);
	KDASSERT(ih->ih_func);

	switch (evtcode) {
#if 0
#define	IRL(irq)	(0x200 + ((irq) << 5))
	case IRL(5): case IRL(6): case IRL(7): case IRL(8):
	case IRL(9): case IRL(10): case IRL(11): case IRL(12):
	{
		int level;
		uint8_t inten, bit;

		bit = 1 << (EVTCODE_TO_MAP_INDEX(evtcode) - 5);
		inten = _reg_read_1(LANDISK_INTEN);
		_reg_write_1(LANDISK_INTEN, inten & ~bit);
		level = (_IPL_NSOFT + 1) << 4;	/* disable softintr */
		ssr &= 0xf0;
		if (level < ssr)
			level = ssr;
		(void)_cpu_intr_resume(level);
		(*ih->ih_func)(ih->ih_arg);
		_reg_write_1(LANDISK_INTEN, inten);
		break;
	}
#endif
	default:
		(void)_cpu_intr_resume(ih->ih_level);
		(*ih->ih_func)(ih->ih_arg);
		break;

	case SH_INTEVT_TMU0_TUNI0:
		(void)_cpu_intr_resume(ih->ih_level);
		cf.spc = spc;
		cf.ssr = ssr;
		cf.ssp = ssp;
		(*ih->ih_func)(&cf);
		break;

	case SH_INTEVT_NMI:
		printf("NMI ignored.\n");
		break;
	}
}

void
intr_init(void)
{

	_reg_write_1(LANDISK_INTEN, INTEN_ALL_MASK);
}

void *
extintr_establish(int irq, int level, int (*ih_fun)(void *), void *ih_arg)
{
	static struct intrhand fakehand = {fakeintr};
	struct extintr_handler *eih;
	struct intrhand **p, *q, *ih;
	const char *name;
	int evtcode;
	int s;

	KDASSERT(irq >= 5 && irq <= 12);

	ih = malloc(sizeof(*ih), M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("intr_establish: can't malloc handler info");

	s = _cpu_intr_suspend();

	switch (level) {
	default:
#if defined(DEBUG)
		panic("extintr_establish: unknown level %d", level);
		/*NOTREACHED*/
#endif
	case IPL_VM:
		break;
	}

	eih = &extintr_handler[irq - 5];
	if (eih->eih_func == NULL) {
		evtcode = 0x200 + (irq << 5);
		eih->eih_func = intc_intr_establish(evtcode, IST_LEVEL, level,
		    extintr_intr_handler, eih);
	}

	/*
	 * Figure out where to put the handler.
	 * This is O(N^2), but we want to preserve the order, and N is
	 * generally small.
	 */
	for (p = &eih->eih_ih; (q = *p) != NULL; p = &q->ih_next)
		continue;

	/*
	 * Actually install a fake handler momentarily, since we might be doing
	 * this with interrupts enabled and don't want the real routine called
	 * until masking is set up.
	 */
	fakehand.ih_level = level;
	*p = &fakehand;

	/*
	 * Poke the real handler in now.
	 */
	memset(ih, 0, sizeof(*ih));
	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
	ih->ih_next = NULL;
	ih->ih_enable = 1;
	ih->ih_level = level;
	ih->ih_irq = irq - 5;
	name = extintr_names[irq - 5];
	evcnt_attach_dynamic(&ih->ih_evcnt, EVCNT_TYPE_INTR,
	    NULL, "ext", name);
	*p = ih;

	if (++eih->eih_nih == 1) {
		/* Unmask interrupt */
		_reg_bset_1(LANDISK_INTEN, (1 << (irq - 5)));
	}

	splx(s);

	return (ih);
}

void
extintr_disestablish(void *aux)
{
	struct intrhand *ih = aux;
	struct intrhand **p, *q;
	struct extintr_handler *eih;
	int irq;
	int s;

	KDASSERT(ih != NULL);

	s = _cpu_intr_suspend();

	irq = ih->ih_irq;
	eih = &extintr_handler[irq];

	/*
	 * Remove the handler from the chain.
	 * This is O(n^2), too.
	 */
	for (p = &eih->eih_ih; (q = *p) != NULL && q != ih; p = &q->ih_next)
		continue;
	if (q == NULL)
		panic("extintr_disestablish: handler not registered");

	*p = q->ih_next;

	evcnt_detach(&ih->ih_evcnt);

	free((void *)ih, M_DEVBUF);

	if (--eih->eih_nih == 0) {
		intc_intr_disestablish(eih->eih_func);

		/* Mask interrupt */
		_reg_bclr_1(LANDISK_INTEN, (1 << irq));
	}

	splx(s);
}

void
extintr_enable(void *aux)
{
	struct intrhand *ih = aux;
	struct intrhand *p, *q;
	struct extintr_handler *eih;
	int irq;
	int cnt;
	int s;

	KDASSERT(ih != NULL);

	s = _cpu_intr_suspend();

	irq = ih->ih_irq;
	KDASSERT(irq >= 0 && irq < 8);
	eih = &extintr_handler[irq];
	for (cnt = 0, p = eih->eih_ih, q = NULL; p != NULL; p = p->ih_next) {
		if (p->ih_enable) {
			cnt++;
		}
		if (p == ih) {
			q = p;
			p->ih_enable = 1;
		}
	}
	KDASSERT(q != NULL);

	if (cnt == 0) {
		/* Unmask interrupt */
		_reg_bset_1(LANDISK_INTEN, (1 << irq));
	}

	splx(s);
}

void
extintr_disable(void *aux)
{
	struct intrhand *ih = aux;
	struct intrhand *p, *q;
	struct extintr_handler *eih;
	int irq;
	int cnt;
	int s;

	KDASSERT(ih != NULL);

	s = _cpu_intr_suspend();

	irq = ih->ih_irq;
	KDASSERT(irq >= 0 && irq < 8);
	eih = &extintr_handler[irq];
	for (cnt = 0, p = eih->eih_ih, q = NULL; p != NULL; p = p->ih_next) {
		if (p == ih) {
			q = p;
			p->ih_enable = 0;
		}
		if (!ih->ih_enable) {
			cnt++;
		}
	}
	KDASSERT(q != NULL);

	if (cnt == 0) {
		/* Mask interrupt */
		_reg_bclr_1(LANDISK_INTEN, (1 << irq));
	}

	splx(s);
}

void
extintr_disable_by_num(int irq)
{
	struct extintr_handler *eih;
	struct intrhand *ih;
	int s;

	KDASSERT(irq >= 5 && irq <= 12);

	s = _cpu_intr_suspend();
	eih = &extintr_handler[irq - 5];
	for (ih = eih->eih_ih; ih != NULL; ih = ih->ih_next) {
		ih->ih_enable = 0;
	}
	/* Mask interrupt */
	_reg_bclr_1(LANDISK_INTEN, (1 << irq));
	splx(s);
}

static int
fakeintr(void *arg)
{

	return 0;
}

static int
extintr_intr_handler(void *arg)
{
	struct extintr_handler *eih = arg;
	struct intrhand *ih;
	int r;

	if (__predict_true(eih != NULL)) {
		for (ih = eih->eih_ih; ih != NULL; ih = ih->ih_next) {
			if (__predict_true(ih->ih_enable)) {
				r = (*ih->ih_fun)(ih->ih_arg);
				if (__predict_true(r != 0)) {
					ih->ih_evcnt.ev_count++;
				}
			}
		}
		return 1;
	}
	return 0;
}

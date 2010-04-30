/*	$NetBSD: ap_ms104_sh4_intr.c,v 1.1.2.2 2010/04/30 14:39:19 uebayasi Exp $	*/

/*-
 * Copyright (c) 2009 NONAKA Kimihiro <nonaka@netbsd.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ap_ms104_sh4_intr.c,v 1.1.2.2 2010/04/30 14:39:19 uebayasi Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <sh3/devreg.h>
#include <sh3/exception.h>

#include <machine/intr.h>

#include <evbsh3/ap_ms104_sh4/ap_ms104_sh4reg.h>
#include <evbsh3/ap_ms104_sh4/ap_ms104_sh4var.h>

#define	_N_EXTINTR	16

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
	void		*eih_func;
	struct intrhand	*eih_ih;
	int		eih_nih;
};
static struct extintr_handler extintr_handler[_N_EXTINTR];

static const char *extintr_names[_N_EXTINTR] = {
	"irq0", "irq1", "irq2", "irq3",
	"irq4", "irq5", "irq6", "irq7",
	"irq8", "irq9", "irq10", "irq11",
	"irq12", "irq13", "irq14", "irq15"
};

static int fakeintr(void *arg);
static int extintr_intr_handler(void *arg);

void
extintr_init(void)
{

	_reg_write_1(EXTINTR_MASK1, 0);
	_reg_write_1(EXTINTR_MASK2, 0);
	_reg_write_1(EXTINTR_MASK3, 0);
	_reg_write_1(EXTINTR_MASK4, 0);
}

/*ARGSUSED*/
static int
fakeintr(void *arg)
{

	return 0;
}

void *
extintr_establish(int irq, int trigger, int level,
    int (*ih_fun)(void *), void *ih_arg)
{
	static struct intrhand fakehand = {fakeintr};
	struct extintr_handler *eih;
	struct intrhand **p, *q, *ih;
	const char *name;
	int evtcode;
	int s;

	KDASSERT(irq >= 1 && irq <= 14);

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

	eih = &extintr_handler[irq];
	if (eih->eih_func == NULL) {
		evtcode = 0x200 + (irq << 5);
		eih->eih_func = intc_intr_establish(evtcode, trigger, level,
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
	ih->ih_irq = irq;
	name = extintr_names[irq];
	evcnt_attach_dynamic(&ih->ih_evcnt, EVCNT_TYPE_INTR, NULL, "ext", name);
	*p = ih;

	if (++eih->eih_nih == 1) {
		uint8_t reg;

		/* Unmask interrupt */
		switch (irq) {
		case 1: case 2:
			reg = _reg_read_1(EXTINTR_MASK4);
			reg |= 1 << (2 - irq);
			_reg_write_1(EXTINTR_MASK4, reg);
			break;

		case 3: case 4: case 5: case 6:
			reg = _reg_read_1(EXTINTR_MASK3);
			reg |= 1 << (6 - irq);
			_reg_write_1(EXTINTR_MASK3, reg);
			break;

		case 7: case 8: case 9: case 10:
			reg = _reg_read_1(EXTINTR_MASK2);
			reg |= 1 << (10 - irq);
			_reg_write_1(EXTINTR_MASK2, reg);
			break;

		case 11: case 12: case 13: case 14:
			reg = _reg_read_1(EXTINTR_MASK1);
			reg |= 1 << (14 - irq);
			_reg_write_1(EXTINTR_MASK1, reg);
			break;

		default:
			panic("unknown irq%d\n", irq);
			/*NOTREACHED*/
			break;
		}
	}

	splx(s);

	return (ih);
}

void
extintr_disestablish(void *cookie)
{
	struct intrhand *ih = (struct intrhand *)cookie;
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
		uint8_t reg;

		intc_intr_disestablish(eih->eih_func);
		eih->eih_func = NULL;

		/* Mask interrupt */
		switch (irq) {
		case 1: case 2:
			reg = _reg_read_1(EXTINTR_MASK4);
			reg &= ~(1 << (2 - irq));
			_reg_write_1(EXTINTR_MASK4, reg);
			break;

		case 3: case 4: case 5: case 6:
			reg = _reg_read_1(EXTINTR_MASK3);
			reg &= ~(1 << (6 - irq));
			_reg_write_1(EXTINTR_MASK3, reg);
			break;

		case 7: case 8: case 9: case 10:
			reg = _reg_read_1(EXTINTR_MASK2);
			reg &= ~(1 << (10 - irq));
			_reg_write_1(EXTINTR_MASK2, reg);
			break;

		case 11: case 12: case 13: case 14:
			reg = _reg_read_1(EXTINTR_MASK1);
			reg &= ~(1 << (14 - irq));
			_reg_write_1(EXTINTR_MASK1, reg);
			break;

		default:
			panic("unknown irq%d\n", irq);
			/*NOTREACHED*/
			break;
		}
	}

	splx(s);
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

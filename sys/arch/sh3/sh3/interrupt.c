/*	$NetBSD: interrupt.c,v 1.14 2005/06/29 16:56:58 christos Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.14 2005/06/29 16:56:58 christos Exp $");

#include <sys/param.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>	/* uvmexp.intrs */

#include <net/netisr.h>

#include <sh3/exception.h>
#include <sh3/clock.h>
#include <sh3/intcreg.h>
#include <sh3/tmureg.h>
#include <machine/intr.h>

void intc_intr_priority(int, int);
struct intc_intrhand *intc_alloc_ih(void);
void intc_free_ih(struct intc_intrhand *);
int intc_unknown_intr(void *);
void netintr(void);
void tmu1_oneshot(void);
int tmu1_intr(void *);
void tmu2_oneshot(void);
int tmu2_intr(void *);

/*
 * EVTCODE to intc_intrhand mapper.
 * max #60 is SH7709_INTEVT2_ADC_ADI (0x980)
 */
int8_t __intc_evtcode_to_ih[64];

struct intc_intrhand __intc_intrhand[_INTR_N + 1] = {
	/* Place holder interrupt handler for unregistered interrupt. */
	[0] = { .ih_func = intc_unknown_intr, .ih_level = 0xf0 }
};

struct sh_soft_intr sh_soft_intrs[_IPL_NSOFT];
struct sh_soft_intrhand *softnet_intrhand;

/*
 * SH INTC support.
 */
void
intc_init()
{

	switch (cpu_product) {
	case CPU_PRODUCT_7709:
	case CPU_PRODUCT_7709A:
		_reg_write_2(SH7709_IPRC, 0);
		_reg_write_2(SH7709_IPRD, 0);
		_reg_write_2(SH7709_IPRE, 0);
		/* FALLTHROUGH */
	case CPU_PRODUCT_7708:
	case CPU_PRODUCT_7708S:
	case CPU_PRODUCT_7708R:
		_reg_write_2(SH3_IPRA, 0);
		_reg_write_2(SH3_IPRB, 0);
		break;
	case CPU_PRODUCT_7750S:
	case CPU_PRODUCT_7750R:
	case CPU_PRODUCT_7751:
	case CPU_PRODUCT_7751R: 
		_reg_write_2(SH4_IPRD, 0);
		/* FALLTHROUGH */
	case CPU_PRODUCT_7750:
		_reg_write_2(SH4_IPRA, 0);
		_reg_write_2(SH4_IPRB, 0);
		_reg_write_2(SH4_IPRC, 0);
		break;
	}
}

void *
intc_intr_establish(int evtcode, int trigger, int level,
    int (*ih_func)(void *), void *ih_arg)
{
	struct intc_intrhand *ih;

	KDASSERT(evtcode >= 0x200 && level > 0);

	ih = intc_alloc_ih();
	ih->ih_func	= ih_func;
	ih->ih_arg	= ih_arg;
	ih->ih_level	= level << 4;	/* convert to SR.IMASK format. */
	ih->ih_evtcode	= evtcode;

	/* Map interrupt handler */
	EVTCODE_TO_IH_INDEX(evtcode) = ih->ih_idx;

	/* Priority */
	intc_intr_priority(evtcode, level);

	/* Sense select (SH7709, SH7709A only) XXX notyet */

	return (ih);
}

void
intc_intr_disestablish(void *arg)
{
	struct intc_intrhand *ih = arg;
	int evtcode = ih->ih_evtcode;

	/* Mask interrupt if IPR can manage it. if not, cascaded ICU will do */
	intc_intr_priority(evtcode, 0);

	/* Unmap interrupt handler */
	EVTCODE_TO_IH_INDEX(evtcode) = 0;

	intc_free_ih(ih);
}

void
intc_intr_disable(int evtcode)
{
	int s;

	s = _cpu_intr_suspend();
	KASSERT(EVTCODE_TO_IH_INDEX(evtcode) != 0); /* there is a handler */
	intc_intr_priority(evtcode, 0);
	_cpu_intr_resume(s);
}

void
intc_intr_enable(int evtcode)
{
	struct intc_intrhand *ih;
	int s;

	s = _cpu_intr_suspend();
	KASSERT(EVTCODE_TO_IH_INDEX(evtcode) != 0); /* there is a handler */
	ih = EVTCODE_IH(evtcode);
	/* ih_level is in the SR.IMASK format */
	intc_intr_priority(evtcode, (ih->ih_level >> 4));
	_cpu_intr_resume(s);
}


/*
 * int intc_intr_priority(int evtcode, int level)
 *	Setup interrupt priority register.
 *	SH7708, SH7708S, SH7708R, SH7750, SH7750S ... evtcode is INTEVT
 *	SH7709, SH7709A				  ... evtcode is INTEVT2
 */
void
intc_intr_priority(int evtcode, int level)
{
	volatile uint16_t *iprreg;
	int pos;
	uint16_t r;

#define	__SH_IPR(_sh, _ipr, _pos)					   \
	do {								   \
		iprreg = (volatile uint16_t *)(SH ## _sh ## _IPR ## _ipr); \
		pos = (_pos);						   \
	} while (/*CONSTCOND*/0)

#define	SH3_IPR(_ipr, _pos)		__SH_IPR(3, _ipr, _pos)
#define	SH4_IPR(_ipr, _pos)		__SH_IPR(4, _ipr, _pos)
#define	SH7709_IPR(_ipr, _pos)		__SH_IPR(7709, _ipr, _pos)

#define	SH_IPR(_ipr, _pos)						\
	do {								\
		if (CPU_IS_SH3)						\
			SH3_IPR(_ipr, _pos);				\
		else							\
			SH4_IPR(_ipr, _pos);				\
	} while (/*CONSTCOND*/0)

	iprreg = 0;
	pos = -1;

	switch (evtcode) {
	case SH_INTEVT_TMU0_TUNI0:
		SH_IPR(A, 12);
		break;
	case SH_INTEVT_TMU1_TUNI1:
		SH_IPR(A, 8);
		break;
	case SH_INTEVT_TMU2_TUNI2:
		SH_IPR(A, 4);
		break;
	case SH_INTEVT_SCI_ERI:
	case SH_INTEVT_SCI_RXI:
	case SH_INTEVT_SCI_TXI:
	case SH_INTEVT_SCI_TEI:
		SH_IPR(B, 4);
		break;
	}

	if (CPU_IS_SH3)
		switch (evtcode) {
		case SH7709_INTEVT2_IRQ3:
			SH7709_IPR(C, 12);
			break;
		case SH7709_INTEVT2_IRQ2:
			SH7709_IPR(C, 8);
			break;
		case SH7709_INTEVT2_IRQ1:
			SH7709_IPR(C, 4);
			break;
		case SH7709_INTEVT2_IRQ0:
			SH7709_IPR(C, 0);
			break;
		case SH7709_INTEVT2_PINT07:
			SH7709_IPR(D, 12);
			break;
		case SH7709_INTEVT2_PINT8F:
			SH7709_IPR(D, 8);
			break;
		case SH7709_INTEVT2_IRQ5:
			SH7709_IPR(D, 4);
			break;
		case SH7709_INTEVT2_IRQ4:
			SH7709_IPR(D, 0);
			break;
		case SH7709_INTEVT2_DEI0:
		case SH7709_INTEVT2_DEI1:
		case SH7709_INTEVT2_DEI2:
		case SH7709_INTEVT2_DEI3:
			SH7709_IPR(E, 12);
			break;
		case SH7709_INTEVT2_IRDA_ERI:
		case SH7709_INTEVT2_IRDA_RXI:
		case SH7709_INTEVT2_IRDA_BRI:
		case SH7709_INTEVT2_IRDA_TXI:
			SH7709_IPR(E, 8);
			break;
		case SH7709_INTEVT2_SCIF_ERI:
		case SH7709_INTEVT2_SCIF_RXI:
		case SH7709_INTEVT2_SCIF_BRI:
		case SH7709_INTEVT2_SCIF_TXI:
			SH7709_IPR(E, 4);
			break;
		case SH7709_INTEVT2_ADC:
			SH7709_IPR(E, 0);
			break;
		}
	else
		switch (evtcode) {
		case SH4_INTEVT_SCIF_ERI:
		case SH4_INTEVT_SCIF_RXI:
		case SH4_INTEVT_SCIF_BRI:
		case SH4_INTEVT_SCIF_TXI:
			SH4_IPR(C, 4);
			break;
		}

	/*
	 * XXX: This function gets called even for interrupts that
	 * don't have their priority defined by IPR registers.
	 */
	if (pos < 0)
		return;

	r = _reg_read_2(iprreg);
	r = (r & ~(0xf << (pos))) | (level << (pos));
	_reg_write_2(iprreg, r);
}

/*
 * Interrupt handler holder allocater.
 */
struct intc_intrhand *
intc_alloc_ih()
{
	/* #0 is reserved for unregistered interrupt. */
	struct intc_intrhand *ih = &__intc_intrhand[1];
	int i;

	for (i = 1; i <= _INTR_N; i++, ih++)
		if (ih->ih_idx == 0) {	/* no driver uses this. */
			ih->ih_idx = i;	/* register myself */
			return (ih);
		}

	panic("increase _INTR_N greater than %d", _INTR_N);
	return (NULL);
}

void
intc_free_ih(struct intc_intrhand *ih)
{

	memset(ih, 0, sizeof(*ih));
}

/* Place-holder for debugging */
int
intc_unknown_intr(void *arg)
{

	printf("INTEVT=0x%x", _reg_read_4(SH_(INTEVT)));
	if (cpu_product == CPU_PRODUCT_7709 || cpu_product == CPU_PRODUCT_7709A)
		printf(" INTEVT2=0x%x", _reg_read_4(SH7709_INTEVT2));
	printf("\n");

	panic("unknown interrupt");
	/* NOTREACHED */
	return (0);
}

/*
 * Software interrupt support
 */
void
softintr_init()
{
	static const char *softintr_names[] = IPL_SOFTNAMES;
	struct sh_soft_intr *asi;
	int i;

	for (i = 0; i < _IPL_NSOFT; i++) {
		asi = &sh_soft_intrs[i];
		TAILQ_INIT(&asi->softintr_q);

		asi->softintr_ipl = IPL_SOFT + i;
		simple_lock_init(&asi->softintr_slock);
		evcnt_attach_dynamic(&asi->softintr_evcnt, EVCNT_TYPE_INTR,
		    NULL, "soft", softintr_names[i]);
	}

	/* XXX Establish legacy soft interrupt handlers. */
	softnet_intrhand = softintr_establish(IPL_SOFTNET,
	    (void (*)(void *))netintr, NULL);
	KDASSERT(softnet_intrhand != NULL);

	intc_intr_establish(SH_INTEVT_TMU1_TUNI1, IST_LEVEL, IPL_SOFT,
	    tmu1_intr, NULL);
	intc_intr_establish(SH_INTEVT_TMU2_TUNI2, IST_LEVEL, IPL_SOFTNET,
	    tmu2_intr, NULL);
}

void
softintr_dispatch(int ipl)
{
	struct sh_soft_intr *asi;
	struct sh_soft_intrhand *sih;
	int s;

	s = _cpu_intr_suspend();

	asi = &sh_soft_intrs[ipl - IPL_SOFT];

	if (TAILQ_FIRST(&asi->softintr_q) != NULL)
		asi->softintr_evcnt.ev_count++;

	while ((sih = TAILQ_FIRST(&asi->softintr_q)) != NULL) {
		TAILQ_REMOVE(&asi->softintr_q, sih, sih_q);
		sih->sih_pending = 0;

		uvmexp.softs++;

		_cpu_intr_resume(s);
		(*sih->sih_fn)(sih->sih_arg);
		s = _cpu_intr_suspend();
	}

	_cpu_intr_resume(s);
}

void
setsoft(int ipl)
{

	if (ipl < IPL_SOFTNET)
		tmu1_oneshot();
	else
		tmu2_oneshot();
}

/* Register a software interrupt handler. */
void *
softintr_establish(int ipl, void (*func)(void *), void *arg)
{
	struct sh_soft_intr *asi;
	struct sh_soft_intrhand *sih;
	int s;

	if (__predict_false(ipl >= (IPL_SOFT + _IPL_NSOFT) ||
			    ipl < IPL_SOFT))
		panic("softintr_establish");

	sih = malloc(sizeof(*sih), M_DEVBUF, M_NOWAIT);

	s = _cpu_intr_suspend();
	asi = &sh_soft_intrs[ipl - IPL_SOFT];
	if (__predict_true(sih != NULL)) {
		sih->sih_intrhead = asi;
		sih->sih_fn = func;
		sih->sih_arg = arg;
		sih->sih_pending = 0;
	}
	_cpu_intr_resume(s);

	return (sih);
}

/* Unregister a software interrupt handler. */
void
softintr_disestablish(void *arg)
{
	struct sh_soft_intrhand *sih = arg;
	struct sh_soft_intr *asi = sih->sih_intrhead;
	int s;

	s = _cpu_intr_suspend();
	if (sih->sih_pending) {
		TAILQ_REMOVE(&asi->softintr_q, sih, sih_q);
		sih->sih_pending = 0;
	}
	_cpu_intr_resume(s);

	free(sih, M_DEVBUF);
}

/*
 * Software (low priority) network interrupt. i.e. softnet().
 */
void
netintr()
{
#define	DONETISR(bit, fn)						\
	do {								\
		if (n & (1 << bit))					\
			fn();						\
	} while (/*CONSTCOND*/0)

	int s, n;

	s = splnet();
	n = netisr;
	netisr = 0;
	splx(s);
#include <net/netisr_dispatch.h>

#undef DONETISR
}

/*
 * Software interrupt is simulated with TMU one-shot timer.
 */
void
tmu1_oneshot()
{

	_reg_bclr_1(SH_(TSTR), TSTR_STR1);
	_reg_write_4(SH_(TCNT1), 0);
	_reg_bset_1(SH_(TSTR), TSTR_STR1);
}

int
tmu1_intr(void *arg)
{

	_reg_bclr_1(SH_(TSTR), TSTR_STR1);
	_reg_bclr_2(SH_(TCR1), TCR_UNF);

	softintr_dispatch(IPL_SOFTCLOCK);
	softintr_dispatch(IPL_SOFT);

	return (0);
}

void
tmu2_oneshot()
{

	_reg_bclr_1(SH_(TSTR), TSTR_STR2);
	_reg_write_4(SH_(TCNT2), 0);
	_reg_bset_1(SH_(TSTR), TSTR_STR2);
}

int
tmu2_intr(void *arg)
{

	_reg_bclr_1(SH_(TSTR), TSTR_STR2);
	_reg_bclr_2(SH_(TCR2), TCR_UNF);

	softintr_dispatch(IPL_SOFTSERIAL);
	softintr_dispatch(IPL_SOFTNET);

	return (0);
}

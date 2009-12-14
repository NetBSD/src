/*	$NetBSD: interrupt.c,v 1.4 2009/12/14 00:46:00 matt Exp $	*/

/*-
 * Copyright (c) 2006 Izumi Tsutsui.  All rights reserved.
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
 * Copyright (c) 2000 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.4 2009/12/14 00:46:00 matt Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/cpu.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <mips/mips3_clock.h>
#include <machine/bus.h>

#include <dev/ic/i8259reg.h>
#include <dev/isa/isareg.h>

#include <cobalt/dev/gtreg.h>

#define ICU_LEVEL	4
#define IRQ_SLAVE	2

#define IO_ELCR		0x4d0
#define IO_ELCRSIZE	2
#define ELCR0		0
#define ELCR1		1

#define ICU1_READ(reg)		\
    bus_space_read_1(icu_bst, icu1_bsh, (reg))
#define ICU1_WRITE(reg, val)	\
    bus_space_write_1(icu_bst, icu1_bsh, (reg), (val))
#define ICU2_READ(reg)		\
    bus_space_read_1(icu_bst, icu2_bsh, (reg))
#define ICU2_WRITE(reg, val)	\
    bus_space_write_1(icu_bst, icu2_bsh, (reg), (val))
#define ELCR_READ(reg)		\
    bus_space_read_1(icu_bst, elcr_bsh, (reg))
#define ELCR_WRITE(reg, val)	\
    bus_space_write_1(icu_bst, elcr_bsh, (reg), (val))

static u_int icu_imask, icu_elcr;
static bus_space_tag_t icu_bst;
static bus_space_handle_t icu1_bsh, icu2_bsh, elcr_bsh;

struct icu_intrhead {
	LIST_HEAD(, cobalt_intrhand) intr_q;
	int intr_type;
	struct evcnt intr_evcnt;
	char intr_evname[32];
};
static struct icu_intrhead icu_intrtab[NICU_INT];

struct cpu_intrhead {
	struct cobalt_intrhand intr_ih;
	struct evcnt intr_evcnt;
	char intr_evname[32];
};
static struct cpu_intrhead cpu_intrtab[NCPU_INT];

static int	icu_intr(void *);
static void	icu_set(void);

void
intr_init(void)
{
	int i;

	/*
	 * Initialize CPU interrupts.
	 */
	for (i = 0; i < NCPU_INT; i++) {
		snprintf(cpu_intrtab[i].intr_evname,
		    sizeof(cpu_intrtab[i].intr_evname), "int %d", i);
		evcnt_attach_dynamic(&cpu_intrtab[i].intr_evcnt,
		    EVCNT_TYPE_INTR, NULL, "mips", cpu_intrtab[i].intr_evname);
	}

	/*
	 * Initialize ICU interrupts.
	 */
	icu_bst = 0;	/* XXX unused on cobalt */
	bus_space_map(icu_bst, PCIB_BASE + IO_ICU1, IO_ICUSIZE, 0, &icu1_bsh);
	bus_space_map(icu_bst, PCIB_BASE + IO_ICU2, IO_ICUSIZE, 0, &icu2_bsh);
	bus_space_map(icu_bst, PCIB_BASE + IO_ELCR, IO_ELCRSIZE, 0, &elcr_bsh);

	/* All interrupts default to "masked off". */
	icu_imask = 0xffff;

	/* All interrupts default to edge-triggered. */
	icu_elcr = 0;

	/* Initialize master PIC */

	/* reset; program device, four bytes */
	ICU1_WRITE(PIC_ICW1, ICW1_SELECT | ICW1_IC4);
	/* starting at this vector index */
	ICU1_WRITE(PIC_ICW2, 0);			/* XXX */
	/* slave on line 2 */
	ICU1_WRITE(PIC_ICW3, ICW3_CASCADE(IRQ_SLAVE));
	/* special fully nested mode, 8086 mode */
	ICU1_WRITE(PIC_ICW4, ICW4_SFNM | ICW4_8086);
	/* mask all interrupts */
	ICU1_WRITE(PIC_OCW1, icu_imask & 0xff);
	/* special mask mode */
	ICU1_WRITE(PIC_OCW3, OCW3_SELECT | OCW3_SSMM | OCW3_SMM);
	/* read IRR by default */
	ICU1_WRITE(PIC_OCW3, OCW3_SELECT | OCW3_RR);

	/* Initialize slave PIC */

	/* reset; program device, four bytes */
	ICU2_WRITE(PIC_ICW1, ICW1_SELECT | ICW1_IC4);
	/* starting at this vector index */
	ICU2_WRITE(PIC_ICW2, 8);			/* XXX */
	/* slave connected to line 2 of master */
	ICU2_WRITE(PIC_ICW3, ICW3_SIC(IRQ_SLAVE));
	/* special fully nested mode, 8086 mode */
	ICU2_WRITE(PIC_ICW4, ICW4_SFNM | ICW4_8086);
	/* mask all interrupts */
	ICU1_WRITE(PIC_OCW1, (icu_imask >> 8) & 0xff);
	/* special mask mode */
	ICU2_WRITE(PIC_OCW3, OCW3_SELECT | OCW3_SSMM | OCW3_SMM);
	/* read IRR by default */
	ICU2_WRITE(PIC_OCW3, OCW3_SELECT | OCW3_RR);

	/* default to edge-triggered */
	ELCR_WRITE(ELCR0, icu_elcr & 0xff);
	ELCR_WRITE(ELCR1, (icu_elcr >> 8) & 0xff);

	wbflush();

	/* Initialize our interrupt table. */
	for (i = 0; i < NICU_INT; i++) {
		LIST_INIT(&icu_intrtab[i].intr_q);
		snprintf(icu_intrtab[i].intr_evname,
		    sizeof(icu_intrtab[i].intr_evname), "irq %d", i);
		evcnt_attach_dynamic(&icu_intrtab[i].intr_evcnt,
		    EVCNT_TYPE_INTR, &cpu_intrtab[ICU_LEVEL].intr_evcnt,
		    "icu", icu_intrtab[i].intr_evname);
		icu_intrtab[i].intr_type = IST_NONE;
	}

	cpu_intr_establish(ICU_LEVEL, IPL_NONE, icu_intr, NULL);
}

void *
icu_intr_establish(int irq, int type, int ipl, int (*func)(void *), void *arg)
{
	struct cobalt_intrhand *ih;
	int s;

	if (irq >= NICU_INT || irq == IRQ_SLAVE || type == IST_NONE)
		panic("%s: bad irq or type", __func__);

	switch (icu_intrtab[irq].intr_type) {
	case IST_NONE:
		icu_intrtab[irq].intr_type = type;
		break;

	case IST_EDGE:
	case IST_LEVEL:
		if (type == icu_intrtab[irq].intr_type)
			break;
		/* FALLTHROUGH */
	case IST_PULSE:
		/*
		 * We can't share interrupts in this case.
		 */
		return NULL;
	}

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return NULL;

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_irq = irq;
	ih->ih_cookie_type = COBALT_COOKIE_TYPE_ICU;

	s = splhigh();

	/* Insert the handler into the table. */
	LIST_INSERT_HEAD(&icu_intrtab[irq].intr_q, ih, ih_q);

	/* Enable it, set trigger mode. */
	icu_imask &= ~(1U << irq);
	if (icu_intrtab[irq].intr_type == IST_LEVEL)
		icu_elcr |= (1U << irq);
	else
		icu_elcr &= ~(1U << irq);

	icu_set();

	splx(s);

	return ih;
}

void
icu_intr_disestablish(void *cookie)
{
	struct cobalt_intrhand *ih = cookie;
	int s;

	if (ih->ih_cookie_type == COBALT_COOKIE_TYPE_ICU) {
		s = splhigh();

		LIST_REMOVE(ih, ih_q);

		if (LIST_FIRST(&icu_intrtab[ih->ih_irq].intr_q) == NULL) {
			icu_imask |= (1U << ih->ih_irq);
			icu_set();
		}
		splx(s);
		free(ih, M_DEVBUF);
	}
}

void
icu_set(void)
{

	if ((icu_imask & 0xff00) != 0xff00)
		icu_imask &= ~(1U << IRQ_SLAVE);
	else
		icu_imask |= (1U << IRQ_SLAVE);

	ICU1_WRITE(PIC_OCW1, icu_imask);
	ICU2_WRITE(PIC_OCW1, icu_imask >> 8);

	ELCR_WRITE(ELCR0, icu_elcr);
	ELCR_WRITE(ELCR1, icu_elcr >> 8);
}

int
icu_intr(void *arg)
{
	struct cobalt_intrhand *ih;
	int irq, handled;

	handled = 0;

	for (;;) {
		/* check requested irq */
		ICU1_WRITE(PIC_OCW3, OCW3_SELECT | OCW3_POLL);
		irq = ICU1_READ(PIC_OCW3);
		if ((irq & OCW3_POLL_PENDING) == 0)
			return handled;

		irq = OCW3_POLL_IRQ(irq);
		if (irq == IRQ_SLAVE) {
			ICU2_WRITE(PIC_OCW3, OCW3_SELECT | OCW3_POLL);
			irq = OCW3_POLL_IRQ(ICU2_READ(PIC_OCW3)) + 8;
		}

		icu_intrtab[irq].intr_evcnt.ev_count++;
		LIST_FOREACH(ih, &icu_intrtab[irq].intr_q, ih_q) {
			if (__predict_false(ih->ih_func == NULL))
				printf("%s: spurious interrupt (irq = %d)\n",
				    __func__, irq);
			else if (__predict_true((*ih->ih_func)(ih->ih_arg))) {
				handled = 1;
			}
		}

		/* issue EOI to ack */
		if (irq >= 8) {
			ICU2_WRITE(PIC_OCW2,
			    OCW2_SELECT | OCW2_SL | OCW2_EOI |
			    OCW2_ILS(irq - 8));
			irq = IRQ_SLAVE;
		}
		ICU1_WRITE(PIC_OCW2,
		    OCW2_SELECT | OCW2_SL | OCW2_EOI | OCW2_ILS(irq));
	}
}

void *
cpu_intr_establish(int level, int ipl, int (*func)(void *), void *arg)
{
	struct cobalt_intrhand *ih;

	if (level < 0 || level >= NCPU_INT)
		panic("invalid interrupt level");

	ih = &cpu_intrtab[level].intr_ih;

	if (ih->ih_func != NULL)
		panic("cannot share CPU interrupts");

	ih->ih_cookie_type = COBALT_COOKIE_TYPE_CPU;
	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_irq = NICU_INT + level;

	return ih;
}

void
cpu_intr_disestablish(void *cookie)
{
	struct cobalt_intrhand *ih = cookie;

	if (ih->ih_cookie_type == COBALT_COOKIE_TYPE_CPU) {
		ih->ih_func = NULL;
		ih->ih_arg = NULL;
		ih->ih_cookie_type = 0;
	}
}

void
cpu_intr(uint32_t status, uint32_t cause, vaddr_t pc, uint32_t ipending)
{
	struct clockframe cf;
	struct cobalt_intrhand *ih;
	struct cpu_info *ci;
	uint32_t handled;

	handled = 0;
	ci = curcpu();
	ci->ci_idepth++;
	uvmexp.intrs++;

	if (ipending & MIPS_INT_MASK_5) {
		/* call the common MIPS3 clock interrupt handler */
		cf.pc = pc;
		cf.sr = status;
		mips3_clockintr(&cf);

		handled |= MIPS_INT_MASK_5;
	}
	_splset((status & handled) | MIPS_SR_INT_IE);

	if (__predict_false(ipending & MIPS_INT_MASK_0)) {
		/* GT64x11 timer0 */
		volatile uint32_t *irq_src =
		    (uint32_t *)MIPS_PHYS_TO_KSEG1(GT_BASE + GT_INTR_CAUSE);

		if (__predict_true((*irq_src & T0EXP) != 0)) {
			/* GT64x11 timer is no longer used for hardclock(9) */
			*irq_src = 0;
		}
		handled |= MIPS_INT_MASK_0;
	}

	if (ipending & MIPS_INT_MASK_3) {
		/* 16650 serial */
		ih = &cpu_intrtab[3].intr_ih;
		if (__predict_true(ih->ih_func != NULL)) {
			if (__predict_true((*ih->ih_func)(ih->ih_arg))) {
				cpu_intrtab[3].intr_evcnt.ev_count++;
			}
		}
		handled |= MIPS_INT_MASK_3;
	}
	_splset((status & handled) | MIPS_SR_INT_IE);

	if (ipending & MIPS_INT_MASK_1) {
		/* tulip primary */
		ih = &cpu_intrtab[1].intr_ih;
		if (__predict_true(ih->ih_func != NULL)) {
			if (__predict_true((*ih->ih_func)(ih->ih_arg))) {
				cpu_intrtab[1].intr_evcnt.ev_count++;
			}
		}
		handled |= MIPS_INT_MASK_1;
	}
	if (ipending & MIPS_INT_MASK_2) {
		/* tulip secondary */
		ih = &cpu_intrtab[2].intr_ih;
		if (__predict_true(ih->ih_func != NULL)) {
			if (__predict_true((*ih->ih_func)(ih->ih_arg))) {
				cpu_intrtab[2].intr_evcnt.ev_count++;
			}
		}
		handled |= MIPS_INT_MASK_2;
	}

	if (ipending & MIPS_INT_MASK_4) {
		/* ICU interrupts */
		ih = &cpu_intrtab[4].intr_ih;
		if (__predict_true(ih->ih_func != NULL)) {
			if (__predict_true((*ih->ih_func)(ih->ih_arg))) {
				cpu_intrtab[4].intr_evcnt.ev_count++;
			}
		}
		handled |= MIPS_INT_MASK_4;
	}
	cause &= ~handled;
	_splset((status & ~cause & MIPS_HARD_INT_MASK) | MIPS_SR_INT_IE);
	ci->ci_idepth--;

#ifdef __HAVE_FAST_SOFTINTS
	/* software interrupt */
	ipending &= (MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0);
	if (ipending == 0)
		return;
	_clrsoftintr(ipending);
	softintr_dispatch(ipending);
#endif
}


static const int ipl2spl_table[] = {
	[IPL_NONE] = 0,
	[IPL_SOFTCLOCK] = MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] = MIPS_SOFT_INT_MASK_0|MIPS_SOFT_INT_MASK_1,
	[IPL_VM] = SPLVM,
	[IPL_SCHED] = SPLSCHED,
};

ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._spl = ipl2spl_table[ipl]};
}

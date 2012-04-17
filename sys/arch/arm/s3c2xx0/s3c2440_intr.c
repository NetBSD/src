/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Fleischer <paul@xpg.dk>
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

/* Derived from s3c2410_intr.c */
/*
 * Copyright (c) 2003  Genetec corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec corporation may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORP. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORP.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * IRQ handler for Samsung S3C2440 processor.
 * It has integrated interrupt controller.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: s3c2440_intr.c,v 1.1.6.2 2012/04/17 00:06:06 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/atomic.h>
#include <sys/bus.h>
#include <machine/intr.h>
#include <arm/cpufunc.h>

#include <arm/s3c2xx0/s3c2440reg.h>
#include <arm/s3c2xx0/s3c2440var.h>

/*
 * interrupt dispatch table.
 */

struct s3c2xx0_intr_dispatch handler[ICU_LEN];


volatile int intr_mask;
#ifdef __HAVE_FAST_SOFTINTS
volatile int softint_pending;
volatile int soft_intr_mask;
#endif
volatile int global_intr_mask = 0; /* mask some interrupts at all spl level */

/* interrupt masks for each level */
int s3c2xx0_imask[NIPL];
int s3c2xx0_ilevel[ICU_LEN];
#ifdef __HAVE_FAST_SOFTINTS
int s3c24x0_soft_imask[NIPL];
#endif

vaddr_t intctl_base;		/* interrupt controller registers */
#define icreg(offset) \
	(*(volatile uint32_t *)(intctl_base+(offset)))

#ifdef __HAVE_FAST_SOFTINTS
/*
 * Map a software interrupt queue to an interrupt priority level.
 */
static const int si_to_ipl[] = {
	[SI_SOFTBIO]	= IPL_SOFTBIO,
	[SI_SOFTCLOCK]	= IPL_SOFTCLOCK,
	[SI_SOFTNET]	= IPL_SOFTNET,
	[SI_SOFTSERIAL] = IPL_SOFTSERIAL,
};
#endif

#define PENDING_CLEAR_MASK	(~0)

/*
 * called from irq_entry.
 */
void s3c2440_irq_handler(struct clockframe *);
void
s3c2440_irq_handler(struct clockframe *frame)
{
	uint32_t irqbits;
	int irqno;
	int saved_spl_level;
	struct cpu_info * const ci = curcpu();

	saved_spl_level = curcpl();

#ifdef	DIAGNOSTIC
	if (curcpu()->ci_intr_depth > 10)
		panic("nested intr too deep");
#endif

	while ((irqbits = icreg(INTCTL_INTPND)) != 0) {
		/* Note: Only one bit in INTPND register is set */

		irqno = icreg(INTCTL_INTOFFSET);

#ifdef	DIAGNOSTIC
		if (__predict_false((irqbits & (1<<irqno)) == 0)) {
			/* This shouldn't happen */
			printf("INTOFFSET=%d, INTPND=%x\n", irqno, irqbits);
			break;
		}
#endif
		/* raise spl to stop interrupts of lower priorities */
		if (saved_spl_level < handler[irqno].level)
			s3c2xx0_setipl(handler[irqno].level);

		/* clear pending bit */
		icreg(INTCTL_SRCPND) = PENDING_CLEAR_MASK & (1 << irqno);
		icreg(INTCTL_INTPND) = PENDING_CLEAR_MASK & (1 << irqno);

		handler[irqno].ev.ev_count++;
		ci->ci_data.cpu_nintr++;

		enable_interrupts(I32_bit); /* allow nested interrupts */

		(*handler[irqno].func) (
		    handler[irqno].cookie == 0
		    ? frame : handler[irqno].cookie);

		disable_interrupts(I32_bit);

		/* restore spl to that was when this interrupt happen */
		s3c2xx0_setipl(saved_spl_level);

	}

#ifdef __HAVE_FAST_SOFTINTS
	cpu_dosoftints();
#endif
}

/*
 * Handler for main IRQ of cascaded interrupts.
 */
static int
cascade_irq_handler(void *cookie)
{
	int index = (int)cookie - 1;
	uint32_t irqbits;
	int irqno, i;
	int save = disable_interrupts(I32_bit);

	KASSERT(0 <= index && index <= 3);

	irqbits = icreg(INTCTL_SUBSRCPND) &
	    ~icreg(INTCTL_INTSUBMSK) & (0x07 << (3*index));

	for (irqno = 3*index; irqbits; ++irqno) {
		if ((irqbits & (1<<irqno)) == 0)
			continue;

		/* clear pending bit */
		irqbits &= ~(1<<irqno);
		icreg(INTCTL_SUBSRCPND) = (1 << irqno);

		/* allow nested interrupts. SPL is already set
		 * correctly by main handler. */
		restore_interrupts(save);

		i = S3C2440_SUBIRQ_MIN + irqno;
		(* handler[i].func)(handler[i].cookie);

		disable_interrupts(I32_bit);
	}

	return 1;
}


static const uint8_t subirq_to_main[] = {
	S3C2440_INT_UART0,
	S3C2440_INT_UART0,
	S3C2440_INT_UART0,
	S3C2440_INT_UART1,
	S3C2440_INT_UART1,
	S3C2440_INT_UART1,
	S3C2440_INT_UART2,
	S3C2440_INT_UART2,
	S3C2440_INT_UART2,
	S3C24X0_INT_ADCTC,
	S3C24X0_INT_ADCTC,
};

void *
s3c24x0_intr_establish(int irqno, int level, int type,
    int (* func) (void *), void *cookie)
{
	int save;

	if (irqno < 0 || irqno >= ICU_LEN ||
	    type < IST_NONE || IST_EDGE_BOTH < type)
		panic("intr_establish: bogus irq or type");

	save = disable_interrupts(I32_bit);

	handler[irqno].cookie = cookie;
	handler[irqno].func = func;
	handler[irqno].level = level;

	if (irqno >= S3C2440_SUBIRQ_MIN) {
		/* cascaded interrupts. */
		int main_irqno;
		int i = (irqno - S3C2440_SUBIRQ_MIN);

		main_irqno = subirq_to_main[i];

		/* establish main irq if first time
		 * be careful that cookie shouldn't be 0 */
		if (handler[main_irqno].func != cascade_irq_handler)
			s3c24x0_intr_establish(main_irqno, level, type,
			    cascade_irq_handler, (void *)((i/3) + 1));

		/* unmask it in submask register */
		icreg(INTCTL_INTSUBMSK) &= ~(1<<i);

		restore_interrupts(save);
		return &handler[irqno];
	}

	s3c2xx0_update_intr_masks(irqno, level);

	/*
	 * set trigger type for external interrupts 0..3
	 */
	if (irqno <= S3C24X0_INT_EXT(3)) {
		/*
		 * Update external interrupt control
		 */
		s3c2440_setup_extint(irqno, type);
	}

	s3c2xx0_setipl(curcpl());

	restore_interrupts(save);

	return &handler[irqno];
}


static void
init_interrupt_masks(void)
{
	int i;

	for (i=0; i < NIPL; ++i)
		s3c2xx0_imask[i] = 0;

#ifdef __HAVE_FAST_SOFTINTS
	s3c24x0_soft_imask[IPL_NONE] = SI_TO_IRQBIT(SI_SOFTSERIAL) |
		SI_TO_IRQBIT(SI_SOFTNET) | SI_TO_IRQBIT(SI_SOFTCLOCK) |
		SI_TO_IRQBIT(SI_SOFT);

	s3c24x0_soft_imask[IPL_SOFT] = SI_TO_IRQBIT(SI_SOFTSERIAL) |
		SI_TO_IRQBIT(SI_SOFTNET) | SI_TO_IRQBIT(SI_SOFTCLOCK);

	/*
	 * splsoftclock() is the only interface that users of the
	 * generic software interrupt facility have to block their
	 * soft intrs, so splsoftclock() must also block IPL_SOFT.
	 */
	s3c24x0_soft_imask[IPL_SOFTCLOCK] = SI_TO_IRQBIT(SI_SOFTSERIAL) |
		SI_TO_IRQBIT(SI_SOFTNET);

	/*
	 * splsoftnet() must also block splsoftclock(), since we don't
	 * want timer-driven network events to occur while we're
	 * processing incoming packets.
	 */
	s3c24x0_soft_imask[IPL_SOFTNET] = SI_TO_IRQBIT(SI_SOFTSERIAL);

	for (i = IPL_BIO; i < IPL_SOFTSERIAL; ++i)
		s3c24x0_soft_imask[i] = SI_TO_IRQBIT(SI_SOFTSERIAL);
#endif
}

void
s3c2440_intr_init(struct s3c24x0_softc *sc)
{
	intctl_base = (vaddr_t) bus_space_vaddr(sc->sc_sx.sc_iot,
	    sc->sc_sx.sc_intctl_ioh);

	s3c2xx0_intr_mask_reg = (uint32_t *)(intctl_base + INTCTL_INTMSK);

	/* clear all pending interrupt */
	icreg(INTCTL_SRCPND) = ~0;
	icreg(INTCTL_INTPND) = ~0;

	/* mask all sub interrupts */
	icreg(INTCTL_INTSUBMSK) = 0x7ff;

	init_interrupt_masks();

	s3c2xx0_intr_init(handler, ICU_LEN);

}


/*
 * mask/unmask sub interrupts
 */
void
s3c2440_mask_subinterrupts(int bits)
{
	int psw = disable_interrupts(IF32_bits);
	icreg(INTCTL_INTSUBMSK) |= bits;
	restore_interrupts(psw);

}

void
s3c2440_unmask_subinterrupts(int bits)
{
	int psw = disable_interrupts(IF32_bits);
	icreg(INTCTL_INTSUBMSK) &= ~bits;
	restore_interrupts(psw);

}

/*
 * Update external interrupt control
 */
static const u_char s3c24x0_ist[] = {
	EXTINTR_LOW,		/* NONE */
	EXTINTR_FALLING,	/* PULSE */
	EXTINTR_FALLING,	/* EDGE */
	EXTINTR_LOW,		/* LEVEL */
	EXTINTR_HIGH,
	EXTINTR_RISING,
	EXTINTR_BOTH,
};

void
s3c2440_setup_extint(int extint, int type)
{
        uint32_t reg;
        u_int   trig;
        int     i = extint % 8;
        int     regidx = extint/8;      /* GPIO_EXTINT[0:2] */
	int	save;
	uint32_t gpio;
	uint32_t offset;

        trig = s3c24x0_ist[type];

	save = disable_interrupts(I32_bit);

        reg = bus_space_read_4(s3c2xx0_softc->sc_iot,
            s3c2xx0_softc->sc_gpio_ioh,
            GPIO_EXTINT(regidx));

        reg = reg & ~(0x07 << (4*i));
        reg |= trig << (4*i);

        bus_space_write_4(s3c2xx0_softc->sc_iot, s3c2xx0_softc->sc_gpio_ioh,
            GPIO_EXTINT(regidx), reg);

	/* Setup GPIO-pin to serve as interrupt */
	if (extint < 8 ) {
	  gpio = GPIO_PFCON;
	  offset = extint;
	} else {
	  gpio = GPIO_PGCON;
	  offset = 8-extint;
	}
	reg = bus_space_read_4(s3c2xx0_softc->sc_iot, s3c2xx0_softc->sc_gpio_ioh,
				 gpio);
	reg = GPIO_SET_FUNC(reg, offset, 2);
	bus_space_write_4(s3c2xx0_softc->sc_iot, s3c2xx0_softc->sc_gpio_ioh,
			  gpio, reg);


	restore_interrupts(save);
}

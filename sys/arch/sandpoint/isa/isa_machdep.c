/*	$NetBSD: isa_machdep.c,v 1.4.6.2 2001/08/30 02:08:45 briggs Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *
 *	@(#)isa.c	7.2 (Berkeley) 5/13/91
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <machine/pio.h>
#include <machine/intr.h>
#include <machine/openpicreg.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <sandpoint/isa/icu.h>

#define IRQ_SLAVE 2

static void		*isa_irqh;
static int		isa_pending;
static int		isa_reg_level=0xffff;
static struct intrhand	*isa_intrhand[16];
static int		isa_intrtype[16];
static int		isa_intrmask[16];

static void isa_intr_mask __P((int));
static void isa_intr_clear __P((int));
static int do_isa_intr __P((void *));
static int isa_intr __P((void));

const struct evcnt *
isa_intr_evcnt(isa_chipset_tag_t ic, int irq)
{
	/* XXX for now, no evcnt parent reported */
	return NULL;
}

static int
do_isa_intr(void *iarg)
{
	int	irq, intbit;
	struct intrhand *ih;
	extern int sandpoint_icpl;

	irq = isa_intr();
	if (irq == -1)
		goto out;

	do {
		if (isa_intrmask[irq] & sandpoint_icpl) {
			intbit = 1 << irq;
			isa_pending |= intbit;
			isa_intr_clear(irq);
			isa_intr_mask(isa_pending);
			setsoftisa();
		} else {
			isa_intr_clear(irq);
			ih = isa_intrhand[irq];
			while (ih) {
				(*ih->ih_fun)(ih->ih_arg);
				ih = ih->ih_next;
			}
		}
		irq = isa_intr();
	} while (irq != -1);

out:
	return 0;
}

/*
 * Set up an interrupt handler to start being called.
 */
void *
isa_intr_establish(ic, irq, type, level, ih_fun, ih_arg)
	isa_chipset_tag_t ic;
	int irq;
	int type;
	int level;
	int (*ih_fun) __P((void *));
	void *ih_arg;
{
	extern int fakeintr(void *arg);
	extern char *intr_typename(int);
	struct intrhand **p, *q, *ih;
	static struct intrhand fakehand = {fakeintr};

	if (irq < 0 || irq > 15)
		panic("isa_intr_establish: bogus irq or type");

	ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("isa_intr_establish: can't malloc handler info");

	switch (isa_intrtype[irq]) {
	case IST_NONE:
		isa_intrtype[irq] = type;
		break;
	case IST_EDGE:
	case IST_LEVEL:
		if (type == isa_intrtype[irq])
			break;
	case IST_PULSE:
		if (type != IST_NONE) {
			panic("isa_intr_establish: can't share %s with %s",
				intr_typename(isa_intrtype[irq]),
				intr_typename(type));
		}
	}

	for (p = &isa_intrhand[irq]; (q = *p) != NULL; p = &q->ih_next)
		;

	fakehand.ih_level = level;
	*p = &fakehand;

	isa_intr_calculate_masks(0);

	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
	ih->ih_count = 0;
	ih->ih_next = NULL;
	ih->ih_level = level;
	ih->ih_irq = irq;
	*p = ih;

	if (level < isa_reg_level) {
		if (isa_irqh) {
			intr_disestablish(isa_irqh);
		}
		isa_irqh = intr_establish(SANDPOINT_INTR_ISA, IST_LEVEL, level,
		    do_isa_intr, NULL);
		isa_reg_level = level;
	}
	return ih;
}

void
isa_do_pending_int(int pcpl)
{
	struct intrhand *ih;
	int	irq;

	/* Interrupts disabled & reentrancy prevented by caller */
	for (irq=0 ; irq<16 ; irq++) {
		if (   (isa_pending & (1 << irq))
		    && ((isa_intrmask[irq] & pcpl) == 0)) {
			isa_pending &= ~(1 << irq);
			ih = isa_intrhand[irq];
			while (ih) {
				(*ih->ih_fun)(ih->ih_arg);
				ih = ih->ih_next;
			}
		}
	}
	isa_intr_mask(isa_pending);
}

void
isa_intr_calculate_masks(int reset_level)
{
	int	irq, minlevel, irqs;
	struct intrhand **p, *q;

	/* 
	 * calculate isa_intrmasks so that bits are set according to
	 * spl levels so that the mask in do_isa_intr is valid.
	 */
	/* First, clear masks */
	for (irq=0 ; irq<16 ; irq++) {
		isa_intrmask[irq] = 0;
	}
	irqs = 0;
	minlevel = 0xffff;
	for (irq=0 ; irq<16 ; irq++) {
		if (isa_intrhand[irq]) {
			irqs |= 1 << irq;
		}
		for (p = &isa_intrhand[irq]; (q = *p) != NULL;
		    p = &q->ih_next) {
			isa_intrmask[irq] |= (1 << q->ih_level);
			if (q->ih_level < minlevel) {
				minlevel = q->ih_level;
			}
		}
	}
	isa_intr_mask(~irqs);
	if (reset_level && minlevel != 0xffff) {
		if (minlevel != isa_reg_level) {
			if (isa_irqh) {
				intr_disestablish(isa_irqh);
			}
			isa_irqh = intr_establish(SANDPOINT_INTR_ISA,
			    IST_LEVEL, minlevel, do_isa_intr, NULL);
			isa_reg_level = minlevel;
		}
	}
}

/*
 * Deregister an interrupt handler.
 */
void
isa_intr_disestablish(ic, arg)
	isa_chipset_tag_t ic;
	void *arg;
{
	struct intrhand *ih = arg;
	int irq = ih->ih_irq, cpl;
	struct intrhand **p, *q;

	if (irq < 0 || irq > 15)
		panic("intr_disestablish: bogus irq");

	cpl = splhigh();
	for (p = &isa_intrhand[irq]; (q = *p) != NULL && q != ih;
	    p = &q->ih_next)
		;
	if (q)
		*p = q->ih_next;
	else
		panic("isa_intr_disestablish: handler not registered");
	free((void *)ih, M_DEVBUF);

	isa_intr_calculate_masks(1);

	if (isa_intrhand[irq] == NULL) {
		isa_intrtype[irq] = IST_NONE;
	}
	splx(cpl);
}

#define SUPERIO_CONF_IDX		0x15C
#define SUPERIO_CONF_DATA		0x15D
#define SUPERIO_CONF_LDN		0x07
#define SUPERIO_CONF_ACTIVATE		0x30
#define SUPERIO_CONF_CFG1_ATDRIVE	0x04
void
isa_attach_hook(parent, self, iba)
	struct device *parent, *self;
	struct isabus_attach_args *iba;
{
	u_int8_t cfg;
	int ldn;

	/* Set PS/2 drive mode */
	isa_outb(SUPERIO_CONF_IDX, 0x21);
	cfg = isa_inb(SUPERIO_CONF_DATA);
	cfg &= ~SUPERIO_CONF_CFG1_ATDRIVE;
	isa_outb(SUPERIO_CONF_DATA, cfg);

	/* Enable the 9 Super I/O devices. */
	for (ldn=0; ldn <= 8; ldn++) {
		isa_outb(SUPERIO_CONF_IDX,  SUPERIO_CONF_LDN);
		isa_outb(SUPERIO_CONF_DATA, ldn);
		isa_outb(SUPERIO_CONF_IDX,  SUPERIO_CONF_ACTIVATE);
		isa_outb(SUPERIO_CONF_DATA, 0x01);
	}
}

static int
isa_intr(void)
{
	int irq;

	isa_outb(IO_ICU1, 0x0c);
	irq = isa_inb(IO_ICU1);
	if (!(irq & 0x80)) {
		return -1;
	}

	irq &= 0x07;
	if (irq == IRQ_SLAVE) {
		isa_outb(IO_ICU2, 0x0c);
		irq = (isa_inb(IO_ICU2+1) & 0x07) + 8;
	}

	return (irq);
}

void
isa_intr_mask(int mask)
{
	isa_outb(IO_ICU1 + 1, mask);
	isa_outb(IO_ICU2 + 1, mask >> 8);
}

static void
isa_intr_clear(irq)
	int irq;
{
	if (irq < 8) {
		isa_outb(IO_ICU1, 0x60 | irq);
	} else {
		isa_outb(IO_ICU2, 0x60 | (irq%8));
		isa_outb(IO_ICU1, 0x60 | IRQ_SLAVE);
	}
}

void
isa_intr_init(void)
{
	isa_outb(IO_ICU1, 0x19);
	isa_outb(IO_ICU1+1, 0);
	isa_outb(IO_ICU1+1, 1 << IRQ_SLAVE);
	isa_outb(IO_ICU1+1, 3);
	isa_outb(IO_ICU1+1, 0xff);
	isa_outb(IO_ICU1, 0x68);
	isa_outb(IO_ICU1, 0x0a);

	isa_outb(IO_ICU2, 0x19);
	isa_outb(IO_ICU2+1, 8);
	isa_outb(IO_ICU2+1, IRQ_SLAVE);
	isa_outb(IO_ICU2+1, 3);
	isa_outb(IO_ICU2+1, 0xff);
	isa_outb(IO_ICU2, 0x68);
	isa_outb(IO_ICU2, 0x0a);
}

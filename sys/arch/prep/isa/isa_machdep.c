/*	$NetBSD: isa_machdep.c,v 1.8 2003/08/07 16:29:20 agc Exp $	*/

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
 * 3. Neither the name of the University nor the names of its contributors
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isa_machdep.c,v 1.8 2003/08/07 16:29:20 agc Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <machine/pio.h>
#include <machine/intr.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#define	IO_ELCR1	0x4d0
#define	IO_ELCR2	0x4d1

const struct evcnt *
isa_intr_evcnt(isa_chipset_tag_t ic, int irq)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

/*
 * Set up an interrupt handler to start being called.
 * XXX PRONE TO RACE CONDITIONS, UGLY, 'INTERESTING' INSERTION ALGORITHM.
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

	return (void *)intr_establish(irq, type, level, ih_fun, ih_arg);
}

/*
 * Deregister an interrupt handler.
 */
void
isa_intr_disestablish(ic, arg)
	isa_chipset_tag_t ic;
	void *arg;
{

	intr_disestablish(arg);
}

void
isa_attach_hook(parent, self, iba)
	struct device *parent, *self;
	struct isabus_attach_args *iba;
{

	/* Nothing to do. */
}

int
isa_intr(void)
{
	int irq;

	isa_outb(IO_ICU1, 0x0c);
	irq = isa_inb(IO_ICU1) & 0x07;
	if (irq == IRQ_SLAVE) {
		isa_outb(IO_ICU2, 0x0c);
		irq = (isa_inb(IO_ICU2) & 0x07) + 8;
	}

	return (irq);
}

void
isa_intr_mask(mask)
	int mask;
{

	isa_outb(IO_ICU1+1, mask);
	isa_outb(IO_ICU2+1, mask >> 8);
}

void
isa_intr_clr(irq)
	int irq;
{

	if (irq < 8) {
		isa_outb(IO_ICU1, 0xe0 | irq);
	} else {
		isa_outb(IO_ICU2, 0xe0 | (irq & 7));
		isa_outb(IO_ICU1, 0xe0 | IRQ_SLAVE);
	}
}

/* 
 * Initialize the Interrupt controller logic.
 */
void
init_icu(int lvlmask)
{
	int i;
	extern int intrtype[];

	for (i= 0; i < ICU_LEN; i++) {
		switch (i) {
		case 0:
		case 2:
		case 8:
			intrtype[i] = IST_EDGE;
			break;
		default:
			intrtype[i] = (1 << i) & lvlmask ? IST_LEVEL : IST_NONE;
		}
	}

#ifdef DEBUG
	printf("old elcr = 0x%02x%02x, new = 0x%04x\n",
		isa_inb(IO_ELCR2), isa_inb(IO_ELCR1), lvlmask);
#endif
	isa_outb(IO_ELCR1, (lvlmask >> 0) & 0xff);
	isa_outb(IO_ELCR2, (lvlmask >> 8) & 0xff);

	isa_outb(IO_ICU1, 0x11);		/* program device, four bytes */
	isa_outb(IO_ICU1+1, 0);			/* starting at this vector */
	isa_outb(IO_ICU1+1, 1 << IRQ_SLAVE);	/* slave on line 2 */
	isa_outb(IO_ICU1+1, 1);			/* 8086 mode */
	isa_outb(IO_ICU1+1, 0xff);		/* leave interrupts masked */

	isa_outb(IO_ICU2, 0x11);		/* program device, four bytes */
	isa_outb(IO_ICU2+1, 8);			/* starting at this vector */
	isa_outb(IO_ICU2+1, IRQ_SLAVE);
	isa_outb(IO_ICU2+1, 1);			/* 8086 mode */
	isa_outb(IO_ICU2+1, 0xff);		/* leave interrupts masked */
}

void
isa_setirqstat(irq, enabled, type)
	int irq, enabled, type;
{
	u_int8_t elcr[2];
	int icu, bit;

	icu = irq / 8;
	bit = irq % 8;

	elcr[0] = isa_inb(IO_ELCR1);
	elcr[1] = isa_inb(IO_ELCR2);

	if (type == IST_LEVEL)
		elcr[icu] |= 1 << bit;
	else
		elcr[icu] &= ~(1 << bit);

	isa_outb(IO_ELCR1, elcr[0]);
	isa_outb(IO_ELCR2, elcr[1]);
}

int
isa_intr_alloc(isa_chipset_tag_t c, int mask, int type, int *irq_p)
{
	int irq;
	int maybe_irq = -1;
	int shared_depth = 0;
	mask &= 0x8b28;	/* choose from 3, 5, 8, 9, 11, 15 XXX */
	for (irq = 0; mask != 0; mask >>= 1, irq++) {
		if ((mask & 1) == 0)
			continue;
		if (intrtype[irq] == IST_NONE) {
			*irq_p = irq;
			return 0;
		}
		/* Level interrupts can be shared */
		if (type == IST_LEVEL && intrtype[irq] == IST_LEVEL) {
			struct intrhand *ih = intrhand[irq];
			int depth;
			if (maybe_irq == -1) {
				maybe_irq = irq;
				continue;
			}
			for (depth = 0; ih != NULL; ih = ih->ih_next)
				depth++;
			if (depth < shared_depth) {
				maybe_irq = irq;
				shared_depth = depth;
			}
		}
	}
	if (maybe_irq != -1) {
		*irq_p = maybe_irq;
		return 0;
	}
	return 1;
}

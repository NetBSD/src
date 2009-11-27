/*	$NetBSD: isabus.c,v 1.46 2009/11/27 03:23:04 rmind Exp $	*/
/*	$OpenBSD: isabus.c,v 1.15 1998/03/16 09:38:46 pefo Exp $	*/
/*	NetBSD: isa.c,v 1.33 1995/06/28 04:30:51 cgd Exp 	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
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
 *	@(#)isa.c	7.2 (Berkeley) 5/12/91
 */
/*-
 * Copyright (c) 1995 Per Fogelstrom
 * Copyright (c) 1993, 1994 Charles M. Hannum.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
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
 *	@(#)isa.c	7.2 (Berkeley) 5/12/91
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
  Copyright 1988, 1989 by Intel Corporation, Santa Clara, California.

		All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isabus.c,v 1.46 2009/11/27 03:23:04 rmind Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/extent.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/pio.h>
#include <machine/autoconf.h>
#include <machine/intr.h>

#include <mips/locore.h>

#include <dev/ic/i8253reg.h>
#include <dev/ic/i8259reg.h>
#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <arc/isa/isabrvar.h>
#include <arc/isa/spkrreg.h>

#include <arc/arc/timervar.h>

static int beeping;
static callout_t sysbeep_ch;

static long isa_mem_ex_storage[EXTENT_FIXED_STORAGE_SIZE(16) / sizeof(long)];
static long isa_io_ex_storage[EXTENT_FIXED_STORAGE_SIZE(16) / sizeof(long)];

#define	IRQ_SLAVE	2

/* Definition of the driver for autoconfig. */
static int isabrprint(void *, const char *);

extern struct arc_bus_space arc_bus_io, arc_bus_mem;

static void isabr_attach_hook(device_t , device_t,
    struct isabus_attach_args *);
static void isabr_detach_hook(isa_chipset_tag_t, device_t);
static const struct evcnt *isabr_intr_evcnt(isa_chipset_tag_t, int);
static void *isabr_intr_establish(isa_chipset_tag_t, int, int, int,
    int (*)(void *), void *);
static void isabr_intr_disestablish(isa_chipset_tag_t, void*);
static void isabr_initicu(void);
static void intr_calculatemasks(void);
static int fakeintr(void *a);

struct isabr_config *isabr_conf = NULL;
uint32_t imask[_IPL_N];	/* XXX */

void
isabrattach(struct isabr_softc *sc)
{
	struct isabus_attach_args iba;

	callout_init(&sysbeep_ch, 0);

	if (isabr_conf == NULL)
		panic("isabr_conf isn't initialized");

	aprint_normal("\n");

	/* Initialize interrupt controller */
	isabr_initicu();

	sc->arc_isa_cs.ic_attach_hook = isabr_attach_hook;
	sc->arc_isa_cs.ic_detach_hook = isabr_detach_hook;
	sc->arc_isa_cs.ic_intr_evcnt = isabr_intr_evcnt;
	sc->arc_isa_cs.ic_intr_establish = isabr_intr_establish;
	sc->arc_isa_cs.ic_intr_disestablish = isabr_intr_disestablish;

	arc_bus_space_init_extent(&arc_bus_mem, (void *)isa_mem_ex_storage,
	    sizeof(isa_mem_ex_storage));
	arc_bus_space_init_extent(&arc_bus_io, (void *)isa_io_ex_storage,
	    sizeof(isa_io_ex_storage));

	iba.iba_iot = &arc_bus_io;
	iba.iba_memt = &arc_bus_mem;
	iba.iba_dmat = &sc->sc_dmat;
	iba.iba_ic = &sc->arc_isa_cs;
	config_found_ia(sc->sc_dev, "isabus", &iba, isabrprint);
}

static int
isabrprint(void *aux, const char *pnp)
{

        if (pnp)
                aprint_normal("isa at %s", pnp);
        aprint_verbose(" isa_io_base 0x%lx isa_mem_base 0x%lx",
	    arc_bus_io.bs_vbase, arc_bus_mem.bs_vbase);
        return UNCONF;
}


/*
 *	Interrupt system driver code
 *	============================
 */
#define LEGAL_IRQ(x)    ((x) >= 0 && (x) < ICU_LEN && (x) != 2)

int	imen;
int	intrtype[ICU_LEN], intrmask[ICU_LEN], intrlevel[ICU_LEN];
struct isa_intrhand *isa_intrhand[ICU_LEN];

static int
fakeintr(void *a)
{

	return 0;
}

/*
 * Recalculate the interrupt masks from scratch.
 * We could code special registry and deregistry versions of this function that
 * would be faster, but the code would be nastier, and we don't expect this to
 * happen very much anyway.
 */
static void
intr_calculatemasks(void)
{
	int irq, level;
	struct isa_intrhand *q;

	/* First, figure out which levels each IRQ uses. */
	for (irq = 0; irq < ICU_LEN; irq++) {
		int levels = 0;
		for (q = isa_intrhand[irq]; q; q = q->ih_next)
			levels |= 1 << q->ih_level;
		intrlevel[irq] = levels;
	}

	/* Then figure out which IRQs use each level. */
	for (level = 0; level < _IPL_N; level++) {
		int irqs = 0;
		for (irq = 0; irq < ICU_LEN; irq++)
			if (intrlevel[irq] & (1 << level))
				irqs |= 1 << irq;
		imask[level] = irqs;
	}

	imask[IPL_NONE] = 0;

	imask[IPL_SOFTCLOCK] |= imask[IPL_NONE];
	imask[IPL_SOFTNET] |= imask[IPL_SOFTCLOCK];

	/*
	 * Enforce a hierarchy that gives slow devices a better chance at not
	 * dropping data.
	 */
	imask[IPL_VM] |= imask[IPL_SOFTNET];

	/*
	 * Since run queues may be manipulated by both the statclock and tty,
	 * network, and diskdrivers, clock > tty.
	 */
	imask[IPL_SCHED] |= imask[IPL_VM];

	/* And eventually calculate the complete masks. */
	for (irq = 0; irq < ICU_LEN; irq++) {
		int irqs = 1 << irq;
		for (q = isa_intrhand[irq]; q; q = q->ih_next)
			irqs |= imask[q->ih_level];
		intrmask[irq] = irqs;
	}

	/* Lastly, determine which IRQs are actually in use. */
	{
		int irqs = 0;
		for (irq = 0; irq < ICU_LEN; irq++)
			if (isa_intrhand[irq])
				irqs |= 1 << irq;
		if (irqs >= 0x100) /* any IRQs >= 8 in use */
			irqs |= 1 << IRQ_SLAVE;
		imen = ~irqs;
		isa_outb(IO_ICU1 + PIC_OCW1, imen);
		isa_outb(IO_ICU2 + PIC_OCW1, imen >> 8);
	}
}

static void
isabr_attach_hook(struct device *parent, struct device *self,
    struct isabus_attach_args *iba)
{

	/* Nothing to do. */
}

static void
isabr_detach_hook(isa_chipset_tag_t ic, device_t self)
{

	/* Nothing to do. */
}

static const struct evcnt *
isabr_intr_evcnt(isa_chipset_tag_t ic, int irq)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

/*
 *	Establish a ISA bus interrupt.
 */
static void *
isabr_intr_establish(isa_chipset_tag_t ic, int irq, int type, int level,
    int (*ih_fun)(void *), void *ih_arg)
{
	struct isa_intrhand **p, *q, *ih;
	static struct isa_intrhand fakehand = {NULL, fakeintr};

	/* no point in sleeping unless someone can free memory. */
	ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("isa_intr_establish: can't malloc handler info");

	if (!LEGAL_IRQ(irq) || type == IST_NONE)
		panic("intr_establish: bogus irq or type");

	switch (intrtype[irq]) {
	case IST_NONE:
		intrtype[irq] = type;
		break;
	case IST_EDGE:
	case IST_LEVEL:
		if (type == intrtype[irq])
			break;
	case IST_PULSE:
		if (type != IST_NONE)
			panic("intr_establish: can't share %s with %s",
			    isa_intr_typename(intrtype[irq]),
			    isa_intr_typename(type));
		break;
	}

	/*
	 * Figure out where to put the handler.
	 * This is O(N^2), but we want to preserve the order, and N is
	 * generally small.
	 */
	for (p = &isa_intrhand[irq]; (q = *p) != NULL; p = &q->ih_next)
		;

	/*
	 * Actually install a fake handler momentarily, since we might be doing
	 * this with interrupts enabled and don't want the real routine called
	 * until masking is set up.
	 */
	fakehand.ih_level = level;
	*p = &fakehand;

	intr_calculatemasks();

	/*
	 * Poke the real handler in now.
	 */
	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
	ih->ih_count = 0;
	ih->ih_next = NULL;
	ih->ih_level = level;
	ih->ih_irq = irq;
	snprintf(ih->ih_evname, sizeof(ih->ih_evname), "irq %d", irq);
	evcnt_attach_dynamic(&ih->ih_evcnt, EVCNT_TYPE_INTR, NULL, "isa",
	    ih->ih_evname);
	*p = ih;

	return ih;
}

static void
isabr_intr_disestablish(isa_chipset_tag_t ic, void *arg)
{

}

/*
 *	Process an interrupt from the ISA bus.
 */
uint32_t
isabr_iointr(uint32_t mask, struct clockframe *cf)
{
	struct isa_intrhand *ih;
	int isa_vector;
	int o_imen;

	isa_vector = (*isabr_conf->ic_intr_status)();
	if (isa_vector < 0)
		return 0;

	o_imen = imen;
	imen |= 1 << (isa_vector & (ICU_LEN - 1));
	if (isa_vector & 0x08) {
		isa_inb(IO_ICU2 + PIC_OCW1);
		isa_outb(IO_ICU2 + PIC_OCW1, imen >> 8);
		isa_outb(IO_ICU2 + PIC_OCW2,
		    OCW2_SELECT | OCW2_EOI | OCW2_SL |
		    OCW2_ILS((isa_vector & 7)));
		isa_outb(IO_ICU1,
		    OCW2_SELECT | OCW2_EOI | OCW2_SL | IRQ_SLAVE);
	} else {
		isa_inb(IO_ICU1 + PIC_OCW1);
		isa_outb(IO_ICU1 + PIC_OCW1, imen);
		isa_outb(IO_ICU1 + PIC_OCW2,
		    OCW2_SELECT | OCW2_EOI | OCW2_SL | OCW2_ILS(isa_vector));
	}
	ih = isa_intrhand[isa_vector];
	if (isa_vector == 0 && ih) {	/* Clock */	/*XXX*/
		last_cp0_count = mips3_cp0_count_read();
		/* XXX: spllowerclock() not allowed */
		cf->sr &= ~MIPS_SR_INT_IE;
		if ((*ih->ih_fun)(cf))
			ih->ih_evcnt.ev_count++;
		ih = ih->ih_next;
	}
	while (ih) {
		if ((*ih->ih_fun)(ih->ih_arg))
			ih->ih_evcnt.ev_count++;
		ih = ih->ih_next;
	}
	imen = o_imen;
	isa_inb(IO_ICU1 + PIC_OCW1);
	isa_inb(IO_ICU2 + PIC_OCW1);
	isa_outb(IO_ICU1 + PIC_OCW1, imen);
	isa_outb(IO_ICU2 + PIC_OCW1, imen >> 8);

	return MIPS_INT_MASK_2;
}


/*
 * Initialize the Interrupt controller logic.
 */
static void
isabr_initicu(void)
{

	int i;

	for (i = 0; i < ICU_LEN; i++) {
		switch (i) {
		case 2:
		case 8:
			intrtype[i] = IST_EDGE;
			break;
		default:
			intrtype[i] = IST_NONE;
			break;
		}
	}

	/* reset; program device, four bytes */
	isa_outb(IO_ICU1 + PIC_ICW1, ICW1_SELECT | ICW1_IC4);
	/* starting at this vector index */
	isa_outb(IO_ICU1 + PIC_ICW2, 0);
	/* slave on line 2 */
	isa_outb(IO_ICU1 + PIC_ICW3, ICW3_CASCADE(IRQ_SLAVE));
	/* 8086 mode */
	isa_outb(IO_ICU1 + PIC_ICW4, ICW4_8086);

	/* leave interrupts masked */
	isa_outb(IO_ICU1 + PIC_OCW1, 0xff);

	/* special mask mode (if available) */
	isa_outb(IO_ICU1 + PIC_OCW3, OCW3_SELECT | OCW3_SSMM | OCW3_SMM);
	/* Read IRR by default. */
	isa_outb(IO_ICU1 + PIC_OCW3, OCW3_SELECT | OCW3_RR);
#ifdef REORDER_IRQ
	/* pri order 3-7, 0-2 (com2 first) */
	isa_outb(IO_ICU1 + PIC_OCW2,
	    OCW2_SELECT | OCW2_R | OCW2_SL OCW2_ILS(3 - 1));
#endif

	/* reset; program device, four bytes */
	isa_outb(IO_ICU2 + PIC_ICW1, ICW1_SELECT | ICW1_IC4);
	/* staring at this vector index */
	isa_outb(IO_ICU2 + PIC_ICW2, 8);
	/* slave connected to line 2 of master */
	isa_outb(IO_ICU2 + PIC_ICW3, ICW3_SIC(IRQ_SLAVE));
	/* 8086 mode */
	isa_outb(IO_ICU2 + PIC_ICW4, ICW4_8086);	

	/* leave interrupts masked */
	isa_outb(IO_ICU2 + PIC_OCW1, 0xff);

	/* special mask mode (if available) */
	isa_outb(IO_ICU2 + PIC_OCW3, OCW3_SELECT | OCW3_SSMM | OCW3_SMM);
	/* Read IRR by default. */
	isa_outb(IO_ICU2 + PIC_OCW3, OCW3_SELECT | OCW3_RR);
}


/*
 *	SPEAKER BEEPER...
 */
void
sysbeepstop(void *arg)
{
	int s;

	/* disable counter 2 */
	s = splhigh();
	isa_outb(PITAUX_PORT, isa_inb(PITAUX_PORT) & ~PIT_SPKR);
	splx(s);
	beeping = 0;
}

void
sysbeep(int pitch, int period)
{
	static int last_pitch, last_period;
	int s;

	if (cold)
		return;		/* Can't beep yet. */

	if (beeping)
		callout_stop(&sysbeep_ch);
	if (!beeping || last_pitch != pitch) {
		s = splhigh();
		isa_outb(IO_TIMER1 + TIMER_MODE,
		    TIMER_SEL2 | TIMER_16BIT | TIMER_SQWAVE);
		isa_outb(IO_TIMER1 + TIMER_CNTR2, TIMER_DIV(pitch) % 256);
		isa_outb(IO_TIMER1 + TIMER_CNTR2, TIMER_DIV(pitch) / 256);
		isa_outb(PITAUX_PORT, isa_inb(PITAUX_PORT) | PIT_SPKR);
		splx(s);
	}
	last_pitch = pitch;
	beeping = last_period = period;
	callout_reset(&sysbeep_ch, period, sysbeepstop, NULL);
}

int
isa_intr_alloc(isa_chipset_tag_t c, int mask, int type, int *irq_p)
{
	int irq;
	int maybe_irq = -1;
	int shared_depth = 0;
	mask &= 0x8b28; /* choose from 3, 5, 8, 9, 11, 15 XXX */
	for (irq = 0; mask != 0; mask >>= 1, irq++) {
		if ((mask & 1) == 0)
			continue;
		if (intrtype[irq] == IST_NONE) {
			*irq_p = irq;
			return 0;
		}
		/* Level interrupts can be shared */
		if (type == IST_LEVEL && intrtype[irq] == IST_LEVEL) {
			struct isa_intrhand *ih = isa_intrhand[irq];
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

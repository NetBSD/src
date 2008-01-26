/*	$NetBSD: isa_machdep.c,v 1.6.50.4 2008/01/26 19:27:10 chris Exp $	*/

/*-
 * Copyright (c) 1996-1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mark Brinicombe, Charles M. Hannum and by Jason R. Thorpe of the
 * Numerical Aerospace Simulation Facility, NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: isa_machdep.c,v 1.6.50.4 2008/01/26 19:27:10 chris Exp $");

#include "opt_irqstats.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#define _ARM32_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <machine/intr.h>
#include <machine/pio.h>
#include <machine/bootconfig.h>
#include <machine/isa_machdep.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/isa/isadmareg.h>
#include <dev/isa/isadmavar.h>
#include <arm/footbridge/isa/icu.h>
#include <arm/footbridge/dc21285reg.h>
#include <arm/footbridge/dc21285mem.h>
#include <dev/ic/i8259reg.h>

#include <uvm/uvm_extern.h>

#include "isadma.h"

/* prototypes */
static void isa_icu_init __P((void));

struct arm32_isa_chipset isa_chipset_tag;

void isa_strayintr __P((int));
void intr_calculatemasks __P((void));

int isa_irqdispatch __P((void *arg));

uint32_t imen;

static void isa_set_irq_mask(uint32_t intr_enabled);
static void isa_set_irq_hardware_type(int irq, int type);
static struct pic_softc isa_pic =
{
	.pic_ops.pic_set_irq_hardware_mask = isa_set_irq_mask,
	.pic_ops.pic_set_irq_hardware_type = isa_set_irq_hardware_type,
	.pic_nirqs = ICU_LEN,
	.pic_pre_assigned_irqs = 0xefbf,
	.pic_name = "isa"
};


#define AUTO_EOI_1
#define AUTO_EOI_2

/*
 * Fill in default interrupt table (in case of spuruious interrupt
 * during configuration of kernel, setup interrupt control unit
 */
static void
isa_icu_init(void)
{
	/* reset; program device, four bytes */
	outb(IO_ICU1 + PIC_ICW1, ICW1_SELECT | ICW1_IC4);

	/* starting at this vector index */
	outb(IO_ICU1 + PIC_ICW2, ICU_OFFSET);
	/* slave on line 2 */
	outb(IO_ICU1 + PIC_ICW3, ICW3_CASCADE(IRQ_SLAVE));

#ifdef AUTO_EOI_1
	/* auto EOI, 8086 mode */
	outb(IO_ICU1 + PIC_ICW4, ICW4_AEOI | ICW4_8086);
#else
	/* 8086 mode */
	outb(IO_ICU1 + PIC_ICW4, ICW4_8086);
#endif
	/* leave interrupts masked */
	outb(IO_ICU1 + PIC_OCW1, 0xff);
	/* special mask mode (if available) */
	outb(IO_ICU1 + PIC_OCW3, OCW3_SELECT | OCW3_SSMM | OCW3_SMM);
	/* Read IRR by default. */
	outb(IO_ICU1 + PIC_OCW3, OCW3_SELECT | OCW3_RR);
#ifdef REORDER_IRQ
	/* pri order 3-7, 0-2 (com2 first) */
	outb(IO_ICU1 + PIC_OCW2, OCW2_SELECT | OCW2_R | OCW2_SL |
		OCW2_ILS(3 - 1));
#endif

	/* reset; program device, four bytes */
	outb(IO_ICU2 + PIC_ICW1, ICW1_SELECT | ICW1_IC4);

	/* staring at this vector index */
	outb(IO_ICU2 + PIC_ICW2, ICU_OFFSET + 8);
	/* slave connected to line 2 of master */
	outb(IO_ICU2 + PIC_ICW3, ICW3_SIC(IRQ_SLAVE));
#ifdef AUTO_EOI_2
	/* auto EOI, 8086 mode */
	outb(IO_ICU2 + PIC_ICW4, ICW4_AEOI | ICW4_8086);
#else
	/* 8086 mode */
	outb(IO_ICU2 + PIC_ICW4, ICW4_8086);
#endif
	/* leave interrupts masked */
	outb(IO_ICU2 + PIC_OCW1, 0xff);
        /* special mask mode (if available) */
	outb(IO_ICU2 + PIC_OCW3, OCW3_SELECT | OCW3_SSMM | OCW3_SMM);
        /* Read IRR by default. */
	outb(IO_ICU2 + PIC_OCW3, OCW3_SELECT | OCW3_RR);
}

/*
 * Caught a stray interrupt, notify
 */
void
isa_strayintr(irq)
	int irq;
{
	static u_long strays;

        /*
         * Stray interrupts on irq 7 occur when an interrupt line is raised
         * and then lowered before the CPU acknowledges it.  This generally
         * means either the device is screwed or something is cli'ing too
         * long and it's timing out.
         */
	if (++strays <= 5)
		log(LOG_ERR, "stray interrupt %d%s\n", irq,
		    strays >= 5 ? "; stopped logging" : "");
}

#define	LEGAL_IRQ(x)	((x) >= 0 && (x) < ICU_LEN && (x) != 2)

int
isa_intr_alloc(ic, mask, type, irq)
	isa_chipset_tag_t ic;
	int mask;
	int type;
	int *irq;
{
	int i, tmp, bestirq, count;
	struct intrline *il;
	struct intrhand *ih;

	if (type == IST_NONE)
		panic("intr_alloc: bogus type");

	bestirq = -1;
	count = -1;

	/* some interrupts should never be dynamically allocated */
	mask &= 0xdef8;

	/*
	 * XXX some interrupts will be used later (6 for fdc, 12 for pms).
	 * the right answer is to do "breadth-first" searching of devices.
	 */
	mask &= 0xefbf;

	for (i = 0; i < ICU_LEN; i++) {
		if (LEGAL_IRQ(i) == 0 || (mask & (1<<i)) == 0)
			continue;
		
		/* XXX shouldn't expose internals of arm_intr here */
		il = &(isa_pic.pic_intrlines[i]);
		switch(il->il_ist) {
		case IST_NONE:
			/*
			 * if nothing's using the irq, just return it
			 */
			*irq = i;
			return (0);

		case IST_EDGE:
		case IST_LEVEL:
			if (type != il->il_ist)
				continue;
			/*
			 * if the irq is shareable, count the number of other
			 * handlers, and if it's smaller than the last irq like
			 * this, remember it
			 *
			 * XXX We should probably also consider the
			 * interrupt level and stick IPL_TTY with other
			 * IPL_TTY, etc.
			 */
			tmp = 0;
			TAILQ_FOREACH(ih, &(il->il_handler_list), ih_list)
			     tmp++;
			if ((bestirq == -1) || (count > tmp)) {
				bestirq = i;
				count = tmp;
			}
			break;

		case IST_PULSE:
			/* this just isn't shareable */
			continue;
		}
	}

	if (bestirq == -1)
		return (1);

	*irq = bestirq;

	return (0);
}

const struct evcnt *
isa_intr_evcnt(isa_chipset_tag_t ic, int irq)
{
    return arm_intr_evcnt(&isa_pic, irq);
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
	if (!LEGAL_IRQ(irq) || type == IST_NONE)
		panic("intr_establish: bogus irq or type");

	return arm_intr_claim(&isa_pic, irq, type, level, NULL, ih_fun, ih_arg);
}

/*
 * Deregister an interrupt handler.
 */
void
isa_intr_disestablish(ic, arg)
	isa_chipset_tag_t ic;
	void *arg;
{
	return arm_intr_disestablish(&isa_pic, arg);
}

static void
isa_set_irq_mask(uint32_t intr_enabled)
{
	uint32_t oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	/* slave is always enabled */
	imen = ~(intr_enabled | (1 << IRQ_SLAVE));
	imen &=0xffff;
	SET_ICUS();
	restore_interrupts(oldirqstate);
}

static void
isa_set_irq_hardware_type(int irq, int type)
{
	/* irq trigger types are setup in the m1543 */
	if (irq < 8) {
		outb(0x4d0, (inb(0x4d0) & ~(1 << irq))
    				| ((type == IST_LEVEL) ? (1 << irq) : 0));
	} else {
		outb(0x4d1, (inb(0x4d1) & ~(1 << irq))
    				| ((type == IST_LEVEL) ? (1 << irq) : 0));
	}
}
		
/*
 * isa_intr_init()
 *
 * Initialise the ISA ICU and attach an ISA interrupt handler to the
 * ISA interrupt line on the footbridge.
 */
void
isa_intr_init(void)
{
	static void *isa_ih;
 
	isa_icu_init();

	arm_intr_register_pic(&isa_pic);
	
	/* something to break the build in an informative way */
#ifndef ISA_FOOTBRIDGE_IRQ 
#warning Before using isa with footbridge you must define ISA_FOOTBRIDGE_IRQ
#endif
	isa_ih = footbridge_intr_claim(ISA_FOOTBRIDGE_IRQ, IPL_IRQBUS, "isabus",
	    isa_irqdispatch, NULL);
	
}

/* Static array of ISA DMA segments. We only have one on CATS */
#if NISADMA > 0
struct arm32_dma_range machdep_isa_dma_ranges[1];
#endif

void
isa_footbridge_init(iobase, membase)
	u_int iobase, membase;
{
#if NISADMA > 0
	extern struct arm32_dma_range *footbridge_isa_dma_ranges;
	extern int footbridge_isa_dma_nranges;

	machdep_isa_dma_ranges[0].dr_sysbase = bootconfig.dram[0].address;
	machdep_isa_dma_ranges[0].dr_busbase = bootconfig.dram[0].address;
	machdep_isa_dma_ranges[0].dr_len = (16 * 1024 * 1024);

	footbridge_isa_dma_ranges = machdep_isa_dma_ranges;
	footbridge_isa_dma_nranges = 1;
#endif

	isa_io_init(iobase, membase);
}

void
isa_attach_hook(parent, self, iba)
	struct device *parent, *self;
	struct isabus_attach_args *iba;
{
	/*
	 * Since we can only have one ISA bus, we just use a single
	 * statically allocated ISA chipset structure.  Pass it up
	 * now.
	 */
	iba->iba_ic = &isa_chipset_tag;
#if NISADMA > 0
	isa_dma_init();
#endif
}

int
isa_irqdispatch(arg)
	void *arg;
{
	uint32_t ipendingmask;
	uint32_t oldirqstate;

	/* disable irqs while reading from the ICUs
	 * Note that this could be an splhigh, except that serial ports
	 * attach to isa, and so they wouldn't be blocked */
	oldirqstate = disable_interrupts(I32_bit);
	
	/* read from the isa registers */
	ipendingmask = inb(IO_ICU1);

	if (ipendingmask & (1 << IRQ_SLAVE))
	{
		ipendingmask &= ~(1 << IRQ_SLAVE);
		ipendingmask |= inb(IO_ICU2) << 8;
	}
	restore_interrupts(oldirqstate);
	
	/*
	 * Setup the interrupts into the ipl lists.
	 * They'll be processed later, as the only way to get here is from
	 * an interrupt
	 */
	arm_intr_queue_irqs(&isa_pic, ipendingmask);

	return 1;
}


void
isa_fillw(val, addr, len)
	u_int val;
	void *addr;
	size_t len;
{
	if ((u_int)addr >= isa_mem_data_vaddr()
	    && (u_int)addr < isa_mem_data_vaddr() + 0x100000) {
		bus_size_t offset = ((u_int)addr) & 0xfffff;
		bus_space_set_region_2(&isa_mem_bs_tag,
		    (bus_space_handle_t)isa_mem_bs_tag.bs_cookie, offset,
		    val, len);
	} else {
		u_short *ptr = addr;

		while (len > 0) {
			*ptr++ = val;
			--len;
		}
	}
}

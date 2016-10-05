/*	$NetBSD: ralink_intr.c,v 1.3.30.1 2016/10/05 20:55:32 skrll Exp $	*/
/*-
 * Copyright (c) 2011 CradlePoint Technology, Inc.
 * All rights reserved.
 *
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
 * THIS SOFTWARE IS PROVIDED BY CRADLEPOINT TECHNOLOGY, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define __INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ralink_intr.c,v 1.3.30.1 2016/10/05 20:55:32 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <mips/locore.h>

#include <mips/ralink/ralink_reg.h>
#include <mips/ralink/ralink_var.h>

static int ra_pic_intr(void *arg);

/*
 * evbmips spl integration:
 *  this is a mask of bits to clear in the SR when we go to a
 *  given hardware interrupt priority level.
 */
static const struct ipl_sr_map ralink_ipl_sr_map = {
    .sr_bits = {
	    [IPL_NONE]		= 0,
	    [IPL_SOFTCLOCK]	= MIPS_SOFT_INT_MASK_0,
	    [IPL_SOFTBIO]	= MIPS_SOFT_INT_MASK_0,
	    [IPL_SOFTNET]	= MIPS_SOFT_INT_MASK,
	    [IPL_SOFTSERIAL]	= MIPS_SOFT_INT_MASK,
	    [IPL_VM]		= MIPS_INT_MASK ^ MIPS_INT_MASK_5,
	    [IPL_SCHED]		= MIPS_INT_MASK,
	    [IPL_DDB]		= MIPS_INT_MASK,
	    [IPL_HIGH]		= MIPS_INT_MASK,
     },
};


/*
 * RT3052 Interrupt Block Definitions
 *
 * HW_INT0 - Low Priority Chip Interrupts (Lowest Priority)
 * HW_INT1 - High Priority Chip Interrupts
 * HW_INT2 - PCIe/PCI (3883 only)
 * HW_INT3 - Frame Engine
 * HW_INT4 - 802.11n NIC
 * HW_INT5 - Timer Interrupt (Highest Priority)
 *
 * HW_INT0 and HW_INT1 can be configured to fire with any of the other
 *  interrupts on chip.  They can be masked for either INT0 or INT1
 *  but not both.
 *
 * SYSCTL
 * TIMER0
 * WDTIMER
 * ILLACC
 * PCM
 * UARTF
 * PIO
 * DMA
 * NAND
 * PERF
 * I2S
 * UARTL
 * ETHSW
 * USB
 */


/*
 * we use 5 MIPS cpu interrupts: 
 *	MIPS INT0 .. INT4
 */
#define NCPUINTRS	5

struct ra_intr {
	LIST_HEAD(, evbmips_intrhand) intr_list;
	struct evcnt intr_evcnt;
};

/*
 * ordering for ra_intrtab[] and ra_intr_names[]
 * corresponds to the RA_IRQ_* definitions
 * which include the CPU intrs and the PIC intrs
 */
static struct ra_intr ra_intrtab[RA_IRQ_MAX];
static const char * const ra_intr_names[RA_IRQ_MAX] = {
        "intr 0 (lowpri)",
        "intr 1 (highpri)",
        "intr 2 (pci)",
        "intr 3 (frame)",
        "intr 4 (wlan)",
        "intr 5 (timer)",
	"intr 0 (sysctl)",
	"intr 1 (timer0)",
	"intr 2 (watchdog)",
	"intr 3 (illacc)",
	"intr 4 (pcm)",
	"intr 5 (uartf)",
	"intr 6 (gpio)",
	"intr 7 (dma)",
	"intr 8 (nand)",
	"intr 9 (perf)",
	"intr 10 (i2s)",
	"intr 12 (uartl)",
	"intr 17 (ethsw)",
	"intr 18 (usb)"
};

/* determine if irq belongs to the PIC */
#define PIC_IRQ_P(irq)	((irq) > RA_IRQ_TIMER)

/* map the IRQ num to PIC reg bits */
static const uint8_t irq2bit[RA_IRQ_MAX] = {
    -1, -1, -1, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 17, 18
};

/* map the PIC reg bits to IRQ num */
static const uint8_t bit2irq[19] = {
    6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 255, 17, 255, 255, 255, 255, 18, 19
};



static inline uint32_t
intctl_read(u_int offset)
{
	return *RA_IOREG_VADDR(RA_INTCTL_BASE, offset);
}

static inline void
intctl_write(u_int offset, uint32_t val)
{
	*RA_IOREG_VADDR(RA_INTCTL_BASE, offset) = val;
}


void
evbmips_intr_init(void)
{
	ipl_sr_map = ralink_ipl_sr_map;

	for (int irq=0; irq < RA_IRQ_MAX; irq++) {
		LIST_INIT(&ra_intrtab[irq].intr_list);
		if (PIC_IRQ_P(irq)) {
			evcnt_attach_dynamic(&ra_intrtab[irq].intr_evcnt,
				EVCNT_TYPE_INTR, NULL, "pic",
				ra_intr_names[irq]);
		} else {
			evcnt_attach_dynamic(&ra_intrtab[irq].intr_evcnt,
				EVCNT_TYPE_INTR, NULL, "cpu0",
				ra_intr_names[irq]);
		}
	}

	/*
	 * make sure we start without any misc interrupts enabled,
	 * but the block enabled
	 */
	intctl_write(RA_INTCTL_DISABLE, ~0);
	intctl_write(RA_INTCTL_ENABLE, INT_GLOBAL);

	/* 
	 * establish the low/high priority cpu interrupts.
	 * note here we pass the value of the priority as the argument
	 * so it is passed to ra_pic_intr() correctly.
	 */
	ra_intr_establish(RA_IRQ_HIGH, ra_pic_intr,
		(void *)1, 1);
	ra_intr_establish(RA_IRQ_LOW, ra_pic_intr,
		(void *)0, 0);
}


void *
ra_intr_establish(int intr, int (*func)(void *), void *arg, int priority)
{
	struct evbmips_intrhand *ih;

	if ((ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT)) == NULL) {
		KASSERTMSG(0, "%s: cannot malloc intrhand", __func__);
		return NULL;
	}

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_irq = intr;

	const int s = splhigh();

	LIST_INSERT_HEAD(&ra_intrtab[intr].intr_list, ih, ih_q);

	if (PIC_IRQ_P(intr)) {
		/* irq belongs to the PIC */
		uint32_t r;
		r = intctl_read(RA_INTCTL_TYPE);
		r |= (priority << irq2bit[intr]);
		intctl_write(RA_INTCTL_TYPE, r);
		r = intctl_read(RA_INTCTL_ENABLE);
		r |= (1 << irq2bit[intr]);
		intctl_write(RA_INTCTL_ENABLE, r);
	}

	splx(s);

	return ih;
}

void
ra_intr_disestablish(void *arg)
{
	struct evbmips_intrhand * const ih = arg;

	const int s = splhigh();

	LIST_REMOVE(ih, ih_q);
	if (PIC_IRQ_P(ih->ih_irq) &&
	    LIST_EMPTY(&ra_intrtab[ih->ih_irq].intr_list)) {
		uint32_t r;
		r = intctl_read(RA_INTCTL_DISABLE);
		r &= ~(1 << irq2bit[ih->ih_irq]);
		intctl_write(RA_INTCTL_DISABLE, r);
	}

	splx(s);

	free(ih, M_DEVBUF);
}

/*
 * ra_pic_intr - service PIC interrupts
 *
 * caller handles priority by the calling this function w/ PRI_HIGH first
 */
static int
ra_pic_intr(void *arg)
{
	const int priority = (intptr_t)arg;
	const u_int off = (priority == 0) ?
		RA_INTCTL_IRQ0STAT : RA_INTCTL_IRQ1STAT;
	uint32_t pending = intctl_read(off);

	while (pending != 0) {
		const u_int bitno = 31 - __builtin_clz(pending);
		pending ^= (1 << bitno);
		const int irq = bit2irq[bitno];
		KASSERT(PIC_IRQ_P(irq));
		ra_intrtab[irq].intr_evcnt.ev_count++;
		struct evbmips_intrhand *ih;
		LIST_FOREACH(ih, &ra_intrtab[irq].intr_list, ih_q)
			(*ih->ih_func)(ih->ih_arg);
	}

	return 1;
}

/*
 * evbmips_iointr - process CPU interrupts
 *
 * we only see IRQ 4..0 here as IRQ 5 is handled
 * in the generic MIPS code for the timer
 */
void
evbmips_iointr(int ipl, uint32_t ipending, struct clockframe *cf)
{
	while (ipending != 0) {
		const u_int bitno = 31 - __builtin_clz(ipending);
		ipending ^= (1 << bitno);
		const int irq = bitno - (31 - __builtin_clz(MIPS_INT_MASK_0));
		KASSERT(!PIC_IRQ_P(irq));
		ra_intrtab[irq].intr_evcnt.ev_count++;
		struct evbmips_intrhand *ih;
		LIST_FOREACH(ih, &ra_intrtab[irq].intr_list, ih_q)
			(*ih->ih_func)(ih->ih_arg);
	}
}

/*	$NetBSD: ralink_intr.c,v 1.3.12.1 2017/12/03 11:36:28 jdolecek Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: ralink_intr.c,v 1.3.12.1 2017/12/03 11:36:28 jdolecek Exp $");

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
	/* CPU interrupts */
	[RA_IRQ_LOW]	= "intr 0 (lowpri)",
	[RA_IRQ_HIGH]	= "intr 1 (highpri)",
	[RA_IRQ_PCI]	= "intr 2 (pci)",
	[RA_IRQ_FENGINE]= "intr 3 (frame)",
	[RA_IRQ_WLAN]	= "intr 4 (wlan)",
	[RA_IRQ_TIMER]	= "intr 5 (timer)",

	/* Interrupt controller */
	[RA_IRQ_SYSCTL]	= "intc sysctl",
	[RA_IRQ_TIMER0]	= "intc timer0",
	[RA_IRQ_WDOG]	= "intc wdog",
	[RA_IRQ_ILLACC]	= "intc illacc",
	[RA_IRQ_PCM]	= "intc pcm",
	[RA_IRQ_UARTF]	= "intc uartf",
	[RA_IRQ_PIO]	= "intc gpio",
	[RA_IRQ_DMA]	= "intc dma",
	[RA_IRQ_NAND]	= "intc nand",
	[RA_IRQ_PERF]	= "intc pef",
	[RA_IRQ_I2S]	= "intc i2s",
	[RA_IRQ_SPI]	= "intc spi",
	[RA_IRQ_UARTL]	= "intc uartl",
	[RA_IRQ_CRYPTO]	= "intc crypto",
	[RA_IRQ_SDHC]	= "intc sdhc",
	[RA_IRQ_R2P]	= "intc r2p",
	[RA_IRQ_ETHSW]	= "intc ethsw",
	[RA_IRQ_USB]	= "intc usb",
	[RA_IRQ_UDEV]	= "intc udev",
	[RA_IRQ_UART1]	= "intc uart1",
	[RA_IRQ_UART2]	= "intc uart2",
};

/* determine if irq belongs to the PIC */
#define PIC_IRQ_P(irq)	((irq) > RA_IRQ_TIMER)

/* map the IRQ num to PIC reg bits */
static const uint8_t irq2bit[RA_IRQ_MAX] = {
	/* CPU interrupts */
	[RA_IRQ_LOW]	= -1,
	[RA_IRQ_HIGH]	= -1,
	[RA_IRQ_PCI]	= -1,
	[RA_IRQ_FENGINE]= -1,
	[RA_IRQ_WLAN]	= -1,
	[RA_IRQ_TIMER]	= -1,

	/* Interrupt controller */
	[RA_IRQ_SYSCTL]	= INT_SYSCTL,
	[RA_IRQ_TIMER0]	= INT_TIMER0,
	[RA_IRQ_WDOG]	= INT_WDOG,
	[RA_IRQ_ILLACC]	= INT_ILLACC,
	[RA_IRQ_PCM]	= INT_PCM,
	[RA_IRQ_UARTF]	= INT_UARTF,
	[RA_IRQ_PIO]	= INT_PIO,
	[RA_IRQ_DMA]	= INT_DMA,
	[RA_IRQ_NAND]	= INT_NAND,
	[RA_IRQ_PERF]	= INT_PERF,
	[RA_IRQ_I2S]	= INT_I2S,
	[RA_IRQ_SPI]	= INT_SPI,
	[RA_IRQ_UARTL]	= INT_UARTL,
#ifdef INT_UART1
	[RA_IRQ_UART1]	= INT_UART1,
#endif
#ifdef INT_UART2
	[RA_IRQ_UART2]	= INT_UART2,
#endif
	[RA_IRQ_CRYPTO]	= INT_CRYPTO,
	[RA_IRQ_SDHC]	= INT_SDHC,
	[RA_IRQ_R2P]	= INT_R2P,
	[RA_IRQ_ETHSW]	= INT_ETHSW,
	[RA_IRQ_USB]	= INT_USB,
	[RA_IRQ_UDEV]	= INT_UDEV
};

/* map the PIC reg bits to IRQ num */
static const uint8_t bit2irq[32] = {
	[INT_SYSCTL]	= RA_IRQ_SYSCTL,
	[INT_TIMER0]	= RA_IRQ_TIMER0,
	[INT_WDOG]	= RA_IRQ_WDOG,
	[INT_ILLACC]	= RA_IRQ_ILLACC,
	[INT_PCM]	= RA_IRQ_PCM,
	[INT_UARTF]	= RA_IRQ_UARTF,
	[INT_PIO]	= RA_IRQ_PIO,
	[INT_DMA]	= RA_IRQ_DMA,
	[INT_NAND]	= RA_IRQ_NAND,
	[INT_PERF]	= RA_IRQ_PERF,
	[INT_I2S]	= RA_IRQ_I2S,
	[INT_SPI]	= RA_IRQ_SPI,
	[INT_UARTL]	= RA_IRQ_UARTL,
#ifdef INT_UART1
	[INT_UART1]	= RA_IRQ_UART1,
#endif
#ifdef INT_UART2
	[INT_UART2]	= RA_IRQ_UART2,
#endif
	[INT_CRYPTO]	= RA_IRQ_CRYPTO,
	[INT_SDHC]	= RA_IRQ_SDHC,
	[INT_R2P]	= RA_IRQ_R2P,
	[INT_ETHSW]	= RA_IRQ_ETHSW,
	[INT_USB]	= RA_IRQ_USB,
	[INT_UDEV]	= RA_IRQ_UDEV
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
	intctl_write(RA_INTCTL_ENABLE, INT_GLOBAL_EN);

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

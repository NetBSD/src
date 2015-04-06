/*	$NetBSD: bcm2835_intr.c,v 1.4.2.1 2015/04/06 15:17:52 skrll Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcm2835_intr.c,v 1.4.2.1 2015/04/06 15:17:52 skrll Exp $");

#define _INTR_PRIVATE

#include "opt_bcm283x.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <machine/intr.h>

#include <arm/locore.h>

#include <arm/pic/picvar.h>
#include <arm/cortex/gtmr_var.h>

#include <arm/broadcom/bcm_amba.h>
#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835var.h>

static void bcm2835_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void bcm2835_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static int bcm2835_pic_find_pending_irqs(struct pic_softc *);
static void bcm2835_pic_establish_irq(struct pic_softc *, struct intrsource *);
static void bcm2835_pic_source_name(struct pic_softc *, int, char *,
    size_t);

#if defined(BCM2836)
static void bcm2836mp_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void bcm2836mp_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static int bcm2836mp_pic_find_pending_irqs(struct pic_softc *);
static void bcm2836mp_pic_establish_irq(struct pic_softc *, struct intrsource *);
static void bcm2836mp_pic_source_name(struct pic_softc *, int, char *,
    size_t);
#if 0
#ifdef MULTIPROCESSOR
int bcm2836mp_ipi_handler(void *);
static void bcm2836mp_cpu_init(struct pic_softc *, struct cpu_info *);
static void bcm2836mp_send_ipi(struct pic_softc *, const kcpuset_t *, u_long);
#endif
#endif
#endif

#ifdef MULTIPROCESSOR
static void
bcm2835_dummy(struct pic_softc *pic, const kcpuset_t *kcp, u_long ipi)
{
}
#endif

static int  bcm2835_icu_match(device_t, cfdata_t, void *);
static void bcm2835_icu_attach(device_t, device_t, void *);

static struct pic_ops bcm2835_picops = {
	.pic_unblock_irqs = bcm2835_pic_unblock_irqs,
	.pic_block_irqs = bcm2835_pic_block_irqs,
	.pic_find_pending_irqs = bcm2835_pic_find_pending_irqs,
	.pic_establish_irq = bcm2835_pic_establish_irq,
	.pic_source_name = bcm2835_pic_source_name,
#if defined(MULTIPROCESSOR)
	.pic_ipi_send = bcm2835_dummy,
#endif
};

struct pic_softc bcm2835_pic = {
	.pic_ops = &bcm2835_picops,
	.pic_maxsources = BCM2835_NIRQ,
	.pic_name = "bcm2835 pic",
};

#if defined(BCM2836)
static struct pic_ops bcm2836mp_picops = {
	.pic_unblock_irqs = bcm2836mp_pic_unblock_irqs,
	.pic_block_irqs = bcm2836mp_pic_block_irqs,
	.pic_find_pending_irqs = bcm2836mp_pic_find_pending_irqs,
	.pic_establish_irq = bcm2836mp_pic_establish_irq,
	.pic_source_name = bcm2836mp_pic_source_name,
#if 0 && defined(MULTIPROCESSOR)
	.pic_cpu_init = bcm2836mp_cpu_init,
	.pic_ipi_send = bcm2836mp_send_ipi,
#endif
};

struct pic_softc bcm2836mp_pic = {
	.pic_ops = &bcm2836mp_picops,
	.pic_maxsources = BCM2836MP_NIRQ,
	.pic_name = "bcm2836 mp pic",
};
#endif

struct bcm2835icu_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct pic_softc	*sc_pic;
};

struct bcm2835icu_softc *bcmicu_sc;

#define read_bcm2835reg(o)	\
	bus_space_read_4(bcmicu_sc->sc_iot, bcmicu_sc->sc_ioh, (o))

#define write_bcm2835reg(o, v)	\
	bus_space_write_4(bcmicu_sc->sc_iot, bcmicu_sc->sc_ioh, (o), (v))


#define bcm2835_barrier() \
	bus_space_barrier(bcmicu_sc->sc_iot, bcmicu_sc->sc_ioh, 0, \
	    BCM2835_ARMICU_SIZE, BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE)

static const char * const bcm2835_sources[BCM2835_NIRQ] = {
	"(unused  0)",	"(unused  1)",	"(unused  2)",	"timer3",
	"(unused  4)",	"(unused  5)",	"(unused  6)",	"jpeg",
	"(unused  8)",	"usb",		"(unused 10)",	"(unused 11)",
	"(unused 12)",	"(unused 13)",	"(unused 14)",	"(unused 15)",
	"dma0",		"dma1",		"dma2",		"dma3",
	"dma4",		"dma5",		"dma6",		"dma7",
	"dma8",		"dma9",		"dma10",	"dma11",
	"dma12",	"aux",		"(unused 30)",	"(unused 31)",
	"(unused 32)",	"(unused 33)",	"(unused 34)",	"(unused 35)",
	"(unused 36)",	"(unused 37)",	"(unused 38)",	"(unused 39)",
	"(unused 40)",	"(unused 41)",	"(unused 42)",	"i2c spl slv",
	"(unused 44)",	"pwa0",		"pwa1",		"(unused 47)",
	"smi",		"gpio[0]",	"gpio[1]",	"gpio[2]",
	"gpio[3]",	"i2c",		"spi",		"pcm",
	"sdio",		"uart",		"(unused 58)",	"(unused 59)",
	"(unused 60)",	"(unused 61)",	"emmc",		"(unused 63)",
	"Timer",	"Mailbox",	"Doorbell0",	"Doorbell1",
	"GPU0 Halted",	"GPU1 Halted",	"Illegal #1",	"Illegal #0"
};

#if defined(BCM2836)
static const char * const bcm2836mp_sources[BCM2836MP_NIRQ] = {
	"cntpsirq",	"cntpnsirq",	"cnthpirq",	"cntvirq",
	"mailbox0",	"mailbox1",	"mailbox2",	"mailbox3",
};
#endif

#define	BCM2836_INTBIT_GPUPENDING	__BIT(8)

#define	BCM2835_INTBIT_PENDING1		__BIT(8)
#define	BCM2835_INTBIT_PENDING2		__BIT(9)
#define	BCM2835_INTBIT_ARM		__BITS(0,7)
#define	BCM2835_INTBIT_GPU0		__BITS(10,14)
#define	BCM2835_INTBIT_GPU1		__BITS(15,20)

CFATTACH_DECL_NEW(bcmicu, sizeof(struct bcm2835icu_softc),
    bcm2835_icu_match, bcm2835_icu_attach, NULL, NULL);

static int
bcm2835_icu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct amba_attach_args *aaa = aux;

	if (strcmp(aaa->aaa_name, "icu") != 0)
		return 0;

	return 1;
}

static void
bcm2835_icu_attach(device_t parent, device_t self, void *aux)
{
	struct bcm2835icu_softc *sc = device_private(self);
	struct amba_attach_args *aaa = aux;

	sc->sc_dev = self;
	sc->sc_iot = aaa->aaa_iot;
	sc->sc_pic = &bcm2835_pic;

	if (bus_space_map(aaa->aaa_iot, aaa->aaa_addr, aaa->aaa_size, 0,
	    &sc->sc_ioh)) {
		aprint_error_dev(self, "unable to map device\n");
		return;
	}

	bcmicu_sc = sc;

	pic_add(sc->sc_pic, 0);

#if defined(BCM2836)
#if 0 && defined(MULTIPROCESSOR)
	aprint_normal(": Multiprocessor");
#endif
	pic_add(&bcm2836mp_pic, BCM2836_INT_LOCALBASE);
#endif

	aprint_normal("\n");
}

void
bcm2835_irq_handler(void *frame)
{
	struct cpu_info * const ci = curcpu();
	const int oldipl = ci->ci_cpl;
	const uint32_t oldipl_mask = __BIT(oldipl);
	int ipl_mask = 0;

	ci->ci_data.cpu_nintr++;

	bcm2835_barrier();
	ipl_mask = bcm2835_pic_find_pending_irqs(&bcm2835_pic);
#if defined(BCM2836)
	ipl_mask |= bcm2836mp_pic_find_pending_irqs(&bcm2836mp_pic);
#endif

	/*
	 * Record the pending_ipls and deliver them if we can.
	 */
	if ((ipl_mask & ~oldipl_mask) > oldipl_mask)
		pic_do_pending_ints(I32_bit, oldipl, frame);
}

static void
bcm2835_pic_unblock_irqs(struct pic_softc *pic, size_t irqbase,
    uint32_t irq_mask)
{

	write_bcm2835reg(BCM2835_INTC_ENABLEBASE + (irqbase >> 3), irq_mask);
	bcm2835_barrier();
}

static void
bcm2835_pic_block_irqs(struct pic_softc *pic, size_t irqbase,
    uint32_t irq_mask)
{

	write_bcm2835reg(BCM2835_INTC_DISABLEBASE + (irqbase >> 3), irq_mask);
	bcm2835_barrier();
}

/*
 * Called with interrupts disabled
 */
static int
bcm2835_pic_find_pending_irqs(struct pic_softc *pic)
{
	int ipl = 0;
	uint32_t bpending, gpu0irq, gpu1irq, armirq;

	bcm2835_barrier();
	bpending = read_bcm2835reg(BCM2835_INTC_IRQBPENDING);
	if (bpending == 0)
		return 0;

	armirq = bpending & BCM2835_INTBIT_ARM;
	gpu0irq = bpending & BCM2835_INTBIT_GPU0;
	gpu1irq = bpending & BCM2835_INTBIT_GPU1;

	if (armirq) {
		ipl |= pic_mark_pending_sources(pic, BCM2835_INT_BASICBASE,
		    armirq);

	}

	if (gpu0irq || (bpending & BCM2835_INTBIT_PENDING1)) {
		uint32_t pending1;

		pending1 = read_bcm2835reg(BCM2835_INTC_IRQ1PENDING);
		ipl |= pic_mark_pending_sources(pic, BCM2835_INT_GPU0BASE,
		    pending1);
	}
	if (gpu1irq || (bpending & BCM2835_INTBIT_PENDING2)) {
		uint32_t pending2;

		pending2 = read_bcm2835reg(BCM2835_INTC_IRQ2PENDING);
		ipl |= pic_mark_pending_sources(pic, BCM2835_INT_GPU1BASE,
		    pending2);
	}

	return ipl;
}

static void
bcm2835_pic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{

	/* Nothing really*/
	KASSERT(is->is_irq < BCM2835_NIRQ);
	KASSERT(is->is_type == IST_LEVEL);
}

static void
bcm2835_pic_source_name(struct pic_softc *pic, int irq, char *buf, size_t len)
{

	strlcpy(buf, bcm2835_sources[irq], len);
}


#if defined(BCM2836)

#define	BCM2836MP_TIMER_IRQS	__BITS(3,0)
#define	BCM2836MP_MAILBOX_IRQS	__BITS(4,4)

#define	BCM2836MP_ALL_IRQS	\
     (BCM2836MP_TIMER_IRQS | BCM2836MP_MAILBOX_IRQS)

static void
bcm2836mp_pic_unblock_irqs(struct pic_softc *pic, size_t irqbase,
    uint32_t irq_mask)
{
	const int cpuid = 0;

//printf("%s: irqbase %zu irq_mask %08x\n", __func__, irqbase, irq_mask);

	if (irq_mask & BCM2836MP_TIMER_IRQS) {
		uint32_t mask = __SHIFTOUT(irq_mask, BCM2836MP_TIMER_IRQS);
		uint32_t val = bus_space_read_4(al_iot, al_ioh,
		    BCM2836_LOCAL_TIMER_IRQ_CONTROLN(cpuid));
		val |= mask;
		bus_space_write_4(al_iot, al_ioh,
		    BCM2836_LOCAL_TIMER_IRQ_CONTROLN(cpuid),
		    val);
		bus_space_barrier(al_iot, al_ioh,
		    BCM2836_LOCAL_TIMER_IRQ_CONTROL_BASE,
		    BCM2836_LOCAL_TIMER_IRQ_CONTROL_SIZE,
		    BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE);
//printf("%s: val %08x\n", __func__, val);
	} else if (irq_mask & BCM2836MP_MAILBOX_IRQS) {
		uint32_t mask = __SHIFTOUT(irq_mask, BCM2836MP_MAILBOX_IRQS);
		uint32_t val = bus_space_read_4(al_iot, al_ioh,
		    BCM2836_LOCAL_MAILBOX_IRQ_CONTROLN(cpuid));
		val |= mask;
		bus_space_write_4(al_iot, al_ioh,
		    BCM2836_LOCAL_MAILBOX_IRQ_CONTROLN(cpuid),
		    val);
		bus_space_barrier(al_iot, al_ioh,
		    BCM2836_LOCAL_MAILBOX_IRQ_CONTROL_BASE,
		    BCM2836_LOCAL_MAILBOX_IRQ_CONTROL_SIZE,
		    BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE);
	}

	return;
}

static void
bcm2836mp_pic_block_irqs(struct pic_softc *pic, size_t irqbase,
    uint32_t irq_mask)
{
	const int cpuid = 0;

//printf("%s: irqbase %zu irq_mask %08x\n", __func__, irqbase, irq_mask);
	if (irq_mask & BCM2836MP_TIMER_IRQS) {
		uint32_t mask = __SHIFTOUT(irq_mask, BCM2836MP_TIMER_IRQS);
		uint32_t val = bus_space_read_4(al_iot, al_ioh,
		    BCM2836_LOCAL_TIMER_IRQ_CONTROLN(cpuid));
		val &= ~mask;
		bus_space_write_4(al_iot, al_ioh,
		    BCM2836_LOCAL_TIMER_IRQ_CONTROLN(cpuid),
		    val);
//printf("%s: val %08x\n", __func__, val);
	} else if (irq_mask & BCM2836MP_MAILBOX_IRQS) {
		uint32_t mask = __SHIFTOUT(irq_mask, BCM2836MP_MAILBOX_IRQS);
		uint32_t val = bus_space_read_4(al_iot, al_ioh,
		    BCM2836_LOCAL_MAILBOX_IRQ_CONTROLN(cpuid));
		val &= ~mask;
		bus_space_write_4(al_iot, al_ioh,
		    BCM2836_LOCAL_MAILBOX_IRQ_CONTROLN(cpuid),
		    val);
	}

	bcm2835_barrier();
	return;
}


static int
bcm2836mp_pic_find_pending_irqs(struct pic_softc *pic)
{
	const int cpuid = 0;
	uint32_t lpending;
	int ipl = 0;

	bcm2835_barrier();

	lpending = bus_space_read_4(al_iot, al_ioh,
	    BCM2836_LOCAL_INTC_IRQPENDINGN(cpuid));

	lpending &= ~BCM2836_INTBIT_GPUPENDING;
	if (lpending & BCM2836MP_ALL_IRQS) {
		ipl |= pic_mark_pending_sources(pic, 0 /* BCM2836_INT_LOCALBASE */,
		    lpending & BCM2836MP_ALL_IRQS);
	}

	return ipl;
}

static void
bcm2836mp_pic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{

	/* Nothing really*/
	KASSERT(is->is_irq >= 0);
	KASSERT(is->is_irq < BCM2836MP_NIRQ);
//	KASSERT(is->is_type == IST_LEVEL);


}

static void
bcm2836mp_pic_source_name(struct pic_softc *pic, int irq, char *buf, size_t len)
{
	irq %= 32;
	strlcpy(buf, bcm2836mp_sources[irq], len);
}
#endif

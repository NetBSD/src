/*	$NetBSD: bcm2835_intr.c,v 1.1.2.4 2013/02/13 01:36:14 riz Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_intr.c,v 1.1.2.4 2013/02/13 01:36:14 riz Exp $");

#define _INTR_PRIVATE

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/device.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <arm/pic/picvar.h>

#include <arm/broadcom/bcm_amba.h>
#include <arm/broadcom/bcm2835reg.h>

static void bcm2835_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void bcm2835_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static int bcm2835_pic_find_pending_irqs(struct pic_softc *);
static void bcm2835_pic_establish_irq(struct pic_softc *, struct intrsource *);
static void bcm2835_pic_source_name(struct pic_softc *, int, char *,
    size_t);

static int  bcm2835_icu_match(device_t, cfdata_t, void *);
static void bcm2835_icu_attach(device_t, device_t, void *);

static struct pic_ops bcm2835_picops = {
	.pic_unblock_irqs = bcm2835_pic_unblock_irqs,
	.pic_block_irqs = bcm2835_pic_block_irqs,
	.pic_find_pending_irqs = bcm2835_pic_find_pending_irqs,
	.pic_establish_irq = bcm2835_pic_establish_irq,
	.pic_source_name = bcm2835_pic_source_name,
};

struct pic_softc bcm2835_pic = {
	.pic_ops = &bcm2835_picops,
	.pic_maxsources = BCM2835_NIRQ,
	.pic_name = "bcm2835 pic",
};

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
	"(unused 16)",	"(unused 17)",	"dma2",		"dma3",
	"(unused 20)",	"(unused 21)",	"(unused 22)",	"(unused 23)",
	"(unused 24)",	"(unused 25)",	"(unused 26)",	"(unused 27)",
	"(unused 28)",	"aux",		"(unused 30)",	"(unused 31)",
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

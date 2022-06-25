/*	$NetBSD: bcm2835_intr.c,v 1.43 2022/06/25 12:41:55 jmcneill Exp $	*/

/*-
 * Copyright (c) 2012, 2015, 2019 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_intr.c,v 1.43 2022/06/25 12:41:55 jmcneill Exp $");

#define _INTR_PRIVATE

#include "opt_bcm283x.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/proc.h>

#include <dev/fdt/fdtvar.h>

#include <machine/intr.h>

#include <arm/locore.h>

#include <arm/pic/picvar.h>
#include <arm/cortex/gtmr_var.h>

#include <arm/broadcom/bcm2835_intr.h>
#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835var.h>

#include <arm/fdt/arm_fdtvar.h>

static void bcm2835_irq_handler(void *);

static void bcm2835_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void bcm2835_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static int bcm2835_pic_find_pending_irqs(struct pic_softc *);
static void bcm2835_pic_establish_irq(struct pic_softc *, struct intrsource *);
static void bcm2835_pic_source_name(struct pic_softc *, int, char *,
    size_t);

static void bcm2836mp_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void bcm2836mp_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static int bcm2836mp_pic_find_pending_irqs(struct pic_softc *);
static void bcm2836mp_pic_establish_irq(struct pic_softc *, struct intrsource *);
static void bcm2836mp_pic_source_name(struct pic_softc *, int, char *,
    size_t);
#ifdef MULTIPROCESSOR
int bcm2836mp_ipi_handler(void *);
static void bcm2836mp_intr_init(struct cpu_info *);
static void bcm2836mp_cpu_init(struct pic_softc *, struct cpu_info *);
static void bcm2836mp_send_ipi(struct pic_softc *, const kcpuset_t *, u_long);
#endif

static int bcm2835_icu_fdt_decode_irq(u_int *);
static void *bcm2835_icu_fdt_establish(device_t, u_int *, int, int,
    int (*)(void *), void *, const char *);
static void bcm2835_icu_fdt_disestablish(device_t, void *);
static bool bcm2835_icu_fdt_intrstr(device_t, u_int *, char *, size_t);

static int bcm2835_icu_intr(void *);

static int bcm2836mp_icu_fdt_decode_irq(u_int *);
static void *bcm2836mp_icu_fdt_establish(device_t, u_int *, int, int,
    int (*)(void *), void *, const char *);
static void bcm2836mp_icu_fdt_disestablish(device_t, void *);
static bool bcm2836mp_icu_fdt_intrstr(device_t, u_int *, char *, size_t);

static int  bcm2835_icu_match(device_t, cfdata_t, void *);
static void bcm2835_icu_attach(device_t, device_t, void *);

static int bcm2835_int_base;
static int bcm2836mp_int_base[BCM2836_NCPUS];

#define	BCM2835_INT_BASE		bcm2835_int_base
#define	BCM2836_INT_BASECPUN(n)		bcm2836mp_int_base[(n)]

#define BCM2836_INT_CNTPSIRQ_CPUN(n)	(BCM2836_INT_BASECPUN(n) + BCM2836_INT_CNTPSIRQ)
#define BCM2836_INT_CNTPNSIRQ_CPUN(n)	(BCM2836_INT_BASECPUN(n) + BCM2836_INT_CNTPNSIRQ)
#define BCM2836_INT_CNTVIRQ_CPUN(n)	(BCM2836_INT_BASECPUN(n) + BCM2836_INT_CNTVIRQ)
#define BCM2836_INT_CNTHPIRQ_CPUN(n)	(BCM2836_INT_BASECPUN(n) + BCM2836_INT_CNTHPIRQ)
#define BCM2836_INT_MAILBOX0_CPUN(n)	(BCM2836_INT_BASECPUN(n) + BCM2836_INT_MAILBOX0)

/* Periperal Interrupt sources */
#define	BCM2835_NIRQ			96

#define BCM2835_INT_GPU0BASE		(BCM2835_INT_BASE + 0)
#define BCM2835_INT_TIMER0		(BCM2835_INT_GPU0BASE + 0)
#define BCM2835_INT_TIMER1		(BCM2835_INT_GPU0BASE + 1)
#define BCM2835_INT_TIMER2		(BCM2835_INT_GPU0BASE + 2)
#define BCM2835_INT_TIMER3		(BCM2835_INT_GPU0BASE + 3)
#define BCM2835_INT_USB			(BCM2835_INT_GPU0BASE + 9)
#define BCM2835_INT_DMA0		(BCM2835_INT_GPU0BASE + 16)
#define BCM2835_INT_DMA2		(BCM2835_INT_GPU0BASE + 18)
#define BCM2835_INT_DMA3		(BCM2835_INT_GPU0BASE + 19)
#define BCM2835_INT_AUX			(BCM2835_INT_GPU0BASE + 29)
#define BCM2835_INT_ARM			(BCM2835_INT_GPU0BASE + 30)

#define BCM2835_INT_GPU1BASE		(BCM2835_INT_BASE + 32)
#define BCM2835_INT_GPIO0		(BCM2835_INT_GPU1BASE + 17)
#define BCM2835_INT_GPIO1		(BCM2835_INT_GPU1BASE + 18)
#define BCM2835_INT_GPIO2		(BCM2835_INT_GPU1BASE + 19)
#define BCM2835_INT_GPIO3		(BCM2835_INT_GPU1BASE + 20)
#define BCM2835_INT_BSC			(BCM2835_INT_GPU1BASE + 21)
#define BCM2835_INT_SPI0		(BCM2835_INT_GPU1BASE + 22)
#define BCM2835_INT_PCM			(BCM2835_INT_GPU1BASE + 23)
#define BCM2835_INT_SDHOST		(BCM2835_INT_GPU1BASE + 24)
#define BCM2835_INT_UART0		(BCM2835_INT_GPU1BASE + 25)
#define BCM2835_INT_EMMC		(BCM2835_INT_GPU1BASE + 30)

#define BCM2835_INT_BASICBASE		(BCM2835_INT_BASE + 64)
#define BCM2835_INT_ARMTIMER		(BCM2835_INT_BASICBASE + 0)
#define BCM2835_INT_ARMMAILBOX		(BCM2835_INT_BASICBASE + 1)
#define BCM2835_INT_ARMDOORBELL0	(BCM2835_INT_BASICBASE + 2)
#define BCM2835_INT_ARMDOORBELL1	(BCM2835_INT_BASICBASE + 3)
#define BCM2835_INT_GPU0HALTED		(BCM2835_INT_BASICBASE + 4)
#define BCM2835_INT_GPU1HALTED		(BCM2835_INT_BASICBASE + 5)
#define BCM2835_INT_ILLEGALTYPE0	(BCM2835_INT_BASICBASE + 6)
#define BCM2835_INT_ILLEGALTYPE1	(BCM2835_INT_BASICBASE + 7)

static void
bcm2835_set_priority(struct pic_softc *pic, int ipl)
{
	curcpu()->ci_cpl = ipl;
}

static struct pic_ops bcm2835_picops = {
	.pic_unblock_irqs = bcm2835_pic_unblock_irqs,
	.pic_block_irqs = bcm2835_pic_block_irqs,
	.pic_find_pending_irqs = bcm2835_pic_find_pending_irqs,
	.pic_establish_irq = bcm2835_pic_establish_irq,
	.pic_source_name = bcm2835_pic_source_name,
	.pic_set_priority = bcm2835_set_priority,
};

static struct pic_softc bcm2835_pic = {
	.pic_ops = &bcm2835_picops,
	.pic_maxsources = BCM2835_NIRQ,
	.pic_name = "bcm2835 pic",
};

static struct pic_ops bcm2836mp_picops = {
	.pic_unblock_irqs = bcm2836mp_pic_unblock_irqs,
	.pic_block_irqs = bcm2836mp_pic_block_irqs,
	.pic_find_pending_irqs = bcm2836mp_pic_find_pending_irqs,
	.pic_establish_irq = bcm2836mp_pic_establish_irq,
	.pic_source_name = bcm2836mp_pic_source_name,
#if defined(MULTIPROCESSOR)
	.pic_cpu_init = bcm2836mp_cpu_init,
	.pic_ipi_send = bcm2836mp_send_ipi,
#endif
};

static struct pic_softc bcm2836mp_pic[BCM2836_NCPUS] = {
	[0 ... BCM2836_NCPUS - 1] = {
		.pic_ops = &bcm2836mp_picops,
		.pic_maxsources = BCM2836_NIRQPERCPU,
		.pic_name = "bcm2836 pic",
	}
};

static struct fdtbus_interrupt_controller_func bcm2835icu_fdt_funcs = {
	.establish = bcm2835_icu_fdt_establish,
	.disestablish = bcm2835_icu_fdt_disestablish,
	.intrstr = bcm2835_icu_fdt_intrstr
};

static struct fdtbus_interrupt_controller_func bcm2836mpicu_fdt_funcs = {
	.establish = bcm2836mp_icu_fdt_establish,
	.disestablish = bcm2836mp_icu_fdt_disestablish,
	.intrstr = bcm2836mp_icu_fdt_intrstr
};

struct bcm2836mp_interrupt {
	bool bi_done;
	TAILQ_ENTRY(bcm2836mp_interrupt) bi_next;
	int bi_irq;
	int bi_ipl;
	int bi_flags;
	int (*bi_func)(void *);
	void *bi_arg;
	void *bi_ihs[BCM2836_NCPUS];
};

static TAILQ_HEAD(, bcm2836mp_interrupt) bcm2836mp_interrupts =
    TAILQ_HEAD_INITIALIZER(bcm2836mp_interrupts);

struct bcm2835icu_irqhandler;
struct bcm2835icu_irq;
struct bcm2835icu_softc;

struct bcm2835icu_irqhandler {
	struct bcm2835icu_irq	*ih_irq;
	int			(*ih_fn)(void *);
	void			*ih_arg;
	TAILQ_ENTRY(bcm2835icu_irqhandler) ih_next;
};

struct bcm2835icu_irq {
	struct bcm2835icu_softc	*intr_sc;
	void			*intr_ih;
	void			*intr_arg;
	int			intr_refcnt;
	int			intr_ipl;
	int			intr_irq;
	int			intr_mpsafe;
	TAILQ_HEAD(, bcm2835icu_irqhandler) intr_handlers;
};

struct bcm2835icu_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	struct bcm2835icu_irq	*sc_irq[BCM2835_NIRQ];

	int sc_phandle;
};

static struct bcm2835icu_softc *bcml1icu_sc;
static struct bcm2835icu_softc *bcmicu_sc;

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
	"sdhost",	"uart",		"(unused 58)",	"(unused 59)",
	"(unused 60)",	"(unused 61)",	"emmc",		"(unused 63)",
	"Timer",	"Mailbox",	"Doorbell0",	"Doorbell1",
	"GPU0 Halted",	"GPU1 Halted",	"Illegal #1",	"Illegal #0"
};

static const char * const bcm2836mp_sources[BCM2836_NIRQPERCPU] = {
	"cntpsirq",	"cntpnsirq",	"cnthpirq",	"cntvirq",
	"mailbox0",	"mailbox1",	"mailbox2",	"mailbox3",
	"gpu",		"pmu"
};

#define	BCM2836_INTBIT_GPUPENDING	__BIT(8)

#define	BCM2835_INTBIT_PENDING1		__BIT(8)
#define	BCM2835_INTBIT_PENDING2		__BIT(9)
#define	BCM2835_INTBIT_ARM		__BITS(0,7)
#define	BCM2835_INTBIT_GPU0		__BITS(10,14)
#define	BCM2835_INTBIT_GPU1		__BITS(15,20)

CFATTACH_DECL_NEW(bcmicu, sizeof(struct bcm2835icu_softc),
    bcm2835_icu_match, bcm2835_icu_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "brcm,bcm2708-armctrl-ic",	.value = 0 },
	{ .compat = "brcm,bcm2709-armctrl-ic",	.value = 0 },
	{ .compat = "brcm,bcm2835-armctrl-ic",	.value = 0 },
	{ .compat = "brcm,bcm2836-armctrl-ic",	.value = 0 },
	{ .compat = "brcm,bcm2836-l1-intc",	.value = 1 },
	DEVICE_COMPAT_EOL
};

static int
bcm2835_icu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
bcm2835_icu_attach(device_t parent, device_t self, void *aux)
{
	struct bcm2835icu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	struct fdtbus_interrupt_controller_func *ifuncs;
	const struct device_compatible_entry *dce;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	bus_space_handle_t ioh;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;

	if (bus_space_map(sc->sc_iot, addr, size, 0, &ioh) != 0) {
		aprint_error(": couldn't map device\n");
		return;
	}

	sc->sc_ioh = ioh;
	sc->sc_phandle = phandle;

	dce = of_compatible_lookup(faa->faa_phandle, compat_data);
	KASSERT(dce != NULL);

	if (dce->value != 0) {
#if defined(MULTIPROCESSOR)
		aprint_normal(": Multiprocessor");
#endif
		bcml1icu_sc = sc;

		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    BCM2836_LOCAL_CONTROL, 0);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    BCM2836_LOCAL_PRESCALER, 0x80000000);

		ifuncs = &bcm2836mpicu_fdt_funcs;

#if defined(MULTIPROCESSOR)
		/*
		 * Register all PICs here in order to avoid pic_add() from
		 * cpu_hatch().  This is the only approved method.
		 */
		CPU_INFO_ITERATOR cii;
		struct cpu_info *ci;
		for (CPU_INFO_FOREACH(cii, ci)) {
			const cpuid_t cpuid = ci->ci_core_id;
			struct pic_softc * const pic = &bcm2836mp_pic[cpuid];

			KASSERT(cpuid < BCM2836_NCPUS);

			pic->pic_cpus = ci->ci_kcpuset;
			/*
			 * Append "#n" to avoid duplication of .pic_name[]
			 * It should be a unique id for intr_get_source()
			 */
			char suffix[sizeof("#00000")];
			snprintf(suffix, sizeof(suffix), "#%lu", cpuid);
			strlcat(pic->pic_name, suffix, sizeof(pic->pic_name));

			bcm2836mp_int_base[cpuid] =
			    pic_add(pic, PIC_IRQBASE_ALLOC);
			bcm2836mp_intr_init(ci);
		}
#endif
	} else {
		if (bcml1icu_sc == NULL)
			arm_fdt_irq_set_handler(bcm2835_irq_handler);
		bcmicu_sc = sc;
		sc->sc_ioh = ioh;
		sc->sc_phandle = phandle;
		bcm2835_int_base = pic_add(&bcm2835_pic, PIC_IRQBASE_ALLOC);
		ifuncs = &bcm2835icu_fdt_funcs;
	}

	error = fdtbus_register_interrupt_controller(self, phandle, ifuncs);
	if (error != 0) {
		aprint_error(": couldn't register with fdtbus: %d\n", error);
		return;
	}
	aprint_normal("\n");
}

static void
bcm2835_irq_handler(void *frame)
{
	struct cpu_info * const ci = curcpu();
	const int oldipl = ci->ci_cpl;
	const cpuid_t cpuid = ci->ci_core_id;
	const uint32_t oldipl_mask = __BIT(oldipl);
	int ipl_mask = 0;

	KASSERT(cpuid < BCM2836_NCPUS);

	ci->ci_data.cpu_nintr++;

	bcm2835_barrier();
	if (cpuid == 0) {
		ipl_mask = bcm2835_pic_find_pending_irqs(&bcm2835_pic);
	}
#if defined(SOC_BCM2836)
	ipl_mask |= bcm2836mp_pic_find_pending_irqs(&bcm2836mp_pic[cpuid]);
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
		ipl |= pic_mark_pending_sources(pic,
		    BCM2835_INT_BASICBASE - BCM2835_INT_BASE, armirq);
	}

	if (gpu0irq || (bpending & BCM2835_INTBIT_PENDING1)) {
		uint32_t pending1;

		pending1 = read_bcm2835reg(BCM2835_INTC_IRQ1PENDING);
		ipl |= pic_mark_pending_sources(pic,
		    BCM2835_INT_GPU0BASE - BCM2835_INT_BASE, pending1);
	}
	if (gpu1irq || (bpending & BCM2835_INTBIT_PENDING2)) {
		uint32_t pending2;

		pending2 = read_bcm2835reg(BCM2835_INTC_IRQ2PENDING);
		ipl |= pic_mark_pending_sources(pic,
		    BCM2835_INT_GPU1BASE - BCM2835_INT_BASE, pending2);
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

static int
bcm2835_icu_fdt_decode_irq(u_int *specifier)
{
	u_int base;

	if (!specifier)
		return -1;

	/* 1st cell is the bank number. 0 = ARM, 1 = GPU0, 2 = GPU1 */
	/* 2nd cell is the irq relative to that bank */

	const u_int bank = be32toh(specifier[0]);
	switch (bank) {
	case 0:
		base = BCM2835_INT_BASICBASE;
		break;
	case 1:
		base = BCM2835_INT_GPU0BASE;
		break;
	case 2:
		base = BCM2835_INT_GPU1BASE;
		break;
	default:
		return -1;
	}
	const u_int off = be32toh(specifier[1]);

	return base + off;
}

static void *
bcm2835_icu_fdt_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	struct bcm2835icu_softc * const sc = device_private(dev);
	struct bcm2835icu_irq *firq;
	struct bcm2835icu_irqhandler *firqh;
	int iflags = (flags & FDT_INTR_MPSAFE) ? IST_MPSAFE : 0;
	int irq, irqidx;

	irq = bcm2835_icu_fdt_decode_irq(specifier);
	if (irq == -1)
		return NULL;
	irqidx = irq - BCM2835_INT_BASE;

	KASSERT(irqidx < BCM2835_NIRQ);

	firq = sc->sc_irq[irqidx];
	if (firq == NULL) {
		firq = kmem_alloc(sizeof(*firq), KM_SLEEP);
		firq->intr_sc = sc;
		firq->intr_refcnt = 0;
		firq->intr_arg = arg;
		firq->intr_ipl = ipl;
		firq->intr_mpsafe = iflags;
		firq->intr_irq = irq;
		TAILQ_INIT(&firq->intr_handlers);
		if (arg == NULL) {
			firq->intr_ih = intr_establish_xname(irq, ipl,
			    IST_LEVEL | iflags, func, NULL, xname);
		} else {
			firq->intr_ih = intr_establish_xname(irq, ipl,
			    IST_LEVEL | iflags, bcm2835_icu_intr, firq, xname);
		}
		if (firq->intr_ih == NULL) {
			kmem_free(firq, sizeof(*firq));
			return NULL;
		}
		sc->sc_irq[irqidx] = firq;
	} else {
		if (firq->intr_arg == NULL || arg == NULL) {
			device_printf(dev,
			    "cannot share irq with NULL-arg handler\n");
			return NULL;
		}
		if (firq->intr_ipl != ipl) {
			device_printf(dev,
			    "cannot share irq with different ipl\n");
			return NULL;
		}
		if (firq->intr_mpsafe != iflags) {
			device_printf(dev,
			    "cannot share irq between mpsafe/non-mpsafe\n");
			return NULL;
		}
	}

	firqh = kmem_alloc(sizeof(*firqh), KM_SLEEP);
	firqh->ih_irq = firq;
	firqh->ih_fn = func;
	firqh->ih_arg = arg;

	firq->intr_refcnt++;
	TAILQ_INSERT_TAIL(&firq->intr_handlers, firqh, ih_next);

	/*
	 * XXX interrupt_distribute(9) assumes that any interrupt
	 * handle can be used as an input to the MD interrupt_distribute
	 * implementationm, so we are forced to return the handle
	 * we got back from intr_establish().  Upshot is that the
	 * input to bcm2835_icu_fdt_disestablish() is ambiguous for
	 * shared IRQs, rendering them un-disestablishable.
	 */

	return firq->intr_ih;
}

static void
bcm2835_icu_fdt_disestablish(device_t dev, void *ih)
{
	struct bcm2835icu_softc * const sc = device_private(dev);
	struct bcm2835icu_irqhandler *firqh;
	struct bcm2835icu_irq *firq;
	u_int n;

	for (n = 0; n < BCM2835_NIRQ; n++) {
		firq = sc->sc_irq[n];
		if (firq == NULL || firq->intr_ih != ih)
			continue;

		KASSERT(firq->intr_refcnt > 0);
		KASSERT(n == (firq->intr_irq - BCM2835_INT_BASE));

		/* XXX see above */
		if (firq->intr_refcnt > 1)
			panic("%s: cannot disestablish shared irq", __func__);

		intr_disestablish(firq->intr_ih);

		firqh = TAILQ_FIRST(&firq->intr_handlers);
		TAILQ_REMOVE(&firq->intr_handlers, firqh, ih_next);
		kmem_free(firqh, sizeof(*firqh));

		sc->sc_irq[n] = NULL;
		kmem_free(firq, sizeof(*firq));

		return;
	}

	panic("%s: interrupt not established", __func__);
}

static int
bcm2835_icu_intr(void *priv)
{
	struct bcm2835icu_irq *firq = priv;
	struct bcm2835icu_irqhandler *firqh;
	int handled = 0;

	TAILQ_FOREACH(firqh, &firq->intr_handlers, ih_next) {
		handled |= firqh->ih_fn(firqh->ih_arg);
	}

	return handled;
}

static bool
bcm2835_icu_fdt_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{
	int irq;

	irq = bcm2835_icu_fdt_decode_irq(specifier);
	if (irq == -1)
		return false;

	snprintf(buf, buflen, "icu irq %d", irq);

	return true;
}

#define	BCM2836MP_TIMER_IRQS	__BITS(3,0)
#define	BCM2836MP_MAILBOX_IRQS	__BITS(4,7)
#define	BCM2836MP_GPU_IRQ	__BIT(8)
#define	BCM2836MP_PMU_IRQ	__BIT(9)
#define	BCM2836MP_ALL_IRQS	(BCM2836MP_TIMER_IRQS | BCM2836MP_MAILBOX_IRQS | BCM2836MP_GPU_IRQ | BCM2836MP_PMU_IRQ)

static void
bcm2836mp_pic_unblock_irqs(struct pic_softc *pic, size_t irqbase,
    uint32_t irq_mask)
{
	const bus_space_tag_t iot = bcml1icu_sc->sc_iot;
	const bus_space_handle_t ioh = bcml1icu_sc->sc_ioh;
	const cpuid_t cpuid = pic - &bcm2836mp_pic[0];

	KASSERT(cpuid < BCM2836_NCPUS);
	KASSERT(irqbase == 0);

	if (irq_mask & BCM2836MP_TIMER_IRQS) {
		uint32_t mask = __SHIFTOUT(irq_mask, BCM2836MP_TIMER_IRQS);
		uint32_t val = bus_space_read_4(iot, ioh,
		    BCM2836_LOCAL_TIMER_IRQ_CONTROLN(cpuid));
		val |= mask;
		bus_space_write_4(iot, ioh,
		    BCM2836_LOCAL_TIMER_IRQ_CONTROLN(cpuid),
		    val);
		bus_space_barrier(iot, ioh,
		    BCM2836_LOCAL_TIMER_IRQ_CONTROL_BASE,
		    BCM2836_LOCAL_TIMER_IRQ_CONTROL_SIZE,
		    BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE);
	}
	if (irq_mask & BCM2836MP_MAILBOX_IRQS) {
		uint32_t mask = __SHIFTOUT(irq_mask, BCM2836MP_MAILBOX_IRQS);
		uint32_t val = bus_space_read_4(iot, ioh,
		    BCM2836_LOCAL_MAILBOX_IRQ_CONTROLN(cpuid));
		val |= mask;
		bus_space_write_4(iot, ioh,
		    BCM2836_LOCAL_MAILBOX_IRQ_CONTROLN(cpuid),
		    val);
		bus_space_barrier(iot, ioh,
		    BCM2836_LOCAL_MAILBOX_IRQ_CONTROL_BASE,
		    BCM2836_LOCAL_MAILBOX_IRQ_CONTROL_SIZE,
		    BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE);
	}
	if (irq_mask & BCM2836MP_PMU_IRQ) {
		bus_space_write_4(iot, ioh, BCM2836_LOCAL_PM_ROUTING_SET,
		    __BIT(cpuid));
		bus_space_barrier(iot, ioh, BCM2836_LOCAL_PM_ROUTING_SET, 4,
		    BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE);
	}

	return;
}

static void
bcm2836mp_pic_block_irqs(struct pic_softc *pic, size_t irqbase,
    uint32_t irq_mask)
{
	const bus_space_tag_t iot = bcml1icu_sc->sc_iot;
	const bus_space_handle_t ioh = bcml1icu_sc->sc_ioh;
	const cpuid_t cpuid = pic - &bcm2836mp_pic[0];

	KASSERT(cpuid < BCM2836_NCPUS);
	KASSERT(irqbase == 0);

	if (irq_mask & BCM2836MP_TIMER_IRQS) {
		uint32_t mask = __SHIFTOUT(irq_mask, BCM2836MP_TIMER_IRQS);
		uint32_t val = bus_space_read_4(iot, ioh,
		    BCM2836_LOCAL_TIMER_IRQ_CONTROLN(cpuid));
		val &= ~mask;
		bus_space_write_4(iot, ioh,
		    BCM2836_LOCAL_TIMER_IRQ_CONTROLN(cpuid),
		    val);
	}
	if (irq_mask & BCM2836MP_MAILBOX_IRQS) {
		uint32_t mask = __SHIFTOUT(irq_mask, BCM2836MP_MAILBOX_IRQS);
		uint32_t val = bus_space_read_4(iot, ioh,
		    BCM2836_LOCAL_MAILBOX_IRQ_CONTROLN(cpuid));
		val &= ~mask;
		bus_space_write_4(iot, ioh,
		    BCM2836_LOCAL_MAILBOX_IRQ_CONTROLN(cpuid),
		    val);
	}
	if (irq_mask & BCM2836MP_PMU_IRQ) {
		bus_space_write_4(iot, ioh, BCM2836_LOCAL_PM_ROUTING_CLR,
		    __BIT(cpuid));
	}

	bcm2835_barrier();
	return;
}

static int
bcm2836mp_pic_find_pending_irqs(struct pic_softc *pic)
{
	struct cpu_info * const ci = curcpu();
	const cpuid_t cpuid = ci->ci_core_id;
	uint32_t lpending;
	int ipl = 0;

	KASSERT(cpuid < BCM2836_NCPUS);
	KASSERT(pic == &bcm2836mp_pic[cpuid]);

	bcm2835_barrier();

	lpending = bus_space_read_4(bcml1icu_sc->sc_iot, bcml1icu_sc->sc_ioh,
	    BCM2836_LOCAL_INTC_IRQPENDINGN(cpuid));

	lpending &= ~BCM2836_INTBIT_GPUPENDING;
	const uint32_t allirqs = lpending & BCM2836MP_ALL_IRQS;
	if (allirqs) {
		ipl |= pic_mark_pending_sources(pic, 0, allirqs);
	}

	return ipl;
}

static void
bcm2836mp_pic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	/* Nothing really*/
	KASSERT(is->is_irq >= 0);
	KASSERT(is->is_irq < BCM2836_NIRQPERCPU);
}

static void
bcm2836mp_pic_source_name(struct pic_softc *pic, int irq, char *buf, size_t len)
{

	irq %= BCM2836_NIRQPERCPU;
	strlcpy(buf, bcm2836mp_sources[irq], len);
}


#if defined(MULTIPROCESSOR)
static void bcm2836mp_cpu_init(struct pic_softc *pic, struct cpu_info *ci)
{
	const cpuid_t cpuid = ci->ci_core_id;

	KASSERT(cpuid < BCM2836_NCPUS);

	/* Enable IRQ and not FIQ */
	bus_space_write_4(bcml1icu_sc->sc_iot, bcml1icu_sc->sc_ioh,
	    BCM2836_LOCAL_MAILBOX_IRQ_CONTROLN(cpuid), 1);
}

static void
bcm2836mp_send_ipi(struct pic_softc *pic, const kcpuset_t *kcp, u_long ipi)
{
	KASSERT(pic != NULL);
	KASSERT(pic != &bcm2835_pic);
	KASSERT(pic->pic_cpus != NULL);

	const cpuid_t cpuid = pic - &bcm2836mp_pic[0];
	KASSERT(cpuid < BCM2836_NCPUS);

	bus_space_write_4(bcml1icu_sc->sc_iot, bcml1icu_sc->sc_ioh,
	    BCM2836_LOCAL_MAILBOX0_SETN(cpuid), __BIT(ipi));
}

int
bcm2836mp_ipi_handler(void *priv)
{
	const struct cpu_info *ci = priv;
	const cpuid_t cpuid = ci->ci_core_id;
	uint32_t ipimask, bit;

	KASSERT(cpuid < BCM2836_NCPUS);

	ipimask = bus_space_read_4(bcml1icu_sc->sc_iot, bcml1icu_sc->sc_ioh,
	    BCM2836_LOCAL_MAILBOX0_CLRN(cpuid));
	bus_space_write_4(bcml1icu_sc->sc_iot, bcml1icu_sc->sc_ioh,
	    BCM2836_LOCAL_MAILBOX0_CLRN(cpuid), ipimask);

	while ((bit = ffs(ipimask)) > 0) {
		const u_int ipi = bit - 1;
		switch (ipi) {
		case IPI_AST:
			pic_ipi_ast(priv);
			break;
		case IPI_NOP:
			pic_ipi_nop(priv);
			break;
#ifdef __HAVE_PREEMPTION
		case IPI_KPREEMPT:
			pic_ipi_kpreempt(priv);
			break;
#endif
		case IPI_XCALL:
			pic_ipi_xcall(priv);
			break;
		case IPI_GENERIC:
			pic_ipi_generic(priv);
			break;
		case IPI_SHOOTDOWN:
			pic_ipi_shootdown(priv);
			break;
#ifdef DDB
		case IPI_DDB:
			pic_ipi_ddb(priv);
			break;
#endif
		}
		ipimask &= ~__BIT(ipi);
	}

	return 1;
}
#endif

#if defined(MULTIPROCESSOR)
static void
bcm2836mp_intr_init(struct cpu_info *ci)
{
	const cpuid_t cpuid = ci->ci_core_id;

	KASSERT(cpuid < BCM2836_NCPUS);

	intr_establish(BCM2836_INT_MAILBOX0_CPUN(cpuid), IPL_HIGH,
	    IST_LEVEL | IST_MPSAFE, bcm2836mp_ipi_handler, ci);
}
#endif

static int
bcm2836mp_icu_fdt_decode_irq(u_int *specifier)
{

	if (!specifier)
		return -1;
	return be32toh(specifier[0]);
}

static void *
bcm2836mp_icu_fdt_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	int iflags = (flags & FDT_INTR_MPSAFE) ? IST_MPSAFE : 0;

	int irq = bcm2836mp_icu_fdt_decode_irq(specifier);
	if (irq == -1)
		return NULL;

	void *ihs[BCM2836_NCPUS];
	for (cpuid_t cpuid = 0; cpuid < BCM2836_NCPUS; cpuid++) {
		const int cpuirq = BCM2836_INT_BASECPUN(cpuid) + irq;
		ihs[cpuid] = intr_establish_xname(cpuirq, ipl,
		    IST_LEVEL | iflags, func, arg, xname);
		if (!ihs[cpuid]) {
			for (cpuid_t undo = 0; undo < cpuid; undo++) {
				intr_disestablish(ihs[undo]);
			}
			return NULL;
		}

	}

	/*
	 * Return the intr_establish handle for cpu 0 for API compatibility.
	 * Any cpu would do here as these sources don't support set_affinity
	 * when the handle is used in interrupt_distribute(9)
	 */
	return ihs[0];
}

static void
bcm2836mp_icu_fdt_disestablish(device_t dev, void *ih)
{
	intr_disestablish(ih);
}

static bool
bcm2836mp_icu_fdt_intrstr(device_t dev, u_int *specifier, char *buf,
    size_t buflen)
{
	int irq;

	irq = bcm2836mp_icu_fdt_decode_irq(specifier);
	if (irq == -1)
		return false;

	snprintf(buf, buflen, "local_intc irq %d", irq);

	return true;
}

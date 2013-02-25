/* $Id: imx23_icoll.c,v 1.2.6.2 2013/02/25 00:28:27 tls Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/systm.h>

#define _INTR_PRIVATE
#include <arm/pic/picvar.h>

#include <arm/imx/imx23_icollreg.h>
#include <arm/imx/imx23var.h>

#define ICOLL_SOFT_RST_LOOP 455		/* At least 1 us ... */
#define ICOLL_READ(sc, reg)						\
	bus_space_read_4(sc->sc_iot, sc->sc_hdl, (reg))
#define ICOLL_WRITE(sc, reg, val)					\
	bus_space_write_4(sc->sc_iot, sc->sc_hdl, (reg), (val))

#define ICOLL_IRQ_REG_SIZE 0x10
#define ICOLL_CLR_IRQ(sc, irq)						\
	ICOLL_WRITE(sc, HW_ICOLL_INTERRUPT0_CLR +			\
			(irq) * ICOLL_IRQ_REG_SIZE,			\
			HW_ICOLL_INTERRUPT_ENABLE)
#define ICOLL_SET_IRQ(sc, irq)						\
	ICOLL_WRITE(sc, HW_ICOLL_INTERRUPT0_SET +			\
			(irq) * ICOLL_IRQ_REG_SIZE,			\
			HW_ICOLL_INTERRUPT_ENABLE)
#define ICOLL_GET_PRIO(sc, irq)						\
	__SHIFTOUT(ICOLL_READ(sc, HW_ICOLL_INTERRUPT0 +			\
			(irq) * ICOLL_IRQ_REG_SIZE),			\
			HW_ICOLL_INTERRUPT_PRIORITY)

#define PICTOSOFTC(pic)							\
	((struct icoll_softc *)((char *)(pic) -				\
		offsetof(struct icoll_softc, sc_pic)))

/*
 * pic callbacks.
 */
static void	icoll_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void	icoll_block_irqs(struct pic_softc *, size_t, uint32_t);
static int	icoll_find_pending_irqs(struct pic_softc *);
static void	icoll_establish_irq(struct pic_softc *, struct intrsource *);
static void	icoll_source_name(struct pic_softc *, int, char *, size_t);
static void	icoll_set_priority(struct pic_softc *, int);

/*
 * autoconf(9) callbacks.
 */
static int	icoll_match(device_t, cfdata_t, void *);
static void	icoll_attach(device_t, device_t, void *);
static int	icoll_activate(device_t, enum devact);

/*
 * ARM interrupt handler.
 */
void imx23_intr_dispatch(struct clockframe *);

const static struct pic_ops icoll_pic_ops = {
	.pic_unblock_irqs = icoll_unblock_irqs,
	.pic_block_irqs = icoll_block_irqs,
	.pic_find_pending_irqs = icoll_find_pending_irqs,
	.pic_establish_irq = icoll_establish_irq,
	.pic_source_name = icoll_source_name,
	.pic_set_priority = icoll_set_priority
};

struct icoll_softc {
	device_t sc_dev;
	struct pic_softc sc_pic;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_hdl;
};

/* For IRQ handler. */
static struct icoll_softc *icoll_sc;

/*
 * Private to driver.
 */
static void	icoll_reset(struct icoll_softc *);

CFATTACH_DECL3_NEW(icoll,
	sizeof(struct icoll_softc),
	icoll_match,
	icoll_attach,
	NULL,
	icoll_activate,
	NULL,
	NULL,
	0);

/*
 * ARM interrupt handler.
 */
void
imx23_intr_dispatch(struct clockframe *frame)
{
	struct cpu_info * const ci = curcpu();
	struct pic_softc *pic_sc;
	int saved_spl;
	uint8_t irq;
	uint8_t prio;

	pic_sc = &icoll_sc->sc_pic;

	ci->ci_data.cpu_nintr++;

	/* Save current spl. */
	saved_spl = curcpl();

	/* IRQ to be handled. */
	irq = __SHIFTOUT(ICOLL_READ(icoll_sc, HW_ICOLL_STAT),
	    HW_ICOLL_STAT_VECTOR_NUMBER);

	/* Save IRQ's priority. Acknowledge it later. */
	prio = ICOLL_GET_PRIO(icoll_sc, irq);

	/*
	 * Notify ICOLL to deassert IRQ before re-enabling the IRQ's.
	 * This is done by writing anything to HW_ICOLL_VECTOR.
	 */
	ICOLL_WRITE(icoll_sc, HW_ICOLL_VECTOR,
	    __SHIFTIN(0x3fffffff, HW_ICOLL_VECTOR_IRQVECTOR));

	/* Bogus IRQ. */
	if (irq == 0x7f) {
		cpsie(I32_bit);
		ICOLL_WRITE(icoll_sc, HW_ICOLL_LEVELACK, (1<<prio));
		cpsid(I32_bit);
		return;
	}

	/* Raise the spl to the level of the IRQ. */
	if (pic_sc->pic_sources[irq]->is_ipl > ci->ci_cpl)
		saved_spl = _splraise(pic_sc->pic_sources[irq]->is_ipl);

	/* Call the handler registered for the IRQ. */
	cpsie(I32_bit);
	pic_dispatch(pic_sc->pic_sources[irq], frame);

	/*
	 * Acknowledge the IRQ by writing its priority to HW_ICOLL_LEVELACK.
	 * Interrupts should be enabled.
	 */
	ICOLL_WRITE(icoll_sc, HW_ICOLL_LEVELACK, (1<<prio));
	cpsid(I32_bit);

	/* Restore the saved spl. */
	splx(saved_spl);

	return;
}

/*
 * pic callbacks.
 */
static void
icoll_unblock_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct icoll_softc *sc = PICTOSOFTC(pic);
	uint8_t b;

	for (;;) {
		b = ffs(irq_mask);
		if (b == 0) break;
		b--;	/* Zero based index. */
		ICOLL_SET_IRQ(sc, irq_base + b);
		irq_mask &= ~(1<<b);
	}
		
	return;
}

static void
icoll_block_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct icoll_softc *sc = PICTOSOFTC(pic);
	uint8_t b;

	for (;;) {
		b = ffs(irq_mask);
		if (b == 0) break;
		b--;	/* Zero based index. */
		ICOLL_CLR_IRQ(sc, irq_base + b);
		irq_mask &= ~(1<<b);
	}

	return;
}

static int
icoll_find_pending_irqs(struct pic_softc *pic)
{
	return 0; /* ICOLL HW doesn't provide list of pending interrupts. */
}

static void
icoll_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	return; /* Nothing to establish. */
}

static void
icoll_source_name(struct pic_softc *pic, int irq, char *is_source, size_t size)
{
	snprintf(is_source, size, "irq %d", irq);
}

/*
 * Set new interrupt priority level by enabling or disabling IRQ's.
 */
static void
icoll_set_priority(struct pic_softc *pic, int newipl)
{
	struct icoll_softc *sc = PICTOSOFTC(pic);
	struct intrsource *is;
	int i;

	for (i = 0; i < pic->pic_maxsources; i++) {
		is = pic->pic_sources[i];
		if (is == NULL)
			continue;
		if (is->is_ipl > newipl)
			ICOLL_SET_IRQ(sc, pic->pic_irqbase + is->is_irq);
		else
			ICOLL_CLR_IRQ(sc, pic->pic_irqbase + is->is_irq);
	}
}

/*
 * autoconf(9) callbacks.
 */
static int
icoll_match(device_t parent, cfdata_t match, void *aux)
{
	struct apb_attach_args *aa = aux;

	if ((aa->aa_addr == HW_ICOLL_BASE) && (aa->aa_size == HW_ICOLL_SIZE))
		return 1;

	return 0;
}

static void
icoll_attach(device_t parent, device_t self, void *aux)
{
	static int icoll_attached = 0;
	struct icoll_softc *sc = device_private(self);
	struct apb_attach_args *aa = aux;
	
	if (icoll_attached)
		return;

	icoll_sc = sc;

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;

	sc->sc_pic.pic_maxsources = IRQ_LAST + 1;
	sc->sc_pic.pic_ops = &icoll_pic_ops;
	strlcpy(sc->sc_pic.pic_name, device_xname(self),
	    sizeof(sc->sc_pic.pic_name));

	if (bus_space_map(sc->sc_iot,
	    aa->aa_addr, aa->aa_size, 0, &(sc->sc_hdl))) {
		aprint_error_dev(sc->sc_dev, "unable to map bus space\n");
		return;
	}

	icoll_reset(sc);
	pic_add(&sc->sc_pic, 0);
	aprint_normal("\n");
	icoll_attached = 1;

	return;
}

static int
icoll_activate(device_t self, enum devact act)
{
	return EOPNOTSUPP;
}

/*
 * Reset the ICOLL block.
 *
 * Inspired by i.MX23 RM "39.3.10 Correct Way to Soft Reset a Block"
 */
static void
icoll_reset(struct icoll_softc *sc)
{
	unsigned int loop;

	/*
	 * Prepare for soft-reset by making sure that SFTRST is not currently
	 * asserted. Also clear CLKGATE so we can wait for its assertion below.
	 */
	ICOLL_WRITE(sc, HW_ICOLL_CTRL_CLR, HW_ICOLL_CTRL_SFTRST);

	/* Wait at least a microsecond for SFTRST to deassert. */
	loop = 0;
	while ((ICOLL_READ(sc, HW_ICOLL_CTRL) & HW_ICOLL_CTRL_SFTRST) ||
	    (loop < ICOLL_SOFT_RST_LOOP)) {
		loop++;
	}

	/* Clear CLKGATE so we can wait for its assertion below. */
	ICOLL_WRITE(sc, HW_ICOLL_CTRL_CLR, HW_ICOLL_CTRL_CLKGATE);

	/* Soft-reset the block. */
	ICOLL_WRITE(sc, HW_ICOLL_CTRL_SET, HW_ICOLL_CTRL_SFTRST);

	/* Wait until clock is in the gated state. */
	while (!(ICOLL_READ(sc, HW_ICOLL_CTRL) & HW_ICOLL_CTRL_CLKGATE));

	/* Bring block out of reset. */
	ICOLL_WRITE(sc, HW_ICOLL_CTRL_CLR, HW_ICOLL_CTRL_SFTRST);

	loop = 0;
	while ((ICOLL_READ(sc, HW_ICOLL_CTRL) & HW_ICOLL_CTRL_SFTRST) ||
	    (loop < ICOLL_SOFT_RST_LOOP)) {
		loop++;
	}

	ICOLL_WRITE(sc, HW_ICOLL_CTRL_CLR, HW_ICOLL_CTRL_CLKGATE);
	
	/* Wait until clock is in the NON-gated state. */
	while (ICOLL_READ(sc, HW_ICOLL_CTRL) & HW_ICOLL_CTRL_CLKGATE);

	return;
}

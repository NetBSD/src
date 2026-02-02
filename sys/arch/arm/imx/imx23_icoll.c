/* $NetBSD: imx23_icoll.c,v 1.8 2026/02/02 06:23:37 skrll Exp $ */

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

#define _INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx23_icoll.c,v 1.8 2026/02/02 06:23:37 skrll Exp $");

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/cpufunc.h>

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
	((struct imx23_icoll_softc *)((char *)(pic) -			\
		offsetof(struct imx23_icoll_softc, sc_pic)))

struct imx23_icoll_softc {
	struct pic_softc sc_pic;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_hdl;
};

/*
 * pic callbacks.
 */
static void	imx23_icoll_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void	imx23_icoll_block_irqs(struct pic_softc *, size_t, uint32_t);
static int	imx23_icoll_find_pending_irqs(struct pic_softc *);
static void	imx23_icoll_establish_irq(struct pic_softc *,
			  		  struct intrsource *);
static void	imx23_icoll_source_name(struct pic_softc *, int, char *,
					size_t);
static void	imx23_icoll_set_priority(struct pic_softc *, int);

/*
 * autoconf(9) callbacks.
 */
static int	imx23_icoll_match(device_t, cfdata_t, void *);
static void	imx23_icoll_attach(device_t, device_t, void *);

/*
 * fdt callbacks
 */
static void *	imx23_icoll_fdt_establish(device_t, u_int *, int, int,
			 int (*)(void *), void *, const char *);
static void	imx23_icoll_fdt_disestablish(device_t, void *);
static bool	imx23_icoll_fdt_intrstr(device_t, u_int *, char *, size_t);
void 		imx23_icoll_intr_dispatch(struct clockframe *);

const static struct pic_ops imx23_icoll_pic_ops = {
	.pic_unblock_irqs = imx23_icoll_unblock_irqs,
	.pic_block_irqs = imx23_icoll_block_irqs,
	.pic_find_pending_irqs = imx23_icoll_find_pending_irqs,
	.pic_establish_irq = imx23_icoll_establish_irq,
	.pic_source_name = imx23_icoll_source_name,
	.pic_set_priority = imx23_icoll_set_priority
};

struct fdtbus_interrupt_controller_func imx23_icoll_fdt_funcs = {
	.establish = imx23_icoll_fdt_establish,
	.disestablish = imx23_icoll_fdt_disestablish,
	.intrstr = imx23_icoll_fdt_intrstr
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx23-icoll" },
	{ .compat = "fsl,icoll" },
	DEVICE_COMPAT_EOL
};

/* For IRQ handler. */
static struct imx23_icoll_softc *icoll_sc;

/*
 * Private to driver.
 */
static void	imx23_icoll_reset(struct imx23_icoll_softc *);

CFATTACH_DECL_NEW(imx23icoll, sizeof(struct imx23_icoll_softc),
		  imx23_icoll_match, imx23_icoll_attach, NULL, NULL);

/*
 * ARM interrupt handler.
 */
void
imx23_icoll_intr_dispatch(struct clockframe *frame)
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
imx23_icoll_unblock_irqs(struct pic_softc *pic, size_t irq_base,
			 uint32_t irq_mask)
{
	struct imx23_icoll_softc *sc = PICTOSOFTC(pic);
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
imx23_icoll_block_irqs(struct pic_softc *pic, size_t irqbase, uint32_t irq_mask)
{
	struct imx23_icoll_softc *sc = PICTOSOFTC(pic);
	uint8_t b;

	for (;;) {
		b = ffs(irq_mask);
		if (b == 0) break;
		b--;	/* Zero based index. */
		ICOLL_CLR_IRQ(sc, irqbase + b);
		irq_mask &= ~(1<<b);
	}

	return;
}

static int
imx23_icoll_find_pending_irqs(struct pic_softc *pic)
{
	return 0; /* ICOLL HW doesn't provide list of pending interrupts. */
}

static void
imx23_icoll_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	return; /* Nothing to establish. */
}

static void
imx23_icoll_source_name(struct pic_softc *pic, int irq, char *is_source,
			size_t size)
{
	snprintf(is_source, size, "irq %d", irq);
}

/*
 * Set new interrupt priority level by enabling or disabling IRQ's.
 */
static void
imx23_icoll_set_priority(struct pic_softc *pic, int newipl)
{
	struct imx23_icoll_softc *sc = PICTOSOFTC(pic);
	struct intrsource *is;
	int i;

	register_t psw = DISABLE_INTERRUPT_SAVE();

	for (i = 0; i < pic->pic_maxsources; i++) {
		is = pic->pic_sources[i];
		if (is == NULL)
			continue;
		if (is->is_ipl > newipl)
			ICOLL_SET_IRQ(sc, pic->pic_irqbase + is->is_irq);
		else
			ICOLL_CLR_IRQ(sc, pic->pic_irqbase + is->is_irq);
	}

	curcpu()->ci_cpl = newipl;

	if ((psw & I32_bit) == 0) {
		ENABLE_INTERRUPT();
	}
}

static bool
imx23_icoll_fdt_intrstr(device_t dev, u_int *specifier, char *buf, size_t bufln)
{
	const u_int irq = be32toh(*specifier);

	snprintf(buf, bufln, "icoll irq %d", irq);

	return true;
}

static void *
imx23_icoll_fdt_establish(device_t dev, u_int *specifier, int ipl, int flags,
			 int (*func)(void *), void *arg, const char *xname)
{
	const u_int irq = be32toh(*specifier);
	const u_int mpsafe = (flags & FDT_INTR_MPSAFE) ? IST_MPSAFE : 0;

	return intr_establish_xname(irq, ipl, IST_LEVEL | mpsafe, func, arg,
				    xname);
}

static void
imx23_icoll_fdt_disestablish(device_t dev, void *ih)
{
	intr_disestablish(ih);
}

/*
 * autoconf(9) callbacks.
 */
static int
imx23_icoll_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx23_icoll_attach(device_t parent, device_t self, void *aux)
{
	struct imx23_icoll_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	icoll_sc = sc;
	sc->sc_iot = faa->faa_bst;
	sc->sc_pic.pic_maxsources = IRQ_LAST + 1;
	sc->sc_pic.pic_ops = &imx23_icoll_pic_ops;
	strlcpy(sc->sc_pic.pic_name, device_xname(self),
		sizeof(sc->sc_pic.pic_name));

	bus_addr_t addr;
	bus_size_t size;
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get register address\n");
		return;
	}
	if (bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_hdl)) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	imx23_icoll_reset(sc);
	pic_add(&sc->sc_pic, 0);

	int error = fdtbus_register_interrupt_controller(self, phandle,
						&imx23_icoll_fdt_funcs);
	if (error) {
		aprint_error(
		    "imx23icoll_fdt: couldn't register with fdtbus: %d\n",
		    error);
		return;
	}

	arm_fdt_irq_set_handler((void (*)(void *))imx23_icoll_intr_dispatch);

	aprint_normal("\n");
}

/*
 * Reset the ICOLL block.
 *
 * Inspired by i.MX23 RM "39.3.10 Correct Way to Soft Reset a Block"
 */
static void
imx23_icoll_reset(struct imx23_icoll_softc *sc)
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

/* $NetBSD $ */

/*-
* Copyright (c) 2025 The NetBSD Foundation, Inc.
* All rights reserved.
*
* This code is derived from software contributed to The NetBSD Foundation
* by Yuri Honegger.
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

/*
 * Interrupt controller for the TI AM18XX SOC
 */

#define _INTR_PRIVATE

#include <sys/cdefs.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

#include <arm/armreg.h>
#include <arm/pic/picvar.h>

struct am18xx_intc_softc {
	struct pic_softc sc_pic;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	int sc_num_irqs;
};

static int 	am18xx_intc_match(device_t, cfdata_t, void *);
static void 	am18xx_intc_attach(device_t, device_t, void *);

static void	am18xx_intc_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void	am18xx_intc_block_irqs(struct pic_softc *, size_t, uint32_t);
static void	am18xx_intc_establish_irq(struct pic_softc *,
					  struct intrsource *);
static void	am18xx_intc_set_priority(struct pic_softc *, int);
static void	am18xx_intc_irq_handler(void *);
static void	*am18xx_intc_fdt_establish(device_t, u_int *, int, int,
					   int (*)(void *), void *,
					   const char *);
static void 	am18xx_intc_fdt_disestablish(device_t, void *);
static bool 	am18xx_intc_fdt_intrstr(device_t, u_int *, char *, size_t);
static void 	am18xx_intc_enable_interrupts(struct am18xx_intc_softc *);

#define AM18XX_AINTC_CR 	0x4
#define AM18XX_AINTC_GER 	0x10
#define AM18XX_AINTC_SECR1	0x280
#define AM18XX_AINTC_ESR1 	0x300
#define AM18XX_AINTC_ECR1 	0x380
#define AM18XX_AINTC_CMR0 	0x400
#define AM18XX_AINTC_HIER 	0x1500

#define AM18XX_AINTC_GER_ENABLE	1
#define AM18XX_AINTC_HIER_IRQ	2
#define AM18XX_AINTC_IRQ_CHANNEL	4 /* any number between 2 and 31 works */

#define	INTC_READ(sc, reg)					\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, reg)
#define	INTC_WRITE(sc, reg, val)				\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, reg, val)
#define	PICTOSOFTC(pic)	\
	((struct am18xx_intc_softc *) \
	     ((uintptr_t)(pic) - offsetof(struct am18xx_intc_softc, sc_pic)))

static struct am18xx_intc_softc *intc_softc;

CFATTACH_DECL_NEW(am18xxintc, sizeof(struct am18xx_intc_softc),
		  am18xx_intc_match, am18xx_intc_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,cp-intc" },
	DEVICE_COMPAT_EOL
};

static const struct fdtbus_interrupt_controller_func am18xx_intc_fdt_funcs = {
	.establish = am18xx_intc_fdt_establish,
	.disestablish = am18xx_intc_fdt_disestablish,
	.intrstr = am18xx_intc_fdt_intrstr,
};

static const struct pic_ops am18xx_intc_picops = {
	.pic_unblock_irqs = am18xx_intc_unblock_irqs,
	.pic_block_irqs = am18xx_intc_block_irqs,
	.pic_establish_irq = am18xx_intc_establish_irq,
	.pic_set_priority = am18xx_intc_set_priority,
};

static void *
am18xx_intc_fdt_establish(device_t dev, u_int *specifier, int ipl, int flags,
			  int (*func)(void *), void *arg, const char *xname)
{
	struct am18xx_intc_softc * const sc = device_private(dev);

	const u_int irq = be32toh(specifier[0]);
	if (irq >= sc->sc_num_irqs) {
		device_printf(dev, "IRQ %u is out of range\n", irq);
		return NULL;
	}

	const u_int mpsafe = (flags & FDT_INTR_MPSAFE) ? IST_MPSAFE : 0;
	return intr_establish_xname(irq, ipl, IST_LEVEL | mpsafe, func, arg,
				    xname);
}

static void
am18xx_intc_fdt_disestablish(device_t dev, void *ih)
{
	return intr_disestablish(ih);
}

static bool
am18xx_intc_fdt_intrstr(device_t dev, u_int *specifier, char *buf,
		      size_t buflen)
{
	const u_int irq = be32toh(specifier[0]);
	snprintf(buf, buflen, "irq %d", irq);
	return true;
}

static void
am18xx_intc_unblock_irqs(struct pic_softc *pic, size_t irq_base, uint32_t mask)
{
	struct am18xx_intc_softc * const sc = PICTOSOFTC(pic);

	const size_t group = irq_base / 32;
	uint32_t esr_reg = AM18XX_AINTC_ESR1 + 4 * group;
	INTC_WRITE(sc, esr_reg, mask);
}

static void
am18xx_intc_block_irqs(struct pic_softc *pic, size_t irq_base, uint32_t mask)
{
	struct am18xx_intc_softc * const sc = PICTOSOFTC(pic);

	const size_t group = irq_base / 32;
	uint32_t ecr_reg = AM18XX_AINTC_ECR1 + 4 * group;
	INTC_WRITE(sc, ecr_reg, mask);
}

static void
am18xx_intc_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	struct am18xx_intc_softc * const sc = PICTOSOFTC(pic);

	/* there is one CMR register per 4 interrupts*/
	uint32_t cmr_reg = AM18XX_AINTC_CMR0 + (is->is_irq & (~0x3));
	uint32_t cmr_shift = (is->is_irq % 4) * 8;

	/* update CMR register with channel */
	uint32_t cmr = INTC_READ(sc, cmr_reg);
	cmr = cmr & ~(0xFF<<cmr_shift);
	cmr |= AM18XX_AINTC_IRQ_CHANNEL << cmr_shift;
	INTC_WRITE(sc, cmr_reg, cmr);
}

static void
am18xx_intc_set_priority(struct pic_softc *pic, int new_ipl)
{
	curcpu()->ci_cpl = new_ipl;
}

static int
find_pending_irqs(struct am18xx_intc_softc *sc, size_t group)
{
	uint32_t reg = AM18XX_AINTC_SECR1 + (4 * group);
	uint32_t pending = INTC_READ(sc, reg);

	/* clear interrupts */
	INTC_WRITE(sc, reg, pending);

	if (pending == 0)
		return 0;

	return pic_mark_pending_sources(&sc->sc_pic, group * 32, pending);
}

static void
am18xx_intc_irq_handler(void *frame)
{
	struct cpu_info * const ci = curcpu();
	struct am18xx_intc_softc * const sc = intc_softc;
	const int old_ipl = ci->ci_cpl;
	const uint32_t oldipl_mask = __BIT(old_ipl);
	int ipl_mask = 0;

	ci->ci_data.cpu_nintr++;

	for (int group = 0; group <= sc->sc_num_irqs/32; group++) {
		ipl_mask |= find_pending_irqs(sc, group);
	}

	if ((ipl_mask & ~oldipl_mask) > oldipl_mask)
		pic_do_pending_ints(I32_bit, old_ipl, frame);
}

/*
 * Enable interrupt according to the TRM section 11.3.2
 */
static void am18xx_intc_enable_interrupts(struct am18xx_intc_softc *sc)
{
	/* disable interrupts */
	INTC_WRITE(sc, AM18XX_AINTC_GER, 0);
	INTC_WRITE(sc, AM18XX_AINTC_HIER, 0);

	/* disable nesting and prioritization in CR */
	INTC_WRITE(sc, AM18XX_AINTC_CR, 0);

	/* disable all interrupts, clear their status */
	for (int i = 0; i <= sc->sc_num_irqs / 32; i++) {
		INTC_WRITE(sc, AM18XX_AINTC_ECR1 + 4 * i, 0xFFFFFFFF);
		INTC_WRITE(sc, AM18XX_AINTC_SECR1 + 4 * i, 0xFFFFFFFF);
	}

	/* enable IRQ interrupts */
	INTC_WRITE(sc, AM18XX_AINTC_HIER, AM18XX_AINTC_HIER_IRQ);

	/* set enable bit in GER */
	INTC_WRITE(sc, AM18XX_AINTC_GER, AM18XX_AINTC_GER_ENABLE);
}

int
am18xx_intc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

void
am18xx_intc_attach(device_t parent, device_t self, void *aux)
{
	struct am18xx_intc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	uint32_t maxsources;

	sc->sc_bst = faa->faa_bst;
	intc_softc = sc;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	if (of_getprop_uint32(phandle, "ti,intc-size", &maxsources) != 0) {
		aprint_error(": couldn't get max interrupt number\n");
		return;
	}
	sc->sc_num_irqs = maxsources;

	sc->sc_pic.pic_ops = &am18xx_intc_picops;
	sc->sc_pic.pic_maxsources = maxsources;
	strlcpy(sc->sc_pic.pic_name, device_xname(self),
		sizeof(sc->sc_pic.pic_name));

	pic_add(&sc->sc_pic, 0);
	fdtbus_register_interrupt_controller(self, phandle,
					     &am18xx_intc_fdt_funcs);

	arm_fdt_irq_set_handler(am18xx_intc_irq_handler);
	am18xx_intc_enable_interrupts(sc);

	aprint_normal("\n");
}


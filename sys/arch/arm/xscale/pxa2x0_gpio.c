/*	$NetBSD: pxa2x0_gpio.c,v 1.7.24.1 2008/01/01 15:39:47 chris Exp $	*/

/*
 * Copyright 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pxa2x0_gpio.c,v 1.7.24.1 2008/01/01 15:39:47 chris Exp $");

#include "opt_pxa2x0_gpio.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include "locators.h"

struct gpio_irq_handler {
	struct gpio_irq_handler *gh_next;
	int (*gh_func)(void *);
	void *gh_arg;
	int gh_spl;
	u_int gh_gpio;
	int gh_level;
};

struct pxagpio_softc {
	struct device sc_dev;
	bus_space_tag_t sc_bust;
	bus_space_handle_t sc_bush;
	void *sc_irqcookie[4];
	u_int32_t sc_mask[4];
#ifdef PXAGPIO_HAS_GPION_INTRS
	struct gpio_irq_handler *sc_handlers[GPIO_NPINS];
#else
	struct gpio_irq_handler *sc_handlers[2];
#endif
};

static int	pxagpio_match(struct device *, struct cfdata *, void *);
static void	pxagpio_attach(struct device *, struct device *, void *);

CFATTACH_DECL(pxagpio, sizeof(struct pxagpio_softc),
    pxagpio_match, pxagpio_attach, NULL, NULL);

static struct pxagpio_softc *pxagpio_softc;
static vaddr_t pxagpio_regs;
#define GPIO_BOOTSTRAP_REG(reg)	\
	(*((volatile u_int32_t *)(pxagpio_regs + (reg))))

static int gpio_intr0(void *);
static int gpio_intr1(void *);
#ifdef PXAGPIO_HAS_GPION_INTRS
static int gpio_dispatch(struct pxagpio_softc *, int);
static int gpio_intrN(void *);
#endif

static inline u_int32_t
pxagpio_reg_read(struct pxagpio_softc *sc, int reg)
{
	if (__predict_true(sc != NULL))
		return (bus_space_read_4(sc->sc_bust, sc->sc_bush, reg));
	else
	if (pxagpio_regs)
		return (GPIO_BOOTSTRAP_REG(reg));
	panic("pxagpio_reg_read: not bootstrapped");
}

static inline void
pxagpio_reg_write(struct pxagpio_softc *sc, int reg, u_int32_t val)
{
	if (__predict_true(sc != NULL))
		bus_space_write_4(sc->sc_bust, sc->sc_bush, reg, val);
	else
	if (pxagpio_regs)
		GPIO_BOOTSTRAP_REG(reg) = val;
	else
		panic("pxagpio_reg_write: not bootstrapped");
	return;
}

static int
pxagpio_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pxaip_attach_args *pxa = aux;

	if (pxagpio_softc != NULL || pxa->pxa_addr != PXA2X0_GPIO_BASE)
		return (0);

	pxa->pxa_size = PXA2X0_GPIO_SIZE;

	return (1);
}

static void
pxagpio_attach(struct device *parent, struct device *self, void *aux)
{
	struct pxagpio_softc *sc = (struct pxagpio_softc *)self;
	struct pxaip_attach_args *pxa = aux;

	sc->sc_bust = pxa->pxa_iot;

	aprint_normal(": GPIO Controller\n");

	if (bus_space_map(sc->sc_bust, pxa->pxa_addr, pxa->pxa_size, 0,
	    &sc->sc_bush)) {
		aprint_error("%s: Can't map registers!\n", sc->sc_dev.dv_xname);
		return;
	}

	pxagpio_regs = (vaddr_t)bus_space_vaddr(sc->sc_bust, sc->sc_bush);

	memset(sc->sc_handlers, 0, sizeof(sc->sc_handlers));

	/*
	 * Disable all GPIO interrupts
	 */
	pxagpio_reg_write(sc, GPIO_GRER0, 0);
	pxagpio_reg_write(sc, GPIO_GRER1, 0);
	pxagpio_reg_write(sc, GPIO_GRER2, 0);
	pxagpio_reg_write(sc, GPIO_GFER0, 0);
	pxagpio_reg_write(sc, GPIO_GFER1, 0);
	pxagpio_reg_write(sc, GPIO_GFER2, 0);
	pxagpio_reg_write(sc, GPIO_GEDR0, ~0);
	pxagpio_reg_write(sc, GPIO_GEDR1, ~0);
	pxagpio_reg_write(sc, GPIO_GEDR2, ~0);
#ifdef	CPU_XSCALE_PXA270
	if (CPU_IS_PXA270) {
		pxagpio_reg_write(sc, GPIO_GRER3, 0);
		pxagpio_reg_write(sc, GPIO_GFER3, 0);
		pxagpio_reg_write(sc, GPIO_GEDR3, ~0);
	}
#endif

#ifdef PXAGPIO_HAS_GPION_INTRS
	sc->sc_irqcookie[2] = pxa2x0_intr_establish(PXA2X0_INT_GPION, IPL_BIO,
	    gpio_intrN, sc);
	if (sc->sc_irqcookie[2] == NULL) {
		aprint_error("%s: failed to hook main GPIO interrupt\n",
		    sc->sc_dev.dv_xname);
		return;
	}
#endif

	sc->sc_irqcookie[0] = sc->sc_irqcookie[1] = NULL;

	pxagpio_softc = sc;
}

void
pxa2x0_gpio_bootstrap(vaddr_t gpio_regs)
{

	pxagpio_regs = gpio_regs;
}

void *
pxa2x0_gpio_intr_establish(u_int gpio, int level, int spl, int (*func)(void *),
    void *arg)
{
	struct pxagpio_softc *sc = pxagpio_softc;
	struct gpio_irq_handler *gh;
	u_int32_t bit, reg;

#ifdef DEBUG
#ifdef PXAGPIO_HAS_GPION_INTRS
	if (gpio >= GPIO_NPINS)
		panic("pxa2x0_gpio_intr_establish: bad pin number: %d", gpio);
#else
	if (gpio > 1)
		panic("pxa2x0_gpio_intr_establish: bad pin number: %d", gpio);
#endif
#endif

	if (!GPIO_IS_GPIO_IN(pxa2x0_gpio_get_function(gpio)))
		panic("pxa2x0_gpio_intr_establish: Pin %d not GPIO_IN", gpio);

	switch (level) {
	case IST_EDGE_FALLING:
	case IST_EDGE_RISING:
	case IST_EDGE_BOTH:
		break;

	default:
		panic("pxa2x0_gpio_intr_establish: bad level: %d", level);
		break;
	}

	if (sc->sc_handlers[gpio] != NULL)
		panic("pxa2x0_gpio_intr_establish: illegal shared interrupt");

	MALLOC(gh, struct gpio_irq_handler *, sizeof(struct gpio_irq_handler),
	    M_DEVBUF, M_NOWAIT);

	gh->gh_func = func;
	gh->gh_arg = arg;
	gh->gh_spl = spl;
	gh->gh_gpio = gpio;
	gh->gh_level = level;
	gh->gh_next = sc->sc_handlers[gpio];
	sc->sc_handlers[gpio] = gh;

	if (gpio == 0) {
		KDASSERT(sc->sc_irqcookie[0] == NULL);
		sc->sc_irqcookie[0] = pxa2x0_intr_establish(PXA2X0_INT_GPIO0,
		    spl, gpio_intr0, sc);
		KDASSERT(sc->sc_irqcookie[0]);
	} else
	if (gpio == 1) {
		KDASSERT(sc->sc_irqcookie[1] == NULL);
		sc->sc_irqcookie[1] = pxa2x0_intr_establish(PXA2X0_INT_GPIO1,
		    spl, gpio_intr1, sc);
		KDASSERT(sc->sc_irqcookie[1]);
	}

	bit = GPIO_BIT(gpio);
	sc->sc_mask[GPIO_BANK(gpio)] |= bit;

	switch (level) {
	case IST_EDGE_FALLING:
		reg = pxagpio_reg_read(sc, GPIO_REG(GPIO_GFER0, gpio));
		pxagpio_reg_write(sc, GPIO_REG(GPIO_GFER0, gpio), reg | bit);
		break;

	case IST_EDGE_RISING:
		reg = pxagpio_reg_read(sc, GPIO_REG(GPIO_GRER0, gpio));
		pxagpio_reg_write(sc, GPIO_REG(GPIO_GRER0, gpio), reg | bit);
		break;

	case IST_EDGE_BOTH:
		reg = pxagpio_reg_read(sc, GPIO_REG(GPIO_GFER0, gpio));
		pxagpio_reg_write(sc, GPIO_REG(GPIO_GFER0, gpio), reg | bit);
		reg = pxagpio_reg_read(sc, GPIO_REG(GPIO_GRER0, gpio));
		pxagpio_reg_write(sc, GPIO_REG(GPIO_GRER0, gpio), reg | bit);
		break;
	}

	return (gh);
}

void
pxa2x0_gpio_intr_disestablish(void *cookie)
{
	struct pxagpio_softc *sc = pxagpio_softc;
	struct gpio_irq_handler *gh = cookie;
	u_int32_t bit, reg;

	bit = GPIO_BIT(gh->gh_gpio);

	reg = pxagpio_reg_read(sc, GPIO_REG(GPIO_GFER0, gh->gh_gpio));
	reg &= ~bit;
	pxagpio_reg_write(sc, GPIO_REG(GPIO_GFER0, gh->gh_gpio), reg);
	reg = pxagpio_reg_read(sc, GPIO_REG(GPIO_GRER0, gh->gh_gpio));
	reg &= ~bit;
	pxagpio_reg_write(sc, GPIO_REG(GPIO_GRER0, gh->gh_gpio), reg);

	pxagpio_reg_write(sc, GPIO_REG(GPIO_GEDR0, gh->gh_gpio), bit);

	sc->sc_mask[GPIO_BANK(gh->gh_gpio)] &= ~bit;
	sc->sc_handlers[gh->gh_gpio] = NULL;

	if (gh->gh_gpio == 0) {
#if 0
		pxa2x0_intr_disestablish(sc->sc_irqcookie[0]);
		sc->sc_irqcookie[0] = NULL;
#else
		panic("pxa2x0_gpio_intr_disestablish: can't unhook GPIO#0");
#endif
	} else
	if (gh->gh_gpio == 1) {
#if 0
		pxa2x0_intr_disestablish(sc->sc_irqcookie[1]);
		sc->sc_irqcookie[0] = NULL;
#else
		panic("pxa2x0_gpio_intr_disestablish: can't unhook GPIO#0");
#endif
	}

	FREE(gh, M_DEVBUF);
}

static int
gpio_intr0(void *arg)
{
	struct pxagpio_softc *sc = arg;

#ifdef DIAGNOSTIC
	if (sc->sc_handlers[0] == NULL) {
		printf("%s: stray GPIO#0 edge interrupt\n",
		    sc->sc_dev.dv_xname);
		return (0);
	}
#endif

	bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_REG(GPIO_GEDR0, 0),
	    GPIO_BIT(0));

	return ((sc->sc_handlers[0]->gh_func)(sc->sc_handlers[0]->gh_arg));
}

static int
gpio_intr1(void *arg)
{
	struct pxagpio_softc *sc = arg;

#ifdef DIAGNOSTIC
	if (sc->sc_handlers[1] == NULL) {
		printf("%s: stray GPIO#1 edge interrupt\n",
		    sc->sc_dev.dv_xname);
		return (0);
	}
#endif

	bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_REG(GPIO_GEDR0, 1),
	    GPIO_BIT(1));

	return ((sc->sc_handlers[1]->gh_func)(sc->sc_handlers[1]->gh_arg));
}

#ifdef PXAGPIO_HAS_GPION_INTRS
static int
gpio_dispatch(struct pxagpio_softc *sc, int gpio_base)
{
	struct gpio_irq_handler **ghp, *gh;
	int i, s, nhandled, handled, pins;
	u_int32_t gedr, mask;
	int bank;

	/* Fetch bitmap of pending interrupts on this GPIO bank */
	gedr = pxagpio_reg_read(sc, GPIO_REG(GPIO_GEDR0, gpio_base));

	/* Don't handle GPIO 0/1 here */
	if (gpio_base == 0)
		gedr &= ~(GPIO_BIT(0) | GPIO_BIT(1));

	/* Bail early if there are no pending interrupts in this bank */
	if (gedr == 0)
		return (0);

	/* Acknowledge pending interrupts. */
	pxagpio_reg_write(sc, GPIO_REG(GPIO_GEDR0, gpio_base), gedr);

	bank = GPIO_BANK(gpio_base);

	/*
	 * We're only interested in those for which we have a handler
	 * registered
	 */
#ifdef DEBUG
	if ((gedr & sc->sc_mask[bank]) == 0) {
		printf("%s: stray GPIO interrupt. Bank %d, GEDR 0x%08x, mask 0x%08x\n",
		    sc->sc_dev.dv_xname, bank, gedr, sc->sc_mask[bank]);
		return (1);	/* XXX: Pretend we dealt with it */
	}
#endif

	gedr &= sc->sc_mask[bank];
	ghp = &sc->sc_handlers[gpio_base];
	if (CPU_IS_PXA270)
		pins = (gpio_base < 96) ? 32 : 25;
	else
		pins = (gpio_base < 64) ? 32 : 17;
	handled = 0;

	for (i = 0, mask = 1; i < pins && gedr; i++, ghp++, mask <<= 1) {
		if ((gedr & mask) == 0)
			continue;
		gedr &= ~mask;

		if ((gh = *ghp) == NULL) {
			printf("%s: unhandled GPIO interrupt. GPIO#%d\n",
			    sc->sc_dev.dv_xname, gpio_base + i);
			continue;
		}

		s = _splraise(gh->gh_spl);
		do {
			nhandled = (gh->gh_func)(gh->gh_arg);
			handled |= nhandled;
			gh = gh->gh_next;
		} while (gh != NULL);
		splx(s);
	}

	return (handled);
}

static int
gpio_intrN(void *arg)
{
	struct pxagpio_softc *sc = arg;
	int handled;

	handled = gpio_dispatch(sc, 0);
	handled |= gpio_dispatch(sc, 32);
	handled |= gpio_dispatch(sc, 64);
	if (CPU_IS_PXA270)
		handled |= gpio_dispatch(sc, 96);
	return (handled);
}
#endif	/* PXAGPIO_HAS_GPION_INTRS */

u_int
pxa2x0_gpio_get_function(u_int gpio)
{
	struct pxagpio_softc *sc = pxagpio_softc;
	u_int32_t rv, io;

	KDASSERT(gpio < GPIO_NPINS);

	rv = pxagpio_reg_read(sc, GPIO_FN_REG(gpio)) >> GPIO_FN_SHIFT(gpio);
	rv = GPIO_FN(rv);

	io = pxagpio_reg_read(sc, GPIO_REG(GPIO_GPDR0, gpio));
	if (io & GPIO_BIT(gpio))
		rv |= GPIO_OUT;

	io = pxagpio_reg_read(sc, GPIO_REG(GPIO_GPLR0, gpio));
	if (io & GPIO_BIT(gpio))
		rv |= GPIO_SET;

	return (rv);
}

u_int
pxa2x0_gpio_set_function(u_int gpio, u_int fn)
{
	struct pxagpio_softc *sc = pxagpio_softc;
	u_int32_t rv, bit;
	u_int oldfn;

	KDASSERT(gpio < GPIO_NPINS);

	oldfn = pxa2x0_gpio_get_function(gpio);

	if (GPIO_FN(fn) == GPIO_FN(oldfn) &&
	    GPIO_FN_IS_OUT(fn) == GPIO_FN_IS_OUT(oldfn)) {
		/*
		 * The pin's function is not changing.
		 * For Alternate Functions and GPIO input, we can just
		 * return now.
		 * For GPIO output pins, check the initial state is
		 * the same.
		 *
		 * Return 'fn' instead of 'oldfn' so the caller can
		 * reliably detect that we didn't change anything.
		 * (The initial state might be different for non-
		 * GPIO output pins).
		 */
		if (!GPIO_IS_GPIO_OUT(fn) ||
		    GPIO_FN_IS_SET(fn) == GPIO_FN_IS_SET(oldfn))
			return (fn);
	}

	/*
	 * See section 4.1.3.7 of the PXA2x0 Developer's Manual for
	 * the correct procedure for changing GPIO pin functions.
	 */

	bit = GPIO_BIT(gpio);

	/*
	 * 1. Configure the correct set/clear state of the pin
	 */
	if (GPIO_FN_IS_SET(fn))
		pxagpio_reg_write(sc, GPIO_REG(GPIO_GPSR0, gpio), bit);
	else
		pxagpio_reg_write(sc, GPIO_REG(GPIO_GPCR0, gpio), bit);

	/*
	 * 2. Configure the pin as an input or output as appropriate
	 */
	rv = pxagpio_reg_read(sc, GPIO_REG(GPIO_GPDR0, gpio)) & ~bit;
	if (GPIO_FN_IS_OUT(fn))
		rv |= bit;
	pxagpio_reg_write(sc, GPIO_REG(GPIO_GPDR0, gpio), rv);

	/*
	 * 3. Configure the pin's function
	 */
	bit = GPIO_FN_MASK << GPIO_FN_SHIFT(gpio);
	fn = GPIO_FN(fn) << GPIO_FN_SHIFT(gpio);
	rv = pxagpio_reg_read(sc, GPIO_FN_REG(gpio)) & ~bit;
	pxagpio_reg_write(sc, GPIO_FN_REG(gpio), rv | fn);

	return (oldfn);
}

/* 
 * Quick function to read pin value
 */
int
pxa2x0_gpio_get_bit(u_int gpio)
{
	struct pxagpio_softc *sc = pxagpio_softc;
	int bit;

	bit = GPIO_BIT(gpio);
	if (pxagpio_reg_read(sc, GPIO_REG(GPIO_GPLR0, gpio)) & bit)
		return 1;
	else 
		return 0;
}

/* 
 * Quick function to set pin to 1
 */
void
pxa2x0_gpio_set_bit(u_int gpio)
{
	struct pxagpio_softc *sc = pxagpio_softc;
	int bit;

	bit = GPIO_BIT(gpio);
	pxagpio_reg_write(sc, GPIO_REG(GPIO_GPSR0, gpio), bit);
}

/* 
 * Quick function to set pin to 0
 */
void
pxa2x0_gpio_clear_bit(u_int gpio)
{
	struct pxagpio_softc *sc = pxagpio_softc;
	int bit;

	bit = GPIO_BIT(gpio);
	pxagpio_reg_write(sc, GPIO_REG(GPIO_GPCR0, gpio), bit);
}

/* 
 * Quick function to change pin direction
 */
void
pxa2x0_gpio_set_dir(u_int gpio, int dir)
{
	struct pxagpio_softc *sc = pxagpio_softc;
	int bit;
	u_int32_t reg;

	bit = GPIO_BIT(gpio);

	reg = pxagpio_reg_read(sc, GPIO_REG(GPIO_GPDR0, gpio)) & ~bit;
	if (GPIO_FN_IS_OUT(dir))
		reg |= bit;
	pxagpio_reg_write(sc, GPIO_REG(GPIO_GPDR0, gpio), reg);
}

/* 
 * Quick function to clear interrupt status on a pin
 * GPIO pins may be toggle in an interrupt and we dont want
 * extra spurious interrupts to occur.
 * Suppose this causes a slight race if a key is pressed while
 * the interrupt handler is running. (yes this is for the keyboard driver)
 */
void
pxa2x0_gpio_clear_intr(u_int gpio)
{
	struct pxagpio_softc *sc = pxagpio_softc;
	int bit;

	bit = GPIO_BIT(gpio);
	pxagpio_reg_write(sc, GPIO_REG(GPIO_GEDR0, gpio), bit);
}

/*
 * Quick function to mask (disable) a GPIO interrupt
 */
void
pxa2x0_gpio_intr_mask(void *v)
{
	struct gpio_irq_handler *gh = (struct gpio_irq_handler *)v;

	pxa2x0_gpio_set_intr_level(gh->gh_gpio, IPL_NONE);
}

/*
 * Quick function to unmask (enable) a GPIO interrupt
 */
void
pxa2x0_gpio_intr_unmask(void *v)
{
	struct gpio_irq_handler *gh = (struct gpio_irq_handler *)v;

	pxa2x0_gpio_set_intr_level(gh->gh_gpio, gh->gh_level);
}

/*
 * Configure the edge sensitivity of interrupt pins
 */
void
pxa2x0_gpio_set_intr_level(u_int gpio, int level)
{
	struct pxagpio_softc *sc = pxagpio_softc;
	u_int32_t bit;
	u_int32_t gfer;
	u_int32_t grer;
	int s;

	s = splhigh();

	bit = GPIO_BIT(gpio);
	gfer = pxagpio_reg_read(sc, GPIO_REG(GPIO_GFER0, gpio));
	grer = pxagpio_reg_read(sc, GPIO_REG(GPIO_GRER0, gpio));

	switch (level) {
	case IST_NONE:
		gfer &= ~bit;
		grer &= ~bit;
		break;
	case IST_EDGE_FALLING:
		gfer |= bit;
		grer &= ~bit;
		break;
	case IST_EDGE_RISING:
		gfer &= ~bit;
		grer |= bit;
		break;
	case IST_EDGE_BOTH:
		gfer |= bit;
		grer |= bit;
		break;
	default:
		panic("pxa2x0_gpio_set_intr_level: bad level: %d", level);
		break;
	}

	pxagpio_reg_write(sc, GPIO_REG(GPIO_GFER0, gpio), gfer);
	pxagpio_reg_write(sc, GPIO_REG(GPIO_GRER0, gpio), grer);

	splx(s);
}


#if defined(CPU_XSCALE_PXA250)
/*
 * Configurations of GPIO for PXA25x
 */
struct pxa2x0_gpioconf pxa25x_com_btuart_gpioconf[] = {
	{ 42, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* BTRXD */
	{ 43, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* BTTXD */

#if 0	/* optional */
	{ 44, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* BTCTS */
	{ 45, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* BTRTS */
#endif

	{ -1 }
};

struct pxa2x0_gpioconf pxa25x_com_ffuart_gpioconf[] = {
	{ 34, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* FFRXD */

#if 0	/* optional */
	{ 35, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* CTS */
	{ 36, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* DCD */
	{ 37, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* DSR */
	{ 38, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* RI */
#endif

	{ 39, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* FFTXD */

#if 0	/* optional */
	{ 40, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* DTR */
	{ 41, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* RTS */
#endif

	{ -1 }
};

struct pxa2x0_gpioconf pxa25x_com_hwuart_gpioconf[] = {
#if 0	/* We can select and/or. */
	{ 42, GPIO_CLR | GPIO_ALT_FN_3_IN },	/* HWRXD */
	{ 49, GPIO_CLR | GPIO_ALT_FN_2_IN },	/* HWRXD */

	{ 43, GPIO_CLR | GPIO_ALT_FN_3_OUT },	/* HWTXD */
	{ 48, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* HWTXD */

#if 0	/* optional */
	{ 44, GPIO_CLR | GPIO_ALT_FN_3_IN },	/* HWCST */
	{ 51, GPIO_CLR | GPIO_ALT_FN_3_IN },	/* HWCST */

	{ 45, GPIO_CLR | GPIO_ALT_FN_3_OUT },	/* HWRST */
	{ 52, GPIO_CLR | GPIO_ALT_FN_3_OUT },	/* HWRST */
#endif
#endif

	{ -1 }
};

struct pxa2x0_gpioconf pxa25x_com_stuart_gpioconf[] = {
	{ 46, GPIO_CLR | GPIO_ALT_FN_2_IN },	/* RXD */
	{ 47, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* TXD */
	{ -1 }
};

struct pxa2x0_gpioconf pxa25x_i2c_gpioconf[] = {
	{ -1 }
};

struct pxa2x0_gpioconf pxa25x_i2s_gpioconf[] = {
	{ 28, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* BITCLK */
	{ 29, GPIO_CLR | GPIO_ALT_FN_2_IN },	/* SDATA_IN */
	{ 30, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* SDATA_OUT */
	{ 31, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* SYNC */
	{ 32, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* SYSCLK */
	{ -1 }
};

struct pxa2x0_gpioconf pxa25x_pcic_gpioconf[] = {
	{ 48, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* nPOE */
	{ 49, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* nPWE */
	{ 50, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* nPIOR */
	{ 51, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* nPIOW */

#if 0	/* We can select and/or. */
	{ 52, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* nPCE1 */
	{ 53, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* nPCE2 */
#endif

	{ 54, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* pSKTSEL */
	{ 55, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* nPREG */
	{ 56, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* nPWAIT */
	{ 57, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* nIOIS16 */
	{ -1 }
};

struct pxa2x0_gpioconf pxa25x_pxaacu_gpioconf[] = {
	{ 28, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* BITCLK */
	{ 30, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* SDATA_OUT */
	{ 31, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* SYNC */

#if 0	/* We can select and/or. */
	{ 29, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* SDATA_IN0 */
	{ 32, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* SDATA_IN1 */
#endif

	{ -1 }
};

struct pxa2x0_gpioconf pxa25x_pxamci_gpioconf[] = {
#if 0	/* We can select and/or. */
	{  6, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* MMCCLK */
	{ 53, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* MMCCLK */
	{ 54, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* MMCCLK */

	{  8, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* MMCCS0 */
	{ 34, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* MMCCS0 */
	{ 67, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* MMCCS0 */

	{  9, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* MMCCS1 */
	{ 39, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* MMCCS1 */
	{ 68, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* MMCCS1 */
#endif

	{  -1 }
};
#endif

#if defined(CPU_XSCALE_PXA270)
/*
 * Configurations of GPIO for PXA27x
 */
struct pxa2x0_gpioconf pxa27x_com_btuart_gpioconf[] = {
	{  42, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* BTRXD */
	{  43, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* BTTXD */

#if 0	/* optional */
	{  44, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* BTCTS */
	{  45, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* BTRTS */
#endif

	{  -1 }
};

struct pxa2x0_gpioconf pxa27x_com_ffuart_gpioconf[] = {
#if 0	/* We can select and/or. */
	{  16, GPIO_CLR | GPIO_ALT_FN_3_OUT },	/* FFTXD */
	{  37, GPIO_CLR | GPIO_ALT_FN_3_OUT },	/* FFTXD */
	{  39, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* FFTXD */
	{  83, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* FFTXD */
	{  99, GPIO_CLR | GPIO_ALT_FN_3_OUT },	/* FFTXD */

	{  19, GPIO_CLR | GPIO_ALT_FN_3_IN },	/* FFRXD */
	{  33, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* FFRXD */
	{  34, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* FFRXD */
	{  41, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* FFRXD */
	{  53, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* FFRXD */
	{  85, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* FFRXD */
	{  96, GPIO_CLR | GPIO_ALT_FN_3_IN },	/* FFRXD */
	{ 102, GPIO_CLR | GPIO_ALT_FN_3_IN },	/* FFRXD */

	{   9, GPIO_CLR | GPIO_ALT_FN_3_IN },	/* FFCTS */
	{  26, GPIO_CLR | GPIO_ALT_FN_3_IN },	/* FFCTS */
	{  35, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* FFCTS */
	{ 100, GPIO_CLR | GPIO_ALT_FN_3_IN },	/* FFCTS */

	{  27, GPIO_CLR | GPIO_ALT_FN_3_OUT },	/* FFRTS */
	{  41, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* FFRTS */
	{  83, GPIO_CLR | GPIO_ALT_FN_3_OUT },	/* FFRTS */
	{  98, GPIO_CLR | GPIO_ALT_FN_3_OUT },	/* FFRTS */

	{  40, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* FFDTR */
	{  82, GPIO_CLR | GPIO_ALT_FN_3_OUT },	/* FFDTR */

	{  36, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* FFDCD */

	{  33, GPIO_CLR | GPIO_ALT_FN_2_IN },	/* FFDSR */
	{  37, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* FFDSR */

	{  38, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* FFRI */
#endif
	{  -1 }
};

struct pxa2x0_gpioconf pxa27x_com_hwuart_gpioconf[] = {
	{  -1 }
};

struct pxa2x0_gpioconf pxa27x_com_stuart_gpioconf[] = {
	{  46, GPIO_CLR | GPIO_ALT_FN_2_IN },	/* STD_RXD */
	{  47, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* STD_TXD */
	{  -1 }
};

struct pxa2x0_gpioconf pxa27x_i2c_gpioconf[] = {
	{ 117, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* SCL */
	{ 118, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* SDA */
	{  -1 }
};

struct pxa2x0_gpioconf pxa27x_i2s_gpioconf[] = {
	{  28, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* I2S_BITCLK */
	{  29, GPIO_CLR | GPIO_ALT_FN_2_IN },	/* I2S_SDATA_IN */
	{  30, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* I2S_SDATA_OUT */
	{  31, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* I2S_SYNC */
	{ 113, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* I2S_SYSCLK */
	{  -1 }
};

struct pxa2x0_gpioconf pxa27x_pcic_gpioconf[] = {
	{  48, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* nPOE */
	{  49, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* nPWE */
	{  50, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* nPIOR */
	{  51, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* nPIOW */
	{  55, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* nPREG */
	{  56, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* nPWAIT */
	{  57, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* nIOIS16 */
	{ 104, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* pSKTSEL */

#if 0	/* We can select and/or. */
	{  85, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* nPCE1 */
	{  86, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* nPCE1 */
	{ 102, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* nPCE1 */

	{  54, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* nPCE2 */
	{  78, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* nPCE2 */
	{ 105, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* nPCE2 */
#endif

	{  -1 }
};

struct pxa2x0_gpioconf pxa27x_pxaacu_gpioconf[] = {
	{  28, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* BITCLK */
	{  30, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* SDATA_OUT */

#if 0	/* We can select and/or. */
	{  31, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* SYNC */
	{  94, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* SYNC */

	{  29, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* SDATA_IN0 */
	{ 116, GPIO_CLR | GPIO_ALT_FN_2_IN },	/* SDATA_IN0 */

	{  32, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* SDATA_IN1 */
	{  99, GPIO_CLR | GPIO_ALT_FN_2_IN },	/* SDATA_IN1 */

	{  95, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* RESET_n */
	{ 113, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* RESET_n */
#endif

	{  -1 }
};

struct pxa2x0_gpioconf pxa27x_pxamci_gpioconf[] = {
	{  32, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* MMCLK */
	{ 112, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* MMCMD */
	{  92, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* MMDAT<0> */

#if 0	/* optional */
	{ 109, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* MMDAT<1> */
	{ 110, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* MMDAT<2>/MMCCS<0> */
	{ 111, GPIO_CLR | GPIO_ALT_FN_1_IN },	/* MMDAT<3>/MMCCS<1> */
#endif

	{  -1 }
};
#endif

void
pxa2x0_gpio_config(struct pxa2x0_gpioconf **conflist)
{
	int i, j;

	for (i = 0; conflist[i] != NULL; i++)
		for (j = 0; conflist[i][j].pin != -1; j++)
			pxa2x0_gpio_set_function(conflist[i][j].pin,
			    conflist[i][j].value);
}

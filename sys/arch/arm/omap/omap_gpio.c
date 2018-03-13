/*	$NetBSD: omap_gpio.c,v 1.8 2018/03/13 06:41:53 ryo Exp $ */

/*
 * The OMAP GPIO Controller interface is inspired by pxa2x0_gpio.c
 *
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
__KERNEL_RCSID(0, "$NetBSD: omap_gpio.c,v 1.8 2018/03/13 06:41:53 ryo Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <arm/omap/omap_tipb.h>
#include <arm/omap/omap_gpio.h>
#include <arm/omap/omap_gpioreg.h>

#include "omapgpio.h"

#define GPIOEVNAMESZ 25

struct gpio_irq_handler {
	int (*gh_func)(void *);
	void *gh_arg;
	int gh_spl;
	u_int gh_gpio;
    	char name[GPIOEVNAMESZ];
	struct evcnt ev;
};

struct omapgpio_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bust;
	bus_space_handle_t sc_bush;
	void *sc_irqcookie;
	uint16_t sc_mask;
	struct gpio_irq_handler *sc_handlers[GPIO_NPINS];
};

static int	omapgpio_match(device_t, cfdata_t, void *);
static void	omapgpio_attach(device_t, device_t, void *);

extern struct cfdriver omapgpio_cd;

CFATTACH_DECL_NEW(omapgpio, sizeof(struct omapgpio_softc),
    omapgpio_match, omapgpio_attach, NULL, NULL);

static int	omapgpio_intr(void *);

static int
omapgpio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct tipb_attach_args *tipb = aux;

	if (tipb->tipb_addr == -1 || tipb->tipb_intr == -1) {
		panic("omapgpio must have addr and intr specified in config.");
	}

	tipb->tipb_size = OMAP_GPIO_SIZE;

	return (1);
}

void
omapgpio_attach(device_t parent, device_t self, void *aux)
{
	struct omapgpio_softc *sc = device_private(self);
	struct tipb_attach_args *tipb = aux;
	uint32_t reg;

	sc->sc_dev = self;
	sc->sc_bust = tipb->tipb_iot;

	aprint_normal(": GPIO Controller\n");

	if (device_unit(self) > NOMAPGPIO - 1) {
		aprint_error("%s: Unsupported GPIO module unit number.\n",
		    device_xname(sc->sc_dev));
		return;
	}

	if (bus_space_map(sc->sc_bust, tipb->tipb_addr, tipb->tipb_size, 0,
	    &sc->sc_bush)) {
		aprint_error("%s: Failed to map registers.\n",
		    device_xname(sc->sc_dev));
		return;
	}

	sc->sc_mask = 0;
	memset(sc->sc_handlers, 0, sizeof(sc->sc_handlers));

	/* Reset the module and wait for it to come back online. */
	reg = bus_space_read_4(sc->sc_bust, sc->sc_bush, GPIO_SYSCONFIG);
	bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_SYSCONFIG,
			  reg | (1 << GPIO_SYSCONFIG_SOFTRESET));
	do {
		reg = bus_space_read_4(sc->sc_bust, sc->sc_bush,
				       GPIO_SYSSTATUS);
	} while ((reg & 1) == 0);

	/* Enable sleep wakeups, and need "smart idle" mode for that, plus
	   autoidle the OCP interface clock. */

	reg = bus_space_read_4(sc->sc_bust, sc->sc_bush, GPIO_SYSCONFIG);
	reg &= ~(GPIO_SYSCONFIG_IDLEMODE_MASK << GPIO_SYSCONFIG_IDLEMODE);
	bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_SYSCONFIG,
			  reg | (1 << GPIO_SYSCONFIG_ENAWAKEUP) |
			  (GPIO_SYSCONFIG_SMARTIDLE <<
			   GPIO_SYSCONFIG_IDLEMODE) |
			  (1 << GPIO_SYSCONFIG_AUTOIDLE));

	/* Install our ISR. */
	sc->sc_irqcookie = omap_intr_establish(tipb->tipb_intr, IPL_BIO,
	    device_xname(sc->sc_dev), omapgpio_intr, sc);
	if (sc->sc_irqcookie == NULL) {
		aprint_error("%s: Failed to install interrupt handler.\n",
		    device_xname(sc->sc_dev));
		return;
	}
}

u_int
omap_gpio_get_direction(u_int gpio)
{
	struct omapgpio_softc *sc;
	uint32_t reg, bit;
	u_int rval;

	KDASSERT(gpio < NOMAPGPIO * GPIO_NPINS);

	sc = device_lookup_private(&omapgpio_cd, GPIO_MODULE(gpio));
	if (sc == NULL) {
		panic("omapgpio: GPIO Module for pin %d not configured.\n", gpio);
	}

	rval = 0;
	bit = GPIO_BIT(gpio);
	reg = bus_space_read_4(sc->sc_bust, sc->sc_bush, GPIO_DIRECTION);

	if ((reg & bit) == 0)
		/* Output. */
		rval |= GPIO_OUT;

	return (rval);
}

void
omap_gpio_set_direction(u_int gpio, u_int dir)
{
	struct omapgpio_softc *sc;
	uint32_t reg, bit;

	KDASSERT(gpio < NOMAPGPIO * GPIO_NPINS);

	sc = device_lookup_private(&omapgpio_cd, GPIO_MODULE(gpio));
	if (sc == NULL) {
		panic("omapgpio: GPIO Module for pin %d not configured.\n", gpio);
	}

	bit = GPIO_BIT(gpio);
	reg = bus_space_read_4(sc->sc_bust, sc->sc_bush,
	    GPIO_DIRECTION);

	if (GPIO_IS_IN(dir)) {
		/* Input. */
		bus_space_write_4(sc->sc_bust, sc->sc_bush,
		    GPIO_DIRECTION, reg | bit);
	} else {
		/* Output. */
		bus_space_write_4(sc->sc_bust, sc->sc_bush,
		    GPIO_DIRECTION, (reg & ~bit));
	}
}

u_int omap_gpio_read(u_int gpio)
{
	struct omapgpio_softc *sc;
	u_int bit;

	sc = device_lookup_private(&omapgpio_cd, GPIO_MODULE(gpio));
	if (sc == NULL)
		panic("omapgpio: GPIO Module for pin %d not configured.",
		      gpio);

	bit = GPIO_BIT(gpio);

	return (bus_space_read_4(sc->sc_bust, sc->sc_bush,
				 GPIO_DATAIN) & bit) ? 1 : 0;
}

void omap_gpio_write(u_int gpio, u_int value)
{
	struct omapgpio_softc *sc;
	u_int bit;

	sc = device_lookup_private(&omapgpio_cd, GPIO_MODULE(gpio));
	if (sc == NULL)
		panic("omapgpio: GPIO Module for pin %d not configured.",
		      gpio);

	bit = GPIO_BIT(gpio);

	if (value) {
		bus_space_write_4(sc->sc_bust, sc->sc_bush,
				  GPIO_SET_DATAOUT, bit);
	} else {
		bus_space_write_4(sc->sc_bust, sc->sc_bush,
				  GPIO_CLEAR_DATAOUT, bit);
	}
}

void *
omap_gpio_intr_establish(u_int gpio, int level, int spl,
    const char *name, int (*func)(void *), void *arg)
{
	struct omapgpio_softc *sc;
	struct gpio_irq_handler *gh;
	uint32_t bit, levelreg;
	u_int dir, relnum, off, reg;
	int levelctrl;

	KDASSERT(gpio < NOMAPGPIO * GPIO_NPINS);

	sc = device_lookup_private(&omapgpio_cd, GPIO_MODULE(gpio));
	if (sc == NULL) {
		panic("omapgpio: GPIO Module for pin %d not configured.", gpio);
	}

	dir = omap_gpio_get_direction(gpio);
	if (!GPIO_IS_IN(dir)) {
		panic("omapgpio: GPIO pin %d not an input.", gpio);
	}

	relnum = GPIO_RELNUM(gpio);
	bit = GPIO_BIT(gpio);

	if (sc->sc_handlers[relnum] != NULL) {
		panic("omapgpio: Illegal shared interrupt on pin %d", gpio);
	}

	gh = malloc(sizeof(struct gpio_irq_handler), M_DEVBUF, M_NOWAIT);
	if (gh == NULL)
		return gh;

	gh->gh_func = func;
	gh->gh_arg = arg;
	gh->gh_spl = spl;
	gh->gh_gpio = gpio;
	sc->sc_handlers[relnum] = gh;
	sc->sc_mask |= bit;
	snprintf(gh->name, GPIOEVNAMESZ, "#%d %s", gpio, name);
	evcnt_attach_dynamic(&gh->ev, EVCNT_TYPE_INTR, NULL,
			     "omap gpio", gh->name);

	/*
	 * Set up the level control.
	 *
	 * Note: Pins 0->7 on a module use EDGE_CTRL1, pins 8->15 use
	 * EDGE_CTRL2.
	 */
	levelctrl = 0;
	switch (level) {
	case IST_EDGE_FALLING:
	case IST_LEVEL_LOW: /* emulation */
		levelctrl = 1;
		break;
	case IST_EDGE_RISING:
	case IST_LEVEL_HIGH: /* emulation */
		levelctrl = 2;
		break;
	case IST_EDGE_BOTH:
		levelctrl = 3;
		break;
	default:
		panic("omapgpio: Unknown level %d.", level);
		/* NOTREACHED */
	}

	off = 2 * (relnum & 7);
	if (relnum < 8) {
		reg = GPIO_EDGE_CTRL1;
	} else {
		reg = GPIO_EDGE_CTRL2;
	}

	/* Temporarily set the level control to no trigger. */
	levelreg = bus_space_read_4(sc->sc_bust, sc->sc_bush, reg);
	levelreg &= ~(0x3 << off);
 	bus_space_write_4(sc->sc_bust, sc->sc_bush, reg, levelreg);

	/* Clear the IRQSTATUS bit for the pin we're about to change. */
	bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_IRQSTATUS, bit);

	/* Set the new level control value. */
	levelreg |= levelctrl << off;
	bus_space_write_4(sc->sc_bust, sc->sc_bush, reg, levelreg);

	/* Disable sleep wakeups for this pin unless enabled later. */
	bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_CLEAR_WAKEUPENA,
	    bit);

	/* Enable interrupt generation for that pin. */
	bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_SET_IRQENABLE,
	    bit);

	return (gh);
}

void
omap_gpio_intr_disestablish(void *cookie)
{
	struct omapgpio_softc *sc;
	struct gpio_irq_handler *gh = cookie;
	uint32_t bit;
	u_int gpio, relnum;

	KDASSERT(cookie != NULL);

	gpio = gh->gh_gpio;
	sc = device_lookup_private(&omapgpio_cd, GPIO_MODULE(gpio));
	bit = GPIO_BIT(gpio);
	relnum = GPIO_RELNUM(gpio);
	evcnt_detach(&gh->ev);

	/* Disable Wakeup enable for this gpio. */
	bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_CLEAR_WAKEUPENA,
	    bit);

	/* Disable interrupt generation for that gpio. */
	bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_CLEAR_IRQENABLE,
	    bit);

	sc->sc_mask &= ~bit;
	sc->sc_handlers[relnum] = NULL;


	free(gh, M_DEVBUF);
}

void
omap_gpio_intr_mask(void *cookie)
{
	struct omapgpio_softc *sc;
	struct gpio_irq_handler *gh = cookie;
	uint32_t bit;
	u_int gpio, relnum;

	KDASSERT(cookie != NULL);

	gpio = gh->gh_gpio;
	sc = device_lookup_private(&omapgpio_cd, GPIO_MODULE(gpio));
	bit = GPIO_BIT(gpio);
	relnum = GPIO_RELNUM(gpio);
	__USE(relnum);

	/* Disable interrupt generation for that gpio. */
	bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_CLEAR_IRQENABLE,
	    bit);

	sc->sc_mask &= ~bit;
}

void
omap_gpio_intr_unmask(void *cookie)
{
	struct omapgpio_softc *sc;
	struct gpio_irq_handler *gh = cookie;
	uint32_t bit;
	u_int gpio, relnum;

	KDASSERT(cookie != NULL);

	gpio = gh->gh_gpio;
	sc = device_lookup_private(&omapgpio_cd, GPIO_MODULE(gpio));
	bit = GPIO_BIT(gpio);
	relnum = GPIO_RELNUM(gpio);
	__USE(relnum);

	/* Enable interrupt generation for that pin. */
	bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_SET_IRQENABLE,
	    bit);

	sc->sc_mask |= bit;
}

void
omap_gpio_intr_wakeup(void *cookie, int enable)
{
	struct omapgpio_softc *sc;
	struct gpio_irq_handler *gh = cookie;
	uint32_t bit;
	u_int gpio, relnum;

	KDASSERT(cookie != NULL);

	gpio = gh->gh_gpio;
	sc = device_lookup_private(&omapgpio_cd, GPIO_MODULE(gpio));
	bit = GPIO_BIT(gpio);
	relnum = GPIO_RELNUM(gpio);
	__USE(relnum);

	if (enable)
		bus_space_write_4(sc->sc_bust, sc->sc_bush,
				  GPIO_SET_WAKEUPENA, bit);
	else
		bus_space_write_4(sc->sc_bust, sc->sc_bush,
				  GPIO_CLEAR_WAKEUPENA, bit);
}

static int
omapgpio_intr(void *arg)
{
	struct omapgpio_softc *sc = arg;
	struct gpio_irq_handler *gh;
	uint32_t irqs;
	int idx, handled, s, nattempts;

	/* Fetch the GPIO interrupts pending.  */
	irqs = bus_space_read_4(sc->sc_bust, sc->sc_bush, GPIO_IRQSTATUS);
	irqs &= GPIO_REG_MASK;
 	bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_IRQSTATUS, irqs);

	/*
	 * Since IRQSTATUS can change out from under us while we are busy
	 * servicing everything, keep on doing things until the IRQSTATUS
	 * register is clear.
	 */
	for (nattempts = 0, handled = 0;;) {
		/*
		 * Note: Apparently the GPIO block will set the bits in IRQSTATUS if
		 * the level trigger for that pin is set to anything other than NONE
		 * regardless of the IRQENABLE status.  Just mask off the ones that we
		 * care about when processing the ISR.
		 */
		irqs &= sc->sc_mask;
		if (irqs == 0) {
			/* Pretend that we handled everything. */
			return (1);
		}

		for (idx = 0; idx < GPIO_NPINS; idx++, irqs >>= 1) {
			if ((irqs & 1) == 0)
				continue;

			if ((gh = sc->sc_handlers[idx]) == NULL) {
				printf("%s: unhandled GPIO interrupt. GPIO# %d\n",
				    device_xname(sc->sc_dev), idx);
				continue;
			}

			gh->ev.ev_count++;
			s = _splraise(gh->gh_spl);
			handled |= (gh->gh_func)(gh->gh_arg);
  			splx(s);
		}

		/* Check IRQSTATUS again. */
		irqs = bus_space_read_4(sc->sc_bust, sc->sc_bush, GPIO_IRQSTATUS);
		irqs &= GPIO_REG_MASK;
		if (irqs == 0) {
			/* Done servicing interrupts. */
			break;
		} else if (nattempts++ == 10000) {
			/* TODO: Fix up the # of attempts and this logic after
			   some experimentation. */

			/* Ensure that we don't get stuck here. */
			panic("%s: Stuck in GPIO interrupt service routine.",
			    device_xname(sc->sc_dev));
		}
		bus_space_write_4(sc->sc_bust, sc->sc_bush, GPIO_IRQSTATUS, irqs);
	}

	return (handled);
}


/*	$NetBSD: mvsocgpp.c,v 1.3.2.2 2013/01/16 05:32:49 yamt Exp $	*/
/*
 * Copyright (c) 2008, 2010 KIYOHARA Takashi
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mvsocgpp.c,v 1.3.2.2 2013/01/16 05:32:49 yamt Exp $");

#include "gpio.h"

#define _INTR_PRIVATE

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/evcnt.h>
#include <sys/gpio.h>
#include <sys/kmem.h>

#include <machine/intr.h>

#include <arm/marvell/mvsocreg.h>
#include <arm/marvell/mvsocvar.h>
#include <arm/marvell/mvsocgppreg.h>
#include <arm/marvell/mvsocgppvar.h>
#include <arm/pic/picvar.h>

#include <dev/marvell/marvellvar.h>

#if NGPIO > 0
#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>
#endif

#define MVSOCGPP_DUMPREG

#define MVSOCGPP_READ(sc, reg) \
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define MVSOCGPP_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))

struct mvsocgpp_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	struct mvsocgpp_pic {
		struct pic_softc gpio_pic;
		int group;
		int shift;
		uint32_t edge;
		uint32_t level;
	} *sc_pic;

#if NGPIO > 0
	struct gpio_chipset_tag sc_gpio_chipset;
	gpio_pin_t *sc_pins;
#endif
};

static int mvsocgpp_match(device_t, struct cfdata *, void *);
static void mvsocgpp_attach(device_t, device_t, void *);

#ifdef MVSOCGPP_DUMPREG
static void mvsocgpp_dump_reg(struct mvsocgpp_softc *);
#endif

static void gpio_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void gpio_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static int gpio_pic_find_pending_irqs(struct pic_softc *);
static void gpio_pic_establish_irq(struct pic_softc *, struct intrsource *);

static struct pic_ops gpio_pic_ops = {
	.pic_unblock_irqs = gpio_pic_unblock_irqs,
	.pic_block_irqs = gpio_pic_block_irqs,
	.pic_find_pending_irqs = gpio_pic_find_pending_irqs,
	.pic_establish_irq = gpio_pic_establish_irq,
};

static struct mvsocgpp_softc *mvsocgpp_softc;	/* One unit per One SoC */
int gpp_irqbase = 0;
int gpp_npins = 0;


CFATTACH_DECL_NEW(mvsocgpp, sizeof(struct mvsocgpp_softc),
    mvsocgpp_match, mvsocgpp_attach, NULL, NULL);


/* ARGSUSED */
static int
mvsocgpp_match(device_t parent, struct cfdata *match, void *aux)
{
	struct marvell_attach_args *mva = aux;

	if (strcmp(mva->mva_name, match->cf_name) != 0)
		return 0;
	if (mva->mva_offset == MVA_OFFSET_DEFAULT ||
	    mva->mva_irq == MVA_IRQ_DEFAULT)
		return 0;

	mva->mva_size = MVSOC_GPP_SIZE;
	return 1;
}

/* ARGSUSED */
static void
mvsocgpp_attach(device_t parent, device_t self, void *aux)
{
	struct mvsocgpp_softc *sc = device_private(self);
	struct marvell_attach_args *mva = aux;
	struct pic_softc *gpio_pic;
#if NGPIO > 0
	struct gpiobus_attach_args gba;
	gpio_pin_t *pins;
	uint32_t mask, dir, valin, valout, polarity, blink;
#endif
	int i, j;
	void *ih;

	dir = valin = valout = polarity = blink = 0;

	aprint_normal(": Marvell SoC General Purpose I/O Port Interface\n");
	aprint_naive("\n");

	sc->sc_dev = self;
	sc->sc_iot = mva->mva_iot;
	/* Map I/O registers for oriongpp */
	if (bus_space_subregion(mva->mva_iot, mva->mva_ioh,
				mva->mva_offset, mva->mva_size, &sc->sc_ioh)) {
		aprint_error_dev(self, "can't map registers\n");
		return;
	}

	if (gpp_npins > 0)
		aprint_normal_dev(self, "%d gpio pins\n", gpp_npins);
	else {
		aprint_error_dev(self, "gpp_npins not initialized\n");
		return;
	}

	mvsocgpp_softc = sc;

	for (i = 0; i < gpp_npins; i += 32)
		MVSOCGPP_WRITE(sc, MVSOCGPP_GPIOIC(i), 0);

	sc->sc_pic =
	    kmem_zalloc(sizeof(struct mvsocgpp_pic) * howmany(gpp_npins, 8),
		KM_SLEEP);
	for (i = 0, j = 0; i < gpp_npins; i += 8, j++) {
		gpio_pic = &(sc->sc_pic + j)->gpio_pic;
		gpio_pic->pic_ops = &gpio_pic_ops;
		snprintf(gpio_pic->pic_name, sizeof(gpio_pic->pic_name),
		    "%s[%d:%d]", device_xname(self), i + 7, i);
		gpio_pic->pic_maxsources =
		    (gpp_npins - i) > 8 ? 8 : gpp_npins - i;
		pic_add(gpio_pic, gpp_irqbase + i);
		aprint_normal_dev(self, "interrupts %d..%d",
		    gpp_irqbase + i, gpp_irqbase + i + 7);
		ih = intr_establish(mva->mva_irq + j,
		    IPL_HIGH, IST_LEVEL_HIGH, pic_handle_intr, gpio_pic);
		aprint_normal(", intr %d\n", mva->mva_irq + j);

		(sc->sc_pic + j)->group = j;
		(sc->sc_pic + j)->shift = (j & 3) * 8;
	}

#ifdef MVSOCGPP_DUMPREG
	mvsocgpp_dump_reg(sc);
#endif

#if NGPIO > 0
	sc->sc_pins = kmem_zalloc(sizeof(gpio_pin_t) * gpp_npins, KM_SLEEP);

	for (i = 0, mask = 1; i < gpp_npins; i++, mask <<= 1) {
		if ((i & (32 - 1)) == 0) {
			mask = 1;
			dir = MVSOCGPP_READ(sc, MVSOCGPP_GPIODOEC(i));
			valin = MVSOCGPP_READ(sc, MVSOCGPP_GPIODI(i));
			valout = MVSOCGPP_READ(sc, MVSOCGPP_GPIODO(i));
			polarity = MVSOCGPP_READ(sc, MVSOCGPP_GPIODIP(i));
			blink = MVSOCGPP_READ(sc, MVSOCGPP_GPIOBE(i));
		}
		pins = &sc->sc_pins[i];
		pins->pin_num = i;
		pins->pin_caps = (GPIO_PIN_INPUT | GPIO_PIN_OUTPUT |
		    GPIO_PIN_INVIN | GPIO_PIN_PULSATE);
		if (dir & mask) {
			pins->pin_flags = GPIO_PIN_INPUT;
			pins->pin_state =
			    (valin & mask) ? GPIO_PIN_HIGH : GPIO_PIN_LOW;
		} else {
			pins->pin_flags = GPIO_PIN_OUTPUT;
			pins->pin_state =
			    (valout & mask) ? GPIO_PIN_HIGH : GPIO_PIN_LOW;
		}
		if (polarity & mask) {
			pins->pin_flags |= GPIO_PIN_INVIN;
		}
		if (blink & mask) {
			pins->pin_flags |= GPIO_PIN_PULSATE;
		}
	}
	sc->sc_gpio_chipset.gp_cookie = sc;
	sc->sc_gpio_chipset.gp_pin_read = mvsocgpp_pin_read;
	sc->sc_gpio_chipset.gp_pin_write = mvsocgpp_pin_write;
	sc->sc_gpio_chipset.gp_pin_ctl = mvsocgpp_pin_ctl;
	gba.gba_gc = &sc->sc_gpio_chipset;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = gpp_npins;
	config_found_ia(self, "gpiobus", &gba, gpiobus_print);
#endif
}

/*
 * arch/arm/pic functions.
 */

static void
gpio_pic_unblock_irqs(struct pic_softc *pic, size_t irqbase, uint32_t irq_mask)
{
	struct mvsocgpp_softc *sc = mvsocgpp_softc;
	struct mvsocgpp_pic *mvsocgpp_pic = (struct mvsocgpp_pic *)pic;
	uint32_t mask;
	int pin = mvsocgpp_pic->group << 3;

	irq_mask = irq_mask << mvsocgpp_pic->shift;
	MVSOCGPP_WRITE(sc, MVSOCGPP_GPIOIC(pin),
	    MVSOCGPP_READ(sc, MVSOCGPP_GPIOIC(pin)) & ~irq_mask);
	if (irq_mask & mvsocgpp_pic->edge) {
		mask = MVSOCGPP_READ(sc, MVSOCGPP_GPIOIM(pin));
		mask |= (irq_mask & mvsocgpp_pic->edge);
		MVSOCGPP_WRITE(sc, MVSOCGPP_GPIOIM(pin), mask);
	}
	if (irq_mask & mvsocgpp_pic->level) {
		mask = MVSOCGPP_READ(sc, MVSOCGPP_GPIOILM(pin));
		mask |= (irq_mask & mvsocgpp_pic->level);
		MVSOCGPP_WRITE(sc, MVSOCGPP_GPIOILM(pin), mask);
	}
}

/* ARGSUSED */
static void
gpio_pic_block_irqs(struct pic_softc *pic, size_t irqbase, uint32_t irq_mask)
{
	struct mvsocgpp_softc *sc = mvsocgpp_softc;
	struct mvsocgpp_pic *mvsocgpp_pic = (struct mvsocgpp_pic *)pic;
	int pin = mvsocgpp_pic->group << 3;

	irq_mask = irq_mask << mvsocgpp_pic->shift;
	MVSOCGPP_WRITE(sc, MVSOCGPP_GPIOIM(pin),
	    MVSOCGPP_READ(sc, MVSOCGPP_GPIOIM(pin)) & ~irq_mask);
	MVSOCGPP_WRITE(sc, MVSOCGPP_GPIOILM(pin),
	    MVSOCGPP_READ(sc, MVSOCGPP_GPIOILM(pin)) & ~irq_mask);
}

static int
gpio_pic_find_pending_irqs(struct pic_softc *pic)
{
	struct mvsocgpp_softc *sc = mvsocgpp_softc;
	struct mvsocgpp_pic *mvsocgpp_pic = (struct mvsocgpp_pic *)pic;
	uint32_t pending;
	int pin = mvsocgpp_pic->group << 3;

	pending = MVSOCGPP_READ(sc, MVSOCGPP_GPIOIC(pin));
	pending &= (0xff << mvsocgpp_pic->shift);
	pending &= (MVSOCGPP_READ(sc, MVSOCGPP_GPIOIM(pin)) |
		    MVSOCGPP_READ(sc, MVSOCGPP_GPIOILM(pin)));
	pending = pending >> mvsocgpp_pic->shift;

	if (pending == 0)
		return 0;

	return pic_mark_pending_sources(pic, 0, pending);
}

static void
gpio_pic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	struct mvsocgpp_softc *sc = mvsocgpp_softc;
	struct mvsocgpp_pic *mvsocgpp_pic = (struct mvsocgpp_pic *)pic;
	uint32_t im, ilm, mask;
	int type, pin;

	type = is->is_type;
	pin = pic->pic_irqbase + is->is_irq - gpp_irqbase;
	mask = MVSOCGPP_GPIOPIN(pin);

	switch (type) {
	case IST_LEVEL_LOW:
	case IST_EDGE_FALLING:
		mvsocgpp_pin_ctl(NULL, pin, GPIO_PIN_INPUT | GPIO_PIN_INVIN);
		break;

	case IST_LEVEL_HIGH:
	case IST_EDGE_RISING:
		mvsocgpp_pin_ctl(NULL, pin, GPIO_PIN_INPUT);
		break;

	default:
		panic("unknwon interrupt type %d for pin %d.\n", type, pin);
	}

	im = MVSOCGPP_READ(sc, MVSOCGPP_GPIOIM(pin));
	ilm = MVSOCGPP_READ(sc, MVSOCGPP_GPIOILM(pin));
	switch (type) {
	case IST_EDGE_FALLING:
	case IST_EDGE_RISING:
		im |= mask;
		ilm &= ~mask;
		mvsocgpp_pic->edge |= mask;
		mvsocgpp_pic->level &= ~mask;
		break;

	case IST_LEVEL_LOW:
	case IST_LEVEL_HIGH:
		im &= ~mask;
		ilm |= mask;
		mvsocgpp_pic->edge &= ~mask;
		mvsocgpp_pic->level |= mask;
		break;
	}
	MVSOCGPP_WRITE(sc, MVSOCGPP_GPIOIM(pin), im);
	MVSOCGPP_WRITE(sc, MVSOCGPP_GPIOILM(pin), ilm);
}


/*
 * gpio(4) functions, and can call you.
 */

/* ARGSUSED */
int
mvsocgpp_pin_read(void *arg, int pin)
{
	struct mvsocgpp_softc *sc = mvsocgpp_softc;
	uint32_t val;

	KASSERT(sc != NULL);

	val = MVSOCGPP_READ(sc, MVSOCGPP_GPIODI(pin));
	return (val & MVSOCGPP_GPIOPIN(pin)) != 0;
}

/* ARGSUSED */
void
mvsocgpp_pin_write(void *arg, int pin, int value)
{
	struct mvsocgpp_softc *sc = mvsocgpp_softc;
	uint32_t old, new, mask = MVSOCGPP_GPIOPIN(pin);

	KASSERT(sc != NULL);

	old = MVSOCGPP_READ(sc, MVSOCGPP_GPIODO(pin));
	if (value)
		new = old | mask;
	else
		new = old & ~mask;
	if (new != old)
		MVSOCGPP_WRITE(sc, MVSOCGPP_GPIODO(pin), new);
}

/* ARGSUSED */
void
mvsocgpp_pin_ctl(void *arg, int pin, int flags)
{
	struct mvsocgpp_softc *sc = mvsocgpp_softc;
	uint32_t old, new, mask = MVSOCGPP_GPIOPIN(pin);

	KASSERT(sc != NULL);

	old = MVSOCGPP_READ(sc, MVSOCGPP_GPIODOEC(pin));
	switch (flags & (GPIO_PIN_INPUT|GPIO_PIN_OUTPUT)) {
	case GPIO_PIN_INPUT:
		new = old | mask;
		break;

	case GPIO_PIN_OUTPUT:
		new = old & ~mask;
		break;

	default:
		return;
	}
	if (new != old)
		MVSOCGPP_WRITE(sc, MVSOCGPP_GPIODOEC(pin), new);

	/* Blink every 2^24 TCLK */
	old = MVSOCGPP_READ(sc, MVSOCGPP_GPIOBE(pin));
	if (flags & GPIO_PIN_PULSATE)
		new = old | mask;
	else
		new = old & ~mask;
	if (new != old)
		MVSOCGPP_WRITE(sc, MVSOCGPP_GPIOBE(pin), new);

	old = MVSOCGPP_READ(sc, MVSOCGPP_GPIODIP(pin));
	if (flags & GPIO_PIN_INVIN)
		new = old | mask;
	else
		new = old & ~mask;
	if (new != old)
		MVSOCGPP_WRITE(sc, MVSOCGPP_GPIODIP(pin), new);
}


#ifdef MVSOCGPP_DUMPREG
static void
mvsocgpp_dump_reg(struct mvsocgpp_softc *sc)
{

	aprint_normal_dev(sc->sc_dev, "  Data Out:                 \t0x%08x\n",
	    MVSOCGPP_READ(sc, MVSOCGPP_GPIODO(0)));
	aprint_normal_dev(sc->sc_dev, "  Data Out Enable Control:  \t0x%08x\n",
	    MVSOCGPP_READ(sc, MVSOCGPP_GPIODOEC(0)));
	aprint_normal_dev(sc->sc_dev, "  Data Blink Enable:        \t0x%08x\n",
	    MVSOCGPP_READ(sc, MVSOCGPP_GPIOBE(0)));
	aprint_normal_dev(sc->sc_dev, "  Data In Polarity:         \t0x%08x\n",
	    MVSOCGPP_READ(sc, MVSOCGPP_GPIODIP(0)));
	aprint_normal_dev(sc->sc_dev, "  Data In:                  \t0x%08x\n",
	    MVSOCGPP_READ(sc, MVSOCGPP_GPIODI(0)));
	aprint_normal_dev(sc->sc_dev, "  Interrupt Cause:          \t0x%08x\n",
	    MVSOCGPP_READ(sc, MVSOCGPP_GPIOIC(0)));
	aprint_normal_dev(sc->sc_dev, "  Interrupt Mask:           \t0x%08x\n",
	    MVSOCGPP_READ(sc, MVSOCGPP_GPIOIM(0)));
	aprint_normal_dev(sc->sc_dev, "  Interrupt Level Mask:     \t0x%08x\n",
	    MVSOCGPP_READ(sc, MVSOCGPP_GPIOILM(0)));

	if (gpp_npins <= 32)
		return;

	aprint_normal_dev(sc->sc_dev, "  High Data Out:            \t0x%08x\n",
	    MVSOCGPP_READ(sc, MVSOCGPP_GPIODO(32)));
	aprint_normal_dev(sc->sc_dev, "  High Data Out Enable Ctrl:\t0x%08x\n",
	    MVSOCGPP_READ(sc, MVSOCGPP_GPIODOEC(32)));
	aprint_normal_dev(sc->sc_dev, "  High Blink Enable:        \t0x%08x\n",
	    MVSOCGPP_READ(sc, MVSOCGPP_GPIOBE(32)));
	aprint_normal_dev(sc->sc_dev, "  High Data In Polarity:    \t0x%08x\n",
	    MVSOCGPP_READ(sc, MVSOCGPP_GPIODIP(32)));
	aprint_normal_dev(sc->sc_dev, "  High Data In:             \t0x%08x\n",
	    MVSOCGPP_READ(sc, MVSOCGPP_GPIODI(32)));
	aprint_normal_dev(sc->sc_dev, "  High Interrupt Cause:     \t0x%08x\n",
	    MVSOCGPP_READ(sc, MVSOCGPP_GPIOIC(32)));
	aprint_normal_dev(sc->sc_dev, "  High Interrupt Mask:      \t0x%08x\n",
	    MVSOCGPP_READ(sc, MVSOCGPP_GPIOIM(32)));
	aprint_normal_dev(sc->sc_dev, "  High Interrupt Level Mask:\t0x%08x\n",
	    MVSOCGPP_READ(sc, MVSOCGPP_GPIOILM(32)));
}
#endif

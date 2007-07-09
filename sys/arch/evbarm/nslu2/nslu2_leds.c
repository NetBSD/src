/*	$NetBSD: nslu2_leds.c,v 1.6 2007/07/09 20:52:11 ad Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: nslu2_leds.c,v 1.6 2007/07/09 20:52:11 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/proc.h>

#include <machine/intr.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbcdc.h>
#include <dev/usb/usbdi.h>		/* XXX: For IPL_USB */

#include <arm/xscale/ixp425var.h>

#include <evbarm/nslu2/nslu2reg.h>

#define	SLUGLED_FLASH_LEN	(hz/8)	/* How many ticks an LED stays lit */

#define	LEDBITS_USB0		(1u << GPIO_LED_DISK1)
#define	LEDBITS_USB1		(1u << GPIO_LED_DISK2)

/*
 * The Ready/Status bits control a tricolour LED.
 * Ready is green, status is red.
 */
#define	LEDBITS_READY		(1u << GPIO_LED_READY)
#define	LEDBITS_STATUS		(1u << GPIO_LED_STATUS)

struct slugled_softc {
	struct device sc_dev;
	void *sc_tmr_ih;
	struct callout sc_usb0;
	void *sc_usb0_ih;
	struct callout sc_usb1;
	void *sc_usb1_ih;
	struct callout sc_usb2;
	void *sc_usb2_ih;
};

static int slugled_attached;

static void
slugled_callout(void *arg)
{
	uint32_t reg, bit;
	int is;

	bit = (uint32_t)(uintptr_t)arg;

	is = disable_interrupts(I32_bit);
	reg = GPIO_CONF_READ_4(ixp425_softc, IXP425_GPIO_GPOUTR);
	GPIO_CONF_WRITE_4(ixp425_softc, IXP425_GPIO_GPOUTR, reg | bit);
	restore_interrupts(is);
}

static int
slugled_intr0(void *arg)
{
	struct slugled_softc *sc = arg;
	uint32_t reg;
	int is;

	is = disable_interrupts(I32_bit);
	reg = GPIO_CONF_READ_4(ixp425_softc, IXP425_GPIO_GPOUTR);
	reg &= ~LEDBITS_USB0;
	GPIO_CONF_WRITE_4(ixp425_softc, IXP425_GPIO_GPOUTR, reg);
	restore_interrupts(is);

	callout_schedule(&sc->sc_usb0, SLUGLED_FLASH_LEN);

	return (1);
}

static int
slugled_intr1(void *arg)
{
	struct slugled_softc *sc = arg;
	uint32_t reg;
	int is;

	is = disable_interrupts(I32_bit);
	reg = GPIO_CONF_READ_4(ixp425_softc, IXP425_GPIO_GPOUTR);
	reg &= ~LEDBITS_USB1;
	GPIO_CONF_WRITE_4(ixp425_softc, IXP425_GPIO_GPOUTR, reg);
	restore_interrupts(is);

	callout_schedule(&sc->sc_usb1, SLUGLED_FLASH_LEN);

	return (1);
}

static int
slugled_intr2(void *arg)
{
	struct slugled_softc *sc = arg;
	uint32_t reg;
	int is;

	is = disable_interrupts(I32_bit);
	reg = GPIO_CONF_READ_4(ixp425_softc, IXP425_GPIO_GPOUTR);
	reg &= ~(LEDBITS_USB0 | LEDBITS_USB1);
	GPIO_CONF_WRITE_4(ixp425_softc, IXP425_GPIO_GPOUTR, reg);
	restore_interrupts(is);

	callout_schedule(&sc->sc_usb2, SLUGLED_FLASH_LEN);

	return (1);
}

static int
slugled_tmr(void *arg)
{
	struct clockframe *frame = arg;
	uint32_t reg, bit;
	int is;

	if (CLKF_INTR(frame) || sched_curcpu_runnable_p() ||
	    (curlwp != NULL && curlwp != curcpu()->ci_data.cpu_idlelwp))
		bit = LEDBITS_STATUS;
	else
		bit = 0;

	is = disable_interrupts(I32_bit);
	reg = GPIO_CONF_READ_4(ixp425_softc, IXP425_GPIO_GPOUTR);
	reg &= ~LEDBITS_STATUS;
	GPIO_CONF_WRITE_4(ixp425_softc, IXP425_GPIO_GPOUTR, reg | bit);
	restore_interrupts(is);

	return (1);
}

static void
slugled_shutdown(void *arg)
{
	struct slugled_softc *sc = arg;
	uint32_t reg;
	int s;

	ixp425_intr_disestablish(sc->sc_usb0_ih);
	ixp425_intr_disestablish(sc->sc_usb1_ih);
	ixp425_intr_disestablish(sc->sc_tmr_ih);

	/* Cancel the callouts */
	s = splsoftclock();
	callout_stop(&sc->sc_usb0);
	callout_stop(&sc->sc_usb1);
	splx(s);

	/* Turn off the disk LEDs, and set Ready/Status to amber */
	s = splhigh();
	reg = GPIO_CONF_READ_4(ixp425_softc,IXP425_GPIO_GPOUTR);
	reg |= LEDBITS_USB0 | LEDBITS_USB1 | LEDBITS_STATUS | LEDBITS_READY;
	GPIO_CONF_WRITE_4(ixp425_softc,IXP425_GPIO_GPOUTR, reg);
	splx(s);
}

static void
slugled_defer(struct device *self)
{
	struct slugled_softc *sc = (struct slugled_softc *) self;
	struct ixp425_softc *ixsc = ixp425_softc;
	uint32_t reg;
	int s;

	s = splhigh();

	/* Configure LED GPIO pins as output */
	reg = GPIO_CONF_READ_4(ixsc, IXP425_GPIO_GPOER);
	reg &= ~(LEDBITS_USB0 | LEDBITS_USB1);
	reg &= ~(LEDBITS_READY | LEDBITS_STATUS);
	GPIO_CONF_WRITE_4(ixsc, IXP425_GPIO_GPOER, reg);

	/* All LEDs off */
	reg = GPIO_CONF_READ_4(ixsc, IXP425_GPIO_GPOUTR);
	reg |= LEDBITS_USB0 | LEDBITS_USB1;
	reg &= ~(LEDBITS_STATUS | LEDBITS_READY);
	GPIO_CONF_WRITE_4(ixsc, IXP425_GPIO_GPOUTR, reg);

	splx(s);

	if (shutdownhook_establish(slugled_shutdown, sc) == NULL)
		aprint_error("%s: WARNING - Failed to register shutdown hook\n",
		    sc->sc_dev.dv_xname);

	callout_init(&sc->sc_usb0, 0);
	callout_setfunc(&sc->sc_usb0, slugled_callout,
	    (void *)(uintptr_t)LEDBITS_USB0);

	callout_init(&sc->sc_usb1, 0);
	callout_setfunc(&sc->sc_usb1, slugled_callout,
	    (void *)(uintptr_t)LEDBITS_USB1);

	callout_init(&sc->sc_usb2, 0);
	callout_setfunc(&sc->sc_usb2, slugled_callout,
	    (void *)(uintptr_t)(LEDBITS_USB0 | LEDBITS_USB1));

	sc->sc_usb0_ih = ixp425_intr_establish(PCI_INT_A, IPL_USB,
	    slugled_intr0, sc);
	KDASSERT(sc->sc_usb0_ih != NULL);
	sc->sc_usb1_ih = ixp425_intr_establish(PCI_INT_B, IPL_USB,
	    slugled_intr1, sc);
	KDASSERT(sc->sc_usb1_ih != NULL);
	sc->sc_usb2_ih = ixp425_intr_establish(PCI_INT_C, IPL_USB,
	    slugled_intr2, sc);
	KDASSERT(sc->sc_usb2_ih != NULL);

	sc->sc_tmr_ih = ixp425_intr_establish(IXP425_INT_TMR0, IPL_CLOCK,
	    slugled_tmr, NULL);
	KDASSERT(sc->sc_tmr_ih != NULL);
}

static int
slugled_match(struct device *parent, struct cfdata *match, void *aux)
{

	return (slugled_attached == 0);
}

static void
slugled_attach(struct device *parent, struct device *self, void *aux)
{

	aprint_normal(": LED support\n");

	slugled_attached = 1;

	config_interrupts(self, slugled_defer);
}

CFATTACH_DECL(slugled, sizeof(struct slugled_softc),
    slugled_match, slugled_attach, NULL, NULL);

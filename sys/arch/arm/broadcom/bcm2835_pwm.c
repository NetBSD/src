/*	$NetBSD: bcm2835_pwm.c,v 1.2.16.2 2017/12/03 11:35:52 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael van Elst
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
 * Driver for BCM2835 Pulse Width Modulator
 *
 * Each channel can be allocated and used individually, but
 * for FIFO usage, both channels must to be requested.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcm2835_pwm.c,v 1.2.16.2 2017/12/03 11:35:52 jdolecek Exp $");

#include "bcmdmac.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/atomic.h>
#include <sys/intr.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm_amba.h>

#include <arm/broadcom/bcm2835_pwm.h>

struct bcm_pwm_channel {
	struct bcm2835pwm_softc *sc;
	uint32_t ctlmask, stamask, gapomask;
	int rng, dat;
	bool inuse;
	uint32_t ctlsave, rngsave, datsave;
};

struct bcm2835pwm_softc {
	device_t		sc_dev;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_iob;

	int			sc_clockrate;
	struct bcm_pwm_channel	sc_channels[2];
	kmutex_t		sc_lock;
};

#define PWM_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define PWM_READ(sc, reg) \
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))

static int bcmpwm_match(device_t, cfdata_t, void *);
static void bcmpwm_attach(device_t, device_t, void *);
static int bcmpwm_wait(struct bcm2835pwm_softc *);

CFATTACH_DECL_NEW(bcmpwm_amba, sizeof(struct bcm2835pwm_softc),
    bcmpwm_match, bcmpwm_attach, NULL, NULL);

/* ARGSUSED */
static int
bcmpwm_match(device_t parent, cfdata_t match, void *aux)
{
	struct amba_attach_args *aaa = aux;

	if (strcmp(aaa->aaa_name, "bcmpwm") != 0)
		return 0;

	if (aaa->aaa_addr != BCM2835_PWM_BASE)
		return 0;

	return 1;
}

static void
bcmpwm_attach(device_t parent, device_t self, void *aux)
{
	struct bcm2835pwm_softc *sc = device_private(self);
 	struct amba_attach_args *aaa = aux;
	const prop_dictionary_t cfg = device_properties(self);

	aprint_naive("\n");
	aprint_normal(": PWM\n");

	sc->sc_dev = self;
	sc->sc_iot = aaa->aaa_iot;
	sc->sc_iob = aaa->aaa_addr;

	if (bus_space_map(aaa->aaa_iot, aaa->aaa_addr, BCM2835_PWM_SIZE, 0,
	    &sc->sc_ioh)) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		goto fail0;
	}

	prop_dictionary_get_uint32(cfg, "pwmclockrate", &sc->sc_clockrate);

	sc->sc_channels[0].sc = sc;
	sc->sc_channels[0].ctlmask = PWM_CTL_MSEN1 | PWM_CTL_USEF1 |
				     PWM_CTL_POLA1 | PWM_CTL_SBIT1 |
				     PWM_CTL_RPTL1 | PWM_CTL_MODE1 |
				     PWM_CTL_PWEN1;
	sc->sc_channels[0].stamask = PWM_STA_STA1;
	sc->sc_channels[0].gapomask = PWM_STA_GAPO1;
	sc->sc_channels[0].rng = PWM_RNG1;
	sc->sc_channels[0].dat = PWM_DAT1;

	sc->sc_channels[1].sc = sc;
	sc->sc_channels[1].ctlmask = PWM_CTL_MSEN2 | PWM_CTL_USEF2 |
				     PWM_CTL_POLA2 | PWM_CTL_SBIT2 |
				     PWM_CTL_RPTL2 | PWM_CTL_MODE2 |
				     PWM_CTL_PWEN2;
	sc->sc_channels[1].stamask = PWM_STA_STA2;
	sc->sc_channels[1].gapomask = PWM_STA_GAPO2;
	sc->sc_channels[1].rng = PWM_RNG2;
	sc->sc_channels[1].dat = PWM_DAT2;

	/* The PWM hardware can be used by vcaudio if the
	 * analog output is selected
	 */
	sc->sc_channels[0].inuse = false;
	sc->sc_channels[1].inuse = false;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	/* Success!  */

fail0:	return;
}

struct bcm_pwm_channel *
bcm_pwm_alloc(int num)
{
	struct bcm2835pwm_softc *sc;
	device_t dev;
	struct bcm_pwm_channel *pwm;

	dev = device_find_by_driver_unit("bcmpwm", 0);
	if (dev == NULL)
		return NULL;
	sc = device_private(dev);

	if (num < 0 || num >= __arraycount(sc->sc_channels))
		return NULL;

	pwm = &sc->sc_channels[num];

	mutex_enter(&sc->sc_lock);
	if (pwm->inuse)
		pwm = NULL;
	else
		pwm->inuse = true;
	mutex_exit(&sc->sc_lock);

	if (pwm) {
		pwm->datsave = PWM_READ(pwm->sc, pwm->dat);
		pwm->ctlsave = PWM_READ(pwm->sc, PWM_CTL);
		pwm->rngsave = PWM_READ(pwm->sc, pwm->rng);
	}

	return pwm;
}

void
bcm_pwm_free(struct bcm_pwm_channel *pwm)
{
	struct bcm2835pwm_softc *sc = pwm->sc;

	KASSERT(pwm->inuse);

	PWM_WRITE(pwm->sc, pwm->rng, pwm->rngsave);
	PWM_WRITE(pwm->sc, PWM_CTL, pwm->ctlsave & ~PWM_CTL_WRITEZERO);
	PWM_WRITE(pwm->sc, pwm->dat, pwm->datsave);

	mutex_enter(&sc->sc_lock);
	pwm->inuse = false;
	mutex_exit(&sc->sc_lock);
}

void
bcm_pwm_control(struct bcm_pwm_channel *pwm, uint32_t ctl, uint32_t rng)
{
	struct bcm2835pwm_softc *sc = pwm->sc;
	uint32_t w;

	KASSERT(pwm->inuse);

	/* set control bits like for channel 0
	 * there are generic bit definitions that the caller can use.
	 */
	w = PWM_READ(pwm->sc, PWM_CTL);
	ctl = (w & ~pwm->ctlmask) | __SHIFTIN(ctl, pwm->ctlmask);

	/* when FIFO usage gets enabled but wasn't clear the FIFO */
	if ((w & (PWM_CTL_USEF1|PWM_CTL_USEF2)) == 0 &&
	    (ctl & (PWM_CTL_USEF1|PWM_CTL_USEF2)) != 0)
		ctl |= PWM_CTL_CLRF1;

	PWM_WRITE(sc, pwm->rng, rng);
	PWM_WRITE(sc, PWM_CTL, ctl & ~PWM_CTL_WRITEZERO);
}

uint32_t
bcm_pwm_status(struct bcm_pwm_channel *pwm)
{
	uint32_t w;
	uint32_t common = PWM_STA_BERR | PWM_STA_RERR1 |
			  PWM_STA_WERR1 | PWM_STA_EMPT1 | PWM_STA_FULL1;

	/* return status bits like for channel 0
	 * there are generic bit definitions that the caller can use.
	 *
	 * The BERR bit is returned for both channels.
	 */
	w = PWM_READ(pwm->sc, PWM_STA);
	PWM_WRITE(pwm->sc, PWM_STA, w &
		(pwm->stamask | pwm->gapomask | common));

	w = __SHIFTIN(__SHIFTOUT(w, pwm->stamask), PWM_STA_STA)
	  | __SHIFTIN(__SHIFTOUT(w, pwm->gapomask), PWM_STA_GAPO)
	  | (w & common);

	return w;
}

static int
bcmpwm_wait(struct bcm2835pwm_softc *sc)
{
	int i;
	uint32_t s;

	for (i=0; i<1000; ++i) {
		s = PWM_READ(sc, PWM_STA);
		if ((s & PWM_STA_FULL1) == 0)
			break;
		delay(1);
	}
	if (i >= 1000)
		return -1;

	return 0;
}

int
bcm_pwm_write(struct bcm_pwm_channel *pwm, uint32_t *data1, uint32_t *data2,
    int len)
{
	struct bcm2835pwm_softc *sc = pwm->sc;
	int n;
	uint32_t r;
	bool even = false;

	KASSERT(pwm->inuse);

	n = len;
	while (n > 0) {
		if (bcmpwm_wait(sc))
			break;
		r = even ? *data2++ : *data1++;
		PWM_WRITE(sc, PWM_FIFO, r);
		if (data2 != NULL)
			even = !even;
		--n;
	}

	return len - n;
}

void
bcm_pwm_set(struct bcm_pwm_channel *pwm, uint32_t w)
{
	struct bcm2835pwm_softc *sc = pwm->sc;

	PWM_WRITE(sc, pwm->dat, w);
}

int
bcm_pwm_flush(struct bcm_pwm_channel *pwm)
{
	struct bcm2835pwm_softc *sc = pwm->sc;

	return bcmpwm_wait(sc) ? EIO : 0;
}

void
bcm_pwm_dma_enable(struct bcm_pwm_channel *pwm, bool enable)
{
	struct bcm2835pwm_softc *sc = pwm->sc;
	uint32_t w;

#if 0
	w = PWM_READ(sc, PWM_DMAC);
	if (enable)
		w |= PWM_DMAC_ENAB;
	else
		w &= ~PWM_DMAC_ENAB;
#else
	w = (enable ? PWM_DMAC_ENAB : 0)
	  | __SHIFTIN(7, PWM_DMAC_PANIC)
	  | __SHIFTIN(7, PWM_DMAC_DREQ);
#endif
	PWM_WRITE(sc, PWM_DMAC, w & ~PWM_DMAC_WRITEZERO);
}

uint32_t
bcm_pwm_dma_address(struct bcm_pwm_channel *pwm)
{
	struct bcm2835pwm_softc *sc = pwm->sc;

	return sc->sc_iob + PWM_FIFO;
}


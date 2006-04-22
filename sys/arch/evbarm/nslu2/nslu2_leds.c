/*	$NetBSD: nslu2_leds.c,v 1.3.2.2 2006/04/22 11:37:24 simonb Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: nslu2_leds.c,v 1.3.2.2 2006/04/22 11:37:24 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/disk.h>

#include <arm/xscale/ixp425var.h>

#include <evbarm/nslu2/nslu2reg.h>

#define	SLUGLED_NLEDS		2
#define	SLUGLED_DISKNAMES	"sd%d"	/* XXX: Make this configurable! */

#define	SLUGLED_POLL		(hz/10)	/* Poll disks 10 times per second */
#define	SLUGLED_FLASH_LEN	2	/* How long, in terms of SLUGLED_POLL,
					   to light up an LED */
#define	SLUGLED_READY_FLASH	3	/* Ditto for flashing Ready LED */

#define	LEDBITS_DISK0		(1u << GPIO_LED_DISK1)
#define	LEDBITS_DISK1		(1u << GPIO_LED_DISK2)

/*
 * The Ready/Status bits control a tricolour LED.
 * Ready is green, status is red.
 */
#define	LEDBITS_READY		(1u << GPIO_LED_READY)
#define	LEDBITS_STATUS		(1u << GPIO_LED_STATUS)

struct slugled_softc {
	struct device sc_dev;
	struct callout sc_co;
	struct {
		char sc_dk_name[DK_DISKNAMELEN];
		struct timeval sc_dk_time;
		int sc_dk_flash;	/* Countdown to LED clear */
	} sc_dk[SLUGLED_NLEDS];
	uint32_t sc_state;
	int sc_count;
};

static int slugled_attached;

static void
slugled_callout(void *arg)
{
	struct slugled_softc *sc = arg;
	struct timeval t;
	struct disk *dk;
	uint32_t reg, bit, new_state;
	int s, i, dkbusy;

	new_state = sc->sc_state;

	for (i = 0; i < SLUGLED_NLEDS; i++) {
		bit = i ? LEDBITS_DISK1 : LEDBITS_DISK0;

		s = splbio();
		if ((dk = disk_find(sc->sc_dk[i].sc_dk_name)) == NULL) {
			splx(s);
			if (sc->sc_dk[i].sc_dk_flash > 0) {
				new_state |= bit;
				sc->sc_dk[i].sc_dk_flash = 0;
				sc->sc_dk[i].sc_dk_time = mono_time;
			}

			continue;
		}

		dkbusy = dk->dk_stats->busy;
		t = dk->dk_timestamp;
		splx(s);

		if (dkbusy || t.tv_sec != sc->sc_dk[i].sc_dk_time.tv_sec ||
		    t.tv_usec != sc->sc_dk[i].sc_dk_time.tv_usec) {
			sc->sc_dk[i].sc_dk_flash = SLUGLED_FLASH_LEN;
			sc->sc_dk[i].sc_dk_time = t;
			new_state &= ~bit;
		} else
		if (sc->sc_dk[i].sc_dk_flash > 0 &&
		    --(sc->sc_dk[i].sc_dk_flash) == 0)
			new_state |= bit;
	}

	if ((++(sc->sc_count) % SLUGLED_READY_FLASH) == 0)
		new_state ^= LEDBITS_READY;

	if (sc->sc_state != new_state) {
		sc->sc_state = new_state;
		s = splhigh();
		reg = GPIO_CONF_READ_4(ixp425_softc, IXP425_GPIO_GPOUTR);
		reg &= ~(LEDBITS_DISK0 | LEDBITS_DISK1 | LEDBITS_READY);
		reg |= new_state;
		GPIO_CONF_WRITE_4(ixp425_softc, IXP425_GPIO_GPOUTR, reg);
		splx(s);
	}

	callout_schedule(&sc->sc_co, SLUGLED_POLL);
}

static void
slugled_shutdown(void *arg)
{
	struct slugled_softc *sc = arg;
	uint32_t reg;
	int s;

	/* Cancel the disk activity callout */
	s = splsoftclock();
	callout_stop(&sc->sc_co);
	splx(s);

	/* Turn off the disk LEDs, and set Ready/Status to red */
	s = splhigh();
	reg = GPIO_CONF_READ_4(ixp425_softc,IXP425_GPIO_GPOUTR);
	reg &= ~LEDBITS_READY;
	reg |= LEDBITS_DISK0 | LEDBITS_DISK1 | LEDBITS_STATUS;
	GPIO_CONF_WRITE_4(ixp425_softc,IXP425_GPIO_GPOUTR, reg);
	splx(s);
}

static void
slugled_defer(struct device *self)
{
	struct slugled_softc *sc = (struct slugled_softc *) self;
	struct ixp425_softc *ixsc = ixp425_softc;
	uint32_t reg;
	int i, s;

	s = splhigh();

	/* Configure LED GPIO pins as output */
	reg = GPIO_CONF_READ_4(ixsc, IXP425_GPIO_GPOER);
	reg &= ~(LEDBITS_DISK0 | LEDBITS_DISK1);
	reg &= ~(LEDBITS_READY | LEDBITS_STATUS);
	GPIO_CONF_WRITE_4(ixsc, IXP425_GPIO_GPOER, reg);

	/* All LEDs off, except Ready LED */
	reg = GPIO_CONF_READ_4(ixsc, IXP425_GPIO_GPOUTR);
	reg |= LEDBITS_DISK0 | LEDBITS_DISK1 | LEDBITS_READY;
	reg &= ~LEDBITS_STATUS;
	GPIO_CONF_WRITE_4(ixsc, IXP425_GPIO_GPOUTR, reg);

	splx(s);

	for (i = 0; i < SLUGLED_NLEDS; i++) {
		sprintf(sc->sc_dk[i].sc_dk_name, SLUGLED_DISKNAMES, i);
		sc->sc_dk[i].sc_dk_flash = 0;
		sc->sc_dk[i].sc_dk_time = mono_time;
	}

	sc->sc_state = LEDBITS_DISK0 | LEDBITS_DISK1 | LEDBITS_READY;
	sc->sc_count = 0;

	if (shutdownhook_establish(slugled_shutdown, sc) == NULL)
		aprint_error("%s: WARNING - Failed to register shutdown hook\n",
		    sc->sc_dev.dv_xname);

	callout_init(&sc->sc_co);
	callout_setfunc(&sc->sc_co, slugled_callout, sc);
	callout_schedule(&sc->sc_co, SLUGLED_POLL);
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

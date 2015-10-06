/*	$NetBSD: auxio.c,v 1.25 2015/10/06 16:40:36 martin Exp $	*/

/*
 * Copyright (c) 2000, 2001, 2015 Matthew R. Green
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * AUXIO registers support on the sbus & ebus2, used for the floppy driver
 * and to control the system LED, for the BLINK option.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: auxio.c,v 1.25 2015/10/06 16:40:36 martin Exp $");

#include "opt_auxio.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/callout.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>
#include <dev/sbus/sbusvar.h>
#include <sparc64/dev/auxioreg.h>
#include <sparc64/dev/auxiovar.h>

static uint32_t	auxio_read_led(struct auxio_softc *);
static void	auxio_write_led(struct auxio_softc *, uint32_t);
void	auxio_attach_common(struct auxio_softc *);

extern struct cfdriver auxio_cd;

static __inline__ uint32_t
auxio_read_led(struct auxio_softc *sc)
{
	uint32_t led;

	if (sc->sc_flags & AUXIO_EBUS)
		led = le32toh(bus_space_read_4(sc->sc_tag, sc->sc_led, 0));
	else
		led = bus_space_read_1(sc->sc_tag, sc->sc_led, 0);

	return led;
}

static __inline__ void
auxio_write_led(struct auxio_softc *sc, uint32_t led)
{

	if (sc->sc_flags & AUXIO_EBUS)
		bus_space_write_4(sc->sc_tag, sc->sc_led, 0, htole32(led));
	else
		bus_space_write_1(sc->sc_tag, sc->sc_led, 0, led);
}

#ifdef BLINK
static callout_t blink_ch;
static void auxio_blink(void *);

/* let someone disable it if it's already turned on; XXX sysctl? */
int do_blink = 1;

static void
auxio_blink(void *x)
{
	struct auxio_softc *sc = x;
	int s;
	uint32_t led;

	if (do_blink == 0)
		return;

	mutex_enter(&sc->sc_lock);
	led = auxio_read_led(sc);
	led = led ^ AUXIO_LED_LED;
	auxio_write_led(sc, led);
	mutex_exit(&sc->sc_lock);

	/*
	 * Blink rate is:
	 *	full cycle every second if completely idle (loadav = 0)
	 *	full cycle every 2 seconds if loadav = 1
	 *	full cycle every 3 seconds if loadav = 2
	 * etc.
	 */
	s = (((averunnable.ldavg[0] + FSCALE) * hz) >> (FSHIFT + 1));
	callout_reset(&blink_ch, s, auxio_blink, sc);
}
#endif

void
auxio_attach_common(struct auxio_softc *sc)
{
#ifdef BLINK
	static int do_once = 1;

	/* only start one blinker */
	if (do_once) {
		mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_HIGH);
		callout_init(&blink_ch, CALLOUT_MPSAFE);
		auxio_blink(sc);
		do_once = 0;
	}
#endif
	printf("\n");
}

int
auxio_fd_control(u_int32_t bits)
{
	struct auxio_softc *sc;
	u_int32_t led;

	if (auxio_cd.cd_ndevs == 0) {
		return ENXIO;
	}

	/*
	 * XXX This does not handle > 1 auxio correctly.
	 * We'll assume the floppy drive is tied to first auxio found.
	 */
	sc = device_lookup_private(&auxio_cd, 0);
	if (!sc) {
		return ENXIO;
	}

	mutex_enter(&sc->sc_lock);
	led = auxio_read_led(sc);
	led = (led & ~AUXIO_LED_FLOPPY_MASK) | bits;
	auxio_write_led(sc, led);
	mutex_exit(&sc->sc_lock);

	return 0;
}

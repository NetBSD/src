/* $NetBSD: tskp.c,v 1.6.14.1 2009/05/13 17:16:39 jym Exp $ */

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is from software contributed to The NetBSD Foundation
 * by Jesse Off.
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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tskp.c,v 1.6.14.1 2009/05/13 17:16:39 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/poll.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/select.h>

#include <machine/bus.h>
#include <machine/autoconf.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <arm/ep93xx/ep93xxreg.h>
#include <arm/ep93xx/epgpioreg.h>
#include <dev/ic/matrixkpvar.h>
#include <evbarm/tsarm/tspldvar.h>
#include <evbarm/tsarm/tsarmreg.h>

struct tskp_softc {
	struct device sc_dev;
	struct matrixkp_softc sc_mxkp;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_gpioh;
};

#define KC(n) KS_KEYCODE(n)
static const keysym_t mxkp_keydesc_default[] = {
/*  pos				normal			shifted		*/
	KC(0), 			KS_1,
	KC(1), 			KS_2,
	KC(2), 			KS_3,
	KC(3), 			KS_A,
	KC(4), 			KS_4,
	KC(5), 			KS_5,
	KC(6), 			KS_6,
	KC(7), 			KS_B,
	KC(8), 			KS_7,
	KC(9),	 		KS_8,
	KC(10), 		KS_9,
	KC(11), 		KS_C,
	KC(12), 		KS_asterisk,
	KC(13), 		KS_0,
	KC(14), 		KS_numbersign,
	KC(15), 		KS_D,
};
#undef KC
#define KBD_MAP(name, base, map) \
			{ name, base, sizeof(map)/sizeof(keysym_t), map }
const struct wscons_keydesc mxkp_keydesctab[] = {
	KBD_MAP(KB_US,		0,	mxkp_keydesc_default),
	{0, 0, 0, 0}
};
#undef KBD_MAP

struct wskbd_mapdata mxkp_keymapdata = {
	mxkp_keydesctab,
	KB_US,
};

static int	tskp_match(struct device *, struct cfdata *, void *);
static void	tskp_attach(struct device *, struct device *, void *);
static void	tskp_scankeys(struct matrixkp_softc *, u_int32_t *);

CFATTACH_DECL(tskp, sizeof(struct tskp_softc),
    tskp_match, tskp_attach, NULL, NULL);

static int
tskp_match(struct device *parent, struct cfdata *match, void *aux)
{
	return 1;
}

#define GPIO_GET(x)	bus_space_read_1(sc->sc_iot, sc->sc_gpioh, \
	(EP93XX_GPIO_ ## x))

#define GPIO_SET(x, y)	bus_space_write_1(sc->sc_iot, sc->sc_gpioh, \
	(EP93XX_GPIO_ ## x), (y))

#define GPIO_SETBITS(x, y)	bus_space_write_1(sc->sc_iot, sc->sc_gpioh, \
	(EP93XX_GPIO_ ## x), GPIO_GET(x) | (y))

#define GPIO_CLEARBITS(x, y)	bus_space_write_1(sc->sc_iot, sc->sc_gpioh, \
	(EP93XX_GPIO_ ## x), GPIO_GET(x) & (~(y)))

static void
tskp_attach(struct device *parent, struct device *self, void *aux)
{
	struct tskp_softc *sc = (void *)self;
	struct tspld_attach_args *taa = aux;
	struct wskbddev_attach_args wa;

	sc->sc_iot = taa->ta_iot;
	if (bus_space_map(sc->sc_iot, EP93XX_APB_HWBASE + EP93XX_APB_GPIO,
		EP93XX_APB_GPIO_SIZE, 0, &sc->sc_gpioh))
		panic("tskp_attach: couldn't map GPIO registers");

	sc->sc_mxkp.mxkp_scankeys = tskp_scankeys;
	sc->sc_mxkp.mxkp_event = mxkp_wskbd_event;
	sc->sc_mxkp.mxkp_nkeys = 16; 	/* 4 x 4 matrix keypad */
	sc->sc_mxkp.debounce_stable_ms = 3;
	sc->sc_mxkp.sc_dev = self;
	sc->sc_mxkp.poll_freq = hz;

	GPIO_SET(PBDDR, 0xff);		/* tristate all lines */

	printf(": 4x4 matrix keypad, polling at %d hz\n", hz);

	mxkp_attach(&sc->sc_mxkp);
	wa.console = 0;
	wa.keymap = &mxkp_keymapdata;
	wa.accessops = &mxkp_accessops;
	wa.accesscookie = &sc->sc_mxkp;
	sc->sc_mxkp.sc_wskbddev = config_found(self, &wa, wskbddevprint);
}

static void
tskp_scankeys(struct matrixkp_softc *mxkp_sc, u_int32_t *keys)
{
	struct tskp_softc *sc = (void *)mxkp_sc->sc_dev;
	u_int32_t pos;

	for(pos = 0; pos < 4; pos++) {
		GPIO_SET(PBDDR, (1 << pos));
		GPIO_SET(PBDR, ~(1 << pos));
		delay(1);
		keys[0] |= (~(GPIO_GET(PBDR) >> 4) & 0xf) << (4 * pos);
	}
}

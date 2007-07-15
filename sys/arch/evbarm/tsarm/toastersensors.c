/* $NetBSD: toastersensors.c,v 1.4.6.1 2007/07/15 13:15:50 ad Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: toastersensors.c,v 1.4.6.1 2007/07/15 13:15:50 ad Exp $");

#include <sys/param.h>
#include <sys/sysctl.h>
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

struct toastersensors_softc {
	struct device sc_dev;
	struct matrixkp_softc sc_mxkp;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_gpioh;
	bus_space_handle_t sc_adch;
	u_int32_t toast_down;
	u_int32_t burnlevel_knob;
	u_int32_t cancel_key;
	u_int32_t toast_key;
	u_int32_t frozen_key;
	u_int32_t warm_key;
	u_int32_t bagel_key;
	u_int32_t toast_down_ticks;
	u_int32_t cancel_key_ticks;
	u_int32_t toast_key_ticks;
	u_int32_t frozen_key_ticks;
	u_int32_t warm_key_ticks;
	u_int32_t bagel_key_ticks;
	struct callout poll;
};

#define KC(n) KS_KEYCODE(n)
static const keysym_t mxkp_keydesc_default[] = {
/*  pos				normal			shifted		*/
	KC(0), 			KS_f,
	KC(1), 			KS_b,
	KC(2), 			KS_w,
	KC(3), 			KS_t,
	KC(4), 			KS_c,
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

static int	toastersensors_match(struct device *, struct cfdata *, void *);
static void	toastersensors_attach(struct device *, struct device *, void *);
static void	toastersensors_scankeys(struct matrixkp_softc *, u_int32_t *);
static void	toastersensors_poll(void *);

CFATTACH_DECL(toastersensors, sizeof(struct toastersensors_softc),
    toastersensors_match, toastersensors_attach, NULL, NULL);

static int
toastersensors_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
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
toastersensors_poll(arg)
	void *arg;
{
	struct toastersensors_softc *sc = arg;

	if (sc->cancel_key) sc->cancel_key_ticks++;
	if (sc->toast_key) sc->toast_key_ticks++;
	if (sc->warm_key) sc->warm_key_ticks++;
	if (sc->frozen_key) sc->frozen_key_ticks++;
	if (sc->bagel_key) sc->bagel_key_ticks++;
	if (sc->toast_down) sc->toast_down_ticks++;

	sc->burnlevel_knob = bus_space_read_1(sc->sc_iot, sc->sc_adch, 0) |
		(bus_space_read_1(sc->sc_iot, sc->sc_adch, 1) << 8);
	/* Initiate another conversion */
	bus_space_write_1(sc->sc_iot, sc->sc_adch, 0, 0x40);
	callout_schedule(&sc->poll, 1);
}

static void
toastersensors_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct toastersensors_softc *sc = (void *)self;
	struct tspld_attach_args *taa = aux;
	struct wskbddev_attach_args wa;
        const struct sysctlnode *node, *datnode;
	u_int32_t i;

	sc->sc_iot = taa->ta_iot;
	if (bus_space_map(sc->sc_iot, EP93XX_APB_HWBASE + EP93XX_APB_GPIO,
		EP93XX_APB_GPIO_SIZE, 0, &sc->sc_gpioh))
		panic("toastersensors_attach: couldn't map GPIO registers");

	if (bus_space_map(sc->sc_iot, TS7XXX_IO8_HWBASE + TS7XXX_MAX197ADC,
		2, 0, &sc->sc_adch))
		panic("toastersensors_attach: couldn't map MAX197-ADC registers");

	sc->sc_mxkp.mxkp_scankeys = toastersensors_scankeys;
	sc->sc_mxkp.mxkp_event = mxkp_wskbd_event;
	sc->sc_mxkp.mxkp_nkeys = 5;
	sc->sc_mxkp.debounce_stable_ms = 3;
	sc->sc_mxkp.sc_dev = self;
	sc->sc_mxkp.poll_freq = hz;

	GPIO_CLEARBITS(PBDDR, 0x3f);		/* tristate all lines */

	aprint_normal(": internal toaster sensor inputs\n");
	aprint_normal("%s: using signal DIO_0 for toast down sensor\n", sc->sc_dev.dv_xname);
	aprint_normal("%s: using signals DIO_1-DIO_5 for panel buttons\n", sc->sc_dev.dv_xname);
	aprint_normal("%s: using 12-bit MAX197-ADC channel 0 for burnlevel knob\n", sc->sc_dev.dv_xname);

	if (sysctl_createv(NULL, 0, NULL, NULL,
				CTLFLAG_PERMANENT, CTLTYPE_NODE, "hw",
				NULL, NULL, 0, NULL, 0,
				CTL_HW, CTL_EOL) != 0) {
		printf("%s: could not create sysctl\n",
			sc->sc_dev.dv_xname);
		return;
	}
	if (sysctl_createv(NULL, 0, NULL, &node,
        			0, CTLTYPE_NODE, sc->sc_dev.dv_xname,
        			NULL,
        			NULL, 0, NULL, 0,
				CTL_HW, CTL_CREATE, CTL_EOL) != 0) {
                printf("%s: could not create sysctl\n",
			sc->sc_dev.dv_xname);
		return;
	}


	if ((i = sysctl_createv(NULL, 0, NULL, &datnode,
        			0, CTLTYPE_INT,
				"burnlevel_knob",
        			SYSCTL_DESCR(
				"12-bit analog reading of front-panel potentiometer"),
        			NULL, 0, &sc->burnlevel_knob, 0,
				CTL_HW, node->sysctl_num,
				CTL_CREATE, CTL_EOL))
				!= 0) {
                printf("%s: could not create sysctl\n",
			sc->sc_dev.dv_xname);
		return;
	}

#define KEYSYSCTL_SETUP(x) if ((i = sysctl_createv(NULL, 0, NULL, &datnode,	\
        			0, CTLTYPE_INT,					\
				#x,						\
        			SYSCTL_DESCR(					\
				"toaster user input sensor"),			\
        			NULL, 0, &sc->x, 0,				\
				CTL_HW, node->sysctl_num,			\
				CTL_CREATE, CTL_EOL))				\
				!= 0) {						\
                printf("%s: could not create sysctl\n",				\
			sc->sc_dev.dv_xname);					\
		return;								\
	}									\
										\
	if ((i = sysctl_createv(NULL, 0, NULL, &datnode,			\
        			CTLFLAG_READWRITE, CTLTYPE_INT,			\
				#x "_ticks",					\
        			SYSCTL_DESCR(					\
				"running total of on-time in ticks"),		\
        			NULL, 0, &sc->x ## _ticks, 0,			\
				CTL_HW, node->sysctl_num,			\
				CTL_CREATE, CTL_EOL))				\
				!= 0) {						\
                printf("%s: could not create sysctl\n",				\
			sc->sc_dev.dv_xname);					\
		return;								\
	}									\

	KEYSYSCTL_SETUP(cancel_key);
	KEYSYSCTL_SETUP(toast_key);
	KEYSYSCTL_SETUP(bagel_key);
	KEYSYSCTL_SETUP(warm_key);
	KEYSYSCTL_SETUP(frozen_key);
	KEYSYSCTL_SETUP(toast_down);

	sc->cancel_key_ticks = 0;
	sc->toast_key_ticks = 0;
	sc->frozen_key_ticks = 0;
	sc->warm_key_ticks = 0;
	sc->bagel_key_ticks = 0;
	sc->toast_down_ticks = 0;

	mxkp_attach(&sc->sc_mxkp);
	wa.console = 0;
	wa.keymap = &mxkp_keymapdata;
	wa.accessops = &mxkp_accessops;
	wa.accesscookie = &sc->sc_mxkp;
	sc->sc_mxkp.sc_wskbddev = config_found(self, &wa, wskbddevprint);

	callout_init(&sc->poll, 0);
	callout_setfunc(&sc->poll, toastersensors_poll, sc);
	callout_schedule(&sc->poll, 1);
}

static void
toastersensors_scankeys(mxkp_sc, keys)
	struct matrixkp_softc *mxkp_sc;
	u_int32_t *keys;
{
	struct toastersensors_softc *sc = (void *)mxkp_sc->sc_dev;
	u_int32_t val = GPIO_GET(PBDR) & 0x3f;

	/*
	 * toast_down isn't a key, but we update its state here since its 
         * read in the same reg.
	  */
	sc->toast_down = (val & 0x1) ? 0 : 1;

	*keys = ~(val >> 1);

	sc->cancel_key = (*keys & 0x10) ? 1 : 0;
	sc->toast_key = (*keys & 0x8) ? 1 : 0;
	sc->warm_key = (*keys & 0x4) ? 1 : 0;
	sc->bagel_key = (*keys & 0x2) ? 1 : 0;
	sc->frozen_key = (*keys & 0x1) ? 1 : 0;
}

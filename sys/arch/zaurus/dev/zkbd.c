/*	$NetBSD: zkbd.c,v 1.6 2007/07/21 01:47:58 nonaka Exp $	*/
/* $OpenBSD: zaurus_kbd.c,v 1.28 2005/12/21 20:36:03 deraadt Exp $ */

/*
 * Copyright (c) 2005 Dale Rahn <drahn@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zkbd.c,v 1.6 2007/07/21 01:47:58 nonaka Exp $");

#include "opt_wsdisplay_compat.h"
#include "lcd.h"
#if 0	/* XXX */
#include "apm.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/callout.h>

#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <zaurus/dev/zkbdmap.h>
#include <zaurus/zaurus/zaurus_var.h>

static const int gpio_sense_pins_c3000[] = {
	12,
	17,
	91,
	34,
	36,
	38,
	39,
	-1
};

static const int gpio_strobe_pins_c3000[] = {
	88,
	23,
	24,
	25,
	26,
	27,
	52,
	103,
	107,
	-1,
	108,
	114
};

static const int stuck_keys[] = {
	7,
	15,
	23,
	31
};

#define REP_DELAY1 400
#define REP_DELAYN 100

struct zkbd_softc {
	struct device sc_dev;

	const int *sc_sense_array;
	const int *sc_strobe_array;
	int sc_nsense;
	int sc_nstrobe;

	short sc_onkey_pin;
	short sc_sync_pin;
	short sc_swa_pin;
	short sc_swb_pin;
	char *sc_okeystate;
	char *sc_keystate;
	char sc_hinge;		/* 0=open, 1=nonsense, 2=backwards, 3=closed */
	char sc_maxkbdcol;

	struct callout sc_roll_to;

	/* console stuff */
	int sc_polling;
	int sc_pollUD;
	int sc_pollkey;

	/* wskbd bits */
	struct device *sc_wskbddev;
	int sc_rawkbd;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	const char *sc_xt_keymap;
	struct callout sc_rawrepeat_ch;
#define MAXKEYS 20
	char sc_rep[MAXKEYS];
	int sc_nrep;
#endif
	void *sc_powerhook;
};

static struct zkbd_softc *zkbd_sc;

static int	zkbd_match(struct device *, struct cfdata *, void *);
static void	zkbd_attach(struct device *, struct device *, void *);

CFATTACH_DECL(zkbd, sizeof(struct zkbd_softc),
	zkbd_match, zkbd_attach, NULL, NULL);

static int	zkbd_irq(void *v);
static void	zkbd_poll(void *v);
static int	zkbd_on(void *v);
static int	zkbd_sync(void *v);
static int	zkbd_hinge(void *v);
static void	zkbd_power(int why, void *arg);

int zkbd_modstate;

static int	zkbd_enable(void *, int);
static void	zkbd_set_leds(void *, int);
static int	zkbd_ioctl(void *, u_long, void *, int, struct lwp *);
#ifdef WSDISPLAY_COMPAT_RAWKBD
static void	zkbd_rawrepeat(void *v);
#endif

static struct wskbd_accessops zkbd_accessops = {
	zkbd_enable,
	zkbd_set_leds,
	zkbd_ioctl,
};

static void	zkbd_cngetc(void *, u_int *, int *);
static void	zkbd_cnpollc(void *, int);

static struct wskbd_consops zkbd_consops = {
	zkbd_cngetc,
	zkbd_cnpollc,
};

static struct wskbd_mapdata zkbd_keymapdata = {
	zkbd_keydesctab,
	KB_US,
};

static int
zkbd_match(struct device *parent, struct cfdata *cf, void *aux)
{

	if (zkbd_sc)
		return 0;

	return 1;
}

static void
zkbd_attach(struct device *parent, struct device *self, void *aux)
{
	struct zkbd_softc *sc = (struct zkbd_softc *)self;
	struct wskbddev_attach_args a;
	int pin, i;

	zkbd_sc = sc;

	printf("\n");

	sc->sc_polling = 0;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	sc->sc_rawkbd = 0;
#endif

	callout_init(&sc->sc_roll_to, 0);
	callout_setfunc(&sc->sc_roll_to, zkbd_poll, sc);
#ifdef WSDISPLAY_COMPAT_RAWKBD
	callout_init(&sc->sc_rawrepeat_ch, 0);
	callout_setfunc(&sc->sc_rawrepeat_ch, zkbd_rawrepeat, sc);
#endif

	if (ZAURUS_ISC3000) {
		sc->sc_sense_array = gpio_sense_pins_c3000;
		sc->sc_strobe_array = gpio_strobe_pins_c3000;
		sc->sc_nsense = __arraycount(gpio_sense_pins_c3000);
		sc->sc_nstrobe = __arraycount(gpio_strobe_pins_c3000);
		sc->sc_maxkbdcol = 10;
		sc->sc_onkey_pin = 95;
		sc->sc_sync_pin = 16;
		sc->sc_swa_pin = 97;
		sc->sc_swb_pin = 96;
#ifdef WSDISPLAY_COMPAT_RAWKBD
		sc->sc_xt_keymap = xt_keymap;
#endif
	} else {
		/* XXX */
		return;
	}

	sc->sc_powerhook = powerhook_establish(sc->sc_dev.dv_xname,
	    zkbd_power, sc);
	if (sc->sc_powerhook == NULL) {
		printf("%s: unable to establish powerhook\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_okeystate = malloc(sc->sc_nsense * sc->sc_nstrobe,
	    M_DEVBUF, M_NOWAIT);
	memset(sc->sc_okeystate, 0, sc->sc_nsense * sc->sc_nstrobe);

	sc->sc_keystate = malloc(sc->sc_nsense * sc->sc_nstrobe,
	    M_DEVBUF, M_NOWAIT);
	memset(sc->sc_keystate, 0, sc->sc_nsense * sc->sc_nstrobe);

	/* set all the strobe bits */
	for (i = 0; i < sc->sc_nstrobe; i++) {
		pin = sc->sc_strobe_array[i];
		if (pin == -1)
			continue;
		pxa2x0_gpio_set_function(pin, GPIO_SET|GPIO_OUT);
	}

	/* set all the sense bits */
	for (i = 0; i < sc->sc_nsense; i++) {
		pin = sc->sc_sense_array[i];
		if (pin == -1)
			continue;
		pxa2x0_gpio_set_function(pin, GPIO_IN);
		pxa2x0_gpio_intr_establish(pin, IST_EDGE_BOTH, IPL_TTY,
		    zkbd_irq, sc);
	}

	pxa2x0_gpio_intr_establish(sc->sc_onkey_pin, IST_EDGE_BOTH, IPL_TTY,
	    zkbd_on, sc);
	pxa2x0_gpio_intr_establish(sc->sc_sync_pin, IST_EDGE_RISING, IPL_TTY,
	    zkbd_sync, sc);
	pxa2x0_gpio_intr_establish(sc->sc_swa_pin, IST_EDGE_BOTH, IPL_TTY,
	    zkbd_hinge, sc);
	pxa2x0_gpio_intr_establish(sc->sc_swb_pin, IST_EDGE_BOTH, IPL_TTY,
	    zkbd_hinge, sc);

	if (glass_console) {
		wskbd_cnattach(&zkbd_consops, sc, &zkbd_keymapdata);
		a.console = 1;
	} else {
		a.console = 0;
	}
	a.keymap = &zkbd_keymapdata;
	a.accessops = &zkbd_accessops;
	a.accesscookie = sc;

	zkbd_hinge(sc);		/* to initialize sc_hinge */

	sc->sc_wskbddev = config_found(self, &a, wskbddevprint);
}

#ifdef WSDISPLAY_COMPAT_RAWKBD
static void
zkbd_rawrepeat(void *v)
{
	struct zkbd_softc *sc = (struct zkbd_softc *)v;
	int s;

	s = spltty();
	wskbd_rawinput(sc->sc_wskbddev, sc->sc_rep, sc->sc_nrep);
	splx(s);
	callout_schedule(&sc->sc_rawrepeat_ch, hz * REP_DELAYN / 1000);
}
#endif

/* XXX only deal with keys that can be pressed when display is open? */
/* XXX are some not in the array? */
/* handle keypress interrupt */
static int
zkbd_irq(void *v)
{

	zkbd_poll(v);

	return 1;
}

static void
zkbd_poll(void *v)
{
	struct zkbd_softc *sc = (struct zkbd_softc *)v;
	int i, j, col, pin, type, keysdown = 0;
	int stuck;
	int keystate;
	int s;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	int npress = 0, ncbuf = 0, c;
	char cbuf[MAXKEYS * 2];
#endif

	s = spltty();

	/* discharge all */
	for (i = 0; i < sc->sc_nstrobe; i++) {
		pin = sc->sc_strobe_array[i];
		if (pin == -1)
			continue;
		pxa2x0_gpio_clear_bit(pin);
		pxa2x0_gpio_set_dir(pin, GPIO_IN);
	}

	delay(10);
	for (col = 0; col < sc->sc_nstrobe; col++) {
		pin = sc->sc_strobe_array[col];
		if (pin == -1)
			continue;

		/* activate_col */
		pxa2x0_gpio_set_bit(pin);
		pxa2x0_gpio_set_dir(pin, GPIO_OUT);

		/* wait activate delay */
		delay(10);

		/* read row */
		for (i = 0; i < sc->sc_nsense; i++) {
			int bit;

			if (sc->sc_sense_array[i] == -1)
				continue;
			bit = pxa2x0_gpio_get_bit(sc->sc_sense_array[i]);
			if (bit && sc->sc_hinge && col < sc->sc_maxkbdcol)
				continue;
			sc->sc_keystate[i + (col * sc->sc_nsense)] = bit;
		}

		/* reset_col */
		pxa2x0_gpio_set_dir(pin, GPIO_IN);

		/* wait discharge delay */
		delay(10);
	}

	/* charge all */
	for (i = 0; i < sc->sc_nstrobe; i++) {
		pin = sc->sc_strobe_array[i];
		if (pin == -1)
			continue;
		pxa2x0_gpio_set_bit(pin);
		pxa2x0_gpio_set_dir(pin, GPIO_OUT);
	}

	/* force the irqs to clear as we have just played with them. */
	for (i = 0; i < sc->sc_nsense; i++) {
		pin = sc->sc_sense_array[i];
		if (pin == -1)
			continue;
		pxa2x0_gpio_clear_intr(pin);
	}

	/* process after resetting interrupt */
	zkbd_modstate = (
		(sc->sc_keystate[84] ? (1 << 0) : 0) | /* shift */
		(sc->sc_keystate[93] ? (1 << 1) : 0) | /* Fn */
		(sc->sc_keystate[14] ? (1 << 2) : 0)); /* 'alt' */

	for (i = 0; i < sc->sc_nsense * sc->sc_nstrobe; i++) {
		stuck = 0;
		/* extend  xt_keymap to do this faster. */
		/* ignore 'stuck' keys' */
		for (j = 0; j < __arraycount(stuck_keys); j++) {
			if (stuck_keys[j] == i) {
				stuck = 1;
				break;
			}
		}
		if (stuck)
			continue;

		keystate = sc->sc_keystate[i];
		keysdown |= keystate; /* if any keys held */

#ifdef WSDISPLAY_COMPAT_RAWKBD
		if (sc->sc_polling == 0 && sc->sc_rawkbd) {
			if ((keystate) || (sc->sc_okeystate[i] != keystate)) {
				c = sc->sc_xt_keymap[i];
				if (c & 0x80) {
					cbuf[ncbuf++] = 0xe0;
				}
				cbuf[ncbuf] = c & 0x7f;

				if (keystate) {
					if (c & 0x80) {
						sc->sc_rep[npress++] = 0xe0;
					}
					sc->sc_rep[npress++] = c & 0x7f;
				} else {
					cbuf[ncbuf] |= 0x80;
				}
				ncbuf++;
				sc->sc_okeystate[i] = keystate;
			}
		}
#endif

		if ((!sc->sc_rawkbd) && (sc->sc_okeystate[i] != keystate)) {
			type = keystate ? WSCONS_EVENT_KEY_DOWN :
			    WSCONS_EVENT_KEY_UP;

			if (sc->sc_polling) {
				sc->sc_pollkey = i;
				sc->sc_pollUD = type;
			} else {
				wskbd_input(sc->sc_wskbddev, type, i);
			}

			sc->sc_okeystate[i] = keystate;
		}
	}

#ifdef WSDISPLAY_COMPAT_RAWKBD
	if (sc->sc_polling == 0 && sc->sc_rawkbd) {
		wskbd_rawinput(sc->sc_wskbddev, cbuf, ncbuf);
		sc->sc_nrep = npress;
		if (npress != 0)
			callout_schedule(&sc->sc_rawrepeat_ch,
			    hz * REP_DELAY1 / 1000);
		else 
			callout_stop(&sc->sc_rawrepeat_ch);
	}
#endif
	if (keysdown)
		callout_schedule(&sc->sc_roll_to, hz * REP_DELAYN / 1000 / 2);
	else 
		callout_stop(&sc->sc_roll_to);	/* always cancel? */

	splx(s);
}

#if NAPM > 0
extern	int kbd_reset;
extern	int apm_suspends;
static	int zkbdondown;				/* on key is pressed */
static	struct timeval zkbdontv = { 0, 0 };	/* last on key event */
const	struct timeval zkbdhalttv = { 3, 0 };	/*  3s for safe shutdown */
const	struct timeval zkbdsleeptv = { 0, 250000 };	/* .25s for suspend */
extern	int lid_suspend;
#endif

static int
zkbd_on(void *v)
{
#if NAPM > 0
	struct zkbd_softc *sc = (struct zkbd_softc *)v;
	int down = pxa2x0_gpio_get_bit(sc->sc_onkey_pin) ? 1 : 0;

	/*
	 * Change run mode depending on how long the key is held down.
	 * Ignore the key if it gets pressed while the lid is closed.
	 *
	 * Keys can bounce and we have to work around missed interrupts.
	 * Only the second edge is detected upon exit from sleep mode.
	 */
	if (down) {
		if (sc->sc_hinge == 3) {
			zkbdondown = 0;
		} else {
			microuptime(&zkbdontv);
			zkbdondown = 1;
		}
	} else if (zkbdondown) {
		if (ratecheck(&zkbdontv, &zkbdhalttv)) {
			if (kbd_reset == 1) {
				kbd_reset = 0;
				psignal(initproc, SIGUSR1);
			}
		} else if (ratecheck(&zkbdontv, &zkbdsleeptv)) {
			apm_suspends++;
		}
		zkbdondown = 0;
	}
#endif
	return 1;
}

static int
zkbd_sync(void *v)
{

	return 1;
}

static int
zkbd_hinge(void *v)
{
	struct zkbd_softc *sc = (struct zkbd_softc *)v;
	int a = pxa2x0_gpio_get_bit(sc->sc_swa_pin) ? 1 : 0;
	int b = pxa2x0_gpio_get_bit(sc->sc_swb_pin) ? 2 : 0;
#if NLCD > 0
	extern void lcd_blank(int);
#endif

	sc->sc_hinge = a | b;

	if (sc->sc_hinge == 3) {
#if NAPM > 0 
		if (lid_suspend)
			apm_suspends++;
#endif
#if NLCD > 0
		lcd_blank(1);
#endif
	} else {
#if NLCD > 0
		lcd_blank(0);
#endif
	}

	return 1;
}

static int
zkbd_enable(void *v, int on)
{

	return 0;
}

void
zkbd_set_leds(void *v, int on)
{
}

static int
zkbd_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
#ifdef WSDISPLAY_COMPAT_RAWKBD
	struct zkbd_softc *sc = (struct zkbd_softc *)v;
#endif

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_ZAURUS;
		return 0;

	case WSKBDIO_SETLEDS:
		return 0;

	case WSKBDIO_GETLEDS:
		*(int *)data = 0;
		return 0;

#ifdef WSDISPLAY_COMPAT_RAWKBD
	case WSKBDIO_SETMODE:
		sc->sc_rawkbd = *(int *)data == WSKBD_RAW;
		callout_stop(&sc->sc_rawrepeat_ch);
		return 0;
#endif
 
	}
	return EPASSTHROUGH;
}

/* implement polling for zaurus_kbd */
static void
zkbd_cngetc(void *v, u_int *type, int *data)
{
	struct zkbd_softc *sc = (struct zkbd_softc *)zkbd_sc;

	sc->sc_pollkey = -1;
	sc->sc_pollUD = -1;
	sc->sc_polling = 1;
	while (sc->sc_pollkey == -1) {
		zkbd_poll(sc);
		DELAY(10000);	/* XXX */
	}
	sc->sc_polling = 0;
	*data = sc->sc_pollkey;
	*type = sc->sc_pollUD;
}

static void
zkbd_cnpollc(void *v, int on)
{
}

static void
zkbd_power(int why, void *arg)
{

	zkbd_hinge(arg);
}

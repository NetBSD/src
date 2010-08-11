/*	$NetBSD: wzero3_keypad.c,v 1.1.4.2 2010/08/11 22:52:05 yamt Exp $	*/

/*
 * Copyright (c) 2010 NONAKA Kimihiro <nonaka@netbsd.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wzero3_keypad.c,v 1.1.4.2 2010/08/11 22:52:05 yamt Exp $");

#include "wzero3lcd.h"
#include "opt_wsdisplay_compat.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/callout.h>

#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include <machine/bus.h>
#include <machine/bootinfo.h>
#include <machine/config_hook.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/wscons/wsksymdef.h>

#ifdef WSDISPLAY_COMPAT_RAWKBD
#include <dev/hpc/pckbd_encode.h>
#endif

#include <arch/hpcarm/dev/wzero3_reg.h>
#include <arch/hpcarm/dev/wzero3_sspvar.h>

enum {
	KD_0,
	KD_1,
	KD_2,
	KD_3,
	KD_4,
	KD_5,
	KD_6,
	KD_7,
	KD_8,
	KD_9,
	KD_ASTERISK,
	KD_NUMBER,
	KD_WINDOWS,
	KD_OK,
	KD_ONHOOK,
	KD_OFFHOOK,
	KD_CLEAR,
	KD_MOJI,
	KD_UP,
	KD_DOWN,
	KD_LEFT,
	KD_RIGHT,
	KD_CENTER_BUTTON,
	KD_LSOFT,
	KD_RSOFT,
	KD_NUM,

	KD_INVALID = -1
};

static int ws011sh_keyscan2keydown[32] = {
	KD_INVALID,
	KD_CLEAR,
	KD_INVALID,
	KD_OK,
	KD_INVALID,
	KD_LEFT,
	KD_INVALID,
	KD_ONHOOK,
	KD_INVALID,
	KD_UP,
	KD_DOWN,
	KD_MOJI,
	KD_INVALID,
	KD_WINDOWS,
	KD_INVALID,
	KD_RIGHT,
	KD_INVALID,
	KD_1,
	KD_4,
	KD_7,
	KD_ASTERISK,
	KD_2,
	KD_5,
	KD_8,
	KD_0,
	KD_CENTER_BUTTON,
	KD_INVALID,
	KD_3,
	KD_6,
	KD_9,
	KD_NUMBER,
	KD_INVALID,
};

struct wzero3keypad_softc {
	device_t sc_dev;

	void *sc_ih;
	int sc_intr_pin;

	uint32_t sc_okeystat;

	struct callout sc_poll_ch;
	int sc_poll_interval;

	device_t sc_wskbddev;

#ifdef WSDISPLAY_COMPAT_RAWKBD
	int sc_rawkbd;
#endif
};

static int wzero3keypad_match(device_t, cfdata_t, void *);
static void wzero3keypad_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(wzero3keypad, sizeof(struct wzero3keypad_softc),
    wzero3keypad_match, wzero3keypad_attach, NULL, NULL);

static int wzero3keypad_wskbd_enable(void *, int);
static void wzero3keypad_wskbd_set_leds(void *, int);
static int wzero3keypad_wskbd_ioctl(void *, u_long, void *, int, struct lwp *);

static const int wzero3keypad_wskbd_keys[] = {
	82,	/* KD_0: 0 */
	79,	/* KD_1: 1 */
	80,	/* KD_2: 2 */
	81,	/* KD_3: 3 */
	75,	/* KD_4: 4 */
	76,	/* KD_5: 5 */
	77,	/* KD_6: 6 */
	71,	/* KD_7: 7 */
	72,	/* KD_8: 8 */
	73,	/* KD_9: 9 */
	64,	/* KD_ASTERISK: f6 */
	65,	/* KD_NUMBER: f7 */
	221,	/* KD_WINDOWS: Menu */
	61,	/* KD_OK: f3 */
	59,	/* KD_ONHOOK: f1 */
	60,	/* KD_OFFHOOK: f2 */
	62,	/* KD_CLEAR: f4 */
	63,	/* KD_MOJI: f5 */
	200,	/* KD_UP: Up */
	208,	/* KD_DOWN: Down */
	203,	/* KD_LEFT: Left */
	205,	/* KD_RIGHT: Right */
	156,	/* KD_CENTER_BUTTON: KP_Enter */
	87,	/* KD_LSOFT: f11 */
	88,	/* KD_RSOFT: f12 */
};

static const keysym_t wzero3keypad_wskbd_keydesc[] = {
	KS_KEYCODE(59),		KS_f1,
	KS_KEYCODE(60),		KS_f2,
	KS_KEYCODE(61),		KS_f3,
	KS_KEYCODE(62),		KS_f4,
	KS_KEYCODE(63),		KS_f5,
	KS_KEYCODE(64),		KS_f6,
	KS_KEYCODE(65),		KS_f7,
	KS_KEYCODE(71),		KS_7,
	KS_KEYCODE(72),		KS_8,
	KS_KEYCODE(73),		KS_9,
	KS_KEYCODE(75),		KS_4,
	KS_KEYCODE(76),		KS_5,
	KS_KEYCODE(77),		KS_6,
	KS_KEYCODE(79),		KS_1,
	KS_KEYCODE(80),		KS_2,
	KS_KEYCODE(81),		KS_3,
	KS_KEYCODE(82),		KS_0,
	KS_KEYCODE(87),		KS_f11,
	KS_KEYCODE(88),		KS_f12,
	KS_KEYCODE(156),	KS_KP_Enter,
	KS_KEYCODE(200),	KS_Up,
	KS_KEYCODE(203),	KS_Left,
	KS_KEYCODE(205),	KS_Right,
	KS_KEYCODE(208),	KS_Down,
	KS_KEYCODE(221),	KS_Menu,
};

static const struct wscons_keydesc wzero3keypad_wskbd_keydesctab[] = {
	{ KB_JP, 0,
	  sizeof(wzero3keypad_wskbd_keydesc) / sizeof(keysym_t),
	  wzero3keypad_wskbd_keydesc
	},

	{ 0, 0, 0, 0 }
};

static const struct wskbd_mapdata wzero3keypad_wskbd_keymapdata = {
	wzero3keypad_wskbd_keydesctab, KB_JP
};

static const struct wskbd_accessops wzero3keypad_wskbd_accessops = {
	wzero3keypad_wskbd_enable,
	wzero3keypad_wskbd_set_leds,
	wzero3keypad_wskbd_ioctl,
};

static int wzero3keypad_intr(void *);
static void wzero3keypad_poll(void *);
static void wzero3keypad_poll1(struct wzero3keypad_softc *, int);

static void wzero3keypad_init(struct wzero3keypad_softc *);
static uint32_t wzero3keypad_getkeydown(struct wzero3keypad_softc *, int);

static const struct wzero3keypad_model {
	platid_mask_t *platid;
	int intr_pin;
} wzero3keypad_table[] = {
#if 0
	/* WS007SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS007SH,
		-1,	/* XXX */
	},
#endif
	/* WS011SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS011SH,
		GPIO_WS011SH_KEYPAD,
	},

	{ NULL, -1, }
};

static const struct wzero3keypad_model *
wzero3keypad_lookup(void)
{
	const struct wzero3keypad_model *model;

	for (model = wzero3keypad_table; model->platid != NULL; model++) {
		if (platid_match(&platid, model->platid)) {
			return model;
		}
	}
	return NULL;
}

static int
wzero3keypad_match(struct device *parent, struct cfdata *cf, void *aux)
{

	if (strcmp(cf->cf_name, "wzero3keypad") != 0)
		return 0;
	if (wzero3keypad_lookup() == NULL)
		return 0;
	return 1;
}

static void
wzero3keypad_attach(struct device *parent, struct device *self, void *aux)
{
	struct wzero3keypad_softc *sc = device_private(self);
	const struct wzero3keypad_model *model;
	struct wskbddev_attach_args wska;
#if NWZERO3LCD > 0
	extern int screen_rotate;
#endif

	sc->sc_dev = self;
	sc->sc_okeystat = 0;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	sc->sc_rawkbd = 0;
#endif

	model = wzero3keypad_lookup();
	if (model == NULL) {
		aprint_error(": unknown model\n");
		return;
	}

	aprint_normal(": keypad\n");
	aprint_naive("\n");

	sc->sc_intr_pin = model->intr_pin;

	callout_init(&sc->sc_poll_ch, 0);
	callout_setfunc(&sc->sc_poll_ch, wzero3keypad_poll, sc);
	sc->sc_poll_interval = hz / 32;

#if NWZERO3LCD > 0
	switch (screen_rotate) {
	default:
	case 0:
		break;

	case 270:	/* counter clock-wise */
		ws011sh_keyscan2keydown[5] = KD_UP;
		ws011sh_keyscan2keydown[9] = KD_RIGHT;
		ws011sh_keyscan2keydown[10] = KD_LEFT;
		ws011sh_keyscan2keydown[15] = KD_DOWN;
		break;
	}
#endif

	/* attach wskbd */
	wska.console = 0;
	wska.keymap = &wzero3keypad_wskbd_keymapdata;
	wska.accessops = &wzero3keypad_wskbd_accessops;
	wska.accesscookie = sc;
	sc->sc_wskbddev = config_found_ia(self, "wskbddev", &wska,
	    wskbddevprint);

	/* setup keypad interrupt */
	pxa2x0_gpio_set_function(sc->sc_intr_pin, GPIO_IN);
	sc->sc_ih = pxa2x0_gpio_intr_establish(sc->sc_intr_pin,
	    IST_EDGE_RISING, IPL_TTY, wzero3keypad_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish keypad interrupt\n");
	}

	/* init hardware */
	wzero3keypad_init(sc);
}

static int
wzero3keypad_wskbd_enable(void *arg, int onoff)
{

	return 0;
}

static void
wzero3keypad_wskbd_set_leds(void *arg, int leds)
{

	/* Nothing to do */
}

static int
wzero3keypad_wskbd_ioctl(void *arg, u_long cmd, void *data, int flags,
    struct lwp *l)
{
#ifdef WSDISPLAY_COMPAT_RAWKBD
	struct wzero3keypad_softc *sc = (struct wzero3keypad_softc *)arg;
#endif

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_HPC_KBD;
		return 0;
	case WSKBDIO_SETLEDS:
		return 0;
	case WSKBDIO_GETLEDS:
		*(int *)data = 0;
		return 0;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	case WSKBDIO_SETMODE:
		sc->sc_rawkbd = (*(int *)data == WSKBD_RAW);
		return 0;
#endif
	}

	return EPASSTHROUGH;
}

static int
wzero3keypad_intr(void *arg)
{
	struct wzero3keypad_softc *sc = (struct wzero3keypad_softc *)arg;

	pxa2x0_gpio_clear_intr(sc->sc_intr_pin);

	wzero3keypad_poll1(sc, 0);

	callout_schedule(&sc->sc_poll_ch, sc->sc_poll_interval);

	return 1;
}

static void
wzero3keypad_poll(void *v)
{
	struct wzero3keypad_softc *sc = (struct wzero3keypad_softc *)v;

	wzero3keypad_poll1(sc, 1);

	callout_stop(&sc->sc_poll_ch);
}

static void
wzero3keypad_poll1(struct wzero3keypad_softc *sc, int doscan)
{
	uint32_t keydown;
	uint32_t diff;
	int i;
	int s;

	s = spltty();

	keydown = wzero3keypad_getkeydown(sc, doscan);
	diff = keydown ^ sc->sc_okeystat;
	if (diff == 0)
		goto out;

	for (i = 0; i < KD_NUM; i++) {
		if (diff & (1 << i)) {
			int state = keydown & (1 << i);
			int type = state ? WSCONS_EVENT_KEY_DOWN :
			    WSCONS_EVENT_KEY_UP;
			int key = wzero3keypad_wskbd_keys[i];
#ifdef WSDISPLAY_COMPAT_RAWKBD
			if (sc->sc_rawkbd) {
				int n;
				u_char data[16];

				n = pckbd_encode(type, key, data);
				wskbd_rawinput(sc->sc_wskbddev, data, n);
			} else
#endif
				wskbd_input(sc->sc_wskbddev, type, key);
		}
	}
	sc->sc_okeystat = keydown;

out:
	splx(s);
}

/*----------------------------------------------------------------------------
 * AK4184 keypad controller for WS011SH
 */
/* ak4184 command register */
#define	AKMCTRL_WR_SH	7
#define	AKMCTRL_PAGE_SH	6
#define	AKMCTRL_ADDR_SH	0
#define	AKMCTRL_WRITE	(0<<AKMCTRL_WR_SH)
#define	AKMCTRL_READ	(1<<AKMCTRL_WR_SH)
#define	AKMCTRL_DATA	(0<<AKMCTRL_PAGE_SH)
#define	AKMCTRL_CTRL	(1<<AKMCTRL_PAGE_SH)

static void
wzero3keypad_init(struct wzero3keypad_softc *sc)
{
	int s;

	s = spltty();

#if 0
	/*
	 * - key interrupt enable
	 * - key touch scan
	 * - debounce time: 1ms
	 * - wait 100us for debounce time
	 */
	(void) wzero3ssp_ic_send(WZERO3_SSP_IC_AK4184_KEYPAD,
	    AKMCTRL_WRITE | AKMCTRL_CTRL | (0<<AKMCTRL_ADDR_SH), 0);
#endif

	/* unmask all keys & columns */
	(void) wzero3ssp_ic_send(WZERO3_SSP_IC_AK4184_KEYPAD,
	    AKMCTRL_WRITE | AKMCTRL_CTRL | (1<<AKMCTRL_ADDR_SH), 0);
	(void) wzero3ssp_ic_send(WZERO3_SSP_IC_AK4184_KEYPAD,
	    AKMCTRL_WRITE | AKMCTRL_CTRL | (2<<AKMCTRL_ADDR_SH), 0);
	(void) wzero3ssp_ic_send(WZERO3_SSP_IC_AK4184_KEYPAD,
	    AKMCTRL_WRITE | AKMCTRL_CTRL | (3<<AKMCTRL_ADDR_SH), 0);

	/* Enable keypad interrupt (kpdata dummy read) */
	(void) wzero3ssp_ic_send(WZERO3_SSP_IC_AK4184_KEYPAD,
	    AKMCTRL_READ | AKMCTRL_DATA | (1<<AKMCTRL_ADDR_SH), 0);

	splx(s);
}

static uint32_t
wzero3keypad_getkeydown(struct wzero3keypad_softc *sc, int doscan)
{
	uint32_t keydown = 0;
	uint16_t status;
	uint16_t kpdata;
	int timo;

	if (doscan) {
		/* host scan */
		(void) wzero3ssp_ic_send(WZERO3_SSP_IC_AK4184_KEYPAD,
		    AKMCTRL_WRITE | AKMCTRL_CTRL | (4<<AKMCTRL_ADDR_SH), 0);
		delay(100);
	}

	timo = 1000;
	do {
		status = wzero3ssp_ic_send(WZERO3_SSP_IC_AK4184_KEYPAD,
		    AKMCTRL_READ | AKMCTRL_CTRL | (0<<AKMCTRL_ADDR_SH), 0);
	} while ((status & 0xc000) == 0 && timo-- > 0);

	kpdata = wzero3ssp_ic_send(WZERO3_SSP_IC_AK4184_KEYPAD,
	    AKMCTRL_READ | AKMCTRL_DATA | (1<<AKMCTRL_ADDR_SH), 0);
	if ((status & 0xc000) == 0xc000) {
		if (!(kpdata & 0x8000)) {
			int i;

			for (i = 0; i < 3; i++) {
				int key, bits;

				key = kpdata & 0x1f;
				if (key == 0)
					break;
				bits = ws011sh_keyscan2keydown[key];
				if (bits != KD_INVALID)
					keydown |= 1 << bits;
				kpdata >>= 5;
			}
		}
	}

	return keydown;
}

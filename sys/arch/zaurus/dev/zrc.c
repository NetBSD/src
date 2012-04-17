/*	$NetBSD: zrc.c,v 1.7.2.1 2012/04/17 00:07:13 yamt Exp $	*/
/*	$OpenBSD: zaurus_remote.c,v 1.1 2005/11/17 05:26:31 uwe Exp $	*/

/*
 * Copyright (c) 2005 Uwe Stuehler <uwe@openbsd.org>
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
__KERNEL_RCSID(0, "$NetBSD: zrc.c,v 1.7.2.1 2012/04/17 00:07:13 yamt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/callout.h>
#include <sys/systm.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include <machine/intr.h>

#include <zaurus/zaurus/zaurus_reg.h>
#include <zaurus/zaurus/zaurus_var.h>
#include <zaurus/dev/zsspvar.h>
#include <zaurus/dev/scoopvar.h>
#include <zaurus/dev/ioexpvar.h>

#define RESCAN_INTERVAL		(hz/100)

#define KEY_RELEASE		0	/* button release */
#define KEY_VOL_DOWN		1
#define KEY_MUTE		2
#define KEY_REWIND		3
#define KEY_VOL_UP		4
#define KEY_FORWARD		5
#define KEY_PLAY		6
#define KEY_STOP		7
#define KEY_EARPHONE		8

#ifdef DEBUG
static const char *zrc_keyname[] = {
	"(release)", "volume down", "mute", "rewind", "volume up",
	"forward", "play", "stop", "(earphone)"
};
#endif

struct zrc_akey {
	int	min;		/* minimum ADC value or INT_MIN */
	int	key;		/* remote control key number */
};

/* Values match the resistors in the CE-RH2 remote control. */
static const struct zrc_akey zrc_akeytab_c3000[] = {
	{ 238,		KEY_RELEASE  },
	{ 202,		KEY_VOL_DOWN },
	{ 168,		KEY_MUTE     },
	{ 135,		KEY_REWIND   },
	{ 105,		KEY_VOL_UP   },
	{ 74,		KEY_FORWARD  },
	{ 42,		KEY_PLAY     },
	{ 12,		KEY_STOP     },
	{ INT_MIN,	KEY_EARPHONE }
};

static const struct zrc_akey *zrc_akeytab = zrc_akeytab_c3000;

struct zrc_softc {
	device_t	 sc_dev;
	struct callout	 sc_to;
	void		*sc_ih;
	int		 sc_key;	/* being scanned */
	int		 sc_scans;	/* rescan counter */
	int		 sc_noise;	/* discard if too noisy? */
	int		 sc_keydown;	/* currently pressed key */
	struct device   *sc_wskbddev;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	int		 sc_rawkbd;
#endif
};

static int	zrc_match(struct device *, struct cfdata *, void *);
static void	zrc_attach(struct device *, struct device *, void *);

CFATTACH_DECL_NEW(zrc, sizeof(struct zrc_softc), 
    zrc_match, zrc_attach, NULL, NULL);

static int	zrc_finalize(device_t);
static int	zrc_intr(void *);
static void	zrc_timeout(void *);
static int	zrc_scan(void);
static void	zrc_input(struct zrc_softc *, int, int);

static int	zrc_enable(void *, int);
static void	zrc_set_leds(void *, int);
static int	zrc_ioctl(void *, u_long, void *, int, struct lwp *);

struct wskbd_accessops zrc_accessops = {
	zrc_enable,
	zrc_set_leds,
	zrc_ioctl,
};

#define	KC(n) KS_KEYCODE(n)

/* XXX what keys should be generated in translated mode? */
static const keysym_t zrc_keydesc[] = {
	KC(KEY_VOL_DOWN),	KS_Cmd_VolumeUp,
	KC(KEY_MUTE),		KS_Cmd_VolumeToggle,
	KC(KEY_REWIND),		KS_b,
	KC(KEY_VOL_UP),		KS_Cmd_VolumeDown,
	KC(KEY_FORWARD),	KS_f,
	KC(KEY_PLAY),		KS_p,
	KC(KEY_STOP),		KS_s,
};

#ifdef WSDISPLAY_COMPAT_RAWKBD
#define	RAWKEY_AudioRewind	0xa0
#define	RAWKEY_AudioForward	0xa1
#define	RAWKEY_AudioPlay	0xa2
#define	RAWKEY_AudioStop	0xa3
static const keysym_t zrc_xt_keymap[] = {
	/* KC(KEY_RELEASE), */	RAWKEY_Null,
	/* KC(KEY_VOL_DOWN), */	RAWKEY_AudioLower,
	/* KC(KEY_MUTE), */	RAWKEY_AudioMute,
	/* KC(KEY_REWIND), */	RAWKEY_AudioRewind,
	/* KC(KEY_VOL_UP), */	RAWKEY_AudioRaise,
	/* KC(KEY_FORWARD), */	RAWKEY_AudioForward,
	/* KC(KEY_PLAY), */	RAWKEY_AudioPlay,
	/* KC(KEY_STOP), */	RAWKEY_AudioStop,
};
#endif

static const struct wscons_keydesc zrc_keydesctab[] = {
        {KB_US, 0, sizeof(zrc_keydesc)/sizeof(keysym_t), zrc_keydesc},
        {0, 0, 0, 0}
};

struct wskbd_mapdata zrc_keymapdata = {
	zrc_keydesctab, KB_US
};

#undef	KC

static int
zrc_match(device_t parent, cfdata_t cf, void *aux)
{

	if (ZAURUS_ISC1000 || ZAURUS_ISC3000)
		return 1;
	return 0;
}

static void
zrc_attach(device_t parent, device_t self, void *aux)
{
	struct zrc_softc *sc = device_private(self);
	struct wskbddev_attach_args a;

	sc->sc_dev = self;

	aprint_normal(": CE-RH2 remote control\n");
	aprint_naive("\n");

	/* Configure remote control interrupt handling. */
	callout_init(&sc->sc_to, 0);
	callout_setfunc(&sc->sc_to, zrc_timeout, sc);

	/* Establish interrput */
	pxa2x0_gpio_set_function(C3000_RC_IRQ_PIN, GPIO_IN);
	sc->sc_ih = pxa2x0_gpio_intr_establish(C3000_RC_IRQ_PIN,
	    IST_EDGE_BOTH, IPL_BIO, zrc_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(sc->sc_dev, "couldn't establish interrupt.\n");
		return;
	}

	/* defer enabling pullup until ioexp or scoop is attached */
	config_finalize_register(self, zrc_finalize);

	sc->sc_keydown = KEY_RELEASE;

	a.console = 0;
	a.keymap = &zrc_keymapdata;
	a.accessops = &zrc_accessops;
	a.accesscookie = sc;

	sc->sc_wskbddev = config_found(self, &a, wskbddevprint);
}

static int
zrc_finalize(device_t dv)
{

	/* Enable the pullup while waiting for an interrupt. */
	if (ZAURUS_ISC1000)
		ioexp_akin_pullup(1);
	else
		scoop_akin_pullup(1);

	return 0;
}

static int
zrc_intr(void *v)
{
	struct zrc_softc *sc = v;

	/* just return if remote control isn't present */

	pxa2x0_gpio_intr_mask(sc->sc_ih);
	if (ZAURUS_ISC1000)
		ioexp_akin_pullup(0);
	else
		scoop_akin_pullup(0);
	sc->sc_key = zrc_scan();
	sc->sc_scans = 0;
	sc->sc_noise = 0;
	callout_schedule(&sc->sc_to, RESCAN_INTERVAL);

	return 1;
}

static void
zrc_timeout(void *v)
{
	struct zrc_softc *sc = v;
	int key;

	key = zrc_scan();
	switch (sc->sc_scans) {
	case 0:
	case 1:
	case 2:
		/* wait for a stable read */
		if (sc->sc_key == key)
			sc->sc_scans++;
		else {
			sc->sc_key = key;
			sc->sc_scans = 0;
			sc->sc_noise++;
		}
		callout_schedule(&sc->sc_to, RESCAN_INTERVAL);
		break;
	case 3:
		/* generate key press event */
		if (sc->sc_key != key) {
			key = sc->sc_key;
			sc->sc_noise++;
		}
		sc->sc_scans++;
		switch (key) {
		case KEY_EARPHONE:
		case KEY_RELEASE:
			sc->sc_scans = 6;
			break;
		default:
#ifdef DEBUG
			printf("%s: %s pressed (%d noise)\n",
			    device_xname(sc->sc_dev),
			    zrc_keyname[key], sc->sc_noise);
#endif
			sc->sc_keydown = key;
			sc->sc_noise = 0;
			zrc_input(sc, key, 1);
			break;
		}
		callout_schedule(&sc->sc_to, RESCAN_INTERVAL);
		break;
	case 4:
	case 5:
		/* wait for key release, permit noise */
		if (sc->sc_key == key) {
			if (sc->sc_scans == 5)
				sc->sc_noise++;
			sc->sc_scans = 4;
		} else
			sc->sc_scans++;
		callout_schedule(&sc->sc_to, RESCAN_INTERVAL);
		break;
	case 6:
		/* generate key release event */
		if (sc->sc_keydown != KEY_RELEASE) {
			zrc_input(sc, sc->sc_keydown, 0);
#ifdef DEBUG
			printf("%s: %s released (%d noise)\n",
			    device_xname(sc->sc_dev),
			    zrc_keyname[sc->sc_keydown], sc->sc_noise);
#endif
			sc->sc_keydown = KEY_RELEASE;
		}
		/* FALLTHROUGH */
	default:
		/* unmask interrupt again */
		callout_stop(&sc->sc_to);
		sc->sc_scans = 7;
		if (ZAURUS_ISC1000)
			ioexp_akin_pullup(1);
		else
			scoop_akin_pullup(1);
		pxa2x0_gpio_intr_unmask(sc->sc_ih);
	}
}

static int
zrc_scan(void)
{
	int val;
	int i;

/* XXX MAX1111 command word - also appears in zaurus_apm.c */
#define MAXCTRL_PD0		(1<<0)
#define MAXCTRL_PD1		(1<<1)
#define MAXCTRL_SGL		(1<<2)
#define MAXCTRL_UNI		(1<<3)
#define MAXCTRL_SEL_SHIFT	4
#define MAXCTRL_STR		(1<<7)

#define C3000_ADCCH_ZRC 0
	val = zssp_read_max1111(MAXCTRL_PD0 | MAXCTRL_PD1 | MAXCTRL_SGL |
	    MAXCTRL_UNI | (C3000_ADCCH_ZRC << MAXCTRL_SEL_SHIFT) |
	    MAXCTRL_STR);
	for (i = 0; zrc_akeytab[i].min != INT_MIN; i++)
		if (val >= zrc_akeytab[i].min)
			break;
	return zrc_akeytab[i].key;
}

static void
zrc_input(struct zrc_softc *sc, int key, int down)
{
	u_int type = down ? WSCONS_EVENT_KEY_DOWN : WSCONS_EVENT_KEY_UP;
	int s;

	s = spltty();

#ifdef WSDISPLAY_COMPAT_RAWKBD
	if (sc->sc_rawkbd) {
		int	c;
		u_char	cbuf[2];
		int	ncbuf = 0;

		c = zrc_xt_keymap[key];
		if (c & 0x80)
			cbuf[ncbuf++] = 0xe0;
		cbuf[ncbuf] = c & 0x7f;

		if (!down)
			cbuf[ncbuf] |= 0x80;
		ncbuf++;

		wskbd_rawinput(sc->sc_wskbddev, cbuf, ncbuf);
	} else
#endif
		wskbd_input(sc->sc_wskbddev, type, key);

	splx(s);
}

static int
zrc_enable(void *v, int on)
{

	return 0;
}

static void
zrc_set_leds(void *v, int on)
{

	/* Nothing to do */
}

static int
zrc_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
#ifdef WSDISPLAY_COMPAT_RAWKBD
	struct zrc_softc *sc = v;
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
		sc->sc_rawkbd = (*(int *)data == WSKBD_RAW);
		return 0;
#endif
	}
	return EPASSTHROUGH;
}

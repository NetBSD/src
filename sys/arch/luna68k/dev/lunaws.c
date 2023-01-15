/* $NetBSD: lunaws.c,v 1.42 2023/01/15 05:08:33 tsutsui Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: lunaws.c,v 1.42 2023/01/15 05:08:33 tsutsui Exp $");

#include "opt_wsdisplay_compat.h"
#include "wsmouse.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/rndsource.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/wscons/wsmousevar.h>

#include <luna68k/dev/omkbdmap.h>
#include <luna68k/dev/sioreg.h>
#include <luna68k/dev/siovar.h>

#include <machine/board.h>

#include "ioconf.h"

#define OMKBD_RXQ_LEN		64
#define OMKBD_RXQ_LEN_MASK	(OMKBD_RXQ_LEN - 1)
#define OMKBD_NEXTRXQ(x)	(((x) + 1) & OMKBD_RXQ_LEN_MASK)
#define OMKBD_TXQ_LEN		16
#define OMKBD_TXQ_LEN_MASK	(OMKBD_TXQ_LEN - 1)
#define OMKBD_NEXTTXQ(x)	(((x) + 1) & OMKBD_TXQ_LEN_MASK)

/* Keyboard commands */
/*  000XXXXXb : LED commands */
#define OMKBD_LED_ON_KANA	0x10	/* kana LED on */
#define OMKBD_LED_OFF_KANA	0x00	/* kana LED off */
#define OMKBD_LED_ON_CAPS	0x11	/* caps LED on */
#define OMKBD_LED_OFF_CAPS	0x01	/* caps LED off */
/*  010XXXXXb : buzzer commands */
#define OMKBD_BUZZER		0x40
#define OMKBD_BUZZER_PERIOD	0x18
#define OMKBD_BUZZER_40MS	0x00
#define OMKBD_BUZZER_150MS	0x08
#define OMKBD_BUZZER_400MS	0x10
#define OMKBD_BUZZER_700MS	0x18
#define OMKBD_BUZZER_PITCH	0x07
#define OMKBD_BUZZER_6000HZ	0x00
#define OMKBD_BUZZER_3000HZ	0x01
#define OMKBD_BUZZER_1500HZ	0x02
#define OMKBD_BUZZER_1000HZ	0x03
#define OMKBD_BUZZER_600HZ	0x04
#define OMKBD_BUZZER_300HZ	0x05
#define OMKBD_BUZZER_150HZ	0x06
#define OMKBD_BUZZER_100HZ	0x07
/*  011XXXXXb : mouse on command */
#define OMKBD_MOUSE_ON		0x60
/*  001XXXXXb : mouse off command */
#define OMKBD_MOUSE_OFF		0x20

#define OMKBD_BUZZER_DEFAULT	\
	(OMKBD_BUZZER | OMKBD_BUZZER_40MS | OMKBD_BUZZER_1500HZ)

static const uint8_t ch1_regs[6] = {
	WR0_RSTINT,				/* Reset E/S Interrupt */
	WR1_RXALLS | WR1_TXENBL,		/* Rx per char, Tx */
	0,					/* */
	WR3_RX8BIT | WR3_RXENBL,		/* Rx */
	WR4_BAUD96 | WR4_STOP1 | WR4_NPARITY,	/* Tx/Rx */
	WR5_TX8BIT | WR5_TXENBL,		/* Tx */
};

struct ws_conscookie {
	struct sioreg	*cc_sio;
	int		cc_polling;
	struct ws_softc	*cc_sc;
};

struct ws_softc {
	device_t	sc_dev;
	struct sioreg	*sc_ctl;
	uint8_t		sc_wr[6];
	device_t	sc_wskbddev;
	uint8_t		sc_rxq[OMKBD_RXQ_LEN];
	u_int		sc_rxqhead;
	u_int		sc_rxqtail;
	uint8_t		sc_txq[OMKBD_TXQ_LEN];
	u_int		sc_txqhead;
	u_int		sc_txqtail;
	bool		sc_tx_busy;
	bool		sc_tx_done;
	int		sc_leds;
#if NWSMOUSE > 0
	device_t	sc_wsmousedev;
	int		sc_msbuttons, sc_msdx, sc_msdy;
#endif
	int		sc_msreport;
	void		*sc_si;
	int		sc_rawkbd;

	struct ws_conscookie *sc_conscookie;
	krndsource_t	sc_rndsource;
};

static void omkbd_input(struct ws_softc *, int);
static void omkbd_send(struct ws_softc *, uint8_t);
static void omkbd_decode(struct ws_softc *, int, u_int *, int *);

static int  omkbd_enable(void *, int);
static void omkbd_set_leds(void *, int);
static int  omkbd_ioctl(void *, u_long, void *, int, struct lwp *);

static void omkbd_complex_buzzer(struct ws_softc *, struct wskbd_bell_data *);
static uint8_t omkbd_get_buzcmd(struct ws_softc *, struct wskbd_bell_data *,
    uint8_t);

static const struct wskbd_mapdata omkbd_keymapdata = {
	.keydesc = omkbd_keydesctab,
	.layout  = KB_JP,
};
static const struct wskbd_accessops omkbd_accessops = {
	.enable   = omkbd_enable,
	.set_leds = omkbd_set_leds,
	.ioctl    = omkbd_ioctl,
};

void	ws_cnattach(void);
static void ws_cngetc(void *, u_int *, int *);
static void ws_cnpollc(void *, int);
static void ws_cnbell(void *, u_int, u_int, u_int);
static const struct wskbd_consops ws_consops = {
	.getc  = ws_cngetc,
	.pollc = ws_cnpollc,
	.bell  = ws_cnbell,
};
static struct ws_conscookie ws_conscookie;

#if NWSMOUSE > 0
static int  omms_enable(void *);
static int  omms_ioctl(void *, u_long, void *, int, struct lwp *);
static void omms_disable(void *);

static const struct wsmouse_accessops omms_accessops = {
	.enable  = omms_enable,
	.ioctl   = omms_ioctl,
	.disable = omms_disable,
};
#endif

static void wsintr(void *);
static void wssoftintr(void *);

static int  wsmatch(device_t, cfdata_t, void *);
static void wsattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ws, sizeof(struct ws_softc),
    wsmatch, wsattach, NULL, NULL);

/* #define LUNAWS_DEBUG */

#ifdef LUNAWS_DEBUG
#define DEBUG_KBDTX	0x01
#define DEBUG_RXSOFT	0x02
#define DEBUG_BUZZER	0x04
uint32_t lunaws_debug = 0x00 /* | DEBUG_BUZZER | DEBUG_KBDTX | DEBUG_RXSOFT */;
#define DPRINTF(x, y)   if (lunaws_debug & (x)) printf y
#else
#define DPRINTF(x, y)   __nothing
#endif

static int
wsmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct sio_attach_args *args = aux;

	if (args->channel != 1)
		return 0;
	return 1;
}

static void
wsattach(device_t parent, device_t self, void *aux)
{
	struct ws_softc *sc = device_private(self);
	struct sio_softc *siosc = device_private(parent);
	struct sio_attach_args *args = aux;
	int channel = args->channel;
	struct wskbddev_attach_args a;

	sc->sc_dev = self;
	sc->sc_ctl = &siosc->sc_ctl[channel];
	memcpy(sc->sc_wr, ch1_regs, sizeof(ch1_regs));
	siosc->sc_intrhand[channel].ih_func = wsintr;
	siosc->sc_intrhand[channel].ih_arg = sc;

	sc->sc_conscookie = &ws_conscookie;
	sc->sc_conscookie->cc_sc = sc;
	sc->sc_conscookie->cc_polling = 0;

	setsioreg(sc->sc_ctl, WR0, sc->sc_wr[WR0]);
	setsioreg(sc->sc_ctl, WR4, sc->sc_wr[WR4]);
	setsioreg(sc->sc_ctl, WR3, sc->sc_wr[WR3]);
	setsioreg(sc->sc_ctl, WR5, sc->sc_wr[WR5]);
	setsioreg(sc->sc_ctl, WR0, sc->sc_wr[WR0]);

	sc->sc_rxqhead = 0;
	sc->sc_rxqtail = 0;
	sc->sc_txqhead = 0;
	sc->sc_txqtail = 0;
	sc->sc_tx_busy = false;
	sc->sc_tx_done = false;

	sc->sc_si = softint_establish(SOFTINT_SERIAL, wssoftintr, sc);
	rnd_attach_source(&sc->sc_rndsource, device_xname(self),
	    RND_TYPE_TTY, RND_FLAG_DEFAULT);

	/* enable interrupt */
	setsioreg(sc->sc_ctl, WR1, sc->sc_wr[WR1]);

	aprint_normal("\n");

	/* keep mouse quiet */
	omkbd_send(sc, OMKBD_MOUSE_OFF);

	a.console = (args->hwflags == 1);
	a.keymap = &omkbd_keymapdata;
	a.accessops = &omkbd_accessops;
	a.accesscookie = (void *)sc;
	sc->sc_wskbddev = config_found(self, &a, wskbddevprint,
	    CFARGS(.iattr = "wskbddev"));

#if NWSMOUSE > 0
	{
	struct wsmousedev_attach_args b;
	b.accessops = &omms_accessops;
	b.accesscookie = (void *)sc;
	sc->sc_wsmousedev = config_found(self, &b, wsmousedevprint,
	    CFARGS(.iattr = "wsmousedev"));
	}
#endif
	sc->sc_msreport = 0;
}

static void
wsintr(void *arg)
{
	struct ws_softc *sc = arg;
	struct sioreg *sio = sc->sc_ctl;
	uint8_t code = 0;
	uint16_t rr, rndcsr = 0;
	bool handled = false;

	rr = getsiocsr(sio);
	rndcsr = rr;
	if ((rr & RR_RXRDY) != 0) {
		do {
			code = sio->sio_data;
			if (rr & (RR_FRAMING | RR_OVERRUN | RR_PARITY)) {
				sio->sio_cmd = WR0_ERRRST;
				continue;
			}
			sc->sc_rxq[sc->sc_rxqtail] = code;
			sc->sc_rxqtail = OMKBD_NEXTRXQ(sc->sc_rxqtail);
		} while (((rr = getsiocsr(sio)) & RR_RXRDY) != 0);
		handled = true;
	}
	if ((rr & RR_TXRDY) != 0) {
		sio->sio_cmd = WR0_RSTPEND;
		if (sc->sc_tx_busy) {
			sc->sc_tx_busy = false;
			sc->sc_tx_done = true;
			handled = true;
		}
	}
	if (handled) {
		softint_schedule(sc->sc_si);
		rnd_add_uint32(&sc->sc_rndsource, (rndcsr << 8) | code);
	}
}

static void
wssoftintr(void *arg)
{
	struct ws_softc *sc = arg;
	uint8_t code;

	/* handle pending keyboard commands */
	if (sc->sc_tx_done) {
		int s;

		s = splserial();
		sc->sc_tx_done = false;
		DPRINTF(DEBUG_KBDTX, ("%s: tx complete\n", __func__));
		if (sc->sc_txqhead != sc->sc_txqtail) {
			struct sioreg *sio = sc->sc_ctl;

			sc->sc_tx_busy = true;
			sio->sio_data = sc->sc_txq[sc->sc_txqhead];
			DPRINTF(DEBUG_KBDTX,
			    ("%s: sio_data <- txq[%2d] (%02x)\n", __func__,
			    sc->sc_txqhead, sc->sc_txq[sc->sc_txqhead]));

			sc->sc_txqhead = OMKBD_NEXTTXQ(sc->sc_txqhead);
		}
		splx(s);
	}

	/* handle received keyboard and mouse data */
	while (sc->sc_rxqhead != sc->sc_rxqtail) {
		code = sc->sc_rxq[sc->sc_rxqhead];
		DPRINTF(DEBUG_RXSOFT, ("%s: %02x <- rxq[%2d]\n", __func__,
		    code, sc->sc_rxqhead));
		sc->sc_rxqhead = OMKBD_NEXTRXQ(sc->sc_rxqhead);
		/*
		 * if (code >= 0x80 && code <= 0x87), i.e.
		 * if ((code & 0xf8) == 0x80), then
		 * it's the first byte of 3 byte long mouse report
		 *	code[0] & 07 -> LMR button condition
		 *	code[1], [2] -> x,y delta
		 * otherwise, key press or release event.
		 */
		if (sc->sc_msreport == 1) {
#if NWSMOUSE > 0
			sc->sc_msdx = (int8_t)code;
#endif
			sc->sc_msreport = 2;
			continue;
		} else if (sc->sc_msreport == 2) {
#if NWSMOUSE > 0
			sc->sc_msdy = (int8_t)code;
			wsmouse_input(sc->sc_wsmousedev,
			    sc->sc_msbuttons, sc->sc_msdx, sc->sc_msdy, 0, 0,
			    WSMOUSE_INPUT_DELTA);
#endif
			sc->sc_msreport = 0;
			continue;
		}
		if ((code & 0xf8) == 0x80) {
#if NWSMOUSE > 0
			/* buttons: Negative logic to positive */
			code = ~code;
			/* LMR->RML: wsevent counts 0 for leftmost */
			sc->sc_msbuttons =
			    ((code & 1) << 2) | (code & 2) | ((code & 4) >> 2);
#endif
			sc->sc_msreport = 1;
			continue;
		}
		omkbd_input(sc, code);
	}
}

static void
omkbd_send(struct ws_softc *sc, uint8_t txdata)
{
	int s;

	if (!sc->sc_tx_busy) {
		struct sioreg *sio = sc->sc_ctl;

		DPRINTF(DEBUG_KBDTX,
		    ("%s: sio_data <- %02x\n", __func__, txdata));
		s = splserial();
		sc->sc_tx_busy = true;
		sio->sio_data = txdata;
		splx(s);
	} else {
		s = splsoftserial();
		sc->sc_txq[sc->sc_txqtail] = txdata;
		DPRINTF(DEBUG_KBDTX,
		    ("%s: txq[%2d] <- %02x\n", __func__,
		    sc->sc_txqtail, sc->sc_txq[sc->sc_txqtail]));
		sc->sc_txqtail = OMKBD_NEXTTXQ(sc->sc_txqtail);
		splx(s);
		softint_schedule(sc->sc_si);
	}
}

static void
omkbd_input(struct ws_softc *sc, int data)
{
	u_int type;
	int key;

	omkbd_decode(sc, data, &type, &key);

#ifdef WSDISPLAY_COMPAT_RAWKBD
	if (sc->sc_rawkbd) {
		uint8_t cbuf[2];
		int c, j = 0;

		c = omkbd_raw[key];
		if (c == 0x70 /* Kana */ ||
		    c == 0x3a /* CAP  */) {
			/* See comment in !sc->sc_rawkbd case */
			cbuf[0] = c;
			wskbd_rawinput(sc->sc_wskbddev, cbuf, 1);
			cbuf[0] = c | 0x80;
			wskbd_rawinput(sc->sc_wskbddev, cbuf, 1);
		} else
		if (c != 0x00) {
			/* fake extended scancode if necessary */
			if (c & 0x80)
				cbuf[j++] = 0xe0;
			cbuf[j] = c & 0x7f;
			if (type == WSCONS_EVENT_KEY_UP)
				cbuf[j] |= 0x80;
			j++;

			wskbd_rawinput(sc->sc_wskbddev, cbuf, j);
		}
	} else
#endif
	{
		if (sc->sc_wskbddev != NULL) {
			if (key == 0x0b /* Kana */ ||
			    key == 0x0e /* CAP  */) {
				/*
				 * LUNA's keyboard doesn't send any keycode
				 * when these modifier keys are released.
				 * Instead, it sends a pressed or released code
				 * per how each modifier LED status will be
				 * changed when the modifier keys are pressed.
				 * To handle this quirk in MI wskbd(4) layer,
				 * we have to send a faked
				 * "pressed and released" sequence here.
				 */
				wskbd_input(sc->sc_wskbddev,
				    WSCONS_EVENT_KEY_DOWN, key);
				wskbd_input(sc->sc_wskbddev,
				    WSCONS_EVENT_KEY_UP, key);
			} else {
				wskbd_input(sc->sc_wskbddev, type, key);
			}
		}
	}
}

static void
omkbd_decode(struct ws_softc *sc, int datain, u_int *type, int *dataout)
{

	*type = (datain & 0x80) ? WSCONS_EVENT_KEY_UP : WSCONS_EVENT_KEY_DOWN;
	*dataout = datain & 0x7f;
}

static void
omkbd_complex_buzzer(struct ws_softc *sc, struct wskbd_bell_data *wbd)
{
	uint8_t buzcmd;

	buzcmd = omkbd_get_buzcmd(sc, wbd, OMKBD_BUZZER_DEFAULT);
	omkbd_send(sc, buzcmd);
}

static uint8_t
omkbd_get_buzcmd(struct ws_softc *sc, struct wskbd_bell_data *wbd,
    uint8_t obuzcmd)
{
	u_int pitch, period;
	uint8_t buzcmd;

	pitch  = wbd->pitch;
	period = wbd->period;
	buzcmd = OMKBD_BUZZER;

	if ((wbd->which & WSKBD_BELL_DOPERIOD) == 0)
		buzcmd |= obuzcmd & OMKBD_BUZZER_PERIOD;
	else if (period >= 700)
		buzcmd |= OMKBD_BUZZER_700MS;
	else if (period >= 400)
		buzcmd |= OMKBD_BUZZER_400MS;
	else if (period >= 150)
		buzcmd |= OMKBD_BUZZER_150MS;
	else
		buzcmd |= OMKBD_BUZZER_40MS;

	if ((wbd->which & WSKBD_BELL_DOPITCH) == 0)
		buzcmd |= obuzcmd & OMKBD_BUZZER_PITCH;
	else if (pitch >= 6000)
		buzcmd |= OMKBD_BUZZER_6000HZ;
	else if (pitch >= 3000)
		buzcmd |= OMKBD_BUZZER_3000HZ;
	else if (pitch >= 1500)
		buzcmd |= OMKBD_BUZZER_1500HZ;
	else if (pitch >= 1000)
		buzcmd |= OMKBD_BUZZER_1000HZ;
	else if (pitch >= 600)
		buzcmd |= OMKBD_BUZZER_600HZ;
	else if (pitch >= 300)
		buzcmd |= OMKBD_BUZZER_300HZ;
	else if (pitch >= 150)
		buzcmd |= OMKBD_BUZZER_150HZ;
	else
		buzcmd |= OMKBD_BUZZER_100HZ;

	/* no volume control for buzzer on the LUNA keyboards */

	return buzcmd;
}

static void
ws_cngetc(void *cookie, u_int *type, int *data)
{
	struct ws_conscookie *conscookie = cookie;
	struct sioreg *sio = conscookie->cc_sio;
	struct ws_softc *sc = conscookie->cc_sc;	/* currently unused */
	int code;

	code = siogetc(sio);
	omkbd_decode(sc, code, type, data);
}

static void
ws_cnpollc(void *cookie, int on)
{
	struct ws_conscookie *conscookie = cookie;

	conscookie->cc_polling = on;
}

static void
ws_cnbell(void *cookie, u_int pitch, u_int period, u_int volume)
{
	struct ws_conscookie *conscookie = cookie;
	struct sioreg *sio = conscookie->cc_sio;
	struct ws_softc *sc = conscookie->cc_sc;	/* currently unused */
	struct wskbd_bell_data wbd;
	uint8_t buzcmd;

	/*
	 * XXX cnbell(9) man page should describe each args..
	 *     (it looks similar to the struct wskbd_bell_data)
	 * pitch:  bell frequency in hertz
	 * period: bell period in ms
	 * volume: bell volume as a percentage (0-100) (as spkr(4))
	 */
	wbd.which  = WSKBD_BELL_DOALL;
	wbd.period = period;
	wbd.pitch  = pitch;
	wbd.volume = volume;
	buzcmd = omkbd_get_buzcmd(sc, &wbd, OMKBD_BUZZER_DEFAULT);

	sioputc(sio, buzcmd);
}

/* EXPORT */ void
ws_cnattach(void)
{
	struct sioreg *sio_base;

	sio_base = (struct sioreg *)OBIO_SIO;
	ws_conscookie.cc_sio = &sio_base[1];	/* channel B */

	/* XXX need CH.B initialization XXX */

	wskbd_cnattach(&ws_consops, &ws_conscookie, &omkbd_keymapdata);
}

static int
omkbd_enable(void *cookie, int on)
{

	return 0;
}

static void
omkbd_set_leds(void *cookie, int leds)
{
	struct ws_softc *sc = cookie;
	uint8_t capsledcmd, kanaledcmd;

	if (sc == NULL) {
		/*
		 * This has been checked by the caller in wskbd(9) layer
		 * for the early console, but just for sanity.
		 */
		return;
	}

	sc->sc_leds = leds;
	if ((leds & WSKBD_LED_CAPS) != 0) {
		capsledcmd = OMKBD_LED_ON_CAPS;
	} else {
		capsledcmd = OMKBD_LED_OFF_CAPS;
	}

#if 0	/* no KANA lock support in wskbd */
	if ((leds & WSKBD_LED_KANA) != 0) {
		kanaledcmd = OMKBD_LED_ON_KANA;
	} else
#endif
	{
		kanaledcmd = OMKBD_LED_OFF_KANA;
	}

	if (sc->sc_conscookie->cc_polling != 0) {
		struct sioreg *sio = sc->sc_ctl;

		sioputc(sio, capsledcmd);
		sioputc(sio, kanaledcmd);
	} else {
		omkbd_send(sc, capsledcmd);
		omkbd_send(sc, kanaledcmd);
	}
}

static int
omkbd_ioctl(void *cookie, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct ws_softc *sc = cookie;
	struct wskbd_bell_data *wbd;

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_LUNA;
		return 0;
	case WSKBDIO_SETLEDS:
		omkbd_set_leds(cookie, *(int *)data);
		return 0;
	case WSKBDIO_GETLEDS:
		*(int *)data = sc->sc_leds;
		return 0;

	/*
	 * Note all WSKBDIO_*BELL ioctl(2)s except WSKBDIO_COMPLEXBELL
	 * are handled MI wskbd(4) layer.
	 * (wskbd_displayioctl() in src/sys/dev/wscons/wskbd.c)
	 */
	case WSKBDIO_COMPLEXBELL:
		wbd = data;
		DPRINTF(DEBUG_BUZZER,
		    ("%s: WSKBDIO_COMPLEXBELL: pitch = %d, period = %d\n",
		    __func__, wbd->pitch, wbd->period));
		omkbd_complex_buzzer(sc, wbd);
		return 0;

#ifdef WSDISPLAY_COMPAT_RAWKBD
	case WSKBDIO_SETMODE:
		sc->sc_rawkbd = *(int *)data == WSKBD_RAW;
		return 0;
	case WSKBDIO_GETMODE:
		*(int *)data = sc->sc_rawkbd;
		return 0;
#endif
	}
	return EPASSTHROUGH;
}

#if NWSMOUSE > 0

static int
omms_enable(void *cookie)
{
	struct ws_softc *sc = cookie;

	/* enable 3 byte long mouse reporting */
	omkbd_send(sc, OMKBD_MOUSE_ON);

	return 0;
}

/*ARGUSED*/
static int
omms_ioctl(void *cookie, u_long cmd, void *data, int flag, struct lwp *l)
{

	if (cmd == WSMOUSEIO_GTYPE) {
		*(u_int *)data = 0x19991005; /* XXX */
		return 0;
	}
	return EPASSTHROUGH;
}

static void
omms_disable(void *cookie)
{
	struct ws_softc *sc = cookie;

	omkbd_send(sc, OMKBD_MOUSE_OFF);
}
#endif

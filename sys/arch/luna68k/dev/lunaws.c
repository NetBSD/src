/* $NetBSD: lunaws.c,v 1.30 2014/07/20 11:14:56 tsutsui Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: lunaws.c,v 1.30 2014/07/20 11:14:56 tsutsui Exp $");

#include "opt_wsdisplay_compat.h"
#include "wsmouse.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/wscons/wsmousevar.h>

#include <luna68k/dev/omkbdmap.h>
#include <luna68k/dev/sioreg.h>
#include <luna68k/dev/siovar.h>

#include "ioconf.h"

#define OMKBD_RXQ_LEN		64
#define OMKBD_RXQ_LEN_MASK	(OMKBD_RXQ_LEN - 1)
#define OMKBD_NEXTRXQ(x)	(((x) + 1) & OMKBD_RXQ_LEN_MASK)

static const uint8_t ch1_regs[6] = {
	WR0_RSTINT,				/* Reset E/S Interrupt */
	WR1_RXALLS,				/* Rx per char, No Tx */
	0,					/* */
	WR3_RX8BIT | WR3_RXENBL,		/* Rx */
	WR4_BAUD96 | WR4_STOP1 | WR4_NPARITY,	/* Tx/Rx */
	WR5_TX8BIT | WR5_TXENBL,		/* Tx */
};

struct ws_softc {
	device_t	sc_dev;
	struct sioreg	*sc_ctl;
	uint8_t		sc_wr[6];
	device_t	sc_wskbddev;
	uint8_t		sc_rxq[OMKBD_RXQ_LEN];
	u_int		sc_rxqhead;
	u_int		sc_rxqtail;
#if NWSMOUSE > 0
	device_t	sc_wsmousedev;
	int		sc_msreport;
	int		sc_msbuttons, sc_msdx, sc_msdy;
#endif
	void		*sc_si;
	int		sc_rawkbd;
};

static void omkbd_input(void *, int);
static void omkbd_decode(void *, int, u_int *, int *);
static int  omkbd_enable(void *, int);
static void omkbd_set_leds(void *, int);
static int  omkbd_ioctl(void *, u_long, void *, int, struct lwp *);

static const struct wskbd_mapdata omkbd_keymapdata = {
	omkbd_keydesctab,
	KB_JP,
};
static const struct wskbd_accessops omkbd_accessops = {
	omkbd_enable,
	omkbd_set_leds,
	omkbd_ioctl,
};

void	ws_cnattach(void);
static void ws_cngetc(void *, u_int *, int *);
static void ws_cnpollc(void *, int);
static const struct wskbd_consops ws_consops = {
	ws_cngetc,
	ws_cnpollc,
};

#if NWSMOUSE > 0
static int  omms_enable(void *);
static int  omms_ioctl(void *, u_long, void *, int, struct lwp *);
static void omms_disable(void *);

static const struct wsmouse_accessops omms_accessops = {
	omms_enable,
	omms_ioctl,
	omms_disable,
};
#endif

static void wsintr(void *);
static void wssoftintr(void *);

static int  wsmatch(device_t, cfdata_t, void *);
static void wsattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ws, sizeof(struct ws_softc),
    wsmatch, wsattach, NULL, NULL);

extern int  syscngetc(dev_t);
extern void syscnputc(dev_t, int);

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

	setsioreg(sc->sc_ctl, WR0, sc->sc_wr[WR0]);
	setsioreg(sc->sc_ctl, WR4, sc->sc_wr[WR4]);
	setsioreg(sc->sc_ctl, WR3, sc->sc_wr[WR3]);
	setsioreg(sc->sc_ctl, WR5, sc->sc_wr[WR5]);
	setsioreg(sc->sc_ctl, WR0, sc->sc_wr[WR0]);
	setsioreg(sc->sc_ctl, WR1, sc->sc_wr[WR1]);

	syscnputc((dev_t)1, 0x20); /* keep quiet mouse */

	sc->sc_rxqhead = 0;
	sc->sc_rxqtail = 0;

	sc->sc_si = softint_establish(SOFTINT_SERIAL, wssoftintr, sc);

	aprint_normal("\n");

	a.console = (args->hwflags == 1);
	a.keymap = &omkbd_keymapdata;
	a.accessops = &omkbd_accessops;
	a.accesscookie = (void *)sc;
	sc->sc_wskbddev = config_found_ia(self, "wskbddev", &a, wskbddevprint);

#if NWSMOUSE > 0
	{
	struct wsmousedev_attach_args b;
	b.accessops = &omms_accessops;
	b.accesscookie = (void *)sc;
	sc->sc_wsmousedev =
	    config_found_ia(self, "wsmousedev", &b, wsmousedevprint);
	sc->sc_msreport = 0;
	}
#endif
}

/*ARGSUSED*/
static void
wsintr(void *arg)
{
	struct ws_softc *sc = arg;
	struct sioreg *sio = sc->sc_ctl;
	uint8_t code;
	int rr;

	rr = getsiocsr(sio);
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
		softint_schedule(sc->sc_si);
	}
	if ((rr & RR_TXRDY) != 0)
		sio->sio_cmd = WR0_RSTPEND;
	/* not capable of transmit, yet */
}

static void
wssoftintr(void *arg)
{
	struct ws_softc *sc = arg;
	uint8_t code;

	while (sc->sc_rxqhead != sc->sc_rxqtail) {
		code = sc->sc_rxq[sc->sc_rxqhead];
		sc->sc_rxqhead = OMKBD_NEXTRXQ(sc->sc_rxqhead);
#if NWSMOUSE > 0
		/*
		 * if (code >= 0x80 && code <= 0x87), then
		 * it's the first byte of 3 byte long mouse report
		 *	code[0] & 07 -> LMR button condition
		 *	code[1], [2] -> x,y delta
		 * otherwise, key press or release event.
		 */
		if (sc->sc_msreport == 0) {
			if (code < 0x80 || code > 0x87) {
				omkbd_input(sc, code);
				continue;
			}
			code = (code & 07) ^ 07;
			/* LMR->RML: wsevent counts 0 for leftmost */
			sc->sc_msbuttons = (code & 02);
			if ((code & 01) != 0)
				sc->sc_msbuttons |= 04;
			if ((code & 04) != 0)
				sc->sc_msbuttons |= 01;
			sc->sc_msreport = 1;
		} else if (sc->sc_msreport == 1) {
			sc->sc_msdx = (int8_t)code;
			sc->sc_msreport = 2;
		} else if (sc->sc_msreport == 2) {
			sc->sc_msdy = (int8_t)code;
			wsmouse_input(sc->sc_wsmousedev,
			    sc->sc_msbuttons, sc->sc_msdx, sc->sc_msdy, 0, 0,
			    WSMOUSE_INPUT_DELTA);

			sc->sc_msreport = 0;
		}
#else
		omkbd_input(sc, code);
#endif
	}
}

static void
omkbd_input(void *v, int data)
{
	struct ws_softc *sc = v;
	u_int type;
	int key;

	omkbd_decode(v, data, &type, &key);

#ifdef WSDISPLAY_COMPAT_RAWKBD
	if (sc->sc_rawkbd) {
		uint8_t cbuf[2];
		int c, j = 0;

		c = omkbd_raw[key];
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
		if (sc->sc_wskbddev != NULL)
			wskbd_input(sc->sc_wskbddev, type, key);
	}
}

static void
omkbd_decode(void *v, int datain, u_int *type, int *dataout)
{

	*type = (datain & 0x80) ? WSCONS_EVENT_KEY_UP : WSCONS_EVENT_KEY_DOWN;
	*dataout = datain & 0x7f;
}

static void
ws_cngetc(void *v, u_int *type, int *data)
{
	int code;

	code = syscngetc((dev_t)1);
	omkbd_decode(v, code, type, data);
}

static void
ws_cnpollc(void *v, int on)
{
}

/* EXPORT */ void
ws_cnattach(void)
{
	static int voidfill;

	/* XXX need CH.B initialization XXX */

	wskbd_cnattach(&ws_consops, &voidfill, &omkbd_keymapdata);
}

static int
omkbd_enable(void *v, int on)
{

	return 0;
}

static void
omkbd_set_leds(void *v, int leds)
{

#if 0
	syscnputc((dev_t)1, 0x10); /* kana LED on */
	syscnputc((dev_t)1, 0x00); /* kana LED off */
	syscnputc((dev_t)1, 0x11); /* caps LED on */
	syscnputc((dev_t)1, 0x01); /* caps LED off */
#endif
}

static int
omkbd_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
#ifdef WSDISPLAY_COMPAT_RAWKBD
	struct ws_softc *sc = v;
#endif

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_LUNA;
		return 0;
	case WSKBDIO_SETLEDS:
	case WSKBDIO_GETLEDS:
	case WSKBDIO_COMPLEXBELL:	/* XXX capable of complex bell */
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
omms_enable(void *v)
{
	struct ws_softc *sc = v;

	syscnputc((dev_t)1, 0x60); /* enable 3 byte long mouse reporting */
	sc->sc_msreport = 0;
	return 0;
}

/*ARGUSED*/
static int
omms_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{

	if (cmd == WSMOUSEIO_GTYPE) {
		*(u_int *)data = 0x19991005; /* XXX */
		return 0;
	}
	return EPASSTHROUGH;
}

static void
omms_disable(void *v)
{
	struct ws_softc *sc = v;

	syscnputc((dev_t)1, 0x20); /* quiet mouse */
	sc->sc_msreport = 0;
}
#endif

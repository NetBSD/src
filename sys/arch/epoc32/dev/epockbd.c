/*	$NetBSD: epockbd.c,v 1.2.2.3 2014/08/20 00:02:52 tls Exp $	*/
/*
 * Copyright (c) 2013 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: epockbd.c,v 1.2.2.3 2014/08/20 00:02:52 tls Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <epoc32/dev/epockbdvar.h>
#include <epoc32/dev/epockbdmap.h>

#define EPOCKBD_COLUMN0		8

#define EPOCKBD_SCAN_READ(sc) \
	bus_space_read_1((sc)->sc_scant, (sc)->sc_scanh, 0)
#define EPOCKBD_SCAN_WRITE(sc, cmd) \
	bus_space_write_1((sc)->sc_scant, (sc)->sc_scanh, 0, (cmd))
#define EPOCKBD_DATA_READ(sc) \
	bus_space_read_1((sc)->sc_datat, (sc)->sc_datah, 0)

#define EPOC2WS_KBD_DATA(r, c)	(((c) << 3) + (31 - __builtin_clz(r)) + 1)

static int epockbd_enable(void *, int);
static void epockbd_set_leds(void *, int);
static int epockbd_ioctl(void *, u_long, void *, int, struct lwp *);

static void epockbd_poll(void *);

static void epockbd_cngetc(void *, u_int *, int *);
static void epockbd_cnpollc(void *, int);

static struct wskbd_consops epockbd_consops = {
	epockbd_cngetc,
	epockbd_cnpollc,
	NULL
};

static struct wskbd_accessops epockbd_accessops = {
	epockbd_enable,
	epockbd_set_leds,
	epockbd_ioctl,
};

static struct wskbd_mapdata epockbd_mapdata = {
	epockbd_keydesctab,
	KB_UK,
};

static int is_console;

void
epockbd_attach(struct epockbd_softc *sc)
{
	struct wskbddev_attach_args aa;

	aprint_normal("\n");
	aprint_naive("\n");

	if (is_console)
		wskbd_cnattach(&epockbd_consops, sc, &epockbd_mapdata);

	callout_init(&sc->sc_c, 0);
	callout_reset(&sc->sc_c, hz, epockbd_poll, sc);
	sc->sc_flags |= EPOCKBD_FLAG_ENABLE;

	aa.console = is_console;
	aa.keymap = &epockbd_mapdata;
	aa.accessops = &epockbd_accessops;
	aa.accesscookie = sc;
	sc->sc_wskbddev = config_found(sc->sc_dev, &aa, wskbddevprint);
}

/*
 * wskbd(9) functions
 */

static int
epockbd_enable(void *v, int on)
{
	struct epockbd_softc *sc = v;

	if (on) {
		sc->sc_flags |= EPOCKBD_FLAG_ENABLE;
		callout_schedule(&sc->sc_c, hz / 20);
	} else {
		sc->sc_flags &= ~EPOCKBD_FLAG_ENABLE;
		callout_stop(&sc->sc_c);
	}
	return 0;
}

static void
epockbd_set_leds(void *v, int on)
{
	/* Nothing */
}

static int
epockbd_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_EPOC;
		return 0;

	case WSKBDIO_SETLEDS:
		return 0;

	case WSKBDIO_GETLEDS:
		*(int *)data = 0;
		return 0;

#ifdef WSDISPLAY_COMPAT_RAWKBD
	case WSKBDIO_SETMODE:
		*(int *)data == WSKBD_RAW;
		sc->sc_flags |= EPOCKBD_FLAG_RAW;

		/* Not yet...: return 0; */
#endif
	}
	return EPASSTHROUGH;
}

static void
epockbd_poll(void *v)
{
	struct epockbd_softc *sc = v;
	u_int type;
	int data;

	epockbd_cngetc(v, &type, &data);
	if (data != 0)
		wskbd_input(sc->sc_wskbddev, type, data);

	callout_schedule(&sc->sc_c, hz / 20);
}

void
epockbd_cnattach(void)
{

	is_console = 1;
}

static void
epockbd_cngetc(void *conscookie, u_int *type, int *data)
{
	struct epockbd_softc *sc = conscookie;
	uint8_t cmd, key, mask;
	int column, row;
#if 1
	/*
	 * For some machines which return a strange response, it calculates
	 * using this variable.
	 */
	int pc = 0, pr = 0;
#endif

	*type = 0;
	*data = 0;
	mask = (1 << sc->sc_kbd_nrow) - 1;
	for (column = 0; column < sc->sc_kbd_ncolumn; column++) {
		delay(16);
		cmd = EPOCKBD_SCAN_READ(sc);
		cmd &= ~0xf;
		cmd |= (EPOCKBD_COLUMN0 + column);
		EPOCKBD_SCAN_WRITE(sc, cmd);
		delay(16);
		key = EPOCKBD_DATA_READ(sc) & mask;
		if (sc->sc_state[column] != key) {
			row = sc->sc_state[column] ^ key;
			sc->sc_state[column] = key;
#if 1
			if (*data == 0 ||
			    (row == pr && pc == 0 &&
			     column == sc->sc_kbd_ncolumn - 1))
#else
			if (*data == 0)
#endif
			{
				if (key & row)
					*type = WSCONS_EVENT_KEY_DOWN;
				else
					*type = WSCONS_EVENT_KEY_UP;
				*data = EPOC2WS_KBD_DATA(row, column);
#if 1
				pc = column;
				pr = row;
#endif
			}
		}
	}
}

/* ARGSUSED */
static void
epockbd_cnpollc(void *conckookie, int on)
{
	/* Nothing */
}

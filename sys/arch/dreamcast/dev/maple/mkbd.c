/*	$NetBSD: mkbd.c,v 1.22.26.1 2007/03/12 05:47:36 rmind Exp $	*/

/*-
 * Copyright (c) 2001 Marcus Comstedt
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Marcus Comstedt.
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
__KERNEL_RCSID(0, "$NetBSD: mkbd.c,v 1.22.26.1 2007/03/12 05:47:36 rmind Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/systm.h>

#include "wskbd.h"

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <dreamcast/dev/maple/maple.h>
#include <dreamcast/dev/maple/mapleconf.h>
#include <dreamcast/dev/maple/mkbdvar.h>
#include <dreamcast/dev/maple/mkbdmap.h>

/*
 * Function declarations.
 */
static	int mkbdmatch(struct device *, struct cfdata *, void *);
static	void mkbdattach(struct device *, struct device *, void *);
static	int mkbddetach(struct device *, int);

int	mkbd_enable(void *, int);
void	mkbd_set_leds(void *, int);
int	mkbd_ioctl(void *, u_long, void *, int, struct lwp *);

struct wskbd_accessops mkbd_accessops = {
	mkbd_enable,
	mkbd_set_leds,
	mkbd_ioctl,
};

static void mkbd_intr(void *, struct maple_response *, int, int);

void	mkbd_cngetc(void *, u_int *, int *);
void	mkbd_cnpollc(void *, int);
int	mkbd_cnattach(void);

struct wskbd_consops mkbd_consops = {
	mkbd_cngetc,
	mkbd_cnpollc,
};

struct wskbd_mapdata mkbd_keymapdata = {
	mkbd_keydesctab,
	KB_JP,
};

static struct mkbd_softc *mkbd_console_softc;

static int mkbd_is_console;
static int mkbd_console_initted;

CFATTACH_DECL(mkbd, sizeof(struct mkbd_softc),
    mkbdmatch, mkbdattach, mkbddetach, NULL);

static int
mkbdmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct maple_attach_args *ma = aux;

	return ma->ma_function == MAPLE_FN_KEYBOARD ? MAPLE_MATCH_FUNC : 0;
}

static void
mkbdattach(struct device *parent, struct device *self, void *aux)
{
	struct mkbd_softc *sc = (struct mkbd_softc *) self;
	struct maple_attach_args *ma = aux;
#if NWSKBD > 0
	struct wskbddev_attach_args a;
#endif
	uint32_t kbdtype;

	sc->sc_parent = parent;
	sc->sc_unit = ma->ma_unit;

	kbdtype = maple_get_function_data(ma->ma_devinfo,
	    MAPLE_FN_KEYBOARD) >> 24;
	switch (kbdtype) {
	case 1:
		printf(": Japanese keyboard");
		mkbd_keymapdata.layout = KB_JP;
		break;
	case 2:
		printf(": US keyboard");
		mkbd_keymapdata.layout = KB_US;
		break;
	case 3:
		printf(": European keyboard");
		mkbd_keymapdata.layout = KB_UK;
		break;
	default:
		printf(": Unknown keyboard %d", kbdtype);
	}
	printf("\n");
#ifdef MKBD_LAYOUT
	/* allow user to override the default keymap */
	mkbd_keymapdata.layout = MKBD_LAYOUT;
#endif
#ifdef MKBD_SWAPCTRLCAPS
	/* allow user to specify swapctrlcaps with the default keymap */
	mkbd_keymapdata.layout |= KB_SWAPCTRLCAPS;
#endif

#if NWSKBD > 0
	if ((a.console = mkbd_is_console) != 0) {
		mkbd_is_console = 0;
		if (!mkbd_console_initted)
			wskbd_cnattach(&mkbd_consops, NULL, &mkbd_keymapdata);
		mkbd_console_softc = sc;
	}
	a.keymap = &mkbd_keymapdata;
	a.accessops = &mkbd_accessops;
	a.accesscookie = sc;
	sc->sc_wskbddev = config_found(self, &a, wskbddevprint);
#endif

	maple_set_callback(parent, sc->sc_unit, MAPLE_FN_KEYBOARD,
	    mkbd_intr, sc);
	maple_enable_periodic(parent, sc->sc_unit, MAPLE_FN_KEYBOARD, 1);
}

static int
mkbddetach(struct device *self, int flags)
{
	struct mkbd_softc *sc = (struct mkbd_softc *) self;
	int rv = 0;

	if (sc == mkbd_console_softc) {
		/*
		 * Hack to allow another Maple keyboard to be new console.
		 * XXX Should some other type device can be console.
		 */
		printf("%s: was console keyboard\n", sc->sc_dev.dv_xname);
		wskbd_cndetach();
		mkbd_console_softc = NULL;
		mkbd_console_initted = 0;
		mkbd_is_console = 1;
	}
	if (sc->sc_wskbddev)
		rv = config_detach(sc->sc_wskbddev, flags);

	return rv;
}

int
mkbd_enable(void *v, int on)
{

	return 0;
}

void
mkbd_set_leds(void *v, int on)
{
}

int
mkbd_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *) data = WSKBD_TYPE_MAPLE;
		return 0;
	case WSKBDIO_SETLEDS:
		return 0;
	case WSKBDIO_GETLEDS:
		*(int *) data = 0;
		return 0;
	case WSKBDIO_BELL:
	case WSKBDIO_COMPLEXBELL:
		return 0;
	}

	return EPASSTHROUGH;
}

int
mkbd_cnattach(void)
{

	wskbd_cnattach(&mkbd_consops, NULL, &mkbd_keymapdata);
	mkbd_console_initted = 1;
	mkbd_is_console = 1;

	return 0;
}

static int polledkey;
extern int maple_polling;

#define SHIFT_KEYCODE_BASE 0xe0
#define UP_KEYCODE_FLAG 0x1000

#define KEY_UP(n) do {							\
	if (maple_polling)						\
		polledkey = (n)|UP_KEYCODE_FLAG;			\
	else								\
		wskbd_input(sc->sc_wskbddev, WSCONS_EVENT_KEY_UP, (n));	\
	} while (/*CONSTCOND*/0)

#define KEY_DOWN(n) do {						\
	if (maple_polling)						\
		polledkey = (n);					\
	else								\
		wskbd_input(sc->sc_wskbddev, WSCONS_EVENT_KEY_DOWN, (n)); \
	} while (/*CONSTCOND*/0)

#define SHIFT_UP(n)	KEY_UP((n) | SHIFT_KEYCODE_BASE)
#define SHIFT_DOWN(n)	KEY_DOWN((n) | SHIFT_KEYCODE_BASE)

static void
mkbd_intr(void *arg, struct maple_response *response, int sz, int flags)
{
	struct mkbd_softc *sc = arg;
	struct mkbd_condition *kbddata = (void *) response->data;

	if ((flags & MAPLE_FLAG_PERIODIC) &&
	    sz >= sizeof(struct mkbd_condition)) {
		int i, j, v;

		v = sc->sc_condition.shift & ~kbddata->shift;
		if (v)
			for (i = 0; i < 8; i++)
				if (v & (1 << i))
					SHIFT_UP(i);

		v = kbddata->shift & ~sc->sc_condition.shift;
		if (v)
			for (i = 0; i < 8; i++)
				if (v & (1 << i))
					SHIFT_DOWN(i);

		for (i = 0, j = 0; i < 6; i++)
			if (sc->sc_condition.key[i] < 4)
				break;
			else if (sc->sc_condition.key[i] == kbddata->key[j])
				j++;
			else
				KEY_UP(sc->sc_condition.key[i]);

		for (; j < 6; j++)
			if (kbddata->key[j] < 4)
				break;
			else
				KEY_DOWN(kbddata->key[j]);

		memcpy(&sc->sc_condition, kbddata,
		    sizeof(struct mkbd_condition));
	}
}

void
mkbd_cngetc(void *v, u_int *type, int *data)
{
	int key;

	polledkey = -1;
	maple_polling = 1;
	while (polledkey == -1) {
		if (mkbd_console_softc != NULL ||
		    mkbd_console_softc->sc_parent != NULL) {
			int t;
			for (t = 0; t < 1000000; t++);
			maple_run_polling(mkbd_console_softc->sc_parent);
		}
	}
	maple_polling = 0;
	key = polledkey;

	*data = key & ~UP_KEYCODE_FLAG;
	*type = (key & UP_KEYCODE_FLAG) ?
	    WSCONS_EVENT_KEY_UP : WSCONS_EVENT_KEY_DOWN;
}

void
mkbd_cnpollc(void *v, int on)
{
}

/*	$NetBSD: mkbd.c,v 1.13 2002/03/25 18:59:40 uch Exp $	*/

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

int	mkbd_enable(void *, int);
void	mkbd_set_leds(void *, int);
int	mkbd_ioctl(void *, u_long, caddr_t, int, struct proc *);

struct wskbd_accessops mkbd_accessops = {
	mkbd_enable,
	mkbd_set_leds,
	mkbd_ioctl,
};

static void mkbd_intr(struct mkbd_softc *, struct mkbd_condition *, int);

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

static struct mkbd_softc *mkbd_console_softc = NULL;

static int mkbd_console_initted = 0;

/* Driver definition. */
struct cfattach mkbd_ca = {
	sizeof(struct mkbd_softc), mkbdmatch, mkbdattach
};

static int
mkbdmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct maple_attach_args *ma = aux;

	return ((ma->ma_function & MAPLE_FUNC_KEYBOARD) != 0);
}

static void
mkbdattach(struct device *parent, struct device *self, void *aux)
{
	struct mkbd_softc *sc = (struct mkbd_softc *) self;
	struct maple_attach_args *ma = aux;
#if NWSKBD > 0
	struct wskbddev_attach_args a;
#endif
	u_int32_t kbdtype;

	sc->sc_parent = parent;
	sc->sc_port = ma->ma_port;
	sc->sc_subunit = ma->ma_subunit;

	kbdtype = maple_get_function_data(ma->ma_devinfo,
	    MAPLE_FUNC_KEYBOARD) >> 24;
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

#if NWSKBD > 0
	a.console = ((sc->sc_dev.dv_unit == 0) && (mkbd_console_initted == 1))
	    ? 1 : 0;
	a.keymap = &mkbd_keymapdata;
	a.accessops = &mkbd_accessops;
	a.accesscookie = sc;
	if (a.console)
		mkbd_console_softc = sc;
	sc->sc_wskbddev = config_found(self, &a, wskbddevprint);
#endif

	maple_set_condition_callback(parent, sc->sc_port, sc->sc_subunit,
	    MAPLE_FUNC_KEYBOARD,(void (*) (void *, void *, int)) mkbd_intr, sc);
}



int
mkbd_enable(void *v, int on)
{

	return (0);
}

void
mkbd_set_leds(void *v, int on)
{
}

int
mkbd_ioctl(void *v, u_long cmd, caddr_t data, int flag, struct proc *p)
{

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *) data = WSKBD_TYPE_USB;	/* XXX */
		return (0);
	case WSKBDIO_SETLEDS:
		return (0);
	case WSKBDIO_GETLEDS:
		*(int *) data = 0;
		return (0);
	case WSKBDIO_BELL:
	case WSKBDIO_COMPLEXBELL:
		return (0);
	}

	return (EPASSTHROUGH);
}

int
mkbd_cnattach()
{

	wskbd_cnattach(&mkbd_consops, NULL, &mkbd_keymapdata);
	mkbd_console_initted = 1;

	return (0);
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
mkbd_intr(struct mkbd_softc *sc, struct mkbd_condition *kbddata, int sz)
{

	if (sz >= sizeof(struct mkbd_condition)) {
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
mkbd_cngetc(void *v, u_int *type,int *data)
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

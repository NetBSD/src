/*	$NetBSD: adb_bt.c,v 1.1.2.2 2007/05/07 10:55:24 yamt Exp $ */

/*-
 * Copyright (c) 2006 Michael Lorenz
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
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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
__KERNEL_RCSID(0, "$NetBSD: adb_bt.c,v 1.1.2.2 2007/05/07 10:55:24 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/wscons/wsmousevar.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <machine/autoconf.h>
#include <machine/keyboard.h>
#include <machine/adbsys.h>

#include <dev/adb/adbvar.h>
#include <dev/adb/adb_keymap.h>

#include "opt_wsdisplay_compat.h"
#include "adbdebug.h"

#ifdef ADBBT_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

#define BT_VOL_UP	0x06
#define BT_VOL_DOWN	0x07
#define BT_VOL_MUTE	0x08
#define BT_BRT_UP	0x09
#define BT_BRT_DOWN	0x0a
#define BT_EJECT	0x0b
#define BT_F7		0x0c
#define BT_NUMLOCK	0x7f

static int adbbt_match(struct device *, struct cfdata *, void *);
static void adbbt_attach(struct device *, struct device *, void *);

struct adbbt_softc {
	struct device sc_dev;
	struct adb_device *sc_adbdev;
	struct adb_bus_accessops *sc_ops;
	struct device *sc_wskbddev;
	int sc_msg_len;
	int sc_event;
	int sc_poll;
	int sc_polled_chars;
	int sc_rawkbd;
	uint8_t sc_buffer[16];
	uint8_t sc_pollbuf[16];
	uint8_t sc_trans[16];
	uint8_t sc_us;
};	

/* Driver definition. */
CFATTACH_DECL(adbbt, sizeof(struct adbbt_softc),
    adbbt_match, adbbt_attach, NULL, NULL);

extern struct cfdriver adbbt_cd;

static int adbbt_enable(void *, int);
static void adbbt_set_leds(void *, int);
static int adbbt_ioctl(void *, u_long, void *, int, struct lwp *);
static void adbbt_handler(void *, int, uint8_t *);

struct wskbd_accessops adbbt_accessops = {
	adbbt_enable,
	adbbt_set_leds,
	adbbt_ioctl,
};

struct wskbd_mapdata adbbt_keymapdata = {
	akbd_keydesctab,
#ifdef AKBD_LAYOUT
	AKBD_LAYOUT,
#else
	KB_US,
#endif
};

static int
adbbt_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void   *aux;
{
	struct adb_attach_args *aaa = aux;

	if ((aaa->dev->original_addr == ADBADDR_MISC) &&
	    (aaa->dev->handler_id == 0x1f))
		return 100;
	else
		return 0;
}

static void
adbbt_attach(struct device *parent, struct device *self, void *aux)
{
	struct adbbt_softc *sc = (struct adbbt_softc *)self;
	struct adb_attach_args *aaa = aux;
	struct wskbddev_attach_args a;

	sc->sc_ops = aaa->ops;
	sc->sc_adbdev = aaa->dev;
	sc->sc_adbdev->cookie = sc;
	sc->sc_adbdev->handler = adbbt_handler;
	sc->sc_us = ADBTALK(sc->sc_adbdev->current_addr, 0);

	sc->sc_msg_len = 0;
	sc->sc_rawkbd = 0;
	sc->sc_trans[6]  = 96;	/* F5  */  
	sc->sc_trans[7]  = 118;	/* F4  */
	sc->sc_trans[8]  = 99;	/* F3  */
	sc->sc_trans[9]  = 120;	/* F2  */
	sc->sc_trans[10] = 122;	/* F1  */
	sc->sc_trans[11] = 111;	/* F12 */

	printf(" addr %d: button device\n", sc->sc_adbdev->current_addr);

	a.console = 0;
	a.keymap = &adbbt_keymapdata;
	a.accessops = &adbbt_accessops;
	a.accesscookie = sc;

	sc->sc_wskbddev = config_found_ia(self, "wskbddev", &a, wskbddevprint);
}

static void
adbbt_handler(void *cookie, int len, uint8_t *data)
{
	struct adbbt_softc *sc = cookie;
	int type;
	uint8_t k, keyval, scancode;

#ifdef ADBBT_DEBUG
	int i;
	printf("%s: %02x - ", sc->sc_dev.dv_xname, sc->sc_us);
	for (i = 0; i < len; i++) {
		printf(" %02x", data[i]);
	}
	printf("\n");
#endif
	k = data[2];
	keyval = ADBK_KEYVAL(k);
	if ((keyval < 6) || (keyval > 0x0c))
		return;
	scancode = sc->sc_trans[keyval];

#ifdef WSDISPLAY_COMPAT_RAWKBD
	if (sc->sc_rawkbd) {
		char cbuf[2];
		int s;

		cbuf[0] = scancode | (k & 0x80);

		s = spltty();
		wskbd_rawinput(sc->sc_wskbddev, cbuf, 1);
		splx(s);
	} else {
#endif

	/* normal event */
	type = ADBK_PRESS(k) ? 
	    WSCONS_EVENT_KEY_DOWN : WSCONS_EVENT_KEY_UP;
	wskbd_input(sc->sc_wskbddev, type, scancode);
#ifdef WSDISPLAY_COMPAT_RAWKBD
	}
#endif
}

static void
adbbt_set_leds(void *cookie, int leds)
{
	/* we have no LEDs */
}

static int
adbbt_enable(void *v, int on)
{
	return 0;
}

static int
adbbt_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
#ifdef WSDISPLAY_COMPAT_RAWKBD
	struct adbbt_softc *sc = (struct adbbt_softc *) v;
#endif

	switch (cmd) {

	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_ADB;
		return 0;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	case WSKBDIO_SETMODE:
		sc->sc_rawkbd = *(int *)data == WSKBD_RAW;
		return 0;
#endif
	}

	return EPASSTHROUGH;
}

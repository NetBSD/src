/*	$NetBSD: dtkbd.c,v 1.10.12.1 2014/08/20 00:03:19 tls Exp $	*/

/*-
 * Copyright (c) 2002, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dtkbd.c,v 1.10.12.1 2014/08/20 00:03:19 tls Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/bus.h> 
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/systm.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/dec/wskbdmap_lk201.h>
#include <dev/tc/tcvar.h>

#include <pmax/tc/dtreg.h>
#include <pmax/tc/dtvar.h>

#include <pmax/pmax/cons.h>

struct dtkbd_softc {
	device_t	sc_dev;
	device_t	sc_wskbddev;
	int		sc_enabled;
};

int	dtkbd_match(device_t, cfdata_t, void *);
void	dtkbd_attach(device_t, device_t, void *);
int	dtkbd_enable(void *, int);
int	dtkbd_ioctl(void *, u_long, void *, int, struct lwp *);
void	dtkbd_cngetc(void *, u_int *, int *);
void	dtkbd_cnpollc(void *, int);
int	dtkbd_process_msg(struct dt_msg *, u_int *, int *);
void	dtkbd_handler(void *, struct dt_msg *);
void	dtkbd_set_leds(void *, int);

const struct wskbd_accessops dtkbd_accessops = {
	dtkbd_enable,
	dtkbd_set_leds,
	dtkbd_ioctl,
};

const struct wskbd_consops dtkbd_consops = {
	dtkbd_cngetc,
	dtkbd_cnpollc,
};

CFATTACH_DECL_NEW(dtkbd, sizeof(struct dtkbd_softc),
    dtkbd_match, dtkbd_attach, NULL, NULL);

const struct wskbd_mapdata dtkbd_keymapdata = {
	lkkbd_keydesctab,
#ifdef DTKBD_LAYOUT
	DTKBD_LAYOUT,
#else
	KB_US | KB_LK401,
#endif
};

int	dtkbd_isconsole;
uint8_t	dtkbd_map[10];
int	dtkbd_maplen;

int
dtkbd_match(device_t parent, cfdata_t cf, void *aux)
{
	struct dt_attach_args *dta;

	dta = aux;
	return (dta->dta_addr == DT_ADDR_KBD);
}

void
dtkbd_attach(device_t parent, device_t self, void *aux)
{
	struct dt_softc *dt;
	struct dtkbd_softc *sc;
	struct wskbddev_attach_args a;

	dt = device_private(parent);
	sc = device_private(self);
	sc->sc_dev = self;

	printf("\n");

	if (dt_establish_handler(dt, &dt_kbd_dv, sc, dtkbd_handler)) {
		printf("%s: unable to establish handler\n", device_xname(self));
		return;
	}

	sc->sc_enabled = 1;

	a.console = dtkbd_isconsole;
	a.keymap = &dtkbd_keymapdata;
	a.accessops = &dtkbd_accessops;
	a.accesscookie = sc;
	sc->sc_wskbddev = config_found(self, &a, wskbddevprint);
}

void
dtkbd_cnattach(void)
{

	dtkbd_isconsole = 1;
	dt_cninit();

	wskbd_cnattach(&dtkbd_consops, &dtkbd_map, &dtkbd_keymapdata);
}

int
dtkbd_enable(void *v, int on)
{
	struct dtkbd_softc *sc;

	sc = v;
	sc->sc_enabled = on;

	return (0);
}

void
dtkbd_cngetc(void *v, u_int *type, int *data)
{
	struct dt_msg msg;
	static u_int types[20];
	static int cnt, i, vals[20];

	while (i >= cnt) {
		for (;;) {
			if (dt_msg_get(&msg, 0) == DT_GET_DONE)
				if (msg.src == dt_kbd_addr &&
				    !DT_CTL_P(msg.ctl))
					break;
			DELAY(1000);
		}

		cnt = dtkbd_process_msg(&msg, types, vals);
		i = 0;
	}

	*type = types[i++];
	*data = vals[i++];
}

void
dtkbd_cnpollc(void *v, int on)
{

	/* XXX */
}

int
dtkbd_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_LK201;
		return 0;
	default:
		/* XXX */
		return (EPASSTHROUGH);
	}
}

void
dtkbd_set_leds(void *v, int state)
{

	/* XXX */
}

void
dtkbd_handler(void *cookie, struct dt_msg *msg)
{
	struct dtkbd_softc *sc;
	u_int types[20];
	int i, cnt, vals[20];

	sc = cookie;

	if (!sc->sc_enabled)
		return;

	cnt = dtkbd_process_msg(msg, types, vals);
	for (i = 0; i < cnt; i++)
		wskbd_input(sc->sc_wskbddev, types[i], vals[i]);
}

int
dtkbd_process_msg(struct dt_msg *msg, u_int *types, int *vals)
{
	u_int len, c, count;
	int i, j;

	len = DT_CTL_LEN(msg->ctl);

	if ((msg->body[0] < DT_KBD_KEY_MIN && msg->body[0] != DT_KBD_EMPTY) ||
	    len > 10) {
		printf("dtkbd0: error: %x %x %x\n", len, msg->body[0],
		    msg->body[1]);

		/*
		 * Fake an "all ups" to avoid the stuck key syndrome.
		 */
		msg->body[0] = DT_KBD_EMPTY;
		len = 1;
	}

	if (msg->body[0] == DT_KBD_EMPTY) {
		types[0] = WSCONS_EVENT_ALL_KEYS_UP;
		vals[0] = 0;
		dtkbd_maplen = 0;
		return (1);
	}

	count = 0;

	for (i = 0; i < len; i++) {
		c = msg->body[i];

		for (j = 0; j < dtkbd_maplen; j++)
			if (dtkbd_map[j] == c)
				break;

		if (j == dtkbd_maplen) {
			types[count] = WSCONS_EVENT_KEY_DOWN;
			vals[count] = c - MIN_LK201_KEY;
			count++;
		}
	}

	for (j = 0; j < dtkbd_maplen; j++) {
		c = dtkbd_map[j];

		for (i = 0; i < len; i++)
			if (msg->body[i] == c)
				break;

		if (i == len) {
			types[count] = WSCONS_EVENT_KEY_UP;
			vals[count] = c - MIN_LK201_KEY;
			count++;
		}
	}

	memcpy(dtkbd_map, msg->body, len);
	dtkbd_maplen = len;

	return (count);
}

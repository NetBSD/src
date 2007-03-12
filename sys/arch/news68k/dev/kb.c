/*	$NetBSD: kb.c,v 1.7.26.1 2007/03/12 05:49:38 rmind Exp $	*/

/*
 * Copyright (c) 2001 Izumi Tsutsui.
 * Copyright (c) 2000 Tsubai Masanari.
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kb.c,v 1.7.26.1 2007/03/12 05:49:38 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <machine/bus.h>

#include <arch/news68k/dev/kbvar.h>

/* #define KB_DEBUG */

void	kb_cngetc(void *, u_int *, int *);
void	kb_cnpollc(void *, int);
int	kb_enable(void *, int);
void	kb_set_leds(void *, int);
int	kb_ioctl(void *, u_long, void *, int, struct lwp *);

extern struct wscons_keydesc newskb_keydesctab[];

struct wskbd_accessops kb_accessops = {
	kb_enable,
	kb_set_leds,
	kb_ioctl,
};

struct wskbd_consops kb_consops = {
	kb_cngetc,
	kb_cnpollc,
};

struct wskbd_mapdata kb_keymapdata = {
	newskb_keydesctab,
	KB_JP,
};

void
kb_intr(struct kb_softc *sc)
{
	struct console_softc *kb_conssc = sc->sc_conssc;
	bus_space_tag_t bt = sc->sc_bt;
	bus_space_handle_t bh = sc->sc_bh;
	bus_size_t offset = sc->sc_offset;
	int key, val;
	u_int type;

	kb_conssc->cs_nkeyevents++;

	key = bus_space_read_1(bt, bh, offset);
	type = (key & 0x80) ? WSCONS_EVENT_KEY_UP : WSCONS_EVENT_KEY_DOWN;
	val = key & 0x7f;

#ifdef KB_DEBUG
	printf("kb_intr: key=%02x, type=%d, val=%02x\n", key, type, val);
#endif
	kb_conssc->cs_key = key;
	kb_conssc->cs_type = type;
	kb_conssc->cs_val = val;

	if (!kb_conssc->cs_polling)
		wskbd_input(sc->sc_wskbddev, type, val);
}

int
kb_cnattach(struct console_softc *conssc_p)
{

	wskbd_cnattach(&kb_consops, conssc_p, &kb_keymapdata);
	return 0;
}

void
kb_cngetc(void *v, u_int *type, int *data)
{
	struct console_softc *conssc = v;
	u_int nkey;

	/* set to polling mode */
	conssc->cs_polling = 1;

	/* wait until any keyevent occur */
	nkey = conssc->cs_nkeyevents;
	while (conssc->cs_nkeyevents == nkey)
		;

	/* get last keyevent */
	*data = conssc->cs_val;
	*type = conssc->cs_type;

	conssc->cs_polling = 0;
}

void
kb_cnpollc(void *v, int on)
{
}

int
kb_enable(void *v, int on)
{

	return 0;
}

void
kb_set_leds(void *v, int on)
{
}

int
kb_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
#if 0
	struct console_softc *cs = v;
#endif

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = 0;	/* XXX */
		return 0;
	case WSKBDIO_SETLEDS:
		return 0;
	case WSKBDIO_GETLEDS:
		*(int *)data = 0;
		return 0;
	case WSKBDIO_COMPLEXBELL:
		return 0;
	}

	return EPASSTHROUGH;
}

/*	$NetBSD: dtop.c,v 1.1.2.5 1999/05/11 06:43:14 nisimura Exp $ */

/*
 * Copyright (c) 1998, 1999 Tohru Nishimura.  All rights reserved.
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
 *      This product includes software developed by Tohru Nishimura
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <dev/cons.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsmousevar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/dec/wskbdmap_lk201.h>

#include <pmax/pmax/pmaxtype.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicvar.h>
#include <pmax/tc/ioasicreg.h>	/* XXX XXX XXX */
#include <pmax/pmax/maxine.h>

#include <dev/dec/lk201reg.h>

struct dtmessage {
	u_int8_t src;
	u_int8_t ctl;
	u_int8_t body[1];
};

struct lk501_state {
#define LK_KLL 8
	int down_keys_list[LK_KLL];
	int bellvol;
	int leds_state;
};

struct dtop_softc {
	struct device	sc_dv;
	struct device	*sc_wskbddev;
	struct device	*sc_wsmousedev;
	struct lk501_state *lk501_ks;

	bus_space_tag_t	sc_bst;
	bus_space_handle_t sc_bsh;

	u_int8_t	sc_xmit[256];
	u_int8_t	sc_recv[256];
	u_int8_t	*sc_xmitp, *sc_xtailp;
	u_int8_t	*sc_recvp, *sc_rtailp;
};

int  dtopmatch	__P((struct device *, struct cfdata *, void *));
void dtopattach	__P((struct device *, struct device *, void *));

struct cfattach dtop_ca = {
	sizeof(struct dtop_softc), dtopmatch, dtopattach
};

int  lk501_init __P((struct lk501_state *));
int  lk501_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
int  lk501_bell __P((struct lk501_state *, struct wskbd_bell_data *));
void lk501_set_leds __P((struct lk501_state *, int));
int  lk501_decode __P((struct lk501_state *, struct dtmessage *));
int  nop_enable __P((void *, int));

struct wskbd_accessops lk501_accessops = {
	nop_enable,
	(void (*)(void *, int))lk501_set_leds,
	lk501_ioctl,
};

struct wskbd_mapdata lk501_keymapdata = {
#if 1
	NULL
#else
	zskbd_keydesctab,	/* XXX mis-no-ner XXX */
#endif
	KB_US | KB_LK401,
};

void dtopkbd_cngetc __P((void *, u_int *, int *));
void dtopkbd_cnpollc __P((void *, int));
void dtopgetmsg __P((void *, struct dtmessage *));
void dtopputmsg __P((void *, struct dtmessage *));

const struct wskbd_consops dtopkbd_consops = {
	dtopkbd_cngetc,
	dtopkbd_cnpollc,
};

int wscons_dtop;
struct lk501_state dtopkbd_private;

int dtopintr __P((void *));
int dtop_decode __P((struct dtop_softc *, struct dtmessage *));
int dtopkbd_cnattach __P((paddr_t));		/* EXPORT */

int
dtopmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct ioasicdev_attach_args *d = aux;

	if (strcmp(d->iada_modname, "dtop") != 0)
		return 0;
	if (systype != DS_MAXINE)
		return 0;

	return 1;
}
void
dtopattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ioasicdev_attach_args *d = aux;
	struct dtop_softc *sc = (struct dtop_softc*)self;
	struct lk501_state *dti;

	ioasic_intr_establish(parent, d->iada_cookie, IPL_TTY, dtopintr, sc);
	printf("\n");

	if (wscons_dtop)
		dti = &dtopkbd_private;
	else {
		dti = malloc(sizeof(struct lk501_state), M_DEVBUF, M_NOWAIT);
		lk501_init(dti);
	}
	sc->lk501_ks = dti;

	sc->sc_bst = ((struct ioasic_softc *)parent)->sc_bst;
	sc->sc_bsh = ((struct ioasic_softc *)parent)->sc_bsh;
	sc->sc_xmitp = sc->sc_xtailp = sc->sc_xmit;
	sc->sc_recvp = sc->sc_rtailp = sc->sc_recv;

	{
	struct wskbddev_attach_args a;
	a.console = wscons_dtop;
	a.keymap = &lk501_keymapdata;
	a.accessops = &lk501_accessops;
	a.accesscookie = (void *)sc;
	sc->sc_wskbddev = config_found(self, &a, wskbddevprint);
	}

#if 0
	{
	struct wsmousedev_attach_args b;
	b.accessops = &dtopms_accessops;
	b.accesscookie = (void *)sc;
	sc->sc_wsmousedev = config_found(self, &b, wsmousedevprint);
	}
#endif
}

/* XXX needs to have smart packet receiver logic XXX */
#define	DTOP_IDLE		0
#define	DTOP_SRC_RECEIVED	1
#define	DTOP_BODY_RECEIVING	2
#define	DTOP_BODY_RECEIVED	3

#define	XINE_DTOP_DATA	IOASIC_SLOT_10_START

int
dtopintr(v)
	void *v;
{
	struct dtop_softc *sc = v;
	u_int32_t intr;
	int cc;

	intr = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_INTR);
	if (intr & XINE_INTR_DTOP_RX) {
		/*
		 * receive messages
		 */
		cc = bus_space_read_2(sc->sc_bst, sc->sc_bsh, XINE_DTOP_DATA);
		*sc->sc_rtailp++ = cc >> 8;
	}
	if (intr & XINE_INTR_DTOP_TX) {
		/*
		 * transmit messages
		 */
		cc = *sc->sc_xmitp++ << 8;
		bus_space_write_2(sc->sc_bst, sc->sc_bsh, XINE_DTOP_DATA, cc);
	}
	return 0;
}

int
dtop_decode(sc, pkt)
	struct dtop_softc *sc;
	struct dtmessage *pkt;
{
	if (pkt->src == 0x6c) {
		lk501_decode(sc->lk501_ks, pkt);
	}
	else if (pkt->src == 0x6a) {
		int buttons, x, y;

		buttons = pkt->body[0] << 8 | pkt->body[1];
		/* adjust button bit order; MRL -> LMR */
		if (buttons & 01)
			buttons |= 010;
		buttons = (buttons >> 1) & 03;

		x = pkt->body[2] << 8 | pkt->body[3];	/* sign extended */
		y = pkt->body[4] << 8 | pkt->body[5];	/* sign extended */

		wsmouse_input(sc->sc_wsmousedev, buttons, x, y, 0);
	}
	return 1;
}

int
lk501_decode(lks, pkt)
	struct lk501_state *lks;
	struct dtmessage *pkt;
{
	return 1;
}

int
lk501_init(lks)
	struct lk501_state *lks;
{
	lks->bellvol = -1;
	lks->leds_state = 0;

	return 0;
}

int
lk501_bell(lks, bell)
	struct lk501_state *lks;
	struct wskbd_bell_data *bell;
{
	unsigned int vol;

	if (bell->which & WSKBD_BELL_DOVOLUME) {
		vol = 8 - bell->volume * 8 / 100;
		if (vol > 7)
			vol = 7;
	} else
		vol = 3;

	if (vol != lks->bellvol) {
		/* build beep volume set packet and send it to LK501 */
		lks->bellvol = vol;
	}
	/* build "ring bell" packet and send it to LK501 */
	return 1;
}

void
lk501_set_leds(lks, leds)
	struct lk501_state *lks;
	int leds;
{
	unsigned newleds;

	newleds = 0;
	if (leds & WSKBD_LED_SCROLL)
		newleds |= LK_LED_WAIT;
	if (leds & WSKBD_LED_CAPS)
		newleds |= LK_LED_LOCK;

	/* build LED control packet and sent it to LK501 */
	lks->leds_state = leds;
}

int
lk501_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct dtop_softc *sc = v;

	switch (cmd) {
	    case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_LK401;
		return 0;
	    case WSKBDIO_SETLEDS:
		lk501_set_leds(sc->lk501_ks, *(int *)data);
		return 0;
	    case WSKBDIO_GETLEDS:
		*(int *)data = sc->lk501_ks->leds_state;
		return 0;
	    case WSKBDIO_BELL:
		lk501_bell(sc->lk501_ks, (struct wskbd_bell_data *)data);
		return 0;
	}
	return ENOTTY;
}

int
nop_enable(v, onoff)
	void *v;
	int onoff;
{
	return 1;
}

/* EXPORT */ int
dtopkbd_cnattach(addr)
	paddr_t addr; /* XXX IOASIC base XXX */
{
	struct lk501_state *dti;

	dti = &dtopkbd_private;
	lk501_init(dti);

	wskbd_cnattach(&dtopkbd_consops, dti, &lk501_keymapdata);
	wscons_dtop = 1;

	return 0;
}

/*ARGUSED*/
void
dtopkbd_cngetc(v, type, data)
	void *v;
	u_int *type;
	int *data;
{
	struct lk501_state *lks = v;
	struct dtmessage msg;

	do {
		dtopgetmsg(lks, &msg);
	} while (!lk501_decode(lks, &msg));
}

/*ARGUSED*/
void
dtopkbd_cnpollc(v, on)
	void *v;
	int on;
{
}

/*ARGUSED*/
void
dtopgetmsg(v, pkt)
	void *v;
	struct dtmessage *pkt;
{
}

/*ARGUSED*/
void
dtopputmsg(v, pkt)
	void *v;
	struct dtmessage *pkt;
{
}

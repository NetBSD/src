/* $NetBSD: dtop.c,v 1.1.2.6 1999/11/19 08:56:48 nisimura Exp $ */

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

#include "opt_ddb.h"	/* XXX TBD XXX */

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

#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicvar.h>
#include <pmax/tc/ioasicreg.h>
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
	u_int8_t msg[256];
};

struct dti_softc {
	struct device	sc_dv;
	struct lk501_state *lk501_ks;

	bus_space_tag_t	sc_bst;
	bus_space_handle_t sc_bsh;

	struct device	*sc_wskbddev;
	struct device	*sc_wsmousedev;

	/* xmit/recv msg buffer pool management here */
};

static int  dtimatch __P((struct device *, struct cfdata *, void *));
static void dtiattach __P((struct device *, struct device *, void *));

const struct cfattach dtop_ca = {
	sizeof(struct dti_softc), dtimatch, dtiattach
};

static int lk501_init __P((struct lk501_state *));
static int lk501_decode __P((struct lk501_state *,
				struct dtmessage *, u_int *, int *));
static int lk501_null __P((void));

const struct wskbd_accessops lk501_accessops = {
	(void *)lk501_null,
	(void *)lk501_null,
	(void *)lk501_null,
};

const struct wskbd_mapdata lk501_keymapdata = {
	zskbd_keydesctab,	/* XXX mis-no-ner XXX */
	KB_US | KB_LK401,
};

void dtikbd_cnattach __P((void));
static void dtikbd_cngetc __P((void *, u_int *, int *));
static void dtikbd_cnpollc __P((void *, int));
void dtikbd_input __P((struct dti_softc *, struct dtmessage *));

const struct wskbd_consops dtikbd_consops = {
	dtikbd_cngetc,
	dtikbd_cnpollc,
};

static int  dtims_enable __P((void *));
static int  dtims_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
static void dtims_disable __P((void *));
void dtims_input __P((struct dti_softc *, struct dtmessage *));

static const struct wsmouse_accessops dtims_accessops = { 
	dtims_enable,
	dtims_ioctl,
	dtims_disable,
};

static int wscons_dti;
static struct lk501_state dti_private;


static int
dtimatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct ioasicdev_attach_args *d = aux;

	if (strcmp(d->iada_modname, "dtop") != 0)
		return 0;

	return 1;
}

static void
dtiattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct dti_softc *sc = (void *)self;
	struct lk501_state *dti;
#if 0
	struct ioasicdev_attach_args *d = aux;
#endif

	printf(": unsable\n");

	if (wscons_dti)
		dti = &dti_private;
	else {
		dti = malloc(sizeof(struct lk501_state), M_DEVBUF, M_NOWAIT);
		lk501_init(dti);
	}
	sc->lk501_ks = dti;

	sc->sc_bst = ((struct ioasic_softc *)parent)->sc_bst;
	sc->sc_bsh = ((struct ioasic_softc *)parent)->sc_bsh;

#if 0
	Send a query and wait for a while any response

	foreach responder
	switch whoitis {
	case 0x6c: /* LK501 keyboard */ {
		struct wskbddev_attach_args a;
		a.console = wscons_dti;
		a.keymap = &lk501_keymapdata;
		a.accessops = &dtikbd_accessops;
		a.accesscookie = (void *)sc;
		sc->sc_wskbddev = config_found(self, &a, wskbddevprint);
	    	break;
		}
	case 0x6a: /* VSXXX-GB mouse */ {
		struct wsmousedev_attach_args b;
		b.accessops = &dtims_accessops;
		b.accesscookie = (void *)sc;
		sc->sc_wsmousedev = config_found(self, &b, wsmousedevprint);
		break;
		}
	}

	ioasic_intr_establish(parent, d->iada_cookie, IPL_TTY, dtiintr, sc);
#endif
}

/* EXPORT */ void
dtikbd_cnattach()
{
	struct lk501_state *dti;

	dti = &dti_private;
	lk501_init(dti);

	wskbd_cnattach(&dtikbd_consops, dti, &lk501_keymapdata);
	wscons_dti = 1;
}

static void
dtikbd_cngetc(v, type, data)
	void *v;
	u_int *type;
	int *data;
{
#if 0
	struct lk501_state *lks = v;
	struct dtmessage *pkt = lks->msg;

	do {
		do {
			dtimsgpoll(lks, pkt);
		} while (pkt->src != 0x6c);
	} while (!lk501_decode(lks, &msg, type, data));
#endif
}

void
dtikbd_cnpollc(v, on)
	void *v;
	int on;
{
}

/* ------------------------------------------------------- */

void
dtikbd_input(sc, data)
	struct dti_softc *sc;
	struct dtmessage *data;
{
	u_int type;
	int val;

	if (lk501_decode(sc->lk501_ks, data, &type, &val))
		wskbd_input(sc->sc_wskbddev, type, val);
}

static int
lk501_decode(lks, pkt, type, dataout)
	struct lk501_state *lks;
	struct dtmessage *pkt;
	u_int *type;
	int *dataout;
{
	return 1;
}

static int
lk501_init(lks)
	struct lk501_state *lks;
{
	lks->bellvol = -1;
	lks->leds_state = 0;

	return 0;
}

static int
lk501_null()
{
	return 1;
}

/* ------------------------------------------------------- */

static int
dtims_enable(v)
	void *v;
{
	return 0;
}

static void
dtims_disable(v)
	void *v;
{
}

/*ARGUSED*/
static int
dtims_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	if (cmd == WSMOUSEIO_GTYPE) {
		*(u_int *)data = WSMOUSE_TYPE_VSXXX;
		return 0;
	}
	return -1;
}

void
dtims_input(sc, pkt)
	struct dti_softc *sc;
	struct dtmessage *pkt;
{
	int buttons, x, y;

	buttons = pkt->body[0] << 8 | pkt->body[1];
	/* adjust button bit order; MRL -> LMR */
	if (buttons & 01)
		buttons |= 010;
	buttons = (buttons >> 1) & 03;

	x = pkt->body[2] << 8 | pkt->body[3];
	y = pkt->body[4] << 8 | pkt->body[5];

	wsmouse_input(sc->sc_wsmousedev, buttons, x, y, 0);
}

/* ------------------------------------------------------- */
#if 0

/* XXX needs to have smart packet receiver logic XXX */
#define	DTOP_IDLE		0
#define	DTOP_SRC_RECEIVED	1
#define	DTOP_BODY_RECEIVING	2
#define	DTOP_BODY_RECEIVED	3

#define	XINE_DTOP_DATA	IOASIC_SLOT_10_START

int
dtiintr(v)
	void *v;
{
	struct dti_softc *sc = v;
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
	return 1;
}

#endif

/* $NetBSD: dtop.c,v 1.1.2.7 1999/11/26 07:17:51 nisimura Exp $ */

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
#include <dev/dec/lk201var.h>
#include <dev/dec/vsxxxvar.h>

expand: illegal option -- a
usage: expand [-t tablist] [file ...]
		dtimsgpoll(lks, pkt);
	} while (pkt->src != 0x6c || !lk501_decode(lks, &msg, type, data));
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
	buttons &= 07;
	if (buttons & 01)
		buttons |= 010;
	buttons >>= 1;

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

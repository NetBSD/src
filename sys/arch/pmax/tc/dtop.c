/*	$NetBSD: dtop.c,v 1.1.2.2 1998/10/23 13:06:56 nisimura Exp $ */

/*
 * Copyright (c) 1996, 1998 Tohru Nishimura.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
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
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/time.h>
#include <sys/syslog.h>
#include <dev/cons.h>

#include <machine/autoconf.h>
#include <pmax/pmax/pmaxtype.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicvar.h>
#include <pmax/tc/ioasicreg.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsmousevar.h>

#include <dev/dec/lk201.h>
#include <dev/dec/lk201vsxxxvar.h>

/* XXX XXX XXX */
#include <pmax/pmax/maxine.h>
#define	DTOP_IDLE		0
#define	DTOP_SRC_RECEIVED	1
#define	DTOP_BODY_RECEIVING	2
#define	DTOP_BODY_RECEIVED	3
#define	DATA(x)	(*(x) >> 8)
/* XXX XXX XXX */

int wscons_dtop;

struct dtop_softc {
	struct device	sc_dv;

	u_int32_t	*sc_reg;
	u_int32_t	*sc_intr;
	struct device	*sc_wskbddev;
	struct device	*sc_wsmousedev;
	int		sc_mouse_enabled;
	int		sc_state;
	u_int8_t	sc_msgbuf[127 + 4];
	u_int8_t	*sc_rcvp;
	int		sc_len;
};
extern struct cfdriver ioasic_cd;

int  dtopmatch	__P((struct device *, struct cfdata *, void *));
void dtopattach	__P((struct device *, struct device *, void *));

struct cfattach dtop_ca = {
	sizeof(struct dtop_softc), dtopmatch, dtopattach
};

int dtopintr __P((void *));

int
dtopmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct ioasicdev_attach_args *d = aux;

	if (systype != DS_MAXINE)
		return (0);
	if (strcmp(d->iada_modname, "dtop") != 0)
		return (0);
	return (1);
}

void
dtopattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ioasicdev_attach_args *d = aux;
	struct dtop_softc *sc = (struct dtop_softc*)self;
#if 0
	struct wskbddev_attach_args a;
	struct wsmousedev_attach_args b;
#endif

	sc->sc_reg = (void *)(ioasic_base + IOASIC_SLOT_10_START);
	sc->sc_intr = (void *)(ioasic_base + IOASIC_INTR);

	sc->sc_state = 0;
	sc->sc_len = 0;
	sc->sc_rcvp = sc->sc_msgbuf;

	ioasic_intr_establish(parent,
			d->iada_cookie, TC_IPL_TTY, dtopintr, sc);
	printf("\n");

#if 0
	a.console = wscons_dtop;
	a.keymap = &lk201_keymapdata;
	a.accessops = &lk201_accessops;
	a.accesscookie = (void *)sc;

	b.accessops = &vsxxx_accessops;
	b.accesscookie = (void *)sc;

	sc->sc_wskbddev = config_found(self, &a, wskbddevprint);
	sc->sc_wsmousedev = config_found(self, &b, wsmousedevprint);
#endif
}

struct packet {
	u_int8_t src;
	u_int8_t ctl;
	u_int8_t body[1];
};

int
dtopintr(v)
	void *v;
{
	struct dtop_softc *sc = v;
#if 1 
	int npoll;

	npoll = 20;
	do {
		switch (sc->sc_state) {
		case DTOP_IDLE:
			*sc->sc_rcvp++ = DATA(sc->sc_reg);
			sc->sc_state = DTOP_SRC_RECEIVED;
			break;

		case DTOP_SRC_RECEIVED:
			*sc->sc_rcvp++ = sc->sc_len = DATA(sc->sc_reg);
			if (sc->sc_len == 0xf8)
				sc->sc_len = 0;
			sc->sc_len &= 0x7f;
			sc->sc_len += 1;
			sc->sc_state = DTOP_BODY_RECEIVING;
			break;

		case DTOP_BODY_RECEIVING:
			*sc->sc_rcvp++ = DATA(sc->sc_reg);
			if (--sc->sc_len == 0)
				goto gotall;
			break;
		}
	} while (--npoll > 0 && (*sc->sc_intr & XINE_INTR_DTOP_RX));
	return 1;

gotall:
#else
	*sc->sc_rcvp++ = DATA(sc->sc_reg);
#endif
	sc->sc_state = DTOP_IDLE;
	sc->sc_rcvp = sc->sc_msgbuf;

	{
	int i;
	struct packet *pkt = (void *)sc->sc_msgbuf;
	npoll = (pkt->ctl == 0xf8) ? 1 : pkt->ctl & 0x7f;
	printf("src=%d ctl=%x", pkt->src, pkt->ctl);
	for (i = 0; i < npoll; i++)
		printf(" %02x", pkt->body[i]);
	printf("\n");
	}

	return 1;
}

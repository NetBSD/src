/* $NetBSD: dckbd.c,v 1.1.2.4 1999/11/25 08:57:49 nisimura Exp $ */

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dckbd.c,v 1.1.2.4 1999/11/25 08:57:49 nisimura Exp $");

/*
 * WSCONS attachments for LK201 and DC7085 combo
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/dec/wskbdmap_lk201.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/dec/lk201reg.h>
#include <dev/dec/lk201var.h>

#include <pmax/ibus/dc7085reg.h>
#include <pmax/ibus/dc7085var.h>

#include "locators.h"

static struct lk201_state dckbd_private;
static int wscons_dc;

static int  dckbd_match __P((struct device *, struct cfdata *, void *));
static void dckbd_attach __P((struct device *, struct device *, void *));

const struct cfattach dckbd_ca = {
	sizeof(struct lkkbd_softc), dckbd_match, dckbd_attach,
};
extern struct cfdriver lkkbd_cd;


int dckbd_cnattach __P((paddr_t));		/* EXPORT */
extern int  dc_cngetc __P((dev_t));		/* IMPORT */
extern void dc_cnputc __P((dev_t, int));	/* IMPORT */
extern void dc_cninit __P((paddr_t, int, int));	/* IMPORT */

static void dckbd_input __P((int));
static void dckbd_cngetc __P((void *, u_int *, int *));
static void dckbd_cnpollc __P((void *, int));

const struct wskbd_consops dckbd_consops = {
	dckbd_cngetc,
	dckbd_cnpollc,
};

const struct wskbd_mapdata lk201_keymapdata = {
	zskbd_keydesctab,
	KB_US | KB_LK401,
};


static int
dckbd_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct dc_attach_args *args = aux;

	/* Exact match is better than wildcard. */
	if (cf->cf_loc[DCCF_LINE] == args->line)
		return 2;

	/* This driver accepts wildcard. */
	if (cf->cf_loc[DCCF_LINE] == DCCF_LINE_DEFAULT)
		return 1;

	return 0;
}

static void
dckbd_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct dc_softc *dc = (void *)parent;
	struct lkkbd_softc *dckbd = (void *)self;
	struct lk201_state *dci;
	struct wskbddev_attach_args a;

	if (wscons_dc)
		dci = &dckbd_private;
	else {
		dc_cninit((paddr_t)dc->sc_bsh, LKKBD, 4800);
		dci = malloc(sizeof(struct lk201_state), M_DEVBUF, M_NOWAIT);
		dci->attmt.sendchar = (void *)dc_cnputc;
		dci->attmt.cookie = (dev_t)LKKBD;
		lk201_init(dci);
	}
	dckbd->lk201_ks = dci;

#if 0
	initialize has done; this channel is 4800N8.
	send LK_REQ_ID (0xab) and wait for the reply of 2 octets for a while.
	if a valid reply was not received in the timeout period,
		mark this channel unavailable and return complaining it.
	the pair of two octets tells the keyboard ID.
#endif
	printf("\n");

	dckbd->kbd_type = WSKBD_TYPE_LK201;

	a.console = wscons_dc;
	a.keymap = &lk201_keymapdata;
	a.accessops = &lk201_accessops;
	a.accesscookie = (void *)dckbd;

	dckbd->sc_wskbddev = config_found(self, &a, wskbddevprint);

	dc->sc_wscons[LKKBD] = dckbd_input;
}

static void
dckbd_input(cc)
	int cc;
{
	struct lkkbd_softc *sc = (void *)lkkbd_cd.cd_devs[0];
	u_int type;
	int val;

	if (lk201_decode(sc->lk201_ks, cc, &type, &val))
		wskbd_input(sc->sc_wskbddev, type, val);
}

/* EXPORT */ int
dckbd_cnattach(addr)
	paddr_t addr;
{
	struct lk201_state *dci;

	dc_cninit(addr, LKKBD, 4800);

	dci = &dckbd_private;
	dci->attmt.sendchar = (void *)dc_cnputc;
	dci->attmt.cookie = (void *)LKKBD;
	lk201_init(dci);

	wskbd_cnattach(&dckbd_consops, dci, &lk201_keymapdata);
	wscons_dc = 1;

	return 0;
}

static void
dckbd_cngetc(v, type, data)
	void *v;
	u_int *type;
	int *data;
{
	struct lk201_state *lks = v;
	int c;

	do {
		c = dc_cngetc((dev_t)LKKBD);
	} while (!lk201_decode(lks, c, type, data));
}

static void
dckbd_cnpollc(v, on)
	void *v;
	int on;
{
}

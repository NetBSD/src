/* $NetBSD: dckbd.c,v 1.1.2.2 1999/04/17 13:45:53 nisimura Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: dckbd.c,v 1.1.2.2 1999/04/17 13:45:53 nisimura Exp $");

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
#include <pmax/ibus/dc7085reg.h>	/* XXX dev/ic/dc7085reg.h XXX */
#include <pmax/ibus/dc7085var.h>	/* XXX machine/dc7085var.h XXX */

#include "locators.h"

static struct lk201_state dckbd_private;
static int wscons_dc;

static int  dckbd_match __P((struct device *, struct cfdata *, void *));
static void dckbd_attach __P((struct device *, struct device *, void *));
static void dckbd_cngetc __P((void *, u_int *, int *));
static void dckbd_cnpollc __P((void *, int));

const struct cfattach dckbd_ca = {
	sizeof(struct lkkbd_softc), dckbd_match, dckbd_attach,
};

const struct wskbd_consops dckbd_consops = {
	dckbd_cngetc,
	dckbd_cnpollc,
};

int dckbd_cnattach __P((paddr_t));			/* EXPORT */

extern int  dcgetc __P((struct dc7085reg *, int));	/* IMPORT */
extern void dcputc __P((struct dc7085reg *, int, int));	/* IMPORT */

static void dckbd_input __P((void *, int));
/*XXX*/ int xxxgetc __P((void *));
static void xxxputc __P((void *, int));

static int
dckbd_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	if (strcmp(CFNAME(parent), "dc") != 0)
		return 0;
	if (cf->cf_loc[DCCF_LINE] != DCCF_LINE_DEFAULT) /* XXX correct? XXX */
		return 0;
#ifdef later
	initialize line #1 with 4800N8
	send a query and wait for the reply for a while
	if a valid reply with keyboard ID was received,
		return 1
	return 0;
#else
	return 1;
#endif
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
		dci = malloc(sizeof(struct lk201_state), M_DEVBUF, M_NOWAIT);
		dci->attmt.sendchar = (int (*)(void *, u_char))xxxputc;
		dci->attmt.cookie = /* XXX */ (void *)dc->sc_bsh;
		lk201_init(dci);
	}
	dckbd->lk201_ks = dci;

	printf("\n");
#if 0
	s = spltty();
        reg->dccsr = DC_CLR;
        delay(100);
        reg->dctcr = LKKBD;
        reg->dclpr = DC_RE | DC_B4800 | DC_BITS8 | LKKBD << 3;
        reg->dccsr = DC_MSE;	/* wrong? */
	splx(s);
#endif

	dckbd->kbd_type = WSKBD_TYPE_LK201;

	a.console = wscons_dc;
	a.keymap = &lk201_keymapdata;
	a.accessops = &lk201_accessops;
	a.accesscookie = (void *)dckbd;

	dckbd->sc_wskbddev = config_found(self, &a, wskbddevprint);

	dc->sc_line[LKKBD].f = dckbd_input;	/* XXX */
	dc->sc_line[LKKBD].a = (void *)dckbd;	/* XXX */
}

static void
dckbd_input(v, cc)
	void *v;
	int cc;
{
	struct lkkbd_softc *sc = v;
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
	struct dc7085reg *reg = (void *)MIPS_PHYS_TO_KSEG1(addr);

        reg->dccsr = DC_CLR;
        delay(100);
        reg->dctcr = LKKBD;
        reg->dclpr = DC_RE | DC_B4800 | DC_BITS8 | LKKBD << 3;
        reg->dccsr = DC_MSE;

	dci = &dckbd_private;
	dci->attmt.sendchar = (int (*)(void *, u_char))xxxputc;
	dci->attmt.cookie = (void *)reg;
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
		c = dcgetc((struct dc7085reg *)lks->attmt.cookie, LKKBD);
	} while (!lk201_decode(lks, c, type, data));
}

static void
dckbd_cnpollc(v, on)
	void *v;
	int on;
{
}

/*XXX*/ int
xxxgetc(v)
	void *v;
{
	return dcgetc((struct dc7085reg *)v, LKKBD);
}

static void
xxxputc(v, c)
	void *v;
	int c;
{
	dcputc((struct dc7085reg *)v, LKKBD, c);
}

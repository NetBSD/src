/*	$NetBSD: lkc.c,v 1.1 1998/06/04 15:51:12 ragge Exp $ */
/*
 * Copyright (c) 1998 Ludd, University of Lule}, Sweden.
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
 *      This product includes software developed at Ludd, University of 
 *      Lule}, Sweden and its contributors.
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




#include <sys/types.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/wscons/wscons_callbacks.h>

#include <dev/dec/lk201.h>

#include <machine/vsbus.h>

#include <vax/uba/dzreg.h>
#include <vax/uba/dzvar.h>

#include "ioconf.h"

static  int lkc_match __P((struct device *, struct cfdata *, void *));
static  void lkc_attach __P((struct device *, struct device *, void *));
static	int lkc_catch __P((int, int));

struct  lkc_softc {
	struct  device ls_dev;
	int	ls_shifted;	/* Shift key pressed */
	int	ls_ctrl;	/* Ctrl key pressed */
	int	ls_lastchar;	/* last key pressed (for repeat) */
};

struct cfattach lkc_ca = {
	sizeof(struct lkc_softc), lkc_match, lkc_attach,
};

int
lkc_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

void
lkc_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct dz_softc *dz = (void *)parent;

	printf("\n");
	dz->sc_catch = lkc_catch; /* Catch keyb & mouse chars fast */
	*dz->sc_dr.dr_lpr = 0x1c18; /* XXX */
	vsbus_intr_enable(INR_SR);
}

int
lkc_catch(line, ch)
	int line, ch;
{
	struct lkc_softc *ls = lkc_cd.cd_devs[0]; /* XXX */
	extern unsigned short q_key[], q_shift_key[];
	extern char *q_special[];
	u_short hej;
	char c, *cp;

	if (line > 1)
		return 0;

	switch (ch) {
	case KEY_SHIFT:
		ls->ls_shifted ^= 1;
		return 1;

	case KEY_UP:
		ls->ls_shifted = ls->ls_ctrl = 0;
		return 1;

	case KEY_CONTROL:
		ls->ls_ctrl ^= 1;
		return 1;

	case KEY_REPEAT:
		ch = ls->ls_lastchar;
		break;

	default:
		break;
	}

	hej = (ls->ls_shifted ? q_shift_key[ch] : q_key[ch]);
	c = hej;
	ls->ls_lastchar = ch;

	if (ls->ls_ctrl && c > 64)
		c &= 31;

	if (hej > 255) {
		cp = q_special[hej & 255];
		wsdisplay_kbdinput(wsdisplay_cd.cd_devs[0], cp, strlen(cp));
	} else
		wsdisplay_kbdinput(wsdisplay_cd.cd_devs[0], &c, 1);
	return 1;
}

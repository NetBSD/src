/*	$NetBSD: lkc.c,v 1.13 2002/02/25 14:58:09 ad Exp $ */
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

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/wscons/wsksymvar.h>
#include <dev/wscons/wscons_callbacks.h>

#include <dev/dec/lk201.h>

#include <machine/vsbus.h>

#include <dev/dec/dzreg.h>
#include <dev/dec/dzvar.h>

#include "ioconf.h"

static  int lkc_match(struct device *, struct cfdata *, void *);
static  void lkc_attach(struct device *, struct device *, void *);
static	int lkc_catch(int, int);
	int lkc_decode(int);

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
	bus_space_write_2(dz->sc_iot, dz->sc_ioh, 4, 0x1c18);
}

extern	char *q_special[];

int
lkc_catch(line, ch)
	int line, ch;
{
	int hej, i;
	char *cp;

	if (line > 0)
		return 0;

	if ((hej = lkc_decode(ch)) == -1)
		return 1;

	if (hej > 255) {
		cp = q_special[hej & 255];
		for (i = 0; i < strlen(cp); i++)
			wsdisplay_kbdinput(wsdisplay_cd.cd_devs[0], cp[i]);
	} else {
		wsdisplay_kbdinput(wsdisplay_cd.cd_devs[0], (keysym_t)hej);
	}
	return 1;
}

int
lkc_decode(ch)
	int ch;
{
	extern unsigned short q_key[], q_shift_key[];
	static int shifted, ctrl, lastchar;
	int hej;

	switch (ch) {
	case KEY_SHIFT:
		shifted ^= 1;
		return -1;

	case KEY_UP:
		shifted = ctrl = 0;
		return -1;

	case KEY_CONTROL:
		ctrl ^= 1;
		return -1;

	case KEY_REPEAT:
		ch = lastchar;
		break;

#if defined(DDB)
	case 113: /* ESC */
		if ((shifted & ctrl) == 0)
			break;
		Debugger();
		return -1;
#endif
	case 86:
	case 87:
	case 88:
	case 89:
	case 90:
	case 100:
	case 101:
	case 102:
		if ((shifted & ctrl) == 0)
			break;
		ch -= 86;
		if (ch > 10)
			ch -= 9;
		wsdisplay_switch(wsdisplay_cd.cd_devs[0], ch, 0);
		return -1;

	default:
		break;
	}

	hej = (shifted ? q_shift_key[ch] : q_key[ch]);
	lastchar = ch;

	if (ctrl)
		hej &= 31;

	return hej;
}


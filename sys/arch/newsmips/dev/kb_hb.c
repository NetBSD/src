/*	$NetBSD: kb_hb.c,v 1.7 2003/07/15 02:59:29 lukem Exp $	*/

/*-
 * Copyright (c) 2000 Tsubai Masanari.  All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: kb_hb.c,v 1.7 2003/07/15 02:59:29 lukem Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <machine/adrsmap.h>

#include <newsmips/dev/hbvar.h>

struct kbreg {
	u_char kb_data;
	u_char kb_stat;
	u_char kb_reset;
	u_char kb_init;
};

struct kb_hb_softc {
	struct device sc_dev;
	volatile struct kbreg *sc_reg;
	struct device *sc_wskbddev;
};

int kb_hb_match(struct device *, struct cfdata *, void *);
void kb_hb_attach(struct device *, struct device *, void *);
int kb_hb_intr(void *);

void kb_hb_cnattach(void);
void kb_hb_cngetc(void *, u_int *, int *);
void kb_hb_cnpollc(void *, int);

int kb_hb_enable(void *, int);
void kb_hb_setleds(void *, int);
int kb_hb_ioctl(void *, u_long, caddr_t, int, struct proc *);

extern struct wscons_keydesc newskb_keydesctab[];

CFATTACH_DECL(kb_hb, sizeof(struct kb_hb_softc),
    kb_hb_match, kb_hb_attach, NULL, NULL);

struct wskbd_accessops kb_hb_accessops = {
	kb_hb_enable,
	kb_hb_setleds,
	kb_hb_ioctl
};

struct wskbd_consops kb_hb_consops = {
	kb_hb_cngetc,
	kb_hb_cnpollc
};

struct wskbd_mapdata kb_hb_keymapdata = {
	newskb_keydesctab,
	KB_JP
};

int
kb_hb_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct hb_attach_args *ha = aux;

	if (strcmp(ha->ha_name, "kb") == 0)
		return 1;

	return 0;
}

void
kb_hb_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct kb_hb_softc *sc = (void *)self;
	struct hb_attach_args *ha = aux;
	volatile struct kbreg *reg;
	volatile int *dipsw = (void *)DIP_SWITCH;
	struct wskbddev_attach_args aa;
	int intr, cons;

	reg = (struct kbreg *)ha->ha_addr;
	intr = ha->ha_level;

	if (intr == -1)
		intr = 2;

	sc->sc_reg = reg;
	reg->kb_reset = 0x01;
	reg->kb_init = 0xf0;	/* 9600 bps */

	printf(" level %d", intr);
	cons = 0;
	if (*dipsw & 7) {
		cons = 1;
		printf(" (console)");
	}
	printf("\n");

	hb_intr_establish(intr, INTEN0_KBDINT, IPL_TTY, kb_hb_intr, sc);

	aa.console = cons;
	aa.keymap = &kb_hb_keymapdata;
	aa.accessops = &kb_hb_accessops;
	aa.accesscookie = sc;
	sc->sc_wskbddev = config_found(self, &aa, wskbddevprint);
}

int
kb_hb_intr(v)
	void *v;
{
	struct kb_hb_softc *sc = v;
	volatile struct kbreg *reg = sc->sc_reg;
	volatile u_char *ien = (void *)INTEN0;
	int code, type, release, val;
	int rv = 0;

	*ien &= ~RX_KBINTE;

	while (reg->kb_stat & RX_KBRDY) {
		code = reg->kb_data;
		release = code & 0x80;
		val = code & 0x7f;
		type = release ? WSCONS_EVENT_KEY_UP : WSCONS_EVENT_KEY_DOWN;
		if (sc->sc_wskbddev)
			wskbd_input(sc->sc_wskbddev, type, val);
		rv = 1;
	}

	*ien |= RX_KBINTE;
	return rv;
}

void
kb_hb_cnattach()
{
	volatile int *dipsw = (void *)DIP_SWITCH;
	volatile struct kbreg *reg = (void *)KEYB_DATA;

	if (*dipsw & 7)
		wskbd_cnattach(&kb_hb_consops, (void *)reg, &kb_hb_keymapdata);
}

void
kb_hb_cngetc(v, type, data)
	void *v;
	u_int *type;
	int *data;
{
	volatile struct kbreg *reg = v;
	volatile u_char *ien = (void *)INTEN0;
	int code, release, ointr;

	ointr = *ien & RX_KBINTE;
	*ien &= ~RX_KBINTE;

	/* Wait for key data. */
	while ((reg->kb_stat & RX_KBRDY) == 0);

	code = reg->kb_data;
	release = code & 0x80;
	*data = code & 0x7f;
	*type = release ? WSCONS_EVENT_KEY_UP : WSCONS_EVENT_KEY_DOWN;

	*ien |= ointr;
}

void
kb_hb_cnpollc(v, on)
	void *v;
	int on;
{
}

int
kb_hb_enable(v, on)
	void *v;
	int on;
{
	return 0;
}

void
kb_hb_setleds(v, on)
	void *v;
	int on;
{
}

int
kb_hb_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	switch (cmd) {

	case WSKBDIO_GTYPE:
		*(int *)data = 0;		/* XXX */
		return 0;
	case WSKBDIO_SETLEDS:
		return 0;
	case WSKBDIO_GETLEDS:
		*(int *)data = 0;
		return 0;
	}

	return EPASSTHROUGH;
}

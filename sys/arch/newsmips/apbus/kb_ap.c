/*	$NetBSD: kb_ap.c,v 1.1.10.1 2002/04/01 07:41:39 nathanw Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>

#include <machine/adrsmap.h>
#include <newsmips/apbus/apbusvar.h>

struct kbreg {
	u_int kb_rx_data;
	u_int kb_rx_stat;
	u_int kb_rx_intr_en;
	u_int kb_rx_reset;
	u_int kb_rx_speed;

	u_int ms_rx_data;
	u_int ms_rx_stat;
	u_int ms_rx_intr_en;
	u_int ms_rx_reset;
	u_int ms_rx_speed;

	u_int kb_buzzf;
	u_int kb_buzz;

	u_int kb_tx_data;
	u_int kb_tx_stat;
	u_int kb_tx_intr_en;
	u_int kb_tx_reset;
	u_int kb_tx_speed;
};

struct kb_ap_softc {
	struct device sc_dev;
	volatile struct kbreg *sc_reg;
	struct device *sc_wskbddev;
};

int kb_ap_match(struct device *, struct cfdata *, void *);
void kb_ap_attach(struct device *, struct device *, void *);
int kb_ap_intr(void *);

void kb_ap_cnattach(void);
void kb_ap_cngetc(void *, u_int *, int *);
void kb_ap_cnpollc(void *, int);

int kb_ap_enable(void *, int);
void kb_ap_setleds(void *, int);
int kb_ap_ioctl(void *, u_long, caddr_t, int, struct proc *);

extern struct wscons_keydesc newskb_keydesctab[];

struct cfattach kb_ap_ca = {
	sizeof(struct kb_ap_softc), kb_ap_match, kb_ap_attach
};

struct wskbd_accessops kb_ap_accessops = {
	kb_ap_enable,
	kb_ap_setleds,
	kb_ap_ioctl,
};

struct wskbd_consops kb_ap_consops = {
	kb_ap_cngetc,
	kb_ap_cnpollc,
};

struct wskbd_mapdata kb_ap_keymapdata = {
	newskb_keydesctab,
	KB_JP,
};

int
kb_ap_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct apbus_attach_args *apa = aux;

	if (strcmp(apa->apa_name, "kb") == 0)
		return 1;

	return 0;
}

void
kb_ap_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct kb_ap_softc *sc = (void *)self;
	struct apbus_attach_args *apa = aux;
	volatile struct kbreg *reg = (void *)apa->apa_hwbase;
	volatile int *dipsw = (void *)NEWS5000_DIP_SWITCH;
	struct wskbddev_attach_args waa;
	int cons = 0;

	printf(" slot%d addr 0x%lx", apa->apa_slotno, apa->apa_hwbase);

	sc->sc_reg = reg;

	if (*dipsw & 7) {
		printf(" (console)");
		cons = 1;
	}
	printf("\n");

	reg->kb_rx_reset = 0x03;
	reg->kb_tx_reset = 0x03;

	reg->kb_rx_speed = 0x04;
	reg->kb_tx_speed = 0x04;

	reg->kb_rx_intr_en = 1;
	reg->kb_tx_intr_en = 0;

	apbus_intr_establish(1, NEWS5000_INT1_KBD, 0, kb_ap_intr, sc,
	    "", apa->apa_ctlnum);

	waa.console = cons;
	waa.keymap = &kb_ap_keymapdata;
	waa.accessops = &kb_ap_accessops;
	waa.accesscookie = sc;

	sc->sc_wskbddev = config_found(self, &waa, wskbddevprint);
}

int
kb_ap_intr(v)
	void *v;
{
	struct kb_ap_softc *sc = v;
	volatile struct kbreg *reg = sc->sc_reg;
	int key, val, type, release;
	int rv = 0;

	while (reg->kb_rx_stat & RX_KBRDY) {
		key = reg->kb_rx_data;
		val = key & 0x7f;
		release = key & 0x80;
		type = release ? WSCONS_EVENT_KEY_UP : WSCONS_EVENT_KEY_DOWN;

		if (sc->sc_wskbddev)
			wskbd_input(sc->sc_wskbddev, type, val);

		rv = 1;
	}

	return rv;
}

void
kb_ap_cnattach()
{
	wskbd_cnattach(&kb_ap_consops, (void *)0xbf900000, &kb_ap_keymapdata);
}

void
kb_ap_cngetc(v, type, data)
	void *v;
	u_int *type;
	int *data;
{
	volatile struct kbreg *reg = v;
	int key, release, ointr;

	/* Disable keyboard interrupt. */
	ointr = reg->kb_rx_intr_en;
	reg->kb_rx_intr_en = 0;

	/* Wait for key data. */
	while ((reg->kb_rx_stat & RX_KBRDY) == 0);

	key = reg->kb_rx_data;
	release = key & 0x80;
	*data = key & 0x7f;
	*type = release ? WSCONS_EVENT_KEY_UP : WSCONS_EVENT_KEY_DOWN;

	reg->kb_rx_intr_en = ointr;
}

void
kb_ap_cnpollc(v, on)
	void *v;
	int on;
{
}

int
kb_ap_enable(v, on)
	void *v;
	int on;
{
	return 0;
}

void
kb_ap_setleds(v, on)
	void *v;
	int on;
{
}

int
kb_ap_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = 0;	/* XXX */
		return 0;
	case WSKBDIO_SETLEDS:
		return 0;
	case WSKBDIO_GETLEDS:
		*(int *)data = 0;
		return 0;
	}

	return EPASSTHROUGH;
}

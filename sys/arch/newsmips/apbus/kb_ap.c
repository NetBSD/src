/*	$NetBSD: kb_ap.c,v 1.9 2008/04/09 15:40:30 tsutsui Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: kb_ap.c,v 1.9 2008/04/09 15:40:30 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>

#include <machine/adrsmap.h>
#include <newsmips/apbus/apbusvar.h>

struct kbreg {
	volatile uint32_t kb_rx_data;
	volatile uint32_t kb_rx_stat;
	volatile uint32_t kb_rx_intr_en;
	volatile uint32_t kb_rx_reset;
	volatile uint32_t kb_rx_speed;

	volatile uint32_t ms_rx_data;
	volatile uint32_t ms_rx_stat;
	volatile uint32_t ms_rx_intr_en;
	volatile uint32_t ms_rx_reset;
	volatile uint32_t ms_rx_speed;

	volatile uint32_t kb_buzzf;
	volatile uint32_t kb_buzz;

	volatile uint32_t kb_tx_data;
	volatile uint32_t kb_tx_stat;
	volatile uint32_t kb_tx_intr_en;
	volatile uint32_t kb_tx_reset;
	volatile uint32_t kb_tx_speed;
};

struct kb_ap_softc {
	device_t sc_dev;
	struct kbreg *sc_reg;
	struct device *sc_wskbddev;
};

int kb_ap_match(device_t, cfdata_t, void *);
void kb_ap_attach(device_t, device_t, void *);
int kb_ap_intr(void *);

void kb_ap_cnattach(void);
void kb_ap_cngetc(void *, u_int *, int *);
void kb_ap_cnpollc(void *, int);

int kb_ap_enable(void *, int);
void kb_ap_setleds(void *, int);
int kb_ap_ioctl(void *, u_long, void *, int, struct lwp *);

extern struct wscons_keydesc newskb_keydesctab[];

CFATTACH_DECL_NEW(kb_ap, sizeof(struct kb_ap_softc),
    kb_ap_match, kb_ap_attach, NULL, NULL);

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
kb_ap_match(device_t parent, cfdata_t cf, void *aux)
{
	struct apbus_attach_args *apa = aux;

	if (strcmp(apa->apa_name, "kb") == 0)
		return 1;

	return 0;
}

void
kb_ap_attach(device_t parent, device_t self, void *aux)
{
	struct kb_ap_softc *sc = device_private(self);
	struct apbus_attach_args *apa = aux;
	struct kbreg *reg = (void *)apa->apa_hwbase;
	volatile uint32_t *dipsw = (void *)NEWS5000_DIP_SWITCH;
	struct wskbddev_attach_args waa;
	int cons = 0;

	sc->sc_dev = self;
	aprint_normal(" slot%d addr 0x%lx", apa->apa_slotno, apa->apa_hwbase);

	sc->sc_reg = reg;

	if (*dipsw & 7) {
		aprint_normal(" (console)");
		cons = 1;
	}
	aprint_normal("\n");

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
kb_ap_intr(void *v)
{
	struct kb_ap_softc *sc = v;
	struct kbreg *reg = sc->sc_reg;
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
kb_ap_cnattach(void)
{

	wskbd_cnattach(&kb_ap_consops, (void *)0xbf900000, &kb_ap_keymapdata);
}

void
kb_ap_cngetc(void *v, u_int *type, int *data)
{
	struct kbreg *reg = v;
	int key, release, ointr;

	/* Disable keyboard interrupt. */
	ointr = reg->kb_rx_intr_en;
	reg->kb_rx_intr_en = 0;

	/* Wait for key data. */
	while ((reg->kb_rx_stat & RX_KBRDY) == 0)
		;

	key = reg->kb_rx_data;
	release = key & 0x80;
	*data = key & 0x7f;
	*type = release ? WSCONS_EVENT_KEY_UP : WSCONS_EVENT_KEY_DOWN;

	reg->kb_rx_intr_en = ointr;
}

void
kb_ap_cnpollc(void *v, int on)
{
}

int
kb_ap_enable(void *v, int on)
{

	return 0;
}

void
kb_ap_setleds(void *v, int on)
{
}

int
kb_ap_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
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

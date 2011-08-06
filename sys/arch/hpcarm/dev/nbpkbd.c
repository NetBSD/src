/*	$NetBSD: nbpkbd.c,v 1.1 2011/08/06 03:53:40 kiyohara Exp $ */
/*
 * Copyright (c) 2011 KIYOHARA Takashi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nbpkbd.c,v 1.1 2011/08/06 03:53:40 kiyohara Exp $");

#include <sys/param.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <arm/xscale/pxa2x0_i2c.h>

#include <hpcarm/dev/nbppconvar.h>

#include <dev/hpc/hpckbdvar.h>

#include <dev/i2c/i2cvar.h>

#include "locators.h"

#ifdef NBPKBD_DEBUG
#define DPRINTF(x)	printf x
#else
#define DPRINTF(x)
#endif

#define IS_KEY_DOWN(x)	(((x) & 0x80) == 0x00)
#define KEY_CODE(x)	((x) & 0x7f)
#define KEY_INITIAL_INTERVAL	(500 * hz / 1000)	/* 500ms */
#define KEY_REPEAT_INTERVAL	(200 * hz / 1000)	/* 200ms */

struct nbpkbd_softc {
	device_t sc_dev;
	device_t sc_parent;
	int sc_tag;

	struct hpckbd_ic_if sc_if;
	struct hpckbd_if *sc_hpckbd;

	int sc_enabled;

	callout_t  sc_callout;
	uint8_t sc_last_scancode;
};

static int nbpkbd_match(device_t, cfdata_t, void *);
static void nbpkbd_attach(device_t, device_t, void *);

static int nbpkbd_input_establish(void *, struct hpckbd_if *);
static int nbpkbd_input(void *, int, char *);
static int nbpkbd_poll(void *);

static void nbpkbd_key_repeat(void *);

CFATTACH_DECL_NEW(nbpkbd, sizeof(struct nbpkbd_softc),
    nbpkbd_match, nbpkbd_attach, NULL, NULL);


/* ARGSUSED */
static int
nbpkbd_match(device_t parent, cfdata_t match, void *aux)
{
	struct nbppcon_attach_args *pcon = aux;

	if (strcmp(pcon->aa_name, match->cf_name) ||
	    pcon->aa_tag == NBPPCONCF_TAG_DEFAULT)
		return 0;

	return 1;
}

static void
nbpkbd_attach(device_t parent, device_t self, void *aux)
{
	struct nbpkbd_softc *sc = device_private(self);
	struct nbppcon_attach_args *pcon = aux;
	struct hpckbd_attach_args haa;

	aprint_normal(": NETBOOK PRO Keyboard\n");
	aprint_naive("\n");

	sc->sc_dev = self;
	sc->sc_parent = parent;
	sc->sc_tag = pcon->aa_tag;
	sc->sc_if.hii_ctx = sc;
	sc->sc_if.hii_establish = nbpkbd_input_establish;
	sc->sc_if.hii_poll = nbpkbd_poll;

	hpckbd_cnattach(&sc->sc_if);

	callout_init(&sc->sc_callout, 0);
	callout_setfunc(&sc->sc_callout, nbpkbd_key_repeat, sc);

	/* Attach hpckbd */
	haa.haa_ic = &sc->sc_if;
	config_found(self, &haa, hpckbd_print);

	if (nbppcon_regist_callback(parent, sc->sc_tag, nbpkbd_input, sc) ==
	    NULL)
		aprint_error_dev(self, "callback regist failed\n");
}


static int
nbpkbd_input_establish(void *arg, struct hpckbd_if *kbdif)
{
	struct nbpkbd_softc *sc = arg;

	sc->sc_hpckbd = kbdif;
	sc->sc_enabled = 1;

	return 0;
}

static int
nbpkbd_input(void *arg, int buflen, char *buf)
{
	struct nbpkbd_softc *sc = arg;

#ifdef NBPKBD_DEBUG
	{
		int i;

		printf("%s:", __func__);
		for (i = 0; i < buflen; i++)
			printf(" %02x", buf[i]);
		printf("\n");
	}
#endif

	if (buflen != 2) {
		aprint_error_dev(sc->sc_dev, "ugly length %d\n", buflen);
		return -1;
	}

	hpckbd_input(sc->sc_hpckbd, IS_KEY_DOWN(buf[1]), KEY_CODE(buf[1]));

	sc->sc_last_scancode = buf[1];
	if (IS_KEY_DOWN(buf[1]))
		callout_schedule(&sc->sc_callout, KEY_INITIAL_INTERVAL);
	else
		callout_stop(&sc->sc_callout);

	delay(50);	/* XXXX */

	if (nbppcon_kbd_ready(sc->sc_parent) != 0)
		aprint_error_dev(sc->sc_dev,
		    "%s: kbd-ready failed\n", __func__);

	return 0;
}

static int
nbpkbd_poll(void *arg)
{
	struct nbpkbd_softc *sc = arg;
	int rv, s;
	char buf[2];

	if (!sc->sc_enabled)
		return 0;

	s = spltty();

	rv = nbppcon_poll(sc->sc_parent, sc->sc_tag, sizeof(buf), buf);

	DPRINTF(("%s: received %02x %02x\n",
	    device_xname(sc->sc_dev), buf[0], buf[1]));

	hpckbd_input(sc->sc_hpckbd, IS_KEY_DOWN(buf[1]), KEY_CODE(buf[1]));

	delay(50);	/* XXXX */

	rv = nbppcon_kbd_ready(sc->sc_parent);

	splx(s);

	if (rv != 0)
		aprint_error_dev(sc->sc_dev,
		    "%s: kbd-ready failed\n", __func__);

	return 0;
}

static void
nbpkbd_key_repeat(void *arg)
{
	struct nbpkbd_softc *sc = arg;

	hpckbd_input(sc->sc_hpckbd, IS_KEY_DOWN(sc->sc_last_scancode),
	    KEY_CODE(sc->sc_last_scancode));

	if (IS_KEY_DOWN(sc->sc_last_scancode))
		callout_schedule(&sc->sc_callout, KEY_REPEAT_INTERVAL);
}

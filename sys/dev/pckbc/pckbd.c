/* $NetBSD: pckbd.c,v 1.6 1998/04/28 17:48:35 thorpej Exp $ */

/*-
 * Copyright (c) 1993, 1994, 1995 Charles Hannum.  All rights reserved.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)pccons.c	5.11 (Berkeley) 5/21/91
 */

/*
 * code to work keyboard for PC-style console
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/isa/pckbcvar.h>

#include <dev/pckbc/pckbdreg.h>
#include <dev/pckbc/pckbdvar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/wscons/wskbdmap_mfii.h>

#ifdef __i386__
#include <sys/kernel.h> /* XXX for hz */
#endif

#include "locators.h"

struct pckbd_internal {
	int t_isconsole;
	pckbc_tag_t t_kbctag;
	int t_kbcslot;

	int t_led_state;
	int t_lastchar;
	int t_extended;
	int t_extended1;

	struct pckbd_softc *t_sc; /* back pointer */
};

struct pckbd_softc {
        struct  device sc_dev;

	struct pckbd_internal *id;

	struct device *sc_wskbddev;
};

int pckbdprobe __P((struct device *, struct cfdata *, void *));
void pckbdattach __P((struct device *, struct device *, void *));

struct cfattach pckbd_ca = {
	sizeof(struct pckbd_softc), pckbdprobe, pckbdattach,
};

void	pckbd_set_leds __P((void *, int));
int	pckbd_ioctl __P((void *, u_long, caddr_t, int, struct proc *));

void	pckbd_cngetc __P((void *, u_int *, int *));
void	pckbd_cnpollc __P((void *, int));

int	pckbd_set_xtscancode __P((pckbc_tag_t, pckbc_slot_t));
int	pckbd_init __P((struct pckbd_internal *, pckbc_tag_t, pckbc_slot_t,
			int));
void	pckbd_input __P((void *, int));

static int	pckbd_decode __P((struct pckbd_internal *, int,
				  u_int *, int *));
static int	pckbd_led_encode __P((int));
static int	pckbd_led_decode __P((int));

struct pckbd_internal pckbd_consdata;

int
pckbd_set_xtscancode(kbctag, kbcslot)
	pckbc_tag_t kbctag;
	pckbc_slot_t kbcslot;
{
	u_char cmd[2];
	int res;

	/*
	 * Some keyboard/8042 combinations do not seem to work if the keyboard
	 * is set to table 1; in fact, it would appear that some keyboards just
	 * ignore the command altogether.  So by default, we use the AT scan
	 * codes and have the 8042 translate them.  Unfortunately, this is
	 * known to not work on some PS/2 machines.  We try desparately to deal
	 * with this by checking the (lack of a) translate bit in the 8042 and
	 * attempting to set the keyboard to XT mode.  If this all fails, well,
	 * tough luck.
	 *
	 * XXX It would perhaps be a better choice to just use AT scan codes
	 * and not bother with this.
	 */
	if (pckbc_xt_translation(kbctag, kbcslot, 1)) {
		/* The 8042 is translating for us; use AT codes. */
		cmd[0] = KBC_SETTABLE;
		cmd[1] = 2;
		res = pckbc_poll_cmd(kbctag, kbcslot, cmd, 2, 0, 0, 0);
		if (res)
			printf("pckbd: error setting scanset 2\n");
	} else {
		/* Stupid 8042; set keyboard to XT codes. */
		cmd[0] = KBC_SETTABLE;
		cmd[1] = 1;
		res = pckbc_poll_cmd(kbctag, kbcslot, cmd, 2, 0, 0, 0);
		if (res)
			printf("pckbd: error setting scanset 1\n");
	}
	return (res);
}

/*
 * these are both bad jokes
 */
int
pckbdprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pckbc_attach_args *pa = aux;
	u_char cmd[1], resp[1];
	int res;

	/*
	 * XXX There are rumours that a keyboard can be connected
	 * to the aux port as well. For me, this didn't work.
	 * For further experiments, allow it if explicitely
	 * wired in the config file.
	 */
	if ((pa->pa_slot != PCKBC_KBD_SLOT) &&
	    (cf->cf_loc[PCKBCCF_SLOT] == PCKBCCF_SLOT_DEFAULT))
		return (0);

#if 1
	/* Flush any garbage. */
	pckbc_flush(pa->pa_tag, pa->pa_slot);

	/* Reset the keyboard. */
	cmd[0] = KBC_RESET;
	res = pckbc_poll_cmd(pa->pa_tag, pa->pa_slot, cmd, 1, 1, resp, 1);
	if (res) {
		printf("pckbdprobe: reset error %d\n", 1);
		goto lose;
	}
	if (resp[0] != KBR_RSTDONE) {
		printf("pckbdprobe: reset error %d\n", 2);
		goto lose;
	}

	/*
	 * Some keyboards seem to leave a second ack byte after the reset.
	 * This is kind of stupid, but we account for them anyway by just
	 * flushing the buffer.
	 */
	pckbc_flush(pa->pa_tag, pa->pa_slot);

	/* Just to be sure. */
	cmd[0] = KBC_ENABLE;
	res = pckbc_poll_cmd(pa->pa_tag, pa->pa_slot, cmd, 1, 0, 0, 0);
	if (res) {
		printf("pckbdprobe: reset error %d\n", 3);
		goto lose;
	}

	if (pckbd_set_xtscancode(pa->pa_tag, pa->pa_slot))
		goto lose;

	return (2);

lose:
	/*
	 * Technically, we should probably fail the probe.  But we'll be nice
	 * and allow keyboard-less machines to boot with the console.
	 */
#endif
	return (1);
}

void
pckbdattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pckbd_softc *sc = (void *)self;
	struct pckbc_attach_args *pa = aux;
	int isconsole;
	struct wskbddev_attach_args a;

	printf("\n");

	isconsole = pckbd_consdata.t_isconsole &&
	    (pckbd_consdata.t_kbctag == pa->pa_tag) &&
	    (pckbd_consdata.t_kbcslot == pa->pa_slot);

	if (isconsole)
		sc->id = &pckbd_consdata;
	else {
		sc->id = malloc(sizeof(struct pckbd_internal),
				M_DEVBUF, M_WAITOK);
		(void) pckbd_init(sc->id, pa->pa_tag, pa->pa_slot, 0);
	}

	sc->id->t_sc = sc;

	pckbc_set_inputhandler(sc->id->t_kbctag, sc->id->t_kbcslot,
			       pckbd_input, sc);

	a.console = isconsole;
#ifdef PCKBD_LAYOUT
	a.layout = PCKBD_LAYOUT;
#else
	a.layout = KB_US;
#endif
	a.keydesc = pckbd_keydesctab;
	a.num_keydescs = sizeof(pckbd_keydesctab)/sizeof(pckbd_keydesctab[0]);

	if (isconsole) {
		a.getc = pckbd_cngetc;
		a.pollc = pckbd_cnpollc;
	} else {
		a.getc = NULL;
		a.pollc = NULL;
	}

	a.set_leds = pckbd_set_leds;
	a.ioctl = pckbd_ioctl;
	a.accesscookie = sc->id;

	/*
	 * Attach the wskbd, saving a handle to it.
	 * XXX XXX XXX
	 */
	sc->sc_wskbddev = config_found(self, &a, wskbddevprint);
}

static int pckbd_decode(id, datain, type, dataout)
	struct pckbd_internal *id;
	int datain;
	u_int *type;
	int *dataout;
{
	if (datain == KBR_EXTENDED0) {
		id->t_extended = 1;
		return(0);
	} else if (datain == KBR_EXTENDED1) {
		id->t_extended1 = 2;
		return(0);
	}

	/* process BREAK key (EXT1 1D 45  EXT1 9D C5) map to (unused) code 7F */
	if (id->t_extended1 == 2 && (datain == 0x1d || datain == 0x9d)) {
		id->t_extended1 = 1;
		return(0);
	} else if (id->t_extended1 == 1 && (datain == 0x45 || datain == 0xc5)) {
		id->t_extended1 = 0;
		datain = (datain & 0x80) | 0x7f;
	} else if (id->t_extended1 > 0) {
		id->t_extended1 = 0;
	}
 
	/* Always ignore typematic keys */
	if (datain == id->t_lastchar)
		return(0);
	id->t_lastchar = datain;

	if (datain & 0x80)
		*type = WSCONS_EVENT_KEY_UP;
	else
		*type = WSCONS_EVENT_KEY_DOWN;

	/* map extended keys to (unused) codes 128-254 */
	*dataout = (datain & 0x7f) | (id->t_extended ? 0x80 : 0);

	id->t_extended = 0;
	return(1);
}

int
pckbd_init(t, kbctag, kbcslot, console)
	struct pckbd_internal *t;
	pckbc_tag_t kbctag;
	pckbc_slot_t kbcslot;
	int console;
{
	bzero(t, sizeof(struct pckbd_internal));

	t->t_isconsole = console;
	t->t_kbctag = kbctag;
	t->t_kbcslot = kbcslot;

	return (pckbd_set_xtscancode(kbctag, kbcslot));
}

static int
pckbd_led_encode(led)
	int led;
{
	int res;

	res = 0;

	if (led & WSKBD_LED_SCROLL)
		res |= 0x01;
	if (led & WSKBD_LED_NUM)
		res |= 0x02;
	if (led & WSKBD_LED_CAPS)
		res |= 0x04;
	return(res);
}

static int
pckbd_led_decode(led)
	int led;
{
	int res;

	res = 0;
	if (led & 0x01)
		res |= WSKBD_LED_SCROLL;
	if (led & 0x02)
		res |= WSKBD_LED_NUM;
	if (led & 0x04)
		res |= WSKBD_LED_CAPS;
	return(res);
}

void
pckbd_set_leds(v, leds)
	void *v;
	int leds;
{
	struct pckbd_internal *t = v;
	u_char cmd[2];

	cmd[0] = KBC_MODEIND;
	cmd[1] = pckbd_led_encode(leds);
	t->t_led_state = cmd[1];

	(void) pckbc_enqueue_cmd(t->t_kbctag, t->t_kbcslot, cmd, 2, 0, 0, 0);
}

/*
 * Got a console receive interrupt -
 * the console processor wants to give us a character.
 */
void
pckbd_input(vsc, data)
	void *vsc;
	int data;
{
	struct pckbd_softc *sc = vsc;
	int type, key;

	if (pckbd_decode(sc->id, data, &type, &key))
		wskbd_input(sc->sc_wskbddev, type, key);
}

int
pckbd_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct pckbd_internal *t = v;

	switch (cmd) {
	    case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_PC_XT;
		return 0;
	    case WSKBDIO_SETLEDS: {
		char cmd[2];
		int res;
		cmd[0] = KBC_MODEIND;
		cmd[1] = pckbd_led_encode(*(int *)data);
		t->t_led_state = cmd[1];
		res = pckbc_enqueue_cmd(t->t_kbctag, t->t_kbcslot, cmd, 2, 0,
					1, 0);
		return (res);
		}
	    case WSKBDIO_GETLEDS:
		*(int *)data = pckbd_led_decode(t->t_led_state);
		return (0);
	    case WSKBDIO_COMPLEXBELL:
#define d ((struct wskbd_bell_data *)data)
		/* keyboard can't beep - use md code */
#ifdef __i386__
		sysbeep(d->pitch, d->period * hz / 1000);
		/* comes in as ms, goes out as ticks; volume ignored */
#endif
#undef d
		return (0);
	}
	return -1;
}

int
pckbd_cnattach(kbctag, kbcslot)
	pckbc_tag_t kbctag;
	int kbcslot;
{
	int res;
	struct wskbddev_attach_args a;

	res = pckbd_init(&pckbd_consdata, kbctag, kbcslot, 1);
	if (res)
		return (res);

	a.console = 1;
#ifdef PCKBD_LAYOUT
	a.layout = PCKBD_LAYOUT;
#else
	a.layout = KB_US;
#endif
	a.keydesc = pckbd_keydesctab;
	a.num_keydescs = sizeof(pckbd_keydesctab)/sizeof(pckbd_keydesctab[0]);
	a.getc = pckbd_cngetc;
	a.pollc = pckbd_cnpollc;
	a.set_leds = pckbd_set_leds;
	a.ioctl = NULL;
	a.accesscookie = &pckbd_consdata;

	wskbd_cnattach(&a);

	return (0);
}

/* ARGSUSED */
void
pckbd_cngetc(v, type, data)
	void *v;
	u_int *type;
	int *data;
{
        struct pckbd_internal *t = v;
	int val;

	for (;;) {
		val = pckbc_poll_data(t->t_kbctag, t->t_kbcslot);
		if ((val != -1) && pckbd_decode(t, val, type, data))
			return;
	}
}

void
pckbd_cnpollc(v, on)
	void *v;
        int on;
{
	struct pckbd_internal *t = v;

	pckbc_set_poll(t->t_kbctag, t->t_kbcslot, on);
}

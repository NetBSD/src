/* $NetBSD: pckbd.c,v 1.1 1998/03/22 15:41:27 drochner Exp $ */

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

#include <dev/pckbc/pckbd_scancodes.h>

#include "locators.h"

struct pckbd_internal {
	int t_isconsole;
	pckbc_tag_t t_kbctag;
	int t_kbcslot;

	pckbd_scan_def *t_scancodes;

	int extended;
	u_char shift_state, lock_state;

	struct pckbd_softc *t_sc; /* back pointer */
};

struct pckbd_softc {
        struct  device sc_dev;

	struct pckbd_internal *id;
	int sc_lastchar;

	struct device *sc_wskbddev;
};

#ifdef __BROKEN_INDIRECT_CONFIG
int pckbdprobe __P((struct device *, void *, void *));
#else
int pckbdprobe __P((struct device *, struct cfdata *, void *));
#endif
void pckbdattach __P((struct device *, struct device *, void *));

struct cfattach pckbd_ca = {
	sizeof(struct pckbd_softc), pckbdprobe, pckbdattach,
};

int	pckbd_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
const char *pckbd_translate __P((void *, u_int, int));

const struct wskbd_accessops pckbd_accessops = {
	pckbd_ioctl,
	pckbd_translate,
};

int	pckbd_cngetc __P((void *));
void	pckbd_cnpollc __P((void *, int));

const struct wskbd_consops pckbd_consops = {
	pckbd_cngetc,
	pckbd_cnpollc,
};

int	pckbd_set_xtscancode __P((pckbc_tag_t, pckbc_slot_t));
void	pckbd_init __P((struct pckbd_internal *, pckbc_tag_t, pckbc_slot_t,
			int));
void	pckbd_input __P((void *, int));
void	pckbd_update_leds __P((struct pckbd_internal *));

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
pckbdprobe(parent, match, aux)
	struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
	struct cfdata *cf = match;
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
	    (cf->cf_loc[PCKBCCF_SLOT] != PCKBC_KBD_SLOT))
		return (0);

#if 1
	/* Flush any garbage. */
	pckbc_flush(pa->pa_tag, pa->pa_slot);

	/* Reset the keyboard. */
	cmd[0] = KBC_RESET;
	res = pckbc_poll_cmd(pa->pa_tag, pa->pa_slot, cmd, 1, 1, resp, 1);
	if (res) {
		printf("pcprobe: reset error %d\n", 1);
		goto lose;
	}
	if (resp[0] != KBR_RSTDONE) {
		printf("pcprobe: reset error %d\n", 2);
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
		printf("pcprobe: reset error %d\n", 3);
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

	if (isconsole) {
		sc->id = &pckbd_consdata;
	} else {
		sc->id = malloc(sizeof(struct pckbd_internal),
				M_DEVBUF, M_WAITOK);
		pckbd_init(sc->id, pa->pa_tag, pa->pa_slot, 0);
	}

	sc->id->t_sc = sc;

	(void) pckbd_set_xtscancode(sc->id->t_kbctag, sc->id->t_kbcslot);
	pckbc_set_inputhandler(sc->id->t_kbctag, sc->id->t_kbcslot,
			       pckbd_input, sc);

	a.console = isconsole;
	a.accessops = &pckbd_accessops;
	a.accesscookie = sc->id;

	/*
	 * Attach the wskbd, saving a handle to it.
	 * XXX XXX XXX
	 */
	sc->sc_wskbddev = config_found(self, &a, wskbddevprint);
}

void
pckbd_init(t, kbctag, kbcslot, console)
	struct pckbd_internal *t;
	pckbc_tag_t kbctag;
	pckbc_slot_t kbcslot;
	int console;
{
	t->t_isconsole = console;
	t->t_kbctag = kbctag;
	t->t_kbcslot = kbcslot;
	t->t_scancodes = pckbd_scan_codes_us;
	t->extended = 0;
	t->shift_state = t->lock_state = 0;
}

void
pckbd_update_leds(t)
	struct pckbd_internal *t;
{
	u_char cmd[2];

	cmd[0] = KBC_MODEIND;
	cmd[1] = t->lock_state;

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
	int type;

	/* Always ignore typematic keys */
	if (data == sc->sc_lastchar)
		return;
	sc->sc_lastchar = data;

	switch (data) {
	    case KBR_EXTENDED:
		type = WSCONS_EVENT_KEY_OTHER;
		break;
	    default:
		type = (data & 0x80) ? WSCONS_EVENT_KEY_UP :
		    WSCONS_EVENT_KEY_DOWN;
		data &= ~0x80;
		break;
	}
	wskbd_input(sc->sc_wskbddev, type, data);
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

/* printf("pckdb_ioctl 0x%lx, 0x%lx\n", cmd, WSKBDIO_GTYPE); */
	switch (cmd) {
	    case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_PC_XT;
		return 0;
	    case WSKBDIO_SETLEDS: {
		char cmd[2];
		int res;
		cmd[0] = KBC_MODEIND;
		cmd[1] = *(int*)data;
		res = pckbc_enqueue_cmd(t->t_kbctag, t->t_kbcslot, cmd, 2, 0,
					1, 0);
		return (res);
		}
	}
	return -1;
}

/*
 * Get characters from the keyboard.  If none are present, return NULL.
 */
const char *
pckbd_translate(v, type, value)
	void *v;
	u_int type;
	int value;
{
	struct pckbd_internal *t = v;
	u_char dt = value;
	static u_char capchar[2];

	if (type == WSCONS_EVENT_KEY_OTHER && dt == KBR_EXTENDED) {
		t->extended = 1;
		return NULL;
	}

#ifdef DDB
	/*
	 * Check for cntl-alt-esc.
	 */
	if (t->t_isconsole && (type == WSCONS_EVENT_KEY_DOWN) &&
	    (dt == 1) && ((t->shift_state & (CTL | ALT)) == (CTL | ALT))
	    /* XXX && we're not already in the debugger */) {
		Debugger();
#if 0
		dt |= 0x80;	/* discard esc (ddb discarded ctl-alt) */
#else
		t->extended = 0;
		return (NULL);
#endif
	}
#endif

	/* XXX XXX temporary hack to get virtual consoles working */
	if (t->t_sc && ((t->shift_state & (CTL | ALT)) == (CTL | ALT)) &&
	    (dt >= 59 && dt <= 66)) {
		if (type == WSCONS_EVENT_KEY_DOWN)
			wskbd_ctlinput(t->t_sc->sc_wskbddev, dt - 59);
		t->extended = 0;
		return (NULL);
	}

	/*
	 * Check for make/break.
	 */
	if (type == WSCONS_EVENT_KEY_UP) {
		/*
		 * break
		 */
		switch (t->t_scancodes[dt].type) {
		case NUM:
			t->shift_state &= ~NUM;
			break;
		case CAPS:
			t->shift_state &= ~CAPS;
			break;
		case SCROLL:
			t->shift_state &= ~SCROLL;
			break;
		case SHIFT:
			t->shift_state &= ~SHIFT;
			break;
		case ALT:
			if (t->extended) 
				t->shift_state &= ~ALTGR;
			else
				t->shift_state &= ~ALT;
			break;
		case CTL:
			t->shift_state &= ~CTL;
			break;
		}
	} else if (type == WSCONS_EVENT_KEY_DOWN) {
		/*
		 * make
		 */
		/* fix numeric / on non US keyboard */
		if (t->extended && dt == 53) {
			capchar[0] = '/';
			t->extended = 0;
			return capchar;
		}

		switch (t->t_scancodes[dt].type) {
		/*
		 * locking keys
		 */
		case NUM:
			if (t->shift_state & NUM)
				break;
			t->shift_state |= NUM;
			t->lock_state ^= NUM;
			pckbd_update_leds(t);
			break;
		case CAPS:
			if (t->shift_state & CAPS)
				break;
			t->shift_state |= CAPS;
			t->lock_state ^= CAPS;
			pckbd_update_leds(t);
			break;
		case SCROLL:
			if (t->shift_state & SCROLL)
				break;
			t->shift_state |= SCROLL;
			t->lock_state ^= SCROLL;
			if (t->t_sc)
				wskbd_holdscreen(t->t_sc->sc_wskbddev,
						 t->lock_state & SCROLL);
			pckbd_update_leds(t);
			break;
		/*
		 * non-locking keys
		 */
		case SHIFT:
			t->shift_state |= SHIFT;
			break;
		case ALT:
			if (t->extended)  
				t->shift_state |= ALTGR;
			else
				t->shift_state |= ALT;
			break;
		case CTL:
			t->shift_state |= CTL;
			break;
		case ASCII:
			/* control has highest priority */
			if (t->shift_state & CTL)
				capchar[0] = t->t_scancodes[dt].ctl[0];
			else if (t->shift_state & ALTGR)
				capchar[0] = t->t_scancodes[dt].altgr[0];
			else if (t->shift_state & SHIFT)
				capchar[0] = t->t_scancodes[dt].shift[0];
			else
				capchar[0] = t->t_scancodes[dt].unshift[0];
			if ((t->lock_state & CAPS) && capchar[0] >= 'a' &&
			    capchar[0] <= 'z') {
				capchar[0] -= ('a' - 'A');
			}
			capchar[0] |= (t->shift_state & ALT);
			t->extended = 0;
			return capchar;
		case NONE:
			break;
		case FUNC: {
			char *more_chars;
			if (t->shift_state & SHIFT)
				more_chars = t->t_scancodes[dt].shift;
			else if (t->shift_state & CTL)
				more_chars = t->t_scancodes[dt].ctl;
			else
				more_chars = t->t_scancodes[dt].unshift;
			t->extended = 0;
			return more_chars;
		}
		case KP: {
			char *more_chars;
			if (t->shift_state & (SHIFT | CTL) ||
			    (t->lock_state & NUM) == 0 || t->extended)
				more_chars = t->t_scancodes[dt].shift;
			else
				more_chars = t->t_scancodes[dt].unshift;
			t->extended = 0;
			return more_chars;
		}
		}
	}

	t->extended = 0;
	return (NULL);
}

int
pckbd_cnattach(kbctag, kbcslot)
	pckbc_tag_t kbctag;
	int kbcslot;
{
	pckbd_init(&pckbd_consdata, kbctag, kbcslot, 1);

	wskbd_cnattach(&pckbd_consops, NULL);

	return (0);
}

/* ARGSUSED */
int
pckbd_cngetc(v)
	void *v;
{
        register const char *cp = NULL;
	u_int type;
	int data;
#if 0
	static u_char last;
#endif

        do {
		/* wait for byte */
		do {
			data = pckbc_poll_data(pckbd_consdata.t_kbctag,
					       pckbd_consdata.t_kbcslot);
		} while (data == -1);

#if 0
		/* Ignore typematic keys */
		if (data == last)
			continue;
		last = data;
#endif

		switch (data) { 
		case KBR_EXTENDED:
			type = WSCONS_EVENT_KEY_OTHER;
			break;
		default:
			type = (data & 0x80) ? WSCONS_EVENT_KEY_UP :
			    WSCONS_EVENT_KEY_DOWN;
			data &= ~0x80;
			break;
		}

		cp = pckbd_translate(&pckbd_consdata, type, data);
        } while (!cp);
        if (*cp == '\r')
                return ('\n');
        return (*cp);
}

void
pckbd_cnpollc(v, on)
	void *v;
        int on;
{
	pckbc_set_poll(pckbd_consdata.t_kbctag, pckbd_consdata.t_kbcslot, on);
}

/*	$NetBSD: skbd.c,v 1.2 2000/01/12 14:56:22 uch Exp $ */

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include "opt_tx39xx.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/tty.h> 

#include <machine/bus.h>
#include <machine/intr.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/pckbc/wskbdmap_mfii.h>

#include <hpcmips/dev/skbdvar.h>
#include <hpcmips/dev/skbdkeymap.h>

#ifdef TX39XX
#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/txsnd.h>
#endif

struct skbd_softc;

struct skbd_chip {
	skbd_tag_t	sk_ic;
	const u_int8_t	*sk_keymap;
	int		sk_polling;
	int		sk_console;
	u_int		sk_type;
	int		sk_data;

	int		sk_enabled;
	struct device		*sk_wskbddev;
	struct skbd_softc*	sk_sc;	/* back link */
};

struct skbd_softc {
	struct	device		sc_dev;
	struct skbd_chip	*sc_sk;
};

int	skbd_match __P((struct device*, struct cfdata*, void*));
void	skbd_attach __P((struct device*, struct device*, void*));

int	__skbd_input __P((void*, int, int));
void	__skbd_input_hook __P((void*));

int	skbd_keymap_lookup __P((struct skbd_chip*));

struct cfattach skbd_ca = {
	sizeof(struct skbd_softc), skbd_match, skbd_attach
};

/* wskbd accessopts */
int	skbd_enable __P((void *, int));
void	skbd_set_leds __P((void *, int));
int	skbd_ioctl __P((void *, u_long, caddr_t, int, struct proc *));

/* consopts */
struct	skbd_chip skbd_consdata;
void	skbd_cngetc __P((void*, u_int*, int*));
void	skbd_cnpollc __P((void *, int));

const struct wskbd_accessops skbd_accessops = {
	skbd_enable,
	skbd_set_leds,
	skbd_ioctl,
};

const struct wskbd_consops skbd_consops = {
	skbd_cngetc,
	skbd_cnpollc,
};

struct wskbd_mapdata skbd_keymapdata = {
	pckbd_keydesctab,
	KB_US
};

int
skbd_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

void
skbd_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct skbd_attach_args *saa = aux;
	struct skbd_softc *sc = (void*)self;
	struct skbd_chip *sk;
	struct wskbddev_attach_args wa;

	sc->sc_sk = sk = &skbd_consdata;
	sk->sk_polling = 0;
	/* back link */
	sk->sk_sc = sc;
	/* buffer/controller chip */
	sk->sk_ic = saa->saa_ic;

	/*
	 * platform dependent keymapping
	 */
	if (skbd_keymap_lookup(sk)) {
		printf(": no keymap.");
	}
	
	printf("\n");

	/*
	 * register skbd function to parent controller.
	 */
	skbdif_establish(sk->sk_ic, __skbd_input, __skbd_input_hook, sk);

	wa.console = sk->sk_console;
	wa.keymap = &skbd_keymapdata;
	wa.accessops = &skbd_accessops;
	wa.accesscookie = sk;

	sk->sk_wskbddev = config_found(self, &wa, wskbddevprint);
}

int
skbd_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	return pnp ? QUIET : UNCONF;
}

int
skbd_keymap_lookup(sk)
	struct skbd_chip *sk;
{
	const struct skbd_keymap_table *tab;
	platid_mask_t mask;

	for (tab = skbd_keymap_table; tab->st_keymap; tab++) {

		mask = PLATID_DEREF(&tab->st_platform);

		if (platid_match(&platid, &mask)) {
			sk->sk_keymap = tab->st_keymap;
			skbd_keymapdata.layout = tab->st_layout;

			return 0;
		}
	}

	/* no keymap. use default. */

	sk->sk_keymap = default_keymap;
	skbd_keymapdata.layout = KB_US;

	return 1;
}

void
__skbd_input_hook(arg)
 	void *arg;
{
	struct skbd_chip *sk = arg;

	if (sk->sk_polling) {
		sk->sk_type = WSCONS_EVENT_ALL_KEYS_UP;
	}
}


int
__skbd_input(arg, flag, scancode)
 	void *arg;
	int flag, scancode;
{
	struct skbd_chip *sk = arg;
	int type, key;

	if (flag) {
#ifdef TX39XX
		tx_sound_click(tx_conf_get_tag());
#endif
		type = WSCONS_EVENT_KEY_DOWN; 
	} else {
		type = WSCONS_EVENT_KEY_UP;
	}
	
	if ((key = sk->sk_keymap[scancode]) == UNK) {
		printf("skbd: unknown scan code %#x\n", scancode);

		return 0;
	}

	if (key == IGN) {
		return 0;
	}


	if (sk->sk_polling) {
		sk->sk_type = type;
		sk->sk_data = sk->sk_keymap[scancode];
	} else {
		wskbd_input(sk->sk_wskbddev, type,
			    sk->sk_keymap[scancode]);
	}

	return 0;
}

/*
 * console support routines
 */
int
skbd_cnattach(ic)
	struct skbd_controller *ic;
{
	struct skbd_chip *sk = &skbd_consdata;

	sk->sk_console = 1;
	
	skbd_keymap_lookup(sk);

	/* attach controller */
	sk->sk_ic = ic;

	skbdif_establish(sk->sk_ic, __skbd_input, __skbd_input_hook, sk);

	wskbd_cnattach(&skbd_consops, sk, &skbd_keymapdata);
	
	return 0;
}

void
skbd_cngetc(arg, type, data)
	void *arg;
	u_int *type;
	int *data;
{
	struct skbd_chip *sk = arg;
	int s;

	if (!sk->sk_console || !sk->sk_polling || !sk->sk_ic) {
		return;
	}

	s = splimp();

	if (sk->sk_type == WSCONS_EVENT_ALL_KEYS_UP) {
		/* inquire of controller */
		skbdif_poll(sk->sk_ic);
	}

	*type = sk->sk_type;
	*data = sk->sk_data;

	sk->sk_type = WSCONS_EVENT_ALL_KEYS_UP;

	splx(s);
}

void
skbd_cnpollc(arg, on)
	void *arg;
        int on;
{
	struct skbd_chip *sk = arg;
	int s = splimp();

	sk->sk_polling = on;

	splx(s);
}

int
skbd_enable(arg, on)
	void *arg;
	int on;
{
	struct skbd_chip *sk = arg;

	if (on) {
		if (sk->sk_enabled) {
			return EBUSY;
		}

		sk->sk_enabled = 1;
	} else {
		if (sk->sk_console) {
			return EBUSY;
		}

		sk->sk_enabled = 0;
	}


	return 0;
}

void
skbd_set_leds(arg, leds)
	void *arg;
	int leds;
{
	/* No LEDs */
}

int
skbd_ioctl(arg, cmd, data, flag, p)
	void *arg;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	/* No ioctls */

	return 0;
}

/* $NetBSD: g42xxeb_kmkbd.c,v 1.9.12.1 2009/05/13 17:16:38 jym Exp $ */

/*-
 * Copyright (c) 2002, 2003, 2005 Genetec corp.
 * All rights reserved.
 *
 * 4x5 matrix key switch driver for TWINTAIL.
 * Written by Hiroyuki Bessho for Genetec corp.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Use on-board matrix switches as wskbd.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: g42xxeb_kmkbd.c,v 1.9.12.1 2009/05/13 17:16:38 jym Exp $" );

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/callout.h>
#include <sys/kernel.h>			/* for hz */

#include <machine/bus.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <arch/evbarm/g42xxeb/g42xxeb_var.h>

#include "locators.h"

/*#include "opt_pckbd_layout.h"*/
/*#include "opt_wsdisplay_compat.h"*/

#define DEBOUNCE_TICKS	((hz<=50)?1:hz/50)	/* 20ms */
#define RELEASE_WATCH_TICKS  (hz/10)	/* 100ms */

struct kmkbd_softc {
        struct  device dev;

	struct device *wskbddev;
	void *ih;			/* interrupt handler */
	struct callout callout;

	uint32_t  notified_bits;	/* reported state of keys */
	uint32_t  last_bits;		/* used for debounce */
	u_char  debounce_counter;
#define DEBOUNCE_COUNT  3
	u_char  polling;
	enum kmkbd_state {
		ST_INIT,
		ST_DISABLED,
		ST_ALL_UP,		/* waiting for interrupt */
		ST_DEBOUNCE,		/* doing debounce */
		ST_KEY_PRESSED		/* some keys are pressed */
	} state;
};


int kmkbd_match(struct device *, struct cfdata *, void *);
void kmkbd_attach(struct device *, struct device *, void *);

CFATTACH_DECL(kmkbd, sizeof(struct kmkbd_softc),
    kmkbd_match, kmkbd_attach, NULL, NULL);

static  int	kmkbd_enable(void *, int);
static  void	kmkbd_set_leds(void *, int);
static  int	kmkbd_ioctl(void *, u_long, void *, int, struct lwp *);

const struct wskbd_accessops kmkbd_accessops = {
	kmkbd_enable,
	kmkbd_set_leds,
	kmkbd_ioctl,
};

#if 0
void	kmkbd_cngetc(void *, u_int *, int *);
void	kmkbd_cnpollc(void *, int);
void	kmkbd_cnbell(void *, u_int, u_int, u_int);

const struct wskbd_consops kmkbd_consops = {
	kmkbd_cngetc,
	kmkbd_cnpollc,
	kmkbd_cnbell,
};
#endif

static const keysym_t kmkbd_keydesc_0[] = {
/*	pos              normal		shifted */
	KS_KEYCODE(0),   KS_a,  	KS_A,
	KS_KEYCODE(1),   KS_b,  	KS_B,
	KS_KEYCODE(2),   KS_c,  	KS_C,
	KS_KEYCODE(3),   KS_d,  	KS_D,
	KS_KEYCODE(4),   KS_e,  	KS_E,
	KS_KEYCODE(5),   KS_f,  	KS_F,
	KS_KEYCODE(6),   KS_g,  	KS_G,
	KS_KEYCODE(7),   KS_h,  	KS_H,
	KS_KEYCODE(8),   KS_i,  	KS_I,
	KS_KEYCODE(9),   KS_j,  	KS_J,
	KS_KEYCODE(10),   KS_k,  	KS_K,
	KS_KEYCODE(11),   KS_l,  	KS_L,
	KS_KEYCODE(12),   KS_m,  	KS_M,
	KS_KEYCODE(13),   KS_n,  	KS_N,
	KS_KEYCODE(14),   KS_o,  	KS_O,
	KS_KEYCODE(15),   KS_p,  	KS_P,
	KS_KEYCODE(16),   KS_q,  	KS_Q,
	KS_KEYCODE(17),   KS_r,  	KS_R,
	KS_KEYCODE(18),   '\003',  	'\003',
	KS_KEYCODE(19),   KS_Return,  	KS_Linefeed,
};

#define KBD_MAP(name, base, map) \
			{ name, base, sizeof(map)/sizeof(keysym_t), map }

static const struct wscons_keydesc kmkbd_keydesctab[] = {
	KBD_MAP(KB_MACHDEP, 0, kmkbd_keydesc_0),
	{0, 0, 0, 0}
};

const struct wskbd_mapdata kmkbd_keymapdata = {
	kmkbd_keydesctab,
#ifdef KMKBD_LAYOUT
	KMKBD_LAYOUT,
#else
	KB_MACHDEP,
#endif
};

/*
 * Hackish support for a bell on the PC Keyboard; when a suitable feeper
 * is found, it attaches itself into the pckbd driver here.
 */
void	(*kmkbd_bell_fn)(void *, u_int, u_int, u_int, int);
void	*kmkbd_bell_fn_arg;

void	kmkbd_bell(u_int, u_int, u_int, int);
void	kmkbd_hookup_bell(void (* fn)(void *, u_int, u_int, u_int, int), void *arg);

static int 	kmkbd_intr(void *);
static void 	kmkbd_new_state(struct kmkbd_softc *, enum kmkbd_state);

/*struct kmkbd_internal kmkbd_consdata;*/

static int
kmkbd_is_console(void)
{
#if 0
	return (kmkbd_consdata.t_isconsole &&
		(tag == kmkbd_consdata.t_kbctag) &&
		(slot == kmkbd_consdata.t_kbcslot));
#else
	return 0;
#endif
}

int
kmkbd_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return 1;
}

void
kmkbd_attach(struct device *parent, struct device *self, void *aux)
{
	struct kmkbd_softc *sc = (void *)self;
	/*struct obio_attach_args *oa = aux;*/
	int state0;
	struct wskbddev_attach_args a;
	struct obio_softc *osc = (struct obio_softc *)parent;
	int s;

	printf("\n");

	sc->state = ST_INIT;

	if (kmkbd_is_console()){
		a.console = 1;
		state0 = ST_ALL_UP;
	} else {
		a.console = 0;
		state0 = ST_DISABLED;
	}

	callout_init(&sc->callout, 0);

	s = spltty();
	sc->ih = obio_intr_establish(osc, G42XXEB_INT_KEY, IPL_TTY, 
	    IST_EDGE_FALLING, kmkbd_intr, (void *)sc);
	kmkbd_new_state(sc, state0);
	splx(s);

	a.keymap = &kmkbd_keymapdata;

	a.accessops = &kmkbd_accessops;
	a.accesscookie = sc;


	/* Attach the wskbd. */
	sc->wskbddev = config_found(self, &a, wskbddevprint);

}

static int
kmkbd_enable(void *v, int on)
{
	struct kmkbd_softc *sc = v;

	if (on) {
		if (sc->state != ST_DISABLED) {
#ifdef DIAGNOSTIC
			printf("kmkbd_enable: bad enable (state=%d)\n", sc->state);
#endif
			return (EBUSY);
		}

		kmkbd_new_state(sc, ST_ALL_UP);
	} else {
#if 0
		if (sc->id->t_isconsole)
			return (EBUSY);
#endif

		kmkbd_new_state(sc, ST_DISABLED);
	}

	return (0);
}



static void
kmkbd_set_leds(void *v, int leds)
{
}

static int
kmkbd_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
	/*struct kmkbd_softc *sc = v;*/

	switch (cmd) {
	    case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_PC_XT; /* XXX */
		return 0;
	    case WSKBDIO_COMPLEXBELL:
#define d ((struct wskbd_bell_data *)data)
		/*
		 * Keyboard can't beep directly; we have an
		 * externally-provided global hook to do this.
		 */
		kmkbd_bell(d->pitch, d->period, d->volume, 0);
#undef d
		return (0);
#ifdef WSDISPLAY_COMPAT_RAWKBD
	    case WSKBDIO_SETMODE:
		sc->rawkbd = (*(int *)data == WSKBD_RAW);
		return (0);
#endif

#if 0
	    case WSKBDIO_SETLEDS:
	    case WSKBDIO_GETLEDS:
		    /* no LED support */
#endif
	}
	return EPASSTHROUGH;
}

void
kmkbd_bell(u_int pitch, u_int period, u_int volume, int poll)
{

	if (kmkbd_bell_fn != NULL)
		(*kmkbd_bell_fn)(kmkbd_bell_fn_arg, pitch, period,
		    volume, poll);
}

void
kmkbd_hookup_bell(void (* fn)(void *, u_int, u_int, u_int, int), void *arg)
{

	if (kmkbd_bell_fn == NULL) {
		kmkbd_bell_fn = fn;
		kmkbd_bell_fn_arg = arg;
	}
}

#if 0
int
kmkbd_cnattach(pckbc_tag_t kbctag, int kbcslot)
{
	int res;

	res = kmkbd_init(&kmkbd_consdata, kbctag, kbcslot, 1);

	wskbd_cnattach(&kmkbd_consops, &kmkbd_consdata, &kmkbd_keymapdata);

	return (0);
}

void
kmkbd_cngetc(void *v, u_int type, int *data)
{
        struct kmkbd_internal *t = v;
	int val;

	for (;;) {
		val = pckbc_poll_data(t->t_kbctag, t->t_kbcslot);
		if ((val != -1) && kmkbd_decode(t, val, type, data))
			return;
	}
}

void
kmkbd_cnpollc(void *v, int on)
{
	struct kmkbd_internal *t = v;

	pckbc_set_poll(t->t_kbctag, t->t_kbcslot, on);
}

void
kmkbd_cnbell(void *v, u_int pitch, u_int period, u_int volume)
{

	kmkbd_bell(pitch, period, volume, 1);
}
#endif


/*
 * low level access to key matrix
 *
 * returns bitset of keys being pressed.
 */
static u_int
kmkbd_read_matrix(struct kmkbd_softc *sc)
{
	int i;
	u_int ret, data;
	struct obio_softc *osc = (struct obio_softc *)device_parent(&sc->dev);
	bus_space_tag_t iot = osc->sc_iot;
	bus_space_handle_t ioh = osc->sc_obioreg_ioh;

#define KMDELAY()	delay(3)

	bus_space_write_2( iot, ioh, G42XXEB_KEYSCAN, 0 );
	KMDELAY();

	data = KEYSCAN_SENSE_IN &
	    bus_space_read_2(iot, ioh, G42XXEB_KEYSCAN);

	bus_space_write_2(iot, ioh, G42XXEB_KEYSCAN, KEYSCAN_SCAN_OUT);

	if (data == KEYSCAN_SENSE_IN)
		return 0;

	ret = 0;
	for( i=0; i<5; ++i ){
		/* scan one line */
		bus_space_write_2(iot, ioh, G42XXEB_KEYSCAN, ~(0x0100<<i));
		KMDELAY();
		data = bus_space_read_2(iot, ioh, G42XXEB_KEYSCAN );

		data = ~data & KEYSCAN_SENSE_IN;
		ret |= data << (i*4);
	}
    
	bus_space_write_2(iot, ioh, G42XXEB_KEYSCAN, KEYSCAN_SCAN_OUT);
	return ret;

#undef KMDELAY

}

/*
 * report key status change to wskbd subsystem.
 */
static void
kmkbd_report(struct kmkbd_softc *sc, u_int bitset)
{
	u_int changed;
	int i;

	if (bitset == sc->notified_bits)
		return;

	if (sc->notified_bits && bitset == 0){
		wskbd_input(sc->wskbddev, WSCONS_EVENT_ALL_KEYS_UP, 0);
		sc->notified_bits = 0;
		return;
	}

	changed = bitset ^ sc->notified_bits;
	for( i=0; changed; ++i){
		if ((changed & (1<<i)) == 0)
			continue;
		changed &= ~(1<<i);

		wskbd_input(sc->wskbddev, 
		    (bitset & (1<<i)) ? WSCONS_EVENT_KEY_DOWN : WSCONS_EVENT_KEY_UP,
		    i);
	}

	sc->notified_bits = bitset;
}

static int
kmkbd_intr(void *arg)
{
	struct kmkbd_softc *sc = arg;
	struct obio_softc *osc = (struct obio_softc *)device_parent(&sc->dev);

	if ( sc->state != ST_ALL_UP ){
		printf("Spurious interrupt from key matrix\n");
		obio_intr_mask(osc, sc->ih);
		return 1;
	}

	kmkbd_new_state(sc, ST_DEBOUNCE);

	return 1;
}

static void
kmkbd_debounce(void *arg)
{
	struct kmkbd_softc *sc = arg;
	u_int newbits;
	enum kmkbd_state new_state = ST_DEBOUNCE;
	int s = spltty();

	newbits = kmkbd_read_matrix(sc);

	if (newbits != sc->last_bits){
		sc->last_bits = newbits;
		sc->debounce_counter = 0;
	}
	else if( ++(sc->debounce_counter) >= DEBOUNCE_COUNT ){
		new_state = newbits == 0 ? ST_ALL_UP : ST_KEY_PRESSED;
		kmkbd_report(sc, newbits);
	}

	kmkbd_new_state(sc, new_state);
	splx(s);
}

/* callout routine to watch key release */
static void
kmkbd_watch(void *arg)
{
	int s = spltty();
	struct kmkbd_softc *sc = arg;
	u_int newbits;
	int new_state = ST_KEY_PRESSED;

	newbits = kmkbd_read_matrix(sc);

	if (newbits != sc->last_bits){
		/* some keys are released or new keys are pressed.
		   start debounce */
		new_state = ST_DEBOUNCE;
		sc->last_bits = newbits;
	}

	kmkbd_new_state(sc, new_state);
	splx(s);
}

static void
kmkbd_new_state(struct kmkbd_softc *sc, enum kmkbd_state new_state)
{
	struct obio_softc *osc = (struct obio_softc *)device_parent(&sc->dev);

	switch(new_state){
	case ST_DISABLED:
		if (sc->state != ST_DISABLED){
			callout_stop(&sc->callout);
			obio_intr_mask(osc,sc->ih);
		}
		break;
	case ST_DEBOUNCE:
		if (sc->state == ST_ALL_UP){
			obio_intr_mask(osc, sc->ih);
			sc->last_bits = kmkbd_read_matrix(sc);
		}
		if (sc->state != ST_DEBOUNCE)
			sc->debounce_counter = 0;

		/* start debounce timer */
		callout_reset(&sc->callout, DEBOUNCE_TICKS, kmkbd_debounce, sc);
		break;
	case ST_KEY_PRESSED:
		/* start timer to check key release */
		callout_reset(&sc->callout, RELEASE_WATCH_TICKS, kmkbd_watch, sc);
		break;
	case ST_ALL_UP:
		if (sc->state != ST_ALL_UP){
			bus_space_tag_t iot = osc->sc_iot;
			bus_space_handle_t ioh = osc->sc_obioreg_ioh;

			obio_intr_unmask(osc, sc->ih);
			bus_space_write_2(iot, ioh, G42XXEB_KEYSCAN, 0);
		}
		break;
	case ST_INIT:
		; /* Nothing to do */
	}

	sc->state = new_state;
}

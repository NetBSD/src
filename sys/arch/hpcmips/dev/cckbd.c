/*	$NetBSD: cckbd.c,v 1.1 1999/11/21 06:48:02 uch Exp $ */

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
/* XXX tentative driver XXX */

#include "opt_tx39_debug.h"
#include "opt_use_poll.h"
#include "opt_cckbd_poll.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/tty.h> 

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/pckbc/wskbdmap_mfii.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/txcsbusvar.h>
#include <hpcmips/dev/cckbdvar.h>
#include <hpcmips/dev/cckbdjpkeymap.h>

int	cckbd_match __P((struct device*, struct cfdata*, void*));
void	cckbd_attach __P((struct device*, struct device*, void*));
int	cckbd_intr __P((void*));

#define	CCKBD_RX_RING_SIZE	256
#define CCKBD_RX_RING_MASK (CCKBD_RX_RING_SIZE-1)

#define SERACH_MAX	129 /* XXX why 129? this is experimental value XXX */
#define PATTERN_MAX	70

struct cckbd_softc;

struct cckbd_chip {
	tx_chipset_tag_t	cc_tc;
	bus_space_tag_t		cc_cst;
	bus_space_handle_t	cc_csh;
	u_int16_t		cc_buf[SERACH_MAX];
	u_int32_t cc_key;
	int cc_state;
	int cc_polling;
	int cc_console;
	/*
	 * The receive ring buffer.
	 */
	volatile int	cc_kbd_rbget;	/* ring buffer `get' index */
	volatile int	cc_kbd_rbput;	/* ring buffer `put' index */
	int	cc_kbd_rbuf[CCKBD_RX_RING_SIZE];

	struct cckbd_softc* cc_sc;	/* back link */
};

struct cckbd_softc {
	struct	device		sc_dev;
	struct cckbd_chip	*sc_cc;
	struct device		*sc_wskbddev;
};

struct cfattach cckbd_ca = {
	sizeof(struct cckbd_softc), cckbd_match, cckbd_attach
};

void	__cckbd_decode	__P((int*, int, int, u_int*, int*));
/* wskbd accessopts */
int cckbd_enable __P((void *, int));
void cckbd_set_leds __P((void *, int));
int cckbd_ioctl __P((void *, u_long, caddr_t, int, struct proc *));

/* consopts */
struct cckbd_chip cckbd_consdata;
void cckbd_cngetc __P((void*, u_int*, int*));
void cckbd_cnpollc __P((void *, int));

const struct wskbd_accessops cckbd_accessops = {
	cckbd_enable,
	cckbd_set_leds,
	cckbd_ioctl,
};

const struct wskbd_consops cckbd_consops = {
	cckbd_cngetc,
	cckbd_cnpollc,
};

/* XXX dummy keymap */
const struct wskbd_mapdata cckbd_keymapdata = {
	pckbd_keydesctab,
	KB_US,
};

int
cckbd_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

void
cckbd_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct cs_attach_args *ca = aux;
	struct cckbd_softc *sc = (void*)self;
	struct cckbd_chip *cc;
	struct wskbddev_attach_args wa;

	sc->sc_cc = cc = &cckbd_consdata;
	cc->cc_polling = 0;
	cc->cc_sc = sc; /* back link */

	cc->cc_tc = ca->ca_tc;
	cc->cc_cst = ca->ca_csio.cstag;
	printf("\n");
	/* Allocate kbd buffer */
	bus_space_map(cc->cc_cst, ca->ca_csio.csbase, ca->ca_csio.cssize, 
		      0, &cc->cc_csh);
#ifdef CCKBD_POLL
	tx39_poll_establish(cc->cc_tc, 5, IST_EDGE, IPL_TTY, cckbd_intr, sc);
#else
#warning nointrrupt
#endif
	wa.console = cc->cc_console;
	wa.keymap = &cckbd_keymapdata;
	wa.accessops = &cckbd_accessops;
	wa.accesscookie = sc;

	sc->sc_wskbddev = config_found(self, &wa, wskbddevprint);
}

void
__cckbd_decode(state, datain1, datain2, type, dataout)
	int *state;
	int datain1, datain2;
	u_int *type;
	int *dataout;
{
	*dataout = __cckbd_keydesc_jp[datain1][datain2].normal;
}

static __inline int
__get_index(int i)
{
	switch (i) {
	case 129:
		return 0;
	case 128:
		return 1;
	case 96:
		return 2;
	case 80:
		return 3;
	case 72:
		return 4;
	case 68:
		return 5;
	case 66:
		return 6;
	case 65:
		return 7;
	case 2:
		return 8;
	}
	return -1;
}

int
cckbd_intr(arg)
 	void *arg;
{
	struct cckbd_softc *sc = arg;
	struct cckbd_chip *cc = sc->sc_cc;
	bus_space_tag_t t = cc->cc_cst;
	bus_space_handle_t h = cc->cc_csh;
	u_int16_t buf[SERACH_MAX], pattern, n, mask;
	u_int32_t key;
	int i, j, k, shift;

	n = pattern = 0;
	for (i = 0; i < SERACH_MAX; i++) {
		if ((buf[i] = bus_space_read_2(t, h, i * 2))) {
			pattern |= buf[i];
			n++;
		}
	}

	if (!pattern) {
		cc->cc_key = 0;
		return 0; 
	}
	if (key == cc->cc_key) {
		return 0;
	}

	cc->cc_key = key;
	
	for (i = 0; i < 16; i++) { /* Loop for each bit */
		u_int type;
		int val;
		mask = 1 << i;
		if (pattern & mask) {
			k = 0;
			for (j = 0; j < SERACH_MAX; j++) {
				if (buf[j] & mask)
					k++;
			}
			if ((shift = __get_index(k)) < 0) {
				break;
			}
			__cckbd_decode(&cc->cc_state, i, shift, &type, &val);
			{
				int put = cc->cc_kbd_rbput;
				cc->cc_kbd_rbuf[put] = val;
				cc->cc_kbd_rbput = (put + 1) & CCKBD_RX_RING_MASK; /* XXX overrun */
			}
			if (!cc->cc_polling) {
				char buf = val;
				wskbd_rawinput(sc->sc_wskbddev, &buf, 1);
				cc->cc_kbd_rbget = (cc->cc_kbd_rbget + 1) & CCKBD_RX_RING_MASK;
			}
		}
	}

	return 0;
}


/*
 * console support routines
 */
int
cckbd_cnattach(iot, iobase)
	bus_space_tag_t iot;
	int iobase;
{
	cckbd_consdata.cc_console = 1;
	wskbd_cnattach(&cckbd_consops, &cckbd_consdata, &cckbd_keymapdata);

	return 0;
}

void
cckbd_cngetc(arg, type, data)
	void *arg;
	u_int *type;
	int *data;
{
	struct cckbd_chip *cc = arg;
	struct cckbd_softc tmpsc;
	volatile int get;

	if (!cc->cc_console || !cc->cc_polling) {
		return;
	}
	tmpsc.sc_cc = cc;

	for (;;) {
		cckbd_intr(&tmpsc);

		get = cc->cc_kbd_rbget;
		if (get != cc->cc_kbd_rbput) {
			cc->cc_kbd_rbget = (get + 1) & CCKBD_RX_RING_MASK;
			*data = cc->cc_kbd_rbuf[get];
			return;
		}
	}
}

void
cckbd_cnpollc(arg, on)
	void *arg;
        int on;
{
	struct cckbd_chip *cc = arg;

	cc->cc_polling = on;
}

int
cckbd_enable(scx, on)
	void *scx;
	int on;
{
	return 0;
}

void
cckbd_set_leds(scx, leds)
	void *scx;
	int leds;
{
}

int
cckbd_ioctl(scx, cmd, data, flag, p)
	void *scx;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	return 0;
}

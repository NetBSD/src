/*	$NetBSD: ewskbd.c,v 1.4.2.1 2007/07/15 13:15:52 ad Exp $	*/

/*
 * Copyright (c) 2005 Izumi Tsutsui
 * Copyright (c) 2004 Steve Rumble
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * EWS4800 serial keyboard driver attached to zs channel 0 at 4800 bps.
 * This layer is the parent of wskbd.
 *
 * This driver is taken from sgimips.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ewskbd.c,v 1.4.2.1 2007/07/15 13:15:52 ad Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <dev/ic/z8530reg.h>
#include <machine/z8530var.h>

#include <ews4800mips/dev/ews4800keymap.h>

#define EWSKBD_BAUD		4800

#define EWSKBD_TXQ_LEN		16		/* power of 2 */
#define EWSKBD_TXQ_LEN_MASK	(EWSKBD_TXQ_LEN - 1)
#define EWSKBD_NEXTTXQ(x)	(((x) + 1) & EWSKBD_TXQ_LEN_MASK)

#define EWSKBD_RXQ_LEN		64		/* power of 2 */
#define EWSKBD_RXQ_LEN_MASK	(EWSKBD_RXQ_LEN - 1)
#define EWSKBD_NEXTRXQ(x)	(((x) + 1) & EWSKBD_RXQ_LEN_MASK)

#define EWSKBD_KEY_UP		0x80
#define EWSKBD_KEY_MASK		0x7f

#ifdef EWSKBD_DEBUG
int ewskbd_debug = 0;
#define DPRINTF(_x) if (ewskbd_debug) printf _x
#else
#define DPRINTF(_x)
#endif

struct ewskbd_softc {
	struct device sc_dev;
	struct ewskbd_devconfig *sc_dc;
};

struct ewskbd_devconfig {
	/* transmit tail-chasing fifo */
	uint8_t txq[EWSKBD_TXQ_LEN];
	u_int txq_head;
	u_int txq_tail;

	/* receive tail-chasing fifo */
	uint8_t rxq[EWSKBD_RXQ_LEN];
	u_int rxq_head;
	u_int rxq_tail;

	/* state */
	u_int state;
#define TX_READY	0x01

	/* LED status */
	uint8_t	leds;
#define EWSKBD_SETLEDS	0x90
#define EWSKBD_CAPSLOCK	0x02
#define EWSKBD_KANA	0x04

	/* wscons glue */
	struct device *wskbddev;
	int enabled;
};

static int  ewskbd_zsc_match(struct device *, struct cfdata *, void *);
static void ewskbd_zsc_attach(struct device *, struct device *, void *);
static int  ewskbd_zsc_init(struct zs_chanstate *);
static void ewskbd_zsc_rxint(struct zs_chanstate *);
static void ewskbd_zsc_stint(struct zs_chanstate *, int);
static void ewskbd_zsc_txint(struct zs_chanstate *);
static void ewskbd_zsc_softint(struct zs_chanstate *);
static void ewskbd_zsc_send(struct zs_chanstate *, uint8_t *, u_int);

static void ewskbd_wskbd_input(struct zs_chanstate *, u_char);
static int  ewskbd_wskbd_enable(void *, int);
static void ewskbd_wskbd_set_leds(void *, int);
static int  ewskbd_wskbd_get_leds(void *);
static int  ewskbd_wskbd_ioctl(void *, u_long, void *, int, struct lwp *);

void ewskbd_zsc_cnattach(uint32_t, uint32_t, int);
static void ewskbd_zsc_wskbd_getc(void *, u_int *, int *);
static void ewskbd_wskbd_pollc(void *, int);
static void ewskbd_wskbd_bell(void *, u_int, u_int, u_int);

CFATTACH_DECL(ewskbd_zsc, sizeof(struct ewskbd_softc),
    ewskbd_zsc_match, ewskbd_zsc_attach, NULL, NULL);

static struct zsops ewskbd_zsops = {
	ewskbd_zsc_rxint,
	ewskbd_zsc_stint,
	ewskbd_zsc_txint,
	ewskbd_zsc_softint
};

const struct wskbd_mapdata ews4800kbd_wskbd_keymapdata = {
	ews4800kbd_keydesctab,
	KB_JP
};

const struct wskbd_accessops ewskbd_wskbd_accessops = {
	ewskbd_wskbd_enable,
	ewskbd_wskbd_set_leds,
	ewskbd_wskbd_ioctl
};

const struct wskbd_consops ewskbd_wskbd_consops = {
	ewskbd_zsc_wskbd_getc,
	ewskbd_wskbd_pollc,
	ewskbd_wskbd_bell
};

static struct ewskbd_devconfig ewskbd_console_dc;
static struct zs_chanstate conschan;
static int ewskbd_is_console;

static int
ewskbd_zsc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct zsc_attach_args *zsc_args = aux;
	struct zsc_softc *zsc = (void *)parent;

	/* keyboard is on channel B */
	if ((zsc->zsc_flags & 0x0001 /* kbms port */) != 0 &&
	    zsc_args->channel == 1)
		/* prior to generic zstty(4) */
		return 3;

	return 0;
}

static void
ewskbd_zsc_attach(struct device *parent, struct device *self, void *aux)
{
	struct ewskbd_softc *sc;
	struct zs_chanstate *cs;
	struct zsc_softc *zsc;
	struct zsc_attach_args *zsc_args;
	struct wskbddev_attach_args wskaa;
	int channel;

	zsc = (void *)parent;
	sc = (void *)self;
	zsc_args = aux;

	/* Establish ourself with the MD z8530 driver */
	channel = zsc_args->channel;
	cs = zsc->zsc_cs[channel];

	if (ewskbd_is_console) {
		sc->sc_dc = &ewskbd_console_dc;
		wskaa.console = 1;
		sc->sc_dc->enabled = 1;
	} else {
		wskaa.console = 0;

		sc->sc_dc = malloc(sizeof(struct ewskbd_devconfig), M_DEVBUF,
		    M_WAITOK | M_ZERO);
		if (sc->sc_dc == NULL) {
			printf(": can't allocate memory\n");
			return;
		}
		sc->sc_dc->enabled = 0;
	}
	cs->cs_defspeed = EWSKBD_BAUD;
	cs->cs_ops = &ewskbd_zsops;
	cs->cs_private = sc;

	sc->sc_dc->txq_head = 0;
	sc->sc_dc->txq_tail = 0;
	sc->sc_dc->rxq_head = 0;
	sc->sc_dc->rxq_tail = 0;
	sc->sc_dc->state = TX_READY;
	sc->sc_dc->leds = 0;

	ewskbd_zsc_init(cs);

	/* set default LED */
	ewskbd_wskbd_set_leds(cs, 0);

	printf(": baud rate %d\n", EWSKBD_BAUD);

	/* attach wskbd */
	wskaa.keymap = &ews4800kbd_wskbd_keymapdata;
	wskaa.accessops = &ewskbd_wskbd_accessops;
	wskaa.accesscookie = cs;
	sc->sc_dc->wskbddev = config_found(self, &wskaa, wskbddevprint);
}

static int
ewskbd_zsc_init(struct zs_chanstate *cs)
{
	int s;

	s = splzs();

	zs_write_reg(cs, 9, ZSWR9_B_RESET);
	DELAY(100);
	zs_write_reg(cs, 9, ZSWR9_MASTER_IE | ZSWR9_NO_VECTOR);

	cs->cs_preg[1] = ZSWR1_RIE | ZSWR1_TIE;
	cs->cs_preg[2] = 0;
	cs->cs_preg[3] = ZSWR3_RX_8 | ZSWR3_RX_ENABLE;
	cs->cs_preg[4] = ZSWR4_CLK_X16 | ZSWR4_ONESB | ZSWR4_PARENB;
	cs->cs_preg[5] = ZSWR5_TX_8 | ZSWR5_RTS | ZSWR5_TX_ENABLE;
	cs->cs_preg[6] = 0;
	cs->cs_preg[7] = 0;
	cs->cs_preg[8] = 0;
	cs->cs_preg[9] = ZSWR9_MASTER_IE | ZSWR9_NO_VECTOR;
	cs->cs_preg[10] = 0;
	cs->cs_preg[11] = ZSWR11_RXCLK_BAUD | ZSWR11_TXCLK_BAUD |
	    ZSWR11_TRXC_OUT_ENA | ZSWR11_TRXC_BAUD;
	/* reg[11] and reg[12] are set by zs_set_speed() with cs_brg_clk */
	zs_set_speed(cs, EWSKBD_BAUD);
	cs->cs_preg[14] = ZSWR14_BAUD_FROM_PCLK | ZSWR14_BAUD_ENA;
	cs->cs_preg[15] = 0;

	zs_loadchannelregs(cs);

	splx(s);

	return 0;
}

static void
ewskbd_zsc_rxint(struct zs_chanstate *cs)
{
	struct ewskbd_softc *sc;
	struct ewskbd_devconfig *dc;
	uint8_t c, r;

	sc = cs->cs_private;
	dc = sc->sc_dc;

	/* clear errors */
	r = zs_read_reg(cs, 1);
	if (r & (ZSRR1_FE | ZSRR1_DO | ZSRR1_PE))
		zs_write_csr(cs, ZSWR0_RESET_ERRORS);

	/* read byte and append to our queue */
	c = zs_read_data(cs);

	dc->rxq[dc->rxq_tail] = c;
	dc->rxq_tail = EWSKBD_NEXTRXQ(dc->rxq_tail);

	cs->cs_softreq = 1;
}

static void
ewskbd_zsc_stint(struct zs_chanstate *cs, int force)
{

	zs_write_csr(cs, ZSWR0_RESET_STATUS);
	cs->cs_softreq = 1;
}

static void
ewskbd_zsc_txint(struct zs_chanstate *cs)
{
	struct ewskbd_softc *sc;

	sc = cs->cs_private;
	zs_write_reg(cs, 0, ZSWR0_RESET_TXINT);
	sc->sc_dc->state |= TX_READY;
	cs->cs_softreq = 1;
}

static void
ewskbd_zsc_softint(struct zs_chanstate *cs)
{
	struct ewskbd_softc *sc;
	struct ewskbd_devconfig *dc;

	sc = cs->cs_private;
	dc = sc->sc_dc;

	/* handle pending transmissions */
	if (dc->txq_head != dc->txq_tail && (dc->state & TX_READY)) {
		int s;

		dc->state &= ~TX_READY;

		s = splzs();
		zs_write_data(cs, dc->txq[dc->txq_head]);
		splx(s);

		dc->txq_head = EWSKBD_NEXTTXQ(dc->txq_head);
	}

	/* don't bother if nobody is listening */
	if (!dc->enabled) {
		dc->rxq_head = dc->rxq_tail;
		return;
	}

	/* handle incoming keystrokes/config */
	while (dc->rxq_head != dc->rxq_tail) {
		uint8_t key = dc->rxq[dc->rxq_head];

		/* toss wskbd a bone */
		ewskbd_wskbd_input(cs, key);

		dc->rxq_head = EWSKBD_NEXTRXQ(dc->rxq_head);
	}
}

/* expects to be in splzs() */
static void
ewskbd_zsc_send(struct zs_chanstate *cs, uint8_t *c, u_int len)
{
	struct ewskbd_softc *sc;
	struct ewskbd_devconfig *dc;
	int i;

	sc = cs->cs_private;
	dc = sc->sc_dc;

	for (i = 0; i < len; i++) {
		if (dc->state & TX_READY) {
			zs_write_data(cs, c[i]);
			dc->state &= ~TX_READY;
		} else {
			dc->txq[dc->txq_tail] = c[i];
			dc->txq_tail = EWSKBD_NEXTTXQ(dc->txq_tail);
			cs->cs_softreq = 1;
		}
	}
}

/******************************************************************************
 * wskbd glue
 ******************************************************************************/

static void
ewskbd_wskbd_input(struct zs_chanstate *cs, uint8_t key)
{
	struct ewskbd_softc *sc;
	u_int type;

	sc = cs->cs_private;

	if (key & EWSKBD_KEY_UP)
		type = WSCONS_EVENT_KEY_UP;
	else
		type = WSCONS_EVENT_KEY_DOWN;

	wskbd_input(sc->sc_dc->wskbddev, type, (key & EWSKBD_KEY_MASK));

	DPRINTF(("ewskbd_wskbd_input: inputted key 0x%x\n", key));

#ifdef WSDISPLAY_COMPAT_RAWKBD
	wskbd_rawinput(sc->sc_dc->wskbddev, &key, 1);
#endif
}

static int
ewskbd_wskbd_enable(void *cookie, int on)
{
	struct zs_chanstate *cs;
	struct ewskbd_softc *sc;

	cs = cookie;
	sc = cs->cs_private;

	if (on) {
		if (sc->sc_dc->enabled)
			return EBUSY;
		else
			sc->sc_dc->enabled = 1;
	} else
		sc->sc_dc->enabled = 0;

	DPRINTF(("ewskbd_wskbd_enable: %s\n", on ? "enabled" : "disabled"));

	return 0;
}

static void
ewskbd_wskbd_set_leds(void *cookie, int leds)
{
	struct zs_chanstate *cs;
	struct ewskbd_softc *sc;
	int s;
	uint8_t	cmd;

	cs = cookie;
	sc = cs->cs_private;
	cmd = 0;

	if (leds & WSKBD_LED_CAPS)
		cmd |= EWSKBD_CAPSLOCK;

	sc->sc_dc->leds = cmd;

	cmd |= EWSKBD_SETLEDS;

	s = splzs();
	ewskbd_zsc_send(cs, &cmd, 1);
	splx(s);
}

static int
ewskbd_wskbd_get_leds(void *cookie)
{
	struct zs_chanstate *cs;
	struct ewskbd_softc *sc;
	int leds;

	cs = cookie;
	sc = cs->cs_private;
	leds = 0;

	if (sc->sc_dc->leds & EWSKBD_CAPSLOCK)
		leds |= WSKBD_LED_CAPS;

	return leds;
}

static int
ewskbd_wskbd_ioctl(void *cookie, u_long cmd, void *data, int flag,
    struct lwp *l)
{

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_EWS4800;
		break;

#ifdef notyet
	case WSKBDIO_BELL:
	case WSKBDIO_COMPLEXBELL:
	case WSKBDIO_SETBELL:
	case WSKBDIO_GETBELL:
	case WSKBDIO_SETDEFAULTBELL:
	case WSKBDIO_GETDEFAULTBELL:
	case WSKBDIO_SETKEYREPEAT:
	case WSKBDIO_GETKEYREPEAT:
	case WSKBDIO_SETDEFAULTKEYREPEAT:
	case WSKBDIO_GETDEFAULTKEYREPEAT:
#endif

	case WSKBDIO_SETLEDS:
		ewskbd_wskbd_set_leds(cookie, *(int *)data);
		break;

	case WSKBDIO_GETLEDS:
		*(int *)data = ewskbd_wskbd_get_leds(cookie);
		break;

#ifdef notyet
	case WSKBDIO_GETMAP:
	case WSKBDIO_SETMAP:
	case WSKBDIO_GETENCODING:
	case WSKBDIO_SETENCODING:
	case WSKBDIO_SETMODE:
	case WSKBDIO_GETMODE:
	case WSKBDIO_SETKEYCLICK:
	case WSKBDIO_GETKEYCLICK:
#endif

	default:
		return EPASSTHROUGH;
	}

	return 0;
}

/*
 * console routines
 */
void
ewskbd_zsc_cnattach(uint32_t csr, uint32_t data, int pclk)
{
	struct zs_chanstate *cs;

	cs = &conschan;

	cs->cs_reg_csr  = (void *)csr;
	cs->cs_reg_data = (void *)data;
	cs->cs_brg_clk =  pclk / 16;
	cs->cs_defspeed = EWSKBD_BAUD;

	ewskbd_zsc_init(cs);

	zs_putc(cs, EWSKBD_SETLEDS);

	wskbd_cnattach(&ewskbd_wskbd_consops, cs, &ews4800kbd_wskbd_keymapdata);
	ewskbd_is_console = 1;
}

static void
ewskbd_zsc_wskbd_getc(void *cookie, u_int *type, int *data)
{
	int key;

	key = zs_getc(cookie);

	if (key & EWSKBD_KEY_UP)
		*type = WSCONS_EVENT_KEY_UP;
	else
		*type = WSCONS_EVENT_KEY_DOWN;

	*data = key & EWSKBD_KEY_MASK;
}

static void
ewskbd_wskbd_pollc(void *cookie, int on)
{

	static bool __polling = false;
	static int s;

	if (on && !__polling) {
		/* disable interrupt driven I/O */
		s = splhigh();
		__polling = true;
	} else if (!on && __polling) {
		/* enable interrupt driven I/O */
		__polling = false;
		splx(s);
	}
}

static void
ewskbd_wskbd_bell(void *cookie, u_int pitch, u_int period, u_int volume)
{

	/* nothing */
}

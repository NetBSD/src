/*	$NetBSD: ewsms.c,v 1.1 2005/12/29 15:20:08 tsutsui Exp $	*/

/*
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
 * EWS4800 serial mouse driver attached to zs channel 0 at 4800 or 1200bps.
 * This layer feeds wsmouse.
 *
 * 5 byte packets: sync, x1, y1, x2, y2
 * sync format: binary 10000LMR (left, middle, right) 0 is down
 *
 * This driver is taken from sgimips, but ews4800 has the similar protocol.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ewsms.c,v 1.1 2005/12/29 15:20:08 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#include <dev/ic/z8530reg.h>
#include <machine/z8530var.h>

#define EWSMS_BAUD		1200
#define EWSMS_BAUD1		4800

#define EWSMS_RXQ_LEN		64	/* power of 2 */
#define EWSMS_RXQ_LEN_MASK	(EWSMS_RXQ_LEN - 1)
#define EWSMS_NEXTRXQ(x)	(((x) + 1) & EWSMS_RXQ_LEN_MASK)

/* protocol */
#define EWSMS_SYNC0		0x80
#define EWSMS_SYNC1		0x88
#define EWSMS_SYNC_MASK		0xf8
#define EWSMS_SYNC_BTN_R	0x01
#define EWSMS_SYNC_BTN_M	0x02
#define EWSMS_SYNC_BTN_L	0x04
#define EWSMS_SYNC_BTN_MASK	\
	(EWSMS_SYNC_BTN_R|EWSMS_SYNC_BTN_M|EWSMS_SYNC_BTN_L)

struct ewsms_softc {
	struct device sc_dev;

	/* tail-chasing fifo */
	uint8_t sc_rxq[EWSMS_RXQ_LEN];
	u_int sc_rxq_head;
	u_int sc_rxq_tail;

	/* 5-byte packet as described above */
	int8_t	sc_packet[5];
#define EWSMS_PACKET_SYNC	0
#define EWSMS_PACKET_X1		1
#define EWSMS_PACKET_Y1		2
#define EWSMS_PACKET_X2		3
#define EWSMS_PACKET_Y2		4

	u_int sc_state;
#define EWSMS_STATE_SYNC	0x01
#define EWSMS_STATE_X1		0x02
#define EWSMS_STATE_Y1		0x04
#define EWSMS_STATE_X2		0x08
#define EWSMS_STATE_Y2		0x10

	u_int sc_baud;
	u_int sc_flags;
#define EWSMS_F_FASTBAUD	0x01
#define EWSMS_F_RSTCMD		0x02
#define EWSMS_F_SWAPBTN		0x04
#define EWSMS_F_SYNC1		0x08

	/* wsmouse bits */
	int sc_enabled;
	struct device *sc_wsmousedev;
};

static int  ewsms_zsc_match(struct device *, struct cfdata *, void *);
static void ewsms_zsc_attach(struct device *, struct device *, void *);
static int  ewsms_zsc_reset(struct zs_chanstate *);
static void ewsms_zsc_rxint(struct zs_chanstate *);
static void ewsms_zsc_txint(struct zs_chanstate *);
static void ewsms_zsc_stint(struct zs_chanstate *, int);
static void ewsms_zsc_softint(struct zs_chanstate *);

static void ewsms_wsmouse_input(struct ewsms_softc *);
static int  ewsms_wsmouse_enable(void *);
static void ewsms_wsmouse_disable(void *);
static int  ewsms_wsmouse_ioctl(void *, u_long, caddr_t, int, struct lwp *);

CFATTACH_DECL(ewsms_zsc, sizeof(struct ewsms_softc),
    ewsms_zsc_match, ewsms_zsc_attach, NULL, NULL);

static struct zsops ewsms_zsops = {
	ewsms_zsc_rxint,
	ewsms_zsc_stint,
	ewsms_zsc_txint,
	ewsms_zsc_softint
};

static const struct wsmouse_accessops ewsms_wsmouse_accessops = {
	ewsms_wsmouse_enable,
	ewsms_wsmouse_ioctl,
	ewsms_wsmouse_disable
};

int
ewsms_zsc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct zsc_softc *zsc = (void *)parent;
	struct zsc_attach_args *zsc_args = aux;

	/* mouse on channel A */
	if ((zsc->zsc_flags & 0x0001 /* kbms port */) != 0 &&
	    zsc_args->channel == 0)
		/* prior to generic zstty(4) */
		return 3;

	return 0;
}

void
ewsms_zsc_attach(struct device *parent, struct device *self, void *aux)
{
	struct zsc_softc *zsc = (void *)parent;
	struct ewsms_softc *sc = (void *)self;
	struct zsc_attach_args *zsc_args = aux;
	struct zs_chanstate *cs;
	struct wsmousedev_attach_args wsmaa;
	int channel;

	/* Establish ourself with the MD z8530 driver */
	channel = zsc_args->channel;
	cs = zsc->zsc_cs[channel];
	cs->cs_ops = &ewsms_zsops;
	cs->cs_private = sc;

	sc->sc_enabled = 0;
	sc->sc_rxq_head = 0;
	sc->sc_rxq_tail = 0;
	sc->sc_state = EWSMS_STATE_SYNC;

	if (zsc->zsc_flags & 0x0002) {
		sc->sc_flags |= EWSMS_F_RSTCMD;
		sc->sc_flags |= EWSMS_F_FASTBAUD;
		sc->sc_flags |= EWSMS_F_SWAPBTN;
		sc->sc_flags |= EWSMS_F_SYNC1;
	}

	ewsms_zsc_reset(cs);

	printf(": baud rate %d\n", EWSMS_BAUD);

	/* attach wsmouse */
	wsmaa.accessops = &ewsms_wsmouse_accessops;
	wsmaa.accesscookie = sc;
	sc->sc_wsmousedev = config_found(self, &wsmaa, wsmousedevprint);
}

int
ewsms_zsc_reset(struct zs_chanstate *cs)
{
	struct ewsms_softc *sc = cs->cs_private;
	u_int baud;
	int i, s;
	const static uint8_t cmd[] =
	    { 0x02, 0x52, 0x53, 0x3b, 0x4d, 0x54, 0x2c, 0x36, 0x0d };

	s = splzs();
	zs_write_reg(cs, 9, ZSWR9_A_RESET);
	cs->cs_preg[1] = ZSWR1_RIE;
	baud = EWSMS_BAUD;
	if (sc->sc_flags & EWSMS_F_FASTBAUD)
		baud = EWSMS_BAUD1;
	zs_set_speed(cs, baud);
	zs_loadchannelregs(cs);

	if (sc->sc_flags & EWSMS_F_RSTCMD) {
		for (i = 0; i < sizeof(cmd); i++)
			zs_putc(cs, cmd[i]);
	}

	splx(s);

	return 0;
}

void
ewsms_zsc_rxint(struct zs_chanstate *cs)
{
	struct ewsms_softc *sc = cs->cs_private;
	u_char c, r;

	/* clear errors */
	r = zs_read_reg(cs, 1);
	if (r & (ZSRR1_FE | ZSRR1_DO | ZSRR1_PE))
		zs_write_csr(cs, ZSWR0_RESET_ERRORS);

	/* read byte and append to our queue */
	c = zs_read_data(cs);

	sc->sc_rxq[sc->sc_rxq_tail] = c;
	sc->sc_rxq_tail = EWSMS_NEXTRXQ(sc->sc_rxq_tail);

	cs->cs_softreq = 1;
}

/* We should never get here. */
void
ewsms_zsc_txint(struct zs_chanstate *cs)
{
	zs_write_reg(cs, 0, ZSWR0_RESET_TXINT);

	/* seems like the in thing to do */
	cs->cs_softreq = 1;
}

void
ewsms_zsc_stint(struct zs_chanstate *cs, int force)
{

	zs_write_csr(cs, ZSWR0_RESET_STATUS);
	cs->cs_softreq = 1;
}

void
ewsms_zsc_softint(struct zs_chanstate *cs)
{
	struct ewsms_softc *sc = cs->cs_private;
	uint8_t sync;

	/* No need to keep score if nobody is listening */
	if (!sc->sc_enabled) {
		sc->sc_rxq_head = sc->sc_rxq_tail;
		return;
	}

	/*
	 * Here's the real action. Read a full packet and
	 * then let wsmouse know what has happened.
	 */
	while (sc->sc_rxq_head != sc->sc_rxq_tail) {
		int8_t c = sc->sc_rxq[sc->sc_rxq_head];

		switch (sc->sc_state) {
		case EWSMS_STATE_SYNC:
			sync = EWSMS_SYNC0;
			if (sc->sc_flags & EWSMS_F_SYNC1)
				sync = EWSMS_SYNC1;
			if ((c & EWSMS_SYNC_MASK) == sync) {
				sc->sc_packet[EWSMS_PACKET_SYNC] = c;
				sc->sc_state = EWSMS_STATE_X1;
			}
			break;

		case EWSMS_STATE_X1:
			sc->sc_packet[EWSMS_PACKET_X1] = c;
			sc->sc_state = EWSMS_STATE_Y1;
			break;

		case EWSMS_STATE_Y1:
			sc->sc_packet[EWSMS_PACKET_Y1] = c;
			sc->sc_state = EWSMS_STATE_X2;
			break;

		case EWSMS_STATE_X2:
			sc->sc_packet[EWSMS_PACKET_X2] = c;
			sc->sc_state = EWSMS_STATE_Y2;
			break;

		case EWSMS_STATE_Y2:
			sc->sc_packet[EWSMS_PACKET_Y2] = c;

			/* tweak wsmouse */
			ewsms_wsmouse_input(sc);

			sc->sc_state = EWSMS_STATE_SYNC;
		}

		sc->sc_rxq_head = EWSMS_NEXTRXQ(sc->sc_rxq_head);
	}
}

/******************************************************************************
 * wsmouse glue
 ******************************************************************************/

static void
ewsms_wsmouse_input(struct ewsms_softc *sc)
{
	u_int btns;
	boolean_t bl, bm, br;
	int dx, dy;

	btns = (uint8_t)sc->sc_packet[EWSMS_PACKET_SYNC] & EWSMS_SYNC_BTN_MASK;
	if (sc->sc_flags & EWSMS_F_SWAPBTN) {
		bl = (btns & EWSMS_SYNC_BTN_R) == 0;
		bm = (btns & EWSMS_SYNC_BTN_M) == 0;
		br = (btns & EWSMS_SYNC_BTN_L) == 0;
	} else {
		bl = (btns & EWSMS_SYNC_BTN_L) == 0;
		bm = (btns & EWSMS_SYNC_BTN_M) == 0;
		br = (btns & EWSMS_SYNC_BTN_R) == 0;
	}
	/* for wsmouse(4), 1 is down, 0 is up, most left button is LSB */
	btns = (bl ? (1 << 0) : 0) | (bm ? (1 << 1) : 0) | (br ? (1 << 2) : 0);

	/* delta values are signed */
	/* moving left is minus, moving right is plus */
	dx = (int)sc->sc_packet[EWSMS_PACKET_X1] +
	     (int)sc->sc_packet[EWSMS_PACKET_X2];
	/* moving back is minus, moving front is plus */
	dy = (int)sc->sc_packet[EWSMS_PACKET_Y1] +
	     (int)sc->sc_packet[EWSMS_PACKET_Y2];

	wsmouse_input(sc->sc_wsmousedev, btns, dx, dy, 0, WSMOUSE_INPUT_DELTA);
}

static int
ewsms_wsmouse_enable(void *cookie)
{
	struct ewsms_softc *sc = cookie;

	if (sc->sc_enabled)
		return EBUSY;

	sc->sc_state = EWSMS_STATE_SYNC;
	sc->sc_enabled = 1;

	return 0;
}

void
ewsms_wsmouse_disable(void *cookie)
{
	struct ewsms_softc *sc = cookie;

	sc->sc_enabled = 0;
}

static int
ewsms_wsmouse_ioctl(void *cookie, u_long cmd, caddr_t data, int flag,
    struct lwp *l)
{

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = 0;
		break;

#ifdef notyet
	case WSMOUSEIO_SRES:
	case WSMOUSEIO_SSCALE:
	case WSMOUSEIO_SRATE:
	case WSMOUSEIO_SCALIBCOORDS:
	case WSMOUSEIO_GCALIBCOORDS:
	case WSMOUSEIO_GETID:
#endif

	default:
		return EPASSTHROUGH;
	}

	return 0;
}

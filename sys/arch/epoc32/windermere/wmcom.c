/*      $NetBSD: wmcom.c,v 1.4 2014/08/10 16:44:33 tls Exp $      */
/*
 * Copyright (c) 2012 KIYOHARA Takashi
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
__KERNEL_RCSID(0, "$NetBSD: wmcom.c,v 1.4 2014/08/10 16:44:33 tls Exp $");

#include "rnd.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/intr.h>
#include <sys/kauth.h>
#include <sys/lwp.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/termios.h>
#include <sys/tty.h>
#include <sys/types.h>

#include <epoc32/windermere/windermerereg.h>
#include <epoc32/windermere/windermerevar.h>

#include <dev/cons.h>

#ifdef RND_COM
#include <sys/rnd.h>
#endif

#include "ioconf.h"
#include "locators.h"

#define COMUNIT_MASK	0x7ffff
#define COMDIALOUT_MASK	0x80000

#define COMUNIT(x)	(minor(x) & COMUNIT_MASK)
#define COMDIALOUT(x)	(minor(x) & COMDIALOUT_MASK)

#define WMCOM_RING_SIZE	2048

struct wmcom_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	void *sc_si;

	struct tty *sc_tty;

	u_char *sc_tba;
	u_int sc_tbc;
	u_char *sc_rbuf;
	char *volatile sc_rbget;
	char *volatile sc_rbput;
	volatile int sc_rbavail;

	int sc_tx_done;
	int sc_rx_ready;

	int sc_hwflags;
#define COM_HW_CONSOLE	(1 << 0)
#define COM_HW_DEV_OK	(1 << 1)
#define COM_HW_KGDB	(1 << 2)
	int sc_swflags;

	int sc_flags;
#define WMCOM_IRDA	(1 << 0)

#ifdef RND_COM
	krandsource_t rnd_source;
#endif
};

static int wmcom_match(device_t, cfdata_t, void *);
static void wmcom_attach(device_t, device_t, void *);

static int wmcom_intr(void *);
static void wmcom_soft(void *);

static void wmcom_start(struct tty *);
static int wmcom_param(struct tty *, struct termios *);
static int wmcom_hwiflow(struct tty *, int);

dev_type_open(wmcomopen);
dev_type_close(wmcomclose);
dev_type_read(wmcomread);
dev_type_write(wmcomwrite);
dev_type_ioctl(wmcomioctl);
dev_type_stop(wmcomstop);
dev_type_tty(wmcomtty);
dev_type_poll(wmcompoll);

static void wmcom_iflush(struct wmcom_softc *);
static void wmcom_shutdown(struct wmcom_softc *);
static void wmcom_break(struct wmcom_softc *, int);

static void wmcom_rxsoft(struct wmcom_softc *, struct tty *);

static inline uint32_t wmcom_rate2lcr(int);
static uint8_t wmcom_cflag2fcr(tcflag_t);

static int wmcom_cngetc(dev_t);
static void wmcom_cnputc(dev_t, int);
static void wmcom_cnpollc(dev_t, int);

CFATTACH_DECL_NEW(wmcom, sizeof(struct wmcom_softc),
    wmcom_match, wmcom_attach, NULL, NULL);

const struct cdevsw wmcom_cdevsw = {
	.d_open = wmcomopen,
	.d_close = wmcomclose,
	.d_read = wmcomread,
	.d_write = wmcomwrite,
	.d_ioctl = wmcomioctl,
	.d_stop = wmcomstop,
	.d_tty = wmcomtty,
	.d_poll = wmcompoll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

static struct cnm_state wmcom_cnm_state;
static vaddr_t wmcom_cnaddr;
static int wmcom_cnrate;
static tcflag_t wmcom_cncflag;


/* ARGSUSED */
static int
wmcom_match(device_t parent, cfdata_t match, void *aux)
{
	struct windermere_attach_args *aa = aux;

	/* Wildcard not accept */
	if (aa->aa_offset == WINDERMERECF_OFFSET_DEFAULT ||
	    aa->aa_irq == WINDERMERECF_IRQ_DEFAULT)
		return 0;

	aa->aa_size = UART_SIZE;
	return 1;
}

/* ARGSUSED */
static void
wmcom_attach(device_t parent, device_t self, void *aux)
{
	struct wmcom_softc *sc = device_private(self);
	struct windermere_attach_args *aa = aux;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;
	if (windermere_bus_space_subregion(aa->aa_iot, *aa->aa_ioh,
				aa->aa_offset, aa->aa_size, &sc->sc_ioh) != 0) {
		aprint_error_dev(self, "can't map registers\n");
		return;
	}
	sc->sc_iot = aa->aa_iot;
	if (intr_establish(aa->aa_irq, IPL_SERIAL, 0, wmcom_intr, sc) == NULL) {
		aprint_error_dev(self, "can't establish interrupt\n");
		return;
	}

	if (aa->aa_offset == (wmcom_cnaddr & 0xfff))
		SET(sc->sc_hwflags, COM_HW_CONSOLE);

	if (aa->aa_offset == WINDERMERE_COM0_OFFSET)
		SET(sc->sc_flags, WMCOM_IRDA);

	sc->sc_tty = tty_alloc();
	sc->sc_tty->t_oproc = wmcom_start;
	sc->sc_tty->t_param = wmcom_param;
	sc->sc_tty->t_hwiflow = wmcom_hwiflow;

	sc->sc_tbc = 0;
	sc->sc_rbuf = malloc(WMCOM_RING_SIZE << 1, M_DEVBUF, M_NOWAIT);
	if (sc->sc_rbuf == NULL) {
		aprint_error_dev(self, "unable to allocate ring buffer\n");
		return;
	}
	sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
	sc->sc_rbavail = WMCOM_RING_SIZE;

	tty_attach(sc->sc_tty);

	if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
		int maj = cdevsw_lookup_major(&wmcom_cdevsw);

		sc->sc_tty->t_dev = makedev(maj, device_unit(sc->sc_dev));
		cn_tab->cn_dev = sc->sc_tty->t_dev;

		aprint_normal_dev(self, "console\n");
	}

	sc->sc_si = softint_establish(SOFTINT_SERIAL, wmcom_soft, sc);

#ifdef RND_COM
	rnd_attach_source(&sc->rnd_source, device_xname(sc->sc_dev),
	    RND_TYPE_TTY, RND_FLAG_DEFAULT);
#endif

	SET(sc->sc_hwflags, COM_HW_DEV_OK);
}

static int
wmcom_intr(void *arg)
{
	struct wmcom_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int cc;
	uint32_t data;
	uint8_t fr, intm;
	u_char *put;

	if (!device_is_active(sc->sc_dev))
		return 0;

	fr = bus_space_read_1(iot, ioh, UARTFR);
	intm = bus_space_read_1(iot, ioh, UARTINTM);
	if (bus_space_read_1(iot, ioh, UARTINT) & INT_RXINT) {
		put = sc->sc_rbput;
		cc = sc->sc_rbavail;
		while (cc > 0) {
			if (ISSET(fr, FR_RXFE))
				break;
			data = bus_space_read_4(iot, ioh, UARTDR);
			cn_check_magic(sc->sc_tty->t_dev, data & 0xff,
			    wmcom_cnm_state);

			put[0] = data & 0xff;
			put[1] = (data >> 8) & 0xff;
			put += 2;
			if (put >= sc->sc_rbuf + (WMCOM_RING_SIZE << 1))
				put = sc->sc_rbuf;
			cc--;
			sc->sc_rx_ready = 1;

			fr = bus_space_read_1(iot, ioh, UARTFR);
		}

		/*
		 * Current string of incoming characters ended because
		 * no more data was available or we ran out of space.
		 * Schedule a receive event if any data was received.
		 * If we're out of space, turn off receive interrupts.
		 */
		sc->sc_rbput = put;
		sc->sc_rbavail = cc;

		/*
		 * See if we are in danger of overflowing a buffer. If
		 * so, use hardware flow control to ease the pressure.
		 */

		/* but wmcom cannot. X-( */

		/*
		 * If we're out of space, disable receive interrupts
		 * until the queue has drained a bit.
		 */
		if (cc <= 0)
			CLR(intm, INT_RXINT);
	}

	/*
	 * Done handling any receive interrupts. See if data can be
	 * transmitted as well. Schedule tx done event if no data left
	 * and tty was marked busy.
	 */

	if (!ISSET(fr, FR_TXFF)) {
		/* Output the next chunk of the contiguous buffer, if any. */
		if (sc->sc_tbc > 0) {
			while (sc->sc_tbc > 0 && !ISSET(fr, FR_TXFF)) {
				bus_space_write_1(iot, ioh, UARTDR,
				    *sc->sc_tba);
				sc->sc_tba++;
				sc->sc_tbc--;
				fr = bus_space_read_1(iot, ioh, UARTFR);
			}
		} else if (!ISSET(fr, FR_BUSY) && ISSET(intm, INT_TXINT)) {
			CLR(intm, INT_TXINT);
			sc->sc_tx_done = 1;
		}
	}

	bus_space_write_1(iot, ioh, UARTINTM, intm);

	/* Wake up the poller. */
	softint_schedule(sc->sc_si);

	return 1;
}

static void
wmcom_soft(void *arg)
{
	struct wmcom_softc *sc = arg;
	struct tty *tp = sc->sc_tty;

	if (!device_is_active(sc->sc_dev))
		return;

	if (sc->sc_rx_ready) {
		sc->sc_rx_ready = 0;
		wmcom_rxsoft(sc, tp);
	}
	if (sc->sc_tx_done) {
		sc->sc_tx_done = 0;
		CLR(tp->t_state, TS_BUSY);
		if (ISSET(tp->t_state, TS_FLUSH))
			CLR(tp->t_state, TS_FLUSH);
		else
			ndflush(&tp->t_outq,
			    (int)(sc->sc_tba - tp->t_outq.c_cf));
		(*tp->t_linesw->l_start)(tp);
	}
}

static void
wmcom_start(struct tty *tp)
{
	struct wmcom_softc *sc
		= device_lookup_private(&wmcom_cd, COMUNIT(tp->t_dev));
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int s, n;
	uint8_t intm;

	if (!device_is_active(sc->sc_dev))
		return;

	s = spltty();
	if (ISSET(tp->t_state, TS_BUSY | TS_TIMEOUT | TS_TTSTOP))
		goto out;
	if (!ttypull(tp))
		goto out;

	/* Grab the first contiguous region of buffer space. */
	{
		u_char *tba;
		int tbc;

		tba = tp->t_outq.c_cf;
		tbc = ndqb(&tp->t_outq, 0);

		(void)splserial();

		sc->sc_tba = tba;
		sc->sc_tbc = tbc;
	}

	SET(tp->t_state, TS_BUSY);

	intm = bus_space_read_1(iot, ioh, UARTINTM);
	if (!ISSET(intm, INT_TXINT)) {
		bus_space_write_1(iot, ioh, UARTINTM, intm | INT_TXINT);

		/* Output the first chunk of the contiguous buffer. */
		n = min(sc->sc_tbc, UART_FIFO_SIZE);
		bus_space_write_multi_1(iot, ioh, UARTDR, sc->sc_tba, n);
		sc->sc_tba += n;
		sc->sc_tbc -= n;
	}
out:
	splx(s);
	return;
}

static int
wmcom_param(struct tty *tp, struct termios *t)
{
	struct wmcom_softc *sc =
	    device_lookup_private(&wmcom_cd, COMUNIT(tp->t_dev));
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int s;

	if (!device_is_active(sc->sc_dev))
		return ENXIO;
	if (t->c_ispeed && t->c_ispeed != t->c_ospeed)
		return EINVAL;

	/*
	 * For the console, always force CLOCAL and !HUPCL, so that the port
	 * is always active.
	 */
	if (ISSET(sc->sc_swflags, TIOCFLAG_SOFTCAR) ||
	    ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
		SET(t->c_cflag, CLOCAL);
		CLR(t->c_cflag, HUPCL);
	}

	/*
	 * If there were no changes, don't do anything.  This avoids dropping
	 * input and improves performance when all we did was frob things like
	 * VMIN and VTIME.
	 */
	if (tp->t_ospeed == t->c_ospeed &&
	    tp->t_cflag == t->c_cflag)
		return 0;

	s = splserial();
	bus_space_write_4(iot, ioh, UARTLCR, wmcom_rate2lcr(t->c_ospeed));
	bus_space_write_1(iot, ioh, UARTFCR, wmcom_cflag2fcr(t->c_cflag));

	/* And copy to tty. */
	tp->t_ispeed = 0;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	splx(s);

	/*
	 * Update the tty layer's idea of the carrier bit.
	 * We tell tty the carrier is always on.
	 */
	(*tp->t_linesw->l_modem)(tp, 1);

	return 0;
}

static int
wmcom_hwiflow(struct tty *tp, int block)
{
	/* Nothing */
	return 0;
}

/* ARGSUSED */
int
wmcomopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct wmcom_softc *sc;
	struct tty *tp;
	int error, s, s2;
	uint8_t con;

	sc = device_lookup_private(&wmcom_cd, COMUNIT(dev));
	if (sc == NULL || !ISSET(sc->sc_hwflags, COM_HW_DEV_OK))
		return ENXIO;
	if (!device_is_active(sc->sc_dev))
		return ENXIO;

#ifdef KGDB
	/*
	 * If this is the kgdb port, no other use is permitted.
	 */
	if (ISSET(sc->sc_hwflags, COM_HW_KGDB))
		return EBUSY;
#endif

	tp = sc->sc_tty;

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return EBUSY;

	s = spltty();

	/*
	 * Do the following iff this is a first open.
	 */
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		struct termios t;

		tp->t_dev = dev;

		/* Enable and turn on interrupt */
		con = CON_UARTEN;
		if (ISSET(sc->sc_flags, WMCOM_IRDA))
			con |= CON_IRTXM;
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, UARTCON, con);
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, UARTINTM, INT_RXINT);

		/*
		 * Initialize the termios status to the defaults.  Add in the
		 * sticky bits from TIOCSFLAGS.
		 */
		t.c_ispeed = 0;
		if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
			t.c_ospeed = wmcom_cnrate;
			t.c_cflag = wmcom_cncflag;
		} else {
			t.c_ospeed = TTYDEF_SPEED;
			t.c_cflag = TTYDEF_CFLAG;
		}
		if (ISSET(sc->sc_swflags, TIOCFLAG_CLOCAL))
			SET(t.c_cflag, CLOCAL);
		if (ISSET(sc->sc_swflags, TIOCFLAG_CRTSCTS))
			SET(t.c_cflag, CRTSCTS);
		if (ISSET(sc->sc_swflags, TIOCFLAG_MDMBUF))
			SET(t.c_cflag, MDMBUF);
		/* Make sure wmcom_param() we do something */
		tp->t_ospeed = 0;
		wmcom_param(tp, &t);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		ttychars(tp);
		ttsetwater(tp);

		s2 = splserial();

		/* Clear the input ring. */
		sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
		sc->sc_rbavail = WMCOM_RING_SIZE;
		wmcom_iflush(sc);

		splx(s2);
	}

	splx(s);

	error = ttyopen(tp, COMDIALOUT(dev), ISSET(flag, O_NONBLOCK));
	if (error)
		goto bad;

	error = (*tp->t_linesw->l_open)(dev, tp);
	if (error)
		goto bad;
	return 0;

bad:
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * We failed to open the device, and nobody else had it opened.
		 * Clean up the state as appropriate.
		 */
		wmcom_shutdown(sc);

		/* Disable UART */
		if (!ISSET(sc->sc_hwflags, COM_HW_CONSOLE))
			bus_space_write_1(sc->sc_iot, sc->sc_ioh, UARTCON, 0);
	}

	return error;
}

/* ARGSUSED */
int
wmcomclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct wmcom_softc *sc = device_lookup_private(&wmcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	/* XXXX This is for cons.c. */
	if (!ISSET(tp->t_state, TS_ISOPEN))
		return 0;

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);

	if (!device_is_active(sc->sc_dev))
		return 0;

	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * Although we got a last close, the device may still be in
		 * use; e.g. if this was the dialout node, and there are still
		 * processes waiting for carrier on the non-dialout node.
		 */
		wmcom_shutdown(sc);

		/* Disable UART */
		if (!ISSET(sc->sc_hwflags, COM_HW_CONSOLE))
			bus_space_write_1(sc->sc_iot, sc->sc_ioh, UARTCON, 0);
	}

	return 0;
}

int
wmcomread(dev_t dev, struct uio *uio, int flag)
{
	struct wmcom_softc *sc = device_lookup_private(&wmcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (!device_is_active(sc->sc_dev))
		return EIO;

	return (*tp->t_linesw->l_read)(tp, uio, flag);
}

int
wmcomwrite(dev_t dev, struct uio *uio, int flag)
{
	struct wmcom_softc *sc = device_lookup_private(&wmcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (!device_is_active(sc->sc_dev))
		return EIO;

	return (*tp->t_linesw->l_write)(tp, uio, flag);
}

int
wmcomioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct wmcom_softc *sc = device_lookup_private(&wmcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;
	int error, s;

	if (!device_is_active(sc->sc_dev))
		return EIO;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	error = ttioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	switch (cmd) {
	case TIOCSFLAGS:
		error = kauth_authorize_device_tty(l->l_cred,
		    KAUTH_DEVICE_TTY_PRIVSET, tp);
		break;
	default:
		break;
	}
	if (error)
		return error;

	s = splserial();
	error = 0;
	switch (cmd) {
	case TIOCSBRK:
		wmcom_break(sc, 1);
		break;

	case TIOCCBRK:
		wmcom_break(sc, 0);
		break;

	case TIOCGFLAGS:
		*(int *)data = sc->sc_swflags;
		break;

	case TIOCSFLAGS:
		sc->sc_swflags = *(int *)data;
		break;

	default:
		error = EPASSTHROUGH;
		break;
	}
	splx(s);
	return error;
}

int
wmcompoll(dev_t dev, int events, struct lwp *l)
{
	struct wmcom_softc *sc = device_lookup_private(&wmcom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (!device_is_active(sc->sc_dev))
		return EIO;

	return (*tp->t_linesw->l_poll)(tp, events, l);
}

struct tty *
wmcomtty(dev_t dev)
{
	struct wmcom_softc *sc = device_lookup_private(&wmcom_cd, COMUNIT(dev));

	return sc->sc_tty;
}

void
wmcomstop(struct tty *tp, int flag)
{
	int s;

	s = splserial();
	if (ISSET(tp->t_state, TS_BUSY)) {
		/* Stop transmitting at the next chunk. */
		if (!ISSET(tp->t_state, TS_TTSTOP))
			SET(tp->t_state, TS_FLUSH);
	}
	splx(s);
}


static void
wmcom_iflush(struct wmcom_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int timo;

	timo = 50000;
	while ((bus_space_read_1(iot, ioh, UARTFR) & FR_RXFE) == 0 &&
	    timo--)
		bus_space_read_1(iot, ioh, UARTDR);
	if (timo == 0)
		printf("%s: iflush timeout\n", device_xname(sc->sc_dev));
}

static void
wmcom_shutdown(struct wmcom_softc *sc)
{
	int s;

	s = splserial();

	/* Turn off interrupt */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, UARTINTM, 0);

	/* Clear any break condition set with TIOCSBRK. */
	wmcom_break(sc, 0);

	splx(s);
}

static void
wmcom_break(struct wmcom_softc *sc, int onoff)
{
	int s;
	uint8_t fcr;

	s = splserial();
	fcr = bus_space_read_1(sc->sc_iot, sc->sc_ioh, UARTFCR);
	if (onoff)
		SET(fcr, FCR_BREAK);
	else
		CLR(fcr, FCR_BREAK);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, UARTFCR, fcr);
	splx(s);
}

static void
wmcom_rxsoft(struct wmcom_softc *sc, struct tty *tp)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int code, s;
	u_int cc, scc;
	uint8_t intm;
	u_char sts, *get;

	get = sc->sc_rbget;
	scc = cc = WMCOM_RING_SIZE - sc->sc_rbavail;
	while (cc) {
		code = get[0];
		sts = get[1];
		if (ISSET(sts, RSR_FE | RSR_PE | RSR_OE)) {
			if (ISSET(sts, (RSR_FE)))
				SET(code, TTY_FE);
			if (ISSET(sts, RSR_PE))
				SET(code, TTY_PE);
			if (ISSET(sts, RSR_OE))
				;		/* XXXXX: Overrun */
		}
		if ((*tp->t_linesw->l_rint)(code, tp) == -1) {
			/*
			 * The line discipline's buffer is out of space.
			 */
			/*
			 * We're either not using flow control, or the
			 * line discipline didn't tell us to block for
			 * some reason.  Either way, we have no way to
			 * know when there's more space available, so
			 * just drop the rest of the data.
			 */
			get += cc << 1;
			if (get >= sc->sc_rbuf + (WMCOM_RING_SIZE << 1))
				get -= (WMCOM_RING_SIZE << 1);
			cc = 0;
			break;
		}
		get += 2;
		if (get >= sc->sc_rbuf + (WMCOM_RING_SIZE << 1))
			get = sc->sc_rbuf;
		cc--;
	}

	if (cc != scc) {
		sc->sc_rbget = get;
		s = splserial();
		
		cc = sc->sc_rbavail += scc - cc;
		/* Buffers should be ok again, release possible block. */
		if (cc >= 1) {
			intm = bus_space_read_1(iot, ioh, UARTINTM);
			SET(intm, INT_RXINT);
			bus_space_write_1(iot, ioh, UARTINTM, intm);
		}
		splx(s);
	}
}

static inline uint32_t
wmcom_rate2lcr(int rate)
{

	return 7372800 / (16 * rate) - 1;
}

static uint8_t
wmcom_cflag2fcr(tcflag_t cflag)
{
	int8_t fcr = FCR_UFIFOEN;

	switch (cflag & CSIZE) {
	case CS5: SET(fcr, FCR_WLEN_5); break;
	case CS6: SET(fcr, FCR_WLEN_6); break;
	case CS7: SET(fcr, FCR_WLEN_7); break;
	case CS8: SET(fcr, FCR_WLEN_8); break;
	default:  SET(fcr, FCR_WLEN_8); break;
	}
	if (cflag & CSTOPB)
		SET(fcr, FCR_XSTOP);
	if (cflag & PARENB) {
		SET(fcr, (FCR_PRTEN | FCR_EVENPRT));
		if (cflag & PARODD)
			CLR(fcr, FCR_EVENPRT);
	}
	return fcr;
}

#define WMCOM_CNREAD_1(offset) \
	(*(volatile uint8_t *)(wmcom_cnaddr + ((offset) << 2)))
#define WMCOM_CNREAD_4(offset) \
	(*(volatile uint32_t *)(wmcom_cnaddr + ((offset) << 2)))
#define WMCOM_CNWRITE_1(offset, val) \
	(*(volatile uint8_t *)(wmcom_cnaddr + ((offset) << 2)) = val)
#define WMCOM_CNWRITE_4(offset, val) \
	(*(volatile uint32_t *)(wmcom_cnaddr + ((offset) << 2)) = val)

static struct consdev wmcomcons = {
	NULL, NULL, wmcom_cngetc, wmcom_cnputc, wmcom_cnpollc, NULL, NULL, NULL,
	NODEV, CN_NORMAL
};

int
wmcom_cnattach(vaddr_t addr, int rate, tcflag_t cflag, int irda)
{

	wmcom_cnaddr = addr;
	wmcom_cnrate = rate;
	wmcom_cncflag = cflag;
	WMCOM_CNWRITE_4(UARTLCR, wmcom_rate2lcr(rate));
	WMCOM_CNWRITE_1(UARTFCR, wmcom_cflag2fcr(cflag));
	if (irda)
		WMCOM_CNWRITE_1(UARTCON, CON_UARTEN | CON_IRTXM);
	else
		WMCOM_CNWRITE_1(UARTCON, CON_UARTEN);

	cn_tab = &wmcomcons;
	cn_init_magic(&wmcom_cnm_state);
	cn_set_magic("\047\001");	/* default magic is BREAK */

	return 0;
}

/* ARGSUSED */
static int
wmcom_cngetc(dev_t dev)
{
	int s = splserial();
	char ch;

	while (WMCOM_CNREAD_1(UARTFR) & FR_RXFE);

	ch = WMCOM_CNREAD_4(UARTDR);

	{
#ifdef DDB
		extern int db_active;
		if (!db_active)
#endif
			cn_check_magic(dev, ch, wmcom_cnm_state);
	}

	splx(s);
	return ch;
}

/* ARGSUSED */
static void
wmcom_cnputc(dev_t dev, int c)
{
	int s = splserial();

	while (WMCOM_CNREAD_1(UARTFR) & FR_TXFF);

	WMCOM_CNWRITE_1(UARTDR, c);

	/* Make sure output. */
	while (WMCOM_CNREAD_1(UARTFR) & FR_BUSY);

	splx(s);
}

/* ARGSUSED */
static void
wmcom_cnpollc(dev_t dev, int on)
{
	/* Nothing */
}

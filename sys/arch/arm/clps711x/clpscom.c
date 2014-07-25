/*      $NetBSD: clpscom.c,v 1.3 2014/07/25 08:10:32 dholland Exp $      */
/*
 * Copyright (c) 2013 KIYOHARA Takashi
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
__KERNEL_RCSID(0, "$NetBSD: clpscom.c,v 1.3 2014/07/25 08:10:32 dholland Exp $");

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

#include <arm/clps711x/clps711xreg.h>
#include <arm/clps711x/clpssocvar.h>

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

#define CLPSCOM_RING_SIZE	2048
#define UART_FIFO_SIZE		16

#define CLPSCOM_READ_CON(sc) \
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, PS711X_SYSCON)
#define CLPSCOM_WRITE_CON(sc, val) \
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, PS711X_SYSCON, (val))
#define CLPSCOM_READ_FLG(sc) \
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, PS711X_SYSFLG)
#define CLPSCOM_WRITE_FLG(sc, val) \
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, PS711X_SYSFLG, (val))
#define CLPSCOM_READ(sc) \
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, PS711X_UARTDR)
#define CLPSCOM_WRITE(sc, val) \
	bus_space_write_1((sc)->sc_iot, (sc)->sc_ioh, PS711X_UARTDR, (val))
#define CLPSCOM_WRITE_MULTI(sc, val, n) \
  bus_space_write_multi_1((sc)->sc_iot, (sc)->sc_ioh, PS711X_UARTDR, (val), (n))
#define CLPSCOM_READ_UBRLCR(sc) \
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, PS711X_UBRLCR)
#define CLPSCOM_WRITE_UBRLCR(sc, val) \
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, PS711X_UBRLCR, (val))

struct clpscom_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	int sc_irq[3];
	void *sc_ih[3];
#define CLPSCOM_TXINT	0
#define CLPSCOM_RXINT	1
#define CLPSCOM_MSINT	2

	void *sc_si;

	struct tty *sc_tty;

	u_char *sc_tba;
	u_int sc_tbc;
	u_char *sc_rbuf;
	char *volatile sc_rbget;
	char *volatile sc_rbput;
	volatile int sc_rbavail;

#define CLPSCOM_MODEM_STATUS_MASK (SYSFLG_DCD | SYSFLG_DSR | SYSFLG_CTS)
	uint32_t sc_ms;
	uint32_t sc_ms_dcd;
	uint32_t sc_ms_cts;
	uint32_t sc_ms_mask;
	uint32_t sc_ms_delta;

	int sc_tx_stopped;

	int sc_tx_done;
	int sc_rx_ready;
	int sc_ms_changed;

	int sc_hwflags;
#define COM_HW_CONSOLE	(1 << 0)
#define COM_HW_DEV_OK	(1 << 1)
#define COM_HW_KGDB	(1 << 2)
	int sc_swflags;

#ifdef RND_COM
	krandsource_t rnd_source;
#endif
};

static int clpscom_match(device_t, cfdata_t, void *);
static void clpscom_attach(device_t, device_t, void *);

static int clpscom_txintr(void *);
static int clpscom_rxintr(void *);
static int clpscom_msintr(void *);
static void clpscom_soft(void *);

static void clpscom_start(struct tty *);
static int clpscom_param(struct tty *, struct termios *);
static int clpscom_hwiflow(struct tty *, int);

dev_type_open(clpscomopen);
dev_type_close(clpscomclose);
dev_type_read(clpscomread);
dev_type_write(clpscomwrite);
dev_type_ioctl(clpscomioctl);
dev_type_stop(clpscomstop);
dev_type_tty(clpscomtty);
dev_type_poll(clpscompoll);

static void clpscom_iflush(struct clpscom_softc *);
static void clpscom_shutdown(struct clpscom_softc *);
static void clpscom_break(struct clpscom_softc *, int);
static int clpscom_to_tiocm(struct clpscom_softc *);

static void clpscom_rxsoft(struct clpscom_softc *, struct tty *);
static void clpscom_mssoft(struct clpscom_softc *, struct tty *);

static inline uint32_t clpscom_rate2ubrlcr(int);
static uint32_t clpscom_cflag2ubrlcr(tcflag_t);

static int clpscom_cngetc(dev_t);
static void clpscom_cnputc(dev_t, int);
static void clpscom_cnpollc(dev_t, int);

CFATTACH_DECL_NEW(clpscom, sizeof(struct clpscom_softc),
    clpscom_match, clpscom_attach, NULL, NULL);

const struct cdevsw clpscom_cdevsw = {
	.d_open = clpscomopen,
	.d_close = clpscomclose,
	.d_read = clpscomread,
	.d_write = clpscomwrite,
	.d_ioctl = clpscomioctl,
	.d_stop = clpscomstop,
	.d_tty = clpscomtty,
	.d_poll = clpscompoll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

static struct cnm_state clpscom_cnm_state;
static vaddr_t clpscom_cnaddr = 0;
static int clpscom_cnrate;
static tcflag_t clpscom_cncflag;


/* ARGSUSED */
static int
clpscom_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

/* ARGSUSED */
static void
clpscom_attach(device_t parent, device_t self, void *aux)
{
	struct clpscom_softc *sc = device_private(self);
	struct clpssoc_attach_args *aa = aux;
	int i;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	sc->sc_ioh = *aa->aa_ioh;
	for (i = 0; i < __arraycount(aa->aa_irq); i++) {
		sc->sc_irq[i] = aa->aa_irq[i];
		sc->sc_ih[i] = NULL;
	}

	if (clpscom_cnaddr != 0)
		SET(sc->sc_hwflags, COM_HW_CONSOLE);

	sc->sc_tty = tty_alloc();
	sc->sc_tty->t_oproc = clpscom_start;
	sc->sc_tty->t_param = clpscom_param;
	sc->sc_tty->t_hwiflow = clpscom_hwiflow;

	sc->sc_tbc = 0;
	sc->sc_rbuf = malloc(CLPSCOM_RING_SIZE << 1, M_DEVBUF, M_NOWAIT);
	if (sc->sc_rbuf == NULL) {
		aprint_error_dev(self, "unable to allocate ring buffer\n");
		return;
	}
	sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
	sc->sc_rbavail = CLPSCOM_RING_SIZE;

	tty_attach(sc->sc_tty);

	if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
		int maj = cdevsw_lookup_major(&clpscom_cdevsw);

		sc->sc_tty->t_dev = makedev(maj, device_unit(sc->sc_dev));
		cn_tab->cn_dev = sc->sc_tty->t_dev;

		aprint_normal_dev(self, "console\n");
	}

	sc->sc_si = softint_establish(SOFTINT_SERIAL, clpscom_soft, sc);

#ifdef RND_COM
	rnd_attach_source(&sc->rnd_source, device_xname(sc->sc_dev),
	    RND_TYPE_TTY, 0);
#endif

	SET(sc->sc_hwflags, COM_HW_DEV_OK);
}

static int
clpscom_txintr(void *arg)
{
	struct clpscom_softc *sc = arg;
	uint32_t sysflg;

	if (!device_is_active(sc->sc_dev))
		return 0;

	sysflg = CLPSCOM_READ_FLG(sc);

	/*
	 * Done handling any receive interrupts. See if data can be
	 * transmitted as well. Schedule tx done event if no data left
	 * and tty was marked busy.
	 */

	if (!ISSET(sysflg, SYSFLG_UTXFF)) {
		/* Output the next chunk of the contiguous buffer, if any. */
		if (sc->sc_tbc > 0) {
			while (sc->sc_tbc > 0 && !ISSET(sysflg, SYSFLG_UTXFF)) {
				CLPSCOM_WRITE(sc, *sc->sc_tba);
				sc->sc_tba++;
				sc->sc_tbc--;
				sysflg = CLPSCOM_READ_FLG(sc);
			}
		} else if (!ISSET(sysflg, SYSFLG_UBUSY) &&
					sc->sc_ih[CLPSCOM_TXINT] != NULL) {
			intr_disestablish(sc->sc_ih[CLPSCOM_TXINT]);
			sc->sc_ih[CLPSCOM_TXINT] = NULL;
			sc->sc_tx_done = 1;
		}
	}

	/* Wake up the poller. */
	softint_schedule(sc->sc_si);

	return 1;
}

static int
clpscom_rxintr(void *arg)
{
	struct clpscom_softc *sc = arg;
	int cc;
	uint32_t sysflg;
	uint16_t data;
	u_char *put;

	if (!device_is_active(sc->sc_dev))
		return 0;

	if (sc->sc_ih[CLPSCOM_RXINT] != NULL) {
		put = sc->sc_rbput;
		cc = sc->sc_rbavail;
		while (cc > 0) {
			sysflg = CLPSCOM_READ_FLG(sc);
			if (ISSET(sysflg, SYSFLG_URXFE))
				break;
			data = CLPSCOM_READ(sc);
			cn_check_magic(sc->sc_tty->t_dev, data & 0xff,
			    clpscom_cnm_state);

			put[0] = data & 0xff;
			put[1] = (data >> 8) & 0xff;
			put += 2;
			if (put >= sc->sc_rbuf + (CLPSCOM_RING_SIZE << 1))
				put = sc->sc_rbuf;
			cc--;
			sc->sc_rx_ready = 1;
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

		/* but clpscom cannot. X-( */

		/*
		 * If we're out of space, disable receive interrupts
		 * until the queue has drained a bit.
		 */
		if (cc <= 0) {
			intr_disestablish(sc->sc_ih[CLPSCOM_RXINT]);
			sc->sc_ih[CLPSCOM_RXINT] = NULL;
		}
	}

	/* Wake up the poller. */
	softint_schedule(sc->sc_si);

	return 1;
}

static int
clpscom_msintr(void *arg)
{
	struct clpscom_softc *sc = arg;
	uint32_t ms, delta;

	if (!device_is_active(sc->sc_dev))
		return 0;

	if (sc->sc_ih[CLPSCOM_MSINT] != NULL) {
		ms = CLPSCOM_READ_FLG(sc) & CLPSCOM_MODEM_STATUS_MASK;
		delta = ms ^ sc->sc_ms;
		sc->sc_ms = ms;

		if (ISSET(delta, sc->sc_ms_mask)) {
			SET(sc->sc_ms_delta, delta);

			/*
			 * Stop output immediately if we lose the output
			 * flow control signal or carrier detect.
			 */
			if (ISSET(~ms, sc->sc_ms_mask))
				sc->sc_tbc = 0;
			sc->sc_ms_changed = 1;
		}
	}
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, PS711X_UMSEOI, 1);

	/* Wake up the poller. */
	softint_schedule(sc->sc_si);

	return 1;
}

static void
clpscom_soft(void *arg)
{
	struct clpscom_softc *sc = arg;
	struct tty *tp = sc->sc_tty;

	if (!device_is_active(sc->sc_dev))
		return;

	if (sc->sc_rx_ready) {
		sc->sc_rx_ready = 0;
		clpscom_rxsoft(sc, tp);
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
	if (sc->sc_ms_changed == 1) {
		sc->sc_ms_changed = 0;
		clpscom_mssoft(sc, tp);
	}
}

static void
clpscom_start(struct tty *tp)
{
	struct clpscom_softc *sc
		= device_lookup_private(&clpscom_cd, COMUNIT(tp->t_dev));
	int s, n;

	if (!device_is_active(sc->sc_dev))
		return;

	s = spltty();
	if (ISSET(tp->t_state, TS_BUSY | TS_TIMEOUT | TS_TTSTOP))
		goto out;
	if (sc->sc_tx_stopped)
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

	if (sc->sc_ih[CLPSCOM_TXINT] == NULL) {
		sc->sc_ih[CLPSCOM_TXINT] =
		    intr_establish(sc->sc_irq[CLPSCOM_TXINT], IPL_SERIAL, 0,
		    clpscom_txintr, sc);
		if (sc->sc_ih[CLPSCOM_TXINT] == NULL)
			printf("%s: can't establish tx interrupt\n",
			    device_xname(sc->sc_dev));

		/* Output the first chunk of the contiguous buffer. */
		n = min(sc->sc_tbc, UART_FIFO_SIZE);
		CLPSCOM_WRITE_MULTI(sc, sc->sc_tba, n);
		sc->sc_tba += n;
		sc->sc_tbc -= n;
	}
out:
	splx(s);
	return;
}

static int
clpscom_param(struct tty *tp, struct termios *t)
{
	struct clpscom_softc *sc =
	    device_lookup_private(&clpscom_cd, COMUNIT(tp->t_dev));
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

	/*
	 * If we're not in a mode that assumes a connection is present, then
	 * ignore carrier changes.
	 */
	if (ISSET(t->c_cflag, CLOCAL | MDMBUF))
		sc->sc_ms_dcd = 0;
	else
		sc->sc_ms_dcd = SYSFLG_DCD;
	/*
	 * Set the flow control pins depending on the current flow control
	 * mode.
	 */
	if (ISSET(t->c_cflag, CRTSCTS)) {
		sc->sc_ms_cts = SYSFLG_CTS;
	} else if (ISSET(t->c_cflag, MDMBUF)) {
		/*
		 * For DTR/DCD flow control, make sure we don't toggle DTR for
		 * carrier detection.
		 */
		sc->sc_ms_cts = SYSFLG_DCD;
	} else {
		/*
		 * If no flow control, then always set RTS.  This will make
		 * the other side happy if it mistakenly thinks we're doing
		 * RTS/CTS flow control.
		 */
		sc->sc_ms_cts = 0;
	}
	sc->sc_ms_mask = sc->sc_ms_cts | sc->sc_ms_dcd;

	s = splserial();
	CLPSCOM_WRITE_UBRLCR(sc,
	    UBRLCR_FIFOEN |
	    clpscom_rate2ubrlcr(t->c_ospeed) |
	    clpscom_cflag2ubrlcr(t->c_cflag));

	/* And copy to tty. */
	tp->t_ispeed = 0;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	splx(s);

	/*
	 * Update the tty layer's idea of the carrier bit, in case we changed
	 * CLOCAL or MDMBUF.  We don't hang up here; we only do that by
	 * explicit request.
	 */
	(*tp->t_linesw->l_modem)(tp, ISSET(sc->sc_ms, SYSFLG_DCD));

	if (!ISSET(t->c_cflag, CHWFLOW))
		if (sc->sc_tx_stopped) {
			sc->sc_tx_stopped = 0;
			clpscom_start(tp);
		}

	return 0;
}

static int
clpscom_hwiflow(struct tty *tp, int block)
{
	/* Nothing */
	return 0;
}

/* ARGSUSED */
int
clpscomopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct clpscom_softc *sc;
	struct tty *tp;
	int error, s, s2;

	sc = device_lookup_private(&clpscom_cd, COMUNIT(dev));
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
		CLPSCOM_WRITE_CON(sc, CLPSCOM_READ_CON(sc) | SYSCON_UARTEN);

		/* Fetch the current modem control status, needed later. */
		sc->sc_ms = CLPSCOM_READ_FLG(sc) & CLPSCOM_MODEM_STATUS_MASK;

		/*
		 * Initialize the termios status to the defaults.  Add in the
		 * sticky bits from TIOCSFLAGS.
		 */
		t.c_ispeed = 0;
		if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
			t.c_ospeed = clpscom_cnrate;
			t.c_cflag = clpscom_cncflag;
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
		/* Make sure pscom_param() we do something */
		tp->t_ospeed = 0;
		clpscom_param(tp, &t);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		ttychars(tp);
		ttsetwater(tp);

		s2 = splserial();

		/* Clear the input ring. */
		sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
		sc->sc_rbavail = CLPSCOM_RING_SIZE;
		clpscom_iflush(sc);

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
		clpscom_shutdown(sc);

		/* Disable UART */
		if (!ISSET(sc->sc_hwflags, COM_HW_CONSOLE))
			CLPSCOM_WRITE_CON(sc,
			    CLPSCOM_READ_CON(sc) & ~SYSCON_UARTEN);
	}

	return error;
}

/* ARGSUSED */
int
clpscomclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct clpscom_softc *sc =
	    device_lookup_private(&clpscom_cd, COMUNIT(dev));
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
		clpscom_shutdown(sc);

		/* Disable UART */
		if (!ISSET(sc->sc_hwflags, COM_HW_CONSOLE))
			CLPSCOM_WRITE_CON(sc,
			    CLPSCOM_READ_CON(sc) & ~SYSCON_UARTEN);
	}

	return 0;
}

int
clpscomread(dev_t dev, struct uio *uio, int flag)
{
	struct clpscom_softc *sc =
	    device_lookup_private(&clpscom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (!device_is_active(sc->sc_dev))
		return EIO;

	return (*tp->t_linesw->l_read)(tp, uio, flag);
}

int
clpscomwrite(dev_t dev, struct uio *uio, int flag)
{
	struct clpscom_softc *sc =
	    device_lookup_private(&clpscom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (!device_is_active(sc->sc_dev))
		return EIO;

	return (*tp->t_linesw->l_write)(tp, uio, flag);
}

int
clpscomioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct clpscom_softc *sc =
	    device_lookup_private(&clpscom_cd, COMUNIT(dev));
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
		clpscom_break(sc, 1);
		break;

	case TIOCCBRK:
		clpscom_break(sc, 0);
		break;

	case TIOCGFLAGS:
		*(int *)data = sc->sc_swflags;
		break;

	case TIOCSFLAGS:
		sc->sc_swflags = *(int *)data;
		break;

	case TIOCMGET:
		*(int *)data = clpscom_to_tiocm(sc);
		break;

	default:
		error = EPASSTHROUGH;
		break;
	}
	splx(s);
	return error;
}

int
clpscompoll(dev_t dev, int events, struct lwp *l)
{
	struct clpscom_softc *sc =
	    device_lookup_private(&clpscom_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (!device_is_active(sc->sc_dev))
		return EIO;

	return (*tp->t_linesw->l_poll)(tp, events, l);
}

struct tty *
clpscomtty(dev_t dev)
{
	struct clpscom_softc *sc =
	    device_lookup_private(&clpscom_cd, COMUNIT(dev));

	return sc->sc_tty;
}

void
clpscomstop(struct tty *tp, int flag)
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
clpscom_iflush(struct clpscom_softc *sc)
{
	int timo;

	timo = 50000;
	while ((CLPSCOM_READ_FLG(sc) & SYSFLG_URXFE) == 0
	    && timo--)
		CLPSCOM_READ(sc);
	if (timo == 0)
		printf("%s: iflush timeout\n", device_xname(sc->sc_dev));
}

static void
clpscom_shutdown(struct clpscom_softc *sc)
{
	int s;

	s = splserial();

	/* Turn off all interrupts */
	if (sc->sc_ih[CLPSCOM_TXINT] != NULL)
		intr_disestablish(sc->sc_ih[CLPSCOM_TXINT]);
	sc->sc_ih[CLPSCOM_TXINT] = NULL;
	if (sc->sc_ih[CLPSCOM_RXINT] != NULL)
		intr_disestablish(sc->sc_ih[CLPSCOM_RXINT]);
	sc->sc_ih[CLPSCOM_RXINT] = NULL;
	if (sc->sc_ih[CLPSCOM_MSINT] != NULL)
		intr_disestablish(sc->sc_ih[CLPSCOM_MSINT]);
	sc->sc_ih[CLPSCOM_MSINT] = NULL;

	/* Clear any break condition set with TIOCSBRK. */
	clpscom_break(sc, 0);

	splx(s);
}

static void
clpscom_break(struct clpscom_softc *sc, int onoff)
{
	int s;
	uint8_t ubrlcr;

	s = splserial();
	ubrlcr = CLPSCOM_READ_UBRLCR(sc);
	if (onoff)
		SET(ubrlcr, UBRLCR_BREAK);
	else
		CLR(ubrlcr, UBRLCR_BREAK);
	CLPSCOM_WRITE_UBRLCR(sc, ubrlcr);
	splx(s);
}

static int
clpscom_to_tiocm(struct clpscom_softc *sc)
{
	uint32_t combits;
	int ttybits = 0;

	combits = sc->sc_ms;
	if (ISSET(combits, SYSFLG_DCD))
		SET(ttybits, TIOCM_CD);
	if (ISSET(combits, SYSFLG_CTS))
		SET(ttybits, TIOCM_CTS);
	if (ISSET(combits, SYSFLG_DSR))
		SET(ttybits, TIOCM_DSR);

	return ttybits;
}

static void
clpscom_rxsoft(struct clpscom_softc *sc, struct tty *tp)
{
	int code, s;
	u_int cc, scc;
	u_char sts, *get;

	get = sc->sc_rbget;
	scc = cc = CLPSCOM_RING_SIZE - sc->sc_rbavail;
	while (cc) {
		code = get[0];
		sts = get[1];
		if (ISSET(sts, UARTDR_FRMERR | UARTDR_PARERR | UARTDR_OVERR)) {
			if (ISSET(sts, (UARTDR_FRMERR)))
				SET(code, TTY_FE);
			if (ISSET(sts, UARTDR_PARERR))
				SET(code, TTY_PE);
			if (ISSET(sts, UARTDR_OVERR))
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
			if (get >= sc->sc_rbuf + (CLPSCOM_RING_SIZE << 1))
				get -= (CLPSCOM_RING_SIZE << 1);
			cc = 0;
			break;
		}
		get += 2;
		if (get >= sc->sc_rbuf + (CLPSCOM_RING_SIZE << 1))
			get = sc->sc_rbuf;
		cc--;
	}

	if (cc != scc) {
		sc->sc_rbget = get;
		s = splserial();
		
		cc = sc->sc_rbavail += scc - cc;
		/* Buffers should be ok again, release possible block. */
		if (cc >= 1) {
			if (sc->sc_ih[CLPSCOM_RXINT] == NULL) {
				sc->sc_ih[CLPSCOM_RXINT] =
				    intr_establish(sc->sc_irq[CLPSCOM_RXINT],
				    IPL_SERIAL, 0, clpscom_rxintr, sc);
				if (sc->sc_ih[CLPSCOM_RXINT] == NULL)
					printf("%s: can't establish"
					    " rx interrupt\n",
					    device_xname(sc->sc_dev));
			}
			if (sc->sc_ih[CLPSCOM_MSINT] == NULL) {
				sc->sc_ih[CLPSCOM_MSINT] =
				    intr_establish(sc->sc_irq[CLPSCOM_MSINT],
				    IPL_SERIAL, 0, clpscom_msintr, sc);
				if (sc->sc_ih[CLPSCOM_MSINT] == NULL)
					printf("%s: can't establish"
					    " ms interrupt\n",
					    device_xname(sc->sc_dev));
			}
		}
		splx(s);
	}
}

static void
clpscom_mssoft(struct clpscom_softc *sc, struct tty *tp)
{
	uint32_t ms, delta;

	ms = sc->sc_ms;
	delta = sc->sc_ms_delta;
	sc->sc_ms_delta = 0;

	if (ISSET(delta, sc->sc_ms_dcd))
		/*
		 * Inform the tty layer that carrier detect changed.
		 */
		(void) (*tp->t_linesw->l_modem)(tp, ISSET(ms, SYSFLG_DCD));

	if (ISSET(delta, sc->sc_ms_cts)) {
		/* Block or unblock output according to flow control. */
		if (ISSET(ms, sc->sc_ms_cts)) {
			sc->sc_tx_stopped = 0;
			(*tp->t_linesw->l_start)(tp);
		} else
			sc->sc_tx_stopped = 1;
	}
}

static inline uint32_t
clpscom_rate2ubrlcr(int rate)
{

	return 230400 / rate - 1;
}

static uint32_t
clpscom_cflag2ubrlcr(tcflag_t cflag)
{
	int32_t ubrlcr = 0;

	switch (cflag & CSIZE) {
	case CS5: SET(ubrlcr, UBRLCR_WRDLEN_5B); break;
	case CS6: SET(ubrlcr, UBRLCR_WRDLEN_6B); break;
	case CS7: SET(ubrlcr, UBRLCR_WRDLEN_7B); break;
	case CS8: SET(ubrlcr, UBRLCR_WRDLEN_8B); break;
	default:  SET(ubrlcr, UBRLCR_WRDLEN_8B); break;
	}
	if (cflag & CSTOPB)
		SET(ubrlcr, UBRLCR_XSTOP);
	if (cflag & PARENB) {
		SET(ubrlcr, (UBRLCR_PRTEN | UBRLCR_EVENPRT));
		if (cflag & PARODD)
			CLR(ubrlcr, UBRLCR_EVENPRT);
	}
	return ubrlcr;
}

#define CLPSCOM_CNREAD() \
	(*(volatile uint32_t *)(clpscom_cnaddr + PS711X_UARTDR))
#define CLPSCOM_CNWRITE(val) \
	(*(volatile uint8_t *)(clpscom_cnaddr + PS711X_UARTDR) = val)
#define CLPSCOM_CNSTATUS() \
	(*(volatile uint32_t *)(clpscom_cnaddr + PS711X_SYSFLG))

static struct consdev clpscomcons = {
	NULL, NULL, clpscom_cngetc, clpscom_cnputc, clpscom_cnpollc,
	NULL, NULL, NULL, NODEV, CN_NORMAL
};

int
clpscom_cnattach(vaddr_t addr, int rate, tcflag_t cflag)
{

	clpscom_cnaddr = addr;
	clpscom_cnrate = rate;
	clpscom_cncflag = cflag;
	*(volatile uint32_t *)(clpscom_cnaddr + PS711X_SYSFLG) |= SYSCON_UARTEN;
	*(volatile uint32_t *)(clpscom_cnaddr + PS711X_UBRLCR) =
	    UBRLCR_FIFOEN |
	    clpscom_cflag2ubrlcr(cflag) |
	    clpscom_rate2ubrlcr(rate);

	cn_tab = &clpscomcons;
	cn_init_magic(&clpscom_cnm_state);
	cn_set_magic("\047\001");	/* default magic is BREAK */

	return 0;
}

/* ARGSUSED */
static int
clpscom_cngetc(dev_t dev)
{
	int s = splserial();
	char ch;

	while (CLPSCOM_CNSTATUS() & SYSFLG_URXFE);

	ch = CLPSCOM_CNREAD();

	{
#ifdef DDB
		extern int db_active;
		if (!db_active)
#endif
			cn_check_magic(dev, ch, clpscom_cnm_state);
	}

	splx(s);
	return ch;
}

/* ARGSUSED */
static void
clpscom_cnputc(dev_t dev, int c)
{
	int s = splserial();

	while (CLPSCOM_CNSTATUS() & SYSFLG_UTXFF);

	CLPSCOM_CNWRITE(c);

	/* Make sure output. */
	while (CLPSCOM_CNSTATUS() & SYSFLG_UBUSY);

	splx(s);
}

/* ARGSUSED */
static void
clpscom_cnpollc(dev_t dev, int on)
{
	/* Nothing */
}

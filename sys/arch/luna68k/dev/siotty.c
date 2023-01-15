/* $NetBSD: siotty.c,v 1.53 2023/01/15 05:08:33 tsutsui Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: siotty.c,v 1.53 2023/01/15 05:08:33 tsutsui Exp $");

#include "opt_ddb.h"
#include "siotty.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/uio.h>
#include <sys/callout.h>
#include <sys/fcntl.h>
#include <dev/cons.h>
#include <sys/kauth.h>
#include <sys/kmem.h>

#include <machine/board.h>
#include <machine/cpu.h>

#include <luna68k/dev/sioreg.h>
#include <luna68k/dev/siovar.h>

#include "ioconf.h"

#define	TIOCM_BREAK 01000 /* non standard use */

static const uint8_t ch0_regs[6] = {
	WR0_RSTINT,				/* reset E/S interrupt */
	WR1_RXALLS | WR1_TXENBL | WR1_ESENBL,	/* Rx per char, Tx, E/S */
	0,					/* */
	WR3_RX8BIT | WR3_RXENBL,		/* Rx */
	WR4_BAUD96 | WR4_STOP1,			/* Tx/Rx */
	WR5_TX8BIT | WR5_TXENBL | WR5_DTR | WR5_RTS, /* Tx */
};

static const struct speedtab siospeedtab[] = {
	{ 2400,	WR4_BAUD24, },
	{ 4800,	WR4_BAUD48, },
	{ 9600,	WR4_BAUD96, },
	{ -1,	0, },
};

struct siotty_rxqdata {
	uint8_t data;
	uint8_t stat;
};

typedef struct siotty_rxqdata rxqdata_t;

struct siotty_softc {
	device_t	sc_dev;
	struct tty	*sc_tty;
	struct sioreg	*sc_ctl;
	u_int		sc_flags;
	uint8_t		sc_wr[6];
	void		*sc_si;		/* software interrupt handler */
	u_int		sc_hwflags;
#define	SIOTTY_HW_CONSOLE	0x0001

	rxqdata_t	*sc_rbuf;
	rxqdata_t	*sc_rbufend;
	rxqdata_t * volatile sc_rbget;
	rxqdata_t * volatile sc_rbput;
	volatile u_int	sc_rbavail;

	uint8_t		*sc_tba;
	u_int		sc_tbc;

	bool		sc_rx_ready;
	bool		sc_tx_busy;
	bool		sc_tx_done;
};

#define	SIOTTY_RING_SIZE	2048
u_int siotty_rbuf_size = SIOTTY_RING_SIZE;

static struct cnm_state	siotty_cnm_state;

static void siostart(struct tty *);
static int  sioparam(struct tty *, struct termios *);
static void siottyintr(void *);
static void siottysoft(void *);
static void siotty_rxsoft(struct siotty_softc *, struct tty *);
static void siotty_txsoft(struct siotty_softc *, struct tty *);
static int  siomctl(struct siotty_softc *, int, int);

static int  siotty_match(device_t, cfdata_t, void *);
static void siotty_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(siotty, sizeof(struct siotty_softc),
    siotty_match, siotty_attach, NULL, NULL);

static dev_type_open(sioopen);
static dev_type_close(sioclose);
static dev_type_read(sioread);
static dev_type_write(siowrite);
static dev_type_ioctl(sioioctl);
static dev_type_stop(siostop);
static dev_type_tty(siotty);
static dev_type_poll(siopoll);

static dev_type_cninit(siottycninit);
static dev_type_cngetc(siottycngetc);
static dev_type_cnputc(siottycnputc);

const struct cdevsw siotty_cdevsw = {
	.d_open = sioopen,
	.d_close = sioclose,
	.d_read = sioread,
	.d_write = siowrite,
	.d_ioctl = sioioctl,
	.d_stop = siostop,
	.d_tty = siotty,
	.d_poll = siopoll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

static int
siotty_match(device_t parent, cfdata_t cf, void *aux)
{
	struct sio_attach_args *args = aux;

	if (args->channel != 0) /* XXX allow tty on Ch.B XXX */
		return 0;
	return 1;
}

static void
siotty_attach(device_t parent, device_t self, void *aux)
{
	struct sio_softc *siosc = device_private(parent);
	struct siotty_softc *sc = device_private(self);
	struct sio_attach_args *args = aux;
	int channel;
	struct tty *tp;

	sc->sc_dev = self;
	channel = args->channel;
	sc->sc_ctl = &siosc->sc_ctl[channel];
	memcpy(sc->sc_wr, ch0_regs, sizeof(ch0_regs));
	siosc->sc_intrhand[channel].ih_func = siottyintr;
	siosc->sc_intrhand[channel].ih_arg = sc;
	if (args->hwflags == 1)
		sc->sc_hwflags |= SIOTTY_HW_CONSOLE;

	if ((sc->sc_hwflags & SIOTTY_HW_CONSOLE) != 0) {
		aprint_normal(" (console)");
		sc->sc_flags = TIOCFLAG_SOFTCAR;
	} else {
		setsioreg(sc->sc_ctl, WR0, WR0_CHANRST);
		setsioreg(&siosc->sc_ctl[0], WR2A, WR2A_VEC86 | WR2A_INTR_1);
		setsioreg(&siosc->sc_ctl[1], WR2B, 0);
		setsioreg(sc->sc_ctl, WR0, sc->sc_wr[WR0]);
		setsioreg(sc->sc_ctl, WR4, sc->sc_wr[WR4]);
		setsioreg(sc->sc_ctl, WR3, sc->sc_wr[WR3]);
		setsioreg(sc->sc_ctl, WR5, sc->sc_wr[WR5]);
		setsioreg(sc->sc_ctl, WR0, sc->sc_wr[WR0]);
	}
	setsioreg(sc->sc_ctl, WR1, sc->sc_wr[WR1]); /* now interrupt driven */

	aprint_normal("\n");

	sc->sc_rbuf = kmem_alloc(siotty_rbuf_size * sizeof(rxqdata_t),
	    KM_SLEEP);
	sc->sc_rbufend = sc->sc_rbuf + siotty_rbuf_size;
	sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
	sc->sc_rbavail = siotty_rbuf_size;

	tp = tty_alloc();
	tp->t_oproc = siostart;
	tp->t_param = sioparam;
	tp->t_hwiflow = NULL /* XXX siohwiflow XXX */;
	if ((sc->sc_hwflags & SIOTTY_HW_CONSOLE) != 0)
		tp->t_dev = cn_tab->cn_dev;
	sc->sc_tty = tp;

	tty_attach(tp);

	sc->sc_si = softint_establish(SOFTINT_SERIAL, siottysoft, sc);
}

/*--------------------  low level routine --------------------*/

static void
siottyintr(void *arg)
{
	struct siotty_softc *sc;
	struct sioreg *sio;
	rxqdata_t *put, *end;
	uint8_t c;
	uint16_t rr;
	int cc;

	sc = arg;
	end = sc->sc_rbufend;
	put = sc->sc_rbput;
	cc = sc->sc_rbavail;

	sio = sc->sc_ctl;
	rr = getsiocsr(sio);
	if ((rr & RR_BREAK) != 0) {
		cn_check_magic(sc->sc_tty->t_dev, CNC_BREAK, siotty_cnm_state);
	}
	/* XXX should handle RR_DCD and RR_CTS */
	sio->sio_cmd = WR0_RSTINT;

	if ((rr & RR_RXRDY) != 0) {
		do {
			if (cc > 0) {
				c = sio->sio_data;
				cn_check_magic(sc->sc_tty->t_dev, c,
				    siotty_cnm_state);
				put->data = c;
				put->stat = rr & 0xff;
				put++;
				if (put >= end)
					put = sc->sc_rbuf;
				cc--;
			}
			if ((rr & (RR_FRAMING | RR_OVERRUN | RR_PARITY)) != 0)
				sio->sio_cmd = WR0_ERRRST;

			sc->sc_rbput = put;
			sc->sc_rbavail = cc;
			sc->sc_rx_ready = true;
		} while (((rr = getsiocsr(sio)) & RR_RXRDY) != 0);
	}
	if ((rr & RR_TXRDY) != 0) {
		sio->sio_cmd = WR0_RSTPEND;
		if (sc->sc_tbc > 0) {
			sio->sio_data = *sc->sc_tba;
			sc->sc_tba++;
			sc->sc_tbc--;
		} else {
			if (sc->sc_tx_busy) {
				sc->sc_tx_busy = false;
				sc->sc_tx_done = true;
			}
		}
	}
	softint_schedule(sc->sc_si);
}

static void
siottysoft(void *arg)
{
	struct siotty_softc *sc;
	struct tty *tp;

	sc = arg;
	tp = sc->sc_tty;

	if (sc->sc_rx_ready) {
		sc->sc_rx_ready = false;
		siotty_rxsoft(sc, tp);
	}
	if (sc->sc_tx_done) {
		sc->sc_tx_done = false;
		siotty_txsoft(sc, tp);
	}
}

static void
siotty_rxsoft(struct siotty_softc *sc, struct tty *tp)
{
	rxqdata_t *get, *end;
	u_int cc, scc;
	unsigned int code;
	uint8_t stat;
	int s;

	end = sc->sc_rbufend;
	get = sc->sc_rbget;
	scc = cc = siotty_rbuf_size - sc->sc_rbavail;

	if (cc == siotty_rbuf_size) {
		printf("%s: rx buffer overflow\n", device_xname(sc->sc_dev));
	}

	while (cc > 0) {
		code = get->data;
		stat = get->stat;
		if ((stat & RR_FRAMING) != 0)
			code |= TTY_FE;
		else if ((stat & RR_PARITY) != 0)
			code |= TTY_PE;

		(*tp->t_linesw->l_rint)(code, tp);
		get++;
		if (get >= end)
			get = sc->sc_rbuf;
		cc--;
	}

	if (cc != scc) {
		s = splserial();
		sc->sc_rbget = get;
		sc->sc_rbavail += scc - cc;
		splx(s);
	}
}

static void
siotty_txsoft(struct siotty_softc *sc, struct tty *tp)
{

	tp->t_state &= ~TS_BUSY;
	if ((tp->t_state & TS_FLUSH) != 0)
		tp->t_state &= ~TS_FLUSH;
	else
		ndflush(&tp->t_outq, (int)(sc->sc_tba - tp->t_outq.c_cf));
	(*tp->t_linesw->l_start)(tp);
}

static void
siostart(struct tty *tp)
{
	struct siotty_softc *sc;
	int s;
	uint8_t *tba;
	int tbc;

	sc = device_lookup_private(&siotty_cd, minor(tp->t_dev));
	s = splserial();
	if ((tp->t_state & (TS_BUSY|TS_TIMEOUT|TS_TTSTOP)) != 0)
		goto out;
	if (!ttypull(tp))
		goto out;
	tp->t_state |= TS_BUSY;

	tba = tp->t_outq.c_cf;
	tbc = ndqb(&tp->t_outq, 0);

	sc->sc_tba = tba;
	sc->sc_tbc = tbc;
	sc->sc_tx_busy = true;

	sc->sc_ctl->sio_data = *sc->sc_tba;
	sc->sc_tba++;
	sc->sc_tbc--;
out:
	splx(s);
}

static void
siostop(struct tty *tp, int flag)
{
	int s;

	s = splserial();
	if (TS_BUSY == (tp->t_state & (TS_BUSY|TS_TTSTOP))) {
		/*
		 * Device is transmitting; must stop it.
		 */
		tp->t_state |= TS_FLUSH;
	}
	splx(s);
}

static int
sioparam(struct tty *tp, struct termios *t)
{
	struct siotty_softc *sc;
	int wr4, s;

	sc = device_lookup_private(&siotty_cd, minor(tp->t_dev));
	if (t->c_ispeed && t->c_ispeed != t->c_ospeed)
		return EINVAL;
	wr4 = ttspeedtab(t->c_ospeed, siospeedtab);
	if (wr4 < 0)
		return EINVAL;

	if ((sc->sc_flags & TIOCFLAG_SOFTCAR) != 0) {
		t->c_cflag |= CLOCAL;
		t->c_cflag &= ~HUPCL;
	}
	if ((sc->sc_flags & TIOCFLAG_CLOCAL) != 0)
		t->c_cflag |= CLOCAL;

	/*
	 * If there were no changes, don't do anything.  This avoids dropping
	 * input and improves performance when all we did was frob things like
	 * VMIN and VTIME.
	 */
	if (tp->t_ospeed == t->c_ospeed && tp->t_cflag == t->c_cflag)
		return 0;

	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	sc->sc_wr[WR3] &= ~WR3_RX8BIT;
	sc->sc_wr[WR5] &= ~WR5_TX8BIT;
	switch (tp->t_cflag & CSIZE) {
	case CS7:
		sc->sc_wr[WR3] |= WR3_RX7BIT;
		sc->sc_wr[WR5] |= WR5_TX7BIT;
		break;
	case CS8:
		sc->sc_wr[WR3] |= WR3_RX8BIT;
		sc->sc_wr[WR5] |= WR5_TX8BIT;
		break;
	}
	if ((tp->t_cflag & PARENB) != 0) {
		wr4 |= WR4_PARENAB;
		if ((tp->t_cflag & PARODD) == 0)
			wr4 |= WR4_EPARITY;
	}
	wr4 |= (tp->t_cflag & CSTOPB) ? WR4_STOP2 : WR4_STOP1;
	sc->sc_wr[WR4] = wr4;

	s = splserial();
	setsioreg(sc->sc_ctl, WR4, sc->sc_wr[WR4]);
	setsioreg(sc->sc_ctl, WR3, sc->sc_wr[WR3]);
	setsioreg(sc->sc_ctl, WR5, sc->sc_wr[WR5]);
	splx(s);

	return 0;
}

static int
siomctl(struct siotty_softc *sc, int control, int op)
{
	int val, s;
	uint8_t wr5;
	uint16_t rr;

	val = 0;
	if ((control & TIOCM_BREAK) != 0)
		val |= WR5_BREAK;
	if ((control & TIOCM_DTR) != 0)
		val |= WR5_DTR;
	if ((control & TIOCM_RTS) != 0)
		val |= WR5_RTS;
	s = splserial();
	wr5 = sc->sc_wr[WR5];
	switch (op) {
	case DMSET:
		wr5 &= ~(WR5_BREAK|WR5_DTR|WR5_RTS);
		/* FALLTHRU */
	case DMBIS:
		wr5 |= val;
		break;
	case DMBIC:
		wr5 &= ~val;
		break;
	case DMGET:
		val = 0;
		rr = getsiocsr(sc->sc_ctl);
		if ((wr5 & WR5_DTR) != 0)
			val |= TIOCM_DTR;
		if ((wr5 & WR5_RTS) != 0)
			val |= TIOCM_RTS;
		if ((rr & RR_CTS) != 0)
			val |= TIOCM_CTS;
		if ((rr & RR_DCD) != 0)
			val |= TIOCM_CD;
		goto done;
	}
	sc->sc_wr[WR5] = wr5;
	setsioreg(sc->sc_ctl, WR5, wr5);
	val = 0;
 done:
	splx(s);
	return val;
}

/*--------------------  cdevsw[] interface --------------------*/

static int
sioopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct siotty_softc *sc;
	struct tty *tp;
	int error;
	int s;

	sc = device_lookup_private(&siotty_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;

	tp = sc->sc_tty;

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return EBUSY;

	if ((tp->t_state & TS_ISOPEN) == 0 && tp->t_wopen == 0) {
		struct termios t;

		tp->t_dev = dev;
		t.c_ispeed = t.c_ospeed = TTYDEF_SPEED;
		t.c_cflag = TTYDEF_CFLAG;
		tp->t_ospeed = 0; /* force register update */
		(void)sioparam(tp, &t);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		ttychars(tp);
		ttsetwater(tp);
		/* raise RTS and DTR here; but, DTR lead is not wired */
		/* then check DCD condition; but, DCD lead is not wired */
#if 0
		if ((sc->sc_flags & TIOCFLAG_SOFTCAR) != 0 ||
		    (tp->t_cflag & MDMBUF) != 0 ||
		    (getsiocsr(sc->sc_ctl) & RR_DCD) != 0)
			tp->t_state |= TS_CARR_ON;
		else
			tp->t_state &= ~TS_CARR_ON;
#else
		tp->t_state |= TS_CARR_ON; /* assume detected all the time */
#endif

		s = splserial();
		sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
		sc->sc_rbavail = siotty_rbuf_size;
		splx(s);
	}

	error = ttyopen(tp, 0, (flag & O_NONBLOCK));
	if (error > 0)
		return error;
	return (*tp->t_linesw->l_open)(dev, tp);
}

static int
sioclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct siotty_softc *sc = device_lookup_private(&siotty_cd,minor(dev));
	struct tty *tp = sc->sc_tty;
	int s;

	(*tp->t_linesw->l_close)(tp, flag);

	s = splserial();
	siomctl(sc, TIOCM_BREAK, DMBIC);
#if 0 /* because unable to feed DTR signal */
	if ((tp->t_cflag & HUPCL) != 0 ||
	    tp->t_wopen || (tp->t_state & TS_ISOPEN) == 0) {
		siomctl(sc, TIOCM_DTR, DMBIC);
		/* Yield CPU time to others for 1 second, then ... */
		siomctl(sc, TIOCM_DTR, DMBIS);
	}
#endif
	splx(s);
	return ttyclose(tp);
}

static int
sioread(dev_t dev, struct uio *uio, int flag)
{
	struct siotty_softc *sc;
	struct tty *tp;

	sc = device_lookup_private(&siotty_cd, minor(dev));
	tp = sc->sc_tty;
	return (*tp->t_linesw->l_read)(tp, uio, flag);
}

static int
siowrite(dev_t dev, struct uio *uio, int flag)
{
	struct siotty_softc *sc;
	struct tty *tp;

	sc = device_lookup_private(&siotty_cd, minor(dev));
	tp = sc->sc_tty;
	return (*tp->t_linesw->l_write)(tp, uio, flag);
}

static int
siopoll(dev_t dev, int events, struct lwp *l)
{
	struct siotty_softc *sc;
	struct tty *tp;

	sc = device_lookup_private(&siotty_cd, minor(dev));
	tp = sc->sc_tty;
	return (*tp->t_linesw->l_poll)(tp, events, l);
}

static int
sioioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct siotty_softc *sc;
	struct tty *tp;
	int error;

	sc = device_lookup_private(&siotty_cd, minor(dev));
	tp = sc->sc_tty;
	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	error = ttioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	/* the last resort for TIOC ioctl tranversing */
	switch (cmd) {
	case TIOCSBRK: /* Set the hardware into BREAK condition */
		siomctl(sc, TIOCM_BREAK, DMBIS);
		break;
	case TIOCCBRK: /* Clear the hardware BREAK condition */
		siomctl(sc, TIOCM_BREAK, DMBIC);
		break;
	case TIOCSDTR: /* Assert DTR signal */
		siomctl(sc, TIOCM_DTR|TIOCM_RTS, DMBIS);
		break;
	case TIOCCDTR: /* Clear DTR signal */
		siomctl(sc, TIOCM_DTR|TIOCM_RTS, DMBIC);
		break;
	case TIOCMSET: /* Set modem state replacing current one */
		siomctl(sc, *(int *)data, DMSET);
		break;
	case TIOCMGET: /* Return current modem state */
		*(int *)data = siomctl(sc, 0, DMGET);
		break;
	case TIOCMBIS: /* Set individual bits of modem state */
		siomctl(sc, *(int *)data, DMBIS);
		break;
	case TIOCMBIC: /* Clear individual bits of modem state */
		siomctl(sc, *(int *)data, DMBIC);
		break;
	case TIOCSFLAGS: /* Instruct how serial port behaves */
		sc->sc_flags = *(int *)data;
		break;
	case TIOCGFLAGS: /* Return current serial port state */
		*(int *)data = sc->sc_flags;
		break;
	default:
		return EPASSTHROUGH;
	}
	return 0;
}

/* ARSGUSED */
static struct tty *
siotty(dev_t dev)
{
	struct siotty_softc *sc;

	sc = device_lookup_private(&siotty_cd, minor(dev));
	return sc->sc_tty;
}

/*---------------------  console interface ----------------------*/

struct consdev siottycons = {
	.cn_probe = NULL,
	.cn_init  = siottycninit,
	.cn_getc  = siottycngetc,
	.cn_putc  = siottycnputc,
	.cn_pollc = nullcnpollc,
	.cn_bell  = NULL,
	.cn_halt  = NULL,
	.cn_flush = NULL,
	.cn_dev   = NODEV,
	.cn_pri   = CN_REMOTE,
};

static void
siottycninit(struct consdev *cn)
{
	/*
	 * Channel A is immediately initialized with 9600N1 right after cold
	 * boot/reset/poweron.  ROM monitor emits one line message on CH.A.
	 */
	struct sioreg *sio, *sio_base, *sio_a, *sio_b;

	sio_base = (struct sioreg *)OBIO_SIO;
	sio_a = &sio_base[0];
	sio_b = &sio_base[1];
	sio   = sio_a;

	siottycons.cn_dev = makedev(cdevsw_lookup_major(&siotty_cdevsw), 0);
	cn_init_magic(&siotty_cnm_state);
	cn_set_magic("\047\001");

	setsioreg(sio, WR0, WR0_CHANRST);
	setsioreg(sio_a, WR2A, WR2A_VEC86 | WR2A_INTR_1);
	setsioreg(sio_b, WR2B, 0);
	setsioreg(sio, WR0, ch0_regs[WR0]);
	setsioreg(sio, WR4, ch0_regs[WR4]);
	setsioreg(sio, WR3, ch0_regs[WR3]);
	setsioreg(sio, WR5, ch0_regs[WR5]);
	setsioreg(sio, WR0, ch0_regs[WR0]);
}

static int
siottycngetc(dev_t dev)
{
	struct sioreg *sio;

	sio = (struct sioreg *)OBIO_SIO;
	return siogetc(sio);
}

static void
siottycnputc(dev_t dev, int c)
{
	struct sioreg *sio;

	sio = (struct sioreg *)OBIO_SIO;
	sioputc(sio, c);
}

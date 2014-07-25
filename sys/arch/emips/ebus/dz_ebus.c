/*	$NetBSD: dz_ebus.c,v 1.8 2014/07/25 08:10:32 dholland Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was written by Alessandro Forin and Neil Pittman
 * at Microsoft Research and contributed to The NetBSD Foundation
 * by Microsoft Corporation.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dz_ebus.c,v 1.8 2014/07/25 08:10:32 dholland Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/kauth.h>

#include <machine/bus.h>
#include <machine/emipsreg.h>

#include <dev/cons.h>


#include <emips/ebus/ebusvar.h>
#include <emips/emips/cons.h>
#if 0
#include <emips/emips/machdep.h>
#endif

#include "ioconf.h" /* for dz_cd */

#define DZ_C2I(c)	((c) << 3)	/* convert controller # to index */
#define DZ_I2C(c)	((c) >> 3)	/* convert minor to controller # */
#define DZ_PORT(u)	((u) & 07)	/* extract the port # */

struct	dz_softc {
	device_t	sc_dev;		/* Autoconf blaha */
	struct	evcnt	sc_rintrcnt;	/* recevive interrupt counts */
	struct	evcnt	sc_tintrcnt;	/* transmit interrupt counts */
	struct	_Usart	*sc_dr;		/* reg pointers */
	bus_space_tag_t	sc_iot;
	bus_space_handle_t sc_ioh;
	int		sc_consline;	/* console line, or -1 */
	int		sc_rxint;	/* Receive interrupt count XXX */
	u_char		sc_brk;		/* Break asserted on some lines */
	u_char		sc_dsr;		/* DSR set bits if no mdm ctrl */
	struct dz_linestate {
		struct	dz_softc *dz_sc;	/* backpointer to softc */
		int		dz_line;	/* channel number */
		struct	tty	*dz_tty;	/* what we work on */
	} sc_dz;
};

void	dzrint(struct dz_softc *, uint32_t);
void	dzxint(struct dz_softc *, uint32_t);

#ifndef TIOCM_BRK
#define TIOCM_BRK		0100000		/* no equivalent */

static void	dzstart(struct tty *);
static int	dzparam(struct tty *, struct termios *);
static unsigned dzmctl(struct dz_softc *sc, int line,
                       int bits, /* one of the TIOCM_xx */
                       int how); /* one of the DMSET/BIS.. */

#include <dev/dec/dzkbdvar.h>
#endif

dev_type_open(dzopen);
dev_type_close(dzclose);
dev_type_read(dzread);
dev_type_write(dzwrite);
dev_type_ioctl(dzioctl);
dev_type_stop(dzstop);
dev_type_tty(dztty);
dev_type_poll(dzpoll);

const struct cdevsw dz_cdevsw = {
	.d_open = dzopen,
	.d_close = dzclose,
	.d_read = dzread,
	.d_write = dzwrite,
	.d_ioctl = dzioctl,
	.d_stop = dzstop,
	.d_tty = dztty,
	.d_poll = dzpoll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

int
dzopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct tty *tp;
	int unit, line;
	struct dz_softc *sc;
	int s, error = 0;

	unit = DZ_I2C(minor(dev));
	sc = device_lookup_private(&dz_cd, unit);
	if (sc == NULL)
		return ENXIO;

	line = DZ_PORT(minor(dev));
	if (line > 0) /* FIXME for more than one line */
		return ENXIO;

	tp = sc->sc_dz.dz_tty;
	if (tp == NULL)
		return ENODEV;
	tp->t_oproc = dzstart;
	tp->t_param = dzparam;
	tp->t_dev   = dev;

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return (EBUSY);

	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);
		if (tp->t_ispeed == 0) {
			tp->t_iflag = TTYDEF_IFLAG;
			tp->t_oflag = TTYDEF_OFLAG;
			tp->t_cflag = TTYDEF_CFLAG;
			tp->t_lflag = TTYDEF_LFLAG;
			tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		}
		(void)dzparam(tp, &tp->t_termios);
		ttsetwater(tp);
	}
	/* we have no modem control but..*/
	if (dzmctl(sc, line, TIOCM_DTR, DMBIS) & TIOCM_CD)
		tp->t_state |= TS_CARR_ON;
		s = spltty();
		while (!(flag & O_NONBLOCK) && !(tp->t_cflag & CLOCAL) &&
		    !(tp->t_state & TS_CARR_ON)) {
			tp->t_wopen++;
			error = ttysleep(tp, &tp->t_rawcv, true, 0);
			tp->t_wopen--;
			if (error)
				break;
		}
	(void)splx(s);
	if (error)
		return error;
	return (*tp->t_linesw->l_open)(dev, tp);
}

int
dzclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct dz_softc *sc;
	struct tty *tp;
	int unit, line;

	unit = DZ_I2C(minor(dev));
	sc = device_lookup_private(&dz_cd, unit);
	line = DZ_PORT(minor(dev));

	tp = sc->sc_dz.dz_tty;

	(*tp->t_linesw->l_close)(tp, flag);

	/* Make sure a BREAK state is not left enabled. */
	(void)dzmctl(sc, line, TIOCM_BRK, DMBIC);

	/* Do a hangup if so required. */
	if ((tp->t_cflag & HUPCL) || tp->t_wopen || !(tp->t_state & TS_ISOPEN))
		(void)dzmctl(sc, line, 0, DMSET);

	return ttyclose(tp);
}

int
dzread(dev_t dev, struct uio *uio, int flag)
{
	struct tty *tp;
	struct dz_softc *sc;

	sc = device_lookup_private(&dz_cd, DZ_I2C(minor(dev)));

	tp = sc->sc_dz.dz_tty;
	return (*tp->t_linesw->l_read)(tp, uio, flag);
}

int
dzwrite(dev_t dev, struct uio *uio, int flag)
{
	struct tty *tp;
	struct dz_softc *sc;

	sc = device_lookup_private(&dz_cd, DZ_I2C(minor(dev)));

	tp = sc->sc_dz.dz_tty;
	return (*tp->t_linesw->l_write)(tp, uio, flag);
}

/*ARGSUSED*/
int
dzioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct	dz_softc *sc;
	struct tty *tp;
	int unit, line;
	int error;

	unit = DZ_I2C(minor(dev));
	line = 0;
	sc = device_lookup_private(&dz_cd, unit);
	tp = sc->sc_dz.dz_tty;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error >= 0)
		return error;

	error = ttioctl(tp, cmd, data, flag, l);
	if (error >= 0)
		return error;

	switch (cmd) {

	case TIOCSBRK:
		(void)dzmctl(sc, line, TIOCM_BRK, DMBIS);
		break;

	case TIOCCBRK:
		(void)dzmctl(sc, line, TIOCM_BRK, DMBIC);
		break;

	case TIOCSDTR:
		(void)dzmctl(sc, line, TIOCM_DTR, DMBIS);
		break;

	case TIOCCDTR:
		(void)dzmctl(sc, line, TIOCM_DTR, DMBIC);
		break;

	case TIOCMSET:
		(void)dzmctl(sc, line, *(int *)data, DMSET);
		break;

	case TIOCMBIS:
		(void)dzmctl(sc, line, *(int *)data, DMBIS);
		break;

	case TIOCMBIC:
		(void)dzmctl(sc, line, *(int *)data, DMBIC);
		break;

	case TIOCMGET:
		*(int *)data = dzmctl(sc, line, 0, DMGET) & ~TIOCM_BRK;
		break;

	default:
		return EPASSTHROUGH;
	}
	return 0;
}

/*ARGSUSED*/
void
dzstop(struct tty *tp, int flag)
{

	if (tp->t_state & TS_BUSY)
		if (!(tp->t_state & TS_TTSTOP))
			tp->t_state |= TS_FLUSH;
}

struct tty *
dztty(dev_t dev)
{
	struct dz_softc *sc;
	struct tty *tp;

	sc = device_lookup_private(&dz_cd, DZ_I2C(minor(dev)));
	tp = sc->sc_dz.dz_tty;

	return tp;
}

int
dzpoll(dev_t dev, int events, struct lwp *l)
{
	struct dz_softc *sc;
	struct tty *tp;

	sc = device_lookup_private(&dz_cd, DZ_I2C(minor(dev)));

	tp = sc->sc_dz.dz_tty;
	return (*tp->t_linesw->l_poll)(tp, events, l);
}

void
dzstart(struct tty *tp)
{
	struct dz_softc *sc;
	struct clist *cl;
	int unit, s;

	unit = DZ_I2C(minor(tp->t_dev));
	sc = device_lookup_private(&dz_cd, unit);

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) {
		splx(s);
		return;
	}
	cl = &tp->t_outq;
	ttypull(tp);
	if (cl->c_cc == 0) {
		splx(s);
		return;
	}

	tp->t_state |= TS_BUSY;

	/* was idle, get it started */
	dzxint(sc,USI_TXRDY);
	splx(s);
}

static int rclk = 25000000; /* BUGBUGBUGBUG */

static int
dzdivisor(int baudrate)
{
	int act_baud, divisor, error;

	if (baudrate <= 0)
		return 0;

	divisor = (rclk / 8) / (baudrate);
	divisor = (divisor / 2) + (divisor & 1);

	if (divisor <= 0)
		return -1;
	act_baud = rclk / (divisor * 16);

	/* 10 times error in percent: */
	error = ((act_baud - baudrate) * 2000 / baudrate + 1) >> 1;

	/* 3.0% maximum error tolerance: */
	if (error < -30 || error > 30)
		return -1;

	return divisor;
}

static int
dzparam(struct tty *tp, struct termios *t)
{
	struct	dz_softc *sc;
	int cflag = t->c_cflag;
	int unit, line;
	int speed;
	unsigned lpr;
	int s;
	struct _Usart *dzr;

	unit = DZ_I2C(minor(tp->t_dev));
	line = DZ_PORT(minor(tp->t_dev));
	sc = device_lookup_private(&dz_cd, unit);

	/* check requested parameters */
	if (t->c_ispeed != t->c_ospeed)
		return EINVAL;
	speed = dzdivisor(t->c_ispeed);
	if (speed < 0)
		return EINVAL;

	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = cflag;

	{
		/* XXX */
		static int didit = 0;
		if (!didit && t->c_ispeed != 38400)
			printf("dzparam: c_ispeed %d ignored, keeping 38400\n",
			    t->c_ispeed);
		didit = 1;
	}
	speed = dzdivisor(38400);

	if (speed == 0) {
		(void)dzmctl(sc, line, 0, DMSET);	/* hang up line */
		return 0;
	}

	switch (cflag & CSIZE) {
	case CS5:
		lpr = USC_BPC_5;
		break;
	case CS6:
		lpr = USC_BPC_6;
		break;
	case CS7:
		lpr = USC_BPC_7;
		break;
	default:
		lpr = USC_BPC_8;
		break;
	}
	if (cflag & CSTOPB)
		lpr |= USC_2STOP;

	if (cflag & PARENB) {
		if (cflag & PARODD)
			lpr |= USC_ODD;
		else
			lpr |= USC_EVEN;
	} else
		lpr |= USC_NONE;

	s = spltty();

	dzr = sc->sc_dr;

	dzr->Baud = speed;
	dzr->Control = USC_CLKDIV_4 | USC_TXEN | USC_RXEN | lpr;
#define USI_INTRS (USI_RXRDY|USI_RXBRK|USI_OVRE|USI_FRAME|USI_PARE)
	dzr->IntrEnable = USI_INTRS;

	(void)splx(s);
	return 0;
}

static unsigned
dzmctl(struct dz_softc *sc, int line, int bits, int how)
{
	unsigned int mbits;
	int s;
	struct _Usart *dzr;

	mbits = 0;

	s = spltty();

	dzr = sc->sc_dr;

	/* we have no modem control bits (CD,RI,DTR,DSR,..) */
	mbits |= TIOCM_CD;
	mbits |= TIOCM_DTR;

	if (dzr->ChannelStatus & USI_RXBRK)
		mbits |= TIOCM_BRK;

	switch (how) {
	case DMSET:
		mbits = bits;
		break;

	case DMBIS:
		mbits |= bits;
		break;

	case DMBIC:
		mbits &= ~bits;
		break;

	case DMGET:
		(void)splx(s);
		return mbits;
	}

	/* BUGBUG work in progress */
	if (mbits & TIOCM_BRK) {
		sc->sc_brk |= (1 << line);
		dzr->Control |= USC_STTBRK;
	} else {
		sc->sc_brk &= ~(1 << line);
		dzr->Control |= USC_STPBRK;
	}

	(void)splx(s);
	return mbits;
}


#if defined(DDB)
int dz_ddb = 0;
#endif

/* Receiver Interrupt */

void
dzrint(struct dz_softc *sc, uint32_t csr)
{
	struct tty *tp;
	int cc;
	struct _Usart *dzr;

	sc->sc_rxint++;
	dzr = sc->sc_dr;

	cc = dzr->RxData;
	tp = sc->sc_dz.dz_tty;

	/* clear errors before we print or bail out */
	if (csr & (USI_OVRE|USI_FRAME|USI_PARE))
		dzr->Control = USC_RSTSTA;

	if (!(tp->t_state & TS_ISOPEN)) {
		wakeup(&tp->t_rawq);
		return;
	}

	if (csr & USI_OVRE) {
		log(LOG_WARNING, "%s: silo overflow, line %d\n",
		    device_xname(sc->sc_dev), 0);
	}

	if (csr & USI_FRAME)
		cc |= TTY_FE;
	if (csr & USI_PARE)
		cc |= TTY_PE;

#if defined(DDB)
	/* ^P drops into DDB */
	if (dz_ddb && (cc == 0x10))
		Debugger();
#endif
	(*tp->t_linesw->l_rint)(cc, tp);
}

/* Transmitter Interrupt */

void
dzxint(struct dz_softc *sc, uint32_t csr)
{
	struct tty *tp;
	struct clist *cl;
	int ch;
	struct _Usart *dzr;

	dzr = sc->sc_dr;

	tp = sc->sc_dz.dz_tty;
	cl = &tp->t_outq;
	tp->t_state &= ~TS_BUSY;

	/* Just send out a char if we have one */
	if (cl->c_cc) {
		tp->t_state |= TS_BUSY;
		ch = getc(cl);
		dzr->TxData = ch;
		dzr->IntrEnable = USI_TXRDY;
		return;
	}

	/* Nothing to send; turn off intr */
	dzr->IntrDisable = USI_TXRDY;

	if (tp->t_state & TS_FLUSH)
		tp->t_state &= ~TS_FLUSH;
	else
		ndflush(&tp->t_outq, cl->c_cc);

	(*tp->t_linesw->l_start)(tp);
}

/*
 * Machdep part of the driver
 */
int	dz_ebus_match(device_t, cfdata_t, void *);
void	dz_ebus_attach(device_t, device_t, void *);
int	dz_ebus_intr(void *, void *);

void	dz_ebus_cnsetup(paddr_t);
void	dz_ebus_cninit(struct consdev *);
int	dz_ebus_cngetc(dev_t);
void	dz_ebus_cnputc(dev_t, int);
void	dz_ebus_cnpollc(dev_t, int);

static int	dz_ebus_getmajor(void);

CFATTACH_DECL_NEW(dz_ebus, sizeof(struct dz_softc),
    dz_ebus_match, dz_ebus_attach, NULL, NULL);

struct consdev dz_ebus_consdev = {
	NULL, dz_ebus_cninit, dz_ebus_cngetc, dz_ebus_cnputc,
	dz_ebus_cnpollc, NULL, NULL, NULL, NODEV, CN_NORMAL,
};

/*
 * Points to the console regs. Special mapping until VM is turned on.
 */
struct _Usart *dzcn;

int
dz_ebus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct ebus_attach_args *iba;
	struct _Usart *us;

	iba = aux;

	if (strcmp(iba->ia_name, "dz") != 0)
		return 0;

	us = (struct _Usart *)iba->ia_vaddr;
	if ((us == NULL) ||
	    (us->Tag != PMTTAG_USART))
		return 0;

	return 1;
}

void
dz_ebus_attach(device_t parent, device_t self, void *aux)
{
	struct ebus_attach_args *iba;
	struct dz_softc *sc;

	sc = device_private(self);
	iba = aux;

	sc->sc_dev = self;
	sc->sc_dr = (struct _Usart *)iba->ia_vaddr;
#if DEBUG
	printf(" virt=%p ", (void *)sc->sc_dr);
#endif

	printf(": neilsart 1 line");
	ebus_intr_establish(parent, (void *)iba->ia_cookie, IPL_TTY,
	    dz_ebus_intr, sc);

	sc->sc_rxint = sc->sc_brk = 0;
	sc->sc_consline = 0;

	/* Initialize our softc structure. Should be done in open? */

	sc->sc_dz.dz_sc = sc;
	sc->sc_dz.dz_line = 0;
	sc->sc_dz.dz_tty = tty_alloc();

	evcnt_attach_dynamic(&sc->sc_rintrcnt, EVCNT_TYPE_INTR, NULL,
	    device_xname(self), "rintr");
	evcnt_attach_dynamic(&sc->sc_tintrcnt, EVCNT_TYPE_INTR, NULL,
	    device_xname(self), "tintr");

	/* Initialize hw regs */
#if 0
	DZ_WRITE_WORD(dr_csr, DZ_CSR_MSE | DZ_CSR_RXIE | DZ_CSR_TXIE);
	DZ_WRITE_BYTE(dr_dtr, 0);
	DZ_WRITE_BYTE(dr_break, 0);
#endif

	/* Switch the console to virtual mode */
	dzcn = sc->sc_dr;
	/* And test it */
	printf("\n");
}

static int
dz_ebus_getmajor(void)
{
	extern const struct cdevsw dz_cdevsw;
	static int cache = -1;

	if (cache != -1)
		return cache;

	return cache = cdevsw_lookup_major(&dz_cdevsw);
}

int
dz_ebus_intr(void *cookie, void *f)
{
	struct dz_softc *sc;
	struct _Usart *dzr;
	uint32_t csr;

	sc = cookie;
	dzr = sc->sc_dr;

#define USI_INTERRUPTS (USI_INTRS|USI_TXRDY)

	for (; ((csr = (dzr->ChannelStatus & dzr->IntrMask)) &
	    USI_INTERRUPTS) != 0;) {
		if ((csr & USI_INTRS) != 0)
			dzrint(sc, csr);
		if ((csr & USI_TXRDY) != 0)
			dzxint(sc, csr);
	}

	return 0;
}

void
dz_ebus_cnsetup(paddr_t addr)
{

	dzcn = (struct _Usart *)addr;

#if 0
	/*
	 * Initialize enough to xmit/recv via polling.
	 * Bootloader might or might not have done it.
	 */
	dzcn->Control =
	    USC_RXEN |
	    USC_TXEN |
	    USC_BPC_8 |
	    USC_NONE |
	    USC_1STOP |
	    USC_CLKDIV_4;
	dzcn->Baud = 0x29; /* 38400 */
#endif

	/*
	 * Point the console at us
	 */
	cn_tab = &dz_ebus_consdev;
	cn_tab->cn_pri = CN_NORMAL;/*CN_REMOTE?*/
	cn_tab->cn_dev = makedev(dz_ebus_getmajor(), 0);
}

void
dz_ebus_cninit(struct consdev *cn)
{
}

int
dz_ebus_cngetc(dev_t dev)
{
	int c, s;

	c = 0;
	s = spltty();

	while ((dzcn->ChannelStatus & USI_RXRDY) == 0)
		DELAY(10);
	c = dzcn->RxData;

	splx(s);
	if (c == 13) /* map cr->ln */
		c = 10;
	return c;
}

int dzflipped = 0;
void
dz_ebus_cnputc(dev_t dev, int ch)
{
	int timeout, s;

	/* Don't hang the machine! */
	timeout = 1 << 15;

	s = spltty();

#if 1
	/* Keep wired to hunt for a bug */
	if (dzcn && (dzcn != (struct _Usart *)0xfff90000)) {
		dzcn = (struct _Usart *)0xfff90000;
		dzflipped++;
	}
#endif

	/* Wait until ready */
	while ((dzcn->ChannelStatus & USI_TXRDY) == 0)
		if (--timeout < 0)
			break;

	/* Put the character */
	dzcn->TxData = ch;

	splx(s);
}

/*
 * Called before/after going into poll mode
 */
void
dz_ebus_cnpollc(dev_t dev, int on)
{
}

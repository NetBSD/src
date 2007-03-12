/*	$NetBSD: dz.c,v 1.24.4.1 2007/03/12 05:53:09 rmind Exp $	*/
/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 */

/*
 * Copyright (c) 1996  Ken C. Wellsch.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dz.c,v 1.24.4.1 2007/03/12 05:53:09 rmind Exp $");

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

#include <dev/dec/dzreg.h>
#include <dev/dec/dzvar.h>

#include <dev/cons.h>

#define	DZ_READ_BYTE(adr) \
	bus_space_read_1(sc->sc_iot, sc->sc_ioh, sc->sc_dr.adr)
#define	DZ_READ_WORD(adr) \
	bus_space_read_2(sc->sc_iot, sc->sc_ioh, sc->sc_dr.adr)
#define	DZ_WRITE_BYTE(adr, val) \
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, sc->sc_dr.adr, val)
#define	DZ_WRITE_WORD(adr, val) \
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, sc->sc_dr.adr, val)
#define	DZ_BARRIER() \
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, sc->sc_dr.dr_firstreg, \
	    sc->sc_dr.dr_winsize, \
	    BUS_SPACE_BARRIER_WRITE | BUS_SPACE_BARRIER_READ)

#include "ioconf.h"

/* Flags used to monitor modem bits, make them understood outside driver */

#define DML_DTR		TIOCM_DTR
#define DML_DCD		TIOCM_CD
#define DML_RI		TIOCM_RI
#define DML_BRK		0100000		/* no equivalent, we will mask */

static const struct speedtab dzspeedtab[] =
{
  {       0,	0		},
  {      50,	DZ_LPR_B50	},
  {      75,	DZ_LPR_B75	},
  {     110,	DZ_LPR_B110	},
  {     134,	DZ_LPR_B134	},
  {     150,	DZ_LPR_B150	},
  {     300,	DZ_LPR_B300	},
  {     600,	DZ_LPR_B600	},
  {    1200,	DZ_LPR_B1200	},
  {    1800,	DZ_LPR_B1800	},
  {    2000,	DZ_LPR_B2000	},
  {    2400,	DZ_LPR_B2400	},
  {    3600,	DZ_LPR_B3600	},
  {    4800,	DZ_LPR_B4800	},
  {    7200,	DZ_LPR_B7200	},
  {    9600,	DZ_LPR_B9600	},
  {   19200,	DZ_LPR_B19200	},
  {      -1,	-1		}
};

static void	dzstart(struct tty *);
static int	dzparam(struct tty *, struct termios *);
static unsigned	dzmctl(struct dz_softc *, int, int, int);
static void	dzscan(void *);

dev_type_open(dzopen);
dev_type_close(dzclose);
dev_type_read(dzread);
dev_type_write(dzwrite);
dev_type_ioctl(dzioctl);
dev_type_stop(dzstop);
dev_type_tty(dztty);
dev_type_poll(dzpoll);

const struct cdevsw dz_cdevsw = {
	dzopen, dzclose, dzread, dzwrite, dzioctl,
	dzstop, dztty, dzpoll, nommap, ttykqfilter, D_TTY
};

/*
 * The DZ series doesn't interrupt on carrier transitions,
 * so we have to use a timer to watch it.
 */
int	dz_timer;	/* true if timer started */
struct callout dzscan_ch;
static struct cnm_state dz_cnm_state;

void
dzattach(struct dz_softc *sc, struct evcnt *parent_evcnt, int consline)
{
	int n;

	sc->sc_rxint = sc->sc_brk = 0;
	sc->sc_consline = consline;

	sc->sc_dr.dr_tcrw = sc->sc_dr.dr_tcr;
	DZ_WRITE_WORD(dr_csr, DZ_CSR_MSE | DZ_CSR_RXIE | DZ_CSR_TXIE);
	DZ_WRITE_BYTE(dr_dtr, 0);
	DZ_WRITE_BYTE(dr_break, 0);
	DZ_BARRIER();

	/* Initialize our softc structure. Should be done in open? */

	for (n = 0; n < sc->sc_type; n++) {
		sc->sc_dz[n].dz_sc = sc;
		sc->sc_dz[n].dz_line = n;
		sc->sc_dz[n].dz_tty = ttymalloc();
	}

	evcnt_attach_dynamic(&sc->sc_rintrcnt, EVCNT_TYPE_INTR, parent_evcnt,
		sc->sc_dev.dv_xname, "rintr");
	evcnt_attach_dynamic(&sc->sc_tintrcnt, EVCNT_TYPE_INTR, parent_evcnt,
		sc->sc_dev.dv_xname, "tintr");

	/* Console magic keys */
	cn_init_magic(&dz_cnm_state);
	cn_set_magic("\047\001"); /* default magic is BREAK */
				  /* VAX will change it in MD code */

	/* Alas no interrupt on modem bit changes, so we manually scan */

	if (dz_timer == 0) {
		dz_timer = 1;
		callout_init(&dzscan_ch);
		callout_reset(&dzscan_ch, hz, dzscan, NULL);
	}
	printf("\n");
}

/* Receiver Interrupt */

void
dzrint(void *arg)
{
	struct dz_softc *sc = arg;
	struct tty *tp;
	int cc, mcc, line;
	unsigned c;
	int overrun = 0;

	sc->sc_rxint++;

	while ((c = DZ_READ_WORD(dr_rbuf)) & DZ_RBUF_DATA_VALID) {
		cc = c & 0xFF;
		line = DZ_PORT(c>>8);
		tp = sc->sc_dz[line].dz_tty;

		/* Must be caught early */
		if (sc->sc_dz[line].dz_catch &&
		    (*sc->sc_dz[line].dz_catch)(sc->sc_dz[line].dz_private, cc))
			continue;

		if ((c & (DZ_RBUF_FRAMING_ERR | 0xff)) == DZ_RBUF_FRAMING_ERR)
			mcc = CNC_BREAK;
		else
			mcc = cc;

		cn_check_magic(tp->t_dev, mcc, dz_cnm_state);

		if (!(tp->t_state & TS_ISOPEN)) {
			wakeup((void *)&tp->t_rawq);
			continue;
		}

		if ((c & DZ_RBUF_OVERRUN_ERR) && overrun == 0) {
			log(LOG_WARNING, "%s: silo overflow, line %d\n",
			    sc->sc_dev.dv_xname, line);
			overrun = 1;
		}

		if (c & DZ_RBUF_FRAMING_ERR)
			cc |= TTY_FE;
		if (c & DZ_RBUF_PARITY_ERR)
			cc |= TTY_PE;

		(*tp->t_linesw->l_rint)(cc, tp);
	}
}

/* Transmitter Interrupt */

void
dzxint(void *arg)
{
	struct dz_softc *sc = arg;
	struct tty *tp;
	struct clist *cl;
	int line, ch, csr;
	u_char tcr;

	/*
	 * Switch to POLLED mode.
	 *   Some simple measurements indicated that even on
	 *  one port, by freeing the scanner in the controller
	 *  by either providing a character or turning off
	 *  the port when output is complete, the transmitter
	 *  was ready to accept more output when polled again.
	 *   With just two ports running the game "worms,"
	 *  almost every interrupt serviced both transmitters!
	 *   Each UART is double buffered, so if the scanner
	 *  is quick enough and timing works out, we can even
	 *  feed the same port twice.
	 *
	 * Ragge 980517:
	 * Do not need to turn off interrupts, already at interrupt level.
	 * Remove the pdma stuff; no great need of it right now.
	 */

	while (((csr = DZ_READ_WORD(dr_csr)) & DZ_CSR_TX_READY) != 0) {

		line = DZ_PORT(csr>>8);

		tp = sc->sc_dz[line].dz_tty;
		cl = &tp->t_outq;
		tp->t_state &= ~TS_BUSY;

		/* Just send out a char if we have one */
		/* As long as we can fill the chip buffer, we just loop here */
		if (cl->c_cc) {
			tp->t_state |= TS_BUSY;
			ch = getc(cl);
			DZ_WRITE_BYTE(dr_tbuf, ch);
			DZ_BARRIER();
			continue;
		}
		/* Nothing to send; clear the scan bit */
		/* Clear xmit scanner bit; dzstart may set it again */
		tcr = DZ_READ_WORD(dr_tcrw);
		tcr &= 255;
		tcr &= ~(1 << line);
		DZ_WRITE_BYTE(dr_tcr, tcr);
		DZ_BARRIER();
		if (sc->sc_dz[line].dz_catch)
			continue;

		if (tp->t_state & TS_FLUSH)
			tp->t_state &= ~TS_FLUSH;
		else
			ndflush (&tp->t_outq, cl->c_cc);

		(*tp->t_linesw->l_start)(tp);
	}
}

int
dzopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct tty *tp;
	int unit, line;
	struct	dz_softc *sc;
	int s, error = 0;

	unit = DZ_I2C(minor(dev));
	line = DZ_PORT(minor(dev));
	if (unit >= dz_cd.cd_ndevs ||  dz_cd.cd_devs[unit] == NULL)
		return (ENXIO);

	sc = dz_cd.cd_devs[unit];

	if (line >= sc->sc_type)
		return ENXIO;

	/* if some other device is using the line, it's busy */
	if (sc->sc_dz[line].dz_catch)
		return EBUSY;

	tp = sc->sc_dz[line].dz_tty;
	if (tp == NULL)
		return (ENODEV);
	tp->t_oproc   = dzstart;
	tp->t_param   = dzparam;
	tp->t_dev = dev;

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
		(void) dzparam(tp, &tp->t_termios);
		ttsetwater(tp);
	}
	/* Use DMBIS and *not* DMSET or else we clobber incoming bits */
	if (dzmctl(sc, line, DML_DTR, DMBIS) & DML_DCD)
		tp->t_state |= TS_CARR_ON;
	s = spltty();
	while (!(flag & O_NONBLOCK) && !(tp->t_cflag & CLOCAL) &&
	       !(tp->t_state & TS_CARR_ON)) {
		tp->t_wopen++;
		error = ttysleep(tp, (void *)&tp->t_rawq,
				TTIPRI | PCATCH, ttopen, 0);
		tp->t_wopen--;
		if (error)
			break;
	}
	(void) splx(s);
	if (error)
		return (error);
	return ((*tp->t_linesw->l_open)(dev, tp));
}

/*ARGSUSED*/
int
dzclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct	dz_softc *sc;
	struct tty *tp;
	int unit, line;


	unit = DZ_I2C(minor(dev));
	line = DZ_PORT(minor(dev));
	sc = dz_cd.cd_devs[unit];

	tp = sc->sc_dz[line].dz_tty;

	(*tp->t_linesw->l_close)(tp, flag);

	/* Make sure a BREAK state is not left enabled. */
	(void) dzmctl(sc, line, DML_BRK, DMBIC);

	/* Do a hangup if so required. */
	if ((tp->t_cflag & HUPCL) || tp->t_wopen || !(tp->t_state & TS_ISOPEN))
		(void) dzmctl(sc, line, 0, DMSET);

	return (ttyclose(tp));
}

int
dzread(dev_t dev, struct uio *uio, int flag)
{
	struct tty *tp;
	struct	dz_softc *sc;

	sc = dz_cd.cd_devs[DZ_I2C(minor(dev))];

	tp = sc->sc_dz[DZ_PORT(minor(dev))].dz_tty;
	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
dzwrite(dev_t dev, struct uio *uio, int flag)
{
	struct tty *tp;
	struct	dz_softc *sc;

	sc = dz_cd.cd_devs[DZ_I2C(minor(dev))];

	tp = sc->sc_dz[DZ_PORT(minor(dev))].dz_tty;
	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
dzpoll(dev, events, l)
	dev_t dev;
	int events;
	struct lwp *l;
{
	struct tty *tp;
	struct	dz_softc *sc;

	sc = dz_cd.cd_devs[DZ_I2C(minor(dev))];

	tp = sc->sc_dz[DZ_PORT(minor(dev))].dz_tty;
	return ((*tp->t_linesw->l_poll)(tp, events, l));
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
	line = DZ_PORT(minor(dev));
	sc = dz_cd.cd_devs[unit];
	tp = sc->sc_dz[line].dz_tty;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error >= 0)
		return (error);

	error = ttioctl(tp, cmd, data, flag, l);
	if (error >= 0)
		return (error);

	switch (cmd) {

	case TIOCSBRK:
		(void) dzmctl(sc, line, DML_BRK, DMBIS);
		break;

	case TIOCCBRK:
		(void) dzmctl(sc, line, DML_BRK, DMBIC);
		break;

	case TIOCSDTR:
		(void) dzmctl(sc, line, DML_DTR, DMBIS);
		break;

	case TIOCCDTR:
		(void) dzmctl(sc, line, DML_DTR, DMBIC);
		break;

	case TIOCMSET:
		(void) dzmctl(sc, line, *(int *)data, DMSET);
		break;

	case TIOCMBIS:
		(void) dzmctl(sc, line, *(int *)data, DMBIS);
		break;

	case TIOCMBIC:
		(void) dzmctl(sc, line, *(int *)data, DMBIC);
		break;

	case TIOCMGET:
		*(int *)data = (dzmctl(sc, line, 0, DMGET) & ~DML_BRK);
		break;

	default:
		return (EPASSTHROUGH);
	}
	return (0);
}

struct tty *
dztty(dev_t dev)
{
	struct	dz_softc *sc = dz_cd.cd_devs[DZ_I2C(minor(dev))];
        struct tty *tp = sc->sc_dz[DZ_PORT(minor(dev))].dz_tty;

        return (tp);
}

/*ARGSUSED*/
void
dzstop(struct tty *tp, int flag)
{
	if (tp->t_state & TS_BUSY)
		if (!(tp->t_state & TS_TTSTOP))
			tp->t_state |= TS_FLUSH;
}

void
dzstart(struct tty *tp)
{
	struct dz_softc *sc;
	struct clist *cl;
	int unit, line, s;
	char state;

	unit = DZ_I2C(minor(tp->t_dev));
	line = DZ_PORT(minor(tp->t_dev));
	sc = dz_cd.cd_devs[unit];

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) {
		splx(s);
		return;
	}
	cl = &tp->t_outq;
	if (cl->c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((void *)cl);
		}
		selwakeup(&tp->t_wsel);
	}
	if (cl->c_cc == 0) {
		splx(s);
		return;
	}

	tp->t_state |= TS_BUSY;

	state = DZ_READ_WORD(dr_tcrw) & 255;
	if ((state & (1 << line)) == 0) {
		DZ_WRITE_BYTE(dr_tcr, state | (1 << line));
		DZ_BARRIER();
	}
	dzxint(sc);
	splx(s);
}

static int
dzparam(struct tty *tp, struct termios *t)
{
	struct	dz_softc *sc;
	int cflag = t->c_cflag;
	int unit, line;
	int ispeed = ttspeedtab(t->c_ispeed, dzspeedtab);
	int ospeed = ttspeedtab(t->c_ospeed, dzspeedtab);
	unsigned lpr;
	int s;

	unit = DZ_I2C(minor(tp->t_dev));
	line = DZ_PORT(minor(tp->t_dev));
	sc = dz_cd.cd_devs[unit];

	/* check requested parameters */
        if (ospeed < 0 || ispeed < 0 || ispeed != ospeed)
                return (EINVAL);

        tp->t_ispeed = t->c_ispeed;
        tp->t_ospeed = t->c_ospeed;
        tp->t_cflag = cflag;

	if (ospeed == 0) {
		(void) dzmctl(sc, line, 0, DMSET);	/* hang up line */
		return (0);
	}

	s = spltty();

	lpr = DZ_LPR_RX_ENABLE | ((ispeed&0xF)<<8) | line;

	switch (cflag & CSIZE)
	{
	  case CS5:
		lpr |= DZ_LPR_5_BIT_CHAR;
		break;
	  case CS6:
		lpr |= DZ_LPR_6_BIT_CHAR;
		break;
	  case CS7:
		lpr |= DZ_LPR_7_BIT_CHAR;
		break;
	  default:
		lpr |= DZ_LPR_8_BIT_CHAR;
		break;
	}
	if (cflag & PARENB)
		lpr |= DZ_LPR_PARENB;
	if (cflag & PARODD)
		lpr |= DZ_LPR_OPAR;
	if (cflag & CSTOPB)
		lpr |= DZ_LPR_2_STOP;

	DZ_WRITE_WORD(dr_lpr, lpr);
	DZ_BARRIER();

	(void) splx(s);
	return (0);
}

static unsigned
dzmctl(struct dz_softc *sc, int line, int bits, int how)
{
	unsigned status;
	unsigned mbits;
	unsigned bit;
	int s;

	s = spltty();

	mbits = 0;

	bit = (1 << line);

	/* external signals as seen from the port */

	status = DZ_READ_BYTE(dr_dcd) | sc->sc_dsr;

	if (status & bit)
		mbits |= DML_DCD;

	status = DZ_READ_BYTE(dr_ring);

	if (status & bit)
		mbits |= DML_RI;

	/* internal signals/state delivered to port */

	status = DZ_READ_BYTE(dr_dtr);

	if (status & bit)
		mbits |= DML_DTR;

	if (sc->sc_brk & bit)
		mbits |= DML_BRK;

	switch (how)
	{
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
		(void) splx(s);
		return (mbits);
	}

	if (mbits & DML_DTR) {
		DZ_WRITE_BYTE(dr_dtr, DZ_READ_BYTE(dr_dtr) | bit);
	} else {
		DZ_WRITE_BYTE(dr_dtr, DZ_READ_BYTE(dr_dtr) & ~bit);
	}

	if (mbits & DML_BRK) {
		sc->sc_brk |= bit;
		DZ_WRITE_BYTE(dr_break, sc->sc_brk);
	} else {
		sc->sc_brk &= ~bit;
		DZ_WRITE_BYTE(dr_break, sc->sc_brk);
	}

	DZ_BARRIER();
	(void) splx(s);
	return (mbits);
}

/*
 * This is called by timeout() periodically.
 * Check to see if modem status bits have changed.
 */
static void
dzscan(void *arg)
{
	struct dz_softc *sc;
	struct tty *tp;
	int n, bit, port;
	unsigned csr;
	int s;

	s = spltty();

	for (n = 0; n < dz_cd.cd_ndevs; n++) {

		if (dz_cd.cd_devs[n] == NULL)
			continue;

		sc = dz_cd.cd_devs[n];

		for (port = 0; port < sc->sc_type; port++) {

			tp = sc->sc_dz[port].dz_tty;
			bit = (1 << port);

			if ((DZ_READ_BYTE(dr_dcd) | sc->sc_dsr) & bit) {
				if (!(tp->t_state & TS_CARR_ON))
					(*tp->t_linesw->l_modem) (tp, 1);
			} else if ((tp->t_state & TS_CARR_ON) &&
			    (*tp->t_linesw->l_modem)(tp, 0) == 0) {
				DZ_WRITE_BYTE(dr_tcr,
				    (DZ_READ_WORD(dr_tcrw) & 255) & ~bit);
				DZ_BARRIER();
			}
	    	}

		/*
		 *  If the RX interrupt rate is this high, switch
		 *  the controller to Silo Alarm - which means don't
	 	 *  interrupt until the RX silo has 16 characters in
	 	 *  it (the silo is 64 characters in all).
		 *  Avoid oscillating SA on and off by not turning
		 *  if off unless the rate is appropriately low.
		 */

		csr = DZ_READ_WORD(dr_csr);

		if (sc->sc_rxint > (16*10)) {
			if ((csr & DZ_CSR_SAE) == 0)
				DZ_WRITE_WORD(dr_csr, csr | DZ_CSR_SAE);
	    	} else if ((csr & DZ_CSR_SAE) != 0)
			if (sc->sc_rxint < 10)
				DZ_WRITE_WORD(dr_csr, csr & ~(DZ_CSR_SAE));

		DZ_BARRIER();
		sc->sc_rxint = 0;
	}
	(void) splx(s);
	callout_reset(&dzscan_ch, hz, dzscan, NULL);
}

/*
 * Called after an ubareset. The DZ card is reset, but the only thing
 * that must be done is to start the receiver and transmitter again.
 * No DMA setup to care about.
 */
void
dzreset(struct device *dev)
{
	struct dz_softc *sc = (void *)dev;
	struct tty *tp;
	int i;

	for (i = 0; i < sc->sc_type; i++) {
		tp = sc->sc_dz[i].dz_tty;

		if (((tp->t_state & TS_ISOPEN) == 0) || (tp->t_wopen == 0))
			continue;

		dzparam(tp, &tp->t_termios);
		dzmctl(sc, i, DML_DTR, DMSET);
		tp->t_state &= ~TS_BUSY;
		dzstart(tp);	/* Kick off transmitter again */
	}
}

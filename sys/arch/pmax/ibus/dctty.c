/* $NetBSD: dctty.c,v 1.1.2.4 1999/11/20 06:28:21 nisimura Exp $ */

/*
 * Copyright (c) 1999 Tohru Nishimura.  All rights reserved.
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
 *      This product includes software developed by Tohru Nishimura
 *      for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 * DC7085 (DZ-11 look alike) tty interface
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dctty.c,v 1.1.2.4 1999/11/20 06:28:21 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/time.h>
#include <dev/cons.h>

#include <machine/bus.h>
#include <pmax/ibus/ibusvar.h>
#include <pmax/ibus/dc7085reg.h>
#include <pmax/ibus/dc7085var.h>

#include "locators.h"

int  dctty_match __P((struct device *, struct cfdata *, void *));
void dctty_attach __P((struct device *, struct device *, void *));

struct cfattach dctty_ca = {
	sizeof(struct device), dctty_match, dctty_attach
};

extern struct cfdriver dc_cd;			/* IMPORT */
extern struct speedtab dcspeedtab[];		/* IMPORT */
extern int dc_major;				/* IMPORT */

void dcstart __P((struct tty *));		/* EXPORT */
int  dcparam __P((struct tty *, struct termios *));
void dcmodem __P((struct dc_softc *, int, int));

int
dctty_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct dc_attach_args *args = aux;

	if (parent->dv_cfdata->cf_driver != &dc_cd)
		return 0;

	/* Exact match is better than wildcard. */
	if (cf->cf_loc[DCCF_LINE] == args->line)
		return 2;

	/* This driver accepts wildcard. */
	if (cf->cf_loc[DCCF_LINE] == DCCF_LINE_DEFAULT)
		return 1;

	return 0;
}
	
void
dctty_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct dc_softc *dc = (void *)parent;
	struct dc_attach_args *args = aux;
	struct tty *tp;
	int line;

	printf("\n");

	line = args->line;
	tp = ttymalloc();
	tp->t_dev = makedev(dc_major, DCMINOR(dc->sc_unit, line));
	tp->t_oproc = dcstart;
	tp->t_param = dcparam;
	tp->t_hwiflow = NULL;
	tty_attach(tp);
	dc->sc_tty[line] = tp;
}

cdev_decl(dc);

int
dcopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct dc_softc *sc;
	struct tty *tp;
	int unit, line;
	int error = 0, firstopen = 0;
	int s;

	unit = DCUNIT(dev);
	line = DCLINE(dev);
	if (unit >= dc_cd.cd_ndevs)
		return ENXIO;
	sc = dc_cd.cd_devs[unit];
	tp = sc->sc_tty[line];
	if (tp == NULL)
		return ENXIO;

	if ((tp->t_state & TS_ISOPEN) == 0 && tp->t_wopen == 0) {
		struct termios t;
		firstopen = 1;
		t.c_ispeed = t.c_ospeed = TTYDEF_SPEED;
		t.c_cflag = TTYDEF_CFLAG;
		tp->t_ospeed = 0; /* force to update "tp" */
		(void)dcparam(tp, &t);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		ttychars(tp);
		ttsetwater(tp);
		dcmodem(sc, line, 1);

	} else if ((tp->t_state & TS_XCLUDE) && curproc->p_ucred->cr_uid != 0)
		return EBUSY;

	s = spltty();
	/* if we're doing a blocking open... */
	if ((flag & O_NONBLOCK) == 0 && (tp->t_cflag & CLOCAL) == 0) {
		/* ...then wait for carrier */
		while ((tp->t_state & TS_CARR_ON) == 0) {
			tp->t_wopen++;
			error = ttysleep(tp, (caddr_t)&tp->t_rawq,
						TTIPRI | PCATCH, ttopen, 0);
			tp->t_wopen--;
			if (error) {
				/* interrupted while waiting for carrier */
				if ((tp->t_state & TS_ISOPEN) == 0) {
					dcmodem(sc, line, 0);
					ttwakeup(tp); /* ??? */
				}
				break;
			}
		}
	}
	splx(s);
	if (error == 0)
		return error;

	return (*linesw[tp->t_line].l_open)(dev, tp);
}

/*ARGSUSED*/
int
dcclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct dc_softc *sc;
	struct tty *tp;
	int line, s;

	sc = dc_cd.cd_devs[DCUNIT(dev)];
	line = DCLINE(dev);
	tp = sc->sc_tty[line];
	s = spltty();
	/* turn off the break condition if it is set */
	if (sc->dc_flags[line] & DC_CHIP_BREAK) {
		sc->dc_flags[line] &= ~DC_CHIP_BREAK;
		ttyoutput(0, tp);
	}
	splx(s);

	(*linesw[tp->t_line].l_close)(tp, flag);

	if ((tp->t_state & TS_ISOPEN) == 0 && tp->t_wopen == 0
			&& (tp->t_cflag & HUPCL)) {
		dcmodem(sc, line, 0);
		(void)tsleep(sc, TTIPRI, ttclos, hz);
	}
	return ttyclose(tp);
}

int
dcread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	struct dc_softc *sc;
	struct tty *tp;

	sc = dc_cd.cd_devs[DCUNIT(dev)];
	tp = sc->sc_tty[DCLINE(dev)];

#ifdef HW_FLOW_CONTROL			/* XXX t_hwiflow() routine XXX */
	if ((tp->t_cflag & CRTSCTS) && (tp->t_state & TS_TBLOCK) &&
	    tp->t_rawq.c_cc < TTYHOG/5) {
		tp->t_state &= ~TS_TBLOCK;
		(void)dcmctl(sc, DCLINE(dev), SML_RTS, DMBIS);
	}
#endif /* HW_FLOW_CONTROL */

	return (*linesw[tp->t_line].l_read)(tp, uio, flag);
}

int
dcwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	struct dc_softc *sc;
	struct tty *tp;

	sc = dc_cd.cd_devs[DCUNIT(dev)];
	tp = sc->sc_tty[DCLINE(dev)];
	return (*linesw[tp->t_line].l_write)(tp, uio, flag);
}

struct tty *
dctty(dev)
	dev_t dev;
{
	struct dc_softc *sc;

	sc = dc_cd.cd_devs[DCUNIT(dev)];
	return sc->sc_tty[DCLINE(dev)];
}

int
dcioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct dc_softc *sc;
	struct tty *tp;
	int unit, line, error;

	unit = DCUNIT(dev);
	line = DCLINE(dev);
	sc = dc_cd.cd_devs[unit];
	tp = sc->sc_tty[line];

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
		return error;
	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return error;

	switch (cmd) {

	case TIOCSBRK:
		sc->dc_flags[line] |= DC_CHIP_BREAK;
		ttyoutput(0, tp);
		break;

	case TIOCCBRK:
		sc->dc_flags[line] &= ~DC_CHIP_BREAK;
		ttyoutput(0, tp);
		break;

	case TIOCSDTR:
		dcmodem(sc, line, 1);
		break;

	case TIOCCDTR:
		dcmodem(sc, line, 0);
		break;

	case TIOCGFLAGS:
		/* 07 means SOFTCAR|CLOCAL|CRTSCTS */
		*(int *)data = sc->dc_flags[line] & 07;
		break;

	case TIOCSFLAGS:
		error = suser(p->p_ucred, &p->p_acflag);
		if (error)
			return error;
		sc->dc_flags[line] |= *(int *)data & 07;
		break;

	default:
		return ENOTTY;
	}
	return 0;
}

void
dcstart(tp)
	struct tty *tp;
{
	struct dc_softc *sc;
	struct clist *cl;
	int s, line, tcr;

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out;
	cl = &tp->t_outq;
	if (cl->c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup(&tp->t_outq);
			if (cl->c_cc == 0)
				goto out;
		}
		selwakeup(&tp->t_wsel);
	}
	if (cl->c_cc == 0)
		goto out;

	tp->t_state |= TS_BUSY;

	sc = dc_cd.cd_devs[DCUNIT(tp->t_dev)];
	tcr = bus_space_read_2(sc->sc_bst, sc->sc_bsh, DCTCR);
	tcr |= 1 << line;
	bus_space_write_2(sc->sc_bst, sc->sc_bsh, DCTCR, tcr);
out:
	splx(s);
}

/*ARGSUSED*/
void
dcstop(tp, flag)
	struct tty *tp;
	int flag;
{
	if ((tp->t_state & (TS_BUSY|TS_TTSTOP)) == TS_BUSY)
		tp->t_state |= TS_FLUSH;
}

int
dcparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
	struct dc_softc *sc;
	int ospeed, line, s;
	unsigned val;

	if (t->c_ispeed && t->c_ispeed != t->c_ospeed)
		return EINVAL;
#if notyet
	sc = dc_cd.cd_devs[DCUNIT(tp->t_dev)];
	if (t->c_ospeed == 19200 && sc->sc_chip38400)
		return EINVAL;
	if (t->c_ospeed == 38400 && !sc->sc_chip38400)
		return EINVAL;
#endif
	ospeed = ttspeedtab(t->c_ospeed, dcspeedtab);
	if (ospeed < 0)
		return EINVAL;
	if ((t->c_cflag & CSIZE) == CS5 || (t->c_cflag & CSIZE) == CS6)
		return EINVAL;

	line = DCLINE(tp->t_dev);
	if (sc->dc_flags[line] & TIOCFLAG_SOFTCAR) {
		t->c_cflag |= CLOCAL;
		t->c_cflag &= ~HUPCL;
	}
	if (sc->dc_flags[line] & TIOCFLAG_CLOCAL)
		t->c_cflag |= CLOCAL;
	if (sc->dc_flags[line] & TIOCFLAG_CRTSCTS)
		t->c_cflag |= CRTSCTS;
	/*
	 * If there were no changes, don't do anything.	 This avoids dropping
	 * input and improves performance when all we did was frob things like
	 * VMIN and VTIME.
	 */
	if (tp->t_ospeed == t->c_ospeed && tp->t_cflag == t->c_cflag)
		return 0;

	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	if (ospeed == 0) {
		dcmodem(sc, line, 0); /* ??? tsleep(9) ??? */
		return 0;
	}

	val = DC_RE | ospeed | line;
	val |= ((t->c_cflag & CSIZE) == CS7) ? DC_BITS7 : DC_BITS8;
	if (t->c_cflag & PARENB)
		val |= DC_PENABLE;
	if (t->c_cflag & PARODD)
		val |= DC_OPAR;
	if (t->c_cflag & CSTOPB)
		val |= DC_TWOSB;

	s = spltty();
	bus_space_write_2(sc->sc_bst, sc->sc_bsh, DCLPR, val);
	splx(s);
	DELAY(10);
	return 0;
}

#if 0
int dcmctl __P((struct dc_softc *, int, int, int));

/*
 * XXX Need REDO -- do model specific case analysis for 3100/5100/5000 XXX
 */
int
dcmctl(sc, line, bits, how)
	struct dc_softc *sc;
	int line, bits, how;
{
	struct dc7085reg *reg = sc->sc_reg;
	struct tty *tp = sc->sc_tty[line];
	int mbits, tcr, msr, s;

	if (line == 0 || line == 1 || (sc->dc_flags[line] & DC_HW_CLOCALS))
		return SML_DTR | SML_RTS | SML_DSR | SML_CAR;

	s = spltty();
	mbits = 0;
	switch (line) {
	case  2: /* pmax can detect only DSR, plus DCD/CTS/RI on 3max */
		tcr = reg->dctcr;
		if (tcr & DS3100_DC_L2_DTR)
			mbits |= SML_DTR;
		if ((tcr & DS3100_DC_L2_DSR) || (tp->t_cflag & CRTSCTS) == 0)
			mbits |= SML_RTS;
		msr = reg->dcmsr;
		if (msr & DS5000_DC_L2_CD)
			mbits |= SML_CAR;
		if (msr & DS5000_DC_L2_DSR) {
			mbits |= SML_DSR;
			if (tp->t_cflag & CLOCAL)
				mbits |= SML_CAR;
		}
		break;
	 case 3: /* no modem control on pmax, 3max detects DSR/CTS/DCD/RI */
		tcr = reg->dctcr;
		if (tcr & DS5000_DC_L3_DTR)
			mbits |= SML_DTR;
		if ((tcr & DS5000_DC_L3_RTS) || (tp->t_cflag & CRTSCTS) == 0)
			mbits |= SML_RTS;
		msr = reg->dcmsr;
		if (msr & DS5000_DC_L3_CD)
			mbits |= SML_CAR;
		if (msr & DS5000_DC_L3_DSR) {
			mbits |= SML_DSR;
			if (tp->t_cflag & CLOCAL)
				mbits |= SML_CAR;
		}
		break;
	}
	switch (how) {
	case DMSET:
		mbits = bits;	break;

	case DMBIS:
		mbits |= bits;	break;

	case DMBIC:
		mbits &= ~bits; break;

	case DMGET:
	default:
		splx(s);
		return mbits;
	}
	switch (line) {
	case  2: /* pmax can control only DTR, plus RTS on 3max */
		tcr = reg->dctcr;
		if (mbits & SML_DTR)
			tcr |= DS3100_DC_L2_DTR;
		else
			tcr &= ~DS3100_DC_L2_DTR;
		if (tp->t_cflag & CRTSCTS) {
			if (mbits & SML_RTS)
				tcr |= DS5100_DC_L2_RTS;
			else
				tcr &= ~DS5100_DC_L2_RTS;
		}
		reg->dctcr = tcr;
		break;
	case 3: /* pmax can control nothing, while 3max can do DTR/RTS */
		tcr = reg->dctcr;
		if (mbits & SML_DTR)
			tcr |= DS5000_DC_L3_DTR;
		else
			tcr &= ~DS5000_DC_L3_DTR;
		if (tp->t_cflag & CRTSCTS) {
			if (mbits & SML_RTS)
				tcr |= DS5000_DC_L3_RTS;
			else
				tcr &= ~DS5000_DC_L3_RTS;
		}
		reg->dctcr = tcr;
		break;
	}
	splx(s);
	return mbits;
}
#endif

#if defined(pmax)
#include <pmax/pmax/pmaxtype.h>

/*
 * Raise or lower modem control (DTR/RTS) signals.
 *			pmax		3max			mipsmate
 * line.2 detects	only DSR	DSR/CTS/DCD/RI		DSR/CTS/DCD/RI
 * line.3 detects	only DSR	DSR/CTS/DCD/RI		-???-
 *
 * line.2 sets		only DTR	only DTR		SS/DTR/RTS
 * line.3 sets		only DTR	DTR/RTS			-???-
 */
void
dcmodem(sc, line, onoff)
	struct dc_softc *sc;
	int line, onoff;
{
	int mbits, tcr, s;
	static u_int16_t ds5000[2] = {
		DS5000_DC_L2_DTR,
		DS5000_DC_L3_DTR|DS5000_DC_L3_RTS,
	};

	if (line == 0 || line == 1)
		return;

	mbits = 0;
	if (systype == DS_PMAX) {
		mbits = (line == 2) ? DS3100_DC_L2_DTR : DS3100_DC_L3_DTR;
	}
	if (systype == DS_MIPSMATE && line == 2) {
		mbits = DS5100_DC_L2_DTR|DS5100_DC_L2_RTS;
	}
	if (systype == DS_3MAX) {
		mbits = ds5000[line - 2];
	}
	if (mbits == 0)
		return;

	s = spltty();
	tcr = bus_space_read_2(sc->sc_bst, sc->sc_bsh, DCTCR);
	if (onoff)
		tcr |= mbits;
	else
		tcr &= ~mbits;
	bus_space_write_2(sc->sc_bst, sc->sc_bsh, DCTCR, tcr);
	splx(s);
}
#endif

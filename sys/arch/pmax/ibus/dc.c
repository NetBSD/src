/*	$NetBSD: dc.c,v 1.1.2.5 1999/01/07 06:32:04 nisimura Exp $ */

/*
 * DC7085 (DZ-11 look alike) quad asynchronous serial interface
 */

#include "opt_ddb.h"			/* XXX TBD XXX */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dc.c,v 1.1.2.5 1999/01/07 06:32:04 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/time.h>
#include <dev/cons.h>

#include <machine/autoconf.h>
#include <mips/locore.h>		/* XXX XXX XXX */
#include <pmax/ibus/ibusvar.h>
#include <pmax/ibus/dc7085reg.h>	/* XXX dev/dec/dc7085reg.h XXX */
#include <pmax/ibus/dc7085var.h>	/* XXX 'Think different(ly)' XXX */

extern struct cfdriver dc_cd;

static struct speedtab dcspeedtab[] = {
	{ 0,	0,	  },
	{ 50,	DC_B50	  },
	{ 75,	DC_B75	  },
	{ 110,	DC_B110	  },
	{ 134,	DC_B134_5 },
	{ 150,	DC_B150	  },
	{ 300,	DC_B300	  },
	{ 600,	DC_B600	  },
	{ 1200,	DC_B1200  },
	{ 1800,	DC_B1800  },
	{ 2400,	DC_B2400  },
	{ 4800,	DC_B4800  },
	{ 9600,	DC_B9600  },
	{ 19200, DC_B19200},
#ifdef notyet
	{ 38400, DC_B38400},	/* Overloaded with 19200, per chip. */
#endif
	{ -1,	-1 }
};

#include "dc.h"
#if NDC > 0

static void dcstart __P((struct tty *));
static int  dcparam __P((struct tty *, struct termios *));
static void dc_modem __P((struct dc_softc *, int, int));
static void dcttyrint __P((void *, int));
int  dcintr __P((void *));			/* EXPORT */
extern int dc_major;				/* IMPORT */

int
dcintr(v)
	void *v;
{
	struct dc_softc *sc = v;
	void (*input) __P((void *, int));
	struct tty *tp;
	int c, cc, line;
	unsigned csr;

	csr = sc->sc_reg->dccsr;
	if ((csr & (DC_RDONE | DC_TRDY)) == 0)
		return 0;
	do {	
		if (csr & DC_RDONE) {
			while ((c = sc->sc_reg->dcrbuf) < 0) {
				cc = c & 0xff;
				line = (c >> 8) & 3;
				input = sc->sc_input[line].f;
				if (input == NULL)
					continue;
				if (c & DC_FE)
					cc |= TTY_FE;
				if (c & DC_PE)
					cc |= TTY_PE;
				(*input)(sc->sc_input[line].a, cc);
			}
		}
		if (csr & DC_TRDY) {
			line = (csr >> 8) & 3;
			tp = sc->sc_tty[line];
			if (tp == NULL)
				continue;
			if (sc->sc_xmit[line].p < sc->sc_xmit[line].e) {
				sc->sc_reg->dctbuf = *sc->sc_xmit[line].p++;	
				wbflush();
				DELAY(10);
				continue;
			}
			tp->t_state &= ~TS_BUSY;
			if (tp->t_state & TS_FLUSH)
				tp->t_state &= ~TS_FLUSH;
			else {
				ndflush(&tp->t_outq,
				sc->sc_xmit[line].e - tp->t_outq.c_cf);
			}
			if (tp->t_line)
				(*linesw[tp->t_line].l_start)(tp);
			else
				dcstart(tp);
			if (tp->t_outq.c_cc == 0 || !(tp->t_state & TS_BUSY)) {
				sc->sc_reg->dctcr &= ~(1 << line);
				wbflush();
				DELAY(10);
			}
		}
	} while ((csr = sc->sc_reg->dccsr) & (DC_RDONE | DC_TRDY));
	return 1;
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
	if (unit >= dc_cd.cd_ndevs || line >= 4)
		return ENXIO;
	sc = dc_cd.cd_devs[unit];
	if ((tp = sc->sc_tty[line]) == NULL) {
		tp = sc->sc_tty[line] = ttymalloc();
		tty_attach(tp);
		sc->sc_input[line].f = dcttyrint;
		sc->sc_input[line].a = tp;
	}
	tp->t_oproc = dcstart;
	tp->t_param = dcparam;
	tp->t_dev = dev;
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
		dc_modem(sc, line, 1);

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
					dc_modem(sc, line, 0);
					ttwakeup(tp); /* ??? */
				}
				break;
			}
		}
	}
	splx(s);
	if (error == 0)
		error = (*linesw[tp->t_line].l_open)(dev, tp);

	return error;
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
	if (sc->dc_flags[line] & DC_CHIP_BRK) {
		sc->dc_flags[line] &= ~DC_CHIP_BRK;
		ttyoutput(0, tp);
	}
	splx(s);
	(*linesw[tp->t_line].l_close)(tp, flag);
	ttyclose(tp);

	if ((tp->t_state & TS_ISOPEN) == 0 && tp->t_wopen == 0
			&& (tp->t_cflag & HUPCL)) {
		dc_modem(sc, line, 0);
		(void)tsleep(sc, TTIPRI, ttclos, hz);
	}
	return 0;
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
		sc->dc_flags[line] |= DC_CHIP_BRK;
		ttyoutput(0, tp);
		break;

	case TIOCCBRK:
		sc->dc_flags[line] &= ~DC_CHIP_BRK;
		ttyoutput(0, tp);
		break;

	case TIOCSDTR:
		dc_modem(sc, line, 1);
		break;

	case TIOCCDTR:
		dc_modem(sc, line, 0);
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

static void
dcstart(tp)
	struct tty *tp;
{
	struct dc_softc *sc;
	int s, cc, line;

	sc = dc_cd.cd_devs[DCUNIT(tp->t_dev)];
	line = DCLINE(tp->t_dev);

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out;
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
			if (tp->t_outq.c_cc == 0)
				goto out;
		}
		selwakeup(&tp->t_wsel);
	}
  	cc = ndqb(&tp->t_outq, 0);
	if (cc == 0)
		goto out;

	sc->sc_xmit[line].p = tp->t_outq.c_cf;
	sc->sc_xmit[line].e = sc->sc_xmit[line].p + cc;
	tp->t_state |= TS_BUSY;
	sc->sc_reg->dctcr |= 1 << line;
out:
	splx(s);
}

/*ARGSUSED*/
void
dcstop(tp, flag)
	struct tty *tp;
	int flag;
{
	int s;

	s = spltty();
	if ((tp->t_state & (TS_BUSY|TS_TTSTOP)) == TS_BUSY)
		tp->t_state |= TS_FLUSH;
	splx(s);
}

static int
dcparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
	struct dc_softc *sc;
	int ospeed, line, s;
	unsigned val;

	if (t->c_ispeed && t->c_ispeed != t->c_ospeed)
		return EINVAL;
	ospeed = ttspeedtab(t->c_ospeed, dcspeedtab);
	if (ospeed < 0)
		return EINVAL;
	if ((t->c_cflag & CSIZE) == CS5 || (t->c_cflag & CSIZE) == CS6)
		return EINVAL;

	sc = dc_cd.cd_devs[DCUNIT(tp->t_dev)];
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
	 * If there were no changes, don't do anything.  This avoids dropping
	 * input and improves performance when all we did was frob things like
	 * VMIN and VTIME.
	 */
	if (tp->t_ospeed == t->c_ospeed && tp->t_cflag == t->c_cflag)
		return 0;

	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	if (ospeed == 0) {
		dc_modem(sc, line, 0); /* ??? tsleep(9) ??? */
		return 0;
	}

	val = DC_RE | ospeed | (1 << line);
	val |= ((t->c_cflag & CSIZE) == CS7) ? DC_BITS7 : DC_BITS8;
	if (t->c_cflag & PARODD)
		val |= DC_OPAR;
	if (t->c_cflag & CSTOPB)
		val |= DC_TWOSB;

	s = spltty();
	sc->sc_reg->dclpr = val;
	wbflush();
	splx(s);
	DELAY(10);
	return 0;
}

#if 0
int  dcmctl __P((struct dc_softc *, int, int, int));

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
		mbits &= ~bits;	break;

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

#if defined(__pmax__)
#include <pmax/pmax/pmaxtype.h>

/*
 * Raise or lower modem control (DTR/RTS) signals.
 *			pmax		3max			mipsmate
 * line.2 detects 	only DSR	DSR/CTS/DCD/RI		DSR/CTS/DCD/RI
 * line.3 detects	only DSR	DSR/CTS/DCD/RI		-???-
 *
 * line.2 sets		only DTR	only DTR		SS/DTR/RTS
 * line.3 sets		only DTR	DTR/RTS			-???-
 */
static void
dc_modem(sc, line, onoff)
	struct dc_softc *sc;
	int line, onoff;
{
	struct dc7085reg *reg = sc->sc_reg;
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

	tcr = reg->dctcr;
	switch (onoff) {
	case 1:
		tcr |= mbits;
		break;
	case 0:
		tcr &= ~mbits;
		break;
	}
	reg->dctcr = tcr;
	wbflush(); 

	splx(s);
}
#endif

static void
dcttyrint(v, cc)
	void *v;
	int cc;
{
	struct tty *tp = v;

	(*linesw[tp->t_line].l_rint)(cc, tp);
}

int	dcgetc __P((struct dc7085reg *, int));		/* EXPORT */
void	dcputc __P((struct dc7085reg *, int, int));	/* EXPORT */

int
dcgetc(reg, line)
	struct dc7085reg *reg;
	int line;
{
	int s;
	u_int16_t c;

	s = splhigh();
again:
	while ((reg->dccsr & DC_RDONE) == 0)
		DELAY(10);
	c = reg->dcrbuf;
	if ((c & (DC_PE | DC_FE | DC_DO)) || line != ((c >> 8) & 3))
		goto again;
	splx(s);
	
	return c & 0xff;
}

void
dcputc(reg, line, c)
	struct dc7085reg *reg;
	int line, c;
{
	int s, timeout = 1 << 15;

	s = splhigh();
	while ((reg->dccsr & DC_TRDY) == 0 && --timeout > 0)
		DELAY(100);
	if (line == ((reg->dccsr >> 8) & 3))
		reg->dctbuf = c & 0xff;
	else {
		u_int16_t tcr = reg->dctcr;
		reg->dctcr = (tcr & 0xff00) | (1 << line);
		wbflush();
		reg->dctbuf = c & 0xff;
		wbflush();
		reg->dctcr = tcr;
	}
	wbflush();
	splx(s);
}

tc_addr_t dc_cons_addr;		/* XXX */

static int  dc_cngetc __P((dev_t));
static void dc_cnputc __P((dev_t, int));
static void dc_cnpollc __P((dev_t, int));
int dc_cnattach __P((paddr_t, int, int, int));		/* EXPORT */

struct consdev dc_cons = {
	NULL, NULL, dc_cngetc, dc_cnputc, dc_cnpollc, NODEV, CN_NORMAL,
};

static int
dc_cngetc(dev)
	dev_t dev;
{
	return dcgetc((struct dc7085reg *)dc_cons_addr, minor(dev));
}

static void
dc_cnputc(dev, c)
	dev_t dev;
	int c;
{
	if (DCUNIT(dev) != 0)
		return;
	dcputc((struct dc7085reg *)dc_cons_addr, DCLINE(dev), c);
}

static void
dc_cnpollc(dev, onoff)
	dev_t dev;
	int onoff;
{
}

/*ARGSUSED*/
int
dc_cnattach(addr, line, rate, cflag)
	paddr_t addr;
	int line, rate, cflag;
{
	struct dc7085reg *reg;

	dc_cons_addr = (tc_addr_t)MIPS_PHYS_TO_KSEG1(addr); /* XXX */

	if (badaddr((caddr_t)dc_cons_addr, 2))
		return 1;

	reg = (void *)dc_cons_addr;
	reg->dccsr = DC_CLR;
	wbflush();
	reg->dctcr = line << 3;
	reg->dclpr = DC_RE | DC_B9600 | DC_BITS8 | (line << 3);
	reg->dccsr = DC_MSE | DC_RIE | DC_TIE;
	wbflush();

	cn_tab = &dc_cons;
	cn_tab->cn_pri = CN_REMOTE;
	cn_tab->cn_dev = makedev(dc_major, line);

	return 0;
}

#endif

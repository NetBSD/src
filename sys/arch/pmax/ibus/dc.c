/* $NetBSD: dc.c,v 1.1.2.2 1998/10/21 11:24:32 nisimura Exp $ */

/*
 * DC7085 (DZ-11 look alike) quad asynchronous serial interface
 */

#define	DML_RTS 0100 /* XXX stub XXX */
#define	DML_CAR 0200 /* XXX stub XXX */
#define	DML_DTR 0400 /* XXX stub XXX */
#define	DML_DSR 0010 /* XXX stub XXX */

#include "opt_ddb.h"

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
#include <mips/locore.h>
#include <pmax/ibus/ibusvar.h>
#include <pmax/ibus/dc7085reg.h> /* XXX dev/dec/dc7085reg.h XXX */

struct dc7085reg {
	u_int16_t	dz_csr;	/* Control and Status */
	unsigned : 16;
	unsigned : 32;
	u_int16_t	dz_xxx;	/* Rcv Buffer(R) or Line Parameter(W) */
	unsigned : 16;
	unsigned : 32;
	u_int16_t	dz_tcr;	/* Xmt Control (R/W) */
	unsigned : 16;
	unsigned : 32;
	u_int16_t	dz_yyy;	/* Xmit Buffer(W) or Modem Status(R) */
	unsigned : 16;
	unsigned : 8;
	u_int8_t	dz_brk;
};
#define dccsr		dz_csr
#define dcrbuf		dz_xxx
#define dclpr		dz_xxx
#define dctcr		dz_tcr
#define dctbuf		dz_yyy
#define dcmsr		dz_yyy
#define dcbrk		dz_brk

struct dc_softc {
	struct device sc_dv;
	struct dc7085reg *sc_reg;
	struct tty *dc_tty[4];
	u_int16_t dc_flags[4];
#define	DC_HW_CONSOLE	0x100
#define	DC_HW_KBD	0x200
#define	DC_HW_PTR	0x400
#define	DC_HW_DUMB	0x800
#define	DC_HW_CLOCALS	(DC_HW_CONSOLE|DC_HW_KBD|DC_HW_PTR|DC_HW_DUMB)
#define	DC_CHIP_BRK	0x080
#define	DC_CHIP_19200	0x040
	struct {
		u_char *p, *e;
	} sc_xmit[4];
};
#define DCUNIT(dev) (minor(dev) >> 2)
#define DCLINE(dev) (minor(dev) & 3)

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

int  dcmatch	__P((struct device *, struct cfdata *, void *));
void dcattach	__P((struct device *, struct device *, void *));

struct cfattach dc_ca = {
	sizeof(struct dc_softc), dcmatch, dcattach
};
extern struct cfdriver dc_cd;

void dcstart __P((struct tty *));
void dcstop __P((struct tty *, int));
int  dcparam __P((struct tty *, struct termios *));
int  dcintr __P((void *));
int  dcmctl __P((struct dc_softc *, int, int, int));

int dc_major = 16;

int
dcmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct ibus_attach_args *d = aux;

	if (strcmp(d->ia_name, "dc") != 0 &&
	    strcmp(d->ia_name, "mdc") != 0 &&
	    strcmp(d->ia_name, "dc7085") != 0)
		return (0);

	if (badaddr((caddr_t)d->ia_addr, 2))
		return (0);

	return (1);
}

void
dcattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ibus_attach_args *d = aux;
	struct dc_softc *sc = (struct dc_softc*)self;

	sc->sc_reg = (void *)d->ia_addr;

	ibus_intr_establish(parent, d->ia_cookie, IPL_TTY, dcintr, sc);

	printf(": DC7085\n");
}

int
dcintr(v)
	void *v;
{
	struct dc_softc *sc = v;
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
				tp = sc->dc_tty[line];
				if (tp == NULL)
					continue;
				if (c & DC_FE)
					cc |= TTY_FE;
				if (c & DC_PE)
					cc |= TTY_PE;
				(*linesw[tp->t_line].l_rint)(cc, tp);
			}
		}
		if (csr & DC_TRDY) {
			line = (csr >> 8) & 3;
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
		return (ENXIO);
	sc = dc_cd.cd_devs[unit];
	if ((tp = sc->dc_tty[line]) == NULL) {
		tp = sc->dc_tty[line] = ttymalloc();
		tty_attach(tp);
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

	} else if ((tp->t_state & TS_XCLUDE) && curproc->p_ucred->cr_uid != 0)
		return (EBUSY);

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
					(void)dcmctl(sc, line, 0, DMSET);
					ttwakeup(tp);
				}
				break;
			}
		}
	}
	splx(s);
	if (error == 0)
		error = (*linesw[tp->t_line].l_open)(dev, tp);

	return (error);
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
	tp = sc->dc_tty[line];
	s = spltty();
	/* turn off the break condition if it is set */
	if (sc->dc_flags[line] & DC_CHIP_BRK) {
		sc->dc_flags[line] &= ~DC_CHIP_BRK;
		ttyoutput(0, tp);
	}
	splx(s);
	(*linesw[tp->t_line].l_close)(tp, flag);
	if ((tp->t_cflag & HUPCL)
	    || tp->t_wopen || (tp->t_state & TS_ISOPEN) == 0)
		(void)dcmctl(sc, line, 0, DMSET);
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
	tp = sc->dc_tty[DCLINE(dev)];

#ifdef HW_FLOW_CONTROL
	if ((tp->t_cflag & CRTSCTS) && (tp->t_state & TS_TBLOCK) &&
	    tp->t_rawq.c_cc < TTYHOG/5) {
		tp->t_state &= ~TS_TBLOCK;
		(void)dcmctl(sc, DCLINE(dev), DML_RTS, DMBIS);
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
	tp = sc->dc_tty[DCLINE(dev)];
	return (*linesw[tp->t_line].l_write)(tp, uio, flag);
}

struct tty *
dctty(dev)
        dev_t dev;
{
	struct dc_softc *sc;
	struct tty *tp;

	sc = dc_cd.cd_devs[DCUNIT(dev)];
	tp = sc->dc_tty[DCLINE(dev)];
        return (tp);
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
	tp = sc->dc_tty[line];

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);

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
		(void)dcmctl(sc, line, DML_DTR|DML_RTS, DMBIS);
		break;

	case TIOCCDTR:
		(void)dcmctl(sc, line, DML_DTR|DML_RTS, DMBIC);
		break;

	case TIOCMSET:
		(void)dcmctl(sc, line, *(int *)data, DMSET);
		break;

	case TIOCMBIS:
		(void)dcmctl(sc, line, *(int *)data, DMBIS);
		break;

	case TIOCMBIC:
		(void)dcmctl(sc, line, *(int *)data, DMBIC);
		break;

	case TIOCMGET:
		*(int *)data = dcmctl(sc, line, 0, DMGET);
		break;

	case TIOCGFLAGS:
		*(int *)data = sc->dc_flags[line];
		break;

	case TIOCSFLAGS:
                error = suser(p->p_ucred, &p->p_acflag);
                if (error)
                        return (error);
		/* 0x7 means SOFTCAR|CLOCAL|CRTSCTS */
		sc->dc_flags[line] |= *(int *)data & 0x7;
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}

void
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
{
	int s;

	s = spltty();
	if (tp->t_state & TS_BUSY) {
		if ((tp->t_state & TS_TTSTOP) == 0)
			tp->t_state |= TS_FLUSH;
	}
	splx(s);
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
		return (EINVAL);
	ospeed = ttspeedtab(t->c_ospeed, dcspeedtab);
	if (ospeed < 0)
		return (EINVAL);
	if ((t->c_cflag & CSIZE) == CS5 || (t->c_cflag & CSIZE) == CS6)
		return (EINVAL);

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
		return (0);

	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	if (ospeed == 0) {
		(void)dcmctl(sc, line, 0, DMSET);
		return (0);
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
	return (0);
}

/*
 * XXX do model specific case analysis for 3100/5100/5000 XXX
 */
int
dcmctl(sc, line, bits, how)
	struct dc_softc *sc;
	int line, bits, how;
{
	struct dc7085reg *reg = sc->sc_reg;
	struct tty *tp = sc->dc_tty[line];
	int mbits, tcr, msr, s;

	mbits = DML_DTR | DML_RTS | DML_DSR | DML_CAR;

	s = spltty();
	switch (line) {
	case  2: /* pmax can detect only DSR, plus DCD/CTS/RI on 3max */
		mbits = 0;
		tcr = reg->dctcr;
		if (tcr & DS3100_DC_L2_DTR)
			mbits |= DML_DTR;
		if ((tcr & DS3100_DC_L2_DSR) || (tp->t_cflag & CRTSCTS) == 0)
			mbits |= DML_RTS;
		msr = reg->dcmsr;
		if (msr & DS5000_DC_L2_CD)
			mbits |= DML_CAR;
		if (msr & DS5000_DC_L2_DSR) {
			mbits |= DML_DSR;
			if (tp->t_cflag & CLOCAL)
				mbits |= DML_CAR;
		}
		break;
	case 3: /* no modem control on pmax, 3max detects DSR/CTS/DSR/DCD/RI */
		if (sc->dc_flags[3] & DC_HW_DUMB)
			break;
		mbits = 0;
		tcr = reg->dctcr;
		if (tcr & DS5000_DC_L3_DTR)
			mbits |= DML_DTR;
		if ((tcr & DS5000_DC_L3_RTS) || (tp->t_cflag & CRTSCTS) == 0)
			mbits |= DML_RTS;
		msr = reg->dcmsr;
		if (msr & DS5000_DC_L3_CD)
			mbits |= DML_CAR;
		if (msr & DS5000_DC_L3_DSR) {
			mbits |= DML_DSR;
			if (tp->t_cflag & CLOCAL)
				mbits |= DML_CAR;
		}
		break;
	}
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
		splx(s);
		return (mbits);
	}
	switch (line) {
	case  2: /* pmax can control only DTR, plus RTS on 3max */
		tcr = reg->dctcr;
		if (mbits & DML_DTR)
			tcr |= DS3100_DC_L2_DTR;
		else
			tcr &= ~DS3100_DC_L2_DTR;
		if (tp->t_cflag & CRTSCTS) {
			if (mbits & DML_RTS)
				tcr |= DS5100_DC_L2_RTS;
			else
				tcr &= ~DS5100_DC_L2_RTS;
		}
		reg->dctcr = tcr;
		break;
	case 3: /* pmax can control nothing, while 3max can do DTR/RTS */
		if (sc->dc_flags[3] & DC_HW_DUMB)
			break;
		tcr = reg->dctcr;
		if (mbits & DML_DTR)
			tcr |= DS5000_DC_L3_DTR;
		else
			tcr &= ~DS5000_DC_L3_DTR;
		if (tp->t_cflag & CRTSCTS) {
			if (mbits & DML_RTS)
				tcr |= DS5000_DC_L3_RTS;
			else
				tcr &= ~DS5000_DC_L3_RTS;
		}
		reg->dctcr = tcr;
		break;
	}
	splx(s);
	return (mbits);
}

int	dcgetc __P((struct dc7085reg *, int));
void	dcputc __P((struct dc7085reg *, int, int));

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

tc_addr_t dc_cons_addr;

int	dc_cngetc __P((dev_t));
void	dc_cnputc __P((dev_t, int));
void	dc_cnpollc __P((dev_t, int));
int	dc_cnattach __P((tc_addr_t, int, int, int));
void	dc_ws_cnattach __P((tc_addr_t));

struct consdev dc_cons = {
	NULL, NULL, dc_cngetc, dc_cnputc, dc_cnpollc, NODEV, CN_NORMAL,
};

int
dc_cngetc(dev)
	dev_t dev;
{
	return dcgetc((struct dc7085reg *)dc_cons_addr, minor(dev));
}

void
dc_cnputc(dev, c)
	dev_t dev;
	int c;
{
	if (DCUNIT(dev) != 0)
		return;
	dcputc((struct dc7085reg *)dc_cons_addr, DCLINE(dev), c);
}

void
dc_cnpollc(dev, onoff)
	dev_t dev;
	int onoff;
{
}

/*ARGSUSED*/
int
dc_cnattach(addr, line, rate, cflag)
	tc_addr_t addr;
	int line, rate, cflag;
{
	struct dc7085reg *reg;

	dc_cons_addr = (tc_addr_t)MIPS_PHYS_TO_KSEG1(addr);

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

/* XXX XXX XXX */
void
dc_ws_cnattach(addr)
	tc_addr_t addr;
{
	/* Tie lk201 keyboard with line 0 */
}
/* XXX XXX XXX */
#endif

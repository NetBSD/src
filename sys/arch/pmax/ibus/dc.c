/* $Id: dc.c,v 1.1.2.1 1998/10/15 02:41:15 nisimura Exp $ */
/* $NetBSD: dc.c,v 1.1.2.1 1998/10/15 02:41:15 nisimura Exp $ */

/*
 * DC7085 (DZ-11 look alike) quad serial driver
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

/* Control status register definitions (dccsr) */
#define DC_OFF		0x00		/* Modem control off		*/
#define DC_MAINT	0x08		/* Maintenance			*/
#define DC_CLR		0x10		/* Reset dc7085 chip		*/
#define DC_MSE		0x20		/* Master Scan Enable		*/
#define DC_RIE		0x40		/* Receive IE */
#define DC_RDONE	0x80		/* Receiver done		*/
#define DC_TIE		0x4000		/* Trasmit IE */
#define DC_TRDY		0x8000		/* Transmit ready		*/

/* Line parameter register definitions (dclpr) */
#define DC_BITS5	0x00		/* 5 bit char width		*/
#define DC_BITS6	0x08		/* 6 bit char width		*/
#define DC_BITS7	0x10		/* 7 bit char width		*/
#define DC_BITS8	0x18		/* 8 bit char width		*/
#define DC_TWOSB	0x20		/* two stop bits		*/
#define DC_PENABLE	0x40		/* parity enable		*/
#define DC_OPAR		0x80		/* odd parity			*/
#define DC_B50		0x000		/* 50 BPS speed			*/
#define DC_B75		0x100		/* 75 BPS speed			*/
#define DC_B110		0x200		/* 110 BPS speed		*/
#define DC_B134_5	0x300		/* 134.5 BPS speed		*/
#define DC_B150		0x400		/* 150 BPS speed		*/
#define DC_B300		0x500		/* 300 BPS speed		*/
#define DC_B600		0x600		/* 600 BPS speed		*/
#define DC_B1200	0x700		/* 1200 BPS speed		*/
#define DC_B1800	0x800		/* 1800 BPS speed		*/
#define DC_B2000	0x900		/* 2000 BPS speed		*/
#define DC_B2400	0xa00		/* 2400 BPS speed		*/
#define DC_B3600	0xb00		/* 3600 BPS speed		*/
#define DC_B4800	0xc00		/* 4800 BPS speed		*/
#define DC_B7200	0xd00		/* 7200 BPS speed		*/
#define DC_B9600	0xe00		/* 9600 BPS speed		*/
#define DC_B19200	0xf00		/* 19200 BPS speed		*/
#define DC_B38400	0xf00		/* 38400 BPS speed - see LED2	*/
#define DC_RE		0x1000		/* Receive enable		*/

/* Transmit Control Register (dctcr) */
#define DC_TCR_EN_0	0x1		/* enable transmit on line 0	*/
#define DC_TCR_EN_1	0x2		/* enable transmit on line 1	*/
#define DC_TCR_EN_2	0x4		/* enable transmit on line 2	*/
#define DC_TCR_EN_3	0x8		/* enable transmit on line 3	*/

/* CPU specific Transmit Control Register definitions */
#define DS3100_DC_L2_DTR	0x0400	/* pmax DTR on line 2		*/
#define DS3100_DC_L2_DSR	0x0200	/* pmax DSR on line 2		*/
#define DS3100_DC_L2_XMIT	DS3100_DC_L2_DSR  /* All modem leads ready */

#define DS3100_DC_L3_DTR	0x0800	/* pmax DTR on line 3		*/
#define DS3100_DC_L3_DSR	0x0001	/* pmax DSR on line 3		*/
#define DS3100_DC_L3_XMIT	DS3100_DC_L3_DSR  /* All modem leads ready */

#define DS5000_DC_L2_DTR	0x0400	/* 3max DTR on line 2		*/

#define DS5000_DC_L3_DTR	0x0100	/* 3max DTR on line 3		*/
#define DS5000_DC_L3_RTS	0x0200	/* 3max RTS on line 3		*/

#define DS5100_DC_L2_SS		0x0200	/* mipsmate SS on line 2	*/
#define DS5100_DC_L2_DTR	0x0400	/* mipsmate DTR on line 2	*/
#define DS5100_DC_L2_RTS	0x0800	/* mipsmate RTS on line 2	*/

/* CPU specific Modem Status Register definitions */
#define DS3100_DC_L2_SS		0x0200	/* mipsmate ss on line 2	*/

#define DS5000_DC_L2_CTS	0x0100	/* 3max CTS on line 2		*/
#define DS5000_DC_L2_DSR	0x0200	/* 3max DSR on line 2		*/
#define DS5000_DC_L2_CD		0x0400	/* 3max CD on line 2		*/
#define DS5000_DC_L2_RI		0x0800	/* 3max RI on line 2		*/
#define DS5000_DC_L2_XMIT	(DS5000_DC_L2_CTS| DS5000_DC_L2_DSR| DS5000_DC_L2_CD)					/* All modem leads ready	*/

#define DS5000_DC_L3_CTS	0x0001	/* 3max CTS on line 3		*/
#define DS5000_DC_L3_DSR	0x0002	/* 3max DSR on line 3		*/
#define DS5000_DC_L3_CD		0x0004	/* 3max CD on line 3		*/
#define DS5000_DC_L3_RI		0x0008	/* 3max RI on line 3		*/
#define DS5000_DC_L3_XMIT	(DS5000_DC_L3_CTS| DS5000_DC_L3_DSR| DS5000_DC_L3_CD)					/* All modem leads ready	*/

#define DS5100_DC_L2_CTS	0x0001	/* mipsmate CTS on line 2	*/
#define DS5100_DC_L2_DSR	0x0002	/* mipsmate DSR on line 2	*/
#define DS5100_DC_L2_CD		0x0004	/* mipsmate CD on line 2	*/
#define DS5100_DC_L2_RI		0x0008	/* mipsmate CD on line 2	*/
#define DS5100_DC_L2_SI		0x0080	/* mipsmate CD on line 2	*/
#define DS5100_DC_L2_XMIT	(DS5100_DC_L2_CTS| DS5100_DC_L2_DSR| DS5100_DC_L2_CD)					/* All modem leads ready	*/

/* Receiver buffer register definitions (dcrbuf) */
#define DC_PE		0x1000		/* Parity error			*/
#define DC_FE		0x2000		/* Framing error		*/
#define DC_DO		0x4000		/* Data overrun error		*/
#define DC_DVAL		0x8000		/* Receive buffer data valid	*/

/* Line control status definitions (dclcs) */
#define DC_SR		0x08		/* Secondary Receive		*/
#define DC_CTS		0x10		/* Clear To Send		*/
#define DC_CD		0x20		/* Carrier Detect		*/
#define DC_RI		0x40		/* Ring Indicate		*/
#define DC_DSR		0x80		/* Data Set Ready		*/
#define DC_LE		0x100		/* Line Enable			*/
#define DC_DTR		0x200		/* Data Terminal Ready		*/
#define DC_BRK		0x400		/* Break			*/
#define DC_ST		0x800		/* Secondary Transmit		*/
#define DC_RTS		0x1000		/* Request To Send		*/

/* DM lsr definitions */
#define SML_LE		0x01		/* Line enable			*/
#define SML_DTR		0x02		/* Data terminal ready		*/
#define SML_RTS		0x04		/* Request to send		*/
#define SML_ST		0x08		/* Secondary transmit		*/
#define SML_SR		0x10		/* Secondary receive		*/
#define SML_CTS		0x20		/* Clear to send		*/
#define SML_CAR		0x40		/* Carrier detect		*/
#define SML_RNG		0x80		/* Ring				*/
#define SML_DSR		0x100		/* Data set ready, not DM bit	*/

/* Line Prameter Register bits */
#define SER_KBD		000000
#define SER_POINTER	000001
#define SER_COMLINE	000002
#define SER_PRINTER	000003
#define SER_CHARW	000030
#define SER_STOP	000040
#define SER_PARENB	000100
#define SER_ODDPAR	000200
#define SER_SPEED	006000
#define SER_RXENAB	010000

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

tc_addr_t dc_cons_addr;
int	dc_cons_line;

int	dc_cngetc __P((dev_t));
void	dc_cnputc __P((dev_t, int));
void	dc_cnpollc __P((dev_t, int));
void	dc_cnattach __P((tc_addr_t, tc_offset_t, int, int, int));

struct consdev dc_cons = {
	NULL, NULL, dc_cngetc, dc_cnputc, dc_cnpollc, NODEV, CN_NORMAL,
};

int
dc_cngetc(dev)
	dev_t dev;
{
	int s;
	u_int16_t c;
	struct dc7085reg *reg = (void *)dc_cons_addr;

	s = splhigh();
again:
	while ((reg->dccsr & DC_RDONE) == 0)
		DELAY(10);
	c = reg->dcrbuf;
	if ((c & (DC_PE | DC_FE | DC_DO)) || dc_cons_line != ((c >> 8) & 3))
		goto again;
	splx(s);
	
	return c & 0xff;
}

void
dc_cnputc(dev, c)
	dev_t dev;
	int c;
{
	int s, timeout;
	struct dc7085reg *reg = (void *)dc_cons_addr;

	s = splhigh();

	timeout = 1 << 15;
	while ((reg->dccsr & DC_TRDY) == 0 && --timeout > 0)
		DELAY(100);
	if (dc_cons_line == ((reg->dccsr >> 8) & 3))
		reg->dctbuf = c & 0xff;
	else {
		u_int16_t tcr = reg->dctcr;
		reg->dctcr = (tcr & 0xff00) | (1 << dc_cons_line);
		wbflush();
		reg->dctbuf = c & 0xff;
		wbflush();
		reg->dctcr = tcr;
	}
	wbflush();

	splx(s);
}

void
dc_cnpollc(dev, onoff)
	dev_t dev;
	int onoff;
{
}

/*ARGSUSED*/
void
dc_cnattach(addr, offset, line, rate, cflag)
	tc_addr_t addr;
	tc_offset_t offset;
	int line, rate, cflag;
{
	struct dc7085reg *reg;

	dc_cons_addr = MIPS_PHYS_TO_KSEG1(addr + offset);
	dc_cons_line = line;

	reg = (void *)dc_cons_addr;
	reg->dccsr = DC_CLR;
	wbflush();
	reg->dctcr = line << 3;
	reg->dclpr = DC_RE | DC_B9600 | DC_BITS8 | (line << 3);
	reg->dccsr = DC_MSE | DC_RIE | DC_TIE;
	wbflush();

	cn_tab = &dc_cons;
}
#endif

/*	$NetBSD: dc.c,v 1.1.2.7 1999/04/17 11:32:38 nisimura Exp $ */

/*
 * DC7085 (DZ-11 look alike) quad asynchronous serial interface
 */

#include "opt_ddb.h"			/* XXX TBD XXX */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dc.c,v 1.1.2.7 1999/04/17 11:32:38 nisimura Exp $");

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

#include <machine/cpu.h>
#include <machine/bus.h>

#include <pmax/ibus/ibusvar.h>
#include <pmax/ibus/dc7085reg.h>	/* XXX dev/dec/dc7085reg.h XXX */

					/* XXX machine/dc7085var.h XXX */
struct dc7085reg {	
	u_int16_t	dz_csr;	/* control and status */
	unsigned :16; unsigned :32;
	u_int16_t	dz_xxx;	/* rcv buffer(R) or line parameter(W) */
	unsigned :16; unsigned :32;
	u_int16_t	dz_tcr;	/* xmit control (R/W) */	
	unsigned :16; unsigned :32;
	u_int16_t	dz_yyy; /* xmit buffer(W) or modem status(R) */
	unsigned :16; unsigned :8;
	u_int8_t	dz_brk;
};
#define DCCSR	offsetof(struct dc7085reg, dz_csr)
#define DCRBUF	offsetof(struct dc7085reg, dz_xxx)
#define DCLPR	offsetof(struct dc7085reg, dz_xxx)
#define DCTCR	offsetof(struct dc7085reg, dz_tcr)
#define DCTBUF	offsetof(struct dc7085reg, dz_yyy)
#define DCMSR	offsetof(struct dc7085reg, dz_yyy)
#define DCBRK	offsetof(struct dc7085reg, dz_brk)
#define	dccsr	dz_csr
#define	dcrbuf	dz_xxx
#define	dclpr	dz_xxx
#define	dctcr	dz_tcr
#define	dctbuf	dz_yyy
#define	dcmsr	dz_yyy
#define	dcbrk	dz_brk

#define DCUNIT(dev) (minor(dev) >> 2)
#define DCLINE(dev) (minor(dev) & 3)
#define	DCMINOR(u,l) ((u)<<2)|(l)

#define	LKKBD	0
#define	VSMSE	1
#define	DCCOM	2
#define	DCPRT	3

struct dc_softc {
	struct device sc_dv;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	int sc_unit;
	int sc_chip38400;

	struct tty *sc_tty[4];
#define	DC_CHIP_BREAK	0x010
	int dc_flags[4];

	/* XXX XXX XXX */
	u_int16_t dc_rbuf[256];
	int dc_rbget, dc_rbput;
	struct {
		void (*f) __P((void *, int));
		void *a;
	} sc_line[4];
	struct {
		u_char *p, *e;
	} sc_xmit[4];
	/* XXX XXX XXX */
};
#define	DC_RX_RING_MASK 255

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

int dcintr __P((void *));			/* EXPORT */
int dcsoftintr __P((struct dc_softc *));	/* EXPORT */
caddr_t dc_cons_addr;				/* EXPORT */
extern int dc_major;				/* IMPORT */
extern int softisr;				/* IMPORT */

static void dcstart __P((struct tty *));
static int  dcparam __P((struct tty *, struct termios *));
static void dcmodem __P((struct dc_softc *, int, int));

int
dcintr(v)
	void *v;
{
	struct dc_softc *sc = v;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	int dc_softreq, c, line, put, put_next;
	struct tty *tp;
	unsigned csr;

	csr = bus_space_read_2(bst, bsh, DCCSR);
	if ((csr & (DC_RDONE | DC_TRDY)) == 0)
		return 0;

	dc_softreq = 0;
	do {
		if (csr & DC_RDONE) {
			c = bus_space_read_2(bst, bsh, DCRBUF);
			while (c & 0x8000) {
				sc->dc_rbuf[put] = c & 0x3ff;
				put_next = (put + 1) & DC_RX_RING_MASK;
				if (put_next == sc->dc_rbget)
					continue; /* overflow */
				sc->dc_rbput = put = put_next;
				c = bus_space_read_2(bst, bsh, DCRBUF);
			}
			dc_softreq = 1;
		}
		if (csr & DC_TRDY) {
			line = (csr >> 8) & 3;
			if (sc->sc_xmit[line].p < sc->sc_xmit[line].e) {
				c = *sc->sc_xmit[line].p++;	
				bus_space_write_2(bst, bsh, DCTBUF, c);
				DELAY(10);
				continue;
			}
			tp = sc->sc_tty[line];
			if (tp == NULL)
				continue;
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
				c = bus_space_read_2(bst, bsh, DCTCR);
				c &= ~(1 << line);
				bus_space_write_2(bst, bsh, DCTCR, c);
				DELAY(10);
			}
		}
		csr = bus_space_read_2(bst, bsh, DCCSR);
	} while (csr & (DC_RDONE | DC_TRDY));

	if (dc_softreq == 1) {
		softisr |= 1 << sc->sc_unit;
		_setsoftintr(MIPS_SOFT_INT_MASK_1);
	}
	return 1;
}

int
dcsoftintr(sc)
	struct dc_softc *sc;
{
	int c, cc, line, get, s;
	void (*input) __P((void *, int));

	/* Atomically get and clear flags. */
	s = splhigh();
#if 0
	intr_flags = sc->dc_intr_flags;
	sc->dc_intr_flags = 0;
#endif
	/* Now lower to spltty for the rest. */
	(void)spltty();
	get = sc->dc_rbget;
	while (get != sc->dc_rbput) {
		c = sc->dc_rbuf[get];
		get = (get + 1) & DC_RX_RING_MASK;
		line = (c >> 8) & 3;
		input = sc->sc_line[line].f;
		if (input == NULL)
			continue;
		cc = c & 0xff;
		if (c & DC_FE)
			cc |= TTY_FE;
		if (c & DC_PE)
			cc |= TTY_PE;
		(*input)(sc->sc_tty[line], cc);
	}
	splx(s);
	return 1;
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
	return dcgetc((struct dc7085reg *)dc_cons_addr, DCLINE(dev));
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
	struct dc7085reg *reg;
	int csr;

	reg = (void *)dc_cons_addr;
	csr = DC_MSE;
	if (onoff)
		csr |= DC_RIE | DC_TIE;
	else
		csr &= ~(DC_RIE | DC_TIE);
	reg->dccsr = csr;
	wbflush();
}

/*ARGSUSED*/
int
dc_cnattach(addr, line, rate, cflag)
	paddr_t addr;
	int line, rate, cflag;
{
	struct dc7085reg *reg;
	int speed;

	dc_cons_addr = (caddr_t)MIPS_PHYS_TO_KSEG1(addr); /* XXX */

	if (badaddr((caddr_t)dc_cons_addr, 2))
		return 1;

	speed = ttspeedtab(rate, dcspeedtab);

	reg = (void *)dc_cons_addr;
	reg->dccsr = DC_CLR;
	wbflush();
	reg->dctcr = line << 3;
	reg->dclpr = DC_RE | speed | DC_BITS8 | (line << 3);
	reg->dccsr = DC_MSE;
	wbflush();

	cn_tab = &dc_cons;
	cn_tab->cn_pri = CN_REMOTE;
	cn_tab->cn_dev = makedev(dc_major, line);

	return 0;
}


/* XXX XXX dctty.c XXX XXX */

#define	DCCF_LINE 0
#define	DCCF_LINE_DEFAULT -1

struct dc_attach_args {
	int line;
};

int  dctty_match __P((struct device *, struct cfdata *, void *));
void dctty_attach __P((struct device *, struct device *, void *));

struct cfattach dctty_ca = {
	sizeof(struct device), dctty_match, dctty_attach
};

int
dctty_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
#define	CFNAME(cf) ((cf)->dv_cfdata->cf_driver->cd_name)
	struct dc_attach_args *args = aux;

	if (strcmp(CFNAME(parent), "dc") != 0)
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
	if (unit >= dc_cd.cd_ndevs || line >= 4)
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
	if (sc->dc_flags[line] & DC_CHIP_BREAK) {
		sc->dc_flags[line] &= ~DC_CHIP_BREAK;
		ttyoutput(0, tp);
	}
	splx(s);
	(*linesw[tp->t_line].l_close)(tp, flag);
	ttyclose(tp);

	if ((tp->t_state & TS_ISOPEN) == 0 && tp->t_wopen == 0
			&& (tp->t_cflag & HUPCL)) {
		dcmodem(sc, line, 0);
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
	tp = sc->sc_line[DCLINE(dev)].a;

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
	tp = sc->sc_line[DCLINE(dev)].a;
	return (*linesw[tp->t_line].l_write)(tp, uio, flag);
}

struct tty *
dctty(dev)
        dev_t dev;
{
	struct dc_softc *sc;

	sc = dc_cd.cd_devs[DCUNIT(dev)];
        return sc->sc_line[DCLINE(dev)].a;
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

static void
dcstart(tp)
	struct tty *tp;
{
	struct dc_softc *sc;
	int s, cc, line, tcr;

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

	sc = dc_cd.cd_devs[DCUNIT(tp->t_dev)];
	line = DCLINE(tp->t_dev);
	sc->sc_xmit[line].p = tp->t_outq.c_cf;
	sc->sc_xmit[line].e = sc->sc_xmit[line].p + cc;

	tp->t_state |= TS_BUSY;
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
		dcmodem(sc, line, 0); /* ??? tsleep(9) ??? */
		return 0;
	}

	val = DC_RE | ospeed | (1 << line);
	val |= ((t->c_cflag & CSIZE) == CS7) ? DC_BITS7 : DC_BITS8;
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

#if defined(pmax)
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

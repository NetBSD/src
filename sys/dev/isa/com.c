/*	$NetBSD: com.c,v 1.53 1995/04/28 00:34:08 hpeyerl Exp $	*/

/*-
 * Copyright (c) 1993, 1994, 1995 Charles Hannum.  All rights reserved.
 * Copyright (c) 1991 The Regents of the University of California.
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
 *
 *	@(#)com.c	7.5 (Berkeley) 5/16/91
 */

/*
 * COM driver, based on HP dca driver
 * uses National Semiconductor NS16450/NS16550AF UART
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/pio.h>

#include <dev/isa/isavar.h>
#include <dev/isa/comreg.h>
#include <dev/ic/ns16550.h>

struct com_softc {
	struct device sc_dev;
	void *sc_ih;
	struct tty *sc_tty;

	int sc_overflows;
	int sc_iobase;
	u_char sc_hwflags;
#define	COM_HW_NOIEN	0x01
#define	COM_HW_FIFO	0x02
#define	COM_HW_CONSOLE	0x40
	u_char sc_swflags;
#define	COM_SW_SOFTCAR	0x01
#define	COM_SW_CLOCAL	0x02
#define	COM_SW_CRTSCTS	0x04
#define	COM_SW_MDMBUF	0x08
	u_char sc_msr, sc_mcr;
};

int comprobe __P((struct device *, void *, void *));
void comattach __P((struct device *, struct device *, void *));
int comopen __P((dev_t, int, int, struct proc *));
int comclose __P((dev_t, int, int, struct proc *));
void comdiag __P((void *));
int comintr __P((void *));
int comparam __P((struct tty *, struct termios *));
void comstart __P((struct tty *));

struct cfdriver comcd = {
	NULL, "com", comprobe, comattach, DV_TTY, sizeof(struct com_softc)
};

int	comdefaultrate = TTYDEF_SPEED;
#ifdef COMCONSOLE
int	comconsole = COMCONSOLE;
#else
int	comconsole = -1;
#endif
int	comconsinit;
int	commajor;

#ifdef KGDB
#include <machine/remote-sl.h>
extern int kgdb_dev;
extern int kgdb_rate;
extern int kgdb_debug_init;
#endif

#define	COMUNIT(x)	(minor(x))

#define	bis(c, b)	do { const register int com_ad = (c); \
			     outb(com_ad, inb(com_ad) | (b)); } while(0)
#define	bic(c, b)	do { const register int com_ad = (c); \
			     outb(com_ad, inb(com_ad) & ~(b)); } while(0)

int
comspeed(speed)
	long speed;
{
#define	divrnd(n, q)	(((n)*2/(q)+1)/2)	/* divide and round off */

	int x, err;

	if (speed == 0)
		return 0;
	if (speed < 0)
		return -1;
	x = divrnd((COM_FREQ / 16), speed);
	if (x <= 0)
		return -1;
	err = divrnd((COM_FREQ / 16) * 1000, speed * x) - 1000;
	if (err < 0)
		err = -err;
	if (err > COM_TOLERANCE)
		return -1;
	return x;

#undef	divrnd(n, q)
}

int
comprobe1(iobase)
	int iobase;
{

	/* force access to id reg */
	outb(iobase + com_cfcr, 0);
	outb(iobase + com_iir, 0);
	if (inb(iobase + com_iir) & 0x38)
		return 0;

	return 1;
}

int
comprobe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct isa_attach_args *ia = aux;
	int iobase = ia->ia_iobase;

	if (!comprobe1(iobase))
		return 0;

	ia->ia_iosize = COM_NPORTS;
	ia->ia_msize = 0;
	return 1;
}

void
comattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct com_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
	struct cfdata *cf = sc->sc_dev.dv_cfdata;
	int iobase = ia->ia_iobase;
	struct tty *tp;

	sc->sc_iobase = iobase;
	sc->sc_hwflags = cf->cf_flags & COM_HW_NOIEN;
	sc->sc_swflags = 0;

	if (sc->sc_dev.dv_unit == comconsole)
		delay(1000);

	/* look for a NS 16550AF UART with FIFOs */
	outb(iobase + com_fifo,
	    FIFO_ENABLE | FIFO_RCV_RST | FIFO_XMT_RST | FIFO_TRIGGER_14);
	delay(100);
	if ((inb(iobase + com_iir) & IIR_FIFO_MASK) == IIR_FIFO_MASK)
		if ((inb(iobase + com_fifo) & FIFO_TRIGGER_14) == FIFO_TRIGGER_14) {
			sc->sc_hwflags |= COM_HW_FIFO;
			printf(": ns16550a, working fifo\n");
		} else
			printf(": ns82550 or ns16550, broken fifo\n");
	else
		printf(": ns82450 or ns16450, no fifo\n");
	outb(iobase + com_fifo, 0);

	/* disable interrupts */
	outb(iobase + com_ier, 0);
	outb(iobase + com_mcr, 0);

	if (ia->ia_irq != IRQUNK)
		sc->sc_ih = isa_intr_establish(ia->ia_irq, ISA_IST_EDGE,
		    ISA_IPL_TTY, comintr, sc);

#ifdef KGDB
	if (kgdb_dev == makedev(commajor, unit)) {
		if (comconsole == unit)
			kgdb_dev = -1;	/* can't debug over console port */
		else {
			(void) cominit(unit, kgdb_rate);
			if (kgdb_debug_init) {
				/*
				 * Print prefix of device name,
				 * let kgdb_connect print the rest.
				 */
				printf("%s: ", sc->sc_dev.dv_xname);
				kgdb_connect(1);
			} else
				printf("%s: kgdb enabled\n",
				    sc->sc_dev.dv_xname);
		}
	}
#endif

	if (sc->sc_dev.dv_unit == comconsole) {
		/*
		 * Need to reset baud rate, etc. of next print so reset
		 * comconsinit.  Also make sure console is always "hardwired".
		 */
		comconsinit = 0;
		sc->sc_hwflags |= COM_HW_CONSOLE;
		sc->sc_swflags |= COM_SW_SOFTCAR;
	}
}

int
comopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int unit = COMUNIT(dev);
	struct com_softc *sc;
	int iobase;
	struct tty *tp;
	int s;
	int error = 0;
 
	if (unit >= comcd.cd_ndevs)
		return ENXIO;
	sc = comcd.cd_devs[unit];
	if (!sc)
		return ENXIO;

	s = spltty();

	if (!sc->sc_tty)
		tp = sc->sc_tty = ttymalloc();
	else
		tp = sc->sc_tty;

	tp->t_oproc = comstart;
	tp->t_param = comparam;
	tp->t_dev = dev;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_state |= TS_WOPEN;
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		if (sc->sc_swflags & COM_SW_CLOCAL)
			tp->t_cflag |= CLOCAL;
		if (sc->sc_swflags & COM_SW_CRTSCTS)
			tp->t_cflag |= CRTSCTS;
		if (sc->sc_swflags & COM_SW_MDMBUF)
			tp->t_cflag |= MDMBUF;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = comdefaultrate;
		comparam(tp, &tp->t_termios);
		ttsetwater(tp);

		iobase = sc->sc_iobase;
		/* Set the FIFO threshold based on the receive speed. */
		if (sc->sc_hwflags & COM_HW_FIFO)
			outb(iobase + com_fifo,
			    FIFO_ENABLE | FIFO_RCV_RST | FIFO_XMT_RST |
			    (tp->t_ispeed <= 1200 ? FIFO_TRIGGER_1 : FIFO_TRIGGER_8));
		/* flush any pending I/O */
		(void) inb(iobase + com_lsr);
		(void) inb(iobase + com_data);
		/* you turn me on, baby */
		sc->sc_mcr = MCR_DTR | MCR_RTS;
		if (!(sc->sc_hwflags & COM_HW_NOIEN))
			sc->sc_mcr |= MCR_IENABLE;
		outb(iobase + com_mcr, sc->sc_mcr);
		outb(iobase + com_ier,
		    IER_ERXRDY | IER_ETXRDY | IER_ERLS | IER_EMSC);

		sc->sc_msr = inb(iobase + com_msr);
		if (sc->sc_swflags & COM_SW_SOFTCAR || sc->sc_msr & MSR_DCD ||
		    tp->t_cflag & MDMBUF)
			tp->t_state |= TS_CARR_ON;
		else
			tp->t_state &= ~TS_CARR_ON;
	} else if (tp->t_state&TS_XCLUDE && p->p_ucred->cr_uid != 0) {
		splx(s);
		return EBUSY;
	}

	/* wait for carrier if necessary */
	if ((flag & O_NONBLOCK) == 0)
		while ((tp->t_cflag & CLOCAL) == 0 &&
		    (tp->t_state & TS_CARR_ON) == 0) {
			tp->t_state |= TS_WOPEN;
			error = ttysleep(tp, (caddr_t)&tp->t_rawq, 
			    TTIPRI | PCATCH, ttopen, 0);
			if (error) {
				/* XXX should turn off chip if we're the
				   only waiter */
				splx(s);
				return error;
			}
		}
	splx(s);

	return (*linesw[tp->t_line].l_open)(dev, tp);
}
 
int
comclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int unit = COMUNIT(dev);
	struct com_softc *sc = comcd.cd_devs[unit];
	struct tty *tp = sc->sc_tty;
	int iobase = sc->sc_iobase;

	(*linesw[tp->t_line].l_close)(tp, flag);
#ifdef KGDB
	/* do not disable interrupts if debugging */
	if (kgdb_dev != makedev(commajor, unit))
#endif
	{
		bic(iobase + com_cfcr, CFCR_SBREAK);
		outb(iobase + com_ier, 0);
		if (tp->t_cflag & HUPCL &&
		    (sc->sc_swflags & COM_SW_SOFTCAR) == 0)
			/* XXX perhaps only clear DTR */
			outb(iobase + com_mcr, 0);
	}
	ttyclose(tp);
#ifdef notyet /* XXXX */
	if (unit != comconsole) {
		ttyfree(tp);
		sc->sc_tty = 0;
	}
#endif
	return 0;
}
 
int
comread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct com_softc *sc = comcd.cd_devs[COMUNIT(dev)];
	struct tty *tp = sc->sc_tty;
 
	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}
 
int
comwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct com_softc *sc = comcd.cd_devs[COMUNIT(dev)];
	struct tty *tp = sc->sc_tty;
 
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

struct tty *
comtty(dev)
	dev_t dev;
{
	struct com_softc *sc = comcd.cd_devs[COMUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return (tp);
}
 
static u_char
tiocm_xxx2mcr(data)
	int data;
{
	u_char m = 0;

	if (data & TIOCM_DTR)
		m |= MCR_DTR;
	if (data & TIOCM_RTS)
		m |= MCR_RTS;
	return m;
}

int
comioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	int unit = COMUNIT(dev);
	struct com_softc *sc = comcd.cd_devs[unit];
	struct tty *tp = sc->sc_tty;
	int iobase = sc->sc_iobase;
	int error;

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
		return error;
	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return error;

	switch (cmd) {
	case TIOCSBRK:
		bis(iobase + com_cfcr, CFCR_SBREAK);
		break;
	case TIOCCBRK:
		bic(iobase + com_cfcr, CFCR_SBREAK);
		break;
	case TIOCSDTR:
		outb(iobase + com_mcr, sc->sc_mcr |= (MCR_DTR | MCR_RTS));
		break;
	case TIOCCDTR:
		outb(iobase + com_mcr, sc->sc_mcr &= ~(MCR_DTR | MCR_RTS));
		break;
	case TIOCMSET:
		sc->sc_mcr &= ~(MCR_DTR | MCR_RTS);
	case TIOCMBIS:
		outb(iobase + com_mcr,
		    sc->sc_mcr |= tiocm_xxx2mcr(*(int *)data));
		break;
	case TIOCMBIC:
		outb(iobase + com_mcr,
		    sc->sc_mcr &= ~tiocm_xxx2mcr(*(int *)data));
		break;
	case TIOCMGET: {
		u_char m;
		int bits = 0;

		m = sc->sc_mcr;
		if (m & MCR_DTR)
			bits |= TIOCM_DTR;
		if (m & MCR_RTS)
			bits |= TIOCM_RTS;
		m = sc->sc_msr;
		if (m & MSR_DCD)
			bits |= TIOCM_CD;
		if (m & MSR_CTS)
			bits |= TIOCM_CTS;
		if (m & MSR_DSR)
			bits |= TIOCM_DSR;
		if (m & (MSR_RI | MSR_TERI))
			bits |= TIOCM_RI;
		if (inb(iobase + com_ier))
			bits |= TIOCM_LE;
		*(int *)data = bits;
		break;
	}
	case TIOCGFLAGS: {
		int bits = 0;

		if (sc->sc_swflags & COM_SW_SOFTCAR)
			bits |= TIOCFLAG_SOFTCAR;
		if (sc->sc_swflags & COM_SW_CLOCAL)
			bits |= TIOCFLAG_CLOCAL;
		if (sc->sc_swflags & COM_SW_CRTSCTS)
			bits |= TIOCFLAG_CRTSCTS;
		if (sc->sc_swflags & COM_SW_MDMBUF)
			bits |= TIOCFLAG_MDMBUF;

		*(int *)data = bits;
		break;
	}
	case TIOCSFLAGS: {
		int userbits, driverbits = 0;

		error = suser(p->p_ucred, &p->p_acflag); 
		if (error != 0)
			return(EPERM); 

		userbits = *(int *)data;
		if ((userbits & TIOCFLAG_SOFTCAR) ||
		    (sc->sc_hwflags & COM_HW_CONSOLE))
			driverbits |= COM_SW_SOFTCAR;
		if (userbits & TIOCFLAG_CLOCAL)
			driverbits |= COM_SW_CLOCAL;
		if (userbits & TIOCFLAG_CRTSCTS)
			driverbits |= COM_SW_CRTSCTS;
		if (userbits & TIOCFLAG_MDMBUF)
			driverbits |= COM_SW_MDMBUF;

		sc->sc_swflags = driverbits;
		break;
	}
	default:
		return ENOTTY;
	}

	return 0;
}

int
comparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
	struct com_softc *sc = comcd.cd_devs[COMUNIT(tp->t_dev)];
	int iobase = sc->sc_iobase;
	int ospeed = comspeed(t->c_ospeed);
	u_char cfcr;
	int s;

	/* check requested parameters */
	if (ospeed < 0 || (t->c_ispeed && t->c_ispeed != t->c_ospeed))
		return EINVAL;

	switch (t->c_cflag & CSIZE) {
	case CS5:
		cfcr = CFCR_5BITS;
		break;
	case CS6:
		cfcr = CFCR_6BITS;
		break;
	case CS7:
		cfcr = CFCR_7BITS;
		break;
	case CS8:
		cfcr = CFCR_8BITS;
		break;
	}
	if (t->c_cflag & PARENB) {
		cfcr |= CFCR_PENAB;
		if ((t->c_cflag & PARODD) == 0)
			cfcr |= CFCR_PEVEN;
	}
	if (t->c_cflag & CSTOPB)
		cfcr |= CFCR_STOPB;

	s = spltty();

	if (ospeed == 0)
		outb(iobase + com_mcr, sc->sc_mcr &= ~MCR_DTR);

	/*
	 * Set the FIFO threshold based on the receive speed, if we are
	 * changing it.
	 *
	 * XXX
	 * It would be better if we waited for the FIFO to empty, so we don't
	 * lose any in-transit characters.
	 */
	if (tp->t_ispeed != t->c_ispeed) {
		if (sc->sc_hwflags & COM_HW_FIFO)
			outb(iobase + com_fifo,
			    FIFO_ENABLE | FIFO_RCV_RST | FIFO_XMT_RST |
			    (t->c_ispeed <= 1200 ? FIFO_TRIGGER_1 : FIFO_TRIGGER_8));
	}

	outb(iobase + com_cfcr, cfcr | CFCR_DLAB);
	outb(iobase + com_dlbl, ospeed);
	outb(iobase + com_dlbh, ospeed>>8);
	outb(iobase + com_cfcr, cfcr);

	if (ospeed != 0)
		outb(iobase + com_mcr, sc->sc_mcr |= MCR_DTR);

	/* When not using CRTSCTS, RTS follows DTR. */
	if ((t->c_cflag & CRTSCTS) == 0) {
		if (sc->sc_mcr & MCR_DTR) {
			if ((sc->sc_mcr & MCR_RTS) == 0)
				outb(iobase + com_mcr, sc->sc_mcr |= MCR_RTS);
		} else {
			if (sc->sc_mcr & MCR_RTS)
				outb(iobase + com_mcr, sc->sc_mcr &= ~MCR_RTS);
		}
	}

	/*
	 * If CTS is off and CRTSCTS is changed, we must toggle TS_TTSTOP.
	 * XXX should be done at tty layer.
	 */
	if ((sc->sc_msr & MSR_CTS) == 0 &&
	    (tp->t_cflag & CRTSCTS) != (t->c_cflag & CRTSCTS)) {
		if ((t->c_cflag & CRTSCTS) == 0) {
			tp->t_state &= ~TS_TTSTOP;
			(*linesw[tp->t_line].l_start)(tp);
		} else
			tp->t_state |= TS_TTSTOP;
	}

	/*
	 * If DCD is off and MDMBUF is changed, we must toggle TS_TTSTOP.
	 * XXX should be done at tty layer.
	 */
	if ((sc->sc_swflags & COM_SW_SOFTCAR) == 0 &&
	    (sc->sc_msr & MSR_DCD) == 0 &&
	    (tp->t_cflag & MDMBUF) != (t->c_cflag & MDMBUF)) {
		if ((t->c_cflag & MDMBUF) == 0) {
			tp->t_state &= ~TS_TTSTOP;
			(*linesw[tp->t_line].l_start)(tp);
		} else
			tp->t_state |= TS_TTSTOP;
	}

	/* and copy to tty */
	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	splx(s);
	return 0;
}

void
comstart(tp)
	struct tty *tp;
{
	struct com_softc *sc = comcd.cd_devs[COMUNIT(tp->t_dev)];
	int iobase = sc->sc_iobase;
	int s;

	s = spltty();
	if (tp->t_state & (TS_TTSTOP | TS_BUSY))
		goto out;
#if 0 /* XXXX I think this is handled adequately by commint() and comparam(). */
	if (tp->t_cflag & CRTSCTS && (sc->sc_mcr & MSR_CTS) == 0)
		goto out;
#endif
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
	}
	if (tp->t_outq.c_cc == 0)
		goto out;
	tp->t_state |= TS_BUSY;
	if ((inb(iobase + com_lsr) & LSR_TXRDY) == 0)
		goto out;
	if (sc->sc_hwflags & COM_HW_FIFO) {
		u_char buffer[16], *cp = buffer;
		int n = q_to_b(&tp->t_outq, cp, sizeof buffer);
		do {
			outb(iobase + com_data, *cp++);
		} while (--n);
	} else
		outb(iobase + com_data, getc(&tp->t_outq));
out:
	splx(s);
}

/*
 * Stop output on a line.
 */
void
comstop(tp, flag)
	struct tty *tp;
{
	int s;

	s = spltty();
	if (tp->t_state & TS_BUSY)
		if ((tp->t_state & TS_TTSTOP) == 0)
			tp->t_state |= TS_FLUSH;
	splx(s);
}

static inline void
comeint(sc, stat)
	struct com_softc *sc;
	int stat;
{
	struct tty *tp = sc->sc_tty;
	int iobase = sc->sc_iobase;
	int c;

	c = inb(iobase + com_data);
	if ((tp->t_state & TS_ISOPEN) == 0) {
#ifdef KGDB
		/* we don't care about parity errors */
		if (((stat & (LSR_BI | LSR_FE | LSR_PE)) == LSR_PE) &&
		    kgdb_dev == makedev(commajor, unit) && c == FRAME_END)
			kgdb_connect(0); /* trap into kgdb */
#endif
		return;
	}
#ifdef COMCONSOLE
	if ((stat & LSR_BI) && (sc->sc_dev.dv_unit == comconsole))
		Debugger();
#endif
	if (stat & (LSR_BI | LSR_FE))
		c |= TTY_FE;
	else if (stat & LSR_PE)
		c |= TTY_PE;
	if (stat & LSR_OE) {
		if (sc->sc_overflows++ == 0)
			timeout(comdiag, sc, 60 * hz);
	}
	/* XXXX put in FIFO and process later */
	(*linesw[tp->t_line].l_rint)(c, tp);
}
 
static inline void
commint(sc)
	struct com_softc *sc;
{
	struct tty *tp = sc->sc_tty;
	int iobase = sc->sc_iobase;
	u_char msr, delta;

	msr = inb(iobase + com_msr);
	delta = msr ^ sc->sc_msr;
	sc->sc_msr = msr;

	if (delta & MSR_DCD && (sc->sc_swflags & COM_SW_SOFTCAR) == 0) {
		if (msr & MSR_DCD)
			(void)(*linesw[tp->t_line].l_modem)(tp, 1);
		else if ((*linesw[tp->t_line].l_modem)(tp, 0) == 0)
			outb(iobase + com_mcr,
			    sc->sc_mcr &= ~(MCR_DTR | MCR_RTS));
	}
	if (delta & MSR_CTS && tp->t_cflag & CRTSCTS) {
		/* the line is up and we want to do rts/cts flow control */
		if (msr & MSR_CTS) {
			tp->t_state &= ~TS_TTSTOP;
			(*linesw[tp->t_line].l_start)(tp);
		} else
			tp->t_state |= TS_TTSTOP;
	}
}

void
comdiag(arg)
	void *arg;
{
	struct com_softc *sc = arg;
	int overflows;
	int s;

	s = spltty();
	overflows = sc->sc_overflows;
	sc->sc_overflows = 0;
	splx(s);

	if (overflows)
		log(LOG_WARNING, "%s: %d silo overflow%s\n",
		    sc->sc_dev.dv_xname, overflows, overflows == 1 ? "" : "s");
}

int
comintr(arg)
	void *arg;
{
	struct com_softc *sc = arg;
	int iobase = sc->sc_iobase;
	struct tty *tp;
	u_char code;

	code = inb(iobase + com_iir) & IIR_IMASK;
	if (code & IIR_NOPEND)
		return 0;

	for (;;) {
		if (code & IIR_RXRDY) {
			tp = sc->sc_tty;
			/* XXXX put in FIFO and process later */
			while (code = (inb(iobase + com_lsr) & LSR_RCV_MASK)) {
				if (code & LSR_RXRDY) {
					code = inb(iobase + com_data);
					if (tp->t_state & TS_ISOPEN)
						(*linesw[tp->t_line].l_rint)(code, tp);
#ifdef KGDB
					else {
						if (kgdb_dev == makedev(commajor, unit) &&
						    code == FRAME_END)
							kgdb_connect(0);
					}
#endif
				} else
					comeint(sc, code);
			}
		} else if (code == IIR_TXRDY) {
			tp = sc->sc_tty;
			tp->t_state &= ~TS_BUSY;
			if (tp->t_state & TS_FLUSH)
				tp->t_state &= ~TS_FLUSH;
			else
				if (tp->t_line)
					(*linesw[tp->t_line].l_start)(tp);
				else
					comstart(tp);
		} else if (code == IIR_MLSC) {
			commint(sc);
		} else {
			log(LOG_WARNING, "%s: weird interrupt: iir=0x%02x\n",
			    sc->sc_dev.dv_xname, code);
		}
		code = inb(iobase + com_iir) & IIR_IMASK;
		if (code & IIR_NOPEND)
			return 1;
	}
}

/*
 * Following are all routines needed for COM to act as console
 */
#include <dev/cons.h>

void
comcnprobe(cp)
	struct consdev *cp;
{

	if (!comprobe1(CONADDR)) {
		cp->cn_pri = CN_DEAD;
		return;
	}

	/* locate the major number */
	for (commajor = 0; commajor < nchrdev; commajor++)
		if (cdevsw[commajor].d_open == comopen)
			break;

	/* initialize required fields */
	cp->cn_dev = makedev(commajor, CONUNIT);
#ifdef	COMCONSOLE
	cp->cn_pri = CN_REMOTE;		/* Force a serial port console */
#else
	cp->cn_pri = CN_NORMAL;
#endif
}

void
comcninit(cp)
	struct consdev *cp;
{

	cominit(CONUNIT, comdefaultrate);
	comconsole = CONUNIT;
	comconsinit = 0;
}

cominit(unit, rate)
	int unit, rate;
{
	int s = splhigh();
	int iobase = CONADDR;
	u_char stat;

	outb(iobase + com_cfcr, CFCR_DLAB);
	rate = comspeed(comdefaultrate);
	outb(iobase + com_dlbl, rate);
	outb(iobase + com_dlbh, rate >> 8);
	outb(iobase + com_cfcr, CFCR_8BITS);
	outb(iobase + com_ier, IER_ERXRDY | IER_ETXRDY);
	outb(iobase + com_fifo, FIFO_ENABLE | FIFO_RCV_RST | FIFO_XMT_RST | FIFO_TRIGGER_4);
	stat = inb(iobase + com_iir);
	splx(s);
}

comcngetc(dev)
	dev_t dev;
{
	int s = splhigh();
	int iobase = CONADDR;
	u_char stat, c;

	while (((stat = inb(iobase + com_lsr)) & LSR_RXRDY) == 0)
		;
	c = inb(iobase + com_data);
	stat = inb(iobase + com_iir);
	splx(s);
	return c;
}

/*
 * Console kernel output character routine.
 */
void
comcnputc(dev, c)
	dev_t dev;
	int c;
{
	int s = splhigh();
	int iobase = CONADDR;
	u_char stat;
	register int timo;

#ifdef KGDB
	if (dev != kgdb_dev)
#endif
	if (comconsinit == 0) {
		(void) cominit(COMUNIT(dev), comdefaultrate);
		comconsinit = 1;
	}
	/* wait for any pending transmission to finish */
	timo = 50000;
	while (((stat = inb(iobase + com_lsr)) & LSR_TXRDY) == 0 && --timo)
		;
	outb(iobase + com_data, c);
	/* wait for this transmission to complete */
	timo = 1500000;
	while (((stat = inb(iobase + com_lsr)) & LSR_TXRDY) == 0 && --timo)
		;
	/* clear any interrupts generated by this transmission */
	stat = inb(iobase + com_iir);
	splx(s);
}

void
comcnpollc(dev, on)
	dev_t dev;
	int on;
{

}

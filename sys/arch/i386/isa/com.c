/*-
 * Copyright (c) 1993 Charles Hannum.
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
 *	from: @(#)com.c	7.5 (Berkeley) 5/16/91
 *	$Id: com.c,v 1.12.2.16 1993/10/29 19:59:09 mycroft Exp $
 */

/*
 * COM driver, based originally on HP dca driver
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

#include <i386/isa/isavar.h>
#include <i386/isa/icu.h>
#include <i386/isa/comreg.h>

struct com_softc {
	struct	device sc_dev;
	struct	isadev sc_id;
	struct	intrhand sc_ih;

	u_short	sc_iobase;
	u_char	sc_flags;
#define	COM_SOFTCAR	0x01
#define	COM_FIFO	0x02
	u_char	sc_bufn;
	u_char	*sc_buf;
};
/* XXXX should be in com_softc, but not ready for that yet */
#include "com.h"
struct	tty *com_tty[NCOM];

int	comdefaultrate = TTYDEF_SPEED;

static int comprobe __P((struct device *, struct cfdata *, void *));
static void comforceintr __P((void *));
static void comattach __P((struct device *, struct device *, void *));
static int comintr __P((struct com_softc *));

struct cfdriver comcd =
{ NULL, "com", comprobe, comattach, DV_TTY, sizeof (struct com_softc) };

int comparam __P((struct tty *, struct termios *));
void comstart __P((struct tty *));

int	comconsole = -1;
int	comconsinit;
int	commajor;
extern	struct tty *constty;

#define	COMUNIT(x)		(minor(x))

#define	bis(c, b)	do { const register u_short com_ad = (c); \
			     outb(com_ad, inb(com_ad) | (b)); } while(0)
#define	bic(c, b)	do { const register u_short com_ad = (c); \
			     outb(com_ad, inb(com_ad) &~ (b)); } while(0)

static int
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

static int
_comprobe(iobase)
	u_short iobase;
{

	/* force access to id reg */
	outb(iobase + com_cfcr, 0);
	outb(iobase + com_iir, 0);
	if (inb(iobase + com_iir) & 0x38)
		return 0;
	outb(iobase + com_ier, 0);
	return 1;
}

static int
comprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct	isa_attach_args *ia = aux;
	u_short	iobase = ia->ia_iobase;

	if (iobase == IOBASEUNK)
		return 0;

	if (!_comprobe(iobase))
		return 0;

	if (ia->ia_irq == IRQUNK) {
		ia->ia_irq = isa_discoverintr(comforceintr, aux);
		if (ia->ia_irq == IRQNONE)
			return 0;
	}

	/* disable interrupts */
	outb(iobase + com_ier, 0);
	outb(iobase + com_mcr, 0);

	ia->ia_iosize = COM_NPORTS;
	ia->ia_drq = DRQUNK;
	ia->ia_msize = 0;
	return 1;
}

static void
comforceintr(aux)
	void *aux;
{
	struct	isa_attach_args *ia = aux;
	u_short	iobase = ia->ia_iobase;

#if 0
	/*
	 * As usual, the PC compatible world isn't.  We'd like to use the
	 * loopback feature to generate an interrupt, but alas, some lame
	 * clones don't support it.
	 */
	outb(iobase + com_mcr, MCR_IENABLE | MCR_LOOPBACK);
	outb(iobase + com_ier, IER_EMSC);
	outb(iobase + com_mcr, MCR_IENABLE | MCR_LOOPBACK | MCR_DRS);
	outb(iobase + com_mcr, MCR_IENABLE | MCR_LOOPBACK);
#else
	/*
	 * So instead we try to force the transmit buffer empty (though
	 * it probably is already) and get a transmit interrupt.
	 */
	outb(iobase + com_cfcr, CFCR_8BITS);	/* turn off DLAB */
	outb(iobase + com_ier, 0);
	outb(iobase + com_fifo, 0);
	outb(iobase + com_lsr, LSR_TXRDY | LSR_TSRE);
	outb(iobase + com_mcr, MCR_IENABLE);
	outb(iobase + com_ier, IER_ETXRDY);
#endif
}

static void
comattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct	com_softc *sc = (struct com_softc *)self;
	struct	isa_attach_args *ia = aux;
	u_short	iobase = ia->ia_iobase;
	struct	tty *tp;
	u_char	unit = sc->sc_dev.dv_unit;

	if (iobase == comconsole)
		delay(1000);

	sc->sc_iobase = iobase;
	sc->sc_flags = 0;

	/* look for a NS 16550AF UART with FIFOs */
	outb(iobase + com_fifo,
	     FIFO_ENABLE | FIFO_RCV_RST | FIFO_XMT_RST | FIFO_TRIGGER_14);
	delay(100);
	if ((inb(iobase + com_iir) & IIR_FIFO_MASK) == IIR_FIFO_MASK)
		if ((inb(iobase + com_fifo) & FIFO_TRIGGER_14) == FIFO_TRIGGER_14) {
			sc->sc_flags |= COM_FIFO;
			printf(": ns16550a, working fifo\n");
		} else
			printf(": ns82550 or ns16550, broken fifo\n");
	else
		printf(": ns82450 or ns16450, no fifo\n");
	outb(iobase + com_fifo, 0);
	isa_establish(&sc->sc_id, &sc->sc_dev);

	outb(iobase + com_ier, 0);
	outb(iobase + com_mcr, 0);

	sc->sc_ih.ih_fun = comintr;
	sc->sc_ih.ih_arg = sc;
	intr_establish(ia->ia_irq, &sc->sc_ih, DV_TTY);

	/*
	 * Need to reset baud rate, etc. of next print so reset comconsinit.
	 * Also make sure console is always "hardwired"
	 */
	if (iobase == comconsole) {
		constty = com_tty[unit] = ttymalloc();
		comconsinit = 0;
		sc->sc_flags |= COM_SOFTCAR;
	}
}

int
comopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int	unit = COMUNIT(dev);
	struct	com_softc *sc;
	u_short	iobase;
	struct	tty *tp;
	int	s;
	int	error = 0;
 
	if (unit >= comcd.cd_ndevs)
		return ENXIO;
	sc = comcd.cd_devs[unit];
	if (!sc)
		return ENXIO;

	if (!com_tty[unit])
		tp = com_tty[unit] = ttymalloc();
	else
		tp = com_tty[unit];

	tp->t_oproc = comstart;
	tp->t_param = comparam;
	tp->t_dev = dev;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_state |= TS_WOPEN;
		ttychars(tp);
		/* preserve previous speed, if any */
		if (tp->t_ispeed == 0) {
			tp->t_iflag = TTYDEF_IFLAG;
			tp->t_oflag = TTYDEF_OFLAG;
			tp->t_cflag = TTYDEF_CFLAG;
			tp->t_lflag = TTYDEF_LFLAG;
			tp->t_ispeed = tp->t_ospeed = comdefaultrate;
		}
		comparam(tp, &tp->t_termios);
		ttsetwater(tp);
	} else if (tp->t_state & TS_XCLUDE && p->p_ucred->cr_uid != 0)
		return EBUSY;

	s = spltty();
	iobase = sc->sc_iobase;
	/* flush any pending I/O */
	if (sc->sc_flags & COM_FIFO)
		outb(iobase + com_fifo,
		     FIFO_ENABLE | FIFO_RCV_RST | FIFO_XMT_RST | FIFO_TRIGGER_8);
	(void) inb(iobase + com_lsr);
	(void) inb(iobase + com_data);
	/* you turn me on, baby */
	outb(iobase + com_mcr, MCR_DTR | MCR_RTS | MCR_IENABLE);
	outb(iobase + com_ier, IER_ERXRDY | IER_ETXRDY | IER_ERLS | IER_EMSC);

	if (sc->sc_flags & COM_SOFTCAR || inb(iobase + com_msr) & MSR_DCD)
		tp->t_state |= TS_CARR_ON;
	else
		tp->t_state &=~ TS_CARR_ON;
	while ((flag & O_NONBLOCK) == 0 && (tp->t_cflag & CLOCAL) == 0 &&
	       (tp->t_state & TS_CARR_ON) == 0) {
		/* wait for carrier; allow signals */
		tp->t_state |= TS_WOPEN;
		if (error = ttysleep(tp, (caddr_t)&tp->t_rawq, TTIPRI | PCATCH,
		    ttopen, 0)) {
			splx(s);
			return error;
		}
	}
	splx(s);

	return (*linesw[tp->t_line].l_open)(dev, tp);
}
 
int
comclose(dev, flag)
	dev_t dev;
	int flag;
{
	int	unit = COMUNIT(dev);
	struct	com_softc *sc = comcd.cd_devs[unit];
	u_short	iobase = sc->sc_iobase;
	struct	tty *tp = com_tty[unit];
 
	(*linesw[tp->t_line].l_close)(tp, flag);
	bic(iobase + com_cfcr, CFCR_SBREAK);
	outb(iobase + com_ier, 0);
	if (tp->t_cflag & HUPCL)
		outb(iobase + com_mcr, 0);
	ttyclose(tp);
#ifdef notyet /* XXXX */
	if (iobase != comconsole) {
		ttyfree(tp);
		com_tty[unit] = (struct tty *)NULL;
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
	struct	tty *tp = com_tty[COMUNIT(dev)];
 
	return (*linesw[tp->t_line].l_read)(tp, uio, flag);
}
 
int
comwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct	tty *tp = com_tty[COMUNIT(dev)];
 
#if 0
	/* XXXX what is this for? */
	if (constty == tp)
		constty = NULL;
#endif
	return (*linesw[tp->t_line].l_write)(tp, uio, flag);
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
comioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	int	unit = COMUNIT(dev);
	struct	com_softc *sc = comcd.cd_devs[unit];
	u_short	iobase = sc->sc_iobase;
	struct	tty *tp = com_tty[unit];
	int	error;
 
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if (error >= 0)
		return error;
	error = ttioctl(tp, cmd, data, flag);
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
		bis(iobase + com_mcr, MCR_DTR | MCR_RTS);
		break;
	    case TIOCCDTR:
		bic(iobase + com_mcr, MCR_DTR | MCR_RTS);
		break;
	    case TIOCMSET:
		outb(iobase + com_mcr, tiocm_xxx2mcr(*(int *)data) | MCR_IENABLE);
		break;
	    case TIOCMBIS:
		bis(iobase + com_mcr, tiocm_xxx2mcr(*(int *)data));
		break;
	    case TIOCMBIC:
		bic(iobase + com_mcr, tiocm_xxx2mcr(*(int *)data));
		break;
	    case TIOCMGET:
		{
			u_char	m = inb(iobase + com_mcr);
			int	bits = 0;

			m = inb(iobase + com_mcr);
			if (m & MCR_DTR)
				bits |= TIOCM_DTR;
			if (m & MCR_RTS)
				bits |= TIOCM_RTS;
			m = inb(iobase + com_msr);
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
		break;
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
	int	unit = COMUNIT(tp->t_dev);
	struct	com_softc *sc = comcd.cd_devs[unit];
	u_short	iobase = sc->sc_iobase;
	int	ospeed = comspeed(t->c_ospeed);
	u_char	cfcr;
	int	s;
 
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

	/* and copy to tty */
	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	if (ospeed == 0)
		bic(iobase + com_mcr, MCR_DTR | MCR_RTS);
	else
		bis(iobase + com_mcr, MCR_DTR | MCR_RTS);

	outb(iobase + com_cfcr, cfcr | CFCR_DLAB);
	outb(iobase + com_dlbl, ospeed);
	outb(iobase + com_dlbh, ospeed>>8);
	outb(iobase + com_cfcr, cfcr);

	/* when not using CRTS_IFLOW, RTS follows DTR */
	if ((t->c_cflag & CRTS_IFLOW) == 0) {
		u_char	mcr = inb(iobase + com_mcr);

		if (mcr & MCR_DTR)
			if ((mcr & MCR_RTS) == 0)
				outb(iobase + com_mcr, mcr | MCR_RTS);
			else
				;
		else
			if (mcr & MCR_RTS)
				outb(iobase + com_mcr, mcr &~ MCR_RTS);
			else
				;
	}

	splx(s);

	return 0;
}
 
void
comstart(tp)
	struct tty *tp;
{
	int	unit = COMUNIT(tp->t_dev);
	struct	com_softc *sc = comcd.cd_devs[unit];
	u_short	iobase = sc->sc_iobase;
	int	s;
 
	s = spltty();
	if (tp->t_state & (TS_TTSTOP | TS_BUSY))
		goto out;
	if (tp->t_cflag & CCTS_OFLOW && (inb(iobase + com_mcr) & MSR_CTS) == 0)
		goto out;
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &=~ TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
	}
	if (tp->t_outq.c_cc == 0)
		goto out;
	tp->t_state |= TS_BUSY;
	if ((inb(iobase + com_lsr) & LSR_TXRDY) == 0)
		goto out;
	if (sc->sc_flags & COM_FIFO) {
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
	int	s;

	s = spltty();
	if (tp->t_state & TS_BUSY)
		if ((tp->t_state & TS_TTSTOP) == 0)
			tp->t_state |= TS_FLUSH;
	splx(s);
}
 
static void
comeint(sc, stat)
	struct com_softc *sc;
	int stat;
{
	u_short	iobase = sc->sc_iobase;
	struct	tty *tp = com_tty[sc->sc_dev.dv_unit];
	int	c;

	c = inb(iobase + com_data);
	if ((tp->t_state & TS_ISOPEN) == 0)
		return;
	if (stat & (LSR_BI | LSR_FE))
		c |= TTY_FE;
	else if (stat & LSR_PE)
		c |= TTY_PE;
	else if (stat & LSR_OE) {
		c |= TTY_PE;		/* XXX ought to have its own define */
		log(LOG_WARNING, "%s: silo overflow\n", sc->sc_dev.dv_xname);
	}
	/* XXXX put in FIFO and process later */
	(*linesw[tp->t_line].l_rint)(c, tp);
}

static void
commint(sc)
	struct com_softc *sc;
{
	u_short	iobase = sc->sc_iobase;
	struct	tty *tp = com_tty[sc->sc_dev.dv_unit];
	u_char	stat;

	stat = inb(iobase + com_msr);
	if (stat & MSR_DDCD && (sc->sc_flags & COM_SOFTCAR) == 0) {
		if (stat & MSR_DCD)
			(void)(*linesw[tp->t_line].l_modem)(tp, 1);
		else if ((*linesw[tp->t_line].l_modem)(tp, 0) == 0)
			bic(iobase + com_mcr, MCR_DTR | MCR_RTS);
	}
	if (stat & MSR_DCTS && tp->t_state & TS_ISOPEN && tp->t_cflag & CRTSCTS) {
		/* the line is up and we want to do rts/cts flow control */
		if (stat & MSR_CTS) {
			tp->t_state &=~ TS_TTSTOP;
			ttstart(tp);
		} else
			tp->t_state |= TS_TTSTOP;
	}
}

static int
comintr(sc)
	struct com_softc *sc;
{
	u_short	iobase = sc->sc_iobase;
	struct	tty *tp;
	u_char	code;

	code = inb(iobase + com_iir);
	if (code & IIR_NOPEND)
		return 0;

	for (;;) {
		switch (code & IIR_IMASK) {
		    case IIR_RXRDY:
		    case IIR_RXTOUT:
			tp = com_tty[sc->sc_dev.dv_unit];
			/* XXXX put in FIFO and process later */
#define	RCVBYTE() \
			code = inb(iobase + com_data); \
			if (tp->t_state & TS_ISOPEN) \
				(*linesw[tp->t_line].l_rint)(code, tp)
			RCVBYTE();
			if (sc->sc_flags & COM_FIFO)
				while (code = inb(iobase + com_lsr) & LSR_RCV_MASK) {
					if (code == LSR_RXRDY) {
						RCVBYTE();
					} else
						comeint(sc, code);
				}
			break;
		    case IIR_TXRDY:
			tp = com_tty[sc->sc_dev.dv_unit];
			tp->t_state &=~ TS_BUSY;
			if (tp->t_state & TS_FLUSH)
				tp->t_state &=~ TS_FLUSH;
			else
				if (tp->t_line)
					(*linesw[tp->t_line].l_start)(tp);
				else
					comstart(tp);
			break;
		    case IIR_MLSC:
			commint(sc);
			break;
		    case IIR_RLS:
			comeint(sc, inb(iobase + com_lsr));
			break;
		    default:
			log(LOG_WARNING, "%s: weird iir=0x%x\n",
			    sc->sc_dev.dv_xname, code);
			/* fall through */
		}

		code = inb(iobase + com_iir);
		if (code & IIR_NOPEND)
			return 1;
	}
}

/* XXXXXXXXXXXXXXXX ---- gremlins below here ---- XXXXXXXXXXXXXXXX */
 
#if 0
/*
 * Following are all routines needed for COM to act as console
 */
#include "i386/i386/cons.h"

comcnprobe(cp)
	struct consdev *cp;
{
	/* XXXX */
	if (!_comprobe(CONADDR))
		return CN_DEAD;

	/* locate the major number */
	for (commajor = 0; commajor < nchrdev; commajor++)
		if (cdevsw[commajor].d_open == comopen)
			break;

	/* initialize required fields */
	cp->cn_dev = makedev(commajor, unit);
#ifdef	COMCONSOLE
	cp->cn_pri = CN_REMOTE;		/* Force a serial port console */
#else
	cp->cn_pri = CN_NORMAL;
#endif
}

comcninit(cp)
	struct consdev *cp;
{
	int unit = COMUNIT(cp->cn_dev);

	comconsole = CONADDR;
	comconsinit = 0;
}

cominit(unit, rate)
	int unit, rate;
{
	int com;
	int s;
	short stat;

	com = com_addr[unit];
	s = splhigh();
	outb(com+com_cfcr, CFCR_DLAB);
	rate = COM_BAUDDIV(rate);
	outb(com+com_data, rate & 0xFF);
	outb(com+com_ier, rate >> 8);
	outb(com+com_cfcr, CFCR_8BITS);
	outb(com+com_ier, IER_ERXRDY | IER_ETXRDY);
	outb(com+com_fifo, FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER_4);
	stat = inb(com+com_iir);
	splx(s);
}

comcngetc(dev)
{
	com = com_addr[COMUNIT(dev)];
	short stat;
	int c, s;

	s = splhigh();
	while (((stat = inb(com+com_lsr)) & LSR_RXRDY) == 0)
		;
	c = inb(com+com_data);
	stat = inb(com+com_iir);
	splx(s);
	return c;
}

/*
 * Console kernel output character routine.
 */
comcnputc(dev, c)
	dev_t dev;
	int c;
{
	com = com_addr[COMUNIT(dev)];
	int timo;
	short stat;
	int s = splhigh();

	if (comconsinit == 0) {
		(void) cominit(COMUNIT(dev), comdefaultrate);
		comconsinit = 1;
	}
	/* wait for any pending transmission to finish */
	timo = 50000;
	while (((stat = inb(com+com_lsr)) & LSR_TXRDY) == 0 && --timo)
		;
	outb(com+com_data, c);
	/* wait for this transmission to complete */
	timo = 1500000;
	while (((stat = inb(com+com_lsr)) & LSR_TXRDY) == 0 && --timo)
		;
	/* clear any interrupts generated by this transmission */
	stat = inb(com+com_iir);
	splx(s);
}
#endif

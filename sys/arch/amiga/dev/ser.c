/*
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
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
 *	@(#)ser.c	7.12 (Berkeley) 6/27/91
 */

#include "ser.h"

#if NSER > 0
#include "sys/param.h"
#include "sys/systm.h"
#include "sys/ioctl.h"
#include "sys/tty.h"
#include "sys/proc.h"
#include "sys/conf.h"
#include "sys/file.h"
#include "sys/malloc.h"
#include "sys/uio.h"
#include "sys/kernel.h"
#include "sys/syslog.h"

#include "device.h"
#include "serreg.h"
#include "machine/cpu.h"

#include "../amiga/custom.h"
#include "../amiga/cia.h"

int	serprobe();
struct	driver serdriver = {
	serprobe, "ser",
};

int	serstart(), serparam(), serintr();
int	sersoftCAR;
int	ser_active;
int	ser_hasfifo;
int	nser = NSER;
#ifdef SERCONSOLE
int	serconsole = SERCONSOLE;
#else
int	serconsole = -1;
#endif
int	serconsinit;
int	serdefaultrate = TTYDEF_SPEED;
int	sermajor;
struct	serdevice *ser_addr[NSER];
struct	tty ser_cons;
struct	tty *ser_tty[NSER] = { &ser_cons };

struct speedtab serspeedtab[] = {
	0,	0,
	50,	SERBRD(50),
	75,	SERBRD(75),
	110,	SERBRD(110),
	134,	SERBRD(134),
	150,	SERBRD(150),
	200,	SERBRD(200),
	300,	SERBRD(300),
	600,	SERBRD(600),
	1200,	SERBRD(1200),
	1800,	SERBRD(1800),
	2400,	SERBRD(2400),
	4800,	SERBRD(4800),
	9600,	SERBRD(9600),
	19200,	SERBRD(19200),
	38400,	SERBRD(38400),
	-1,	-1
};


/* since this UART is not particularly bright (nice put), we'll have to do
   parity stuff on our own. this table contains the 8th bit in 7bit character
   mode, for even parity. If you want odd parity, flip the bit. (for
   generation of the table, see genpar.c) */

u_char even_parity[] = {
   0,  1,  1,  0,  1,  0,  0,  1,  1,  0,  0,  1,  0,  1,  1,  0, 
   1,  0,  0,  1,  0,  1,  1,  0,  0,  1,  1,  0,  1,  0,  0,  1, 
   1,  0,  0,  1,  0,  1,  1,  0,  0,  1,  1,  0,  1,  0,  0,  1, 
   0,  1,  1,  0,  1,  0,  0,  1,  1,  0,  0,  1,  0,  1,  1,  0, 
   1,  0,  0,  1,  0,  1,  1,  0,  0,  1,  1,  0,  1,  0,  0,  1, 
   0,  1,  1,  0,  1,  0,  0,  1,  1,  0,  0,  1,  0,  1,  1,  0, 
   0,  1,  1,  0,  1,  0,  0,  1,  1,  0,  0,  1,  0,  1,  1,  0, 
   1,  0,  0,  1,  0,  1,  1,  0,  0,  1,  1,  0,  1,  0,  0,  1, 
};


/* since we don't get interrupts for changes on the modem control line,
   well have to fake them by comparing current settings to the settings
   we remembered on last invocation. */
u_char last_ciab_pra;

extern	struct tty *constty;
#ifdef KGDB
#include "machine/remote-sl.h"

extern dev_t kgdb_dev;
extern int kgdb_rate;
extern int kgdb_debug_init;
#endif

#if 0
#define	UNIT(x)		minor(x)
#else
/* just always force this to 0, so we can later interprete special
   settings out of the unit number.. */
#define UNIT(x)		0
#endif

#ifdef DEBUG
long	fifoin[17];
long	fifoout[17];
long	serintrcount[16];
long	sermintcount[16];
#endif

serprobe(ad)
	register struct amiga_device *ad;
{
	register struct serdevice *ser;
	register int unit;

	ser = (struct serdevice *) ad->amiga_addr;
	unit = ad->amiga_unit;
	if (unit == serconsole)
		DELAY(100000);

	ad->amiga_ipl = 2;
	ser_addr[unit] = ser;
	ser_active |= 1 << unit;
	sersoftCAR = ad->amiga_flags;
#ifdef KGDB
	if (kgdb_dev == makedev(sermajor, unit)) {
		if (serconsole == unit)
			kgdb_dev = NODEV; /* can't debug over console port */
		else {
			(void) serinit(unit, kgdb_rate);
			serconsinit = 1;	/* don't re-init in serputc */
			if (kgdb_debug_init) {
				/*
				 * Print prefix of device name,
				 * let kgdb_connect print the rest.
				 */
				printf("ser%d: ", unit);
				kgdb_connect(1);
			} else
				printf("ser%d: kgdb enabled\n", unit);
		}
	}
#endif
	/*
	 * Need to reset baud rate, etc. of next print so reset serconsinit.
	 * Also make sure console is always "hardwired."
	 */
	if (unit == serconsole) {
		serconsinit = 0;
		sersoftCAR |= (1 << unit);
	}
	return (1);
}

/* ARGSUSED */
#ifdef __STDC__
seropen(dev_t dev, int flag, int mode, struct proc *p)
#else
seropen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
#endif
{
	register struct tty *tp;
	register int unit;
	int error = 0;

	unit = minor (dev);
	
	if (unit == 1)	
	  {
	    unit = 0;
	    sersoftCAR = 0;
	  }
	else if (unit == 2)
	  {
	    unit = 0;
	    sersoftCAR = 0xff;
	  }
	else
	  unit = 0;

	if (unit >= NSER || (ser_active & (1 << unit)) == 0)
		return (ENXIO);
	if(!ser_tty[unit]) {
		MALLOC(tp, struct tty *, sizeof(struct tty), M_TTYS, M_WAITOK);
		bzero(tp, sizeof(struct tty));
		ser_tty[unit] = tp;
	} else
		tp = ser_tty[unit];
	tp->t_oproc = serstart;
	tp->t_param = serparam;
	tp->t_dev = dev;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_state |= TS_WOPEN;
		ttychars(tp);
		if (tp->t_ispeed == 0) {
			tp->t_iflag = TTYDEF_IFLAG | IXOFF;	/* XXXXX */
			tp->t_oflag = TTYDEF_OFLAG;
#if 0
			tp->t_cflag = TTYDEF_CFLAG;
#else
			tp->t_cflag = (CREAD | CS8 | CLOCAL);	/* XXXXX */
#endif
			tp->t_lflag = TTYDEF_LFLAG;
			tp->t_ispeed = tp->t_ospeed = serdefaultrate;
		}
		serparam(tp, &tp->t_termios);
		ttsetwater(tp);
	} else if (tp->t_state&TS_XCLUDE && p->p_ucred->cr_uid != 0)
		return (EBUSY);
	(void) sermctl (dev, TIOCM_DTR | TIOCM_RTS, DMSET);
	if ((sersoftCAR & (1 << unit)) || (sermctl(dev, 0, DMGET) & TIOCM_CD))
		tp->t_state |= TS_CARR_ON;
	(void) spltty();
	while ((flag&O_NONBLOCK) == 0 && (tp->t_cflag&CLOCAL) == 0 &&
	       (tp->t_state & TS_CARR_ON) == 0) {
		tp->t_state |= TS_WOPEN;
		if (error = ttysleep(tp, (caddr_t)&tp->t_raw, TTIPRI | PCATCH,
		    ttopen, 0))
			break;
	}
	(void) spl0();
	if (error == 0)
		error = (*linesw[tp->t_line].l_open)(dev, tp);
	return (error);
}
 
/*ARGSUSED*/
serclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	register struct tty *tp;
	register struct serdevice *ser;
	register int unit;
 
	unit = UNIT(dev);

	ser = ser_addr[unit];
	tp = ser_tty[unit];
	(*linesw[tp->t_line].l_close)(tp, flag);
	custom.adkcon = ADKCONF_UARTBRK;	/* clear break */
#ifdef KGDB
	/* do not disable interrupts if debugging */
	if (dev != kgdb_dev)
#endif
	custom.intena = INTF_RBF | INTF_VERTB; /* clear interrupt enable */
	custom.intreq = INTF_RBF | INTF_VERTB; /* and   interrupt request */
#if 0
/* if the device is closed, it's close, no matter whether we deal with modem
   control signals nor not. */
	if (tp->t_cflag&HUPCL || tp->t_state&TS_WOPEN ||
	    (tp->t_state&TS_ISOPEN) == 0)
#endif
		(void) sermctl(dev, 0, DMSET);
	ttyclose(tp);
#if 0
	if (tp != &ser_cons)
	  {
	    FREE(tp, M_TTYS);
	    ser_tty[unit] = (struct tty *)NULL;
	  }
#endif
	return (0);
}
 
serread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp = ser_tty[UNIT(dev)];
	int error = (*linesw[tp->t_line].l_read)(tp, uio, flag);

	return error;
}
 
serwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	int unit = UNIT(dev);
	register struct tty *tp = ser_tty[unit];
 
	/*
	 * (XXX) We disallow virtual consoles if the physical console is
	 * a serial port.  This is in case there is a display attached that
	 * is not the console.  In that situation we don't need/want the X
	 * server taking over the console.
	 */
	if (constty && unit == serconsole)
		constty = NULL;
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}
 
serintr(unit)
	register int unit;
{
	register struct serdevice *ser;
	register u_short code;
	register u_char ch;
	register u_short ints;
	register struct tty *tp;

	ser = ser_addr[unit];

again:
	ints = custom.intreqr & INTF_RBF;
	if (! ints)
	  return 0;

	/* clear interrupt(s) */
	custom.intreq = ints;

	/* this register contains both data and status bits! */
	code = custom.serdatr;

	if (ints & INTF_RBF)
	  {
	    tp = ser_tty[unit];
/*
 * Process a received byte.  Inline for speed...
 */
#ifdef KGDB
#define	RCVBYTE() \
	    ch = code & 0xff; \
	    if ((tp->t_state & TS_ISOPEN) == 0) { \
		if (ch == FRAME_END && \
		    kgdb_dev == makedev(sermajor, unit)) \
			kgdb_connect(0); /* trap into kgdb */ \
	    }
#else
#define	RCVBYTE()
#endif
	    RCVBYTE();
	    /* sereint does the receive-processing */
	    sereint (unit, code, ser);
	  }

	/* fake modem-control interrupt */
	sermint (unit, ser);
	/* try to save interrupt load.. */
	goto again;
}

sereint(unit, stat, ser)
	register int unit, stat;
	register struct serdevice *ser;
{
	register struct tty *tp;
	register int c;
	register u_char ch;

	tp = ser_tty[unit];
	if ((tp->t_state & TS_ISOPEN) == 0) {
#ifdef KGDB
		/* we don't care about parity errors */
		if (kgdb_dev == makedev(sermajor, unit) && c == FRAME_END)
			kgdb_connect(0); /* trap into kgdb */
#endif
		return;
	}
	ch = stat & 0xff;
	c = ch;
	/* all databits 0 including stop indicate break condition */
	if (!(stat & 0x1ff))
	  c |= TTY_FE;

	/* if parity checking enabled, check parity */
	else if ((tp->t_cflag & PARENB) &&
		 (((ch >> 7) + even_parity[ch & 0x7f] + !!(tp->t_cflag & PARODD)) & 1))
	  c |= TTY_PE;

	if (stat & SERDATRF_OVRUN)
	  log(LOG_WARNING, "ser%d: silo overflow\n", unit);

	(*linesw[tp->t_line].l_rint)(c, tp);
}

sermint(unit)
	register int unit;
{
	register struct tty *tp;
	register u_char stat, last, istat;
	register struct serdevice *ser;

	tp = ser_tty[unit];
	stat = ciab.pra;
	last = last_ciab_pra;
	last_ciab_pra = stat;
	
	/* check whether any interesting signal changed state */
	istat = stat ^ last;

	if ((istat & CIAB_PRA_CD) && (sersoftCAR & (1 << unit)) == 0) 
	  {
	    if (ISDCD (stat))
	      (*linesw[tp->t_line].l_modem)(tp, 1);
	    else if ((*linesw[tp->t_line].l_modem)(tp, 0) == 0)
	      {
	        CLRDTR (stat);
	        CLRRTS (stat);
	        ciab.pra = stat;
	        last_ciab_pra = stat;
	      }
	  } 
	else if ((istat & CIAB_PRA_CTS) && (tp->t_state & TS_ISOPEN) &&
		 (tp->t_flags & CRTSCTS)) 
	  {
	    /* the line is up and we want to do rts/cts flow control */
	    if (ISCTS (stat))
	      {
		tp->t_state &=~ TS_TTSTOP;
		ttstart(tp);
	      }
	    else
	      tp->t_state |= TS_TTSTOP;
	}
}

serioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{
	register struct tty *tp;
	register int unit = UNIT(dev);
	register struct serdevice *ser;
	register int error;
 
	tp = ser_tty[unit];
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, data, flag);
	if (error >= 0)
		return (error);

	ser = ser_addr[unit];
	switch (cmd) {

	case TIOCSBRK:
		custom.adkcon = ADKCONF_SETCLR | ADKCONF_UARTBRK;
		break;

	case TIOCCBRK:
		custom.adkcon = ADKCONF_UARTBRK;
		break;

	case TIOCSDTR:
		(void) sermctl(dev, TIOCM_DTR | TIOCM_RTS, DMBIS);
		break;

	case TIOCCDTR:
		(void) sermctl(dev, TIOCM_DTR | TIOCM_RTS, DMBIC);
		break;

	case TIOCMSET:
		(void) sermctl(dev, *(int *)data, DMSET);
		break;

	case TIOCMBIS:
		(void) sermctl(dev, *(int *)data, DMBIS);
		break;

	case TIOCMBIC:
		(void) sermctl(dev, *(int *)data, DMBIC);
		break;

	case TIOCMGET:
		*(int *)data = sermctl(dev, 0, DMGET);
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}

serparam(tp, t)
	register struct tty *tp;
	register struct termios *t;
{
	register struct serdevice *ser;
	register int cfcr, cflag = t->c_cflag;
	int unit = UNIT(tp->t_dev);
	int ospeed = ttspeedtab(t->c_ospeed, serspeedtab);
 
	/* check requested parameters */
        if (ospeed < 0 || (t->c_ispeed && t->c_ispeed != t->c_ospeed))
                return (EINVAL);
        /* and copy to tty */
        tp->t_ispeed = t->c_ispeed;
        tp->t_ospeed = t->c_ospeed;
        tp->t_cflag = cflag;

	custom.intena = INTF_SETCLR | INTF_RBF;
	custom.intreq = INTF_RBF;
	last_ciab_pra = ciab.pra;

	if (ospeed == 0) {
		(void) sermctl(unit, 0, DMSET);	/* hang up line */
		return (0);
	}
	/* set the baud rate */
	custom.serper = (0<<15) | ospeed;  /* select 8 bit mode (instead of 9 bit) */

	return (0);
}
 
serstart(tp)
	register struct tty *tp;
{
	register struct serdevice *ser;
	int s, unit;
	u_short c;
 
	unit = UNIT(tp->t_dev);
	ser = ser_addr[unit];
	s = spltty();
again:
	if (tp->t_state & (TS_TIMEOUT|TS_TTSTOP))
		goto out;
	if (RB_LEN(&tp->t_out) <= tp->t_lowat) {
		if (tp->t_state&TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_out);
		}
		selwakeup (&tp->t_wsel);
	}
	if (RB_LEN(&tp->t_out) == 0)
		goto out;

	while (! (custom.serdatr & SERDATRF_TBE)) ;

	c = rbgetc(&tp->t_out);
	/* tp->t_state |= TS_BUSY; */
	
	/* handle truncation of character if necessary */
	if ((tp->t_cflag & CSIZE) == CS7)
	  c &= 0x7f;

	/* handle parity if necessary (forces CS7) */
	if (tp->t_cflag & PARENB)
	  {
	    if (even_parity[c & 0x7f])
	      c |= 0x80;
	    if (tp->t_cflag & PARODD)
	      c ^= 0x80;
	  }

	/* add stop bit(s) */
	if (tp->t_cflag & CSTOPB)
	  c |= 0x300;
	else
	  c |= 0x100;
	
	custom.serdat = c;
	
	/* if there's input on the line, stop spitting out characters */
	if (! (custom.intreqr & INTF_RBF))
	  goto again;

out:
	splx(s);
}
 
/*
 * Stop output on a line.
 */
/*ARGSUSED*/
serstop(tp, flag)
	register struct tty *tp;
{
	register int s;

	s = spltty();
	if (tp->t_state & TS_BUSY) {
		if ((tp->t_state&TS_TTSTOP)==0)
			tp->t_state |= TS_FLUSH;
	}
	splx(s);
}
 
sermctl(dev, bits, how)
	dev_t dev;
	int bits, how;
{
	register struct serdevice *ser;
	register int unit;
	u_char ub;
	int s;

	unit = UNIT(dev);
	ser = ser_addr[unit];

	/* convert TIOCM* mask into CIA mask (which is really low-active!!) */
	if (how != DMGET)
	  {
	    ub = 0;
	    if (bits & TIOCM_DTR) ub |= CIAB_PRA_DTR;
	    if (bits & TIOCM_RTS) ub |= CIAB_PRA_RTS;
	    if (bits & TIOCM_CTS) ub |= CIAB_PRA_CTS;
	    if (bits & TIOCM_CD)  ub |= CIAB_PRA_CD;
	    if (bits & TIOCM_RI)  ub |= CIAB_PRA_SEL;	/* collision with /dev/par ! */
	    if (bits & TIOCM_DSR) ub |= CIAB_PRA_DSR;
	  }


	s = spltty();
	switch (how) {

	case DMSET:
		/* invert and set */
		ciab.pra = ~ub;
		break;

	case DMBIC:
		ciab.pra |= ub;
		ub = ~ciab.pra;
		break;

	case DMBIS:
		ciab.pra &= ~ub;
		ub = ~ciab.pra;
		break;

	case DMGET:
		ub = ~ciab.pra;
		break;
	}
	(void) splx(s);

	bits = 0;
	if (ub & CIAB_PRA_DTR) bits |= TIOCM_DTR;
	if (ub & CIAB_PRA_RTS) bits |= TIOCM_RTS;
	if (ub & CIAB_PRA_CTS) bits |= TIOCM_CTS;
	if (ub & CIAB_PRA_CD)  bits |= TIOCM_CD;
	if (ub & CIAB_PRA_SEL) bits |= TIOCM_RI;
	if (ub & CIAB_PRA_DSR) bits |= TIOCM_DSR;

	return bits;
}

/*
 * Following are all routines needed for SER to act as console
 */
#include "../amiga/cons.h"

sercnprobe(cp)
	struct consdev *cp;
{
	int unit = CONUNIT;

	/* locate the major number */
	for (sermajor = 0; sermajor < nchrdev; sermajor++)
		if (cdevsw[sermajor].d_open == seropen)
			break;

	/* XXX: ick */
	unit = CONUNIT;

	/* initialize required fields */
	cp->cn_dev = makedev(sermajor, unit);
	cp->cn_tp = ser_tty[unit];
	cp->cn_pri = CN_NORMAL;

	/*
	 * If serconsole is initialized, raise our priority.
	 */
	if (serconsole == unit)
		cp->cn_pri = CN_REMOTE;
#ifdef KGDB
	if (major(kgdb_dev) == 1)			/* XXX */
		kgdb_dev = makedev(sermajor, minor(kgdb_dev));
#endif
}

sercninit(cp)
	struct consdev *cp;
{
	int unit = UNIT(cp->cn_dev);

	serinit(unit, serdefaultrate);
	serconsole = unit;
	serconsinit = 1;
}

serinit(unit, rate)
	int unit, rate;
{
	int s;

#ifdef lint
	stat = unit; if (stat) return;
#endif
	s = splhigh();
	/* might want to fiddle with the CIA later ??? */
	custom.serper = ttspeedtab(rate, serspeedtab);
	splx(s);
}

sercngetc(dev)
{
	u_short stat;
	int c, s;

#ifdef lint
	stat = dev; if (stat) return (0);
#endif
	s = splhigh();
	while (!((stat = custom.serdatr & 0xffff) & SERDATRF_RBF))
		;
	c = stat & 0xff;
	/* clear interrupt */
	custom.intreq = INTF_RBF;
	splx(s);
	return (c);
}

/*
 * Console kernel output character routine.
 */
sercnputc(dev, c)
	dev_t dev;
	register int c;
{
	register int timo;
	short stat;
	int s = splhigh();

#ifdef lint
	stat = dev; if (stat) return;
#endif
	if (serconsinit == 0) {
		(void) serinit(UNIT(dev), serdefaultrate);
		serconsinit = 1;
	}
	/* wait for any pending transmission to finish */
	timo = 50000;
        while (! (custom.serdatr & SERDATRF_TBE) && --timo)
		;
        custom.serdat = (c&0xff) | 0x100;
	/* wait for this transmission to complete */
	timo = 1500000;
        while (! (custom.serdatr & SERDATRF_TBE) && --timo) 
		;
	/* wait for the device (my vt100..) to process the data, since
	   we don't do flow-control with cnputc */
        for (timo = 0; timo < 30000; timo++) ;

	/* clear any interrupts generated by this transmission */
        custom.intreq = INTF_TBE;
	splx(s);
}


serspit(c)
	int c;
{
	register struct Custom *cu asm("a2") = CUSTOMbase;
	register int timo asm("d2");
	extern int cold;
	int s;

	if (c == 10)
	  serspit (13);

	s = splhigh();

	/* wait for any pending transmission to finish */
	timo = 500000;
        while (! (cu->serdatr & (SERDATRF_TBE|SERDATRF_TSRE)) && --timo)
		;
        cu->serdat = (c&0xff) | 0x100;
	/* wait for this transmission to complete */
	timo = 15000000;
        while (! (cu->serdatr & SERDATRF_TBE) && --timo) 
		;
	/* clear any interrupts generated by this transmission */
        cu->intreq = INTF_TBE;
        
        for (timo = 0; timo < 30000; timo++) ;

	splx (s);
}

int
serselect(dev, rw, p)
	dev_t dev;
	int rw;
	struct proc *p;
{
	register struct tty *tp = ser_tty[UNIT(dev)];
	int nread;
	int s = spltty();
        struct proc *selp;

	switch (rw) {

	case FREAD:
		nread = ttnread(tp);
		if (nread > 0 || 
		   ((tp->t_cflag&CLOCAL) == 0 && (tp->t_state&TS_CARR_ON) == 0))
			goto win;
		selrecord(p, &tp->t_rsel);
		break;

	case FWRITE:
		if (RB_LEN(&tp->t_out) <= tp->t_lowat)
			goto win;
		selrecord(p, &tp->t_wsel);
		break;
	}
	splx(s);
	return (0);
  win:
	splx(s);
	return (1);
}

#endif

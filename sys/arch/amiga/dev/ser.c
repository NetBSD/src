/*	$NetBSD: ser.c,v 1.62 2002/03/17 19:40:31 atatat Exp $ */

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
/*
 * XXX This file needs major cleanup it will never service more than one
 * XXX unit.
 */

#include "opt_amigacons.h"
#include "opt_kgdb.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ser.c,v 1.62 2002/03/17 19:40:31 atatat Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/queue.h>
#include <machine/cpu.h>
#include <amiga/amiga/device.h>
#include <amiga/dev/serreg.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/cia.h>
#include <amiga/amiga/cc.h>

#include <dev/cons.h>

#include <sys/conf.h>
#include <machine/conf.h>

#include "ser.h"
#if NSER > 0

void serattach(struct device *, struct device *, void *);
int sermatch(struct device *, struct cfdata *, void *);

struct ser_softc {
	struct device dev;
	struct tty *ser_tty;
};

struct cfattach ser_ca = {
	sizeof(struct ser_softc), sermatch, serattach
};

extern struct cfdriver ser_cd;

#ifndef SEROBUF_SIZE
#define SEROBUF_SIZE 32
#endif
#ifndef SERIBUF_SIZE
#define SERIBUF_SIZE 512
#endif

#define splser() spl5()

void	serstart(struct tty *);
void	ser_shutdown(struct ser_softc *);
int	serparam(struct tty *, struct termios *);
void	serintr(void);
int	serhwiflow(struct tty *, int);
int	sermctl(dev_t dev, int, int);
void	ser_fastint(void);
void	sereint(int);
static	void ser_putchar(struct tty *, u_short);
void	ser_outintr(void);
void	sercnprobe(struct consdev *);
void	sercninit(struct consdev *);
void	serinit(int);
int	sercngetc(dev_t dev);
void	sercnputc(dev_t, int);
void	sercnpollc(dev_t, int);

int	nser = NSER;
#ifdef SERCONSOLE
int	serconsole = 0;
#else
int	serconsole = -1;
#endif
int	serconsinit;
int	serdefaultrate = TTYDEF_SPEED;
int	sermajor;
int	serswflags;

struct	vbl_node ser_vbl_node;
struct	tty ser_cons;
struct	tty *ser_tty;

static u_short serbuf[SERIBUF_SIZE];
static u_short *sbrpt = serbuf;
static u_short *sbwpt = serbuf;
static u_short sbcnt;
static u_short sbovfl;
static u_char serdcd;

/*
 * Since this UART is not particularly bright (to put it nicely), we'll
 * have to do parity stuff on our own.	This table contains the 8th bit
 * in 7bit character mode, for even parity.  If you want odd parity,
 * flip the bit.  (for generation of the table, see genpar.c)
 */

u_char	even_parity[] = {
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
};

/*
 * Since we don't get interrupts for changes on the modem control line,
 * we'll have to fake them by comparing current settings to the settings
 * we remembered on last invocation.
 */

u_char	last_ciab_pra;

extern struct tty *constty;

extern int ser_open_speed;	/* current speed of open serial device */

#ifdef KGDB
#include <machine/remote-sl.h>

extern dev_t kgdb_dev;
extern int kgdb_rate;
extern int kgdb_debug_init;
#endif

#ifdef DEBUG
long	fifoin[17];
long	fifoout[17];
long	serintrcount[16];
long	sermintcount[16];
#endif

void	sermint(register int unit);

int
sermatch(struct device *pdp, struct cfdata *cfp, void *auxp)
{
	static int ser_matched = 0;
	static int ser_matched_real = 0;

	/* Allow only once instance. */
	if (matchname("ser", (char *)auxp) == 0)
		return(0);

	if (amiga_realconfig) {
		if (ser_matched_real)
			return(0);
		ser_matched_real = 1;
	} else {
		if (serconsole != 0)
			return(0);

		if (ser_matched != 0)
			return(0);

		ser_matched = 1;
	}
	return(1);
}


void
serattach(struct device *pdp, struct device *dp, void *auxp)
{
	struct ser_softc *sc;
	struct tty *tp;
	u_short ir;

	sc = (struct ser_softc *)dp;

	ir = custom.intenar;
	if (serconsole == 0)
		DELAY(100000);

	ser_vbl_node.function = (void (*) (void *)) sermint;
	add_vbl_function(&ser_vbl_node, SER_VBL_PRIORITY, (void *) 0);
#ifdef KGDB
	if (kgdb_dev == makedev(sermajor, 0)) {
		if (serconsole == 0)
			kgdb_dev = NODEV; /* can't debug over console port */
		else {
			(void) serinit(kgdb_rate);
			serconsinit = 1;       /* don't re-init in serputc */
			if (kgdb_debug_init == 0)
				printf(" kgdb enabled\n");
			else {
				/*
				 * Print prefix of device name,
				 * let kgdb_connect print the rest.
				 */
				printf("ser0: ");
				kgdb_connect(1);
			}
		}
	}
#endif
	/*
	 * Need to reset baud rate, etc. of next print so reset serconsinit.
	 */
	if (0 == serconsole)
		serconsinit = 0;

	tp = ttymalloc();
	tp->t_oproc = (void (*) (struct tty *)) serstart;
	tp->t_param = serparam;
	tp->t_hwiflow = serhwiflow;
	tty_attach(tp);
	sc->ser_tty = ser_tty = tp;

	if (dp)
		printf(": input fifo %d output fifo %d\n", SERIBUF_SIZE,
		    SEROBUF_SIZE);
}


/* ARGSUSED */
int
seropen(dev_t dev, int flag, int mode, struct proc *p)
{
	struct ser_softc *sc;
	struct tty *tp;
	int unit, error, s, s2;

	error = 0;
	unit = SERUNIT(dev);

	if (unit >= ser_cd.cd_ndevs)
		return (ENXIO);

	sc = ser_cd.cd_devs[unit];
	if (sc == 0)
		return (ENXIO);

	/* XXX com.c: insert KGDB check here */

	/* XXX ser.c had: s = spltty(); */

	tp = sc->ser_tty;

	if ((tp->t_state & TS_ISOPEN) &&
	    (tp->t_state & TS_XCLUDE) &&
	    p->p_ucred->cr_uid != 0)
		return (EBUSY);

	s = spltty();

	/*
	 * If this is a first open...
	 */

	if ((tp->t_state & TS_ISOPEN) == 0 && tp->t_wopen == 0) {
		struct termios t;

		tp->t_dev = dev;

		s2 = splser();
		/*
		 * XXX here: hw enable,
		 */
		last_ciab_pra = ciab.pra;

		splx(s2);
		t.c_ispeed = 0;

		/* XXX serconsolerate? */
		t.c_ospeed = TTYDEF_SPEED;
		t.c_cflag = TTYDEF_CFLAG;

		if (serswflags & TIOCFLAG_CLOCAL)
			t.c_cflag |= CLOCAL;
		if (serswflags & TIOCFLAG_CRTSCTS)
			t.c_cflag |= CRTSCTS;
		if (serswflags & TIOCFLAG_MDMBUF)
			t.c_cflag |= MDMBUF;

		/* Make sure serparam() will do something. */
		tp->t_ospeed = 0;
		serparam(tp, &t);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		ttychars(tp);
		ttsetwater(tp);

		s2 = splser();
		(void)sermctl(dev, TIOCM_DTR, DMSET);
		/* clear input ring */
		sbrpt = sbwpt = serbuf;
		sbcnt = 0;
		splx(s2);
	}

	splx(s);

	error = ttyopen(tp, DIALOUT(dev), flag & O_NONBLOCK);
	if (error)
		goto bad;

	error =  tp->t_linesw->l_open(dev, tp);
	if (error)
		goto bad;

	return (0);

bad:
	if (!(tp->t_state & TS_ISOPEN) && tp->t_wopen == 0) {
		ser_shutdown(sc);
	}

	return (error);
}

/*ARGSUSED*/
int
serclose(dev_t dev, int flag, int mode, struct proc *p)
{
	struct ser_softc *sc;
	struct tty *tp;

	sc = ser_cd.cd_devs[0];
	tp = ser_tty;

	/* XXX This is for cons.c, according to com.c */
	if (!(tp->t_state & TS_ISOPEN))
		return (0);

	tp->t_linesw->l_close(tp, flag);
	ttyclose(tp);

	if (!(tp->t_state & TS_ISOPEN) && tp->t_wopen == 0) {
		ser_shutdown(sc);
	}
	return (0);
}

void
ser_shutdown(struct ser_softc *sc)
{
	struct tty *tp = sc->ser_tty;
	int s;

	s = splser();

	custom.adkcon = ADKCONF_UARTBRK;	/* clear break */
#if 0 /* XXX fix: #ifdef KGDB */
	/*
	 * do not disable interrupts if debugging
	 */
	if (dev != kgdb_dev)
#endif
		custom.intena = INTF_RBF | INTF_TBE;	/* disable interrupts */
	custom.intreq = INTF_RBF | INTF_TBE;		/* clear intr request */

	/*
	 * If HUPCL is not set, leave DTR unchanged.
	 */
	if (tp->t_cflag & HUPCL) {
		(void)sermctl(tp->t_dev, TIOCM_DTR, DMBIC);
		/*
		 * Idea from dev/ic/com.c:
		 * sleep a bit so that other side will notice, even if we
		 * reopen immediately.
		 */
		(void) tsleep(tp, TTIPRI, ttclos, hz);
	}

#if not_yet
	if (tp != &ser_cons) {
		remove_vbl_function(&ser_vbl_node);
		ttyfree(tp);
		ser_tty = (struct tty *) NULL;
	}
#endif
	ser_open_speed = tp->t_ispeed;
	return;
}

int
serread(dev_t dev, struct uio *uio, int flag)
{
	/* ARGSUSED */

	return ser_tty->t_linesw->l_read(ser_tty, uio, flag);
}

int
serwrite(dev_t dev, struct uio *uio, int flag)
{
	/* ARGSUSED */

	return ser_tty->t_linesw->l_write(ser_tty, uio, flag);
}

int
serpoll(dev_t dev, int events, struct proc *p)
{
	/* ARGSUSED */

	return ser_tty->t_linesw->l_poll(ser_tty, events, p);
}

struct tty *
sertty(dev_t dev)
{
	/* ARGSUSED */

	return (ser_tty);
}

/*
 * We don't do any processing of data here, so we store the raw code
 * obtained from the uart register.  In theory, 110kBaud gives you
 * 11kcps, so 16k buffer should be more than enough, interrupt
 * latency of 1s should never happen, or something is seriously
 * wrong..
 * buffers moved to above seropen()	-is
 */

/*
 * This is a replacement for the lack of a hardware fifo.  32k should be
 * enough (there's only one unit anyway, so this is not going to
 * accumulate).
 */
void
ser_fastint(void)
{
	/*
	 * We're at RBE-level, which is higher than VBL-level which is used
	 * to periodically transmit contents of this buffer up one layer,
	 * so no spl-raising is necessary.
	 */
	u_short code;

	/*
	 * This register contains both data and status bits!
	 */
	code = custom.serdatr;

	/*
	 * Use SERDATF_RBF instead of INTF_RBF; they're equivalent, but
	 * we save one (slow) custom chip access.
	 */
	if ((code & SERDATRF_RBF) == 0)
		return;

	/*
	 * clear interrupt
	 */
	custom.intreq = INTF_RBF;

	/*
	 * check for buffer overflow.
	 */
	if (sbcnt == SERIBUF_SIZE) {
		++sbovfl;
		return;
	}
	/*
	 * store in buffer
	 */
	*sbwpt++ = code;
	if (sbwpt == serbuf + SERIBUF_SIZE)
		sbwpt = serbuf;
	++sbcnt;
	if (sbcnt > SERIBUF_SIZE - 20)
		CLRRTS(ciab.pra);	/* drop RTS if buffer almost full */
}


void
serintr(void)
{
	int s1, s2, ovfl;
	struct tty *tp = ser_tty;

	/*
	 * Make sure we're not interrupted by another
	 * vbl, but allow level5 ints
	 */
	s1 = spltty();

	/*
	 * pass along any acumulated information
	 */
	while (sbcnt > 0 && (tp->t_state & TS_TBLOCK) == 0) {
		/*
		 * no collision with ser_fastint()
		 */
		sereint(*sbrpt++);

		ovfl = 0;
		/* lock against ser_fastint() */
		s2 = splser();
		sbcnt--;
		if (sbrpt == serbuf + SERIBUF_SIZE)
			sbrpt = serbuf;
		if (sbovfl != 0) {
			ovfl = sbovfl;
			sbovfl = 0;
		}
		splx(s2);
		if (ovfl != 0)
			log(LOG_WARNING, "ser0: %d ring buffer overflows.\n",
			    ovfl);
	}
	s2 = splser();
	if (sbcnt == 0 && (tp->t_state & TS_TBLOCK) == 0)
		SETRTS(ciab.pra);	/* start accepting data again */
	splx(s2);
	splx(s1);
}

void
sereint(int stat)
{
	struct tty *tp;
	u_char ch;
	int c;

	tp = ser_tty;
	ch = stat & 0xff;
	c = ch;

	if ((tp->t_state & TS_ISOPEN) == 0) {
#ifdef KGDB
		/* we don't care about parity errors */
		if (kgdb_dev == makedev(sermajor, 0) && c == FRAME_END)
			kgdb_connect(0);	/* trap into kgdb */
#endif
		return;
	}

	/*
	 * Check for break and (if enabled) parity error.
	 */
	if ((stat & 0x1ff) == 0)
		c |= TTY_FE;
	else if ((tp->t_cflag & PARENB) &&
		    (((ch >> 7) + even_parity[ch & 0x7f]
		    + !!(tp->t_cflag & PARODD)) & 1))
			c |= TTY_PE;

	if (stat & SERDATRF_OVRUN)
		log(LOG_WARNING, "ser0: silo overflow\n");

	tp->t_linesw->l_rint(c, tp);
}

/*
 * This interrupt is periodically invoked in the vertical blank
 * interrupt.  It's used to keep track of the modem control lines
 * and (new with the fast_int code) to move accumulated data
 * up into the tty layer.
 */
void
sermint(int unit)
{
	struct tty *tp;
	u_char stat, last, istat;

	tp = ser_tty;
	if (!tp)
		return;

	/*
	if ((tp->t_state & TS_ISOPEN) == 0 || tp->t_wopen == 0) {
		sbrpt = sbwpt = serbuf;
		return;
	}
	*/
	/*
	 * empty buffer
	 */
	serintr();

	stat = ciab.pra;
	last = last_ciab_pra;
	last_ciab_pra = stat;

	/*
	 * check whether any interesting signal changed state
	 */
	istat = stat ^ last;

	if (istat & serdcd) {
		tp->t_linesw->l_modem(tp, ISDCD(stat));
	}

	if ((istat & CIAB_PRA_CTS) && (tp->t_state & TS_ISOPEN) &&
	    (tp->t_cflag & CRTSCTS)) {
#if 0
		/* the line is up and we want to do rts/cts flow control */
		if (ISCTS(stat)) {
			tp->t_state &= ~TS_TTSTOP;
			ttstart(tp);
			/* cause tbe-int if we were stuck there */
			custom.intreq = INTF_SETCLR | INTF_TBE;
		} else
			tp->t_state |= TS_TTSTOP;
#else
		/* do this on hardware level, not with tty driver */
		if (ISCTS(stat)) {
			tp->t_state &= ~TS_TTSTOP;
			/* cause TBE interrupt */
			custom.intreq = INTF_SETCLR | INTF_TBE;
		}
#endif
	}
}

int
serioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	register struct tty *tp;
	register int error;

	tp = ser_tty;
	if (!tp)
		return ENXIO;

	error = tp->t_linesw->l_ioctl(tp, cmd, data, flag, p);
	if (error != EPASSTHROUGH)
		return(error);

	error = ttioctl(tp, cmd, data, flag, p);
	if (error != EPASSTHROUGH)
		return(error);

	switch (cmd) {
	case TIOCSBRK:
		custom.adkcon = ADKCONF_SETCLR | ADKCONF_UARTBRK;
		break;

	case TIOCCBRK:
		custom.adkcon = ADKCONF_UARTBRK;
		break;

	case TIOCSDTR:
		(void) sermctl(dev, TIOCM_DTR, DMBIS);
		break;

	case TIOCCDTR:
		(void) sermctl(dev, TIOCM_DTR, DMBIC);
		break;

	case TIOCMSET:
		(void) sermctl(dev, *(int *) data, DMSET);
		break;

	case TIOCMBIS:
		(void) sermctl(dev, *(int *) data, DMBIS);
		break;

	case TIOCMBIC:
		(void) sermctl(dev, *(int *) data, DMBIC);
		break;

	case TIOCMGET:
		*(int *)data = sermctl(dev, 0, DMGET);
		break;
	case TIOCGFLAGS:
		*(int *)data = serswflags;
		break;
	case TIOCSFLAGS:
		error = suser(p->p_ucred, &p->p_acflag);
		if (error != 0)
			return(EPERM);

		serswflags = *(int *)data;
                serswflags &= /* only allow valid flags */
                  (TIOCFLAG_SOFTCAR | TIOCFLAG_CLOCAL | TIOCFLAG_CRTSCTS);
		break;
	default:
		return(EPASSTHROUGH);
	}

	return(0);
}

int
serparam(struct tty *tp, struct termios *t)
{
	int cflag, ospeed = 0;

	if (t->c_ospeed > 0) {
		if (t->c_ospeed < 110)
			return(EINVAL);
		ospeed = SERBRD(t->c_ospeed);
	}

	if (t->c_ispeed && t->c_ispeed != t->c_ospeed)
		return(EINVAL);

	if (serswflags & TIOCFLAG_SOFTCAR || serconsole == 0) {
		t->c_cflag = (t->c_cflag & ~HUPCL) | CLOCAL;
	}

	/* if no changes, dont do anything. com.c explains why. */
	if (tp->t_ospeed == t->c_ospeed &&
	    tp->t_cflag == t->c_cflag)
		return (0);

	cflag = t->c_cflag;

	if (cflag & (CLOCAL | MDMBUF))
		serdcd = 0;
	else
		serdcd = CIAB_PRA_CD;

	/* TODO: support multiple flow control protocols like com.c */

	/*
	 * copy to tty
	 */
	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = cflag;
	ser_open_speed = tp->t_ispeed;

	/*
	 * enable interrupts
	 */
	custom.intena = INTF_SETCLR | INTF_RBF | INTF_TBE;
	last_ciab_pra = ciab.pra;

	if (t->c_ospeed == 0)
		(void)sermctl(tp->t_dev, 0, DMSET);	/* hang up line */
	else {
		/*
		 * (re)enable DTR
		 * and set baud rate. (8 bit mode)
		 */
		(void)sermctl(tp->t_dev, TIOCM_DTR, DMSET);
		custom.serper = (0 << 15) | ospeed;
	}
	(void)tp->t_linesw->l_modem(tp, ISDCD(last_ciab_pra));

	return(0);
}

int serhwiflow(struct tty *tp, int flag)
{
#if 0
	printf ("serhwiflow %d\n", flag);
#endif
        if (flag)
		CLRRTS(ciab.pra);
	else
	        SETRTS(ciab.pra);
        return 1;
}

static void
ser_putchar(struct tty *tp, u_short c)
{
	if ((tp->t_cflag & CSIZE) == CS7 || (tp->t_cflag & PARENB))
		c &= 0x7f;

	/*
	 * handle parity if necessary
	 */
	if (tp->t_cflag & PARENB) {
		if (even_parity[c])
			c |= 0x80;
		if (tp->t_cflag & PARODD)
			c ^= 0x80;
	}
	/*
	 * add stop bit(s)
	 */
	if (tp->t_cflag & CSTOPB)
		c |= 0x300;
	else
		c |= 0x100;

	custom.serdat = c;
}


static u_char ser_outbuf[SEROBUF_SIZE];
static u_char *sob_ptr = ser_outbuf, *sob_end = ser_outbuf;

void
ser_outintr(void)
{
	struct tty *tp;
	int s;

	tp = ser_tty;
	s = spltty();

	if (tp == 0)
		goto out;

	if ((custom.intreqr & INTF_TBE) == 0)
		goto out;

	/*
	 * clear interrupt
	 */
	custom.intreq = INTF_TBE;

	if (sob_ptr == sob_end) {
		tp->t_state &= ~(TS_BUSY | TS_FLUSH);
		if (tp->t_linesw)
			tp->t_linesw->l_start(tp);
		else
			serstart(tp);
		goto out;
	}

	/*
	 * Do hardware flow control here.  if the CTS line goes down, don't
	 * transmit anything.  That way, we'll be restarted by the periodic
	 * interrupt when CTS comes back up.
	 */
	if (ISCTS(ciab.pra))
		ser_putchar(tp, *sob_ptr++);
	else
		CLRCTS(last_ciab_pra);	/* Remember that CTS is off */
out:
	splx(s);
}

void
serstart(struct tty *tp)
{
	int cc, s, hiwat;
#ifdef DIAGNOSTIC
	int unit;
#endif

	hiwat = 0;

	if ((tp->t_state & TS_ISOPEN) == 0)
		return;

#ifdef DIAGNOSTIC
	unit = SERUNIT(tp->t_dev);
	if (unit)
		panic("serstart: unit is %d\n", unit);
#endif

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT | TS_TTSTOP))
		goto out;

	cc = tp->t_outq.c_cc;
	if (cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t) & tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
	}
	if (cc == 0 || (tp->t_state & TS_BUSY))
		goto out;

	/*
	 * We only do bulk transfers if using CTSRTS flow control, not for
	 * (probably sloooow) ixon/ixoff devices.
	 */
	if ((tp->t_cflag & CRTSCTS) == 0)
		cc = 1;

	/*
	 * Limit the amount of output we do in one burst
	 * to prevent hogging the CPU.
	 */
	if (cc > SEROBUF_SIZE) {
		hiwat++;
		cc = SEROBUF_SIZE;
	}
	cc = q_to_b(&tp->t_outq, ser_outbuf, cc);
	if (cc > 0) {
		tp->t_state |= TS_BUSY;

		sob_ptr = ser_outbuf;
		sob_end = ser_outbuf + cc;

		/*
		 * Get first character out, then have TBE-interrupts blow out
		 * further characters, until buffer is empty, and TS_BUSY gets
		 * cleared.
		 */
		ser_putchar(tp, *sob_ptr++);
	}
out:
	splx(s);
}

/*
 * Stop output on a line.
 */
/*ARGSUSED*/
void
serstop(struct tty *tp, int flag)
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
sermctl(dev_t dev, int bits, int how)
{
	int s;
	u_char ub = 0;

	/*
	 * convert TIOCM* mask into CIA mask
	 * which is active low
	 */
	if (how != DMGET) {
		ub = 0;
		if (bits & TIOCM_DTR)
			ub |= CIAB_PRA_DTR;
		if (bits & TIOCM_RTS)
			ub |= CIAB_PRA_RTS;
		if (bits & TIOCM_CTS)
			ub |= CIAB_PRA_CTS;
		if (bits & TIOCM_CD)
			ub |= CIAB_PRA_CD;
		if (bits & TIOCM_RI)
			ub |= CIAB_PRA_SEL;	/* collision with /dev/par ! */
		if (bits & TIOCM_DSR)
			ub |= CIAB_PRA_DSR;
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
	(void)splx(s);

	bits = 0;
	if (ub & CIAB_PRA_DTR)
		bits |= TIOCM_DTR;
	if (ub & CIAB_PRA_RTS)
		bits |= TIOCM_RTS;
	if (ub & CIAB_PRA_CTS)
		bits |= TIOCM_CTS;
	if (ub & CIAB_PRA_CD)
		bits |= TIOCM_CD;
	if (ub & CIAB_PRA_SEL)
		bits |= TIOCM_RI;
	if (ub & CIAB_PRA_DSR)
		bits |= TIOCM_DSR;

	return(bits);
}

/*
 * Following are all routines needed for SER to act as console
 */
void
sercnprobe(struct consdev *cp)
{
	int unit;

	/* locate the major number */
	for (sermajor = 0; sermajor < nchrdev; sermajor++)
		if (cdevsw[sermajor].d_open == (void *)seropen)
			break;


	unit = CONUNIT;			/* XXX: ick */

	/*
	 * initialize required fields
	 */
	cp->cn_dev = makedev(sermajor, unit);
	if (serconsole == unit)
		cp->cn_pri = CN_REMOTE;
	else
		cp->cn_pri = CN_NORMAL;
#ifdef KGDB
	if (major(kgdb_dev) == 1)	/* XXX */
		kgdb_dev = makedev(sermajor, minor(kgdb_dev));
#endif
}

void
sercninit(struct consdev *cp)
{
	int unit;

	unit = SERUNIT(cp->cn_dev);

	serinit(serdefaultrate);
	serconsole = unit;
	serconsinit = 1;
}

void
serinit(int rate)
{
	int s;

	s = splser();
	/*
	 * might want to fiddle with the CIA later ???
	 */
	custom.serper = (rate>=110 ? SERBRD(rate) : 0);
	splx(s);
}

int
sercngetc(dev_t dev)
{
	u_short stat;
	int c, s;

	s = splser();
	/*
	 * poll
	 */
	while (((stat = custom.serdatr & 0xffff) & SERDATRF_RBF) == 0)
		;
	c = stat & 0xff;
	/*
	 * clear interrupt
	 */
	custom.intreq = INTF_RBF;
	splx(s);
	return(c);
}

/*
 * Console kernel output character routine.
 */
void
sercnputc(dev_t dev, int c)
{
	register int timo;
	int s;

	s = splhigh();

	if (serconsinit == 0) {
		(void)serinit(serdefaultrate);
		serconsinit = 1;
	}

	/*
	 * wait for any pending transmission to finish
	 */
	timo = 50000;
	while (!(custom.serdatr & SERDATRF_TBE) && --timo);

	/*
	 * transmit char.
	 */
	custom.serdat = (c & 0xff) | 0x100;

	/*
	 * wait for this transmission to complete
	 */
	timo = 1500000;
	while (!(custom.serdatr & SERDATRF_TBE) && --timo)
		;

	/*
	 * Wait for the device (my vt100..) to process the data, since we
	 * don't do flow-control with cnputc
	 */
	for (timo = 0; timo < 30000; timo++)
		;

	/*
	 * We set TBE so that ser_outintr() is called right after to check
	 * whether there still are chars to process.
	 * We used to clear this, but it hung the tty output if the kernel
	 * output a char while userland did on the same serial port.
	 */
	custom.intreq = INTF_SETCLR | INTF_TBE;
	splx(s);
}

void
sercnpollc(dev_t dev, int on)
{
}
#endif

/*	$NetBSD: zs.c,v 1.1.1.1 1995/03/26 07:12:14 leo Exp $	*/

/*
 * Copyright (c) 1995 L. Weppelman (Atari modifications)
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)zs.c	8.1 (Berkeley) 7/19/93
 */

/*
 * Zilog Z8530 (ZSCC) driver.
 *
 * Runs two tty ports (modem2 and serial2) on zs0.
 *
 * This driver knows far too much about chip to usage mappings.
 */
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/syslog.h>

#include <machine/cpu.h>
#include <machine/iomap.h>
#include <machine/scu.h>
#include <machine/mfp.h>

#include <atari/dev/zsreg.h>
#include <atari/dev/zsvar.h>
#include "zs.h"
#if NZS > 1
#error "This driver supports only 1 85C30!"
#endif

#if NZS > 0

#define PCLK	(8000000)	/* PCLK pin input clock rate */

#define splzs	spl5

/*
 * Software state per found chip.
 */
struct zs_softc {
    struct	device		zi_dev;    /* base device		  */
    volatile struct zsdevice	*zi_zs;    /* chip registers		  */
    struct	zs_chanstate	zi_cs[2];  /* chan A and B software state */
};

/*
 * Define the registers for a closed port
 */
u_char zs_init_regs[16] = {
/*  0 */	0,
/*  1 */	0,
/*  2 */	0x60,
/*  3 */	0,
/*  4 */	0,
/*  5 */	0,
/*  6 */	0,
/*  7 */	0,
/*  8 */	0,
/*  9 */	ZSWR9_VECTOR_INCL_STAT,
/* 10 */	ZSWR10_NRZ,
/* 11 */	ZSWR11_TXCLK_BAUD | ZSWR11_RXCLK_BAUD,
/* 12 */	0,
/* 13 */	0,
/* 14 */	ZSWR14_BAUD_FROM_PCLK | ZSWR14_BAUD_ENA,
/* 15 */	0
};

struct tty *zs_tty[NZS * 2];		/* XXX should be dynamic */

/* Definition of the driver for autoconfig. */
static int	zsmatch __P((struct device *, struct cfdata *, void *));
static void	zsattach __P((struct device *, struct device *, void *));
struct cfdriver zscd = {
	NULL, "zs", (cfmatch_t)zsmatch, zsattach, DV_TTY,
	sizeof(struct zs_softc), NULL, 0 };

/* Interrupt handlers. */
int		zshard __P((long));
static int	zssoft __P((long));
static int	zsrint __P((struct zs_chanstate *, volatile struct zschan *));
static int	zsxint __P((struct zs_chanstate *, volatile struct zschan *));
static int	zssint __P((struct zs_chanstate *, volatile struct zschan *));

struct zs_chanstate *zslist;

/* Routines called from other code. */
static void	zsstart __P((struct tty *));
void		zsstop __P((struct tty *, int));
static int	zsparam __P((struct tty *, struct termios *));

/* Routines purely local to this driver. */
static void	zs_reset __P((volatile struct zschan *, int, int));
static int	zs_modem __P((struct zs_chanstate *, int, int));
static void	zs_loadchannelregs __P((volatile struct zschan *, u_char *));

int zsshortcuts;	/* number of "shortcut" software interrupts */

static int zsmatch(pdp, cfp, auxp)
struct device	*pdp;
struct cfdata	*cfp;
void		*auxp;
{
	if(strcmp("zs", auxp) || cfp->cf_unit != 0)
		return(0);
	return(1);
}

/*
 * Attach a found zs.
 */
static void
zsattach(parent, dev, aux)
struct device	*parent;
struct device	*dev;
void		*aux;
{
	register struct zs_softc		*zi;
	register struct zs_chanstate		*cs;
	register volatile struct zsdevice	*addr;
	register struct tty			*tp;
		 char				tmp;

	addr      = (struct zsdevice *)AD_SCC;
	zi        = (struct zs_softc *)dev;
	zi->zi_zs = addr;
	cs        = zi->zi_cs;

	/*
	 * Get the command register into a known state.
	 */
	tmp = addr->zs_chan[CHAN_A].zc_csr;
	tmp = addr->zs_chan[CHAN_A].zc_csr;
	tmp = addr->zs_chan[CHAN_B].zc_csr;
	tmp = addr->zs_chan[CHAN_B].zc_csr;

	/*
	 * Do a hardware reset.
	 */
	ZS_WRITE(&addr->zs_chan[CHAN_A], 9, ZSWR9_HARD_RESET);
	delay(50000);	/*enough ? */
	ZS_WRITE(&addr->zs_chan[CHAN_A], 9, 0);

	/*
	 * Initialize both channels
	 */
	zs_loadchannelregs(&addr->zs_chan[CHAN_A], zs_init_regs);
	zs_loadchannelregs(&addr->zs_chan[CHAN_B], zs_init_regs);

	/*
	 * enable scc related interrupts
	 */
	SCU->sys_mask |= SCU_SCC;

	/* link into interrupt list with order (A,B) (B=A+1) */
	cs[0].cs_next = &cs[1];
	cs[1].cs_next = zslist;
	zslist        = cs;

	cs->cs_unit  = 0;
	cs->cs_zc    = &addr->zs_chan[CHAN_A];
	cs++;
	cs->cs_unit  = 1;
	cs->cs_zc    = &addr->zs_chan[CHAN_B];

	printf(": serial2 on channel a and modem2 on channel b\n");
}

/*
 * Open a zs serial port.
 */
int
zsopen(dev, flags, mode, p)
dev_t		dev;
int		flags;
int		mode;
struct proc	*p;
{
	register struct tty		*tp;
	register struct zs_chanstate	*cs;
		 struct zs_softc	*zi;
		 int			unit = ZS_UNIT(dev);
		 int			zs = unit >> 1;
		 int			error, s;

	if(zs >= zscd.cd_ndevs || (zi = zscd.cd_devs[zs]) == NULL)
		return (ENXIO);
	cs = &zi->zi_cs[unit & 1];
	tp = cs->cs_ttyp;
	if(tp == NULL) {
		cs->cs_ttyp  = tp = zs_tty[unit] = ttymalloc();
		tp->t_dev    = dev;
		tp->t_oproc  = zsstart;
		tp->t_param  = zsparam;
	}

	s  = spltty();
	if((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);
		if(tp->t_ispeed == 0) {
			tp->t_iflag = TTYDEF_IFLAG;
			tp->t_oflag = TTYDEF_OFLAG;
			tp->t_cflag = TTYDEF_CFLAG;
			tp->t_lflag = TTYDEF_LFLAG;
			tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		}
		(void)zsparam(tp, &tp->t_termios);
		ttsetwater(tp);
	}
	else if(tp->t_state & TS_XCLUDE && p->p_ucred->cr_uid != 0) {
			splx(s);
			return (EBUSY);
	}
	error = 0;
	for(;;) {
		/* loop, turning on the device, until carrier present */
		zs_modem(cs, ZSWR5_RTS|ZSWR5_DTR, DMSET);
		if(cs->cs_softcar)
			tp->t_state |= TS_CARR_ON;
		if(flags & O_NONBLOCK || tp->t_cflag & CLOCAL ||
		    tp->t_state & TS_CARR_ON)
			break;
		tp->t_state |= TS_WOPEN;
		if(error = ttysleep(tp, (caddr_t)&tp->t_rawq, TTIPRI | PCATCH,
		    ttopen, 0)) {
			if(!(tp->t_state & TS_ISOPEN)) {
				zs_modem(cs, 0, DMSET);
				tp->t_state &= ~TS_WOPEN;
				ttwakeup(tp);
			}
			splx(s);
			return error;
		}
	}
	splx(s);
	if(error == 0)
		error = linesw[tp->t_line].l_open(dev, tp);
	if(error)
		zs_modem(cs, 0, DMSET);
	return(error);
}

/*
 * Close a zs serial port.
 */
int
zsclose(dev, flags, mode, p)
dev_t		dev;
int		flags;
int		mode;
struct proc	*p;
{
	register struct zs_chanstate	*cs;
	register struct tty		*tp;
		 struct zs_softc	*zi;
		 int			unit = ZS_UNIT(dev);
		 int			s;

	zi = zscd.cd_devs[unit >> 1];
	cs = &zi->zi_cs[unit & 1];
	tp = cs->cs_ttyp;
	linesw[tp->t_line].l_close(tp, flags);
	if(tp->t_cflag & HUPCL || tp->t_state & TS_WOPEN ||
	    (tp->t_state & TS_ISOPEN) == 0) {
		zs_modem(cs, 0, DMSET);
		/* hold low for 1 second */
		(void)tsleep((caddr_t)cs, TTIPRI, ttclos, hz);
	}
	if(cs->cs_creg[5] & ZSWR5_BREAK) {
		s = splzs();
		cs->cs_preg[5] &= ~ZSWR5_BREAK;
		cs->cs_creg[5] &= ~ZSWR5_BREAK;
		ZS_WRITE(cs->cs_zc, 5, cs->cs_creg[5]);
		splx(s);
	}
	ttyclose(tp);

	/*
	 * Drop all lines and cancel interrupts
	 */
	zs_loadchannelregs(&zi->zi_zs->zs_chan[unit & 1], zs_init_regs);
	return (0);
}

/*
 * Read/write zs serial port.
 */
int
zsread(dev, uio, flags)
dev_t		dev;
struct uio	*uio;
int		flags;
{
	register struct tty *tp = zs_tty[ZS_UNIT(dev)];

	return(linesw[tp->t_line].l_read(tp, uio, flags));
}

int zswrite(dev, uio, flags)
dev_t		dev;
struct uio	*uio;
int		flags;
{
	register struct tty *tp = zs_tty[ZS_UNIT(dev)];

	return(linesw[tp->t_line].l_write(tp, uio, flags));
}

/*
 * ZS hardware interrupt.  Scan all ZS channels.  NB: we know here that
 * channels are kept in (A,B) pairs.
 *
 * Do just a little, then get out; set a software interrupt if more
 * work is needed.
 *
 * We deliberately ignore the vectoring Zilog gives us, and match up
 * only the number of `reset interrupt under service' operations, not
 * the order.
 */
int
zshard(sr)
long sr;
{
	register struct zs_chanstate	*a;
#define	b (a + 1)
	register volatile struct zschan *zc;
	register int			rr3, intflags = 0, v, i;

	for(a = zslist; a != NULL; a = b->cs_next) {
		rr3 = ZS_READ(a->cs_zc, 3);
		if(rr3 & (ZSRR3_IP_A_RX|ZSRR3_IP_A_TX|ZSRR3_IP_A_STAT)) {
			intflags |= 2;
			zc = a->cs_zc;
			i  = a->cs_rbput;
			if(rr3 & ZSRR3_IP_A_RX && (v = zsrint(a, zc)) != 0) {
				a->cs_rbuf[i++ & ZLRB_RING_MASK] = v;
				intflags |= 1;
			}
			if(rr3 & ZSRR3_IP_A_TX && (v = zsxint(a, zc)) != 0) {
				a->cs_rbuf[i++ & ZLRB_RING_MASK] = v;
				intflags |= 1;
			}
			if(rr3 & ZSRR3_IP_A_STAT && (v = zssint(a, zc)) != 0) {
				a->cs_rbuf[i++ & ZLRB_RING_MASK] = v;
				intflags |= 1;
			}
			a->cs_rbput = i;
		}
		if(rr3 & (ZSRR3_IP_B_RX|ZSRR3_IP_B_TX|ZSRR3_IP_B_STAT)) {
			intflags |= 2;
			zc = b->cs_zc;
			i  = b->cs_rbput;
			if(rr3 & ZSRR3_IP_B_RX && (v = zsrint(b, zc)) != 0) {
				b->cs_rbuf[i++ & ZLRB_RING_MASK] = v;
				intflags |= 1;
			}
			if(rr3 & ZSRR3_IP_B_TX && (v = zsxint(b, zc)) != 0) {
				b->cs_rbuf[i++ & ZLRB_RING_MASK] = v;
				intflags |= 1;
			}
			if(rr3 & ZSRR3_IP_B_STAT && (v = zssint(b, zc)) != 0) {
				b->cs_rbuf[i++ & ZLRB_RING_MASK] = v;
				intflags |= 1;
			}
			b->cs_rbput = i;
		}
	}
#undef b

	if(intflags & 1) {
		if(BASEPRI(sr)) {
			spl1();
			zsshortcuts++;
			return(zssoft(sr));
		}
		else add_sicallback(zssoft, 0, 0);
	}
	return(intflags & 2);
}

static int
zsrint(cs, zc)
register struct zs_chanstate	*cs;
register volatile struct zschan	*zc;
{
	register int c = zc->zc_data;

	/* compose receive character and status */
	c <<= 8;
	c |= ZS_READ(zc, 1);

	/* clear receive error & interrupt condition */
	zc->zc_csr = ZSWR0_RESET_ERRORS;
	zc->zc_csr = ZSWR0_CLR_INTR;

	return(ZRING_MAKE(ZRING_RINT, c));
}

static int
zsxint(cs, zc)
register struct zs_chanstate	*cs;
register volatile struct zschan	*zc;
{
	register int i = cs->cs_tbc;

	if(i == 0) {
		zc->zc_csr = ZSWR0_RESET_TXINT;
		zc->zc_csr = ZSWR0_CLR_INTR;
		return(ZRING_MAKE(ZRING_XINT, 0));
	}
	cs->cs_tbc = i - 1;
	zc->zc_data = *cs->cs_tba++;
	zc->zc_csr = ZSWR0_CLR_INTR;
	return (0);
}

static int
zssint(cs, zc)
register struct zs_chanstate	*cs;
register volatile struct zschan	*zc;
{
	register int rr0;

	rr0 = zc->zc_csr;
	zc->zc_csr = ZSWR0_RESET_STATUS;
	zc->zc_csr = ZSWR0_CLR_INTR;
	/*
	 * The chip's hardware flow control is, as noted in zsreg.h,
	 * busted---if the DCD line goes low the chip shuts off the
	 * receiver (!).  If we want hardware CTS flow control but do
	 * not have it, and carrier is now on, turn HFC on; if we have
	 * HFC now but carrier has gone low, turn it off.
	 */
	if(rr0 & ZSRR0_DCD) {
		if(cs->cs_ttyp->t_cflag & CCTS_OFLOW &&
		    (cs->cs_creg[3] & ZSWR3_HFC) == 0) {
			cs->cs_creg[3] |= ZSWR3_HFC;
			ZS_WRITE(zc, 3, cs->cs_creg[3]);
		}
	}
	else {
		if (cs->cs_creg[3] & ZSWR3_HFC) {
			cs->cs_creg[3] &= ~ZSWR3_HFC;
			ZS_WRITE(zc, 3, cs->cs_creg[3]);
		}
	}
	return(ZRING_MAKE(ZRING_SINT, rr0));
}

/*
 * Print out a ring or fifo overrun error message.
 */
static void
zsoverrun(unit, ptime, what)
int	unit;
long	*ptime;
char	*what;
{

	if(*ptime != time.tv_sec) {
		*ptime = time.tv_sec;
		log(LOG_WARNING, "zs%d%c: %s overrun\n", unit >> 1,
		    (unit & 1) + 'a', what);
	}
}

/*
 * ZS software interrupt.  Scan all channels for deferred interrupts.
 */
int
zssoft(sr)
long sr;
{
    register struct zs_chanstate	*cs;
    register volatile struct zschan	*zc;
    register struct linesw		*line;
    register struct tty			*tp;
    register int			get, n, c, cc, unit, s;
 	     int			retval = 0;

    s = spltty();
    for(cs = zslist; cs != NULL; cs = cs->cs_next) {
	get = cs->cs_rbget;
again:
	n = cs->cs_rbput;	/* atomic			*/
	if(get == n)		/* nothing more on this line	*/
		continue;
	retval = 1;
	unit   = cs->cs_unit;	/* set up to handle interrupts	*/
	zc     = cs->cs_zc;
	tp     = cs->cs_ttyp;
	line   = &linesw[tp->t_line];
	/*
	 * Compute the number of interrupts in the receive ring.
	 * If the count is overlarge, we lost some events, and
	 * must advance to the first valid one.  It may get
	 * overwritten if more data are arriving, but this is
	 * too expensive to check and gains nothing (we already
	 * lost out; all we can do at this point is trade one
	 * kind of loss for another).
	 */
	n -= get;
	if(n > ZLRB_RING_SIZE) {
		zsoverrun(unit, &cs->cs_rotime, "ring");
		get += n - ZLRB_RING_SIZE;
		n    = ZLRB_RING_SIZE;
	}
	while(--n >= 0) {
		/* race to keep ahead of incoming interrupts */
		c = cs->cs_rbuf[get++ & ZLRB_RING_MASK];
		switch (ZRING_TYPE(c)) {

		case ZRING_RINT:
			c = ZRING_VALUE(c);
			if(c & ZSRR1_DO)
				zsoverrun(unit, &cs->cs_fotime, "fifo");
			cc = c >> 8;
			if(c & ZSRR1_FE)
				cc |= TTY_FE;
			if(c & ZSRR1_PE)
				cc |= TTY_PE;
			line->l_rint(cc, tp);
			break;

		case ZRING_XINT:
			/*
			 * Transmit done: change registers and resume,
			 * or clear BUSY.
			 */
			if(cs->cs_heldchange) {
				int sps;

				sps = splzs();
				c = zc->zc_csr;
				if((c & ZSRR0_DCD) == 0)
					cs->cs_preg[3] &= ~ZSWR3_HFC;
				bcopy((caddr_t)cs->cs_preg,
				    (caddr_t)cs->cs_creg, 16);
				zs_loadchannelregs(zc, cs->cs_creg);
				splx(sps);
				cs->cs_heldchange = 0;
				if(cs->cs_heldtbc
					&& (tp->t_state & TS_TTSTOP) == 0) {
					cs->cs_tbc = cs->cs_heldtbc - 1;
					zc->zc_data = *cs->cs_tba++;
					goto again;
				}
			}
			tp->t_state &= ~TS_BUSY;
			if(tp->t_state & TS_FLUSH)
				tp->t_state &= ~TS_FLUSH;
			else ndflush(&tp->t_outq,cs->cs_tba
						- (caddr_t)tp->t_outq.c_cf);
			line->l_start(tp);
			break;

		case ZRING_SINT:
			/*
			 * Status line change.  HFC bit is run in
			 * hardware interrupt, to avoid locking
			 * at splzs here.
			 */
			c = ZRING_VALUE(c);
			if((c ^ cs->cs_rr0) & ZSRR0_DCD) {
				cc = (c & ZSRR0_DCD) != 0;
				if(line->l_modem(tp, cc) == 0)
					zs_modem(cs, ZSWR5_RTS|ZSWR5_DTR,
							cc ? DMBIS : DMBIC);
			}
			cs->cs_rr0 = c;
			break;

		default:
			log(LOG_ERR, "zs%d%c: bad ZRING_TYPE (%x)\n",
			    unit >> 1, (unit & 1) + 'a', c);
			break;
		}
	}
	cs->cs_rbget = get;
	goto again;
    }
    splx(s);
    return (retval);
}

int
zsioctl(dev, cmd, data, flag, p)
dev_t		dev;
u_long		cmd;
caddr_t		data;
int		flag;
struct proc	*p;
{
		 int			unit = ZS_UNIT(dev);
		 struct zs_softc	*zi = zscd.cd_devs[unit >> 1];
	register struct tty		*tp = zi->zi_cs[unit & 1].cs_ttyp;
	register int			error, s;
	register struct zs_chanstate	*cs = &zi->zi_cs[unit & 1];

	error = linesw[tp->t_line].l_ioctl(tp, cmd, data, flag, p);
	if(error >= 0)
		return(error);
	error = ttioctl(tp, cmd, data, flag, p);
	if(error >= 0)
		return (error);

	switch (cmd) {
	case TIOCSBRK:
		s = splzs();
		cs->cs_preg[5] |= ZSWR5_BREAK;
		cs->cs_creg[5] |= ZSWR5_BREAK;
		ZS_WRITE(cs->cs_zc, 5, cs->cs_creg[5]);
		splx(s);
		break;
	case TIOCCBRK:
		s = splzs();
		cs->cs_preg[5] &= ~ZSWR5_BREAK;
		cs->cs_creg[5] &= ~ZSWR5_BREAK;
		ZS_WRITE(cs->cs_zc, 5, cs->cs_creg[5]);
		splx(s);
		break;
	case TIOCGFLAGS: {
		int bits = 0;

		if(cs->cs_softcar)
			bits |= TIOCFLAG_SOFTCAR;
		if(cs->cs_creg[15] & ZSWR15_DCD_IE)
			bits |= TIOCFLAG_CLOCAL;
		if(cs->cs_creg[3] & ZSWR3_HFC)
			bits |= TIOCFLAG_CRTSCTS;
		*(int *)data = bits;
		break;
	}
	case TIOCSFLAGS: {
		int userbits, driverbits = 0;

		error = suser(p->p_ucred, &p->p_acflag);
		if(error != 0)
			return (EPERM);

		userbits = *(int *)data;

		/*
		 * can have `local' or `softcar', and `rtscts' or `mdmbuf'
		 # defaulting to software flow control.
		 */
		if(userbits & TIOCFLAG_SOFTCAR && userbits & TIOCFLAG_CLOCAL)
			return(EINVAL);
		if(userbits & TIOCFLAG_MDMBUF)	/* don't support this (yet?) */
			return(ENXIO);

		s = splzs();
		if((userbits & TIOCFLAG_SOFTCAR)) {
			cs->cs_softcar = 1;	/* turn on softcar */
			cs->cs_preg[15] &= ~ZSWR15_DCD_IE; /* turn off dcd */
			cs->cs_creg[15] &= ~ZSWR15_DCD_IE;
			ZS_WRITE(cs->cs_zc, 15, cs->cs_creg[15]);
		}
		else if(userbits & TIOCFLAG_CLOCAL) {
			cs->cs_softcar = 0; 	/* turn off softcar */
			cs->cs_preg[15] |= ZSWR15_DCD_IE; /* turn on dcd */
			cs->cs_creg[15] |= ZSWR15_DCD_IE;
			ZS_WRITE(cs->cs_zc, 15, cs->cs_creg[15]);
			tp->t_termios.c_cflag |= CLOCAL;
		}
		if(userbits & TIOCFLAG_CRTSCTS) {
			cs->cs_preg[15] |= ZSWR15_CTS_IE;
			cs->cs_creg[15] |= ZSWR15_CTS_IE;
			ZS_WRITE(cs->cs_zc, 15, cs->cs_creg[15]);
			cs->cs_preg[3] |= ZSWR3_HFC;
			cs->cs_creg[3] |= ZSWR3_HFC;
			ZS_WRITE(cs->cs_zc, 3, cs->cs_creg[3]);
			tp->t_termios.c_cflag |= CRTSCTS;
		}
		else {
			/* no mdmbuf, so we must want software flow control */
			cs->cs_preg[15] &= ~ZSWR15_CTS_IE;
			cs->cs_creg[15] &= ~ZSWR15_CTS_IE;
			ZS_WRITE(cs->cs_zc, 15, cs->cs_creg[15]);
			cs->cs_preg[3] &= ~ZSWR3_HFC;
			cs->cs_creg[3] &= ~ZSWR3_HFC;
			ZS_WRITE(cs->cs_zc, 3, cs->cs_creg[3]);
			tp->t_termios.c_cflag &= ~CRTSCTS;
		}
		splx(s);
		break;
	}
	case TIOCSDTR:
		zs_modem(cs, ZSWR5_DTR, DMBIS);
		break;
	case TIOCCDTR:
		zs_modem(cs, ZSWR5_DTR, DMBIC);
		break;
	case TIOCMGET:
		zs_modem(cs, 0, DMGET);
		break;
	case TIOCMSET:
	case TIOCMBIS:
	case TIOCMBIC:
	default:
		return (ENOTTY);
	}
	return (0);
}

/*
 * Start or restart transmission.
 */
static void
zsstart(tp)
register struct tty *tp;
{
	register struct zs_chanstate	*cs;
	register int			s, nch;
		 int			unit = ZS_UNIT(tp->t_dev);
		 struct zs_softc	*zi = zscd.cd_devs[unit >> 1];

	cs = &zi->zi_cs[unit & 1];
	s  = spltty();

	/*
	 * If currently active or delaying, no need to do anything.
	 */
	if(tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP))
		goto out;

	/*
	 * If there are sleepers, and output has drained below low
	 * water mark, awaken.
	 */
	if(tp->t_outq.c_cc <= tp->t_lowat) {
		if(tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
	}

	nch = ndqb(&tp->t_outq, 0);	/* XXX */
	if(nch) {
		register char *p = tp->t_outq.c_cf;

		/* mark busy, enable tx done interrupts, & send first byte */
		tp->t_state |= TS_BUSY;
		(void) splzs();
		cs->cs_preg[1] |= ZSWR1_TIE;
		cs->cs_creg[1] |= ZSWR1_TIE;
		ZS_WRITE(cs->cs_zc, 1, cs->cs_creg[1]);
		cs->cs_zc->zc_data = *p;
		cs->cs_tba = p + 1;
		cs->cs_tbc = nch - 1;
	} else {
		/*
		 * Nothing to send, turn off transmit done interrupts.
		 * This is useful if something is doing polled output.
		 */
		(void) splzs();
		cs->cs_preg[1] &= ~ZSWR1_TIE;
		cs->cs_creg[1] &= ~ZSWR1_TIE;
		ZS_WRITE(cs->cs_zc, 1, cs->cs_creg[1]);
	}
out:
	splx(s);
}

/*
 * Stop output, e.g., for ^S or output flush.
 */
void
zsstop(tp, flag)
register struct tty	*tp;
	 int		flag;
{
	register struct zs_chanstate	*cs;
	register int			s, unit = ZS_UNIT(tp->t_dev);
		 struct zs_softc	*zi = zscd.cd_devs[unit >> 1];

	cs = &zi->zi_cs[unit & 1];
	s  = splzs();
	if(tp->t_state & TS_BUSY) {
		/*
		 * Device is transmitting; must stop it.
		 */
		cs->cs_tbc = 0;
		if ((tp->t_state & TS_TTSTOP) == 0)
			tp->t_state |= TS_FLUSH;
	}
	splx(s);
}

/*
 * Set ZS tty parameters from termios.
 *
 * This routine makes use of the fact that only registers
 * 1, 3, 4, 5, 9, 10, 11, 12, 13, 14, and 15 are written.
 */
static int
zsparam(tp, t)
register struct tty	*tp;
register struct termios	*t;
{
		 int			unit = ZS_UNIT(tp->t_dev);
		 struct zs_softc	*zi = zscd.cd_devs[unit >> 1];
	register struct zs_chanstate	*cs = &zi->zi_cs[unit & 1];
	register int			tmp, tmp5, cflag, s;

	tmp = t->c_ospeed;
	if(tmp < 0 || (t->c_ispeed && t->c_ispeed != tmp))
		return(EINVAL);
	if(tmp == 0) {
		/* stty 0 => drop DTR and RTS */
		zs_modem(cs, 0, DMSET);
		return(0);
	}
	tmp = BPS_TO_TCONST(PCLK / 16, tmp);
	if(tmp < 2)
		return(EINVAL);

	cflag = t->c_cflag;
	tp->t_ispeed = tp->t_ospeed = TCONST_TO_BPS(PCLK / 16, tmp);
	tp->t_cflag = cflag;

	/*
	 * Block interrupts so that state will not
	 * be altered until we are done setting it up.
	 */
	s = splzs();
	cs->cs_preg[12] = tmp;
	cs->cs_preg[13] = tmp >> 8;
	cs->cs_preg[1]  = ZSWR1_RIE | ZSWR1_TIE | ZSWR1_SIE;
	switch(cflag & CSIZE) {
	case CS5:
		tmp  = ZSWR3_RX_5;
		tmp5 = ZSWR5_TX_5;
		break;
	case CS6:
		tmp  = ZSWR3_RX_6;
		tmp5 = ZSWR5_TX_6;
		break;
	case CS7:
		tmp  = ZSWR3_RX_7;
		tmp5 = ZSWR5_TX_7;
		break;
	case CS8:
	default:
		tmp  = ZSWR3_RX_8;
		tmp5 = ZSWR5_TX_8;
		break;
	}

	/*
	 * Output hardware flow control on the chip is horrendous: if
	 * carrier detect drops, the receiver is disabled.  Hence we
	 * can only do this when the carrier is on.
	 */
	if(cflag & CCTS_OFLOW && cs->cs_zc->zc_csr & ZSRR0_DCD)
		tmp |= ZSWR3_HFC | ZSWR3_RX_ENABLE;
	else tmp |= ZSWR3_RX_ENABLE;
	cs->cs_preg[3] = tmp;
	cs->cs_preg[5] = tmp5 | ZSWR5_TX_ENABLE | ZSWR5_DTR | ZSWR5_RTS;

	tmp = ZSWR4_CLK_X16 | (cflag & CSTOPB ? ZSWR4_TWOSB : ZSWR4_ONESB);
	if((cflag & PARODD) == 0)
		tmp |= ZSWR4_EVENP;
	if (cflag & PARENB)
		tmp |= ZSWR4_PARENB;
	cs->cs_preg[4]  = tmp;
	cs->cs_preg[9]  = ZSWR9_MASTER_IE | ZSWR9_VECTOR_INCL_STAT;
	cs->cs_preg[10] = ZSWR10_NRZ;
	cs->cs_preg[11] = ZSWR11_TXCLK_BAUD | ZSWR11_RXCLK_BAUD;
	cs->cs_preg[14] = ZSWR14_BAUD_FROM_PCLK | ZSWR14_BAUD_ENA;
	cs->cs_preg[15] = ZSWR15_BREAK_IE | ZSWR15_DCD_IE;

	/*
	 * If nothing is being transmitted, set up new current values,
	 * else mark them as pending.
	 */
	if(cs->cs_heldchange == 0) {
		if (cs->cs_ttyp->t_state & TS_BUSY) {
			cs->cs_heldtbc = cs->cs_tbc;
			cs->cs_tbc = 0;
			cs->cs_heldchange = 1;
		}
		else {
			bcopy((caddr_t)cs->cs_preg, (caddr_t)cs->cs_creg, 16);
			zs_loadchannelregs(cs->cs_zc, cs->cs_creg);
		}
	}
	splx(s);
	return (0);
}

/*
 * Raise or lower modem control (DTR/RTS) signals.  If a character is
 * in transmission, the change is deferred.
 */
static int
zs_modem(cs, bits, how)
struct zs_chanstate	*cs;
int			bits, how;
{
	int s, mbits;

	bits  &= ZSWR5_DTR | ZSWR5_RTS;

	s = splzs();
	mbits  = cs->cs_preg[5] &  (ZSWR5_DTR | ZSWR5_RTS);

	switch(how) {
		case DMSET:
				mbits  = bits;
				break;
		case DMBIS:
				mbits |= bits;
				break;
		case DMBIC:
				mbits &= ~bits;
				break;
		case DMGET:
				splx(s);
				return(mbits);
	}

	cs->cs_preg[5] = (cs->cs_preg[5] & ~(ZSWR5_DTR | ZSWR5_RTS)) | mbits;
	if(cs->cs_heldchange == 0) {
		if(cs->cs_ttyp->t_state & TS_BUSY) {
			cs->cs_heldtbc = cs->cs_tbc;
			cs->cs_tbc = 0;
			cs->cs_heldchange = 1;
		}
		else {
			ZS_WRITE(cs->cs_zc, 5, cs->cs_creg[5]);
		}
	}
	splx(s);
	return(0);
}

/*
 * Write the given register set to the given zs channel in the proper order.
 * The channel must not be transmitting at the time.  The receiver will
 * be disabled for the time it takes to write all the registers.
 */
static void
zs_loadchannelregs(zc, reg)
volatile struct zschan	*zc;
u_char			*reg;
{
	int i;

	zc->zc_csr = ZSM_RESET_ERR;	/* reset error condition */
	i = zc->zc_data;		/* drain fifo */
	i = zc->zc_data;
	i = zc->zc_data;
	ZS_WRITE(zc,  4, reg[4]);
	ZS_WRITE(zc, 10, reg[10]);
	ZS_WRITE(zc,  3, reg[3] & ~ZSWR3_RX_ENABLE);
	ZS_WRITE(zc,  5, reg[5] & ~ZSWR5_TX_ENABLE);
	ZS_WRITE(zc,  1, reg[1]);
	ZS_WRITE(zc,  9, reg[9]);
	ZS_WRITE(zc, 11, reg[11]);
	ZS_WRITE(zc, 12, reg[12]);
	ZS_WRITE(zc, 13, reg[13]);
	ZS_WRITE(zc, 14, reg[14]);
	ZS_WRITE(zc, 15, reg[15]);
	ZS_WRITE(zc,  3, reg[3]);
	ZS_WRITE(zc,  5, reg[5]);
}
#endif /* NZS > 1 */

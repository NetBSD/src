/*	$NetBSD: dhu.c,v 1.1 1996/03/02 13:30:53 ragge Exp $	*/
/*
 * Copyright (c) 1996  Ken C. Wellsch.  All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
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
 */

#include "dhu.h"

#if NDHU > 0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <machine/trap.h>

#include <vax/uba/ubavar.h>

#include <vax/uba/dhureg.h>

/* A DHU has 16 ports while a DHV has only 8 */

#define	NDHULINE 	(NDHU*16)

struct	dhu_softc
{
	dhuregs *	sc_addr;	/* controller reg address */
	struct	tty *	sc_tty;		/* what we work on */
	char *		sc_mem;		/* pointers to current tx */
	char *		sc_end;		/* character and end */
};

struct	dhu_softc dhu_softc[NDHULINE];

/*  On a stock DHV, channel pairs (0/1, 2/3, etc.) must use */
/* a baud rate from the same group.  So limiting to B is likely */
/* best, although clone boards like the ABLE QHV allow all settings. */

static struct speedtab dhuspeedtab[] =
{
  {       0,	0		},	/* Groups  */
  {      50,	DHU_LPR_B50	},	/* A	   */
  {      75,	DHU_LPR_B75	},	/* 	 B */
  {     110,	DHU_LPR_B110	},	/* A and B */
  {     134,	DHU_LPR_B134	},	/* A and B */
  {     150,	DHU_LPR_B150	},	/* 	 B */
  {     300,	DHU_LPR_B300	},	/* A and B */
  {     600,	DHU_LPR_B600	},	/* A and B */
  {    1200,	DHU_LPR_B1200	},	/* A and B */
  {    1800,	DHU_LPR_B1800	},	/* 	 B */
  {    2000,	DHU_LPR_B2000	},	/* 	 B */
  {    2400,	DHU_LPR_B2400	},	/* A and B */
  {    4800,	DHU_LPR_B4800	},	/* A and B */
  {    7200,	DHU_LPR_B7200	},	/* A	   */
  {    9600,	DHU_LPR_B9600	},	/* A and B */
  {   19200,	DHU_LPR_B19200	},	/* 	 B */
  {   38400,	DHU_LPR_B38400	},	/* A	   */
  {      -1,	-1		}
};

static int	dhu_match __P((struct device *, void *, void *));
static void	dhu_attach __P((struct device *, struct device *, void *));

struct	cfdriver dhucd =
{
	NULL, "dhu", dhu_match, dhu_attach, DV_DULL, sizeof(struct device),
};

static void	dhuintr __P((int));
static void	 dhurint __P((int));
static void	 dhuxint __P((int));

static void	dhustart __P((struct tty *));
static int	dhuparam __P((struct tty *, struct termios *));

static int	dhumctl __P((int, int, int));

/* Autoconfig handles: setup the controller to interrupt, */
/* then complete the housecleaning for full operation */

static int
dhu_match (parent, match, aux)
        struct device *parent;
        void *match, *aux;
{
	struct device *dev = match;
	struct uba_attach_args *ua = aux;
	static int nunits = 0;
	register dhuregs *dhuaddr;
	register int n;

	if ( nunits > NDHU )
		return 0;

	dhuaddr = (dhuregs *) ua->ua_addr;

	/* Reset controller to initialize, enable tx/rx interrupts */
	/* to catch floating vector info elsewhere when completed */

	dhuaddr->dhu_csr = (DHU_CSR_MASTER_RESET | DHU_CSR_RXIE | DHU_CSR_TXIE);

	/* Now wait up to 3 seconds for self-test to complete. */

	for (n = 0; n < 300; n++) {
		DELAY(10000);
		if ( ( dhuaddr->dhu_csr & DHU_CSR_MASTER_RESET ) == 0 )
			break;
	}

	/* If the RESET did not clear after 3 seconds, */
	/* the controller must be broken. */

	if ( n >= 300 )
		return 0;

	/* Check whether diagnostic run has signalled a failure. */

	if ( ( dhuaddr->dhu_csr & DHU_CSR_DIAG_FAIL ) != 0 )
		return 0;

	/* Register the rx interrupt handler and pass unit # as arg */

	ua->ua_ivec = dhuintr;
	ua->ua_iarg = nunits;

	nunits++;
       	return 1;
}

static void
dhu_attach (parent, self, aux)
        struct device *parent, *self;
        void *aux;
{
	register struct uba_attach_args *ua = aux;
	register struct	uba_softc *sc = (struct uba_softc *)parent;
	register dhuregs *dhuaddr;
	register unsigned c;
	register int n;

	dhuaddr = (dhuregs *) ua->ua_addr;

	/* Process the 8 bytes of diagnostic info put into */
	/* the FIFO following the master reset operation. */

	printf ("\ndhv:");
	for (n = 0; n < 8; n++) {
		c = dhuaddr->dhu_rbuf;

		if ( (c&DHU_DIAG_CODE) == DHU_DIAG_CODE ) {
			if ( (c&0200) == 0000 )
				printf (" rom(%d) version %d",
					((c>>1)&01), ((c>>2)&037));
			else if ( ((c>>2)&07) != 0 )
				printf (" diag-error(proc%d)=%x",
					((c>>1)&01), ((c>>2)&07));
		}
	}
	printf ("\n");

	/* Initialize our static softc structure. */

	for (n = 0; n < 8; n++) {
		dhu_softc[(self->dv_unit<<4)+n].sc_addr = dhuaddr;
		dhu_softc[(self->dv_unit<<4)+n].sc_tty = ttymalloc();
	}

	/* Now stuff tx interrupt handler in place */

	n = ua->ua_cvec + 1;
	sc->uh_idsp[n].hoppaddr = dhuintr;
	sc->uh_idsp[n].pushlarg = self->dv_unit;

	return;
}

/* Using a single interrupt entry point currently, to keep */
/* CSR reads to a minimum (and avoid losing TX.ACTION signal) */

static void
dhuintr (cntlr)
	int cntlr;
{
	register dhuregs *dhuaddr;
	register unsigned csr;

	dhuaddr = dhu_softc[cntlr].sc_addr;

	/*  The DHV manual says TX.ACTION is cleared when */
	/* the CSR is read, so we've got to be careful. */

	while ((csr = dhuaddr->dhu_csr) & (DHU_CSR_TX_ACTION|
					DHU_CSR_RX_DATA_AVAIL))
	{
		if (csr & DHU_CSR_RX_DATA_AVAIL)
			dhurint (cntlr);

		if (csr & DHU_CSR_TX_ACTION)
			dhuxint ((cntlr<<4)|((csr>>8)&017));
	}
	/* XXX check for spurious interrupts? */
	return;
}

/* Receiver Interrupt */

static void
dhurint (cntlr)
	int cntlr;
{
	register dhuregs *dhuaddr;
	register struct tty *tp;
	register int cc, unit;
	register unsigned c;
	int overrun = 0;

	dhuaddr = dhu_softc[cntlr].sc_addr;

	while ((c = dhuaddr->dhu_rbuf) & DHU_RBUF_DATA_VALID)
	{
		/* Ignore diagnostic FIFO entries. */

		if ((c&DHU_DIAG_CODE) == DHU_DIAG_CODE)
			continue;

		cc = c & 0xff;
		unit = (cntlr<<4) | ((c>>8)&017);
		tp = dhu_softc[unit].sc_tty;

		/* LINK.TYPE is set so we get modem control FIFO entries */

		if ((c & DHU_DIAG_CODE) == DHU_MODEM_CODE) {
			c = ( c << 8 );
			/* Do MDMBUF flow control, wakeup sleeping opens */
			if (c & DHU_STAT_DCD) {
				if (!(tp->t_state & TS_CARR_ON))
				    (void)(*linesw[tp->t_line].l_modem)(tp, 1);
			}
			else if ((tp->t_state & TS_CARR_ON) &&
				(*linesw[tp->t_line].l_modem)(tp, 0) == 0)
					(void) dhumctl (unit, 0, DMSET);
			return;
		}

		if ((c & DHU_RBUF_OVERRUN_ERR) && overrun == 0) {
			log(LOG_WARNING, "dhv%d,%d: silo overflow\n",
				cntlr, (c >> 8) & 017);
			overrun = 1;
		}
		/* A BREAK key will appear as a NULL with a framing error */
		if (c & DHU_RBUF_FRAMING_ERR)
			cc |= TTY_FE;
		if (c & DHU_RBUF_PARITY_ERR)
			cc |= TTY_PE;

		(*linesw[tp->t_line].l_rint)(cc, tp);
	}
	return;
}

/* Transmitter Interrupt */

static void
dhuxint (unit)
	int unit;
{
	register dhuregs *dhuaddr;
	register struct dhu_softc *sc;
	register struct tty *tp;

	sc = &dhu_softc[unit];

	/* Using simple character-at-a-time for now. */
	/* XXX should be checking status of a DMA transfer */

	if (sc->sc_mem < sc->sc_end) {
		dhuaddr = sc->sc_addr;
		dhuaddr->dhu_csr = (DHU_CSR_RXIE | DHU_CSR_TXIE | (unit & 017));
		dhuaddr->dhu_txchar = DHU_TXCHAR_DATA_VALID | *sc->sc_mem++; 
		return;
	}

	tp = sc->sc_tty;
	tp->t_state &= ~TS_BUSY;
	if (tp->t_state & TS_FLUSH)
		tp->t_state &= ~TS_FLUSH;
	else {
		ndflush(&tp->t_outq, sc->sc_mem - (caddr_t) tp->t_outq.c_cf);
		sc->sc_end = sc->sc_mem = tp->t_outq.c_cf;
	}

	if (tp->t_line)
		(*linesw[tp->t_line].l_start)(tp);
	else
		dhustart (tp);

	return;
}

int
dhuopen (dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	register struct tty *tp;
	register int unit;
	int s, error = 0;

	unit = minor(dev);
	if (unit >= NDHULINE)
		return (ENXIO);
	tp = dhu_softc[unit].sc_tty;
	if (tp == NULL)
		tp = dhu_softc[unit].sc_tty = ttymalloc();
	tp->t_oproc = dhustart;
	tp->t_param = dhuparam;
	tp->t_dev = dev;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_state |= TS_WOPEN;
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		(void) dhuparam(tp, &tp->t_termios);
		ttsetwater(tp);
	} else if ((tp->t_state & TS_XCLUDE) && curproc->p_ucred->cr_uid != 0)
		return (EBUSY);
	/* Use DMBIS and *not* DMSET or else we clobber incoming bits */
	if (dhumctl (unit, DML_DTR|DML_RTS, DMBIS) & DML_DCD)
		tp->t_state |= TS_CARR_ON;
	s = spltty();
	while (!(flag & O_NONBLOCK) && !(tp->t_cflag & CLOCAL) &&
	       !(tp->t_state & TS_CARR_ON)) {
		tp->t_state |= TS_WOPEN;
		error = ttysleep(tp, (caddr_t)&tp->t_rawq,
				TTIPRI | PCATCH, ttopen, 0);
		if (error)
			break;
	}
	(void) splx(s);
	if (error)
		return (error);
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

/*ARGSUSED*/
int
dhuclose (dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	register struct tty *tp;
	register int unit = minor(dev);

	tp = dhu_softc[unit].sc_tty;

	(*linesw[tp->t_line].l_close)(tp, flag);

	/* Make sure a BREAK state is not left enabled. */

	(void) dhumctl (unit, DML_BRK, DMBIC);

	/* Do a hangup if so required. */

	if ((tp->t_cflag & HUPCL) || (tp->t_state & TS_WOPEN) ||
	    !(tp->t_state & TS_ISOPEN))
		(void) dhumctl (unit, 0, DMSET);

	return (ttyclose(tp));
}

int
dhuread (dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;

	tp = dhu_softc[minor(dev)].sc_tty;
	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

int
dhuwrite (dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;

	tp = dhu_softc[minor(dev)].sc_tty;
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

/*ARGSUSED*/
int
dhuioctl (dev, cmd, data, flag, p)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	register struct tty *tp;
	register int unit = minor(dev);
	int error;

	tp = dhu_softc[unit].sc_tty;
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);

	switch (cmd) {

	case TIOCSBRK:
		(void) dhumctl (unit, DML_BRK, DMBIS);
		break;

	case TIOCCBRK:
		(void) dhumctl (unit, DML_BRK, DMBIC);
		break;

	case TIOCSDTR:
		(void) dhumctl (unit, DML_DTR|DML_RTS, DMBIS);
		break;

	case TIOCCDTR:
		(void) dhumctl (unit, DML_DTR|DML_RTS, DMBIC);
		break;

	case TIOCMSET:
		(void) dhumctl (unit, *(int *)data, DMSET);
		break;

	case TIOCMBIS:
		(void) dhumctl (unit, *(int *)data, DMBIS);
		break;

	case TIOCMBIC:
		(void) dhumctl (unit, *(int *)data, DMBIC);
		break;

	case TIOCMGET:
		*(int *)data = dhumctl (unit, 0, DMGET);
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}

/*ARGSUSED*/
int
dhustop (tp, flag)
	register struct tty *tp;
{
#ifdef notyet
	register dhuregs *dhuaddr;
#endif
	register struct dhu_softc *sc;
	int unit = minor(tp->t_dev);
	int s;

	s = spltty();
	if (tp->t_state & TS_BUSY) {

		sc = &dhu_softc[unit];
#ifdef notyet
		dhuaddr = sc->sc_addr;
		dhuaddr->dhu_csr = (DHU_CSR_RXIE | DHU_CSR_TXIE | (unit & 017));
		dhuaddr->dhu_lnctrl |= DHU_LNCTRL_DMA_ABORT;
		/* Aborting a DMA operation leaves it restartable. */
#else
		sc->sc_end = sc->sc_mem;
#endif

		if (!(tp->t_state & TS_TTSTOP))
			tp->t_state |= TS_FLUSH;
	}
	(void) splx(s);
	return 0;
}

struct tty *
dhutty (dev)
        dev_t dev;
{
        struct tty *tp = dhu_softc[minor(dev)].sc_tty;
        return (tp);
}

static void
dhustart (tp)
	register struct tty *tp;
{
	register struct dhu_softc *sc;
	register dhuregs *dhuaddr;
	register int unit = minor(tp->t_dev);
	register int cc;
	int s;

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out;
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
	}
	if (tp->t_outq.c_cc == 0)
		goto out;
	cc = ndqb(&tp->t_outq, 0);
	if (cc == 0) 
		goto out;

	tp->t_state |= TS_BUSY;

	sc = &dhu_softc[unit];
	sc->sc_end = sc->sc_mem = tp->t_outq.c_cf;
	sc->sc_end += cc;

#ifdef notyet
	dhuaddr = sc->sc_addr;
	dhuaddr->dhu_csr = ( DHU_CSR_RXIE | DHU_CSR_TXIE | (unit & 017) );
	/* XXX Still have to actually setup DMA action! */
#else
	dhuaddr = sc->sc_addr;
	dhuaddr->dhu_csr = (DHU_CSR_RXIE | DHU_CSR_TXIE | (unit & 017));
	dhuaddr->dhu_txchar = DHU_TXCHAR_DATA_VALID | *sc->sc_mem++; 
#endif

out:
	(void) splx(s);
	return;
}

static int
dhuparam (tp, t)
	register struct tty *tp;
	register struct termios *t;
{
	register dhuregs *dhuaddr;
	register int cflag = t->c_cflag;
	int unit = minor(tp->t_dev);
	int ispeed = ttspeedtab(t->c_ispeed, dhuspeedtab);
	int ospeed = ttspeedtab(t->c_ospeed, dhuspeedtab);
	register unsigned lpr;
	int s;

	/* check requested parameters */
        if (ospeed < 0 || ispeed < 0)
                return (EINVAL);

        tp->t_ispeed = t->c_ispeed;
        tp->t_ospeed = t->c_ospeed;
        tp->t_cflag = cflag;

	if (ospeed == 0) {
		(void) dhumctl (unit, 0, DMSET);	/* hang up line */
		return (0);
	}

	s = spltty();
	dhuaddr = dhu_softc[unit].sc_addr;
	dhuaddr->dhu_csr = ( DHU_CSR_RXIE | DHU_CSR_TXIE | (unit & 017) );

	lpr = ((ispeed&017)<<8) | ((ospeed&017)<<12) ;

	switch (cflag & CSIZE)
	{
	  case CS5:
		lpr |= DHU_LPR_5_BIT_CHAR;
		break;
	  case CS6:
		lpr |= DHU_LPR_6_BIT_CHAR;
		break;
	  case CS7:
		lpr |= DHU_LPR_7_BIT_CHAR;
		break;
	  default:
		lpr |= DHU_LPR_8_BIT_CHAR;
		break;
	}
	if (cflag & PARENB)
		lpr |= DHU_LPR_PARENB;
	if (!(cflag & PARODD))
		lpr |= DHU_LPR_EPAR;
	if (cflag & CSTOPB)
		lpr |= DHU_LPR_2_STOP;

	dhuaddr->dhu_lpr = lpr;

	/* Setting LINK.TYPE enables modem signal change interrupts. */

	dhuaddr->dhu_lnctrl  |= (DHU_LNCTRL_RX_ENABLE | DHU_LNCTRL_LINK_TYPE);
	dhuaddr->dhu_tbufad2 |= DHU_TBUFAD2_TX_ENABLE;

	(void) splx(s);
	return (0);
}

static int
dhumctl (unit, bits, how)
	int unit, bits, how;
{
	register dhuregs *dhuaddr;
	register unsigned mdmstat;
	register int mbits;
	int s;

	s = spltty();

	dhuaddr = dhu_softc[unit].sc_addr;
	dhuaddr->dhu_csr = ( DHU_CSR_RXIE | DHU_CSR_TXIE | (unit & 017) );

	mbits = 0;

	/* external signals as seen from the port */

	mdmstat = dhuaddr->dhu_stat;

	if (mdmstat & DHU_STAT_CTS)
		mbits |= DML_CTS;

	if (mdmstat & DHU_STAT_DCD)
		mbits |= DML_DCD;

	if (mdmstat & DHU_STAT_DSR)
		mbits |= DML_DSR;

	if (mdmstat & DHU_STAT_RI)
		mbits |= DML_RI;

	/* internal signals/state delivered to port */

	mdmstat = dhuaddr->dhu_lnctrl;

	if (mdmstat & DHU_LNCTRL_RTS)
		mbits |= DML_RTS;

	if (mdmstat & DHU_LNCTRL_DTR)
		mbits |= DML_DTR;

	if (mdmstat & DHU_LNCTRL_BREAK)
		mbits |= DML_BRK;

	switch (how)
	{
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
		(void) splx(s);
		return (mbits);
	}

	if (mbits & DML_RTS)
		dhuaddr->dhu_lnctrl |= DHU_LNCTRL_RTS;
	else
		dhuaddr->dhu_lnctrl &= ~DHU_LNCTRL_RTS;

	if (mbits & DML_DTR)
		dhuaddr->dhu_lnctrl |= DHU_LNCTRL_DTR;
	else
		dhuaddr->dhu_lnctrl &= ~DHU_LNCTRL_DTR;

	if (mbits & DML_BRK)
		dhuaddr->dhu_lnctrl |= DHU_LNCTRL_BREAK;
	else
		dhuaddr->dhu_lnctrl &= ~DHU_LNCTRL_BREAK;

	(void) splx(s);
	return (mbits);
}

#endif	/* #if NTS > 0 */

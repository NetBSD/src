/*	$NetBSD: ser.c,v 1.20 1995/04/12 14:55:45 briggs Exp $	*/

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
 */

/*
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
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
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	Hacked by Brad Parker, <brad@fcr.com>
 *		added CTS input flow control
 *		added DCD event detection
 *		added software fifo's
 *
 *	Mac II serial device interface
 *
 * 	Information used in this source was gleaned from low-memory
 *	 global variables in MacOS and the Advanced Micro Devices
 *	 1992 Data Book/Handbook.
 */

#include "ser.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/frame.h>

#include <dev/cons.h>

#include <dev/ic/z8530.h>
#include <mac68k/dev/serreg.h>

/*#define DEBUG*/
#undef DEBUG

volatile unsigned char *sccA = (unsigned char *) 0x4000;

static void	serstart __P((register struct tty *));
static int	serparam __P((register struct tty *, register struct termios *));
static int	serctl   __P((dev_t dev, int bits, int how));
extern int	ser_intr  __P((void));

static int	ser_active = 0;
static int	nser = NSER;
static int	serdefaultrate = TTYDEF_SPEED;

struct	tty *ser_tty[NSER];

extern	struct tty *constty;

#define	UNIT(x)		minor(x)

struct ser_status {
	unsigned char ddcd, dcts;	/* delta (change) flags */
	unsigned char dcd, cts;		/* last state of signal */
	unsigned char dtr, rts;		/* current state of signal */
	int	oflo;			/* s/w fifo over flow */
	int	over;			/* h/w fifo over flow */
	int	flags;
#define	SER_BUSY	0x01
} ser_status[NSER];

#define	SCC_INT		10
#define	SCC_SPEED	11


/* SCC initialization string from Steve Allen (wormey@eskimo.com) */
static unsigned char ser_init_bytes[]={
	 4, 0x44,	/* Transmit/Receive control.  Select Async or Sync
			   mode and clock multiplier. */
	 3, 0xc0,	/* select receiver control.  Bit d0 (rx enable)
			   must be set to 0 at this time. */
	 5, 0xe2,	/* select transmit control.  Bit d3 (tx enable)
			   must be set to 0 at this time. */
	 9, 0x06,	/* select interrupt control.  Bit d3 (mie)
			   must be set to 0 at this time. */
	10, 0x00,	/* miscellaneous control. */
	11, 0x50,	/* clock control. */
	12, 0x04,	/* time constant LB. */
	13, 0x00,	/* time constant HB. */
	14, 0x00,	/* miscellaneous control.  Bit d0 (BR gen enable)
			   must be set to 0 at this time. */
	 3, 0xc1,	/* set d0 (rx enable). */
	 5, 0xea,	/* set d3 (tx enable). */
	 0, 0x80,	/* reset txCRC. */
	14, 0x01,	/* BR gen enable.  Enable DPLL. */
	 1, 0x00,	/* make sure DMA not set. */
	15, 0x00,	/* disable external interrupts. */
	 0, 0x10,	/* reset ext/status twice. */
	 0, 0x10,
	 1, 0x0a,	/* enable rcv and xmit interrupts. */
	 9, 0x0e,	/* enable master interrupt bit d3. */
};

extern int matchbyname();

static void
serinit(int running_interrupts)
{
static	int initted=0;
	int bcount;
	int i, s, spd;

	/*
	 * Will be called twice if we're running a serial console.
	 */
	if (initted++)
		return;

	sccA = IOBase + sccA;

	spd = SERBRD(serdefaultrate);

	s = splhigh();

	SER_DOCNTL(0, 9, 0xc0);

	/*
	 * initialize ports, substituting proper speed.
	 */
	bcount = sizeof(ser_init_bytes);
	for(i = 0; i < bcount; i += 2){
		if (ser_init_bytes[i] == 12)	/* baud rate low byte */
			ser_init_bytes[i+1] = (spd & 0xff);
		if (ser_init_bytes[i] == 13)	/* baud rate high byte */
			ser_init_bytes[i+1] = ((spd>>8) & 0xff);
		if (!running_interrupts) {
			if (   ser_init_bytes[i]   == 0x01
			    && ser_init_bytes[i+1] == 0x0a)
				break;
		}
		SER_DOCNTL(0, ser_init_bytes[i], ser_init_bytes[i + 1]);
		SER_DOCNTL(1, ser_init_bytes[i], ser_init_bytes[i + 1]);
	}

	splx(s);
}

static void
serattach(parent, dev, aux)
	struct device	*parent, *dev;
	void		*aux;
{
	if (mac68k_machine.serial_boot_echo) {
		printf(" (serial boot echo is on)\n");
	}
	printf("\n");

	serinit(1);
}

struct cfdriver sercd =
      {	NULL, "ser", matchbyname, serattach,
	DV_TTY, sizeof(struct device), NULL, 0 };

/* ARGSUSED */
extern int
seropen(dev_t dev, int flag, int mode, struct proc *p)
{
	register struct tty *tp;
	register int unit;
	int error = 0;
 
#if defined(DEBUG)
	printf("ser: entered seropen(%d, %d, %d, xx)\n", dev, flag, mode);
#endif
	unit = UNIT(dev);
	if (unit >= NSER ){
		return (ENXIO);
	}
	ser_active |= 1 << unit;
	if (ser_tty[unit]) {
		tp = ser_tty[unit];
	} else {
		tp = ser_tty[unit] = ttymalloc();
	}
	tp->t_oproc = serstart;
	tp->t_param = serparam;
	tp->t_dev = dev;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_state |= TS_WOPEN;
		ttychars(tp);
		if (tp->t_ispeed == 0) {
			tp->t_iflag = TTYDEF_IFLAG;
			tp->t_oflag = TTYDEF_OFLAG;
			tp->t_cflag = TTYDEF_CFLAG;
			tp->t_lflag = TTYDEF_LFLAG;
			tp->t_ispeed = tp->t_ospeed = serdefaultrate;
		}
		serparam(tp, &tp->t_termios);
		ttsetwater(tp);
	} else if (tp->t_state&TS_XCLUDE && p->p_ucred->cr_uid != 0){
		printf("ser%d: device is busy.\n", unit);
		return (EBUSY);
	}

	/* serial device open code */

	bzero((char *)&ser_status[unit], sizeof(struct ser_status));

	/* turn on RTS & DTR */
	serctl(unit, ZSWR5_RTS | ZSWR5_DTR, DMSET);

	if(serctl(unit, 0, DMGET) & ZSRR0_DCD)
		tp->t_state |= TS_CARR_ON;

	/* enable interrupts */
	serctl(unit, 1, SCC_INT);

	/* end serial device open code */

	(void) spltty();
	while ((flag&O_NONBLOCK) == 0 && (tp->t_cflag&CLOCAL) == 0 &&
	       (tp->t_state & TS_CARR_ON) == 0) {
		tp->t_state |= TS_WOPEN;
		if (error = ttysleep(tp, (caddr_t)&tp->t_rawq, TTIPRI | PCATCH,
		    ttopen, 0))
			break;
	}
	(void) spl0();
	if (error == 0)
		error = (*linesw[tp->t_line].l_open)(dev, tp);

#if defined(DEBUG)
	printf("ser: exiting seropen()\n");
#endif

	return (error);
}
 
/*ARGSUSED*/
extern int
serclose(dev_t dev, int flag, int mode, struct proc *p)
{
	register struct tty *tp;
	register int unit;
	int s;
 
#if defined(DEBUG)
	printf("ser: entered serclose()\n");
#endif
	unit = UNIT(dev);
	tp = ser_tty[unit];
	(*linesw[tp->t_line].l_close)(tp, flag);

	/* serial device close code */

	/* disable interrupts */
	serctl(unit, 0, SCC_INT);

	if (tp->t_cflag&HUPCL || tp->t_state&TS_WOPEN ||
	    (tp->t_state&TS_ISOPEN) == 0)
		serctl(unit, 0, DMSET);	/* turn RTS and DTR off */

	ser_active &= ~(1 << unit);
	/* end of serial device close code */

	ttyclose(tp);
#ifdef broken
	ttyfree(tp);
	ser_tty[unit] = NULL;
#endif

#if defined(DEBUG)
	printf("ser: exiting serclose()\n");
#endif
	return (0);
}
 
extern int
serread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct tty *tp = ser_tty[UNIT(dev)];
#if defined(DEBUG)
	printf("ser: called serread()\n");
#endif
 
	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}
 
extern int
serwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	int unit = UNIT(dev);
	register struct tty *tp = ser_tty[unit];
 
#if defined(DEBUG)
	printf("ser: called serwrite()\n");
#endif
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

/* private buffers used by driver at splscc() */
#define INBUFLEN	128
#define OUTBUFLEN	512
static unsigned char ser_inbuf[NSER][INBUFLEN];
static volatile unsigned char ser_inlen[NSER] = {0,0};
static unsigned char ser_intail[NSER] = {0,0};

static unsigned char ser_outbuf[NSER][OUTBUFLEN];
static volatile unsigned int ser_outlen[NSER] = {0,0};
static volatile unsigned int ser_outtail[NSER] = {0,0};

/* NOTE: This function is called by locore.s on a level 4 interrupt. 
   since "splscc()" is level 4, and this is currently higher than
   anything except splhigh(), you can't call anything from this
   routine or you'll break the syncronization.  basically we just
   do i/o from our local buffers and signal the upper layer with
   a software interrupt.
*/

extern int
ser_intr(void)
{
    /* serial interrupt code */
    unsigned char reg0, reg1, ch, ch1, c, bits;
    int s;
    register int unit;

    /* read status to reset SCC state machine */
    reg0 = SCCCNTL(0);

    /* reset port B vector to see who interrupted us */
    bits = SER_STATUS(1, 2) & 0x0e;
    if (bits < 8)
	unit = 1;
    else
	unit = 0;

    reg0 = SER_STATUS(unit, 0);
    switch ((bits & 7) >> 1) {
      case 0:	/* tranmitter buffer empty */
	if (ser_outlen[unit] > 0)
	{
	    c = ser_outbuf[unit][ser_outtail[unit]];
	    ser_outtail[unit] = (ser_outtail[unit] + 1) % OUTBUFLEN;
	    SCCRDWR(unit) = c;
	    ser_outlen[unit]--;
	} else {
	    SER_DOCNTL(unit, 0, ZSWR0_RESET_TXINT);
	    ser_status[unit].flags &= ~SER_BUSY;
	    setsoftserial();
	}
	SER_DOCNTL(unit, 0, ZSWR0_CLR_INTR);
	break;
      case 1:	/* ext/status change */
	if ((reg0 & ZSRR0_DCD) && ser_status[unit].dcd == 0)
		ser_status[unit].ddcd = 1;
	else
		if (!(reg0 & ZSRR0_DCD) && ser_status[unit].dcd != 0)
			ser_status[unit].ddcd = 1;
	ser_status[unit].dcd = reg0 & ZSRR0_DCD;

	if ((reg0 & ZSRR0_CTS) && ser_status[unit].cts == 0)
		ser_status[unit].dcts = 1;
	else
		if (!(reg0 & ZSRR0_CTS) && ser_status[unit].cts != 0)
			ser_status[unit].dcts = 1;
	ser_status[unit].cts = reg0 & ZSRR0_CTS;

	if (reg0 & ZSRR0_TXUNDER)
	    SER_DOCNTL(unit, 0, ZSWR0_RESET_EOM);

	SER_DOCNTL(unit, 0, ZSWR0_RESET_STATUS);
	SER_DOCNTL(unit, 0, ZSWR0_CLR_INTR);
	break;
      case 2:	/* recv char available */
	ch = SCCRDWR(unit);
	c = 1;
	if (SER_STATUS(unit, 0) & ZSRR0_RX_READY) {
		ch1 = SCCRDWR(unit);
		c = 2;
	}

	if (ser_inlen[unit] < INBUFLEN) 
	ser_inbuf[unit][(ser_intail[unit] + (ser_inlen[unit]++)) % INBUFLEN] = ch;
	else ser_status[unit].oflo++;
	if (c > 1) {
		if (ser_inlen[unit] < INBUFLEN) 
			ser_inbuf[unit][(ser_intail[unit] + (ser_inlen[unit]++)) % INBUFLEN] = ch1;
		else ser_status[unit].oflo++;
	}
	setsoftserial();

	SER_DOCNTL(unit, 0, ZSWR0_CLR_INTR);
	break;
      case 3:	/* spec recv condition */
	reg1 = SER_STATUS(unit, 1);
	SCCRDWR(unit); /* flush fifo */
	if (reg1 & ZSRR1_DO)
		ser_status[unit].over++;
	SER_DOCNTL(unit, 0, ZSWR0_RESET_ERRORS);
	SER_DOCNTL(unit, 0, ZSWR0_CLR_INTR);
	break;
    }

    return(1);
    /* end of serial interrupt code */
}

/* serial software interrupt. do all the things we could
   not do at splscc();
*/

extern void
sersir(void)
{
	int unit, s, c;
	register struct tty *tp;

	for (unit = 0; unit < 2; unit++) {
		if ((tp = ser_tty[unit]) == 0)
			continue;

		/* check for overflows */
		if (ser_status[unit].oflo || ser_status[unit].over) {
			s = splhigh();
			ser_status[unit].oflo = 0;
			ser_status[unit].over = 0;
			splx(s);
			if (tp->t_state & TS_ISOPEN)
				(*linesw[tp->t_line].l_rint)('#', tp);
		}
		/* check for change in DCD */
		if (ser_status[unit].ddcd) {
			s = splhigh();
			ser_status[unit].ddcd = 0;
			splx(s);
			if (0) {
				if (ser_status[unit].dcd)
					tp->t_state |= TS_CARR_ON;
				else
					tp->t_state &= ~TS_CARR_ON;

				(*linesw[tp->t_line].l_modem)(tp,
					ser_status[unit].dcd ? 1 : 0);
			}
		}
		/* check for change in CTS */
		if (ser_status[unit].dcts) {
			s = splhigh();
			ser_status[unit].dcts = 0;
			splx(s);
			if ((tp->t_state & TS_ISOPEN) &&
			    (tp->t_flags & CRTSCTS)) {
				tp->t_state &= ~TS_TTSTOP;
				serstart(tp);
			} else
				tp->t_state |= TS_TTSTOP;
		}
		/* drain input fifo */
		while (ser_inlen[unit] > 0) {
			if (tp->t_rawq.c_cc + tp->t_canq.c_cc >= TTYHOG) {
				setsoftserial();
				break;
			}
			s = splhigh();
			c = ser_inbuf[unit][ser_intail[unit]];
			ser_intail[unit] = (ser_intail[unit] + 1) % INBUFLEN;
			ser_inlen[unit]--;
			splx(s);
			if (tp->t_state & TS_ISOPEN)
				(*linesw[tp->t_line].l_rint)(c, tp);
		}
		/* fill output fifo */
		if (ser_outlen[unit] == 0) {
			if (tp->t_line)
				(*linesw[tp->t_line].l_start)(tp);
			else
				serstart(tp);	
		}
	}
}

extern int
serioctl(dev_t dev, int cmd, caddr_t data, int flag, struct proc *p)
{
	register struct tty *tp;
	register int unit = UNIT(dev);
	register int error;
 
#if defined(DEBUG)
	printf("ser: entering ioctl()\n");
#endif
	tp = ser_tty[unit];
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);

	switch (cmd) {
#if 0
	case TIOCSBRK:	/* turn break on */
		dca->dca_cfcr |= CFCR_SBREAK;
		break;

	case TIOCCBRK:	/* turn break off */
		dca->dca_cfcr &= ~CFCR_SBREAK;
		break;
#endif
	case TIOCSDTR:	/* set DTR */
		(void) serctl(dev, ZSWR5_DTR | ZSWR5_RTS, DMBIS);
		break;

	case TIOCCDTR:	/* clear DTR */
		(void) serctl(dev, ZSWR5_DTR | ZSWR5_RTS, DMBIC);
		break;

	case TIOCMSET:	/* set modem control bits */
		(void) serctl(dev, *(int *)data, DMSET);
		break;

	case TIOCMBIS:	/* OR bits on */
		(void) serctl(dev, *(int *)data, DMBIS);
		break;

	case TIOCMBIC:  /* AND bits off */
		(void) serctl(dev, *(int *)data, DMBIC);
		break;

	case TIOCMGET:	/* get modem bits */
		*(int *)data = serctl(dev, 0, DMGET);
		break;

	case TIOCGFLAGS:
		{
			int	bits = 0;

			*(int *)data = bits;
			break;
		}

	case TIOCSFLAGS:
		{
			int	userbits, driverbits = 0;

			error = suser(p->p_ucred, &p->p_acflag);
			if (error != 0)
				return (EPERM);
			userbits = *(int *)data;
			break;
		}

	default:
#if defined(DEBUG)
		printf("ser%d: unknown ioctl(,0x%x,)\n", UNIT(dev), cmd);
#endif
		return (ENOTTY);
	}
#if defined(DEBUG)
	printf("ser: exiting ioctl()\n");
#endif
	return (0);
}

static int
ser_calc_regs(int unit, int cflag, unsigned char *preg3, unsigned char *preg4,
	      unsigned char *preg5)
{
	unsigned char r3, r4, r5;

	r3 = ZSWR3_RX_ENABLE;
	r5 = ZSWR5_TX_ENABLE;
	if (ser_status[unit].dtr)
		r5 |= ZSWR5_DTR;
	if (ser_status[unit].rts)
		r5 |= ZSWR5_RTS;
	switch (cflag&CSIZE) {
	    case CS5:
		r3 |= ZSWR3_RX_5;
		r5 |= ZSWR5_TX_5;
		break;
	    case CS6:
		r3 |= ZSWR3_RX_6;
		r5 |= ZSWR5_TX_6;
		break;
	    case CS7:
		r3 |= ZSWR3_RX_7;
		r5 |= ZSWR5_TX_7;
		break;
	    case CS8:
		r3 |= ZSWR3_RX_8;
		r5 |= ZSWR5_TX_8;
		break;
	}
	r4 = 0;
	if ((cflag & PARODD) == 0)
		r4 |= ZSWR4_EVENP;
	if (cflag & PARENB)
		r4 |= ZSWR4_PARENB;
	if (cflag & CSTOPB)
		r4 |= ZSWR4_TWOSB;
	else
		r4 |= ZSWR4_ONESB;
	
	*preg3 = r3;
	*preg4 = r4;
	*preg5 = r5;
}
 
static int
serparam(register struct tty *tp, register struct termios *t)
{
	register int cflag = t->c_cflag;
	unsigned char reg3, reg4, reg5;
	int unit = UNIT(tp->t_dev);
	int ospeed = t->c_ospeed;
	int s;

#if defined(DEBUG)
	printf("ser: entering serparam()\n");
#endif
 
	/* check requested parameters */
        if (ospeed < 0 || (t->c_ispeed && t->c_ispeed != t->c_ospeed)){
		printf("ser: serparam() returning EINVAL\n");
                return (EINVAL);
	}
        /* and copy to tty */
        tp->t_ispeed = t->c_ispeed;
        tp->t_ospeed = t->c_ospeed;
        tp->t_cflag = cflag;

	/* Start of serial specific param code */
	if(ospeed == 0)	{
		serctl(unit, 0, DMSET);	/* hang up line */
		return(0);
	}

	serctl(unit, ospeed, SCC_SPEED);
/*
	ser_calc_regs(unit, cflag, &reg3, &reg4, &reg5);

	s = splhigh();
	SER_DOCNTL(unit, 3, reg3);
	SER_DOCNTL(unit, 4, reg4);
	SER_DOCNTL(unit, 5, reg5);
	splx(s);
*/
	serctl(unit, 1, SCC_INT);
	serctl(unit, ZSWR5_DTR | ZSWR5_RTS, DMSET);

	/* End of serial specific param code */

#if defined(DEBUG)
	printf("ser: exiting serparam()\n");
#endif
	return (0);
}

extern void
serstart(register struct tty *tp)
{
	int s, s1;
	int i, space, unit, c, need_start, first_char;
 
	unit = UNIT(tp->t_dev);
	s = spltty();
	if (tp->t_state & (TS_TIMEOUT|TS_TTSTOP)) {
		goto out;
	}
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state&TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		selwakeup(&(tp->t_wsel));
	}
	if (tp->t_outq.c_cc == 0 || (tp->t_state & TS_BUSY) ||
	    (ser_status[unit].flags & SER_BUSY))
		goto out;
	tp->t_state |= TS_BUSY;

	if(ser_outlen[unit] == 0){
		first_char = (char)getc(&tp->t_outq);
		need_start = 1;
	} else
		need_start = 0;

	/* put characters into a buffer that ser_intr() will empty */
	/* out on transmit-ready interrupts. */

	/* get free space in s/w fifo - this will only get better */
	s1 = splhigh();
	space = OUTBUFLEN - ser_outlen[unit];
	splx(s1);

	while(tp->t_outq.c_cc && space > 0) {
		/* note that getc goes spltty() */
		c = getc(&tp->t_outq);
		/* protect s/w fifo at splhigh() */
		s1 = splhigh();
		ser_outbuf[unit][(ser_outtail[unit] + (ser_outlen[unit]++))
		    % OUTBUFLEN] = (char)c;
		splx(s1);
		space--;
	}
	tp->t_state &= ~TS_BUSY;

	if (need_start) {
		s1 = splhigh();
		ser_status[unit].flags |= SER_BUSY;
		SCCRDWR(unit) = first_char;	/* to start chain */
		splx(s1);
	}

out:
	splx(s);
}
 
/*
 * Stop output on a line.
 */
/*ARGSUSED*/
extern int
serstop(register struct tty *tp, int flag)
{
	register int s;

#if defined(DEBUG)
	printf("ser: entering serstop()\n");
#endif
	s = spltty();
	if (tp->t_state & TS_BUSY) {
		if ((tp->t_state&TS_TTSTOP)==0)
			tp->t_state |= TS_FLUSH;
	}
#if defined(DEBUG)
	printf("ser: exiting serstop()\n");
#endif
	splx(s);
}

static int
serctl(dev_t dev, int bits, int how)
{
	int unit, s;

	unit = UNIT(dev);
	/* run at splhigh so we don't get interrupted by i/o */
	s = splhigh();
	switch (how) {

	case DMSET:
		ser_status[unit].dtr = bits & ZSWR5_DTR;
		ser_status[unit].rts = bits & ZSWR5_RTS;
		SER_DOCNTL(unit, 5, bits | 0x68);
		break;

	case DMBIS:
		break;

	case DMBIC:
		break;

	case DMGET:
		bits = SER_STATUS(unit, 0);
		break;

	/* */
	case SCC_INT:
		if (bits) {
			SER_DOCNTL(unit, 0, ZSWR0_RESET_ERRORS);
			SER_DOCNTL(unit, 0, ZSWR0_CLR_INTR);
			SER_DOCNTL(unit, 1, ZSWR1_SIE | ZSWR1_RIE | ZSWR1_TIE);
		} else
			SER_DOCNTL(unit, 1, 0);
		break;
	case SCC_SPEED:
		SER_DOCNTL(unit, 12, SERBRD(bits) & 0xff);
		SER_DOCNTL(unit, 13, (SERBRD(bits) >> 8) & 0xff);
		break;
	}

	(void) splx(s);
	return(bits);
}

/*
 * Console functions.
 */

dev_t	mac68k_serdev;

sercnprobe(struct consdev *cp)
{
    int	maj, unit;

    for (maj = 0 ; maj < nchrdev ; maj++) {
        if (cdevsw[maj].d_open == seropen) {
            break;
        }
    }
    if (maj == nchrdev)
        goto nosercon;

    cp->cn_pri = CN_NORMAL;     /* Lower than CN_INTERNAL */
    if (mac68k_machine.serial_console & 0x01)
        cp->cn_pri = CN_REMOTE; /* Higher than CN_INTERNAL */

    unit = (mac68k_machine.serial_console & 0x02) ? 1 : 0;

    cp->cn_dev = makedev(maj, unit);

    mac68k_machine.serial_boot_echo = 0;
    return 0;

nosercon:
    if (mac68k_machine.serial_boot_echo) {
	/* major number doesn't really matter. */
	mac68k_serdev = makedev(maj, 0);
        serinit(1);
    }

    return 0;
}

sercninit(struct consdev *cp)
{
    serinit(1);
}

sercngetc(dev_t dev)
{
    int unit, c;

    unit = UNIT(dev);

    while (!(SER_STATUS(unit, 0) & ZSRR0_RX_READY));
    c = SCCRDWR(unit);
    SER_STATUS(unit, 0) = ZSWR0_RESET_STATUS;

    return c;
}

sercnputc(dev_t dev, int c)
{
    int	unit;

    unit = UNIT(dev);

    while (!(SER_STATUS(unit, 0) & ZSRR0_TX_READY));
    SCCRDWR(unit) = c;
}

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
 * $Id: ser.c,v 1.2 1993/11/29 00:32:58 briggs Exp $
 *
 *	Mac II serial device interface
 *
 * 	Information used in this source was gleaned from low-memory
 *	 global variables in MacOS and the Advanced Micro Devices
 *	 1992 Data Book/Handbook.
 */

#define NSER 2	/* Could be more later with proprietary serial iface? */

#include "sys/param.h"
#include "sys/systm.h"
#include "sys/ioctl.h"
#include "sys/tty.h"
#include "sys/proc.h"
#include "sys/conf.h"
#include "sys/file.h"
#include "sys/uio.h"
#include "sys/kernel.h"
#include "sys/syslog.h"

#include "sys/device.h"
#include "serreg.h"
#include "machine/cpu.h"

/*#define DEBUG*/
#undef DEBUG


volatile unsigned char *sccA = (unsigned char *) 0x50004000;

int	serstart(), serparam(), serintr();
int	ser_active = 0;
int	nser = NSER;
int	serdefaultrate = TTYDEF_SPEED;
int	sermajor;
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

char serial_id_string[] = "Two MacII serial devices built in.";
char serial_debug_id_string[] = "Two MacII serial devices--one in use for debugging.";
char serial_0_string[] = "MacBSD Serial Driver, Port 0\n\r";
char serial_1_string[] = "MacBSD Serial Driver, Port 1\n\r";


unsigned char ser_0_init_bytes[]={
#if 0	/* BG -- I made this table from scratch according to the docs. */
	9, SER_W9_NV | SER_W9_DLC,	/* No interrupt vector */
					/* Disable lower chain */	
	4, SER_W4_1SBIT,		/* One stop bit, clock times 1 */
	10, SER_W10_NRZ,		/* NRZ mode encoding */
	11, SER_W11_TXBR | SER_W11_RXBR,/* Receive and Transmit using BR clock */
	12, 102,			
	13, 0,				/* Time code for 19200 baud */
	14, SER_W14_ENBBR,		/* Enable BR clock */
	15, SER_W15_ABRTINT,		/* Abort pending interrupts */
	3, SER_W3_RX8DBITS | SER_W3_ENBRX,
					/* Receive 8 bit data, rx int ready */
	5, SER_W5_TX8DBITS | SER_W5_ENBTX,
					/* Receive 8 bit data, rx int ready */
	0, SER_W0_ENBRXRDY,		/* Enable rx int on next char */
	1, SER_W1_ENBRXRDY | SER_W1_ENBTXRDY,
					/* Enable receive and transmit ints */
	9, SER_W9_MIE			/* Enable Master Interrupt */
#else	/* but it doesn't work as well as this hacked job does: */
	2, 0,
	10, 0,
	11, 0x50,
	4, 0x44,
	3, 0xc1,
	5, 0x68,
	14, 0x01,
	15, 0x80,
	12, 4,
	13, 0,
	1, 0x13,
	0, 0x10,
	0, 0x20,
	9, 0x08
#endif
};

unsigned char ser_1_init_bytes[]={
	2, 0,
	10, 0,
	11, 0x50,
	4, 0x44,
	3, 0xc1,
	5, 0x68,
	14, 0x01,
	15, 0x80,
	12, 4,
	13, 0,
	1, 0x13,
	0, 0x10,
	0, 0x20,
	9, 0x08
};

#if 0
int
serinit(register struct macdriver *md)
{
extern	int serial_boot_echo;
	int bcount;
	int i, s;


	md->hwfound = 1;
	md->name = serial_id_string;

	if (serial_boot_echo) {
		md->name = serial_debug_id_string;
	}

	SER_DOCNTL(0, 9, 0xc0);	/* force hardware reset */

	s = splhigh();

	/* initialize port 0 */
	bcount = sizeof(ser_0_init_bytes);
	for(i = 0; i < bcount; i += 2){
		SER_DOCNTL(0, ser_0_init_bytes[i], ser_0_init_bytes[i + 1]);
	}

	/* initialize port 1 */
	bcount = sizeof(ser_1_init_bytes);
	for(i = 0; i < bcount; i += 2){
		SER_DOCNTL(1, ser_1_init_bytes[i], ser_1_init_bytes[i + 1]);
	}

	splx(s);
	return(1);
}
#endif

extern int matchbyname();

static void
serattach(parent, dev, aux)
	struct device	*parent, *dev;
	void		*aux;
{
extern	int serial_boot_echo;
	int bcount;
	int i, s;

	printf("\n");
	if (serial_boot_echo) {
		printf("(serial boot echo is on)\n");
	}

	SER_DOCNTL(0, 9, 0xc0);	/* force hardware reset */

	s = splhigh();

	/* initialize port 0 */
	bcount = sizeof(ser_0_init_bytes);
	for(i = 0; i < bcount; i += 2){
		SER_DOCNTL(0, ser_0_init_bytes[i], ser_0_init_bytes[i + 1]);
	}

	/* initialize port 1 */
	bcount = sizeof(ser_1_init_bytes);
	for(i = 0; i < bcount; i += 2){
		SER_DOCNTL(1, ser_1_init_bytes[i], ser_1_init_bytes[i + 1]);
	}

	splx(s);
}

struct cfdriver sercd =
      {	NULL, "ser", matchbyname, serattach,
	DV_TTY, sizeof(struct device), NULL, 0 };

/* ARGSUSED */
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
	serctl(unit, SER_W5_RTS | SER_W5_DTR, DMSET);

	if(serctl(unit, 0, DMGET) & SER_R0_DCD)
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
 
int
serintr()
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
	    SER_DOCNTL(unit, 0, SER_W0_RSTTXPND);
	    ser_status[unit].flags &= ~SER_BUSY;
	    setsoftserial();
	}
	SER_DOCNTL(unit, 0, SER_W0_RSTIUS);
	break;
      case 1:	/* ext/status change */
	if ((reg0 & SER_R0_DCD) && ser_status[unit].dcd == 0)
		ser_status[unit].ddcd = 1;
	else
		if (!(reg0 & SER_R0_DCD) && ser_status[unit].dcd != 0)
			ser_status[unit].ddcd = 1;
	ser_status[unit].dcd = reg0 & SER_R0_DCD;

	if ((reg0 & SER_R0_CTS) && ser_status[unit].cts == 0)
		ser_status[unit].dcts = 1;
	else
		if (!(reg0 & SER_R0_CTS) && ser_status[unit].cts != 0)
			ser_status[unit].dcts = 1;
	ser_status[unit].cts = reg0 & SER_R0_CTS;

	if (reg0 & SER_R0_TXUNDERRUN)
	    SER_DOCNTL(unit, 0, SER_W0_RSTTXUNDERRUN);

	SER_DOCNTL(unit, 0, SER_W0_RSTESINTS);
	SER_DOCNTL(unit, 0, SER_W0_RSTIUS);
	break;
      case 2:	/* recv char available */
	ch = SCCRDWR(unit);
	c = 1;
	if (SER_STATUS(unit, 0) & SER_R0_RXREADY) {
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

	SER_DOCNTL(unit, 0, SER_W0_RSTIUS);
	break;
      case 3:	/* spec recv condition */
	reg1 = SER_STATUS(unit, 1);
	SCCRDWR(unit); /* flush fifo */
	if (reg1 & SER_R1_RXOVERRUN)
		ser_status[unit].over++;
	SER_DOCNTL(unit, 0, SER_W0_RSTERR);
	SER_DOCNTL(unit, 0, SER_W0_RSTIUS);
	break;
    }

    return(1);
    /* end of serial interrupt code */
}

/* serial software interrupt. do all the things we could
   not do at splscc();
*/

void
sersir()
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

int
serioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	register struct tty *tp;
	register int unit = UNIT(dev);
	register int error;
 
#if defined(DEBUG)
	printf("ser: entering ioctl()\n");
#endif
	tp = ser_tty[unit];
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, data, flag);
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
		(void) serctl(dev, SER_W5_DTR | SER_W5_RTS, DMBIS);
		break;

	case TIOCCDTR:	/* clear DTR */
		(void) serctl(dev, SER_W5_DTR | SER_W5_RTS, DMBIC);
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

	default:
		printf("ser%d: ioctl(,%d,,)\n", UNIT(dev), cmd);
		return (ENOTTY);
	}
#if defined(DEBUG)
	printf("ser: exiting ioctl()\n");
#endif
	return (0);
}

int
serparam(tp, t)
	register struct tty *tp;
	register struct termios *t;
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

printf("ospeed %d\n", ospeed);
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
	serctl(unit, SER_W5_DTR | SER_W5_RTS, DMSET);

	/* End of serial specific param code */

#if defined(DEBUG)
	printf("ser: exiting serparam()\n");
#endif
	return (0);
}

ser_calc_regs(unit, cflag, preg3, preg4, preg5)
	int unit;
	unsigned char *preg3, *preg4, *preg5;
{
	unsigned char r3, r4, r5;

	r3 = SER_W3_ENBRX;
	r5 = SER_W5_ENBTX;
	if (ser_status[unit].dtr)
		r5 |= SER_W5_DTR;
	if (ser_status[unit].rts)
		r5 |= SER_W5_RTS;
	switch (cflag&CSIZE) {
	    case CS5:
		r3 |= SER_W3_RX5DBITS;
		r5 |= SER_W5_TX5DBITS;
		break;
	    case CS6:
		r3 |= SER_W3_RX6DBITS;
		r5 |= SER_W5_TX6DBITS;
		break;
	    case CS7:
		r3 |= SER_W3_RX7DBITS;
		r5 |= SER_W5_TX7DBITS;
		break;
	    case CS8:
		r3 |= SER_W3_RX8DBITS;
		r5 |= SER_W5_TX8DBITS;
		break;
	}
	r4 = 0;
	if(cflag & PARENB)
		r4 |= (cflag & PARODD) ? SER_W4_PARODD : SER_W4_PAREVEN;
	if(cflag & CSTOPB)
		r4 |= SER_W4_2SBIT;
	else
		r4 |= SER_W4_1SBIT;
	
	*preg3 = r3;
	*preg4 = r4;
	*preg5 = r5;
}
 
int
serstart(tp)
	register struct tty *tp;
{
	int s, s1;
	int i, space, unit, c, need_start;
 
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
		c = (char)getc(&tp->t_outq);
		need_start = 1;
	} else
		need_start = 0;

	/* put characters into a buffer that serintr() will empty */
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
		SCCRDWR(unit) = c;	/* to start chain */
		splx(s1);
	}

out:
	splx(s);
}
 
/*
 * Stop output on a line.
 */
/*ARGSUSED*/
serstop(tp, flag)
	register struct tty *tp;
	int flag;
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

serctl(dev, bits, how)
	dev_t dev;
	int bits, how;
{
	int unit, s;

	unit = UNIT(dev);
	/* run at splhigh so we don't get interrupted by i/o */
	s = splhigh();
	switch (how) {

	case DMSET:
		ser_status[unit].dtr = bits & SER_W5_DTR;
		ser_status[unit].rts = bits & SER_W5_RTS;
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
			SER_DOCNTL(unit, 0, SER_W0_RSTERR);
			SER_DOCNTL(unit, 0, SER_W0_RSTIUS);
			SER_DOCNTL(unit, 1,
				   SER_W1_ENBEXTINT |
				   SER_W1_ENBRXINT |
				   SER_W1_ENBTXINT);
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



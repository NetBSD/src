/*-
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
 */
#ident "$Id: ser.c,v 1.1.1.1 1993/09/29 06:09:27 briggs Exp $"

/*
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

#include "device.h"
#include "serreg.h"
#include "machine/cpu.h"


#define DEBUG

volatile unsigned char *sccA = (unsigned char *) 0x50004000;

int	serstart(), serparam(), serintr();
int	ser_active = 0;
int	nser = NSER;
int	serdefaultrate = TTYDEF_SPEED;
int	sermajor;
struct	tty *ser_tty[NSER];

extern	struct tty *constty;

#define	UNIT(x)		minor(x)


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
	5, 0xea,
	14, 0x01,
	15, 0x80,
	12, 4,
	13, 0,
	1, 0x13,
	0, 0x10,
	0, 0x20,
	0, 0x08,
	9, 0x0C
#endif
};

unsigned char ser_1_init_bytes[]={
	2, 0,
	10, 0,
	11, 0x50,
	4, 0x44,
	3, 0xc1,
	5, 0xea,
	14, 0x01,
	15, 0x80,
	12, 4,
	13, 0,
	1, 0x13,
	0, 0x10,
	0, 0x20,
	0, 0x08,
	9, 0x0C
};


void SER_DOCNTL(
	int unit,
	int reg,
	int data)
{
	if(reg != 0)
		SCCCNTL(unit) = (unsigned char)reg;
	SCCCNTL(unit) = (unsigned char)data;
}

int SER_STATUS(
	int unit,
	int reg)
{
	if(reg != 0)
		SCCCNTL(unit) = (unsigned char)reg;
	return(SCCCNTL(unit));
}


serinit(register struct macdriver *md)
{
extern	int serial_boot_echo;
	int bcount;
	int i;
	int s;

#if defined(DEBUG)
	printf("ser: entering serinit()\n");
#endif

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
	for(i = 0; i < 10; i++)
		SCCRDWR(0) = "Hello, 0\r\n"[i];

	splx(s);

#if 1
	s = splhigh();
		/* initialize port 1 */
	bcount = sizeof(ser_1_init_bytes);
	for(i = 0; i < bcount; i += 2){
		SER_DOCNTL(1, ser_1_init_bytes[i], ser_1_init_bytes[i + 1]);
	}
	for(i = 0; i < 10; i++)
		SCCRDWR(1) = "Hello, 1\r\n"[i];

	splx(s);

#endif

#if defined(DEBUG)
	printf("ser: exiting serinit()\n");
#endif
	return(1);
}


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
	/* SER_DOCNTL(unit, 5, SER_W5_RTSON | SER_W5_DTRON); */
	if(SER_STATUS(unit, 0) & SER_R0_DCD)
		tp->t_state |= TS_CARR_ON;
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
 
#if defined(DEBUG)
	printf("ser: entered serclose()\n");
#endif
	unit = UNIT(dev);
	tp = ser_tty[unit];
	(*linesw[tp->t_line].l_close)(tp, flag);

		/* serial device close code */
	/* SER_DOCNTL(unit, 5, SER_W5_BREAK); */
	/* SER_DOCNTL(unit, 1, 0);	/* Disables all ints */
	if (tp->t_cflag&HUPCL || tp->t_state&TS_WOPEN ||
	    (tp->t_state&TS_ISOPEN) == 0)
		SER_DOCNTL(unit, 5, 0);	/* turn RTS and DTR off */
	ser_active &= ~(1 << unit);
		/* end of serial device close code */

	ttyclose(tp);
	ttyfree(tp);
	ser_tty[unit] = NULL;
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

#define OUTBUFLEN	128
static unsigned char ser_outbuf[NSER][OUTBUFLEN];
static unsigned char ser_outlen[NSER] = {0,0};
static unsigned char ser_outtail[NSER] = {0,0};
 
serintr()
{
	/* serial interrupt code */
   /* This function is called by locore.s on a level 4 interrupt. */

   int reg0, ch, s;
   unsigned char c;
   char str[20];
   register struct tty *tp;
   static int int_so_far = 0;

   if(int_so_far < 10)printf("ser: interrupt %d\n", int_so_far++);

   tp = ser_tty[0];

   reg0 = SCCCNTL(0);	/* Should be SER_STATUS(0, 0) */

   while(reg0 & SER_R0_RXREADY)
   {
      printf("ser:0 rx rdy\n");
      ch = SCCRDWR(0);
      SER_DOCNTL(0, 0, SER_W0_RSTIUS); /* Reset external/status interrupt */
      if ((tp->t_state & TS_ISOPEN) != 0)
         (*linesw[tp->t_line].l_rint)(ch, tp);
   }

   if(reg0 & SER_R0_TXREADY)
   {
      /* write_ack = 0; */
      printf("ser:0 tx rdy\n");
      SER_DOCNTL(0, 0, SER_W0_RSTTXPND);
      SER_DOCNTL(0, 0, SER_W0_RSTIUS);
      if (ser_outlen[0] > 0)
      {
        s = splhigh();
        c = ser_outbuf[0][ser_outtail[0]];
        ser_outtail[0] = (ser_outtail[0] + 1) % OUTBUFLEN;
	SCCRDWR(0) = c;
        /* write_ack = 1; */
        ser_outlen[0]--;
        splx(s);
      }
   }

   tp = ser_tty[1];
   reg0 = SCCCNTL(1);	/* Should be SER_STATUS(1, 0) */

   while(reg0 & SER_R0_RXREADY)
   {
      printf("ser:1 rx rdy\n");
      ch = SCCRDWR(1);
      SER_DOCNTL(1, 0, SER_W0_RSTIUS); /* Reset external/status interrupt */
      if ((tp->t_state & TS_ISOPEN) != 0)
         (*linesw[tp->t_line].l_rint)(ch, tp);
   }

   if(reg0 & SER_R0_TXREADY)
   {
      /* write_ack = 0; */
      printf("ser:1 tx rdy\n");
      SER_DOCNTL(1, 0, SER_W0_RSTTXPND);
      SER_DOCNTL(1, 0, SER_W0_RSTIUS);
      if (ser_outlen[1] > 0)
      {
        s = splhigh();
        c = ser_outbuf[1][ser_outtail[1]];
        ser_outtail[1] = (ser_outtail[1] + 1) % OUTBUFLEN;
	printf("ser1: sent '%c'\n", c);
	SCCRDWR(1) = c;
        /* write_ack = 1; */
        ser_outlen[1]--;
        splx(s);
      }
   }

   return(1);
	/* end of serial interrupt code */
}


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

	case TIOCSDTR:	/* set DTR */
		(void) dcamctl(dev, MCR_DTR | MCR_RTS, DMBIS);
		break;

	case TIOCCDTR:	/* clear DTR */
		(void) dcamctl(dev, MCR_DTR | MCR_RTS, DMBIC);
		break;

	case TIOCMSET:	/* set modem control bits */
		(void) dcamctl(dev, *(int *)data, DMSET);
		break;

	case TIOCMBIS:	/* OR bits on */
		(void) dcamctl(dev, *(int *)data, DMBIS);
		break;

	case TIOCMBIC:  /* AND bits off */
		(void) dcamctl(dev, *(int *)data, DMBIC);
		break;

	case TIOCMGET:	/* get modem bits */
		*(int *)data = dcamctl(dev, 0, DMGET);
		break;

#endif
	default:
		printf("ser%d: ioctl(,%d,,)\n", UNIT(dev), cmd);
		return (ENOTTY);
	}
#if defined(DEBUG)
	printf("ser: exiting ioctl()\n");
#endif
	return (0);
}

serparam(tp, t)
	register struct tty *tp;
	register struct termios *t;
{
	register int cflag = t->c_cflag;
	unsigned char rbits, dbits, reg4;
	int unit = UNIT(tp->t_dev);
	int ospeed = t->c_ospeed;

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
	/* SER_DOCNTL(unit, 1, SER_W1_ENBTXRDY | SER_W1_ENBRXRDY); */
	/* SER_DOCNTL(unit, 0, SER_W0_ENBRXRDY); */
	if(ospeed == 0)			/* Hang up */
		SER_DOCNTL(unit, 5, 0);	/* turn RTS and DTR off */
	SER_DOCNTL(unit, 12, SERBRD(ospeed) & 0xff);
	SER_DOCNTL(unit, 13, (SERBRD(ospeed) >> 8) & 0xff);
	rbits = SER_W3_ENBRX;
	dbits = SER_W5_ENBTX;
	switch (cflag&CSIZE) {
	    case CS5:
		rbits |= SER_W3_RX5DBITS;
		dbits |= SER_W5_TX5DBITS;
		break;
	    case CS6:
		rbits |= SER_W3_RX6DBITS;
		dbits |= SER_W5_TX6DBITS;
		break;
	    case CS7:
		rbits |= SER_W3_RX7DBITS;
		dbits |= SER_W5_TX7DBITS;
		break;
	    case CS8:
		rbits |= SER_W3_RX8DBITS;
		dbits |= SER_W5_TX8DBITS;
		break;
	}
	reg4 = 0;
	if(cflag & PARENB)
		reg4 |= (cflag & PARODD) ? SER_W4_PARODD : SER_W4_PAREVEN;
	if(cflag & CSTOPB)
		reg4 |= SER_W4_2SBIT;
	else
		reg4 |= SER_W4_1SBIT;
	/* SER_DOCNTL(unit, 3, dbits); */
	/* SER_DOCNTL(unit, 5, rbits); */
	/* SER_DOCNTL(unit, 4, reg4); */
		/* End of serial specific param code */

#if defined(DEBUG)
	printf("ser: exiting serparam()\n");
#endif
	return (0);
}
 
serstart(tp)
	register struct tty *tp;
{
	register struct dcadevice *dca;
	int s, unit, c;
	int i;
 
#if defined(DEBUG)
	printf("ser: entering serstart()\n");
#endif
	unit = UNIT(tp->t_dev);
	s = spltty();
	if (tp->t_state & (TS_TIMEOUT|TS_TTSTOP))
		goto out;
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state&TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		selwakeup(&(tp->t_wsel));
	}
	if (tp->t_outq.c_cc == 0)
		goto out;
	tp->t_state |= TS_BUSY;
	if(tp->t_outq.c_cc){
		/* Really should check to see if the port is busy. */
		c = (char)getc(&tp->t_outq);
	}
	while(tp->t_outq.c_cc)
	{
		/* put characters into a buffer that serintr() will empty */
		/*  out on transmit-ready interrupts. */
		while(tp->t_outq.c_cc && ser_outlen[unit] < OUTBUFLEN){
			ser_outbuf[unit][ser_outtail[unit] + (ser_outlen[unit]++)
			    % OUTBUFLEN] = (char)getc(&tp->t_outq);
		}
	}
	tp->t_state &= ~TS_BUSY;

	SER_DOCNTL(unit, 0, 0x48);
	SCCRDWR(unit) = c;	/* to start chain */

out:
#if defined(DEBUG)
	printf("ser: exiting serstart()\n");
#endif
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

/*	$NetBSD: ser.c,v 1.17 1995/02/11 19:06:57 briggs Exp $	*/

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
 *      Further disgusting kludges by Dave Leonard, <d@fnarg.net.au>
 *	      redocumented z8530 stuff
 *	      added break ioctls and detection
 *	      attempt at bringing all chip ops together
 *				deepened s/w fifos to match chip's
 *				added TIOC[SG]FLAG
 *				farming/parity errors passed to discipline
 *
 *      Hacked by Brad Parker, <brad@fcr.com>
 *	      added CTS input flow control
 *	      added DCD event detection
 *	      added software fifo's
 *
 *      Mac II serial device interface
 *
 *      Information used in this source was gleaned from low-memory
 *       global variables in MacOS and the Advanced Micro Devices
 *       1992 Data Book/Handbook.
 *
 *      Also from the Zilog Scc User's manual for the z85x30
 *
 *--
 *
 * Notes on complying with appletalk (not yet in this module)
 #
 *	unlike system 7 which requires a reboot when you change the 
 *	usage of a port from serial to appletalk, it should be possible
 *	to re-configure the chip on the fly (just like old System 6s)
 *
 *	This driver should be augmented to provide LLAP.
 *
 *	To quote from a Zilog manual:
 *	"A majority of the difficult timing and all of the hardware interface 
 *	 problems crop up in the LLAP driver. These problems are so difficult 
 *	 that it makes sense to start writing such a driver by writing 
 *	 experimental routines that transmit and recieve frames."
 *	The manual then goes on to provide discussion and code for a z80181
 *	based implementation.
 *
 *	LLAP uses csma/ca and implementations with zilog's SCC put it into
 *	SDLC (synchronous) mode. ie each frame starts with a 01111110 sequence 
 *	(which is why you sometimes see ~'s when localtalk kicks in on a 
 *	dumb terminal) The frame ends with a sequence of 12 to 18 1's which
 *	the SCC interprets as a Abort sequence and loses sync. This is handy
 *	because you can prepare your packet then wait for the abort interrupt
 *	to send it.
 *
 *	A faster way gets you to pulse the RTS line for at least one bit time
 *	and then idle it for at least 2 bit times to check for collision
 *	before sending. "This syncronisation is obtained by first enabling
 *	the hardware line so that an edge is detected by all the recievers on
 *	the network." It looks like the start of a clocking period. The
 *	other SCCs see the idle period shortly after and "assume the clock
 *	has been lost (missing clock bit set on RR10)". Zilog dont have an
 *	interrupt on RR10, so it must be polled. :(
 *
 *	LLAP nodes have a dynamically allocated node ID. ie you get a random
 *	one then check to see if anyone else has got it by sending a lapENQ
 *	and if no-one winges withing 200 usec, then its yours. (You have to
 *	send a good couple of lapENQs in case the packet was lost, or the 
 *	winging node was busy, etc) Node ID 0xFF is a broadcast address.
 *
 *	The 200usec and stuff can be timed by the chip by setting a counter
 *	and getting an interrupt when it reaches zero.
 *
 *	LLAP packets are made up of:
 *	
 *	1	dest ID
 *	1	source ID
 *	1	LLAP type	{ 0x0 ... 0x7f, lapENQ, lapACK, lapRTS, lapCTS}
 *	0-600	data
 *	1	CRC (TxUnderrun Int.)
 *	1	CRC
 *	1	flag
 *
 * Init string template for LLAP:
 *	 4=0x20 1=0x00 2=0x00 3=0xcc 5=0x60 6=0x00 7=0x7e 8=0x01
 *	 10=0xe0 11=0xf6 12=0x06 13=0x0 14=0x60 14=0xc0 14=0xa0 
 *	 14=0x20 14=0x01 3=0xcc 15=0x0 16=0x10 1=0x00 9=0x09
 *	(go through this with a fine comb)
 *	
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
#include "serreg.h"
#include <machine/cpu.h>
#include <machine/frame.h>

#include <dev/cons.h>

#undef DEBUG
/* #define DEBUG /**/

/*#define use_hw_flow*/ /* attempt to use hw flow control -buggy */

volatile unsigned char *sccA = (unsigned char *) 0x4000;

static void     serstart __P((register struct tty *));
static int      serparam __P((register struct tty *, register struct termios *));
static int      serctl   __P((dev_t dev, int bits, int how));
extern int      ser_intr  __P((void));

static int      nser = NSER;
static int      serdefaultrate = TTYDEF_SPEED;

struct  tty *ser_tty[NSER];

extern  struct tty *constty;

#define UNIT(x)	 minor(x)

/*
 * This'll end up in a header file before (next) Christmas
 */

/*
 * This structure is persistent across open/closes and is 
 * zeroed only at init time - it is reliable as the current state 
 * of the serial channel.
 */

#define SER_WRITEREG(unit,reg,bits)  \
			ser_status[unit].wr[reg]=(bits),\
			SER_DOCNTL(unit,reg,bits)

#define SERCTL_DTR	0x001	/* DTR */
#define SERCTL_RTS	0x002	/* RTS */
#define SERCTL_AUTO	0x004	/* chip level hardware flow control */
#define SERCTL_DCD	0x008	/* DCD [readonly] */
#define SERCTL_BRK	0x010	/* raise break */
#define SERCTL_CTS	0x020	/* CTS [readonly] */
#define SERCTL_BUSY     0x040	/* tx in progress [readonly] */
#define SERCTL_INHIB	0X080	/* hardware inhibit of transmitter */
#define SERCTL_INBRK	0x100	/* break recv'd before this char [ro]*/

/* writable bits */
#define SERCTL_MASK		( /* bits that can be user modified */ \
						SERCTL_DTR|		\
						SERCTL_RTS|		\
						SERCTL_AUTO|	\
						SERCTL_BRK|		\
						SERCTL_INHIB|	\
					0)
/* 
 * NOTE: by some magical coincidence, SERCTL_{DCD,RTS} match up with
 * SER_R0_{DCD,CTS}!!! This feature isn't used yet; but helps in debugging.
 */

volatile
struct ser_status {
	unsigned char	wr[16];		/* last register writes */

	unsigned char	ddcd, dcts;     /* delta (change) flags */
	int     	oflo;	   /* s/w fifo over flow */
	int     	over;	   /* h/w fifo over flow */
	unsigned char	intpend;	/* number of sw ints pending */

	int		state;		/* (intended|derived) state of chip */
} ser_status[NSER];

#define SCC_INT	 10
#define SCC_SPEED       11

/*
 * End of stuff for header file.
 */

#ifdef DEBUG
/*
 * This is one big printf that dumps all the interesting
 * info about a serial port
 */

/* DBG used to restrict debugging to a particular serial port */
#define DBG(u) ((u)==0)/**/

/* fwd decl */
static ser_apply_state(int);

static void
ser_dbg( char*msg, int unit )
{
	volatile struct ser_status *s = &ser_status[unit];
	struct tty *t = ser_tty[unit];

	if (!DBG(unit)) return;

	if (!t) {  /* sanity */
		printf("ser%d: %s (no attached tty)\n",unit,msg);
		return;
	}

	printf( "ser%d: %s %s%s"
 		" %s=(%s%s%s%s%s%s%s%s%s)" /*ser*/
		" %s=(%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s)" /*tty*/
		" %s=(%s%s%s%s%s%s%s%s%s%s%s%s%s%s)" /*cflag*/
		"\n",
		unit,msg,

		s->oflo?"oflo ":"", s->over?"over ":"",

		"ser",
		    s->state&SERCTL_BUSY?"busy ":"",
		    s->state&SERCTL_INHIB?"txinhib ":"",
		    s->state&SERCTL_INBRK?"inbrk ":"",
		    s->state&SERCTL_DTR?"DTR ":"",
		    s->state&SERCTL_RTS?"RTS ":"",
		    s->state&SERCTL_BRK?"BRK ":"",
		    s->state&SERCTL_DCD?"DCD ":"",
		    s->state&SERCTL_CTS?"CTS ":"",
		    s->state&SERCTL_AUTO?"AUTO ":"",

		"tty",
		    t->t_state&TS_ASLEEP    ?"ASLEEP ":"",
		    t->t_state&TS_ASYNC     ?"ASYNC ":"",
		    t->t_state&TS_BUSY      ?"BUSY ":"",
		    t->t_state&TS_CARR_ON   ?"CARR_ON ":"",
		    t->t_state&TS_FLUSH     ?"FLUSH ":"",
		    t->t_state&TS_ISOPEN    ?"ISOPEN ":"",
		    t->t_state&TS_TBLOCK    ?"TBLOCK ":"",
		    t->t_state&TS_TIMEOUT   ?"TIMEOUT ":"",
		    t->t_state&TS_TTSTOP    ?"TTSTOP ":"",
		    t->t_state&TS_WOPEN     ?"WOPEN ":"",
		    t->t_state&TS_XCLUDE    ?"XCLUDE ":"",
		    t->t_state&TS_BKSL      ?"BKSL ":"",
		    t->t_state&TS_CNTTB     ?"CNTTB ":"",
		    t->t_state&TS_ERASE     ?"ERASE ":"",
		    t->t_state&TS_LNCH      ?"LNCH ":"",
		    t->t_state&TS_TYPEN     ?"TYPEN ":"",
		    t->t_state&TS_LOCAL     ?"LOCAL ":"",

		"cflag",
		    t->t_cflag&CIGNORE  ?"CIGNORE ":"",
		    (t->t_cflag&CSIZE)==CS5?"CS5 ":"",
		    (t->t_cflag&CSIZE)==CS6?"CS6 ":"",
		    (t->t_cflag&CSIZE)==CS7?"CS7 ":"",
		    (t->t_cflag&CSIZE)==CS8?"CS8 ":"",
		    t->t_cflag&CSTOPB   ?"CSTOPB ":"",
		    t->t_cflag&CREAD    ?"CREAD ":"",
		    t->t_cflag&PARENB   ?"PARENB ":"",
		    t->t_cflag&PARODD   ?"PARODD ":"",
		    t->t_cflag&HUPCL    ?"HUPCL ":"",
		    t->t_cflag&CLOCAL   ?"CLOCAL ":"",
		    t->t_cflag&CRTSCTS  ?"CRTSCTS ":"",
		    t->t_cflag&MDMBUF   ?"MDMBUF ":"",
		    t->t_cflag&CHWFLOW  ?"CHWFLOW ":"",
		0 );

}
#else
#define DBG(u) 0
#endif DEBUG


/* SCC initialization string from Steve Allen (wormey@eskimo.com) */

static unsigned char ser_init_bytes[]={

     4, SER_W4_PARNONE|	 /* no parity */
	SER_W4_1SBIT|	   /* one stop bit async */
	SER_W4_CLKX16,	  /* clock = 16 * data rate */

     3, SER_W3_RX8DBITS,	/* 8 bit data rx'd */
				/* dont enable RX yet */

     5, /*SER_W5_DTR|	     /* enable DTR */
	/*SER_W5_RTS|	     /* enable RTS */
	SER_W5_TX8DBITS,	/* 8 data bits tx'd */
				/* TX disabled */

     9, SER_W9_NV|	      /* No vector set up yet */
	SER_W9_DLC,	     /* disable lower chain */
				/* do not enable master interrupt (mie) yet */

    10, SER_W10_NRZ,	    /* nrz mode */

    11, SER_W11_TXBR|	   /* source of tx clock is Baud rate gen */
	SER_W11_RXBR,	   /* source of rx clock is baud rate gen */

    12, 0x04,		   /* time constant LB. Will be replaced below */
    13, 0x00,		   /* time constant HB. Will be replaced below */

    14, 0x00,		   /* don't enable the BR yet */

     5, /*SER_W5_DTR|*/
	/*SER_W5_RTS|*/
	SER_W5_TX8DBITS|
	SER_W5_ENBTX,	   /* enable tx */

     3, SER_W3_RX8DBITS|
	SER_W3_ENBRX,	   /* enable rx */

     0, 0x80,		   /* reset txCRC.  XXX is this needed? */

    14, SER_W14_ENBBR,	  /* enable BR. DPLL not affected */

     1, 0x00,		   /* WAIT/DMA request, disable rx int */

    15, SER_W15_DCDINT|	 /* enable DCD, CTS and Break interrupts */
	SER_W15_CTSINT|
	SER_W15_BRKINT,

     0, SER_W0_RSTESINTS,       /* reset ext/status ints */
     0, SER_W0_RSTESINTS,       /* twice */

   0xff,0xff,			/* marker before interrupts are enabled */

     1, SER_W1_ENBTXINT|	/* enable tx intertrupts */
		SER_W1_ENBRXINT,
       /* SER_W1_ENBR1INT,	/* and 1st rx */

     9, SER_W9_NV|
	SER_W9_DLC|
	SER_W9_MIE,	     /* globally enable interrupts */
};

extern int matchbyname();

static void
serinit(int running_interrupts)
{
static  int initted=0;
	int bcount;
	int i, s, spd;
		int unit;

	/*
	 * Will be called twice if we're running a serial console.
	 */

	if (initted++)
		return;

	sccA = IOBase + sccA;

	spd = SERBRD(serdefaultrate);

	s = splhigh();

	SER_WRITEREG(0, 9, SER_W9_HWRESET);

	/* short delay after the hardware reset */
	/* take this time to init some structs */

	for(unit=0;unit<NSER;unit++)
		bzero((char *)&ser_status[unit], sizeof(struct ser_status));

	/*
	 * initialize ports, substituting proper speed.
	 */

	bcount = sizeof(ser_init_bytes);
	for(i = 0; i < bcount; i += 2) {
		/*
		 * Baud rate, LSB
		 */
		if (ser_init_bytes[i]==12)
			ser_init_bytes[i+1] = (spd & 0xff);
		/*
		 * Baud rate, MSB
		 */
		if (ser_init_bytes[i]==13)
			ser_init_bytes[i+1] = ((spd>>8) & 0xff);

		if (ser_init_bytes[i]==0xff) {
			if (!running_interrupts) break;
		} else {
			for(unit=0;unit<NSER;unit++) 
				SER_WRITEREG(unit, ser_init_bytes[i], 
						ser_init_bytes[i + 1]);
		}
	}

	splx(s);
}

static void
serattach(parent, dev, aux)
	struct device   *parent, *dev;
	void	    *aux;
{
	if (mac68k_machine.serial_boot_echo) {
		printf(" (serial boot echo is on)\n");
	}
	printf("\n");

	serinit(1);
}

struct cfdriver sercd =
      { NULL, "ser", matchbyname, serattach,
	DV_TTY, sizeof(struct device), NULL, 0 };

/* ARGSUSED */
extern int
seropen(dev_t dev, int flag, int mode, struct proc *p)
{
	register struct tty *tp;
	register int unit;
	unsigned char reg0;
	int error = 0;
 
	unit = UNIT(dev);
	if (unit >= NSER ){
		return (ENXIO);
	}

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

#ifdef DEBUG
	ser_dbg("seropen",unit);
#endif
	/*
	 * set up the initial (unlatched) values of cts rts
	 * assumes that the port is quiescent until interrupts enabled
	 */

	SER_DOCNTL(unit, 15, 
		ser_status[unit].wr[15] & ~(SER_W15_DCDINT|SER_W15_CTSINT) );
	reg0 = SER_STATUS(unit, 0);
	SER_DOCNTL(unit, 15, ser_status[unit].wr[15]);

	ser_status[unit].state = 0;	/* blam */

	SER_WRITEREG(unit, 0, SER_W0_RSTESINTS);

	if (reg0 & SER_R0_DCD)  {
		ser_status[unit].state |= SERCTL_DCD;
		tp->t_state |= TS_CARR_ON;
	}
	if (!/*!!?*/reg0 & SER_R0_CTS)	/* kapow */
		ser_status[unit].state |= SERCTL_CTS;

	/*
	 * enable interrupts here 
	 * because raising RTS/DTR later might 
	 * trigger an external response 
	 */

	serctl(unit, 1, SCC_INT);

	/* turn on RTS & DTR */

	serctl(unit, SERCTL_RTS|SERCTL_DTR, DMSET);

	/* end serial device open code */

	(void) spltty();
	while ((flag&O_NONBLOCK) == 0 && (tp->t_cflag&CLOCAL) == 0 &&
	       (tp->t_state & TS_CARR_ON) == 0) {
		tp->t_state |= TS_WOPEN;
#ifdef DEBUG
		ser_dbg("seropen: sleeping",unit);
#endif
		if (error = ttysleep(tp, (caddr_t)&tp->t_rawq, TTIPRI | PCATCH,
		    ttopen, 0))
			break;
	}
	(void) spl0();
#ifdef DEBUG
	ser_dbg("seropen: ok",unit);
#endif
	if (error == 0)
		error = (*linesw[tp->t_line].l_open)(dev, tp);

	return (error);
}
 
/*ARGSUSED*/
extern int
serclose(dev_t dev, int flag, int mode, struct proc *p)
{
	register struct tty *tp;
	register int unit;
	int s;
 
	unit = UNIT(dev);
	tp = ser_tty[unit];
	(*linesw[tp->t_line].l_close)(tp, flag);

	/* serial device close code */
#ifdef DEBUG
	ser_dbg("serclose",unit);
#endif
	/* disable interrupts */
	serctl(unit, 0, SCC_INT);
	ser_status[unit].state &= ~SERCTL_BUSY;	 /* in case busy */

	if (tp->t_cflag&HUPCL || tp->t_state&TS_WOPEN ||
	    (tp->t_state&TS_ISOPEN) == 0)
				/* turn RTS and DTR off */
		serctl(unit, SERCTL_DTR|SERCTL_RTS, DMBIC); 

	/* ser_active &= ~(1 << unit); */

#ifdef DEBUG
	ser_dbg("serclose: closed",unit);
#endif
	/* end of serial device close code */

	ttyclose(tp);
#ifdef broken
	ttyfree(tp);
	ser_tty[unit] = NULL;
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
 
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

/* private buffers used by driver at splscc() */
#define INBUFLEN	256
#define OUTBUFLEN       512
static unsigned int ser_inbuf[NSER][INBUFLEN]; /* was unsigned char[] */
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
#define HWFIFO 3  /* depth of the hardware fifo */
	unsigned char r0o, reg0, reg1, reg2, reg3, r1[HWFIFO],  c,cdpth;
	unsigned int ch[HWFIFO];
	int s,reset_ext=0;
	volatile int *state;
	register int unit;

	/* read status to reset SCC state machine */

	r0o = SCCCNTL(0);

	/* port B's vector contains info on the interrupt */

	reg2 = SER_STATUS(1, 2);

	switch (reg2 & SER_R2_CHANMASK) {
		case SER_R2_CHANA: unit=0; break;/* channel that caused int */
		case SER_R2_CHANB: unit=1; break;
	}

	reg0 = SER_STATUS(unit, 0);

#ifdef DEBUG
	/*if (DBG(unit))printf("%02x%02x.",reg0,reg2);*/
#endif

	state = &ser_status[unit].state; /* optimisation */

	switch (reg2 & SER_R2_MASK) {

		case SER_R2_TX:    /* tranmitter buffer empty */

			if (ser_outlen[unit] == 0) {
				SER_WRITEREG(unit, 0, SER_W0_RSTTXPND);
				*state &= ~SERCTL_BUSY;
				setsoftserial();
				ser_status[unit].intpend++;
			} else {
				c = ser_outbuf[unit][ser_outtail[unit]];
				ser_outtail[unit] =
					(ser_outtail[unit] + 1) % OUTBUFLEN;
				SCCRDWR(unit) = c;
				ser_outlen[unit]--;
			}

			/*
			 * Flow control - the Zilog way(TM)
			 *
			 * The zilog 85x30 has a magic feature called
			 * 'auto enable' which, in short, allows the CTS pin
			 * to enable/disable the TX line. However, in Zilog's
			 * infinite wisdom, 'auto enable' also forces you to
			 * have DCD {en,dis}able the RX. This is bad if you are
			 * in CLOCAL mode and are using CRTSCTS. 
			 *
			 * The safest thing to do is just fall through to
			 * status check when we are doing flow control and
			 * there's an ext/change int pending - we use magic
			 * there...
			 */

			if ( ser_tty[unit] && ser_tty[unit]->t_cflag&CRTSCTS) {
				reg3 = SER_STATUS(1, 3);
				if ( reg3 & (SER_R3_AIPES|SER_R3_BIPES) )
					goto check_status;
			}

			break;

		case SER_R2_EXTCHG:   /* ext/status change */
check_status:
			reset_ext++;

			/* cts */

			/* XXX why????
			 * for some bizarre reason, cts seems to be inverted
			 * this might be because the interrupt latches the
			 * previous value of the line? then why not for DCD?
			 */
	
			if (  !  ((reg0^*state)&SER_R0_CTS))  {

				*state ^= SERCTL_CTS;

#ifdef DEBUG
				if (DBG(unit))
					printf("c%c",*state&SERCTL_CTS?'+':'-');
#endif
				if (*state & SERCTL_CTS) {

					/* cts is now on */

					/*
					 * we turn on the txinhibit if needed
					 * here it can be turned off at an
					 * upper level
					 */
					if (
#ifdef use_hw_flow
					     /* not on auto, but */
					    !(*state&SERCTL_AUTO) &&
#endif use_hw_flow
					    ser_tty[unit] && 
					    ser_tty[unit]->t_cflag&CRTSCTS
						/* doing flow control */
				   	   ) {
						serctl( unit, SERCTL_INHIB,
							DMBIS ); /* blam */
					}
				} else {
					/* cts is now off */

					/* we were inhibiting */
					if (*state & SERCTL_INHIB) {
						*state&=~SERCTL_INHIB;
						SER_DOCNTL( unit, 5,
				ser_status[unit].wr[5] |= SER_W5_ENBTX );
						/* but not any more */
					}
				}
				ser_status[unit].dcts = 1;
			}

			/* dcd */

			if (   (!(reg0&SER_R0_DCD) &&  (*state&SERCTL_DCD))
			    || ( (reg0&SER_R0_DCD) && !(*state&SERCTL_DCD))) {

#ifdef DEBUG
				if (DBG(unit))
					printf("d%c",reg0&SER_R0_DCD?'+':'-');
#endif

				ser_status[unit].ddcd = 1; /* flag delta */

				if (reg0&SER_R0_DCD) { 
					/* 
				 	 * we have DCD 
				 	 */

					*state |= SERCTL_DCD;
#ifdef use_hw_flow
					/*
					 * ... and need flow control
					 */
					if (   ser_tty[unit]
					    && ser_tty[unit]->t_cflag&CRTSCTS) {
						/* TOO SLOW (but more understandable) */
						/*
						serctl( unit, SERCTL_AUTO, DMBIS );
						serctl( unit, SERCTL_INHIB, DMBIC );
						*/
						/* use the force, luke */
						*state &= ~SERCTL_INHIB; /* take that you scoundrel! */
						*state |=  SERCTL_AUTO; /* and that! */ /* pow */
						SER_DOCNTL(unit,3,ser_status[unit].wr[3]|=SER_W3_AUTOEN);
						SER_DOCNTL(unit,5,ser_status[unit].wr[5]&=~SER_W5_ENBTX);
						/* blam */
					}
#endif use_hw_flow
				} else {
					/*
				 	 * we don't have DCD 
				 	 */
					*state &= ~SERCTL_DCD;
#ifdef use_hw_flow
					if (*state & SERCTL_AUTO)	/* but, omigosh we need auto */
					{
						/* we need to switch-over to manual inhibit */
						if (!(*state & SERCTL_CTS)){/* we have no cts */
							*state |= SERCTL_INHIB; /* so we need to inhibit tx */
							SER_DOCNTL(unit,5,
								ser_status[unit].wr[5]|=SER_W5_ENBTX);
						}
						*state &= ~ SERCTL_AUTO;	/* no more auto */
						SER_DOCNTL(unit,3,ser_status[unit].wr[3]&=~SER_W3_AUTOEN);
					}
#endif use_hw_flow
				}
			}

			/* tx underrun */

			if (reg0 & SER_R0_TXUNDERRUN)
				SER_WRITEREG(unit, 0, SER_W0_RSTTXUNDERRUN);

			/* break */

			if (reg0 & SER_R0_BREAK) {
				/* 
				 * breaks are now enqueued onto the receive queue
				 * and turned into a null with a framing error.
				 * XXX This seems to be broken XXX
				 *
				 * a point to note is that ext/stat interrupts are lower
				 * priority than tx or rx (at the scc level), but 
				 * this should be okay
				 */
#ifdef DEBUG
				/*if (DBG(unit))*/
					printf("b");
#endif
				/* break leaves a null in the input fifo - this will be
				 * picked up when the break ends and sent to the disc */

				*state |= SERCTL_INBRK; /* flag so that next rx knows is a brk */

			}

			setsoftserial();
			ser_status[unit].intpend++;

       			break;

		case SER_R2_RX:   /* recv char available */

			/* fast read of scc's hardware fifo 
			 *    this is actually a race between the cpu and the scc hardware
			 *	  that the cpu will most likely win
			 *
			 * We'll get an rxoverrun if our s/w buffer fills up first :(
			 */

			for(c=0;c<HWFIFO && (SER_STATUS(unit, 0) & SER_R0_RXREADY); c++) {
				r1[c] = SER_STATUS(unit, 1);
				if (r1[c]&(SER_R1_PARITYERR|SER_R1_CRCERR))
					/* clear parity/framing errs */
					SER_WRITEREG(unit,0,SER_W0_RSTERR); 
				ch[c] = SCCRDWR(unit);
			}

			/* now that we have siphoned it into an array on the stack, 
			 * put it into a global */

			for(cdpth=c,c=0;c<cdpth;c++) {
				if (*state & SERCTL_INBRK /*&& !ch[c]*/) {
					*state &= ~SERCTL_INBRK;
					ch[c] = TTY_FE;	/* null with Frame Error == Break */
				} else {
					if (r1[c]&SER_R1_PARITYERR) ch[c]|=TTY_PE;
					if (r1[c]&SER_R1_CRCERR) ch[c]|=TTY_FE;
				}
				if (ser_inlen[unit] < INBUFLEN) 
					ser_inbuf [unit] 
						[(ser_intail[unit] + (ser_inlen[unit]++)) % INBUFLEN] 
							= ch[c];
				else ser_status[unit].oflo++;
			}

			setsoftserial();
			ser_status[unit].intpend++;

			break;

		case SER_R2_SPEC:   /* spec recv condition */

			reg1 = SER_STATUS(unit, 1);
			SCCRDWR(unit); /* flush fifo */
			if (reg1 & SER_R1_RXOVERRUN)
				ser_status[unit].over++;
			SER_WRITEREG(unit, 0, SER_W0_RSTERR);
			break;

		default: /* in case the #define's were set up badly */
			printf("ser%d: unknown serial interrupt 0x%x\n",unit,reg2);
			panic("bizarre interrupt");
	}

	if (reset_ext) SER_WRITEREG(unit, 0, SER_W0_RSTESINTS);
	SER_WRITEREG(unit, 0, SER_W0_RSTIUS);

	return(1);
	/* end of serial interrupt code */
}

/*
 * serial software interrupt. do all the things we could
 * not do at splscc();
 */

extern void
sersir(void)
{
	int unit, s, c;
	register struct tty *tp;

	for (unit = 0; unit < NSER; unit++) {
		if ((tp = ser_tty[unit]) == 0)
			continue;

		if (!ser_status[unit].intpend) continue;
#ifdef DEBUG
		/* if (DBG(unit)) printf(","); */
#endif
		s=splhigh();
		ser_status[unit].intpend--;
		splx(s);

		/*
		 * check for change in CTS
		 * this done first when doing s/w crtscts flow
		 */

		if (ser_status[unit].dcts) {
			ser_status[unit].dcts = 0;
			if ((tp->t_state & TS_ISOPEN) && (tp->t_flags & CRTSCTS)){
				if (ser_status[unit].state & SERCTL_CTS) {
					tp->t_state &= ~TS_TTSTOP;

					/*
					 * next 3 lines not needed when AUTO flow is on,
					 * but it doesn't hurt
					 */
					serctl(unit, SERCTL_INHIB, DMBIC );

					serstart(tp);
				} else {
					tp->t_state |= TS_TTSTOP;
				}
			}
#ifdef DEBUG
			ser_dbg("cts",unit);
#endif
		}

		/*
		 * check for overflows
		 */
		if (ser_status[unit].oflo || ser_status[unit].over) {
			int ooflo;
			s = splhigh();
			ooflo = ser_status[unit].oflo;
			ser_status[unit].oflo = 0;
			ser_status[unit].over = 0;
			splx(s);
			/* where's _our_ flow control? */
			printf("ser%d: silo overflow, oflo = %d\n",unit,ooflo);
			if (tp->t_state & TS_ISOPEN)
				(*linesw[tp->t_line].l_rint)('#', tp);
		}

		/*
		 * check for change in DCD
		 */

		if (ser_status[unit].ddcd) {

			s = splhigh();
			ser_status[unit].ddcd = 0;
			splx(s);

			/*
			 * the sun3 (and sparc) ports also have a 8530 (zs)
			 * and in that code, the authors winge about the
			 * zilog hardware flow control braindeath syndrome.
			 * [when dcd goes low with hw flow tx is disabled]
			 * Their solution is to turn hfc off when dcd goes 
			 * off. We do similar, but instead, allow a software
			 * flow control that tx and rx interrupts try to 
			 * figure out
			 */
#ifdef use_hw_flow
			if (ser_status[unit].state & SERCTL_DCD) {
				tp->t_state |= TS_CARR_ON;
				if (tp->t_cflag&CRTSCTS)
					serctl( tp->t_dev, SERCTL_AUTO, DMBIS );
			} else {
				tp->t_state &= ~TS_CARR_ON;
				if (tp->t_cflag&CRTSCTS)
					serctl( tp->t_dev, SERCTL_AUTO, DMBIC );
			}
#endif use_hw_flow

#ifdef DEBUG
			ser_dbg("carrier",unit);
#endif
			(*linesw[tp->t_line].l_modem)(tp,
					(ser_status[unit].state & SERCTL_DCD) ? 1 : 0);

		}

		/*
		 * drain input fifo
		 */
		while (ser_inlen[unit] > 0) {
			if (tp->t_rawq.c_cc + tp->t_canq.c_cc >= TTYHOG) {
				setsoftserial();
				ser_status[unit].intpend++;
				break;
			}
			s = splhigh();
			c = ser_inbuf[unit][ser_intail[unit]];
			ser_intail[unit] = (ser_intail[unit] + 1) % INBUFLEN;
			ser_inlen[unit]--;
			splx(s);
#ifdef DEBUG
			if ( c&(TTY_FE|TTY_PE) || !c) 
				printf("ser%d: l_rint( %x, )\n", unit, c);
#endif
			if (tp->t_state & TS_ISOPEN)
				(*linesw[tp->t_line].l_rint)(c, tp);
		}
		/*
		 * fill output fifo
		 */
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
 
	if (unit>NSER) return ENOTTY;

	tp = ser_tty[unit];

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);

	switch (cmd) {

		case TIOCSBRK:  /* turn break on */
			(void) serctl(dev, SERCTL_BRK, DMBIS);
			break;

		case TIOCCBRK:  /* turn break off */
			(void) serctl(dev, SERCTL_BRK, DMBIC);
			break;

		case TIOCSDTR:  /* set DTR */
			(void) serctl(dev, SERCTL_DTR | SERCTL_RTS, DMBIS);
			break;

		case TIOCCDTR:  /* clear DTR */
			(void) serctl(dev, SERCTL_DTR | SERCTL_RTS, DMBIC);
			break;

		case TIOCGFLAGS:
			{
				int     bits = 0;

				if (!tp) return(ENOTTY);

				/*
				 * the (possibly incorrect) strategy here is
				 * to keep the tty structure up to date
				 * with TIOC flags
				 */

				if (tp->t_cflag&CLOCAL)  {
					bits|=TIOCFLAG_CLOCAL;
					bits|=TIOCFLAG_SOFTCAR;
				}

				if (tp->t_cflag&CRTSCTS) 
					bits|=TIOCFLAG_CRTSCTS;

				if (tp->t_cflag&MDMBUF)  
					bits|=TIOCFLAG_MDMBUF;


				*(int *)data = bits;
				break;
			}

	case TIOCSFLAGS:
			{
				int     userbits, driverbits = 0;

				if (!tp) return(ENOTTY);
				error = suser(p->p_ucred, &p->p_acflag);
				if (error != 0)
					return (EPERM);

				userbits = *(int *)data;

				if (userbits&(TIOCFLAG_CLOCAL|TIOCFLAG_SOFTCAR))
					tp->t_cflag |= CLOCAL;
				else
					tp->t_cflag &=~CLOCAL;

				if (userbits&TIOCFLAG_CRTSCTS)
					tp->t_cflag |= CRTSCTS;
				else 
					tp->t_cflag &=~CRTSCTS;

				if (userbits&TIOCFLAG_MDMBUF)
					tp->t_cflag |= MDMBUF;
				else
					tp->t_cflag &=~MDMBUF;

				break;
			}

#ifdef allow_fiddle
		case TIOCMSET:  /* set modem control bits */
			(void) serctl(dev, *(int *)data, DMSET);
			break;

		case TIOCMBIS:  /* OR bits on */
			(void) serctl(dev, *(int *)data, DMBIS);
			break;

		case TIOCMBIC:  /* AND bits off */
			(void) serctl(dev, *(int *)data, DMBIC);
			break;

		case TIOCMGET:  /* get modem bits */
			*(int *)data = serctl(dev, 0, DMGET);
			break;
#endif allow_fiddle

		default:
			return (ENOTTY);
	}
	return (0);
}

static int
ser_apply_state( int unit )
{
	int s, state = ser_status[unit].state;
	struct tty*tp = ser_tty[unit];
	unsigned char r3, r4, r5;

	/*
	 * Here, we read the status bits from ser_status.state
	 * and write to write registers 3 4 and 5.
	 * In addition, we look at the tty struct (if any)
	 * and set up some stuff using that too. Basically,
	 * we bring the scc into line with what the flags are
	 */

	r3 = r4 = r5 = 0;

	r4 |= SER_W4_CLKX16;		/* XXX not the case with appletalk */

	if (state&SERCTL_DTR) r5 |= SER_W5_DTR;
	if (state&SERCTL_RTS) r5 |= SER_W5_RTS;
	if (state&SERCTL_BRK) r5 |= SER_W5_BREAK;

	if (tp)  {

		if (tp->t_cflag&CREAD)
			r3 |= SER_W3_ENBRX;

		switch (tp->t_cflag&CSIZE) {
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
				
		if (tp->t_cflag & PARENB)
			r4 |= (tp->t_cflag & PARODD)?SER_W4_PARODD:SER_W4_PAREVEN;
		if (tp->t_cflag & CSTOPB)
			r4 |= SER_W4_2SBIT;
		else
			r4 |= SER_W4_1SBIT;
#ifdef use_hw_flow
		if (tp->t_cflag & CRTSCTS && state&SERCTL_DCD)
			state|=SERCTL_AUTO;
#endif
		if (tp->t_cflag & CRTSCTS && !state&SERCTL_CTS)
			state|=SERCTL_INHIB;
	} else {
		/* we don't have a tty struct */
		r4 |= SER_W4_1SBIT;		/* arbitary default of 8N1 */
		r3 |= SER_W3_RX8DBITS;
		r5 |= SER_W5_TX8DBITS;
	}

	s = splhigh();

	/* next two are fairly volatile */

	if (!(state&SERCTL_INHIB))
		r5 |= SER_W5_ENBTX;
#ifdef use_hw_flow
	if (state&SERCTL_AUTO)
		r3 |= SER_W3_AUTOEN; 
#endif

#ifdef DEBUG
	if (DBG(unit))
		printf("ser%d: apply w3<-%02x w4<-%02x w5<-%02x\n",unit,r3,r4,r5);
	ser_dbg("apply",unit);
#endif
	SER_WRITEREG(unit, 3, r3);
	SER_WRITEREG(unit, 4, r4);
	SER_WRITEREG(unit, 5, r5);
	splx(s);
}
 
static int
serparam(register struct tty *tp, register struct termios *t)
{
	register int cflag = t->c_cflag;
	unsigned char reg3, reg4, reg5;
	int unit = UNIT(tp->t_dev);
	int ospeed = t->c_ospeed;
	int s;

 
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
	if (ospeed == 0) {
		serctl(unit, SERCTL_DTR , DMBIC); /* hang up line */
		return(0);
	}

	serctl(unit, ospeed, SCC_SPEED);

		ser_apply_state( unit );

	/* serctl(unit, 1, SCC_INT); */ /* why? it should already be on */
	serctl(unit, SERCTL_DTR | SERCTL_RTS, DMSET);

	/* End of serial specific param code */

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
	    (ser_status[unit].state & SERCTL_BUSY)) {
			goto out;
		}
	tp->t_state |= TS_BUSY;

	if (ser_outlen[unit] == 0){
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

	while (tp->t_outq.c_cc && space > 0) {
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
		ser_status[unit].state |= SERCTL_BUSY;
		SCCRDWR(unit) = first_char;     /* to start chain */
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

#ifdef DEBUG
	ser_dbg("serstop",UNIT(tp->t_dev));
#endif

	s = spltty();
	if (tp->t_state & TS_BUSY) {
		if ((tp->t_state&TS_TTSTOP)==0)
			tp->t_state |= TS_FLUSH;
	}
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
			ser_status[unit].state =
				(ser_status[unit].state & ~SERCTL_MASK)|
				(bits&SERCTL_MASK);
			ser_apply_state(unit);
			break;

		case DMBIS:
			ser_status[unit].state =
				(ser_status[unit].state)|(bits&SERCTL_MASK);
			ser_apply_state(unit);
			break;

		case DMBIC:
			ser_status[unit].state =
				(ser_status[unit].state)&~(bits&SERCTL_MASK);
			ser_apply_state(unit);
			break;

		case DMGET:
			bits = ser_status[unit].state;
			break;

		case SCC_INT:
#ifdef DEBUG
			if (DBG(unit))
			printf("ser%d: %s interrupts\n",unit,
					bits?"enabling":"disabling");
#endif
			if (bits) {
				/*
				 * Why not a channel reset?
				 * This would mean sending the initialisation string
				 * with a bit of a (tiny) delay after the chan reset
				 * In addition, cts/dcd might change which an
				 * external device plugged in might object to.
				 *
				 * soft-resetting all the flaggy bits should
				 * do the trick
				 */
				SER_WRITEREG(unit, 0, SER_W0_RSTESINTS|
					   SER_W0_RSTTXUNDERRUN);
				SER_WRITEREG(unit, 0, SER_W0_RSTERR);
				SER_WRITEREG(unit, 0, SER_W0_RSTIUS);
				SER_WRITEREG(unit, 1,
					   SER_W1_ENBEXTINT |
				/*either*/ SER_W1_ENBRXINT |
				/*or*/   /*SER_W1_ENBR1INT |*/
					   SER_W1_ENBTXINT |
					   0);
				SER_WRITEREG(unit, 15, 
					   SER_W15_DCDINT|
					   SER_W15_CTSINT|
					   SER_W15_BRKINT|
					   0);
				ser_apply_state(unit); /* turn all the other stuff on */
			} else {
				SER_WRITEREG(unit, 1, 0); /* disables all interrupts */
				SER_WRITEREG(unit, 15, 0);
			}
			break;

		case SCC_SPEED:
			SER_WRITEREG(unit, 12,  SERBRD(bits) & 0xff);
			SER_WRITEREG(unit, 13, (SERBRD(bits) >> 8) & 0xff);
			break;

	}

	(void) splx(s);
	return(bits);
}

/*
 * Console functions.
 */

dev_t   mac68k_serdev;

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

	while (!(SER_STATUS(unit, 0) & SER_R0_RXREADY));
	c = SCCRDWR(unit);
	SER_STATUS(unit, 0) = SER_W0_RSTESINTS;

	return c;
}

sercnputc(dev_t dev, int c)
{
	int unit;

	unit = UNIT(dev);

gray_bar();
	while (!(SER_STATUS(unit, 0) & SER_R0_TXREADY));
	SCCRDWR(unit) = c;
}

/* $NetBSD: scif.c,v 1.10 2000/06/19 09:30:35 msaitoh Exp $ */

/*-
 * Copyright (C) 1999 T.Horiuchi and SAITOH Masanobu.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
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
 * SH internal serial driver
 *
 * This code is derived from both z8530tty.c and com.c
 */

#include "opt_pclock.h"
#include "opt_scif.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/syslog.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <dev/cons.h>

#include <machine/cpu.h>
#include <sh3/scifreg.h>
#include <sh3/tmureg.h>

#include <machine/shbvar.h>

static void	scifstart __P((struct tty *));
static int	scifparam __P((struct tty *, struct termios *));

void scifcnprobe __P((struct consdev *));
void scifcninit __P((struct consdev *));
void scifcnputc __P((dev_t, int));
int scifcngetc __P((dev_t));
void scifcnpoolc __P((dev_t, int));
void scif_intr_init __P((void));
int scifintr __P((void *));

struct scif_softc {
	struct device sc_dev;		/* boilerplate */
	struct tty *sc_tty;
	void *sc_ih;

	struct callout sc_diag_ch;

#if 0
	bus_space_tag_t sc_iot;		/* ISA i/o space identifier */
	bus_space_handle_t   sc_ioh;	/* ISA io handle */

	int sc_drq;

	int sc_frequency;
#endif

	u_int sc_overflows,
	      sc_floods,
	      sc_errors;		/* number of retries so far */
	u_char sc_status[7];		/* copy of registers */

	int sc_hwflags;
	int sc_swflags;
	u_int sc_fifolen;

	u_int sc_r_hiwat,
	      sc_r_lowat;
	u_char *volatile sc_rbget,
	       *volatile sc_rbput;
 	volatile u_int sc_rbavail;
	u_char *sc_rbuf,
	       *sc_ebuf;

 	u_char *sc_tba;			/* transmit buffer address */
 	u_int sc_tbc,			/* transmit byte count */
	      sc_heldtbc;

	volatile u_char sc_rx_flags,
#define	RX_TTY_BLOCKED		0x01
#define	RX_TTY_OVERFLOWED	0x02
#define	RX_IBUF_BLOCKED		0x04
#define	RX_IBUF_OVERFLOWED	0x08
#define	RX_ANY_BLOCK		0x0f
			sc_tx_busy,	/* working on an output chunk */
			sc_tx_done,	/* done with one output chunk */
			sc_tx_stopped,	/* H/W level stop (lost CTS) */
			sc_st_check,	/* got a status interrupt */
			sc_rx_ready;

	volatile u_char sc_heldchange;
};

/* controller driver configuration */
static int scif_match __P((struct device *, struct cfdata *, void *));
static void scif_attach __P((struct device *, struct device *, void *));

void	scif_break	__P((struct scif_softc *, int));
void	scif_iflush	__P((struct scif_softc *));

#define	integrate	static inline
#ifdef __GENERIC_SOFT_INTERRUPTS
void 	scifsoft	__P((void *));
#else
#ifndef __NO_SOFT_SERIAL_INTERRUPT
void 	scifsoft	__P((void));
#else
void 	scifsoft	__P((void *));
#endif
#endif
integrate void scif_rxsoft	__P((struct scif_softc *, struct tty *));
integrate void scif_txsoft	__P((struct scif_softc *, struct tty *));
integrate void scif_stsoft	__P((struct scif_softc *, struct tty *));
integrate void scif_schedrx	__P((struct scif_softc *));
void	scifdiag		__P((void *));


#define	SCIFUNIT_MASK		0x7ffff
#define	SCIFDIALOUT_MASK	0x80000

#define	SCIFUNIT(x)	(minor(x) & SCIFUNIT_MASK)
#define	SCIFDIALOUT(x)	(minor(x) & SCIFDIALOUT_MASK)

/* Macros to clear/set/test flags. */
#define SET(t, f)	(t) |= (f)
#define CLR(t, f)	(t) &= ~(f)
#define ISSET(t, f)	((t) & (f))

/* Hardware flag masks */
#define	SCIF_HW_NOIEN	0x01
#define	SCIF_HW_FIFO	0x02
#define	SCIF_HW_FLOW	0x08
#define	SCIF_HW_DEV_OK	0x20
#define	SCIF_HW_CONSOLE	0x40
#define	SCIF_HW_KGDB	0x80

/* Buffer size for character buffer */
#define	SCIF_RING_SIZE	2048

/* Stop input when 3/4 of the ring is full; restart when only 1/4 is full. */
u_int scif_rbuf_hiwat = (SCIF_RING_SIZE * 1) / 4;
u_int scif_rbuf_lowat = (SCIF_RING_SIZE * 3) / 4;

#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
int scifconscflag = CONMODE;
int scifisconsole = 0;

#ifdef SCIFCN_SPEED
unsigned int scifcn_speed = SCIFCN_SPEED;
#else
unsigned int scifcn_speed = 9600;
#endif

#define	divrnd(n, q)	(((n)*2/(q)+1)/2)	/* divide and round off */

#ifndef __GENERIC_SOFT_INTERRUPTS
#ifdef __NO_SOFT_SERIAL_INTERRUPT
volatile int	scif_softintr_scheduled;
struct callout scif_soft_ch = CALLOUT_INITIALIZER;
#endif
#endif

u_int scif_rbuf_size = SCIF_RING_SIZE;

struct cfattach scif_ca = {
	sizeof(struct scif_softc), scif_match, scif_attach
};

extern struct cfdriver scif_cd;

cdev_decl(scif);

void InitializeScif  __P((unsigned int));

/*
 * following functions are debugging prupose only
 */
#define CR      0x0D
#define USART_ON (unsigned int)~0x08

static void WaitFor __P((int));
void PutcScif __P((unsigned char));
void PutStrScif __P((unsigned char *));
int ScifErrCheck __P((void));
unsigned char GetcScif __P((void));
int GetStrScif __P((unsigned char *, int));

/*
 * WaitFor
 * : int mSec;
 */
static void
WaitFor(mSec)
	int mSec;
{

	/* Disable Under Flow interrupt, rising edge, 1/4 */
	SHREG_TCR2 = 0x0000;

	/* Set counter value (count down with 4 KHz) */
	SHREG_TCNT2 = mSec * 4;

	/* start Channel2 */
	SHREG_TSTR |= TSTR_STR2;

	/* wait for under flag ON of channel2 */
	while ((SHREG_TCR2 & TCR_UNF) == 0)
		;

	/* stop channel2 */
	SHREG_TSTR &= ~TSTR_STR2;
}

/*
 * InitializeScif
 * : unsigned int bps;
 * : SCIF(Serial Communication Interface)
 */

void
InitializeScif(bps)
	unsigned int bps;
{

	/* Initialize SCR */
	SHREG_SCSCR2 = 0x00;

#if 0
	SHREG_SCFCR2 = SCFCR2_TFRST | SCFCR2_RFRST | SCFCR2_MCE;
#else
	SHREG_SCFCR2 = SCFCR2_TFRST | SCFCR2_RFRST;
#endif
	/* Serial Mode Register */
	SHREG_SCSMR2 = 0x00;	/* 8bit,NonParity,Even,1Stop */

	/* Bit Rate Register */
	SHREG_SCBRR2 = divrnd(PCLOCK, 32 * bps) - 1;

	/*
	 * wait 1mSec, because Send/Recv must begin 1 bit period after
	 * BRR is set.
	 */
	WaitFor(1);

#if 0
	SHREG_SCFCR2 = FIFO_RCV_TRIGGER_14 | FIFO_XMT_TRIGGER_1 | SCFCR2_MCE;
#else
	SHREG_SCFCR2 = FIFO_RCV_TRIGGER_14 | FIFO_XMT_TRIGGER_1;
#endif

	/* Send permission, Recieve permission ON */
	SHREG_SCSCR2 = SCSCR2_TE | SCSCR2_RE;

	/* Serial Status Register */
	SHREG_SCSSR2 &= SCSSR2_TDFE;	/* Clear Status */
}


/*
 * PutcScif
 *  : unsigned char c;
 */

void
PutcScif(c)
	unsigned char c;
{

	/* wait for ready */
	while ((SHREG_SCFDR2 & SCFDR2_TXCNT) == SCFDR2_TXF_FULL)
		;

	/* write send data to send register */
	SHREG_SCFTDR2 = c;

	/* clear ready flag */
	SHREG_SCSSR2 &= ~(SCSSR2_TDFE | SCSSR2_TEND);

	if (c == '\n') {
		while ((SHREG_SCFDR2 & SCFDR2_TXCNT) == SCFDR2_TXF_FULL)
			;

		SHREG_SCFTDR2 = '\r';

		SHREG_SCSSR2 &= ~(SCSSR2_TDFE | SCSSR2_TEND);
	}
}

/*
 * PutStrScif
 * : unsigned char *s;
 */
void
PutStrScif(s)
	unsigned char *s;
{

	while (*s)
		PutcScif(*s++);
}

/*
 * : ScifErrCheck
 *	0x80 = error
 *	0x08 = frame error
 *	0x04 = parity error
 */
int
ScifErrCheck(void)
{

	return(SHREG_SCSSR2 & (SCSSR2_ER | SCSSR2_FER | SCSSR2_PER));
}

/*
 * GetcScif
 */
#if 0
/* Old code */
unsigned char
GetcScif(void)
{
	unsigned char c, err_c;

	while (((err_c = SHREG_SCSSR2)
		& (SCSSR2_RDF | SCSSR2_ER | SCSSR2_FER | SCSSR2_PER | SCSSR2_DR)) == 0)
		;
	if ((err_c & (SCSSR2_ER | SCSSR2_FER | SCSSR2_PER)) != 0) {
		SHREG_SCSSR2 &= ~SCSSR2_ER;
		return(err_c |= 0x80);
	}

	c = SHREG_SCFRDR2;

	SHREG_SCSSR2 &= ~(SCSSR2_ER | SCSSR2_RDF | SCSSR2_DR);

	return(c);
}
#else
unsigned char
GetcScif(void)
{
	unsigned char c, err_c;

	/* wait for ready */
	while ((SHREG_SCFDR2 & SCFDR2_RECVCNT) == 0)
		;
	err_c = SHREG_SCSSR2;
	if ((err_c & (SCSSR2_ER | SCSSR2_BRK | SCSSR2_FER | SCSSR2_PER)) != 0)
		return(err_c |= 0x80);

	c = SHREG_SCFRDR2;

	SHREG_SCSSR2 &= ~(SCSSR2_ER | SCSSR2_BRK | SCSSR2_RDF | SCSSR2_DR);

	return(c);
}
#endif

/*
 * GetStrScif
 *  : unsigned char *s;
 *  : int size;
 */
int
GetStrScif(s, size)
	unsigned char *s;
	int size;
{

	for (; size ; size--) {
		*s = GetcScif();
		if (*s & 0x80)
			return -1;
		if (*s == CR) {
			*s = 0;
			break;
		}
		s++;
	}
	if (size == 0)
		*s = 0;
	return 0;
}

#if 0
#define SCIF_MAX_UNITS 2
#else
#define SCIF_MAX_UNITS 1
#endif


static int
scif_match(parent, cfp, aux)
	struct device *parent;
	struct cfdata *cfp;
	void *aux;
{
	struct shb_attach_args *sa = aux;

	if (strcmp(cfp->cf_driver->cd_name, "scif")
	    || cfp->cf_unit >= SCIF_MAX_UNITS)
		return 0;

	sa->ia_iosize = 0x10;
	return 1;
}

static void
scif_attach(parent, self, aux)
	struct device	*parent, *self;
	void		*aux;
{
	struct scif_softc *sc = (struct scif_softc *)self;
	struct tty *tp;
	int irq;
	struct shb_attach_args *ia = aux;

	sc->sc_hwflags = 0;	/* XXX */
	sc->sc_swflags = 0;	/* XXX */
	sc->sc_fifolen = 16;

	irq = ia->ia_irq;

	if (scifisconsole) {
		/* InitializeScif(scifcn_speed); */
		SET(sc->sc_hwflags, SCIF_HW_CONSOLE);
		SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);
		printf("\n%s: console\n", sc->sc_dev.dv_xname);
	} else {
		InitializeScif(9600);
		printf("\n");
	}

	callout_init(&sc->sc_diag_ch);

#if 0
	if (irq != IRQUNK) {
		sc->sc_ih = shb_intr_establish(irq,
		    IST_EDGE, IPL_SERIAL, scifintr, sc);
	}
#else
	if (irq != IRQUNK) {
		sc->sc_ih = shb_intr_establish(SCIF_IRQ,
		    IST_EDGE, IPL_SERIAL, scifintr, sc);
	}
#endif

	SET(sc->sc_hwflags, SCIF_HW_DEV_OK);

	tp = ttymalloc();
	tp->t_oproc = scifstart;
	tp->t_param = scifparam;
	tp->t_hwiflow = NULL;

	sc->sc_tty = tp;
	sc->sc_rbuf = malloc(scif_rbuf_size << 1, M_DEVBUF, M_NOWAIT);
	if (sc->sc_rbuf == NULL) {
		printf("%s: unable to allocate ring buffer\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	sc->sc_ebuf = sc->sc_rbuf + (scif_rbuf_size << 1);

	tty_attach(tp);
}

/*
 * Start or restart transmission.
 */
static void
scifstart(tp)
	struct tty *tp;
{
	struct scif_softc *sc = scif_cd.cd_devs[SCIFUNIT(tp->t_dev)];
	int s;

	s = spltty();
	if (ISSET(tp->t_state, TS_BUSY | TS_TIMEOUT | TS_TTSTOP))
		goto out;
	if (sc->sc_tx_stopped)
		goto out;

	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (ISSET(tp->t_state, TS_ASLEEP)) {
			CLR(tp->t_state, TS_ASLEEP);
			wakeup(&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
		if (tp->t_outq.c_cc == 0)
			goto out;
	}

	/* Grab the first contiguous region of buffer space. */
	{
		u_char *tba;
		int tbc;

		tba = tp->t_outq.c_cf;
		tbc = ndqb(&tp->t_outq, 0);

		(void)splserial();

		sc->sc_tba = tba;
		sc->sc_tbc = tbc;
	}

	SET(tp->t_state, TS_BUSY);
	sc->sc_tx_busy = 1;

	/* Enable transmit completion interrupts if necessary. */
	SHREG_SCSCR2 |= SCSCR2_TIE | SCSCR2_RIE;

	/* Output the first chunk of the contiguous buffer. */
	{
		int n;
		int max;
		int i;

		n = sc->sc_tbc;
		max = sc->sc_fifolen - ((SHREG_SCFDR2 & SCFDR2_TXCNT) >> 8);
		if (n > max)
			n = max;

		for (i = 0; i < n; i++) {
			PutcScif(*(sc->sc_tba));
			sc->sc_tba++;
		}
		sc->sc_tbc -= n;
	}
out:
	splx(s);
	return;
}

/*
 * Set SCIF tty parameters from termios.
 * XXX - Should just copy the whole termios after
 * making sure all the changes could be done.
 */
static int
scifparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
	struct scif_softc *sc = scif_cd.cd_devs[SCIFUNIT(tp->t_dev)];
	int ospeed = t->c_ospeed;
	int s;

	if (ISSET(sc->sc_dev.dv_flags, DVF_ACTIVE) == 0)
		return (EIO);

	/* Check requested parameters. */
	if (ospeed < 0)
		return (EINVAL);
	if (t->c_ispeed && t->c_ispeed != t->c_ospeed)
		return (EINVAL);

	/*
	 * For the console, always force CLOCAL and !HUPCL, so that the port
	 * is always active.
	 */
	if (ISSET(sc->sc_swflags, TIOCFLAG_SOFTCAR) ||
	    ISSET(sc->sc_hwflags, SCIF_HW_CONSOLE)) {
		SET(t->c_cflag, CLOCAL);
		CLR(t->c_cflag, HUPCL);
	}

	/*
	 * If there were no changes, don't do anything.  This avoids dropping
	 * input and improves performance when all we did was frob things like
	 * VMIN and VTIME.
	 */
	if (tp->t_ospeed == t->c_ospeed &&
	    tp->t_cflag == t->c_cflag)
		return (0);

#if 0
/* XXX (msaitoh) */
	lcr = ISSET(sc->sc_lcr, LCR_SBREAK) | cflag2lcr(t->c_cflag);
#endif

	s = splserial();

	/*
	 * Set the flow control pins depending on the current flow control
	 * mode.
	 */
	if (ISSET(t->c_cflag, CRTSCTS)) {
		SHREG_SCFCR2 |= SCFCR2_MCE;
	} else {
		SHREG_SCFCR2 &= ~SCFCR2_MCE;
	}

	SHREG_SCBRR2 = divrnd(PCLOCK, 32 * ospeed) -1;

	/*
	 * Set the FIFO threshold based on the receive speed.
	 *
	 *  * If it's a low speed, it's probably a mouse or some other
	 *    interactive device, so set the threshold low.
	 *  * If it's a high speed, trim the trigger level down to prevent
	 *    overflows.
	 *  * Otherwise set it a bit higher.
	 */
#if 0
/* XXX (msaitoh) */
	if (ISSET(sc->sc_hwflags, SCIF_HW_HAYESP))
		sc->sc_fifo = FIFO_DMA_MODE | FIFO_ENABLE | FIFO_TRIGGER_8;
	else if (ISSET(sc->sc_hwflags, SCIF_HW_FIFO))
		sc->sc_fifo = FIFO_ENABLE |
		    (t->c_ospeed <= 1200 ? FIFO_TRIGGER_1 :
		     t->c_ospeed <= 38400 ? FIFO_TRIGGER_8 : FIFO_TRIGGER_4);
	else
		sc->sc_fifo = 0;
#endif

	/* And copy to tty. */
	tp->t_ispeed = 0;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		}
#if 0
/* XXX (msaitoh) */
		else
			scif_loadchannelregs(sc);
#endif
	}

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		/* Disable the high water mark. */
		sc->sc_r_hiwat = 0;
		sc->sc_r_lowat = 0;
		if (ISSET(sc->sc_rx_flags, RX_TTY_OVERFLOWED)) {
			CLR(sc->sc_rx_flags, RX_TTY_OVERFLOWED);
			scif_schedrx(sc);
		}
	} else {
		sc->sc_r_hiwat = scif_rbuf_hiwat;
		sc->sc_r_lowat = scif_rbuf_lowat;
	}

	splx(s);

#ifdef SCIF_DEBUG
	if (scif_debug)
		scifstatus(sc, "scifparam ");
#endif

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		if (sc->sc_tx_stopped) {
			sc->sc_tx_stopped = 0;
			scifstart(tp);
		}
	}

	return (0);
}

void
scif_iflush(sc)
	struct scif_softc *sc;
{
	int i;
	unsigned char c;

	i = SHREG_SCFDR2 & SCFDR2_RECVCNT;

	while (i > 0) {
		c = SHREG_SCFRDR2;
		SHREG_SCSSR2 &= ~(SCSSR2_RDF | SCSSR2_DR);
		i--;
	}
}

int scif_getc __P((void));
void scif_putc __P((int));

int
scif_getc()
{

	return (GetcScif());
}

void
scif_putc(int c)
{

	PutcScif(c);
}

int
scifopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int unit = SCIFUNIT(dev);
	struct scif_softc *sc;
	struct tty *tp;
	int s, s2;
	int error;

	if (unit >= scif_cd.cd_ndevs)
		return (ENXIO);
	sc = scif_cd.cd_devs[unit];
	if (sc == 0 || !ISSET(sc->sc_hwflags, SCIF_HW_DEV_OK) ||
	    sc->sc_rbuf == NULL)
		return (ENXIO);

	if (ISSET(sc->sc_dev.dv_flags, DVF_ACTIVE) == 0)
		return (ENXIO);

#ifdef KGDB
	/*
	 * If this is the kgdb port, no other use is permitted.
	 */
	if (ISSET(sc->sc_hwflags, SCIF_HW_KGDB))
		return (EBUSY);
#endif

	tp = sc->sc_tty;

	if (ISSET(tp->t_state, TS_ISOPEN) &&
	    ISSET(tp->t_state, TS_XCLUDE) &&
	    p->p_ucred->cr_uid != 0)
		return (EBUSY);

	s = spltty();

	/*
	 * Do the following iff this is a first open.
	 */
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		struct termios t;

		tp->t_dev = dev;

		s2 = splserial();

		/* Turn on interrupts. */
		SHREG_SCSCR2 |= SCSCR2_TIE | SCSCR2_RIE;

		splx(s2);

		/*
		 * Initialize the termios status to the defaults.  Add in the
		 * sticky bits from TIOCSFLAGS.
		 */
		t.c_ispeed = 0;
		if (ISSET(sc->sc_hwflags, SCIF_HW_CONSOLE)) {
			t.c_ospeed = scifcn_speed;	/* XXX (msaitoh) */
			t.c_cflag = scifconscflag;
		} else {
			t.c_ospeed = TTYDEF_SPEED;
			t.c_cflag = TTYDEF_CFLAG;
		}
		if (ISSET(sc->sc_swflags, TIOCFLAG_CLOCAL))
			SET(t.c_cflag, CLOCAL);
		if (ISSET(sc->sc_swflags, TIOCFLAG_CRTSCTS))
			SET(t.c_cflag, CRTSCTS);
		if (ISSET(sc->sc_swflags, TIOCFLAG_MDMBUF))
			SET(t.c_cflag, MDMBUF);
		/* Make sure scifparam() will do something. */
		tp->t_ospeed = 0;
		(void) scifparam(tp, &t);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		ttychars(tp);
		ttsetwater(tp);

		s2 = splserial();

		/* Clear the input ring, and unblock. */
		sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
		sc->sc_rbavail = scif_rbuf_size;
		scif_iflush(sc);
		CLR(sc->sc_rx_flags, RX_ANY_BLOCK);
#if 0
/* XXX (msaitoh) */
		scif_hwiflow(sc);
#endif

#ifdef SCIF_DEBUG
		if (scif_debug)
			scifstatus(sc, "scifopen  ");
#endif

		splx(s2);
	}

	splx(s);

	error = ttyopen(tp, SCIFDIALOUT(dev), ISSET(flag, O_NONBLOCK));
	if (error)
		goto bad;

	error = (*linesw[tp->t_line].l_open)(dev, tp);
	if (error)
		goto bad;

	return (0);

bad:

	return (error);
}

int
scifclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct scif_softc *sc = scif_cd.cd_devs[SCIFUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	/* XXX This is for cons.c. */
	if (!ISSET(tp->t_state, TS_ISOPEN))
		return (0);

	(*linesw[tp->t_line].l_close)(tp, flag);
	ttyclose(tp);

	if (ISSET(sc->sc_dev.dv_flags, DVF_ACTIVE) == 0)
		return (0);

	return (0);
}

int
scifread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct scif_softc *sc = scif_cd.cd_devs[SCIFUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

int
scifwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct scif_softc *sc = scif_cd.cd_devs[SCIFUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

struct tty *
sciftty(dev)
	dev_t dev;
{
	struct scif_softc *sc = scif_cd.cd_devs[SCIFUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return (tp);
}

int
scifioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct scif_softc *sc = scif_cd.cd_devs[SCIFUNIT(dev)];
	struct tty *tp = sc->sc_tty;
	int error;
	int s;

	if (ISSET(sc->sc_dev.dv_flags, DVF_ACTIVE) == 0)
		return (EIO);

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);

	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);

	error = 0;

	s = splserial();

	switch (cmd) {
	case TIOCSBRK:
		scif_break(sc, 1);
		break;

	case TIOCCBRK:
		scif_break(sc, 0);
		break;

	case TIOCGFLAGS:
		*(int *)data = sc->sc_swflags;
		break;

	case TIOCSFLAGS:
		error = suser(p->p_ucred, &p->p_acflag);
		if (error)
			break;
		sc->sc_swflags = *(int *)data;
		break;

	default:
		error = ENOTTY;
		break;
	}

	splx(s);

	return (error);
}

integrate void
scif_schedrx(sc)
	struct scif_softc *sc;
{

	sc->sc_rx_ready = 1;

	/* Wake up the poller. */
#ifdef __GENERIC_SOFT_INTERRUPTS
	softintr_schedule(sc->sc_si);
#else
#ifndef __NO_SOFT_SERIAL_INTERRUPT
	setsoftserial();
#else
	if (!scif_softintr_scheduled) {
		scif_softintr_scheduled = 1;
		callout_reset(&scif_soft_ch, 1, scifsoft, NULL);
	}
#endif
#endif
}

void
scif_break(sc, onoff)
	struct scif_softc *sc;
	int onoff;
{

	if (onoff)
		SHREG_SCSSR2 &= ~SCSSR2_TDFE;
	else
		SHREG_SCSSR2 |= SCSSR2_TDFE;

#if 0	/* XXX */
	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			scif_loadchannelregs(sc);
	}
#endif
}

/*
 * Stop output, e.g., for ^S or output flush.
 */
void
scifstop(tp, flag)
	struct tty *tp;
	int flag;
{
	struct scif_softc *sc = scif_cd.cd_devs[SCIFUNIT(tp->t_dev)];
	int s;

	s = splserial();
	if (ISSET(tp->t_state, TS_BUSY)) {
		/* Stop transmitting at the next chunk. */
		sc->sc_tbc = 0;
		sc->sc_heldtbc = 0;
		if (!ISSET(tp->t_state, TS_TTSTOP))
			SET(tp->t_state, TS_FLUSH);
	}
	splx(s);
}

void
scif_intr_init()
{
	/* XXX */
}

void
scifdiag(arg)
	void *arg;
{
	struct scif_softc *sc = arg;
	int overflows, floods;
	int s;

	s = splserial();
	overflows = sc->sc_overflows;
	sc->sc_overflows = 0;
	floods = sc->sc_floods;
	sc->sc_floods = 0;
	sc->sc_errors = 0;
	splx(s);

	log(LOG_WARNING, "%s: %d silo overflow%s, %d ibuf flood%s\n",
	    sc->sc_dev.dv_xname,
	    overflows, overflows == 1 ? "" : "s",
	    floods, floods == 1 ? "" : "s");
}

integrate void
scif_rxsoft(sc, tp)
	struct scif_softc *sc;
	struct tty *tp;
{
	int (*rint) __P((int c, struct tty *tp)) = linesw[tp->t_line].l_rint;
	u_char *get, *end;
	u_int cc, scc;
	u_char ssr2;
	int code;
	int s;

	end = sc->sc_ebuf;
	get = sc->sc_rbget;
	scc = cc = scif_rbuf_size - sc->sc_rbavail;

	if (cc == scif_rbuf_size) {
		sc->sc_floods++;
		if (sc->sc_errors++ == 0)
			callout(&sc->sc_diag_ch, 60 * hz, scifdiag, sc);
	}

	while (cc) {
		code = get[0];
		ssr2 = get[1];
		if (ISSET(ssr2, SCSSR2_BRK | SCSSR2_FER | SCSSR2_PER)) {
			if (ISSET(ssr2, SCSSR2_BRK | SCSSR2_FER))
				SET(code, TTY_FE);
			if (ISSET(ssr2, SCSSR2_PER))
				SET(code, TTY_PE);
		}
		if ((*rint)(code, tp) == -1) {
			/*
			 * The line discipline's buffer is out of space.
			 */
			if (!ISSET(sc->sc_rx_flags, RX_TTY_BLOCKED)) {
				/*
				 * We're either not using flow control, or the
				 * line discipline didn't tell us to block for
				 * some reason.  Either way, we have no way to
				 * know when there's more space available, so
				 * just drop the rest of the data.
				 */
				get += cc << 1;
				if (get >= end)
					get -= scif_rbuf_size << 1;
				cc = 0;
			} else {
				/*
				 * Don't schedule any more receive processing
				 * until the line discipline tells us there's
				 * space available (through scifhwiflow()).
				 * Leave the rest of the data in the input
				 * buffer.
				 */
				SET(sc->sc_rx_flags, RX_TTY_OVERFLOWED);
			}
			break;
		}
		get += 2;
		if (get >= end)
			get = sc->sc_rbuf;
		cc--;
	}

	if (cc != scc) {
		sc->sc_rbget = get;
		s = splserial();
		cc = sc->sc_rbavail += scc - cc;
		/* Buffers should be ok again, release possible block. */
		if (cc >= sc->sc_r_lowat) {
			if (ISSET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
				SHREG_SCSCR2 |= SCSCR2_RIE;
			}
#if 0
			if (ISSET(sc->sc_rx_flags, RX_IBUF_BLOCKED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_BLOCKED);
				scif_hwiflow(sc);
			}
#endif
		}
		splx(s);
	}
}

integrate void
scif_txsoft(sc, tp)
	struct scif_softc *sc;
	struct tty *tp;
{

	CLR(tp->t_state, TS_BUSY);
	if (ISSET(tp->t_state, TS_FLUSH))
		CLR(tp->t_state, TS_FLUSH);
	else
		ndflush(&tp->t_outq, (int)(sc->sc_tba - tp->t_outq.c_cf));
	(*linesw[tp->t_line].l_start)(tp);
}

integrate void
scif_stsoft(sc, tp)
	struct scif_softc *sc;
	struct tty *tp;
{
#if 0
/* XXX (msaitoh) */
	u_char msr, delta;
	int s;

	s = splserial();
	msr = sc->sc_msr;
	delta = sc->sc_msr_delta;
	sc->sc_msr_delta = 0;
	splx(s);

	if (ISSET(delta, sc->sc_msr_dcd)) {
		/*
		 * Inform the tty layer that carrier detect changed.
		 */
		(void) (*linesw[tp->t_line].l_modem)(tp, ISSET(msr, MSR_DCD));
	}

	if (ISSET(delta, sc->sc_msr_cts)) {
		/* Block or unblock output according to flow control. */
		if (ISSET(msr, sc->sc_msr_cts)) {
			sc->sc_tx_stopped = 0;
			(*linesw[tp->t_line].l_start)(tp);
		} else {
			sc->sc_tx_stopped = 1;
		}
	}

#ifdef SCIF_DEBUG
	if (scif_debug)
		scifstatus(sc, "scif_stsoft");
#endif
#endif
}

#ifdef __GENERIC_SOFT_INTERRUPTS
void
scifsoft(arg)
	void *arg;
{
	struct scif_softc *sc = arg;
	struct tty *tp;

	if (ISSET(sc->sc_dev.dv_flags, DVF_ACTIVE) == 0)
		return;

	{
#else
void
#ifndef __NO_SOFT_SERIAL_INTERRUPT
scifsoft()
#else
scifsoft(arg)
	void *arg;
#endif
{
	struct scif_softc	*sc;
	struct tty	*tp;
	int	unit;
#ifdef __NO_SOFT_SERIAL_INTERRUPT
	int s;

	s = splsoftserial();
	scif_softintr_scheduled = 0;
#endif

	for (unit = 0; unit < scif_cd.cd_ndevs; unit++) {
		sc = scif_cd.cd_devs[unit];
		if (sc == NULL || !ISSET(sc->sc_hwflags, SCIF_HW_DEV_OK))
			continue;

		if (ISSET(sc->sc_dev.dv_flags, DVF_ACTIVE) == 0)
			continue;

		tp = sc->sc_tty;
		if (tp == NULL)
			continue;
		if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0)
			continue;
#endif
		tp = sc->sc_tty;

		if (sc->sc_rx_ready) {
			sc->sc_rx_ready = 0;
			scif_rxsoft(sc, tp);
		}

#if 0
		if (sc->sc_st_check) {
			sc->sc_st_check = 0;
			scif_stsoft(sc, tp);
		}
#endif

		if (sc->sc_tx_done) {
			sc->sc_tx_done = 0;
			scif_txsoft(sc, tp);
		}
	}

#ifndef __GENERIC_SOFT_INTERRUPTS
#ifdef __NO_SOFT_SERIAL_INTERRUPT
	splx(s);
#endif
#endif
}

int
scifintr(arg)
	void *arg;
{
	struct scif_softc *sc = arg;
	u_char *put, *end;
	u_int cc;
	u_short ssr2;
	int count;

	if (ISSET(sc->sc_dev.dv_flags, DVF_ACTIVE) == 0)
		return (0);

	end = sc->sc_ebuf;
	put = sc->sc_rbput;
	cc = sc->sc_rbavail;

	ssr2 = SHREG_SCSSR2;
	if (ISSET(ssr2, SCSSR2_BRK)) {
		SHREG_SCSSR2 &= ~(SCSSR2_ER | SCSSR2_BRK | SCSSR2_DR);
#if defined(DDB) || defined(KGDB)
#ifdef DDB
		if (ISSET(sc->sc_hwflags, SCIF_HW_CONSOLE)) {
			console_debugger();
		}
#endif
#ifdef KGDB
		if (ISSET(sc->sc_hwflags, SCIF_HW_KGDB)) {
			kgdb_connect(1);
		}
#endif
#endif /* DDB || KGDB */
	}
	count = SHREG_SCFDR2 & SCFDR2_RECVCNT;
	if (count != 0) {
		while ((cc > 0) && (count > 0)) {
			put[0] = SHREG_SCFRDR2;
			put[1] = (u_char)(SHREG_SCSSR2 & 0x00ff);

			SHREG_SCSSR2 &= ~(SCSSR2_ER | SCSSR2_RDF | SCSSR2_DR);

			put += 2;
			if (put >= end)
				put = sc->sc_rbuf;
			cc--;
			count--;
		}

		/*
		 * Current string of incoming characters ended because
		 * no more data was available or we ran out of space.
		 * Schedule a receive event if any data was received.
		 * If we're out of space, turn off receive interrupts.
		 */
		sc->sc_rbput = put;
		sc->sc_rbavail = cc;
		if (!ISSET(sc->sc_rx_flags, RX_TTY_OVERFLOWED))
			sc->sc_rx_ready = 1;

		/*
		 * See if we are in danger of overflowing a buffer. If
		 * so, use hardware flow control to ease the pressure.
		 */
		if (!ISSET(sc->sc_rx_flags, RX_IBUF_BLOCKED) &&
		    cc < sc->sc_r_hiwat) {
			SET(sc->sc_rx_flags, RX_IBUF_BLOCKED);
#if 0
			scif_hwiflow(sc);
#endif
		}

		/*
		 * If we're out of space, disable receive interrupts
		 * until the queue has drained a bit.
		 */
		if (!cc) {
			SHREG_SCSCR2 &= ~SCSCR2_RIE;
		}
	} else {
		if (SHREG_SCSSR2 & (SCSSR2_RDF | SCSSR2_DR)) {
			SHREG_SCSCR2 &= ~(SCSCR2_TIE | SCSCR2_RIE);
		}
	}

#if 0
	msr = bus_space_read_1(iot, ioh, scif_msr);
	delta = msr ^ sc->sc_msr;
	sc->sc_msr = msr;
	if (ISSET(delta, sc->sc_msr_mask)) {
		SET(sc->sc_msr_delta, delta);

		/*
		 * Pulse-per-second clock signal on edge of DCD?
		 */
		if (ISSET(delta, sc->sc_ppsmask)) {
			struct timeval tv;
			if (ISSET(msr, sc->sc_ppsmask) ==
			    sc->sc_ppsassert) {
				/* XXX nanotime() */
				microtime(&tv);
				TIMEVAL_TO_TIMESPEC(&tv,
						    &sc->ppsinfo.assert_timestamp);
				if (sc->ppsparam.mode & PPS_OFFSETASSERT) {
					timespecadd(&sc->ppsinfo.assert_timestamp,
						    &sc->ppsparam.assert_offset,
						    &sc->ppsinfo.assert_timestamp);
					TIMESPEC_TO_TIMEVAL(&tv, &sc->ppsinfo.assert_timestamp);
				}

#ifdef PPS_SYNC
				if (sc->ppsparam.mode & PPS_HARDPPSONASSERT)
					hardpps(&tv, tv.tv_usec);
#endif
				sc->ppsinfo.assert_sequence++;
				sc->ppsinfo.current_mode =
					sc->ppsparam.mode;

			} else if (ISSET(msr, sc->sc_ppsmask) ==
				   sc->sc_ppsclear) {
				/* XXX nanotime() */
				microtime(&tv);
				TIMEVAL_TO_TIMESPEC(&tv,
						    &sc->ppsinfo.clear_timestamp);
				if (sc->ppsparam.mode & PPS_OFFSETCLEAR) {
					timespecadd(&sc->ppsinfo.clear_timestamp,
						    &sc->ppsparam.clear_offset,
						    &sc->ppsinfo.clear_timestamp);
					TIMESPEC_TO_TIMEVAL(&tv, &sc->ppsinfo.clear_timestamp);
				}

#ifdef PPS_SYNC
				if (sc->ppsparam.mode & PPS_HARDPPSONCLEAR)
					hardpps(&tv, tv.tv_usec);
#endif
				sc->ppsinfo.clear_sequence++;
				sc->ppsinfo.current_mode =
					sc->ppsparam.mode;
			}
		}

		/*
		 * Stop output immediately if we lose the output
		 * flow control signal or carrier detect.
		 */
		if (ISSET(~msr, sc->sc_msr_mask)) {
			sc->sc_tbc = 0;
			sc->sc_heldtbc = 0;
#ifdef SCIF_DEBUG
			if (scif_debug)
				scifstatus(sc, "scifintr  ");
#endif
		}

		sc->sc_st_check = 1;
	}
#endif

	/*
	 * Done handling any receive interrupts. See if data can be
	 * transmitted as well. Schedule tx done event if no data left
	 * and tty was marked busy.
	 */
	if (((SHREG_SCFDR2 & SCFDR2_TXCNT) >> 8) != 16) { /* XXX (msaitoh) */
		/*
		 * If we've delayed a parameter change, do it now, and restart
		 * output.
		 */
		if (sc->sc_heldchange) {
			sc->sc_heldchange = 0;
			sc->sc_tbc = sc->sc_heldtbc;
			sc->sc_heldtbc = 0;
		}

		/* Output the next chunk of the contiguous buffer, if any. */
		if (sc->sc_tbc > 0) {
			int n;
			int max;
			int i;

			n = sc->sc_tbc;
			max = sc->sc_fifolen -
				((SHREG_SCFDR2 & SCFDR2_TXCNT) >> 8);
			if (n > max)
				n = max;

			for (i = 0; i < n; i++) {
				PutcScif(*(sc->sc_tba));
				sc->sc_tba++;
			}
			sc->sc_tbc -= n;
		} else {
			/* Disable transmit completion interrupts if necessary. */
#if 0
			if (ISSET(sc->sc_ier, IER_ETXRDY))
#endif
				SHREG_SCSCR2 &= ~SCSCR2_TIE;

			if (sc->sc_tx_busy) {
				sc->sc_tx_busy = 0;
				sc->sc_tx_done = 1;
			}
		}
	}

	/* Wake up the poller. */
#ifdef __GENERIC_SOFT_INTERRUPTS
	softintr_schedule(sc->sc_si);
#else
#ifndef __NO_SOFT_SERIAL_INTERRUPT
	setsoftserial();
#else
	if (!scif_softintr_scheduled) {
		scif_softintr_scheduled = 1;
		callout_reset(&scif_soft_ch, 1, scifsoft, NULL);
	}
#endif
#endif

#if NRND > 0 && defined(RND_SCIF)
	rnd_add_uint32(&sc->rnd_source, iir | lsr);
#endif

	return (1);
}

void
scifcnprobe(cp)
	struct consdev *cp;
{
	int maj;

	/* locate the major number */
	for (maj = 0; maj < nchrdev; maj++)
		if (cdevsw[maj].d_open == scifopen)
			break;

	/* Initialize required fields. */
	cp->cn_dev = makedev(maj, 0);
#ifdef SCIFCONSOLE
	cp->cn_pri = CN_REMOTE;
#else
	cp->cn_pri = CN_NORMAL;
#endif
}

#define scif_gets GetStrScif
#define scif_puts PutStrScif

void
scifcninit(cp)
	struct consdev *cp;
{

	InitializeScif(scifcn_speed);
	scifisconsole = 1;
}

#define scif_getc GetcScif
#define scif_putc PutcScif

int
scifcngetc(dev)
	dev_t dev;
{
	int c;
	int s;

	s = splserial();
	c = scif_getc();
	splx(s);

	return (c);
}

void
scifcnputc(dev, c)
	dev_t dev;
	int c;
{
	int s;

	s = splserial();
	scif_putc(c);
	splx(s);
}

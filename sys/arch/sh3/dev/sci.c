/* $NetBSD: sci.c,v 1.23 2002/04/26 10:22:54 msaitoh Exp $ */

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

#include "opt_kgdb.h"
#include "opt_sci.h"

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

#include <sh3/clock.h>
#include <sh3/scireg.h>
#include <sh3/pfcreg.h>
#include <sh3/tmureg.h>
#include <sh3/exception.h>
#include <machine/intr.h>

static void	scistart(struct tty *);
static int	sciparam(struct tty *, struct termios *);

void scicnprobe(struct consdev *);
void scicninit(struct consdev *);
void scicnputc(dev_t, int);
int scicngetc(dev_t);
void scicnpoolc(dev_t, int);
int sciintr(void *);

struct sci_softc {
	struct device sc_dev;		/* boilerplate */
	struct tty *sc_tty;
	void *sc_si;
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
	u_int sc_fifolen;		/* XXX always 0? */

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

	volatile u_char sc_rx_flags,	/* receiver blocked */
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
static int sci_match(struct device *, struct cfdata *, void *);
static void sci_attach(struct device *, struct device *, void *);

void	sci_break(struct sci_softc *, int);
void	sci_iflush(struct sci_softc *);

#define	integrate	static inline
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
void 	scisoft(void *);
#else
#ifndef __NO_SOFT_SERIAL_INTERRUPT
void 	scisoft(void);
#else
void 	scisoft(void *);
#endif
#endif
integrate void sci_rxsoft(struct sci_softc *, struct tty *);
integrate void sci_txsoft(struct sci_softc *, struct tty *);
integrate void sci_stsoft(struct sci_softc *, struct tty *);
integrate void sci_schedrx(struct sci_softc *);
void	scidiag(void *);

#define	SCIUNIT_MASK		0x7ffff
#define	SCIDIALOUT_MASK	0x80000

#define	SCIUNIT(x)	(minor(x) & SCIUNIT_MASK)
#define	SCIDIALOUT(x)	(minor(x) & SCIDIALOUT_MASK)

/* Macros to clear/set/test flags. */
#define SET(t, f)	(t) |= (f)
#define CLR(t, f)	(t) &= ~(f)
#define ISSET(t, f)	((t) & (f))

/* Hardware flag masks */
#define	SCI_HW_NOIEN	0x01
#define	SCI_HW_FIFO	0x02
#define	SCI_HW_FLOW	0x08
#define	SCI_HW_DEV_OK	0x20
#define	SCI_HW_CONSOLE	0x40
#define	SCI_HW_KGDB	0x80

/* Buffer size for character buffer */
#define	SCI_RING_SIZE	2048

/* Stop input when 3/4 of the ring is full; restart when only 1/4 is full. */
u_int sci_rbuf_hiwat = (SCI_RING_SIZE * 1) / 4;
u_int sci_rbuf_lowat = (SCI_RING_SIZE * 3) / 4;

#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
int sciconscflag = CONMODE;
int sciisconsole = 0;

#ifdef SCICN_SPEED
int scicn_speed = SCICN_SPEED;
#else
int scicn_speed = 9600;
#endif

#define	divrnd(n, q)	(((n)*2/(q)+1)/2)	/* divide and round off */

#ifndef __HAVE_GENERIC_SOFT_INTERRUPTS
#ifdef __NO_SOFT_SERIAL_INTERRUPT
volatile int	sci_softintr_scheduled;
struct callout sci_soft_ch = CALLOUT_INITIALIZER;
#endif
#endif

u_int sci_rbuf_size = SCI_RING_SIZE;

struct cfattach sci_ca = {
	sizeof(struct sci_softc), sci_match, sci_attach
};

extern struct cfdriver sci_cd;

cdev_decl(sci);

void InitializeSci (unsigned int);

/*
 * following functions are debugging prupose only
 */
#define CR      0x0D
#define I2C_ADRS (*(volatile unsigned int *)0xa8000000)
#define USART_ON (unsigned int)~0x08

void sci_putc(unsigned char);
unsigned char sci_getc(void);
int SciErrCheck(void);

/*
 * InitializeSci
 * : unsigned int bps;
 * : SCI(Serial Communication Interface)
 */

void
InitializeSci(unsigned int bps)
{

	/* Initialize SCR */
	SHREG_SCSCR = 0x00;

	/* Serial Mode Register */
	SHREG_SCSMR = 0x00;	/* Async,8bit,NonParity,Even,1Stop,NoMulti */

	/* Bit Rate Register */
	SHREG_SCBRR = divrnd(sh_clock_get_pclock(), 32 * bps) - 1;

	/*
	 * wait 1mSec, because Send/Recv must begin 1 bit period after
	 * BRR is set.
	 */
	delay(1000);

	/* Send permission, Receive permission ON */
	SHREG_SCSCR = SCSCR_TE | SCSCR_RE;

	/* Serial Status Register */
	SHREG_SCSSR &= SCSSR_TDRE;	/* Clear Status */

#if 0
	I2C_ADRS &= ~0x08;	/* enable RS-232C */
#endif
}


/*
 * sci_putc
 *  : unsigned char c;
 */
void
sci_putc(unsigned char c)
{

	if (c == '\n')
		sci_putc('\r');

	/* wait for ready */
	while ((SHREG_SCSSR & SCSSR_TDRE) == NULL)
		;

	/* write send data to send register */
	SHREG_SCTDR = c;

	/* clear ready flag */
	SHREG_SCSSR &= ~SCSSR_TDRE;
}

/*
 * : SciErrCheck
 *	0x20 = over run
 *	0x10 = frame error
 *	0x80 = parity error
 */
int
SciErrCheck(void)
{

	return(SHREG_SCSSR & (SCSSR_ORER | SCSSR_FER | SCSSR_PER));
}

/*
 * sci_getc
 */
unsigned char
sci_getc(void)
{
	unsigned char c, err_c;

	while (((err_c = SHREG_SCSSR)
		& (SCSSR_RDRF | SCSSR_ORER | SCSSR_FER | SCSSR_PER)) == 0)
		;
	if ((err_c & (SCSSR_ORER | SCSSR_FER | SCSSR_PER)) != 0) {
		SHREG_SCSSR &= ~(SCSSR_ORER | SCSSR_FER | SCSSR_PER);
		return(err_c |= 0x80);
	}

	c = SHREG_SCRDR;

	SHREG_SCSSR &= ~SCSSR_RDRF;

	return(c);
}

#if 0
#define SCI_MAX_UNITS 2
#else
#define SCI_MAX_UNITS 1
#endif


static int
sci_match(struct device *parent, struct cfdata *cfp, void *aux)
{

	if (strcmp(cfp->cf_driver->cd_name, "sci")
	    || cfp->cf_unit >= SCI_MAX_UNITS) //XXX __BROKEN_CONFIG_UNIT_USAGE
		return 0;

	return 1;
}

static void
sci_attach(struct device *parent, struct device *self, void *aux)
{
	struct sci_softc *sc = (struct sci_softc *)self;
	struct tty *tp;

	sc->sc_hwflags = 0;	/* XXX */
	sc->sc_swflags = 0;	/* XXX */
	sc->sc_fifolen = 0;	/* XXX */

	if (sciisconsole) {
		SET(sc->sc_hwflags, SCI_HW_CONSOLE);
		SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);
		printf("\n%s: console\n", sc->sc_dev.dv_xname);
	} else {
		InitializeSci(9600);
		printf("\n");
	}

	callout_init(&sc->sc_diag_ch);

	intc_intr_establish(SH_INTEVT_SCI_ERI, IST_LEVEL, IPL_SERIAL, sciintr,
	    sc);
	intc_intr_establish(SH_INTEVT_SCI_RXI, IST_LEVEL, IPL_SERIAL, sciintr,
	    sc);
	intc_intr_establish(SH_INTEVT_SCI_TXI, IST_LEVEL, IPL_SERIAL, sciintr,
	    sc);
	intc_intr_establish(SH_INTEVT_SCI_TEI, IST_LEVEL, IPL_SERIAL, sciintr,
	    sc);

	SET(sc->sc_hwflags, SCI_HW_DEV_OK);

	tp = ttymalloc();
	tp->t_oproc = scistart;
	tp->t_param = sciparam;
	tp->t_hwiflow = NULL;

	sc->sc_tty = tp;
	sc->sc_rbuf = malloc(sci_rbuf_size << 1, M_DEVBUF, M_NOWAIT);
	if (sc->sc_rbuf == NULL) {
		printf("%s: unable to allocate ring buffer\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	sc->sc_ebuf = sc->sc_rbuf + (sci_rbuf_size << 1);

	tty_attach(tp);
}

/*
 * Start or restart transmission.
 */
static void
scistart(struct tty *tp)
{
	struct sci_softc *sc = sci_cd.cd_devs[SCIUNIT(tp->t_dev)];
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
	SHREG_SCSCR |= SCSCR_TIE | SCSCR_RIE;

	/* Output the first byte of the contiguous buffer. */
	{
		if (sc->sc_tbc > 0) {
			sci_putc(*(sc->sc_tba));
			sc->sc_tba++;
			sc->sc_tbc--;
		}
	}
out:
	splx(s);
	return;
}

/*
 * Set SCI tty parameters from termios.
 * XXX - Should just copy the whole termios after
 * making sure all the changes could be done.
 */
static int
sciparam(struct tty *tp, struct termios *t)
{
	struct sci_softc *sc = sci_cd.cd_devs[SCIUNIT(tp->t_dev)];
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
	    ISSET(sc->sc_hwflags, SCI_HW_CONSOLE)) {
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
	if (ISSET(sc->sc_hwflags, SCI_HW_HAYESP))
		sc->sc_fifo = FIFO_DMA_MODE | FIFO_ENABLE | FIFO_TRIGGER_8;
	else if (ISSET(sc->sc_hwflags, SCI_HW_FIFO))
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
			sci_loadchannelregs(sc);
#endif
	}

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		/* Disable the high water mark. */
		sc->sc_r_hiwat = 0;
		sc->sc_r_lowat = 0;
		if (ISSET(sc->sc_rx_flags, RX_TTY_OVERFLOWED)) {
			CLR(sc->sc_rx_flags, RX_TTY_OVERFLOWED);
			sci_schedrx(sc);
		}
	} else {
		sc->sc_r_hiwat = sci_rbuf_hiwat;
		sc->sc_r_lowat = sci_rbuf_lowat;
	}

	splx(s);

#ifdef SCI_DEBUG
	if (sci_debug)
		scistatus(sc, "sciparam ");
#endif

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		if (sc->sc_tx_stopped) {
			sc->sc_tx_stopped = 0;
			scistart(tp);
		}
	}

	return (0);
}

void
sci_iflush(struct sci_softc *sc)
{
	unsigned char err_c;
	volatile unsigned char c;

	if (((err_c = SHREG_SCSSR)
	     & (SCSSR_RDRF | SCSSR_ORER | SCSSR_FER | SCSSR_PER)) != 0) {

		if ((err_c & (SCSSR_ORER | SCSSR_FER | SCSSR_PER)) != 0) {
			SHREG_SCSSR &= ~(SCSSR_ORER | SCSSR_FER | SCSSR_PER);
			return;
		}

		c = SHREG_SCRDR;

		SHREG_SCSSR &= ~SCSSR_RDRF;
	}
}

int
sciopen(dev_t dev, int flag, int mode, struct proc *p)
{
	int unit = SCIUNIT(dev);
	struct sci_softc *sc;
	struct tty *tp;
	int s, s2;
	int error;

	if (unit >= sci_cd.cd_ndevs)
		return (ENXIO);
	sc = sci_cd.cd_devs[unit];
	if (sc == 0 || !ISSET(sc->sc_hwflags, SCI_HW_DEV_OK) ||
	    sc->sc_rbuf == NULL)
		return (ENXIO);

	if (ISSET(sc->sc_dev.dv_flags, DVF_ACTIVE) == 0)
		return (ENXIO);

#ifdef KGDB
	/*
	 * If this is the kgdb port, no other use is permitted.
	 */
	if (ISSET(sc->sc_hwflags, SCI_HW_KGDB))
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
		SHREG_SCSCR |= SCSCR_TIE | SCSCR_RIE;

		splx(s2);

		/*
		 * Initialize the termios status to the defaults.  Add in the
		 * sticky bits from TIOCSFLAGS.
		 */
		t.c_ispeed = 0;
		if (ISSET(sc->sc_hwflags, SCI_HW_CONSOLE)) {
			t.c_ospeed = scicn_speed;
			t.c_cflag = sciconscflag;
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
		/* Make sure sciparam() will do something. */
		tp->t_ospeed = 0;
		(void) sciparam(tp, &t);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		ttychars(tp);
		ttsetwater(tp);

		s2 = splserial();

		/* Clear the input ring, and unblock. */
		sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
		sc->sc_rbavail = sci_rbuf_size;
		sci_iflush(sc);
		CLR(sc->sc_rx_flags, RX_ANY_BLOCK);
#if 0
/* XXX (msaitoh) */
		sci_hwiflow(sc);
#endif

#ifdef SCI_DEBUG
		if (sci_debug)
			scistatus(sc, "sciopen  ");
#endif

		splx(s2);
	}

	splx(s);

	error = ttyopen(tp, SCIDIALOUT(dev), ISSET(flag, O_NONBLOCK));
	if (error)
		goto bad;

	error = (*tp->t_linesw->l_open)(dev, tp);
	if (error)
		goto bad;

	return (0);

bad:

	return (error);
}

int
sciclose(dev_t dev, int flag, int mode, struct proc *p)
{
	struct sci_softc *sc = sci_cd.cd_devs[SCIUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	/* XXX This is for cons.c. */
	if (!ISSET(tp->t_state, TS_ISOPEN))
		return (0);

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);

	if (ISSET(sc->sc_dev.dv_flags, DVF_ACTIVE) == 0)
		return (0);

	return (0);
}

int
sciread(dev_t dev, struct uio *uio, int flag)
{
	struct sci_softc *sc = sci_cd.cd_devs[SCIUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
sciwrite(dev_t dev, struct uio *uio, int flag)
{
	struct sci_softc *sc = sci_cd.cd_devs[SCIUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
scipoll(dev_t dev, int events, struct proc *p)
{
	struct sci_softc *sc = sci_cd.cd_devs[SCIUNIT(dev)];
	struct tty *tp = sc->sc_tty;
 
	return ((*tp->t_linesw->l_poll)(tp, events, p));
}

struct tty *
scitty(dev_t dev)
{
	struct sci_softc *sc = sci_cd.cd_devs[SCIUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return (tp);
}

int
sciioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct sci_softc *sc = sci_cd.cd_devs[SCIUNIT(dev)];
	struct tty *tp = sc->sc_tty;
	int error;
	int s;

	if (ISSET(sc->sc_dev.dv_flags, DVF_ACTIVE) == 0)
		return (EIO);

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, p);
	if (error != EPASSTHROUGH)
		return (error);

	error = ttioctl(tp, cmd, data, flag, p);
	if (error != EPASSTHROUGH)
		return (error);

	error = 0;

	s = splserial();

	switch (cmd) {
	case TIOCSBRK:
		sci_break(sc, 1);
		break;

	case TIOCCBRK:
		sci_break(sc, 0);
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
		error = EPASSTHROUGH;
		break;
	}

	splx(s);

	return (error);
}

integrate void
sci_schedrx(struct sci_softc *sc)
{

	sc->sc_rx_ready = 1;

	/* Wake up the poller. */
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	softintr_schedule(sc->sc_si);
#else
#ifndef __NO_SOFT_SERIAL_INTERRUPT
	setsoftserial();
#else
	if (!sci_softintr_scheduled) {
		sci_softintr_scheduled = 1;
		callout_reset(&sci_soft_ch, 1, scisoft, NULL);
	}
#endif
#endif
}

void
sci_break(struct sci_softc *sc, int onoff)
{

	if (onoff)
		SHREG_SCSSR &= ~SCSSR_TDRE;
	else
		SHREG_SCSSR |= SCSSR_TDRE;

#if 0	/* XXX */
	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			sci_loadchannelregs(sc);
	}
#endif
}

/*
 * Stop output, e.g., for ^S or output flush.
 */
void
scistop(struct tty *tp, int flag)
{
	struct sci_softc *sc = sci_cd.cd_devs[SCIUNIT(tp->t_dev)];
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
scidiag(void *arg)
{
	struct sci_softc *sc = arg;
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
sci_rxsoft(struct sci_softc *sc, struct tty *tp)
{
	int (*rint)(int c, struct tty *tp) = tp->t_linesw->l_rint;
	u_char *get, *end;
	u_int cc, scc;
	u_char ssr;
	int code;
	int s;

	end = sc->sc_ebuf;
	get = sc->sc_rbget;
	scc = cc = sci_rbuf_size - sc->sc_rbavail;

	if (cc == sci_rbuf_size) {
		sc->sc_floods++;
		if (sc->sc_errors++ == 0)
			callout_reset(&sc->sc_diag_ch, 60 * hz, scidiag, sc);
	}

	while (cc) {
		code = get[0];
		ssr = get[1];
		if (ISSET(ssr, SCSSR_FER | SCSSR_PER)) {
			if (ISSET(ssr, SCSSR_FER))
				SET(code, TTY_FE);
			if (ISSET(ssr, SCSSR_PER))
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
					get -= sci_rbuf_size << 1;
				cc = 0;
			} else {
				/*
				 * Don't schedule any more receive processing
				 * until the line discipline tells us there's
				 * space available (through scihwiflow()).
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
				SHREG_SCSCR |= SCSCR_RIE;
			}
#if 0
			if (ISSET(sc->sc_rx_flags, RX_IBUF_BLOCKED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_BLOCKED);
				sci_hwiflow(sc);
			}
#endif
		}
		splx(s);
	}
}

integrate void
sci_txsoft(sc, tp)
	struct sci_softc *sc;
	struct tty *tp;
{

	CLR(tp->t_state, TS_BUSY);
	if (ISSET(tp->t_state, TS_FLUSH))
		CLR(tp->t_state, TS_FLUSH);
	else
		ndflush(&tp->t_outq, (int)(sc->sc_tba - tp->t_outq.c_cf));
	(*tp->t_linesw->l_start)(tp);
}

integrate void
sci_stsoft(struct sci_softc *sc, struct tty *tp)
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
		(void) (*tp->t_linesw->l_modem)(tp, ISSET(msr, MSR_DCD));
	}

	if (ISSET(delta, sc->sc_msr_cts)) {
		/* Block or unblock output according to flow control. */
		if (ISSET(msr, sc->sc_msr_cts)) {
			sc->sc_tx_stopped = 0;
			(*tp->t_linesw->l_start)(tp);
		} else {
			sc->sc_tx_stopped = 1;
		}
	}

#ifdef SCI_DEBUG
	if (sci_debug)
		scistatus(sc, "sci_stsoft");
#endif
#endif
}

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
void
scisoft(void *arg)
{
	struct sci_softc *sc = arg;
	struct tty *tp;

	if (ISSET(sc->sc_dev.dv_flags, DVF_ACTIVE) == 0)
		return;

	{
#else
void
#ifndef __NO_SOFT_SERIAL_INTERRUPT
scisoft()
#else
scisoft(void *arg)
#endif
{
	struct sci_softc	*sc;
	struct tty	*tp;
	int	unit;
#ifdef __NO_SOFT_SERIAL_INTERRUPT
	int s;

	s = splsoftserial();
	sci_softintr_scheduled = 0;
#endif

	for (unit = 0; unit < sci_cd.cd_ndevs; unit++) {
		sc = sci_cd.cd_devs[unit];
		if (sc == NULL || !ISSET(sc->sc_hwflags, SCI_HW_DEV_OK))
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
			sci_rxsoft(sc, tp);
		}

#if 0
		if (sc->sc_st_check) {
			sc->sc_st_check = 0;
			sci_stsoft(sc, tp);
		}
#endif

		if (sc->sc_tx_done) {
			sc->sc_tx_done = 0;
			sci_txsoft(sc, tp);
		}
	}

#ifndef __HAVE_GENERIC_SOFT_INTERRUPTS
#ifdef __NO_SOFT_SERIAL_INTERRUPT
	splx(s);
#endif
#endif
}

int
sciintr(void *arg)
{
	struct sci_softc *sc = arg;
	u_char *put, *end;
	u_int cc;
	u_short ssr;

	if (ISSET(sc->sc_dev.dv_flags, DVF_ACTIVE) == 0)
		return (0);

	end = sc->sc_ebuf;
	put = sc->sc_rbput;
	cc = sc->sc_rbavail;

	ssr = SHREG_SCSSR;
	if (ISSET(ssr, SCSSR_FER)) {
		SHREG_SCSSR &= ~(SCSSR_ORER | SCSSR_PER | SCSSR_FER);
#if defined(DDB) || defined(KGDB)
#ifdef SH4
		if ((SHREG_SCSPTR & SCSPTR_SPB0DT) != 0) {
#else
		if ((SHREG_SCSPDR & SCSPDR_SCP0DT) != 0) {
#endif
#ifdef DDB
			if (ISSET(sc->sc_hwflags, SCI_HW_CONSOLE)) {
				console_debugger();
			}
#endif
#ifdef KGDB
			if (ISSET(sc->sc_hwflags, SCI_HW_KGDB)) {
				kgdb_connect(1);
			}
#endif
		}
#endif /* DDB || KGDB */
	}
	if ((SHREG_SCSSR & SCSSR_RDRF) != 0) {
		if (cc > 0) {
			put[0] = SHREG_SCRDR;
			put[1] = SHREG_SCSSR & 0x00ff;

			SHREG_SCSSR &= ~(SCSSR_ORER | SCSSR_FER | SCSSR_PER |
					 SCSSR_RDRF);

			put += 2;
			if (put >= end)
				put = sc->sc_rbuf;
			cc--;
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
			sci_hwiflow(sc);
#endif
		}

		/*
		 * If we're out of space, disable receive interrupts
		 * until the queue has drained a bit.
		 */
		if (!cc) {
			SHREG_SCSCR &= ~SCSCR_RIE;
		}
	} else {
		if (SHREG_SCSSR & SCSSR_RDRF) {
			SHREG_SCSCR &= ~(SCSCR_TIE | SCSCR_RIE);
		}
	}
	
#if 0
	msr = bus_space_read_1(iot, ioh, sci_msr);
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
#ifdef SCI_DEBUG
			if (sci_debug)
				scistatus(sc, "sciintr  ");
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
	if ((SHREG_SCSSR & SCSSR_TDRE) != 0) {
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
			sci_putc(*(sc->sc_tba));
			sc->sc_tba++;
			sc->sc_tbc--;
		} else {
			/* Disable transmit completion interrupts if necessary. */
#if 0
			if (ISSET(sc->sc_ier, IER_ETXRDY))
#endif
				SHREG_SCSCR &= ~SCSCR_TIE;

			if (sc->sc_tx_busy) {
				sc->sc_tx_busy = 0;
				sc->sc_tx_done = 1;
			}
		}
	}

	/* Wake up the poller. */
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	softintr_schedule(sc->sc_si);
#else
#ifndef __NO_SOFT_SERIAL_INTERRUPT
	setsoftserial();
#else
	if (!sci_softintr_scheduled) {
		sci_softintr_scheduled = 1;
		callout_reset(&sci_soft_ch, 1, scisoft, 1);
	}
#endif
#endif

#if NRND > 0 && defined(RND_SCI)
	rnd_add_uint32(&sc->rnd_source, iir | lsr);
#endif

	return (1);
}

void
scicnprobe(cp)
	struct consdev *cp;
{
	int maj;

	/* locate the major number */
	for (maj = 0; maj < nchrdev; maj++)
		if (cdevsw[maj].d_open == sciopen)
			break;

	/* Initialize required fields. */
	cp->cn_dev = makedev(maj, 0);
#ifdef SCICONSOLE
	cp->cn_pri = CN_REMOTE;
#else
	cp->cn_pri = CN_NORMAL;
#endif
}

void
scicninit(struct consdev *cp)
{

	InitializeSci(scicn_speed);
	sciisconsole = 1;
}

int
scicngetc(dev_t dev)
{
	int c;
	int s;

	s = splserial();
	c = sci_getc();
	splx(s);

	return (c);
}

void
scicnputc(dev_t dev, int c)
{
	int s;

	s = splserial();
	sci_putc((u_char)c);
	splx(s);
}

/*	$NetBSD: scif.c,v 1.64 2014/07/25 08:10:34 dholland Exp $ */

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
 * 3. Neither the name of the University nor the names of its contributors
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: scif.c,v 1.64 2014/07/25 08:10:34 dholland Exp $");

#include "opt_kgdb.h"
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
#include <sys/kgdb.h>
#include <sys/kauth.h>
#include <sys/intr.h>

#include <dev/cons.h>

#include <sh3/clock.h>
#include <sh3/exception.h>
#include <sh3/scifreg.h>

#include <sh3/dev/scifvar.h>

#include "locators.h"


struct scif_softc {
	device_t sc_dev;

	struct tty *sc_tty;
	void *sc_si;

	callout_t sc_diag_ch;

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


static int scif_match(device_t, cfdata_t, void *);
static void scif_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(scif, sizeof(struct scif_softc),
    scif_match, scif_attach, NULL, NULL);

static int scif_attached = 0;	/* XXX: FIXME: don't limit to just one! */

extern struct cfdriver scif_cd;

#define	SCIFUNIT_MASK		0x7ffff
#define	SCIFDIALOUT_MASK	0x80000

#define	SCIFUNIT(x)	(minor(x) & SCIFUNIT_MASK)
#define	SCIFDIALOUT(x)	(minor(x) & SCIFDIALOUT_MASK)


/* console */
dev_type_cnprobe(scifcnprobe);
dev_type_cninit(scifcninit);
dev_type_cngetc(scifcngetc);
dev_type_cnputc(scifcnputc);


/* cdevsw */
dev_type_open(scifopen);
dev_type_close(scifclose);
dev_type_read(scifread);
dev_type_write(scifwrite);
dev_type_ioctl(scifioctl);
dev_type_stop(scifstop);
dev_type_tty(sciftty);
dev_type_poll(scifpoll);

const struct cdevsw scif_cdevsw = {
	.d_open = scifopen,
	.d_close = scifclose,
	.d_read = scifread,
	.d_write = scifwrite,
	.d_ioctl = scifioctl,
	.d_stop = scifstop,
	.d_tty = sciftty,
	.d_poll = scifpoll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};


/* struct tty */
static void scifstart(struct tty *);
static int scifparam(struct tty *, struct termios *);


void InitializeScif (unsigned int);
int ScifErrCheck(void);
void scif_putc(unsigned char);
unsigned char scif_getc(void);

static int scifintr(void *);
static void scifsoft(void *);
static void scif_rxsoft(struct scif_softc *, struct tty *);
static void scif_txsoft(struct scif_softc *, struct tty *);
#if 0
static void scif_stsoft(struct scif_softc *, struct tty *);
#endif
static void scif_schedrx(struct scif_softc *);
static void scifdiag(void *);

static void scif_break(struct scif_softc *, int);
static void scif_iflush(struct scif_softc *);


/* Hardware flag masks (sc_hwflags) */
#define	SCIF_HW_NOIEN	0x01
#define	SCIF_HW_FIFO	0x02
#define	SCIF_HW_FLOW	0x08
#define	SCIF_HW_DEV_OK	0x20
#define	SCIF_HW_CONSOLE	0x40
#define	SCIF_HW_KGDB	0x80


/* Buffer size for character buffer */
#define	SCIF_RING_SIZE	2048
static unsigned int scif_rbuf_size = SCIF_RING_SIZE;

/* Stop input when 3/4 of the ring is full; restart when only 1/4 is full. */
static unsigned int scif_rbuf_hiwat = (SCIF_RING_SIZE * 1) / 4;
static unsigned int scif_rbuf_lowat = (SCIF_RING_SIZE * 3) / 4;


#ifdef SCIFCN_SPEED
unsigned int scifcn_speed = SCIFCN_SPEED;
#else
unsigned int scifcn_speed = 9600;
#endif

#define	CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
int scifconscflag = CONMODE;

static int scifisconsole = 0;

#ifdef KGDB
static int kgdb_attached = 0;
#endif


#define	divrnd(n, q)	(((n)*2/(q)+1)/2)	/* divide and round off */


/* XXX: uwe
 * Prepare for bus_spacification.  The difference in access widths is
 * still handled by the magic definitions in scifreg.h
 */
#define scif_smr_read()		SHREG_SCSMR2
#define scif_smr_write(v)	(SHREG_SCSMR2 = (v))

#define scif_brr_read()		SHREG_SCBRR2
#define scif_brr_write(v)	(SHREG_SCBRR2 = (v))

#define scif_scr_read()		SHREG_SCSCR2
#define scif_scr_write(v)	(SHREG_SCSCR2 = (v))

#define scif_ftdr_write(v)	(SHREG_SCFTDR2 = (v))

#define scif_ssr_read()		SHREG_SCSSR2
#define scif_ssr_write(v)	(SHREG_SCSSR2 = (v))

#define scif_frdr_read()	SHREG_SCFRDR2

#define scif_fcr_read()		SHREG_SCFCR2
#define scif_fcr_write(v)	(SHREG_SCFCR2 = (v))

#define scif_fdr_read()		SHREG_SCFDR2

#ifdef SH4 /* additional registers in sh4 */

#define scif_sptr_read()	SHREG_SCSPTR2
#define scif_sptr_write(v)	(SHREG_SCSPTR2 = (v))

#define scif_lsr_read()		SHREG_SCLSR2
#define scif_lsr_write(v)	(SHREG_SCLSR2 = (v))

#endif /* SH4 */


void
InitializeScif(unsigned int bps)
{

	/* Initialize SCR */
	scif_scr_write(0x00);

#if 0
	scif_fcr_write(SCFCR2_TFRST | SCFCR2_RFRST | SCFCR2_MCE);
#else
	scif_fcr_write(SCFCR2_TFRST | SCFCR2_RFRST);
#endif
	/* Serial Mode Register */
	scif_smr_write(0x00);	/* 8bit,NonParity,Even,1Stop */

	/* Bit Rate Register */
	scif_brr_write(divrnd(sh_clock_get_pclock(), 32 * bps) - 1);

	/*
	 * wait 2m Sec, because Send/Recv must begin 1 bit period after
	 * BRR is set.
	 */
	delay(2000);

#if 0
	scif_fcr_write(FIFO_RCV_TRIGGER_14 | FIFO_XMT_TRIGGER_1 | SCFCR2_MCE);
#else
	scif_fcr_write(FIFO_RCV_TRIGGER_14 | FIFO_XMT_TRIGGER_1);
#endif

	/* Send permission, Receive permission ON */
	scif_scr_write(SCSCR2_TE | SCSCR2_RE);

	/* Serial Status Register */
	scif_ssr_write(scif_ssr_read() & SCSSR2_TDFE); /* Clear Status */
}

int
ScifErrCheck(void)
{

	return (scif_ssr_read() & (SCSSR2_ER | SCSSR2_FER | SCSSR2_PER));
}

void
scif_putc(unsigned char c)
{

	/* wait for ready */
	while ((scif_fdr_read() & SCFDR2_TXCNT) == SCFDR2_TXF_FULL)
		continue;

	/* write send data to send register */
	scif_ftdr_write(c);

	/* clear ready flag */
	scif_ssr_write(scif_ssr_read() & ~(SCSSR2_TDFE | SCSSR2_TEND));
}

unsigned char
scif_getc(void)
{
	unsigned char c, err_c;
#ifdef SH4
	unsigned short err_c2 = 0; /* XXXGCC: -Wuninitialized */
#endif

	for (;;) {
		/* wait for ready */
		while ((scif_fdr_read() & SCFDR2_RECVCNT) == 0)
			continue;

		c = scif_frdr_read();
		err_c = scif_ssr_read();
		scif_ssr_write(scif_ssr_read()
			& ~(SCSSR2_ER | SCSSR2_BRK | SCSSR2_RDF | SCSSR2_DR));
#ifdef SH4
		if (CPU_IS_SH4) {
			err_c2 = scif_lsr_read();
			scif_lsr_write(scif_lsr_read() & ~SCLSR2_ORER);
		}
#endif
		if ((err_c & (SCSSR2_ER | SCSSR2_BRK | SCSSR2_FER
		    | SCSSR2_PER)) == 0)
		{
#ifdef SH4
			if (CPU_IS_SH4 && ((err_c2 & SCLSR2_ORER) == 0))
#endif
			return(c);
		}
	}

}

static int
scif_match(device_t parent, cfdata_t cfp, void *aux)
{

	if (scif_attached)
		return 0;

	if (strcmp(cfp->cf_name, "scif") != 0)
		return 0;

	return 1;
}

static void
scif_attach(device_t parent, device_t self, void *aux)
{
	struct scif_softc *sc;
	struct tty *tp;

	sc = device_private(self);
	sc->sc_dev = self;

	scif_attached = 1;

	sc->sc_hwflags = 0;	/* XXX */
	sc->sc_swflags = 0;	/* XXX */
	sc->sc_fifolen = 16;

	aprint_normal("\n");
	if (scifisconsole) {
		aprint_naive(" (console)\n");
		aprint_normal_dev(self, "console\n");
		SET(sc->sc_hwflags, SCIF_HW_CONSOLE);
		SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);
	}
#ifdef KGDB
	else if (kgdb_attached) {
		aprint_naive(" (kgdb)\n");
		aprint_normal_dev(self, "kgdb\n");
		SET(sc->sc_hwflags, SCIF_HW_KGDB);
		SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);
	}
#endif
	else {
		aprint_naive("\n");
		InitializeScif(9600); /* XXX */
	}

	callout_init(&sc->sc_diag_ch, 0);
#ifdef SH4
	intc_intr_establish(SH4_INTEVT_SCIF_ERI, IST_LEVEL, IPL_SERIAL,
	    scifintr, sc);
	intc_intr_establish(SH4_INTEVT_SCIF_RXI, IST_LEVEL, IPL_SERIAL,
	    scifintr, sc);
	intc_intr_establish(SH4_INTEVT_SCIF_BRI, IST_LEVEL, IPL_SERIAL,
	    scifintr, sc);
	intc_intr_establish(SH4_INTEVT_SCIF_TXI, IST_LEVEL, IPL_SERIAL,
	    scifintr, sc);
#else
	intc_intr_establish(SH7709_INTEVT2_SCIF_ERI, IST_LEVEL, IPL_SERIAL,
	    scifintr, sc);
	intc_intr_establish(SH7709_INTEVT2_SCIF_RXI, IST_LEVEL, IPL_SERIAL,
	    scifintr, sc);
	intc_intr_establish(SH7709_INTEVT2_SCIF_BRI, IST_LEVEL, IPL_SERIAL,
	    scifintr, sc);
	intc_intr_establish(SH7709_INTEVT2_SCIF_TXI, IST_LEVEL, IPL_SERIAL,
	    scifintr, sc);
#endif

	sc->sc_si = softint_establish(SOFTINT_SERIAL, scifsoft, sc);
	SET(sc->sc_hwflags, SCIF_HW_DEV_OK);

	tp = tty_alloc();
	tp->t_oproc = scifstart;
	tp->t_param = scifparam;
	tp->t_hwiflow = NULL;

	sc->sc_tty = tp;
	sc->sc_rbuf = malloc(scif_rbuf_size << 1, M_DEVBUF, M_NOWAIT);
	if (sc->sc_rbuf == NULL) {
		aprint_error_dev(self, "unable to allocate ring buffer\n");
		return;
	}
	sc->sc_ebuf = sc->sc_rbuf + (scif_rbuf_size << 1);

	tty_attach(tp);

	/* XXX: TODO */
	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "unable to establish power handler\n");
}

/*
 * Start or restart transmission.
 */
static void
scifstart(struct tty *tp)
{
	struct scif_softc *sc;
	int s;

	sc = device_lookup_private(&scif_cd, SCIFUNIT(tp->t_dev));

	s = spltty();
	if (ISSET(tp->t_state, TS_BUSY | TS_TIMEOUT | TS_TTSTOP))
		goto out;
	if (sc->sc_tx_stopped)
		goto out;
	if (!ttypull(tp))
		goto out;

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
	scif_scr_write(scif_scr_read() | SCSCR2_TIE | SCSCR2_RIE);

	/* Output the first chunk of the contiguous buffer. */
	{
		int n;
		int maxchars;
		int i;

		n = sc->sc_tbc;
		maxchars = sc->sc_fifolen
			- ((scif_fdr_read() & SCFDR2_TXCNT) >> 8);
		if (n > maxchars)
			n = maxchars;

		for (i = 0; i < n; i++) {
			scif_putc(*(sc->sc_tba));
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
scifparam(struct tty *tp, struct termios *t)
{
	struct scif_softc *sc;
	int ospeed = t->c_ospeed;
	int s;

	sc = device_lookup_private(&scif_cd, SCIFUNIT(tp->t_dev));
	if (!device_is_active(sc->sc_dev))
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
		scif_fcr_write(scif_fcr_read() | SCFCR2_MCE);
	} else {
		scif_fcr_write(scif_fcr_read() & ~SCFCR2_MCE);
	}

	scif_brr_write(divrnd(sh_clock_get_pclock(), 32 * ospeed) -1);

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

static void
scif_iflush(struct scif_softc *sc)
{
	int i;

	i = scif_fdr_read() & SCFDR2_RECVCNT;

	while (i > 0) {
		(void)scif_frdr_read();
		scif_ssr_write(scif_ssr_read() & ~(SCSSR2_RDF | SCSSR2_DR));
		i--;
	}
}

int
scifopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct scif_softc *sc;
	struct tty *tp;
	int s, s2;
	int error;

	sc = device_lookup_private(&scif_cd, SCIFUNIT(dev));
	if (sc == 0 || !ISSET(sc->sc_hwflags, SCIF_HW_DEV_OK) ||
	    sc->sc_rbuf == NULL)
		return (ENXIO);

	if (!device_is_active(sc->sc_dev))
		return (ENXIO);

#ifdef KGDB
	/*
	 * If this is the kgdb port, no other use is permitted.
	 */
	if (ISSET(sc->sc_hwflags, SCIF_HW_KGDB))
		return (EBUSY);
#endif /* KGDB */

	tp = sc->sc_tty;

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
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
		scif_scr_write(scif_scr_read() | SCSCR2_TIE | SCSCR2_RIE);

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

	error = (*tp->t_linesw->l_open)(dev, tp);
	if (error)
		goto bad;

	return (0);

bad:

	return (error);
}

int
scifclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct scif_softc *sc;
	struct tty *tp;

	sc = device_lookup_private(&scif_cd, SCIFUNIT(dev));
	tp = sc->sc_tty;

	/* XXX This is for cons.c. */
	if (!ISSET(tp->t_state, TS_ISOPEN))
		return (0);

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);

	if (!device_is_active(sc->sc_dev))
		return (0);

	return (0);
}

int
scifread(dev_t dev, struct uio *uio, int flag)
{
	struct scif_softc *sc;
	struct tty *tp;

	sc = device_lookup_private(&scif_cd, SCIFUNIT(dev));
	tp = sc->sc_tty;

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
scifwrite(dev_t dev, struct uio *uio, int flag)
{
	struct scif_softc *sc;
	struct tty *tp;

	sc = device_lookup_private(&scif_cd, SCIFUNIT(dev));
	tp = sc->sc_tty;

	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
scifpoll(dev_t dev, int events, struct lwp *l)
{
	struct scif_softc *sc;
	struct tty *tp;

	sc = device_lookup_private(&scif_cd, SCIFUNIT(dev));
	tp = sc->sc_tty;

	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

struct tty *
sciftty(dev_t dev)
{
	struct scif_softc *sc;
	struct tty *tp;

	sc = device_lookup_private(&scif_cd, SCIFUNIT(dev));
	tp = sc->sc_tty;

	return (tp);
}

int
scifioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct scif_softc *sc;
	struct tty *tp;
	int error;
	int s;

	sc = device_lookup_private(&scif_cd, SCIFUNIT(dev));
	if (!device_is_active(sc->sc_dev))
		return (EIO);

	tp = sc->sc_tty;
	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	error = ttioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
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
		error = kauth_authorize_device_tty(l->l_cred,
		    KAUTH_DEVICE_TTY_PRIVSET, tp);
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

static void
scif_schedrx(struct scif_softc *sc)
{

	sc->sc_rx_ready = 1;

	/* Wake up the poller. */
	softint_schedule(sc->sc_si);
}

static void
scif_break(struct scif_softc *sc, int onoff)
{

	if (onoff)
		scif_ssr_write(scif_ssr_read() & ~SCSSR2_TDFE);
	else
		scif_ssr_write(scif_ssr_read() | SCSSR2_TDFE);

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
scifstop(struct tty *tp, int flag)
{
	struct scif_softc *sc;
	int s;

	sc = device_lookup_private(&scif_cd, SCIFUNIT(tp->t_dev));

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

static void
scifdiag(void *arg)
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
	    device_xname(sc->sc_dev),
	    overflows, overflows == 1 ? "" : "s",
	    floods, floods == 1 ? "" : "s");
}

static void
scif_rxsoft(struct scif_softc *sc, struct tty *tp)
{
	int (*rint)(int, struct tty *) = tp->t_linesw->l_rint;
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
			callout_reset(&sc->sc_diag_ch, 60 * hz, scifdiag, sc);
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
				scif_scr_write(scif_scr_read() | SCSCR2_RIE);
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

static void
scif_txsoft(struct scif_softc *sc, struct tty *tp)
{

	CLR(tp->t_state, TS_BUSY);
	if (ISSET(tp->t_state, TS_FLUSH))
		CLR(tp->t_state, TS_FLUSH);
	else
		ndflush(&tp->t_outq, (int)(sc->sc_tba - tp->t_outq.c_cf));
	(*tp->t_linesw->l_start)(tp);
}

#if 0 /* XXX (msaitoh) */
static void
scif_stsoft(struct scif_softc *sc, struct tty *tp)
{
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

#ifdef SCIF_DEBUG
	if (scif_debug)
		scifstatus(sc, "scif_stsoft");
#endif
}
#endif /* 0 */

static void
scifsoft(void *arg)
{
	struct scif_softc *sc = arg;
	struct tty *tp;

	if (!device_is_active(sc->sc_dev))
		return;

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

static int
scifintr(void *arg)
{
	struct scif_softc *sc = arg;
	u_char *put, *end;
	u_int cc;
	u_short ssr2;
	int count;

	if (!device_is_active(sc->sc_dev))
		return (0);

	end = sc->sc_ebuf;
	put = sc->sc_rbput;
	cc = sc->sc_rbavail;

	do {
		ssr2 = scif_ssr_read();
		if (ISSET(ssr2, SCSSR2_BRK)) {
			scif_ssr_write(scif_ssr_read()
				& ~(SCSSR2_ER | SCSSR2_BRK | SCSSR2_DR));
#ifdef DDB
			if (ISSET(sc->sc_hwflags, SCIF_HW_CONSOLE)) {
				console_debugger();
			}
#endif /* DDB */
#ifdef KGDB
			if (ISSET(sc->sc_hwflags, SCIF_HW_KGDB)) {
				kgdb_connect(1);
			}
#endif /* KGDB */
		}
		count = scif_fdr_read() & SCFDR2_RECVCNT;
		if (count != 0) {
			for (;;) {
				u_char c = scif_frdr_read();
				u_char err = (u_char)(scif_ssr_read() & 0x00ff);

				scif_ssr_write(scif_ssr_read()
				    & ~(SCSSR2_ER | SCSSR2_RDF | SCSSR2_DR));
#ifdef SH4
				if (CPU_IS_SH4)
					scif_lsr_write(scif_lsr_read()
						       & ~SCLSR2_ORER);
#endif
				if ((cc > 0) && (count > 0)) {
					put[0] = c;
					put[1] = err;
					put += 2;
					if (put >= end)
						put = sc->sc_rbuf;
					cc--;
					count--;
				} else
					break;
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
				SET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
				scif_scr_write(scif_scr_read() & ~SCSCR2_RIE);
			}
		} else {
			if (scif_ssr_read() & (SCSSR2_RDF | SCSSR2_DR)) {
				scif_scr_write(scif_scr_read()
					       & ~(SCSCR2_TIE | SCSCR2_RIE));
				delay(10);
				scif_scr_write(scif_scr_read()
					       | SCSCR2_TIE | SCSCR2_RIE);
				continue;
			}
		}
	} while (scif_ssr_read() & (SCSSR2_RDF | SCSSR2_DR));

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
	if (((scif_fdr_read() & SCFDR2_TXCNT) >> 8) != 16) { /* XXX (msaitoh) */
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
			int maxchars;
			int i;

			n = sc->sc_tbc;
			maxchars = sc->sc_fifolen -
				((scif_fdr_read() & SCFDR2_TXCNT) >> 8);
			if (n > maxchars)
				n = maxchars;

			for (i = 0; i < n; i++) {
				scif_putc(*(sc->sc_tba));
				sc->sc_tba++;
			}
			sc->sc_tbc -= n;
		} else {
			/* Disable transmit completion interrupts if necessary. */
#if 0
			if (ISSET(sc->sc_ier, IER_ETXRDY))
#endif
				scif_scr_write(scif_scr_read() & ~SCSCR2_TIE);

			if (sc->sc_tx_busy) {
				sc->sc_tx_busy = 0;
				sc->sc_tx_done = 1;
			}
		}
	}

	/* Wake up the poller. */
	softint_schedule(sc->sc_si);

#ifdef RND_SCIF
	rnd_add_uint32(&sc->rnd_source, iir | lsr);
#endif

	return (1);
}

void
scifcnprobe(struct consdev *cp)
{
	int maj;

	/* locate the major number */
	maj = cdevsw_lookup_major(&scif_cdevsw);

	/* Initialize required fields. */
	cp->cn_dev = makedev(maj, 0);
#ifdef SCIFCONSOLE
	cp->cn_pri = CN_REMOTE;
#else
	cp->cn_pri = CN_NORMAL;
#endif
}

void
scifcninit(struct consdev *cp)
{

	InitializeScif(scifcn_speed);
	scifisconsole = 1;
}

int
scifcngetc(dev_t dev)
{
	int c;
	int s;

	s = splserial();
	c = scif_getc();
	splx(s);

	return (c);
}

void
scifcnputc(dev_t dev, int c)
{
	int s;

	s = splserial();
	scif_putc((u_char)c);
	splx(s);
}

#ifdef KGDB
int
scif_kgdb_init(void)
{

	if (strcmp(kgdb_devname, "scif") != 0)
		return (1);

	if (scifisconsole)
		return (1);	/* can't share with console */

	InitializeScif(kgdb_rate);

	kgdb_attach((int (*)(void *))scifcngetc,
	    (void (*)(void *, int))scifcnputc, NULL);
	kgdb_dev = 123; /* unneeded, only to satisfy some tests */
	kgdb_attached = 1;

	return (0);
}
#endif /* KGDB */

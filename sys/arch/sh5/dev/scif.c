/*	$NetBSD: scif.c,v 1.10 2003/03/13 13:19:01 scw Exp $	*/

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
 * SH-5 internal serial driver.
 *
 * This code is derived from the SH-3 version of the driver,
 * which in turn says:
 *
 * "This code is derived from both z8530tty.c and com.c"
 *
 * The code needs some overhauling to make it truely MI so it can
 * be shared with SH-3.
 */

#include "opt_kgdb.h"

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

#if 0
#include <sys/kgdb.h>
#endif

#include <dev/cons.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <sh5/dev/pbridgevar.h>
#include <sh5/dev/scifreg.h>
#include <sh5/dev/scifvar.h>
#include <sh5/dev/cprcvar.h>
#include <sh5/dev/intcreg.h>

#include "locators.h"

static void	scifstart(struct tty *);
static int	scifparam(struct tty *, struct termios *);
static int kgdb_attached;

void scifcnprobe(struct consdev *);
void scifcninit(struct consdev *);
void scifcnputc(dev_t, int);
int scifcngetc(dev_t);
void scifcnpoolc(dev_t, int);
void scif_intr_init(void);
int scifintr(void *);

struct scif_softc {
	struct device sc_dev;		/* boilerplate */
	struct tty *sc_tty;
	void *sc_si;

	struct callout sc_diag_ch;

	bus_space_tag_t sc_iot;		/* ISA i/o space identifier */
	bus_space_handle_t sc_ioh;	/* ISA io handle */

	int sc_drq;

	int sc_frequency;

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
static int scif_match(struct device *, struct cfdata *, void *);
static void scif_attach(struct device *, struct device *, void *);

void	scif_break(struct scif_softc *, int);
void	scif_iflush(struct scif_softc *);

#define	integrate	static inline
void 	scifsoft(void *);
integrate void scif_rxsoft(struct scif_softc *, struct tty *);
integrate void scif_txsoft(struct scif_softc *, struct tty *);
integrate void scif_stsoft(struct scif_softc *, struct tty *);
integrate void scif_schedrx(struct scif_softc *);
void	scifdiag(void *);


#define	SCIFUNIT_MASK		0x7ffff
#define	SCIFDIALOUT_MASK	0x80000

#define	SCIFUNIT(x)	(minor(x) & SCIFUNIT_MASK)
#define	SCIFDIALOUT(x)	(minor(x) & SCIFDIALOUT_MASK)

/* Macros to clear/set/test flags. */
#define	SET(t, f)	(t) |= (f)
#define	CLR(t, f)	(t) &= ~(f)
#define	ISSET(t, f)	((t) & (f))

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

#define	CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
int scifconscflag = CONMODE;
int scifisconsole = 0;

#ifdef SCIFCN_SPEED
unsigned int scifcn_speed = SCIFCN_SPEED;
#else
unsigned int scifcn_speed = 38400;
#endif

#define	divrnd(n, q)	(((n)*2/(q)+1)/2)	/* divide and round off */

u_int scif_rbuf_size = SCIF_RING_SIZE;

CFATTACH_DECL(scif, sizeof(struct scif_softc),
    scif_match, scif_attach, NULL, NULL);

extern struct cfdriver scif_cd;

dev_type_open(scifopen);
dev_type_close(scifclose);
dev_type_read(scifread);
dev_type_write(scifwrite);
dev_type_ioctl(scifioctl);
dev_type_stop(scifstop);
dev_type_tty(sciftty);
dev_type_poll(scifpoll);

const struct cdevsw scif_cdevsw = {
	scifopen, scifclose, scifread, scifwrite, scifioctl,
	scifstop, sciftty, scifpoll, nommap, ttykqfilter, D_TTY
};

void InitializeScif(bus_space_tag_t, bus_space_handle_t, unsigned int);

/*
 * following functions are debugging prupose only
 */
#define	CR      0x0D
#define	USART_ON (unsigned int)~0x08

static void scif_putc(bus_space_tag_t, bus_space_handle_t, unsigned char);
static unsigned char scif_getc(bus_space_tag_t, bus_space_handle_t);


#define scif_rd_scsmr2(sc) \
	    bus_space_read_2((sc)->sc_iot, (sc)->sc_ioh, SCIF_REG_SCSMR2)
#define scif_wr_scsmr2(sc,v) \
	    bus_space_write_2((sc)->sc_iot, (sc)->sc_ioh, SCIF_REG_SCSMR2, (v))
#define scif_rd_scbrr2(sc) \
	    bus_space_read_1((sc)->sc_iot, (sc)->sc_ioh, SCIF_REG_SCBRR2)
#define scif_wr_scbrr2(sc,v) \
	    bus_space_write_1((sc)->sc_iot, (sc)->sc_ioh, SCIF_REG_SCBRR2, (v))
#define scif_rd_scscr2(sc) \
	    bus_space_read_2((sc)->sc_iot, (sc)->sc_ioh, SCIF_REG_SCSCR2)
#define scif_wr_scscr2(sc,v) \
	    bus_space_write_2((sc)->sc_iot, (sc)->sc_ioh, SCIF_REG_SCSCR2, (v))
#define scif_rd_scftdr2(sc) \
	    bus_space_read_1((sc)->sc_iot, (sc)->sc_ioh, SCIF_REG_SCFTDR2)
#define scif_wr_scftdr2(sc,v) \
	    bus_space_write_1((sc)->sc_iot, (sc)->sc_ioh, SCIF_REG_SCFTDR2, (v))
#define scif_rd_scfsr2(sc) \
	    bus_space_read_2((sc)->sc_iot, (sc)->sc_ioh, SCIF_REG_SCFSR2)
#define scif_wr_scfsr2(sc,v) \
	    bus_space_write_2((sc)->sc_iot, (sc)->sc_ioh, SCIF_REG_SCFSR2, (v))
#define scif_rd_scfrd2(sc) \
	    bus_space_read_1((sc)->sc_iot, (sc)->sc_ioh, SCIF_REG_SCFRD2)
#define scif_wr_scfrd2(sc,v) \
	    bus_space_write_1((sc)->sc_iot, (sc)->sc_ioh, SCIF_REG_SCFRD2, (v))
#define scif_rd_scfcr2(sc) \
	    bus_space_read_2((sc)->sc_iot, (sc)->sc_ioh, SCIF_REG_SCFCR2)
#define scif_wr_scfcr2(sc,v) \
	    bus_space_write_2((sc)->sc_iot, (sc)->sc_ioh, SCIF_REG_SCFCR2, (v))
#define scif_rd_scfdr2(sc) \
	    bus_space_read_2((sc)->sc_iot, (sc)->sc_ioh, SCIF_REG_SCFDR2)
#define scif_wr_scfdr2(sc,v) \
	    bus_space_write_2((sc)->sc_iot, (sc)->sc_ioh, SCIF_REG_SCFDR2, (v))
#define scif_rd_scsptr2(sc) \
	    bus_space_read_2((sc)->sc_iot, (sc)->sc_ioh, SCIF_REG_SCSPTR2)
#define scif_wr_scsptr2(sc,v) \
	    bus_space_write_2((sc)->sc_iot, (sc)->sc_ioh, SCIF_REG_SCSPTR2, (v))
#define scif_rd_sclsr2(sc) \
	    bus_space_read_2((sc)->sc_iot, (sc)->sc_ioh, SCIF_REG_SCLSR2)
#define scif_wr_sclsr2(sc,v) \
	    bus_space_write_2((sc)->sc_iot, (sc)->sc_ioh, SCIF_REG_SCLSR2, (v))

/*
 * InitializeScif
 * : unsigned int bps;
 * : SCIF(Serial Communication Interface)
 */

void
InitializeScif(bus_space_tag_t bt, bus_space_handle_t bh, unsigned int bps)
{

	/* Initialize SCR */
	bus_space_write_2(bt, bh, SCIF_REG_SCSCR2, 0x00);

	bus_space_write_2(bt, bh, SCIF_REG_SCFCR2,
	    SCIF_SCFCR2_TFRST | SCIF_SCFCR2_RFRST);

	/* Serial Mode Register */
	/* 8bit,NonParity,Even,1Stop */
	bus_space_write_2(bt, bh, SCIF_REG_SCSMR2, 0x00);
#if 0
	/* Bit Rate Register */
	bus_space_write_1(bt, bh, SCIF_REG_SCBRR2,
	    divrnd(cprc_clocks.cc_peripheral, 32 * bps) - 1);
#endif
	/*
	 * wait 1mSec, because Send/Recv must begin 1 bit period after
	 * BRR is set.
	 */
	delay(1000);

	bus_space_write_2(bt, bh, SCIF_REG_SCFCR2,
	    SCIF_SCFCR2_RTRG_14 | SCIF_SCFCR2_TTRG_1);

	/* Send permission, Receive permission ON */
	bus_space_write_2(bt, bh, SCIF_REG_SCSCR2,
	    SCIF_SCSCR2_TE | SCIF_SCSCR2_RE | 0x0a);

	bus_space_write_2(bt, bh, SCIF_REG_SCSPTR2, 0x85);

	/* Serial Status Register */
	/* Clear Status */
	bus_space_write_2(bt, bh, SCIF_REG_SCFSR2,
	    bus_space_read_2(bt, bh, SCIF_REG_SCFSR2) & SCIF_SCFSR2_TDFE);
}


/*
 * scif_putc
 *  : unsigned char c;
 */

static void
scif_putc(bus_space_tag_t bt, bus_space_handle_t bh, unsigned char c)
{
	u_int16_t reg;

	if (c == '\n')
		scif_putc(bt, bh, '\r');

	/* wait for ready */
	for (;;) {
		reg = bus_space_read_2(bt, bh, SCIF_REG_SCFDR2);
		reg >>= SCIF_SCFDR2_T_SHIFT;
		reg &= SCIF_SCFDR2_T_MASK;
		if (reg < 15)
			break;
	}

	/* write send data to send register */
	bus_space_write_1(bt, bh, SCIF_REG_SCFTDR2, c);

	/* clear ready flag */
	bus_space_write_2(bt, bh, SCIF_REG_SCFSR2,
	    bus_space_read_2(bt, bh, SCIF_REG_SCFSR2) &
	    ~(SCIF_SCFSR2_TDFE | SCIF_SCFSR2_TEND));
}

/*
 * scif_getc
 */
static unsigned char
scif_getc(bus_space_tag_t bt, bus_space_handle_t bh)
{
#if 0
	u_int8_t c, err_c;
	u_int16_t err_c2;

	while (1) {
		/* wait for ready */
		for (;;) {
			u_int16_t reg;

			reg = bus_space_read_2(bt, bh, SCIF_REG_SCFDR2);
			reg >>= SCIF_SCFDR2_R_SHIFT;
			reg &= SCIF_SCFDR2_R_MASK;
			if (reg)
				break;
		}

		c = bus_space_read_1(bt, bh, SCIF_REG_SCFDR2);

		err_c = bus_space_read_2(bt, bh, SCIF_REG_SCFSR2);
		bus_space_write_2(bt, bh, SCIF_REG_SCFSR2,
		    err_c & ~(SCIF_SCFSR2_ER | SCIF_SCFSR2_BRK |
			      SCIF_SCFSR2_RDF | SCIF_SCFSR2_DR));

		err_c2 = bus_space_read_2(bt, bh, SCIF_REG_SCLSR2);
		bus_space_write_2(bt, bh, SCIF_REG_SCLSR2,
		    err_c2 & ~SCIF_SCLSR2_ORER);

		if ((err_c & (SCIF_SCFSR2_ER | SCIF_SCFSR2_BRK | SCIF_SCFSR2_FER
		    | SCIF_SCFSR2_PER)) == 0) {
			if ((err_c2 & SCIF_SCLSR2_ORER) == 0)
				return(c);
		}
	}
#else
	unsigned char c;

	while (!(bus_space_read_2(bt, bh, SCIF_REG_SCFSR2) &
	    (SCIF_SCFSR2_DR | SCIF_SCFSR2_RDF)))
		;

	c = bus_space_read_1(bt, bh, SCIF_REG_SCFRD2);

	bus_space_write_2(bt, bh, SCIF_REG_SCFSR2, 0);

	return (c);
#endif
}

#if 0
#define	SCIF_MAX_UNITS 2
#else
#define	SCIF_MAX_UNITS 1
#endif


static int
scif_match(struct device *parent, struct cfdata *cf, void *args)
{
	struct pbridge_attach_args *pa = args;

	if (strcmp(pa->pa_name, scif_cd.cd_name))
		return 0;

	if ((pa->pa_ipl = cf->cf_loc[PBRIDGECF_IPL]) == PBRIDGECF_IPL_DEFAULT)
		pa->pa_ipl = IPL_SERIAL;

	if ((pa->pa_intevt = cf->cf_loc[PBRIDGECF_INTEVT]) ==
	    PBRIDGECF_INTEVT_DEFAULT)
		pa->pa_intevt = INTC_INTEVT_SCIF_ERI;

	return (1);
}

static void
scif_attach(struct device *parent, struct device *self, void *args)
{
	struct pbridge_attach_args *pa = args;
	struct scif_softc *sc = (struct scif_softc *)self;
	struct tty *tp;

	sc->sc_hwflags = 0;	/* XXX */
	sc->sc_swflags = 0;	/* XXX */
	sc->sc_fifolen = 16;

	sc->sc_iot = pa->pa_bust;
	bus_space_map(sc->sc_iot, pa->pa_offset, SCIF_REG_SZ, 0, &sc->sc_ioh);

	if (scifisconsole || kgdb_attached) {
		/* InitializeScif(scifcn_speed); */
		SET(sc->sc_hwflags, SCIF_HW_CONSOLE);
		SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);
		if (kgdb_attached) {
			SET(sc->sc_hwflags, SCIF_HW_KGDB);
			printf("\n%s: kgdb\n", sc->sc_dev.dv_xname);
		} else {
			printf("\n%s: console\n", sc->sc_dev.dv_xname);
		}
	} else {
		InitializeScif(sc->sc_iot, sc->sc_ioh, 9600);
		printf("\n");
	}

	callout_init(&sc->sc_diag_ch);

	sh5_intr_establish(pa->pa_intevt, IST_LEVEL, pa->pa_ipl,
	    scifintr, sc);
	sh5_intr_establish(pa->pa_intevt + 0x20, IST_LEVEL, pa->pa_ipl,
	    scifintr, sc);
	sh5_intr_establish(pa->pa_intevt + 0x40, IST_LEVEL, pa->pa_ipl,
	    scifintr, sc);
	sh5_intr_establish(pa->pa_intevt + 0x60, IST_LEVEL, pa->pa_ipl,
	    scifintr, sc);

	sc->sc_si = softintr_establish(IPL_SOFTSERIAL, scifsoft, sc);

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
scifstart(struct tty *tp)
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
	scif_wr_scscr2(sc, scif_rd_scscr2(sc) |
	    SCIF_SCSCR2_TIE | SCIF_SCSCR2_RIE);

	/* Output the first chunk of the contiguous buffer. */
	{
		int n;
		int max;
		int i;

		n = sc->sc_tbc;
		max = sc->sc_fifolen -
		    ((scif_rd_scfdr2(sc) >> SCIF_SCFDR2_T_SHIFT) &
		        SCIF_SCFDR2_T_MASK);
		if (n > max)
			n = max;

		for (i = 0; i < n; i++) {
			scif_putc(sc->sc_iot, sc->sc_ioh, *(sc->sc_tba));
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
	if (ISSET(t->c_cflag, CRTSCTS))
		scif_wr_scfcr2(sc, scif_rd_scfcr2(sc) | SCIF_SCFCR2_MCE);
	else
		scif_wr_scfcr2(sc, scif_rd_scfcr2(sc) & ~SCIF_SCFCR2_MCE);

	scif_wr_scbrr2(sc, divrnd(cprc_clocks.cc_peripheral, 32 * ospeed) - 1);

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
scif_iflush(struct scif_softc *sc)
{
	int i;
	unsigned char c;

	i = scif_rd_scfdr2(sc) & SCIF_SCFDR2_R_MASK;

	while (i > 0) {
		c = scif_rd_scfrd2(sc);

		scif_wr_scfsr2(sc, scif_rd_scfsr2(sc) &
		    ~(SCIF_SCFSR2_RDF | SCIF_SCFSR2_DR));

		i--;
	}
}

int
scifopen(dev_t dev, int flag, int mode, struct proc *p)
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
#endif /* KGDB */

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
		scif_wr_scscr2(sc, scif_rd_scscr2(sc) |
		    SCIF_SCSCR2_TIE | SCIF_SCSCR2_RIE);

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
scifclose(dev_t dev, int flag, int mode, struct proc *p)
{
	struct scif_softc *sc = scif_cd.cd_devs[SCIFUNIT(dev)];
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
scifread(dev_t dev, struct uio *uio, int flag)
{
	struct scif_softc *sc = scif_cd.cd_devs[SCIFUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
scifwrite(dev_t dev, struct uio *uio, int flag)
{
	struct scif_softc *sc = scif_cd.cd_devs[SCIFUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
scifpoll(dev_t dev, int events, struct proc *p)
{
	struct scif_softc *sc = scif_cd.cd_devs[SCIFUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_poll)(tp, events, p));
}

struct tty *
sciftty(dev_t dev)
{
	struct scif_softc *sc = scif_cd.cd_devs[SCIFUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return (tp);
}

int
scifioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct scif_softc *sc = scif_cd.cd_devs[SCIFUNIT(dev)];
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
		error = EPASSTHROUGH;
		break;
	}

	splx(s);

	return (error);
}

integrate void
scif_schedrx(struct scif_softc *sc)
{

	sc->sc_rx_ready = 1;

	/* Wake up the poller. */
	softintr_schedule(sc->sc_si);
}

void
scif_break(struct scif_softc *sc, int onoff)
{

	if (onoff)
		scif_wr_scfsr2(sc, scif_rd_scfsr2(sc) & ~SCIF_SCFSR2_TDFE);
	else
		scif_wr_scfsr2(sc, scif_rd_scfsr2(sc) | SCIF_SCFSR2_TDFE);

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
	    sc->sc_dev.dv_xname,
	    overflows, overflows == 1 ? "" : "s",
	    floods, floods == 1 ? "" : "s");
}

integrate void
scif_rxsoft(struct scif_softc *sc, struct tty *tp)
{
	int (*rint)(int c, struct tty *tp) = tp->t_linesw->l_rint;
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
		if (ISSET(ssr2, SCIF_SCFSR2_BRK |
		    SCIF_SCFSR2_FER | SCIF_SCFSR2_PER)) {
			if (ISSET(ssr2, SCIF_SCFSR2_BRK | SCIF_SCFSR2_FER))
				SET(code, TTY_FE);
			if (ISSET(ssr2, SCIF_SCFSR2_PER))
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
				scif_wr_scscr2(sc, scif_rd_scscr2(sc) |
				    SCIF_SCSCR2_RIE);
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
scif_txsoft(struct scif_softc *sc, struct tty *tp)
{

	CLR(tp->t_state, TS_BUSY);
	if (ISSET(tp->t_state, TS_FLUSH))
		CLR(tp->t_state, TS_FLUSH);
	else
		ndflush(&tp->t_outq, (int)(sc->sc_tba - tp->t_outq.c_cf));
	(*tp->t_linesw->l_start)(tp);
}

integrate void
scif_stsoft(struct scif_softc *sc, struct tty *tp)
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

#ifdef SCIF_DEBUG
	if (scif_debug)
		scifstatus(sc, "scif_stsoft");
#endif
#endif
}

void
scifsoft(void *arg)
{
	struct scif_softc *sc = arg;
	struct tty *tp;

	if (ISSET(sc->sc_dev.dv_flags, DVF_ACTIVE) == 0)
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

int
scifintr(void *arg)
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

	do {
		ssr2 = scif_rd_scfsr2(sc);

		if (ISSET(ssr2, SCIF_SCFSR2_BRK)) {
			scif_wr_scfsr2(sc, scif_rd_scfsr2(sc) &
			    ~(SCIF_SCFSR2_ER | SCIF_SCFSR2_BRK |
			      SCIF_SCFSR2_DR));

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
		count = scif_rd_scfdr2(sc) & SCIF_SCFDR2_R_MASK;
		if (count != 0) {
			while (1) {
				u_char c = scif_rd_scfrd2(sc);
				u_char err;

				err = (u_char) scif_rd_scfsr2(sc) & 0x00ff;

				scif_wr_scfsr2(sc, scif_rd_scfsr2(sc) &
				    ~(SCIF_SCFSR2_ER | SCIF_SCFSR2_RDF |
				      SCIF_SCFSR2_DR));

				scif_wr_sclsr2(sc, scif_rd_sclsr2(sc) &
				    ~SCIF_SCLSR2_ORER);

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
				scif_wr_scscr2(sc, scif_rd_scscr2(sc) &
				    ~SCIF_SCSCR2_RIE);
			}
		} else {
			if (scif_rd_scfsr2(sc) &
			    (SCIF_SCFSR2_RDF | SCIF_SCFSR2_DR)) {
				scif_wr_scscr2(sc, scif_rd_scscr2(sc) &
				    ~(SCIF_SCSCR2_RIE | SCIF_SCSCR2_TIE));
				delay(10);
				scif_wr_scscr2(sc, scif_rd_scscr2(sc) |
				    SCIF_SCSCR2_RIE | SCIF_SCSCR2_TIE);
				continue;
			}
		}
	} while (scif_rd_scfsr2(sc) & (SCIF_SCFSR2_RDF | SCIF_SCFSR2_DR));

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
	if (((scif_rd_scfdr2(sc) >> SCIF_SCFDR2_T_SHIFT)&SCIF_SCFDR2_T_MASK) !=
	    15) {
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
			    ((scif_rd_scfdr2(sc) >> SCIF_SCFDR2_T_SHIFT) &
				SCIF_SCFDR2_T_MASK);
			if (n > max)
				n = max;

			for (i = 0; i < n; i++) {
				scif_putc(sc->sc_iot, sc->sc_ioh,*(sc->sc_tba));
				sc->sc_tba++;
			}
			sc->sc_tbc -= n;
		} else {
			/* Disable transmit completion interrupts if necessary. */
#if 0
			if (ISSET(sc->sc_ier, IER_ETXRDY))
#endif
				scif_wr_scscr2(sc, scif_rd_scscr2(sc) &
				    ~SCIF_SCSCR2_TIE);

			if (sc->sc_tx_busy) {
				sc->sc_tx_busy = 0;
				sc->sc_tx_done = 1;
			}
		}
	}

	/* Wake up the poller. */
	softintr_schedule(sc->sc_si);

#if NRND > 0 && defined(RND_SCIF)
rnd_add_uint32(&sc->rnd_source, iir | lsr);
#endif

	return (1);
}

void
scifcnprobe(struct consdev *cp)
{
	/* Initialize required fields. */
	cp->cn_dev = makedev(cdevsw_lookup_major(&scif_cdevsw), 0);
	cp->cn_pri = CN_NORMAL;
}

void
scifcninit(struct consdev *cp)
{
	extern bus_space_tag_t _sh5_scif_bt;
	extern bus_space_handle_t _sh5_scif_bh;

	InitializeScif(_sh5_scif_bt, _sh5_scif_bh, scifcn_speed);
	scifisconsole = 1;
}

int
scifcngetc(dev_t dev)
{
	extern bus_space_tag_t _sh5_scif_bt;
	extern bus_space_handle_t _sh5_scif_bh;
	int c;
	int s;

	s = splserial();
	c = scif_getc(_sh5_scif_bt, _sh5_scif_bh);
	splx(s);

	return (c);
}

void
scifcnputc(dev_t dev, int c)
{
	extern bus_space_tag_t _sh5_scif_bt;
	extern bus_space_handle_t _sh5_scif_bh;
	int s;

	s = splserial();
	scif_putc(_sh5_scif_bt, _sh5_scif_bh, (u_char)c);
	splx(s);
}

#ifdef KGDB
int
scif_kgdb_init()
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

/*	$NetBSD: ixp12x0_com.c,v 1.1 2002/07/15 16:27:17 ichiro Exp $ */
#define POLLING_COM
/*
 * Copyright (c) 1998, 1999, 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA and Naoto Shimazaki.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
 *      @(#)com.c       7.5 (Berkeley) 5/16/91
 */


#include "opt_com.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"

#include "rnd.h"
#if NRND > 0 && defined(RND_COM)
#include <sys/rnd.h>
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/tty.h>
#include <sys/uio.h>
#include <sys/vnode.h>

#include <dev/cons.h>

#include <machine/bus.h>

#include <arm/ixp12x0/ixp12x0_comreg.h>
#include <arm/ixp12x0/ixp12x0_comvar.h>
#include <arm/ixp12x0/ixp12x0reg.h>
#include <arm/ixp12x0/ixp12x0var.h>

#include <arm/ixp12x0/ixpsipvar.h> 

#include "ixpcom.h"

#ifdef POLLING_COM
#undef CR_RIE
#define CR_RIE 0
#undef CR_XIE
#define CR_XIE 0
#endif

#ifdef NOXIE
#undef CR_XIE
#define CR_XIE 0
#endif


cdev_decl(ixpcom);

static int	ixpcom_match(struct device *, struct cfdata *, void *);
static void	ixpcom_attach(struct device *, struct device *, void *);
static int	ixpcom_detach(struct device *, int);
static int	ixpcom_activate(struct device *, enum devact);

static int	ixpcomparam(struct tty *, struct termios *);
static void	ixpcomstart(struct tty *);
static int	ixpcomhwiflow(struct tty *, int);

static void	ixpcom_attach_subr(struct ixpcom_softc *);

static u_int	cflag2cr(tcflag_t);

int             ixpcomcngetc(dev_t);
void            ixpcomcnputc(dev_t, int);
void            ixpcomcnpollc(dev_t, int);

static void	ixpcomsoft(void* arg);
inline static void	ixpcom_txsoft(struct ixpcom_softc *, struct tty *);
inline static void	ixpcom_rxsoft(struct ixpcom_softc *, struct tty *);

void            ixpcomcnprobe(struct consdev *);
void            ixpcomcninit(struct consdev *);

static int	ixpcomintr(void* arg);

u_int32_t	ixpcom_cr = 0;		/* tell cr to *_intr.c */
u_int32_t	ixpcom_imask = 0;	/* intrrupt mask from *_intr.c */

static bus_space_tag_t ixpcomconstag;
static bus_space_handle_t ixpcomconsioh;
static bus_addr_t ixpcomconsaddr = IXPCOM_UART_BASE; /* XXX initial value */

static int ixpcomconsattached;
static int ixpcomconsrate;
static tcflag_t ixpcomconscflag;

struct cfattach ixpcom_ca = {
	sizeof(struct ixpcom_softc), ixpcom_match, ixpcom_attach,
	ixpcom_detach, ixpcom_activate
};
extern struct cfdriver ixpcom_cd;

struct consdev ixpcomcons = {
	NULL, NULL, ixpcomcngetc, ixpcomcnputc, ixpcomcnpollc, NULL,
	NODEV, CN_NORMAL
};

#ifndef DEFAULT_COMSPEED
#define DEFAULT_COMSPEED 38400
#endif

#define COMUNIT_MASK    0x7ffff
#define COMDIALOUT_MASK 0x80000

#define COMUNIT(x)	(minor(x) & COMUNIT_MASK)
#define COMDIALOUT(x)	(minor(x) & COMDIALOUT_MASK)

#define COM_ISALIVE(sc)	((sc)->enabled != 0 && \
			ISSET((sc)->sc_dev.dv_flags, DVF_ACTIVE))

#define COM_BARRIER(t, h, f) bus_space_barrier((t), (h), 0, COM_NPORTS, (f))

#define	COM_LOCK(sc);
#define	COM_UNLOCK(sc);

#define SET(t, f)	(t) |= (f)
#define CLR(t, f)	(t) &= ~(f)
#define ISSET(t, f)	((t) & (f))

static int
ixpcom_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return (1);
}

void
ixpcom_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ixpcom_softc *sc = (struct ixpcom_softc *)self;
	struct ixpsip_attach_args *sa = aux;

	printf("\n");

        sc->sc_iot = sa->sa_iot;
        sc->sc_baseaddr = sa->sa_addr;

	if(bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 0,
			&sc->sc_ioh)) {
		printf("%s: unable to map registers\n", sc->sc_dev.dv_xname);
		return;
        }

	printf("%s: IXP12x0 UART\n", sc->sc_dev.dv_xname);

	ixpcom_attach_subr(sc);

#ifdef POLLING_COM
	{ void* d; d = d = ixpcomintr; }
#else
	ixp12x0_intr_establish(IXP12X0_INTR_UART, IPL_SERIAL, ixpcomintr, sc);
#endif
}

void
ixpcom_attach_subr(sc)
	struct ixpcom_softc *sc;
{
	bus_addr_t iobase = sc->sc_baseaddr;
	bus_space_tag_t iot = sc->sc_iot;
	struct tty *tp;

	if (iot == ixpcomconstag && iobase == ixpcomconsaddr) {
		ixpcomconsattached = 1;
		sc->sc_speed = IXPCOMSPEED(ixpcomconsrate);

		/* Make sure the console is always "hardwired". */
		delay(10000);	/* wait for output to finish */
		SET(sc->sc_hwflags, COM_HW_CONSOLE);
		SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);
	}

	tp = ttymalloc();
	tp->t_oproc = ixpcomstart;
	tp->t_param = ixpcomparam;
	tp->t_hwiflow = ixpcomhwiflow;

	sc->sc_tty = tp;
	sc->sc_rbuf = malloc(IXPCOM_RING_SIZE << 1, M_DEVBUF, M_NOWAIT);
	sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
	sc->sc_rbavail = IXPCOM_RING_SIZE;
	if (sc->sc_rbuf == NULL) {
		printf("%s: unable to allocate ring buffer\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	sc->sc_ebuf = sc->sc_rbuf + (IXPCOM_RING_SIZE << 1);
	sc->sc_tbc = 0;

	sc->sc_rie = sc->sc_xie = 0;
	ixpcom_cr = IXPCOMSPEED2BRD(DEFAULT_COMSPEED)
		| CR_UE | sc->sc_rie | sc->sc_xie | DSS_8BIT;

	tty_attach(tp);

	if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
		int maj;

		/* locate the major number */
		for (maj = 0; maj < nchrdev; maj++)
			if (cdevsw[maj].d_open == ixpcomopen)
				break;

		cn_tab->cn_dev = makedev(maj, sc->sc_dev.dv_unit);

		delay(10000); /* XXX */
		printf("%s: console\n", sc->sc_dev.dv_xname);
		delay(10000); /* XXX */
	}

	sc->sc_si = softintr_establish(IPL_SOFTSERIAL, ixpcomsoft, sc);

#if NRND > 0 && defined(RND_COM)
	rnd_attach_source(&sc->rnd_source, sc->sc_dev.dv_xname,
			  RND_TYPE_TTY, 0);
#endif

	/* if there are no enable/disable functions, assume the device
	   is always enabled */
	if (!sc->enable)
		sc->enabled = 1;

	/* XXX configure register */
	/* xxx_config(sc) */

	SET(sc->sc_hwflags, COM_HW_DEV_OK);
}

int
ixpcom_detach(self, flags)
	struct device *self;
	int flags;
{
	struct ixpcom_softc *sc = (struct ixpcom_softc *)self;
	int maj, mn;

	/* locate the major number */
	for (maj = 0; maj < nchrdev; maj++)
		if (cdevsw[maj].d_open == ixpcomopen)
			break;

	/* Nuke the vnodes for any open instances. */
	mn = self->dv_unit;
	vdevgone(maj, mn, mn, VCHR);

	mn |= COMDIALOUT_MASK;
	vdevgone(maj, mn, mn, VCHR);

	/* Free the receive buffer. */
	free(sc->sc_rbuf, M_DEVBUF);

	/* Detach and free the tty. */
	tty_detach(sc->sc_tty);
	ttyfree(sc->sc_tty);

#if NRND > 0 && defined(RND_COM)
	/* Unhook the entropy source. */
	rnd_detach_source(&sc->rnd_source);
#endif

	return (0);
}

int
ixpcom_activate(self, act)
	struct device *self;
	enum devact act;
{
	struct ixpcom_softc *sc = (struct ixpcom_softc *)self;
	int s, rv = 0;

	s = splserial();
	COM_LOCK(sc);
	switch (act) {
	case DVACT_ACTIVATE:
		rv = EOPNOTSUPP;
		break;
	case DVACT_DEACTIVATE:
		if (sc->sc_hwflags & (COM_HW_CONSOLE|COM_HW_KGDB))
			rv = EBUSY;
		break;
	}

	COM_UNLOCK(sc); 
	splx(s);
	return (rv);
}

int
ixpcomparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
	return (0);
}

int
ixpcomhwiflow(tp, block)
	struct tty *tp;
	int block;
{
	return (0);
}

void
ixpcomstart(tp)
	struct tty *tp;
{
	return;
}

int
ixpcomopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	return (0);
}
static u_int
cflag2cr(cflag)
	tcflag_t cflag;
{
	u_int cr;

	cr  = (cflag & PARENB) ? CR_PE : 0;
	cr |= (cflag & PARODD) ? CR_OES : 0;
	cr |= (cflag & CSTOPB) ? CR_SBS : 0;
	cr |= ((cflag & CSIZE) == CS8) ? DSS_8BIT : 0;
	
	return (cr);
}

static void
ixpcom_loadchannelregs(sc)
	struct ixpcom_softc *sc;
{
	/* XXX */
	ixpcom_cr &= ~(CR_RIE | CR_XIE);
	ixpcom_cr |= sc->sc_rie | sc->sc_xie;
	ixpcom_cr &= ixpcom_imask;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IXPCOM_CR, ixpcom_cr);
}

/* Initialization for serial console */
int
ixpcominit(iot, iobase, baud, cflag, iohp)
	bus_space_tag_t iot;
	bus_addr_t iobase;
	int baud;
	tcflag_t cflag;
	bus_space_handle_t *iohp;
{
	int cr;

	if (bus_space_map(iot, iobase, IXPCOM_UART_SIZE, 0, iohp))
		printf("register map failed\n");

	cr = cflag2cr(cflag);
	cr |= (IXPCOMSPEED(baud) << 16);
#if 0
	cr |= (CR_UE | CR_RIE | CR_XIE);
#endif
	cr |= CR_UE;
	ixpcom_cr = cr;

	/* enable the UART */
	bus_space_write_4(iot, *iohp, IXPCOM_CR, cr);

	return (0);
}

int
ixpcomcnattach(iot, iobase, rate, cflag)
	bus_space_tag_t iot;
	bus_addr_t iobase;
	int rate;
	tcflag_t cflag;
{
	int res;

	if ((res = ixpcominit(iot, iobase, rate, cflag, &ixpcomconsioh)))
		return (res);
	cn_tab = &ixpcomcons;

	ixpcomconstag = iot;
	ixpcomconsaddr = iobase;
	ixpcomconsrate = rate;
	ixpcomconscflag = cflag;

	return (0);
}

#if 0
void
ixpcomcninit(cp)
	struct consdev *cp;
{
	if (cp == NULL) {
		/* XXX cp == NULL means that MMU is disabled. */
		ixpcomconsioh = IXPCOM_UART_BASE;
		ixpcomconstag = &ixpcom_bs_tag;
		cn_tab = &ixpcomcons;

		IXPREG(IXPCOM_UART_BASE + IXPCOM_CR)
			= (IXPCOMSPEED(38400) << 16)
			| DSS_8BIT
			| (CR_UE | CR_RIE | CR_XIE);
		return;
	}

	if (ixpcominit(&ixpcom_bs_tag, CONADDR, CONSPEED,
                          CONMODE, &ixpcomconsioh))
		panic("can't init serial console @%x", CONADDR);
	cn_tab = &ixpcomcons;
	ixpcomconstag = &ixpcom_bs_tag;
}
#endif

void
ixpcomcnprobe(cp)
	struct consdev *cp;
{
	cp->cn_pri = CN_REMOTE;
}

void
ixpcomcnpollc(dev, on)
	dev_t dev;
	int on;
{
}

void
ixpcomcnputc(dev, c)
	dev_t dev;
	int c;
{
	int s;

	s = spltty();   /* XXX do we need this? */

	while(!(bus_space_read_4(ixpcomconstag, ixpcomconsioh, IXPCOM_SR)
	    & SR_TXR))
		;

	bus_space_write_4(ixpcomconstag, ixpcomconsioh, IXPCOM_DR, c);
	splx(s);
}

int
ixpcomcngetc(dev)
        dev_t dev;
{
        int c, s;

        s = spltty();   /* XXX do we need this? */

	while(!(bus_space_read_4(ixpcomconstag, ixpcomconsioh, IXPCOM_SR)
	    & SR_RXR))
		;

	c = bus_space_read_4(ixpcomconstag, ixpcomconsioh, IXPCOM_DR);
	c &= 0xff;
	splx(s);

	return (c);
}

inline static void
ixpcom_txsoft(sc, tp)
	struct ixpcom_softc *sc;
	struct tty *tp;
{
}

inline static void
ixpcom_rxsoft(sc, tp)
	struct ixpcom_softc *sc;
	struct tty *tp;
{
	int (*rint) __P((int c, struct tty *tp)) = tp->t_linesw->l_rint;
	u_char *get, *end;
	u_int cc, scc;
	u_char lsr;
	int code;
	int s;

	end = sc->sc_ebuf;
	get = sc->sc_rbget;
	scc = cc = IXPCOM_RING_SIZE - sc->sc_rbavail;
#if 0
	if (cc == IXPCOM_RING_SIZE) {
		sc->sc_floods++;
		if (sc->sc_errors++ == 0)
			callout_reset(&sc->sc_diag_callout, 60 * hz,
			    comdiag, sc);
	}
#endif
	while (cc) {
		code = get[0];
		lsr = get[1];
		if (ISSET(lsr, DR_ROR | DR_FRE | DR_PRE)) {
#if 0
			if (ISSET(lsr, DR_ROR)) {
				sc->sc_overflows++;
				if (sc->sc_errors++ == 0)
					callout_reset(&sc->sc_diag_callout,
					    60 * hz, comdiag, sc);
			}
#endif
			if (ISSET(lsr, DR_FRE))
				SET(code, TTY_FE);
			if (ISSET(lsr, DR_PRE))
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
					get -= IXPCOM_RING_SIZE << 1;
				cc = 0;
			} else {
				/*
				 * Don't schedule any more receive processing
				 * until the line discipline tells us there's
				 * space available (through comhwiflow()).
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
		COM_LOCK(sc);
		
		cc = sc->sc_rbavail += scc - cc;
		/* Buffers should be ok again, release possible block. */
		if (cc >= 1) {
			if (ISSET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
				SET(sc->sc_rie, CR_RIE);
				ixpcom_loadchannelregs(sc);
			}
			if (ISSET(sc->sc_rx_flags, RX_IBUF_BLOCKED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_BLOCKED);
#if 0
				com_hwiflow(sc);
#endif
			}
		}
		COM_UNLOCK(sc);
		splx(s);
	}
}

static void
ixpcomsoft(void* arg)
{
	struct ixpcom_softc *sc = arg;

	if (COM_ISALIVE(sc) == 0)
		return;

	if (sc->sc_rx_ready) {
		sc->sc_rx_ready = 0;
		ixpcom_rxsoft(sc, sc->sc_tty);
	}
	if (sc->sc_tx_done) {
		sc->sc_tx_done = 0;
		ixpcom_txsoft(sc, sc->sc_tty);
	}
}

static int
ixpcomintr(void* arg)
{
	struct ixpcom_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_char *put, *end;
	u_int cc;
	u_int cr;
	u_int sr;
	u_int32_t c;

	if (COM_ISALIVE(sc) == 0)
		return (0);

	COM_LOCK(sc);
	cr = bus_space_read_4(iot, ioh, IXPCOM_CR) & CR_UE;

	if (!cr) {
		COM_UNLOCK(sc);
		return (0);
	}

	sr = bus_space_read_4(iot, ioh, IXPCOM_SR);
	if (!ISSET(sr, SR_TXR | SR_RXR))
		return (0);

#if 0
	/*
	 * IPX12x0 doesn't have a "Receiver End of Break Status" bit
	 * in status registar.  Currentry I have no idea to determine
	 * whether break signal is received.
	 */
	if (XXX) {
		bus_space_write_4(iot, ioh, SACOM_SR0, SR0_REB);
#if defined(DDB) || defined(KGDB)
#ifndef DDB_BREAK_CHAR
		if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
			console_debugger();
		}
#endif
#endif /* DDB || KGDB */
	}
#endif

	end = sc->sc_ebuf;
	put = sc->sc_rbput;
	cc = sc->sc_rbavail;

	if (ISSET(sr, SR_RXR)) {
		if (!ISSET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED)) {
			while (cc > 0) {
				if (!ISSET(sr, SR_RXR))
					break;
				c = bus_space_read_4(iot, ioh, IXPCOM_DR);
				put[0] = c & 0xff;
				put[1] = (c >> 8) & 0xff;
#if defined(DDB) && defined(DDB_BREAK_CHAR)
				if (put[0] == DDB_BREAK_CHAR &&
				    ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
					console_debugger();

					sr = bus_space_read_4(iot, ioh,
							      IXPCOM_SR);
					continue;
				}
#endif
				put += 2;
				if (put >= end)
					put = sc->sc_rbuf;
				cc--;

				sr = bus_space_read_4(iot, ioh, IXPCOM_SR);
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

			/* XXX do RX hardware flow control */

			/*
			 * If we're out of space, disable receive interrupts
			 * until the queue has drained a bit.
			 */
			if (!cc) {
				SET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
				CLR(sc->sc_rie, CR_RIE);
				ixpcom_loadchannelregs(sc);
			}
		} else {
#ifdef DIAGNOSTIC
			panic("ixpcomintr: we shouldn't reach here\n");
#endif
			CLR(sc->sc_rie, CR_RIE);
			ixpcom_loadchannelregs(sc);
		}
	}

	/*
	 * Done handling any receive interrupts. See if data can be
	 * transmitted as well. Schedule tx done event if no data left
	 * and tty was marked busy.
	 */
#if 0
	sr = bus_space_read_4(iot, ioh, IXPCOM_SR);
	if (ISSET(sr, SR_TXR)) {
		/*
		 * If we've delayed a parameter change, do it now, and restart
		 * output.
		 * XXX sacom_loadchanelregs() waits TX completion,
		 * XXX resulting in ~0.1s hang (300bps, 4 bytes) in worst case
		 */
		if (sc->sc_heldchange) {
			ixpcom_loadparams(sc);
			sc->sc_heldchange = 0;
			sc->sc_tbc = sc->sc_heldtbc;
			sc->sc_heldtbc = 0;
		}

		/* Output the next chunk of the contiguous buffer, if any. */
		if (sc->sc_tbc > 0) {
			sacom_filltx(sc);
		} else {
			/* Disable transmit completion interrupts if necessary. */
			if (ISSET(sc->sc_cr3, CR3_XIE)) {
				CLR(sc->sc_cr3, CR3_XIE);
				bus_space_write_4(iot, ioh, SACOM_CR3,
						  sc->sc_cr3);
			}
			if (sc->sc_tx_busy) {
				sc->sc_tx_busy = 0;
				sc->sc_tx_done = 1;
			}
		}
	}
#endif
	COM_UNLOCK(sc);

	/* Wake up the poller. */
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	softintr_schedule(sc->sc_si);
#else
	setsoftserial();
#endif

#if NRND > 0 && defined(RND_COM)
	rnd_add_uint32(&sc->rnd_source, iir | lsr);
#endif
	return (1);
}

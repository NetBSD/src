/*	$Id: at91usart.c,v 1.5.2.3 2013/01/16 05:32:45 yamt Exp $	*/
/*	$NetBSD: at91usart.c,v 1.5.2.3 2013/01/16 05:32:45 yamt Exp $ */

/*
 * Copyright (c) 2007 Embedtronics Oy. All rights reserved.
 *
 * Copyright (c) 1998, 1999, 2001, 2002, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jesse Off
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
 *      @(#)com.c       7.5 (Berkeley) 5/16/91
 */

/*
 * TODO: hardware flow control
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: at91usart.c,v 1.5.2.3 2013/01/16 05:32:45 yamt Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"

#include "rnd.h"
#ifdef RND_COM
#include <sys/rnd.h>
#endif

#ifdef	NOTYET
/*
 * Override cnmagic(9) macro before including <sys/systm.h>.
 * We need to know if cn_check_magic triggered debugger, so set a flag.
 * Callers of cn_check_magic must declare int cn_trapped = 0;
 * XXX: this is *ugly*!
 */
#define cn_trap()				\
	do {					\
		console_debugger();		\
		cn_trapped = 1;			\
	} while (/* CONSTCOND */ 0)
#endif	/* NOTYET */


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/tty.h>
#include <sys/uio.h>
#include <sys/vnode.h>
#include <sys/kauth.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <arm/at91/at91reg.h>
#include <arm/at91/at91var.h>
#include <arm/at91/at91usartreg.h>
#include <arm/at91/at91usartvar.h>

#include <dev/cons.h>

static int	at91usart_param(struct tty *, struct termios *);
static void	at91usart_start(struct tty *);
static int	at91usart_hwiflow(struct tty *, int);

#if 0
static u_int	cflag2lcrhi(tcflag_t);
#endif
static void	at91usart_set(struct at91usart_softc *);

#if	NOTYET
int             at91usart_cn_getc(dev_t);
void            at91usart_cn_putc(dev_t, int);
void            at91usart_cn_pollc(dev_t, int);
void            at91usart_cn_probe(struct consdev *);
void            at91usart_cn_init(struct consdev *);

static struct at91usart_cons_softc {
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_hwbase;
	int			sc_ospeed;
	tcflag_t		sc_cflag;
	int			sc_attached;

	uint8_t			*sc_rx_ptr;
	uint8_t			sc_rx_fifo[64];
} usart_cn_sc;

static struct cnm_state at91usart_cnm_state;
#endif	/* NOTYET */

static void	at91usart_soft(void* arg);
inline static void	at91usart_txsoft(struct at91usart_softc *, struct tty *);
inline static void	at91usart_rxsoft(struct at91usart_softc *, struct tty *, unsigned csr);

#define	PDC_BLOCK_SIZE	64

//CFATTACH_DECL_NEW(at91usart, sizeof(struct at91usart_softc),
//	      at91usart_match, at91usart_attach, NULL, NULL);

//#define	USART_DEBUG	10

#ifdef	USART_DEBUG
int usart_debug = USART_DEBUG;
#define	DPRINTFN(n,fmt) if (usart_debug >= (n)) printf fmt
#else
#define	DPRINTFN(n,fmt)
#endif

extern struct cfdriver at91usart_cd;

dev_type_open(at91usart_open);
dev_type_close(at91usart_close);
dev_type_read(at91usart_read);
dev_type_write(at91usart_write);
dev_type_ioctl(at91usart_ioctl);
dev_type_stop(at91usart_stop);
dev_type_tty(at91usart_tty);
dev_type_poll(at91usart_poll);

const struct cdevsw at91usart_cdevsw = {
	at91usart_open, at91usart_close, at91usart_read, at91usart_write, at91usart_ioctl,
	at91usart_stop, at91usart_tty, at91usart_poll, nommap, ttykqfilter, D_TTY
};

#if	NOTYET
struct consdev at91usart_cons = {
	at91usart_cn_probe, NULL, at91usart_cn_getc, at91usart_cn_putc, at91usart_cn_pollc, NULL,
	NULL, NULL, NODEV, CN_REMOTE
};
#endif	/* NOTYET */

#ifndef DEFAULT_COMSPEED
#define DEFAULT_COMSPEED 115200
#endif

#define COMUNIT_MASK    0x7ffff
#define COMDIALOUT_MASK 0x80000

#define COMUNIT(x)	(minor(x) & COMUNIT_MASK)
#define COMDIALOUT(x)	(minor(x) & COMDIALOUT_MASK)

#define COM_ISALIVE(sc)	((sc)->enabled != 0 && device_is_active((sc)->sc_dev))

static inline void
at91usart_writereg(struct at91usart_softc *sc, int reg, u_int val)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, reg, val);
}

static inline u_int
at91usart_readreg(struct at91usart_softc *sc, int reg)
{
	return bus_space_read_4(sc->sc_iot, sc->sc_ioh, reg);
}
#if 0
static int
at91usart_match(device_t parent, cfdata_t cf, void *aux)
{
	if (strcmp(cf->cf_name, "at91usart") == 0)
		return 1;
	return 0;
}
#endif
static int at91usart_intr(void* arg);

void
at91usart_attach_subr(struct at91usart_softc *sc, struct at91bus_attach_args *sa)
{
	struct tty *tp;
	int err;

	printf("\n");

	if (bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 0, &sc->sc_ioh))
		panic("%s: Cannot map registers", device_xname(sc->sc_dev));

	sc->sc_iot = sa->sa_iot;
	sc->sc_hwbase = sa->sa_addr;
	sc->sc_dmat = sa->sa_dmat;
	sc->sc_pid = sa->sa_pid;

	/* allocate fifos */
	err = at91pdc_alloc_fifo(sc->sc_dmat, &sc->sc_rx_fifo, AT91USART_RING_SIZE, BUS_DMA_READ | BUS_DMA_STREAMING);
	if (err)
		panic("%s: cannot allocate rx fifo", device_xname(sc->sc_dev));

	err = at91pdc_alloc_fifo(sc->sc_dmat, &sc->sc_tx_fifo, AT91USART_RING_SIZE, BUS_DMA_WRITE | BUS_DMA_STREAMING);
	if (err)
		panic("%s: cannot allocate tx fifo", device_xname(sc->sc_dev));

	/* initialize uart */
	at91_peripheral_clock(sc->sc_pid, 1);

	at91usart_writereg(sc, US_IDR, -1);
	at91usart_writereg(sc, US_RTOR, 12);	// 12-bit timeout
	at91usart_writereg(sc, US_PDC + PDC_PTCR, PDC_PTCR_TXTDIS | PDC_PTCR_RXTDIS);
	at91_intr_establish(sa->sa_pid, IPL_TTY, INTR_HIGH_LEVEL, at91usart_intr, sc);
	USART_INIT(sc, 115200U);

#ifdef	NOTYET
	if (sc->sc_iot == usart_cn_sc.sc_iot
	    && sc->sc_hwbase == usart_cn_sc.sc_hwbase) {
		usart_cn_sc.sc_attached = 1;
		/* Make sure the console is always "hardwired". */
		delay(10000);	/* wait for output to finish */
		SET(sc->sc_hwflags, COM_HW_CONSOLE);
		SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);
		SET(sc->sc_ier, USART_INT_RXRDY);
		USARTREG(USART_IER) = USART_INT_RXRDY; // @@@@@
	}
#endif	// NOTYET

	tp = tty_alloc();
	tp->t_oproc = at91usart_start;
	tp->t_param = at91usart_param;
	tp->t_hwiflow = at91usart_hwiflow;

	sc->sc_tty = tp;

	tty_attach(tp);

#if	NOTYET
	if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
		int maj;

		/* locate the major number */
		maj = cdevsw_lookup_major(&at91usart_cdevsw);

		cn_tab->cn_dev = makedev(maj, device_unit(sc->sc_dev));

		aprint_normal("%s: console (maj %u  min %u  cn_dev %u)\n",
		    device_xname(sc->sc_dev), maj, device_unit(sc->sc_dev),
		    cn_tab->cn_dev);
	}
#endif	/* NOTYET */

	sc->sc_si = softint_establish(SOFTINT_SERIAL, at91usart_soft, sc);

#ifdef RND_COM
	rnd_attach_source(&sc->rnd_source, device_xname(sc->sc_dev),
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

static int
at91usart_param(struct tty *tp, struct termios *t)
{
	struct at91usart_softc *sc
		= device_lookup_private(&at91usart_cd, COMUNIT(tp->t_dev));
	int s;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);

	if (t->c_ispeed && t->c_ispeed != t->c_ospeed)
		return (EINVAL);

	/*
	 * For the console, always force CLOCAL and !HUPCL, so that the port
	 * is always active.
	 */
	if (ISSET(sc->sc_swflags, TIOCFLAG_SOFTCAR) ||
	    ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
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

	s = spltty();

	sc->sc_brgr = (AT91_MSTCLK / 16 + t->c_ospeed / 2) / t->c_ospeed;
	
	/* And copy to tty. */
	tp->t_ispeed = 0;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	at91usart_set(sc);

	splx(s);

	/*
	 * Update the tty layer's idea of the carrier bit.
	 * We tell tty the carrier is always on.
	 */
	(void) (*tp->t_linesw->l_modem)(tp, 1);

#ifdef COM_DEBUG
	if (com_debug)
		comstatus(sc, "comparam ");
#endif

	/* tell the upper layer about hwflow.. */
	if (sc->hwflow)
		(*sc->hwflow)(sc, t->c_cflag);

	return (0);
}

static int
at91usart_hwiflow(struct tty *tp, int block)
{
	if (block) {
		/* tty discipline wants to block */
	} else {
		/* tty discipline wants to unblock */
	}
	return (0);
}

static __inline void
at91usart_start_tx(struct at91usart_softc *sc)
{
	if (!sc->start_tx)
		at91usart_writereg(sc, US_PDC + PDC_PTCR, PDC_PTCR_TXTEN);
	else
		(*sc->start_tx)(sc);
}

static __inline void
at91usart_stop_tx(struct at91usart_softc *sc)
{
	if (!sc->stop_tx)
		at91usart_writereg(sc, US_PDC + PDC_PTCR, PDC_PTCR_TXTDIS);
	else
		(*sc->stop_tx)(sc);
}

static __inline void
at91usart_rx_started(struct at91usart_softc *sc)
{
	if (sc->rx_started)
		(*sc->rx_started)(sc);
}

static __inline void
at91usart_rx_stopped(struct at91usart_softc *sc)
{
	if (sc->rx_stopped)
		(*sc->rx_stopped)(sc);
}

static __inline void
at91usart_rx_rts_ctl(struct at91usart_softc *sc, int enabled)
{
	if (sc->rx_rts_ctl)
		(*sc->rx_rts_ctl)(sc, enabled);
}

static void
at91usart_filltx(struct at91usart_softc *sc)
{
	struct tty *tp = sc->sc_tty;
	int len;
	void *dst;

	// post write handler
	AT91PDC_FIFO_POSTWRITE(sc->sc_iot, sc->sc_ioh, sc->sc_dmat, US_PDC,
				&sc->sc_tx_fifo);

	// copy more data to fifo:
	if (sc->sc_tbc > 0
	    && (dst = AT91PDC_FIFO_WRPTR(&sc->sc_tx_fifo, &len)) != NULL) {
		// copy data to fifo
		if (len > sc->sc_tbc)
			len = sc->sc_tbc;
		memcpy(dst, sc->sc_tba, len);
		sc->sc_tba += len;
		if ((sc->sc_tbc -= len) <= 0)
			CLR(tp->t_state, TS_BUSY);
		// update fifo
		AT91PDC_FIFO_WRITTEN(&sc->sc_tx_fifo, len);
		// tell tty interface we've sent some bytes
		ndflush(&tp->t_outq, len);
	}

	// start sending data...
	if (AT91PDC_FIFO_PREWRITE(sc->sc_iot, sc->sc_ioh, sc->sc_dmat,
				   US_PDC, &sc->sc_tx_fifo, PDC_BLOCK_SIZE)) {
		at91usart_start_tx(sc);
		SET(sc->sc_ier, US_CSR_TXEMPTY | US_CSR_ENDTX);
	} else {
		CLR(sc->sc_ier, US_CSR_ENDTX);
	}
}

static void
at91usart_start(struct tty *tp)
{
	struct at91usart_softc *sc
		= device_lookup_private(&at91usart_cd, COMUNIT(tp->t_dev));
	int s;

	if (COM_ISALIVE(sc) == 0) {
		DPRINTFN(5, ("%s: %s / COM_ISALIVE == 0\n", device_xname(sc->sc_dev), __FUNCTION__));
		return;
	}

	s = spltty();
	if (ISSET(tp->t_state, TS_BUSY | TS_TIMEOUT | TS_TTSTOP)) {
		DPRINTFN(5, ("%s: %s: TS_BUSY || TS_TIMEOUT || TS_TTSTOP\n", device_xname(sc->sc_dev), __FUNCTION__));
		goto out;
	}

	if (!ttypull(tp))
		goto out;

	/* Grab the first contiguous region of buffer space. */
	{
		u_char *tba;
		int tbc;

		tba = tp->t_outq.c_cf;
		tbc = ndqb(&tp->t_outq, 0);

		sc->sc_tba = tba;
		sc->sc_tbc = tbc;
	}

	SET(tp->t_state, TS_BUSY);

	/* Output the first chunk of the contiguous buffer. */
	at91usart_filltx(sc);
	at91usart_writereg(sc, US_IER, sc->sc_ier);
	DPRINTFN(5, ("%s: %s, ier=%08x (csr=%08x)\n", device_xname(sc->sc_dev), __FUNCTION__, sc->sc_ier, at91usart_readreg(sc, US_CSR)));

out:
	splx(s);

	return;
}

static __inline__ void
at91usart_break(struct at91usart_softc *sc, int onoff)
{
	at91usart_writereg(sc, US_CR, onoff ? US_CR_STTBRK : US_CR_STPBRK);
}

static void
at91usart_shutdown(struct at91usart_softc *sc)
{
	int s;

	s = spltty();

	/* turn of dma */
	at91usart_writereg(sc, US_PDC + PDC_PTCR, PDC_PTCR_TXTDIS | PDC_PTCR_RXTDIS);
	at91usart_writereg(sc, US_PDC + PDC_TNCR, 0);
	at91usart_writereg(sc, US_PDC + PDC_TCR, 0);
	at91usart_writereg(sc, US_PDC + PDC_RNCR, 0);
	at91usart_writereg(sc, US_PDC + PDC_RCR, 0);

	/* Turn off interrupts. */
	at91usart_writereg(sc, US_IDR, -1);

	/* Clear any break condition set with TIOCSBRK. */
	at91usart_break(sc, 0);
	at91usart_set(sc);

	if (sc->disable) {
#ifdef DIAGNOSTIC
		if (!sc->enabled)
			panic("at91usart_shutdown: not enabled?");
#endif
		(*sc->disable)(sc);
		sc->enabled = 0;
	}
	splx(s);
}

int
at91usart_open(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct at91usart_softc *sc;
	struct tty *tp;
	int s;
	int error;

	sc = device_lookup_private(&at91usart_cd, COMUNIT(dev));
	if (sc == NULL || !ISSET(sc->sc_hwflags, COM_HW_DEV_OK))
		return (ENXIO);

	if (!device_is_active(sc->sc_dev))
		return (ENXIO);

#ifdef KGDB
	/*
	 * If this is the kgdb port, no other use is permitted.
	 */
	if (ISSET(sc->sc_hwflags, COM_HW_KGDB))
		return (EBUSY);
#endif

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

		if (sc->enable) {
			if ((*sc->enable)(sc)) {
				splx(s);
				printf("%s: device enable failed\n",
				       device_xname(sc->sc_dev));
				return (EIO);
			}
			sc->enabled = 1;
#if 0
/* XXXXXXXXXXXXXXX */
			com_config(sc);
#endif
		}

		/* reset fifos: */
		AT91PDC_RESET_FIFO(sc->sc_iot, sc->sc_ioh, sc->sc_dmat, US_PDC, &sc->sc_rx_fifo, 0);
		AT91PDC_RESET_FIFO(sc->sc_iot, sc->sc_ioh, sc->sc_dmat, US_PDC, &sc->sc_tx_fifo, 1);

		/* reset receive */
		at91usart_writereg(sc, US_CR, US_CR_RSTSTA | US_CR_STTTO);

		/* Turn on interrupts. */
		sc->sc_ier = US_CSR_ENDRX|US_CSR_RXBUFF|US_CSR_TIMEOUT|US_CSR_RXBRK;
		at91usart_writereg(sc, US_IER, sc->sc_ier);

		/* enable DMA: */
		at91usart_writereg(sc, US_PDC + PDC_PTCR, PDC_PTCR_RXTEN);

		/*
		 * Initialize the termios status to the defaults.  Add in the
		 * sticky bits from TIOCSFLAGS.
		 */
		t.c_ispeed = 0;
/*		if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
			t.c_ospeed = usart_cn_sc.sc_ospeed;
			t.c_cflag = usart_cn_sc.sc_cflag;
		} else*/ {
			t.c_ospeed = TTYDEF_SPEED;
			t.c_cflag = TTYDEF_CFLAG;
		}
		if (ISSET(sc->sc_swflags, TIOCFLAG_CLOCAL))
			SET(t.c_cflag, CLOCAL);
		if (ISSET(sc->sc_swflags, TIOCFLAG_CRTSCTS))
			SET(t.c_cflag, CRTSCTS);
		if (ISSET(sc->sc_swflags, TIOCFLAG_MDMBUF))
			SET(t.c_cflag, MDMBUF);

		/* Make sure at91usart_param() will do something. */
		tp->t_ospeed = 0;
		(void) at91usart_param(tp, &t);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		ttychars(tp);
		ttsetwater(tp);

		/* and unblock. */
		CLR(sc->sc_rx_flags, RX_ANY_BLOCK);

#ifdef COM_DEBUG
		if (at91usart_debug)
			comstatus(sc, "at91usart_open  ");
#endif

	}
	
	splx(s);

	error = ttyopen(tp, COMDIALOUT(dev), ISSET(flag, O_NONBLOCK));
	if (error)
		goto bad;

	error = (*tp->t_linesw->l_open)(dev, tp);
	if (error)
		goto bad;

	return (0);

bad:
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * We failed to open the device, and nobody else had it opened.
		 * Clean up the state as appropriate.
		 */
		at91usart_shutdown(sc);
	}

	return (error);
}

int
at91usart_close(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct at91usart_softc *sc = device_lookup_private(&at91usart_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	/* XXX This is for cons.c. */
	if (!ISSET(tp->t_state, TS_ISOPEN))
		return (0);

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);

	if (COM_ISALIVE(sc) == 0)
		return (0);

	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * Although we got a last close, the device may still be in
		 * use; e.g. if this was the dialout node, and there are still
		 * processes waiting for carrier on the non-dialout node.
		 */
		at91usart_shutdown(sc);
	}

	return (0);
}

int
at91usart_read(dev_t dev, struct uio *uio, int flag)
{
	struct at91usart_softc *sc = device_lookup_private(&at91usart_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);
 
	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
at91usart_write(dev_t dev, struct uio *uio, int flag)
{
	struct at91usart_softc *sc = device_lookup_private(&at91usart_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);
 
	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
at91usart_poll(dev_t dev, int events, struct lwp *l)
{
	struct at91usart_softc *sc = device_lookup_private(&at91usart_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);
 
	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

struct tty *
at91usart_tty(dev_t dev)
{
	struct at91usart_softc *sc = device_lookup_private(&at91usart_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	return (tp);
}

int
at91usart_ioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct at91usart_softc *sc = device_lookup_private(&at91usart_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;
	int error;
	int s;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	error = ttioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	error = 0;

	s = spltty();

	switch (cmd) {
	case TIOCSBRK:
		at91usart_break(sc, 1);
		break;

	case TIOCCBRK:
		at91usart_break(sc, 0);
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

/*
 * Stop output on a line.
 */
void
at91usart_stop(struct tty *tp, int flag)
{
	struct at91usart_softc *sc
		= device_lookup_private(&at91usart_cd, COMUNIT(tp->t_dev));
	int s;

	s = spltty();
	if (ISSET(tp->t_state, TS_BUSY)) {
		/* Stop transmitting at the next chunk. */
		sc->sc_tbc = 0;
		if (!ISSET(tp->t_state, TS_TTSTOP))
			SET(tp->t_state, TS_FLUSH);
	}
	splx(s);
}

#if 0
static u_int
cflag2lcrhi(tcflag_t cflag)
{
	uint32_t	mr;

	switch (cflag & CSIZE) {
	default:
		mr = 0x0;
		break;
	}
#if 0
	mr |= (cflag & PARENB) ? LinCtrlHigh_PEN : 0;
	mr |= (cflag & PARODD) ? 0 : LinCtrlHigh_EPS;
	mr |= (cflag & CSTOPB) ? LinCtrlHigh_STP2 : 0;
	mr |= LinCtrlHigh_FEN;  /* FIFO always enabled */
#endif
	mr |= USART_MR_PAR_NONE;
	return (mr);
}
#endif


static void
at91usart_set(struct at91usart_softc *sc)
{
	at91usart_writereg(sc, US_MR, US_MR_CHRL_8 | US_MR_PAR_NONE | US_MR_NBSTOP_1);
	at91usart_writereg(sc, US_BRGR, sc->sc_brgr);
	at91usart_writereg(sc, US_CR, US_CR_TXEN | US_CR_RXEN); // @@@ just in case
}

#if	NOTYET
int
at91usart_cn_attach(bus_space_tag_t iot, bus_addr_t iobase, bus_space_handle_t ioh,
		    uint32_t mstclk, int ospeed, tcflag_t cflag)
{
	cn_tab = &at91usart_cons;
	cn_init_magic(&at91usart_cnm_state);
	cn_set_magic("\047\001");

	usart_cn_sc.sc_iot = iot;
	usart_cn_sc.sc_ioh = ioh;
	usart_cn_sc.sc_hwbase = iobase;
	usart_cn_sc.sc_ospeed = ospeed;
	usart_cn_sc.sc_cflag = cflag;

	USART_INIT(mstclk, ospeed);

	return (0);
}

void
at91usart_cn_probe(struct consdev *cp)
{
	cp->cn_pri = CN_REMOTE;
}

void
at91usart_cn_pollc(dev_t dev, int on)
{
	if (on) {
		// enable polling mode
		USARTREG(US_IDR) = USART_INT_RXRDY;
	} else {
		// disable polling mode
		USARTREG(US_IER) = USART_INT_RXRDY;
	}
}

void
at91usart_cn_putc(dev_t dev, int c)
{
	int			s;
#if 0
	bus_space_tag_t		iot = usart_cn_sc.sc_iot;
	bus_space_handle_t	ioh = usart_cn_sc.sc_ioh;
#endif
	s = spltty();

	USART_PUTC(c);

#ifdef DEBUG
	if (c == '\r') {
		while((USARTREG(USART_SR) & USART_SR_TXEMPTY) == 0)
			;
	}
#endif

	splx(s);
}

int
at91usart_cn_getc(dev_t dev)
{
	int			c, sr;
	int			s;
#if 0
	bus_space_tag_t		iot = usart_cn_sc.sc_iot;
	bus_space_handle_t	ioh = usart_cn_sc.sc_ioh;
#endif

        s = spltty();

	while ((c = USART_PEEKC()) == -1) {
	  splx(s);
	  s = spltty();
	}
		;
	sr = USARTREG(USART_SR);
	if (ISSET(sr, USART_SR_FRAME) && c == 0) {
		USARTREG(USART_CR) = USART_CR_RSTSTA;	// reset status bits
		c = CNC_BREAK;
	}
#ifdef DDB
	extern int db_active;
	if (!db_active)
#endif
	{
		int cn_trapped = 0; /* unused */

		cn_check_magic(dev, c, at91usart_cnm_state);
	}
	splx(s);

	c &= 0xff;

	return (c);
}
#endif	/* NOTYET */

inline static void
at91usart_rxsoft(struct at91usart_softc *sc, struct tty *tp, unsigned csr)
{
	u_char *start, *get, *end;
	int cc;

	AT91PDC_FIFO_POSTREAD(sc->sc_iot, sc->sc_ioh, sc->sc_dmat, US_PDC,
			      &sc->sc_rx_fifo);

	if (ISSET(csr, US_CSR_TIMEOUT | US_CSR_RXBRK))
		at91usart_rx_stopped(sc);

	while ((start = AT91PDC_FIFO_RDPTR(&sc->sc_rx_fifo, &cc)) != NULL) {
		int (*rint)(int, struct tty *) = tp->t_linesw->l_rint;
		int code;

		if (!ISSET(csr, US_CSR_TIMEOUT | US_CSR_RXBRK))
			at91usart_rx_started(sc);

		for (get = start, end = start + cc; get < end; get++) {
			code = *get;
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
					get = end;
					printf("%s: receive missing data!\n",
					     device_xname(sc->sc_dev));
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
		}

		// tell we've read some bytes...
		AT91PDC_FIFO_READ(&sc->sc_rx_fifo, get - start);

		if (ISSET(sc->sc_rx_flags, RX_TTY_BLOCKED))
			break;
	}

	// h/w flow control hook:
	if (ISSET(sc->sc_swflags, TIOCFLAG_CRTSCTS))
		at91usart_rx_rts_ctl(sc, (AT91PDC_FIFO_SPACE(&sc->sc_rx_fifo) > PDC_BLOCK_SIZE * 2));

	// write next pointer if USART is ready:
	if (AT91PDC_FIFO_PREREAD(sc->sc_iot, sc->sc_ioh, sc->sc_dmat, US_PDC,
				  &sc->sc_rx_fifo, PDC_BLOCK_SIZE)) {
		SET(sc->sc_ier, US_CSR_ENDRX | US_CSR_RXBUFF | US_CSR_TIMEOUT | US_CSR_RXBRK);
	} else {
		CLR(sc->sc_ier, US_CSR_ENDRX | US_CSR_RXBUFF | US_CSR_TIMEOUT | US_CSR_RXBRK);
	}
}

inline static void
at91usart_txsoft(struct at91usart_softc *sc, struct tty *tp)
{
	at91usart_filltx(sc);
	if (!ISSET(tp->t_state, TS_BUSY))
		(*tp->t_linesw->l_start)(tp);
}


static void
at91usart_soft(void* arg)
{
	struct at91usart_softc *sc = arg;
	int s;
	u_int csr;

	if (COM_ISALIVE(sc) == 0)
		return;

	s = spltty();
	csr = sc->sc_csr;
	while (csr != 0) {
		if ((csr &= sc->sc_ier) == 0)
			break;
//		splx(s);
		DPRINTFN(5, ("%s: %s / csr = 0x%08x\n", device_xname(sc->sc_dev), __FUNCTION__, csr));
		if (ISSET(csr, US_CSR_ENDRX | US_CSR_RXBUFF | US_CSR_TIMEOUT | US_CSR_RXBRK)) {
			/* receive interrupt */
			if (ISSET(csr, US_CSR_RXBRK)) {
				// break received!
				at91usart_writereg(sc, US_CR, US_CR_RSTSTA | US_CR_STTTO);
			} else if (ISSET(csr, US_CSR_TIMEOUT)) {
				// timeout received
				at91usart_writereg(sc, US_CR, US_CR_STTTO);
			}
			at91usart_rxsoft(sc, sc->sc_tty, csr);
		}
		if (ISSET(csr, US_CSR_TXEMPTY)) {
			at91usart_stop_tx(sc);
			CLR(sc->sc_ier, US_CSR_TXEMPTY);
			if (AT91PDC_FIFO_EMPTY(&sc->sc_tx_fifo)) {
				// everything sent!
				if (ISSET(sc->sc_tty->t_state, TS_FLUSH))
					CLR(sc->sc_tty->t_state, TS_FLUSH);
			}
		}
		if (ISSET(csr, US_CSR_TXEMPTY | US_CSR_ENDTX)) {
			/* transmit interrupt! */
			at91usart_txsoft(sc, sc->sc_tty);
		}
//		s = spltty();
		csr = at91usart_readreg(sc, US_CSR);
	}
	sc->sc_csr = 0;
	at91usart_writereg(sc, US_IER, sc->sc_ier);	// re-enable interrupts
	splx(s);
}


static int
at91usart_intr(void* arg)
{
	struct at91usart_softc *sc = arg;
	u_int csr, imr;

	// get out if interrupts are not enabled
	imr = at91usart_readreg(sc, US_IMR);
	if (!imr)
		return 0;
	// get out if pending interrupt is not enabled
	csr = at91usart_readreg(sc, US_CSR);
	DPRINTFN(6,("%s: csr=%08X imr=%08X\n", device_xname(sc->sc_dev), csr, imr));
	if (!ISSET(csr, imr))
		return 0;

	// ok, we DO have some interrupts to serve! let softint do it
	sc->sc_csr = csr;
	at91usart_writereg(sc, US_IDR, -1);

	/* Wake up the poller. */
	softint_schedule(sc->sc_si);

	/* we're done for now */
	return (1);

}

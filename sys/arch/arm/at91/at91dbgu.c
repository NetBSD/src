/*	$Id: at91dbgu.c,v 1.15 2015/09/21 13:31:30 skrll Exp $	*/
/*	$NetBSD: at91dbgu.c,v 1.15 2015/09/21 13:31:30 skrll Exp $ */

/*
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
__KERNEL_RCSID(0, "$NetBSD: at91dbgu.c,v 1.15 2015/09/21 13:31:30 skrll Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"

#include "rnd.h"
#ifdef RND_COM
#include <sys/rndsource.h>
#endif

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
#include <sys/kauth.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <arm/at91/at91reg.h>
#include <arm/at91/at91var.h>
#include <arm/at91/at91dbguvar.h>
#include <arm/at91/at91dbgureg.h>

#include <dev/cons.h>

static int	at91dbgu_match(device_t, cfdata_t, void *);
static void	at91dbgu_attach(device_t, device_t, void *);

static int	at91dbgu_param(struct tty *, struct termios *);
static void	at91dbgu_start(struct tty *);
static int	at91dbgu_hwiflow(struct tty *, int);

#if 0
static u_int	cflag2lcrhi(tcflag_t);
#endif
static void	at91dbgu_iflush(struct at91dbgu_softc *);
static void	at91dbgu_set(struct at91dbgu_softc *);

int             at91dbgu_cn_getc(dev_t);
void            at91dbgu_cn_putc(dev_t, int);
void            at91dbgu_cn_pollc(dev_t, int);

static void	at91dbgu_soft(void* arg);
inline static void	at91dbgu_txsoft(struct at91dbgu_softc *, struct tty *);
inline static void	at91dbgu_rxsoft(struct at91dbgu_softc *, struct tty *);

void            at91dbgu_cn_probe(struct consdev *);
void            at91dbgu_cn_init(struct consdev *);

static struct at91dbgu_cons_softc {
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_hwbase;
	int			sc_ospeed;
	tcflag_t		sc_cflag;
	int			sc_attached;

	uint8_t			*sc_rx_ptr;
	uint8_t			sc_rx_fifo[64];
} dbgu_cn_sc;

static struct cnm_state at91dbgu_cnm_state;

CFATTACH_DECL_NEW(at91dbgu, sizeof(struct at91dbgu_softc),
	      at91dbgu_match, at91dbgu_attach, NULL, NULL);

extern struct cfdriver at91dbgu_cd;

dev_type_open(at91dbgu_open);
dev_type_close(at91dbgu_close);
dev_type_read(at91dbgu_read);
dev_type_write(at91dbgu_write);
dev_type_ioctl(at91dbgu_ioctl);
dev_type_stop(at91dbgu_stop);
dev_type_tty(at91dbgu_tty);
dev_type_poll(at91dbgu_poll);

const struct cdevsw at91dbgu_cdevsw = {
	.d_open = at91dbgu_open,
	.d_close = at91dbgu_close,
	.d_read = at91dbgu_read,
	.d_write = at91dbgu_write,
	.d_ioctl = at91dbgu_ioctl,
	.d_stop = at91dbgu_stop,
	.d_tty = at91dbgu_tty,
	.d_poll = at91dbgu_poll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

struct consdev at91dbgu_cons = {
	at91dbgu_cn_probe, NULL, at91dbgu_cn_getc, at91dbgu_cn_putc, at91dbgu_cn_pollc, NULL,
	NULL, NULL, NODEV, CN_REMOTE
};

#ifndef DEFAULT_COMSPEED
#define DEFAULT_COMSPEED 115200
#endif

#define COMUNIT(x)	TTUNIT(x)
#define COMDIALOUT(x)	TTDIALOUT(x)

#define COM_ISALIVE(sc)	((sc)->enabled != 0 && device_is_active((sc)->sc_dev))

static int
at91dbgu_match(device_t parent, cfdata_t match, void *aux)
{
	if (strcmp(match->cf_name, "at91dbgu") == 0)
		return 2;
	return 0;
}

static int
dbgu_intr(void* arg);

static void
at91dbgu_attach(device_t parent, device_t self, void *aux)
{
	struct at91dbgu_softc *sc = device_private(self);
	struct at91bus_attach_args *sa = aux;
	struct tty *tp;

	printf("\n");

	sc->sc_dev = self;
	bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 0, &sc->sc_ioh);
	sc->sc_iot = sa->sa_iot;
	sc->sc_hwbase = sa->sa_addr;

	DBGUREG(DBGU_IDR) = -1;
	at91_intr_establish(sa->sa_pid, IPL_SERIAL, INTR_HIGH_LEVEL, dbgu_intr, sc);
	DBGU_INIT(AT91_MSTCLK, 115200U);

	if (sc->sc_iot == dbgu_cn_sc.sc_iot
	    && sc->sc_hwbase == dbgu_cn_sc.sc_hwbase) {
		dbgu_cn_sc.sc_attached = 1;
		/* Make sure the console is always "hardwired". */
		delay(10000);	/* wait for output to finish */
		SET(sc->sc_hwflags, COM_HW_CONSOLE);
		SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);
		SET(sc->sc_ier, DBGU_INT_RXRDY);
		DBGUREG(DBGU_IER) = DBGU_INT_RXRDY; // @@@@@
	}

	tp = tty_alloc();
	tp->t_oproc = at91dbgu_start;
	tp->t_param = at91dbgu_param;
	tp->t_hwiflow = at91dbgu_hwiflow;

	sc->sc_tty = tp;
	sc->sc_rbuf = malloc(AT91DBGU_RING_SIZE << 1, M_DEVBUF, M_NOWAIT);
	sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
	sc->sc_rbavail = AT91DBGU_RING_SIZE;
	if (sc->sc_rbuf == NULL) {
		printf("%s: unable to allocate ring buffer\n",
		    device_xname(sc->sc_dev));
		return;
	}
	sc->sc_ebuf = sc->sc_rbuf + (AT91DBGU_RING_SIZE << 1);
	sc->sc_tbc = 0;

	tty_attach(tp);

	if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
		int maj;

		/* locate the major number */
		maj = cdevsw_lookup_major(&at91dbgu_cdevsw);

		cn_tab->cn_dev = makedev(maj, device_unit(sc->sc_dev));

		aprint_normal("%s: console (maj %u min %u cn_dev %#"PRIx64")\n",
		    device_xname(sc->sc_dev), maj, device_unit(sc->sc_dev),
		    cn_tab->cn_dev);
	}

	sc->sc_si = softint_establish(SOFTINT_SERIAL, at91dbgu_soft, sc);

#ifdef RND_COM
	rnd_attach_source(&sc->rnd_source, device_xname(sc->sc_dev),
			  RND_TYPE_TTY, RND_FLAG_DEFAULT);
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
at91dbgu_param(struct tty *tp, struct termios *t)
{
	struct at91dbgu_softc *sc
		= device_lookup_private(&at91dbgu_cd, COMUNIT(tp->t_dev));
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

	s = splserial();

	sc->sc_brgr = (AT91_MSTCLK / 16 + t->c_ospeed / 2) / t->c_ospeed;
	
	/* And copy to tty. */
	tp->t_ispeed = 0;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	at91dbgu_set(sc);

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

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		if (sc->sc_tx_stopped) {
			sc->sc_tx_stopped = 0;
			at91dbgu_start(tp);
		}
	}

	return (0);
}

static int
at91dbgu_hwiflow(struct tty *tp, int block)
{
	return (0);
}

static void
at91dbgu_filltx(struct at91dbgu_softc *sc)
{
#if 0
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
#endif
	int n;

	n = 0;
        while (DBGUREG(DBGU_SR) & DBGU_SR_TXRDY) {
		if (n >= sc->sc_tbc)
			break;
		DBGUREG(DBGU_THR) = *(sc->sc_tba + n) & 0xff;
		n++;
        }
        sc->sc_tbc -= n;
        sc->sc_tba += n;
}

static void
at91dbgu_start(struct tty *tp)
{
	struct at91dbgu_softc *sc
		= device_lookup_private(&at91dbgu_cd, COMUNIT(tp->t_dev));
	int s;

	if (COM_ISALIVE(sc) == 0)
		return;

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

	/* Output the first chunk of the contiguous buffer. */
	at91dbgu_filltx(sc);

	SET(sc->sc_ier, DBGU_INT_TXRDY);
	DBGUREG(DBGU_IER) = DBGU_INT_TXRDY;

out:
	splx(s);
	return;
}

static void
at91dbgu_break(struct at91dbgu_softc *sc, int onoff)
{
	// @@@ we must disconnect the TX pin from the DBGU and control
	// @@@ the pin directly to support this...
}

static void
at91dbgu_shutdown(struct at91dbgu_softc *sc)
{
	int s;

	s = splserial();

	/* Turn off interrupts. */
	DBGUREG(DBGU_IDR) = -1;

	/* Clear any break condition set with TIOCSBRK. */
	at91dbgu_break(sc, 0);
	at91dbgu_set(sc);

	if (sc->disable) {
#ifdef DIAGNOSTIC
		if (!sc->enabled)
			panic("at91dbgu_shutdown: not enabled?");
#endif
		(*sc->disable)(sc);
		sc->enabled = 0;
	}
	splx(s);
}

int
at91dbgu_open(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct at91dbgu_softc *sc;
	struct tty *tp;
	int s, s2;
	int error;

	sc = device_lookup_private(&at91dbgu_cd, COMUNIT(dev));
	if (sc == NULL || !ISSET(sc->sc_hwflags, COM_HW_DEV_OK) ||
		sc->sc_rbuf == NULL)
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

		s2 = splserial();

		if (sc->enable) {
			if ((*sc->enable)(sc)) {
				splx(s2);
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

		/* Turn on interrupts. */
		sc->sc_ier |= DBGU_INT_RXRDY;
		DBGUREG(DBGU_IER) = DBGU_INT_RXRDY;

#if 0
		/* Fetch the current modem control status, needed later. */
		sc->sc_msr = bus_space_read_1(sc->sc_iot, sc->sc_ioh, com_msr);

		/* Clear PPS capture state on first open. */
		sc->sc_ppsmask = 0;
		sc->ppsparam.mode = 0;
#endif

		splx(s2);

		/*
		 * Initialize the termios status to the defaults.  Add in the
		 * sticky bits from TIOCSFLAGS.
		 */
		t.c_ispeed = 0;
		if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
			t.c_ospeed = dbgu_cn_sc.sc_ospeed;
			t.c_cflag = dbgu_cn_sc.sc_cflag;
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
		/* Make sure at91dbgu_param() will do something. */
		tp->t_ospeed = 0;
		(void) at91dbgu_param(tp, &t);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		ttychars(tp);
		ttsetwater(tp);

		s2 = splserial();

		/* Clear the input ring, and unblock. */
		sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
		sc->sc_rbavail = AT91DBGU_RING_SIZE;
		at91dbgu_iflush(sc);
		CLR(sc->sc_rx_flags, RX_ANY_BLOCK);

#ifdef COM_DEBUG
		if (at91dbgu_debug)
			comstatus(sc, "at91dbgu_open  ");
#endif

		splx(s2);
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
		at91dbgu_shutdown(sc);
	}

	return (error);
}

int
at91dbgu_close(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct at91dbgu_softc *sc = device_lookup_private(&at91dbgu_cd, COMUNIT(dev));
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
		at91dbgu_shutdown(sc);
	}

	return (0);
}

int
at91dbgu_read(dev_t dev, struct uio *uio, int flag)
{
	struct at91dbgu_softc *sc = device_lookup_private(&at91dbgu_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);
 
	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
at91dbgu_write(dev_t dev, struct uio *uio, int flag)
{
	struct at91dbgu_softc *sc = device_lookup_private(&at91dbgu_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);
 
	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
at91dbgu_poll(dev_t dev, int events, struct lwp *l)
{
	struct at91dbgu_softc *sc = device_lookup_private(&at91dbgu_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);
 
	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

struct tty *
at91dbgu_tty(dev_t dev)
{
	struct at91dbgu_softc *sc = device_lookup_private(&at91dbgu_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	return (tp);
}

int
at91dbgu_ioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct at91dbgu_softc *sc = device_lookup_private(&at91dbgu_cd, COMUNIT(dev));
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

	s = splserial();

	switch (cmd) {
	case TIOCSBRK:
		at91dbgu_break(sc, 1);
		break;

	case TIOCCBRK:
		at91dbgu_break(sc, 0);
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
at91dbgu_stop(struct tty *tp, int flag)
{
	struct at91dbgu_softc *sc
		= device_lookup_private(&at91dbgu_cd, COMUNIT(tp->t_dev));
	int s;

	s = splserial();
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
	mr |= DBGU_MR_PAR_NONE;
	return (mr);
}
#endif

static void
at91dbgu_iflush(struct at91dbgu_softc *sc)
{
#if 0
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
#endif
#ifdef DIAGNOSTIC
	int reg;
#endif
	int timo;

#ifdef DIAGNOSTIC
	reg = 0xffff;
#endif
	timo = 50000;
	/* flush any pending I/O */
	while (DBGUREG(DBGU_SR) & DBGU_SR_RXRDY
	       && --timo)
#ifdef DIAGNOSTIC
		reg =
#else
			(void)
#endif
			DBGUREG(DBGU_RHR);
#ifdef DIAGNOSTIC
	if (!timo)
		printf("%s: com_iflush timeout %02x\n", device_xname(sc->sc_dev),
		       reg);
#endif
}

static void
at91dbgu_set(struct at91dbgu_softc *sc)
{
	DBGUREG(DBGU_MR) = DBGU_MR_PAR_NONE;
	DBGUREG(DBGU_BRGR) = sc->sc_brgr;
	DBGUREG(DBGU_CR) = DBGU_CR_TXEN | DBGU_CR_RXEN; // @@@ just in case
}

void
at91dbgu_cn_attach(bus_space_tag_t iot, bus_addr_t iobase, bus_space_handle_t ioh,
		   int ospeed, tcflag_t cflag);
void
at91dbgu_cn_attach(bus_space_tag_t iot, bus_addr_t iobase, bus_space_handle_t ioh,
		   int ospeed, tcflag_t cflag)
{
	cn_tab = &at91dbgu_cons;
	cn_init_magic(&at91dbgu_cnm_state);
	cn_set_magic("\047\001");

	dbgu_cn_sc.sc_iot = iot;
	dbgu_cn_sc.sc_ioh = ioh;
	dbgu_cn_sc.sc_hwbase = iobase;
	dbgu_cn_sc.sc_ospeed = ospeed;
	dbgu_cn_sc.sc_cflag = cflag;

	DBGU_INIT(AT91_MSTCLK, ospeed);
}

void at91dbgu_attach_cn(bus_space_tag_t iot, int ospeed, int cflag)
{
	bus_space_handle_t ioh;
	bus_space_map(iot, AT91DBGU_BASE, AT91DBGU_SIZE, 0, &ioh);
	at91dbgu_cn_attach(iot, AT91DBGU_BASE, ioh, ospeed, cflag);
}

void
at91dbgu_cn_probe(struct consdev *cp)
{
	cp->cn_pri = CN_REMOTE;
}

void
at91dbgu_cn_pollc(dev_t dev, int on)
{
	if (on) {
		// enable polling mode
		DBGUREG(DBGU_IDR) = DBGU_INT_RXRDY;
	} else {
		// disable polling mode
		DBGUREG(DBGU_IER) = DBGU_INT_RXRDY;
	}
}

void
at91dbgu_cn_putc(dev_t dev, int c)
{
#if 0
	bus_space_tag_t		iot = dbgu_cn_sc.sc_iot;
	bus_space_handle_t	ioh = dbgu_cn_sc.sc_ioh;
#endif
	DBGU_PUTC(c);

#ifdef DEBUG
	if (c == '\r') {
		int s = splserial();
		while((DBGUREG(DBGU_SR) & DBGU_SR_TXEMPTY) == 0) {
			splx(s);
			s = splserial();
		}
		splx(s);
	}
#endif

}

int
at91dbgu_cn_getc(dev_t dev)
{
	int			c, sr;
	int			s;
#if 0
	bus_space_tag_t		iot = dbgu_cn_sc.sc_iot;
	bus_space_handle_t	ioh = dbgu_cn_sc.sc_ioh;
#endif

        s = splserial();

	while ((c = DBGU_PEEKC()) == -1) {
	  splx(s);
	  s = splserial();
	}
		;
	sr = DBGUREG(DBGU_SR);
	if (ISSET(sr, DBGU_SR_FRAME) && c == 0) {
		DBGUREG(DBGU_CR) = DBGU_CR_RSTSTA;	// reset status bits
		c = CNC_BREAK;
	}
#ifdef DDB
	extern int db_active;
	if (!db_active)
#endif
	{
		int cn_trapped __unused = 0;

		cn_check_magic(dev, c, at91dbgu_cnm_state);
	}
	splx(s);

	c &= 0xff;

	return (c);
}

inline static void
at91dbgu_txsoft(struct at91dbgu_softc *sc, struct tty *tp)
{
	CLR(tp->t_state, TS_BUSY);
	if (ISSET(tp->t_state, TS_FLUSH))
		CLR(tp->t_state, TS_FLUSH);
        else
		ndflush(&tp->t_outq, (int)(sc->sc_tba - tp->t_outq.c_cf));
	(*tp->t_linesw->l_start)(tp);
}

inline static void
at91dbgu_rxsoft(struct at91dbgu_softc *sc, struct tty *tp)
{
	int (*rint)(int, struct tty *) = tp->t_linesw->l_rint;
	u_char *get, *end;
	u_int cc, scc;
	u_char sts;
	int code;
	int s;

	end = sc->sc_ebuf;
	get = sc->sc_rbget;
	scc = cc = AT91DBGU_RING_SIZE - sc->sc_rbavail;
#if 0
	if (cc == AT91DBGU_RING_SIZE) {
		sc->sc_floods++;
		if (sc->sc_errors++ == 0)
			callout_reset(&sc->sc_diag_callout, 60 * hz,
			    comdiag, sc);
	}
#endif
	while (cc) {
		code = get[0];
		sts = get[1];
		if (ISSET(sts, DBGU_SR_PARE | DBGU_SR_FRAME | DBGU_SR_OVRE)) {
#if 0
			if (ISSET(lsr, DR_ROR)) {
				sc->sc_overflows++;
				if (sc->sc_errors++ == 0)
					callout_reset(&sc->sc_diag_callout,
					    60 * hz, comdiag, sc);
			}
#endif
			if (ISSET(sts, (DBGU_SR_FRAME | DBGU_SR_OVRE)))
				SET(code, TTY_FE);
			if (ISSET(sts, DBGU_SR_PARE))
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
					get -= AT91DBGU_RING_SIZE << 1;
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
		
		cc = sc->sc_rbavail += scc - cc;
		/* Buffers should be ok again, release possible block. */
		if (cc >= 1) {
			if (ISSET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
				DBGUREG(DBGU_IER) = DBGU_INT_RXRDY;
			}
			if (ISSET(sc->sc_rx_flags, RX_IBUF_BLOCKED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_BLOCKED);
#if 0
				com_hwiflow(sc);
#endif
			}
		}
		splx(s);
	}
}

static void
at91dbgu_soft(void* arg)
{
	struct at91dbgu_softc *sc = arg;

	if (COM_ISALIVE(sc) == 0)
		return;

	if (sc->sc_rx_ready) {
		sc->sc_rx_ready = 0;
		at91dbgu_rxsoft(sc, sc->sc_tty);
	}
	if (sc->sc_tx_done) {
		sc->sc_tx_done = 0;
		at91dbgu_txsoft(sc, sc->sc_tty);
	}
}


static int
dbgu_intr(void* arg)
{
	struct at91dbgu_softc *sc = arg;
#if 0
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
#endif
	u_char *put, *end;
	u_int cc;
	uint32_t imr, sr;
	int c = 0;
	imr = DBGUREG(DBGU_IMR);
#if 0
	if (!imr)
		return 0;
#endif
	sr = DBGUREG(DBGU_SR);
	if (!(sr & imr)) {
		if (sr & DBGU_SR_RXRDY) {
//			printf("sr=0x%08x imr=0x%08x\n", sr, imr);
		}
		return 0;
	}

	end = sc->sc_ebuf;
	put = sc->sc_rbput;
	cc = sc->sc_rbavail;

	// ok, we DO have some interrupts to serve!
	if (sr & DBGU_SR_RXRDY) {
		int cn_trapped = 0;

		c = DBGUREG(DBGU_RHR);
		if (ISSET(sr, (DBGU_SR_OVRE | DBGU_SR_FRAME | DBGU_SR_PARE)))
			DBGUREG(DBGU_CR) = DBGU_CR_RSTSTA;
		if (ISSET(sr, DBGU_SR_FRAME) && c == 0) {
			c = CNC_BREAK;
		}
#ifdef DDB
		extern int db_active;
		if (!db_active)
#endif
			cn_check_magic(cn_tab->cn_dev, c, at91dbgu_cnm_state);
		if (!cn_trapped && cc) {
			put[0] = c & 0xff;
			put[1] = sr & 0xff;
			put += 2;
			if (put >= end)
				put = sc->sc_rbuf;
			cc--;
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

			/* but at91dbgu cannot (yet). X-( */

			/*
			 * If we're out of space, disable receive interrupts
			 * until the queue has drained a bit.
			 */
			if (!cc) {
				SET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
			}
		}
	}

	/*
	 * Done handling any receive interrupts. See if data can be
	 * transmitted as well. Schedule tx done event if no data left
	 * and tty was marked busy.
	 */

	if (ISSET(sr, DBGU_SR_TXRDY) && sc->sc_tbc > 0) {
		/* Output the next chunk of the contiguous buffer, if any. */
		at91dbgu_filltx(sc);
	} else {
		/* Disable transmit completion interrupts if necessary. */
		DBGUREG(DBGU_IDR) = DBGU_INT_TXRDY;
		if (sc->sc_tx_busy) {
			sc->sc_tx_busy = 0;
			sc->sc_tx_done = 1;
		}
	}

	/* Wake up the poller. */
	softint_schedule(sc->sc_si);
#if 0
#ifdef RND_COM
	rnd_add_uint32(&sc->rnd_source, imr ^ sr ^ c);
#endif
#endif

	return (1);
}

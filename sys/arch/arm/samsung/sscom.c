/*	$NetBSD: sscom.c,v 1.7 2014/10/02 09:03:43 skrll Exp $ */

/*
 * Copyright (c) 2002, 2003 Fujitsu Component Limited
 * Copyright (c) 2002, 2003 Genetec Corporation
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
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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
 * Support integrated UARTs of Samsung S3C2800/2400X/2410X
 * Derived from sys/dev/ic/com.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sscom.c,v 1.7 2014/10/02 09:03:43 skrll Exp $");

#include "opt_sscom.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"

#include "rnd.h"
#ifdef RND_COM
#include <sys/rnd.h>
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
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/timepps.h>
#include <sys/vnode.h>
#include <sys/kauth.h>
#include <sys/intr.h>
#include <sys/bus.h>
#include <sys/mutex.h>

#include <arm/samsung/sscom_reg.h>
#include <arm/samsung/sscom_var.h>
#include <dev/cons.h>

dev_type_open(sscomopen);
dev_type_close(sscomclose);
dev_type_read(sscomread);
dev_type_write(sscomwrite);
dev_type_ioctl(sscomioctl);
dev_type_stop(sscomstop);
dev_type_tty(sscomtty);
dev_type_poll(sscompoll);

int	sscomcngetc	(dev_t);
void	sscomcnputc	(dev_t, int);
void	sscomcnpollc	(dev_t, int);

#define	integrate	static inline
void 	sscomsoft	(void *);

integrate void sscom_rxsoft	(struct sscom_softc *, struct tty *);
integrate void sscom_txsoft	(struct sscom_softc *, struct tty *);
integrate void sscom_stsoft	(struct sscom_softc *, struct tty *);
integrate void sscom_schedrx	(struct sscom_softc *);
static void	sscom_modem(struct sscom_softc *, int);
static void	sscom_break(struct sscom_softc *, int);
static void	sscom_iflush(struct sscom_softc *);
static void	sscom_hwiflow(struct sscom_softc *);
static void	sscom_loadchannelregs(struct sscom_softc *);
static void	tiocm_to_sscom(struct sscom_softc *, u_long, int);
static int	sscom_to_tiocm(struct sscom_softc *);
static void	tiocm_to_sscom(struct sscom_softc *, u_long, int);
static int	sscom_to_tiocm(struct sscom_softc *);
static void	sscom_iflush(struct sscom_softc *);

static int	sscomhwiflow(struct tty *tp, int block);
#if defined(KGDB) || \
    defined(SSCOM0CONSOLE) || defined(SSCOM1CONSOLE) || \
    defined(SSCOM2CONSOLE) || defined(SSCOM3CONSOLE)
static int	sscom_init(bus_space_tag_t, bus_space_handle_t,
		    const struct sscom_uart_info *,
		    int, int, tcflag_t, bus_space_handle_t *);
#endif

extern struct cfdriver sscom_cd;

const struct cdevsw sscom_cdevsw = {
	.d_open = sscomopen,
	.d_close = sscomclose,
	.d_read = sscomread,
	.d_write = sscomwrite,
	.d_ioctl = sscomioctl,
	.d_stop = sscomstop,
	.d_tty = sscomtty,
	.d_poll = sscompoll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_flag = D_TTY
};

/*
 * Make this an option variable one can patch.
 * But be warned:  this must be a power of 2!
 */
u_int sscom_rbuf_size = SSCOM_RING_SIZE;

/* Stop input when 3/4 of the ring is full; restart when only 1/4 is full. */
u_int sscom_rbuf_hiwat = (SSCOM_RING_SIZE * 1) / 4;
u_int sscom_rbuf_lowat = (SSCOM_RING_SIZE * 3) / 4;

static int	sscomconsunit = -1;
static bus_space_tag_t sscomconstag;
static bus_space_handle_t sscomconsioh;
static int	sscomconsattached;
static int	sscomconsrate;
static tcflag_t sscomconscflag;
static struct cnm_state sscom_cnm_state;

#ifdef KGDB
#include <sys/kgdb.h>

static int sscom_kgdb_unit = -1;
static bus_space_tag_t sscom_kgdb_iot;
static bus_space_handle_t sscom_kgdb_ioh;
static int sscom_kgdb_attached;

int	sscom_kgdb_getc (void *);
void	sscom_kgdb_putc (void *, int);
#endif /* KGDB */

#define	SSCOMUNIT_MASK  	0x7f
#define	SSCOMDIALOUT_MASK	0x80

#define	SSCOMUNIT(x)	(minor(x) & SSCOMUNIT_MASK)
#define	SSCOMDIALOUT(x)	(minor(x) & SSCOMDIALOUT_MASK)

#if 0
#define	SSCOM_ISALIVE(sc)	((sc)->enabled != 0 && \
				 device_is_active(&(sc)->sc_dev))
#else
#define	SSCOM_ISALIVE(sc)	device_is_active((sc)->sc_dev)
#endif

#define	BR	BUS_SPACE_BARRIER_READ
#define	BW	BUS_SPACE_BARRIER_WRITE
#define SSCOM_BARRIER(t, h, f) /* no-op */

#if (defined(MULTIPROCESSOR) || defined(LOCKDEBUG)) && defined(SSCOM_MPLOCK)

#define SSCOM_LOCK(sc) mutex_enter((sc)->sc_lock)
#define SSCOM_UNLOCK(sc) mutex_exit((sc)->sc_lock)

#else

#define SSCOM_LOCK(sc)
#define SSCOM_UNLOCK(sc)

#endif

#ifndef SSCOM_TOLERANCE
#define	SSCOM_TOLERANCE	30	/* XXX: baud rate tolerance, in 0.1% units */
#endif

/* value for UCON */
#define UCON_RXINT_MASK	  \
	(UCON_RXMODE_MASK|UCON_ERRINT|UCON_TOINT|UCON_RXINT_TYPE)
#define UCON_RXINT_ENABLE \
	(UCON_RXMODE_INT|UCON_ERRINT|UCON_TOINT|UCON_RXINT_TYPE_LEVEL)
#define UCON_TXINT_MASK   (UCON_TXMODE_MASK|UCON_TXINT_TYPE)
#define UCON_TXINT_ENABLE (UCON_TXMODE_INT|UCON_TXINT_TYPE_LEVEL)

/* we don't want tx interrupt on debug port, but it is needed to
   have transmitter active */
#define UCON_DEBUGPORT	  (UCON_RXINT_ENABLE|UCON_TXINT_ENABLE)


static inline void
__sscom_output_chunk(struct sscom_softc *sc, int ufstat)
{
	int n, space;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	n = sc->sc_tbc;
	space = 16 - __SHIFTOUT(ufstat, UFSTAT_TXCOUNT);

	if (n > space)
		n = space;

	if (n > 0) {
		bus_space_write_multi_1(iot, ioh, SSCOM_UTXH, sc->sc_tba, n);
		sc->sc_tbc -= n;
		sc->sc_tba += n;
	}
}

static void
sscom_output_chunk(struct sscom_softc *sc)
{
	uint32_t ufstat = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSCOM_UFSTAT);

	if (!(ufstat & UFSTAT_TXFULL))
		__sscom_output_chunk(sc, ufstat);
}

int
sscomspeed(long speed, long frequency)
{
#define	divrnd(n, q)	(((n)*2/(q)+1)/2)	/* divide and round off */

	int x, err;

	if (speed <= 0)
		return -1;
	x = divrnd(frequency / 16, speed);
	if (x <= 0)
		return -1;
	err = divrnd(((quad_t)frequency) * 1000 / 16, speed * x) - 1000;
	if (err < 0)
		err = -err;
	if (err > SSCOM_TOLERANCE)
		return -1;
	return x-1;

#undef	divrnd
}

void sscomstatus (struct sscom_softc *, const char *);

#ifdef SSCOM_DEBUG
int	sscom_debug = 0;

/* XXX not all is printed in this version XXX */
void
sscomstatus(struct sscom_softc *sc, const char *str)
{
	struct tty *tp = sc->sc_tty;
	int umstat = bus_space_read_1(sc->sc_iot, sc->sc_ioh, SSCOM_UMSTAT);
	int umcon = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSCOM_UMCON);

	printf("%s: %s %sclocal  %sdcd %sts_carr_on %sdtr %stx_stopped\n",
	    device_xname(sc->sc_dev), str,
	    ISSET(tp->t_cflag, CLOCAL) ? "+" : "-",
	    "+",			/* DCD */
	    ISSET(tp->t_state, TS_CARR_ON) ? "+" : "-",
	    "+",			/* DTR */
	    sc->sc_tx_stopped ? "+" : "-");

	printf("%s: %s %scrtscts %scts %sts_ttstop  %srts %xrx_flags\n",
	    device_xname(sc->sc_dev), str,
	    ISSET(tp->t_cflag, CRTSCTS) ? "+" : "-",
	    ISSET(umstat, UMSTAT_CTS) ? "+" : "-",
	    ISSET(tp->t_state, TS_TTSTOP) ? "+" : "-",
	    ISSET(umcon, UMCON_RTS) ? "+" : "-",
	    sc->sc_rx_flags);
}
#else
#define sscom_debug  0
#endif

static void
sscom_enable_debugport(struct sscom_softc *sc)
{
	int s;

	/* Turn on line break interrupt, set carrier. */
	s = splserial();
	SSCOM_LOCK(sc);
	sc->sc_ucon = UCON_DEBUGPORT;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSCOM_UCON, sc->sc_ucon);
	sc->sc_umcon = UMCON_RTS|UMCON_DTR;
	sc->sc_set_modem_control(sc);
	sscom_enable_rxint(sc);
	sscom_disable_txint(sc);
	SSCOM_UNLOCK(sc);
	splx(s);
}

static void
sscom_set_modem_control(struct sscom_softc *sc)
{
	/* flob RTS */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
	    SSCOM_UMCON, sc->sc_umcon & UMCON_HW_MASK);
	/* ignore DTR */
}

static int
sscom_read_modem_status(struct sscom_softc *sc)
{
	int msts;

	msts = bus_space_read_1(sc->sc_iot, sc->sc_ioh, SSCOM_UMSTAT);

	/* DCD and DSR are always on */
	return (msts & UMSTAT_CTS) | MSTS_DCD | MSTS_DSR;
}

void
sscom_attach_subr(struct sscom_softc *sc)
{
	int unit = sc->sc_unit;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct tty *tp;

	callout_init(&sc->sc_diag_callout, 0);
#if (defined(MULTIPROCESSOR) || defined(LOCKDEBUG)) && defined(SSCOM_MPLOCK)
	sc->sc_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_SERIAL);
#endif

	sc->sc_ucon = UCON_RXINT_ENABLE|UCON_TXINT_ENABLE;

	/*
	 * set default for modem control hook
	 */
	if (sc->sc_set_modem_control == NULL)
		sc->sc_set_modem_control = sscom_set_modem_control;
	if (sc->sc_read_modem_status == NULL)
		sc->sc_read_modem_status = sscom_read_modem_status;

	/* Disable interrupts before configuring the device. */
	KASSERT(sc->sc_change_txrx_interrupts != NULL);
	KASSERT(sc->sc_clear_interrupts != NULL);
	sscom_disable_txrxint(sc);

#ifdef KGDB
	/*
	 * Allow kgdb to "take over" this port.  If this is
	 * the kgdb device, it has exclusive use.
	 */
	if (unit == sscom_kgdb_unit) {
		SET(sc->sc_hwflags, SSCOM_HW_KGDB);
		sc->sc_ucon = UCON_DEBUGPORT;
	}
#endif

	if (unit == sscomconsunit) {
		uint32_t stat;
		int timo;

		sscomconsattached = 1;
		sscomconstag = iot;
		sscomconsioh = ioh;

		/* wait for this transmission to complete */
		timo = 1500000;
		do {
			stat = bus_space_read_4(iot, ioh, SSCOM_UTRSTAT);
		} while ((stat & UTRSTAT_TXEMPTY) == 0 && --timo > 0);

		/* Make sure the console is always "hardwired". */
		SET(sc->sc_hwflags, SSCOM_HW_CONSOLE);
		SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);

		sc->sc_ucon = UCON_DEBUGPORT;
	}

	/* set RX/TX trigger to half values */
	bus_space_write_4(iot, ioh, SSCOM_UFCON,
	    __SHIFTIN(4, UFCON_TXTRIGGER) |
	    __SHIFTIN(4, UFCON_RXTRIGGER) |
	     UFCON_FIFO_ENABLE | 
	     UFCON_TXFIFO_RESET|
	     UFCON_RXFIFO_RESET);
	/* tx/rx fifo reset are auto-cleared */

	bus_space_write_4(iot, ioh, SSCOM_UCON, sc->sc_ucon);

#ifdef KGDB
	if (ISSET(sc->sc_hwflags, SSCOM_HW_KGDB)) {
		sscom_kgdb_attached = 1;
		printf("%s: kgdb\n", device_xname(sc->sc_dev));
		sscom_enable_debugport(sc);
		return;
	}
#endif

	tp = tty_alloc();
	tp->t_oproc = sscomstart;
	tp->t_param = sscomparam;
	tp->t_hwiflow = sscomhwiflow;

	sc->sc_tty = tp;
	sc->sc_rbuf = malloc(sscom_rbuf_size << 1, M_DEVBUF, M_NOWAIT);
	sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
	sc->sc_rbavail = sscom_rbuf_size;
	if (sc->sc_rbuf == NULL) {
		printf("%s: unable to allocate ring buffer\n",
		    device_xname(sc->sc_dev));
		return;
	}
	sc->sc_ebuf = sc->sc_rbuf + (sscom_rbuf_size << 1);

	tty_attach(tp);

	if (ISSET(sc->sc_hwflags, SSCOM_HW_CONSOLE)) {
		int maj;

		/* locate the major number */
		maj = cdevsw_lookup_major(&sscom_cdevsw);

		cn_tab->cn_dev = makedev(maj, device_unit(sc->sc_dev));

		printf("%s: console (major=%d)\n", device_xname(sc->sc_dev), maj);
	}


	sc->sc_si = softint_establish(SOFTINT_SERIAL, sscomsoft, sc);

#ifdef RND_COM
	rnd_attach_source(&sc->rnd_source, device_xname(sc->sc_dev),
			  RND_TYPE_TTY, RND_FLAG_DEFAULT);
#endif

	/* if there are no enable/disable functions, assume the device
	   is always enabled */

	if (ISSET(sc->sc_hwflags, SSCOM_HW_CONSOLE))
		sscom_enable_debugport(sc);
	else 
		sscom_disable_txrxint(sc);

	SET(sc->sc_hwflags, SSCOM_HW_DEV_OK);
}

int
sscom_detach(device_t self, int flags)
{
	struct sscom_softc *sc = device_private(self);

	if (sc->sc_hwflags & (SSCOM_HW_CONSOLE|SSCOM_HW_KGDB))
		return EBUSY;

	return 0;
}

int
sscom_activate(device_t self, enum devact act)
{
#ifdef notyet
	struct sscom_softc *sc = device_private(self);
#endif

	switch (act) {
	case DVACT_DEACTIVATE:
#ifdef notyet
		sc->enabled = 0;
#endif
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

void
sscom_shutdown(struct sscom_softc *sc)
{
#ifdef notyet
	struct tty *tp = sc->sc_tty;
	int s;

	s = splserial();
	SSCOM_LOCK(sc);	

	/* If we were asserting flow control, then deassert it. */
	SET(sc->sc_rx_flags, RX_IBUF_BLOCKED);
	sscom_hwiflow(sc);

	/* Clear any break condition set with TIOCSBRK. */
	sscom_break(sc, 0);

	/*
	 * Hang up if necessary.  Wait a bit, so the other side has time to
	 * notice even if we immediately open the port again.
	 * Avoid tsleeping above splhigh().
	 */
	if (ISSET(tp->t_cflag, HUPCL)) {
		sscom_modem(sc, 0);
		SSCOM_UNLOCK(sc);
		splx(s);
		/* XXX tsleep will only timeout */
		(void) tsleep(sc, TTIPRI, ttclos, hz);
		s = splserial();
		SSCOM_LOCK(sc);	
	}

	if (ISSET(sc->sc_hwflags, SSCOM_HW_CONSOLE))
		/* interrupt on break */
		sc->sc_ucon = UCON_DEBUGPORT;
	else
		sc->sc_ucon = 0;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSCOM_UCON, sc->sc_ucon);

#ifdef DIAGNOSTIC
	if (!sc->enabled)
		panic("sscom_shutdown: not enabled?");
#endif
	sc->enabled = 0;
	SSCOM_UNLOCK(sc);
	splx(s);
#endif
}

int
sscomopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct sscom_softc *sc;
	struct tty *tp;
	int s, s2;
	int error;

	sc = device_lookup_private(&sscom_cd, SSCOMUNIT(dev));
	if (sc == NULL || !ISSET(sc->sc_hwflags, SSCOM_HW_DEV_OK) ||
		sc->sc_rbuf == NULL)
		return ENXIO;

	if (!device_is_active(sc->sc_dev))
		return ENXIO;

#ifdef KGDB
	/*
	 * If this is the kgdb port, no other use is permitted.
	 */
	if (ISSET(sc->sc_hwflags, SSCOM_HW_KGDB))
		return EBUSY;
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
		SSCOM_LOCK(sc);

		/* Turn on interrupts. */
		sscom_enable_txrxint(sc);

		/* Fetch the current modem control status, needed later. */
		sc->sc_msts = sc->sc_read_modem_status(sc);

#if 0
		/* Clear PPS capture state on first open. */
		sc->sc_ppsmask = 0;
		sc->ppsparam.mode = 0;
#endif

		SSCOM_UNLOCK(sc);
		splx(s2);

		/*
		 * Initialize the termios status to the defaults.  Add in the
		 * sticky bits from TIOCSFLAGS.
		 */
		t.c_ispeed = 0;
		if (ISSET(sc->sc_hwflags, SSCOM_HW_CONSOLE)) {
			t.c_ospeed = sscomconsrate;
			t.c_cflag = sscomconscflag;
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
		/* Make sure sscomparam() will do something. */
		tp->t_ospeed = 0;
		(void) sscomparam(tp, &t);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		ttychars(tp);
		ttsetwater(tp);

		s2 = splserial();
		SSCOM_LOCK(sc);

		/*
		 * Turn on DTR.  We must always do this, even if carrier is not
		 * present, because otherwise we'd have to use TIOCSDTR
		 * immediately after setting CLOCAL, which applications do not
		 * expect.  We always assert DTR while the device is open
		 * unless explicitly requested to deassert it.
		 */
		sscom_modem(sc, 1);

		/* Clear the input ring, and unblock. */
		sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
		sc->sc_rbavail = sscom_rbuf_size;
		sscom_iflush(sc);
		CLR(sc->sc_rx_flags, RX_ANY_BLOCK);
		sscom_hwiflow(sc);

		if (sscom_debug)
			sscomstatus(sc, "sscomopen  ");

		SSCOM_UNLOCK(sc);
		splx(s2);
	}
	
	splx(s);

	error = ttyopen(tp, SSCOMDIALOUT(dev), ISSET(flag, O_NONBLOCK));
	if (error)
		goto bad;

	error = (*tp->t_linesw->l_open)(dev, tp);
	if (error)
		goto bad;

	return 0;

bad:
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * We failed to open the device, and nobody else had it opened.
		 * Clean up the state as appropriate.
		 */
		sscom_shutdown(sc);
	}

	return error;
}
 
int
sscomclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct sscom_softc *sc = device_lookup_private(&sscom_cd, SSCOMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	/* XXX This is for cons.c. */
	if (!ISSET(tp->t_state, TS_ISOPEN))
		return 0;

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);

	if (SSCOM_ISALIVE(sc) == 0)
		return 0;

	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * Although we got a last close, the device may still be in
		 * use; e.g. if this was the dialout node, and there are still
		 * processes waiting for carrier on the non-dialout node.
		 */
		sscom_shutdown(sc);
	}

	return 0;
}
 
int
sscomread(dev_t dev, struct uio *uio, int flag)
{
	struct sscom_softc *sc = device_lookup_private(&sscom_cd, SSCOMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (SSCOM_ISALIVE(sc) == 0)
		return EIO;
 
	return (*tp->t_linesw->l_read)(tp, uio, flag);
}
 
int
sscomwrite(dev_t dev, struct uio *uio, int flag)
{
	struct sscom_softc *sc = device_lookup_private(&sscom_cd, SSCOMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (SSCOM_ISALIVE(sc) == 0)
		return EIO;
 
	return (*tp->t_linesw->l_write)(tp, uio, flag);
}

int
sscompoll(dev_t dev, int events, struct lwp *l)
{
	struct sscom_softc *sc = device_lookup_private(&sscom_cd, SSCOMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (SSCOM_ISALIVE(sc) == 0)
		return EIO;
 
	return (*tp->t_linesw->l_poll)(tp, events, l);
}

struct tty *
sscomtty(dev_t dev)
{
	struct sscom_softc *sc = device_lookup_private(&sscom_cd, SSCOMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	return tp;
}

int
sscomioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct sscom_softc *sc = device_lookup_private(&sscom_cd, SSCOMUNIT(dev));
	struct tty *tp = sc->sc_tty;
	int error;
	int s;

	if (SSCOM_ISALIVE(sc) == 0)
		return EIO;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	error = ttioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	error = 0;

	s = splserial();
	SSCOM_LOCK(sc);	

	switch (cmd) {
	case TIOCSBRK:
		sscom_break(sc, 1);
		break;

	case TIOCCBRK:
		sscom_break(sc, 0);
		break;

	case TIOCSDTR:
		sscom_modem(sc, 1);
		break;

	case TIOCCDTR:
		sscom_modem(sc, 0);
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

	case TIOCMSET:
	case TIOCMBIS:
	case TIOCMBIC:
		tiocm_to_sscom(sc, cmd, *(int *)data);
		break;

	case TIOCMGET:
		*(int *)data = sscom_to_tiocm(sc);
		break;

	default:
		error = EPASSTHROUGH;
		break;
	}

	SSCOM_UNLOCK(sc);
	splx(s);

	if (sscom_debug)
		sscomstatus(sc, "sscomioctl ");

	return error;
}

integrate void
sscom_schedrx(struct sscom_softc *sc)
{

	sc->sc_rx_ready = 1;

	/* Wake up the poller. */
	softint_schedule(sc->sc_si);
}

static void
sscom_break(struct sscom_softc *sc, int onoff)
{

	if (onoff)
		SET(sc->sc_ucon, UCON_SBREAK);
	else
		CLR(sc->sc_ucon, UCON_SBREAK);

	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			sscom_loadchannelregs(sc);
	}
}

static void
sscom_modem(struct sscom_softc *sc, int onoff)
{
	if (onoff)
		SET(sc->sc_umcon, UMCON_DTR);
	else
		CLR(sc->sc_umcon, UMCON_DTR);

	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			sscom_loadchannelregs(sc);
	}
}

static void
tiocm_to_sscom(struct sscom_softc *sc, u_long how, int ttybits)
{
	u_char sscombits;

	sscombits = 0;
	if (ISSET(ttybits, TIOCM_DTR))
		sscombits = UMCON_DTR;
	if (ISSET(ttybits, TIOCM_RTS))
		SET(sscombits, UMCON_RTS);
 
	switch (how) {
	case TIOCMBIC:
		CLR(sc->sc_umcon, sscombits);
		break;

	case TIOCMBIS:
		SET(sc->sc_umcon, sscombits);
		break;

	case TIOCMSET:
		CLR(sc->sc_umcon, UMCON_DTR);
		SET(sc->sc_umcon, sscombits);
		break;
	}

	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			sscom_loadchannelregs(sc);
	}
}

static int
sscom_to_tiocm(struct sscom_softc *sc)
{
	u_char sscombits;
	int ttybits = 0;

	sscombits = sc->sc_umcon;
#if 0
	if (ISSET(sscombits, MCR_DTR))
		SET(ttybits, TIOCM_DTR);
#endif
	if (ISSET(sscombits, UMCON_RTS))
		SET(ttybits, TIOCM_RTS);

	sscombits = sc->sc_msts;
	if (ISSET(sscombits, MSTS_DCD))
		SET(ttybits, TIOCM_CD);
	if (ISSET(sscombits, MSTS_DSR))
		SET(ttybits, TIOCM_DSR);
	if (ISSET(sscombits, MSTS_CTS))
		SET(ttybits, TIOCM_CTS);

	if (sc->sc_ucon != 0)
		SET(ttybits, TIOCM_LE);

	return ttybits;
}

static int
cflag2lcr(tcflag_t cflag)
{
	u_char lcr = ULCON_PARITY_NONE;

	switch (cflag & (PARENB|PARODD)) {
	case PARENB|PARODD: lcr = ULCON_PARITY_ODD; break;
	case PARENB: lcr = ULCON_PARITY_EVEN;
	}

	switch (ISSET(cflag, CSIZE)) {
	case CS5:
		SET(lcr, ULCON_LENGTH_5);
		break;
	case CS6:
		SET(lcr, ULCON_LENGTH_6);
		break;
	case CS7:
		SET(lcr, ULCON_LENGTH_7);
		break;
	case CS8:
		SET(lcr, ULCON_LENGTH_8);
		break;
	}
	if (ISSET(cflag, CSTOPB))
		SET(lcr, ULCON_STOP);

	return lcr;
}

int
sscomparam(struct tty *tp, struct termios *t)
{
	struct sscom_softc *sc = device_lookup_private(&sscom_cd, SSCOMUNIT(tp->t_dev));
	int ospeed;
	u_char lcr;
	int s;

	if (SSCOM_ISALIVE(sc) == 0)
		return EIO;

	ospeed = sscomspeed(t->c_ospeed, sc->sc_frequency);

	/* Check requested parameters. */
	if (ospeed < 0)
		return EINVAL;
	if (t->c_ispeed && t->c_ispeed != t->c_ospeed)
		return EINVAL;

	/*
	 * For the console, always force CLOCAL and !HUPCL, so that the port
	 * is always active.
	 */
	if (ISSET(sc->sc_swflags, TIOCFLAG_SOFTCAR) ||
	    ISSET(sc->sc_hwflags, SSCOM_HW_CONSOLE)) {
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
		return 0;

	lcr = cflag2lcr(t->c_cflag);

	s = splserial();
	SSCOM_LOCK(sc);	

	sc->sc_ulcon = lcr;

	/*
	 * If we're not in a mode that assumes a connection is present, then
	 * ignore carrier changes.
	 */
	if (ISSET(t->c_cflag, CLOCAL | MDMBUF))
		sc->sc_msr_dcd = 0;
	else
		sc->sc_msr_dcd = MSTS_DCD;

	/*
	 * Set the flow control pins depending on the current flow control
	 * mode.
	 */
	if (ISSET(t->c_cflag, CRTSCTS)) {
		sc->sc_mcr_dtr = UMCON_DTR;
		sc->sc_mcr_rts = UMCON_RTS;
		sc->sc_msr_cts = MSTS_CTS;
	}
	else if (ISSET(t->c_cflag, MDMBUF)) {
		/*
		 * For DTR/DCD flow control, make sure we don't toggle DTR for
		 * carrier detection.
		 */
		sc->sc_mcr_dtr = 0;
		sc->sc_mcr_rts = UMCON_DTR;
		sc->sc_msr_cts = MSTS_DCD;
	} 
	else {
		/*
		 * If no flow control, then always set RTS.  This will make
		 * the other side happy if it mistakenly thinks we're doing
		 * RTS/CTS flow control.
		 */
		sc->sc_mcr_dtr = UMCON_DTR | UMCON_RTS;
		sc->sc_mcr_rts = 0;
		sc->sc_msr_cts = 0;
		if (ISSET(sc->sc_umcon, UMCON_DTR))
			SET(sc->sc_umcon, UMCON_RTS);
		else
			CLR(sc->sc_umcon, UMCON_RTS);
	}
	sc->sc_msr_mask = sc->sc_msr_cts | sc->sc_msr_dcd;

	if (ospeed == 0)
		CLR(sc->sc_umcon, sc->sc_mcr_dtr);
	else
		SET(sc->sc_umcon, sc->sc_mcr_dtr);

	sc->sc_ubrdiv = ospeed;

	/* And copy to tty. */
	tp->t_ispeed = 0;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			sscom_loadchannelregs(sc);
	}

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		/* Disable the high water mark. */
		sc->sc_r_hiwat = 0;
		sc->sc_r_lowat = 0;
		if (ISSET(sc->sc_rx_flags, RX_TTY_OVERFLOWED)) {
			CLR(sc->sc_rx_flags, RX_TTY_OVERFLOWED);
			sscom_schedrx(sc);
		}
		if (ISSET(sc->sc_rx_flags, RX_TTY_BLOCKED|RX_IBUF_BLOCKED)) {
			CLR(sc->sc_rx_flags, RX_TTY_BLOCKED|RX_IBUF_BLOCKED);
			sscom_hwiflow(sc);
		}
	} else {
		sc->sc_r_hiwat = sscom_rbuf_hiwat;
		sc->sc_r_lowat = sscom_rbuf_lowat;
	}

	SSCOM_UNLOCK(sc);
	splx(s);

	/*
	 * Update the tty layer's idea of the carrier bit, in case we changed
	 * CLOCAL or MDMBUF.  We don't hang up here; we only do that by
	 * explicit request.
	 */
	(void) (*tp->t_linesw->l_modem)(tp, ISSET(sc->sc_msts, MSTS_DCD));

	if (sscom_debug)
		sscomstatus(sc, "sscomparam ");

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		if (sc->sc_tx_stopped) {
			sc->sc_tx_stopped = 0;
			sscomstart(tp);
		}
	}

	return 0;
}

static void
sscom_iflush(struct sscom_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int timo;


	timo = 50000;
	/* flush any pending I/O */
	while ( sscom_rxrdy(iot, ioh) && --timo)
		(void)sscom_getc(iot,ioh);
#ifdef DIAGNOSTIC
	if (!timo)
		printf("%s: sscom_iflush timeout\n", device_xname(sc->sc_dev));
#endif
}

static void
sscom_loadchannelregs(struct sscom_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	/* XXXXX necessary? */
	sscom_iflush(sc);

	bus_space_write_4(iot, ioh, SSCOM_UCON, 0);

#if 0
	if (ISSET(sc->sc_hwflags, COM_HW_FLOW)) {
		bus_space_write_1(iot, ioh, com_lcr, LCR_EERS);
		bus_space_write_1(iot, ioh, com_efr, sc->sc_efr);
	}
#endif

	bus_space_write_2(iot, ioh, SSCOM_UBRDIV, sc->sc_ubrdiv);
	bus_space_write_1(iot, ioh, SSCOM_ULCON, sc->sc_ulcon);
	sc->sc_set_modem_control(sc);
	bus_space_write_4(iot, ioh, SSCOM_UCON, sc->sc_ucon);
}

static int
sscomhwiflow(struct tty *tp, int block)
{
	struct sscom_softc *sc = device_lookup_private(&sscom_cd, SSCOMUNIT(tp->t_dev));
	int s;

	if (SSCOM_ISALIVE(sc) == 0)
		return 0;

	if (sc->sc_mcr_rts == 0)
		return 0;

	s = splserial();
	SSCOM_LOCK(sc);
	
	if (block) {
		if (!ISSET(sc->sc_rx_flags, RX_TTY_BLOCKED)) {
			SET(sc->sc_rx_flags, RX_TTY_BLOCKED);
			sscom_hwiflow(sc);
		}
	} else {
		if (ISSET(sc->sc_rx_flags, RX_TTY_OVERFLOWED)) {
			CLR(sc->sc_rx_flags, RX_TTY_OVERFLOWED);
			sscom_schedrx(sc);
		}
		if (ISSET(sc->sc_rx_flags, RX_TTY_BLOCKED)) {
			CLR(sc->sc_rx_flags, RX_TTY_BLOCKED);
			sscom_hwiflow(sc);
		}
	}

	SSCOM_UNLOCK(sc);
	splx(s);
	return 1;
}
	
/*
 * (un)block input via hw flowcontrol
 */
static void
sscom_hwiflow(struct sscom_softc *sc)
{
	if (sc->sc_mcr_rts == 0)
		return;

	if (ISSET(sc->sc_rx_flags, RX_ANY_BLOCK)) {
		CLR(sc->sc_umcon, sc->sc_mcr_rts);
		CLR(sc->sc_mcr_active, sc->sc_mcr_rts);
	} else {
		SET(sc->sc_umcon, sc->sc_mcr_rts);
		SET(sc->sc_mcr_active, sc->sc_mcr_rts);
	}
	sc->sc_set_modem_control(sc);
}


void
sscomstart(struct tty *tp)
{
	struct sscom_softc *sc = device_lookup_private(&sscom_cd, SSCOMUNIT(tp->t_dev));
	int s;

	if (SSCOM_ISALIVE(sc) == 0)
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
		SSCOM_LOCK(sc);

		sc->sc_tba = tba;
		sc->sc_tbc = tbc;
	}

	SET(tp->t_state, TS_BUSY);
	sc->sc_tx_busy = 1;

	/* Output the first chunk of the contiguous buffer. */
	sscom_output_chunk(sc);

	/* Enable transmit completion interrupts if necessary. */
	if ((sc->sc_hwflags & SSCOM_HW_TXINT) == 0)
		sscom_enable_txint(sc);

	SSCOM_UNLOCK(sc);
out:
	splx(s);
	return;
}

/*
 * Stop output on a line.
 */
void
sscomstop(struct tty *tp, int flag)
{
	struct sscom_softc *sc = device_lookup_private(&sscom_cd, SSCOMUNIT(tp->t_dev));
	int s;

	s = splserial();
	SSCOM_LOCK(sc);
	if (ISSET(tp->t_state, TS_BUSY)) {
		/* Stop transmitting at the next chunk. */
		sc->sc_tbc = 0;
		sc->sc_heldtbc = 0;
		if (!ISSET(tp->t_state, TS_TTSTOP))
			SET(tp->t_state, TS_FLUSH);
	}
	SSCOM_UNLOCK(sc);	
	splx(s);
}

void
sscomdiag(void *arg)
{
	struct sscom_softc *sc = arg;
	int overflows, floods;
	int s;

	s = splserial();
	SSCOM_LOCK(sc);
	overflows = sc->sc_overflows;
	sc->sc_overflows = 0;
	floods = sc->sc_floods;
	sc->sc_floods = 0;
	sc->sc_errors = 0;
	SSCOM_UNLOCK(sc);
	splx(s);

	log(LOG_WARNING, "%s: %d silo overflow%s, %d ibuf flood%s\n",
	    device_xname(sc->sc_dev),
	    overflows, overflows == 1 ? "" : "s",
	    floods, floods == 1 ? "" : "s");
}

integrate void
sscom_rxsoft(struct sscom_softc *sc, struct tty *tp)
{
	int (*rint) (int, struct tty *) = tp->t_linesw->l_rint;
	u_char *get, *end;
	u_int cc, scc;
	u_char rsr;
	int code;
	int s;

	end = sc->sc_ebuf;
	get = sc->sc_rbget;
	scc = cc = sscom_rbuf_size - sc->sc_rbavail;

	if (cc == sscom_rbuf_size) {
		sc->sc_floods++;
		if (sc->sc_errors++ == 0)
			callout_reset(&sc->sc_diag_callout, 60 * hz,
			    sscomdiag, sc);
	}

	while (cc) {
		code = get[0];
		rsr = get[1];
		if (rsr) {
			if (ISSET(rsr, UERSTAT_OVERRUN)) {
				sc->sc_overflows++;
				if (sc->sc_errors++ == 0)
					callout_reset(&sc->sc_diag_callout,
					    60 * hz, sscomdiag, sc);
			}
			if (ISSET(rsr, UERSTAT_BREAK | UERSTAT_FRAME))
				SET(code, TTY_FE);
			if (ISSET(rsr, UERSTAT_PARITY))
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
					get -= sscom_rbuf_size << 1;
				cc = 0;
			} else {
				/*
				 * Don't schedule any more receive processing
				 * until the line discipline tells us there's
				 * space available (through sscomhwiflow()).
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
		SSCOM_LOCK(sc);
		
		cc = sc->sc_rbavail += scc - cc;
		/* Buffers should be ok again, release possible block. */
		if (cc >= sc->sc_r_lowat) {
			if (ISSET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
				sscom_enable_rxint(sc);
				sc->sc_ucon |= UCON_ERRINT;
				bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSCOM_UCON, 
						  sc->sc_ucon);

			}
			if (ISSET(sc->sc_rx_flags, RX_IBUF_BLOCKED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_BLOCKED);
				sscom_hwiflow(sc);
			}
		}
		SSCOM_UNLOCK(sc);
		splx(s);
	}
}

integrate void
sscom_txsoft(struct sscom_softc *sc, struct tty *tp)
{

	CLR(tp->t_state, TS_BUSY);
	if (ISSET(tp->t_state, TS_FLUSH))
		CLR(tp->t_state, TS_FLUSH);
	else
		ndflush(&tp->t_outq, (int)(sc->sc_tba - tp->t_outq.c_cf));
	(*tp->t_linesw->l_start)(tp);
}

integrate void
sscom_stsoft(struct sscom_softc *sc, struct tty *tp)
{
	u_char msr, delta;
	int s;

	s = splserial();
	SSCOM_LOCK(sc);
	msr = sc->sc_msts;
	delta = sc->sc_msr_delta;
	sc->sc_msr_delta = 0;
	SSCOM_UNLOCK(sc);	
	splx(s);

	if (ISSET(delta, sc->sc_msr_dcd)) {
		/*
		 * Inform the tty layer that carrier detect changed.
		 */
		(void) (*tp->t_linesw->l_modem)(tp, ISSET(msr, MSTS_DCD));
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

	if (sscom_debug)
		sscomstatus(sc, "sscom_stsoft");
}

void
sscomsoft(void *arg)
{
	struct sscom_softc *sc = arg;
	struct tty *tp;

	if (SSCOM_ISALIVE(sc) == 0)
		return;

	{
		tp = sc->sc_tty;
		
		if (sc->sc_rx_ready) {
			sc->sc_rx_ready = 0;
			sscom_rxsoft(sc, tp);
		}

		if (sc->sc_st_check) {
			sc->sc_st_check = 0;
			sscom_stsoft(sc, tp);
		}

		if (sc->sc_tx_done) {
			sc->sc_tx_done = 0;
			sscom_txsoft(sc, tp);
		}
	}
}


int
sscomrxintr(void *arg)
{
	struct sscom_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_char *put, *end;
	u_int cc;

	if (SSCOM_ISALIVE(sc) == 0)
		return 0;

	SSCOM_LOCK(sc);

	end = sc->sc_ebuf;
	put = sc->sc_rbput;
	cc = sc->sc_rbavail;

	do {
		u_char	msts, delta;
		u_char  uerstat;
		uint32_t ufstat;

		ufstat = bus_space_read_4(iot, ioh, SSCOM_UFSTAT);

		/* XXX: break interrupt with no character? */

		if ( (ufstat & (UFSTAT_RXCOUNT|UFSTAT_RXFULL)) &&
		    !ISSET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED)) {

			while (cc > 0) {
				int cn_trapped = 0;

				/* get status and received character.
				   read status register first */
				uerstat = sscom_geterr(iot, ioh);
				put[0] = sscom_getc(iot, ioh);

				if (ISSET(uerstat, UERSTAT_BREAK)) {
					int con_trapped = 0;
					cn_check_magic(sc->sc_tty->t_dev,
					    CNC_BREAK, sscom_cnm_state);
					if (con_trapped)
						continue;
#if defined(KGDB)
					if (ISSET(sc->sc_hwflags,
					    SSCOM_HW_KGDB)) {
						kgdb_connect(1);
						continue;
					}
#endif
				}

				put[1] = uerstat;
				cn_check_magic(sc->sc_tty->t_dev,
					       put[0], sscom_cnm_state);
				if (!cn_trapped) {
					put += 2;
					if (put >= end)
						put = sc->sc_rbuf;
					cc--;
				}

				ufstat = bus_space_read_4(iot, ioh, SSCOM_UFSTAT);
				if ( (ufstat & (UFSTAT_RXFULL|UFSTAT_RXCOUNT)) == 0 )
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
				sscom_hwiflow(sc);
			}

			/*
			 * If we're out of space, disable receive interrupts
			 * until the queue has drained a bit.
			 */
			if (!cc) {
				SET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
				sscom_disable_rxint(sc);
				sc->sc_ucon &= ~UCON_ERRINT;
				bus_space_write_4(iot, ioh, SSCOM_UCON, sc->sc_ucon);
			}
		}


		msts = sc->sc_read_modem_status(sc);
		delta = msts ^ sc->sc_msts;
		sc->sc_msts = msts;

#ifdef notyet
		/*
		 * Pulse-per-second (PSS) signals on edge of DCD?
		 * Process these even if line discipline is ignoring DCD.
		 */
		if (delta & sc->sc_ppsmask) {
			struct timeval tv;
		    	if ((msr & sc->sc_ppsmask) == sc->sc_ppsassert) {
				/* XXX nanotime() */
				microtime(&tv);
				TIMEVAL_TO_TIMESPEC(&tv, 
				    &sc->ppsinfo.assert_timestamp);
				if (sc->ppsparam.mode & PPS_OFFSETASSERT) {
					timespecadd(&sc->ppsinfo.assert_timestamp,
					    &sc->ppsparam.assert_offset,
						    &sc->ppsinfo.assert_timestamp);
				}

#ifdef PPS_SYNC
				if (sc->ppsparam.mode & PPS_HARDPPSONASSERT)
					hardpps(&tv, tv.tv_usec);
#endif
				sc->ppsinfo.assert_sequence++;
				sc->ppsinfo.current_mode = sc->ppsparam.mode;

			} else if ((msr & sc->sc_ppsmask) == sc->sc_ppsclear) {
				/* XXX nanotime() */
				microtime(&tv);
				TIMEVAL_TO_TIMESPEC(&tv, 
				    &sc->ppsinfo.clear_timestamp);
				if (sc->ppsparam.mode & PPS_OFFSETCLEAR) {
					timespecadd(&sc->ppsinfo.clear_timestamp,
					    &sc->ppsparam.clear_offset,
					    &sc->ppsinfo.clear_timestamp);
				}

#ifdef PPS_SYNC
				if (sc->ppsparam.mode & PPS_HARDPPSONCLEAR)
					hardpps(&tv, tv.tv_usec);
#endif
				sc->ppsinfo.clear_sequence++;
				sc->ppsinfo.current_mode = sc->ppsparam.mode;
			}
		}
#endif

		/*
		 * Process normal status changes
		 */
		if (ISSET(delta, sc->sc_msr_mask)) {
			SET(sc->sc_msr_delta, delta);

			/*
			 * Stop output immediately if we lose the output
			 * flow control signal or carrier detect.
			 */
			if (ISSET(~msts, sc->sc_msr_mask)) {
				sc->sc_tbc = 0;
				sc->sc_heldtbc = 0;
#ifdef SSCOM_DEBUG
				if (sscom_debug)
					sscomstatus(sc, "sscomintr  ");
#endif
			}

			sc->sc_st_check = 1;
		}

		/* 
		 * Done handling any receive interrupts. 
		 */

		/*
		 * If we've delayed a parameter change, do it
		 * now, and restart * output.
		 */
		if ((ufstat & UFSTAT_TXCOUNT) == 0) {
			/* XXX: we should check transmitter empty also */

			if (sc->sc_heldchange) {
				sscom_loadchannelregs(sc);
				sc->sc_heldchange = 0;
				sc->sc_tbc = sc->sc_heldtbc;
				sc->sc_heldtbc = 0;
			}
		}


	} while (0);

	SSCOM_UNLOCK(sc);

	/* Wake up the poller. */
	softint_schedule(sc->sc_si);

#ifdef RND_COM
	rnd_add_uint32(&sc->rnd_source, iir | rsr);
#endif

	return 1;
}

int
sscomtxintr(void *arg)
{
	struct sscom_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	uint32_t ufstat;

	if (SSCOM_ISALIVE(sc) == 0)
		return 0;

	SSCOM_LOCK(sc);

	ufstat = bus_space_read_4(iot, ioh, SSCOM_UFSTAT);

	/*
	 * If we've delayed a parameter change, do it
	 * now, and restart * output.
	 */
	if (sc->sc_heldchange && (ufstat & UFSTAT_TXCOUNT) == 0) {
		/* XXX: we should check transmitter empty also */
		sscom_loadchannelregs(sc);
		sc->sc_heldchange = 0;
		sc->sc_tbc = sc->sc_heldtbc;
		sc->sc_heldtbc = 0;
	}

	/*
	 * See if data can be transmitted as well. Schedule tx
	 * done event if no data left and tty was marked busy.
	 */
	if (!ISSET(ufstat,UFSTAT_TXFULL)) {
		/* 
		 * Output the next chunk of the contiguous
		 * buffer, if any.
		 */
		if (sc->sc_tbc > 0) {
			__sscom_output_chunk(sc, ufstat);
		}
		else {
			/*
			 * Disable transmit sscompletion
			 * interrupts if necessary.
			 */
			if (sc->sc_hwflags & SSCOM_HW_TXINT)
				sscom_disable_txint(sc);
			if (sc->sc_tx_busy) {
				sc->sc_tx_busy = 0;
				sc->sc_tx_done = 1;
			}
		}
	}

	SSCOM_UNLOCK(sc);

	/* Wake up the poller. */
	softint_schedule(sc->sc_si);

#ifdef RND_COM
	rnd_add_uint32(&sc->rnd_source, iir | rsr);
#endif

	return 1;
}

int
sscomintr(void *v)
{
	struct sscom_softc *sc = v;
	int clear = 0;

	if (sscomrxintr(v))
		clear |= SSCOM_HW_RXINT;
	if (sscomtxintr(v))
		clear |= SSCOM_HW_TXINT;

	if (clear)
		sc->sc_clear_interrupts(sc, clear);

	return clear? 1: 0;
}


#if defined(KGDB) || \
    defined(SSCOM0CONSOLE) || defined(SSCOM1CONSOLE) || \
    defined(SSCOM2CONSOLE) || defined(SSCOM3CONSOLE)
/*
 * Initialize UART for use as console or KGDB line.
 */
static int
sscom_init(bus_space_tag_t iot, bus_space_handle_t base_ioh,
    const struct sscom_uart_info *config,
    int rate, int frequency, tcflag_t cflag, bus_space_handle_t *iohp)
{
	bus_space_handle_t ioh;
	bus_addr_t iobase = config->iobase;
	int timo = 150000;

	bus_space_subregion(iot, base_ioh, iobase, SSCOM_SIZE, &ioh);

	/* wait until all is transmitted until we enable this device */
	while (!(bus_space_read_4(iot, ioh, SSCOM_UTRSTAT) &
		UTRSTAT_TXSHIFTER_EMPTY) && --timo);

	/* reset UART control */
	bus_space_write_4(iot, ioh, SSCOM_UCON, 0);

	/* set RX/TX trigger to half values */
	bus_space_write_4(iot, ioh, SSCOM_UFCON,
	    __SHIFTIN(4, UFCON_TXTRIGGER) |
	    __SHIFTIN(4, UFCON_RXTRIGGER) |
	    UFCON_FIFO_ENABLE | 
	    UFCON_TXFIFO_RESET|
	    UFCON_RXFIFO_RESET);
	/* tx/rx fifo reset are auto-cleared */

	rate = sscomspeed(rate, frequency);
	bus_space_write_4(iot, ioh, SSCOM_UBRDIV, rate);
	bus_space_write_4(iot, ioh, SSCOM_ULCON, cflag2lcr(cflag));

	/* enable UART */
	bus_space_write_4(iot, ioh, SSCOM_UCON, 
	    UCON_TXMODE_INT|UCON_RXMODE_INT);
	bus_space_write_4(iot, ioh, SSCOM_UMCON, UMCON_RTS);

	*iohp = ioh;
	return 0;
}

#endif

#if \
    defined(SSCOM0CONSOLE) || defined(SSCOM1CONSOLE) || \
    defined(SSCOM2CONSOLE) || defined(SSCOM3CONSOLE)
/*
 * Following are all routines needed for SSCOM to act as console
 */
struct consdev sscomcons = {
	NULL, NULL, sscomcngetc, sscomcnputc, sscomcnpollc, NULL,
	NULL, NULL, NODEV, CN_NORMAL
};


int
sscom_cnattach(bus_space_tag_t iot, bus_space_handle_t ioh,
    const struct sscom_uart_info *config,
    int rate, int frequency, tcflag_t cflag)
{
	int res;

	res = sscom_init(iot, ioh, config, rate, frequency, cflag,
	    &sscomconsioh);
	if (res)
		return res;

	cn_tab = &sscomcons;
	cn_init_magic(&sscom_cnm_state);
	cn_set_magic("\047\001"); /* default magic is BREAK */

	sscomconstag = iot;
	sscomconsunit = config->unit;
	sscomconsrate = rate;
	sscomconscflag = cflag;

	return 0;
}

void
sscom_cndetach(void)
{
	bus_space_unmap(sscomconstag, sscomconsioh, SSCOM_SIZE);
	sscomconstag = NULL;

	cn_tab = NULL;
}

/*
 * The read-ahead code is so that you can detect pending in-band
 * cn_magic in polled mode while doing output rather than having to
 * wait until the kernel decides it needs input.
 */

#define MAX_READAHEAD	20
static int sscom_readahead[MAX_READAHEAD];
static int sscom_readaheadcount = 0;

int
sscomcngetc(dev_t dev)
{
	int s = splserial();
	u_char __attribute__((__unused__)) stat;
	u_char c;

	/* got a character from reading things earlier */
	if (sscom_readaheadcount > 0) {
		int i;

		c = sscom_readahead[0];
		for (i = 1; i < sscom_readaheadcount; i++) {
			sscom_readahead[i-1] = sscom_readahead[i];
		}
		sscom_readaheadcount--;
		splx(s);
		return c;
	}

	/* block until a character becomes available */
	while (!sscom_rxrdy(sscomconstag, sscomconsioh))
		;

	c = sscom_getc(sscomconstag, sscomconsioh);
	stat = sscom_geterr(sscomconstag, sscomconsioh);
	{
		int __attribute__((__unused__))cn_trapped = 0;
#ifdef DDB
		extern int db_active;
		if (!db_active)
#endif
			cn_check_magic(dev, c, sscom_cnm_state);
	}
	splx(s);
	return c;
}

/*
 * Console kernel output character routine.
 */
void
sscomcnputc(dev_t dev, int c)
{
	int s = splserial();
	int timo;

	int cin;
	int __attribute__((__unused__)) stat;
	if (sscom_readaheadcount < MAX_READAHEAD && 
	    sscom_rxrdy(sscomconstag, sscomconsioh)) {
	    
		int __attribute__((__unused__))cn_trapped = 0;
		cin = sscom_getc(sscomconstag, sscomconsioh);
		stat = sscom_geterr(sscomconstag, sscomconsioh);
		cn_check_magic(dev, cin, sscom_cnm_state);
		sscom_readahead[sscom_readaheadcount++] = cin;
	}

	/* wait for any pending transmission to finish */
	timo = 150000;
	while (ISSET(bus_space_read_4(sscomconstag, sscomconsioh, SSCOM_UFSTAT), 
		   UFSTAT_TXFULL) && --timo)
		continue;

	bus_space_write_1(sscomconstag, sscomconsioh, SSCOM_UTXH, c);
	SSCOM_BARRIER(sscomconstag, sscomconsioh, BR | BW);

#if 0	
	/* wait for this transmission to complete */
	timo = 1500000;
	while (!ISSET(bus_space_read_4(sscomconstag, sscomconsioh, SSCOM_UTRSTAT), 
		   UTRSTAT_TXEMPTY) && --timo)
		continue;
#endif
	splx(s);
}

void
sscomcnpollc(dev_t dev, int on)
{

	sscom_readaheadcount = 0;
}

#endif /* SSCOM0CONSOLE||SSCOM1CONSOLE */

#ifdef KGDB
int
sscom_kgdb_attach(bus_space_tag_t iot, bus_space_handle_t ioh,
    const struct sscom_uart_info *config,
    int rate, int frequency, tcflag_t cflag)
{
	int res;

	if (iot == sscomconstag && config->unit == sscomconsunit) {
		printf( "console==kgdb_port (%d): kgdb disabled\n", sscomconsunit);
		return EBUSY; /* cannot share with console */
	}

	res = sscom_init(iot, ioh, config,
		rate, frequency, cflag, &sscom_kgdb_ioh);
	if (res)
		return res;

	kgdb_attach(sscom_kgdb_getc, sscom_kgdb_putc, NULL);
	kgdb_dev = 123; /* unneeded, only to satisfy some tests */

	sscom_kgdb_iot = iot;
	sscom_kgdb_unit = config->unit;

	return 0;
}

/* ARGSUSED */
int
sscom_kgdb_getc(void *arg)
{
	int c, stat;

	/* block until a character becomes available */
	while (!sscom_rxrdy(sscom_kgdb_iot, sscom_kgdb_ioh))
		;

	c = sscom_getc(sscom_kgdb_iot, sscom_kgdb_ioh);
	stat = sscom_geterr(sscom_kgdb_iot, sscom_kgdb_ioh);

	return c;
}

/* ARGSUSED */
void
sscom_kgdb_putc(void *arg, int c)
{
	int timo;

	/* wait for any pending transmission to finish */
	timo = 150000;
	while (ISSET(bus_space_read_4(sscom_kgdb_iot, sscom_kgdb_ioh,
	    SSCOM_UFSTAT), UFSTAT_TXFULL) && --timo)
		continue;

	bus_space_write_1(sscom_kgdb_iot, sscom_kgdb_ioh, SSCOM_UTXH, c);
	SSCOM_BARRIER(sscom_kgdb_iot, sscom_kgdb_ioh, BR | BW);

#if 0	
	/* wait for this transmission to complete */
	timo = 1500000;
	while (!ISSET(bus_space_read_4(sscom_kgdb_iot, sscom_kgdb_ioh,
	    SSCOM_UTRSTAT), UTRSTAT_TXEMPTY) && --timo)
		continue;
#endif
}
#endif /* KGDB */

/* helper function to identify the sscom ports used by
 console or KGDB (and not yet autoconf attached) */
int
sscom_is_console(bus_space_tag_t iot, int unit,
    bus_space_handle_t *ioh)
{
	bus_space_handle_t help;

	if (!sscomconsattached &&
	    iot == sscomconstag && unit == sscomconsunit)
		help = sscomconsioh;
#ifdef KGDB
	else if (!sscom_kgdb_attached &&
	    iot == sscom_kgdb_iot && unit == sscom_kgdb_unit)
		help = sscom_kgdb_ioh;
#endif
	else
		return 0;

	if (ioh)
		*ioh = help;
	return 1;
}


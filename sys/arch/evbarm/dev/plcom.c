/*	$NetBSD: plcom.c,v 1.10.2.5 2008/01/21 09:36:09 yamt Exp $	*/

/*-
 * Copyright (c) 2001 ARM Ltd
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
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
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
 * COM driver for the Prime Cell PL010 UART, which is similar to the 16C550,
 * but has a completely different programmer's model.
 * Derived from the NS16550AF com driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: plcom.c,v 1.10.2.5 2008/01/21 09:36:09 yamt Exp $");

#include "opt_plcom.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"

#include "rnd.h"
#if NRND > 0 && defined(RND_COM)
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
#include <sys/user.h>
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

#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>

#include <dev/cons.h>

static void plcom_enable_debugport (struct plcom_softc *);

void	plcom_config	(struct plcom_softc *);
void	plcom_shutdown	(struct plcom_softc *);
int	plcomspeed	(long, long);
static	u_char	cflag2lcr (tcflag_t);
int	plcomparam	(struct tty *, struct termios *);
void	plcomstart	(struct tty *);
int	plcomhwiflow	(struct tty *, int);

void	plcom_loadchannelregs (struct plcom_softc *);
void	plcom_hwiflow	(struct plcom_softc *);
void	plcom_break	(struct plcom_softc *, int);
void	plcom_modem	(struct plcom_softc *, int);
void	tiocm_to_plcom	(struct plcom_softc *, u_long, int);
int	plcom_to_tiocm	(struct plcom_softc *);
void	plcom_iflush	(struct plcom_softc *);

int	plcom_common_getc (dev_t, bus_space_tag_t, bus_space_handle_t);
void	plcom_common_putc (dev_t, bus_space_tag_t, bus_space_handle_t, int);

int	plcominit	(bus_space_tag_t, bus_addr_t, int, int, tcflag_t,
			    bus_space_handle_t *);

dev_type_open(plcomopen);
dev_type_close(plcomclose);
dev_type_read(plcomread);
dev_type_write(plcomwrite);
dev_type_ioctl(plcomioctl);
dev_type_stop(plcomstop);
dev_type_tty(plcomtty);
dev_type_poll(plcompoll);

int	plcomcngetc	(dev_t);
void	plcomcnputc	(dev_t, int);
void	plcomcnpollc	(dev_t, int);

#define	integrate	static inline
void 	plcomsoft	(void *);
integrate void plcom_rxsoft	(struct plcom_softc *, struct tty *);
integrate void plcom_txsoft	(struct plcom_softc *, struct tty *);
integrate void plcom_stsoft	(struct plcom_softc *, struct tty *);
integrate void plcom_schedrx	(struct plcom_softc *);
void	plcomdiag		(void *);

extern struct cfdriver plcom_cd;

const struct cdevsw plcom_cdevsw = {
	plcomopen, plcomclose, plcomread, plcomwrite, plcomioctl,
	plcomstop, plcomtty, plcompoll, nommap, ttykqfilter, D_TTY
};

/*
 * Make this an option variable one can patch.
 * But be warned:  this must be a power of 2!
 */
u_int plcom_rbuf_size = PLCOM_RING_SIZE;

/* Stop input when 3/4 of the ring is full; restart when only 1/4 is full. */
u_int plcom_rbuf_hiwat = (PLCOM_RING_SIZE * 1) / 4;
u_int plcom_rbuf_lowat = (PLCOM_RING_SIZE * 3) / 4;

static int	plcomconsunit = -1;
static bus_space_tag_t plcomconstag;
static bus_space_handle_t plcomconsioh;
static int	plcomconsattached;
static int plcomconsrate;
static tcflag_t plcomconscflag;
static struct cnm_state plcom_cnm_state;

static int ppscap =
	PPS_TSFMT_TSPEC |
	PPS_CAPTUREASSERT | 
	PPS_CAPTURECLEAR |
#ifdef  PPS_SYNC 
	PPS_HARDPPSONASSERT | PPS_HARDPPSONCLEAR |
#endif	/* PPS_SYNC */
	PPS_OFFSETASSERT | PPS_OFFSETCLEAR;

#ifdef KGDB
#include <sys/kgdb.h>

static int plcom_kgdb_unit;
static bus_space_tag_t plcom_kgdb_iot;
static bus_space_handle_t plcom_kgdb_ioh;
static int plcom_kgdb_attached;

int	plcom_kgdb_getc (void *);
void	plcom_kgdb_putc (void *, int);
#endif /* KGDB */

#define	PLCOMUNIT_MASK	0x7ffff
#define	PLCOMDIALOUT_MASK	0x80000

#define	PLCOMUNIT(x)	(minor(x) & PLCOMUNIT_MASK)
#define	PLCOMDIALOUT(x)	(minor(x) & PLCOMDIALOUT_MASK)

#define	PLCOM_ISALIVE(sc)	((sc)->enabled != 0 && \
				 device_is_active(&(sc)->sc_dev))

#define	BR	BUS_SPACE_BARRIER_READ
#define	BW	BUS_SPACE_BARRIER_WRITE
#define PLCOM_BARRIER(t, h, f) bus_space_barrier((t), (h), 0, PLCOM_UART_SIZE, (f))

#define PLCOM_LOCK(sc) simple_lock(&(sc)->sc_lock)
#define PLCOM_UNLOCK(sc) simple_unlock(&(sc)->sc_lock)

int
plcomspeed(long speed, long frequency)
{
#define	divrnd(n, q)	(((n)*2/(q)+1)/2)	/* divide and round off */

	int x, err;

#if 0
	if (speed == 0)
		return 0;
#endif
	if (speed <= 0)
		return -1;
	x = divrnd(frequency / 16, speed);
	if (x <= 0)
		return -1;
	err = divrnd(((quad_t)frequency) * 1000 / 16, speed * x) - 1000;
	if (err < 0)
		err = -err;
	if (err > PLCOM_TOLERANCE)
		return -1;
	return x;

#undef	divrnd
}

#ifdef PLCOM_DEBUG
int	plcom_debug = 0;

void plcomstatus (struct plcom_softc *, char *);
void
plcomstatus(struct plcom_softc *sc, char *str)
{
	struct tty *tp = sc->sc_tty;

	printf("%s: %s %sclocal  %sdcd %sts_carr_on %sdtr %stx_stopped\n",
	    sc->sc_dev.dv_xname, str,
	    ISSET(tp->t_cflag, CLOCAL) ? "+" : "-",
	    ISSET(sc->sc_msr, MSR_DCD) ? "+" : "-",
	    ISSET(tp->t_state, TS_CARR_ON) ? "+" : "-",
	    ISSET(sc->sc_mcr, MCR_DTR) ? "+" : "-",
	    sc->sc_tx_stopped ? "+" : "-");

	printf("%s: %s %scrtscts %scts %sts_ttstop  %srts %xrx_flags\n",
	    sc->sc_dev.dv_xname, str,
	    ISSET(tp->t_cflag, CRTSCTS) ? "+" : "-",
	    ISSET(sc->sc_msr, MSR_CTS) ? "+" : "-",
	    ISSET(tp->t_state, TS_TTSTOP) ? "+" : "-",
	    ISSET(sc->sc_mcr, MCR_RTS) ? "+" : "-",
	    sc->sc_rx_flags);
}
#endif

int
plcomprobe1(bus_space_tag_t iot, bus_space_handle_t ioh)
{
	int data;

	/* Disable the UART.  */
	bus_space_write_1(iot, ioh, plcom_cr, 0);
	/* Make sure the FIFO is off.  */
	bus_space_write_1(iot, ioh, plcom_lcr, LCR_8BITS);
	/* Disable interrupts.  */
	bus_space_write_1(iot, ioh, plcom_iir, 0);

	/* Make sure we swallow anything in the receiving register.  */
	data = bus_space_read_1(iot, ioh, plcom_dr);

	if (bus_space_read_1(iot, ioh, plcom_lcr) != LCR_8BITS)
		return 0;

	data = bus_space_read_1(iot, ioh, plcom_fr) & (FR_RXFF | FR_RXFE);

	if (data != FR_RXFE)
		return 0;

	return 1;
}

static void
plcom_enable_debugport(struct plcom_softc *sc)
{
	int s;

	/* Turn on line break interrupt, set carrier. */
	s = splserial();
	PLCOM_LOCK(sc);
	sc->sc_cr = CR_RIE | CR_RTIE | CR_UARTEN;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, plcom_cr, sc->sc_cr);
	SET(sc->sc_mcr, MCR_DTR | MCR_RTS);
	/* XXX device_unit() abuse */
	sc->sc_set_mcr(sc->sc_set_mcr_arg, device_unit(&sc->sc_dev),
	    sc->sc_mcr);
	PLCOM_UNLOCK(sc);
	splx(s);
}

void
plcom_attach_subr(struct plcom_softc *sc)
{
	int unit = sc->sc_iounit;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct tty *tp;

	callout_init(&sc->sc_diag_callout, 0);
	simple_lock_init(&sc->sc_lock);

	/* Disable interrupts before configuring the device. */
	sc->sc_cr = 0;

	if (plcomconstag && unit == plcomconsunit) {
		plcomconsattached = 1;

		plcomconstag = iot;
		plcomconsioh = ioh;

		/* Make sure the console is always "hardwired". */
		delay(1000);			/* wait for output to finish */
		SET(sc->sc_hwflags, PLCOM_HW_CONSOLE);
		SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);
		/* Must re-enable the console immediately, or we will
		   hang when trying to print.  */
		sc->sc_cr = CR_UARTEN;
	}

	bus_space_write_1(iot, ioh, plcom_cr, sc->sc_cr);

	/* The PL010 has a 16-byte fifo, but the tx interrupt triggers when
	   there is space for 8 more bytes.  */
	sc->sc_fifolen = 8;
	printf("\n");

	if (ISSET(sc->sc_hwflags, PLCOM_HW_TXFIFO_DISABLE)) {
		sc->sc_fifolen = 1;
		printf("%s: txfifo disabled\n", sc->sc_dev.dv_xname);
	}

	if (sc->sc_fifolen > 1)
		SET(sc->sc_hwflags, PLCOM_HW_FIFO);

	tp = ttymalloc();
	tp->t_oproc = plcomstart;
	tp->t_param = plcomparam;
	tp->t_hwiflow = plcomhwiflow;

	sc->sc_tty = tp;
	sc->sc_rbuf = malloc(plcom_rbuf_size << 1, M_DEVBUF, M_NOWAIT);
	sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
	sc->sc_rbavail = plcom_rbuf_size;
	if (sc->sc_rbuf == NULL) {
		printf("%s: unable to allocate ring buffer\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	sc->sc_ebuf = sc->sc_rbuf + (plcom_rbuf_size << 1);

	tty_attach(tp);

	if (ISSET(sc->sc_hwflags, PLCOM_HW_CONSOLE)) {
		int maj;

		/* locate the major number */
		maj = cdevsw_lookup_major(&plcom_cdevsw);

		cn_tab->cn_dev = makedev(maj, device_unit(&sc->sc_dev));

		printf("%s: console\n", sc->sc_dev.dv_xname);
	}

#ifdef KGDB
	/*
	 * Allow kgdb to "take over" this port.  If this is
	 * the kgdb device, it has exclusive use.
	 */
	if (iot == plcom_kgdb_iot && unit == plcom_kgdb_unit) {
		plcom_kgdb_attached = 1;

		SET(sc->sc_hwflags, PLCOM_HW_KGDB);
		printf("%s: kgdb\n", sc->sc_dev.dv_xname);
	}
#endif

	sc->sc_si = softint_establish(SOFTINT_SERIAL, plcomsoft, sc);

#if NRND > 0 && defined(RND_COM)
	rnd_attach_source(&sc->rnd_source, sc->sc_dev.dv_xname,
			  RND_TYPE_TTY, 0);
#endif

	/* if there are no enable/disable functions, assume the device
	   is always enabled */
	if (!sc->enable)
		sc->enabled = 1;

	plcom_config(sc);

	SET(sc->sc_hwflags, PLCOM_HW_DEV_OK);
}

void
plcom_config(struct plcom_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	/* Disable interrupts before configuring the device. */
	sc->sc_cr = 0;
	bus_space_write_1(iot, ioh, plcom_cr, sc->sc_cr);

	if (ISSET(sc->sc_hwflags, PLCOM_HW_CONSOLE|PLCOM_HW_KGDB))
		plcom_enable_debugport(sc);
}

int
plcom_detach(self, flags)
	struct device *self;
	int flags;
{
	struct plcom_softc *sc = (struct plcom_softc *)self;
	int maj, mn;

	/* locate the major number */
	maj = cdevsw_lookup_major(&plcom_cdevsw);

	/* Nuke the vnodes for any open instances. */
	mn = device_unit(self);
	vdevgone(maj, mn, mn, VCHR);

	mn |= PLCOMDIALOUT_MASK;
	vdevgone(maj, mn, mn, VCHR);

	/* Free the receive buffer. */
	free(sc->sc_rbuf, M_DEVBUF);

	/* Detach and free the tty. */
	tty_detach(sc->sc_tty);
	ttyfree(sc->sc_tty);

	/* Unhook the soft interrupt handler. */
	softint_disestablish(sc->sc_si);

#if NRND > 0 && defined(RND_COM)
	/* Unhook the entropy source. */
	rnd_detach_source(&sc->rnd_source);
#endif

	return 0;
}

int
plcom_activate(struct device *self, enum devact act)
{
	struct plcom_softc *sc = (struct plcom_softc *)self;
	int s, rv = 0;

	s = splserial();
	PLCOM_LOCK(sc);
	switch (act) {
	case DVACT_ACTIVATE:
		rv = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		if (sc->sc_hwflags & (PLCOM_HW_CONSOLE|PLCOM_HW_KGDB)) {
			rv = EBUSY;
			break;
		}

		if (sc->disable != NULL && sc->enabled != 0) {
			(*sc->disable)(sc);
			sc->enabled = 0;
		}
		break;
	}

	PLCOM_UNLOCK(sc);	
	splx(s);
	return rv;
}

void
plcom_shutdown(struct plcom_softc *sc)
{
	struct tty *tp = sc->sc_tty;
	int s;

	s = splserial();
	PLCOM_LOCK(sc);	

	/* If we were asserting flow control, then deassert it. */
	SET(sc->sc_rx_flags, RX_IBUF_BLOCKED);
	plcom_hwiflow(sc);

	/* Clear any break condition set with TIOCSBRK. */
	plcom_break(sc, 0);

	/* Turn off PPS capture on last close. */
	sc->sc_ppsmask = 0;
	sc->ppsparam.mode = 0;

	/*
	 * Hang up if necessary.  Wait a bit, so the other side has time to
	 * notice even if we immediately open the port again.
	 * Avoid tsleeping above splhigh().
	 */
	if (ISSET(tp->t_cflag, HUPCL)) {
		plcom_modem(sc, 0);
		PLCOM_UNLOCK(sc);
		splx(s);
		/* XXX tsleep will only timeout */
		(void) tsleep(sc, TTIPRI, ttclos, hz);
		s = splserial();
		PLCOM_LOCK(sc);	
	}

	/* Turn off interrupts. */
	if (ISSET(sc->sc_hwflags, PLCOM_HW_CONSOLE))
		/* interrupt on break */
		sc->sc_cr = CR_RIE | CR_RTIE | CR_UARTEN;
	else
		sc->sc_cr = 0;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, plcom_cr, sc->sc_cr);

	if (sc->disable) {
#ifdef DIAGNOSTIC
		if (!sc->enabled)
			panic("plcom_shutdown: not enabled?");
#endif
		(*sc->disable)(sc);
		sc->enabled = 0;
	}
	PLCOM_UNLOCK(sc);
	splx(s);
}

int
plcomopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct plcom_softc *sc;
	struct tty *tp;
	int s, s2;
	int error;

	sc = device_lookup(&plcom_cd, PLCOMUNIT(dev));
	if (sc == NULL || !ISSET(sc->sc_hwflags, PLCOM_HW_DEV_OK) ||
		sc->sc_rbuf == NULL)
		return ENXIO;

	if (!device_is_active(&sc->sc_dev))
		return ENXIO;

#ifdef KGDB
	/*
	 * If this is the kgdb port, no other use is permitted.
	 */
	if (ISSET(sc->sc_hwflags, PLCOM_HW_KGDB))
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
		PLCOM_LOCK(sc);

		if (sc->enable) {
			if ((*sc->enable)(sc)) {
				PLCOM_UNLOCK(sc);
				splx(s2);
				splx(s);
				printf("%s: device enable failed\n",
				       sc->sc_dev.dv_xname);
				return EIO;
			}
			sc->enabled = 1;
			plcom_config(sc);
		}

		/* Turn on interrupts. */
		/* IER_ERXRDY | IER_ERLS | IER_EMSC;  */
		sc->sc_cr = CR_RIE | CR_RTIE | CR_MSIE | CR_UARTEN;
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, plcom_cr, sc->sc_cr);

		/* Fetch the current modem control status, needed later. */
		sc->sc_msr = bus_space_read_1(sc->sc_iot, sc->sc_ioh, plcom_fr);

		/* Clear PPS capture state on first open. */
		sc->sc_ppsmask = 0;
		sc->ppsparam.mode = 0;

		PLCOM_UNLOCK(sc);
		splx(s2);

		/*
		 * Initialize the termios status to the defaults.  Add in the
		 * sticky bits from TIOCSFLAGS.
		 */
		t.c_ispeed = 0;
		if (ISSET(sc->sc_hwflags, PLCOM_HW_CONSOLE)) {
			t.c_ospeed = plcomconsrate;
			t.c_cflag = plcomconscflag;
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
		/* Make sure plcomparam() will do something. */
		tp->t_ospeed = 0;
		(void) plcomparam(tp, &t);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		ttychars(tp);
		ttsetwater(tp);

		s2 = splserial();
		PLCOM_LOCK(sc);

		/*
		 * Turn on DTR.  We must always do this, even if carrier is not
		 * present, because otherwise we'd have to use TIOCSDTR
		 * immediately after setting CLOCAL, which applications do not
		 * expect.  We always assert DTR while the device is open
		 * unless explicitly requested to deassert it.
		 */
		plcom_modem(sc, 1);

		/* Clear the input ring, and unblock. */
		sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
		sc->sc_rbavail = plcom_rbuf_size;
		plcom_iflush(sc);
		CLR(sc->sc_rx_flags, RX_ANY_BLOCK);
		plcom_hwiflow(sc);

#ifdef PLCOM_DEBUG
		if (plcom_debug)
			plcomstatus(sc, "plcomopen  ");
#endif

		PLCOM_UNLOCK(sc);
		splx(s2);
	}
	
	splx(s);

	error = ttyopen(tp, PLCOMDIALOUT(dev), ISSET(flag, O_NONBLOCK));
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
		plcom_shutdown(sc);
	}

	return error;
}
 
int
plcomclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct plcom_softc *sc = device_lookup(&plcom_cd, PLCOMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	/* XXX This is for cons.c. */
	if (!ISSET(tp->t_state, TS_ISOPEN))
		return 0;

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);

	if (PLCOM_ISALIVE(sc) == 0)
		return 0;

	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * Although we got a last close, the device may still be in
		 * use; e.g. if this was the dialout node, and there are still
		 * processes waiting for carrier on the non-dialout node.
		 */
		plcom_shutdown(sc);
	}

	return 0;
}
 
int
plcomread(dev_t dev, struct uio *uio, int flag)
{
	struct plcom_softc *sc = device_lookup(&plcom_cd, PLCOMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (PLCOM_ISALIVE(sc) == 0)
		return EIO;
 
	return (*tp->t_linesw->l_read)(tp, uio, flag);
}
 
int
plcomwrite(dev_t dev, struct uio *uio, int flag)
{
	struct plcom_softc *sc = device_lookup(&plcom_cd, PLCOMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (PLCOM_ISALIVE(sc) == 0)
		return EIO;
 
	return (*tp->t_linesw->l_write)(tp, uio, flag);
}

int
plcompoll(dev_t dev, int events, struct lwp *l)
{
	struct plcom_softc *sc = device_lookup(&plcom_cd, PLCOMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (PLCOM_ISALIVE(sc) == 0)
		return EIO;
 
	return (*tp->t_linesw->l_poll)(tp, events, l);
}

struct tty *
plcomtty(dev_t dev)
{
	struct plcom_softc *sc = device_lookup(&plcom_cd, PLCOMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	return tp;
}

int
plcomioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct plcom_softc *sc = device_lookup(&plcom_cd, PLCOMUNIT(dev));
	struct tty *tp = sc->sc_tty;
	int error;
	int s;

	if (PLCOM_ISALIVE(sc) == 0)
		return EIO;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	error = ttioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	error = 0;

	s = splserial();
	PLCOM_LOCK(sc);	

	switch (cmd) {
	case TIOCSBRK:
		plcom_break(sc, 1);
		break;

	case TIOCCBRK:
		plcom_break(sc, 0);
		break;

	case TIOCSDTR:
		plcom_modem(sc, 1);
		break;

	case TIOCCDTR:
		plcom_modem(sc, 0);
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
		tiocm_to_plcom(sc, cmd, *(int *)data);
		break;

	case TIOCMGET:
		*(int *)data = plcom_to_tiocm(sc);
		break;

	case PPS_IOC_CREATE:
		break;

	case PPS_IOC_DESTROY:
		break;

	case PPS_IOC_GETPARAMS: {
		pps_params_t *pp;
		pp = (pps_params_t *)data;
		*pp = sc->ppsparam;
		break;
	}

	case PPS_IOC_SETPARAMS: {
	  	pps_params_t *pp;
		int mode;
		pp = (pps_params_t *)data;
		if (pp->mode & ~ppscap) {
			error = EINVAL;
			break;
		}
		sc->ppsparam = *pp;
	 	/* 
		 * Compute msr masks from user-specified timestamp state.
		 */
		mode = sc->ppsparam.mode;
#ifdef	PPS_SYNC
		if (mode & PPS_HARDPPSONASSERT) {
			mode |= PPS_CAPTUREASSERT;
			/* XXX revoke any previous HARDPPS source */
		}
		if (mode & PPS_HARDPPSONCLEAR) {
			mode |= PPS_CAPTURECLEAR;
			/* XXX revoke any previous HARDPPS source */
		}
#endif	/* PPS_SYNC */
		switch (mode & PPS_CAPTUREBOTH) {
		case 0:
			sc->sc_ppsmask = 0;
			break;
	
		case PPS_CAPTUREASSERT:
			sc->sc_ppsmask = MSR_DCD;
			sc->sc_ppsassert = MSR_DCD;
			sc->sc_ppsclear = -1;
			break;
	
		case PPS_CAPTURECLEAR:
			sc->sc_ppsmask = MSR_DCD;
			sc->sc_ppsassert = -1;
			sc->sc_ppsclear = 0;
			break;

		case PPS_CAPTUREBOTH:
			sc->sc_ppsmask = MSR_DCD;
			sc->sc_ppsassert = MSR_DCD;
			sc->sc_ppsclear = 0;
			break;

		default:
			error = EINVAL;
			break;
		}
		break;
	}

	case PPS_IOC_GETCAP:
		*(int*)data = ppscap;
		break;

	case PPS_IOC_FETCH: {
		pps_info_t *pi;
		pi = (pps_info_t *)data;
		*pi = sc->ppsinfo;
		break;
	}

	case TIOCDCDTIMESTAMP:	/* XXX old, overloaded  API used by xntpd v3 */
		/*
		 * Some GPS clocks models use the falling rather than
		 * rising edge as the on-the-second signal. 
		 * The old API has no way to specify PPS polarity.
		 */
		sc->sc_ppsmask = MSR_DCD;
#ifndef PPS_TRAILING_EDGE
		sc->sc_ppsassert = MSR_DCD;
		sc->sc_ppsclear = -1;
		TIMESPEC_TO_TIMEVAL((struct timeval *)data, 
		    &sc->ppsinfo.assert_timestamp);
#else
		sc->sc_ppsassert = -1
		sc->sc_ppsclear = 0;
		TIMESPEC_TO_TIMEVAL((struct timeval *)data, 
		    &sc->ppsinfo.clear_timestamp);
#endif
		break;

	default:
		error = EPASSTHROUGH;
		break;
	}

	PLCOM_UNLOCK(sc);
	splx(s);

#ifdef PLCOM_DEBUG
	if (plcom_debug)
		plcomstatus(sc, "plcomioctl ");
#endif

	return error;
}

integrate void
plcom_schedrx(struct plcom_softc *sc)
{

	sc->sc_rx_ready = 1;

	/* Wake up the poller. */
	softint_schedule(sc->sc_si);
}

void
plcom_break(struct plcom_softc *sc, int onoff)
{

	if (onoff)
		SET(sc->sc_lcr, LCR_BRK);
	else
		CLR(sc->sc_lcr, LCR_BRK);

	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			plcom_loadchannelregs(sc);
	}
}

void
plcom_modem(struct plcom_softc *sc, int onoff)
{

	if (sc->sc_mcr_dtr == 0)
		return;

	if (onoff)
		SET(sc->sc_mcr, sc->sc_mcr_dtr);
	else
		CLR(sc->sc_mcr, sc->sc_mcr_dtr);

	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			plcom_loadchannelregs(sc);
	}
}

void
tiocm_to_plcom(struct plcom_softc *sc, u_long how, int ttybits)
{
	u_char plcombits;

	plcombits = 0;
	if (ISSET(ttybits, TIOCM_DTR))
		SET(plcombits, MCR_DTR);
	if (ISSET(ttybits, TIOCM_RTS))
		SET(plcombits, MCR_RTS);
 
	switch (how) {
	case TIOCMBIC:
		CLR(sc->sc_mcr, plcombits);
		break;

	case TIOCMBIS:
		SET(sc->sc_mcr, plcombits);
		break;

	case TIOCMSET:
		CLR(sc->sc_mcr, MCR_DTR | MCR_RTS);
		SET(sc->sc_mcr, plcombits);
		break;
	}

	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			plcom_loadchannelregs(sc);
	}
}

int
plcom_to_tiocm(struct plcom_softc *sc)
{
	u_char plcombits;
	int ttybits = 0;

	plcombits = sc->sc_mcr;
	if (ISSET(plcombits, MCR_DTR))
		SET(ttybits, TIOCM_DTR);
	if (ISSET(plcombits, MCR_RTS))
		SET(ttybits, TIOCM_RTS);

	plcombits = sc->sc_msr;
	if (ISSET(plcombits, MSR_DCD))
		SET(ttybits, TIOCM_CD);
	if (ISSET(plcombits, MSR_CTS))
		SET(ttybits, TIOCM_CTS);
	if (ISSET(plcombits, MSR_DSR))
		SET(ttybits, TIOCM_DSR);

	if (sc->sc_cr != 0)
		SET(ttybits, TIOCM_LE);

	return ttybits;
}

static u_char
cflag2lcr(tcflag_t cflag)
{
	u_char lcr = 0;

	switch (ISSET(cflag, CSIZE)) {
	case CS5:
		SET(lcr, LCR_5BITS);
		break;
	case CS6:
		SET(lcr, LCR_6BITS);
		break;
	case CS7:
		SET(lcr, LCR_7BITS);
		break;
	case CS8:
		SET(lcr, LCR_8BITS);
		break;
	}
	if (ISSET(cflag, PARENB)) {
		SET(lcr, LCR_PEN);
		if (!ISSET(cflag, PARODD))
			SET(lcr, LCR_EPS);
	}
	if (ISSET(cflag, CSTOPB))
		SET(lcr, LCR_STP2);

	return lcr;
}

int
plcomparam(struct tty *tp, struct termios *t)
{
	struct plcom_softc *sc = device_lookup(&plcom_cd, PLCOMUNIT(tp->t_dev));
	int ospeed;
	u_char lcr;
	int s;

	if (PLCOM_ISALIVE(sc) == 0)
		return EIO;

	ospeed = plcomspeed(t->c_ospeed, sc->sc_frequency);

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
	    ISSET(sc->sc_hwflags, PLCOM_HW_CONSOLE)) {
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

	lcr = ISSET(sc->sc_lcr, LCR_BRK) | cflag2lcr(t->c_cflag);

	s = splserial();
	PLCOM_LOCK(sc);	

	sc->sc_lcr = lcr;

	/*
	 * PL010 has a fixed-length FIFO trigger point.
	 */
	if (ISSET(sc->sc_hwflags, PLCOM_HW_FIFO))
		sc->sc_fifo = 1;
	else
		sc->sc_fifo = 0;

	if (sc->sc_fifo)
		SET(sc->sc_lcr, LCR_FEN);

	/*
	 * If we're not in a mode that assumes a connection is present, then
	 * ignore carrier changes.
	 */
	if (ISSET(t->c_cflag, CLOCAL | MDMBUF))
		sc->sc_msr_dcd = 0;
	else
		sc->sc_msr_dcd = MSR_DCD;
	/*
	 * Set the flow control pins depending on the current flow control
	 * mode.
	 */
	if (ISSET(t->c_cflag, CRTSCTS)) {
		sc->sc_mcr_dtr = MCR_DTR;
		sc->sc_mcr_rts = MCR_RTS;
		sc->sc_msr_cts = MSR_CTS;
	} else if (ISSET(t->c_cflag, MDMBUF)) {
		/*
		 * For DTR/DCD flow control, make sure we don't toggle DTR for
		 * carrier detection.
		 */
		sc->sc_mcr_dtr = 0;
		sc->sc_mcr_rts = MCR_DTR;
		sc->sc_msr_cts = MSR_DCD;
	} else {
		/*
		 * If no flow control, then always set RTS.  This will make
		 * the other side happy if it mistakenly thinks we're doing
		 * RTS/CTS flow control.
		 */
		sc->sc_mcr_dtr = MCR_DTR | MCR_RTS;
		sc->sc_mcr_rts = 0;
		sc->sc_msr_cts = 0;
		if (ISSET(sc->sc_mcr, MCR_DTR))
			SET(sc->sc_mcr, MCR_RTS);
		else
			CLR(sc->sc_mcr, MCR_RTS);
	}
	sc->sc_msr_mask = sc->sc_msr_cts | sc->sc_msr_dcd;

#if 0
	if (ospeed == 0)
		CLR(sc->sc_mcr, sc->sc_mcr_dtr);
	else
		SET(sc->sc_mcr, sc->sc_mcr_dtr);
#endif

	sc->sc_dlbl = ospeed;
	sc->sc_dlbh = ospeed >> 8;

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
			plcom_loadchannelregs(sc);
	}

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		/* Disable the high water mark. */
		sc->sc_r_hiwat = 0;
		sc->sc_r_lowat = 0;
		if (ISSET(sc->sc_rx_flags, RX_TTY_OVERFLOWED)) {
			CLR(sc->sc_rx_flags, RX_TTY_OVERFLOWED);
			plcom_schedrx(sc);
		}
		if (ISSET(sc->sc_rx_flags, RX_TTY_BLOCKED|RX_IBUF_BLOCKED)) {
			CLR(sc->sc_rx_flags, RX_TTY_BLOCKED|RX_IBUF_BLOCKED);
			plcom_hwiflow(sc);
		}
	} else {
		sc->sc_r_hiwat = plcom_rbuf_hiwat;
		sc->sc_r_lowat = plcom_rbuf_lowat;
	}

	PLCOM_UNLOCK(sc);
	splx(s);

	/*
	 * Update the tty layer's idea of the carrier bit, in case we changed
	 * CLOCAL or MDMBUF.  We don't hang up here; we only do that by
	 * explicit request.
	 */
	(void) (*tp->t_linesw->l_modem)(tp, ISSET(sc->sc_msr, MSR_DCD));

#ifdef PLCOM_DEBUG
	if (plcom_debug)
		plcomstatus(sc, "plcomparam ");
#endif

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		if (sc->sc_tx_stopped) {
			sc->sc_tx_stopped = 0;
			plcomstart(tp);
		}
	}

	return 0;
}

void
plcom_iflush(struct plcom_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
#ifdef DIAGNOSTIC
	int reg;
#endif
	int timo;

#ifdef DIAGNOSTIC
	reg = 0xffff;
#endif
	timo = 50000;
	/* flush any pending I/O */
	while (! ISSET(bus_space_read_1(iot, ioh, plcom_fr), FR_RXFE)
	    && --timo)
#ifdef DIAGNOSTIC
		reg =
#else
		    (void)
#endif
		    bus_space_read_1(iot, ioh, plcom_dr);
#ifdef DIAGNOSTIC
	if (!timo)
		printf("%s: plcom_iflush timeout %02x\n", sc->sc_dev.dv_xname,
		       reg);
#endif
}

void
plcom_loadchannelregs(struct plcom_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	/* XXXXX necessary? */
	plcom_iflush(sc);

	bus_space_write_1(iot, ioh, plcom_cr, 0);

	bus_space_write_1(iot, ioh, plcom_dlbl, sc->sc_dlbl);
	bus_space_write_1(iot, ioh, plcom_dlbh, sc->sc_dlbh);
	bus_space_write_1(iot, ioh, plcom_lcr, sc->sc_lcr);
	/* XXX device_unit() abuse */
	sc->sc_set_mcr(sc->sc_set_mcr_arg, device_unit(&sc->sc_dev),
	    sc->sc_mcr_active = sc->sc_mcr);

	bus_space_write_1(iot, ioh, plcom_cr, sc->sc_cr);
}

int
plcomhwiflow(struct tty *tp, int block)
{
	struct plcom_softc *sc = device_lookup(&plcom_cd, PLCOMUNIT(tp->t_dev));
	int s;

	if (PLCOM_ISALIVE(sc) == 0)
		return 0;

	if (sc->sc_mcr_rts == 0)
		return 0;

	s = splserial();
	PLCOM_LOCK(sc);
	
	if (block) {
		if (!ISSET(sc->sc_rx_flags, RX_TTY_BLOCKED)) {
			SET(sc->sc_rx_flags, RX_TTY_BLOCKED);
			plcom_hwiflow(sc);
		}
	} else {
		if (ISSET(sc->sc_rx_flags, RX_TTY_OVERFLOWED)) {
			CLR(sc->sc_rx_flags, RX_TTY_OVERFLOWED);
			plcom_schedrx(sc);
		}
		if (ISSET(sc->sc_rx_flags, RX_TTY_BLOCKED)) {
			CLR(sc->sc_rx_flags, RX_TTY_BLOCKED);
			plcom_hwiflow(sc);
		}
	}

	PLCOM_UNLOCK(sc);
	splx(s);
	return 1;
}
	
/*
 * (un)block input via hw flowcontrol
 */
void
plcom_hwiflow(struct plcom_softc *sc)
{
	if (sc->sc_mcr_rts == 0)
		return;

	if (ISSET(sc->sc_rx_flags, RX_ANY_BLOCK)) {
		CLR(sc->sc_mcr, sc->sc_mcr_rts);
		CLR(sc->sc_mcr_active, sc->sc_mcr_rts);
	} else {
		SET(sc->sc_mcr, sc->sc_mcr_rts);
		SET(sc->sc_mcr_active, sc->sc_mcr_rts);
	}
	/* XXX device_unit() abuse */
	sc->sc_set_mcr(sc->sc_set_mcr_arg, device_unit(&sc->sc_dev),
	    sc->sc_mcr_active);
}


void
plcomstart(struct tty *tp)
{
	struct plcom_softc *sc = device_lookup(&plcom_cd, PLCOMUNIT(tp->t_dev));
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int s;

	if (PLCOM_ISALIVE(sc) == 0)
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
		PLCOM_LOCK(sc);

		sc->sc_tba = tba;
		sc->sc_tbc = tbc;
	}

	SET(tp->t_state, TS_BUSY);
	sc->sc_tx_busy = 1;

	/* Enable transmit completion interrupts if necessary. */
	if (!ISSET(sc->sc_cr, CR_TIE)) {
		SET(sc->sc_cr, CR_TIE);
		bus_space_write_1(iot, ioh, plcom_cr, sc->sc_cr);
	}

	/* Output the first chunk of the contiguous buffer. */
	{
		int n;

		n = sc->sc_tbc;
		if (n > sc->sc_fifolen)
			n = sc->sc_fifolen;
		bus_space_write_multi_1(iot, ioh, plcom_dr, sc->sc_tba, n);
		sc->sc_tbc -= n;
		sc->sc_tba += n;
	}
	PLCOM_UNLOCK(sc);
out:
	splx(s);
	return;
}

/*
 * Stop output on a line.
 */
void
plcomstop(struct tty *tp, int flag)
{
	struct plcom_softc *sc = device_lookup(&plcom_cd, PLCOMUNIT(tp->t_dev));
	int s;

	s = splserial();
	PLCOM_LOCK(sc);
	if (ISSET(tp->t_state, TS_BUSY)) {
		/* Stop transmitting at the next chunk. */
		sc->sc_tbc = 0;
		sc->sc_heldtbc = 0;
		if (!ISSET(tp->t_state, TS_TTSTOP))
			SET(tp->t_state, TS_FLUSH);
	}
	PLCOM_UNLOCK(sc);	
	splx(s);
}

void
plcomdiag(void *arg)
{
	struct plcom_softc *sc = arg;
	int overflows, floods;
	int s;

	s = splserial();
	PLCOM_LOCK(sc);
	overflows = sc->sc_overflows;
	sc->sc_overflows = 0;
	floods = sc->sc_floods;
	sc->sc_floods = 0;
	sc->sc_errors = 0;
	PLCOM_UNLOCK(sc);
	splx(s);

	log(LOG_WARNING, "%s: %d silo overflow%s, %d ibuf flood%s\n",
	    sc->sc_dev.dv_xname,
	    overflows, overflows == 1 ? "" : "s",
	    floods, floods == 1 ? "" : "s");
}

integrate void
plcom_rxsoft(struct plcom_softc *sc, struct tty *tp)
{
	int (*rint) (int, struct tty *) = tp->t_linesw->l_rint;
	u_char *get, *end;
	u_int cc, scc;
	u_char rsr;
	int code;
	int s;

	end = sc->sc_ebuf;
	get = sc->sc_rbget;
	scc = cc = plcom_rbuf_size - sc->sc_rbavail;

	if (cc == plcom_rbuf_size) {
		sc->sc_floods++;
		if (sc->sc_errors++ == 0)
			callout_reset(&sc->sc_diag_callout, 60 * hz,
			    plcomdiag, sc);
	}

	while (cc) {
		code = get[0];
		rsr = get[1];
		if (ISSET(rsr, RSR_OE | RSR_BE | RSR_FE | RSR_PE)) {
			if (ISSET(rsr, RSR_OE)) {
				sc->sc_overflows++;
				if (sc->sc_errors++ == 0)
					callout_reset(&sc->sc_diag_callout,
					    60 * hz, plcomdiag, sc);
			}
			if (ISSET(rsr, RSR_BE | RSR_FE))
				SET(code, TTY_FE);
			if (ISSET(rsr, RSR_PE))
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
					get -= plcom_rbuf_size << 1;
				cc = 0;
			} else {
				/*
				 * Don't schedule any more receive processing
				 * until the line discipline tells us there's
				 * space available (through plcomhwiflow()).
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
		PLCOM_LOCK(sc);
		
		cc = sc->sc_rbavail += scc - cc;
		/* Buffers should be ok again, release possible block. */
		if (cc >= sc->sc_r_lowat) {
			if (ISSET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
				SET(sc->sc_cr, CR_RIE | CR_RTIE);
				bus_space_write_1(sc->sc_iot, sc->sc_ioh, plcom_cr, sc->sc_cr);
			}
			if (ISSET(sc->sc_rx_flags, RX_IBUF_BLOCKED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_BLOCKED);
				plcom_hwiflow(sc);
			}
		}
		PLCOM_UNLOCK(sc);
		splx(s);
	}
}

integrate void
plcom_txsoft(struct plcom_softc *sc, struct tty *tp)
{

	CLR(tp->t_state, TS_BUSY);
	if (ISSET(tp->t_state, TS_FLUSH))
		CLR(tp->t_state, TS_FLUSH);
	else
		ndflush(&tp->t_outq, (int)(sc->sc_tba - tp->t_outq.c_cf));
	(*tp->t_linesw->l_start)(tp);
}

integrate void
plcom_stsoft(struct plcom_softc *sc, struct tty *tp)
{
	u_char msr, delta;
	int s;

	s = splserial();
	PLCOM_LOCK(sc);
	msr = sc->sc_msr;
	delta = sc->sc_msr_delta;
	sc->sc_msr_delta = 0;
	PLCOM_UNLOCK(sc);	
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

#ifdef PLCOM_DEBUG
	if (plcom_debug)
		plcomstatus(sc, "plcom_stsoft");
#endif
}

void
plcomsoft(void *arg)
{
	struct plcom_softc *sc = arg;
	struct tty *tp;

	if (PLCOM_ISALIVE(sc) == 0)
		return;

	tp = sc->sc_tty;
		
	if (sc->sc_rx_ready) {
		sc->sc_rx_ready = 0;
		plcom_rxsoft(sc, tp);
	}

	if (sc->sc_st_check) {
		sc->sc_st_check = 0;
		plcom_stsoft(sc, tp);
	}

	if (sc->sc_tx_done) {
		sc->sc_tx_done = 0;
		plcom_txsoft(sc, tp);
	}
}

int
plcomintr(void *arg)
{
	struct plcom_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_char *put, *end;
	u_int cc;
	u_char rsr, iir;

	if (PLCOM_ISALIVE(sc) == 0)
		return 0;

	PLCOM_LOCK(sc);
	iir = bus_space_read_1(iot, ioh, plcom_iir);
	if (! ISSET(iir, IIR_IMASK)) {
		PLCOM_UNLOCK(sc);
		return 0;
	}

	end = sc->sc_ebuf;
	put = sc->sc_rbput;
	cc = sc->sc_rbavail;

	do {
		u_char	msr, delta, fr;

		fr = bus_space_read_1(iot, ioh, plcom_fr);

		if (!ISSET(fr, FR_RXFE) &&
		    !ISSET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED)) {
			while (cc > 0) {
				int cn_trapped = 0;
				put[0] = bus_space_read_1(iot, ioh,
				    plcom_dr);
				rsr = bus_space_read_1(iot, ioh, plcom_rsr);
				/* Clear any error status.  */
				if (ISSET(rsr,
				    (RSR_BE | RSR_OE | RSR_PE | RSR_FE)))
					bus_space_write_1(iot, ioh, plcom_ecr,
					    0);
				if (ISSET(rsr, RSR_BE)) {
					cn_trapped = 0;
					cn_check_magic(sc->sc_tty->t_dev,
					    CNC_BREAK, plcom_cnm_state);
					if (cn_trapped)
						continue;
#if defined(KGDB)
					if (ISSET(sc->sc_hwflags,
					    PLCOM_HW_KGDB)) {
						kgdb_connect(1);
						continue;
					}
#endif
				}

				put[1] = rsr;
				cn_trapped = 0;
				cn_check_magic(sc->sc_tty->t_dev,
					       put[0], plcom_cnm_state);
				if (cn_trapped) {
					fr = bus_space_read_1(iot, ioh,
					    plcom_fr);
					if (ISSET(fr, FR_RXFE))
						break;

					continue;
				}
				put += 2;
				if (put >= end)
					put = sc->sc_rbuf;
				cc--;

				fr = bus_space_read_1(iot, ioh, plcom_fr);
				if (ISSET(fr, FR_RXFE))
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
				plcom_hwiflow(sc);
			}

			/*
			 * If we're out of space, disable receive interrupts
			 * until the queue has drained a bit.
			 */
			if (!cc) {
				SET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
				CLR(sc->sc_cr, CR_RIE | CR_RTIE);
				bus_space_write_1(iot, ioh, plcom_cr,
				    sc->sc_cr);
			}
		} else {
			if (ISSET(iir, IIR_RIS)) {
				bus_space_write_1(iot, ioh, plcom_cr, 0);
				delay(10);
				bus_space_write_1(iot, ioh, plcom_cr,
				    sc->sc_cr);
				continue;
			}
		}

		msr = bus_space_read_1(iot, ioh, plcom_fr);
		delta = msr ^ sc->sc_msr;
		sc->sc_msr = msr;
		/* Clear any pending modem status interrupt.  */
		if (iir & IIR_MIS)
			bus_space_write_1(iot, ioh, plcom_icr, 0);
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

		/*
		 * Process normal status changes
		 */
		if (ISSET(delta, sc->sc_msr_mask)) {
			SET(sc->sc_msr_delta, delta);

			/*
			 * Stop output immediately if we lose the output
			 * flow control signal or carrier detect.
			 */
			if (ISSET(~msr, sc->sc_msr_mask)) {
				sc->sc_tbc = 0;
				sc->sc_heldtbc = 0;
#ifdef PLCOM_DEBUG
				if (plcom_debug)
					plcomstatus(sc, "plcomintr  ");
#endif
			}

			sc->sc_st_check = 1;
		}

		/* 
		 * Done handling any receive interrupts. See if data
		 * can be * transmitted as well. Schedule tx done
		 * event if no data left * and tty was marked busy.
		 */
		if (ISSET(iir, IIR_TIS)) {
			/*
			 * If we've delayed a parameter change, do it
			 * now, and restart * output.
			 */
			if (sc->sc_heldchange) {
				plcom_loadchannelregs(sc);
				sc->sc_heldchange = 0;
				sc->sc_tbc = sc->sc_heldtbc;
				sc->sc_heldtbc = 0;
			}

			/* 
			 * Output the next chunk of the contiguous
			 * buffer, if any.
			 */
			if (sc->sc_tbc > 0) {
				int n;

				n = sc->sc_tbc;
				if (n > sc->sc_fifolen)
					n = sc->sc_fifolen;
				bus_space_write_multi_1(iot, ioh, plcom_dr,
				    sc->sc_tba, n);
				sc->sc_tbc -= n;
				sc->sc_tba += n;
			} else {
				/*
				 * Disable transmit plcompletion
				 * interrupts if necessary.
				 */
				if (ISSET(sc->sc_cr, CR_TIE)) {
					CLR(sc->sc_cr, CR_TIE);
					bus_space_write_1(iot, ioh, plcom_cr,
					    sc->sc_cr);
				}
				if (sc->sc_tx_busy) {
					sc->sc_tx_busy = 0;
					sc->sc_tx_done = 1;
				}
			}
		}
	} while (ISSET((iir = bus_space_read_1(iot, ioh, plcom_iir)),
	    IIR_IMASK));

	PLCOM_UNLOCK(sc);

	/* Wake up the poller. */
	softint_schedule(sc->sc_si);

#if NRND > 0 && defined(RND_COM)
	rnd_add_uint32(&sc->rnd_source, iir | rsr);
#endif

	return 1;
}

/*
 * The following functions are polled getc and putc routines, shared
 * by the console and kgdb glue.
 * 
 * The read-ahead code is so that you can detect pending in-band
 * cn_magic in polled mode while doing output rather than having to
 * wait until the kernel decides it needs input.
 */

#define MAX_READAHEAD	20
static int plcom_readahead[MAX_READAHEAD];
static int plcom_readaheadcount = 0;

int
plcom_common_getc(dev_t dev, bus_space_tag_t iot, bus_space_handle_t ioh)
{
	int s = splserial();
	u_char stat, c;

	/* got a character from reading things earlier */
	if (plcom_readaheadcount > 0) {
		int i;

		c = plcom_readahead[0];
		for (i = 1; i < plcom_readaheadcount; i++) {
			plcom_readahead[i-1] = plcom_readahead[i];
		}
		plcom_readaheadcount--;
		splx(s);
		return c;
	}

	/* block until a character becomes available */
	while (ISSET(stat = bus_space_read_1(iot, ioh, plcom_fr), FR_RXFE))
		;

	c = bus_space_read_1(iot, ioh, plcom_dr);
	stat = bus_space_read_1(iot, ioh, plcom_iir);
	{
		int cn_trapped = 0; /* unused */
#ifdef DDB
		extern int db_active;
		if (!db_active)
#endif
			cn_check_magic(dev, c, plcom_cnm_state);
	}
	splx(s);
	return c;
}

void
plcom_common_putc(dev_t dev, bus_space_tag_t iot, bus_space_handle_t ioh,
    int c)
{
	int s = splserial();
	int timo;

	int cin, stat;
	if (plcom_readaheadcount < MAX_READAHEAD 
	     && !ISSET(stat = bus_space_read_1(iot, ioh, plcom_fr), FR_RXFE)) {
		int cn_trapped = 0;
		cin = bus_space_read_1(iot, ioh, plcom_dr);
		stat = bus_space_read_1(iot, ioh, plcom_iir);
		cn_check_magic(dev, cin, plcom_cnm_state);
		plcom_readahead[plcom_readaheadcount++] = cin;
	}

	/* wait for any pending transmission to finish */
	timo = 150000;
	while (!ISSET(bus_space_read_1(iot, ioh, plcom_fr), FR_TXFE) && --timo)
		continue;

	bus_space_write_1(iot, ioh, plcom_dr, c);
	PLCOM_BARRIER(iot, ioh, BR | BW);

	/* wait for this transmission to complete */
	timo = 1500000;
	while (!ISSET(bus_space_read_1(iot, ioh, plcom_fr), FR_TXFE) && --timo)
		continue;

	splx(s);
}

/*
 * Initialize UART for use as console or KGDB line.
 */
int
plcominit(bus_space_tag_t iot, bus_addr_t iobase, int rate, int frequency,
    tcflag_t cflag, bus_space_handle_t *iohp)
{
	bus_space_handle_t ioh;

	if (bus_space_map(iot, iobase, PLCOM_UART_SIZE, 0, &ioh))
		return ENOMEM; /* ??? */

	rate = plcomspeed(rate, frequency);
	bus_space_write_1(iot, ioh, plcom_cr, 0);
	bus_space_write_1(iot, ioh, plcom_dlbl, rate);
	bus_space_write_1(iot, ioh, plcom_dlbh, rate >> 8);
	bus_space_write_1(iot, ioh, plcom_lcr, cflag2lcr(cflag) | LCR_FEN);
	bus_space_write_1(iot, ioh, plcom_cr, CR_UARTEN);

#if 0
	/* Ought to do something like this, but we have no sc to
	   dereference. */
	/* XXX device_unit() abuse */
	sc->sc_set_mcr(sc->sc_set_mcr_arg, device_unit(&sc->sc_dev),
	    MCR_DTR | MCR_RTS);
#endif

	*iohp = ioh;
	return 0;
}

/*
 * Following are all routines needed for PLCOM to act as console
 */
struct consdev plcomcons = {
	NULL, NULL, plcomcngetc, plcomcnputc, plcomcnpollc, NULL,
	NULL, NULL, NODEV, CN_NORMAL
};


int
plcomcnattach(bus_space_tag_t iot, bus_addr_t iobase, int rate, int frequency,
    tcflag_t cflag, int unit)
{
	int res;

	res = plcominit(iot, iobase, rate, frequency, cflag, &plcomconsioh);
	if (res)
		return res;

	cn_tab = &plcomcons;
	cn_init_magic(&plcom_cnm_state);
	cn_set_magic("\047\001"); /* default magic is BREAK */

	plcomconstag = iot;
	plcomconsunit = unit;
	plcomconsrate = rate;
	plcomconscflag = cflag;

	return 0;
}

void
plcomcndetach(void)
{
	bus_space_unmap(plcomconstag, plcomconsioh, PLCOM_UART_SIZE);
	plcomconstag = NULL;

	cn_tab = NULL;
}

int
plcomcngetc(dev_t dev)
{
	return plcom_common_getc(dev, plcomconstag, plcomconsioh);
}

/*
 * Console kernel output character routine.
 */
void
plcomcnputc(dev_t dev, int c)
{
	plcom_common_putc(dev, plcomconstag, plcomconsioh, c);
}

void
plcomcnpollc(dev_t dev, int on)
{

}

#ifdef KGDB
int
plcom_kgdb_attach(bus_space_tag_t iot, bus_addr_t iobase, int rate,
   int frequency, tcflag_t cflag, int unit)
{
	int res;

	if (iot == plcomconstag && iobase == plcomconsunit)
		return EBUSY; /* cannot share with console */

	res = plcominit(iot, iobase, rate, frequency, cflag, &plcom_kgdb_ioh);
	if (res)
		return res;

	kgdb_attach(plcom_kgdb_getc, plcom_kgdb_putc, NULL);
	kgdb_dev = 123; /* unneeded, only to satisfy some tests */

	plcom_kgdb_iot = iot;
	plcom_kgdb_unit = unit;

	return 0;
}

/* ARGSUSED */
int
plcom_kgdb_getc(void *arg)
{
	return plcom_common_getc(NODEV, plcom_kgdb_iot, plcom_kgdb_ioh);
}

/* ARGSUSED */
void
plcom_kgdb_putc(void *arg, int c)
{
	plcom_common_putc(NODEV, plcom_kgdb_iot, plcom_kgdb_ioh, c);
}
#endif /* KGDB */

/* helper function to identify the plcom ports used by
 console or KGDB (and not yet autoconf attached) */
int
plcom_is_console(bus_space_tag_t iot, int unit,
    bus_space_handle_t *ioh)
{
	bus_space_handle_t help;

	if (!plcomconsattached &&
	    iot == plcomconstag && unit == plcomconsunit)
		help = plcomconsioh;
#ifdef KGDB
	else if (!plcom_kgdb_attached &&
	    iot == plcom_kgdb_iot && unit == plcom_kgdb_unit)
		help = plcom_kgdb_ioh;
#endif
	else
		return 0;

	if (ioh)
		*ioh = help;
	return 1;
}

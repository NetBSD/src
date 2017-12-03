/*	$NetBSD: plcom.c,v 1.42.2.4 2017/12/03 11:36:04 jdolecek Exp $	*/

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
 * Copyright (c) 1998, 1999, 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and Nick Hudson.
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
 * COM driver for the Prime Cell PL010 and PL011 UARTs. Both are is similar to
 * the 16C550, but have a completely different programmer's model.
 * Derived from the NS16550AF com driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: plcom.c,v 1.42.2.4 2017/12/03 11:36:04 jdolecek Exp $");

#include "opt_plcom.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"

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
#ifdef RND_COM
#include <sys/rndsource.h>
#endif

#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>

#include <dev/cons.h>

static void plcom_enable_debugport (struct plcom_softc *);

void	plcom_config	(struct plcom_softc *);
void	plcom_shutdown	(struct plcom_softc *);
int	pl010comspeed	(long, long);
int	pl011comspeed	(long, long);
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

int	plcom_common_getc (dev_t, struct plcom_instance *);
void	plcom_common_putc (dev_t, struct plcom_instance *, int);

int	plcominit	(struct plcom_instance *, int, int, tcflag_t);

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

bool	plcom_intstatus(struct plcom_instance *, u_int *);

extern struct cfdriver plcom_cd;

const struct cdevsw plcom_cdevsw = {
	.d_open = plcomopen,
	.d_close = plcomclose,
	.d_read = plcomread,
	.d_write = plcomwrite,
	.d_ioctl = plcomioctl,
	.d_stop = plcomstop,
	.d_tty = plcomtty,
	.d_poll = plcompoll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
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
static struct plcom_instance plcomcons_info;

static int plcomconsattached;
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

static struct plcom_instance plcomkgdb_info;
static int plcom_kgdb_attached;

int	plcom_kgdb_getc (void *);
void	plcom_kgdb_putc (void *, int);
#endif /* KGDB */

#define	PLCOMDIALOUT_MASK	TTDIALOUT_MASK

#define	PLCOMUNIT(x)	TTUNIT(x)
#define	PLCOMDIALOUT(x)	TTDIALOUT(x)

#define	PLCOM_ISALIVE(sc)	((sc)->enabled != 0 && \
				 device_is_active((sc)->sc_dev))

#define	BR	BUS_SPACE_BARRIER_READ
#define	BW	BUS_SPACE_BARRIER_WRITE
#define PLCOM_BARRIER(pi, f)	\
    bus_space_barrier((pi)->pi_iot, (pi)->pi_ioh, 0, (pi)->pi_size, (f))

static uint8_t
pread1(struct plcom_instance *pi, bus_size_t reg)
{
	if (!ISSET(pi->pi_flags, PLC_FLAG_32BIT_ACCESS))
		return bus_space_read_1(pi->pi_iot, pi->pi_ioh, reg);

	return bus_space_read_4(pi->pi_iot, pi->pi_ioh, reg & -4) >>
	    (8 * (reg & 3));
}
int nhcr;
static void
pwrite1(struct plcom_instance *pi, bus_size_t o, uint8_t val)
{
	if (!ISSET(pi->pi_flags, PLC_FLAG_32BIT_ACCESS)) {
		bus_space_write_1(pi->pi_iot, pi->pi_ioh, o, val);
	} else {
		const size_t shift = 8 * (o & 3);
		o &= -4;
		uint32_t tmp = bus_space_read_4(pi->pi_iot, pi->pi_ioh, o);
		tmp = (val << shift) | (tmp & ~(0xff << shift));
		bus_space_write_4(pi->pi_iot, pi->pi_ioh, o, tmp);
	}
}

static void
pwritem1(struct plcom_instance *pi, bus_size_t o, const uint8_t *datap,
    bus_size_t count)
{
	if (!ISSET(pi->pi_flags, PLC_FLAG_32BIT_ACCESS)) {
		bus_space_write_multi_1(pi->pi_iot, pi->pi_ioh, o, datap, count);
	} else {
		KASSERT((o & 3) == 0);
		while (count--) {
			bus_space_write_4(pi->pi_iot, pi->pi_ioh, o, *datap++);
		};
	}
}

#define	PREAD1(pi, reg)		pread1(pi, reg)
#define	PREAD4(pi, reg)		\
	(bus_space_read_4((pi)->pi_iot, (pi)->pi_ioh, (reg)))

#define	PWRITE1(pi, reg, val)	pwrite1(pi, reg, val)
#define	PWRITEM1(pi, reg, d, c)	pwritem1(pi, reg, d, c)
#define	PWRITE4(pi, reg, val)	\
	(bus_space_write_4((pi)->pi_iot, (pi)->pi_ioh, (reg), (val)))

int
pl010comspeed(long speed, long frequency)
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

int
pl011comspeed(long speed, long frequency)
{
	int denom = 16 * speed;
	int div = frequency / denom;
	int rem = frequency % denom;

	int ibrd = div << 6;
	int fbrd = (((8 * rem) / speed) + 1) / 2;

	/* Tolerance? */
	return ibrd | fbrd;
}

#ifdef PLCOM_DEBUG
int	plcom_debug = 0;

void plcomstatus (struct plcom_softc *, const char *);
void
plcomstatus(struct plcom_softc *sc, const char *str)
{
	struct tty *tp = sc->sc_tty;

	printf("%s: %s %sclocal  %sdcd %sts_carr_on %sdtr %stx_stopped\n",
	    device_xname(sc->sc_dev), str,
	    ISSET(tp->t_cflag, CLOCAL) ? "+" : "-",
	    ISSET(sc->sc_msr, PL01X_MSR_DCD) ? "+" : "-",
	    ISSET(tp->t_state, TS_CARR_ON) ? "+" : "-",
	    ISSET(sc->sc_mcr, PL01X_MCR_DTR) ? "+" : "-",
	    sc->sc_tx_stopped ? "+" : "-");

	printf("%s: %s %scrtscts %scts %sts_ttstop  %srts %xrx_flags\n",
	    device_xname(sc->sc_dev), str,
	    ISSET(tp->t_cflag, CRTSCTS) ? "+" : "-",
	    ISSET(sc->sc_msr, PL01X_MSR_CTS) ? "+" : "-",
	    ISSET(tp->t_state, TS_TTSTOP) ? "+" : "-",
	    ISSET(sc->sc_mcr, PL01X_MCR_RTS) ? "+" : "-",
	    sc->sc_rx_flags);
}
#endif

#if 0
int
plcomprobe1(bus_space_tag_t iot, bus_space_handle_t ioh)
{
	int data;

	/* Disable the UART.  */
	bus_space_write_1(iot, ioh, plcom_cr, 0);
	/* Make sure the FIFO is off.  */
	bus_space_write_1(iot, ioh, plcom_lcr, PL01X_LCR_8BITS);
	/* Disable interrupts.  */
	bus_space_write_1(iot, ioh, plcom_iir, 0);

	/* Make sure we swallow anything in the receiving register.  */
	data = bus_space_read_1(iot, ioh, plcom_dr);

	if (bus_space_read_1(iot, ioh, plcom_lcr) != PL01X_LCR_8BITS)
		return 0;

	data = bus_space_read_1(iot, ioh, plcom_fr) & (PL01X_FR_RXFF | PL01X_FR_RXFE);

	if (data != PL01X_FR_RXFE)
		return 0;

	return 1;
}
#endif

/*
 * No locking in this routine; it is only called during attach,
 * or with the port already locked.
 */
static void
plcom_enable_debugport(struct plcom_softc *sc)
{
	struct plcom_instance *pi = &sc->sc_pi;

	sc->sc_cr = PL01X_CR_UARTEN;
	SET(sc->sc_mcr, PL01X_MCR_DTR | PL01X_MCR_RTS);

	/* Turn on line break interrupt, set carrier. */
	switch (pi->pi_type) {
	case PLCOM_TYPE_PL010:
		SET(sc->sc_cr, PL010_CR_RIE | PL010_CR_RTIE);
		PWRITE1(pi, PL010COM_CR, sc->sc_cr);
		if (sc->sc_set_mcr) {
			/* XXX device_unit() abuse */
			sc->sc_set_mcr(sc->sc_set_mcr_arg,
			    device_unit(sc->sc_dev), sc->sc_mcr);
		}
		break;
	case PLCOM_TYPE_PL011:
		sc->sc_imsc = PL011_INT_RX | PL011_INT_RT;
		SET(sc->sc_cr, PL011_CR_RXE | PL011_CR_TXE);
		SET(sc->sc_cr, PL011_MCR(sc->sc_mcr));
		PWRITE4(pi, PL011COM_CR, sc->sc_cr);
		PWRITE4(pi, PL011COM_IMSC, sc->sc_imsc);
		break;
	}

}

void
plcom_attach_subr(struct plcom_softc *sc)
{
	struct plcom_instance *pi = &sc->sc_pi;
	struct tty *tp;

	aprint_naive("\n");

	callout_init(&sc->sc_diag_callout, 0);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_HIGH);

	switch (pi->pi_type) {
	case PLCOM_TYPE_PL010:
	case PLCOM_TYPE_PL011:
		break;
	default:
		aprint_error_dev(sc->sc_dev,
		    "Unknown plcom type: %d\n", pi->pi_type);
		return;
	}

	/* Disable interrupts before configuring the device. */
	sc->sc_cr = 0;
	sc->sc_imsc = 0;

	if (bus_space_is_equal(pi->pi_iot, plcomcons_info.pi_iot) &&
	    pi->pi_iobase == plcomcons_info.pi_iobase) {
		plcomconsattached = 1;

		/* Make sure the console is always "hardwired". */
		delay(1000);			/* wait for output to finish */
		SET(sc->sc_hwflags, PLCOM_HW_CONSOLE);
		SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);
		/*
		 * Must re-enable the console immediately, or we will
		 * hang when trying to print.
		 */
		sc->sc_cr = PL01X_CR_UARTEN;
		if (pi->pi_type == PLCOM_TYPE_PL011)
			SET(sc->sc_cr, PL011_CR_RXE | PL011_CR_TXE);
	}

	switch (pi->pi_type) {
	case PLCOM_TYPE_PL010:
		PWRITE1(pi, PL010COM_CR, sc->sc_cr);
		break;

	case PLCOM_TYPE_PL011:
		PWRITE4(pi, PL011COM_CR, sc->sc_cr);
		PWRITE4(pi, PL011COM_IMSC, sc->sc_imsc);
		break;
	}

	if (sc->sc_fifolen == 0) {
		switch (pi->pi_type) {
		case PLCOM_TYPE_PL010:
			/*
			 * The PL010 has a 16-byte fifo, but the tx interrupt
			 * triggers when there is space for 8 more bytes.
			*/
			sc->sc_fifolen = 8;
			break;
		case PLCOM_TYPE_PL011:
			/* Some revisions have a 32 byte TX FIFO */
			sc->sc_fifolen = 16;
			break;
		}
	}
	aprint_normal("\n");

	if (ISSET(sc->sc_hwflags, PLCOM_HW_TXFIFO_DISABLE)) {
		sc->sc_fifolen = 1;
		aprint_normal_dev(sc->sc_dev, "txfifo disabled\n");
	}

	if (sc->sc_fifolen > 1)
		SET(sc->sc_hwflags, PLCOM_HW_FIFO);

	tp = tty_alloc();
	tp->t_oproc = plcomstart;
	tp->t_param = plcomparam;
	tp->t_hwiflow = plcomhwiflow;

	sc->sc_tty = tp;
	sc->sc_rbuf = malloc(plcom_rbuf_size << 1, M_DEVBUF, M_NOWAIT);
	sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
	sc->sc_rbavail = plcom_rbuf_size;
	if (sc->sc_rbuf == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "unable to allocate ring buffer\n");
		return;
	}
	sc->sc_ebuf = sc->sc_rbuf + (plcom_rbuf_size << 1);

	tty_attach(tp);

	if (ISSET(sc->sc_hwflags, PLCOM_HW_CONSOLE)) {
		int maj;

		/* locate the major number */
		maj = cdevsw_lookup_major(&plcom_cdevsw);

		tp->t_dev = cn_tab->cn_dev = makedev(maj, device_unit(sc->sc_dev));

		aprint_normal_dev(sc->sc_dev, "console\n");
	}

#ifdef KGDB
	/*
	 * Allow kgdb to "take over" this port.  If this is
	 * the kgdb device, it has exclusive use.
	 */
	if (bus_space_is_equal(pi->pi_iot, plcomkgdb_info.pi_iot) &&
	    pi->pi_iobase == plcomkgdb_info.pi_iobase) {
		if (!ISSET(sc->sc_hwflags, PLCOM_HW_CONSOLE)) {
			plcom_kgdb_attached = 1;

			SET(sc->sc_hwflags, PLCOM_HW_KGDB);
		}
		aprint_normal_dev(sc->sc_dev, "kgdb\n");
	}
#endif

	sc->sc_si = softint_establish(SOFTINT_SERIAL, plcomsoft, sc);

#ifdef RND_COM
	rnd_attach_source(&sc->rnd_source, device_xname(sc->sc_dev),
	    RND_TYPE_TTY, RND_FLAG_DEFAULT);
#endif

	/*
	 * if there are no enable/disable functions, assume the device
	 * is always enabled
	 */
	if (!sc->enable)
		sc->enabled = 1;

	plcom_config(sc);

	SET(sc->sc_hwflags, PLCOM_HW_DEV_OK);
}

void
plcom_config(struct plcom_softc *sc)
{
	struct plcom_instance *pi = &sc->sc_pi;

	/* Disable interrupts before configuring the device. */
	sc->sc_cr = 0;
	sc->sc_imsc = 0;
	switch (pi->pi_type) {
	case PLCOM_TYPE_PL010:
		PWRITE1(pi, PL010COM_CR, sc->sc_cr);
		break;

	case PLCOM_TYPE_PL011:
		PWRITE4(pi, PL011COM_CR, sc->sc_cr);
		PWRITE4(pi, PL011COM_IMSC, sc->sc_imsc);
		break;
	}

	if (ISSET(sc->sc_hwflags, PLCOM_HW_CONSOLE|PLCOM_HW_KGDB))
		plcom_enable_debugport(sc);
}

int
plcom_detach(device_t self, int flags)
{
	struct plcom_softc *sc = device_private(self);
	int maj, mn;

	if (sc->sc_hwflags & (PLCOM_HW_CONSOLE|PLCOM_HW_KGDB))
		return EBUSY;

	if (sc->disable != NULL && sc->enabled != 0) {
		(*sc->disable)(sc);
		sc->enabled = 0;
	}

	/* locate the major number */
	maj = cdevsw_lookup_major(&plcom_cdevsw);

	/* Nuke the vnodes for any open instances. */
	mn = device_unit(self);
	vdevgone(maj, mn, mn, VCHR);

	mn |= PLCOMDIALOUT_MASK;
	vdevgone(maj, mn, mn, VCHR);

	if (sc->sc_rbuf == NULL) {
		/*
		 * Ring buffer allocation failed in the plcom_attach_subr,
		 * only the tty is allocated, and nothing else.
		 */
		tty_free(sc->sc_tty);
		return 0;
	}

	/* Free the receive buffer. */
	free(sc->sc_rbuf, M_DEVBUF);

	/* Detach and free the tty. */
	tty_detach(sc->sc_tty);
	tty_free(sc->sc_tty);

	/* Unhook the soft interrupt handler. */
	softint_disestablish(sc->sc_si);

#ifdef RND_COM
	/* Unhook the entropy source. */
	rnd_detach_source(&sc->rnd_source);
#endif
	callout_destroy(&sc->sc_diag_callout);

	/* Destroy the lock. */
	mutex_destroy(&sc->sc_lock);

	return 0;
}

int
plcom_activate(device_t self, enum devact act)
{
	struct plcom_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->enabled = 0;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

void
plcom_shutdown(struct plcom_softc *sc)
{
	struct plcom_instance *pi = &sc->sc_pi;
	struct tty *tp = sc->sc_tty;
	mutex_spin_enter(&sc->sc_lock);

	/* If we were asserting flow control, then deassert it. */
	SET(sc->sc_rx_flags, RX_IBUF_BLOCKED);
	plcom_hwiflow(sc);

	/* Clear any break condition set with TIOCSBRK. */
	plcom_break(sc, 0);

	/* Turn off PPS capture on last close. */
	mutex_spin_enter(&timecounter_lock);
	sc->sc_ppsmask = 0;
	sc->ppsparam.mode = 0;
	mutex_spin_exit(&timecounter_lock);

	/*
	 * Hang up if necessary.  Wait a bit, so the other side has time to
	 * notice even if we immediately open the port again.
	 * Avoid tsleeping above splhigh().
	 */
	if (ISSET(tp->t_cflag, HUPCL)) {
		plcom_modem(sc, 0);
		mutex_spin_exit(&sc->sc_lock);
		/* XXX will only timeout */
		(void) kpause(ttclos, false, hz, NULL);
		mutex_spin_enter(&sc->sc_lock);
	}

	sc->sc_cr = 0;
	sc->sc_imsc = 0;
	/* Turn off interrupts. */
	if (ISSET(sc->sc_hwflags, PLCOM_HW_CONSOLE)) {
		/* interrupt on break */

		sc->sc_cr = PL01X_CR_UARTEN;
		sc->sc_imsc = 0;
		switch (pi->pi_type) {
		case PLCOM_TYPE_PL010:
			SET(sc->sc_cr, PL010_CR_RIE | PL010_CR_RTIE);
			break;
		case PLCOM_TYPE_PL011:
			SET(sc->sc_cr, PL011_CR_RXE);
			SET(sc->sc_imsc, PL011_INT_RT | PL011_INT_RX);
			break;
		}
	}
	switch (pi->pi_type) {
	case PLCOM_TYPE_PL010:
		SET(sc->sc_cr, PL010_CR_RIE | PL010_CR_RTIE);
		PWRITE1(pi, PL010COM_CR, sc->sc_cr);
		break;
	case PLCOM_TYPE_PL011:
		SET(sc->sc_cr, PL011_CR_RXE | PL011_CR_TXE);
		SET(sc->sc_imsc, PL011_INT_RT | PL011_INT_RX);
		PWRITE4(pi, PL011COM_CR, sc->sc_cr);
		PWRITE4(pi, PL011COM_IMSC, sc->sc_imsc);
		break;
	}

	mutex_spin_exit(&sc->sc_lock);
	if (sc->disable) {
#ifdef DIAGNOSTIC
		if (!sc->enabled)
			panic("plcom_shutdown: not enabled?");
#endif
		(*sc->disable)(sc);
		sc->enabled = 0;
	}
}

int
plcomopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct plcom_softc *sc;
	struct plcom_instance *pi;
	struct tty *tp;
	int s;
	int error;

	sc = device_lookup_private(&plcom_cd, PLCOMUNIT(dev));
	if (sc == NULL || !ISSET(sc->sc_hwflags, PLCOM_HW_DEV_OK) ||
		sc->sc_rbuf == NULL)
		return ENXIO;

	if (!device_is_active(sc->sc_dev))
		return ENXIO;

	pi = &sc->sc_pi;

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

		if (sc->enable) {
			if ((*sc->enable)(sc)) {
				splx(s);
				aprint_error_dev(sc->sc_dev,
				    "device enable failed\n");
				return EIO;
			}
			mutex_spin_enter(&sc->sc_lock);
			sc->enabled = 1;
			plcom_config(sc);
		} else {
			mutex_spin_enter(&sc->sc_lock);
		}

		/* Turn on interrupts. */
		/* IER_ERXRDY | IER_ERLS | IER_EMSC;  */
		/* Fetch the current modem control status, needed later. */
		sc->sc_cr = PL01X_CR_UARTEN;
		switch (pi->pi_type) {
		case PLCOM_TYPE_PL010:
			SET(sc->sc_cr,
			    PL010_CR_RIE | PL010_CR_RTIE | PL010_CR_MSIE);
			PWRITE1(pi, PL010COM_CR, sc->sc_cr);
			sc->sc_msr = PREAD1(pi, PL01XCOM_FR);
			break;
		case PLCOM_TYPE_PL011:
			SET(sc->sc_cr, PL011_CR_RXE | PL011_CR_TXE);
			SET(sc->sc_imsc, PL011_INT_RT | PL011_INT_RX |
			    PL011_INT_MSMASK);
			PWRITE4(pi, PL011COM_IMSC, sc->sc_imsc);
			sc->sc_msr = PREAD4(pi, PL01XCOM_FR);
			break;
		}

		/* Clear PPS capture state on first open. */

		mutex_spin_enter(&timecounter_lock);
		sc->sc_ppsmask = 0;
		sc->ppsparam.mode = 0;
		mutex_spin_exit(&timecounter_lock);

		mutex_spin_exit(&sc->sc_lock);

		/*
		 * Initialize the termios status to the defaults.  Add in the
		 * sticky bits from TIOCSFLAGS.
		 */
		if (ISSET(sc->sc_hwflags, PLCOM_HW_CONSOLE)) {
			t.c_ospeed = plcomconsrate;
			t.c_cflag = plcomconscflag;
		} else {
			t.c_ospeed = TTYDEF_SPEED;
			t.c_cflag = TTYDEF_CFLAG;
		}
		t.c_ispeed = t.c_ospeed;

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

		mutex_spin_enter(&sc->sc_lock);

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

		mutex_spin_exit(&sc->sc_lock);
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
	struct plcom_softc *sc =
		device_lookup_private(&plcom_cd, PLCOMUNIT(dev));
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
	struct plcom_softc *sc =
		device_lookup_private(&plcom_cd, PLCOMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (PLCOM_ISALIVE(sc) == 0)
		return EIO;

	return (*tp->t_linesw->l_read)(tp, uio, flag);
}

int
plcomwrite(dev_t dev, struct uio *uio, int flag)
{
	struct plcom_softc *sc =
		device_lookup_private(&plcom_cd, PLCOMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (PLCOM_ISALIVE(sc) == 0)
		return EIO;

	return (*tp->t_linesw->l_write)(tp, uio, flag);
}

int
plcompoll(dev_t dev, int events, struct lwp *l)
{
	struct plcom_softc *sc =
		device_lookup_private(&plcom_cd, PLCOMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (PLCOM_ISALIVE(sc) == 0)
		return EIO;

	return (*tp->t_linesw->l_poll)(tp, events, l);
}

struct tty *
plcomtty(dev_t dev)
{
	struct plcom_softc *sc =
		device_lookup_private(&plcom_cd, PLCOMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	return tp;
}

int
plcomioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct plcom_softc *sc =
		device_lookup_private(&plcom_cd, PLCOMUNIT(dev));
	struct tty *tp;
	int error;

	if (sc == NULL)
		return ENXIO;
	if (PLCOM_ISALIVE(sc) == 0)
		return EIO;

	tp = sc->sc_tty;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	error = ttioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	error = 0;
	switch (cmd) {
	case TIOCSFLAGS:
		error = kauth_authorize_device_tty(l->l_cred,
		    KAUTH_DEVICE_TTY_PRIVSET, tp);
		break;
	default:
		/* nothing */
		break;
	}
	if (error) {
		return error;
	}

	mutex_spin_enter(&sc->sc_lock);
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
		mutex_spin_enter(&timecounter_lock);
		*pp = sc->ppsparam;
		mutex_spin_exit(&timecounter_lock);
		break;
	}

	case PPS_IOC_SETPARAMS: {
	  	pps_params_t *pp;
		int mode;
		pp = (pps_params_t *)data;
		mutex_spin_enter(&timecounter_lock);
		if (pp->mode & ~ppscap) {
			error = EINVAL;
			mutex_spin_exit(&timecounter_lock);
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
			sc->sc_ppsmask = PL01X_MSR_DCD;
			sc->sc_ppsassert = PL01X_MSR_DCD;
			sc->sc_ppsclear = -1;
			break;

		case PPS_CAPTURECLEAR:
			sc->sc_ppsmask = PL01X_MSR_DCD;
			sc->sc_ppsassert = -1;
			sc->sc_ppsclear = 0;
			break;

		case PPS_CAPTUREBOTH:
			sc->sc_ppsmask = PL01X_MSR_DCD;
			sc->sc_ppsassert = PL01X_MSR_DCD;
			sc->sc_ppsclear = 0;
			break;

		default:
			error = EINVAL;
			break;
		}
		mutex_spin_exit(&timecounter_lock);
		break;
	}

	case PPS_IOC_GETCAP:
		*(int*)data = ppscap;
		break;

	case PPS_IOC_FETCH: {
		pps_info_t *pi;
		pi = (pps_info_t *)data;
		mutex_spin_enter(&timecounter_lock);
		*pi = sc->ppsinfo;
		mutex_spin_exit(&timecounter_lock);
		break;
	}

	case TIOCDCDTIMESTAMP:	/* XXX old, overloaded  API used by xntpd v3 */
		/*
		 * Some GPS clocks models use the falling rather than
		 * rising edge as the on-the-second signal.
		 * The old API has no way to specify PPS polarity.
		 */
		mutex_spin_enter(&timecounter_lock);
		sc->sc_ppsmask = PL01X_MSR_DCD;
#ifndef PPS_TRAILING_EDGE
		sc->sc_ppsassert = PL01X_MSR_DCD;
		sc->sc_ppsclear = -1;
		TIMESPEC_TO_TIMEVAL((struct timeval *)data,
		    &sc->ppsinfo.assert_timestamp);
#else
		sc->sc_ppsassert = -1
		sc->sc_ppsclear = 0;
		TIMESPEC_TO_TIMEVAL((struct timeval *)data,
		    &sc->ppsinfo.clear_timestamp);
#endif
		mutex_spin_exit(&timecounter_lock);
		break;

	default:
		error = EPASSTHROUGH;
		break;
	}

	mutex_spin_exit(&sc->sc_lock);

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
		SET(sc->sc_lcr, PL01X_LCR_BRK);
	else
		CLR(sc->sc_lcr, PL01X_LCR_BRK);

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
		SET(plcombits, PL01X_MCR_DTR);
	if (ISSET(ttybits, TIOCM_RTS))
		SET(plcombits, PL01X_MCR_RTS);

	switch (how) {
	case TIOCMBIC:
		CLR(sc->sc_mcr, plcombits);
		break;

	case TIOCMBIS:
		SET(sc->sc_mcr, plcombits);
		break;

	case TIOCMSET:
		CLR(sc->sc_mcr, PL01X_MCR_DTR | PL01X_MCR_RTS);
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
	if (ISSET(plcombits, PL01X_MCR_DTR))
		SET(ttybits, TIOCM_DTR);
	if (ISSET(plcombits, PL01X_MCR_RTS))
		SET(ttybits, TIOCM_RTS);

	plcombits = sc->sc_msr;
	if (ISSET(plcombits, PL01X_MSR_DCD))
		SET(ttybits, TIOCM_CD);
	if (ISSET(plcombits, PL01X_MSR_CTS))
		SET(ttybits, TIOCM_CTS);
	if (ISSET(plcombits, PL01X_MSR_DSR))
		SET(ttybits, TIOCM_DSR);
	if (ISSET(plcombits, PL011_MSR_RI))
		SET(ttybits, TIOCM_RI);

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
		SET(lcr, PL01X_LCR_5BITS);
		break;
	case CS6:
		SET(lcr, PL01X_LCR_6BITS);
		break;
	case CS7:
		SET(lcr, PL01X_LCR_7BITS);
		break;
	case CS8:
		SET(lcr, PL01X_LCR_8BITS);
		break;
	}
	if (ISSET(cflag, PARENB)) {
		SET(lcr, PL01X_LCR_PEN);
		if (!ISSET(cflag, PARODD))
			SET(lcr, PL01X_LCR_EPS);
	}
	if (ISSET(cflag, CSTOPB))
		SET(lcr, PL01X_LCR_STP2);

	return lcr;
}

int
plcomparam(struct tty *tp, struct termios *t)
{
	struct plcom_softc *sc =
		device_lookup_private(&plcom_cd, PLCOMUNIT(tp->t_dev));
	struct plcom_instance *pi = &sc->sc_pi;
	int ospeed = -1;
	u_char lcr;

	if (PLCOM_ISALIVE(sc) == 0)
		return EIO;

	switch (pi->pi_type) {
	case PLCOM_TYPE_PL010:
		ospeed = pl010comspeed(t->c_ospeed, sc->sc_frequency);
		break;
	case PLCOM_TYPE_PL011:
		ospeed = pl011comspeed(t->c_ospeed, sc->sc_frequency);
		break;
	}

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

	lcr = ISSET(sc->sc_lcr, PL01X_LCR_BRK) | cflag2lcr(t->c_cflag);

	mutex_spin_enter(&sc->sc_lock);

	sc->sc_lcr = lcr;

	/*
	 * PL010 has a fixed-length FIFO trigger point.
	 */
	if (ISSET(sc->sc_hwflags, PLCOM_HW_FIFO))
		sc->sc_fifo = 1;
	else
		sc->sc_fifo = 0;

	if (sc->sc_fifo)
		SET(sc->sc_lcr, PL01X_LCR_FEN);

	/*
	 * If we're not in a mode that assumes a connection is present, then
	 * ignore carrier changes.
	 */
	if (ISSET(t->c_cflag, CLOCAL | MDMBUF))
		sc->sc_msr_dcd = 0;
	else
		sc->sc_msr_dcd = PL01X_MSR_DCD;
	/*
	 * Set the flow control pins depending on the current flow control
	 * mode.
	 */
	if (ISSET(t->c_cflag, CRTSCTS)) {
		sc->sc_mcr_dtr = PL01X_MCR_DTR;
		sc->sc_mcr_rts = PL01X_MCR_RTS;
		sc->sc_msr_cts = PL01X_MSR_CTS;
	} else if (ISSET(t->c_cflag, MDMBUF)) {
		/*
		 * For DTR/DCD flow control, make sure we don't toggle DTR for
		 * carrier detection.
		 */
		sc->sc_mcr_dtr = 0;
		sc->sc_mcr_rts = PL01X_MCR_DTR;
		sc->sc_msr_cts = PL01X_MSR_DCD;
	} else {
		/*
		 * If no flow control, then always set RTS.  This will make
		 * the other side happy if it mistakenly thinks we're doing
		 * RTS/CTS flow control.
		 */
		sc->sc_mcr_dtr = PL01X_MCR_DTR | PL01X_MCR_RTS;
		sc->sc_mcr_rts = 0;
		sc->sc_msr_cts = 0;
		if (ISSET(sc->sc_mcr, PL01X_MCR_DTR))
			SET(sc->sc_mcr, PL01X_MCR_RTS);
		else
			CLR(sc->sc_mcr, PL01X_MCR_RTS);
	}
	sc->sc_msr_mask = sc->sc_msr_cts | sc->sc_msr_dcd;

#if 0
	if (ospeed == 0)
		CLR(sc->sc_mcr, sc->sc_mcr_dtr);
	else
		SET(sc->sc_mcr, sc->sc_mcr_dtr);
#endif

	switch (pi->pi_type) {
	case PLCOM_TYPE_PL010:
		sc->sc_ratel = ospeed & 0xff;
		sc->sc_rateh = (ospeed >> 8) & 0xff;
		break;
	case PLCOM_TYPE_PL011:
		sc->sc_ratel = ospeed & ((1 << 6) - 1);
		sc->sc_rateh = ospeed >> 6;
		break;
	}

	/* And copy to tty. */
	tp->t_ispeed = t->c_ospeed;
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

	mutex_spin_exit(&sc->sc_lock);

	/*
	 * Update the tty layer's idea of the carrier bit, in case we changed
	 * CLOCAL or MDMBUF.  We don't hang up here; we only do that by
	 * explicit request.
	 */
	(void) (*tp->t_linesw->l_modem)(tp, ISSET(sc->sc_msr, PL01X_MSR_DCD));

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
	struct plcom_instance *pi = &sc->sc_pi;
#ifdef DIAGNOSTIC
	int reg;
#endif
	int timo;

#ifdef DIAGNOSTIC
	reg = 0xffff;
#endif
	timo = 50000;
	/* flush any pending I/O */
	while (! ISSET(PREAD1(pi, PL01XCOM_FR), PL01X_FR_RXFE)
	    && --timo)
#ifdef DIAGNOSTIC
		reg =
#else
		    (void)
#endif
		    PREAD1(pi, PL01XCOM_DR);
#ifdef DIAGNOSTIC
	if (!timo)
		aprint_error_dev(sc->sc_dev, ": %s timeout %02x\n", __func__,
		    reg);
#endif
}

void
plcom_loadchannelregs(struct plcom_softc *sc)
{
	struct plcom_instance *pi = &sc->sc_pi;

	/* XXXXX necessary? */
	plcom_iflush(sc);

	switch (pi->pi_type) {
	case PLCOM_TYPE_PL010:
		PWRITE1(pi, PL010COM_CR, 0);
		PWRITE1(pi, PL010COM_DLBL, sc->sc_ratel);
		PWRITE1(pi, PL010COM_DLBH, sc->sc_rateh);
		PWRITE1(pi, PL010COM_LCR, sc->sc_lcr);

		/* XXX device_unit() abuse */
		if (sc->sc_set_mcr)
			sc->sc_set_mcr(sc->sc_set_mcr_arg,
			    device_unit(sc->sc_dev),
			    sc->sc_mcr_active = sc->sc_mcr);

		PWRITE1(pi, PL010COM_CR, sc->sc_cr);
		break;

	case PLCOM_TYPE_PL011:
		PWRITE4(pi, PL011COM_CR, 0);
		PWRITE1(pi, PL011COM_FBRD, sc->sc_ratel);
		PWRITE4(pi, PL011COM_IBRD, sc->sc_rateh);
		PWRITE1(pi, PL011COM_LCRH, sc->sc_lcr);
		sc->sc_mcr_active = sc->sc_mcr;
		CLR(sc->sc_cr, PL011_MCR(PL01X_MCR_RTS | PL01X_MCR_DTR));
		SET(sc->sc_cr, PL011_MCR(sc->sc_mcr_active));
		PWRITE4(pi, PL011COM_CR, sc->sc_cr);
		break;
	}
}

int
plcomhwiflow(struct tty *tp, int block)
{
	struct plcom_softc *sc =
		device_lookup_private(&plcom_cd, PLCOMUNIT(tp->t_dev));

	if (PLCOM_ISALIVE(sc) == 0)
		return 0;

	if (sc->sc_mcr_rts == 0)
		return 0;

	mutex_spin_enter(&sc->sc_lock);

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

	mutex_spin_exit(&sc->sc_lock);
	return 1;
}

/*
 * (un)block input via hw flowcontrol
 */
void
plcom_hwiflow(struct plcom_softc *sc)
{
	struct plcom_instance *pi = &sc->sc_pi;

	if (sc->sc_mcr_rts == 0)
		return;

	if (ISSET(sc->sc_rx_flags, RX_ANY_BLOCK)) {
		CLR(sc->sc_mcr, sc->sc_mcr_rts);
		CLR(sc->sc_mcr_active, sc->sc_mcr_rts);
	} else {
		SET(sc->sc_mcr, sc->sc_mcr_rts);
		SET(sc->sc_mcr_active, sc->sc_mcr_rts);
	}
	switch (pi->pi_type) {
	case PLCOM_TYPE_PL010:
		if (sc->sc_set_mcr)
			/* XXX device_unit() abuse */
			sc->sc_set_mcr(sc->sc_set_mcr_arg,
			     device_unit(sc->sc_dev), sc->sc_mcr_active);
		break;
	case PLCOM_TYPE_PL011:
		CLR(sc->sc_cr, PL011_MCR(PL01X_MCR_RTS | PL01X_MCR_DTR));
		SET(sc->sc_cr, PL011_MCR(sc->sc_mcr_active));
		PWRITE4(pi, PL011COM_CR, sc->sc_cr);
		break;
	}
}


void
plcomstart(struct tty *tp)
{
	struct plcom_softc *sc =
		device_lookup_private(&plcom_cd, PLCOMUNIT(tp->t_dev));
	struct plcom_instance *pi = &sc->sc_pi;
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

		mutex_spin_enter(&sc->sc_lock);

		sc->sc_tba = tba;
		sc->sc_tbc = tbc;
	}

	SET(tp->t_state, TS_BUSY);
	sc->sc_tx_busy = 1;

	/* Enable transmit completion interrupts if necessary. */
	switch (pi->pi_type) {
	case PLCOM_TYPE_PL010:
		if (!ISSET(sc->sc_cr, PL010_CR_TIE)) {
			SET(sc->sc_cr, PL010_CR_TIE);
			PWRITE1(pi, PL010COM_CR, sc->sc_cr);
		}
		break;
	case PLCOM_TYPE_PL011:
		if (!ISSET(sc->sc_imsc, PL011_INT_TX)) {
			SET(sc->sc_imsc, PL011_INT_TX);
			PWRITE4(pi, PL011COM_IMSC, sc->sc_imsc);
		}
		break;
	}

	/* Output the first chunk of the contiguous buffer. */
	{
		int n;

		n = sc->sc_tbc;
		if (n > sc->sc_fifolen)
			n = sc->sc_fifolen;
		PWRITEM1(pi, PL01XCOM_DR, sc->sc_tba, n);
		sc->sc_tbc -= n;
		sc->sc_tba += n;
	}
	mutex_spin_exit(&sc->sc_lock);
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
	struct plcom_softc *sc =
		device_lookup_private(&plcom_cd, PLCOMUNIT(tp->t_dev));

	mutex_spin_enter(&sc->sc_lock);
	if (ISSET(tp->t_state, TS_BUSY)) {
		/* Stop transmitting at the next chunk. */
		sc->sc_tbc = 0;
		sc->sc_heldtbc = 0;
		if (!ISSET(tp->t_state, TS_TTSTOP))
			SET(tp->t_state, TS_FLUSH);
	}
	mutex_spin_exit(&sc->sc_lock);
}

void
plcomdiag(void *arg)
{
	struct plcom_softc *sc = arg;
	int overflows, floods;

	mutex_spin_enter(&sc->sc_lock);
	overflows = sc->sc_overflows;
	sc->sc_overflows = 0;
	floods = sc->sc_floods;
	sc->sc_floods = 0;
	sc->sc_errors = 0;
	mutex_spin_exit(&sc->sc_lock);

	log(LOG_WARNING, "%s: %d silo overflow%s, %d ibuf flood%s\n",
	    device_xname(sc->sc_dev),
	    overflows, overflows == 1 ? "" : "s",
	    floods, floods == 1 ? "" : "s");
}

integrate void
plcom_rxsoft(struct plcom_softc *sc, struct tty *tp)
{
	int (*rint) (int, struct tty *) = tp->t_linesw->l_rint;
	struct plcom_instance *pi = &sc->sc_pi;
	u_char *get, *end;
	u_int cc, scc;
	u_char rsr;
	int code;

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
		if (ISSET(rsr, PL01X_RSR_ERROR)) {
			if (ISSET(rsr, PL01X_RSR_OE)) {
				sc->sc_overflows++;
				if (sc->sc_errors++ == 0)
					callout_reset(&sc->sc_diag_callout,
					    60 * hz, plcomdiag, sc);
			}
			if (ISSET(rsr, PL01X_RSR_BE | PL01X_RSR_FE))
				SET(code, TTY_FE);
			if (ISSET(rsr, PL01X_RSR_PE))
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
		mutex_spin_enter(&sc->sc_lock);

		cc = sc->sc_rbavail += scc - cc;
		/* Buffers should be ok again, release possible block. */
		if (cc >= sc->sc_r_lowat) {
			if (ISSET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
				switch (pi->pi_type) {
				case PLCOM_TYPE_PL010:
					SET(sc->sc_cr,
					    PL010_CR_RIE | PL010_CR_RTIE);
					PWRITE1(pi, PL010COM_CR, sc->sc_cr);
					break;
				case PLCOM_TYPE_PL011:
					SET(sc->sc_imsc,
					    PL011_INT_RX | PL011_INT_RT);
					PWRITE4(pi, PL011COM_IMSC, sc->sc_imsc);
					break;
				}
			}
			if (ISSET(sc->sc_rx_flags, RX_IBUF_BLOCKED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_BLOCKED);
				plcom_hwiflow(sc);
			}
		}
		mutex_spin_exit(&sc->sc_lock);
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

	mutex_spin_enter(&sc->sc_lock);
	msr = sc->sc_msr;
	delta = sc->sc_msr_delta;
	sc->sc_msr_delta = 0;
	mutex_spin_exit(&sc->sc_lock);

	if (ISSET(delta, sc->sc_msr_dcd)) {
		/*
		 * Inform the tty layer that carrier detect changed.
		 */
		(void) (*tp->t_linesw->l_modem)(tp, ISSET(msr, PL01X_MSR_DCD));
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

bool
plcom_intstatus(struct plcom_instance *pi, u_int *istatus)
{
	bool ret = false;
	u_int stat = 0;

	switch (pi->pi_type) {
	case PLCOM_TYPE_PL010:
		stat = PREAD1(pi, PL010COM_IIR);
		ret = ISSET(stat, PL010_IIR_IMASK);
		break;
	case PLCOM_TYPE_PL011:
		stat = PREAD4(pi, PL011COM_MIS);
		ret = ISSET(stat, PL011_INT_ALLMASK);
		break;
	}
	*istatus = stat;

	return ret;
}

int
plcomintr(void *arg)
{
	struct plcom_softc *sc = arg;
	struct plcom_instance *pi = &sc->sc_pi;
	u_char *put, *end;
	u_int cc;
	u_int istatus = 0;
	u_char rsr;
	bool intr = false;

	PLCOM_BARRIER(pi, BR | BW);

	if (PLCOM_ISALIVE(sc) == 0)
		return 0;

	mutex_spin_enter(&sc->sc_lock);
	intr = plcom_intstatus(pi, &istatus);
	if (!intr) {
		mutex_spin_exit(&sc->sc_lock);
		return 0;
	}

	end = sc->sc_ebuf;
	put = sc->sc_rbput;
	cc = sc->sc_rbavail;

	do {
		u_int msr = 0, delta, fr;
		bool rxintr = false, txintr = false, msintr;

		/* don't need RI here*/
		fr = PREAD1(pi, PL01XCOM_FR);

		if (!ISSET(fr, PL01X_FR_RXFE) &&
		    !ISSET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED)) {
			while (cc > 0) {
				int cn_trapped = 0;
				put[0] = PREAD1(pi, PL01XCOM_DR);
				rsr = PREAD1(pi, PL01XCOM_RSR);
				/* Clear any error status.  */
				if (ISSET(rsr, PL01X_RSR_ERROR))
					PWRITE1(pi, PL01XCOM_ECR, 0);
				if (ISSET(rsr, PL01X_RSR_BE)) {
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
				cn_check_magic(sc->sc_tty->t_dev, put[0],
				    plcom_cnm_state);
				if (cn_trapped) {
					fr = PREAD1(pi, PL01XCOM_FR);
					if (ISSET(fr, PL01X_FR_RXFE))
						break;

					continue;
				}
				put += 2;
				if (put >= end)
					put = sc->sc_rbuf;
				cc--;

				/* don't need RI here*/
				fr = PREAD1(pi, PL01XCOM_FR);
				if (ISSET(fr, PL01X_FR_RXFE))
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
				switch (pi->pi_type) {
				case PLCOM_TYPE_PL010:
					CLR(sc->sc_cr,
					    PL010_CR_RIE | PL010_CR_RTIE);
					PWRITE1(pi, PL010COM_CR, sc->sc_cr);
					break;
				case PLCOM_TYPE_PL011:
					CLR(sc->sc_imsc,
					    PL011_INT_RT | PL011_INT_RX);
					PWRITE4(pi, PL011COM_IMSC, sc->sc_imsc);
					break;
				}
			}
		} else {
			switch (pi->pi_type) {
			case PLCOM_TYPE_PL010:
				rxintr = ISSET(istatus, PL010_IIR_RIS);
				if (rxintr) {
					PWRITE1(pi, PL010COM_CR, 0);
					delay(10);
					PWRITE1(pi, PL010COM_CR, sc->sc_cr);
					continue;
				}
				break;
			case PLCOM_TYPE_PL011:
				rxintr = ISSET(istatus, PL011_INT_RX);
				if (rxintr) {
					PWRITE4(pi, PL011COM_CR, 0);
					delay(10);
					PWRITE4(pi, PL011COM_CR, sc->sc_cr);
					continue;
				}
				break;
			}
		}

		switch (pi->pi_type) {
		case PLCOM_TYPE_PL010:
			msr = PREAD1(pi, PL01XCOM_FR);
			break;
		case PLCOM_TYPE_PL011:
			msr = PREAD4(pi, PL01XCOM_FR);
			break;
		}
		delta = msr ^ sc->sc_msr;
		sc->sc_msr = msr;

		/* Clear any pending modem status interrupt.  */
		switch (pi->pi_type) {
		case PLCOM_TYPE_PL010:
			msintr = ISSET(istatus, PL010_IIR_MIS);
			if (msintr) {
				PWRITE1(pi, PL010COM_ICR, 0);
			}
			break;
		case PLCOM_TYPE_PL011:
			msintr = ISSET(istatus, PL011_INT_MSMASK);
			if (msintr) {
				PWRITE4(pi, PL011COM_ICR, PL011_INT_MSMASK);
			}
			break;
		}
		/*
		 * Pulse-per-second (PSS) signals on edge of DCD?
		 * Process these even if line discipline is ignoring DCD.
		 */
		if (delta & sc->sc_ppsmask) {
			struct timeval tv;
			mutex_spin_enter(&timecounter_lock);
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
			mutex_spin_exit(&timecounter_lock);
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
		 * can be transmitted as well. Schedule tx done
		 * event if no data left and tty was marked busy.
		 */

		switch (pi->pi_type) {
		case PLCOM_TYPE_PL010:
			txintr = ISSET(istatus, PL010_IIR_TIS);
			break;
		case PLCOM_TYPE_PL011:
			txintr = ISSET(istatus, PL011_INT_TX);
			break;
		}
		if (txintr) {
			/*
			 * If we've delayed a parameter change, do it
			 * now, and restart * output.
			 */
// PWRITE4(pi, PL011COM_ICR, PL011_INT_TX);
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
				PWRITEM1(pi, PL01XCOM_DR, sc->sc_tba, n);
				sc->sc_tbc -= n;
				sc->sc_tba += n;
			} else {
				/*
				 * Disable transmit completion
				 * interrupts if necessary.
				 */
				switch (pi->pi_type) {
				case PLCOM_TYPE_PL010:
					if (ISSET(sc->sc_cr, PL010_CR_TIE)) {
						CLR(sc->sc_cr, PL010_CR_TIE);
						PWRITE1(pi, PL010COM_CR,
						    sc->sc_cr);
					}
					break;
				case PLCOM_TYPE_PL011:
					if (ISSET(sc->sc_imsc, PL011_INT_TX)) {
						CLR(sc->sc_imsc, PL011_INT_TX);
						PWRITE4(pi, PL011COM_IMSC,
						    sc->sc_imsc);
					}
					break;
				}
				if (sc->sc_tx_busy) {
					sc->sc_tx_busy = 0;
					sc->sc_tx_done = 1;
				}
			}
		}

	} while (plcom_intstatus(pi, &istatus));

	mutex_spin_exit(&sc->sc_lock);

	/* Wake up the poller. */
	softint_schedule(sc->sc_si);

#ifdef RND_COM
	rnd_add_uint32(&sc->rnd_source, istatus | rsr);
#endif

	PLCOM_BARRIER(pi, BR | BW);

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
plcom_common_getc(dev_t dev, struct plcom_instance *pi)
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
	while (ISSET(stat = PREAD1(pi, PL01XCOM_FR), PL01X_FR_RXFE))
		;

	c = PREAD1(pi, PL01XCOM_DR);
	{
		int cn_trapped __unused = 0;
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
plcom_common_putc(dev_t dev, struct plcom_instance *pi, int c)
{
	int s = splserial();
	int timo;

	int cin, stat;
	if (plcom_readaheadcount < MAX_READAHEAD
	     && !ISSET(stat = PREAD1(pi, PL01XCOM_FR), PL01X_FR_RXFE)) {
		int cn_trapped __unused = 0;
		cin = PREAD1(pi, PL01XCOM_DR);
		cn_check_magic(dev, cin, plcom_cnm_state);
		plcom_readahead[plcom_readaheadcount++] = cin;
	}

	/* wait for any pending transmission to finish */
	timo = 150000;
	while (ISSET(PREAD1(pi, PL01XCOM_FR), PL01X_FR_TXFF) && --timo)
		continue;

	PWRITE1(pi, PL01XCOM_DR, c);
	PLCOM_BARRIER(pi, BR | BW);

	/* wait for this transmission to complete */
	timo = 1500000;
	while (!ISSET(PREAD1(pi, PL01XCOM_FR), PL01X_FR_TXFE) && --timo)
		continue;

	splx(s);
}

/*
 * Initialize UART for use as console or KGDB line.
 */
int
plcominit(struct plcom_instance *pi, int rate, int frequency, tcflag_t cflag)
{
	u_char lcr;

	switch (pi->pi_type) {
	case PLCOM_TYPE_PL010:
		if (pi->pi_size == 0)
			pi->pi_size = PL010COM_UART_SIZE;
		break;
	case PLCOM_TYPE_PL011:
		if (pi->pi_size == 0)
			pi->pi_size = PL011COM_UART_SIZE;
		break;
	default:
		panic("Unknown plcom type");
	}

	if (bus_space_map(pi->pi_iot, pi->pi_iobase, pi->pi_size, 0,
	    &pi->pi_ioh))
		return ENOMEM; /* ??? */

	lcr = cflag2lcr(cflag) | PL01X_LCR_FEN;
	switch (pi->pi_type) {
	case PLCOM_TYPE_PL010:
		PWRITE1(pi, PL010COM_CR, 0);

		rate = pl010comspeed(rate, frequency);
		PWRITE1(pi, PL010COM_DLBL, (rate & 0xff));
		PWRITE1(pi, PL010COM_DLBH, ((rate >> 8) & 0xff));
		PWRITE1(pi, PL010COM_LCR, lcr);
		PWRITE1(pi, PL010COM_CR, PL01X_CR_UARTEN);
		break;
	case PLCOM_TYPE_PL011:
		PWRITE4(pi, PL011COM_CR, 0);

		rate = pl011comspeed(rate, frequency);
		PWRITE1(pi, PL011COM_FBRD, rate & ((1 << 6) - 1));
		PWRITE4(pi, PL011COM_IBRD, rate >> 6);
		PWRITE1(pi, PL011COM_LCRH, lcr);
		PWRITE4(pi, PL011COM_CR,
		    PL01X_CR_UARTEN | PL011_CR_RXE | PL011_CR_TXE);
		break;
	}

#if 0
	/* Ought to do something like this, but we have no sc to
	   dereference. */
	/* XXX device_unit() abuse */
	sc->sc_set_mcr(sc->sc_set_mcr_arg, device_unit(sc->sc_dev),
	    PL01X_MCR_DTR | PL01X_MCR_RTS);
#endif

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
plcomcnattach(struct plcom_instance *pi, int rate, int frequency,
    tcflag_t cflag, int unit)
{
	int res;

	plcomcons_info = *pi;

	res = plcominit(&plcomcons_info, rate, frequency, cflag);
	if (res)
		return res;

	cn_tab = &plcomcons;
	cn_init_magic(&plcom_cnm_state);
	cn_set_magic("\047\001"); /* default magic is BREAK */

	plcomconsunit = unit;
	plcomconsrate = rate;
	plcomconscflag = cflag;

	return 0;
}

void
plcomcndetach(void)
{

	bus_space_unmap(plcomcons_info.pi_iot, plcomcons_info.pi_ioh,
	    plcomcons_info.pi_size);
	plcomcons_info.pi_iot = NULL;

	cn_tab = NULL;
}

int
plcomcngetc(dev_t dev)
{
	return plcom_common_getc(dev, &plcomcons_info);
}

/*
 * Console kernel output character routine.
 */
void
plcomcnputc(dev_t dev, int c)
{
	plcom_common_putc(dev, &plcomcons_info, c);
}

void
plcomcnpollc(dev_t dev, int on)
{

	plcom_readaheadcount = 0;
}

#ifdef KGDB
int
plcom_kgdb_attach(struct plcom_instance *pi, int rate, int frequency,
    tcflag_t cflag, int unit)
{
	int res;

	if (pi->pi_iot == plcomcons_info.pi_iot &&
	    pi->pi_iobase == plcomcons_info.pi_iobase)
		return EBUSY; /* cannot share with console */

	res = plcominit(pi, rate, frequency, cflag);
	if (res)
		return res;

	kgdb_attach(plcom_kgdb_getc, plcom_kgdb_putc, NULL);
	kgdb_dev = 123; /* unneeded, only to satisfy some tests */

	plcomkgdb_info.pi_iot = pi->pi_iot;
	plcomkgdb_info.pi_ioh = pi->pi_ioh;
	plcomkgdb_info.pi_iobase = pi->pi_iobase;

	return 0;
}

/* ARGSUSED */
int
plcom_kgdb_getc(void *arg)
{
	return plcom_common_getc(NODEV, &plcomkgdb_info);
}

/* ARGSUSED */
void
plcom_kgdb_putc(void *arg, int c)
{
	plcom_common_putc(NODEV, &plcomkgdb_info, c);
}
#endif /* KGDB */

/* helper function to identify the plcom ports used by
 console or KGDB (and not yet autoconf attached) */
int
plcom_is_console(bus_space_tag_t iot, bus_addr_t iobase,
    bus_space_handle_t *ioh)
{
	bus_space_handle_t help;

	if (!plcomconsattached &&
	    bus_space_is_equal(iot, plcomcons_info.pi_iot) &&
	    iobase == plcomcons_info.pi_iobase)
		help = plcomcons_info.pi_ioh;
#ifdef KGDB
	else if (!plcom_kgdb_attached &&
	    bus_space_is_equal(iot, plcomkgdb_info.pi_iot) &&
	    iobase == plcomkgdb_info.pi_iobase) 
		help = plcomkgdb_info.pi_ioh;
#endif
	else
		return 0;

	if (ioh)
		*ioh = help;
	return 1;
}

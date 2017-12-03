/*	$NetBSD: zynq_uart.c,v 1.2.18.2 2017/12/03 11:35:57 jdolecek Exp $	*/

/*
 * Copyright (c) 2012  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho, Hashimoto Kenichi for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * derived from sys/dev/ic/com.c
 */

/*-
 * Copyright (c) 1998, 1999, 2004, 2008 The NetBSD Foundation, Inc.
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
 * driver for UART in Zynq-7000.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zynq_uart.c,v 1.2.18.2 2017/12/03 11:35:57 jdolecek Exp $");

#include "opt_zynq.h"
#include "opt_zynquart.h"

#include "opt_com.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_ntp.h"

/*
 * Override cnmagic(9) macro before including <sys/systm.h>.
 * We need to know if cn_check_magic triggered debugger, so set a flag.
 * Callers of cn_check_magic must declare int cn_trapped = 0;
 * XXX: this is *ugly*!
 */
#define	cn_trap()				\
	do {					\
		console_debugger();		\
		cn_trapped = 1;			\
	} while (/* CONSTCOND */ 0)

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/timepps.h>
#include <sys/vnode.h>
#include <sys/kauth.h>
#include <sys/intr.h>
#ifdef RND_COM
#include <sys/rndsource.h>
#endif

#include <sys/bus.h>

#include <arm/zynq/zynq7000_reg.h>
#include <arm/zynq/zynq_uartreg.h>
#include <arm/zynq/zynq_uartvar.h>
#include <dev/cons.h>

#ifndef	ZYNQUART_RING_SIZE
#define	ZYNQUART_RING_SIZE	2048
#endif

typedef struct zynquart_softc {
	device_t	sc_dev;

	struct zynquart_regs {
		bus_space_tag_t		ur_iot;
		bus_space_handle_t	ur_ioh;
		bus_addr_t		ur_iobase;
	} sc_regs;

#define	sc_bt	sc_regs.ur_iot
#define	sc_bh	sc_regs.ur_ioh

	uint32_t	sc_intrspec_enb;
	uint32_t	sc_cr;
	uint32_t	sc_mcr;
	uint32_t	sc_msr;

	uint		sc_init_cnt;

	bus_addr_t	sc_addr;
	bus_size_t	sc_size;
	int		sc_intr;

	u_char	sc_hwflags;
/* Hardware flag masks */
#define	ZYNQUART_HW_FLOW 	__BIT(0)
#define	ZYNQUART_HW_DEV_OK	__BIT(1)
#define	ZYNQUART_HW_CONSOLE	__BIT(2)
#define	ZYNQUART_HW_KGDB 	__BIT(3)

	bool	enabled;

	u_char	sc_swflags;

	u_char sc_rx_flags;
#define	ZYNQUART_RX_TTY_BLOCKED  	__BIT(0)
#define	ZYNQUART_RX_TTY_OVERFLOWED	__BIT(1)
#define	ZYNQUART_RX_IBUF_BLOCKED 	__BIT(2)
#define	ZYNQUART_RX_IBUF_OVERFLOWED	__BIT(3)
#define	ZYNQUART_RX_ANY_BLOCK					\
	(ZYNQUART_RX_TTY_BLOCKED|ZYNQUART_RX_TTY_OVERFLOWED| 	\
	    ZYNQUART_RX_IBUF_BLOCKED|ZYNQUART_RX_IBUF_OVERFLOWED)

	bool	sc_tx_busy, sc_tx_done, sc_tx_stopped;
	bool	sc_rx_ready,sc_st_check;
	u_short	sc_txfifo_len, sc_txfifo_thresh;

	uint16_t	*sc_rbuf;
	u_int		sc_rbuf_size;
	u_int		sc_rbuf_in;
	u_int		sc_rbuf_out;
#define	ZYNQUART_RBUF_AVAIL(sc)					\
	((sc->sc_rbuf_out <= sc->sc_rbuf_in) ?			\
	(sc->sc_rbuf_in - sc->sc_rbuf_out) :			\
	(sc->sc_rbuf_size - (sc->sc_rbuf_out - sc->sc_rbuf_in)))

#define	ZYNQUART_RBUF_SPACE(sc)	\
	((sc->sc_rbuf_in <= sc->sc_rbuf_out ?			    \
	    sc->sc_rbuf_size - (sc->sc_rbuf_out - sc->sc_rbuf_in) : \
	    sc->sc_rbuf_in - sc->sc_rbuf_out) - 1)
/* increment ringbuffer pointer */
#define	ZYNQUART_RBUF_INC(sc,v,i)	(((v) + (i))&((sc->sc_rbuf_size)-1))
	u_int	sc_r_lowat;
	u_int	sc_r_hiwat;

	/* output chunk */
 	u_char *sc_tba;
 	u_int sc_tbc;
	u_int sc_heldtbc;
	/* pending parameter changes */
	u_char	sc_pending;
#define	ZYNQUART_PEND_PARAM	__BIT(0)
#define	ZYNQUART_PEND_SPEED	__BIT(1)


	struct callout sc_diag_callout;
	kmutex_t sc_lock;
	void *sc_ih;		/* interrupt handler */
	void *sc_si;		/* soft interrupt */
	struct tty		*sc_tty;

	/* power management hooks */
	int (*enable)(struct zynquart_softc *);
	void (*disable)(struct zynquart_softc *);

	struct {
		ulong err;
		ulong brk;
		ulong prerr;
		ulong frmerr;
		ulong ovrrun;
	}	sc_errors;

	struct zynquart_baudrate_ratio {
		uint16_t numerator;	/* UBIR */
		uint16_t modulator;	/* UBMR */
	} sc_ratio;

} zynquart_softc_t;


int	zynquartspeed(long, struct zynquart_baudrate_ratio *);
int	zynquartparam(struct tty *, struct termios *);
void	zynquartstart(struct tty *);
int	zynquarthwiflow(struct tty *, int);

void	zynquart_shutdown(struct zynquart_softc *);
void	zynquart_loadchannelregs(struct zynquart_softc *);
void	zynquart_hwiflow(struct zynquart_softc *);
void	zynquart_break(struct zynquart_softc *, bool);
void	zynquart_modem(struct zynquart_softc *, int);
void	tiocm_to_zynquart(struct zynquart_softc *, u_long, int);
int	zynquart_to_tiocm(struct zynquart_softc *);
void	zynquart_iflush(struct zynquart_softc *);
int	zynquartintr(void *);

int	zynquart_common_getc(dev_t, struct zynquart_regs *);
void	zynquart_common_putc(dev_t, struct zynquart_regs *, int);


int	zynquart_init(struct zynquart_regs *, int, tcflag_t);

int	zynquartcngetc(dev_t);
void	zynquartcnputc(dev_t, int);

static void zynquartintr_read(struct zynquart_softc *);
static void zynquartintr_send(struct zynquart_softc *);

static void zynquart_enable_debugport(struct zynquart_softc *);
static void zynquart_disable_all_interrupts(struct zynquart_softc *);
static void zynquart_control_rxint(struct zynquart_softc *, bool);
static void zynquart_control_txint(struct zynquart_softc *, bool);

static	uint32_t cflag_to_zynquart(tcflag_t, uint32_t);

CFATTACH_DECL_NEW(zynquart, sizeof(struct zynquart_softc),
    zynquart_match, zynquart_attach, NULL, NULL);


#define	integrate	static inline
void 	zynquartsoft(void *);
integrate void zynquart_rxsoft(struct zynquart_softc *, struct tty *);
integrate void zynquart_txsoft(struct zynquart_softc *, struct tty *);
integrate void zynquart_stsoft(struct zynquart_softc *, struct tty *);
integrate void zynquart_schedrx(struct zynquart_softc *);
void	zynquartdiag(void *);
static void zynquart_load_speed(struct zynquart_softc *);
static void zynquart_load_params(struct zynquart_softc *);
integrate void zynquart_load_pendings(struct zynquart_softc *);


extern struct cfdriver zynquart_cd;

dev_type_open(zynquartopen);
dev_type_close(zynquartclose);
dev_type_read(zynquartread);
dev_type_write(zynquartwrite);
dev_type_ioctl(zynquartioctl);
dev_type_stop(zynquartstop);
dev_type_tty(zynquarttty);
dev_type_poll(zynquartpoll);

const struct cdevsw zynquart_cdevsw = {
	.d_open = zynquartopen,
	.d_close = zynquartclose,
	.d_read = zynquartread,
	.d_write = zynquartwrite,
	.d_ioctl = zynquartioctl,
	.d_stop = zynquartstop,
	.d_tty = zynquarttty,
	.d_poll = zynquartpoll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

/*
 * Make this an option variable one can patch.
 * But be warned:  this must be a power of 2!
 */
u_int zynquart_rbuf_size = ZYNQUART_RING_SIZE;

/* Stop input when 3/4 of the ring is full; restart when only 1/4 is full. */
u_int zynquart_rbuf_hiwat = (ZYNQUART_RING_SIZE * 1) / 4;
u_int zynquart_rbuf_lowat = (ZYNQUART_RING_SIZE * 3) / 4;

static struct zynquart_regs zynquartconsregs;
static int zynquartconsattached;
static int zynquartconsrate;
static tcflag_t zynquartconscflag;
static struct cnm_state zynquart_cnm_state;

u_int zynquart_freq;
u_int zynquart_freqdiv;

#ifdef KGDB
#include <sys/kgdb.h>

static struct zynquart_regs zynquart_kgdb_regs;
static int zynquart_kgdb_attached;

int	zynquart_kgdb_getc(void *);
void	zynquart_kgdb_putc(void *, int);
#endif /* KGDB */

#define	ZYNQUART_UNIT_MASK	0x7ffff
#define	ZYNQUART_DIALOUT_MASK	0x80000

#define	ZYNQUART_UNIT(x)	(minor(x) & ZYNQUART_UNIT_MASK)
#define	ZYNQUART_DIALOUT(x)	(minor(x) & ZYNQUART_DIALOUT_MASK)

#define	ZYNQUART_ISALIVE(sc)	((sc)->enabled != 0 && \
			 device_is_active((sc)->sc_dev))

#define	BR	BUS_SPACE_BARRIER_READ
#define	BW	BUS_SPACE_BARRIER_WRITE
#define	ZYNQUART_BARRIER(r, f) \
	bus_space_barrier((r)->ur_iot, (r)->ur_ioh, 0, UART_SIZE, (f))


void
zynquart_attach_common(device_t parent, device_t self,
    bus_space_tag_t iot, paddr_t iobase, size_t size, int intr, int flags)
{
	zynquart_softc_t *sc = device_private(self);
	struct zynquart_regs *regsp = &sc->sc_regs;
	struct tty *tp;
	bus_space_handle_t ioh;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;

	if (size <= 0)
		size = UART_SIZE;

	sc->sc_intr = intr;
	regsp->ur_iot = iot;
	regsp->ur_iobase = iobase;

	if (bus_space_map(iot, regsp->ur_iobase, size, 0, &ioh)) {
		return;
	}
	regsp->ur_ioh = ioh;

	callout_init(&sc->sc_diag_callout, 0);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_HIGH);

	sc->sc_cr = bus_space_read_4(iot, ioh, UART_CONTROL);

	/* Disable interrupts before configuring the device. */
	zynquart_disable_all_interrupts(sc);

	if (regsp->ur_iobase == zynquartconsregs.ur_iobase) {
		zynquartconsattached = 1;

		/* Make sure the console is always "hardwired". */
		SET(sc->sc_hwflags, ZYNQUART_HW_CONSOLE);
		SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);
	}

	tp = tty_alloc();
	tp->t_oproc = zynquartstart;
	tp->t_param = zynquartparam;
	tp->t_hwiflow = zynquarthwiflow;

	sc->sc_tty = tp;
	sc->sc_rbuf = malloc(sizeof (*sc->sc_rbuf) * zynquart_rbuf_size,
	    M_DEVBUF, M_NOWAIT);
	sc->sc_rbuf_size = zynquart_rbuf_size;
	sc->sc_rbuf_in = sc->sc_rbuf_out = 0;
	if (sc->sc_rbuf == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "unable to allocate ring buffer\n");
		return;
	}

	sc->sc_txfifo_len = 64;
	sc->sc_txfifo_thresh = 32;

	tty_attach(tp);

	if (ISSET(sc->sc_hwflags, ZYNQUART_HW_CONSOLE)) {
		int maj;

		/* locate the major number */
		maj = cdevsw_lookup_major(&zynquart_cdevsw);

		if (maj != NODEVMAJOR) {
			tp->t_dev = cn_tab->cn_dev = makedev(maj,
			    device_unit(sc->sc_dev));

			aprint_normal_dev(sc->sc_dev, "console\n");
		}
	}

	sc->sc_ih = intr_establish(sc->sc_intr, IPL_SERIAL, IST_LEVEL,
	    zynquartintr, sc);
	if (sc->sc_ih == NULL)
		aprint_error_dev(sc->sc_dev, "intr_establish failed\n");

	/* reset receive time out */
	bus_space_write_4(iot, ioh, UART_RCVR_TIMEOUT, 0);
	bus_space_write_4(iot, ioh, UART_RCVR_FIFO_TRIGGER, 1);

#ifdef KGDB
	/*
	 * Allow kgdb to "take over" this port.  If this is
	 * not the console and is the kgdb device, it has
	 * exclusive use.  If it's the console _and_ the
	 * kgdb device, it doesn't.
	 */
	if (regsp->ur_iobase == zynquart_kgdb_regs.ur_iobase) {
		if (!ISSET(sc->sc_hwflags, ZYNQUART_HW_CONSOLE)) {
			zynquart_kgdb_attached = 1;

			SET(sc->sc_hwflags, ZYNQUART_HW_KGDB);
		}
		aprint_normal_dev(sc->sc_dev, "kgdb\n");
	}
#endif

	sc->sc_si = softint_establish(SOFTINT_SERIAL, zynquartsoft, sc);

#ifdef RND_COM
	rnd_attach_source(&sc->rnd_source, device_xname(sc->sc_dev),
			  RND_TYPE_TTY, 0);
#endif

	/* if there are no enable/disable functions, assume the device
	   is always enabled */
	if (!sc->enable)
		sc->enabled = 1;

	zynquart_enable_debugport(sc);

	SET(sc->sc_hwflags, ZYNQUART_HW_DEV_OK);
}

int
zynquartspeed(long speed, struct zynquart_baudrate_ratio *ratio)
{
	return 0;
}

#ifdef ZYNQUART_DEBUG
int	zynquart_debug = 0;

void zynquartstatus(struct zynquart_softc *, const char *);
void
zynquartstatus(struct zynquart_softc *sc, const char *str)
{
	struct tty *tp = sc->sc_tty;

	aprint_normal_dev(sc->sc_dev,
	    "%s %cclocal  %cdcd %cts_carr_on %cdtr %ctx_stopped\n",
	    str,
	    ISSET(tp->t_cflag, CLOCAL) ? '+' : '-',
	    ISSET(sc->sc_msr, MSR_DCD) ? '+' : '-',
	    ISSET(tp->t_state, TS_CARR_ON) ? '+' : '-',
	    ISSET(sc->sc_mcr, MCR_DTR) ? '+' : '-',
	    sc->sc_tx_stopped ? '+' : '-');

	aprint_normal_dev(sc->sc_dev,
	    "%s %ccrtscts %ccts %cts_ttstop  %crts rx_flags=0x%x\n",
	    str,
	    ISSET(tp->t_cflag, CRTSCTS) ? '+' : '-',
	    ISSET(sc->sc_msr, MSR_CTS) ? '+' : '-',
	    ISSET(tp->t_state, TS_TTSTOP) ? '+' : '-',
	    ISSET(sc->sc_mcr, MCR_RTS) ? '+' : '-',
	    sc->sc_rx_flags);
}
#endif

#if 0
int
zynquart_detach(device_t self, int flags)
{
	struct zynquart_softc *sc = device_private(self);
	int maj, mn;

        if (ISSET(sc->sc_hwflags, ZYNQUART_HW_CONSOLE))
		return EBUSY;

	/* locate the major number */
	maj = cdevsw_lookup_major(&zynquart_cdevsw);

	/* Nuke the vnodes for any open instances. */
	mn = device_unit(self);
	vdevgone(maj, mn, mn, VCHR);

	mn |= ZYNQUART_DIALOUT_MASK;
	vdevgone(maj, mn, mn, VCHR);

	if (sc->sc_rbuf == NULL) {
		/*
		 * Ring buffer allocation failed in the zynquart_attach_subr,
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

	return (0);
}
#endif

#ifdef notyet
int
zynquart_activate(device_t self, enum devact act)
{
	struct zynquart_softc *sc = device_private(self);
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		rv = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		if (sc->sc_hwflags & (ZYNQUART_HW_CONSOLE|ZYNQUART_HW_KGDB)) {
			rv = EBUSY;
			break;
		}

		if (sc->disable != NULL && sc->enabled != 0) {
			(*sc->disable)(sc);
			sc->enabled = 0;
		}
		break;
	}

	return (rv);
}
#endif

void
zynquart_shutdown(struct zynquart_softc *sc)
{
	struct tty *tp = sc->sc_tty;

	mutex_spin_enter(&sc->sc_lock);

	/* If we were asserting flow control, then deassert it. */
	SET(sc->sc_rx_flags, ZYNQUART_RX_IBUF_BLOCKED);
	zynquart_hwiflow(sc);

	/* Clear any break condition set with TIOCSBRK. */
	zynquart_break(sc, false);

	/*
	 * Hang up if necessary.  Wait a bit, so the other side has time to
	 * notice even if we immediately open the port again.
	 * Avoid tsleeping above splhigh().
	 */
	if (ISSET(tp->t_cflag, HUPCL)) {
		zynquart_modem(sc, 0);
		mutex_spin_exit(&sc->sc_lock);
		/* XXX will only timeout */
		(void) kpause(ttclos, false, hz, NULL);
		mutex_spin_enter(&sc->sc_lock);
	}

	/* Turn off interrupts. */
	zynquart_disable_all_interrupts(sc);
	/* re-enable recv interrupt for console or kgdb port */
	zynquart_enable_debugport(sc);

	mutex_spin_exit(&sc->sc_lock);

#ifdef	notyet
	if (sc->disable) {
#ifdef DIAGNOSTIC
		if (!sc->enabled)
			panic("zynquart_shutdown: not enabled?");
#endif
		(*sc->disable)(sc);
		sc->enabled = 0;
	}
#endif
}

int
zynquartopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct zynquart_softc *sc;
	struct tty *tp;
	int s;
	int error;

	sc = device_lookup_private(&zynquart_cd, ZYNQUART_UNIT(dev));
	if (sc == NULL || !ISSET(sc->sc_hwflags, ZYNQUART_HW_DEV_OK) ||
		sc->sc_rbuf == NULL)
		return (ENXIO);

	if (!device_is_active(sc->sc_dev))
		return (ENXIO);

#ifdef KGDB
	/*
	 * If this is the kgdb port, no other use is permitted.
	 */
	if (ISSET(sc->sc_hwflags, ZYNQUART_HW_KGDB))
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


#ifdef notyet
		if (sc->enable) {
			if ((*sc->enable)(sc)) {
				splx(s);
				aprint_error_dev(sc->sc_dev,
				    "device enable failed\n");
				return (EIO);
			}
			sc->enabled = 1;
		}
#endif

		mutex_spin_enter(&sc->sc_lock);

		zynquart_disable_all_interrupts(sc);

		/* Fetch the current modem control status, needed later. */

#ifdef	ZYNQUART_PPS
		/* Clear PPS capture state on first open. */
		mutex_spin_enter(&timecounter_lock);
		memset(&sc->sc_pps_state, 0, sizeof(sc->sc_pps_state));
		sc->sc_pps_state.ppscap = PPS_CAPTUREASSERT | PPS_CAPTURECLEAR;
		pps_init(&sc->sc_pps_state);
		mutex_spin_exit(&timecounter_lock);
#endif

		mutex_spin_exit(&sc->sc_lock);

		/*
		 * Initialize the termios status to the defaults.  Add in the
		 * sticky bits from TIOCSFLAGS.
		 */
		if (ISSET(sc->sc_hwflags, ZYNQUART_HW_CONSOLE)) {
			t.c_ospeed = zynquartconsrate;
			t.c_cflag = zynquartconscflag;
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
		/* Make sure zynquartparam() will do something. */
		tp->t_ospeed = 0;
		(void) zynquartparam(tp, &t);
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
		zynquart_modem(sc, 1);

		/* Clear the input ring, and unblock. */
		sc->sc_rbuf_in = sc->sc_rbuf_out = 0;
		zynquart_iflush(sc);
		CLR(sc->sc_rx_flags, ZYNQUART_RX_ANY_BLOCK);
		zynquart_hwiflow(sc);

		/* Turn on interrupts. */
		zynquart_control_rxint(sc, true);

#ifdef ZYNQUART_DEBUG
		if (zynquart_debug)
			zynquartstatus(sc, "zynquartopen  ");
#endif

		mutex_spin_exit(&sc->sc_lock);
	}

	splx(s);

#if 0
	error = ttyopen(tp, ZYNQUART_DIALOUT(dev), ISSET(flag, O_NONBLOCK));
#else
	error = ttyopen(tp, 1, ISSET(flag, O_NONBLOCK));
#endif
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
		zynquart_shutdown(sc);
	}

	return (error);
}

int
zynquartclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct zynquart_softc *sc =
	    device_lookup_private(&zynquart_cd, ZYNQUART_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	/* XXX This is for cons.c. */
	if (!ISSET(tp->t_state, TS_ISOPEN))
		return (0);

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);

	if (ZYNQUART_ISALIVE(sc) == 0)
		return (0);

	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * Although we got a last close, the device may still be in
		 * use; e.g. if this was the dialout node, and there are still
		 * processes waiting for carrier on the non-dialout node.
		 */
		zynquart_shutdown(sc);
	}

	return (0);
}

int
zynquartread(dev_t dev, struct uio *uio, int flag)
{
	struct zynquart_softc *sc =
	    device_lookup_private(&zynquart_cd, ZYNQUART_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (ZYNQUART_ISALIVE(sc) == 0)
		return (EIO);

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
zynquartwrite(dev_t dev, struct uio *uio, int flag)
{
	struct zynquart_softc *sc =
	    device_lookup_private(&zynquart_cd, ZYNQUART_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (ZYNQUART_ISALIVE(sc) == 0)
		return (EIO);

	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
zynquartpoll(dev_t dev, int events, struct lwp *l)
{
	struct zynquart_softc *sc =
	    device_lookup_private(&zynquart_cd, ZYNQUART_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (ZYNQUART_ISALIVE(sc) == 0)
		return (POLLHUP);

	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

struct tty *
zynquarttty(dev_t dev)
{
	struct zynquart_softc *sc =
	    device_lookup_private(&zynquart_cd, ZYNQUART_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	return (tp);
}

int
zynquartioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct zynquart_softc *sc;
	struct tty *tp;
	int error;

	sc = device_lookup_private(&zynquart_cd, ZYNQUART_UNIT(dev));
	if (sc == NULL)
		return ENXIO;
	if (ZYNQUART_ISALIVE(sc) == 0)
		return (EIO);

	tp = sc->sc_tty;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	error = ttioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

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
		zynquart_break(sc, true);
		break;

	case TIOCCBRK:
		zynquart_break(sc, false);
		break;

	case TIOCSDTR:
		zynquart_modem(sc, 1);
		break;

	case TIOCCDTR:
		zynquart_modem(sc, 0);
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
		tiocm_to_zynquart(sc, cmd, *(int *)data);
		break;

	case TIOCMGET:
		*(int *)data = zynquart_to_tiocm(sc);
		break;

#ifdef notyet
	case PPS_IOC_CREATE:
	case PPS_IOC_DESTROY:
	case PPS_IOC_GETPARAMS:
	case PPS_IOC_SETPARAMS:
	case PPS_IOC_GETCAP:
	case PPS_IOC_FETCH:
#ifdef PPS_SYNC
	case PPS_IOC_KCBIND:
#endif
		mutex_spin_enter(&timecounter_lock);
		error = pps_ioctl(cmd, data, &sc->sc_pps_state);
		mutex_spin_exit(&timecounter_lock);
		break;

	case TIOCDCDTIMESTAMP:	/* XXX old, overloaded  API used by xntpd v3 */
		mutex_spin_enter(&timecounter_lock);
#ifndef PPS_TRAILING_EDGE
		TIMESPEC_TO_TIMEVAL((struct timeval *)data,
		    &sc->sc_pps_state.ppsinfo.assert_timestamp);
#else
		TIMESPEC_TO_TIMEVAL((struct timeval *)data,
		    &sc->sc_pps_state.ppsinfo.clear_timestamp);
#endif
		mutex_spin_exit(&timecounter_lock);
		break;
#endif

	default:
		error = EPASSTHROUGH;
		break;
	}

	mutex_spin_exit(&sc->sc_lock);

#ifdef ZYNQUART_DEBUG
	if (zynquart_debug)
		zynquartstatus(sc, "zynquartioctl ");
#endif

	return (error);
}

integrate void
zynquart_schedrx(struct zynquart_softc *sc)
{
	sc->sc_rx_ready = 1;

	/* Wake up the poller. */
	softint_schedule(sc->sc_si);
}

void
zynquart_break(struct zynquart_softc *sc, bool onoff)
{
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;

	if (onoff)
		SET(sc->sc_cr, CR_STPBRK);
	else
		CLR(sc->sc_cr, CR_STPBRK);

	bus_space_write_4(iot, ioh, UART_CONTROL, sc->sc_cr);
}

void
zynquart_modem(struct zynquart_softc *sc, int onoff)
{
#ifdef notyet
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
			zynquart_loadchannelregs(sc);
	}
#endif
}

void
tiocm_to_zynquart(struct zynquart_softc *sc, u_long how, int ttybits)
{
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;

	u_char combits;

	combits = 0;
	if (ISSET(ttybits, TIOCM_DTR))
		SET(combits, MODEMCR_DTR);
	if (ISSET(ttybits, TIOCM_RTS))
		SET(combits, MODEMCR_RTS);

	switch (how) {
	case TIOCMBIC:
		CLR(sc->sc_mcr, combits);
		break;

	case TIOCMBIS:
		SET(sc->sc_mcr, combits);
		break;

	case TIOCMSET:
		CLR(sc->sc_mcr, MODEMCR_DTR | MODEMCR_RTS);
		SET(sc->sc_mcr, combits);
		break;
	}

	bus_space_write_4(iot, ioh, UART_MODEM_CTRL, sc->sc_mcr);
}

int
zynquart_to_tiocm(struct zynquart_softc *sc)
{
#ifdef	notyet
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;
#endif
	uint32_t combits;
	int ttybits = 0;

	combits = sc->sc_mcr;
	if (ISSET(combits, MODEMCR_DTR))
		SET(ttybits, TIOCM_DTR);
	if (ISSET(combits, MODEMCR_RTS))
		SET(ttybits, TIOCM_RTS);

	combits = sc->sc_msr;
	if (ISSET(combits, MODEMSR_DCD))
		SET(ttybits, TIOCM_CD);
	if (ISSET(combits, MODEMSR_CTS))
		SET(ttybits, TIOCM_CTS);
	if (ISSET(combits, MODEMSR_DSR))
		SET(ttybits, TIOCM_DSR);
	if (ISSET(combits, MODEMSR_RI | MODEMSR_TERI))
		SET(ttybits, TIOCM_RI);

#ifdef	notyet
	combits = bus_space_read_4(iot, ioh, UART_INTRPT_MASK);
	if (ISSET(sc->sc_imr, IER_ERXRDY | IER_ETXRDY | IER_ERLS | IER_EMSC))
		SET(ttybits, TIOCM_LE);
#endif

	return (ttybits);
}

static uint32_t
cflag_to_zynquart(tcflag_t cflag, uint32_t oldval)
{
	uint32_t val = oldval;

	CLR(val, MR_CHMODE | MR_NBSTOP | MR_PAR | MR_CHRL | MR_CLKS);

	switch (cflag & CSIZE) {
	case CS5:
		/* not suppreted. use 7-bits */
	case CS6:
		SET(val, CHRL_6BIT);
		break;
	case CS7:
		SET(val, CHRL_7BIT);
		break;
	case CS8:
		SET(val, CHRL_8BIT);
		break;
	}

	if (ISSET(cflag, PARENB)) {
		/* odd parity */
		if (!ISSET(cflag, PARODD))
			SET(val, PAR_ODD);
		else
			SET(val, PAR_EVEN);
	} else {
		SET(val, PAR_NONE);
	}

	if (ISSET(cflag, CSTOPB))
		SET(val, NBSTOP_2);

	return val;
}

int
zynquartparam(struct tty *tp, struct termios *t)
{
	struct zynquart_softc *sc =
	    device_lookup_private(&zynquart_cd, ZYNQUART_UNIT(tp->t_dev));
	struct zynquart_baudrate_ratio ratio;
	uint32_t mcr;
	bool change_speed = tp->t_ospeed != t->c_ospeed;

	if (ZYNQUART_ISALIVE(sc) == 0)
		return (EIO);

	/* Check requested parameters. */
	if (t->c_ispeed && t->c_ispeed != t->c_ospeed)
		return (EINVAL);

	/*
	 * For the console, always force CLOCAL and !HUPCL, so that the port
	 * is always active.
	 */
	if (ISSET(sc->sc_swflags, TIOCFLAG_SOFTCAR) ||
	    ISSET(sc->sc_hwflags, ZYNQUART_HW_CONSOLE)) {
		SET(t->c_cflag, CLOCAL);
		CLR(t->c_cflag, HUPCL);
	}

	/*
	 * If there were no changes, don't do anything.  This avoids dropping
	 * input and improves performance when all we did was frob things like
	 * VMIN and VTIME.
	 */
	if ( !change_speed && tp->t_cflag == t->c_cflag)
		return (0);

	if (change_speed) {
		/* calculate baudrate modulator value */
		if (zynquartspeed(t->c_ospeed, &ratio) < 0)
			return (EINVAL);
		sc->sc_ratio = ratio;
	}

	mcr = cflag_to_zynquart(t->c_cflag, sc->sc_mcr);

	mutex_spin_enter(&sc->sc_lock);

#if 0
	/* flow control stuff.  not yet */
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
		sc->sc_efr = EFR_AUTORTS | EFR_AUTOCTS;
	} else if (ISSET(t->c_cflag, MDMBUF)) {
		/*
		 * For DTR/DCD flow control, make sure we don't toggle DTR for
		 * carrier detection.
		 */
		sc->sc_mcr_dtr = 0;
		sc->sc_mcr_rts = MCR_DTR;
		sc->sc_msr_cts = MSR_DCD;
		sc->sc_efr = 0;
	} else {
		/*
		 * If no flow control, then always set RTS.  This will make
		 * the other side happy if it mistakenly thinks we're doing
		 * RTS/CTS flow control.
		 */
		sc->sc_mcr_dtr = MCR_DTR | MCR_RTS;
		sc->sc_mcr_rts = 0;
		sc->sc_msr_cts = 0;
		sc->sc_efr = 0;
		if (ISSET(sc->sc_mcr, MCR_DTR))
			SET(sc->sc_mcr, MCR_RTS);
		else
			CLR(sc->sc_mcr, MCR_RTS);
	}
	sc->sc_msr_mask = sc->sc_msr_cts | sc->sc_msr_dcd;
#endif

	/* And copy to tty. */
	tp->t_ispeed = t->c_ospeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	if (!change_speed && mcr == sc->sc_mcr) {
		/* noop */
	} else if (!sc->sc_pending && !sc->sc_tx_busy) {
		if (mcr != sc->sc_mcr) {
			sc->sc_mcr = mcr;
			zynquart_load_params(sc);
		}
		if (change_speed)
			zynquart_load_speed(sc);
	} else {
		if (!sc->sc_pending) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
		}
		sc->sc_pending |=
		    (mcr == sc->sc_mcr ? 0 : ZYNQUART_PEND_PARAM) |
		    (change_speed ? 0 : ZYNQUART_PEND_SPEED);
		sc->sc_mcr = mcr;
	}

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		/* Disable the high water mark. */
		sc->sc_r_hiwat = 0;
		sc->sc_r_lowat = 0;
		if (ISSET(sc->sc_rx_flags, ZYNQUART_RX_TTY_OVERFLOWED)) {
			CLR(sc->sc_rx_flags, ZYNQUART_RX_TTY_OVERFLOWED);
			zynquart_schedrx(sc);
		}
		if (ISSET(sc->sc_rx_flags,
			ZYNQUART_RX_TTY_BLOCKED|ZYNQUART_RX_IBUF_BLOCKED)) {
			CLR(sc->sc_rx_flags,
			    ZYNQUART_RX_TTY_BLOCKED|ZYNQUART_RX_IBUF_BLOCKED);
			zynquart_hwiflow(sc);
		}
	} else {
		sc->sc_r_hiwat = zynquart_rbuf_hiwat;
		sc->sc_r_lowat = zynquart_rbuf_lowat;
	}

	mutex_spin_exit(&sc->sc_lock);

	/*
	 * Update the tty layer's idea of the carrier bit, in case we changed
	 * CLOCAL or MDMBUF.  We don't hang up here; we only do that by
	 * explicit request.
	 */
	(void) (*tp->t_linesw->l_modem)(tp, ISSET(sc->sc_msr, MODEMSR_DCD));

#ifdef ZYNQUART_DEBUG
	if (zynquart_debug)
		zynquartstatus(sc, "zynquartparam ");
#endif

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		if (sc->sc_tx_stopped) {
			sc->sc_tx_stopped = 0;
			zynquartstart(tp);
		}
	}

	return (0);
}

void
zynquart_iflush(struct zynquart_softc *sc)
{
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;
#ifdef DIAGNOSTIC
	uint32_t reg = 0xffff;
#endif
	int timo;

	timo = 50000;
	/* flush any pending I/O */
	while (!ISSET(bus_space_read_4(iot, ioh, UART_CHANNEL_STS), STS_REMPTY) &&
	    --timo)
#ifdef DIAGNOSTIC
		reg =
#else
		    (void)
#endif
		    bus_space_read_4(iot, ioh, UART_TX_RX_FIFO);

#ifdef DIAGNOSTIC
	if (!timo)
		aprint_error_dev(sc->sc_dev, "zynquart_iflush timeout %02x\n", reg);
#endif
}

int
zynquarthwiflow(struct tty *tp, int block)
{
	struct zynquart_softc *sc =
	    device_lookup_private(&zynquart_cd, ZYNQUART_UNIT(tp->t_dev));

	if (ZYNQUART_ISALIVE(sc) == 0)
		return (0);

#ifdef notyet
	if (sc->sc_mcr_rts == 0)
		return (0);
#endif

	mutex_spin_enter(&sc->sc_lock);

	if (block) {
		if (!ISSET(sc->sc_rx_flags, ZYNQUART_RX_TTY_BLOCKED)) {
			SET(sc->sc_rx_flags, ZYNQUART_RX_TTY_BLOCKED);
			zynquart_hwiflow(sc);
		}
	} else {
		if (ISSET(sc->sc_rx_flags, ZYNQUART_RX_TTY_OVERFLOWED)) {
			CLR(sc->sc_rx_flags, ZYNQUART_RX_TTY_OVERFLOWED);
			zynquart_schedrx(sc);
		}
		if (ISSET(sc->sc_rx_flags, ZYNQUART_RX_TTY_BLOCKED)) {
			CLR(sc->sc_rx_flags, ZYNQUART_RX_TTY_BLOCKED);
			zynquart_hwiflow(sc);
		}
	}

	mutex_spin_exit(&sc->sc_lock);
	return (1);
}

/*
 * (un)block input via hw flowcontrol
 */
void
zynquart_hwiflow(struct zynquart_softc *sc)
{
#ifdef notyet
	struct zynquart_regs *regsp= &sc->sc_regs;

	if (sc->sc_mcr_rts == 0)
		return;

	if (ISSET(sc->sc_rx_flags, RX_ANY_BLOCK)) {
		CLR(sc->sc_mcr, sc->sc_mcr_rts);
		CLR(sc->sc_mcr_active, sc->sc_mcr_rts);
	} else {
		SET(sc->sc_mcr, sc->sc_mcr_rts);
		SET(sc->sc_mcr_active, sc->sc_mcr_rts);
	}
	UR_WRITE_1(regsp, ZYNQUART_REG_MCR, sc->sc_mcr_active);
#endif
}


void
zynquartstart(struct tty *tp)
{
	struct zynquart_softc *sc =
	    device_lookup_private(&zynquart_cd, ZYNQUART_UNIT(tp->t_dev));
	int s;
	u_char *tba;
	int tbc;
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;

	if (ZYNQUART_ISALIVE(sc) == 0)
		return;

	s = spltty();
	if (ISSET(tp->t_state, TS_BUSY | TS_TIMEOUT | TS_TTSTOP))
		goto out;
	if (sc->sc_tx_stopped)
		goto out;
	if (!ttypull(tp))
		goto out;

	/* Grab the first contiguous region of buffer space. */
	tba = tp->t_outq.c_cf;
	tbc = ndqb(&tp->t_outq, 0);

	mutex_spin_enter(&sc->sc_lock);

	sc->sc_tba = tba;
	sc->sc_tbc = tbc;

	SET(tp->t_state, TS_BUSY);
	sc->sc_tx_busy = 1;

	while (sc->sc_tbc > 0 &&
	    !(bus_space_read_4(iot, ioh, UART_CHANNEL_STS) & STS_TFUL)) {
		bus_space_write_4(iot, ioh, UART_TX_RX_FIFO, *sc->sc_tba);
		sc->sc_tbc--;
		sc->sc_tba++;
	}

	/* Enable transmit completion interrupts */
	zynquart_control_txint(sc, true);

	mutex_spin_exit(&sc->sc_lock);
out:
	splx(s);
	return;
}

/*
 * Stop output on a line.
 */
void
zynquartstop(struct tty *tp, int flag)
{
	struct zynquart_softc *sc =
	    device_lookup_private(&zynquart_cd, ZYNQUART_UNIT(tp->t_dev));

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
zynquartdiag(void *arg)
{
#ifdef notyet
	struct zynquart_softc *sc = arg;
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
#endif
}

integrate void
zynquart_rxsoft(struct zynquart_softc *sc, struct tty *tp)
{
	int (*rint)(int, struct tty *) = tp->t_linesw->l_rint;
	u_int cc, scc, outp;
	uint16_t data;
	u_int code;

	scc = cc = ZYNQUART_RBUF_AVAIL(sc);

#if 0
	if (cc == zynquart_rbuf_size-1) {
		sc->sc_floods++;
		if (sc->sc_errors++ == 0)
			callout_reset(&sc->sc_diag_callout, 60 * hz,
			    zynquartdiag, sc);
	}
#endif

	/* If not yet open, drop the entire buffer content here */
	if (!ISSET(tp->t_state, TS_ISOPEN)) {
		sc->sc_rbuf_out = sc->sc_rbuf_in;
		cc = 0;
	}

	outp = sc->sc_rbuf_out;

#define	ERRBITS (INT_PARE|INT_FRAME|INT_ROVR)

	while (cc) {
	        data = sc->sc_rbuf[outp];
		code = data & 0xff;
		if (ISSET(__SHIFTOUT(data, ERROR_BITS), ERRBITS)) {
			if (sc->sc_errors.err == 0)
				callout_reset(&sc->sc_diag_callout,
				    60 * hz, zynquartdiag, sc);
			if (ISSET(__SHIFTOUT(data, ERROR_BITS), INT_ROVR))
				sc->sc_errors.ovrrun++;
			if (ISSET(__SHIFTOUT(data, ERROR_BITS), INT_FRAME)) {
				sc->sc_errors.frmerr++;
				SET(code, TTY_FE);
			}
			if (ISSET(__SHIFTOUT(data, ERROR_BITS), INT_PARE)) {
				sc->sc_errors.prerr++;
				SET(code, TTY_PE);
			}
		}
		if ((*rint)(code, tp) == -1) {
			/*
			 * The line discipline's buffer is out of space.
			 */
			if (!ISSET(sc->sc_rx_flags, ZYNQUART_RX_TTY_BLOCKED)) {
				/*
				 * We're either not using flow control, or the
				 * line discipline didn't tell us to block for
				 * some reason.  Either way, we have no way to
				 * know when there's more space available, so
				 * just drop the rest of the data.
				 */
				sc->sc_rbuf_out = sc->sc_rbuf_in;
				cc = 0;
			} else {
				/*
				 * Don't schedule any more receive processing
				 * until the line discipline tells us there's
				 * space available (through zynquarthwiflow()).
				 * Leave the rest of the data in the input
				 * buffer.
				 */
				SET(sc->sc_rx_flags, ZYNQUART_RX_TTY_OVERFLOWED);
			}
			break;
		}
		outp = ZYNQUART_RBUF_INC(sc, outp, 1);
		cc--;
	}

	if (cc != scc) {
		sc->sc_rbuf_out = outp;
		mutex_spin_enter(&sc->sc_lock);

		cc = ZYNQUART_RBUF_SPACE(sc);

		/* Buffers should be ok again, release possible block. */
		if (cc >= sc->sc_r_lowat) {
			if (ISSET(sc->sc_rx_flags, ZYNQUART_RX_IBUF_OVERFLOWED)) {
				CLR(sc->sc_rx_flags, ZYNQUART_RX_IBUF_OVERFLOWED);
				zynquart_control_rxint(sc, true);
			}
			if (ISSET(sc->sc_rx_flags, ZYNQUART_RX_IBUF_BLOCKED)) {
				CLR(sc->sc_rx_flags, ZYNQUART_RX_IBUF_BLOCKED);
				zynquart_hwiflow(sc);
			}
		}
		mutex_spin_exit(&sc->sc_lock);
	}
}

integrate void
zynquart_txsoft(struct zynquart_softc *sc, struct tty *tp)
{

	CLR(tp->t_state, TS_BUSY);
	if (ISSET(tp->t_state, TS_FLUSH))
		CLR(tp->t_state, TS_FLUSH);
	else
		ndflush(&tp->t_outq, (int)(sc->sc_tba - tp->t_outq.c_cf));
	(*tp->t_linesw->l_start)(tp);
}

integrate void
zynquart_stsoft(struct zynquart_softc *sc, struct tty *tp)
{
#ifdef notyet
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

#endif
#ifdef ZYNQUART_DEBUG
	if (zynquart_debug)
		zynquartstatus(sc, "zynquart_stsoft");
#endif
}

void
zynquartsoft(void *arg)
{
	struct zynquart_softc *sc = arg;
	struct tty *tp;

	if (ZYNQUART_ISALIVE(sc) == 0)
		return;

	tp = sc->sc_tty;

	if (sc->sc_rx_ready) {
		sc->sc_rx_ready = 0;
		zynquart_rxsoft(sc, tp);
	}

	if (sc->sc_st_check) {
		sc->sc_st_check = 0;
		zynquart_stsoft(sc, tp);
	}

	if (sc->sc_tx_done) {
		sc->sc_tx_done = 0;
		zynquart_txsoft(sc, tp);
	}
}

int
zynquartintr(void *arg)
{
	struct zynquart_softc *sc = arg;
	uint32_t sts;
	uint32_t int_sts;
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;

	if (ZYNQUART_ISALIVE(sc) == 0)
		return (0);

	mutex_spin_enter(&sc->sc_lock);

	int_sts = bus_space_read_4(iot, ioh, UART_CHNL_INT_STS);
	do {
		sts = bus_space_read_4(iot, ioh, UART_CHANNEL_STS);
		if (!(sts & STS_REMPTY))
			zynquartintr_read(sc);
	} while (!(sts & STS_REMPTY));

	if (sts & STS_TEMPTY)
		zynquartintr_send(sc);

	bus_space_write_4(iot, ioh, UART_CHNL_INT_STS, int_sts);

	mutex_spin_exit(&sc->sc_lock);

	/* Wake up the poller. */
	softint_schedule(sc->sc_si);

#ifdef RND_COM
	rnd_add_uint32(&sc->rnd_source, iir | lsr);
#endif

	return (1);
}


/*
 * called when there is least one character in rxfifo
 *
 */

static void
zynquartintr_read(struct zynquart_softc *sc)
{
	int cc;
	uint16_t rd;
	uint32_t sts;
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;

	cc = ZYNQUART_RBUF_SPACE(sc);

	/* clear aging timer interrupt */
	bus_space_write_4(iot, ioh, UART_CHNL_INT_STS, INT_TIMEOUT);

	while (cc > 0) {
		int cn_trapped = 0;

		sc->sc_rbuf[sc->sc_rbuf_in] = rd =
		    bus_space_read_4(iot, ioh, UART_TX_RX_FIFO);

		cn_check_magic(sc->sc_tty->t_dev,
		    rd & 0xff, zynquart_cnm_state);

		if (!cn_trapped) {
			sc->sc_rbuf_in = ZYNQUART_RBUF_INC(sc, sc->sc_rbuf_in, 1);
			cc--;
		}

		sts = bus_space_read_4(iot, ioh, UART_CHANNEL_STS);
		if (sts & STS_REMPTY)
			break;
	}

	/*
	 * Current string of incoming characters ended because
	 * no more data was available or we ran out of space.
	 * Schedule a receive event if any data was received.
	 * If we're out of space, turn off receive interrupts.
	 */
	if (!ISSET(sc->sc_rx_flags, ZYNQUART_RX_TTY_OVERFLOWED))
		sc->sc_rx_ready = 1;
	/*
	 * See if we are in danger of overflowing a buffer. If
	 * so, use hardware flow control to ease the pressure.
	 */
	if (!ISSET(sc->sc_rx_flags, ZYNQUART_RX_IBUF_BLOCKED) &&
	    cc < sc->sc_r_hiwat) {
		sc->sc_rx_flags |= ZYNQUART_RX_IBUF_BLOCKED;
		zynquart_hwiflow(sc);
	}

	/*
	 * If we're out of space, disable receive interrupts
	 * until the queue has drained a bit.
	 */
	if (!cc) {
		sc->sc_rx_flags |= ZYNQUART_RX_IBUF_OVERFLOWED;
		zynquart_control_rxint(sc, false);
	}
}

void
zynquartintr_send(struct zynquart_softc *sc)
{
	uint32_t sts;
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;

	sts = bus_space_read_4(iot, ioh, UART_CHANNEL_STS);

	if (sc->sc_pending) {
		if (sts & STS_TEMPTY) {
			zynquart_load_pendings(sc);
			sc->sc_tbc = sc->sc_heldtbc;
			sc->sc_heldtbc = 0;
		} else {
			/* wait for TX fifo empty */
			zynquart_control_txint(sc, true);
			return;
		}
	}

	while (sc->sc_tbc > 0 &&
	    !(bus_space_read_4(iot, ioh, UART_CHANNEL_STS) & STS_TFUL)) {
		bus_space_write_4(iot, ioh, UART_TX_RX_FIFO, *sc->sc_tba);
		sc->sc_tbc--;
		sc->sc_tba++;
	}

	if (sc->sc_tbc > 0)
		zynquart_control_txint(sc, true);
	else {
		/* no more chars to send.
		   we don't need tx interrupt any more. */
		zynquart_control_txint(sc, false);
		if (sc->sc_tx_busy) {
			sc->sc_tx_busy = 0;
			sc->sc_tx_done = 1;
		}
	}
}

static void
zynquart_disable_all_interrupts(struct zynquart_softc *sc)
{
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;

	bus_space_write_4(iot, ioh, UART_INTRPT_DIS, 0xffffffff);
}

static void
zynquart_control_rxint(struct zynquart_softc *sc, bool enable)
{
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;
	uint32_t mask = INT_TIMEOUT | INT_PARE | INT_FRAME | INT_ROVR | INT_RFUL | INT_RTRIG;
	uint32_t sts;

	/* clear */
	sts = bus_space_read_4(iot, ioh, UART_CHNL_INT_STS);
	bus_space_write_4(iot, ioh, UART_CHNL_INT_STS, sts);

	if (enable)
		bus_space_write_4(iot, ioh, UART_INTRPT_EN, mask);
	else
		bus_space_write_4(iot, ioh, UART_INTRPT_DIS, mask);
}

static void
zynquart_control_txint(struct zynquart_softc *sc, bool enable)
{
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;
	uint32_t mask = INT_TEMPTY;

	if (enable)
		bus_space_write_4(iot, ioh, UART_INTRPT_EN, mask);
	else
		bus_space_write_4(iot, ioh, UART_INTRPT_DIS, mask);
}


static void
zynquart_load_params(struct zynquart_softc *sc)
{
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;

	bus_space_write_4(iot, ioh, UART_MODE, sc->sc_mcr);
}

static void
zynquart_load_speed(struct zynquart_softc *sc)
{
	/* bus_space_tag_t iot = sc->sc_regs.ur_iot; */
	/* bus_space_handle_t ioh = sc->sc_regs.ur_ioh; */

	/* XXX */
}


static void
zynquart_load_pendings(struct zynquart_softc *sc)
{
	if (sc->sc_pending & ZYNQUART_PEND_PARAM)
		zynquart_load_params(sc);
	if (sc->sc_pending & ZYNQUART_PEND_SPEED)
		zynquart_load_speed(sc);
	sc->sc_pending = 0;
}

#if defined(ZYNQUARTCONSOLE) || defined(KGDB)

/*
 * The following functions are polled getc and putc routines, shared
 * by the console and kgdb glue.
 *
 * The read-ahead code is so that you can detect pending in-band
 * cn_magic in polled mode while doing output rather than having to
 * wait until the kernel decides it needs input.
 */

#define	READAHEAD_RING_LEN	16
static int zynquart_readahead[READAHEAD_RING_LEN];
static int zynquart_readahead_in = 0;
static int zynquart_readahead_out = 0;
#define	READAHEAD_IS_EMPTY()	(zynquart_readahead_in==zynquart_readahead_out)
#define	READAHEAD_IS_FULL()	\
	(((zynquart_readahead_in+1) & (READAHEAD_RING_LEN-1)) ==zynquart_readahead_out)

int
zynquart_common_getc(dev_t dev, struct zynquart_regs *regsp)
{
	int s = splserial();
	u_char c;
	bus_space_tag_t iot = regsp->ur_iot;
	bus_space_handle_t ioh = regsp->ur_ioh;
	uint32_t sts;

	/* got a character from reading things earlier */
	if (zynquart_readahead_in != zynquart_readahead_out) {

		c = zynquart_readahead[zynquart_readahead_out];
		zynquart_readahead_out = (zynquart_readahead_out + 1) &
		    (READAHEAD_RING_LEN-1);
		splx(s);
		return (c);
	}

	/* block until a character becomes available */
	while ((sts = bus_space_read_4(iot, ioh, UART_CHANNEL_STS)) & STS_REMPTY)
		;

	c = 0xff & bus_space_read_4(iot, ioh, UART_TX_RX_FIFO);

	{
		int __attribute__((__unused__))cn_trapped = 0; /* unused */
#ifdef DDB
		extern int db_active;
		if (!db_active)
#endif
			cn_check_magic(dev, c, zynquart_cnm_state);
	}
	splx(s);
	return (c);
}

void
zynquart_common_putc(dev_t dev, struct zynquart_regs *regsp, int c)
{
	int s = splserial();
	int cin, timo;
	bus_space_tag_t iot = regsp->ur_iot;
	bus_space_handle_t ioh = regsp->ur_ioh;

	if (!READAHEAD_IS_FULL() &&
	    !(bus_space_read_4(iot, ioh, UART_CHANNEL_STS) & STS_REMPTY)) {

		int __attribute__((__unused__))cn_trapped = 0;
		cin = bus_space_read_4(iot, ioh, UART_TX_RX_FIFO);
		cn_check_magic(dev, cin & 0xff, zynquart_cnm_state);
		zynquart_readahead_in = (zynquart_readahead_in + 1) &
		    (READAHEAD_RING_LEN-1);
	}

	/* wait for any pending transmission to finish */
	timo = 150000;
	do {
		if (!(bus_space_read_4(iot, ioh, UART_CHANNEL_STS) & STS_TFUL)) {
			bus_space_write_4(iot, ioh, UART_TX_RX_FIFO, c);
			break;
		}
	} while(--timo > 0);

	ZYNQUART_BARRIER(regsp, BR | BW);

	splx(s);
}

/*
 * Initialize UART for use as console or KGDB line.
 */
int
zynquart_init(struct zynquart_regs *regsp, int rate, tcflag_t cflag)
{
	struct zynquart_baudrate_ratio ratio;

	if (bus_space_map(regsp->ur_iot, regsp->ur_iobase, UART_SIZE, 0,
		&regsp->ur_ioh))
		return ENOMEM; /* ??? */

	if (zynquartspeed(rate, &ratio) < 0)
		return EINVAL;

	/* clear status registers */
	bus_space_write_4(regsp->ur_iot, regsp->ur_ioh, UART_CHNL_INT_STS, 0xffff);
	bus_space_write_4(regsp->ur_iot, regsp->ur_ioh, UART_CHANNEL_STS, 0xffff);

	return (0);
}


#endif


#ifdef	ZYNQUARTCONSOLE
/*
 * Following are all routines needed for UART to act as console
 */
struct consdev zynquartcons = {
	.cn_getc = zynquartcngetc,
	.cn_putc = zynquartcnputc,
	.cn_pollc = nullcnpollc
};


int
zynquart_cons_attach(bus_space_tag_t iot, paddr_t iobase, u_int rate,
		    tcflag_t cflag)
{
	struct zynquart_regs regs;
	int res;

	regs.ur_iot = iot;
	regs.ur_iobase = iobase;

	res = zynquart_init(&regs, rate, cflag);
	if (res)
		return (res);

	cn_tab = &zynquartcons;
	cn_init_magic(&zynquart_cnm_state);
	cn_set_magic("\047\001"); /* default magic is BREAK */

	zynquartconsrate = rate;
	zynquartconscflag = cflag;

	zynquartconsregs = regs;

	return 0;
}

int
zynquartcngetc(dev_t dev)
{
	return (zynquart_common_getc(dev, &zynquartconsregs));
}

/*
 * Console kernel output character routine.
 */
void
zynquartcnputc(dev_t dev, int c)
{
	zynquart_common_putc(dev, &zynquartconsregs, c);
}

#endif	/* ZYNQUARTCONSOLE */

#ifdef KGDB
int
zynquart_kgdb_attach(bus_space_tag_t iot, paddr_t iobase, u_int rate,
    tcflag_t cflag)
{
	int res;

	if (iot == zynquartconsregs.ur_iot &&
	    iobase == zynquartconsregs.ur_iobase) {
#if !defined(DDB)
		return (EBUSY); /* cannot share with console */
#else
		zynquart_kgdb_regs.ur_iot = iot;
		zynquart_kgdb_regs.ur_ioh = zynquartconsregs.ur_ioh;
		zynquart_kgdb_regs.ur_iobase = iobase;
#endif
	} else {
		zynquart_kgdb_regs.ur_iot = iot;
		zynquart_kgdb_regs.ur_iobase = iobase;

		res = zynquart_init(&zynquart_kgdb_regs, rate, cflag);
		if (res)
			return (res);

		/*
		 * XXXfvdl this shouldn't be needed, but the cn_magic goo
		 * expects this to be initialized
		 */
		cn_init_magic(&zynquart_cnm_state);
		cn_set_magic("\047\001");
	}

	kgdb_attach(zynquart_kgdb_getc, zynquart_kgdb_putc, &zynquart_kgdb_regs);
	kgdb_dev = 123; /* unneeded, only to satisfy some tests */

	return (0);
}

/* ARGSUSED */
int
zynquart_kgdb_getc(void *arg)
{
	struct zynquart_regs *regs = arg;

	return (zynquart_common_getc(NODEV, regs));
}

/* ARGSUSED */
void
zynquart_kgdb_putc(void *arg, int c)
{
	struct zynquart_regs *regs = arg;

	zynquart_common_putc(NODEV, regs, c);
}
#endif /* KGDB */

/* helper function to identify the zynquart ports used by
 console or KGDB (and not yet autoconf attached) */
int
zynquart_is_console(bus_space_tag_t iot, bus_addr_t iobase, bus_space_handle_t *ioh)
{
	bus_space_handle_t help;

	if (!zynquartconsattached &&
	    iot == zynquartconsregs.ur_iot && iobase == zynquartconsregs.ur_iobase)
		help = zynquartconsregs.ur_ioh;
#ifdef KGDB
	else if (!zynquart_kgdb_attached &&
	    iot == zynquart_kgdb_regs.ur_iot && iobase == zynquart_kgdb_regs.ur_iobase)
		help = zynquart_kgdb_regs.ur_ioh;
#endif
	else
		return (0);

	if (ioh)
		*ioh = help;
	return (1);
}

#ifdef notyet

bool
zynquart_cleanup(device_t self, int how)
{
/*
 * this routine exists to serve as a shutdown hook for systems that
 * have firmware which doesn't interact properly with a zynquart device in
 * FIFO mode.
 */
	struct zynquart_softc *sc = device_private(self);

	if (ISSET(sc->sc_hwflags, ZYNQUART_HW_FIFO))
		UR_WRITE_1(&sc->sc_regs, ZYNQUART_REG_FIFO, 0);

	return true;
}
#endif

#ifdef notyet
bool
zynquart_suspend(device_t self PMF_FN_ARGS)
{
	struct zynquart_softc *sc = device_private(self);

	UR_WRITE_1(&sc->sc_regs, ZYNQUART_REG_IER, 0);
	(void)CSR_READ_1(&sc->sc_regs, ZYNQUART_REG_IIR);

	return true;
}
#endif

#ifdef notyet
bool
zynquart_resume(device_t self PMF_FN_ARGS)
{
	struct zynquart_softc *sc = device_private(self);

	mutex_spin_enter(&sc->sc_lock);
	zynquart_loadchannelregs(sc);
	mutex_spin_exit(&sc->sc_lock);

	return true;
}
#endif

static void
zynquart_enable_debugport(struct zynquart_softc *sc)
{
	/* bus_space_tag_t iot = sc->sc_regs.ur_iot; */
	/* bus_space_handle_t ioh = sc->sc_regs.ur_ioh; */
}


void
zynquart_set_frequency(u_int freq, u_int div)
{
	zynquart_freq = freq;
	zynquart_freqdiv = div;
}

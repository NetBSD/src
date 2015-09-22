/* $NetBSD: imxuart.c,v 1.16.2.2 2015/09/22 12:05:37 skrll Exp $ */

/*
 * Copyright (c) 2009, 2010  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
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
 * driver for UART in i.MX SoC.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imxuart.c,v 1.16.2.2 2015/09/22 12:05:37 skrll Exp $");

#include "opt_imxuart.h"
#include "opt_ddb.h"
#include "opt_ddbparam.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_ntp.h"
#include "opt_imxuart.h"
#include "opt_imx.h"

#ifdef RND_COM
#include <sys/rndsource.h>
#endif

#ifndef	IMXUART_TOLERANCE
#define	IMXUART_TOLERANCE	30	/* baud rate tolerance, in 0.1% units */
#endif

#ifndef	IMXUART_FREQDIV
#define	IMXUART_FREQDIV		2	/* XXX */
#endif

#ifndef	IMXUART_FREQ
#define	IMXUART_FREQ	(56900000)
#endif

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

#include <sys/bus.h>

#include <arm/imx/imxuartreg.h>
#include <arm/imx/imxuartvar.h>
#include <dev/cons.h>

#ifndef	IMXUART_RING_SIZE
#define	IMXUART_RING_SIZE	2048
#endif

typedef struct imxuart_softc {
	device_t	sc_dev;

	struct imxuart_regs {
		bus_space_tag_t		ur_iot;
		bus_space_handle_t	ur_ioh;
		bus_addr_t		ur_iobase;
#if 0
		bus_size_t		ur_nports;
		bus_size_t		ur_map[16];
#endif
	} sc_regs;

#define	sc_bt	sc_regs.ur_iot
#define	sc_bh	sc_regs.ur_ioh

	uint32_t		sc_intrspec_enb;
	uint32_t	sc_ucr2_d;	/* target value for UCR2 */
	uint32_t	sc_ucr[4];	/* cached value of UCRn */
#define	sc_ucr1	sc_ucr[0]
#define	sc_ucr2	sc_ucr[1]
#define	sc_ucr3	sc_ucr[2]
#define	sc_ucr4	sc_ucr[3]

	uint			sc_init_cnt;

	bus_addr_t		sc_addr;
	bus_size_t		sc_size;
	int			sc_intr;

	u_char	sc_hwflags;
/* Hardware flag masks */
#define	IMXUART_HW_FLOW 	__BIT(0)
#define	IMXUART_HW_DEV_OK	__BIT(1)
#define	IMXUART_HW_CONSOLE	__BIT(2)
#define	IMXUART_HW_KGDB 	__BIT(3)


	bool	enabled;

	u_char	sc_swflags;

	u_char sc_rx_flags;
#define	IMXUART_RX_TTY_BLOCKED  	__BIT(0)
#define	IMXUART_RX_TTY_OVERFLOWED	__BIT(1)
#define	IMXUART_RX_IBUF_BLOCKED 	__BIT(2)
#define	IMXUART_RX_IBUF_OVERFLOWED	__BIT(3)
#define	IMXUART_RX_ANY_BLOCK					\
	(IMXUART_RX_TTY_BLOCKED|IMXUART_RX_TTY_OVERFLOWED| 	\
	    IMXUART_RX_IBUF_BLOCKED|IMXUART_RX_IBUF_OVERFLOWED)

	bool	sc_tx_busy, sc_tx_done, sc_tx_stopped;
	bool	sc_rx_ready,sc_st_check;
	u_short	sc_txfifo_len, sc_txfifo_thresh;

	uint16_t	*sc_rbuf;
	u_int		sc_rbuf_size;
	u_int		sc_rbuf_in;
	u_int		sc_rbuf_out;
#define	IMXUART_RBUF_AVAIL(sc)					\
	((sc->sc_rbuf_out <= sc->sc_rbuf_in) ?			\
	(sc->sc_rbuf_in - sc->sc_rbuf_out) :			\
	(sc->sc_rbuf_size - (sc->sc_rbuf_out - sc->sc_rbuf_in)))

#define	IMXUART_RBUF_SPACE(sc)	\
	((sc->sc_rbuf_in <= sc->sc_rbuf_out ?			    \
	    sc->sc_rbuf_size - (sc->sc_rbuf_out - sc->sc_rbuf_in) : \
	    sc->sc_rbuf_in - sc->sc_rbuf_out) - 1)
/* increment ringbuffer pointer */
#define	IMXUART_RBUF_INC(sc,v,i)	(((v) + (i))&((sc->sc_rbuf_size)-1))
	u_int	sc_r_lowat;
	u_int	sc_r_hiwat;

	/* output chunk */
 	u_char *sc_tba;
 	u_int sc_tbc;
	u_int sc_heldtbc;
	/* pending parameter changes */
	u_char	sc_pending;
#define	IMXUART_PEND_PARAM	__BIT(0)
#define	IMXUART_PEND_SPEED	__BIT(1)


	struct callout sc_diag_callout;
	kmutex_t sc_lock;
	void *sc_ih;		/* interrupt handler */
	void *sc_si;		/* soft interrupt */
	struct tty		*sc_tty;

	/* power management hooks */
	int (*enable)(struct imxuart_softc *);
	void (*disable)(struct imxuart_softc *);

	struct {
		ulong err;
		ulong brk;
		ulong prerr;
		ulong frmerr;
		ulong ovrrun;
	}	sc_errors;

	struct imxuart_baudrate_ratio {
		uint16_t numerator;	/* UBIR */
		uint16_t modulator;	/* UBMR */
	} sc_ratio;

} imxuart_softc_t;


int	imxuspeed(long, struct imxuart_baudrate_ratio *);
int	imxuparam(struct tty *, struct termios *);
void	imxustart(struct tty *);
int	imxuhwiflow(struct tty *, int);

void	imxuart_shutdown(struct imxuart_softc *);
void	imxuart_loadchannelregs(struct imxuart_softc *);
void	imxuart_hwiflow(struct imxuart_softc *);
void	imxuart_break(struct imxuart_softc *, bool);
void	imxuart_modem(struct imxuart_softc *, int);
void	tiocm_to_imxu(struct imxuart_softc *, u_long, int);
int	imxuart_to_tiocm(struct imxuart_softc *);
void	imxuart_iflush(struct imxuart_softc *);
int	imxuintr(void *);

int	imxuart_common_getc(dev_t, struct imxuart_regs *);
void	imxuart_common_putc(dev_t, struct imxuart_regs *, int);


int	imxuart_init(struct imxuart_regs *, int, tcflag_t, int);

int	imxucngetc(dev_t);
void	imxucnputc(dev_t, int);
void	imxucnpollc(dev_t, int);

static void imxuintr_read(struct imxuart_softc *);
static void imxuintr_send(struct imxuart_softc *);

static void imxuart_enable_debugport(struct imxuart_softc *);
static void imxuart_disable_all_interrupts(struct imxuart_softc *);
static void imxuart_control_rxint(struct imxuart_softc *, bool);
static void imxuart_control_txint(struct imxuart_softc *, bool);
static u_int imxuart_txfifo_space(struct imxuart_softc *sc);

static	uint32_t	cflag_to_ucr2(tcflag_t, uint32_t);

CFATTACH_DECL_NEW(imxuart, sizeof(struct imxuart_softc),
    imxuart_match, imxuart_attach, NULL, NULL);


#define	integrate	static inline
void 	imxusoft(void *);
integrate void imxuart_rxsoft(struct imxuart_softc *, struct tty *);
integrate void imxuart_txsoft(struct imxuart_softc *, struct tty *);
integrate void imxuart_stsoft(struct imxuart_softc *, struct tty *);
integrate void imxuart_schedrx(struct imxuart_softc *);
void	imxudiag(void *);
static void imxuart_load_speed(struct imxuart_softc *);
static void imxuart_load_params(struct imxuart_softc *);
integrate void imxuart_load_pendings(struct imxuart_softc *);


extern struct cfdriver imxuart_cd;

dev_type_open(imxuopen);
dev_type_close(imxuclose);
dev_type_read(imxuread);
dev_type_write(imxuwrite);
dev_type_ioctl(imxuioctl);
dev_type_stop(imxustop);
dev_type_tty(imxutty);
dev_type_poll(imxupoll);

const struct cdevsw imxcom_cdevsw = {
	.d_open = imxuopen,
	.d_close = imxuclose,
	.d_read = imxuread,
	.d_write = imxuwrite,
	.d_ioctl = imxuioctl,
	.d_stop = imxustop,
	.d_tty = imxutty,
	.d_poll = imxupoll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

/*
 * Make this an option variable one can patch.
 * But be warned:  this must be a power of 2!
 */
u_int imxuart_rbuf_size = IMXUART_RING_SIZE;

/* Stop input when 3/4 of the ring is full; restart when only 1/4 is full. */
u_int imxuart_rbuf_hiwat = (IMXUART_RING_SIZE * 1) / 4;
u_int imxuart_rbuf_lowat = (IMXUART_RING_SIZE * 3) / 4;

static struct imxuart_regs imxuconsregs;
static int imxuconsattached;
static int imxuconsrate;
static tcflag_t imxuconscflag;
static struct cnm_state imxuart_cnm_state;

u_int imxuart_freq = IMXUART_FREQ;
u_int imxuart_freqdiv = IMXUART_FREQDIV;

#ifdef KGDB
#include <sys/kgdb.h>

static struct imxuart_regs imxu_kgdb_regs;
static int imxu_kgdb_attached;

int	imxuart_kgdb_getc(void *);
void	imxuart_kgdb_putc(void *, int);
#endif /* KGDB */

#define	IMXUART_DIALOUT_MASK	TTDIALOUT_MASK

#define	IMXUART_UNIT(x)		TTUNIT(x)
#define	IMXUART_DIALOUT(x)	TTDIALOUT(x)

#define	IMXUART_ISALIVE(sc)	((sc)->enabled != 0 && \
			 device_is_active((sc)->sc_dev))

#define	BR	BUS_SPACE_BARRIER_READ
#define	BW	BUS_SPACE_BARRIER_WRITE
#define	IMXUART_BARRIER(r, f) \
	bus_space_barrier((r)->ur_iot, (r)->ur_ioh, 0, IMX_UART_SIZE, (f))


void
imxuart_attach_common(device_t parent, device_t self,
    bus_space_tag_t iot, paddr_t iobase, size_t size, int intr, int flags)
{
	imxuart_softc_t *sc = device_private(self);
	struct imxuart_regs *regsp = &sc->sc_regs;
	struct tty *tp;
	bus_space_handle_t ioh;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;

	if (size <= 0)
		size = IMX_UART_SIZE;

	sc->sc_intr = intr;
	regsp->ur_iot = iot;
	regsp->ur_iobase = iobase;

	if (bus_space_map(iot, regsp->ur_iobase, size, 0, &ioh)) {
		return;
	}
	regsp->ur_ioh = ioh;

	callout_init(&sc->sc_diag_callout, 0);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_HIGH);

	if (regsp->ur_iobase != imxuconsregs.ur_iobase)
		imxuart_init(&sc->sc_regs, TTYDEF_SPEED, TTYDEF_CFLAG, false);

	bus_space_read_region_4(iot, ioh, IMX_UCR1, sc->sc_ucr, 4);
	sc->sc_ucr2_d = sc->sc_ucr2;

	/* Disable interrupts before configuring the device. */
	imxuart_disable_all_interrupts(sc);

	if (regsp->ur_iobase == imxuconsregs.ur_iobase) {
		imxuconsattached = 1;

		/* Make sure the console is always "hardwired". */
#if 0
		delay(10000);			/* wait for output to finish */
#endif
		SET(sc->sc_hwflags, IMXUART_HW_CONSOLE);
		SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);
	}


	tp = tty_alloc();
	tp->t_oproc = imxustart;
	tp->t_param = imxuparam;
	tp->t_hwiflow = imxuhwiflow;

	sc->sc_tty = tp;
	sc->sc_rbuf = malloc(sizeof (*sc->sc_rbuf) * imxuart_rbuf_size,
	    M_DEVBUF, M_NOWAIT);
	sc->sc_rbuf_size = imxuart_rbuf_size;
	sc->sc_rbuf_in = sc->sc_rbuf_out = 0;
	if (sc->sc_rbuf == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "unable to allocate ring buffer\n");
		return;
	}

	sc->sc_txfifo_len = 32;
	sc->sc_txfifo_thresh = 16;	/* when USR1.TRDY, fifo has space
					 * for this many characters */

	tty_attach(tp);

	if (ISSET(sc->sc_hwflags, IMXUART_HW_CONSOLE)) {
		int maj;

		/* locate the major number */
		maj = cdevsw_lookup_major(&imxcom_cdevsw);

		if (maj != NODEVMAJOR) {
			tp->t_dev = cn_tab->cn_dev = makedev(maj,
			    device_unit(sc->sc_dev));

			aprint_normal_dev(sc->sc_dev, "console\n");
		}
	}

	sc->sc_ih = intr_establish(sc->sc_intr, IPL_SERIAL, IST_LEVEL,
	    imxuintr, sc);
	if (sc->sc_ih == NULL)
		aprint_error_dev(sc->sc_dev, "intr_establish failed\n");

#ifdef KGDB
	/*
	 * Allow kgdb to "take over" this port.  If this is
	 * not the console and is the kgdb device, it has
	 * exclusive use.  If it's the console _and_ the
	 * kgdb device, it doesn't.
	 */
	if (regsp->ur_iobase == imxu_kgdb_regs.ur_iobase) {
		if (!ISSET(sc->sc_hwflags, IMXUART_HW_CONSOLE)) {
			imxu_kgdb_attached = 1;

			SET(sc->sc_hwflags, IMXUART_HW_KGDB);
		}
		aprint_normal_dev(sc->sc_dev, "kgdb\n");
	}
#endif

	sc->sc_si = softint_establish(SOFTINT_SERIAL, imxusoft, sc);

#ifdef RND_COM
	rnd_attach_source(&sc->rnd_source, device_xname(sc->sc_dev),
			  RND_TYPE_TTY, RND_FLAG_COLLECT_TIME |
					RND_FLAG_ESTIMATE_TIME);
#endif

	/* if there are no enable/disable functions, assume the device
	   is always enabled */
	if (!sc->enable)
		sc->enabled = 1;

	imxuart_enable_debugport(sc);

	SET(sc->sc_hwflags, IMXUART_HW_DEV_OK);

	//shutdownhook_establish(imxuart_shutdownhook, sc);


#if 0
	{
		uint32_t reg;
		reg = bus_space_read_4(iot, ioh, IMX_UCR1);
		reg |= IMX_UCR1_TXDMAEN | IMX_UCR1_RXDMAEN;
		bus_space_write_4(iot, ioh, IMX_UCR1, reg);
	}
#endif
}

/*
 * baudrate = RefFreq / (16 * (UMBR + 1)/(UBIR + 1))
 *
 * (UBIR + 1) / (UBMR + 1) = (16 * BaurdRate) / RefFreq
 */

static long
gcd(long m, long n)
{

	if (m < n)
		return gcd(n, m);

	if (n <= 0)
		return m;
	return gcd(n, m % n);
}


int
imxuspeed(long speed, struct imxuart_baudrate_ratio *ratio)
{
#define	divrnd(n, q)	(((n)*2/(q)+1)/2)	/* divide and round off */
	long b = 16 * speed;
	long f = imxuart_freq / imxuart_freqdiv;
	long d;
	int err = 0;

	/* reduce b/f */
	while ((f > (1<<16) || b > (1<<16)) && (d = gcd(f, b)) > 1) {
		f /= d;
		b /= d;
	}


	while (f > (1<<16) || b > (1<<16)) {
		f /= 2;
		b /= 2;
	}
	if (f <= 0 || b <= 0)
		return -1;

#ifdef	DIAGNOSTIC
	err = divrnd(((uint64_t)imxuart_freq) * 1000 / imxuart_freqdiv,
		     (uint64_t)speed * 16 * f / b) - 1000;
	if (err < 0)
		err = -err;
#endif

	ratio->numerator = b-1;
	ratio->modulator = f-1;

	if (err > IMXUART_TOLERANCE)
		return -1;

	return 0;
#undef	divrnd
}

#ifdef IMXUART_DEBUG
int	imxuart_debug = 0;

void imxustatus(struct imxuart_softc *, const char *);
void
imxustatus(struct imxuart_softc *sc, const char *str)
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
imxuart_detach(device_t self, int flags)
{
	struct imxuart_softc *sc = device_private(self);
	int maj, mn;

        if (ISSET(sc->sc_hwflags, IMXUART_HW_CONSOLE))
		return EBUSY;

	/* locate the major number */
	maj = cdevsw_lookup_major(&imxcom_cdevsw);

	/* Nuke the vnodes for any open instances. */
	mn = device_unit(self);
	vdevgone(maj, mn, mn, VCHR);

	mn |= IMXUART_DIALOUT_MASK;
	vdevgone(maj, mn, mn, VCHR);

	if (sc->sc_rbuf == NULL) {
		/*
		 * Ring buffer allocation failed in the imxuart_attach_subr,
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
imxuart_activate(device_t self, enum devact act)
{
	struct imxuart_softc *sc = device_private(self);
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		rv = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		if (sc->sc_hwflags & (IMXUART_HW_CONSOLE|IMXUART_HW_KGDB)) {
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
imxuart_shutdown(struct imxuart_softc *sc)
{
	struct tty *tp = sc->sc_tty;

	mutex_spin_enter(&sc->sc_lock);

	/* If we were asserting flow control, then deassert it. */
	SET(sc->sc_rx_flags, IMXUART_RX_IBUF_BLOCKED);
	imxuart_hwiflow(sc);

	/* Clear any break condition set with TIOCSBRK. */
	imxuart_break(sc, false);

	/*
	 * Hang up if necessary.  Wait a bit, so the other side has time to
	 * notice even if we immediately open the port again.
	 * Avoid tsleeping above splhigh().
	 */
	if (ISSET(tp->t_cflag, HUPCL)) {
		imxuart_modem(sc, 0);
		mutex_spin_exit(&sc->sc_lock);
		/* XXX will only timeout */
		(void) kpause(ttclos, false, hz, NULL);
		mutex_spin_enter(&sc->sc_lock);
	}

	/* Turn off interrupts. */
	imxuart_disable_all_interrupts(sc);
	/* re-enable recv interrupt for console or kgdb port */
	imxuart_enable_debugport(sc);

	mutex_spin_exit(&sc->sc_lock);

#ifdef	notyet
	if (sc->disable) {
#ifdef DIAGNOSTIC
		if (!sc->enabled)
			panic("imxuart_shutdown: not enabled?");
#endif
		(*sc->disable)(sc);
		sc->enabled = 0;
	}
#endif
}

int
imxuopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct imxuart_softc *sc;
	struct tty *tp;
	int s;
	int error;

	sc = device_lookup_private(&imxuart_cd, IMXUART_UNIT(dev));
	if (sc == NULL || !ISSET(sc->sc_hwflags, IMXUART_HW_DEV_OK) ||
		sc->sc_rbuf == NULL)
		return (ENXIO);

	if (!device_is_active(sc->sc_dev))
		return (ENXIO);

#ifdef KGDB
	/*
	 * If this is the kgdb port, no other use is permitted.
	 */
	if (ISSET(sc->sc_hwflags, IMXUART_HW_KGDB))
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

		imxuart_disable_all_interrupts(sc);

		/* Fetch the current modem control status, needed later. */

#ifdef	IMXUART_PPS
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
		if (ISSET(sc->sc_hwflags, IMXUART_HW_CONSOLE)) {
			t.c_ospeed = imxuconsrate;
			t.c_cflag = imxuconscflag;
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
		/* Make sure imxuparam() will do something. */
		tp->t_ospeed = 0;
		(void) imxuparam(tp, &t);
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
		imxuart_modem(sc, 1);

		/* Clear the input ring, and unblock. */
		sc->sc_rbuf_in = sc->sc_rbuf_out = 0;
		imxuart_iflush(sc);
		CLR(sc->sc_rx_flags, IMXUART_RX_ANY_BLOCK);
		imxuart_hwiflow(sc);

		/* Turn on interrupts. */
		imxuart_control_rxint(sc, true);

#ifdef IMXUART_DEBUG
		if (imxuart_debug)
			imxustatus(sc, "imxuopen  ");
#endif

		mutex_spin_exit(&sc->sc_lock);
	}

	splx(s);

#if 0
	error = ttyopen(tp, IMXUART_DIALOUT(dev), ISSET(flag, O_NONBLOCK));
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
		imxuart_shutdown(sc);
	}

	return (error);
}

int
imxuclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct imxuart_softc *sc =
	    device_lookup_private(&imxuart_cd, IMXUART_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	/* XXX This is for cons.c. */
	if (!ISSET(tp->t_state, TS_ISOPEN))
		return (0);

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);

	if (IMXUART_ISALIVE(sc) == 0)
		return (0);

	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * Although we got a last close, the device may still be in
		 * use; e.g. if this was the dialout node, and there are still
		 * processes waiting for carrier on the non-dialout node.
		 */
		imxuart_shutdown(sc);
	}

	return (0);
}

int
imxuread(dev_t dev, struct uio *uio, int flag)
{
	struct imxuart_softc *sc =
	    device_lookup_private(&imxuart_cd, IMXUART_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (IMXUART_ISALIVE(sc) == 0)
		return (EIO);

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
imxuwrite(dev_t dev, struct uio *uio, int flag)
{
	struct imxuart_softc *sc =
	    device_lookup_private(&imxuart_cd, IMXUART_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (IMXUART_ISALIVE(sc) == 0)
		return (EIO);

	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
imxupoll(dev_t dev, int events, struct lwp *l)
{
	struct imxuart_softc *sc =
	    device_lookup_private(&imxuart_cd, IMXUART_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (IMXUART_ISALIVE(sc) == 0)
		return (POLLHUP);

	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

struct tty *
imxutty(dev_t dev)
{
	struct imxuart_softc *sc =
	    device_lookup_private(&imxuart_cd, IMXUART_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	return (tp);
}

int
imxuioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct imxuart_softc *sc;
	struct tty *tp;
	int error;

	sc = device_lookup_private(&imxuart_cd, IMXUART_UNIT(dev));
	if (sc == NULL)
		return ENXIO;
	if (IMXUART_ISALIVE(sc) == 0)
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
		imxuart_break(sc, true);
		break;

	case TIOCCBRK:
		imxuart_break(sc, false);
		break;

	case TIOCSDTR:
		imxuart_modem(sc, 1);
		break;

	case TIOCCDTR:
		imxuart_modem(sc, 0);
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
		tiocm_to_imxu(sc, cmd, *(int *)data);
		break;

	case TIOCMGET:
		*(int *)data = imxuart_to_tiocm(sc);
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

#ifdef IMXUART_DEBUG
	if (imxuart_debug)
		imxustatus(sc, "imxuioctl ");
#endif

	return (error);
}

integrate void
imxuart_schedrx(struct imxuart_softc *sc)
{
	sc->sc_rx_ready = 1;

	/* Wake up the poller. */
	softint_schedule(sc->sc_si);
}

void
imxuart_break(struct imxuart_softc *sc, bool onoff)
{
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;

	if (onoff)
		SET(sc->sc_ucr1, IMX_UCR1_SNDBRK);
	else
		CLR(sc->sc_ucr1, IMX_UCR1_SNDBRK);

	bus_space_write_4(iot, ioh, IMX_UCR1, sc->sc_ucr1);
}

void
imxuart_modem(struct imxuart_softc *sc, int onoff)
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
			imxuart_loadchannelregs(sc);
	}
#endif
}

/*
 * RTS output is controlled by UCR2.CTS bit.
 * DTR output is controlled by UCR3.DSR bit.
 * (i.MX reference manual uses names in DCE mode)
 *
 * note: if UCR2.CTSC == 1 for automatic HW flow control, UCR2.CTS is ignored.
 */
void
tiocm_to_imxu(struct imxuart_softc *sc, u_long how, int ttybits)
{
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;

	uint32_t ucr2 = sc->sc_ucr2_d;
	uint32_t ucr3 = sc->sc_ucr3;

	uint32_t ucr2_mask = 0;
	uint32_t ucr3_mask = 0;


	if (ISSET(ttybits, TIOCM_DTR))
		ucr3_mask = IMX_UCR3_DSR;
	if (ISSET(ttybits, TIOCM_RTS))
		ucr2_mask = IMX_UCR2_CTS;

	switch (how) {
	case TIOCMBIC:
		CLR(ucr2, ucr2_mask);
		CLR(ucr3, ucr3_mask);
		break;

	case TIOCMBIS:
		SET(ucr2, ucr2_mask);
		SET(ucr3, ucr3_mask);
		break;

	case TIOCMSET:
		CLR(ucr2, ucr2_mask);
		CLR(ucr3, ucr3_mask);
		SET(ucr2, ucr2_mask);
		SET(ucr3, ucr3_mask);
		break;
	}

	if (ucr3 != sc->sc_ucr3) {
		bus_space_write_4(iot, ioh, IMX_UCR3, ucr3);
		sc->sc_ucr3 = ucr3;
	}

	if (ucr2 == sc->sc_ucr2_d)
		return;

	sc->sc_ucr2_d = ucr2;
	/* update CTS bit only */
	ucr2 = (sc->sc_ucr2 & ~IMX_UCR2_CTS) |
	    (ucr2 & IMX_UCR2_CTS);

	bus_space_write_4(iot, ioh, IMX_UCR2, ucr2);
	sc->sc_ucr2 = ucr2;
}

int
imxuart_to_tiocm(struct imxuart_softc *sc)
{
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;
	int ttybits = 0;
	uint32_t usr[2];

	if (ISSET(sc->sc_ucr3, IMX_UCR3_DSR))
		SET(ttybits, TIOCM_DTR);
	if (ISSET(sc->sc_ucr2, IMX_UCR2_CTS))
		SET(ttybits, TIOCM_RTS);

	bus_space_read_region_4(iot, ioh, IMX_USR1, usr, 2);

	if (ISSET(usr[0], IMX_USR1_RTSS))
		SET(ttybits, TIOCM_CTS);

	if (ISSET(usr[1], IMX_USR2_DCDIN))
		SET(ttybits, TIOCM_CD);

#if 0
	/* XXXbsh: I couldn't find the way to read ipp_uart_dsr_dte_i signal,
	   although there are bits in UART registers to detect delta of DSR.
	*/
	if (ISSET(imxubits, MSR_DSR))
		SET(ttybits, TIOCM_DSR);
#endif

	if (ISSET(usr[1], IMX_USR2_RIIN))
		SET(ttybits, TIOCM_RI);


#ifdef	notyet
	if (ISSET(sc->sc_ier, IER_ERXRDY | IER_ETXRDY | IER_ERLS | IER_EMSC))
		SET(ttybits, TIOCM_LE);
#endif

	return (ttybits);
}

static uint32_t
cflag_to_ucr2(tcflag_t cflag, uint32_t oldval)
{
	uint32_t val = oldval;

	CLR(val,IMX_UCR2_WS|IMX_UCR2_PREN|IMX_UCR2_PROE|IMX_UCR2_STPB);

	switch (cflag & CSIZE) {
	case CS5:
	case CS6:
		/* not suppreted. use 7-bits */
	case CS7:
		break;
	case CS8:
		SET(val, IMX_UCR2_WS);
		break;
	}


	if (ISSET(cflag, PARENB)) {
		SET(val, IMX_UCR2_PREN);

		/* odd parity */
		if (!ISSET(cflag, PARODD))
			SET(val, IMX_UCR2_PROE);
	}

	if (ISSET(cflag, CSTOPB))
		SET(val, IMX_UCR2_STPB);

	val |= IMX_UCR2_TXEN| IMX_UCR2_RXEN|IMX_UCR2_SRST;

	return val;
}

int
imxuparam(struct tty *tp, struct termios *t)
{
	struct imxuart_softc *sc =
	    device_lookup_private(&imxuart_cd, IMXUART_UNIT(tp->t_dev));
	struct imxuart_baudrate_ratio ratio;
	uint32_t ucr2;
	bool change_speed = tp->t_ospeed != t->c_ospeed;

	if (IMXUART_ISALIVE(sc) == 0)
		return (EIO);

	/* Check requested parameters. */
	if (t->c_ispeed && t->c_ispeed != t->c_ospeed)
		return (EINVAL);

	/*
	 * For the console, always force CLOCAL and !HUPCL, so that the port
	 * is always active.
	 */
	if (ISSET(sc->sc_swflags, TIOCFLAG_SOFTCAR) ||
	    ISSET(sc->sc_hwflags, IMXUART_HW_CONSOLE)) {
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
		if (imxuspeed(t->c_ospeed, &ratio) < 0)
			return (EINVAL);
		sc->sc_ratio = ratio;
	}

	ucr2 = cflag_to_ucr2(t->c_cflag, sc->sc_ucr2_d);

	mutex_spin_enter(&sc->sc_lock);

#if 0	/* flow control stuff.  not yet */
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

	if (!change_speed && ucr2 == sc->sc_ucr2_d) {
		/* noop */
	}
	else if (!sc->sc_pending && !sc->sc_tx_busy) {
		if (ucr2 != sc->sc_ucr2_d) {
			sc->sc_ucr2_d = ucr2;
			imxuart_load_params(sc);
		}
		if (change_speed)
			imxuart_load_speed(sc);
	}
	else {
		if (!sc->sc_pending) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
		}
		sc->sc_pending |=
		    (ucr2 == sc->sc_ucr2_d ? 0 : IMXUART_PEND_PARAM) |
		    (change_speed ? 0 : IMXUART_PEND_SPEED);
		sc->sc_ucr2_d = ucr2;
	}

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		/* Disable the high water mark. */
		sc->sc_r_hiwat = 0;
		sc->sc_r_lowat = 0;
		if (ISSET(sc->sc_rx_flags, IMXUART_RX_TTY_OVERFLOWED)) {
			CLR(sc->sc_rx_flags, IMXUART_RX_TTY_OVERFLOWED);
			imxuart_schedrx(sc);
		}
		if (ISSET(sc->sc_rx_flags,
			IMXUART_RX_TTY_BLOCKED|IMXUART_RX_IBUF_BLOCKED)) {
			CLR(sc->sc_rx_flags,
			    IMXUART_RX_TTY_BLOCKED|IMXUART_RX_IBUF_BLOCKED);
			imxuart_hwiflow(sc);
		}
	} else {
		sc->sc_r_hiwat = imxuart_rbuf_hiwat;
		sc->sc_r_lowat = imxuart_rbuf_lowat;
	}

	mutex_spin_exit(&sc->sc_lock);

#if 0
	/*
	 * Update the tty layer's idea of the carrier bit, in case we changed
	 * CLOCAL or MDMBUF.  We don't hang up here; we only do that by
	 * explicit request.
	 */
	(void) (*tp->t_linesw->l_modem)(tp, ISSET(sc->sc_msr, MSR_DCD));
#else
	/* XXX: always report that we have DCD */
	(void) (*tp->t_linesw->l_modem)(tp, 1);
#endif

#ifdef IMXUART_DEBUG
	if (imxuart_debug)
		imxustatus(sc, "imxuparam ");
#endif

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		if (sc->sc_tx_stopped) {
			sc->sc_tx_stopped = 0;
			imxustart(tp);
		}
	}

	return (0);
}

void
imxuart_iflush(struct imxuart_softc *sc)
{
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;
#ifdef DIAGNOSTIC
	uint32_t reg = 0xffff;
#endif
	int timo;

	timo = 50000;
	/* flush any pending I/O */
	while (ISSET(bus_space_read_4(iot, ioh, IMX_USR2), IMX_USR2_RDR)
	    && --timo)
#ifdef DIAGNOSTIC
		reg =
#else
		    (void)
#endif
		    bus_space_read_4(iot, ioh, IMX_URXD);
#ifdef DIAGNOSTIC
	if (!timo)
		aprint_error_dev(sc->sc_dev, "imxuart_iflush timeout %02x\n", reg);
#endif
}

int
imxuhwiflow(struct tty *tp, int block)
{
	struct imxuart_softc *sc =
	    device_lookup_private(&imxuart_cd, IMXUART_UNIT(tp->t_dev));

	if (IMXUART_ISALIVE(sc) == 0)
		return (0);

#ifdef notyet
	if (sc->sc_mcr_rts == 0)
		return (0);
#endif

	mutex_spin_enter(&sc->sc_lock);

	if (block) {
		if (!ISSET(sc->sc_rx_flags, IMXUART_RX_TTY_BLOCKED)) {
			SET(sc->sc_rx_flags, IMXUART_RX_TTY_BLOCKED);
			imxuart_hwiflow(sc);
		}
	} else {
		if (ISSET(sc->sc_rx_flags, IMXUART_RX_TTY_OVERFLOWED)) {
			CLR(sc->sc_rx_flags, IMXUART_RX_TTY_OVERFLOWED);
			imxuart_schedrx(sc);
		}
		if (ISSET(sc->sc_rx_flags, IMXUART_RX_TTY_BLOCKED)) {
			CLR(sc->sc_rx_flags, IMXUART_RX_TTY_BLOCKED);
			imxuart_hwiflow(sc);
		}
	}

	mutex_spin_exit(&sc->sc_lock);
	return (1);
}

/*
 * (un)block input via hw flowcontrol
 */
void
imxuart_hwiflow(struct imxuart_softc *sc)
{
#ifdef notyet
	struct imxuart_regs *regsp= &sc->sc_regs;

	if (sc->sc_mcr_rts == 0)
		return;

	if (ISSET(sc->sc_rx_flags, RX_ANY_BLOCK)) {
		CLR(sc->sc_mcr, sc->sc_mcr_rts);
		CLR(sc->sc_mcr_active, sc->sc_mcr_rts);
	} else {
		SET(sc->sc_mcr, sc->sc_mcr_rts);
		SET(sc->sc_mcr_active, sc->sc_mcr_rts);
	}
	UR_WRITE_1(regsp, IMXUART_REG_MCR, sc->sc_mcr_active);
#endif
}


void
imxustart(struct tty *tp)
{
	struct imxuart_softc *sc =
	    device_lookup_private(&imxuart_cd, IMXUART_UNIT(tp->t_dev));
	int s;
	u_char *tba;
	int tbc;
	u_int n;
	u_int space;
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;

	if (IMXUART_ISALIVE(sc) == 0)
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

	space = imxuart_txfifo_space(sc);
	n = MIN(sc->sc_tbc, space);

	bus_space_write_multi_1(iot, ioh, IMX_UTXD, sc->sc_tba, n);
	sc->sc_tbc -= n;
	sc->sc_tba += n;

	/* Enable transmit completion interrupts */
	imxuart_control_txint(sc, true);

	mutex_spin_exit(&sc->sc_lock);
out:
	splx(s);
	return;
}

/*
 * Stop output on a line.
 */
void
imxustop(struct tty *tp, int flag)
{
	struct imxuart_softc *sc =
	    device_lookup_private(&imxuart_cd, IMXUART_UNIT(tp->t_dev));

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
imxudiag(void *arg)
{
#ifdef notyet
	struct imxuart_softc *sc = arg;
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
imxuart_rxsoft(struct imxuart_softc *sc, struct tty *tp)
{
	int (*rint)(int, struct tty *) = tp->t_linesw->l_rint;
	u_int cc, scc, outp;
	uint16_t data;
	u_int code;

	scc = cc = IMXUART_RBUF_AVAIL(sc);

#if 0
	if (cc == imxuart_rbuf_size-1) {
		sc->sc_floods++;
		if (sc->sc_errors++ == 0)
			callout_reset(&sc->sc_diag_callout, 60 * hz,
			    imxudiag, sc);
	}
#endif

	/* If not yet open, drop the entire buffer content here */
	if (!ISSET(tp->t_state, TS_ISOPEN)) {
		sc->sc_rbuf_out = sc->sc_rbuf_in;
		cc = 0;
	}

	outp = sc->sc_rbuf_out;

#define	ERRBITS (IMX_URXD_PRERR|IMX_URXD_BRK|IMX_URXD_FRMERR|IMX_URXD_OVRRUN)

	while (cc) {
	        data = sc->sc_rbuf[outp];
		code = data & IMX_URXD_RX_DATA;
		if (ISSET(data, ERRBITS)) {
			if (sc->sc_errors.err == 0)
				callout_reset(&sc->sc_diag_callout,
				    60 * hz, imxudiag, sc);
			if (ISSET(data, IMX_URXD_OVRRUN))
				sc->sc_errors.ovrrun++;
			if (ISSET(data, IMX_URXD_BRK)) {
				sc->sc_errors.brk++;
				SET(code, TTY_FE);
			}
			if (ISSET(data, IMX_URXD_FRMERR)) {
				sc->sc_errors.frmerr++;
				SET(code, TTY_FE);
			}
			if (ISSET(data, IMX_URXD_PRERR)) {
				sc->sc_errors.prerr++;
				SET(code, TTY_PE);
			}
		}
		if ((*rint)(code, tp) == -1) {
			/*
			 * The line discipline's buffer is out of space.
			 */
			if (!ISSET(sc->sc_rx_flags, IMXUART_RX_TTY_BLOCKED)) {
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
				 * space available (through imxuhwiflow()).
				 * Leave the rest of the data in the input
				 * buffer.
				 */
				SET(sc->sc_rx_flags, IMXUART_RX_TTY_OVERFLOWED);
			}
			break;
		}
		outp = IMXUART_RBUF_INC(sc, outp, 1);
		cc--;
	}

	if (cc != scc) {
		sc->sc_rbuf_out = outp;
		mutex_spin_enter(&sc->sc_lock);

		cc = IMXUART_RBUF_SPACE(sc);

		/* Buffers should be ok again, release possible block. */
		if (cc >= sc->sc_r_lowat) {
			if (ISSET(sc->sc_rx_flags, IMXUART_RX_IBUF_OVERFLOWED)) {
				CLR(sc->sc_rx_flags, IMXUART_RX_IBUF_OVERFLOWED);
				imxuart_control_rxint(sc, true);
			}
			if (ISSET(sc->sc_rx_flags, IMXUART_RX_IBUF_BLOCKED)) {
				CLR(sc->sc_rx_flags, IMXUART_RX_IBUF_BLOCKED);
				imxuart_hwiflow(sc);
			}
		}
		mutex_spin_exit(&sc->sc_lock);
	}
}

integrate void
imxuart_txsoft(struct imxuart_softc *sc, struct tty *tp)
{

	CLR(tp->t_state, TS_BUSY);
	if (ISSET(tp->t_state, TS_FLUSH))
		CLR(tp->t_state, TS_FLUSH);
	else
		ndflush(&tp->t_outq, (int)(sc->sc_tba - tp->t_outq.c_cf));
	(*tp->t_linesw->l_start)(tp);
}

integrate void
imxuart_stsoft(struct imxuart_softc *sc, struct tty *tp)
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
#ifdef IMXUART_DEBUG
	if (imxuart_debug)
		imxustatus(sc, "imxuart_stsoft");
#endif
}

void
imxusoft(void *arg)
{
	struct imxuart_softc *sc = arg;
	struct tty *tp;

	if (IMXUART_ISALIVE(sc) == 0)
		return;

	tp = sc->sc_tty;

	if (sc->sc_rx_ready) {
		sc->sc_rx_ready = 0;
		imxuart_rxsoft(sc, tp);
	}

	if (sc->sc_st_check) {
		sc->sc_st_check = 0;
		imxuart_stsoft(sc, tp);
	}

	if (sc->sc_tx_done) {
		sc->sc_tx_done = 0;
		imxuart_txsoft(sc, tp);
	}
}

int
imxuintr(void *arg)
{
	struct imxuart_softc *sc = arg;
	uint32_t usr1, usr2;
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;


	if (IMXUART_ISALIVE(sc) == 0)
		return (0);

	mutex_spin_enter(&sc->sc_lock);

	usr2 = bus_space_read_4(iot, ioh, IMX_USR2);


	do {
		bus_space_write_4(iot, ioh, IMX_USR2,
		    usr2 & (IMX_USR2_BRCD|IMX_USR2_ORE));
		if (usr2 & IMX_USR2_BRCD) {
			/* Break signal detected */
			int cn_trapped = 0;

			cn_check_magic(sc->sc_tty->t_dev,
				       CNC_BREAK, imxuart_cnm_state);
			if (cn_trapped)
				continue;
#if defined(KGDB) && !defined(DDB)
			if (ISSET(sc->sc_hwflags, IMXUART_HW_KGDB)) {
				kgdb_connect(1);
				continue;
			}
#endif
		}

		if (usr2 & IMX_USR2_RDR)
			imxuintr_read(sc);

#ifdef	IMXUART_PPS
		{
			u_char	msr, delta;

			msr = CSR_READ_1(regsp, IMXUART_REG_MSR);
			delta = msr ^ sc->sc_msr;
			sc->sc_msr = msr;
			if ((sc->sc_pps_state.ppsparam.mode & PPS_CAPTUREBOTH) &&
			    (delta & MSR_DCD)) {
				mutex_spin_enter(&timecounter_lock);
				pps_capture(&sc->sc_pps_state);
				pps_event(&sc->sc_pps_state,
				    (msr & MSR_DCD) ?
				    PPS_CAPTUREASSERT :
				    PPS_CAPTURECLEAR);
				mutex_spin_exit(&timecounter_lock);
			}
		}
#endif

#ifdef notyet
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
#ifdef IMXUART_DEBUG
				if (imxuart_debug)
					imxustatus(sc, "imxuintr  ");
#endif
			}

			sc->sc_st_check = 1;
		}
#endif

		usr2 = bus_space_read_4(iot, ioh, IMX_USR2);
	} while (usr2 & (IMX_USR2_RDR|IMX_USR2_BRCD));

	usr1 = bus_space_read_4(iot, ioh, IMX_USR1);
	if (usr1 & IMX_USR1_TRDY)
		imxuintr_send(sc);

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
imxuintr_read(struct imxuart_softc *sc)
{
	int cc;
	uint16_t rd;
	uint32_t usr2;
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;

	cc = IMXUART_RBUF_SPACE(sc);

	/* clear aging timer interrupt */
	bus_space_write_4(iot, ioh, IMX_USR1, IMX_USR1_AGTIM);

	while (cc > 0) {
		int cn_trapped = 0;


		sc->sc_rbuf[sc->sc_rbuf_in] = rd =
		    bus_space_read_4(iot, ioh, IMX_URXD);

		cn_check_magic(sc->sc_tty->t_dev,
		    rd & 0xff, imxuart_cnm_state);

		if (!cn_trapped) {
#if defined(DDB) && defined(DDB_KEYCODE)
			/*
			 * Temporary hack so that I can force the kernel into
			 * the debugger via the serial port
			 */
			if ((rd & 0xff) == DDB_KEYCODE)
				Debugger();
#endif
			sc->sc_rbuf_in = IMXUART_RBUF_INC(sc, sc->sc_rbuf_in, 1);
			cc--;
		}

		usr2 = bus_space_read_4(iot, ioh, IMX_USR2);
		if (!(usr2 & IMX_USR2_RDR))
			break;
	}

	/*
	 * Current string of incoming characters ended because
	 * no more data was available or we ran out of space.
	 * Schedule a receive event if any data was received.
	 * If we're out of space, turn off receive interrupts.
	 */
	if (!ISSET(sc->sc_rx_flags, IMXUART_RX_TTY_OVERFLOWED))
		sc->sc_rx_ready = 1;
	/*
	 * See if we are in danger of overflowing a buffer. If
	 * so, use hardware flow control to ease the pressure.
	 */
	if (!ISSET(sc->sc_rx_flags, IMXUART_RX_IBUF_BLOCKED) &&
	    cc < sc->sc_r_hiwat) {
		sc->sc_rx_flags |= IMXUART_RX_IBUF_BLOCKED;
		imxuart_hwiflow(sc);
	}

	/*
	 * If we're out of space, disable receive interrupts
	 * until the queue has drained a bit.
	 */
	if (!cc) {
		sc->sc_rx_flags |= IMXUART_RX_IBUF_OVERFLOWED;
		imxuart_control_rxint(sc, false);
	}
}



/*
 * find how many chars we can put into tx-fifo
 */
static u_int
imxuart_txfifo_space(struct imxuart_softc *sc)
{
	uint32_t usr1, usr2;
	u_int cc;
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;

	usr2 = bus_space_read_4(iot, ioh, IMX_USR2);
	if (usr2 & IMX_USR2_TXFE)
		cc = sc->sc_txfifo_len;
	else {
		usr1 = bus_space_read_4(iot, ioh, IMX_USR1);
		if (usr1 & IMX_USR1_TRDY)
			cc = sc->sc_txfifo_thresh;
		else
			cc = 0;
	}

	return cc;
}

void
imxuintr_send(struct imxuart_softc *sc)
{
	uint32_t usr2;
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;
	int cc = 0;

	usr2 = bus_space_read_4(iot, ioh, IMX_USR2);

	if (sc->sc_pending) {
		if (usr2 & IMX_USR2_TXFE) {
			imxuart_load_pendings(sc);
			sc->sc_tbc = sc->sc_heldtbc;
			sc->sc_heldtbc = 0;
		}
		else {
			/* wait for TX fifo empty */
			imxuart_control_txint(sc, true);
			return;
		}
	}

	cc = imxuart_txfifo_space(sc);
	cc = MIN(cc, sc->sc_tbc);

	if (cc > 0) {
		bus_space_write_multi_1(iot, ioh, IMX_UTXD, sc->sc_tba, cc);
		sc->sc_tbc -= cc;
		sc->sc_tba += cc;
	}

	if (sc->sc_tbc > 0)
		imxuart_control_txint(sc, true);
	else {
		/* no more chars to send.
		   we don't need tx interrupt any more. */
		imxuart_control_txint(sc, false);
		if (sc->sc_tx_busy) {
			sc->sc_tx_busy = 0;
			sc->sc_tx_done = 1;
		}
	}
}

static void
imxuart_disable_all_interrupts(struct imxuart_softc *sc)
{
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;

	sc->sc_ucr1 &= ~IMXUART_INTRS_UCR1;
	sc->sc_ucr2 &= ~IMXUART_INTRS_UCR2;
	sc->sc_ucr3 &= ~IMXUART_INTRS_UCR3;
	sc->sc_ucr4 &= ~IMXUART_INTRS_UCR4;


	bus_space_write_region_4(iot, ioh, IMX_UCR1, sc->sc_ucr, 4);
}

static void
imxuart_control_rxint(struct imxuart_softc *sc, bool enable)
{
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;
	uint32_t ucr1, ucr2;

	ucr1 = sc->sc_ucr1;
	ucr2 = sc->sc_ucr2;

	if (enable) {
		ucr1 |= IMX_UCR1_RRDYEN;
		ucr2 |= IMX_UCR2_ATEN;
	}
	else {
		ucr1 &= ~IMX_UCR1_RRDYEN;
		ucr2 &= ~IMX_UCR2_ATEN;
	}

	if (ucr1 != sc->sc_ucr1 || ucr2 != sc->sc_ucr2) {
		sc->sc_ucr1 = ucr1;
		sc->sc_ucr2 = ucr2;
		bus_space_write_region_4(iot, ioh, IMX_UCR1, sc->sc_ucr, 2);
	}
}

static void
imxuart_control_txint(struct imxuart_softc *sc, bool enable)
{
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;
	uint32_t ucr1;
	uint32_t mask;

	/* if parameter change is pending, get interrupt when Tx fifo
	   is completely empty.  otherwise, get interrupt when txfifo
	   has less characters than threshold */
	mask = sc->sc_pending ? IMX_UCR1_TXMPTYEN : IMX_UCR1_TRDYEN;

	ucr1 = sc->sc_ucr1;

	CLR(ucr1, IMX_UCR1_TXMPTYEN|IMX_UCR1_TRDYEN);
	if (enable)
		SET(ucr1, mask);

	if (ucr1 != sc->sc_ucr1) {
		bus_space_write_4(iot, ioh, IMX_UCR1, ucr1);
		sc->sc_ucr1 = ucr1;
	}
}


static void
imxuart_load_params(struct imxuart_softc *sc)
{
	uint32_t ucr2;
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;

	ucr2 = (sc->sc_ucr2_d & ~IMX_UCR2_ATEN) |
	    (sc->sc_ucr2 & IMX_UCR2_ATEN);

	bus_space_write_4(iot, ioh, IMX_UCR2, ucr2);
	sc->sc_ucr2 = ucr2;
}

static void
imxuart_load_speed(struct imxuart_softc *sc)
{
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;
	int n, rfdiv, ufcr;

#ifdef notyet
	/*
	 * Set the FIFO threshold based on the receive speed.
	 *
	 *  * If it's a low speed, it's probably a mouse or some other
	 *    interactive device, so set the threshold low.
	 *  * If it's a high speed, trim the trigger level down to prevent
	 *    overflows.
	 *  * Otherwise set it a bit higher.
	 */
	if (t->c_ospeed <= 1200)
		sc->sc_fifo = FIFO_ENABLE | FIFO_TRIGGER_1;
	else if (t->c_ospeed <= 38400)
		sc->sc_fifo = FIFO_ENABLE | FIFO_TRIGGER_8;
	else
		sc->sc_fifo = FIFO_ENABLE | FIFO_TRIGGER_4;
#endif

	n = 32 - sc->sc_txfifo_thresh;
	n = MAX(2, n);

	rfdiv = IMX_UFCR_DIVIDER_TO_RFDIV(imxuart_freqdiv);

	ufcr = (n << IMX_UFCR_TXTL_SHIFT) |
		(rfdiv << IMX_UFCR_RFDIV_SHIFT) |
		(16 << IMX_UFCR_RXTL_SHIFT);

	/* keep DCE/DTE bit */
	ufcr |= bus_space_read_4(iot, ioh, IMX_UFCR) & IMX_UFCR_DCEDTE;

	bus_space_write_4(iot, ioh, IMX_UFCR, ufcr);

	/* UBIR must updated before UBMR */
	bus_space_write_4(iot, ioh,
	    IMX_UBIR, sc->sc_ratio.numerator);
	bus_space_write_4(iot, ioh,
	    IMX_UBMR, sc->sc_ratio.modulator);


}


static void
imxuart_load_pendings(struct imxuart_softc *sc)
{
	if (sc->sc_pending & IMXUART_PEND_PARAM)
		imxuart_load_params(sc);
	if (sc->sc_pending & IMXUART_PEND_SPEED)
		imxuart_load_speed(sc);
	sc->sc_pending = 0;
}

#if defined(IMXUARTCONSOLE) || defined(KGDB)

/*
 * The following functions are polled getc and putc routines, shared
 * by the console and kgdb glue.
 *
 * The read-ahead code is so that you can detect pending in-band
 * cn_magic in polled mode while doing output rather than having to
 * wait until the kernel decides it needs input.
 */

#define	READAHEAD_RING_LEN	16
static int imxuart_readahead[READAHEAD_RING_LEN];
static int imxuart_readahead_in = 0;
static int imxuart_readahead_out = 0;
#define	READAHEAD_IS_EMPTY()	(imxuart_readahead_in==imxuart_readahead_out)
#define	READAHEAD_IS_FULL()	\
	(((imxuart_readahead_in+1) & (READAHEAD_RING_LEN-1)) ==imxuart_readahead_out)

int
imxuart_common_getc(dev_t dev, struct imxuart_regs *regsp)
{
	int s = splserial();
	u_char c;
	bus_space_tag_t iot = regsp->ur_iot;
	bus_space_handle_t ioh = regsp->ur_ioh;
	uint32_t usr2;

	/* got a character from reading things earlier */
	if (imxuart_readahead_in != imxuart_readahead_out) {

		c = imxuart_readahead[imxuart_readahead_out];
		imxuart_readahead_out = (imxuart_readahead_out + 1) &
		    (READAHEAD_RING_LEN-1);
		splx(s);
		return (c);
	}

	/* block until a character becomes available */
	while (!((usr2 = bus_space_read_4(iot, ioh, IMX_USR2)) & IMX_USR2_RDR))
		;

	c = 0xff & bus_space_read_4(iot, ioh, IMX_URXD);

	{
		int __attribute__((__unused__))cn_trapped = 0; /* unused */
#ifdef DDB
		extern int db_active;
		if (!db_active)
#endif
			cn_check_magic(dev, c, imxuart_cnm_state);
	}
	splx(s);
	return (c);
}

void
imxuart_common_putc(dev_t dev, struct imxuart_regs *regsp, int c)
{
	int s = splserial();
	int cin, timo;
	bus_space_tag_t iot = regsp->ur_iot;
	bus_space_handle_t ioh = regsp->ur_ioh;
	uint32_t usr2;

	if (!READAHEAD_IS_FULL() &&
	    ((usr2 = bus_space_read_4(iot, ioh, IMX_USR2)) & IMX_USR2_RDR)) {

		int __attribute__((__unused__))cn_trapped = 0;
		cin = bus_space_read_4(iot, ioh, IMX_URXD);
		cn_check_magic(dev, cin & 0xff, imxuart_cnm_state);
		imxuart_readahead_in = (imxuart_readahead_in + 1) &
		    (READAHEAD_RING_LEN-1);
	}

	/* wait for any pending transmission to finish */
	timo = 150000;
	do {
		if (bus_space_read_4(iot, ioh, IMX_USR1) & IMX_USR1_TRDY) {
			bus_space_write_4(iot, ioh, IMX_UTXD, c);
			break;
		}
	} while(--timo > 0);

	IMXUART_BARRIER(regsp, BR | BW);

	splx(s);
}
#endif /* defined(IMXUARTCONSOLE) || defined(KGDB) */

/*
 * Initialize UART
 */
int
imxuart_init(struct imxuart_regs *regsp, int rate, tcflag_t cflag, int domap)
{
	struct imxuart_baudrate_ratio ratio;
	int rfdiv = IMX_UFCR_DIVIDER_TO_RFDIV(imxuart_freqdiv);
	uint32_t ufcr;
	int error;

	if (domap && (error = bus_space_map(regsp->ur_iot, regsp->ur_iobase,
	     IMX_UART_SIZE, 0, &regsp->ur_ioh)) != 0)
		return error;

	if (imxuspeed(rate, &ratio) < 0)
		return EINVAL;

	/* UBIR must updated before UBMR */
	bus_space_write_4(regsp->ur_iot, regsp->ur_ioh,
	    IMX_UBIR, ratio.numerator);
	bus_space_write_4(regsp->ur_iot, regsp->ur_ioh,
	    IMX_UBMR, ratio.modulator);


	/* XXX: DTREN, DPEC */
	bus_space_write_4(regsp->ur_iot, regsp->ur_ioh, IMX_UCR3,
	    IMX_UCR3_DSR|IMX_UCR3_RXDMUXSEL);

	ufcr = (8 << IMX_UFCR_TXTL_SHIFT) | (rfdiv << IMX_UFCR_RFDIV_SHIFT) |
		(1 << IMX_UFCR_RXTL_SHIFT);
	/* XXX: keep DCE/DTE bit */
	ufcr |= bus_space_read_4(regsp->ur_iot, regsp->ur_ioh, IMX_UFCR) &
		IMX_UFCR_DCEDTE;

	bus_space_write_4(regsp->ur_iot, regsp->ur_ioh, IMX_UFCR, ufcr);

	bus_space_write_4(regsp->ur_iot, regsp->ur_ioh, IMX_ONEMS,
	    imxuart_freq / imxuart_freqdiv / 1000);

	bus_space_write_4(regsp->ur_iot, regsp->ur_ioh, IMX_UCR2,
			  IMX_UCR2_IRTS|
			  IMX_UCR2_CTSC|
			  IMX_UCR2_WS|IMX_UCR2_TXEN|
			  IMX_UCR2_RXEN|IMX_UCR2_SRST);
	/* clear status registers */
	bus_space_write_4(regsp->ur_iot, regsp->ur_ioh, IMX_USR1, 0xffff);
	bus_space_write_4(regsp->ur_iot, regsp->ur_ioh, IMX_USR2, 0xffff);


	bus_space_write_4(regsp->ur_iot, regsp->ur_ioh, IMX_UCR1,
	    IMX_UCR1_UARTEN);

	return (0);
}


#ifdef	IMXUARTCONSOLE
/*
 * Following are all routines needed for UART to act as console
 */
struct consdev imxucons = {
	NULL, NULL, imxucngetc, imxucnputc, imxucnpollc, NULL, NULL, NULL,
	NODEV, CN_NORMAL
};


int
imxuart_cons_attach(bus_space_tag_t iot, paddr_t iobase, u_int rate,
		    tcflag_t cflag)
{
	struct imxuart_regs regs;
	int res;

	regs.ur_iot = iot;
	regs.ur_iobase = iobase;

	res = imxuart_init(&regs, rate, cflag, true);
	if (res)
		return (res);

	cn_tab = &imxucons;
	cn_init_magic(&imxuart_cnm_state);
	cn_set_magic("\047\001"); /* default magic is BREAK */

	imxuconsrate = rate;
	imxuconscflag = cflag;

	imxuconsregs = regs;

	return 0;
}

int
imxucngetc(dev_t dev)
{
	return (imxuart_common_getc(dev, &imxuconsregs));
}

/*
 * Console kernel output character routine.
 */
void
imxucnputc(dev_t dev, int c)
{
	imxuart_common_putc(dev, &imxuconsregs, c);
}

void
imxucnpollc(dev_t dev, int on)
{

	imxuart_readahead_in = 0;
	imxuart_readahead_out = 0;
}

#endif	/* IMXUARTCONSOLE */

#ifdef KGDB
int
imxuart_kgdb_attach(bus_space_tag_t iot, paddr_t iobase, u_int rate,
    tcflag_t cflag)
{
	int res;

	if (iot == imxuconsregs.ur_iot &&
	    iobase == imxuconsregs.ur_iobase) {
#if !defined(DDB)
		return (EBUSY); /* cannot share with console */
#else
		imxu_kgdb_regs.ur_iot = iot;
		imxu_kgdb_regs.ur_ioh = imxuconsregs.ur_ioh;
		imxu_kgdb_regs.ur_iobase = iobase;
#endif
	} else {
		imxu_kgdb_regs.ur_iot = iot;
		imxu_kgdb_regs.ur_iobase = iobase;

		res = imxuart_init(&imxu_kgdb_regs, rate, cflag, true);
		if (res)
			return (res);

		/*
		 * XXXfvdl this shouldn't be needed, but the cn_magic goo
		 * expects this to be initialized
		 */
		cn_init_magic(&imxuart_cnm_state);
		cn_set_magic("\047\001");
	}

	kgdb_attach(imxuart_kgdb_getc, imxuart_kgdb_putc, &imxu_kgdb_regs);
	kgdb_dev = 123; /* unneeded, only to satisfy some tests */

	return (0);
}

/* ARGSUSED */
int
imxuart_kgdb_getc(void *arg)
{
	struct imxuart_regs *regs = arg;

	return (imxuart_common_getc(NODEV, regs));
}

/* ARGSUSED */
void
imxuart_kgdb_putc(void *arg, int c)
{
	struct imxuart_regs *regs = arg;

	imxuart_common_putc(NODEV, regs, c);
}
#endif /* KGDB */

/* helper function to identify the imxu ports used by
 console or KGDB (and not yet autoconf attached) */
int
imxuart_is_console(bus_space_tag_t iot, bus_addr_t iobase, bus_space_handle_t *ioh)
{
	bus_space_handle_t help;

	if (!imxuconsattached &&
	    iot == imxuconsregs.ur_iot && iobase == imxuconsregs.ur_iobase)
		help = imxuconsregs.ur_ioh;
#ifdef KGDB
	else if (!imxu_kgdb_attached &&
	    iot == imxu_kgdb_regs.ur_iot && iobase == imxu_kgdb_regs.ur_iobase)
		help = imxu_kgdb_regs.ur_ioh;
#endif
	else
		return (0);

	if (ioh)
		*ioh = help;
	return (1);
}

#ifdef notyet

bool
imxuart_cleanup(device_t self, int how)
{
/*
 * this routine exists to serve as a shutdown hook for systems that
 * have firmware which doesn't interact properly with a imxuart device in
 * FIFO mode.
 */
	struct imxuart_softc *sc = device_private(self);

	if (ISSET(sc->sc_hwflags, IMXUART_HW_FIFO))
		UR_WRITE_1(&sc->sc_regs, IMXUART_REG_FIFO, 0);

	return true;
}
#endif

#ifdef notyet
bool
imxuart_suspend(device_t self PMF_FN_ARGS)
{
	struct imxuart_softc *sc = device_private(self);

	UR_WRITE_1(&sc->sc_regs, IMXUART_REG_IER, 0);
	(void)CSR_READ_1(&sc->sc_regs, IMXUART_REG_IIR);

	return true;
}
#endif

#ifdef notyet
bool
imxuart_resume(device_t self PMF_FN_ARGS)
{
	struct imxuart_softc *sc = device_private(self);

	mutex_spin_enter(&sc->sc_lock);
	imxuart_loadchannelregs(sc);
	mutex_spin_exit(&sc->sc_lock);

	return true;
}
#endif

static void
imxuart_enable_debugport(struct imxuart_softc *sc)
{
	bus_space_tag_t iot = sc->sc_regs.ur_iot;
	bus_space_handle_t ioh = sc->sc_regs.ur_ioh;

	if (sc->sc_hwflags & (IMXUART_HW_CONSOLE|IMXUART_HW_KGDB)) {

		/* Turn on line break interrupt, set carrier. */

		sc->sc_ucr3 |= IMX_UCR3_DSR;
		bus_space_write_4(iot, ioh, IMX_UCR3, sc->sc_ucr3);

		sc->sc_ucr4 |= IMX_UCR4_BKEN;
		bus_space_write_4(iot, ioh, IMX_UCR4, sc->sc_ucr4);

		sc->sc_ucr2 |= IMX_UCR2_TXEN|IMX_UCR2_RXEN|
		    IMX_UCR2_CTS;
		bus_space_write_4(iot, ioh, IMX_UCR2, sc->sc_ucr2);

		sc->sc_ucr1 |= IMX_UCR1_UARTEN;
		bus_space_write_4(iot, ioh, IMX_UCR1, sc->sc_ucr1);
	}
}


void
imxuart_set_frequency(u_int freq, u_int div)
{
	imxuart_freq = freq;
	imxuart_freqdiv = div;
}

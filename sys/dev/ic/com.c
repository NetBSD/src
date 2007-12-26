/*	$NetBSD: com.c,v 1.266.2.2 2007/12/26 21:39:25 ad Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2004 The NetBSD Foundation, Inc.
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
 * COM driver, uses National Semiconductor NS16450/NS16550AF UART
 * Supports automatic hardware flow control on StarTech ST16C650A UART
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com.c,v 1.266.2.2 2007/12/26 21:39:25 ad Exp $");

#include "opt_com.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_ntp.h"

#include "rnd.h"
#if NRND > 0 && defined(RND_COM)
#include <sys/rnd.h>
#endif

/* The COM16650 option was renamed to COM_16650. */
#ifdef COM16650
#error Obsolete COM16650 option; use COM_16650 instead.
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
#include <sys/poll.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/user.h>
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

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#include <dev/ic/ns16550reg.h>
#include <dev/ic/st16650reg.h>
#ifdef COM_HAYESP
#include <dev/ic/hayespreg.h>
#endif
#define	com_lcr	com_cfcr
#include <dev/cons.h>

#ifdef	COM_REGMAP
#define	CSR_WRITE_1(r, o, v)	\
	bus_space_write_1((r)->cr_iot, (r)->cr_ioh, (r)->cr_map[o], v)
#define	CSR_READ_1(r, o)	\
	bus_space_read_1((r)->cr_iot, (r)->cr_ioh, (r)->cr_map[o])
#define	CSR_WRITE_2(r, o, v)	\
	bus_space_write_2((r)->cr_iot, (r)->cr_ioh, (r)->cr_map[o], v)
#define	CSR_READ_2(r, o)	\
	bus_space_read_2((r)->cr_iot, (r)->cr_ioh, (r)->cr_map[o])
#define	CSR_WRITE_MULTI(r, o, p, n)	\
	bus_space_write_multi_1((r)->cr_iot, (r)->cr_ioh, (r)->cr_map[o], p, n)
#else
#define	CSR_WRITE_1(r, o, v)	\
	bus_space_write_1((r)->cr_iot, (r)->cr_ioh, o, v)
#define	CSR_READ_1(r, o)	\
	bus_space_read_1((r)->cr_iot, (r)->cr_ioh, o)
#define	CSR_WRITE_2(r, o, v)	\
	bus_space_write_2((r)->cr_iot, (r)->cr_ioh, o, v)
#define	CSR_READ_2(r, o)	\
	bus_space_read_2((r)->cr_iot, (r)->cr_ioh, o)
#define	CSR_WRITE_MULTI(r, o, p, n)	\
	bus_space_write_multi_1((r)->cr_iot, (r)->cr_ioh, o, p, n)
#endif


static void com_enable_debugport(struct com_softc *);

void	com_config(struct com_softc *);
void	com_shutdown(struct com_softc *);
int	comspeed(long, long, int);
static	u_char	cflag2lcr(tcflag_t);
int	comparam(struct tty *, struct termios *);
void	comstart(struct tty *);
int	comhwiflow(struct tty *, int);

void	com_loadchannelregs(struct com_softc *);
void	com_hwiflow(struct com_softc *);
void	com_break(struct com_softc *, int);
void	com_modem(struct com_softc *, int);
void	tiocm_to_com(struct com_softc *, u_long, int);
int	com_to_tiocm(struct com_softc *);
void	com_iflush(struct com_softc *);

int	com_common_getc(dev_t, struct com_regs *);
void	com_common_putc(dev_t, struct com_regs *, int);

int	cominit(struct com_regs *, int, int, int, tcflag_t);

int	comcngetc(dev_t);
void	comcnputc(dev_t, int);
void	comcnpollc(dev_t, int);

#define	integrate	static inline
void 	comsoft(void *);
integrate void com_rxsoft(struct com_softc *, struct tty *);
integrate void com_txsoft(struct com_softc *, struct tty *);
integrate void com_stsoft(struct com_softc *, struct tty *);
integrate void com_schedrx(struct com_softc *);
void	comdiag(void *);

extern struct cfdriver com_cd;

dev_type_open(comopen);
dev_type_close(comclose);
dev_type_read(comread);
dev_type_write(comwrite);
dev_type_ioctl(comioctl);
dev_type_stop(comstop);
dev_type_tty(comtty);
dev_type_poll(compoll);

const struct cdevsw com_cdevsw = {
	comopen, comclose, comread, comwrite, comioctl,
	comstop, comtty, compoll, nommap, ttykqfilter, D_TTY
};

/*
 * Make this an option variable one can patch.
 * But be warned:  this must be a power of 2!
 */
u_int com_rbuf_size = COM_RING_SIZE;

/* Stop input when 3/4 of the ring is full; restart when only 1/4 is full. */
u_int com_rbuf_hiwat = (COM_RING_SIZE * 1) / 4;
u_int com_rbuf_lowat = (COM_RING_SIZE * 3) / 4;

static struct com_regs comconsregs;
static int comconsattached;
static int comconsrate;
static tcflag_t comconscflag;
static struct cnm_state com_cnm_state;

#ifndef __HAVE_TIMECOUNTER
static int ppscap =
	PPS_TSFMT_TSPEC |
	PPS_CAPTUREASSERT |
	PPS_CAPTURECLEAR |
	PPS_OFFSETASSERT | PPS_OFFSETCLEAR;
#endif /* !__HAVE_TIMECOUNTER */

#ifdef KGDB
#include <sys/kgdb.h>

static struct com_regs comkgdbregs;
static int com_kgdb_attached;

int	com_kgdb_getc(void *);
void	com_kgdb_putc(void *, int);
#endif /* KGDB */

#ifdef COM_REGMAP
/* initializer for typical 16550-ish hardware */
#define	COM_REG_16550	{ \
	com_data, com_data, com_dlbl, com_dlbh, com_ier, com_iir, com_fifo, \
	com_efr, com_lcr, com_mcr, com_lsr, com_msr }

const bus_size_t com_std_map[16] = COM_REG_16550;
#endif /* COM_REGMAP */

#define	COMUNIT_MASK	0x7ffff
#define	COMDIALOUT_MASK	0x80000

#define	COMUNIT(x)	(minor(x) & COMUNIT_MASK)
#define	COMDIALOUT(x)	(minor(x) & COMDIALOUT_MASK)

#define	COM_ISALIVE(sc)	((sc)->enabled != 0 && \
			 device_is_active(&(sc)->sc_dev))

#define	BR	BUS_SPACE_BARRIER_READ
#define	BW	BUS_SPACE_BARRIER_WRITE
#define COM_BARRIER(r, f) \
	bus_space_barrier((r)->cr_iot, (r)->cr_ioh, 0, (r)->cr_nports, (f))

/*ARGSUSED*/
int
comspeed(long speed, long frequency, int type)
{
#define	divrnd(n, q)	(((n)*2/(q)+1)/2)	/* divide and round off */

	int x, err;

#if 0
	if (speed == 0)
		return (0);
#endif
	if (speed <= 0)
		return (-1);
	x = divrnd(frequency / 16, speed);
	if (x <= 0)
		return (-1);
	err = divrnd(((quad_t)frequency) * 1000 / 16, speed * x) - 1000;
	if (err < 0)
		err = -err;
	if (err > COM_TOLERANCE)
		return (-1);
	return (x);

#undef	divrnd
}

#ifdef COM_DEBUG
int	com_debug = 0;

void comstatus(struct com_softc *, const char *);
void
comstatus(struct com_softc *sc, const char *str)
{
	struct tty *tp = sc->sc_tty;

	printf("%s: %s %cclocal  %cdcd %cts_carr_on %cdtr %ctx_stopped\n",
	    sc->sc_dev.dv_xname, str,
	    ISSET(tp->t_cflag, CLOCAL) ? '+' : '-',
	    ISSET(sc->sc_msr, MSR_DCD) ? '+' : '-',
	    ISSET(tp->t_state, TS_CARR_ON) ? '+' : '-',
	    ISSET(sc->sc_mcr, MCR_DTR) ? '+' : '-',
	    sc->sc_tx_stopped ? '+' : '-');

	printf("%s: %s %ccrtscts %ccts %cts_ttstop  %crts rx_flags=0x%x\n",
	    sc->sc_dev.dv_xname, str,
	    ISSET(tp->t_cflag, CRTSCTS) ? '+' : '-',
	    ISSET(sc->sc_msr, MSR_CTS) ? '+' : '-',
	    ISSET(tp->t_state, TS_TTSTOP) ? '+' : '-',
	    ISSET(sc->sc_mcr, MCR_RTS) ? '+' : '-',
	    sc->sc_rx_flags);
}
#endif

int
com_probe_subr(struct com_regs *regs)
{

	/* force access to id reg */
	CSR_WRITE_1(regs, COM_REG_LCR, LCR_8BITS);
	CSR_WRITE_1(regs, COM_REG_IIR, 0);
	if ((CSR_READ_1(regs, COM_REG_LCR) != LCR_8BITS) ||
	    (CSR_READ_1(regs, COM_REG_IIR) & 0x38))
		return (0);

	return (1);
}

int
comprobe1(bus_space_tag_t iot, bus_space_handle_t ioh)
{
	struct com_regs	regs;

	regs.cr_iot = iot;
	regs.cr_ioh = ioh;
#ifdef	COM_REGMAP
	memcpy(regs.cr_map, com_std_map, sizeof (regs.cr_map));;
#endif

	return com_probe_subr(&regs);
}

/*
 * No locking in this routine; it is only called during attach,
 * or with the port already locked.
 */
static void
com_enable_debugport(struct com_softc *sc)
{

	/* Turn on line break interrupt, set carrier. */
	sc->sc_ier = IER_ERXRDY;
	if (sc->sc_type == COM_TYPE_PXA2x0)
		sc->sc_ier |= IER_EUART | IER_ERXTOUT;
	CSR_WRITE_1(&sc->sc_regs, COM_REG_IER, sc->sc_ier);
	SET(sc->sc_mcr, MCR_DTR | MCR_RTS);
	CSR_WRITE_1(&sc->sc_regs, COM_REG_MCR, sc->sc_mcr);
}

void
com_attach_subr(struct com_softc *sc)
{
	struct com_regs *regsp = &sc->sc_regs;
	struct tty *tp;
#ifdef COM_16650
	u_int8_t lcr;
#endif
	const char *fifo_msg = NULL;

	aprint_naive("\n");

	callout_init(&sc->sc_diag_callout, 0);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_HIGH);

	/* Disable interrupts before configuring the device. */
	if (sc->sc_type == COM_TYPE_PXA2x0)
		sc->sc_ier = IER_EUART;
	else
		sc->sc_ier = 0;

	CSR_WRITE_1(regsp, COM_REG_IER, sc->sc_ier);

	if (regsp->cr_iot == comconsregs.cr_iot &&
	    regsp->cr_iobase == comconsregs.cr_iobase) {
		comconsattached = 1;

		/* Make sure the console is always "hardwired". */
		delay(10000);			/* wait for output to finish */
		SET(sc->sc_hwflags, COM_HW_CONSOLE);
		SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);
	}

	/* Probe for FIFO */
	switch (sc->sc_type) {
	case COM_TYPE_HAYESP:
		goto fifodone;

	case COM_TYPE_AU1x00:
		sc->sc_fifolen = 16;
		fifo_msg = "Au1X00 UART, working fifo";
		SET(sc->sc_hwflags, COM_HW_FIFO);
		goto fifodelay;
	}

	sc->sc_fifolen = 1;
	/* look for a NS 16550AF UART with FIFOs */
	CSR_WRITE_1(regsp, COM_REG_FIFO,
	    FIFO_ENABLE | FIFO_RCV_RST | FIFO_XMT_RST | FIFO_TRIGGER_14);
	delay(100);
	if (ISSET(CSR_READ_1(regsp, COM_REG_IIR), IIR_FIFO_MASK)
	    == IIR_FIFO_MASK)
		if (ISSET(CSR_READ_1(regsp, COM_REG_FIFO), FIFO_TRIGGER_14)
		    == FIFO_TRIGGER_14) {
			SET(sc->sc_hwflags, COM_HW_FIFO);

#ifdef COM_16650
			/*
			 * IIR changes into the EFR if LCR is set to LCR_EERS
			 * on 16650s. We also know IIR != 0 at this point.
			 * Write 0 into the EFR, and read it. If the result
			 * is 0, we have a 16650.
			 *
			 * Older 16650s were broken; the test to detect them
			 * is taken from the Linux driver. Apparently
			 * setting DLAB enable gives access to the EFR on
			 * these chips.
			 */
			lcr = CSR_READ_1(regsp, COM_REG_LCR);
			CSR_WRITE_1(regsp, COM_REG_LCR, LCR_EERS);
			CSR_WRITE_1(regsp, COM_REG_EFR, 0);
			if (CSR_READ_1(regsp, COM_REG_EFR) == 0) {
				CSR_WRITE_1(regsp, COM_REG_LCR,
				    lcr | LCR_DLAB);
				if (CSR_READ_1(regsp, COM_REG_EFR) == 0) {
					CLR(sc->sc_hwflags, COM_HW_FIFO);
					sc->sc_fifolen = 0;
				} else {
					SET(sc->sc_hwflags, COM_HW_FLOW);
					sc->sc_fifolen = 32;
				}
			} else
#endif
				sc->sc_fifolen = 16;

#ifdef COM_16650
			CSR_WRITE_1(regsp, COM_REG_LCR, lcr);
			if (sc->sc_fifolen == 0)
				fifo_msg = "st16650, broken fifo";
			else if (sc->sc_fifolen == 32)
				fifo_msg = "st16650a, working fifo";
			else
#endif
				fifo_msg = "ns16550a, working fifo";
		} else
			fifo_msg = "ns16550, broken fifo";
	else
		fifo_msg = "ns8250 or ns16450, no fifo";
	CSR_WRITE_1(regsp, COM_REG_FIFO, 0);
fifodelay:
	/*
	 * Some chips will clear down both Tx and Rx FIFOs when zero is
	 * written to com_fifo. If this chip is the console, writing zero
	 * results in some of the chip/FIFO description being lost, so delay
	 * printing it until now.
	 */
	delay(10);
	aprint_normal(": %s\n", fifo_msg);
	if (ISSET(sc->sc_hwflags, COM_HW_TXFIFO_DISABLE)) {
		sc->sc_fifolen = 1;
		aprint_normal("%s: txfifo disabled\n", sc->sc_dev.dv_xname);
	}

fifodone:

	tp = ttymalloc();
	tp->t_oproc = comstart;
	tp->t_param = comparam;
	tp->t_hwiflow = comhwiflow;

	sc->sc_tty = tp;
	sc->sc_rbuf = malloc(com_rbuf_size << 1, M_DEVBUF, M_NOWAIT);
	sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
	sc->sc_rbavail = com_rbuf_size;
	if (sc->sc_rbuf == NULL) {
		aprint_error("%s: unable to allocate ring buffer\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	sc->sc_ebuf = sc->sc_rbuf + (com_rbuf_size << 1);

	tty_attach(tp);

	if (!ISSET(sc->sc_hwflags, COM_HW_NOIEN))
		SET(sc->sc_mcr, MCR_IENABLE);

	if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
		int maj;

		/* locate the major number */
		maj = cdevsw_lookup_major(&com_cdevsw);

		tp->t_dev = cn_tab->cn_dev = makedev(maj,
						     device_unit(&sc->sc_dev));

		aprint_normal("%s: console\n", sc->sc_dev.dv_xname);
	}

#ifdef KGDB
	/*
	 * Allow kgdb to "take over" this port.  If this is
	 * not the console and is the kgdb device, it has
	 * exclusive use.  If it's the console _and_ the
	 * kgdb device, it doesn't.
	 */
	if (regsp->cr_iot == comkgdbregs.cr_iot &&
	    regsp->cr_iobase == comkgdbregs.cr_iobase) {
		if (!ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
			com_kgdb_attached = 1;

			SET(sc->sc_hwflags, COM_HW_KGDB);
		}
		aprint_normal("%s: kgdb\n", sc->sc_dev.dv_xname);
	}
#endif

	sc->sc_si = softint_establish(SOFTINT_SERIAL, comsoft, sc);

#if NRND > 0 && defined(RND_COM)
	rnd_attach_source(&sc->rnd_source, sc->sc_dev.dv_xname,
			  RND_TYPE_TTY, 0);
#endif

	/* if there are no enable/disable functions, assume the device
	   is always enabled */
	if (!sc->enable)
		sc->enabled = 1;

	com_config(sc);

	SET(sc->sc_hwflags, COM_HW_DEV_OK);
}

void
com_config(struct com_softc *sc)
{
	struct com_regs *regsp = &sc->sc_regs;

	/* Disable interrupts before configuring the device. */
	if (sc->sc_type == COM_TYPE_PXA2x0)
		sc->sc_ier = IER_EUART;
	else
		sc->sc_ier = 0;
	CSR_WRITE_1(regsp, COM_REG_IER, sc->sc_ier);
	(void) CSR_READ_1(regsp, COM_REG_IIR);

#ifdef COM_HAYESP
	/* Look for a Hayes ESP board. */
	if (sc->sc_type == COM_TYPE_HAYESP) {

		/* Set 16550 compatibility mode */
		bus_space_write_1(regsp->cr_iot, sc->sc_hayespioh, HAYESP_CMD1,
				  HAYESP_SETMODE);
		bus_space_write_1(regsp->cr_iot, sc->sc_hayespioh, HAYESP_CMD2,
				  HAYESP_MODE_FIFO|HAYESP_MODE_RTS|
				  HAYESP_MODE_SCALE);

		/* Set RTS/CTS flow control */
		bus_space_write_1(regsp->cr_iot, sc->sc_hayespioh, HAYESP_CMD1,
				  HAYESP_SETFLOWTYPE);
		bus_space_write_1(regsp->cr_iot, sc->sc_hayespioh, HAYESP_CMD2,
				  HAYESP_FLOW_RTS);
		bus_space_write_1(regsp->cr_iot, sc->sc_hayespioh, HAYESP_CMD2,
				  HAYESP_FLOW_CTS);

		/* Set flow control levels */
		bus_space_write_1(regsp->cr_iot, sc->sc_hayespioh, HAYESP_CMD1,
				  HAYESP_SETRXFLOW);
		bus_space_write_1(regsp->cr_iot, sc->sc_hayespioh, HAYESP_CMD2,
				  HAYESP_HIBYTE(HAYESP_RXHIWMARK));
		bus_space_write_1(regsp->cr_iot, sc->sc_hayespioh, HAYESP_CMD2,
				  HAYESP_LOBYTE(HAYESP_RXHIWMARK));
		bus_space_write_1(regsp->cr_iot, sc->sc_hayespioh, HAYESP_CMD2,
				  HAYESP_HIBYTE(HAYESP_RXLOWMARK));
		bus_space_write_1(regsp->cr_iot, sc->sc_hayespioh, HAYESP_CMD2,
				  HAYESP_LOBYTE(HAYESP_RXLOWMARK));
	}
#endif

	if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE|COM_HW_KGDB))
		com_enable_debugport(sc);
}

int
com_detach(struct device *self, int flags)
{
	struct com_softc *sc = (struct com_softc *)self;
	int maj, mn;

	/* locate the major number */
	maj = cdevsw_lookup_major(&com_cdevsw);

	/* Nuke the vnodes for any open instances. */
	mn = device_unit(self);
	vdevgone(maj, mn, mn, VCHR);

	mn |= COMDIALOUT_MASK;
	vdevgone(maj, mn, mn, VCHR);

	if (sc->sc_rbuf == NULL) {
		/*
		 * Ring buffer allocation failed in the com_attach_subr,
		 * only the tty is allocated, and nothing else.
		 */
		ttyfree(sc->sc_tty);
		return 0;
	}

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

	return (0);
}

int
com_activate(struct device *self, enum devact act)
{
	struct com_softc *sc = (struct com_softc *)self;
	int rv = 0;

	mutex_spin_enter(&sc->sc_lock);
	switch (act) {
	case DVACT_ACTIVATE:
		rv = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		if (sc->sc_hwflags & (COM_HW_CONSOLE|COM_HW_KGDB)) {
			rv = EBUSY;
			break;
		}

		if (sc->disable != NULL && sc->enabled != 0) {
			(*sc->disable)(sc);
			sc->enabled = 0;
		}
		break;
	}

	mutex_spin_exit(&sc->sc_lock);
	return (rv);
}

void
com_shutdown(struct com_softc *sc)
{
	struct tty *tp = sc->sc_tty;

	mutex_spin_enter(&sc->sc_lock);

	/* If we were asserting flow control, then deassert it. */
	SET(sc->sc_rx_flags, RX_IBUF_BLOCKED);
	com_hwiflow(sc);

	/* Clear any break condition set with TIOCSBRK. */
	com_break(sc, 0);

#ifndef __HAVE_TIMECOUNTER
	/* Turn off PPS capture on last close. */
	sc->sc_ppsmask = 0;
	sc->ppsparam.mode = 0;
#endif /* !__HAVE_TIMECOUNTER */

	/*
	 * Hang up if necessary.  Wait a bit, so the other side has time to
	 * notice even if we immediately open the port again.
	 * Avoid tsleeping above splhigh().
	 */
	if (ISSET(tp->t_cflag, HUPCL)) {
		com_modem(sc, 0);
		mutex_spin_exit(&sc->sc_lock);
		/* XXX will only timeout */
		(void) kpause(ttclos, false, hz, NULL);
		mutex_spin_enter(&sc->sc_lock);
	}

	/* Turn off interrupts. */
	if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
		sc->sc_ier = IER_ERXRDY; /* interrupt on break */
		if (sc->sc_type == COM_TYPE_PXA2x0)
			sc->sc_ier |= IER_ERXTOUT;
	} else
		sc->sc_ier = 0;

	if (sc->sc_type == COM_TYPE_PXA2x0)
		sc->sc_ier |= IER_EUART;

	CSR_WRITE_1(&sc->sc_regs, COM_REG_IER, sc->sc_ier);

	if (sc->disable) {
#ifdef DIAGNOSTIC
		if (!sc->enabled)
			panic("com_shutdown: not enabled?");
#endif
		(*sc->disable)(sc);
		sc->enabled = 0;
	}
	mutex_spin_exit(&sc->sc_lock);
}

int
comopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct com_softc *sc;
	struct tty *tp;
	int s;
	int error;

	sc = device_lookup(&com_cd, COMUNIT(dev));
	if (sc == NULL || !ISSET(sc->sc_hwflags, COM_HW_DEV_OK) ||
		sc->sc_rbuf == NULL)
		return (ENXIO);

	if (!device_is_active(&sc->sc_dev))
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

		mutex_spin_enter(&sc->sc_lock);

		if (sc->enable) {
			if ((*sc->enable)(sc)) {
				mutex_spin_exit(&sc->sc_lock);
				splx(s);
				printf("%s: device enable failed\n",
				       sc->sc_dev.dv_xname);
				return (EIO);
			}
			sc->enabled = 1;
			com_config(sc);
		}

		/* Turn on interrupts. */
		sc->sc_ier = IER_ERXRDY | IER_ERLS | IER_EMSC;
		if (sc->sc_type == COM_TYPE_PXA2x0)
			sc->sc_ier |= IER_EUART | IER_ERXTOUT;
		CSR_WRITE_1(&sc->sc_regs, COM_REG_IER, sc->sc_ier);

		/* Fetch the current modem control status, needed later. */
		sc->sc_msr = CSR_READ_1(&sc->sc_regs, COM_REG_MSR);

		/* Clear PPS capture state on first open. */
#ifdef __HAVE_TIMECOUNTER
		memset(&sc->sc_pps_state, 0, sizeof(sc->sc_pps_state));
		sc->sc_pps_state.ppscap = PPS_CAPTUREASSERT | PPS_CAPTURECLEAR;
		pps_init(&sc->sc_pps_state);
#else /* !__HAVE_TIMECOUNTER */
		sc->sc_ppsmask = 0;
		sc->ppsparam.mode = 0;
#endif /* !__HAVE_TIMECOUNTER */

		mutex_spin_exit(&sc->sc_lock);

		/*
		 * Initialize the termios status to the defaults.  Add in the
		 * sticky bits from TIOCSFLAGS.
		 */
		if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
			t.c_ospeed = comconsrate;
			t.c_cflag = comconscflag;
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
		/* Make sure comparam() will do something. */
		tp->t_ospeed = 0;
		(void) comparam(tp, &t);
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
		com_modem(sc, 1);

		/* Clear the input ring, and unblock. */
		sc->sc_rbput = sc->sc_rbget = sc->sc_rbuf;
		sc->sc_rbavail = com_rbuf_size;
		com_iflush(sc);
		CLR(sc->sc_rx_flags, RX_ANY_BLOCK);
		com_hwiflow(sc);

#ifdef COM_DEBUG
		if (com_debug)
			comstatus(sc, "comopen  ");
#endif

		mutex_spin_exit(&sc->sc_lock);
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
		com_shutdown(sc);
	}

	return (error);
}

int
comclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct com_softc *sc = device_lookup(&com_cd, COMUNIT(dev));
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
		com_shutdown(sc);
	}

	return (0);
}

int
comread(dev_t dev, struct uio *uio, int flag)
{
	struct com_softc *sc = device_lookup(&com_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
comwrite(dev_t dev, struct uio *uio, int flag)
{
	struct com_softc *sc = device_lookup(&com_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);

	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
compoll(dev_t dev, int events, struct lwp *l)
{
	struct com_softc *sc = device_lookup(&com_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (COM_ISALIVE(sc) == 0)
		return (POLLHUP);

	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

struct tty *
comtty(dev_t dev)
{
	struct com_softc *sc = device_lookup(&com_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	return (tp);
}

int
comioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct com_softc *sc = device_lookup(&com_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;
	int error;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);

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
		com_break(sc, 1);
		break;

	case TIOCCBRK:
		com_break(sc, 0);
		break;

	case TIOCSDTR:
		com_modem(sc, 1);
		break;

	case TIOCCDTR:
		com_modem(sc, 0);
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
		tiocm_to_com(sc, cmd, *(int *)data);
		break;

	case TIOCMGET:
		*(int *)data = com_to_tiocm(sc);
		break;

#ifdef __HAVE_TIMECOUNTER
	case PPS_IOC_CREATE:
	case PPS_IOC_DESTROY:
	case PPS_IOC_GETPARAMS:
	case PPS_IOC_SETPARAMS:
	case PPS_IOC_GETCAP:
	case PPS_IOC_FETCH:
#ifdef PPS_SYNC
	case PPS_IOC_KCBIND:
#endif
		error = pps_ioctl(cmd, data, &sc->sc_pps_state);
		break;
#else /* !__HAVE_TIMECOUNTER */
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

#ifdef PPS_SYNC
	case PPS_IOC_KCBIND: {
		int edge = (*(int *)data) & PPS_CAPTUREBOTH;

		if (edge == 0) {
			/*
			 * remove binding for this source; ignore
			 * the request if this is not the current
			 * hardpps source
			 */
			if (pps_kc_hardpps_source == sc) {
				pps_kc_hardpps_source = NULL;
				pps_kc_hardpps_mode = 0;
			}
		} else {
			/*
			 * bind hardpps to this source, replacing any
			 * previously specified source or edges
			 */
			pps_kc_hardpps_source = sc;
			pps_kc_hardpps_mode = edge;
		}
		break;
	}
#endif /* PPS_SYNC */
#endif /* !__HAVE_TIMECOUNTER */

	case TIOCDCDTIMESTAMP:	/* XXX old, overloaded  API used by xntpd v3 */
#ifdef __HAVE_TIMECOUNTER
#ifndef PPS_TRAILING_EDGE
		TIMESPEC_TO_TIMEVAL((struct timeval *)data,
		    &sc->sc_pps_state.ppsinfo.assert_timestamp);
#else
		TIMESPEC_TO_TIMEVAL((struct timeval *)data,
		    &sc->sc_pps_state.ppsinfo.clear_timestamp);
#endif
#else /* !__HAVE_TIMECOUNTER */
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
		sc->sc_ppsassert = -1;
		sc->sc_ppsclear = 0;
		TIMESPEC_TO_TIMEVAL((struct timeval *)data,
		    &sc->ppsinfo.clear_timestamp);
#endif
#endif /* !__HAVE_TIMECOUNTER */
		break;

	default:
		error = EPASSTHROUGH;
		break;
	}

	mutex_spin_exit(&sc->sc_lock);

#ifdef COM_DEBUG
	if (com_debug)
		comstatus(sc, "comioctl ");
#endif

	return (error);
}

integrate void
com_schedrx(struct com_softc *sc)
{

	sc->sc_rx_ready = 1;

	/* Wake up the poller. */
	softint_schedule(sc->sc_si);
}

void
com_break(struct com_softc *sc, int onoff)
{

	if (onoff)
		SET(sc->sc_lcr, LCR_SBREAK);
	else
		CLR(sc->sc_lcr, LCR_SBREAK);

	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			com_loadchannelregs(sc);
	}
}

void
com_modem(struct com_softc *sc, int onoff)
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
			com_loadchannelregs(sc);
	}
}

void
tiocm_to_com(struct com_softc *sc, u_long how, int ttybits)
{
	u_char combits;

	combits = 0;
	if (ISSET(ttybits, TIOCM_DTR))
		SET(combits, MCR_DTR);
	if (ISSET(ttybits, TIOCM_RTS))
		SET(combits, MCR_RTS);

	switch (how) {
	case TIOCMBIC:
		CLR(sc->sc_mcr, combits);
		break;

	case TIOCMBIS:
		SET(sc->sc_mcr, combits);
		break;

	case TIOCMSET:
		CLR(sc->sc_mcr, MCR_DTR | MCR_RTS);
		SET(sc->sc_mcr, combits);
		break;
	}

	if (!sc->sc_heldchange) {
		if (sc->sc_tx_busy) {
			sc->sc_heldtbc = sc->sc_tbc;
			sc->sc_tbc = 0;
			sc->sc_heldchange = 1;
		} else
			com_loadchannelregs(sc);
	}
}

int
com_to_tiocm(struct com_softc *sc)
{
	u_char combits;
	int ttybits = 0;

	combits = sc->sc_mcr;
	if (ISSET(combits, MCR_DTR))
		SET(ttybits, TIOCM_DTR);
	if (ISSET(combits, MCR_RTS))
		SET(ttybits, TIOCM_RTS);

	combits = sc->sc_msr;
	if (ISSET(combits, MSR_DCD))
		SET(ttybits, TIOCM_CD);
	if (ISSET(combits, MSR_CTS))
		SET(ttybits, TIOCM_CTS);
	if (ISSET(combits, MSR_DSR))
		SET(ttybits, TIOCM_DSR);
	if (ISSET(combits, MSR_RI | MSR_TERI))
		SET(ttybits, TIOCM_RI);

	if (ISSET(sc->sc_ier, IER_ERXRDY | IER_ETXRDY | IER_ERLS | IER_EMSC))
		SET(ttybits, TIOCM_LE);

	return (ttybits);
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
		SET(lcr, LCR_PENAB);
		if (!ISSET(cflag, PARODD))
			SET(lcr, LCR_PEVEN);
	}
	if (ISSET(cflag, CSTOPB))
		SET(lcr, LCR_STOPB);

	return (lcr);
}

int
comparam(struct tty *tp, struct termios *t)
{
	struct com_softc *sc = device_lookup(&com_cd, COMUNIT(tp->t_dev));
	int ospeed;
	u_char lcr;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);

#ifdef COM_HAYESP
	if (sc->sc_type == COM_TYPE_HAYESP) {
		int prescaler, speed;

		/*
		 * Calculate UART clock prescaler.  It should be in
		 * range of 0 .. 3.
		 */
		for (prescaler = 0, speed = t->c_ospeed; prescaler < 4;
		    prescaler++, speed /= 2)
			if ((ospeed = comspeed(speed, sc->sc_frequency,
					       sc->sc_type)) > 0)
				break;

		if (prescaler == 4)
			return (EINVAL);
		sc->sc_prescaler = prescaler;
	} else
#endif
	ospeed = comspeed(t->c_ospeed, sc->sc_frequency, sc->sc_type);

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

	lcr = ISSET(sc->sc_lcr, LCR_SBREAK) | cflag2lcr(t->c_cflag);

	mutex_spin_enter(&sc->sc_lock);

	sc->sc_lcr = lcr;

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

#if 0
	if (ospeed == 0)
		CLR(sc->sc_mcr, sc->sc_mcr_dtr);
	else
		SET(sc->sc_mcr, sc->sc_mcr_dtr);
#endif

	sc->sc_dlbl = ospeed;
	sc->sc_dlbh = ospeed >> 8;

	/*
	 * Set the FIFO threshold based on the receive speed.
	 *
	 *  * If it's a low speed, it's probably a mouse or some other
	 *    interactive device, so set the threshold low.
	 *  * If it's a high speed, trim the trigger level down to prevent
	 *    overflows.
	 *  * Otherwise set it a bit higher.
	 */
	if (sc->sc_type == COM_TYPE_HAYESP)
		sc->sc_fifo = FIFO_DMA_MODE | FIFO_ENABLE | FIFO_TRIGGER_8;
	else if (ISSET(sc->sc_hwflags, COM_HW_FIFO))
		sc->sc_fifo = FIFO_ENABLE |
		    (t->c_ospeed <= 1200 ? FIFO_TRIGGER_1 : FIFO_TRIGGER_8);
	else
		sc->sc_fifo = 0;

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
			com_loadchannelregs(sc);
	}

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		/* Disable the high water mark. */
		sc->sc_r_hiwat = 0;
		sc->sc_r_lowat = 0;
		if (ISSET(sc->sc_rx_flags, RX_TTY_OVERFLOWED)) {
			CLR(sc->sc_rx_flags, RX_TTY_OVERFLOWED);
			com_schedrx(sc);
		}
		if (ISSET(sc->sc_rx_flags, RX_TTY_BLOCKED|RX_IBUF_BLOCKED)) {
			CLR(sc->sc_rx_flags, RX_TTY_BLOCKED|RX_IBUF_BLOCKED);
			com_hwiflow(sc);
		}
	} else {
		sc->sc_r_hiwat = com_rbuf_hiwat;
		sc->sc_r_lowat = com_rbuf_lowat;
	}

	mutex_spin_exit(&sc->sc_lock);

	/*
	 * Update the tty layer's idea of the carrier bit, in case we changed
	 * CLOCAL or MDMBUF.  We don't hang up here; we only do that by
	 * explicit request.
	 */
	(void) (*tp->t_linesw->l_modem)(tp, ISSET(sc->sc_msr, MSR_DCD));

#ifdef COM_DEBUG
	if (com_debug)
		comstatus(sc, "comparam ");
#endif

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		if (sc->sc_tx_stopped) {
			sc->sc_tx_stopped = 0;
			comstart(tp);
		}
	}

	return (0);
}

void
com_iflush(struct com_softc *sc)
{
	struct com_regs	*regsp = &sc->sc_regs;
#ifdef DIAGNOSTIC
	int reg;
#endif
	int timo;

#ifdef DIAGNOSTIC
	reg = 0xffff;
#endif
	timo = 50000;
	/* flush any pending I/O */
	while (ISSET(CSR_READ_1(regsp, COM_REG_LSR), LSR_RXRDY)
	    && --timo)
#ifdef DIAGNOSTIC
		reg =
#else
		    (void)
#endif
		    CSR_READ_1(regsp, COM_REG_RXDATA);
#ifdef DIAGNOSTIC
	if (!timo)
		printf("%s: com_iflush timeout %02x\n", sc->sc_dev.dv_xname,
		       reg);
#endif
}

void
com_loadchannelregs(struct com_softc *sc)
{
	struct com_regs *regsp = &sc->sc_regs;

	/* XXXXX necessary? */
	com_iflush(sc);

	if (sc->sc_type == COM_TYPE_PXA2x0)
		CSR_WRITE_1(regsp, COM_REG_IER, IER_EUART);
	else
		CSR_WRITE_1(regsp, COM_REG_IER, 0);

	if (ISSET(sc->sc_hwflags, COM_HW_FLOW)) {
		if (sc->sc_type != COM_TYPE_AU1x00) {	/* no EFR on alchemy */
			CSR_WRITE_1(regsp, COM_REG_EFR, sc->sc_efr);
			CSR_WRITE_1(regsp, COM_REG_LCR, LCR_EERS);
		}
	}
	if (sc->sc_type == COM_TYPE_AU1x00) {
		/* alchemy has single separate 16-bit clock divisor register */
		CSR_WRITE_2(regsp, COM_REG_DLBL, sc->sc_dlbl +
		    (sc->sc_dlbh << 8));
	} else {
		CSR_WRITE_1(regsp, COM_REG_LCR, sc->sc_lcr | LCR_DLAB);
		CSR_WRITE_1(regsp, COM_REG_DLBL, sc->sc_dlbl);
		CSR_WRITE_1(regsp, COM_REG_DLBH, sc->sc_dlbh);
	}
	CSR_WRITE_1(regsp, COM_REG_LCR, sc->sc_lcr);
	CSR_WRITE_1(regsp, COM_REG_MCR, sc->sc_mcr_active = sc->sc_mcr);
	CSR_WRITE_1(regsp, COM_REG_FIFO, sc->sc_fifo);
#ifdef COM_HAYESP
	if (sc->sc_type == COM_TYPE_HAYESP) {
		bus_space_write_1(regsp->cr_iot, sc->sc_hayespioh, HAYESP_CMD1,
		    HAYESP_SETPRESCALER);
		bus_space_write_1(regsp->cr_iot, sc->sc_hayespioh, HAYESP_CMD2,
		    sc->sc_prescaler);
	}
#endif

	CSR_WRITE_1(regsp, COM_REG_IER, sc->sc_ier);
}

int
comhwiflow(struct tty *tp, int block)
{
	struct com_softc *sc = device_lookup(&com_cd, COMUNIT(tp->t_dev));

	if (COM_ISALIVE(sc) == 0)
		return (0);

	if (sc->sc_mcr_rts == 0)
		return (0);

	mutex_spin_enter(&sc->sc_lock);

	if (block) {
		if (!ISSET(sc->sc_rx_flags, RX_TTY_BLOCKED)) {
			SET(sc->sc_rx_flags, RX_TTY_BLOCKED);
			com_hwiflow(sc);
		}
	} else {
		if (ISSET(sc->sc_rx_flags, RX_TTY_OVERFLOWED)) {
			CLR(sc->sc_rx_flags, RX_TTY_OVERFLOWED);
			com_schedrx(sc);
		}
		if (ISSET(sc->sc_rx_flags, RX_TTY_BLOCKED)) {
			CLR(sc->sc_rx_flags, RX_TTY_BLOCKED);
			com_hwiflow(sc);
		}
	}

	mutex_spin_exit(&sc->sc_lock);
	return (1);
}

/*
 * (un)block input via hw flowcontrol
 */
void
com_hwiflow(struct com_softc *sc)
{
	struct com_regs *regsp= &sc->sc_regs;

	if (sc->sc_mcr_rts == 0)
		return;

	if (ISSET(sc->sc_rx_flags, RX_ANY_BLOCK)) {
		CLR(sc->sc_mcr, sc->sc_mcr_rts);
		CLR(sc->sc_mcr_active, sc->sc_mcr_rts);
	} else {
		SET(sc->sc_mcr, sc->sc_mcr_rts);
		SET(sc->sc_mcr_active, sc->sc_mcr_rts);
	}
	CSR_WRITE_1(regsp, COM_REG_MCR, sc->sc_mcr_active);
}


void
comstart(struct tty *tp)
{
	struct com_softc *sc = device_lookup(&com_cd, COMUNIT(tp->t_dev));
	struct com_regs *regsp = &sc->sc_regs;
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

		mutex_spin_enter(&sc->sc_lock);

		sc->sc_tba = tba;
		sc->sc_tbc = tbc;
	}

	SET(tp->t_state, TS_BUSY);
	sc->sc_tx_busy = 1;

	/* Enable transmit completion interrupts if necessary. */
	if (!ISSET(sc->sc_ier, IER_ETXRDY)) {
		SET(sc->sc_ier, IER_ETXRDY);
		CSR_WRITE_1(regsp, COM_REG_IER, sc->sc_ier);
	}

	/* Output the first chunk of the contiguous buffer. */
	if (!ISSET(sc->sc_hwflags, COM_HW_NO_TXPRELOAD)) {
		u_int n;

		n = sc->sc_tbc;
		if (n > sc->sc_fifolen)
			n = sc->sc_fifolen;
		CSR_WRITE_MULTI(regsp, COM_REG_TXDATA, sc->sc_tba, n);
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
comstop(struct tty *tp, int flag)
{
	struct com_softc *sc = device_lookup(&com_cd, COMUNIT(tp->t_dev));

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
comdiag(void *arg)
{
	struct com_softc *sc = arg;
	int overflows, floods;

	mutex_spin_enter(&sc->sc_lock);
	overflows = sc->sc_overflows;
	sc->sc_overflows = 0;
	floods = sc->sc_floods;
	sc->sc_floods = 0;
	sc->sc_errors = 0;
	mutex_spin_exit(&sc->sc_lock);

	log(LOG_WARNING, "%s: %d silo overflow%s, %d ibuf flood%s\n",
	    sc->sc_dev.dv_xname,
	    overflows, overflows == 1 ? "" : "s",
	    floods, floods == 1 ? "" : "s");
}

integrate void
com_rxsoft(struct com_softc *sc, struct tty *tp)
{
	int (*rint)(int, struct tty *) = tp->t_linesw->l_rint;
	u_char *get, *end;
	u_int cc, scc;
	u_char lsr;
	int code;

	end = sc->sc_ebuf;
	get = sc->sc_rbget;
	scc = cc = com_rbuf_size - sc->sc_rbavail;

	if (cc == com_rbuf_size) {
		sc->sc_floods++;
		if (sc->sc_errors++ == 0)
			callout_reset(&sc->sc_diag_callout, 60 * hz,
			    comdiag, sc);
	}

	/* If not yet open, drop the entire buffer content here */
	if (!ISSET(tp->t_state, TS_ISOPEN)) {
		get += cc << 1;
		if (get >= end)
			get -= com_rbuf_size << 1;
		cc = 0;
	}
	while (cc) {
		code = get[0];
		lsr = get[1];
		if (ISSET(lsr, LSR_OE | LSR_BI | LSR_FE | LSR_PE)) {
			if (ISSET(lsr, LSR_OE)) {
				sc->sc_overflows++;
				if (sc->sc_errors++ == 0)
					callout_reset(&sc->sc_diag_callout,
					    60 * hz, comdiag, sc);
			}
			if (ISSET(lsr, LSR_BI | LSR_FE))
				SET(code, TTY_FE);
			if (ISSET(lsr, LSR_PE))
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
					get -= com_rbuf_size << 1;
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
		mutex_spin_enter(&sc->sc_lock);

		cc = sc->sc_rbavail += scc - cc;
		/* Buffers should be ok again, release possible block. */
		if (cc >= sc->sc_r_lowat) {
			if (ISSET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
				SET(sc->sc_ier, IER_ERXRDY);
#ifdef COM_PXA2X0
				if (sc->sc_type == COM_TYPE_PXA2x0)
					SET(sc->sc_ier, IER_ERXTOUT);
#endif
				CSR_WRITE_1(&sc->sc_regs, COM_REG_IER, sc->sc_ier);
			}
			if (ISSET(sc->sc_rx_flags, RX_IBUF_BLOCKED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_BLOCKED);
				com_hwiflow(sc);
			}
		}
		mutex_spin_exit(&sc->sc_lock);
	}
}

integrate void
com_txsoft(struct com_softc *sc, struct tty *tp)
{

	CLR(tp->t_state, TS_BUSY);
	if (ISSET(tp->t_state, TS_FLUSH))
		CLR(tp->t_state, TS_FLUSH);
	else
		ndflush(&tp->t_outq, (int)(sc->sc_tba - tp->t_outq.c_cf));
	(*tp->t_linesw->l_start)(tp);
}

integrate void
com_stsoft(struct com_softc *sc, struct tty *tp)
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

#ifdef COM_DEBUG
	if (com_debug)
		comstatus(sc, "com_stsoft");
#endif
}

void
comsoft(void *arg)
{
	struct com_softc *sc = arg;
	struct tty *tp;

	if (COM_ISALIVE(sc) == 0)
		return;

	tp = sc->sc_tty;

	if (sc->sc_rx_ready) {
		sc->sc_rx_ready = 0;
		com_rxsoft(sc, tp);
	}

	if (sc->sc_st_check) {
		sc->sc_st_check = 0;
		com_stsoft(sc, tp);
	}

	if (sc->sc_tx_done) {
		sc->sc_tx_done = 0;
		com_txsoft(sc, tp);
	}
}

int
comintr(void *arg)
{
	struct com_softc *sc = arg;
	struct com_regs *regsp = &sc->sc_regs;

	u_char *put, *end;
	u_int cc;
	u_char lsr, iir;

	if (COM_ISALIVE(sc) == 0)
		return (0);

	mutex_spin_enter(&sc->sc_lock);
	iir = CSR_READ_1(regsp, COM_REG_IIR);
	if (ISSET(iir, IIR_NOPEND)) {
		mutex_spin_exit(&sc->sc_lock);
		return (0);
	}

	end = sc->sc_ebuf;
	put = sc->sc_rbput;
	cc = sc->sc_rbavail;

again:	do {
		u_char	msr, delta;

		lsr = CSR_READ_1(regsp, COM_REG_LSR);
		if (ISSET(lsr, LSR_BI)) {
			int cn_trapped = 0;

			cn_check_magic(sc->sc_tty->t_dev,
				       CNC_BREAK, com_cnm_state);
			if (cn_trapped)
				continue;
#if defined(KGDB) && !defined(DDB)
			if (ISSET(sc->sc_hwflags, COM_HW_KGDB)) {
				kgdb_connect(1);
				continue;
			}
#endif
		}

		if (ISSET(lsr, LSR_RCV_MASK) &&
		    !ISSET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED)) {
			while (cc > 0) {
				int cn_trapped = 0;
				put[0] = CSR_READ_1(regsp, COM_REG_RXDATA);
				put[1] = lsr;
				cn_check_magic(sc->sc_tty->t_dev,
					       put[0], com_cnm_state);
				if (cn_trapped)
					goto next;
				put += 2;
				if (put >= end)
					put = sc->sc_rbuf;
				cc--;
			next:
				lsr = CSR_READ_1(regsp, COM_REG_LSR);
				if (!ISSET(lsr, LSR_RCV_MASK))
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
				com_hwiflow(sc);
			}

			/*
			 * If we're out of space, disable receive interrupts
			 * until the queue has drained a bit.
			 */
			if (!cc) {
				SET(sc->sc_rx_flags, RX_IBUF_OVERFLOWED);
#ifdef COM_PXA2X0
				if (sc->sc_type == COM_TYPE_PXA2x0)
					CLR(sc->sc_ier, IER_ERXRDY|IER_ERXTOUT);
				else
#endif
					CLR(sc->sc_ier, IER_ERXRDY);
				CSR_WRITE_1(regsp, COM_REG_IER, sc->sc_ier);
			}
		} else {
			if ((iir & (IIR_RXRDY|IIR_TXRDY)) == IIR_RXRDY) {
				(void) CSR_READ_1(regsp, COM_REG_RXDATA);
				continue;
			}
		}

		msr = CSR_READ_1(regsp, COM_REG_MSR);
		delta = msr ^ sc->sc_msr;
		sc->sc_msr = msr;
#ifdef __HAVE_TIMECOUNTER
		if ((sc->sc_pps_state.ppsparam.mode & PPS_CAPTUREBOTH) &&
		    (delta & MSR_DCD)) {
			pps_capture(&sc->sc_pps_state);
			pps_event(&sc->sc_pps_state,
			    (msr & MSR_DCD) ?
			    PPS_CAPTUREASSERT :
			    PPS_CAPTURECLEAR);
		}
#else /* !__HAVE_TIMECOUNTER */
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
				if (pps_kc_hardpps_source == sc &&
				    pps_kc_hardpps_mode & PPS_CAPTUREASSERT) {
					hardpps(&tv, tv.tv_usec);
				}
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
				if (pps_kc_hardpps_source == sc &&
				    pps_kc_hardpps_mode & PPS_CAPTURECLEAR) {
					hardpps(&tv, tv.tv_usec);
				}
#endif
				sc->ppsinfo.clear_sequence++;
				sc->ppsinfo.current_mode = sc->ppsparam.mode;
			}
		}
#endif /* !__HAVE_TIMECOUNTER */

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
#ifdef COM_DEBUG
				if (com_debug)
					comstatus(sc, "comintr  ");
#endif
			}

			sc->sc_st_check = 1;
		}
	} while (!ISSET((iir =
	    CSR_READ_1(regsp, COM_REG_IIR)), IIR_NOPEND) &&
	    /*
	     * Since some device (e.g., ST16C1550) doesn't clear IIR_TXRDY
	     * by IIR read, so we can't do this way: `process all interrupts,
	     * then do TX if possble'.
	     */
	    (iir & IIR_IMASK) != IIR_TXRDY);

	/*
	 * Read LSR again, since there may be an interrupt between
	 * the last LSR read and IIR read above.
	 */
	lsr = CSR_READ_1(regsp, COM_REG_LSR);

	/*
	 * See if data can be transmitted as well.
	 * Schedule tx done event if no data left
	 * and tty was marked busy.
	 */
	if (ISSET(lsr, LSR_TXRDY)) {
		/*
		 * If we've delayed a parameter change, do it now, and restart
		 * output.
		 */
		if (sc->sc_heldchange) {
			com_loadchannelregs(sc);
			sc->sc_heldchange = 0;
			sc->sc_tbc = sc->sc_heldtbc;
			sc->sc_heldtbc = 0;
		}

		/* Output the next chunk of the contiguous buffer, if any. */
		if (sc->sc_tbc > 0) {
			u_int n;

			n = sc->sc_tbc;
			if (n > sc->sc_fifolen)
				n = sc->sc_fifolen;
			CSR_WRITE_MULTI(regsp, COM_REG_TXDATA, sc->sc_tba, n);
			sc->sc_tbc -= n;
			sc->sc_tba += n;
		} else {
			/* Disable transmit completion interrupts if necessary. */
			if (ISSET(sc->sc_ier, IER_ETXRDY)) {
				CLR(sc->sc_ier, IER_ETXRDY);
				CSR_WRITE_1(regsp, COM_REG_IER, sc->sc_ier);
			}
			if (sc->sc_tx_busy) {
				sc->sc_tx_busy = 0;
				sc->sc_tx_done = 1;
			}
		}
	}

	if (!ISSET((iir = CSR_READ_1(regsp, COM_REG_IIR)), IIR_NOPEND))
		goto again;

	mutex_spin_exit(&sc->sc_lock);

	/* Wake up the poller. */
	softint_schedule(sc->sc_si);

#if NRND > 0 && defined(RND_COM)
	rnd_add_uint32(&sc->rnd_source, iir | lsr);
#endif

	return (1);
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
static int com_readahead[MAX_READAHEAD];
static int com_readaheadcount = 0;

int
com_common_getc(dev_t dev, struct com_regs *regsp)
{
	int s = splserial();
	u_char stat, c;

	/* got a character from reading things earlier */
	if (com_readaheadcount > 0) {
		int i;

		c = com_readahead[0];
		for (i = 1; i < com_readaheadcount; i++) {
			com_readahead[i-1] = com_readahead[i];
		}
		com_readaheadcount--;
		splx(s);
		return (c);
	}

	/* block until a character becomes available */
	while (!ISSET(stat = CSR_READ_1(regsp, COM_REG_LSR), LSR_RXRDY))
		;

	c = CSR_READ_1(regsp, COM_REG_RXDATA);
	stat = CSR_READ_1(regsp, COM_REG_IIR);
	{
		int cn_trapped = 0; /* unused */
#ifdef DDB
		extern int db_active;
		if (!db_active)
#endif
			cn_check_magic(dev, c, com_cnm_state);
	}
	splx(s);
	return (c);
}

void
com_common_putc(dev_t dev, struct com_regs *regsp, int c)
{
	int s = splserial();
	int cin, stat, timo;

	if (com_readaheadcount < MAX_READAHEAD
	     && ISSET(stat = CSR_READ_1(regsp, COM_REG_LSR), LSR_RXRDY)) {
		int cn_trapped = 0;
		cin = CSR_READ_1(regsp, COM_REG_RXDATA);
		stat = CSR_READ_1(regsp, COM_REG_IIR);
		cn_check_magic(dev, cin, com_cnm_state);
		com_readahead[com_readaheadcount++] = cin;
	}

	/* wait for any pending transmission to finish */
	timo = 150000;
	while (!ISSET(CSR_READ_1(regsp, COM_REG_LSR), LSR_TXRDY) && --timo)
		continue;

	CSR_WRITE_1(regsp, COM_REG_TXDATA, c);
	COM_BARRIER(regsp, BR | BW);

	splx(s);
}

/*
 * Initialize UART for use as console or KGDB line.
 */
int
cominit(struct com_regs *regsp, int rate, int frequency, int type,
    tcflag_t cflag)
{

	if (bus_space_map(regsp->cr_iot, regsp->cr_iobase, regsp->cr_nports, 0,
		&regsp->cr_ioh))
		return (ENOMEM); /* ??? */

	rate = comspeed(rate, frequency, type);
	if (type != COM_TYPE_AU1x00) {
		/* no EFR on alchemy */
		CSR_WRITE_1(regsp, COM_REG_LCR, LCR_EERS);
		CSR_WRITE_1(regsp, COM_REG_EFR, 0);
		CSR_WRITE_1(regsp, COM_REG_LCR, LCR_DLAB);
		CSR_WRITE_1(regsp, COM_REG_DLBL, rate & 0xff);
		CSR_WRITE_1(regsp, COM_REG_DLBH, rate >> 8);
	} else {
		CSR_WRITE_1(regsp, COM_REG_DLBL, rate);
	}
	CSR_WRITE_1(regsp, COM_REG_LCR, cflag2lcr(cflag));
	CSR_WRITE_1(regsp, COM_REG_MCR, MCR_DTR | MCR_RTS);
	CSR_WRITE_1(regsp, COM_REG_FIFO,
	    FIFO_ENABLE | FIFO_RCV_RST | FIFO_XMT_RST | FIFO_TRIGGER_1);
#ifdef COM_PXA2X0
	if (type == COM_TYPE_PXA2x0)
		CSR_WRITE_1(regsp, COM_REG_IER, IER_EUART);
	else
#endif
		CSR_WRITE_1(regsp, COM_REG_IER, 0);

	return (0);
}

/*
 * Following are all routines needed for COM to act as console
 */
struct consdev comcons = {
	NULL, NULL, comcngetc, comcnputc, comcnpollc, NULL, NULL, NULL,
	NODEV, CN_NORMAL
};


int
comcnattach1(struct com_regs *regsp, int rate, int frequency, int type,
    tcflag_t cflag)
{
	int res;

	comconsregs = *regsp;

	res = cominit(&comconsregs, rate, frequency, type, cflag);
	if (res)
		return (res);

	cn_tab = &comcons;
	cn_init_magic(&com_cnm_state);
	cn_set_magic("\047\001"); /* default magic is BREAK */

	comconsrate = rate;
	comconscflag = cflag;

	return (0);
}

int
comcnattach(bus_space_tag_t iot, bus_addr_t iobase, int rate, int frequency,
    int type, tcflag_t cflag)
{
	struct com_regs	regs;

	memset(&regs, 0, sizeof regs);
	regs.cr_iot = iot;
	regs.cr_iobase = iobase;
	regs.cr_nports = COM_NPORTS;
#ifdef	COM_REGMAP
	memcpy(regs.cr_map, com_std_map, sizeof (regs.cr_map));
#endif

	return comcnattach1(&regs, rate, frequency, type, cflag);
}

int
comcngetc(dev_t dev)
{

	return (com_common_getc(dev, &comconsregs));
}

/*
 * Console kernel output character routine.
 */
void
comcnputc(dev_t dev, int c)
{

	com_common_putc(dev, &comconsregs, c);
}

void
comcnpollc(dev_t dev, int on)
{

}

#ifdef KGDB
int
com_kgdb_attach1(struct com_regs *regsp, int rate, int frequency, int type,
    tcflag_t cflag)
{
	int res;

	if (regsp->cr_iot == comconsregs.cr_iot &&
	    regsp->cr_iobase == comconsregs.cr_iobase) {
#if !defined(DDB)
		return (EBUSY); /* cannot share with console */
#else
		comkgdbregs = *regsp;
		comkgdbregs.cr_ioh = comconsregs.cr_ioh;
#endif
	} else {
		comkgdbregs = *regsp;
		res = cominit(&comkgdbregs, rate, frequency, type, cflag);
		if (res)
			return (res);

		/*
		 * XXXfvdl this shouldn't be needed, but the cn_magic goo
		 * expects this to be initialized
		 */
		cn_init_magic(&com_cnm_state);
		cn_set_magic("\047\001");
	}

	kgdb_attach(com_kgdb_getc, com_kgdb_putc, NULL);
	kgdb_dev = 123; /* unneeded, only to satisfy some tests */

	return (0);
}

int
com_kgdb_attach(bus_space_tag_t iot, bus_addr_t iobase, int rate,
    int frequency, int type, tcflag_t cflag)
{
	struct com_regs regs;

	regs.cr_iot = iot;
	regs.cr_nports = COM_NPORTS;
	regs.cr_iobase = iobase;
#ifdef COM_REGMAP
	memcpy(regs.cr_map, com_std_map, sizeof (regs.cr_map));
#endif

	return com_kgdb_attach1(&regs, rate, frequency, type, cflag);
}

/* ARGSUSED */
int
com_kgdb_getc(void *arg)
{

	return (com_common_getc(NODEV, &comkgdbregs));
}

/* ARGSUSED */
void
com_kgdb_putc(void *arg, int c)
{

	com_common_putc(NODEV, &comkgdbregs, c);
}
#endif /* KGDB */

/* helper function to identify the com ports used by
 console or KGDB (and not yet autoconf attached) */
int
com_is_console(bus_space_tag_t iot, bus_addr_t iobase, bus_space_handle_t *ioh)
{
	bus_space_handle_t help;

	if (!comconsattached &&
	    iot == comconsregs.cr_iot && iobase == comconsregs.cr_iobase)
		help = comconsregs.cr_ioh;
#ifdef KGDB
	else if (!com_kgdb_attached &&
	    iot == comkgdbregs.cr_iot && iobase == comkgdbregs.cr_iobase)
		help = comkgdbregs.cr_ioh;
#endif
	else
		return (0);

	if (ioh)
		*ioh = help;
	return (1);
}

/*
 * this routine exists to serve as a shutdown hook for systems that
 * have firmware which doesn't interact properly with a com device in
 * FIFO mode.
 */
void
com_cleanup(void *arg)
{
	struct com_softc *sc = arg;

	if (ISSET(sc->sc_hwflags, COM_HW_FIFO))
		CSR_WRITE_1(&sc->sc_regs, COM_REG_FIFO, 0);
}

bool
com_resume(device_t dev)
{
	struct com_softc *sc = device_private(dev);

	mutex_spin_enter(&sc->sc_lock);
	com_loadchannelregs(sc);
	mutex_spin_exit(&sc->sc_lock);

	return true;
}

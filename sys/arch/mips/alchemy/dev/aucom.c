/*	$NetBSD: aucom.c,v 1.14 2004/05/01 19:03:59 thorpej Exp $	*/
/*	 NetBSD: com.c,v 1.222 2003/11/08 02:54:47 simonb Exp	*/

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
 *
 * XXX: hacked to work with almost 16550-alike Alchemy Au1X00 on-chip uarts
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aucom.c,v 1.14 2004/05/01 19:03:59 thorpej Exp $");

#include "opt_com.h"
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
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/timepps.h>
#include <sys/vnode.h>

#include <machine/intr.h>
#include <machine/bus.h>

#ifdef COM_AU1x00
#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/dev/aucomreg.h>
#include <mips/alchemy/dev/aucomvar.h>
#else	/* COM_AU1x00 */
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#include <dev/ic/ns16550reg.h>
#include <dev/ic/st16650reg.h>
#ifdef COM_HAYESP
#include <dev/ic/hayespreg.h>
#endif
#endif	/* COM_AU1x00 */

#define	com_lcr	com_cfcr
#include <dev/cons.h>

#ifdef COM_AU1x00
/* Renamed local functions (from cdev_decl) */
#define	com_cdevsw	aucom_cdevsw
#define	comopen		aucomopen
#define	comclose	aucomclose
#define	comread		aucomread
#define	comwrite	aucomwrite
#define	comioctl	aucomioctl
#define	comstop		aucomstop
#define	comtty		aucomtty
#define	compoll		aucompoll

/* Renamed local functions (could be made static) */
#define	com_config		aucom_config
#define	com_shutdown		aucom_shutdown
#define	comspeed		aucomspeed
#define	cflag2lcr		aucflag2lcr
#define	comparam		aucomparam
#define	comstart		aucomstart
#define	comhwiflow		aucomhwiflow
#define	com_loadchannelregs	aucom_loadchannelregs
#define	com_hwiflow		aucom_hwiflow
#define	com_break		aucom_break
#define	com_modem		aucom_modem
#define	tiocm_to_com		autiocm_to_com
#define	com_to_tiocm		aucom_to_tiocm
#define	com_iflush		aucom_iflush
#define	com_common_getc		aucom_common_getc
#define	com_common_putc		aucom_common_putc
#define	cominit			aucominit
#define	comcngetc		aucomcngetc
#define	comcnputc		aucomcnputc
#define	comcnpollc		aucomcnpollc
#define	comsoft			aucomsoft
#define	comdiag			aucomdiag
#define	comstatus		aucomstatus
#define	comprobeHAYESP		aucomprobeHAYESP
#endif	/* COM_AU1x00 */

#ifdef COM_HAYESP
int comprobeHAYESP(bus_space_handle_t hayespioh, struct com_softc *sc);
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

int	com_common_getc(dev_t, bus_space_tag_t, bus_space_handle_t);
void	com_common_putc(dev_t, bus_space_tag_t, bus_space_handle_t, int);

int	cominit(bus_space_tag_t, bus_addr_t, int, int, int, tcflag_t,
	    bus_space_handle_t *);

int	comcngetc(dev_t);
void	comcnputc(dev_t, int);
void	comcnpollc(dev_t, int);

#define	integrate	static inline
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
void 	comsoft(void *);
#else
#ifndef __NO_SOFT_SERIAL_INTERRUPT
void 	comsoft(void);
#else
void 	comsoft(void *);
static struct callout comsoft_callout = CALLOUT_INITIALIZER;
#endif
#endif
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

static bus_addr_t	comconsaddr;
static bus_space_tag_t comconstag;
static bus_space_handle_t comconsioh;
static int	comconsattached;
static int comconsrate;
static tcflag_t comconscflag;
static struct cnm_state com_cnm_state;

static int ppscap =
	PPS_TSFMT_TSPEC |
	PPS_CAPTUREASSERT | 
	PPS_CAPTURECLEAR |
#ifdef  PPS_SYNC 
	PPS_HARDPPSONASSERT | PPS_HARDPPSONCLEAR |
#endif	/* PPS_SYNC */
	PPS_OFFSETASSERT | PPS_OFFSETCLEAR;

#ifndef __HAVE_GENERIC_SOFT_INTERRUPTS
#ifdef __NO_SOFT_SERIAL_INTERRUPT
volatile int	com_softintr_scheduled;
#endif
#endif

#ifdef KGDB
#include <sys/kgdb.h>

static bus_addr_t com_kgdb_addr;
static bus_space_tag_t com_kgdb_iot;
static bus_space_handle_t com_kgdb_ioh;
static int com_kgdb_attached;

int	com_kgdb_getc(void *);
void	com_kgdb_putc(void *, int);
#endif /* KGDB */

#define	COMUNIT_MASK	0x7ffff
#define	COMDIALOUT_MASK	0x80000

#define	COMUNIT(x)	(minor(x) & COMUNIT_MASK)
#define	COMDIALOUT(x)	(minor(x) & COMDIALOUT_MASK)

#define	COM_ISALIVE(sc)	((sc)->enabled != 0 && \
			 ISSET((sc)->sc_dev.dv_flags, DVF_ACTIVE))

#define	BR	BUS_SPACE_BARRIER_READ
#define	BW	BUS_SPACE_BARRIER_WRITE
#define COM_BARRIER(t, h, f) bus_space_barrier((t), (h), 0, COM_NPORTS, (f))

#if (defined(MULTIPROCESSOR) || defined(LOCKDEBUG)) && defined(COM_MPLOCK)

#define COM_LOCK(sc) simple_lock(&(sc)->sc_lock)
#define COM_UNLOCK(sc) simple_unlock(&(sc)->sc_lock)

#else

#define COM_LOCK(sc)
#define COM_UNLOCK(sc)

#endif

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

void comstatus(struct com_softc *, char *);
void
comstatus(struct com_softc *sc, char *str)
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
comprobe1(bus_space_tag_t iot, bus_space_handle_t ioh)
{

	/* force access to id reg */
	bus_space_write_1(iot, ioh, com_lcr, LCR_8BITS);
	bus_space_write_1(iot, ioh, com_iir, 0);
	if ((bus_space_read_1(iot, ioh, com_lcr) != LCR_8BITS) ||
	    (bus_space_read_1(iot, ioh, com_iir) & 0x38))
		return (0);

	return (1);
}

#ifdef COM_HAYESP
int
comprobeHAYESP(bus_space_handle_t hayespioh, struct com_softc *sc)
{
	char	val, dips;
	int	combaselist[] = { 0x3f8, 0x2f8, 0x3e8, 0x2e8 };
	bus_space_tag_t iot = sc->sc_iot;

	/*
	 * Hayes ESP cards have two iobases.  One is for compatibility with
	 * 16550 serial chips, and at the same ISA PC base addresses.  The
	 * other is for ESP-specific enhanced features, and lies at a
	 * different addressing range entirely (0x140, 0x180, 0x280, or 0x300).
	 */

	/* Test for ESP signature */
	if ((bus_space_read_1(iot, hayespioh, 0) & 0xf3) == 0)
		return (0);

	/*
	 * ESP is present at ESP enhanced base address; unknown com port
	 */

	/* Get the dip-switch configurations */
	bus_space_write_1(iot, hayespioh, HAYESP_CMD1, HAYESP_GETDIPS);
	dips = bus_space_read_1(iot, hayespioh, HAYESP_STATUS1);

	/* Determine which com port this ESP card services: bits 0,1 of  */
	/*  dips is the port # (0-3); combaselist[val] is the com_iobase */
	if (sc->sc_iobase != combaselist[dips & 0x03])
		return (0);

	printf(": ESP");

 	/* Check ESP Self Test bits. */
	/* Check for ESP version 2.0: bits 4,5,6 == 010 */
	bus_space_write_1(iot, hayespioh, HAYESP_CMD1, HAYESP_GETTEST);
	val = bus_space_read_1(iot, hayespioh, HAYESP_STATUS1); /* Clear reg1 */
	val = bus_space_read_1(iot, hayespioh, HAYESP_STATUS2);
	if ((val & 0x70) < 0x20) {
		printf("-old (%o)", val & 0x70);
		/* we do not support the necessary features */
		return (0);
	}

	/* Check for ability to emulate 16550: bit 8 == 1 */
	if ((dips & 0x80) == 0) {
		printf(" slave");
		/* XXX Does slave really mean no 16550 support?? */
		return (0);
	}

	/*
	 * If we made it this far, we are a full-featured ESP v2.0 (or
	 * better), at the correct com port address.
	 */

	sc->sc_type = COM_TYPE_HAYESP;
	printf(", 1024 byte fifo\n");
	return (1);
}
#endif

static void
com_enable_debugport(struct com_softc *sc)
{
	int s;

	/* Turn on line break interrupt, set carrier. */
	s = splserial();
	COM_LOCK(sc);
	sc->sc_ier = IER_ERXRDY;
#ifdef COM_PXA2X0
	if (sc->sc_type == COM_TYPE_PXA2x0)
		sc->sc_ier |= IER_EUART | IER_ERXTOUT;
#endif
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, com_ier, sc->sc_ier);
	SET(sc->sc_mcr, MCR_DTR | MCR_RTS);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, com_mcr, sc->sc_mcr);
	COM_UNLOCK(sc);
	splx(s);
}

void
com_attach_subr(struct com_softc *sc)
{
	bus_addr_t iobase = sc->sc_iobase;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct tty *tp;
#ifdef COM_16650
	u_int8_t lcr;
#endif
#ifdef COM_HAYESP
	int	hayesp_ports[] = { 0x140, 0x180, 0x280, 0x300, 0 };
	int	*hayespp;
#endif
	const char *fifo_msg = NULL;

	callout_init(&sc->sc_diag_callout);
#if (defined(MULTIPROCESSOR) || defined(LOCKDEBUG)) && defined(COM_MPLOCK)
	simple_lock_init(&sc->sc_lock);
#endif

	/* Disable interrupts before configuring the device. */
#ifdef COM_PXA2X0
	if (sc->sc_type == COM_TYPE_PXA2x0)
		sc->sc_ier = IER_EUART;
	else
#endif
		sc->sc_ier = 0;
	bus_space_write_1(iot, ioh, com_ier, sc->sc_ier);

	if (iot == comconstag && iobase == comconsaddr) {
		comconsattached = 1;

		/* Make sure the console is always "hardwired". */
		delay(1000);			/* wait for output to finish */
		SET(sc->sc_hwflags, COM_HW_CONSOLE);
		SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);
	}

#ifdef COM_HAYESP
	sc->sc_prescaler = 0;			/* set prescaler to x1. */

	/* Look for a Hayes ESP board. */
	for (hayespp = hayesp_ports; *hayespp != 0; hayespp++) {
		bus_space_handle_t hayespioh;

#define	HAYESP_NPORTS	8			/* XXX XXX XXX ??? ??? ??? */
		if (bus_space_map(iot, *hayespp, HAYESP_NPORTS, 0, &hayespioh))
			continue;
		if (comprobeHAYESP(hayespioh, sc)) {
			sc->sc_hayespioh = hayespioh;
			sc->sc_fifolen = 1024;

			break;
		}
		bus_space_unmap(iot, hayespioh, HAYESP_NPORTS);
	}
	/* No ESP; look for other things. */
	if (sc->sc_type != COM_TYPE_HAYESP) {
#endif
#ifdef COM_AU1x00
	if (sc->sc_type == COM_TYPE_AU1x00) {
		fifo_msg = "Au1X00 UART, working fifo";
		sc->sc_fifolen = 16;
		SET(sc->sc_hwflags, COM_HW_FIFO);
	} else {
#endif /* !COM_AU1x00 */
	sc->sc_fifolen = 1;
	/* look for a NS 16550AF UART with FIFOs */
	bus_space_write_1(iot, ioh, com_fifo,
	    FIFO_ENABLE | FIFO_RCV_RST | FIFO_XMT_RST | FIFO_TRIGGER_14);
	delay(100);
	if (ISSET(bus_space_read_1(iot, ioh, com_iir), IIR_FIFO_MASK)
	    == IIR_FIFO_MASK)
		if (ISSET(bus_space_read_1(iot, ioh, com_fifo), FIFO_TRIGGER_14)
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
			lcr = bus_space_read_1(iot, ioh, com_lcr);
			bus_space_write_1(iot, ioh, com_lcr, LCR_EERS);
			bus_space_write_1(iot, ioh, com_efr, 0);
			if (bus_space_read_1(iot, ioh, com_efr) == 0) {
				bus_space_write_1(iot, ioh, com_lcr,
				    lcr | LCR_DLAB);
				if (bus_space_read_1(iot, ioh, com_efr) == 0) {
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
			bus_space_write_1(iot, ioh, com_lcr, lcr);
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
#ifdef COM_AU1x00
	}
#endif /* !COM_AU1x00 */
	bus_space_write_1(iot, ioh, com_fifo, 0);
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
#ifdef COM_HAYESP
	}
#endif

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

		tp->t_dev = cn_tab->cn_dev = makedev(maj, sc->sc_dev.dv_unit);

		aprint_normal("%s: console\n", sc->sc_dev.dv_xname);
	}

#ifdef KGDB
	/*
	 * Allow kgdb to "take over" this port.  If this is
	 * not the console and is the kgdb device, it has
	 * exclusive use.  If it's the console _and_ the
	 * kgdb device, it doesn't.
	 */
	if (iot == com_kgdb_iot && iobase == com_kgdb_addr) {
		if (!ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
			com_kgdb_attached = 1;

			SET(sc->sc_hwflags, COM_HW_KGDB);
		}
		aprint_normal("%s: kgdb\n", sc->sc_dev.dv_xname);
	}
#endif

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	sc->sc_si = softintr_establish(IPL_SOFTSERIAL, comsoft, sc);
#endif

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
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	/* Disable interrupts before configuring the device. */
#ifdef COM_PXA2X0
	if (sc->sc_type == COM_TYPE_PXA2x0)
		sc->sc_ier = IER_EUART;
	else
#endif
		sc->sc_ier = 0;
	bus_space_write_1(iot, ioh, com_ier, sc->sc_ier);

#ifdef COM_HAYESP
	/* Look for a Hayes ESP board. */
	if (sc->sc_type == COM_TYPE_HAYESP) {
		sc->sc_fifolen = 1024;

		/* Set 16550 compatibility mode */
		bus_space_write_1(iot, sc->sc_hayespioh, HAYESP_CMD1,
				  HAYESP_SETMODE);
		bus_space_write_1(iot, sc->sc_hayespioh, HAYESP_CMD2, 
				  HAYESP_MODE_FIFO|HAYESP_MODE_RTS|
				  HAYESP_MODE_SCALE);

		/* Set RTS/CTS flow control */
		bus_space_write_1(iot, sc->sc_hayespioh, HAYESP_CMD1,
				  HAYESP_SETFLOWTYPE);
		bus_space_write_1(iot, sc->sc_hayespioh, HAYESP_CMD2,
				  HAYESP_FLOW_RTS);
		bus_space_write_1(iot, sc->sc_hayespioh, HAYESP_CMD2,
				  HAYESP_FLOW_CTS);

		/* Set flow control levels */
		bus_space_write_1(iot, sc->sc_hayespioh, HAYESP_CMD1,
				  HAYESP_SETRXFLOW);
		bus_space_write_1(iot, sc->sc_hayespioh, HAYESP_CMD2, 
				  HAYESP_HIBYTE(HAYESP_RXHIWMARK));
		bus_space_write_1(iot, sc->sc_hayespioh, HAYESP_CMD2,
				  HAYESP_LOBYTE(HAYESP_RXHIWMARK));
		bus_space_write_1(iot, sc->sc_hayespioh, HAYESP_CMD2,
				  HAYESP_HIBYTE(HAYESP_RXLOWMARK));
		bus_space_write_1(iot, sc->sc_hayespioh, HAYESP_CMD2,
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
	mn = self->dv_unit;
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

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	/* Unhook the soft interrupt handler. */
	softintr_disestablish(sc->sc_si);
#endif

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
	int s, rv = 0;

	s = splserial();
	COM_LOCK(sc);
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

	COM_UNLOCK(sc);	
	splx(s);
	return (rv);
}

void
com_shutdown(struct com_softc *sc)
{
	struct tty *tp = sc->sc_tty;
	int s;

	s = splserial();
	COM_LOCK(sc);	

	/* If we were asserting flow control, then deassert it. */
	SET(sc->sc_rx_flags, RX_IBUF_BLOCKED);
	com_hwiflow(sc);

	/* Clear any break condition set with TIOCSBRK. */
	com_break(sc, 0);

	/* Turn off PPS capture on last close. */
	sc->sc_ppsmask = 0;
	sc->ppsparam.mode = 0;

	/*
	 * Hang up if necessary.  Wait a bit, so the other side has time to
	 * notice even if we immediately open the port again.
	 * Avoid tsleeping above splhigh().
	 */
	if (ISSET(tp->t_cflag, HUPCL)) {
		com_modem(sc, 0);
		COM_UNLOCK(sc);
		splx(s);
		/* XXX tsleep will only timeout */
		(void) tsleep(sc, TTIPRI, ttclos, hz);
		s = splserial();
		COM_LOCK(sc);	
	}

	/* Turn off interrupts. */
	if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
		sc->sc_ier = IER_ERXRDY; /* interrupt on break */
#ifdef COM_PXA2X0
		if (sc->sc_type == COM_TYPE_PXA2x0)
			sc->sc_ier |= IER_ERXTOUT;
#endif
	} else
		sc->sc_ier = 0;

#ifdef COM_PXA2X0
	if (sc->sc_type == COM_TYPE_PXA2x0)
		sc->sc_ier |= IER_EUART;
#endif

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, com_ier, sc->sc_ier);

	if (sc->disable) {
#ifdef DIAGNOSTIC
		if (!sc->enabled)
			panic("com_shutdown: not enabled?");
#endif
		(*sc->disable)(sc);
		sc->enabled = 0;
	}
	COM_UNLOCK(sc);
	splx(s);
}

int
comopen(dev_t dev, int flag, int mode, struct proc *p)
{
	struct com_softc *sc;
	struct tty *tp;
	int s, s2;
	int error;

	sc = device_lookup(&com_cd, COMUNIT(dev));
	if (sc == NULL || !ISSET(sc->sc_hwflags, COM_HW_DEV_OK) ||
		sc->sc_rbuf == NULL)
		return (ENXIO);

	if (ISSET(sc->sc_dev.dv_flags, DVF_ACTIVE) == 0)
		return (ENXIO);

#ifdef KGDB
	/*
	 * If this is the kgdb port, no other use is permitted.
	 */
	if (ISSET(sc->sc_hwflags, COM_HW_KGDB))
		return (EBUSY);
#endif

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
		COM_LOCK(sc);

		if (sc->enable) {
			if ((*sc->enable)(sc)) {
				COM_UNLOCK(sc);
				splx(s2);
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
#ifdef COM_PXA2X0
		if (sc->sc_type == COM_TYPE_PXA2x0)
			sc->sc_ier |= IER_EUART | IER_ERXTOUT;
#endif
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, com_ier, sc->sc_ier);

		/* Fetch the current modem control status, needed later. */
		sc->sc_msr = bus_space_read_1(sc->sc_iot, sc->sc_ioh, com_msr);

		/* Clear PPS capture state on first open. */
		sc->sc_ppsmask = 0;
		sc->ppsparam.mode = 0;

		COM_UNLOCK(sc);
		splx(s2);

		/*
		 * Initialize the termios status to the defaults.  Add in the
		 * sticky bits from TIOCSFLAGS.
		 */
		t.c_ispeed = 0;
		if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
			t.c_ospeed = comconsrate;
			t.c_cflag = comconscflag;
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
		/* Make sure comparam() will do something. */
		tp->t_ospeed = 0;
		(void) comparam(tp, &t);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		ttychars(tp);
		ttsetwater(tp);

		s2 = splserial();
		COM_LOCK(sc);

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

		COM_UNLOCK(sc);
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
		com_shutdown(sc);
	}

	return (error);
}
 
int
comclose(dev_t dev, int flag, int mode, struct proc *p)
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
compoll(dev_t dev, int events, struct proc *p)
{
	struct com_softc *sc = device_lookup(&com_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);
 
	return ((*tp->t_linesw->l_poll)(tp, events, p));
}

struct tty *
comtty(dev_t dev)
{
	struct com_softc *sc = device_lookup(&com_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;

	return (tp);
}

int
comioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct com_softc *sc = device_lookup(&com_cd, COMUNIT(dev));
	struct tty *tp = sc->sc_tty;
	int error;
	int s;

	if (COM_ISALIVE(sc) == 0)
		return (EIO);

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, p);
	if (error != EPASSTHROUGH)
		return (error);

	error = ttioctl(tp, cmd, data, flag, p);
	if (error != EPASSTHROUGH)
		return (error);

	error = 0;

	s = splserial();
	COM_LOCK(sc);	

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
		error = suser(p->p_ucred, &p->p_acflag); 
		if (error)
			break;
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
		sc->sc_ppsassert = -1;
		sc->sc_ppsclear = 0;
		TIMESPEC_TO_TIMEVAL((struct timeval *)data, 
		    &sc->ppsinfo.clear_timestamp);
#endif
		break;

	default:
		error = EPASSTHROUGH;
		break;
	}

	COM_UNLOCK(sc);
	splx(s);

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
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	softintr_schedule(sc->sc_si);
#else
#ifndef __NO_SOFT_SERIAL_INTERRUPT
	setsoftserial();
#else
	if (!com_softintr_scheduled) {
		com_softintr_scheduled = 1;
		callout_reset(&comsoft_callout, 1, comsoft, NULL);
	}
#endif
#endif
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

#ifdef COM_PXA2X0
	if (sc->sc_type == COM_TYPE_PXA2x0) {
		if ((sc->sc_ier & 0x0f) != 0)
			SET(ttybits, TIOCM_LE);
	} else
#endif
	if ((sc->sc_ier & 0xbf) != 0)
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
	int s;

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

	s = splserial();
	COM_LOCK(sc);	

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
#ifdef com_efr	/* XXX: no com_efr on Au1X00 */
		sc->sc_efr = EFR_AUTORTS | EFR_AUTOCTS;
#endif
	} else if (ISSET(t->c_cflag, MDMBUF)) {
		/*
		 * For DTR/DCD flow control, make sure we don't toggle DTR for
		 * carrier detection.
		 */
		sc->sc_mcr_dtr = 0;
		sc->sc_mcr_rts = MCR_DTR;
		sc->sc_msr_cts = MSR_DCD;
#ifdef com_efr	/* XXX: no com_efr on Au1X00 */
		sc->sc_efr = 0;
#endif
	} else {
		/*
		 * If no flow control, then always set RTS.  This will make
		 * the other side happy if it mistakenly thinks we're doing
		 * RTS/CTS flow control.
		 */
		sc->sc_mcr_dtr = MCR_DTR | MCR_RTS;
		sc->sc_mcr_rts = 0;
		sc->sc_msr_cts = 0;
#ifdef com_efr	/* XXX: no com_efr on Au1X00 */
		sc->sc_efr = 0;
#endif
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
#ifdef COM_HAYESP
	if (sc->sc_type == COM_TYPE_HAYESP)
		sc->sc_fifo = FIFO_DMA_MODE | FIFO_ENABLE | FIFO_TRIGGER_8;
	else
#endif
	if (ISSET(sc->sc_hwflags, COM_HW_FIFO))
		sc->sc_fifo = FIFO_ENABLE |
		    (t->c_ospeed <= 1200 ? FIFO_TRIGGER_1 :
		     t->c_ospeed <= 38400 ? FIFO_TRIGGER_8 : FIFO_TRIGGER_4);
	else
		sc->sc_fifo = 0;

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

	COM_UNLOCK(sc);
	splx(s);

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
	while (ISSET(bus_space_read_1(iot, ioh, com_lsr), LSR_RXRDY)
	    && --timo)
#ifdef DIAGNOSTIC
		reg =
#else
		    (void)
#endif
		    bus_space_read_1(iot, ioh, com_rxdata);
#ifdef DIAGNOSTIC
	if (!timo)
		printf("%s: com_iflush timeout %02x\n", sc->sc_dev.dv_xname,
		       reg);
#endif
}

void
com_loadchannelregs(struct com_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	/* XXXXX necessary? */
	com_iflush(sc);

#ifdef COM_PXA2X0
	if (sc->sc_type == COM_TYPE_PXA2x0)
		bus_space_write_1(iot, ioh, com_ier, IER_EUART);
	else
#endif
		bus_space_write_1(iot, ioh, com_ier, 0);

#ifdef com_efr	/* XXX: no com_efr on Au1X00 */
	if (ISSET(sc->sc_hwflags, COM_HW_FLOW)) {
		bus_space_write_1(iot, ioh, com_lcr, LCR_EERS);
		bus_space_write_1(iot, ioh, com_efr, sc->sc_efr);
	}
#endif
#ifdef com_dlb	/* XXXsimonb: Au1X00 has single clock divisor register */
	bus_space_write_2(iot, ioh, com_dlb, sc->sc_dlbl + (sc->sc_dlbh << 8));
#else
	bus_space_write_1(iot, ioh, com_lcr, sc->sc_lcr | LCR_DLAB);
	bus_space_write_1(iot, ioh, com_dlbl, sc->sc_dlbl);
	bus_space_write_1(iot, ioh, com_dlbh, sc->sc_dlbh);
#endif
	bus_space_write_1(iot, ioh, com_lctl, sc->sc_lcr);
	bus_space_write_1(iot, ioh, com_mcr, sc->sc_mcr_active = sc->sc_mcr);
	bus_space_write_1(iot, ioh, com_fifo, sc->sc_fifo);
#ifdef COM_HAYESP
	if (sc->sc_type == COM_TYPE_HAYESP) {
		bus_space_write_1(iot, sc->sc_hayespioh, HAYESP_CMD1,
		    HAYESP_SETPRESCALER);
		bus_space_write_1(iot, sc->sc_hayespioh, HAYESP_CMD2,
		    sc->sc_prescaler);
	}
#endif

	bus_space_write_1(iot, ioh, com_ier, sc->sc_ier);
}

int
comhwiflow(struct tty *tp, int block)
{
	struct com_softc *sc = device_lookup(&com_cd, COMUNIT(tp->t_dev));
	int s;

	if (COM_ISALIVE(sc) == 0)
		return (0);

	if (sc->sc_mcr_rts == 0)
		return (0);

	s = splserial();
	COM_LOCK(sc);
	
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

	COM_UNLOCK(sc);
	splx(s);
	return (1);
}
	
/*
 * (un)block input via hw flowcontrol
 */
void
com_hwiflow(struct com_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	if (sc->sc_mcr_rts == 0)
		return;

	if (ISSET(sc->sc_rx_flags, RX_ANY_BLOCK)) {
		CLR(sc->sc_mcr, sc->sc_mcr_rts);
		CLR(sc->sc_mcr_active, sc->sc_mcr_rts);
	} else {
		SET(sc->sc_mcr, sc->sc_mcr_rts);
		SET(sc->sc_mcr_active, sc->sc_mcr_rts);
	}
	bus_space_write_1(iot, ioh, com_mcr, sc->sc_mcr_active);
}


void
comstart(struct tty *tp)
{
	struct com_softc *sc = device_lookup(&com_cd, COMUNIT(tp->t_dev));
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int s;

	if (COM_ISALIVE(sc) == 0)
		return;

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
		COM_LOCK(sc);

		sc->sc_tba = tba;
		sc->sc_tbc = tbc;
	}

	SET(tp->t_state, TS_BUSY);
	sc->sc_tx_busy = 1;

	/* Enable transmit completion interrupts if necessary. */
	if (!ISSET(sc->sc_ier, IER_ETXRDY)) {
		SET(sc->sc_ier, IER_ETXRDY);
		bus_space_write_1(iot, ioh, com_ier, sc->sc_ier);
	}

	/* Output the first chunk of the contiguous buffer. */
	if (!ISSET(sc->sc_hwflags, COM_HW_NO_TXPRELOAD)) {
		u_int n;

		n = sc->sc_tbc;
		if (n > sc->sc_fifolen)
			n = sc->sc_fifolen;
		bus_space_write_multi_1(iot, ioh, com_txdata, sc->sc_tba, n);
		sc->sc_tbc -= n;
		sc->sc_tba += n;
	}
	COM_UNLOCK(sc);
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
	int s;

	s = splserial();
	COM_LOCK(sc);
	if (ISSET(tp->t_state, TS_BUSY)) {
		/* Stop transmitting at the next chunk. */
		sc->sc_tbc = 0;
		sc->sc_heldtbc = 0;
		if (!ISSET(tp->t_state, TS_TTSTOP))
			SET(tp->t_state, TS_FLUSH);
	}
	COM_UNLOCK(sc);	
	splx(s);
}

void
comdiag(void *arg)
{
	struct com_softc *sc = arg;
	int overflows, floods;
	int s;

	s = splserial();
	COM_LOCK(sc);
	overflows = sc->sc_overflows;
	sc->sc_overflows = 0;
	floods = sc->sc_floods;
	sc->sc_floods = 0;
	sc->sc_errors = 0;
	COM_UNLOCK(sc);
	splx(s);

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
	int s;

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
		s = splserial();
		COM_LOCK(sc);
		
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
				bus_space_write_1(sc->sc_iot, sc->sc_ioh,
				    com_ier, sc->sc_ier);
			}
			if (ISSET(sc->sc_rx_flags, RX_IBUF_BLOCKED)) {
				CLR(sc->sc_rx_flags, RX_IBUF_BLOCKED);
				com_hwiflow(sc);
			}
		}
		COM_UNLOCK(sc);
		splx(s);
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
	int s;

	s = splserial();
	COM_LOCK(sc);
	msr = sc->sc_msr;
	delta = sc->sc_msr_delta;
	sc->sc_msr_delta = 0;
	COM_UNLOCK(sc);	
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

#ifdef COM_DEBUG
	if (com_debug)
		comstatus(sc, "com_stsoft");
#endif
}

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
void
comsoft(void *arg)
{
	struct com_softc *sc = arg;
	struct tty *tp;

	if (COM_ISALIVE(sc) == 0)
		return;

	{
#else
void
#ifndef __NO_SOFT_SERIAL_INTERRUPT
comsoft(void)
#else
comsoft(void *arg)
#endif
{
	struct com_softc	*sc;
	struct tty	*tp;
	int	unit;
#ifdef __NO_SOFT_SERIAL_INTERRUPT
	int s;

	s = splsoftserial();
	com_softintr_scheduled = 0;
#endif

	for (unit = 0; unit < com_cd.cd_ndevs; unit++) {
		sc = device_lookup(&com_cd, unit);
		if (sc == NULL || !ISSET(sc->sc_hwflags, COM_HW_DEV_OK))
			continue;

		if (COM_ISALIVE(sc) == 0)
			continue;

		tp = sc->sc_tty;
		if (tp == NULL)
			continue;
		if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0)
			continue;
#endif
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

#ifndef __HAVE_GENERIC_SOFT_INTERRUPTS
#ifdef __NO_SOFT_SERIAL_INTERRUPT
	splx(s);
#endif
#endif
}

#ifdef __ALIGN_BRACKET_LEVEL_FOR_CTAGS
	/* there has got to be a better way to do comsoft() */
}}
#endif

int
comintr(void *arg)
{
	struct com_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_char *put, *end;
	u_int cc;
	u_char lsr, iir;

	if (COM_ISALIVE(sc) == 0)
		return (0);

	COM_LOCK(sc);
	iir = bus_space_read_1(iot, ioh, com_iir);
	if (ISSET(iir, IIR_NOPEND)) {
		COM_UNLOCK(sc);
		return (0);
	}

	end = sc->sc_ebuf;
	put = sc->sc_rbput;
	cc = sc->sc_rbavail;

again:	do {
		u_char	msr, delta;

		lsr = bus_space_read_1(iot, ioh, com_lsr);
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
				put[0] = bus_space_read_1(iot, ioh, com_rxdata);
				put[1] = lsr;
				cn_check_magic(sc->sc_tty->t_dev,
					       put[0], com_cnm_state);
				if (cn_trapped) {
					lsr = bus_space_read_1(iot, ioh, com_lsr);
					if (!ISSET(lsr, LSR_RCV_MASK))
						break;

					continue;
				}
				put += 2;
				if (put >= end)
					put = sc->sc_rbuf;
				cc--;

				lsr = bus_space_read_1(iot, ioh, com_lsr);
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
				CLR(sc->sc_ier, IER_ERXRDY);
#ifdef COM_PXA2X0
				if (sc->sc_type == COM_TYPE_PXA2x0)
					CLR(sc->sc_ier, IER_ERXTOUT);
#endif
				bus_space_write_1(iot, ioh, com_ier,
				    sc->sc_ier);
			}
		} else {
			if ((iir & IIR_IMASK) == IIR_RXRDY) {
#ifdef COM_PXA2X0
				if (sc->sc_type == COM_TYPE_PXA2x0)
					bus_space_write_1(iot, ioh, com_ier,
					    IER_EUART);
				else
#endif
					bus_space_write_1(iot, ioh, com_ier, 0);
				delay(10);
				bus_space_write_1(iot, ioh, com_ier,sc->sc_ier);
				continue;
			}
		}

		msr = bus_space_read_1(iot, ioh, com_msr);
		delta = msr ^ sc->sc_msr;
		sc->sc_msr = msr;
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
#ifdef COM_DEBUG
				if (com_debug)
					comstatus(sc, "comintr  ");
#endif
			}

			sc->sc_st_check = 1;
		}
	} while (ISSET((iir = bus_space_read_1(iot, ioh, com_iir)), IIR_RXRDY)
	    || ((iir & IIR_IMASK) == 0));

	/*
	 * Done handling any receive interrupts. See if data can be
	 * transmitted as well. Schedule tx done event if no data left
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
			bus_space_write_multi_1(iot, ioh, com_txdata, sc->sc_tba, n);
			sc->sc_tbc -= n;
			sc->sc_tba += n;
		} else {
			/* Disable transmit completion interrupts if necessary. */
			if (ISSET(sc->sc_ier, IER_ETXRDY)) {
				CLR(sc->sc_ier, IER_ETXRDY);
				bus_space_write_1(iot, ioh, com_ier, sc->sc_ier);
			}
			if (sc->sc_tx_busy) {
				sc->sc_tx_busy = 0;
				sc->sc_tx_done = 1;
			}
		}
	}

	if (!ISSET((iir = bus_space_read_1(iot, ioh, com_iir)), IIR_NOPEND))
		goto again;

	COM_UNLOCK(sc);

	/* Wake up the poller. */
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	softintr_schedule(sc->sc_si);
#else
#ifndef __NO_SOFT_SERIAL_INTERRUPT
	setsoftserial();
#else
	if (!com_softintr_scheduled) {
		com_softintr_scheduled = 1;
		callout_reset(&comsoft_callout, 1, comsoft, NULL);
	}
#endif
#endif

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
com_common_getc(dev_t dev, bus_space_tag_t iot, bus_space_handle_t ioh)
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
	while (!ISSET(stat = bus_space_read_1(iot, ioh, com_lsr), LSR_RXRDY))
		;

	c = bus_space_read_1(iot, ioh, com_rxdata);
	stat = bus_space_read_1(iot, ioh, com_iir);
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
com_common_putc(dev_t dev, bus_space_tag_t iot, bus_space_handle_t ioh, int c)
{
	int s = splserial();
	int cin, stat, timo;

	if (com_readaheadcount < MAX_READAHEAD 
	     && ISSET(stat = bus_space_read_1(iot, ioh, com_lsr), LSR_RXRDY)) {
		int cn_trapped = 0;
		cin = bus_space_read_1(iot, ioh, com_rxdata);
		stat = bus_space_read_1(iot, ioh, com_iir);
		cn_check_magic(dev, cin, com_cnm_state);
		com_readahead[com_readaheadcount++] = cin;
	}

	/* wait for any pending transmission to finish */
	timo = 150000;
	while (!ISSET(bus_space_read_1(iot, ioh, com_lsr), LSR_TXRDY) && --timo)
		continue;

	bus_space_write_1(iot, ioh, com_txdata, c);
	COM_BARRIER(iot, ioh, BR | BW);

	/* wait for this transmission to complete */
	timo = 1500000;
	while (!ISSET(bus_space_read_1(iot, ioh, com_lsr), LSR_TXRDY) && --timo)
		continue;

	splx(s);
}

/*
 * Initialize UART for use as console or KGDB line.
 */
int
cominit(bus_space_tag_t iot, bus_addr_t iobase, int rate, int frequency,
    int type, tcflag_t cflag, bus_space_handle_t *iohp)
{
	bus_space_handle_t ioh;

	if (bus_space_map(iot, iobase, COM_NPORTS, 0, &ioh))
		return (ENOMEM); /* ??? */

#ifdef com_efr	/* XXX: no com_efr on Au1X00 */
	bus_space_write_1(iot, ioh, com_lcr, LCR_EERS);
	bus_space_write_1(iot, ioh, com_efr, 0);
	bus_space_write_1(iot, ioh, com_lcr, LCR_DLAB);
#endif
	rate = comspeed(rate, frequency, type);
#ifdef com_dlb	/* XXXsimonb: Au1X00 has single clock divisor register */
	bus_space_write_2(iot, ioh, com_dlb, rate);
#else
	bus_space_write_1(iot, ioh, com_dlbl, rate);
	bus_space_write_1(iot, ioh, com_dlbh, rate >> 8);
#endif
	bus_space_write_1(iot, ioh, com_lctl, cflag2lcr(cflag));
	bus_space_write_1(iot, ioh, com_mcr, MCR_DTR | MCR_RTS);
	bus_space_write_1(iot, ioh, com_fifo,
	    FIFO_ENABLE | FIFO_RCV_RST | FIFO_XMT_RST | FIFO_TRIGGER_1);
#ifdef COM_PXA2X0
	if (sc->sc_type == COM_TYPE_PXA2x0)
		bus_space_write_1(iot, ioh, com_ier, IER_EUART);
	else
#endif
		bus_space_write_1(iot, ioh, com_ier, 0);

	*iohp = ioh;
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
comcnattach(bus_space_tag_t iot, bus_addr_t iobase, int rate, int frequency,
    int type, tcflag_t cflag)
{
	int res;

	res = cominit(iot, iobase, rate, frequency, type, cflag, &comconsioh);
	if (res)
		return (res);

	cn_tab = &comcons;
	cn_init_magic(&com_cnm_state);
	cn_set_magic("\047\001"); /* default magic is BREAK */

	comconstag = iot;
	comconsaddr = iobase;
	comconsrate = rate;
	comconscflag = cflag;

	return (0);
}

int
comcngetc(dev_t dev)
{

	return (com_common_getc(dev, comconstag, comconsioh));
}

/*
 * Console kernel output character routine.
 */
void
comcnputc(dev_t dev, int c)
{

	com_common_putc(dev, comconstag, comconsioh, c);
}

void
comcnpollc(dev_t dev, int on)
{

}

#ifdef KGDB
int
com_kgdb_attach(bus_space_tag_t iot, bus_addr_t iobase, int rate,
    int frequency, int type, tcflag_t cflag)
{
	int res;

	if (iot == comconstag && iobase == comconsaddr) {
#if !defined(DDB)
		return (EBUSY); /* cannot share with console */
#else
		com_kgdb_ioh = comconsioh;
#endif
	} else {
		res = cominit(iot, iobase, rate, frequency, type, cflag,
			      &com_kgdb_ioh);
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

	com_kgdb_iot = iot;
	com_kgdb_addr = iobase;

	return (0);
}

/* ARGSUSED */
int
com_kgdb_getc(void *arg)
{

	return (com_common_getc(NODEV, com_kgdb_iot, com_kgdb_ioh));
}

/* ARGSUSED */
void
com_kgdb_putc(void *arg, int c)
{

	com_common_putc(NODEV, com_kgdb_iot, com_kgdb_ioh, c);
}
#endif /* KGDB */

/* helper function to identify the com ports used by
 console or KGDB (and not yet autoconf attached) */
int
com_is_console(bus_space_tag_t iot, bus_addr_t iobase, bus_space_handle_t *ioh)
{
	bus_space_handle_t help;

	if (!comconsattached &&
	    iot == comconstag && iobase == comconsaddr)
		help = comconsioh;
#ifdef KGDB
	else if (!com_kgdb_attached &&
	    iot == com_kgdb_iot && iobase == com_kgdb_addr)
		help = com_kgdb_ioh;
#endif
	else
		return (0);

	if (ioh)
		*ioh = help;
	return (1);
}

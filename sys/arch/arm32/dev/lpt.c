/* $NetBSD: lpt.c,v 1.9 1996/06/13 21:51:53 cgd Exp $ */

/*
 * Copyright (c) 1994 Matthias Pfaller.
 * Copyright (c) 1993, 1994 Charles Hannum.
 * Copyright (c) 1990 William F. Jolitz, TeleMuse
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
 *	This software is a component of "386BSD" developed by 
 *	William F. Jolitz, TeleMuse.
 * 4. Neither the name of the developer nor the name "386BSD"
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS A COMPONENT OF 386BSD DEVELOPED BY WILLIAM F. JOLITZ 
 * AND IS INTENDED FOR RESEARCH AND EDUCATIONAL PURPOSES ONLY. THIS 
 * SOFTWARE SHOULD NOT BE CONSIDERED TO BE A COMMERCIAL PRODUCT. 
 * THE DEVELOPER URGES THAT USERS WHO REQUIRE A COMMERCIAL PRODUCT 
 * NOT MAKE USE OF THIS WORK.
 *
 * FOR USERS WHO WISH TO UNDERSTAND THE 386BSD SYSTEM DEVELOPED
 * BY WILLIAM F. JOLITZ, WE RECOMMEND THE USER STUDY WRITTEN 
 * REFERENCES SUCH AS THE  "PORTING UNIX TO THE 386" SERIES 
 * (BEGINNING JANUARY 1991 "DR. DOBBS JOURNAL", USA AND BEGINNING 
 * JUNE 1991 "UNIX MAGAZIN", GERMANY) BY WILLIAM F. JOLITZ AND 
 * LYNNE GREER JOLITZ, AS WELL AS OTHER BOOKS ON UNIX AND THE 
 * ON-LINE 386BSD USER MANUAL BEFORE USE. A BOOK DISCUSSING THE INTERNALS 
 * OF 386BSD ENTITLED "386BSD FROM THE INSIDE OUT" WILL BE AVAILABLE LATE 1992.
 *
 * THIS SOFTWARE IS PROVIDED BY THE DEVELOPER ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE DEVELOPER BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Device Driver for AT parallel printer port
 */

/*
 * PLIP code Copyright (c) 1994 Matthias Pfaller from pc532/dev/lpt.c
 *
 * Merged into the arm32/mainbus/lpt.c by Mark Brinicombe
 */

/* NOTE: The PLIP code does not work at the moment
 *
 * The lpt code needed upgrading to fix some bugs in the previous version
 * The new version uses the bus_io_*() macros which makes this code
 * incompatible with the previous plip driver.
 * The plip code is being upgraded but is not complete.
 *
 * Normally I would hold off committing until I had finished but
 * more of people want a working lpt driver than the plip driver.
 *
 * Thus if you use plip stick to the old version of this driver for now.
 */
#undef PLIP

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/syslog.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/katelib.h>
#include <machine/irqhandler.h>
#include <machine/io.h>
#include <arm32/mainbus/lptreg.h>
#include <arm32/mainbus/mainbus.h>

#if defined(INET) && defined(PLIP)
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/netisr.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#endif

#define	TIMEOUT		hz*16	/* wait up to 16 seconds for a ready */
#define	STEP		hz/4

#define	LPTPRI		(PZERO+8)
#define	LPT_BSIZE	1024

#if !defined(DEBUG) || !defined(notdef)
#define LPRINTF(a)
#else
#define LPRINTF		if (lptdebug) printf a
int lptdebug = 1;
#endif

#if defined(INET) && defined(PLIP)
#ifndef PLIPMTU			/* MTU for the plip# interfaces */
#if defined(COMPAT_PLIP10)
#define	PLIPMTU		1600				/* Linux 1.0.x */
#elif defined(COMPAT_PLIP11)
#define	PLIPMTU		(ETHERMTU - ifp->if_hdrlen)	/* Linux 1.1.x */
#else
#define PLIPMTU		ETHERMTU			/* Linux 1.3.x */
#endif
#endif

#ifndef PLIPMXSPIN1		/* DELAY factor for the plip# interfaces */
#define	PLIPMXSPIN1	2000	/* Spinning for remote intr to happen */
#endif

#ifndef PLIPMXSPIN2		/* DELAY factor for the plip# interfaces */
#define	PLIPMXSPIN2	6000	/* Spinning for remote handshake to happen */
#endif

#ifndef PLIPMXERRS		/* Max errors before !RUNNING */
#define	PLIPMXERRS	20
#endif
#ifndef PLIPMXRETRY
#define PLIPMXRETRY	20	/* Max number of retransmits */
#endif
#ifndef PLIPRETRY
#define PLIPRETRY	hz/50	/* Time between retransmits */
#endif
#endif

struct lpt_softc {
	struct device sc_dev;
	irqhandler_t sc_ih;

	size_t sc_count;
	struct buf *sc_inbuf;
	u_char *sc_cp;
	int sc_spinmax;
	int sc_iobase;
	bus_chipset_tag_t sc_bc;
	bus_io_handle_t sc_ioh;
	int sc_irq;
	u_char sc_state;
#define	LPT_OPEN	0x01	/* device is open */
#define	LPT_OBUSY	0x02	/* printer is busy doing output */
#define	LPT_INIT	0x04	/* waiting to initialize for open */
#define LPT_PLIP	0x08	/* busy with PLIP */
	u_char sc_flags;
#define	LPT_AUTOLF	0x20	/* automatic LF on CR */
#define	LPT_NOPRIME	0x40	/* don't prime on open */
#define	LPT_NOINTR	0x80	/* do not use interrupt */
	u_char sc_control;
	u_char sc_laststatus;
#if defined(INET) && defined(PLIP)
	struct	arpcom	sc_arpcom;
	u_char		*sc_ifbuf;
	int		sc_ifierrs;	/* consecutive input errors */
	int		sc_ifoerrs;	/* consecutive output errors */
	int		sc_ifsoftint;	/* i/o software interrupt */
	volatile int	sc_pending;	/* interrputs pending */
#define PLIP_IPENDING	1
#define PLIP_OPENDING	2

#if defined(COMPAT_PLIP10)
	u_char		sc_adrcksum;
#endif
#endif
};

/* XXX does not belong here */
cdev_decl(lpt);

int lptprobe __P((struct device *, void *, void *));
void lptattach __P((struct device *, struct device *, void *));
int lptintr __P((void *));

#if defined(INET) && defined(PLIP)
/* Functions for the plip# interface */
static void plipattach(struct lpt_softc *,int);
static int plipioctl(struct ifnet *, u_long, caddr_t);
void plipsoftint(struct lpt_softc *);
static void plipinput(struct lpt_softc *);
static void plipstart(struct ifnet *);
static void plipoutput(struct lpt_softc *);
#endif

struct cfattach lpt_ca = {
	sizeof(struct lpt_softc), lptprobe, lptattach
};

struct cfdriver lpt_cd = {
	NULL, "lpt", DV_TTY
};

#define	LPTUNIT(s)	(minor(s) & 0x1f)
#define	LPTFLAGS(s)	(minor(s) & 0xe0)

#define	LPS_INVERT	(LPS_SELECT|LPS_NERR|LPS_NBSY|LPS_NACK)
#define	LPS_MASK	(LPS_SELECT|LPS_NERR|LPS_NBSY|LPS_NACK|LPS_NOPAPER)
#define	NOT_READY()	((bus_io_read_1(bc, ioh, lpt_status) ^ LPS_INVERT) & LPS_MASK)
#define	NOT_READY_ERR()	not_ready(bus_io_read_1(bc, ioh, lpt_status), sc)
static int not_ready __P((u_char, struct lpt_softc *));

static void lptwakeup __P((void *arg));
static int pushbytes __P((struct lpt_softc *));

int	lpt_port_test __P((bus_chipset_tag_t, bus_io_handle_t, bus_io_addr_t,
	    bus_io_size_t, u_char, u_char));

/*
 * Internal routine to lptprobe to do port tests of one byte value.
 */
int
lpt_port_test(bc, ioh, base, off, data, mask)
	bus_chipset_tag_t bc;
	bus_io_handle_t ioh;
	bus_io_addr_t base;
	bus_io_size_t off;
	u_char data, mask;
{
	int timeout;
	u_char temp;

	data &= mask;
	bus_io_write_1(bc, ioh, off, data);
	timeout = 1000;
	do {
		delay(10);
		temp = bus_io_read_1(bc, ioh, off) & mask;
	} while (temp != data && --timeout);
	LPRINTF(("lpt: port=0x%x out=0x%x in=0x%x timeout=%d\n", base + off,
	    data, temp, timeout));
	return (temp == data);
}

/*
 * Logic:
 *	1) You should be able to write to and read back the same value
 *	   to the data port.  Do an alternating zeros, alternating ones,
 *	   walking zero, and walking one test to check for stuck bits.
 *
 *	2) You should be able to write to and read back the same value
 *	   to the control port lower 5 bits, the upper 3 bits are reserved
 *	   per the IBM PC technical reference manauls and different boards
 *	   do different things with them.  Do an alternating zeros, alternating
 *	   ones, walking zero, and walking one test to check for stuck bits.
 *
 *	   Some printers drag the strobe line down when the are powered off
 * 	   so this bit has been masked out of the control port test.
 *
 *	   XXX Some printers may not like a fast pulse on init or strobe, I
 *	   don't know at this point, if that becomes a problem these bits
 *	   should be turned off in the mask byte for the control port test.
 *
 *	3) Set the data and control ports to a value of 0
 */
int
lptprobe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct mainbus_attach_args *mb = aux;
	bus_chipset_tag_t bc;
	bus_io_handle_t ioh;
	u_long base;
	u_char mask, data;
	int i, rv;

#ifdef DEBUG
#define	ABORT	do {printf("lptprobe: mask %x data %x failed\n", mask, data); \
		    goto out;} while (0)
#else
#define	ABORT	goto out
#endif

	bc = NULL;
	base = mb->mb_iobase;
	if (bus_io_map(bc, base, LPT_NPORTS, &ioh))
		return 0;

	rv = 0;
	mask = 0xff;

	data = 0x55;				/* Alternating zeros */
	if (!lpt_port_test(bc, ioh, base, lpt_data, data, mask))
		ABORT;

	data = 0xaa;				/* Alternating ones */
	if (!lpt_port_test(bc, ioh, base, lpt_data, data, mask))
		ABORT;

	for (i = 0; i < CHAR_BIT; i++) {	/* Walking zero */
		data = ~(1 << i);
		if (!lpt_port_test(bc, ioh, base, lpt_data, data, mask))
			ABORT;
	}

	for (i = 0; i < CHAR_BIT; i++) {	/* Walking one */
		data = (1 << i);
		if (!lpt_port_test(bc, ioh, base, lpt_data, data, mask))
			ABORT;
	}

	bus_io_write_1(bc, ioh, lpt_data, 0);
	bus_io_write_1(bc, ioh, lpt_control, 0);

	mb->mb_iosize = LPT_NPORTS;

	rv = 1;

out:
	bus_io_unmap(bc, ioh, LPT_NPORTS);
	return rv;
}

void
lptattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct lpt_softc *sc = (void *)self;
	struct mainbus_attach_args *mb = aux;
	bus_chipset_tag_t bc;
	bus_io_handle_t ioh;

	if (mb->mb_irq != IRQUNK)
		printf("\n");
	else
		printf(": polled\n");

	sc->sc_iobase = mb->mb_iobase;
	sc->sc_irq = mb->mb_irq;
	sc->sc_state = 0;

	bc = sc->sc_bc = NULL;
	if (bus_io_map(bc, sc->sc_iobase, LPT_NPORTS, &ioh))
		panic("lptattach: couldn't map I/O ports");
	sc->sc_ioh = ioh;

	bus_io_write_1(bc, ioh, lpt_control, LPC_NINIT);

#if defined(INET) && defined(PLIP)
	plipattach(sc, self->dv_unit);
#endif

	if (mb->mb_irq != IRQUNK) {
	 	sc->sc_ih.ih_func = lptintr;
 		sc->sc_ih.ih_arg = sc;
 		sc->sc_ih.ih_level = IPL_NONE;
 		sc->sc_ih.ih_name = "lpt";
 
		if (irq_claim(mb->mb_irq, &sc->sc_ih))
			panic("Cannot claim IRQ %d for lpt%d\n", mb->mb_irq, sc->sc_dev.dv_unit);
	}
#if defined(INET) && defined(PLIP)
	else
		printf("Warning PLIP device needs an interrupt driven lpt driver\n");
#endif

}

/*
 * Reset the printer, then wait until it's selected and not busy.
 */
int
lptopen(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	int unit = LPTUNIT(dev);
	u_char flags = LPTFLAGS(dev);
	struct lpt_softc *sc;
	bus_chipset_tag_t bc;
	bus_io_handle_t ioh;
	u_char control;
	int error;
	int spin;

	if (unit >= lpt_cd.cd_ndevs)
		return ENXIO;
	sc = lpt_cd.cd_devs[unit];
	if (!sc)
		return ENXIO;

	if (sc->sc_irq == IRQUNK && (flags & LPT_NOINTR) == 0)
		return ENXIO;

#ifdef DIAGNOSTIC
	if (sc->sc_state)
		printf("%s: stat=0x%x not zero\n", sc->sc_dev.dv_xname,
		    sc->sc_state);
#endif

	if (sc->sc_state)
		return EBUSY;

	sc->sc_state = LPT_INIT;
	sc->sc_flags = flags;
	LPRINTF(("%s: open: flags=0x%x\n", sc->sc_dev.dv_xname, flags));
	bc = sc->sc_bc;
	ioh = sc->sc_ioh;

	if ((flags & LPT_NOPRIME) == 0) {
		/* assert INIT for 100 usec to start up printer */
		bus_io_write_1(bc, ioh, lpt_control, LPC_SELECT);
		delay(100);
	}

	control = LPC_SELECT | LPC_NINIT;
	bus_io_write_1(bc, ioh, lpt_control, control);

	/* wait till ready (printer running diagnostics) */
	for (spin = 0; NOT_READY_ERR(); spin += STEP) {
		if (spin >= TIMEOUT) {
			sc->sc_state = 0;
			return EBUSY;
		}

		/* wait 1/4 second, give up if we get a signal */
		error = tsleep((caddr_t)sc, LPTPRI | PCATCH, "lptopen", STEP);
		if (error != EWOULDBLOCK) {
			sc->sc_state = 0;
			return error;
		}
	}

	if ((flags & LPT_NOINTR) == 0)
		control |= LPC_IENABLE;
	if (flags & LPT_AUTOLF)
		control |= LPC_AUTOLF;
	sc->sc_control = control;
	bus_io_write_1(bc, ioh, lpt_control, control);

	sc->sc_inbuf = geteblk(LPT_BSIZE);
	sc->sc_count = 0;
	sc->sc_state = LPT_OPEN;

	if ((sc->sc_flags & LPT_NOINTR) == 0)
		lptwakeup(sc);

	LPRINTF(("%s: opened\n", sc->sc_dev.dv_xname));
	return 0;
}

int
not_ready(status, sc)
	u_char status;
	struct lpt_softc *sc;
{
	u_char new;

	status = (status ^ LPS_INVERT) & LPS_MASK;
	new = status & ~sc->sc_laststatus;
	sc->sc_laststatus = status;

	if (new & LPS_SELECT)
		log(LOG_NOTICE, "%s: offline\n", sc->sc_dev.dv_xname);
	else if (new & LPS_NOPAPER)
		log(LOG_NOTICE, "%s: out of paper\n", sc->sc_dev.dv_xname);
	else if (new & LPS_NERR)
		log(LOG_NOTICE, "%s: output error\n", sc->sc_dev.dv_xname);

	return status;
}

void
lptwakeup(arg)
	void *arg;
{
	struct lpt_softc *sc = arg;
	int s;

	s = spltty();
	lptintr(sc);
	splx(s);

	timeout(lptwakeup, sc, STEP);
}

/*
 * Close the device, and free the local line buffer.
 */
int
lptclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	int unit = LPTUNIT(dev);
	struct lpt_softc *sc = lpt_cd.cd_devs[unit];
	bus_chipset_tag_t bc = sc->sc_bc;
	bus_io_handle_t ioh = sc->sc_ioh;

	if (sc->sc_count)
		(void) pushbytes(sc);

	if ((sc->sc_flags & LPT_NOINTR) == 0)
		untimeout(lptwakeup, sc);

	bus_io_write_1(bc, ioh, lpt_control, LPC_NINIT);
	sc->sc_state = 0;
	bus_io_write_1(bc, ioh, lpt_control, LPC_NINIT);
	brelse(sc->sc_inbuf);

	LPRINTF(("%s: closed\n", sc->sc_dev.dv_xname));
	return 0;
}

int
pushbytes(sc)
	struct lpt_softc *sc;
{
	bus_chipset_tag_t bc = sc->sc_bc;
	bus_io_handle_t ioh = sc->sc_ioh;
	int error;

	if (sc->sc_flags & LPT_NOINTR) {
		int spin, tic;
		u_char control = sc->sc_control;

		while (sc->sc_count > 0) {
			spin = 0;
			while (NOT_READY()) {
				if (++spin < sc->sc_spinmax)
					continue;
				tic = 0;
				/* adapt busy-wait algorithm */
				sc->sc_spinmax++;
				while (NOT_READY_ERR()) {
					/* exponential backoff */
					tic = tic + tic + 1;
					if (tic > TIMEOUT)
						tic = TIMEOUT;
					error = tsleep((caddr_t)sc,
					    LPTPRI | PCATCH, "lptpsh", tic);
					if (error != EWOULDBLOCK)
						return error;
				}
				break;
			}

			bus_io_write_1(bc, ioh, lpt_data, *sc->sc_cp++);
			bus_io_write_1(bc, ioh, lpt_control, control | LPC_STROBE);
			sc->sc_count--;
			bus_io_write_1(bc, ioh, lpt_control, control);

			/* adapt busy-wait algorithm */
			if (spin*2 + 16 < sc->sc_spinmax)
				sc->sc_spinmax--;
		}
	} else {
		int s;

		while (sc->sc_count > 0) {
			/* if the printer is ready for a char, give it one */
			if ((sc->sc_state & LPT_OBUSY) == 0) {
				LPRINTF(("%s: write %d\n", sc->sc_dev.dv_xname,
				    sc->sc_count));
				s = spltty();
				(void) lptintr(sc);
				splx(s);
			}
			error = tsleep((caddr_t)sc, LPTPRI | PCATCH,
			    "lptwrite2", 0);
			if (error)
				return error;
		}
	}
	return 0;
}

/* 
 * Copy a line from user space to a local buffer, then call putc to get the
 * chars moved to the output queue.
 */
int
lptwrite(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	struct lpt_softc *sc = lpt_cd.cd_devs[LPTUNIT(dev)];
	size_t n;
	int error = 0;

	while ((n = min(LPT_BSIZE, uio->uio_resid)) != 0) {
		uiomove(sc->sc_cp = sc->sc_inbuf->b_data, n, uio);
		sc->sc_count = n;
		error = pushbytes(sc);
		if (error) {
			/*
			 * Return accurate residual if interrupted or timed
			 * out.
			 */
			uio->uio_resid += sc->sc_count;
			sc->sc_count = 0;
			return error;
		}
	}
	return 0;
}

/*
 * Handle printer interrupts which occur when the printer is ready to accept
 * another char.
 */
int
lptintr(arg)
	void *arg;
{
	struct lpt_softc *sc = arg;
	bus_chipset_tag_t bc = sc->sc_bc;
	bus_io_handle_t ioh = sc->sc_ioh;

#if 0
	if ((sc->sc_state & LPT_OPEN) == 0)
		return 0;
#endif

#if defined(INET) && defined(PLIP)
	if (sc->sc_arpcom.ac_if.if_flags & IFF_UP) {
/*		i8255->port_a &= ~LPA_ACKENABLE;*/
/* YYYY */
		sc->sc_pending |= PLIP_IPENDING;
		setsoftintr(sc->sc_ifsoftint);
		return(0);
	}
#endif

	/* is printer online and ready for output */
	if (NOT_READY() && NOT_READY_ERR())
		return 0;

	if (sc->sc_count) {
		u_char control = sc->sc_control;
		/* send char */
		bus_io_write_1(bc, ioh, lpt_data, *sc->sc_cp++);
		bus_io_write_1(bc, ioh, lpt_control, control | LPC_STROBE);
		sc->sc_count--;
		bus_io_write_1(bc, ioh, lpt_control, control);
		sc->sc_state |= LPT_OBUSY;
	} else
		sc->sc_state &= ~LPT_OBUSY;

	if (sc->sc_count == 0) {
		/* none, wake up the top half to get more */
		wakeup((caddr_t)sc);
	}

	return 1;
}

int
lptioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	int error = 0;

	switch (cmd) {
	default:
		error = ENODEV;
	}

	return error;
}


#if defined(INET) && defined(PLIP)

#define PLIP_INTR_ENABLE	(LPC_NINIT | LPC_SELECT | LPC_IENABLE)
#define PLIP_INTR_DISABLE	(LPC_NINIT | LPC_SELECT)
#define PLIP_DATA		(lpt_data)
#define PLIP_STATUS		(lpt_status)
#define PLIP_CONTROL		(lpt_control)
#define PLIP_REMOTE_TRIGGER	0x08
#define PLIP_DELAY_UNIT		50

#if PLIP_DELAY_UNIT > 0	
#define PLIP_DELAY		DELAY(PLIP_DELAY_UNIT)
#else
#define PLIP_DELAY
#endif
#define PLIP_DEBUG_RX	0x01
#define PLIP_DEBUG_TX	0x02
#define PLIP_DEBUG_IF	0x04
#define PLIP_DEBUG	0x07
#if PLIP_DEBUG != 0
static int plip_debug = PLIP_DEBUG;
#endif

static void
plipattach(struct lpt_softc *sc, int unit)
{
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;

	sc->sc_ifbuf = NULL;
	sprintf(ifp->if_xname, "plip%d", unit);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS;
	ifp->if_output = ether_output;
	ifp->if_start = plipstart;
	ifp->if_ioctl = plipioctl;
	ifp->if_watchdog = 0;

	ifp->if_type = IFT_ETHER;
	ifp->if_addrlen = 6;
	ifp->if_hdrlen = 14;
	ifp->if_mtu = PLIPMTU;
	sc->sc_ifsoftint = IRQMASK_SOFTPLIP;

	printf("plip%d at lpt%d: mtu=%d,%d,%d", unit, unit, (int) ifp->if_mtu,
	    ifp->if_hdrlen, ifp->if_addrlen);

	if_attach(ifp);
}

/*
 * Process an ioctl request.
 */
static int
plipioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct proc *p = curproc;
	struct lpt_softc *sc = (struct lpt_softc *)(ifp->if_softc);
	bus_chipset_tag_t bc = sc->sc_bc;
	bus_io_handle_t ioh = sc->sc_ioh;
	struct ifaddr *ifa = (struct ifaddr *)data;
	struct ifreq *ifr = (struct ifreq *)data; 
	int s;
	int error = 0;

#if PLIP_DEBUG > 0
	printf("plipioctl: cmd=%08lx ifp=%08x data=%08x\n", cmd, (u_int)ifp, (u_int)data);
	printf("plipioctl: ifp->flags=%08x\n", ifp->if_flags);
#endif

	switch (cmd) {

	case SIOCSIFFLAGS:
		if (((ifp->if_flags & IFF_UP) == 0) &&
		    (ifp->if_flags & IFF_RUNNING)) {
		        ifp->if_flags &= ~IFF_RUNNING;
/* Deactive the parallel port */
#if PLIP_DEBUG != 0
			if (plip_debug & PLIP_DEBUG_IF)
				printf("plip: Disabling lpt irqs\n");
#endif
			bus_io_write_1(bc, ioh, lpt_data, 0x00);
			bus_io_write_1(bc, ioh, lpt_control, PLIP_INTR_DISABLE);

/*			sc->sc_i8255->port_control = LPT_MODE;
			i8255->port_a = LPA_ACTIVE | LPA_NPRIME;*/
			if (sc->sc_ifbuf)
				free(sc->sc_ifbuf, M_DEVBUF);
			sc->sc_ifbuf = NULL;
		}
		if (((ifp->if_flags & IFF_UP)) &&
		    ((ifp->if_flags & IFF_RUNNING) == 0)) {
			if (sc->sc_state) {
				error = EBUSY;
				break;
			}
			if (!sc->sc_ifbuf)
				sc->sc_ifbuf =
					malloc(ifp->if_mtu + ifp->if_hdrlen,
					       M_DEVBUF, M_WAITOK);
		        ifp->if_flags |= IFF_RUNNING;

/* This starts it running */
/* Enable lpt interrupts */

#if PLIP_DEBUG != 0
			if (plip_debug & PLIP_DEBUG_IF)
				printf("plip: Enabling lpt irqs\n");
#endif

			bus_io_write_1(bc, ioh, lpt_data, 0x00);
			bus_io_write_1(bc, ioh, lpt_control, PLIP_INTR_ENABLE);
/*			sc->sc_i8255->port_control = LPT_IRQDISABLE;
			sc->sc_i8255->port_b = 0;
			sc->sc_i8255->port_a |= LPA_ACKENABLE;*/
		}
		break;

	case SIOCSIFADDR:
		if (ifa->ifa_addr->sa_family == AF_INET) {
			if (!sc->sc_ifbuf)
				sc->sc_ifbuf =
					malloc(PLIPMTU + ifp->if_hdrlen,
					       M_DEVBUF, M_WAITOK);
			sc->sc_arpcom.ac_enaddr[0] = 0xfc;
			sc->sc_arpcom.ac_enaddr[1] = 0xfc;
			bcopy((caddr_t)&IA_SIN(ifa)->sin_addr,
			      (caddr_t)&sc->sc_arpcom.ac_enaddr[2], 4);
#if defined(COMPAT_PLIP10)
			if (ifp->if_flags & IFF_LINK0) {
				int i;
				sc->sc_arpcom.ac_enaddr[0] = 0xfd;
				sc->sc_arpcom.ac_enaddr[1] = 0xfd;
				for (i = sc->sc_adrcksum = 0; i < 5; i++)
					sc->sc_adrcksum += sc->sc_arpcom.ac_enaddr[i];
				sc->sc_adrcksum *= 2;
			}
#endif
			ifp->if_flags |= IFF_RUNNING | IFF_UP;
#if 0
			for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
				struct sockaddr_dl *sdl;
				if ((sdl = (struct sockaddr_dl *)ifa->ifa_addr) &&
				    sdl->sdl_family == AF_LINK) {
					sdl->sdl_type = IFT_ETHER;
					sdl->sdl_alen = ifp->if_addrlen;
					bcopy((caddr_t)((struct arpcom *)ifp)->ac_enaddr,
					      LLADDR(sdl), ifp->if_addrlen);
					break;
				    }
			}
#endif
/* Looks the same as the start condition above */
/* Enable lpt interrupts */

#if PLIP_DEBUG != 0
			if (plip_debug & PLIP_DEBUG_IF)
				printf("plip: Enabling lpt irqs\n");
#endif
			bus_io_write_1(bc, ioh, lpt_data, 0x00);
			bus_io_write_1(bc, ioh, lpt_control, PLIP_INTR_ENABLE);
/*			sc->sc_i8255->port_control = LPT_IRQDISABLE;
			sc->sc_i8255->port_b = 0;
			sc->sc_i8255->port_a |= LPA_ACKENABLE;*/
			arp_ifinit(&sc->sc_arpcom, ifa);
		} else
			error = EAFNOSUPPORT;
		break;

	case SIOCAIFADDR:
	case SIOCDIFADDR:
	case SIOCSIFDSTADDR:
		if (ifa->ifa_addr->sa_family != AF_INET)
			error = EAFNOSUPPORT;
		break;

	case SIOCSIFMTU:
        	if ((error = suser(p->p_ucred, &p->p_acflag)))
            		return(error);
		if (ifp->if_mtu != ifr->ifr_mtu) {
		        ifp->if_mtu = ifr->ifr_mtu;
			if (sc->sc_ifbuf) {
				s = splimp();
				free(sc->sc_ifbuf, M_DEVBUF);
				sc->sc_ifbuf =
					malloc(ifp->if_mtu + ifp->if_hdrlen,
					       M_DEVBUF, M_WAITOK);
				splx(s);
			}
		}
		break;

	default:
		error = EINVAL;
	}
	return (error);
}

void
plipintr(struct lpt_softc *sc)
{
	int pending;
	sc = lpt_cd.cd_devs[0];
	pending = sc->sc_pending;

	while (sc->sc_pending & PLIP_IPENDING) {
		pending |= sc->sc_pending;
		sc->sc_pending = 0;
		plipinput(sc);
	}

	if (pending & PLIP_OPENDING)
		plipoutput(sc);
}

static int
plipreceive(bc, ioh, buf, len)
	bus_chipset_tag_t bc;
	bus_io_handle_t ioh;
	u_char *buf;
	int len;
{
	int i;
	u_char cksum = 0, c;

	while (len--) {
		i = PLIPMXSPIN2;
		while (((bus_io_read_1(bc, ioh, lpt_status)) & LPS_NBSY) != 0)
			if (i-- < 0) return -1;
		c = ((bus_io_read_1(bc, ioh, lpt_status)) >> 3) & 0x0f;
		bus_io_write_1(bc, ioh, lpt_data, 0x11);
		while (((bus_io_read_1(bc, ioh, lpt_status)) & LPS_NBSY) == 0)
			if (i-- < 0) return -1;
		c |= ((bus_io_read_1(bc, ioh, lpt_status)) << 1) & 0xf0;
		bus_io_write_1(bc, ioh, lpt_data, 0x01);
		cksum += (*buf++ = c);
	}
	return(cksum);
}

static void
plipinput(struct lpt_softc *sc)
{
	extern struct mbuf *m_devget(char *, int, int, struct ifnet *, void (*)());
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	bus_chipset_tag_t bc = sc->sc_bc;
	bus_io_handle_t ioh = sc->sc_ioh;
	struct mbuf *m;
	struct ether_header *eh;
	u_char *p = sc->sc_ifbuf, minibuf[4];
	int c, i = 0, s, len, cksum;
/* YYYY */

/*	if (!(i8255->port_c & LPC_NACK)) {
		i8255->port_a |= LPA_ACKENABLE;
		ifp->if_collisions++;
		return;
	}*/
	bus_io_write_1(bc, ioh, lpt_data, 0x01);
	bus_io_write_1(bc, ioh, lpt_control, PLIP_INTR_DISABLE);
/*	i8255->port_b = 0x01;
	i8255->port_a &= ~(LPA_ACKENABLE | LPA_ACTIVE);*/

#if defined(COMPAT_PLIP10)
	if (ifp->if_flags & IFF_LINK0) {
		if (plipreceive(bc, ioh, minibuf, 3) < 0) goto err;
		len = (minibuf[1] << 8) | minibuf[2];
		if (len > (ifp->if_mtu + ifp->if_hdrlen)) goto err;

		switch (minibuf[0]) {
		case 0xfc:
			p[0] = p[ 6] = ifp->ac_enaddr[0];
			p[1] = p[ 7] = ifp->ac_enaddr[1];
			p[2] = p[ 8] = ifp->ac_enaddr[2];
			p[3] = p[ 9] = ifp->ac_enaddr[3];
			p[4] = p[10] = ifp->ac_enaddr[4];
			p += 5;
			if ((cksum = plipreceive(bc, ioh, p, 1)) < 0) goto err;
			p += 6;
			if ((c = plipreceive(bc, ioh, p, len - 11)) < 0) goto err;
			cksum += c + sc->sc_adrcksum;
			c = p[1]; p[1] = p[2]; p[2] = c;
			cksum &= 0xff;
			break;
		case 0xfd:
			if ((cksum = plipreceive(bc, ioh, p, len)) < 0) goto err;
			break;
		default:
			goto err;
		}
	} else
#endif
	{
		if (plipreceive(bc, ioh, minibuf, 2) < 0) goto err;
		len = (minibuf[1] << 8) | minibuf[0];
		if (len > (ifp->if_mtu + ifp->if_hdrlen)) {
			log(LOG_NOTICE, "%s: packet > MTU\n", ifp->if_softc);
			goto err;
		}
		if ((cksum = plipreceive(bc, ioh, p, len)) < 0) goto err;
	}

	if (plipreceive(bc, ioh, minibuf, 1) < 0) goto err;
	if (cksum != minibuf[0]) {
		log(LOG_NOTICE, "%s: checksum error\n", ifp->if_xname);
		goto err;
	}
	bus_io_write_1(bc, ioh, lpt_data, 0x00);
/*	i8255->port_b = 0x00;*/

	s = splimp();
	if (m = m_devget(sc->sc_ifbuf, len, 0, ifp, NULL)) {
		/* We assume that the header fit entirely in one mbuf. */
		eh = mtod(m, struct ether_header *);
		m->m_pkthdr.len -= sizeof(*eh);
		m->m_len -= sizeof(*eh);
		m->m_data += sizeof(*eh);
		ether_input(ifp, eh, m);
	}
	splx(s);
	sc->sc_ifierrs = 0;
	ifp->if_ipackets++;
	bus_io_write_1(bc, ioh, lpt_control, PLIP_INTR_ENABLE);
/*	i8255->port_a |= LPA_ACKENABLE | LPA_ACTIVE;*/
	return;

err:
	bus_io_write_1(bc, ioh, lpt_data, 0x00);
/*	i8255->port_b = 0x00;*/

	if (sc->sc_ifierrs < PLIPMXERRS) {
		bus_io_write_1(bc, ioh, lpt_control, PLIP_INTR_ENABLE);
/*		i8255->port_a |= LPA_ACKENABLE | LPA_ACTIVE;*/
	} else {
		/* We are not able to send receive anything for now,
		 * so stop wasting our time and leave the interrupt
		 * disabled.
		 */
		if (sc->sc_ifierrs == PLIPMXERRS)
			log(LOG_NOTICE, "%s: rx hard error\n", ifp->if_xname);
		bus_io_write_1(bc, ioh, lpt_control, PLIP_INTR_ENABLE);
/*		i8255->port_a |= LPA_ACTIVE;*/
	}
	ifp->if_ierrors++;
	sc->sc_ifierrs++;
	return;
}

static int
pliptransmit(bc, ioh, buf, len)
	bus_chipset_tag_t bc;
	bus_io_handle_t ioh;
	u_char *buf;
	int len;
{
	int i;
	u_char cksum = 0, c;

	while (len--) {
		i = PLIPMXSPIN2;
		cksum += (c = *buf++);
		while (((bus_io_read_1(bc, ioh, lpt_status)) & LPS_NBSY) == 0)
			if (i-- < 0) return -1;
		bus_io_write_1(bc, ioh, lpt_data, (c & 0x0f));
		bus_io_write_1(bc, ioh, lpt_data, (c & 0x0f) | 0x10);
		c >>= 4;
		while (((bus_io_read_1(bc, ioh, lpt_status)) & LPS_NBSY) != 0)
			if (i-- < 0) return -1;
		bus_io_write_1(bc, ioh, lpt_data, c | 0x10);
		bus_io_write_1(bc, ioh, lpt_data, c);
	}
	return(cksum);
}

/*
 * Setup output on interface.
 */
static void
plipstart(struct ifnet *ifp)
{
	struct lpt_softc *sc = (struct lpt_softc *)(ifp->if_softc);
	sc->sc_pending |= PLIP_OPENDING;
	setsoftintr(sc->sc_ifsoftint);
}

static void
plipoutput(struct lpt_softc *sc)
{
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	bus_chipset_tag_t bc = sc->sc_bc;
	bus_io_handle_t ioh = sc->sc_ioh;
	struct mbuf *m0, *m;
	u_char minibuf[4], cksum;
	int len, i, s;

	if (ifp->if_flags & IFF_OACTIVE)
		return;
	ifp->if_flags |= IFF_OACTIVE;

	if (sc->sc_ifoerrs)
		untimeout((void (*)(void *))plipoutput, sc);

	for (;;) {
		s = splnet();
		IF_DEQUEUE(&ifp->if_snd, m0);
		splx(s);
		if (!m0)
			break;

		for (len = 0, m = m0; m; m = m->m_next)
			len += m->m_len;
#if defined(COMPAT_PLIP10)
		if (ifp->if_flags & IFF_LINK0) {
			minibuf[0] = 3;
			minibuf[1] = 0xfd;
			minibuf[2] = len >> 8;
			minibuf[3] = len;
		} else
#endif
		{
			minibuf[0] = 2;
			minibuf[1] = len;
			minibuf[2] = len >> 8;
		}

		/* Trigger remote interrupt */
		i = PLIPMXSPIN1;
		do {
			if (sc->sc_pending & PLIP_IPENDING) {
				bus_io_write_1(bc, ioh, lpt_data, 0x00);
/*				i8255->port_b = 0x00;*/
				sc->sc_pending = 0;
				plipinput(sc);
				i = PLIPMXSPIN1;
			} else if (i-- < 0)
				goto retry;
			/* Retrigger remote interrupt */
			bus_io_write_1(bc, ioh, lpt_data, PLIP_REMOTE_TRIGGER);
/*			i8255->port_b = 0x08;*/
		} while ((i8255->port_c & LPC_NERROR) == 0);
/*		i8255->port_a &= ~(LPA_ACKENABLE | LPA_ACTIVE);*/
		bus_io_write_1(bc, ioh, lpt_control, PLIP_INTR_DISABLE);

		if (pliptransmit(bc, ioh, minibuf + 1, minibuf[0]) < 0) goto retry;
		for (cksum = 0, m = m0; m; m = m->m_next) {
			i = pliptransmit(bc, ioh, mtod(m, u_char *), m->m_len);
			if (i < 0) goto retry;
			cksum += i;
		}
		if (pliptransmit(bc, ioh, &cksum, 1) < 0) goto retry;
		i = PLIPMXSPIN2;
		while (((bus_io_read_1(bc, ioh, lpt_status)) & LPS_NBSY) == 0)
			if (i-- < 0) goto retry;
				bus_io_write_1(bc, ioh, lpt_data, 0x00);
/*		i8255->port_b = 0x00;*/

		ifp->if_opackets++;
		ifp->if_obytes += len + 4;
		sc->sc_ifoerrs = 0;
		s = splimp();
		m_freem(m0);
		splx(s);
		i8255->port_a |= LPA_ACKENABLE;
	}
	i8255->port_a |= LPA_ACTIVE;
	ifp->if_flags &= ~IFF_OACTIVE;
	return;

retry:
	if (i8255->port_c & LPC_NACK)
		ifp->if_collisions++;
	else
		ifp->if_oerrors++;

	ifp->if_flags &= ~IFF_OACTIVE;
				bus_io_write_1(bc, ioh, lpt_data, 0x00);
/*	i8255->port_b = 0x00;*/

	if ((ifp->if_flags & (IFF_RUNNING | IFF_UP)) == (IFF_RUNNING | IFF_UP)
	    && sc->sc_ifoerrs < PLIPMXRETRY) {
		s = splnet();
		IF_PREPEND(&ifp->if_snd, m0);
		splx(s);
		i8255->port_a |= LPA_ACKENABLE | LPA_ACTIVE;
		timeout((void (*)(void *))plipoutput, sc, PLIPRETRY);
	} else {
		if (sc->sc_ifoerrs == PLIPMXRETRY) {
			log(LOG_NOTICE, "%s: tx hard error\n", ifp->if_xname);
		}
		s = splimp();
		m_freem(m0);
		splx(s);
		i8255->port_a |= LPA_ACTIVE;
	}
	sc->sc_ifoerrs++;
}

#endif

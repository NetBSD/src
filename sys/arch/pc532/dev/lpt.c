/*	$NetBSD: lpt.c,v 1.2 1995/05/08 19:37:45 phil Exp $	*/

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
 *
 */

/*
 * Device Driver for Matthias's parallel printer port.
 * This driver is based on the i386 lpt driver.
 */

#include "lpt.h"
#if NLPT > 0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/device.h>
#include <sys/syslog.h>
#include <sys/malloc.h>

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

#include <machine/cpu.h>

#include <sys/device.h>
#include "lptreg.h"

#define	TIMEOUT		hz*16	/* wait up to 16 seconds for a ready */
#define	STEP		hz/4

#define	LPTPRI		(PZERO+8)
#define	LPT_BSIZE	1024

#if defined(INET) && defined(PLIP)
#ifndef PLIPMTU			/* MTU for the lp# interfaces */
#define	PLIPMTU		1600
#endif

#ifndef PLIPMXSPIN1		/* DELAY factor for the lp# interfaces */
#define	PLIPMXSPIN1	2048	/* Spinning for remote intr to happen */
#endif

#ifndef PLIPMXSPIN2		/* DELAY factor for the lp# interfaces */
#define	PLIPMXSPIN2	5120	/* Spinning for remote handshake to happen */
#endif

#ifndef PLIPMXERRS		/* Max errors before !RUNNING */
#define	PLIPMXERRS	100
#endif
#ifndef PLIPMXRETRY
#define PLIPMXRETRY	20	/* Max number of retransmits */
#endif
#ifndef PLIPRETRY
#define PLIPRETRY	hz/50	/* Time between retransmits */
#endif
#endif

#ifndef DEBUG
#define lprintf
#else
#define lprintf		if (lptdebug) printf
int lptdebug = 1;
#endif

struct lpt_softc {
	struct device sc_dev;
	size_t sc_count;
	u_char *sc_inbuf;
	u_char *sc_cp;
	volatile struct i8255 *sc_i8255;
	int sc_irq;
	u_char sc_state;
	u_char sc_status;
#define	LPT_OPEN	0x01	/* device is open */
#define	LPT_INIT	0x02	/* waiting to initialize for open */

	u_char sc_flags;
#define	LPT_AUTOLF	0x20	/* automatic LF on CR */
#define	LPT_NOPRIME	0x40	/* don't prime on open */

#if defined(INET) && defined(PLIP)
	struct	arpcom	sc_arpcom;
	u_char		*sc_ifbuf;
	int		sc_iferrs;
	int		sc_ifretry;
	u_char		sc_adrcksum;
#endif
};

int lptmatch __P((struct device *, void *, void *aux));
void lptattach __P((struct device *, struct device *, void *));
int lptintr __P((int));

#if defined(INET) && defined(PLIP)
/* Functions for the plip# interface */
static void plipattach(struct lpt_softc *,int);
static int plipioctl(struct ifnet *, u_long, caddr_t);
static int plipstart(struct ifnet *);
static void plipintr(int, struct ifnet *);
#endif

struct cfdriver lptcd = {
	NULL,
	"lpt",
	lptmatch,
	lptattach,
	DV_TTY,
	sizeof(struct lpt_softc),
	NULL,
	0
};

#define	LPTUNIT(s)	(minor(s) & 0x1f)
#define	LPTFLAGS(s)	(minor(s) & 0xe0)

#define	LPT_INVERT	(LPC_NBUSY|LPC_NERROR|LPC_NACK|LPC_ONLINE)
#define	LPT_MASK	(LPC_NBUSY|LPC_NERROR|LPC_NACK|LPC_NOPAPER|LPC_ONLINE)
#define DIAGNOSTIC
#ifndef DIAGNOSTIC
#define	NOT_READY()	((i8255->port_c ^ LPT_INVERT) & LPT_MASK)
#else
#define	NOT_READY()	notready(i8255->port_c, sc)
static int notready __P((u_char, struct lpt_softc *));
#endif

static void lptout __P((void *arg));
static int pushbytes __P((struct lpt_softc *));

lptmatch(struct device *parent, void *cf, void *aux)
{
	volatile struct i8255 *i8255 =
		(volatile struct i8255 *)((struct cfdata *)cf)->cf_loc[2];
	int unit = ((struct cfdata *)cf)->cf_unit;

	if (unit >= LPT_MAX)
		return(0);

	if ((int) i8255 == -1)
		i8255 = LPT_ADR(unit);

	i8255->port_control = LPT_PROBE_MODE;

	i8255->port_control = LPT_PROBE_CLR;
	if (i8255->port_c & LPT_PROBE_MASK)
		return 0;
	
	i8255->port_control = LPT_PROBE_SET;
	if (!(i8255->port_c & LPT_PROBE_MASK))
		return 0;

	i8255->port_control = LPT_PROBE_CLR;
	if (i8255->port_c & LPT_PROBE_MASK)
		return 0;

	i8255->port_control = LPT_MODE;
	
	return 1;
}

void
lptattach(struct device *parent, struct device *self, void *aux)
{
	struct lpt_softc *sc = (struct lpt_softc *) self;
	volatile struct i8255 *i8255 =
			(volatile struct i8255 *)self->dv_cfdata->cf_loc[2];
	int irq;

	if ((irq = self->dv_cfdata->cf_loc[4]) == -1)
		irq = LPT_IRQ(self->dv_unit);
#if defined(INET) && defined(PLIP)
	PL_net |= 1 << irq;
#endif
	PL_tty |= 1 << irq;
	PL_zero |= PL_tty;

	if ((int)i8255 == -1)
		i8255 = LPT_ADR(self->dv_unit);
	i8255->port_control = LPT_MODE;
	i8255->port_control = LPT_IRQDISABLE;

	sc->sc_state = 0;
	sc->sc_i8255 = i8255;

#if defined(INET) && defined(PLIP)
	plipattach(sc, self->dv_unit);
#endif

	printf(" addr 0x%x\n", (int) i8255);
}

/*
 * Reset the printer, then wait until it's selected and not busy.
 */
int
lptopen(dev_t dev, int flag)
{
	struct lpt_softc *sc = (struct lpt_softc *) lptcd.cd_devs[LPTUNIT(dev)];
	volatile struct i8255 *i8255 = sc->sc_i8255;
	u_char flags = LPTFLAGS(dev);
	int error;
	int spin;

	if (LPTUNIT(dev) >= NLPT || !sc)
		return ENXIO;

	if (sc->sc_state || (sc->sc_arpcom.ac_if.if_flags & IFF_UP))
		return EBUSY;

	sc->sc_state = LPT_INIT;
	sc->sc_flags = flags;
	lprintf("%s: open: flags=0x%x\n", sc->sc_dev.dv_xname, flags);

	if ((flags & LPT_NOPRIME) == 0) {
		/* assert INIT for 100 usec to start up printer */
		i8255->port_a = LPA_PRIME;
		DELAY(100);
	}

	if (flags & LPT_AUTOLF)
		i8255->port_a = LPA_SELECT | LPA_ALF;
	else
		i8255->port_a = LPA_SELECT;

	/* wait till ready (printer running diagnostics) */
	for (spin = 0; NOT_READY(); spin += STEP) {
		if (spin >= TIMEOUT) {
			sc->sc_state = 0;
			return EBUSY;
		}

		/* wait 1/4 second, give up if we get a signal */
		if (error = tsleep((caddr_t)sc, LPTPRI | PCATCH, "lptopen",
		    STEP) != EWOULDBLOCK) {
			sc->sc_state = 0;
			return error;
		}
	}

	sc->sc_inbuf  = malloc(LPT_BSIZE, M_DEVBUF, M_WAITOK);
	sc->sc_status =
	sc->sc_count  = 0;
	sc->sc_state  = LPT_OPEN;

	lprintf("%s: opened\n", sc->sc_dev.dv_xname);
	return 0;
}

#ifdef DIAGNOSTIC
int
notready(u_char status, struct lpt_softc *sc)
{
	status ^= LPT_INVERT;

	if (status != sc->sc_status) {
		if (status & LPC_NOPAPER)
			log(LOG_NOTICE, "%s: out of paper\n", sc->sc_dev.dv_xname);
		else if (status & LPC_ONLINE)
			log(LOG_NOTICE, "%s: offline\n", sc->sc_dev.dv_xname);
		else if (status & LPC_NERROR)
			log(LOG_NOTICE, "%s: output error\n", sc->sc_dev.dv_xname);
		sc->sc_status = status;
	}
	return status & LPT_MASK;
}
#endif

void
lptout(void *arg)
{
	struct lpt_softc *sc = (struct lpt_softc *) arg;
	if ((sc->sc_state & LPT_OPEN) == 0)
		sc->sc_i8255->port_control = LPT_IRQENABLE;
}

/*
 * Close the device, and free the local line buffer.
 */
lptclose(dev_t dev, int flag)
{
	struct lpt_softc *sc = (struct lpt_softc *) lptcd.cd_devs[LPTUNIT(dev)];

	if (sc->sc_count)
		(void) pushbytes(sc);

	sc->sc_i8255->port_control = LPT_IRQDISABLE;
	sc->sc_state = 0;
	free(sc->sc_inbuf, M_DEVBUF);

	lprintf("%s: closed\n", sc->sc_dev.dv_xname);
	return 0;
}

int
pushbytes(struct lpt_softc *sc)
{
	volatile struct i8255 *i8255 = sc->sc_i8255;
	int error;

	while (sc->sc_count > 0) {
		i8255->port_control = LPT_IRQENABLE;
		if (error = tsleep((caddr_t)sc, LPTPRI | PCATCH,
		    "lptwrite", 0))
			return error;
	}
	return 0;
}

/* 
 * Copy a line from user space to a local buffer, then call putc to get the
 * chars moved to the output queue.
 */
lptwrite(dev_t dev, struct uio *uio)
{
	struct lpt_softc *sc = (struct lpt_softc *) lptcd.cd_devs[LPTUNIT(dev)];
	size_t n;
	int error = 0;

	if (sc->sc_count) return EBUSY;
	while (n = min(LPT_BSIZE, uio->uio_resid)) {
		uiomove(sc->sc_cp = sc->sc_inbuf, n, uio);
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
lptintr(int unit)
{
	struct lpt_softc *sc = (struct lpt_softc *) lptcd.cd_devs[unit];
	volatile struct i8255 *i8255 = sc->sc_i8255;

#if defined(INET) && defined(PLIP)
	if(sc->sc_arpcom.ac_if.if_flags & IFF_UP) {
		i8255->port_a |= LPA_ACTIVE;
		plipintr(unit, &sc->sc_arpcom.ac_if);
		i8255->port_a &= ~LPA_ACTIVE;
		return 1;
	}
#endif

	if ((sc->sc_state & LPT_OPEN) == 0) {
		i8255->port_control = LPT_IRQDISABLE;
		return 0;
	}

	if (sc->sc_count) {
		/* is printer online and ready for output? */
		if (NOT_READY()) {
			i8255->port_control = LPT_IRQDISABLE;
			timeout(lptout, sc, STEP);
			return 0;
		}
		/* send char */
		i8255->port_a |= LPA_ACTIVE;
		i8255->port_b = *sc->sc_cp++;
		sc->sc_count--;
		i8255->port_a &= ~LPA_ACTIVE;
	}

	if (sc->sc_count == 0) {
		/* none, wake up the top half to get more */
		i8255->port_control = LPT_IRQDISABLE;
		wakeup((caddr_t)sc);
	}

	return 1;
}

int
lptioctl(dev_t dev, int cmd, caddr_t data, int flag)
{
	int error = 0;

	switch (cmd) {
	default:
		error = EINVAL;
	}

	return error;
}

#if defined(INET) && defined(PLIP)

static void
plipattach(struct lpt_softc *sc, int unit)
{
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;

	sc->sc_ifbuf = NULL;
	ifp->if_name = "plip";
	ifp->if_unit = unit;
	ifp->if_type = IFT_ETHER;
	ifp->if_addrlen = 6;
	ifp->if_hdrlen = 14;
	ifp->if_mtu = PLIPMTU;
	ifp->if_flags = IFF_NOTRAILERS | IFF_BROADCAST | IFF_SIMPLEX;
	ifp->if_output = ether_output;
	ifp->if_start = plipstart;
	ifp->if_ioctl = plipioctl;
	ifp->if_watchdog = 0;
	if_attach(ifp);
}

/*
 * Process an ioctl request.
 */
static int
plipioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
        struct proc *p = curproc;
	struct lpt_softc *sc = (struct lpt_softc *) lptcd.cd_devs[ifp->if_unit];
	volatile struct i8255 *i8255 = sc->sc_i8255;
	struct ifaddr *ifa = (struct ifaddr *)data;
	struct ifreq *ifr = (struct ifreq *)data; 
	int s;
	int error = 0;

	switch (cmd) {

	case SIOCSIFFLAGS:
		if (((ifp->if_flags & IFF_UP) == 0) &&
		    (ifp->if_flags & IFF_RUNNING)) {
		        ifp->if_flags &= ~IFF_RUNNING;
			sc->sc_i8255->port_a &= ~LPA_ACKENABLE;
			sc->sc_i8255->port_control = LPT_MODE;
			if (sc->sc_ifbuf)
				free(sc->sc_ifbuf, M_DEVBUF);
			sc->sc_ifbuf = NULL;
		}
		if (((ifp->if_flags & IFF_UP)) &&
		    ((ifp->if_flags & IFF_RUNNING) == 0)) {
			if (!sc->sc_ifbuf)
				sc->sc_ifbuf =
					malloc(ifp->if_mtu, M_DEVBUF, M_WAITOK);
		        ifp->if_flags |= IFF_RUNNING;
			sc->sc_i8255->port_control = LPT_IRQDISABLE;
			sc->sc_i8255->port_b = 0;
			sc->sc_i8255->port_a |= LPA_ACKENABLE;
		}
		break;

	case SIOCAIFADDR:
		if (ifa->ifa_addr->sa_family != AF_INET)
			error = EAFNOSUPPORT;
		break;

	case SIOCSIFADDR:
		if (ifa->ifa_addr->sa_family == AF_INET) {
			if (!sc->sc_ifbuf)
				sc->sc_ifbuf =
					malloc(PLIPMTU, M_DEVBUF, M_WAITOK);
			bcopy((caddr_t)&IA_SIN(ifa)->sin_addr,
			      (caddr_t)&sc->sc_arpcom.ac_enaddr[2], 4);
			sc->sc_arpcom.ac_ipaddr = IA_SIN(ifa)->sin_addr;
			if (ifp->if_flags & IFF_LINK0) {
				int i;
				sc->sc_arpcom.ac_enaddr[0] = 0xfd;
				sc->sc_arpcom.ac_enaddr[1] = 0xfd;
				for (i = sc->sc_adrcksum = 0; i < 5; i++)
					sc->sc_adrcksum += sc->sc_arpcom.ac_enaddr[i];
				sc->sc_adrcksum *= 2;
			} else {
				sc->sc_arpcom.ac_enaddr[0] = 0xfc;
				sc->sc_arpcom.ac_enaddr[1] = 0xfc;
			}
			ifp->if_flags |= IFF_RUNNING | IFF_UP;
			sc->sc_i8255->port_control = LPT_IRQDISABLE;
			sc->sc_i8255->port_b = 0;
			sc->sc_i8255->port_a |= LPA_ACKENABLE;
			arpwhohas(&sc->sc_arpcom, &IA_SIN(ifa)->sin_addr);
		} else
			error = EAFNOSUPPORT;
		break;

	case SIOCSIFDSTADDR:
		if (ifa->ifa_addr->sa_family != AF_INET)
			error = EAFNOSUPPORT;
		break;

	case SIOCSIFMTU:
        	if ((error = suser(p->p_ucred, &p->p_acflag)))
            		return (error);
		if (ifp->if_mtu != ifr->ifr_metric) {
		        ifp->if_mtu = ifr->ifr_metric;
			if (sc->sc_ifbuf) {
				s = splimp();
				free(sc->sc_ifbuf, M_DEVBUF);
				sc->sc_ifbuf =
					malloc(ifp->if_mtu, M_DEVBUF, M_WAITOK);
				splx(s);
			}
		}
		break;

        case SIOCGIFMTU:
	        ifr->ifr_metric = ifp->if_mtu;
		break;

	default:
		error = EINVAL;
	}
	return (error);
}

static int
plipreceive(volatile struct i8255 *i8255, u_char *buf, int len)
{
	int i;
	u_char cksum = 0, c;
#if 1
	static unsigned char tl[] = {
		0x00, 0x01, 0x08, 0x09,
		0x04, 0x05, 0x0c, 0x0d,
		0x02, 0x03, 0x0a, 0x0b,
		0x06, 0x07, 0x0e, 0x0f
	};
# define LOWNIBBLE (tl[i8255->port_c >> 4])

	static unsigned char th[] = {
		0x00, 0x10, 0x80, 0x90,
		0x40, 0x50, 0xc0, 0xd0,
		0x20, 0x30, 0xa0, 0xb0,
		0x60, 0x70, 0xe0, 0xf0
	};
#define HIGHNIBBLE (th[i8255->port_c >> 4])
#else
# define LOWNIBBLE (i8255->port_c >> 4)
# define HIGHNIBBLE (i8255->port_c & 0xf0)
#endif

	while (len--) {
		for (i = PLIPMXSPIN2; (i8255->port_c & LPC_NBUSY); i--)
			if (i <= 0) return(-1);
		c = LOWNIBBLE;
		i8255->port_b = 0x11;
		for (i = PLIPMXSPIN2; !(i8255->port_c & LPC_NBUSY); i--)
			if (i <= 0) return(-1);
		c |= HIGHNIBBLE;
		i8255->port_b = 0x01;
		cksum += (*buf++ = c);
	}

	return(cksum);
}

static void
plipintr(int unit, struct ifnet *ifp)
{
	extern struct mbuf *m_devget(char *, int, int, struct ifnet *, void (*)());
	struct lpt_softc *sc = (struct lpt_softc *) lptcd.cd_devs[unit];
	volatile struct i8255 *i8255 = sc->sc_i8255;
	struct mbuf *m;
	struct ether_header *eh;
	u_char *p = sc->sc_ifbuf, minibuf[4];
	int c, i = 0, s, len, cksum;

	s = splimp();
	i8255->port_a &= ~LPA_ACKENABLE;
	i8255->port_b = 0x01;

	if (ifp->if_flags & IFF_LINK0) {
		if (plipreceive(i8255, minibuf, 3) < 0) goto err;
		len = (minibuf[1] << 8) | minibuf[2];
		if (len > ifp->if_mtu) goto err;

		switch (minibuf[0]) {
		case 0xfc:
			memcpy(p + 0, sc->sc_arpcom.ac_enaddr, 5);
			memcpy(p + 6, sc->sc_arpcom.ac_enaddr, 5);
			p += 5;
			if ((cksum = plipreceive(i8255, p, 1)) < 0) goto err;
			p += 6;
			if ((c = plipreceive(i8255, p, len - 11)) < 0) goto err;
			cksum += c + sc->sc_adrcksum;
			c = p[1]; p[1] = p[2]; p[2] = c;
			cksum &= 0xff;
			break;
		case 0xfd:
			if ((cksum = plipreceive(i8255, p, len)) < 0) goto err;
			break;
		default:
			goto err;
		}
	} else {
		if (plipreceive(i8255, minibuf, 2) < 0) goto err;
		len = (minibuf[1] << 8) | minibuf[0];
		if (len > ifp->if_mtu) goto err;

		if ((cksum = plipreceive(i8255, p, len)) < 0) goto err;
	}
	
	if (plipreceive(i8255, minibuf, 1) < 0) goto err;
	if (cksum != minibuf[0]) {
		log(LOG_NOTICE, "plip%d: checksum error\n", unit);
		goto err;
	}
	i8255->port_b = 0x00;

	if (m = m_devget(sc->sc_ifbuf, len, 0, ifp, NULL)) {
		/* We assume that the header fit entirely in one mbuf. */
		eh = mtod(m, struct ether_header *);
		m->m_pkthdr.len -= sizeof(*eh);
		m->m_len -= sizeof(*eh);
		m->m_data += sizeof(*eh);
		ether_input(ifp, eh, m);
	}
done:
	sc->sc_iferrs = 0;
	ifp->if_ipackets++;
	i8255->port_a |= LPA_ACKENABLE;
	splx(s);
	return;

err:
	i8255->port_b = 0x00;

	ifp->if_ierrors++;
	sc->sc_iferrs++;
	if(sc->sc_iferrs > PLIPMXERRS) {
		/* We are not able to send receive anything for now,
		 * so stop wasting our time and leave the interrupt
		 * disabled.
		 */
		if (sc->sc_iferrs == PLIPMXERRS + 1)
			log(LOG_NOTICE, "plip%d: rx hard error\n", ifp->if_unit);
	} else
		i8255->port_a |= LPA_ACKENABLE;
	splx(s);
	return;
}

static int
pliptransmit(volatile struct i8255 *i8255, u_char *buf, int len)
{
	int i;
	u_char cksum = 0, c;

	while (len--) {
		cksum += (c = *buf++);
		i8255->port_b = c & 0x0f;
		i8255->port_b = c & 0x0f | 0x10;
		for (i = PLIPMXSPIN2; (i8255->port_c & LPC_NBUSY); i--) {
			if (i < 0) return -1;
			if (i8255->port_c & LPC_NACK) return -1;
		}
		c >>= 4;
		i8255->port_b = c & 0x0f | 0x10;
		i8255->port_b = c & 0x0f;
		for (i = PLIPMXSPIN2; !(i8255->port_c & LPC_NBUSY); i--) {
			if (i < 0) return -1;
			if (i8255->port_c & LPC_NACK) return -1;
		}
	}
	return(cksum);
}

/*
 * Setup output on interface.
 */
static int
plipstart(struct ifnet *ifp)
{
	struct lpt_softc *sc = (struct lpt_softc *) lptcd.cd_devs[ifp->if_unit];
	volatile struct i8255 *i8255 = sc->sc_i8255;
	struct mbuf *m0, *m;
	u_char *cp, minibuf[4], cksum;
	int len, i, s;

	s = splimp();
	if (ifp->if_flags & IFF_OACTIVE) {
		splx(s);
		return;
	}
	ifp->if_flags |= IFF_OACTIVE;

	if (sc->sc_ifretry)
		untimeout((void (*)(void *))plipstart, (caddr_t)ifp);

	i8255->port_a |= LPA_ACTIVE;
	for (;;) {
		IF_DEQUEUE(&ifp->if_snd, m0);
		if (!m0)
			break;

		for (len = 0, m = m0; m; m = m->m_next)
			len += m->m_len;
		if (ifp->if_flags & IFF_LINK0) {
			minibuf[0] = 3;
			minibuf[1] = 0xfd;
			minibuf[2] = len >> 8;
			minibuf[3] = len;
		} else {
			minibuf[0] = 2;
			minibuf[1] = len;
			minibuf[2] = len >> 8;
		}

		for (i = PLIPMXSPIN1; i8255->port_c & LPC_NERROR; i--)
			if (i <= 0) goto retry;

		/* Trigger remote interrupt */
		i8255->port_a &= ~LPA_ACKENABLE;
		i8255->port_b = 0x08;

		for (i = PLIPMXSPIN1; !(i8255->port_c & LPC_NERROR); i--) {
			if (i <= 0 || (i8255->port_c & LPC_NACK
			  	       && i > PLIPMXSPIN1/3))
				goto retry;
			if ((i & 0x1f) == 0)
				i8255->port_b ^= 0x08;
		}

		if (pliptransmit(i8255, minibuf + 1, minibuf[0]) < 0) goto retry;
		for (cksum = 0, m = m0; m; m = m->m_next) {
			cp = mtod(m, u_char *);
			i = pliptransmit(i8255, mtod(m, u_char *), m->m_len);
			if (i < 0) goto retry;
			cksum += i;
		}
		if (pliptransmit(i8255, &cksum, 1) < 0) goto retry;
end:
		i8255->port_b = 0x00;
		if(i > 0) {
			ifp->if_opackets++;
			ifp->if_obytes += len + 4;
		}
		sc->sc_ifretry = 0;
		m_freem(m0);
		i8255->port_a |= LPA_ACKENABLE;
	}
	i8255->port_a &= ~LPA_ACTIVE;
	ifp->if_flags &= ~IFF_OACTIVE;
	splx(s);
	return(0);

retry:
	if (i8255->port_c & LPC_NACK)
		ifp->if_collisions++;
	else
		ifp->if_oerrors++;
	if ((ifp->if_flags & (IFF_RUNNING | IFF_UP)) == (IFF_RUNNING | IFF_UP)
	    && sc->sc_ifretry < PLIPMXRETRY) {
		sc->sc_ifretry++;
		IF_PREPEND(&ifp->if_snd, m0);
		timeout((void (*)(void *))plipstart, (caddr_t)ifp, PLIPRETRY);
	} else {
		if (sc->sc_ifretry != PLIPMXRETRY + 1)
			log(LOG_NOTICE, "plip%d: tx hard error\n", ifp->if_unit);
		m_freem(m0);
	}
	ifp->if_flags &= ~IFF_OACTIVE;
	i8255->port_b = 0x00;
	i8255->port_a |= LPA_ACKENABLE;
	i8255->port_a &= ~LPA_ACTIVE;
	splx(s);
	return(0);
}

#endif

#endif

/*	$NetBSD: if_eg.c,v 1.11 1995/04/11 05:10:20 mycroft Exp $	*/

/*
 * Copyright (c) 1993 Dean Huxley <dean@fsa.ca>
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
 *      This product includes software developed by Dean Huxley.
 * 4. The name of Dean Huxley may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* To do:
 * - multicast
 * - promiscuous
 */
#include "bpfilter.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/netisr.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/netisr.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/cpu.h>
#include <machine/pio.h>

#include <i386/isa/isavar.h>
#include <dev/isa/if_egreg.h>
#include <dev/isa/elink.h>

/* for debugging convenience */
#ifdef EGDEBUG
#define dprintf(x) printf x
#else
#define dprintf(x)
#endif

#define ETHER_MIN_LEN	64
#define ETHER_MAX_LEN	1518
#define ETHER_ADDR_LEN	6

#define EG_INLEN  	10
#define EG_BUFLEN	0x0670

/*
 * Ethernet software status per interface.
 */
struct eg_softc {
	struct device sc_dev;
	struct intrhand sc_ih;
	struct arpcom sc_arpcom;	/* Ethernet common part */
	int eg_cmd;			/* Command register R/W */
	int eg_ctl;			/* Control register R/W (EG_CTL_*) */
	int eg_stat;			/* Status register R/O (EG_STAT_*) */
	int eg_data;			/* Data register R/W (16 bits) */
	u_char  eg_rom_major;		/* Cards ROM version (major number) */ 
	u_char  eg_rom_minor;		/* Cards ROM version (minor number) */ 
	short	eg_ram;			/* Amount of RAM on the card */
	u_char	eg_pcb[64];		/* Primary Command Block buffer */
	u_char  eg_incount;		/* Number of buffers currently used */
	u_char  *eg_inbuf;		/* Incoming packet buffer */
	u_char  *eg_outbuf;		/* Outgoing packet buffer */
};

int egprobe __P((struct device *, void *, void *));
void egattach __P((struct device *, struct device *, void *));

struct cfdriver egcd = {
	NULL, "eg", egprobe, egattach, DV_IFNET, sizeof(struct eg_softc)
};

int egintr __P((struct eg_softc *));
static void eginit __P((struct eg_softc *));
static int egioctl __P((struct ifnet *, u_long, caddr_t));
static int egrecv __P((struct eg_softc *));
static void egstart __P((struct ifnet *));
static void egwatchdog __P((int));
static void egreset __P((struct eg_softc *));
static inline void egread __P((struct eg_softc *, caddr_t, int));
static struct mbuf *egget __P((caddr_t, int, struct ifnet *));
static void egstop __P((struct eg_softc *));

/*
 * Support stuff
 */
	
static inline void
egprintpcb(sc)
	struct eg_softc *sc;
{
	int i;
	
	for (i = 0; i < sc->eg_pcb[1] + 2; i++)
		dprintf(("pcb[%2d] = %x\n", i, sc->eg_pcb[i]));
}


static inline void
egprintstat(b)
	u_char b;
{
	dprintf(("%s %s %s %s %s %s %s\n", 
		 (b & EG_STAT_HCRE)?"HCRE":"",
		 (b & EG_STAT_ACRF)?"ACRF":"",
		 (b & EG_STAT_DIR )?"DIR ":"",
		 (b & EG_STAT_DONE)?"DONE":"",
		 (b & EG_STAT_ASF3)?"ASF3":"",
		 (b & EG_STAT_ASF2)?"ASF2":"",
		 (b & EG_STAT_ASF1)?"ASF1":""));
}

static int
egoutPCB(sc, b)
	struct eg_softc *sc;
	u_char b;
{
	int i;

	for (i=0; i < 4000; i++) {
		if (inb(sc->eg_stat) & EG_STAT_HCRE) {
			outb(sc->eg_cmd, b);
			return 0;
		}
		delay(10);
	}
	dprintf(("egoutPCB failed\n"));
	return 1;
}
	
static int
egreadPCBstat(sc, statb)
	struct eg_softc *sc;
	u_char statb;
{
	int i;

	for (i=0; i < 5000; i++) {
		if (EG_PCB_STAT(inb(sc->eg_stat))) 
			break;
		delay(10);
	}
	if (EG_PCB_STAT(inb(sc->eg_stat)) == statb)
		return 0;
	return 1;
}

static int
egreadPCBready(sc)
	struct eg_softc *sc;
{
	int i;

	for (i=0; i < 10000; i++) {
		if (inb(sc->eg_stat) & EG_STAT_ACRF)
			return 0;
		delay(5);
	}
	dprintf(("PCB read not ready\n"));
	return 1;
}
	
static int
egwritePCB(sc)
	struct eg_softc *sc;
{
	int i;
	u_char len;

	outb(sc->eg_ctl, EG_PCB_MASK(inb(sc->eg_ctl)));

	len = sc->eg_pcb[1] + 2;
	for (i = 0; i < len; i++)
		egoutPCB(sc, sc->eg_pcb[i]);
	
	for (i=0; i < 4000; i++) {
		if (inb(sc->eg_stat) & EG_STAT_HCRE)
			break;
		delay(10);
	}
	outb(sc->eg_ctl, EG_PCB_MASK(inb(sc->eg_ctl)) | EG_PCB_DONE);
	egoutPCB(sc, len);

	if (egreadPCBstat(sc, EG_PCB_ACCEPT))
		return 1;
	return 0;
}	
	
static int
egreadPCB(sc)
	struct eg_softc *sc;
{
	int i;
	u_char b;
	
	outb(sc->eg_ctl, EG_PCB_MASK(inb(sc->eg_ctl)));

	bzero(sc->eg_pcb, sizeof(sc->eg_pcb));

	if (egreadPCBready(sc))
		return 1;

	sc->eg_pcb[0] = inb(sc->eg_cmd);
	
	if (egreadPCBready(sc))
		return 1;

	sc->eg_pcb[1] = inb(sc->eg_cmd);

	if (sc->eg_pcb[1] > 62) {
		dprintf(("len %d too large\n", sc->eg_pcb[1]));
		return 1;
	}
	
	for (i = 0; i < sc->eg_pcb[1]; i++) {
		if (egreadPCBready(sc))
			return 1;
		sc->eg_pcb[2+i] = inb(sc->eg_cmd);
	}
	if (egreadPCBready(sc))
		return 1;
	if (egreadPCBstat(sc, EG_PCB_DONE))
		return 1;
	if ((b = inb(sc->eg_cmd)) != sc->eg_pcb[1] + 2) {
		dprintf(("%d != %d\n", b, sc->eg_pcb[1] + 2));
		return 1;
	}
	outb(sc->eg_ctl, EG_PCB_MASK(inb(sc->eg_ctl)) | EG_PCB_ACCEPT);
	return 0;
}	

/*
 * Real stuff
 */

int
egprobe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct eg_softc *sc = match;
	struct isa_attach_args *ia = aux;
	int i;

	if (ia->ia_iobase & ~0x07f0 != 0) {
		dprintf(("Weird iobase %x\n", ia->ia_iobase));
		return 0;
	}
	
	sc->eg_cmd = ia->ia_iobase + EG_COMMAND;
	sc->eg_ctl = ia->ia_iobase + EG_CONTROL;
	sc->eg_stat = ia->ia_iobase + EG_STATUS;
	sc->eg_data = ia->ia_iobase + EG_DATA;

	/* hard reset card */
	outb(sc->eg_ctl, EG_CTL_RESET); 
	outb(sc->eg_ctl, 0);
	for (i = 0; i < 5000; i++) {
		delay(1000);
		if (EG_PCB_STAT(inb(sc->eg_stat)) == 0)
			break;
	}
	if (EG_PCB_STAT(inb(sc->eg_stat)) != 0) {
		dprintf(("eg: Reset failed\n"));
		return 0;
	}
	sc->eg_pcb[0] = EG_CMD_GETINFO; /* Get Adapter Info */
	sc->eg_pcb[1] = 0;
	if (egwritePCB(sc) != 0)
		return 0;
	
	if (egreadPCB(sc) != 0) {
		egprintpcb(sc);
		return 0;
	}

	if (sc->eg_pcb[0] != EG_RSP_GETINFO || /* Get Adapter Info Response */
	    sc->eg_pcb[1] != 0x0a) {
		egprintpcb(sc);
		return 0;
	}
	sc->eg_rom_major = sc->eg_pcb[3];
	sc->eg_rom_minor = sc->eg_pcb[2];
	sc->eg_ram = sc->eg_pcb[6] | (sc->eg_pcb[7] << 8);
	
	ia->ia_iosize = 0x08;
	ia->ia_msize = 0;
	return 1;
}

static void
egattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct eg_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	int i;
	
	egstop(sc);

	sc->eg_pcb[0] = EG_CMD_GETEADDR; /* Get Station address */
	sc->eg_pcb[1] = 0;
	if (egwritePCB(sc) != 0) {
		dprintf(("write error\n"));
		return;
	}	
	if (egreadPCB(sc) != 0) {
		dprintf(("read error\n"));
		egprintpcb(sc);
		return;
	}

	/* check Get station address response */
	if (sc->eg_pcb[0] != EG_RSP_GETEADDR || sc->eg_pcb[1] != 0x06) { 
		dprintf(("parse error\n"));
		egprintpcb(sc);
		return;
	}
	bcopy(&sc->eg_pcb[2], sc->sc_arpcom.ac_enaddr, ETHER_ADDR_LEN);

	printf(": ROM v%d.%02d %dk address %s\n",
	    sc->eg_rom_major, sc->eg_rom_minor, sc->eg_ram,
	    ether_sprintf(sc->sc_arpcom.ac_enaddr));

	sc->eg_pcb[0] = EG_CMD_SETEADDR; /* Set station address */
	if (egwritePCB(sc) != 0) {
		dprintf(("write error2\n"));
		return;
	}
	if (egreadPCB(sc) != 0) {
		dprintf(("read error2\n"));
		egprintpcb(sc);
		return;
	}
	if (sc->eg_pcb[0] != EG_RSP_SETEADDR || sc->eg_pcb[1] != 0x02 ||
	    sc->eg_pcb[2] != 0 || sc->eg_pcb[3] != 0) {
		dprintf(("parse error2\n"));
		egprintpcb(sc);
		return;
	}

	/* Initialize ifnet structure. */
	ifp->if_unit = sc->sc_dev.dv_unit;
	ifp->if_name = egcd.cd_name;
	ifp->if_start = egstart;
	ifp->if_ioctl = egioctl;
	ifp->if_watchdog = egwatchdog;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS;
	
	/* Now we can attach the interface. */
	if_attach(ifp);
	ether_ifattach(ifp);
	
#if NBPFILTER > 0
	bpfattach(&ifp->if_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif
	
	sc->sc_ih.ih_fun = egintr;
	sc->sc_ih.ih_arg = sc;
	sc->sc_ih.ih_level = IPL_NET;
	intr_establish(ia->ia_irq, IST_EDGE, &sc->sc_ih);
}

static void
eginit(sc)
	register struct eg_softc *sc;
{
	register struct ifnet *ifp = &sc->sc_arpcom.ac_if;

	/* Address not known. */
	if (ifp->if_addrlist == 0)
		return;

	/* soft reset the board */
	outb(sc->eg_ctl, EG_CTL_FLSH);
	delay(100);
	outb(sc->eg_ctl, EG_CTL_ATTN);
	delay(100);
	outb(sc->eg_ctl, 0);
	delay(200);

	sc->eg_pcb[0] = EG_CMD_CONFIG82586; /* Configure 82586 */
	sc->eg_pcb[1] = 2;
	sc->eg_pcb[2] = 3; /* receive broadcast & multicast */
	sc->eg_pcb[3] = 0;
	if (egwritePCB(sc) != 0)
		dprintf(("write error3\n"));

	if (egreadPCB(sc) != 0) {
		dprintf(("read error\n"));
		egprintpcb(sc);
	} else if (sc->eg_pcb[2] != 0 || sc->eg_pcb[3] != 0)
		printf("%s: configure card command failed\n",
		    sc->sc_dev.dv_xname);

	if (sc->eg_inbuf == NULL)
		sc->eg_inbuf = malloc(EG_BUFLEN, M_TEMP, M_NOWAIT);
	sc->eg_incount = 0;

	if (sc->eg_outbuf == NULL)
		sc->eg_outbuf = malloc(EG_BUFLEN, M_TEMP, M_NOWAIT);
	
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	outb(sc->eg_ctl, EG_CTL_CMDE);

	egstart(ifp);
	egrecv(sc);
}

static int
egrecv(sc)
	struct eg_softc *sc;
{

	while (sc->eg_incount < EG_INLEN) {
		sc->eg_pcb[0] = EG_CMD_RECVPACKET;
		sc->eg_pcb[1] = 0x08;
		sc->eg_pcb[2] = 0; /* address not used.. we send zero */
		sc->eg_pcb[3] = 0;
		sc->eg_pcb[4] = 0;
		sc->eg_pcb[5] = 0;
		sc->eg_pcb[6] = EG_BUFLEN & 0xff; /* our buffer size */
		sc->eg_pcb[7] = (EG_BUFLEN >> 8) & 0xff;
		sc->eg_pcb[8] = 0; /* timeout, 0 == none */
		sc->eg_pcb[9] = 0;
		if (egwritePCB(sc) == 0)
			sc->eg_incount++;
		else
			break;
	}
}

static void
egstart(ifp)
	struct ifnet *ifp;
{
	register struct eg_softc *sc = egcd.cd_devs[ifp->if_unit];
	struct mbuf *m0, *m;
	int len;
	short *ptr;

	/* Don't transmit if interface is busy or not running */
	if ((sc->sc_arpcom.ac_if.if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return 0;

	/* Dequeue the next datagram. */
	IF_DEQUEUE(&sc->sc_arpcom.ac_if.if_snd, m0);
	if (m0 == NULL)
		return 0;
	
	sc->sc_arpcom.ac_if.if_flags |= IFF_OACTIVE;

	/* Copy the datagram to the buffer. */
	len = 0;
	for (m = m0; m; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		if (len + m->m_len > EG_BUFLEN) {
			dprintf(("Packet too large to send\n"));
			m_freem(m0);
			sc->sc_arpcom.ac_if.if_flags &= ~IFF_OACTIVE;
			sc->sc_arpcom.ac_if.if_oerrors++;
			return 0;
		}
		bcopy(mtod(m, caddr_t), sc->eg_outbuf + len, m->m_len);
		len += m->m_len;
	}
#if NBPFILTER > 0
	if (sc->sc_arpcom.ac_if.if_bpf)
		bpf_mtap(sc->sc_arpcom.ac_if.if_bpf, m0);
#endif
	m_freem(m0);
	
	/* length must be a minimum of ETHER_MIN_LEN bytes */
	len = max(len, ETHER_MIN_LEN);

	/* set direction bit: host -> adapter */
	outb(sc->eg_ctl, inb(sc->eg_ctl) & ~EG_CTL_DIR); 
	
	sc->eg_pcb[0] = EG_CMD_SENDPACKET;
	sc->eg_pcb[1] = 0x06;
	sc->eg_pcb[2] = 0; /* address not used, we send zero */
	sc->eg_pcb[3] = 0;
	sc->eg_pcb[4] = 0;
	sc->eg_pcb[5] = 0;
	sc->eg_pcb[6] = len & 0xff; /* length of packet */
	sc->eg_pcb[7] = (len >> 8) & 0xff;
	if (egwritePCB(sc) == 0) {
		for (ptr = (short *) sc->eg_outbuf; len > 0; len -= 2) {
			outw(sc->eg_data, *ptr++);
			while (!(inb(sc->eg_stat) & EG_STAT_HRDY))
				; /* XXX need timeout here */
		}
	} else {
		dprintf(("egwritePCB in egstart failed\n"));
		sc->sc_arpcom.ac_if.if_oerrors++;
		sc->sc_arpcom.ac_if.if_flags &= ~IFF_OACTIVE;
	}
	
	/* Set direction bit : Adapter -> host */
	outb(sc->eg_ctl, inb(sc->eg_ctl) | EG_CTL_DIR); 

	return 0;
}

int
egintr(sc)
	register struct eg_softc *sc;
{
	int i, len;
	short *ptr;

	while (inb(sc->eg_stat) & EG_STAT_ACRF) {
		egreadPCB(sc);
		switch (sc->eg_pcb[0]) {
		case EG_RSP_RECVPACKET:
			len = sc->eg_pcb[6] | (sc->eg_pcb[7] << 8);
			for (ptr = (short *) sc->eg_inbuf; len > 0; len -= 2) {
				while (!(inb(sc->eg_stat) & EG_STAT_HRDY))
					;
				*ptr++ = inw(sc->eg_data);
			}
			len = sc->eg_pcb[8] | (sc->eg_pcb[9] << 8);
			egrecv(sc);
			sc->sc_arpcom.ac_if.if_ipackets++;
			egread(sc, sc->eg_inbuf, len);
			sc->eg_incount--;
			break;

		case EG_RSP_SENDPACKET:
			if (sc->eg_pcb[6] || sc->eg_pcb[7]) {
				dprintf(("packet dropped\n"));
				sc->sc_arpcom.ac_if.if_oerrors++;
			} else
				sc->sc_arpcom.ac_if.if_opackets++;
			sc->sc_arpcom.ac_if.if_collisions += sc->eg_pcb[8] & 0xf;
			sc->sc_arpcom.ac_if.if_flags &= ~IFF_OACTIVE;
			egstart(&sc->sc_arpcom.ac_if);
			break;

		case EG_RSP_GETSTATS:
			dprintf(("Card Statistics\n"));
			bcopy(&sc->eg_pcb[2], &i, sizeof(i));
			dprintf(("Receive Packets %d\n", i));
			bcopy(&sc->eg_pcb[6], &i, sizeof(i));
			dprintf(("Transmit Packets %d\n", i));
			dprintf(("CRC errors %d\n", *(short*) &sc->eg_pcb[10]));
			dprintf(("alignment errors %d\n", *(short*) &sc->eg_pcb[12]));
			dprintf(("no resources errors %d\n", *(short*) &sc->eg_pcb[14]));
			dprintf(("overrun errors %d\n", *(short*) &sc->eg_pcb[16]));
			break;
			
		default:
			dprintf(("egintr: Unknown response %x??\n",
			    sc->eg_pcb[0]));
			egprintpcb(sc);
			break;
		}
	}

	return 0;
}

/*
 * Pass a packet up to the higher levels.
 */
static inline void
egread(sc, buf, len)
	struct eg_softc *sc;
	caddr_t buf;
	int len;
{
	struct ifnet *ifp;
	struct mbuf *m;
	struct ether_header *eh;
	
	if (len <= sizeof(struct ether_header) ||
	    len > ETHER_MAX_LEN) {
		dprintf(("Unacceptable packet size %d\n", len));
		sc->sc_arpcom.ac_if.if_ierrors++;
		return;
	}

	/* Pull packet off interface. */
	ifp = &sc->sc_arpcom.ac_if;
	m = egget(buf, len, ifp);
	if (m == 0) {
		dprintf(("egget returned 0\n"));
		sc->sc_arpcom.ac_if.if_ierrors++;
		return;
	}

	/* We assume the header fit entirely in one mbuf. */
	eh = mtod(m, struct ether_header *);

#if NBPFILTER > 0
	/*
	 * Check if there's a BPF listener on this interface.
	 * If so, hand off the raw packet to BPF.
	 */
	if (ifp->if_bpf) {
		bpf_mtap(ifp->if_bpf, m);

		/*
		 * Note that the interface cannot be in promiscuous mode if
		 * there are no BPF listeners.  And if we are in promiscuous
		 * mode, we have to check if this packet is really ours.
		 */
		if ((ifp->if_flags & IFF_PROMISC) &&
		    (eh->ether_dhost[0] & 1) == 0 && /* !mcast and !bcast */
		    bcmp(eh->ether_dhost, sc->sc_arpcom.ac_enaddr,
			    sizeof(eh->ether_dhost)) != 0) {
			m_freem(m);
			return;
		}
	}
#endif

	/* We assume the header fit entirely in one mbuf. */
	m->m_pkthdr.len -= sizeof(*eh);
	m->m_len -= sizeof(*eh);
	m->m_data += sizeof(*eh);

	ether_input(ifp, eh, m);
}

/*
 * convert buf into mbufs
 */
static struct mbuf *
egget(buf, totlen, ifp)
	caddr_t buf;
	int totlen;
	struct ifnet *ifp;
{
	struct mbuf *top, **mp, *m;
	int len;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0) {
		dprintf(("MGETHDR returns 0\n"));
		return 0;
	}
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = totlen;
	len = MHLEN;
	top = 0;
	mp = &top;

	while (totlen > 0) {
		if (top) {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0) {
				m_freem(top);
				dprintf(("MGET returns 0\n"));
				return 0;
			}
			len = MLEN;
		}
		if (totlen >= MINCLSIZE) {
			MCLGET(m, M_DONTWAIT);
			if (m->m_flags & M_EXT)
				len = MCLBYTES;
		}
		m->m_len = len = min(totlen, len);
		bcopy((caddr_t)buf, mtod(m, caddr_t), len);
		buf += len;
		totlen -= len;
		*mp = m;
		mp = &m->m_next;
	}

	return top;
}

static int
egioctl(ifp, command, data)
	register struct ifnet *ifp;
	u_long command;
	caddr_t data;
{
	struct eg_softc *sc = egcd.cd_devs[ifp->if_unit];
	register struct ifaddr *ifa = (struct ifaddr *)data;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	s = splimp();

	switch (command) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			eginit(sc);
			arp_ifinit(&sc->sc_arpcom, ifa);
			break;
#endif
#ifdef NS
		case AF_NS:
		    {
			register struct ns_addr *ina = &IA_SNS(ifa)->sns_addr;
				
			if (ns_nullhost(*ina))
				ina->x_host =
				    *(union ns_host *)(sc->sc_arpcom.ac_enaddr);
			else
				bcopy(ina->x_host.c_host,
				    sc->sc_arpcom.ac_enaddr,
				    sizeof(sc->sc_arpcom.ac_enaddr));
			/* Set new address. */
			eginit(sc);
			break;
		    }
#endif
		default:
			eginit(sc);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			/*
			 * If interface is marked down and it is running, then
			 * stop it.
			 */
			egstop(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
			   (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			eginit(sc);
		} else {
			sc->eg_pcb[0] = EG_CMD_GETSTATS;
			sc->eg_pcb[1] = 0;
			if (egwritePCB(sc) != 0)
				dprintf(("write error\n"));
			/*
			 * XXX deal with flags changes:
			 * IFF_MULTICAST, IFF_PROMISC,
			 * IFF_LINK0, IFF_LINK1,
			 */
		}
		break;

	default:
		error = EINVAL;
	}

	splx(s);
	return error;
}

static void
egreset(sc)
	struct eg_softc *sc;
{
	int s;

	dprintf(("egreset()\n"));
	s = splimp();
	egstop(sc);
	eginit(sc);
	splx(s);
}

static void
egwatchdog(unit)
	int     unit;
{
	struct eg_softc *sc = egcd.cd_devs[unit];

	log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
	sc->sc_arpcom.ac_if.if_oerrors++;

	egreset(sc);
	return 0;
}

static void
egstop(sc)
	register struct eg_softc *sc;
{
	
	outb(sc->eg_ctl, 0);
}

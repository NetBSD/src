/*	$NetBSD: if_ec.c,v 1.13 2008/01/02 06:53:09 dogcow Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthew Fredette.
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
 * 3Com 3C400 device driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ec.c,v 1.13 2008/01/02 06:53:09 dogcow Exp $");

#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h"
#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/endian.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>

#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
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
#include <machine/autoconf.h>
#include <machine/idprom.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <sun2/dev/if_ecreg.h>

/*
 * Interface softc.
 */
struct ec_softc {
	struct device sc_dev;
	void *sc_ih;

	struct ethercom sc_ethercom;	/* ethernet common */
	struct ifmedia sc_media;	/* our supported media */

	bus_space_tag_t sc_iot;	/* bus space tag */
	bus_space_handle_t sc_ioh;	/* bus space handle */

	u_char sc_jammed;	/* nonzero if the net is jammed */
	u_char sc_colliding;	/* nonzero if the net is colliding */
	uint32_t sc_backoff_seed;	/* seed for the backoff PRNG */

#if NRND > 0
	rndsource_element_t rnd_source;
#endif
};

/* Macros to read and write the CSR. */
#define	ECREG_CSR_RD bus_space_read_2(sc->sc_iot, sc->sc_ioh, ECREG_CSR)
#define	ECREG_CSR_WR(val) bus_space_write_2(sc->sc_iot, sc->sc_ioh, ECREG_CSR, val)

/* After this many collisions, the packet is dropped. */
#define	EC_COLLISIONS_JAMMED		16

/*
 * Various constants used in the backoff pseudorandom
 * number generator.
 */
#define	EC_BACKOFF_PRNG_COLL_MAX	10
#define	EC_BACKOFF_PRNG_MUL		1103515245
#define	EC_BACKOFF_PRNG_ADD		12345
#define	EC_BACKOFF_PRNG_MASK		0x7fffffff

/*
 * Prototypes
 */
int ec_intr(void *);
void ec_reset(struct ifnet *);
int ec_init(struct ifnet *);
int ec_ioctl(struct ifnet *, u_long, void *);
void ec_watchdog(struct ifnet *);
void ec_start(struct ifnet *);

void ec_recv(struct ec_softc *, int);
void ec_coll(struct ec_softc *);
void ec_copyin(struct ec_softc *, void *, int, size_t);
void ec_copyout(struct ec_softc *, const void *, int, size_t);

int ec_mediachange(struct ifnet *);
void ec_mediastatus(struct ifnet *, struct ifmediareq *);

int ec_match(struct device *, struct cfdata *, void *);
void ec_attach(struct device *, struct device *, void *);

CFATTACH_DECL(ec, sizeof(struct ec_softc),
    ec_match, ec_attach, NULL, NULL);

/*
 * Copy board memory to kernel.
 */
void 
ec_copyin(struct ec_softc *sc, void *p, int offset, size_t size)
{
	bus_space_copyin(sc->sc_iot, sc->sc_ioh, offset, p, size);
}

/*
 * Copy from kernel space to board memory.
 */
void 
ec_copyout(struct ec_softc *sc, const void *p, int offset, size_t size)
{
	bus_space_copyout(sc->sc_iot, sc->sc_ioh, offset, p, size);
}

int 
ec_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct mbmem_attach_args *mbma = aux;
	bus_space_handle_t bh;
	int matched;

	/* No default Multibus address. */
	if (mbma->mbma_paddr == -1)
		return (0);

	/* Make sure there is something there... */
	if (bus_space_map(mbma->mbma_bustag, mbma->mbma_paddr, ECREG_BANK_SZ, 
			  0, &bh))
		return (0);
	matched = (bus_space_peek_2(mbma->mbma_bustag, bh, 0, NULL) == 0);
	bus_space_unmap(mbma->mbma_bustag, bh, ECREG_BANK_SZ);
	if (!matched)
		return (0);

	/* Default interrupt priority. */
	if (mbma->mbma_pri == -1)
		mbma->mbma_pri = 3;

	return (1);
}

void 
ec_attach(struct device *parent, struct device *self, void *aux)
{
	struct ec_softc *sc = (void *) self;
	struct mbmem_attach_args *mbma = aux;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint8_t myaddr[ETHER_ADDR_LEN];

	printf("\n");

	/* Map in the board control regs. */
	sc->sc_iot = mbma->mbma_bustag;
	if (bus_space_map(mbma->mbma_bustag, mbma->mbma_paddr, ECREG_BANK_SZ,
		0, &sc->sc_ioh))
		panic("ec_attach: can't map regs");

	/* Reset the board. */
	ECREG_CSR_WR(EC_CSR_RESET);
	delay(160);

	/*
	 * Copy out the board ROM Ethernet address,
	 * and use the non-vendor-ID part to seed
	 * our backoff pseudorandom number generator.
	 */
	bus_space_read_region_1(sc->sc_iot, sc->sc_ioh, ECREG_AROM, myaddr, ETHER_ADDR_LEN);
	sc->sc_backoff_seed = (myaddr[3] << 16) | (myaddr[4] << 8) | (myaddr[5]) | 1;

	/* Initialize ifnet structure. */
	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = ec_start;
	ifp->if_ioctl = ec_ioctl;
	ifp->if_init = ec_init;
	ifp->if_watchdog = ec_watchdog;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS;
	IFQ_SET_READY(&ifp->if_snd);

        /* Initialize ifmedia structures. */
	ifmedia_init(&sc->sc_media, 0, ec_mediachange, ec_mediastatus);
	ifmedia_add(&sc->sc_media, IFM_ETHER|IFM_MANUAL, 0, NULL);
	ifmedia_set(&sc->sc_media, IFM_ETHER|IFM_MANUAL);

	/* Now we can attach the interface. */
	if_attach(ifp);
	idprom_etheraddr(myaddr);
	ether_ifattach(ifp, myaddr);
	printf("%s: address %s\n", self->dv_xname, ether_sprintf(myaddr));

	bus_intr_establish(mbma->mbma_bustag, mbma->mbma_pri, IPL_NET, 0,
	    ec_intr, sc);

#if NRND > 0
	rnd_attach_source(&sc->rnd_source, sc->sc_dev.dv_xname,
	    RND_TYPE_NET, 0);
#endif
}

/*
 * Reset interface.
 */     
void 
ec_reset(struct ifnet *ifp)
{
        int s;

        s = splnet();
        ec_init(ifp);
        splx(s);
}


/*
 * Initialize interface.
 */
int 
ec_init(struct ifnet *ifp)
{
	struct ec_softc *sc = ifp->if_softc;

	/* Reset the board. */
	ECREG_CSR_WR(EC_CSR_RESET);
	delay(160);

	/* Set the Ethernet address. */
	bus_space_write_region_1(sc->sc_iot, sc->sc_ioh, ECREG_ARAM, CLLADDR(sc->sc_ethercom.ec_if.if_sadl), ETHER_ADDR_LEN);
	ECREG_CSR_WR((ECREG_CSR_RD & EC_CSR_INTPA) | EC_CSR_AMSW);
	ECREG_CSR_WR(ECREG_CSR_RD & 0);

	/* Enable interrupts. */
	ECREG_CSR_WR((ECREG_CSR_RD & EC_CSR_INTPA) | EC_CSR_BBSW | EC_CSR_ABSW | EC_CSR_BINT | EC_CSR_AINT | (ifp->if_flags & IFF_PROMISC ? EC_CSR_PROMISC : EC_CSR_PA));

	/* Set flags appropriately. */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	/* Start output. */
	ec_start(ifp);

	return (0);
}

/*
 * Start output on interface.
 */
void 
ec_start(struct ifnet *ifp)
{
	struct ec_softc *sc = ifp->if_softc;
	struct mbuf *m, *m0;
	int s;
	u_int count, realcount;
	bus_size_t off;
	static uint8_t padding[ETHER_MIN_LEN - ETHER_CRC_LEN] = {0};

	s = splnet();

	/* Don't do anything if output is active. */
	if ((ifp->if_flags & IFF_OACTIVE) != 0) {
		splx(s);
		return;
	}
	/* Don't do anything if the output queue is empty. */
	IFQ_DEQUEUE(&ifp->if_snd, m0);
	if (m0 == NULL) {
		splx(s);
		return;
	}

#if NBPFILTER > 0
	/* The BPF tap. */
	if (ifp->if_bpf)
		bpf_mtap(ifp->if_bpf, m0);
#endif

	/* Size the packet. */
	count = EC_BUF_SZ - m0->m_pkthdr.len;

	/* Copy the packet into the xmit buffer. */
	realcount = MIN(count, EC_PKT_MAXTDOFF);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, ECREG_TBUF, realcount);
	for (off = realcount, m = m0; m != 0; off += m->m_len, m = m->m_next)
		ec_copyout(sc, mtod(m, uint8_t *), ECREG_TBUF + off, m->m_len);
	m_freem(m0);
	if (count - realcount)
		ec_copyout(sc, padding, ECREG_TBUF + off, count - realcount);

	/* Enable the transmitter. */
	ECREG_CSR_WR((ECREG_CSR_RD & EC_CSR_PA) | EC_CSR_TBSW | EC_CSR_TINT | EC_CSR_JINT);
	ifp->if_flags |= IFF_OACTIVE;

	/* Done. */
	splx(s);
}

/*
 * Controller interrupt.
 */
int 
ec_intr(void *arg)
{
	struct ec_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int recv_first;
	int recv_second;
	int retval;
	struct mbuf *m0;

	retval = 0;

	/* Check for received packet(s). */
	recv_first = recv_second = 0;
	switch (ECREG_CSR_RD & (EC_CSR_BBSW | EC_CSR_ABSW | EC_CSR_RBBA)) {

	case (EC_CSR_BBSW | EC_CSR_ABSW):
	case (EC_CSR_BBSW | EC_CSR_ABSW | EC_CSR_RBBA):
		/* Neither buffer is full.  Is this a transmit interrupt?
		 * Acknowledge the interrupt ourselves. */
		ECREG_CSR_WR(ECREG_CSR_RD & (EC_CSR_TINT | EC_CSR_JINT | EC_CSR_PAMASK));
		ECREG_CSR_WR((ECREG_CSR_RD & EC_CSR_INTPA) | EC_CSR_BINT | EC_CSR_AINT);
		break;

	case EC_CSR_BBSW:
	case (EC_CSR_BBSW | EC_CSR_RBBA):
		/* Only the A buffer is full. */
		recv_first = EC_CSR_AINT;
		break;

	case EC_CSR_ABSW:
	case (EC_CSR_ABSW | EC_CSR_RBBA):
		/* Only the B buffer is full. */
		recv_first = EC_CSR_BINT;
		break;

	case 0:
		/* Both the A buffer and the B buffer are full, and the A
		 * buffer is older than the B buffer. */
		recv_first = EC_CSR_AINT;
		recv_second = EC_CSR_BINT;
		break;

	case EC_CSR_RBBA:
		/* Both the A buffer and the B buffer are full, and the B
		 * buffer is older than the A buffer. */
		recv_first = EC_CSR_BINT;
		recv_second = EC_CSR_AINT;
		break;
	}

	/* Receive packets. */
	if (recv_first) {

		/* Acknowledge the interrupt. */
		ECREG_CSR_WR(ECREG_CSR_RD & ((EC_CSR_BINT | EC_CSR_AINT | EC_CSR_TINT | EC_CSR_JINT | EC_CSR_PAMASK) ^ (recv_first | recv_second)));

		/* Receive a packet. */
		ec_recv(sc, recv_first);

		/* Receive a packet. */
		if (recv_second)
			ec_recv(sc, recv_second);

		retval++;
	}
	/* Check for a transmitted packet. */
	if (ifp->if_flags & IFF_OACTIVE) {

		/* If we got a collision. */
		if (ECREG_CSR_RD & EC_CSR_JAM) {
			ECREG_CSR_WR(ECREG_CSR_RD & (EC_CSR_BINT | EC_CSR_AINT | EC_CSR_PAMASK));
			sc->sc_ethercom.ec_if.if_collisions++;
			retval++;
			ec_coll(sc);

		}
		/* If we transmitted a packet. */
		else if ((ECREG_CSR_RD & EC_CSR_TBSW) == 0) {
			ECREG_CSR_WR(ECREG_CSR_RD & (EC_CSR_BINT | EC_CSR_AINT | EC_CSR_PAMASK));
			retval++;
			sc->sc_ethercom.ec_if.if_opackets++;
			sc->sc_jammed = 0;
			ifp->if_flags &= ~IFF_OACTIVE;
			IFQ_POLL(&ifp->if_snd, m0);
			if (m0 != NULL)
				ec_start(ifp);
		}
	} else {

		/* Make sure we disable transmitter interrupts. */
		ECREG_CSR_WR(ECREG_CSR_RD & (EC_CSR_BINT | EC_CSR_AINT | EC_CSR_PAMASK));
	}

	return retval;
}

/*
 * Read in a packet from the board.
 */
void 
ec_recv(struct ec_softc *sc, int intbit)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *m0, *m, *newm;
	bus_size_t buf;
	uint16_t status;
	uint16_t doff;
	int length, total_length;

	buf = EC_CSR_INT_BUF(intbit);

	/* Read in the packet status. */
	status = bus_space_read_2(sc->sc_iot, sc->sc_ioh, buf);
	doff = status & EC_PKT_DOFF;

	for (total_length = -1, m0 = 0;;) {

		/* Check for an error. */
		if (status & (EC_PKT_FCSERR | EC_PKT_RGERR | EC_PKT_FRERR) ||
		    doff < EC_PKT_MINRDOFF ||
		    doff > EC_PKT_MAXRDOFF) {
			printf("%s: garbled packet, status 0x%04x; dropping\n",
			    sc->sc_dev.dv_xname, (unsigned int) status);
			break;
		}

		/* Adjust for the header. */
		total_length = doff - EC_PKT_RDOFF;
		buf += EC_PKT_RDOFF;

		/* XXX - sometimes the card reports a large data offset. */
		if (total_length > (ETHER_MAX_LEN - ETHER_CRC_LEN)) {
#ifdef DEBUG
			printf("%s: fixing too-large length of %d\n",
			    sc->sc_dev.dv_xname, total_length);
#endif
			total_length = (ETHER_MAX_LEN - ETHER_CRC_LEN);
		}

		MGETHDR(m0, M_DONTWAIT, MT_DATA);
		if (m0 == 0)
			break;
		m0->m_pkthdr.rcvif = ifp;
		m0->m_pkthdr.len = total_length;
		length = MHLEN;
		m = m0;

		while (total_length > 0) {
			if (total_length >= MINCLSIZE) {
				MCLGET(m, M_DONTWAIT);
				if ((m->m_flags & M_EXT) == 0)
					break;
				length = MCLBYTES;
			}
			m->m_len = length = min(total_length, length);
			ec_copyin(sc, mtod(m, uint8_t *), buf, length);
			total_length -= length;
			buf += length;

			if (total_length > 0) {
				MGET(newm, M_DONTWAIT, MT_DATA);
				if (newm == 0)
					break;
				length = MLEN;
				m = m->m_next = newm;
			}
		}
		break;
	}

	if (total_length == 0) {
		ifp->if_ipackets++;

#if NBPFILTER > 0
		/*
	 	* Check if there's a BPF listener on this interface.
	 	* If so, hand off the raw packet to BPF.
	 	*/
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m0);
#endif

		/* Pass the packet up. */
		(*ifp->if_input) (ifp, m0);

	} else {
		/* Something went wrong. */
		if (m0)
			m_freem(m0);
		ifp->if_ierrors++;
	}

	/* Give the receive buffer back to the card. */
	buf = EC_CSR_INT_BUF(intbit);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, buf, 0);
	ECREG_CSR_WR((ECREG_CSR_RD & EC_CSR_INTPA) | EC_CSR_INT_BSW(intbit) | intbit);
}

int 
ec_mediachange(struct ifnet *ifp)
{               
	return (0);
}

void 
ec_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{  
	if ((ifp->if_flags & IFF_UP) == 0)
		return;
        
	ifmr->ifm_status = IFM_AVALID | IFM_ACTIVE;
}

/*
 * Process an ioctl request. This code needs some work - it looks pretty ugly.
 */
int 
ec_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct ifaddr *ifa = (struct ifaddr *) data;
	struct ifreq *ifr = (struct ifreq *)data;
	struct ec_softc *sc = ifp->if_softc;
	int s, error = 0;

	s = splnet();

	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			ec_init(ifp);
			arp_ifinit(ifp, ifa);
			break;
#endif
#ifdef NS
			/* XXX - This code is probably wrong. */
		case AF_NS:
			{
				struct ns_addr *ina = &IA_SNS(ifa)->sns_addr;

				if (ns_nullhost(*ina))
					ina->x_host =
					    *(union ns_host *) LLADDR(ifp->if_sadl);
				else
					bcopy(ina->x_host.c_host, LLADDR(ifp->if_sadl),
					    ETHER_ADDR_LEN);
				/* Set new address. */
				ec_init(ifp);
				break;
			}
#endif
		default:
			ec_init(ifp);
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
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
		    (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			ec_init(ifp);
		} else {
			/*
			 * Some other important flag might have changed, so
			 * reset.
			 */
			ec_reset(ifp);
		}
		break;

	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_media, cmd);
		break;

	default:
		error = EINVAL;
		break;
	}

	splx(s);
	return error;
}

/*
 * Collision routine.
 */
void 
ec_coll(struct ec_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	u_short jams;
	struct mbuf *m0;

	if ((++sc->sc_colliding) >= EC_COLLISIONS_JAMMED) {
		sc->sc_ethercom.ec_if.if_oerrors++;
		if (!sc->sc_jammed)
			printf("%s: ethernet jammed\n",
			    sc->sc_dev.dv_xname);
		sc->sc_jammed = 1;
		sc->sc_colliding = 0;
		ifp->if_flags &= ~IFF_OACTIVE;
		IFQ_POLL(&ifp->if_snd, m0);
		if (m0 != NULL)
			ec_start(ifp);
	} else {
		jams = MAX(sc->sc_colliding, EC_BACKOFF_PRNG_COLL_MAX);
		sc->sc_backoff_seed = ((sc->sc_backoff_seed * EC_BACKOFF_PRNG_MUL) + EC_BACKOFF_PRNG_ADD) & EC_BACKOFF_PRNG_MASK;
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, ECREG_BACKOFF, -(((sc->sc_backoff_seed >> 8) & ~(-1 << jams)) + 1));
		ECREG_CSR_WR((ECREG_CSR_RD & EC_CSR_INTPA) | EC_CSR_JAM | EC_CSR_TINT | EC_CSR_JINT);
	}
}

/*
 * Device timeout routine.
 */
void 
ec_watchdog(struct ifnet *ifp)
{
	struct ec_softc *sc = ifp->if_softc;

	log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
	sc->sc_ethercom.ec_if.if_oerrors++;

	ec_reset(ifp);
}

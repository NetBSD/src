/*	$NetBSD: mb8795.c,v 1.68 2022/03/17 08:08:03 andvar Exp $	*/
/*
 * Copyright (c) 1998 Darrin B. Jewell
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mb8795.c,v 1.68 2022/03/17 08:08:03 andvar Exp $");

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/rndsource.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/bpf.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#endif

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

/* @@@ this is here for the REALIGN_DMABUF hack below */
#include "nextdmareg.h"
#include "nextdmavar.h"

#include "mb8795reg.h"
#include "mb8795var.h"

#include "bmapreg.h"

#ifdef DEBUG
#define MB8795_DEBUG
#endif

#define PRINTF(x) printf x;
#ifdef MB8795_DEBUG
int mb8795_debug = 0;
#define DPRINTF(x) if (mb8795_debug) printf x;
#else
#define DPRINTF(x)
#endif

extern int turbo;

/*
 * Support for
 * Fujitsu Ethernet Data Link Controller (MB8795)
 * and the Fujitsu Manchester Encoder/Decoder (MB502).
 */

void	mb8795_shutdown(void *);

bus_dmamap_t mb8795_txdma_restart(bus_dmamap_t, void *);
void	mb8795_start_dma(struct mb8795_softc *);

int	mb8795_mediachange(struct ifnet *);
void	mb8795_mediastatus(struct ifnet *, struct ifmediareq *);

void
mb8795_config(struct mb8795_softc *sc, int *media, int nmedia, int defmedia)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	DPRINTF(("%s: mb8795_config()\n", device_xname(sc->sc_dev)));

	/* Initialize ifnet structure. */
	memcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = mb8795_start;
	ifp->if_ioctl = mb8795_ioctl;
	ifp->if_watchdog = mb8795_watchdog;
	ifp->if_flags = IFF_BROADCAST;

	/* Initialize media goo. */
	sc->sc_ethercom.ec_ifmedia = &sc->sc_media;
	ifmedia_init(&sc->sc_media, 0, mb8795_mediachange, mb8795_mediastatus);
	if (media != NULL) {
		int i;
		for (i = 0; i < nmedia; i++)
			ifmedia_add(&sc->sc_media, media[i], 0, NULL);
		ifmedia_set(&sc->sc_media, defmedia);
	} else {
		ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_MANUAL, 0, NULL);
		ifmedia_set(&sc->sc_media, IFM_ETHER | IFM_MANUAL);
	}

	/* Attach the interface. */
	if_attach(ifp);
	ether_ifattach(ifp, sc->sc_enaddr);

	sc->sc_sh = shutdownhook_establish(mb8795_shutdown, sc);
	if (sc->sc_sh == NULL)
		panic("mb8795_config: can't establish shutdownhook");

	rnd_attach_source(&sc->rnd_source, device_xname(sc->sc_dev),
	    RND_TYPE_NET, RND_FLAG_DEFAULT);

	DPRINTF(("%s: leaving mb8795_config()\n", device_xname(sc->sc_dev)));
}

/*
 * Media change callback.
 */
int
mb8795_mediachange(struct ifnet *ifp)
{
	struct mb8795_softc *sc = ifp->if_softc;
	int data;

	if (turbo)
		return 0;

	switch IFM_SUBTYPE(sc->sc_media.ifm_media) {
	case IFM_AUTO:
		if ((bus_space_read_1(sc->sc_bmap_bst, sc->sc_bmap_bsh,
			    BMAP_DATA) & BMAP_DATA_UTPENABLED_MASK) ||
		    !(bus_space_read_1(sc->sc_bmap_bst, sc->sc_bmap_bsh,
			    BMAP_DATA) & BMAP_DATA_UTPCARRIER_MASK)) {
			data = BMAP_DATA_UTPENABLE;
			sc->sc_media.ifm_cur->ifm_data = IFM_ETHER | IFM_10_T;
		} else {
			data = BMAP_DATA_BNCENABLE;
			sc->sc_media.ifm_cur->ifm_data = IFM_ETHER | IFM_10_2;
		}
		break;
	case IFM_10_T:
		data = BMAP_DATA_UTPENABLE;
		break;
	case IFM_10_2:
		data = BMAP_DATA_BNCENABLE;
		break;
	default:
		return 1;
		break;
	}

	bus_space_write_1(sc->sc_bmap_bst, sc->sc_bmap_bsh,
			  BMAP_DDIR, BMAP_DDIR_UTPENABLE_MASK);
	bus_space_write_1(sc->sc_bmap_bst, sc->sc_bmap_bsh,
			  BMAP_DATA, data);

	return 0;
}

/*
 * Media status callback.
 */
void
mb8795_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct mb8795_softc *sc = ifp->if_softc;

	if (turbo)
		return;

	if (IFM_SUBTYPE(ifmr->ifm_active) == IFM_AUTO)
		ifmr->ifm_active = sc->sc_media.ifm_cur->ifm_data;

	if (IFM_SUBTYPE(ifmr->ifm_active) == IFM_10_T) {
		ifmr->ifm_status = IFM_AVALID;
		if (!(bus_space_read_1(sc->sc_bmap_bst, sc->sc_bmap_bsh,
		    BMAP_DATA) & BMAP_DATA_UTPCARRIER_MASK))
			ifmr->ifm_status |= IFM_ACTIVE;
	} else
		ifmr->ifm_status &= ~IFM_AVALID; /* don't know for 10_2 */

	return;
}

/****************************************************************/
#ifdef MB8795_DEBUG
#define XCHR(x) hexdigits[(x) & 0xf]
static void
mb8795_hex_dump(unsigned char *pkt, size_t len)
{
	size_t i, j;

	printf("00000000  ");
	for (i=0; i<len; i++) {
		printf("%c%c ", XCHR(pkt[i]>>4), XCHR(pkt[i]));
		if ((i+1) % 16 == 8) {
			printf(" ");
		}
		if ((i+1) % 16 == 0) {
			printf(" %c", '|');
			for (j=0; j<16; j++) {
				printf("%c",
				    (pkt[i-15+j] >= 32 && pkt[i-15+j] < 127)
				    ? pkt[i-15+j] : '.');
			}
			printf("%c\n%c%c%c%c%c%c%c%c  ", '|',
			    XCHR((i+1)>>28), XCHR((i+1)>>24), XCHR((i+1)>>20),
			    XCHR((i+1)>>16), XCHR((i+1)>>12), XCHR((i+1)>>8),
			    XCHR((i+1)>>4), XCHR(i+1));
		}
	}
	printf("\n");
}
#undef XCHR
#endif

/*
 * Controller receive interrupt.
 */
void
mb8795_rint(struct mb8795_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int error = 0;
	u_char rxstat;
	u_char rxmask;

	rxstat = MB_READ_REG(sc, MB8795_RXSTAT);
	rxmask = MB_READ_REG(sc, MB8795_RXMASK);
	__USE(rxmask);

	MB_WRITE_REG(sc, MB8795_RXSTAT, MB8795_RXSTAT_CLEAR);

	if (rxstat & MB8795_RXSTAT_RESET) {
		DPRINTF(("%s: rx reset packet\n", device_xname(sc->sc_dev)));
		error++;
	}
	if (rxstat & MB8795_RXSTAT_SHORT) {
		DPRINTF(("%s: rx short packet\n", device_xname(sc->sc_dev)));
		error++;
	}
	if (rxstat & MB8795_RXSTAT_ALIGNERR) {
		DPRINTF(("%s: rx alignment error\n",device_xname(sc->sc_dev)));
#if 0
		error++;
#endif
	}
	if (rxstat & MB8795_RXSTAT_CRCERR) {
		DPRINTF(("%s: rx CRC error\n", device_xname(sc->sc_dev)));
#if 0
		error++;
#endif
	}
	if (rxstat & MB8795_RXSTAT_OVERFLOW) {
		DPRINTF(("%s: rx overflow error\n", device_xname(sc->sc_dev)));
#if 0
		error++;
#endif
	}

	if (error) {
		if_statinc(ifp, if_ierrors);
		/* @@@ handle more gracefully, free memory, etc. */
	}

	if (rxstat & MB8795_RXSTAT_OK) {
		struct mbuf *m;
		int s;
		s = spldma();

		while ((m = MBDMA_RX_MBUF (sc))) {
			/* CRC is included with the packet; trim it. */
			m->m_pkthdr.len = m->m_len = m->m_len - ETHER_CRC_LEN;
			m_set_rcvif(m, ifp);

			/* Find receive length, keep crc */
			/* enable DMA interrupts while we process the packet */
			splx(s);

#if defined(MB8795_DEBUG)
			/* Peek at the packet */
			DPRINTF(("%s: received packet, at VA %p-%p,len %d\n",
				device_xname(sc->sc_dev), mtod(m,u_char *),
				mtod(m,u_char *) + m->m_len, m->m_len));
			if (mb8795_debug > 3) {
				mb8795_hex_dump(mtod(m,u_char *),
				    m->m_pkthdr.len);
			} else if (mb8795_debug > 2) {
				mb8795_hex_dump(mtod(m,u_char *),
				    m->m_pkthdr.len < 255
				    ? m->m_pkthdr.len : 128);
			}
#endif

			/* Pass the packet up. */
			if_percpuq_enqueue(ifp->if_percpuq, m);

			s = spldma();
		}

		splx(s);

	}

#ifdef MB8795_DEBUG
	if (mb8795_debug) {
		char sbuf[256];

		snprintb(sbuf, sizeof(sbuf), MB8795_RXSTAT_BITS, rxstat);
		printf("%s: rx interrupt, rxstat = %s\n",
		       device_xname(sc->sc_dev), sbuf);

		snprintb(sbuf, sizeof(sbuf), MB8795_RXSTAT_BITS,
		    MB_READ_REG(sc, MB8795_RXSTAT));

		printf("rxstat = %s\n", sbuf);

		snprintb(sbuf, sizeof(sbuf), MB8795_RXMASK_BITS,
		    MB_READ_REG(sc, MB8795_RXMASK));
		printf("rxmask = %s\n", sbuf);

		snprintb(sbuf, sizeof(sbuf), MB8795_RXMODE_BITS,
		    MB_READ_REG(sc, MB8795_RXMODE));
		printf("rxmode = %s\n", sbuf);
	}
#endif

	return;
}

/*
 * Controller transmit interrupt.
 */
void
mb8795_tint(struct mb8795_softc *sc)
{
	u_char txstat;
	u_char txmask;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	panic ("tint");
	txstat = MB_READ_REG(sc, MB8795_TXSTAT);
	txmask = MB_READ_REG(sc, MB8795_TXMASK);
	__USE(txmask);

	if ((txstat & MB8795_TXSTAT_READY) ||
	    (txstat & MB8795_TXSTAT_TXRECV)) {
		/* printf("X"); */
		MB_WRITE_REG(sc, MB8795_TXSTAT, MB8795_TXSTAT_CLEAR);
		/* MB_WRITE_REG(sc, MB8795_TXMASK, txmask & ~MB8795_TXMASK_READYIE); */
		/* MB_WRITE_REG(sc, MB8795_TXMASK, txmask & ~MB8795_TXMASK_TXRXIE); */
		MB_WRITE_REG(sc, MB8795_TXMASK, 0);
		if ((ifp->if_flags & IFF_RUNNING)
		    && !IF_IS_EMPTY(&sc->sc_tx_snd)) {
			void mb8795_start_dma(struct mb8795_softc *); /* XXX */
			/* printf ("Z"); */
			mb8795_start_dma(sc);
		}
		return;
	}

	if (txstat & MB8795_TXSTAT_SHORTED) {
		printf("%s: tx cable shorted\n", device_xname(sc->sc_dev));
		if_statinc(ifp, if_oerrors);
	}
	if (txstat & MB8795_TXSTAT_UNDERFLOW) {
		printf("%s: tx underflow\n", device_xname(sc->sc_dev));
		if_statinc(ifp, if_oerrors);
	}
	if (txstat & MB8795_TXSTAT_COLLERR) {
		DPRINTF(("%s: tx collision\n", device_xname(sc->sc_dev)));
		if_statinc(ifp, if_collisions);
	}
	if (txstat & MB8795_TXSTAT_COLLERR16) {
		printf("%s: tx 16th collision\n", device_xname(sc->sc_dev));
		if_statinc(ifp, if_oerrors);
		if_statadd(ifp, if_collisions, 16);
	}

#if 0
	if (txstat & MB8795_TXSTAT_READY) {
		char sbuf[256];

		snprintb(sbuf, sizeof(sbuf), MB8795_TXSTAT_BITS, txstat);
		panic("%s: unexpected tx interrupt %s",
				device_xname(sc->sc_dev), sbuf);

		/* turn interrupt off */
		MB_WRITE_REG(sc, MB8795_TXMASK,
		    txmask & ~MB8795_TXMASK_READYIE);
	}
#endif

	return;
}

/****************************************************************/

void
mb8795_reset(struct mb8795_softc *sc)
{
	int s;
	int i;

	s = splnet();

	DPRINTF (("%s: mb8795_reset()\n", device_xname(sc->sc_dev)));

	sc->sc_ethercom.ec_if.if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	sc->sc_ethercom.ec_if.if_timer = 0;

	MBDMA_RESET(sc);

	MB_WRITE_REG(sc, MB8795_RESET,  MB8795_RESET_MODE);

	mb8795_mediachange(&sc->sc_ethercom.ec_if);

#if 0 /* This interrupt was sometimes failing to ack correctly
       * causing a loop @@@
       */
	MB_WRITE_REG(sc, MB8795_TXMASK,
	    MB8795_TXMASK_UNDERFLOWIE | MB8795_TXMASK_COLLIE
	    | MB8795_TXMASK_COLL16IE | MB8795_TXMASK_PARERRIE);
#else
	MB_WRITE_REG(sc, MB8795_TXMASK, 0);
#endif
	MB_WRITE_REG(sc, MB8795_TXSTAT, MB8795_TXSTAT_CLEAR);

#if 0
	MB_WRITE_REG(sc, MB8795_RXMASK,
	    MB8795_RXMASK_OKIE | MB8795_RXMASK_RESETIE | MB8795_RXMASK_SHORTIE
	    | MB8795_RXMASK_ALIGNERRIE | MB8795_RXMASK_CRCERRIE
	    | MB8795_RXMASK_OVERFLOWIE);
#else
	MB_WRITE_REG(sc, MB8795_RXMASK,
	    MB8795_RXMASK_OKIE | MB8795_RXMASK_RESETIE
	    | MB8795_RXMASK_SHORTIE);
#endif

	MB_WRITE_REG(sc, MB8795_RXSTAT, MB8795_RXSTAT_CLEAR);

	for (i=0;i<sizeof(sc->sc_enaddr);i++)
		MB_WRITE_REG(sc, MB8795_ENADDR+i, sc->sc_enaddr[i]);

	DPRINTF(("%s: initializing ethernet %02x:%02x:%02x:%02x:%02x:%02x, "
		"size=%d\n", device_xname(sc->sc_dev),
		 sc->sc_enaddr[0],sc->sc_enaddr[1],sc->sc_enaddr[2],
		 sc->sc_enaddr[3],sc->sc_enaddr[4],sc->sc_enaddr[5],
		 sizeof(sc->sc_enaddr)));

	MB_WRITE_REG(sc, MB8795_RESET, 0);

	splx(s);
}

void
mb8795_watchdog(struct ifnet *ifp)
{
	struct mb8795_softc *sc = ifp->if_softc;

	log(LOG_ERR, "%s: device timeout\n", device_xname(sc->sc_dev));
	if_statinc(ifp, if_oerrors);

	ifp->if_flags &= ~IFF_RUNNING;
	mb8795_init(sc);
}

/*
 * Initialization of interface; set up initialization block
 * and transmit/receive descriptor rings.
 */
void
mb8795_init(struct mb8795_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int s;

	DPRINTF (("%s: mb8795_init()\n", device_xname(sc->sc_dev)));

	if (ifp->if_flags & IFF_UP) {
		int rxmode;

		s = spldma();
		if ((ifp->if_flags & IFF_RUNNING) == 0)
			mb8795_reset(sc);

		if (ifp->if_flags & IFF_PROMISC)
			rxmode = MB8795_RXMODE_PROMISCUOUS;
		else
			rxmode = MB8795_RXMODE_NORMAL;
		/* XXX add support for multicast */
		if (turbo)
			rxmode |= MB8795_RXMODE_TEST;

		/* switching mode probably borken now with turbo */
		MB_WRITE_REG(sc, MB8795_TXMODE,
		    turbo ? MB8795_TXMODE_TURBO1 : MB8795_TXMODE_LB_DISABLE);
		MB_WRITE_REG(sc, MB8795_RXMODE, rxmode);

		if ((ifp->if_flags & IFF_RUNNING) == 0) {
			MBDMA_RX_SETUP(sc);
			MBDMA_TX_SETUP(sc);

			ifp->if_flags |= IFF_RUNNING;
			ifp->if_flags &= ~IFF_OACTIVE;
			ifp->if_timer = 0;

			MBDMA_RX_GO(sc);
		}
		splx(s);
#if 0
		s = spldma();
		if (! IF_IS_EMPTY(&sc->sc_tx_snd))
			mb8795_start_dma(ifp);
		splx(s);
#endif
	} else
		mb8795_reset(sc);
}

void
mb8795_shutdown(void *arg)
{
	struct mb8795_softc *sc = (struct mb8795_softc *)arg;

	DPRINTF(("%s: mb8795_shutdown()\n", device_xname(sc->sc_dev)));

	mb8795_reset(sc);
}

/****************************************************************/
int
mb8795_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct mb8795_softc *sc = ifp->if_softc;
	struct ifaddr *ifa = (struct ifaddr *)data;
	int s, error = 0;

	s = splnet();

	DPRINTF(("%s: mb8795_ioctl()\n", device_xname(sc->sc_dev)));

	switch (cmd) {

	case SIOCINITIFADDR:
		DPRINTF(("%s: mb8795_ioctl() SIOCINITIFADDR\n",
			device_xname(sc->sc_dev)));
		ifp->if_flags |= IFF_UP;

		mb8795_init(sc);
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			arp_ifinit(ifp, ifa);
			break;
#endif
		default:
			break;
		}
		break;


	case SIOCSIFFLAGS:
		DPRINTF(("%s: mb8795_ioctl() SIOCSIFFLAGS\n",
			device_xname(sc->sc_dev)));
		if ((error = ifioctl_common(ifp, cmd, data)) != 0)
			break;
		switch (ifp->if_flags & (IFF_UP | IFF_RUNNING)) {
		case IFF_RUNNING:
			/*
			 * If interface is marked down and it is running, then
			 * stop it.
			 */
#if 0
 			ifp->if_flags &= ~IFF_RUNNING;
#endif
			mb8795_reset(sc);
			break;
		case IFF_UP:
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			mb8795_init(sc);
			break;
		default:
			/*
			 * Reset the interface to pick up changes in any other
			 * flags that affect hardware registers.
			 */
			mb8795_init(sc);
			break;
		}
#ifdef MB8795_DEBUG
		if (ifp->if_flags & IFF_DEBUG)
			sc->sc_debug = 1;
		else
			sc->sc_debug = 0;
#endif
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		DPRINTF(("%s: mb8795_ioctl() SIOCADDMULTI\n",
		    device_xname(sc->sc_dev)));
		if ((error = ether_ioctl(ifp, cmd, data)) == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			if (ifp->if_flags & IFF_RUNNING)
				mb8795_init(sc);
			error = 0;
		}
		break;

	default:
		error = ether_ioctl(ifp, cmd, data);
		break;
	}

	splx(s);

#if 0
	DPRINTF(("DEBUG: mb8795_ioctl(0x%lx) returning %d\n", cmd, error));
#endif

	return error;
}

/*
 * Setup output on interface.
 * Get another datagram to send off of the interface queue, and map it to the
 * interface before starting the output.
 * Called only at splnet or interrupt level.
 */
void
mb8795_start(struct ifnet *ifp)
{
	struct mb8795_softc *sc = ifp->if_softc;
	struct mbuf *m;
	int s;

	DPRINTF(("%s: mb8795_start()\n", device_xname(sc->sc_dev)));

#ifdef DIAGNOSTIC
	IFQ_POLL(&ifp->if_snd, m);
	if (m == 0)
		panic("%s: No packet to start", device_xname(sc->sc_dev));
#endif

	while (1) {
		if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE))
		    != IFF_RUNNING)
			return;

#if 0
		return;	/* @@@ Turn off xmit for debugging */
#endif

		ifp->if_flags |= IFF_OACTIVE;

		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == 0) {
			ifp->if_flags &= ~IFF_OACTIVE;
			return;
		}

		/* Pass packet to bpf if there is a listener. */
		bpf_mtap(ifp, m, BPF_D_OUT);

		s = spldma();
		IF_ENQUEUE(&sc->sc_tx_snd, m);
		if (!MBDMA_TX_ISACTIVE(sc))
			mb8795_start_dma(sc);
		splx(s);

		ifp->if_flags &= ~IFF_OACTIVE;
	}
}

void
mb8795_start_dma(struct mb8795_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *m;
	u_char txmask;

	DPRINTF(("%s: mb8795_start_dma()\n", device_xname(sc->sc_dev)));

#if (defined(DIAGNOSTIC))
	{
		u_char txstat;
		txstat = MB_READ_REG(sc, MB8795_TXSTAT);
		if (!turbo && !(txstat & MB8795_TXSTAT_READY)) {
			/*
			 * @@@ I used to panic here, but then it panicked once.
			 * Let's see if I can just reset instead.
			 * [ dbj 980706.1900 ]
			 */
			printf("%s: transmitter not ready\n",
				device_xname(sc->sc_dev));
			ifp->if_flags &= ~IFF_RUNNING;
			mb8795_init(sc);
			return;
		}
	}
#endif

#if 0
	return;	/* @@@ Turn off xmit for debugging */
#endif

	IF_DEQUEUE(&sc->sc_tx_snd, m);
	if (m == 0) {
#ifdef DIAGNOSTIC
		panic("%s: No packet to start_dma", device_xname(sc->sc_dev));
#endif
		return;
	}

	MB_WRITE_REG(sc, MB8795_TXSTAT, MB8795_TXSTAT_CLEAR);
	txmask = MB_READ_REG(sc, MB8795_TXMASK);
	__USE(txmask);
	/* MB_WRITE_REG(sc, MB8795_TXMASK, txmask | MB8795_TXMASK_READYIE); */
	/* MB_WRITE_REG(sc, MB8795_TXMASK, txmask | MB8795_TXMASK_TXRXIE); */

	ifp->if_timer = 5;

	if (MBDMA_TX_MBUF(sc, m))
		return;

	MBDMA_TX_GO(sc);
	if (turbo)
		MB_WRITE_REG(sc, MB8795_TXMODE,
		    MB8795_TXMODE_TURBO1 | MB8795_TXMODE_TURBOSTART);

	if_statinc(ifp, if_opackets);
}

/****************************************************************/

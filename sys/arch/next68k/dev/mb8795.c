/*	$NetBSD: mb8795.c,v 1.22 2001/04/16 14:12:12 dbj Exp $	*/
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Darrin B. Jewell
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include "opt_inet.h"
#include "opt_ccitt.h"
#include "opt_llc.h"
#include "opt_ns.h"
#include "bpfilter.h"
#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h> 
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>

#if 0
#include <net/if_media.h>
#endif

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if defined(CCITT) && defined(LLC)
#include <sys/socketvar.h>
#include <netccitt/x25.h>
#include <netccitt/pk.h>
#include <netccitt/pk_var.h>
#include <netccitt/pk_extern.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

/* @@@ this is here for the REALIGN_DMABUF hack below */
#include "nextdmareg.h"
#include "nextdmavar.h"

#include "mb8795reg.h"
#include "mb8795var.h"

#if 1
#define XE_DEBUG
#endif

#ifdef XE_DEBUG
int xe_debug = 0;
#define DPRINTF(x) if (xe_debug) printf x;
#else
#define DPRINTF(x)
#endif


/*
 * Support for
 * Fujitsu Ethernet Data Link Controller (MB8795)
 * and the Fujitsu Manchester Encoder/Decoder (MB502).
 */

void mb8795_shutdown __P((void *));

struct mbuf * mb8795_rxdmamap_load __P((struct mb8795_softc *,
		bus_dmamap_t map));

bus_dmamap_t mb8795_rxdma_continue __P((void *));
void mb8795_rxdma_completed __P((bus_dmamap_t,void *));
bus_dmamap_t mb8795_txdma_continue __P((void *));
void mb8795_txdma_completed __P((bus_dmamap_t,void *));
void mb8795_rxdma_shutdown __P((void *));
void mb8795_txdma_shutdown __P((void *));
bus_dmamap_t mb8795_txdma_restart __P((bus_dmamap_t,void *));

void
mb8795_config(sc)
     struct mb8795_softc *sc;
{
  struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	DPRINTF(("%s: mb8795_config()\n",sc->sc_dev.dv_xname));

  /* Initialize ifnet structure. */
  bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
  ifp->if_softc = sc;
  ifp->if_start = mb8795_start;
  ifp->if_ioctl = mb8795_ioctl;
  ifp->if_watchdog = mb8795_watchdog;
  ifp->if_flags =
    IFF_BROADCAST | IFF_NOTRAILERS;

  /* Attach the interface. */
  if_attach(ifp);
  ether_ifattach(ifp, sc->sc_enaddr);

	/* decrease the mtu on this interface to deal with
	 * alignment problems
	 */
	ifp->if_mtu -= 16;

  sc->sc_sh = shutdownhook_establish(mb8795_shutdown, sc);
  if (sc->sc_sh == NULL)
    panic("mb8795_config: can't establish shutdownhook");

#if NRND > 0
  rnd_attach_source(&sc->rnd_source, sc->sc_dev.dv_xname,
                    RND_TYPE_NET, 0);
#endif

  /* Initialize the dma maps */
  {
    int error;
    if ((error = bus_dmamap_create(sc->sc_tx_dmat, MCLBYTES,
		    (MCLBYTES/MSIZE), MCLBYTES, 0, BUS_DMA_ALLOCNOW,
				&sc->sc_tx_dmamap)) != 0) {
      panic("%s: can't create tx DMA map, error = %d\n",
					sc->sc_dev.dv_xname, error);
    }
		{
			int i;
			for(i=0;i<MB8795_NRXBUFS;i++) {
				if ((error = bus_dmamap_create(sc->sc_rx_dmat, MCLBYTES,
						(MCLBYTES/MSIZE), MCLBYTES, 0, BUS_DMA_ALLOCNOW,
						&sc->sc_rx_dmamap[i])) != 0) {
					panic("%s: can't create rx DMA map, error = %d\n",
							sc->sc_dev.dv_xname, error);
				}
				sc->sc_rx_mb_head[i] = NULL;
			}
			sc->sc_rx_loaded_idx = 0;
			sc->sc_rx_completed_idx = 0;
			sc->sc_rx_handled_idx = 0;
    }
  }

	/* @@@ more next hacks 
	 * the  2000 covers at least a 1500 mtu + headers
	 * + DMA_BEGINALIGNMENT+ DMA_ENDALIGNMENT
	 */
	sc->sc_txbuf = malloc(2000, M_DEVBUF, M_NOWAIT);
	if (!sc->sc_txbuf) panic("%s: can't malloc tx DMA buffer",
			sc->sc_dev.dv_xname);

	sc->sc_tx_mb_head = NULL;
	sc->sc_tx_loaded = 0;

	sc->sc_tx_nd->nd_shutdown_cb = mb8795_txdma_shutdown;
	sc->sc_tx_nd->nd_continue_cb = mb8795_txdma_continue;
	sc->sc_tx_nd->nd_completed_cb = mb8795_txdma_completed;
	sc->sc_tx_nd->nd_cb_arg = sc;

	sc->sc_rx_nd->nd_shutdown_cb = mb8795_rxdma_shutdown;
	sc->sc_rx_nd->nd_continue_cb = mb8795_rxdma_continue;
	sc->sc_rx_nd->nd_completed_cb = mb8795_rxdma_completed;
	sc->sc_rx_nd->nd_cb_arg = sc;

	DPRINTF(("%s: leaving mb8795_config()\n",sc->sc_dev.dv_xname));
}


/****************************************************************/
#ifdef XE_DEBUG
#define XCHR(x) "0123456789abcdef"[(x) & 0xf]
static void
xe_hex_dump(unsigned char *pkt, size_t len)
{
	size_t i, j;

	printf("00000000  ");
	for(i=0; i<len; i++) {
		printf("%c%c ", XCHR(pkt[i]>>4), XCHR(pkt[i]));
		if ((i+1) % 16 == 8) {
			printf(" ");
		}
		if ((i+1) % 16 == 0) {
			printf(" %c", '|');
			for(j=0; j<16; j++) {
				printf("%c", pkt[i-15+j]>=32 && pkt[i-15+j]<127?pkt[i-15+j]:'.');
			}
			printf("%c\n%c%c%c%c%c%c%c%c  ", '|', 
					XCHR((i+1)>>28),XCHR((i+1)>>24),XCHR((i+1)>>20),XCHR((i+1)>>16),
					XCHR((i+1)>>12), XCHR((i+1)>>8), XCHR((i+1)>>4), XCHR(i+1));
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
mb8795_rint(sc)
     struct mb8795_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int error = 0;
	u_char rxstat;
	u_char rxmask;

	rxstat = bus_space_read_1(sc->sc_bst,sc->sc_bsh, XE_RXSTAT);
	rxmask = bus_space_read_1(sc->sc_bst,sc->sc_bsh, XE_RXMASK);

	bus_space_write_1(sc->sc_bst,sc->sc_bsh, XE_RXSTAT, XE_RXSTAT_CLEAR);

	if (rxstat & XE_RXSTAT_RESET) {
		DPRINTF(("%s: rx reset packet\n",
				sc->sc_dev.dv_xname));
		error++;
	}
	if (rxstat & XE_RXSTAT_SHORT) {
		DPRINTF(("%s: rx short packet\n",
				sc->sc_dev.dv_xname));
		error++;
	}
	if (rxstat & XE_RXSTAT_ALIGNERR) {
		DPRINTF(("%s: rx alignment error\n",
				sc->sc_dev.dv_xname));
#if 0
		error++;
#endif
	}
	if (rxstat & XE_RXSTAT_CRCERR) {
		DPRINTF(("%s: rx CRC error\n",
				sc->sc_dev.dv_xname));
#if 0
		error++;
#endif
	}
	if (rxstat & XE_RXSTAT_OVERFLOW) {
		DPRINTF(("%s: rx overflow error\n",
				sc->sc_dev.dv_xname));
#if 0
		error++;
#endif
	}

	if (error) {
		ifp->if_ierrors++;
		/* @@@ handle more gracefully, free memory, etc. */
	}

	if (rxstat & XE_RXSTAT_OK) {
		int s;
		s = spldma();

		while(sc->sc_rx_handled_idx != sc->sc_rx_completed_idx) {
			struct mbuf *m;
			bus_dmamap_t map;

			sc->sc_rx_handled_idx++;
			sc->sc_rx_handled_idx %= MB8795_NRXBUFS;

			/* Should probably not do this much while interrupts
			 * are disabled, but for now we will.
			 */

			map = sc->sc_rx_dmamap[sc->sc_rx_handled_idx];
			m = sc->sc_rx_mb_head[sc->sc_rx_handled_idx];

			m->m_pkthdr.len = m->m_len = map->dm_xfer_len;
			m->m_flags |= M_HASFCS;
			m->m_pkthdr.rcvif = ifp;

			bus_dmamap_sync(sc->sc_rx_dmat, map,
					0, map->dm_mapsize, BUS_DMASYNC_POSTREAD);

			bus_dmamap_unload(sc->sc_rx_dmat, map);

			/* Install a fresh mbuf for next packet */
			
			sc->sc_rx_mb_head[sc->sc_rx_handled_idx] = 
					mb8795_rxdmamap_load(sc,map);

			/* Punt runt packets
			 * dma restarts create 0 length packets for example
			 */
			if (m->m_len < ETHER_MIN_LEN) {
				m_freem(m);
				continue;
			}

			/* Find receive length, keep crc */
			/* enable dma interrupts while we process the packet */
			splx(s);

#if defined(XE_DEBUG)
			/* Peek at the packet */
			DPRINTF(("%s: received packet, at VA 0x%08x-0x%08x,len %d\n",
					sc->sc_dev.dv_xname,mtod(m,u_char *),mtod(m,u_char *)+m->m_len,m->m_len));
			if (xe_debug > 3) {
				xe_hex_dump(mtod(m,u_char *), m->m_pkthdr.len);
			} else if (xe_debug > 2) {
				xe_hex_dump(mtod(m,u_char *), m->m_pkthdr.len < 255 ? m->m_pkthdr.len : 128 );
			}
#endif

#if NBPFILTER > 0
			/*
			 * Pass packet to bpf if there is a listener.
			 */
			if (ifp->if_bpf)
				bpf_mtap(ifp->if_bpf, m);
#endif

			{
				ifp->if_ipackets++;

				/* Pass the packet up. */
				(*ifp->if_input)(ifp, m);
			}

			s = spldma();

		}

		splx(s);

	}

#ifdef XE_DEBUG
	if (xe_debug) {
		char sbuf[256];

		bitmask_snprintf(rxstat, XE_RXSTAT_BITS, sbuf, sizeof(sbuf));
		printf("%s: rx interrupt, rxstat = %s\n",
		       sc->sc_dev.dv_xname, sbuf);

		bitmask_snprintf(bus_space_read_1(sc->sc_bst,sc->sc_bsh, XE_RXSTAT),
				 XE_RXSTAT_BITS, sbuf, sizeof(sbuf));
		printf("rxstat = 0x%s\n", sbuf);

		bitmask_snprintf(bus_space_read_1(sc->sc_bst,sc->sc_bsh, XE_RXMASK),
				 XE_RXMASK_BITS, sbuf, sizeof(sbuf));
		printf("rxmask = 0x%s\n", sbuf);

		bitmask_snprintf(bus_space_read_1(sc->sc_bst,sc->sc_bsh, XE_RXMODE),
				 XE_RXMODE_BITS, sbuf, sizeof(sbuf));
		printf("rxmode = 0x%s\n", sbuf);
	}
#endif

	return;
}

/*
 * Controller transmit interrupt.
 */
void
mb8795_tint(sc)
     struct mb8795_softc *sc;
	
{
	u_char txstat;
	u_char txmask;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	txstat = bus_space_read_1(sc->sc_bst,sc->sc_bsh, XE_TXSTAT);
	txmask = bus_space_read_1(sc->sc_bst,sc->sc_bsh, XE_TXMASK);

	if (txstat & XE_TXSTAT_SHORTED) {
		printf("%s: tx cable shorted\n", sc->sc_dev.dv_xname);
		ifp->if_oerrors++;
	}
	if (txstat & XE_TXSTAT_UNDERFLOW) {
		printf("%s: tx underflow\n", sc->sc_dev.dv_xname);
		ifp->if_oerrors++;
	}
	if (txstat & XE_TXSTAT_COLLERR) {
		DPRINTF(("%s: tx collision\n", sc->sc_dev.dv_xname));
		ifp->if_collisions++;
	}
	if (txstat & XE_TXSTAT_COLLERR16) {
		printf("%s: tx 16th collision\n", sc->sc_dev.dv_xname);
		ifp->if_oerrors++;
		ifp->if_collisions += 16;
	}

#if 0
	if (txstat & XE_TXSTAT_READY) {
		char sbuf[256];

		bitmask_snprintf(txstat, XE_TXSTAT_BITS, sbuf, sizeof(sbuf));
		panic("%s: unexpected tx interrupt %s",
				sc->sc_dev.dv_xname, sbuf);

		/* turn interrupt off */
		bus_space_write_1(sc->sc_bst,sc->sc_bsh, XE_TXMASK, 
				txmask & ~XE_TXMASK_READYIE);
	}
#endif

  return;
}

/****************************************************************/

void
mb8795_reset(sc)
	struct mb8795_softc *sc;
{
	int s;

	s = splnet();
	mb8795_init(sc);
	splx(s);
}

void
mb8795_watchdog(ifp)
	struct ifnet *ifp;
{
	struct mb8795_softc *sc = ifp->if_softc;

	log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
	++ifp->if_oerrors;

	DPRINTF(("%s: %d input errors, %d input packets\n",
			sc->sc_dev.dv_xname, ifp->if_ierrors, ifp->if_ipackets));

	mb8795_reset(sc);
}

/*
 * Initialization of interface; set up initialization block
 * and transmit/receive descriptor rings.
 * @@@ error handling is bogus in here. memory leaks
 */
void
mb8795_init(sc)
     struct mb8795_softc *sc;
{
  struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	m_freem(sc->sc_tx_mb_head);
	sc->sc_tx_mb_head = NULL;
	sc->sc_tx_loaded = 0;

	{
		int i;
		for(i=0;i<MB8795_NRXBUFS;i++) {
			if (sc->sc_rx_mb_head[i]) {
				bus_dmamap_unload(sc->sc_rx_dmat, sc->sc_rx_dmamap[i]);
				m_freem(sc->sc_rx_mb_head[i]);
			}
			sc->sc_rx_mb_head[i] = 
					mb8795_rxdmamap_load(sc, sc->sc_rx_dmamap[i]);
		}
		sc->sc_rx_loaded_idx = 0;
		sc->sc_rx_completed_idx = 0;
		sc->sc_rx_handled_idx = 0;
	}

  bus_space_write_1(sc->sc_bst,sc->sc_bsh, XE_RESET,  XE_RESET_MODE);

  bus_space_write_1(sc->sc_bst,sc->sc_bsh, XE_TXMODE, XE_TXMODE_LB_DISABLE);
#if 0 /* This interrupt was sometimes failing to ack correctly
			 * causing a loop @@@
			 */
  bus_space_write_1(sc->sc_bst,sc->sc_bsh, XE_TXMASK, 
			XE_TXMASK_UNDERFLOWIE | XE_TXMASK_COLLIE | XE_TXMASK_COLL16IE
			| XE_TXMASK_PARERRIE);
#else
  bus_space_write_1(sc->sc_bst,sc->sc_bsh, XE_TXMASK, 0);
#endif
  bus_space_write_1(sc->sc_bst,sc->sc_bsh, XE_TXSTAT, XE_TXSTAT_CLEAR);

  bus_space_write_1(sc->sc_bst,sc->sc_bsh, XE_RXMODE, XE_RXMODE_NORMAL);

#if 0
  bus_space_write_1(sc->sc_bst,sc->sc_bsh, XE_RXMASK,
			XE_RXMASK_OKIE | XE_RXMASK_RESETIE | XE_RXMASK_SHORTIE	|
			XE_RXMASK_ALIGNERRIE	|  XE_RXMASK_CRCERRIE | XE_RXMASK_OVERFLOWIE);
#else
  bus_space_write_1(sc->sc_bst,sc->sc_bsh, XE_RXMASK,
			XE_RXMASK_OKIE | XE_RXMASK_RESETIE | XE_RXMASK_SHORTIE);
#endif

  bus_space_write_1(sc->sc_bst,sc->sc_bsh, XE_RXSTAT, XE_RXSTAT_CLEAR);

	{
		int i;
		for(i=0;i<sizeof(sc->sc_enaddr);i++) {
			bus_space_write_1(sc->sc_bst,sc->sc_bsh,XE_ENADDR+i,sc->sc_enaddr[i]);
		}
	}

	DPRINTF(("%s: initializing ethernet %02x:%02x:%02x:%02x:%02x:%02x, size=%d\n",
			sc->sc_dev.dv_xname,
			sc->sc_enaddr[0],sc->sc_enaddr[1],sc->sc_enaddr[2],
			sc->sc_enaddr[3],sc->sc_enaddr[4],sc->sc_enaddr[5],
			sizeof(sc->sc_enaddr)));

  bus_space_write_1(sc->sc_bst,sc->sc_bsh, XE_RESET, 0);

  ifp->if_flags |= IFF_RUNNING;
  ifp->if_flags &= ~IFF_OACTIVE;
  ifp->if_timer = 0;

	nextdma_init(sc->sc_tx_nd);
	nextdma_init(sc->sc_rx_nd);

	nextdma_start(sc->sc_rx_nd, DMACSR_SETREAD);

	if (ifp->if_snd.ifq_head != NULL) {
		mb8795_start(ifp);
	}
}

void
mb8795_stop(sc)
	struct mb8795_softc *sc;
{
  printf("%s: stop not implemented\n", sc->sc_dev.dv_xname);
}


void
mb8795_shutdown(arg)
	void *arg;
{
  struct mb8795_softc *sc = (struct mb8795_softc *)arg;
  mb8795_stop(sc);
}

/****************************************************************/
int
mb8795_ioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	register struct mb8795_softc *sc = ifp->if_softc;
	struct ifaddr *ifa = (struct ifaddr *)data;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	s = splnet();

	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			mb8795_init(sc);
			arp_ifinit(ifp, ifa);
			break;
#endif
#ifdef NS
		case AF_NS:
		    {
			register struct ns_addr *ina = &IA_SNS(ifa)->sns_addr;

			if (ns_nullhost(*ina))
				ina->x_host =
				    *(union ns_host *)LLADDR(ifp->if_sadl);
			else {
				bcopy(ina->x_host.c_host,
				    LLADDR(ifp->if_sadl),
				    sizeof(sc->sc_enaddr));
			}	
			/* Set new address. */
			mb8795_init(sc);
			break;
		    }
#endif
		default:
			mb8795_init(sc);
			break;
		}
		break;

#if defined(CCITT) && defined(LLC)
	case SIOCSIFCONF_X25:
		ifp->if_flags |= IFF_UP;
		ifa->ifa_rtrequest = cons_rtrequest; /* XXX */
		error = x25_llcglue(PRC_IFUP, ifa->ifa_addr);
		if (error == 0)
			mb8795_init(sc);
		break;
#endif /* CCITT && LLC */

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			/*
			 * If interface is marked down and it is running, then
			 * stop it.
			 */
			mb8795_stop(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
		    	   (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			mb8795_init(sc);
		} else {
			/*
			 * Reset the interface to pick up changes in any other
			 * flags that affect hardware registers.
			 */
			/*mb8795_stop(sc);*/
			mb8795_init(sc);
		}
#ifdef XE_DEBUG
		if (ifp->if_flags & IFF_DEBUG)
			sc->sc_debug = 1;
		else
			sc->sc_debug = 0;
#endif
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		error = (cmd == SIOCADDMULTI) ?
		    ether_addmulti(ifr, &sc->sc_ethercom) :
		    ether_delmulti(ifr, &sc->sc_ethercom);

		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			mb8795_reset(sc);
			error = 0;
		}
		break;

#if 0
	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_media, cmd);
		break;
#endif

	default:
		error = EINVAL;
		break;
	}

	splx(s);

#if 0
	DPRINTF(("DEBUG: mb8795_ioctl(0x%lx) returning %d\n",
			cmd,error));
#endif

	return (error);
}

/*
 * Setup output on interface.
 * Get another datagram to send off of the interface queue, and map it to the
 * interface before starting the output.
 * Called only at splnet or interrupt level.
 */
void
mb8795_start(ifp)
     struct ifnet *ifp;
{
  int error;
  struct mb8795_softc *sc = ifp->if_softc;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

  DPRINTF(("%s: mb8795_start()\n",sc->sc_dev.dv_xname));

#if (defined(DIAGNOSTIC))
  {
    u_char txstat;
    txstat = bus_space_read_1(sc->sc_bst,sc->sc_bsh, XE_TXSTAT);
    if (!(txstat & XE_TXSTAT_READY)) {
			/* @@@ I used to panic here, but then it paniced once.
			 * Let's see if I can just reset instead. [ dbj 980706.1900 ]
			 */
      printf("%s: transmitter not ready\n", sc->sc_dev.dv_xname);
			mb8795_reset(sc);
			return;
    }
  }
#endif

#if 0
	return;	/* @@@ Turn off xmit for debugging */
#endif

  bus_space_write_1(sc->sc_bst,sc->sc_bsh, XE_TXSTAT, XE_TXSTAT_CLEAR);

  IF_DEQUEUE(&ifp->if_snd, sc->sc_tx_mb_head);
  if (sc->sc_tx_mb_head == 0) {
    printf("%s: No packet to start\n",
				sc->sc_dev.dv_xname);
    return;
  }

	ifp->if_timer = 5;

/* The following is a next specific hack that should
 * probably be moved out of MI code.
 * This macro assumes it can move forward as needed
 * in the buffer.  Perhaps it should zero the extra buffer.
 */
#define REALIGN_DMABUF(s,l) \
	{ (s) = ((u_char *)(((unsigned)(s)+DMA_BEGINALIGNMENT-1) \
			&~(DMA_BEGINALIGNMENT-1))); \
    (l) = ((u_char *)(((unsigned)((s)+(l))+DMA_ENDALIGNMENT-1) \
				&~(DMA_ENDALIGNMENT-1)))-(s);}

#if 0
  error = bus_dmamap_load_mbuf(sc->sc_tx_dmat,
			sc->sc_tx_dmamap,
			sc->sc_tx_mb_head,
			BUS_DMA_NOWAIT);
#else
	{
		u_char *buf = sc->sc_txbuf;
		int buflen = 0;
		struct mbuf *m = sc->sc_tx_mb_head;
		buflen = m->m_pkthdr.len;

		/* Fix runt packets,  @@@ memory overrun */
		if (buflen < ETHERMIN+sizeof(struct ether_header)) {
			buflen = ETHERMIN+sizeof(struct ether_header);
		}

		buflen += 15;
		REALIGN_DMABUF(buf,buflen);
		if (buflen > 1520) {
			panic("%s: packet too long\n",sc->sc_dev.dv_xname);
		}

		{
			u_char *p = buf;
			for (m=sc->sc_tx_mb_head; m; m = m->m_next) {
				if (m->m_len == 0) continue;
				bcopy(mtod(m, u_char *), p, m->m_len);
				p += m->m_len;
			}
		}

		error = bus_dmamap_load(sc->sc_tx_dmat, sc->sc_tx_dmamap,
				buf,buflen,NULL,BUS_DMA_NOWAIT);
	}
#endif
  if (error) {
    printf("%s: can't load mbuf chain, error = %d\n",
				sc->sc_dev.dv_xname, error);
    m_freem(sc->sc_tx_mb_head);
    sc->sc_tx_mb_head = NULL;
    return;
  }

#ifdef DIAGNOSTIC
	if (sc->sc_tx_loaded != 0) {
			panic("%s: sc->sc_tx_loaded is %d",sc->sc_dev.dv_xname,
					sc->sc_tx_loaded);
	}
#endif

	ifp->if_flags |= IFF_OACTIVE;

  bus_dmamap_sync(sc->sc_tx_dmat, sc->sc_tx_dmamap, 0,
			sc->sc_tx_dmamap->dm_mapsize, BUS_DMASYNC_PREWRITE);

	nextdma_start(sc->sc_tx_nd, DMACSR_SETWRITE);

#if NBPFILTER > 0
  /*
   * Pass packet to bpf if there is a listener.
   */
  if (ifp->if_bpf)
    bpf_mtap(ifp->if_bpf, sc->sc_tx_mb_head);
#endif

}

/****************************************************************/

void 
mb8795_txdma_completed(map, arg)
	bus_dmamap_t map;
	void *arg;
{
	struct mb8795_softc *sc = arg;

  DPRINTF(("%s: mb8795_txdma_completed()\n",sc->sc_dev.dv_xname));

#ifdef DIAGNOSTIC
	if (!sc->sc_tx_loaded) {
		panic("%s: tx completed never loaded ",sc->sc_dev.dv_xname);
	}
	if (map != sc->sc_tx_dmamap) {
		panic("%s: unexpected tx completed map",sc->sc_dev.dv_xname);
	}

#endif
}

void 
mb8795_txdma_shutdown(arg)
	void *arg;
{
	struct mb8795_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

  DPRINTF(("%s: mb8795_txdma_shutdown()\n",sc->sc_dev.dv_xname));

#ifdef DIAGNOSTIC
	if (!sc->sc_tx_loaded) {
		panic("%s: tx shutdown never loaded ",sc->sc_dev.dv_xname);
	}
#endif

	{

		if (sc->sc_tx_loaded) {
			bus_dmamap_sync(sc->sc_tx_dmat, sc->sc_tx_dmamap,
					0, sc->sc_tx_dmamap->dm_mapsize,
					BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(sc->sc_tx_dmat, sc->sc_tx_dmamap);
			m_freem(sc->sc_tx_mb_head);
			sc->sc_tx_mb_head = NULL;

			sc->sc_tx_loaded--;
		}

#ifdef DIAGNOSTIC
		if (sc->sc_tx_loaded != 0) {
			panic("%s: sc->sc_tx_loaded is %d",sc->sc_dev.dv_xname,
					sc->sc_tx_loaded);
		}
#endif

		ifp->if_flags &= ~IFF_OACTIVE;

		ifp->if_timer = 0;

		if (ifp->if_snd.ifq_head != NULL) {
			mb8795_start(ifp);
		}

	}

#if 0
	/* Enable ready interrupt */
	bus_space_write_1(sc->sc_bst,sc->sc_bsh, XE_TXMASK, 
			bus_space_read_1(sc->sc_bst,sc->sc_bsh, XE_TXMASK)
			| XE_TXMASK_READYIE);
#endif
}


void 
mb8795_rxdma_completed(map, arg)
	bus_dmamap_t map;
	void *arg;
{
	struct mb8795_softc *sc = arg;

	sc->sc_rx_completed_idx++;
	sc->sc_rx_completed_idx %= MB8795_NRXBUFS;

  DPRINTF(("%s: mb8795_rxdma_completed(), sc->sc_rx_completed_idx = %d\n",
			sc->sc_dev.dv_xname, sc->sc_rx_completed_idx));

#if (defined(DIAGNOSTIC))
	if (map != sc->sc_rx_dmamap[sc->sc_rx_completed_idx]) {
		panic("%s: Unexpected rx dmamap completed\n",
				sc->sc_dev.dv_xname);
	}
#endif
}

void 
mb8795_rxdma_shutdown(arg)
	void *arg;
{
	struct mb8795_softc *sc = arg;

  DPRINTF(("%s: mb8795_rxdma_shutdown(), restarting.\n",sc->sc_dev.dv_xname));

	nextdma_start(sc->sc_rx_nd, DMACSR_SETREAD);
}


/*
 * load a dmamap with a freshly allocated mbuf
 */
struct mbuf *
mb8795_rxdmamap_load(sc,map)
	struct mb8795_softc *sc;
	bus_dmamap_t map;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *m;
	int error;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			m = NULL;
		} else {
			m->m_len = MCLBYTES;
		}
	}
	if (!m) {
		/* @@@ Handle this gracefully by reusing a scratch buffer
		 * or something.
		 */
		panic("Unable to get memory for incoming ethernet\n");
	}

	/* Align buffer, @@@ next specific.
	 * perhaps should be using M_ALIGN here instead? 
	 * First we give us a little room to align with.
	 */
	{
		u_char *buf = m->m_data;
		int buflen = m->m_len;
		buflen -= DMA_ENDALIGNMENT+DMA_BEGINALIGNMENT;
		REALIGN_DMABUF(buf, buflen);
		m->m_data = buf;
		m->m_len = buflen;
	}

	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = m->m_len;

  error = bus_dmamap_load_mbuf(sc->sc_rx_dmat,
			map, m, BUS_DMA_NOWAIT);

  bus_dmamap_sync(sc->sc_rx_dmat, map, 0,
			map->dm_mapsize, BUS_DMASYNC_PREREAD);
	
  if (error) {
		DPRINTF(("DEBUG: m->m_data = 0x%08x, m->m_len = %d\n",
				m->m_data, m->m_len));
		DPRINTF(("DEBUG: MCLBYTES = %d, map->_dm_size = %d\n",
				MCLBYTES, map->_dm_size));

    panic("%s: can't load rx mbuf chain, error = %d\n",
				sc->sc_dev.dv_xname, error);
    m_freem(m);
		m = NULL;
  }

	return(m);
}

bus_dmamap_t 
mb8795_rxdma_continue(arg)
	void *arg;
{
	struct mb8795_softc *sc = arg;
	bus_dmamap_t map = NULL;

	/*
	 * Currently, starts dumping new packets if the buffers
	 * fill up.  This should probably reclaim unhandled
	 * buffers instead so we drop older packets instead
	 * of newer ones.
	 */
	if (((sc->sc_rx_loaded_idx+1)%MB8795_NRXBUFS) != sc->sc_rx_handled_idx) {
		sc->sc_rx_loaded_idx++;
		sc->sc_rx_loaded_idx %= MB8795_NRXBUFS;
		map = sc->sc_rx_dmamap[sc->sc_rx_loaded_idx];

		DPRINTF(("%s: mb8795_rxdma_continue() sc->sc_rx_loaded_idx = %d\nn",
				sc->sc_dev.dv_xname,sc->sc_rx_loaded_idx));
	}
#if (defined(DIAGNOSTIC))
	else {
		panic("%s: out of receive DMA buffers\n",sc->sc_dev.dv_xname);
	}
#endif

	return(map);
}

bus_dmamap_t 
mb8795_txdma_continue(arg)
	void *arg;
{
	struct mb8795_softc *sc = arg;
	bus_dmamap_t map;

  DPRINTF(("%s: mb8795_txdma_continue()\n",sc->sc_dev.dv_xname));

	if (sc->sc_tx_loaded) {
		map = NULL;
	} else {
		map = sc->sc_tx_dmamap;
		sc->sc_tx_loaded++;
	}

#ifdef DIAGNOSTIC
	if (sc->sc_tx_loaded != 1) {
		panic("%s: sc->sc_tx_loaded is %d",sc->sc_dev.dv_xname,
				sc->sc_tx_loaded);
	}
#endif

	return(map);
}

/****************************************************************/

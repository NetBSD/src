/*	$NetBSD: if_hvn.c,v 1.2.2.5 2019/11/24 08:11:06 martin Exp $	*/
/*	$OpenBSD: if_hvn.c,v 1.39 2018/03/11 14:31:34 mikeb Exp $	*/

/*-
 * Copyright (c) 2009-2012,2016 Microsoft Corp.
 * Copyright (c) 2010-2012 Citrix Inc.
 * Copyright (c) 2012 NetApp Inc.
 * Copyright (c) 2016 Mike Belopuhov <mike@esdenera.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
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

/*
 * The OpenBSD port was done under funding by Esdenera Networks GmbH.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_hvn.c,v 1.2.2.5 2019/11/24 08:11:06 martin Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_inet6.h"
#include "opt_net_mpsafe.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/atomic.h>
#include <sys/bus.h>
#include <sys/intr.h>
#include <sys/kmem.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <net/bpf.h>

#include <dev/ic/ndisreg.h>
#include <dev/ic/rndisreg.h>

#include <dev/hyperv/vmbusvar.h>
#include <dev/hyperv/if_hvnreg.h>

#ifndef EVL_PRIO_BITS
#define EVL_PRIO_BITS	13
#endif

#ifndef ETHER_ALIGN
#define ETHER_ALIGN	2
#endif

#define HVN_NVS_MSGSIZE			32
#define HVN_NVS_BUFSIZE			PAGE_SIZE

/*
 * RNDIS control interface
 */
#define HVN_RNDIS_CTLREQS		4
#define HVN_RNDIS_BUFSIZE		512

struct rndis_cmd {
	uint32_t			rc_id;
	struct hvn_nvs_rndis		rc_msg;
	void				*rc_req;
	bus_dmamap_t			rc_dmap;
	bus_dma_segment_t		rc_segs;
	int				rc_nsegs;
	uint64_t			rc_gpa;
	struct rndis_packet_msg		rc_cmp;
	uint32_t			rc_cmplen;
	uint8_t				rc_cmpbuf[HVN_RNDIS_BUFSIZE];
	int				rc_done;
	TAILQ_ENTRY(rndis_cmd)		rc_entry;
};
TAILQ_HEAD(rndis_queue, rndis_cmd);

#define HVN_MAXMTU			(9 * 1024)

#define HVN_RNDIS_XFER_SIZE		2048

/*
 * Tx ring
 */
#define HVN_TX_DESC			256
#define HVN_TX_FRAGS			15		/* 31 is the max */
#define HVN_TX_FRAG_SIZE		PAGE_SIZE
#define HVN_TX_PKT_SIZE			16384

#define HVN_RNDIS_PKT_LEN					\
	(sizeof(struct rndis_packet_msg) +			\
	 sizeof(struct rndis_pktinfo) + NDIS_VLAN_INFO_SIZE +	\
	 sizeof(struct rndis_pktinfo) + NDIS_TXCSUM_INFO_SIZE)

struct hvn_tx_desc {
	uint32_t			txd_id;
	int				txd_ready;
	struct vmbus_gpa		txd_sgl[HVN_TX_FRAGS + 1];
	int				txd_nsge;
	struct mbuf			*txd_buf;
	bus_dmamap_t			txd_dmap;
	struct vmbus_gpa		txd_gpa;
	struct rndis_packet_msg		*txd_req;
};

struct hvn_softc {
	device_t			sc_dev;

	struct vmbus_softc		*sc_vmbus;
	struct vmbus_channel		*sc_chan;
	bus_dma_tag_t			sc_dmat;

	struct ethercom			sc_ec;
	struct ifmedia			sc_media;
	struct if_percpuq		*sc_ipq;
	int				sc_link_state;
	int				sc_promisc;

	uint32_t			sc_flags;
#define	HVN_SCF_ATTACHED	__BIT(0)

	/* NVS protocol */
	int				sc_proto;
	uint32_t			sc_nvstid;
	uint8_t				sc_nvsrsp[HVN_NVS_MSGSIZE];
	uint8_t				*sc_nvsbuf;
	int				sc_nvsdone;

	/* RNDIS protocol */
	int				sc_ndisver;
	uint32_t			sc_rndisrid;
	struct rndis_queue		sc_cntl_sq; /* submission queue */
	kmutex_t			sc_cntl_sqlck;
	struct rndis_queue		sc_cntl_cq; /* completion queue */
	kmutex_t			sc_cntl_cqlck;
	struct rndis_queue		sc_cntl_fq; /* free queue */
	kmutex_t			sc_cntl_fqlck;
	struct rndis_cmd		sc_cntl_msgs[HVN_RNDIS_CTLREQS];
	struct hvn_nvs_rndis		sc_data_msg;

	/* Rx ring */
	uint8_t				*sc_rx_ring;
	int				sc_rx_size;
	uint32_t			sc_rx_hndl;
	struct hyperv_dma		sc_rx_dma;

	/* Tx ring */
	uint32_t			sc_tx_next;
	uint32_t			sc_tx_avail;
	struct hvn_tx_desc		sc_tx_desc[HVN_TX_DESC];
	bus_dmamap_t			sc_tx_rmap;
	uint8_t				*sc_tx_msgs;
	bus_dma_segment_t		sc_tx_mseg;
};

#define SC2IFP(_sc_)	(&(_sc_)->sc_ec.ec_if)
#define IFP2SC(_ifp_)	((_ifp_)->if_softc)


static int	hvn_match(device_t, cfdata_t, void *);
static void	hvn_attach(device_t, device_t, void *);
static int	hvn_detach(device_t, int);

CFATTACH_DECL_NEW(hvn, sizeof(struct hvn_softc),
    hvn_match, hvn_attach, hvn_detach, NULL);

static int	hvn_ioctl(struct ifnet *, u_long, void *);
static int	hvn_media_change(struct ifnet *);
static void	hvn_media_status(struct ifnet *, struct ifmediareq *);
static int	hvn_iff(struct hvn_softc *);
static int	hvn_init(struct ifnet *);
static void	hvn_stop(struct ifnet *, int);
static void	hvn_start(struct ifnet *);
static int	hvn_encap(struct hvn_softc *, struct mbuf *,
		    struct hvn_tx_desc **);
static void	hvn_decap(struct hvn_softc *, struct hvn_tx_desc *);
static void	hvn_txeof(struct hvn_softc *, uint64_t);
static int	hvn_rx_ring_create(struct hvn_softc *);
static int	hvn_rx_ring_destroy(struct hvn_softc *);
static int	hvn_tx_ring_create(struct hvn_softc *);
static void	hvn_tx_ring_destroy(struct hvn_softc *);
static int	hvn_set_capabilities(struct hvn_softc *);
static int	hvn_get_lladdr(struct hvn_softc *, uint8_t *);
static void	hvn_get_link_status(struct hvn_softc *);

/* NSVP */
static int	hvn_nvs_attach(struct hvn_softc *);
static void	hvn_nvs_intr(void *);
static int	hvn_nvs_cmd(struct hvn_softc *, void *, size_t, uint64_t, int);
static int	hvn_nvs_ack(struct hvn_softc *, uint64_t);
static void	hvn_nvs_detach(struct hvn_softc *);

/* RNDIS */
static int	hvn_rndis_attach(struct hvn_softc *);
static int	hvn_rndis_cmd(struct hvn_softc *, struct rndis_cmd *, int);
static void	hvn_rndis_input(struct hvn_softc *, uint64_t, void *);
static void	hvn_rxeof(struct hvn_softc *, uint8_t *, uint32_t);
static void	hvn_rndis_complete(struct hvn_softc *, uint8_t *, uint32_t);
static int	hvn_rndis_output(struct hvn_softc *, struct hvn_tx_desc *);
static void	hvn_rndis_status(struct hvn_softc *, uint8_t *, uint32_t);
static int	hvn_rndis_query(struct hvn_softc *, uint32_t, void *, size_t *);
static int	hvn_rndis_set(struct hvn_softc *, uint32_t, void *, size_t);
static int	hvn_rndis_open(struct hvn_softc *);
static int	hvn_rndis_close(struct hvn_softc *);
static void	hvn_rndis_detach(struct hvn_softc *);

static int
hvn_match(device_t parent, cfdata_t match, void *aux)
{
	struct vmbus_attach_args *aa = aux;

	if (memcmp(aa->aa_type, &hyperv_guid_network, sizeof(*aa->aa_type)))
		return 0;
	return 1;
}

static void
hvn_attach(device_t parent, device_t self, void *aux)
{
	struct hvn_softc *sc = device_private(self);
	struct vmbus_attach_args *aa = aux;
	struct ifnet *ifp = SC2IFP(sc);
	uint8_t enaddr[ETHER_ADDR_LEN];
	int error;

	sc->sc_dev = self;
	sc->sc_vmbus = (struct vmbus_softc *)device_private(parent);
	sc->sc_chan = aa->aa_chan;
	sc->sc_dmat = sc->sc_vmbus->sc_dmat;

	aprint_naive("\n");
	aprint_normal(": Hyper-V NetVSC\n");

	strlcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);

	if (hvn_nvs_attach(sc)) {
		aprint_error_dev(self, "failed to init NVSP\n");
		return;
	}

	if (hvn_rx_ring_create(sc)) {
		aprint_error_dev(self, "failed to create Rx ring\n");
		goto fail1;
	}

	if (hvn_tx_ring_create(sc)) {
		aprint_error_dev(self, "failed to create Tx ring\n");
		goto fail1;
	}

	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = hvn_ioctl;
	ifp->if_start = hvn_start;
	ifp->if_init = hvn_init;
	ifp->if_stop = hvn_stop;
	ifp->if_capabilities = IFCAP_CSUM_IPv4_Tx | IFCAP_CSUM_IPv4_Rx;
	ifp->if_capabilities |= IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_TCPv4_Rx;
	ifp->if_capabilities |= IFCAP_CSUM_TCPv6_Tx | IFCAP_CSUM_TCPv6_Rx;
	if (sc->sc_ndisver > NDIS_VERSION_6_30) {
		ifp->if_capabilities |= IFCAP_CSUM_UDPv4_Tx;
		ifp->if_capabilities |= IFCAP_CSUM_UDPv4_Rx;
		ifp->if_capabilities |= IFCAP_CSUM_UDPv6_Tx;
		ifp->if_capabilities |= IFCAP_CSUM_UDPv6_Rx;
	}
	if (sc->sc_proto >= HVN_NVS_PROTO_VERSION_2) {
		sc->sc_ec.ec_capabilities |= ETHERCAP_VLAN_HWTAGGING;
		sc->sc_ec.ec_capabilities |= ETHERCAP_VLAN_MTU;
	}

	IFQ_SET_MAXLEN(&ifp->if_snd, HVN_TX_DESC - 1);
	IFQ_SET_READY(&ifp->if_snd);

	ifmedia_init(&sc->sc_media, IFM_IMASK, hvn_media_change,
	    hvn_media_status);
	ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_MANUAL, 0, NULL);
	ifmedia_set(&sc->sc_media, IFM_ETHER | IFM_MANUAL);

	error = if_initialize(ifp);
	if (error) {
		aprint_error_dev(self, "if_initialize failed(%d)\n", error);
		goto fail2;
	}
	sc->sc_ipq = if_percpuq_create(ifp);
	if_deferred_start_init(ifp, NULL);

	if (hvn_rndis_attach(sc)) {
		aprint_error_dev(self, "failed to init RNDIS\n");
		goto fail1;
	}

	aprint_normal_dev(self, "NVS %d.%d NDIS %d.%d\n",
	    sc->sc_proto >> 16, sc->sc_proto & 0xffff,
	    sc->sc_ndisver >> 16 , sc->sc_ndisver & 0xffff);

	if (hvn_set_capabilities(sc)) {
		aprint_error_dev(self, "failed to setup offloading\n");
		goto fail2;
	}

	if (hvn_get_lladdr(sc, enaddr)) {
		aprint_error_dev(self,
		    "failed to obtain an ethernet address\n");
		goto fail2;
	}
	aprint_normal_dev(self, "Ethernet address %s\n", ether_sprintf(enaddr));

	ether_ifattach(ifp, enaddr);
	if_register(ifp);

	if (pmf_device_register(self, NULL, NULL))
		pmf_class_network_register(self, ifp);
	else
		aprint_error_dev(self, "couldn't establish power handler\n");

	SET(sc->sc_flags, HVN_SCF_ATTACHED);
	return;

fail2:	hvn_rndis_detach(sc);
fail1:	hvn_rx_ring_destroy(sc);
	hvn_tx_ring_destroy(sc);
	hvn_nvs_detach(sc);
}

static int
hvn_detach(device_t self, int flags)
{
	struct hvn_softc *sc = device_private(self);
	struct ifnet *ifp = SC2IFP(sc);

	if (!ISSET(sc->sc_flags, HVN_SCF_ATTACHED))
		return 0;

	hvn_stop(ifp, 1);

	pmf_device_deregister(self);

	ether_ifdetach(ifp);
	if_detach(ifp);
	if_percpuq_destroy(sc->sc_ipq);

	hvn_rndis_detach(sc);
	hvn_rx_ring_destroy(sc);
	hvn_tx_ring_destroy(sc);
	hvn_nvs_detach(sc);

	return 0;
}

static int
hvn_ioctl(struct ifnet *ifp, u_long command, void * data)
{
	struct hvn_softc *sc = IFP2SC(ifp);
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	s = splnet();

	switch (command) {
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			if (ifp->if_flags & IFF_RUNNING)
				error = ENETRESET;
			else {
				error = hvn_init(ifp);
				if (error)
					ifp->if_flags &= ~IFF_UP;
			}
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				hvn_stop(ifp, 1);
		}
		break;
	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_media, command);
		break;
	default:
		error = ether_ioctl(ifp, command, data);
		break;
	}

	if (error == ENETRESET) {
		if (ifp->if_flags & IFF_RUNNING)
			hvn_iff(sc);
		error = 0;
	}

	splx(s);

	return error;
}

static int
hvn_media_change(struct ifnet *ifp)
{

	return 0;
}

static void
hvn_media_status(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct hvn_softc *sc = IFP2SC(ifp);
	int link_state;

	link_state = sc->sc_link_state;
	hvn_get_link_status(sc);
	if (link_state != sc->sc_link_state)
		if_link_state_change(ifp, sc->sc_link_state);

	ifmr->ifm_status = IFM_AVALID;
	ifmr->ifm_active = IFM_ETHER | IFM_MANUAL;
	if (sc->sc_link_state == LINK_STATE_UP)
		ifmr->ifm_status |= IFM_ACTIVE;
}

static int
hvn_iff(struct hvn_softc *sc)
{

	/* XXX */
	sc->sc_promisc = 0;

	return 0;
}

static int
hvn_init(struct ifnet *ifp)
{
	struct hvn_softc *sc = IFP2SC(ifp);
	int error;

	hvn_stop(ifp, 0);

	error = hvn_iff(sc);
	if (error)
		return error;

	error = hvn_rndis_open(sc);
	if (error == 0) {
		ifp->if_flags |= IFF_RUNNING;
		ifp->if_flags &= ~IFF_OACTIVE;
	}
	return error;
}

static void
hvn_stop(struct ifnet *ifp, int disable)
{
	struct hvn_softc *sc = IFP2SC(ifp);

	hvn_rndis_close(sc);

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
}

static void
hvn_start(struct ifnet *ifp)
{
	struct hvn_softc *sc = IFP2SC(ifp);
	struct hvn_tx_desc *txd;
	struct mbuf *m;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	for (;;) {
		if (!sc->sc_tx_avail) {
			/* transient */
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;

		if (hvn_encap(sc, m, &txd)) {
			/* the chain is too large */
			ifp->if_oerrors++;
			m_freem(m);
			continue;
		}

		bpf_mtap(ifp, m);

		if (hvn_rndis_output(sc, txd)) {
			hvn_decap(sc, txd);
			ifp->if_oerrors++;
			m_freem(m);
			continue;
		}

		sc->sc_tx_next++;
	}
}

static inline char *
hvn_rndis_pktinfo_append(struct rndis_packet_msg *pkt, size_t pktsize,
    size_t datalen, uint32_t type)
{
	struct rndis_pktinfo *pi;
	size_t pi_size = sizeof(*pi) + datalen;
	char *cp;

	KASSERT(pkt->rm_pktinfooffset + pkt->rm_pktinfolen + pi_size <=
	    pktsize);

	cp = (char *)pkt + pkt->rm_pktinfooffset + pkt->rm_pktinfolen;
	pi = (struct rndis_pktinfo *)cp;
	pi->rm_size = pi_size;
	pi->rm_type = type;
	pi->rm_pktinfooffset = sizeof(*pi);
	pkt->rm_pktinfolen += pi_size;
	pkt->rm_dataoffset += pi_size;
	pkt->rm_len += pi_size;

	return (char *)pi->rm_data;
}

static int
hvn_encap(struct hvn_softc *sc, struct mbuf *m, struct hvn_tx_desc **txd0)
{
	struct hvn_tx_desc *txd;
	struct rndis_packet_msg *pkt;
	bus_dma_segment_t *seg;
	size_t pktlen;
	int i, rv;

	do {
		txd = &sc->sc_tx_desc[sc->sc_tx_next % HVN_TX_DESC];
		sc->sc_tx_next++;
	} while (!txd->txd_ready);
	txd->txd_ready = 0;

	pkt = txd->txd_req;
	memset(pkt, 0, HVN_RNDIS_PKT_LEN);
	pkt->rm_type = REMOTE_NDIS_PACKET_MSG;
	pkt->rm_len = sizeof(*pkt) + m->m_pkthdr.len;
	pkt->rm_dataoffset = RNDIS_DATA_OFFSET;
	pkt->rm_datalen = m->m_pkthdr.len;
	pkt->rm_pktinfooffset = sizeof(*pkt); /* adjusted below */
	pkt->rm_pktinfolen = 0;

	rv = bus_dmamap_load_mbuf(sc->sc_dmat, txd->txd_dmap, m, BUS_DMA_READ |
	    BUS_DMA_NOWAIT);
	switch (rv) {
	case 0:
		break;
	case EFBIG:
		if (m_defrag(m, M_NOWAIT) != NULL &&
		    bus_dmamap_load_mbuf(sc->sc_dmat, txd->txd_dmap, m,
		      BUS_DMA_READ | BUS_DMA_NOWAIT) == 0)
			break;
		/* FALLTHROUGH */
	default:
		DPRINTF("%s: failed to load mbuf\n", device_xname(sc->sc_dev));
		return -1;
	}
	txd->txd_buf = m;

	if (m->m_flags & M_VLANTAG) {
		uint32_t vlan;
		char *cp;

		vlan = NDIS_VLAN_INFO_MAKE(
		    EVL_VLANOFTAG(m->m_pkthdr.ether_vtag),
		    EVL_PRIOFTAG(m->m_pkthdr.ether_vtag), 0);
		cp = hvn_rndis_pktinfo_append(pkt, HVN_RNDIS_PKT_LEN,
		    NDIS_VLAN_INFO_SIZE, NDIS_PKTINFO_TYPE_VLAN);
		memcpy(cp, &vlan, NDIS_VLAN_INFO_SIZE);
	}

	if (m->m_pkthdr.csum_flags & (M_CSUM_IPv4 | M_CSUM_UDPv4 |
	    M_CSUM_TCPv4)) {
		uint32_t csum = NDIS_TXCSUM_INFO_IPV4;
		char *cp;

		if (m->m_pkthdr.csum_flags & M_CSUM_IPv4)
			csum |= NDIS_TXCSUM_INFO_IPCS;
		if (m->m_pkthdr.csum_flags & M_CSUM_TCPv4)
			csum |= NDIS_TXCSUM_INFO_TCPCS;
		if (m->m_pkthdr.csum_flags & M_CSUM_UDPv4)
			csum |= NDIS_TXCSUM_INFO_UDPCS;
		cp = hvn_rndis_pktinfo_append(pkt, HVN_RNDIS_PKT_LEN,
		    NDIS_TXCSUM_INFO_SIZE, NDIS_PKTINFO_TYPE_CSUM);
		memcpy(cp, &csum, NDIS_TXCSUM_INFO_SIZE);
	}

	pktlen = pkt->rm_pktinfooffset + pkt->rm_pktinfolen;
	pkt->rm_pktinfooffset -= RNDIS_HEADER_OFFSET;

	/* Attach an RNDIS message to the first slot */
	txd->txd_sgl[0].gpa_page = txd->txd_gpa.gpa_page;
	txd->txd_sgl[0].gpa_ofs = txd->txd_gpa.gpa_ofs;
	txd->txd_sgl[0].gpa_len = pktlen;
	txd->txd_nsge = txd->txd_dmap->dm_nsegs + 1;

	for (i = 0; i < txd->txd_dmap->dm_nsegs; i++) {
		seg = &txd->txd_dmap->dm_segs[i];
		txd->txd_sgl[1 + i].gpa_page = atop(seg->ds_addr);
		txd->txd_sgl[1 + i].gpa_ofs = seg->ds_addr & PAGE_MASK;
		txd->txd_sgl[1 + i].gpa_len = seg->ds_len;
	}

	*txd0 = txd;

	atomic_dec_uint(&sc->sc_tx_avail);

	return 0;
}

static void
hvn_decap(struct hvn_softc *sc, struct hvn_tx_desc *txd)
{
	struct ifnet *ifp = SC2IFP(sc);

	bus_dmamap_sync(sc->sc_dmat, txd->txd_dmap, 0, 0,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	bus_dmamap_unload(sc->sc_dmat, txd->txd_dmap);
	txd->txd_buf = NULL;
	txd->txd_nsge = 0;
	txd->txd_ready = 1;
	atomic_inc_uint(&sc->sc_tx_avail);
	ifp->if_flags &= ~IFF_OACTIVE;
}

static void
hvn_txeof(struct hvn_softc *sc, uint64_t tid)
{
	struct ifnet *ifp = SC2IFP(sc);
	struct hvn_tx_desc *txd;
	struct mbuf *m;
	uint32_t id = tid >> 32;

	if ((tid & 0xffffffffU) != 0)
		return;

	id -= HVN_NVS_CHIM_SIG;
	if (id >= HVN_TX_DESC) {
		device_printf(sc->sc_dev, "tx packet index too large: %u", id);
		return;
	}

	txd = &sc->sc_tx_desc[id];

	if ((m = txd->txd_buf) == NULL) {
		device_printf(sc->sc_dev, "no mbuf @%u\n", id);
		return;
	}
	txd->txd_buf = NULL;

	bus_dmamap_sync(sc->sc_dmat, txd->txd_dmap, 0, 0,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	bus_dmamap_unload(sc->sc_dmat, txd->txd_dmap);
	m_freem(m);
	ifp->if_opackets++;

	txd->txd_ready = 1;

	atomic_inc_uint(&sc->sc_tx_avail);
	ifp->if_flags &= ~IFF_OACTIVE;
}

static int
hvn_rx_ring_create(struct hvn_softc *sc)
{
	struct hvn_nvs_rxbuf_conn cmd;
	struct hvn_nvs_rxbuf_conn_resp *rsp;
	uint64_t tid;

	if (sc->sc_proto <= HVN_NVS_PROTO_VERSION_2)
		sc->sc_rx_size = 15 * 1024 * 1024;	/* 15MB */
	else
		sc->sc_rx_size = 16 * 1024 * 1024; 	/* 16MB */
	sc->sc_rx_ring = hyperv_dma_alloc(sc->sc_dmat, &sc->sc_rx_dma,
	    sc->sc_rx_size, PAGE_SIZE, PAGE_SIZE, sc->sc_rx_size / PAGE_SIZE);
	if (sc->sc_rx_ring == NULL) {
		DPRINTF("%s: failed to allocate Rx ring buffer\n",
		    device_xname(sc->sc_dev));
		return -1;
	}
	if (vmbus_handle_alloc(sc->sc_chan, &sc->sc_rx_dma, sc->sc_rx_size,
	    &sc->sc_rx_hndl)) {
		DPRINTF("%s: failed to obtain a PA handle\n",
		    device_xname(sc->sc_dev));
		goto errout;
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.nvs_type = HVN_NVS_TYPE_RXBUF_CONN;
	cmd.nvs_gpadl = sc->sc_rx_hndl;
	cmd.nvs_sig = HVN_NVS_RXBUF_SIG;

	tid = atomic_inc_uint_nv(&sc->sc_nvstid);
	if (hvn_nvs_cmd(sc, &cmd, sizeof(cmd), tid, 100))
		goto errout;

	rsp = (struct hvn_nvs_rxbuf_conn_resp *)&sc->sc_nvsrsp;
	if (rsp->nvs_status != HVN_NVS_STATUS_OK) {
		DPRINTF("%s: failed to set up the Rx ring\n",
		    device_xname(sc->sc_dev));
		goto errout;
	}
	if (rsp->nvs_nsect > 1) {
		DPRINTF("%s: invalid number of Rx ring sections: %u\n",
		    device_xname(sc->sc_dev), rsp->nvs_nsect);
		hvn_rx_ring_destroy(sc);
		return -1;
	}
	return 0;

 errout:
	if (sc->sc_rx_hndl) {
		vmbus_handle_free(sc->sc_chan, sc->sc_rx_hndl);
		sc->sc_rx_hndl = 0;
	}
	if (sc->sc_rx_ring) {
		kmem_free(sc->sc_rx_ring, sc->sc_rx_size);
		sc->sc_rx_ring = NULL;
	}
	return -1;
}

static int
hvn_rx_ring_destroy(struct hvn_softc *sc)
{
	struct hvn_nvs_rxbuf_disconn cmd;
	uint64_t tid;

	if (sc->sc_rx_ring == NULL)
		return 0;

	memset(&cmd, 0, sizeof(cmd));
	cmd.nvs_type = HVN_NVS_TYPE_RXBUF_DISCONN;
	cmd.nvs_sig = HVN_NVS_RXBUF_SIG;

	tid = atomic_inc_uint_nv(&sc->sc_nvstid);
	if (hvn_nvs_cmd(sc, &cmd, sizeof(cmd), tid, 0))
		return -1;

	delay(100);

	vmbus_handle_free(sc->sc_chan, sc->sc_rx_hndl);

	sc->sc_rx_hndl = 0;

	kmem_free(sc->sc_rx_ring, sc->sc_rx_size);
	sc->sc_rx_ring = NULL;

	return 0;
}

static int
hvn_tx_ring_create(struct hvn_softc *sc)
{
	const int dmaflags = cold ? BUS_DMA_NOWAIT : BUS_DMA_WAITOK;
	struct hvn_tx_desc *txd;
	bus_dma_segment_t *seg;
	size_t msgsize;
	int i, rsegs;
	paddr_t pa;

	msgsize = roundup(HVN_RNDIS_PKT_LEN, 128);

	/* Allocate memory to store RNDIS messages */
	if (bus_dmamem_alloc(sc->sc_dmat, msgsize * HVN_TX_DESC, PAGE_SIZE, 0,
	    &sc->sc_tx_mseg, 1, &rsegs, dmaflags)) {
		DPRINTF("%s: failed to allocate memory for RDNIS messages\n",
		    device_xname(sc->sc_dev));
		goto errout;
	}
	if (bus_dmamem_map(sc->sc_dmat, &sc->sc_tx_mseg, 1, msgsize *
	    HVN_TX_DESC, (void **)&sc->sc_tx_msgs, dmaflags)) {
		DPRINTF("%s: failed to establish mapping for RDNIS messages\n",
		    device_xname(sc->sc_dev));
		goto errout;
	}
	memset(sc->sc_tx_msgs, 0, msgsize * HVN_TX_DESC);
	if (bus_dmamap_create(sc->sc_dmat, msgsize * HVN_TX_DESC, 1,
	    msgsize * HVN_TX_DESC, 0, dmaflags, &sc->sc_tx_rmap)) {
		DPRINTF("%s: failed to create map for RDNIS messages\n",
		    device_xname(sc->sc_dev));
		goto errout;
	}
	if (bus_dmamap_load(sc->sc_dmat, sc->sc_tx_rmap, sc->sc_tx_msgs,
	    msgsize * HVN_TX_DESC, NULL, dmaflags)) {
		DPRINTF("%s: failed to create map for RDNIS messages\n",
		    device_xname(sc->sc_dev));
		goto errout;
	}

	for (i = 0; i < HVN_TX_DESC; i++) {
		txd = &sc->sc_tx_desc[i];
		if (bus_dmamap_create(sc->sc_dmat, HVN_TX_PKT_SIZE,
		    HVN_TX_FRAGS, HVN_TX_FRAG_SIZE, PAGE_SIZE, dmaflags,
		    &txd->txd_dmap)) {
			DPRINTF("%s: failed to create map for TX descriptors\n",
			    device_xname(sc->sc_dev));
			goto errout;
		}
		seg = &sc->sc_tx_rmap->dm_segs[0];
		pa = seg->ds_addr + (msgsize * i);
		txd->txd_gpa.gpa_page = atop(pa);
		txd->txd_gpa.gpa_ofs = pa & PAGE_MASK;
		txd->txd_gpa.gpa_len = msgsize;
		txd->txd_req = (void *)(sc->sc_tx_msgs + (msgsize * i));
		txd->txd_id = i + HVN_NVS_CHIM_SIG;
		txd->txd_ready = 1;
	}
	sc->sc_tx_avail = HVN_TX_DESC;

	return 0;

 errout:
	hvn_tx_ring_destroy(sc);
	return -1;
}

static void
hvn_tx_ring_destroy(struct hvn_softc *sc)
{
	struct hvn_tx_desc *txd;
	int i;

	for (i = 0; i < HVN_TX_DESC; i++) {
		txd = &sc->sc_tx_desc[i];
		if (txd->txd_dmap == NULL)
			continue;
		bus_dmamap_sync(sc->sc_dmat, txd->txd_dmap, 0, 0,
		    BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, txd->txd_dmap);
		bus_dmamap_destroy(sc->sc_dmat, txd->txd_dmap);
		txd->txd_dmap = NULL;
		if (txd->txd_buf == NULL)
			continue;
		m_free(txd->txd_buf);
		txd->txd_buf = NULL;
	}
	if (sc->sc_tx_rmap) {
		bus_dmamap_sync(sc->sc_dmat, sc->sc_tx_rmap, 0, 0,
		    BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, sc->sc_tx_rmap);
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_tx_rmap);
	}
	if (sc->sc_tx_msgs) {
		size_t msgsize = roundup(HVN_RNDIS_PKT_LEN, 128);

		bus_dmamem_unmap(sc->sc_dmat, sc->sc_tx_msgs,
		    msgsize * HVN_TX_DESC);
		bus_dmamem_free(sc->sc_dmat, &sc->sc_tx_mseg, 1);
	}
	sc->sc_tx_rmap = NULL;
	sc->sc_tx_msgs = NULL;
}

static int
hvn_get_lladdr(struct hvn_softc *sc, uint8_t *enaddr)
{
	size_t addrlen = ETHER_ADDR_LEN;
	int rv;

	rv = hvn_rndis_query(sc, OID_802_3_PERMANENT_ADDRESS, enaddr, &addrlen);
	if (rv == 0 && addrlen != ETHER_ADDR_LEN)
		rv = -1;
	return rv;
}

static void
hvn_get_link_status(struct hvn_softc *sc)
{
	uint32_t state;
	size_t len = sizeof(state);

	if (hvn_rndis_query(sc, OID_GEN_MEDIA_CONNECT_STATUS,
	    &state, &len) == 0)
		sc->sc_link_state = (state == NDIS_MEDIA_STATE_CONNECTED) ?
		    LINK_STATE_UP : LINK_STATE_DOWN;
}

static int
hvn_nvs_attach(struct hvn_softc *sc)
{
	static const uint32_t protos[] = {
		HVN_NVS_PROTO_VERSION_5,
		HVN_NVS_PROTO_VERSION_4,
		HVN_NVS_PROTO_VERSION_2,
		HVN_NVS_PROTO_VERSION_1
	};
	const int kmemflags = cold ? KM_NOSLEEP : KM_SLEEP;
	struct hvn_nvs_init cmd;
	struct hvn_nvs_init_resp *rsp;
	struct hvn_nvs_ndis_init ncmd;
	struct hvn_nvs_ndis_conf ccmd;
	uint32_t ndisver, ringsize;
	uint64_t tid;
	int i;

	sc->sc_nvsbuf = kmem_zalloc(HVN_NVS_BUFSIZE, kmemflags);
	if (sc->sc_nvsbuf == NULL) {
		DPRINTF("%s: failed to allocate channel data buffer\n",
		    device_xname(sc->sc_dev));
		return -1;
	}

	/* We need to be able to fit all RNDIS control and data messages */
	ringsize = HVN_RNDIS_CTLREQS *
	    (sizeof(struct hvn_nvs_rndis) + sizeof(struct vmbus_gpa)) +
	    HVN_TX_DESC * (sizeof(struct hvn_nvs_rndis) +
	    (HVN_TX_FRAGS + 1) * sizeof(struct vmbus_gpa));

	sc->sc_chan->ch_flags &= ~CHF_BATCHED;

	/* Associate our interrupt handler with the channel */
	if (vmbus_channel_open(sc->sc_chan, ringsize, NULL, 0,
	    hvn_nvs_intr, sc)) {
		DPRINTF("%s: failed to open channel\n",
		    device_xname(sc->sc_dev));
		kmem_free(sc->sc_nvsbuf, HVN_NVS_BUFSIZE);
		return -1;
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.nvs_type = HVN_NVS_TYPE_INIT;
	for (i = 0; i < __arraycount(protos); i++) {
		cmd.nvs_ver_min = cmd.nvs_ver_max = protos[i];
		tid = atomic_inc_uint_nv(&sc->sc_nvstid);
		if (hvn_nvs_cmd(sc, &cmd, sizeof(cmd), tid, 100))
			return -1;

		rsp = (struct hvn_nvs_init_resp *)&sc->sc_nvsrsp;
		if (rsp->nvs_status == HVN_NVS_STATUS_OK) {
			sc->sc_proto = protos[i];
			break;
		}
	}
	if (i == __arraycount(protos)) {
		DPRINTF("%s: failed to negotiate NVSP version\n",
		    device_xname(sc->sc_dev));
		return -1;
	}

	if (sc->sc_proto >= HVN_NVS_PROTO_VERSION_2) {
		memset(&ccmd, 0, sizeof(ccmd));
		ccmd.nvs_type = HVN_NVS_TYPE_NDIS_CONF;
		ccmd.nvs_mtu = HVN_MAXMTU;
		ccmd.nvs_caps = HVN_NVS_NDIS_CONF_VLAN;

		tid = atomic_inc_uint_nv(&sc->sc_nvstid);
		if (hvn_nvs_cmd(sc, &ccmd, sizeof(ccmd), tid, 100))
			return -1;
	}

	memset(&ncmd, 0, sizeof(ncmd));
	ncmd.nvs_type = HVN_NVS_TYPE_NDIS_INIT;
	if (sc->sc_proto <= HVN_NVS_PROTO_VERSION_4)
		ndisver = NDIS_VERSION_6_1;
	else
		ndisver = NDIS_VERSION_6_30;
	ncmd.nvs_ndis_major = (ndisver & 0xffff0000) >> 16;
	ncmd.nvs_ndis_minor = ndisver & 0x0000ffff;

	tid = atomic_inc_uint_nv(&sc->sc_nvstid);
	if (hvn_nvs_cmd(sc, &ncmd, sizeof(ncmd), tid, 100))
		return -1;

	sc->sc_ndisver = ndisver;

	return 0;
}

static void
hvn_nvs_intr(void *arg)
{
	struct hvn_softc *sc = arg;
	struct ifnet *ifp = SC2IFP(sc);
	struct vmbus_chanpkt_hdr *cph;
	const struct hvn_nvs_hdr *nvs;
	uint64_t rid;
	uint32_t rlen;
	int rv;
	bool dotx = false;

	for (;;) {
		rv = vmbus_channel_recv(sc->sc_chan, sc->sc_nvsbuf,
		    HVN_NVS_BUFSIZE, &rlen, &rid, 1);
		if (rv != 0 || rlen == 0) {
			if (rv != EAGAIN)
				device_printf(sc->sc_dev,
				    "failed to receive an NVSP packet\n");
			break;
		}
		cph = (struct vmbus_chanpkt_hdr *)sc->sc_nvsbuf;
		nvs = (const struct hvn_nvs_hdr *)VMBUS_CHANPKT_CONST_DATA(cph);

		if (cph->cph_type == VMBUS_CHANPKT_TYPE_COMP) {
			switch (nvs->nvs_type) {
			case HVN_NVS_TYPE_INIT_RESP:
			case HVN_NVS_TYPE_RXBUF_CONNRESP:
			case HVN_NVS_TYPE_CHIM_CONNRESP:
			case HVN_NVS_TYPE_SUBCH_RESP:
				/* copy the response back */
				memcpy(&sc->sc_nvsrsp, nvs, HVN_NVS_MSGSIZE);
				sc->sc_nvsdone = 1;
				wakeup(&sc->sc_nvsrsp);
				break;
			case HVN_NVS_TYPE_RNDIS_ACK:
				dotx = true;
				hvn_txeof(sc, cph->cph_tid);
				break;
			default:
				device_printf(sc->sc_dev,
				    "unhandled NVSP packet type %u "
				    "on completion\n", nvs->nvs_type);
				break;
			}
		} else if (cph->cph_type == VMBUS_CHANPKT_TYPE_RXBUF) {
			switch (nvs->nvs_type) {
			case HVN_NVS_TYPE_RNDIS:
				hvn_rndis_input(sc, cph->cph_tid, cph);
				break;
			default:
				device_printf(sc->sc_dev,
				    "unhandled NVSP packet type %u "
				    "on receive\n", nvs->nvs_type);
				break;
			}
		} else
			device_printf(sc->sc_dev,
			    "unknown NVSP packet type %u\n", cph->cph_type);
	}

	if (dotx)
		if_schedule_deferred_start(ifp);
}

static int
hvn_nvs_cmd(struct hvn_softc *sc, void *cmd, size_t cmdsize, uint64_t tid,
    int timo)
{
	struct hvn_nvs_hdr *hdr = cmd;
	int tries = 10;
	int rv, s;

	sc->sc_nvsdone = 0;

	do {
		rv = vmbus_channel_send(sc->sc_chan, cmd, cmdsize,
		    tid, VMBUS_CHANPKT_TYPE_INBAND,
		    timo ? VMBUS_CHANPKT_FLAG_RC : 0);
		if (rv == EAGAIN) {
			if (cold)
				delay(1000);
			else
				tsleep(cmd, PRIBIO, "nvsout", mstohz(1));
		} else if (rv) {
			DPRINTF("%s: NVSP operation %u send error %d\n",
			    device_xname(sc->sc_dev), hdr->nvs_type, rv);
			return rv;
		}
	} while (rv != 0 && --tries > 0);

	if (tries == 0 && rv != 0) {
		device_printf(sc->sc_dev,
		    "NVSP operation %u send error %d\n", hdr->nvs_type, rv);
		return rv;
	}

	if (timo == 0)
		return 0;

	do {
		if (cold) {
			delay(1000);
			s = splnet();
			hvn_nvs_intr(sc);
			splx(s);
		} else
			tsleep(sc, PRIBIO | PCATCH, "nvscmd", mstohz(1));
	} while (--timo > 0 && sc->sc_nvsdone != 1);

	if (timo == 0 && sc->sc_nvsdone != 1) {
		device_printf(sc->sc_dev, "NVSP operation %u timed out\n",
		    hdr->nvs_type);
		return ETIMEDOUT;
	}
	return 0;
}

static int
hvn_nvs_ack(struct hvn_softc *sc, uint64_t tid)
{
	struct hvn_nvs_rndis_ack cmd;
	int tries = 5;
	int rv;

	cmd.nvs_type = HVN_NVS_TYPE_RNDIS_ACK;
	cmd.nvs_status = HVN_NVS_STATUS_OK;
	do {
		rv = vmbus_channel_send(sc->sc_chan, &cmd, sizeof(cmd),
		    tid, VMBUS_CHANPKT_TYPE_COMP, 0);
		if (rv == EAGAIN)
			delay(10);
		else if (rv) {
			DPRINTF("%s: NVSP acknowledgement error %d\n",
			    device_xname(sc->sc_dev), rv);
			return rv;
		}
	} while (rv != 0 && --tries > 0);
	return rv;
}

static void
hvn_nvs_detach(struct hvn_softc *sc)
{

	if (vmbus_channel_close(sc->sc_chan) == 0) {
		kmem_free(sc->sc_nvsbuf, HVN_NVS_BUFSIZE);
		sc->sc_nvsbuf = NULL;
	}
}

static inline struct rndis_cmd *
hvn_alloc_cmd(struct hvn_softc *sc)
{
	struct rndis_cmd *rc;

	mutex_enter(&sc->sc_cntl_fqlck);
	while ((rc = TAILQ_FIRST(&sc->sc_cntl_fq)) == NULL)
		/* XXX use condvar(9) instead of mtsleep */
		mtsleep(&sc->sc_cntl_fq, PRIBIO, "nvsalloc", 1,
		    &sc->sc_cntl_fqlck);
	TAILQ_REMOVE(&sc->sc_cntl_fq, rc, rc_entry);
	mutex_exit(&sc->sc_cntl_fqlck);
	return rc;
}

static inline void
hvn_submit_cmd(struct hvn_softc *sc, struct rndis_cmd *rc)
{

	mutex_enter(&sc->sc_cntl_sqlck);
	TAILQ_INSERT_TAIL(&sc->sc_cntl_sq, rc, rc_entry);
	mutex_exit(&sc->sc_cntl_sqlck);
}

static inline struct rndis_cmd *
hvn_complete_cmd(struct hvn_softc *sc, uint32_t id)
{
	struct rndis_cmd *rc;

	mutex_enter(&sc->sc_cntl_sqlck);
	TAILQ_FOREACH(rc, &sc->sc_cntl_sq, rc_entry) {
		if (rc->rc_id == id) {
			TAILQ_REMOVE(&sc->sc_cntl_sq, rc, rc_entry);
			break;
		}
	}
	mutex_exit(&sc->sc_cntl_sqlck);
	if (rc != NULL) {
		mutex_enter(&sc->sc_cntl_cqlck);
		TAILQ_INSERT_TAIL(&sc->sc_cntl_cq, rc, rc_entry);
		mutex_exit(&sc->sc_cntl_cqlck);
	}
	return rc;
}

static inline void
hvn_release_cmd(struct hvn_softc *sc, struct rndis_cmd *rc)
{

	mutex_enter(&sc->sc_cntl_cqlck);
	TAILQ_REMOVE(&sc->sc_cntl_cq, rc, rc_entry);
	mutex_exit(&sc->sc_cntl_cqlck);
}

static inline int
hvn_rollback_cmd(struct hvn_softc *sc, struct rndis_cmd *rc)
{
	struct rndis_cmd *rn;

	mutex_enter(&sc->sc_cntl_sqlck);
	TAILQ_FOREACH(rn, &sc->sc_cntl_sq, rc_entry) {
		if (rn == rc) {
			TAILQ_REMOVE(&sc->sc_cntl_sq, rc, rc_entry);
			mutex_exit(&sc->sc_cntl_sqlck);
			return 0;
		}
	}
	mutex_exit(&sc->sc_cntl_sqlck);
	return -1;
}

static inline void
hvn_free_cmd(struct hvn_softc *sc, struct rndis_cmd *rc)
{

	memset(rc->rc_req, 0, sizeof(struct rndis_packet_msg));
	memset(&rc->rc_cmp, 0, sizeof(rc->rc_cmp));
	memset(&rc->rc_msg, 0, sizeof(rc->rc_msg));
	mutex_enter(&sc->sc_cntl_fqlck);
	TAILQ_INSERT_TAIL(&sc->sc_cntl_fq, rc, rc_entry);
	mutex_exit(&sc->sc_cntl_fqlck);
	wakeup(&sc->sc_cntl_fq);
}

static int
hvn_rndis_attach(struct hvn_softc *sc)
{
	const int dmaflags = cold ? BUS_DMA_NOWAIT : BUS_DMA_WAITOK;
	struct rndis_init_req *req;
	struct rndis_init_comp *cmp;
	struct rndis_cmd *rc;
	int i, rv;

	/* RNDIS control message queues */
	TAILQ_INIT(&sc->sc_cntl_sq);
	TAILQ_INIT(&sc->sc_cntl_cq);
	TAILQ_INIT(&sc->sc_cntl_fq);
	mutex_init(&sc->sc_cntl_sqlck, MUTEX_DEFAULT, IPL_NET);
	mutex_init(&sc->sc_cntl_cqlck, MUTEX_DEFAULT, IPL_NET);
	mutex_init(&sc->sc_cntl_fqlck, MUTEX_DEFAULT, IPL_NET);

	for (i = 0; i < HVN_RNDIS_CTLREQS; i++) {
		rc = &sc->sc_cntl_msgs[i];
		if (bus_dmamap_create(sc->sc_dmat, PAGE_SIZE, 1, PAGE_SIZE, 0,
		    dmaflags, &rc->rc_dmap)) {
			DPRINTF("%s: failed to create RNDIS command map\n",
			    device_xname(sc->sc_dev));
			goto errout;
		}
		if (bus_dmamem_alloc(sc->sc_dmat, PAGE_SIZE, PAGE_SIZE,
		    0, &rc->rc_segs, 1, &rc->rc_nsegs, dmaflags)) {
			DPRINTF("%s: failed to allocate RNDIS command\n",
			    device_xname(sc->sc_dev));
			bus_dmamap_destroy(sc->sc_dmat, rc->rc_dmap);
			goto errout;
		}
		if (bus_dmamem_map(sc->sc_dmat, &rc->rc_segs, rc->rc_nsegs,
		    PAGE_SIZE, (void **)&rc->rc_req, dmaflags)) {
			DPRINTF("%s: failed to allocate RNDIS command\n",
			    device_xname(sc->sc_dev));
			bus_dmamem_free(sc->sc_dmat, &rc->rc_segs,
			    rc->rc_nsegs);
			bus_dmamap_destroy(sc->sc_dmat, rc->rc_dmap);
			goto errout;
		}
		memset(rc->rc_req, 0, PAGE_SIZE);
		if (bus_dmamap_load(sc->sc_dmat, rc->rc_dmap, rc->rc_req,
		    PAGE_SIZE, NULL, dmaflags)) {
			DPRINTF("%s: failed to load RNDIS command map\n",
			    device_xname(sc->sc_dev));
			bus_dmamem_free(sc->sc_dmat, &rc->rc_segs,
			    rc->rc_nsegs);
			bus_dmamap_destroy(sc->sc_dmat, rc->rc_dmap);
			goto errout;
		}
		rc->rc_gpa = atop(rc->rc_dmap->dm_segs[0].ds_addr);
		TAILQ_INSERT_TAIL(&sc->sc_cntl_fq, rc, rc_entry);
	}

	rc = hvn_alloc_cmd(sc);

	bus_dmamap_sync(sc->sc_dmat, rc->rc_dmap, 0, PAGE_SIZE,
	    BUS_DMASYNC_PREREAD);

	rc->rc_id = atomic_inc_uint_nv(&sc->sc_rndisrid);

	req = rc->rc_req;
	req->rm_type = REMOTE_NDIS_INITIALIZE_MSG;
	req->rm_len = sizeof(*req);
	req->rm_rid = rc->rc_id;
	req->rm_ver_major = RNDIS_VERSION_MAJOR;
	req->rm_ver_minor = RNDIS_VERSION_MINOR;
	req->rm_max_xfersz = HVN_RNDIS_XFER_SIZE;

	rc->rc_cmplen = sizeof(*cmp);

	bus_dmamap_sync(sc->sc_dmat, rc->rc_dmap, 0, PAGE_SIZE,
	    BUS_DMASYNC_PREWRITE);

	if ((rv = hvn_rndis_cmd(sc, rc, 500)) != 0) {
		DPRINTF("%s: INITIALIZE_MSG failed, error %d\n",
		    device_xname(sc->sc_dev), rv);
		hvn_free_cmd(sc, rc);
		goto errout;
	}
	cmp = (struct rndis_init_comp *)&rc->rc_cmp;
	if (cmp->rm_status != RNDIS_STATUS_SUCCESS) {
		DPRINTF("%s: failed to init RNDIS, error %#x\n",
		    device_xname(sc->sc_dev), cmp->rm_status);
		hvn_free_cmd(sc, rc);
		goto errout;
	}

	hvn_free_cmd(sc, rc);

	/* Initialize RNDIS Data command */
	memset(&sc->sc_data_msg, 0, sizeof(sc->sc_data_msg));
	sc->sc_data_msg.nvs_type = HVN_NVS_TYPE_RNDIS;
	sc->sc_data_msg.nvs_rndis_mtype = HVN_NVS_RNDIS_MTYPE_DATA;
	sc->sc_data_msg.nvs_chim_idx = HVN_NVS_CHIM_IDX_INVALID;

	return 0;

errout:
	for (i = 0; i < HVN_RNDIS_CTLREQS; i++) {
		rc = &sc->sc_cntl_msgs[i];
		if (rc->rc_req == NULL)
			continue;
		TAILQ_REMOVE(&sc->sc_cntl_fq, rc, rc_entry);
		bus_dmamem_free(sc->sc_dmat, &rc->rc_segs, rc->rc_nsegs);
		rc->rc_req = NULL;
		bus_dmamap_destroy(sc->sc_dmat, rc->rc_dmap);
	}
	return -1;
}

static int
hvn_set_capabilities(struct hvn_softc *sc)
{
	struct ndis_offload_params params;
	size_t len = sizeof(params);

	memset(&params, 0, sizeof(params));

	params.ndis_hdr.ndis_type = NDIS_OBJTYPE_DEFAULT;
	if (sc->sc_ndisver < NDIS_VERSION_6_30) {
		params.ndis_hdr.ndis_rev = NDIS_OFFLOAD_PARAMS_REV_2;
		len = params.ndis_hdr.ndis_size = NDIS_OFFLOAD_PARAMS_SIZE_6_1;
	} else {
		params.ndis_hdr.ndis_rev = NDIS_OFFLOAD_PARAMS_REV_3;
		len = params.ndis_hdr.ndis_size = NDIS_OFFLOAD_PARAMS_SIZE;
	}

	params.ndis_ip4csum = NDIS_OFFLOAD_PARAM_TXRX;
	params.ndis_tcp4csum = NDIS_OFFLOAD_PARAM_TXRX;
	params.ndis_tcp6csum = NDIS_OFFLOAD_PARAM_TXRX;
	if (sc->sc_ndisver >= NDIS_VERSION_6_30) {
		params.ndis_udp4csum = NDIS_OFFLOAD_PARAM_TXRX;
		params.ndis_udp6csum = NDIS_OFFLOAD_PARAM_TXRX;
	}

	return hvn_rndis_set(sc, OID_TCP_OFFLOAD_PARAMETERS, &params, len);
}

static int
hvn_rndis_cmd(struct hvn_softc *sc, struct rndis_cmd *rc, int timo)
{
	struct hvn_nvs_rndis *msg = &rc->rc_msg;
	struct rndis_msghdr *hdr = rc->rc_req;
	struct vmbus_gpa sgl[1];
	int tries = 10;
	int rv, s;

	KASSERT(timo > 0);

	msg->nvs_type = HVN_NVS_TYPE_RNDIS;
	msg->nvs_rndis_mtype = HVN_NVS_RNDIS_MTYPE_CTRL;
	msg->nvs_chim_idx = HVN_NVS_CHIM_IDX_INVALID;

	sgl[0].gpa_page = rc->rc_gpa;
	sgl[0].gpa_len = hdr->rm_len;
	sgl[0].gpa_ofs = 0;

	rc->rc_done = 0;

	hvn_submit_cmd(sc, rc);

	do {
		rv = vmbus_channel_send_sgl(sc->sc_chan, sgl, 1, &rc->rc_msg,
		    sizeof(*msg), rc->rc_id);
		if (rv == EAGAIN) {
			if (cold)
				delay(1000);
			else
				tsleep(rc, PRIBIO, "rndisout", mstohz(1));
		} else if (rv) {
			DPRINTF("%s: RNDIS operation %u send error %d\n",
			    device_xname(sc->sc_dev), hdr->rm_type, rv);
			hvn_rollback_cmd(sc, rc);
			return rv;
		}
	} while (rv != 0 && --tries > 0);

	if (tries == 0 && rv != 0) {
		device_printf(sc->sc_dev,
		    "RNDIS operation %u send error %d\n", hdr->rm_type, rv);
		return rv;
	}

	bus_dmamap_sync(sc->sc_dmat, rc->rc_dmap, 0, PAGE_SIZE,
	    BUS_DMASYNC_POSTWRITE);

	do {
		if (cold) {
			delay(1000);
			s = splnet();
			hvn_nvs_intr(sc);
			splx(s);
		} else
			tsleep(rc, PRIBIO | PCATCH, "rndiscmd", mstohz(1));
	} while (--timo > 0 && rc->rc_done != 1);

	bus_dmamap_sync(sc->sc_dmat, rc->rc_dmap, 0, PAGE_SIZE,
	    BUS_DMASYNC_POSTREAD);

	if (rc->rc_done != 1) {
		rv = timo == 0 ? ETIMEDOUT : EINTR;
		if (hvn_rollback_cmd(sc, rc)) {
			hvn_release_cmd(sc, rc);
			rv = 0;
		} else if (rv == ETIMEDOUT) {
			device_printf(sc->sc_dev,
			    "RNDIS operation %u timed out\n", hdr->rm_type);
		}
		return rv;
	}

	hvn_release_cmd(sc, rc);
	return 0;
}

static void
hvn_rndis_input(struct hvn_softc *sc, uint64_t tid, void *arg)
{
	struct vmbus_chanpkt_prplist *cp = arg;
	uint32_t off, len, type;
	int i;

	if (sc->sc_rx_ring == NULL) {
		DPRINTF("%s: invalid rx ring\n", device_xname(sc->sc_dev));
		return;
	}

	for (i = 0; i < cp->cp_range_cnt; i++) {
		off = cp->cp_range[i].gpa_ofs;
		len = cp->cp_range[i].gpa_len;

		KASSERT(off + len <= sc->sc_rx_size);
		KASSERT(len >= RNDIS_HEADER_OFFSET + 4);

		memcpy(&type, sc->sc_rx_ring + off, sizeof(type));
		switch (type) {
		/* data message */
		case REMOTE_NDIS_PACKET_MSG:
			hvn_rxeof(sc, sc->sc_rx_ring + off, len);
			break;
		/* completion messages */
		case REMOTE_NDIS_INITIALIZE_CMPLT:
		case REMOTE_NDIS_QUERY_CMPLT:
		case REMOTE_NDIS_SET_CMPLT:
		case REMOTE_NDIS_RESET_CMPLT:
		case REMOTE_NDIS_KEEPALIVE_CMPLT:
			hvn_rndis_complete(sc, sc->sc_rx_ring + off, len);
			break;
		/* notification message */
		case REMOTE_NDIS_INDICATE_STATUS_MSG:
			hvn_rndis_status(sc, sc->sc_rx_ring + off, len);
			break;
		default:
			device_printf(sc->sc_dev,
			    "unhandled RNDIS message type %u\n", type);
			break;
		}
	}

	hvn_nvs_ack(sc, tid);
}

static inline struct mbuf *
hvn_devget(struct hvn_softc *sc, void *buf, uint32_t len)
{
	struct ifnet *ifp = SC2IFP(sc);
	struct mbuf *m;
	size_t size = len + ETHER_ALIGN;

	MGETHDR(m, M_NOWAIT, MT_DATA);
	if (m == NULL)
		return NULL;

	if (size > MHLEN) {
		if (size <= MCLBYTES)
			MCLGET(m, M_NOWAIT);
		else
			MEXTMALLOC(m, size, M_NOWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			return NULL;
		}
	}

	m->m_len = m->m_pkthdr.len = size;
	m_adj(m, ETHER_ALIGN);
	m_copyback(m, 0, len, buf);
	m_set_rcvif(m, ifp);
	return m;
}

static void
hvn_rxeof(struct hvn_softc *sc, uint8_t *buf, uint32_t len)
{
	struct ifnet *ifp = SC2IFP(sc);
	struct rndis_packet_msg *pkt;
	struct rndis_pktinfo *pi;
	uint32_t csum, vlan;
	struct mbuf *m;

	if (!(ifp->if_flags & IFF_RUNNING))
		return;

	if (len < sizeof(*pkt)) {
		device_printf(sc->sc_dev, "data packet too short: %u\n",
		    len);
		return;
	}

	pkt = (struct rndis_packet_msg *)buf;
	if (pkt->rm_dataoffset + pkt->rm_datalen > len) {
		device_printf(sc->sc_dev,
		    "data packet out of bounds: %u@%u\n", pkt->rm_dataoffset,
		    pkt->rm_datalen);
		return;
	}

	if ((m = hvn_devget(sc, buf + RNDIS_HEADER_OFFSET + pkt->rm_dataoffset,
	    pkt->rm_datalen)) == NULL) {
		ifp->if_ierrors++;
		return;
	}

	if (pkt->rm_pktinfooffset + pkt->rm_pktinfolen > len) {
		device_printf(sc->sc_dev,
		    "pktinfo is out of bounds: %u@%u vs %u\n",
		    pkt->rm_pktinfolen, pkt->rm_pktinfooffset, len);
		goto done;
	}

	pi = (struct rndis_pktinfo *)(buf + RNDIS_HEADER_OFFSET +
	    pkt->rm_pktinfooffset);
	while (pkt->rm_pktinfolen > 0) {
		if (pi->rm_size > pkt->rm_pktinfolen) {
			device_printf(sc->sc_dev,
			    "invalid pktinfo size: %u/%u\n", pi->rm_size,
			    pkt->rm_pktinfolen);
			break;
		}

		switch (pi->rm_type) {
		case NDIS_PKTINFO_TYPE_CSUM:
			memcpy(&csum, pi->rm_data, sizeof(csum));
			if (csum & NDIS_RXCSUM_INFO_IPCS_OK)
				m->m_pkthdr.csum_flags |= M_CSUM_IPv4;
			if (csum & NDIS_RXCSUM_INFO_TCPCS_OK)
				m->m_pkthdr.csum_flags |= M_CSUM_TCPv4;
			if (csum & NDIS_RXCSUM_INFO_UDPCS_OK)
				m->m_pkthdr.csum_flags |= M_CSUM_UDPv4;
			break;
		case NDIS_PKTINFO_TYPE_VLAN:
			memcpy(&vlan, pi->rm_data, sizeof(vlan));
			if (vlan != 0xffffffff) {
				m->m_pkthdr.ether_vtag =
				    NDIS_VLAN_INFO_ID(vlan) |
				    (NDIS_VLAN_INFO_PRI(vlan) << EVL_PRIO_BITS);
				m->m_flags |= M_VLANTAG;
			}
			break;
		default:
			DPRINTF("%s: unhandled pktinfo type %u\n",
			    device_xname(sc->sc_dev), pi->rm_type);
			break;
		}

		pkt->rm_pktinfolen -= pi->rm_size;
		pi = (struct rndis_pktinfo *)((char *)pi + pi->rm_size);
	}

 done:
	if_percpuq_enqueue(sc->sc_ipq, m);
}

static void
hvn_rndis_complete(struct hvn_softc *sc, uint8_t *buf, uint32_t len)
{
	struct rndis_cmd *rc;
	uint32_t id;

	memcpy(&id, buf + RNDIS_HEADER_OFFSET, sizeof(id));
	if ((rc = hvn_complete_cmd(sc, id)) != NULL) {
		if (len < rc->rc_cmplen)
			device_printf(sc->sc_dev,
			    "RNDIS response %u too short: %u\n", id, len);
		else
			memcpy(&rc->rc_cmp, buf, rc->rc_cmplen);
		if (len > rc->rc_cmplen &&
		    len - rc->rc_cmplen > HVN_RNDIS_BUFSIZE)
			device_printf(sc->sc_dev,
			    "RNDIS response %u too large: %u\n", id, len);
		else if (len > rc->rc_cmplen)
			memcpy(&rc->rc_cmpbuf, buf + rc->rc_cmplen,
			    len - rc->rc_cmplen);
		rc->rc_done = 1;
		wakeup(rc);
	} else {
		DPRINTF("%s: failed to complete RNDIS request id %u\n",
		    device_xname(sc->sc_dev), id);
	}
}

static int
hvn_rndis_output(struct hvn_softc *sc, struct hvn_tx_desc *txd)
{
	uint64_t rid = (uint64_t)txd->txd_id << 32;
	int rv;

	rv = vmbus_channel_send_sgl(sc->sc_chan, txd->txd_sgl, txd->txd_nsge,
	    &sc->sc_data_msg, sizeof(sc->sc_data_msg), rid);
	if (rv) {
		DPRINTF("%s: RNDIS data send error %d\n",
		    device_xname(sc->sc_dev), rv);
		return rv;
	}
	return 0;
}

static void
hvn_rndis_status(struct hvn_softc *sc, uint8_t *buf, uint32_t len)
{
	struct ifnet *ifp = SC2IFP(sc);
	uint32_t status;
	int link_state = sc->sc_link_state;

	memcpy(&status, buf + RNDIS_HEADER_OFFSET, sizeof(status));
	switch (status) {
	case RNDIS_STATUS_MEDIA_CONNECT:
		sc->sc_link_state = LINK_STATE_UP;
		break;
	case RNDIS_STATUS_MEDIA_DISCONNECT:
		sc->sc_link_state = LINK_STATE_DOWN;
		break;
	/* Ignore these */
	case RNDIS_STATUS_OFFLOAD_CURRENT_CONFIG:
		return;
	default:
		DPRINTF("%s: unhandled status %#x\n", device_xname(sc->sc_dev),
		    status);
		return;
	}
	if (link_state != sc->sc_link_state)
		if_link_state_change(ifp, sc->sc_link_state);
}

static int
hvn_rndis_query(struct hvn_softc *sc, uint32_t oid, void *res, size_t *length)
{
	struct rndis_cmd *rc;
	struct rndis_query_req *req;
	struct rndis_query_comp *cmp;
	size_t olength = *length;
	int rv;

	rc = hvn_alloc_cmd(sc);

	bus_dmamap_sync(sc->sc_dmat, rc->rc_dmap, 0, PAGE_SIZE,
	    BUS_DMASYNC_PREREAD);

	rc->rc_id = atomic_inc_uint_nv(&sc->sc_rndisrid);

	req = rc->rc_req;
	req->rm_type = REMOTE_NDIS_QUERY_MSG;
	req->rm_len = sizeof(*req);
	req->rm_rid = rc->rc_id;
	req->rm_oid = oid;
	req->rm_infobufoffset = sizeof(*req) - RNDIS_HEADER_OFFSET;

	rc->rc_cmplen = sizeof(*cmp);

	bus_dmamap_sync(sc->sc_dmat, rc->rc_dmap, 0, PAGE_SIZE,
	    BUS_DMASYNC_PREWRITE);

	if ((rv = hvn_rndis_cmd(sc, rc, 500)) != 0) {
		DPRINTF("%s: QUERY_MSG failed, error %d\n",
		    device_xname(sc->sc_dev), rv);
		hvn_free_cmd(sc, rc);
		return rv;
	}

	cmp = (struct rndis_query_comp *)&rc->rc_cmp;
	switch (cmp->rm_status) {
	case RNDIS_STATUS_SUCCESS:
		if (cmp->rm_infobuflen > olength) {
			rv = EINVAL;
			break;
		}
		memcpy(res, rc->rc_cmpbuf, cmp->rm_infobuflen);
		*length = cmp->rm_infobuflen;
		break;
	default:
		*length = 0;
		rv = EIO;
		break;
	}

	hvn_free_cmd(sc, rc);
	return rv;
}

static int
hvn_rndis_set(struct hvn_softc *sc, uint32_t oid, void *data, size_t length)
{
	struct rndis_cmd *rc;
	struct rndis_set_req *req;
	struct rndis_set_comp *cmp;
	int rv;

	rc = hvn_alloc_cmd(sc);

	bus_dmamap_sync(sc->sc_dmat, rc->rc_dmap, 0, PAGE_SIZE,
	    BUS_DMASYNC_PREREAD);

	rc->rc_id = atomic_inc_uint_nv(&sc->sc_rndisrid);

	req = rc->rc_req;
	req->rm_type = REMOTE_NDIS_SET_MSG;
	req->rm_len = sizeof(*req) + length;
	req->rm_rid = rc->rc_id;
	req->rm_oid = oid;
	req->rm_infobufoffset = sizeof(*req) - RNDIS_HEADER_OFFSET;

	rc->rc_cmplen = sizeof(*cmp);

	if (length > 0) {
		KASSERT(sizeof(*req) + length < PAGE_SIZE);
		req->rm_infobuflen = length;
		memcpy(req + 1, data, length);
	}

	bus_dmamap_sync(sc->sc_dmat, rc->rc_dmap, 0, PAGE_SIZE,
	    BUS_DMASYNC_PREWRITE);

	if ((rv = hvn_rndis_cmd(sc, rc, 500)) != 0) {
		DPRINTF("%s: SET_MSG failed, error %d\n",
		    device_xname(sc->sc_dev), rv);
		hvn_free_cmd(sc, rc);
		return rv;
	}

	cmp = (struct rndis_set_comp *)&rc->rc_cmp;
	if (cmp->rm_status != RNDIS_STATUS_SUCCESS)
		rv = EIO;

	hvn_free_cmd(sc, rc);
	return rv;
}

static int
hvn_rndis_open(struct hvn_softc *sc)
{
	uint32_t filter;
	int rv;

	if (sc->sc_promisc)
		filter = RNDIS_PACKET_TYPE_PROMISCUOUS;
	else
		filter = RNDIS_PACKET_TYPE_BROADCAST |
		    RNDIS_PACKET_TYPE_ALL_MULTICAST |
		    RNDIS_PACKET_TYPE_DIRECTED;

	rv = hvn_rndis_set(sc, OID_GEN_CURRENT_PACKET_FILTER,
	    &filter, sizeof(filter));
	if (rv) {
		DPRINTF("%s: failed to set RNDIS filter to %#x\n",
		    device_xname(sc->sc_dev), filter);
	}
	return rv;
}

static int
hvn_rndis_close(struct hvn_softc *sc)
{
	uint32_t filter = 0;
	int rv;

	rv = hvn_rndis_set(sc, OID_GEN_CURRENT_PACKET_FILTER,
	    &filter, sizeof(filter));
	if (rv) {
		DPRINTF("%s: failed to clear RNDIS filter\n",
		    device_xname(sc->sc_dev));
	}
	return rv;
}

static void
hvn_rndis_detach(struct hvn_softc *sc)
{
	struct rndis_cmd *rc;
	struct rndis_halt_req *req;
	int rv;

	rc = hvn_alloc_cmd(sc);

	bus_dmamap_sync(sc->sc_dmat, rc->rc_dmap, 0, PAGE_SIZE,
	    BUS_DMASYNC_PREREAD);

	rc->rc_id = atomic_inc_uint_nv(&sc->sc_rndisrid);

	req = rc->rc_req;
	req->rm_type = REMOTE_NDIS_HALT_MSG;
	req->rm_len = sizeof(*req);
	req->rm_rid = rc->rc_id;

	bus_dmamap_sync(sc->sc_dmat, rc->rc_dmap, 0, PAGE_SIZE,
	    BUS_DMASYNC_PREWRITE);

	if ((rv = hvn_rndis_cmd(sc, rc, 500)) != 0) {
		DPRINTF("%s: HALT_MSG failed, error %d\n",
		    device_xname(sc->sc_dev), rv);
	}
	hvn_free_cmd(sc, rc);
}

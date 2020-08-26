/*      $NetBSD: if_xennet_xenbus.c,v 1.128 2020/08/26 15:54:10 riastradh Exp $      */

/*
 * Copyright (c) 2006 Manuel Bouyer.
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

/*
 * Copyright (c) 2004 Christian Limpach.
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

/*
 * This file contains the xennet frontend code required for the network
 * communication between two Xen domains.
 * It ressembles xbd, but is a little more complex as it must deal with two
 * rings:
 * - the TX ring, to transmit packets to backend (inside => outside)
 * - the RX ring, to receive packets from backend (outside => inside)
 *
 * Principles are following.
 *
 * For TX:
 * Purpose is to transmit packets to the outside. The start of day is in
 * xennet_start() (output routine of xennet) scheduled via a softint.
 * xennet_start() generates the requests associated
 * to the TX mbufs queued (see altq(9)).
 * The backend's responses are processed by xennet_tx_complete(), called
 * from xennet_start()
 *
 * for RX:
 * Purpose is to process the packets received from the outside. RX buffers
 * are pre-allocated through xennet_alloc_rx_buffer(), during xennet autoconf
 * attach. During pre-allocation, frontend pushes requests in the I/O ring, in
 * preparation for incoming packets from backend.
 * When RX packets need to be processed, backend takes the requests previously
 * offered by frontend and pushes the associated responses inside the I/O ring.
 * When done, it notifies frontend through an event notification, which will
 * asynchronously call xennet_handler() in frontend.
 * xennet_handler() processes the responses, generates the associated mbuf, and
 * passes it to the MI layer for further processing.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_xennet_xenbus.c,v 1.128 2020/08/26 15:54:10 riastradh Exp $");

#include "opt_xen.h"
#include "opt_nfs_boot.h"
#include "opt_net_mpsafe.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/intr.h>
#include <sys/rndsource.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/bpf.h>

#if defined(NFS_BOOT_BOOTSTATIC)
#include <sys/fstypes.h>
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <netinet/in.h>
#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>
#include <nfs/nfsdiskless.h>
#include <xen/if_xennetvar.h>
#endif /* defined(NFS_BOOT_BOOTSTATIC) */

#include <xen/xennet_checksum.h>

#include <uvm/uvm.h>

#include <xen/intr.h>
#include <xen/hypervisor.h>
#include <xen/evtchn.h>
#include <xen/granttables.h>
#include <xen/include/public/io/netif.h>
#include <xen/xenpmap.h>

#include <xen/xenbus.h>
#include "locators.h"

#undef XENNET_DEBUG_DUMP
#undef XENNET_DEBUG

#ifdef XENNET_DEBUG
#define XEDB_FOLLOW     0x01
#define XEDB_INIT       0x02
#define XEDB_EVENT      0x04
#define XEDB_MBUF       0x08
#define XEDB_MEM        0x10
int xennet_debug = 0xff;
#define DPRINTF(x) if (xennet_debug) printf x;
#define DPRINTFN(n,x) if (xennet_debug & (n)) printf x;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define GRANT_INVALID_REF -1 /* entry is free */

#define NET_TX_RING_SIZE __CONST_RING_SIZE(netif_tx, PAGE_SIZE)
#define NET_RX_RING_SIZE __CONST_RING_SIZE(netif_rx, PAGE_SIZE)

struct xennet_txreq {
	SLIST_ENTRY(xennet_txreq) txreq_next;
	uint16_t txreq_id; /* ID passed to backend */
	grant_ref_t txreq_gntref; /* grant ref of this request */
	struct mbuf *txreq_m; /* mbuf being transmitted */
	bus_dmamap_t txreq_dmamap;
};

struct xennet_rxreq {
	SLIST_ENTRY(xennet_rxreq) rxreq_next;
	uint16_t rxreq_id; /* ID passed to backend */
	grant_ref_t rxreq_gntref; /* grant ref of this request */
	struct mbuf *rxreq_m;
	bus_dmamap_t rxreq_dmamap;
};

struct xennet_xenbus_softc {
	device_t sc_dev;
	struct ethercom sc_ethercom;
	uint8_t sc_enaddr[ETHER_ADDR_LEN];
	struct xenbus_device *sc_xbusd;

	netif_tx_front_ring_t sc_tx_ring;
	netif_rx_front_ring_t sc_rx_ring;

	unsigned int sc_evtchn;
	struct intrhand *sc_ih;

	grant_ref_t sc_tx_ring_gntref;
	grant_ref_t sc_rx_ring_gntref;

	kmutex_t sc_tx_lock; /* protects free TX list, TX ring */
	kmutex_t sc_rx_lock; /* protects free RX list, RX ring, rxreql */
	struct xennet_txreq sc_txreqs[NET_TX_RING_SIZE];
	struct xennet_rxreq sc_rxreqs[NET_RX_RING_SIZE];
	SLIST_HEAD(,xennet_txreq) sc_txreq_head; /* list of free TX requests */
	SLIST_HEAD(,xennet_rxreq) sc_rxreq_head; /* list of free RX requests */
	int sc_free_txreql; /* number of free transmit request structs */
	int sc_free_rxreql; /* number of free receive request structs */

	int sc_backend_status; /* our status with backend */
#define BEST_CLOSED		0
#define BEST_DISCONNECTED	1
#define BEST_CONNECTED		2
#define BEST_SUSPENDED		3
	int sc_features;
#define FEATURE_IPV6CSUM	0x01	/* IPv6 checksum offload */
#define FEATURE_SG		0x02	/* scatter-gatter */
#define FEATURE_RX_COPY		0x04	/* RX-copy */
#define FEATURE_BITS		"\20\1IPV6-CSUM\2SG\3RX-COPY"
	krndsource_t sc_rnd_source;
	struct evcnt sc_cnt_tx_defrag;
	struct evcnt sc_cnt_tx_queue_full;
	struct evcnt sc_cnt_tx_drop;
	struct evcnt sc_cnt_tx_frag;
	struct evcnt sc_cnt_rx_frag;
	struct evcnt sc_cnt_rx_cksum_blank;
	struct evcnt sc_cnt_rx_cksum_undefer;
};

static pool_cache_t if_xennetrxbuf_cache;
static int if_xennetrxbuf_cache_inited = 0;

static int  xennet_xenbus_match(device_t, cfdata_t, void *);
static void xennet_xenbus_attach(device_t, device_t, void *);
static int  xennet_xenbus_detach(device_t, int);
static void xennet_backend_changed(void *, XenbusState);

static void xennet_alloc_rx_buffer(struct xennet_xenbus_softc *);
static void xennet_free_rx_buffer(struct xennet_xenbus_softc *, bool);
static void xennet_tx_complete(struct xennet_xenbus_softc *);
static void xennet_rx_mbuf_free(struct mbuf *, void *, size_t, void *);
static int  xennet_handler(void *);
static bool xennet_talk_to_backend(struct xennet_xenbus_softc *);
#ifdef XENNET_DEBUG_DUMP
static void xennet_hex_dump(const unsigned char *, size_t, const char *, int);
#endif

static int  xennet_init(struct ifnet *);
static void xennet_stop(struct ifnet *, int);
static void xennet_start(struct ifnet *);
static int  xennet_ioctl(struct ifnet *, u_long, void *);

static bool xennet_xenbus_suspend(device_t dev, const pmf_qual_t *);
static bool xennet_xenbus_resume(device_t dev, const pmf_qual_t *);

CFATTACH_DECL3_NEW(xennet, sizeof(struct xennet_xenbus_softc),
   xennet_xenbus_match, xennet_xenbus_attach, xennet_xenbus_detach, NULL,
   NULL, NULL, DVF_DETACH_SHUTDOWN);

static int
xennet_xenbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct xenbusdev_attach_args *xa = aux;

	if (strcmp(xa->xa_type, "vif") != 0)
		return 0;

	if (match->cf_loc[XENBUSCF_ID] != XENBUSCF_ID_DEFAULT &&
	    match->cf_loc[XENBUSCF_ID] != xa->xa_id)
		return 0;

	return 1;
}

static void
xennet_xenbus_attach(device_t parent, device_t self, void *aux)
{
	struct xennet_xenbus_softc *sc = device_private(self);
	struct xenbusdev_attach_args *xa = aux;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int err;
	netif_tx_sring_t *tx_ring;
	netif_rx_sring_t *rx_ring;
	RING_IDX i;
	char *e, *p;
	unsigned long uval;
	extern int ifqmaxlen; /* XXX */
	char mac[32];
	char buf[64];
	bus_size_t maxsz;
	int nsegs;

	aprint_normal(": Xen Virtual Network Interface\n");
	sc->sc_dev = self;

	sc->sc_xbusd = xa->xa_xbusd;
	sc->sc_xbusd->xbusd_otherend_changed = xennet_backend_changed;

	/* read feature support flags */
	err = xenbus_read_ul(NULL, sc->sc_xbusd->xbusd_otherend,
	    "feature-rx-copy", &uval, 10);
	if (!err && uval == 1)
		sc->sc_features |= FEATURE_RX_COPY;
	err = xenbus_read_ul(NULL, sc->sc_xbusd->xbusd_otherend,
	    "feature-ipv6-csum-offload", &uval, 10);
	if (!err && uval == 1)
		sc->sc_features |= FEATURE_IPV6CSUM;
	err = xenbus_read_ul(NULL, sc->sc_xbusd->xbusd_otherend,
	    "feature-sg", &uval, 10);
	if (!err && uval == 1)
		sc->sc_features |= FEATURE_SG;
	snprintb(buf, sizeof(buf), FEATURE_BITS, sc->sc_features);
	aprint_normal_dev(sc->sc_dev, "backend features %s\n", buf);

	/* xenbus ensure 2 devices can't be probed at the same time */
	if (if_xennetrxbuf_cache_inited == 0) {
		if_xennetrxbuf_cache = pool_cache_init(PAGE_SIZE, 0, 0, 0,
		    "xnfrx", NULL, IPL_NET, NULL, NULL, NULL);
		if_xennetrxbuf_cache_inited = 1;
	}

	/* initialize free RX and RX request lists */
	if (sc->sc_features & FEATURE_SG) {
		maxsz = ETHER_MAX_LEN_JUMBO;
		/*
		 * Linux netback drops the packet if the request has more
		 * segments than XEN_NETIF_NR_SLOTS_MIN (== 18). With 2KB
		 * MCLBYTES this means maximum packet size 36KB, in reality
		 * less due to mbuf chain fragmentation.
		 */
		nsegs = XEN_NETIF_NR_SLOTS_MIN;
	} else {
		maxsz = PAGE_SIZE;
		nsegs = 1;
	}
	mutex_init(&sc->sc_tx_lock, MUTEX_DEFAULT, IPL_NET);
	SLIST_INIT(&sc->sc_txreq_head);
	for (i = 0; i < NET_TX_RING_SIZE; i++) {
		struct xennet_txreq *txreq = &sc->sc_txreqs[i];

		txreq->txreq_id = i;
		if (bus_dmamap_create(sc->sc_xbusd->xbusd_dmat, maxsz, nsegs,
		    PAGE_SIZE, PAGE_SIZE, BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW,
		    &txreq->txreq_dmamap) != 0)
			break;

		SLIST_INSERT_HEAD(&sc->sc_txreq_head, &sc->sc_txreqs[i],
		    txreq_next);
	}
	sc->sc_free_txreql = i;

	mutex_init(&sc->sc_rx_lock, MUTEX_DEFAULT, IPL_NET);
	SLIST_INIT(&sc->sc_rxreq_head);
	for (i = 0; i < NET_RX_RING_SIZE; i++) {
		struct xennet_rxreq *rxreq = &sc->sc_rxreqs[i];
		rxreq->rxreq_id = i;
		if (bus_dmamap_create(sc->sc_xbusd->xbusd_dmat, maxsz, nsegs,
		    PAGE_SIZE, PAGE_SIZE, BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW,
		    &rxreq->rxreq_dmamap) != 0)
			break;
		rxreq->rxreq_gntref = GRANT_INVALID_REF;
		SLIST_INSERT_HEAD(&sc->sc_rxreq_head, rxreq, rxreq_next);
	}
	sc->sc_free_rxreql = i;
	if (sc->sc_free_rxreql == 0) {
		aprint_error_dev(self, "failed to allocate rx memory\n");
		return;
	}

	/* read mac address */
	err = xenbus_read(NULL, sc->sc_xbusd->xbusd_path, "mac",
	    mac, sizeof(mac));
	if (err) {
		aprint_error_dev(self, "can't read mac address, err %d\n", err);
		return;
	}
	for (i = 0, p = mac; i < ETHER_ADDR_LEN; i++) {
		sc->sc_enaddr[i] = strtoul(p, &e, 16);
		if ((e[0] == '\0' && i != 5) && e[0] != ':') {
			aprint_error_dev(self,
			    "%s is not a valid mac address\n", mac);
			return;
		}
		p = &e[1];
	}
	aprint_normal_dev(self, "MAC address %s\n",
	    ether_sprintf(sc->sc_enaddr));

	/* Initialize ifnet structure and attach interface */
	strlcpy(ifp->if_xname, device_xname(self), IFNAMSIZ);
	sc->sc_ethercom.ec_capabilities |= ETHERCAP_VLAN_MTU;
	if (sc->sc_features & FEATURE_SG)
		sc->sc_ethercom.ec_capabilities |= ETHERCAP_JUMBO_MTU;
	ifp->if_softc = sc;
	ifp->if_start = xennet_start;
	ifp->if_ioctl = xennet_ioctl;
	ifp->if_init = xennet_init;
	ifp->if_stop = xennet_stop;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_extflags = IFEF_MPSAFE;
	ifp->if_snd.ifq_maxlen = uimax(ifqmaxlen, NET_TX_RING_SIZE * 2);
	ifp->if_capabilities =
		IFCAP_CSUM_UDPv4_Rx | IFCAP_CSUM_UDPv4_Tx
		| IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_TCPv4_Tx
		| IFCAP_CSUM_UDPv6_Rx
		| IFCAP_CSUM_TCPv6_Rx;
#define XN_M_CSUM_SUPPORTED						\
	(M_CSUM_TCPv4 | M_CSUM_UDPv4 | M_CSUM_TCPv6 | M_CSUM_UDPv6)

	if (sc->sc_features & FEATURE_IPV6CSUM) {
		/*
		 * If backend supports IPv6 csum offloading, we can skip
		 * IPv6 csum for Tx packets. Rx packet validation can
		 * be skipped regardless.
		 */
		ifp->if_capabilities |=
		    IFCAP_CSUM_UDPv6_Tx | IFCAP_CSUM_TCPv6_Tx;
	}

	IFQ_SET_MAXLEN(&ifp->if_snd, uimax(2 * NET_TX_RING_SIZE, IFQ_MAXLEN));
	IFQ_SET_READY(&ifp->if_snd);
	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, sc->sc_enaddr);

	/* alloc shared rings */
	tx_ring = (void *)uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
	    UVM_KMF_WIRED);
	rx_ring = (void *)uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
	    UVM_KMF_WIRED);
	if (tx_ring == NULL || rx_ring == NULL)
		panic("%s: can't alloc rings", device_xname(self));

	sc->sc_tx_ring.sring = tx_ring;
	sc->sc_rx_ring.sring = rx_ring;

	rnd_attach_source(&sc->sc_rnd_source, device_xname(sc->sc_dev),
	    RND_TYPE_NET, RND_FLAG_DEFAULT);

	evcnt_attach_dynamic(&sc->sc_cnt_tx_defrag, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx packet defrag");
	evcnt_attach_dynamic(&sc->sc_cnt_tx_frag, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx multi-segment packet");
	evcnt_attach_dynamic(&sc->sc_cnt_tx_drop, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx packet dropped");
	evcnt_attach_dynamic(&sc->sc_cnt_tx_queue_full, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx queue full");
	evcnt_attach_dynamic(&sc->sc_cnt_rx_frag, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx multi-segment packet");
	evcnt_attach_dynamic(&sc->sc_cnt_rx_cksum_blank, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx csum blank");
	evcnt_attach_dynamic(&sc->sc_cnt_rx_cksum_undefer, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx csum undeferred");

	if (!pmf_device_register(self, xennet_xenbus_suspend,
	    xennet_xenbus_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
	else
		pmf_class_network_register(self, ifp);

	/* resume shared structures and tell backend that we are ready */
	if (xennet_xenbus_resume(self, PMF_Q_NONE) == false) {
		uvm_km_free(kernel_map, (vaddr_t)tx_ring, PAGE_SIZE,
		    UVM_KMF_WIRED);
		uvm_km_free(kernel_map, (vaddr_t)rx_ring, PAGE_SIZE,
		    UVM_KMF_WIRED);
		return;
	}
}

static int
xennet_xenbus_detach(device_t self, int flags)
{
	struct xennet_xenbus_softc *sc = device_private(self);
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	if ((flags & (DETACH_SHUTDOWN | DETACH_FORCE)) == DETACH_SHUTDOWN) {
		/* Trigger state transition with backend */
		xenbus_switch_state(sc->sc_xbusd, NULL, XenbusStateClosing);
		return EBUSY;
	}

	DPRINTF(("%s: xennet_xenbus_detach\n", device_xname(self)));

	/* stop interface */
	IFNET_LOCK(ifp);
	xennet_stop(ifp, 1);
	IFNET_UNLOCK(ifp);
	if (sc->sc_ih != NULL) {
		xen_intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}

	/* collect any outstanding TX responses */
	mutex_enter(&sc->sc_tx_lock);
	xennet_tx_complete(sc);
	while (sc->sc_tx_ring.sring->rsp_prod != sc->sc_tx_ring.rsp_cons) {
		kpause("xndetach", true, hz/2, &sc->sc_tx_lock);
		xennet_tx_complete(sc);
	}
	mutex_exit(&sc->sc_tx_lock);

	mutex_enter(&sc->sc_rx_lock);
	xennet_free_rx_buffer(sc, true);
	mutex_exit(&sc->sc_rx_lock);

	ether_ifdetach(ifp);
	if_detach(ifp);

	evcnt_detach(&sc->sc_cnt_tx_defrag);
	evcnt_detach(&sc->sc_cnt_tx_frag);
	evcnt_detach(&sc->sc_cnt_tx_drop);
	evcnt_detach(&sc->sc_cnt_tx_queue_full);
	evcnt_detach(&sc->sc_cnt_rx_frag);
	evcnt_detach(&sc->sc_cnt_rx_cksum_blank);
	evcnt_detach(&sc->sc_cnt_rx_cksum_undefer);

	/* Unhook the entropy source. */
	rnd_detach_source(&sc->sc_rnd_source);

	/* Wait until the tx/rx rings stop being used by backend */
	mutex_enter(&sc->sc_tx_lock);
	while (xengnt_status(sc->sc_tx_ring_gntref))
		kpause("xntxref", true, hz/2, &sc->sc_tx_lock);
	xengnt_revoke_access(sc->sc_tx_ring_gntref);
	mutex_exit(&sc->sc_tx_lock);
	uvm_km_free(kernel_map, (vaddr_t)sc->sc_tx_ring.sring, PAGE_SIZE,
	    UVM_KMF_WIRED);
	mutex_enter(&sc->sc_rx_lock);
	while (xengnt_status(sc->sc_rx_ring_gntref))
		kpause("xnrxref", true, hz/2, &sc->sc_rx_lock);
	xengnt_revoke_access(sc->sc_rx_ring_gntref);
	mutex_exit(&sc->sc_rx_lock);
	uvm_km_free(kernel_map, (vaddr_t)sc->sc_rx_ring.sring, PAGE_SIZE,
	    UVM_KMF_WIRED);

	pmf_device_deregister(self);

	sc->sc_backend_status = BEST_DISCONNECTED;

	DPRINTF(("%s: xennet_xenbus_detach done\n", device_xname(self)));
	return 0;
}

static bool
xennet_xenbus_resume(device_t dev, const pmf_qual_t *qual)
{
	struct xennet_xenbus_softc *sc = device_private(dev);
	int error;
	netif_tx_sring_t *tx_ring;
	netif_rx_sring_t *rx_ring;
	paddr_t ma;

	/* All grants were removed during suspend */
	sc->sc_tx_ring_gntref = GRANT_INVALID_REF;
	sc->sc_rx_ring_gntref = GRANT_INVALID_REF;

	mutex_enter(&sc->sc_rx_lock);
	/* Free but don't revoke, the grant is gone */
	xennet_free_rx_buffer(sc, false);
	KASSERT(sc->sc_free_rxreql == NET_TX_RING_SIZE);
	mutex_exit(&sc->sc_rx_lock);

	tx_ring = sc->sc_tx_ring.sring;
	rx_ring = sc->sc_rx_ring.sring;

	/* Initialize rings */
	memset(tx_ring, 0, PAGE_SIZE);
	SHARED_RING_INIT(tx_ring);
	FRONT_RING_INIT(&sc->sc_tx_ring, tx_ring, PAGE_SIZE);

	memset(rx_ring, 0, PAGE_SIZE);
	SHARED_RING_INIT(rx_ring);
	FRONT_RING_INIT(&sc->sc_rx_ring, rx_ring, PAGE_SIZE);

	(void)pmap_extract_ma(pmap_kernel(), (vaddr_t)tx_ring, &ma);
	error = xenbus_grant_ring(sc->sc_xbusd, ma, &sc->sc_tx_ring_gntref);
	if (error)
		goto abort_resume;
	(void)pmap_extract_ma(pmap_kernel(), (vaddr_t)rx_ring, &ma);
	error = xenbus_grant_ring(sc->sc_xbusd, ma, &sc->sc_rx_ring_gntref);
	if (error)
		goto abort_resume;

	if (sc->sc_ih != NULL) {
		xen_intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}
	error = xenbus_alloc_evtchn(sc->sc_xbusd, &sc->sc_evtchn);
	if (error)
		goto abort_resume;
	aprint_verbose_dev(dev, "using event channel %d\n",
	    sc->sc_evtchn);
	sc->sc_ih = xen_intr_establish_xname(-1, &xen_pic, sc->sc_evtchn,
	    IST_LEVEL, IPL_NET, &xennet_handler, sc, true, device_xname(dev));
	KASSERT(sc->sc_ih != NULL);

	/* Re-fill Rx ring */
	mutex_enter(&sc->sc_rx_lock);
	xennet_alloc_rx_buffer(sc);
	KASSERT(sc->sc_free_rxreql == 0);
	mutex_exit(&sc->sc_rx_lock);

	xenbus_switch_state(sc->sc_xbusd, NULL, XenbusStateInitialised);

	if (sc->sc_backend_status == BEST_SUSPENDED) {
		if (xennet_talk_to_backend(sc)) {
			xenbus_device_resume(sc->sc_xbusd);
			hypervisor_unmask_event(sc->sc_evtchn);
			xenbus_switch_state(sc->sc_xbusd, NULL,
			    XenbusStateConnected);
		}
	}

	return true;

abort_resume:
	xenbus_dev_fatal(sc->sc_xbusd, error, "resuming device");
	return false;
}

static bool
xennet_talk_to_backend(struct xennet_xenbus_softc *sc)
{
	int error;
	struct xenbus_transaction *xbt;
	const char *errmsg;

again:
	xbt = xenbus_transaction_start();
	if (xbt == NULL)
		return false;
	error = xenbus_printf(xbt, sc->sc_xbusd->xbusd_path,
	    "vifname", "%s", device_xname(sc->sc_dev));
	if (error) {
		errmsg = "vifname";
		goto abort_transaction;
	}
	error = xenbus_printf(xbt, sc->sc_xbusd->xbusd_path,
	    "tx-ring-ref","%u", sc->sc_tx_ring_gntref);
	if (error) {
		errmsg = "writing tx ring-ref";
		goto abort_transaction;
	}
	error = xenbus_printf(xbt, sc->sc_xbusd->xbusd_path,
	    "rx-ring-ref","%u", sc->sc_rx_ring_gntref);
	if (error) {
		errmsg = "writing rx ring-ref";
		goto abort_transaction;
	}
	error = xenbus_printf(xbt, sc->sc_xbusd->xbusd_path,
	    "request-rx-copy", "%u", 1);
	if (error) {
		errmsg = "writing request-rx-copy";
		goto abort_transaction;
	}
	error = xenbus_printf(xbt, sc->sc_xbusd->xbusd_path,
	    "feature-rx-notify", "%u", 1);
	if (error) {
		errmsg = "writing feature-rx-notify";
		goto abort_transaction;
	}
	error = xenbus_printf(xbt, sc->sc_xbusd->xbusd_path,
	    "feature-ipv6-csum-offload", "%u", 1);
	if (error) {
		errmsg = "writing feature-ipv6-csum-offload";
		goto abort_transaction;
	}
	error = xenbus_printf(xbt, sc->sc_xbusd->xbusd_path,
	    "feature-sg", "%u", 1);
	if (error) {
		errmsg = "writing feature-sg";
		goto abort_transaction;
	}
	error = xenbus_printf(xbt, sc->sc_xbusd->xbusd_path,
	    "event-channel", "%u", sc->sc_evtchn);
	if (error) {
		errmsg = "writing event channel";
		goto abort_transaction;
	}
	error = xenbus_transaction_end(xbt, 0);
	if (error == EAGAIN)
		goto again;
	if (error) {
		xenbus_dev_fatal(sc->sc_xbusd, error, "completing transaction");
		return false;
	}
	mutex_enter(&sc->sc_rx_lock);
	xennet_alloc_rx_buffer(sc);
	mutex_exit(&sc->sc_rx_lock);

	sc->sc_backend_status = BEST_CONNECTED;

	return true;

abort_transaction:
	xenbus_transaction_end(xbt, 1);
	xenbus_dev_fatal(sc->sc_xbusd, error, "%s", errmsg);
	return false;
}

static bool
xennet_xenbus_suspend(device_t dev, const pmf_qual_t *qual)
{
	struct xennet_xenbus_softc *sc = device_private(dev);

	/*
	 * xennet_stop() is called by pmf(9) before xennet_xenbus_suspend(),
	 * so we do not mask event channel here
	 */

	mutex_enter(&sc->sc_tx_lock);

	/* collect any outstanding TX responses */
	xennet_tx_complete(sc);
	while (sc->sc_tx_ring.sring->rsp_prod != sc->sc_tx_ring.rsp_cons) {
		kpause("xnsuspend", true, hz/2, &sc->sc_tx_lock);
		xennet_tx_complete(sc);
	}
	KASSERT(sc->sc_free_txreql == NET_RX_RING_SIZE);
	mutex_exit(&sc->sc_tx_lock);

	/*
	 * dom0 may still use references to the grants we gave away
	 * earlier during RX buffers allocation. So we do not free RX buffers
	 * here, as dom0 does not expect the guest domain to suddenly revoke
	 * access to these grants.
	 */
	sc->sc_backend_status = BEST_SUSPENDED;

	xenbus_device_suspend(sc->sc_xbusd);
	aprint_verbose_dev(dev, "removed event channel %d\n", sc->sc_evtchn);

	return true;
}

static void xennet_backend_changed(void *arg, XenbusState new_state)
{
	struct xennet_xenbus_softc *sc = device_private((device_t)arg);
	DPRINTF(("%s: new backend state %d\n",
	    device_xname(sc->sc_dev), new_state));

	switch (new_state) {
	case XenbusStateInitialising:
	case XenbusStateInitialised:
	case XenbusStateConnected:
		break;
	case XenbusStateClosing:
		sc->sc_backend_status = BEST_CLOSED;
		xenbus_switch_state(sc->sc_xbusd, NULL, XenbusStateClosed);
		break;
	case XenbusStateInitWait:
		if (sc->sc_backend_status == BEST_CONNECTED
		   || sc->sc_backend_status == BEST_SUSPENDED)
			break;

		if (xennet_talk_to_backend(sc))
			xenbus_switch_state(sc->sc_xbusd, NULL,
			    XenbusStateConnected);
		break;
	case XenbusStateUnknown:
	default:
		panic("bad backend state %d", new_state);
	}
}

/*
 * Allocate RX buffers and put the associated request structures
 * in the ring. This allows the backend to use them to communicate with
 * frontend when some data is destined to frontend
 */
static void
xennet_alloc_rx_buffer(struct xennet_xenbus_softc *sc)
{
	RING_IDX req_prod = sc->sc_rx_ring.req_prod_pvt;
	RING_IDX i;
	netif_rx_request_t *rxreq;
	struct xennet_rxreq *req;
	int otherend_id, notify;
	struct mbuf *m;
	vaddr_t va;
	paddr_t pa, ma;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	KASSERT(mutex_owned(&sc->sc_rx_lock));

	otherend_id = sc->sc_xbusd->xbusd_otherend_id;

	for (i = 0; sc->sc_free_rxreql != 0; i++) {
		req  = SLIST_FIRST(&sc->sc_rxreq_head);
		KASSERT(req != NULL);
		KASSERT(req == &sc->sc_rxreqs[req->rxreq_id]);
		KASSERT(req->rxreq_m == NULL);
		KASSERT(req->rxreq_gntref == GRANT_INVALID_REF);

		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (__predict_false(m == NULL)) {
			printf("%s: rx no mbuf\n", ifp->if_xname);
			break;
		}

		va = (vaddr_t)pool_cache_get_paddr(
		    if_xennetrxbuf_cache, PR_NOWAIT, &pa);
		if (__predict_false(va == 0)) {
			printf("%s: rx no cluster\n", ifp->if_xname);
			m_freem(m);
			break;
		}

		MEXTADD(m, va, PAGE_SIZE,
		    M_DEVBUF, xennet_rx_mbuf_free, NULL);
		m->m_len = m->m_pkthdr.len = PAGE_SIZE;
		m->m_ext.ext_paddr = pa;
		m->m_flags |= M_EXT_RW; /* we own the buffer */

		/* Set M_EXT_CLUSTER so that load_mbuf uses m_ext.ext_paddr */
		m->m_flags |= M_EXT_CLUSTER;
		if (__predict_false(bus_dmamap_load_mbuf(
		    sc->sc_xbusd->xbusd_dmat,
		    req->rxreq_dmamap, m, BUS_DMA_NOWAIT) != 0)) {
			printf("%s: rx mbuf load failed", ifp->if_xname);
			m->m_flags &= ~M_EXT_CLUSTER;
			m_freem(m);
			break;
		}
		m->m_flags &= ~M_EXT_CLUSTER;

		KASSERT(req->rxreq_dmamap->dm_nsegs == 1);
		ma = req->rxreq_dmamap->dm_segs[0].ds_addr;

		if (xengnt_grant_access(otherend_id, trunc_page(ma),
		    0, &req->rxreq_gntref) != 0) {
			m_freem(m);
			break;
		}

		req->rxreq_m = m;

		rxreq = RING_GET_REQUEST(&sc->sc_rx_ring, req_prod + i);
		rxreq->id = req->rxreq_id;
		rxreq->gref = req->rxreq_gntref;

		SLIST_REMOVE_HEAD(&sc->sc_rxreq_head, rxreq_next);
		sc->sc_free_rxreql--;
	}

	/* Notify backend if more Rx is possible */
	if (i > 0) {
		sc->sc_rx_ring.req_prod_pvt = req_prod + i;
		RING_PUSH_REQUESTS_AND_CHECK_NOTIFY(&sc->sc_rx_ring, notify);
		if (notify)
			hypervisor_notify_via_evtchn(sc->sc_evtchn);
	}
}

/*
 * Reclaim all RX buffers used by the I/O ring between frontend and backend
 */
static void
xennet_free_rx_buffer(struct xennet_xenbus_softc *sc, bool revoke)
{
	RING_IDX i;

	KASSERT(mutex_owned(&sc->sc_rx_lock));

	DPRINTF(("%s: xennet_free_rx_buffer\n", device_xname(sc->sc_dev)));
	/* get back memory from RX ring */
	for (i = 0; i < NET_RX_RING_SIZE; i++) {
		struct xennet_rxreq *rxreq = &sc->sc_rxreqs[i];

		if (rxreq->rxreq_gntref != GRANT_INVALID_REF) {
			/*
			 * this req is still granted. Get back the page or
			 * allocate a new one, and remap it.
			 */
			SLIST_INSERT_HEAD(&sc->sc_rxreq_head, rxreq,
			    rxreq_next);
			sc->sc_free_rxreql++;

			if (revoke)
				xengnt_revoke_access(rxreq->rxreq_gntref);
			rxreq->rxreq_gntref = GRANT_INVALID_REF;
		}

		if (rxreq->rxreq_m != NULL) {
			m_freem(rxreq->rxreq_m);
			rxreq->rxreq_m = NULL;
		}
	}
	DPRINTF(("%s: xennet_free_rx_buffer done\n", device_xname(sc->sc_dev)));
}

/*
 * Clears a used RX request when its associated mbuf has been processed
 */
static void
xennet_rx_mbuf_free(struct mbuf *m, void *buf, size_t size, void *arg)
{
	KASSERT(buf == m->m_ext.ext_buf);
	KASSERT(arg == NULL);
	KASSERT(m != NULL);
	vaddr_t va = (vaddr_t)(buf) & ~((vaddr_t)PAGE_MASK);
	pool_cache_put_paddr(if_xennetrxbuf_cache,
	    (void *)va, m->m_ext.ext_paddr);
	pool_cache_put(mb_cache, m);
};

static void
xennet_rx_free_req(struct xennet_xenbus_softc *sc, struct xennet_rxreq *req)
{
	KASSERT(mutex_owned(&sc->sc_rx_lock));

	/* puts back the RX request in the list of free RX requests */
	SLIST_INSERT_HEAD(&sc->sc_rxreq_head, req, rxreq_next);
	sc->sc_free_rxreql++;

	/*
	 * ring needs more requests to be pushed in, allocate some
	 * RX buffers to catch-up with backend's consumption
	 */
	if (sc->sc_free_rxreql >= (NET_RX_RING_SIZE * 4 / 5) &&
	    __predict_true(sc->sc_backend_status == BEST_CONNECTED)) {
		xennet_alloc_rx_buffer(sc);
	}
}

/*
 * Process responses associated to the TX mbufs sent previously through
 * xennet_start()
 * Called at splsoftnet.
 */
static void
xennet_tx_complete(struct xennet_xenbus_softc *sc)
{
	struct xennet_txreq *req;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	RING_IDX resp_prod, i;

	DPRINTFN(XEDB_EVENT, ("xennet_tx_complete prod %d cons %d\n",
	    sc->sc_tx_ring.sring->rsp_prod, sc->sc_tx_ring.rsp_cons));

	KASSERT(mutex_owned(&sc->sc_tx_lock));
again:
	resp_prod = sc->sc_tx_ring.sring->rsp_prod;
	xen_rmb();
	for (i = sc->sc_tx_ring.rsp_cons; i != resp_prod; i++) {
		req = &sc->sc_txreqs[RING_GET_RESPONSE(&sc->sc_tx_ring, i)->id];
		KASSERT(req->txreq_id ==
		    RING_GET_RESPONSE(&sc->sc_tx_ring, i)->id);
		KASSERT(xengnt_status(req->txreq_gntref) == 0);
		xengnt_revoke_access(req->txreq_gntref);
		req->txreq_gntref = GRANT_INVALID_REF;

		/* Cleanup/statistics if this is the master req of a chain */
		if (req->txreq_m) {
			if (__predict_false(
			    RING_GET_RESPONSE(&sc->sc_tx_ring, i)->status !=
			    NETIF_RSP_OKAY))
				if_statinc(ifp, if_oerrors);
			else
				if_statinc(ifp, if_opackets);
			bus_dmamap_unload(sc->sc_xbusd->xbusd_dmat,
			    req->txreq_dmamap);
			m_freem(req->txreq_m);
			req->txreq_m = NULL;
		}

		SLIST_INSERT_HEAD(&sc->sc_txreq_head, req, txreq_next);
		sc->sc_free_txreql++;
	}

	sc->sc_tx_ring.rsp_cons = resp_prod;
	/* set new event and check for race with rsp_cons update */
	sc->sc_tx_ring.sring->rsp_event =
	    resp_prod + ((sc->sc_tx_ring.sring->req_prod - resp_prod) >> 1) + 1;
	xen_wmb();
	if (resp_prod != sc->sc_tx_ring.sring->rsp_prod)
		goto again;
}

/*
 * Xennet event handler.
 * Get outstanding responses of TX packets, then collect all responses of
 * pending RX packets
 * Called at splnet.
 */
static int
xennet_handler(void *arg)
{
	struct xennet_xenbus_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	RING_IDX resp_prod, i;
	struct xennet_rxreq *req;
	struct mbuf *m, *m0;
	int rxflags, m0_rxflags;
	int more_to_do;

	if (sc->sc_backend_status != BEST_CONNECTED)
		return 1;

	/* Poke Tx queue if we run out of Tx buffers earlier */
	if_schedule_deferred_start(ifp);

	rnd_add_uint32(&sc->sc_rnd_source, sc->sc_tx_ring.req_prod_pvt);

again:
	DPRINTFN(XEDB_EVENT, ("xennet_handler prod %d cons %d\n",
	    sc->sc_rx_ring.sring->rsp_prod, sc->sc_rx_ring.rsp_cons));

	mutex_enter(&sc->sc_rx_lock);
	resp_prod = sc->sc_rx_ring.sring->rsp_prod;
	xen_rmb(); /* ensure we see replies up to resp_prod */

	m0 = NULL;
	for (i = sc->sc_rx_ring.rsp_cons; i != resp_prod; i++) {
		netif_rx_response_t *rx = RING_GET_RESPONSE(&sc->sc_rx_ring, i);
		req = &sc->sc_rxreqs[rx->id];
		KASSERT(req->rxreq_gntref != GRANT_INVALID_REF);
		KASSERT(req->rxreq_id == rx->id);

		xengnt_revoke_access(req->rxreq_gntref);
		req->rxreq_gntref = GRANT_INVALID_REF;

		m = req->rxreq_m;
		req->rxreq_m = NULL;

		m->m_len = m->m_pkthdr.len = rx->status;
		bus_dmamap_sync(sc->sc_xbusd->xbusd_dmat, req->rxreq_dmamap, 0,
		     m->m_pkthdr.len, BUS_DMASYNC_PREREAD);

		if (m0 == NULL) {
			MCLAIM(m, &sc->sc_ethercom.ec_rx_mowner);
			m_set_rcvif(m, ifp);
		}

		rxflags = rx->flags;

		if (m0 || rxflags & NETRXF_more_data) {
			/*
			 * On Rx, every fragment (even first one) contain
			 * just length of data in the fragment.
			 */
			if (m0 == NULL) {
				m0 = m;
				m0_rxflags = rxflags;
			} else {
				m_cat(m0, m);
				m0->m_pkthdr.len += m->m_len;
			}

			if (rxflags & NETRXF_more_data) {
				/* Still more fragments to receive */
				xennet_rx_free_req(sc, req);
				continue;
			}

			sc->sc_cnt_rx_frag.ev_count++;
			m = m0;
			m0 = NULL;
			rxflags = m0_rxflags;
		}

		if (rxflags & NETRXF_csum_blank) {
			xennet_checksum_fill(ifp, m, &sc->sc_cnt_rx_cksum_blank,
			    &sc->sc_cnt_rx_cksum_undefer);
		} else if (rxflags & NETRXF_data_validated)
			m->m_pkthdr.csum_flags = XN_M_CSUM_SUPPORTED;

		/* We'are done with req */
		xennet_rx_free_req(sc, req);

		/* Pass the packet up. */
		if_percpuq_enqueue(ifp->if_percpuq, m);
	}
	/* If the queued Rx fragments did not finish the packet, drop it */
	if (m0) {
		if_statinc(ifp, if_iqdrops);
		m_freem(m0);
	}
	xen_rmb();
	sc->sc_rx_ring.rsp_cons = i;
	RING_FINAL_CHECK_FOR_RESPONSES(&sc->sc_rx_ring, more_to_do);
	mutex_exit(&sc->sc_rx_lock);

	if (more_to_do) {
		DPRINTF(("%s: %s more_to_do\n", ifp->if_xname, __func__));
		goto again;
	}

	return 1;
}

static bool
xennet_submit_tx_request(struct xennet_xenbus_softc *sc, struct mbuf *m,
    struct xennet_txreq *req0, int *req_prod)
{
	struct xennet_txreq *req = req0;
	netif_tx_request_t *txreq;
	int i, prod = *req_prod;
	const bool multiseg = (req0->txreq_dmamap->dm_nsegs > 1);
	const int lastseg = req0->txreq_dmamap->dm_nsegs - 1;
	bus_dma_segment_t *ds;
	SLIST_HEAD(, xennet_txreq) txchain;

	KASSERT(mutex_owned(&sc->sc_tx_lock));
	KASSERT(req0->txreq_dmamap->dm_nsegs > 0);

	bus_dmamap_sync(sc->sc_xbusd->xbusd_dmat, req->txreq_dmamap, 0,
	     m->m_pkthdr.len, BUS_DMASYNC_POSTWRITE);
	MCLAIM(m, &sc->sc_ethercom.ec_tx_mowner);
	SLIST_INIT(&txchain);

	for (i = 0; i < req0->txreq_dmamap->dm_nsegs; i++) {
		KASSERT(req != NULL);

		ds = &req0->txreq_dmamap->dm_segs[i];

		if (__predict_false(xengnt_grant_access(
		    sc->sc_xbusd->xbusd_otherend_id,
		    trunc_page(ds->ds_addr),
		    GNTMAP_readonly, &req->txreq_gntref) != 0)) {
			goto grant_fail;
		}

		KASSERT(SLIST_FIRST(&sc->sc_txreq_head) == req);
		SLIST_REMOVE_HEAD(&sc->sc_txreq_head, txreq_next);
		SLIST_INSERT_HEAD(&txchain, req, txreq_next);
		sc->sc_free_txreql--;
		req->txreq_m = (req == req0) ? m : NULL;

		txreq = RING_GET_REQUEST(&sc->sc_tx_ring, prod + i);
		txreq->id = req->txreq_id;
		txreq->gref = req->txreq_gntref;
		txreq->offset = ds->ds_addr & PAGE_MASK;
		/* For Tx, first fragment size is always set to total size */
		txreq->size = (i == 0) ? m->m_pkthdr.len : ds->ds_len;
		txreq->flags = 0;
		if (i == 0) {
			if (m->m_pkthdr.csum_flags & XN_M_CSUM_SUPPORTED) {
				txreq->flags |= NETTXF_csum_blank;
			} else {
				txreq->flags |= NETTXF_data_validated;
			}
		}
		if (multiseg && i < lastseg)
			txreq->flags |= NETTXF_more_data;

		req = SLIST_FIRST(&sc->sc_txreq_head);
	}

	if (i > 1)
		sc->sc_cnt_tx_frag.ev_count++;

	/* All done */
	*req_prod += i;
	return true;

grant_fail:
	printf("%s: grant_access failed\n", device_xname(sc->sc_dev));
	while (!SLIST_EMPTY(&txchain)) {
		req = SLIST_FIRST(&txchain);
		SLIST_REMOVE_HEAD(&txchain, txreq_next);
		xengnt_revoke_access(req->txreq_gntref);
		req->txreq_gntref = GRANT_INVALID_REF;
		SLIST_INSERT_HEAD(&sc->sc_txreq_head, req, txreq_next);
		sc->sc_free_txreql++;
	}
	req0->txreq_m = NULL;
	return false;
}

/*
 * The output routine of a xennet interface. Prepares mbufs for TX,
 * and notify backend when finished.
 * Called at splsoftnet.
 */
void
xennet_start(struct ifnet *ifp)
{
	struct xennet_xenbus_softc *sc = ifp->if_softc;
	struct mbuf *m;
	RING_IDX req_prod;
	struct xennet_txreq *req;
	int notify;

	mutex_enter(&sc->sc_tx_lock);

	rnd_add_uint32(&sc->sc_rnd_source, sc->sc_tx_ring.req_prod_pvt);

	xennet_tx_complete(sc);

	req_prod = sc->sc_tx_ring.req_prod_pvt;
	while (/*CONSTCOND*/1) {
		req = SLIST_FIRST(&sc->sc_txreq_head);
		if (__predict_false(req == NULL)) {
			if (!IFQ_IS_EMPTY(&ifp->if_snd))
				sc->sc_cnt_tx_queue_full.ev_count++;
			break;
		}
		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;

		/*
		 * For short packets it's always way faster passing
		 * single defragmented packet, even with feature-sg.
		 * Try to defragment first if the result is likely to fit
		 * into a single mbuf.
		 */
		if (m->m_pkthdr.len < MCLBYTES && m->m_next)
			(void)m_defrag(m, M_DONTWAIT);

		/* Try to load the mbuf as-is, if that fails defrag */
		if (__predict_false(bus_dmamap_load_mbuf(
		    sc->sc_xbusd->xbusd_dmat,
		    req->txreq_dmamap, m, BUS_DMA_NOWAIT) != 0)) {
			sc->sc_cnt_tx_defrag.ev_count++;
			if (__predict_false(m_defrag(m, M_DONTWAIT) == NULL)) {
				DPRINTF(("%s: defrag failed\n",
				    device_xname(sc->sc_dev)));
				m_freem(m);
				break;
			}

			if (__predict_false(bus_dmamap_load_mbuf(
			    sc->sc_xbusd->xbusd_dmat,
			    req->txreq_dmamap, m, BUS_DMA_NOWAIT) != 0)) {
				printf("%s: cannot load new mbuf len %d\n",
				    device_xname(sc->sc_dev),
				    m->m_pkthdr.len);
				m_freem(m);
				break;
			}
		}

		if (req->txreq_dmamap->dm_nsegs > sc->sc_free_txreql) {
			/* Not enough slots right now, postpone */
			sc->sc_cnt_tx_queue_full.ev_count++;
			sc->sc_cnt_tx_drop.ev_count++;
			bus_dmamap_unload(sc->sc_xbusd->xbusd_dmat,
			    req->txreq_dmamap);
			m_freem(m);
			break;
		}

		DPRINTFN(XEDB_MBUF, ("xennet_start id %d, "
		    "mbuf %p, buf %p, size %d\n",
		    req->txreq_id, m, mtod(m, void *), m->m_pkthdr.len));

#ifdef XENNET_DEBUG_DUMP
		xennet_hex_dump(mtod(m, u_char *), m->m_pkthdr.len, "s",
		    req->txreq_id);
#endif

		if (!xennet_submit_tx_request(sc, m, req, &req_prod)) {
			/* Grant failed, postpone */
			sc->sc_cnt_tx_drop.ev_count++;
			bus_dmamap_unload(sc->sc_xbusd->xbusd_dmat,
			    req->txreq_dmamap);
			m_freem(m);
			break;
		}

		/*
		 * Pass packet to bpf if there is a listener.
		 */
		bpf_mtap(ifp, m, BPF_D_OUT);
	}

	sc->sc_tx_ring.req_prod_pvt = req_prod;
	RING_PUSH_REQUESTS_AND_CHECK_NOTIFY(&sc->sc_tx_ring, notify);
	if (notify)
		hypervisor_notify_via_evtchn(sc->sc_evtchn);

	mutex_exit(&sc->sc_tx_lock);

	DPRINTFN(XEDB_FOLLOW, ("%s: xennet_start() done\n",
	    device_xname(sc->sc_dev)));
}

int
xennet_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
#ifdef XENNET_DEBUG
	struct xennet_xenbus_softc *sc = ifp->if_softc;
#endif
	int error = 0;

#ifdef NET_MPSAFE
#ifdef notyet
	/* XXX IFNET_LOCK() is not taken in some cases e.g. multicast ioctls */
	KASSERT(IFNET_LOCKED(ifp));
#endif
#endif
	int s = splnet();

	DPRINTFN(XEDB_FOLLOW, ("%s: xennet_ioctl()\n",
	    device_xname(sc->sc_dev)));
	error = ether_ioctl(ifp, cmd, data);
	if (error == ENETRESET)
		error = 0;

	DPRINTFN(XEDB_FOLLOW, ("%s: xennet_ioctl() returning %d\n",
	    device_xname(sc->sc_dev), error));

	splx(s);

	return error;
}

int
xennet_init(struct ifnet *ifp)
{
	struct xennet_xenbus_softc *sc = ifp->if_softc;

	KASSERT(IFNET_LOCKED(ifp));

	DPRINTFN(XEDB_FOLLOW, ("%s: xennet_init()\n",
	    device_xname(sc->sc_dev)));

	if ((ifp->if_flags & IFF_RUNNING) == 0) {
		mutex_enter(&sc->sc_rx_lock);
		sc->sc_rx_ring.sring->rsp_event =
		    sc->sc_rx_ring.rsp_cons + 1;
		mutex_exit(&sc->sc_rx_lock);
		hypervisor_unmask_event(sc->sc_evtchn);
		hypervisor_notify_via_evtchn(sc->sc_evtchn);
	}
	ifp->if_flags |= IFF_RUNNING;

	return 0;
}

void
xennet_stop(struct ifnet *ifp, int disable)
{
	struct xennet_xenbus_softc *sc = ifp->if_softc;

	KASSERT(IFNET_LOCKED(ifp));

	ifp->if_flags &= ~IFF_RUNNING;
	hypervisor_mask_event(sc->sc_evtchn);
}

#if defined(NFS_BOOT_BOOTSTATIC)
int
xennet_bootstatic_callback(struct nfs_diskless *nd)
{
#if 0
	struct ifnet *ifp = nd->nd_ifp;
	struct xennet_xenbus_softc *sc =
	    (struct xennet_xenbus_softc *)ifp->if_softc;
#endif
	int flags = 0;
	union xen_cmdline_parseinfo xcp;
	struct sockaddr_in *sin;

	memset(&xcp, 0, sizeof(xcp.xcp_netinfo));
	xcp.xcp_netinfo.xi_ifno = /* XXX sc->sc_ifno */ 0;
	xcp.xcp_netinfo.xi_root = nd->nd_root.ndm_host;
	xen_parse_cmdline(XEN_PARSE_NETINFO, &xcp);

	if (xcp.xcp_netinfo.xi_root[0] != '\0') {
		flags |= NFS_BOOT_HAS_SERVER;
		if (strchr(xcp.xcp_netinfo.xi_root, ':') != NULL)
			flags |= NFS_BOOT_HAS_ROOTPATH;
	}

	nd->nd_myip.s_addr = ntohl(xcp.xcp_netinfo.xi_ip[0]);
	nd->nd_gwip.s_addr = ntohl(xcp.xcp_netinfo.xi_ip[2]);
	nd->nd_mask.s_addr = ntohl(xcp.xcp_netinfo.xi_ip[3]);

	sin = (struct sockaddr_in *) &nd->nd_root.ndm_saddr;
	memset((void *)sin, 0, sizeof(*sin));
	sin->sin_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = ntohl(xcp.xcp_netinfo.xi_ip[1]);

	if (nd->nd_myip.s_addr)
		flags |= NFS_BOOT_HAS_MYIP;
	if (nd->nd_gwip.s_addr)
		flags |= NFS_BOOT_HAS_GWIP;
	if (nd->nd_mask.s_addr)
		flags |= NFS_BOOT_HAS_MASK;
	if (sin->sin_addr.s_addr)
		flags |= NFS_BOOT_HAS_SERVADDR;

	return flags;
}
#endif /* defined(NFS_BOOT_BOOTSTATIC) */

#ifdef XENNET_DEBUG_DUMP
#define XCHR(x) hexdigits[(x) & 0xf]
static void
xennet_hex_dump(const unsigned char *pkt, size_t len, const char *type, int id)
{
	size_t i, j;

	printf("pkt %p len %zd/%zx type %s id %d\n", pkt, len, len, type, id);
	printf("00000000  ");
	for(i=0; i<len; i++) {
		printf("%c%c ", XCHR(pkt[i]>>4), XCHR(pkt[i]));
		if ((i+1) % 16 == 8)
			printf(" ");
		if ((i+1) % 16 == 0) {
			printf(" %c", '|');
			for(j=0; j<16; j++)
				printf("%c", pkt[i-15+j]>=32 &&
				    pkt[i-15+j]<127?pkt[i-15+j]:'.');
			printf("%c\n%c%c%c%c%c%c%c%c  ", '|',
			    XCHR((i+1)>>28), XCHR((i+1)>>24),
			    XCHR((i+1)>>20), XCHR((i+1)>>16),
			    XCHR((i+1)>>12), XCHR((i+1)>>8),
			    XCHR((i+1)>>4), XCHR(i+1));
		}
	}
	printf("\n");
}
#undef XCHR
#endif

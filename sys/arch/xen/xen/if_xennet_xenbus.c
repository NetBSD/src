/*      $NetBSD: if_xennet_xenbus.c,v 1.51.2.1 2011/08/21 11:24:10 cherry Exp $      */

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
 *
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
 * xennet_start() (default output routine of xennet) that schedules a softint,
 * xennet_softstart(). xennet_softstart() generates the requests associated
 * to the TX mbufs queued (see altq(9)).
 * The backend's responses are processed by xennet_tx_complete(), called either
 * from:
 * - xennet_start()
 * - xennet_handler(), during an asynchronous event notification from backend
 *   (similar to an IRQ).
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
__KERNEL_RCSID(0, "$NetBSD: if_xennet_xenbus.c,v 1.51.2.1 2011/08/21 11:24:10 cherry Exp $");

#include "opt_xen.h"
#include "opt_nfs_boot.h"
#include "rnd.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/intr.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/bpf.h>
#include <net/bpfdesc.h>

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

#include <xen/hypervisor.h>
#include <xen/evtchn.h>
#include <xen/granttables.h>
#include <xen/xen3-public/io/netif.h>
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
#define GRANT_STACK_REF   -2 /* entry owned by the network stack */

#define NET_TX_RING_SIZE __RING_SIZE((netif_tx_sring_t *)0, PAGE_SIZE)
#define NET_RX_RING_SIZE __RING_SIZE((netif_rx_sring_t *)0, PAGE_SIZE)

struct xennet_txreq {
	SLIST_ENTRY(xennet_txreq) txreq_next;
	uint16_t txreq_id; /* ID passed to backend */
	grant_ref_t txreq_gntref; /* grant ref of this request */
	struct mbuf *txreq_m; /* mbuf being transmitted */
};

struct xennet_rxreq {
	SLIST_ENTRY(xennet_rxreq) rxreq_next;
	uint16_t rxreq_id; /* ID passed to backend */
	grant_ref_t rxreq_gntref; /* grant ref of this request */
/* va/pa for this receive buf. ma will be provided by backend */
	paddr_t rxreq_pa;
	vaddr_t rxreq_va;
	struct xennet_xenbus_softc *rxreq_sc; /* pointer to our interface */
};

struct xennet_xenbus_softc {
	device_t sc_dev;
	struct ethercom sc_ethercom;
	uint8_t sc_enaddr[6];
	struct xenbus_device *sc_xbusd;

	netif_tx_front_ring_t sc_tx_ring;
	netif_rx_front_ring_t sc_rx_ring;

	unsigned int sc_evtchn;
	void *sc_softintr;

	grant_ref_t sc_tx_ring_gntref;
	grant_ref_t sc_rx_ring_gntref;

	struct xennet_txreq sc_txreqs[NET_TX_RING_SIZE];
	struct xennet_rxreq sc_rxreqs[NET_RX_RING_SIZE];
	SLIST_HEAD(,xennet_txreq) sc_txreq_head; /* list of free TX requests */
	SLIST_HEAD(,xennet_rxreq) sc_rxreq_head; /* list of free RX requests */
	int sc_free_rxreql; /* number of free receive request struct */

	int sc_backend_status; /* our status with backend */
#define BEST_CLOSED		0
#define BEST_DISCONNECTED	1
#define BEST_CONNECTED		2
#define BEST_SUSPENDED		3
	unsigned long sc_rx_feature;
#define FEATURE_RX_FLIP		0
#define FEATURE_RX_COPY		1
#if NRND > 0
	rndsource_element_t     sc_rnd_source;
#endif
};
#define SC_NLIVEREQ(sc) ((sc)->sc_rx_ring.req_prod_pvt - \
			    (sc)->sc_rx_ring.sring->rsp_prod)

/* too big to be on stack */
static multicall_entry_t rx_mcl[NET_RX_RING_SIZE+1];
static u_long xennet_pages[NET_RX_RING_SIZE];

static int  xennet_xenbus_match(device_t, cfdata_t, void *);
static void xennet_xenbus_attach(device_t, device_t, void *);
static int  xennet_xenbus_detach(device_t, int);
static void xennet_backend_changed(void *, XenbusState);

static int  xennet_xenbus_resume(void *);
static void xennet_alloc_rx_buffer(struct xennet_xenbus_softc *);
static void xennet_free_rx_buffer(struct xennet_xenbus_softc *);
static void xennet_tx_complete(struct xennet_xenbus_softc *);
static void xennet_rx_mbuf_free(struct mbuf *, void *, size_t, void *);
static int  xennet_handler(void *);
static int  xennet_talk_to_backend(struct xennet_xenbus_softc *);
#ifdef XENNET_DEBUG_DUMP
static void xennet_hex_dump(const unsigned char *, size_t, const char *, int);
#endif

static int  xennet_init(struct ifnet *);
static void xennet_stop(struct ifnet *, int);
static void xennet_reset(struct xennet_xenbus_softc *);
static void xennet_softstart(void *);
static void xennet_start(struct ifnet *);
static int  xennet_ioctl(struct ifnet *, u_long, void *);
static void xennet_watchdog(struct ifnet *);

CFATTACH_DECL_NEW(xennet, sizeof(struct xennet_xenbus_softc),
   xennet_xenbus_match, xennet_xenbus_attach, xennet_xenbus_detach, NULL);

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
	RING_IDX i;
	char *val, *e, *p;
	int s;
	extern int ifqmaxlen; /* XXX */
#ifdef XENNET_DEBUG
	char **dir;
	int dir_n = 0;
	char id_str[20];
#endif

	aprint_normal(": Xen Virtual Network Interface\n");
	sc->sc_dev = self;

#ifdef XENNET_DEBUG
	printf("path: %s\n", xa->xa_xbusd->xbusd_path);
	snprintf(id_str, sizeof(id_str), "%d", xa->xa_id);
	err = xenbus_directory(NULL, "device/vif", id_str, &dir_n, &dir);
	if (err) {
		aprint_error_dev(self, "xenbus_directory err %d\n", err);
	} else {
		printf("%s/\n", xa->xa_xbusd->xbusd_path);
		for (i = 0; i < dir_n; i++) {
			printf("\t/%s", dir[i]);
			err = xenbus_read(NULL, xa->xa_xbusd->xbusd_path,
				          dir[i], NULL, &val);
			if (err) {
				aprint_error_dev(self, "xenbus_read err %d\n",
					         err);
			} else {
				printf(" = %s\n", val);
				free(val, M_DEVBUF);
			}
		}
	}
#endif /* XENNET_DEBUG */
	sc->sc_xbusd = xa->xa_xbusd;
	sc->sc_xbusd->xbusd_otherend_changed = xennet_backend_changed;

	/* initialize free RX and RX request lists */
	SLIST_INIT(&sc->sc_txreq_head);
	for (i = 0; i < NET_TX_RING_SIZE; i++) {
		sc->sc_txreqs[i].txreq_id = i;
		SLIST_INSERT_HEAD(&sc->sc_txreq_head, &sc->sc_txreqs[i],
		    txreq_next);
	}
	SLIST_INIT(&sc->sc_rxreq_head);
	s = splvm();
	for (i = 0; i < NET_RX_RING_SIZE; i++) {
		struct xennet_rxreq *rxreq = &sc->sc_rxreqs[i];
		rxreq->rxreq_id = i;
		rxreq->rxreq_sc = sc;
		rxreq->rxreq_va = uvm_km_alloc(kernel_map,
		    PAGE_SIZE, PAGE_SIZE, UVM_KMF_WIRED | UVM_KMF_ZERO);
		if (rxreq->rxreq_va == 0)
			break;
		if (!pmap_extract(pmap_kernel(), rxreq->rxreq_va,
		    &rxreq->rxreq_pa))
			panic("%s: no pa for mapped va ?", device_xname(self));
		rxreq->rxreq_gntref = GRANT_INVALID_REF;
		SLIST_INSERT_HEAD(&sc->sc_rxreq_head, rxreq, rxreq_next);
	}
	splx(s);
	sc->sc_free_rxreql = i;
	if (sc->sc_free_rxreql == 0) {
		aprint_error_dev(self, "failed to allocate rx memory\n");
		return;
	}

	/* read mac address */
	err = xenbus_read(NULL, xa->xa_xbusd->xbusd_path, "mac", NULL, &val);
	if (err) {
		aprint_error_dev(self, "can't read mac address, err %d\n", err);
		return;
	}
	for (i = 0, p = val; i < 6; i++) {
		sc->sc_enaddr[i] = strtoul(p, &e, 16);
		if ((e[0] == '\0' && i != 5) && e[0] != ':') {
			aprint_error_dev(self,
			    "%s is not a valid mac address\n", val);
			free(val, M_DEVBUF);
			return;
		}
		p = &e[1];
	}
	free(val, M_DEVBUF);
	aprint_normal_dev(self, "MAC address %s\n",
	    ether_sprintf(sc->sc_enaddr));
	/* Initialize ifnet structure and attach interface */
	strlcpy(ifp->if_xname, device_xname(self), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = xennet_start;
	ifp->if_ioctl = xennet_ioctl;
	ifp->if_watchdog = xennet_watchdog;
	ifp->if_init = xennet_init;
	ifp->if_stop = xennet_stop;
	ifp->if_flags = IFF_BROADCAST|IFF_SIMPLEX|IFF_NOTRAILERS|IFF_MULTICAST;
	ifp->if_timer = 0;
	ifp->if_snd.ifq_maxlen = max(ifqmaxlen, NET_TX_RING_SIZE * 2);
	ifp->if_capabilities = IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_UDPv4_Tx;
	IFQ_SET_READY(&ifp->if_snd);
	if_attach(ifp);
	ether_ifattach(ifp, sc->sc_enaddr);
	sc->sc_softintr = softint_establish(SOFTINT_NET, xennet_softstart, sc);
	if (sc->sc_softintr == NULL)
		panic("%s: can't establish soft interrupt",
			device_xname(self));

#if NRND > 0
	rnd_attach_source(&sc->sc_rnd_source, device_xname(sc->sc_dev),
	    RND_TYPE_NET, 0);
#endif

	/* resume shared structures and tell backend that we are ready */
	xennet_xenbus_resume(sc);
}

static int
xennet_xenbus_detach(device_t self, int flags)
{
	struct xennet_xenbus_softc *sc = device_private(self);
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int s0, s1;
	RING_IDX i;

	DPRINTF(("%s: xennet_xenbus_detach\n", device_xname(self)));
	s0 = splnet();
	xennet_stop(ifp, 1);
	/* wait for pending TX to complete, and collect pending RX packets */
	xennet_handler(sc);
	while (sc->sc_tx_ring.sring->rsp_prod != sc->sc_tx_ring.rsp_cons) {
		tsleep(xennet_xenbus_detach, PRIBIO, "xnet_detach", hz/2);
		xennet_handler(sc);
	}
	xennet_free_rx_buffer(sc);

	s1 = splvm();
	for (i = 0; i < NET_RX_RING_SIZE; i++) {
		struct xennet_rxreq *rxreq = &sc->sc_rxreqs[i];
		uvm_km_free(kernel_map, rxreq->rxreq_va, PAGE_SIZE,
		    UVM_KMF_WIRED);
	}
	splx(s1);
		
	ether_ifdetach(ifp);
	if_detach(ifp);

#if NRND > 0
	/* Unhook the entropy source. */
	rnd_detach_source(&sc->sc_rnd_source);
#endif

	while (xengnt_status(sc->sc_tx_ring_gntref)) {
		tsleep(xennet_xenbus_detach, PRIBIO, "xnet_txref", hz/2);
	}
	xengnt_revoke_access(sc->sc_tx_ring_gntref);
	uvm_km_free(kernel_map, (vaddr_t)sc->sc_tx_ring.sring, PAGE_SIZE,
	    UVM_KMF_WIRED);
	while (xengnt_status(sc->sc_rx_ring_gntref)) {
		tsleep(xennet_xenbus_detach, PRIBIO, "xnet_rxref", hz/2);
	}
	xengnt_revoke_access(sc->sc_rx_ring_gntref);
	uvm_km_free(kernel_map, (vaddr_t)sc->sc_rx_ring.sring, PAGE_SIZE,
	    UVM_KMF_WIRED);
	softint_disestablish(sc->sc_softintr);
	event_remove_handler(sc->sc_evtchn, &xennet_handler, sc);
	splx(s0);
	DPRINTF(("%s: xennet_xenbus_detach done\n", device_xname(self)));
	return 0;
}

static int
xennet_xenbus_resume(void *p)
{
	struct xennet_xenbus_softc *sc = p;
	int error;
	netif_tx_sring_t *tx_ring;
	netif_rx_sring_t *rx_ring;
	paddr_t ma;

	sc->sc_tx_ring_gntref = GRANT_INVALID_REF;
	sc->sc_rx_ring_gntref = GRANT_INVALID_REF;

	/* setup device: alloc event channel and shared rings */
	tx_ring = (void *)uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
	     UVM_KMF_WIRED | UVM_KMF_ZERO);
	rx_ring = (void *)uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
	    UVM_KMF_WIRED | UVM_KMF_ZERO);
	if (tx_ring == NULL || rx_ring == NULL)
		panic("xennet_xenbus_resume: can't alloc rings");

	SHARED_RING_INIT(tx_ring);
	FRONT_RING_INIT(&sc->sc_tx_ring, tx_ring, PAGE_SIZE);
	SHARED_RING_INIT(rx_ring);
	FRONT_RING_INIT(&sc->sc_rx_ring, rx_ring, PAGE_SIZE);

	(void)pmap_extract_ma(pmap_kernel(), (vaddr_t)tx_ring, &ma);
	error = xenbus_grant_ring(sc->sc_xbusd, ma, &sc->sc_tx_ring_gntref);
	if (error)
		return error;
	(void)pmap_extract_ma(pmap_kernel(), (vaddr_t)rx_ring, &ma);
	error = xenbus_grant_ring(sc->sc_xbusd, ma, &sc->sc_rx_ring_gntref);
	if (error)
		return error;
	error = xenbus_alloc_evtchn(sc->sc_xbusd, &sc->sc_evtchn);
	if (error)
		return error;
	aprint_verbose_dev(sc->sc_dev, "using event channel %d\n",
	    sc->sc_evtchn);
	event_set_handler(sc->sc_evtchn, &xennet_handler, sc,
	    IPL_NET, device_xname(sc->sc_dev));

	return 0;
}

static int
xennet_talk_to_backend(struct xennet_xenbus_softc *sc)
{
	int error;
	unsigned long rx_copy;
	struct xenbus_transaction *xbt;
	const char *errmsg;

	error = xenbus_read_ul(NULL, sc->sc_xbusd->xbusd_otherend,
	    "feature-rx-copy", &rx_copy, 10);
	if (error)
		rx_copy = 0; /* default value if key is absent */

	if (rx_copy == 1) {
		aprint_normal_dev(sc->sc_dev, "using RX copy mode\n");
		sc->sc_rx_feature = FEATURE_RX_COPY;
	} else {
		aprint_normal_dev(sc->sc_dev, "using RX flip mode\n");
		sc->sc_rx_feature = FEATURE_RX_FLIP;
	}

again:
	xbt = xenbus_transaction_start();
	if (xbt == NULL)
		return ENOMEM;
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
	    "request-rx-copy", "%lu", rx_copy);
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
		return -1;
	}
	xennet_alloc_rx_buffer(sc);
	sc->sc_backend_status = BEST_CONNECTED;
	return 0;

abort_transaction:
	xenbus_transaction_end(xbt, 1);
	xenbus_dev_fatal(sc->sc_xbusd, error, "%s", errmsg);
	return error;
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
		if (sc->sc_backend_status == BEST_CONNECTED)
			break;
		if (xennet_talk_to_backend(sc) == 0)
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
	struct xennet_rxreq *req;
	struct xen_memory_reservation reservation;
	int s1, s2, otherend_id;
	paddr_t pfn;

	otherend_id = sc->sc_xbusd->xbusd_otherend_id;

	s1 = splnet();
	for (i = 0; sc->sc_free_rxreql != 0; i++) {
		req  = SLIST_FIRST(&sc->sc_rxreq_head);
		KASSERT(req != NULL);
		KASSERT(req == &sc->sc_rxreqs[req->rxreq_id]);
		RING_GET_REQUEST(&sc->sc_rx_ring, req_prod + i)->id =
		    req->rxreq_id;

		switch (sc->sc_rx_feature) {
		case FEATURE_RX_COPY:
			if (xengnt_grant_access(otherend_id,
			    xpmap_ptom_masked(req->rxreq_pa),
			    0, &req->rxreq_gntref) != 0) {
				goto out_loop;
			}
			break;
		case FEATURE_RX_FLIP:
			if (xengnt_grant_transfer(otherend_id,
			    &req->rxreq_gntref) != 0) {
				goto out_loop;
			}
			break;
		default:
			panic("%s: unsupported RX feature mode: %ld\n",
			    __func__, sc->sc_rx_feature);
		}

		RING_GET_REQUEST(&sc->sc_rx_ring, req_prod + i)->gref =
		    req->rxreq_gntref;

		SLIST_REMOVE_HEAD(&sc->sc_rxreq_head, rxreq_next);
		sc->sc_free_rxreql--;

		if (sc->sc_rx_feature == FEATURE_RX_FLIP) {
			/* unmap the page */
			MULTI_update_va_mapping(&rx_mcl[i],
			    req->rxreq_va, 0, 0);
			/*
			 * Remove this page from pseudo phys map before
			 * passing back to Xen.
			 */
			pfn = (req->rxreq_pa - XPMAP_OFFSET) >> PAGE_SHIFT;
			xennet_pages[i] = xpmap_phys_to_machine_mapping[pfn];
			xpmap_phys_to_machine_mapping[pfn] = INVALID_P2M_ENTRY;
		}
	}

out_loop:
	if (i == 0) {
		splx(s1);
		return;
	}

	if (sc->sc_rx_feature == FEATURE_RX_FLIP) {
		/* also make sure to flush all TLB entries */
		rx_mcl[i-1].args[MULTI_UVMFLAGS_INDEX] =
		    UVMF_TLB_FLUSH | UVMF_ALL;
		/*
		 * We may have allocated buffers which have entries
		 * outstanding in the page update queue -- make sure we flush
		 * those first!
		 */
		s2 = splvm();
		xpq_queue_lock();
		xpq_flush_queue();
		xpq_queue_unlock();		
		splx(s2);
		/* now decrease reservation */
		xenguest_handle(reservation.extent_start) = xennet_pages;
		reservation.nr_extents = i;
		reservation.extent_order = 0;
		reservation.address_bits = 0;
		reservation.domid = DOMID_SELF;
		rx_mcl[i].op = __HYPERVISOR_memory_op;
		rx_mcl[i].args[0] = XENMEM_decrease_reservation;
		rx_mcl[i].args[1] = (unsigned long)&reservation;
		HYPERVISOR_multicall(rx_mcl, i+1);
		if (__predict_false(rx_mcl[i].result != i)) {
			panic("xennet_alloc_rx_buffer: "
			    "XENMEM_decrease_reservation");
		}
	}

	sc->sc_rx_ring.req_prod_pvt = req_prod + i;
	RING_PUSH_REQUESTS(&sc->sc_rx_ring);

	splx(s1);
	return;
}

/*
 * Reclaim all RX buffers used by the I/O ring between frontend and backend
 */
static void
xennet_free_rx_buffer(struct xennet_xenbus_softc *sc)
{
	paddr_t ma, pa;
	vaddr_t va;
	RING_IDX i;
	mmu_update_t mmu[1];
	multicall_entry_t mcl[2];

	int s = splbio();
	
	DPRINTF(("%s: xennet_free_rx_buffer\n", device_xname(sc->sc_dev)));
	/* get back memory from RX ring */
	for (i = 0; i < NET_RX_RING_SIZE; i++) {
		struct xennet_rxreq *rxreq = &sc->sc_rxreqs[i];

		/*
		 * if the buffer is in transit in the network stack, wait for
		 * the network stack to free it.
		 */
		while ((volatile grant_ref_t)rxreq->rxreq_gntref ==
		    GRANT_STACK_REF)
			tsleep(xennet_xenbus_detach, PRIBIO, "xnet_free", hz/2);

		if (rxreq->rxreq_gntref != GRANT_INVALID_REF) {
			/*
			 * this req is still granted. Get back the page or
			 * allocate a new one, and remap it.
			 */
			SLIST_INSERT_HEAD(&sc->sc_rxreq_head, rxreq,
			    rxreq_next);
			sc->sc_free_rxreql++;

			switch (sc->sc_rx_feature) {
			case FEATURE_RX_COPY:
				xengnt_revoke_access(rxreq->rxreq_gntref);
				rxreq->rxreq_gntref = GRANT_INVALID_REF;
				break;
			case FEATURE_RX_FLIP:
				ma = xengnt_revoke_transfer(
				    rxreq->rxreq_gntref);
				rxreq->rxreq_gntref = GRANT_INVALID_REF;
				if (ma == 0) {
					u_long pfn;
					struct xen_memory_reservation xenres;
					/*
					 * transfer not complete, we lost the page.
					 * Get one from hypervisor
					 */
					xenguest_handle(xenres.extent_start) = &pfn;
					xenres.nr_extents = 1;
					xenres.extent_order = 0;
					xenres.address_bits = 31;
					xenres.domid = DOMID_SELF;
					if (HYPERVISOR_memory_op(
					    XENMEM_increase_reservation, &xenres) < 0) {
						panic("xennet_free_rx_buffer: "
						    "can't get memory back");
					}
					ma = pfn;
					KASSERT(ma != 0);
				}
				pa = rxreq->rxreq_pa;
				va = rxreq->rxreq_va;
				/* remap the page */
				mmu[0].ptr = (ma << PAGE_SHIFT) | MMU_MACHPHYS_UPDATE;
				mmu[0].val = ((pa - XPMAP_OFFSET) >> PAGE_SHIFT);
				MULTI_update_va_mapping(&mcl[0], va, 
				    (ma << PAGE_SHIFT) | PG_V | PG_KW,
				    UVMF_TLB_FLUSH|UVMF_ALL);
				xpmap_phys_to_machine_mapping[
				    (pa - XPMAP_OFFSET) >> PAGE_SHIFT] = ma;
				mcl[1].op = __HYPERVISOR_mmu_update;
				mcl[1].args[0] = (unsigned long)mmu;
				mcl[1].args[1] = 1;
				mcl[1].args[2] = 0;
				mcl[1].args[3] = DOMID_SELF;
				HYPERVISOR_multicall(mcl, 2);
				break;
			default:
				panic("%s: unsupported RX feature mode: %ld\n",
				    __func__, sc->sc_rx_feature);
			}
		}

	}
	splx(s);
	DPRINTF(("%s: xennet_free_rx_buffer done\n", device_xname(sc->sc_dev)));
}

/*
 * Clears a used RX request when its associated mbuf has been processed
 */
static void
xennet_rx_mbuf_free(struct mbuf *m, void *buf, size_t size, void *arg)
{
	struct xennet_rxreq *req = arg;
	struct xennet_xenbus_softc *sc = req->rxreq_sc;

	int s = splnet();

	/* puts back the RX request in the list of free RX requests */
	SLIST_INSERT_HEAD(&sc->sc_rxreq_head, req, rxreq_next);
	sc->sc_free_rxreql++;

	/*
	 * ring needs more requests to be pushed in, allocate some
	 * RX buffers to catch-up with backend's consumption
	 */
	req->rxreq_gntref = GRANT_INVALID_REF;
	if (sc->sc_free_rxreql >= SC_NLIVEREQ(sc) &&
	    __predict_true(sc->sc_backend_status == BEST_CONNECTED)) {
		xennet_alloc_rx_buffer(sc);
	}

	if (m)
		pool_cache_put(mb_cache, m);
	splx(s);
}

/*
 * Process responses associated to the TX mbufs sent previously through
 * xennet_softstart()
 * Called at splnet.
 */
static void
xennet_tx_complete(struct xennet_xenbus_softc *sc)
{
	struct xennet_txreq *req;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	RING_IDX resp_prod, i;

	DPRINTFN(XEDB_EVENT, ("xennet_tx_complete prod %d cons %d\n",
	    sc->sc_tx_ring.sring->rsp_prod, sc->sc_tx_ring.rsp_cons));

again:
	resp_prod = sc->sc_tx_ring.sring->rsp_prod;
	xen_rmb();
	for (i = sc->sc_tx_ring.rsp_cons; i != resp_prod; i++) {
		req = &sc->sc_txreqs[RING_GET_RESPONSE(&sc->sc_tx_ring, i)->id];
		KASSERT(req->txreq_id ==
		    RING_GET_RESPONSE(&sc->sc_tx_ring, i)->id);
		if (__predict_false(xengnt_status(req->txreq_gntref))) {
			aprint_verbose_dev(sc->sc_dev,
					   "grant still used by backend\n");
			sc->sc_tx_ring.rsp_cons = i;
			goto end;
		}
		if (__predict_false(
		    RING_GET_RESPONSE(&sc->sc_tx_ring, i)->status !=
		    NETIF_RSP_OKAY))
			ifp->if_oerrors++;
		else
			ifp->if_opackets++;
		xengnt_revoke_access(req->txreq_gntref);
		m_freem(req->txreq_m);
		SLIST_INSERT_HEAD(&sc->sc_txreq_head, req, txreq_next);
	}
	sc->sc_tx_ring.rsp_cons = resp_prod;
	/* set new event and check for race with rsp_cons update */
	sc->sc_tx_ring.sring->rsp_event = 
	    resp_prod + ((sc->sc_tx_ring.sring->req_prod - resp_prod) >> 1) + 1;
	ifp->if_timer = 0;
	xen_wmb();
	if (resp_prod != sc->sc_tx_ring.sring->rsp_prod)
		goto again;
end:
	if (ifp->if_flags & IFF_OACTIVE) {
		ifp->if_flags &= ~IFF_OACTIVE;
		xennet_softstart(sc);
	}
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
	paddr_t ma, pa;
	vaddr_t va;
	mmu_update_t mmu[1];
	multicall_entry_t mcl[2];
	struct mbuf *m;
	void *pktp;
	int more_to_do;

	if (sc->sc_backend_status != BEST_CONNECTED)
		return 1;

	xennet_tx_complete(sc);

#if NRND > 0
	rnd_add_uint32(&sc->sc_rnd_source, sc->sc_tx_ring.req_prod_pvt);
#endif
again:
	DPRINTFN(XEDB_EVENT, ("xennet_handler prod %d cons %d\n",
	    sc->sc_rx_ring.sring->rsp_prod, sc->sc_rx_ring.rsp_cons));

	resp_prod = sc->sc_rx_ring.sring->rsp_prod;
	xen_rmb(); /* ensure we see replies up to resp_prod */
	for (i = sc->sc_rx_ring.rsp_cons; i != resp_prod; i++) {
		netif_rx_response_t *rx = RING_GET_RESPONSE(&sc->sc_rx_ring, i);
		req = &sc->sc_rxreqs[rx->id];
		KASSERT(req->rxreq_gntref != GRANT_INVALID_REF);
		KASSERT(req->rxreq_id == rx->id);

		ma = 0;
		switch (sc->sc_rx_feature) {
		case FEATURE_RX_COPY:
			xengnt_revoke_access(req->rxreq_gntref);
			break;
		case FEATURE_RX_FLIP:
			ma = xengnt_revoke_transfer(req->rxreq_gntref);
			if (ma == 0) {
				DPRINTFN(XEDB_EVENT, ("xennet_handler ma == 0\n"));
				/*
				 * the remote could't send us a packet.
				 * we can't free this rxreq as no page will be mapped
				 * here. Instead give it back immediatly to backend.
				 */
				ifp->if_ierrors++;
				RING_GET_REQUEST(&sc->sc_rx_ring,
				    sc->sc_rx_ring.req_prod_pvt)->id = req->rxreq_id;
				RING_GET_REQUEST(&sc->sc_rx_ring,
				    sc->sc_rx_ring.req_prod_pvt)->gref =
					req->rxreq_gntref;
				sc->sc_rx_ring.req_prod_pvt++;
				RING_PUSH_REQUESTS(&sc->sc_rx_ring);
				continue;
			}
			break;
		default:
			panic("%s: unsupported RX feature mode: %ld\n",
			    __func__, sc->sc_rx_feature);
		}

		req->rxreq_gntref = GRANT_INVALID_REF;

		pa = req->rxreq_pa;
		va = req->rxreq_va;

		if (sc->sc_rx_feature == FEATURE_RX_FLIP) {
			/* remap the page */
			mmu[0].ptr = (ma << PAGE_SHIFT) | MMU_MACHPHYS_UPDATE;
			mmu[0].val = ((pa - XPMAP_OFFSET) >> PAGE_SHIFT);
			MULTI_update_va_mapping(&mcl[0], va, 
			    (ma << PAGE_SHIFT) | PG_V | PG_KW, UVMF_TLB_FLUSH|UVMF_ALL);
			xpmap_phys_to_machine_mapping[
			    (pa - XPMAP_OFFSET) >> PAGE_SHIFT] = ma;
			mcl[1].op = __HYPERVISOR_mmu_update;
			mcl[1].args[0] = (unsigned long)mmu;
			mcl[1].args[1] = 1;
			mcl[1].args[2] = 0;
			mcl[1].args[3] = DOMID_SELF;
			HYPERVISOR_multicall(mcl, 2);
		}

		pktp = (void *)(va + rx->offset);
#ifdef XENNET_DEBUG_DUMP
		xennet_hex_dump(pktp, rx->status, "r", rx->id);
#endif
		if ((ifp->if_flags & IFF_PROMISC) == 0) {
			struct ether_header *eh = pktp;
			if (ETHER_IS_MULTICAST(eh->ether_dhost) == 0 &&
			    memcmp(CLLADDR(ifp->if_sadl), eh->ether_dhost,
			    ETHER_ADDR_LEN) != 0) {
				DPRINTFN(XEDB_EVENT,
				    ("xennet_handler bad dest\n"));
				/* packet not for us */
				xennet_rx_mbuf_free(NULL, (void *)va, PAGE_SIZE,
				    req);
				continue;
			}
		}
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (__predict_false(m == NULL)) {
			printf("%s: rx no mbuf\n", ifp->if_xname);
			ifp->if_ierrors++;
			xennet_rx_mbuf_free(NULL, (void *)va, PAGE_SIZE, req);
			continue;
		}
		MCLAIM(m, &sc->sc_ethercom.ec_rx_mowner);

		m->m_pkthdr.rcvif = ifp;
		if (__predict_true(sc->sc_rx_ring.req_prod_pvt != 
		    sc->sc_rx_ring.sring->rsp_prod)) {
			m->m_len = m->m_pkthdr.len = rx->status;
			MEXTADD(m, pktp, rx->status,
			    M_DEVBUF, xennet_rx_mbuf_free, req);
			m->m_flags |= M_EXT_RW; /* we own the buffer */
			req->rxreq_gntref = GRANT_STACK_REF;
		} else {
			/*
			 * This was our last receive buffer, allocate
			 * memory, copy data and push the receive
			 * buffer back to the hypervisor.
			 */
			m->m_len = min(MHLEN, rx->status);
			m->m_pkthdr.len = 0;
			m_copyback(m, 0, rx->status, pktp);
			xennet_rx_mbuf_free(NULL, (void *)va, PAGE_SIZE, req);
			if (m->m_pkthdr.len < rx->status) {
				/* out of memory, just drop packets */
				ifp->if_ierrors++;
				m_freem(m);
				continue;
			}
		}
		if ((rx->flags & NETRXF_csum_blank) != 0) {
			xennet_checksum_fill(&m);
			if (m == NULL) {
				ifp->if_ierrors++;
				continue;
			}
		}
		/*
		 * Pass packet to bpf if there is a listener.
		 */
		bpf_mtap(ifp, m);

		ifp->if_ipackets++;

		/* Pass the packet up. */
		(*ifp->if_input)(ifp, m);
	}
	xen_rmb();
	sc->sc_rx_ring.rsp_cons = i;
	RING_FINAL_CHECK_FOR_RESPONSES(&sc->sc_rx_ring, more_to_do);
	if (more_to_do)
		goto again;
	return 1;
}

/* 
 * The output routine of a xennet interface
 * Called at splnet.
 */
void
xennet_start(struct ifnet *ifp)
{
	struct xennet_xenbus_softc *sc = ifp->if_softc;

	DPRINTFN(XEDB_FOLLOW, ("%s: xennet_start()\n", device_xname(sc->sc_dev)));

#if NRND > 0
	rnd_add_uint32(&sc->sc_rnd_source, sc->sc_tx_ring.req_prod_pvt);
#endif

	xennet_tx_complete(sc);

	if (__predict_false(
	    (ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING))
		return;

	/*
	 * The Xen communication channel is much more efficient if we can
	 * schedule batch of packets for domain0. To achieve this, we
	 * schedule a soft interrupt, and just return. This way, the network
	 * stack will enqueue all pending mbufs in the interface's send queue
	 * before it is processed by xennet_softstart().
	 */
	softint_schedule(sc->sc_softintr);
	return;
}

/*
 * Prepares mbufs for TX, and notify backend when finished
 * Called at splsoftnet
 */
void
xennet_softstart(void *arg)
{
	struct xennet_xenbus_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *m, *new_m;
	netif_tx_request_t *txreq;
	RING_IDX req_prod;
	paddr_t pa, pa2;
	struct xennet_txreq *req;
	int notify;
	int do_notify = 0;
	int s;

	s = splnet();
	if (__predict_false(
	    (ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)) {
		splx(s);
		return;
	}

	req_prod = sc->sc_tx_ring.req_prod_pvt;
	while (/*CONSTCOND*/1) {
		uint16_t txflags;

		req = SLIST_FIRST(&sc->sc_txreq_head);
		if (__predict_false(req == NULL)) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}
		IFQ_POLL(&ifp->if_snd, m);
		if (m == NULL)
			break;

		switch (m->m_flags & (M_EXT|M_EXT_CLUSTER)) {
		case M_EXT|M_EXT_CLUSTER:
			KASSERT(m->m_ext.ext_paddr != M_PADDR_INVALID);
			pa = m->m_ext.ext_paddr +
				(m->m_data - m->m_ext.ext_buf);
			break;
		case 0:
			KASSERT(m->m_paddr != M_PADDR_INVALID);
			pa = m->m_paddr + M_BUFOFFSET(m) +
				(m->m_data - M_BUFADDR(m));
			break;
		default:
			if (__predict_false(
			    !pmap_extract(pmap_kernel(), (vaddr_t)m->m_data,
			    &pa))) {
				panic("xennet_start: no pa");
			}
			break;
		}

		if ((m->m_pkthdr.csum_flags &
		    (M_CSUM_TCPv4 | M_CSUM_UDPv4)) != 0) {
			txflags = NETTXF_csum_blank;
		} else {
			txflags = 0;
		}

		if (m->m_pkthdr.len != m->m_len ||
		    (pa ^ (pa + m->m_pkthdr.len - 1)) & PG_FRAME) {

			MGETHDR(new_m, M_DONTWAIT, MT_DATA);
			if (__predict_false(new_m == NULL)) {
				printf("%s: cannot allocate new mbuf\n",
				       device_xname(sc->sc_dev));
				break;
			}
			if (m->m_pkthdr.len > MHLEN) {
				MCLGET(new_m, M_DONTWAIT);
				if (__predict_false(
				    (new_m->m_flags & M_EXT) == 0)) {
					DPRINTF(("%s: no mbuf cluster\n",
					    device_xname(sc->sc_dev)));
					m_freem(new_m);
					break;
				}
			}

			m_copydata(m, 0, m->m_pkthdr.len, mtod(new_m, void *));
			new_m->m_len = new_m->m_pkthdr.len = m->m_pkthdr.len;

			if ((new_m->m_flags & M_EXT) != 0) {
				pa = new_m->m_ext.ext_paddr;
				KASSERT(new_m->m_data == new_m->m_ext.ext_buf);
				KASSERT(pa != M_PADDR_INVALID);
			} else {
				pa = new_m->m_paddr;
				KASSERT(pa != M_PADDR_INVALID);
				KASSERT(new_m->m_data == M_BUFADDR(new_m));
				pa += M_BUFOFFSET(new_m);
			}
			if (__predict_false(xengnt_grant_access(
			    sc->sc_xbusd->xbusd_otherend_id,
			    xpmap_ptom_masked(pa),
			    GNTMAP_readonly, &req->txreq_gntref) != 0)) {
				m_freem(new_m);
				ifp->if_flags |= IFF_OACTIVE;
				break;
			}
			/* we will be able to send new_m */
			IFQ_DEQUEUE(&ifp->if_snd, m);
			m_freem(m);
			m = new_m;
		} else {
			if (__predict_false(xengnt_grant_access(
			    sc->sc_xbusd->xbusd_otherend_id,
			    xpmap_ptom_masked(pa),
			    GNTMAP_readonly, &req->txreq_gntref) != 0)) {
				ifp->if_flags |= IFF_OACTIVE;
				break;
			}
			/* we will be able to send m */
			IFQ_DEQUEUE(&ifp->if_snd, m);
		}
		MCLAIM(m, &sc->sc_ethercom.ec_tx_mowner);

		KASSERT(((pa ^ (pa + m->m_pkthdr.len -  1)) & PG_FRAME) == 0);

		SLIST_REMOVE_HEAD(&sc->sc_txreq_head, txreq_next);
		req->txreq_m = m;

		DPRINTFN(XEDB_MBUF, ("xennet_start id %d, "
		    "mbuf %p, buf %p/%p/%p, size %d\n",
		    req->txreq_id, m, mtod(m, void *), (void *)pa,
		    (void *)xpmap_ptom_masked(pa), m->m_pkthdr.len));
		pmap_extract_ma(pmap_kernel(), mtod(m, vaddr_t), &pa2);
		DPRINTFN(XEDB_MBUF, ("xennet_start pa %p ma %p/%p\n",
		    (void *)pa, (void *)xpmap_ptom_masked(pa), (void *)pa2));
#ifdef XENNET_DEBUG_DUMP
		xennet_hex_dump(mtod(m, u_char *), m->m_pkthdr.len, "s",
			       	req->txreq_id);
#endif

		txreq = RING_GET_REQUEST(&sc->sc_tx_ring, req_prod);
		txreq->id = req->txreq_id;
		txreq->gref = req->txreq_gntref;
		txreq->offset = pa & ~PG_FRAME;
		txreq->size = m->m_pkthdr.len;
		txreq->flags = txflags;

		req_prod++;
		sc->sc_tx_ring.req_prod_pvt = req_prod;
		RING_PUSH_REQUESTS_AND_CHECK_NOTIFY(&sc->sc_tx_ring, notify);
		if (notify)
			do_notify = 1;

#ifdef XENNET_DEBUG
		DPRINTFN(XEDB_MEM, ("packet addr %p/%p, physical %p/%p, "
		    "m_paddr %p, len %d/%d\n", M_BUFADDR(m), mtod(m, void *),
		    (void *)*kvtopte(mtod(m, vaddr_t)),
		    (void *)xpmap_mtop(*kvtopte(mtod(m, vaddr_t))),
		    (void *)m->m_paddr, m->m_pkthdr.len, m->m_len));
		DPRINTFN(XEDB_MEM, ("id %d gref %d offset %d size %d flags %d"
		    " prod %d\n",
		    txreq->id, txreq->gref, txreq->offset, txreq->size,
		    txreq->flags, req_prod));
#endif

		/*
		 * Pass packet to bpf if there is a listener.
		 */
		bpf_mtap(ifp, m);
	}

	if (do_notify) {
		hypervisor_notify_via_evtchn(sc->sc_evtchn);
		ifp->if_timer = 5;
	}
	splx(s);

	DPRINTFN(XEDB_FOLLOW, ("%s: xennet_start() done\n",
	    device_xname(sc->sc_dev)));
}

int
xennet_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
#ifdef XENNET_DEBUG
	struct xennet_xenbus_softc *sc = ifp->if_softc;
#endif
	int s, error = 0;

	s = splnet();

	DPRINTFN(XEDB_FOLLOW, ("%s: xennet_ioctl()\n",
	    device_xname(sc->sc_dev)));
	error = ether_ioctl(ifp, cmd, data);
	if (error == ENETRESET)
		error = 0;
	splx(s);

	DPRINTFN(XEDB_FOLLOW, ("%s: xennet_ioctl() returning %d\n",
	    device_xname(sc->sc_dev), error));

	return error;
}

void
xennet_watchdog(struct ifnet *ifp)
{
	aprint_verbose_ifnet(ifp, "xennet_watchdog\n");
}

int
xennet_init(struct ifnet *ifp)
{
	struct xennet_xenbus_softc *sc = ifp->if_softc;
	int s = splnet();

	DPRINTFN(XEDB_FOLLOW, ("%s: xennet_init()\n",
	    device_xname(sc->sc_dev)));

	if ((ifp->if_flags & IFF_RUNNING) == 0) {
		sc->sc_rx_ring.sring->rsp_event =
		    sc->sc_rx_ring.rsp_cons + 1;
		hypervisor_enable_event(sc->sc_evtchn);
		hypervisor_notify_via_evtchn(sc->sc_evtchn);
		xennet_reset(sc);
	}
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	ifp->if_timer = 0;
	splx(s);
	return 0;
}

void
xennet_stop(struct ifnet *ifp, int disable)
{
	struct xennet_xenbus_softc *sc = ifp->if_softc;
	int s = splnet();

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	hypervisor_mask_event(sc->sc_evtchn);
	xennet_reset(sc);
	splx(s);
}

void
xennet_reset(struct xennet_xenbus_softc *sc)
{

	DPRINTFN(XEDB_FOLLOW, ("%s: xennet_reset()\n",
	    device_xname(sc->sc_dev)));
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

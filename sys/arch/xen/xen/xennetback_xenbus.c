/*      $NetBSD: xennetback_xenbus.c,v 1.108.4.3 2023/12/22 13:48:59 martin Exp $      */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xennetback_xenbus.c,v 1.108.4.3 2023/12/22 13:48:59 martin Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/queue.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <net/bpf.h>

#include <net/if_ether.h>

#include <xen/intr.h>
#include <xen/hypervisor.h>
#include <xen/xen.h>
#include <xen/xen_shm.h>
#include <xen/evtchn.h>
#include <xen/xenbus.h>
#include <xen/xennet_checksum.h>

#include <uvm/uvm.h>

/*
 * Backend network device driver for Xen.
 */

#ifdef XENDEBUG_NET
#define XENPRINTF(x) printf x
#else
#define XENPRINTF(x)
#endif

#define NET_TX_RING_SIZE __RING_SIZE((netif_tx_sring_t *)0, PAGE_SIZE)
#define NET_RX_RING_SIZE __RING_SIZE((netif_rx_sring_t *)0, PAGE_SIZE)

/*
 * Number of packets to transmit in one hypercall (= number of pages to
 * transmit at once).
 */
#define NB_XMIT_PAGES_BATCH 64
CTASSERT(NB_XMIT_PAGES_BATCH >= XEN_NETIF_NR_SLOTS_MIN);

/* ratecheck(9) for pool allocation failures */
static const struct timeval xni_pool_errintvl = { 30, 0 };  /* 30s, each */

/* state of a xnetback instance */
typedef enum {
	CONNECTED,
	DISCONNECTING,
	DISCONNECTED
} xnetback_state_t;

struct xnetback_xstate {
	bus_dmamap_t xs_dmamap;
	bool xs_loaded;
	struct mbuf *xs_m;
	struct netif_tx_request xs_tx;
	uint16_t xs_tx_size;		/* Size of data in this Tx fragment */
};

/* we keep the xnetback instances in a linked list */
struct xnetback_instance {
	SLIST_ENTRY(xnetback_instance) next;
	struct xenbus_device *xni_xbusd; /* our xenstore entry */
	domid_t xni_domid;		/* attached to this domain */
	uint32_t xni_handle;	/* domain-specific handle */
	xnetback_state_t xni_status;

	/* network interface stuff */
	struct ethercom xni_ec;
	struct callout xni_restart;
	uint8_t xni_enaddr[ETHER_ADDR_LEN];

	/* remote domain communication stuff */
	unsigned int xni_evtchn; /* our event channel */
	struct intrhand *xni_ih;
	netif_tx_back_ring_t xni_txring;
	netif_rx_back_ring_t xni_rxring;
	grant_handle_t xni_tx_ring_handle; /* to unmap the ring */
	grant_handle_t xni_rx_ring_handle;
	vaddr_t xni_tx_ring_va; /* to unmap the ring */
	vaddr_t xni_rx_ring_va;

	/* arrays used in xennetback_ifstart(), used for both Rx and Tx */
	gnttab_copy_t     	xni_gop_copy[NB_XMIT_PAGES_BATCH];
	struct xnetback_xstate	xni_xstate[NB_XMIT_PAGES_BATCH];

	/* event counters */
	struct evcnt xni_cnt_rx_cksum_blank;
	struct evcnt xni_cnt_rx_cksum_undefer;
};
#define xni_if    xni_ec.ec_if
#define xni_bpf   xni_if.if_bpf

       void xvifattach(int);
static int  xennetback_ifioctl(struct ifnet *, u_long, void *);
static void xennetback_ifstart(struct ifnet *);
static void xennetback_ifsoftstart_copy(struct xnetback_instance *);
static void xennetback_ifwatchdog(struct ifnet *);
static int  xennetback_ifinit(struct ifnet *);
static void xennetback_ifstop(struct ifnet *, int);

static int  xennetback_xenbus_create(struct xenbus_device *);
static int  xennetback_xenbus_destroy(void *);
static void xennetback_frontend_changed(void *, XenbusState);

static inline void xennetback_tx_response(struct xnetback_instance *,
    int, int);

static SLIST_HEAD(, xnetback_instance) xnetback_instances;
static kmutex_t xnetback_lock;

static bool xnetif_lookup(domid_t, uint32_t);
static int  xennetback_evthandler(void *);

static struct xenbus_backend_driver xvif_backend_driver = {
	.xbakd_create = xennetback_xenbus_create,
	.xbakd_type = "vif"
};

void
xvifattach(int n)
{
	XENPRINTF(("xennetback_init\n"));

	SLIST_INIT(&xnetback_instances);
	mutex_init(&xnetback_lock, MUTEX_DEFAULT, IPL_NONE);

	xenbus_backend_register(&xvif_backend_driver);
}

static int
xennetback_xenbus_create(struct xenbus_device *xbusd)
{
	struct xnetback_instance *xneti;
	long domid, handle;
	struct ifnet *ifp;
	extern int ifqmaxlen; /* XXX */
	char *e, *p;
	char mac[32];
	int i, err;
	struct xenbus_transaction *xbt;

	if ((err = xenbus_read_ul(NULL, xbusd->xbusd_path,
	    "frontend-id", &domid, 10)) != 0) {
		aprint_error("xvif: can't read %s/frontend-id: %d\n",
		    xbusd->xbusd_path, err);
		return err;
	}
	if ((err = xenbus_read_ul(NULL, xbusd->xbusd_path,
	    "handle", &handle, 10)) != 0) {
		aprint_error("xvif: can't read %s/handle: %d\n",
		    xbusd->xbusd_path, err);
		return err;
	}

	xneti = kmem_zalloc(sizeof(*xneti), KM_SLEEP);
	xneti->xni_domid = domid;
	xneti->xni_handle = handle;
	xneti->xni_status = DISCONNECTED;

	/* Need to keep the lock for lookup and the list update */
	mutex_enter(&xnetback_lock);
	if (xnetif_lookup(domid, handle)) {
		mutex_exit(&xnetback_lock);
		kmem_free(xneti, sizeof(*xneti));
		return EEXIST;
	}
	SLIST_INSERT_HEAD(&xnetback_instances, xneti, next);
	mutex_exit(&xnetback_lock);

	xbusd->xbusd_u.b.b_cookie = xneti;
	xbusd->xbusd_u.b.b_detach = xennetback_xenbus_destroy;
	xneti->xni_xbusd = xbusd;

	ifp = &xneti->xni_if;
	ifp->if_softc = xneti;
	snprintf(ifp->if_xname, IFNAMSIZ, "xvif%di%d",
	    (int)domid, (int)handle);

	/* read mac address */
	err = xenbus_read(NULL, xbusd->xbusd_path, "mac", mac, sizeof(mac));
	if (err) {
		aprint_error_ifnet(ifp, "can't read %s/mac: %d\n",
		    xbusd->xbusd_path, err);
		goto fail;
	}
	for (i = 0, p = mac; i < ETHER_ADDR_LEN; i++) {
		xneti->xni_enaddr[i] = strtoul(p, &e, 16);
		if ((e[0] == '\0' && i != 5) && e[0] != ':') {
			aprint_error_ifnet(ifp,
			    "%s is not a valid mac address\n", mac);
			err = EINVAL;
			goto fail;
		}
		p = &e[1];
	}

	/* we can't use the same MAC addr as our guest */
	xneti->xni_enaddr[3]++;

	/* Initialize DMA map, used only for loading PA */
	for (i = 0; i < __arraycount(xneti->xni_xstate); i++) {
		if (bus_dmamap_create(xneti->xni_xbusd->xbusd_dmat,
		    ETHER_MAX_LEN_JUMBO, XEN_NETIF_NR_SLOTS_MIN,
		    PAGE_SIZE, PAGE_SIZE, BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW,
		    &xneti->xni_xstate[i].xs_dmamap)
		    != 0) {
			aprint_error_ifnet(ifp,
			    "failed to allocate dma map\n");
			err = ENOMEM;
			goto fail;
		}
	}

	evcnt_attach_dynamic(&xneti->xni_cnt_rx_cksum_blank, EVCNT_TYPE_MISC,
	    NULL, ifp->if_xname, "Rx csum blank");
	evcnt_attach_dynamic(&xneti->xni_cnt_rx_cksum_undefer, EVCNT_TYPE_MISC,
	    NULL, ifp->if_xname, "Rx csum undeferred");

	/* create pseudo-interface */
	aprint_verbose_ifnet(ifp, "Ethernet address %s\n",
	    ether_sprintf(xneti->xni_enaddr));
	xneti->xni_ec.ec_capabilities |= ETHERCAP_VLAN_MTU | ETHERCAP_JUMBO_MTU;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_snd.ifq_maxlen =
	    uimax(ifqmaxlen, NET_TX_RING_SIZE * 2);
	ifp->if_capabilities =
		IFCAP_CSUM_UDPv4_Rx | IFCAP_CSUM_UDPv4_Tx
		| IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_TCPv4_Tx
		| IFCAP_CSUM_UDPv6_Rx | IFCAP_CSUM_UDPv6_Tx
		| IFCAP_CSUM_TCPv6_Rx | IFCAP_CSUM_TCPv6_Tx;
#define XN_M_CSUM_SUPPORTED						\
	(M_CSUM_TCPv4 | M_CSUM_UDPv4 | M_CSUM_TCPv6 | M_CSUM_UDPv6)

	ifp->if_ioctl = xennetback_ifioctl;
	ifp->if_start = xennetback_ifstart;
	ifp->if_watchdog = xennetback_ifwatchdog;
	ifp->if_init = xennetback_ifinit;
	ifp->if_stop = xennetback_ifstop;
	ifp->if_timer = 0;
	IFQ_SET_MAXLEN(&ifp->if_snd, uimax(2 * NET_TX_RING_SIZE, IFQ_MAXLEN));
	IFQ_SET_READY(&ifp->if_snd);
	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(&xneti->xni_if, xneti->xni_enaddr);

	xbusd->xbusd_otherend_changed = xennetback_frontend_changed;

	do {
		xbt = xenbus_transaction_start();
		if (xbt == NULL) {
			aprint_error_ifnet(ifp,
			    "%s: can't start transaction\n",
			    xbusd->xbusd_path);
			goto fail;
		}
		err = xenbus_printf(xbt, xbusd->xbusd_path,
		    "vifname", "%s", ifp->if_xname);
		if (err) {
			aprint_error_ifnet(ifp,
			    "failed to write %s/vifname: %d\n",
			    xbusd->xbusd_path, err);
			goto abort_xbt;
		}
		err = xenbus_printf(xbt, xbusd->xbusd_path,
		    "feature-rx-copy", "%d", 1);
		if (err) {
			aprint_error_ifnet(ifp,
			    "failed to write %s/feature-rx-copy: %d\n",
			    xbusd->xbusd_path, err);
			goto abort_xbt;
		}
		err = xenbus_printf(xbt, xbusd->xbusd_path,
		    "feature-ipv6-csum-offload", "%d", 1);
		if (err) {
			aprint_error_ifnet(ifp,
			    "failed to write %s/feature-ipv6-csum-offload: %d\n",
			    xbusd->xbusd_path, err);
			goto abort_xbt;
		}
		err = xenbus_printf(xbt, xbusd->xbusd_path,
		    "feature-sg", "%d", 1);
		if (err) {
			aprint_error_ifnet(ifp,
			    "failed to write %s/feature-sg: %d\n",
			    xbusd->xbusd_path, err);
			goto abort_xbt;
		}
	} while ((err = xenbus_transaction_end(xbt, 0)) == EAGAIN);
	if (err) {
		aprint_error_ifnet(ifp,
		    "%s: can't end transaction: %d\n",
		    xbusd->xbusd_path, err);
	}

	err = xenbus_switch_state(xbusd, NULL, XenbusStateInitWait);
	if (err) {
		aprint_error_ifnet(ifp,
		    "failed to switch state on %s: %d\n",
		    xbusd->xbusd_path, err);
		goto fail;
	}
	return 0;

abort_xbt:
	xenbus_transaction_end(xbt, 1);
fail:
	kmem_free(xneti, sizeof(*xneti));
	return err;
}

int
xennetback_xenbus_destroy(void *arg)
{
	struct xnetback_instance *xneti = arg;

	aprint_verbose_ifnet(&xneti->xni_if, "disconnecting\n");

	if (xneti->xni_ih != NULL) {
		hypervisor_mask_event(xneti->xni_evtchn);
		xen_intr_disestablish(xneti->xni_ih);
		xneti->xni_ih = NULL;
	}

	mutex_enter(&xnetback_lock);
	SLIST_REMOVE(&xnetback_instances,
	    xneti, xnetback_instance, next);
	mutex_exit(&xnetback_lock);

	ether_ifdetach(&xneti->xni_if);
	if_detach(&xneti->xni_if);

	evcnt_detach(&xneti->xni_cnt_rx_cksum_blank);
	evcnt_detach(&xneti->xni_cnt_rx_cksum_undefer);

	/* Destroy DMA maps */
	for (int i = 0; i < __arraycount(xneti->xni_xstate); i++) {
		if (xneti->xni_xstate[i].xs_dmamap != NULL) {
			bus_dmamap_destroy(xneti->xni_xbusd->xbusd_dmat,
			    xneti->xni_xstate[i].xs_dmamap);
			xneti->xni_xstate[i].xs_dmamap = NULL;
		}
	}

	if (xneti->xni_txring.sring) {
		xen_shm_unmap(xneti->xni_tx_ring_va, 1,
		    &xneti->xni_tx_ring_handle);
	}
	if (xneti->xni_rxring.sring) {
		xen_shm_unmap(xneti->xni_rx_ring_va, 1,
		    &xneti->xni_rx_ring_handle);
	}
	if (xneti->xni_tx_ring_va != 0) {
		uvm_km_free(kernel_map, xneti->xni_tx_ring_va,
		    PAGE_SIZE, UVM_KMF_VAONLY);
		xneti->xni_tx_ring_va = 0;
	}
	if (xneti->xni_rx_ring_va != 0) {
		uvm_km_free(kernel_map, xneti->xni_rx_ring_va,
		    PAGE_SIZE, UVM_KMF_VAONLY);
		xneti->xni_rx_ring_va = 0;
	}
	kmem_free(xneti, sizeof(*xneti));
	return 0;
}

static int
xennetback_connect(struct xnetback_instance *xneti)
{
	int err;
	netif_tx_sring_t *tx_ring;
	netif_rx_sring_t *rx_ring;
	evtchn_op_t evop;
	u_long tx_ring_ref, rx_ring_ref;
	grant_ref_t gtx_ring_ref, grx_ring_ref;
	u_long revtchn, rx_copy;
	struct xenbus_device *xbusd = xneti->xni_xbusd;

	/* read communication information */
	err = xenbus_read_ul(NULL, xbusd->xbusd_otherend,
	    "tx-ring-ref", &tx_ring_ref, 10);
	if (err) {
		xenbus_dev_fatal(xbusd, err, "reading %s/tx-ring-ref",
		    xbusd->xbusd_otherend);
		return -1;
	}
	err = xenbus_read_ul(NULL, xbusd->xbusd_otherend,
	    "rx-ring-ref", &rx_ring_ref, 10);
	if (err) {
		xenbus_dev_fatal(xbusd, err, "reading %s/rx-ring-ref",
		    xbusd->xbusd_otherend);
		return -1;
	}
	err = xenbus_read_ul(NULL, xbusd->xbusd_otherend,
	    "event-channel", &revtchn, 10);
	if (err) {
		xenbus_dev_fatal(xbusd, err, "reading %s/event-channel",
		    xbusd->xbusd_otherend);
		return -1;
	}
	err = xenbus_read_ul(NULL, xbusd->xbusd_otherend,
	    "request-rx-copy", &rx_copy, 10);
	if (err == ENOENT || !rx_copy) {
		xenbus_dev_fatal(xbusd, err,
		    "%s/request-rx-copy not supported by frontend",
		    xbusd->xbusd_otherend);
		return -1;
	} else if (err) {
		xenbus_dev_fatal(xbusd, err, "reading %s/request-rx-copy",
		    xbusd->xbusd_otherend);
		return -1;
	}

	/* allocate VA space and map rings */
	xneti->xni_tx_ring_va = uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
	    UVM_KMF_VAONLY);
	if (xneti->xni_tx_ring_va == 0) {
		xenbus_dev_fatal(xbusd, ENOMEM,
		    "can't get VA for TX ring", xbusd->xbusd_otherend);
		goto err1;
	}
	tx_ring = (void *)xneti->xni_tx_ring_va;

	xneti->xni_rx_ring_va = uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
	    UVM_KMF_VAONLY);
	if (xneti->xni_rx_ring_va == 0) {
		xenbus_dev_fatal(xbusd, ENOMEM,
		    "can't get VA for RX ring", xbusd->xbusd_otherend);
		goto err1;
	}
	rx_ring = (void *)xneti->xni_rx_ring_va;

	gtx_ring_ref = tx_ring_ref;
        if (xen_shm_map(1, xneti->xni_domid, &gtx_ring_ref,
	    xneti->xni_tx_ring_va, &xneti->xni_tx_ring_handle, 0) != 0) {
		aprint_error_ifnet(&xneti->xni_if,
		    "can't map TX grant ref\n");
		goto err2;
	}
	BACK_RING_INIT(&xneti->xni_txring, tx_ring, PAGE_SIZE);

	grx_ring_ref = rx_ring_ref;
        if (xen_shm_map(1, xneti->xni_domid, &grx_ring_ref,
	    xneti->xni_rx_ring_va, &xneti->xni_rx_ring_handle, 0) != 0) {
		aprint_error_ifnet(&xneti->xni_if,
		    "can't map RX grant ref\n");
		goto err2;
	}
	BACK_RING_INIT(&xneti->xni_rxring, rx_ring, PAGE_SIZE);

	evop.cmd = EVTCHNOP_bind_interdomain;
	evop.u.bind_interdomain.remote_dom = xneti->xni_domid;
	evop.u.bind_interdomain.remote_port = revtchn;
	err = HYPERVISOR_event_channel_op(&evop);
	if (err) {
		aprint_error_ifnet(&xneti->xni_if,
		    "can't get event channel: %d\n", err);
		goto err2;
	}
	xneti->xni_evtchn = evop.u.bind_interdomain.local_port;
	xen_wmb();
	xneti->xni_status = CONNECTED;
	xen_wmb();

	xneti->xni_ih = xen_intr_establish_xname(-1, &xen_pic,
	    xneti->xni_evtchn, IST_LEVEL, IPL_NET, xennetback_evthandler,
	    xneti, false, xneti->xni_if.if_xname);
	KASSERT(xneti->xni_ih != NULL);
	xennetback_ifinit(&xneti->xni_if);
	hypervisor_unmask_event(xneti->xni_evtchn);
	hypervisor_notify_via_evtchn(xneti->xni_evtchn);
	return 0;

err2:
	/* unmap rings */
	if (xneti->xni_tx_ring_handle != 0) {
		xen_shm_unmap(xneti->xni_tx_ring_va, 1,
		    &xneti->xni_tx_ring_handle);
	}

	if (xneti->xni_rx_ring_handle != 0) {
		xen_shm_unmap(xneti->xni_rx_ring_va, 1,
		    &xneti->xni_rx_ring_handle);
	}
err1:
	/* free rings VA space */
	if (xneti->xni_rx_ring_va != 0)
		uvm_km_free(kernel_map, xneti->xni_rx_ring_va,
		    PAGE_SIZE, UVM_KMF_VAONLY);

	if (xneti->xni_tx_ring_va != 0)
		uvm_km_free(kernel_map, xneti->xni_tx_ring_va,
		    PAGE_SIZE, UVM_KMF_VAONLY);

	return -1;

}

static void
xennetback_frontend_changed(void *arg, XenbusState new_state)
{
	struct xnetback_instance *xneti = arg;
	struct xenbus_device *xbusd = xneti->xni_xbusd;

	XENPRINTF(("%s: new state %d\n", xneti->xni_if.if_xname, new_state));
	switch(new_state) {
	case XenbusStateInitialising:
	case XenbusStateInitialised:
		break;

	case XenbusStateConnected:
		if (xneti->xni_status == CONNECTED)
			break;
		if (xennetback_connect(xneti) == 0)
			xenbus_switch_state(xbusd, NULL, XenbusStateConnected);
		break;

	case XenbusStateClosing:
		xneti->xni_status = DISCONNECTING;
		xneti->xni_if.if_flags &= ~IFF_RUNNING;
		xneti->xni_if.if_timer = 0;
		xenbus_switch_state(xbusd, NULL, XenbusStateClosing);
		break;

	case XenbusStateClosed:
		/* otherend_changed() should handle it for us */
		panic("xennetback_frontend_changed: closed\n");
	case XenbusStateUnknown:
	case XenbusStateInitWait:
	default:
		aprint_error("%s: invalid frontend state %d\n",
		    xneti->xni_if.if_xname, new_state);
		break;
	}
	return;

}

/* lookup a xneti based on domain id and interface handle */
static bool
xnetif_lookup(domid_t dom , uint32_t handle)
{
	struct xnetback_instance *xneti;
	bool found = false;

	KASSERT(mutex_owned(&xnetback_lock));

	SLIST_FOREACH(xneti, &xnetback_instances, next) {
		if (xneti->xni_domid == dom && xneti->xni_handle == handle) {
			found = true;
			break;
		}
	}

	return found;
}

static inline void
xennetback_tx_response(struct xnetback_instance *xneti, int id, int status)
{
	RING_IDX resp_prod;
	netif_tx_response_t *txresp;
	int do_event;

	resp_prod = xneti->xni_txring.rsp_prod_pvt;
	txresp = RING_GET_RESPONSE(&xneti->xni_txring, resp_prod);

	txresp->id = id;
	txresp->status = status;
	xneti->xni_txring.rsp_prod_pvt++;
	RING_PUSH_RESPONSES_AND_CHECK_NOTIFY(&xneti->xni_txring, do_event);
	if (do_event) {
		XENPRINTF(("%s send event\n", xneti->xni_if.if_xname));
		hypervisor_notify_via_evtchn(xneti->xni_evtchn);
	}
}

static const char *
xennetback_tx_check_packet(const netif_tx_request_t *txreq, bool first)
{
	if (__predict_false((txreq->flags & NETTXF_more_data) == 0 &&
	    txreq->offset + txreq->size > PAGE_SIZE))
		return "crossing page boundary";

	if (__predict_false(txreq->size > ETHER_MAX_LEN_JUMBO))
		return "bigger then jumbo";

	if (first &&
	    __predict_false(txreq->size < ETHER_HDR_LEN))
		return "too short";

	return NULL;
}

static int
xennetback_copy(struct ifnet *ifp, gnttab_copy_t *gop, int copycnt,
    const char *dir)
{
	/*
	 * Copy the data and ack it. Delaying it until the mbuf is
	 * freed will stall transmit.
	 */
	if (HYPERVISOR_grant_table_op(GNTTABOP_copy, gop, copycnt) != 0) {
		printf("%s: GNTTABOP_copy %s failed", ifp->if_xname, dir);
		return EINVAL;
	}

	for (int i = 0; i < copycnt; i++) {
		if (gop->status != GNTST_okay) {
			printf("%s GNTTABOP_copy[%d] %s %d\n",
			    ifp->if_xname, i, dir, gop->status);
			return EINVAL;
		}
	}

	return 0;
}

static void
xennetback_tx_copy_abort(struct ifnet *ifp, struct xnetback_instance *xneti,
	int queued)
{
	struct xnetback_xstate *xst;

	for (int i = 0; i < queued; i++) {
		xst = &xneti->xni_xstate[i];

		if (xst->xs_loaded) {
			KASSERT(xst->xs_m != NULL);
			bus_dmamap_unload(xneti->xni_xbusd->xbusd_dmat,
			    xst->xs_dmamap);
			xst->xs_loaded = false;
			m_freem(xst->xs_m);
		}

		xennetback_tx_response(xneti, xst->xs_tx.id, NETIF_RSP_ERROR);
		if_statinc(ifp, if_ierrors);
	}
}

static void
xennetback_tx_copy_process(struct ifnet *ifp, struct xnetback_instance *xneti,
	int queued)
{
	gnttab_copy_t *gop;
	struct xnetback_xstate *xst;
	int copycnt = 0, seg = 0;
	size_t goff = 0, segoff = 0, gsize, take;
	bus_dmamap_t dm = NULL;
	paddr_t ma;

	for (int i = 0; i < queued; i++) {
		xst = &xneti->xni_xstate[i];

		if (xst->xs_m != NULL) {
			KASSERT(xst->xs_m->m_pkthdr.len == xst->xs_tx.size);
			if (__predict_false(bus_dmamap_load_mbuf(
			    xneti->xni_xbusd->xbusd_dmat,
			    xst->xs_dmamap, xst->xs_m, BUS_DMA_NOWAIT) != 0))
				goto abort;
			xst->xs_loaded = true;
			dm = xst->xs_dmamap;
			seg = 0;
			goff = segoff = 0;
		}

		gsize = xst->xs_tx_size;
		goff = 0;
		for (; seg < dm->dm_nsegs && gsize > 0; seg++) {
			bus_dma_segment_t *ds = &dm->dm_segs[seg];
			ma = ds->ds_addr;
			take = uimin(gsize, ds->ds_len);

			KASSERT(copycnt <= NB_XMIT_PAGES_BATCH);
			if (copycnt == NB_XMIT_PAGES_BATCH) {
				if (xennetback_copy(ifp, xneti->xni_gop_copy,
				    copycnt, "Tx") != 0)
					goto abort;
				copycnt = 0;
			}

			/* Queue for the copy */
			gop = &xneti->xni_gop_copy[copycnt++];
			memset(gop, 0, sizeof(*gop));
			gop->flags = GNTCOPY_source_gref;
			gop->len = take;

			gop->source.u.ref = xst->xs_tx.gref;
			gop->source.offset = xst->xs_tx.offset + goff;
			gop->source.domid = xneti->xni_domid;

			gop->dest.offset = (ma & PAGE_MASK) + segoff;
			KASSERT(gop->dest.offset <= PAGE_SIZE);
			gop->dest.domid = DOMID_SELF;
			gop->dest.u.gmfn = ma >> PAGE_SHIFT;

			goff += take;
			gsize -= take;
			if (take + segoff < ds->ds_len) {
				segoff += take;
				/* Segment not completely consumed yet */
				break;
			}
			segoff = 0;
		}
		KASSERT(gsize == 0);
		KASSERT(goff == xst->xs_tx_size);
	}
	if (copycnt > 0) {
		if (xennetback_copy(ifp, xneti->xni_gop_copy, copycnt, "Tx"))
			goto abort;
		copycnt = 0;
	}

	/* If we got here, the whole copy was successful */
	for (int i = 0; i < queued; i++) {
		xst = &xneti->xni_xstate[i];

		xennetback_tx_response(xneti, xst->xs_tx.id, NETIF_RSP_OKAY);

		if (xst->xs_m != NULL) {
			KASSERT(xst->xs_loaded);
			bus_dmamap_unload(xneti->xni_xbusd->xbusd_dmat,
			    xst->xs_dmamap);

			if (xst->xs_tx.flags & NETTXF_csum_blank) {
				xennet_checksum_fill(ifp, xst->xs_m,
				    &xneti->xni_cnt_rx_cksum_blank,
				    &xneti->xni_cnt_rx_cksum_undefer);
			} else if (xst->xs_tx.flags & NETTXF_data_validated) {
				xst->xs_m->m_pkthdr.csum_flags =
				    XN_M_CSUM_SUPPORTED;
			}
			m_set_rcvif(xst->xs_m, ifp);

			if_percpuq_enqueue(ifp->if_percpuq, xst->xs_m);
		}
	}

	return;

abort:
	xennetback_tx_copy_abort(ifp, xneti, queued);
}

static int
xennetback_tx_m0len_fragment(struct xnetback_instance *xneti,
    int m0_len, int req_cons, int *cntp)
{
	netif_tx_request_t *txreq;

	/* This assumes all the requests are already pushed into the ring */
	*cntp = 1;
	do {
		txreq = RING_GET_REQUEST(&xneti->xni_txring, req_cons);
		if (m0_len <= txreq->size || *cntp > XEN_NETIF_NR_SLOTS_MIN)
			return -1;
		if (RING_REQUEST_CONS_OVERFLOW(&xneti->xni_txring, req_cons))
			return -1;
			
		m0_len -= txreq->size;
		req_cons++;
		(*cntp)++;
	} while (txreq->flags & NETTXF_more_data);

	return m0_len;
}

static int
xennetback_evthandler(void *arg)
{
	struct xnetback_instance *xneti = arg;
	struct ifnet *ifp = &xneti->xni_if;
	netif_tx_request_t txreq;
	struct mbuf *m, *m0 = NULL, *mlast = NULL;
	int receive_pending;
	RING_IDX req_cons;
	int queued = 0, m0_len = 0;
	struct xnetback_xstate *xst;
	const bool nupnrun = ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) !=
	    (IFF_UP | IFF_RUNNING));
	bool discard = 0;

	XENPRINTF(("xennetback_evthandler "));
	req_cons = xneti->xni_txring.req_cons;
	while (1) {
		xen_rmb(); /* be sure to read the request before updating */
		xneti->xni_txring.req_cons = req_cons;
		xen_wmb();
		RING_FINAL_CHECK_FOR_REQUESTS(&xneti->xni_txring,
		    receive_pending);
		if (receive_pending == 0)
			break;
		RING_COPY_REQUEST(&xneti->xni_txring, req_cons,
		    &txreq);
		xen_rmb();
		XENPRINTF(("%s pkt size %d\n", xneti->xni_if.if_xname,
		    txreq.size));
		req_cons++;
		if (__predict_false(nupnrun || discard)) {
			/* interface not up, drop all requests */
			if_statinc(ifp, if_iqdrops);
			discard = (txreq.flags & NETTXF_more_data) != 0;
			xennetback_tx_response(xneti, txreq.id,
			    NETIF_RSP_DROPPED);
			continue;
		}

		/*
		 * Do some sanity checks, and queue copy of the data.
		 */
		const char *msg = xennetback_tx_check_packet(&txreq,
		    m0 == NULL);
		if (__predict_false(msg != NULL)) {
			printf("%s: packet with size %d is %s\n",
			    ifp->if_xname, txreq.size, msg);
			discard = (txreq.flags & NETTXF_more_data) != 0;
			xennetback_tx_response(xneti, txreq.id,
			    NETIF_RSP_ERROR);
			if_statinc(ifp, if_ierrors);
			continue;
		}

		/* get a mbuf for this fragment */
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (__predict_false(m == NULL)) {
			static struct timeval lasttime;
mbuf_fail:
			if (ratecheck(&lasttime, &xni_pool_errintvl))
				printf("%s: mbuf alloc failed\n",
				    ifp->if_xname);
			xennetback_tx_copy_abort(ifp, xneti, queued);
			queued = 0;
			m0 = NULL;
			discard = (txreq.flags & NETTXF_more_data) != 0;
			xennetback_tx_response(xneti, txreq.id,
			    NETIF_RSP_DROPPED);
			if_statinc(ifp, if_ierrors);
			continue;
		}
		m->m_len = m->m_pkthdr.len = txreq.size;

		if (!m0 && (txreq.flags & NETTXF_more_data)) {
			/*
			 * The first fragment of multi-fragment Tx request
			 * contains total size. Need to read whole
			 * chain to determine actual size of the first
			 * (i.e. current) fragment.
			 */
			int cnt;
			m0_len = xennetback_tx_m0len_fragment(xneti,
			    txreq.size, req_cons, &cnt);
			if (m0_len < 0) {
				m_freem(m);
				discard = 1;
				xennetback_tx_response(xneti, txreq.id,
				    NETIF_RSP_DROPPED);
				if_statinc(ifp, if_ierrors);
				continue;
			}
			m->m_len = m0_len;
			KASSERT(cnt <= XEN_NETIF_NR_SLOTS_MIN);

			if (queued + cnt >= NB_XMIT_PAGES_BATCH) {
				/*
				 * Flush queue if too full to fit this
				 * new packet whole.
				 */
				xennetback_tx_copy_process(ifp, xneti, queued);
				queued = 0;
			}
		}

		if (m->m_len > MHLEN) {
			MCLGET(m, M_DONTWAIT);
			if (__predict_false((m->m_flags & M_EXT) == 0)) {
				m_freem(m);
				goto mbuf_fail;
			}
			if (__predict_false(m->m_len > MCLBYTES)) {
				/* one more mbuf necessary */
				struct mbuf *mn;
				MGET(mn, M_DONTWAIT, MT_DATA);
				if (__predict_false(mn == NULL)) {
					m_freem(m);
					goto mbuf_fail;
				}
				if (m->m_len - MCLBYTES > MLEN) {
					MCLGET(mn, M_DONTWAIT);
					if ((mn->m_flags & M_EXT) == 0) {
						m_freem(mn);
						m_freem(m);
						goto mbuf_fail;
					}
				}
				mn->m_len = m->m_len - MCLBYTES;
				m->m_len = MCLBYTES;
				m->m_next = mn;
				KASSERT(mn->m_len <= MCLBYTES);
			}
			KASSERT(m->m_len <= MCLBYTES);
		}

		if (m0 || (txreq.flags & NETTXF_more_data)) {
			if (m0 == NULL) {
				m0 = m;
				mlast = (m->m_next) ? m->m_next : m;
				KASSERT(mlast->m_next == NULL);
			} else {
				/* Coalesce like m_cat(), but without copy */
				KASSERT(mlast != NULL);
				if (M_TRAILINGSPACE(mlast) >= m->m_pkthdr.len) {
					mlast->m_len +=  m->m_pkthdr.len;
					m_freem(m);
				} else {
					mlast->m_next = m;
					mlast = (m->m_next) ? m->m_next : m;
					KASSERT(mlast->m_next == NULL);
				}
			}
		}

		XENPRINTF(("%s pkt offset %d size %d id %d req_cons %d\n",
		    xneti->xni_if.if_xname, txreq.offset,
		    txreq.size, txreq.id, req_cons & (RING_SIZE(&xneti->xni_txring) - 1)));

		xst = &xneti->xni_xstate[queued];
		xst->xs_m = (m0 == NULL || m == m0) ? m : NULL;
		xst->xs_tx = txreq;
		/* Fill the length of _this_ fragment */
		xst->xs_tx_size = (m == m0) ? m0_len : m->m_pkthdr.len;
		queued++;

		KASSERT(queued <= NB_XMIT_PAGES_BATCH);
		if (__predict_false(m0 &&
		    (txreq.flags & NETTXF_more_data) == 0)) {
			/* Last fragment, stop appending mbufs */
			m0 = NULL;
		}
		if (queued == NB_XMIT_PAGES_BATCH) {
			KASSERT(m0 == NULL);
			xennetback_tx_copy_process(ifp, xneti, queued);
			queued = 0;
		}
	}
	if (m0) {
		/* Queue empty, and still unfinished multi-fragment request */
		printf("%s: dropped unfinished multi-fragment\n",
		    ifp->if_xname);
		xennetback_tx_copy_abort(ifp, xneti, queued);
		queued = 0;
		m0 = NULL;
	}
	if (queued > 0)
		xennetback_tx_copy_process(ifp, xneti, queued);

	/* check to see if we can transmit more packets */
	if_schedule_deferred_start(ifp);

	return 1;
}

static int
xennetback_ifioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	//struct xnetback_instance *xneti = ifp->if_softc;
	//struct ifreq *ifr = (struct ifreq *)data;
	int s, error;

	s = splnet();
	error = ether_ioctl(ifp, cmd, data);
	if (error == ENETRESET)
		error = 0;
	splx(s);
	return error;
}

static void
xennetback_ifstart(struct ifnet *ifp)
{
	struct xnetback_instance *xneti = ifp->if_softc;

	/*
	 * The Xen communication channel is much more efficient if we can
	 * schedule batch of packets for the domain. Deferred start by network
	 * stack will enqueue all pending mbufs in the interface's send queue
	 * before it is processed by the soft interrupt handler.
	 */
	xennetback_ifsoftstart_copy(xneti);
}

static void
xennetback_rx_copy_process(struct ifnet *ifp, struct xnetback_instance *xneti,
	int queued, int copycnt)
{
	int notify;
	struct xnetback_xstate *xst;

	if (xennetback_copy(ifp, xneti->xni_gop_copy, copycnt, "Rx") != 0) {
		/* message already displayed */
		goto free_mbufs;
	}

	/* update pointer */
	xen_rmb();
	xneti->xni_rxring.req_cons += queued;
	xneti->xni_rxring.rsp_prod_pvt += queued;
	RING_PUSH_RESPONSES_AND_CHECK_NOTIFY(&xneti->xni_rxring, notify);

	/* send event */
	if (notify) {
		xen_rmb();
		XENPRINTF(("%s receive event\n",
		    xneti->xni_if.if_xname));
		hypervisor_notify_via_evtchn(xneti->xni_evtchn);
	}

free_mbufs:
	/* now that data was copied we can free the mbufs */
	for (int j = 0; j < queued; j++) {
		xst = &xneti->xni_xstate[j];
		if (xst->xs_loaded) {
			bus_dmamap_unload(xneti->xni_xbusd->xbusd_dmat,
			    xst->xs_dmamap);
			xst->xs_loaded = false;
		}
		if (xst->xs_m != NULL) {
			m_freem(xst->xs_m);
			xst->xs_m = NULL;
		}
	}
}

static void
xennetback_rx_copy_queue(struct xnetback_instance *xneti,
    struct xnetback_xstate *xst0, int rsp_prod_pvt, int *queued, int *copycntp)
{
	struct xnetback_xstate *xst = xst0;
	gnttab_copy_t *gop;
	struct netif_rx_request rxreq;
	netif_rx_response_t *rxresp;
	paddr_t ma;
	size_t goff, segoff, segsize, take, totsize;
	int copycnt = *copycntp, reqcnt = *queued;
	const bus_dmamap_t dm = xst0->xs_dmamap;
	const bool multiseg = (dm->dm_nsegs > 1);

	KASSERT(xst0 == &xneti->xni_xstate[reqcnt]);

	RING_COPY_REQUEST(&xneti->xni_rxring,
	    xneti->xni_rxring.req_cons + reqcnt, &rxreq);
	goff = 0;
	rxresp = RING_GET_RESPONSE(&xneti->xni_rxring, rsp_prod_pvt + reqcnt);
	reqcnt++;

	rxresp->id = rxreq.id;
	rxresp->offset = 0;
	if ((xst0->xs_m->m_pkthdr.csum_flags & XN_M_CSUM_SUPPORTED) != 0) {
		rxresp->flags = NETRXF_csum_blank;
	} else {
		rxresp->flags = NETRXF_data_validated;
	}
	if (multiseg)
		rxresp->flags |= NETRXF_more_data;

	totsize = xst0->xs_m->m_pkthdr.len;

	/*
	 * Arrange for the mbuf contents to be copied into one or more
	 * provided memory pages.
	 */
	for (int seg = 0; seg < dm->dm_nsegs; seg++) {
		ma = dm->dm_segs[seg].ds_addr;
		segsize = dm->dm_segs[seg].ds_len;
		segoff = 0;

		while (segoff < segsize) {
			take = uimin(PAGE_SIZE - goff, segsize - segoff);
			KASSERT(take <= totsize);

			/* add copy request */
			gop = &xneti->xni_gop_copy[copycnt++];
			gop->flags = GNTCOPY_dest_gref;
			gop->source.offset = (ma & PAGE_MASK) + segoff;
			gop->source.domid = DOMID_SELF;
			gop->source.u.gmfn = ma >> PAGE_SHIFT;

			gop->dest.u.ref = rxreq.gref;
			gop->dest.offset = goff;
			gop->dest.domid = xneti->xni_domid;

			gop->len = take;

			segoff += take;
			goff += take;
			totsize -= take;

			if (goff == PAGE_SIZE && totsize > 0) {
				rxresp->status = goff;

				/* Take next grant */
				RING_COPY_REQUEST(&xneti->xni_rxring,
				    xneti->xni_rxring.req_cons + reqcnt,
				    &rxreq);
				goff = 0;
				rxresp = RING_GET_RESPONSE(&xneti->xni_rxring,
				    rsp_prod_pvt + reqcnt);
				reqcnt++;

				rxresp->id = rxreq.id;
				rxresp->offset = 0;
				rxresp->flags = NETRXF_more_data;

				xst++;
				xst->xs_m = NULL;
			}
		}
	}
	rxresp->flags &= ~NETRXF_more_data;
	rxresp->status = goff;
	KASSERT(totsize == 0);

	KASSERT(copycnt > *copycntp);
	KASSERT(reqcnt > *queued);
	*copycntp = copycnt;
	*queued = reqcnt;
}

static void
xennetback_ifsoftstart_copy(struct xnetback_instance *xneti)
{
	struct ifnet *ifp = &xneti->xni_if;
	struct mbuf *m;
	int queued = 0;
	RING_IDX req_prod, rsp_prod_pvt;
	struct xnetback_xstate *xst;
	int copycnt = 0;
	bool abort;

	XENPRINTF(("xennetback_ifsoftstart_copy "));
	int s = splnet();
	if (__predict_false((ifp->if_flags & IFF_RUNNING) == 0)) {
		splx(s);
		return;
	}

	while (!IFQ_IS_EMPTY(&ifp->if_snd)) {
		XENPRINTF(("pkt\n"));
		req_prod = xneti->xni_rxring.sring->req_prod;
		rsp_prod_pvt = xneti->xni_rxring.rsp_prod_pvt;
		xen_rmb();

		abort = false;
		KASSERT(queued == 0);
		KASSERT(copycnt == 0);
		while (copycnt < NB_XMIT_PAGES_BATCH) {
#define XN_RING_FULL(cnt)	\
			req_prod == xneti->xni_rxring.req_cons + (cnt) ||  \
			xneti->xni_rxring.req_cons - (rsp_prod_pvt + cnt) ==  \
			NET_RX_RING_SIZE

			if (__predict_false(XN_RING_FULL(1))) {
				/* out of ring space */
				XENPRINTF(("xennetback_ifstart: ring full "
				    "req_prod 0x%x req_cons 0x%x rsp_prod_pvt "
				    "0x%x\n",
				    req_prod,
				    xneti->xni_rxring.req_cons + queued,
				    rsp_prod_pvt + queued));
				abort = true;
				break;
			}

			IFQ_DEQUEUE(&ifp->if_snd, m);
			if (m == NULL)
				break;

again:
			xst = &xneti->xni_xstate[queued];

			/*
			 * For short packets it's always way faster passing
			 * single defragmented packet, even with feature-sg.
			 * Try to defragment first if the result is likely
			 * to fit into a single mbuf.
			 */
			if (m->m_pkthdr.len < MCLBYTES && m->m_next)
				(void)m_defrag(m, M_DONTWAIT);

			if (bus_dmamap_load_mbuf(
			    xneti->xni_xbusd->xbusd_dmat,
			    xst->xs_dmamap, m, BUS_DMA_NOWAIT) != 0) {
				if (m_defrag(m, M_DONTWAIT) == NULL) {
					m_freem(m);
					static struct timeval lasttime;
					if (ratecheck(&lasttime, &xni_pool_errintvl))
						printf("%s: fail defrag mbuf\n",
						    ifp->if_xname);
					continue;
				}

				if (__predict_false(bus_dmamap_load_mbuf(
				    xneti->xni_xbusd->xbusd_dmat,
				    xst->xs_dmamap, m, BUS_DMA_NOWAIT) != 0)) {
					printf("%s: cannot load mbuf\n",
					    ifp->if_xname);
					m_freem(m);
					continue;
				}
			}
			KASSERT(xst->xs_dmamap->dm_nsegs < NB_XMIT_PAGES_BATCH);
			KASSERTMSG(queued <= copycnt, "queued %d > copycnt %d",
			    queued, copycnt);

			if (__predict_false(XN_RING_FULL(
			    xst->xs_dmamap->dm_nsegs))) {
				/* Ring too full to fit the packet */
				bus_dmamap_unload(xneti->xni_xbusd->xbusd_dmat,
				    xst->xs_dmamap);
				m_freem(m);
				abort = true;
				break;
			}
			if (__predict_false(copycnt + xst->xs_dmamap->dm_nsegs >
			    NB_XMIT_PAGES_BATCH)) {
				/* Batch already too full, flush and retry */
				bus_dmamap_unload(xneti->xni_xbusd->xbusd_dmat,
				    xst->xs_dmamap);
				xennetback_rx_copy_process(ifp, xneti, queued,
				    copycnt);
				queued = copycnt = 0;
				goto again;
			}

			/* Now committed to send */
			xst->xs_loaded = true;
			xst->xs_m = m;
			xennetback_rx_copy_queue(xneti, xst,
			    rsp_prod_pvt, &queued, &copycnt);

			if_statinc(ifp, if_opackets);
			bpf_mtap(ifp, m, BPF_D_OUT);
		}
		KASSERT(copycnt <= NB_XMIT_PAGES_BATCH);
		KASSERT(queued <= copycnt);
		if (copycnt > 0) {
			xennetback_rx_copy_process(ifp, xneti, queued, copycnt);
			queued = copycnt = 0;
		}
		/*
		 * note that we don't use RING_FINAL_CHECK_FOR_REQUESTS()
		 * here, as the frontend doesn't notify when adding
		 * requests anyway
		 */
		if (__predict_false(abort ||
		    !RING_HAS_UNCONSUMED_REQUESTS(&xneti->xni_rxring))) {
			/* ring full */
			ifp->if_timer = 1;
			break;
		}
	}
	splx(s);
}

static void
xennetback_ifwatchdog(struct ifnet * ifp)
{
	/*
	 * We can get to the following condition: transmit stalls because the
	 * ring is full when the ifq is full too.
	 *
	 * In this case (as, unfortunately, we don't get an interrupt from xen
	 * on transmit) nothing will ever call xennetback_ifstart() again.
	 * Here we abuse the watchdog to get out of this condition.
	 */
	XENPRINTF(("xennetback_ifwatchdog\n"));
	xennetback_ifstart(ifp);
}

static int
xennetback_ifinit(struct ifnet *ifp)
{
	struct xnetback_instance *xneti = ifp->if_softc;
	int s = splnet();

	if ((ifp->if_flags & IFF_UP) == 0) {
		splx(s);
		return 0;
	}
	if (xneti->xni_status == CONNECTED)
		ifp->if_flags |= IFF_RUNNING;
	splx(s);
	return 0;
}

static void
xennetback_ifstop(struct ifnet *ifp, int disable)
{
	struct xnetback_instance *xneti = ifp->if_softc;
	int s = splnet();

	ifp->if_flags &= ~IFF_RUNNING;
	ifp->if_timer = 0;
	if (xneti->xni_status == CONNECTED) {
		xennetback_evthandler(ifp->if_softc); /* flush pending RX requests */
	}
	splx(s);
}

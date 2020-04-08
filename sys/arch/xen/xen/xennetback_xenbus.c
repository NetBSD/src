/*      $NetBSD: xennetback_xenbus.c,v 1.94 2020/04/07 11:47:06 jdolecek Exp $      */

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
__KERNEL_RCSID(0, "$NetBSD: xennetback_xenbus.c,v 1.94 2020/04/07 11:47:06 jdolecek Exp $");

#include "opt_xen.h"

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
#include <sys/intr.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <net/netisr.h>
#include <net/bpf.h>

#include <net/if_ether.h>

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

extern pt_entry_t xpmap_pg_nx;

#define NET_TX_RING_SIZE __RING_SIZE((netif_tx_sring_t *)0, PAGE_SIZE)
#define NET_RX_RING_SIZE __RING_SIZE((netif_rx_sring_t *)0, PAGE_SIZE)

/* linux wants at last 16 bytes free in front of the packet */
#define LINUX_REQUESTED_OFFSET 16

/* ratecheck(9) for pool allocation failures */
static const struct timeval xni_pool_errintvl = { 30, 0 };  /* 30s, each */

/* state of a xnetback instance */
typedef enum {
	CONNECTED,
	DISCONNECTING,
	DISCONNECTED
} xnetback_state_t;

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
static void xennetback_mbuf_addr(struct mbuf *, paddr_t *, int *);

static SLIST_HEAD(, xnetback_instance) xnetback_instances;
static kmutex_t xnetback_lock;

static bool xnetif_lookup(domid_t, uint32_t);
static int  xennetback_evthandler(void *);

static struct xenbus_backend_driver xvif_backend_driver = {
	.xbakd_create = xennetback_xenbus_create,
	.xbakd_type = "vif"
};

/*
 * Number of packets to transmit in one hypercall (= number of pages to
 * transmit at once).
 */
#define NB_XMIT_PAGES_BATCH 64

/* arrays used in xennetback_ifstart(), too large to allocate on stack */
/* XXXSMP */
static gnttab_copy_t     xstart_gop_copy[NB_XMIT_PAGES_BATCH];
static struct mbuf *mbufs_sent[NB_XMIT_PAGES_BATCH];
static struct _req_info {
	int id;
	int flags;
} xstart_req[NB_XMIT_PAGES_BATCH];


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

	if (xnetif_lookup(domid, handle)) {
		return EEXIST;
	}
	xneti = kmem_zalloc(sizeof(*xneti), KM_SLEEP);
	xneti->xni_domid = domid;
	xneti->xni_handle = handle;
	xneti->xni_status = DISCONNECTED;

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
	/* create pseudo-interface */
	aprint_verbose_ifnet(ifp, "Ethernet address %s\n",
	    ether_sprintf(xneti->xni_enaddr));
	xneti->xni_ec.ec_capabilities |= ETHERCAP_VLAN_MTU;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_snd.ifq_maxlen =
	    uimax(ifqmaxlen, NET_TX_RING_SIZE * 2);
	ifp->if_capabilities =
		IFCAP_CSUM_IPv4_Rx | IFCAP_CSUM_IPv4_Tx
		| IFCAP_CSUM_UDPv4_Rx | IFCAP_CSUM_UDPv4_Tx
		| IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_TCPv4_Tx
		| IFCAP_CSUM_UDPv6_Rx | IFCAP_CSUM_UDPv6_Tx
		| IFCAP_CSUM_TCPv6_Rx | IFCAP_CSUM_TCPv6_Tx;
#define XN_M_CSUM_SUPPORTED	(				\
		M_CSUM_TCPv4 | M_CSUM_UDPv4 | M_CSUM_IPv4	\
		| M_CSUM_TCPv6 | M_CSUM_UDPv6			\
	)
	ifp->if_ioctl = xennetback_ifioctl;
	ifp->if_start = xennetback_ifstart;
	ifp->if_watchdog = xennetback_ifwatchdog;
	ifp->if_init = xennetback_ifinit;
	ifp->if_stop = xennetback_ifstop;
	ifp->if_timer = 0;
	IFQ_SET_READY(&ifp->if_snd);
	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(&xneti->xni_if, xneti->xni_enaddr);

	mutex_enter(&xnetback_lock);
	SLIST_INSERT_HEAD(&xnetback_instances, xneti, next);
	mutex_exit(&xnetback_lock);

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
	struct gnttab_unmap_grant_ref op;
	int err;

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

	if (xneti->xni_txring.sring) {
		op.host_addr = xneti->xni_tx_ring_va;
		op.handle = xneti->xni_tx_ring_handle;
		op.dev_bus_addr = 0;
		err = HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref,
		    &op, 1);
		if (err)
			aprint_error_ifnet(&xneti->xni_if,
					"unmap_grant_ref failed: %d\n", err);
	}
	if (xneti->xni_rxring.sring) {
		op.host_addr = xneti->xni_rx_ring_va;
		op.handle = xneti->xni_rx_ring_handle;
		op.dev_bus_addr = 0;
		err = HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref,
		    &op, 1);
		if (err)
			aprint_error_ifnet(&xneti->xni_if,
					"unmap_grant_ref failed: %d\n", err);
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
	struct gnttab_map_grant_ref op;
	struct gnttab_unmap_grant_ref uop;
	evtchn_op_t evop;
	u_long tx_ring_ref, rx_ring_ref;
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

	op.host_addr = xneti->xni_tx_ring_va;
	op.flags = GNTMAP_host_map;
	op.ref = tx_ring_ref;
	op.dom = xneti->xni_domid;
	err = HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, &op, 1);
	if (err || op.status) {
		aprint_error_ifnet(&xneti->xni_if,
		    "can't map TX grant ref: err %d status %d\n",
		    err, op.status);
		goto err2;
	}
	xneti->xni_tx_ring_handle = op.handle;
	BACK_RING_INIT(&xneti->xni_txring, tx_ring, PAGE_SIZE);

	op.host_addr = xneti->xni_rx_ring_va;
	op.flags = GNTMAP_host_map;
	op.ref = rx_ring_ref;
	op.dom = xneti->xni_domid;
	err = HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, &op, 1);
	if (err || op.status) {
		aprint_error_ifnet(&xneti->xni_if,
		    "can't map RX grant ref: err %d status %d\n",
		    err, op.status);
		goto err2;
	}
	xneti->xni_rx_ring_handle = op.handle;
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

	xneti->xni_ih = xen_intr_establish_xname(-1, &xen_pic, xneti->xni_evtchn,
	    IST_LEVEL, IPL_NET, xennetback_evthandler, xneti, false,
	    xneti->xni_if.if_xname);
	KASSERT(xneti->xni_ih != NULL);
	xennetback_ifinit(&xneti->xni_if);
	hypervisor_unmask_event(xneti->xni_evtchn);
	hypervisor_notify_via_evtchn(xneti->xni_evtchn);
	return 0;

err2:
	/* unmap rings */
	if (xneti->xni_tx_ring_handle != 0) {
		uop.host_addr = xneti->xni_tx_ring_va;
		uop.handle = xneti->xni_tx_ring_handle;
		uop.dev_bus_addr = 0;
		err = HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref,
		    &uop, 1);
		if (err)
			aprint_error_ifnet(&xneti->xni_if,
			    "unmap_grant_ref failed: %d\n", err);
	}

	if (xneti->xni_rx_ring_handle != 0) {
		uop.host_addr = xneti->xni_rx_ring_va;
		uop.handle = xneti->xni_rx_ring_handle;
		uop.dev_bus_addr = 0;
		err = HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref,
		    &uop, 1);
		if (err)
			aprint_error_ifnet(&xneti->xni_if,
			    "unmap_grant_ref failed: %d\n", err);
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

	mutex_enter(&xnetback_lock);
	SLIST_FOREACH(xneti, &xnetback_instances, next) {
		if (xneti->xni_domid == dom && xneti->xni_handle == handle) {
			found = true;
			break;
		}
	}
	mutex_exit(&xnetback_lock);

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

static inline const char *
xennetback_tx_check_packet(const netif_tx_request_t *txreq, int vlan)
{
	if (__predict_false(txreq->size < ETHER_HDR_LEN))
		return "too small";

	if (__predict_false(txreq->offset + txreq->size > PAGE_SIZE))
		return "crossing a page boundary";

	int maxlen = ETHER_MAX_LEN - ETHER_CRC_LEN;
	if (vlan)
		maxlen += ETHER_VLAN_ENCAP_LEN;
	if (__predict_false(txreq->size > maxlen))
		return "too big";

	/* Somewhat duplicit, MCLBYTES is > ETHER_MAX_LEN */
	if (__predict_false(txreq->size > MCLBYTES))
		return "bigger than MCLBYTES";

	return NULL;
}

static void
xennetback_tx_copy_process(struct ifnet *ifp, struct xnetback_instance *xneti,
	int queued)
{
	int i = 0;
	gnttab_copy_t *gop;
	struct mbuf *m;
	struct _req_info *req;

	/*
	 * Copy the data and ack it. Delaying it until the mbuf is
	 * freed will stall transmit.
	 */
	if (HYPERVISOR_grant_table_op(GNTTABOP_copy, xstart_gop_copy, queued)
	    != 0) {
		printf("%s: GNTTABOP_copy failed", ifp->if_xname);
		goto abort;
	}

	for (; i < queued; i++) {
		gop = &xstart_gop_copy[i];
		m = mbufs_sent[i];
		req = &xstart_req[i];

		if (gop->status != GNTST_okay) {
			printf("%s GNTTABOP_copy[%d] %d\n",
			    ifp->if_xname, i, gop->status);
			goto abort;
		}

		xennetback_tx_response(xneti, req->id, NETIF_RSP_OKAY);

		if ((ifp->if_flags & IFF_PROMISC) == 0) {
			struct ether_header *eh =
			    mtod(m, struct ether_header *);
			if (ETHER_IS_MULTICAST(eh->ether_dhost) == 0 &&
			    memcmp(CLLADDR(ifp->if_sadl), eh->ether_dhost,
			    ETHER_ADDR_LEN) != 0) {
				m_freem(m);
				continue; /* packet is not for us */
			}
		}

		if (req->flags & NETTXF_csum_blank)
			xennet_checksum_fill(ifp, m);
		else if (req->flags & NETTXF_data_validated)
			m->m_pkthdr.csum_flags = XN_M_CSUM_SUPPORTED;
		m_set_rcvif(m, ifp);

		if_percpuq_enqueue(ifp->if_percpuq, m);
	}

	return;

abort:
	for (; i < queued; i++) {
		m = mbufs_sent[i];
		req = &xstart_req[i];

		m_freem(m);
		xennetback_tx_response(xneti, req->id, NETIF_RSP_ERROR);
		if_statinc(ifp, if_ierrors);
	}
}

static int
xennetback_evthandler(void *arg)
{
	struct xnetback_instance *xneti = arg;
	struct ifnet *ifp = &xneti->xni_if;
	netif_tx_request_t txreq;
	struct mbuf *m;
	int receive_pending;
	RING_IDX req_cons;
	gnttab_copy_t *gop;
	paddr_t pa;
	int offset, queued = 0;

	XENPRINTF(("xennetback_evthandler "));
	req_cons = xneti->xni_txring.req_cons;
	xen_rmb();
	while (1) {
		xen_rmb(); /* be sure to read the request before updating */
		xneti->xni_txring.req_cons = req_cons;
		xen_wmb();
		RING_FINAL_CHECK_FOR_REQUESTS(&xneti->xni_txring,
		    receive_pending);
		if (receive_pending == 0)
			break;
		RING_COPY_REQUEST(&xneti->xni_txring, req_cons, &txreq);
		xen_rmb();
		XENPRINTF(("%s pkt size %d\n", xneti->xni_if.if_xname,
		    txreq.size));
		req_cons++;
		if (__predict_false((ifp->if_flags & (IFF_UP | IFF_RUNNING)) !=
		    (IFF_UP | IFF_RUNNING))) {
			/* interface not up, drop */
			xennetback_tx_response(xneti, txreq.id,
			    NETIF_RSP_DROPPED);
			continue;
		}

		/*
		 * Do some sanity checks, and map the packet's page.
		 */
		const char *msg = xennetback_tx_check_packet(&txreq,
		    xneti->xni_ec.ec_capenable & ETHERCAP_VLAN_MTU);
		if (msg) {
			printf("%s: packet with size %d is %s\n",
			    ifp->if_xname, txreq.size, msg);
			xennetback_tx_response(xneti, txreq.id,
			    NETIF_RSP_ERROR);
			if_statinc(ifp, if_ierrors);
			continue;
		}

		/* get a mbuf for this packet */
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (__predict_false(m == NULL)) {
			static struct timeval lasttime;
			if (ratecheck(&lasttime, &xni_pool_errintvl))
				printf("%s: mbuf alloc failed\n",
				    ifp->if_xname);
			xennetback_tx_response(xneti, txreq.id,
			    NETIF_RSP_DROPPED);
			if_statinc(ifp, if_ierrors);
			continue;
		}
		if (txreq.size > MHLEN) {
			MCLGET(m, M_DONTWAIT);
			if (__predict_false(m->m_ext_storage.ext_buf == NULL)) {
				m_freem(m);
				xennetback_tx_response(xneti, txreq.id,
				    NETIF_RSP_DROPPED);
				if_statinc(ifp, if_ierrors);
				continue;
			}
		}

		XENPRINTF(("%s pkt offset %d size %d id %d req_cons %d\n",
		    xneti->xni_if.if_xname, txreq.offset,
		    txreq.size, txreq.id, MASK_NETIF_TX_IDX(req_cons)));

		xennetback_mbuf_addr(m, &pa, &offset);

		/* Queue for the copy */
		gop = &xstart_gop_copy[queued];
		memset(gop, 0, sizeof(*gop));
		gop->flags = GNTCOPY_source_gref;
		gop->len = txreq.size;

		gop->source.u.ref = txreq.gref;
		gop->source.offset = txreq.offset;
		gop->source.domid = xneti->xni_domid;

		gop->dest.offset = offset;
		gop->dest.domid = DOMID_SELF;
		gop->dest.u.gmfn = xpmap_ptom(pa) >> PAGE_SHIFT;

		m->m_len = m->m_pkthdr.len = txreq.size;
		mbufs_sent[queued] = m;

		xstart_req[queued].id = txreq.id;
		xstart_req[queued].flags = txreq.flags;

		queued++;

		KASSERT(queued <= NB_XMIT_PAGES_BATCH);
		if (queued == NB_XMIT_PAGES_BATCH) {
			xennetback_tx_copy_process(ifp, xneti, queued);
			queued = 0;
		}
	}
	if (queued > 0)
		xennetback_tx_copy_process(ifp, xneti, queued);
	xen_rmb(); /* be sure to read the request before updating pointer */
	xneti->xni_txring.req_cons = req_cons;
	xen_wmb();

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

/*
 * sighly different from m_dup(); for some reason m_dup() can return
 * a chain where the data area can cross a page boundary.
 * This doesn't happens with the function below.
 */
static struct mbuf *
xennetback_copymbuf(struct mbuf *m)
{
	struct mbuf *new_m;

	MGETHDR(new_m, M_DONTWAIT, MT_DATA);
	if (__predict_false(new_m == NULL)) {
		return NULL;
	}
	if (m->m_pkthdr.len > MHLEN) {
		MCLGET(new_m, M_DONTWAIT);
		if (__predict_false(
		    (new_m->m_flags & M_EXT) == 0)) {
			m_freem(new_m);
			return NULL;
		}
	}
	m_copydata(m, 0, m->m_pkthdr.len,
	    mtod(new_m, void *));
	new_m->m_len = new_m->m_pkthdr.len =
	    m->m_pkthdr.len;

	/*
	 * Need to retain csum flags to know if csum was actually computed.
	 * This is used to set NETRXF_csum_blank/NETRXF_data_validated.
	 */
	new_m->m_pkthdr.csum_flags = m->m_pkthdr.csum_flags;

	return new_m;
}

/* return physical page address and offset of data area of an mbuf */
static void
xennetback_mbuf_addr(struct mbuf *m, paddr_t *xmit_pa, int *offset)
{
	switch (m->m_flags & (M_EXT|M_EXT_CLUSTER)) {
	case M_EXT|M_EXT_CLUSTER:
		KASSERT(m->m_ext.ext_paddr != M_PADDR_INVALID);
		*xmit_pa = m->m_ext.ext_paddr;
		*offset = m->m_data - m->m_ext.ext_buf;
		break;
	case 0:
		KASSERT(m->m_paddr != M_PADDR_INVALID);
		*xmit_pa = m->m_paddr;
		*offset = M_BUFOFFSET(m) +
		    (m->m_data - M_BUFADDR(m));
		break;
	default:
		if (__predict_false(
		    !pmap_extract(pmap_kernel(),
		    (vaddr_t)m->m_data, xmit_pa))) {
			panic("xennet_start: no pa");
		}
		*offset = 0;
		break;
	}
	*offset += (*xmit_pa & ~PTE_FRAME);
	*xmit_pa = (*xmit_pa & PTE_FRAME);
}

static void
xennetback_ifsoftstart_copy(struct xnetback_instance *xneti)
{
	struct ifnet *ifp = &xneti->xni_if;
	struct mbuf *m, *new_m;
	paddr_t xmit_pa;
	paddr_t xmit_ma;
	int i, j;
	netif_rx_response_t *rxresp;
	netif_rx_request_t rxreq;
	RING_IDX req_prod, resp_prod;
	int do_event = 0;
	gnttab_copy_t *gop;
	int id, offset;
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
		resp_prod = xneti->xni_rxring.rsp_prod_pvt;
		xen_rmb();

		gop = xstart_gop_copy;
		abort = false;
		for (i = 0; !IFQ_IS_EMPTY(&ifp->if_snd);) {
			XENPRINTF(("have a packet\n"));
			IFQ_POLL(&ifp->if_snd, m);
			if (__predict_false(m == NULL))
				panic("xennetback_ifstart: IFQ_POLL");
			if (__predict_false(
			    req_prod == xneti->xni_rxring.req_cons ||
			    xneti->xni_rxring.req_cons - resp_prod ==
			    NET_RX_RING_SIZE)) {
				/* out of ring space */
				XENPRINTF(("xennetback_ifstart: ring full "
				    "req_prod 0x%x req_cons 0x%x resp_prod "
				    "0x%x\n",
				    req_prod, xneti->xni_rxring.req_cons,
				    resp_prod));
				abort = true;
				break;
			}
			if (__predict_false(i == NB_XMIT_PAGES_BATCH))
				break; /* we filled the array */

			xennetback_mbuf_addr(m, &xmit_pa, &offset);
			if (m->m_pkthdr.len != m->m_len ||
			    (offset + m->m_pkthdr.len) > PAGE_SIZE) {
				new_m = xennetback_copymbuf(m);
				if (__predict_false(new_m == NULL)) {
					static struct timeval lasttime;
					if (ratecheck(&lasttime, &xni_pool_errintvl))
						printf("%s: cannot allocate new mbuf\n",
						    ifp->if_xname);
					abort = 1;
					break;
				} else {
					IFQ_DEQUEUE(&ifp->if_snd, m);
					m_freem(m);
					m = new_m;
					xennetback_mbuf_addr(m,
					    &xmit_pa, &offset);
				}
			} else {
				IFQ_DEQUEUE(&ifp->if_snd, m);
			}

			KASSERT(xmit_pa != POOL_PADDR_INVALID);
			KASSERT((offset + m->m_pkthdr.len) <= PAGE_SIZE);
			xmit_ma = xpmap_ptom(xmit_pa);
			/* start filling ring */
			gop->flags = GNTCOPY_dest_gref;
			gop->source.offset = offset;
			gop->source.domid = DOMID_SELF;
			gop->source.u.gmfn = xmit_ma >> PAGE_SHIFT;

			RING_COPY_REQUEST(&xneti->xni_rxring,
			    xneti->xni_rxring.req_cons, &rxreq);
			gop->dest.u.ref = rxreq.gref;
			gop->dest.offset = 0;
			gop->dest.domid = xneti->xni_domid;

			gop->len = m->m_pkthdr.len;
			gop++;

			id = rxreq.id;
			xen_rmb();
			xneti->xni_rxring.req_cons++;
			rxresp = RING_GET_RESPONSE(&xneti->xni_rxring,
			    resp_prod);
			rxresp->id = id;
			rxresp->offset = 0;
			rxresp->status = m->m_pkthdr.len;
			if ((m->m_pkthdr.csum_flags &
			    XN_M_CSUM_SUPPORTED) != 0) {
				rxresp->flags = NETRXF_csum_blank;
			} else {
				rxresp->flags = NETRXF_data_validated;
			}

			mbufs_sent[i] = m;
			resp_prod++;
			i++; /* this packet has been queued */
			if_statinc(ifp, if_opackets);
			bpf_mtap(ifp, m, BPF_D_OUT);
		}
		if (i != 0) {
			if (HYPERVISOR_grant_table_op(GNTTABOP_copy,
			    xstart_gop_copy, i) != 0) {
				panic("%s: GNTTABOP_copy failed",
				    ifp->if_xname);
			}

			for (j = 0; j < i; j++) {
				if (xstart_gop_copy[j].status != GNTST_okay) {
					printf("%s GNTTABOP_copy[%d] %d\n",
					    ifp->if_xname,
					    j, xstart_gop_copy[j].status);
					printf("%s: req_prod %u req_cons "
					    "%u rsp_prod %u rsp_prod_pvt %u "
					    "i %d\n",
					    ifp->if_xname,
					    xneti->xni_rxring.sring->req_prod,
					    xneti->xni_rxring.req_cons,
					    xneti->xni_rxring.sring->rsp_prod,
					    xneti->xni_rxring.rsp_prod_pvt,
					    i);
					rxresp = RING_GET_RESPONSE(
					    &xneti->xni_rxring,
					    xneti->xni_rxring.rsp_prod_pvt + j);
					rxresp->status = NETIF_RSP_ERROR;
				}
			}

			/* update pointer */
			KASSERT(
			    xneti->xni_rxring.rsp_prod_pvt + i == resp_prod);
			xneti->xni_rxring.rsp_prod_pvt = resp_prod;
			RING_PUSH_RESPONSES_AND_CHECK_NOTIFY(
			    &xneti->xni_rxring, j);
			if (j)
				do_event = 1;
			/* now we can free the mbufs */
			for (j = 0; j < i; j++) {
				m_freem(mbufs_sent[j]);
			}
		}
		/* send event */
		if (do_event) {
			xen_rmb();
			XENPRINTF(("%s receive event\n",
			    xneti->xni_if.if_xname));
			hypervisor_notify_via_evtchn(xneti->xni_evtchn);
			do_event = 0;
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
		XENPRINTF(("%s: req_prod 0x%x resp_prod 0x%x req_cons 0x%x "
		    "event 0x%x\n", ifp->if_xname, xneti->xni_txring->req_prod,
		    xneti->xni_txring->resp_prod, xneti->xni_txring->req_cons,
		    xneti->xni_txring->event));
		xennetback_evthandler(ifp->if_softc); /* flush pending RX requests */
	}
	splx(s);
}

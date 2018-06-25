/*      $NetBSD: xennetback_xenbus.c,v 1.61.2.2 2018/06/25 07:25:48 pgoyette Exp $      */

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
__KERNEL_RCSID(0, "$NetBSD: xennetback_xenbus.c,v 1.61.2.2 2018/06/25 07:25:48 pgoyette Exp $");

#include "opt_xen.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
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

/* hash list for TX requests */
/* descriptor of a packet being handled by the kernel */
struct xni_pkt {
	int pkt_id; /* packet's ID */
	grant_handle_t pkt_handle;
	struct xnetback_instance *pkt_xneti; /* pointer back to our softc */
};

static inline void xni_pkt_unmap(struct xni_pkt *, vaddr_t);


/* pools for xni_pkt */
struct pool xni_pkt_pool;
/* ratecheck(9) for pool allocation failures */
struct timeval xni_pool_errintvl = { 30, 0 };  /* 30s, each */
/*
 * Backend network device driver for Xen
 */

/* state of a xnetback instance */
typedef enum {CONNECTED, DISCONNECTING, DISCONNECTED} xnetback_state_t;

/* we keep the xnetback instances in a linked list */
struct xnetback_instance {
	SLIST_ENTRY(xnetback_instance) next;
	struct xenbus_device *xni_xbusd; /* our xenstore entry */
	domid_t xni_domid;		/* attached to this domain */
	uint32_t xni_handle;	/* domain-specific handle */
	xnetback_state_t xni_status;
	void *xni_softintr;

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
static void xennetback_ifsoftstart_transfer(void *);
static void xennetback_ifsoftstart_copy(void *);
static void xennetback_ifwatchdog(struct ifnet *);
static int  xennetback_ifinit(struct ifnet *);
static void xennetback_ifstop(struct ifnet *, int);

static int  xennetback_xenbus_create(struct xenbus_device *);
static int  xennetback_xenbus_destroy(void *);
static void xennetback_frontend_changed(void *, XenbusState);

static inline void xennetback_tx_response(struct xnetback_instance *,
    int, int);
static void xennetback_tx_free(struct mbuf * , void *, size_t, void *);

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
/*
 * We will transfer a mapped page to the remote domain, and remap another
 * page in place immediately. For this we keep a list of pages available.
 * When the list is empty, we ask the hypervisor to give us
 * NB_XMIT_PAGES_BATCH pages back.
 */
static unsigned long mcl_pages[NB_XMIT_PAGES_BATCH]; /* our physical pages */
int mcl_pages_alloc; /* current index in mcl_pages */
static int  xennetback_get_mcl_page(paddr_t *);
static void xennetback_get_new_mcl_pages(void);
/*
 * If we can't transfer the mbuf directly, we have to copy it to a page which
 * will be transferred to the remote domain. We use a pool_cache
 * for this, or the mbuf cluster pool cache if MCLBYTES == PAGE_SIZE
 */
#if MCLBYTES != PAGE_SIZE
pool_cache_t xmit_pages_cache;
#endif
pool_cache_t xmit_pages_cachep;

/* arrays used in xennetback_ifstart(), too large to allocate on stack */
/* XXXSMP */
static mmu_update_t xstart_mmu[NB_XMIT_PAGES_BATCH];
static multicall_entry_t xstart_mcl[NB_XMIT_PAGES_BATCH + 1];
static gnttab_transfer_t xstart_gop_transfer[NB_XMIT_PAGES_BATCH];
static gnttab_copy_t     xstart_gop_copy[NB_XMIT_PAGES_BATCH];
static struct mbuf *mbufs_sent[NB_XMIT_PAGES_BATCH];
static struct _pages_pool_free {
	vaddr_t va;
	paddr_t pa;
} pages_pool_free[NB_XMIT_PAGES_BATCH];


static inline void
xni_pkt_unmap(struct xni_pkt *pkt, vaddr_t pkt_va)
{
	xen_shm_unmap(pkt_va, 1, &pkt->pkt_handle);
	pool_put(&xni_pkt_pool, pkt);
}

void
xvifattach(int n)
{
	int i;
	struct pglist mlist;
	struct vm_page *pg;

	XENPRINTF(("xennetback_init\n"));

	/*
	 * steal some non-managed pages to the VM system, to replace
	 * mbuf cluster or xmit_pages_pool pages given to foreign domains.
	 */
	if (uvm_pglistalloc(PAGE_SIZE * NB_XMIT_PAGES_BATCH, 0, 0xffffffff,
	    0, 0, &mlist, NB_XMIT_PAGES_BATCH, 0) != 0)
		panic("xennetback_init: uvm_pglistalloc");
	for (i = 0, pg = mlist.tqh_first; pg != NULL;
	    pg = pg->pageq.queue.tqe_next, i++)
		mcl_pages[i] = xpmap_ptom(VM_PAGE_TO_PHYS(pg)) >> PAGE_SHIFT;
	if (i != NB_XMIT_PAGES_BATCH)
		panic("xennetback_init: %d mcl pages", i);
	mcl_pages_alloc = NB_XMIT_PAGES_BATCH - 1;

	/* initialise pools */
	pool_init(&xni_pkt_pool, sizeof(struct xni_pkt), 0, 0, 0,
	    "xnbpkt", NULL, IPL_VM);
#if MCLBYTES != PAGE_SIZE
	xmit_pages_cache = pool_cache_init(PAGE_SIZE, 0, 0, 0, "xnbxm", NULL,
	    IPL_VM, NULL, NULL, NULL);
	xmit_pages_cachep = xmit_pages_cache;
#else
	xmit_pages_cachep = mcl_cache;
#endif

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
	char *val, *e, *p;
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
	if ((err = xenbus_read(NULL, xbusd->xbusd_path, "mac", NULL, &val))) {
		aprint_error_ifnet(ifp, "can't read %s/mac: %d\n",
		    xbusd->xbusd_path, err);
		goto fail;
	}
	for (i = 0, p = val; i < 6; i++) {
		xneti->xni_enaddr[i] = strtoul(p, &e, 16);
		if ((e[0] == '\0' && i != 5) && e[0] != ':') {
			aprint_error_ifnet(ifp,
			    "%s is not a valid mac address\n", val);
			free(val, M_DEVBUF);
			err = EINVAL;
			goto fail;
		}
		p = &e[1];
	}
	free(val, M_DEVBUF);

	/* we can't use the same MAC addr as our guest */
	xneti->xni_enaddr[3]++;
	/* create pseudo-interface */
	aprint_verbose_ifnet(ifp, "Ethernet address %s\n",
	    ether_sprintf(xneti->xni_enaddr));
	xneti->xni_ec.ec_capabilities |= ETHERCAP_VLAN_MTU;
	ifp->if_flags =
	    IFF_BROADCAST|IFF_SIMPLEX|IFF_NOTRAILERS|IFF_MULTICAST;
	ifp->if_snd.ifq_maxlen =
	    max(ifqmaxlen, NET_TX_RING_SIZE * 2);
	ifp->if_capabilities = IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_UDPv4_Tx;
	ifp->if_ioctl = xennetback_ifioctl;
	ifp->if_start = xennetback_ifstart;
	ifp->if_watchdog = xennetback_ifwatchdog;
	ifp->if_init = xennetback_ifinit;
	ifp->if_stop = xennetback_ifstop;
	ifp->if_timer = 0;
	IFQ_SET_READY(&ifp->if_snd);
	if_attach(ifp);
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
		    "feature-rx-flip", "%d", 1);
		if (err) {
			aprint_error_ifnet(ifp,
			    "failed to write %s/feature-rx-flip: %d\n",
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

	if (xneti->xni_status == CONNECTED) {
		KASSERT(xneti->xni_ih);
		hypervisor_mask_event(xneti->xni_evtchn);
		intr_disestablish(xneti->xni_ih);
		xneti->xni_ih = NULL;

		if (xneti->xni_softintr) {
			softint_disestablish(xneti->xni_softintr);
			xneti->xni_softintr = NULL;
		}
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

	/* read comunication informations */
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
	if (err == ENOENT)
		rx_copy = 0;
	else if (err) {
		xenbus_dev_fatal(xbusd, err, "reading %s/request-rx-copy",
		    xbusd->xbusd_otherend);
		return -1;
	}

	if (rx_copy)
		xneti->xni_softintr = softint_establish(SOFTINT_NET,
		    xennetback_ifsoftstart_copy, xneti);
	else
		xneti->xni_softintr = softint_establish(SOFTINT_NET,
		    xennetback_ifsoftstart_transfer, xneti);

	if (xneti->xni_softintr == NULL) {
		err = ENOMEM;
		xenbus_dev_fatal(xbusd, ENOMEM,
		    "can't allocate softint", xbusd->xbusd_otherend);
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

	xneti->xni_ih = intr_establish_xname(0, &xen_pic, xneti->xni_evtchn,
	    IST_LEVEL, IPL_NET, xennetback_evthandler, xneti, false,
	    xneti->xni_if.if_xname);
	KASSERT(xneti->xni_ih != NULL);
	xennetback_ifinit(&xneti->xni_if);
	hypervisor_enable_event(xneti->xni_evtchn);
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

	softint_disestablish(xneti->xni_softintr);
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
		xneti->xni_if.if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
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

/* get a page to remplace a mbuf cluster page given to a domain */
static int
xennetback_get_mcl_page(paddr_t *map)
{
	if (mcl_pages_alloc < 0)
		/*
		 * we exhausted our allocation. We can't allocate new ones yet
		 * because the current pages may not have been loaned to
		 * the remote domain yet. We have to let the caller do this.
		 */
		return -1;

	*map = ((paddr_t)mcl_pages[mcl_pages_alloc]) << PAGE_SHIFT;
	mcl_pages_alloc--;
	return 0;
}

static void
xennetback_get_new_mcl_pages(void)
{
	int nb_pages;
	struct xen_memory_reservation res;

	/* get some new pages. */
	set_xen_guest_handle(res.extent_start, mcl_pages);
	res.nr_extents = NB_XMIT_PAGES_BATCH;
	res.extent_order = 0;
	res.address_bits = 0;
	res.domid = DOMID_SELF;

	nb_pages = HYPERVISOR_memory_op(XENMEM_increase_reservation, &res);
	if (nb_pages <= 0) {
		printf("xennetback: can't get new mcl pages (%d)\n", nb_pages);
		return;
	}
	if (nb_pages != NB_XMIT_PAGES_BATCH)
		printf("xennetback: got only %d new mcl pages\n", nb_pages);

	mcl_pages_alloc = nb_pages - 1;
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

	return NULL;
}

static int
xennetback_evthandler(void *arg)
{
	struct xnetback_instance *xneti = arg;
	struct ifnet *ifp = &xneti->xni_if;
	netif_tx_request_t txreq;
	struct xni_pkt *pkt;
	vaddr_t pkt_va;
	struct mbuf *m;
	int receive_pending, err;
	RING_IDX req_cons;

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
			ifp->if_ierrors++;
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
			ifp->if_ierrors++;
			continue;
		}

		XENPRINTF(("%s pkt offset %d size %d id %d req_cons %d\n",
		    xneti->xni_if.if_xname, txreq.offset,
		    txreq.size, txreq.id, MASK_NETIF_TX_IDX(req_cons)));

		pkt = pool_get(&xni_pkt_pool, PR_NOWAIT);
		if (__predict_false(pkt == NULL)) {
			static struct timeval lasttime;
			if (ratecheck(&lasttime, &xni_pool_errintvl))
				printf("%s: xnbpkt alloc failed\n",
				    ifp->if_xname);
			xennetback_tx_response(xneti, txreq.id,
			    NETIF_RSP_DROPPED);
			ifp->if_ierrors++;
			m_freem(m);
			continue;
		}
		err = xen_shm_map(1, xneti->xni_domid, &txreq.gref, &pkt_va,
		    &pkt->pkt_handle, XSHM_RO);
		if (__predict_false(err == ENOMEM)) {
			xennetback_tx_response(xneti, txreq.id,
			    NETIF_RSP_DROPPED);
			ifp->if_ierrors++;
			pool_put(&xni_pkt_pool, pkt);
			m_freem(m);
			continue;
		}

		if (__predict_false(err)) {
			printf("%s: mapping foreing page failed: %d\n",
			    xneti->xni_if.if_xname, err);
			xennetback_tx_response(xneti, txreq.id,
			    NETIF_RSP_ERROR);
			ifp->if_ierrors++;
			pool_put(&xni_pkt_pool, pkt);
			m_freem(m);
			continue;
		}

		if ((ifp->if_flags & IFF_PROMISC) == 0) {
			struct ether_header *eh =
			    (void*)(pkt_va + txreq.offset);
			if (ETHER_IS_MULTICAST(eh->ether_dhost) == 0 &&
			    memcmp(CLLADDR(ifp->if_sadl), eh->ether_dhost,
			    ETHER_ADDR_LEN) != 0) {
				xni_pkt_unmap(pkt, pkt_va);
				m_freem(m);
				xennetback_tx_response(xneti, txreq.id,
				    NETIF_RSP_OKAY);
				continue; /* packet is not for us */
			}
		}
#ifdef notyet
a lot of work is needed in the tcp stack to handle read-only ext storage
so always copy for now.
		if (((req_cons + 1) & (NET_TX_RING_SIZE - 1)) ==
		    (xneti->xni_txring.rsp_prod_pvt & (NET_TX_RING_SIZE - 1)))
#else
		if (1)
#endif /* notyet */
		{
			/*
			 * This is the last TX buffer. Copy the data and
			 * ack it. Delaying it until the mbuf is
			 * freed will stall transmit.
			 */
			m->m_len = min(MHLEN, txreq.size);
			m->m_pkthdr.len = 0;
			m_copyback(m, 0, txreq.size,
			    (void *)(pkt_va + txreq.offset));
			xni_pkt_unmap(pkt, pkt_va);
			if (m->m_pkthdr.len < txreq.size) {
				ifp->if_ierrors++;
				m_freem(m);
				xennetback_tx_response(xneti, txreq.id,
				    NETIF_RSP_DROPPED);
				continue;
			}
			xennetback_tx_response(xneti, txreq.id,
			    NETIF_RSP_OKAY);
		} else {

			pkt->pkt_id = txreq.id;
			pkt->pkt_xneti = xneti;

			MEXTADD(m, pkt_va + txreq.offset,
			    txreq.size, M_DEVBUF, xennetback_tx_free, pkt);
			m->m_pkthdr.len = m->m_len = txreq.size;
			m->m_flags |= M_EXT_ROMAP;
		}
		if ((txreq.flags & NETTXF_csum_blank) != 0) {
			xennet_checksum_fill(&m);
			if (m == NULL) {
				ifp->if_ierrors++;
				continue;
			}
		}
		m_set_rcvif(m, ifp);

		if_percpuq_enqueue(ifp->if_percpuq, m);
	}
	xen_rmb(); /* be sure to read the request before updating pointer */
	xneti->xni_txring.req_cons = req_cons;
	xen_wmb();
	/* check to see if we can transmit more packets */
	softint_schedule(xneti->xni_softintr);

	return 1;
}

static void
xennetback_tx_free(struct mbuf *m, void *va, size_t size, void *arg)
{
	int s = splnet();
	struct xni_pkt *pkt = arg;
	struct xnetback_instance *xneti = pkt->pkt_xneti;

	XENPRINTF(("xennetback_tx_free\n"));

	xennetback_tx_response(xneti, pkt->pkt_id, NETIF_RSP_OKAY);

	xni_pkt_unmap(pkt, (vaddr_t)va & ~PAGE_MASK);

	if (m)
		pool_cache_put(mb_cache, m);
	splx(s);
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
	 * schedule batch of packets for the domain. To achieve this, we
	 * schedule a soft interrupt, and just return. This way, the network
	 * stack will enqueue all pending mbufs in the interface's send queue
	 * before it is processed by the soft interrupt handler().
	 */
	softint_schedule(xneti->xni_softintr);
}

static void
xennetback_ifsoftstart_transfer(void *arg)
{
	struct xnetback_instance *xneti = arg;
	struct ifnet *ifp = &xneti->xni_if;
	struct mbuf *m;
	vaddr_t xmit_va;
	paddr_t xmit_pa;
	paddr_t xmit_ma;
	paddr_t newp_ma = 0; /* XXX gcc */
	int i, j, nppitems;
	mmu_update_t *mmup;
	multicall_entry_t *mclp;
	netif_rx_response_t *rxresp;
	netif_rx_request_t rxreq;
	RING_IDX req_prod, resp_prod;
	int do_event = 0;
	gnttab_transfer_t *gop;
	int id, offset;

	XENPRINTF(("xennetback_ifsoftstart_transfer "));
	int s = splnet();
	if (__predict_false(
	    (ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)) {
		splx(s);
		return;
	}

	while (!IFQ_IS_EMPTY(&ifp->if_snd)) {
		XENPRINTF(("pkt\n"));
		req_prod = xneti->xni_rxring.sring->req_prod;
		resp_prod = xneti->xni_rxring.rsp_prod_pvt;
		xen_rmb();

		mmup = xstart_mmu;
		mclp = xstart_mcl;
		gop = xstart_gop_transfer;
		for (nppitems = 0, i = 0; !IFQ_IS_EMPTY(&ifp->if_snd);) {
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
				ifp->if_timer = 1;
				break;
			}
			if (__predict_false(i == NB_XMIT_PAGES_BATCH))
				break; /* we filled the array */
			if (__predict_false(
			    xennetback_get_mcl_page(&newp_ma) != 0))
				break; /* out of memory */
			if ((m->m_flags & M_EXT_CLUSTER) != 0 &&
			    !M_READONLY(m) && MCLBYTES == PAGE_SIZE) {
				/* we can give this page away */
				xmit_pa = m->m_ext.ext_paddr;
				xmit_ma = xpmap_ptom(xmit_pa);
				xmit_va = (vaddr_t)m->m_ext.ext_buf;
				KASSERT(xmit_pa != M_PADDR_INVALID);
				KASSERT((xmit_va & PAGE_MASK) == 0);
				offset = m->m_data - m->m_ext.ext_buf;
			} else {
				/* we have to copy the packet */
				xmit_va = (vaddr_t)pool_cache_get_paddr(
				    xmit_pages_cachep,
				    PR_NOWAIT, &xmit_pa);
				if (__predict_false(xmit_va == 0))
					break; /* out of memory */

				KASSERT(xmit_pa != POOL_PADDR_INVALID);
				xmit_ma = xpmap_ptom(xmit_pa);
				XENPRINTF(("xennetback_get_xmit_page: got va "
				    "0x%x ma 0x%x\n", (u_int)xmit_va,
				    (u_int)xmit_ma));
				m_copydata(m, 0, m->m_pkthdr.len,
				    (char *)xmit_va + LINUX_REQUESTED_OFFSET);
				offset = LINUX_REQUESTED_OFFSET;
				pages_pool_free[nppitems].va = xmit_va;
				pages_pool_free[nppitems].pa = xmit_pa;
				nppitems++;
			}
			/* start filling ring */
			RING_COPY_REQUEST(&xneti->xni_rxring,
			    xneti->xni_rxring.req_cons, &rxreq);
			gop->ref = rxreq.gref;
			id = rxreq.id;
			xen_rmb();
			xneti->xni_rxring.req_cons++;
			rxresp = RING_GET_RESPONSE(&xneti->xni_rxring,
			    resp_prod);
			rxresp->id = id;
			rxresp->offset = offset;
			rxresp->status = m->m_pkthdr.len;
			if ((m->m_pkthdr.csum_flags &
			    (M_CSUM_TCPv4 | M_CSUM_UDPv4)) != 0) {
				rxresp->flags = NETRXF_csum_blank;
			} else {
				rxresp->flags = 0;
			}
			/*
			 * transfers the page containing the packet to the
			 * remote domain, and map newp in place.
			 */
			xpmap_ptom_map(xmit_pa, newp_ma);
			MULTI_update_va_mapping(mclp, xmit_va,
			    newp_ma | PG_V | PG_RW | PG_U | PG_M | xpmap_pg_nx, 0);
			mclp++;
			gop->mfn = xmit_ma >> PAGE_SHIFT;
			gop->domid = xneti->xni_domid;
			gop++;

			mmup->ptr = newp_ma | MMU_MACHPHYS_UPDATE;
			mmup->val = xmit_pa >> PAGE_SHIFT;
			mmup++;

			/* done with this packet */
			IFQ_DEQUEUE(&ifp->if_snd, m);
			mbufs_sent[i] = m;
			resp_prod++;
			i++; /* this packet has been queued */
			ifp->if_opackets++;
			bpf_mtap(ifp, m);
		}
		if (i != 0) {
			/*
			 * We may have allocated buffers which have entries
			 * outstanding in the page update queue -- make sure
			 * we flush those first!
			 */
			int svm = splvm();
			xpq_flush_queue();
			splx(svm);
			mclp[-1].args[MULTI_UVMFLAGS_INDEX] =
			    UVMF_TLB_FLUSH|UVMF_ALL;
			mclp->op = __HYPERVISOR_mmu_update;
			mclp->args[0] = (unsigned long)xstart_mmu;
			mclp->args[1] = i;
			mclp->args[2] = 0;
			mclp->args[3] = DOMID_SELF;
			mclp++;
			/* update the MMU */
			if (HYPERVISOR_multicall(xstart_mcl, i + 1) != 0) {
				panic("%s: HYPERVISOR_multicall failed",
				    ifp->if_xname);
			}
			for (j = 0; j < i + 1; j++) {
				if (xstart_mcl[j].result != 0) {
					printf("%s: xstart_mcl[%d] "
					    "failed (%lu)\n", ifp->if_xname,
					    j, xstart_mcl[j].result);
					printf("%s: req_prod %u req_cons "
					    "%u rsp_prod %u rsp_prod_pvt %u "
					    "i %u\n",
					    ifp->if_xname,
					    xneti->xni_rxring.sring->req_prod,
					    xneti->xni_rxring.req_cons,
					    xneti->xni_rxring.sring->rsp_prod,
					    xneti->xni_rxring.rsp_prod_pvt,
					    i);
				}
			}
			if (HYPERVISOR_grant_table_op(GNTTABOP_transfer,
			    xstart_gop_transfer, i) != 0) {
				panic("%s: GNTTABOP_transfer failed",
				    ifp->if_xname);
			}

			for (j = 0; j < i; j++) {
				if (xstart_gop_transfer[j].status != GNTST_okay) {
					printf("%s GNTTABOP_transfer[%d] %d\n",
					    ifp->if_xname,
					    j, xstart_gop_transfer[j].status);
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
			for (j = 0; j < nppitems; j++) {
				pool_cache_put_paddr(xmit_pages_cachep,
				    (void *)pages_pool_free[j].va,
				    pages_pool_free[j].pa);
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
		/* check if we need to get back some pages */
		if (mcl_pages_alloc < 0) {
			xennetback_get_new_mcl_pages();
			if (mcl_pages_alloc < 0) {
				/*
				 * setup the watchdog to try again, because
				 * xennetback_ifstart() will never be called
				 * again if queue is full.
				 */
				printf("xennetback_ifstart: no mcl_pages\n");
				ifp->if_timer = 1;
				break;
			}
		}
		/*
		 * note that we don't use RING_FINAL_CHECK_FOR_REQUESTS()
		 * here, as the frontend doesn't notify when adding
		 * requests anyway
		 */
		if (__predict_false(
		    !RING_HAS_UNCONSUMED_REQUESTS(&xneti->xni_rxring))) {
			/* ring full */
			break;
		}
	}
	splx(s);
}

static void
xennetback_ifsoftstart_copy(void *arg)
{
	struct xnetback_instance *xneti = arg;
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

	XENPRINTF(("xennetback_ifsoftstart_copy "));
	int s = splnet();
	if (__predict_false(
	    (ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)) {
		splx(s);
		return;
	}

	while (!IFQ_IS_EMPTY(&ifp->if_snd)) {
		XENPRINTF(("pkt\n"));
		req_prod = xneti->xni_rxring.sring->req_prod;
		resp_prod = xneti->xni_rxring.rsp_prod_pvt;
		xen_rmb();

		gop = xstart_gop_copy;
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
				ifp->if_timer = 1;
				break;
			}
			if (__predict_false(i == NB_XMIT_PAGES_BATCH))
				break; /* we filled the array */
			switch (m->m_flags & (M_EXT|M_EXT_CLUSTER)) {
			case M_EXT|M_EXT_CLUSTER:
				KASSERT(m->m_ext.ext_paddr != M_PADDR_INVALID);
				xmit_pa = m->m_ext.ext_paddr;
				offset = m->m_data - m->m_ext.ext_buf;
				break;
			case 0:
				KASSERT(m->m_paddr != M_PADDR_INVALID);
				xmit_pa = m->m_paddr;
				offset = M_BUFOFFSET(m) +
				    (m->m_data - M_BUFADDR(m));
				break;
			default:
				if (__predict_false(
				    !pmap_extract(pmap_kernel(),
				    (vaddr_t)m->m_data, &xmit_pa))) {
					panic("xennet_start: no pa");
				}
				offset = 0;
				break;
			}
			offset += (xmit_pa & ~PG_FRAME);
			xmit_pa = (xmit_pa & PG_FRAME);
			if (m->m_pkthdr.len != m->m_len ||
			    (offset + m->m_pkthdr.len) > PAGE_SIZE) {
				MGETHDR(new_m, M_DONTWAIT, MT_DATA);
				if (__predict_false(new_m == NULL)) {
					printf("%s: cannot allocate new mbuf\n",
					    ifp->if_xname);
					break;
				}
				if (m->m_pkthdr.len > MHLEN) {
					MCLGET(new_m, M_DONTWAIT);
					if (__predict_false(
					    (new_m->m_flags & M_EXT) == 0)) {
						XENPRINTF((
						    "%s: no mbuf cluster\n",
						    ifp->if_xname));
						m_freem(new_m);
						break;
					}
					xmit_pa = new_m->m_ext.ext_paddr;
					offset = new_m->m_data -
					    new_m->m_ext.ext_buf;
				} else {
					xmit_pa = new_m->m_paddr;
					offset = M_BUFOFFSET(new_m) +
					    (new_m->m_data - M_BUFADDR(new_m));
				}
				offset += (xmit_pa & ~PG_FRAME);
				xmit_pa = (xmit_pa & PG_FRAME);
				m_copydata(m, 0, m->m_pkthdr.len,
				    mtod(new_m, void *));
				new_m->m_len = new_m->m_pkthdr.len =
				    m->m_pkthdr.len;
				IFQ_DEQUEUE(&ifp->if_snd, m);
				m_freem(m);
				m = new_m;
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
			    (M_CSUM_TCPv4 | M_CSUM_UDPv4)) != 0) {
				rxresp->flags = NETRXF_csum_blank;
			} else {
				rxresp->flags = 0;
			}

			mbufs_sent[i] = m;
			resp_prod++;
			i++; /* this packet has been queued */
			ifp->if_opackets++;
			bpf_mtap(ifp, m);
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
		if (__predict_false(
		    !RING_HAS_UNCONSUMED_REQUESTS(&xneti->xni_rxring))) {
			/* ring full */
			break;
		}
	}
	splx(s);
}

static void
xennetback_ifwatchdog(struct ifnet * ifp)
{
	/*
	 * We can get to the following condition:
	 * transmit stalls because the ring is full when the ifq is full too.
	 * In this case (as, unfortunably, we don't get an interrupt from xen
	 * on transmit) noting will ever call xennetback_ifstart() again.
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

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
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

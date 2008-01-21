/*      $NetBSD: xennetback.c,v 1.11.2.5 2008/01/21 09:40:37 yamt Exp $      */

/*
 * Copyright (c) 2005 Manuel Bouyer.
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
 *      This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
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
 *
 */

#include "opt_xen.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
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
#include "bpfilter.h"
#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <net/if_ether.h>


#include <xen/xen.h>
#include <xen/xen_shm.h>
#include <xen/evtchn.h>
#include <xen/ctrl_if.h>

#ifdef XEN3
#else
#include <xen/xen-public/io/domain_controller.h>
#endif

#include <uvm/uvm.h>

#ifdef XENDEBUG_NET
#define XENPRINTF(x) printf x
#else
#define XENPRINTF(x)
#endif

/* hash list for TX requests */
/* descriptor of a packet being handled by the kernel */
struct xni_pkt {
	int pkt_id; /* packet's ID */
	struct xnetback_instance *pkt_xneti; /* pointer back to our softc */
	struct xni_page *pkt_page; /* page containing this packet */
};

struct xni_page {
	SLIST_ENTRY(xni_page) xni_page_next;
	int refcount;
	vaddr_t va; /* address the page is mapped to */
	paddr_t ma; /* page's machine address */
};

/* hash list of packets mapped by machine address */
SLIST_HEAD(xni_pages_hash, xni_page);
#define XNI_PAGE_HASH_SIZE 256 /* must be power of 2 */
#define XNI_PAGE_HASH_MASK (XNI_PAGE_HASH_SIZE - 1)
struct xni_pages_hash xni_tx_pages_hash[XNI_PAGE_HASH_SIZE];

/* pools for xni_pkt and xni_page */
struct pool xni_pkt_pool;
struct pool xni_page_pool;
/* ratecheck(9) for pool allocation failures */
struct timeval xni_pool_errintvl = { 30, 0 };  /* 30s, each */
/*
 * Backend network device driver for Xen
 */

static void xnetback_ctrlif_rx(ctrl_msg_t *, unsigned long);

/* state of a xnetback instance */
typedef enum {CONNECTED, DISCONNECTED} xnetback_state_t;

/* we keep the xnetback instances in a linked list */
struct xnetback_instance {
	SLIST_ENTRY(xnetback_instance) next; 
	domid_t domid;		/* attached to this domain */
	uint32_t handle;	/* domain-specific handle */
	xnetback_state_t status;
	void *xni_softintr;

	/* network interface stuff */
	struct ethercom xni_ec;
	struct callout xni_restart;
	u_int8_t xni_enaddr[ETHER_ADDR_LEN];

	/* remote domain communication stuff */
	unsigned int xni_evtchn;
	paddr_t xni_ma_rxring; /* machine address of rx shared ring */
	paddr_t xni_ma_txring; /* machine address of tx shared ring */

	netif_tx_interface_t *xni_txring;
	netif_rx_interface_t *xni_rxring;
	NETIF_RING_IDX rxreq_cons; /* our index in the RX request ring */
};
#define xni_if    xni_ec.ec_if
#define xni_bpf   xni_if.if_bpf

static int  xennetback_ifioctl(struct ifnet *, u_long, void *);
static void xennetback_ifstart(struct ifnet *);
static void xennetback_ifsoftstart(void *);
static void xennetback_ifwatchdog(struct ifnet *);
static int  xennetback_ifinit(struct ifnet *);
static void xennetback_ifstop(struct ifnet *, int);

static inline void xennetback_tx_response(struct xnetback_instance *,
    int, int);
static void xennetback_tx_free(struct mbuf * , void *, size_t, void *);


SLIST_HEAD(, xnetback_instance) xnetback_instances;

static struct xnetback_instance *xnetif_lookup(domid_t, uint32_t);
static int  xennetback_evthandler(void *);

/*
 * Number of packets to transmit in one hypercall (= number of pages to
 * transmit at once).
 */
#define NB_XMIT_PAGES_BATCH 64
/*
 * We will transfers a mapped page to the remote domain, and remap another
 * page in place immediatly. For this we keep a list of pages available.
 * When the list is empty, we ask the hypervisor to give us
 * NB_XMIT_PAGES_BATCH pages back.
 */
static unsigned long mcl_pages[NB_XMIT_PAGES_BATCH]; /* our physical pages */
int mcl_pages_alloc; /* current index in mcl_pages */
static int  xennetback_get_mcl_page(paddr_t *);
static void xennetback_get_new_mcl_pages(void);
/*
 * If we can't transfer the mbuf directly, we have to copy it to a page which
 * will be transfered to the remote domain. We use a pool_cache
 * for this, or the mbuf cluster pool cache if MCLBYTES == PAGE_SIZE
 */
#if MCLBYTES != PAGE_SIZE
pool_cache_t xmit_pages_cache;
#endif
pool_cache_t xmit_pages_cachep;

/* arrays used in xennetback_ifstart(), too large to allocate on stack */
static mmu_update_t xstart_mmu[NB_XMIT_PAGES_BATCH * 3];
static multicall_entry_t xstart_mcl[NB_XMIT_PAGES_BATCH * 2];
struct mbuf *mbufs_sent[NB_XMIT_PAGES_BATCH];
struct _pages_pool_free {
	vaddr_t va;
	paddr_t pa;
} pages_pool_free[NB_XMIT_PAGES_BATCH];

void
xennetback_init()
{
	ctrl_msg_t cmsg;
	netif_be_driver_status_t st;
	int i;
	struct pglist mlist;
	struct vm_page *pg;

	if ( !(xen_start_info.flags & SIF_INITDOMAIN) &&
	     !(xen_start_info.flags & SIF_NET_BE_DOMAIN))
		return;

	XENPRINTF(("xennetback_init\n"));

	/* initialize the mapped pages hash table */
	for (i = 0; i < XNI_PAGE_HASH_SIZE; i++) {
		SLIST_INIT(&xni_tx_pages_hash[i]);
	}


	/*
	 * steal some non-managed pages to the VM system, to remplace
	 * mbuf cluster or xmit_pages_pool pages given to foreing domains.
	 */
	if (uvm_pglistalloc(PAGE_SIZE * NB_XMIT_PAGES_BATCH, 0, 0xffffffff,
	    0, 0, &mlist, NB_XMIT_PAGES_BATCH, 0) != 0)
		panic("xennetback_init: uvm_pglistalloc");
	for (i = 0, pg = mlist.tqh_first; pg != NULL;
	    pg = pg->pageq.tqe_next, i++)
		mcl_pages[i] = xpmap_ptom(VM_PAGE_TO_PHYS(pg)) >> PAGE_SHIFT;
	if (i != NB_XMIT_PAGES_BATCH)
		panic("xennetback_init: %d mcl pages", i);
	mcl_pages_alloc = NB_XMIT_PAGES_BATCH - 1;

	/* initialise pools */
	pool_init(&xni_pkt_pool, sizeof(struct xni_pkt), 0, 0, 0,
	    "xnbpkt", NULL, IPL_VM);
	pool_init(&xni_page_pool, sizeof(struct xni_page), 0, 0, 0,
	    "xnbpa", NULL, IPL_VM);
#if MCLBYTES != PAGE_SIZE
	xmit_pages_cache = pool_cache_init(PAGE_SIZE, 0, 0, 0, "xnbxm", NULL,
	    IPL_VM, NULL, NULL, NULL);
	xmit_pages_cachep = xmit_pages_cache;
#else
	xmit_pages_cachep = mcl_cache;
#endif

	/*
	 * initialize the backend driver, register the control message handler
	 * and send driver up message.
	 */
	SLIST_INIT(&xnetback_instances);
	(void)ctrl_if_register_receiver(CMSG_NETIF_BE, xnetback_ctrlif_rx,
	    CALLBACK_IN_BLOCKING_CONTEXT);

	cmsg.type      = CMSG_NETIF_BE;
	cmsg.subtype   = CMSG_NETIF_BE_DRIVER_STATUS;
	cmsg.length    = sizeof(netif_be_driver_status_t);
	st.status      = NETIF_DRIVER_STATUS_UP;
	memcpy(cmsg.msg, &st, sizeof(st));
	ctrl_if_send_message_block(&cmsg, NULL, 0, 0);
}

static void
xnetback_ctrlif_rx(ctrl_msg_t *msg, unsigned long id)
{
	struct xnetback_instance *xneti;

	XENPRINTF(("xnetback msg %d\n", msg->subtype));
	switch (msg->subtype) {
	case CMSG_NETIF_BE_CREATE:
	{
		netif_be_create_t *req = (netif_be_create_t *)&msg->msg[0];
		struct ifnet *ifp;
		extern int ifqmaxlen; /* XXX */

		if (msg->length != sizeof(netif_be_create_t))
			goto error;
		if (xnetif_lookup(req->domid, req->netif_handle) != NULL) {
			req->status = NETIF_BE_STATUS_INTERFACE_EXISTS;
			goto end;
		}
		xneti = malloc(sizeof(struct xnetback_instance), M_DEVBUF,
		    M_NOWAIT | M_ZERO);
		if (xneti == NULL) {
			req->status = NETIF_BE_STATUS_OUT_OF_MEMORY;
			goto end;
		}
		xneti->domid = req->domid;
		xneti->handle = req->netif_handle;
		xneti->status = DISCONNECTED;

		xneti->xni_softintr = softint_establish(SOFTINT_NET,
		    xennetback_ifsoftstart, xneti);
		if (xneti->xni_softintr == NULL) {
			free(xneti, M_DEVBUF);
			req->status = NETIF_BE_STATUS_OUT_OF_MEMORY;
			goto end;
		}

		ifp = &xneti->xni_if;
		ifp->if_softc = xneti;

		/* create pseudo-interface */
		memcpy(xneti->xni_enaddr, req->mac, ETHER_ADDR_LEN);
		/* we can't use the same MAC addr as our guest */
		xneti->xni_enaddr[3]++;
		snprintf(ifp->if_xname, IFNAMSIZ, "xvif%d.%d",
		    req->domid, req->netif_handle);
		printf("%s: Ethernet address %s\n", ifp->if_xname,
		    ether_sprintf(xneti->xni_enaddr));
		ifp->if_flags =
		    IFF_BROADCAST|IFF_SIMPLEX|IFF_NOTRAILERS|IFF_MULTICAST;
		ifp->if_snd.ifq_maxlen =
		    max(ifqmaxlen, NETIF_RX_RING_SIZE * 2);
		ifp->if_ioctl = xennetback_ifioctl;
		ifp->if_start = xennetback_ifstart;
		ifp->if_watchdog = xennetback_ifwatchdog;
		ifp->if_init = xennetback_ifinit;
		ifp->if_stop = xennetback_ifstop;
		ifp->if_timer = 0;
		IFQ_SET_READY(&ifp->if_snd);
		if_attach(ifp);
		ether_ifattach(&xneti->xni_if, xneti->xni_enaddr);

		req->status = NETIF_BE_STATUS_OKAY;
		SLIST_INSERT_HEAD(&xnetback_instances, xneti, next);
		break;
	}
	case CMSG_NETIF_BE_DESTROY:
	{
		netif_be_destroy_t *req = (netif_be_destroy_t *)&msg->msg[0];
		if (msg->length != sizeof(netif_be_destroy_t))
			goto error;
		xneti = xnetif_lookup(req->domid, req->netif_handle);
		if (xneti == NULL) {
			req->status = NETIF_BE_STATUS_INTERFACE_NOT_FOUND;
			goto end;
		}
		if (xneti->status == CONNECTED) {
			req->status = NETIF_BE_STATUS_INTERFACE_CONNECTED;
			goto end;
		}
		SLIST_REMOVE(&xnetback_instances,
		    xneti, xnetback_instance, next);

		ether_ifdetach(&xneti->xni_if);
		if_detach(&xneti->xni_if);

		free(xneti, M_DEVBUF);
		req->status = NETIF_BE_STATUS_OKAY;
		break;
	}
	case CMSG_NETIF_BE_CONNECT:
	{
		netif_be_connect_t *req = (netif_be_connect_t *)&msg->msg[0];
		vaddr_t ring_rxaddr, ring_txaddr;
		int error;

		if (msg->length != sizeof(netif_be_connect_t))
			goto error;
		xneti = xnetif_lookup(req->domid, req->netif_handle);
		if (xneti == NULL) {
			req->status = NETIF_BE_STATUS_INTERFACE_NOT_FOUND;
			goto end;
		}
		if (xneti->status == CONNECTED) {
			req->status = NETIF_BE_STATUS_INTERFACE_CONNECTED;
			goto end;
		}
		ring_rxaddr = uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
		    UVM_KMF_VAONLY);
		if (ring_rxaddr == 0) {
			printf("%s: can't alloc ring VM\n",
			    xneti->xni_if.if_xname);
			req->status = NETIF_BE_STATUS_OUT_OF_MEMORY;
			goto end;
		}
		ring_txaddr = uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
		    UVM_KMF_VAONLY);
		if (ring_txaddr == 0) {
			printf("%s: can't alloc ring VM\n",
			    xneti->xni_if.if_xname);
			uvm_km_free(kernel_map, ring_rxaddr, PAGE_SIZE,
			    UVM_KMF_VAONLY);
			req->status = NETIF_BE_STATUS_OUT_OF_MEMORY;
			goto end;
		}
		xneti->xni_ma_rxring = req->rx_shmem_frame << PAGE_SHIFT;
		xneti->xni_ma_txring = req->tx_shmem_frame << PAGE_SHIFT;
		error = pmap_enter_ma(pmap_kernel(), ring_rxaddr,
		   xneti->xni_ma_rxring, 0, VM_PROT_READ | VM_PROT_WRITE,
		   PMAP_WIRED | PMAP_CANFAIL, req->domid);
		if (error) {
			goto fail_1;
		}
		error = pmap_enter_ma(pmap_kernel(), ring_txaddr,
		   xneti->xni_ma_txring, 0, VM_PROT_READ | VM_PROT_WRITE,
		   PMAP_WIRED | PMAP_CANFAIL, req->domid);
		if (error) {
			pmap_remove(pmap_kernel(), ring_rxaddr,
			    ring_rxaddr + PAGE_SIZE);
			pmap_update(pmap_kernel());
fail_1:
			uvm_km_free(kernel_map, ring_rxaddr, PAGE_SIZE,
			    UVM_KMF_VAONLY);
			uvm_km_free(kernel_map, ring_txaddr, PAGE_SIZE,
			    UVM_KMF_VAONLY);
			printf("%s: can't remap ring: error %d\n",
			    xneti->xni_if.if_xname, error);
			if (error == ENOMEM)
				req->status = NETIF_BE_STATUS_OUT_OF_MEMORY;
			else if (error == EFAULT)
				req->status = NETIF_BE_STATUS_MAPPING_ERROR;
			else
				req->status = NETIF_BE_STATUS_ERROR;
			goto end;
		}
		xneti->xni_rxring = (void *)ring_rxaddr;
		xneti->xni_txring = (void *)ring_txaddr;
		xneti->xni_evtchn = req->evtchn;
		event_set_handler(xneti->xni_evtchn, xennetback_evthandler,
		    xneti, IPL_NET, xneti->xni_if.if_xname);
		printf("%s using event channel %d\n",
		    xneti->xni_if.if_xname, xneti->xni_evtchn);
		hypervisor_enable_event(xneti->xni_evtchn);
		xneti->status = CONNECTED;
		if (xneti->xni_if.if_flags & IFF_UP)
			xneti->xni_if.if_flags |= IFF_RUNNING;
		req->status = NETIF_BE_STATUS_OKAY;
		break;
	}
	case CMSG_NETIF_BE_DISCONNECT:
	{
		netif_be_disconnect_t *req =
		    (netif_be_disconnect_t *)&msg->msg[0];
		vaddr_t ring_addr;

		if (msg->length != sizeof(netif_be_disconnect_t))
			goto error;
		xneti = xnetif_lookup(req->domid, req->netif_handle);
		if (xneti == NULL) {
			req->status = NETIF_BE_STATUS_INTERFACE_NOT_FOUND;
			goto end;
		}
		xneti->status = DISCONNECTED;
		xneti->xni_if.if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
		xneti->xni_if.if_timer = 0;
		hypervisor_mask_event(xneti->xni_evtchn);
		event_remove_handler(xneti->xni_evtchn,
		    xennetback_evthandler, xneti);
		softint_disestablish(xneti->xni_softintr);
		ring_addr = (vaddr_t)xneti->xni_rxring;
		pmap_remove(pmap_kernel(), ring_addr, ring_addr + PAGE_SIZE);
		uvm_km_free(kernel_map, ring_addr, PAGE_SIZE,
		    UVM_KMF_VAONLY);
		ring_addr = (vaddr_t)xneti->xni_txring;
		pmap_remove(pmap_kernel(), ring_addr, ring_addr + PAGE_SIZE);
		uvm_km_free(kernel_map, ring_addr, PAGE_SIZE,
		    UVM_KMF_VAONLY);

		req->status = NETIF_BE_STATUS_OKAY;
		break;
	}
	default:
error:
		printf("xnetback: wrong message subtype %d len %d\n",
		    msg->subtype, msg->length);
		msg->length = 0;
	}
end:
	XENPRINTF(("xnetback msg rep %d\n", msg->length));
	ctrl_if_send_response(msg);
	return;
}

/* lookup a xneti based on domain id and interface handle */
static struct xnetback_instance *
xnetif_lookup(domid_t dom , uint32_t handle)
{
	struct xnetback_instance *xneti;

	SLIST_FOREACH(xneti, &xnetback_instances, next) {
		if (xneti->domid == dom && xneti->handle == handle)
			return xneti;
	}
	return NULL;
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

	*map = mcl_pages[mcl_pages_alloc] << PAGE_SHIFT;
	mcl_pages_alloc--;
	return 0;
	
}

static void
xennetback_get_new_mcl_pages(void)
{
	int nb_pages;

	/* get some new pages. */
	nb_pages = HYPERVISOR_dom_mem_op(MEMOP_increase_reservation, mcl_pages,
	    NB_XMIT_PAGES_BATCH, 0);
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
	NETIF_RING_IDX resp_prod;
	netif_tx_response_t *txresp;

	resp_prod = xneti->xni_txring->resp_prod;
	txresp = &xneti->xni_txring->ring[MASK_NETIF_TX_IDX(resp_prod)].resp;

	txresp->id = id;
	txresp->status = status;
	x86_lfence();
	xneti->xni_txring->resp_prod++;
	x86_lfence();
	if (xneti->xni_txring->event == xneti->xni_txring->resp_prod) {
		XENPRINTF(("%s send event\n", xneti->xni_if.if_xname));
		hypervisor_notify_via_evtchn(xneti->xni_evtchn);
	}
}


static int
xennetback_evthandler(void *arg)
{
	struct xnetback_instance *xneti = arg;
	struct ifnet *ifp = &xneti->xni_if;
	netif_tx_request_t *txreq;
	struct xni_pkt *pkt;
	struct xni_pages_hash *pkt_hash;
	struct xni_page *pkt_page;
	NETIF_RING_IDX req_prod;
	NETIF_RING_IDX req_cons, i;
	vaddr_t pkt_va;
	paddr_t pkt_ma;
	struct mbuf *m;

	XENPRINTF(("xennetback_evthandler "));
again:
	req_prod = xneti->xni_txring->req_prod;
	x86_lfence(); /* ensure we see all requests up to req_prod */
	req_cons = xneti->xni_txring->req_cons;
	XENPRINTF(("%s event req_prod %d resp_prod %d req_cons %d event %d\n",
	    xneti->xni_if.if_xname,
	    xneti->xni_txring->req_prod,
	    xneti->xni_txring->resp_prod,
	    xneti->xni_txring->req_cons,
	    xneti->xni_txring->event));
	for (i = 0; req_cons != req_prod; req_cons++, i++) {
		txreq =
		    &xneti->xni_txring->ring[MASK_NETIF_TX_IDX(req_cons)].req;
		XENPRINTF(("%s pkt size %d\n", xneti->xni_if.if_xname, txreq->size));
		if (__predict_false((ifp->if_flags & (IFF_UP | IFF_RUNNING)) !=
		    (IFF_UP | IFF_RUNNING))) {
			/* interface not up, drop */
			xennetback_tx_response(xneti, txreq->id,
			    NETIF_RSP_DROPPED);
			continue;
		}
		/*
		 * Do some sanity checks, and map the packet's page.
		 */
		if (__predict_false(txreq->size < ETHER_HDR_LEN ||
		   txreq->size > (ETHER_MAX_LEN - ETHER_CRC_LEN))) {
			printf("%s: packet size %d too big\n",
			    ifp->if_xname, txreq->size);
			xennetback_tx_response(xneti, txreq->id,
			    NETIF_RSP_ERROR);
			ifp->if_ierrors++;
			continue;
		}
		/* don't cross page boundaries */
		if (__predict_false(
		    (txreq->addr & PAGE_MASK) + txreq->size > PAGE_SIZE)) {
			printf("%s: packet cross page boundary\n",
			    ifp->if_xname);
			xennetback_tx_response(xneti, txreq->id,
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
			xennetback_tx_response(xneti, txreq->id,
			    NETIF_RSP_DROPPED);
			ifp->if_ierrors++;
			continue;
		}

		pkt_ma = txreq->addr  & ~PAGE_MASK;
		XENPRINTF(("%s pkt ma 0x%lx size %d id %d req_cons %d\n",
		    xneti->xni_if.if_xname, pkt_ma,
		    txreq->size, txreq->id, MASK_NETIF_TX_IDX(req_cons)));
		    
		pkt = pool_get(&xni_pkt_pool, PR_NOWAIT);
		if (__predict_false(pkt == NULL)) {
			static struct timeval lasttime;
			if (ratecheck(&lasttime, &xni_pool_errintvl))
				printf("%s: xnbpkt alloc failed\n",
				    ifp->if_xname);
			xennetback_tx_response(xneti, txreq->id,
			    NETIF_RSP_DROPPED);
			ifp->if_ierrors++;
			m_freem(m);
			continue;
		}

		pkt_hash = &xni_tx_pages_hash[
		    (pkt_ma >> PAGE_SHIFT) & XNI_PAGE_HASH_MASK];
		SLIST_FOREACH(pkt_page, pkt_hash, xni_page_next) {
			if (pkt_page->ma == pkt_ma)
				break;
		}
		if (pkt_page == NULL) {
			pkt_page = pool_get(&xni_page_pool, PR_NOWAIT);
			if (__predict_false(pkt_page == NULL)) {
				static struct timeval lasttime;
				if (ratecheck(&lasttime, &xni_pool_errintvl))
					printf("%s: xnbpa alloc failed\n",
					    ifp->if_xname);
				xennetback_tx_response(xneti, txreq->id,
				    NETIF_RSP_DROPPED);
				ifp->if_ierrors++;
				m_freem(m);
				pool_put(&xni_pkt_pool, pkt);
				continue;
			}
			pkt_page->refcount = 0;
			if (__predict_false(xen_shm_map(&pkt_ma,
			    1, xneti->domid, &pkt_va, 0) != 0)) {
				static struct timeval lasttime;
				if (ratecheck(&lasttime, &xni_pool_errintvl))
					printf("%s: can't map packet page\n",
					    ifp->if_xname);
				xennetback_tx_response(xneti, txreq->id,
				    NETIF_RSP_DROPPED);
				ifp->if_ierrors++;
				m_freem(m);
				pool_put(&xni_pkt_pool, pkt);
				pool_put(&xni_page_pool, pkt_page);
				continue;
			}
			XENPRINTF(("new pkt_page va 0x%lx mbuf %p\n",
			    pkt_va, m));
			pkt_page->ma = pkt_ma;
			pkt_page->va = pkt_va;
			SLIST_INSERT_HEAD(pkt_hash, pkt_page, xni_page_next);
		} else {
			KASSERT(pkt_page->refcount > 0);
			pkt_va = pkt_page->va;
			XENPRINTF(("pkt_page refcount %d va 0x%lx m %p\n",
			    pkt_page->refcount, pkt_va, m));
		}
		if ((ifp->if_flags & IFF_PROMISC) == 0) {
			struct ether_header *eh =
			    (void*)(pkt_va | (txreq->addr & PAGE_MASK));
			if (ETHER_IS_MULTICAST(eh->ether_dhost) == 0 &&
			    memcmp(CLLADDR(ifp->if_sadl), eh->ether_dhost,
			    ETHER_ADDR_LEN) != 0) {
				pool_put(&xni_pkt_pool, pkt);
				m_freem(m);
				if (pkt_page->refcount == 0) {
					xen_shm_unmap(pkt_page->va,
					    &pkt_page->ma, 1, xneti->domid);
					SLIST_REMOVE(pkt_hash, pkt_page,
					    xni_page, xni_page_next);
					pool_put(&xni_page_pool, pkt_page);
				}
				xennetback_tx_response(xneti, txreq->id,
				    NETIF_RSP_OKAY);
				continue; /* packet is not for us */
			}
		}

		if (MASK_NETIF_TX_IDX(req_cons + 1) ==
		    MASK_NETIF_TX_IDX(xneti->xni_txring->resp_prod)) {
			/*
			 * This is the last TX buffer. Copy the data and
			 * ack it. Delaying it until the mbuf is
			 * freed will stall transmit.
			 */
			pool_put(&xni_pkt_pool, pkt);
			m->m_len = min(MHLEN, txreq->size);
			m->m_pkthdr.len = 0;
			m_copyback(m, 0, txreq->size,
			    (void *)(pkt_va | (txreq->addr & PAGE_MASK)));
			if (pkt_page->refcount == 0) {
				xen_shm_unmap(pkt_page->va, &pkt_page->ma, 1,
				    xneti->domid);
				SLIST_REMOVE(pkt_hash, pkt_page, xni_page,
				    xni_page_next);
				pool_put(&xni_page_pool, pkt_page);
			}
			if (m->m_pkthdr.len < txreq->size) {
				ifp->if_ierrors++;
				m_freem(m);
				xennetback_tx_response(xneti, txreq->id,
				    NETIF_RSP_DROPPED);
				continue;
			}
			xennetback_tx_response(xneti, txreq->id,
			    NETIF_RSP_OKAY);
		} else {
			pkt->pkt_id = txreq->id;
			pkt->pkt_xneti = xneti;
			pkt->pkt_page = pkt_page;
			pkt_page->refcount++;

			MEXTADD(m, pkt_va | (txreq->addr & PAGE_MASK),
			    txreq->size, M_DEVBUF, xennetback_tx_free, pkt);
			m->m_pkthdr.len = m->m_len = txreq->size;
		}
		m->m_pkthdr.rcvif = ifp;
		ifp->if_ipackets++;
		
#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif
		(*ifp->if_input)(ifp, m);
	}
	xneti->xni_txring->req_cons = req_cons;
	/*
	 * make sure the guest will see our replies before testing for more
	 * work.
	 */
	x86_lfence(); /* ensure we see all requests up to req_prod */
	if (i > 0)
		goto again; /* more work to do ? */

	/* check to see if we can transmit more packets */
	softint_schedule(xneti->xni_softintr);

	return 1;
}
static void
xennetback_tx_free(struct mbuf *m, void *va, size_t size, void *arg)
{
	int s = splnet();
	struct xni_pkt *pkt = arg;
	struct xni_page *pkt_page = pkt->pkt_page;
	struct xnetback_instance *xneti = pkt->pkt_xneti;
	NETIF_RING_IDX resp_prod;
	netif_tx_response_t *txresp;
	struct xni_pages_hash *pkt_hash;

	XENPRINTF(("xennetback_tx_free ma 0x%lx refcount %d\n",
	   pkt_page->ma, pkt_page->refcount));

	resp_prod = xneti->xni_txring->resp_prod;
	XENPRINTF(("ack id %d resp_prod %d\n",
	    pkt->pkt_id, MASK_NETIF_TX_IDX(resp_prod)));
	txresp = &xneti->xni_txring->ring[MASK_NETIF_TX_IDX(resp_prod)].resp;
	txresp->id = pkt->pkt_id;
	txresp->status = NETIF_RSP_OKAY;
	x86_lfence();
	resp_prod++;
	xneti->xni_txring->resp_prod = resp_prod;
	x86_lfence();
	if (resp_prod == xneti->xni_txring->event) {
		XENPRINTF(("%s send event\n",
		    xneti->xni_if.if_xname));
		hypervisor_notify_via_evtchn(xneti->xni_evtchn);
	}
	KASSERT(pkt_page->refcount > 0);
	pkt_page->refcount--;
	pool_put(&xni_pkt_pool, pkt);

	if (pkt_page->refcount == 0) {
		xen_shm_unmap(pkt_page->va, &pkt_page->ma, 1, xneti->domid);
		pkt_hash = &xni_tx_pages_hash[
		    (pkt_page->ma >> PAGE_SHIFT) & XNI_PAGE_HASH_MASK];
		SLIST_REMOVE(pkt_hash, pkt_page, xni_page, xni_page_next);
		pool_put(&xni_page_pool, pkt_page);
	}
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
	 * before it is processed by xennet_softstart().
	 */
	softint_schedule(xneti->xni_softintr);
}

static void
xennetback_ifsoftstart(void *arg)
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
	netif_rx_request_t *rxreq;
	netif_rx_response_t *rxresp;
	NETIF_RING_IDX req_prod, resp_prod;
	int do_event = 0;

	XENPRINTF(("xennetback_ifsoftstart "));
	int s = splnet();
	if (__predict_false(
	    (ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)) {
		splx(s);
		return;
	}

	while (!IFQ_IS_EMPTY(&ifp->if_snd)) {
		XENPRINTF(("pkt\n"));
		req_prod = xneti->xni_rxring->req_prod;
		resp_prod = xneti->xni_rxring->resp_prod;
		x86_lfence();

		mmup = xstart_mmu;
		mclp = xstart_mcl;
		for (nppitems = 0, i = 0; !IFQ_IS_EMPTY(&ifp->if_snd);) {
			XENPRINTF(("have a packet\n"));
			IFQ_POLL(&ifp->if_snd, m);
			if (__predict_false(m == NULL))
				panic("xennetback_ifstart: IFQ_POLL");
			if (__predict_false(req_prod == xneti->rxreq_cons)) {
				/* out of ring space */
				XENPRINTF(("xennetback_ifstart: ring full "
				    "req_prod 0x%x req_cons 0x%x resp_prod "
				    "0x%x\n",
				    req_prod, xneti->rxreq_cons, resp_prod));
				ifp->if_timer = 1;
				break;
			}
			if (__predict_false(i == NB_XMIT_PAGES_BATCH))
				break; /* we filled the array */
			if (__predict_false(
			    xennetback_get_mcl_page(&newp_ma) != 0))
				break; /* out of memory */
			/* start filling ring */
			rxreq = &xneti->xni_rxring->ring[
			    MASK_NETIF_RX_IDX(xneti->rxreq_cons)].req;
			rxresp = &xneti->xni_rxring->ring[
			    MASK_NETIF_RX_IDX(resp_prod)].resp;
			rxresp->id = rxreq->id;
			rxresp->status = m->m_pkthdr.len;
			if ((m->m_flags & M_CLUSTER) != 0 &&
			    !M_READONLY(m) && MCLBYTES == PAGE_SIZE) {
				/* we can give this page away */
				xmit_pa = m->m_ext.ext_paddr;
				xmit_ma = xpmap_ptom(xmit_pa);
				xmit_va = (vaddr_t)m->m_ext.ext_buf;
				KASSERT(xmit_pa != M_PADDR_INVALID);
				KASSERT((xmit_va & PAGE_MASK) == 0);
				rxresp->addr =
				    xmit_ma + m->m_data - m->m_ext.ext_buf;
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
				    (void *)xmit_va);
				rxresp->addr = xmit_ma;
				pages_pool_free[nppitems].va = xmit_va;
				pages_pool_free[nppitems].pa = xmit_pa;
				nppitems++;
			}
			/*
			 * transfers the page containing the packet to the
			 * remote domain, and map newp in place.
			 */
			xpmap_phys_to_machine_mapping[
			    (xmit_pa - XPMAP_OFFSET) >> PAGE_SHIFT] =
			    newp_ma >> PAGE_SHIFT;
			mmup[0].ptr = newp_ma | MMU_MACHPHYS_UPDATE;
			mmup[0].val =
			    (xmit_pa - XPMAP_OFFSET) >> PAGE_SHIFT;
			mmup[1].ptr = MMU_EXTENDED_COMMAND;
			mmup[1].val = MMUEXT_SET_FOREIGNDOM |
			    ((int)xneti->domid << 16);
			mmup[2].ptr = xmit_ma | MMU_EXTENDED_COMMAND;
			mmup[2].val = MMUEXT_REASSIGN_PAGE;
			mclp[0].op = __HYPERVISOR_update_va_mapping;
			mclp[0].args[0] = xmit_va >> PAGE_SHIFT;
			mclp[0].args[1] =
			    newp_ma | PG_V | PG_RW | PG_U | PG_M;
			mclp[0].args[2] = UVMF_INVLPG;
			mclp[1].op = __HYPERVISOR_mmu_update;
			mclp[1].args[0] = (unsigned long)mmup;
			mclp[1].args[1] = 3;
			mclp[1].args[2] = 0;
			mmup += 3;
			mclp += 2;

			/* done with this packet */
			xneti->rxreq_cons++;
			resp_prod++;
			do_event = 1;
			IFQ_DEQUEUE(&ifp->if_snd, m);
			i++; /* this packet has been queued */
			ifp->if_opackets++;
#if NBPFILTER > 0
			if (ifp->if_bpf)
				bpf_mtap(ifp->if_bpf, m);
#endif
			mbufs_sent[i-1] = m;
		}
		if (i != 0) {
			/* update the MMU */
			if (HYPERVISOR_multicall(xstart_mcl, i * 2) != 0) {
				panic("%s: HYPERVISOR_multicall failed", 
				    ifp->if_xname);
			}
			for (j = 0; j < i * 2; j++) {
				if (xstart_mcl[j].args[5] != 0)
					printf("%s: xstart_mcl[%d] failed\n",
					    ifp->if_xname, j);
			}
			x86_lfence();
			/* update pointer */
			xneti->xni_rxring->resp_prod += i;
			x86_lfence();
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
			x86_lfence();
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
		if ((volatile NETIF_RING_IDX)(xneti->xni_rxring->req_prod) ==
		    xneti->rxreq_cons) {
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
	if (xneti->status == CONNECTED)
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
	if (xneti->status == CONNECTED) {
		XENPRINTF(("%s: req_prod 0x%x resp_prod 0x%x req_cons 0x%x "
		    "event 0x%x\n", ifp->if_xname, xneti->xni_txring->req_prod,
		    xneti->xni_txring->resp_prod, xneti->xni_txring->req_cons,
		    xneti->xni_txring->event));
		xennetback_evthandler(ifp->if_softc); /* flush pending RX requests */
	}
	splx(s);
}

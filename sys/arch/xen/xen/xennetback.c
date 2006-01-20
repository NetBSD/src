/*      $NetBSD: xennetback.c,v 1.4.2.7 2006/01/20 21:14:47 riz Exp $      */

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


#include <machine/xen.h>
#include <machine/xen_shm.h>
#include <machine/evtchn.h>
#include <machine/ctrl_if.h>

#include <machine/xen-public/io/domain_controller.h>

#include <uvm/uvm.h>

#ifdef XENDEBUG_NET
#define XENPRINTF(x) printf x
#else
#define XENPRINTF(x)
#endif

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

static int  xennetback_ifioctl(struct ifnet *, u_long, caddr_t);
static void xennetback_ifstart(struct ifnet *);
static void xennetback_ifwatchdog(struct ifnet *);
static int  xennetback_ifinit(struct ifnet *);
static void xennetback_ifstop(struct ifnet *, int);


SLIST_HEAD(, xnetback_instance) xnetback_instances;

static struct xnetback_instance *xnetif_lookup(domid_t, uint32_t);
static int  xennetback_evthandler(void *);

/* extra pages to give to forein domains on xmit */
#define NB_XMIT_PAGES_BATCH 64 /* number of pages to allocate at once */
static unsigned long xmit_pages[NB_XMIT_PAGES_BATCH]; /* our physical pages */
vaddr_t xmit_pages_vaddr_base; /* vm space we map it to */
int xmit_pages_alloc; /* current index in xmit_pages */

int  xennetback_get_xmit_page(vaddr_t *, paddr_t *);
static void xennetback_get_new_xmit_pages(void);

/* arrays used in xennetback_ifstart(), too large to allocate on stack */
static mmu_update_t xstart_mmu[NB_XMIT_PAGES_BATCH * 3];
static multicall_entry_t xstart_mcl[NB_XMIT_PAGES_BATCH * 2];

void
xennetback_init()
{
	ctrl_msg_t cmsg;
	netif_be_driver_status_t st;
	int i;
	struct vm_page *pg;
	paddr_t pa;
	multicall_entry_t mcl[NB_XMIT_PAGES_BATCH];

	if ( !(xen_start_info.flags & SIF_INITDOMAIN) &&
	     !(xen_start_info.flags & SIF_NET_BE_DOMAIN))
		return;

	XENPRINTF(("xennetback_init\n"));

	xmit_pages_vaddr_base = uvm_km_valloc(kernel_map,
	    NB_XMIT_PAGES_BATCH * PAGE_SIZE);
	xmit_pages_vaddr_base = xmit_pages_vaddr_base >> PAGE_SHIFT;
	xmit_pages_alloc = -1;
	if (xmit_pages_vaddr_base == 0)
		panic("xennetback_init: no VM space");

	XENPRINTF(("xmit_pages_vaddr_base=0x%x\n", (u_int)xmit_pages_vaddr_base));

	/* 
	 * I don't like this much, but it seems we can't get MEMOP_increase
	 * to work until we've given back a few pages first.
	 * So prime transmit pages pool with some uvm-managed pages.
	 */
	for (i = 0; i < NB_XMIT_PAGES_BATCH; i++) {
		pg = uvm_pagealloc(NULL, 0, NULL, 0);
		if (pg == NULL)
			panic("xennetback_init: no page\n");
		pa = VM_PAGE_TO_PHYS(pg);
		xmit_pages[i] = xpmap_ptom(pa) >> PAGE_SHIFT;
		xpmap_phys_to_machine_mapping[
		    (pa - XPMAP_OFFSET) >> PAGE_SHIFT] = INVALID_P2M_ENTRY;
		XENPRINTF(("got page pa 0x%x machine_pa 0x%x\n",
		    (u_int)pa, (u_int)xmit_pages[i]));
		mcl[i].op = __HYPERVISOR_update_va_mapping;
		mcl[i].args[0] = (xmit_pages_vaddr_base + i);
		mcl[i].args[1] = (xmit_pages[i] << PAGE_SHIFT) |
		    PG_V | PG_RW | PG_U | PG_M;
		mcl[i].args[2] = 0;
	}
	if (HYPERVISOR_multicall(mcl, NB_XMIT_PAGES_BATCH) != 0)
		panic("xennetback_init: HYPERVISOR_multicall");
	for (i = 0; i < NB_XMIT_PAGES_BATCH; i++) {
		if ((mcl[i].args[5] != 0)) {
			printf("xennetback_init: "
			    "mcl[%d] failed\n", i);
			panic("xennetback_init");
		}
	}
	xmit_pages_alloc = NB_XMIT_PAGES_BATCH - 1;

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
		ring_rxaddr = uvm_km_alloc(kernel_map, PAGE_SIZE);
		if (ring_rxaddr == 0) {
			printf("%s: can't alloc ring VM\n",
			    xneti->xni_if.if_xname);
			req->status = NETIF_BE_STATUS_OUT_OF_MEMORY;
			goto end;
		}
		ring_txaddr = uvm_km_alloc(kernel_map, PAGE_SIZE);
		if (ring_txaddr == 0) {
			printf("%s: can't alloc ring VM\n",
			    xneti->xni_if.if_xname);
			uvm_km_free(kernel_map, ring_rxaddr, PAGE_SIZE);
			req->status = NETIF_BE_STATUS_OUT_OF_MEMORY;
			goto end;
		}
		xneti->xni_ma_rxring = req->rx_shmem_frame << PAGE_SHIFT;
		xneti->xni_ma_txring = req->tx_shmem_frame << PAGE_SHIFT;
		error = pmap_remap_pages(pmap_kernel(), ring_rxaddr,
		   xneti->xni_ma_rxring, 1, VM_PROT_READ | VM_PROT_WRITE,
		   PMAP_WIRED | PMAP_CANFAIL, req->domid);
		if (error == 0)
			error = pmap_remap_pages(pmap_kernel(), ring_txaddr,
			   xneti->xni_ma_txring, 1,
			   VM_PROT_READ | VM_PROT_WRITE,
			   PMAP_WIRED | PMAP_CANFAIL, req->domid);
		if (error) {
			uvm_km_free(kernel_map, ring_rxaddr, PAGE_SIZE);
			uvm_km_free(kernel_map, ring_txaddr, PAGE_SIZE);
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
		ring_addr = (vaddr_t)xneti->xni_rxring;
		pmap_remove(pmap_kernel(), ring_addr, ring_addr + PAGE_SIZE);
		uvm_km_free(kernel_map, ring_addr, PAGE_SIZE);
		ring_addr = (vaddr_t)xneti->xni_txring;
		pmap_remove(pmap_kernel(), ring_addr, ring_addr + PAGE_SIZE);
		uvm_km_free(kernel_map, ring_addr, PAGE_SIZE);

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


/* get a free page to give away to a forein guest for packet transmit */
int
xennetback_get_xmit_page(vaddr_t *vap, paddr_t *map)
{
	if (xmit_pages_alloc < 0)
		/*
		 * we exhausted our allocation. We can't allocate new ones yet
		 * because the current pages may not have been loaned to
		 * the remote domain yet. We have to let the caller do this.
		 */
		return -1;

	*map = xmit_pages[xmit_pages_alloc] << PAGE_SHIFT;
	*vap = (xmit_pages_vaddr_base + xmit_pages_alloc) << PAGE_SHIFT;
	xmit_pages_alloc--;
	return 0;
}

static void
xennetback_get_new_xmit_pages(void)
{
	int i;
	multicall_entry_t mcl[NB_XMIT_PAGES_BATCH];
	int nb_pages;

	/* get some new pages */
	nb_pages = HYPERVISOR_dom_mem_op(MEMOP_increase_reservation, xmit_pages,
	    NB_XMIT_PAGES_BATCH, 0);
	if (nb_pages <= 0) {
		printf("xennetback: can't get new xmit pages (%d)\n", nb_pages);
		return;
	}
	if (nb_pages != NB_XMIT_PAGES_BATCH)
		printf("xennetback: got only %d new xmit pages\n", nb_pages);

	/* map the new pages */
	for (i = 0; i < nb_pages; i++) {
		mcl[i].op = __HYPERVISOR_update_va_mapping;
		mcl[i].args[0] = (xmit_pages_vaddr_base + i);
		mcl[i].args[1] = (xmit_pages[i] << PAGE_SHIFT) |
		    PG_V | PG_RW | PG_U | PG_M;
		mcl[i].args[2] = 0;
	}
	if (HYPERVISOR_multicall(mcl, nb_pages) != 0)
		panic("xennetback_get_new_xmit_page: HYPERVISOR_multicall");
	for (i = 0; i < nb_pages; i++) {
		if ((mcl[i].args[5] != 0)) {
			printf("xennetback_get_new_xmit_page: "
			    "mcl[%d] failed\n", i);
			panic("xennetback_get_new_xmit_page");
		}
	}
	xmit_pages_alloc = nb_pages - 1;
}


static int
xennetback_evthandler(void *arg)
{
	struct xnetback_instance *xneti = arg;
	struct ifnet *ifp = &xneti->xni_if;
	netif_tx_request_t *txreq;
	netif_tx_response_t *txresp;
	NETIF_RING_IDX req_prod;
	NETIF_RING_IDX req_cons, resp_prod, i;
	vaddr_t pkt;
	paddr_t ma;
	struct mbuf *m;
	int do_event = 0;

again:
	req_prod = xneti->xni_txring->req_prod;
	x86_lfence(); /* ensure we see all requests up to req_prod */
	resp_prod = xneti->xni_txring->resp_prod;
	req_cons = xneti->xni_txring->req_cons;
	XENPRINTF(("%s event req_prod %d resp_prod %d req_cons %d event %d\n",
	    xneti->xni_if.if_xname,
	    xneti->xni_txring->req_prod,
	    xneti->xni_txring->resp_prod,
	    xneti->xni_txring->req_cons,
	    xneti->xni_txring->event));
	for (i = 0; req_cons != req_prod;
	    req_cons++, 
	    resp_prod++, i++) {
		txreq =
		    &xneti->xni_txring->ring[MASK_NETIF_TX_IDX(req_cons)].req;
		txresp =
		    &xneti->xni_txring->ring[MASK_NETIF_TX_IDX(resp_prod)].resp;
		if (xneti->xni_txring->event == resp_prod)
			do_event = 1;

		XENPRINTF(("%s pkg size %d\n", xneti->xni_if.if_xname, txreq->size));
		if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) !=
		    (IFF_UP | IFF_RUNNING)) {
			/* interface not up, drop */
			txresp->id = txreq->id;
			txresp->status = NETIF_RSP_DROPPED;
			continue;
		}
		/*
		 * Do some sanity checks, and map the packet's page.
		 */
		if (txreq->size < ETHER_HDR_LEN ||
		   txreq->size > (ETHER_MAX_LEN - ETHER_CRC_LEN)) {
			printf("%s: packet size %d too big\n",
			    ifp->if_xname, txreq->size);
			txresp->id = txreq->id;
			txresp->status = NETIF_RSP_ERROR;
			ifp->if_ierrors++;
			continue;
		}
		/* don't cross page boundaries */
		if ((txreq->addr & PAGE_MASK) + txreq->size > PAGE_SIZE) {
			printf("%s: packet cross page boundary\n",
			    ifp->if_xname);
			txresp->id = txreq->id;
			txresp->status = NETIF_RSP_ERROR;
			ifp->if_ierrors++;
			continue;
		}

		ma = txreq->addr  & ~PAGE_MASK;
		if (xen_shm_map(&ma, 1, xneti->domid, &pkt, 0) != 0) {
			printf("%s: can't map packet page\n", ifp->if_xname);
			txresp->id = txreq->id;
			txresp->status = NETIF_RSP_ERROR;
			ifp->if_ierrors++;
			continue;
		}
		pkt |= (txreq->addr & PAGE_MASK);
		/* get a mbuf for this packet */
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m != NULL) {
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				m_freem(m);
				m = NULL;
			}
		}
		if (m == NULL) {
			txresp->id = txreq->id;
			txresp->status = NETIF_RSP_DROPPED;
			xen_shm_unmap(pkt, &ma, 1, xneti->domid);
			ifp->if_ierrors++;
			continue;
		}
		memcpy(mtod(m, caddr_t), (caddr_t)pkt, txreq->size);
		m->m_pkthdr.len = m->m_len = txreq->size;
		ifp->if_ipackets++;
		m->m_pkthdr.rcvif = ifp;
		xen_shm_unmap(pkt, &ma, 1, xneti->domid);
		txresp->id = txreq->id;
		txresp->status = NETIF_RSP_OKAY;
#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif
		(*ifp->if_input)(ifp, m);
	}
	if (xneti->xni_txring->event == resp_prod)
		do_event = 1;
	x86_lfence(); /* make sure the guest see our responses */
	xneti->xni_txring->req_cons = req_cons;
	xneti->xni_txring->resp_prod = resp_prod;
	/*
	 * make sure the guest will see our replies before testing for more
	 * work.
	 */
	x86_lfence(); /* ensure we see all requests up to req_prod */
	if (i > 0)
		goto again; /* more work to do ? */
	if (do_event) {
		XENPRINTF(("%s send event\n", xneti->xni_if.if_xname));
		hypervisor_notify_via_evtchn(xneti->xni_evtchn);
	}
	return 1;
}

static int
xennetback_ifioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
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
	struct mbuf *m;
	vaddr_t xmit_va;
	paddr_t xmit_pa;
	int i, j;
	mmu_update_t *mmup;
	multicall_entry_t *mclp;
	netif_rx_request_t *rxreq;
	netif_rx_response_t *rxresp;
	NETIF_RING_IDX req_prod, resp_prod;


	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return;

	while (!IFQ_IS_EMPTY(&ifp->if_snd)) {
		req_prod = xneti->xni_rxring->req_prod;
		resp_prod = xneti->xni_rxring->resp_prod;
		x86_lfence();

		mmup = xstart_mmu;
		mclp = xstart_mcl;
		for (i = 0; !IFQ_IS_EMPTY(&ifp->if_snd);) {
			XENPRINTF(("have a packet\n"));
			IFQ_POLL(&ifp->if_snd, m);
			if (__predict_false(m == NULL))
				panic("xennetback_ifstart: IFQ_POLL");
			if (__predict_false(req_prod == xneti->rxreq_cons)) {
				/* out of ring space */
				XENPRINTF(("xennetback_ifstart: ring full req_prod 0x%x req_cons 0x%x resp_prod 0x%x\n",
				    req_prod, xneti->rxreq_cons, resp_prod));
				ifp->if_timer = 1;
				break;
			}
			if (__predict_false(i == NB_XMIT_PAGES_BATCH))
				break; /* we filled the array */
			if (__predict_false(
			    xennetback_get_xmit_page(&xmit_va, &xmit_pa) != 0))
				break; /* out of memory */

			XENPRINTF(("xennetback_get_xmit_page: got va 0x%x pa 0x%x\n", (u_int)xmit_va, (u_int)xmit_pa));
			IFQ_DEQUEUE(&ifp->if_snd, m);
			i++; /* this packet will be queued */
#if NBPFILTER > 0
			if (ifp->if_bpf)
				bpf_mtap(ifp->if_bpf, m);
#endif
			m_copydata(m, 0, m->m_pkthdr.len, (caddr_t)xmit_va);
			/* schedule mmu update */
			mmup[0].ptr = MMU_EXTENDED_COMMAND;
			mmup[0].val =
			    MMUEXT_SET_FOREIGNDOM | ((int)xneti->domid << 16);
			mmup[1].ptr = xmit_pa | MMU_EXTENDED_COMMAND;
			mmup[1].val = MMUEXT_REASSIGN_PAGE;
			mclp[0].op = __HYPERVISOR_update_va_mapping;
			mclp[0].args[0] = xmit_va >> PAGE_SHIFT;
			mclp[0].args[1] = 0;
			mclp[0].args[2] = 0;
			mclp[1].op = __HYPERVISOR_mmu_update;
			mclp[1].args[0] = (unsigned long)mmup;
			mclp[1].args[1] = 2;
			mclp[1].args[2] = 0;
			mmup += 2;
			mclp += 2;
			/* fill in ring */
			rxreq = &xneti->xni_rxring->ring[
			    MASK_NETIF_RX_IDX(xneti->rxreq_cons)].req;
			rxresp = &xneti->xni_rxring->ring[
			    MASK_NETIF_RX_IDX(resp_prod)].resp;
			rxresp->id = rxreq->id;
			rxresp->status = m->m_pkthdr.len;
			rxresp->addr = xmit_pa;
			xneti->rxreq_cons++;
			resp_prod++;

			/* done with this packet */
			m_freem(m);
			ifp->if_opackets++;
		}
		if (i != 0) {
			/* update the MMU */
			if (HYPERVISOR_multicall(xstart_mcl, i * 2) != 0) {
				panic("%s: HYPERVISOR_multicall failed", 
				    ifp->if_xname);
			}
			for (j = 0; j < i; j++) {
				if (xstart_mcl[j].args[5] != 0)
					printf("%s: xstart_mcl[%d] failed\n",
					    ifp->if_xname, j);
			}
			x86_lfence();
			/* update pointer */
			xneti->xni_rxring->resp_prod += i;
			x86_lfence();
		}
		/* check if we need to allocate new xmit pages */
		if (xmit_pages_alloc < 0) {
			xennetback_get_new_xmit_pages();
			if (xmit_pages_alloc < 0) {
				/*
				 * setup the watchdog to try again, because 
				 * xennetback_ifstart() will never be called
				 * again if queue is full.
				 */
				printf("xennetback_ifstart: no xmit_pages\n");
				ifp->if_timer = 1;
			}
		}
		if (ifp->if_timer) {
			/* transmit is out of ressources, stop here */
			break;
		}
	}
	/* send event */
	x86_lfence();
	XENPRINTF(("%s receive event\n", xneti->xni_if.if_xname));
	hypervisor_notify_via_evtchn(xneti->xni_evtchn);
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

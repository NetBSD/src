/*	$NetBSD: if_xennet.c,v 1.3 2004/04/21 12:14:45 cl Exp $	*/

/*
 *
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christian Limpach.
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
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_xennet.c,v 1.3 2004/04/21 12:14:45 cl Exp $");

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <net/if_ether.h>

#ifdef mediacode
#include <net/if_media.h>
#endif

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#endif

#include <nfs/rpcv2.h>

#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>
#include <nfs/nfsdiskless.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>

#include <machine/xen.h>
#include <machine/hypervisor.h>
#include <machine/events.h>

#include <machine/if_xennetvar.h>

#ifdef DEBUG
#define XENNET_DEBUG
#endif
#if defined(XENNET_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif
/* #define XENNET_DEBUG_DUMP */

#ifdef XENNET_DEBUG
#define XEDB_FOLLOW	0x01
#define XEDB_INIT	0x02
#define XEDB_EVENT	0x04
#define XEDB_MBUF	0x08
#define XEDB_MEM	0x10
int xennet_debug = 0;
#define DPRINTF(x) if (xennet_debug) printf x;
#define DPRINTFN(n,x) if (xennet_debug & (n)) printf x;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif
#define PRINTF(x) printf x;

#ifdef XENNET_DEBUG_DUMP
static void xennet_hex_dump(unsigned char *, size_t);
#endif

int xennet_match (struct device *, struct cfdata *, void *);
void xennet_attach (struct device *, struct device *, void *);
static void xennet_tx_mbuf_free(struct mbuf *, caddr_t, size_t, void *);
static void xennet_rx_mbuf_free(struct mbuf *, caddr_t, size_t, void *);
static int xen_network_handler(void *);
static void network_tx_buf_gc(struct xennet_softc *);
static void network_alloc_rx_buffers(struct xennet_softc *);
static void network_alloc_tx_buffers(struct xennet_softc *);
void xennet_init(struct xennet_softc *);
void xennet_reset(struct xennet_softc *);
#ifdef mediacode
static int xennet_mediachange (struct ifnet *);
static void xennet_mediastatus(struct ifnet *, struct ifmediareq *);
#endif

CFATTACH_DECL(xennet, sizeof(struct xennet_softc),
    xennet_match, xennet_attach, NULL, NULL);

#define TX_MAX_ENTRIES (TX_RING_SIZE - 2)
#define RX_MAX_ENTRIES (RX_RING_SIZE - 2)
#define TX_ENTRIES 32
#define RX_ENTRIES 32

#define	TX_RING_INC(_i)    (((_i)+1) & (TX_RING_SIZE-1))
#define RX_RING_INC(_i)    (((_i)+1) & (RX_RING_SIZE-1))
#define TX_RING_ADD(_i,_j) (((_i)+(_j)) & (TX_RING_SIZE-1))
#define RX_RING_ADD(_i,_j) (((_i)+(_j)) & (RX_RING_SIZE-1))

#ifdef mediacode
static int xennet_media[] = {
	IFM_ETHER|IFM_AUTO,
};
static int nxennet_media = (sizeof(xennet_media)/sizeof(xennet_media[0]));
#endif

int
xennet_scan(struct device *self, struct xennet_attach_args *xneta,
    cfprint_t print)
{
	int i;

	for (i = 0; i < MAX_DOMAIN_VIFS; i++) {
		xneta->xa_netop.cmd = NETOP_RESET_RINGS;
		xneta->xa_netop.vif = i;
		if (HYPERVISOR_net_io_op(&xneta->xa_netop) != 0)
			continue;
		xneta->xa_netop.cmd = NETOP_GET_VIF_INFO;
		xneta->xa_netop.vif = i;
		if (HYPERVISOR_net_io_op(&xneta->xa_netop) != 0)
			continue;
		config_found(self, xneta, print);
	}

	return 0;
}

int
xennet_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct xennet_attach_args *xa = (struct xennet_attach_args *)aux;

	if (strcmp(xa->xa_busname, "xennet") == 0)
		return 1;
	return 0;
}

static struct xennet_softc *thesc;

void
xennet_attach(struct device *parent, struct device *self, void *aux)
{
	struct xennet_attach_args *xneta = (struct xennet_attach_args *)aux;
	struct xennet_softc *sc = (struct xennet_softc *)self;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int idx;

	aprint_normal(": Xen Virtual Network Interface\n");

	sc->sc_ifno = xneta->xa_netop.vif;

        memcpy(sc->sc_enaddr, xneta->xa_netop.u.get_vif_info.vmac,
	    ETHER_ADDR_LEN);

	/* Initialize ifnet structure. */
	memcpy(ifp->if_xname, sc->sc_dev.dv_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = xennet_start;
	ifp->if_ioctl = xennet_ioctl;
	ifp->if_watchdog = xennet_watchdog;
	ifp->if_flags = IFF_BROADCAST | IFF_NOTRAILERS;

#ifdef mediacode
	ifmedia_init(&sc->sc_media, 0, xennet_mediachange,
	    xennet_mediastatus);
	for (idx = 0; idx < nxennet_media; idx++)
		ifmedia_add(&sc->sc_media, xennet_media[idx], 0, NULL);
	ifmedia_set(&sc->sc_media, xennet_media[0]);
#endif

	sc->sc_net_ring = (net_ring_t *)
		uvm_km_valloc_align(kernel_map, PAGE_SIZE, PAGE_SIZE);
	pmap_kenter_ma((vaddr_t)sc->sc_net_ring,
	    xneta->xa_netop.u.get_vif_info.ring_mfn << PAGE_SHIFT,
	    VM_PROT_READ|VM_PROT_WRITE);
	DPRINTFN(XEDB_INIT, ("net ring va %p and wired to %p\n",
	    sc->sc_net_ring, (void *)(xneta->xa_netop.u.get_vif_info.ring_mfn
		<< PAGE_SHIFT)));
	sc->sc_net_idx = &HYPERVISOR_shared_info->net_idx[sc->sc_ifno];

	for (idx = 0; idx < TX_RING_SIZE; idx++)
		sc->sc_tx_bufa[idx].xb_next = idx + 1;
	for (idx = 0; idx < RX_RING_SIZE; idx++)
		sc->sc_rx_bufa[idx].xb_next = idx + 1;

	network_alloc_rx_buffers(sc);
	SLIST_INIT(&sc->sc_tx_bufs);
	network_alloc_tx_buffers(sc);

	if_attach(ifp);
	ether_ifattach(ifp, sc->sc_enaddr);

	thesc = sc;

	event_set_handler(_EVENT_NET, &xen_network_handler, IPL_NET);
	hypervisor_enable_event(_EVENT_NET);

        aprint_normal("%s: MAC address %s\n", sc->sc_dev.dv_xname,
            ether_sprintf(sc->sc_enaddr));

#if NRND > 0
	rnd_attach_source(&sc->rnd_source, sc->sc_dev.dv_xname,
	    RND_TYPE_NET, 0);
#endif

}

static void
xennet_tx_mbuf_free(struct mbuf *m, caddr_t buf, size_t size, void *arg)
{
	struct xennet_txbuf *txbuf = (struct xennet_txbuf *)arg;

	DPRINTFN(XEDB_MBUF, ("xennet_tx_mbuf_free %p pa %p\n", txbuf,
	    (void *)txbuf->xt_pa));
	SLIST_INSERT_HEAD(&txbuf->xt_sc->sc_tx_bufs, txbuf, xt_next);
	pool_cache_put(&mbpool_cache, m);
}

static void
xennet_rx_mbuf_free(struct mbuf *m, caddr_t buf, size_t size, void *arg)
{
	union xennet_bufarray *xb = (union xennet_bufarray *)arg;
	struct xennet_softc *sc = xb->xb_rx.xbrx_sc;
	netop_t netop;
	int id = (xb - sc->sc_rx_bufa), ringidx;

	DPRINTFN(XEDB_MBUF, ("xennet_rx_mbuf_free id %d, mbuf %p, buf %p, "
	    "size %d\n", id, m, buf, size));
	KASSERT(sc == thesc);

	ringidx = sc->sc_net_idx->rx_req_prod;

	DPRINTFN(XEDB_MEM, ("adding ptp %p/%p for page va %p ma %p to rx_ring "
	    "at %d with id %d\n", (void *)sc->sc_rx_bufa[id].xb_rx.xbrx_ptpa,
	    (void *)xpmap_ptom(sc->sc_rx_bufa[id].xb_rx.xbrx_ptpa),
	    (void *)sc->sc_rx_bufa[id].xb_rx.xbrx_va,
	    (void *)(PTE_BASE[x86_btop (sc->sc_rx_bufa[id].xb_rx.xbrx_va)] &
		PG_FRAME), ringidx, id));

	sc->sc_net_ring->rx_ring[ringidx].req.id = id;
	sc->sc_net_ring->rx_ring[ringidx].req.addr =
		xpmap_ptom(sc->sc_rx_bufa[id].xb_rx.xbrx_ptpa);
	sc->sc_rx_bufs_to_notify++;
	ringidx = RX_RING_INC(ringidx);
	sc->sc_net_idx->rx_req_prod = ringidx;

	pool_cache_put(&mbpool_cache, m);

	/* Batch Xen notifications. */
	if (sc->sc_rx_bufs_to_notify > (RX_ENTRIES / 4)) {
		netop.cmd = NETOP_PUSH_BUFFERS;
		netop.vif = sc->sc_ifno;
		(void)HYPERVISOR_net_io_op(&netop);
		sc->sc_rx_bufs_to_notify = 0;
	}
}

static int
xen_network_handler(void *arg)
{
	struct xennet_softc *sc = thesc;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	rx_resp_entry_t *rx;
	paddr_t pa;
	int ringidx;
	struct mbuf *m;

	network_tx_buf_gc(sc);

 again:
	for (ringidx = sc->sc_rx_resp_cons;
	     ringidx != sc->sc_net_idx->rx_resp_prod;
	     ringidx = RX_RING_INC(ringidx)) {
		rx  = &sc->sc_net_ring->rx_ring[ringidx].resp;

                MGETHDR(m, M_DONTWAIT, MT_DATA);
                if (m == NULL) {
			printf("xennet: rx no mbuf\n");
			break;
		}

		pa = PTE_BASE[x86_btop(sc->sc_rx_bufa[rx->id].xb_rx.xbrx_va)] &
			PG_FRAME;
		xpmap_phys_to_machine_mapping[
			machine_to_phys_mapping[pa >> PAGE_SHIFT]] =
			pa >> PAGE_SHIFT;

		DPRINTFN(XEDB_EVENT, ("rx event %d for id %d, size %d, "
		    "status %d, offset %d\n", ringidx, rx->id, rx->size,
		    rx->status, rx->offset));
		DPRINTFN(XEDB_MBUF, ("rx packet mbuf %p ptp %p va %p pa %p "
		    "ma %p\n", m,
		    (void *)sc->sc_rx_bufa[rx->id].xb_rx.xbrx_ptpa,
		    (void *)sc->sc_rx_bufa[rx->id].xb_rx.xbrx_va,
		    (void *)(xpmap_mtop(PTE_BASE[x86_btop
			(sc->sc_rx_bufa[rx->id].xb_rx.xbrx_va)] & PG_FRAME)),
		    (void *)(PTE_BASE[x86_btop
			(sc->sc_rx_bufa[rx->id].xb_rx.xbrx_va)] & PG_FRAME)));

		MEXTADD(m, (void *)(sc->sc_rx_bufa[rx->id].xb_rx.xbrx_va +
		    rx->offset), rx->size, M_DEVBUF, xennet_rx_mbuf_free,
		    &sc->sc_rx_bufa[rx->id]);
		m->m_len = m->m_pkthdr.len = rx->size;
		m->m_pkthdr.rcvif = ifp;

#ifdef XENNET_DEBUG_DUMP
		xennet_hex_dump(mtod(m, u_char *), m->m_pkthdr.len);
#endif

#if NBPFILTER > 0
		/*
		 * Pass packet to bpf if there is a listener.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif

		ifp->if_ipackets++;

		/* Pass the packet up. */
		(*ifp->if_input)(ifp, m);
	}

	sc->sc_rx_resp_cons = ringidx;
	sc->sc_net_idx->rx_event = RX_RING_INC(sc->sc_rx_resp_cons);

	if (sc->sc_net_idx->rx_resp_prod != ringidx)
		goto again;

	return 0;
}

static inline int
get_bufarray_entry(union xennet_bufarray *a)
{
	int idx;

	idx = a[0].xb_next;
	a[0].xb_next = a[idx].xb_next;
	return idx;
}

static inline void
put_bufarray_entry(union xennet_bufarray *a, int idx)
{

	a[idx].xb_next = a[0].xb_next;
	a[0].xb_next = idx;
}

static void
network_tx_buf_gc(struct xennet_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	tx_entry_t *tx_ring = sc->sc_net_ring->tx_ring;
	int idx, prod;

	do {
		prod = sc->sc_net_idx->tx_resp_prod;

		for (idx = sc->sc_tx_resp_cons; idx != prod;
		     idx = TX_RING_INC(idx)) {
			DPRINTFN(XEDB_EVENT, ("tx event at pos %d, status: "
			    "%d, id: %d, mbuf %p\n", idx,
			    tx_ring[idx].resp.status, tx_ring[idx].resp.id,
			    M_BUFADDR
			    (sc->sc_tx_bufa[tx_ring[idx].resp.id].xb_m)));
			m_freem(sc->sc_tx_bufa[tx_ring[idx].resp.id].xb_m);
			put_bufarray_entry(sc->sc_tx_bufa,
			    tx_ring[idx].resp.id);
			sc->sc_tx_entries--; /* atomic */
		}

		sc->sc_tx_resp_cons = prod;

		/*
		 * Set a new event, then check for race with update of
		 * tx_cons.
		 */
		sc->sc_net_idx->tx_event = /* atomic */
			TX_RING_ADD(prod, (sc->sc_tx_entries >> 1) + 1);
		__insn_barrier();
	} while (prod != sc->sc_net_idx->tx_resp_prod);

	if (sc->sc_net_idx->tx_resp_prod == sc->sc_net_idx->tx_req_prod)
		ifp->if_timer = 0;
	/* KDASSERT(sc->sc_net_idx->tx_req_prod == */
	/* TX_RING_ADD(sc->sc_net_idx->tx_resp_prod, sc->sc_tx_entries)); */
}

static void
network_alloc_rx_buffers(struct xennet_softc *sc)
{
	vaddr_t rxpages, va;
	paddr_t ptpa;
	struct vm_page *pg;
#if 0
	netop_t netop;
#endif
	int end, ringidx, id;

	end = RX_RING_ADD(sc->sc_rx_resp_cons, RX_MAX_ENTRIES);    

	if ((ringidx = sc->sc_net_idx->rx_req_prod) == end)
		return;

	rxpages = uvm_km_valloc_align(kernel_map, RX_ENTRIES * PAGE_SIZE,
	    PAGE_SIZE);
	for (va = rxpages; va < rxpages + RX_ENTRIES * PAGE_SIZE;
	     va += PAGE_SIZE) {
		pg = uvm_pagealloc(NULL, 0, NULL, 0);
		if (pg == NULL)
			panic("network_alloc_rx_buffers: no pages");
		pmap_kenter_pa(va, VM_PAGE_TO_PHYS(pg),
		    VM_PROT_READ | VM_PROT_WRITE);

		id = get_bufarray_entry(sc->sc_rx_bufa);
		sc->sc_rx_bufa[id].xb_rx.xbrx_va = va;
		sc->sc_rx_bufa[id].xb_rx.xbrx_sc = sc;

		if (pmap_extract(pmap_kernel(),
			(vaddr_t)&PTE_BASE[x86_btop(va)], &ptpa) != TRUE)
			panic("pmap_extract failed");
		DPRINTFN(XEDB_MEM, ("adding ptp %p/%p for page va %p pa %p "
		    "ma %p to rx_ring at %d with id %d\n", (void *)ptpa,
		    (void *)xpmap_ptom(ptpa), (void *)va,
		    (void *)(VM_PAGE_TO_PHYS(pg) & PG_FRAME),
		    (void *)(PTE_BASE[x86_btop(va)] & PG_FRAME),
		    ringidx, id));
		sc->sc_rx_bufa[id].xb_rx.xbrx_ptpa = ptpa;
		sc->sc_net_ring->rx_ring[ringidx].req.id = id;
		sc->sc_net_ring->rx_ring[ringidx].req.addr = xpmap_ptom(ptpa);
		sc->sc_rx_bufs_to_notify++;

		ringidx = RX_RING_INC(ringidx);
		if (ringidx == end)
			break;
	}

	sc->sc_net_idx->rx_req_prod = ringidx;
	sc->sc_net_idx->rx_event = RX_RING_INC(sc->sc_rx_resp_cons);
        DPRINTFN(XEDB_EVENT, ("expecting rx event at %d\n",
	    sc->sc_net_idx->rx_event));

#if 0
	/* Batch Xen notifications. */
	if (sc->sc_rx_bufs_to_notify > (RX_ENTRIES / 4)) {
		netop.cmd = NETOP_PUSH_BUFFERS;
		netop.vif = sc->sc_ifno;
		(void)HYPERVISOR_net_io_op(&netop);
		sc->sc_rx_bufs_to_notify = 0;
	}
#endif
}

static void
network_alloc_tx_buffers(struct xennet_softc *sc)
{
	vaddr_t txpages, va;
	struct vm_page *pg;
	struct xennet_txbuf *txbuf;
	int i;

	txpages = uvm_km_valloc_align(kernel_map,
	    (TX_ENTRIES / TXBUF_PER_PAGE) * PAGE_SIZE, PAGE_SIZE);
	for (va = txpages;
	     va < txpages + (TX_ENTRIES / TXBUF_PER_PAGE) * PAGE_SIZE;
	     va += PAGE_SIZE) {
		pg = uvm_pagealloc(NULL, 0, NULL, 0);
		if (pg == NULL)
			panic("network_alloc_tx_buffers: no pages");
		pmap_kenter_pa(va, VM_PAGE_TO_PHYS(pg),
		    VM_PROT_READ | VM_PROT_WRITE);

		for (i = 0; i < TXBUF_PER_PAGE; i++) {
			txbuf = (struct xennet_txbuf *)
				(va + i * (PAGE_SIZE / TXBUF_PER_PAGE));
			txbuf->xt_sc = sc;
			txbuf->xt_pa = VM_PAGE_TO_PHYS(pg) +
				i * (PAGE_SIZE / TXBUF_PER_PAGE) +
				sizeof(struct xennet_txbuf);
			SLIST_INSERT_HEAD(&sc->sc_tx_bufs, txbuf, xt_next);
		}
	}
}

/* 
 * Called at splnet.
 */
void
xennet_start(struct ifnet *ifp)
{
	struct xennet_softc *sc = ifp->if_softc;
	struct mbuf *m, *new_m;
	struct xennet_txbuf *txbuf;
	netop_t netop;
	paddr_t pa;
	int idx, bufid;

	DPRINTFN(XEDB_FOLLOW, ("%s: xennet_start()\n", sc->sc_dev.dv_xname));

#ifdef DIAGNOSTIC
	IFQ_POLL(&ifp->if_snd, m);
	if (m == 0)
		panic("%s: No packet to start", sc->sc_dev.dv_xname);
#endif

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	while (/*CONSTCOND*/1) {
		idx = sc->sc_net_idx->tx_req_prod;

		IFQ_POLL(&ifp->if_snd, m);
		if (m == NULL)
			break;

		switch (m->m_flags & (M_EXT|M_EXT_CLUSTER)) {
		case M_EXT|M_EXT_CLUSTER:
			pa = m->m_ext.ext_paddr +
				(m->m_data - m->m_ext.ext_buf);
			break;
		default:
		case 0:
			pa = m->m_paddr + M_BUFOFFSET(m) +
				(m->m_data - M_BUFADDR(m));
			break;
		}

		if (m->m_pkthdr.len != m->m_len ||
		    (pa ^ (pa + m->m_pkthdr.len)) & PG_FRAME) {
			txbuf = SLIST_FIRST(&sc->sc_tx_bufs);
			if (txbuf == NULL) {
				printf("xennet: no tx bufs\n");
				break;
			}

			MGETHDR(new_m, M_DONTWAIT, MT_DATA);
			if (new_m == NULL) {
				printf("xennet: no mbuf\n");
				break;
			}

			SLIST_REMOVE_HEAD(&sc->sc_tx_bufs, xt_next);
			IFQ_DEQUEUE(&ifp->if_snd, m);

			KASSERT(m->m_flags & M_PKTHDR);
			M_COPY_PKTHDR(new_m, m);
			m_copydata(m, 0, m->m_pkthdr.len, txbuf->xt_buf);
			MEXTADD(new_m, txbuf->xt_buf, m->m_pkthdr.len,
			    M_DEVBUF, xennet_tx_mbuf_free, txbuf);
			new_m->m_ext.ext_paddr = txbuf->xt_pa;
			new_m->m_len = new_m->m_pkthdr.len = m->m_pkthdr.len;

			m_freem(m);
			m = new_m;

			pa = m->m_ext.ext_paddr +
				(m->m_data - m->m_ext.ext_buf);
		} else
			IFQ_DEQUEUE(&ifp->if_snd, m);

		bufid = get_bufarray_entry(sc->sc_tx_bufa);
		sc->sc_tx_bufa[bufid].xb_m = m;

		idx = sc->sc_net_idx->tx_req_prod;
		sc->sc_net_ring->tx_ring[idx].req.id = bufid;
		sc->sc_net_ring->tx_ring[idx].req.addr =
			(xpmap_ptom(pa) & PG_FRAME) | (pa & ~PG_FRAME);
		sc->sc_net_ring->tx_ring[idx].req.size = m->m_pkthdr.len;
		sc->sc_net_idx->tx_req_prod = TX_RING_INC(idx);
		sc->sc_tx_entries++; /* XXX atomic */

#ifdef XENNET_DEBUG
		DPRINTFN(XEDB_MEM, ("packet addr %p/%p, physical %p/%p, "
		    "m_paddr %p, len %d/%d\n", M_BUFADDR(m), mtod(m, void *),
		    (void *)*kvtopte(mtod(m, vaddr_t)),
		    (void *)xpmap_mtop(*kvtopte(mtod(m, vaddr_t))),
		    (void *)m->m_paddr, m->m_pkthdr.len, m->m_len));
#endif
#ifdef XENNET_DEBUG_DUMP
		xennet_hex_dump(mtod(m, u_char *), m->m_pkthdr.len);
#endif

#if NBPFILTER > 0
		/*
		 * Pass packet to bpf if there is a listener.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif
	}

	ifp->if_flags &= ~IFF_OACTIVE;

	network_tx_buf_gc(sc);

	__insn_barrier();
	if (sc->sc_net_idx->tx_resp_prod != idx) {
		netop.cmd = NETOP_PUSH_BUFFERS;
		netop.vif = sc->sc_ifno;
		(void)HYPERVISOR_net_io_op(&netop);
	}

	ifp->if_timer = 5;

	ifp->if_opackets++;

	DPRINTFN(XEDB_FOLLOW, ("%s: xennet_start() done\n",
	    sc->sc_dev.dv_xname));
}

int
xennet_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct xennet_softc *sc = ifp->if_softc;
	struct ifaddr *ifa = (struct ifaddr *)data;
#ifdef mediacode
	struct ifreq *ifr = (struct ifreq *)data;
#endif
	int s, error = 0;

	s = splnet();

	DPRINTFN(XEDB_FOLLOW, ("%s: xennet_ioctl()\n", sc->sc_dev.dv_xname));

	switch(cmd) {
	case SIOCSIFADDR:
		DPRINTFN(XEDB_FOLLOW, ("%s: xennet_ioctl() SIOCSIFADDR\n",
		    sc->sc_dev.dv_xname));
		ifp->if_flags |= IFF_UP;
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			xennet_init(sc);
			arp_ifinit(ifp, ifa);
			break;
#endif
		default:
			xennet_init(sc);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		DPRINTFN(XEDB_FOLLOW, ("%s: xennet_ioctl() SIOCSIFFLAGS\n",
		    sc->sc_dev.dv_xname));
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		DPRINTFN(XEDB_FOLLOW, ("%s: xennet_ioctl() SIOC*MULTI\n",
		    sc->sc_dev.dv_xname));
		break;

#ifdef mediacode
	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		DPRINTFN(XEDB_FOLLOW, ("%s: xennet_ioctl() SIOC*IFMEDIA\n",
		    sc->sc_dev.dv_xname));
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_media, cmd);
		break;
#endif

	default:
		DPRINTFN(XEDB_FOLLOW, ("%s: xennet_ioctl(0x%lx) unknown cmd\n",
		    sc->sc_dev.dv_xname, cmd));
		error = EINVAL;
		break;
	}

	splx(s);

	DPRINTFN(XEDB_FOLLOW, ("%s: xennet_ioctl() returning %d\n",
	    sc->sc_dev.dv_xname, error));

	return error;
}

void
xennet_watchdog(struct ifnet *ifp)
{

	panic("xennet_watchdog\n");
}

void
xennet_init(struct xennet_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	DPRINTFN(XEDB_FOLLOW, ("%s: xennet_init()\n", sc->sc_dev.dv_xname));

	if (ifp->if_flags & IFF_UP) {

		if ((ifp->if_flags & IFF_RUNNING) == 0)
			xennet_reset(sc);

		sc->sc_tx_resp_cons = 0;

		if ((ifp->if_flags & IFF_RUNNING) == 0) {
			ifp->if_flags |= IFF_RUNNING;
			ifp->if_flags &= ~IFF_OACTIVE;
			ifp->if_timer = 0;
		}
	} else
		xennet_reset(sc);
}

void
xennet_reset(struct xennet_softc *sc)
{

	DPRINTFN(XEDB_FOLLOW, ("%s: xennet_reset()\n", sc->sc_dev.dv_xname));
}


#ifdef mediacode
/*
 * Media change callback.
 */
static int
xennet_mediachange(struct ifnet *ifp)
{
	struct xennet_softc *sc = ifp->if_softc;

	switch IFM_SUBTYPE(sc->sc_media.ifm_media) {
	case IFM_AUTO:
		break;
	default:
		return (1);
		break;
	}

	return (0);
}

/*
 * Media status callback.
 */
static void
xennet_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct xennet_softc *sc = ifp->if_softc;
	
	if (IFM_SUBTYPE(ifmr->ifm_active) == IFM_AUTO)
		ifmr->ifm_active = sc->sc_media.ifm_cur->ifm_data;

	ifmr->ifm_status &= ~IFM_AVALID;
}
#endif

int
xennet_bootstatic_callback(struct nfs_diskless *nd)
{
	struct ifnet *ifp = nd->nd_ifp;
	struct xennet_softc *sc = (struct xennet_softc *)ifp->if_softc;
	struct xen_netinfo xi;
	struct sockaddr_in *sin;

	memset(&xi, 0, sizeof(xi));
	xi.xi_ifno = sc->sc_ifno;
	xi.xi_root = nd->nd_root.ndm_host;
	xen_parse_cmdline(NULL, &xi);

	nd->nd_myip.s_addr = ntohl(xi.xi_ip[0]);
	nd->nd_gwip.s_addr = ntohl(xi.xi_ip[2]);
	nd->nd_mask.s_addr = ntohl(xi.xi_ip[3]);

	sin = (struct sockaddr_in *) &nd->nd_root.ndm_saddr;
	memset((caddr_t)sin, 0, sizeof(*sin));
	sin->sin_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = ntohl(xi.xi_ip[1]);

	return (NFS_BOOTSTATIC_HAS_MYIP|NFS_BOOTSTATIC_HAS_GWIP|
	    NFS_BOOTSTATIC_HAS_MASK|NFS_BOOTSTATIC_HAS_SERVADDR|
	    NFS_BOOTSTATIC_HAS_SERVER);
}


#ifdef XENNET_DEBUG_DUMP
#define XCHR(x) "0123456789abcdef"[(x) & 0xf]
static void
xennet_hex_dump(unsigned char *pkt, size_t len)
{
	size_t i, j;

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

/*	$NetBSD: if_xennet.c,v 1.13.2.16 2006/01/05 05:28:11 riz Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: if_xennet.c,v 1.13.2.16 2006/01/05 05:28:11 riz Exp $");

#include "opt_inet.h"
#include "opt_nfs_boot.h"
#include "rnd.h"

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

#if defined(NFS_BOOT_BOOTSTATIC)
#include <nfs/rpcv2.h>

#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>
#include <nfs/nfsdiskless.h>
#endif /* defined(NFS_BOOT_BOOTSTATIC) */

#include "bpfilter.h"
#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>

#include <machine/xen.h>
#include <machine/hypervisor.h>
#include <machine/evtchn.h>
#include <machine/ctrl_if.h>

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
int xennet_debug = 0x0;
#define DPRINTF(x) if (xennet_debug) printk x;
#define DPRINTFN(n,x) if (xennet_debug & (n)) printk x;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif
#define PRINTF(x) printf x;

#ifdef XENNET_DEBUG_DUMP
static void xennet_hex_dump(unsigned char *, size_t, char *, int);
#endif

int xennet_match (struct device *, struct cfdata *, void *);
void xennet_attach (struct device *, struct device *, void *);
static void xennet_ctrlif_rx(ctrl_msg_t *, unsigned long);
static int xennet_driver_count_connected(void);
static void xennet_driver_status_change(netif_fe_driver_status_t *);
static void xennet_interface_status_change(netif_fe_interface_status_t *);
static void xennet_tx_mbuf_free(struct mbuf *, caddr_t, size_t, void *);
static void xennet_rx_mbuf_free(struct mbuf *, caddr_t, size_t, void *);
static int xen_network_handler(void *);
static void network_tx_buf_gc(struct xennet_softc *);
static void network_alloc_rx_buffers(struct xennet_softc *);
static void network_alloc_tx_buffers(struct xennet_softc *);
int  xennet_init(struct ifnet *);
void xennet_stop(struct ifnet *, int);
void xennet_reset(struct xennet_softc *);
#ifdef mediacode
static int xennet_mediachange (struct ifnet *);
static void xennet_mediastatus(struct ifnet *, struct ifmediareq *);
#endif

CFATTACH_DECL(xennet, sizeof(struct xennet_softc),
    xennet_match, xennet_attach, NULL, NULL);

#define TX_MAX_ENTRIES (NETIF_TX_RING_SIZE - 2)
#define RX_MAX_ENTRIES (NETIF_RX_RING_SIZE - 2)
#define TX_ENTRIES 128
#define RX_ENTRIES 128

static unsigned long rx_pfn_array[NETIF_RX_RING_SIZE];
static multicall_entry_t rx_mcl[NETIF_RX_RING_SIZE+1];
static mmu_update_t rx_mmu[NETIF_RX_RING_SIZE];

/** Network interface info. */
struct xennet_ctrl {
	/** Number of interfaces. */
	int xc_interfaces;
	/** Number of connected interfaces. */
	int xc_connected;
	/** Error code. */
	int xc_err;
	/** Driver status. */
	int xc_up;

	cfprint_t xc_cfprint;
	struct device *xc_parent;
};

static struct xennet_ctrl netctrl = { -1, 0, 0 };

#ifdef mediacode
static int xennet_media[] = {
	IFM_ETHER|IFM_AUTO,
};
static int nxennet_media = (sizeof(xennet_media)/sizeof(xennet_media[0]));
#endif

int in_autoconf = 0;


int
xennet_scan(struct device *self, struct xennet_attach_args *xneta,
    cfprint_t print)
{
	ctrl_msg_t cmsg;
	netif_fe_driver_status_t st;

	if ((xen_start_info.flags & SIF_INITDOMAIN) ||
	    (xen_start_info.flags & SIF_NET_BE_DOMAIN))
		return 0;

	netctrl.xc_parent = self;
	netctrl.xc_cfprint = print;

	printf("Initialising Xen virtual ethernet frontend driver.\n");

	(void)ctrl_if_register_receiver(CMSG_NETIF_FE, xennet_ctrlif_rx,
	    CALLBACK_IN_BLOCKING_CONTEXT);

	/* Send a driver-UP notification to the domain controller. */
	cmsg.type      = CMSG_NETIF_FE;
	cmsg.subtype   = CMSG_NETIF_FE_DRIVER_STATUS;
	cmsg.length    = sizeof(netif_fe_driver_status_t);
	st.status      = NETIF_DRIVER_STATUS_UP;
	st.max_handle  = 0;
	memcpy(cmsg.msg, &st, sizeof(st));
	ctrl_if_send_message_block(&cmsg, NULL, 0, 0);
	in_autoconf = 1;
	config_pending_incr();

	return 0;
}

int
xennet_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct xennet_attach_args *xa = (struct xennet_attach_args *)aux;

	if (strcmp(xa->xa_device, "xennet") == 0)
		return 1;
	return 0;
}

void
xennet_attach(struct device *parent, struct device *self, void *aux)
{
	struct xennet_attach_args *xneta = (struct xennet_attach_args *)aux;
	struct xennet_softc *sc = (struct xennet_softc *)self;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int idx;

	aprint_normal(": Xen Virtual Network Interface\n");

	sc->sc_ifno = xneta->xa_handle;

	/* Initialize ifnet structure. */
	memcpy(ifp->if_xname, sc->sc_dev.dv_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = xennet_start;
	ifp->if_ioctl = xennet_ioctl;
	ifp->if_watchdog = xennet_watchdog;
	ifp->if_init = xennet_init;
	ifp->if_stop = xennet_stop;
	ifp->if_flags =
	    IFF_BROADCAST|IFF_SIMPLEX|IFF_NOTRAILERS|IFF_MULTICAST;
	ifp->if_timer = 0;
	IFQ_SET_READY(&ifp->if_snd);

#ifdef mediacode
	ifmedia_init(&sc->sc_media, 0, xennet_mediachange,
	    xennet_mediastatus);
	for (idx = 0; idx < nxennet_media; idx++)
		ifmedia_add(&sc->sc_media, xennet_media[idx], 0, NULL);
	ifmedia_set(&sc->sc_media, xennet_media[0]);
#endif

	for (idx = 0; idx < NETIF_TX_RING_SIZE; idx++)
		sc->sc_tx_bufa[idx].xb_next = idx + 1;
	for (idx = 0; idx < NETIF_RX_RING_SIZE; idx++)
		sc->sc_rx_bufa[idx].xb_next = idx + 1;
}

static struct xennet_softc *
find_device(int handle)
{
	struct device *dv;
	struct xennet_softc *xs = NULL;

	for (dv = alldevs.tqh_first; dv != NULL; dv = dv->dv_list.tqe_next) {
		if (dv->dv_cfattach == NULL ||
		    dv->dv_cfattach->ca_attach != xennet_attach)
			continue;
		xs = (struct xennet_softc *)dv;
		if (xs->sc_ifno == handle)
			break;
	}
	return dv ? xs : NULL;
}

static void
xennet_ctrlif_rx(ctrl_msg_t *msg, unsigned long id)
{
	int respond = 1;

	DPRINTFN(XEDB_EVENT, ("> ctrlif_rx=%d\n", msg->subtype));
	switch (msg->subtype) {
	case CMSG_NETIF_FE_INTERFACE_STATUS:
		if (msg->length != sizeof(netif_fe_interface_status_t))
			goto error;
		xennet_interface_status_change(
			(netif_fe_interface_status_t *)&msg->msg[0]);
		break;

	case CMSG_NETIF_FE_DRIVER_STATUS:
		if (msg->length != sizeof(netif_fe_driver_status_t))
			goto error;
		xennet_driver_status_change(
			(netif_fe_driver_status_t *)&msg->msg[0]);
		break;

	error:
	default:
		msg->length = 0;
		break;
	}

	if (respond)
		ctrl_if_send_response(msg);
}

static void
xennet_driver_status_change(netif_fe_driver_status_t *status)
{

	DPRINTFN(XEDB_EVENT, ("xennet_driver_status_change(%d)\n",
		     status->status));

	netctrl.xc_up = status->status;
	xennet_driver_count_connected();
}

static int
xennet_driver_count_connected(void)
{
	struct device *dv;
	struct xennet_softc *xs = NULL;

	netctrl.xc_interfaces = netctrl.xc_connected = 0;
	for (dv = alldevs.tqh_first; dv != NULL; dv = dv->dv_list.tqe_next) {
		if (dv->dv_cfattach == NULL ||
		    dv->dv_cfattach->ca_attach != xennet_attach)
			continue;
		xs = (struct xennet_softc *)dv;
		netctrl.xc_interfaces++;
		if (xs->sc_backend_state == BEST_CONNECTED)
			netctrl.xc_connected++;
	}

	return netctrl.xc_connected;
}

static void
xennet_interface_status_change(netif_fe_interface_status_t *status)
{
	ctrl_msg_t cmsg;
	netif_fe_interface_connect_t up;
	struct xennet_softc *sc;
	struct ifnet *ifp;
	struct xennet_attach_args xneta;

	DPRINTFN(XEDB_EVENT, ("xennet_interface_status_change(%d,%d,%02x:%02x:%02x:%02x:%02x:%02x)\n",
	    status->status,
	    status->handle,
	    status->mac[0], status->mac[1], status->mac[2],
	    status->mac[3], status->mac[4], status->mac[5]));

	sc = find_device(status->handle);
	if (sc == NULL) {
		xneta.xa_device = "xennet";
		xneta.xa_handle = status->handle;
		config_found_ia(netctrl.xc_parent, "xendevbus", &xneta,
		    netctrl.xc_cfprint);
		sc = find_device(status->handle);
		if (sc == NULL) {
			if (in_autoconf) {
				in_autoconf = 0;
				config_pending_decr();
			}
			printf("Status change: invalid netif handle %u\n",
			    status->handle);
			return;
		}
	}
	ifp = &sc->sc_ethercom.ec_if;

	DPRINTFN(XEDB_EVENT, ("xennet_interface_status_change(%d,%p,%02x:%02x:%02x:%02x:%02x:%02x)\n",
		     status->handle, sc,
		     status->mac[0], status->mac[1], status->mac[2],
		     status->mac[3], status->mac[4], status->mac[5]));

	switch (status->status) {
	case NETIF_INTERFACE_STATUS_CLOSED:
		printf("Unexpected netif-CLOSED message in state %d\n",
		    sc->sc_backend_state);
		break;

	case NETIF_INTERFACE_STATUS_DISCONNECTED:
#if 0
		if (sc->sc_backend_state != BEST_CLOSED) {
			printk("Unexpected netif-DISCONNECTED message"
			    " in state %d\n", sc->sc_backend_state);
			printk("Attempting to reconnect network interface\n");

			/* Begin interface recovery.
			 *
			 * NB. Whilst we're recovering, we turn the
			 * carrier state off.  We take measures to
			 * ensure that this device isn't used for
			 * anything.  We also stop the queue for this
			 * device.  Various different approaches
			 * (e.g. continuing to buffer packets) have
			 * been tested but don't appear to improve the
			 * overall impact on TCP connections.
			 *
			 * TODO: (MAW) Change the Xend<->Guest
			 * protocol so that a recovery is initiated by
			 * a special "RESET" message - disconnect
			 * could just mean we're not allowed to use
			 * this interface any more.
			 */

			/* Stop old i/f to prevent errors whilst we
			 * rebuild the state. */
			spin_lock_irq(&np->tx_lock);
			spin_lock(&np->rx_lock);
			netif_stop_queue(dev);
			np->backend_state = BEST_DISCONNECTED;
			spin_unlock(&np->rx_lock);
			spin_unlock_irq(&np->tx_lock);

			/* Free resources. */
			unbind_evtchn_from_irq(np->evtchn);
			event_remove_handler(np->evtchn,
			    &xen_network_handler, sc);
			free_page((unsigned long)np->tx);
			free_page((unsigned long)np->rx);
		}
#endif

		if (sc->sc_backend_state == BEST_CLOSED) {
			/* Move from CLOSED to DISCONNECTED state. */
			sc->sc_tx = (netif_tx_interface_t *)
				uvm_km_valloc_align(kernel_map, PAGE_SIZE, PAGE_SIZE);
			if (sc->sc_tx == NULL)
				panic("netif: no tx va");
			sc->sc_rx = (netif_rx_interface_t *)
				uvm_km_valloc_align(kernel_map, PAGE_SIZE, PAGE_SIZE);
			if (sc->sc_rx == NULL)
				panic("netif: no rx va");
			sc->sc_pg_tx = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_ZERO);
			if (sc->sc_pg_tx == NULL) {
				panic("netif: no tx pages");
			}
			pmap_kenter_pa((vaddr_t)sc->sc_tx, VM_PAGE_TO_PHYS(sc->sc_pg_tx),
			    VM_PROT_READ | VM_PROT_WRITE);
			sc->sc_pg_rx = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_ZERO);
			if (sc->sc_pg_rx == NULL) {
				panic("netif: no rx pages");
			}
			pmap_kenter_pa((vaddr_t)sc->sc_rx, VM_PAGE_TO_PHYS(sc->sc_pg_rx),
			    VM_PROT_READ | VM_PROT_WRITE);
			sc->sc_backend_state = BEST_DISCONNECTED;
		}

		/* Construct an interface-CONNECT message for the
		 * domain controller. */
		cmsg.type      = CMSG_NETIF_FE;
		cmsg.subtype   = CMSG_NETIF_FE_INTERFACE_CONNECT;
		cmsg.length    = sizeof(netif_fe_interface_connect_t);
		up.handle      = status->handle;
		up.tx_shmem_frame = xpmap_ptom(VM_PAGE_TO_PHYS(sc->sc_pg_tx)) >> PAGE_SHIFT;
		up.rx_shmem_frame = xpmap_ptom(VM_PAGE_TO_PHYS(sc->sc_pg_rx)) >> PAGE_SHIFT;
		memcpy(cmsg.msg, &up, sizeof(up));

		/* Tell the controller to bring up the interface. */
		ctrl_if_send_message_block(&cmsg, NULL, 0, 0);
		break;

	case NETIF_INTERFACE_STATUS_CONNECTED:
		if (sc->sc_backend_state == BEST_CLOSED) {
			printf("Unexpected netif-CONNECTED message"
			    " in state %d\n", sc->sc_backend_state);
			break;
		}

		memcpy(sc->sc_enaddr, status->mac, ETHER_ADDR_LEN);
#if 0
		if (xen_start_info.flags & SIF_PRIVILEGED) {
			/* XXX for domain-0 change out ethernet address to be
			 * different than the physical address since arp
			 * replies from other domains will report the physical
			 * address.
			 */
			if (sc->sc_enaddr[0] != 0xaa)
				sc->sc_enaddr[0] = 0xaa;
			else
				sc->sc_enaddr[0] = 0xab;
		}
#endif

		/* Recovery procedure: */

		/* Step 1: Reinitialise variables. */
		sc->sc_rx_resp_cons = sc->sc_tx_resp_cons = /* sc->sc_tx_full = */ 0;
		sc->sc_rx->event = sc->sc_tx->event = 1;

		/* Step 2: Rebuild the RX and TX ring contents. */
		network_alloc_rx_buffers(sc);
		SLIST_INIT(&sc->sc_tx_bufs);
		network_alloc_tx_buffers(sc);

		/* Step 3: All public and private state should now be
		 * sane.  Get ready to start sending and receiving
		 * packets and give the driver domain a kick because
		 * we've probably just requeued some packets.
		 */
		sc->sc_backend_state = BEST_CONNECTED;
		__insn_barrier();
		hypervisor_notify_via_evtchn(status->evtchn);  
		network_tx_buf_gc(sc);

		if_attach(ifp);
		ether_ifattach(ifp, sc->sc_enaddr);

		sc->sc_evtchn = status->evtchn;
		aprint_verbose("%s: using event channel %d\n",
		    sc->sc_dev.dv_xname, sc->sc_evtchn);
		event_set_handler(sc->sc_evtchn,
		    &xen_network_handler, sc, IPL_NET, sc->sc_dev.dv_xname);
		hypervisor_enable_event(sc->sc_evtchn);
		xennet_driver_count_connected();

		aprint_normal("%s: MAC address %s\n", sc->sc_dev.dv_xname,
		    ether_sprintf(sc->sc_enaddr));

#if NRND > 0
		rnd_attach_source(&sc->sc_rnd_source, sc->sc_dev.dv_xname,
		    RND_TYPE_NET, 0);
#endif
		if (in_autoconf) {
			in_autoconf = 0;
			config_pending_decr();
		}
		break;

	default:
		printf("Status change to unknown value %d\n", 
		    status->status);
		break;
	}
	DPRINTFN(XEDB_EVENT, ("xennet_interface_status_change()\n"));
}

static void
xennet_tx_mbuf_free(struct mbuf *m, caddr_t buf, size_t size, void *arg)
{
	struct xennet_txbuf *txbuf = (struct xennet_txbuf *)arg;

	DPRINTFN(XEDB_MBUF, ("xennet_tx_mbuf_free %p pa %p\n", txbuf,
	    (void *)txbuf->xt_pa));
	SLIST_INSERT_HEAD(&txbuf->xt_sc->sc_tx_bufs, txbuf, xt_next);

	if (m != NULL) {
		pool_cache_put(&mbpool_cache, m);
	}
}

static void
xennet_rx_push_buffer(struct xennet_softc *sc, int id)
{
	NETIF_RING_IDX ringidx;
	int nr_pfns;
	int s;

	ringidx = sc->sc_rx->req_prod;
	nr_pfns = 0;

	DPRINTFN(XEDB_MEM, ("readding page va %p pa %p ma %p/%p to rx_ring "
		     "at %d with id %d\n",
		     (void *)sc->sc_rx_bufa[id].xb_rx.xbrx_va,
		     (void *)sc->sc_rx_bufa[id].xb_rx.xbrx_pa,
		     (void *)(PTE_BASE[x86_btop
				  (sc->sc_rx_bufa[id].xb_rx.xbrx_va)] &
			 PG_FRAME),
		     (void *)xpmap_ptom(sc->sc_rx_bufa[id].xb_rx.xbrx_pa),
		     ringidx, id));

	sc->sc_rx->ring[MASK_NETIF_RX_IDX(ringidx)].req.id = id;

	rx_pfn_array[nr_pfns] = xpmap_ptom(sc->sc_rx_bufa[id].xb_rx.xbrx_pa)
		>> PAGE_SHIFT;

	/* Remove this page from pseudo phys map before
	 * passing back to Xen. */
	xpmap_phys_to_machine_mapping[(sc->sc_rx_bufa[id].xb_rx.xbrx_pa - XPMAP_OFFSET) >> PAGE_SHIFT] =
		INVALID_P2M_ENTRY;

	rx_mcl[nr_pfns].op = __HYPERVISOR_update_va_mapping;
	rx_mcl[nr_pfns].args[0] = sc->sc_rx_bufa[id].xb_rx.xbrx_va >> PAGE_SHIFT;
	rx_mcl[nr_pfns].args[1] = 0;
	rx_mcl[nr_pfns].args[2] = 0;

	nr_pfns++;

	sc->sc_rx_bufs_to_notify++;

	ringidx++;

	/*
	 * We may have allocated buffers which have entries
	 * outstanding in the page update queue -- make sure we flush
	 * those first!
	 */
	s = splvm();
	xpq_flush_queue();
	splx(s);

	/* After all PTEs have been zapped we blow away stale TLB entries. */
	rx_mcl[nr_pfns-1].args[2] = UVMF_FLUSH_TLB;

	/* Give away a batch of pages. */
	rx_mcl[nr_pfns].op = __HYPERVISOR_dom_mem_op;
	rx_mcl[nr_pfns].args[0] = MEMOP_decrease_reservation;
	rx_mcl[nr_pfns].args[1] = (unsigned long)rx_pfn_array;
	rx_mcl[nr_pfns].args[2] = (unsigned long)nr_pfns;
	rx_mcl[nr_pfns].args[3] = 0;
	rx_mcl[nr_pfns].args[4] = DOMID_SELF;

	/* Zap PTEs and give away pages in one big multicall. */
	(void)HYPERVISOR_multicall(rx_mcl, nr_pfns+1);

	/* Check return status of HYPERVISOR_dom_mem_op(). */
	if ( rx_mcl[nr_pfns].args[5] != nr_pfns )
		panic("Unable to reduce memory reservation\n");

	/* Above is a suitable barrier to ensure backend will see requests. */
	sc->sc_rx->req_prod = ringidx;
}

static void
xennet_rx_mbuf_free(struct mbuf *m, caddr_t buf, size_t size, void *arg)
{
	union xennet_bufarray *xb = (union xennet_bufarray *)arg;
	struct xennet_softc *sc = xb->xb_rx.xbrx_sc;
	int id = (xb - sc->sc_rx_bufa);

	DPRINTFN(XEDB_MBUF, ("xennet_rx_mbuf_free id %d, mbuf %p, buf %p, "
	    "size %d\n", id, m, buf, size));

	xennet_rx_push_buffer(sc, id);

	if (m != NULL) {
		pool_cache_put(&mbpool_cache, m);
	}
}

static int
xen_network_handler(void *arg)
{
	struct xennet_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	netif_rx_response_t *rx;
	paddr_t pa;
	NETIF_RING_IDX ringidx, resp_prod;
	mmu_update_t *mmu = rx_mmu;
	multicall_entry_t *mcl = rx_mcl;
	struct mbuf *m;

	network_tx_buf_gc(sc);
	ifp->if_flags &= ~IFF_OACTIVE;
	xennet_start(ifp);

#if NRND > 0
	rnd_add_uint32(&sc->sc_rnd_source, sc->sc_rx_resp_cons);
#endif

 again:
	resp_prod = sc->sc_rx->resp_prod;
	x86_lfence(); /* ensure we see all requests up to resp_prod */
	for (ringidx = sc->sc_rx_resp_cons;
	     ringidx != resp_prod;
	     ringidx++) {
		rx = &sc->sc_rx->ring[MASK_NETIF_RX_IDX(ringidx)].resp;

		if (rx->status < 0)
			panic("rx->status < 0");
		/* XXXcl check rx->status for error */

                MGETHDR(m, M_DONTWAIT, MT_DATA);
                if (m == NULL) {
			printf("xennet: rx no mbuf\n");
			break;
		}

		pa = sc->sc_rx_bufa[rx->id].xb_rx.xbrx_pa;

		DPRINTFN(XEDB_EVENT, ("rx event %d for id %d, size %d, "
			     "status %d, ma %08lx, pa %08lx\n", ringidx,
			     rx->id, rx->status, rx->status, rx->addr, pa));

		/* Remap the page. */
		mmu->ptr  = (rx->addr & PG_FRAME) | MMU_MACHPHYS_UPDATE;
		mmu->val  = (pa - XPMAP_OFFSET) >> PAGE_SHIFT;
		mmu++;
		mcl->op = __HYPERVISOR_update_va_mapping;
		mcl->args[0] = sc->sc_rx_bufa[rx->id].xb_rx.xbrx_va >> PAGE_SHIFT;
		mcl->args[1] = (rx->addr & PG_FRAME) | PG_V|PG_KW;
		mcl->args[2] = UVMF_FLUSH_TLB; // 0;
		mcl++;

		xpmap_phys_to_machine_mapping
			[(pa - XPMAP_OFFSET) >> PAGE_SHIFT] =
			rx->addr >> PAGE_SHIFT;

		/* Do all the remapping work, and M->P updates, in one
		 * big hypercall. */
		if ((mcl - rx_mcl) != 0) {
			mcl->op = __HYPERVISOR_mmu_update;
			mcl->args[0] = (unsigned long)rx_mmu;
			mcl->args[1] = mmu - rx_mmu;
			mcl->args[2] = 0;
			mcl++;
			(void)HYPERVISOR_multicall(rx_mcl, mcl - rx_mcl);
		}
		if (0)
		printf("page mapped at va %08lx -> %08x/%08lx\n",
		    sc->sc_rx_bufa[rx->id].xb_rx.xbrx_va,
		    PTE_BASE[x86_btop(sc->sc_rx_bufa[rx->id].xb_rx.xbrx_va)],
		    rx->addr);
		mmu = rx_mmu;
		mcl = rx_mcl;

		DPRINTFN(XEDB_MBUF, ("rx packet mbuf %p va %p pa %p/%p "
		    "ma %p\n", m,
		    (void *)sc->sc_rx_bufa[rx->id].xb_rx.xbrx_va,
		    (void *)(xpmap_mtop(PTE_BASE[x86_btop
					    (sc->sc_rx_bufa[rx->id].xb_rx.xbrx_va)] & PG_FRAME)), (void *)pa,
		    (void *)(PTE_BASE[x86_btop
			(sc->sc_rx_bufa[rx->id].xb_rx.xbrx_va)] & PG_FRAME)));

		m->m_len = m->m_pkthdr.len = rx->status;
		m->m_pkthdr.rcvif = ifp;
		if (sc->sc_rx->req_prod != sc->sc_rx->resp_prod) {
			MEXTADD(m, (void *)(sc->sc_rx_bufa[rx->id].xb_rx.
			    xbrx_va + (rx->addr & PAGE_MASK)), rx->status, M_DEVBUF,
			    xennet_rx_mbuf_free,
			    &sc->sc_rx_bufa[rx->id]);
		} else {
			/*
			 * This was our last receive buffer, allocate
			 * memory, copy data and push the receive
			 * buffer back to the hypervisor.
			 */
			MEXTMALLOC(m, rx->status, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				printf("xennet: rx no mbuf 2\n");
				m_free(m);
				break;
			}
			memcpy(m->m_data, (void *)(sc->sc_rx_bufa[rx->id].
			    xb_rx.xbrx_va + (rx->addr & PAGE_MASK)), rx->status);
			xennet_rx_push_buffer(sc, rx->id);
		}

#ifdef XENNET_DEBUG_DUMP
		xennet_hex_dump(mtod(m, u_char *), m->m_pkthdr.len, "r", rx->id);
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
	sc->sc_rx->event = resp_prod + 1;
	x86_lfence();
	  /* ensure backend see the new sc_rx->event before we start again */

	if (sc->sc_rx->resp_prod != resp_prod)
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
	NETIF_RING_IDX idx, prod;

	do {
		prod = sc->sc_tx->resp_prod;

		for (idx = sc->sc_tx_resp_cons; idx != prod; idx++) {
			DPRINTFN(XEDB_EVENT, ("tx event at pos %d, status: "
				     "%d, id: %d, mbuf %p, buf %p\n", idx,
				     sc->sc_tx->ring[MASK_NETIF_TX_IDX(idx)].resp.status,
				     sc->sc_tx->ring[MASK_NETIF_TX_IDX(idx)].resp.id,
				     sc->sc_tx_bufa[sc->sc_tx->ring[MASK_NETIF_TX_IDX(idx)].resp.id].xb_tx.xbtx_m,
				     mtod(sc->sc_tx_bufa[sc->sc_tx->ring[MASK_NETIF_TX_IDX(idx)].resp.id].xb_tx.xbtx_m, void *)));
			m_freem(sc->sc_tx_bufa[sc->sc_tx->ring[MASK_NETIF_TX_IDX(idx)].resp.id].xb_tx.xbtx_m);
			put_bufarray_entry(sc->sc_tx_bufa,
			    sc->sc_tx->ring[MASK_NETIF_TX_IDX(idx)].resp.id);
			sc->sc_tx_entries--; /* atomic */

			if (sc->sc_tx->ring[MASK_NETIF_TX_IDX(idx)].resp.status
			    == NETIF_RSP_OKAY) {
				ifp->if_opackets++;
			} else {
				ifp->if_oerrors++;
			}
		}

		sc->sc_tx_resp_cons = prod;

		/*
		 * Set a new event, then check for race with update of
		 * tx_cons.
		 */
		sc->sc_tx->event = /* atomic */
			prod + (sc->sc_tx_entries >> 1) + 1;
		x86_lfence();
	} while (prod != sc->sc_tx->resp_prod);

	if (sc->sc_tx->resp_prod == sc->sc_tx->req_prod)
		ifp->if_timer = 0;
	/* KDASSERT(sc->sc_net_idx->tx_req_prod == */
	/* TX_RING_ADD(sc->sc_net_idx->tx_resp_prod, sc->sc_tx_entries)); */
}

static void
network_alloc_rx_buffers(struct xennet_softc *sc)
{
	vaddr_t rxpages, va;
	paddr_t pa;
	struct vm_page *pg;
	int id, nr_pfns;
	NETIF_RING_IDX ringidx;
	int snet, svm;

	ringidx = sc->sc_rx->req_prod;
	if ((ringidx - sc->sc_rx_resp_cons) > (RX_MAX_ENTRIES / 2))
		return;

	nr_pfns = 0;

	rxpages = uvm_km_valloc_align(kernel_map, RX_ENTRIES * PAGE_SIZE,
	    PAGE_SIZE);

	snet = splnet();
	for (va = rxpages; va < rxpages + RX_ENTRIES * PAGE_SIZE;
	     va += PAGE_SIZE) {
		pg = uvm_pagealloc(NULL, 0, NULL, 0);
		if (pg == NULL)
			panic("network_alloc_rx_buffers: no pages");
		pmap_kenter_pa(va, VM_PAGE_TO_PHYS(pg),
		    VM_PROT_READ | VM_PROT_WRITE);

		id = get_bufarray_entry(sc->sc_rx_bufa);
		KASSERT(id < NETIF_RX_RING_SIZE);
		sc->sc_rx_bufa[id].xb_rx.xbrx_va = va;
		sc->sc_rx_bufa[id].xb_rx.xbrx_sc = sc;

		pa = VM_PAGE_TO_PHYS(pg);
		DPRINTFN(XEDB_MEM, ("adding page va %p pa %p/%p "
		    "ma %p/%p to rx_ring at %d with id %d\n", (void *)va,
			     (void *)(VM_PAGE_TO_PHYS(pg) & PG_FRAME), (void *)xpmap_mtop(PTE_BASE[x86_btop(va)]),
		    (void *)(PTE_BASE[x86_btop(va)] & PG_FRAME),
			     (void *)xpmap_ptom(VM_PAGE_TO_PHYS(pg)),
		    ringidx, id));
		sc->sc_rx_bufa[id].xb_rx.xbrx_pa = pa;
		sc->sc_rx->ring[MASK_NETIF_RX_IDX(ringidx)].req.id = id;

		rx_pfn_array[nr_pfns] = xpmap_ptom(pa) >> PAGE_SHIFT;

		/* Remove this page from pseudo phys map before
		 * passing back to Xen. */
		xpmap_phys_to_machine_mapping[(pa - XPMAP_OFFSET) >> PAGE_SHIFT] =
			INVALID_P2M_ENTRY;

		rx_mcl[nr_pfns].op = __HYPERVISOR_update_va_mapping;
		rx_mcl[nr_pfns].args[0] = va >> PAGE_SHIFT;
		rx_mcl[nr_pfns].args[1] = 0;
		rx_mcl[nr_pfns].args[2] = 0;

		nr_pfns++;

		sc->sc_rx_bufs_to_notify++;

		ringidx++;
		if ((ringidx - sc->sc_rx_resp_cons) == RX_MAX_ENTRIES)
			break;
	}

	if (nr_pfns == 0) {
		splx(snet);
		return;
	}

	/*
	 * We may have allocated buffers which have entries
	 * outstanding in the page update queue -- make sure we flush
	 * those first!
	 */
	svm = splvm();
	xpq_flush_queue();
	splx(svm);

	/* After all PTEs have been zapped we blow away stale TLB entries. */
	rx_mcl[nr_pfns-1].args[2] = UVMF_FLUSH_TLB;

	/* Give away a batch of pages. */
	rx_mcl[nr_pfns].op = __HYPERVISOR_dom_mem_op;
	rx_mcl[nr_pfns].args[0] = MEMOP_decrease_reservation;
	rx_mcl[nr_pfns].args[1] = (unsigned long)rx_pfn_array;
	rx_mcl[nr_pfns].args[2] = (unsigned long)nr_pfns;
	rx_mcl[nr_pfns].args[3] = 0;
	rx_mcl[nr_pfns].args[4] = DOMID_SELF;

	/* Zap PTEs and give away pages in one big multicall. */
	(void)HYPERVISOR_multicall(rx_mcl, nr_pfns+1);

	/* Check return status of HYPERVISOR_dom_mem_op(). */
	if (rx_mcl[nr_pfns].args[5] != nr_pfns)
		panic("Unable to reduce memory reservation\n");

	/* Above is a suitable barrier to ensure backend will see requests. */
	sc->sc_rx->req_prod = ringidx;

	splx(snet);

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
	netif_tx_request_t *txreq;
	NETIF_RING_IDX idx;
	paddr_t pa;
	int bufid;

	DPRINTFN(XEDB_FOLLOW, ("%s: xennet_start()\n", sc->sc_dev.dv_xname));

#if NRND > 0
	rnd_add_uint32(&sc->sc_rnd_source, sc->sc_tx->req_prod);
#endif

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	network_tx_buf_gc(sc);

	idx = sc->sc_tx->req_prod;
	while (/*CONSTCOND*/1) {

		IFQ_POLL(&ifp->if_snd, m);
		if (m == NULL)
			break;

		if (sc->sc_tx_entries >= NETIF_TX_RING_SIZE - 1) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		switch (m->m_flags & (M_EXT|M_EXT_CLUSTER)) {
		case M_EXT|M_EXT_CLUSTER:
			pa = m->m_ext.ext_paddr +
				(m->m_data - m->m_ext.ext_buf);
			break;
		case 0:
			pa = m->m_paddr + M_BUFOFFSET(m) +
				(m->m_data - M_BUFADDR(m));
			break;
		default:
			if (!pmap_extract(pmap_kernel(), (vaddr_t)m->m_data,
			    &pa)) {
				panic("xennet_start: no pa");
			}
			break;
		}

		if (m->m_pkthdr.len != m->m_len ||
		    (pa ^ (pa + m->m_pkthdr.len)) & PG_FRAME) {
			txbuf = SLIST_FIRST(&sc->sc_tx_bufs);
			if (txbuf == NULL) {
				// printf("xennet: no tx bufs\n");
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
		KASSERT(bufid < NETIF_TX_RING_SIZE);
		sc->sc_tx_bufa[bufid].xb_tx.xbtx_m = m;

		DPRINTFN(XEDB_MBUF, ("xennet_start id %d, mbuf %p, buf %p/%p, "
			     "size %d\n", bufid, m, mtod(m, void *),
			     (void *)pa, m->m_pkthdr.len));
#ifdef XENNET_DEBUG_DUMP
		xennet_hex_dump(mtod(m, u_char *), m->m_pkthdr.len, "s", bufid);
#endif

		txreq = &sc->sc_tx->ring[MASK_NETIF_TX_IDX(idx)].req;
		txreq->id = bufid;
		txreq->addr = xpmap_ptom(pa);
		txreq->size = m->m_pkthdr.len;

		x86_lfence();
		idx++;
		sc->sc_tx->req_prod = idx;

		sc->sc_tx_entries++; /* XXX atomic */
		x86_lfence();

#ifdef XENNET_DEBUG
		DPRINTFN(XEDB_MEM, ("packet addr %p/%p, physical %p/%p, "
		    "m_paddr %p, len %d/%d\n", M_BUFADDR(m), mtod(m, void *),
		    (void *)*kvtopte(mtod(m, vaddr_t)),
		    (void *)xpmap_mtop(*kvtopte(mtod(m, vaddr_t))),
		    (void *)m->m_paddr, m->m_pkthdr.len, m->m_len));
#endif

#if NBPFILTER > 0
		/*
		 * Pass packet to bpf if there is a listener.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif
	}

	x86_lfence();
	if (sc->sc_tx->resp_prod != idx) {
		hypervisor_notify_via_evtchn(sc->sc_evtchn);
		ifp->if_timer = 5;
	}

	DPRINTFN(XEDB_FOLLOW, ("%s: xennet_start() done\n",
	    sc->sc_dev.dv_xname));
}

int
xennet_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
#ifdef mediacode
	struct ifreq *ifr = (struct ifreq *)data;
#endif
#ifdef XENNET_DEBUG
	struct xennet_softc *sc = ifp->if_softc;
#endif
	int s, error = 0;

	s = splnet();

	DPRINTFN(XEDB_FOLLOW, ("%s: xennet_ioctl()\n", sc->sc_dev.dv_xname));
	error = ether_ioctl(ifp, cmd, data);
	if (error == ENETRESET)
		error = 0;
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

int
xennet_init(struct ifnet *ifp)
{
	struct xennet_softc *sc = ifp->if_softc;
	int s = splnet();

	DPRINTFN(XEDB_FOLLOW, ("%s: xennet_init()\n", sc->sc_dev.dv_xname));

	if (ifp->if_flags & IFF_UP) {
		if ((ifp->if_flags & IFF_RUNNING) == 0)
			xennet_reset(sc);

		ifp->if_flags |= IFF_RUNNING;
		ifp->if_flags &= ~IFF_OACTIVE;
		ifp->if_timer = 0;
	} else {
		ifp->if_flags &= ~IFF_RUNNING;
		xennet_reset(sc);
	}
	splx(s);
	return 0;
}

void
xennet_stop(struct ifnet *ifp, int disable)
{
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
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

#if defined(NFS_BOOT_BOOTSTATIC)
int
xennet_bootstatic_callback(struct nfs_diskless *nd)
{
	struct ifnet *ifp = nd->nd_ifp;
	struct xennet_softc *sc = (struct xennet_softc *)ifp->if_softc;
	union xen_cmdline_parseinfo xcp;
	struct sockaddr_in *sin;

	memset(&xcp, 0, sizeof(xcp.xcp_netinfo));
	xcp.xcp_netinfo.xi_ifno = sc->sc_ifno;
	xcp.xcp_netinfo.xi_root = nd->nd_root.ndm_host;
	xen_parse_cmdline(XEN_PARSE_NETINFO, &xcp);

	nd->nd_myip.s_addr = ntohl(xcp.xcp_netinfo.xi_ip[0]);
	nd->nd_gwip.s_addr = ntohl(xcp.xcp_netinfo.xi_ip[2]);
	nd->nd_mask.s_addr = ntohl(xcp.xcp_netinfo.xi_ip[3]);

	sin = (struct sockaddr_in *) &nd->nd_root.ndm_saddr;
	memset((caddr_t)sin, 0, sizeof(*sin));
	sin->sin_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = ntohl(xcp.xcp_netinfo.xi_ip[1]);

	return (NFS_BOOTSTATIC_HAS_MYIP|NFS_BOOTSTATIC_HAS_GWIP|
	    NFS_BOOTSTATIC_HAS_MASK|NFS_BOOTSTATIC_HAS_SERVADDR|
	    NFS_BOOTSTATIC_HAS_SERVER);
}
#endif /* defined(NFS_BOOT_BOOTSTATIC) */


#ifdef XENNET_DEBUG_DUMP
#define XCHR(x) "0123456789abcdef"[(x) & 0xf]
static void
xennet_hex_dump(unsigned char *pkt, size_t len, char *type, int id)
{
	size_t i, j;

	printf("pkt %p len %d/%x type %s id %d\n", pkt, len, len, type, id);
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

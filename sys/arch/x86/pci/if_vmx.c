/*	$NetBSD: if_vmx.c,v 1.5 2014/08/14 05:42:16 hikaru Exp $	*/
/*	$OpenBSD: if_vmx.c,v 1.16 2014/01/22 06:04:17 brad Exp $	*/

/*
 * Copyright (c) 2013 Tsubai Masanari
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_vmx.c,v 1.5 2014/08/14 05:42:16 hikaru Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/mbuf.h>
#include <sys/sockio.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <netinet/if_inarp.h>
#include <netinet/in_systm.h>	/* for <netinet/ip.h> */
#include <netinet/in.h>		/* for <netinet/ip.h> */
#include <netinet/ip.h>		/* for struct ip */
#include <netinet/tcp.h>	/* for struct tcphdr */
#include <netinet/udp.h>	/* for struct udphdr */

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <arch/x86/pci/if_vmxreg.h>

#define NRXQUEUE 1
#define NTXQUEUE 1

#define NTXDESC 128 /* tx ring size */
#define NTXSEGS 8 /* tx descriptors per packet */
#define NRXDESC 128
#define NTXCOMPDESC NTXDESC
#define NRXCOMPDESC (NRXDESC * 2)	/* ring1 + ring2 */

#define VMXNET3_DRIVER_VERSION 0x00010000

struct vmxnet3_txring {
	struct mbuf *m[NTXDESC];
	bus_dmamap_t dmap[NTXDESC];
	struct vmxnet3_txdesc *txd;
	u_int head;
	u_int next;
	uint8_t gen;
};

struct vmxnet3_rxring {
	struct mbuf *m[NRXDESC];
	bus_dmamap_t dmap[NRXDESC];
	struct vmxnet3_rxdesc *rxd;
	u_int fill;
	uint8_t gen;
	uint8_t rid;
};

struct vmxnet3_comp_ring {
	union {
		struct vmxnet3_txcompdesc *txcd;
		struct vmxnet3_rxcompdesc *rxcd;
	};
	u_int next;
	uint8_t gen;
};

struct vmxnet3_txqueue {
	struct vmxnet3_txring cmd_ring;
	struct vmxnet3_comp_ring comp_ring;
	struct vmxnet3_txq_shared *ts;
};

struct vmxnet3_rxqueue {
	struct vmxnet3_rxring cmd_ring[2];
	struct vmxnet3_comp_ring comp_ring;
	struct vmxnet3_rxq_shared *rs;
};

struct vmxnet3_softc {
	device_t sc_dev;
	struct ethercom sc_ethercom;
	struct ifmedia sc_media;

	bus_space_tag_t	sc_iot0;
	bus_space_tag_t	sc_iot1;
	bus_space_handle_t sc_ioh0;
	bus_space_handle_t sc_ioh1;
	bus_dma_tag_t sc_dmat;

	struct vmxnet3_txqueue sc_txq[NTXQUEUE];
	struct vmxnet3_rxqueue sc_rxq[NRXQUEUE];
	struct vmxnet3_driver_shared *sc_ds;
	uint8_t *sc_mcast;
};

#define VMXNET3_STAT

#ifdef VMXNET3_STAT
struct {
	u_int ntxdesc;
	u_int nrxdesc;
	u_int txhead;
	u_int txdone;
	u_int maxtxlen;
	u_int rxdone;
	u_int rxfill;
	u_int intr;
} vmxstat = {
	.ntxdesc = NTXDESC,
	.nrxdesc = NRXDESC
};
#endif

#define JUMBO_LEN (MCLBYTES - ETHER_ALIGN)	/* XXX */
#define DMAADDR(map) ((map)->dm_segs[0].ds_addr)

#define READ_BAR0(sc, reg) bus_space_read_4((sc)->sc_iot0, (sc)->sc_ioh0, reg)
#define READ_BAR1(sc, reg) bus_space_read_4((sc)->sc_iot1, (sc)->sc_ioh1, reg)
#define WRITE_BAR0(sc, reg, val) \
	bus_space_write_4((sc)->sc_iot0, (sc)->sc_ioh0, reg, val)
#define WRITE_BAR1(sc, reg, val) \
	bus_space_write_4((sc)->sc_iot1, (sc)->sc_ioh1, reg, val)
#define WRITE_CMD(sc, cmd) WRITE_BAR1(sc, VMXNET3_BAR1_CMD, cmd)
#define vtophys(va) 0		/* XXX ok? */

int vmxnet3_match(device_t, cfdata_t, void *);
void vmxnet3_attach(device_t, device_t, void *);
int vmxnet3_dma_init(struct vmxnet3_softc *);
int vmxnet3_alloc_txring(struct vmxnet3_softc *, int);
int vmxnet3_alloc_rxring(struct vmxnet3_softc *, int);
void vmxnet3_txinit(struct vmxnet3_softc *, struct vmxnet3_txqueue *);
void vmxnet3_rxinit(struct vmxnet3_softc *, struct vmxnet3_rxqueue *);
void vmxnet3_txstop(struct vmxnet3_softc *, struct vmxnet3_txqueue *);
void vmxnet3_rxstop(struct vmxnet3_softc *, struct vmxnet3_rxqueue *);
void vmxnet3_link_state(struct vmxnet3_softc *);
void vmxnet3_enable_all_intrs(struct vmxnet3_softc *);
void vmxnet3_disable_all_intrs(struct vmxnet3_softc *);
int vmxnet3_intr(void *);
void vmxnet3_evintr(struct vmxnet3_softc *);
void vmxnet3_txintr(struct vmxnet3_softc *, struct vmxnet3_txqueue *);
void vmxnet3_rxintr(struct vmxnet3_softc *, struct vmxnet3_rxqueue *);
void vmxnet3_iff(struct vmxnet3_softc *);
int vmxnet3_ifflags_cb(struct ethercom *);
void vmxnet3_rx_csum(struct vmxnet3_rxcompdesc *, struct mbuf *);
int vmxnet3_getbuf(struct vmxnet3_softc *, struct vmxnet3_rxring *);
void vmxnet3_stop(struct ifnet *, int disable);
void vmxnet3_reset(struct ifnet *);
int vmxnet3_init(struct ifnet *);
int vmxnet3_ioctl(struct ifnet *, u_long, void *);
int vmxnet3_change_mtu(struct vmxnet3_softc *, int);
void vmxnet3_start(struct ifnet *);
int vmxnet3_load_mbuf(struct vmxnet3_softc *, struct mbuf *);
void vmxnet3_watchdog(struct ifnet *);
void vmxnet3_media_status(struct ifnet *, struct ifmediareq *);
int vmxnet3_media_change(struct ifnet *);
void *vmxnet3_dma_allocmem(struct vmxnet3_softc *, u_int, u_int, bus_addr_t *);

CFATTACH_DECL3_NEW(vmx, sizeof(struct vmxnet3_softc),
    vmxnet3_match, vmxnet3_attach, NULL, NULL, NULL, NULL, 0);

int
vmxnet3_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_VMWARE &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_VMWARE_VMXNET3)
		return 1;

	return 0;
}

void
vmxnet3_attach(device_t parent, device_t self, void *aux)
{
	struct vmxnet3_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	pci_intr_handle_t ih;
	const char *intrstr;
	void *vih;
	u_int memtype, ver, macl, mach;
	pcireg_t preg;
	u_char enaddr[ETHER_ADDR_LEN];
	char intrbuf[PCI_INTRSTR_LEN];

	sc->sc_dev = self;

	pci_aprint_devinfo_fancy(pa, "Ethernet controller", "vmxnet3", 1);

	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, 0x10);
	if (pci_mapreg_map(pa, 0x10, memtype, 0, &sc->sc_iot0, &sc->sc_ioh0,
	    NULL, NULL)) {
		aprint_error_dev(sc->sc_dev, "failed to map BAR0\n");
		return;
	}
	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, 0x14);
	if (pci_mapreg_map(pa, 0x14, memtype, 0, &sc->sc_iot1, &sc->sc_ioh1,
	    NULL, NULL)) {
		aprint_error_dev(sc->sc_dev, "failed to map BAR1\n");
		return;
	}

	ver = READ_BAR1(sc, VMXNET3_BAR1_VRRS);
	if ((ver & 0x1) == 0) {
		aprint_error_dev(sc->sc_dev,
		    "unsupported hardware version 0x%x\n", ver);
		return;
	}
	WRITE_BAR1(sc, VMXNET3_BAR1_VRRS, 1);

	ver = READ_BAR1(sc, VMXNET3_BAR1_UVRS);
	if ((ver & 0x1) == 0) {
		aprint_error_dev(sc->sc_dev,
		    "incompatiable UPT version 0x%x\n", ver);
		return;
	}
	WRITE_BAR1(sc, VMXNET3_BAR1_UVRS, 1);

	preg = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	preg |= PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, preg);

	if (pci_dma64_available(pa))
		sc->sc_dmat = pa->pa_dmat64;
	else
		sc->sc_dmat = pa->pa_dmat;
	if (vmxnet3_dma_init(sc)) {
		aprint_error_dev(sc->sc_dev, "failed to setup DMA\n");
		return;
	}

	if (pci_intr_map(pa, &ih)) {
		aprint_error_dev(sc->sc_dev, "failed to map interrupt\n");
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, ih, intrbuf, sizeof(intrbuf));
	vih = pci_intr_establish(pa->pa_pc, ih, IPL_NET, vmxnet3_intr, sc);
	if (vih == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "unable to establish interrupt%s%s\n",
		    intrstr ? " at " : "", intrstr ? intrstr : "");
		return;
	}
	aprint_normal_dev(sc->sc_dev, "interrupting at %s\n", intrstr);

	WRITE_CMD(sc, VMXNET3_CMD_GET_MACL);
	macl = READ_BAR1(sc, VMXNET3_BAR1_CMD);
	enaddr[0] = macl;
	enaddr[1] = macl >> 8;
	enaddr[2] = macl >> 16;
	enaddr[3] = macl >> 24;
	WRITE_CMD(sc, VMXNET3_CMD_GET_MACH);
	mach = READ_BAR1(sc, VMXNET3_BAR1_CMD);
	enaddr[4] = mach;
	enaddr[5] = mach >> 8;

	WRITE_BAR1(sc, VMXNET3_BAR1_MACL, macl);
	WRITE_BAR1(sc, VMXNET3_BAR1_MACH, mach);
	aprint_normal_dev(sc->sc_dev, "Ethernet address %s\n",
	    ether_sprintf(enaddr));

	strlcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_MULTICAST | IFF_SIMPLEX;
	ifp->if_ioctl = vmxnet3_ioctl;
	ifp->if_start = vmxnet3_start;
	ifp->if_watchdog = vmxnet3_watchdog;
	ifp->if_init = vmxnet3_init;
	ifp->if_stop = vmxnet3_stop;
	sc->sc_ethercom.ec_capabilities |= ETHERCAP_VLAN_MTU;
	if (sc->sc_ds->upt_features & UPT1_F_CSUM)
		sc->sc_ethercom.ec_if.if_capabilities |=
		    IFCAP_CSUM_IPv4_Rx |
		    IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_TCPv4_Rx |
		    IFCAP_CSUM_UDPv4_Tx | IFCAP_CSUM_UDPv4_Rx;
	if (sc->sc_ds->upt_features & UPT1_F_VLAN)
		sc->sc_ethercom.ec_capabilities |= ETHERCAP_VLAN_HWTAGGING;

	IFQ_SET_MAXLEN(&ifp->if_snd, NTXDESC);
	IFQ_SET_READY(&ifp->if_snd);

	ifmedia_init(&sc->sc_media, IFM_IMASK, vmxnet3_media_change,
	    vmxnet3_media_status);
	ifmedia_add(&sc->sc_media, IFM_ETHER|IFM_AUTO, 0, NULL);
	ifmedia_add(&sc->sc_media, IFM_ETHER|IFM_10G_T|IFM_FDX, 0, NULL);
	ifmedia_add(&sc->sc_media, IFM_ETHER|IFM_10G_T, 0, NULL);
	ifmedia_add(&sc->sc_media, IFM_ETHER|IFM_1000_T|IFM_FDX, 0, NULL);
	ifmedia_add(&sc->sc_media, IFM_ETHER|IFM_1000_T, 0, NULL);
	ifmedia_set(&sc->sc_media, IFM_ETHER|IFM_AUTO);

	if_attach(ifp);
	ether_ifattach(ifp, enaddr);
	ether_set_ifflags_cb(&sc->sc_ethercom, vmxnet3_ifflags_cb);
	vmxnet3_link_state(sc);
}

int
vmxnet3_dma_init(struct vmxnet3_softc *sc)
{
	struct vmxnet3_driver_shared *ds;
	struct vmxnet3_txq_shared *ts;
	struct vmxnet3_rxq_shared *rs;
	bus_addr_t ds_pa, qs_pa, mcast_pa;
	int i, queue, qs_len;
	u_int major, minor, release_code, rev;

	qs_len = NTXQUEUE * sizeof *ts + NRXQUEUE * sizeof *rs;
	ts = vmxnet3_dma_allocmem(sc, qs_len, VMXNET3_DMADESC_ALIGN, &qs_pa);
	if (ts == NULL)
		return -1;
	for (queue = 0; queue < NTXQUEUE; queue++)
		sc->sc_txq[queue].ts = ts++;
	rs = (void *)ts;
	for (queue = 0; queue < NRXQUEUE; queue++)
		sc->sc_rxq[queue].rs = rs++;

	for (queue = 0; queue < NTXQUEUE; queue++)
		if (vmxnet3_alloc_txring(sc, queue))
			return -1;
	for (queue = 0; queue < NRXQUEUE; queue++)
		if (vmxnet3_alloc_rxring(sc, queue))
			return -1;

	sc->sc_mcast = vmxnet3_dma_allocmem(sc, 682 * ETHER_ADDR_LEN, 32, &mcast_pa);
	if (sc->sc_mcast == NULL)
		return -1;

	ds = vmxnet3_dma_allocmem(sc, sizeof *sc->sc_ds, 8, &ds_pa);
	if (ds == NULL)
		return -1;
	sc->sc_ds = ds;
	ds->magic = VMXNET3_REV1_MAGIC;
	ds->version = VMXNET3_DRIVER_VERSION;

	/*
	 * XXX FreeBSD version uses following values:
	 * (Does the device behavior depend on them?)
	 *
	 * major = __FreeBSD_version / 100000;
	 * minor = (__FreeBSD_version / 1000) % 100;
	 * release_code = (__FreeBSD_version / 100) % 10;
	 * rev = __FreeBSD_version % 100;
	 */
	major = 0;
	minor = 0;
	release_code = 0;
	rev = 0;
#ifdef __LP64__
	ds->guest = release_code << 30 | rev << 22 | major << 14 | minor << 6
	    | VMXNET3_GOS_FREEBSD | VMXNET3_GOS_64BIT;
#else
	ds->guest = release_code << 30 | rev << 22 | major << 14 | minor << 6
	    | VMXNET3_GOS_FREEBSD | VMXNET3_GOS_32BIT;
#endif
	ds->vmxnet3_revision = 1;
	ds->upt_version = 1;
	ds->upt_features = UPT1_F_CSUM | UPT1_F_VLAN;
	ds->driver_data = vtophys(sc);
	ds->driver_data_len = sizeof(struct vmxnet3_softc);
	ds->queue_shared = qs_pa;
	ds->queue_shared_len = qs_len;
	ds->mtu = ETHERMTU;
	ds->ntxqueue = NTXQUEUE;
	ds->nrxqueue = NRXQUEUE;
	ds->mcast_table = mcast_pa;
	ds->automask = 1;
	ds->nintr = VMXNET3_NINTR;
	ds->evintr = 0;
	ds->ictrl = VMXNET3_ICTRL_DISABLE_ALL;
	for (i = 0; i < VMXNET3_NINTR; i++)
		ds->modlevel[i] = UPT1_IMOD_ADAPTIVE;
	WRITE_BAR1(sc, VMXNET3_BAR1_DSL, ds_pa);
	WRITE_BAR1(sc, VMXNET3_BAR1_DSH, (uint64_t)ds_pa >> 32);
	return 0;
}

int
vmxnet3_alloc_txring(struct vmxnet3_softc *sc, int queue)
{
	struct vmxnet3_txqueue *tq = &sc->sc_txq[queue];
	struct vmxnet3_txq_shared *ts;
	struct vmxnet3_txring *ring = &tq->cmd_ring;
	struct vmxnet3_comp_ring *comp_ring = &tq->comp_ring;
	bus_addr_t pa, comp_pa;
	int idx;

	ring->txd = vmxnet3_dma_allocmem(sc, NTXDESC * sizeof ring->txd[0], 512, &pa);
	if (ring->txd == NULL)
		return -1;
	comp_ring->txcd = vmxnet3_dma_allocmem(sc,
	    NTXCOMPDESC * sizeof comp_ring->txcd[0], 512, &comp_pa);
	if (comp_ring->txcd == NULL)
		return -1;

	for (idx = 0; idx < NTXDESC; idx++) {
		if (bus_dmamap_create(sc->sc_dmat, JUMBO_LEN, NTXSEGS,
		    JUMBO_LEN, 0, BUS_DMA_NOWAIT, &ring->dmap[idx]))
			return -1;
	}

	ts = tq->ts;
	memset(ts, 0, sizeof *ts);
	ts->npending = 0;
	ts->intr_threshold = 1;
	ts->cmd_ring = pa;
	ts->cmd_ring_len = NTXDESC;
	ts->comp_ring = comp_pa;
	ts->comp_ring_len = NTXCOMPDESC;
	ts->driver_data = vtophys(tq);
	ts->driver_data_len = sizeof *tq;
	ts->intr_idx = 0;
	ts->stopped = 1;
	ts->error = 0;
	return 0;
}

int
vmxnet3_alloc_rxring(struct vmxnet3_softc *sc, int queue)
{
	struct vmxnet3_rxqueue *rq = &sc->sc_rxq[queue];
	struct vmxnet3_rxq_shared *rs;
	struct vmxnet3_rxring *ring;
	struct vmxnet3_comp_ring *comp_ring;
	bus_addr_t pa[2], comp_pa;
	int i, idx;

	for (i = 0; i < 2; i++) {
		ring = &rq->cmd_ring[i];
		ring->rxd = vmxnet3_dma_allocmem(sc, NRXDESC * sizeof ring->rxd[0],
		    512, &pa[i]);
		if (ring->rxd == NULL)
			return -1;
	}
	comp_ring = &rq->comp_ring;
	comp_ring->rxcd = vmxnet3_dma_allocmem(sc,
	    NRXCOMPDESC * sizeof comp_ring->rxcd[0], 512, &comp_pa);
	if (comp_ring->rxcd == NULL)
		return -1;

	for (i = 0; i < 2; i++) {
		ring = &rq->cmd_ring[i];
		ring->rid = i;
		for (idx = 0; idx < NRXDESC; idx++) {
			if (bus_dmamap_create(sc->sc_dmat, JUMBO_LEN, 1,
			    JUMBO_LEN, 0, BUS_DMA_NOWAIT, &ring->dmap[idx]))
				return -1;
		}
	}

	rs = rq->rs;
	memset(rs, 0, sizeof *rs);
	rs->cmd_ring[0] = pa[0];
	rs->cmd_ring[1] = pa[1];
	rs->cmd_ring_len[0] = NRXDESC;
	rs->cmd_ring_len[1] = NRXDESC;
	rs->comp_ring = comp_pa;
	rs->comp_ring_len = NRXCOMPDESC;
	rs->driver_data = vtophys(rq);
	rs->driver_data_len = sizeof *rq;
	rs->intr_idx = 0;
	rs->stopped = 1;
	rs->error = 0;
	return 0;
}

void
vmxnet3_txinit(struct vmxnet3_softc *sc, struct vmxnet3_txqueue *tq)
{
	struct vmxnet3_txring *ring = &tq->cmd_ring;
	struct vmxnet3_comp_ring *comp_ring = &tq->comp_ring;

	ring->head = ring->next = 0;
	ring->gen = 1;
	comp_ring->next = 0;
	comp_ring->gen = 1;
	memset(ring->txd, 0, NTXDESC * sizeof ring->txd[0]);
	memset(comp_ring->txcd, 0, NTXCOMPDESC * sizeof comp_ring->txcd[0]);
}

void
vmxnet3_rxinit(struct vmxnet3_softc *sc, struct vmxnet3_rxqueue *rq)
{
	struct vmxnet3_rxring *ring;
	struct vmxnet3_comp_ring *comp_ring;
	int i, idx;

	for (i = 0; i < 2; i++) {
		ring = &rq->cmd_ring[i];
		ring->fill = 0;
		ring->gen = 1;
		memset(ring->rxd, 0, NRXDESC * sizeof ring->rxd[0]);
		for (idx = 0; idx < NRXDESC; idx++) {
			if (vmxnet3_getbuf(sc, ring))
				break;
		}
	}
	comp_ring = &rq->comp_ring;
	comp_ring->next = 0;
	comp_ring->gen = 1;
	memset(comp_ring->rxcd, 0, NRXCOMPDESC * sizeof comp_ring->rxcd[0]);
}

void
vmxnet3_txstop(struct vmxnet3_softc *sc, struct vmxnet3_txqueue *tq)
{
	struct vmxnet3_txring *ring = &tq->cmd_ring;
	int idx;

	for (idx = 0; idx < NTXDESC; idx++) {
		if (ring->m[idx]) {
			bus_dmamap_unload(sc->sc_dmat, ring->dmap[idx]);
			m_freem(ring->m[idx]);
			ring->m[idx] = NULL;
		}
	}
}

void
vmxnet3_rxstop(struct vmxnet3_softc *sc, struct vmxnet3_rxqueue *rq)
{
	struct vmxnet3_rxring *ring;
	int i, idx;

	for (i = 0; i < 2; i++) {
		ring = &rq->cmd_ring[i];
		for (idx = 0; idx < NRXDESC; idx++) {
			if (ring->m[idx]) {
				m_freem(ring->m[idx]);
				ring->m[idx] = NULL;
			}
		}
	}
}

void
vmxnet3_link_state(struct vmxnet3_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	u_int x, link, speed;

	WRITE_CMD(sc, VMXNET3_CMD_GET_LINK);
	x = READ_BAR1(sc, VMXNET3_BAR1_CMD);
	speed = x >> 16;
	if (x & 1) {
		ifp->if_baudrate = IF_Mbps(speed);
		link = LINK_STATE_UP;
	} else
		link = LINK_STATE_DOWN;

	if_link_state_change(ifp, link);
}

static inline void
vmxnet3_enable_intr(struct vmxnet3_softc *sc, int irq)
{
	WRITE_BAR0(sc, VMXNET3_BAR0_IMASK(irq), 0);
}

static inline void
vmxnet3_disable_intr(struct vmxnet3_softc *sc, int irq)
{
	WRITE_BAR0(sc, VMXNET3_BAR0_IMASK(irq), 1);
}

void
vmxnet3_enable_all_intrs(struct vmxnet3_softc *sc)
{
	int i;

	sc->sc_ds->ictrl &= ~VMXNET3_ICTRL_DISABLE_ALL;
	for (i = 0; i < VMXNET3_NINTR; i++)
		vmxnet3_enable_intr(sc, i);
}

void
vmxnet3_disable_all_intrs(struct vmxnet3_softc *sc)
{
	int i;

	sc->sc_ds->ictrl |= VMXNET3_ICTRL_DISABLE_ALL;
	for (i = 0; i < VMXNET3_NINTR; i++)
		vmxnet3_disable_intr(sc, i);
}

int
vmxnet3_intr(void *arg)
{
	struct vmxnet3_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return 0;
	if (READ_BAR1(sc, VMXNET3_BAR1_INTR) == 0)
		return 0;
	if (sc->sc_ds->event)
		vmxnet3_evintr(sc);
	vmxnet3_rxintr(sc, &sc->sc_rxq[0]);
	vmxnet3_txintr(sc, &sc->sc_txq[0]);
#ifdef VMXNET3_STAT
	vmxstat.intr++;
#endif
	vmxnet3_enable_intr(sc, 0);
	return 1;
}

void
vmxnet3_evintr(struct vmxnet3_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	u_int event = sc->sc_ds->event;
	struct vmxnet3_txq_shared *ts;
	struct vmxnet3_rxq_shared *rs;

	/* Clear events. */
	WRITE_BAR1(sc, VMXNET3_BAR1_EVENT, event);

	/* Link state change? */
	if (event & VMXNET3_EVENT_LINK)
		vmxnet3_link_state(sc);

	/* Queue error? */
	if (event & (VMXNET3_EVENT_TQERROR | VMXNET3_EVENT_RQERROR)) {
		WRITE_CMD(sc, VMXNET3_CMD_GET_STATUS);

		ts = sc->sc_txq[0].ts;
		if (ts->stopped)
			printf("%s: TX error 0x%x\n", ifp->if_xname, ts->error);
		rs = sc->sc_rxq[0].rs;
		if (rs->stopped)
			printf("%s: RX error 0x%x\n", ifp->if_xname, rs->error);
		vmxnet3_reset(ifp);
	}

	if (event & VMXNET3_EVENT_DIC)
		printf("%s: device implementation change event\n",
		    ifp->if_xname);
	if (event & VMXNET3_EVENT_DEBUG)
		printf("%s: debug event\n", ifp->if_xname);
}

void
vmxnet3_txintr(struct vmxnet3_softc *sc, struct vmxnet3_txqueue *tq)
{
	struct vmxnet3_txring *ring = &tq->cmd_ring;
	struct vmxnet3_comp_ring *comp_ring = &tq->comp_ring;
	struct vmxnet3_txcompdesc *txcd;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	u_int sop;

	for (;;) {
		txcd = &comp_ring->txcd[comp_ring->next];

		if (le32toh((txcd->txc_word3 >> VMXNET3_TXC_GEN_S) &
		    VMXNET3_TXC_GEN_M) != comp_ring->gen)
			break;

		comp_ring->next++;
		if (comp_ring->next == NTXCOMPDESC) {
			comp_ring->next = 0;
			comp_ring->gen ^= 1;
		}

		sop = ring->next;
		if (ring->m[sop] == NULL)
			panic("vmxnet3_txintr");
		m_freem(ring->m[sop]);
		ring->m[sop] = NULL;
		bus_dmamap_unload(sc->sc_dmat, ring->dmap[sop]);
		ring->next = (le32toh((txcd->txc_word0 >>
		    VMXNET3_TXC_EOPIDX_S) & VMXNET3_TXC_EOPIDX_M) + 1)
		    % NTXDESC;

		ifp->if_flags &= ~IFF_OACTIVE;
	}
	if (ring->head == ring->next)
		ifp->if_timer = 0;
	vmxnet3_start(ifp);
}

void
vmxnet3_rxintr(struct vmxnet3_softc *sc, struct vmxnet3_rxqueue *rq)
{
	struct vmxnet3_comp_ring *comp_ring = &rq->comp_ring;
	struct vmxnet3_rxring *ring;
	struct vmxnet3_rxdesc *rxd;
	struct vmxnet3_rxcompdesc *rxcd;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *m;
	int idx, len;

	for (;;) {
		rxcd = &comp_ring->rxcd[comp_ring->next];
		if (le32toh((rxcd->rxc_word3 >> VMXNET3_RXC_GEN_S) &
		    VMXNET3_RXC_GEN_M) != comp_ring->gen)
			break;

		comp_ring->next++;
		if (comp_ring->next == NRXCOMPDESC) {
			comp_ring->next = 0;
			comp_ring->gen ^= 1;
		}

		idx = le32toh((rxcd->rxc_word0 >> VMXNET3_RXC_IDX_S) &
		    VMXNET3_RXC_IDX_M);
		if (le32toh((rxcd->rxc_word0 >> VMXNET3_RXC_QID_S) &
		    VMXNET3_RXC_QID_M) < NRXQUEUE)
			ring = &rq->cmd_ring[0];
		else
			ring = &rq->cmd_ring[1];
		rxd = &ring->rxd[idx];
		len = le32toh((rxcd->rxc_word2 >> VMXNET3_RXC_LEN_S) &
		    VMXNET3_RXC_LEN_M);
		m = ring->m[idx];
		ring->m[idx] = NULL;
		bus_dmamap_unload(sc->sc_dmat, ring->dmap[idx]);

		if (m == NULL)
			panic("NULL mbuf");

		if (le32toh((rxd->rx_word2 >> VMXNET3_RX_BTYPE_S) &
		    VMXNET3_RX_BTYPE_M) != VMXNET3_BTYPE_HEAD) {
			m_freem(m);
			goto skip_buffer;
		}
		if (le32toh(rxcd->rxc_word2 & VMXNET3_RXC_ERROR)) {
			ifp->if_ierrors++;
			m_freem(m);
			goto skip_buffer;
		}
		if (len < VMXNET3_MIN_MTU) {
			printf("%s: short packet (%d)\n", ifp->if_xname, len);
			m_freem(m);
			goto skip_buffer;
		}

		ifp->if_ipackets++;
		ifp->if_ibytes += len;

		vmxnet3_rx_csum(rxcd, m);
		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = m->m_len = len;
		if (le32toh(rxcd->rxc_word2 & VMXNET3_RXC_VLAN)) {
			VLAN_INPUT_TAG(ifp, m,
			    le32toh((rxcd->rxc_word2 >>
			    VMXNET3_RXC_VLANTAG_S) & VMXNET3_RXC_VLANTAG_M),
			    m_freem(m); goto skip_buffer);
		}

		bpf_mtap(ifp, m);

		(*ifp->if_input)(ifp, m);

skip_buffer:
#ifdef VMXNET3_STAT
		vmxstat.rxdone = idx;
#endif
		if (rq->rs->update_rxhead) {
			u_int qid = le32toh((rxcd->rxc_word0 >>
			    VMXNET3_RXC_QID_S) & VMXNET3_RXC_QID_M);

			idx = (idx + 1) % NRXDESC;
			if (qid < NRXQUEUE) {
				WRITE_BAR0(sc, VMXNET3_BAR0_RXH1(qid), idx);
			} else {
				qid -= NRXQUEUE;
				WRITE_BAR0(sc, VMXNET3_BAR0_RXH2(qid), idx);
			}
		}
	}

	/* XXX Should we (try to) allocate buffers for ring 2 too? */
	ring = &rq->cmd_ring[0];
	for (;;) {
		idx = ring->fill;
		if (ring->m[idx])
			return;
		if (vmxnet3_getbuf(sc, ring))
			return;
	}
}

void
vmxnet3_iff(struct vmxnet3_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ethercom *ec = &sc->sc_ethercom;
	struct vmxnet3_driver_shared *ds = sc->sc_ds;
	struct ether_multi *enm;
	struct ether_multistep step;
	u_int mode;
	uint8_t *p;

	ds->mcast_tablelen = 0;
	CLR(ifp->if_flags, IFF_ALLMULTI);

	/*
	 * Always accept broadcast frames.
	 * Always accept frames destined to our station address.
	 */
	mode = VMXNET3_RXMODE_BCAST | VMXNET3_RXMODE_UCAST;

	if (ISSET(ifp->if_flags, IFF_PROMISC) || ec->ec_multicnt > 682)
		goto allmulti;

	p = sc->sc_mcast;
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			/*
			 * We must listen to a range of multicast addresses.
			 * For now, just accept all multicasts, rather than
			 * trying to set only those filter bits needed to match
			 * the range.  (At this time, the only use of address
			 * ranges is for IP multicast routing, for which the
			 * range is big enough to require all bits set.)
			 */
			goto allmulti;
		}
		memcpy(p, enm->enm_addrlo, ETHER_ADDR_LEN);

		p += ETHER_ADDR_LEN;

		ETHER_NEXT_MULTI(step, enm);
	}

	if (ec->ec_multicnt > 0) {
		SET(mode, VMXNET3_RXMODE_MCAST);
		ds->mcast_tablelen = p - sc->sc_mcast;
	}

	goto setit;

allmulti:
	SET(ifp->if_flags, IFF_ALLMULTI);
	SET(mode, (VMXNET3_RXMODE_ALLMULTI | VMXNET3_RXMODE_MCAST));
	if (ifp->if_flags & IFF_PROMISC)
		SET(mode, VMXNET3_RXMODE_PROMISC);

setit:
	WRITE_CMD(sc, VMXNET3_CMD_SET_FILTER);
	ds->rxmode = mode;
	WRITE_CMD(sc, VMXNET3_CMD_SET_RXMODE);
}

int
vmxnet3_ifflags_cb(struct ethercom *ec)
{

	vmxnet3_iff((struct vmxnet3_softc *)ec->ec_if.if_softc);

	return 0;
}


void
vmxnet3_rx_csum(struct vmxnet3_rxcompdesc *rxcd, struct mbuf *m)
{
	if (le32toh(rxcd->rxc_word0 & VMXNET3_RXC_NOCSUM))
		return;

	if (rxcd->rxc_word3 & VMXNET3_RXC_IPV4) {
		m->m_pkthdr.csum_flags |= M_CSUM_IPv4;
		if ((rxcd->rxc_word3 & VMXNET3_RXC_IPSUM_OK) == 0)
			m->m_pkthdr.csum_flags |= M_CSUM_IPv4_BAD;
	}

	if (rxcd->rxc_word3 & VMXNET3_RXC_FRAGMENT)
		return;

	if (rxcd->rxc_word3 & VMXNET3_RXC_TCP) {
		m->m_pkthdr.csum_flags |= M_CSUM_TCPv4;
		if ((rxcd->rxc_word3 & VMXNET3_RXC_CSUM_OK) == 0)
			m->m_pkthdr.csum_flags |= M_CSUM_TCP_UDP_BAD;
	}

	if (rxcd->rxc_word3 & VMXNET3_RXC_UDP) {
		m->m_pkthdr.csum_flags |= M_CSUM_UDPv4;
		if ((rxcd->rxc_word3 & VMXNET3_RXC_CSUM_OK) == 0)
			m->m_pkthdr.csum_flags |= M_CSUM_TCP_UDP_BAD;
	}
}

int
vmxnet3_getbuf(struct vmxnet3_softc *sc, struct vmxnet3_rxring *ring)
{
	int idx = ring->fill;
	struct vmxnet3_rxdesc *rxd = &ring->rxd[idx];
	struct mbuf *m;
	int btype;

	if (ring->m[idx])
		panic("vmxnet3_getbuf: buffer has mbuf");

#if 1
	/* XXX Don't allocate buffers for ring 2 for now. */
	if (ring->rid != 0)
		return -1;
	btype = VMXNET3_BTYPE_HEAD;
#else
	if (ring->rid == 0)
		btype = VMXNET3_BTYPE_HEAD;
	else
		btype = VMXNET3_BTYPE_BODY;
#endif

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return -1;

	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return -1;
	}

	m->m_pkthdr.len = m->m_len = JUMBO_LEN;
	m_adj(m, ETHER_ALIGN);
	ring->m[idx] = m;

	if (bus_dmamap_load_mbuf(sc->sc_dmat, ring->dmap[idx], m,
	    BUS_DMA_NOWAIT))
		panic("load mbuf");
	rxd->rx_addr = htole64(DMAADDR(ring->dmap[idx]));
	rxd->rx_word2 = htole32(((m->m_pkthdr.len & VMXNET3_RX_LEN_M) <<
	    VMXNET3_RX_LEN_S) | ((btype & VMXNET3_RX_BTYPE_M) <<
	    VMXNET3_RX_BTYPE_S) | ((ring->gen & VMXNET3_RX_GEN_M) <<
	    VMXNET3_RX_GEN_S));
	idx++;
	if (idx == NRXDESC) {
		idx = 0;
		ring->gen ^= 1;
	}
	ring->fill = idx;
#ifdef VMXNET3_STAT
	vmxstat.rxfill = ring->fill;
#endif
	return 0;
}

void
vmxnet3_stop(struct ifnet *ifp, int disable)
{
	struct vmxnet3_softc *sc = ifp->if_softc;
	int queue;

	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return;

	vmxnet3_disable_all_intrs(sc);
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;

	if (!disable)
		return;

	WRITE_CMD(sc, VMXNET3_CMD_DISABLE);

	for (queue = 0; queue < NTXQUEUE; queue++)
		vmxnet3_txstop(sc, &sc->sc_txq[queue]);
	for (queue = 0; queue < NRXQUEUE; queue++)
		vmxnet3_rxstop(sc, &sc->sc_rxq[queue]);
}

void
vmxnet3_reset(struct ifnet *ifp)
{
	struct vmxnet3_softc *sc = ifp->if_softc;

	vmxnet3_stop(ifp, 1);
	WRITE_CMD(sc, VMXNET3_CMD_RESET);
	vmxnet3_init(ifp);
}

int
vmxnet3_init(struct ifnet *ifp)
{
	struct vmxnet3_softc *sc = ifp->if_softc;
	int queue;

	if (ifp->if_flags & IFF_RUNNING)
		return 0;

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	for (queue = 0; queue < NTXQUEUE; queue++)
		vmxnet3_txinit(sc, &sc->sc_txq[queue]);
	for (queue = 0; queue < NRXQUEUE; queue++)
		vmxnet3_rxinit(sc, &sc->sc_rxq[queue]);

	WRITE_CMD(sc, VMXNET3_CMD_ENABLE);
	if (READ_BAR1(sc, VMXNET3_BAR1_CMD)) {
		printf("%s: failed to initialize\n", ifp->if_xname);
		vmxnet3_stop(ifp, 1);
		return EIO;
	}

	for (queue = 0; queue < NRXQUEUE; queue++) {
		WRITE_BAR0(sc, VMXNET3_BAR0_RXH1(queue), 0);
		WRITE_BAR0(sc, VMXNET3_BAR0_RXH2(queue), 0);
	}

	vmxnet3_iff(sc);
	vmxnet3_enable_all_intrs(sc);
	vmxnet3_link_state(sc);
	return 0;
}

int
vmxnet3_change_mtu(struct vmxnet3_softc *sc, int mtu)
{
	struct vmxnet3_driver_shared *ds = sc->sc_ds;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int error;

	if (mtu < VMXNET3_MIN_MTU || mtu > VMXNET3_MAX_MTU)
		return EINVAL;
	vmxnet3_stop(ifp, 1);
	ifp->if_mtu = ds->mtu = mtu;
	error = vmxnet3_init(ifp);
	return error;
}

int
vmxnet3_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct vmxnet3_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int error = 0, s;

	s = splnet();

	switch (cmd) {
	case SIOCSIFMTU:
		error = vmxnet3_change_mtu(sc, ifr->ifr_mtu);
		break;
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_media, cmd);
		break;
	default:
		error = ether_ioctl(ifp, cmd, data);
	}

	if (error == ENETRESET) {
		if (ifp->if_flags & IFF_RUNNING)
			vmxnet3_iff(sc);
		error = 0;
	}

	splx(s);
	return error;
}

void
vmxnet3_start(struct ifnet *ifp)
{
	struct vmxnet3_softc *sc = ifp->if_softc;
	struct vmxnet3_txqueue *tq = &sc->sc_txq[0];
	struct vmxnet3_txring *ring = &tq->cmd_ring;
	struct mbuf *m;
	int n = 0;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	for (;;) {
		IFQ_POLL(&ifp->if_snd, m);
		if (m == NULL)
			break;
		if ((ring->next - ring->head - 1) % NTXDESC < NTXSEGS) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (vmxnet3_load_mbuf(sc, m) != 0) {
			ifp->if_oerrors++;
			continue;
		}
		bpf_mtap(ifp, m);

		ifp->if_timer = 5;
		ifp->if_opackets++;
		n++;
	}

	if (n > 0)
		WRITE_BAR0(sc, VMXNET3_BAR0_TXH(0), ring->head);
#ifdef VMXNET3_STAT
	vmxstat.txhead = ring->head;
	vmxstat.txdone = ring->next;
	vmxstat.maxtxlen =
	    max(vmxstat.maxtxlen, (ring->head - ring->next) % NTXDESC);
#endif
}

int
vmxnet3_load_mbuf(struct vmxnet3_softc *sc, struct mbuf *m)
{
	struct vmxnet3_txqueue *tq = &sc->sc_txq[0];
	struct vmxnet3_txring *ring = &tq->cmd_ring;
	struct vmxnet3_txdesc *txd = NULL, *sop;
	struct mbuf *mp;
	struct m_tag *mtag;
	struct ip *ip;
	bus_dmamap_t map = ring->dmap[ring->head];
	u_int hlen = ETHER_HDR_LEN, csum_off = 0;
	int offp, gen, i;

#if 0
	if (m->m_pkthdr.csum_flags & M_CSUM_IPv4) {
		printf("%s: IP checksum offloading is not supported\n",
		    sc->sc_dev.dv_xname);
		return -1;
	}
#endif
	if (m->m_pkthdr.csum_flags & (M_CSUM_UDPv4|M_CSUM_TCPv4)) {
		if (m->m_pkthdr.csum_flags & M_CSUM_TCPv4)
			csum_off = offsetof(struct tcphdr, th_sum);
		else
			csum_off = offsetof(struct udphdr, uh_sum);

		mp = m_pulldown(m, hlen, sizeof(*ip), &offp);
		if (mp == NULL)
			return (-1);

		ip = (struct ip *)(mp->m_data + offp);
		hlen += ip->ip_hl << 2;

		mp = m_pulldown(m, 0, hlen + csum_off + 2, &offp);
		if (mp == NULL)
			return (-1);
	}

	switch (bus_dmamap_load_mbuf(sc->sc_dmat, map, m, BUS_DMA_NOWAIT)) {
	case 0:
		break;
	case EFBIG:
		mp = m_defrag(m, M_DONTWAIT);
		if (mp != NULL) {
			m = mp;
			if (bus_dmamap_load_mbuf(sc->sc_dmat, map, m,
			    BUS_DMA_NOWAIT) == 0)
				break;
		}
		/* FALLTHROUGH */
	default:
		m_freem(m);
		return -1;
	}

	ring->m[ring->head] = m;
	sop = &ring->txd[ring->head];
	gen = ring->gen ^ 1;		/* owned by cpu (yet) */
	for (i = 0; i < map->dm_nsegs; i++) {
		txd = &ring->txd[ring->head];
		txd->tx_addr = htole64(map->dm_segs[i].ds_addr);
		txd->tx_word2 = htole32(((map->dm_segs[i].ds_len &
		    VMXNET3_TX_LEN_M) << VMXNET3_TX_LEN_S) |
		    ((gen & VMXNET3_TX_GEN_M) << VMXNET3_TX_GEN_S));
		txd->tx_word3 = 0;
		ring->head++;
		if (ring->head == NTXDESC) {
			ring->head = 0;
			ring->gen ^= 1;
		}
		gen = ring->gen;
	}
	if (txd != NULL)
		txd->tx_word3 |= htole32(VMXNET3_TX_EOP | VMXNET3_TX_COMPREQ);

	if ((mtag = VLAN_OUTPUT_TAG(&sc->sc_ethercom, m)) != NULL) {
		sop->tx_word3 |= htole32(VMXNET3_TX_VTAG_MODE);
		sop->tx_word3 |= htole32((VLAN_TAG_VALUE(mtag) &
		    VMXNET3_TX_VLANTAG_M) << VMXNET3_TX_VLANTAG_S);
	}
	if (m->m_pkthdr.csum_flags & (M_CSUM_UDPv4|M_CSUM_TCPv4)) {
		sop->tx_word2 |= htole32(((hlen + csum_off) &
		    VMXNET3_TX_OP_M) << VMXNET3_TX_OP_S);
		sop->tx_word3 |= htole32(((hlen & VMXNET3_TX_HLEN_M) <<
		    VMXNET3_TX_HLEN_S) | (VMXNET3_OM_CSUM << VMXNET3_TX_OM_S));
	}

	/* Change the ownership by flipping the "generation" bit */
	sop->tx_word2 ^= htole32(VMXNET3_TX_GEN_M << VMXNET3_TX_GEN_S);

	return (0);
}

void
vmxnet3_watchdog(struct ifnet *ifp)
{
	int s;

	printf("%s: device timeout\n", ifp->if_xname);
	s = splnet();
	vmxnet3_stop(ifp, 1);
	vmxnet3_init(ifp);
	splx(s);
}

void
vmxnet3_media_status(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct vmxnet3_softc *sc = ifp->if_softc;

	vmxnet3_link_state(sc);

	ifmr->ifm_status = IFM_AVALID;
	ifmr->ifm_active = IFM_ETHER;

	if (ifp->if_link_state != LINK_STATE_UP)
		return;

	ifmr->ifm_status |= IFM_ACTIVE;

	if (ifp->if_baudrate >= IF_Gbps(10ULL))
		ifmr->ifm_active |= IFM_10G_T;
}

int
vmxnet3_media_change(struct ifnet *ifp)
{
	return 0;
}

void *
vmxnet3_dma_allocmem(struct vmxnet3_softc *sc, u_int size, u_int align, bus_addr_t *pa)
{
	bus_dma_tag_t t = sc->sc_dmat;
	bus_dma_segment_t segs[1];
	bus_dmamap_t map;
	void *va;
	int n;

	if (bus_dmamem_alloc(t, size, align, 0, segs, 1, &n, BUS_DMA_NOWAIT))
		return NULL;
	if (bus_dmamem_map(t, segs, 1, size, &va, BUS_DMA_NOWAIT))
		return NULL;
	if (bus_dmamap_create(t, size, 1, size, 0, BUS_DMA_NOWAIT, &map))
		return NULL;
	if (bus_dmamap_load(t, map, va, size, NULL, BUS_DMA_NOWAIT))
		return NULL;
	memset(va, 0, size);
	*pa = DMAADDR(map);
	bus_dmamap_unload(t, map);
	bus_dmamap_destroy(t, map);
	return va;
}

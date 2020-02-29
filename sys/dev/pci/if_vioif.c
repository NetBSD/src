/*	$NetBSD: if_vioif.c,v 1.51.2.1 2020/02/29 20:19:11 ad Exp $	*/

/*
 * Copyright (c) 2010 Minoura Makoto.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_vioif.c,v 1.51.2.1 2020/02/29 20:19:11 ad Exp $");

#ifdef _KERNEL_OPT
#include "opt_net_mpsafe.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/condvar.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <sys/sockio.h>
#include <sys/cpu.h>
#include <sys/module.h>
#include <sys/pcq.h>

#include <dev/pci/virtioreg.h>
#include <dev/pci/virtiovar.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <net/bpf.h>

#include "ioconf.h"

#ifdef NET_MPSAFE
#define VIOIF_MPSAFE	1
#define VIOIF_MULTIQ	1
#endif

#ifdef SOFTINT_INTR
#define VIOIF_SOFTINT_INTR	1
#endif

/*
 * if_vioifreg.h:
 */
/* Configuration registers */
#define VIRTIO_NET_CONFIG_MAC		0 /* 8bit x 6byte */
#define VIRTIO_NET_CONFIG_STATUS	6 /* 16bit */
#define VIRTIO_NET_CONFIG_MAX_VQ_PAIRS	8 /* 16bit */

/* Feature bits */
#define VIRTIO_NET_F_CSUM		__BIT(0)
#define VIRTIO_NET_F_GUEST_CSUM		__BIT(1)
#define VIRTIO_NET_F_MAC		__BIT(5)
#define VIRTIO_NET_F_GSO		__BIT(6)
#define VIRTIO_NET_F_GUEST_TSO4		__BIT(7)
#define VIRTIO_NET_F_GUEST_TSO6		__BIT(8)
#define VIRTIO_NET_F_GUEST_ECN		__BIT(9)
#define VIRTIO_NET_F_GUEST_UFO		__BIT(10)
#define VIRTIO_NET_F_HOST_TSO4		__BIT(11)
#define VIRTIO_NET_F_HOST_TSO6		__BIT(12)
#define VIRTIO_NET_F_HOST_ECN		__BIT(13)
#define VIRTIO_NET_F_HOST_UFO		__BIT(14)
#define VIRTIO_NET_F_MRG_RXBUF		__BIT(15)
#define VIRTIO_NET_F_STATUS		__BIT(16)
#define VIRTIO_NET_F_CTRL_VQ		__BIT(17)
#define VIRTIO_NET_F_CTRL_RX		__BIT(18)
#define VIRTIO_NET_F_CTRL_VLAN		__BIT(19)
#define VIRTIO_NET_F_CTRL_RX_EXTRA	__BIT(20)
#define VIRTIO_NET_F_GUEST_ANNOUNCE	__BIT(21)
#define VIRTIO_NET_F_MQ			__BIT(22)

#define VIRTIO_NET_FLAG_BITS \
	VIRTIO_COMMON_FLAG_BITS \
	"\x17""MQ" \
	"\x16""GUEST_ANNOUNCE" \
	"\x15""CTRL_RX_EXTRA" \
	"\x14""CTRL_VLAN" \
	"\x13""CTRL_RX" \
	"\x12""CTRL_VQ" \
	"\x11""STATUS" \
	"\x10""MRG_RXBUF" \
	"\x0f""HOST_UFO" \
	"\x0e""HOST_ECN" \
	"\x0d""HOST_TSO6" \
	"\x0c""HOST_TSO4" \
	"\x0b""GUEST_UFO" \
	"\x0a""GUEST_ECN" \
	"\x09""GUEST_TSO6" \
	"\x08""GUEST_TSO4" \
	"\x07""GSO" \
	"\x06""MAC" \
	"\x02""GUEST_CSUM" \
	"\x01""CSUM"

/* Status */
#define VIRTIO_NET_S_LINK_UP	1

/* Packet header structure */
struct virtio_net_hdr {
	uint8_t		flags;
	uint8_t		gso_type;
	uint16_t	hdr_len;
	uint16_t	gso_size;
	uint16_t	csum_start;
	uint16_t	csum_offset;
#if 0
	uint16_t	num_buffers; /* if VIRTIO_NET_F_MRG_RXBUF enabled */
#endif
} __packed;

#define VIRTIO_NET_HDR_F_NEEDS_CSUM	1 /* flags */
#define VIRTIO_NET_HDR_GSO_NONE		0 /* gso_type */
#define VIRTIO_NET_HDR_GSO_TCPV4	1 /* gso_type */
#define VIRTIO_NET_HDR_GSO_UDP		3 /* gso_type */
#define VIRTIO_NET_HDR_GSO_TCPV6	4 /* gso_type */
#define VIRTIO_NET_HDR_GSO_ECN		0x80 /* gso_type, |'ed */

#define VIRTIO_NET_MAX_GSO_LEN		(65536+ETHER_HDR_LEN)

/* Control virtqueue */
struct virtio_net_ctrl_cmd {
	uint8_t	class;
	uint8_t	command;
} __packed;
#define VIRTIO_NET_CTRL_RX		0
# define VIRTIO_NET_CTRL_RX_PROMISC	0
# define VIRTIO_NET_CTRL_RX_ALLMULTI	1

#define VIRTIO_NET_CTRL_MAC		1
# define VIRTIO_NET_CTRL_MAC_TABLE_SET	0

#define VIRTIO_NET_CTRL_VLAN		2
# define VIRTIO_NET_CTRL_VLAN_ADD	0
# define VIRTIO_NET_CTRL_VLAN_DEL	1

#define VIRTIO_NET_CTRL_MQ			4
# define VIRTIO_NET_CTRL_MQ_VQ_PAIRS_SET	0
# define VIRTIO_NET_CTRL_MQ_VQ_PAIRS_MIN	1
# define VIRTIO_NET_CTRL_MQ_VQ_PAIRS_MAX	0x8000

struct virtio_net_ctrl_status {
	uint8_t	ack;
} __packed;
#define VIRTIO_NET_OK			0
#define VIRTIO_NET_ERR			1

struct virtio_net_ctrl_rx {
	uint8_t	onoff;
} __packed;

struct virtio_net_ctrl_mac_tbl {
	uint32_t nentries;
	uint8_t macs[][ETHER_ADDR_LEN];
} __packed;

struct virtio_net_ctrl_vlan {
	uint16_t id;
} __packed;

struct virtio_net_ctrl_mq {
	uint16_t virtqueue_pairs;
} __packed;

struct vioif_ctrl_cmdspec {
	bus_dmamap_t	dmamap;
	void		*buf;
	bus_size_t	bufsize;
};

/*
 * if_vioifvar.h:
 */

/*
 * Locking notes:
 * + a field in vioif_txqueue is protected by txq_lock (a spin mutex), and
 *   a filds in vioif_rxqueue is protected by rxq_lock (a spin mutex).
 *      - more than one lock cannot be held at onece
 * + ctrlq_inuse is protected by ctrlq_wait_lock.
 *      - other fields in vioif_ctrlqueue are protected by ctrlq_inuse
 *      - txq_lock or rxq_lock cannot be held along with ctrlq_wait_lock
 */

struct vioif_txqueue {
	kmutex_t		*txq_lock;	/* lock for tx operations */

	struct virtqueue	*txq_vq;
	bool			txq_stopping;
	bool			txq_link_active;
	pcq_t			*txq_intrq;

	struct virtio_net_hdr	*txq_hdrs;
	bus_dmamap_t		*txq_hdr_dmamaps;

	struct mbuf		**txq_mbufs;
	bus_dmamap_t		*txq_dmamaps;

	void			*txq_deferred_transmit;
};

struct vioif_rxqueue {
	kmutex_t		*rxq_lock;	/* lock for rx operations */

	struct virtqueue	*rxq_vq;
	bool			rxq_stopping;

	struct virtio_net_hdr	*rxq_hdrs;
	bus_dmamap_t		*rxq_hdr_dmamaps;

	struct mbuf		**rxq_mbufs;
	bus_dmamap_t		*rxq_dmamaps;

	void			*rxq_softint;
};

struct vioif_ctrlqueue {
	struct virtqueue		*ctrlq_vq;
	enum {
		FREE, INUSE, DONE
	}				ctrlq_inuse;
	kcondvar_t			ctrlq_wait;
	kmutex_t			ctrlq_wait_lock;
	struct lwp			*ctrlq_owner;

	struct virtio_net_ctrl_cmd	*ctrlq_cmd;
	struct virtio_net_ctrl_status	*ctrlq_status;
	struct virtio_net_ctrl_rx	*ctrlq_rx;
	struct virtio_net_ctrl_mac_tbl	*ctrlq_mac_tbl_uc;
	struct virtio_net_ctrl_mac_tbl	*ctrlq_mac_tbl_mc;
	struct virtio_net_ctrl_mq	*ctrlq_mq;

	bus_dmamap_t			ctrlq_cmd_dmamap;
	bus_dmamap_t			ctrlq_status_dmamap;
	bus_dmamap_t			ctrlq_rx_dmamap;
	bus_dmamap_t			ctrlq_tbl_uc_dmamap;
	bus_dmamap_t			ctrlq_tbl_mc_dmamap;
	bus_dmamap_t			ctrlq_mq_dmamap;
};

struct vioif_softc {
	device_t		sc_dev;

	struct virtio_softc	*sc_virtio;
	struct virtqueue	*sc_vqs;

	int			sc_max_nvq_pairs;
	int			sc_req_nvq_pairs;
	int			sc_act_nvq_pairs;

	uint8_t			sc_mac[ETHER_ADDR_LEN];
	struct ethercom		sc_ethercom;
	short			sc_deferred_init_done;
	bool			sc_link_active;

	struct vioif_txqueue	*sc_txq;
	struct vioif_rxqueue	*sc_rxq;

	bool			sc_has_ctrl;
	struct vioif_ctrlqueue	sc_ctrlq;

	bus_dma_segment_t	sc_hdr_segs[1];
	void			*sc_dmamem;
	void			*sc_kmem;

	void			*sc_ctl_softint;
};
#define VIRTIO_NET_TX_MAXNSEGS		(16) /* XXX */
#define VIRTIO_NET_CTRL_MAC_MAXENTRIES	(64) /* XXX */

/* cfattach interface functions */
static int	vioif_match(device_t, cfdata_t, void *);
static void	vioif_attach(device_t, device_t, void *);
static void	vioif_deferred_init(device_t);

/* ifnet interface functions */
static int	vioif_init(struct ifnet *);
static void	vioif_stop(struct ifnet *, int);
static void	vioif_start(struct ifnet *);
static void	vioif_start_locked(struct ifnet *, struct vioif_txqueue *);
static int	vioif_transmit(struct ifnet *, struct mbuf *);
static void	vioif_transmit_locked(struct ifnet *, struct vioif_txqueue *);
static int	vioif_ioctl(struct ifnet *, u_long, void *);
static void	vioif_watchdog(struct ifnet *);

/* rx */
static int	vioif_add_rx_mbuf(struct vioif_rxqueue *, int);
static void	vioif_free_rx_mbuf(struct vioif_rxqueue *, int);
static void	vioif_populate_rx_mbufs(struct vioif_rxqueue *);
static void	vioif_populate_rx_mbufs_locked(struct vioif_rxqueue *);
static int	vioif_rx_deq(struct vioif_rxqueue *);
static int	vioif_rx_deq_locked(struct vioif_rxqueue *);
static int	vioif_rx_vq_done(struct virtqueue *);
static void	vioif_rx_softint(void *);
static void	vioif_rx_drain(struct vioif_rxqueue *);

/* tx */
static int	vioif_tx_vq_done(struct virtqueue *);
static int	vioif_tx_vq_done_locked(struct virtqueue *);
static void	vioif_tx_drain(struct vioif_txqueue *);
static void	vioif_deferred_transmit(void *);

/* other control */
static bool	vioif_is_link_up(struct vioif_softc *);
static void	vioif_update_link_status(struct vioif_softc *);
static int	vioif_ctrl_rx(struct vioif_softc *, int, bool);
static int	vioif_set_promisc(struct vioif_softc *, bool);
static int	vioif_set_allmulti(struct vioif_softc *, bool);
static int	vioif_set_rx_filter(struct vioif_softc *);
static int	vioif_rx_filter(struct vioif_softc *);
static int	vioif_ctrl_vq_done(struct virtqueue *);
static int	vioif_config_change(struct virtio_softc *);
static void	vioif_ctl_softint(void *);
static int	vioif_ctrl_mq_vq_pairs_set(struct vioif_softc *, int);
static void	vioif_enable_interrupt_vqpairs(struct vioif_softc *);
static void	vioif_disable_interrupt_vqpairs(struct vioif_softc *);

CFATTACH_DECL_NEW(vioif, sizeof(struct vioif_softc),
		  vioif_match, vioif_attach, NULL, NULL);

static int
vioif_match(device_t parent, cfdata_t match, void *aux)
{
	struct virtio_attach_args *va = aux;

	if (va->sc_childdevid == PCI_PRODUCT_VIRTIO_NETWORK)
		return 1;

	return 0;
}

static void
vioif_alloc_queues(struct vioif_softc *sc)
{
	int nvq_pairs = sc->sc_max_nvq_pairs;
	int nvqs = nvq_pairs * 2;
	int i;

	KASSERT(nvq_pairs <= VIRTIO_NET_CTRL_MQ_VQ_PAIRS_MAX);

	sc->sc_rxq = kmem_zalloc(sizeof(sc->sc_rxq[0]) * nvq_pairs,
	    KM_SLEEP);
	sc->sc_txq = kmem_zalloc(sizeof(sc->sc_txq[0]) * nvq_pairs,
	    KM_SLEEP);

	if (sc->sc_has_ctrl)
		nvqs++;

	sc->sc_vqs = kmem_zalloc(sizeof(sc->sc_vqs[0]) * nvqs, KM_SLEEP);
	nvqs = 0;
	for (i = 0; i < nvq_pairs; i++) {
		sc->sc_rxq[i].rxq_vq = &sc->sc_vqs[nvqs++];
		sc->sc_txq[i].txq_vq = &sc->sc_vqs[nvqs++];
	}

	if (sc->sc_has_ctrl)
		sc->sc_ctrlq.ctrlq_vq = &sc->sc_vqs[nvqs++];
}

static void
vioif_free_queues(struct vioif_softc *sc)
{
	int nvq_pairs = sc->sc_max_nvq_pairs;
	int nvqs = nvq_pairs * 2;

	if (sc->sc_ctrlq.ctrlq_vq)
		nvqs++;

	if (sc->sc_txq) {
		kmem_free(sc->sc_txq, sizeof(sc->sc_txq[0]) * nvq_pairs);
		sc->sc_txq = NULL;
	}

	if (sc->sc_rxq) {
		kmem_free(sc->sc_rxq, sizeof(sc->sc_rxq[0]) * nvq_pairs);
		sc->sc_rxq = NULL;
	}

	if (sc->sc_vqs) {
		kmem_free(sc->sc_vqs, sizeof(sc->sc_vqs[0]) * nvqs);
		sc->sc_vqs = NULL;
	}
}

/* allocate memory */
/*
 * dma memory is used for:
 *   rxq_hdrs[slot]:	 metadata array for received frames (READ)
 *   txq_hdrs[slot]:	 metadata array for frames to be sent (WRITE)
 *   ctrlq_cmd:		 command to be sent via ctrl vq (WRITE)
 *   ctrlq_status:	 return value for a command via ctrl vq (READ)
 *   ctrlq_rx:		 parameter for a VIRTIO_NET_CTRL_RX class command
 *			 (WRITE)
 *   ctrlq_mac_tbl_uc:	 unicast MAC address filter for a VIRTIO_NET_CTRL_MAC
 *			 class command (WRITE)
 *   ctrlq_mac_tbl_mc:	 multicast MAC address filter for a VIRTIO_NET_CTRL_MAC
 *			 class command (WRITE)
 * ctrlq_* structures are allocated only one each; they are protected by
 * ctrlq_inuse variable and ctrlq_wait condvar.
 */
/*
 * dynamically allocated memory is used for:
 *   rxq_hdr_dmamaps[slot]:	bus_dmamap_t array for sc_rx_hdrs[slot]
 *   txq_hdr_dmamaps[slot]:	bus_dmamap_t array for sc_tx_hdrs[slot]
 *   rxq_dmamaps[slot]:		bus_dmamap_t array for received payload
 *   txq_dmamaps[slot]:		bus_dmamap_t array for sent payload
 *   rxq_mbufs[slot]:		mbuf pointer array for received frames
 *   txq_mbufs[slot]:		mbuf pointer array for sent frames
 */
static int
vioif_alloc_mems(struct vioif_softc *sc)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	struct vioif_txqueue *txq;
	struct vioif_rxqueue *rxq;
	struct vioif_ctrlqueue *ctrlq = &sc->sc_ctrlq;
	int allocsize, allocsize2, r, rsegs, i, qid;
	void *vaddr;
	intptr_t p;

	allocsize = 0;
	for (qid = 0; qid < sc->sc_max_nvq_pairs; qid++) {
		rxq = &sc->sc_rxq[qid];
		txq = &sc->sc_txq[qid];

		allocsize +=
		    sizeof(struct virtio_net_hdr) * rxq->rxq_vq->vq_num;
		allocsize +=
		    sizeof(struct virtio_net_hdr) * txq->txq_vq->vq_num;
	}
	if (sc->sc_has_ctrl) {
		allocsize += sizeof(struct virtio_net_ctrl_cmd) * 1;
		allocsize += sizeof(struct virtio_net_ctrl_status) * 1;
		allocsize += sizeof(struct virtio_net_ctrl_rx) * 1;
		allocsize += sizeof(struct virtio_net_ctrl_mac_tbl)
		    + sizeof(struct virtio_net_ctrl_mac_tbl)
		    + ETHER_ADDR_LEN * VIRTIO_NET_CTRL_MAC_MAXENTRIES;
		allocsize += sizeof(struct virtio_net_ctrl_mq) * 1;
	}
	r = bus_dmamem_alloc(virtio_dmat(vsc), allocsize, 0, 0,
	    &sc->sc_hdr_segs[0], 1, &rsegs, BUS_DMA_NOWAIT);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev,
		    "DMA memory allocation failed, size %d, "
		    "error code %d\n", allocsize, r);
		goto err_none;
	}
	r = bus_dmamem_map(virtio_dmat(vsc),
	    &sc->sc_hdr_segs[0], 1, allocsize, &vaddr, BUS_DMA_NOWAIT);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev,
		    "DMA memory map failed, error code %d\n", r);
		goto err_dmamem_alloc;
	}

#define P(p, p0, p0size)	do { p0 = (void *) p;		\
				     p += p0size; } while (0)
	memset(vaddr, 0, allocsize);
	sc->sc_dmamem = vaddr;
	p = (intptr_t) vaddr;

	for (qid = 0; qid < sc->sc_max_nvq_pairs; qid++) {
		rxq = &sc->sc_rxq[qid];
		txq = &sc->sc_txq[qid];

		P(p, rxq->rxq_hdrs,
		    sizeof(rxq->rxq_hdrs[0]) * rxq->rxq_vq->vq_num);
		P(p, txq->txq_hdrs,
		    sizeof(txq->txq_hdrs[0]) * txq->txq_vq->vq_num);
	}
	if (sc->sc_has_ctrl) {
		P(p, ctrlq->ctrlq_cmd, sizeof(*ctrlq->ctrlq_cmd));
		P(p, ctrlq->ctrlq_status, sizeof(*ctrlq->ctrlq_status));
		P(p, ctrlq->ctrlq_rx, sizeof(*ctrlq->ctrlq_rx));
		P(p, ctrlq->ctrlq_mac_tbl_uc, sizeof(*ctrlq->ctrlq_mac_tbl_uc));
		P(p, ctrlq->ctrlq_mac_tbl_mc, sizeof(*ctrlq->ctrlq_mac_tbl_mc)
		    + ETHER_ADDR_LEN * VIRTIO_NET_CTRL_MAC_MAXENTRIES);
		P(p, ctrlq->ctrlq_mq, sizeof(*ctrlq->ctrlq_mq));
	}

	allocsize2 = 0;
	for (qid = 0; qid < sc->sc_max_nvq_pairs; qid++) {
		int rxqsize, txqsize;

		rxq = &sc->sc_rxq[qid];
		txq = &sc->sc_txq[qid];
		rxqsize = rxq->rxq_vq->vq_num;
		txqsize = txq->txq_vq->vq_num;

		allocsize2 += sizeof(rxq->rxq_dmamaps[0]) * rxqsize;
		allocsize2 += sizeof(rxq->rxq_hdr_dmamaps[0]) * rxqsize;
		allocsize2 += sizeof(rxq->rxq_mbufs[0]) * rxqsize;

		allocsize2 += sizeof(txq->txq_dmamaps[0]) * txqsize;
		allocsize2 += sizeof(txq->txq_hdr_dmamaps[0]) * txqsize;
		allocsize2 += sizeof(txq->txq_mbufs[0]) * txqsize;
	}
	vaddr = kmem_zalloc(allocsize2, KM_SLEEP);
	sc->sc_kmem = vaddr;
	p = (intptr_t) vaddr;

	for (qid = 0; qid < sc->sc_max_nvq_pairs; qid++) {
		int rxqsize, txqsize;
		rxq = &sc->sc_rxq[qid];
		txq = &sc->sc_txq[qid];
		rxqsize = rxq->rxq_vq->vq_num;
		txqsize = txq->txq_vq->vq_num;

		P(p, rxq->rxq_hdr_dmamaps,
		    sizeof(rxq->rxq_hdr_dmamaps[0]) * rxqsize);
		P(p, txq->txq_hdr_dmamaps,
		    sizeof(txq->txq_hdr_dmamaps[0]) * txqsize);
		P(p, rxq->rxq_dmamaps, sizeof(rxq->rxq_dmamaps[0]) * rxqsize);
		P(p, txq->txq_dmamaps, sizeof(txq->txq_dmamaps[0]) * txqsize);
		P(p, rxq->rxq_mbufs, sizeof(rxq->rxq_mbufs[0]) * rxqsize);
		P(p, txq->txq_mbufs, sizeof(txq->txq_mbufs[0]) * txqsize);
	}
#undef P

#define C(map, size, nsegs, usage)					      \
	do {								      \
		r = bus_dmamap_create(virtio_dmat(vsc), size, nsegs, size, 0, \
		    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,			      \
		    &map);						      \
		if (r != 0) {						      \
			aprint_error_dev(sc->sc_dev,			      \
			    usage " dmamap creation failed, "		      \
			    "error code %d\n", r);			      \
			goto err_reqs;					      \
		}							      \
	} while (0)
#define C_L(map, buf, size, nsegs, rw, usage)				\
	C(map, size, nsegs, usage);					\
	do {								\
		r = bus_dmamap_load(virtio_dmat(vsc), map,		\
				    buf, size, NULL,			\
				    rw | BUS_DMA_NOWAIT);		\
		if (r != 0) {						\
			aprint_error_dev(sc->sc_dev,			\
			    usage " dmamap load failed, "		\
			    "error code %d\n", r);			\
			goto err_reqs;					\
		}							\
	} while (0)

	for (qid = 0; qid < sc->sc_max_nvq_pairs; qid++) {
		rxq = &sc->sc_rxq[qid];
		txq = &sc->sc_txq[qid];

		for (i = 0; i < rxq->rxq_vq->vq_num; i++) {
			C_L(rxq->rxq_hdr_dmamaps[i], &rxq->rxq_hdrs[i],
			    sizeof(rxq->rxq_hdrs[0]), 1,
			    BUS_DMA_READ, "rx header");
			C(rxq->rxq_dmamaps[i], MCLBYTES, 1, "rx payload");
		}

		for (i = 0; i < txq->txq_vq->vq_num; i++) {
			C_L(txq->txq_hdr_dmamaps[i], &txq->txq_hdrs[i],
			    sizeof(txq->txq_hdrs[0]), 1,
			    BUS_DMA_READ, "tx header");
			C(txq->txq_dmamaps[i], ETHER_MAX_LEN,
			    VIRTIO_NET_TX_MAXNSEGS, "tx payload");
		}
	}

	if (sc->sc_has_ctrl) {
		/* control vq class & command */
		C_L(ctrlq->ctrlq_cmd_dmamap,
		    ctrlq->ctrlq_cmd, sizeof(*ctrlq->ctrlq_cmd), 1,
		    BUS_DMA_WRITE, "control command");
		C_L(ctrlq->ctrlq_status_dmamap,
		    ctrlq->ctrlq_status, sizeof(*ctrlq->ctrlq_status), 1,
		    BUS_DMA_READ, "control status");

		/* control vq rx mode command parameter */
		C_L(ctrlq->ctrlq_rx_dmamap,
		    ctrlq->ctrlq_rx, sizeof(*ctrlq->ctrlq_rx), 1,
		    BUS_DMA_WRITE, "rx mode control command");

		/* multiqueue set command */
		C_L(ctrlq->ctrlq_mq_dmamap,
		    ctrlq->ctrlq_mq, sizeof(*ctrlq->ctrlq_mq), 1,
		    BUS_DMA_WRITE, "multiqueue set command");

		/* control vq MAC filter table for unicast */
		/* do not load now since its length is variable */
		C(ctrlq->ctrlq_tbl_uc_dmamap,
		    sizeof(*ctrlq->ctrlq_mac_tbl_uc) + 0, 1,
		    "unicast MAC address filter command");

		/* control vq MAC filter table for multicast */
		C(ctrlq->ctrlq_tbl_mc_dmamap,
		    sizeof(*ctrlq->ctrlq_mac_tbl_mc)
		    + ETHER_ADDR_LEN * VIRTIO_NET_CTRL_MAC_MAXENTRIES, 1,
		    "multicast MAC address filter command");
	}
#undef C_L
#undef C

	return 0;

err_reqs:
#define D(map)								\
	do {								\
		if (map) {						\
			bus_dmamap_destroy(virtio_dmat(vsc), map);	\
			map = NULL;					\
		}							\
	} while (0)
	D(ctrlq->ctrlq_tbl_mc_dmamap);
	D(ctrlq->ctrlq_tbl_uc_dmamap);
	D(ctrlq->ctrlq_rx_dmamap);
	D(ctrlq->ctrlq_status_dmamap);
	D(ctrlq->ctrlq_cmd_dmamap);
	for (qid = 0; qid < sc->sc_max_nvq_pairs; qid++) {
		rxq = &sc->sc_rxq[qid];
		txq = &sc->sc_txq[qid];

		for (i = 0; i < txq->txq_vq->vq_num; i++) {
			D(txq->txq_dmamaps[i]);
			D(txq->txq_hdr_dmamaps[i]);
		}
		for (i = 0; i < rxq->rxq_vq->vq_num; i++) {
			D(rxq->rxq_dmamaps[i]);
			D(rxq->rxq_hdr_dmamaps[i]);
		}
	}
#undef D
	if (sc->sc_kmem) {
		kmem_free(sc->sc_kmem, allocsize2);
		sc->sc_kmem = NULL;
	}
	bus_dmamem_unmap(virtio_dmat(vsc), sc->sc_dmamem, allocsize);
err_dmamem_alloc:
	bus_dmamem_free(virtio_dmat(vsc), &sc->sc_hdr_segs[0], 1);
err_none:
	return -1;
}

static void
vioif_attach(device_t parent, device_t self, void *aux)
{
	struct vioif_softc *sc = device_private(self);
	struct virtio_softc *vsc = device_private(parent);
	struct vioif_ctrlqueue *ctrlq = &sc->sc_ctrlq;
	struct vioif_txqueue *txq;
	struct vioif_rxqueue *rxq;
	uint32_t features, req_features;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	u_int softint_flags;
	int r, i, nvqs=0, req_flags;

	if (virtio_child(vsc) != NULL) {
		aprint_normal(": child already attached for %s; "
		    "something wrong...\n", device_xname(parent));
		return;
	}

	sc->sc_dev = self;
	sc->sc_virtio = vsc;
	sc->sc_link_active = false;

	sc->sc_max_nvq_pairs = 1;
	sc->sc_req_nvq_pairs = 1;
	sc->sc_act_nvq_pairs = 1;

	req_flags = 0;

#ifdef VIOIF_MPSAFE
	req_flags |= VIRTIO_F_PCI_INTR_MPSAFE;
#endif
#ifdef VIOIF_SOFTINT_INTR
	req_flags |= VIRTIO_F_PCI_INTR_SOFTINT;
#endif
	req_flags |= VIRTIO_F_PCI_INTR_MSIX;

	req_features =
	    VIRTIO_NET_F_MAC | VIRTIO_NET_F_STATUS | VIRTIO_NET_F_CTRL_VQ |
	    VIRTIO_NET_F_CTRL_RX | VIRTIO_F_NOTIFY_ON_EMPTY;
#ifdef VIOIF_MULTIQ
	req_features |= VIRTIO_NET_F_MQ;
#endif
	virtio_child_attach_start(vsc, self, IPL_NET, NULL,
	    vioif_config_change, virtio_vq_intr, req_flags,
	    req_features, VIRTIO_NET_FLAG_BITS);

	features = virtio_features(vsc);

	if (features & VIRTIO_NET_F_MAC) {
		for (i = 0; i < __arraycount(sc->sc_mac); i++) {
			sc->sc_mac[i] = virtio_read_device_config_1(vsc,
			    VIRTIO_NET_CONFIG_MAC + i);
		}
	} else {
		/* code stolen from sys/net/if_tap.c */
		struct timeval tv;
		uint32_t ui;
		getmicrouptime(&tv);
		ui = (tv.tv_sec ^ tv.tv_usec) & 0xffffff;
		memcpy(sc->sc_mac+3, (uint8_t *)&ui, 3);
		for (i = 0; i < __arraycount(sc->sc_mac); i++) {
			virtio_write_device_config_1(vsc,
			    VIRTIO_NET_CONFIG_MAC + i, sc->sc_mac[i]);
		}
	}

	aprint_normal_dev(self, "Ethernet address %s\n",
	    ether_sprintf(sc->sc_mac));

	if ((features & VIRTIO_NET_F_CTRL_VQ) &&
	    (features & VIRTIO_NET_F_CTRL_RX)) {
		sc->sc_has_ctrl = true;

		cv_init(&ctrlq->ctrlq_wait, "ctrl_vq");
		mutex_init(&ctrlq->ctrlq_wait_lock, MUTEX_DEFAULT, IPL_NET);
		ctrlq->ctrlq_inuse = FREE;
	} else {
		sc->sc_has_ctrl = false;
	}

	if (sc->sc_has_ctrl && (features & VIRTIO_NET_F_MQ)) {
		sc->sc_max_nvq_pairs = virtio_read_device_config_2(vsc,
		    VIRTIO_NET_CONFIG_MAX_VQ_PAIRS);

		if (sc->sc_max_nvq_pairs > VIRTIO_NET_CTRL_MQ_VQ_PAIRS_MAX)
			goto err;

		/* Limit the number of queue pairs to use */
		sc->sc_req_nvq_pairs = MIN(sc->sc_max_nvq_pairs, ncpu);
	}

	vioif_alloc_queues(sc);
	virtio_child_attach_set_vqs(vsc, sc->sc_vqs, sc->sc_req_nvq_pairs);

#ifdef VIOIF_MPSAFE
	softint_flags = SOFTINT_NET | SOFTINT_MPSAFE;
#else
	softint_flags = SOFTINT_NET;
#endif

	/*
	 * Allocating a virtqueues
	 */
	for (i = 0; i < sc->sc_max_nvq_pairs; i++) {
		rxq = &sc->sc_rxq[i];
		txq = &sc->sc_txq[i];
		char qname[32];

		rxq->rxq_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NET);

		rxq->rxq_softint = softint_establish(softint_flags,
		    vioif_rx_softint, rxq);
		if (rxq->rxq_softint == NULL) {
			aprint_error_dev(self, "cannot establish rx softint\n");
			goto err;
		}
		snprintf(qname, sizeof(qname), "rx%d", i);
		r = virtio_alloc_vq(vsc, rxq->rxq_vq, nvqs,
		    MCLBYTES+sizeof(struct virtio_net_hdr), nvqs, qname);
		if (r != 0)
			goto err;
		nvqs++;
		rxq->rxq_vq->vq_done = vioif_rx_vq_done;
		rxq->rxq_vq->vq_done_ctx = (void *)rxq;
		rxq->rxq_stopping = true;

		txq->txq_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NET);
		txq->txq_deferred_transmit = softint_establish(softint_flags,
		    vioif_deferred_transmit, txq);
		if (txq->txq_deferred_transmit == NULL) {
			aprint_error_dev(self, "cannot establish tx softint\n");
			goto err;
		}
		snprintf(qname, sizeof(qname), "tx%d", i);
		r = virtio_alloc_vq(vsc, txq->txq_vq, nvqs,
		    sizeof(struct virtio_net_hdr)
		    + (ETHER_MAX_LEN - ETHER_HDR_LEN),
		    VIRTIO_NET_TX_MAXNSEGS + 1, qname);
		if (r != 0)
			goto err;
		nvqs++;
		txq->txq_vq->vq_done = vioif_tx_vq_done;
		txq->txq_vq->vq_done_ctx = (void *)txq;
		txq->txq_link_active = sc->sc_link_active;
		txq->txq_stopping = false;
		txq->txq_intrq = pcq_create(txq->txq_vq->vq_num, KM_SLEEP);
	}

	if (sc->sc_has_ctrl) {
		/*
		 * Allocating a virtqueue for control channel
		 */
		r = virtio_alloc_vq(vsc, ctrlq->ctrlq_vq, nvqs,
		    NBPG, 1, "control");
		if (r != 0) {
			aprint_error_dev(self, "failed to allocate "
			    "a virtqueue for control channel, error code %d\n",
			    r);

			sc->sc_has_ctrl = false;
			cv_destroy(&ctrlq->ctrlq_wait);
			mutex_destroy(&ctrlq->ctrlq_wait_lock);
		} else {
			nvqs++;
			ctrlq->ctrlq_vq->vq_done = vioif_ctrl_vq_done;
			ctrlq->ctrlq_vq->vq_done_ctx = (void *) ctrlq;
		}
	}

	sc->sc_ctl_softint = softint_establish(softint_flags,
	    vioif_ctl_softint, sc);
	if (sc->sc_ctl_softint == NULL) {
		aprint_error_dev(self, "cannot establish ctl softint\n");
		goto err;
	}

	if (vioif_alloc_mems(sc) < 0)
		goto err;

	if (virtio_child_attach_finish(vsc) != 0)
		goto err;

	strlcpy(ifp->if_xname, device_xname(self), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
#ifdef VIOIF_MPSAFE
	ifp->if_extflags = IFEF_MPSAFE;
#endif
	ifp->if_start = vioif_start;
	if (sc->sc_req_nvq_pairs > 1)
		ifp->if_transmit = vioif_transmit;
	ifp->if_ioctl = vioif_ioctl;
	ifp->if_init = vioif_init;
	ifp->if_stop = vioif_stop;
	ifp->if_capabilities = 0;
	ifp->if_watchdog = vioif_watchdog;
	txq = &sc->sc_txq[0];
	IFQ_SET_MAXLEN(&ifp->if_snd, MAX(txq->txq_vq->vq_num, IFQ_MAXLEN));
	IFQ_SET_READY(&ifp->if_snd);

	sc->sc_ethercom.ec_capabilities |= ETHERCAP_VLAN_MTU;

	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, sc->sc_mac);

	return;

err:
	for (i = 0; i < sc->sc_max_nvq_pairs; i++) {
		rxq = &sc->sc_rxq[i];
		txq = &sc->sc_txq[i];

		if (rxq->rxq_lock) {
			mutex_obj_free(rxq->rxq_lock);
			rxq->rxq_lock = NULL;
		}

		if (rxq->rxq_softint) {
			softint_disestablish(rxq->rxq_softint);
			rxq->rxq_softint = NULL;
		}

		if (txq->txq_lock) {
			mutex_obj_free(txq->txq_lock);
			txq->txq_lock = NULL;
		}

		if (txq->txq_deferred_transmit) {
			softint_disestablish(txq->txq_deferred_transmit);
			txq->txq_deferred_transmit = NULL;
		}

		if (txq->txq_intrq) {
			pcq_destroy(txq->txq_intrq);
			txq->txq_intrq = NULL;
		}
	}

	if (sc->sc_has_ctrl) {
		cv_destroy(&ctrlq->ctrlq_wait);
		mutex_destroy(&ctrlq->ctrlq_wait_lock);
	}

	while (nvqs > 0)
		virtio_free_vq(vsc, &sc->sc_vqs[--nvqs]);

	vioif_free_queues(sc);

	virtio_child_attach_failed(vsc);
	return;
}

/* we need interrupts to make promiscuous mode off */
static void
vioif_deferred_init(device_t self)
{
	struct vioif_softc *sc = device_private(self);
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int r;

	if (ifp->if_flags & IFF_PROMISC)
		return;

	r =  vioif_set_promisc(sc, false);
	if (r != 0)
		aprint_error_dev(self, "resetting promisc mode failed, "
		    "error code %d\n", r);
}

static void
vioif_enable_interrupt_vqpairs(struct vioif_softc *sc)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	struct vioif_txqueue *txq;
	struct vioif_rxqueue *rxq;
	int i;

	for (i = 0; i < sc->sc_act_nvq_pairs; i++) {
		txq = &sc->sc_txq[i];
		rxq = &sc->sc_rxq[i];

		virtio_start_vq_intr(vsc, txq->txq_vq);
		virtio_start_vq_intr(vsc, rxq->rxq_vq);
	}
}

static void
vioif_disable_interrupt_vqpairs(struct vioif_softc *sc)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	struct vioif_txqueue *txq;
	struct vioif_rxqueue *rxq;
	int i;

	for (i = 0; i < sc->sc_act_nvq_pairs; i++) {
		txq = &sc->sc_txq[i];
		rxq = &sc->sc_rxq[i];

		virtio_stop_vq_intr(vsc, txq->txq_vq);
		virtio_stop_vq_intr(vsc, rxq->rxq_vq);
	}
}

/*
 * Interface functions for ifnet
 */
static int
vioif_init(struct ifnet *ifp)
{
	struct vioif_softc *sc = ifp->if_softc;
	struct virtio_softc *vsc = sc->sc_virtio;
	struct vioif_rxqueue *rxq;
	struct vioif_ctrlqueue *ctrlq = &sc->sc_ctrlq;
	int r, i;

	vioif_stop(ifp, 0);

	virtio_reinit_start(vsc);
	virtio_negotiate_features(vsc, virtio_features(vsc));

	for (i = 0; i < sc->sc_req_nvq_pairs; i++) {
		rxq = &sc->sc_rxq[i];

		/* Have to set false before vioif_populate_rx_mbufs */
		rxq->rxq_stopping = false;
		vioif_populate_rx_mbufs(rxq);
	}

	virtio_reinit_end(vsc);

	if (sc->sc_has_ctrl)
		virtio_start_vq_intr(vsc, ctrlq->ctrlq_vq);

	r = vioif_ctrl_mq_vq_pairs_set(sc, sc->sc_req_nvq_pairs);
	if (r == 0)
		sc->sc_act_nvq_pairs = sc->sc_req_nvq_pairs;
	else
		sc->sc_act_nvq_pairs = 1;

	for (i = 0; i < sc->sc_act_nvq_pairs; i++)
		sc->sc_txq[i].txq_stopping = false;

	vioif_enable_interrupt_vqpairs(sc);

	if (!sc->sc_deferred_init_done) {
		sc->sc_deferred_init_done = 1;
		if (sc->sc_has_ctrl)
			vioif_deferred_init(sc->sc_dev);
	}

	vioif_update_link_status(sc);
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	vioif_rx_filter(sc);

	return 0;
}

static void
vioif_stop(struct ifnet *ifp, int disable)
{
	struct vioif_softc *sc = ifp->if_softc;
	struct virtio_softc *vsc = sc->sc_virtio;
	struct vioif_txqueue *txq;
	struct vioif_rxqueue *rxq;
	struct vioif_ctrlqueue *ctrlq = &sc->sc_ctrlq;
	int i;

	/* Take the locks to ensure that ongoing TX/RX finish */
	for (i = 0; i < sc->sc_act_nvq_pairs; i++) {
		txq = &sc->sc_txq[i];
		rxq = &sc->sc_rxq[i];

		mutex_enter(txq->txq_lock);
		txq->txq_stopping = true;
		mutex_exit(txq->txq_lock);

		mutex_enter(rxq->rxq_lock);
		rxq->rxq_stopping = true;
		mutex_exit(rxq->rxq_lock);
	}

	/* disable interrupts */
	vioif_disable_interrupt_vqpairs(sc);

	if (sc->sc_has_ctrl)
		virtio_stop_vq_intr(vsc, ctrlq->ctrlq_vq);

	/* only way to stop I/O and DMA is resetting... */
	virtio_reset(vsc);
	for (i = 0; i < sc->sc_act_nvq_pairs; i++)
		vioif_rx_deq(&sc->sc_rxq[i]);

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	sc->sc_link_active = false;

	for (i = 0; i < sc->sc_act_nvq_pairs; i++) {
		txq = &sc->sc_txq[i];
		rxq = &sc->sc_rxq[i];

		txq->txq_link_active = false;

		if (disable)
			vioif_rx_drain(rxq);

		vioif_tx_drain(txq);
	}
}

static void
vioif_send_common_locked(struct ifnet *ifp, struct vioif_txqueue *txq,
    bool is_transmit)
{
	struct vioif_softc *sc = ifp->if_softc;
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtqueue *vq = txq->txq_vq;
	struct mbuf *m;
	int queued = 0;

	KASSERT(mutex_owned(txq->txq_lock));

	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return;

	if (!txq->txq_link_active || txq->txq_stopping)
		return;

	if ((ifp->if_flags & IFF_OACTIVE) != 0 && !is_transmit)
		return;

	for (;;) {
		int slot, r;

		if (is_transmit)
			m = pcq_get(txq->txq_intrq);
		else
			IFQ_DEQUEUE(&ifp->if_snd, m);

		if (m == NULL)
			break;

		r = virtio_enqueue_prep(vsc, vq, &slot);
		if (r == EAGAIN) {
			ifp->if_flags |= IFF_OACTIVE;
			m_freem(m);
			break;
		}
		if (r != 0)
			panic("enqueue_prep for a tx buffer");

		r = bus_dmamap_load_mbuf(virtio_dmat(vsc),
		    txq->txq_dmamaps[slot], m, BUS_DMA_WRITE | BUS_DMA_NOWAIT);
		if (r != 0) {
			/* maybe just too fragmented */
			struct mbuf *newm;

			newm = m_defrag(m, M_NOWAIT);
			if (newm == NULL) {
				aprint_error_dev(sc->sc_dev,
				    "m_defrag() failed\n");
				goto skip;
			}

			m = newm;
			r = bus_dmamap_load_mbuf(virtio_dmat(vsc),
			    txq->txq_dmamaps[slot], m,
			    BUS_DMA_WRITE | BUS_DMA_NOWAIT);
			if (r != 0) {
				aprint_error_dev(sc->sc_dev,
				    "tx dmamap load failed, error code %d\n",
				    r);
skip:
				m_freem(m);
				virtio_enqueue_abort(vsc, vq, slot);
				continue;
			}
		}

		/* This should actually never fail */
		r = virtio_enqueue_reserve(vsc, vq, slot,
					txq->txq_dmamaps[slot]->dm_nsegs + 1);
		if (r != 0) {
			aprint_error_dev(sc->sc_dev,
			    "virtio_enqueue_reserve failed, error code %d\n",
			    r);
			bus_dmamap_unload(virtio_dmat(vsc),
					  txq->txq_dmamaps[slot]);
			/* slot already freed by virtio_enqueue_reserve */
			m_freem(m);
			continue;
		}

		txq->txq_mbufs[slot] = m;

		memset(&txq->txq_hdrs[slot], 0, sizeof(struct virtio_net_hdr));
		bus_dmamap_sync(virtio_dmat(vsc), txq->txq_dmamaps[slot],
		    0, txq->txq_dmamaps[slot]->dm_mapsize,
		    BUS_DMASYNC_PREWRITE);
		bus_dmamap_sync(virtio_dmat(vsc), txq->txq_hdr_dmamaps[slot],
		    0, txq->txq_hdr_dmamaps[slot]->dm_mapsize,
		    BUS_DMASYNC_PREWRITE);
		virtio_enqueue(vsc, vq, slot, txq->txq_hdr_dmamaps[slot], true);
		virtio_enqueue(vsc, vq, slot, txq->txq_dmamaps[slot], true);
		virtio_enqueue_commit(vsc, vq, slot, false);

		queued++;
		bpf_mtap(ifp, m, BPF_D_OUT);
	}

	if (queued > 0) {
		virtio_enqueue_commit(vsc, vq, -1, true);
		ifp->if_timer = 5;
	}
}

static void
vioif_start_locked(struct ifnet *ifp, struct vioif_txqueue *txq)
{

	/*
	 * ifp->if_obytes and ifp->if_omcasts are added in if_transmit()@if.c.
	 */
	vioif_send_common_locked(ifp, txq, false);

}

static void
vioif_start(struct ifnet *ifp)
{
	struct vioif_softc *sc = ifp->if_softc;
	struct vioif_txqueue *txq = &sc->sc_txq[0];

#ifdef VIOIF_MPSAFE
	KASSERT(if_is_mpsafe(ifp));
#endif

	mutex_enter(txq->txq_lock);
	if (!txq->txq_stopping)
		vioif_start_locked(ifp, txq);
	mutex_exit(txq->txq_lock);
}

static inline int
vioif_select_txqueue(struct ifnet *ifp, struct mbuf *m)
{
	struct vioif_softc *sc = ifp->if_softc;
	u_int cpuid = cpu_index(curcpu());

	return cpuid % sc->sc_act_nvq_pairs;
}

static void
vioif_transmit_locked(struct ifnet *ifp, struct vioif_txqueue *txq)
{

	vioif_send_common_locked(ifp, txq, true);
}

static int
vioif_transmit(struct ifnet *ifp, struct mbuf *m)
{
	struct vioif_softc *sc = ifp->if_softc;
	struct vioif_txqueue *txq;
	int qid;

	qid = vioif_select_txqueue(ifp, m);
	txq = &sc->sc_txq[qid];

	if (__predict_false(!pcq_put(txq->txq_intrq, m))) {
		m_freem(m);
		return ENOBUFS;
	}

	net_stat_ref_t nsr = IF_STAT_GETREF(ifp);
	if_statadd_ref(nsr, if_obytes, m->m_pkthdr.len);
	if (m->m_flags & M_MCAST)
		if_statinc_ref(nsr, if_omcasts);
	IF_STAT_PUTREF(ifp);

	if (mutex_tryenter(txq->txq_lock)) {
		if (!txq->txq_stopping)
			vioif_transmit_locked(ifp, txq);
		mutex_exit(txq->txq_lock);
	}

	return 0;
}

static void
vioif_deferred_transmit(void *arg)
{
	struct vioif_txqueue *txq = arg;
	struct virtio_softc *vsc = txq->txq_vq->vq_owner;
	struct vioif_softc *sc = device_private(virtio_child(vsc));
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	if (mutex_tryenter(txq->txq_lock)) {
		vioif_send_common_locked(ifp, txq, true);
		mutex_exit(txq->txq_lock);
	}
}

static int
vioif_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	int s, r;

	s = splnet();

	r = ether_ioctl(ifp, cmd, data);
	if ((r == 0 && cmd == SIOCSIFFLAGS) ||
	    (r == ENETRESET && (cmd == SIOCADDMULTI || cmd == SIOCDELMULTI))) {
		if (ifp->if_flags & IFF_RUNNING)
			r = vioif_rx_filter(ifp->if_softc);
		else
			r = 0;
	}

	splx(s);

	return r;
}

void
vioif_watchdog(struct ifnet *ifp)
{
	struct vioif_softc *sc = ifp->if_softc;
	int i;

	if (ifp->if_flags & IFF_RUNNING) {
		for (i = 0; i < sc->sc_act_nvq_pairs; i++)
			vioif_tx_vq_done(sc->sc_txq[i].txq_vq);
	}
}


/*
 * Receive implementation
 */
/* allocate and initialize a mbuf for receive */
static int
vioif_add_rx_mbuf(struct vioif_rxqueue *rxq, int i)
{
	struct virtio_softc *vsc = rxq->rxq_vq->vq_owner;
	struct mbuf *m;
	int r;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return ENOBUFS;
	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return ENOBUFS;
	}
	rxq->rxq_mbufs[i] = m;
	m->m_len = m->m_pkthdr.len = m->m_ext.ext_size;
	r = bus_dmamap_load_mbuf(virtio_dmat(vsc),
	    rxq->rxq_dmamaps[i], m, BUS_DMA_READ | BUS_DMA_NOWAIT);
	if (r) {
		m_freem(m);
		rxq->rxq_mbufs[i] = NULL;
		return r;
	}

	return 0;
}

/* free a mbuf for receive */
static void
vioif_free_rx_mbuf(struct vioif_rxqueue *rxq, int i)
{
	struct virtio_softc *vsc = rxq->rxq_vq->vq_owner;

	bus_dmamap_unload(virtio_dmat(vsc), rxq->rxq_dmamaps[i]);
	m_freem(rxq->rxq_mbufs[i]);
	rxq->rxq_mbufs[i] = NULL;
}

/* add mbufs for all the empty receive slots */
static void
vioif_populate_rx_mbufs(struct vioif_rxqueue *rxq)
{

	mutex_enter(rxq->rxq_lock);
	vioif_populate_rx_mbufs_locked(rxq);
	mutex_exit(rxq->rxq_lock);
}

static void
vioif_populate_rx_mbufs_locked(struct vioif_rxqueue *rxq)
{
	struct virtqueue *vq = rxq->rxq_vq;
	struct virtio_softc *vsc = vq->vq_owner;
	struct vioif_softc *sc = device_private(virtio_child(vsc));
	int i, r, ndone = 0;

	KASSERT(mutex_owned(rxq->rxq_lock));

	if (rxq->rxq_stopping)
		return;

	for (i = 0; i < vq->vq_num; i++) {
		int slot;
		r = virtio_enqueue_prep(vsc, vq, &slot);
		if (r == EAGAIN)
			break;
		if (r != 0)
			panic("enqueue_prep for rx buffers");
		if (rxq->rxq_mbufs[slot] == NULL) {
			r = vioif_add_rx_mbuf(rxq, slot);
			if (r != 0) {
				aprint_error_dev(sc->sc_dev,
				    "rx mbuf allocation failed, "
				    "error code %d\n", r);
				break;
			}
		}
		r = virtio_enqueue_reserve(vsc, vq, slot,
		    rxq->rxq_dmamaps[slot]->dm_nsegs + 1);
		if (r != 0) {
			vioif_free_rx_mbuf(rxq, slot);
			break;
		}
		bus_dmamap_sync(virtio_dmat(vsc), rxq->rxq_hdr_dmamaps[slot],
		    0, sizeof(struct virtio_net_hdr), BUS_DMASYNC_PREREAD);
		bus_dmamap_sync(virtio_dmat(vsc), rxq->rxq_dmamaps[slot],
		    0, MCLBYTES, BUS_DMASYNC_PREREAD);
		virtio_enqueue(vsc, vq, slot, rxq->rxq_hdr_dmamaps[slot],
		    false);
		virtio_enqueue(vsc, vq, slot, rxq->rxq_dmamaps[slot], false);
		virtio_enqueue_commit(vsc, vq, slot, false);
		ndone++;
	}
	if (ndone > 0)
		virtio_enqueue_commit(vsc, vq, -1, true);
}

/* dequeue received packets */
static int
vioif_rx_deq(struct vioif_rxqueue *rxq)
{
	int r;

	KASSERT(rxq->rxq_stopping);

	mutex_enter(rxq->rxq_lock);
	r = vioif_rx_deq_locked(rxq);
	mutex_exit(rxq->rxq_lock);

	return r;
}

/* dequeue received packets */
static int
vioif_rx_deq_locked(struct vioif_rxqueue *rxq)
{
	struct virtqueue *vq = rxq->rxq_vq;
	struct virtio_softc *vsc = vq->vq_owner;
	struct vioif_softc *sc = device_private(virtio_child(vsc));
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *m;
	int r = 0;
	int slot, len;

	KASSERT(mutex_owned(rxq->rxq_lock));

	while (virtio_dequeue(vsc, vq, &slot, &len) == 0) {
		len -= sizeof(struct virtio_net_hdr);
		r = 1;
		bus_dmamap_sync(virtio_dmat(vsc), rxq->rxq_hdr_dmamaps[slot],
		    0, sizeof(struct virtio_net_hdr), BUS_DMASYNC_POSTREAD);
		bus_dmamap_sync(virtio_dmat(vsc), rxq->rxq_dmamaps[slot],
		    0, MCLBYTES, BUS_DMASYNC_POSTREAD);
		m = rxq->rxq_mbufs[slot];
		KASSERT(m != NULL);
		bus_dmamap_unload(virtio_dmat(vsc), rxq->rxq_dmamaps[slot]);
		rxq->rxq_mbufs[slot] = NULL;
		virtio_dequeue_commit(vsc, vq, slot);
		m_set_rcvif(m, ifp);
		m->m_len = m->m_pkthdr.len = len;

		mutex_exit(rxq->rxq_lock);
		if_percpuq_enqueue(ifp->if_percpuq, m);
		mutex_enter(rxq->rxq_lock);

		if (rxq->rxq_stopping)
			break;
	}

	return r;
}

/* rx interrupt; call _dequeue above and schedule a softint */
static int
vioif_rx_vq_done(struct virtqueue *vq)
{
	struct vioif_rxqueue *rxq = vq->vq_done_ctx;
	int r = 0;

#ifdef VIOIF_SOFTINT_INTR
	KASSERT(!cpu_intr_p());
#endif

	mutex_enter(rxq->rxq_lock);

	if (rxq->rxq_stopping)
		goto out;

	r = vioif_rx_deq_locked(rxq);
	if (r)
#ifdef VIOIF_SOFTINT_INTR
		vioif_populate_rx_mbufs_locked(rxq);
#else
		softint_schedule(rxq->rxq_softint);
#endif

out:
	mutex_exit(rxq->rxq_lock);
	return r;
}

/* softint: enqueue receive requests for new incoming packets */
static void
vioif_rx_softint(void *arg)
{
	struct vioif_rxqueue *rxq = arg;

	vioif_populate_rx_mbufs(rxq);
}

/* free all the mbufs; called from if_stop(disable) */
static void
vioif_rx_drain(struct vioif_rxqueue *rxq)
{
	struct virtqueue *vq = rxq->rxq_vq;
	int i;

	for (i = 0; i < vq->vq_num; i++) {
		if (rxq->rxq_mbufs[i] == NULL)
			continue;
		vioif_free_rx_mbuf(rxq, i);
	}
}


/*
 * Transmition implementation
 */
/* actual transmission is done in if_start */
/* tx interrupt; dequeue and free mbufs */
/*
 * tx interrupt is actually disabled; this should be called upon
 * tx vq full and watchdog
 */
static int
vioif_tx_vq_done(struct virtqueue *vq)
{
	struct virtio_softc *vsc = vq->vq_owner;
	struct vioif_softc *sc = device_private(virtio_child(vsc));
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct vioif_txqueue *txq = vq->vq_done_ctx;
	int r = 0;

	mutex_enter(txq->txq_lock);

	if (txq->txq_stopping)
		goto out;

	r = vioif_tx_vq_done_locked(vq);

out:
	mutex_exit(txq->txq_lock);
	if (r) {
		if_schedule_deferred_start(ifp);

		KASSERT(txq->txq_deferred_transmit != NULL);
		softint_schedule(txq->txq_deferred_transmit);
	}
	return r;
}

static int
vioif_tx_vq_done_locked(struct virtqueue *vq)
{
	struct virtio_softc *vsc = vq->vq_owner;
	struct vioif_softc *sc = device_private(virtio_child(vsc));
	struct vioif_txqueue *txq = vq->vq_done_ctx;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *m;
	int r = 0;
	int slot, len;

	KASSERT(mutex_owned(txq->txq_lock));

	while (virtio_dequeue(vsc, vq, &slot, &len) == 0) {
		r++;
		bus_dmamap_sync(virtio_dmat(vsc), txq->txq_hdr_dmamaps[slot],
		    0, sizeof(struct virtio_net_hdr), BUS_DMASYNC_POSTWRITE);
		bus_dmamap_sync(virtio_dmat(vsc), txq->txq_dmamaps[slot],
		    0, txq->txq_dmamaps[slot]->dm_mapsize,
		    BUS_DMASYNC_POSTWRITE);
		m = txq->txq_mbufs[slot];
		bus_dmamap_unload(virtio_dmat(vsc), txq->txq_dmamaps[slot]);
		txq->txq_mbufs[slot] = NULL;
		virtio_dequeue_commit(vsc, vq, slot);
		if_statinc(ifp, if_opackets);
		m_freem(m);
	}

	if (r)
		ifp->if_flags &= ~IFF_OACTIVE;
	return r;
}

/* free all the mbufs already put on vq; called from if_stop(disable) */
static void
vioif_tx_drain(struct vioif_txqueue *txq)
{
	struct virtqueue *vq = txq->txq_vq;
	struct virtio_softc *vsc = vq->vq_owner;
	int i;

	KASSERT(txq->txq_stopping);

	for (i = 0; i < vq->vq_num; i++) {
		if (txq->txq_mbufs[i] == NULL)
			continue;
		bus_dmamap_unload(virtio_dmat(vsc), txq->txq_dmamaps[i]);
		m_freem(txq->txq_mbufs[i]);
		txq->txq_mbufs[i] = NULL;
	}
}

/*
 * Control vq
 */
/* issue a VIRTIO_NET_CTRL_RX class command and wait for completion */
static void
vioif_ctrl_acquire(struct vioif_softc *sc)
{
	struct vioif_ctrlqueue *ctrlq = &sc->sc_ctrlq;

	mutex_enter(&ctrlq->ctrlq_wait_lock);
	while (ctrlq->ctrlq_inuse != FREE)
		cv_wait(&ctrlq->ctrlq_wait, &ctrlq->ctrlq_wait_lock);
	ctrlq->ctrlq_inuse = INUSE;
	ctrlq->ctrlq_owner = curlwp;
	mutex_exit(&ctrlq->ctrlq_wait_lock);
}

static void
vioif_ctrl_release(struct vioif_softc *sc)
{
	struct vioif_ctrlqueue *ctrlq = &sc->sc_ctrlq;

	KASSERT(ctrlq->ctrlq_inuse != FREE);
	KASSERT(ctrlq->ctrlq_owner == curlwp);

	mutex_enter(&ctrlq->ctrlq_wait_lock);
	ctrlq->ctrlq_inuse = FREE;
	ctrlq->ctrlq_owner = NULL;
	cv_signal(&ctrlq->ctrlq_wait);
	mutex_exit(&ctrlq->ctrlq_wait_lock);
}

static int
vioif_ctrl_load_cmdspec(struct vioif_softc *sc,
    struct vioif_ctrl_cmdspec *specs, int nspecs)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	int i, r, loaded;

	loaded = 0;
	for (i = 0; i < nspecs; i++) {
		r = bus_dmamap_load(virtio_dmat(vsc),
		    specs[i].dmamap, specs[i].buf, specs[i].bufsize,
		    NULL, BUS_DMA_WRITE | BUS_DMA_NOWAIT);
		if (r) {
			aprint_error_dev(sc->sc_dev, "control command dmamap"
			    " load failed, error code %d\n", r);
			goto err;
		}
		loaded++;

	}

	return r;

err:
	for (i = 0; i < loaded; i++) {
		bus_dmamap_unload(virtio_dmat(vsc), specs[i].dmamap);
	}

	return r;
}

static void
vioif_ctrl_unload_cmdspec(struct vioif_softc *sc,
    struct vioif_ctrl_cmdspec *specs, int nspecs)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	int i;

	for (i = 0; i < nspecs; i++) {
		bus_dmamap_unload(virtio_dmat(vsc), specs[i].dmamap);
	}
}

static int
vioif_ctrl_send_command(struct vioif_softc *sc, uint8_t class, uint8_t cmd,
    struct vioif_ctrl_cmdspec *specs, int nspecs)
{
	struct vioif_ctrlqueue *ctrlq = &sc->sc_ctrlq;
	struct virtqueue *vq = ctrlq->ctrlq_vq;
	struct virtio_softc *vsc = sc->sc_virtio;
	int i, r, slot;

	ctrlq->ctrlq_cmd->class = class;
	ctrlq->ctrlq_cmd->command = cmd;

	bus_dmamap_sync(virtio_dmat(vsc), ctrlq->ctrlq_cmd_dmamap,
	    0, sizeof(struct virtio_net_ctrl_cmd), BUS_DMASYNC_PREWRITE);
	for (i = 0; i < nspecs; i++) {
		bus_dmamap_sync(virtio_dmat(vsc), specs[i].dmamap,
		    0, specs[i].bufsize, BUS_DMASYNC_PREWRITE);
	}
	bus_dmamap_sync(virtio_dmat(vsc), ctrlq->ctrlq_status_dmamap,
	    0, sizeof(struct virtio_net_ctrl_status), BUS_DMASYNC_PREREAD);

	r = virtio_enqueue_prep(vsc, vq, &slot);
	if (r != 0)
		panic("%s: control vq busy!?", device_xname(sc->sc_dev));
	r = virtio_enqueue_reserve(vsc, vq, slot, nspecs + 2);
	if (r != 0)
		panic("%s: control vq busy!?", device_xname(sc->sc_dev));
	virtio_enqueue(vsc, vq, slot, ctrlq->ctrlq_cmd_dmamap, true);
	for (i = 0; i < nspecs; i++) {
		virtio_enqueue(vsc, vq, slot, specs[i].dmamap, true);
	}
	virtio_enqueue(vsc, vq, slot, ctrlq->ctrlq_status_dmamap, false);
	virtio_enqueue_commit(vsc, vq, slot, true);

	/* wait for done */
	mutex_enter(&ctrlq->ctrlq_wait_lock);
	while (ctrlq->ctrlq_inuse != DONE)
		cv_wait(&ctrlq->ctrlq_wait, &ctrlq->ctrlq_wait_lock);
	mutex_exit(&ctrlq->ctrlq_wait_lock);
	/* already dequeueued */

	bus_dmamap_sync(virtio_dmat(vsc), ctrlq->ctrlq_cmd_dmamap, 0,
	    sizeof(struct virtio_net_ctrl_cmd), BUS_DMASYNC_POSTWRITE);
	for (i = 0; i < nspecs; i++) {
		bus_dmamap_sync(virtio_dmat(vsc), specs[i].dmamap, 0,
		    specs[i].bufsize, BUS_DMASYNC_POSTWRITE);
	}
	bus_dmamap_sync(virtio_dmat(vsc), ctrlq->ctrlq_status_dmamap, 0,
	    sizeof(struct virtio_net_ctrl_status), BUS_DMASYNC_POSTREAD);

	if (ctrlq->ctrlq_status->ack == VIRTIO_NET_OK)
		r = 0;
	else {
		aprint_error_dev(sc->sc_dev, "failed setting rx mode\n");
		r = EIO;
	}

	return r;
}

static int
vioif_ctrl_rx(struct vioif_softc *sc, int cmd, bool onoff)
{
	struct virtio_net_ctrl_rx *rx = sc->sc_ctrlq.ctrlq_rx;
	struct vioif_ctrl_cmdspec specs[1];
	int r;

	if (!sc->sc_has_ctrl)
		return ENOTSUP;

	vioif_ctrl_acquire(sc);

	rx->onoff = onoff;
	specs[0].dmamap = sc->sc_ctrlq.ctrlq_rx_dmamap;
	specs[0].buf = rx;
	specs[0].bufsize = sizeof(*rx);

	r = vioif_ctrl_send_command(sc, VIRTIO_NET_CTRL_RX, cmd,
	    specs, __arraycount(specs));

	vioif_ctrl_release(sc);
	return r;
}

static int
vioif_set_promisc(struct vioif_softc *sc, bool onoff)
{
	return vioif_ctrl_rx(sc, VIRTIO_NET_CTRL_RX_PROMISC, onoff);
}

static int
vioif_set_allmulti(struct vioif_softc *sc, bool onoff)
{
	return vioif_ctrl_rx(sc, VIRTIO_NET_CTRL_RX_ALLMULTI, onoff);
}

/* issue VIRTIO_NET_CTRL_MAC_TABLE_SET command and wait for completion */
static int
vioif_set_rx_filter(struct vioif_softc *sc)
{
	/* filter already set in ctrlq->ctrlq_mac_tbl */
	struct virtio_net_ctrl_mac_tbl *mac_tbl_uc, *mac_tbl_mc;
	struct vioif_ctrl_cmdspec specs[2];
	int nspecs = __arraycount(specs);
	int r;

	mac_tbl_uc = sc->sc_ctrlq.ctrlq_mac_tbl_uc;
	mac_tbl_mc = sc->sc_ctrlq.ctrlq_mac_tbl_mc;

	if (!sc->sc_has_ctrl)
		return ENOTSUP;

	vioif_ctrl_acquire(sc);

	specs[0].dmamap = sc->sc_ctrlq.ctrlq_tbl_uc_dmamap;
	specs[0].buf = mac_tbl_uc;
	specs[0].bufsize = sizeof(*mac_tbl_uc)
	    + (ETHER_ADDR_LEN * mac_tbl_uc->nentries);

	specs[1].dmamap = sc->sc_ctrlq.ctrlq_tbl_mc_dmamap;
	specs[1].buf = mac_tbl_mc;
	specs[1].bufsize = sizeof(*mac_tbl_mc)
	    + (ETHER_ADDR_LEN * mac_tbl_mc->nentries);

	r = vioif_ctrl_load_cmdspec(sc, specs, nspecs);
	if (r != 0)
		goto out;

	r = vioif_ctrl_send_command(sc,
	    VIRTIO_NET_CTRL_MAC, VIRTIO_NET_CTRL_MAC_TABLE_SET,
	    specs, nspecs);

	vioif_ctrl_unload_cmdspec(sc, specs, nspecs);

out:
	vioif_ctrl_release(sc);

	return r;
}

static int
vioif_ctrl_mq_vq_pairs_set(struct vioif_softc *sc, int nvq_pairs)
{
	struct virtio_net_ctrl_mq *mq = sc->sc_ctrlq.ctrlq_mq;
	struct vioif_ctrl_cmdspec specs[1];
	int r;

	if (!sc->sc_has_ctrl)
		return ENOTSUP;

	if (nvq_pairs <= 1)
		return EINVAL;

	vioif_ctrl_acquire(sc);

	mq->virtqueue_pairs = nvq_pairs;
	specs[0].dmamap = sc->sc_ctrlq.ctrlq_mq_dmamap;
	specs[0].buf = mq;
	specs[0].bufsize = sizeof(*mq);

	r = vioif_ctrl_send_command(sc,
	    VIRTIO_NET_CTRL_MQ, VIRTIO_NET_CTRL_MQ_VQ_PAIRS_SET,
	    specs, __arraycount(specs));

	vioif_ctrl_release(sc);

	return r;
}

/* ctrl vq interrupt; wake up the command issuer */
static int
vioif_ctrl_vq_done(struct virtqueue *vq)
{
	struct virtio_softc *vsc = vq->vq_owner;
	struct vioif_ctrlqueue *ctrlq = vq->vq_done_ctx;
	int r, slot;

	r = virtio_dequeue(vsc, vq, &slot, NULL);
	if (r == ENOENT)
		return 0;
	virtio_dequeue_commit(vsc, vq, slot);

	mutex_enter(&ctrlq->ctrlq_wait_lock);
	ctrlq->ctrlq_inuse = DONE;
	cv_signal(&ctrlq->ctrlq_wait);
	mutex_exit(&ctrlq->ctrlq_wait_lock);

	return 1;
}

/*
 * If IFF_PROMISC requested,  set promiscuous
 * If multicast filter small enough (<=MAXENTRIES) set rx filter
 * If large multicast filter exist use ALLMULTI
 */
/*
 * If setting rx filter fails fall back to ALLMULTI
 * If ALLMULTI fails fall back to PROMISC
 */
static int
vioif_rx_filter(struct vioif_softc *sc)
{
	struct ethercom *ec = &sc->sc_ethercom;
	struct ifnet *ifp = &ec->ec_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	struct vioif_ctrlqueue *ctrlq = &sc->sc_ctrlq;
	int nentries;
	int promisc = 0, allmulti = 0, rxfilter = 0;
	int r;

	if (!sc->sc_has_ctrl) {	/* no ctrl vq; always promisc */
		ifp->if_flags |= IFF_PROMISC;
		return 0;
	}

	if (ifp->if_flags & IFF_PROMISC) {
		promisc = 1;
		goto set;
	}

	nentries = -1;
	ETHER_LOCK(ec);
	ETHER_FIRST_MULTI(step, ec, enm);
	while (nentries++, enm != NULL) {
		if (nentries >= VIRTIO_NET_CTRL_MAC_MAXENTRIES) {
			allmulti = 1;
			goto set_unlock;
		}
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			allmulti = 1;
			goto set_unlock;
		}
		memcpy(ctrlq->ctrlq_mac_tbl_mc->macs[nentries],
		    enm->enm_addrlo, ETHER_ADDR_LEN);
		ETHER_NEXT_MULTI(step, enm);
	}
	rxfilter = 1;

set_unlock:
	ETHER_UNLOCK(ec);

set:
	if (rxfilter) {
		ctrlq->ctrlq_mac_tbl_uc->nentries = 0;
		ctrlq->ctrlq_mac_tbl_mc->nentries = nentries;
		r = vioif_set_rx_filter(sc);
		if (r != 0) {
			rxfilter = 0;
			allmulti = 1; /* fallback */
		}
	} else {
		/* remove rx filter */
		ctrlq->ctrlq_mac_tbl_uc->nentries = 0;
		ctrlq->ctrlq_mac_tbl_mc->nentries = 0;
		r = vioif_set_rx_filter(sc);
		/* what to do on failure? */
	}
	if (allmulti) {
		r = vioif_set_allmulti(sc, true);
		if (r != 0) {
			allmulti = 0;
			promisc = 1; /* fallback */
		}
	} else {
		r = vioif_set_allmulti(sc, false);
		/* what to do on failure? */
	}
	if (promisc) {
		r = vioif_set_promisc(sc, true);
	} else {
		r = vioif_set_promisc(sc, false);
	}

	return r;
}

static bool
vioif_is_link_up(struct vioif_softc *sc)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	uint16_t status;

	if (virtio_features(vsc) & VIRTIO_NET_F_STATUS)
		status = virtio_read_device_config_2(vsc,
		    VIRTIO_NET_CONFIG_STATUS);
	else
		status = VIRTIO_NET_S_LINK_UP;

	return ((status & VIRTIO_NET_S_LINK_UP) != 0);
}

/* change link status */
static void
vioif_update_link_status(struct vioif_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct vioif_txqueue *txq;
	bool active, changed;
	int link, i;

	active = vioif_is_link_up(sc);
	changed = false;

	if (active) {
		if (!sc->sc_link_active)
			changed = true;

		link = LINK_STATE_UP;
		sc->sc_link_active = true;
	} else {
		if (sc->sc_link_active)
			changed = true;

		link = LINK_STATE_DOWN;
		sc->sc_link_active = false;
	}

	if (changed) {
		for (i = 0; i < sc->sc_act_nvq_pairs; i++) {
			txq = &sc->sc_txq[i];

			mutex_enter(txq->txq_lock);
			txq->txq_link_active = sc->sc_link_active;
			mutex_exit(txq->txq_lock);
		}

		if_link_state_change(ifp, link);
	}
}

static int
vioif_config_change(struct virtio_softc *vsc)
{
	struct vioif_softc *sc = device_private(virtio_child(vsc));

#ifdef VIOIF_SOFTINT_INTR
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
#endif

#ifdef VIOIF_SOFTINT_INTR
	KASSERT(!cpu_intr_p());
	vioif_update_link_status(sc);
	vioif_start(ifp);
#else
	softint_schedule(sc->sc_ctl_softint);
#endif

	return 0;
}

static void
vioif_ctl_softint(void *arg)
{
	struct vioif_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	vioif_update_link_status(sc);
	vioif_start(ifp);
}

MODULE(MODULE_CLASS_DRIVER, if_vioif, "virtio");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
if_vioif_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;

#ifdef _MODULE
	switch (cmd) {
	case MODULE_CMD_INIT:
		error = config_init_component(cfdriver_ioconf_if_vioif,
		    cfattach_ioconf_if_vioif, cfdata_ioconf_if_vioif);
		break;
	case MODULE_CMD_FINI:
		error = config_fini_component(cfdriver_ioconf_if_vioif,
		    cfattach_ioconf_if_vioif, cfdata_ioconf_if_vioif);
		break;
	default:
		error = ENOTTY;
		break;
	}
#endif

	return error;
}

/*	$NetBSD: if_vioif.c,v 1.109 2023/05/13 11:19:19 andvar Exp $	*/

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: if_vioif.c,v 1.109 2023/05/13 11:19:19 andvar Exp $");

#ifdef _KERNEL_OPT
#include "opt_net_mpsafe.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/bus.h>
#include <sys/condvar.h>
#include <sys/device.h>
#include <sys/evcnt.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <sys/sockio.h>
#include <sys/syslog.h>
#include <sys/cpu.h>
#include <sys/module.h>
#include <sys/pcq.h>
#include <sys/workqueue.h>
#include <sys/xcall.h>

#include <dev/pci/virtioreg.h>
#include <dev/pci/virtiovar.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <net/bpf.h>

#include "ioconf.h"

#ifdef NET_MPSAFE
#define VIOIF_MPSAFE	1
#define VIOIF_MULTIQ	1
#endif

/*
 * if_vioifreg.h:
 */
/* Configuration registers */
#define VIRTIO_NET_CONFIG_MAC		 0 /* 8bit x 6byte */
#define VIRTIO_NET_CONFIG_STATUS	 6 /* 16bit */
#define VIRTIO_NET_CONFIG_MAX_VQ_PAIRS	 8 /* 16bit */
#define VIRTIO_NET_CONFIG_MTU		10 /* 16bit */

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
#define VIRTIO_NET_F_CTRL_MAC_ADDR 	__BIT(23)

#define VIRTIO_NET_FLAG_BITS			\
	VIRTIO_COMMON_FLAG_BITS			\
	"b\x17" "CTRL_MAC\0"			\
	"b\x16" "MQ\0"				\
	"b\x15" "GUEST_ANNOUNCE\0"		\
	"b\x14" "CTRL_RX_EXTRA\0"		\
	"b\x13" "CTRL_VLAN\0"			\
	"b\x12" "CTRL_RX\0"			\
	"b\x11" "CTRL_VQ\0"			\
	"b\x10" "STATUS\0"			\
	"b\x0f" "MRG_RXBUF\0"			\
	"b\x0e" "HOST_UFO\0"			\
	"b\x0d" "HOST_ECN\0"			\
	"b\x0c" "HOST_TSO6\0"			\
	"b\x0b" "HOST_TSO4\0"			\
	"b\x0a" "GUEST_UFO\0"			\
	"b\x09" "GUEST_ECN\0"			\
	"b\x08" "GUEST_TSO6\0"			\
	"b\x07" "GUEST_TSO4\0"			\
	"b\x06" "GSO\0"				\
	"b\x05" "MAC\0"				\
	"b\x01" "GUEST_CSUM\0"			\
	"b\x00" "CSUM\0"

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

	uint16_t	num_buffers; /* VIRTIO_NET_F_MRG_RXBUF enabled or v1 */
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
# define  VIRTIO_NET_CTRL_MAC_ADDR_SET	1

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

struct virtio_net_ctrl_mac_addr {
	uint8_t mac[ETHER_ADDR_LEN];
} __packed;

struct virtio_net_ctrl_vlan {
	uint16_t id;
} __packed;

struct virtio_net_ctrl_mq {
	uint16_t virtqueue_pairs;
} __packed;

/*
 * if_vioifvar.h:
 */

/*
 * Locking notes:
 * + a field in vioif_netqueue is protected by netq_lock (a spin mutex)
 *      - more than one lock cannot be held at onece
 * + a field in vioif_tx_context and vioif_rx_context is also protected
 *   by netq_lock.
 * + ctrlq_inuse is protected by ctrlq_wait_lock.
 *      - other fields in vioif_ctrlqueue are protected by ctrlq_inuse
 *      - netq_lock cannot be held along with ctrlq_wait_lock
 * + fields in vioif_softc except queues are protected by
 *   sc->sc_lock(an adaptive mutex)
 *      - the lock is held before acquisition of other locks
 */

struct vioif_ctrl_cmdspec {
	bus_dmamap_t	dmamap;
	void		*buf;
	bus_size_t	bufsize;
};

struct vioif_work {
	struct work	 cookie;
	void		(*func)(void *);
	void		*arg;
	unsigned int	 added;
};

struct vioif_net_map {
	struct virtio_net_hdr	*vnm_hdr;
	bus_dmamap_t		 vnm_hdr_map;
	struct mbuf		*vnm_mbuf;
	bus_dmamap_t		 vnm_mbuf_map;
};

#define VIOIF_NETQ_RX		0
#define VIOIF_NETQ_TX		1
#define VIOIF_NETQ_IDX		2
#define VIOIF_NETQ_DIR(n)	((n) % VIOIF_NETQ_IDX)
#define VIOIF_NETQ_PAIRIDX(n)	((n) / VIOIF_NETQ_IDX)
#define VIOIF_NETQ_RXQID(n)	((n) * VIOIF_NETQ_IDX + VIOIF_NETQ_RX)
#define VIOIF_NETQ_TXQID(n)	((n) * VIOIF_NETQ_IDX + VIOIF_NETQ_TX)

struct vioif_netqueue {
	kmutex_t		 netq_lock;
	struct virtqueue	*netq_vq;
	bool			 netq_stopping;
	bool			 netq_running_handle;
	void			*netq_maps_kva;
	struct vioif_net_map	*netq_maps;

	void			*netq_softint;
	struct vioif_work	 netq_work;
	bool			 netq_workqueue;

	char			 netq_evgroup[32];
	struct evcnt		 netq_mbuf_load_failed;
	struct evcnt		 netq_enqueue_failed;

	void			*netq_ctx;
};

struct vioif_tx_context {
	bool			 txc_link_active;
	bool			 txc_no_free_slots;
	pcq_t			*txc_intrq;
	void			*txc_deferred_transmit;

	struct evcnt		 txc_defrag_failed;
};

struct vioif_rx_context {
	struct evcnt		 rxc_mbuf_enobufs;
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
	struct virtio_net_ctrl_mac_addr	*ctrlq_mac_addr;
	struct virtio_net_ctrl_mq	*ctrlq_mq;

	bus_dmamap_t			ctrlq_cmd_dmamap;
	bus_dmamap_t			ctrlq_status_dmamap;
	bus_dmamap_t			ctrlq_rx_dmamap;
	bus_dmamap_t			ctrlq_tbl_uc_dmamap;
	bus_dmamap_t			ctrlq_tbl_mc_dmamap;
	bus_dmamap_t			ctrlq_mac_addr_dmamap;
	bus_dmamap_t			ctrlq_mq_dmamap;

	struct evcnt			ctrlq_cmd_load_failed;
	struct evcnt			ctrlq_cmd_failed;
};

struct vioif_softc {
	device_t		sc_dev;
	kmutex_t		sc_lock;
	struct sysctllog	*sc_sysctllog;

	struct virtio_softc	*sc_virtio;
	struct virtqueue	*sc_vqs;
	u_int			 sc_hdr_size;

	int			sc_max_nvq_pairs;
	int			sc_req_nvq_pairs;
	int			sc_act_nvq_pairs;

	uint8_t			sc_mac[ETHER_ADDR_LEN];
	struct ethercom		sc_ethercom;
	int			sc_link_state;

	struct vioif_netqueue	*sc_netqs;

	bool			sc_has_ctrl;
	struct vioif_ctrlqueue	sc_ctrlq;

	bus_dma_segment_t	 sc_segs[1];
	void			*sc_dmamem;
	void			*sc_kmem;

	void			*sc_cfg_softint;

	struct workqueue	*sc_txrx_workqueue;
	bool			 sc_txrx_workqueue_sysctl;
	u_int			 sc_tx_intr_process_limit;
	u_int			 sc_tx_process_limit;
	u_int			 sc_rx_intr_process_limit;
	u_int			 sc_rx_process_limit;
};
#define VIRTIO_NET_TX_MAXNSEGS		(16) /* XXX */
#define VIRTIO_NET_CTRL_MAC_MAXENTRIES	(64) /* XXX */

#define VIOIF_TX_INTR_PROCESS_LIMIT	256
#define VIOIF_TX_PROCESS_LIMIT		256
#define VIOIF_RX_INTR_PROCESS_LIMIT	0U
#define VIOIF_RX_PROCESS_LIMIT		256

#define VIOIF_WORKQUEUE_PRI		PRI_SOFTNET
#define VIOIF_IS_LINK_ACTIVE(_sc)	((_sc)->sc_link_state == LINK_STATE_UP ? \
					    true : false)

/* cfattach interface functions */
static int	vioif_match(device_t, cfdata_t, void *);
static void	vioif_attach(device_t, device_t, void *);
static int	vioif_finalize_teardown(device_t);

/* ifnet interface functions */
static int	vioif_init(struct ifnet *);
static void	vioif_stop(struct ifnet *, int);
static void	vioif_start(struct ifnet *);
static int	vioif_transmit(struct ifnet *, struct mbuf *);
static int	vioif_ioctl(struct ifnet *, u_long, void *);
static void	vioif_watchdog(struct ifnet *);
static int	vioif_ifflags(struct vioif_softc *);
static int	vioif_ifflags_cb(struct ethercom *);

/* tx & rx */
static int	vioif_netqueue_init(struct vioif_softc *,
		    struct virtio_softc *, size_t, u_int);
static void	vioif_netqueue_teardown(struct vioif_softc *,
		    struct virtio_softc *, size_t);
static void	vioif_net_intr_enable(struct vioif_softc *,
		    struct virtio_softc *);
static void	vioif_net_intr_disable(struct vioif_softc *,
		    struct virtio_softc *);
static void	vioif_net_sched_handle(struct vioif_softc *,
		    struct vioif_netqueue *);

/* rx */
static void	vioif_populate_rx_mbufs_locked(struct vioif_softc *,
		    struct vioif_netqueue *);
static int	vioif_rx_intr(void *);
static void	vioif_rx_handle(void *);
static void	vioif_rx_queue_clear(struct vioif_softc *,
		    struct virtio_softc *, struct vioif_netqueue *);

/* tx */
static void	vioif_start_locked(struct ifnet *, struct vioif_netqueue *);
static void	vioif_transmit_locked(struct ifnet *, struct vioif_netqueue *);
static void	vioif_deferred_transmit(void *);
static int	vioif_tx_intr(void *);
static void	vioif_tx_handle(void *);
static void	vioif_tx_queue_clear(struct vioif_softc *, struct virtio_softc *,
		    struct vioif_netqueue *);

/* controls */
static int	vioif_ctrl_intr(void *);
static int	vioif_ctrl_rx(struct vioif_softc *, int, bool);
static int	vioif_set_promisc(struct vioif_softc *, bool);
static int	vioif_set_allmulti(struct vioif_softc *, bool);
static int	vioif_set_rx_filter(struct vioif_softc *);
static int	vioif_rx_filter(struct vioif_softc *);
static int	vioif_set_mac_addr(struct vioif_softc *);
static int	vioif_ctrl_mq_vq_pairs_set(struct vioif_softc *, int);

/* config interrupt */
static int	vioif_config_change(struct virtio_softc *);
static void	vioif_cfg_softint(void *);
static void	vioif_update_link_status(struct vioif_softc *);

/* others */
static void	vioif_alloc_queues(struct vioif_softc *);
static void	vioif_free_queues(struct vioif_softc *);
static int	vioif_alloc_mems(struct vioif_softc *);
static struct workqueue*
		vioif_workq_create(const char *, pri_t, int, int);
static void	vioif_workq_destroy(struct workqueue *);
static void	vioif_work_set(struct vioif_work *, void(*)(void *), void *);
static void	vioif_work_add(struct workqueue *, struct vioif_work *);
static void	vioif_work_wait(struct workqueue *, struct vioif_work *);
static int	vioif_setup_sysctl(struct vioif_softc *);
static void	vioif_setup_stats(struct vioif_softc *);

CFATTACH_DECL_NEW(vioif, sizeof(struct vioif_softc),
		  vioif_match, vioif_attach, NULL, NULL);

static void
vioif_intr_barrier(void)
{

	/* wait for finish all interrupt handler */
	xc_barrier(0);
}

static void
vioif_notify(struct virtio_softc *vsc, struct virtqueue *vq)
{

	virtio_enqueue_commit(vsc, vq, -1, true);
}

static int
vioif_match(device_t parent, cfdata_t match, void *aux)
{
	struct virtio_attach_args *va = aux;

	if (va->sc_childdevid == VIRTIO_DEVICE_ID_NETWORK)
		return 1;

	return 0;
}

static void
vioif_attach(device_t parent, device_t self, void *aux)
{
	struct vioif_softc *sc = device_private(self);
	struct virtio_softc *vsc = device_private(parent);
	struct vioif_netqueue *txq0;
	struct vioif_ctrlqueue *ctrlq = &sc->sc_ctrlq;
	uint64_t features, req_features;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	u_int softint_flags;
	int r, i, req_flags;
	char xnamebuf[MAXCOMLEN];
	size_t nvqs;

	if (virtio_child(vsc) != NULL) {
		aprint_normal(": child already attached for %s; "
		    "something wrong...\n", device_xname(parent));
		return;
	}

	sc->sc_dev = self;
	sc->sc_virtio = vsc;
	sc->sc_link_state = LINK_STATE_UNKNOWN;

	sc->sc_max_nvq_pairs = 1;
	sc->sc_req_nvq_pairs = 1;
	sc->sc_act_nvq_pairs = 1;
	sc->sc_txrx_workqueue_sysctl = true;
	sc->sc_tx_intr_process_limit = VIOIF_TX_INTR_PROCESS_LIMIT;
	sc->sc_tx_process_limit = VIOIF_TX_PROCESS_LIMIT;
	sc->sc_rx_intr_process_limit = VIOIF_RX_INTR_PROCESS_LIMIT;
	sc->sc_rx_process_limit = VIOIF_RX_PROCESS_LIMIT;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	snprintf(xnamebuf, sizeof(xnamebuf), "%s_txrx", device_xname(self));
	sc->sc_txrx_workqueue = vioif_workq_create(xnamebuf, VIOIF_WORKQUEUE_PRI,
	    IPL_NET, WQ_PERCPU | WQ_MPSAFE);
	if (sc->sc_txrx_workqueue == NULL)
		goto err;

	req_flags = 0;

#ifdef VIOIF_MPSAFE
	req_flags |= VIRTIO_F_INTR_MPSAFE;
#endif
	req_flags |= VIRTIO_F_INTR_MSIX;

	req_features =
	    VIRTIO_NET_F_MAC | VIRTIO_NET_F_STATUS | VIRTIO_NET_F_CTRL_VQ |
	    VIRTIO_NET_F_CTRL_RX | VIRTIO_F_NOTIFY_ON_EMPTY;
	req_features |= VIRTIO_F_RING_EVENT_IDX;
	req_features |= VIRTIO_NET_F_CTRL_MAC_ADDR;
#ifdef VIOIF_MULTIQ
	req_features |= VIRTIO_NET_F_MQ;
#endif

	virtio_child_attach_start(vsc, self, IPL_NET,
	    req_features, VIRTIO_NET_FLAG_BITS);
	features = virtio_features(vsc);

	if (features == 0)
		goto err;

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

	/* 'Ethernet' with capital follows other ethernet driver attachment */
	aprint_normal_dev(self, "Ethernet address %s\n",
	    ether_sprintf(sc->sc_mac));

	if (features & (VIRTIO_NET_F_MRG_RXBUF | VIRTIO_F_VERSION_1)) {
		sc->sc_hdr_size = sizeof(struct virtio_net_hdr);
	} else {
		sc->sc_hdr_size = offsetof(struct virtio_net_hdr, num_buffers);
	}

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

		if (sc->sc_max_nvq_pairs > 1)
			req_flags |= VIRTIO_F_INTR_PERVQ;
	}

	vioif_alloc_queues(sc);

#ifdef VIOIF_MPSAFE
	softint_flags = SOFTINT_NET | SOFTINT_MPSAFE;
#else
	softint_flags = SOFTINT_NET;
#endif

	/*
	 * Initialize network queues
	 */
	nvqs = sc->sc_max_nvq_pairs * 2;
	for (i = 0; i < nvqs; i++) {
		r = vioif_netqueue_init(sc, vsc, i, softint_flags);
		if (r != 0)
			goto err;
	}

	if (sc->sc_has_ctrl) {
		int ctrlq_idx = nvqs;

		nvqs++;
		/*
		 * Allocating a virtqueue for control channel
		 */
		sc->sc_ctrlq.ctrlq_vq = &sc->sc_vqs[ctrlq_idx];
		virtio_init_vq(vsc, ctrlq->ctrlq_vq, ctrlq_idx,
		    vioif_ctrl_intr, ctrlq);

		r = virtio_alloc_vq(vsc, ctrlq->ctrlq_vq, NBPG, 1, "control");
		if (r != 0) {
			aprint_error_dev(self, "failed to allocate "
			    "a virtqueue for control channel, error code %d\n",
			    r);

			sc->sc_has_ctrl = false;
			cv_destroy(&ctrlq->ctrlq_wait);
			mutex_destroy(&ctrlq->ctrlq_wait_lock);
		}
	}

	sc->sc_cfg_softint = softint_establish(softint_flags,
	    vioif_cfg_softint, sc);
	if (sc->sc_cfg_softint == NULL) {
		aprint_error_dev(self, "cannot establish ctl softint\n");
		goto err;
	}

	if (vioif_alloc_mems(sc) < 0)
		goto err;

	r = virtio_child_attach_finish(vsc, sc->sc_vqs, nvqs,
	    vioif_config_change, req_flags);
	if (r != 0)
		goto err;

	if (vioif_setup_sysctl(sc) != 0) {
		aprint_error_dev(self, "unable to create sysctl node\n");
		/* continue */
	}

	vioif_setup_stats(sc);

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
	txq0 = &sc->sc_netqs[VIOIF_NETQ_TXQID(0)];
	IFQ_SET_MAXLEN(&ifp->if_snd, MAX(txq0->netq_vq->vq_num, IFQ_MAXLEN));
	IFQ_SET_READY(&ifp->if_snd);

	sc->sc_ethercom.ec_capabilities |= ETHERCAP_VLAN_MTU;

	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, sc->sc_mac);
	ether_set_ifflags_cb(&sc->sc_ethercom, vioif_ifflags_cb);

	return;

err:
	nvqs = sc->sc_max_nvq_pairs * 2;
	for (i = 0; i < nvqs; i++) {
		vioif_netqueue_teardown(sc, vsc, i);
	}

	if (sc->sc_has_ctrl) {
		cv_destroy(&ctrlq->ctrlq_wait);
		mutex_destroy(&ctrlq->ctrlq_wait_lock);
		virtio_free_vq(vsc, ctrlq->ctrlq_vq);
		ctrlq->ctrlq_vq = NULL;
	}

	vioif_free_queues(sc);
	mutex_destroy(&sc->sc_lock);
	virtio_child_attach_failed(vsc);
	config_finalize_register(self, vioif_finalize_teardown);

	return;
}

static int
vioif_finalize_teardown(device_t self)
{
	struct vioif_softc *sc = device_private(self);

	if (sc->sc_txrx_workqueue != NULL) {
		vioif_workq_destroy(sc->sc_txrx_workqueue);
		sc->sc_txrx_workqueue = NULL;
	}

	return 0;
}

/*
 * Interface functions for ifnet
 */
static int
vioif_init(struct ifnet *ifp)
{
	struct vioif_softc *sc = ifp->if_softc;
	struct virtio_softc *vsc = sc->sc_virtio;
	struct vioif_netqueue *netq;
	struct vioif_ctrlqueue *ctrlq = &sc->sc_ctrlq;
	int r, i;

	vioif_stop(ifp, 0);

	r = virtio_reinit_start(vsc);
	if (r != 0) {
		log(LOG_ERR, "%s: reset failed\n", ifp->if_xname);
		return EIO;
	}

	virtio_negotiate_features(vsc, virtio_features(vsc));

	for (i = 0; i < sc->sc_req_nvq_pairs; i++) {
		netq = &sc->sc_netqs[VIOIF_NETQ_RXQID(i)];

		mutex_enter(&netq->netq_lock);
		vioif_populate_rx_mbufs_locked(sc, netq);
		mutex_exit(&netq->netq_lock);
	}

	virtio_reinit_end(vsc);

	if (sc->sc_has_ctrl)
		virtio_start_vq_intr(vsc, ctrlq->ctrlq_vq);

	r = vioif_ctrl_mq_vq_pairs_set(sc, sc->sc_req_nvq_pairs);
	if (r == 0)
		sc->sc_act_nvq_pairs = sc->sc_req_nvq_pairs;
	else
		sc->sc_act_nvq_pairs = 1;

	SET(ifp->if_flags, IFF_RUNNING);

	vioif_net_intr_enable(sc, vsc);

	vioif_update_link_status(sc);
	r = vioif_rx_filter(sc);

	return r;
}

static void
vioif_stop(struct ifnet *ifp, int disable)
{
	struct vioif_softc *sc = ifp->if_softc;
	struct virtio_softc *vsc = sc->sc_virtio;
	struct vioif_netqueue *netq;
	struct vioif_ctrlqueue *ctrlq = &sc->sc_ctrlq;
	size_t i, act_qnum;

	act_qnum = sc->sc_act_nvq_pairs * 2;

	CLR(ifp->if_flags, IFF_RUNNING);
	for (i = 0; i < act_qnum; i++) {
		netq = &sc->sc_netqs[i];

		mutex_enter(&netq->netq_lock);
		netq->netq_stopping = true;
		mutex_exit(&netq->netq_lock);
	}

	/* disable interrupts */
	vioif_net_intr_disable(sc, vsc);
	if (sc->sc_has_ctrl)
		virtio_stop_vq_intr(vsc, ctrlq->ctrlq_vq);

	/*
	 * only way to stop interrupt, I/O and DMA is resetting...
	 *
	 * NOTE: Devices based on VirtIO draft specification can not
	 * stop interrupt completely even if virtio_stop_vq_intr() is called.
	 */
	virtio_reset(vsc);

	vioif_intr_barrier();

	for (i = 0; i < act_qnum; i++) {
		netq = &sc->sc_netqs[i];
		vioif_work_wait(sc->sc_txrx_workqueue, &netq->netq_work);
	}

	for (i = 0; i < sc->sc_act_nvq_pairs; i++) {
		netq = &sc->sc_netqs[VIOIF_NETQ_RXQID(i)];
		vioif_rx_queue_clear(sc, vsc, netq);

		netq = &sc->sc_netqs[VIOIF_NETQ_TXQID(i)];
		vioif_tx_queue_clear(sc, vsc, netq);
	}

	/* all packet processing is stopped */
	for (i = 0; i < act_qnum; i++) {
		netq = &sc->sc_netqs[i];

		mutex_enter(&netq->netq_lock);
		netq->netq_stopping = false;
		mutex_exit(&netq->netq_lock);
	}
}

static void
vioif_start(struct ifnet *ifp)
{
	struct vioif_softc *sc = ifp->if_softc;
	struct vioif_netqueue *txq0 = &sc->sc_netqs[VIOIF_NETQ_TXQID(0)];

#ifdef VIOIF_MPSAFE
	KASSERT(if_is_mpsafe(ifp));
#endif

	mutex_enter(&txq0->netq_lock);
	vioif_start_locked(ifp, txq0);
	mutex_exit(&txq0->netq_lock);
}

static inline int
vioif_select_txqueue(struct ifnet *ifp, struct mbuf *m)
{
	struct vioif_softc *sc = ifp->if_softc;
	u_int cpuid = cpu_index(curcpu());

	return VIOIF_NETQ_TXQID(cpuid % sc->sc_act_nvq_pairs);
}

static int
vioif_transmit(struct ifnet *ifp, struct mbuf *m)
{
	struct vioif_softc *sc = ifp->if_softc;
	struct vioif_netqueue *netq;
	struct vioif_tx_context *txc;
	int qid;

	qid = vioif_select_txqueue(ifp, m);
	netq = &sc->sc_netqs[qid];
	txc = netq->netq_ctx;

	if (__predict_false(!pcq_put(txc->txc_intrq, m))) {
		m_freem(m);
		return ENOBUFS;
	}

	net_stat_ref_t nsr = IF_STAT_GETREF(ifp);
	if_statadd_ref(nsr, if_obytes, m->m_pkthdr.len);
	if (m->m_flags & M_MCAST)
		if_statinc_ref(nsr, if_omcasts);
	IF_STAT_PUTREF(ifp);

	if (mutex_tryenter(&netq->netq_lock)) {
		vioif_transmit_locked(ifp, netq);
		mutex_exit(&netq->netq_lock);
	}

	return 0;
}

void
vioif_watchdog(struct ifnet *ifp)
{
	struct vioif_softc *sc = ifp->if_softc;
	struct vioif_netqueue *netq;
	int i;

	if (ISSET(ifp->if_flags, IFF_RUNNING)) {
		if (ISSET(ifp->if_flags, IFF_DEBUG)) {
			log(LOG_DEBUG, "%s: watchdog timed out\n",
			    ifp->if_xname);
		}

		for (i = 0; i < sc->sc_act_nvq_pairs; i++) {
			netq = &sc->sc_netqs[VIOIF_NETQ_TXQID(i)];

			mutex_enter(&netq->netq_lock);
			if (!netq->netq_running_handle) {
				netq->netq_running_handle = true;
				vioif_net_sched_handle(sc, netq);
			}
			mutex_exit(&netq->netq_lock);
		}
	}
}

static int
vioif_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	int s, r;

	s = splnet();

	r = ether_ioctl(ifp, cmd, data);
	if (r == ENETRESET && (cmd == SIOCADDMULTI || cmd == SIOCDELMULTI)) {
		if (ifp->if_flags & IFF_RUNNING) {
			r = vioif_rx_filter(ifp->if_softc);
		} else {
			r = 0;
		}
	}

	splx(s);

	return r;
}

static int
vioif_ifflags(struct vioif_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	bool onoff;
	int r;

	if (!sc->sc_has_ctrl) {
		/* no ctrl vq; always promisc and allmulti */
		ifp->if_flags |= (IFF_PROMISC | IFF_ALLMULTI);
		return 0;
	}

	onoff = ifp->if_flags & IFF_ALLMULTI ? true : false;
	r = vioif_set_allmulti(sc, onoff);
	if (r != 0) {
		log(LOG_WARNING,
		    "%s: couldn't %sable ALLMULTI\n",
		    ifp->if_xname, onoff ? "en" : "dis");
		if (onoff) {
			CLR(ifp->if_flags, IFF_ALLMULTI);
		} else {
			SET(ifp->if_flags, IFF_ALLMULTI);
		}
	}

	onoff = ifp->if_flags & IFF_PROMISC ? true : false;
	r = vioif_set_promisc(sc, onoff);
	if (r != 0) {
		log(LOG_WARNING,
		    "%s: couldn't %sable PROMISC\n",
		    ifp->if_xname, onoff ? "en" : "dis");
		if (onoff) {
			CLR(ifp->if_flags, IFF_PROMISC);
		} else {
			SET(ifp->if_flags, IFF_PROMISC);
		}
	}

	return 0;
}

static int
vioif_ifflags_cb(struct ethercom *ec)
{
	struct ifnet *ifp = &ec->ec_if;
	struct vioif_softc *sc = ifp->if_softc;

	return vioif_ifflags(sc);
}

static int
vioif_setup_sysctl(struct vioif_softc *sc)
{
	const char *devname;
	struct sysctllog **log;
	const struct sysctlnode *rnode, *rxnode, *txnode;
	int error;

	log = &sc->sc_sysctllog;
	devname = device_xname(sc->sc_dev);

	error = sysctl_createv(log, 0, NULL, &rnode,
	    0, CTLTYPE_NODE, devname,
	    SYSCTL_DESCR("virtio-net information and settings"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "txrx_workqueue",
	    SYSCTL_DESCR("Use workqueue for packet processing"),
	    NULL, 0, &sc->sc_txrx_workqueue_sysctl, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rnode, &rxnode,
	    0, CTLTYPE_NODE, "rx",
	    SYSCTL_DESCR("virtio-net information and settings for Rx"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rxnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "intr_process_limit",
	    SYSCTL_DESCR("max number of Rx packets to process for interrupt processing"),
	    NULL, 0, &sc->sc_rx_intr_process_limit, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rxnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "process_limit",
	    SYSCTL_DESCR("max number of Rx packets to process for deferred processing"),
	    NULL, 0, &sc->sc_rx_process_limit, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rnode, &txnode,
	    0, CTLTYPE_NODE, "tx",
	    SYSCTL_DESCR("virtio-net information and settings for Tx"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &txnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "intr_process_limit",
	    SYSCTL_DESCR("max number of Tx packets to process for interrupt processing"),
	    NULL, 0, &sc->sc_tx_intr_process_limit, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &txnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "process_limit",
	    SYSCTL_DESCR("max number of Tx packets to process for deferred processing"),
	    NULL, 0, &sc->sc_tx_process_limit, 0, CTL_CREATE, CTL_EOL);

out:
	if (error)
		sysctl_teardown(log);

	return error;
}

static void
vioif_setup_stats(struct vioif_softc *sc)
{
	struct vioif_netqueue *netq;
	struct vioif_tx_context *txc;
	struct vioif_rx_context *rxc;
	size_t i, netq_num;

	netq_num = sc->sc_max_nvq_pairs * 2;
	for (i = 0; i < netq_num; i++) {
		netq = &sc->sc_netqs[i];
		evcnt_attach_dynamic(&netq->netq_mbuf_load_failed, EVCNT_TYPE_MISC,
		    NULL, netq->netq_evgroup, "failed to load mbuf to DMA");
		evcnt_attach_dynamic(&netq->netq_enqueue_failed,
		    EVCNT_TYPE_MISC, NULL, netq->netq_evgroup,
		    "virtqueue enqueue failed failed");

		switch (VIOIF_NETQ_DIR(i)) {
		case VIOIF_NETQ_RX:
			rxc = netq->netq_ctx;
			evcnt_attach_dynamic(&rxc->rxc_mbuf_enobufs,
			    EVCNT_TYPE_MISC, NULL, netq->netq_evgroup,
			    "no receive buffer");
			break;
		case VIOIF_NETQ_TX:
			txc = netq->netq_ctx;
			evcnt_attach_dynamic(&txc->txc_defrag_failed,
			    EVCNT_TYPE_MISC, NULL, netq->netq_evgroup,
			    "m_defrag() failed");
			break;
		}
	}

	evcnt_attach_dynamic(&sc->sc_ctrlq.ctrlq_cmd_load_failed, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "control command dmamap load failed");
	evcnt_attach_dynamic(&sc->sc_ctrlq.ctrlq_cmd_failed, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "control command failed");
}

/*
 * allocate memory
 */
static int
vioif_dmamap_create(struct vioif_softc *sc, bus_dmamap_t *map,
    bus_size_t size, int nsegs, const char *usage)
{
	int r;

	r = bus_dmamap_create(virtio_dmat(sc->sc_virtio), size,
	    nsegs, size, 0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, map);

	if (r != 0) {
		aprint_error_dev(sc->sc_dev, "%s dmamap creation failed, "
		    "error code %d\n", usage, r);
	}

	return r;
}

static void
vioif_dmamap_destroy(struct vioif_softc *sc, bus_dmamap_t *map)
{

	if (*map) {
		bus_dmamap_destroy(virtio_dmat(sc->sc_virtio), *map);
		*map = NULL;
	}
}

static int
vioif_dmamap_create_load(struct vioif_softc *sc, bus_dmamap_t *map,
    void *buf, bus_size_t size, int nsegs, int rw, const char *usage)
{
	int r;

	r = vioif_dmamap_create(sc, map, size, nsegs, usage);
	if (r != 0)
		return 1;

	r = bus_dmamap_load(virtio_dmat(sc->sc_virtio), *map, buf,
	    size, NULL, rw | BUS_DMA_NOWAIT);
	if (r != 0) {
		vioif_dmamap_destroy(sc, map);
		aprint_error_dev(sc->sc_dev, "%s dmamap load failed. "
		    "error code %d\n", usage, r);
	}

	return r;
}

static void *
vioif_assign_mem(intptr_t *p, size_t size)
{
	intptr_t rv;

	rv = *p;
	*p += size;

	return (void *)rv;
}

/*
 * dma memory is used for:
 *   netq_maps_kva:	 metadata array for received frames (READ) and
 *			 sent frames (WRITE)
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
static int
vioif_alloc_mems(struct vioif_softc *sc)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	struct vioif_netqueue *netq;
	struct vioif_ctrlqueue *ctrlq = &sc->sc_ctrlq;
	struct vioif_net_map *maps;
	unsigned int vq_num;
	int r, rsegs;
	bus_size_t dmamemsize;
	size_t qid, i, netq_num, kmemsize;
	void *vaddr;
	intptr_t p;

	netq_num = sc->sc_max_nvq_pairs * 2;

	/* allocate DMA memory */
	dmamemsize = 0;

	for (qid = 0; qid < netq_num; qid++) {
		maps = sc->sc_netqs[qid].netq_maps;
		vq_num = sc->sc_netqs[qid].netq_vq->vq_num;
		dmamemsize += sizeof(*maps[0].vnm_hdr) * vq_num;
	}

	if (sc->sc_has_ctrl) {
		dmamemsize += sizeof(struct virtio_net_ctrl_cmd);
		dmamemsize += sizeof(struct virtio_net_ctrl_status);
		dmamemsize += sizeof(struct virtio_net_ctrl_rx);
		dmamemsize += sizeof(struct virtio_net_ctrl_mac_tbl)
		    + ETHER_ADDR_LEN;
		dmamemsize += sizeof(struct virtio_net_ctrl_mac_tbl)
		    + ETHER_ADDR_LEN * VIRTIO_NET_CTRL_MAC_MAXENTRIES;
		dmamemsize += sizeof(struct virtio_net_ctrl_mac_addr);
		dmamemsize += sizeof(struct virtio_net_ctrl_mq);
	}

	r = bus_dmamem_alloc(virtio_dmat(vsc), dmamemsize, 0, 0,
	    &sc->sc_segs[0], 1, &rsegs, BUS_DMA_NOWAIT);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev,
		    "DMA memory allocation failed, size %" PRIuBUSSIZE ", "
		    "error code %d\n", dmamemsize, r);
		goto err_none;
	}
	r = bus_dmamem_map(virtio_dmat(vsc), &sc->sc_segs[0], 1,
	    dmamemsize, &vaddr, BUS_DMA_NOWAIT);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev,
		    "DMA memory map failed, error code %d\n", r);
		goto err_dmamem_alloc;
	}

	/* assign DMA memory */
	memset(vaddr, 0, dmamemsize);
	sc->sc_dmamem = vaddr;
	p = (intptr_t) vaddr;

	for (qid = 0; qid < netq_num; qid++) {
		netq = &sc->sc_netqs[qid];
		maps = netq->netq_maps;
		vq_num = netq->netq_vq->vq_num;

		netq->netq_maps_kva = vioif_assign_mem(&p,
		    sizeof(*maps[0].vnm_hdr) * vq_num);
	}

	if (sc->sc_has_ctrl) {
		ctrlq->ctrlq_cmd = vioif_assign_mem(&p,
		    sizeof(*ctrlq->ctrlq_cmd));
		ctrlq->ctrlq_status = vioif_assign_mem(&p,
		    sizeof(*ctrlq->ctrlq_status));
		ctrlq->ctrlq_rx = vioif_assign_mem(&p,
		    sizeof(*ctrlq->ctrlq_rx));
		ctrlq->ctrlq_mac_tbl_uc = vioif_assign_mem(&p,
		    sizeof(*ctrlq->ctrlq_mac_tbl_uc)
		    + ETHER_ADDR_LEN);
		ctrlq->ctrlq_mac_tbl_mc = vioif_assign_mem(&p,
		    sizeof(*ctrlq->ctrlq_mac_tbl_mc)
		    + ETHER_ADDR_LEN * VIRTIO_NET_CTRL_MAC_MAXENTRIES);
		ctrlq->ctrlq_mac_addr = vioif_assign_mem(&p,
		    sizeof(*ctrlq->ctrlq_mac_addr));
		ctrlq->ctrlq_mq = vioif_assign_mem(&p, sizeof(*ctrlq->ctrlq_mq));
	}

	/* allocate kmem */
	kmemsize = 0;

	for (qid = 0; qid < netq_num; qid++) {
		netq = &sc->sc_netqs[qid];
		vq_num = netq->netq_vq->vq_num;

		kmemsize += sizeof(netq->netq_maps[0]) * vq_num;
	}

	vaddr = kmem_zalloc(kmemsize, KM_SLEEP);
	sc->sc_kmem = vaddr;

	/* assign allocated kmem */
	p = (intptr_t) vaddr;

	for (qid = 0; qid < netq_num; qid++) {
		netq = &sc->sc_netqs[qid];
		vq_num = netq->netq_vq->vq_num;

		netq->netq_maps = vioif_assign_mem(&p,
		    sizeof(netq->netq_maps[0]) * vq_num);
	}

	/* prepare dmamaps */
	for (qid = 0; qid < netq_num; qid++) {
		static const struct {
			const char	*msg_hdr;
			const char	*msg_payload;
			int		 dma_flag;
			bus_size_t	 dma_size;
			int		 dma_nsegs;
		} dmaparams[VIOIF_NETQ_IDX] = {
			[VIOIF_NETQ_RX] = {
				.msg_hdr	= "rx header",
				.msg_payload	= "rx payload",
				.dma_flag	= BUS_DMA_READ,
				.dma_size	= MCLBYTES - ETHER_ALIGN,
				.dma_nsegs	= 1,
			},
			[VIOIF_NETQ_TX] = {
				.msg_hdr	= "tx header",
				.msg_payload	= "tx payload",
				.dma_flag	= BUS_DMA_WRITE,
				.dma_size	= ETHER_MAX_LEN,
				.dma_nsegs	= VIRTIO_NET_TX_MAXNSEGS,
			}
		};

		struct virtio_net_hdr *hdrs;
		int dir;

		dir = VIOIF_NETQ_DIR(qid);
		netq = &sc->sc_netqs[qid];
		vq_num = netq->netq_vq->vq_num;
		maps = netq->netq_maps;
		hdrs = netq->netq_maps_kva;

		for (i = 0; i < vq_num; i++) {
			maps[i].vnm_hdr = &hdrs[i];
	
			r = vioif_dmamap_create_load(sc, &maps[i].vnm_hdr_map,
			    maps[i].vnm_hdr, sc->sc_hdr_size, 1,
			    dmaparams[dir].dma_flag, dmaparams[dir].msg_hdr);
			if (r != 0)
				goto err_reqs;

			r = vioif_dmamap_create(sc, &maps[i].vnm_mbuf_map,
			    dmaparams[dir].dma_size, dmaparams[dir].dma_nsegs,
			    dmaparams[dir].msg_payload);
			if (r != 0)
				goto err_reqs;
		}
	}

	if (sc->sc_has_ctrl) {
		/* control vq class & command */
		r = vioif_dmamap_create_load(sc, &ctrlq->ctrlq_cmd_dmamap,
		    ctrlq->ctrlq_cmd, sizeof(*ctrlq->ctrlq_cmd), 1,
		    BUS_DMA_WRITE, "control command");
		if (r != 0)
			goto err_reqs;

		r = vioif_dmamap_create_load(sc, &ctrlq->ctrlq_status_dmamap,
		    ctrlq->ctrlq_status, sizeof(*ctrlq->ctrlq_status), 1,
		    BUS_DMA_READ, "control status");
		if (r != 0)
			goto err_reqs;

		/* control vq rx mode command parameter */
		r = vioif_dmamap_create_load(sc, &ctrlq->ctrlq_rx_dmamap,
		    ctrlq->ctrlq_rx, sizeof(*ctrlq->ctrlq_rx), 1,
		    BUS_DMA_WRITE, "rx mode control command");
		if (r != 0)
			goto err_reqs;

		/* multiqueue set command */
		r = vioif_dmamap_create_load(sc, &ctrlq->ctrlq_mq_dmamap,
		    ctrlq->ctrlq_mq, sizeof(*ctrlq->ctrlq_mq), 1,
		    BUS_DMA_WRITE, "multiqueue set command");
		if (r != 0)
			goto err_reqs;

		/* control vq MAC filter table for unicast */
		/* do not load now since its length is variable */
		r = vioif_dmamap_create(sc, &ctrlq->ctrlq_tbl_uc_dmamap,
		    sizeof(*ctrlq->ctrlq_mac_tbl_uc)
		    + ETHER_ADDR_LEN, 1,
		    "unicast MAC address filter command");
		if (r != 0)
			goto err_reqs;

		/* control vq MAC filter table for multicast */
		r = vioif_dmamap_create(sc, &ctrlq->ctrlq_tbl_mc_dmamap,
		    sizeof(*ctrlq->ctrlq_mac_tbl_mc)
		    + ETHER_ADDR_LEN * VIRTIO_NET_CTRL_MAC_MAXENTRIES, 1,
		    "multicast MAC address filter command");
		if (r != 0)
			goto err_reqs;

		/* control vq MAC address set command */
		r = vioif_dmamap_create_load(sc,
		    &ctrlq->ctrlq_mac_addr_dmamap,
		    ctrlq->ctrlq_mac_addr,
		    sizeof(*ctrlq->ctrlq_mac_addr), 1,
		    BUS_DMA_WRITE, "mac addr set command");
		if (r != 0)
			goto err_reqs;
	}

	return 0;

err_reqs:
	vioif_dmamap_destroy(sc, &ctrlq->ctrlq_tbl_mc_dmamap);
	vioif_dmamap_destroy(sc, &ctrlq->ctrlq_tbl_uc_dmamap);
	vioif_dmamap_destroy(sc, &ctrlq->ctrlq_rx_dmamap);
	vioif_dmamap_destroy(sc, &ctrlq->ctrlq_status_dmamap);
	vioif_dmamap_destroy(sc, &ctrlq->ctrlq_cmd_dmamap);
	vioif_dmamap_destroy(sc, &ctrlq->ctrlq_mac_addr_dmamap);
	for (qid = 0; qid < netq_num; qid++) {
		vq_num = sc->sc_netqs[qid].netq_vq->vq_num;
		maps = sc->sc_netqs[qid].netq_maps;

		for (i = 0; i < vq_num; i++) {
			vioif_dmamap_destroy(sc, &maps[i].vnm_mbuf_map);
			vioif_dmamap_destroy(sc, &maps[i].vnm_hdr_map);
		}
	}
	if (sc->sc_kmem) {
		kmem_free(sc->sc_kmem, kmemsize);
		sc->sc_kmem = NULL;
	}
	bus_dmamem_unmap(virtio_dmat(vsc), sc->sc_dmamem, dmamemsize);
err_dmamem_alloc:
	bus_dmamem_free(virtio_dmat(vsc), &sc->sc_segs[0], 1);
err_none:
	return -1;
}

static void
vioif_alloc_queues(struct vioif_softc *sc)
{
	int nvq_pairs = sc->sc_max_nvq_pairs;
	size_t nvqs, netq_num;

	KASSERT(nvq_pairs <= VIRTIO_NET_CTRL_MQ_VQ_PAIRS_MAX);

	nvqs = netq_num = sc->sc_max_nvq_pairs * 2;
	if (sc->sc_has_ctrl)
		nvqs++;

	sc->sc_vqs = kmem_zalloc(sizeof(sc->sc_vqs[0]) * nvqs, KM_SLEEP);
	sc->sc_netqs = kmem_zalloc(sizeof(sc->sc_netqs[0]) * netq_num,
	    KM_SLEEP);
}

static void
vioif_free_queues(struct vioif_softc *sc)
{
	size_t nvqs, netq_num;

	nvqs = netq_num = sc->sc_max_nvq_pairs * 2;
	if (sc->sc_ctrlq.ctrlq_vq)
		nvqs++;

	kmem_free(sc->sc_netqs, sizeof(sc->sc_netqs[0]) * netq_num);
	kmem_free(sc->sc_vqs, sizeof(sc->sc_vqs[0]) * nvqs);
	sc->sc_netqs = NULL;
	sc->sc_vqs = NULL;
}

/*
 * Network queues
 */
static int
vioif_netqueue_init(struct vioif_softc *sc, struct virtio_softc *vsc,
    size_t qid, u_int softint_flags)
{
	static const struct {
		const char	*dirname;
		int		 segsize;
		int		 nsegs;
		int 		(*intrhand)(void *);
		void		(*sihand)(void *);
	} params[VIOIF_NETQ_IDX] = {
		[VIOIF_NETQ_RX] = {
			.dirname	= "rx",
			.segsize	= MCLBYTES,
			.nsegs		= 2,
			.intrhand	= vioif_rx_intr,
			.sihand		= vioif_rx_handle,
		},
		[VIOIF_NETQ_TX] = {
			.dirname	= "tx",
			.segsize	= ETHER_MAX_LEN - ETHER_HDR_LEN,
			.nsegs		= 2,
			.intrhand	= vioif_tx_intr,
			.sihand		= vioif_tx_handle,
		}
	};

	struct virtqueue *vq;
	struct vioif_netqueue *netq;
	struct vioif_tx_context *txc;
	struct vioif_rx_context *rxc;
	char qname[32];
	int r, dir;

	txc = NULL;
	rxc = NULL;
	netq = &sc->sc_netqs[qid];
	vq = &sc->sc_vqs[qid];
	dir = VIOIF_NETQ_DIR(qid);

	netq->netq_vq = &sc->sc_vqs[qid];
	netq->netq_stopping = false;
	netq->netq_running_handle = false;

	snprintf(qname, sizeof(qname), "%s%zu",
	    params[dir].dirname, VIOIF_NETQ_PAIRIDX(qid));
	snprintf(netq->netq_evgroup, sizeof(netq->netq_evgroup),
	    "%s-%s", device_xname(sc->sc_dev), qname);

	mutex_init(&netq->netq_lock, MUTEX_DEFAULT, IPL_NET);
	virtio_init_vq(vsc, vq, qid, params[dir].intrhand, netq);

	r = virtio_alloc_vq(vsc, vq,
	    params[dir].segsize + sc->sc_hdr_size,
	    params[dir].nsegs, qname);
	if (r != 0)
		goto err;
	netq->netq_vq = vq;

	netq->netq_softint = softint_establish(softint_flags,
	    params[dir].sihand, netq);
	if (netq->netq_softint == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish %s softint\n",
		    params[dir].dirname);
		goto err;
	}
	vioif_work_set(&netq->netq_work, params[dir].sihand, netq);

	switch (dir) {
	case VIOIF_NETQ_RX:
		rxc = kmem_zalloc(sizeof(*rxc), KM_SLEEP);
		netq->netq_ctx = rxc;
		/* nothing to do */
		break;
	case VIOIF_NETQ_TX:
		txc = kmem_zalloc(sizeof(*txc), KM_SLEEP);
		netq->netq_ctx = (void *)txc;
		txc->txc_deferred_transmit = softint_establish(softint_flags,
		    vioif_deferred_transmit, netq);
		if (txc->txc_deferred_transmit == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "couldn't establish softint for "
			    "tx deferred transmit\n");
			goto err;
		}
		txc->txc_link_active = VIOIF_IS_LINK_ACTIVE(sc);
		txc->txc_no_free_slots = false;
		txc->txc_intrq = pcq_create(vq->vq_num, KM_SLEEP);
		break;
	}

	return 0;

err:
	netq->netq_ctx = NULL;

	if (rxc != NULL) {
		kmem_free(rxc, sizeof(*rxc));
	}

	if (txc != NULL) {
		if (txc->txc_deferred_transmit != NULL)
			softint_disestablish(txc->txc_deferred_transmit);
		if (txc->txc_intrq != NULL)
			pcq_destroy(txc->txc_intrq);
		kmem_free(txc, sizeof(txc));
	}

	vioif_work_set(&netq->netq_work, NULL, NULL);
	if (netq->netq_softint != NULL) {
		softint_disestablish(netq->netq_softint);
		netq->netq_softint = NULL;
	}

	virtio_free_vq(vsc, vq);
	mutex_destroy(&netq->netq_lock);
	netq->netq_vq = NULL;

	return -1;
}

static void
vioif_netqueue_teardown(struct vioif_softc *sc, struct virtio_softc *vsc,
    size_t qid)
{
	struct vioif_netqueue *netq;
	struct vioif_rx_context *rxc;
	struct vioif_tx_context *txc;
	int dir;

	netq = &sc->sc_netqs[qid];

	if (netq->netq_vq == NULL)
		return;

	netq = &sc->sc_netqs[qid];
	dir = VIOIF_NETQ_DIR(qid);
	switch (dir) {
	case VIOIF_NETQ_RX:
		rxc = netq->netq_ctx;
		netq->netq_ctx = NULL;
		kmem_free(rxc, sizeof(*rxc));
		break;
	case VIOIF_NETQ_TX:
		txc = netq->netq_ctx;
		netq->netq_ctx = NULL;
		softint_disestablish(txc->txc_deferred_transmit);
		pcq_destroy(txc->txc_intrq);
		kmem_free(txc, sizeof(*txc));
		break;
	}

	softint_disestablish(netq->netq_softint);
	virtio_free_vq(vsc, netq->netq_vq);
	mutex_destroy(&netq->netq_lock);
	netq->netq_vq = NULL;
}

static void
vioif_net_sched_handle(struct vioif_softc *sc, struct vioif_netqueue *netq)
{

	KASSERT(mutex_owned(&netq->netq_lock));
	KASSERT(!netq->netq_stopping);

	if (netq->netq_workqueue) {
		vioif_work_add(sc->sc_txrx_workqueue, &netq->netq_work);
	} else {
		softint_schedule(netq->netq_softint);
	}
}

static int
vioif_net_load_mbuf(struct virtio_softc *vsc, struct vioif_net_map *map,
   struct mbuf *m, int dma_flags)
{
	int r;

	KASSERT(map->vnm_mbuf == NULL);

	r = bus_dmamap_load_mbuf(virtio_dmat(vsc),
	    map->vnm_mbuf_map, m, dma_flags | BUS_DMA_NOWAIT);
	if (r == 0) {
		map->vnm_mbuf = m;
	}

	return r;
}

static void
vioif_net_unload_mbuf(struct virtio_softc *vsc, struct vioif_net_map *map)
{

	KASSERT(map->vnm_mbuf != NULL);
	bus_dmamap_unload(virtio_dmat(vsc), map->vnm_mbuf_map);
	map->vnm_mbuf = NULL;
}

static int
vioif_net_enqueue(struct virtio_softc *vsc, struct virtqueue *vq,
    int slot, struct vioif_net_map *map, int dma_ops, bool is_write)
{
	int r;

	KASSERT(map->vnm_mbuf != NULL);

	/* This should actually never fail */
	r = virtio_enqueue_reserve(vsc, vq, slot,
	    map->vnm_mbuf_map->dm_nsegs + 1);
	if (r != 0) {
		/* slot already freed by virtio_enqueue_reserve */
		return r;
	}

	bus_dmamap_sync(virtio_dmat(vsc), map->vnm_mbuf_map,
	    0, map->vnm_mbuf_map->dm_mapsize, dma_ops);
	bus_dmamap_sync(virtio_dmat(vsc), map->vnm_hdr_map,
	    0, map->vnm_hdr_map->dm_mapsize, dma_ops);

	virtio_enqueue(vsc, vq, slot, map->vnm_hdr_map, is_write);
	virtio_enqueue(vsc, vq, slot, map->vnm_mbuf_map, is_write);
	virtio_enqueue_commit(vsc, vq, slot, false);

	return 0;
}

static int
vioif_net_enqueue_tx(struct virtio_softc *vsc, struct virtqueue *vq,
    int slot, struct vioif_net_map *map)
{

	return vioif_net_enqueue(vsc, vq, slot, map,
	    BUS_DMASYNC_PREWRITE, true);
}

static int
vioif_net_enqueue_rx(struct virtio_softc *vsc, struct virtqueue *vq,
    int slot, struct vioif_net_map *map)
{

	return vioif_net_enqueue(vsc, vq, slot, map,
	    BUS_DMASYNC_PREREAD, false);
}

static struct mbuf *
vioif_net_dequeue_commit(struct virtio_softc *vsc, struct virtqueue *vq,
   int slot, struct vioif_net_map *map, int dma_flags)
{
	struct mbuf *m;

	m = map->vnm_mbuf;
	KASSERT(m != NULL);
	map->vnm_mbuf = NULL;

	bus_dmamap_sync(virtio_dmat(vsc), map->vnm_hdr_map,
	    0, map->vnm_hdr_map->dm_mapsize, dma_flags);
	bus_dmamap_sync(virtio_dmat(vsc), map->vnm_mbuf_map,
	    0, map->vnm_mbuf_map->dm_mapsize, dma_flags);

	bus_dmamap_unload(virtio_dmat(vsc), map->vnm_mbuf_map);
	virtio_dequeue_commit(vsc, vq, slot);

	return m;
}

static void
vioif_net_intr_enable(struct vioif_softc *sc, struct virtio_softc *vsc)
{
	struct vioif_netqueue *netq;
	size_t i, act_qnum;
	int enqueued;

	act_qnum = sc->sc_act_nvq_pairs * 2;
	for (i = 0; i < act_qnum; i++) {
		netq = &sc->sc_netqs[i];

		KASSERT(!netq->netq_stopping);
		KASSERT(!netq->netq_running_handle);

		enqueued = virtio_start_vq_intr(vsc, netq->netq_vq);
		if (enqueued != 0) {
			virtio_stop_vq_intr(vsc, netq->netq_vq);

			mutex_enter(&netq->netq_lock);
			netq->netq_running_handle = true;
			vioif_net_sched_handle(sc, netq);
			mutex_exit(&netq->netq_lock);
		}
	}
}

static void
vioif_net_intr_disable(struct vioif_softc *sc, struct virtio_softc *vsc)
{
	struct vioif_netqueue *netq;
	size_t i, act_qnum;

	act_qnum = sc->sc_act_nvq_pairs * 2;
	for (i = 0; i < act_qnum; i++) {
		netq = &sc->sc_netqs[i];

		virtio_stop_vq_intr(vsc, netq->netq_vq);
	}
}

/*
 * Receive implementation
 */
/* enqueue mbufs to receive slots */
static void
vioif_populate_rx_mbufs_locked(struct vioif_softc *sc, struct vioif_netqueue *netq)
{
	struct virtqueue *vq = netq->netq_vq;
	struct virtio_softc *vsc = vq->vq_owner;
	struct vioif_rx_context *rxc;
	struct vioif_net_map *map;
	struct mbuf *m;
	int i, r, ndone = 0;

	KASSERT(mutex_owned(&netq->netq_lock));

	rxc = netq->netq_ctx;

	for (i = 0; i < vq->vq_num; i++) {
		int slot;
		r = virtio_enqueue_prep(vsc, vq, &slot);
		if (r == EAGAIN)
			break;
		if (__predict_false(r != 0))
			panic("enqueue_prep for rx buffers");

		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == NULL) {
			virtio_enqueue_abort(vsc, vq, slot);
			rxc->rxc_mbuf_enobufs.ev_count++;
			break;
		}
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			virtio_enqueue_abort(vsc, vq, slot);
			m_freem(m);
			rxc->rxc_mbuf_enobufs.ev_count++;
			break;
		}

		m->m_len = m->m_pkthdr.len = MCLBYTES;
		m_adj(m, ETHER_ALIGN);

		map = &netq->netq_maps[slot];
		r = vioif_net_load_mbuf(vsc, map, m, BUS_DMA_READ);
		if (r != 0) {
			virtio_enqueue_abort(vsc, vq, slot);
			m_freem(m);
			netq->netq_mbuf_load_failed.ev_count++;
			break;
		}

		r = vioif_net_enqueue_rx(vsc, vq, slot, map);
		if (r != 0) {
			vioif_net_unload_mbuf(vsc, map);
			netq->netq_enqueue_failed.ev_count++;
			m_freem(m);
			/* slot already freed by vioif_net_enqueue_rx */
			break;
		}

		ndone++;
	}

	if (ndone > 0)
		vioif_notify(vsc, vq);
}

/* dequeue received packets */
static bool
vioif_rx_deq_locked(struct vioif_softc *sc, struct virtio_softc *vsc,
    struct vioif_netqueue *netq, u_int limit, size_t *ndeqp)
{
	struct virtqueue *vq = netq->netq_vq;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct vioif_net_map *map;
	struct mbuf *m;
	int slot, len;
	bool more;
	size_t ndeq;

	KASSERT(mutex_owned(&netq->netq_lock));

	more = false;
	ndeq = 0;

	if (virtio_vq_is_enqueued(vsc, vq) == false)
		goto done;

	for (;;ndeq++) {
		if (ndeq >= limit) {
			more = true;
			break;
		}

		if (virtio_dequeue(vsc, vq, &slot, &len) != 0)
			break;

		map = &netq->netq_maps[slot];
		KASSERT(map->vnm_mbuf != NULL);
		m = vioif_net_dequeue_commit(vsc, vq, slot,
		    map, BUS_DMASYNC_POSTREAD);
		KASSERT(m != NULL);

		m->m_len = m->m_pkthdr.len = len - sc->sc_hdr_size;
		m_set_rcvif(m, ifp);
		if_percpuq_enqueue(ifp->if_percpuq, m);
	}

done:
	if (ndeqp != NULL)
		*ndeqp = ndeq;

	return more;
}

static void
vioif_rx_queue_clear(struct vioif_softc *sc, struct virtio_softc *vsc,
    struct vioif_netqueue *netq)
{
	struct vioif_net_map *map;
	struct mbuf *m;
	unsigned int i, vq_num;
	bool more;

	mutex_enter(&netq->netq_lock);

	vq_num = netq->netq_vq->vq_num;
	for (;;) {
		more = vioif_rx_deq_locked(sc, vsc, netq, vq_num, NULL);
		if (more == false)
			break;
	}

	for (i = 0; i < vq_num; i++) {
		map = &netq->netq_maps[i];

		m = map->vnm_mbuf;
		if (m == NULL)
			continue;

		vioif_net_unload_mbuf(vsc, map);
		m_freem(m);
	}
	mutex_exit(&netq->netq_lock);
}

static void
vioif_rx_handle_locked(void *xnetq, u_int limit)
{
	struct vioif_netqueue *netq = xnetq;
	struct virtqueue *vq = netq->netq_vq;
	struct virtio_softc *vsc = vq->vq_owner;
	struct vioif_softc *sc = device_private(virtio_child(vsc));
	bool more;
	int enqueued;
	size_t ndeq;

	KASSERT(mutex_owned(&netq->netq_lock));
	KASSERT(!netq->netq_stopping);

	more = vioif_rx_deq_locked(sc, vsc, netq, limit, &ndeq);
	if (ndeq > 0)
		vioif_populate_rx_mbufs_locked(sc, netq);

	if (more) {
		vioif_net_sched_handle(sc, netq);
		return;
	}

	enqueued = virtio_start_vq_intr(vsc, netq->netq_vq);
	if (enqueued != 0) {
		virtio_stop_vq_intr(vsc, netq->netq_vq);
		vioif_net_sched_handle(sc, netq);
		return;
	}

	netq->netq_running_handle = false;
}

static int
vioif_rx_intr(void *arg)
{
	struct vioif_netqueue *netq = arg;
	struct virtqueue *vq = netq->netq_vq;
	struct virtio_softc *vsc = vq->vq_owner;
	struct vioif_softc *sc = device_private(virtio_child(vsc));
	u_int limit;

	mutex_enter(&netq->netq_lock);

	/* handler is already running in softint/workqueue */
	if (netq->netq_running_handle)
		goto done;

	if (netq->netq_stopping)
		goto done;

	netq->netq_running_handle = true;

	limit = sc->sc_rx_intr_process_limit;
	virtio_stop_vq_intr(vsc, vq);
	vioif_rx_handle_locked(netq, limit);

done:
	mutex_exit(&netq->netq_lock);
	return 1;
}

static void
vioif_rx_handle(void *xnetq)
{
	struct vioif_netqueue *netq = xnetq;
	struct virtqueue *vq = netq->netq_vq;
	struct virtio_softc *vsc = vq->vq_owner;
	struct vioif_softc *sc = device_private(virtio_child(vsc));
	u_int limit;

	mutex_enter(&netq->netq_lock);

	KASSERT(netq->netq_running_handle);

	if (netq->netq_stopping) {
		netq->netq_running_handle = false;
		goto done;
	}

	limit = sc->sc_rx_process_limit;
	vioif_rx_handle_locked(netq, limit);

done:
	mutex_exit(&netq->netq_lock);
}

/*
 * Transmition implementation
 */
/* enqueue mbufs to send */
static void
vioif_send_common_locked(struct ifnet *ifp, struct vioif_netqueue *netq,
    bool is_transmit)
{
	struct vioif_softc *sc = ifp->if_softc;
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtqueue *vq = netq->netq_vq;
	struct vioif_tx_context *txc;
	struct vioif_net_map *map;
	struct mbuf *m;
	int queued = 0;

	KASSERT(mutex_owned(&netq->netq_lock));

	if (netq->netq_stopping ||
	    !ISSET(ifp->if_flags, IFF_RUNNING))
		return;

	txc = netq->netq_ctx;

	if (!txc->txc_link_active ||
	    txc->txc_no_free_slots)
		return;

	for (;;) {
		int slot, r;
		r = virtio_enqueue_prep(vsc, vq, &slot);
		if (r == EAGAIN) {
			txc->txc_no_free_slots = true;
			break;
		}
		if (__predict_false(r != 0))
			panic("enqueue_prep for tx buffers");

		if (is_transmit)
			m = pcq_get(txc->txc_intrq);
		else
			IFQ_DEQUEUE(&ifp->if_snd, m);

		if (m == NULL) {
			virtio_enqueue_abort(vsc, vq, slot);
			break;
		}

		map = &netq->netq_maps[slot];
		KASSERT(map->vnm_mbuf == NULL);

		r = vioif_net_load_mbuf(vsc, map, m, BUS_DMA_WRITE);
		if (r != 0) {
			/* maybe just too fragmented */
			struct mbuf *newm;

			newm = m_defrag(m, M_NOWAIT);
			if (newm != NULL) {
				m = newm;
				r = vioif_net_load_mbuf(vsc, map, m,
				    BUS_DMA_WRITE);
			} else {
				txc->txc_defrag_failed.ev_count++;
				r = -1;
			}

			if (r != 0) {
				netq->netq_mbuf_load_failed.ev_count++;
				m_freem(m);
				if_statinc(ifp, if_oerrors);
				virtio_enqueue_abort(vsc, vq, slot);
				continue;
			}
		}

		memset(map->vnm_hdr, 0, sc->sc_hdr_size);

		r = vioif_net_enqueue_tx(vsc, vq, slot, map);
		if (r != 0) {
			netq->netq_enqueue_failed.ev_count++;
			vioif_net_unload_mbuf(vsc, map);
			m_freem(m);
			/* slot already freed by vioif_net_enqueue_tx */

			if_statinc(ifp, if_oerrors);
			continue;
		}

		queued++;
		bpf_mtap(ifp, m, BPF_D_OUT);
	}

	if (queued > 0) {
		vioif_notify(vsc, vq);
		ifp->if_timer = 5;
	}
}

/* dequeue sent mbufs */
static bool
vioif_tx_deq_locked(struct vioif_softc *sc, struct virtio_softc *vsc,
    struct vioif_netqueue *netq, u_int limit, size_t *ndeqp)
{
	struct virtqueue *vq = netq->netq_vq;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct vioif_net_map *map;
	struct mbuf *m;
	int slot, len;
	bool more;
	size_t ndeq;

	KASSERT(mutex_owned(&netq->netq_lock));

	more = false;
	ndeq = 0;

	if (virtio_vq_is_enqueued(vsc, vq) == false)
		goto done;

	for (;;ndeq++) {
		if (limit-- == 0) {
			more = true;
			break;
		}

		if (virtio_dequeue(vsc, vq, &slot, &len) != 0)
			break;

		map = &netq->netq_maps[slot];
		KASSERT(map->vnm_mbuf != NULL);
		m = vioif_net_dequeue_commit(vsc, vq, slot,
		    map, BUS_DMASYNC_POSTWRITE);
		KASSERT(m != NULL);

		if_statinc(ifp, if_opackets);
		m_freem(m);
	}

done:
	if (ndeqp != NULL)
		*ndeqp = ndeq;
	return more;
}

static void
vioif_tx_queue_clear(struct vioif_softc *sc, struct virtio_softc *vsc,
    struct vioif_netqueue *netq)
{
	struct vioif_tx_context *txc;
	struct vioif_net_map *map;
	struct mbuf *m;
	unsigned int i, vq_num;
	bool more;

	mutex_enter(&netq->netq_lock);

	txc = netq->netq_ctx;
	vq_num = netq->netq_vq->vq_num;

	for (;;) {
		more = vioif_tx_deq_locked(sc, vsc, netq, vq_num, NULL);
		if (more == false)
			break;
	}

	for (i = 0; i < vq_num; i++) {
		map = &netq->netq_maps[i];

		m = map->vnm_mbuf;
		if (m == NULL)
			continue;

		vioif_net_unload_mbuf(vsc, map);
		m_freem(m);
	}

	txc->txc_no_free_slots = false;

	mutex_exit(&netq->netq_lock);
}

static void
vioif_start_locked(struct ifnet *ifp, struct vioif_netqueue *netq)
{

	/*
	 * ifp->if_obytes and ifp->if_omcasts are added in if_transmit()@if.c.
	 */
	vioif_send_common_locked(ifp, netq, false);

}

static void
vioif_transmit_locked(struct ifnet *ifp, struct vioif_netqueue *netq)
{

	vioif_send_common_locked(ifp, netq, true);
}

static void
vioif_deferred_transmit(void *arg)
{
	struct vioif_netqueue *netq = arg;
	struct virtio_softc *vsc = netq->netq_vq->vq_owner;
	struct vioif_softc *sc = device_private(virtio_child(vsc));
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	mutex_enter(&netq->netq_lock);
	vioif_send_common_locked(ifp, netq, true);
	mutex_exit(&netq->netq_lock);
}

static void
vioif_tx_handle_locked(struct vioif_netqueue *netq, u_int limit)
{
	struct virtqueue *vq = netq->netq_vq;
	struct vioif_tx_context *txc = netq->netq_ctx;
	struct virtio_softc *vsc = vq->vq_owner;
	struct vioif_softc *sc = device_private(virtio_child(vsc));
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	bool more;
	int enqueued;
	size_t ndeq;

	KASSERT(mutex_owned(&netq->netq_lock));
	KASSERT(!netq->netq_stopping);

	more = vioif_tx_deq_locked(sc, vsc, netq, limit, &ndeq);
	if (txc->txc_no_free_slots && ndeq > 0) {
		txc->txc_no_free_slots = false;
		softint_schedule(txc->txc_deferred_transmit);
	}

	if (more) {
		vioif_net_sched_handle(sc, netq);
		return;
	}

	enqueued = (virtio_features(vsc) & VIRTIO_F_RING_EVENT_IDX) ?
	    virtio_postpone_intr_smart(vsc, vq):
	    virtio_start_vq_intr(vsc, vq);
	if (enqueued != 0) {
		virtio_stop_vq_intr(vsc, vq);
		vioif_net_sched_handle(sc, netq);
		return;
	}

	netq->netq_running_handle = false;

	/* for ALTQ */
	if (netq == &sc->sc_netqs[VIOIF_NETQ_TXQID(0)])
		if_schedule_deferred_start(ifp);

	softint_schedule(txc->txc_deferred_transmit);
}

static int
vioif_tx_intr(void *arg)
{
	struct vioif_netqueue *netq = arg;
	struct virtqueue *vq = netq->netq_vq;
	struct virtio_softc *vsc = vq->vq_owner;
	struct vioif_softc *sc = device_private(virtio_child(vsc));
	u_int limit;

	mutex_enter(&netq->netq_lock);

	/* tx handler is already running in softint/workqueue */
	if (netq->netq_running_handle)
		goto done;

	if (netq->netq_stopping)
		goto done;

	netq->netq_running_handle = true;

	virtio_stop_vq_intr(vsc, vq);
	netq->netq_workqueue = sc->sc_txrx_workqueue_sysctl;
	limit = sc->sc_tx_intr_process_limit;
	vioif_tx_handle_locked(netq, limit);

done:
	mutex_exit(&netq->netq_lock);
	return 1;
}

static void
vioif_tx_handle(void *xnetq)
{
	struct vioif_netqueue *netq = xnetq;
	struct virtqueue *vq = netq->netq_vq;
	struct virtio_softc *vsc = vq->vq_owner;
	struct vioif_softc *sc = device_private(virtio_child(vsc));
	u_int limit;

	mutex_enter(&netq->netq_lock);

	KASSERT(netq->netq_running_handle);

	if (netq->netq_stopping) {
		netq->netq_running_handle = false;
		goto done;
	}

	limit = sc->sc_tx_process_limit;
	vioif_tx_handle_locked(netq, limit);

done:
	mutex_exit(&netq->netq_lock);
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
			sc->sc_ctrlq.ctrlq_cmd_load_failed.ev_count++;
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

	/* we need to explicitly (re)start vq intr when using RING EVENT IDX */
	if (virtio_features(vsc) & VIRTIO_F_RING_EVENT_IDX)
		virtio_start_vq_intr(vsc, ctrlq->ctrlq_vq);

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
	/* already dequeued */

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
		device_printf(sc->sc_dev, "failed setting rx mode\n");
		sc->sc_ctrlq.ctrlq_cmd_failed.ev_count++;
		r = EIO;
	}

	return r;
}

/* ctrl vq interrupt; wake up the command issuer */
static int
vioif_ctrl_intr(void *arg)
{
	struct vioif_ctrlqueue *ctrlq = arg;
	struct virtqueue *vq = ctrlq->ctrlq_vq;
	struct virtio_softc *vsc = vq->vq_owner;
	int r, slot;

	if (virtio_vq_is_enqueued(vsc, vq) == false)
		return 0;

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

	mq->virtqueue_pairs = virtio_rw16(sc->sc_virtio, nvq_pairs);
	specs[0].dmamap = sc->sc_ctrlq.ctrlq_mq_dmamap;
	specs[0].buf = mq;
	specs[0].bufsize = sizeof(*mq);

	r = vioif_ctrl_send_command(sc,
	    VIRTIO_NET_CTRL_MQ, VIRTIO_NET_CTRL_MQ_VQ_PAIRS_SET,
	    specs, __arraycount(specs));

	vioif_ctrl_release(sc);

	return r;
}

static int
vioif_set_mac_addr(struct vioif_softc *sc)
{
	struct virtio_net_ctrl_mac_addr *ma =
	    sc->sc_ctrlq.ctrlq_mac_addr;
	struct vioif_ctrl_cmdspec specs[1];
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int nspecs = __arraycount(specs);
	uint64_t features;
	int r;
	size_t i;

	if (!sc->sc_has_ctrl)
		return ENOTSUP;

	if (memcmp(CLLADDR(ifp->if_sadl), sc->sc_mac,
	    ETHER_ADDR_LEN) == 0) {
		return 0;
	}

	memcpy(sc->sc_mac, CLLADDR(ifp->if_sadl), ETHER_ADDR_LEN);

	features = virtio_features(sc->sc_virtio);
	if (features & VIRTIO_NET_F_CTRL_MAC_ADDR) {
		vioif_ctrl_acquire(sc);

		memcpy(ma->mac, sc->sc_mac, ETHER_ADDR_LEN);
		specs[0].dmamap = sc->sc_ctrlq.ctrlq_mac_addr_dmamap;
		specs[0].buf = ma;
		specs[0].bufsize = sizeof(*ma);

		r = vioif_ctrl_send_command(sc,
		    VIRTIO_NET_CTRL_MAC, VIRTIO_NET_CTRL_MAC_ADDR_SET,
		    specs, nspecs);

		vioif_ctrl_release(sc);
	} else {
		for (i = 0; i < __arraycount(sc->sc_mac); i++) {
			virtio_write_device_config_1(sc->sc_virtio,
			    VIRTIO_NET_CONFIG_MAC + i, sc->sc_mac[i]);
		}
		r = 0;
	}

	return r;
}

static int
vioif_set_rx_filter(struct vioif_softc *sc)
{
	/* filter already set in ctrlq->ctrlq_mac_tbl */
	struct virtio_softc *vsc = sc->sc_virtio;
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
	    + (ETHER_ADDR_LEN * virtio_rw32(vsc, mac_tbl_uc->nentries));

	specs[1].dmamap = sc->sc_ctrlq.ctrlq_tbl_mc_dmamap;
	specs[1].buf = mac_tbl_mc;
	specs[1].bufsize = sizeof(*mac_tbl_mc)
	    + (ETHER_ADDR_LEN * virtio_rw32(vsc, mac_tbl_mc->nentries));

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

/*
 * If multicast filter small enough (<=MAXENTRIES) set rx filter
 * If large multicast filter exist use ALLMULTI
 * If setting rx filter fails fall back to ALLMULTI
 */
static int
vioif_rx_filter(struct vioif_softc *sc)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	struct ethercom *ec = &sc->sc_ethercom;
	struct ifnet *ifp = &ec->ec_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	struct vioif_ctrlqueue *ctrlq = &sc->sc_ctrlq;
	int nentries;
	bool allmulti = 0;
	int r;

	if (!sc->sc_has_ctrl) {
		goto set_ifflags;
	}

	memcpy(ctrlq->ctrlq_mac_tbl_uc->macs[0],
	    CLLADDR(ifp->if_sadl), ETHER_ADDR_LEN);

	nentries = 0;
	allmulti = false;

	ETHER_LOCK(ec);
	for (ETHER_FIRST_MULTI(step, ec, enm); enm != NULL;
	    ETHER_NEXT_MULTI(step, enm)) {
		if (nentries >= VIRTIO_NET_CTRL_MAC_MAXENTRIES) {
			allmulti = true;
			break;
		}
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			allmulti = true;
			break;
		}

		memcpy(ctrlq->ctrlq_mac_tbl_mc->macs[nentries],
		    enm->enm_addrlo, ETHER_ADDR_LEN);
		nentries++;
	}
	ETHER_UNLOCK(ec);

	r = vioif_set_mac_addr(sc);
	if (r != 0) {
		log(LOG_WARNING, "%s: couldn't set MAC address\n",
		    ifp->if_xname);
	}

	if (!allmulti) {
		ctrlq->ctrlq_mac_tbl_uc->nentries = virtio_rw32(vsc, 1);
		ctrlq->ctrlq_mac_tbl_mc->nentries = virtio_rw32(vsc, nentries);
		r = vioif_set_rx_filter(sc);
		if (r != 0) {
			allmulti = true; /* fallback */
		}
	}

	if (allmulti) {
		ctrlq->ctrlq_mac_tbl_uc->nentries = virtio_rw32(vsc, 0);
		ctrlq->ctrlq_mac_tbl_mc->nentries = virtio_rw32(vsc, 0);
		r = vioif_set_rx_filter(sc);
		if (r != 0) {
			log(LOG_DEBUG, "%s: couldn't clear RX filter\n",
			    ifp->if_xname);
			/* what to do on failure? */
		}

		ifp->if_flags |= IFF_ALLMULTI;
	}

set_ifflags:
	r = vioif_ifflags(sc);

	return r;
}

/*
 * VM configuration changes
 */
static int
vioif_config_change(struct virtio_softc *vsc)
{
	struct vioif_softc *sc = device_private(virtio_child(vsc));

	softint_schedule(sc->sc_cfg_softint);
	return 0;
}

static void
vioif_cfg_softint(void *arg)
{
	struct vioif_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	vioif_update_link_status(sc);
	vioif_start(ifp);
}

static int
vioif_get_link_status(struct vioif_softc *sc)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	uint16_t status;

	if (virtio_features(vsc) & VIRTIO_NET_F_STATUS)
		status = virtio_read_device_config_2(vsc,
		    VIRTIO_NET_CONFIG_STATUS);
	else
		status = VIRTIO_NET_S_LINK_UP;

	if ((status & VIRTIO_NET_S_LINK_UP) != 0)
		return LINK_STATE_UP;

	return LINK_STATE_DOWN;
}

static void
vioif_update_link_status(struct vioif_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct vioif_netqueue *netq;
	struct vioif_tx_context *txc;
	bool active;
	int link, i;

	mutex_enter(&sc->sc_lock);

	link = vioif_get_link_status(sc);

	if (link == sc->sc_link_state)
		goto done;

	sc->sc_link_state = link;

	active = VIOIF_IS_LINK_ACTIVE(sc);
	for (i = 0; i < sc->sc_act_nvq_pairs; i++) {
		netq = &sc->sc_netqs[VIOIF_NETQ_TXQID(i)];

		mutex_enter(&netq->netq_lock);
		txc = netq->netq_ctx;
		txc->txc_link_active = active;
		mutex_exit(&netq->netq_lock);
	}

	if_link_state_change(ifp, sc->sc_link_state);

done:
	mutex_exit(&sc->sc_lock);
}

static void
vioif_workq_work(struct work *wk, void *context)
{
	struct vioif_work *work;

	work = container_of(wk, struct vioif_work, cookie);

	atomic_store_relaxed(&work->added, 0);
	work->func(work->arg);
}

static struct workqueue *
vioif_workq_create(const char *name, pri_t prio, int ipl, int flags)
{
	struct workqueue *wq;
	int error;

	error = workqueue_create(&wq, name, vioif_workq_work, NULL,
	    prio, ipl, flags);

	if (error)
		return NULL;

	return wq;
}

static void
vioif_workq_destroy(struct workqueue *wq)
{

	workqueue_destroy(wq);
}

static void
vioif_work_set(struct vioif_work *work, void (*func)(void *), void *arg)
{

	memset(work, 0, sizeof(*work));
	work->func = func;
	work->arg = arg;
}

static void
vioif_work_add(struct workqueue *wq, struct vioif_work *work)
{

	if (atomic_load_relaxed(&work->added) != 0)
		return;

	atomic_store_relaxed(&work->added, 1);
	kpreempt_disable();
	workqueue_enqueue(wq, &work->cookie, NULL);
	kpreempt_enable();
}

static void
vioif_work_wait(struct workqueue *wq, struct vioif_work *work)
{

	workqueue_wait(wq, &work->cookie);
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

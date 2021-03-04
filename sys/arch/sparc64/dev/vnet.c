/*	$OpenBSD: vnet.c,v 1.62 2020/07/10 13:26:36 patrick Exp $	*/
/*
 * Copyright (c) 2009, 2015 Mark Kettenis
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

#if 0
FIXME openbsd
#include "bpfilter.h"
#endif

#include <sys/kmem.h>
#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/systm.h>
#if 0
FIXME openbsd
#include <sys/timeout.h>
#endif

#include <machine/autoconf.h>
#include <machine/hypervisor.h>
#include <machine/openfirm.h>

#include <net/if.h>
#include <net/if_media.h>

#include <netinet/in.h>
#include <net/if_ether.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <uvm/uvm_extern.h>

#include <sparc64/dev/cbusvar.h>
#include <sparc64/dev/ldcvar.h>
#include <sparc64/dev/viovar.h>

#ifdef VNET_DEBUG
#define DPRINTF(x)	printf x
#else
#define DPRINTF(x)
#endif

#define VNET_TX_ENTRIES		32
#define VNET_RX_ENTRIES		32

struct vnet_attr_info {
	struct vio_msg_tag	tag;
	uint8_t			xfer_mode;
	uint8_t			addr_type;
	uint16_t		ack_freq;
	uint32_t		_reserved1;
	uint64_t		addr;
	uint64_t		mtu;
	uint64_t		_reserved2[3];
};

/* Address types. */
#define VNET_ADDR_ETHERMAC	0x01

/* Sub-Type envelopes. */
#define VNET_MCAST_INFO		0x0101

#define VNET_NUM_MCAST		7

struct vnet_mcast_info {
	struct vio_msg_tag	tag;
	uint8_t			set;
	uint8_t			count;
	uint8_t			mcast_addr[VNET_NUM_MCAST][ETHER_ADDR_LEN];
	uint32_t		_reserved;
};

struct vnet_desc {
	struct vio_dring_hdr	hdr;
	uint32_t		nbytes;
	uint32_t		ncookies;
	struct ldc_cookie	cookie[2];
};

struct vnet_desc_msg {
	struct vio_msg_tag	tag;
	uint64_t		seq_no;
	uint64_t		desc_handle;
	uint32_t		nbytes;
	uint32_t		ncookies;
	struct ldc_cookie	cookie[1];
};

struct vnet_dring {
	bus_dmamap_t		vd_map;
	bus_dma_segment_t	vd_seg;
	struct vnet_desc	*vd_desc;
	int			vd_nentries;
};

struct vnet_dring *vnet_dring_alloc(bus_dma_tag_t, int);
void	vnet_dring_free(bus_dma_tag_t, struct vnet_dring *);

/*
 * For now, we only support vNet 1.0.
 */
#define VNET_MAJOR	1
#define VNET_MINOR	0

/*
 * The vNet protocol wants the IP header to be 64-bit aligned, so
 * define out own variant of ETHER_ALIGN.
 */
#define VNET_ETHER_ALIGN	6

struct vnet_soft_desc {
	int		vsd_map_idx;
#if 0
FIXME openbsd		
	caddr_t		vsd_buf;
#else		
	unsigned char *vsd_buf;
#endif		
};

struct vnet_softc {
	struct device	sc_dv;
	bus_space_tag_t	sc_bustag;
	bus_dma_tag_t	sc_dmatag;

	uint64_t	sc_tx_ino;
	uint64_t	sc_rx_ino;
	void		*sc_tx_ih;
	void		*sc_rx_ih;

	struct ldc_conn	sc_lc;

	uint16_t	sc_vio_state;
#define VIO_SND_VER_INFO	0x0001
#define VIO_ACK_VER_INFO	0x0002
#define VIO_RCV_VER_INFO	0x0004
#define VIO_SND_ATTR_INFO	0x0008
#define VIO_ACK_ATTR_INFO	0x0010
#define VIO_RCV_ATTR_INFO	0x0020
#define VIO_SND_DRING_REG	0x0040
#define VIO_ACK_DRING_REG	0x0080
#define VIO_RCV_DRING_REG	0x0100
#define VIO_SND_RDX		0x0200
#define VIO_ACK_RDX		0x0400
#define VIO_RCV_RDX		0x0800

#if 0
FIXME openbsd		
	struct timeout	sc_handshake_to;
#endif		

	uint8_t		sc_xfer_mode;

	uint32_t	sc_local_sid;
	uint64_t	sc_dring_ident;
	uint64_t	sc_seq_no;

	u_int		sc_tx_prod;
	u_int		sc_tx_cons;

	u_int		sc_peer_state;

	struct ldc_map	*sc_lm;
	struct vnet_dring *sc_vd;
	struct vnet_soft_desc *sc_vsd;
#define VNET_NUM_SOFT_DESC	128

	size_t		sc_peer_desc_size;
	struct ldc_cookie sc_peer_dring_cookie;
	int		sc_peer_dring_nentries;

	struct pool	sc_pool;

	struct ethercom	sc_ethercom;
	struct ifmedia	sc_media;
	u_int8_t sc_macaddr[ETHER_ADDR_LEN];
};

int vnet_match (device_t, cfdata_t, void *);
void vnet_attach (device_t, device_t, void *);

CFATTACH_DECL_NEW(vnet, sizeof(struct vnet_softc),
    vnet_match, vnet_attach, NULL, NULL);

int	vnet_tx_intr(void *);
int	vnet_rx_intr(void *);
void	vnet_handshake(void *);

void	vio_rx_data(struct ldc_conn *, struct ldc_pkt *);
void	vnet_rx_vio_ctrl(struct vnet_softc *, struct vio_msg *);
void	vnet_rx_vio_ver_info(struct vnet_softc *, struct vio_msg_tag *);
void	vnet_rx_vio_attr_info(struct vnet_softc *, struct vio_msg_tag *);
void	vnet_rx_vio_dring_reg(struct vnet_softc *, struct vio_msg_tag *);
void	vnet_rx_vio_rdx(struct vnet_softc *sc, struct vio_msg_tag *);
void	vnet_rx_vio_data(struct vnet_softc *sc, struct vio_msg *);
void	vnet_rx_vio_desc_data(struct vnet_softc *sc, struct vio_msg_tag *);
void	vnet_rx_vio_dring_data(struct vnet_softc *sc, struct vio_msg_tag *);

void	vnet_ldc_reset(struct ldc_conn *);
void	vnet_ldc_start(struct ldc_conn *);

void	vnet_sendmsg(struct vnet_softc *, void *, size_t);
void	vnet_send_ver_info(struct vnet_softc *, uint16_t, uint16_t);
void	vnet_send_attr_info(struct vnet_softc *);
void	vnet_send_dring_reg(struct vnet_softc *);
void	vio_send_rdx(struct vnet_softc *);
void	vnet_send_dring_data(struct vnet_softc *, uint32_t);

void	vnet_start(struct ifnet *);
void	vnet_start_desc(struct ifnet *);
int		vnet_ioctl (struct ifnet *, u_long, void *);
void	vnet_watchdog(struct ifnet *);

int		vnet_media_change(struct ifnet *);
void	vnet_media_status(struct ifnet *, struct ifmediareq *);

void	vnet_link_state(struct vnet_softc *sc);

void	vnet_setmulti(struct vnet_softc *, int);

void	vnet_init(struct ifnet *);
void	vnet_stop(struct ifnet *);

int vnet_match(device_t parent, cfdata_t match, void *aux)
{

	struct cbus_attach_args *ca = aux;
	if (strcmp(ca->ca_name, "network") == 0)
		return (1);

	return (0);
}

void
vnet_attach(struct device *parent, struct device *self, void *aux)
{
	struct vnet_softc *sc = device_private(self);
	struct cbus_attach_args *ca = aux;
	struct ldc_conn *lc;
	struct ifnet *ifp;

	sc->sc_bustag = ca->ca_bustag;
	sc->sc_dmatag = ca->ca_dmatag;
	sc->sc_tx_ino = ca->ca_tx_ino;
	sc->sc_rx_ino = ca->ca_rx_ino;

	printf(": ivec 0x%lx, 0x%lx", sc->sc_tx_ino, sc->sc_rx_ino);

	/*
	 * Un-configure queues before registering interrupt handlers,
	 * such that we dont get any stale LDC packets or events.
	 */
	hv_ldc_tx_qconf(ca->ca_id, 0, 0);
	hv_ldc_rx_qconf(ca->ca_id, 0, 0);

#if 0
FIXME openbsd	
	sc->sc_tx_ih = bus_intr_establish(ca->ca_bustag, sc->sc_tx_ino,
	    IPL_NET, BUS_INTR_ESTABLISH_MPSAFE, vnet_tx_intr,
	    sc, sc->sc_dv.dv_xname);
	sc->sc_rx_ih = bus_intr_establish(ca->ca_bustag, sc->sc_rx_ino,
	    IPL_NET, BUS_INTR_ESTABLISH_MPSAFE, vnet_rx_intr,
	    sc, sc->sc_dv.dv_xname);
	if (sc->sc_tx_ih == NULL || sc->sc_rx_ih == NULL) {
		printf(", can't establish interrupt\n");
		return;
	}
#else
	sc->sc_tx_ih = bus_intr_establish(ca->ca_bustag, sc->sc_tx_ino,
	    IPL_NET, vnet_tx_intr, sc);
	sc->sc_rx_ih = bus_intr_establish(ca->ca_bustag, sc->sc_rx_ino,
	    IPL_NET, vnet_rx_intr, sc);
	if (sc->sc_tx_ih == NULL || sc->sc_rx_ih == NULL) {
		printf(", can't establish interrupts\n");
		return;
	}
#endif	

	lc = &sc->sc_lc;
	lc->lc_id = ca->ca_id;
	lc->lc_sc = sc;
	lc->lc_reset = vnet_ldc_reset;
	lc->lc_start = vnet_ldc_start;
	lc->lc_rx_data = vio_rx_data;

#if 0
FIXME openbsd	
	timeout_set(&sc->sc_handshake_to, vnet_handshake, sc);
#endif 
	sc->sc_peer_state = VIO_DP_STOPPED;

#if OPENBSD_BUSDMA
	lc->lc_txq = ldc_queue_alloc(sc->sc_dmatag, VNET_TX_ENTRIES);
	if (lc->lc_txq == NULL) {
		printf(", can't allocate tx queue\n");
		return;
	}

	lc->lc_rxq = ldc_queue_alloc(sc->sc_dmatag, VNET_RX_ENTRIES);
	if (lc->lc_rxq == NULL) {
		printf(", can't allocate rx queue\n");
		goto free_txqueue;
	}
#else	
	lc->lc_txq = ldc_queue_alloc(VNET_TX_ENTRIES);
	if (lc->lc_txq == NULL) {
		printf(", can't allocate tx queue\n");
		return;
	}

	lc->lc_rxq = ldc_queue_alloc(VNET_RX_ENTRIES);
	if (lc->lc_rxq == NULL) {
		printf(", can't allocate rx queue\n");
		goto free_txqueue;
	}
#endif
	
	if (OF_getprop(ca->ca_node, "local-mac-address",
				   sc->sc_macaddr, ETHER_ADDR_LEN) > 0) {
		printf(", address %s", ether_sprintf(sc->sc_macaddr));
	} else {
		printf(", cannot retrieve local mac address\n");
		return;
	}

	/*
	 * Each interface gets its own pool.
	 */
#if 0
FIXME openbsd	
	pool_init(&sc->sc_pool, 2048, 0, IPL_NET, 0, sc->sc_dv.dv_xname, NULL);
#else
	pool_init(&sc->sc_pool, 2048, 0, 0, 0, sc->sc_dv.dv_xname, NULL, IPL_NET);
#endif
 
	ifp = &sc->sc_ethercom.ec_if;
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_link_state = LINK_STATE_DOWN;
	ifp->if_ioctl = vnet_ioctl;
	ifp->if_start = vnet_start;
	ifp->if_watchdog = vnet_watchdog;
	strlcpy(ifp->if_xname, device_xname(self), IFNAMSIZ);
#if 0
FIXME openbsd	
	ifq_set_maxlen(&ifp->if_snd, 31); /* XXX */
#else 
 IFQ_SET_MAXLEN(&ifp->if_snd, 31); /* XXX */
#endif 

	ifmedia_init(&sc->sc_media, 0, vnet_media_change, vnet_media_status);
	ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_AUTO, 0, NULL);
	ifmedia_set(&sc->sc_media, IFM_ETHER | IFM_AUTO);

	int error = if_initialize(ifp);
	if (error != 0) {
		printf(", if_initialize() failed\n");
		return;
	}
	ether_ifattach(ifp, sc->sc_macaddr);
	if_register(ifp);
	
	printf("\n");
	return;
free_txqueue:
#if OPENBSD_BUSDMA
	ldc_queue_free(sc->sc_dmatag, lc->lc_txq);
#else	
	ldc_queue_free(lc->lc_txq);
#endif 
}

int
vnet_tx_intr(void *arg)
{
	DPRINTF(("%s: entry\n", __func__));

	struct vnet_softc *sc = arg;
	struct ldc_conn *lc = &sc->sc_lc;
	uint64_t tx_head, tx_tail, tx_state;

	hv_ldc_tx_get_state(lc->lc_id, &tx_head, &tx_tail, &tx_state);
	if (tx_state != lc->lc_tx_state) {
		switch (tx_state) {
		case LDC_CHANNEL_DOWN:
			DPRINTF(("%s: Tx link down\n", __func__));
			break;
		case LDC_CHANNEL_UP:
			DPRINTF(("%s: Tx link up\n", __func__));
			break;
		case LDC_CHANNEL_RESET:
			DPRINTF(("%s: Tx link reset\n", __func__));
			break;
		}
		lc->lc_tx_state = tx_state;
	}

	DPRINTF(("%s: exit\n", __func__));
	return (1);
}
 
int
vnet_rx_intr(void *arg)
{
	DPRINTF(("%s: entry\n", __func__));

	struct vnet_softc *sc = arg;
	struct ldc_conn *lc = &sc->sc_lc;
	uint64_t rx_head, rx_tail, rx_state;
	struct ldc_pkt *lp;
	int err;

	err = hv_ldc_rx_get_state(lc->lc_id, &rx_head, &rx_tail, &rx_state);
	if (err == H_EINVAL)
		return (0);
	if (err != H_EOK) {
		printf("hv_ldc_rx_get_state %d\n", err);
		return (0);
	}

	if (rx_state != lc->lc_rx_state) {
		switch (rx_state) {
		case LDC_CHANNEL_DOWN:
			DPRINTF(("%s: Rx link down\n", __func__));
			lc->lc_tx_seqid = 0;
			lc->lc_state = 0;
			lc->lc_reset(lc);
			if (rx_head == rx_tail)
				break;
			/* Discard and ack pending I/O. */
			DPRINTF(("setting rx qhead to %lld\n", rx_tail));
			err = hv_ldc_rx_set_qhead(lc->lc_id, rx_tail);
			if (err == H_EOK)
				break;
			printf("%s: hv_ldc_rx_set_qhead %d\n", __func__, err);
			break;
		case LDC_CHANNEL_UP:
			DPRINTF(("%s: Rx link up\n", __func__));
#if 0
FIXME openbsd			
			timeout_add_msec(&sc->sc_handshake_to, 500);
#endif 
			break;
		case LDC_CHANNEL_RESET:
			DPRINTF(("%s: Rx link reset\n", __func__));
			lc->lc_tx_seqid = 0;
			lc->lc_state = 0;
			lc->lc_reset(lc);
#if 0
FIXME openbsd			
			timeout_add_msec(&sc->sc_handshake_to, 500);
#endif 
			if (rx_head == rx_tail)
				break;
			/* Discard and ack pending I/O. */
			DPRINTF(("setting rx qhead to %lld\n", rx_tail));
			err = hv_ldc_rx_set_qhead(lc->lc_id, rx_tail);
			if (err == H_EOK)
				break;
			printf("%s: hv_ldc_rx_set_qhead %d\n", __func__, err);
			break;
		}
		lc->lc_rx_state = rx_state;
		return (1);
	}

	if (rx_head == rx_tail)
		return (0);

	lp = (struct ldc_pkt *)(lc->lc_rxq->lq_va + rx_head);
	switch (lp->type) {
	case LDC_CTRL:
		ldc_rx_ctrl(lc, lp);
		break;

	case LDC_DATA:
		ldc_rx_data(lc, lp);
		break;

	default:
		DPRINTF(("%0x02/%0x02/%0x02\n", lp->type, lp->stype,
		    lp->ctrl));
		ldc_reset(lc);
		break;
	}

	if (lc->lc_state == 0)
		return (1);

	rx_head += sizeof(*lp);
	rx_head &= ((lc->lc_rxq->lq_nentries * sizeof(*lp)) - 1);
	err = hv_ldc_rx_set_qhead(lc->lc_id, rx_head);
	if (err != H_EOK)
		printf("%s: hv_ldc_rx_set_qhead %d\n", __func__, err);

	DPRINTF(("%s: exit\n", __func__));
	return (1);
}
 
void
vnet_handshake(void *arg)
{
	struct vnet_softc *sc = arg;

	ldc_send_vers(&sc->sc_lc);
}
 
void
vio_rx_data(struct ldc_conn *lc, struct ldc_pkt *lp)
{
	struct vio_msg *vm = (struct vio_msg *)lp;

	switch (vm->type) {
	case VIO_TYPE_CTRL:
		if ((lp->env & LDC_FRAG_START) == 0 &&
		    (lp->env & LDC_FRAG_STOP) == 0)
			return;
		vnet_rx_vio_ctrl(lc->lc_sc, vm);
		break;

	case VIO_TYPE_DATA:
		if((lp->env & LDC_FRAG_START) == 0)
			return;
		vnet_rx_vio_data(lc->lc_sc, vm);
		break;

	default:
		DPRINTF(("Unhandled packet type 0x%02x\n", vm->type));
		ldc_reset(lc);
		break;
	}
}

void
vnet_rx_vio_ctrl(struct vnet_softc *sc, struct vio_msg *vm)
{
	struct vio_msg_tag *tag = (struct vio_msg_tag *)&vm->type;

	switch (tag->stype_env) {
	case VIO_VER_INFO:
		vnet_rx_vio_ver_info(sc, tag);
		break;
	case VIO_ATTR_INFO:
		vnet_rx_vio_attr_info(sc, tag);
		break;
	case VIO_DRING_REG:
		vnet_rx_vio_dring_reg(sc, tag);
		break;
	case VIO_RDX:
		vnet_rx_vio_rdx(sc, tag);
		break;
	default:
		DPRINTF(("CTRL/0x%02x/0x%04x\n", tag->stype, tag->stype_env));
		break;
	}
}

void
vnet_rx_vio_ver_info(struct vnet_softc *sc, struct vio_msg_tag *tag)
{
	struct vio_ver_info *vi = (struct vio_ver_info *)tag;

	switch (vi->tag.stype) {
	case VIO_SUBTYPE_INFO:
		DPRINTF(("CTRL/INFO/VER_INFO\n"));

		/* Make sure we're talking to a virtual network device. */
		if (vi->dev_class != VDEV_NETWORK &&
		    vi->dev_class != VDEV_NETWORK_SWITCH) {
			/* Huh, we're not talking to a network device? */
			printf("Not a network device\n");
			vi->tag.stype = VIO_SUBTYPE_NACK;
			vnet_sendmsg(sc, vi, sizeof(*vi));
			return;
		}

		if (vi->major != VNET_MAJOR) {
			vi->tag.stype = VIO_SUBTYPE_NACK;
			vi->major = VNET_MAJOR;
			vi->minor = VNET_MINOR;
			vnet_sendmsg(sc, vi, sizeof(*vi));
			return;
		}

		vi->tag.stype = VIO_SUBTYPE_ACK;
		vi->tag.sid = sc->sc_local_sid;
		vi->minor = VNET_MINOR;
		vnet_sendmsg(sc, vi, sizeof(*vi));
		sc->sc_vio_state |= VIO_RCV_VER_INFO;
		break;

	case VIO_SUBTYPE_ACK:
		DPRINTF(("CTRL/ACK/VER_INFO\n"));
		if (!ISSET(sc->sc_vio_state, VIO_SND_VER_INFO)) {
			ldc_reset(&sc->sc_lc);
			break;
		}
		sc->sc_vio_state |= VIO_ACK_VER_INFO;
		break;

	default:
		DPRINTF(("CTRL/0x%02x/VER_INFO\n", vi->tag.stype));
		break;
	}

	if (ISSET(sc->sc_vio_state, VIO_RCV_VER_INFO) &&
	    ISSET(sc->sc_vio_state, VIO_ACK_VER_INFO))
		vnet_send_attr_info(sc);
}

void
vnet_rx_vio_attr_info(struct vnet_softc *sc, struct vio_msg_tag *tag)
{
	struct vnet_attr_info *ai = (struct vnet_attr_info *)tag;

	switch (ai->tag.stype) {
	case VIO_SUBTYPE_INFO:
		DPRINTF(("CTRL/INFO/ATTR_INFO\n"));
		sc->sc_xfer_mode = ai->xfer_mode;

		ai->tag.stype = VIO_SUBTYPE_ACK;
		ai->tag.sid = sc->sc_local_sid;
		vnet_sendmsg(sc, ai, sizeof(*ai));
		sc->sc_vio_state |= VIO_RCV_ATTR_INFO;
		break;

	case VIO_SUBTYPE_ACK:
		DPRINTF(("CTRL/ACK/ATTR_INFO\n"));
		if (!ISSET(sc->sc_vio_state, VIO_SND_ATTR_INFO)) {
			ldc_reset(&sc->sc_lc);
			break;
		}
		sc->sc_vio_state |= VIO_ACK_ATTR_INFO;
		break;

	default:
		DPRINTF(("CTRL/0x%02x/ATTR_INFO\n", ai->tag.stype));
		break;
	}

	if (ISSET(sc->sc_vio_state, VIO_RCV_ATTR_INFO) &&
	    ISSET(sc->sc_vio_state, VIO_ACK_ATTR_INFO)) {
		if (sc->sc_xfer_mode == VIO_DRING_MODE)
			vnet_send_dring_reg(sc);
		else
			vio_send_rdx(sc);
	}
}

void
vnet_rx_vio_dring_reg(struct vnet_softc *sc, struct vio_msg_tag *tag)
{
	struct vio_dring_reg *dr = (struct vio_dring_reg *)tag;

	switch (dr->tag.stype) {
	case VIO_SUBTYPE_INFO:
		DPRINTF(("CTRL/INFO/DRING_REG\n"));

		sc->sc_peer_dring_nentries = dr->num_descriptors;
		sc->sc_peer_desc_size = dr->descriptor_size;
		sc->sc_peer_dring_cookie = dr->cookie[0];

		dr->tag.stype = VIO_SUBTYPE_ACK;
		dr->tag.sid = sc->sc_local_sid;
		vnet_sendmsg(sc, dr, sizeof(*dr));
		sc->sc_vio_state |= VIO_RCV_DRING_REG;
		break;

	case VIO_SUBTYPE_ACK:
		DPRINTF(("CTRL/ACK/DRING_REG\n"));
		if (!ISSET(sc->sc_vio_state, VIO_SND_DRING_REG)) {
			ldc_reset(&sc->sc_lc);
			break;
		}

		sc->sc_dring_ident = dr->dring_ident;
		sc->sc_seq_no = 1;

		sc->sc_vio_state |= VIO_ACK_DRING_REG;
		break;

	default:
		DPRINTF(("CTRL/0x%02x/DRING_REG\n", dr->tag.stype));
		break;
	}

	if (ISSET(sc->sc_vio_state, VIO_RCV_DRING_REG) &&
	    ISSET(sc->sc_vio_state, VIO_ACK_DRING_REG))
		vio_send_rdx(sc);
}

void
vnet_rx_vio_rdx(struct vnet_softc *sc, struct vio_msg_tag *tag)
{
#if 0
FIXME openbsd		
	struct ifnet *ifp = &sc->sc_ac.ac_if;
#else
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
#endif	

	switch(tag->stype) {
	case VIO_SUBTYPE_INFO:
		DPRINTF(("CTRL/INFO/RDX\n"));

		tag->stype = VIO_SUBTYPE_ACK;
		tag->sid = sc->sc_local_sid;
		vnet_sendmsg(sc, tag, sizeof(*tag));
		sc->sc_vio_state |= VIO_RCV_RDX;
		break;

	case VIO_SUBTYPE_ACK:
		DPRINTF(("CTRL/ACK/RDX\n"));
		if (!ISSET(sc->sc_vio_state, VIO_SND_RDX)) {
			ldc_reset(&sc->sc_lc);
			break;
		}
		sc->sc_vio_state |= VIO_ACK_RDX;
		break;

	default:
		DPRINTF(("CTRL/0x%02x/RDX (VIO)\n", tag->stype));
		break;
	}

	if (ISSET(sc->sc_vio_state, VIO_RCV_RDX) &&
	    ISSET(sc->sc_vio_state, VIO_ACK_RDX)) {
		/* Link is up! */
		vnet_link_state(sc);

		/* Configure multicast now that we can. */
		vnet_setmulti(sc, 1);

#if 0
FIXME openbsd	
		KERNEL_LOCK();
#else 
	KERNEL_LOCK(1, curlwp);
#endif 
#if 0
FIXME openbsd	
		ifq_clr_oactive(&ifp->if_snd);
#else
	ifp->if_flags &= ~IFF_OACTIVE;
#endif 
		vnet_start(ifp);
#if 0
FIXME openbsd	
		KERNEL_UNLOCK();
#else
	KERNEL_UNLOCK_ONE(curlwp);
#endif 
	}
}

void
vnet_rx_vio_data(struct vnet_softc *sc, struct vio_msg *vm)
{
	struct vio_msg_tag *tag = (struct vio_msg_tag *)&vm->type;

	if (!ISSET(sc->sc_vio_state, VIO_RCV_RDX) ||
	    !ISSET(sc->sc_vio_state, VIO_ACK_RDX)) {
		DPRINTF(("Spurious DATA/0x%02x/0x%04x\n", tag->stype,
		    tag->stype_env));
		return;
	}

	switch(tag->stype_env) {
	case VIO_DESC_DATA:
		vnet_rx_vio_desc_data(sc, tag);
		break;

	case VIO_DRING_DATA:
		vnet_rx_vio_dring_data(sc, tag);
		break;

	default:
		DPRINTF(("DATA/0x%02x/0x%04x\n", tag->stype, tag->stype_env));
		break;
	}
}

void
vnet_rx_vio_desc_data(struct vnet_softc *sc, struct vio_msg_tag *tag)
{
	struct vnet_desc_msg *dm = (struct vnet_desc_msg *)tag;
	struct ldc_conn *lc = &sc->sc_lc;
	struct ldc_map *map = sc->sc_lm;
#if 0
FIXME openbsd		
	struct ifnet *ifp = &sc->sc_ac.ac_if;
#else 
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
#endif
#if 0
FIXME openbsd	
	struct mbuf_list ml = MBUF_LIST_INITIALIZER();
#endif 
	struct mbuf *m;
	unsigned char *buf;
	paddr_t pa;
	psize_t nbytes;
	u_int cons;
	int err;

	switch(tag->stype) {
	case VIO_SUBTYPE_INFO:
		buf = pool_get(&sc->sc_pool, PR_NOWAIT|PR_ZERO);
		if (buf == NULL) {
#if 0
FIXME openbsd				
			ifp->if_ierrors++;
#else
			if_statinc(ifp, if_ierrors);
#endif 
			goto skip;
		}
		nbytes = roundup(dm->nbytes, 8);

		if (dm->nbytes > (ETHER_MAX_LEN - ETHER_CRC_LEN)) {
#if 0
FIXME openbsd				
			ifp->if_ierrors++;
#else 
			if_statinc(ifp, if_ierrors);
#endif 
			goto skip;
		}

		pmap_extract(pmap_kernel(), (vaddr_t)buf, &pa);
		err = hv_ldc_copy(lc->lc_id, LDC_COPY_IN,
		    dm->cookie[0].addr, pa, nbytes, &nbytes);
		if (err != H_EOK) {
			pool_put(&sc->sc_pool, buf);
#if 0
FIXME openbsd				
			ifp->if_ierrors++;
#else 
			if_statinc(ifp, if_ierrors);
#endif 
			goto skip;
		}

		/* Stupid OBP doesn't align properly. */
#if 0
FIXME openbsd		
                m = m_devget(buf, dm->nbytes, ETHER_ALIGN);
#else
		m = m_devget(buf, dm->nbytes, 0, ifp);
#endif 
		pool_put(&sc->sc_pool, buf);
		if (m == NULL) {
#if 0
FIXME openbsd				
			ifp->if_ierrors++;
#else 
			if_statinc(ifp, if_ierrors);
#endif 
			goto skip;
		}

		/* Pass it on. */
#if 0
FIXME openbsd		
		ml_enqueue(&ml, m);
		if_input(ifp, &ml);
#else		
	if_percpuq_enqueue(ifp->if_percpuq, m);
#endif
	skip:
		dm->tag.stype = VIO_SUBTYPE_ACK;
		dm->tag.sid = sc->sc_local_sid;
		vnet_sendmsg(sc, dm, sizeof(*dm));
		break;

	case VIO_SUBTYPE_ACK:
		DPRINTF(("DATA/ACK/DESC_DATA\n"));

		if (dm->desc_handle != sc->sc_tx_cons) {
			printf("out of order\n");
			return;
		}

		cons = sc->sc_tx_cons & (sc->sc_vd->vd_nentries - 1);

		map->lm_slot[sc->sc_vsd[cons].vsd_map_idx].entry = 0;
#if 0
FIXME openbsd		
		atomic_dec_int(&map->lm_count);
#else
		map->lm_count--;
#endif 

		pool_put(&sc->sc_pool, sc->sc_vsd[cons].vsd_buf);
		sc->sc_vsd[cons].vsd_buf = NULL;

		sc->sc_tx_cons++;
		break;

	case VIO_SUBTYPE_NACK:
		DPRINTF(("DATA/NACK/DESC_DATA\n"));
		break;

	default:
		DPRINTF(("DATA/0x%02x/DESC_DATA\n", tag->stype));
		break;
	}
}

void
vnet_rx_vio_dring_data(struct vnet_softc *sc, struct vio_msg_tag *tag)
{
	struct vio_dring_msg *dm = (struct vio_dring_msg *)tag;
	struct ldc_conn *lc = &sc->sc_lc;
#if 0
FIXME openbsd		
	struct ifnet *ifp = &sc->sc_ac.ac_if;
#else 
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
#endif
	struct mbuf *m = NULL;
	paddr_t pa;
	psize_t nbytes;
	int err;

	switch(tag->stype) {
	case VIO_SUBTYPE_INFO:
	{
		struct vnet_desc desc;
		uint64_t cookie;
		paddr_t desc_pa;
		int idx, ack_end_idx = -1;
#if 0
FIXME openbsd	
		struct mbuf_list ml = MBUF_LIST_INITIALIZER();
#endif 

		idx = dm->start_idx;
		for (;;) {
			cookie = sc->sc_peer_dring_cookie.addr;
			cookie += idx * sc->sc_peer_desc_size;
			nbytes = sc->sc_peer_desc_size;
			pmap_extract(pmap_kernel(), (vaddr_t)&desc, &desc_pa);
			err = hv_ldc_copy(lc->lc_id, LDC_COPY_IN, cookie,
			    desc_pa, nbytes, &nbytes);
			if (err != H_EOK) {
				printf("hv_ldc_copy_in %d\n", err);
				break;
			}

			if (desc.hdr.dstate != VIO_DESC_READY)
				break;

			if (desc.nbytes > (ETHER_MAX_LEN - ETHER_CRC_LEN)) {
#if 0
FIXME openbsd				
				ifp->if_ierrors++;
#else
				if_statinc(ifp, if_ierrors);
#endif 
				goto skip;
			}
#if 0
FIXME openbsd			
			m = MCLGETI(NULL, M_DONTWAIT, NULL, desc.nbytes);
			if (!m)
				break;
#else
			MCLGET(NULL, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0)
				break;
#endif 
			m->m_len = m->m_pkthdr.len = desc.nbytes;
			nbytes = roundup(desc.nbytes + VNET_ETHER_ALIGN, 8);

			pmap_extract(pmap_kernel(), (vaddr_t)m->m_data, &pa);
			err = hv_ldc_copy(lc->lc_id, LDC_COPY_IN,
			    desc.cookie[0].addr, pa, nbytes, &nbytes);
			if (err != H_EOK) {
				m_freem(m);
				goto skip;
			}
			m->m_data += VNET_ETHER_ALIGN;

#if 0
FIXME openbsd		
			ml_enqueue(&ml, m);
#else		
			if_percpuq_enqueue(ifp->if_percpuq, m);
#endif

		skip:
			desc.hdr.dstate = VIO_DESC_DONE;
			nbytes = sc->sc_peer_desc_size;
			err = hv_ldc_copy(lc->lc_id, LDC_COPY_OUT, cookie,
			    desc_pa, nbytes, &nbytes);
			if (err != H_EOK)
				printf("hv_ldc_copy_out %d\n", err);

			ack_end_idx = idx;
			if (++idx == sc->sc_peer_dring_nentries)
				idx = 0;
		}
#if 0
FIXME openbd		
		if_input(ifp, &ml);
#else
		printf("vnet_rx_vio_dring_data() ignoring if_input - FIXME\n");
#endif 

		if (ack_end_idx == -1) {
			dm->tag.stype = VIO_SUBTYPE_NACK;
		} else {
			dm->tag.stype = VIO_SUBTYPE_ACK;
			dm->end_idx = ack_end_idx;
		}
		dm->tag.sid = sc->sc_local_sid;
		dm->proc_state = VIO_DP_STOPPED;
		vnet_sendmsg(sc, dm, sizeof(*dm));
		break;
	}

	case VIO_SUBTYPE_ACK:
	{
		struct ldc_map *map = sc->sc_lm;
		u_int cons, count;

		sc->sc_peer_state = dm->proc_state;

		cons = sc->sc_tx_cons & (sc->sc_vd->vd_nentries - 1);
		while (sc->sc_vd->vd_desc[cons].hdr.dstate == VIO_DESC_DONE) {
			map->lm_slot[sc->sc_vsd[cons].vsd_map_idx].entry = 0;
#if 0
FIXME openbsd		
			atomic_dec_int(&map->lm_count);
#else
			map->lm_count--;
#endif 

			pool_put(&sc->sc_pool, sc->sc_vsd[cons].vsd_buf);
			sc->sc_vsd[cons].vsd_buf = NULL;

			sc->sc_vd->vd_desc[cons].hdr.dstate = VIO_DESC_FREE;
			sc->sc_tx_cons++;
			cons = sc->sc_tx_cons & (sc->sc_vd->vd_nentries - 1);
		}

		count = sc->sc_tx_prod - sc->sc_tx_cons;
		if (count > 0 && sc->sc_peer_state != VIO_DP_ACTIVE)
			vnet_send_dring_data(sc, cons);

#if 0
FIXME openbsd	
		KERNEL_LOCK();
#else 
		KERNEL_LOCK(1, curlwp);
#endif 
		if (count < (sc->sc_vd->vd_nentries - 1))
#if 0
FIXME openbsd	
			ifq_clr_oactive(&ifp->if_snd);
#else
			ifp->if_flags &= ~IFF_OACTIVE;
#endif 
		if (count == 0)
			ifp->if_timer = 0;

		vnet_start(ifp);
#if 0
FIXME openbsd	
		KERNEL_UNLOCK();
#else
		KERNEL_UNLOCK_ONE(curlwp);
#endif 
		break;
	}

	case VIO_SUBTYPE_NACK:
		DPRINTF(("DATA/NACK/DRING_DATA\n"));
		sc->sc_peer_state = VIO_DP_STOPPED;
		break;

	default:
		DPRINTF(("DATA/0x%02x/DRING_DATA\n", tag->stype));
		break;
	}
}

void
vnet_ldc_reset(struct ldc_conn *lc)
{
	struct vnet_softc *sc = lc->lc_sc;
	int i;
#if 0
FIXME openbsd	
	timeout_del(&sc->sc_handshake_to);
#endif 
	sc->sc_tx_prod = sc->sc_tx_cons = 0;
	sc->sc_peer_state = VIO_DP_STOPPED;
	sc->sc_vio_state = 0;
	vnet_link_state(sc);

	sc->sc_lm->lm_next = 1;
	sc->sc_lm->lm_count = 1;
	for (i = 1; i < sc->sc_lm->lm_nentries; i++)
		sc->sc_lm->lm_slot[i].entry = 0;

	for (i = 0; i < sc->sc_vd->vd_nentries; i++) {
		if (sc->sc_vsd[i].vsd_buf) {
			pool_put(&sc->sc_pool, sc->sc_vsd[i].vsd_buf);
			sc->sc_vsd[i].vsd_buf = NULL;
		}
		sc->sc_vd->vd_desc[i].hdr.dstate = VIO_DESC_FREE;
	}
}

void
vnet_ldc_start(struct ldc_conn *lc)
{
	struct vnet_softc *sc = lc->lc_sc;
#if 0
FIXME openbsd	
	timeout_del(&sc->sc_handshake_to);
#endif 
	vnet_send_ver_info(sc, VNET_MAJOR, VNET_MINOR);
}

void
vnet_sendmsg(struct vnet_softc *sc, void *msg, size_t len)
{
	struct ldc_conn *lc = &sc->sc_lc;
	int err;

	err = ldc_send_unreliable(lc, msg, len);
	if (err)
		printf("%s: ldc_send_unreliable: %d\n", __func__, err);
}
 
void
vnet_send_ver_info(struct vnet_softc *sc, uint16_t major, uint16_t minor)
{
	struct vio_ver_info vi;

	bzero(&vi, sizeof(vi));
	vi.tag.type = VIO_TYPE_CTRL;
	vi.tag.stype = VIO_SUBTYPE_INFO;
	vi.tag.stype_env = VIO_VER_INFO;
	vi.tag.sid = sc->sc_local_sid;
	vi.major = major;
	vi.minor = minor;
	vi.dev_class = VDEV_NETWORK;
	vnet_sendmsg(sc, &vi, sizeof(vi));

	sc->sc_vio_state |= VIO_SND_VER_INFO;
}
 
void
vnet_send_attr_info(struct vnet_softc *sc)
{
	struct vnet_attr_info ai;
	int i;

	bzero(&ai, sizeof(ai));
	ai.tag.type = VIO_TYPE_CTRL;
	ai.tag.stype = VIO_SUBTYPE_INFO;
	ai.tag.stype_env = VIO_ATTR_INFO;
	ai.tag.sid = sc->sc_local_sid;
	ai.xfer_mode = VIO_DRING_MODE;
	ai.addr_type = VNET_ADDR_ETHERMAC;
	ai.ack_freq = 0;
	ai.addr = 0;
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		ai.addr <<= 8;
		ai.addr |= sc->sc_macaddr[i];
	}
	ai.mtu = ETHER_MAX_LEN - ETHER_CRC_LEN;
	vnet_sendmsg(sc, &ai, sizeof(ai));

	sc->sc_vio_state |= VIO_SND_ATTR_INFO;
}
 
void
vnet_send_dring_reg(struct vnet_softc *sc)
{
	struct vio_dring_reg dr;

	bzero(&dr, sizeof(dr));
	dr.tag.type = VIO_TYPE_CTRL;
	dr.tag.stype = VIO_SUBTYPE_INFO;
	dr.tag.stype_env = VIO_DRING_REG;
	dr.tag.sid = sc->sc_local_sid;
	dr.dring_ident = 0;
	dr.num_descriptors = sc->sc_vd->vd_nentries;
	dr.descriptor_size = sizeof(struct vnet_desc);
	dr.options = VIO_TX_RING;
	dr.ncookies = 1;
	dr.cookie[0].addr = 0;
	dr.cookie[0].size = PAGE_SIZE;
	vnet_sendmsg(sc, &dr, sizeof(dr));

	sc->sc_vio_state |= VIO_SND_DRING_REG;
};
 
void
vio_send_rdx(struct vnet_softc *sc)
{
	struct vio_msg_tag tag;

	tag.type = VIO_TYPE_CTRL;
	tag.stype = VIO_SUBTYPE_INFO;
	tag.stype_env = VIO_RDX;
	tag.sid = sc->sc_local_sid;
	vnet_sendmsg(sc, &tag, sizeof(tag));

	sc->sc_vio_state |= VIO_SND_RDX;
}
 
void
vnet_send_dring_data(struct vnet_softc *sc, uint32_t start_idx)
{
	struct vio_dring_msg dm;
	u_int peer_state;

	peer_state = atomic_swap_uint(&sc->sc_peer_state, VIO_DP_ACTIVE);
	if (peer_state == VIO_DP_ACTIVE)
		return;

	bzero(&dm, sizeof(dm));
	dm.tag.type = VIO_TYPE_DATA;
	dm.tag.stype = VIO_SUBTYPE_INFO;
	dm.tag.stype_env = VIO_DRING_DATA;
	dm.tag.sid = sc->sc_local_sid;
	dm.seq_no = sc->sc_seq_no++;
	dm.dring_ident = sc->sc_dring_ident;
	dm.start_idx = start_idx;
	dm.end_idx = -1;
	vnet_sendmsg(sc, &dm, sizeof(dm));
}
 
void
vnet_start(struct ifnet *ifp)
{
	struct vnet_softc *sc = ifp->if_softc;
	struct ldc_conn *lc = &sc->sc_lc;
	struct ldc_map *map = sc->sc_lm;
	struct mbuf *m;
	paddr_t pa;
	unsigned char *buf;
	uint64_t tx_head, tx_tail, tx_state;
	u_int start, prod, count;
	int err;

#if 0
FIXME openbsd	
	if (!(ifp->if_flags & IFF_RUNNING) || ifq_is_oactive(&ifp->if_snd))
#else			
	if (!(ifp->if_flags & IFF_RUNNING) || (ifp->if_flags & IFF_OACTIVE))
#endif
		return;

#if 0
FIXME openbsd 
	if (ifq_empty(&ifp->if_snd))
#else
	if (IFQ_IS_EMPTY(&ifp->if_snd))
#endif			
		return;

	/*
	 * We cannot transmit packets until a VIO connection has been
	 * established.
	 */
	if (!ISSET(sc->sc_vio_state, VIO_RCV_RDX) ||
	    !ISSET(sc->sc_vio_state, VIO_ACK_RDX))
		return;

	/*
	 * Make sure there is room in the LDC transmit queue to send a
	 * DRING_DATA message.
	 */
	err = hv_ldc_tx_get_state(lc->lc_id, &tx_head, &tx_tail, &tx_state);
	if (err != H_EOK)
		return;
	tx_tail += sizeof(struct ldc_pkt);
	tx_tail &= ((lc->lc_txq->lq_nentries * sizeof(struct ldc_pkt)) - 1);
	if (tx_tail == tx_head) {
#if 0
FIXME openbsd			
		ifq_set_oactive(&ifp->if_snd);
#else
		ifp->if_flags |= IFF_OACTIVE;
#endif		
		return;
	}

	if (sc->sc_xfer_mode == VIO_DESC_MODE) {
		vnet_start_desc(ifp);
		return;
	}

	start = prod = sc->sc_tx_prod & (sc->sc_vd->vd_nentries - 1);
	while (sc->sc_vd->vd_desc[prod].hdr.dstate == VIO_DESC_FREE) {
		count = sc->sc_tx_prod - sc->sc_tx_cons;
		if (count >= (sc->sc_vd->vd_nentries - 1) ||
		    map->lm_count >= map->lm_nentries) {
#if 0
FIXME openbsd
			ifq_set_oactive(&ifp->if_snd);
#else
			ifp->if_flags |= IFF_OACTIVE;
#endif		
			break;
		}

		buf = pool_get(&sc->sc_pool, PR_NOWAIT|PR_ZERO);
		if (buf == NULL) {
#if 0
FIXME openbsd				
			ifq_set_oactive(&ifp->if_snd);
#else
			ifp->if_flags |= IFF_OACTIVE;
#endif 
			break;
		}
#if 0
FIXME openbsd				
		m = ifq_dequeue(&ifp->if_snd);
#else
		IFQ_DEQUEUE(&ifp->if_snd, m);
#endif 
		if (m == NULL) {
			pool_put(&sc->sc_pool, buf);
			break;
		}

		m_copydata(m, 0, m->m_pkthdr.len, buf + VNET_ETHER_ALIGN);

#if NBPFILTER > 0
		/*
		 * If BPF is listening on this interface, let it see the
		 * packet before we commit it to the wire.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m, BPF_DIRECTION_OUT);
#endif

		pmap_extract(pmap_kernel(), (vaddr_t)buf, &pa);
		KASSERT((pa & ~PAGE_MASK) == (pa & LDC_MTE_RA_MASK));
		while (map->lm_slot[map->lm_next].entry != 0) {
			map->lm_next++;
			map->lm_next &= (map->lm_nentries - 1);
		}
		map->lm_slot[map->lm_next].entry = (pa & LDC_MTE_RA_MASK);
		map->lm_slot[map->lm_next].entry |= LDC_MTE_CPR;
#if 0
FIXME openbsd		
		atomic_inc_int(&map->lm_count);
#else
		map->lm_count++;
#endif 
#if 0
FIXME openbsd		
		sc->sc_vd->vd_desc[prod].nbytes = max(m->m_pkthdr.len, 60);
#else
		sc->sc_vd->vd_desc[prod].nbytes = MAX(m->m_pkthdr.len, 60);
#endif 
		sc->sc_vd->vd_desc[prod].ncookies = 1;
		sc->sc_vd->vd_desc[prod].cookie[0].addr =
		    map->lm_next << PAGE_SHIFT | (pa & PAGE_MASK);
		sc->sc_vd->vd_desc[prod].cookie[0].size = 2048;
		membar_producer();
		sc->sc_vd->vd_desc[prod].hdr.dstate = VIO_DESC_READY;

		sc->sc_vsd[prod].vsd_map_idx = map->lm_next;
		sc->sc_vsd[prod].vsd_buf = buf;

		sc->sc_tx_prod++;
		prod = sc->sc_tx_prod & (sc->sc_vd->vd_nentries - 1);

		m_freem(m);
	}

	membar_producer();

	if (start != prod && sc->sc_peer_state != VIO_DP_ACTIVE) {
		vnet_send_dring_data(sc, start);
		ifp->if_timer = 5;
	}
}

void
vnet_start_desc(struct ifnet *ifp)
{
	struct vnet_softc *sc = ifp->if_softc;
	struct ldc_map *map = sc->sc_lm;
	struct vnet_desc_msg dm;
	struct mbuf *m;
	paddr_t pa;
	unsigned char *buf;
	u_int prod, count;

	for (;;) {
		count = sc->sc_tx_prod - sc->sc_tx_cons;
		if (count >= (sc->sc_vd->vd_nentries - 1) ||
		    map->lm_count >= map->lm_nentries) {
#if 0
FIXME openbsd				
			ifq_set_oactive(&ifp->if_snd);
#else
			ifp->if_flags |= IFF_OACTIVE;
#endif 
			return;
		}

		buf = pool_get(&sc->sc_pool, PR_NOWAIT|PR_ZERO);
		if (buf == NULL) {
#if 0
FIXME openbsd				
			ifq_set_oactive(&ifp->if_snd);
#else
			ifp->if_flags |= IFF_OACTIVE;
#endif 
			return;
		}

#if 0
FIXME openbsd				
		m = ifq_dequeue(&ifp->if_snd);
#else
		IFQ_DEQUEUE(&ifp->if_snd, m);
#endif 
 
		if (m == NULL) {
			pool_put(&sc->sc_pool, buf);
			return;
		}

		m_copydata(m, 0, m->m_pkthdr.len, buf);

#if NBPFILTER > 0
		/*
		 * If BPF is listening on this interface, let it see the
		 * packet before we commit it to the wire.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m, BPF_DIRECTION_OUT);
#endif

		pmap_extract(pmap_kernel(), (vaddr_t)buf, &pa);
		KASSERT((pa & ~PAGE_MASK) == (pa & LDC_MTE_RA_MASK));
		while (map->lm_slot[map->lm_next].entry != 0) {
			map->lm_next++;
			map->lm_next &= (map->lm_nentries - 1);
		}
		map->lm_slot[map->lm_next].entry = (pa & LDC_MTE_RA_MASK);
		map->lm_slot[map->lm_next].entry |= LDC_MTE_CPR;
#if 0
FIXME openbsd		
		atomic_inc_int(&map->lm_count);
#else
		map->lm_count++;
#endif

		prod = sc->sc_tx_prod & (sc->sc_vd->vd_nentries - 1);
		sc->sc_vsd[prod].vsd_map_idx = map->lm_next;
		sc->sc_vsd[prod].vsd_buf = buf;

		bzero(&dm, sizeof(dm));
		dm.tag.type = VIO_TYPE_DATA;
		dm.tag.stype = VIO_SUBTYPE_INFO;
		dm.tag.stype_env = VIO_DESC_DATA;
		dm.tag.sid = sc->sc_local_sid;
		dm.seq_no = sc->sc_seq_no++;
		dm.desc_handle = sc->sc_tx_prod;
#if 0
FIXME openbsd		
		dm.nbytes = max(m->m_pkthdr.len, 60);
#else
		dm.nbytes = MAX(m->m_pkthdr.len, 60);
#endif 
		dm.ncookies = 1;
		dm.cookie[0].addr =
			map->lm_next << PAGE_SHIFT | (pa & PAGE_MASK);
		dm.cookie[0].size = 2048;
		vnet_sendmsg(sc, &dm, sizeof(dm));

		sc->sc_tx_prod++;
		sc->sc_tx_prod &= (sc->sc_vd->vd_nentries - 1);

		m_freem(m);
	}
}

int
vnet_ioctl(struct ifnet *ifp, u_long cmd, void* data)
{

	struct vnet_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	s = splnet();

	switch (cmd) {

		case SIOCSIFADDR:
			ifp->if_flags |= IFF_UP;
			/* FALLTHROUGH */
		case SIOCSIFFLAGS:
			if (ifp->if_flags & IFF_UP) {
				if ((ifp->if_flags & IFF_RUNNING) == 0)
					vnet_init(ifp);
			} else {
				if (ifp->if_flags & IFF_RUNNING)
					vnet_stop(ifp);
			}
		break;

		case SIOCGIFMEDIA:
		case SIOCSIFMEDIA:
			error = ifmedia_ioctl(ifp, ifr, &sc->sc_media, cmd);
			break;

		case SIOCADDMULTI:
		case SIOCDELMULTI:
			/*
			 * XXX Removing all multicast addresses and adding
			 * most of them back, is somewhat retarded.
			 */
			vnet_setmulti(sc, 0);
			error = ether_ioctl(ifp, cmd, data);
			vnet_setmulti(sc, 1);
			if (error == ENETRESET)
				error = 0;
			break;

		default:
			error = ether_ioctl(ifp, cmd, data);
	}

	splx(s);

	return (error);
}

void
vnet_watchdog(struct ifnet *ifp)
{
	struct vnet_softc *sc = ifp->if_softc;

	printf("%s: watchdog timeout\n", sc->sc_dv.dv_xname);
}

int
vnet_media_change(struct ifnet *ifp)
{
	printf("vnet_media_change()\n");
	return (0);
}

void
vnet_media_status(struct ifnet *ifp, struct ifmediareq *imr)
{
	imr->ifm_active = IFM_ETHER | IFM_AUTO;
	imr->ifm_status = IFM_AVALID;
#if 0
FIXME openbsd	
	if (LINK_STATE_IS_UP(ifp->if_link_state) &&
#else
	if (ifp->if_link_state == LINK_STATE_UP &&
#endif	
	    ifp->if_flags & IFF_UP)
		imr->ifm_status |= IFM_ACTIVE;
}

void
vnet_link_state(struct vnet_softc *sc)
{
#if 0
FIXME openbsd		
	struct ifnet *ifp = &sc->sc_ac.ac_if;
#else
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
#endif	
	int link_state = LINK_STATE_DOWN;

#if 0
FIXME openbsd	
	KERNEL_LOCK();
#else
	KERNEL_LOCK(1, curlwp);
#endif 
	if (ISSET(sc->sc_vio_state, VIO_RCV_RDX) &&
	    ISSET(sc->sc_vio_state, VIO_ACK_RDX))
#if 0
FIXME openbsd			
		link_state = LINK_STATE_FULL_DUPLEX;
#else
		link_state = LINK_STATE_UP;
#endif	
	if (ifp->if_link_state != link_state) {
#if 0
FIXME openbsd		
		ifp->if_link_state = link_state;
		if_link_state_change(ifp);
#else
		if_link_state_change(ifp, link_state);
#endif		
	}
#if 0
FIXME openbsd	
	KERNEL_UNLOCK();
#else
	KERNEL_UNLOCK_ONE(curlwp);
#endif 
}

void
vnet_setmulti(struct vnet_softc *sc, int set)
{
	struct ethercom *ec = &sc->sc_ethercom;
	struct ether_multi *enm;
	struct ether_multistep step;
	struct vnet_mcast_info mi;
	int count = 0;

	if (!ISSET(sc->sc_vio_state, VIO_RCV_RDX) ||
	    !ISSET(sc->sc_vio_state, VIO_ACK_RDX))
		return;

	bzero(&mi, sizeof(mi));
	mi.tag.type = VIO_TYPE_CTRL;
	mi.tag.stype = VIO_SUBTYPE_INFO;
	mi.tag.stype_env = VNET_MCAST_INFO;
	mi.tag.sid = sc->sc_local_sid;
	mi.set = set ? 1 : 0;
#if 0
FIXME openbsd	
	KERNEL_LOCK();
#else
	KERNEL_LOCK(1, curlwp);
#endif 
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		/* XXX What about multicast ranges? */
		bcopy(enm->enm_addrlo, mi.mcast_addr[count], ETHER_ADDR_LEN);
		ETHER_NEXT_MULTI(step, enm);

		count++;
		if (count < VNET_NUM_MCAST)
			continue;

		mi.count = VNET_NUM_MCAST;
		vnet_sendmsg(sc, &mi, sizeof(mi));
		count = 0;
	}

	if (count > 0) {
		mi.count = count;
		vnet_sendmsg(sc, &mi, sizeof(mi));
	}
#if 0
FIXME openbsd	
	KERNEL_UNLOCK();
#else
	KERNEL_UNLOCK_ONE(curlwp);
#endif 
}


void
vnet_init(struct ifnet *ifp)
{

	struct vnet_softc *sc = ifp->if_softc;
	struct ldc_conn *lc = &sc->sc_lc;
	int err;
	vaddr_t va;
	paddr_t pa;

#if OPENBSD_BUSDMA
	sc->sc_lm = ldc_map_alloc(sc->sc_dmatag, 2048);
#else 
	sc->sc_lm = ldc_map_alloc(2048);
#endif 
	if (sc->sc_lm == NULL)
		return;

#if OPENBSD_BUSDMA
	err = hv_ldc_set_map_table(lc->lc_id,
							   sc->sc_lm->lm_map->dm_segs[0].ds_addr, sc->sc_lm->lm_nentries);
#else
	va = (vaddr_t)sc->sc_lm->lm_slot;
	pa = 0;
	if (pmap_extract(pmap_kernel(), va, &pa) == FALSE)
		panic("pmap_extract failed %lx\n", va);
	err = hv_ldc_set_map_table(lc->lc_id, pa, 2048);
#endif	
	if (err != H_EOK) {
		printf("hv_ldc_set_map_table %d\n", err);
		return;
	}

	sc->sc_vd = vnet_dring_alloc(sc->sc_dmatag, VNET_NUM_SOFT_DESC);
	if (sc->sc_vd == NULL)
		return;
	sc->sc_vsd = malloc(VNET_NUM_SOFT_DESC * sizeof(*sc->sc_vsd), M_DEVBUF,
	    M_NOWAIT|M_ZERO);
	if (sc->sc_vsd == NULL)
		return;

	sc->sc_lm->lm_slot[0].entry = sc->sc_vd->vd_map->dm_segs[0].ds_addr;
	sc->sc_lm->lm_slot[0].entry &= LDC_MTE_RA_MASK;
	sc->sc_lm->lm_slot[0].entry |= LDC_MTE_CPR | LDC_MTE_CPW;
	sc->sc_lm->lm_next = 1;
	sc->sc_lm->lm_count = 1;

#if OPENBSD_BUSDMA
	err = hv_ldc_tx_qconf(lc->lc_id,
						  lc->lc_txq->lq_map->dm_segs[0].ds_addr, lc->lc_txq->lq_nentries);
#else
	err = hv_ldc_tx_qconf(lc->lc_id, pa, lc->lc_txq->lq_nentries);
#endif	
	if (err != H_EOK)
		printf("hv_ldc_tx_qconf %d\n", err);

#if OPENBSD_BUSDMA
	err = hv_ldc_rx_qconf(lc->lc_id,
						  lc->lc_rxq->lq_map->dm_segs[0].ds_addr, lc->lc_rxq->lq_nentries);
#else
	err = hv_ldc_rx_qconf(lc->lc_id, pa, lc->lc_rxq->lq_nentries);
#endif	
	if (err != H_EOK)
		printf("hv_ldc_rx_qconf %d\n", err);

	cbus_intr_setenabled(sc->sc_bustag, sc->sc_tx_ino, INTR_ENABLED);
	cbus_intr_setenabled(sc->sc_bustag, sc->sc_rx_ino, INTR_ENABLED);

	ldc_send_vers(lc);

	ifp->if_flags |= IFF_RUNNING;
	
}

void
vnet_stop(struct ifnet *ifp)
		
{
	struct vnet_softc *sc = ifp->if_softc;
	struct ldc_conn *lc = &sc->sc_lc;

	ifp->if_flags &= ~IFF_RUNNING;
#if 0
FIXME openbsd	
	ifq_clr_oactive(&ifp->if_snd);
#else
	ifp->if_flags &= ~IFF_OACTIVE;
#endif 
	ifp->if_timer = 0;

	cbus_intr_setenabled(sc->sc_bustag, sc->sc_tx_ino, INTR_DISABLED);
	cbus_intr_setenabled(sc->sc_bustag, sc->sc_rx_ino, INTR_DISABLED);

#if 0
FIXME openbsd
	intr_barrier(sc->sc_tx_ih);
	intr_barrier(sc->sc_rx_ih);
#else
	printf("vnet_stop() intr_barrier() FIXME\n");
#endif	

	hv_ldc_tx_qconf(lc->lc_id, 0, 0);
	hv_ldc_rx_qconf(lc->lc_id, 0, 0);
	lc->lc_tx_seqid = 0;
	lc->lc_state = 0;
	lc->lc_tx_state = lc->lc_rx_state = LDC_CHANNEL_DOWN;
	vnet_ldc_reset(lc);

#if 0
FIXME openbsd	
	free(sc->sc_vsd, M_DEVBUF, VNET_NUM_SOFT_DESC * sizeof(*sc->sc_vsd));
#else
	free(sc->sc_vsd, M_DEVBUF);
#endif 

	vnet_dring_free(sc->sc_dmatag, sc->sc_vd);

	hv_ldc_set_map_table(lc->lc_id, 0, 0);
#if OPENBSD_BUSDMA
	ldc_map_free(sc->sc_dmatag, sc->sc_lm);
#else
	ldc_map_free(sc->sc_lm);
#endif	
}

struct vnet_dring *
vnet_dring_alloc(bus_dma_tag_t t, int nentries)
{
	struct vnet_dring *vd;
	bus_size_t size;
	vaddr_t va;
#if OPENBSD_BUSDMA
	int nsegs;
#endif	
	int i;

	vd = kmem_zalloc(sizeof(struct vnet_dring), KM_SLEEP);
	if (vd == NULL)
		return NULL;

	size = roundup(nentries * sizeof(struct vnet_desc), PAGE_SIZE);

#if OPENBSD_BUSDMA
	if (bus_dmamap_create(t, size, 1, size, 0,
	    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &vd->vd_map) != 0)
		return (NULL);

	if (bus_dmamem_alloc(t, size, PAGE_SIZE, 0, &vd->vd_seg, 1,
	    &nsegs, BUS_DMA_NOWAIT) != 0)
		goto destroy;

	if (bus_dmamem_map(t, &vd->vd_seg, 1, size, &va,
	    BUS_DMA_NOWAIT) != 0)
		goto free;

	if (bus_dmamap_load(t, vd->vd_map, va, size, NULL,
	    BUS_DMA_NOWAIT) != 0)
		goto unmap;
#else
	va = (vaddr_t)kmem_zalloc(size, KM_SLEEP);
#endif	
	vd->vd_desc = (struct vnet_desc *)va;
	vd->vd_nentries = nentries;
	bzero(vd->vd_desc, nentries * sizeof(struct vnet_desc));
	for (i = 0; i < vd->vd_nentries; i++)
		vd->vd_desc[i].hdr.dstate = VIO_DESC_FREE;
	return (vd);

#if OPENBSD_BUSDMA
unmap:
	bus_dmamem_unmap(t, va, size);
free:
	bus_dmamem_free(t, &vd->vd_seg, 1);
destroy:
	bus_dmamap_destroy(t, vd->vd_map);
#endif
	return (NULL);
}

void
vnet_dring_free(bus_dma_tag_t t, struct vnet_dring *vd)
{
	bus_size_t size;

	size = vd->vd_nentries * sizeof(struct vnet_desc);
	size = roundup(size, PAGE_SIZE);

#if OPENBSD_BUSDMA
	bus_dmamap_unload(t, vd->vd_map);
	bus_dmamem_unmap(t, (caddr_t)vd->vd_desc, size);
	bus_dmamem_free(t, &vd->vd_seg, 1);
	bus_dmamap_destroy(t, vd->vd_map);
#else	
	kmem_free(vd->vd_desc, size);
#endif	
	kmem_free(vd, size);
}


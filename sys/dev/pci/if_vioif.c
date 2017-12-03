/*	$NetBSD: if_vioif.c,v 1.2.12.3 2017/12/03 11:37:08 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: if_vioif.c,v 1.2.12.3 2017/12/03 11:37:08 jdolecek Exp $");

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

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/virtioreg.h>
#include <dev/pci/virtiovar.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <net/bpf.h>

#include "ioconf.h"

#ifdef NET_MPSAFE
#define VIOIF_MPSAFE	1
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

/* Feature bits */
#define VIRTIO_NET_F_CSUM	(1<<0)
#define VIRTIO_NET_F_GUEST_CSUM	(1<<1)
#define VIRTIO_NET_F_MAC	(1<<5)
#define VIRTIO_NET_F_GSO	(1<<6)
#define VIRTIO_NET_F_GUEST_TSO4	(1<<7)
#define VIRTIO_NET_F_GUEST_TSO6	(1<<8)
#define VIRTIO_NET_F_GUEST_ECN	(1<<9)
#define VIRTIO_NET_F_GUEST_UFO	(1<<10)
#define VIRTIO_NET_F_HOST_TSO4	(1<<11)
#define VIRTIO_NET_F_HOST_TSO6	(1<<12)
#define VIRTIO_NET_F_HOST_ECN	(1<<13)
#define VIRTIO_NET_F_HOST_UFO	(1<<14)
#define VIRTIO_NET_F_MRG_RXBUF	(1<<15)
#define VIRTIO_NET_F_STATUS	(1<<16)
#define VIRTIO_NET_F_CTRL_VQ	(1<<17)
#define VIRTIO_NET_F_CTRL_RX	(1<<18)
#define VIRTIO_NET_F_CTRL_VLAN	(1<<19)

#define VIRTIO_NET_FLAG_BITS \
	VIRTIO_COMMON_FLAG_BITS \
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


/*
 * if_vioifvar.h:
 */
struct vioif_softc {
	device_t		sc_dev;

	struct virtio_softc	*sc_virtio;
	struct virtqueue	sc_vq[3];
#define VQ_RX	0
#define VQ_TX	1
#define VQ_CTRL	2

	uint8_t			sc_mac[ETHER_ADDR_LEN];
	struct ethercom		sc_ethercom;
	short			sc_deferred_init_done;
	bool			sc_link_active;

	/* bus_dmamem */
	bus_dma_segment_t	sc_hdr_segs[1];
	struct virtio_net_hdr	*sc_hdrs;
#define sc_rx_hdrs	sc_hdrs
	struct virtio_net_hdr	*sc_tx_hdrs;
	struct virtio_net_ctrl_cmd *sc_ctrl_cmd;
	struct virtio_net_ctrl_status *sc_ctrl_status;
	struct virtio_net_ctrl_rx *sc_ctrl_rx;
	struct virtio_net_ctrl_mac_tbl *sc_ctrl_mac_tbl_uc;
	struct virtio_net_ctrl_mac_tbl *sc_ctrl_mac_tbl_mc;

	/* kmem */
	bus_dmamap_t		*sc_arrays;
#define sc_rxhdr_dmamaps sc_arrays
	bus_dmamap_t		*sc_txhdr_dmamaps;
	bus_dmamap_t		*sc_rx_dmamaps;
	bus_dmamap_t		*sc_tx_dmamaps;
	struct mbuf		**sc_rx_mbufs;
	struct mbuf		**sc_tx_mbufs;

	bus_dmamap_t		sc_ctrl_cmd_dmamap;
	bus_dmamap_t		sc_ctrl_status_dmamap;
	bus_dmamap_t		sc_ctrl_rx_dmamap;
	bus_dmamap_t		sc_ctrl_tbl_uc_dmamap;
	bus_dmamap_t		sc_ctrl_tbl_mc_dmamap;

	void			*sc_rx_softint;
	void			*sc_ctl_softint;

	enum {
		FREE, INUSE, DONE
	}			sc_ctrl_inuse;
	kcondvar_t		sc_ctrl_wait;
	kmutex_t		sc_ctrl_wait_lock;
	kmutex_t		sc_tx_lock;
	kmutex_t		sc_rx_lock;
	bool			sc_stopping;

	bool			sc_has_ctrl;
};
#define VIRTIO_NET_TX_MAXNSEGS		(16) /* XXX */
#define VIRTIO_NET_CTRL_MAC_MAXENTRIES	(64) /* XXX */

#define VIOIF_TX_LOCK(_sc)	mutex_enter(&(_sc)->sc_tx_lock)
#define VIOIF_TX_UNLOCK(_sc)	mutex_exit(&(_sc)->sc_tx_lock)
#define VIOIF_TX_LOCKED(_sc)	mutex_owned(&(_sc)->sc_tx_lock)
#define VIOIF_RX_LOCK(_sc)	mutex_enter(&(_sc)->sc_rx_lock)
#define VIOIF_RX_UNLOCK(_sc)	mutex_exit(&(_sc)->sc_rx_lock)
#define VIOIF_RX_LOCKED(_sc)	mutex_owned(&(_sc)->sc_rx_lock)

/* cfattach interface functions */
static int	vioif_match(device_t, cfdata_t, void *);
static void	vioif_attach(device_t, device_t, void *);
static void	vioif_deferred_init(device_t);

/* ifnet interface functions */
static int	vioif_init(struct ifnet *);
static void	vioif_stop(struct ifnet *, int);
static void	vioif_start(struct ifnet *);
static int	vioif_ioctl(struct ifnet *, u_long, void *);
static void	vioif_watchdog(struct ifnet *);

/* rx */
static int	vioif_add_rx_mbuf(struct vioif_softc *, int);
static void	vioif_free_rx_mbuf(struct vioif_softc *, int);
static void	vioif_populate_rx_mbufs(struct vioif_softc *);
static void	vioif_populate_rx_mbufs_locked(struct vioif_softc *);
static int	vioif_rx_deq(struct vioif_softc *);
static int	vioif_rx_deq_locked(struct vioif_softc *);
static int	vioif_rx_vq_done(struct virtqueue *);
static void	vioif_rx_softint(void *);
static void	vioif_rx_drain(struct vioif_softc *);

/* tx */
static int	vioif_tx_vq_done(struct virtqueue *);
static int	vioif_tx_vq_done_locked(struct virtqueue *);
static void	vioif_tx_drain(struct vioif_softc *);

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

/* allocate memory */
/*
 * dma memory is used for:
 *   sc_rx_hdrs[slot]:	 metadata array for recieved frames (READ)
 *   sc_tx_hdrs[slot]:	 metadata array for frames to be sent (WRITE)
 *   sc_ctrl_cmd:	 command to be sent via ctrl vq (WRITE)
 *   sc_ctrl_status:	 return value for a command via ctrl vq (READ)
 *   sc_ctrl_rx:	 parameter for a VIRTIO_NET_CTRL_RX class command
 *			 (WRITE)
 *   sc_ctrl_mac_tbl_uc: unicast MAC address filter for a VIRTIO_NET_CTRL_MAC
 *			 class command (WRITE)
 *   sc_ctrl_mac_tbl_mc: multicast MAC address filter for a VIRTIO_NET_CTRL_MAC
 *			 class command (WRITE)
 * sc_ctrl_* structures are allocated only one each; they are protected by
 * sc_ctrl_inuse variable and sc_ctrl_wait condvar.
 */
/*
 * dynamically allocated memory is used for:
 *   sc_rxhdr_dmamaps[slot]:	bus_dmamap_t array for sc_rx_hdrs[slot]
 *   sc_txhdr_dmamaps[slot]:	bus_dmamap_t array for sc_tx_hdrs[slot]
 *   sc_rx_dmamaps[slot]:	bus_dmamap_t array for recieved payload
 *   sc_tx_dmamaps[slot]:	bus_dmamap_t array for sent payload
 *   sc_rx_mbufs[slot]:		mbuf pointer array for recieved frames
 *   sc_tx_mbufs[slot]:		mbuf pointer array for sent frames
 */
static int
vioif_alloc_mems(struct vioif_softc *sc)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	int allocsize, allocsize2, r, rsegs, i;
	void *vaddr;
	intptr_t p;
	int rxqsize, txqsize;

	rxqsize = sc->sc_vq[VQ_RX].vq_num;
	txqsize = sc->sc_vq[VQ_TX].vq_num;

	allocsize = sizeof(struct virtio_net_hdr) * rxqsize;
	allocsize += sizeof(struct virtio_net_hdr) * txqsize;
	if (sc->sc_has_ctrl) {
		allocsize += sizeof(struct virtio_net_ctrl_cmd) * 1;
		allocsize += sizeof(struct virtio_net_ctrl_status) * 1;
		allocsize += sizeof(struct virtio_net_ctrl_rx) * 1;
		allocsize += sizeof(struct virtio_net_ctrl_mac_tbl)
			+ sizeof(struct virtio_net_ctrl_mac_tbl)
			+ ETHER_ADDR_LEN * VIRTIO_NET_CTRL_MAC_MAXENTRIES;
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
			   &sc->sc_hdr_segs[0], 1, allocsize,
			   &vaddr, BUS_DMA_NOWAIT);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev,
				 "DMA memory map failed, "
				 "error code %d\n", r);
		goto err_dmamem_alloc;
	}
	sc->sc_hdrs = vaddr;
	memset(vaddr, 0, allocsize);
	p = (intptr_t) vaddr;
	p += sizeof(struct virtio_net_hdr) * rxqsize;
#define P(name,size)	do { sc->sc_ ##name = (void*) p;	\
			     p += size; } while (0)
	P(tx_hdrs, sizeof(struct virtio_net_hdr) * txqsize);
	if (sc->sc_has_ctrl) {
		P(ctrl_cmd, sizeof(struct virtio_net_ctrl_cmd));
		P(ctrl_status, sizeof(struct virtio_net_ctrl_status));
		P(ctrl_rx, sizeof(struct virtio_net_ctrl_rx));
		P(ctrl_mac_tbl_uc, sizeof(struct virtio_net_ctrl_mac_tbl));
		P(ctrl_mac_tbl_mc,
		  (sizeof(struct virtio_net_ctrl_mac_tbl)
		   + ETHER_ADDR_LEN * VIRTIO_NET_CTRL_MAC_MAXENTRIES));
	}
#undef P

	allocsize2 = sizeof(bus_dmamap_t) * (rxqsize + txqsize);
	allocsize2 += sizeof(bus_dmamap_t) * (rxqsize + txqsize);
	allocsize2 += sizeof(struct mbuf*) * (rxqsize + txqsize);
	sc->sc_arrays = kmem_zalloc(allocsize2, KM_SLEEP);
	sc->sc_txhdr_dmamaps = sc->sc_arrays + rxqsize;
	sc->sc_rx_dmamaps = sc->sc_txhdr_dmamaps + txqsize;
	sc->sc_tx_dmamaps = sc->sc_rx_dmamaps + rxqsize;
	sc->sc_rx_mbufs = (void*) (sc->sc_tx_dmamaps + txqsize);
	sc->sc_tx_mbufs = sc->sc_rx_mbufs + rxqsize;

#define C(map, buf, size, nsegs, rw, usage)				\
	do {								\
		r = bus_dmamap_create(virtio_dmat(vsc), size, nsegs, size, 0, \
				      BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW,	\
				      &sc->sc_ ##map);			\
		if (r != 0) {						\
			aprint_error_dev(sc->sc_dev,			\
					 usage " dmamap creation failed, " \
					 "error code %d\n", r);		\
					 goto err_reqs;			\
		}							\
	} while (0)
#define C_L1(map, buf, size, nsegs, rw, usage)				\
	C(map, buf, size, nsegs, rw, usage);				\
	do {								\
		r = bus_dmamap_load(virtio_dmat(vsc), sc->sc_ ##map,	\
				    &sc->sc_ ##buf, size, NULL,		\
				    BUS_DMA_ ##rw | BUS_DMA_NOWAIT);	\
		if (r != 0) {						\
			aprint_error_dev(sc->sc_dev,			\
					 usage " dmamap load failed, "	\
					 "error code %d\n", r);		\
			goto err_reqs;					\
		}							\
	} while (0)
#define C_L2(map, buf, size, nsegs, rw, usage)				\
	C(map, buf, size, nsegs, rw, usage);				\
	do {								\
		r = bus_dmamap_load(virtio_dmat(vsc), sc->sc_ ##map,	\
				    sc->sc_ ##buf, size, NULL,		\
				    BUS_DMA_ ##rw | BUS_DMA_NOWAIT);	\
		if (r != 0) {						\
			aprint_error_dev(sc->sc_dev,			\
					 usage " dmamap load failed, "	\
					 "error code %d\n", r);		\
			goto err_reqs;					\
		}							\
	} while (0)
	for (i = 0; i < rxqsize; i++) {
		C_L1(rxhdr_dmamaps[i], rx_hdrs[i],
		    sizeof(struct virtio_net_hdr), 1,
		    READ, "rx header");
		C(rx_dmamaps[i], NULL, MCLBYTES, 1, 0, "rx payload");
	}

	for (i = 0; i < txqsize; i++) {
		C_L1(txhdr_dmamaps[i], tx_hdrs[i],
		    sizeof(struct virtio_net_hdr), 1,
		    WRITE, "tx header");
		C(tx_dmamaps[i], NULL, ETHER_MAX_LEN, VIRTIO_NET_TX_MAXNSEGS, 0,
		  "tx payload");
	}

	if (sc->sc_has_ctrl) {
		/* control vq class & command */
		C_L2(ctrl_cmd_dmamap, ctrl_cmd,
		    sizeof(struct virtio_net_ctrl_cmd), 1, WRITE,
		    "control command");
	
		/* control vq status */
		C_L2(ctrl_status_dmamap, ctrl_status,
		    sizeof(struct virtio_net_ctrl_status), 1, READ,
		    "control status");

		/* control vq rx mode command parameter */
		C_L2(ctrl_rx_dmamap, ctrl_rx,
		    sizeof(struct virtio_net_ctrl_rx), 1, WRITE,
		    "rx mode control command");

		/* control vq MAC filter table for unicast */
		/* do not load now since its length is variable */
		C(ctrl_tbl_uc_dmamap, NULL,
		  sizeof(struct virtio_net_ctrl_mac_tbl) + 0, 1, WRITE,
		  "unicast MAC address filter command");

		/* control vq MAC filter table for multicast */
		C(ctrl_tbl_mc_dmamap, NULL,
		  (sizeof(struct virtio_net_ctrl_mac_tbl)
		   + ETHER_ADDR_LEN * VIRTIO_NET_CTRL_MAC_MAXENTRIES),
		  1, WRITE, "multicast MAC address filter command");
	}
#undef C_L2
#undef C_L1
#undef C

	return 0;

err_reqs:
#define D(map)								\
	do {								\
		if (sc->sc_ ##map) {					\
			bus_dmamap_destroy(virtio_dmat(vsc), sc->sc_ ##map); \
			sc->sc_ ##map = NULL;				\
		}							\
	} while (0)
	D(ctrl_tbl_mc_dmamap);
	D(ctrl_tbl_uc_dmamap);
	D(ctrl_rx_dmamap);
	D(ctrl_status_dmamap);
	D(ctrl_cmd_dmamap);
	for (i = 0; i < txqsize; i++) {
		D(tx_dmamaps[i]);
		D(txhdr_dmamaps[i]);
	}
	for (i = 0; i < rxqsize; i++) {
		D(rx_dmamaps[i]);
		D(rxhdr_dmamaps[i]);
	}
#undef D
	if (sc->sc_arrays) {
		kmem_free(sc->sc_arrays, allocsize2);
		sc->sc_arrays = 0;
	}
	bus_dmamem_unmap(virtio_dmat(vsc), sc->sc_hdrs, allocsize);
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
	uint32_t features;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	u_int flags;
	int r, nvqs=0, req_flags;

	if (virtio_child(vsc) != NULL) {
		aprint_normal(": child already attached for %s; "
			      "something wrong...\n",
			      device_xname(parent));
		return;
	}

	sc->sc_dev = self;
	sc->sc_virtio = vsc;
	sc->sc_link_active = false;

	req_flags = 0;

#ifdef VIOIF_MPSAFE
	req_flags |= VIRTIO_F_PCI_INTR_MPSAFE;
#endif
#ifdef VIOIF_SOFTINT_INTR
	req_flags |= VIRTIO_F_PCI_INTR_SOFTINT;
#endif
	req_flags |= VIRTIO_F_PCI_INTR_MSIX;

	virtio_child_attach_start(vsc, self, IPL_NET, sc->sc_vq,
	    vioif_config_change, virtio_vq_intr, req_flags,
	    (VIRTIO_NET_F_MAC | VIRTIO_NET_F_STATUS | VIRTIO_NET_F_CTRL_VQ |
	     VIRTIO_NET_F_CTRL_RX | VIRTIO_F_NOTIFY_ON_EMPTY),
	    VIRTIO_NET_FLAG_BITS);

	features = virtio_features(vsc);

	if (features & VIRTIO_NET_F_MAC) {
		sc->sc_mac[0] = virtio_read_device_config_1(vsc,
						    VIRTIO_NET_CONFIG_MAC+0);
		sc->sc_mac[1] = virtio_read_device_config_1(vsc,
						    VIRTIO_NET_CONFIG_MAC+1);
		sc->sc_mac[2] = virtio_read_device_config_1(vsc,
						    VIRTIO_NET_CONFIG_MAC+2);
		sc->sc_mac[3] = virtio_read_device_config_1(vsc,
						    VIRTIO_NET_CONFIG_MAC+3);
		sc->sc_mac[4] = virtio_read_device_config_1(vsc,
						    VIRTIO_NET_CONFIG_MAC+4);
		sc->sc_mac[5] = virtio_read_device_config_1(vsc,
						    VIRTIO_NET_CONFIG_MAC+5);
	} else {
		/* code stolen from sys/net/if_tap.c */
		struct timeval tv;
		uint32_t ui;
		getmicrouptime(&tv);
		ui = (tv.tv_sec ^ tv.tv_usec) & 0xffffff;
		memcpy(sc->sc_mac+3, (uint8_t *)&ui, 3);
		virtio_write_device_config_1(vsc,
					     VIRTIO_NET_CONFIG_MAC+0,
					     sc->sc_mac[0]);
		virtio_write_device_config_1(vsc,
					     VIRTIO_NET_CONFIG_MAC+1,
					     sc->sc_mac[1]);
		virtio_write_device_config_1(vsc,
					     VIRTIO_NET_CONFIG_MAC+2,
					     sc->sc_mac[2]);
		virtio_write_device_config_1(vsc,
					     VIRTIO_NET_CONFIG_MAC+3,
					     sc->sc_mac[3]);
		virtio_write_device_config_1(vsc,
					     VIRTIO_NET_CONFIG_MAC+4,
					     sc->sc_mac[4]);
		virtio_write_device_config_1(vsc,
					     VIRTIO_NET_CONFIG_MAC+5,
					     sc->sc_mac[5]);
	}

	aprint_normal_dev(self, "Ethernet address %s\n", ether_sprintf(sc->sc_mac));

	mutex_init(&sc->sc_tx_lock, MUTEX_DEFAULT, IPL_NET);
	mutex_init(&sc->sc_rx_lock, MUTEX_DEFAULT, IPL_NET);
	sc->sc_stopping = false;

	/*
	 * Allocating a virtqueue for Rx
	 */
	r = virtio_alloc_vq(vsc, &sc->sc_vq[VQ_RX], VQ_RX,
	    MCLBYTES+sizeof(struct virtio_net_hdr), 2, "rx");
	if (r != 0)
		goto err;
	nvqs = 1;
	sc->sc_vq[VQ_RX].vq_done = vioif_rx_vq_done;

	/*
	 * Allocating a virtqueue for Tx
	 */
	r = virtio_alloc_vq(vsc, &sc->sc_vq[VQ_TX], VQ_TX,
	    (sizeof(struct virtio_net_hdr) + (ETHER_MAX_LEN - ETHER_HDR_LEN)),
	    VIRTIO_NET_TX_MAXNSEGS + 1, "tx");
	if (r != 0)
		goto err;
	nvqs = 2;
	sc->sc_vq[VQ_TX].vq_done = vioif_tx_vq_done;

	virtio_start_vq_intr(vsc, &sc->sc_vq[VQ_RX]);
	virtio_stop_vq_intr(vsc, &sc->sc_vq[VQ_TX]); /* not urgent; do it later */

	if ((features & VIRTIO_NET_F_CTRL_VQ) &&
	    (features & VIRTIO_NET_F_CTRL_RX)) {
		/*
		 * Allocating a virtqueue for control channel
		 */
		r = virtio_alloc_vq(vsc, &sc->sc_vq[VQ_CTRL], VQ_CTRL,
		    NBPG, 1, "control");
		if (r != 0) {
			aprint_error_dev(self, "failed to allocate "
			    "a virtqueue for control channel\n");
			goto skip;
		}

		sc->sc_vq[VQ_CTRL].vq_done = vioif_ctrl_vq_done;
		cv_init(&sc->sc_ctrl_wait, "ctrl_vq");
		mutex_init(&sc->sc_ctrl_wait_lock, MUTEX_DEFAULT, IPL_NET);
		sc->sc_ctrl_inuse = FREE;
		virtio_start_vq_intr(vsc, &sc->sc_vq[VQ_CTRL]);
		sc->sc_has_ctrl = true;
		nvqs = 3;
	}
skip:

#ifdef VIOIF_MPSAFE
	flags = SOFTINT_NET | SOFTINT_MPSAFE;
#else
	flags = SOFTINT_NET;
#endif
	sc->sc_rx_softint = softint_establish(flags, vioif_rx_softint, sc);
	if (sc->sc_rx_softint == NULL) {
		aprint_error_dev(self, "cannot establish rx softint\n");
		goto err;
	}

	sc->sc_ctl_softint = softint_establish(flags, vioif_ctl_softint, sc);
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
	ifp->if_start = vioif_start;
	ifp->if_ioctl = vioif_ioctl;
	ifp->if_init = vioif_init;
	ifp->if_stop = vioif_stop;
	ifp->if_capabilities = 0;
	ifp->if_watchdog = vioif_watchdog;
	IFQ_SET_MAXLEN(&ifp->if_snd, MAX(sc->sc_vq[VQ_TX].vq_num, IFQ_MAXLEN));
	IFQ_SET_READY(&ifp->if_snd);

	sc->sc_ethercom.ec_capabilities |= ETHERCAP_VLAN_MTU;

	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, sc->sc_mac);

	return;

err:
	mutex_destroy(&sc->sc_tx_lock);
	mutex_destroy(&sc->sc_rx_lock);

	if (sc->sc_has_ctrl) {
		cv_destroy(&sc->sc_ctrl_wait);
		mutex_destroy(&sc->sc_ctrl_wait_lock);
	}

	while (nvqs > 0)
		virtio_free_vq(vsc, &sc->sc_vq[--nvqs]);

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
				 "errror code %d\n", r);
}

/*
 * Interface functions for ifnet
 */
static int
vioif_init(struct ifnet *ifp)
{
	struct vioif_softc *sc = ifp->if_softc;
	struct virtio_softc *vsc = sc->sc_virtio;

	vioif_stop(ifp, 0);

	virtio_reinit_start(vsc);
	virtio_negotiate_features(vsc, virtio_features(vsc));
	virtio_start_vq_intr(vsc, &sc->sc_vq[VQ_RX]);
	virtio_stop_vq_intr(vsc, &sc->sc_vq[VQ_TX]);
	if (sc->sc_has_ctrl)
		virtio_start_vq_intr(vsc, &sc->sc_vq[VQ_CTRL]);
	virtio_reinit_end(vsc);

	if (!sc->sc_deferred_init_done) {
		sc->sc_deferred_init_done = 1;
		if (sc->sc_has_ctrl)
			vioif_deferred_init(sc->sc_dev);
	}

	/* Have to set false before vioif_populate_rx_mbufs */
	sc->sc_stopping = false;

	vioif_populate_rx_mbufs(sc);

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

	/* Take the locks to ensure that ongoing TX/RX finish */
	VIOIF_TX_LOCK(sc);
	VIOIF_RX_LOCK(sc);
	sc->sc_stopping = true;
	VIOIF_RX_UNLOCK(sc);
	VIOIF_TX_UNLOCK(sc);

	/* disable interrupts */
	virtio_stop_vq_intr(vsc, &sc->sc_vq[VQ_RX]);
	virtio_stop_vq_intr(vsc, &sc->sc_vq[VQ_TX]);
	if (sc->sc_has_ctrl)
		virtio_stop_vq_intr(vsc, &sc->sc_vq[VQ_CTRL]);

	/* only way to stop I/O and DMA is resetting... */
	virtio_reset(vsc);
	vioif_rx_deq(sc);
	vioif_tx_drain(sc);
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	sc->sc_link_active = false;

	if (disable)
		vioif_rx_drain(sc);
}

static void
vioif_start(struct ifnet *ifp)
{
	struct vioif_softc *sc = ifp->if_softc;
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtqueue *vq = &sc->sc_vq[VQ_TX];
	struct mbuf *m;
	int queued = 0;

	VIOIF_TX_LOCK(sc);

	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING ||
	    !sc->sc_link_active)
		goto out;

	if (sc->sc_stopping)
		goto out;

	for (;;) {
		int slot, r;

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
					 sc->sc_tx_dmamaps[slot],
					 m, BUS_DMA_WRITE|BUS_DMA_NOWAIT);
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
					 sc->sc_tx_dmamaps[slot],
					 m, BUS_DMA_WRITE|BUS_DMA_NOWAIT);
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
					sc->sc_tx_dmamaps[slot]->dm_nsegs + 1);
		if (r != 0) {
			aprint_error_dev(sc->sc_dev,
	   		    "virtio_enqueue_reserve failed, error code %d\n",
			    r);
			bus_dmamap_unload(virtio_dmat(vsc),
					  sc->sc_tx_dmamaps[slot]);
			/* slot already freed by virtio_enqueue_reserve */
			m_freem(m);
			continue;
		}

		sc->sc_tx_mbufs[slot] = m;

		memset(&sc->sc_tx_hdrs[slot], 0, sizeof(struct virtio_net_hdr));
		bus_dmamap_sync(virtio_dmat(vsc), sc->sc_tx_dmamaps[slot],
				0, sc->sc_tx_dmamaps[slot]->dm_mapsize,
				BUS_DMASYNC_PREWRITE);
		bus_dmamap_sync(virtio_dmat(vsc), sc->sc_txhdr_dmamaps[slot],
				0, sc->sc_txhdr_dmamaps[slot]->dm_mapsize,
				BUS_DMASYNC_PREWRITE);
		virtio_enqueue(vsc, vq, slot, sc->sc_txhdr_dmamaps[slot], true);
		virtio_enqueue(vsc, vq, slot, sc->sc_tx_dmamaps[slot], true);
		virtio_enqueue_commit(vsc, vq, slot, false);

		queued++;
		bpf_mtap(ifp, m);
	}

	if (queued > 0) {
		virtio_enqueue_commit(vsc, vq, -1, true);
		ifp->if_timer = 5;
	}

out:
	VIOIF_TX_UNLOCK(sc);
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

	if (ifp->if_flags & IFF_RUNNING)
		vioif_tx_vq_done(&sc->sc_vq[VQ_TX]);
}


/*
 * Recieve implementation
 */
/* allocate and initialize a mbuf for recieve */
static int
vioif_add_rx_mbuf(struct vioif_softc *sc, int i)
{
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
	sc->sc_rx_mbufs[i] = m;
	m->m_len = m->m_pkthdr.len = m->m_ext.ext_size;
	r = bus_dmamap_load_mbuf(virtio_dmat(sc->sc_virtio),
				 sc->sc_rx_dmamaps[i],
				 m, BUS_DMA_READ|BUS_DMA_NOWAIT);
	if (r) {
		m_freem(m);
		sc->sc_rx_mbufs[i] = 0;
		return r;
	}

	return 0;
}

/* free a mbuf for recieve */
static void
vioif_free_rx_mbuf(struct vioif_softc *sc, int i)
{
	bus_dmamap_unload(virtio_dmat(sc->sc_virtio), sc->sc_rx_dmamaps[i]);
	m_freem(sc->sc_rx_mbufs[i]);
	sc->sc_rx_mbufs[i] = NULL;
}

/* add mbufs for all the empty recieve slots */
static void
vioif_populate_rx_mbufs(struct vioif_softc *sc)
{
	VIOIF_RX_LOCK(sc);
	vioif_populate_rx_mbufs_locked(sc);
	VIOIF_RX_UNLOCK(sc);
}

static void
vioif_populate_rx_mbufs_locked(struct vioif_softc *sc)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	int i, r, ndone = 0;
	struct virtqueue *vq = &sc->sc_vq[VQ_RX];

	KASSERT(VIOIF_RX_LOCKED(sc));

	if (sc->sc_stopping)
		return;

	for (i = 0; i < vq->vq_num; i++) {
		int slot;
		r = virtio_enqueue_prep(vsc, vq, &slot);
		if (r == EAGAIN)
			break;
		if (r != 0)
			panic("enqueue_prep for rx buffers");
		if (sc->sc_rx_mbufs[slot] == NULL) {
			r = vioif_add_rx_mbuf(sc, slot);
			if (r != 0) {
				printf("%s: rx mbuf allocation failed, "
				       "error code %d\n",
				       device_xname(sc->sc_dev), r);
				break;
			}
		}
		r = virtio_enqueue_reserve(vsc, vq, slot,
					sc->sc_rx_dmamaps[slot]->dm_nsegs + 1);
		if (r != 0) {
			vioif_free_rx_mbuf(sc, slot);
			break;
		}
		bus_dmamap_sync(virtio_dmat(vsc), sc->sc_rxhdr_dmamaps[slot],
			0, sizeof(struct virtio_net_hdr), BUS_DMASYNC_PREREAD);
		bus_dmamap_sync(virtio_dmat(vsc), sc->sc_rx_dmamaps[slot],
			0, MCLBYTES, BUS_DMASYNC_PREREAD);
		virtio_enqueue(vsc, vq, slot, sc->sc_rxhdr_dmamaps[slot], false);
		virtio_enqueue(vsc, vq, slot, sc->sc_rx_dmamaps[slot], false);
		virtio_enqueue_commit(vsc, vq, slot, false);
		ndone++;
	}
	if (ndone > 0)
		virtio_enqueue_commit(vsc, vq, -1, true);
}

/* dequeue recieved packets */
static int
vioif_rx_deq(struct vioif_softc *sc)
{
	int r;

	KASSERT(sc->sc_stopping);

	VIOIF_RX_LOCK(sc);
	r = vioif_rx_deq_locked(sc);
	VIOIF_RX_UNLOCK(sc);

	return r;
}

/* dequeue recieved packets */
static int
vioif_rx_deq_locked(struct vioif_softc *sc)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtqueue *vq = &sc->sc_vq[VQ_RX];
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *m;
	int r = 0;
	int slot, len;

	KASSERT(VIOIF_RX_LOCKED(sc));

	while (virtio_dequeue(vsc, vq, &slot, &len) == 0) {
		len -= sizeof(struct virtio_net_hdr);
		r = 1;
		bus_dmamap_sync(virtio_dmat(vsc), sc->sc_rxhdr_dmamaps[slot],
				0, sizeof(struct virtio_net_hdr),
				BUS_DMASYNC_POSTREAD);
		bus_dmamap_sync(virtio_dmat(vsc), sc->sc_rx_dmamaps[slot],
				0, MCLBYTES,
				BUS_DMASYNC_POSTREAD);
		m = sc->sc_rx_mbufs[slot];
		KASSERT(m != NULL);
		bus_dmamap_unload(virtio_dmat(vsc), sc->sc_rx_dmamaps[slot]);
		sc->sc_rx_mbufs[slot] = 0;
		virtio_dequeue_commit(vsc, vq, slot);
		m_set_rcvif(m, ifp);
		m->m_len = m->m_pkthdr.len = len;

		VIOIF_RX_UNLOCK(sc);
		if_percpuq_enqueue(ifp->if_percpuq, m);
		VIOIF_RX_LOCK(sc);

		if (sc->sc_stopping)
			break;
	}

	return r;
}

/* rx interrupt; call _dequeue above and schedule a softint */
static int
vioif_rx_vq_done(struct virtqueue *vq)
{
	struct virtio_softc *vsc = vq->vq_owner;
	struct vioif_softc *sc = device_private(virtio_child(vsc));
	int r = 0;

#ifdef VIOIF_SOFTINT_INTR
	KASSERT(!cpu_intr_p());
#endif

	VIOIF_RX_LOCK(sc);

	if (sc->sc_stopping)
		goto out;

	r = vioif_rx_deq_locked(sc);
	if (r)
#ifdef VIOIF_SOFTINT_INTR
		vioif_populate_rx_mbufs_locked(sc);
#else
		softint_schedule(sc->sc_rx_softint);
#endif

out:
	VIOIF_RX_UNLOCK(sc);
	return r;
}

/* softint: enqueue recieve requests for new incoming packets */
static void
vioif_rx_softint(void *arg)
{
	struct vioif_softc *sc = arg;

	vioif_populate_rx_mbufs(sc);
}

/* free all the mbufs; called from if_stop(disable) */
static void
vioif_rx_drain(struct vioif_softc *sc)
{
	struct virtqueue *vq = &sc->sc_vq[VQ_RX];
	int i;

	for (i = 0; i < vq->vq_num; i++) {
		if (sc->sc_rx_mbufs[i] == NULL)
			continue;
		vioif_free_rx_mbuf(sc, i);
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
	int r = 0;

	VIOIF_TX_LOCK(sc);

	if (sc->sc_stopping)
		goto out;

	r = vioif_tx_vq_done_locked(vq);

out:
	VIOIF_TX_UNLOCK(sc);
	if (r)
		if_schedule_deferred_start(ifp);
	return r;
}

static int
vioif_tx_vq_done_locked(struct virtqueue *vq)
{
	struct virtio_softc *vsc = vq->vq_owner;
	struct vioif_softc *sc = device_private(virtio_child(vsc));
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *m;
	int r = 0;
	int slot, len;

	KASSERT(VIOIF_TX_LOCKED(sc));

	while (virtio_dequeue(vsc, vq, &slot, &len) == 0) {
		r++;
		bus_dmamap_sync(virtio_dmat(vsc), sc->sc_txhdr_dmamaps[slot],
				0, sizeof(struct virtio_net_hdr),
				BUS_DMASYNC_POSTWRITE);
		bus_dmamap_sync(virtio_dmat(vsc), sc->sc_tx_dmamaps[slot],
				0, sc->sc_tx_dmamaps[slot]->dm_mapsize,
				BUS_DMASYNC_POSTWRITE);
		m = sc->sc_tx_mbufs[slot];
		bus_dmamap_unload(virtio_dmat(vsc), sc->sc_tx_dmamaps[slot]);
		sc->sc_tx_mbufs[slot] = 0;
		virtio_dequeue_commit(vsc, vq, slot);
		ifp->if_opackets++;
		m_freem(m);
	}

	if (r)
		ifp->if_flags &= ~IFF_OACTIVE;
	return r;
}

/* free all the mbufs already put on vq; called from if_stop(disable) */
static void
vioif_tx_drain(struct vioif_softc *sc)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtqueue *vq = &sc->sc_vq[VQ_TX];
	int i;

	KASSERT(sc->sc_stopping);

	for (i = 0; i < vq->vq_num; i++) {
		if (sc->sc_tx_mbufs[i] == NULL)
			continue;
		bus_dmamap_unload(virtio_dmat(vsc), sc->sc_tx_dmamaps[i]);
		m_freem(sc->sc_tx_mbufs[i]);
		sc->sc_tx_mbufs[i] = NULL;
	}
}

/*
 * Control vq
 */
/* issue a VIRTIO_NET_CTRL_RX class command and wait for completion */
static int
vioif_ctrl_rx(struct vioif_softc *sc, int cmd, bool onoff)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtqueue *vq = &sc->sc_vq[VQ_CTRL];
	int r, slot;

	if (!sc->sc_has_ctrl)
		return ENOTSUP;

	mutex_enter(&sc->sc_ctrl_wait_lock);
	while (sc->sc_ctrl_inuse != FREE)
		cv_wait(&sc->sc_ctrl_wait, &sc->sc_ctrl_wait_lock);
	sc->sc_ctrl_inuse = INUSE;
	mutex_exit(&sc->sc_ctrl_wait_lock);

	sc->sc_ctrl_cmd->class = VIRTIO_NET_CTRL_RX;
	sc->sc_ctrl_cmd->command = cmd;
	sc->sc_ctrl_rx->onoff = onoff;

	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_ctrl_cmd_dmamap,
			0, sizeof(struct virtio_net_ctrl_cmd),
			BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_ctrl_rx_dmamap,
			0, sizeof(struct virtio_net_ctrl_rx),
			BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_ctrl_status_dmamap,
			0, sizeof(struct virtio_net_ctrl_status),
			BUS_DMASYNC_PREREAD);

	r = virtio_enqueue_prep(vsc, vq, &slot);
	if (r != 0)
		panic("%s: control vq busy!?", device_xname(sc->sc_dev));
	r = virtio_enqueue_reserve(vsc, vq, slot, 3);
	if (r != 0)
		panic("%s: control vq busy!?", device_xname(sc->sc_dev));
	virtio_enqueue(vsc, vq, slot, sc->sc_ctrl_cmd_dmamap, true);
	virtio_enqueue(vsc, vq, slot, sc->sc_ctrl_rx_dmamap, true);
	virtio_enqueue(vsc, vq, slot, sc->sc_ctrl_status_dmamap, false);
	virtio_enqueue_commit(vsc, vq, slot, true);

	/* wait for done */
	mutex_enter(&sc->sc_ctrl_wait_lock);
	while (sc->sc_ctrl_inuse != DONE)
		cv_wait(&sc->sc_ctrl_wait, &sc->sc_ctrl_wait_lock);
	mutex_exit(&sc->sc_ctrl_wait_lock);
	/* already dequeueued */

	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_ctrl_cmd_dmamap, 0,
			sizeof(struct virtio_net_ctrl_cmd),
			BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_ctrl_rx_dmamap, 0,
			sizeof(struct virtio_net_ctrl_rx),
			BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_ctrl_status_dmamap, 0,
			sizeof(struct virtio_net_ctrl_status),
			BUS_DMASYNC_POSTREAD);

	if (sc->sc_ctrl_status->ack == VIRTIO_NET_OK)
		r = 0;
	else {
		printf("%s: failed setting rx mode\n",
		       device_xname(sc->sc_dev));
		r = EIO;
	}

	mutex_enter(&sc->sc_ctrl_wait_lock);
	sc->sc_ctrl_inuse = FREE;
	cv_signal(&sc->sc_ctrl_wait);
	mutex_exit(&sc->sc_ctrl_wait_lock);

	return r;
}

static int
vioif_set_promisc(struct vioif_softc *sc, bool onoff)
{
	int r;

	r = vioif_ctrl_rx(sc, VIRTIO_NET_CTRL_RX_PROMISC, onoff);

	return r;
}

static int
vioif_set_allmulti(struct vioif_softc *sc, bool onoff)
{
	int r;

	r = vioif_ctrl_rx(sc, VIRTIO_NET_CTRL_RX_ALLMULTI, onoff);

	return r;
}

/* issue VIRTIO_NET_CTRL_MAC_TABLE_SET command and wait for completion */
static int
vioif_set_rx_filter(struct vioif_softc *sc)
{
	/* filter already set in sc_ctrl_mac_tbl */
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtqueue *vq = &sc->sc_vq[VQ_CTRL];
	int r, slot;

	if (!sc->sc_has_ctrl)
		return ENOTSUP;

	mutex_enter(&sc->sc_ctrl_wait_lock);
	while (sc->sc_ctrl_inuse != FREE)
		cv_wait(&sc->sc_ctrl_wait, &sc->sc_ctrl_wait_lock);
	sc->sc_ctrl_inuse = INUSE;
	mutex_exit(&sc->sc_ctrl_wait_lock);

	sc->sc_ctrl_cmd->class = VIRTIO_NET_CTRL_MAC;
	sc->sc_ctrl_cmd->command = VIRTIO_NET_CTRL_MAC_TABLE_SET;

	r = bus_dmamap_load(virtio_dmat(vsc), sc->sc_ctrl_tbl_uc_dmamap,
			    sc->sc_ctrl_mac_tbl_uc,
			    (sizeof(struct virtio_net_ctrl_mac_tbl)
			  + ETHER_ADDR_LEN * sc->sc_ctrl_mac_tbl_uc->nentries),
			    NULL, BUS_DMA_WRITE|BUS_DMA_NOWAIT);
	if (r) {
		printf("%s: control command dmamap load failed, "
		       "error code %d\n", device_xname(sc->sc_dev), r);
		goto out;
	}
	r = bus_dmamap_load(virtio_dmat(vsc), sc->sc_ctrl_tbl_mc_dmamap,
			    sc->sc_ctrl_mac_tbl_mc,
			    (sizeof(struct virtio_net_ctrl_mac_tbl)
			  + ETHER_ADDR_LEN * sc->sc_ctrl_mac_tbl_mc->nentries),
			    NULL, BUS_DMA_WRITE|BUS_DMA_NOWAIT);
	if (r) {
		printf("%s: control command dmamap load failed, "
		       "error code %d\n", device_xname(sc->sc_dev), r);
		bus_dmamap_unload(virtio_dmat(vsc), sc->sc_ctrl_tbl_uc_dmamap);
		goto out;
	}

	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_ctrl_cmd_dmamap,
			0, sizeof(struct virtio_net_ctrl_cmd),
			BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_ctrl_tbl_uc_dmamap, 0,
			(sizeof(struct virtio_net_ctrl_mac_tbl)
			 + ETHER_ADDR_LEN * sc->sc_ctrl_mac_tbl_uc->nentries),
			BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_ctrl_tbl_mc_dmamap, 0,
			(sizeof(struct virtio_net_ctrl_mac_tbl)
			 + ETHER_ADDR_LEN * sc->sc_ctrl_mac_tbl_mc->nentries),
			BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_ctrl_status_dmamap,
			0, sizeof(struct virtio_net_ctrl_status),
			BUS_DMASYNC_PREREAD);

	r = virtio_enqueue_prep(vsc, vq, &slot);
	if (r != 0)
		panic("%s: control vq busy!?", device_xname(sc->sc_dev));
	r = virtio_enqueue_reserve(vsc, vq, slot, 4);
	if (r != 0)
		panic("%s: control vq busy!?", device_xname(sc->sc_dev));
	virtio_enqueue(vsc, vq, slot, sc->sc_ctrl_cmd_dmamap, true);
	virtio_enqueue(vsc, vq, slot, sc->sc_ctrl_tbl_uc_dmamap, true);
	virtio_enqueue(vsc, vq, slot, sc->sc_ctrl_tbl_mc_dmamap, true);
	virtio_enqueue(vsc, vq, slot, sc->sc_ctrl_status_dmamap, false);
	virtio_enqueue_commit(vsc, vq, slot, true);

	/* wait for done */
	mutex_enter(&sc->sc_ctrl_wait_lock);
	while (sc->sc_ctrl_inuse != DONE)
		cv_wait(&sc->sc_ctrl_wait, &sc->sc_ctrl_wait_lock);
	mutex_exit(&sc->sc_ctrl_wait_lock);
	/* already dequeueued */

	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_ctrl_cmd_dmamap, 0,
			sizeof(struct virtio_net_ctrl_cmd),
			BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_ctrl_tbl_uc_dmamap, 0,
			(sizeof(struct virtio_net_ctrl_mac_tbl)
			 + ETHER_ADDR_LEN * sc->sc_ctrl_mac_tbl_uc->nentries),
			BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_ctrl_tbl_mc_dmamap, 0,
			(sizeof(struct virtio_net_ctrl_mac_tbl)
			 + ETHER_ADDR_LEN * sc->sc_ctrl_mac_tbl_mc->nentries),
			BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_ctrl_status_dmamap, 0,
			sizeof(struct virtio_net_ctrl_status),
			BUS_DMASYNC_POSTREAD);
	bus_dmamap_unload(virtio_dmat(vsc), sc->sc_ctrl_tbl_uc_dmamap);
	bus_dmamap_unload(virtio_dmat(vsc), sc->sc_ctrl_tbl_mc_dmamap);

	if (sc->sc_ctrl_status->ack == VIRTIO_NET_OK)
		r = 0;
	else {
		printf("%s: failed setting rx filter\n",
		       device_xname(sc->sc_dev));
		r = EIO;
	}

out:
	mutex_enter(&sc->sc_ctrl_wait_lock);
	sc->sc_ctrl_inuse = FREE;
	cv_signal(&sc->sc_ctrl_wait);
	mutex_exit(&sc->sc_ctrl_wait_lock);

	return r;
}

/* ctrl vq interrupt; wake up the command issuer */
static int
vioif_ctrl_vq_done(struct virtqueue *vq)
{
	struct virtio_softc *vsc = vq->vq_owner;
	struct vioif_softc *sc = device_private(virtio_child(vsc));
	int r, slot;

	r = virtio_dequeue(vsc, vq, &slot, NULL);
	if (r == ENOENT)
		return 0;
	virtio_dequeue_commit(vsc, vq, slot);

	mutex_enter(&sc->sc_ctrl_wait_lock);
	sc->sc_ctrl_inuse = DONE;
	cv_signal(&sc->sc_ctrl_wait);
	mutex_exit(&sc->sc_ctrl_wait_lock);

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
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ether_multi *enm;
	struct ether_multistep step;
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
	ETHER_LOCK(&sc->sc_ethercom);
	ETHER_FIRST_MULTI(step, &sc->sc_ethercom, enm);
	while (nentries++, enm != NULL) {
		if (nentries >= VIRTIO_NET_CTRL_MAC_MAXENTRIES) {
			allmulti = 1;
			goto set_unlock;
		}
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
			   ETHER_ADDR_LEN)) {
			allmulti = 1;
			goto set_unlock;
		}
		memcpy(sc->sc_ctrl_mac_tbl_mc->macs[nentries],
		       enm->enm_addrlo, ETHER_ADDR_LEN);
		ETHER_NEXT_MULTI(step, enm);
	}
	rxfilter = 1;

set_unlock:
	ETHER_UNLOCK(&sc->sc_ethercom);

set:
	if (rxfilter) {
		sc->sc_ctrl_mac_tbl_uc->nentries = 0;
		sc->sc_ctrl_mac_tbl_mc->nentries = nentries;
		r = vioif_set_rx_filter(sc);
		if (r != 0) {
			rxfilter = 0;
			allmulti = 1; /* fallback */
		}
	} else {
		/* remove rx filter */
		sc->sc_ctrl_mac_tbl_uc->nentries = 0;
		sc->sc_ctrl_mac_tbl_mc->nentries = 0;
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
	bool active, changed;
	int link;

	active = vioif_is_link_up(sc);
	changed = false;

	VIOIF_TX_LOCK(sc);
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
	VIOIF_TX_UNLOCK(sc);

	if (changed)
		if_link_state_change(ifp, link);
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

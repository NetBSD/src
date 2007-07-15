/*	$NetBSD: iopl.c,v 1.24.2.2 2007/07/15 13:21:12 ad Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This is an untested driver for I2O LAN interfaces.  It has at least these
 * issues:
 *
 * - Will leak rx/tx descriptors & mbufs on transport failure.
 * - Doesn't handle token-ring, but that's not a big deal.
 * - Interrupts run at IPL_BIO.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iopl.c,v 1.24.2.2 2007/07/15 13:21:12 ad Exp $");

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/proc.h>
#include <sys/callout.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>

#include <machine/bus.h>

#include <uvm/uvm_extern.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <net/if_fddi.h>
#include <net/if_token.h>
#if NBPFILTER > 0
#include <net/bpf.h>
#endif


#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#endif

#include <dev/i2o/i2o.h>
#include <dev/i2o/iopio.h>
#include <dev/i2o/iopvar.h>
#include <dev/i2o/ioplvar.h>

static void	iopl_attach(struct device *, struct device *, void *);
static int	iopl_match(struct device *, struct cfdata *, void *);

static void	iopl_error(struct iopl_softc *, u_int);
static void	iopl_getpg(struct iopl_softc *, int);
static void	iopl_intr_pg(struct device *, struct iop_msg *, void *);
static void	iopl_intr_evt(struct device *, struct iop_msg *, void *);
static void	iopl_intr_null(struct device *, struct iop_msg *, void *);
static void	iopl_intr_rx(struct device *, struct iop_msg *, void *);
static void	iopl_intr_tx(struct device *, struct iop_msg *, void *);
static void	iopl_tick(void *);
static void	iopl_tick_sched(struct iopl_softc *);

static int	iopl_filter_ether(struct iopl_softc *);
static int	iopl_filter_generic(struct iopl_softc *, u_int64_t *);

static int	iopl_rx_alloc(struct iopl_softc *, int);
static void	iopl_rx_free(struct iopl_softc *);
static void	iopl_rx_post(struct iopl_softc *);
static int	iopl_tx_alloc(struct iopl_softc *, int);
static void	iopl_tx_free(struct iopl_softc *);

static int	iopl_ifmedia_change(struct ifnet *);
static void	iopl_ifmedia_status(struct ifnet *, struct ifmediareq *);

static void	iopl_munge_ether(struct mbuf *, u_int8_t *);
static void	iopl_munge_fddi(struct mbuf *, u_int8_t *);

static int	iopl_init(struct ifnet *);
static int	iopl_ioctl(struct ifnet *, u_long, void *);
static void	iopl_start(struct ifnet *);
static void	iopl_stop(struct ifnet *, int);

CFATTACH_DECL(iopl, sizeof(struct iopl_softc),
    iopl_match, iopl_attach, NULL, NULL);

#ifdef I2OVERBOSE
static const char * const iopl_errors[] = {
	"success",
	"device failure",
	"destination not found",
	"transmit error",
	"transmit aborted",
	"receive error",
	"receive aborted",
	"DMA error",
	"bad packet detected",
	"out of memory",
	"bucket overrun",
	"IOP internal error",
	"cancelled",
	"invalid transaction context",
	"destination address detected",
	"destination address omitted",
	"partial packet returned",
	"temporarily suspended",
};
#endif	/* I2OVERBOSE */

static const struct iopl_media iopl_ether_media[] = {
	{ I2O_LAN_CONNECTION_100BASEVG_ETHERNET,	IFM_100_VG },
	{ I2O_LAN_CONNECTION_100BASEVG_TOKEN_RING,	IFM_100_VG },
	{ I2O_LAN_CONNECTION_ETHERNET_AUI,		IFM_10_5 },
	{ I2O_LAN_CONNECTION_ETHERNET_10BASE5,		IFM_10_5 },
	{ I2O_LAN_CONNECTION_ETHERNET_10BASE2,		IFM_10_2 },
	{ I2O_LAN_CONNECTION_ETHERNET_10BASET,		IFM_10_T },
	{ I2O_LAN_CONNECTION_ETHERNET_10BASEFL,		IFM_10_FL },
	{ I2O_LAN_CONNECTION_ETHERNET_100BASETX,	IFM_100_TX },
	{ I2O_LAN_CONNECTION_ETHERNET_100BASEFX,	IFM_100_FX },
	{ I2O_LAN_CONNECTION_ETHERNET_100BASET4,	IFM_100_T4 },
	{ I2O_LAN_CONNECTION_ETHERNET_1000BASESX,	IFM_1000_SX },
	{ I2O_LAN_CONNECTION_ETHERNET_1000BASELX,	IFM_1000_LX },
	{ I2O_LAN_CONNECTION_ETHERNET_1000BASECX,	IFM_1000_CX },
	{ I2O_LAN_CONNECTION_ETHERNET_1000BASET,	IFM_1000_T },
	{ I2O_LAN_CONNECTION_DEFAULT,			IFM_10_T }
};

static const struct iopl_media iopl_fddi_media[] = {
	{ I2O_LAN_CONNECTION_FDDI_125MBIT,		IFM_FDDI_SMF },
	{ I2O_LAN_CONNECTION_DEFAULT,			IFM_FDDI_SMF },
};

/*
 * Match a supported device.
 */
static int
iopl_match(struct device *parent, struct cfdata *match, void *aux)
{

	return (((struct iop_attach_args *)aux)->ia_class == I2O_CLASS_LAN);
}

/*
 * Attach a supported device.
 */
static void
iopl_attach(struct device *parent, struct device *self, void *aux)
{
	struct iop_attach_args *ia;
	struct iopl_softc *sc;
	struct iop_softc *iop;
	struct ifnet *ifp;
	int rv, iff = 0, ifcap, orphanlimit = 0, maxpktsize; /* XXX */
	struct {
		struct	i2o_param_op_results pr;
		struct	i2o_param_read_results prr;
		union {
			struct	i2o_param_lan_device_info ldi;
			struct	i2o_param_lan_transmit_info ti;
			struct	i2o_param_lan_receive_info ri;
			struct	i2o_param_lan_operation lo;
			struct	i2o_param_lan_batch_control bc;
			struct	i2o_param_lan_mac_address lma;
		} p;
	} __attribute__ ((__packed__)) param;
	const char *typestr, *addrstr;
	char wwn[20];
	u_int8_t hwaddr[8];
	u_int tmp;
	u_int32_t tmp1, tmp2, tmp3;

	sc = device_private(self);
	iop = (struct iop_softc *)parent;
	ia = (struct iop_attach_args *)aux;
	ifp = &sc->sc_if.sci_if;
	sc->sc_tid = ia->ia_tid;
	sc->sc_dmat = iop->sc_dmat;

	/* Say what the device is. */
	printf(": LAN interface");
	iop_print_ident(iop, ia->ia_tid);
	printf("\n");

	rv = iop_field_get_all(iop, ia->ia_tid, I2O_PARAM_LAN_DEVICE_INFO,
	    &param, sizeof(param), NULL);
	if (rv != 0)
		return;

	sc->sc_ms_pg = -1;

	switch (sc->sc_mtype = le16toh(param.p.ldi.lantype)) {
	case I2O_LAN_TYPE_ETHERNET:
		typestr = "Ethernet";
		addrstr = ether_sprintf(param.p.ldi.hwaddr);
		sc->sc_ms_pg = I2O_PARAM_LAN_802_3_STATS;
		sc->sc_rx_prepad = 2;
		sc->sc_munge = iopl_munge_ether;
		orphanlimit = sizeof(struct ether_header);
		iff = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
		break;

	case I2O_LAN_TYPE_100BASEVG:
		typestr = "100VG-AnyLAN";
		addrstr = ether_sprintf(param.p.ldi.hwaddr);
		sc->sc_ms_pg = I2O_PARAM_LAN_802_3_STATS;
		sc->sc_rx_prepad = 2;
		sc->sc_munge = iopl_munge_ether;
		orphanlimit = sizeof(struct ether_header);
		iff = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
		break;

	case I2O_LAN_TYPE_FDDI:
		typestr = "FDDI";
		addrstr = fddi_sprintf(param.p.ldi.hwaddr);
		sc->sc_ms_pg = I2O_PARAM_LAN_FDDI_STATS;
		sc->sc_rx_prepad = 0;
		sc->sc_munge = iopl_munge_fddi;
		orphanlimit = sizeof(struct fddi_header);
		iff = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
		break;

	case I2O_LAN_TYPE_TOKEN_RING:
		typestr = "token ring";
		addrstr = token_sprintf(param.p.ldi.hwaddr);
		iff = IFF_BROADCAST | IFF_MULTICAST;
		break;

	case I2O_LAN_TYPE_FIBRECHANNEL:
		typestr = "fibre channel";
		addrstr = wwn;
		snprintf(wwn, sizeof(wwn), "%08x%08x",
		    ((u_int32_t *)param.p.ldi.hwaddr)[0],
		    ((u_int32_t *)param.p.ldi.hwaddr)[1]);
		iff = IFF_BROADCAST | IFF_MULTICAST;
		break;

	default:
		typestr = "unknown medium";
		addrstr = "unknown";
		break;
	}

	memcpy(hwaddr, param.p.ldi.hwaddr, sizeof(hwaddr));
	printf("%s: %s, address %s, %d Mb/s maximum\n", self->dv_xname,
	    typestr, addrstr,
	    (int)(le64toh(param.p.ldi.maxrxbps) / 1000*1000));
	maxpktsize = le32toh(param.p.ldi.maxpktsize);

	if (sc->sc_ms_pg == -1) {
		printf("%s: medium not supported\n", self->dv_xname);
		return;
	}

	/*
	 * Register our initiators.
	 */
	sc->sc_ii_pg.ii_dv = self;
	sc->sc_ii_pg.ii_intr = iopl_intr_pg;
	sc->sc_ii_pg.ii_flags = 0;
	sc->sc_ii_pg.ii_tid = ia->ia_tid;
	iop_initiator_register(iop, &sc->sc_ii_pg);

	sc->sc_ii_evt.ii_dv = self;
	sc->sc_ii_evt.ii_intr = iopl_intr_evt;
	sc->sc_ii_evt.ii_flags = II_NOTCTX | II_UTILITY;
	sc->sc_ii_evt.ii_tid = ia->ia_tid;
	iop_initiator_register(iop, &sc->sc_ii_evt);

	sc->sc_ii_null.ii_dv = self;
	sc->sc_ii_null.ii_intr = iopl_intr_null;
	sc->sc_ii_null.ii_flags = II_NOTCTX | II_UTILITY;
	sc->sc_ii_null.ii_tid = ia->ia_tid;
	iop_initiator_register(iop, &sc->sc_ii_evt);

	sc->sc_ii_rx.ii_dv = self;
	sc->sc_ii_rx.ii_intr = iopl_intr_rx;
	sc->sc_ii_rx.ii_flags = II_NOTCTX | II_UTILITY;
	sc->sc_ii_rx.ii_tid = ia->ia_tid;
	iop_initiator_register(iop, &sc->sc_ii_rx);

	sc->sc_ii_tx.ii_dv = self;
	sc->sc_ii_tx.ii_intr = iopl_intr_tx;
	sc->sc_ii_tx.ii_flags = II_NOTCTX | II_UTILITY;
	sc->sc_ii_tx.ii_tid = ia->ia_tid;
	iop_initiator_register(iop, &sc->sc_ii_tx);

	/*
	 * Determine some of the capabilities of the interface - in
	 * particular, the maximum number of segments per S/G list, and how
	 * much buffer context we'll need to transmit frames (some adapters
	 * may need the destination address in the buffer context).
	 */
	rv = iop_field_get_all(iop, ia->ia_tid, I2O_PARAM_LAN_TRANSMIT_INFO,
	    &param, sizeof(param), NULL);
	if (rv != 0)
		return;

	tmp = le32toh(param.p.ti.txmodes);

	if ((param.p.ti.txmodes & I2O_LAN_MODES_NO_DA_IN_SGL) == 0)
		sc->sc_tx_ohead = 1 + 1 + 2;
	else
		sc->sc_tx_ohead = 1 + 1;

	ifcap = 0;

	if (((le32toh(iop->sc_status.segnumber) >> 12) & 15) ==
	    I2O_VERSION_20) {
		if ((tmp & I2O_LAN_MODES_IPV4_CHECKSUM) != 0)
			ifcap |= IFCAP_CSUM_IPv4_Tx|IFCAP_CSUM_IPv4_Rx;
		if ((tmp & I2O_LAN_MODES_TCP_CHECKSUM) != 0)
			ifcap |= IFCAP_CSUM_TCPv4_Tx|IFCAP_CSUM_TCPv4_Rx;
		if ((tmp & I2O_LAN_MODES_UDP_CHECKSUM) != 0)
			ifcap |= IFCAP_CSUM_UDPv4_Tx|IFCAP_CSUM_UDPv4_Rx;
#ifdef notyet
		if ((tmp & I2O_LAN_MODES_ICMP_CHECKSUM) != 0)
			ifcap |= IFCAP_CSUM_ICMP;
#endif
	}

	sc->sc_tx_maxsegs =
	    min(le32toh(param.p.ti.maxpktsg), IOPL_MAX_SEGS);
	sc->sc_tx_maxout = le32toh(param.p.ti.maxpktsout);
	sc->sc_tx_maxreq = le32toh(param.p.ti.maxpktsreq);

	rv = iop_field_get_all(iop, ia->ia_tid, I2O_PARAM_LAN_RECEIVE_INFO,
	    &param, sizeof(param), NULL);
	if (rv != 0)
		return;

	sc->sc_rx_maxbkt = le32toh(param.p.ri.maxbuckets);

#ifdef I2ODEBUG
	if (sc->sc_tx_maxsegs == 0)
		panic("%s: sc->sc_tx_maxsegs == 0", self->dv_xname);
	if (sc->sc_tx_maxout == 0)
		panic("%s: sc->sc_tx_maxsegs == 0", self->dv_xname);
	if (sc->sc_tx_maxreq == 0)
		panic("%s: sc->sc_tx_maxsegs == 0", self->dv_xname);
	if (sc->sc_rx_maxbkt == 0)
		panic("%s: sc->sc_rx_maxbkt == 0", self->dv_xname);
#endif

	/*
	 * Set the pre-padding and "orphan" limits.  This is to ensure that
	 * for received packets, the L3 payload will be aligned on a 32-bit
	 * boundary, and the L2 header won't be split between buckets.
	 *
	 * While here, enable error reporting for transmits.  We're not
	 * interested in most errors (e.g. excessive collisions), but others
	 * are of more concern.
	 */
	tmp1 = htole32(sc->sc_rx_prepad);
	tmp2 = htole32(orphanlimit);
	tmp3 = htole32(1);				/* XXX */

	if (iop_field_set(iop, ia->ia_tid, I2O_PARAM_LAN_OPERATION,
	    &tmp1, sizeof(tmp1), I2O_PARAM_LAN_OPERATION_pktprepad))
		return;
	if (iop_field_set(iop, ia->ia_tid, I2O_PARAM_LAN_OPERATION,
	    &tmp2, sizeof(tmp2), I2O_PARAM_LAN_OPERATION_pktorphanlimit))
		return;
	if (iop_field_set(iop, ia->ia_tid, I2O_PARAM_LAN_OPERATION,
	    &tmp3, sizeof(tmp3), I2O_PARAM_LAN_OPERATION_userflags))
		return;

	/*
	 * Set the batching parameters.
	 */
#if IOPL_BATCHING_ENABLED
	/* Select automatic batching, and specify the maximum packet count. */
	tmp1 = htole32(0);
	tmp2 = htole32(IOPL_MAX_BATCH);
	tmp3 = htole32(IOPL_MAX_BATCH);
#else
	/* Force batching off. */
	tmp1 = htole32(1);				/* XXX */
	tmp2 = htole32(1);
	tmp3 = htole32(1);
#endif
	if (iop_field_set(iop, ia->ia_tid, I2O_PARAM_LAN_BATCH_CONTROL,
	    &tmp1, sizeof(tmp1), I2O_PARAM_LAN_BATCH_CONTROL_batchflags))
		return;
	if (iop_field_set(iop, ia->ia_tid, I2O_PARAM_LAN_BATCH_CONTROL,
	    &tmp2, sizeof(tmp2), I2O_PARAM_LAN_BATCH_CONTROL_maxrxbatchcount))
		return;
	if (iop_field_set(iop, ia->ia_tid, I2O_PARAM_LAN_BATCH_CONTROL,
	    &tmp3, sizeof(tmp3), I2O_PARAM_LAN_BATCH_CONTROL_maxtxbatchcount))
		return;

	/*
	 * Get multicast parameters.
	 */
	rv = iop_field_get_all(iop, ia->ia_tid, I2O_PARAM_LAN_MAC_ADDRESS,
	    &param, sizeof(param), NULL);
	if (rv != 0)
		return;

	sc->sc_mcast_max = le32toh(param.p.lma.maxmcastaddr);
	sc->sc_mcast_max = min(IOPL_MAX_MULTI, sc->sc_mcast_max);

	/*
	 * Allocate transmit and receive descriptors.
	 */
	if (iopl_tx_alloc(sc, IOPL_DESCRIPTORS)) {
		printf("%s: unable to allocate transmit descriptors\n",
		    sc->sc_dv.dv_xname);
		return;
	}
	if (iopl_rx_alloc(sc, IOPL_DESCRIPTORS)) {
		printf("%s: unable to allocate receive descriptors\n",
		    sc->sc_dv.dv_xname);
		return;
	}

	/*
	 * Claim the device so that we don't get any nasty surprises.  Allow
	 * failure.
	 */
	iop_util_claim(iop, &sc->sc_ii_evt, 0,
	    I2O_UTIL_CLAIM_NO_PEER_SERVICE |
	    I2O_UTIL_CLAIM_NO_MANAGEMENT_SERVICE |
	    I2O_UTIL_CLAIM_PRIMARY_USER);

	/*
	 * Attach the interface.
	 */
	memcpy(ifp->if_xname, self->dv_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_flags = iff;
	ifp->if_capabilities = ifcap;
	ifp->if_ioctl = iopl_ioctl;
	ifp->if_start = iopl_start;
	ifp->if_stop = iopl_stop;
	ifp->if_init = iopl_init;
	IFQ_SET_READY(&ifp->if_snd);

	if_attach(ifp);

	switch (sc->sc_mtype) {
	case I2O_LAN_TYPE_ETHERNET:
	case I2O_LAN_TYPE_100BASEVG:
		/* Can we handle 802.1Q encapsulated frames? */
		if (maxpktsize >= ETHER_MAX_LEN + ETHER_VLAN_ENCAP_LEN)
			sc->sc_if.sci_ec.ec_capabilities |= ETHERCAP_VLAN_MTU;

		ether_ifattach(ifp, (u_char *)hwaddr);
		break;

	case I2O_LAN_TYPE_FDDI:
		fddi_ifattach(ifp, (u_char *)hwaddr);
		break;
	}

	ifmedia_init(&sc->sc_ifmedia, 0, iopl_ifmedia_change,
	    iopl_ifmedia_status);
}

/*
 * Allocate the specified number of TX descriptors.
 */
static int
iopl_tx_alloc(struct iopl_softc *sc, int count)
{
	struct iopl_tx *tx;
	int i, size, rv;

	if (count > sc->sc_tx_maxout)
		count = sc->sc_tx_maxout;

#ifdef I2ODEBUG
	printf("%s: %d TX descriptors\n", sc->sc_dv.dv_xname, count);
#endif

	size = count * sizeof(*tx);
	sc->sc_tx = malloc(size, M_DEVBUF, M_NOWAIT|M_ZERO);

	for (i = 0, tx = sc->sc_tx; i < count; i++, tx++) {
		rv = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    sc->sc_tx_maxsegs, MCLBYTES, 0,
		    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
		    &tx->tx_dmamap);
		if (rv != 0) {
			iopl_tx_free(sc);
			return (rv);
		}

		tx->tx_ident = i;
		SLIST_INSERT_HEAD(&sc->sc_tx_free, tx, tx_chain);
		sc->sc_tx_freecnt++;
	}

	return (0);
}

/*
 * Free all TX descriptors.
 */
static void
iopl_tx_free(struct iopl_softc *sc)
{
	struct iopl_tx *tx;

	while ((tx = SLIST_FIRST(&sc->sc_tx_free)) != NULL) {
		SLIST_REMOVE_HEAD(&sc->sc_tx_free, tx_chain);
		bus_dmamap_destroy(sc->sc_dmat, tx->tx_dmamap);
	}

	free(sc->sc_tx, M_DEVBUF);
	sc->sc_tx = NULL;
	sc->sc_tx_freecnt = 0;
}

/*
 * Allocate the specified number of RX buckets and descriptors.
 */
static int
iopl_rx_alloc(struct iopl_softc *sc, int count)
{
	struct iopl_rx *rx;
	struct mbuf *m;
	int i, size, rv = 0, state = 0;

	if (count > sc->sc_rx_maxbkt)
		count = sc->sc_rx_maxbkt;

#ifdef I2ODEBUG
	printf("%s: %d RX descriptors\n", sc->sc_dv.dv_xname, count);
#endif

	size = count * sizeof(*rx);
	sc->sc_rx = malloc(size, M_DEVBUF, M_NOWAIT|M_ZERO);

	for (i = 0, rx = sc->sc_rx; i < count; i++, rx++) {
		state = 0;

		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == NULL) {
			rv = ENOBUFS;
			goto bad;
		}

		state++;

		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			rv = ENOBUFS;
			goto bad;
		}

		rv = bus_dmamap_create(sc->sc_dmat, PAGE_SIZE,
		    sc->sc_tx_maxsegs, PAGE_SIZE, 0,
		    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &rx->rx_dmamap);
		if (rv != 0)
			goto bad;

		state++;

		rv = bus_dmamap_load_mbuf(sc->sc_dmat, rx->rx_dmamap, m,
		    BUS_DMA_READ | BUS_DMA_NOWAIT);
		if (rv != 0)
			goto bad;

		rx->rx_ident = i;
		SLIST_INSERT_HEAD(&sc->sc_rx_free, rx, rx_chain);
		sc->sc_rx_freecnt++;
	}

 bad:
	if (state > 1)
		bus_dmamap_destroy(sc->sc_dmat, rx->rx_dmamap);
	if (state > 0)
		m_freem(m);

	iopl_rx_free(sc);
	return (rv);
}

/*
 * Free all RX buckets and descriptors.
 */
static void
iopl_rx_free(struct iopl_softc *sc)
{
	struct iopl_rx *rx;

	while ((rx = SLIST_FIRST(&sc->sc_rx_free)) != NULL) {
		SLIST_REMOVE_HEAD(&sc->sc_rx_free, rx_chain);
		bus_dmamap_destroy(sc->sc_dmat, rx->rx_dmamap);
		m_freem(rx->rx_mbuf);
	}

	free(sc->sc_rx, M_DEVBUF);
	sc->sc_rx = NULL;
	sc->sc_rx_freecnt = 0;
}

/*
 * Post all free RX buckets to the device.
 */
static void
iopl_rx_post(struct iopl_softc *sc)
{
	struct i2o_lan_receive_post *mf;
	struct iopl_rx *rx;
	u_int32_t mb[IOP_MAX_MSG_SIZE / sizeof(u_int32_t)], *sp, *p, *ep, *lp;
	bus_dmamap_t dm;
	bus_dma_segment_t *ds;
	bus_addr_t saddr, eaddr;
	u_int i, slen, tlen;

	mf = (struct i2o_lan_receive_post *)mb;
	mf->msgfunc = I2O_MSGFUNC(I2O_TID_IOP, I2O_LAN_RECEIVE_POST);
	mf->msgictx = sc->sc_ii_rx.ii_ictx;

	ep = mb + (sizeof(mb) >> 2);
	sp = (u_int32_t *)(mf + 1);

	while (sc->sc_rx_freecnt != 0) {
		mf->msgflags = I2O_MSGFLAGS(i2o_lan_receive_post);
		mf->bktcnt = 0;
		p = sp;

		/*
		 * Remove RX descriptors from the list, sync their DMA maps,
		 * and add their buckets to the scatter/gather list for
		 * posting.
		 */
		for (;;) {
			rx = SLIST_FIRST(&sc->sc_rx_free);
			SLIST_REMOVE_HEAD(&sc->sc_rx_free, rx_chain);
			dm = rx->rx_dmamap;

			bus_dmamap_sync(sc->sc_dmat, dm, 0, dm->dm_mapsize,
			    BUS_DMASYNC_PREREAD);

			lp = p;
			*p++ = dm->dm_mapsize | I2O_SGL_PAGE_LIST |
			    I2O_SGL_END_BUFFER | I2O_SGL_BC_32BIT;
			*p++ = rx->rx_ident;

			for (i = dm->dm_nsegs, ds = dm->dm_segs; i > 0; i--) {
				slen = ds->ds_len;
				saddr = ds->ds_addr;
				ds++;

				/*
				 * XXX This should be done with a bus_space
				 * flag.
				 */
				while (slen > 0) {
					eaddr = (saddr + PAGE_SIZE) &
					    ~(PAGE_SIZE - 1);
					tlen = min(eaddr - saddr, slen);
					slen -= tlen;
					*p++ = le32toh(saddr);
					saddr = eaddr;
				}
			}

			if (p + 2 + sc->sc_tx_maxsegs >= ep)
				break;
			if (--sc->sc_rx_freecnt <= 0)
				break;
		}

		/*
		 * Terminate the scatter/gather list and fix up the message
		 * frame size and free RX descriptor count.
		 */
		*lp |= I2O_SGL_END;
		mb[0] += ((p - sp) << 16);

		/*
		 * Finally, post the message frame to the device.
		 */
		iop_post((struct iop_softc *)device_parent(&sc->sc_dv), mb);
	}
}

/*
 * Handle completion of periodic parameter group retrievals.
 */
static void
iopl_intr_pg(struct device *dv, struct iop_msg *im, void *reply)
{
	struct i2o_param_lan_stats *ls;
	struct i2o_param_lan_802_3_stats *les;
	struct i2o_param_lan_media_operation *lmo;
	struct iopl_softc *sc;
	struct iop_softc *iop;
	struct ifnet *ifp;
	struct i2o_reply *rb;
	int pg;

	rb = (struct i2o_reply *)reply;
	sc = (struct iopl_softc *)dv;
	iop = (struct iop_softc *)device_parent(dv);
	ifp = &sc->sc_if.sci_if;

	if ((rb->msgflags & I2O_MSGFLAGS_FAIL) != 0) {
		iopl_tick_sched(sc);
		return;
	}

	iop_msg_unmap(iop, im);
	pg = le16toh(((struct iop_pgop *)im->im_dvcontext)->oat.group);
	free(im->im_dvcontext, M_DEVBUF);
	iop_msg_free(iop, im);

	switch (pg) {
	case I2O_PARAM_LAN_MEDIA_OPERATION:
		lmo = &sc->sc_pb.p.lmo;

		sc->sc_curmbps =
		    (int)(le64toh(lmo->currxbps) / (1000 * 1000));
		sc->sc_conntype = le32toh(lmo->connectiontype);

		if (lmo->linkstatus) {
			/* Necessary only for initialisation. */
			sc->sc_flags |= IOPL_LINK;
		}

		/* Chain the next retrieval. */
		sc->sc_next_pg = I2O_PARAM_LAN_STATS;
		break;

	case I2O_PARAM_LAN_STATS:
		ls = &sc->sc_pb.p.ls;

		/* XXX Not all of these stats may be supported. */
		ifp->if_ipackets = le64toh(ls->ipackets);
		ifp->if_opackets = le64toh(ls->opackets);
		ifp->if_ierrors = le64toh(ls->ierrors);
		ifp->if_oerrors = le64toh(ls->oerrors);

		/* Chain the next retrieval. */
		sc->sc_next_pg = sc->sc_ms_pg;
		break;

	case I2O_PARAM_LAN_802_3_STATS:
		les = &sc->sc_pb.p.les;

		/*
		 * This isn't particularly meaningful: the sum of the number
		 * of packets that encounted a single collision and the
		 * number of packets that encountered multiple collisions.
		 *
		 * XXX Not all of these stats may be supported.
		 */
		ifp->if_collisions = le64toh(les->onecollision) +
		    le64toh(les->manycollisions);

		sc->sc_next_pg = -1;
		break;

	case I2O_PARAM_LAN_FDDI_STATS:
		sc->sc_next_pg = -1;
		break;
	}

	iopl_tick_sched(sc);
}

/*
 * Handle an event signalled by the interface.
 */
static void
iopl_intr_evt(struct device *dv, struct iop_msg *im, void *reply)
{
	struct i2o_util_event_register_reply *rb;
	struct iopl_softc *sc;
	u_int event;

	rb = (struct i2o_util_event_register_reply *)reply;

	if ((rb->msgflags & I2O_MSGFLAGS_FAIL) != 0)
		return;

	sc = (struct iopl_softc *)dv;
	event = le32toh(rb->event);

	switch (event) {
	case I2O_EVENT_LAN_MEDIA_CHANGE:
		sc->sc_flags |= IOPL_MEDIA_CHANGE;
		break;
	case I2O_EVENT_LAN_LINK_UP:
		sc->sc_flags |= IOPL_LINK;
		break;
	case I2O_EVENT_LAN_LINK_DOWN:
		sc->sc_flags &= ~IOPL_LINK;
		break;
	default:
		printf("%s: event 0x%08x received\n", dv->dv_xname, event);
		break;
	}
}

/*
 * Bit-bucket initiator: ignore interrupts signaled by the interface.
 */
static void
iopl_intr_null(struct device *dv, struct iop_msg *im, void *reply)
{

}

/*
 * Handle a receive interrupt.
 */
static void
iopl_intr_rx(struct device *dv, struct iop_msg *im, void *reply)
{
	struct i2o_lan_receive_reply *rb;
	struct iopl_softc *sc;
	struct iopl_rx *rx;
	struct ifnet *ifp;
	struct mbuf *m, *m0;
	u_int32_t *p;
	int off, err, flg, first, lastpkt, lastbkt, rv, pkt = 0; /* XXX */
	int len, i, pktlen[IOPL_MAX_BATCH], csumflgs[IOPL_MAX_BATCH];
	struct mbuf *head[IOPL_MAX_BATCH], *tail[IOPL_MAX_BATCH];

	rb = (struct i2o_lan_receive_reply *)reply;
	sc = (struct iopl_softc *)dv;
	ifp = &sc->sc_if.sci_if;
	p = (u_int32_t *)(rb + 1);

	if ((rb->msgflags & I2O_MSGFLAGS_FAIL) != 0) {
		/* XXX We leak if we get here. */
		return;
	}

	memset(head, 0, sizeof(head));
	memset(pktlen, 0, sizeof(pktlen));
	memset(csumflgs, 0, sizeof(csumflgs));

	/*
	 * Scan through the transaction reply list.  The TRL takes this
	 * form:
	 *
	 * 32-bits	Bucket context
	 * 32-bits	1st packet offset (high 8-bits are control flags)
	 * 32-bits	1st packet length (high 8-bits are error status)
	 * 32-bits	2nd packet offset
	 * 32-bits	2nd packet length
	 * ...
	 * 32-bits	Nth packet offset
	 * 32-bits	Nth packet length
	 * ...
	 * 32-bits	Bucket context
	 * 32-bits	1st packet offset
	 * 32-bits	1st packet length
	 * ...
	 */
	for (lastbkt = 0; !lastbkt;) {
		/*
		 * Return the RX descriptor for this bucket back to the free
		 * list.
		 */
		rx = &sc->sc_rx[*p++];
		SLIST_INSERT_HEAD(&sc->sc_rx_free, rx, rx_chain);
		sc->sc_rx_freecnt++;

		/*
		 * Sync the bucket's DMA map.
		 */
		bus_dmamap_sync(sc->sc_dmat, rx->rx_dmamap, 0,
		    rx->rx_dmamap->dm_mapsize, BUS_DMASYNC_POSTREAD);

		/*
		 * If this is a valid receive, go through the PDB entries
		 * and re-assemble all the packet fragments that we find.
		 * Otherwise, just free up the buckets that we had posted -
		 * we have probably received this reply because the
		 * interface has been reset or suspended.
		 */
		if ((rb->trlflags & I2O_LAN_RECEIVE_REPLY_PDB) == 0) {
			lastbkt = (--rb->trlcount == 0);
			continue;
		}

		m = rx->rx_mbuf;

		for (lastpkt = 0, first = 1, pkt = 0; !lastpkt; pkt++) {
			off = p[0] & 0x00ffffff;
			len = p[1] & 0x00ffffff;
			flg = p[0] >> 24;
			err = p[1] >> 24;
			p += 2;

#ifdef I2ODEBUG
			if (pkt >= IOPL_MAX_BATCH)
				panic("iopl_intr_rx: too many packets");
#endif
			/*
			 * Break out at the right spot later on if this is
			 * the last packet in this bucket, or the last
			 * bucket.
			 */
			if ((flg & 0x40) == 0x40)		/* XXX */
				lastpkt = 1;
			if ((flg & 0xc8) == 0xc0)		/* XXX */
				lastbkt = 1;

			/*
			 * Skip dummy PDB entries.
			 */
			if ((flg & 0x07) == 0x02)		/* XXX */
				continue;

			/*
			 * If the packet was received with errors, then
			 * arrange to dump it.  We allow bad L3 and L4
			 * checksums through for accounting purposes.
			 */
			if (pktlen[pkt] == -1)
				continue;
			if ((off & 0x03) == 0x01) {	/* XXX */
				pktlen[pkt] = -1;
				continue;
			}
			if ((err & I2O_LAN_PDB_ERROR_CKSUM_MASK) != 0) {
				if ((err & I2O_LAN_PDB_ERROR_L3_CKSUM_BAD) != 0)
					csumflgs[pkt] |= M_CSUM_IPv4_BAD;
				if ((err & I2O_LAN_PDB_ERROR_L4_CKSUM_BAD) != 0)
					csumflgs[pkt] |= M_CSUM_TCP_UDP_BAD;
				err &= ~I2O_LAN_PDB_ERROR_CKSUM_MASK;
			}
			if (err != I2O_LAN_PDB_ERROR_NONE) {
				pktlen[pkt] = -1;
				continue;
			}

			if (len <= (MHLEN - sc->sc_rx_prepad)) {
				/*
				 * The fragment is small enough to fit in a
				 * single header mbuf - allocate one and
				 * copy the data into it.  This greatly
				 * reduces memory consumption when we
				 * receive lots of small packets.
				 */
				MGETHDR(m0, M_DONTWAIT, MT_DATA);
				if (m0 == NULL) {
					ifp->if_ierrors++;
					m_freem(m);
					continue;
				}
				m0->m_data += sc->sc_rx_prepad;
				m_copydata(m, 0, len, mtod(m0, void *) + off);
				off = 0;
			} else if (!first) {
				/*
				 * The bucket contains multiple fragments
				 * (each from a different packet).  Allocate
				 * an mbuf header and add a reference to the
				 * storage from the bucket's mbuf.
				 */
				m0 = m_copym(m, off, len, M_DONTWAIT);
				off = 0;
			} else {
				/*
				 * This is the first "large" packet in the
				 * bucket.  Allocate replacement mbuf
				 * storage.  If we fail, drop the packet and
				 * continue.
				 */
				MGETHDR(m0, M_DONTWAIT, MT_DATA);
				if (m0 == NULL) {
					pktlen[pkt] = -1;
					continue;
				}

				MCLGET(m0, M_DONTWAIT);
				if ((m0->m_flags & M_EXT) == 0) {
					pktlen[pkt] = -1;
					m_freem(m0);
					continue;
				}

				/*
				 * If we can't load the new mbuf, then drop
				 * the bucket from the RX list.  XXX Ouch.
				 */
				bus_dmamap_unload(sc->sc_dmat, rx->rx_dmamap);
				rv = bus_dmamap_load_mbuf(sc->sc_dmat,
				    rx->rx_dmamap, m0,
				    BUS_DMA_READ | BUS_DMA_NOWAIT);
				if (rv != 0) {
					printf("%s: unable to load mbuf (%d),"
					    " discarding bucket\n",
					    sc->sc_dv.dv_xname, rv);
					SLIST_REMOVE_HEAD(&sc->sc_rx_free,
					    rx_chain);
					sc->sc_rx_freecnt--;
				}

				rx->rx_mbuf = m0;
				m0 = m;
				first = 0;
			}

			/*
			 * Fix up the mbuf header, and append the mbuf to
			 * the chain for this packet.
			 */
			m0->m_len = len;
			m0->m_data += off;
			if (head[pkt] != NULL)
				tail[pkt]->m_next = m0;
			else
				head[pkt] = m0;
			tail[pkt] = m0;
			pktlen[pkt] += len;
		}
	}

	/*
	 * Pass each received packet on.
	 */
	for (i = 0; i < IOPL_MAX_BATCH; i++) {
		if ((m = head[i]) == NULL)
			continue;

		/*
		 * If the packet was received with errors, we dump it here.
		 */
		if ((len = pktlen[i]) < 0) {
			m_freem(m);
			continue;
		}

		/*
		 * Otherwise, fix up the header, trim off the CRC, feed
		 * a copy to BPF, and then pass it on up.
		 */
		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = len;
		m->m_pkthdr.csum_flags = csumflgs[pkt] | sc->sc_rx_csumflgs;
		m_adj(m, -ETHER_CRC_LEN);

#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif /* NBPFILTER > 0 */

		(*ifp->if_input)(ifp, m);
	}

	/*
	 * Re-post the buckets back to the interface, and try to send more
	 * packets.
	 */
	iopl_rx_post(sc);
	iopl_start(&sc->sc_if.sci_if);
}

/*
 * Handle a transmit interrupt.
 */
static void
iopl_intr_tx(struct device *dv, struct iop_msg *im, void *reply)
{
	struct i2o_lan_send_reply *rb;
	struct iopl_softc *sc;
	struct iopl_tx *tx;
	struct ifnet *ifp;
	int i, bktcnt;

	sc = (struct iopl_softc *)dv;
	ifp = &sc->sc_if.sci_if;
	rb = (struct i2o_lan_send_reply *)reply;

	if ((rb->msgflags & I2O_MSGFLAGS_FAIL) != 0) {
		/* XXX We leak if we get here. */
		return;
	}

	if (rb->reqstatus != I2O_STATUS_SUCCESS)
		iopl_error(sc, le16toh(rb->detail));

	/*
	 * For each packet that has been transmitted, unload the DMA map,
	 * free the source mbuf, and then release the transmit descriptor
	 * back to the pool.
	 */
	bktcnt = (le32toh(rb->msgflags) >> 16) - (sizeof(*rb) >> 2);

	for (i = 0; i <= bktcnt; i++) {
		tx = &sc->sc_tx[rb->tctx[i]];

		bus_dmamap_sync(sc->sc_dmat, tx->tx_dmamap, 0,
		     tx->tx_dmamap->dm_mapsize, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, tx->tx_dmamap);

		m_freem(tx->tx_mbuf);

		SLIST_INSERT_HEAD(&sc->sc_tx_free, tx, tx_chain);
		sc->sc_tx_freecnt++;
	}

	/*
	 * Try to send more packets.
	 */
	ifp->if_flags &= ~IFF_OACTIVE;
	iopl_start(&sc->sc_if.sci_if);
}

/*
 * Describe an error code returned by the adapter.
 */
static void
iopl_error(struct iopl_softc *sc, u_int dsc)
{
#ifdef I2OVERBOSE
	const char *errstr;
#endif

	switch (dsc) {
	case I2O_LAN_DSC_RECEIVE_ERROR:
	case I2O_LAN_DSC_RECEIVE_ABORTED:
	case I2O_LAN_DSC_TRANSMIT_ERROR:
	case I2O_LAN_DSC_TRANSMIT_ABORTED:
	case I2O_LAN_DSC_TEMP_SUSPENDED_STATE:	/* ??? */
		break;

	default:
#ifdef I2OVERBOSE
		if (dsc > sizeof(iopl_errors) / sizeof(iopl_errors[0]))
			errstr = "<unknown>";
		else
			errstr = iopl_errors[dsc];
		printf("%s: error 0x%04x: %s\n", sc->sc_dv.dv_xname, dsc,
		    errstr);
#else
		printf("%s: error 0x%04x\n", sc->sc_dv.dv_xname, dsc);
#endif
		break;
	}
}

/*
 * Retrieve the next scheduled parameter group from the interface.  Called
 * periodically.
 */
static void
iopl_tick(void *cookie)
{
	struct iopl_softc *sc;

	sc = cookie;

	iopl_getpg(sc, sc->sc_next_pg);
}

/*
 * Schedule the next PG retrieval.
 */
static void
iopl_tick_sched(struct iopl_softc *sc)
{
	int s;

	if (sc->sc_next_pg == -1) {
		s = splbio();
		if ((sc->sc_flags & IOPL_MEDIA_CHANGE) != 0) {
			sc->sc_next_pg = I2O_PARAM_LAN_MEDIA_OPERATION;
			sc->sc_flags &= ~IOPL_MEDIA_CHANGE;
		} else
			sc->sc_next_pg = I2O_PARAM_LAN_STATS;
		splx(s);
	}

	callout_reset(&sc->sc_pg_callout, hz / IOPL_TICK_HZ, iopl_tick, sc);
}

/*
 * Request the specified parameter group from the interface, to be delivered
 * to the PG initiator.
 */
static void
iopl_getpg(struct iopl_softc *sc, int pg)
{

	iop_field_get_all((struct iop_softc *)device_parent(&sc->sc_dv),
	    sc->sc_tid, pg, &sc->sc_pb, sizeof(sc->sc_pb), &sc->sc_ii_pg);
}

/*
 * Report on current media status.
 */
static void
iopl_ifmedia_status(struct ifnet *ifp, struct ifmediareq *req)
{
	const struct iopl_media *ilm = NULL; /* XXX */
	struct iopl_softc *sc;
	int s, conntype;

	sc = ifp->if_softc;

	s = splbio();
	conntype = sc->sc_conntype;
	splx(s);

	req->ifm_status = IFM_AVALID;
	if ((sc->sc_flags & IOPL_LINK) != 0)
		req->ifm_status |= IFM_ACTIVE;

	switch (sc->sc_mtype) {
	case I2O_LAN_TYPE_100BASEVG:
	case I2O_LAN_TYPE_ETHERNET:
		ilm = iopl_ether_media;
		req->ifm_active = IFM_ETHER;
		break;

	case I2O_LAN_TYPE_FDDI:
		ilm = iopl_fddi_media;
		req->ifm_active = IFM_FDDI;
		break;
	}

	for (; ilm->ilm_i2o != I2O_LAN_CONNECTION_DEFAULT; ilm++)
		if (ilm->ilm_i2o == conntype)
			break;
	req->ifm_active |= ilm->ilm_ifmedia;

	if (ilm->ilm_i2o == I2O_LAN_CONNECTION_DEFAULT)
		printf("%s: unknown connection type 0x%08x; defaulting\n",
		    sc->sc_dv.dv_xname, conntype);
}

/*
 * Change media parameters.
 */
static int
iopl_ifmedia_change(struct ifnet *ifp)
{
	struct iop_softc *iop;
	struct iopl_softc *sc;
	const struct iopl_media *ilm = NULL; /* XXX */
	u_int subtype;
	u_int32_t ciontype;
	u_int8_t fdx;

	sc = ifp->if_softc;
	iop = (struct iop_softc *)device_parent(&sc->sc_dv);

	subtype = IFM_SUBTYPE(sc->sc_ifmedia.ifm_cur->ifm_media);
	if (subtype == IFM_AUTO)
		ciontype = I2O_LAN_CONNECTION_DEFAULT;
	else {
		switch (sc->sc_mtype) {
		case I2O_LAN_TYPE_100BASEVG:
		case I2O_LAN_TYPE_ETHERNET:
			ilm = iopl_ether_media;
			break;

		case I2O_LAN_TYPE_FDDI:
			ilm = iopl_fddi_media;
			break;
		}

		for (; ilm->ilm_i2o != I2O_LAN_CONNECTION_DEFAULT; ilm++)
			if (ilm->ilm_ifmedia == subtype)
				break;
		if (ilm->ilm_i2o == I2O_LAN_CONNECTION_DEFAULT)
			return (EINVAL);
		ciontype = le32toh(ilm->ilm_i2o);
	}

	if ((sc->sc_ifmedia.ifm_cur->ifm_media & IFM_FDX) != 0)
		fdx = 1;
	else if ((sc->sc_ifmedia.ifm_cur->ifm_media & IFM_HDX) != 0)
		fdx = 0;
	else {
		/*
		 * XXX Not defined as auto-detect, but as "default".
		 */
		fdx = 0xff;
	}

	/*
	 * XXX Can we set all these independently?  Will omitting the
	 * connector type screw us up?
	 */
	iop_field_set(iop, sc->sc_tid, I2O_PARAM_LAN_MEDIA_OPERATION,
	    &ciontype, sizeof(ciontype),
	    I2O_PARAM_LAN_MEDIA_OPERATION_connectiontarget);
#if 0
	iop_field_set(iop, sc->sc_tid, I2O_PARAM_LAN_MEDIA_OPERATION,
	    &certype, sizeof(certype),
	    I2O_PARAM_LAN_MEDIA_OPERATION_connectertarget);
#endif
	iop_field_set(iop, sc->sc_tid, I2O_PARAM_LAN_MEDIA_OPERATION,
	    &fdx, sizeof(fdx),
	    I2O_PARAM_LAN_MEDIA_OPERATION_duplextarget);

	ifp->if_baudrate = ifmedia_baudrate(sc->sc_ifmedia.ifm_cur->ifm_media);
	return (0);
}

/*
 * Initialize the interface.
 */
static int
iopl_init(struct ifnet *ifp)
{
	struct i2o_lan_reset mf;
	struct iopl_softc *sc;
	struct iop_softc *iop;
	int rv, s, flg;
	u_int8_t hwaddr[8];
	u_int32_t txmode, rxmode;
	uint64_t ifcap;

	sc = ifp->if_softc;
	iop = (struct iop_softc *)device_parent(&sc->sc_dv);

	s = splbio();
	flg = sc->sc_flags;
	splx(s);

	if ((flg & IOPL_INITTED) == 0) {
		/*
		 * Reset the interface hardware.
		 */
		mf.msgflags = I2O_MSGFLAGS(i2o_lan_reset);
		mf.msgfunc = I2O_MSGFUNC(I2O_TID_IOP, I2O_LAN_RESET);
		mf.msgictx = sc->sc_ii_null.ii_ictx;
		mf.reserved = 0;
		mf.resrcflags = 0;
		iop_post(iop, (u_int32_t *)&mf);
		DELAY(5000);

		/*
		 * Register to receive events from the device.
		 */
		if (iop_util_eventreg(iop, &sc->sc_ii_evt, 0xffffffff))
			printf("%s: unable to register for events\n",
			    sc->sc_dv.dv_xname);

		/*
		 * Trigger periodic parameter group retrievals.
		 */
		s = splbio();
		sc->sc_flags |= (IOPL_MEDIA_CHANGE | IOPL_INITTED);
		splx(s);

		callout_init(&sc->sc_pg_callout, 0);

		sc->sc_next_pg = -1;
		iopl_tick_sched(sc);
	}

	/*
	 * Enable or disable hardware checksumming.
	 */
	s = splbio();
#ifdef IOPL_ENABLE_BATCHING
	sc->sc_tx_tcw = I2O_LAN_TCW_REPLY_BATCH;
#else
	sc->sc_tx_tcw = I2O_LAN_TCW_REPLY_IMMEDIATELY;
#endif
	sc->sc_rx_csumflgs = 0;
	rxmode = 0;
	txmode = 0;

	ifcap = ifp->if_capenable;
	if ((ifcap & IFCAP_CSUM_IPv4_Tx) != 0) {
		sc->sc_tx_tcw |= I2O_LAN_TCW_CKSUM_NETWORK;
		txmode |= I2O_LAN_MODES_IPV4_CHECKSUM;
	}
	if ((ifcap & IFCAP_CSUM_IPv4_Rx) != 0) {
		sc->sc_rx_csumflgs |= M_CSUM_IPv4;
		rxmode |= I2O_LAN_MODES_IPV4_CHECKSUM;
	}

	if ((ifcap & IFCAP_CSUM_TCPv4_Tx) != 0) {
		sc->sc_tx_tcw |= I2O_LAN_TCW_CKSUM_TRANSPORT;
		txmode |= I2O_LAN_MODES_TCP_CHECKSUM;
	}
	if ((ifcap & IFCAP_CSUM_TCPv4_Rx) != 0) {
		sc->sc_rx_csumflgs |= M_CSUM_TCPv4;
		rxmode |= I2O_LAN_MODES_TCP_CHECKSUM;
	}

	if ((ifcap & IFCAP_CSUM_UDPv4_Tx) != 0) {
		sc->sc_tx_tcw |= I2O_LAN_TCW_CKSUM_TRANSPORT;
		txmode |= I2O_LAN_MODES_UDP_CHECKSUM;
	}
	if ((ifcap & IFCAP_CSUM_UDPv4_Rx) != 0) {
		sc->sc_rx_csumflgs |= M_CSUM_UDPv4;
		rxmode |= I2O_LAN_MODES_TCP_CHECKSUM;
	}

	splx(s);

	/* We always want a copy of the checksum. */
	rxmode |= I2O_LAN_MODES_FCS_RECEPTION;
	rxmode = htole32(rxmode);
	txmode = htole32(txmode);

	rv = iop_field_set(iop, sc->sc_tid, I2O_PARAM_LAN_OPERATION,
	    &txmode, sizeof(txmode), I2O_PARAM_LAN_OPERATION_txmodesenable);
	if (rv == 0)
		rv = iop_field_set(iop, sc->sc_tid, I2O_PARAM_LAN_OPERATION,
		    &txmode, sizeof(txmode),
		    I2O_PARAM_LAN_OPERATION_rxmodesenable);
	if (rv != 0)
		return (rv);

	/*
	 * Try to set the active MAC address.
	 */
	memset(hwaddr, 0, sizeof(hwaddr));
	memcpy(hwaddr, LLADDR(ifp->if_sadl), ifp->if_addrlen);
	iop_field_set(iop, sc->sc_tid, I2O_PARAM_LAN_MAC_ADDRESS,
	    hwaddr, sizeof(hwaddr), I2O_PARAM_LAN_MAC_ADDRESS_localaddr);

	ifp->if_flags = (ifp->if_flags | IFF_RUNNING) & ~IFF_OACTIVE;

	/*
	 * Program the receive filter.
	 */
	switch (sc->sc_mtype) {
	case I2O_LAN_TYPE_ETHERNET:
	case I2O_LAN_TYPE_100BASEVG:
	case I2O_LAN_TYPE_FDDI:
		iopl_filter_ether(sc);
		break;
	}

	/*
	 * Post any free receive buckets to the interface.
	 */
	s = splbio();
	iopl_rx_post(sc);
	splx(s);
	return (0);
}

/*
 * Stop the interface.
 */
static void
iopl_stop(struct ifnet *ifp, int disable)
{
	struct i2o_lan_suspend mf;
	struct iopl_softc *sc;
	struct iop_softc *iop;
	int flg, s;

	sc = ifp->if_softc;
	iop = (struct iop_softc *)sc->sc_dv.dv_xname;

	s = splbio();
	flg = sc->sc_flags;
	splx(s);

	if ((flg & IOPL_INITTED) != 0) {
		/*
		 * Block reception of events from the device.
		 */
		if (iop_util_eventreg(iop, &sc->sc_ii_evt, 0))
			printf("%s: unable to register for events\n",
			    sc->sc_dv.dv_xname);

		/*
		 * Stop parameter group retrival.
		 */
		callout_stop(&sc->sc_pg_callout);

		s = splbio();
		sc->sc_flags &= ~IOPL_INITTED;
		splx(s);
	}

	/*
	 * If requested, suspend the interface.
	 */
	if (disable) {
		mf.msgflags = I2O_MSGFLAGS(i2o_lan_suspend);
		mf.msgfunc = I2O_MSGFUNC(I2O_TID_IOP, I2O_LAN_SUSPEND);
		mf.msgictx = sc->sc_ii_null.ii_ictx;
		mf.reserved = 0;
		mf.resrcflags = I2O_LAN_RESRC_RETURN_BUCKETS |
		    I2O_LAN_RESRC_RETURN_XMITS;
		iop_post(iop, (u_int32_t *)&mf);
	}

	ifp->if_timer = 0;
	ifp->if_flags &= ~IFF_RUNNING;
}

/*
 * Start output on the interface.
 */
static void
iopl_start(struct ifnet *ifp)
{
	struct iopl_softc *sc;
	struct iop_softc *iop;
	struct i2o_lan_packet_send *mf;
	struct iopl_tx *tx;
	struct mbuf *m;
	bus_dmamap_t dm;
	bus_dma_segment_t *ds;
	bus_addr_t saddr, eaddr;
	u_int32_t mb[IOP_MAX_MSG_SIZE / sizeof(u_int32_t)];
	u_int32_t *p = NULL, *lp = NULL; /* XXX */
	u_int rv, i, slen, tlen, size;
	int frameleft, nxmits;
	SLIST_HEAD(,iopl_tx) pending;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	sc = (struct iopl_softc *)ifp->if_softc;
	iop = (struct iop_softc *)device_parent(&sc->sc_dv);
	mf = (struct i2o_lan_packet_send *)mb;
	frameleft = -1;
	nxmits = 0;
	SLIST_INIT(&pending);

	/*
	 * Set static fields in the message frame header.
	 */
	mf->msgfunc = I2O_MSGFUNC(I2O_TID_IOP, I2O_LAN_PACKET_SEND);
	mf->msgictx = sc->sc_ii_rx.ii_ictx;
	mf->tcw = sc->sc_tx_tcw;

	for (;;) {
		/*
		 * Grab a packet to send and a transmit descriptor for it.
		 * If we don't get both, then bail out.
		 */
		if ((tx = SLIST_FIRST(&sc->sc_tx_free)) == NULL) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}
		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;

		/*
		 * Load the mbuf into the descriptor's DMA map.  If we fail,
		 * drop the packet on the floor and get out.
		 */
		dm = tx->tx_dmamap;
		rv = bus_dmamap_load_mbuf(sc->sc_dmat, dm, m,
		    BUS_DMA_WRITE | BUS_DMA_NOWAIT);
		if (!rv) {
			printf("%s: unable to load TX buffer; error = %d\n",
			    sc->sc_dv.dv_xname, rv);
			m_freem(m);
			break;
		}
		bus_dmamap_sync(sc->sc_dmat, dm, 0, dm->dm_mapsize,
		    BUS_DMASYNC_PREWRITE);

		/*
		 * Now that the transmit descriptor has resources allocated
		 * to it, remove it from the free list and add it to the
		 * pending list.
		 */
		SLIST_REMOVE_HEAD(&sc->sc_tx_free, tx_chain);
		SLIST_INSERT_HEAD(&pending, tx, tx_chain);
		sc->sc_tx_freecnt--;

		/*
		 * Determine whether we can cram this transmit into an
		 * existing message frame (if any), or whether we need to
		 * send a new one.
		 */
#if IOPL_BATCHING_ENABLED
		if (nxmits >= sc->sc_tx_maxreq)
			size = UINT_MAX;
		else
			size = sc->sc_tx_ohead + sc->sc_tx_maxsegs;
#else
		size = UINT_MAX;
#endif

		if (size > frameleft) {
			if (frameleft >= 0) {
				/*
				 * We have an old message frame to flush.
				 * Clear the pending list if we send it
				 * successfully.
				 */
				*lp |= I2O_SGL_END;
				if (iop_post(iop, mb) == 0)
					SLIST_INIT(&pending);
			}

			/*
			 * Prepare a new message frame.
			 */
			mf->msgflags = I2O_MSGFLAGS(i2o_lan_packet_send);
			p = (u_int32_t *)(mf + 1);
			frameleft = (sizeof(mb) - sizeof(*mf)) >> 2;
			nxmits = 0;
		}

		/*
		 * Fill the scatter/gather list.  The interface may have
		 * requested that the destination address be passed as part
		 * of the buffer context.
		 */
		lp = p;

		if (sc->sc_tx_ohead > 2) {
			*p++ = dm->dm_mapsize | I2O_SGL_PAGE_LIST |
			    I2O_SGL_BC_96BIT | I2O_SGL_END_BUFFER;
			*p++ = tx->tx_ident;
			(*sc->sc_munge)(m, (u_int8_t *)p);
			p += 2;
		} else {
			*p++ = dm->dm_mapsize | I2O_SGL_PAGE_LIST |
			    I2O_SGL_BC_32BIT | I2O_SGL_END_BUFFER;
			*p++ = tx->tx_ident;
		}

		for (i = dm->dm_nsegs, ds = dm->dm_segs; i > 0; i--, ds++) {
			slen = ds->ds_len;
			saddr = ds->ds_addr;

			/* XXX This should be done with a bus_space flag. */
			while (slen > 0) {
				eaddr = (saddr + PAGE_SIZE) & ~(PAGE_SIZE - 1);
				tlen = min(eaddr - saddr, slen);
				slen -= tlen;
				*p++ = le32toh(saddr);
				saddr = eaddr;
			}
		}

		frameleft -= (p - lp);
		nxmits++;

#if NBPFILTER > 0
		/*
		 * If BPF is enabled on this interface, feed it a copy of
		 * the packet.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif
	}

	/*
	 * Flush any waiting message frame.  If it's sent successfully, then
	 * return straight away.
	 */
	if (frameleft >= 0) {
		*lp |= I2O_SGL_END;
		if (iop_post(iop, mb) == 0)
			return;
	}

	/*
	 * Free resources for transmits that failed.
	 */
	while ((tx = SLIST_FIRST(&pending)) != NULL) {
		SLIST_REMOVE_HEAD(&pending, tx_chain);
		SLIST_INSERT_HEAD(&sc->sc_tx_free, tx, tx_chain);
		sc->sc_tx_freecnt++;
		bus_dmamap_sync(sc->sc_dmat, tx->tx_dmamap, 0,
		     tx->tx_dmamap->dm_mapsize, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, tx->tx_dmamap);
		m_freem(tx->tx_mbuf);
	}
}

/*
 * Munge an Ethernet address into buffer context.
 */
static void
iopl_munge_ether(struct mbuf *m, u_int8_t *dp)
{
	struct ether_header *eh;
	u_int8_t *sp;
	int i;

	eh = mtod(m, struct ether_header *);
	sp = (u_int8_t *)eh->ether_dhost;
	for (i = ETHER_ADDR_LEN; i > 0; i--)
		*dp++ = *sp++;
	*dp++ = 0;
	*dp++ = 0;
}

/*
 * Munge an FDDI address into buffer context.
 */
static void
iopl_munge_fddi(struct mbuf *m, u_int8_t *dp)
{
	struct fddi_header *fh;
	u_int8_t *sp;
	int i;

	fh = mtod(m, struct fddi_header *);
	sp = (u_int8_t *)fh->fddi_dhost;
	for (i = 6; i > 0; i--)
		*dp++ = *sp++;
	*dp++ = 0;
	*dp++ = 0;
}

/*
 * Program the receive filter for an Ethernet interface.
 */
static int
iopl_filter_ether(struct iopl_softc *sc)
{
	struct ifnet *ifp;
	struct ethercom *ec;
	struct ether_multi *enm;
	u_int64_t *tbl;
	int i, rv, size;
	struct ether_multistep step;

	ec = &sc->sc_if.sci_ec;
	ifp = &ec->ec_if;

	/*
	 * If there are more multicast addresses than will fit into the
	 * filter table, or we fail to allocate memory for the table, then
	 * enable reception of all multicast packets.
	 */
	if (ec->ec_multicnt > sc->sc_mcast_max)
		goto allmulti;

	size = sizeof(*tbl) * sc->sc_mcast_max;
	if ((tbl = malloc(size, M_DEVBUF, M_WAITOK|M_ZERO)) == NULL)
		goto allmulti;

	ETHER_FIRST_MULTI(step, ec, enm)
	for (i = 0; enm != NULL; i++) {
		/*
		 * For the moment, if a range of multicast addresses was
		 * specified, then just accept all multicast packets.
		 */
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			free(tbl, M_DEVBUF);
			goto allmulti;
		}

		/*
		 * Add the address to the table.
		 */
		memset(&tbl[i], 0, sizeof(tbl[i]));
		memcpy(&tbl[i], enm->enm_addrlo, ETHER_ADDR_LEN);

		ETHER_NEXT_MULTI(step, enm);
	}

	sc->sc_mcast_cnt = i;
	ifp->if_flags &= ~IFF_ALLMULTI;
 	rv = iopl_filter_generic(sc, tbl);
 	free(tbl, M_DEVBUF);
 	return (0);

 allmulti:
	sc->sc_mcast_cnt = 0;
	ifp->if_flags |= IFF_ALLMULTI;
	return (iopl_filter_generic(sc, NULL));
}

/*
 * Generic receive filter programming.
 */
static int
iopl_filter_generic(struct iopl_softc *sc, u_int64_t *tbl)
{
	struct iop_softc *iop;
	struct ifnet *ifp;
	int i, rv;
	u_int32_t tmp1;

	ifp = &sc->sc_if.sci_if;
	iop = (struct iop_softc *)device_parent(&sc->sc_dv);

	/*
	 * Clear out the existing multicast table and set in the new one, if
	 * any.
	 */
	if (sc->sc_mcast_max != 0) {
		iop_table_clear(iop, sc->sc_tid,
		    I2O_PARAM_LAN_MCAST_MAC_ADDRESS);

		for (i = 0; i < sc->sc_mcast_cnt; i++) {
			rv = iop_table_add_row(iop, sc->sc_tid,
			    I2O_PARAM_LAN_MCAST_MAC_ADDRESS,
			    &tbl[i], sizeof(tbl[i]), i);
			if (rv != 0) {
				ifp->if_flags |= IFF_ALLMULTI;
				break;
			}
		}
	}

	/*
	 * Set the filter mask.
	 */
	if ((ifp->if_flags & IFF_PROMISC) != 0)
		tmp1 = I2O_LAN_FILTERMASK_PROMISC_ENABLE;
	else  {
		if ((ifp->if_flags & IFF_ALLMULTI) != 0)
			tmp1 = I2O_LAN_FILTERMASK_PROMISC_MCAST_ENABLE;
		else
			tmp1 = 0;

		if ((ifp->if_flags & IFF_BROADCAST) == 0)
			tmp1 |= I2O_LAN_FILTERMASK_BROADCAST_DISABLE;
	}
	tmp1 = htole32(tmp1);

	return (iop_field_set(iop, sc->sc_tid, I2O_PARAM_LAN_MAC_ADDRESS,
	    &tmp1, sizeof(tmp1), I2O_PARAM_LAN_MAC_ADDRESS_filtermask));
}

/*
 * Handle control operations.
 */
static int
iopl_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct iopl_softc *sc;
	struct ifaddr *ifa;
	struct ifreq *ifr;
	int s, rv;

	ifr = (struct ifreq *)data;
	sc = ifp->if_softc;
	s = splnet();
	rv = 0;

	switch (cmd) {
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		rv = ifmedia_ioctl(ifp, ifr, &sc->sc_ifmedia, cmd);
		goto out;
	}

	switch (sc->sc_mtype) {
	case I2O_LAN_TYPE_ETHERNET:
	case I2O_LAN_TYPE_100BASEVG:
		rv = ether_ioctl(ifp, cmd, data);
		if (rv == ENETRESET) {
			/*
			 * Flags and/or multicast list has changed; need to
			 * set the hardware filter accordingly.
			 */
			if (ifp->if_flags & IFF_RUNNING)
				rv = iopl_filter_ether(sc);
			else
				rv = 0;
		}
		break;

	case I2O_LAN_TYPE_FDDI:
		/*
		 * XXX This should be shared.
		 */
		switch (cmd) {
		case SIOCSIFADDR:
			ifa = (struct ifaddr *)data;
			ifp->if_flags |= IFF_UP;

			switch (ifa->ifa_addr->sa_family) {
#if defined(INET)
			case AF_INET:
				iopl_init(ifp);
				arp_ifinit(ifp, ifa);
				break;
#endif /* INET */

			default:
				iopl_init(ifp);
				break;
			}
			break;

		case SIOCGIFADDR:
			ifr = (struct ifreq *)data;
			memcpy(((struct sockaddr *)&ifr->ifr_data)->sa_data,
			    LLADDR(ifp->if_sadl), 6);
			break;

		case SIOCSIFFLAGS:
			iopl_init(ifp);
			break;

		case SIOCADDMULTI:
		case SIOCDELMULTI:
			ifr = (struct ifreq *)data;
			if (cmd == SIOCADDMULTI)
				rv = ether_addmulti(ifr, &sc->sc_if.sci_ec);
			else
				rv = ether_delmulti(ifr, &sc->sc_if.sci_ec);
			if (rv == ENETRESET) {
				if (ifp->if_flags & IFF_RUNNING)
					rv = iopl_filter_ether(sc);
				else
					rv = 0;
			}
			break;

		case SIOCSIFMTU:
			ifr = (struct ifreq *)data;
			if (ifr->ifr_mtu > FDDIMTU) {
				rv = EINVAL;
				break;
			}
			ifp->if_mtu = ifr->ifr_mtu;
			break;

		default:
			rv = ENOTTY;
			break;
		}
	}

 out:
	splx(s);
	return (rv);
}

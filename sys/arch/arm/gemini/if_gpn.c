/* $NetBSD: if_gpn.c,v 1.4.12.1 2017/12/03 11:35:53 jdolecek Exp $ */
/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>
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

#include <sys/cdefs.h>

#include "opt_gemini.h"

__KERNEL_RCSID(0, "$NetBSD: if_gpn.c,v 1.4.12.1 2017/12/03 11:35:53 jdolecek Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/mbuf.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <net/if_dl.h>

#include <net/bpf.h>

#include <sys/bus.h>

#include <arm/gemini/gemini_var.h>
#include <arm/gemini/gemini_ipm.h>

#define	GPN_MOF		0x00	/* Middle Of Frame */
#define	GPN_SOF		0x01	/* Start of Frame */
#define	GPN_EOF		0x02	/* End of Frame */
#define	GPN_FRAME	0x03	/* Complete Frame */

#define	GPN_IFUP	0x05	/* partner is up */
#define	GPN_IFDOWN	0x06	/* partner is down */

#define	GPN_ACK0	0x10	/* placeholder */
#define	GPN_ACK1	0x11	/* Ack 1 descriptor */
#define	GPN_ACK2	0x12	/* Ack 2 descriptors */
#define	GPN_ACK3	0x13	/* Ack 3 descriptors */
#define	GPN_ACK4	0x14	/* Ack 4 descriptors */
#define	GPN_ACK5	0x15	/* Ack 5 descriptors */
#define	GPN_ACK6	0x16	/* Ack 6 descriptors */
#define	GPN_ACK7	0x17	/* Ack 7 descriptors */
#define	GPN_ACK8	0x18	/* Ack 8 descriptors */
#define	GPN_ACK9	0x19	/* Ack 9 descriptors */
#define	GPN_ACK10	0x1a	/* Ack 10 descriptors */
#define	GPN_ACK11	0x1b	/* Ack 11 descriptors */
#define	GPN_ACK12	0x1c	/* Ack 12 descriptors */
#define	GPN_ACK13	0x1d	/* Ack 13 descriptors */
#define	GPN_ACK14	0x1e	/* Ack 14 descriptors */

typedef struct {
	uint8_t gd_tag;
	uint8_t gd_subtype;
	uint8_t gd_txid;
	uint8_t gd_pktlen64;
	uint16_t gd_len1;
	uint16_t gd_len2;
	uint32_t gd_addr1;
	uint32_t gd_addr2;
} ipm_gpn_desc_t;

typedef struct {
	uint8_t agd_tag;
	uint8_t agd_subtype;
	uint8_t agd_txids[14];
} ipm_gpn_ack_desc_t;

#define	MAX_TXACTIVE	60

struct gpn_txinfo {
	struct mbuf *ti_mbuf;
	bus_dmamap_t ti_map;
};

struct gpn_softc {
	device_t sc_dev;
	bus_dma_tag_t sc_dmat;
	struct ifmedia sc_im;
	struct ethercom sc_ec;
#define	sc_if sc_ec.ec_if
	size_t sc_free;
	size_t sc_txactive;
	void *sc_ih;
	ipm_gpn_ack_desc_t sc_ack_desc;
	struct mbuf *sc_rxmbuf;
	struct gpn_txinfo sc_txinfo[MAX_TXACTIVE];
	uint8_t sc_lastid;
	bool sc_remoteup;		/* remote side up? */ 
};

CTASSERT((GPN_SOF | GPN_EOF) == GPN_FRAME);
CTASSERT((GPN_SOF & GPN_EOF) == 0);

extern struct cfdriver gpn_cd;

static void gpn_ifstart(struct ifnet *);

#ifdef GPNDEBUG
static uint32_t
m_crc32_le(struct mbuf *m)
{
	static const uint32_t crctab[] = {
		0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
		0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
		0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
		0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
	};
	uint32_t crc;
	size_t i;

	crc = 0xffffffffU;	/* initial value */

	for (; m; m = m->m_next) {
		for (i = 0; i < m->m_len; i++) {
			crc ^= m->m_data[i];
			crc = (crc >> 4) ^ crctab[crc & 0xf];
			crc = (crc >> 4) ^ crctab[crc & 0xf];
		}
	}

	return (crc);
}
#endif

static void
gpn_free_dmamaps(struct gpn_softc *sc)
{
	struct gpn_txinfo *ti = sc->sc_txinfo;
	struct gpn_txinfo * const end_ti = ti + __arraycount(sc->sc_txinfo);

	for (; ti < end_ti; ti++) {
		if (ti->ti_map == NULL)
			continue;
		bus_dmamap_destroy(sc->sc_dmat, ti->ti_map);
		ti->ti_map = NULL;
	}
}

static int
gpn_alloc_dmamaps(struct gpn_softc *sc)
{
	struct gpn_txinfo *ti = sc->sc_txinfo;
	struct gpn_txinfo * const end_ti = ti + __arraycount(sc->sc_txinfo);
	int error;

	for (error = 0; ti < end_ti; ti++) {
		if (ti->ti_map != NULL)
			continue;
		error = bus_dmamap_create(sc->sc_dmat,
		    10000, 2, 8192, 0, 
		    BUS_DMA_ALLOCNOW|BUS_DMA_WAITOK,
		    &ti->ti_map);
		if (error)
			break;
	}

	if (error)
		gpn_free_dmamaps(sc);

	return error;
}

static bool
gpn_add_data(struct gpn_softc *sc, bus_addr_t addr, bus_size_t len)
{
	struct mbuf *m, *m0;
	size_t space;

	m = sc->sc_rxmbuf;
	KASSERT(m != NULL);

	m->m_pkthdr.len += len;

	while (m->m_next != NULL)
		m = m->m_next;

	KASSERT(len > 0);
	space = M_TRAILINGSPACE(m);
	for (;;) {
		if (space > 0) {
			if (len < space)
				space = len;
			gemini_ipm_copyin(mtod(m, uint8_t *) + m->m_len, addr,
			    space);
			len -= space;
			m->m_len += space;
			if (len == 0)
				return true;
			addr += space;
		}
		MGET(m0, M_DONTWAIT, MT_DATA);
		if (m0 == NULL)
			break;
		space = MLEN;
		if (len > space) {
			MCLGET(m0, M_DONTWAIT);
			if (m0->m_flags & M_EXT)
				space = MCLBYTES;
		}
		m->m_len = 0;
		m->m_next = m0;
		m = m0;
	}
	return false;
}

static void
gpn_ack_txid(struct gpn_softc *sc, unsigned int txid)
{
	ipm_gpn_ack_desc_t * const agd = &sc->sc_ack_desc;
	agd->agd_txids[agd->agd_subtype] = txid;
	if (++agd->agd_subtype == __arraycount(agd->agd_txids)) {
		agd->agd_subtype += GPN_ACK0;
		sc->sc_free--;
		gemini_ipm_produce(agd, 1);
		agd->agd_subtype = 0;
	}
}

static void
gpn_process_data(struct gpn_softc *sc, const ipm_gpn_desc_t *gd)
{
	struct ifnet * const ifp = &sc->sc_if;
	size_t pktlen = gd->gd_pktlen64 * 64;
	unsigned int subtype = gd->gd_subtype;
	bool ok;

	if ((subtype & GPN_SOF) == 0 && sc->sc_rxmbuf == NULL) {
		ifp->if_ierrors++;
		goto out;
	}

	if ((subtype & GPN_SOF) && sc->sc_rxmbuf != NULL) {
		ifp->if_ierrors++;
		m_freem(sc->sc_rxmbuf);
		sc->sc_rxmbuf = NULL;
	}

	if (sc->sc_rxmbuf == NULL) {
		struct mbuf *m;
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == NULL) {
			ifp->if_ierrors++;
			goto out;
		}
		if (pktlen > MHLEN - 2) {
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				ifp->if_ierrors++;
				m_free(m);
				goto out;
			}
		}
		m->m_data += 2;	/* makes ethernet payload 32bit aligned */
		m->m_len = 0;
		m->m_pkthdr.len = 0;
		sc->sc_rxmbuf = m;
	}

	ok = gpn_add_data(sc, gd->gd_addr1, gd->gd_len1);
	if (ok && gd->gd_addr2 && gd->gd_len2)
		ok = gpn_add_data(sc, gd->gd_addr2, gd->gd_len2);
	if (!ok) {
		ifp->if_ierrors++;
		m_freem(sc->sc_rxmbuf);
		sc->sc_rxmbuf = NULL;
		goto out;
	}

	if (subtype & GPN_EOF) {
		struct mbuf *m;
		m = sc->sc_rxmbuf;
		sc->sc_rxmbuf = NULL;
		m_set_rcvif(m, ifp);
		KASSERT(((m->m_pkthdr.len + 63) >> 6) == gd->gd_pktlen64);
		ifp->if_ibytes += m->m_pkthdr.len;
#ifdef GPNDEBUG
		printf("%s: rx len=%d crc=%#x\n", ifp->if_xname,
		    m->m_pkthdr.len, m_crc32_le(m));
#endif
		if_percpuq_enqueue(ifp->if_percpuq, m);
	}

out:
	gpn_ack_txid(sc, gd->gd_txid);
}

static void
gpn_free_txid(struct gpn_softc *sc, size_t txid)
{
	struct gpn_txinfo * const ti = sc->sc_txinfo + txid;

	KASSERT(txid < MAX_TXACTIVE);

	if (ti->ti_mbuf == NULL)
		return;

	bus_dmamap_sync(sc->sc_dmat, ti->ti_map,
	    0, ti->ti_mbuf->m_len, BUS_DMASYNC_POSTREAD);
	bus_dmamap_unload(sc->sc_dmat, ti->ti_map);
	m_freem(ti->ti_mbuf);
	ti->ti_mbuf = NULL;
	sc->sc_txactive--;
	KASSERT(sc->sc_txactive < MAX_TXACTIVE);
	if (sc->sc_if.if_flags & IFF_OACTIVE) {
		sc->sc_if.if_flags &= ~IFF_OACTIVE;
		gpn_ifstart(&sc->sc_if);
	}
	
}

static void
gpn_ipm_rebate(void *arg, size_t count)
{
	struct gpn_softc * const sc = arg;
	int s;

	s = splnet();
	sc->sc_free += count;

	sc->sc_if.if_flags &= ~IFF_OACTIVE;
	gpn_ifstart(&sc->sc_if);
	splx(s);
}

static void
gpn_ifstart(struct ifnet *ifp)
{
	struct gpn_softc * const sc = ifp->if_softc;

	for (;;) {
		struct mbuf *m, *m0;
		ipm_gpn_desc_t gd;
		ipm_gpn_desc_t *last_gd;
		size_t count;

		if (sc->sc_free == 0) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		IF_DEQUEUE(&ifp->if_snd, m);
		if (!m)
			break;

		if ((ifp->if_flags & IFF_UP) == 0) {
			m_freem(m);
			continue;
		}

		/*
		 * Make sure to send any pending acks first.
		 */
		if (sc->sc_ack_desc.agd_subtype) {
			sc->sc_free--;
			sc->sc_ack_desc.agd_subtype += GPN_ACK0;
			gemini_ipm_produce(&sc->sc_ack_desc, 1);
			sc->sc_ack_desc.agd_subtype = 0;
		}

		/*
		 * Let's find out how many mbufs we are using.
		 */
		for (m0 = m, count = 0; m0; m0 = m0->m_next) {
			if (m0->m_len == 0)
				continue;
			count++;
		}

		/*
		 * Make sure there is always enough room.
		 */
		if (sc->sc_free < count
		    || sc->sc_txactive + count > MAX_TXACTIVE) {
			IF_PREPEND(&ifp->if_snd, m);
			ifp->if_flags |= IFF_OACTIVE;
			return;
		}

		bpf_mtap(ifp, m);
#ifdef GPNDEBUG
		printf("%s: tx len=%d crc=%#x\n", ifp->if_xname,
		    m->m_pkthdr.len, m_crc32_le(m));
#endif

		last_gd = NULL;
		gd.gd_tag = IPM_TAG_GPN;
		gd.gd_subtype = GPN_SOF;
		gd.gd_pktlen64 = (m->m_pkthdr.len + 63) >> 6;
		for (; m != NULL; m = m0) {
			struct gpn_txinfo *ti;
			bus_dmamap_t map;
			size_t id;
			int error;

			m0 = m->m_next;
			m->m_next = NULL;
			if (m->m_len == 0) {
				m_free(m);
				continue;
			}
			if (last_gd) {
				sc->sc_txactive++;
				sc->sc_free--;
				gemini_ipm_produce(last_gd, 1);
				last_gd = NULL;
				gd.gd_subtype = GPN_MOF;
			}
			for (id = sc->sc_lastid;
			     sc->sc_txinfo[id].ti_mbuf != NULL;) {
				if (++id == __arraycount(sc->sc_txinfo))
					id = 0;
			}
			KASSERT(id < MAX_TXACTIVE);
			ti = sc->sc_txinfo + id;
			map = ti->ti_map;
			error = bus_dmamap_load(sc->sc_dmat, map,
			    mtod(m, void *), m->m_len, NULL,
			    BUS_DMA_READ|BUS_DMA_NOWAIT);
			if (error) {
				ifp->if_oerrors++;
				m_freem(m);
				break;
			}
			bus_dmamap_sync(sc->sc_dmat, map, 0,
			    m->m_len, BUS_DMASYNC_PREREAD);
			KASSERT(map->dm_nsegs > 0);
			KASSERT(map->dm_nsegs <= 2);
			KASSERT(map->dm_segs[0].ds_addr != 0);
			gd.gd_len1 = map->dm_segs[0].ds_len;
			gd.gd_addr1 = map->dm_segs[0].ds_addr;
			if (map->dm_nsegs == 1) {
				gd.gd_len2 = 0;
				gd.gd_addr2 = 0;
			} else {
				KASSERT(map->dm_segs[0].ds_addr != 0);
				gd.gd_len2 = map->dm_segs[1].ds_len;
				gd.gd_addr2 = map->dm_segs[1].ds_addr;
			}

			gd.gd_txid = id;
			ti->ti_mbuf = m;
			last_gd = &gd;
			ifp->if_obytes += m->m_len;
		}
		ifp->if_opackets++;

		/*
		 * XXX XXX 'last_gd' could be NULL
		 */
		last_gd->gd_subtype |= GPN_EOF;

		sc->sc_txactive++;
		sc->sc_free--;
		gemini_ipm_produce(last_gd, 1);
	}
}

static void
gpn_ipm_ifup(struct gpn_softc *sc)
{
	sc->sc_remoteup = true;
	if (sc->sc_if.if_flags & IFF_UP)
		ifmedia_set(&sc->sc_im, IFM_ETHER|IFM_1000_T|IFM_FDX);
}

static void
gpn_ipm_ifdown(struct gpn_softc *sc)
{
	struct gpn_txinfo *ti = sc->sc_txinfo;
	struct gpn_txinfo * const end_ti = ti + __arraycount(sc->sc_txinfo);

	if (sc->sc_rxmbuf) {
		m_freem(sc->sc_rxmbuf);
		sc->sc_rxmbuf = NULL;
	}

	IF_PURGE(&sc->sc_if.if_snd);

	for (; ti < end_ti; ti++) {
		if (ti->ti_mbuf == NULL)
			continue;
		bus_dmamap_sync(sc->sc_dmat, ti->ti_map,
		    0, ti->ti_mbuf->m_len, BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(sc->sc_dmat, ti->ti_map);
		m_freem(ti->ti_mbuf);
		ti->ti_mbuf = NULL;
	}
	sc->sc_lastid = 0;
	ifmedia_set(&sc->sc_im, IFM_ETHER|IFM_NONE);
	sc->sc_remoteup = false;
}

static void
gpn_ipm_handler(void *arg, const void *desc)
{
	struct gpn_softc * const sc = arg;
	const ipm_gpn_desc_t * const gd = desc;
	const ipm_gpn_ack_desc_t * const agd = desc;
	int s;

	s = splnet();

	switch (gd->gd_subtype) {
	case GPN_ACK14: gpn_free_txid(sc, agd->agd_txids[13]); /* FALLTHROUGH */
	case GPN_ACK13: gpn_free_txid(sc, agd->agd_txids[12]); /* FALLTHROUGH */
	case GPN_ACK12: gpn_free_txid(sc, agd->agd_txids[11]); /* FALLTHROUGH */
	case GPN_ACK11: gpn_free_txid(sc, agd->agd_txids[10]); /* FALLTHROUGH */
	case GPN_ACK10: gpn_free_txid(sc, agd->agd_txids[9]); /* FALLTHROUGH */
	case GPN_ACK9: gpn_free_txid(sc, agd->agd_txids[8]); /* FALLTHROUGH */
	case GPN_ACK8: gpn_free_txid(sc, agd->agd_txids[7]); /* FALLTHROUGH */
	case GPN_ACK7: gpn_free_txid(sc, agd->agd_txids[6]); /* FALLTHROUGH */
	case GPN_ACK6: gpn_free_txid(sc, agd->agd_txids[5]); /* FALLTHROUGH */
	case GPN_ACK5: gpn_free_txid(sc, agd->agd_txids[4]); /* FALLTHROUGH */
	case GPN_ACK4: gpn_free_txid(sc, agd->agd_txids[3]); /* FALLTHROUGH */
	case GPN_ACK3: gpn_free_txid(sc, agd->agd_txids[2]); /* FALLTHROUGH */
	case GPN_ACK2: gpn_free_txid(sc, agd->agd_txids[1]); /* FALLTHROUGH */
	case GPN_ACK1: gpn_free_txid(sc, agd->agd_txids[0]); break;
	case GPN_MOF:
	case GPN_SOF:
	case GPN_FRAME:
	case GPN_EOF:
		gpn_process_data(sc, gd);
		break;
	case GPN_IFUP:
		gpn_ipm_ifup(sc);
		break;
	case GPN_IFDOWN:
		gpn_ipm_ifdown(sc);
		break;
	default:
		KASSERT(0);
	}

	splx(s);
}

static int
gpn_ifinit(struct ifnet *ifp)
{
	struct gpn_softc * const sc = ifp->if_softc;
	ipm_gpn_desc_t gd;
	int error;

	error = gpn_alloc_dmamaps(sc);
	if (error)
		return error;

	memset(&gd, 0, sizeof(gd));
	gd.gd_tag = IPM_TAG_GPN;
	gd.gd_subtype = GPN_IFUP;
	KASSERT(sc->sc_free > 0);
	sc->sc_free--;
	gemini_ipm_produce(&gd, 1);

	if (sc->sc_remoteup)
		ifmedia_set(&sc->sc_im, IFM_ETHER|IFM_1000_T|IFM_FDX);

	ifp->if_flags |= IFF_RUNNING;

	return error;
}

static void
gpn_ifstop(struct ifnet *ifp, int disable)
{
	struct gpn_softc * const sc = ifp->if_softc;
	ipm_gpn_desc_t gd;

	memset(&gd, 0, sizeof(gd));
	gd.gd_tag = IPM_TAG_GPN;
	gd.gd_subtype = GPN_IFDOWN;
	KASSERT(sc->sc_free > 0);
	sc->sc_free--;
	gemini_ipm_produce(&gd, 1);
	ifp->if_flags &= ~IFF_RUNNING;
	gpn_ipm_ifdown(sc);

	if (disable) {
		gpn_free_dmamaps(sc);
	}
}

static int
gpn_ifioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct gpn_softc * const sc = ifp->if_softc;
	struct ifreq * const ifr = data;
	struct ifaliasreq * const ifra = data;
	int s, error;

	s = splnet();

	switch (cmd) {
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_im, cmd);
		break;
	case SIOCSIFPHYADDR: {
		const struct sockaddr_dl *sdl = satosdl(&ifra->ifra_addr);
                
		if (sdl->sdl_family != AF_LINK) {
			error = EINVAL;
			break;
		}
                
		if_set_sadl(ifp, CLLADDR(sdl), ETHER_ADDR_LEN, false);
		error = 0;
		break;
	}
	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error == ENETRESET)
			error = 0;
		break;
	}

	splx(s);
	return error;
}

static int
gpn_mediachange(struct ifnet *ifp)
{
	return 0;
}

static void
gpn_mediastatus(struct ifnet *ifp, struct ifmediareq *imr)
{
	struct gpn_softc * const sc = ifp->if_softc;
	imr->ifm_active = sc->sc_im.ifm_cur->ifm_media;
}

static int
gpn_match(device_t parent, cfdata_t cf, void *aux)
{
	return strcmp(gpn_cd.cd_name, aux) == 0;
}

static void
gpn_attach(device_t parent, device_t self, void *aux)
{
	struct gpn_softc * const sc = device_private(self);
	struct ifnet * const ifp = &sc->sc_if;
	char enaddr[6];
	
	enaddr[0] = 2;
	enaddr[1] = 0;
	enaddr[2] = 0;
	enaddr[3] = 0;
	enaddr[4] = 0;
#ifdef GEMINI_MASTER
	enaddr[5] = 0;
#elif defined(GEMINI_SLAVE)
	enaddr[5] = 1;
#else
#error not master nor slave
#endif

	aprint_normal("\n");
	aprint_naive("\n");
	sc->sc_dev = self;
	sc->sc_dmat = &gemini_bus_dma_tag;

	/*
	 * Pretend we are full-duplex gigabit ethernet.
	 */
	ifmedia_init(&sc->sc_im, 0, gpn_mediachange, gpn_mediastatus);
	ifmedia_add(&sc->sc_im, IFM_ETHER|IFM_1000_T|IFM_FDX, 0, NULL);
	ifmedia_add(&sc->sc_im, IFM_ETHER|IFM_NONE, 0, NULL);
	ifmedia_set(&sc->sc_im, IFM_ETHER|IFM_NONE);

	strlcpy(ifp->if_xname, device_xname(self), sizeof(ifp->if_xname));
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = gpn_ifioctl;
	ifp->if_start = gpn_ifstart;
	ifp->if_init  = gpn_ifinit;
	ifp->if_stop  = gpn_ifstop;

	IFQ_SET_READY(&ifp->if_snd);

	sc->sc_ec.ec_capabilities = ETHERCAP_VLAN_MTU | ETHERCAP_JUMBO_MTU;

	if_attach(ifp);
	ether_ifattach(ifp, enaddr);

	sc->sc_free = MAX_TXACTIVE*2;
	sc->sc_ih = gemini_ipm_register(IPM_TAG_GPN, IPL_SOFTNET, sc->sc_free,
	    gpn_ipm_handler, gpn_ipm_rebate, sc);
	KASSERT(sc->sc_ih);

	sc->sc_ack_desc.agd_tag = IPM_TAG_GPN;
}

void gpn_print_gd(ipm_gpn_desc_t *);
void
gpn_print_gd(ipm_gpn_desc_t *gd)
{
	printf("%s: %p\n", __FUNCTION__, gd);
	printf("\ttag %d, subtype %d, id %d, pktlen64 %d\n",
		gd->gd_tag, gd->gd_subtype, gd->gd_txid, gd->gd_pktlen64);
	printf("\tlen1 %d, len2 %d, addr1 %#x, addr2 %#x\n",
		gd->gd_len1, gd->gd_len2, gd->gd_addr1, gd->gd_addr2);
}

CFATTACH_DECL_NEW(gpn, sizeof(struct gpn_softc),
    gpn_match, gpn_attach, NULL, NULL);

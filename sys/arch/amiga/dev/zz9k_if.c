/*	$NetBSD: zz9k_if.c,v 1.1 2023/05/03 13:49:30 phx Exp $ */

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Alain Runa.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
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
__KERNEL_RCSID(0, "$NetBSD: zz9k_if.c,v 1.1 2023/05/03 13:49:30 phx Exp $");

/* miscellaneous */
#include <sys/types.h>			/* size_t */
#include <sys/stdint.h>			/* uintXX_t */
#include <sys/stdbool.h>		/* bool */
#include <sys/syslog.h>			/* log(), LOG_ERR */

/* driver(9) */
#include <sys/param.h>			/* NODEV */
#include <sys/device.h>			/* CFATTACH_DECL_NEW(), device_priv() */
#include <sys/errno.h>			/* EINVAL, ENODEV, EPASSTHROUGH */

/* bus_space(9) and zorro bus */
#include <sys/bus.h>			/* bus_space_xxx(), bus_space_xxx_t */
#include <sys/cpu.h>			/* kvtop() */
#include <sys/systm.h>			/* aprint_xxx(), memcpy() */
#include <amiga/dev/zbusvar.h>		/* zbus_args */

/* arp(9) and mbuf(9) */
#include <net/if.h>			/* if_oerrors */
#include <net/if_ether.h>		/* ethercom, ifnet, ether_ifattach() */
#include <net/if_dl.h>			/* satosdl(), sockaddr_dl, dl_addr */
#include <net/bpf.h>			/* bpf_mtap(), BPF_D_OUT */
#include <netinet/if_inarp.h>		/* arp_ifinit() */
#include <sys/mbuf.h>			/* mbuf_xxx */
#include <sys/sockio.h>			/* SIOXXX */

/* Interrupt related */
#include <sys/intr.h>			/* splvm(), splx() */ 
#include <amiga/amiga/isr.h>		/* isr */

/* zz9k related */
#include <amiga/dev/zz9kvar.h>		/* zz9kbus_attach_args */
#include <amiga/dev/zz9kreg.h>		/* ZZ9000 registers */
#include "zz9k_if.h"			/* NZZ9K_IF */


/* The allmighty softc structure */
struct zzif_softc {
	device_t sc_dev;
	struct bus_space_tag sc_bst;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_regh;
	bus_space_handle_t sc_rxh;
	bus_space_handle_t sc_txh;
	
	struct ethercom sc_ethercom;
	struct isr sc_isr;
	void* sc_txbuffer;
	void* sc_rxbuffer;
	uint16_t sc_sequence;
};

/* rx buffer contents */
struct zzif_frame {
	uint16_t size;
	uint16_t serial;
	struct ether_header header;
	uint8_t payload[ETHER_MAX_LEN - ETHER_HDR_LEN];
};


/* ifnet related callbacks */
static void zzif_init(struct zzif_softc *sc);
static void zzif_stop(struct zzif_softc *sc);
static void zzif_start(struct ifnet *ifp);
static int zzif_intr(void *arg);
static int zzif_ioctl(struct ifnet *ifp, u_long cmd, void *data);

/* driver(9) essentials */
static int zzif_match(device_t parent, cfdata_t match, void *aux);
static void zzif_attach(device_t parent, device_t self, void *aux);
CFATTACH_DECL_NEW(
    zz9k_if, sizeof(struct zzif_softc), zzif_match, zzif_attach, NULL, NULL);


/* Oh my God, it's full of stars! */

static int
zzif_match(device_t parent, cfdata_t match, void *aux)
{
	struct zz9kbus_attach_args *bap = aux;

	if (strcmp(bap->zzaa_name, "zz9k_if") != 0)
		return 0;

	return 1;
}

static void
zzif_attach(device_t parent, device_t self, void *aux)
{
	struct zz9kbus_attach_args *bap = aux;
	struct zzif_softc *sc = device_private(self);
	struct zz9k_softc *psc = device_private(parent);
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint8_t lla[ETHER_ADDR_LEN];

	sc->sc_dev = self;
	sc->sc_bst.base = bap->zzaa_base;
	sc->sc_bst.absm = &amiga_bus_stride_1;
	sc->sc_iot = &sc->sc_bst;
	sc->sc_regh = psc->sc_regh;

	if (bus_space_map(sc->sc_iot, ZZ9K_RX_BASE, ZZ9K_RX_SIZE,
	    BUS_SPACE_MAP_LINEAR, &sc->sc_rxh)) {
		aprint_error(": Failed to map MNT ZZ9000 eth rx buffer.\n");
		return;
	}

	if (bus_space_map(sc->sc_iot, ZZ9K_TX_BASE, ZZ9K_TX_SIZE,
	    BUS_SPACE_MAP_LINEAR, &sc->sc_txh)) {
		aprint_error(": Failed to map MNT ZZ9000 eth tx buffer.\n");
		return;
	}

	/* get MAC from NIC */
	uint16_t macHI = ZZREG_R(ZZ9K_ETH_MAC_HI);
	uint16_t macMD = ZZREG_R(ZZ9K_ETH_MAC_MD);
	uint16_t macLO = ZZREG_R(ZZ9K_ETH_MAC_LO);
	lla[0] = macHI >> 8;
	lla[1] = macHI & 0xff;
	lla[2] = macMD >> 8;
	lla[3] = macMD & 0xff;
	lla[4] = macLO >> 8;
	lla[5] = macLO & 0xff;

#if 0
	aprint_normal(": Ethernet address %s  "
	    "(10BASE-T, 100BASE-TX, AUTO)\n", ether_sprintf(lla));
#endif

	aprint_debug_dev(sc->sc_dev, "[DEBUG] registers at %p/%p (pa/va), "
	    "rx buffer at %p/%p (pa/va), tx buffer at %p/%p (pa/va)\n",
	    (void *)kvtop((void *)sc->sc_regh),
	    bus_space_vaddr(sc->sc_iot, sc->sc_regh),
	    (void *)kvtop((void *)sc->sc_rxh),
	    bus_space_vaddr(sc->sc_iot, sc->sc_rxh),
	    (void *)kvtop((void *)sc->sc_txh),
	    bus_space_vaddr(sc->sc_iot, sc->sc_txh));

	/* set rx/tx buffers */
	sc->sc_txbuffer = bus_space_vaddr(sc->sc_iot, sc->sc_txh);
	sc->sc_rxbuffer = bus_space_vaddr(sc->sc_iot, sc->sc_rxh);

	/* configure the network interface */
	memcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_ioctl = zzif_ioctl;
	ifp->if_start = zzif_start;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX;
	ifp->if_mtu   = ETHERMTU;

	/* attach the network interface. */
	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, lla);

	aprint_normal(": Ethernet address %s\n", ether_sprintf(lla));

	/* setup the (receiving) interrupt service routine on int6 (default) */
	sc->sc_isr.isr_intr = zzif_intr;
	sc->sc_isr.isr_arg  = sc;
	sc->sc_isr.isr_ipl  = 6;
	add_isr(&sc->sc_isr);
}

static void
zzif_init(struct zzif_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int s = splvm();

	ZZREG_W(ZZ9K_CONFIG, ZZREG_R(ZZ9K_CONFIG) | ZZ9K_CONFIG_INT_ETH);
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	if_schedule_deferred_start(ifp);
	splx(s);
}

static void
zzif_stop(struct zzif_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int s = splvm();

	ZZREG_W(ZZ9K_CONFIG, ZZREG_R(ZZ9K_CONFIG) & ~ZZ9K_CONFIG_INT_ETH);
	ifp->if_flags &= ~IFF_RUNNING;
	splx(s);
}

static void
zzif_start(struct ifnet *ifp)
{
	struct zzif_softc *sc = ifp->if_softc;
	uint8_t *frame= (uint8_t *)sc->sc_txbuffer;
	struct mbuf *m;
	int size;

	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return;

	ifp->if_flags |= IFF_OACTIVE;

	for (;;) {
		IF_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;

		/* this should never happen, otherwise there's no help */
		if ((m->m_flags & M_PKTHDR) == 0)
			panic("zzif_start: no packet header mbuf");
		
		size = m->m_pkthdr.len;
		if (size == 0)
			panic("zzif_start: header mbuf with 0 len");

		if ((size < (ETHER_HDR_LEN)) ||
		    (size > (ETHER_MAX_LEN-ETHER_CRC_LEN))) {
			aprint_error_dev(sc->sc_dev,
			    ": abnormal tx frame size of %i bytes\n", size);
			m_freem(m);
			break;
		}

		/* make bpf happy */
		bpf_mtap(ifp, m, BPF_D_OUT);
		
		/* copy dequeued mbuf data to tranmit buffer of the ZZ9000 */
		for (struct mbuf *n = m; n != NULL; n = n->m_next) {
			memcpy(frame, n->m_data, n->m_len);
			frame += n->m_len;
		}

		m_freem(m);	/* we don't need it anymore */

		/* tell ZZ9000 to transmit packet with size and check result */ 
		ZZREG_W(ZZ9K_ETH_TX, size);
		size = ZZREG_R(ZZ9K_ETH_TX);
		if (size == 0) {
			if_statinc(ifp, if_opackets);
		} else {
			if_statinc(ifp, if_oerrors);
		}
	}

	ifp->if_flags &= ~IFF_OACTIVE;
}

static int
zzif_intr(void *arg)
{
	struct zzif_softc *sc = arg;

	uint16_t conf = ZZREG_R(ZZ9K_CONFIG);
	if ((conf & ZZ9K_CONFIG_INT_ETH) == 0)
		return 0;

	ZZREG_W(ZZ9K_CONFIG, conf & ~ZZ9K_CONFIG_INT_ETH);  /* disable INT */
	ZZREG_W(ZZ9K_CONFIG, ZZ9K_CONFIG_INT_ACK | ZZ9K_CONFIG_INT_ACK_ETH);

	struct zzif_frame *frame = (struct zzif_frame *)sc->sc_rxbuffer;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *m;

	/* check if interrupt was due to new packet arrival */
	if (frame->serial == sc->sc_sequence) {
		ZZREG_W(ZZ9K_ETH_RX, 1);	/* discard packet */
		ZZREG_W(ZZ9K_CONFIG, conf);	/* restore interrupt */
		return 0;	/* nope, something else triggered it */
	}

	sc->sc_sequence = frame->serial;

	if ((frame->size < (ETHER_MIN_LEN-ETHER_CRC_LEN)) ||
	    (frame->size > ETHER_MAX_LEN)) {
		aprint_error_dev(sc->sc_dev,
		    ": abnormal rx frame size of %i bytes\n", frame->size);
		ZZREG_W(ZZ9K_ETH_RX, 1);	/* discard packet */
		ZZREG_W(ZZ9K_CONFIG, conf);	/* restore interrupt */
		return 0;
	}

	int s = splvm();
	m = m_devget((char *)&frame->header, frame->size, 0, ifp);
	if_percpuq_enqueue(ifp->if_percpuq, m);
	splx(s);
	ZZREG_W(ZZ9K_ETH_RX, 1);	/* packet served */
	ZZREG_W(ZZ9K_CONFIG, conf);	/* restore interrupt */

	return 1;
}

static int
zzif_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct zzif_softc *sc = ifp->if_softc;
	struct ifaddr *ifa;
	struct if_laddrreq *lar;
	const struct sockaddr_dl *sdl;
	int retval = 0;

	int s = splvm();

	switch (cmd) {
	case SIOCINITIFADDR:
		ifa = (struct ifaddr *)data;
		zzif_stop(sc);
		zzif_init(sc);
		switch (ifa->ifa_addr->sa_family) {
		case AF_INET:
			arp_ifinit(ifp, ifa);
			break;
		default:
			break;
		}
		break;
	case SIOCSIFFLAGS:
		retval = ifioctl_common(ifp, cmd, data);
		if (retval != 0)
			break;

		switch (ifp->if_flags & (IFF_UP | IFF_RUNNING)) {
		case IFF_RUNNING: /* and not UP */
			zzif_stop(sc);
			break;
		case IFF_UP: /* and not RUNNING */
			zzif_init(sc);
			break;
		case (IFF_UP | IFF_RUNNING):
			zzif_stop(sc);
			zzif_init(sc);
			break;
		default:
			break;
		}
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		retval = EINVAL;
		break;
	case SIOCALIFADDR:
		lar = (struct if_laddrreq *)data;
		sdl = satocsdl(&lar->addr);
		uint8_t *mac = (uint8_t *)&sdl->sdl_addr.dl_data;

		if ((lar->flags==IFLR_ACTIVE) && (sdl->sdl_family==AF_LINK)) {
			ZZREG_W(ZZ9K_ETH_MAC_HI, (mac[0] << 8) | mac[1]);
			ZZREG_W(ZZ9K_ETH_MAC_MD, (mac[2] << 8) | mac[3]);
			ZZREG_W(ZZ9K_ETH_MAC_LO, (mac[4] << 8) | mac[5]);
		}
		aprint_normal(": Ethernet address %s", ether_sprintf(mac));
		retval = ether_ioctl(ifp, cmd, data);
		break;
	default:
		retval = ether_ioctl(ifp, cmd, data);
		break;
	}

	splx(s);
	
	return retval;
}

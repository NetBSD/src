/*	$NetBSD: an.c,v 1.6 2000/12/13 20:21:10 onoe Exp $	*/
/*
 * Copyright (c) 1997, 1998, 1999
 *	Bill Paul <wpaul@ctr.columbia.edu>.  All rights reserved.
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
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/dev/an/if_an.c,v 1.12 2000/11/13 23:04:12 wpaul Exp $
 */

/*
 * Aironet 4500/4800 802.11 PCMCIA/ISA/PCI driver for FreeBSD.
 *
 * Written by Bill Paul <wpaul@ctr.columbia.edu>
 * Electrical Engineering Department
 * Columbia University, New York City
 */

/*
 * The Aironet 4500/4800 series cards some in PCMCIA, ISA and PCI form.
 * This driver supports all three device types (PCI devices are supported
 * through an extra PCI shim: /sys/pci/if_an_p.c). ISA devices can be
 * supported either using hard-coded IO port/IRQ settings or via Plug
 * and Play. The 4500 series devices support 1Mbps and 2Mbps data rates.
 * The 4800 devices support 1, 2, 5.5 and 11Mbps rates.
 *
 * Like the WaveLAN/IEEE cards, the Aironet NICs are all essentially
 * PCMCIA devices. The ISA and PCI cards are a combination of a PCMCIA
 * device and a PCMCIA to ISA or PCMCIA to PCI adapter card. There are
 * a couple of important differences though:
 *
 * - Lucent doesn't currently offer a PCI card, however Aironet does
 * - Lucent ISA card looks to the host like a PCMCIA controller with
 *   a PCMCIA WaveLAN card inserted. This means that even desktop
 *   machines need to be configured with PCMCIA support in order to
 *   use WaveLAN/IEEE ISA cards. The Aironet cards on the other hand
 *   actually look like normal ISA and PCI devices to the host, so
 *   no PCMCIA controller support is needed
 *
 * The latter point results in a small gotcha. The Aironet PCMCIA
 * cards can be configured for one of two operating modes depending
 * on how the Vpp1 and Vpp2 programming voltages are set when the
 * card is activated. In order to put the card in proper PCMCIA
 * operation (where the CIS table is visible and the interface is
 * programmed for PCMCIA operation), both Vpp1 and Vpp2 have to be
 * set to 5 volts. FreeBSD by default doesn't set the Vpp voltages,
 * which leaves the card in ISA/PCI mode, which prevents it from
 * being activated as an PCMCIA device. Consequently, /sys/pccard/pccard.c
 * has to be patched slightly in order to enable the Vpp voltages in
 * order to make the Aironet PCMCIA cards work.
 *
 * Note that some PCMCIA controller software packages for Windows NT
 * fail to set the voltages as well.
 * 
 * The Aironet devices can operate in both station mode and access point
 * mode. Typically, when programmed for station mode, the card can be set
 * to automatically perform encapsulation/decapsulation of Ethernet II
 * and 802.3 frames within 802.11 frames so that the host doesn't have
 * to do it itself. This driver doesn't program the card that way: the
 * driver handles all of the encapsulation/decapsulation itself.
 */

/*
 * Ported to NetBSD from FreeBSD by Atsushi Onoe at the San Diego
 * IETF meeting.
 */

#include "opt_inet.h"
#include "bpfilter.h"

#ifdef INET
#define ANCACHE			/* enable signal strength cache */
#endif

#include <sys/param.h>
#include <sys/callout.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/ucred.h>
#include <sys/socket.h>
#include <sys/device.h>
#ifdef ANCACHE
#include <sys/syslog.h>
#include <sys/sysctl.h>
#endif

#include <machine/bus.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_ieee80211.h>
#include <net/if_types.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <dev/ic/anreg.h>
#include <dev/ic/anvar.h>

#if !defined(lint)
static const char rcsid[] =
  "$FreeBSD: src/sys/dev/an/if_an.c,v 1.12 2000/11/13 23:04:12 wpaul Exp $";
#endif

/* These are global because we need them in sys/pci/if_an_p.c. */
static void an_reset		__P((struct an_softc *));
static int an_ioctl		__P((struct ifnet *, u_long, caddr_t));
static int an_init		__P((struct ifnet *));
static void an_stop		__P((struct ifnet *, int));
static int an_init_tx_ring	__P((struct an_softc *));
static void an_start		__P((struct ifnet *));
static void an_watchdog		__P((struct ifnet *));
static void an_rxeof		__P((struct an_softc *));
static void an_txeof		__P((struct an_softc *, int));

static void an_promisc		__P((struct an_softc *, int));
static int an_cmd		__P((struct an_softc *, int, int));
static int an_read_record	__P((struct an_softc *, struct an_ltv_gen *));
static int an_write_record	__P((struct an_softc *, struct an_ltv_gen *));
static int an_read_data		__P((struct an_softc *, int,
					int, caddr_t, int));
static int an_write_data	__P((struct an_softc *, int,
					int, caddr_t, int));
static int an_seek		__P((struct an_softc *, int, int, int));
static int an_alloc_nicmem	__P((struct an_softc *, int, int *));
static void an_stats_update	__P((void *));
static void an_setdef		__P((struct an_softc *, struct an_req *));
#ifdef ANCACHE
static void an_cache_store	__P((struct an_softc *, struct ether_header *,
					struct mbuf *, unsigned short));
#endif
#ifdef IFM_IEEE80211
static int an_media_change __P((struct ifnet *ifp));
static void an_media_status __P((struct ifnet *ifp, struct ifmediareq *imr));
#endif

/* 
 * We probe for an Aironet 4500/4800 card by attempting to
 * read the default SSID list. On reset, the first entry in
 * the SSID list will contain the name "tsunami." If we don't
 * find this, then there's no card present.
 */
int an_probe(sc)
	struct an_softc *sc;
{
	struct an_ltv_ssidlist	ssid;

	bzero((char *)&ssid, sizeof(ssid));

	ssid.an_len = sizeof(ssid);
	ssid.an_type = AN_RID_SSIDLIST;

        /* Make sure interrupts are disabled. */
        CSR_WRITE_2(sc, AN_INT_EN, 0);
        CSR_WRITE_2(sc, AN_EVENT_ACK, 0xFFFF);

	an_reset(sc);

	if (an_cmd(sc, AN_CMD_READCFG, 0))
		return(0);

	if (an_read_record(sc, (struct an_ltv_gen *)&ssid))
		return(0);

	/* See if the ssid matches what we expect ... but doesn't have to */
	if (strcmp(ssid.an_ssid1, AN_DEF_SSID))
		return(0);
	
	return(AN_IOSIZ);
}

int an_attach(sc)
	struct an_softc *sc;
{
	struct ifnet		*ifp = &sc->arpcom.ec_if;
#ifdef IFM_IEEE80211
	int i, mtype;
	struct ifmediareq imr;
#endif

	sc->an_associated = 0;

	/* Reset the NIC. */
	an_reset(sc);

	/* Load factory config */
	if (an_cmd(sc, AN_CMD_READCFG, 0)) {
		printf("%s: failed to load config data\n", sc->an_dev.dv_xname);
		return(EIO);
	}

	/* Read the current configuration */
	sc->an_config.an_type = AN_RID_GENCONFIG;
	sc->an_config.an_len = sizeof(struct an_ltv_genconfig);
	if (an_read_record(sc, (struct an_ltv_gen *)&sc->an_config)) {
		printf("%s: read record failed\n", sc->an_dev.dv_xname);
		return(EIO);
	}

	/* Read the card capabilities */
	sc->an_caps.an_type = AN_RID_CAPABILITIES;
	sc->an_caps.an_len = sizeof(struct an_ltv_caps);
	if (an_read_record(sc, (struct an_ltv_gen *)&sc->an_caps)) {
		printf("%s: read record failed\n", sc->an_dev.dv_xname);
		return(EIO);
	}

	/* Read ssid list */
	sc->an_ssidlist.an_type = AN_RID_SSIDLIST;
	sc->an_ssidlist.an_len = sizeof(struct an_ltv_ssidlist);
	if (an_read_record(sc, (struct an_ltv_gen *)&sc->an_ssidlist)) {
		printf("%s: read record failed\n", sc->an_dev.dv_xname);
		return(EIO);
	}

	/* Read AP list */
	sc->an_aplist.an_type = AN_RID_APLIST;
	sc->an_aplist.an_len = sizeof(struct an_ltv_aplist);
	if (an_read_record(sc, (struct an_ltv_gen *)&sc->an_aplist)) {
		printf("%s: read record failed\n", sc->an_dev.dv_xname);
		return(EIO);
	}

	printf("%s: 802.11 address: %s\n", sc->an_dev.dv_xname,
	    ether_sprintf(sc->an_caps.an_oemaddr));

	ifp->if_softc = sc;
	ifp->if_flags =
	    IFF_BROADCAST | IFF_NOTRAILERS | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = an_ioctl;
	ifp->if_start = an_start;
	ifp->if_init = an_init;
	ifp->if_stop = an_stop;
	ifp->if_watchdog = an_watchdog;

	memcpy(ifp->if_xname, sc->an_dev.dv_xname, IFNAMSIZ);

	bzero(sc->an_config.an_nodename, sizeof(sc->an_config.an_nodename));
	bcopy(AN_DEFAULT_NODENAME, sc->an_config.an_nodename,
	    sizeof(AN_DEFAULT_NODENAME) - 1);

	bzero(sc->an_ssidlist.an_ssid1, sizeof(sc->an_ssidlist.an_ssid1));
	bcopy(AN_DEFAULT_NETNAME, sc->an_ssidlist.an_ssid1,
	    sizeof(AN_DEFAULT_NETNAME) - 1);
	sc->an_ssidlist.an_ssid1_len = strlen(AN_DEFAULT_NETNAME);

	sc->an_config.an_opmode = AN_OPMODE_INFRASTRUCTURE_STATION;

	sc->an_tx_rate = 0;
	bzero((char *)&sc->an_stats, sizeof(sc->an_stats));

	/*
	 * Call MI attach routine.
	 */
	if_attach(ifp);
	ether_ifattach(ifp, sc->an_caps.an_oemaddr);

#ifdef IFM_IEEE80211
	ifmedia_init(&sc->sc_media, 0, an_media_change, an_media_status);
	ifmedia_add(&sc->sc_media, IFM_MAKEWORD(IFM_IEEE80211, IFM_AUTO,
	    0, 0), 0, NULL);
	ifmedia_add(&sc->sc_media, IFM_MAKEWORD(IFM_IEEE80211, IFM_AUTO,
	    IFM_IEEE80211_ADHOC, 0), 0, NULL);
	for (i = 0; i < sizeof(sc->an_caps.an_rates); i++) {
		switch (sc->an_caps.an_rates[i]) {
		case AN_RATE_1MBPS:
			mtype = IFM_IEEE80211_DS1;
			break;
		case AN_RATE_2MBPS:
			mtype = IFM_IEEE80211_DS2;
			break;
		case AN_RATE_5_5MBPS:
			mtype = IFM_IEEE80211_DS5;
			break;
		case AN_RATE_11MBPS:
			mtype = IFM_IEEE80211_DS11;
			break;
		default:
			continue;
		}
		ifmedia_add(&sc->sc_media, IFM_MAKEWORD(IFM_IEEE80211, mtype,
		    0, 0), 0, NULL);
		ifmedia_add(&sc->sc_media, IFM_MAKEWORD(IFM_IEEE80211, mtype,
		    IFM_IEEE80211_ADHOC, 0), 0, NULL);
	}
	an_media_status(ifp, &imr);
	ifmedia_set(&sc->sc_media, imr.ifm_active);
#endif
	callout_init(&sc->an_stat_ch);

	return(0);
}

int
an_detach(sc)
	struct an_softc *sc;
{
	struct ifnet *ifp = &sc->arpcom.ec_if;
	int s;

	s = splnet();
	an_stop(ifp, 1);
	ifmedia_delete_instance(&sc->sc_media, IFM_INST_ANY);
	ether_ifdetach(ifp);
	if_detach(ifp);
	splx(s);
	return 0;
}

int
an_activate(self, act)
	struct device *self;
	enum devact act;
{
	struct an_softc *sc = (struct an_softc *)self;
	int s, error = 0;

	s = splnet();
	switch (act) {
	case DVACT_ACTIVATE:
		error = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		if_deactivate(&sc->arpcom.ec_if);
		break;
	}
	splx(s);

	return error;
}

static void an_rxeof(sc)
	struct an_softc		*sc;
{
	struct ifnet		*ifp;
	struct ether_header	*eh;
#ifdef ANCACHE
	struct an_rxframe	rx_frame;
#endif
	struct an_rxframe_802_3	rx_frame_802_3;
	struct mbuf		*m;
	int			id, error = 0;

	ifp = &sc->arpcom.ec_if;

	id = CSR_READ_2(sc, AN_RX_FID);

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL) {
		ifp->if_ierrors++;
		return;
	}
	MCLGET(m, M_DONTWAIT);
	if (!(m->m_flags & M_EXT)) {
		m_freem(m);
		ifp->if_ierrors++;
		return;
	}

	m->m_pkthdr.rcvif = ifp;

	eh = mtod(m, struct ether_header *);

#ifdef ANCACHE
	/* Read NIC frame header */
	if (an_read_data(sc, id, 0, (caddr_t)&rx_frame, sizeof(rx_frame))) {
		ifp->if_ierrors++;
		return;
	}
#endif
	/* Read in the 802_3 frame header */
	if (an_read_data(sc, id, 0x34, (caddr_t)&rx_frame_802_3, 
			 sizeof(rx_frame_802_3))) {
		ifp->if_ierrors++;
		return;
	}

	if (rx_frame_802_3.an_rx_802_3_status != 0) {
		ifp->if_ierrors++;
		return;
	}

	/* Check for insane frame length */
	if (rx_frame_802_3.an_rx_802_3_payload_len > MCLBYTES) {
		ifp->if_ierrors++;
		return;
	}

	m->m_pkthdr.len = m->m_len =
	    rx_frame_802_3.an_rx_802_3_payload_len + 12;

	
	bcopy((char *)&rx_frame_802_3.an_rx_dst_addr,
	    (char *)&eh->ether_dhost, ETHER_ADDR_LEN);
	bcopy((char *)&rx_frame_802_3.an_rx_src_addr,
	    (char *)&eh->ether_shost, ETHER_ADDR_LEN);

	/* in mbuf header type is just before payload */
	error = an_read_data(sc, id, 0x44, (caddr_t)&(eh->ether_type), 
			     rx_frame_802_3.an_rx_802_3_payload_len);

	if (error) {
		m_freem(m);
		ifp->if_ierrors++;
		return;
	}

	ifp->if_ipackets++;

	/* Receive packet. */
#ifdef ANCACHE
	an_cache_store(sc, eh, m, rx_frame.an_rx_signal_strength);
#endif
#if NBPFILTER > 0
	if (ifp->if_bpf)
		bpf_mtap(ifp->if_bpf, m);
#endif
	(*ifp->if_input)(ifp, m);
}

static void an_txeof(sc, status)
	struct an_softc		*sc;
	int			status;
{
	struct ifnet		*ifp;
	int			id;

	/* TX DONE enable lan monitor DJA
	   an_enable_sniff();
	 */

	ifp = &sc->arpcom.ec_if;

	ifp->if_timer = 0;
	ifp->if_flags &= ~IFF_OACTIVE;

	id = CSR_READ_2(sc, AN_TX_CMP_FID);

	if (status & AN_EV_TX_EXC) {
		ifp->if_oerrors++;
	} else
		ifp->if_opackets++;

#if 0 /*XXX*/
	if (id != sc->an_rdata.an_tx_ring[sc->an_rdata.an_tx_cons])
		printf("%s: id mismatch: expected %x, got %x\n",
		    sc->an_dev.dv_xname,
		    sc->an_rdata.an_tx_ring[sc->an_rdata.an_tx_cons], id);
#endif

	sc->an_rdata.an_tx_ring[sc->an_rdata.an_tx_cons] = 0;
	AN_INC(sc->an_rdata.an_tx_cons, AN_TX_RING_CNT);

	return;
}

/*
 * We abuse the stats updater to check the current NIC status. This
 * is important because we don't want to allow transmissions until
 * the NIC has synchronized to the current cell (either as the master
 * in an ad-hoc group, or as a station connected to an access point).
 */
void an_stats_update(xsc)
	void			*xsc;
{
	struct an_softc		*sc;
	struct ifnet		*ifp;

	sc = xsc;
	ifp = &sc->arpcom.ec_if;

	sc->an_status.an_type = AN_RID_STATUS;
	sc->an_status.an_len = sizeof(struct an_ltv_status);
	an_read_record(sc, (struct an_ltv_gen *)&sc->an_status);

	if (sc->an_status.an_opmode & AN_STATUS_OPMODE_IN_SYNC)
		sc->an_associated = 1;
	else
		sc->an_associated = 0;

	/* Don't do this while we're transmitting */
	if (ifp->if_flags & IFF_OACTIVE) {
		callout_reset(&sc->an_stat_ch, hz, an_stats_update, sc);
		return;
	}

	sc->an_stats.an_len = sizeof(struct an_ltv_stats);
	sc->an_stats.an_type = AN_RID_32BITS_CUM;
	an_read_record(sc, (struct an_ltv_gen *)&sc->an_stats.an_len);

	callout_reset(&sc->an_stat_ch, hz, an_stats_update, sc);

	return;
}

int an_intr(xsc)
	void			*xsc;
{
	struct an_softc		*sc;
	struct ifnet		*ifp;
	u_int16_t		status;

	sc = (struct an_softc*)xsc;

	if (!sc->sc_enabled)
		return 0;

	ifp = &sc->arpcom.ec_if;

	if (!(ifp->if_flags & IFF_UP)) {
		CSR_WRITE_2(sc, AN_EVENT_ACK, 0xFFFF);
		CSR_WRITE_2(sc, AN_INT_EN, 0);
		return 0;
	}

	/* Disable interrupts. */
	CSR_WRITE_2(sc, AN_INT_EN, 0);

	status = CSR_READ_2(sc, AN_EVENT_STAT);
	CSR_WRITE_2(sc, AN_EVENT_ACK, ~AN_INTRS);

	if (status & AN_EV_AWAKE) {
		CSR_WRITE_2(sc, AN_EVENT_ACK, AN_EV_AWAKE);
	}

	if (status & AN_EV_LINKSTAT) {
		if (CSR_READ_2(sc, AN_LINKSTAT) == AN_LINKSTAT_ASSOCIATED)
			sc->an_associated = 1;
		else
			sc->an_associated = 0;
		CSR_WRITE_2(sc, AN_EVENT_ACK, AN_EV_LINKSTAT);
	}

	if (status & AN_EV_RX) {
		an_rxeof(sc);
		CSR_WRITE_2(sc, AN_EVENT_ACK, AN_EV_RX);
	}

	if (status & AN_EV_TX) {
		an_txeof(sc, status);
		CSR_WRITE_2(sc, AN_EVENT_ACK, AN_EV_TX);
	}

	if (status & AN_EV_TX_EXC) {
		an_txeof(sc, status);
		CSR_WRITE_2(sc, AN_EVENT_ACK, AN_EV_TX_EXC);
	}

	if (status & AN_EV_ALLOC)
		CSR_WRITE_2(sc, AN_EVENT_ACK, AN_EV_ALLOC);

	/* Re-enable interrupts. */
	CSR_WRITE_2(sc, AN_INT_EN, AN_INTRS);

	if (ifp->if_snd.ifq_head != NULL)
		an_start(ifp);

	return 1;
}

static int an_cmd(sc, cmd, val)
	struct an_softc		*sc;
	int			cmd;
	int			val;
{
	int			i, s = 0;

	CSR_WRITE_2(sc, AN_PARAM0, val);
	CSR_WRITE_2(sc, AN_PARAM1, 0);
	CSR_WRITE_2(sc, AN_PARAM2, 0);
	CSR_WRITE_2(sc, AN_COMMAND, cmd);

	for (i = 0; i < AN_TIMEOUT; i++) {
		if (CSR_READ_2(sc, AN_EVENT_STAT) & AN_EV_CMD)
			break;
		else {
			if (CSR_READ_2(sc, AN_COMMAND) == cmd)
				CSR_WRITE_2(sc, AN_COMMAND, cmd);
		}
	}

	for (i = 0; i < AN_TIMEOUT; i++) {
		CSR_READ_2(sc, AN_RESP0);
		CSR_READ_2(sc, AN_RESP1);
		CSR_READ_2(sc, AN_RESP2);
		s = CSR_READ_2(sc, AN_STATUS);
		if ((s & AN_STAT_CMD_CODE) == (cmd & AN_STAT_CMD_CODE))
			break;
	}

	/* Ack the command */
	CSR_WRITE_2(sc, AN_EVENT_ACK, AN_EV_CMD);

	if (CSR_READ_2(sc, AN_COMMAND) & AN_CMD_BUSY)
		CSR_WRITE_2(sc, AN_EVENT_ACK, AN_EV_CLR_STUCK_BUSY);

	if (i == AN_TIMEOUT)
		return(ETIMEDOUT);

	return(0);
}

/*
 * This reset sequence may look a little strange, but this is the
 * most reliable method I've found to really kick the NIC in the
 * head and force it to reboot correctly.
 */
static void an_reset(sc)
	struct an_softc		*sc;
{
	if (!sc->sc_enabled)
		return;
        
	an_cmd(sc, AN_CMD_ENABLE, 0);
	an_cmd(sc, AN_CMD_FW_RESTART, 0);
	an_cmd(sc, AN_CMD_NOOP2, 0);

	if (an_cmd(sc, AN_CMD_FORCE_SYNCLOSS, 0) == ETIMEDOUT)
		printf("%s: reset failed\n", sc->an_dev.dv_xname);

	an_cmd(sc, AN_CMD_DISABLE, 0);

	return;
}

/*
 * Read an LTV record from the NIC.
 */
static int an_read_record(sc, ltv)
	struct an_softc		*sc;
	struct an_ltv_gen	*ltv;
{
	u_int16_t		*ptr;
	int			i, len;

	if (ltv->an_len == 0 || ltv->an_type == 0)
		return(EINVAL);

	/* Tell the NIC to enter record read mode. */
	if (an_cmd(sc, AN_CMD_ACCESS|AN_ACCESS_READ, ltv->an_type)) {
		printf("%s: RID access failed\n", sc->an_dev.dv_xname);
		return(EIO);
	}

	/* Seek to the record. */
	if (an_seek(sc, ltv->an_type, 0, AN_BAP1)) {
		printf("%s: seek to record failed\n", sc->an_dev.dv_xname);
		return(EIO);
	}

	/*
	 * Read the length and record type and make sure they
	 * match what we expect (this verifies that we have enough
	 * room to hold all of the returned data).
	 */
	len = CSR_READ_2(sc, AN_DATA1);
	if (len > ltv->an_len) {
		printf("%s: record length mismatch -- expected %d, "
		    "got %d\n", sc->an_dev.dv_xname, ltv->an_len, len);
		return(ENOSPC);
	}

	ltv->an_len = len;

	/* Now read the data. */
	ptr = &ltv->an_val;
	for (i = 0; i < (ltv->an_len - 2) >> 1; i++)
		ptr[i] = CSR_READ_2(sc, AN_DATA1);

	return(0);
}

/*
 * Same as read, except we inject data instead of reading it.
 */
static int an_write_record(sc, ltv)
	struct an_softc		*sc;
	struct an_ltv_gen	*ltv;
{
	u_int16_t		*ptr;
	int			i;

	if (an_cmd(sc, AN_CMD_ACCESS|AN_ACCESS_READ, ltv->an_type))
		return(EIO);

	if (an_seek(sc, ltv->an_type, 0, AN_BAP1))
		return(EIO);

	CSR_WRITE_2(sc, AN_DATA1, ltv->an_len-2);
	
	ptr = &ltv->an_val;
	for (i = 0; i < (ltv->an_len - 4) >> 1; i++)
		CSR_WRITE_2(sc, AN_DATA1, ptr[i]);

	if (an_cmd(sc, AN_CMD_ACCESS|AN_ACCESS_WRITE, ltv->an_type))
		return(EIO);

	return(0);
}

static int an_seek(sc, id, off, chan)
	struct an_softc		*sc;
	int			id, off, chan;
{
	int			i;
	int			selreg, offreg;

	switch (chan) {
	case AN_BAP0:
		selreg = AN_SEL0;
		offreg = AN_OFF0;
		break;
	case AN_BAP1:
		selreg = AN_SEL1;
		offreg = AN_OFF1;
		break;
	default:
		printf("%s: invalid data path: %x\n", sc->an_dev.dv_xname, chan);
		return(EIO);
	}

	CSR_WRITE_2(sc, selreg, id);
	CSR_WRITE_2(sc, offreg, off);

	for (i = 0; i < AN_TIMEOUT; i++) {
		if (!(CSR_READ_2(sc, offreg) & (AN_OFF_BUSY|AN_OFF_ERR)))
			break;
	}

	if (i == AN_TIMEOUT)
		return(ETIMEDOUT);

	return(0);
}

static int an_read_data(sc, id, off, buf, len)
	struct an_softc		*sc;
	int			id, off;
	caddr_t			buf;
	int			len;
{
	int			i;
	u_int16_t		*ptr;
	u_int8_t		*ptr2;

	if (off != -1) {
		if (an_seek(sc, id, off, AN_BAP1))
			return(EIO);
	}

	ptr = (u_int16_t *)buf;
	for (i = 0; i < len / 2; i++)
		ptr[i] = CSR_READ_2(sc, AN_DATA1);
	i*=2;
	if (i<len){
	        ptr2 = (u_int8_t *)buf;
	        ptr2[i] = CSR_READ_1(sc, AN_DATA1);
	}

	return(0);
}

static int an_write_data(sc, id, off, buf, len)
	struct an_softc		*sc;
	int			id, off;
	caddr_t			buf;
	int			len;
{
	int			i;
	u_int16_t		*ptr;
	u_int8_t		*ptr2;

	if (off != -1) {
		if (an_seek(sc, id, off, AN_BAP0))
			return(EIO);
	}

	ptr = (u_int16_t *)buf;
	for (i = 0; i < (len / 2); i++)
		CSR_WRITE_2(sc, AN_DATA0, ptr[i]);
	i*=2;
	if (i<len){
	        ptr2 = (u_int8_t *)buf;
	        CSR_WRITE_1(sc, AN_DATA0, ptr2[i]);
	}

	return(0);
}

/*
 * Allocate a region of memory inside the NIC and zero
 * it out.
 */
static int an_alloc_nicmem(sc, len, id)
	struct an_softc		*sc;
	int			len;
	int			*id;
{
	int			i;

	if (an_cmd(sc, AN_CMD_ALLOC_MEM, len)) {
		printf("%s: failed to allocate %d bytes on NIC\n",
		    sc->an_dev.dv_xname, len);
		return(ENOMEM);
	}

	for (i = 0; i < AN_TIMEOUT; i++) {
		if (CSR_READ_2(sc, AN_EVENT_STAT) & AN_EV_ALLOC)
			break;
	}

	if (i == AN_TIMEOUT)
		return(ETIMEDOUT);

	CSR_WRITE_2(sc, AN_EVENT_ACK, AN_EV_ALLOC);
	*id = CSR_READ_2(sc, AN_ALLOC_FID);

	if (an_seek(sc, *id, 0, AN_BAP0))
		return(EIO);

	for (i = 0; i < len / 2; i++)
		CSR_WRITE_2(sc, AN_DATA0, 0);

	return(0);
}

static void an_setdef(sc, areq)
	struct an_softc		*sc;
	struct an_req		*areq;
{
	struct ifnet		*ifp;
	struct an_ltv_genconfig	*cfg;
	struct an_ltv_ssidlist	*ssid;
	struct an_ltv_aplist	*ap;
	struct an_ltv_gen	*sp;

	ifp = &sc->arpcom.ec_if;

	switch (areq->an_type) {
	case AN_RID_GENCONFIG:
		cfg = (struct an_ltv_genconfig *)areq;

		bcopy((char *)&cfg->an_macaddr,
		    (char *)&sc->an_caps.an_oemaddr, ETHER_ADDR_LEN);
		bcopy((char *)&cfg->an_macaddr, LLADDR(ifp->if_sadl),
		    ETHER_ADDR_LEN);

		bcopy((char *)cfg, (char *)&sc->an_config,
			sizeof(struct an_ltv_genconfig));
		break;
	case AN_RID_SSIDLIST:
		ssid = (struct an_ltv_ssidlist *)areq;
		bcopy((char *)ssid, (char *)&sc->an_ssidlist,
			sizeof(struct an_ltv_ssidlist));
		break;
	case AN_RID_APLIST:
		ap = (struct an_ltv_aplist *)areq;
		bcopy((char *)ap, (char *)&sc->an_aplist,
			sizeof(struct an_ltv_aplist));
		break;
	case AN_RID_TX_SPEED:
		sp = (struct an_ltv_gen *)areq;
		sc->an_tx_rate = sp->an_val;
		break;
	case AN_RID_WEP_VOLATILE:
		memcpy(&sc->an_temp_keys, areq, sizeof(sc->an_temp_keys));

		/* Disable the MAC. */
		an_cmd(sc, AN_CMD_DISABLE, 0);
		
		/* Just write the Key, we don't want to save it */
		an_write_record(sc, (struct an_ltv_gen *)areq);
		
		/* Turn the MAC back on. */   
		an_cmd(sc, AN_CMD_ENABLE, 0);
	
		break;
	case AN_RID_WEP_PERSISTENT:

		/* Disable the MAC. */
		an_cmd(sc, AN_CMD_DISABLE, 0);
	
		/* Just write the Key, the card will save it in this mode */
		an_write_record(sc, (struct an_ltv_gen *)areq);
		
		/* Turn the MAC back on. */   
		an_cmd(sc, AN_CMD_ENABLE, 0);
		
		break;
	default:
		printf("%s: unknown RID: %x\n", sc->an_dev.dv_xname, areq->an_type);
		return;
		break;
	}


	/* Reinitialize the card. */
	if (ifp->if_flags & IFF_UP)
		an_init(ifp);

	return;
}

/*
 * We can't change the NIC configuration while the MAC is enabled,
 * so in order to turn on RX monitor mode, we have to turn the MAC
 * off first.
 */
static void an_promisc(sc, promisc)
	struct an_softc		*sc;
	int			promisc;
{
	/* Disable the MAC. */
	an_cmd(sc, AN_CMD_DISABLE, 0);

	/* Set RX mode. */
	if (promisc && 
	    !(sc->an_config.an_rxmode & AN_RXMODE_LAN_MONITOR_CURBSS)
	    ) {
		sc->an_rxmode = sc->an_config.an_rxmode;
		/* kills card DJA, if in sniff mode can't TX packets
		sc->an_config.an_rxmode |=
		    AN_RXMODE_LAN_MONITOR_CURBSS;
		*/
	} else {
		sc->an_config.an_rxmode = sc->an_rxmode;
	}
 
	/* Transfer the configuration to the NIC */
	sc->an_config.an_len = sizeof(struct an_ltv_genconfig);
	sc->an_config.an_type = AN_RID_GENCONFIG;
	if (an_write_record(sc, (struct an_ltv_gen *)&sc->an_config)) {
		printf("%s: failed to set configuration\n", sc->an_dev.dv_xname);
		return;
	}
	/* Turn the MAC back on. */
	an_cmd(sc, AN_CMD_ENABLE, 0);

	return;
}

static int an_ioctl(ifp, command, data)
	struct ifnet		*ifp;
	u_long			command;
	caddr_t			data;
{
	int			i;
	int			error = 0;
	struct an_softc		*sc;
	struct an_req		areq;
	struct ifreq		*ifr;
	struct an_ltv_wepkey	*akey;
	struct ieee80211_nwid	nwid;
	struct ieee80211_nwkey	*nwkey;
	struct ieee80211_power	*power;

	sc = ifp->if_softc;
	ifr = (struct ifreq *)data;

	switch(command) {
	case SIOCSIFFLAGS:
		if ((ifp->if_flags & sc->an_if_flags &
		    (IFF_UP | IFF_RUNNING)) == (IFF_UP | IFF_RUNNING)) {
			if (ifp->if_flags & IFF_PROMISC &&
			    !(sc->an_if_flags & IFF_PROMISC)) {
				an_promisc(sc, 1);
				break;
			}
			if (!(ifp->if_flags & IFF_PROMISC) &&
			    sc->an_if_flags & IFF_PROMISC) {
				an_promisc(sc, 0);
				break;
			}
		}
		error = ether_ioctl(ifp, command, data);
		sc->an_if_flags = ifp->if_flags;
		break;
	case SIOCGAIRONET:
		error = copyin(ifr->ifr_data, &areq, sizeof(areq));
		if (error)
			break;
#ifdef ANCACHE
		if (areq.an_type == AN_RID_ZERO_CACHE) {
			sc->an_sigitems = sc->an_nextitem = 0;
			break;
		} else if (areq.an_type == AN_RID_READ_CACHE) {
			char *pt = (char *)&areq.an_val;
			bcopy((char *)&sc->an_sigitems, (char *)pt,
			    sizeof(int));
			pt += sizeof(int);
			areq.an_len = sizeof(int) / 2;
			bcopy((char *)&sc->an_sigcache, (char *)pt,
			    sizeof(struct an_sigcache) * sc->an_sigitems);
			areq.an_len += ((sizeof(struct an_sigcache) *
			    sc->an_sigitems) / 2) + 1;
		} else
#endif
		if (an_read_record(sc, (struct an_ltv_gen *)&areq)) {
			error = EINVAL;
			break;
		}
		error = copyout(&areq, ifr->ifr_data, sizeof(areq));
		break;
	case SIOCSAIRONET:
		if ((error = suser(curproc->p_ucred, &curproc->p_acflag)))
			goto out;
		error = copyin(ifr->ifr_data, &areq, sizeof(areq));
		if (error)
			break;
		an_setdef(sc, &areq);
		break;
	case SIOCS80211NWID:
		error = copyin(ifr->ifr_data, &nwid, sizeof(nwid));
		if (error)
			break;
		if (nwid.i_len > IEEE80211_NWID_LEN) {
			error = EINVAL;
			break;
		}
		if (sc->an_ssidlist.an_ssid1_len == nwid.i_len &&
		    memcmp(sc->an_ssidlist.an_ssid1, nwid.i_nwid, nwid.i_len)
		    == 0)
			break;
		memset(sc->an_ssidlist.an_ssid1, 0, IEEE80211_NWID_LEN);
		sc->an_ssidlist.an_ssid1_len = nwid.i_len;
		memcpy(sc->an_ssidlist.an_ssid1, nwid.i_nwid, nwid.i_len);
		if (sc->sc_enabled)
			an_init(ifp);
		break;
	case SIOCG80211NWID:
		memset(&nwid, 0, sizeof(nwid));
		if ((ifp->if_flags & IFF_RUNNING) && sc->an_associated) {
			nwid.i_len = sc->an_status.an_ssidlen;
			memcpy(nwid.i_nwid, sc->an_status.an_ssid, nwid.i_len);
		} else {
			nwid.i_len = sc->an_ssidlist.an_ssid1_len;
			memcpy(nwid.i_nwid, sc->an_ssidlist.an_ssid1,
			    nwid.i_len);
		}
		error = copyout(&nwid, ifr->ifr_data, sizeof(nwid));
		break;
	case SIOCS80211NWKEY:
		nwkey = (struct ieee80211_nwkey *)data;
		sc->an_config.an_authtype &= AN_AUTHTYPE_MASK;
		if (nwkey->i_wepon)
			sc->an_config.an_authtype |= (AN_AUTHTYPE_MASK + 1);
		akey = (struct an_ltv_wepkey *)&areq;
		memset(akey, 0, sizeof(struct an_ltv_wepkey));
		akey->an_type = AN_RID_WEP_VOLATILE;
		akey->an_len = sizeof(struct an_ltv_wepkey);
		akey->an_key_index = 0;
		akey->an_mac_addr[0] = 1;	/* default mac */
		akey->an_key_len = nwkey->i_key[0].i_keylen;
		if (akey->an_key_len > sizeof(akey->an_key)) {
			error = EINVAL;
			break;
		}
		if (nwkey->i_key[0].i_keydat != NULL) {
			if ((error = copyin(nwkey->i_key[0].i_keydat,
			    akey->an_key, akey->an_key_len)) != 0)
				break;
		}
		memcpy(&sc->an_temp_keys, akey, sizeof(struct an_ltv_wepkey));
		if (sc->sc_enabled)
			an_init(ifp);
		break;
	case SIOCG80211NWKEY:
		nwkey = (struct ieee80211_nwkey *)data;
		nwkey->i_wepon =
		    sc->an_config.an_authtype & ~AN_AUTHTYPE_MASK ? 1 : 0;
		nwkey->i_defkid = 1;
		if (nwkey->i_key[0].i_keydat == NULL)
			break;
		/* do not show any keys to non-root user */
		if ((error = suser(curproc->p_ucred, &curproc->p_acflag)) != 0)
			break;
		akey = &sc->an_temp_keys;
		nwkey->i_key[0].i_keylen = akey->an_key_len;
		for (i = 1; i < IEEE80211_WEP_NKID; i++)
			nwkey->i_key[i].i_keylen = 0;
		error = copyout(akey->an_key, nwkey->i_key[0].i_keydat,
		    akey->an_key_len);
		break;
	case SIOCS80211POWER:
		power = (struct ieee80211_power *)data;
		sc->an_config.an_psave_mode = power->i_enabled ?
		    AN_PSAVE_PSP : AN_PSAVE_NONE;
		sc->an_config.an_listen_interval = power->i_maxsleep;
		if (sc->sc_enabled)
			an_init(ifp);
		break;
	case SIOCG80211POWER:
		power = (struct ieee80211_power *)data;
		power->i_enabled =
		    sc->an_config.an_psave_mode != AN_PSAVE_NONE ? 1 : 0;
		power->i_maxsleep = sc->an_config.an_listen_interval;
		break;
#ifdef IFM_IEEE80211
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_media, command);
		break;
#endif
	default:
		error = ether_ioctl(ifp, command, data);
		break;
	}
out:
	return(error);
}

#ifdef IFM_IEEE80211
static int
an_media_change(ifp)
	struct ifnet *ifp;
{
	struct an_softc *sc = ifp->if_softc;
	struct ifmedia_entry *ime;
	int error;

	error = 0;
	ime = sc->sc_media.ifm_cur;
	switch (IFM_SUBTYPE(ime->ifm_media)) {
	case IFM_AUTO:
		sc->an_tx_rate = 0;
		break;
	case IFM_IEEE80211_DS1:
		sc->an_tx_rate = AN_RATE_1MBPS;
		break;
	case IFM_IEEE80211_DS2:
		sc->an_tx_rate = AN_RATE_2MBPS;
		break;
	case IFM_IEEE80211_DS5:
		sc->an_tx_rate = AN_RATE_5_5MBPS;
		break;
	case IFM_IEEE80211_DS11:
		sc->an_tx_rate = AN_RATE_11MBPS;
		break;
	}
	if (ime->ifm_media & IFM_IEEE80211_ADHOC)
		sc->an_config.an_opmode = AN_OPMODE_IBSS_ADHOC;
	else
		sc->an_config.an_opmode = AN_OPMODE_INFRASTRUCTURE_STATION;
	/*
	 * XXX: how to set txrate for the firmware?
	 * There is a struct defined as an_txframe, which is used nowhere.
	 * Perhaps we need to change the transmit mode from 802.3 to native.
	 */
	if (sc->sc_enabled)
		an_init(ifp);
	return error;
}

static void
an_media_status(ifp, imr)
	struct ifnet *ifp;
	struct ifmediareq *imr;
{
	struct an_softc *sc = ifp->if_softc;

	imr->ifm_status = IFM_AVALID;
	if (sc->an_associated)
		imr->ifm_status |= IFM_ACTIVE;
	imr->ifm_active = IFM_IEEE80211;
	switch (sc->an_tx_rate) {
	case 0:
		imr->ifm_active |= IFM_AUTO;
		break;
	case AN_RATE_1MBPS:
		imr->ifm_active |= IFM_IEEE80211_DS1;
		break;
	case AN_RATE_2MBPS:
		imr->ifm_active |= IFM_IEEE80211_DS2;
		break;
	case AN_RATE_5_5MBPS:
		imr->ifm_active |= IFM_IEEE80211_DS5;
		break;
	case AN_RATE_11MBPS:
		imr->ifm_active |= IFM_IEEE80211_DS11;
		break;
	}
	if ((sc->an_config.an_opmode & 0x0f) == AN_OPMODE_IBSS_ADHOC)
		imr->ifm_active |= IFM_IEEE80211_ADHOC;
}
#endif /* IFM_IEEE80211 */

static int an_init_tx_ring(sc)
	struct an_softc		*sc;
{
	int			i;
	int			id;

	if (!sc->sc_enabled)
		return (0);

	for (i = 0; i < AN_TX_RING_CNT; i++) {
		if (an_alloc_nicmem(sc, 1518 +
		    0x44, &id))
			return(ENOMEM);
		sc->an_rdata.an_tx_fids[i] = id;
		sc->an_rdata.an_tx_ring[i] = 0;
	}

	sc->an_rdata.an_tx_prod = 0;
	sc->an_rdata.an_tx_cons = 0;

	return(0);
}

static int an_init(ifp)
	struct ifnet *ifp;
{
	struct an_softc		*sc = (struct an_softc *)ifp->if_softc;

	if (ifp->if_flags & IFF_RUNNING)
		an_stop(ifp, 0);

	sc->an_associated = 0;

	if (!sc->sc_enabled) {
		if (sc->sc_enable)
			(*sc->sc_enable)(sc);
		sc->sc_enabled = 1;
	}

	/* Allocate the TX buffers */
	if (an_init_tx_ring(sc)) {
		an_reset(sc);
		if (an_init_tx_ring(sc)) {
			printf("%s: tx buffer allocation "
			    "failed\n", sc->an_dev.dv_xname);
			return ENOMEM;
		}
	}

	/* Set our MAC address. */
	bcopy((char *)&sc->an_caps.an_oemaddr,
	    (char *)&sc->an_config.an_macaddr, ETHER_ADDR_LEN);

	if (ifp->if_flags & IFF_BROADCAST)
		sc->an_config.an_rxmode = AN_RXMODE_BC_ADDR;
	else
		sc->an_config.an_rxmode = AN_RXMODE_ADDR;

	if (ifp->if_flags & IFF_MULTICAST)
		sc->an_config.an_rxmode = AN_RXMODE_BC_MC_ADDR;

	/* Initialize promisc mode. */
	/* Kills card DJA can't TX packet in sniff mode
 	if (ifp->if_flags & IFF_PROMISC)
		sc->an_config.an_rxmode |= AN_RXMODE_LAN_MONITOR_CURBSS;
	*/

	sc->an_rxmode = sc->an_config.an_rxmode;

	/* Set the ssid list */
	sc->an_ssidlist.an_type = AN_RID_SSIDLIST;
	sc->an_ssidlist.an_len = sizeof(struct an_ltv_ssidlist);
	if (an_write_record(sc, (struct an_ltv_gen *)&sc->an_ssidlist)) {
		printf("%s: failed to set ssid list\n", sc->an_dev.dv_xname);
		return ENXIO;
	}

	/* Set the AP list */
	sc->an_aplist.an_type = AN_RID_APLIST;
	sc->an_aplist.an_len = sizeof(struct an_ltv_aplist);
	if (an_write_record(sc, (struct an_ltv_gen *)&sc->an_aplist)) {
		printf("%s: failed to set AP list\n", sc->an_dev.dv_xname);
		return ENXIO;
	}

	/* Set the configuration in the NIC */
	sc->an_config.an_len = sizeof(struct an_ltv_genconfig);
	sc->an_config.an_type = AN_RID_GENCONFIG;
	if (an_write_record(sc, (struct an_ltv_gen *)&sc->an_config)) {
		printf("%s: failed to set configuration\n", sc->an_dev.dv_xname);
		return ENXIO;
	}

	/* Set the WEP Keys */
	if ((sc->an_config.an_authtype & ~AN_AUTHTYPE_MASK) != 0) {
		sc->an_temp_keys.an_len = sizeof(struct an_ltv_wepkey);
		sc->an_temp_keys.an_type = AN_RID_WEP_VOLATILE;
		an_write_record(sc, (struct an_ltv_gen *)&sc->an_temp_keys);
	}

	/* Enable the MAC */
	if (an_cmd(sc, AN_CMD_ENABLE, 0)) {
		printf("%s: failed to enable MAC\n", sc->an_dev.dv_xname);
		return ENXIO;
	}

	/* enable interrupts */
	CSR_WRITE_2(sc, AN_INT_EN, AN_INTRS);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	callout_reset(&sc->an_stat_ch, hz, an_stats_update, sc);
	return 0;
}

static void an_start(ifp)
	struct ifnet		*ifp;
{
	struct an_softc		*sc;
	struct mbuf		*m0 = NULL;
	struct an_txframe_802_3	tx_frame_802_3;
	struct ether_header	*eh;
	int			id;
	int			idx;
	unsigned char           txcontrol;

	sc = ifp->if_softc;

	if (!sc->sc_enabled)
		return;

	if (ifp->if_flags & IFF_OACTIVE)
		return;

	if (!sc->an_associated)
		return;

	idx = sc->an_rdata.an_tx_prod;
	bzero((char *)&tx_frame_802_3, sizeof(tx_frame_802_3));

	while(sc->an_rdata.an_tx_ring[idx] == 0) {
		IF_DEQUEUE(&ifp->if_snd, m0);
		if (m0 == NULL)
			break;

		id = sc->an_rdata.an_tx_fids[idx];
		eh = mtod(m0, struct ether_header *);

		bcopy((char *)&eh->ether_dhost,
		    (char *)&tx_frame_802_3.an_tx_dst_addr, ETHER_ADDR_LEN);
		bcopy((char *)&eh->ether_shost,
		    (char *)&tx_frame_802_3.an_tx_src_addr, ETHER_ADDR_LEN);
 
		tx_frame_802_3.an_tx_802_3_payload_len =
		  m0->m_pkthdr.len - 12;  /* minus src/dest mac & type */

                m_copydata(m0, sizeof(struct ether_header) - 2 ,
                    tx_frame_802_3.an_tx_802_3_payload_len,
                    (caddr_t)&sc->an_txbuf);

		txcontrol=AN_TXCTL_8023;
		/* write the txcontrol only */
		an_write_data(sc, id, 0x08, (caddr_t)&txcontrol,
			      sizeof(txcontrol));
		
		/* 802_3 header */
		an_write_data(sc, id, 0x34, (caddr_t)&tx_frame_802_3,
			      sizeof(struct an_txframe_802_3));
			
		/* in mbuf header type is just before payload */
		an_write_data(sc, id, 0x44, (caddr_t)&sc->an_txbuf,
			    tx_frame_802_3.an_tx_802_3_payload_len);
#if NBPFILTER > 0
		/*
		 * If there's a BPF listner, bounce a copy of
		 * this frame to him.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m0);
#endif

		m_freem(m0);
		m0 = NULL;

		/* TX START disable lan monitor ? DJA 
		   an_disable_sniff():
		 */
		sc->an_rdata.an_tx_ring[idx] = id;
		if (an_cmd(sc, AN_CMD_TX, id))
			printf("%s: xmit failed\n", sc->an_dev.dv_xname);

		AN_INC(idx, AN_TX_RING_CNT);
	}

	if (m0 != NULL)
		ifp->if_flags |= IFF_OACTIVE;

	sc->an_rdata.an_tx_prod = idx;

	/*
	 * Set a timeout in case the chip goes out to lunch.
	 */
	ifp->if_timer = 5;

	return;
}

void an_stop(ifp, disable)
	struct ifnet		*ifp;
	int disable;
{
	struct an_softc		*sc = (struct an_softc *)ifp->if_softc;
	int			i;

	callout_stop(&sc->an_stat_ch);
	if (!sc->sc_enabled)
		return;

	an_cmd(sc, AN_CMD_FORCE_SYNCLOSS, 0);
	CSR_WRITE_2(sc, AN_INT_EN, 0);
	an_cmd(sc, AN_CMD_DISABLE, 0);

	for (i = 0; i < AN_TX_RING_CNT; i++)
		an_cmd(sc, AN_CMD_DEALLOC_MEM, sc->an_rdata.an_tx_fids[i]);

	ifp->if_flags &= ~(IFF_RUNNING|IFF_OACTIVE);

	if (disable) {
		if (sc->sc_disable)
			(*sc->sc_disable)(sc);
		sc->sc_enabled = 0;
	}

	return;
}

static void an_watchdog(ifp)
	struct ifnet		*ifp;
{
	struct an_softc		*sc;

	sc = ifp->if_softc;
	if (!sc->sc_enabled)
		return;

	printf("%s: device timeout\n", sc->an_dev.dv_xname);

	an_reset(sc);
	an_init(ifp);

	ifp->if_oerrors++;
	return;
}

void an_shutdown(dev)
	struct device *		dev;
{
	struct an_softc		*sc = (struct an_softc *)dev;
	an_stop(&sc->arpcom.ec_if, 1);

	return;
}

#ifdef ANCACHE
/* Aironet signal strength cache code.
 * store signal/noise/quality on per MAC src basis in
 * a small fixed cache.  The cache wraps if > MAX slots
 * used.  The cache may be zeroed out to start over.
 * Two simple filters exist to reduce computation:
 * 1. ip only (literally 0x800) which may be used
 * to ignore some packets.  It defaults to ip only.
 * it could be used to focus on broadcast, non-IP 802.11 beacons.
 * 2. multicast/broadcast only.  This may be used to
 * ignore unicast packets and only cache signal strength
 * for multicast/broadcast packets (beacons); e.g., Mobile-IP
 * beacons and not unicast traffic.
 *
 * The cache stores (MAC src(index), IP src (major clue), signal,
 *	quality, noise)
 *
 * No apologies for storing IP src here.  It's easy and saves much
 * trouble elsewhere.  The cache is assumed to be INET dependent, 
 * although it need not be.
 *
 * Note: the Aironet only has a single byte of signal strength value
 * in the rx frame header, and it's not scaled to anything sensible.
 * This is kind of lame, but it's all we've got.
 */

#ifdef documentation

int an_sigitems;                                /* number of cached entries */
struct an_sigcache an_sigcache[MAXANCACHE];  /*  array of cache entries */
int an_nextitem;                                /*  index/# of entries */


#endif

/* control variables for cache filtering.  Basic idea is
 * to reduce cost (e.g., to only Mobile-IP agent beacons
 * which are broadcast or multicast).  Still you might
 * want to measure signal strength anth unicast ping packets
 * on a pt. to pt. ant. setup.
 */
/* set true if you want to limit cache items to broadcast/mcast 
 * only packets (not unicast).  Useful for mobile-ip beacons which
 * are broadcast/multicast at network layer.  Default is all packets
 * so ping/unicast anll work say anth pt. to pt. antennae setup.
 */
static int an_cache_mcastonly = 0;
#if 0
SYSCTL_INT(_machdep, OID_AUTO, an_cache_mcastonly, CTLFLAG_RW, 
	&an_cache_mcastonly, 0, "");
#endif

/* set true if you want to limit cache items to IP packets only
*/
static int an_cache_iponly = 1;
#if 0
SYSCTL_INT(_machdep, OID_AUTO, an_cache_iponly, CTLFLAG_RW, 
	&an_cache_iponly, 0, "");
#endif

/*
 * an_cache_store, per rx packet store signal
 * strength in MAC (src) indexed cache.
 */
static  
void an_cache_store (sc, eh, m, rx_quality)
	struct an_softc *sc;
	struct ether_header *eh;
	struct mbuf *m;
	unsigned short rx_quality;
{
	struct ip *ip = 0; 
	int i;
	static int cache_slot = 0; 	/* use this cache entry */
	static int wrapindex = 0;       /* next "free" cache entry */
	int saanp=0;

	/* filters:
	 * 1. ip only
	 * 2. configurable filter to throw out unicast packets,
	 * keep multicast only.
	 */
 
	if ((ntohs(eh->ether_type) == 0x800)) {
		saanp = 1;
	}

	/* filter for ip packets only 
	*/
	if ( an_cache_iponly && !saanp) {
		return;
	}

	/* filter for broadcast/multicast only
	 */
	if (an_cache_mcastonly && ((eh->ether_dhost[0] & 1) == 0)) {
		return;
	}

#ifdef SIGDEBUG
	printf("an: q value %x (MSB=0x%x, LSB=0x%x) \n",
	    rx_quality & 0xffff, rx_quality >> 8, rx_quality & 0xff);
#endif

	/* find the ip header.  we want to store the ip_src
	 * address.  
	 */
	if (saanp) {
		ip = mtod(m, struct ip *);
	}
        
	/* do a linear search for a matching MAC address 
	 * in the cache table
	 * . MAC address is 6 bytes,
	 * . var w_nextitem holds total number of entries already cached
	 */
	for(i = 0; i < sc->an_nextitem; i++) {
		if (! bcmp(eh->ether_shost , sc->an_sigcache[i].macsrc,  6 )) {
			/* Match!,
			 * so we already have this entry,
			 * update the data
			 */
			break;	
		}
	}

	/* did we find a matching mac address?
	 * if yes, then overwrite a previously existing cache entry
	 */
	if (i < sc->an_nextitem )   {
		cache_slot = i; 
	}
	/* else, have a new address entry,so
	 * add this new entry,
	 * if table full, then we need to replace LRU entry
	 */
	else    {                          

		/* check for space in cache table 
		 * note: an_nextitem also holds number of entries
		 * added in the cache table 
		 */
		if ( sc->an_nextitem < MAXANCACHE ) {
			cache_slot = sc->an_nextitem;
			sc->an_nextitem++;                 
			sc->an_sigitems = sc->an_nextitem;
		}
        	/* no space found, so simply wrap anth wrap index
		 * and "zap" the next entry
		 */
		else {
			if (wrapindex == MAXANCACHE) {
				wrapindex = 0;
			}
			cache_slot = wrapindex++;
		}
	}

	/* invariant: cache_slot now points at some slot
	 * in cache.
	 */
	if (cache_slot < 0 || cache_slot >= MAXANCACHE) {
		log(LOG_ERR, "an_cache_store, bad index: %d of "
		    "[0..%d], gross cache error\n",
		    cache_slot, MAXANCACHE);
		return;
	}

	/*  store items in cache
	 *  .ip source address
	 *  .mac src
	 *  .signal, etc.
	 */
	if (saanp) {
		sc->an_sigcache[cache_slot].ipsrc = ip->ip_src.s_addr;
	}
	bcopy( eh->ether_shost, sc->an_sigcache[cache_slot].macsrc,  6);

	sc->an_sigcache[cache_slot].signal = rx_quality;

	return;
}
#endif

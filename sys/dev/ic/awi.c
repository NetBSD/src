/*	$NetBSD: awi.c,v 1.30.2.4 2002/06/24 22:09:55 nathanw Exp $	*/

/*-
 * Copyright (c) 1999,2000,2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Bill Sommerfeld
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
 * Driver for AMD 802.11 firmware.
 * Uses am79c930 chip driver to talk to firmware running on the am79c930.
 *
 * More-or-less a generic ethernet-like if driver, with 802.11 gorp added.
 */

/*
 * todo:
 *	- flush tx queue on resynch.
 *	- clear oactive on "down".
 *	- rewrite copy-into-mbuf code
 *	- mgmt state machine gets stuck retransmitting assoc requests.
 *	- multicast filter.
 *	- fix device reset so it's more likely to work
 *	- show status goo through ifmedia.
 *
 * more todo:
 *	- deal with more 802.11 frames.
 *		- send reassoc request
 *		- deal with reassoc response
 *		- send/deal with disassociation
 *	- deal with "full" access points (no room for me).
 *	- power save mode
 *
 * later:
 *	- SSID preferences
 *	- need ioctls for poking at the MIBs
 *	- implement ad-hoc mode (including bss creation).
 *	- decide when to do "ad hoc" vs. infrastructure mode (IFF_LINK flags?)
 *		(focus on inf. mode since that will be needed for ietf)
 *	- deal with DH vs. FH versions of the card
 *	- deal with faster cards (2mb/s)
 *	- ?WEP goo (mmm, rc4) (it looks not particularly useful).
 *	- ifmedia revision.
 *	- common 802.11 mibish things.
 *	- common 802.11 media layer.
 */

/*
 * Driver for AMD 802.11 PCnetMobile firmware.
 * Uses am79c930 chip driver to talk to firmware running on the am79c930.
 *
 * The initial version of the driver was written by
 * Bill Sommerfeld <sommerfeld@netbsd.org>.
 * Then the driver module completely rewritten to support cards with DS phy
 * and to support adhoc mode by Atsushi Onoe <onoe@netbsd.org>
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: awi.c,v 1.30.2.4 2002/06/24 22:09:55 nathanw Exp $");

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/if_llc.h>
#include <net/if_ieee80211.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#ifdef __NetBSD__
#include <netinet/if_inarp.h>
#else
#include <netinet/if_ether.h>
#endif
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/am79c930reg.h>
#include <dev/ic/am79c930var.h>
#include <dev/ic/awireg.h>
#include <dev/ic/awivar.h>

static int  awi_init(struct ifnet *);
static void awi_stop(struct ifnet *, int);
static void awi_start(struct ifnet *);
static void awi_watchdog(struct ifnet *);
static int  awi_ioctl(struct ifnet *, u_long, caddr_t);
static int  awi_media_change(struct ifnet *);
static void awi_media_status(struct ifnet *, struct ifmediareq *);
static int  awi_mode_init(struct awi_softc *);
static int  awi_media_rate2opt(struct awi_softc *, int);
static int  awi_media_opt2rate(struct awi_softc *, int);
static void awi_rx_int(struct awi_softc *);
static void awi_tx_int(struct awi_softc *);
static struct mbuf *awi_devget(struct awi_softc *, u_int32_t, u_int16_t);
static int  awi_hw_init(struct awi_softc *);
static int  awi_init_mibs(struct awi_softc *);
static int  awi_chan_check(void *, u_char *);
static int  awi_mib(struct awi_softc *, u_int8_t, u_int8_t, int);
static int  awi_cmd(struct awi_softc *, u_int8_t, int);
static int  awi_cmd_wait(struct awi_softc *);
static void awi_cmd_done(struct awi_softc *);
static int  awi_next_txd(struct awi_softc *, int, u_int32_t *, u_int32_t *);
static int  awi_lock(struct awi_softc *);
static void awi_unlock(struct awi_softc *);
static int  awi_intr_lock(struct awi_softc *);
static void awi_intr_unlock(struct awi_softc *);
static int  awi_newstate(void *, enum ieee80211_state);
static struct mbuf *awi_ether_encap(struct awi_softc *, struct mbuf *);
static struct mbuf *awi_ether_modcap(struct awi_softc *, struct mbuf *);

/* unalligned little endian access */     
#define LE_READ_2(p)							\
	((((u_int8_t *)(p))[0]      ) | (((u_int8_t *)(p))[1] <<  8))
#define LE_READ_4(p)							\
	((((u_int8_t *)(p))[0]      ) | (((u_int8_t *)(p))[1] <<  8) |	\
	 (((u_int8_t *)(p))[2] << 16) | (((u_int8_t *)(p))[3] << 24))
#define LE_WRITE_2(p, v)						\
	((((u_int8_t *)(p))[0] = (((u_int32_t)(v)      ) & 0xff)),	\
	 (((u_int8_t *)(p))[1] = (((u_int32_t)(v) >>  8) & 0xff)))
#define LE_WRITE_4(p, v)						\
	((((u_int8_t *)(p))[0] = (((u_int32_t)(v)      ) & 0xff)),	\
	 (((u_int8_t *)(p))[1] = (((u_int32_t)(v) >>  8) & 0xff)),	\
	 (((u_int8_t *)(p))[2] = (((u_int32_t)(v) >> 16) & 0xff)),	\
	 (((u_int8_t *)(p))[3] = (((u_int32_t)(v) >> 24) & 0xff)))

struct awi_chanset awi_chanset[] = {
    /* PHY type        domain            min max def */
    { AWI_PHY_TYPE_FH, AWI_REG_DOMAIN_JP,  6, 17,  6 },
    { AWI_PHY_TYPE_FH, AWI_REG_DOMAIN_ES,  0, 26,  1 },
    { AWI_PHY_TYPE_FH, AWI_REG_DOMAIN_FR,  0, 32,  1 },
    { AWI_PHY_TYPE_FH, AWI_REG_DOMAIN_US,  0, 77,  1 },
    { AWI_PHY_TYPE_FH, AWI_REG_DOMAIN_CA,  0, 77,  1 },
    { AWI_PHY_TYPE_FH, AWI_REG_DOMAIN_EU,  0, 77,  1 },
    { AWI_PHY_TYPE_DS, AWI_REG_DOMAIN_JP, 14, 14, 14 },
    { AWI_PHY_TYPE_DS, AWI_REG_DOMAIN_ES, 10, 11, 10 },
    { AWI_PHY_TYPE_DS, AWI_REG_DOMAIN_FR, 10, 13, 10 },
    { AWI_PHY_TYPE_DS, AWI_REG_DOMAIN_US,  1, 11,  3 },
    { AWI_PHY_TYPE_DS, AWI_REG_DOMAIN_CA,  1, 11,  3 },
    { AWI_PHY_TYPE_DS, AWI_REG_DOMAIN_EU,  1, 13,  3 },
    { 0, 0 }
};

#ifdef AWI_DEBUG
int awi_debug;

#define	DPRINTF(X)	if (awi_debug) printf X
#define	DPRINTF2(X)	if (awi_debug > 1) printf X
#else
#define	DPRINTF(X)
#define	DPRINTF2(X)
#endif

int
awi_attach(struct awi_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &ic->ic_if;
	int s, i, error, nrate;
	int mword;
	struct ifmediareq imr;

	s = splnet();
	sc->sc_busy = 1;
	ic->ic_state = IEEE80211_S_INIT;
	sc->sc_substate = AWI_ST_NONE;
	if ((error = awi_hw_init(sc)) != 0) {
		sc->sc_invalid = 1;
		splx(s);
		return error;
	}
	error = awi_init_mibs(sc);
	if (error != 0) {
		sc->sc_invalid = 1;
		splx(s);
		return error;
	}
	ifp->if_softc = sc;
	ifp->if_flags =
	    IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST | IFF_NOTRAILERS;
	ifp->if_ioctl = awi_ioctl;
	ifp->if_start = awi_start;
	ifp->if_init = awi_init;
	ifp->if_stop = awi_stop;
	ifp->if_watchdog = awi_watchdog;
	IFQ_SET_READY(&ifp->if_snd);
	memcpy(ifp->if_xname, sc->sc_dev.dv_xname, IFNAMSIZ);

	ic->ic_flags = IEEE80211_F_HASWEP | IEEE80211_F_HASIBSS;
	ic->ic_newstate = awi_newstate;
	ic->ic_chancheck = awi_chan_check;
	nrate = sc->sc_mib_phy.aSuprt_Data_Rates[1];
	memcpy(ic->ic_sup_rates, sc->sc_mib_phy.aSuprt_Data_Rates + 2, nrate);
	memcpy(ic->ic_myaddr, sc->sc_mib_addr.aMAC_Address, IEEE80211_ADDR_LEN);

	printf("%s: IEEE802.11 %s %dMbps (firmware %s)\n",
	    sc->sc_dev.dv_xname,
	    sc->sc_mib_phy.IEEE_PHY_Type == AWI_PHY_TYPE_FH ? "FH" : "DS",
	    (ic->ic_sup_rates[nrate - 1] & IEEE80211_RATE_VAL) / 2,
	    sc->sc_banner);
	printf("%s: 802.11 address: %s\n", sc->sc_dev.dv_xname,
	    ether_sprintf(ic->ic_myaddr));

	if_attach(ifp);
	ieee80211_ifattach(ifp);

	ifmedia_init(&sc->sc_media, 0, awi_media_change, awi_media_status);
	mword = IFM_MAKEWORD(IFM_IEEE80211, IFM_AUTO, 0, 0);
	ifmedia_add(&sc->sc_media, mword, 0, NULL);
	ifmedia_add(&sc->sc_media, mword | IFM_FLAG0, 0, NULL);
	mword |= IFM_IEEE80211_ADHOC;
	ifmedia_add(&sc->sc_media, mword, 0, NULL);
	ifmedia_add(&sc->sc_media, mword | IFM_FLAG0, 0, NULL);
	for (i = 0; i < nrate; i++) {
		mword = awi_media_rate2opt(sc, ic->ic_sup_rates[i]);
		if (mword == 0)
			continue;
		mword |= IFM_IEEE80211;
		ifmedia_add(&sc->sc_media, mword, 0, NULL);
		ifmedia_add(&sc->sc_media, mword | IFM_FLAG0, 0, NULL);
		mword |= IFM_IEEE80211_ADHOC;
		ifmedia_add(&sc->sc_media, mword, 0, NULL);
		if (sc->sc_mib_phy.IEEE_PHY_Type != AWI_PHY_TYPE_FH)
			ifmedia_add(&sc->sc_media, mword | IFM_FLAG0, 0, NULL);
	}
	awi_media_status(ifp, &imr);
	ifmedia_set(&sc->sc_media, imr.ifm_active);

	if ((sc->sc_sdhook = shutdownhook_establish(awi_shutdown, sc)) == NULL)
		printf("%s: WARNING: unable to establish shutdown hook\n",
		    sc->sc_dev.dv_xname);
	if ((sc->sc_powerhook = powerhook_establish(awi_power, sc)) == NULL)
		printf("%s: WARNING: unable to establish power hook\n",
		    sc->sc_dev.dv_xname);
	sc->sc_attached = 1;
	splx(s);

	/* ready to accept ioctl */
	awi_unlock(sc);

	return 0;
}

int
awi_detach(struct awi_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ic.ic_if;
	int s;

	if (!sc->sc_attached)
		return 0;

	s = splnet();
	sc->sc_invalid = 1;
	awi_stop(ifp, 1);
	while (sc->sc_sleep_cnt > 0) {
		wakeup(sc);
		(void)tsleep(sc, PWAIT, "awidet", 1);
	}
	ifmedia_delete_instance(&sc->sc_media, IFM_INST_ANY);
	ieee80211_ifdetach(ifp);
	if_detach(ifp);
	shutdownhook_disestablish(sc->sc_sdhook);
	powerhook_disestablish(sc->sc_powerhook);
	splx(s);
	return 0;
}

int
awi_activate(struct device *self, enum devact act)
{
	struct awi_softc *sc = (struct awi_softc *)self;
	struct ifnet *ifp = &sc->sc_ic.ic_if;
	int s, error = 0;

	s = splnet();
	switch (act) {
	case DVACT_ACTIVATE:
		error = EOPNOTSUPP;
		break;
	case DVACT_DEACTIVATE:
		sc->sc_invalid = 1;
		if_deactivate(ifp);
		break;
	}
	splx(s);
	return error;
}

void
awi_power(int why, void *arg)
{
	struct awi_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ic.ic_if;
	int s;
	int ocansleep;

	DPRINTF(("awi_power: %d\n", why));
	s = splnet();
	ocansleep = sc->sc_cansleep;
	sc->sc_cansleep = 0;
	switch (why) {
	case PWR_SUSPEND:
	case PWR_STANDBY:
		awi_stop(ifp, 1);
		break;
	case PWR_RESUME:
		if (ifp->if_flags & IFF_UP) {
			awi_init(ifp);
			(void)awi_intr(sc);	/* make sure */
		}
		break;
	case PWR_SOFTSUSPEND:
	case PWR_SOFTSTANDBY:
	case PWR_SOFTRESUME:
		break;
	}
	sc->sc_cansleep = ocansleep;
	splx(s);
}

void
awi_shutdown(void *arg)
{
	struct awi_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ic.ic_if;

	if (sc->sc_attached)
		awi_stop(ifp, 1);
}

int
awi_intr(void *arg)
{
	struct awi_softc *sc = arg;
	u_int16_t status;
	int error, handled = 0, ocansleep;
#ifdef AWI_DEBUG
	static const char *intname[] = {
	    "CMD", "RX", "TX", "SCAN_CMPLT",
	    "CFP_START", "DTIM", "CFP_ENDING", "GROGGY",
	    "TXDATA", "TXBCAST", "TXPS", "TXCF",
	    "TXMGT", "#13", "RXDATA", "RXMGT"
	};
#endif

	if (!sc->sc_enabled || !sc->sc_enab_intr || sc->sc_invalid)
		return 0;

	am79c930_gcr_setbits(&sc->sc_chip,
	    AM79C930_GCR_DISPWDN | AM79C930_GCR_ECINT);
	awi_write_1(sc, AWI_DIS_PWRDN, 1);
	ocansleep = sc->sc_cansleep;
	sc->sc_cansleep = 0;

	for (;;) {
		if ((error = awi_intr_lock(sc)) != 0)
			break;
		status = awi_read_1(sc, AWI_INTSTAT);
		awi_write_1(sc, AWI_INTSTAT, 0);
		awi_write_1(sc, AWI_INTSTAT, 0);
		status |= awi_read_1(sc, AWI_INTSTAT2) << 8;
		awi_write_1(sc, AWI_INTSTAT2, 0);
		DELAY(10);
		awi_intr_unlock(sc);
		if (!sc->sc_cmd_inprog)
			status &= ~AWI_INT_CMD;	/* make sure */
		if (status == 0)
			break;
#ifdef AWI_DEBUG
		if (awi_debug > 1) {
			int i;

			printf("awi_intr: status 0x%04x", status);
			for (i = 0; i < sizeof(intname)/sizeof(intname[0]);
			    i++) {
				if (status & (1 << i))
					printf(" %s", intname[i]);
			}
			printf("\n");
		}
#endif
		handled = 1;
		if (status & AWI_INT_RX)
			awi_rx_int(sc);
		if (status & AWI_INT_TX)
			awi_tx_int(sc);
		if (status & AWI_INT_CMD)
			awi_cmd_done(sc);
		if (status & AWI_INT_SCAN_CMPLT) {
			if (sc->sc_ic.ic_state == IEEE80211_S_SCAN)
				ieee80211_next_scan(&sc->sc_ic.ic_if);
		}
	}
	sc->sc_cansleep = ocansleep;
	am79c930_gcr_clearbits(&sc->sc_chip, AM79C930_GCR_DISPWDN);
	awi_write_1(sc, AWI_DIS_PWRDN, 0);
	return handled;
}

static int
awi_init(struct ifnet *ifp)
{
	struct awi_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_bss *bs = &ic->ic_bss;
	int i, error;

	DPRINTF(("awi_init: enabled=%d\n", sc->sc_enabled));
	if (sc->sc_enabled) {
		awi_stop(ifp, 0);
	} else {
		if (sc->sc_enable)
			(*sc->sc_enable)(sc);
		sc->sc_enabled = 1;
		if ((error = awi_hw_init(sc)) != 0) {
			awi_stop(ifp, 1);
			return error;
		}
	}
	ic->ic_state = IEEE80211_S_INIT;

	sc->sc_mib_local.Network_Mode =
	    (ic->ic_flags & IEEE80211_F_ADHOC) ? 0 : 1;

	if ((error = awi_mode_init(sc)) != 0) {
		DPRINTF(("awi_init: awi_mode_init failed %d\n", error));
		awi_stop(ifp, 1);
		return error;
	}

	/* start transmitter */
	sc->sc_txdone = sc->sc_txnext = sc->sc_txbase;
	awi_write_4(sc, sc->sc_txbase + AWI_TXD_START, 0);
	awi_write_4(sc, sc->sc_txbase + AWI_TXD_NEXT, 0);
	awi_write_4(sc, sc->sc_txbase + AWI_TXD_LENGTH, 0);
	awi_write_1(sc, sc->sc_txbase + AWI_TXD_RATE, 0);
	awi_write_4(sc, sc->sc_txbase + AWI_TXD_NDA, 0);
	awi_write_4(sc, sc->sc_txbase + AWI_TXD_NRA, 0);
	awi_write_1(sc, sc->sc_txbase + AWI_TXD_STATE, 0);
	awi_write_4(sc, AWI_CA_TX_DATA, sc->sc_txbase);
	awi_write_4(sc, AWI_CA_TX_MGT, 0);
	awi_write_4(sc, AWI_CA_TX_BCAST, 0);
	awi_write_4(sc, AWI_CA_TX_PS, 0);
	awi_write_4(sc, AWI_CA_TX_CF, 0);
	if ((error = awi_cmd(sc, AWI_CMD_INIT_TX, AWI_WAIT)) != 0) {
		DPRINTF(("awi_init: failed to start transmitter: %d\n", error));
		awi_stop(ifp, 1);
		return error;
	}

	/* start receiver */
	if ((error = awi_cmd(sc, AWI_CMD_INIT_RX, AWI_WAIT)) != 0) {
		DPRINTF(("awi_init: failed to start receiver: %d\n", error));
		awi_stop(ifp, 1);
		return error;
	}
	sc->sc_rxdoff = awi_read_4(sc, AWI_CA_IRX_DATA_DESC);
	sc->sc_rxmoff = awi_read_4(sc, AWI_CA_IRX_PS_DESC);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	if ((sc->sc_ic.ic_flags & IEEE80211_F_ADHOC) && sc->sc_no_bssid) {
		bs->bs_chan = ic->ic_ibss_chan;
		bs->bs_intval = ic->ic_lintval;
		bs->bs_nrate = 0;
		for (i = 0; i < IEEE80211_RATE_SIZE; i++) {
			if (ic->ic_sup_rates[i])
				bs->bs_rates[bs->bs_nrate++] =
				    ic->ic_sup_rates[i];
		}
		memcpy(bs->bs_macaddr, ic->ic_myaddr, IEEE80211_ADDR_LEN);
		memset(bs->bs_bssid, 0, IEEE80211_ADDR_LEN);
		bs->bs_esslen = 0;
		ic->ic_flags |= IEEE80211_F_SIBSS;
		ic->ic_state = IEEE80211_S_SCAN;	/*XXX*/
		sc->sc_substate = AWI_ST_NONE;
		ieee80211_new_state(&ic->ic_if, IEEE80211_S_RUN, -1);
	} else {
		bs->bs_chan = sc->sc_cur_chan;
		ieee80211_new_state(&ic->ic_if, IEEE80211_S_SCAN, -1);
	}
	return 0;
}

static void
awi_stop(struct ifnet *ifp, int disable)
{
	struct awi_softc *sc = ifp->if_softc;

	if (!sc->sc_enabled)
		return;

	DPRINTF(("awi_stop(%d)\n", disable));

	ieee80211_new_state(&sc->sc_ic.ic_if, IEEE80211_S_INIT, -1);

	if (!sc->sc_invalid) {
		if (sc->sc_cmd_inprog)
			(void)awi_cmd_wait(sc);
		(void)awi_cmd(sc, AWI_CMD_KILL_RX, AWI_WAIT);
		sc->sc_cmd_inprog = AWI_CMD_FLUSH_TX;
		awi_write_1(sc, AWI_CA_FTX_DATA, 1);
		awi_write_1(sc, AWI_CA_FTX_MGT, 0);
		awi_write_1(sc, AWI_CA_FTX_BCAST, 0);
		awi_write_1(sc, AWI_CA_FTX_PS, 0);
		awi_write_1(sc, AWI_CA_FTX_CF, 0);
		(void)awi_cmd(sc, AWI_CMD_FLUSH_TX, AWI_WAIT);
	}
	ifp->if_flags &= ~(IFF_RUNNING|IFF_OACTIVE);
	ifp->if_timer = 0;
	sc->sc_tx_timer = sc->sc_rx_timer = 0;
	if (sc->sc_rxpend != NULL) {
		m_freem(sc->sc_rxpend);
		sc->sc_rxpend = NULL;
	}
	IFQ_PURGE(&ifp->if_snd);

	if (disable) {
		if (sc->sc_disable)
			(*sc->sc_disable)(sc);
		sc->sc_enabled = 0;
	}
}

static void
awi_start(struct ifnet *ifp)
{
	struct awi_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct mbuf *m, *m0;
	int len;
	u_int32_t txd, frame, ntxd;
	u_int8_t rate;

	if (!sc->sc_enabled || sc->sc_invalid)
		return;

	for (;;) {
		txd = sc->sc_txnext;
		IF_POLL(&ic->ic_mgtq, m0);
		if (m0 != NULL) {
			if (awi_next_txd(sc, m0->m_pkthdr.len, &frame, &ntxd)) {
				ifp->if_flags |= IFF_OACTIVE;
				break;
			}
			IF_DEQUEUE(&ic->ic_mgtq, m0);
		} else {
			if (ic->ic_state != IEEE80211_S_RUN)
				break;
			IFQ_POLL(&ifp->if_snd, m0);
			if (m0 == NULL)
				break;
			/*
			 * Need to calculate the real length to determine
			 * if the transmit buffer has a room for the packet.
			 */
			len = m0->m_pkthdr.len + sizeof(struct ieee80211_frame);
			if (!(ifp->if_flags & IFF_LINK0) && !sc->sc_adhoc_ap)
				len += sizeof(struct llc) -
				    sizeof(struct ether_header);
			if (ic->ic_flags & IEEE80211_F_WEPON)
				len += IEEE80211_WEP_IVLEN +
				    IEEE80211_WEP_KIDLEN + IEEE80211_WEP_CRCLEN;
			if (awi_next_txd(sc, len, &frame, &ntxd)) {
				ifp->if_flags |= IFF_OACTIVE;
				break;
			}
			IFQ_DEQUEUE(&ifp->if_snd, m0);
			ifp->if_opackets++;
#if NBPFILTER > 0
			if (ifp->if_bpf)
				bpf_mtap(ifp->if_bpf, m0);
#endif
			if ((ifp->if_flags & IFF_LINK0) || sc->sc_adhoc_ap)
				m0 = awi_ether_encap(sc, m0);
			else
				m0 = ieee80211_encap(ifp, m0);
			if ((ic->ic_flags & IEEE80211_F_WEPON) && m0 != NULL)
				m0 = ieee80211_wep_crypt(ifp, m0, 1);
			if (m0 == NULL) {
				ifp->if_oerrors++;
				continue;
			}
#ifdef DIAGNOSTIC
			if (m0->m_pkthdr.len != len) {
				printf("%s: length %d should be %d\n",
				    ifp->if_xname, m0->m_pkthdr.len, len);
				m_freem(m0);
				ifp->if_oerrors++;
				continue;
			}
#endif
		}

		if ((ifp->if_flags & IFF_DEBUG) && (ifp->if_flags & IFF_LINK2))
			ieee80211_dump_pkt(m0->m_data, m0->m_len,
			    ic->ic_bss.bs_rates[ic->ic_bss.bs_txrate] &
			    IEEE80211_RATE_VAL, -1);

		for (m = m0, len = 0; m != NULL; m = m->m_next) {
			awi_write_bytes(sc, frame + len, mtod(m, u_int8_t *),
			    m->m_len);
			len += m->m_len;
		}
		m_freem(m0);
		rate = (ic->ic_bss.bs_rates[ic->ic_bss.bs_txrate] &
		    IEEE80211_RATE_VAL) * 5;
		awi_write_1(sc, ntxd + AWI_TXD_STATE, 0);
		awi_write_4(sc, txd + AWI_TXD_START, frame);
		awi_write_4(sc, txd + AWI_TXD_NEXT, ntxd);
		awi_write_4(sc, txd + AWI_TXD_LENGTH, len);
		awi_write_1(sc, txd + AWI_TXD_RATE, rate);
		awi_write_4(sc, txd + AWI_TXD_NDA, 0);
		awi_write_4(sc, txd + AWI_TXD_NRA, 0);
		awi_write_1(sc, txd + AWI_TXD_STATE, AWI_TXD_ST_OWN);
		sc->sc_txnext = ntxd;

		sc->sc_tx_timer = 5;
		ifp->if_timer = 1;
	}
}

static void
awi_watchdog(struct ifnet *ifp)
{
	struct awi_softc *sc = ifp->if_softc;
	u_int32_t prevdone;
	int ocansleep;

	ifp->if_timer = 0;
	if (!sc->sc_enabled || sc->sc_invalid)
		return;

	ocansleep = sc->sc_cansleep;
	sc->sc_cansleep = 0;
	if (sc->sc_tx_timer) {
		if (--sc->sc_tx_timer == 0) {
			printf("%s: device timeout\n", ifp->if_xname);
			prevdone = sc->sc_txdone;
			awi_tx_int(sc);
			if (sc->sc_txdone == prevdone) {
				ifp->if_oerrors++;
				awi_init(ifp);
				goto out;
			}
		}
		ifp->if_timer = 1;
	}
	if (sc->sc_rx_timer) {
		if (--sc->sc_rx_timer == 0) {
			if (sc->sc_ic.ic_state == IEEE80211_S_RUN) {
				ieee80211_new_state(ifp, IEEE80211_S_SCAN, -1);
				goto out;
			}
		} else
			ifp->if_timer = 1;
	}
	/* TODO: rate control */
	ieee80211_watchdog(ifp);
  out:
	sc->sc_cansleep = ocansleep;
}

static int
awi_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct awi_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error;

	s = splnet();
	/* serialize ioctl, since we may sleep */
	if ((error = awi_lock(sc)) != 0)
		goto cantlock;

	switch (cmd) {
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			if (sc->sc_enabled) {
				/*
				 * To avoid rescanning another access point,
				 * do not call awi_init() here.  Instead,
				 * only reflect promisc mode settings.
				 */
				error = awi_mode_init(sc);
			} else
				error = awi_init(ifp);
		} else if (sc->sc_enabled)
			awi_stop(ifp, 1);
		break;
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_media, cmd);
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		error = (cmd == SIOCADDMULTI) ?
		    ether_addmulti(ifr, &sc->sc_ic.ic_ec) :
		    ether_delmulti(ifr, &sc->sc_ic.ic_ec);
		if (error == ENETRESET) {
			/* do not rescan */
			if (sc->sc_enabled)
				error = awi_mode_init(sc);
			else
				error = 0;
		}
		break;
	default:
		error = ieee80211_ioctl(ifp, cmd, data);
		if (error == ENETRESET) {
			if (sc->sc_enabled)
				error = awi_init(ifp);
			else
				error = 0;
		}
		break;
	}
	awi_unlock(sc);
  cantlock:
	splx(s);
	return error;
}

/*
 * Called from ifmedia_ioctl via awi_ioctl with lock obtained.
 */
static int
awi_media_change(struct ifnet *ifp)
{
	struct awi_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifmedia_entry *ime;
	int i, rate, error = 0;

	ime = sc->sc_media.ifm_cur;
	if (IFM_SUBTYPE(ime->ifm_media) == IFM_AUTO) {
		ic->ic_fixed_rate = -1;
	} else {
		rate = awi_media_opt2rate(sc, ime->ifm_media);
		if (rate == 0)
			return EINVAL;
		for (i = 0; i < IEEE80211_RATE_SIZE; i++) {
			if ((ic->ic_sup_rates[i] & IEEE80211_RATE_VAL) == rate)
				break;
		}
		if (i == IEEE80211_RATE_SIZE)
			return EINVAL;
		ic->ic_fixed_rate = i;
	}

	/*
	 *  ADHOC,-FLAG0	ADHOC,  !no_bssid, !adhoc_ap	IBSS
	 *  ADHOC, FLAG0	ADHOC    no_bssid, !adhoc_ap	WaveLAN adhoc
	 * -ADHOC,-FLAG0	~ADHOC, !no_bssid, !adhoc_ap	Infra
	 * -ADHOC, FLAG0	ADHOC,  !no_bssid,  adhoc_ap	Melco old AP
	 *						also LINK0
	 */
	if (ime->ifm_media & IFM_IEEE80211_ADHOC) {
		if ((ic->ic_flags & IEEE80211_F_ADHOC) == 0) {
			ic->ic_flags |= IEEE80211_F_ADHOC;
			error = ENETRESET;
		}
		ic->ic_flags |= IEEE80211_F_IBSSON;
		if (sc->sc_mib_phy.IEEE_PHY_Type != AWI_PHY_TYPE_FH &&
		    (ime->ifm_media & IFM_FLAG0)) {
			if (sc->sc_no_bssid == 0) {
				sc->sc_no_bssid = 1;
				error = ENETRESET;
			}
		} else {
			if (sc->sc_no_bssid) {
				sc->sc_no_bssid = 0;
				error = ENETRESET;
			}
		}
		if (sc->sc_adhoc_ap) {
			sc->sc_adhoc_ap = 0;
			error = ENETRESET;
		}
	} else {
		ic->ic_flags &= ~IEEE80211_F_IBSSON;
		if (sc->sc_no_bssid) {
			sc->sc_no_bssid = 0;
			error = ENETRESET;
		}
		if (ime->ifm_media & IFM_FLAG0) {
			if ((ic->ic_flags & IEEE80211_F_ADHOC) == 0) {
				ic->ic_flags |= IEEE80211_F_ADHOC;
				error = ENETRESET;
			}
			if (!sc->sc_adhoc_ap) {
				sc->sc_adhoc_ap = 1;
				error = ENETRESET;
			}
		} else {
			if (ic->ic_flags & IEEE80211_F_ADHOC) {
				ic->ic_flags &= ~IEEE80211_F_ADHOC;
				error = ENETRESET;
			}
			if (sc->sc_adhoc_ap) {
				sc->sc_adhoc_ap = 0;
				error = ENETRESET;
			}
		}
	}
	if (error == ENETRESET) {
		if (sc->sc_enabled)
			error = awi_init(ifp);
		else
			error = 0;
	}
	return error;
}

static void
awi_media_status(struct ifnet *ifp, struct ifmediareq *imr)
{
	struct awi_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	int rate;

	imr->ifm_status = IFM_AVALID;
	if (ic->ic_state == IEEE80211_S_RUN)
		imr->ifm_status |= IFM_ACTIVE;
	imr->ifm_active = IFM_IEEE80211;
	if (ic->ic_state == IEEE80211_S_RUN)
		rate = ic->ic_bss.bs_rates[ic->ic_bss.bs_txrate] &
		    IEEE80211_RATE_VAL;
	else {
		if (ic->ic_fixed_rate == -1)
			rate = 0;
		else
			rate = ic->ic_sup_rates[ic->ic_fixed_rate] &
			    IEEE80211_RATE_VAL;
	}
	imr->ifm_active |= awi_media_rate2opt(sc, rate);
	if (ic->ic_flags & IEEE80211_F_ADHOC) {
		if (sc->sc_adhoc_ap)
			imr->ifm_active |= IFM_FLAG0;
		else {
			imr->ifm_active |= IFM_IEEE80211_ADHOC;
			if (sc->sc_no_bssid)
				imr->ifm_active |= IFM_FLAG0;
		}
	}
}

static int
awi_mode_init(struct awi_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ic.ic_if;
	int n, error;
	struct ether_multi *enm;
	struct ether_multistep step;

	/* reinitialize muticast filter */
	n = 0;
	sc->sc_mib_local.Accept_All_Multicast_Dis = 0;
	if (ifp->if_flags & IFF_PROMISC) {
		sc->sc_mib_mac.aPromiscuous_Enable = 1;
		goto set_mib;
	}
	sc->sc_mib_mac.aPromiscuous_Enable = 0;
	ETHER_FIRST_MULTI(step, &sc->sc_ic.ic_ec, enm);
	while (enm != NULL) {
		if (n == AWI_GROUP_ADDR_SIZE ||
		    memcmp(enm->enm_addrlo, enm->enm_addrhi, IEEE80211_ADDR_LEN)
		    != 0)
			goto set_mib;
		memcpy(sc->sc_mib_addr.aGroup_Addresses[n], enm->enm_addrlo,
		    IEEE80211_ADDR_LEN);
		n++;
		ETHER_NEXT_MULTI(step, enm);
	}
	for (; n < AWI_GROUP_ADDR_SIZE; n++)
		memset(sc->sc_mib_addr.aGroup_Addresses[n], 0, IEEE80211_ADDR_LEN);
	sc->sc_mib_local.Accept_All_Multicast_Dis = 1;

  set_mib:
	if (sc->sc_mib_local.Accept_All_Multicast_Dis)
		ifp->if_flags &= ~IFF_ALLMULTI;
	else
		ifp->if_flags |= IFF_ALLMULTI;
	sc->sc_mib_mgt.Wep_Required =
	    (sc->sc_ic.ic_flags & IEEE80211_F_WEPON) ? 1 : 0;

	if ((error = awi_mib(sc, AWI_CMD_SET_MIB, AWI_MIB_LOCAL, AWI_WAIT)) ||
	    (error = awi_mib(sc, AWI_CMD_SET_MIB, AWI_MIB_ADDR, AWI_WAIT)) ||
	    (error = awi_mib(sc, AWI_CMD_SET_MIB, AWI_MIB_MAC, AWI_WAIT)) ||
	    (error = awi_mib(sc, AWI_CMD_SET_MIB, AWI_MIB_MGT, AWI_WAIT)) ||
	    (error = awi_mib(sc, AWI_CMD_SET_MIB, AWI_MIB_PHY, AWI_WAIT))) {
		DPRINTF(("awi_mode_init: MIB set failed: %d\n", error));
		return error;
	}
	return 0;
}

/* XXX should be moved to if_ieee80211subr.c ? */
static int
awi_media_rate2opt(struct awi_softc *sc, int rate)
{
	int mword;

	mword = 0;
	switch (rate & IEEE80211_RATE_VAL) {
	case 2:
		if (sc->sc_mib_phy.IEEE_PHY_Type == AWI_PHY_TYPE_FH)
			mword = IFM_IEEE80211_FH1;
		else
			mword = IFM_IEEE80211_DS1;
		break;
	case 4:
		if (sc->sc_mib_phy.IEEE_PHY_Type == AWI_PHY_TYPE_FH)
			mword = IFM_IEEE80211_FH2;
		else
			mword = IFM_IEEE80211_DS2;
		break;
	case 11:
		if (sc->sc_mib_phy.IEEE_PHY_Type == AWI_PHY_TYPE_DS)
			mword = IFM_IEEE80211_DS5;
		break;
	case 22:
		if (sc->sc_mib_phy.IEEE_PHY_Type == AWI_PHY_TYPE_DS)
			mword = IFM_IEEE80211_DS11;
		break;
	}
	return mword;
}

static int
awi_media_opt2rate(struct awi_softc *sc, int opt)
{
	int rate;

	rate = 0;
	switch (IFM_SUBTYPE(opt)) {
	case IFM_IEEE80211_FH1:
	case IFM_IEEE80211_FH2:
		if (sc->sc_mib_phy.IEEE_PHY_Type != AWI_PHY_TYPE_FH)
			return 0;
		break;
	case IFM_IEEE80211_DS1:
	case IFM_IEEE80211_DS2:
	case IFM_IEEE80211_DS5:
	case IFM_IEEE80211_DS11:
		if (sc->sc_mib_phy.IEEE_PHY_Type != AWI_PHY_TYPE_DS)
			return 0;
		break;
	}

	switch (IFM_SUBTYPE(opt)) {
	case IFM_IEEE80211_FH1:
	case IFM_IEEE80211_DS1:
		rate = 2;
		break;
	case IFM_IEEE80211_FH2:
	case IFM_IEEE80211_DS2:
		rate = 4;
		break;
	case IFM_IEEE80211_DS5:
		rate = 11;
		break;
	case IFM_IEEE80211_DS11:
		rate = 22;
		break;
	}
	return rate;
}

static void
awi_rx_int(struct awi_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ic.ic_if;
	u_int8_t state, rate, rssi;
	u_int16_t len;
	u_int32_t frame, next, timoff, rxoff;
	struct mbuf *m;

	rxoff = sc->sc_rxdoff;
	for (;;) {
		state = awi_read_1(sc, rxoff + AWI_RXD_HOST_DESC_STATE);
		if (state & AWI_RXD_ST_OWN)
			break;
		if (!(state & AWI_RXD_ST_CONSUMED)) {
			if (state & AWI_RXD_ST_RXERROR) {
				ifp->if_ierrors++;
				goto rx_next;
			}
			len    = awi_read_2(sc, rxoff + AWI_RXD_LEN);
			rate   = awi_read_1(sc, rxoff + AWI_RXD_RATE);
			rssi   = awi_read_1(sc, rxoff + AWI_RXD_RSSI);
			frame  = awi_read_4(sc, rxoff + AWI_RXD_START_FRAME) &
			    0x7fff;
			timoff = awi_read_4(sc, rxoff + AWI_RXD_LOCALTIME);
			m = awi_devget(sc, frame, len);
			if (m == NULL) {
				ifp->if_ierrors++;
				goto rx_next;
			}
			if (state & AWI_RXD_ST_LF) {
				/* TODO check my bss */
				if (!(sc->sc_ic.ic_flags & IEEE80211_F_SIBSS) &&
				    sc->sc_ic.ic_state == IEEE80211_S_RUN) {
					sc->sc_rx_timer = 10;
					ifp->if_timer = 1;
				}
				if ((ifp->if_flags & IFF_DEBUG) &&
				    (ifp->if_flags & IFF_LINK2))
					ieee80211_dump_pkt(m->m_data, m->m_len,
					    rate / 5, rssi);
				if ((ifp->if_flags & IFF_LINK0) ||
				    sc->sc_adhoc_ap)
					m = awi_ether_modcap(sc, m);
				if (m == NULL)
					ifp->if_ierrors++;
				else
					ieee80211_input(ifp, m, rssi, timoff);
			} else
				sc->sc_rxpend = m;
  rx_next:
			state |= AWI_RXD_ST_CONSUMED;
			awi_write_1(sc, rxoff + AWI_RXD_HOST_DESC_STATE, state);
		}
		next = awi_read_4(sc, rxoff + AWI_RXD_NEXT);
		if (next & AWI_RXD_NEXT_LAST)
			break;
		/* make sure the next pointer is correct */
		if (next != awi_read_4(sc, rxoff + AWI_RXD_NEXT))
			break;
		state |= AWI_RXD_ST_OWN;
		awi_write_1(sc, rxoff + AWI_RXD_HOST_DESC_STATE, state);
		rxoff = next & 0x7fff;
	}
	sc->sc_rxdoff = rxoff;
}

static void
awi_tx_int(struct awi_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ic.ic_if;
	u_int8_t flags;

	while (sc->sc_txdone != sc->sc_txnext) {
		flags = awi_read_1(sc, sc->sc_txdone + AWI_TXD_STATE);
		if ((flags & AWI_TXD_ST_OWN) || !(flags & AWI_TXD_ST_DONE))
			break;
		if (flags & AWI_TXD_ST_ERROR)
			ifp->if_oerrors++;
		sc->sc_txdone = awi_read_4(sc, sc->sc_txdone + AWI_TXD_NEXT) &
		    0x7fff;
	}
	DPRINTF2(("awi_txint: txdone %d txnext %d txbase %d txend %d\n",
	    sc->sc_txdone, sc->sc_txnext, sc->sc_txbase, sc->sc_txend));
	sc->sc_tx_timer = 0;
	ifp->if_flags &= ~IFF_OACTIVE;
	awi_start(ifp);
}

static struct mbuf *
awi_devget(struct awi_softc *sc, u_int32_t off, u_int16_t len)
{
	struct ifnet *ifp = &sc->sc_ic.ic_if;
	struct mbuf *m;
	struct mbuf *top, **mp;
	u_int tlen;

	top = sc->sc_rxpend;
	mp = &top;
	if (top != NULL) {
		sc->sc_rxpend = NULL;
		top->m_pkthdr.len += len;
		m = top;
		while (*mp != NULL) {
			m = *mp;
			mp = &m->m_next;
		}
		if (m->m_flags & M_EXT)
			tlen = m->m_ext.ext_size;
		else if (m->m_flags & M_PKTHDR)
			tlen = MHLEN;
		else
			tlen = MLEN;
		tlen -= m->m_len;
		if (tlen > len)
			tlen = len;
		awi_read_bytes(sc, off, mtod(m, u_int8_t *) + m->m_len, tlen);
		off += tlen;
		len -= tlen;
	}

	while (len > 0) {
		if (top == NULL) {
			MGETHDR(m, M_DONTWAIT, MT_DATA);
			if (m == NULL)
				return NULL;
			m->m_pkthdr.rcvif = ifp;
			m->m_pkthdr.len = len;
			m->m_len = MHLEN;
			m->m_flags |= M_HASFCS;
		} else {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == NULL) {
				m_freem(top);
				return NULL;
			}
			m->m_len = MLEN;
		}
		if (len >= MINCLSIZE) {
			MCLGET(m, M_DONTWAIT);
			if (m->m_flags & M_EXT)
				m->m_len = m->m_ext.ext_size;
		}
		if (top == NULL) {
			int hdrlen = sizeof(struct ieee80211_frame) +
			    sizeof(struct llc);
			caddr_t newdata = (caddr_t)
			    ALIGN(m->m_data + hdrlen) - hdrlen;
			m->m_len -= newdata - m->m_data;
			m->m_data = newdata;
		}
		if (m->m_len > len)
			m->m_len = len;
		awi_read_bytes(sc, off, mtod(m, u_int8_t *), m->m_len);
		off += m->m_len;
		len -= m->m_len;
		*mp = m;
		mp = &m->m_next;
	}
	return top;
}

/*
 * Initialize hardware and start firmware to accept commands.
 * Called everytime after power on firmware.
 */

static int
awi_hw_init(struct awi_softc *sc)
{
	u_int8_t status;
	u_int16_t intmask;
	int i, error;

	sc->sc_enab_intr = 0;
	sc->sc_invalid = 0;	/* XXX: really? */
	awi_drvstate(sc, AWI_DRV_RESET);

	/* reset firmware */
	am79c930_gcr_setbits(&sc->sc_chip, AM79C930_GCR_CORESET);
	DELAY(100);
	awi_write_1(sc, AWI_SELFTEST, 0);
	awi_write_1(sc, AWI_CMD, 0);
	awi_write_1(sc, AWI_BANNER, 0);
	am79c930_gcr_clearbits(&sc->sc_chip, AM79C930_GCR_CORESET);
	DELAY(100);

	/* wait for selftest completion */
	for (i = 0; ; i++) {
		if (i >= AWI_SELFTEST_TIMEOUT*hz/1000) {
			printf("%s: failed to complete selftest (timeout)\n",
			    sc->sc_dev.dv_xname);
			return ENXIO;
		}
		status = awi_read_1(sc, AWI_SELFTEST);
		if ((status & 0xf0) == 0xf0)
			break;
		if (sc->sc_cansleep) {
			sc->sc_sleep_cnt++;
			(void)tsleep(sc, PWAIT, "awitst", 1);
			sc->sc_sleep_cnt--;
		} else {
			DELAY(1000*1000/hz);
		}
	}
	if (status != AWI_SELFTEST_PASSED) {
		printf("%s: failed to complete selftest (code %x)\n",
		    sc->sc_dev.dv_xname, status);
		return ENXIO;
	}

	/* check banner to confirm firmware write it */
	awi_read_bytes(sc, AWI_BANNER, sc->sc_banner, AWI_BANNER_LEN);
	if (memcmp(sc->sc_banner, "PCnetMobile:", 12) != 0) {
		printf("%s: failed to complete selftest (bad banner)\n",
		    sc->sc_dev.dv_xname);
		for (i = 0; i < AWI_BANNER_LEN; i++)
			printf("%s%02x", i ? ":" : "\t", sc->sc_banner[i]);
		printf("\n");
		return ENXIO;
	}

	/* initializing interrupt */
	sc->sc_enab_intr = 1;
	error = awi_intr_lock(sc);
	if (error)
		return error;
	intmask = AWI_INT_GROGGY | AWI_INT_SCAN_CMPLT |
	    AWI_INT_TX | AWI_INT_RX | AWI_INT_CMD;
	awi_write_1(sc, AWI_INTMASK, ~intmask & 0xff);
	awi_write_1(sc, AWI_INTMASK2, 0);
	awi_write_1(sc, AWI_INTSTAT, 0);
	awi_write_1(sc, AWI_INTSTAT2, 0);
	awi_intr_unlock(sc);
	am79c930_gcr_setbits(&sc->sc_chip, AM79C930_GCR_ENECINT);

	/* issuing interface test command */
	error = awi_cmd(sc, AWI_CMD_NOP, AWI_WAIT);
	if (error) {
		printf("%s: failed to complete selftest", sc->sc_dev.dv_xname);
		if (error == ENXIO)
			printf(" (no hardware)\n");
		else if (error != EWOULDBLOCK)
			printf(" (error %d)\n", error);
		else if (sc->sc_cansleep)
			printf(" (lost interrupt)\n");
		else
			printf(" (command timeout)\n");
	}
	return error;
}

/*
 * Extract the factory default MIB value from firmware and assign the driver
 * default value.
 * Called once at attaching the interface.
 */

static int
awi_init_mibs(struct awi_softc *sc)
{
	int i, error;
	struct awi_chanset *cs;

	if ((error = awi_mib(sc, AWI_CMD_GET_MIB, AWI_MIB_LOCAL, AWI_WAIT)) ||
	    (error = awi_mib(sc, AWI_CMD_GET_MIB, AWI_MIB_ADDR, AWI_WAIT)) ||
	    (error = awi_mib(sc, AWI_CMD_GET_MIB, AWI_MIB_MAC, AWI_WAIT)) ||
	    (error = awi_mib(sc, AWI_CMD_GET_MIB, AWI_MIB_MGT, AWI_WAIT)) ||
	    (error = awi_mib(sc, AWI_CMD_GET_MIB, AWI_MIB_PHY, AWI_WAIT))) {
		printf("%s: failed to get default mib value (error %d)\n",
		    sc->sc_dev.dv_xname, error);
		return error;
	}

	memset(&sc->sc_ic.ic_chan_avail, 0, sizeof(sc->sc_ic.ic_chan_avail));
	for (cs = awi_chanset; ; cs++) {
		if (cs->cs_type == 0) {
			printf("%s: failed to set available channel\n",
			    sc->sc_dev.dv_xname);
			return ENXIO;
		}
		if (cs->cs_type == sc->sc_mib_phy.IEEE_PHY_Type &&
		    cs->cs_region == sc->sc_mib_phy.aCurrent_Reg_Domain)
			break;
	}
	if (sc->sc_mib_phy.IEEE_PHY_Type == AWI_PHY_TYPE_FH) {
		for (i = cs->cs_min; i <= cs->cs_max; i++) {
			setbit(sc->sc_ic.ic_chan_avail, 
			    IEEE80211_FH_CHAN(i % 3 + 1, i));
			/*
			 * According to the IEEE 802.11 specification,
			 * hop pattern parameter for FH phy should be
			 * incremented by 3 for given hop chanset, i.e.,
			 * the chanset parameter is calculated for given
			 * hop patter.  However, BayStack 650 Access Points
			 * apparently use fixed hop chanset parameter value
			 * 1 for any hop pattern.  So we also try this
			 * combination of hop chanset and pattern.
			 */
			setbit(sc->sc_ic.ic_chan_avail, 
			    IEEE80211_FH_CHAN(1, i));
		}
	} else {
		for (i = cs->cs_min; i <= cs->cs_max; i++)
			setbit(sc->sc_ic.ic_chan_avail, i);
	}
	sc->sc_cur_chan = cs->cs_def;

	memset(&sc->sc_mib_mac.aDesired_ESS_ID, 0, AWI_ESS_ID_SIZE);
	sc->sc_mib_mac.aDesired_ESS_ID[0] = IEEE80211_ELEMID_SSID;
	sc->sc_mib_local.Fragmentation_Dis = 1;
	sc->sc_mib_local.Accept_All_Multicast_Dis = 1;
	sc->sc_mib_local.Power_Saving_Mode_Dis = 1;

	/* allocate buffers */
	sc->sc_txbase = AWI_BUFFERS;
	sc->sc_txend = sc->sc_txbase +
	    (AWI_TXD_SIZE + sizeof(struct ieee80211_frame) +
	    sizeof(struct ether_header) + ETHERMTU) * AWI_NTXBUFS;
	LE_WRITE_4(&sc->sc_mib_local.Tx_Buffer_Offset, sc->sc_txbase);
	LE_WRITE_4(&sc->sc_mib_local.Tx_Buffer_Size,
	    sc->sc_txend - sc->sc_txbase);
	LE_WRITE_4(&sc->sc_mib_local.Rx_Buffer_Offset, sc->sc_txend);
	LE_WRITE_4(&sc->sc_mib_local.Rx_Buffer_Size,
	    AWI_BUFFERS_END - sc->sc_txend);
	sc->sc_mib_local.Network_Mode = 1;
	sc->sc_mib_local.Acting_as_AP = 0;
	return 0;
}

static int
awi_chan_check(void *arg, u_char *chanreq)
{
	struct awi_softc *sc = arg;
	int i;
	struct awi_chanset *cs;
	u_char chanlist[(IEEE80211_CHAN_MAX+1)/NBBY];

	for (cs = awi_chanset; cs->cs_type != 0; cs++) {
		if (cs->cs_type != sc->sc_mib_phy.IEEE_PHY_Type)
			continue;
		memset(chanlist, 0, sizeof(chanlist));
		for (i = 0; ; i++) {
			if (i == IEEE80211_CHAN_MAX) {
				sc->sc_mib_phy.aCurrent_Reg_Domain =
				    cs->cs_region;
				memcpy(sc->sc_ic.ic_chan_avail, chanlist,
				    sizeof(sc->sc_ic.ic_chan_avail));
				sc->sc_cur_chan = cs->cs_def;
				return 0;
			}
			if (i >= cs->cs_min && i <= cs->cs_max)
				setbit(chanlist, i);
			else if (isset(chanreq, i))
				break;
		}
	}
	return EINVAL;
}

static int
awi_mib(struct awi_softc *sc, u_int8_t cmd, u_int8_t mib, int wflag)
{
	int error;
	u_int8_t size, *ptr;

	switch (mib) {
	case AWI_MIB_LOCAL:
		ptr = (u_int8_t *)&sc->sc_mib_local;
		size = sizeof(sc->sc_mib_local);
		break;
	case AWI_MIB_ADDR:
		ptr = (u_int8_t *)&sc->sc_mib_addr;
		size = sizeof(sc->sc_mib_addr);
		break;
	case AWI_MIB_MAC:
		ptr = (u_int8_t *)&sc->sc_mib_mac;
		size = sizeof(sc->sc_mib_mac);
		break;
	case AWI_MIB_STAT:
		ptr = (u_int8_t *)&sc->sc_mib_stat;
		size = sizeof(sc->sc_mib_stat);
		break;
	case AWI_MIB_MGT:
		ptr = (u_int8_t *)&sc->sc_mib_mgt;
		size = sizeof(sc->sc_mib_mgt);
		break;
	case AWI_MIB_PHY:
		ptr = (u_int8_t *)&sc->sc_mib_phy;
		size = sizeof(sc->sc_mib_phy);
		break;
	default:
		return EINVAL;
	}
	if (sc->sc_cmd_inprog) {
		if ((error = awi_cmd_wait(sc)) != 0) {
			if (error == EWOULDBLOCK)
				DPRINTF(("awi_mib: cmd %d inprog",
				    sc->sc_cmd_inprog));
			return error;
		}
	}
	sc->sc_cmd_inprog = cmd;
	if (cmd == AWI_CMD_SET_MIB)
		awi_write_bytes(sc, AWI_CA_MIB_DATA, ptr, size);
	awi_write_1(sc, AWI_CA_MIB_TYPE, mib);
	awi_write_1(sc, AWI_CA_MIB_SIZE, size);
	awi_write_1(sc, AWI_CA_MIB_INDEX, 0);
	if ((error = awi_cmd(sc, cmd, wflag)) != 0)
		return error;
	if (cmd == AWI_CMD_GET_MIB) {
		awi_read_bytes(sc, AWI_CA_MIB_DATA, ptr, size);
#ifdef AWI_DEBUG
		if (awi_debug) {
			int i;

			printf("awi_mib: #%d:", mib);
			for (i = 0; i < size; i++)
				printf(" %02x", ptr[i]);
			printf("\n");
		}
#endif
	}
	return 0;
}

static int
awi_cmd(struct awi_softc *sc, u_int8_t cmd, int wflag)
{
	u_int8_t status;
	int error = 0;
#ifdef AWI_DEBUG
	static const char *cmdname[] = {
	    "IDLE", "NOP", "SET_MIB", "INIT_TX", "FLUSH_TX", "INIT_RX",
	    "KILL_RX", "SLEEP", "WAKE", "GET_MIB", "SCAN", "SYNC", "RESUME"
	};
#endif

#ifdef AWI_DEBUG
	if (awi_debug > 1) {
		if (cmd >= sizeof(cmdname)/sizeof(cmdname[0]))
			printf("awi_cmd: #%d", cmd);
		else
			printf("awi_cmd: %s", cmdname[cmd]);
		printf(" %s\n", wflag == AWI_NOWAIT ? "nowait" : "wait");
	}
#endif
	sc->sc_cmd_inprog = cmd;
	awi_write_1(sc, AWI_CMD_STATUS, AWI_STAT_IDLE);
	awi_write_1(sc, AWI_CMD, cmd);
	if (wflag == AWI_NOWAIT)
		return EINPROGRESS;
	if ((error = awi_cmd_wait(sc)) != 0)
		return error;
	status = awi_read_1(sc, AWI_CMD_STATUS);
	awi_write_1(sc, AWI_CMD, 0);
	switch (status) {
	case AWI_STAT_OK:
		break;
	case AWI_STAT_BADPARM:
		return EINVAL;
	default:
		printf("%s: command %d failed %x\n",
		    sc->sc_dev.dv_xname, cmd, status);
		return ENXIO;
	}
	return 0;
}

static int
awi_cmd_wait(struct awi_softc *sc)
{
	int i, error = 0;

	i = 0;
	while (sc->sc_cmd_inprog) {
		if (sc->sc_invalid)
			return ENXIO;
		if (awi_read_1(sc, AWI_CMD) != sc->sc_cmd_inprog) {
			printf("%s: failed to access hardware\n",
			    sc->sc_dev.dv_xname);
			sc->sc_invalid = 1;
			return ENXIO;
		}
		if (sc->sc_cansleep) {
			sc->sc_sleep_cnt++;
			error = tsleep(sc, PWAIT, "awicmd",
			    AWI_CMD_TIMEOUT*hz/1000);
			sc->sc_sleep_cnt--;
		} else {
			if (awi_read_1(sc, AWI_CMD_STATUS) != AWI_STAT_IDLE) {
				awi_cmd_done(sc);
				break;
			}
			if (i++ >= AWI_CMD_TIMEOUT*1000/10)
				error = EWOULDBLOCK;
			else
				DELAY(10);
		}
		if (error)
			break;
	}
	if (error) {
		DPRINTF(("awi_cmd_wait: cmd 0x%x, error %d\n",
		    sc->sc_cmd_inprog, error));
	}
	return error;
}

static void
awi_cmd_done(struct awi_softc *sc)
{
	u_int8_t cmd, status;

	status = awi_read_1(sc, AWI_CMD_STATUS);
	if (status == AWI_STAT_IDLE)
		return;		/* stray interrupt */

	cmd = sc->sc_cmd_inprog;
	sc->sc_cmd_inprog = 0;
	wakeup(sc);
	awi_write_1(sc, AWI_CMD, 0);

	if (status != AWI_STAT_OK) {
		printf("%s: command %d failed %x\n",
		    sc->sc_dev.dv_xname, cmd, status);
		sc->sc_substate = AWI_ST_NONE;
		return;
	}
	if (sc->sc_substate != AWI_ST_NONE)
		(void)ieee80211_new_state(&sc->sc_ic.ic_if, sc->sc_nstate, -1);
}

static int
awi_next_txd(struct awi_softc *sc, int len, u_int32_t *framep, u_int32_t *ntxdp)
{
	u_int32_t txd, ntxd, frame;

	txd = sc->sc_txnext;
	frame = txd + AWI_TXD_SIZE;
	if (frame + len > sc->sc_txend)
		frame = sc->sc_txbase;
	ntxd = frame + len;
	if (ntxd + AWI_TXD_SIZE > sc->sc_txend)
		ntxd = sc->sc_txbase;
	*framep = frame;
	*ntxdp = ntxd;
	/*
	 * Determine if there are any room in ring buffer.
	 *		--- send wait,  === new data,  +++ conflict (ENOBUFS)
	 *   base........................end
	 *	   done----txd=====ntxd		OK
	 *	 --txd=====done++++ntxd--	full
	 *	 --txd=====ntxd    done--	OK
	 *	 ==ntxd    done----txd===	OK
	 *	 ==done++++ntxd----txd===	full
	 *	 ++ntxd    txd=====done++	full
	 */
	if (txd < ntxd) {
		if (txd < sc->sc_txdone && ntxd + AWI_TXD_SIZE > sc->sc_txdone)
			return ENOBUFS;
	} else {
		if (txd < sc->sc_txdone || ntxd + AWI_TXD_SIZE > sc->sc_txdone)
			return ENOBUFS;
	}
	return 0;
}

static int
awi_lock(struct awi_softc *sc)
{
	int error = 0;

	if (curlwp == NULL) {
		/*
		 * XXX
		 * Though driver ioctl should be called with context,
		 * KAME ipv6 stack calls ioctl in interrupt for now.
		 * We simply abort the request if there are other
		 * ioctl requests in progress.
		 */
		if (sc->sc_busy) {
			return EWOULDBLOCK;
			if (sc->sc_invalid)
				return ENXIO;
		}
		sc->sc_busy = 1;
		sc->sc_cansleep = 0;
		return 0;
	}
	while (sc->sc_busy) {
		if (sc->sc_invalid)
			return ENXIO;
		sc->sc_sleep_cnt++;
		error = tsleep(sc, PWAIT | PCATCH, "awilck", 0);
		sc->sc_sleep_cnt--;
		if (error)
			return error;
	}
	sc->sc_busy = 1;
	sc->sc_cansleep = 1;
	return 0;
}

static void
awi_unlock(struct awi_softc *sc)
{
	sc->sc_busy = 0;
	sc->sc_cansleep = 0;
	if (sc->sc_sleep_cnt)
		wakeup(sc);
}

static int
awi_intr_lock(struct awi_softc *sc)
{
	u_int8_t status;
	int i, retry;

	status = 1;
	for (retry = 0; retry < 10; retry++) {
		for (i = 0; i < AWI_LOCKOUT_TIMEOUT*1000/5; i++) {
			if ((status = awi_read_1(sc, AWI_LOCKOUT_HOST)) == 0)
				break;
			DELAY(5);
		}
		if (status != 0)
			break;
		awi_write_1(sc, AWI_LOCKOUT_MAC, 1);
		if ((status = awi_read_1(sc, AWI_LOCKOUT_HOST)) == 0)
			break;
		awi_write_1(sc, AWI_LOCKOUT_MAC, 0);
	}
	if (status != 0) {
		printf("%s: failed to lock interrupt\n",
		    sc->sc_dev.dv_xname);
		return ENXIO;
	}
	return 0;
}

static void
awi_intr_unlock(struct awi_softc *sc)
{

	awi_write_1(sc, AWI_LOCKOUT_MAC, 0);
}

static int
awi_newstate(void *arg, enum ieee80211_state nstate)
{
	struct awi_softc *sc = arg;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_bss *bs = &ic->ic_bss;
	struct ifnet *ifp = &ic->ic_if;
	int error;
	u_int8_t newmode;
	enum ieee80211_state ostate;
#ifdef AWI_DEBUG
	static const char *stname[] =
	    { "INIT", "SCAN", "AUTH", "ASSOC", "RUN" };
	static const char *substname[] =
	    { "NONE", "SCAN_INIT", "SCAN_SETMIB", "SCAN_SCCMD",
	      "SUB_INIT", "SUB_SETSS", "SUB_SYNC" };
#endif /* AWI_DEBUG */

	ostate = ic->ic_state;
	DPRINTF(("awi_newstate: %s (%s/%s) -> %s\n", stname[ostate],
	    stname[sc->sc_nstate], substname[sc->sc_substate], stname[nstate]));

	/* set LED */
	switch (nstate) {
	case IEEE80211_S_INIT:
		awi_drvstate(sc, AWI_DRV_RESET);
		break;
	case IEEE80211_S_SCAN:
		if (ic->ic_flags & IEEE80211_F_ADHOC)
			awi_drvstate(sc, AWI_DRV_ADHSC);
		else
			awi_drvstate(sc, AWI_DRV_INFSY);
		break;
	case IEEE80211_S_AUTH:
		awi_drvstate(sc, AWI_DRV_INFSY);
		break;
	case IEEE80211_S_ASSOC:
		awi_drvstate(sc, AWI_DRV_INFAUTH);
		break;
	case IEEE80211_S_RUN:
		if (ic->ic_flags & IEEE80211_F_ADHOC)
			awi_drvstate(sc, AWI_DRV_ADHSY);
		else
			awi_drvstate(sc, AWI_DRV_INFASSOC);
		break;
	}

	if (nstate == IEEE80211_S_INIT) {
		sc->sc_substate = AWI_ST_NONE;
		ic->ic_flags &= ~IEEE80211_F_SIBSS;
		return 0;
	}

	/* state transition */
	if (nstate == IEEE80211_S_SCAN) {
		/* SCAN substate */
		if (sc->sc_substate == AWI_ST_NONE) {
			sc->sc_nstate = nstate;	/* next state in transition */
			sc->sc_substate = AWI_ST_SCAN_INIT;
		}
		switch (sc->sc_substate) {
		case AWI_ST_SCAN_INIT:
			sc->sc_substate = AWI_ST_SCAN_SETMIB;
			switch (ostate) {
			case IEEE80211_S_RUN:
				/* beacon miss */
				if (ifp->if_flags & IFF_DEBUG)
					printf("%s: no recent beacons from %s;"
					    " rescanning\n",
					    ifp->if_xname,
					    ether_sprintf(ic->ic_bss.bs_bssid));
				/* FALLTHRU */
			case IEEE80211_S_AUTH:
			case IEEE80211_S_ASSOC:
				/* timeout restart scan */
				ieee80211_free_scan(ifp);
				/* FALLTHRU */
			case IEEE80211_S_INIT:
				ic->ic_flags |= IEEE80211_F_ASCAN;
				ic->ic_scan_timer = 0;
				/* FALLTHRU */
			case IEEE80211_S_SCAN:
				/* scan next */
				break;
			}
			if (ic->ic_flags & IEEE80211_F_ASCAN)
				newmode = AWI_SCAN_ACTIVE;
			else
				newmode = AWI_SCAN_PASSIVE;
			if (sc->sc_mib_mgt.aScan_Mode != newmode) {
				sc->sc_mib_mgt.aScan_Mode = newmode;
				if ((error = awi_mib(sc, AWI_CMD_SET_MIB,
				    AWI_MIB_MGT, AWI_NOWAIT)) != 0)
					break;
			}
			/* FALLTHRU */
		case AWI_ST_SCAN_SETMIB:
			sc->sc_substate = AWI_ST_SCAN_SCCMD;
			if (sc->sc_cmd_inprog) {
				if ((error = awi_cmd_wait(sc)) != 0)
					break;
			}
			sc->sc_cmd_inprog = AWI_CMD_SCAN;
			awi_write_2(sc, AWI_CA_SCAN_DURATION,
			    (ic->ic_flags & IEEE80211_F_ASCAN) ?
			    AWI_ASCAN_DURATION : AWI_PSCAN_DURATION);
			if (sc->sc_mib_phy.IEEE_PHY_Type == AWI_PHY_TYPE_FH) {
				awi_write_1(sc, AWI_CA_SCAN_SET,
				    IEEE80211_FH_CHANSET(bs->bs_chan));
				awi_write_1(sc, AWI_CA_SCAN_PATTERN,
				    IEEE80211_FH_CHANPAT(bs->bs_chan));
				awi_write_1(sc, AWI_CA_SCAN_IDX, 1);
			} else {
				awi_write_1(sc, AWI_CA_SCAN_SET, bs->bs_chan);
				awi_write_1(sc, AWI_CA_SCAN_PATTERN, 0);
				awi_write_1(sc, AWI_CA_SCAN_IDX, 0);
			}
			awi_write_1(sc, AWI_CA_SCAN_SUSP, 0);
			sc->sc_cur_chan = bs->bs_chan;
			if ((error = awi_cmd(sc, AWI_CMD_SCAN, AWI_NOWAIT))
			    != 0)
				break;
			/* FALLTHRU */
		case AWI_ST_SCAN_SCCMD:
			if (ic->ic_scan_timer == 0)
				ic->ic_scan_timer =
				    (ic->ic_flags & IEEE80211_F_ASCAN) ?
				    IEEE80211_ASCAN_WAIT : IEEE80211_PSCAN_WAIT;
			ifp->if_timer = 1;
			ic->ic_state = nstate;
			sc->sc_substate = AWI_ST_NONE;
			error = EINPROGRESS;
			break;
		default:
			DPRINTF(("awi_newstate: unexpected state %s/%s\n",
			    stname[nstate], substname[sc->sc_substate]));
			sc->sc_substate = AWI_ST_NONE;
			error = EIO;
			break;
		}
		return error;
	}

	if (ostate == IEEE80211_S_SCAN) {
		/* set SSID and channel */
		/* substate */
		if (sc->sc_substate == AWI_ST_NONE) {
			sc->sc_nstate = nstate;	/* next state in transition */
			sc->sc_substate = AWI_ST_SUB_INIT;
		}
		switch (sc->sc_substate) {
		case AWI_ST_SUB_INIT:
			sc->sc_substate = AWI_ST_SUB_SETSS;
			memcpy(&sc->sc_mib_mgt.aCurrent_BSS_ID, bs->bs_bssid,
			    IEEE80211_ADDR_LEN);
			memset(&sc->sc_mib_mgt.aCurrent_ESS_ID, 0,
			    AWI_ESS_ID_SIZE);
			sc->sc_mib_mgt.aCurrent_ESS_ID[0] =
			    IEEE80211_ELEMID_SSID;
			sc->sc_mib_mgt.aCurrent_ESS_ID[1] = bs->bs_esslen;
			memcpy(&sc->sc_mib_mgt.aCurrent_ESS_ID[2],
			    bs->bs_essid, bs->bs_esslen);
			LE_WRITE_2(&sc->sc_mib_mgt.aBeacon_Period,
			    bs->bs_intval);
			if ((error = awi_mib(sc, AWI_CMD_SET_MIB, AWI_MIB_MGT,
			    AWI_NOWAIT)) != 0)
				break;
			/* FALLTHRU */
		case AWI_ST_SUB_SETSS:
			sc->sc_substate = AWI_ST_SUB_SYNC;
			if (sc->sc_cmd_inprog) {
				if (awi_cmd_wait(sc))
					break;
			}
			sc->sc_cmd_inprog = AWI_CMD_SYNC;
			if (sc->sc_mib_phy.IEEE_PHY_Type == AWI_PHY_TYPE_FH) {
				awi_write_1(sc, AWI_CA_SYNC_SET,
				    IEEE80211_FH_CHANSET(bs->bs_chan));
				awi_write_1(sc, AWI_CA_SYNC_PATTERN,
				    IEEE80211_FH_CHANPAT(bs->bs_chan));
				awi_write_1(sc, AWI_CA_SYNC_IDX,
				    bs->bs_fhindex);
				awi_write_2(sc, AWI_CA_SYNC_DWELL,
				    bs->bs_fhdwell);
			} else {
				awi_write_1(sc, AWI_CA_SYNC_SET, bs->bs_chan);
				awi_write_1(sc, AWI_CA_SYNC_PATTERN, 0);
				awi_write_1(sc, AWI_CA_SYNC_IDX, 0);
				awi_write_2(sc, AWI_CA_SYNC_DWELL, 0);
			}
			if ((ic->ic_flags & IEEE80211_F_SIBSS) &&
			    !sc->sc_no_bssid)
				awi_write_1(sc, AWI_CA_SYNC_STARTBSS, 1);
			else
				awi_write_1(sc, AWI_CA_SYNC_STARTBSS, 0);
			awi_write_2(sc, AWI_CA_SYNC_MBZ, 0);
			awi_write_bytes(sc, AWI_CA_SYNC_TIMESTAMP,
			    bs->bs_tstamp, 8);
			awi_write_4(sc, AWI_CA_SYNC_REFTIME, bs->bs_timoff);
			sc->sc_cur_chan = bs->bs_chan;
			if ((error = awi_cmd(sc, AWI_CMD_SYNC, AWI_NOWAIT))
			    != 0)
				break;
			/* FALLTHRU */
		case AWI_ST_SUB_SYNC:
			sc->sc_substate = AWI_ST_NONE;
			if (ic->ic_flags & IEEE80211_F_SIBSS) {
				if ((error = awi_mib(sc, AWI_CMD_GET_MIB,
				    AWI_MIB_MGT, AWI_WAIT)) != 0)
					break;
				memcpy(bs->bs_bssid,
				    &sc->sc_mib_mgt.aCurrent_BSS_ID,
				    IEEE80211_ADDR_LEN);
			} else {
				if (nstate == IEEE80211_S_RUN) {
					sc->sc_rx_timer = 10;
					ifp->if_timer = 1;
				}
			}
			error = 0;
			break;
		default:
			DPRINTF(("awi_newstate: unexpected state %s/%s\n",
			    stname[nstate], substname[sc->sc_substate]));
			sc->sc_substate = AWI_ST_NONE;
			error = EIO;
			break;
		}
		return error;
	}

	sc->sc_substate = AWI_ST_NONE;

	return 0;
}

static struct mbuf *
awi_ether_encap(struct awi_softc *sc, struct mbuf *m)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_bss *bs = &ic->ic_bss;
	struct ether_header *eh;
	struct ieee80211_frame *wh;

	if (m->m_len < sizeof(struct ether_header)) {
		m = m_pullup(m, sizeof(struct ether_header));
		if (m == NULL)
			return NULL;
	}
	eh = mtod(m, struct ether_header *);
	M_PREPEND(m, sizeof(struct ieee80211_frame), M_DONTWAIT);
	if (m == NULL)
		return NULL;
	wh = mtod(m, struct ieee80211_frame *);
	wh->i_fc[0] = IEEE80211_FC0_VERSION_0 | IEEE80211_FC0_TYPE_DATA;
	*(u_int16_t *)wh->i_dur = 0;
	*(u_int16_t *)wh->i_seq =
	    htole16(bs->bs_txseq << IEEE80211_SEQ_SEQ_SHIFT);
	bs->bs_txseq++;
	if (ic->ic_flags & IEEE80211_F_ADHOC) {
		wh->i_fc[1] = IEEE80211_FC1_DIR_NODS;
		if (sc->sc_adhoc_ap)
			memcpy(wh->i_addr1, bs->bs_macaddr, IEEE80211_ADDR_LEN);
		else
			memcpy(wh->i_addr1, eh->ether_dhost,
			    IEEE80211_ADDR_LEN);
		memcpy(wh->i_addr2, eh->ether_shost, IEEE80211_ADDR_LEN);
		memcpy(wh->i_addr3, bs->bs_bssid, IEEE80211_ADDR_LEN);
	} else {
		wh->i_fc[1] = IEEE80211_FC1_DIR_TODS;
		memcpy(wh->i_addr1, bs->bs_bssid, IEEE80211_ADDR_LEN);
		memcpy(wh->i_addr2, eh->ether_shost, IEEE80211_ADDR_LEN);
		memcpy(wh->i_addr3, eh->ether_dhost, IEEE80211_ADDR_LEN);
	}
	return m;
}

static struct mbuf *
awi_ether_modcap(struct awi_softc *sc, struct mbuf *m)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ether_header eh;
	struct ieee80211_frame wh;
	struct llc *llc;

	if (m->m_len < sizeof(wh) + sizeof(eh)) {
		m = m_pullup(m, sizeof(wh) + sizeof(eh));
		if (m == NULL)
			return NULL;
	}
	memcpy(&wh, mtod(m, caddr_t), sizeof(wh));
	if (wh.i_fc[0] != (IEEE80211_FC0_VERSION_0 | IEEE80211_FC0_TYPE_DATA))
		return m;
	memcpy(&eh, mtod(m, caddr_t) + sizeof(wh), sizeof(eh));
	m_adj(m, sizeof(eh) - sizeof(*llc));
	if (ic->ic_flags & IEEE80211_F_ADHOC)
		memcpy(wh.i_addr2, eh.ether_shost, IEEE80211_ADDR_LEN);
	memcpy(mtod(m, caddr_t), &wh, sizeof(wh));
	llc = (struct llc *)(mtod(m, caddr_t) + sizeof(wh));
	llc->llc_dsap = llc->llc_ssap = LLC_SNAP_LSAP;
	llc->llc_control = LLC_UI;
	llc->llc_snap.org_code[0] = 0;
	llc->llc_snap.org_code[1] = 0;
	llc->llc_snap.org_code[2] = 0;
	llc->llc_snap.ether_type = eh.ether_type;
	return m;
}

/*	$NetBSD: wivar.h,v 1.12 2002/04/04 07:06:17 ichiro Exp $	*/

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
 */


/*
 * FreeBSD driver ported to NetBSD by Bill Sommerfeld in the back of the
 * Oslo IETF plenary meeting.
 */

struct wi_softc	{
	struct device		sc_dev;
	struct ethercom		sc_ethercom;
	struct ifnet		*sc_ifp;
	void			*sc_ih; /* interrupt handler */
	int			(*sc_enable) __P((struct wi_softc *));
	void			(*sc_disable) __P((struct wi_softc *));

	int sc_attached;
	int sc_enabled;
	int sc_firmware_type;
#define	WI_LUCENT	0
#define	WI_INTERSIL	1
#define	WI_SYMBOL	2
	int sc_pri_firmware_ver;	/* Primary firmware version */
	int sc_sta_firmware_ver;	/* Station firmware version */
	int sc_pci;			/* attach to PCI-Bus */

	bus_space_tag_t		sc_iot;	/* bus cookie */
	bus_space_handle_t	sc_ioh; /* bus i/o handle */

	struct callout		wi_inquire_ch;
	struct callout		wi_scan_sh;

	u_int8_t		sc_macaddr[ETHER_ADDR_LEN];
	
	struct ifmedia		sc_media;
	int			wi_tx_data_id;
	int			wi_tx_mgmt_id;
	int			wi_if_flags;
	u_int16_t		wi_ptype;
	u_int16_t		wi_portnum;
	u_int16_t		wi_max_data_len;
	u_int16_t		wi_rts_thresh;
	u_int16_t		wi_ap_density;
	u_int16_t		wi_tx_rate;
	u_int16_t		wi_create_ibss;
	u_int16_t		wi_channel;
	u_int16_t		wi_pm_enabled;
	u_int16_t		wi_mor_enabled;
	u_int16_t		wi_max_sleep;
	u_int16_t		wi_authtype;
	u_int16_t		wi_roaming;

	struct ieee80211_nwid	wi_nodeid;
	struct ieee80211_nwid	wi_netid;
	struct ieee80211_nwid	wi_ibssid;

	u_int16_t		wi_txbuf[1596 / 2];
	int                     wi_has_wep;
	int                     wi_use_wep;
	int                     wi_tx_key;
	struct wi_ltv_keys      wi_keys;
	struct wi_counters	wi_stats;

	struct wi_apinfo	wi_aps[MAXAPINFO];
	int 			wi_naps;
	int			wi_scanning;	/* scan mode */
};

struct wi_card_ident {
	u_int16_t	card_id;
	char		*card_name;
	u_int8_t	firm_type;
};

int	wi_attach __P((struct wi_softc *));
int	wi_detach __P((struct wi_softc *));
int	wi_activate __P((struct device *, enum devact));
int	wi_intr __P((void *arg));
void	wi_power __P((struct wi_softc *, int));
void	wi_shutdown __P((struct wi_softc *));
void	wi_pci_reset __P((struct wi_softc *));

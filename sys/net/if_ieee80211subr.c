/*	$NetBSD: if_ieee80211subr.c,v 1.3.2.6 2002/06/24 22:11:31 nathanw Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Atsushi Onoe.
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
 * IEEE 802.11 generic handler
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ieee80211subr.c,v 1.3.2.6 2002/06/24 22:11:31 nathanw Exp $");

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h> 
#include <sys/mbuf.h>   
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/lwp.h>
#include <sys/proc.h>

#include <machine/endian.h>

#include <crypto/arc4/arc4.h>
 
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <net/if_llc.h>
#include <net/if_ieee80211.h>

#include <dev/ic/wi_ieee.h>	/* XXX */

#if NBPFILTER > 0 
#include <net/bpf.h>
#endif 

#ifdef INET
#include <netinet/in.h> 
#include <netinet/if_inarp.h>
#endif

#ifdef IEEE80211_DEBUG
int ieee80211_debug = 0;
#define	DPRINTF(X)	if (ieee80211_debug) printf X
#else
#define	DPRINTF(X)
#endif

static void ieee80211_send_prreq(struct ieee80211com *);
static void ieee80211_send_auth(struct ieee80211com *, int);
static void ieee80211_send_deauth(struct ieee80211com *, int);
static void ieee80211_send_asreq(struct ieee80211com *, int);
static void ieee80211_send_disassoc(struct ieee80211com *, int);
static void ieee80211_recv_beacon(struct ieee80211com *, struct mbuf *, int,
    u_int);
static void ieee80211_recv_auth(struct ieee80211com *, struct mbuf *);
static void ieee80211_recv_asresp(struct ieee80211com *, struct mbuf *);
static void ieee80211_recv_disassoc(struct ieee80211com *, struct mbuf *);
static void ieee80211_recv_deauth(struct ieee80211com *, struct mbuf *);
static void ieee80211_crc_init(void);
static u_int32_t ieee80211_crc_update(u_int32_t, u_int8_t *, int);

static const u_int8_t ieee80211_bcast_addr[IEEE80211_ADDR_LEN] =
    { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

static const char *ieee80211_mgt_subtype_name[] = {
	"assoc_req",	"assoc_resp",	"reassoc_req",	"reassoc_resp",
	"probe_req",	"probe_resp",	"reserved#6",	"reserved#7",
	"beacon",	"atim",		"disassoc",	"auth",
	"deauth",	"reserved#13",	"reserved#14",	"reserved#15"
};

void
ieee80211_ifattach(struct ifnet *ifp)
{
	struct ieee80211com *ic = (void *)ifp;
	int i, rate;

	ether_ifattach(ifp, ic->ic_myaddr);
	ieee80211_crc_init();
	memcpy(ic->ic_chan_active, ic->ic_chan_avail,
	    sizeof(ic->ic_chan_active));
	if (isclr(ic->ic_chan_active, ic->ic_ibss_chan)) {
		for (i = 0; i <= IEEE80211_CHAN_MAX; i++) {
			if (isset(ic->ic_chan_active, i)) {
				ic->ic_ibss_chan = i;
				break;
			}
		}
	}
	ic->ic_fixed_rate = -1;
	if (ic->ic_lintval == 0)
		ic->ic_lintval = 100;	/* default sleep */
	TAILQ_INIT(&ic->ic_scan);
	rate = 0;
	for (i = 0; i < IEEE80211_RATE_SIZE; i++) {
		if (ic->ic_sup_rates[i] != 0)
			rate = (ic->ic_sup_rates[i] & IEEE80211_RATE_VAL) / 2;
	}
	if (rate)
		ifp->if_baudrate = IF_Mbps(rate);
	ifp->if_hdrlen = sizeof(struct ieee80211_frame);
}

void
ieee80211_ifdetach(struct ifnet *ifp)
{
	struct ieee80211com *ic = (void *)ifp;
	int s;

	s = splnet();
	IF_PURGE(&ic->ic_mgtq);
	if (ic->ic_wep_ctx != NULL) {
		free(ic->ic_wep_ctx, M_DEVBUF);
		ic->ic_wep_ctx = NULL;
	}
	ieee80211_free_scan(ifp);
	ether_ifdetach(ifp);
}

void
ieee80211_input(struct ifnet *ifp, struct mbuf *m, int rssi, u_int timoff)
{
	struct ieee80211com *ic = (void *)ifp;
	struct ieee80211_bss *bs;
	struct ieee80211_frame *wh;
	u_int8_t dir, subtype;
	u_int16_t rxseq;

	/* trim CRC here for WEP can find its own CRC at the end of packet. */
	if (m->m_flags & M_HASFCS) {
		m_adj(m, -IEEE80211_CRC_LEN);
		m->m_flags &= ~M_HASFCS;
	}

	wh = mtod(m, struct ieee80211_frame *);
	if ((wh->i_fc[0] & IEEE80211_FC0_VERSION_MASK) !=
	    IEEE80211_FC0_VERSION_0) {
		if (ifp->if_flags & IFF_DEBUG)
			printf("%s: receive packet with wrong version: %x\n",
			    ifp->if_xname, wh->i_fc[0]);
		goto err;
	}
	dir = wh->i_fc[1] & IEEE80211_FC1_DIR_MASK;

	if (ic->ic_state != IEEE80211_S_SCAN) {
		if (ic->ic_flags & IEEE80211_F_ADHOC) {
			if (memcmp(wh->i_addr3, ic->ic_bss.bs_bssid,
			    IEEE80211_ADDR_LEN) != 0) {
				/* not interested in */
				DPRINTF(("ieee80211_input: other bss %s\n",
				    ether_sprintf(wh->i_addr3)));
				goto out;
			}
			TAILQ_FOREACH(bs, &ic->ic_scan, bs_list) {
				if (memcmp(wh->i_addr2, bs->bs_macaddr,
				    IEEE80211_ADDR_LEN) == 0)
					break;
			}
			if (bs == NULL) {
				DPRINTF(("ieee80211_input: unknown src %s\n",
				    ether_sprintf(wh->i_addr2)));
				bs = &ic->ic_bss;	/* XXX */
			}
		} else {
			bs = &ic->ic_bss;
			if (memcmp(wh->i_addr2, bs->bs_bssid,
			    IEEE80211_ADDR_LEN) != 0) {
				DPRINTF(("ieee80211_input: other bss %s\n",
				    ether_sprintf(wh->i_addr2)));
				/* not interested in */
				goto out;
			}
		}
		bs->bs_rssi = rssi;
		bs->bs_timoff = timoff;
		rxseq = bs->bs_rxseq;
		bs->bs_rxseq =
		    le16toh(*(u_int16_t *)wh->i_seq) >> IEEE80211_SEQ_SEQ_SHIFT;
		/* TODO: fragment */
		if ((wh->i_fc[1] & IEEE80211_FC1_RETRY) &&
		    rxseq == bs->bs_rxseq) {
			/* duplicate, silently discarded */
			goto out;
		}
	}

	if ((wh->i_fc[1] & IEEE80211_FC1_WEP) &&
	    (ic->ic_flags & IEEE80211_F_WEPON)) {
		m = ieee80211_wep_crypt(ifp, m, 0);
		if (m == NULL)
			goto err;
		wh = mtod(m, struct ieee80211_frame *);
	}

	switch (wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) {
	case IEEE80211_FC0_TYPE_DATA:
		if (ic->ic_flags & IEEE80211_F_ADHOC) {
			if (dir != IEEE80211_FC1_DIR_NODS)
				goto out;
		} else {
			if (dir != IEEE80211_FC1_DIR_FROMDS)
				goto out;
			if ((ifp->if_flags & IFF_SIMPLEX) &&
			    (wh->i_addr1[1] & 0x01) != 0 &&
			    memcmp(wh->i_addr3, ic->ic_myaddr,
			    IEEE80211_ADDR_LEN) == 0) {
				/*
				 * In IEEE802.11 network, multicast packet
				 * sent from me is broadcasted from AP.
				 * It should be silently discarded for
				 * SIMPLEX interface.
				 */
				goto out;
			}
		}
		m = ieee80211_decap(ifp, m);
		if (m == NULL)
			goto err;
#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif
		ifp->if_ipackets++;
		(*ifp->if_input)(ifp, m);
		return;

	case IEEE80211_FC0_TYPE_MGT:
		if (dir != IEEE80211_FC1_DIR_NODS)
			goto err;
		subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;

		/* drop frames without interest */
		if (ic->ic_state == IEEE80211_S_SCAN) {
			if (subtype != IEEE80211_FC0_SUBTYPE_BEACON &&
			    subtype != IEEE80211_FC0_SUBTYPE_PROBE_RESP)
				goto out;
		} else {
			if (subtype == IEEE80211_FC0_SUBTYPE_BEACON)
				goto out;
		}

		if (ifp->if_flags & IFF_DEBUG)
			printf("%s: received %s from %s\n",
			    ifp->if_xname,
			    ieee80211_mgt_subtype_name[subtype >> 4],
			    ether_sprintf(wh->i_addr2));
		switch (subtype) {
		case IEEE80211_FC0_SUBTYPE_PROBE_RESP:
		case IEEE80211_FC0_SUBTYPE_BEACON:
			ieee80211_recv_beacon(ic, m, rssi, timoff);
			break;
		case IEEE80211_FC0_SUBTYPE_AUTH:
			ieee80211_recv_auth(ic, m);
			break;
		case IEEE80211_FC0_SUBTYPE_ASSOC_RESP:
		case IEEE80211_FC0_SUBTYPE_REASSOC_RESP:
			ieee80211_recv_asresp(ic, m);
			break;
		case IEEE80211_FC0_SUBTYPE_DEAUTH:
			ieee80211_recv_deauth(ic, m);
			break;
		case IEEE80211_FC0_SUBTYPE_DISASSOC:
			ieee80211_recv_disassoc(ic, m);
			break;
		default:
			break;
		}
		goto out;

	case IEEE80211_FC0_TYPE_CTL:
	default:
		DPRINTF(("ieee80211_input: bad type %x\n", wh->i_fc[0]));
		/* should not come here */
		break;
	}
  err:
	ifp->if_ierrors++;
  out:
	if (m != NULL)
		m_freem(m);
}

int
ieee80211_mgmt_output(struct ifnet *ifp, struct mbuf *m, int type)
{
	struct ieee80211com *ic = (void *)ifp;
	struct ieee80211_frame *wh;

	M_PREPEND(m, sizeof(struct ieee80211_frame), M_DONTWAIT);
	if (m == NULL)
		return ENOMEM;
	wh = mtod(m, struct ieee80211_frame *);
	wh->i_fc[0] = IEEE80211_FC0_VERSION_0 | IEEE80211_FC0_TYPE_MGT | type;
	wh->i_fc[1] = IEEE80211_FC1_DIR_NODS;
	*(u_int16_t *)wh->i_dur = 0;
	*(u_int16_t *)wh->i_seq =
	    htole16(ic->ic_bss.bs_txseq << IEEE80211_SEQ_SEQ_SHIFT);
	ic->ic_bss.bs_txseq++;
	memcpy(wh->i_addr1, ic->ic_bss.bs_macaddr, IEEE80211_ADDR_LEN);
	memcpy(wh->i_addr2, ic->ic_myaddr, IEEE80211_ADDR_LEN);
	memcpy(wh->i_addr3, ic->ic_bss.bs_bssid, IEEE80211_ADDR_LEN);

	if (ifp->if_flags & IFF_DEBUG)
		printf("%s: sending %s to %s\n", ifp->if_xname,
		    ieee80211_mgt_subtype_name[(type >> 4) & 0xf],
		    ether_sprintf(ic->ic_bss.bs_bssid));
	IF_ENQUEUE(&ic->ic_mgtq, m);
	ic->ic_mgt_timer = IEEE80211_TRANS_WAIT;
	ifp->if_timer = 1;
	(*ifp->if_start)(ifp);
	return 0;
}

struct mbuf *
ieee80211_encap(struct ifnet *ifp, struct mbuf *m)
{
	struct ieee80211com *ic = (void *)ifp;
	struct ether_header eh;
	struct ieee80211_frame *wh;
	struct llc *llc;

	if (m->m_len < sizeof(struct ether_header)) {
		m = m_pullup(m, sizeof(struct ether_header));
		if (m == NULL)
			return NULL;
	}
	memcpy(&eh, mtod(m, caddr_t), sizeof(struct ether_header));
	m_adj(m, sizeof(struct ether_header) - sizeof(struct llc));
	llc = mtod(m, struct llc *);
	llc->llc_dsap = llc->llc_ssap = LLC_SNAP_LSAP;
	llc->llc_control = LLC_UI;
	llc->llc_snap.org_code[0] = 0;
	llc->llc_snap.org_code[1] = 0;
	llc->llc_snap.org_code[2] = 0;
	llc->llc_snap.ether_type = eh.ether_type;
	M_PREPEND(m, sizeof(struct ieee80211_frame), M_DONTWAIT);
	if (m == NULL)
		return NULL;
	wh = mtod(m, struct ieee80211_frame *);
	wh->i_fc[0] = IEEE80211_FC0_VERSION_0 | IEEE80211_FC0_TYPE_DATA;
	*(u_int16_t *)wh->i_dur = 0;
	*(u_int16_t *)wh->i_seq =
	    htole16(ic->ic_bss.bs_txseq << IEEE80211_SEQ_SEQ_SHIFT);
	ic->ic_bss.bs_txseq++;
	if (ic->ic_flags & IEEE80211_F_ADHOC) {
		wh->i_fc[1] = IEEE80211_FC1_DIR_NODS;
		memcpy(wh->i_addr1, eh.ether_dhost, IEEE80211_ADDR_LEN);
		memcpy(wh->i_addr2, eh.ether_shost, IEEE80211_ADDR_LEN);
		memcpy(wh->i_addr3, ic->ic_bss.bs_bssid, IEEE80211_ADDR_LEN);
	} else {
		wh->i_fc[1] = IEEE80211_FC1_DIR_TODS;
		memcpy(wh->i_addr1, ic->ic_bss.bs_bssid, IEEE80211_ADDR_LEN);
		memcpy(wh->i_addr2, eh.ether_shost, IEEE80211_ADDR_LEN);
		memcpy(wh->i_addr3, eh.ether_dhost, IEEE80211_ADDR_LEN);
	}
	return m;
}

struct mbuf *
ieee80211_decap(struct ifnet *ifp, struct mbuf *m)
{
	struct ether_header *eh;
	struct ieee80211_frame wh;
	struct llc *llc;

	if (m->m_len < sizeof(wh) + sizeof(*llc)) {
		m = m_pullup(m, sizeof(wh) + sizeof(*llc));
		if (m == NULL)
			return NULL;
	}
	memcpy(&wh, mtod(m, caddr_t), sizeof(wh));
	llc = (struct llc *)(mtod(m, caddr_t) + sizeof(wh));
	if (llc->llc_dsap == LLC_SNAP_LSAP && llc->llc_ssap == LLC_SNAP_LSAP &&
	    llc->llc_control == LLC_UI && llc->llc_snap.org_code[0] == 0 &&
	    llc->llc_snap.org_code[1] == 0 && llc->llc_snap.org_code[2] == 0) {
		m_adj(m, sizeof(wh) + sizeof(struct llc) - sizeof(*eh));
		llc = NULL;
	} else {
		m_adj(m, sizeof(wh) - sizeof(*eh));
	}
	eh = mtod(m, struct ether_header *);
	switch (wh.i_fc[1] & IEEE80211_FC1_DIR_MASK) {
	case IEEE80211_FC1_DIR_NODS:
		memcpy(eh->ether_dhost, wh.i_addr1, IEEE80211_ADDR_LEN);
		memcpy(eh->ether_shost, wh.i_addr2, IEEE80211_ADDR_LEN);
		break;
	case IEEE80211_FC1_DIR_TODS:
		memcpy(eh->ether_dhost, wh.i_addr3, IEEE80211_ADDR_LEN);
		memcpy(eh->ether_shost, wh.i_addr2, IEEE80211_ADDR_LEN);
		break;
	case IEEE80211_FC1_DIR_FROMDS:
		memcpy(eh->ether_dhost, wh.i_addr1, IEEE80211_ADDR_LEN);
		memcpy(eh->ether_shost, wh.i_addr3, IEEE80211_ADDR_LEN);
		break;
	case IEEE80211_FC1_DIR_DSTODS:
		DPRINTF(("ieee80211_decap: DS to DS\n"));
		m_freem(m);
		return NULL;
	}
	if (!ALIGNED_POINTER(mtod(m, caddr_t) + sizeof(*eh), u_int32_t)) {
		struct mbuf *n, *n0, **np;
		caddr_t newdata;
		int off;

		n0 = NULL;
		np = &n0;
		off = 0;
		while (m->m_pkthdr.len > off) {
			if (n0 == NULL) {
				MGETHDR(n, M_DONTWAIT, MT_DATA);
				if (n == NULL) {
					m_freem(m);
					return NULL;
				}
				M_COPY_PKTHDR(n, m);
				n->m_len = MHLEN;
			} else {
				MGET(n, M_DONTWAIT, MT_DATA);
				if (n == NULL) {
					m_freem(m);
					m_freem(n0);
					return NULL;
				}
				n->m_len = MLEN;
			}
			if (m->m_pkthdr.len - off >= MINCLSIZE) {
				MCLGET(n, M_DONTWAIT);
				if (n->m_flags & M_EXT)
					n->m_len = n->m_ext.ext_size;
			}
			if (n0 == NULL) {
				newdata =
				    (caddr_t)ALIGN(n->m_data + sizeof(*eh)) -
				    sizeof(*eh);
				n->m_len -= newdata - n->m_data;
				n->m_data = newdata;
			}
			if (n->m_len > m->m_pkthdr.len - off)
				n->m_len = m->m_pkthdr.len - off;
			m_copydata(m, off, n->m_len, mtod(n, caddr_t));
			off += n->m_len;
			*np = n;
			np = &n->m_next;
		}
		m_freem(m);
		m = n0;
	}
	if (llc != NULL) {
		eh = mtod(m, struct ether_header *);
		eh->ether_type = htons(m->m_pkthdr.len - sizeof(*eh));
	}
	return m;
}

int
ieee80211_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct ieee80211com *ic = (void *)ifp;
	struct ifreq *ifr = (struct ifreq *)data;
	int i, error = 0;
	struct ieee80211_nwid nwid;
	struct ieee80211_nwkey *nwkey;
	struct ieee80211_power *power;
	struct ieee80211_wepkey keys[IEEE80211_WEP_NKID];

	switch (cmd) {
	case SIOCS80211NWID:
		if ((error = copyin(ifr->ifr_data, &nwid, sizeof(nwid))) != 0)
			break;
		if (nwid.i_len > IEEE80211_NWID_LEN) {
			error = EINVAL;
			break;
		}
		memset(ic->ic_des_essid, 0, IEEE80211_NWID_LEN);
		ic->ic_des_esslen = nwid.i_len;
		memcpy(ic->ic_des_essid, nwid.i_nwid, nwid.i_len);
		error = ENETRESET;
		break;
	case SIOCG80211NWID:
		memset(&nwid, 0, sizeof(nwid));
		switch (ic->ic_state) {
		case IEEE80211_S_INIT:
		case IEEE80211_S_SCAN:
			nwid.i_len = ic->ic_des_esslen;
			memcpy(nwid.i_nwid, ic->ic_des_essid, nwid.i_len);
			break;
		default:
			nwid.i_len = ic->ic_bss.bs_esslen;
			memcpy(nwid.i_nwid, ic->ic_bss.bs_essid, nwid.i_len);
			break;
		}
		error = copyout(&nwid, ifr->ifr_data, sizeof(nwid));
		break;
	case SIOCS80211NWKEY:
		nwkey = (struct ieee80211_nwkey *)data;
		if (nwkey->i_wepon == IEEE80211_NWKEY_OPEN) {
			if (ic->ic_flags & IEEE80211_F_WEPON) {
				error = ENETRESET;
				ic->ic_flags &= ~IEEE80211_F_WEPON;
			}
			break;
		}
		if ((ic->ic_flags & IEEE80211_F_HASWEP) == 0) {
			error = EINVAL;
			break;
		}
		/* check and copy keys */
		memset(keys, 0, sizeof(keys));
		for (i = 0; i < IEEE80211_WEP_NKID; i++) {
			keys[i].wk_len = nwkey->i_key[i].i_keylen;
			if ((keys[i].wk_len > 0 &&
			    keys[i].wk_len < IEEE80211_WEP_KEYLEN) ||
			    keys[i].wk_len > sizeof(keys[i].wk_key)) {
				error = EINVAL;
				break;
			}
			if (keys[i].wk_len <= 0)
				continue;
			if ((error = copyin(nwkey->i_key[i].i_keydat,
			    keys[i].wk_key, keys[i].wk_len)) != 0)
				break;
		}
		if (error)
			break;
		i = nwkey->i_defkid - 1;
		if (i < 0 || i >= IEEE80211_WEP_NKID ||
		    keys[i].wk_len == 0 ||
		    (keys[i].wk_len == -1 && ic->ic_nw_keys[i].wk_len == 0)) {
			error = EINVAL;
			break;
		}
		/* save the key */
		ic->ic_flags |= IEEE80211_F_WEPON;
		ic->ic_wep_txkey = i;
		for (i = 0; i < IEEE80211_WEP_NKID; i++) {
			if (keys[i].wk_len < 0)
				continue;
			ic->ic_nw_keys[i].wk_len = keys[i].wk_len;
			memcpy(ic->ic_nw_keys[i].wk_key, keys[i].wk_key,
			    sizeof(keys[i].wk_key));
		}
		error = ENETRESET;
		break;
	case SIOCG80211NWKEY:
		nwkey = (struct ieee80211_nwkey *)data;
		if (ic->ic_flags & IEEE80211_F_WEPON)
			nwkey->i_wepon = IEEE80211_NWKEY_WEP;
		else
			nwkey->i_wepon = IEEE80211_NWKEY_OPEN;
		nwkey->i_defkid = ic->ic_wep_txkey + 1;
		for (i = 0; i < IEEE80211_WEP_NKID; i++) {
			if (nwkey->i_key[i].i_keydat == NULL)
				continue;
			/* do not show any keys to non-root user */
			if ((error = suser(curproc->p_ucred,
			    &curproc->p_acflag)) != 0)
				break;
			nwkey->i_key[i].i_keylen = ic->ic_nw_keys[i].wk_len;
			if ((error = copyout(ic->ic_nw_keys[i].wk_key,
			    nwkey->i_key[i].i_keydat,
			    ic->ic_nw_keys[i].wk_len)) != 0)
				break;
		}
		break;
	case SIOCS80211POWER:
		power = (struct ieee80211_power *)data;
		ic->ic_lintval = power->i_maxsleep;
		if (power->i_enabled != 0) {
			if ((ic->ic_flags & IEEE80211_F_HASPMGT) == 0)
				error = EINVAL;
			else if ((ic->ic_flags & IEEE80211_F_PMGTON) == 0) {
				ic->ic_flags |= IEEE80211_F_PMGTON;
				error = ENETRESET;
			}
		} else {
			if (ic->ic_flags & IEEE80211_F_PMGTON) {
				ic->ic_flags &= ~IEEE80211_F_PMGTON;
				error = ENETRESET;
			}
		}
		break;
	case SIOCG80211POWER:
		power = (struct ieee80211_power *)data;
		power->i_enabled = (ic->ic_flags & IEEE80211_F_PMGTON) ? 1 : 0;
		power->i_maxsleep = ic->ic_lintval;
		break;
	case SIOCGIFGENERIC:
		error = ieee80211_cfgget(ifp, cmd, data);
		break;
	case SIOCSIFGENERIC:
		error = suser(curproc->p_ucred, 
		    &curproc->p_acflag);
		if (error)
			break;
		error = ieee80211_cfgset(ifp, cmd, data);
		break;
	default:
		error = ether_ioctl(ifp, cmd, data);
		break;
	}
	return error;
}

void
ieee80211_print_essid(u_int8_t *essid, int len)
{
	int i;
	u_int8_t *p; 

	if (len > IEEE80211_NWID_LEN)
		len = IEEE80211_NWID_LEN;       /*XXX*/
	/* determine printable or not */
	for (i = 0, p = essid; i < len; i++, p++) {
		if (*p < ' ' || *p > 0x7e)
			break;
	}
	if (i == len) {
		printf("\"");
		for (i = 0, p = essid; i < len; i++, p++)
			printf("%c", *p);
		printf("\"");
	} else {
		printf("0x");
		for (i = 0, p = essid; i < len; i++, p++)
			printf("%02x", *p);
	}
}

void
ieee80211_dump_pkt(u_int8_t *buf, int len, int rate, int rssi)
{
	struct ieee80211_frame *wh;
	int i;

	wh = (struct ieee80211_frame *)buf;
	switch (wh->i_fc[1] & IEEE80211_FC1_DIR_MASK) {
	case IEEE80211_FC1_DIR_NODS:
		printf("NODS %s", ether_sprintf(wh->i_addr2));
		printf("->%s", ether_sprintf(wh->i_addr1));
		printf("(%s)", ether_sprintf(wh->i_addr3));
		break;
	case IEEE80211_FC1_DIR_TODS:
		printf("TODS %s", ether_sprintf(wh->i_addr2));
		printf("->%s", ether_sprintf(wh->i_addr3));
		printf("(%s)", ether_sprintf(wh->i_addr1));
		break;
	case IEEE80211_FC1_DIR_FROMDS:
		printf("FRDS %s", ether_sprintf(wh->i_addr3));
		printf("->%s", ether_sprintf(wh->i_addr1));
		printf("(%s)", ether_sprintf(wh->i_addr2));
		break;
	case IEEE80211_FC1_DIR_DSTODS:
		printf("DSDS %s", ether_sprintf((u_int8_t *)&wh[1]));
		printf("->%s", ether_sprintf(wh->i_addr3));
		printf("(%s", ether_sprintf(wh->i_addr2));
		printf("->%s)", ether_sprintf(wh->i_addr1));
		break;
	}
	switch (wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) {
	case IEEE80211_FC0_TYPE_DATA:
		printf(" data");
		break;
	case IEEE80211_FC0_TYPE_MGT:
		printf(" %s", ieee80211_mgt_subtype_name[
		    (wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) >> 4]);
		break;
	default:
		printf(" type#%d", wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK);
		break;
	}
	if (wh->i_fc[1] & IEEE80211_FC1_WEP)
		printf(" WEP");
	if (rate >= 0)
		printf(" %dM", rate / 2);
	if (rssi >= 0)
		printf(" +%d", rssi);
	printf("\n");
	if (len > 0) {
		for (i = 0; i < len; i++) {
			if ((i & 1) == 0)
				printf(" ");
			printf("%02x", buf[i]);
		}
		printf("\n");
	}
}

void
ieee80211_watchdog(struct ifnet *ifp)
{
	struct ieee80211com *ic = (void *)ifp;

	if (ic->ic_scan_timer) {
		if (--ic->ic_scan_timer == 0) {
			if (ic->ic_state == IEEE80211_S_SCAN)
				ieee80211_end_scan(ifp);
		}
	}
	if (ic->ic_mgt_timer) {
		if (--ic->ic_mgt_timer == 0)
			ieee80211_new_state(ifp, IEEE80211_S_SCAN, -1);
	}
	if (ic->ic_scan_timer != 0 || ic->ic_mgt_timer != 0)
		ifp->if_timer = 1;
}

void
ieee80211_next_scan(struct ifnet *ifp)
{
	struct ieee80211com *ic = (void *)ifp;
	int chan;

	chan = ic->ic_bss.bs_chan;
	for (;;) {
		chan = (chan + 1) % (IEEE80211_CHAN_MAX + 1);
		if (isset(ic->ic_chan_active, chan))
			break;
		if (chan == ic->ic_bss.bs_chan) {
			DPRINTF(("ieee80211_next_scan: no chan available\n"));
			return;
		}
	}
	DPRINTF(("ieee80211_next_scan: chan %d->%d\n",
	    ic->ic_bss.bs_chan, chan));
	ic->ic_bss.bs_chan = chan;
	ieee80211_new_state(ifp, IEEE80211_S_SCAN, -1);
}

void
ieee80211_end_scan(struct ifnet *ifp)
{
	struct ieee80211com *ic = (void *)ifp;
	struct ieee80211_bss *bs, *nextbs, *selbs;
	u_int8_t rate;
	int i, fail;

	bs = TAILQ_FIRST(&ic->ic_scan);
	if (bs == NULL) {
		DPRINTF(("ieee80211_end_scan: no scan candidate\n"));
  notfound:
		if ((ic->ic_flags & IEEE80211_F_ADHOC) &&
		    (ic->ic_flags & IEEE80211_F_IBSSON) &&
		    ic->ic_des_esslen != 0) {
			bs = &ic->ic_bss;
			if (ifp->if_flags & IFF_DEBUG)
				printf("%s: creating ibss\n", ifp->if_xname);
			ic->ic_flags |= IEEE80211_F_SIBSS;
			bs->bs_nrate = 0;
			for (i = 0; i < IEEE80211_RATE_SIZE; i++) {
				if (ic->ic_sup_rates[i])
					bs->bs_rates[bs->bs_nrate++] =
					    ic->ic_sup_rates[i];
			}
			memcpy(bs->bs_macaddr, ic->ic_myaddr,
			    IEEE80211_ADDR_LEN);
			memcpy(bs->bs_bssid, ic->ic_myaddr, IEEE80211_ADDR_LEN);
			bs->bs_bssid[0] |= 0x02;	/* local bit for IBSS */
			bs->bs_esslen = ic->ic_des_esslen;
			memcpy(bs->bs_essid, ic->ic_des_essid, bs->bs_esslen);
			bs->bs_rssi = 0;
			bs->bs_timoff = 0;
			memset(bs->bs_tstamp, 0, sizeof(bs->bs_tstamp));
			bs->bs_intval = ic->ic_lintval;
			bs->bs_capinfo = IEEE80211_CAPINFO_IBSS;
			if (ic->ic_flags & IEEE80211_F_WEPON)
				bs->bs_capinfo |= IEEE80211_CAPINFO_PRIVACY;
			bs->bs_chan = ic->ic_ibss_chan;
			bs->bs_fhdwell = 200;	/* XXX */
			bs->bs_fhindex = 1;
			ieee80211_new_state(ifp, IEEE80211_S_RUN, -1);
			return;
		}
		if (ic->ic_flags & IEEE80211_F_ASCAN) {
			if (ifp->if_flags & IFF_DEBUG)
				printf("%s: entering passive scan mode\n",
				    ifp->if_xname);
			ic->ic_flags &= ~IEEE80211_F_ASCAN;
		}
		ieee80211_next_scan(ifp);
		return;
	}
	selbs = NULL;
	if (ifp->if_flags & IFF_DEBUG)
		printf("%s:\tmacaddr     chan  rssi rate flag  wep  essid\n",
		    ifp->if_xname);
	for (; bs != NULL; bs = nextbs) {
		nextbs = TAILQ_NEXT(bs, bs_list);
		if (bs->bs_fails) {
			/*
			 * The configuration of the access points may change
			 * during my scan.  So delete the entry for the AP
			 * and retry to associate if there is another beacon.
			 */
			if (bs->bs_fails++ > 2) {
				TAILQ_REMOVE(&ic->ic_scan, bs, bs_list);
				free(bs, M_DEVBUF);
			}
			continue;
		}
		fail = 0;
		if (isclr(ic->ic_chan_active, bs->bs_chan))
			fail |= 0x01;
		if (ic->ic_flags & IEEE80211_F_ADHOC) {
			if ((bs->bs_capinfo & IEEE80211_CAPINFO_IBSS) == 0)
				fail |= 0x02;
		} else {
			if ((bs->bs_capinfo & IEEE80211_CAPINFO_ESS) == 0)
				fail |= 0x02;
		}
		if (ic->ic_flags & IEEE80211_F_WEPON) {
			if ((bs->bs_capinfo & IEEE80211_CAPINFO_PRIVACY) == 0)
				fail |= 0x04;
		} else {
			if (bs->bs_capinfo & IEEE80211_CAPINFO_PRIVACY)
				fail |= 0x04;
		}
		rate = ieee80211_fix_rate(ic, bs, IEEE80211_F_DONEGO);
		if (rate & IEEE80211_RATE_BASIC)
			fail |= 0x08;
		if (ic->ic_des_esslen != 0 &&
		    (bs->bs_esslen != ic->ic_des_esslen ||
		     memcmp(bs->bs_essid, ic->ic_des_essid,
		     ic->ic_des_esslen != 0)))
			fail |= 0x10;
		if (ifp->if_flags & IFF_DEBUG) {
			printf(" %c %s", fail ? '-' : '+',
			    ether_sprintf(bs->bs_macaddr));
			printf(" %3d%c", bs->bs_chan, fail & 0x01 ? '!' : ' ');
			printf(" %+4d", bs->bs_rssi);
			printf(" %2dM%c", (rate & IEEE80211_RATE_VAL) / 2,
			    fail & 0x08 ? '!' : ' ');
			printf(" %4s%c",
			    (bs->bs_capinfo & IEEE80211_CAPINFO_ESS) ? "ess" :
			    (bs->bs_capinfo & IEEE80211_CAPINFO_IBSS) ? "ibss" :
			    "????",
			    fail & 0x02 ? '!' : ' ');
			printf(" %3s%c ",
			    (bs->bs_capinfo & IEEE80211_CAPINFO_PRIVACY) ?
			    "wep" : "no",
			    fail & 0x04 ? '!' : ' ');
			ieee80211_print_essid(bs->bs_essid, bs->bs_esslen);
			printf("%s\n", fail & 0x10 ? "!" : "");
		}
		if (!fail) {
			if (selbs == NULL || bs->bs_rssi > selbs->bs_rssi)
				selbs = bs;
		}
	}
	if (selbs == NULL)
		goto notfound;
	ic->ic_bss = *selbs;
	if (ic->ic_flags & IEEE80211_F_ADHOC) {
		ieee80211_fix_rate(ic, &ic->ic_bss, IEEE80211_F_DOFRATE |
		    IEEE80211_F_DONEGO | IEEE80211_F_DODEL);
		if (ic->ic_bss.bs_nrate == 0) {
			selbs->bs_fails++;
			goto notfound;
		}
		ieee80211_new_state(ifp, IEEE80211_S_RUN, -1);
	} else
		ieee80211_new_state(ifp, IEEE80211_S_AUTH, -1);
}

void
ieee80211_free_scan(struct ifnet *ifp)
{
	struct ieee80211com *ic = (void *)ifp;
	struct ieee80211_bss *bs;

	while ((bs = TAILQ_FIRST(&ic->ic_scan)) != NULL) {
		TAILQ_REMOVE(&ic->ic_scan, bs, bs_list);
		free(bs, M_DEVBUF);
	}
}

int
ieee80211_fix_rate(struct ieee80211com *ic, struct ieee80211_bss *bs, int flags)
{
	int i, j, ignore, error;
	int okrate, badrate;
	u_int8_t r;

	error = 0;
	okrate = badrate = 0;
	for (i = 0; i < bs->bs_nrate; ) {
		ignore = 0;
		if (flags & IEEE80211_F_DOSORT) {
			for (j = i + 1; j < bs->bs_nrate; j++) {
				if ((bs->bs_rates[i] & IEEE80211_RATE_VAL) >
				    (bs->bs_rates[j] & IEEE80211_RATE_VAL)) {
					r = bs->bs_rates[i];
					bs->bs_rates[i] = bs->bs_rates[j];
					bs->bs_rates[j] = r;
				}
			}
		}
		r = bs->bs_rates[i] & IEEE80211_RATE_VAL;
		badrate = r;
		if (flags & IEEE80211_F_DOFRATE) {
			if (ic->ic_fixed_rate >= 0 &&
			    r != (ic->ic_sup_rates[ic->ic_fixed_rate] &
			    IEEE80211_RATE_VAL))
				ignore++;
		}
		if (flags & IEEE80211_F_DONEGO) {
			for (j = 0; j < IEEE80211_RATE_SIZE; j++) {
				if (r ==
				    (ic->ic_sup_rates[j] & IEEE80211_RATE_VAL))
					break;
			}
			if (j == IEEE80211_RATE_SIZE) {
				if (bs->bs_rates[i] & IEEE80211_RATE_BASIC)
					error++;
				ignore++;
			}
		}
		if (flags & IEEE80211_F_DODEL) {
			if (ignore) {
				bs->bs_nrate--;
				for (j = i; j < bs->bs_nrate; j++)
					bs->bs_rates[j] = bs->bs_rates[j + 1];
				bs->bs_rates[j] = 0;
				continue;
			}
		}
		if (!ignore)
			okrate = bs->bs_rates[i];
		i++;
	}
	if (okrate == 0 || error != 0)
		return badrate | IEEE80211_RATE_BASIC;
	return okrate & IEEE80211_RATE_VAL;
}

static void
ieee80211_send_prreq(struct ieee80211com *ic)
{
	int i;
	struct mbuf *m;
	u_int8_t *frm;

	/*
	 * prreq frame format
	 *	[tlv] ssid
	 *	[tlv] supported rates
	 */
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return;
	m->m_data += sizeof(struct ieee80211_frame);
	frm = mtod(m, u_int8_t *);

	*frm++ = IEEE80211_ELEMID_SSID;
	*frm++ = ic->ic_des_esslen;
	memcpy(frm, ic->ic_des_essid, ic->ic_des_esslen);
	frm += ic->ic_des_esslen;

	*frm++ = IEEE80211_ELEMID_RATES;
	for (i = 0; i < IEEE80211_RATE_SIZE; i++) {
		if (ic->ic_sup_rates[i] != 0)
			frm[i + 1] = ic->ic_sup_rates[i];
	}
	*frm++ = i;
	frm += i;
	m->m_pkthdr.len = m->m_len = frm - mtod(m, u_int8_t *);

	/* initialize ic_bss for probe response */
	memcpy(ic->ic_bss.bs_macaddr, ieee80211_bcast_addr, IEEE80211_ADDR_LEN);
	memcpy(ic->ic_bss.bs_bssid, ieee80211_bcast_addr, IEEE80211_ADDR_LEN);
	ic->ic_bss.bs_nrate = 0;
	memset(ic->ic_bss.bs_rates, 0, IEEE80211_RATE_SIZE);
	for (i = 0; i < IEEE80211_RATE_SIZE; i++) {
		if (ic->ic_sup_rates[i] != 0)
			ic->ic_bss.bs_rates[ic->ic_bss.bs_nrate++] =
			    ic->ic_sup_rates[i];
	}
	ic->ic_bss.bs_associd = 0;
	ic->ic_bss.bs_timoff = 0;

	ieee80211_mgmt_output(&ic->ic_if, m, IEEE80211_FC0_SUBTYPE_PROBE_REQ);
}

static void
ieee80211_send_auth(struct ieee80211com *ic, int seq)
{
	struct mbuf *m;
	u_int16_t *frm;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return;
	MH_ALIGN(m, 2 * 3);
	m->m_pkthdr.len = m->m_len = 6;
	frm = mtod(m, u_int16_t *);
	/* TODO: shared key auth */
	frm[0] = htole16(IEEE80211_AUTH_ALG_OPEN);
	frm[1] = htole16(seq);
	frm[2] = 0;			/* status */
	ieee80211_mgmt_output(&ic->ic_if, m, IEEE80211_FC0_SUBTYPE_AUTH);
}

static void
ieee80211_send_deauth(struct ieee80211com *ic, int reason)
{
	struct mbuf *m;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return;
	MH_ALIGN(m, 2);
	m->m_pkthdr.len = m->m_len = 2;
	*mtod(m, u_int16_t *) = htole16(reason);
	ieee80211_mgmt_output(&ic->ic_if, m, IEEE80211_FC0_SUBTYPE_DEAUTH);
}

static void
ieee80211_send_asreq(struct ieee80211com *ic, int reassoc)
{
	struct mbuf *m;
	u_int8_t *frm, *rates;
	u_int16_t capinfo;
	int i;

	/*
	 * asreq frame format
	 *	[2] capability information
	 *	[2] listen interval
	 *	[6*] current AP address (reassoc only)
	 *	[tlv] ssid
	 *	[tlv] supported rates
	 */
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return;
	m->m_data += sizeof(struct ieee80211_frame);
	frm = mtod(m, u_int8_t *);

	capinfo = 0;
	if (ic->ic_flags & IEEE80211_F_ADHOC)
		capinfo |= IEEE80211_CAPINFO_IBSS;
	else
		capinfo |= IEEE80211_CAPINFO_ESS;
	if (ic->ic_flags & IEEE80211_F_WEPON)
		capinfo |= IEEE80211_CAPINFO_PRIVACY;
	*(u_int16_t *)frm = htole16(capinfo);
	frm += 2;

	*(u_int16_t *)frm = htole16(ic->ic_lintval);
	frm += 2;

	if (reassoc) {
		memcpy(frm, ic->ic_bss.bs_bssid, IEEE80211_ADDR_LEN);
		frm += IEEE80211_ADDR_LEN;
	}

	*frm++ = IEEE80211_ELEMID_SSID;
	*frm++ = ic->ic_bss.bs_esslen;
	memcpy(frm, ic->ic_bss.bs_essid, ic->ic_bss.bs_esslen);
	frm += ic->ic_bss.bs_esslen;

	*frm++ = IEEE80211_ELEMID_RATES;
	rates = frm++;	/* update later */
	for (i = 0; i < IEEE80211_RATE_SIZE; i++) {
		if (ic->ic_bss.bs_rates[i] != 0)
			*frm++ = ic->ic_bss.bs_rates[i];
	}
	*rates = frm - (rates + 1);
	m->m_pkthdr.len = m->m_len = frm - mtod(m, u_int8_t *);
	ieee80211_mgmt_output(&ic->ic_if, m,
	    reassoc ?  IEEE80211_FC0_SUBTYPE_REASSOC_REQ :
	    IEEE80211_FC0_SUBTYPE_ASSOC_REQ);
}

static void
ieee80211_send_disassoc(struct ieee80211com *ic, int reason)
{
	struct mbuf *m;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return;
	MH_ALIGN(m, 2);
	m->m_pkthdr.len = m->m_len = 2;
	*mtod(m, u_int16_t *) = htole16(reason);
	ieee80211_mgmt_output(&ic->ic_if, m, IEEE80211_FC0_SUBTYPE_DISASSOC);
}

static void
ieee80211_recv_beacon(struct ieee80211com *ic, struct mbuf *m0, int rssi,
    u_int timoff)
{
	struct ieee80211_frame *wh;
	struct ieee80211_bss *bs;
	u_int8_t *frm, *efrm, *tstamp, *bintval, *capinfo, *ssid, *rates;
	u_int8_t chan, fhindex;
	u_int16_t fhdwell;

	if ((ic->ic_flags & IEEE80211_F_ADHOC) == 0 &&
	    ic->ic_state != IEEE80211_S_SCAN) {
		/* XXX: may be useful for background scan */
		return;
	}

	wh = mtod(m0, struct ieee80211_frame *);
	frm = (u_int8_t *)&wh[1];
	efrm = mtod(m0, u_int8_t *) + m0->m_len;
	/*
	 * beacon frame format
	 *	[8] time stamp
	 *	[2] beacon interval
	 *	[2] cabability information
	 *	[tlv] ssid
	 *	[tlv] supported rates
	 *	[tlv] parameter set (FH/DS)
	 */
	tstamp  = frm;	frm += 8;
	bintval = frm;	frm += 2;
	capinfo = frm;	frm += 2;
	ssid = rates = NULL;
	chan = ic->ic_bss.bs_chan;
	fhdwell = 0;
	fhindex = 0;
	while (frm < efrm) {
		switch (*frm) {
		case IEEE80211_ELEMID_SSID:
			ssid = frm;
			break;
		case IEEE80211_ELEMID_RATES:
			rates = frm;
			break;
		case IEEE80211_ELEMID_FHPARMS:
			fhdwell = (frm[3] << 8) | frm[2];
			chan = IEEE80211_FH_CHAN(frm[4], frm[5]);
			fhindex = frm[6];
			break;
		case IEEE80211_ELEMID_DSPARMS:
			chan = frm[2];
			break;
		}
		frm += frm[1] + 2;
	}
	if (ssid == NULL || rates == NULL) {
		DPRINTF(("ieee80211_recv_beacon: ssid=%p, rates=%p, chan=%d\n",
		    ssid, rates, chan));
		return;
	}
	if (ssid[1] > IEEE80211_NWID_LEN) {
		DPRINTF(("ieee80211_recv_beacon: bad ssid len %d from %s\n",
		    ssid[1], ether_sprintf(wh->i_addr2)));
		return;
	}
	TAILQ_FOREACH(bs, &ic->ic_scan, bs_list) {
		if (memcmp(bs->bs_macaddr, wh->i_addr2,
		    IEEE80211_ADDR_LEN) == 0 &&
		    memcmp(bs->bs_bssid, wh->i_addr3,
		    IEEE80211_ADDR_LEN) == 0)
			break;
	}
	if (bs == NULL) {
		bs = malloc(sizeof(*bs), M_DEVBUF, M_NOWAIT);
		if (bs == NULL)
			return;
		memset(bs, 0, sizeof(*bs));
		DPRINTF(("ieee80211_recv_beacon: new beacon from %s\n",
		    ether_sprintf(wh->i_addr3)));
		TAILQ_INSERT_TAIL(&ic->ic_scan, bs, bs_list);
		memcpy(bs->bs_macaddr, wh->i_addr2, IEEE80211_ADDR_LEN);
		memcpy(bs->bs_bssid, wh->i_addr3, IEEE80211_ADDR_LEN);
		bs->bs_esslen = ssid[1];
		memset(bs->bs_essid, 0, sizeof(bs->bs_essid));
		memcpy(bs->bs_essid, ssid + 2, ssid[1]);
	} else if (ssid[1] != 0) {
		/*
		 * Update ESSID at probe response to adopt hidden AP by
		 * Lucent/Cisco, which announces null ESSID in beacon.
		 */
		if ((wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) ==
		    IEEE80211_FC0_SUBTYPE_PROBE_RESP) {
			bs->bs_esslen = ssid[1];
			memset(bs->bs_essid, 0, sizeof(bs->bs_essid));
			memcpy(bs->bs_essid, ssid + 2, ssid[1]);
		}
	}
	memset(bs->bs_rates, 0, IEEE80211_RATE_SIZE);
	bs->bs_nrate = rates[1];
	memcpy(bs->bs_rates, rates + 2, bs->bs_nrate);
	ieee80211_fix_rate(ic, bs, IEEE80211_F_DOSORT);
	bs->bs_rssi = rssi;
	bs->bs_timoff = timoff;
	memcpy(bs->bs_tstamp, tstamp, sizeof(bs->bs_tstamp));
	bs->bs_intval = le16toh(*(u_int16_t *)bintval);
	bs->bs_capinfo = le16toh(*(u_int16_t *)capinfo);
	bs->bs_chan = chan;
	bs->bs_fhdwell = fhdwell;
	bs->bs_fhindex = fhindex;
	if (ic->ic_state == IEEE80211_S_SCAN && ic->ic_scan_timer == 0)
		ieee80211_end_scan(&ic->ic_if);
}

static void
ieee80211_recv_auth(struct ieee80211com *ic, struct mbuf *m0)
{
	struct ieee80211_frame *wh;
	struct ieee80211_bss *bs;
	u_int8_t *frm, *efrm;
	u_int16_t algo, seq, status;

	wh = mtod(m0, struct ieee80211_frame *);
	frm = (u_int8_t *)&wh[1];
	efrm = mtod(m0, u_int8_t *) + m0->m_len;
	/*
	 * auth frame format
	 *	[2] algorithm
	 *	[2] sequence
	 *	[2] status
	 *	[tlv*] challenge
	 */
	if (frm + 6 > efrm) {
		DPRINTF(("ieee80211_recv_auth: too short from %s\n",
		    ether_sprintf(wh->i_addr2)));
		return;
	}
	algo   = le16toh(*(u_int16_t *)frm);
	seq    = le16toh(*(u_int16_t *)(frm + 2));
	status = le16toh(*(u_int16_t *)(frm + 4));
	if (algo != IEEE80211_AUTH_ALG_OPEN) {
		/* TODO: shared key auth */
		DPRINTF(("ieee80211_recv_auth: unsupported auth %d from %s\n",
		    algo, ether_sprintf(wh->i_addr2)));
		return;
	}
	if (ic->ic_flags & IEEE80211_F_ADHOC) {
		if (ic->ic_state != IEEE80211_S_RUN)
			return;
		if (seq == 1) {
			ieee80211_new_state(&ic->ic_if, IEEE80211_S_AUTH,
			    wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK);
			return;
		}
	}
	if (ic->ic_state != IEEE80211_S_AUTH || seq != 2)
		return;
	if (status != 0) {
		printf("%s: authentication failed (reason %d) for %s\n",
		    ic->ic_if.if_xname, status, ether_sprintf(wh->i_addr3));
		TAILQ_FOREACH(bs, &ic->ic_scan, bs_list) {
			if (memcmp(bs->bs_macaddr, wh->i_addr2,
			    IEEE80211_ADDR_LEN) == 0) {
				bs->bs_fails++;
				break;
			}
		}
		return;
	}
	ieee80211_new_state(&ic->ic_if, IEEE80211_S_ASSOC,
	    wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK);
}

static void
ieee80211_recv_asresp(struct ieee80211com *ic, struct mbuf *m0)
{
	struct ifnet *ifp = &ic->ic_if;
	struct ieee80211_frame *wh;
	struct ieee80211_bss *bs = &ic->ic_bss;
	u_int8_t *frm, *efrm, *rates;
	int status;

	wh = mtod(m0, struct ieee80211_frame *);
	frm = (u_int8_t *)&wh[1];
	efrm = mtod(m0, u_int8_t *) + m0->m_len;
	/*
	 * asresp frame format
	 *	[2] capability information
	 *	[2] status
	 *	[2] association ID
	 *	[tlv] supported rates
	 */
	if (frm + 6 > efrm) {
		DPRINTF(("ieee80211_recv_asresp: too short from %s\n",
		    ether_sprintf(wh->i_addr2)));
		return;
	}

	bs->bs_capinfo = le16toh(*(u_int16_t *)frm);
	frm += 2;

	status = le16toh(*(u_int16_t *)frm);
	frm += 2;
	if (status != 0) {
		printf("%s: association failed (reason %d) for %s\n",
		    ifp->if_xname, status, ether_sprintf(wh->i_addr3));
		TAILQ_FOREACH(bs, &ic->ic_scan, bs_list) {
			if (memcmp(bs->bs_macaddr, wh->i_addr2,
			    IEEE80211_ADDR_LEN) == 0) {
				bs->bs_fails++;
				break;
			}
		}
		return;
	}
	bs->bs_associd = le16toh(*(u_int16_t *)frm);
	frm += 2;
	rates = frm;

	memset(bs->bs_rates, 0, IEEE80211_RATE_SIZE);
	bs->bs_nrate = rates[1];
	memcpy(bs->bs_rates, rates + 2, bs->bs_nrate);
	ieee80211_fix_rate(ic, bs, IEEE80211_F_DOSORT | IEEE80211_F_DOFRATE |
	    IEEE80211_F_DONEGO | IEEE80211_F_DODEL);
	if (bs->bs_nrate == 0)
		return;
	ieee80211_new_state(ifp, IEEE80211_S_RUN,
	    wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK);
}

static void
ieee80211_recv_disassoc(struct ieee80211com *ic, struct mbuf *m0)
{
	struct ieee80211_frame *wh;

	wh = mtod(m0, struct ieee80211_frame *);
	if ((ic->ic_flags & IEEE80211_F_ADHOC) == 0)
		ieee80211_new_state(&ic->ic_if, IEEE80211_S_ASSOC,
		    wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK);
}

static void
ieee80211_recv_deauth(struct ieee80211com *ic, struct mbuf *m0)
{
	struct ieee80211_frame *wh;

	wh = mtod(m0, struct ieee80211_frame *);
	if ((ic->ic_flags & IEEE80211_F_ADHOC) == 0)
		ieee80211_new_state(&ic->ic_if, IEEE80211_S_AUTH,
		    wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK);
}

int
ieee80211_new_state(struct ifnet *ifp, enum ieee80211_state nstate, int mgt)
{
	struct ieee80211com *ic = (void *)ifp;
	struct ieee80211_bss *bs;
	int error, ostate;
#ifdef IEEE80211_DEBUG
	static const char *stname[] = 
	    { "INIT", "SCAN", "AUTH", "ASSOC", "RUN" };
#endif

	ostate = ic->ic_state;
	DPRINTF(("ieee80211_new_state: %s -> %s\n",
	    stname[ostate], stname[nstate]));
	if (ic->ic_newstate) {
		error = (*ic->ic_newstate)(ic->ic_softc, nstate);
		if (error == EINPROGRESS)
			return 0;
		if (error != 0)
			return error;
	}

	/* state transition */
	ic->ic_state = nstate;
	switch (nstate) {
	case IEEE80211_S_INIT:
		switch (ostate) {
		case IEEE80211_S_INIT:
			break;
		case IEEE80211_S_RUN:
			if ((ic->ic_flags & IEEE80211_F_ADHOC) == 0)
				ieee80211_send_disassoc(ic,
				    IEEE80211_REASON_ASSOC_LEAVE);
			/* FALLTHRU */
		case IEEE80211_S_ASSOC:
			ieee80211_send_deauth(ic, IEEE80211_REASON_AUTH_LEAVE);
			/* FALLTHRU */
		case IEEE80211_S_AUTH:
		case IEEE80211_S_SCAN:
			ic->ic_scan_timer = 0;
			ic->ic_mgt_timer = 0;
			IF_PURGE(&ic->ic_mgtq);
			if (ic->ic_wep_ctx != NULL) {
				free(ic->ic_wep_ctx, M_DEVBUF);
				ic->ic_wep_ctx = NULL;
			}
			ieee80211_free_scan(ifp);  
			break;
		}
		break;
	case IEEE80211_S_SCAN:
		switch (ostate) {
		case IEEE80211_S_INIT:
			ic->ic_flags |= IEEE80211_F_ASCAN;
			ic->ic_flags &= ~IEEE80211_F_SIBSS;
			ic->ic_scan_timer = IEEE80211_ASCAN_WAIT;
			/* use lowest rate */
			ic->ic_bss.bs_txrate = 0;
			ieee80211_send_prreq(ic);
			break;
		case IEEE80211_S_SCAN:
			/* scan next */
			ic->ic_flags &= ~IEEE80211_F_SIBSS;
			if (ic->ic_flags & IEEE80211_F_ASCAN) {
				if (ic->ic_scan_timer == 0)
					ic->ic_scan_timer =
					    IEEE80211_ASCAN_WAIT;
				ieee80211_send_prreq(ic);
			} else {
				if (ic->ic_scan_timer == 0)
					ic->ic_scan_timer =
					    IEEE80211_PSCAN_WAIT;
				ifp->if_timer = 1;
			}
			break;
		case IEEE80211_S_RUN:
			/* beacon miss */
			if (ifp->if_flags & IFF_DEBUG)
				printf("%s: no recent beacons from %s;"
				    " rescanning\n",
				    ifp->if_xname,
				    ether_sprintf(ic->ic_bss.bs_bssid));
			ieee80211_free_scan(ifp);
			/* FALLTHRU */
		case IEEE80211_S_AUTH:
		case IEEE80211_S_ASSOC:
			/* timeout restart scan */
			TAILQ_FOREACH(bs, &ic->ic_scan, bs_list) {
				if (memcmp(ic->ic_bss.bs_macaddr,
				    bs->bs_macaddr, IEEE80211_ADDR_LEN) == 0) {
					bs->bs_fails++;
					break;
				}
			}
			ic->ic_flags |= IEEE80211_F_ASCAN;
			ic->ic_scan_timer = IEEE80211_ASCAN_WAIT;
			ic->ic_flags &= ~IEEE80211_F_SIBSS;
			ieee80211_send_prreq(ic);
			break;
		}
		break;
	case IEEE80211_S_AUTH:
		switch (ostate) {
		case IEEE80211_S_INIT:
			DPRINTF(("ieee80211_new_state: invalid transition\n"));
			break;
		case IEEE80211_S_SCAN:
			ieee80211_send_auth(ic, 1);
			break;
		case IEEE80211_S_AUTH:
		case IEEE80211_S_ASSOC:
			switch (mgt) {
			case IEEE80211_FC0_SUBTYPE_AUTH:
				/* ??? */
				ieee80211_send_auth(ic, 2);
				break;
			case IEEE80211_FC0_SUBTYPE_DEAUTH:
				/* ignore and retry scan on timeout */
				break;
			}
			break;
		case IEEE80211_S_RUN:
			switch (mgt) {
			case IEEE80211_FC0_SUBTYPE_AUTH:
				ieee80211_send_auth(ic, 2);
				ic->ic_state = ostate;	/* stay RUN */
				break;
			case IEEE80211_FC0_SUBTYPE_DEAUTH:
				/* try to reauth */
				ieee80211_send_auth(ic, 1);
				break;
			}
			break;
		}
		break;
	case IEEE80211_S_ASSOC:
		switch (ostate) {
		case IEEE80211_S_INIT:
		case IEEE80211_S_SCAN:
		case IEEE80211_S_ASSOC:
			DPRINTF(("ieee80211_new_state: invalid transition\n"));
			break;
		case IEEE80211_S_AUTH:
			ieee80211_send_asreq(ic, 0);
			break;
		case IEEE80211_S_RUN:
			ieee80211_send_asreq(ic, 1);
			break;
		}
		break;
	case IEEE80211_S_RUN:
		switch (ostate) {
		case IEEE80211_S_INIT:
		case IEEE80211_S_AUTH:
		case IEEE80211_S_RUN:
			DPRINTF(("ieee80211_new_state: invalid transition\n"));
			break;
		case IEEE80211_S_SCAN:		/* adhoc mode */
		case IEEE80211_S_ASSOC:	/* infra mode */
			if (ifp->if_flags & IFF_DEBUG) {
				printf("%s: associated with %s ssid ",
				    ifp->if_xname,
				    ether_sprintf(ic->ic_bss.bs_bssid));
				ieee80211_print_essid(ic->ic_bss.bs_essid,
				    ic->ic_bss.bs_esslen);
				printf(" channel %d\n", ic->ic_bss.bs_chan);
			}
			/* start with highest negotiated rate */
			ic->ic_bss.bs_txrate = ic->ic_bss.bs_nrate - 1;
			ic->ic_mgt_timer = 0;
			(*ifp->if_start)(ifp);
			break;
		}
		break;
	}
	return 0;
}

struct mbuf *
ieee80211_wep_crypt(struct ifnet *ifp, struct mbuf *m0, int txflag)
{
	struct ieee80211com *ic = (void *)ifp;
	struct mbuf *m, *n, *n0;
	struct ieee80211_frame *wh;
	int left, len, moff, noff, kid;
	u_int32_t iv, crc;
	u_int8_t *ivp;
	void *ctx;
	u_int8_t keybuf[IEEE80211_WEP_IVLEN + IEEE80211_KEYBUF_SIZE];
	u_int8_t crcbuf[IEEE80211_WEP_CRCLEN];

	n0 = NULL;
	if ((ctx = ic->ic_wep_ctx) == NULL) {
		ctx = malloc(arc4_ctxlen(), M_DEVBUF, M_NOWAIT);
		if (ctx == NULL)
			goto fail;
		ic->ic_wep_ctx = ctx;
	}
	m = m0;
	left = m->m_pkthdr.len;
	MGET(n, M_DONTWAIT, m->m_type);
	n0 = n;
	if (n == NULL)
		goto fail;
	M_COPY_PKTHDR(n, m);
	len = IEEE80211_WEP_IVLEN + IEEE80211_WEP_KIDLEN + IEEE80211_WEP_CRCLEN;
	if (txflag) {
		n->m_pkthdr.len += len;
	} else {
		n->m_pkthdr.len -= len;
		left -= len;
	}
	n->m_len = MHLEN;
	if (n->m_pkthdr.len >= MINCLSIZE) {
		MCLGET(n, M_DONTWAIT);
		if (n->m_flags & M_EXT)
			n->m_len = n->m_ext.ext_size;
	}
	len = sizeof(struct ieee80211_frame);
	memcpy(mtod(n, caddr_t), mtod(m, caddr_t), len);
	wh = mtod(n, struct ieee80211_frame *);
	left -= len;
	moff = len;
	noff = len;
	if (txflag) {
		kid = ic->ic_wep_txkey;
		wh->i_fc[1] |= IEEE80211_FC1_WEP;
		/*
		 * XXX
		 * IV must not duplicate during the lifetime of the key.
		 * But no mechanism to renew keys is defined in IEEE 802.11
		 * WEP.  And IV may be duplicated between other stations
		 * because of the session key itself is shared.
		 * So we use pseudo random IV here, though it is not the
		 * best way.
		 */
		iv = random();
		ivp = mtod(n, u_int8_t *) + noff;
		memcpy(ivp, (caddr_t)&iv, IEEE80211_WEP_IVLEN);
		ivp[IEEE80211_WEP_IVLEN] = kid << 6;	/* pad and keyid */
		noff += IEEE80211_WEP_IVLEN + IEEE80211_WEP_KIDLEN;
	} else {
		wh->i_fc[1] &= ~IEEE80211_FC1_WEP;
		ivp = mtod(m, u_int8_t *) + moff;
		kid = ivp[IEEE80211_WEP_IVLEN] >> 6;
		moff += IEEE80211_WEP_IVLEN + IEEE80211_WEP_KIDLEN;
	}
	memcpy(keybuf, ivp, IEEE80211_WEP_IVLEN);
	memcpy(keybuf + IEEE80211_WEP_IVLEN, ic->ic_nw_keys[kid].wk_key,
	    ic->ic_nw_keys[kid].wk_len);
	arc4_setkey(ctx, keybuf,
	    IEEE80211_WEP_IVLEN + ic->ic_nw_keys[kid].wk_len);

	/* encrypt with calculating CRC */
	crc = ~0;
	while (left > 0) {
		len = m->m_len - moff;
		if (len == 0) {
			m = m->m_next;
			moff = 0;
			continue;
		}
		if (len > n->m_len - noff) {
			len = n->m_len - noff;
			if (len == 0) {
				MGET(n->m_next, M_DONTWAIT, n->m_type);
				if (n->m_next == NULL)
					goto fail;
				n = n->m_next;
				n->m_len = MLEN;
				if (left >= MINCLSIZE) {
					MCLGET(n, M_DONTWAIT);
					if (n->m_flags & M_EXT)
						n->m_len = n->m_ext.ext_size;
				}
				noff = 0;
				continue;
			}
		}
		if (len > left)
			len = left;
		arc4_encrypt(ctx, mtod(n, caddr_t) + noff,
		    mtod(m, caddr_t) + moff, len);
		if (txflag)
			crc = ieee80211_crc_update(crc,
			    mtod(m, u_int8_t *) + moff, len);
		else
			crc = ieee80211_crc_update(crc,
			    mtod(n, u_int8_t *) + noff, len);
		left -= len;
		moff += len;
		noff += len;
	}
	crc = ~crc;
	if (txflag) {
		*(u_int32_t *)crcbuf = htole32(crc);
		if (n->m_len >= noff + sizeof(crcbuf))
			n->m_len = noff + sizeof(crcbuf);
		else {
			n->m_len = noff;
			MGET(n->m_next, M_DONTWAIT, n->m_type);
			if (n->m_next == NULL)
				goto fail;
			n = n->m_next;
			n->m_len = sizeof(crcbuf);
			noff = 0;
		}
		arc4_encrypt(ctx, mtod(n, caddr_t) + noff, crcbuf,
		    sizeof(crcbuf));
	} else {
		n->m_len = noff;
		for (noff = 0; noff < sizeof(crcbuf); noff += len) {
			len = sizeof(crcbuf) - noff;
			if (len > m->m_len - moff)
				len = m->m_len - moff;
			if (len > 0)
				arc4_encrypt(ctx, crcbuf + noff,
				    mtod(m, caddr_t) + moff, len);
			m = m->m_next;
			moff = 0;
		}
		if (crc != le32toh(*(u_int32_t *)crcbuf)) {
#ifdef IEEE80211_DEBUG
			printf("ieee80211_wep_crypt: decrypt CRC error\n");
			if (ieee80211_debug)
				ieee80211_dump_pkt(n0->m_data, n0->m_len,
				    -1, -1);
#endif
			goto fail;
		}
	}
	m_freem(m0);
	return n0;

  fail:
	m_freem(m0);
	m_freem(n0);
	return NULL;
}

/*
 * CRC 32 -- routine from RFC 2083
 */

/* Table of CRCs of all 8-bit messages */
static u_int32_t ieee80211_crc_table[256];

/* Make the table for a fast CRC. */
static void
ieee80211_crc_init(void)
{
	u_int32_t c;
	int n, k;

	for (n = 0; n < 256; n++) {
		c = (u_int32_t)n;
		for (k = 0; k < 8; k++) {
			if (c & 1)
				c = 0xedb88320UL ^ (c >> 1);
			else
				c = c >> 1;
		}
		ieee80211_crc_table[n] = c;
	}
}

/*
 * Update a running CRC with the bytes buf[0..len-1]--the CRC
 * should be initialized to all 1's, and the transmitted value
 * is the 1's complement of the final running CRC
 */

static u_int32_t
ieee80211_crc_update(u_int32_t crc, u_int8_t *buf, int len)
{
	u_int8_t *endbuf;

	for (endbuf = buf + len; buf < endbuf; buf++)
		crc = ieee80211_crc_table[(crc ^ *buf) & 0xff] ^ (crc >> 8);
	return crc;
}

/*
 * XXX
 * Wireless LAN specific configuration interface, which is compatible
 * with wiconfig(8).
 */

int
ieee80211_cfgget(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct ieee80211com *ic = (void *)ifp;
	int i, error;
	struct ifreq *ifr = (struct ifreq *)data;
	struct wi_req wreq;
	struct wi_ltv_keys *keys;
#ifdef WICACHE
	struct wi_sigcache wsc;
	struct ieee80211_bss *bp;
#endif /* WICACHE */

	error = copyin(ifr->ifr_data, &wreq, sizeof(wreq));
	if (error)
		return error;
	wreq.wi_len = 0;
	switch (wreq.wi_type) {
	case WI_RID_SERIALNO:
		/* nothing appropriate */
		break;
	case WI_RID_NODENAME:
		strcpy((char *)&wreq.wi_val[1], hostname);
		wreq.wi_val[0] = strlen(hostname);
		wreq.wi_len = (1 + wreq.wi_val[0] + 1) / 2;
		break;
	case WI_RID_CURRENT_SSID:
		if (ic->ic_state == IEEE80211_S_RUN) {
			wreq.wi_val[0] = ic->ic_bss.bs_esslen;
			memcpy(&wreq.wi_val[1], ic->ic_bss.bs_essid,
			    ic->ic_bss.bs_esslen);
		} else {
			wreq.wi_val[0] = 0;
			wreq.wi_val[1] = '\0';
		}
		wreq.wi_len = (1 + wreq.wi_val[0] + 1) / 2;
		break;
	case WI_RID_OWN_SSID:
	case WI_RID_DESIRED_SSID:
		wreq.wi_val[0] = ic->ic_des_esslen;
		memcpy(&wreq.wi_val[1], ic->ic_des_essid, ic->ic_des_esslen);
		wreq.wi_len = (1 + wreq.wi_val[0] + 1) / 2;
		break;
	case WI_RID_CURRENT_BSSID:
		if (ic->ic_state == IEEE80211_S_RUN)
			memcpy(wreq.wi_val, ic->ic_bss.bs_bssid,
			    IEEE80211_ADDR_LEN);
		else
			memset(wreq.wi_val, 0, IEEE80211_ADDR_LEN);
		wreq.wi_len = IEEE80211_ADDR_LEN / 2;
		break;
	case WI_RID_CHANNEL_LIST:
		for (i = 0; i <= IEEE80211_CHAN_MAX; i += 16) {
			wreq.wi_val[i / 16] = ic->ic_chan_active[i / 8] |
			    (ic->ic_chan_active[i / 8 + 1] << 8);
			if (wreq.wi_val[i / 16] != 0)
				wreq.wi_len = i / 16 + 1;
		}
		break;
	case WI_RID_OWN_CHNL:
		wreq.wi_val[0] = ic->ic_ibss_chan;
		wreq.wi_len = 1;
		break;
	case WI_RID_CURRENT_CHAN:
		wreq.wi_val[0] = ic->ic_bss.bs_chan;
		wreq.wi_len = 1;
		break;
	case WI_RID_COMMS_QUALITY:
		wreq.wi_val[0] = 0;			/* quality */
		wreq.wi_val[1] = ic->ic_bss.bs_rssi;	/* signal */
		wreq.wi_val[2] = 0;			/* noise */
		wreq.wi_len = 3;
		break;
	case WI_RID_PROMISC:
		wreq.wi_val[0] = (ifp->if_flags & IFF_PROMISC) ? 1 : 0;
		wreq.wi_len = 1;
		break;
	case WI_RID_PORTTYPE:
		wreq.wi_val[0] = (ic->ic_flags & IEEE80211_F_ADHOC) ? 2 : 1;
		wreq.wi_len = 1;
		break;
	case WI_RID_MAC_NODE:
		memcpy(wreq.wi_val, ic->ic_myaddr, IEEE80211_ADDR_LEN);
		wreq.wi_len = IEEE80211_ADDR_LEN / 2;
		break;
	case WI_RID_TX_RATE:
		if (ic->ic_fixed_rate == -1)
			wreq.wi_val[0] = 0;	/* auto */
		else
			wreq.wi_val[0] = (ic->ic_sup_rates[ic->ic_fixed_rate] &
			    IEEE80211_RATE_VAL) / 2;
		wreq.wi_len = 1;
		break;
	case WI_RID_CUR_TX_RATE:
		wreq.wi_val[0] = (ic->ic_bss.bs_rates[ic->ic_bss.bs_txrate] &
		    IEEE80211_RATE_VAL) / 2;
		wreq.wi_len = 1;
		break;
	case WI_RID_RTS_THRESH:
		wreq.wi_val[0] = IEEE80211_MAX_LEN;	/* TODO: RTS */
		wreq.wi_len = 1;
		break;
	case WI_RID_CREATE_IBSS:
		wreq.wi_val[0] = (ic->ic_flags & IEEE80211_F_IBSSON) ? 1 : 0;
		wreq.wi_len = 1;
		break;
	case WI_RID_MICROWAVE_OVEN:
		wreq.wi_val[0] = 0;	/* no ... not supported */
		wreq.wi_len = 1;
		break;
	case WI_RID_ROAMING_MODE:
		wreq.wi_val[0] = 1;	/* enabled ... not supported */
		wreq.wi_len = 1;
		break;
	case WI_RID_SYSTEM_SCALE:
		wreq.wi_val[0] = 1;	/* low density ... not supported */
		wreq.wi_len = 1;
		break;
	case WI_RID_PM_ENABLED:
		wreq.wi_val[0] = (ic->ic_flags & IEEE80211_F_PMGTON) ? 1 : 0;
		wreq.wi_len = 1;
		break;
	case WI_RID_MAX_SLEEP:
		wreq.wi_val[0] = ic->ic_lintval;
		wreq.wi_len = 1;
		break;
	case WI_RID_WEP_AVAIL:
		wreq.wi_val[0] = (ic->ic_flags & IEEE80211_F_HASWEP) ? 1 : 0;
		wreq.wi_len = 1;
		break;
	case WI_RID_AUTH_CNTL:
		wreq.wi_val[0] = 1;	/* open system authentication only */
		wreq.wi_len = 1;
		break;
	case WI_RID_ENCRYPTION:
		wreq.wi_val[0] = (ic->ic_flags & IEEE80211_F_WEPON) ? 1 : 0;
		wreq.wi_len = 1;
		break;
	case WI_RID_TX_CRYPT_KEY:
		wreq.wi_val[0] = ic->ic_wep_txkey;
		wreq.wi_len = 1;
		break;
	case WI_RID_DEFLT_CRYPT_KEYS:
		keys = (struct wi_ltv_keys *)&wreq;
		/* do not show keys to non-root user */
		error = suser(curproc->p_ucred, 
		    &curproc->p_acflag);
		if (error) {
			memset(keys, 0, sizeof(*keys));
			error = 0;
			break;
		}
		for (i = 0; i < IEEE80211_WEP_NKID; i++) {
			keys->wi_keys[i].wi_keylen = ic->ic_nw_keys[i].wk_len;
			memcpy(keys->wi_keys[i].wi_keydat,
			    ic->ic_nw_keys[i].wk_key, ic->ic_nw_keys[i].wk_len);
		}
		wreq.wi_len = sizeof(*keys) / 2;
		break;
	case WI_RID_MAX_DATALEN:
		wreq.wi_val[0] = IEEE80211_MAX_LEN;	/* TODO: fragment */
		wreq.wi_len = 1;
		break;
	case WI_RID_IFACE_STATS:
		/* not implemented yet */
		wreq.wi_len = 0;
		break;
#ifdef WICACHE
	case WI_RID_READ_CACHE:
		i = 0;
		TAILQ_FOREACH(bs, &ic->ic_scan, bs_list) {
			if (i == MAXCACHE)
				break;
			memcpy(wsc.macsrc, bs->bs_macaddr, IEEE80211_ADDR_LEN);
			memset(&wsc.ipsrc, 0, sizeof(wsc.ipsrc));
			wsc.signal = bp->rssi;
			wsc.noise = 0;
			wsc.quality = 0;
			memcpy((caddr_t)wreq.wi_val + sizeof(wsc) * i,
			    &wsc, sizeof(wsc));
			i++;
		}
		wreq.wi_len = sizeof(wsc) * i / 2;
		break;
#endif /* WICACHE */
	default:
		error = EINVAL;
		break;
	}
	if (error == 0) {
		wreq.wi_len++;
		error = copyout(&wreq, ifr->ifr_data, sizeof(wreq));
	}
	return error;
}

int
ieee80211_cfgset(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct ieee80211com *ic = (void *)ifp;
	int i, error;
	struct ifreq *ifr = (struct ifreq *)data;
	struct wi_ltv_keys *keys;
	struct wi_req wreq;
	u_char chanlist[(IEEE80211_CHAN_MAX+1)/NBBY];

	error = copyin(ifr->ifr_data, &wreq, sizeof(wreq));
	if (error)
		return error;
	if (wreq.wi_len-- < 1)
		return EINVAL;
	switch (wreq.wi_type) {
	case WI_RID_SERIALNO:
	case WI_RID_NODENAME:
		return EPERM;
	case WI_RID_CURRENT_SSID:
		return EPERM;
	case WI_RID_OWN_SSID:
	case WI_RID_DESIRED_SSID:
		if (wreq.wi_len < (1 + wreq.wi_val[0] + 1) / 2)
			return EINVAL;
		if (wreq.wi_val[0] > IEEE80211_NWID_LEN)
			return EINVAL;
		ic->ic_des_esslen = wreq.wi_val[0];
		memset(ic->ic_des_essid, 0, sizeof(ic->ic_des_essid));
		memcpy(ic->ic_des_essid, &wreq.wi_val[1], ic->ic_des_esslen);
		error = ENETRESET;
		break;
	case WI_RID_CURRENT_BSSID:
		return EPERM;
	case WI_RID_CHANNEL_LIST:
		if (wreq.wi_len > (IEEE80211_CHAN_MAX + 1) / 16)
			return EINVAL;
		memset(chanlist, 0, sizeof(chanlist));
		for (i = 0; i < wreq.wi_len; i++) {
			chanlist[i * 2] = wreq.wi_val[i] & 0xff;
			chanlist[i * 2 + 1] = wreq.wi_val[i] >> 8;
		}
		error = EINVAL;
		for (i = 0; i <= IEEE80211_CHAN_MAX; i++) {
			if (isclr(chanlist, i))
				continue;
			if (isclr(ic->ic_chan_avail, i)) {
				if (ic->ic_chancheck == NULL)
					return EPERM;
				error = (*ic->ic_chancheck)(ic->ic_softc,
				    chanlist);
				break;
			}
			error = 0;
		}
		if (error == 0) {
			memcpy(ic->ic_chan_active, chanlist,
			    sizeof(ic->ic_chan_active));
			error = ENETRESET;
		}
		break;
	case WI_RID_OWN_CHNL:
		if (wreq.wi_len != 1)
			return EINVAL;
		if (isclr(ic->ic_chan_active, wreq.wi_val[0]))
			return EINVAL;
		ic->ic_ibss_chan = wreq.wi_val[0];
		if (ic->ic_flags & IEEE80211_F_SIBSS)
			error = ENETRESET;
		break;
	case WI_RID_CURRENT_CHAN:
		return EPERM;
	case WI_RID_COMMS_QUALITY:
		return EPERM;
	case WI_RID_PROMISC:
		if (wreq.wi_len != 1)
			return EINVAL;
		if (ifp->if_flags & IFF_PROMISC) {
			if (wreq.wi_val[0] == 0) {
				ifp->if_flags &= ~IFF_PROMISC;
				error = ENETRESET;
			}
		} else {
			if (wreq.wi_val[0] != 0) {
				ifp->if_flags |= IFF_PROMISC;
				error = ENETRESET;
			}
		}
		break;
	case WI_RID_PORTTYPE:
		if (wreq.wi_len != 1)
			return EINVAL;
		switch (wreq.wi_val[0]) {
		case 1:
			if (ic->ic_flags & IEEE80211_F_ADHOC) {
				ic->ic_flags &= ~IEEE80211_F_ADHOC;
				error = ENETRESET;
			}
			break;
		case 2:
			if ((ic->ic_flags & IEEE80211_F_ADHOC) == 0) {
				ic->ic_flags |= IEEE80211_F_ADHOC;
				error = ENETRESET;
			}
			break;
		default:
			return EINVAL;
		}
		break;
	case WI_RID_MAC_NODE:
		/* XXX: should be implemented? */
		return EPERM;
	case WI_RID_TX_RATE:
		if (wreq.wi_len != 1)
			return EINVAL;
		if (wreq.wi_val[0] == 0) {
			/* auto */
			ic->ic_fixed_rate = -1;
			break;
		}
		for (i = 0; i < IEEE80211_RATE_SIZE; i++) {
			if (wreq.wi_val[0] ==
			    (ic->ic_sup_rates[i] & IEEE80211_RATE_VAL) / 2)
				break;
		}
		if (i == IEEE80211_RATE_SIZE)
			return EINVAL;
		ic->ic_fixed_rate = i;
		error = ENETRESET;
		break;
	case WI_RID_CUR_TX_RATE:
		return EPERM;
		break;
	case WI_RID_RTS_THRESH:
		if (wreq.wi_len != 1)
			return EINVAL;
		if (wreq.wi_val[0] != IEEE80211_MAX_LEN)
			return EINVAL;		/* TODO: RTS */
		break;
	case WI_RID_CREATE_IBSS:
		if (wreq.wi_len != 1)
			return EINVAL;
		if (wreq.wi_val[0]) {
			if ((ic->ic_flags & IEEE80211_F_HASIBSS) == 0)
				return EINVAL;
			if ((ic->ic_flags & IEEE80211_F_IBSSON) == 0) {
				ic->ic_flags |= IEEE80211_F_IBSSON;
				if ((ic->ic_flags & IEEE80211_F_ADHOC) &&
				    ic->ic_state == IEEE80211_S_SCAN)
					error = ENETRESET;
			}
		} else {
			if (ic->ic_flags & IEEE80211_F_IBSSON) {
				ic->ic_flags &= ~IEEE80211_F_IBSSON;
				if (ic->ic_flags & IEEE80211_F_SIBSS) {
					ic->ic_flags &= ~IEEE80211_F_SIBSS;
					error = ENETRESET;
				}
			}
		}
		break;
	case WI_RID_MICROWAVE_OVEN:
		if (wreq.wi_len != 1)
			return EINVAL;
		if (wreq.wi_val[0] != 0)
			return EINVAL;		/* not supported */
		break;
	case WI_RID_ROAMING_MODE:
		if (wreq.wi_len != 1)
			return EINVAL;
		if (wreq.wi_val[0] != 1)
			return EINVAL;		/* not supported */
		break;
	case WI_RID_SYSTEM_SCALE:
		if (wreq.wi_len != 1)
			return EINVAL;
		if (wreq.wi_val[0] != 1)
			return EINVAL;		/* not supported */
		break;
	case WI_RID_PM_ENABLED:
		if (wreq.wi_len != 1)
			return EINVAL;
		if (wreq.wi_val[0] != 0) {
			if ((ic->ic_flags & IEEE80211_F_HASPMGT) == 0)
				return EINVAL;
			if ((ic->ic_flags & IEEE80211_F_PMGTON) == 0) {
				ic->ic_flags |= IEEE80211_F_PMGTON;
				error = ENETRESET;
			}
		} else {
			if (ic->ic_flags & IEEE80211_F_PMGTON) {
				ic->ic_flags &= ~IEEE80211_F_PMGTON;
				error = ENETRESET;
			}
		}
		break;
	case WI_RID_MAX_SLEEP:
		if (wreq.wi_len != 1)
			return EINVAL;
		ic->ic_lintval = wreq.wi_val[0];
		break;
	case WI_RID_WEP_AVAIL:
		return EPERM;
	case WI_RID_AUTH_CNTL:
		if (wreq.wi_len != 1)
			return EINVAL;
		if (wreq.wi_val[0] != 1)
			return EINVAL;		/* TODO: shared key auth */
		break;
	case WI_RID_ENCRYPTION:
		if (wreq.wi_len != 1)
			return EINVAL;
		if (wreq.wi_val[0]) {
			if ((ic->ic_flags & IEEE80211_F_HASWEP) == 0)
				return EINVAL;
			if ((ic->ic_flags & IEEE80211_F_WEPON) == 0) {
				ic->ic_flags |= IEEE80211_F_WEPON;
				error = ENETRESET;
			}
		} else {
			if (ic->ic_flags & IEEE80211_F_WEPON) {
				ic->ic_flags &= ~IEEE80211_F_WEPON;
				error = ENETRESET;
			}
		}
		break;
	case WI_RID_TX_CRYPT_KEY:
		if (wreq.wi_len != 1)
			return EINVAL;
		if (wreq.wi_val[0] >= IEEE80211_WEP_NKID)
			return EINVAL;
		ic->ic_wep_txkey = wreq.wi_val[0];
		break;
	case WI_RID_DEFLT_CRYPT_KEYS:
		if (wreq.wi_len != sizeof(struct wi_ltv_keys) / 2)
			return EINVAL;
		keys = (struct wi_ltv_keys *)&wreq;
		for (i = 0; i < IEEE80211_WEP_NKID; i++) {
			if (keys->wi_keys[i].wi_keylen != 0 &&
			    keys->wi_keys[i].wi_keylen < IEEE80211_WEP_KEYLEN)
				return EINVAL;
			if (keys->wi_keys[i].wi_keylen >
			    sizeof(ic->ic_nw_keys[i].wk_key))
				return EINVAL;
		}
		memset(ic->ic_nw_keys, 0, sizeof(ic->ic_nw_keys));
		for (i = 0; i < IEEE80211_WEP_NKID; i++) {
			ic->ic_nw_keys[i].wk_len = keys->wi_keys[i].wi_keylen;
			memcpy(ic->ic_nw_keys[i].wk_key,
			    keys->wi_keys[i].wi_keydat,
			    ic->ic_nw_keys[i].wk_len);
		}
		error = ENETRESET;
		break;
	case WI_RID_MAX_DATALEN:
		if (wreq.wi_len != 1)
			return EINVAL;
		if (wreq.wi_val[0] < 350 /* ? */ ||
		    wreq.wi_val[0] > IEEE80211_MAX_LEN)
			return EINVAL;
		if (wreq.wi_val[0] != IEEE80211_MAX_LEN)
			return EINVAL;		/* TODO: fragment */
		break;
	case WI_RID_IFACE_STATS:
		error = EPERM;
		break;
	default:
		error = EINVAL;
		break;
	}
	return error;
}

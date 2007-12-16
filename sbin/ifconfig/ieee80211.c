
/*
 * Copyright (c) 1983, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: ieee80211.c,v 1.11 2007/12/16 13:49:21 degroote Exp $");
#endif /* not lint */

#include <sys/param.h> 
#include <sys/ioctl.h> 
#include <sys/socket.h>

#include <net/if.h> 
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/route.h>
#include <net80211/ieee80211.h>
#include <net80211/ieee80211_ioctl.h>
#include <net80211/ieee80211_netbsd.h>

#include <ctype.h>
#include <err.h>
#include <netdb.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <util.h>

#include "extern.h"
#include "ieee80211.h"

static void set80211(int, int, int, u_int8_t *);
static u_int ieee80211_mhz2ieee(u_int, u_int);
static int getmaxrate(const uint8_t [15], u_int8_t);
static const char * getcaps(int);
static void printie(const char*, const uint8_t *, size_t, int);
static int copy_essid(char [], size_t, const u_int8_t *, size_t);
static void scan_and_wait(void);
static void list_scan(void);
static int mappsb(u_int , u_int);
static int mapgsm(u_int , u_int);

static void printies(const u_int8_t *, int, int);
static void printie(const char* , const uint8_t *, size_t , int);
static void printwmeparam(const char *, const u_int8_t *, size_t , int);
static void printwmeinfo(const char *, const u_int8_t *, size_t , int);
static const char * wpa_cipher(const u_int8_t *);
static const char * wpa_keymgmt(const u_int8_t *);
static void printwpaie(const char *, const u_int8_t *, size_t , int);
static const char * rsn_cipher(const u_int8_t *);
static const char * rsn_keymgmt(const u_int8_t *);
static void printrsnie(const char *, const u_int8_t *, size_t , int);
static void printssid(const char *, const u_int8_t *, size_t , int);
static void printrates(const char *, const u_int8_t *, size_t , int);
static void printcountry(const char *, const u_int8_t *, size_t , int);
static int iswpaoui(const u_int8_t *);
static int iswmeinfo(const u_int8_t *);
static int iswmeparam(const u_int8_t *);
static const char * iename(int);

extern int vflag;

static void
set80211(int type, int val, int len, u_int8_t *data)
{       
	struct ieee80211req	ireq;   
        
	(void) memset(&ireq, 0, sizeof(ireq));
	estrlcpy(ireq.i_name, name, sizeof(ireq.i_name));
	ireq.i_type = type;
	ireq.i_val = val;
	ireq.i_len = len;
	ireq.i_data = data;
	if (ioctl(s, SIOCS80211, &ireq) < 0)
		err(1, "SIOCS80211");   
}       

void
sethidessid(const char *val, int d)
{
	set80211(IEEE80211_IOC_HIDESSID, d, 0, NULL);
}

void                    
setapbridge(const char *val, int d)
{
	set80211(IEEE80211_IOC_APBRIDGE, d, 0, NULL);
}

static enum ieee80211_opmode
get80211opmode(void)
{
	struct ifmediareq ifmr;
                
	(void) memset(&ifmr, 0, sizeof(ifmr)); 
	estrlcpy(ifmr.ifm_name, name, sizeof(ifmr.ifm_name));
	if (ioctl(s, SIOCGIFMEDIA, (caddr_t)&ifmr) >= 0) {
		if (ifmr.ifm_current & IFM_IEEE80211_ADHOC)
			return IEEE80211_M_IBSS;        /* XXX ahdemo */
		if (ifmr.ifm_current & IFM_IEEE80211_HOSTAP)
			return IEEE80211_M_HOSTAP;
		if (ifmr.ifm_current & IFM_IEEE80211_MONITOR)
			return IEEE80211_M_MONITOR;
	}

	return IEEE80211_M_STA;  
}

void
setifnwid(const char *val, int d)
{
	struct ieee80211_nwid nwid;
	int len;

	len = sizeof(nwid.i_nwid);
	if (get_string(val, NULL, nwid.i_nwid, &len) == NULL)
		return;
	nwid.i_len = len;
	estrlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	ifr.ifr_data = (void *)&nwid;
	if (ioctl(s, SIOCS80211NWID, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCS80211NWID");
}

void
setifbssid(const char *val, int d)
{
	struct ieee80211_bssid bssid;
	struct ether_addr *ea;

	if (d != 0) {
		/* no BSSID is especially desired */
		memset(&bssid.i_bssid, 0, sizeof(bssid.i_bssid));
	} else {
		ea = ether_aton(val);
		if (ea == NULL) {
			errx(EXIT_FAILURE, "malformed BSSID: %s", val);
			return;
		}
		memcpy(&bssid.i_bssid, ea->ether_addr_octet,
		    sizeof(bssid.i_bssid));
	}
	estrlcpy(bssid.i_name, name, sizeof(bssid.i_name));
	if (ioctl(s, SIOCS80211BSSID, &bssid) == -1)
		err(EXIT_FAILURE, "SIOCS80211BSSID");
}

void
setiffrag(const char *val, int d)
{
	struct ieee80211req ireq;
	int thr;

	if (d != 0)
		thr = IEEE80211_FRAG_MAX;
	else {
		thr = atoi(val);
		if (thr < IEEE80211_FRAG_MIN || thr > IEEE80211_FRAG_MAX) {
			errx(EXIT_FAILURE, "invalid fragmentation threshold: %s", val);
			return;
		}
	}

	estrlcpy(ireq.i_name, name, sizeof(ireq.i_name));
	ireq.i_type = IEEE80211_IOC_FRAGTHRESHOLD;
	ireq.i_val = thr;
	if (ioctl(s, SIOCS80211, &ireq) == -1)
		err(EXIT_FAILURE, "IEEE80211_IOC_FRAGTHRESHOLD");
}

void
setifchan(const char *val, int d)
{
	struct ieee80211chanreq channel;
	int chan;

	if (d != 0)
		chan = IEEE80211_CHAN_ANY;
	else {
		chan = atoi(val);
		if (chan < 0 || chan > 0xffff) {
			errx(EXIT_FAILURE, "invalid channel: %s", val);
		}
	}

	estrlcpy(channel.i_name, name, sizeof(channel.i_name));
	channel.i_channel = (u_int16_t) chan;
	if (ioctl(s, SIOCS80211CHANNEL, &channel) == -1)
		err(EXIT_FAILURE, "SIOCS80211CHANNEL");
}

void
setifnwkey(const char *val, int d)
{
	struct ieee80211_nwkey nwkey;
	int i;
	u_int8_t keybuf[IEEE80211_WEP_NKID][16];

	nwkey.i_wepon = IEEE80211_NWKEY_WEP;
	nwkey.i_defkid = 1;
	for (i = 0; i < IEEE80211_WEP_NKID; i++) {
		nwkey.i_key[i].i_keylen = sizeof(keybuf[i]);
		nwkey.i_key[i].i_keydat = keybuf[i];
	}
	if (d != 0) {
		/* disable WEP encryption */
		nwkey.i_wepon = 0;
		i = 0;
	} else if (strcasecmp("persist", val) == 0) {
		/* use all values from persistent memory */
		nwkey.i_wepon |= IEEE80211_NWKEY_PERSIST;
		nwkey.i_defkid = 0;
		for (i = 0; i < IEEE80211_WEP_NKID; i++)
			nwkey.i_key[i].i_keylen = -1;
	} else if (strncasecmp("persist:", val, 8) == 0) {
		val += 8;
		/* program keys in persistent memory */
		nwkey.i_wepon |= IEEE80211_NWKEY_PERSIST;
		goto set_nwkey;
	} else {
  set_nwkey:
		if (isdigit((unsigned char)val[0]) && val[1] == ':') {
			/* specifying a full set of four keys */
			nwkey.i_defkid = val[0] - '0';
			val += 2;
			for (i = 0; i < IEEE80211_WEP_NKID; i++) {
				val = get_string(val, ",", keybuf[i],
				    &nwkey.i_key[i].i_keylen);
				if (val == NULL)
					return;
			}
			if (*val != '\0') {
				errx(EXIT_FAILURE, "SIOCS80211NWKEY: too many keys.");
			}
		} else {
			val = get_string(val, NULL, keybuf[0],
			    &nwkey.i_key[0].i_keylen);
			if (val == NULL)
				return;
			i = 1;
		}
	}
	for (; i < IEEE80211_WEP_NKID; i++)
		nwkey.i_key[i].i_keylen = 0;
	estrlcpy(nwkey.i_name, name, sizeof(nwkey.i_name));
	if (ioctl(s, SIOCS80211NWKEY, &nwkey) == -1)
		err(EXIT_FAILURE, "SIOCS80211NWKEY");
}

void
setifpowersave(const char *val, int d)
{
	struct ieee80211_power power;

	estrlcpy(power.i_name, name, sizeof(power.i_name));
	if (ioctl(s, SIOCG80211POWER, &power) == -1) {
		err(EXIT_FAILURE, "SIOCG80211POWER");
	}

	power.i_enabled = d;
	if (ioctl(s, SIOCS80211POWER, &power) == -1)
		err(EXIT_FAILURE, "SIOCS80211POWER");
}

void
setifpowersavesleep(const char *val, int d)
{
	struct ieee80211_power power;

	estrlcpy(power.i_name, name, sizeof(power.i_name));
	if (ioctl(s, SIOCG80211POWER, &power) == -1) {
		err(EXIT_FAILURE, "SIOCG80211POWER");
	}

	power.i_maxsleep = atoi(val);
	if (ioctl(s, SIOCS80211POWER, &power) == -1)
		err(EXIT_FAILURE, "SIOCS80211POWER");
}

void
setiflist(const char *val, int dummy)
{
	if (strncasecmp("scan", val, 4) == 0) {
		/* Get list of pre-scanned stations. */
		scan_and_wait();
		list_scan();
	}
	else
		errx(EXIT_FAILURE, "Don't know how to list %s for %s", val, name);
}

void
ieee80211_statistics(void)
{
	struct ieee80211_stats stats;

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_buflen = sizeof(stats);
	ifr.ifr_buf = (caddr_t)&stats;
	estrlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, (zflag) ? SIOCG80211ZSTATS : SIOCG80211STATS,
	    (caddr_t)&ifr) == -1)
		return;
#define	STAT_PRINT(_member, _desc)	\
	printf("\t" _desc ": %" PRIu32 "\n", stats._member)

	STAT_PRINT(is_rx_badversion, "rx frame with bad version");
	STAT_PRINT(is_rx_tooshort, "rx frame too short");
	STAT_PRINT(is_rx_wrongbss, "rx from wrong bssid");
	STAT_PRINT(is_rx_dup, "rx discard 'cuz dup");
	STAT_PRINT(is_rx_wrongdir, "rx w/ wrong direction");
	STAT_PRINT(is_rx_mcastecho, "rx discard 'cuz mcast echo");
	STAT_PRINT(is_rx_notassoc, "rx discard 'cuz sta !assoc");
	STAT_PRINT(is_rx_noprivacy, "rx w/ wep but privacy off");
	STAT_PRINT(is_rx_unencrypted, "rx w/o wep and privacy on");
	STAT_PRINT(is_rx_wepfail, "rx wep processing failed");
	STAT_PRINT(is_rx_decap, "rx decapsulation failed");
	STAT_PRINT(is_rx_mgtdiscard, "rx discard mgt frames");
	STAT_PRINT(is_rx_ctl, "rx discard ctrl frames");
	STAT_PRINT(is_rx_beacon, "rx beacon frames");
	STAT_PRINT(is_rx_rstoobig, "rx rate set truncated");
	STAT_PRINT(is_rx_elem_missing, "rx required element missing");
	STAT_PRINT(is_rx_elem_toobig, "rx element too big");
	STAT_PRINT(is_rx_elem_toosmall, "rx element too small");
	STAT_PRINT(is_rx_elem_unknown, "rx element unknown");
	STAT_PRINT(is_rx_badchan, "rx frame w/ invalid chan");
	STAT_PRINT(is_rx_chanmismatch, "rx frame chan mismatch");
	STAT_PRINT(is_rx_nodealloc, "rx frame dropped");
	STAT_PRINT(is_rx_ssidmismatch, "rx frame ssid mismatch ");
	STAT_PRINT(is_rx_auth_unsupported, "rx w/ unsupported auth alg");
	STAT_PRINT(is_rx_auth_fail, "rx sta auth failure");
	STAT_PRINT(is_rx_auth_countermeasures, "rx auth discard 'cuz CM");
	STAT_PRINT(is_rx_assoc_bss, "rx assoc from wrong bssid");
	STAT_PRINT(is_rx_assoc_notauth, "rx assoc w/o auth");
	STAT_PRINT(is_rx_assoc_capmismatch, "rx assoc w/ cap mismatch");
	STAT_PRINT(is_rx_assoc_norate, "rx assoc w/ no rate match");
	STAT_PRINT(is_rx_assoc_badwpaie, "rx assoc w/ bad WPA IE");
	STAT_PRINT(is_rx_deauth, "rx deauthentication");
	STAT_PRINT(is_rx_disassoc, "rx disassociation");
	STAT_PRINT(is_rx_badsubtype, "rx frame w/ unknown subtyp");
	STAT_PRINT(is_rx_nobuf, "rx failed for lack of buf");
	STAT_PRINT(is_rx_decryptcrc, "rx decrypt failed on crc");
	STAT_PRINT(is_rx_ahdemo_mgt, "rx discard ahdemo mgt fram");
	STAT_PRINT(is_rx_bad_auth, "rx bad auth request");
	STAT_PRINT(is_rx_unauth, "rx on unauthorized port");
	STAT_PRINT(is_rx_badkeyid, "rx w/ incorrect keyid");
	STAT_PRINT(is_rx_ccmpreplay, "rx seq# violation (CCMP)");
	STAT_PRINT(is_rx_ccmpformat, "rx format bad (CCMP)");
	STAT_PRINT(is_rx_ccmpmic, "rx MIC check failed (CCMP)");
	STAT_PRINT(is_rx_tkipreplay, "rx seq# violation (TKIP)");
	STAT_PRINT(is_rx_tkipformat, "rx format bad (TKIP)");
	STAT_PRINT(is_rx_tkipmic, "rx MIC check failed (TKIP)");
	STAT_PRINT(is_rx_tkipicv, "rx ICV check failed (TKIP)");
	STAT_PRINT(is_rx_badcipher, "rx failed 'cuz key type");
	STAT_PRINT(is_rx_nocipherctx, "rx failed 'cuz key !setup");
	STAT_PRINT(is_rx_acl, "rx discard 'cuz acl policy");

	STAT_PRINT(is_tx_nobuf, "tx failed for lack of buf");
	STAT_PRINT(is_tx_nonode, "tx failed for no node");
	STAT_PRINT(is_tx_unknownmgt, "tx of unknown mgt frame");
	STAT_PRINT(is_tx_badcipher, "tx failed 'cuz key type");
	STAT_PRINT(is_tx_nodefkey, "tx failed 'cuz no defkey");
	STAT_PRINT(is_tx_noheadroom, "tx failed 'cuz no space");
	STAT_PRINT(is_tx_fragframes, "tx frames fragmented");
	STAT_PRINT(is_tx_frags, "tx fragments created");

	STAT_PRINT(is_scan_active, "active scans started");
	STAT_PRINT(is_scan_passive, "passive scans started");
	STAT_PRINT(is_node_timeout, "nodes timed out inactivity");
	STAT_PRINT(is_crypto_nomem, "no memory for crypto ctx");
	STAT_PRINT(is_crypto_tkip, "tkip crypto done in s/w");
	STAT_PRINT(is_crypto_tkipenmic, "tkip en-MIC done in s/w");
	STAT_PRINT(is_crypto_tkipdemic, "tkip de-MIC done in s/w");
	STAT_PRINT(is_crypto_tkipcm, "tkip counter measures");
	STAT_PRINT(is_crypto_ccmp, "ccmp crypto done in s/w");
	STAT_PRINT(is_crypto_wep, "wep crypto done in s/w");
	STAT_PRINT(is_crypto_setkey_cipher, "cipher rejected key");
	STAT_PRINT(is_crypto_setkey_nokey, "no key index for setkey");
	STAT_PRINT(is_crypto_delkey, "driver key delete failed");
	STAT_PRINT(is_crypto_badcipher, "unknown cipher");
	STAT_PRINT(is_crypto_nocipher, "cipher not available");
	STAT_PRINT(is_crypto_attachfail, "cipher attach failed");
	STAT_PRINT(is_crypto_swfallback, "cipher fallback to s/w");
	STAT_PRINT(is_crypto_keyfail, "driver key alloc failed");
	STAT_PRINT(is_crypto_enmicfail, "en-MIC failed");
	STAT_PRINT(is_ibss_capmismatch, "merge failed-cap mismatch");
	STAT_PRINT(is_ibss_norate, "merge failed-rate mismatch");
	STAT_PRINT(is_ps_unassoc, "ps-poll for unassoc. sta");
	STAT_PRINT(is_ps_badaid, "ps-poll w/ incorrect aid");
	STAT_PRINT(is_ps_qempty, "ps-poll w/ nothing to send");
	STAT_PRINT(is_ff_badhdr, "fast frame rx'd w/ bad hdr");
	STAT_PRINT(is_ff_tooshort, "fast frame rx decap error");
	STAT_PRINT(is_ff_split, "fast frame rx split error");
	STAT_PRINT(is_ff_decap, "fast frames decap'd");
	STAT_PRINT(is_ff_encap, "fast frames encap'd for tx");
	STAT_PRINT(is_rx_badbintval, "rx frame w/ bogus bintval");
}

void
ieee80211_status(void)
{
	int i, nwkey_verbose;
	struct ieee80211_nwid nwid;
	struct ieee80211_nwkey nwkey;
	struct ieee80211_power power;
	u_int8_t keybuf[IEEE80211_WEP_NKID][16];
	struct ieee80211_bssid bssid;
	struct ieee80211chanreq channel;
	struct ieee80211req ireq;
	struct ether_addr ea;
	static const u_int8_t zero_macaddr[IEEE80211_ADDR_LEN];
	enum ieee80211_opmode opmode = get80211opmode();

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_data = (void *)&nwid;
	estrlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCG80211NWID, &ifr) == -1)
		return;
	if (nwid.i_len > IEEE80211_NWID_LEN) {
		errx(EXIT_FAILURE, "SIOCG80211NWID: wrong length of nwid (%d)", nwid.i_len);
	}
	printf("\tssid ");
	print_string(nwid.i_nwid, nwid.i_len);

	if (opmode == IEEE80211_M_HOSTAP) {
		estrlcpy(ireq.i_name, name, sizeof(ireq.i_name));
		ireq.i_type = IEEE80211_IOC_HIDESSID;
		if (ioctl(s, SIOCG80211, &ireq) != -1) {
                        if (ireq.i_val)
                                printf(" [hidden]");
                        else if (vflag)
                                printf(" [shown]");
                }

		ireq.i_type = IEEE80211_IOC_APBRIDGE;
		if (ioctl(s, SIOCG80211, &ireq) != -1) {
			if (ireq.i_val)
				printf(" apbridge");
			else if (vflag)
				printf(" -apbridge");
		}
        }

	estrlcpy(ireq.i_name, name, sizeof(ireq.i_name));
	ireq.i_type = IEEE80211_IOC_FRAGTHRESHOLD;
	if (ioctl(s, SIOCG80211, &ireq) == -1)
		;
	else if (ireq.i_val < IEEE80211_FRAG_MAX)
		printf(" frag %d", ireq.i_val);
	else if (vflag)
		printf(" -frag");

	memset(&nwkey, 0, sizeof(nwkey));
	estrlcpy(nwkey.i_name, name, sizeof(nwkey.i_name));
	/* show nwkey only when WEP is enabled */
	if (ioctl(s, SIOCG80211NWKEY, &nwkey) == -1 ||
	    nwkey.i_wepon == 0) {
		printf("\n");
		goto skip_wep;
	}

	printf(" nwkey ");
	/* try to retrieve WEP keys */
	for (i = 0; i < IEEE80211_WEP_NKID; i++) {
		nwkey.i_key[i].i_keydat = keybuf[i];
		nwkey.i_key[i].i_keylen = sizeof(keybuf[i]);
	}
	if (ioctl(s, SIOCG80211NWKEY, &nwkey) == -1) {
		printf("*****");
	} else {
		nwkey_verbose = 0;
		/* check to see non default key or multiple keys defined */
		if (nwkey.i_defkid != 1) {
			nwkey_verbose = 1;
		} else {
			for (i = 1; i < IEEE80211_WEP_NKID; i++) {
				if (nwkey.i_key[i].i_keylen != 0) {
					nwkey_verbose = 1;
					break;
				}
			}
		}
		/* check extra ambiguity with keywords */
		if (!nwkey_verbose) {
			if (nwkey.i_key[0].i_keylen >= 2 &&
			    isdigit(nwkey.i_key[0].i_keydat[0]) &&
			    nwkey.i_key[0].i_keydat[1] == ':')
				nwkey_verbose = 1;
			else if (nwkey.i_key[0].i_keylen >= 7 &&
			    strncasecmp("persist",
			    (const char *)nwkey.i_key[0].i_keydat, 7) == 0)
				nwkey_verbose = 1;
		}
		if (nwkey_verbose)
			printf("%d:", nwkey.i_defkid);
		for (i = 0; i < IEEE80211_WEP_NKID; i++) {
			if (i > 0)
				printf(",");
			if (nwkey.i_key[i].i_keylen < 0)
				printf("persist");
			else
				print_string(nwkey.i_key[i].i_keydat,
				    nwkey.i_key[i].i_keylen);
			if (!nwkey_verbose)
				break;
		}
	}
	printf("\n");

 skip_wep:
	estrlcpy(power.i_name, name, sizeof(power.i_name));
	if (ioctl(s, SIOCG80211POWER, &power) == -1)
		goto skip_power;
	printf("\tpowersave ");
	if (power.i_enabled)
		printf("on (%dms sleep)", power.i_maxsleep);
	else
		printf("off");
	printf("\n");

 skip_power:
	estrlcpy(bssid.i_name, name, sizeof(bssid.i_name));
	if (ioctl(s, SIOCG80211BSSID, &bssid) == -1)
		return;
	estrlcpy(channel.i_name, name, sizeof(channel.i_name));
	if (ioctl(s, SIOCG80211CHANNEL, &channel) == -1)
		return;
	if (memcmp(bssid.i_bssid, zero_macaddr, IEEE80211_ADDR_LEN) == 0) {
		if (channel.i_channel != (u_int16_t)-1)
			printf("\tchan %d\n", channel.i_channel);
	} else {
		memcpy(ea.ether_addr_octet, bssid.i_bssid,
		    sizeof(ea.ether_addr_octet));
		printf("\tbssid %s", ether_ntoa(&ea));
		if (channel.i_channel != IEEE80211_CHAN_ANY)
			printf(" chan %d", channel.i_channel);
		printf("\n");
	}
}

static void
scan_and_wait(void)
{
	struct ieee80211req ireq;
	int sroute;

	sroute = socket(PF_ROUTE, SOCK_RAW, 0);
	if (sroute < 0) {
		perror("socket(PF_ROUTE,SOCK_RAW)");
		return;
	}
	(void) memset(&ireq, 0, sizeof(ireq));
	estrlcpy(ireq.i_name, name, sizeof(ireq.i_name));
	ireq.i_type = IEEE80211_IOC_SCAN_REQ;
	/* NB: only root can trigger a scan so ignore errors */
	if (ioctl(s, SIOCS80211, &ireq) >= 0) {
		char buf[2048];
		struct if_announcemsghdr *ifan;
		struct rt_msghdr *rtm;

		do {
			if (read(sroute, buf, sizeof(buf)) < 0) {
				perror("read(PF_ROUTE)");
				break;
			}
			rtm = (struct rt_msghdr *) buf;
			if (rtm->rtm_version != RTM_VERSION)
				break;
			ifan = (struct if_announcemsghdr *) rtm;
		} while (rtm->rtm_type != RTM_IEEE80211 ||
		    ifan->ifan_what != RTM_IEEE80211_SCAN);
	}
	close(sroute);
}

static void
list_scan(void)
{
	u_int8_t buf[24*1024];
	struct ieee80211req ireq;
	char ssid[IEEE80211_NWID_LEN+1];
	const u_int8_t *cp;
	int len, ssidmax;

	(void) memset(&ireq, 0, sizeof(ireq));
	estrlcpy(ireq.i_name, name, sizeof(ireq.i_name));
	ireq.i_type = IEEE80211_IOC_SCAN_RESULTS;
	ireq.i_data = buf;
	ireq.i_len = sizeof(buf);
	if (ioctl(s, SIOCG80211, &ireq) < 0)
		errx(1, "unable to get scan results");
	len = ireq.i_len;
	if (len < sizeof(struct ieee80211req_scan_result))
		return;

	ssidmax = IEEE80211_NWID_LEN;
	printf("%-*.*s  %-17.17s  %4s %4s  %-7s %3s %4s\n"
		, ssidmax, ssidmax, "SSID"
		, "BSSID"
		, "CHAN"
		, "RATE"
		, "S:N"
		, "INT"
		, "CAPS"
	);
	cp = buf;
	do {
		const struct ieee80211req_scan_result *sr;
		const uint8_t *vp;

		sr = (const struct ieee80211req_scan_result *) cp;
		vp = (const u_int8_t *)(sr+1);
		printf("%-*.*s  %s  %3d  %3dM %3d:%-3d  %3d %-4.4s"
			, ssidmax
			  , copy_essid(ssid, ssidmax, vp, sr->isr_ssid_len)
			  , ssid
			, ether_ntoa((const struct ether_addr *) sr->isr_bssid)
			, ieee80211_mhz2ieee(sr->isr_freq, sr->isr_flags)
			, getmaxrate(sr->isr_rates, sr->isr_nrates)
			, sr->isr_rssi, sr->isr_noise
			, sr->isr_intval
			, getcaps(sr->isr_capinfo)
		);
		printies(vp + sr->isr_ssid_len, sr->isr_ie_len, 24);;
		printf("\n");
		cp += sr->isr_len, len -= sr->isr_len;
	} while (len >= sizeof(struct ieee80211req_scan_result));
}
/*
 * Convert MHz frequency to IEEE channel number.
 */
static u_int
ieee80211_mhz2ieee(u_int isrfreq, u_int isrflags)
{
	if ((isrflags & IEEE80211_CHAN_GSM) || (907 <= isrfreq && isrfreq <= 922))
		return mapgsm(isrfreq, isrflags);
	if (isrfreq == 2484)
		return 14;
	if (isrfreq < 2484)
		return (isrfreq - 2407) / 5;
	if (isrfreq < 5000) {
		if (isrflags & (IEEE80211_CHAN_HALF|IEEE80211_CHAN_QUARTER))
			return mappsb(isrfreq, isrflags);
		else if (isrfreq > 4900)
			return (isrfreq - 4000) / 5;
		else
			return 15 + ((isrfreq - 2512) / 20);
	}
	return (isrfreq - 5000) / 5;
}

static int
getmaxrate(const u_int8_t rates[15], u_int8_t nrates)
{
	int i, maxrate = -1;

	for (i = 0; i < nrates; i++) {
		int rate = rates[i] & IEEE80211_RATE_VAL;
		if (rate > maxrate)
			maxrate = rate;
	}
	return maxrate / 2;
}

static const char *
getcaps(int capinfo)
{
	static char capstring[32];
	char *cp = capstring;

	if (capinfo & IEEE80211_CAPINFO_ESS)
		*cp++ = 'E';
	if (capinfo & IEEE80211_CAPINFO_IBSS)
		*cp++ = 'I';
	if (capinfo & IEEE80211_CAPINFO_CF_POLLABLE)
		*cp++ = 'c';
	if (capinfo & IEEE80211_CAPINFO_CF_POLLREQ)
		*cp++ = 'C';
	if (capinfo & IEEE80211_CAPINFO_PRIVACY)
		*cp++ = 'P';
	if (capinfo & IEEE80211_CAPINFO_SHORT_PREAMBLE)
		*cp++ = 'S';
	if (capinfo & IEEE80211_CAPINFO_PBCC)
		*cp++ = 'B';
	if (capinfo & IEEE80211_CAPINFO_CHNL_AGILITY)
		*cp++ = 'A';
	if (capinfo & IEEE80211_CAPINFO_SHORT_SLOTTIME)
		*cp++ = 's';
	if (capinfo & IEEE80211_CAPINFO_RSN)
		*cp++ = 'R';
	if (capinfo & IEEE80211_CAPINFO_DSSSOFDM)
		*cp++ = 'D';
	*cp = '\0';
	return capstring;
}

static void
printie(const char* tag, const uint8_t *ie, size_t ielen, int maxlen)
{
	printf("%s", tag);

	maxlen -= strlen(tag)+2;
	if (2*ielen > maxlen)
		maxlen--;
	printf("<");
	for (; ielen > 0; ie++, ielen--) {
		if (maxlen-- <= 0)
			break;
		printf("%02x", *ie);
	}
	if (ielen != 0)
		printf("-");
	printf(">");
}

#define LE_READ_2(p)					\
	((u_int16_t)					\
	 ((((const u_int8_t *)(p))[0]      ) |		\
	  (((const u_int8_t *)(p))[1] <<  8)))
#define LE_READ_4(p)					\
	((u_int32_t)					\
	 ((((const u_int8_t *)(p))[0]      ) |		\
	  (((const u_int8_t *)(p))[1] <<  8) |		\
	  (((const u_int8_t *)(p))[2] << 16) |		\
	  (((const u_int8_t *)(p))[3] << 24)))

/*
 * NB: The decoding routines assume a properly formatted ie
 *     which should be safe as the kernel only retains them
 *     if they parse ok.
 */

static void
printwmeparam(const char *tag, const u_int8_t *ie, size_t ielen, int maxlen)
{
#define	MS(_v, _f)	(((_v) & _f) >> _f##_S)
	static const char *acnames[] = { "BE", "BK", "VO", "VI" };
	const struct ieee80211_wme_param *wme =
	    (const struct ieee80211_wme_param *) ie;
	int i;

	printf("%s", tag);
	if (!vflag)
		return;
	printf("<qosinfo 0x%x", wme->param_qosInfo);
	ie += offsetof(struct ieee80211_wme_param, params_acParams);
	for (i = 0; i < WME_NUM_AC; i++) {
		const struct ieee80211_wme_acparams *ac =
		    &wme->params_acParams[i];

		printf(" %s[%saifsn %u cwmin %u cwmax %u txop %u]"
			, acnames[i]
			, MS(ac->acp_aci_aifsn, WME_PARAM_ACM) ? "acm " : ""
			, MS(ac->acp_aci_aifsn, WME_PARAM_AIFSN)
			, MS(ac->acp_logcwminmax, WME_PARAM_LOGCWMIN)
			, MS(ac->acp_logcwminmax, WME_PARAM_LOGCWMAX)
			, LE_READ_2(&ac->acp_txop)
		);
	}
	printf(">");
#undef MS
}

static void
printwmeinfo(const char *tag, const u_int8_t *ie, size_t ielen, int maxlen)
{
	printf("%s", tag);
	if (vflag) {
		const struct ieee80211_wme_info *wme =
		    (const struct ieee80211_wme_info *) ie;
		printf("<version 0x%x info 0x%x>",
		    wme->wme_version, wme->wme_info);
	}
}

static const char *
wpa_cipher(const u_int8_t *sel)
{
#define	WPA_SEL(x)	(((x)<<24)|WPA_OUI)
	u_int32_t w = LE_READ_4(sel);

	switch (w) {
	case WPA_SEL(WPA_CSE_NULL):
		return "NONE";
	case WPA_SEL(WPA_CSE_WEP40):
		return "WEP40";
	case WPA_SEL(WPA_CSE_WEP104):
		return "WEP104";
	case WPA_SEL(WPA_CSE_TKIP):
		return "TKIP";
	case WPA_SEL(WPA_CSE_CCMP):
		return "AES-CCMP";
	}
	return "?";		/* NB: so 1<< is discarded */
#undef WPA_SEL
}

static const char *
wpa_keymgmt(const u_int8_t *sel)
{
#define	WPA_SEL(x)	(((x)<<24)|WPA_OUI)
	u_int32_t w = LE_READ_4(sel);

	switch (w) {
	case WPA_SEL(WPA_ASE_8021X_UNSPEC):
		return "8021X-UNSPEC";
	case WPA_SEL(WPA_ASE_8021X_PSK):
		return "8021X-PSK";
	case WPA_SEL(WPA_ASE_NONE):
		return "NONE";
	}
	return "?";
#undef WPA_SEL
}

static void
printwpaie(const char *tag, const u_int8_t *ie, size_t ielen, int maxlen)
{
	u_int8_t len = ie[1];

	printf("%s", tag);
	if (vflag) {
		const char *sep;
		int n;

		ie += 6, len -= 4;		/* NB: len is payload only */

		printf("<v%u", LE_READ_2(ie));
		ie += 2, len -= 2;

		printf(" mc:%s", wpa_cipher(ie));
		ie += 4, len -= 4;

		/* unicast ciphers */
		n = LE_READ_2(ie);
		ie += 2, len -= 2;
		sep = " uc:";
		for (; n > 0; n--) {
			printf("%s%s", sep, wpa_cipher(ie));
			ie += 4, len -= 4;
			sep = "+";
		}

		/* key management algorithms */
		n = LE_READ_2(ie);
		ie += 2, len -= 2;
		sep = " km:";
		for (; n > 0; n--) {
			printf("%s%s", sep, wpa_keymgmt(ie));
			ie += 4, len -= 4;
			sep = "+";
		}

		if (len > 2)		/* optional capabilities */
			printf(", caps 0x%x", LE_READ_2(ie));
		printf(">");
	}
}

static const char *
rsn_cipher(const u_int8_t *sel)
{
#define	RSN_SEL(x)	(((x)<<24)|RSN_OUI)
	u_int32_t w = LE_READ_4(sel);

	switch (w) {
	case RSN_SEL(RSN_CSE_NULL):
		return "NONE";
	case RSN_SEL(RSN_CSE_WEP40):
		return "WEP40";
	case RSN_SEL(RSN_CSE_WEP104):
		return "WEP104";
	case RSN_SEL(RSN_CSE_TKIP):
		return "TKIP";
	case RSN_SEL(RSN_CSE_CCMP):
		return "AES-CCMP";
	case RSN_SEL(RSN_CSE_WRAP):
		return "AES-OCB";
	}
	return "?";
#undef WPA_SEL
}

static const char *
rsn_keymgmt(const u_int8_t *sel)
{
#define	RSN_SEL(x)	(((x)<<24)|RSN_OUI)
	u_int32_t w = LE_READ_4(sel);

	switch (w) {
	case RSN_SEL(RSN_ASE_8021X_UNSPEC):
		return "8021X-UNSPEC";
	case RSN_SEL(RSN_ASE_8021X_PSK):
		return "8021X-PSK";
	case RSN_SEL(RSN_ASE_NONE):
		return "NONE";
	}
	return "?";
#undef RSN_SEL
}

static void
printrsnie(const char *tag, const u_int8_t *ie, size_t ielen, int maxlen)
{
	printf("%s", tag);
	if (vflag) {
		const char *sep;
		int n;

		ie += 2, ielen -= 2;

		printf("<v%u", LE_READ_2(ie));
		ie += 2, ielen -= 2;

		printf(" mc:%s", rsn_cipher(ie));
		ie += 4, ielen -= 4;

		/* unicast ciphers */
		n = LE_READ_2(ie);
		ie += 2, ielen -= 2;
		sep = " uc:";
		for (; n > 0; n--) {
			printf("%s%s", sep, rsn_cipher(ie));
			ie += 4, ielen -= 4;
			sep = "+";
		}

		/* key management algorithms */
		n = LE_READ_2(ie);
		ie += 2, ielen -= 2;
		sep = " km:";
		for (; n > 0; n--) {
			printf("%s%s", sep, rsn_keymgmt(ie));
			ie += 4, ielen -= 4;
			sep = "+";
		}

		if (ielen > 2)		/* optional capabilities */
			printf(", caps 0x%x", LE_READ_2(ie));
		/* XXXPMKID */
		printf(">");
	}
}

/*
 * Copy the ssid string contents into buf, truncating to fit.  If the
 * ssid is entirely printable then just copy intact.  Otherwise convert
 * to hexadecimal.  If the result is truncated then replace the last
 * three characters with "...".
 */
static int
copy_essid(char buf[], size_t bufsize, const u_int8_t *essid, size_t essid_len)
{
	const u_int8_t *p; 
	size_t maxlen;
	int i;

	if (essid_len > bufsize)
		maxlen = bufsize;
	else
		maxlen = essid_len;
	/* determine printable or not */
	for (i = 0, p = essid; i < maxlen; i++, p++) {
		if (*p < ' ' || *p > 0x7e)
			break;
	}
	if (i != maxlen) {		/* not printable, print as hex */
		if (bufsize < 3)
			return 0;
		strlcpy(buf, "0x", bufsize);
		bufsize -= 2;
		p = essid;
		for (i = 0; i < maxlen && bufsize >= 2; i++) {
			sprintf(&buf[2+2*i], "%02x", p[i]);
			bufsize -= 2;
		}
		if (i != essid_len)
			memcpy(&buf[2+2*i-3], "...", 3);
	} else {			/* printable, truncate as needed */
		memcpy(buf, essid, maxlen);
		if (maxlen != essid_len)
			memcpy(&buf[maxlen-3], "...", 3);
	}
	return maxlen;
}

static void
printssid(const char *tag, const u_int8_t *ie, size_t ielen, int maxlen)
{
	char ssid[2*IEEE80211_NWID_LEN+1];

	printf("%s<%.*s>", tag, copy_essid(ssid, maxlen, ie+2, ie[1]), ssid);
}

static void
printrates(const char *tag, const u_int8_t *ie, size_t ielen, int maxlen)
{
	const char *sep;
	int i;

	printf("%s", tag);
	sep = "<";
	for (i = 2; i < ielen; i++) {
		printf("%s%s%d", sep,
		    ie[i] & IEEE80211_RATE_BASIC ? "B" : "",
		    ie[i] & IEEE80211_RATE_VAL);
		sep = ",";
	}
	printf(">");
}

static void
printcountry(const char *tag, const u_int8_t *ie, size_t ielen, int maxlen)
{
	const struct ieee80211_country_ie *cie =
	   (const struct ieee80211_country_ie *) ie;
	int i, nbands, schan, nchan;

	printf("%s<%c%c%c", tag, cie->cc[0], cie->cc[1], cie->cc[2]);
	nbands = (cie->len - 3) / sizeof(cie->band[0]);
	for (i = 0; i < nbands; i++) {
		schan = cie->band[i].schan;
		nchan = cie->band[i].nchan;
		if (nchan != 1)
			printf(" %u-%u,%u", schan, schan + nchan-1,
			    cie->band[i].maxtxpwr);
		else
			printf(" %u,%u", schan, cie->band[i].maxtxpwr);
	}
	printf(">");
}

/* unaligned little endian access */     
#define LE_READ_4(p)					\
	((u_int32_t)					\
	 ((((const u_int8_t *)(p))[0]      ) |		\
	  (((const u_int8_t *)(p))[1] <<  8) |		\
	  (((const u_int8_t *)(p))[2] << 16) |		\
	  (((const u_int8_t *)(p))[3] << 24)))

static int 
iswpaoui(const u_int8_t *frm)
{
	return frm[1] > 3 && LE_READ_4(frm+2) == ((WPA_OUI_TYPE<<24)|WPA_OUI);
}

static int 
iswmeinfo(const u_int8_t *frm)
{
	return frm[1] > 5 && LE_READ_4(frm+2) == ((WME_OUI_TYPE<<24)|WME_OUI) &&
		frm[6] == WME_INFO_OUI_SUBTYPE;
}

static int
iswmeparam(const u_int8_t *frm)
{
	return frm[1] > 5 && LE_READ_4(frm+2) == ((WME_OUI_TYPE<<24)|WME_OUI) &&
		frm[6] == WME_PARAM_OUI_SUBTYPE;
}

static const char *
iename(int elemid)
{
	switch (elemid) {
	case IEEE80211_ELEMID_FHPARMS:	return " FHPARMS";
	case IEEE80211_ELEMID_CFPARMS:	return " CFPARMS";
	case IEEE80211_ELEMID_TIM:	return " TIM";
	case IEEE80211_ELEMID_IBSSPARMS:return " IBSSPARMS";
	case IEEE80211_ELEMID_CHALLENGE:return " CHALLENGE";
	case IEEE80211_ELEMID_PWRCNSTR:	return " PWRCNSTR";
	case IEEE80211_ELEMID_PWRCAP:	return " PWRCAP";
	case IEEE80211_ELEMID_TPCREQ:	return " TPCREQ";
	case IEEE80211_ELEMID_TPCREP:	return " TPCREP";
	case IEEE80211_ELEMID_SUPPCHAN:	return " SUPPCHAN";
	case IEEE80211_ELEMID_CHANSWITCHANN:return " CSA";
	case IEEE80211_ELEMID_MEASREQ:	return " MEASREQ";
	case IEEE80211_ELEMID_MEASREP:	return " MEASREP";
	case IEEE80211_ELEMID_QUIET:	return " QUIET";
	case IEEE80211_ELEMID_IBSSDFS:	return " IBSSDFS";
	case IEEE80211_ELEMID_TPC:	return " TPC";
	case IEEE80211_ELEMID_CCKM:	return " CCKM";
	}
	return " ???";
}

static void
printies(const u_int8_t *vp, int ielen, int maxcols)
{
	while (ielen > 0) {
		switch (vp[0]) {
		case IEEE80211_ELEMID_SSID:
			if (vflag)
				printssid(" SSID", vp, 2+vp[1], maxcols);
			break;
		case IEEE80211_ELEMID_RATES:
		case IEEE80211_ELEMID_XRATES:
			if (vflag)
				printrates(vp[0] == IEEE80211_ELEMID_RATES ?
				    " RATES" : " XRATES", vp, 2+vp[1], maxcols);
			break;
		case IEEE80211_ELEMID_DSPARMS:
			if (vflag)
				printf(" DSPARMS<%u>", vp[2]);
			break;
		case IEEE80211_ELEMID_COUNTRY:
			if (vflag)
				printcountry(" COUNTRY", vp, 2+vp[1], maxcols);
			break;
		case IEEE80211_ELEMID_ERP:
			if (vflag)
				printf(" ERP<0x%x>", vp[2]);
			break;
		case IEEE80211_ELEMID_VENDOR:
			if (iswpaoui(vp))
				printwpaie(" WPA", vp, 2+vp[1], maxcols);
			else if (iswmeinfo(vp))
				printwmeinfo(" WME", vp, 2+vp[1], maxcols);
			else if (iswmeparam(vp))
				printwmeparam(" WME", vp, 2+vp[1], maxcols);
			else if (vflag)
				printie(" VEN", vp, 2+vp[1], maxcols);
			break;
		case IEEE80211_ELEMID_RSN:
			printrsnie(" RSN", vp, 2+vp[1], maxcols);
			break;
		default:
			if (vflag)
				printie(iename(vp[0]), vp, 2+vp[1], maxcols);
			break;
		}
		ielen -= 2+vp[1];
		vp += 2+vp[1];
	}
}

static int
mapgsm(u_int isrfreq, u_int isrflags)
{
	isrfreq *= 10;
	if (isrflags & IEEE80211_CHAN_QUARTER)
		isrfreq += 5;
	else if (isrflags & IEEE80211_CHAN_HALF)
		isrfreq += 10;
	else
		isrfreq += 20;
	/* NB: there is no 907/20 wide but leave room */
	return (isrfreq - 906*10) / 5;
}

static int
mappsb(u_int isrfreq, u_int isrflags)
{
	return 37 + ((isrfreq * 10) + ((isrfreq % 5) == 2 ? 5 : 0) - 49400) / 5;
}

/*	$NetBSD: wiconfig.c,v 1.25 2002/04/09 02:56:17 thorpej Exp $	*/
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
 *	From: Id: wicontrol.c,v 1.6 1999/05/22 16:12:49 wpaul Exp $
 */

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <net/if.h>
#ifdef __FreeBSD__
#include <net/if_var.h>
#include <net/ethernet.h>

#include <machine/if_wavelan_ieee.h>
#else
#include <netinet/in.h>
#include <netinet/if_ether.h>
#ifdef __NetBSD__
#include <net/if_ieee80211.h>
#include <dev/ic/wi_ieee.h>
#else
#include <dev/pcmcia/if_wavelan_ieee.h>
#endif
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>

#if !defined(lint)
__COPYRIGHT(
"@(#) Copyright (c) 1997, 1998, 1999\
	Bill Paul. All rights reserved.");
__RCSID("$NetBSD: wiconfig.c,v 1.25 2002/04/09 02:56:17 thorpej Exp $");
#endif

struct wi_table {
	int wi_type;
	int wi_code;
#define	WI_NONE			0x00
#define	WI_STRING		0x01
#define	WI_BOOL			0x02
#define	WI_WORDS		0x03
#define	WI_HEXBYTES		0x04
#define	WI_KEYSTRUCT		0x05
#define	WI_BITS			0x06
	char *wi_label;			/* label used to print info */
	int wi_opt;			/* option character to set this */
	char *wi_desc;
	char *wi_optval;
};

/* already define in wireg.h XXX */
#define	WI_APRATE_0		0x00	/* NONE */
#define WI_APRATE_1		0x0A	/* 1 Mbps */
#define WI_APRATE_2		0x14	/* 2 Mbps */
#define WI_APRATE_5		0x37	/* 5.5 Mbps */
#define WI_APRATE_11		0x6E	/* 11 Mbps */

#ifdef WI_RID_SCAN_APS
static void wi_apscan		__P((char *));
static int  get_if_flags	__P((int, const char *));
static int  set_if_flags	__P((int, const char *, int));
#endif
static void wi_getval		__P((char *, struct wi_req *));
static void wi_setval		__P((char *, struct wi_req *));
static void wi_printstr		__P((struct wi_req *));
static void wi_setstr		__P((char *, int, char *));
static void wi_setbytes		__P((char *, int, char *, int));
static void wi_setword		__P((char *, int, int));
static void wi_sethex		__P((char *, int, char *));
static void wi_printwords	__P((struct wi_req *));
static void wi_printbool	__P((struct wi_req *));
static void wi_printhex		__P((struct wi_req *));
static void wi_printbits	__P((struct wi_req *));
static void wi_dumpinfo		__P((char *));
static void wi_setkeys		__P((char *, char *, int));
static void wi_printkeys	__P((struct wi_req *));
static void wi_dumpstats	__P((char *));
static void usage		__P((void));
static struct wi_table *
	wi_optlookup __P((struct wi_table *, int));
static int  wi_hex2int(char c);
static void wi_str2key		__P((char *, struct wi_key *));
int main __P((int argc, char **argv));

#ifdef WI_RID_SCAN_APS
static int get_if_flags(s, name)
	int		s;
	const char	*name;
{
	struct ifreq	ifreq;
	int		flags;

	strncpy(ifreq.ifr_name, name, sizeof(ifreq.ifr_name));
	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifreq) == -1)
		  err(1, "SIOCGIFFLAGS");
	flags = ifreq.ifr_flags;

	return flags;
}

static int set_if_flags(s, name, flags)
	int		s;
	const char	*name;
	int		flags;
{
	struct ifreq ifreq;

	ifreq.ifr_flags = flags;
	strncpy(ifreq.ifr_name, name, sizeof(ifreq.ifr_name));
	if (ioctl(s, SIOCSIFFLAGS, (caddr_t)&ifreq) == -1)
		  err(1, "SIOCSIFFLAGS");

	return 0;
}

static void wi_apscan(iface)
	char			*iface;
{
	struct wi_req		wreq;
	struct ifreq		ifr;
	int			s;
	int			naps, rate;
	int			retries = 10;
	int			flags;
	struct wi_apinfo	*w;
	int			i, j;

	if (iface == NULL)
		errx(1, "must specify interface name");

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == -1)
		err(1, "socket");
	flags = get_if_flags(s, iface);
	if ((flags & IFF_UP) == 0)
		flags = set_if_flags(s, iface, flags | IFF_UP);

	memset((char *)&wreq, 0, sizeof(wreq));

	wreq.wi_type = WI_RID_SCAN_APS;
	wreq.wi_len = 4;
	/* note chan. 1 is the least significant bit */
	wreq.wi_val[0] = 0x3fff;	/* 1 bit per channel, 1-14 */
	wreq.wi_val[1] = 0xf;		/* tx rate */

	/* write the request */
	wi_setval(iface, &wreq);

	/* now poll for a result */
	memset((char *)&wreq, 0, sizeof(wreq));

	wreq.wi_type = WI_RID_READ_APS;
	wreq.wi_len = WI_MAX_DATALEN;

	/* we have to do this ourself as opposed to
	 * using getval, because we cannot bail if
 	 * the ioctl fails
	 */
	memset((char *)&ifr, 0, sizeof(ifr));
        strcpy(ifr.ifr_name, iface);
        ifr.ifr_data = (caddr_t)&wreq;

	printf("scanning ...");
	fflush(stdout);
	while (ioctl(s, SIOCGWAVELAN, &ifr) == -1) {
		retries--;
		if (retries >= 0) {
			printf("."); fflush(stdout);
			sleep(1);
		} else
			break;
		errno = 0;
	}

	if (errno) {
		set_if_flags(s, iface, flags);
		close(s);
		err(1, "ioctl");
	}

	naps = *(int *)wreq.wi_val;

	if (naps > 0)
		printf("\nAP Information\n");
	else
		printf("\nNo APs available\n");

	w =  (struct wi_apinfo *)(((char *)&wreq.wi_val) + sizeof(int));
	for ( i = 0; i < naps; i++, w++) {
		printf("ap[%d]:\n", i);
		if (w->scanreason) {
			static char *scanm[] = {
				"Host initiated",
				"Firmware initiated",
				"Inquiry request from host"
			};
			printf("\tScanReason:\t\t\t[ %s ]\n",
				scanm[w->scanreason - 1]);
		}
		printf("\tnetname (SSID):\t\t\t[ ");
			for (j = 0; j < w->namelen; j++) {
				printf("%c", w->name[j]);
			}
			printf(" ]\n");
		printf("\tBSSID:\t\t\t\t[ %02x:%02x:%02x:%02x:%02x:%02x ]\n",
			w->bssid[0]&0xff, w->bssid[1]&0xff,
			w->bssid[2]&0xff, w->bssid[3]&0xff,
			w->bssid[4]&0xff, w->bssid[5]&0xff);
		printf("\tChannel:\t\t\t[ %d ]\n", w->channel);
		printf("\tQuality/Signal/Noise [signal]:\t[ %d / %d / %d ]\n"
		       "\t                        [dBm]:\t[ %d / %d / %d ]\n", 
			w->quality, w->signal, w->noise,
			w->quality, w->signal - 149, w->noise - 149);
		printf("\tBSS Beacon Interval [msec]:\t[ %d ]\n", w->interval); 
		printf("\tCapinfo:\t\t\t[ "); 
			if (w->capinfo & IEEE80211_CAPINFO_ESS)
				printf("ESS ");
			if (w->capinfo & IEEE80211_CAPINFO_PRIVACY)
				printf("WEP ");
			printf("]\n");

		switch (w->rate) {
		case WI_APRATE_1:
			rate = 1;
			break;
		case WI_APRATE_2:
			rate = 2;
			break;
		case WI_APRATE_5:
			rate = 5.5;
			break;
		case WI_APRATE_11:
			rate = 11;
			break;
		case WI_APRATE_0:
		default:
			rate = 0;
			break;
		}
		if (rate) printf("\tDataRate [Mbps]:\t\t[ %d ]\n", rate);
	}

	set_if_flags(s, iface, flags);
	close(s);
}
#endif

static void wi_getval(iface, wreq)
	char			*iface;
	struct wi_req		*wreq;
{
	struct ifreq		ifr;
	int			s;

	bzero((char *)&ifr, sizeof(ifr));

	strcpy(ifr.ifr_name, iface);
	ifr.ifr_data = (caddr_t)wreq;

	s = socket(AF_INET, SOCK_DGRAM, 0);

	if (s == -1)
		err(1, "socket");

	if (ioctl(s, SIOCGWAVELAN, &ifr) == -1)
		err(1, "SIOCGWAVELAN");

	close(s);

	return;
}

static void wi_setval(iface, wreq)
	char			*iface;
	struct wi_req		*wreq;
{
	struct ifreq		ifr;
	int			s;

	bzero((char *)&ifr, sizeof(ifr));

	strcpy(ifr.ifr_name, iface);
	ifr.ifr_data = (caddr_t)wreq;

	s = socket(AF_INET, SOCK_DGRAM, 0);

	if (s == -1)
		err(1, "socket");

	if (ioctl(s, SIOCSWAVELAN, &ifr) == -1)
		err(1, "SIOCSWAVELAN");

	close(s);

	return;
}

void wi_printstr(wreq)
	struct wi_req		*wreq;
{
	char			*ptr;
	int			i;

	if (wreq->wi_type == WI_RID_SERIALNO) {
		ptr = (char *)&wreq->wi_val;
		for (i = 0; i < (wreq->wi_len - 1) * 2; i++) {
			if (ptr[i] == '\0')
				ptr[i] = ' ';
		}
	} else {
		int len = le16toh(wreq->wi_val[0]);

		ptr = (char *)&wreq->wi_val[1];
		for (i = 0; i < len; i++) {
			if (ptr[i] == '\0')
				ptr[i] = ' ';
		}
	}

	ptr[i] = '\0';
	printf("[ %s ]", ptr);

	return;
}

void wi_setstr(iface, code, str)
	char			*iface;
	int			code;
	char			*str;
{
	struct wi_req		wreq;

	bzero((char *)&wreq, sizeof(wreq));

	if (strlen(str) > 30)
		errx(1, "string too long");

	wreq.wi_type = code;
	wreq.wi_len = 18;
	wreq.wi_val[0] = htole16(strlen(str));
	bcopy(str, (char *)&wreq.wi_val[1], strlen(str));

	wi_setval(iface, &wreq);

	return;
}

void wi_setbytes(iface, code, bytes, len)
	char			*iface;
	int			code;
	char			*bytes;
	int			len;
{
	struct wi_req		wreq;

	bzero((char *)&wreq, sizeof(wreq));

	wreq.wi_type = code;
	wreq.wi_len = (len / 2) + 1;
	bcopy(bytes, (char *)&wreq.wi_val[0], len);

	wi_setval(iface, &wreq);

	return;
}

void wi_setword(iface, code, word)
	char			*iface;
	int			code;
	int			word;
{
	struct wi_req		wreq;

	bzero((char *)&wreq, sizeof(wreq));

	wreq.wi_type = code;
	wreq.wi_len = 2;
	wreq.wi_val[0] = htole16(word);

	wi_setval(iface, &wreq);

	return;
}

void wi_sethex(iface, code, str)
	char			*iface;
	int			code;
	char			*str;
{
	struct ether_addr	*addr;

	addr = ether_aton(str);
	if (addr == NULL)
		errx(1, "badly formatted address");

	wi_setbytes(iface, code, (char *)addr, ETHER_ADDR_LEN);

	return;
}

static int
wi_hex2int(char c)
{
        if (c >= '0' && c <= '9')
                return (c - '0');
	if (c >= 'A' && c <= 'F')
	        return (c - 'A' + 10);
	if (c >= 'a' && c <= 'f')
                return (c - 'a' + 10);

	return (0); 
}

static void wi_str2key(s, k)
        char                    *s;
        struct wi_key           *k;
{
        int                     n, i;
        char                    *p;

        /* Is this a hex string? */
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
                /* Yes, convert to int. */
                n = 0;
                p = (char *)&k->wi_keydat[0];
                for (i = 2; i < strlen(s); i+= 2) {
                        *p++ = (wi_hex2int(s[i]) << 4) + wi_hex2int(s[i + 1]);
                        n++;
                }
                k->wi_keylen = htole16(n);
        } else {
                /* No, just copy it in. */
                bcopy(s, k->wi_keydat, strlen(s));
                k->wi_keylen = htole16(strlen(s));
        }

        return;
}

static void wi_setkeys(iface, key, idx)
        char                    *iface;
        char                    *key;
        int                     idx;
{
        struct wi_req           wreq;
        struct wi_ltv_keys      *keys;
        struct wi_key           *k;

        bzero((char *)&wreq, sizeof(wreq));
        wreq.wi_len = WI_MAX_DATALEN;
        wreq.wi_type = WI_RID_WEP_AVAIL;

        wi_getval(iface, &wreq);
        if (le16toh(wreq.wi_val[0]) == 0)
                err(1, "no WEP option available on this card");

        bzero((char *)&wreq, sizeof(wreq));
        wreq.wi_len = WI_MAX_DATALEN;
        wreq.wi_type = WI_RID_DEFLT_CRYPT_KEYS;

        wi_getval(iface, &wreq);
        keys = (struct wi_ltv_keys *)&wreq;

        if (key[0] == '0' && (key[1] == 'x' || key[1] == 'X')) {
	        if (strlen(key) > 30)
		        err(1, "encryption key must be no "
			    "more than 28 hex digits long");
	} else {
	        if (strlen(key) > 14)
		        err(1, "encryption key must be no "
			    "more than 14 characters long");
	}

        if (idx > 3)
                err(1, "only 4 encryption keys available");

        k = &keys->wi_keys[idx];
        wi_str2key(key, k);

        wreq.wi_len = (sizeof(struct wi_ltv_keys) / 2) + 1;
        wreq.wi_type = WI_RID_DEFLT_CRYPT_KEYS;
        wi_setval(iface, &wreq);

        return;
}

static void wi_printkeys(wreq)
        struct wi_req           *wreq;
{
        int                     i, j, bn;
        struct wi_key           *k;
        struct wi_ltv_keys      *keys;
        char                    *ptr;

	keys = (struct wi_ltv_keys *)wreq;

	for (i = 0, bn = 0; i < 4; i++, bn = 0) {
                k = &keys->wi_keys[i];
                ptr = (char *)k->wi_keydat;
                for (j = 0; j < le16toh(k->wi_keylen); j++) {
		        if (!isprint((unsigned char) ptr[j])) {
			        bn = 1;
				break;
			}
		}

		if (bn)	{
		        printf("[ 0x");
		        for (j = 0; j < le16toh(k->wi_keylen); j++)
			      printf("%02x", ((unsigned char *) ptr)[j]);
			printf(" ]");
		} else {
		        ptr[j] = '\0';
			printf("[ %s ]", ptr);
		}
        }

        return;
};

void wi_printwords(wreq)
	struct wi_req		*wreq;
{
	int			i;

	printf("[ ");
	for (i = 0; i < wreq->wi_len - 1; i++)
		printf("%d ", le16toh(wreq->wi_val[i]));
	printf("]");

	return;
}

void wi_printbool(wreq)
	struct wi_req		*wreq;
{
	if (le16toh(wreq->wi_val[0]))
		printf("[ On ]");
	else
		printf("[ Off ]");

	return;
}

void wi_printhex(wreq)
	struct wi_req		*wreq;
{
	int			i;
	unsigned char		*c;

	c = (unsigned char *)&wreq->wi_val;

	printf("[ ");
	for (i = 0; i < (wreq->wi_len - 1) * 2; i++) {
		printf("%02x", c[i]);
		if (i < ((wreq->wi_len - 1) * 2) - 1)
			printf(":");
	}

	printf(" ]");
	return;
}

void wi_printbits(wreq)
	struct wi_req		*wreq;
{
	int			i;
	int bits = le16toh(wreq->wi_val[0]);

	printf("[");
	for (i = 0; i < 16; i++) {
		if (bits & 0x1) {
			printf(" %d", i+1);
		}
		bits >>= 1;
	}
	printf(" ]");
	return;
}

static struct wi_table wi_table[] = {
	{ WI_RID_SERIALNO, WI_STRING, "NIC serial number:\t\t\t" },
	{ WI_RID_NODENAME, WI_STRING, "Station name:\t\t\t\t",
	    's', "station name" },
	{ WI_RID_OWN_SSID, WI_STRING, "SSID for IBSS creation:\t\t\t",
	    'q', "own SSID" },
	{ WI_RID_CURRENT_SSID, WI_STRING, "Current netname (SSID):\t\t\t" },
	{ WI_RID_DESIRED_SSID, WI_STRING, "Desired netname (SSID):\t\t\t",
	    'n', "network name" },
	{ WI_RID_CURRENT_BSSID, WI_HEXBYTES, "Current BSSID:\t\t\t\t" },
	{ WI_RID_CHANNEL_LIST, WI_BITS, "Channel list:\t\t\t\t" },
	{ WI_RID_OWN_CHNL, WI_WORDS, "IBSS channel:\t\t\t\t",
	    'f', "frequency" },
	{ WI_RID_CURRENT_CHAN, WI_WORDS, "Current channel:\t\t\t" },
	{ WI_RID_COMMS_QUALITY, WI_WORDS, "Comms quality/signal/noise:\t\t" },
	{ WI_RID_PROMISC, WI_BOOL, "Promiscuous mode:\t\t\t" },
	{ WI_RID_PORTTYPE, WI_WORDS, "Port type (1=BSS, 3=ad-hoc):\t\t",
	    'p', "port type" },
	{ WI_RID_MAC_NODE, WI_HEXBYTES, "MAC address:\t\t\t\t",
	    'm', "MAC address" },
	{ WI_RID_TX_RATE, WI_WORDS, "TX rate (selection):\t\t\t",
	    't', "TX rate" },
	{ WI_RID_CUR_TX_RATE, WI_WORDS, "TX rate (actual speed):\t\t\t"},
	{ WI_RID_OWN_BEACON_INT, WI_WORDS, "Beacon Interval (current) [msec]:\t"},
	{ WI_RID_MAX_DATALEN, WI_WORDS, "Maximum data length:\t\t\t",
	    'd', "maximum data length" },
	{ WI_RID_RTS_THRESH, WI_WORDS, "RTS/CTS handshake threshold:\t\t",
	    'r', "RTS threshold" },
	{ WI_RID_CREATE_IBSS, WI_BOOL, "Create IBSS:\t\t\t\t",
	    'c', "create ibss" },
	{ WI_RID_MICROWAVE_OVEN, WI_WORDS, "Microwave oven robustness:\t\t",
	    'M', "microwave oven robustness enabled" },
	{ WI_RID_ROAMING_MODE, WI_WORDS, "Roaming mode(1:firm,3:disable):\t\t",
	    'R', "roaming mode" },
	{ WI_RID_SYSTEM_SCALE, WI_WORDS, "Access point density:\t\t\t",
	    'a', "system scale" },
	{ WI_RID_PM_ENABLED, WI_WORDS, "Power Mgmt (1=on, 0=off):\t\t",
	    'P', "power management enabled" },
	{ WI_RID_MAX_SLEEP, WI_WORDS, "Max sleep time (msec):\t\t\t",
	    'S', "max sleep duration" },
	{ 0, WI_NONE }
};

static struct wi_table wi_crypt_table[] = {
	{ WI_RID_ENCRYPTION, WI_BOOL, "WEP encryption:\t\t\t\t",
	    'e', "encryption" },
	{ WI_RID_AUTH_CNTL, WI_WORDS, "Authentication type \n(1=OpenSys, 2=Shared Key):\t\t",
	    'A', "authentication type" },
        { WI_RID_TX_CRYPT_KEY, WI_WORDS, "TX encryption key:\t\t\t" },
        { WI_RID_DEFLT_CRYPT_KEYS, WI_KEYSTRUCT, "Encryption keys:\t\t\t" },
	{ 0, WI_NONE }
};

static struct wi_table *wi_tables[] = {
	wi_table,
	wi_crypt_table,
	NULL
};

static struct wi_table *
wi_optlookup(table, opt)
	struct wi_table *table;
	int opt;
{
	struct wi_table *wt;

	for (wt = table; wt->wi_type != 0; wt++)
		if (wt->wi_opt == opt)
			return (wt);
	return (NULL);
}

static void wi_dumpinfo(iface)
	char			*iface;
{
	struct wi_req		wreq;
	int			i, has_wep;
	struct wi_table		*w;

	bzero((char *)&wreq, sizeof(wreq));

	wreq.wi_len = WI_MAX_DATALEN;
	wreq.wi_type = WI_RID_WEP_AVAIL;

	wi_getval(iface, &wreq);
	has_wep = le16toh(wreq.wi_val[0]);

	w = wi_table;

	for (i = 0; w[i].wi_code != WI_NONE; i++) {
		bzero((char *)&wreq, sizeof(wreq));

		wreq.wi_len = WI_MAX_DATALEN;
		wreq.wi_type = w[i].wi_type;

		wi_getval(iface, &wreq);
		printf("%s", w[i].wi_label);
		switch (w[i].wi_code) {
		case WI_STRING:
			wi_printstr(&wreq);
			break;
		case WI_WORDS:
			wi_printwords(&wreq);
			break;
		case WI_BOOL:
			wi_printbool(&wreq);
			break;
		case WI_HEXBYTES:
			wi_printhex(&wreq);
			break;
		case WI_BITS:
			wi_printbits(&wreq);
			break;
		default:
			break;
		}	
		printf("\n");
	}

	if (has_wep) {
		w = wi_crypt_table;
		for (i = 0; w[i].wi_code != WI_NONE; i++) {
			bzero((char *)&wreq, sizeof(wreq));

			wreq.wi_len = WI_MAX_DATALEN;
			wreq.wi_type = w[i].wi_type;

			wi_getval(iface, &wreq);
			printf("%s", w[i].wi_label);
			switch (w[i].wi_code) {
			case WI_STRING:
				wi_printstr(&wreq);
				break;
			case WI_WORDS:
				if (wreq.wi_type == WI_RID_TX_CRYPT_KEY)
					wreq.wi_val[0] =
					  htole16(le16toh(wreq.wi_val[0]) + 1);
				wi_printwords(&wreq);
				break;
			case WI_BOOL:
				wi_printbool(&wreq);
				break;
			case WI_HEXBYTES:
				wi_printhex(&wreq);
				break;
			case WI_KEYSTRUCT:
				wi_printkeys(&wreq);
				break;
			default:
				break;
			}	
			printf("\n");
		}
	}

	return;
}

static void wi_dumpstats(iface)
	char			*iface;
{
	struct wi_req		wreq;
	struct wi_counters	*c;

	bzero((char *)&wreq, sizeof(wreq));
	wreq.wi_len = WI_MAX_DATALEN;
	wreq.wi_type = WI_RID_IFACE_STATS;

	wi_getval(iface, &wreq);

	c = (struct wi_counters *)&wreq.wi_val;

	/* XXX native byte order */
	printf("Transmitted unicast frames:\t\t%d\n",
	    c->wi_tx_unicast_frames);
	printf("Transmitted multicast frames:\t\t%d\n",
	    c->wi_tx_multicast_frames);
	printf("Transmitted fragments:\t\t\t%d\n",
	    c->wi_tx_fragments);
	printf("Transmitted unicast octets:\t\t%d\n",
	    c->wi_tx_unicast_octets);
	printf("Transmitted multicast octets:\t\t%d\n",
	    c->wi_tx_multicast_octets);
	printf("Single transmit retries:\t\t%d\n",
	    c->wi_tx_single_retries);
	printf("Multiple transmit retries:\t\t%d\n",
	    c->wi_tx_multi_retries);
	printf("Transmit retry limit exceeded:\t\t%d\n",
	    c->wi_tx_retry_limit);
	printf("Transmit discards:\t\t\t%d\n",
	    c->wi_tx_discards);
	printf("Transmit discards due to wrong SA:\t%d\n",
	    c->wi_tx_discards_wrong_sa);
	printf("Received unicast frames:\t\t%d\n",
	    c->wi_rx_unicast_frames);
	printf("Received multicast frames:\t\t%d\n",
	    c->wi_rx_multicast_frames);
	printf("Received fragments:\t\t\t%d\n",
	    c->wi_rx_fragments);
	printf("Received unicast octets:\t\t%d\n",
	    c->wi_rx_unicast_octets);
	printf("Received multicast octets:\t\t%d\n",
	    c->wi_rx_multicast_octets);
	printf("Receive FCS errors:\t\t\t%d\n",
	    c->wi_rx_fcs_errors);
	printf("Receive discards due to no buffer:\t%d\n",
	    c->wi_rx_discards_nobuf);
	printf("Can't decrypt WEP frame:\t\t%d\n",
	    c->wi_rx_WEP_cant_decrypt);
	printf("Received message fragments:\t\t%d\n",
	    c->wi_rx_msg_in_msg_frags);
	printf("Received message bad fragments:\t\t%d\n",
	    c->wi_rx_msg_in_bad_msg_frags);

	return;
}

static void
usage()
{

	fprintf(stderr,
	    "usage: %s interface "
	    "[-oD] [-t tx rate] [-n network name] [-s station name]\n"
	    "       [-e 0|1] [-k key [-v 1|2|3|4]] [-T 1|2|3|4]\n"
	    "       [-c 0|1] [-q SSID] [-p port type] [-a access point density]\n"
	    "       [-m MAC address] [-d max data length] [-r RTS threshold]\n"
	    "       [-f frequency] [-M 0|1] [-P 0|1] [-S max sleep duration]\n"
	    "       [-A 0|1 ] [-R 1|3]\n"
	    ,
	    getprogname());
	exit(1);
}

int main(argc, argv)
	int			argc;
	char			*argv[];
{
	struct wi_table *wt, **table;
	char *iface, *key, *keyv[4], *tx_crypt_key;
	int ch, dumpinfo, dumpstats, modifier, oldind, apscan;

#define	SET_OPERAND(opr, desc) do {				\
	if ((opr) == NULL)					\
		(opr) = optarg;					\
	else							\
		warnx("%s is already specified to %s",		\
		    desc, (opr));				\
} while (0)

	dumpinfo = 1;
	dumpstats = 0;
	apscan = 0;
	iface = key = keyv[0] = keyv[1] = keyv[2] = keyv[3] =
	    tx_crypt_key = NULL;

	if (argc > 1 && argv[1][0] != '-') {
		iface = argv[1];
		optind++;
	}

	while ((ch = getopt(argc, argv,
	    "a:c:d:e:f:hi:k:m:n:op:q:r:s:t:A:M:S:P:R:T:DZ:")) != -1) {
		if (ch != 'i')
			dumpinfo = 0;
		/*
		 * Lookup generic options and remeber operand if found.
		 */
		for (table = wi_tables; *table != NULL; table++)
			if ((wt = wi_optlookup(*table, ch)) != NULL) {
				SET_OPERAND(wt->wi_optval, wt->wi_desc);
				break;
			}
		if (wt == NULL)
			/*
			 * Handle special options.
			 */
			switch (ch) {
			case 'o':
				dumpstats = 1;
				break;
			case 'i':
				SET_OPERAND(iface, "interface");
				break;
			case 'k':
				key = optarg;
				oldind = optind;
				opterr = 0;
				ch = getopt(argc, argv, "v:");
				opterr = 1;
				switch (ch) {
				case 'v':
					modifier = atoi(optarg) - 1;
					break;
				default:
					modifier = 0;
					optind = oldind;
					break;
				}
				keyv[modifier] = key;
				break;
			case 'T':
				SET_OPERAND(tx_crypt_key, "TX encryption key");
				break;
			case 'D':
				apscan = 1;
				break;
			case 'h':
			default:
				usage();
				break;
			}
	}

	if (iface == NULL)
		usage();

	for (table = wi_tables; *table != NULL; table++)
		for (wt = *table; wt->wi_code != WI_NONE; wt++)
			if (wt->wi_optval != NULL) {
				switch (wt->wi_code) {
				case WI_BOOL:
				case WI_WORDS:
					wi_setword(iface, wt->wi_type,
					    atoi(wt->wi_optval));
					break;
				case WI_STRING:
					wi_setstr(iface, wt->wi_type,
					    wt->wi_optval);
					break;
				case WI_HEXBYTES:
					wi_sethex(iface, wt->wi_type,
					    wt->wi_optval);
					break;
				}
			}

	if (tx_crypt_key != NULL)
		wi_setword(iface, WI_RID_TX_CRYPT_KEY, atoi(tx_crypt_key) - 1);

	for (modifier = 0; modifier < sizeof(keyv) / sizeof(keyv[0]);
	    modifier++)
		if (keyv[modifier] != NULL)
			wi_setkeys(iface, keyv[modifier], modifier);

	if (dumpstats)
		wi_dumpstats(iface);
	if (dumpinfo)
		wi_dumpinfo(iface);

	if (apscan)
#ifdef WI_RID_SCAN_APS
		wi_apscan(iface);
#else
		errx(1, "AP scan mode is not available.");
#endif

	exit(0);
}

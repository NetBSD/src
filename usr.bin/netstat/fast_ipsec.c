/*	$NetBSD: */
/* 	$FreeBSD: src/tools/tools/crypto/ipsecstats.c,v 1.1.4.1 2003/06/03 00:13:13 sam Exp $ */

/*-
 * Copyright (c) 2003, 2004 Jonathan Stone
 * Copyright (c) 2002, 2003 Sam Leffler, Errno Consulting
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/tools/tools/crypto/ipsecstats.c,v 1.1.4.1 2003/06/03 00:13:13 sam Exp $
 */

#include <sys/cdefs.h>
#ifndef lint
#ifdef __NetBSD__
__RCSID("$NetBSD: fast_ipsec.c,v 1.1 2004/05/07 00:55:15 jonathan Exp $");
#endif
#endif /* not lint*/

/* Kernel headers required, but not included, by netstat.h */
#include <sys/types.h>
#include <sys/socket.h>

/* Kernel headers for sysctl(3). */
#include <sys/param.h>
#include <sys/sysctl.h>

/* Kernel headers for FAST_IPSEC statistics */
#include <net/pfkeyv2.h>
#include <netipsec/esp_var.h>
#include <netipsec/ah_var.h>
#include <netipsec/ipip_var.h>
#include <netipsec/ipcomp_var.h>
#include <netipsec/ipsec_var.h>
#include <netipsec/keydb.h>

#include <err.h>
#include <stdio.h>
#include <string.h>

#include "netstat.h"

/*
 * Dispatch between fetching and printing (KAME) IPsec statistics,
 * and FAST_IPSEC statistics, so the rest of netstat need not know
 * about the vagaries of the two implementations.
 */
void
ipsec_switch(u_long off, char * name)
{
	int slen, status;
	
	slen = 0;
	status = sysctlbyname("net.inet.ipsec.stats", NULL, &slen, NULL, 0);
	if (status == 0)
	    return fast_ipsec_stats(off, name);

	return ipsec_stats(off, name);
}


/*
 * Table-driven mapping from SADB algorithm codes to string names.
 */
struct alg {
	int		a;
	const char	*name;
};
static const struct alg aalgs[] = {
	{ SADB_AALG_NONE,	"none", },
	{ SADB_AALG_MD5HMAC,	"hmac-md5", },
	{ SADB_AALG_SHA1HMAC,	"hmac-sha1", },
	{ SADB_X_AALG_MD5,	"md5", },
	{ SADB_X_AALG_SHA,	"sha", },
	{ SADB_X_AALG_NULL,	"null", },
	{ SADB_X_AALG_SHA2_256,	"hmac-sha2-256", },
	{ SADB_X_AALG_SHA2_384,	"hmac-sha2-384", },
	{ SADB_X_AALG_SHA2_512,	"hmac-sha2-512", },
};
static const struct alg espalgs[] = {
	{ SADB_EALG_NONE,	"none", },
	{ SADB_EALG_DESCBC,	"des-cbc", },
	{ SADB_EALG_3DESCBC,	"3des-cbc", },
	{ SADB_EALG_NULL,	"null", },
	{ SADB_X_EALG_CAST128CBC, "cast128-cbc", },
	{ SADB_X_EALG_BLOWFISHCBC, "blowfish-cbc", },
	{ SADB_X_EALG_RIJNDAELCBC, "aes-cbc", },
};
static const struct alg ipcompalgs[] = {
	{ SADB_X_CALG_NONE,	"none", },
	{ SADB_X_CALG_OUI,	"oui", },
	{ SADB_X_CALG_DEFLATE,	"deflate", },
	{ SADB_X_CALG_LZS,	"lzs", },
};
#define	N(a)	(sizeof(a)/sizeof(a[0]))

static const char*
algname(int a, const struct alg algs[], int nalgs)
{
	static char buf[80];
	int i;

	for (i = 0; i < nalgs; i++)
		if (algs[i].a == a)
			return algs[i].name;
	snprintf(buf, sizeof(buf), "alg#%u", a);
	return buf;
}

/*
 * Print the fast_ipsec statistics.
 * Since NetBSD's netstat(1) seems not to find us for "netstat -s", 
 * but does(?) find KAME, be prepared to be called explicitly from
 * netstat's main program for "netstat -s"; but silently do nothing
 * if that happens when we are running on KAME IPsec.
 */
void
fast_ipsec_stats(u_long off, char *name)
{
	struct newipsecstat ipsecstats;
	struct ahstat ahstats;
	struct espstat espstats;
	struct ipcompstat ipcs;
	struct ipipstat ipips;
	int status, slen;
	int i;

	memset(&ipsecstats, 0, sizeof(ipsecstats));
	memset(&ahstats, 0, sizeof(ahstats));
	memset(&espstats, 0, sizeof(espstats));
	memset(&ipcs, 0, sizeof(ipcs));
	memset(&ipips, 0, sizeof(ipips));

	/* silence check */
	status = sysctlbyname("net.inet.ipsec.stats", NULL, &slen, NULL, 0);
	if (status != 0)
	    return;

	slen = sizeof(ipsecstats);
	status = sysctlbyname("net.inet.ipsec.stats", &ipsecstats, &slen,
			      NULL, 0);
	if (status < 0)
	  err(1, "net.inet.ipsec.stats");

	slen = sizeof (ahstats);
	if (sysctlbyname("net.inet.ah.stats", &ahstats, &slen, NULL, 0) < 0)
		err(1, "net.inet.ah.stats");
	slen = sizeof (espstats);
	if (sysctlbyname("net.inet.esp.stats", &espstats, &slen, NULL, 0) < 0)
		err(1, "net.inet.esp.stats");
	if (sysctlbyname("net.inet.ipcomp.stats", &ipcs, &slen, NULL, 0) < 0)
		err(1, "net.inet.ipcomp.stats");
	if (sysctlbyname("net.inet.ipip.stats", &ipips, &slen, NULL, 0) < 0)
		err(1, "net.inet.ipip.stats");

	printf("(Fast) IPsec:\n");

#define	STAT(x,fmt)	if ((x) || sflag <= 1) printf("\t%llu " fmt "\n", x)
	if (ipsecstats.ips_in_polvio+ipsecstats.ips_out_polvio)
		printf("\t%llu policy violations: %llu input %llu output\n",
		        ipsecstats.ips_in_polvio + ipsecstats.ips_out_polvio,
			ipsecstats.ips_in_polvio, ipsecstats.ips_out_polvio);
	STAT(ipsecstats.ips_out_nosa, "no SA found (output)");
	STAT(ipsecstats.ips_out_nomem, "no memory available (output)");
	STAT(ipsecstats.ips_out_noroute, "no route available (output)");
	STAT(ipsecstats.ips_out_inval, "generic errors (output)");
	STAT(ipsecstats.ips_out_bundlesa, "bundled SA processed (output)");
	STAT(ipsecstats.ips_spdcache_lookup, "SPD cache lookups");
	STAT(ipsecstats.ips_spdcache_lookup, "SPD cache misses");
#undef STAT
	printf("\n");
	
	printf("IPsec ah:\n");
#define	AHSTAT(x,fmt)	if ((x) || sflag <= 1) printf("\t%llu ah " fmt "\n", x)
	AHSTAT(ahstats.ahs_input,   "input packets processed");
	AHSTAT(ahstats.ahs_output,  "output packets processed");
	AHSTAT(ahstats.ahs_hdrops,  "headers too short");
	AHSTAT(ahstats.ahs_nopf,    "headers for unsupported address family");
	AHSTAT(ahstats.ahs_notdb,   "packets with no SA");
	AHSTAT(ahstats.ahs_badkcr, "packets dropped by crypto returning NULL mbuf");
	AHSTAT(ahstats.ahs_badauth, "packets with bad authentication");
	AHSTAT(ahstats.ahs_noxform, "packets with no xform");
	AHSTAT(ahstats.ahs_qfull, "packets dropped due to queue full");
	AHSTAT(ahstats.ahs_wrap,  "packets dropped for replay counter wrap");
	AHSTAT(ahstats.ahs_replay,  "packets dropped for possible replay");
	AHSTAT(ahstats.ahs_badauthl,"packets dropped for bad authenticator length");
	AHSTAT(ahstats.ahs_invalid, "packets with an invalid SA");
	AHSTAT(ahstats.ahs_toobig,  "packets too big");
	AHSTAT(ahstats.ahs_pdrops,  "packets blocked due to policy");
	AHSTAT(ahstats.ahs_crypto,  "failed crypto requests");
	AHSTAT(ahstats.ahs_tunnel,  "tunnel sanity check failures");

	printf("\tah histogram:\n");
	for (i = 0; i < AH_ALG_MAX; i++)
		if (ahstats.ahs_hist[i])
			printf("\t\tah packets with %s: %llu\n"
				, algname(i, aalgs, N(aalgs))
				, ahstats.ahs_hist[i]
			);
	AHSTAT(ahstats.ahs_ibytes, "bytes received");
	AHSTAT(ahstats.ahs_obytes, "bytes transmitted");
#undef AHSTAT
	printf("\n");

	printf("IPsec esp:\n");
#define	ESPSTAT(x,fmt) if ((x) || sflag <= 1) printf("\t%llu esp " fmt "\n", x)
	ESPSTAT(espstats.esps_input,	"input packets processed");
	ESPSTAT(espstats.esps_output,	"output packets processed");
	ESPSTAT(espstats.esps_hdrops,	"headers too short");
	ESPSTAT(espstats.esps_nopf,  "headers for unsupported address family");
	ESPSTAT(espstats.esps_notdb,	"packets with no SA");
	ESPSTAT(espstats.esps_badkcr,	"packets dropped by crypto returning NULL mbuf");
	ESPSTAT(espstats.esps_qfull,	"packets dropped due to queue full");
	ESPSTAT(espstats.esps_noxform,	"packets with no xform");
	ESPSTAT(espstats.esps_badilen,	"packets with bad ilen");
	ESPSTAT(espstats.esps_badenc,	"packets with bad encryption");
	ESPSTAT(espstats.esps_badauth,	"packets with bad authentication");
	ESPSTAT(espstats.esps_wrap, "packets dropped for replay counter wrap");
	ESPSTAT(espstats.esps_replay,	"packets dropped for possible replay");
	ESPSTAT(espstats.esps_invalid,	"packets with an invalid SA");
	ESPSTAT(espstats.esps_toobig,	"packets too big");
	ESPSTAT(espstats.esps_pdrops,	"packets blocked due to policy");
	ESPSTAT(espstats.esps_crypto,	"failed crypto requests");
	ESPSTAT(espstats.esps_tunnel,	"tunnel sanity check failures");
	printf("\tesp histogram:\n");
	for (i = 0; i < ESP_ALG_MAX; i++)
		if (espstats.esps_hist[i])
			printf("\t\tesp packets with %s: %llu\n"
				, algname(i, espalgs, N(espalgs))
				, espstats.esps_hist[i]
			);
	ESPSTAT(espstats.esps_ibytes, "bytes received");
	ESPSTAT(espstats.esps_obytes, "bytes transmitted");
#undef ESPSTAT
	printf("IPsec ipip:\n");

#define	IPIPSTAT(x,fmt) \
	if ((x) || sflag <= 1) printf("\t%llu ipip " fmt "\n", x)
	IPIPSTAT(ipips.ipips_ipackets,	"total input packets");
	IPIPSTAT(ipips.ipips_opackets,	"total output packets");
	IPIPSTAT(ipips.ipips_hdrops,	"packets too short for header length");
	IPIPSTAT(ipips.ipips_qfull,	"packets dropped due to queue full");
	IPIPSTAT(ipips.ipips_pdrops,	"packets blocked due to policy");
	IPIPSTAT(ipips.ipips_spoof,	"IP spoofing attempts");
	IPIPSTAT(ipips.ipips_family,	"protocol family mismatched");
	IPIPSTAT(ipips.ipips_unspec,	"missing tunnel-endpoint address");
	IPIPSTAT(ipips.ipips_ibytes,	"input bytes received");
	IPIPSTAT(ipips.ipips_obytes,	"output bytes procesesed");
#undef IPIPSTAT

	printf("IPsec ipcomp:\n");
#define	IPCOMP(x,fmt) \
	if ((x) || sflag <= 1) printf("\t%llu ipcomp " fmt "\n", x)

	IPCOMP(ipcs.ipcomps_hdrops,	"packets too short for header length");
	IPCOMP(ipcs.ipcomps_nopf,	"protocol family not supported");
	IPCOMP(ipcs.ipcomps_notdb,	"not db");
	IPCOMP(ipcs.ipcomps_badkcr,	"packets dropped by crypto returning NULL mbuf");
	IPCOMP(ipcs.ipcomps_qfull,	"queue full");
        IPCOMP(ipcs.ipcomps_noxform,	"no support for transform");
	IPCOMP(ipcs.ipcomps_wrap,  "packets dropped for replay counter wrap");
	IPCOMP(ipcs.ipcomps_input,	"input IPcomp packets");
	IPCOMP(ipcs.ipcomps_output,	"output IPcomp packets");
	IPCOMP(ipcs.ipcomps_invalid,	"specified an invalid TDB");
	IPCOMP(ipcs.ipcomps_toobig,	"packets decompressed as too big");
	IPCOMP(ipcs.ipcomps_pdrops,	"packets blocked due to policy");
	IPCOMP(ipcs.ipcomps_crypto,	"failed crypto requests");

	printf("\tIPcomp histogram:\n");
	for (i = 0; i < IPCOMP_ALG_MAX; i++)
		if (ipcs.ipcomps_hist[i])
			printf("\t\tIPcomp packets with %s: %llu\n"
				, algname(i, ipcompalgs, N(ipcompalgs))
				, ipcs.ipcomps_hist[i]
			);
	IPCOMP(ipcs.ipcomps_ibytes,	"input bytes");
	IPCOMP(ipcs.ipcomps_obytes,	"output bytes");
#undef IPCOMP
}

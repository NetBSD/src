/*	$NetBSD: ipsec.c,v 1.14.8.1 2009/05/13 19:19:59 jym Exp $	*/
/*	$KAME: ipsec.c,v 1.33 2003/07/25 09:54:32 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, 1998, and 1999 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1983, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
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
#if 0
static char sccsid[] = "from: @(#)inet.c	8.4 (Berkeley) 4/20/94";
#else
#ifdef __NetBSD__
__RCSID("$NetBSD: ipsec.c,v 1.14.8.1 2009/05/13 19:19:59 jym Exp $");
#endif
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <netinet/in.h>

#ifdef IPSEC
#include <netinet6/ipsec.h>
#include <netkey/keysock.h>
#endif

#include <err.h>
#include <kvm.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "netstat.h"

#ifdef IPSEC 
struct val2str {
	int val;
	const char *str;
};

static struct val2str ipsec_ahnames[] = {
	{ SADB_AALG_NONE, "none", },
	{ SADB_AALG_MD5HMAC, "hmac-md5", },
	{ SADB_AALG_SHA1HMAC, "hmac-sha1", },
	{ SADB_X_AALG_MD5, "md5", },
	{ SADB_X_AALG_SHA, "sha", },
	{ SADB_X_AALG_NULL, "null", },
	{ SADB_X_AALG_SHA2_256, "hmac-sha2-256", },
	{ SADB_X_AALG_SHA2_384, "hmac-sha2-384", },
	{ SADB_X_AALG_SHA2_512, "hmac-sha2-512", },
	{ SADB_X_AALG_RIPEMD160HMAC, "hmac-ripemd160", },
	{ SADB_X_AALG_AES_XCBC_MAC, "aes-xcbc-mac", },
	{ -1, NULL },
};

static struct val2str ipsec_espnames[] = {
	{ SADB_EALG_NONE, "none", },
	{ SADB_EALG_DESCBC, "des-cbc", },
	{ SADB_EALG_3DESCBC, "3des-cbc", },
	{ SADB_EALG_NULL, "null", },
	{ SADB_X_EALG_CAST128CBC, "cast128-cbc", },
	{ SADB_X_EALG_BLOWFISHCBC, "blowfish-cbc", },
	{ SADB_X_EALG_RIJNDAELCBC, "rijndael-cbc", },
	{ SADB_X_EALG_AESCTR, "aes-ctr", },
	{ -1, NULL },
};

static struct val2str ipsec_compnames[] = {
	{ SADB_X_CALG_NONE, "none", },
	{ SADB_X_CALG_OUI, "oui", },
	{ SADB_X_CALG_DEFLATE, "deflate", },
	{ SADB_X_CALG_LZS, "lzs", },
	{ -1, NULL },
};

static const char *pfkey_msgtypenames[] = {
	"reserved", "getspi", "update", "add", "delete",
	"get", "acquire", "register", "expire", "flush",
	"dump", "x_promisc", "x_pchange", "x_spdupdate", "x_spdadd",
	"x_spddelete", "x_spdget", "x_spdacquire", "x_spddump", "x_spdflush",
	"x_spdsetidx", "x_spdexpire", "x_spddelete2"
};

static uint64_t ipsecstat[IPSEC_NSTATS];

static void print_ipsecstats(void);
static const char *pfkey_msgtype_names(int);
static void ipsec_hist(const u_quad_t *, int, const struct val2str *,
	size_t, const char *);

/*
 * Dump IPSEC statistics structure.
 */
static void
ipsec_hist(const uint64_t *hist, int histmax, const struct val2str *name,
	   size_t namemax, const char *title)
{
	int first;
	int proto;
	const struct val2str *p;

	first = 1;
	for (proto = 0; proto < histmax; proto++) {
		if (hist[proto] <= 0)
			continue;
		if (first) {
			printf("\t%s histogram:\n", title);
			first = 0;
		}
		for (p = name; p && p->str; p++) {
			if (p->val == proto)
				break;
		}
		if (p && p->str) {
			printf("\t\t%s: %llu\n", p->str, (unsigned long long)hist[proto]);
		} else {
			printf("\t\t#%ld: %llu\n", (long)proto,
			    (unsigned long long)hist[proto]);
		}
	}
}

static void
print_ipsecstats(void)
{
#define	p(f, m) if (ipsecstat[f] || sflag <= 1) \
    printf(m, (unsigned long long)ipsecstat[f], plural(ipsecstat[f]))
#define	pes(f, m) if (ipsecstat[f] || sflag <= 1) \
    printf(m, (unsigned long long)ipsecstat[f], plurales(ipsecstat[f]))
#define hist(f, c, n, t) \
    ipsec_hist(&(f), (c), (n), sizeof(n)/sizeof(n[0]), (t));

	p(IPSEC_STAT_IN_SUCCESS, "\t%llu inbound packet%s processed successfully\n");
	p(IPSEC_STAT_IN_POLVIO, "\t%llu inbound packet%s violated process security "
	    "policy\n");
	p(IPSEC_STAT_IN_NOSA, "\t%llu inbound packet%s with no SA available\n");
	p(IPSEC_STAT_IN_INVAL, "\t%llu invalid inbound packet%s\n");
	p(IPSEC_STAT_IN_NOMEM, "\t%llu inbound packet%s failed due to insufficient memory\n");
	p(IPSEC_STAT_IN_BADSPI, "\t%llu inbound packet%s failed getting SPI\n");
	p(IPSEC_STAT_IN_AHREPLAY, "\t%llu inbound packet%s failed on AH replay check\n");
	p(IPSEC_STAT_IN_ESPREPLAY, "\t%llu inbound packet%s failed on ESP replay check\n");
	p(IPSEC_STAT_IN_AHAUTHSUCC, "\t%llu inbound packet%s considered authentic\n");
	p(IPSEC_STAT_IN_AHAUTHFAIL, "\t%llu inbound packet%s failed on authentication\n");
	hist(ipsecstat[IPSEC_STAT_IN_AHHIST], 256, ipsec_ahnames, "AH input");
	hist(ipsecstat[IPSEC_STAT_IN_ESPHIST], 256, ipsec_espnames, "ESP input");
	hist(ipsecstat[IPSEC_STAT_IN_COMPHIST], 256, ipsec_compnames, "IPComp input");

	p(IPSEC_STAT_OUT_SUCCESS, "\t%llu outbound packet%s processed successfully\n");
	p(IPSEC_STAT_OUT_POLVIO, "\t%llu outbound packet%s violated process security "
	    "policy\n");
	p(IPSEC_STAT_OUT_NOSA, "\t%llu outbound packet%s with no SA available\n");
	p(IPSEC_STAT_OUT_INVAL, "\t%llu invalid outbound packet%s\n");
	p(IPSEC_STAT_OUT_NOMEM, "\t%llu outbound packet%s failed due to insufficient memory\n");
	p(IPSEC_STAT_OUT_NOROUTE, "\t%llu outbound packet%s with no route\n");
	hist(ipsecstat[IPSEC_STAT_OUT_AHHIST], 256, ipsec_ahnames, "AH output");
	hist(ipsecstat[IPSEC_STAT_OUT_ESPHIST], 256, ipsec_espnames, "ESP output");
	hist(ipsecstat[IPSEC_STAT_OUT_COMPHIST], 256, ipsec_compnames, "IPComp output");

	p(IPSEC_STAT_SPDCACHELOOKUP, "\t%llu SPD cache lookup%s\n");
	pes(IPSEC_STAT_SPDCACHEMISS, "\t%llu SPD cache miss%s\n");
#undef p
#undef pes
#undef hist
}

void
ipsec_stats(u_long off, const char *name)
{

	if (use_sysctl) {
		size_t size = sizeof(ipsecstat);

		if (sysctlbyname("net.inet.ipsec.stats", ipsecstat, &size,
				 NULL, 0) == -1)
			return;
	} else {
		warnx("%s stats not available via KVM.", name);
		return;
	}

	printf ("%s:\n", name);

	print_ipsecstats();
}

static const char *
pfkey_msgtype_names(int x)
{
	const int max =
	    sizeof(pfkey_msgtypenames)/sizeof(pfkey_msgtypenames[0]);
	static char buf[20];

	if (x < max && pfkey_msgtypenames[x])
		return pfkey_msgtypenames[x];
	snprintf(buf, sizeof(buf), "#%d", x);
	return buf;
}

void
pfkey_stats(u_long off, const char *name)
{
	uint64_t pfkeystat[PFKEY_NSTATS];
	int first, type;

	if (use_sysctl) {
		size_t size = sizeof(pfkeystat);

		if (sysctlbyname("net.key.stats", pfkeystat, &size,
				 NULL, 0) == -1)
			return;
	} else {
		warnx("%s stats not available via KVM.", name);
		return;
	}

	printf ("%s:\n", name);

#define	p(f, m) if (pfkeystat[f] || sflag <= 1) \
    printf(m, (unsigned long long)pfkeystat[f], plural(pfkeystat[f]))

	/* userland -> kernel */
	p(PFKEY_STAT_OUT_TOTAL, "\t%llu request%s sent from userland\n");
	p(PFKEY_STAT_OUT_BYTES, "\t%llu byte%s sent from userland\n");
	for (first = 1, type = 0; type < 256; type++) {
		if (pfkeystat[PFKEY_STAT_OUT_MSGTYPE + type] == 0)
			continue;
		if (first) {
			printf("\thistogram by message type:\n");
			first = 0;
		}
		printf("\t\t%s: %llu\n", pfkey_msgtype_names(type),
		    (unsigned long long)pfkeystat[PFKEY_STAT_OUT_MSGTYPE + type]);
	}
	p(PFKEY_STAT_OUT_INVLEN, "\t%llu message%s with invalid length field\n");
	p(PFKEY_STAT_OUT_INVVER, "\t%llu message%s with invalid version field\n");
	p(PFKEY_STAT_OUT_INVMSGTYPE, "\t%llu message%s with invalid message type field\n");
	p(PFKEY_STAT_OUT_TOOSHORT, "\t%llu message%s too short\n");
	p(PFKEY_STAT_OUT_NOMEM, "\t%llu message%s with memory allocation failure\n");
	p(PFKEY_STAT_OUT_DUPEXT, "\t%llu message%s with duplicate extension\n");
	p(PFKEY_STAT_OUT_INVEXTTYPE, "\t%llu message%s with invalid extension type\n");
	p(PFKEY_STAT_OUT_INVSATYPE, "\t%llu message%s with invalid sa type\n");
	p(PFKEY_STAT_OUT_INVADDR, "\t%llu message%s with invalid address extension\n");

	/* kernel -> userland */
	p(PFKEY_STAT_IN_TOTAL, "\t%llu request%s sent to userland\n");
	p(PFKEY_STAT_IN_BYTES, "\t%llu byte%s sent to userland\n");
	for (first = 1, type = 0; type < 256; type++) {
		if (pfkeystat[PFKEY_STAT_IN_MSGTYPE + type] == 0)
			continue;
		if (first) {
			printf("\thistogram by message type:\n");
			first = 0;
		}
		printf("\t\t%s: %llu\n", pfkey_msgtype_names(type),
		    (unsigned long long)pfkeystat[PFKEY_STAT_IN_MSGTYPE + type]);
	}
	p(PFKEY_STAT_IN_MSGTARGET + KEY_SENDUP_ONE,
	    "\t%llu message%s toward single socket\n");
	p(PFKEY_STAT_IN_MSGTARGET + KEY_SENDUP_ALL,
	    "\t%llu message%s toward all sockets\n");
	p(PFKEY_STAT_IN_MSGTARGET + KEY_SENDUP_REGISTERED,
	    "\t%llu message%s toward registered sockets\n");
	p(PFKEY_STAT_IN_NOMEM, "\t%llu message%s with memory allocation failure\n");
#undef p
}
#endif /*IPSEC*/

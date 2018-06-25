/*	$NetBSD: af_inet.c,v 1.24.12.1 2018/06/25 07:25:36 pgoyette Exp $	*/

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
__RCSID("$NetBSD: af_inet.c,v 1.24.12.1 2018/06/25 07:25:36 pgoyette Exp $");
#endif /* not lint */

#include <sys/param.h> 
#include <sys/ioctl.h> 
#include <sys/socket.h>

#include <net/if.h> 
#include <netinet/in.h>
#include <netinet/in_var.h>

#include <arpa/inet.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <util.h>

#include "env.h"
#include "extern.h"
#include "af_inetany.h"
#include "prog_ops.h"

static void in_constructor(void) __attribute__((constructor));
static void in_status(prop_dictionary_t, prop_dictionary_t, bool);
static void in_commit_address(prop_dictionary_t, prop_dictionary_t);
static bool in_addr_tentative(struct ifaddrs *);
static bool in_addr_tentative_or_detached(struct ifaddrs *);
static in_addr_t in_netmask(struct sockaddr *);;
static int  in_prefixlen(struct sockaddr *);
static void in_alias(struct ifaddrs *, prop_dictionary_t, prop_dictionary_t);

static struct afswtch af = {
	.af_name = "inet", .af_af = AF_INET, .af_status = in_status,
	.af_addr_commit = in_commit_address,
	.af_addr_tentative = in_addr_tentative,
	.af_addr_tentative_or_detached = in_addr_tentative_or_detached
};

static in_addr_t
in_netmask(struct sockaddr *sa)
{
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));
	memcpy(&sin, sa, sa->sa_len);
	return ntohl(sin.sin_addr.s_addr);
}

static int
in_prefixlen(struct sockaddr *sa)
{
	in_addr_t mask;
	int cidr;

	mask = in_netmask(sa);
	if (mask == 0)			/* mask 0 ==> /0 */
		return 0;

	cidr = 33 - ffs(mask);		/* 33 - (1 .. 32) -> 32 .. 1 */

	if (cidr < 32) {		/* more than 1 bit in mask */
		/* check for non-contig netmask */
		if ((mask ^ (((1U << cidr) - 1) << (32 - cidr))) != 0)
			return -1;	/* noncontig, no pfxlen */
	}

	return cidr;
}

static void
in_alias(struct ifaddrs *ifa, prop_dictionary_t env, prop_dictionary_t oenv)
{
	char hbuf[NI_MAXHOST];
	const int niflag = Nflag ? 0 : NI_NUMERICHOST;
	int pfxlen;
	char fbuf[1024];

	if (lflag)
		return;

	if (getnameinfo(ifa->ifa_addr, ifa->ifa_addr->sa_len,
			hbuf, sizeof(hbuf), NULL, 0, niflag))
		strlcpy(hbuf, "", sizeof(hbuf));	/* some message? */
	printf("\tinet %s", hbuf);
	pfxlen = in_prefixlen(ifa->ifa_netmask);
	if (pfxlen >= 0)
		printf("/%d", pfxlen);

	if (ifa->ifa_flags & IFF_POINTOPOINT) {
		if (getnameinfo(ifa->ifa_dstaddr, ifa->ifa_dstaddr->sa_len,
				hbuf, sizeof(hbuf), NULL, 0, niflag))
			strlcpy(hbuf, "", sizeof(hbuf)); /* some message? */
		printf(" -> %s", hbuf);
	}

	if (pfxlen < 0)
		printf(" netmask %#x", in_netmask(ifa->ifa_netmask));

	if (ifa->ifa_flags & IFF_BROADCAST) {
		if (getnameinfo(ifa->ifa_broadaddr, ifa->ifa_broadaddr->sa_len,
				hbuf, sizeof(hbuf), NULL, 0, niflag))
			strlcpy(hbuf, "", sizeof(hbuf)); /* some message? */
		printf(" broadcast %s", hbuf);
	}

	(void)snprintb(fbuf, sizeof(fbuf), IN_IFFBITS, ifa->ifa_addrflags);
	printf(" flags %s", fbuf);
}

static void
in_status(prop_dictionary_t env, prop_dictionary_t oenv, bool force)
{
	struct ifaddrs *ifap, *ifa;
	bool printprefs = false;
	const char *ifname;

	if ((ifname = getifname(env)) == NULL)
		err(EXIT_FAILURE, "%s: getifname", __func__);

	if (getifaddrs(&ifap) != 0)
		err(EXIT_FAILURE, "getifaddrs");

	printprefs = ifa_any_preferences(ifname, ifap, AF_INET);

	for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
		if (strcmp(ifname, ifa->ifa_name) != 0)
			continue;
		if (ifa->ifa_addr->sa_family != AF_INET)
			continue;
		/* The first address is not an alias. */
		in_alias(ifa, env, oenv);
		if (printprefs)
			ifa_print_preference(ifa->ifa_name, ifa->ifa_addr);
		printf("\n");
	}
	freeifaddrs(ifap);
}

static void
in_commit_address(prop_dictionary_t env, prop_dictionary_t oenv)
{
	struct ifreq in_ifr;
	struct in_aliasreq in_ifra;
	struct afparam inparam = {
		  .req = BUFPARAM(in_ifra)
		, .dgreq = BUFPARAM(in_ifr)
		, .name = {
			  {.buf = in_ifr.ifr_name,
			   .buflen = sizeof(in_ifr.ifr_name)}
			, {.buf = in_ifra.ifra_name,
			   .buflen = sizeof(in_ifra.ifra_name)}
		  }
		, .dgaddr = BUFPARAM(in_ifr.ifr_addr)
		, .addr = BUFPARAM(in_ifra.ifra_addr)
		, .dst = BUFPARAM(in_ifra.ifra_dstaddr)
		, .brd = BUFPARAM(in_ifra.ifra_broadaddr)
		, .mask = BUFPARAM(in_ifra.ifra_mask)
		, .aifaddr = IFADDR_PARAM(SIOCAIFADDR)
		, .difaddr = IFADDR_PARAM(SIOCDIFADDR)
		, .gifaddr = IFADDR_PARAM(SIOCGIFADDR)
		, .defmask = {.buf = NULL, .buflen = 0}
	};
	memset(&in_ifr, 0, sizeof(in_ifr));
	memset(&in_ifra, 0, sizeof(in_ifra));
	commit_address(env, oenv, &inparam);
}

#ifdef SIOCGIFAFLAG_IN
static bool
in_addr_flags(struct ifaddrs *ifa, int flags)
{
	int s;
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));
	estrlcpy(ifr.ifr_name, ifa->ifa_name, sizeof(ifr.ifr_name));
	ifr.ifr_addr = *ifa->ifa_addr;
	if ((s = getsock(AF_INET)) == -1)
		err(EXIT_FAILURE, "%s: getsock", __func__);
	if (prog_ioctl(s, SIOCGIFAFLAG_IN, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCGIFAFLAG_IN");
	return ifr.ifr_addrflags & flags ? true : false;
	return false;
}
#endif

static bool
in_addr_tentative(struct ifaddrs *ifa)
{

#ifdef SIOCGIFAFLAG_IN
	return in_addr_flags(ifa, IN_IFF_TENTATIVE);
#else
	return false;
#endif
}

static bool
in_addr_tentative_or_detached(struct ifaddrs *ifa)
{

#ifdef SIOCGIFAFLAG_IN
	return in_addr_flags(ifa, IN_IFF_TENTATIVE | IN_IFF_DETACHED);
#else
	return false;
#endif
}

static void
in_constructor(void)
{
	register_family(&af);
}

/*	$NetBSD: af_iso.c,v 1.13 2008/07/15 21:27:58 dyoung Exp $	*/

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
__RCSID("$NetBSD: af_iso.c,v 1.13 2008/07/15 21:27:58 dyoung Exp $");
#endif /* not lint */

#include <err.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <util.h>

#include <sys/param.h> 
#include <sys/ioctl.h> 
#include <sys/socket.h>

#include <net/if.h> 

#define EON
#include <netiso/iso.h>
#include <netiso/iso_var.h>

#include "env.h"
#include "parse.h"
#include "extern.h"
#include "af_inetany.h"

#define	DEFNSELLEN	1

static void iso_constructor(void) __attribute__((constructor));
static int setnsellength(prop_dictionary_t, prop_dictionary_t);
static void iso_status(prop_dictionary_t, prop_dictionary_t, bool);
static void iso_commit_address(prop_dictionary_t, prop_dictionary_t);

static struct afswtch isoaf = {
	.af_name = "iso", .af_af = AF_ISO, .af_status = iso_status,
	.af_addr_commit = iso_commit_address
};

struct pinteger parse_snpaoffset = PINTEGER_INITIALIZER1(&snpaoffset,
    "snpaoffset", INT_MIN, INT_MAX, 10, NULL, "snpaoffset",
    &command_root.pb_parser);
struct pinteger parse_nsellength = PINTEGER_INITIALIZER1(&nsellength,
    "nsellength", 0, UINT8_MAX, 10, setnsellength, "nsellength",
    &command_root.pb_parser);

static const struct kwinst isokw[] = {
	  {.k_word = "nsellength", .k_nextparser = &parse_nsellength.pi_parser}
	, {.k_word = "snpaoffset", .k_nextparser = &parse_snpaoffset.pi_parser}
};

struct pkw iso = PKW_INITIALIZER(&iso, "ISO", NULL, NULL,
    isokw, __arraycount(isokw), NULL);

static cmdloop_branch_t branch;

static void
fixnsel(struct sockaddr_iso *siso, uint8_t nsellength)
{
	siso->siso_tlen = nsellength;
}

/* fixup mask */
static int
iso_pre_aifaddr(prop_dictionary_t env, const struct afparam *param)
{
	struct sockaddr_iso *siso;

	siso = param->mask.buf;
	siso->siso_len = TSEL(siso) - (char *)(siso);
	siso->siso_nlen = 0;
	return 0;
}

static void
iso_commit_address(prop_dictionary_t env, prop_dictionary_t oenv)
{
	uint8_t nsellength;
	struct iso_ifreq ifr = {.ifr_Addr = {.siso_tlen = DEFNSELLEN}};
	struct iso_aliasreq ifra = {
		.ifra_dstaddr = {.siso_tlen = DEFNSELLEN},
		.ifra_addr = {.siso_tlen = DEFNSELLEN}
	};
	struct afparam isoparam = {
		  .req = BUFPARAM(ifra)
		, .dgreq = BUFPARAM(ifr)
		, .name = {
			  {.buf = ifr.ifr_name,
			   .buflen = sizeof(ifr.ifr_name)}
			, {.buf = ifra.ifra_name,
			   .buflen = sizeof(ifra.ifra_name)}
		  }
		, .dgaddr = BUFPARAM(ifr.ifr_Addr)
		, .addr = BUFPARAM(ifra.ifra_addr)
		, .dst = BUFPARAM(ifra.ifra_dstaddr)
		, .brd = BUFPARAM(ifra.ifra_broadaddr)
		, .mask = BUFPARAM(ifra.ifra_mask)
		, .aifaddr = IFADDR_PARAM(SIOCAIFADDR_ISO)
		, .difaddr = IFADDR_PARAM(SIOCDIFADDR_ISO)
		, .gifaddr = IFADDR_PARAM(SIOCGIFADDR_ISO)
		, .defmask = {.buf = NULL, .buflen = 0}
		, .pre_aifaddr = iso_pre_aifaddr
	};
	int64_t snpaoffset;

	if (prop_dictionary_get_int64(env, "snpaoffset", &snpaoffset))
		ifra.ifra_snpaoffset = snpaoffset;

	if (prop_dictionary_get_uint8(env, "nsellength", &nsellength)) {
		fixnsel(&ifr.ifr_Addr, nsellength);
		fixnsel(&ifra.ifra_addr, nsellength);
		fixnsel(&ifra.ifra_dstaddr, nsellength);
	}
	commit_address(env, oenv, &isoparam);
}

static int
setnsellength(prop_dictionary_t env, prop_dictionary_t oenv)
{
	int af;

	if ((af = getaf(env)) == -1 || af != AF_ISO)
		errx(EXIT_FAILURE, "Setting NSEL length valid only for ISO");

	return 0;

}

static void
iso_status(prop_dictionary_t env, prop_dictionary_t oenv, bool force)
{
	struct sockaddr_iso *siso;
	struct iso_ifreq isoifr;
	int s;
	const char *ifname;
	unsigned short flags;

	if ((ifname = getifinfo(env, oenv, &flags)) == NULL)
		err(EXIT_FAILURE, "%s: getifinfo", __func__);

	if ((s = getsock(AF_ISO)) == -1) {
		if (errno == EAFNOSUPPORT)
			return;
		err(EXIT_FAILURE, "socket");
	}
	memset(&isoifr, 0, sizeof(isoifr));
	estrlcpy(isoifr.ifr_name, ifname, sizeof(isoifr.ifr_name));
	if (ioctl(s, SIOCGIFADDR_ISO, &isoifr) == -1) {
		if (errno == EADDRNOTAVAIL || errno == EAFNOSUPPORT) {
			if (!force)
				return;
			memset(&isoifr.ifr_Addr, 0, sizeof(isoifr.ifr_Addr));
		} else
			warn("SIOCGIFADDR_ISO");
	}
	strlcpy(isoifr.ifr_name, ifname, sizeof(isoifr.ifr_name));
	siso = &isoifr.ifr_Addr;
	printf("\tiso %s", iso_ntoa(&siso->siso_addr));
	if (ioctl(s, SIOCGIFNETMASK_ISO, &isoifr) == -1) {
		if (errno == EADDRNOTAVAIL)
			memset(&isoifr.ifr_Addr, 0, sizeof(isoifr.ifr_Addr));
		else
			warn("SIOCGIFNETMASK_ISO");
	} else {
		if (siso->siso_len > offsetof(struct sockaddr_iso, siso_addr))
			siso->siso_addr.isoa_len = siso->siso_len
			    - offsetof(struct sockaddr_iso, siso_addr);
		printf(" netmask %s", iso_ntoa(&siso->siso_addr));
	}

	if (flags & IFF_POINTOPOINT) {
		if (ioctl(s, SIOCGIFDSTADDR_ISO, &isoifr) == -1) {
			if (errno == EADDRNOTAVAIL)
			    memset(&isoifr.ifr_Addr, 0,
				sizeof(isoifr.ifr_Addr));
			else
			    warn("SIOCGIFDSTADDR_ISO");
		}
		strlcpy(isoifr.ifr_name, ifname, sizeof(isoifr.ifr_name));
		siso = &isoifr.ifr_Addr;
		printf(" --> %s", iso_ntoa(&siso->siso_addr));
	}
	printf("\n");
}

static void
iso_constructor(void)
{
	register_family(&isoaf);
	cmdloop_branch_init(&branch, &iso.pk_parser);
	register_cmdloop_branch(&branch);
}

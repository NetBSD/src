/*	$NetBSD: af_link.c,v 1.2 2008/05/13 18:10:17 dyoung Exp $	*/

/*-
 * Copyright (c) 2008 David Young.  All rights reserved.
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
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: af_link.c,v 1.2 2008/05/13 18:10:17 dyoung Exp $");
#endif /* not lint */

#include <sys/param.h> 
#include <sys/ioctl.h> 
#include <sys/socket.h>

#include <net/if.h> 
#include <net/if_dl.h> 

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
#include "af_link.h"
#include "af_inetany.h"

void
link_status(prop_dictionary_t env, prop_dictionary_t oenv, bool force)
{
	const char *delim, *ifname;
	int i, s;
	struct ifaddrs *ifa, *ifap;
	const struct sockaddr_dl *sdl;
	struct if_laddrreq iflr;
	const uint8_t *octets;

	if ((ifname = getifname(env)) == NULL)
		err(EXIT_FAILURE, "%s: getifname", __func__);

	if ((s = getsock(AF_LINK)) == -1)
		err(EXIT_FAILURE, "%s: getsock", __func__);

	if (getifaddrs(&ifap) == -1)
		err(EXIT_FAILURE, "%s: getifaddrs", __func__);

	memset(&iflr, 0, sizeof(iflr));

	strlcpy(iflr.iflr_name, ifname, sizeof(iflr.iflr_name));

	for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
		if (strcmp(ifname, ifa->ifa_name) != 0)
			continue;
		if (ifa->ifa_addr->sa_family != AF_LINK)
			continue;
		if (ifa->ifa_data != NULL)
			continue;

		sdl = satocsdl(ifa->ifa_addr);

		memcpy(&iflr.addr, ifa->ifa_addr, MIN(ifa->ifa_addr->sa_len,
		    sizeof(iflr.addr)));
		iflr.flags = IFLR_PREFIX;
		iflr.prefixlen = sdl->sdl_alen * NBBY;

		if (ioctl(s, SIOCGLIFADDR, &iflr) == -1)
			err(EXIT_FAILURE, "%s: ioctl", __func__);

                if ((iflr.flags & IFLR_ACTIVE) != 0)
			continue;

		octets = (const uint8_t *)&sdl->sdl_data[sdl->sdl_nlen];

		delim = "\tlink ";
		for (i = 0; i < sdl->sdl_alen; i++) {
			printf("%s%02" PRIx8, delim, octets[i]);
			delim = ":";
		}
		printf("\n");
	}
}

static int
link_pre_aifaddr(prop_dictionary_t env, struct afparam *param)
{
	bool active;
	struct if_laddrreq *iflr = param->req.buf;

	if (prop_dictionary_get_bool(env, "active", &active) && active)
		iflr->flags |= IFLR_ACTIVE;

	return 0;
}

void
link_commit_address(prop_dictionary_t env, prop_dictionary_t oenv)
{
	struct if_laddrreq dgreq = {
		.addr = {
			.ss_family = AF_LINK,
			.ss_len = sizeof(dgreq.addr),
		},
	};
	struct if_laddrreq req = {
		.addr = {
			.ss_family = AF_LINK,
			.ss_len = sizeof(req.addr),
		}
	};
	struct afparam linkparam = {
		  .req = BUFPARAM(req)
		, .dgreq = BUFPARAM(dgreq)
		, .name = {
			{.buf = dgreq.iflr_name,
			 .buflen = sizeof(dgreq.iflr_name)},
			{.buf = req.iflr_name,
			 .buflen = sizeof(req.iflr_name)}
		  }
		, .dgaddr = BUFPARAM(dgreq.addr)
		, .addr = BUFPARAM(req.addr)
		, .aifaddr = IFADDR_PARAM(SIOCALIFADDR)
		, .difaddr = IFADDR_PARAM(SIOCDLIFADDR)
		, .gifaddr = IFADDR_PARAM(0)
		, .pre_aifaddr = link_pre_aifaddr
	};
	commit_address(env, oenv, &linkparam);
}

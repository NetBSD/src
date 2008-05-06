/*	$NetBSD: vlan.c,v 1.6 2008/05/06 18:58:47 dyoung Exp $	*/

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
__RCSID("$NetBSD: vlan.c,v 1.6 2008/05/06 18:58:47 dyoung Exp $");
#endif /* not lint */

#include <sys/param.h> 
#include <sys/ioctl.h> 

#include <net/if.h> 
#include <net/if_ether.h>
#include <net/if_vlanvar.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <util.h>

#include "env.h"
#include "extern.h"
#include "vlan.h"

struct pinteger vlan = PINTEGER_INITIALIZER1(&vlan, "vlan", 0, USHRT_MAX, 10,
    setvlan, "vlan", &command_root.pb_parser);

struct piface vlanif = PIFACE_INITIALIZER(&vlanif, "vlanif", setvlanif,
    "vlanif", &command_root.pb_parser);

static int
checkifname(const char *ifname)
{

	return strncmp(ifname, "vlan", 4) != 0 ||
	    !isdigit((unsigned char)ifname[4]);
}

static int
getvlan(prop_dictionary_t env, struct ifreq *ifr, struct vlanreq *vlr,
    bool quiet)
{
	int s;
	const char *ifname;

	if ((s = getsock(AF_UNSPEC)) == -1)
		err(EXIT_FAILURE, "%s: getsock", __func__);
	if ((ifname = getifname(env)) == NULL)
		err(EXIT_FAILURE, "%s: getifname", __func__);

	memset(ifr, 0, sizeof(*ifr));
	memset(vlr, 0, sizeof(*vlr));

	if (checkifname(ifname)) {
		if (quiet)
			return -1;
		errx(EXIT_FAILURE, "valid only with vlan(4) interfaces");
	}

	estrlcpy(ifr->ifr_name, ifname, sizeof(ifr->ifr_name));
	ifr->ifr_data = vlr;

	if (ioctl(s, SIOCGETVLAN, ifr) == -1)
		return -1;

	return s;
}

int
setvlan(prop_dictionary_t env, prop_dictionary_t xenv)
{
	struct vlanreq vlr;
	int s;
	int64_t tag;
	struct ifreq ifr;

	if ((s = getvlan(env, &ifr, &vlr, false)) == -1)
		err(EXIT_FAILURE, "%s: getvlan", __func__);

	if (!prop_dictionary_get_int64(env, "vlantag", &tag)) {
		errno = ENOENT;
		return -1;
	}

	vlr.vlr_tag = tag;

	if (ioctl(s, SIOCSETVLAN, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCSETVLAN");
	return 0;
}

int
setvlanif(prop_dictionary_t env, prop_dictionary_t xenv)
{
	struct vlanreq vlr;
	int s;
	const char *parent;
	int64_t tag;
	struct ifreq ifr;

	if ((s = getvlan(env, &ifr, &vlr, false)) == -1)
		err(EXIT_FAILURE, "%s: getsock", __func__);

	if (!prop_dictionary_get_int64(env, "vlantag", &tag)) {
		errno = ENOENT;
		return -1;
	}

	if (!prop_dictionary_get_cstring_nocopy(env, "vlanif", &parent)) {
		errno = ENOENT;
		return -1;
	}
	strlcpy(vlr.vlr_parent, parent, sizeof(vlr.vlr_parent));
	if (strcmp(parent, "") != 0)
		vlr.vlr_tag = (unsigned short)tag;

	if (ioctl(s, SIOCSETVLAN, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCSETVLAN");
	return 0;
}

void
vlan_status(prop_dictionary_t env, prop_dictionary_t oenv)
{
	struct vlanreq vlr;
	int s;
	struct ifreq ifr;

	if ((s = getvlan(env, &ifr, &vlr, true)) == -1)
		return;

	if (vlr.vlr_tag || vlr.vlr_parent[0] != '\0')
		printf("\tvlan: %d parent: %s\n",
		    vlr.vlr_tag, vlr.vlr_parent[0] == '\0' ?
		    "<none>" : vlr.vlr_parent);
}

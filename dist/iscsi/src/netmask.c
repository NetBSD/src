/* $NetBSD: netmask.c,v 1.8.4.1 2008/01/09 01:26:14 matt Exp $ */

/*
 * Copyright © 2006 Alistair Crooks.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "config.h"

#include <sys/types.h>
#include <sys/param.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "iscsiutil.h"

enum {
	NETMASK_BUFFER_SIZE = 256
};

/* this struct is used to define a magic netmask value */
typedef struct magic_t {
	const char	*magic;	/* string to match */
	const char	*xform;	/* string to transform it into */
} magic_t;


static magic_t	magics[] = {
	{	"any",	"0/0"	},
	{	"all",	"0/0"	},
	{	"none",	"0/32"	},
	{	NULL,	NULL	},
};

#ifndef ISCSI_HTONL
#define ISCSI_HTONL(x)	htonl(x)
#endif

/* return 1 if address is in netmask's range */
int
allow_netmask(const char *netmaskarg, const char *addr)
{
	struct in_addr	 a;
	struct in_addr	 m;
	const char	*netmask;
	magic_t		*mp;
	char	 	 maskaddr[NETMASK_BUFFER_SIZE];
	char		*cp;
	int		 slash;
	int		 i;

	/* firstly check for any magic values in the netmask */
	netmask = netmaskarg;
	for (mp = magics ; mp->magic ; mp++) {
		if (strcmp(netmask, mp->magic) == 0) {
			netmask = mp->xform;
			break;
		}
	}

	/* find out if slash notation has been used */
	(void) memset(&a, 0x0, sizeof(a));
	if ((cp = strchr(netmask, '/')) == NULL) {
		(void) strlcpy(maskaddr, netmask, sizeof(maskaddr));
		slash = 32;
	} else {
		(void) strlcpy(maskaddr, netmask, MIN(sizeof(maskaddr), (int)(cp - netmask) + 1));
		slash = atoi(cp + 1);
	}

	/* if we have a wildcard "slash" netmask, then we allow it */
	if (slash == 0) {
		return 1;
	}

	/* canonicalise IPv4 address to dotted quad */
	for (i = 0, cp = maskaddr ; *cp ; cp++) {
		if (*cp == '.') {
			i += 1;
		}
	}
	for ( ; i < 3 ; i++) {
		(void) snprintf(cp, sizeof(maskaddr) - (int)(cp - maskaddr), ".0");
		cp += 2;
	}

	/* translate netmask to in_addr */
	if (!inet_aton(maskaddr, &m)) {
		(void) fprintf(stderr, "allow_netmask: can't interpret mask `%s' as an IPv4 address\n", maskaddr);
		return 0;
	}

	/* translate address to in_addr */
	if (!inet_aton(addr, &a)) {
		(void) fprintf(stderr, "allow_netmask: can't interpret address `%s' as an IPv4 address\n", addr);
		return 0;
	}

#ifdef ALLOW_NETMASK_DEBUG
	printf("addr %s %08x, mask %s %08x, slash %d\n", addr, (ISCSI_HTONL(a.s_addr) >> (32 - slash)), maskaddr, (ISCSI_HTONL(m.s_addr) >> (32 - slash)), slash);
#endif

	/* and return 1 if address is in netmask */
	return (ISCSI_HTONL(a.s_addr) >> (32 - slash)) == (ISCSI_HTONL(m.s_addr) >> (32 - slash));
}

#ifdef ALLOW_NETMASK_DEBUG
int
main(int argc, char **argv)
{
	int	i;

	for (i = 1 ; i < argc ; i+= 2) {
		if (allow_netmask(argv[i], argv[i + 1])) {
			printf("mask %s matches addr %s\n\n", argv[i], argv[i + 1]);
		} else {
			printf("No match for mask %s from addr %s\n\n", argv[i], argv[i + 1]);
		}
	}
	exit(EXIT_SUCCESS);
}
#endif

#if 0
[11:33:02] agc@sys3 ...local/src/netmask 248 > ./n 10.4/16 10.4.0.29 10.4/16 10.5.0.29 10.4/0 10.4.0.19 10.4 10.4.0.19 10.4.3/8 10.4.3.7 10.4.3/24 10.4.3.7 
mask 10.4/16 matches addr 10.4.0.29

No match for mask 10.4/16 from addr 10.5.0.29

mask 10.4/0 matches addr 10.4.0.19

No match for mask 10.4 from addr 10.4.0.19

mask 10.4.3/8 matches addr 10.4.3.7

mask 10.4.3/24 matches addr 10.4.3.7

[14:44:52] agc@sys3 ...local/src/netmask 249 >
#endif

/*	$NetBSD: if_indextoname.c,v 1.3 2000/07/06 02:54:55 christos Exp $	*/
/*	$KAME: if_indextoname.c,v 1.4 2000/04/24 10:08:41 itojun Exp $	*/

/*-
 * Copyright (c) 1997, 2000
 *	Berkeley Software Design, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY Berkeley Software Design, Inc. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Berkeley Software Design, Inc. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	BSDI Id: if_indextoname.c,v 2.3 2000/04/17 22:38:05 dab Exp
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: if_indextoname.c,v 1.3 2000/07/06 02:54:55 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if_dl.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <string.h>

#ifdef __weak_alias
__weak_alias(if_indextoname,_if_indextoname)
#endif

/*
 * From RFC 2133:
 *
 * 4.2.  Index-to-Name
 *
 *    The second function maps an interface index into its corresponding
 *    name.
 *
 *        #include <net/if.h>
 *
 *        char  *if_indextoname(unsigned int ifindex, char *ifname);
 *
 *    The ifname argument must point to a buffer of at least IFNAMSIZ bytes
 *    into which the interface name corresponding to the specified index is
 *    returned.  (IFNAMSIZ is also defined in <net/if.h> and its value
 *    includes a terminating null byte at the end of the interface name.)
 *    This pointer is also the return value of the function.  If there is
 *    no interface corresponding to the specified index, NULL is returned.
 */

char *
if_indextoname(unsigned int ifindex, char *ifname)
{
	struct ifaddrs *ifaddrs, *ifa;

	if (getifaddrs(&ifaddrs) < 0)
		return(NULL);

	for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr &&
		    ifa->ifa_addr->sa_family == AF_LINK &&
		    ifindex == ((struct sockaddr_dl*)
			(void *)ifa->ifa_addr)->sdl_index)
			break;
	}

	if (ifa == NULL)
		ifname = NULL;
	else
		strncpy(ifname, ifa->ifa_name, IFNAMSIZ);

	freeifaddrs(ifaddrs);

	return(ifname);
}

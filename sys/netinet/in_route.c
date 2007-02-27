/*	$NetBSD: in_route.c,v 1.3.2.2 2007/02/27 16:54:53 yamt Exp $	*/

/*
 * Copyright (c) 2006 David Young.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of David Young may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY David Young ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL David
 * Young BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: in_route.c,v 1.3.2.2 2007/02/27 16:54:53 yamt Exp $");

#include "opt_inet.h"
#include "opt_in_route.h"

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/queue.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/radix.h>
#ifdef RADIX_MPATH
#include <net/radix_mpath.h>
#endif
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>
#include <netinet/in_ifattach.h>
#include <netinet/in_pcb.h>
#include <netinet/in_proto.h>
#include <netinet/in_route.h>

LIST_HEAD(in_rtlist, route) in_rtcache_head =
    LIST_HEAD_INITIALIZER(in_rtcache_head);

#ifdef IN_RTFLUSH_DEBUG
#define	in_rtcache_debug() _in_rtcache_debug
#else /* IN_RTFLUSH_DEBUG */
#define	in_rtcache_debug() 0
#endif /* IN_RTFLUSH_DEBUG */

#ifdef IN_RTFLUSH_DEBUG
static int _in_rtcache_debug = 0;

SYSCTL_SETUP(sysctl_net_inet_ip_rtcache_setup,
    "sysctl net.inet.ip.rtcache_debug setup")
{
	/* XXX do not duplicate */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "net", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "inet",
		       SYSCTL_DESCR("PF_INET related settings"),
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "ip",
		       SYSCTL_DESCR("IPv4 related settings"),
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET, IPPROTO_IP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
	               CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
		       "rtcache_debug",
		       SYSCTL_DESCR("Debug IP route cache"),
		       NULL, 0, &_in_rtcache_debug, 0,
		       CTL_NET, PF_INET, IPPROTO_IP, CTL_CREATE, CTL_EOL);
}
#endif /* IN_RTFLUSH_DEBUG */

void
in_rtcache(struct route *ro)
{
	KASSERT(ro->ro_rt != NULL);
	KASSERT(rtcache_getdst(ro)->sa_family == AF_INET);
	LIST_INSERT_HEAD(&in_rtcache_head, ro, ro_rtcache_next);
}

void
in_rtflush(struct route *ro)
{
	KASSERT(rtcache_getdst(ro)->sa_family == AF_INET);
	KASSERT(ro->ro_rt == NULL);
	LIST_REMOVE(ro, ro_rtcache_next);

	if (in_rtcache_debug()) {
		printf("%s: freeing %s\n", __func__,
		    inet_ntoa((satocsin(rtcache_getdst(ro)))->sin_addr));
	}
}

void
in_rtflushall(void)
{
	int s;
	struct route *ro;

	s = splnet();

	if (in_rtcache_debug())
		printf("%s: enter\n", __func__);

	while ((ro = LIST_FIRST(&in_rtcache_head)) != NULL) {
		KASSERT(ro->ro_rt != NULL);
		rtcache_free(ro);
	}
	splx(s);
}

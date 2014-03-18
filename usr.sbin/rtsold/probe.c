/*	$NetBSD: probe.c,v 1.12 2014/03/18 00:16:49 christos Exp $	*/
/*	$KAME: probe.c,v 1.15 2002/05/31 21:22:08 itojun Exp $	*/

/*
 * Copyright (C) 1998 WIDE Project.
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/queue.h>

#include <net/if.h>
#include <net/if_dl.h>

#include <netinet/in.h>
#include <netinet6/in6_var.h>
#include <netinet/icmp6.h>
#include <netinet6/nd6.h>

#include <arpa/inet.h>

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <stdlib.h>

#include "rtsold.h"

static struct msghdr sndmhdr;
static struct iovec sndiov[2];
static int probesock;
static void sendprobe(struct in6_addr *, struct ifinfo *);

int
probe_init(void)
{
	size_t scmsglen = CMSG_SPACE(sizeof(struct in6_pktinfo)) +
	    CMSG_SPACE(sizeof(int));
	static u_char *sndcmsgbuf = NULL;

	if (sndcmsgbuf == NULL &&
	    (sndcmsgbuf = malloc(scmsglen)) == NULL) {
		warnmsg(LOG_ERR, __func__, "malloc failed");
		return -1;
	}

	if ((probesock = socket(AF_INET6, SOCK_RAW, IPPROTO_NONE)) < 0) {
		warnmsg(LOG_ERR, __func__, "socket: %s", strerror(errno));
		return -1;
	}

	/* make the socket send-only */
	if (shutdown(probesock, 0)) {
		warnmsg(LOG_ERR, __func__, "shutdown: %s", strerror(errno));
		return -1;
	}

	/* initialize msghdr for sending packets */
	sndmhdr.msg_namelen = sizeof(struct sockaddr_in6);
	sndmhdr.msg_iov = sndiov;
	sndmhdr.msg_iovlen = 1;
	sndmhdr.msg_control = sndcmsgbuf;
	sndmhdr.msg_controllen = (socklen_t)scmsglen;
	return 0;
}

/*
 * Probe if each router in the default router list is still alive.
 */
void
defrouter_probe(struct ifinfo *ifinfo)
{
	char ntopbuf[INET6_ADDRSTRLEN];
	struct in6_drlist dr;
	int s, i;
	int ifindex = ifinfo->sdl->sdl_index;

	if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
		warnmsg(LOG_ERR, __func__, "socket: %s", strerror(errno));
		return;
	}
	memset(&dr, 0, sizeof(dr));
	strlcpy(dr.ifname, "lo0", sizeof dr.ifname); /* dummy interface */
	if (ioctl(s, SIOCGDRLST_IN6, &dr) < 0) {
		warnmsg(LOG_ERR, __func__, "ioctl(SIOCGDRLST_IN6): %s",
		    strerror(errno));
		goto closeandend;
	}

	for (i = 0; i < PRLSTSIZ && dr.defrouter[i].if_index; i++) {
		if (ifindex && dr.defrouter[i].if_index == ifindex) {
			/* sanity check */
			if (!IN6_IS_ADDR_LINKLOCAL(&dr.defrouter[i].rtaddr)) {
				warnmsg(LOG_ERR, __func__,
				    "default router list contains a "
				    "non-link-local address(%s)",
				    inet_ntop(AF_INET6,
				    &dr.defrouter[i].rtaddr,
				    ntopbuf, INET6_ADDRSTRLEN));
				continue; /* ignore the address */
			}
			sendprobe(&dr.defrouter[i].rtaddr, ifinfo);
		}
	}

closeandend:
	close(s);
}

static void
sendprobe(struct in6_addr *addr, struct ifinfo *ifinfo)
{
	char ntopbuf[INET6_ADDRSTRLEN], ifnamebuf[IFNAMSIZ];
	struct sockaddr_in6 sa6_probe;
	struct in6_pktinfo *pi;
	struct cmsghdr *cm;
	u_int32_t ifindex = ifinfo->sdl->sdl_index;
	int hoplimit = 1;

	memset(&sa6_probe, 0, sizeof(sa6_probe));
	sa6_probe.sin6_family = AF_INET6;
	sa6_probe.sin6_len = sizeof(sa6_probe);
	sa6_probe.sin6_addr = *addr;
	sa6_probe.sin6_scope_id = ifinfo->linkid;

	sndmhdr.msg_name = &sa6_probe;
	sndmhdr.msg_iov[0].iov_base = NULL;
	sndmhdr.msg_iov[0].iov_len = 0;

	cm = CMSG_FIRSTHDR(&sndmhdr);
	/* specify the outgoing interface */
	cm->cmsg_level = IPPROTO_IPV6;
	cm->cmsg_type = IPV6_PKTINFO;
	cm->cmsg_len = CMSG_LEN(sizeof(struct in6_pktinfo));
	pi = (struct in6_pktinfo *)CMSG_DATA(cm);
	memset(&pi->ipi6_addr, 0, sizeof(pi->ipi6_addr));	/*XXX*/
	pi->ipi6_ifindex = ifindex;

	/* specify the hop limit of the packet for safety */
	cm = CMSG_NXTHDR(&sndmhdr, cm);
	cm->cmsg_level = IPPROTO_IPV6;
	cm->cmsg_type = IPV6_HOPLIMIT;
	cm->cmsg_len = CMSG_LEN(sizeof(int));
	memcpy(CMSG_DATA(cm), &hoplimit, sizeof(int));

	warnmsg(LOG_DEBUG, __func__, "probe a router %s on %s",
	    inet_ntop(AF_INET6, addr, ntopbuf, INET6_ADDRSTRLEN),
	    if_indextoname(ifindex, ifnamebuf));

	if (sendmsg(probesock, &sndmhdr, 0))
		warnmsg(LOG_ERR, __func__, "sendmsg on %s: %s",
		    if_indextoname(ifindex, ifnamebuf), strerror(errno));
}

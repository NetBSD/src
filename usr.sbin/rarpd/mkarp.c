/*	$NetBSD: mkarp.c,v 1.6 2003/08/07 11:25:40 agc Exp $ */

/*
 * Copyright (c) 1984, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Sun Microsystems, Inc.
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
__COPYRIGHT("@(#) Copyright (c) 1984, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)arp.c	8.3 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: mkarp.c,v 1.6 2003/08/07 11:25:40 agc Exp $");
#endif
#endif /* not lint */

/*
 * mkarp - set an arp table entry
 */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_types.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <nlist.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mkarp.h"

/* Roundup the same way rt_xaddrs does */
#define ROUNDUP(a) \
       ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

int	rtmsg(int, int, struct rt_msghdr *, struct sockaddr_inarp *, 
	      struct sockaddr_dl *);
struct	{
	struct	rt_msghdr m_rtm;
	char	m_space[512];
}	m_rtmsg;

/*
 * Set an individual arp entry 
 */
int
mkarp(u_char *haddr, u_int32_t ipaddr)
{
	static struct sockaddr_inarp blank_sin = {sizeof(blank_sin), AF_INET };
	static struct sockaddr_dl blank_sdl = {sizeof(blank_sdl), AF_LINK };

	struct sockaddr_inarp *sin;
	struct sockaddr_dl *sdl;
	struct rt_msghdr *rtm;
	u_int8_t *p, *endp;
	int result;
	int s;

	struct sockaddr_inarp sin_m;
	struct sockaddr_dl sdl_m;

	sin = &sin_m;
	rtm = &(m_rtmsg.m_rtm);

	sdl_m = blank_sdl;		/* struct copy */
	sin_m = blank_sin;		/* struct copy */

	sin->sin_addr.s_addr = ipaddr;

	p = LLADDR(&sdl_m);
	endp = ((caddr_t)&sdl_m) + sdl_m.sdl_len;
	if (endp > (p + ETHER_ADDR_LEN))
		endp = p + ETHER_ADDR_LEN;

	while (p < endp) {
		*p++ = *haddr++;
	}
	sdl_m.sdl_alen = ETHER_ADDR_LEN;

	/*
	 * We need to close and open the socket to prevent routing updates
	 * building up such that when we send our message we never see our
	 * reply (and hang)
	 */
	s = socket(PF_ROUTE, SOCK_RAW, 0);
	if (s < 0)
		err(1, "socket");

	rtm->rtm_flags = 0;

	if (rtmsg(RTM_GET, s, rtm, &sin_m, &sdl_m) < 0) {
#if 0
		warn("%s", host);
#endif
		close(s);
		return (1);
	}
	sin = (struct sockaddr_inarp *)(rtm + 1);
	sdl = (struct sockaddr_dl *)(sin->sin_len + (char *)sin);
	if (sin->sin_addr.s_addr == sin_m.sin_addr.s_addr) {
		if (sdl->sdl_family == AF_LINK &&
		    (rtm->rtm_flags & RTF_LLINFO) &&
		    !(rtm->rtm_flags & RTF_GATEWAY)) switch (sdl->sdl_type) {
		case IFT_ETHER: case IFT_FDDI: case IFT_ISO88023:
		case IFT_ISO88024: case IFT_ISO88025: case IFT_ARCNET:
			goto overwrite;
		}
#if 0
		(void)printf("set: can only proxy for %s\n", host);
#endif
		close(s);
		return (1);
	}
overwrite:
	if (sdl->sdl_family != AF_LINK) {
#if 0
		(void)printf("cannot intuit interface index and type for %s\n",
		    host);
#endif
		close(s);
		return (1);
	}
	sdl_m.sdl_type = sdl->sdl_type;
	sdl_m.sdl_index = sdl->sdl_index;
	result = rtmsg(RTM_ADD, s, rtm, &sin_m, &sdl_m);
	close(s);
	return (result);
}

int
rtmsg(int cmd, int s, struct rt_msghdr *rtm, struct sockaddr_inarp *sin_m,
      struct sockaddr_dl *sdl_m)
{
	static int seq;
	int rlen;
	char *cp;
	int l;
	pid_t pid;
	struct timeval time;

	rtm = &m_rtmsg.m_rtm;
	cp = m_rtmsg.m_space;
	errno = 0;

	pid = getpid();

	(void)memset(&m_rtmsg, 0, sizeof(m_rtmsg));
	rtm->rtm_version = RTM_VERSION;

	switch (cmd) {
	default:
		errx(1, "internal wrong cmd");
		/*NOTREACHED*/
	case RTM_ADD:
		rtm->rtm_addrs |= RTA_GATEWAY;
		(void)gettimeofday(&time, 0);
		rtm->rtm_rmx.rmx_expire = time.tv_sec + 20 * 60;
		rtm->rtm_inits = RTV_EXPIRE;
		rtm->rtm_flags |= (RTF_HOST | RTF_STATIC);
		sin_m->sin_other = 0;

		/* FALLTHROUGH */
	case RTM_GET:
		rtm->rtm_addrs |= RTA_DST;
	}
#define NEXTADDR(w, s) \
	if (rtm->rtm_addrs & (w)) { \
		(void)memcpy(cp, s, ((struct sockaddr *)s)->sa_len); \
                cp += ROUNDUP(((struct sockaddr *)s)->sa_len);}

	NEXTADDR(RTA_DST, sin_m);
	NEXTADDR(RTA_GATEWAY, sdl_m);

	rtm->rtm_msglen = cp - (char *)&m_rtmsg;

	l = rtm->rtm_msglen;
	rtm->rtm_seq = ++seq;
	rtm->rtm_type = cmd;
	if ((rlen = write(s, (char *)&m_rtmsg, l)) < 0) {
		if (errno != ESRCH && errno != EEXIST) {
			warn("writing to routing socket");
			return (-1);
		}
	}
	do {
		l = read(s, (char *)&m_rtmsg, sizeof(m_rtmsg));
	} while (l > 0 && (rtm->rtm_seq != seq || rtm->rtm_pid != pid));
	if (l < 0)
		warn("read from routing socket");
	return (0);
}

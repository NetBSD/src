/*	$NetBSD: rumprouter.c,v 1.5 2010/07/04 17:24:10 pooka Exp $	*/

/*
 * Copyright (c) 2008 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Finnish Cultural Foundation.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <arpa/inet.h>

#include <netinet/in.h>
#include <net/route.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/sockio.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>

#undef DEBUG

#ifdef DEBUG
#define DP	if (1) printf
#else
#define DP	if (0) printf
#endif

static void
configure_interface(const char *ifname, const char *addr, const char *mask,
	const char *bcast)
{
	struct ifaliasreq ia;
	struct sockaddr_in *sin;
	int s, rv;

	DP("Entering %s\n", __FUNCTION__);

	DP("Create an interface(%s)\n", ifname);
	s = atoi(ifname + strlen(ifname) - 1);		/* XXX FIXME XXX */
	if ((s = rump_pub_virtif_create(s)) != 0) {
		err(1, "rump_pub_virtif_create(%d)", s);
	}

	DP("Get a socket for configuring the interface\n");
	if ((s = rump_sys_socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		err(1, "rump_sys_socket");
	}

	/* Address */
	memset(&ia, 0, sizeof(ia));
	strcpy(ia.ifra_name, ifname);
	sin = (struct sockaddr_in *)&ia.ifra_addr;
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(struct sockaddr_in);
	sin->sin_addr.s_addr = inet_addr(addr);

	/* Netmask */
	sin = (struct sockaddr_in *)&ia.ifra_mask;
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(struct sockaddr_in);
	sin->sin_addr.s_addr = inet_addr(mask);

	/* Broadcast address */
	sin = (struct sockaddr_in *)&ia.ifra_broadaddr;
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(struct sockaddr_in);
	sin->sin_addr.s_addr = inet_addr(bcast);

	DP("Set the addresses\n");
	rv = rump_sys_ioctl(s, SIOCAIFADDR, &ia);
	if (rv) {
		err(1, "SIOCAIFADDR");
	}
	rump_sys_close(s);
	DP("Done with %s\n", __FUNCTION__);
}

static void
configure_routing(const char *dst, const char *mask, const char *gw)
{
	size_t len;
	struct {
		struct rt_msghdr m_rtm;
		uint8_t m_space[512];
	} m_rtmsg;
#define rtm m_rtmsg.m_rtm
	uint8_t *bp = m_rtmsg.m_space;
	struct sockaddr_in sinstore;
	int s, rv;

	DP("Entering %s\n", __FUNCTION__);

	DP("Open a routing socket\n");
	s = rump_sys_socket(PF_ROUTE, SOCK_RAW, 0);
	if (s == -1) {
		err(1, "rump_sys_socket");
	}

	memset(&m_rtmsg, 0, sizeof(m_rtmsg));
	rtm.rtm_type = RTM_ADD;
	rtm.rtm_flags = RTF_UP | RTF_GATEWAY | RTF_STATIC;
	rtm.rtm_version = RTM_VERSION;
	rtm.rtm_seq = 2;
	rtm.rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;

	/* dst */
	memset(&sinstore, 0, sizeof(sinstore));
	sinstore.sin_family = AF_INET;
	sinstore.sin_len = sizeof(sinstore);
	sinstore.sin_addr.s_addr = inet_addr(dst);
	memcpy(bp, &sinstore, sizeof(sinstore));
	bp += sizeof(sinstore);

	/* gw */
	memset(&sinstore, 0, sizeof(sinstore));
	sinstore.sin_family = AF_INET;
	sinstore.sin_len = sizeof(sinstore);
	sinstore.sin_addr.s_addr = inet_addr(gw);
	memcpy(bp, &sinstore, sizeof(sinstore));
	bp += sizeof(sinstore);

	/* netmask */
	memset(&sinstore, 0, sizeof(sinstore));
	sinstore.sin_family = AF_INET;
	sinstore.sin_len = sizeof(sinstore);
	sinstore.sin_addr.s_addr = inet_addr(mask);
	memcpy(bp, &sinstore, sizeof(sinstore));
	bp += sizeof(sinstore);

	len = bp - (uint8_t *)&m_rtmsg;
	rtm.rtm_msglen = len;

	DP("Set the route\n");
	rv = rump_sys_write(s, &m_rtmsg, len);
	if (rv != (int)len) {
		err(1, "rump_sys_write");
	}
	rump_sys_close(s);
	DP("Done with %s\n", __FUNCTION__);
}

static void
usage(const char *argv0)
{
	printf("Usage: %s if1 if2 [route]\n", argv0);
	printf("\n");
	printf("where both \"if1\" and \"if2\" are\n");
	printf("\n");
	printf("ifname address netmask broadcast\n");
	printf("\n");
	printf("and \"route\" is an optional default route\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	if (argc < 9 || argc > 10) {
		usage(argv[0]);
	}

	rump_init();
	configure_interface(argv[1], argv[2], argv[3], argv[4]);
	configure_interface(argv[5], argv[6], argv[7], argv[8]);
	if (argc == 10) {
		configure_routing("192.168.3.0", "255.255.255.0", "192.168.1.2");
	}
	printf("Press Ctrl+C to quit...");
	pause();

	return 0;
}

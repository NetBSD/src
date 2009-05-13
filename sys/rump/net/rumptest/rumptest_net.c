/*	$NetBSD: rumptest_net.c,v 1.9.2.1 2009/05/13 17:23:02 jym Exp $	*/

/*
 * Copyright (c) 2008 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by then
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
#include <sys/time.h>
#include <sys/sockio.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEST_ADDR "204.152.190.12"	/* www.NetBSD.org */
#define DEST_PORT 80			/* take a wild guess */

#ifdef FULL_NETWORK_STACK
/*
 * If we are running with the full networking stack, configure
 * virtual interface.  For this to currently work, you *must* have
 * tap0 bridged with your main networking interface.  Essentially
 * the following steps are required:
 *
 * # ifconfig tap0 create
 * # ifconfig tap0 up
 * # ifconfig bridge0 create
 * # brconfig bridge0 add tap0 add yourrealif0
 * # brconfig bridge0 up
 *
 * The usability is likely to be improved later.
 */
#define MYADDR "10.181.181.11"
#define MYBCAST "10.181.181.255"
#define MYMASK "255.255.255.0"
#define MYGW "10.181.181.1"
#define IFNAME "virt0" /* XXX: hardcoded */

static void
configure_interface(void)
{
        struct sockaddr_in *sin;
	struct sockaddr_in sinstore;
	struct ifaliasreq ia;
	size_t len;
	struct {
		struct rt_msghdr m_rtm;
		uint8_t m_space;
	} m_rtmsg;
#define rtm m_rtmsg.m_rtm
	uint8_t *bp = &m_rtmsg.m_space;
	int s, rv;

	if ((rv = rump_virtif_create(0)) != 0) {
		printf("could not configure interface %d\n", rv);
		exit(1);
	}

	/* get a socket for configuring the interface */
	s = rump_sys_socket(PF_INET, SOCK_DGRAM, 0);
	if (s == -1)
		err(1, "configuration socket");

	/* fill out struct ifaliasreq */
	memset(&ia, 0, sizeof(ia));
	strcpy(ia.ifra_name, IFNAME);
	sin = (struct sockaddr_in *)&ia.ifra_addr;
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(struct sockaddr_in);
	sin->sin_addr.s_addr = inet_addr(MYADDR);

	sin = (struct sockaddr_in *)&ia.ifra_broadaddr;
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(struct sockaddr_in);
	sin->sin_addr.s_addr = inet_addr(MYBCAST);

	sin = (struct sockaddr_in *)&ia.ifra_mask;
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(struct sockaddr_in);
	sin->sin_addr.s_addr = inet_addr(MYMASK);

	/* toss to the configuration socket and see what it thinks */
	rv = rump_sys_ioctl(s, SIOCAIFADDR, &ia);
	if (rv)
		err(1, "SIOCAIFADDR");
	rump_sys_close(s);

	/* open routing socket and configure our default router */
	s = rump_sys_socket(PF_ROUTE, SOCK_RAW, 0);
	if (s == -1)
		err(1, "routing socket");

	/* create routing message */
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
        memcpy(bp, &sinstore, sizeof(sinstore));
        bp += sizeof(sinstore);

	/* gw */
        memset(&sinstore, 0, sizeof(sinstore));
        sinstore.sin_family = AF_INET;
        sinstore.sin_len = sizeof(sinstore);
        sinstore.sin_addr.s_addr = inet_addr(MYGW);
        memcpy(bp, &sinstore, sizeof(sinstore));
        bp += sizeof(sinstore);

	/* netmask */
        memset(&sinstore, 0, sizeof(sinstore));
        sinstore.sin_family = AF_INET;
        sinstore.sin_len = sizeof(sinstore);
        memcpy(bp, &sinstore, sizeof(sinstore));
        bp += sizeof(sinstore);

        len = bp - (uint8_t *)&m_rtmsg;
        rtm.rtm_msglen = len;

	/* stuff that to the routing socket and wait for happy days */
	if (rump_sys_write(s, &m_rtmsg, len) != len)
		err(1, "routing incomplete");
	rump_sys_close(s);
}
#endif /* FULL_NETWORK_STACK */

int
main(int argc, char *argv[])
{
	char buf[65536];
	struct sockaddr_in sin;
	struct timeval tv;
	ssize_t n;
	size_t off;
	int s;

	if (rump_init())
		errx(1, "rump_init failed");

#ifdef FULL_NETWORK_STACK
	configure_interface();
#endif

	s = rump_sys_socket(PF_INET, SOCK_STREAM, 0);
	if (s == -1)
		err(1, "can't open socket");

	tv.tv_sec = 5;
	tv.tv_usec = 0;
	if (rump_sys_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,
	    &tv, sizeof(tv)) == -1)
		err(1, "setsockopt");

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(DEST_PORT);
	sin.sin_addr.s_addr = inet_addr(DEST_ADDR);

	if (rump_sys_connect(s, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		err(1, "connect failed");
	}

	printf("connected\n");

	strcpy(buf, "GET / HTTP/1.0\n\n");
	n = rump_sys_write(s, buf, strlen(buf));
	if (n != strlen(buf))
		err(1, "wrote only %zd vs. %zu\n",
		    n, strlen(buf));
	
	memset(buf, 0, sizeof(buf));
	for (off = 0; off < sizeof(buf) && n > 0;) {
		n = rump_sys_read(s, buf+off, sizeof(buf)-off);
		if (n > 0)
			off += n;
	}
	printf("read %zd (max %zu):\n", off, sizeof(buf));
	printf("%s", buf);

	return 0;
}

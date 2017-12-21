/*	$NetBSD: t_pktinfo_send.c,v 1.2.2.2 2017/12/21 21:08:13 snj Exp $	*/

/*-
 * Copyright (c) 2017 Internet Initiative Japan Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: t_pktinfo_send.c,v 1.2.2.2 2017/12/21 21:08:13 snj Exp $");

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include "h_macros.h"
#include "../config/netconfig.c"

#include <atf-c.h>


#define SERVERPORT	54321
#define CLIENTPORT	12345


char message[128] = "Hello IP_PKTINFO";

static void
setup_test_environment(void)
{
	RZ(rump_init());
	netcfg_rump_if("lo0", "127.0.0.2", "255.0.0.0");
	netcfg_rump_if("lo0", "127.0.0.3", "255.0.0.0");
	netcfg_rump_if("lo0", "127.0.0.4", "255.0.0.0");
	netcfg_rump_if("lo0", "127.0.0.5", "255.0.0.0");
}

static void
sock_in_init(struct sockaddr_in *sin, const char *addr, in_port_t port)
{
	memset(sin, 0, sizeof(struct sockaddr_in));

	sin->sin_family = AF_INET;
	sin->sin_port = htons(port);
	inet_pton(AF_INET, addr, &sin->sin_addr);
}

static int
sock_bind(int sock, const char *addr, in_port_t port)
{
	struct sockaddr_in bindaddr;

	sock_in_init(&bindaddr, addr, port);
	return rump_sys_bind(sock,
	    (struct sockaddr *)&bindaddr, sizeof(bindaddr));
}

static int
udp_server(const char *addr, in_port_t port)
{
	int s, rv;

	RL(s = rump_sys_socket(AF_INET, SOCK_DGRAM, 0));
	RL(rv = sock_bind(s, addr, port));

	return s;
}

static int
addrcmp(struct sockaddr_in *sin, const char *addr)
{
	struct in_addr inaddr;
	inet_pton(AF_INET, addr, &inaddr);
	return memcmp(&inaddr, &sin->sin_addr, sizeof(inaddr));
}


static ssize_t
sendto_pktinfo(int s, const void *buf, size_t len, int flags,
               const char *src, const char *dst, in_port_t dstport)
{
	/* for sendmsg */
	struct sockaddr_in to;
	struct msghdr msg;
	char cmsgbuf[CMSG_SPACE(sizeof(struct in_pktinfo)) * 2];
	struct iovec vec;

	/* for store to cmsghdr */
	struct cmsghdr *cmsg;

	/* for pktinfo */
	struct in_pktinfo *pi;
	struct in_addr addr;

	/* setup msghdr */
	sock_in_init(&to, dst, dstport);

	vec.iov_base = __UNCONST(buf);
	vec.iov_len = len;
	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_name = (caddr_t)&to;
	msg.msg_namelen = sizeof(to);
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = 0;
	cmsg = (struct cmsghdr *)cmsgbuf;

	/* setup ip_pktinfo */
	msg.msg_controllen += CMSG_SPACE(sizeof(struct in_pktinfo));
	cmsg->cmsg_len = CMSG_LEN(sizeof(struct in_pktinfo));
	cmsg->cmsg_level = IPPROTO_IP;
	cmsg->cmsg_type = IP_PKTINFO;

	pi = (struct in_pktinfo *)CMSG_DATA(cmsg);
	memset(pi, 0, sizeof(*pi));

	/* treat 0.x.x.x/8 as interface index (like RFC1724 ss.3.3) */
	inet_pton(AF_INET, src, &addr);
	if (ntohl(addr.s_addr) >> 24 == 0)
		pi->ipi_ifindex = ntohl(addr.s_addr) & 0xffffff;
	else {
		pi->ipi_addr = addr;
	}

	if (msg.msg_controllen == 0)
		msg.msg_control = NULL;

	return rump_sys_sendmsg(s, &msg, flags);
}

static void
try_sendmsg_pktinfo(int client, int server, const char *data, size_t datalen,
                    const char *src, const char *dst, in_port_t dstport)
{
	struct sockaddr_in from;
	socklen_t fromlen;
	int rv;
	char buf[sizeof(message) * 2];

	RL(rv = sendto_pktinfo(client, data, datalen, 0,
	    src, dst, dstport));

	fromlen = sizeof(from);
	RL(rv = rump_sys_recvfrom(server, buf, sizeof(buf), 0,
	    (struct sockaddr *)&from, &fromlen));

	ATF_REQUIRE_MSG(addrcmp(&from, src) == 0,
	    "source address of received packet is %s, must be %s",
	    inet_ntoa(from.sin_addr), src);
}

static void
do_send_pktinfo_tests(int client, int server, const char *data, size_t datalen)
{
	struct sockaddr_in sa_before, sa_after;
	socklen_t sa_beforelen, sa_afterlen;
	int rv;
	char ipbuf1[sizeof("255.255.255.255")];
	char ipbuf2[sizeof("255.255.255.255")];

	/* get sockaddr before sendmsg w/IP_PKTINFO */
	sa_beforelen = sizeof(sa_before);
	RL(rv = rump_sys_getsockname(client,
	    (struct sockaddr *)&sa_before, &sa_beforelen));

	/*
	 * sendmsg with IP_PKTINFO: 127.0.0.[2345] -> 127.0.0.1:54321, and
	 * check received packet is from 127.0.0.[2345]
	 */
	try_sendmsg_pktinfo(client, server, data, datalen,
	    "127.0.0.2", "127.0.0.1", SERVERPORT);
	try_sendmsg_pktinfo(client, server, data, datalen,
	    "127.0.0.3", "127.0.0.1", SERVERPORT);
	try_sendmsg_pktinfo(client, server, data, datalen,
	    "127.0.0.4", "127.0.0.1", SERVERPORT);
	try_sendmsg_pktinfo(client, server, data, datalen,
	    "127.0.0.5", "127.0.0.1", SERVERPORT);

	/* get sockaddr after sendmsg w/IP_PKTINFO */
	sa_afterlen = sizeof(sa_after);
	RL(rv = rump_sys_getsockname(client,
	    (struct sockaddr *)&sa_after, &sa_afterlen));

	/* confirm sockaddr is not changed */
	inet_ntop(AF_INET, &sa_before.sin_addr, ipbuf1, sizeof(ipbuf1));
	inet_ntop(AF_INET, &sa_after.sin_addr, ipbuf2, sizeof(ipbuf2));
	ATF_REQUIRE_MSG(sa_before.sin_addr.s_addr == sa_after.sin_addr.s_addr,
	    "sockaddr is different from before send. before=%s, after=%s",
	    ipbuf1, ipbuf2);
}

ATF_TC(pktinfo_send_unbound);
ATF_TC_HEAD(pktinfo_send_unbound, tc)
{
	atf_tc_set_md_var(tc, "descr", "sendmsg with IP_PKTINFO");
	atf_tc_set_md_var(tc, "timeout", "5");
}
ATF_TC_BODY(pktinfo_send_unbound, tc)
{
	int client, server;

	setup_test_environment();

	server = udp_server("127.0.0.1", SERVERPORT);
	RL(client = rump_sys_socket(AF_INET, SOCK_DGRAM, 0));

	/* do sendmsg w/IP_PKTINFO tests */
	do_send_pktinfo_tests(client, server, message, strlen(message));

	rump_sys_close(client);
	rump_sys_close(server);
}


ATF_TC(pktinfo_send_bindany);
ATF_TC_HEAD(pktinfo_send_bindany, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "sendmsg with IP_PKTINFO on bind(INADDR_ANY) socket");
	atf_tc_set_md_var(tc, "timeout", "5");
}
ATF_TC_BODY(pktinfo_send_bindany, tc)
{
	int client, server, rv;

	setup_test_environment();

	server = udp_server("127.0.0.1", SERVERPORT);
	RL(client = rump_sys_socket(AF_INET, SOCK_DGRAM, 0));
	RL(rv = sock_bind(client, "0.0.0.0", 0));

	/* do sendmsg w/IP_PKTINFO tests */
	do_send_pktinfo_tests(client, server, message, strlen(message));

	rump_sys_close(client);
	rump_sys_close(server);
}


ATF_TC(pktinfo_send_bindaddr);
ATF_TC_HEAD(pktinfo_send_bindaddr, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "sendmsg with IP_PKTINFO on bind(addr:0) socket");
	atf_tc_set_md_var(tc, "timeout", "5");
}
ATF_TC_BODY(pktinfo_send_bindaddr, tc)
{
	int client, server, rv;

	setup_test_environment();

	server = udp_server("127.0.0.1", SERVERPORT);
	RL(client = rump_sys_socket(AF_INET, SOCK_DGRAM, 0));
	RL(rv = sock_bind(client, "127.0.0.1", 0));

	/* do sendmsg w/IP_PKTINFO tests */
	do_send_pktinfo_tests(client, server, message, strlen(message));

	rump_sys_close(client);
	rump_sys_close(server);
}


ATF_TC(pktinfo_send_bindport);
ATF_TC_HEAD(pktinfo_send_bindport, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "sendmsg with IP_PKTINFO on bind(INADDR_ANY:12345) socket");
	atf_tc_set_md_var(tc, "timeout", "5");
}
ATF_TC_BODY(pktinfo_send_bindport, tc)
{
	int client, server, rv;

	setup_test_environment();

	server = udp_server("127.0.0.1", SERVERPORT);
	RL(client = rump_sys_socket(AF_INET, SOCK_DGRAM, 0));
	RL(rv = sock_bind(client, "0.0.0.0", CLIENTPORT));

	/* do sendmsg w/IP_PKTINFO tests */
	do_send_pktinfo_tests(client, server, message, strlen(message));

	rump_sys_close(client);
	rump_sys_close(server);
}


ATF_TC(pktinfo_send_bindaddrport);
ATF_TC_HEAD(pktinfo_send_bindaddrport, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "sendmsg with IP_PKTINFO on bind(addr:12345) socket");
	atf_tc_set_md_var(tc, "timeout", "5");
}
ATF_TC_BODY(pktinfo_send_bindaddrport, tc)
{
	int client, server, rv;

	setup_test_environment();

	server = udp_server("127.0.0.1", SERVERPORT);
	RL(client = rump_sys_socket(AF_INET, SOCK_DGRAM, 0));
	RL(rv = sock_bind(client, "127.0.0.2", CLIENTPORT));

	/* do sendmsg w/IP_PKTINFO tests */
	do_send_pktinfo_tests(client, server, message, strlen(message));

	rump_sys_close(client);
	rump_sys_close(server);
}


ATF_TC(pktinfo_send_bindother);
ATF_TC_HEAD(pktinfo_send_bindother, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "sendmsg with IP_PKTINFO from address bound by other socket");
	atf_tc_set_md_var(tc, "timeout", "5");
}
ATF_TC_BODY(pktinfo_send_bindother, tc)
{
	int client, server, other, rv;

	setup_test_environment();

	server = udp_server("127.0.0.1", SERVERPORT);
	other = udp_server("127.0.0.2", CLIENTPORT);
	RL(client = rump_sys_socket(AF_INET, SOCK_DGRAM, 0));
	RL(rv = sock_bind(client, "127.0.0.3", CLIENTPORT));

	/* do sendmsg w/IP_PKTINFO tests */
	do_send_pktinfo_tests(client, server, message, strlen(message));

	rump_sys_close(client);
	rump_sys_close(server);
	rump_sys_close(other);
}


ATF_TC(pktinfo_send_connected);
ATF_TC_HEAD(pktinfo_send_connected, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "sendmsg with IP_PKTINFO on connected socket");
	atf_tc_set_md_var(tc, "timeout", "5");
}
ATF_TC_BODY(pktinfo_send_connected, tc)
{
	struct sockaddr_in connectaddr;
	int client, server, rv;

	setup_test_environment();

	server = udp_server("127.0.0.1", SERVERPORT);
	RL(client = rump_sys_socket(AF_INET, SOCK_DGRAM, 0));

	sock_in_init(&connectaddr, "127.0.0.1", SERVERPORT);
	RL(rv = rump_sys_connect(client,
	    (struct sockaddr *)&connectaddr, sizeof(connectaddr)));

	/* sendmsg w/IP_PKTINFO on connected socket should be error */
	rv = sendto_pktinfo(client, message, strlen(message), 0,
	    "127.0.0.2", "127.0.0.1", SERVERPORT);

	ATF_REQUIRE_MSG(rv == -1,
	    "sendmsg w/IP_PKTINFO on connected socket should be error,"
	    " but success");
	ATF_REQUIRE_MSG(errno == EISCONN,
	    "sendmsg with in-use address:port should be EISCONN,"
	    " but got %s", strerror(errno));


	rump_sys_close(client);
	rump_sys_close(server);
}


ATF_TC(pktinfo_send_notown);
ATF_TC_HEAD(pktinfo_send_notown, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "sendmsg with IP_PKTINFO from no-own address");
	atf_tc_set_md_var(tc, "timeout", "5");
}
ATF_TC_BODY(pktinfo_send_notown, tc)
{
	int client, server, rv;

	setup_test_environment();

	server = udp_server("127.0.0.1", SERVERPORT);
	RL(client = rump_sys_socket(AF_INET, SOCK_DGRAM, 0));

	/* sendmsg w/IP_PKTINFO from 127.0.0.100 (not own) should be error */
	rv = sendto_pktinfo(client, message, strlen(message), 0,
	    "127.0.0.100", "127.0.0.1", SERVERPORT);

	ATF_REQUIRE_MSG(rv == -1,
	    "sendmsg w/IP_PKTINFO from unavailable address"
	    " should be error, but success");
	ATF_REQUIRE_MSG(errno == EADDRNOTAVAIL,
	    "sendmsg with in-use address:port"
	    " should be EADDRNOTAVAIL, but got %s", strerror(errno));


	rump_sys_close(client);
	rump_sys_close(server);
}


ATF_TC(pktinfo_send_notown_bind);
ATF_TC_HEAD(pktinfo_send_notown_bind, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "sendmsg with IP_PKTINFO from no-own address on bind socket");
	atf_tc_set_md_var(tc, "timeout", "5");
}
ATF_TC_BODY(pktinfo_send_notown_bind, tc)
{
	int client, server, rv;

	setup_test_environment();

	server = udp_server("127.0.0.1", SERVERPORT);
	RL(client = rump_sys_socket(AF_INET, SOCK_DGRAM, 0));
	RL(rv = sock_bind(client, "127.0.0.2", CLIENTPORT));

	/* sendmsg w/IP_PKTINFO from 127.0.0.100 (not own) should be error */
	rv = sendto_pktinfo(client, message, strlen(message), 0,
	    "127.0.0.100", "127.0.0.1", SERVERPORT);

	ATF_REQUIRE_MSG(rv == -1,
	    "sendmsg w/IP_PKTINFO from unavailable address should be error,"
	    " but success");
	ATF_REQUIRE_MSG(errno == EADDRNOTAVAIL,
	    "sendmsg with in-use address:port should be EADDRNOTAVAIL,"
	    " but got %s", strerror(errno));


	rump_sys_close(client);
	rump_sys_close(server);
}


static u_int16_t
in_cksum(uint16_t *p, int len)
{
	u_int32_t sum;

	for (sum = 0; len >= 2; len -= 2)
		sum += *p++;
	if (len == 1)
		sum += ntohs(*(uint8_t *)p * 256);
	sum = (sum >> 16) + (sum & 0xffff);
	sum = (sum >> 16) + (sum & 0xffff);
	return ~sum;
}

ATF_TC(pktinfo_send_rawip);
ATF_TC_HEAD(pktinfo_send_rawip, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "sendmsg raw-ip with IP_PKTINFO");
	atf_tc_set_md_var(tc, "timeout", "5");
}
ATF_TC_BODY(pktinfo_send_rawip, tc)
{
	struct icmp icmp;
	size_t icmplen;
	int client, server;

	setup_test_environment();

	RL(server = rump_sys_socket(AF_INET, SOCK_RAW, IPPROTO_ICMP));
	RL(client = rump_sys_socket(AF_INET, SOCK_RAW, IPPROTO_ICMP));

	memset(&icmp, 0, sizeof(icmp));
	icmp.icmp_type = ICMP_ECHOREPLY; /* against confuse REQ with REPLY */
	icmp.icmp_id = htons(getpid());
	icmp.icmp_cksum = in_cksum((uint16_t *)&icmp, sizeof(icmp));
	icmplen = sizeof(icmp);

	/* sendmsg w/IP_PKTINFO from 127.0.0.2 */
	try_sendmsg_pktinfo(client, server, (const char *)&icmp, icmplen,
	    "127.0.0.2", "127.0.0.1", 0);

	rump_sys_close(client);
	rump_sys_close(server);
}


ATF_TC(pktinfo_send_rawip_notown);
ATF_TC_HEAD(pktinfo_send_rawip_notown, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "sendmsg raw-ip with IP_PKTINFO from no-own address");
	atf_tc_set_md_var(tc, "timeout", "5");
}
ATF_TC_BODY(pktinfo_send_rawip_notown, tc)
{
	struct icmp icmp;
	size_t icmplen;
	int client, rv;

	setup_test_environment();

	RL(client = rump_sys_socket(AF_INET, SOCK_RAW, IPPROTO_ICMP));

	memset(&icmp, 0, sizeof(icmp));
	icmp.icmp_type = ICMP_ECHOREPLY; /* against confuse REQ with REPLY */
	icmp.icmp_id = htons(getpid());
	icmp.icmp_cksum = in_cksum((uint16_t *)&icmp, sizeof(icmp));
	icmplen = sizeof(icmp);

	/* sendmsg w/IP_PKTINFO from 127.0.0.100 (not own) should be error */
	rv = sendto_pktinfo(client, (const char *)&icmp, icmplen, 0,
	    "127.0.0.100", "127.0.0.1", 0);

	ATF_REQUIRE_MSG(rv == -1,
	    "sendmsg w/IP_PKTINFO from unavailable address"
	    " should be error, but success");
	ATF_REQUIRE_MSG(errno == EADDRNOTAVAIL,
	    "sendmsg with in-use address:port"
	    " should be EADDRNOTAVAIL, but got %s", strerror(errno));

	rump_sys_close(client);
}


ATF_TC(pktinfo_send_invalidarg);
ATF_TC_HEAD(pktinfo_send_invalidarg, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "sendmsg IP_PKTINFO with invalid argument");
	atf_tc_set_md_var(tc, "timeout", "5");
}
ATF_TC_BODY(pktinfo_send_invalidarg, tc)
{
	int client, rv;

	setup_test_environment();

	RL(client = rump_sys_socket(AF_INET, SOCK_DGRAM, 0));

	/* sendmsg w/IP_PKTINFO (ipi_ifindex=0, ipi_addr=0) does nothing */
	rv = sendto_pktinfo(client, message, strlen(message), 0,
	    "0.0.0.0", "127.0.0.1", SERVERPORT);

	ATF_REQUIRE_MSG(rv != -1,
	    "sendmsg w/IP_PKTINFO ipi_ifindex=0, ipi_addr=0"
	    " does nothing (no error), but error %s", strerror(errno));


	/* sendmsg w/IP_PKTINFO from 0.0.0.99 (ifindex=99) should be error */
	rv = sendto_pktinfo(client, message, strlen(message), 0,
	    "0.0.0.99", "127.0.0.1", SERVERPORT);

	ATF_REQUIRE_MSG(rv == -1,
	    "sendmsg w/IP_PKTINFO from unavailable ifindex"
	    " should be error, but success");
	ATF_REQUIRE_MSG(errno == EADDRNOTAVAIL,
	    "sendmsg with in-use address:port"
	    " should be EADDRNOTAVAIL, but got %s", strerror(errno));

	rump_sys_close(client);
}


ATF_TC(pktinfo_send_ifindex);
ATF_TC_HEAD(pktinfo_send_ifindex, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "sendmsg with IP_PKTINFO to specified interface");
	atf_tc_set_md_var(tc, "timeout", "5");
}
ATF_TC_BODY(pktinfo_send_ifindex, tc)
{
	pid_t child;
	int channel[2], i;
	char ifname2[IFNAMSIZ];
	char ifname3[IFNAMSIZ];
	char ethername[MAXPATHLEN];
	char token;

	snprintf(ethername, sizeof(ethername),
	    "t_pktinfo_send.link.%u", getpid());

	RL(pipe(channel));

	child = fork();

	RZ(rump_init());	/* XXX: lo0 is ifindex 1 */
	netcfg_rump_makeshmif(ethername, ifname2);	/* XXX: ifindex=2 */
	netcfg_rump_makeshmif(ethername, ifname3);	/* XXX: ifindex=3 */

	switch (child) {
	case -1:
		atf_tc_fail_errno("fork failed");
	case 0:
		{
			int client, rv;

			netcfg_rump_if(ifname2, "192.168.2.1", "255.255.255.0");
			netcfg_rump_if(ifname3, "192.168.0.1", "255.255.0.0");

			RL(client = rump_sys_socket(AF_INET, SOCK_DGRAM, 0));

			/* wait to ready */
			close(channel[1]);
			ATF_CHECK(read(channel[0], &token, 1) == 1 &&
			   token == 'U');
			close(channel[0]);

			/* few packets would be discarded while resolving arp */
			for (i = 0; i < 3; i++) {
				/* send from ifindex 3 = 192.168.0.1 */
				snprintf(message, sizeof(message), "Hello PKTINFO %d", i);

				RL(rv = sendto_pktinfo(client, message, strlen(message),
				    0, "0.0.0.3", "192.168.2.2", SERVERPORT));
			}

			rump_sys_close(client);
			pause();
		}
		break;
	default:
		{
			struct sockaddr_in from;
			socklen_t fromlen;
			int server, rv;
			char buf[sizeof(message)];

			netcfg_rump_if(ifname2, "192.168.2.2", "255.255.255.0");
			netcfg_rump_if(ifname3, "192.168.0.2", "255.255.255.0");

			server = udp_server("0.0.0.0", SERVERPORT);

			/* notify to child */
			close(channel[0]);
			ATF_CHECK(write(channel[1], "U", 1) == 1);
			close(channel[1]);

			memset(buf, 0, sizeof(buf));
			fromlen = sizeof(from);
			RL(rv = rump_sys_recvfrom(server, buf, sizeof(buf), 0,
			    (struct sockaddr *)&from, &fromlen));
			printf("%s: received \"%s\" from %s:%u\n", __func__,
			    buf,
			    inet_ntoa(from.sin_addr), ntohs(from.sin_port));

			ATF_REQUIRE_MSG(addrcmp(&from, "192.168.0.1") == 0,
			    "source address of received packet is %s,"
			    " must be %s",
			    inet_ntoa(from.sin_addr), "192.168.0.1");

			rump_sys_close(server);
			kill(child, SIGKILL);
		}
		break;
	}
}


ATF_TC(pktinfo_send_multicast);
ATF_TC_HEAD(pktinfo_send_multicast, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "sendmsg with IP_PKTINFO to multicast address"
	    " and specified interface");
	atf_tc_set_md_var(tc, "timeout", "5");
}
ATF_TC_BODY(pktinfo_send_multicast, tc)
{
	pid_t child;
	int channel[2];
	char ifname2[IFNAMSIZ];
	char ifname3[IFNAMSIZ];
	char ethername[MAXPATHLEN];
	char token;

	snprintf(ethername, sizeof(ethername),
	   "t_pktinfo_send.link.%u", getpid());

	RL(pipe(channel));

	child = fork();

	RZ(rump_init());	/* XXX: lo0 is ifindex 1 */
	netcfg_rump_makeshmif(ethername, ifname2);	/* XXX: ifindex=2 */
	netcfg_rump_makeshmif(ethername, ifname3);	/* XXX: ifindex=3 */

	switch (child) {
	case -1:
		atf_tc_fail_errno("fork failed");
	case 0:
		{
			int client, rv;

			netcfg_rump_if(ifname2, "192.168.2.1", "255.255.255.0");
			netcfg_rump_if(ifname3, "192.168.0.1", "255.255.0.0");

			RL(client = rump_sys_socket(AF_INET, SOCK_DGRAM, 0));

			/* wait to ready */
			close(channel[1]);
			ATF_CHECK(read(channel[0], &token, 1) == 1 &&
			   token == 'U');
			close(channel[0]);

			/* send from ifindex 2 = 192.168.2.1 */
			RL(rv = sendto_pktinfo(client, message, strlen(message),
			    0, "0.0.0.2", "224.0.0.1", SERVERPORT));

			/* send from ifindex 3 = 192.168.0.1 */
			RL(rv = sendto_pktinfo(client, message, strlen(message),
			    0, "0.0.0.3", "224.0.0.1", SERVERPORT));

			rump_sys_close(client);
			pause();
		}
		break;
	default:
		{
			struct sockaddr_in from;
			socklen_t fromlen;
			int server, i, rv;
			char buf[sizeof(message) * 2];

			netcfg_rump_if(ifname2, "192.168.2.2", "255.255.255.0");
			netcfg_rump_if(ifname3, "192.168.0.2", "255.255.255.0");

			server = udp_server("0.0.0.0", SERVERPORT);

			/* notify to child */
			close(channel[0]);
			ATF_CHECK(write(channel[1], "U", 1) == 1);
			close(channel[1]);

			for (i = 0; i < 2; i++) {
				fromlen = sizeof(from);
				RL(rv = rump_sys_recvfrom(server, buf,
				    sizeof(buf), 0,
				    (struct sockaddr *)&from, &fromlen));
				printf("%s: received from %s:%u\n", __func__,
				    inet_ntoa(from.sin_addr),
				    ntohs(from.sin_port));
			}

			rump_sys_close(server);
			kill(child, SIGKILL);
		}
		break;
	}
}


ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, pktinfo_send_unbound);
	ATF_TP_ADD_TC(tp, pktinfo_send_bindany);
	ATF_TP_ADD_TC(tp, pktinfo_send_bindaddr);
	ATF_TP_ADD_TC(tp, pktinfo_send_bindport);
	ATF_TP_ADD_TC(tp, pktinfo_send_bindaddrport);
	ATF_TP_ADD_TC(tp, pktinfo_send_bindother);
	ATF_TP_ADD_TC(tp, pktinfo_send_connected);
	ATF_TP_ADD_TC(tp, pktinfo_send_notown);
	ATF_TP_ADD_TC(tp, pktinfo_send_notown_bind);
	ATF_TP_ADD_TC(tp, pktinfo_send_rawip);
	ATF_TP_ADD_TC(tp, pktinfo_send_rawip_notown);
	ATF_TP_ADD_TC(tp, pktinfo_send_invalidarg);
	ATF_TP_ADD_TC(tp, pktinfo_send_ifindex);
	ATF_TP_ADD_TC(tp, pktinfo_send_multicast);

	return atf_no_error();
}

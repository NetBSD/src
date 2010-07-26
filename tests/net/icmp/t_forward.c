/*	$NetBSD: t_forward.c,v 1.4 2010/07/26 14:07:04 pooka Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: t_forward.c,v 1.4 2010/07/26 14:07:04 pooka Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/sysctl.h>
#include <sys/wait.h>

#include <arpa/inet.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <net/route.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <atf-c.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../h_macros.h"
#include "../config/netconfig.c"

static void
configure_interface(const char *busname, const char *addr, const char *mask,
	const char *bcast)
{
	char ifname[32];
	struct ifaliasreq ia;
	struct sockaddr_in *sin;
	int s, rv, ifnum;

	if ((s = rump_pub_shmif_create(busname, &ifnum)) != 0) {
		atf_tc_fail("rump_pub_shmif_create(%d)", s);
	}

	if ((s = rump_sys_socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		atf_tc_fail_errno("if config socket");
	}
	sprintf(ifname, "shmif%d", ifnum);

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

	rv = rump_sys_ioctl(s, SIOCAIFADDR, &ia);
	if (rv) {
		atf_tc_fail_errno("SIOCAIFADDR");
	}
	rump_sys_close(s);
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

	s = rump_sys_socket(PF_ROUTE, SOCK_RAW, 0);
	if (s == -1) {
		atf_tc_fail_errno("routing socket");
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

	rv = rump_sys_write(s, &m_rtmsg, len);
	if (rv != (int)len) {
		atf_tc_fail_errno("write routing message");
	}
	rump_sys_close(s);
}

/*
 * Since our maxttl is in our private namespace, we don't need raw packet
 * construction like traceroute(8) -- we can just use the global maxttl.
 */
static void
sendttl(void)
{
	extern int rumpns_ip_defttl;
	struct sockaddr_in sin;
	char payload[1024];
	char ifname[IFNAMSIZ];
	int mib[4] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_DEFTTL };
	int nv;
	int s;

	netcfg_rump_makeshmif("bus1", ifname);
	netcfg_rump_if(ifname, "1.0.0.1", "255.255.255.0");
	netcfg_rump_route("0.0.0.0", "0.0.0.0", "1.0.0.2"); /* default router */

	/* set global ttl to 1 */
	nv = 1;
	if (rump_sys___sysctl(mib, 4, NULL, NULL, &nv, sizeof(nv)) == -1)
		atf_tc_fail_errno("set ttl");

	s = rump_sys_socket(PF_INET, SOCK_DGRAM, 0);
	if (s == -1)
		atf_tc_fail_errno("create send socket");

	memset(&sin, 0, sizeof(sin));
	sin.sin_len = sizeof(sin);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(33434);
	sin.sin_addr.s_addr = inet_addr("9.9.9.9");

	/* send udp datagram with ttl == 1 */
	if (rump_sys_sendto(s, payload, sizeof(payload), 0,
	    (struct sockaddr *)&sin, sizeof(sin)) == -1)
		atf_tc_fail_errno("sendto");
}

static void
router(void)
{
	int mib[4] = { CTL_NET, PF_INET, IPPROTO_ICMP,
		    ICMPCTL_RETURNDATABYTES };
	int nv;

	/* set returndatabytes to 200 */
	nv = 200;
	if (rump_sys___sysctl(mib, 4, NULL, NULL, &nv, sizeof(nv)) == -1)
		atf_tc_fail_errno("sysctl returndatabytes");

	configure_interface("bus1", "1.0.0.2", "255.255.255.0", "1.0.0.255");

	/*
	 * Wait for parent to send us the data and for us to have
	 * a chance to process it.
	 */
	sleep(1);
	exit(0);
}

ATF_TC(returndatabytes);
ATF_TC_HEAD(returndatabytes, tc)
{

	atf_tc_set_md_var(tc, "descr", "icmp.returndatabytes with certain "
	    "packets can cause kernel panic");
	atf_tc_set_md_var(tc, "use.fs", "true");
	atf_tc_set_md_var(tc, "timeout", "4"); /* just in case */
	/* PR kern/43548 */
}

ATF_TC_BODY(returndatabytes, tc)
{
	pid_t cpid;
	int status;

	cpid = fork();
	rump_init();

	switch (cpid) {
	case -1:
		atf_tc_fail_errno("fork failed");
	case 0:
		router();
		break;
	default:
		sendttl();
		if (wait(&status) == -1)
			atf_tc_fail_errno("wait");
		if (WIFEXITED(status)) {
			if (WEXITSTATUS(status))
				atf_tc_fail("child exited with status %d",
				    WEXITSTATUS(status));
		} else {
			atf_tc_fail("child died");
		}
	}
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, returndatabytes);

	return atf_no_error();
}

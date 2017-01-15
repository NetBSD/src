/*	$NetBSD: t_can.c,v 1.1.2.1 2017/01/15 20:29:01 bouyer Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Manuel Bouyer
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
__RCSID("$NetBSD: t_can.c,v 1.1.2.1 2017/01/15 20:29:01 bouyer Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <sys/wait.h>
#include <sys/sockio.h>

#include <atf-c.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <net/if.h>
#include <netcan/can.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include "h_macros.h"

static void
cancfg_rump_createif(const char *ifname)
{
	int s, rv;
	struct ifreq ifr;

	s = -1;
	if ((s = rump_sys_socket(AF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		atf_tc_fail_errno("if config socket");
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if ((rv = rump_sys_ioctl(s, SIOCIFCREATE, &ifr)) < 0) {
		atf_tc_fail_errno("if config create");
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if ((rv = rump_sys_ioctl(s, SIOCGIFFLAGS, &ifr)) < 0) {
		atf_tc_fail_errno("if config get flags");
	}

	ifr.ifr_flags |= IFF_UP;
	if ((rv = rump_sys_ioctl(s, SIOCSIFFLAGS, &ifr)) < 0) {
		atf_tc_fail_errno("if config set flags");
	}
}

ATF_TC(canlocreate);
ATF_TC_HEAD(canlocreate, tc)
{

	atf_tc_set_md_var(tc, "descr", "check CAN loopback create/destroy");
	atf_tc_set_md_var(tc, "timeout", "5");
}

ATF_TC_BODY(canlocreate, tc)
{
	const char ifname[] = "canlo0";
	int s, rv;
	struct ifreq ifr;

	rump_init();
	cancfg_rump_createif(ifname);

	s = -1;
	if ((s = rump_sys_socket(AF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		atf_tc_fail_errno("if config socket(2)");
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if ((rv = rump_sys_ioctl(s, SIOCIFDESTROY, &ifr)) < 0) {
		atf_tc_fail_errno("if config destroy");
	}
}

ATF_TC(canwritelo);
ATF_TC_HEAD(canwritelo, tc)
{

	atf_tc_set_md_var(tc, "descr", "check that CAN sockets gets its own message via write");
	atf_tc_set_md_var(tc, "timeout", "5");
}

ATF_TC_BODY(canwritelo, tc)
{
	const char ifname[] = "canlo0";
	int s, rv;
	struct sockaddr_can sa;
	struct ifreq ifr;
	struct can_frame cf_send, cf_receive;

	rump_init();
	cancfg_rump_createif(ifname);

	s = -1;
	if ((s = rump_sys_socket(AF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		atf_tc_fail_errno("CAN socket");
	}

	strcpy(ifr.ifr_name, ifname );
	if ((rv = rump_sys_ioctl(s, SIOCGIFINDEX, &ifr)) < 0) {
		atf_tc_fail_errno("SIOCGIFINDEX");
	}
	ATF_CHECK_MSG(ifr.ifr_ifindex > 0, "%s index is %d (not > 0)",
	    ifname, ifr.ifr_ifindex);

	sa.can_family = AF_CAN;
	sa.can_ifindex = ifr.ifr_ifindex;

	if ((rv = rump_sys_bind(s, (struct sockaddr *)&sa, sizeof(sa))) < 0) {
		atf_tc_fail_errno("bind");
	}

	/*
	 * send a single byte message, but make sure remaining payload is
	 * not 0.
	 */

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = 1;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;
	cf_send.data[1] = 0xad;
	cf_send.data[2] = 0xbe;
	cf_send.data[3] = 0xef;

	if (rump_sys_write(s, &cf_send, sizeof(cf_send) - 7) < 0) {
		atf_tc_fail_errno("write");
	}

	memset(&cf_receive, 0, sizeof(cf_receive));
	if (( rv = rump_sys_read(s, &cf_receive, sizeof(cf_receive))) < 0) {
		atf_tc_fail_errno("read");
	}

	ATF_CHECK_MSG(rv > 0, "short read on socket");

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = 1;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;
	/* other data[] are expected to be 0 */

	ATF_CHECK_MSG(memcmp(&cf_send, &cf_receive, sizeof(cf_send)) == 0,
	    "received packet is not what we sent");
}

ATF_TC(canwriteunbound);
ATF_TC_HEAD(canwriteunbound, tc)
{

	atf_tc_set_md_var(tc, "descr", "check that write to unbound CAN sockets fails");
	atf_tc_set_md_var(tc, "timeout", "5");
}

ATF_TC_BODY(canwriteunbound, tc)
{
	const char ifname[] = "canlo0";
	int s, rv;
	struct sockaddr_can sa;
	struct can_frame cf_send;

	rump_init();
	cancfg_rump_createif(ifname);

	s = -1;
	if ((s = rump_sys_socket(AF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		atf_tc_fail_errno("CAN socket");
	}

	/*
	 * send a single byte message.
	 * not 0.
	 */

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = 1;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;

	rv = rump_sys_write(s, &cf_send, sizeof(cf_send) - 7);
	ATF_CHECK_MSG(rv < 0 && errno == EDESTADDRREQ, 
	    "write to unbound socket didn't fail");
}

ATF_TC(cansendtolo);
ATF_TC_HEAD(cansendtolo, tc)
{

	atf_tc_set_md_var(tc, "descr", "check that CAN sockets gets its own message via sendto");
	atf_tc_set_md_var(tc, "timeout", "5");
}

ATF_TC_BODY(cansendtolo, tc)
{
	const char ifname[] = "canlo0";
	int s, rv;
	struct sockaddr_can sa;
	struct ifreq ifr;
	struct can_frame cf_send, cf_receive;

	rump_init();
	cancfg_rump_createif(ifname);

	s = -1;
	if ((s = rump_sys_socket(AF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		atf_tc_fail_errno("CAN socket");
	}

	strcpy(ifr.ifr_name, ifname );
	if ((rv = rump_sys_ioctl(s, SIOCGIFINDEX, &ifr)) < 0) {
		atf_tc_fail_errno("SIOCGIFINDEX");
	}
	ATF_CHECK_MSG(ifr.ifr_ifindex > 0, "%s index is %d (not > 0)",
	    ifname, ifr.ifr_ifindex);

	sa.can_family = AF_CAN;
	sa.can_ifindex = ifr.ifr_ifindex;

	/*
	 * send a single byte message, but make sure remaining payload is
	 * not 0.
	 */

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = 1;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;
	cf_send.data[1] = 0xad;
	cf_send.data[2] = 0xbe;
	cf_send.data[3] = 0xef;

	if (rump_sys_sendto(s, &cf_send, sizeof(cf_send) - 7,
	    0, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		atf_tc_fail_errno("sendto");
	}

	memset(&cf_receive, 0, sizeof(cf_receive));
	if (( rv = rump_sys_read(s, &cf_receive, sizeof(cf_receive))) < 0) {
		atf_tc_fail_errno("read");
	}

	ATF_CHECK_MSG(rv > 0, "short read on socket");

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = 1;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;
	/* other data[] are expected to be 0 */

	ATF_CHECK_MSG(memcmp(&cf_send, &cf_receive, sizeof(cf_send)) == 0,
	    "received packet is not what we sent");
}

ATF_TC(cansendtowrite);
ATF_TC_HEAD(cansendtowrite, tc)
{

	atf_tc_set_md_var(tc, "descr", "check that write after sendto on unbound socket fails");
	atf_tc_set_md_var(tc, "timeout", "5");
}

ATF_TC_BODY(cansendtowrite, tc)
{
	const char ifname[] = "canlo0";
	int s, rv;
	struct sockaddr_can sa;
	struct ifreq ifr;
	struct can_frame cf_send, cf_receive;

	rump_init();
	cancfg_rump_createif(ifname);

	s = -1;
	if ((s = rump_sys_socket(AF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		atf_tc_fail_errno("CAN socket");
	}

	strcpy(ifr.ifr_name, ifname );
	if ((rv = rump_sys_ioctl(s, SIOCGIFINDEX, &ifr)) < 0) {
		atf_tc_fail_errno("SIOCGIFINDEX");
	}
	ATF_CHECK_MSG(ifr.ifr_ifindex > 0, "%s index is %d (not > 0)",
	    ifname, ifr.ifr_ifindex);

	sa.can_family = AF_CAN;
	sa.can_ifindex = ifr.ifr_ifindex;

	/*
	 * send a single byte message.
	 * not 0.
	 */

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = 1;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;

	if (rump_sys_sendto(s, &cf_send, sizeof(cf_send) - 7,
	    0, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		atf_tc_fail_errno("sendto");
	}

	memset(&cf_receive, 0, sizeof(cf_receive));
	if (( rv = rump_sys_read(s, &cf_receive, sizeof(cf_receive))) < 0) {
		atf_tc_fail_errno("read");
	}

	ATF_CHECK_MSG(rv > 0, "short read on socket");

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = 1;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;
	/* other data[] are expected to be 0 */

	ATF_CHECK_MSG(memcmp(&cf_send, &cf_receive, sizeof(cf_send)) == 0,
	    "received packet is not what we sent");

	rv = rump_sys_write(s, &cf_send, sizeof(cf_send) - 7);
	ATF_CHECK_MSG(rv < 0 && errno == EDESTADDRREQ, 
	    "write to unbound socket didn't fail");
}

ATF_TC(canreadlocal);
ATF_TC_HEAD(canreadlocal, tc)
{

	atf_tc_set_md_var(tc, "descr", "check all CAN sockets get messages");
	atf_tc_set_md_var(tc, "timeout", "5");
}

ATF_TC_BODY(canreadlocal, tc)
{
	const char ifname[] = "canlo0";
	int s1, rv1;
	int s2, rv2;
	struct sockaddr_can sa;
	struct ifreq ifr;
	struct can_frame cf_send, cf_receive1, cf_receive2;

	rump_init();
	cancfg_rump_createif(ifname);

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = 1;
	cf_send.can_dlc = 8;
	cf_send.data[0] = 0xde;
	cf_send.data[1] = 0xad;
	cf_send.data[2] = 0xbe;
	cf_send.data[3] = 0xef;


	s1 = -1;
	if ((s1 = rump_sys_socket(AF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		atf_tc_fail_errno("CAN socket");
	}

	/* create a second socket */

	s2 = -1;
	if ((s2 = rump_sys_socket(AF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		atf_tc_fail_errno("CAN socket");
	}

	strcpy(ifr.ifr_name, ifname );
	if ((rv2 = rump_sys_ioctl(s2, SIOCGIFINDEX, &ifr)) < 0) {
		atf_tc_fail_errno("SIOCGIFINDEX");
	}
	ATF_CHECK_MSG(ifr.ifr_ifindex > 0, "%s index is %d (not > 0)",
	    ifname, ifr.ifr_ifindex);

	sa.can_family = AF_CAN;
	sa.can_ifindex = ifr.ifr_ifindex;

	if ((rv2 = rump_sys_bind(s2, (struct sockaddr *)&sa, sizeof(sa))) < 0) {
		atf_tc_fail_errno("bind");
	}

	/*
	 * send a single byte message, but make sure remaining payload is
	 * not 0.
	 */

	if (rump_sys_write(s2, &cf_send, sizeof(cf_send)) < 0) {
		atf_tc_fail_errno("write");
	}

	memset(&cf_receive2, 0, sizeof(cf_receive2));
	if (( rv2 = rump_sys_read(s2, &cf_receive2, sizeof(cf_receive2))) < 0) {
		atf_tc_fail_errno("read");
	}

	ATF_CHECK_MSG(rv2 > 0, "short read on socket");

	ATF_CHECK_MSG(memcmp(&cf_send, &cf_receive2, sizeof(cf_send)) == 0,
	    "received (2) packet is not what we sent");

	/* now check first socket */
	memset(&cf_receive1, 0, sizeof(cf_receive1));
	if (( rv1 = rump_sys_read(s1, &cf_receive1, sizeof(cf_receive1))) < 0) {
		atf_tc_fail_errno("read");
	}

	ATF_CHECK_MSG(rv1 > 0, "short read on socket");

	ATF_CHECK_MSG(memcmp(&cf_send, &cf_receive1, sizeof(cf_send)) == 0,
	    "received (1) packet is not what we sent");
}

ATF_TC(canrecvfrom);
ATF_TC_HEAD(canrecvfrom, tc)
{

	atf_tc_set_md_var(tc, "descr", "check that recvfrom gets the CAN interface");
	atf_tc_set_md_var(tc, "timeout", "5");
}

ATF_TC_BODY(canrecvfrom, tc)
{
	const char ifname[] = "canlo0";
	int s1, rv1;
	int s2, rv2;
	struct sockaddr_can sa;
	struct ifreq ifr;
	struct can_frame cf_send, cf_receive1, cf_receive2;
	socklen_t salen;

	rump_init();
	cancfg_rump_createif(ifname);

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = 1;
	cf_send.can_dlc = 8;
	cf_send.data[0] = 0xde;
	cf_send.data[1] = 0xad;
	cf_send.data[2] = 0xbe;
	cf_send.data[3] = 0xef;


	s1 = -1;
	if ((s1 = rump_sys_socket(AF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		atf_tc_fail_errno("CAN socket");
	}

	/* create a second socket */

	s2 = -1;
	if ((s2 = rump_sys_socket(AF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		atf_tc_fail_errno("CAN socket");
	}

	strcpy(ifr.ifr_name, ifname );
	if ((rv2 = rump_sys_ioctl(s2, SIOCGIFINDEX, &ifr)) < 0) {
		atf_tc_fail_errno("SIOCGIFINDEX");
	}
	ATF_CHECK_MSG(ifr.ifr_ifindex > 0, "%s index is %d (not > 0)",
	    ifname, ifr.ifr_ifindex);

	sa.can_family = AF_CAN;
	sa.can_ifindex = ifr.ifr_ifindex;

	if ((rv2 = rump_sys_bind(s2, (struct sockaddr *)&sa, sizeof(sa))) < 0) {
		atf_tc_fail_errno("bind");
	}

	if (rump_sys_write(s2, &cf_send, sizeof(cf_send)) < 0) {
		atf_tc_fail_errno("write");
	}

	memset(&cf_receive2, 0, sizeof(cf_receive2));
	if (( rv2 = rump_sys_read(s2, &cf_receive2, sizeof(cf_receive2))) < 0) {
		atf_tc_fail_errno("read");
	}

	ATF_CHECK_MSG(rv2 > 0, "short read on socket");

	ATF_CHECK_MSG(memcmp(&cf_send, &cf_receive2, sizeof(cf_send)) == 0,
	    "received (2) packet is not what we sent");

	/* now check first socket */
	memset(&cf_receive1, 0, sizeof(cf_receive1));
	memset(&sa, 0, sizeof(sa));
	salen = sizeof(sa);
	if (( rv1 = rump_sys_recvfrom(s1, &cf_receive1, sizeof(cf_receive1),
	    0, (struct sockaddr *)&sa, &salen)) < 0) {
		atf_tc_fail_errno("recvfrom");
	}

	ATF_CHECK_MSG(rv1 > 0, "short read on socket");

	ATF_CHECK_MSG(memcmp(&cf_send, &cf_receive1, sizeof(cf_send)) == 0,
	    "recvfrom (1) packet is not what we sent");
	ATF_CHECK_MSG(sa.can_family == AF_CAN,
	    "recvfrom provided wrong %d family", sa.can_family);
	ATF_CHECK_MSG(salen == sizeof(sa),
	    "recvfrom provided wrong size %d (!= %d)", salen, sizeof(sa));
	ATF_CHECK_MSG(sa.can_ifindex == ifr.ifr_ifindex,
	   "recvfrom provided wrong ifindex %d (!= %d)",
	    sa.can_ifindex, ifr.ifr_ifindex);
}

ATF_TC(canbindfilter);
ATF_TC_HEAD(canbindfilter, tc)
{

	atf_tc_set_md_var(tc, "descr", "check that  socket bound to an interface doens't get other interface's messages");
	atf_tc_set_md_var(tc, "timeout", "5");
}

ATF_TC_BODY(canbindfilter, tc)
{
	const char ifname[] = "canlo0";
	const char ifname2[] = "canlo1";
	int s1, rv1;
	int s2, rv2;
	struct sockaddr_can sa;
	struct ifreq ifr;
	struct can_frame cf_send, cf_receive1, cf_receive2;
	socklen_t salen;
	fd_set rfds;
	struct timeval tmout;

	rump_init();
	cancfg_rump_createif(ifname);
	cancfg_rump_createif(ifname2);

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = 1;
	cf_send.can_dlc = 8;
	cf_send.data[0] = 0xde;
	cf_send.data[1] = 0xad;
	cf_send.data[2] = 0xbe;
	cf_send.data[3] = 0xef;


	s1 = -1;
	if ((s1 = rump_sys_socket(AF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		atf_tc_fail_errno("CAN socket");
	}

	strcpy(ifr.ifr_name, ifname );
	if ((rv1 = rump_sys_ioctl(s1, SIOCGIFINDEX, &ifr)) < 0) {
		atf_tc_fail_errno("SIOCGIFINDEX (1)");
	}
	ATF_CHECK_MSG(ifr.ifr_ifindex > 0, "%s index is %d (not > 0)",
	    ifname, ifr.ifr_ifindex);

	sa.can_family = AF_CAN;
	sa.can_ifindex = ifr.ifr_ifindex;

	if ((rv1 = rump_sys_bind(s1, (struct sockaddr *)&sa, sizeof(sa))) < 0) {
		atf_tc_fail_errno("bind (1)");
	}

	/* create a second socket */

	s2 = -1;
	if ((s2 = rump_sys_socket(AF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		atf_tc_fail_errno("CAN socket");
	}

	strcpy(ifr.ifr_name, ifname2);
	if ((rv2 = rump_sys_ioctl(s2, SIOCGIFINDEX, &ifr)) < 0) {
		atf_tc_fail_errno("SIOCGIFINDEX");
	}
	ATF_CHECK_MSG(ifr.ifr_ifindex > 0, "%s index is %d (not > 0)",
	    ifname, ifr.ifr_ifindex);

	sa.can_family = AF_CAN;
	sa.can_ifindex = ifr.ifr_ifindex;

	if ((rv2 = rump_sys_bind(s2, (struct sockaddr *)&sa, sizeof(sa))) < 0) {
		atf_tc_fail_errno("bind");
	}

	if (rump_sys_write(s2, &cf_send, sizeof(cf_send)) < 0) {
		atf_tc_fail_errno("write");
	}

	memset(&cf_receive2, 0, sizeof(cf_receive2));
	if (( rv2 = rump_sys_read(s2, &cf_receive2, sizeof(cf_receive2))) < 0) {
		atf_tc_fail_errno("read");
	}

	ATF_CHECK_MSG(rv2 > 0, "short read on socket");

	ATF_CHECK_MSG(memcmp(&cf_send, &cf_receive2, sizeof(cf_send)) == 0,
	    "received (2) packet is not what we sent");

	/* now check first socket */
	memset(&cf_receive1, 0, sizeof(cf_receive1));
	FD_ZERO(&rfds);
	FD_SET(s1, &rfds);
	/* we should receive no message; wait for 2 seconds */
	tmout.tv_sec = 2;
	tmout.tv_usec = 0;
	rv1 = rump_sys_select(s1 + 1, &rfds, NULL, NULL, &tmout);
	switch(rv1) {
	case -1:
		atf_tc_fail_errno("select");
		break;
	case 0:
		/* timeout: expected case */
		return;
	default: break;
	}
	salen = sizeof(sa);
	ATF_CHECK_MSG(FD_ISSET(s1, &rfds), "select returns but s1 not in set");
	if (( rv1 = rump_sys_recvfrom(s1, &cf_receive1, sizeof(cf_receive1),
	    0, (struct sockaddr *)&sa, &salen)) < 0) {
		atf_tc_fail_errno("recvfrom");
	}

	ATF_CHECK_MSG(rv1 > 0, "short read on socket");

	ATF_CHECK_MSG(memcmp(&cf_send, &cf_receive1, sizeof(cf_send)) == 0,
	    "recvfrom (1) packet is not what we sent");
	ATF_CHECK_MSG(sa.can_family == AF_CAN,
	    "recvfrom provided wrong %d family", sa.can_family);
	ATF_CHECK_MSG(salen == sizeof(sa),
	    "recvfrom provided wrong size %d (!= %d)", salen, sizeof(sa));
	ATF_CHECK_MSG(sa.can_ifindex == ifr.ifr_ifindex,
	   "recvfrom provided wrong ifindex %d (!= %d)",
	    sa.can_ifindex, ifr.ifr_ifindex);
	atf_tc_fail("we got message from other interface");
}

ATF_TP_ADD_TCS(tp)
{

        ATF_TP_ADD_TC(tp, canlocreate);
        ATF_TP_ADD_TC(tp, canwritelo);
        ATF_TP_ADD_TC(tp, canwriteunbound);
        ATF_TP_ADD_TC(tp, cansendtolo);
        ATF_TP_ADD_TC(tp, cansendtowrite);
        ATF_TP_ADD_TC(tp, canreadlocal);
        ATF_TP_ADD_TC(tp, canrecvfrom);
        ATF_TP_ADD_TC(tp, canbindfilter);
	return atf_no_error();
}


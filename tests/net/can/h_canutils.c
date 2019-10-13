/*	$NetBSD: h_canutils.c,v 1.4 2019/10/13 07:46:16 mrg Exp $	*/

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
__RCSID("$NetBSD: h_canutils.c,v 1.4 2019/10/13 07:46:16 mrg Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/sockio.h>
#include <sys/param.h>

#include <atf-c.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <netcan/can.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include "h_macros.h"
#include "h_canutils.h"

void
cancfg_rump_createif(const char *ifname)
{
	int s, rv;
	struct ifreq ifr;

	s = -1;
	if ((s = rump_sys_socket(AF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		atf_tc_fail_errno("if config socket");
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ-1);
	ifr.ifr_name[IFNAMSIZ - 1] = '\0';

	if ((rv = rump_sys_ioctl(s, SIOCIFCREATE, &ifr)) < 0) {
		atf_tc_fail_errno("if config create");
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ-1);
	ifr.ifr_name[IFNAMSIZ - 1] = '\0';

	if ((rv = rump_sys_ioctl(s, SIOCGIFFLAGS, &ifr)) < 0) {
		atf_tc_fail_errno("if config get flags");
	}

	ifr.ifr_flags |= IFF_UP;
	if ((rv = rump_sys_ioctl(s, SIOCSIFFLAGS, &ifr)) < 0) {
		atf_tc_fail_errno("if config set flags");
	}
}

int
can_recvfrom(int s, struct can_frame *cf, int *len, struct sockaddr_can *sa)
{
	socklen_t salen;
	fd_set rfds;
	struct timeval tmout;
	int rv;

	memset(cf,  0, sizeof(struct can_frame));
	FD_ZERO(&rfds);
	FD_SET(s, &rfds);
	/* wait 1 second for the message (in some tests we expect no message) */
	tmout.tv_sec = 1;
	tmout.tv_usec = 0;
	rv = rump_sys_select(s + 1, &rfds, NULL, NULL, &tmout);
	switch(rv) {
	case -1:
		atf_tc_fail_errno("select");
		/* NOTREACHED */
	case 0:
		/* timeout */
		errno = EWOULDBLOCK;
		return -1;
	default: break;
	}
	ATF_CHECK_MSG(FD_ISSET(s, &rfds), "select returns but s not in set");
	salen = sizeof(struct sockaddr_can);
	if (( *len = rump_sys_recvfrom(s, cf, sizeof(struct can_frame),
	    0, (struct sockaddr *)sa, &salen)) < 0) {
		atf_tc_fail_errno("recvfrom");
	}
	ATF_CHECK_MSG(rv > 0, "short read on socket");
	ATF_CHECK_MSG(sa->can_family == AF_CAN,
	    "recvfrom provided wrong %d family", sa->can_family);       
        ATF_CHECK_MSG(salen == sizeof(struct sockaddr_can),
            "recvfrom provided wrong size %d (!= %zu)", salen, sizeof(sa));
	return 0;
}

int
can_read(int s, struct can_frame *cf, int *len)
{
	fd_set rfds;
	struct timeval tmout;
	int rv;

	memset(cf,  0, sizeof(struct can_frame));
	FD_ZERO(&rfds);
	FD_SET(s, &rfds);
	/* wait 1 second for the message (in some tests we expect no message) */
	tmout.tv_sec = 1;
	tmout.tv_usec = 0;
	rv = rump_sys_select(s + 1, &rfds, NULL, NULL, &tmout);
	switch(rv) {
	case -1:
		atf_tc_fail_errno("select");
		/* NOTREACHED */
	case 0:
		/* timeout */
		errno = EWOULDBLOCK;
		return -1;
	default: break;
	}
	ATF_CHECK_MSG(FD_ISSET(s, &rfds), "select returns but s not in set");
	if (( *len = rump_sys_read(s, cf, sizeof(struct can_frame))) < 0) {
		atf_tc_fail_errno("read");
	}
	ATF_CHECK_MSG(rv > 0, "short read on socket");
	return 0;
}

int
can_bind(int s, const char *ifname)
{
	struct ifreq ifr;
	struct sockaddr_can sa;

	strcpy(ifr.ifr_name, ifname );
	if (rump_sys_ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
		atf_tc_fail_errno("SIOCGIFINDEX");
	}
	ATF_CHECK_MSG(ifr.ifr_ifindex > 0, "%s index is %d (not > 0)",
	    ifname, ifr.ifr_ifindex);

	sa.can_family = AF_CAN;
	sa.can_ifindex = ifr.ifr_ifindex;

	if (rump_sys_bind(s, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		atf_tc_fail_errno("bind");
	}
	return ifr.ifr_ifindex;
}

int
can_socket_with_own(void)
{
	int s;
	int v;

	if ((s = rump_sys_socket(AF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		atf_tc_fail_errno("CAN socket");
	}
	v = 1;
	if (rump_sys_setsockopt(s, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS,
	    &v, sizeof(v)) < 0) {
		atf_tc_fail_errno("setsockopt(CAN_RAW_RECV_OWN_MSGS)");
	}
	return s;
}

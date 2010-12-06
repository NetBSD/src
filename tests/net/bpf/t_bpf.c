/*	$NetBSD: t_bpf.c,v 1.1 2010/12/06 11:32:01 pooka Exp $	*/

/*-
 * Copyright (c) 2010 Antti Kantee.  All Rights Reserved.
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

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/bpf.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

/* XXX: atf-c.h has collisions with mbuf */
#undef m_type
#undef m_data
#include <atf-c.h>

#include "../../h_macros.h"

ATF_TC(bpfwriteleak);
ATF_TC_HEAD(bpfwriteleak, tc)
{

	atf_tc_set_md_var(tc, "descr", "Checks that writing to /dev/bpf "
	    "does not leak mbufs");
}

static int
getmtdata(void)
{
	struct mbstat mbstat;
	size_t mbstatlen = sizeof(mbstat);
	const int mbstat_mib[] = { CTL_KERN, KERN_MBUF, MBUF_STATS };

	RL(rump_sys___sysctl(mbstat_mib, __arraycount(mbstat_mib),
	    &mbstat, &mbstatlen, NULL, 0));
	return mbstat.m_mtypes[MT_DATA];
}

ATF_TC_BODY(bpfwriteleak, tc)
{
	char buf[28]; /* sizeof(garbage) > etherhdrlen */
	struct ifreq ifr;
	int ifnum, bpfd;
	u_int x;

	RZ(rump_init());
	RZ(rump_pub_shmif_create(NULL, &ifnum));
	sprintf(ifr.ifr_name, "shmif%d", ifnum);

	RL(bpfd = rump_sys_open("/dev/bpf", O_RDWR));
	RL(rump_sys_ioctl(bpfd, BIOCSETIF, &ifr));
        x = 1;
	RL(rump_sys_ioctl(bpfd, BIOCSFEEDBACK, &ifr));

	if (getmtdata() != 0)
		atf_tc_fail("test precondition failed: MT_DATA mbufs != 0");

	ATF_REQUIRE_ERRNO(ENETDOWN, rump_sys_write(bpfd, buf, sizeof(buf))==-1);

	atf_tc_expect_fail("PR kern/44196");
	ATF_REQUIRE_EQ(getmtdata(), 0);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, bpfwriteleak);
	return atf_no_error();
}

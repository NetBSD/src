/*	$NetBSD: t_ping.c,v 1.1 2010/08/09 15:08:43 pooka Exp $	*/

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
__RCSID("$NetBSD: t_ping.c,v 1.1 2010/08/09 15:08:43 pooka Exp $");
#endif /* not lint */

#include <sys/types.h>

#include <atf-c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rump/rump.h>

#include "../../h_macros.h"
#include "../config/netconfig.c"

ATF_TC(simpleping);
ATF_TC_HEAD(simpleping, tc)
{

	atf_tc_set_md_var(tc, "descr", "check that kernel responds to ping");
	atf_tc_set_md_var(tc, "use.fs", "true");
	atf_tc_set_md_var(tc, "timeout", "2");
}

ATF_TC_BODY(simpleping, tc)
{
	char ifname[IFNAMSIZ];
	pid_t cpid;
	int status;
	bool win, win2;

	cpid = fork();
	rump_init();
	netcfg_rump_makeshmif("but-can-i-buy-your-ether-bus", ifname);

	switch (cpid) {
	case -1:
		atf_tc_fail_errno("fork failed");
	case 0:
		netcfg_rump_if(ifname, "1.1.1.10", "255.255.255.0");
		pause();
		break;
	default:
		break;
	}

	netcfg_rump_if(ifname, "1.1.1.20", "255.255.255.0");

	/*
	 * The beauty of shmif is that we don't have races here.
	 */
	win = netcfg_rump_pingtest("1.1.1.10", 500);
	win2 = netcfg_rump_pingtest("1.1.1.30", 500);

	kill(cpid, SIGKILL);

	if (!win)
		atf_tc_fail("ping failed");
	if (win2)
		atf_tc_fail("non-existent host responded");
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, simpleping);

	return atf_no_error();
}

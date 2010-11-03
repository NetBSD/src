/*	$NetBSD: t_listen.c,v 1.2 2010/11/03 16:10:25 christos Exp $	*/
/*
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
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

#include <err.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>

#include <atf-c.h>

ATF_TC(low_port);
ATF_TC_HEAD(low_port, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks that low-port allocation "
	    "works");
	atf_tc_set_md_var(tc, "require.user", "root");
}
ATF_TC_BODY(low_port, tc)
{
	int sd, val;

	sd = socket(AF_INET, SOCK_STREAM, 0);

	val = IP_PORTRANGE_LOW;
	if (setsockopt(sd, IPPROTO_IP, IP_PORTRANGE, &val,
	    sizeof(val)) == -1)
		atf_tc_fail("setsockopt failed: %s", strerror(errno));

	if (listen(sd, 5) == -1) {
		int serrno = errno;
		atf_tc_fail("listen failed: %s%s",
		    strerror(serrno),
		    serrno != EACCES ? "" :
		    " (see http://mail-index.netbsd.org/"
		    "source-changes/2007/12/16/0011.html)");
	}

	close(sd);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, low_port);
	return 0;
}

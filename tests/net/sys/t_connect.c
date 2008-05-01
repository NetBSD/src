/*	$NetBSD: t_connect.c,v 1.1 2008/05/01 15:38:17 jmmv Exp $	*/
/*
 * Copyright (c) 2007, 2008 The NetBSD Foundation, Inc.
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

#include <arpa/inet.h>
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
	struct sockaddr_in sin;
	int sd, val;

	sd = socket(AF_INET, SOCK_STREAM, 0);

	val = IP_PORTRANGE_LOW;
	if (setsockopt(sd, IPPROTO_IP, IP_PORTRANGE, &val,
	    sizeof(val)) == -1)
		atf_tc_fail("setsockopt failed: %s", strerror(errno));

	memset(&sin, 0, sizeof(sin));

	sin.sin_port = htons(80);
	sin.sin_addr.s_addr = inet_addr("204.152.190.12"); /* www.NetBSD.org */
	sin.sin_family = AF_INET;

	if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		int err = errno;
		atf_tc_fail("connect failed: %s%s",
		    strerror(err),
		    err != EACCES ? "" : " (see http://mail-index.netbsd.org/"
		    "source-changes/2007/12/16/0011.html)");
	}

	close(sd);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, low_port);

	return atf_no_error();
}

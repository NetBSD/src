/*	$NetBSD: t_canfilter.c,v 1.2 2017/05/27 21:02:56 bouyer Exp $	*/

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
__RCSID("$NetBSD: t_canfilter.c,v 1.2 2017/05/27 21:02:56 bouyer Exp $");
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

ATF_TC(canfilter_basic);
ATF_TC_HEAD(canfilter_basic, tc)
{

	atf_tc_set_md_var(tc, "descr", "check a simple CAN filter");
	atf_tc_set_md_var(tc, "timeout", "5");
}

ATF_TC_BODY(canfilter_basic, tc)
{
	const char ifname[] = "canlo0";
	int s, rv;
	struct can_frame cf_send, cf_receive;
	struct can_filter cfi;

	rump_init();
	cancfg_rump_createif(ifname);

	s = can_socket_with_own();

	can_bind(s, ifname);

	/* set filter */
#define MY_ID	1
	cfi.can_id = MY_ID;
	cfi.can_mask = CAN_SFF_MASK | CAN_EFF_FLAG;
	if (rump_sys_setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER,
	    &cfi, sizeof(cfi)) < 0) {
		atf_tc_fail_errno("setsockopt(CAN_RAW_FILTER)");
	}

	/*
	 * send a single byte message, but make sure remaining payload is
	 * not 0.
	 */

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = MY_ID;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;
	cf_send.data[1] = 0xad;
	cf_send.data[2] = 0xbe;
	cf_send.data[3] = 0xef;

	if (rump_sys_write(s, &cf_send, sizeof(cf_send) - 7) < 0) {
		atf_tc_fail_errno("write");
	}

	if (can_read(s, &cf_receive, &rv) < 0) {
		atf_tc_fail_errno("read");
	}

	ATF_CHECK_MSG(rv > 0, "short read on socket");

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = MY_ID;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;
	/* other data[] are expected to be 0 */

	ATF_CHECK_MSG(memcmp(&cf_send, &cf_receive, sizeof(cf_send)) == 0,
	    "received packet is not what we sent");

	/* now send a packet with CAN_RTR_FLAG. Should pass too */

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = MY_ID | CAN_RTR_FLAG;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;
	cf_send.data[1] = 0xad;
	cf_send.data[2] = 0xbe;
	cf_send.data[3] = 0xef;

	if (rump_sys_write(s, &cf_send, sizeof(cf_send) - 7) < 0) {
		atf_tc_fail_errno("write");
	}

	if (can_read(s, &cf_receive, &rv) < 0) {
		atf_tc_fail_errno("read");
	}

	ATF_CHECK_MSG(rv > 0, "short read on socket");

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = MY_ID | CAN_RTR_FLAG;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;
	/* other data[] are expected to be 0 */

	ATF_CHECK_MSG(memcmp(&cf_send, &cf_receive, sizeof(cf_send)) == 0,
	    "received packet is not what we sent");

	/* now send a packet for a different id. Should not pass */

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = MY_ID + 1;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;
	cf_send.data[1] = 0xad;
	cf_send.data[2] = 0xbe;
	cf_send.data[3] = 0xef;

	if (rump_sys_write(s, &cf_send, sizeof(cf_send) - 7) < 0) {
		atf_tc_fail_errno("write");
	}

	if (can_read(s, &cf_receive, &rv) < 0) {
		if (errno == EWOULDBLOCK)
			return; /* expected timeout */
		atf_tc_fail_errno("read");
	}

	ATF_CHECK_MSG(rv > 0, "short read on socket");

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = MY_ID + 1;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;
	/* other data[] are expected to be 0 */

	ATF_CHECK_MSG(memcmp(&cf_send, &cf_receive, sizeof(cf_send)) == 0,
	    "received packet is not what we sent");
	atf_tc_fail("we got our own message");
#undef MY_ID
}

ATF_TC(canfilter_null);
ATF_TC_HEAD(canfilter_null, tc)
{

	atf_tc_set_md_var(tc, "descr", "check a NULL CAN filter");
	atf_tc_set_md_var(tc, "timeout", "5");
}

ATF_TC_BODY(canfilter_null, tc)
{
	const char ifname[] = "canlo0";
	int s, rv;
	struct can_frame cf_send, cf_receive;
	struct can_filter cfi[2];
	socklen_t cfilen;

	rump_init();
	cancfg_rump_createif(ifname);

	s = can_socket_with_own();
	can_bind(s, ifname);

	if (rump_sys_setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER,
	    NULL,  0) < 0) {
		atf_tc_fail_errno("setsockopt(CAN_RAW_FILTER)");
	}

	/* get filter: should be NULL */
	cfilen = sizeof(cfi);
	if (rump_sys_getsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER,
	    &cfi, &cfilen) < 0) {
		atf_tc_fail_errno("getsockopt(CAN_RAW_FILTER)");
	}
	ATF_CHECK_MSG(cfilen == 0,
	    "CAN_RAW_FILTER returns wrong len (%d)", cfilen);
	/*
	 * send a single byte message, but make sure remaining payload is
	 * not 0.
	 */
#define MY_ID	1

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = MY_ID;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;
	cf_send.data[1] = 0xad;
	cf_send.data[2] = 0xbe;
	cf_send.data[3] = 0xef;

	if (rump_sys_write(s, &cf_send, sizeof(cf_send) - 7) < 0) {
		atf_tc_fail_errno("write");
	}

	if (can_read(s, &cf_receive, &rv) < 0) {
		if (errno == EWOULDBLOCK)
			return; /* expected timeout */
		atf_tc_fail_errno("read");
	}

	ATF_CHECK_MSG(rv > 0, "short read on socket");

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = MY_ID;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;
	/* other data[] are expected to be 0 */

	ATF_CHECK_MSG(memcmp(&cf_send, &cf_receive, sizeof(cf_send)) == 0,
	    "received packet is not what we sent");
	atf_tc_fail("we got our own message");
#undef MY_ID
}

ATF_TC(canfilter_multiple);
ATF_TC_HEAD(canfilter_multiple, tc)
{

	atf_tc_set_md_var(tc, "descr", "check multiple CAN filters");
	atf_tc_set_md_var(tc, "timeout", "5");
}

ATF_TC_BODY(canfilter_multiple, tc)
{
	const char ifname[] = "canlo0";
	int s, rv;
	struct can_frame cf_send, cf_receive;
	struct can_filter cfi[2];

	rump_init();
	cancfg_rump_createif(ifname);

	s = can_socket_with_own();

	can_bind(s, ifname);

	/* set filter: accept MY_ID and MY_ID+1 */
#define MY_ID	1
	cfi[0].can_id = MY_ID;
	cfi[0].can_mask = CAN_SFF_MASK | CAN_EFF_FLAG;
	cfi[1].can_id = MY_ID + 1;
	cfi[1].can_mask = CAN_SFF_MASK | CAN_EFF_FLAG;
	if (rump_sys_setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER,
	    &cfi, sizeof(cfi)) < 0) {
		atf_tc_fail_errno("setsockopt(CAN_RAW_FILTER)");
	}

	/*
	 * send a single byte message, but make sure remaining payload is
	 * not 0.
	 */

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = MY_ID;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;
	cf_send.data[1] = 0xad;
	cf_send.data[2] = 0xbe;
	cf_send.data[3] = 0xef;

	if (rump_sys_write(s, &cf_send, sizeof(cf_send) - 7) < 0) {
		atf_tc_fail_errno("write");
	}

	if (can_read(s, &cf_receive, &rv) < 0) {
		atf_tc_fail_errno("read");
	}

	ATF_CHECK_MSG(rv > 0, "short read on socket");

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = MY_ID;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;
	/* other data[] are expected to be 0 */

	ATF_CHECK_MSG(memcmp(&cf_send, &cf_receive, sizeof(cf_send)) == 0,
	    "received packet is not what we sent");

	/* now send a packet with MY_ID+1. Should pass too */

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = MY_ID + 1;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;
	cf_send.data[1] = 0xad;
	cf_send.data[2] = 0xbe;
	cf_send.data[3] = 0xef;

	if (rump_sys_write(s, &cf_send, sizeof(cf_send) - 7) < 0) {
		atf_tc_fail_errno("write");
	}

	if (can_read(s, &cf_receive, &rv) < 0) {
		atf_tc_fail_errno("read");
	}

	ATF_CHECK_MSG(rv > 0, "short read on socket");

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = MY_ID  + 1;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;
	/* other data[] are expected to be 0 */

	ATF_CHECK_MSG(memcmp(&cf_send, &cf_receive, sizeof(cf_send)) == 0,
	    "received packet is not what we sent");

	/* now send a packet with MY_ID + 2. Should not pass */

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = MY_ID + 2;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;
	cf_send.data[1] = 0xad;
	cf_send.data[2] = 0xbe;
	cf_send.data[3] = 0xef;

	if (rump_sys_write(s, &cf_send, sizeof(cf_send) - 7) < 0) {
		atf_tc_fail_errno("write");
	}

	if (can_read(s, &cf_receive, &rv) < 0) {
		if (errno == EWOULDBLOCK)
			return; /* expected timeout */
		atf_tc_fail_errno("read");
	}

	ATF_CHECK_MSG(rv > 0, "short read on socket");

	memset(&cf_send, 0, sizeof(cf_send));
	cf_send.can_id = MY_ID + 2;
	cf_send.can_dlc = 1;
	cf_send.data[0] = 0xde;
	/* other data[] are expected to be 0 */

	ATF_CHECK_MSG(memcmp(&cf_send, &cf_receive, sizeof(cf_send)) == 0,
	    "received packet is not what we sent");
	atf_tc_fail("we got our own message");
#undef MY_ID
}

ATF_TC(canfilter_get);
ATF_TC_HEAD(canfilter_get, tc)
{

	atf_tc_set_md_var(tc, "descr", "check reading CAN filters");
	atf_tc_set_md_var(tc, "timeout", "5");
}

ATF_TC_BODY(canfilter_get, tc)
{
	const char ifname[] = "canlo0";
	int s;
	struct can_filter cfi[2];
	socklen_t cfilen;

	rump_init();
	cancfg_rump_createif(ifname);

	s = -1;
	if ((s = rump_sys_socket(AF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		atf_tc_fail_errno("CAN socket");
	}

	cfilen = sizeof(cfi);
	/* get filter: should be default filter */
	if (rump_sys_getsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER,
	    &cfi, &cfilen) < 0) {
		atf_tc_fail_errno("getsockopt(CAN_RAW_FILTER)");
	}
	ATF_CHECK_MSG(cfilen == sizeof(struct can_filter),
	    "CAN_RAW_FILTER returns wrong len (%d)", cfilen);
	ATF_CHECK_MSG(cfi[0].can_id == 0 && cfi[0].can_mask == 0,
	    "CAN_RAW_FILTER returns wrong filter (%d, %d)",
	    cfi[0].can_id, cfi[0].can_mask);

	/* set filter: accept MY_ID and MY_ID+1 */
#define MY_ID	1
	cfi[0].can_id = MY_ID;
	cfi[0].can_mask = CAN_SFF_MASK | CAN_EFF_FLAG;
	cfi[1].can_id = MY_ID + 1;
	cfi[1].can_mask = CAN_SFF_MASK;
	if (rump_sys_setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER,
	    &cfi, sizeof(cfi)) < 0) {
		atf_tc_fail_errno("setsockopt(CAN_RAW_FILTER)");
	}

	/* and read back */
	cfilen = sizeof(cfi);
	memset(cfi, 0, cfilen);
	if (rump_sys_getsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER,
	    &cfi, &cfilen) < 0) {
		atf_tc_fail_errno("getsockopt(CAN_RAW_FILTER)");
	}
	ATF_CHECK_MSG(cfilen == sizeof(struct can_filter) * 2,
	    "CAN_RAW_FILTER returns wrong len (%d)", cfilen);
	ATF_CHECK_MSG(cfi[0].can_id == MY_ID &&
	    cfi[0].can_mask == (CAN_SFF_MASK | CAN_EFF_FLAG),
	    "CAN_RAW_FILTER returns wrong filter 0 (%d, %d)",
	    cfi[0].can_id, cfi[0].can_mask);
	ATF_CHECK_MSG(cfi[1].can_id == MY_ID + 1 &&
	    cfi[1].can_mask == CAN_SFF_MASK,
	    "CAN_RAW_FILTER returns wrong filter 1 (%d, %d)",
	    cfi[1].can_id, cfi[1].can_mask);

#undef MY_ID
}

ATF_TP_ADD_TCS(tp)
{

        ATF_TP_ADD_TC(tp, canfilter_basic);
        ATF_TP_ADD_TC(tp, canfilter_null);
        ATF_TP_ADD_TC(tp, canfilter_multiple);
        ATF_TP_ADD_TC(tp, canfilter_get);
	return atf_no_error();
}


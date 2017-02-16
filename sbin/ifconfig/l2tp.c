/*	$NetBSD: l2tp.c,v 1.1 2017/02/16 08:28:03 knakahara Exp $	*/

/*
 * Copyright (c) 2017 Internet Initiative Japan Inc.
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
__RCSID("$NetBSD: l2tp.c,v 1.1 2017/02/16 08:28:03 knakahara Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_l2tp.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <util.h>

#include "env.h"
#include "extern.h"
#include "util.h"

static status_func_t status;
static usage_func_t usage;
static cmdloop_branch_t branch;

static void l2tp_constructor(void) __attribute__((constructor));
static void l2tp_status(prop_dictionary_t, prop_dictionary_t);

static int setl2tpsession(prop_dictionary_t, prop_dictionary_t);
static int deletel2tpsession(prop_dictionary_t, prop_dictionary_t);
static int setl2tpcookie(prop_dictionary_t, prop_dictionary_t);
static int deletel2tpcookie(prop_dictionary_t, prop_dictionary_t);

struct pinteger l2tpremotesession = PINTEGER_INITIALIZER1(&l2tpremotesession,
    "remote session id", 0, UINT_MAX, 10, setl2tpsession, "l2tpremotesession",
    &command_root.pb_parser);

struct pinteger l2tplocalsession = PINTEGER_INITIALIZER1(&l2tplocalsession,
    "local session id", 0, UINT_MAX, 10, NULL, "l2tplocalsession",
    &l2tpremotesession.pi_parser);

struct pinteger l2tpremotecookie = PINTEGER_INITIALIZER1(&l2tpremotecookie,
    "remote cookie", INT64_MIN, INT64_MAX, 10, setl2tpcookie, "l2tpremotecookie",
    &command_root.pb_parser);

struct pinteger l2tpremotecookielen = PINTEGER_INITIALIZER1(&l2tpremotecookielen,
    "remote cookie length", 0, UINT16_MAX, 10, NULL, "l2tpremotecookielen",
    &l2tpremotecookie.pi_parser);

struct pinteger l2tplocalcookie = PINTEGER_INITIALIZER1(&l2tplocalcookie,
    "local cookie", INT64_MIN, INT64_MAX, 10, NULL, "l2tplocalcookie",
    &l2tpremotecookielen.pi_parser);

struct pinteger l2tplocalcookielen = PINTEGER_INITIALIZER1(&l2tplocalcookielen,
    "local cookie length", 0, UINT16_MAX, 10, NULL, "l2tplocalcookielen",
    &l2tplocalcookie.pi_parser);

static const struct kwinst l2tpkw[] = {
	 {.k_word = "cookie", .k_nextparser = &l2tplocalcookielen.pi_parser}
	,{.k_word = "deletecookie", .k_exec = deletel2tpcookie,
	  .k_nextparser = &command_root.pb_parser}
	,{.k_word = "session", .k_nextparser = &l2tplocalsession.pi_parser}
	,{.k_word = "deletesession", .k_exec = deletel2tpsession,
	  .k_nextparser = &command_root.pb_parser}
};

struct pkw l2tp = PKW_INITIALIZER(&l2tp, "l2tp", NULL, NULL,
    l2tpkw, __arraycount(l2tpkw), NULL);

#define L2TP_COOKIE_LOCAL  0
#define L2TP_COOKIE_REMOTE 1

static int
checkifname(prop_dictionary_t env)
{
	const char *ifname;

	if ((ifname = getifname(env)) == NULL)
		return 1;

	return strncmp(ifname, "l2tp", 4) != 0 ||
	    !isdigit((unsigned char)ifname[4]);
}

static int
getl2tp(prop_dictionary_t env, struct l2tp_req *l2tpr, bool quiet)
{
	memset(l2tpr, 0, sizeof(*l2tpr));

	if (checkifname(env)) {
		if (quiet)
			return -1;
		errx(EXIT_FAILURE, "valid only with l2tp(4) interfaces");
	}

	if (indirect_ioctl(env, SIOCGL2TP, l2tpr) == -1)
		return -1;

	return 0;
}

int
deletel2tpsession(prop_dictionary_t env, prop_dictionary_t oenv)
{
	struct l2tp_req l2tpr;

	memset(&l2tpr, 0, sizeof(l2tpr));

	if (indirect_ioctl(env, SIOCDL2TPSESSION, &l2tpr) == -1)
		return -1;

	l2tpr.state = L2TP_STATE_DOWN;

	if (indirect_ioctl(env, SIOCSL2TPSTATE, &l2tpr) == -1)
		return -1;


	return 0;
}

int
setl2tpsession(prop_dictionary_t env, prop_dictionary_t oenv)
{
	struct l2tp_req l2tpr;
	int64_t local_session;
	int64_t remote_session;

	memset(&l2tpr, 0, sizeof(l2tpr));

	if (!prop_dictionary_get_int64(env, "l2tplocalsession",
		&local_session)) {
		errno = ENOENT;
		return -1;
	}

	if (!prop_dictionary_get_int64(env, "l2tpremotesession",
		&remote_session)) {
		errno = ENOENT;
		return -1;
	}

	l2tpr.my_sess_id = local_session;
	l2tpr.peer_sess_id = remote_session;

	if (indirect_ioctl(env, SIOCSL2TPSESSION, &l2tpr) == -1)
		return -1;

	l2tpr.state = L2TP_STATE_UP;

	if (indirect_ioctl(env, SIOCSL2TPSTATE, &l2tpr) == -1)
		return -1;

	return 0;
}

int
deletel2tpcookie(prop_dictionary_t env, prop_dictionary_t oenv)
{
	struct l2tp_req l2tpr;

	memset(&l2tpr, 0, sizeof(l2tpr));

	if (indirect_ioctl(env, SIOCDL2TPCOOKIE, &l2tpr) == -1)
		return -1;

	return 0;
}

int
setl2tpcookie(prop_dictionary_t env, prop_dictionary_t oenv)
{
	struct l2tp_req l2tpr;
	uint16_t cookielen;
	uint64_t cookie;

	memset(&l2tpr, 0, sizeof(l2tpr));

	if (!prop_dictionary_get_uint16(env, "l2tplocalcookielen", &cookielen)) {
		errno = ENOENT;
		return -1;
	}
	if (!prop_dictionary_get_uint64(env, "l2tplocalcookie", &cookie)) {
		errno = ENOENT;
		return -1;
	}
	l2tpr.my_cookie_len = cookielen;
	l2tpr.my_cookie = cookie;

	if (!prop_dictionary_get_uint16(env, "l2tpremotecookielen", &cookielen)) {
		errno = ENOENT;
		return -1;
	}
	if (!prop_dictionary_get_uint64(env, "l2tpremotecookie", &cookie)) {
		errno = ENOENT;
		return -1;
	}
	l2tpr.peer_cookie_len = cookielen;
	l2tpr.peer_cookie = cookie;

	if (indirect_ioctl(env, SIOCSL2TPCOOKIE, &l2tpr) == -1)
		return -1;

	return 0;
}

static void
l2tp_status(prop_dictionary_t env, prop_dictionary_t oenv)
{
	struct l2tp_req l2tpr;

	if (getl2tp(env, &l2tpr, true) == -1)
		return;

	if (l2tpr.my_sess_id != 0 || l2tpr.peer_sess_id != 0) {
		printf("\tlocal-session-id: %u\n", l2tpr.my_sess_id);
		printf("\tremote-session-id: %u\n", l2tpr.peer_sess_id);
	}

	if (l2tpr.my_cookie != 0 || l2tpr.peer_cookie != 0) {
		printf("\tlocal-cookie: %" PRIu64 "\n", l2tpr.my_cookie);
		printf("\tremote-cookie: %" PRIu64 "\n", l2tpr.peer_cookie);
	}
}

static void
l2tp_usage(prop_dictionary_t env)
{
	fprintf(stderr, "\t[ session local-session-id remote-session-id ]\n");
	fprintf(stderr, "\t[ cookie local-cookie-length local-cookie remote-cookie-length remote-cookie ]\n");
}

static void
l2tp_constructor(void)
{
	cmdloop_branch_init(&branch, &l2tp.pk_parser);
	register_cmdloop_branch(&branch);
	status_func_init(&status, l2tp_status);
	usage_func_init(&usage, l2tp_usage);
	register_status(&status);
	register_usage(&usage);
}

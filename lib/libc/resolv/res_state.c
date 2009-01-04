/*	$NetBSD: res_state.c,v 1.7.8.2 2009/01/04 17:02:20 christos Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: res_state.c,v 1.7.8.2 2009/01/04 17:02:20 christos Exp $");
#endif

#include "namespace.h"
#include "reentrant.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <resolv.h>

struct __res_state _nres
# if defined(__BIND_RES_TEXT)
	= { .retrans = RES_TIMEOUT, }	/*%< Motorola, et al. */
# endif
	;

res_state __res_get_state_nothread(void);
void __res_put_state_nothread(res_state);

#ifdef __weak_alias
__weak_alias(__res_get_state, __res_get_state_nothread)
__weak_alias(__res_put_state, __res_put_state_nothread)
/* Source compatibility; only for single threaded programs */
__weak_alias(__res_state, __res_get_state_nothread)
__weak_alias(res_watch, _res_watch)
#endif

static int check;
static struct timespec mtime;
static char seq;
static mutex_t check_mutex = MUTEX_INITIALIZER;

static int
checktime(void)
{
	struct stat st;

	mutex_lock(&check_mutex);
	if (stat(_PATH_RESCONF, &st) == -1)
		return 0;

	if (memcmp(&mtime, &st.st_mtimespec, sizeof(mtime)) != 0) {
		mtime = st.st_mtimespec;
		seq++;
		return 1;
	}
	mutex_unlock(&check_mutex);

	return 0;
}

res_state
__res_check(res_state r)
{
	int uninit = (r->options & RES_INIT) == 0;

	if (check == 0)
		check = getenv("RES_CHECK") != NULL;

	if (check && mtime.tv_sec == 0)
		checktime();
	
	if (uninit || (check && (r->seq != seq || checktime()))) {
		if (!uninit)
			res_ndestroy(r);
		if (res_ninit(r) == -1) {
			h_errno = NETDB_INTERNAL;
			return NULL;
		}
		r->seq = seq;
	}
	return r;
}

void
res_watch(int onoff)
{
	check = onoff;
}

res_state
__res_get_state_nothread(void)
{
	return __res_check(&_nres);
}

void
/*ARGSUSED*/
__res_put_state_nothread(res_state res)
{
}

/*	$NetBSD: throttle.c,v 1.6 2009/07/03 06:41:47 tteras Exp $	*/

/* Id: throttle.c,v 1.5 2006/04/05 20:54:50 manubsd Exp */

/*
 * Copyright (C) 2004 Emmanuel Dreyfus
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <resolv.h>

#include "vmbuf.h"
#include "misc.h"
#include "plog.h"
#include "throttle.h"
#include "sockmisc.h"
#include "isakmp_var.h"
#include "isakmp.h"
#include "isakmp_xauth.h"
#include "isakmp_cfg.h"
#include "gcmalloc.h"

static struct throttle_list throttle_list =
	TAILQ_HEAD_INITIALIZER(throttle_list);

struct throttle_entry *
throttle_add(addr)
	struct sockaddr *addr;
{
	struct throttle_entry *te;
	struct timeval now, penalty;
	size_t len;

	len = sizeof(*te) 
	    - sizeof(struct sockaddr_storage) 
	    + sysdep_sa_len(addr);

	if ((te = racoon_malloc(len)) == NULL)
		return NULL;

	sched_get_monotonic_time(&now);
	penalty.tv_sec = isakmp_cfg_config.auth_throttle;
	penalty.tv_usec = 0;
	timeradd(&now, &penalty, &te->penalty_ends);

	memcpy(&te->host, addr, sysdep_sa_len(addr));
	TAILQ_INSERT_HEAD(&throttle_list, te, next);

	return te;
}

int
throttle_host(addr, authfail) 
	struct sockaddr *addr;
	int authfail;
{
	struct throttle_entry *te;
	struct timeval now, res;
	int found = 0;

	if (isakmp_cfg_config.auth_throttle == 0)
		return 0;

	sched_get_monotonic_time(&now);
restart:
	RACOON_TAILQ_FOREACH_REVERSE(te, &throttle_list, throttle_list, next) {
		/*
		 * Remove outdated entries
		 */
		if (timercmp(&te->penalty_ends, &now, <)) {
			TAILQ_REMOVE(&throttle_list, te, next);
			racoon_free(te);
			goto restart;
		}

		if (cmpsaddr(addr, (struct sockaddr *) &te->host) == 0) {
			found = 1;
			break;
		}
	}

	/* 
	 * No match, if auth failed, allocate a new throttle entry
	 * give no penalty even on error: this is the first time
	 * and we are indulgent.
	 */
	if (!found) {
		if (authfail) {
			if ((te = throttle_add(addr)) == NULL) {
				plog(LLV_ERROR, LOCATION, NULL, 
				    "Throttle insertion failed\n");
				return isakmp_cfg_config.auth_throttle;
			}
		}
		return 0;
	} else {
		/*
		 * We had a match and auth failed, increase penalty.
		 */
		if (authfail) {
			struct timeval remaining, penalty;

			timersub(&te->penalty_ends, &now, &remaining);
			penalty.tv_sec = isakmp_cfg_config.auth_throttle;
			penalty.tv_usec = 0;
			timeradd(&penalty, &remaining, &res);
			if (res.tv_sec >= THROTTLE_PENALTY_MAX) {
				res.tv_sec = THROTTLE_PENALTY_MAX;
				res.tv_usec = 0;
			}
			timeradd(&now, &res, &te->penalty_ends);
		}
	}

	timersub(&te->penalty_ends, &now, &res);
	return res.tv_sec;
}


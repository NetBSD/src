/*	$NetBSD: evt.c,v 1.5 2006/09/09 16:22:09 manu Exp $	*/

/* Id: evt.c,v 1.5 2006/06/22 20:11:35 manubsd Exp */

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

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/socket.h>

#include "vmbuf.h"
#include "plog.h"
#include "misc.h"
#include "admin.h"
#include "gcmalloc.h"
#include "evt.h"

#ifdef ENABLE_ADMINPORT
struct evtlist evtlist = TAILQ_HEAD_INITIALIZER(evtlist);
int evtlist_len = 0;

void
evt_push(src, dst, type, optdata)
	struct sockaddr *src;
	struct sockaddr *dst;
	int type;
	vchar_t *optdata;
{
	struct evtdump *evtdump;
	struct evt *evt;
	size_t len;

	/* If admin socket is disabled, silently discard anything */
	if (adminsock_path == NULL)
		return;

	/* If we are above the limit, don't record anything */
	if (evtlist_len > EVTLIST_MAX) {
		plog(LLV_DEBUG, LOCATION, NULL, 
		    "Cannot record event: event queue overflowed\n");
		return;
	}

	/* If we hit the limit, record an overflow event instead */
	if (evtlist_len == EVTLIST_MAX) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Cannot record event: event queue overflow\n");
		src = NULL;
		dst = NULL;
		type = EVTT_OVERFLOW;
		optdata = NULL;
	}

	len = sizeof(*evtdump);
	if (optdata)
		len += optdata->l;

	if ((evtdump = racoon_malloc(len)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, "Cannot record event: %s\n",
		    strerror(errno));
		return;
	}

	if ((evt = racoon_malloc(sizeof(*evt))) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, "Cannot record event: %s\n",
		    strerror(errno));
		racoon_free(evtdump);
		return;
	}

	if (src)
		memcpy(&evtdump->src, src, sysdep_sa_len(src));
	if (dst)
		memcpy(&evtdump->dst, dst, sysdep_sa_len(dst));
	evtdump->len = len;
	evtdump->type = type;
	time(&evtdump->timestamp);

	if (optdata)
		memcpy(evtdump + 1, optdata->v, optdata->l);

	evt->dump = evtdump;
	TAILQ_INSERT_TAIL(&evtlist, evt, next);

	evtlist_len++;

	return;
}

struct evtdump *
evt_pop(void) {
	struct evtdump *evtdump;
	struct evt *evt;

	if ((evt = TAILQ_FIRST(&evtlist)) == NULL)
		return NULL;

	evtdump = evt->dump;
	TAILQ_REMOVE(&evtlist, evt, next);
	racoon_free(evt);
	evtlist_len--;

	return evtdump;
}

vchar_t *
evt_dump(void) {
	struct evtdump *evtdump;
	vchar_t *buf = NULL;

	if ((evtdump = evt_pop()) != NULL) {
		if ((buf = vmalloc(evtdump->len)) == NULL) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "evt_dump failed: %s\n", strerror(errno));
			return NULL;
		}
		memcpy(buf->v, evtdump, evtdump->len);	
		racoon_free(evtdump);
	}

	return buf;
}

#endif /* ENABLE_ADMINPORT */

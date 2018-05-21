/*	$NetBSD: evt.c,v 1.10.48.1 2018/05/21 04:35:49 pgoyette Exp $	*/

/* Id: evt.c,v 1.5 2006/06/22 20:11:35 manubsd Exp */

/*
 * Copyright (C) 2004 Emmanuel Dreyfus
 * Copyright (C) 2008 Timo Teras
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
#include "handler.h"
#include "session.h"
#include "gcmalloc.h"
#include "evt.h"
#include "var.h"

#ifdef ENABLE_ADMINPORT

static EVT_LISTENER_LIST(evt_listeners);

struct evt_message {
	struct admin_com adm;
	struct evt_async evt;
};

struct evt {
	struct evtdump *dump;
	TAILQ_ENTRY(evt) next;
};

TAILQ_HEAD(evtlist, evt);

#define EVTLIST_MAX	32

static struct evtlist evtlist = TAILQ_HEAD_INITIALIZER(evtlist);
static int evtlist_len = 0;
static int evtlist_inuse = 0;

static struct {
	int newtype, oldtype;
} evttype_map[] = {
	{ EVT_RACOON_QUIT,		EVTT_RACOON_QUIT },
	{ EVT_PHASE1_UP,		EVTT_PHASE1_UP },
	{ EVT_PHASE1_DOWN,		EVTT_PHASE1_DOWN },
	{ EVT_PHASE1_NO_RESPONSE,	EVTT_PEER_NO_RESPONSE },
	{ EVT_PHASE1_NO_PROPOSAL,	EVTT_PEERPH1_NOPROP },
	{ EVT_PHASE1_AUTH_FAILED,	EVTT_PEERPH1AUTH_FAILED },
	{ EVT_PHASE1_DPD_TIMEOUT,	EVTT_DPD_TIMEOUT },
	{ EVT_PHASE1_PEER_DELETED,	EVTT_PEER_DELETE },
	{ EVT_PHASE1_MODE_CFG,		EVTT_ISAKMP_CFG_DONE },
	{ EVT_PHASE1_XAUTH_SUCCESS,	EVTT_XAUTH_SUCCESS },
	{ EVT_PHASE1_XAUTH_FAILED,	EVTT_XAUTH_FAILED },
	{ EVT_PHASE2_NO_PHASE1,		-1 },
	{ EVT_PHASE2_UP,		EVTT_PHASE2_UP },
	{ EVT_PHASE2_DOWN,		EVTT_PHASE2_DOWN },
	{ EVT_PHASE2_NO_RESPONSE,	EVTT_PEER_NO_RESPONSE },
};

static void
evt_push(src, dst, type, optdata)
	struct sockaddr *src;
	struct sockaddr *dst;
	int type;
	vchar_t *optdata;
{
	struct evtdump *evtdump;
	struct evt *evt;
	size_t len;
	int i;

	/* If admin socket is disabled, silently discard anything */
	if (adminsock_path == NULL || !evtlist_inuse)
		return;

	/* Map the event type to old */
	for (i = 0; i < sizeof(evttype_map) / sizeof(evttype_map[0]); i++)
		if (evttype_map[i].newtype == type)
			break;
	if (i >= sizeof(evttype_map) / sizeof(evttype_map[0]))
		return;

	type = evttype_map[i].oldtype;
	if (type < 0)
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

static struct evtdump *
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

	if (!evtlist_inuse) {
		evtlist_inuse = 1;
		plog(LLV_ERROR, LOCATION, NULL,
		     "evt_dump: deprecated event polling used\n");
	}

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

static struct evt_message *
evtmsg_create(int type, vchar_t *optdata)
{
	struct evt_message *e;
	size_t len;

	len = sizeof(struct evt_message);
	if (optdata != NULL)
		len += optdata->l;

	if ((e = racoon_malloc(len)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, "Cannot allocate event: %s\n",
		     strerror(errno));
		return NULL;
	}

	memset(e, 0, sizeof(struct evt_message));
	e->adm.ac_len = len;
	e->adm.ac_cmd = ADMIN_SHOW_EVT;
	e->adm.ac_errno = 0;
	e->adm.ac_proto = 0;
	e->evt.ec_type = type;
	time(&e->evt.ec_timestamp);
	if (optdata != NULL)
		memcpy(e + 1, optdata->v, optdata->l);

	return e;
}

static void
evt_unsubscribe(struct evt_listener *l)
{
	plog(LLV_DEBUG, LOCATION, NULL,
	     "[%d] admin connection released\n", l->fd);

	LIST_REMOVE(l, ll_chain);
	unmonitor_fd(l->fd);
	close(l->fd);
	racoon_free(l);
}

static int
evt_unsubscribe_cb(void *ctx, int fd)
{
	evt_unsubscribe((struct evt_listener *) ctx);
	return 0;
}

static void
evtmsg_broadcast(const struct evt_listener_list *ll, struct evt_message *e)
{
	struct evt_listener *l, *nl;

	for (l = LIST_FIRST(ll); l != NULL; l = nl) {
		nl = LIST_NEXT(l, ll_chain);

		if (send(l->fd, e, e->adm.ac_len, MSG_DONTWAIT) < 0) {
			plog(LLV_DEBUG, LOCATION, NULL, "Cannot send event to fd: %s\n",
				strerror(errno));
			evt_unsubscribe(l);
		}
	}
}

void
evt_generic(type, optdata)
	int type;
	vchar_t *optdata;
{
	struct evt_message *e;

	if ((e = evtmsg_create(type, optdata)) == NULL)
		return;

	evtmsg_broadcast(&evt_listeners, e);
	evt_push(&e->evt.ec_ph1src, &e->evt.ec_ph1dst, type, optdata);

	racoon_free(e);
}

void
evt_phase1(ph1, type, optdata)
	const struct ph1handle *ph1;
	int type;
	vchar_t *optdata;
{
	struct evt_message *e;

	if ((e = evtmsg_create(type, optdata)) == NULL)
		return;

	if (ph1->local)
		memcpy(&e->evt.ec_ph1src, ph1->local, sysdep_sa_len(ph1->local));
	if (ph1->remote)
		memcpy(&e->evt.ec_ph1dst, ph1->remote, sysdep_sa_len(ph1->remote));

	evtmsg_broadcast(&ph1->evt_listeners, e);
	evtmsg_broadcast(&evt_listeners, e);
	evt_push(&e->evt.ec_ph1src, &e->evt.ec_ph1dst, type, optdata);

	racoon_free(e);
}

void
evt_phase2(ph2, type, optdata)
	const struct ph2handle *ph2;
	int type;
	vchar_t *optdata;
{
	struct evt_message *e;
	struct ph1handle *ph1 = ph2->ph1;

	if ((e = evtmsg_create(type, optdata)) == NULL)
		return;

	if (ph1) {
		if (ph1->local)
			memcpy(&e->evt.ec_ph1src, ph1->local, sysdep_sa_len(ph1->local));
		if (ph1->remote)
			memcpy(&e->evt.ec_ph1dst, ph1->remote, sysdep_sa_len(ph1->remote));
	}
	e->evt.ec_ph2msgid = ph2->msgid;

	evtmsg_broadcast(&ph2->evt_listeners, e);
	if (ph1)
		evtmsg_broadcast(&ph1->evt_listeners, e);
	evtmsg_broadcast(&evt_listeners, e);
	evt_push(&e->evt.ec_ph1src, &e->evt.ec_ph1dst, type, optdata);

	racoon_free(e);
}

int
evt_subscribe(list, fd)
	struct evt_listener_list *list;
	int fd;
{
	struct evt_listener *l;

	if ((l = racoon_malloc(sizeof(*l))) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
		     "Cannot allocate event listener: %s\n",
		     strerror(errno));
		return errno;
	}

	if (list == NULL)
		list = &evt_listeners;

	LIST_INSERT_HEAD(list, l, ll_chain);
	l->fd = fd;
	monitor_fd(l->fd, evt_unsubscribe_cb, l, 0);

	plog(LLV_DEBUG, LOCATION, NULL,
	     "[%d] admin connection is polling events\n", fd);

	return -2;
}

void
evt_list_init(list)
	struct evt_listener_list *list;
{
	LIST_INIT(list);
}

void
evt_list_cleanup(list)
	struct evt_listener_list *list;
{
	while (!LIST_EMPTY(list))
		evt_unsubscribe(LIST_FIRST(list));
}

#endif /* ENABLE_ADMINPORT */

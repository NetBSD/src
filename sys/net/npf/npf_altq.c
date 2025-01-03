/* NetBSD */
/*
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Nyarko.
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

#ifdef _KERNEL_OPT
#include "opt_altq.h"
#include "opt_inet.h"
#endif

#include <sys/unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mbuf.h>
#include <sys/queue.h>
#include <sys/tree.h>
#include <sys/rwlock.h>
#include <sys/pool.h>
#include <sys/malloc.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>
#include <altq/altq.h>
#include "npf_altq.h"
#include "npf.h"

#ifdef ALTQ

/*
 * starting point of altq kernel routines
 */
bool	 npf_altq_running = false;
uint32_t		 nticket_altqs_active;
uint32_t		 nticket_altqs_inactive;
struct npf_altqqueue	*npf_altqs_active;
struct npf_altqqueue	*npf_altqs_inactive;
struct npf_altqqueue  npf_altqs[2];

struct pool		 npf_altq_pl;
int npf_altq_loaded = 0;

TAILQ_HEAD(npf_tags, npf_tagname)	npf_tags = TAILQ_HEAD_INITIALIZER(npf_tags),
				npf_qids = TAILQ_HEAD_INITIALIZER(npf_qids);
void tag_unref(struct npf_tags *, u_int16_t);
uint16_t npftagname2tag(struct npf_tags *, char *);

#if (NPF_QNAME_SIZE != NPF_TAG_NAME_SIZE)
#error NPF_QNAME_SIZE must be equal to NPF_TAG_NAME_SIZE
#endif

/* npf interface to start altq */
void
npf_altq_init(void)
{
	pool_init(&npf_altq_pl, sizeof(struct npf_altq), 0, 0, 0, "npfaltqpl",
	    &pool_allocator_nointr, IPL_NONE);
	TAILQ_INIT(&npf_altqs[0]);
	TAILQ_INIT(&npf_altqs[1]);
	npf_altqs_active = &npf_altqs[0];
	npf_altqs_inactive = &npf_altqs[1];
}

int
npf_begin_altq(void)
{
	struct npf_altq	*altq;
	int		 error = 0;
	/* Purge the old altq list */
	while ((altq = TAILQ_FIRST(npf_altqs_inactive)) != NULL) {
		TAILQ_REMOVE(npf_altqs_inactive, altq, entries);
		if (altq->qname[0] == 0) {
			/* detach and destroy the discipline */
			if ((error = altq_remove(altq)) != 0)
				return error;
		} else
			npf_qid_unref(altq->qid);
		pool_put(&npf_altq_pl, altq);
	}

	return 0;
}

int
npf_get_altqs(void *data)
{
	struct npfioc_altq	*paa = (struct npfioc_altq *)data;
	struct npf_altq		*altq;
	paa->nr = 0;
	TAILQ_FOREACH(altq, npf_altqs_active, entries)
		paa->nr++;
	paa->ticket = nticket_altqs_active;
	return 0 ;
}

int
npf_add_altq(void *data)
{
	struct npfioc_altq	*paa = (struct npfioc_altq *)data;
	struct npf_altq		*altq, *a;
	int error;

	altq = pool_get(&npf_altq_pl, PR_NOWAIT);
	if (altq == NULL) {
		error = ENOMEM;
		return error;
	}
	memcpy(altq, &paa->altq, sizeof(*altq));
	/*
		* if this is for a queue, find the discipline and
		* copy the necessary fields
		*/
	if (altq->qname[0] != 0) {
		if ((altq->qid = npf_qname2qid(altq->qname)) == 0) {
			error = EBUSY;
			pool_put(&npf_altq_pl, altq);
			return error;
		}
		TAILQ_FOREACH(a, npf_altqs_inactive, entries) {
			if (strncmp(a->ifname, altq->ifname,
				IFNAMSIZ) == 0 && a->qname[0] == 0) {
				altq->altq_disc = a->altq_disc;
				break;
			}
		}
	}
	error = altq_add(altq);
	if (error) {
		pool_put(&npf_altq_pl, altq);
		return error;
	}
	TAILQ_INSERT_TAIL(npf_altqs_inactive, altq, entries);
	memcpy(&paa->altq, altq, sizeof(paa->altq));

	if (!npf_altq_loaded)
		npf_altq_loaded = 1;
	return 0;
}

int
npf_commit_altq(void)
{
	struct npf_altqqueue	*old_altqs;
	struct npf_altq		*altq;
	int			 s, err, error = 0;

	/* swap altqs, keep the old. */
	s = splsoftnet();
	old_altqs = npf_altqs_active;
	npf_altqs_active = npf_altqs_inactive;
	npf_altqs_inactive = old_altqs;
	nticket_altqs_active = nticket_altqs_inactive;
	/* Attach new disciplines */
	TAILQ_FOREACH(altq, npf_altqs_active, entries) {
		if (altq->qname[0] == 0) {
			/* attach the discipline */
			error = altq_npfattach(altq);
			if (error == 0 && !npf_altq_running)
				error = npf_enable_altq(altq);
			if (error != 0) {
				splx(s);
				return error;
			}
		}
	}
	/* Purge the old altq list */
	while ((altq = TAILQ_FIRST(npf_altqs_inactive)) != NULL) {
		TAILQ_REMOVE(npf_altqs_inactive, altq, entries);
		if (altq->qname[0] == 0) {
			/* detach and destroy the discipline */
			if (npf_altq_running)
				error = npf_disable_altq(altq);
			err = altq_npfdetach(altq);
			if (err != 0 && error == 0)
				error = err;
			err = altq_remove(altq);
			if (err != 0 && error == 0)
				error = err;
		} else
			npf_qid_unref(altq->qid);
		pool_put(&npf_altq_pl, altq);
	}
	splx(s);
	return error;
}

u_int32_t
npf_qname2qid(char *qname)
{
	return ((u_int32_t)npftagname2tag(&npf_qids, qname));
}

u_int16_t
npftagname2tag(struct npf_tags *head, char *tagname)
{
	struct npf_tagname	*tag, *p = NULL;
	u_int16_t		 new_tagid = 1;
	TAILQ_FOREACH(tag, head, entries)
		if (strcmp(tagname, tag->name) == 0) {
			tag->ref++;
			return (tag->tag);
		}
	/*
	 * to avoid fragmentation, we do a linear search from the beginning
	 * and take the first free slot we find. if there is none or the list
	 * is empty, append a new entry at the end.
	 */
	/* new entry */
	if (!TAILQ_EMPTY(head))
		for (p = TAILQ_FIRST(head); p != NULL &&
		    p->tag == new_tagid; p = TAILQ_NEXT(p, entries))
			new_tagid = p->tag + 1;
	if (new_tagid > TAGID_MAX)
		return 0;
	/* allocate and fill new struct npf_tagname */
	tag = malloc(sizeof(*tag),
	    M_TEMP, M_NOWAIT);
	if (tag == NULL)
		return 0;
	memset(tag, 0, sizeof(*tag));
	strlcpy(tag->name, tagname, sizeof(tag->name));
	tag->tag = new_tagid;
	tag->ref++;
	if (p != NULL)	/* insert new entry before p */
		TAILQ_INSERT_BEFORE(p, tag, entries);
	else	/* either list empty or no free slot in between */
		TAILQ_INSERT_TAIL(head, tag, entries);
	return (tag->tag);
}

void
tag_unref(struct npf_tags *head, u_int16_t tag)
{
	struct npf_tagname	*p, *next;
	if (tag == 0)
		return;
	for (p = TAILQ_FIRST(head); p != NULL; p = next) {
		next = TAILQ_NEXT(p, entries);
		if (tag == p->tag) {
			if (--p->ref == 0) {
				TAILQ_REMOVE(head, p, entries);
				free(p, M_TEMP);
			}
			break;
		}
	}
}

void
npf_qid_unref(u_int32_t qid)
{
	tag_unref(&npf_qids, (u_int16_t)qid);
}

/* disable, destroy and stop altq routine when packet filtering disabled */
int
npf_altq_destroy(void)
{
	//u_int32_t		 ticket;
	if (npf_begin_altq() == 0)
		npf_commit_altq();
	if (npf_altq_loaded) {
		pool_destroy(&npf_altq_pl);
		npf_altq_loaded = 0;
	}
	return 0;
}

int
npf_altq_start(void)
{
	int error;
    struct npf_altq		*altq;
    /* enable all altq interfaces on active list */
    TAILQ_FOREACH(altq, npf_altqs_active, entries) {
        if (altq->qname[0] == 0) {
            error = npf_enable_altq(altq);
            if (error != 0)
                break;
        }
    }

	return error;
}

int
npf_enable_altq(struct npf_altq *altq)
{
	struct ifnet		*ifp;
	struct tb_profile	 tb;
	int			 s, error = 0;
	if ((ifp = ifunit(altq->ifname)) == NULL)
		return EINVAL;
	if (ifp->if_snd.altq_type != ALTQT_NONE)
		error = altq_enable(&ifp->if_snd);
	/* set tokenbucket regulator */
	if (error == 0 && ifp != NULL && ALTQ_IS_ENABLED(&ifp->if_snd)) {
		tb.rate = altq->ifbandwidth;
		tb.depth = altq->tbrsize;
		s = splnet();
		error = tbr_set(&ifp->if_snd, &tb);
		splx(s);
	}
	if (error == 0)
		npf_altq_running = true;
	return error;
}

int
npf_stop_altq(void)
{
		struct npf_altq		*altq;
		int error;
	/* disable all altq interfaces on active list */
	TAILQ_FOREACH(altq, npf_altqs_active, entries) {
		if (altq->qname[0] == 0) {
			error = npf_disable_altq(altq);
			if (error != 0)
				break;
		}
	}
	//DPFPRINTF(PF_DEBUG_MISC, ("altq: stopped\n"));
	return error;
}

int
npf_disable_altq(struct npf_altq *altq)
{
	struct ifnet		*ifp;
	struct tb_profile	 tb;
	int			 s, error;
	if ((ifp = ifunit(altq->ifname)) == NULL)
		return EINVAL;
	/*
	 * when the discipline is no longer referenced, it was overridden
	 * by a new one.  if so, just return.
	 */
	if (altq->altq_disc != ifp->if_snd.altq_disc)
		return 0;
	error = altq_disable(&ifp->if_snd);
	if (error == 0) {
		/* clear tokenbucket regulator */
		tb.rate = 0;
		s = splnet();
		error = tbr_set(&ifp->if_snd, &tb);
		splx(s);
	}
	if (error == 0)
		npf_altq_running = 0;
	return error;
}

int
npf_get_qstats(void *data)
{
	int error;
	struct npfioc_qstats	*pq = (struct npfioc_qstats *)data;
	struct npf_altq		*altq;
	u_int32_t		 nr;
	int			 nbytes;

	nbytes = pq->nbytes;
	nr = 0;
	altq = TAILQ_FIRST(npf_altqs_active);
	while ((altq != NULL) && (nr < pq->nr)) {
		altq = TAILQ_NEXT(altq, entries);
		nr++;
	}
	if (altq == NULL) {
		error = EBUSY;
		return error;
	}
	error = altq_getqstats(altq, pq->buf, &nbytes);
	if (error == 0) {
		pq->scheduler = altq->scheduler;
		pq->nbytes = nbytes;
	}
	return error;
}

int
npf_get_altq(void *data)
{
	int error = 0;
	struct npfioc_altq	*paa = (struct npfioc_altq *)data;
	struct npf_altq		*altq;
	u_int32_t		 nr;

	nr = 0;
	altq = TAILQ_FIRST(npf_altqs_active);
	while ((altq != NULL) && (nr < paa->nr)) {
		altq = TAILQ_NEXT(altq, entries);
		nr++;
	}
	if (altq == NULL) {
		error = EBUSY;
		return error;
	}

	memcpy(&paa->altq, altq, sizeof(struct npf_altq));
	return error;
}

#endif /*ALTQ */

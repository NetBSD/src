/*	$NetBSD: sainfo.c,v 1.12 2010/01/17 23:03:01 wiz Exp $	*/

/*	$KAME: sainfo.c,v 1.16 2003/06/27 07:32:39 sakane Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <netinet/in.h>
#include <netinet/in.h> 
#include PATH_IPSEC_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "sockmisc.h"
#include "debug.h"

#include "localconf.h"
#include "isakmp_var.h"
#include "isakmp.h"
#include "ipsec_doi.h"
#include "oakley.h"
#include "handler.h"
#include "algorithm.h"
#include "sainfo.h"
#include "gcmalloc.h"

static LIST_HEAD(_sitree, sainfo) sitree, sitree_save, sitree_tmp;

/* %%%
 * modules for ipsec sa info
 */
/*
 * return matching entry.
 * no matching entry found and if there is anonymous entry, return it.
 * else return NULL.
 * First pass is for sainfo from a specified peer, second for others.
 */
struct sainfo *
getsainfo(loc, rmt, peer, client, remoteid)
	const vchar_t *loc, *rmt, *peer, *client;
	int remoteid;
{
	struct sainfo *s = NULL;

	/* debug level output */
	if(loglevel >= LLV_DEBUG) {
		char *dloc, *drmt, *dpeer, *dclient;

		if (loc == NULL)
			dloc = strdup("ANONYMOUS");
		else
			dloc = ipsecdoi_id2str(loc);

		if (rmt == SAINFO_ANONYMOUS)
			drmt = strdup("ANONYMOUS");
		else if (rmt == SAINFO_CLIENTADDR)
			drmt = strdup("CLIENTADDR");
		else
			drmt = ipsecdoi_id2str(rmt);

		if (peer == NULL)
			dpeer = strdup("NULL");
		else
			dpeer = ipsecdoi_id2str(peer);

		if (client == NULL)
			dclient = strdup("NULL");
		else
			dclient = ipsecdoi_id2str(client);

		plog(LLV_DEBUG, LOCATION, NULL,
			"getsainfo params: loc=\'%s\' rmt=\'%s\' peer=\'%s\' client=\'%s\' id=%i\n",
			dloc, drmt, dpeer, dclient, remoteid );
 
                racoon_free(dloc);
                racoon_free(drmt);
                racoon_free(dpeer);
                racoon_free(dclient);
	}

	LIST_FOREACH(s, &sitree, chain) {
		const char *sainfostr = sainfo2str(s);
		plog(LLV_DEBUG, LOCATION, NULL,
			"evaluating sainfo: %s\n", sainfostr);

		if(s->remoteid != remoteid) {
			plog(LLV_DEBUG, LOCATION, NULL,
				"remoteid mismatch: %i != %i\n",
				s->remoteid, remoteid);
				continue;
		}

		/* compare 'from' id value */
		if (s->id_i != NULL)
			if (ipsecdoi_chkcmpids(peer, s->id_i, 0))
				continue;

		/* compare ids - client */
		if( s->iddst == SAINFO_CLIENTADDR ) {
			/*
			 * This sainfo section enforces client address
			 * checking. Prevent match if the client value
			 * ( modecfg or tunnel address ) is NULL.
			 */

			if (client == NULL)
				continue;

			if( rmt == SAINFO_CLIENTADDR ) {
				/*
				 * In the case where a supplied rmt value is
				 * also SAINFO_CLIENTADDR, we are comparing
				 * with another sainfo to check for duplicate.
				 * Only compare the local values to determine
				 * a match.
				 */

				 if (!ipsecdoi_chkcmpids(loc, s->idsrc, 0))
					return s;
			}
			else {
				/*
				 * In the case where a supplied rmt value is
				 * not SAINFO_CLIENTADDR, do a standard match
				 * for local values and enforce that the rmt
				 * id matches the client address value.
				 */

				if (!ipsecdoi_chkcmpids(loc, s->idsrc, 0) &&
				    !ipsecdoi_chkcmpids(rmt, client, 0))
					return s;
			}

			continue;
		}


		/* compare ids - standard */
		if (!ipsecdoi_chkcmpids(loc, s->idsrc, 0) &&
		    !ipsecdoi_chkcmpids(rmt, s->iddst, 0))
			return s;
	}

	return NULL;
}

struct sainfo *
newsainfo()
{
	struct sainfo *new;

	new = racoon_calloc(1, sizeof(*new));
	if (new == NULL)
		return NULL;

	new->lifetime = IPSECDOI_ATTR_SA_LD_SEC_DEFAULT;
	new->lifebyte = IPSECDOI_ATTR_SA_LD_KB_MAX;

	return new;
}

void
delsainfo(si)
	struct sainfo *si;
{
	int i;

	for (i = 0; i < MAXALGCLASS; i++)
		delsainfoalg(si->algs[i]);

	if (si->idsrc)
		vfree(si->idsrc);
	if (si->iddst != NULL &&
		si->iddst != SAINFO_CLIENTADDR)
		vfree(si->iddst);

#ifdef ENABLE_HYBRID
	if (si->group)
		vfree(si->group);
#endif

	racoon_free(si);
}

int prisainfo(s)
	struct sainfo *s;
{
	/*
	 * determine the matching priority
	 * of an sainfo section
	 */

	int pri = 0;

	if(s->remoteid)
		pri += 3;

	if(s->id_i)
		pri += 3;

	if(s->idsrc)
		pri++;

	if(s->iddst)
		pri++;

	return pri;
}

void
inssainfo(new)
	struct sainfo *new;
{
	if(LIST_EMPTY(&sitree)) {

		/* first in list */
		LIST_INSERT_HEAD(&sitree, new, chain);
	}
	else {
		int npri, spri;
		struct sainfo *s, *n;

		/*
		 * insert our new sainfo section
		 * into our list which is sorted
		 * based on the match priority
		 */

		npri = prisainfo(new);

		s = LIST_FIRST(&sitree);
		while (1) {

			spri = prisainfo(s);
			n = LIST_NEXT(s, chain);

			if(npri > spri)
			{
				/* higher priority */
				LIST_INSERT_BEFORE(s, new, chain);
				return;
			}

			if(n == NULL)
			{
				/* last in list */
				LIST_INSERT_AFTER(s, new, chain);
				return;
			}

			s = n;
		}
	}
}

void
remsainfo(si)
	struct sainfo *si;
{
	LIST_REMOVE(si, chain);
}

void
flushsainfo()
{
	struct sainfo *s, *next;

	for (s = LIST_FIRST(&sitree); s; s = next) {
		next = LIST_NEXT(s, chain);
		remsainfo(s);
		delsainfo(s);
	}
}

void
initsainfo()
{
	LIST_INIT(&sitree);
}

struct sainfoalg *
newsainfoalg()
{
	struct sainfoalg *new;

	new = racoon_calloc(1, sizeof(*new));
	if (new == NULL)
		return NULL;

	return new;
}

void
delsainfoalg(alg)
	struct sainfoalg *alg;
{
	struct sainfoalg *a, *next;

	for (a = alg; a; a = next) {
		next = a->next;
		racoon_free(a);
	}
}

void
inssainfoalg(head, new)
	struct sainfoalg **head;
	struct sainfoalg *new;
{
	struct sainfoalg *a;

	for (a = *head; a && a->next; a = a->next)
		;
	if (a)
		a->next = new;
	else
		*head = new;
}

const char *
sainfo2str(si)
	const struct sainfo *si;
{
        static char buf[256];

        char *idloc = NULL, *idrmt = NULL, *id_i;
 
        if (si->idsrc == SAINFO_ANONYMOUS)
                idloc = strdup("ANONYMOUS");
        else
                idloc = ipsecdoi_id2str(si->idsrc);
 
        if (si->iddst == SAINFO_ANONYMOUS)
                idrmt = strdup("ANONYMOUS");
	else if (si->iddst == SAINFO_CLIENTADDR)
                idrmt = strdup("CLIENTADDR");
        else
                idrmt = ipsecdoi_id2str(si->iddst);
 
        if (si->id_i == NULL)
                id_i = strdup("ANY");
        else
                id_i = ipsecdoi_id2str(si->id_i);
 
        snprintf(buf, 255, "loc=\'%s\', rmt=\'%s\', peer=\'%s\', id=%i",
		idloc, idrmt, id_i, si->remoteid);
 
        racoon_free(idloc);
        racoon_free(idrmt);
        racoon_free(id_i);
 
        return buf;
}

void save_sainfotree(void){
	sitree_save=sitree;
	initsainfo();
}

void save_sainfotree_flush(void){
	sitree_tmp=sitree;
	sitree=sitree_save;
	flushsainfo();
	sitree=sitree_tmp;
}

void save_sainfotree_restore(void){
	flushsainfo();
	sitree=sitree_save;
}

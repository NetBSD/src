/*	$KAME: sainfo.c,v 1.13 2000/12/15 13:43:57 sakane Exp $	*/

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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/queue.h>

#include <netkey/key_var.h>
#include <netinet/in.h>
#include <netinet6/ipsec.h>

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

static LIST_HEAD(_sitree, sainfo) sitree;

/* %%%
 * modules for ipsec sa info
 */
/*
 * return matching entry.
 * no matching entry found and if there is anonymous entry, return it.
 * else return NULL.
 * XXX by each data type, should be changed to compare the buffer.
 */
struct sainfo *
getsainfo(src, dst)
	const vchar_t *src, *dst;
{
	struct sainfo *s = NULL;
	struct sainfo *anonymous = NULL;

	LIST_FOREACH(s, &sitree, chain) {
		if (s->idsrc == NULL) {
			anonymous = s;
			continue;
		}

		/* anonymous ? */
		if (src == NULL) {
			if (anonymous != NULL)
				break;
			continue;
		}

		if (memcmp(src->v, s->idsrc->v, s->idsrc->l) == 0
		 && memcmp(dst->v, s->iddst->v, s->iddst->l) == 0)
			return s;
	}

	if (anonymous) {
		plog(LLV_DEBUG, LOCATION, NULL,
			"anonymous sainfo selected.\n");
	}
	return anonymous;
}

struct sainfo *
newsainfo()
{
	struct sainfo *new;

	new = CALLOC(sizeof(*new), struct sainfo *);
	if (new == NULL)
		return NULL;

	new->idvtype = IDTYPE_ADDRESS;
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
	if (si->iddst)
		vfree(si->iddst);

	free(si);
}

void
inssainfo(new)
	struct sainfo *new;
{
	LIST_INSERT_HEAD(&sitree, new, chain);
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

	new = CALLOC(sizeof(*new), struct sainfoalg *);
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
		free(a);
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

	if (si->idsrc == NULL)
		return "anonymous";

	snprintf(buf, sizeof(buf), "%s", ipsecdoi_id2str(si->idsrc));
	snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
		" %s", ipsecdoi_id2str(si->iddst));

	return buf;
}

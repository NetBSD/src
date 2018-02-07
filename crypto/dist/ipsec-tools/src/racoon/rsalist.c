/*	$NetBSD: rsalist.c,v 1.7 2018/02/07 03:59:03 christos Exp $	*/

/* Id: rsalist.c,v 1.3 2004/11/08 12:04:23 ludvigm Exp */

/*
 * Copyright (C) 2004 SuSE Linux AG, Nuernberg, Germany.
 * Contributed by: Michal Ludvig <mludvig@suse.cz>, SUSE Labs
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

#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <netdb.h>

#include <openssl/bn.h>
#include <openssl/rsa.h>

#include "misc.h"
#include "plog.h"
#include "sockmisc.h"
#include "rsalist.h"
#include "genlist.h"
#include "remoteconf.h"
#include "crypto_openssl.h"

#ifndef LIST_FIRST
#define LIST_FIRST(head)        ((head)->lh_first)
#endif

#ifndef LIST_NEXT
#define LIST_NEXT(elm, field)   ((elm)->field.le_next)
#endif

/* from prsa_tok.l */
int prsa_parse_file(struct genlist *list, const char *fname, enum rsa_key_type type);

int
rsa_key_insert(struct genlist *list, struct netaddr *src,
	       struct netaddr *dst, RSA *rsa)
{
	struct rsa_key *rsa_key;

	rsa_key = calloc(sizeof(struct rsa_key), 1);
	rsa_key->rsa = rsa;

	if (src)
		rsa_key->src = src;
	else
		rsa_key->src = calloc(sizeof(*rsa_key->src), 1);

	if (dst)
		rsa_key->dst = dst;
	else
		rsa_key->dst = calloc(sizeof(*rsa_key->dst), 1);

	genlist_append(list, rsa_key);

	return 0;
}

struct rsa_key *
rsa_key_dup(struct rsa_key *key)
{
	struct rsa_key *new;

	new = calloc(sizeof(struct rsa_key), 1);
	if (new == NULL)
		return NULL;

	if (key->rsa) {
		const BIGNUM *d;
		RSA_get0_key(key->rsa, NULL, NULL, &d);
		new->rsa = d != NULL ? RSAPrivateKey_dup(key->rsa) : RSAPublicKey_dup(key->rsa);
		if (new->rsa == NULL)
			goto dup_error;
	}

	if (key->src) {
		new->src = malloc(sizeof(*new->src));
		if (new->src == NULL)
			goto dup_error;
		memcpy(new->src, key->src, sizeof(*new->src));
	}	
	if (key->dst) {
		new->dst = malloc(sizeof(*new->dst));
		if (new->dst == NULL)
			goto dup_error;
		memcpy(new->dst, key->dst, sizeof(*new->dst));
	}

	return new;

dup_error:
	if (new->rsa != NULL)
		RSA_free(new->rsa);
	if (new->dst != NULL)
		free(new->dst);
	if (new->src != NULL)
		free(new->src);

	free(new);
	return NULL;
}

void
rsa_key_free(void *data)
{
	struct rsa_key *rsa_key;

	
	rsa_key = (struct rsa_key *)data;
	if (rsa_key->src)
		free(rsa_key->src);
	if (rsa_key->dst)
		free(rsa_key->dst);
	if (rsa_key->rsa)
		RSA_free(rsa_key->rsa);

	free(rsa_key);
}

static void *
rsa_key_dump_one(void *entry, void *arg)
{
	struct rsa_key *key = entry;

	plog(LLV_DEBUG, LOCATION, NULL, "Entry %s\n",
	     naddrwop2str_fromto("%s -> %s", key->src,
				 key->dst));
	if (loglevel > LLV_DEBUG)
		RSA_print_fp(stdout, key->rsa, 4);

	return NULL;
}

void
rsa_key_dump(struct genlist *list)
{
	genlist_foreach(list, rsa_key_dump_one, NULL);
}

static void *
rsa_list_count_one(void *entry, void *arg)
{
	if (arg)
		(*(unsigned long *)arg)++;
	return NULL;
}

unsigned long
rsa_list_count(struct genlist *list)
{
	unsigned long count = 0;
	genlist_foreach(list, rsa_list_count_one, &count);
	return count;
}

struct lookup_result {
	struct ph1handle *iph1;
	int max_score;
	struct genlist *winners;
};
	
static void *
rsa_lookup_key_one(void *entry, void *data)
{
	int local_score, remote_score;
	struct lookup_result *req = data;
	struct rsa_key *key = entry;

	local_score = naddr_score(key->src, req->iph1->local);
	remote_score = naddr_score(key->dst, req->iph1->remote);

	plog(LLV_DEBUG, LOCATION, NULL, "Entry %s scored %d/%d\n",
		naddrwop2str_fromto("%s -> %s", key->src, key->dst),
		local_score, remote_score);

	if (local_score >= 0 && remote_score >= 0) {
		if (local_score + remote_score > req->max_score) {
			req->max_score = local_score + remote_score;
//			genlist_free(req->winners, NULL);
		}

		if (local_score + remote_score >= req->max_score) {
			genlist_append(req->winners, key);
		}
	}

	/* Always traverse the whole list */
	return NULL;
}

struct genlist *
rsa_lookup_keys(struct ph1handle *iph1, int my)
{
	struct genlist *list;
	struct lookup_result r;

	plog(LLV_DEBUG, LOCATION, NULL, "Looking up RSA key for %s\n",
	     saddr2str_fromto("%s <-> %s", iph1->local, iph1->remote));

	r.iph1 = iph1;
	r.max_score = -1;
	r.winners = genlist_init();

	if (my)
		list = iph1->rmconf->rsa_private;
	else
		list = iph1->rmconf->rsa_public;

	genlist_foreach(list, rsa_lookup_key_one, &r);

	if (loglevel >= LLV_DEBUG)
		rsa_key_dump(r.winners);

	return r.winners;
}

int
rsa_parse_file(struct genlist *list, const char *fname, enum rsa_key_type type)
{
	int ret;
	
	plog(LLV_DEBUG, LOCATION, NULL, "Parsing %s\n", fname);
	ret = prsa_parse_file(list, fname, type);
	if (loglevel >= LLV_DEBUG)
		rsa_key_dump(list);
	return ret;
}

RSA *
rsa_try_check_rsasign(vchar_t *source, vchar_t *sig, struct genlist *list)
{
	struct rsa_key *key;
	struct genlist_entry *gp;

	for(key = genlist_next(list, &gp); key; key = genlist_next(NULL, &gp)) {
		plog(LLV_DEBUG, LOCATION, NULL, "Checking key %s...\n",
			naddrwop2str_fromto("%s -> %s", key->src, key->dst));
		if (eay_check_rsasign(source, sig, key->rsa) == 0) {
			plog(LLV_DEBUG, LOCATION, NULL, " ... YEAH!\n");
			return key->rsa;
		}
		plog(LLV_DEBUG, LOCATION, NULL, " ... nope.\n");
	}
	return NULL;
}

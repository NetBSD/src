/*	$NetBSD: rsalist.h,v 1.4.38.1 2011/03/05 15:08:33 bouyer Exp $	*/

/* Id: rsalist.h,v 1.2 2004/07/12 20:43:51 ludvigm Exp */
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

#ifndef _RSALIST_H
#define _RSALIST_H

#include <netinet/in.h>
#include <openssl/rsa.h>

#include "handler.h"
#include "genlist.h"

enum rsa_key_type {
	RSA_TYPE_ANY = 0,
	RSA_TYPE_PUBLIC,
	RSA_TYPE_PRIVATE
};

struct rsa_key {
	struct netaddr *src;
	struct netaddr *dst;
	RSA *rsa;
};

int rsa_key_insert(struct genlist *list, struct netaddr *src, struct netaddr *dst, RSA *rsa);
void rsa_key_free(void *data);
void rsa_key_dump(struct genlist *list);

struct genlist *rsa_lookup_keys(struct ph1handle *iph1, int my);
RSA *rsa_try_check_rsasign(vchar_t *source, vchar_t *sig, struct genlist *list);

unsigned long rsa_list_count(struct genlist *list);

int rsa_parse_file(struct genlist *list, const char *fname, enum rsa_key_type type);

#endif /* _RSALIST_H */

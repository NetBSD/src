/*	$NetBSD: dnssec.c,v 1.5 2009/03/12 10:57:26 tteras Exp $	*/

/*	$KAME: dnssec.c,v 1.2 2001/08/05 18:46:07 itojun Exp $	*/

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

#include <sys/types.h>
#include <sys/param.h>
#include <stdlib.h>
#include <string.h>

#include "var.h"
#include "vmbuf.h"
#include "misc.h"
#include "plog.h"
#include "debug.h"

#include "isakmp_var.h"
#include "isakmp.h"
#include "ipsec_doi.h"
#include "oakley.h"
#include "netdb_dnssec.h"
#include "strnames.h"
#include "dnssec.h"
#include "gcmalloc.h"

extern int h_errno;

vchar_t *
dnssec_getcert(id)
	vchar_t *id;
{
	vchar_t *cert = NULL;
	struct certinfo *res = NULL;
	struct ipsecdoi_id_b *id_b;
	int type;
	char *name = NULL;
	int namelen;
	int error;

	id_b = (struct ipsecdoi_id_b *)id->v;

	namelen = id->l - sizeof(*id_b);
	name = racoon_malloc(namelen + 1);
	if (!name) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get buffer.\n");
		return NULL;
	}
	memcpy(name, id_b + 1, namelen);
	name[namelen] = '\0';

	switch (id_b->type) {
	case IPSECDOI_ID_FQDN:
		error = getcertsbyname(name, &res);
		if (error != 0) {
			plog(LLV_ERROR, LOCATION, NULL,
				"getcertsbyname(\"%s\") failed.\n", name);
			goto err;
		}
		break;
	case IPSECDOI_ID_IPV4_ADDR:
	case IPSECDOI_ID_IPV6_ADDR:
		/* XXX should be processed to query PTR ? */
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"inpropper ID type passed %s "
			"though getcert method is dnssec.\n",
			s_ipsecdoi_ident(id_b->type));
		goto err;
	}

	/* check response */
	if (res->ci_next != NULL) {
		plog(LLV_WARNING, LOCATION, NULL,
			"not supported multiple CERT RR.\n");
	}
	switch (res->ci_type) {
	case DNSSEC_TYPE_PKIX:
		/* XXX is it enough condition to set this type ? */
		type = ISAKMP_CERT_X509SIGN;
		break;
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"not supported CERT RR type %d.\n", res->ci_type);
		goto err;
	}

	/* create cert holder */
	cert = vmalloc(res->ci_certlen + 1);
	if (cert == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get cert buffer.\n");
		goto err;
	}
	cert->v[0] = type;
	memcpy(&cert->v[1], res->ci_cert, res->ci_certlen);

	plog(LLV_DEBUG, LOCATION, NULL, "created CERT payload:\n");
	plogdump(LLV_DEBUG, cert->v, cert->l);

err:
	if (name)
		racoon_free(name);
	if (res)
		freecertinfo(res);
	return cert;
}

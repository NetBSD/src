/*	$KAME: gssapi.c,v 1.19 2001/04/03 15:51:55 thorpej Exp $	*/

/*
 * Copyright 2000 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * This software was written by Frank van der Linden of Wasabi Systems
 * for Zembu Labs, Inc. http://www.zembu.com/
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Wasabi Systems for
 *	Zembu Labs, Inc. http://www.zembu.com/
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_GSSAPI
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <netdb.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "sockmisc.h"
#include "schedule.h"
#include "debug.h"

#include "localconf.h"
#include "remoteconf.h"
#include "isakmp_var.h"
#include "isakmp.h"
#include "oakley.h"
#include "handler.h"
#include "ipsec_doi.h"
#include "crypto_openssl.h"
#include "pfkey.h"
#include "isakmp_ident.h"
#include "isakmp_inf.h"
#include "vendorid.h"
#include "gcmalloc.h"

#include "gssapi.h"

static void
gssapi_error(OM_uint32 status_code, const char *where,
	     const char *fmt, ...)
{
	OM_uint32 message_context, maj_stat, min_stat;
	gss_buffer_desc status_string;
	va_list ap;

	va_start(ap, fmt);
	plogv(LLV_ERROR, where, NULL, fmt, ap);
	va_end(ap);

	message_context = 0;

	do {
		maj_stat = gss_display_status(&min_stat, status_code,
		    GSS_C_MECH_CODE, GSS_C_NO_OID, &message_context,
		    &status_string);
		if (GSS_ERROR(maj_stat))
			plog(LLV_ERROR, LOCATION, NULL,
			    "UNABLE TO GET GSSAPI ERROR CODE\n");
		else {
			plog(LLV_ERROR, where, NULL,
			    "%s\n", status_string.value);
			gss_release_buffer(&min_stat, &status_string);
		}
	} while (message_context != 0);
}

/*
 * vmbufs and gss_buffer_descs are really just the same on NetBSD, but
 * this is to be portable.
 */
static int
gssapi_vm2gssbuf(vchar_t *vmbuf, gss_buffer_t gsstoken)
{

	gsstoken->value = racoon_malloc(vmbuf->l);
	if (gsstoken->value == NULL)
		return -1;
	memcpy(gsstoken->value, vmbuf->v, vmbuf->l);
	gsstoken->length = vmbuf->l;

	return 0;
}

static int
gssapi_gss2vmbuf(gss_buffer_t gsstoken, vchar_t **vmbuf)
{

	*vmbuf = vmalloc(gsstoken->length);
	if (*vmbuf == NULL)
		return -1;
	memcpy((*vmbuf)->v, gsstoken->value, gsstoken->length);
	(*vmbuf)->l = gsstoken->length;

	return 0;
}

static int
gssapi_get_default_name(struct ph1handle *iph1, int remote, gss_name_t *service)
{
	char name[NI_MAXHOST];
	struct sockaddr *sa;
	gss_buffer_desc name_token;
	OM_uint32 min_stat, maj_stat;

	sa = remote ? iph1->remote : iph1->local;

	if (getnameinfo(sa, sa->sa_len, name, NI_MAXHOST, NULL, 0, 0) != 0)
		return -1;

	name_token.length = asprintf((char **)&name_token.value,
	    "%s@%s", GSSAPI_DEF_NAME, name);  
	maj_stat = gss_import_name(&min_stat, &name_token,
	    GSS_C_NT_HOSTBASED_SERVICE, service);
	if (GSS_ERROR(maj_stat)) {
		gssapi_error(min_stat, LOCATION, "import name\n");
		maj_stat = gss_release_buffer(&min_stat, &name_token);
		if (GSS_ERROR(maj_stat))
			gssapi_error(min_stat, LOCATION, "release name_token");
		return -1;
	}
	maj_stat = gss_release_buffer(&min_stat, &name_token);
	if (GSS_ERROR(maj_stat))
		gssapi_error(min_stat, LOCATION, "release name_token");

	return 0;
}

static int
gssapi_init(struct ph1handle *iph1)
{
	struct gssapi_ph1_state *gps;
	gss_buffer_desc id_token, cred_token;
	gss_buffer_t cred = &cred_token;
	gss_name_t princ, canon_princ;
	OM_uint32 maj_stat, min_stat;

	gps = racoon_calloc(1, sizeof (struct gssapi_ph1_state));
	if (gps == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, "racoon_calloc failed\n");
		return -1;
	}
	gps->gss_context = GSS_C_NO_CONTEXT;
	gps->gss_cred = GSS_C_NO_CREDENTIAL;

	gssapi_set_state(iph1, gps);

	if (iph1->rmconf->proposal->gssid != NULL) {
		id_token.length = iph1->rmconf->proposal->gssid->l;
		id_token.value = iph1->rmconf->proposal->gssid->v;
		maj_stat = gss_import_name(&min_stat, &id_token, GSS_C_NO_OID,
		    &princ);
		if (GSS_ERROR(maj_stat)) {
			gssapi_error(min_stat, LOCATION, "import name\n");
			gssapi_free_state(iph1);
			return -1;
		}
	} else
		gssapi_get_default_name(iph1, 0, &princ);

	maj_stat = gss_canonicalize_name(&min_stat, princ, GSS_C_NO_OID,
	    &canon_princ);
	if (GSS_ERROR(maj_stat)) {
		gssapi_error(min_stat, LOCATION, "canonicalize name\n");
		maj_stat = gss_release_name(&min_stat, &princ);
		if (GSS_ERROR(maj_stat))
			gssapi_error(min_stat, LOCATION, "release princ\n");
		gssapi_free_state(iph1);
		return -1;
	}
	maj_stat = gss_release_name(&min_stat, &princ);
	if (GSS_ERROR(maj_stat))
		gssapi_error(min_stat, LOCATION, "release princ\n");

	maj_stat = gss_export_name(&min_stat, canon_princ, cred);
	if (GSS_ERROR(maj_stat)) {
		gssapi_error(min_stat, LOCATION, "export name\n");
		maj_stat = gss_release_name(&min_stat, &canon_princ);
		if (GSS_ERROR(maj_stat))
			gssapi_error(min_stat, LOCATION,
			    "release canon_princ\n");
		gssapi_free_state(iph1);
		return -1;
	}

	plog(LLV_DEBUG, LOCATION, NULL, "will try to acquire '%*s' creds\n",
	    cred->length, cred->value);
	maj_stat = gss_release_buffer(&min_stat, cred);
	if (GSS_ERROR(maj_stat))
		gssapi_error(min_stat, LOCATION, "release cred buffer\n");

	maj_stat = gss_acquire_cred(&min_stat, canon_princ, GSS_C_INDEFINITE,
	    GSS_C_NO_OID_SET, GSS_C_BOTH, &gps->gss_cred, NULL, NULL);
	if (GSS_ERROR(maj_stat)) {
		gssapi_error(min_stat, LOCATION, "acquire cred\n");
		maj_stat = gss_release_name(&min_stat, &canon_princ);
		if (GSS_ERROR(maj_stat))
			gssapi_error(min_stat, LOCATION,
			    "release canon_princ\n");
		gssapi_free_state(iph1);
		return -1;
	}
	maj_stat = gss_release_name(&min_stat, &canon_princ);
	if (GSS_ERROR(maj_stat))
		gssapi_error(min_stat, LOCATION, "release canon_princ\n");

	return 0;
}

int
gssapi_get_itoken(struct ph1handle *iph1, int *lenp)
{
	struct gssapi_ph1_state *gps;
	gss_buffer_desc empty, name_token;
	gss_buffer_t itoken, rtoken, dummy;
	OM_uint32 maj_stat, min_stat;
	gss_name_t partner;

	if (gssapi_get_state(iph1) == NULL && gssapi_init(iph1) < 0)
		return -1;

	gps = gssapi_get_state(iph1);

	empty.length = 0;
	empty.value = NULL;
	dummy = &empty;

	if (iph1->approval != NULL && iph1->approval->gssid != NULL) {
		plog(LLV_DEBUG, LOCATION, NULL, "using provided service '%s'\n",
		    iph1->approval->gssid->v);
		name_token.length = iph1->approval->gssid->l;
		name_token.value = iph1->approval->gssid->v;
		maj_stat = gss_import_name(&min_stat, &name_token,
		    GSS_C_NO_OID, &partner);
		if (GSS_ERROR(maj_stat)) {
			gssapi_error(min_stat, LOCATION, "import of %s\n",
			    name_token.value);
			return -1;
		}
	} else
		if (gssapi_get_default_name(iph1, 1, &partner) < 0)
			return -1;

	rtoken = gps->gsscnt_p == 0 ? dummy : &gps->gss_p[gps->gsscnt_p - 1];
	itoken = &gps->gss[gps->gsscnt];

	gps->gss_status = gss_init_sec_context(&min_stat, gps->gss_cred,
	    &gps->gss_context, partner, GSS_C_NO_OID,
	    GSS_C_MUTUAL_FLAG | GSS_C_SEQUENCE_FLAG |
		GSS_C_CONF_FLAG | GSS_C_INTEG_FLAG,
	    0, GSS_C_NO_CHANNEL_BINDINGS, rtoken, NULL,
	    itoken, NULL, NULL);

	if (GSS_ERROR(gps->gss_status)) {
		gssapi_error(min_stat, LOCATION, "init_sec_context\n");
		maj_stat = gss_release_name(&min_stat, &partner);
		if (GSS_ERROR(maj_stat))
			gssapi_error(min_stat, LOCATION, "release name\n");
		return -1;
	}
	maj_stat = gss_release_name(&min_stat, &partner);
	if (GSS_ERROR(maj_stat))
		gssapi_error(min_stat, LOCATION, "release name\n");

	plog(LLV_DEBUG, LOCATION, NULL, "gss_init_sec_context status %x\n",
	    gps->gss_status);

	if (lenp)
		*lenp = itoken->length;

	if (itoken->length != 0)
		gps->gsscnt++;

	return 0;
}

/*
 * Call gss_accept_context, with token just read from the wire.
 */
int
gssapi_get_rtoken(struct ph1handle *iph1, int *lenp)
{
	struct gssapi_ph1_state *gps;
	gss_buffer_desc name_token;
	gss_buffer_t itoken, rtoken;
	OM_uint32 min_stat, maj_stat;
	gss_name_t client_name;

	if (gssapi_get_state(iph1) == NULL && gssapi_init(iph1) < 0)
		return -1;

	gps = gssapi_get_state(iph1);

	rtoken = &gps->gss_p[gps->gsscnt_p - 1];
	itoken = &gps->gss[gps->gsscnt];

	gps->gss_status = gss_accept_sec_context(&min_stat, &gps->gss_context,
	    gps->gss_cred, rtoken, GSS_C_NO_CHANNEL_BINDINGS, &client_name,
	    NULL, itoken, NULL, NULL, NULL);

	if (GSS_ERROR(gps->gss_status)) {
		gssapi_error(min_stat, LOCATION, "accept_sec_context\n");
		return -1;
	}

	maj_stat = gss_display_name(&min_stat, client_name, &name_token, NULL);
	if (GSS_ERROR(maj_stat)) {
		gssapi_error(min_stat, LOCATION, "gss_display_name\n");
		maj_stat = gss_release_name(&min_stat, &client_name);
		if (GSS_ERROR(maj_stat))
			gssapi_error(min_stat, LOCATION,
			    "release client_name\n");
		return -1;
	}
	maj_stat = gss_release_name(&min_stat, &client_name);
	if (GSS_ERROR(maj_stat))
		gssapi_error(min_stat, LOCATION, "release client_name\n");

	plog(LLV_DEBUG, LOCATION, NULL,
		"gss_accept_sec_context: other side is %s\n",
		name_token.value);
	maj_stat = gss_release_buffer(&min_stat, &name_token);
	if (GSS_ERROR(maj_stat))
		gssapi_error(min_stat, LOCATION, "release name buffer\n");

	if (itoken->length != 0)
		gps->gsscnt++;

	if (lenp)
		*lenp = itoken->length;

	return 0;
}

int
gssapi_save_received_token(struct ph1handle *iph1, vchar_t *token)
{
	struct gssapi_ph1_state *gps;
	gss_buffer_t gsstoken;
	int ret;

	if (gssapi_get_state(iph1) == NULL && gssapi_init(iph1) < 0)
		return -1;

	gps = gssapi_get_state(iph1);

	gsstoken = &gps->gss_p[gps->gsscnt_p];

	ret = gssapi_vm2gssbuf(token, gsstoken);
	if (ret < 0)
		return ret;
	gps->gsscnt_p++;

	return 0;
}

int
gssapi_get_token_to_send(struct ph1handle *iph1, vchar_t **token)
{
	struct gssapi_ph1_state *gps;
	gss_buffer_t gsstoken;
	int ret;

	gps = gssapi_get_state(iph1);
	if (gps == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "gssapi not yet initialized?\n");
		return -1;
	}
	gsstoken = &gps->gss[gps->gsscnt - 1];
	ret = gssapi_gss2vmbuf(gsstoken, token);
	if (ret < 0)
		return ret;

	return 0;
}

int
gssapi_get_itokens(struct ph1handle *iph1, vchar_t **tokens)
{
	struct gssapi_ph1_state *gps;
	int len, i;
	vchar_t *toks;
	char *p;

	gps = gssapi_get_state(iph1);
	if (gps == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "gssapi not yet initialized?\n");
		return -1;
	}

	for (i = len = 0; i < gps->gsscnt; i++)
		len += gps->gss[i].length;

	toks = vmalloc(len);
	if (toks == 0)
		return -1;
	p = (char *)toks->v;
	for (i = 0; i < gps->gsscnt; i++) {
		memcpy(p, gps->gss[i].value, gps->gss[i].length);
		p += gps->gss[i].length;
	}

	*tokens = toks;

	plog(LLV_DEBUG, LOCATION, NULL,
		"%d itokens of length %d\n", gps->gsscnt, (*tokens)->l);

	return 0;
}

int
gssapi_get_rtokens(struct ph1handle *iph1, vchar_t **tokens)
{
	struct gssapi_ph1_state *gps;
	int len, i;
	vchar_t *toks;
	char *p;

	gps = gssapi_get_state(iph1);
	if (gps == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "gssapi not yet initialized?\n");
		return -1;
	}

	if (gssapi_more_tokens(iph1)) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "gssapi roundtrips not complete\n");
		return -1;
	}

	for (i = len = 0; i < gps->gsscnt_p; i++)
		len += gps->gss_p[i].length;

	toks = vmalloc(len);
	if (toks == 0)
		return -1;
	p = (char *)toks->v;
	for (i = 0; i < gps->gsscnt_p; i++) {
		memcpy(p, gps->gss_p[i].value, gps->gss_p[i].length);
		p += gps->gss_p[i].length;
	}

	*tokens = toks;

	return 0;
}

vchar_t *
gssapi_wraphash(struct ph1handle *iph1)
{
	struct gssapi_ph1_state *gps;
	OM_uint32 maj_stat, min_stat;
	gss_buffer_desc hash_in_buf, hash_out_buf;
	gss_buffer_t hash_in = &hash_in_buf, hash_out = &hash_out_buf;
	vchar_t *outbuf;

	gps = gssapi_get_state(iph1);
	if (gps == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "gssapi not yet initialized?\n");
		return NULL;
	}

	if (gssapi_more_tokens(iph1)) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "gssapi roundtrips not complete\n");
		return NULL;
	}

	if (gssapi_vm2gssbuf(iph1->hash, hash_in) < 0) {
		plog(LLV_ERROR, LOCATION, NULL, "vm2gssbuf failed\n");
		return NULL;
	}

	maj_stat = gss_wrap(&min_stat, gps->gss_context, 1, GSS_C_QOP_DEFAULT,
	    hash_in, NULL, hash_out);
	if (GSS_ERROR(maj_stat)) {
		gssapi_error(min_stat, LOCATION, "wrapping hash value\n");
		maj_stat = gss_release_buffer(&min_stat, hash_in);
		if (GSS_ERROR(maj_stat))
			gssapi_error(min_stat, LOCATION,
			    "release hash_in buffer\n");
		return NULL;
	}

	plog(LLV_DEBUG, LOCATION, NULL, "wrapped HASH, ilen %d olen %d\n",
	    hash_in->length, hash_out->length);

	maj_stat = gss_release_buffer(&min_stat, hash_in);
	if (GSS_ERROR(maj_stat))
		gssapi_error(min_stat, LOCATION, "release hash_in buffer\n");

	if (gssapi_gss2vmbuf(hash_out, &outbuf) < 0) {
		plog(LLV_ERROR, LOCATION, NULL, "gss2vmbuf failed\n");
		maj_stat = gss_release_buffer(&min_stat, hash_out);
		if (GSS_ERROR(maj_stat))
			gssapi_error(min_stat, LOCATION,
			    "release hash_out buffer\n");
		return NULL;
	}
	maj_stat = gss_release_buffer(&min_stat, hash_out);
	if (GSS_ERROR(maj_stat))
		gssapi_error(min_stat, LOCATION, "release hash_out buffer\n");

	return outbuf;
}

vchar_t *
gssapi_unwraphash(struct ph1handle *iph1)
{
	struct gssapi_ph1_state *gps;
	OM_uint32 maj_stat, min_stat;
	gss_buffer_desc hashbuf, hash_outbuf;
	gss_buffer_t hash_in = &hashbuf, hash_out = &hash_outbuf;
	vchar_t *outbuf;

	gps = gssapi_get_state(iph1);
	if (gps == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "gssapi not yet initialized?\n");
		return NULL;
	}


	hashbuf.length = ntohs(iph1->pl_hash->h.len) - sizeof(*iph1->pl_hash);
	hashbuf.value = (char *)(iph1->pl_hash + 1);

	plog(LLV_DEBUG, LOCATION, NULL, "unwrapping HASH of len %d\n",
	    hashbuf.length);

	maj_stat = gss_unwrap(&min_stat, gps->gss_context, hash_in, hash_out,
	    NULL, NULL);
	if (GSS_ERROR(maj_stat)) {
		gssapi_error(min_stat, LOCATION, "unwrapping hash value\n");
		return NULL;
	}

	if (gssapi_gss2vmbuf(hash_out, &outbuf) < 0) {
		plog(LLV_ERROR, LOCATION, NULL, "gss2vmbuf failed\n");
		maj_stat = gss_release_buffer(&min_stat, hash_out);
		if (GSS_ERROR(maj_stat))
			gssapi_error(min_stat, LOCATION,
			    "release hash_out buffer\n");
		return NULL;
	}
	maj_stat = gss_release_buffer(&min_stat, hash_out);
	if (GSS_ERROR(maj_stat))
		gssapi_error(min_stat, LOCATION, "release hash_out buffer\n");

	return outbuf;
}

void
gssapi_set_id_sent(struct ph1handle *iph1)
{
	struct gssapi_ph1_state *gps;

	gps = gssapi_get_state(iph1);

	gps->gss_flags |= GSSFLAG_ID_SENT;
}

int
gssapi_id_sent(struct ph1handle *iph1)
{
	struct gssapi_ph1_state *gps;

	gps = gssapi_get_state(iph1);

	return (gps->gss_flags & GSSFLAG_ID_SENT) != 0;
}

void
gssapi_set_id_rcvd(struct ph1handle *iph1)
{
	struct gssapi_ph1_state *gps;

	gps = gssapi_get_state(iph1);

	gps->gss_flags |= GSSFLAG_ID_RCVD;
}

int
gssapi_id_rcvd(struct ph1handle *iph1)
{
	struct gssapi_ph1_state *gps;

	gps = gssapi_get_state(iph1);

	return (gps->gss_flags & GSSFLAG_ID_RCVD) != 0;
}

void
gssapi_free_state(struct ph1handle *iph1)
{
	struct gssapi_ph1_state *gps;
	OM_uint32 maj_stat, min_stat;

	gps = gssapi_get_state(iph1);

	if (gps == NULL)
		return;

	gssapi_set_state(iph1, NULL);

	if (gps->gss_cred != GSS_C_NO_CREDENTIAL) {
		maj_stat = gss_release_cred(&min_stat, &gps->gss_cred);
		if (GSS_ERROR(maj_stat))
			gssapi_error(min_stat, LOCATION,
			    "releasing credentials\n");
	}
	racoon_free(gps);
}

vchar_t *
gssapi_get_default_id(struct ph1handle *iph1)
{
	gss_buffer_desc id_buffer;
	gss_buffer_t id = &id_buffer;
	gss_name_t defname, canon_name;
	OM_uint32 min_stat, maj_stat;
	vchar_t *vmbuf;

	if (gssapi_get_default_name(iph1, 0, &defname) < 0)
		return NULL;

	maj_stat = gss_canonicalize_name(&min_stat, defname, GSS_C_NO_OID,
	    &canon_name);
	if (GSS_ERROR(maj_stat)) {
		gssapi_error(min_stat, LOCATION, "canonicalize name\n");
		maj_stat = gss_release_name(&min_stat, &defname);
		if (GSS_ERROR(maj_stat))
			gssapi_error(min_stat, LOCATION,
			    "release default name\n");
		return NULL;
	}
	maj_stat = gss_release_name(&min_stat, &defname);
	if (GSS_ERROR(maj_stat))
		gssapi_error(min_stat, LOCATION, "release default name\n");

	maj_stat = gss_export_name(&min_stat, canon_name, id);
	if (GSS_ERROR(maj_stat)) {
		gssapi_error(min_stat, LOCATION, "export name\n");
		maj_stat = gss_release_name(&min_stat, &canon_name);
		if (GSS_ERROR(maj_stat))
			gssapi_error(min_stat, LOCATION,
			    "release canonical name\n");
		return NULL;
	}
	maj_stat = gss_release_name(&min_stat, &canon_name);
	if (GSS_ERROR(maj_stat))
		gssapi_error(min_stat, LOCATION, "release canonical name\n");

	plog(LLV_DEBUG, LOCATION, NULL, "will try to acquire '%*s' creds\n",
	    id->length, id->value);

	if (gssapi_gss2vmbuf(id, &vmbuf) < 0) {
		plog(LLV_ERROR, LOCATION, NULL, "gss2vmbuf failed\n");
		maj_stat = gss_release_buffer(&min_stat, id);
		if (GSS_ERROR(maj_stat))
			gssapi_error(min_stat, LOCATION, "release id buffer\n");
		return NULL;
	}
	maj_stat = gss_release_buffer(&min_stat, id);
	if (GSS_ERROR(maj_stat))
		gssapi_error(min_stat, LOCATION, "release id buffer\n");

	return vmbuf;
}
#else
int __gssapi_dUmMy;
#endif

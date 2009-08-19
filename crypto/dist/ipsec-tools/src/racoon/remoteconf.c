/*	$NetBSD: remoteconf.c,v 1.16 2009/08/19 12:20:02 tteras Exp $	*/

/* Id: remoteconf.c,v 1.38 2006/05/06 15:52:44 manubsd Exp */

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
#include <sys/socket.h>
#include <sys/queue.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

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
#include "genlist.h"
#include "debug.h"

#include "isakmp_var.h"
#ifdef ENABLE_HYBRID
#include "isakmp_xauth.h"
#endif
#include "isakmp.h"
#include "ipsec_doi.h"
#include "crypto_openssl.h"
#include "oakley.h"
#include "remoteconf.h"
#include "localconf.h"
#include "grabmyaddr.h"
#include "policy.h"
#include "proposal.h"
#include "vendorid.h"
#include "gcmalloc.h"
#include "strnames.h"
#include "algorithm.h"
#include "nattraversal.h"
#include "isakmp_frag.h"
#include "handler.h"
#include "genlist.h"

static TAILQ_HEAD(_rmtree, remoteconf) rmtree, rmtree_save, rmtree_tmp;

/*
 * Script hook names and script hook paths
 */
char *script_names[SCRIPT_MAX + 1] = { "phase1_up", "phase1_down" };

/*%%%*/

int
rmconf_match_identity(rmconf, id_p)
	struct remoteconf *rmconf;
	vchar_t *id_p;
{
	struct ipsecdoi_id_b *id_b = (struct ipsecdoi_id_b *) id_p->v;
	struct sockaddr *sa;
	caddr_t sa1, sa2;
	vchar_t ident;
	struct idspec *id;
	struct genlist_entry *gpb;

	/* compare with the ID if specified. */
	if (!genlist_next(rmconf->idvl_p, 0))
		return 0;

	for (id = genlist_next(rmconf->idvl_p, &gpb); id; id = genlist_next(0, &gpb)) {
		/* check the type of both IDs */
		if (id->idtype != doi2idtype(id_b->type))
			continue;  /* ID type mismatch */
		if (id->id == 0)
			return 0;

		/* compare defined ID with the ID sent by peer. */
		switch (id->idtype) {
		case IDTYPE_ASN1DN:
			ident.v = id_p->v + sizeof(*id_b);
			ident.l = id_p->l - sizeof(*id_b);
			if (eay_cmp_asn1dn(id->id, &ident) == 0)
				return 0;
			break;
		case IDTYPE_ADDRESS:
			sa = (struct sockaddr *)id->id->v;
			sa2 = (caddr_t)(id_b + 1);
			switch (sa->sa_family) {
			case AF_INET:
				if (id_p->l - sizeof(*id_b) != sizeof(struct in_addr))
					continue;  /* ID value mismatch */
				sa1 = (caddr_t) &((struct sockaddr_in *)sa)->sin_addr;
				if (memcmp(sa1, sa2, sizeof(struct in_addr)) == 0)
					return 0;
				break;
#ifdef INET6
			case AF_INET6:
				if (id_p->l - sizeof(*id_b) != sizeof(struct in6_addr))
					continue;  /* ID value mismatch */
				sa1 = (caddr_t) &((struct sockaddr_in6 *)sa)->sin6_addr;
				if (memcmp(sa1, sa2, sizeof(struct in6_addr)) == 0)
					return 0;
				break;
#endif
			default:
				break;
			}
			break;
		default:
			if (memcmp(id->id->v, id_b + 1, id->id->l) == 0)
				return 0;
			break;
		}
	}

	plog(LLV_WARNING, LOCATION, NULL, "No ID match.\n");
	if (rmconf->verify_identifier)
		return ISAKMP_NTYPE_INVALID_ID_INFORMATION;

	return 0;
}

static int
rmconf_match_etype_and_approval(rmconf, etype, approval)
	struct remoteconf *rmconf;
	int etype;
	struct isakmpsa *approval;
{
	struct isakmpsa *p;

	if (check_etypeok(rmconf, (void *) (intptr_t) etype) == 0)
		return ISAKMP_NTYPE_NO_PROPOSAL_CHOSEN;

	if (approval == NULL)
		return 0;

	if (etype == ISAKMP_ETYPE_AGG &&
	    approval->dh_group != rmconf->dh_group)
		return ISAKMP_NTYPE_NO_PROPOSAL_CHOSEN;

	if (checkisakmpsa(rmconf->pcheck_level, approval,
			  rmconf->proposal) == NULL)
		return ISAKMP_NTYPE_NO_PROPOSAL_CHOSEN;

	return 0;
}

enum rmconf_match_t {
	MATCH_NONE = 0,
	MATCH_ANONYMOUS,
	MATCH_ADDRESS,
	MATCH_SA,
	MATCH_IDENTITY,
	MATCH_AUTH_IDENTITY,
};

static int
rmconf_match_type(rmsel, rmconf)
	struct rmconfselector *rmsel;
	struct remoteconf *rmconf;
{
	int ret = 1;

	/* No match at all: unwanted anonymous */
	if ((rmsel->flags & GETRMCONF_F_NO_ANONYMOUS) &&
	    rmconf->remote->sa_family == AF_UNSPEC)
		return MATCH_NONE;

	if ((rmsel->flags & GETRMCONF_F_NO_PASSIVE) && rmconf->passive)
		return MATCH_NONE;

	/* Check address */
	if (rmsel->remote != NULL) {
		if (rmconf->remote->sa_family != AF_UNSPEC) {
			if (cmpsaddr(rmsel->remote, rmconf->remote) != 0)
				return MATCH_NONE;

			/* Address matched */
			ret = MATCH_ADDRESS;
		}
	}

	/* Check etype and approval */
	if (rmsel->etype != ISAKMP_ETYPE_NONE) {
		if (rmconf_match_etype_and_approval(rmconf, rmsel->etype,
						    rmsel->approval) != 0)
			return MATCH_NONE;
		ret = MATCH_SA;
	}

	/* Check identity */
	if (rmsel->identity != NULL && rmconf->verify_identifier) {
		if (rmconf_match_identity(rmconf, rmsel->identity) != 0)
			return MATCH_NONE;
		ret = MATCH_IDENTITY;
	}

	/* Check certificate request */
	if (rmsel->certificate_request != NULL) {
		if (oakley_get_certtype(rmsel->certificate_request) !=
		    oakley_get_certtype(rmconf->mycert))
			return MATCH_NONE;

		if (rmsel->certificate_request->l > 1) {
			vchar_t *issuer;

			issuer = eay_get_x509asn1issuername(rmconf->mycert);
			if (rmsel->certificate_request->l - 1 != issuer->l ||
			    memcmp(rmsel->certificate_request->v + 1,
				   issuer->v, issuer->l) != 0) {
				vfree(issuer);
				return MATCH_NONE;
			}
			vfree(issuer);
		} else {
			if (!rmconf->match_empty_cr)
				return MATCH_NONE;
		}

		ret = MATCH_AUTH_IDENTITY;
	}

	return ret;
}

void rmconf_selector_from_ph1(rmsel, iph1)
	struct rmconfselector *rmsel;
	struct ph1handle *iph1;
{
	memset(rmsel, 0, sizeof(*rmsel));
	rmsel->flags = 0;
	rmsel->remote = iph1->remote;
	rmsel->etype = iph1->etype;
	rmsel->approval = iph1->approval;
	rmsel->identity = iph1->id_p;
	rmsel->certificate_request = iph1->cr_p;
}

int
enumrmconf(rmsel, enum_func, enum_arg)
	struct rmconfselector *rmsel;
	int (* enum_func)(struct remoteconf *rmconf, void *arg);
	void *enum_arg;
{
	struct remoteconf *p;
	int ret = 0;

	RACOON_TAILQ_FOREACH_REVERSE(p, &rmtree, _rmtree, chain) {
		if (rmsel != NULL) {
			if (rmconf_match_type(rmsel, p) == 0)
				continue;
		}

		plog(LLV_DEBUG2, LOCATION, NULL,
		     "enumrmconf: \"%s\" matches.\n", p->name);

		ret = (*enum_func)(p, enum_arg);
		if (ret)
			break;
	}

	return ret;
}

struct rmconf_find_context {
	struct rmconfselector sel;

	struct remoteconf *rmconf;
	int match_type;
	int num_found;
};

static int
rmconf_find(rmconf, ctx)
	struct remoteconf *rmconf;
	void *ctx;
{
	struct rmconf_find_context *fctx = (struct rmconf_find_context *) ctx;
	int match_type;

	/* First matching remote conf? */
	match_type = rmconf_match_type(&fctx->sel, rmconf);

	if (fctx->rmconf != NULL) {
		/* More ambiguous matches are ignored. */
		if (match_type < fctx->match_type)
			return 0;

		if (match_type == fctx->match_type) {
			/* Duplicate exact match, something is wrong */
			if (match_type >= MATCH_AUTH_IDENTITY)
				return 1;

			/* Otherwise just remember that this is ambiguous match */
			fctx->num_found++;
			return 0;
		}
	}

	/* More exact match found */
	fctx->match_type = match_type;
	fctx->num_found = 1;
	fctx->rmconf = rmconf;

	return 0;
}

/*
 * search remote configuration.
 * don't use port number to search if its value is either IPSEC_PORT_ANY.
 * If matching anonymous entry, then new entry is copied from anonymous entry.
 * If no anonymous entry found, then return NULL.
 * OUT:	NULL:	NG
 *	Other:	remote configuration entry.
 */

struct remoteconf *
getrmconf(remote, flags)
	struct sockaddr *remote;
	int flags;
{
	struct rmconf_find_context ctx;
	int n = 0;

	memset(&ctx, 0, sizeof(ctx));
	ctx.sel.flags = flags;
	ctx.sel.remote = remote;

	if (enumrmconf(&ctx.sel, rmconf_find, &ctx) != 0) {
		plog(LLV_ERROR, LOCATION, remote,
		     "multiple exact configurations.\n");
		return NULL;
	}

	if (ctx.rmconf == NULL) {
		plog(LLV_DEBUG, LOCATION, remote,
		     "no remote configuration found.\n");
		return NULL;
	}

	if (ctx.num_found != 1) {
		plog(LLV_DEBUG, LOCATION, remote,
		     "multiple non-exact configurations found.\n");
		return NULL;
	}

	plog(LLV_DEBUG, LOCATION, remote,
	     "configuration \"%s\" selected.\n",
	     ctx.rmconf->name);

	return ctx.rmconf;
}

struct remoteconf *
getrmconf_by_ph1(iph1)
	struct ph1handle *iph1;
{
	struct rmconf_find_context ctx;

	memset(&ctx, 0, sizeof(ctx));
	rmconf_selector_from_ph1(&ctx.sel, iph1);
	if (loglevel >= LLV_DEBUG) {
		char *idstr = NULL;

		if (iph1->id_p != NULL)
			idstr = ipsecdoi_id2str(iph1->id_p);

		plog(LLV_DEBUG, LOCATION, iph1->remote,
			"getrmconf_by_ph1: remote %s, identity %s.\n",
			saddr2str(iph1->remote), idstr ? idstr : "<any>");

		if (idstr)
			racoon_free(idstr);
	}

	if (enumrmconf(&ctx.sel, rmconf_find, &ctx) != 0) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
		     "multiple exact configurations.\n");
		return RMCONF_ERR_MULTIPLE;
	}

	if (ctx.rmconf == NULL) {
		plog(LLV_DEBUG, LOCATION, iph1->remote,
		     "no remote configuration found\n");
		return NULL;
	}

	if (ctx.num_found != 1) {
		plog(LLV_DEBUG, LOCATION, iph1->remote,
		     "multiple non-exact configurations found.\n");
		return RMCONF_ERR_MULTIPLE;
	}

	plog(LLV_DEBUG, LOCATION, iph1->remote,
	     "configuration \"%s\" selected.\n",
	     ctx.rmconf->name);

	return ctx.rmconf;
}

struct remoteconf *
getrmconf_by_name(name)
	const char *name;
{
	struct remoteconf *p;

	plog(LLV_DEBUG, LOCATION, NULL,
	     "getrmconf_by_name: remote \"%s\".\n",
	     name);

	RACOON_TAILQ_FOREACH_REVERSE(p, &rmtree, _rmtree, chain) {
		if (p->name == NULL)
			continue;

		if (strcmp(name, p->name) == 0)
			return p;
	}

	return NULL;
}

struct remoteconf *
newrmconf()
{
	struct remoteconf *new;
	int i;

	new = racoon_calloc(1, sizeof(*new));
	if (new == NULL)
		return NULL;

	new->proposal = NULL;

	/* set default */
	new->doitype = IPSEC_DOI;
	new->sittype = IPSECDOI_SIT_IDENTITY_ONLY;
	new->idvtype = IDTYPE_UNDEFINED;
	new->idvl_p = genlist_init();
	new->nonce_size = DEFAULT_NONCE_SIZE;
	new->passive = FALSE;
	new->ike_frag = FALSE;
	new->esp_frag = IP_MAXPACKET;
	new->ini_contact = TRUE;
	new->mode_cfg = FALSE;
	new->pcheck_level = PROP_CHECK_STRICT;
	new->verify_identifier = FALSE;
	new->verify_cert = TRUE;
	new->cacertfile = NULL;
	new->send_cert = TRUE;
	new->send_cr = TRUE;
	new->match_empty_cr = FALSE;
	new->support_proxy = FALSE;
	for (i = 0; i <= SCRIPT_MAX; i++)
		new->script[i] = NULL;
	new->gen_policy = FALSE;
	new->nat_traversal = FALSE;
	new->rsa_private = genlist_init();
	new->rsa_public = genlist_init();
	new->idv = NULL;
	new->key = NULL;

	new->dpd = TRUE; /* Enable DPD support by default */
	new->dpd_interval = 0; /* Disable DPD checks by default */
	new->dpd_retry = 5;
	new->dpd_maxfails = 5;

	new->rekey = REKEY_ON;

	new->weak_phase1_check = 0;

#ifdef ENABLE_HYBRID
	new->xauth = NULL;
#endif

	new->lifetime = oakley_get_defaultlifetime();

	return new;
}

void *
dupidvl(entry, arg)
	void *entry;
	void *arg;
{
	struct idspec *id;
	struct idspec *old = (struct idspec *) entry;
	id = newidspec();
	if (!id) return (void *) -1;

	if (set_identifier(&id->id, old->idtype, old->id) != 0) {
		racoon_free(id);
		return (void *) -1;
	}

	id->idtype = old->idtype;

	genlist_append(arg, id);
	return NULL;
}

struct remoteconf *
duprmconf (rmconf)
	struct remoteconf *rmconf;
{
	struct remoteconf *new;
	struct proposalspec *prspec;

	new = racoon_calloc(1, sizeof(*new));
	if (new == NULL)
		return NULL;

	memcpy(new, rmconf, sizeof(*new));
	new->name = NULL;
	new->inherited_from = rmconf;

	/* duplicate dynamic structures */
	if (new->etypes)
		new->etypes = dupetypes(new->etypes);
	new->idvl_p = genlist_init();
	genlist_foreach(rmconf->idvl_p, dupidvl, new->idvl_p);

	return new;
}

static void
idspec_free(void *data)
{
	vfree (((struct idspec *)data)->id);
	free (data);
}

void
delrmconf(rmconf)
	struct remoteconf *rmconf;
{
#ifdef ENABLE_HYBRID
	if (rmconf->xauth)
		xauth_rmconf_delete(&rmconf->xauth);
#endif
	if (rmconf->etypes){
		deletypes(rmconf->etypes);
		rmconf->etypes=NULL;
	}
	if (rmconf->idvl_p)
		genlist_free(rmconf->idvl_p, idspec_free);
	if (rmconf->dhgrp)
		oakley_dhgrp_free(rmconf->dhgrp);
	if (rmconf->proposal)
		delisakmpsa(rmconf->proposal);
	if (rmconf->mycert)
		vfree(rmconf->mycert);
	if (rmconf->mycertfile)
		racoon_free(rmconf->mycertfile);
	if (rmconf->myprivfile)
		racoon_free(rmconf->myprivfile);
	if (rmconf->peerscert)
		vfree(rmconf->peerscert);
	if (rmconf->peerscertfile)
		racoon_free(rmconf->peerscertfile);
	if (rmconf->cacert)
		vfree(rmconf->cacert);
	if (rmconf->cacertfile)
		racoon_free(rmconf->cacertfile);
	if (rmconf->name)
		racoon_free(rmconf->name);
	racoon_free(rmconf);
}

void
delisakmpsa(sa)
	struct isakmpsa *sa;
{
	if (sa->dhgrp)
		oakley_dhgrp_free(sa->dhgrp);
	if (sa->next)
		delisakmpsa(sa->next);
#ifdef HAVE_GSSAPI
	if (sa->gssid)
		vfree(sa->gssid);
#endif
	racoon_free(sa);
}

struct etypes *
dupetypes(orig)
	struct etypes *orig;
{
	struct etypes *new;

	if (!orig)
		return NULL;

	new = racoon_malloc(sizeof(struct etypes));
	if (new == NULL)
		return NULL;

	new->type = orig->type;
	new->next = NULL;

	if (orig->next)
		new->next=dupetypes(orig->next);

	return new;
}

void
deletypes(e)
	struct etypes *e;
{
	if (e->next)
		deletypes(e->next);
	racoon_free(e);
}

/*
 * insert into head of list.
 */
void
insrmconf(new)
	struct remoteconf *new;
{
	if (new->name == NULL) {
		new->name = racoon_strdup(saddr2str(new->remote));
	}
	if (new->remote == NULL) {
		new->remote = newsaddr(sizeof(struct sockaddr));
		new->remote->sa_family = AF_UNSPEC;
	}

	TAILQ_INSERT_HEAD(&rmtree, new, chain);
}

void
remrmconf(rmconf)
	struct remoteconf *rmconf;
{
	TAILQ_REMOVE(&rmtree, rmconf, chain);
}

void
flushrmconf()
{
	struct remoteconf *p, *next;

	for (p = TAILQ_FIRST(&rmtree); p; p = next) {
		next = TAILQ_NEXT(p, chain);
		remrmconf(p);
		delrmconf(p);
	}
}

void
initrmconf()
{
	TAILQ_INIT(&rmtree);
}

void
save_rmconf()
{
	rmtree_save=rmtree;
	initrmconf();
}

void
save_rmconf_flush()
{
	rmtree_tmp=rmtree;
	rmtree=rmtree_save;
	flushrmconf();
	initrmconf();
	rmtree=rmtree_tmp;
}



/* check exchange type to be acceptable */
int
check_etypeok(rmconf, ctx)
	struct remoteconf *rmconf;
	void *ctx;
{
	u_int8_t etype = (u_int8_t) (intptr_t) ctx;
	struct etypes *e;

	for (e = rmconf->etypes; e != NULL; e = e->next) {
		if (e->type == etype)
			return 1;
	}

	return 0;
}

/*%%%*/
struct isakmpsa *
newisakmpsa()
{
	struct isakmpsa *new;

	new = racoon_calloc(1, sizeof(*new));
	if (new == NULL)
		return NULL;

	/*
	 * Just for sanity, make sure this is initialized.  This is
	 * filled in for real when the ISAKMP proposal is configured.
	 */
	new->vendorid = VENDORID_UNKNOWN;

	new->next = NULL;
#ifdef HAVE_GSSAPI
	new->gssid = NULL;
#endif

	return new;
}

/*
 * insert into tail of list.
 */
void
insisakmpsa(new, rmconf)
	struct isakmpsa *new;
	struct remoteconf *rmconf;
{
	struct isakmpsa *p;

	if (rmconf->proposal == NULL) {
		rmconf->proposal = new;
		return;
	}

	for (p = rmconf->proposal; p->next != NULL; p = p->next)
		;
	p->next = new;
}

static void *
dump_peers_identifiers (void *entry, void *arg)
{
	struct idspec *id = (struct idspec*) entry;
	char buf[1024], *pbuf;
	pbuf = buf;
	pbuf += sprintf (pbuf, "\tpeers_identifier %s",
			 s_idtype (id->idtype));
	if (id->id)
		pbuf += sprintf (pbuf, " \"%s\"", id->id->v);
	plog(LLV_INFO, LOCATION, NULL, "%s;\n", buf);
	return NULL;
}

static int
dump_rmconf_single (struct remoteconf *p, void *data)
{
	struct etypes *etype = p->etypes;
	struct isakmpsa *prop = p->proposal;
	char buf[1024], *pbuf;

	pbuf = buf;

	pbuf += sprintf(pbuf, "remote \"%s\"", p->name);
	if (p->inherited_from)
		pbuf += sprintf(pbuf, " inherit \"%s\"",
				p->inherited_from->name);
	plog(LLV_INFO, LOCATION, NULL, "%s {\n", buf);
	pbuf = buf;
	pbuf += sprintf(pbuf, "\texchange_type ");
	while (etype) {
		pbuf += sprintf (pbuf, "%s%s", s_etype(etype->type),
				 etype->next != NULL ? ", " : ";\n");
		etype = etype->next;
	}
	plog(LLV_INFO, LOCATION, NULL, "%s", buf);
	plog(LLV_INFO, LOCATION, NULL, "\tdoi %s;\n", s_doi(p->doitype));
	pbuf = buf;
	pbuf += sprintf(pbuf, "\tmy_identifier %s", s_idtype (p->idvtype));
	if (p->idvtype == IDTYPE_ASN1DN) {
		plog(LLV_INFO, LOCATION, NULL, "%s;\n", buf);
		plog(LLV_INFO, LOCATION, NULL,
		     "\tcertificate_type %s \"%s\" \"%s\";\n",
		     oakley_get_certtype(p->mycert) == ISAKMP_CERT_X509SIGN
		       ? "x509" : "*UNKNOWN*",
		     p->mycertfile, p->myprivfile);

		switch (oakley_get_certtype(p->peerscert)) {
		case ISAKMP_CERT_NONE:
			plog(LLV_INFO, LOCATION, NULL,
			     "\t/* peers certificate from payload */\n");
			break;
		case ISAKMP_CERT_X509SIGN:
			plog(LLV_INFO, LOCATION, NULL,
			     "\tpeers_certfile \"%s\";\n", p->peerscertfile);
			break;
		case ISAKMP_CERT_DNS:
			plog(LLV_INFO, LOCATION, NULL,
			     "\tpeers_certfile dnssec;\n");
			break;
		default:
			plog(LLV_INFO, LOCATION, NULL,
			     "\tpeers_certfile *UNKNOWN* (%d)\n",
			     oakley_get_certtype(p->peerscert));
			break;
		}
	}
	else {
		if (p->idv)
			pbuf += sprintf (pbuf, " \"%s\"", p->idv->v);
		plog(LLV_INFO, LOCATION, NULL, "%s;\n", buf);
		genlist_foreach(p->idvl_p, &dump_peers_identifiers, NULL);
	}

	plog(LLV_INFO, LOCATION, NULL, "\trekey %s;\n",
		p->rekey == REKEY_FORCE ? "force" : s_switch (p->rekey));
	plog(LLV_INFO, LOCATION, NULL, "\tsend_cert %s;\n",
		s_switch (p->send_cert));
	plog(LLV_INFO, LOCATION, NULL, "\tsend_cr %s;\n",
		s_switch (p->send_cr));
	plog(LLV_INFO, LOCATION, NULL, "\tmatch_empty_cr %s;\n",
		s_switch (p->match_empty_cr));
	plog(LLV_INFO, LOCATION, NULL, "\tverify_cert %s;\n",
		s_switch (p->verify_cert));
	plog(LLV_INFO, LOCATION, NULL, "\tverify_identifier %s;\n",
		s_switch (p->verify_identifier));
	plog(LLV_INFO, LOCATION, NULL, "\tnat_traversal %s;\n",
		p->nat_traversal == NATT_FORCE ?
			"force" : s_switch (p->nat_traversal));
	plog(LLV_INFO, LOCATION, NULL, "\tnonce_size %d;\n",
		p->nonce_size);
	plog(LLV_INFO, LOCATION, NULL, "\tpassive %s;\n",
		s_switch (p->passive));
	plog(LLV_INFO, LOCATION, NULL, "\tike_frag %s;\n",
		p->ike_frag == ISAKMP_FRAG_FORCE ?
			"force" : s_switch (p->ike_frag));
	plog(LLV_INFO, LOCATION, NULL, "\tesp_frag %d;\n", p->esp_frag);
	plog(LLV_INFO, LOCATION, NULL, "\tinitial_contact %s;\n",
		s_switch (p->ini_contact));
	plog(LLV_INFO, LOCATION, NULL, "\tgenerate_policy %s;\n",
		s_switch (p->gen_policy));
	plog(LLV_INFO, LOCATION, NULL, "\tsupport_proxy %s;\n",
		s_switch (p->support_proxy));

	while (prop) {
		plog(LLV_INFO, LOCATION, NULL, "\n");
		plog(LLV_INFO, LOCATION, NULL,
			"\t/* prop_no=%d, trns_no=%d */\n",
			prop->prop_no, prop->trns_no);
		plog(LLV_INFO, LOCATION, NULL, "\tproposal {\n");
		plog(LLV_INFO, LOCATION, NULL, "\t\tlifetime time %lu sec;\n",
			(long)prop->lifetime);
		plog(LLV_INFO, LOCATION, NULL, "\t\tlifetime bytes %zd;\n",
			prop->lifebyte);
		plog(LLV_INFO, LOCATION, NULL, "\t\tdh_group %s;\n",
			alg_oakley_dhdef_name(prop->dh_group));
		plog(LLV_INFO, LOCATION, NULL, "\t\tencryption_algorithm %s;\n",
			alg_oakley_encdef_name(prop->enctype));
		plog(LLV_INFO, LOCATION, NULL, "\t\thash_algorithm %s;\n",
			alg_oakley_hashdef_name(prop->hashtype));
		plog(LLV_INFO, LOCATION, NULL, "\t\tauthentication_method %s;\n",
			alg_oakley_authdef_name(prop->authmethod));
		plog(LLV_INFO, LOCATION, NULL, "\t}\n");
		prop = prop->next;
	}
	plog(LLV_INFO, LOCATION, NULL, "}\n");
	plog(LLV_INFO, LOCATION, NULL, "\n");

	return 0;
}

void
dumprmconf()
{
	enumrmconf(NULL, dump_rmconf_single, NULL);
}

struct idspec *
newidspec()
{
	struct idspec *new;

	new = racoon_calloc(1, sizeof(*new));
	if (new == NULL)
		return NULL;
	new->idtype = IDTYPE_ADDRESS;

	return new;
}

vchar_t *
script_path_add(path)
	vchar_t *path;
{
	char *script_dir;
	vchar_t *new_path;
	vchar_t *new_storage;
	vchar_t **sp;
	size_t len;
	size_t size;

	script_dir = lcconf->pathinfo[LC_PATHTYPE_SCRIPT];

	/* Try to find the script in the script directory */
	if ((path->v[0] != '/') && (script_dir != NULL)) {
		len = strlen(script_dir) + sizeof("/") + path->l + 1;

		if ((new_path = vmalloc(len)) == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "Cannot allocate memory: %s\n", strerror(errno));
			return NULL;
		}

		new_path->v[0] = '\0';
		(void)strlcat(new_path->v, script_dir, len);
		(void)strlcat(new_path->v, "/", len);
		(void)strlcat(new_path->v, path->v, len);

		vfree(path);
		path = new_path;
	}

	return path;
}


struct isakmpsa *
dupisakmpsa(struct isakmpsa *sa)
{
	struct isakmpsa *res = NULL;

	if(sa == NULL)
		return NULL;

	res = newisakmpsa();
	if(res == NULL)
		return NULL;

	*res = *sa;
#ifdef HAVE_GSSAPI
	if (sa->gssid != NULL)
		res->gssid = vdup(sa->gssid);
#endif
	res->next = NULL;

	if(sa->dhgrp != NULL)
		oakley_setdhgroup(sa->dh_group, &res->dhgrp);

	return res;

}

#ifdef ENABLE_HYBRID
int
isakmpsa_switch_authmethod(authmethod)
	int authmethod;
{
	switch(authmethod) {
	case OAKLEY_ATTR_AUTH_METHOD_HYBRID_RSA_R:
		authmethod = OAKLEY_ATTR_AUTH_METHOD_HYBRID_RSA_I;
		break;
	case OAKLEY_ATTR_AUTH_METHOD_HYBRID_DSS_R:
		authmethod = OAKLEY_ATTR_AUTH_METHOD_HYBRID_DSS_I;
		break;
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_PSKEY_R:
		authmethod = OAKLEY_ATTR_AUTH_METHOD_XAUTH_PSKEY_I;
		break;
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSASIG_R:
		authmethod = OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSASIG_I;
		break;
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_DSSSIG_R:
		authmethod = OAKLEY_ATTR_AUTH_METHOD_XAUTH_DSSSIG_I;
		break;
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAENC_R:
		authmethod = OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAENC_I;
		break;
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAREV_R:
		authmethod = OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAREV_I;
		break;
	default:
		break;
	}

	return authmethod;
}
#endif

/*
 * Given a proposed ISAKMP SA, and a list of acceptable
 * ISAKMP SAs, it compares using pcheck_level policy and
 * returns first match (if any).
 */
struct isakmpsa *
checkisakmpsa(pcheck_level, proposal, acceptable)
	int pcheck_level;
	struct isakmpsa *proposal, *acceptable;
{
	struct isakmpsa *p;

	for (p = acceptable; p != NULL; p = p->next){
		if (proposal->authmethod != isakmpsa_switch_authmethod(p->authmethod) ||
		    proposal->enctype != p->enctype ||
                    proposal->dh_group != p->dh_group ||
		    proposal->hashtype != p->hashtype)
			continue;

		switch (pcheck_level) {
		case PROP_CHECK_OBEY:
			break;

		case PROP_CHECK_CLAIM:
		case PROP_CHECK_STRICT:
			if (proposal->encklen < p->encklen ||
#if 0
			    proposal->lifebyte > p->lifebyte ||
#endif
			    proposal->lifetime > p->lifetime)
				continue;
			break;

		case PROP_CHECK_EXACT:
			if (proposal->encklen != p->encklen ||
#if 0
                            proposal->lifebyte != p->lifebyte ||
#endif
			    proposal->lifetime != p->lifetime)
				continue;
			break;

		default:
			continue;
		}

		return p;
	}

	return NULL;
}

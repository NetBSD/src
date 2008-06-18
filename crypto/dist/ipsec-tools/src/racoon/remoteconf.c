/*	$NetBSD: remoteconf.c,v 1.9.4.2 2008/06/18 07:30:19 mgrooms Exp $	*/

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
#include "genlist.h"

static TAILQ_HEAD(_rmtree, remoteconf) rmtree, rmtree_save, rmtree_tmp;

/* 
 * Script hook names and script hook paths
 */
char *script_names[SCRIPT_MAX + 1] = { "phase1_up", "phase1_down" };

/*%%%*/
/*
 * search remote configuration.
 * don't use port number to search if its value is either IPSEC_PORT_ANY.
 * If matching anonymous entry, then new entry is copied from anonymous entry.
 * If no anonymous entry found, then return NULL.
 * OUT:	NULL:	NG
 *	Other:	remote configuration entry.
 */
struct remoteconf *
getrmconf_strict(remote, allow_anon)
	struct sockaddr *remote;
	int allow_anon;
{
	struct remoteconf *p;
	struct remoteconf *anon = NULL;
	int withport;
	char buf[NI_MAXHOST + NI_MAXSERV + 10];
	char addr[NI_MAXHOST], port[NI_MAXSERV];

	withport = 0;

#ifndef ENABLE_NATT
	/* 
	 * We never have ports set in our remote configurations, but when
	 * NAT-T is enabled, the kernel can have policies with ports and
	 * send us an acquire message for a destination that has a port set.
	 * If we do this port check here, we don't find the remote config.
	 *
	 * In an ideal world, we would be able to have remote conf with
	 * port, and the port could be a wildcard. That test could be used.
	 */
	if (remote->sa_family != AF_UNSPEC &&
	    extract_port(remote) != IPSEC_PORT_ANY)
		withport = 1;
#endif /* ENABLE_NATT */

	if (remote->sa_family == AF_UNSPEC)
		snprintf (buf, sizeof(buf), "%s", "anonymous");
	else {
		GETNAMEINFO(remote, addr, port);
		snprintf(buf, sizeof(buf), "%s%s%s%s", addr,
			withport ? "[" : "",
			withport ? port : "",
			withport ? "]" : "");
	}

	TAILQ_FOREACH(p, &rmtree, chain) {
		if ((remote->sa_family == AF_UNSPEC
		     && remote->sa_family == p->remote->sa_family)
		 || (!withport && cmpsaddrwop(remote, p->remote) == 0)
		 || (withport && cmpsaddrstrict(remote, p->remote) == 0)) {
			plog(LLV_DEBUG, LOCATION, NULL,
				"configuration found for %s.\n", buf);
			return p;
		}

		/* save the pointer to the anonymous configuration */
		if (p->remote->sa_family == AF_UNSPEC)
			anon = p;
	}

	if (allow_anon && anon != NULL) {
		plog(LLV_DEBUG, LOCATION, NULL,
			"anonymous configuration selected for %s.\n", buf);
		return anon;
	}

	plog(LLV_DEBUG, LOCATION, NULL,
		"no remote configuration found.\n");

	return NULL;
}

struct remoteconf *
getrmconf(remote)
	struct sockaddr *remote;
{
	return getrmconf_strict(remote, 1);
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
	new->getcert_method = ISAKMP_GETCERT_PAYLOAD;
	new->getcacert_method = ISAKMP_GETCERT_LOCALFILE;
	new->cacerttype = ISAKMP_CERT_X509SIGN;
	new->certtype = ISAKMP_CERT_NONE;
	new->cacertfile = NULL;
	new->send_cert = TRUE;
	new->send_cr = TRUE;
	new->support_proxy = FALSE;
	for (i = 0; i <= SCRIPT_MAX; i++)
		new->script[i] = NULL;
	new->gen_policy = FALSE;
	new->retry_counter = lcconf->retry_counter;
	new->retry_interval = lcconf->retry_interval;
	new->nat_traversal = FALSE;
	new->rsa_private = genlist_init();
	new->rsa_public = genlist_init();
	new->idv = NULL;
	new->key = NULL;

	new->dpd = TRUE; /* Enable DPD support by default */
	new->dpd_interval = 0; /* Disable DPD checks by default */
	new->dpd_retry = 5;
	new->dpd_maxfails = 5;

	new->weak_phase1_check = 0;

#ifdef ENABLE_HYBRID
	new->xauth = NULL;
#endif

	return new;
}

struct remoteconf *
copyrmconf(remote)
	struct sockaddr *remote;
{
	struct remoteconf *new, *old;

	old = getrmconf_strict (remote, 0);
	if (old == NULL) {
		plog (LLV_ERROR, LOCATION, NULL,
		      "Remote configuration for '%s' not found!\n",
		      saddr2str (remote));
		return NULL;
	}

	new = duprmconf (old);

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

	new = racoon_calloc(1, sizeof(*new));
	if (new == NULL)
		return NULL;
	memcpy (new, rmconf, sizeof (*new));
	// FIXME: We should duplicate the proposal as well.
	// This is now handled in the cfparse.y
	// new->proposal = ...;
	
	/* duplicate dynamic structures */
	if (new->etypes)
		new->etypes=dupetypes(new->etypes);
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
struct etypes *
check_etypeok(rmconf, etype)
	struct remoteconf *rmconf;
	u_int8_t etype;
{
	struct etypes *e;

	for (e = rmconf->etypes; e != NULL; e = e->next) {
		if (e->type == etype)
			break;
	}

	return e;
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
	new->rmconf = NULL;
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

	new->rmconf = rmconf;

	if (rmconf->proposal == NULL) {
		rmconf->proposal = new;
		return;
	}

	for (p = rmconf->proposal; p->next != NULL; p = p->next)
		;
	p->next = new;

	return;
}

struct remoteconf *
foreachrmconf(rmconf_func_t rmconf_func, void *data)
{
  struct remoteconf *p, *ret = NULL;
  RACOON_TAILQ_FOREACH_REVERSE(p, &rmtree, _rmtree, chain) {
    ret = (*rmconf_func)(p, data);
    if (ret)
      break;
  }

  return ret;
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

static struct remoteconf *
dump_rmconf_single (struct remoteconf *p, void *data)
{
	struct etypes *etype = p->etypes;
	struct isakmpsa *prop = p->proposal;
	char buf[1024], *pbuf;

	pbuf = buf;
	pbuf += sprintf(pbuf, "remote %s", saddr2str(p->remote));
	if (p->inherited_from)
		pbuf += sprintf(pbuf, " inherit %s",
				saddr2str(p->inherited_from->remote));
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
		plog(LLV_INFO, LOCATION, NULL, "\tcertificate_type %s \"%s\" \"%s\";\n",
			p->certtype == ISAKMP_CERT_X509SIGN ? "x509" : "*UNKNOWN*",
			p->mycertfile, p->myprivfile);
		switch (p->getcert_method) {
		  case 0:
		  	break;
		  case ISAKMP_GETCERT_PAYLOAD:
			plog(LLV_INFO, LOCATION, NULL, "\t/* peers certificate from payload */\n");
			break;
		  case ISAKMP_GETCERT_LOCALFILE:
			plog(LLV_INFO, LOCATION, NULL, "\tpeers_certfile \"%s\";\n", p->peerscertfile);
			break;
		  case ISAKMP_GETCERT_DNS:
			plog(LLV_INFO, LOCATION, NULL, "\tpeer_certfile dnssec;\n");
			break;
		  default:
			plog(LLV_INFO, LOCATION, NULL, "\tpeers_certfile *UNKNOWN* (%d)\n", p->getcert_method);
		}
	}
	else {
		if (p->idv)
			pbuf += sprintf (pbuf, " \"%s\"", p->idv->v);
		plog(LLV_INFO, LOCATION, NULL, "%s;\n", buf);
		genlist_foreach(p->idvl_p, &dump_peers_identifiers, NULL);
	}

	plog(LLV_INFO, LOCATION, NULL, "\tsend_cert %s;\n",
		s_switch (p->send_cert));
	plog(LLV_INFO, LOCATION, NULL, "\tsend_cr %s;\n",
		s_switch (p->send_cr));
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
			"\t/* prop_no=%d, trns_no=%d, rmconf=%s */\n",
			prop->prop_no, prop->trns_no,
			saddr2str(prop->rmconf->remote));
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

	return NULL;
}

void
dumprmconf()
{
	foreachrmconf (dump_rmconf_single, NULL);
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
	struct isakmpsa *res=NULL;

	if(sa == NULL)
		return NULL;

	res=newisakmpsa();
	if(res == NULL)
		return NULL;

	*res=*sa;
#ifdef HAVE_GSSAPI
	/* XXX gssid
	 */
#endif
	res->next=NULL;

	if(sa->dhgrp != NULL)
		oakley_setdhgroup (sa->dh_group, &(res->dhgrp));

	return res;

}

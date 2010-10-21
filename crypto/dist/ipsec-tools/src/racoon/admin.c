/*	$NetBSD: admin.c,v 1.34 2010/10/21 06:04:33 tteras Exp $	*/

/* Id: admin.c,v 1.25 2006/04/06 14:31:04 manubsd Exp */

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
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <net/pfkeyv2.h>

#include <netinet/in.h>
#include PATH_IPSEC_H


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef ENABLE_HYBRID
#include <resolv.h>
#endif

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "sockmisc.h"
#include "debug.h"

#include "schedule.h"
#include "localconf.h"
#include "remoteconf.h"
#include "grabmyaddr.h"
#include "isakmp_var.h"
#include "isakmp.h"
#include "oakley.h"
#include "handler.h"
#include "evt.h"
#include "pfkey.h"
#include "ipsec_doi.h"
#include "policy.h"
#include "admin.h"
#include "admin_var.h"
#include "isakmp_inf.h"
#ifdef ENABLE_HYBRID
#include "isakmp_cfg.h"
#endif
#include "session.h"
#include "gcmalloc.h"

#ifdef ENABLE_ADMINPORT
char *adminsock_path = ADMINSOCK_PATH;
uid_t adminsock_owner = 0;
gid_t adminsock_group = 0;
mode_t adminsock_mode = 0600;

static struct sockaddr_un sunaddr;
static int admin_process __P((int, char *));
static int admin_reply __P((int, struct admin_com *, int, vchar_t *));

static int
admin_handler(ctx, fd)
	void *ctx;
	int fd;
{
	int so2;
	struct sockaddr_storage from;
	socklen_t fromlen = sizeof(from);
	struct admin_com com;
	char *combuf = NULL;
	int len, error = -1;

	so2 = accept(lcconf->sock_admin, (struct sockaddr *)&from, &fromlen);
	if (so2 < 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to accept admin command: %s\n",
			strerror(errno));
		return -1;
	}
	close_on_exec(so2);

	/* get buffer length */
	while ((len = recv(so2, (char *)&com, sizeof(com), MSG_PEEK)) < 0) {
		if (errno == EINTR)
			continue;
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to recv admin command: %s\n",
			strerror(errno));
		goto end;
	}

	/* sanity check */
	if (len < sizeof(com)) {
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid header length of admin command\n");
		goto end;
	}

	/* get buffer to receive */
	if ((combuf = racoon_malloc(com.ac_len)) == 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to alloc buffer for admin command\n");
		goto end;
	}

	/* get real data */
	while ((len = recv(so2, combuf, com.ac_len, 0)) < 0) {
		if (errno == EINTR)
			continue;
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to recv admin command: %s\n",
			strerror(errno));
		goto end;
	}

	error = admin_process(so2, combuf);

end:
	if (error == -2) {
		plog(LLV_DEBUG, LOCATION, NULL,
			"[%d] admin connection established\n", so2);
	} else {
		(void)close(so2);
	}

	if (combuf)
		racoon_free(combuf);

	return error;
}

static int admin_ph1_delete_sa(struct ph1handle *iph1, void *arg)
{
	if (iph1->status >= PHASE1ST_ESTABLISHED)
		isakmp_info_send_d1(iph1);
	purge_remote(iph1);
	return 0;
}

/*
 * main child's process.
 */
static int
admin_process(so2, combuf)
	int so2;
	char *combuf;
{
	struct admin_com *com = (struct admin_com *)combuf;
	vchar_t *buf = NULL;
	vchar_t *id = NULL;
	vchar_t *key = NULL;
	int idtype = 0;
	int error = 0, l_ac_errno = 0;
	struct evt_listener_list *event_list = NULL;

	if (com->ac_cmd & ADMIN_FLAG_VERSION)
		com->ac_cmd &= ~ADMIN_FLAG_VERSION;
	else
		com->ac_version = 0;

	switch (com->ac_cmd) {
	case ADMIN_RELOAD_CONF:
		signal_handler(SIGHUP);
		break;

	case ADMIN_SHOW_SCHED: {
		caddr_t p = NULL;
		int len;

		if (sched_dump(&p, &len) != -1) {
			buf = vmalloc(len);
			if (buf != NULL)
				memcpy(buf->v, p, len);
			else
				l_ac_errno = ENOMEM;
			racoon_free(p);
		} else
			l_ac_errno = ENOMEM;
		break;
	}

	case ADMIN_SHOW_EVT:
		if (com->ac_version == 0) {
			buf = evt_dump();
			l_ac_errno = 0;
		}
		break;

	case ADMIN_SHOW_SA:
		switch (com->ac_proto) {
		case ADMIN_PROTO_ISAKMP:
			buf = dumpph1();
			if (buf == NULL)
				l_ac_errno = ENOMEM;
			break;
		case ADMIN_PROTO_IPSEC:
		case ADMIN_PROTO_AH:
		case ADMIN_PROTO_ESP: {
			u_int p;
			p = admin2pfkey_proto(com->ac_proto);
			if (p != -1) {
				buf = pfkey_dump_sadb(p);
				if (buf == NULL)
					l_ac_errno = ENOMEM;
			} else
				l_ac_errno = EINVAL;
			break;
		}
		case ADMIN_PROTO_INTERNAL:
		default:
			l_ac_errno = ENOTSUP;
			break;
		}
		break;

	case ADMIN_GET_SA_CERT: {
		struct admin_com_indexes *ndx;
		struct sockaddr *src, *dst;
		struct ph1handle *iph1;

		ndx = (struct admin_com_indexes *) ((caddr_t)com + sizeof(*com));
		src = (struct sockaddr *) &ndx->src;
		dst = (struct sockaddr *) &ndx->dst;

		if (com->ac_proto != ADMIN_PROTO_ISAKMP) {
			l_ac_errno = ENOTSUP;
			break;
		}

		iph1 = getph1byaddr(src, dst, 0);
		if (iph1 == NULL) {
			l_ac_errno = ENOENT;
			break;
		}

		if (iph1->cert_p != NULL) {
			vchar_t tmp;
			tmp.v = iph1->cert_p->v + 1;
			tmp.l = iph1->cert_p->l - 1;
			buf = vdup(&tmp);
		}
		break;
	}

	case ADMIN_FLUSH_SA:
		switch (com->ac_proto) {
		case ADMIN_PROTO_ISAKMP:
			flushph1();
			break;
		case ADMIN_PROTO_IPSEC:
		case ADMIN_PROTO_AH:
		case ADMIN_PROTO_ESP:
			pfkey_flush_sadb(com->ac_proto);
			break;
		case ADMIN_PROTO_INTERNAL:
			/*XXX flushph2();*/
		default:
			l_ac_errno = ENOTSUP;
			break;
		}
		break;

	case ADMIN_DELETE_SA: {
		char *loc, *rem;
		struct ph1selector sel;

		memset(&sel, 0, sizeof(sel));
		sel.local = (struct sockaddr *)
			&((struct admin_com_indexes *)
			    ((caddr_t)com + sizeof(*com)))->src;
		sel.remote = (struct sockaddr *)
			&((struct admin_com_indexes *)
			    ((caddr_t)com + sizeof(*com)))->dst;

		loc = racoon_strdup(saddr2str(sel.local));
		rem = racoon_strdup(saddr2str(sel.remote));
		STRDUP_FATAL(loc);
		STRDUP_FATAL(rem);

		plog(LLV_INFO, LOCATION, NULL,
		     "admin delete-sa %s %s\n", loc, rem);
		enumph1(&sel, admin_ph1_delete_sa, NULL);
		remcontacted(sel.remote);

		racoon_free(loc);
		racoon_free(rem);
		break;
	}

#ifdef ENABLE_HYBRID
	case ADMIN_LOGOUT_USER: {
		struct ph1handle *iph1;
		char user[LOGINLEN+1];
		int found = 0, len = com->ac_len - sizeof(*com);

		if (len > LOGINLEN) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "malformed message (login too long)\n");
			break;
		}

		memcpy(user, (char *)(com + 1), len);
		user[len] = 0;

		found = purgeph1bylogin(user);
		plog(LLV_INFO, LOCATION, NULL,
		    "deleted %d SA for user \"%s\"\n", found, user);

		break;
	}
#endif

	case ADMIN_DELETE_ALL_SA_DST: {
		struct ph1handle *iph1;
		struct sockaddr *dst;
		char *loc, *rem;

		dst = (struct sockaddr *)
			&((struct admin_com_indexes *)
			    ((caddr_t)com + sizeof(*com)))->dst;

		rem = racoon_strdup(saddrwop2str(dst));
		STRDUP_FATAL(rem);

		plog(LLV_INFO, LOCATION, NULL,
		    "Flushing all SAs for peer %s\n", rem);

		while ((iph1 = getph1bydstaddr(dst)) != NULL) {
			loc = racoon_strdup(saddrwop2str(iph1->local));
			STRDUP_FATAL(loc);

			if (iph1->status >= PHASE1ST_ESTABLISHED)
				isakmp_info_send_d1(iph1);
			purge_remote(iph1);

			racoon_free(loc);
		}

		racoon_free(rem);
		break;
	}

	case ADMIN_ESTABLISH_SA_PSK: {
		struct admin_com_psk *acp;
		char *data;

		acp = (struct admin_com_psk *)
		    ((char *)com + sizeof(*com) +
		    sizeof(struct admin_com_indexes));

		idtype = acp->id_type;

		if ((id = vmalloc(acp->id_len)) == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "cannot allocate memory: %s\n",
			    strerror(errno));
			break;
		}
		data = (char *)(acp + 1);
		memcpy(id->v, data, id->l);

		if ((key = vmalloc(acp->key_len)) == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "cannot allocate memory: %s\n",
			    strerror(errno));
			vfree(id);
			id = NULL;
			break;
		}
		data = (char *)(data + acp->id_len);
		memcpy(key->v, data, key->l);
	}
	/* FALLTHROUGH */
	case ADMIN_ESTABLISH_SA: {
		struct admin_com_indexes *ndx;
		struct sockaddr *dst;
		struct sockaddr *src;
		char *name = NULL;

		ndx = (struct admin_com_indexes *) ((caddr_t)com + sizeof(*com));
		src = (struct sockaddr *) &ndx->src;
		dst = (struct sockaddr *) &ndx->dst;

		if (com->ac_cmd == ADMIN_ESTABLISH_SA &&
		    com->ac_len > sizeof(*com) + sizeof(*ndx))
			name = (char *) ((caddr_t) ndx + sizeof(*ndx));

		switch (com->ac_proto) {
		case ADMIN_PROTO_ISAKMP: {
			struct ph1handle *ph1;
			struct remoteconf *rmconf;
			u_int16_t port;

			l_ac_errno = -1;

			/* connected already? */
			ph1 = getph1byaddr(src, dst, 0);
			if (ph1 != NULL) {
				event_list = &ph1->evt_listeners;
				if (ph1->status == PHASE1ST_ESTABLISHED)
					l_ac_errno = EEXIST;
				else
					l_ac_errno = 0;
				break;
			}

			/* search appropreate configuration */
			if (name == NULL)
				rmconf = getrmconf(dst, 0);
			else
				rmconf = getrmconf_by_name(name);
			if (rmconf == NULL) {
				plog(LLV_ERROR, LOCATION, NULL,
					"no configuration found "
					"for %s\n", saddrwop2str(dst));
				break;
			}

#ifdef ENABLE_HYBRID
			/* XXX This overwrites rmconf information globally. */
			/* Set the id and key */
			if (id && key) {
				if (xauth_rmconf_used(&rmconf->xauth) == -1)
					break;

				if (rmconf->xauth->login != NULL) {
					vfree(rmconf->xauth->login);
					rmconf->xauth->login = NULL;
				}
				if (rmconf->xauth->pass != NULL) {
					vfree(rmconf->xauth->pass);
					rmconf->xauth->pass = NULL;
				}

				rmconf->xauth->login = id;
				rmconf->xauth->pass = key;
			}
#endif

			plog(LLV_INFO, LOCATION, NULL,
				"accept a request to establish IKE-SA: "
				"%s\n", saddrwop2str(dst));

			/* begin ident mode */
			ph1 = isakmp_ph1begin_i(rmconf, dst, src);
			if (ph1 == NULL)
				break;

			event_list = &ph1->evt_listeners;
			l_ac_errno = 0;
			break;
		}
		case ADMIN_PROTO_AH:
		case ADMIN_PROTO_ESP: {
			struct ph2handle *iph2;
			struct secpolicy *sp_out = NULL, *sp_in = NULL;
			struct policyindex spidx;

			l_ac_errno = -1;

			/* got outbound policy */
			memset(&spidx, 0, sizeof(spidx));
			spidx.dir = IPSEC_DIR_OUTBOUND;
			memcpy(&spidx.src, src, sizeof(spidx.src));
			memcpy(&spidx.dst, dst, sizeof(spidx.dst));
			spidx.prefs = ndx->prefs;
			spidx.prefd = ndx->prefd;
			spidx.ul_proto = ndx->ul_proto;

			sp_out = getsp_r(&spidx);
			if (sp_out) {
				plog(LLV_DEBUG, LOCATION, NULL,
					"suitable outbound SP found: %s.\n",
					spidx2str(&sp_out->spidx));
			} else {
				l_ac_errno = ENOENT;
				plog(LLV_NOTIFY, LOCATION, NULL,
					"no outbound policy found: %s\n",
					spidx2str(&spidx));
				break;
			}

			iph2 = getph2byid(src, dst, sp_out->id);
			if (iph2 != NULL) {
				event_list = &iph2->evt_listeners;
				if (iph2->status == PHASE2ST_ESTABLISHED)
					l_ac_errno = EEXIST;
				else
					l_ac_errno = 0;
				break;
			}

			/* get inbound policy */
			memset(&spidx, 0, sizeof(spidx));
			spidx.dir = IPSEC_DIR_INBOUND;
			memcpy(&spidx.src, dst, sizeof(spidx.src));
			memcpy(&spidx.dst, src, sizeof(spidx.dst));
			spidx.prefs = ndx->prefd;
			spidx.prefd = ndx->prefs;
			spidx.ul_proto = ndx->ul_proto;

			sp_in = getsp_r(&spidx);
			if (sp_in) {
				plog(LLV_DEBUG, LOCATION, NULL,
					"suitable inbound SP found: %s.\n",
					spidx2str(&sp_in->spidx));
			} else {
				l_ac_errno = ENOENT;
				plog(LLV_NOTIFY, LOCATION, NULL,
					"no inbound policy found: %s\n",
				spidx2str(&spidx));
				break;
			}

			/* allocate a phase 2 */
			iph2 = newph2();
			if (iph2 == NULL) {
				plog(LLV_ERROR, LOCATION, NULL,
					"failed to allocate phase2 entry.\n");
				break;
			}
			iph2->side = INITIATOR;
			iph2->satype = admin2pfkey_proto(com->ac_proto);
			iph2->spid = sp_out->id;
			iph2->seq = pk_getseq();
			iph2->status = PHASE2ST_STATUS2;

			/* set end addresses of SA */
			iph2->dst = dupsaddr(dst);
			iph2->src = dupsaddr(src);
			if (iph2->dst == NULL || iph2->src == NULL) {
				delph2(iph2);
				break;
			}

			if (isakmp_get_sainfo(iph2, sp_out, sp_in) < 0) {
				delph2(iph2);
				break;
			}

			insph2(iph2);
			if (isakmp_post_acquire(iph2, NULL) < 0) {
				remph2(iph2);
				delph2(iph2);
				break;
			}

			event_list = &iph2->evt_listeners;
			l_ac_errno = 0;
			break;
		}
		default:
			/* ignore */
			l_ac_errno = ENOTSUP;
		}
		break;
	}

	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid command: %d\n", com->ac_cmd);
		l_ac_errno = ENOTSUP;
	}

	if ((error = admin_reply(so2, com, l_ac_errno, buf)) != 0)
		goto out;

	/* start pushing events if so requested */
	if ((l_ac_errno == 0) &&
	    (com->ac_version >= 1) &&
	    (com->ac_cmd == ADMIN_SHOW_EVT || event_list != NULL))
		error = evt_subscribe(event_list, so2);
out:
	if (buf != NULL)
		vfree(buf);

	return error;
}

static int
admin_reply(so, req, l_ac_errno, buf)
	int so, l_ac_errno;
	struct admin_com *req;
	vchar_t *buf;
{
	int tlen;
	struct admin_com *combuf;
	char *retbuf = NULL;

	if (buf != NULL)
		tlen = sizeof(*combuf) + buf->l;
	else
		tlen = sizeof(*combuf);

	retbuf = racoon_calloc(1, tlen);
	if (retbuf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to allocate admin buffer\n");
		return -1;
	}

	combuf = (struct admin_com *) retbuf;
	combuf->ac_len = tlen;
	combuf->ac_cmd = req->ac_cmd & ~ADMIN_FLAG_VERSION;
	combuf->ac_errno = l_ac_errno;
	combuf->ac_proto = req->ac_proto;

	if (buf != NULL)
		memcpy(retbuf + sizeof(*combuf), buf->v, buf->l);

	tlen = send(so, retbuf, tlen, 0);
	racoon_free(retbuf);
	if (tlen < 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to send admin command: %s\n",
			strerror(errno));
		return -1;
	}

	return 0;
}

/* ADMIN_PROTO -> SADB_SATYPE */
int
admin2pfkey_proto(proto)
	u_int proto;
{
	switch (proto) {
	case ADMIN_PROTO_IPSEC:
		return SADB_SATYPE_UNSPEC;
	case ADMIN_PROTO_AH:
		return SADB_SATYPE_AH;
	case ADMIN_PROTO_ESP:
		return SADB_SATYPE_ESP;
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"unsupported proto for admin: %d\n", proto);
		return -1;
	}
	/*NOTREACHED*/
}

int
admin_init()
{
	if (adminsock_path == NULL) {
		lcconf->sock_admin = -1;
		return 0;
	}

	memset(&sunaddr, 0, sizeof(sunaddr));
	sunaddr.sun_family = AF_UNIX;
	snprintf(sunaddr.sun_path, sizeof(sunaddr.sun_path),
		"%s", adminsock_path);

	lcconf->sock_admin = socket(AF_UNIX, SOCK_STREAM, 0);
	if (lcconf->sock_admin == -1) {
		plog(LLV_ERROR, LOCATION, NULL,
			"socket: %s\n", strerror(errno));
		return -1;
	}
	close_on_exec(lcconf->sock_admin);

	unlink(sunaddr.sun_path);
	if (bind(lcconf->sock_admin, (struct sockaddr *)&sunaddr,
			sizeof(sunaddr)) != 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"bind(sockname:%s): %s\n",
			sunaddr.sun_path, strerror(errno));
		(void)close(lcconf->sock_admin);
		return -1;
	}

	if (chown(sunaddr.sun_path, adminsock_owner, adminsock_group) != 0) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "chown(%s, %d, %d): %s\n",
		    sunaddr.sun_path, adminsock_owner,
		    adminsock_group, strerror(errno));
		(void)close(lcconf->sock_admin);
		return -1;
	}

	if (chmod(sunaddr.sun_path, adminsock_mode) != 0) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "chmod(%s, 0%03o): %s\n",
		    sunaddr.sun_path, adminsock_mode, strerror(errno));
		(void)close(lcconf->sock_admin);
		return -1;
	}

	if (listen(lcconf->sock_admin, 5) != 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"listen(sockname:%s): %s\n",
			sunaddr.sun_path, strerror(errno));
		(void)close(lcconf->sock_admin);
		return -1;
	}

	monitor_fd(lcconf->sock_admin, admin_handler, NULL);
	plog(LLV_DEBUG, LOCATION, NULL,
	     "open %s as racoon management.\n", sunaddr.sun_path);

	return 0;
}

int
admin_close()
{
	unmonitor_fd(lcconf->sock_admin);
	close(lcconf->sock_admin);
	return 0;
}

#endif

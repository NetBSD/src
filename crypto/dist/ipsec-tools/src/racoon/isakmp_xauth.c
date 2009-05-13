/*	$NetBSD: isakmp_xauth.c,v 1.19.2.1 2009/05/13 19:15:54 jym Exp $	*/

/* Id: isakmp_xauth.c,v 1.38 2006/08/22 18:17:17 manubsd Exp */

/*
 * Copyright (C) 2004-2005 Emmanuel Dreyfus
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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include <netdb.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>
#include <resolv.h>

#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "sockmisc.h"
#include "schedule.h"
#include "debug.h"

#include "crypto_openssl.h"
#include "isakmp_var.h"
#include "isakmp.h"
#include "admin.h"
#include "privsep.h"
#include "evt.h"
#include "handler.h"
#include "throttle.h"
#include "remoteconf.h"
#include "isakmp_inf.h"
#include "isakmp_xauth.h"
#include "isakmp_unity.h"
#include "isakmp_cfg.h"
#include "strnames.h"
#include "ipsec_doi.h"
#include "remoteconf.h"
#include "localconf.h"

#ifdef HAVE_LIBRADIUS
#include <radlib.h>
struct rad_handle *radius_auth_state = NULL;
struct rad_handle *radius_acct_state = NULL;
struct xauth_rad_config xauth_rad_config;
#endif

#ifdef HAVE_LIBPAM
#include <security/pam_appl.h>

static char *PAM_usr = NULL;
static char *PAM_pwd = NULL;
static int PAM_conv(int, const struct pam_message **, 
    struct pam_response **, void *);
static struct pam_conv PAM_chat = { &PAM_conv, NULL };
#endif

#ifdef HAVE_LIBLDAP
#include "ldap.h"
#include <arpa/inet.h>
struct xauth_ldap_config xauth_ldap_config;
#endif

void 
xauth_sendreq(iph1)
	struct ph1handle *iph1;
{
	vchar_t *buffer;
	struct isakmp_pl_attr *attr;
	struct isakmp_data *typeattr;
	struct isakmp_data *usrattr;
	struct isakmp_data *pwdattr;
	struct xauth_state *xst = &iph1->mode_cfg->xauth;
	size_t tlen;

	/* Status checks */
	if (iph1->status < PHASE1ST_ESTABLISHED) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Xauth request while phase 1 is not completed\n");
		return;
	}

	if (xst->status != XAUTHST_NOTYET) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Xauth request whith Xauth state %d\n", xst->status);
		return;
	}

	plog(LLV_INFO, LOCATION, NULL, "Sending Xauth request\n");

	tlen = sizeof(*attr) +
	       + sizeof(*typeattr) +
	       + sizeof(*usrattr) +
	       + sizeof(*pwdattr);
	
	if ((buffer = vmalloc(tlen)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, "Cannot allocate buffer\n");
		return;
	}
	
	attr = (struct isakmp_pl_attr *)buffer->v;
	memset(attr, 0, tlen);

	attr->h.len = htons(tlen);
	attr->type = ISAKMP_CFG_REQUEST;
	attr->id = htons(eay_random());

	typeattr = (struct isakmp_data *)(attr + 1);
	typeattr->type = htons(XAUTH_TYPE | ISAKMP_GEN_TV);
	typeattr->lorv = htons(XAUTH_TYPE_GENERIC);

	usrattr = (struct isakmp_data *)(typeattr + 1);
	usrattr->type = htons(XAUTH_USER_NAME | ISAKMP_GEN_TLV);
	usrattr->lorv = htons(0);

	pwdattr = (struct isakmp_data *)(usrattr + 1);
	pwdattr->type = htons(XAUTH_USER_PASSWORD | ISAKMP_GEN_TLV);
	pwdattr->lorv = htons(0);

	isakmp_cfg_send(iph1, buffer, 
	    ISAKMP_NPTYPE_ATTR, ISAKMP_FLAG_E, 1);
	
	vfree(buffer);

	xst->status = XAUTHST_REQSENT;

	return;
}

int
xauth_attr_reply(iph1, attr, id)
	struct ph1handle *iph1;
	struct isakmp_data *attr;
	int id;
{
	char **outlet = NULL;
	size_t alen = 0;
	int type;
	struct xauth_state *xst = &iph1->mode_cfg->xauth;

	if ((iph1->mode_cfg->flags & ISAKMP_CFG_VENDORID_XAUTH) == 0) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Xauth reply but peer did not declare "
		    "itself as Xauth capable\n");
		return -1;
	}

	if (xst->status != XAUTHST_REQSENT) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Xauth reply while Xauth state is %d\n", xst->status);
		return -1;
	}

	type = ntohs(attr->type) & ~ISAKMP_GEN_MASK;
	switch (type) {
	case XAUTH_TYPE:
		switch (ntohs(attr->lorv)) {
		case XAUTH_TYPE_GENERIC:
			xst->authtype = XAUTH_TYPE_GENERIC;
			break;
		default:
			plog(LLV_WARNING, LOCATION, NULL, 
			    "Unexpected authentication type %d\n", 
			    ntohs(type));
			return -1;
		}
		break;

	case XAUTH_USER_NAME:
		outlet = &xst->authdata.generic.usr;
		break;

	case XAUTH_USER_PASSWORD:
		outlet = &xst->authdata.generic.pwd; 
		break;

	default:
		plog(LLV_WARNING, LOCATION, NULL, 
		    "ignored Xauth attribute %d\n", type);
		break;
	}

	if (outlet != NULL) {
		alen = ntohs(attr->lorv);

		if ((*outlet = racoon_malloc(alen + 1)) == NULL) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Cannot allocate memory for Xauth Data\n");
			return -1;
		}

		memcpy(*outlet, attr + 1, alen);
		(*outlet)[alen] = '\0';
		outlet = NULL;
	}

	
	if ((xst->authdata.generic.usr != NULL) &&
	   (xst->authdata.generic.pwd != NULL)) {
		int port;
		int res;
		char *usr = xst->authdata.generic.usr;
		char *pwd = xst->authdata.generic.pwd;
		time_t throttle_delay = 0;

#if 0	/* Real debug, don't do that at home */
		plog(LLV_DEBUG, LOCATION, NULL, 
		    "Got username \"%s\", password \"%s\"\n", usr, pwd);
#endif
		strncpy(iph1->mode_cfg->login, usr, LOGINLEN);
		iph1->mode_cfg->login[LOGINLEN] = '\0';

		res = -1;
		if ((port = isakmp_cfg_getport(iph1)) == -1) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Port pool depleted\n");
			goto skip_auth;
		}	

		switch (isakmp_cfg_config.authsource) {
		case ISAKMP_CFG_AUTH_SYSTEM:
			res = privsep_xauth_login_system(usr, pwd);
			break;
#ifdef HAVE_LIBRADIUS
		case ISAKMP_CFG_AUTH_RADIUS:
			res = xauth_login_radius(iph1, usr, pwd);
			break;
#endif
#ifdef HAVE_LIBPAM
		case ISAKMP_CFG_AUTH_PAM:
			res = privsep_xauth_login_pam(iph1->mode_cfg->port, 
			    iph1->remote, usr, pwd);
			break;
#endif
#ifdef HAVE_LIBLDAP
		case ISAKMP_CFG_AUTH_LDAP:
			res = xauth_login_ldap(iph1, usr, pwd);
			break;
#endif
		default:
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Unexpected authentication source\n");
			res = -1;
			break;
		}

		/*
		 * Optional group authentication
		 */
		if (!res && (isakmp_cfg_config.groupcount))
			res = group_check(iph1,
				isakmp_cfg_config.grouplist,
				isakmp_cfg_config.groupcount);

		/*
		 * On failure, throttle the connexion for the remote host
		 * in order to make password attacks more difficult.
		 */
		throttle_delay = throttle_host(iph1->remote, res);
		if (throttle_delay > 0) {
			char *str;

			str = saddrwop2str(iph1->remote);

			plog(LLV_ERROR, LOCATION, NULL, 
			    "Throttling in action for %s: delay %lds\n",
			    str, (unsigned long)throttle_delay);
			res = -1;
		} else {
			throttle_delay = 0;
		}

skip_auth:
		if (throttle_delay != 0) {
			struct xauth_reply_arg *xra;

			if ((xra = racoon_calloc(1, sizeof(*xra))) == NULL) {
				plog(LLV_ERROR, LOCATION, NULL, 
				    "malloc failed, bypass throttling\n");
				return xauth_reply(iph1, port, id, res);
			}

			/*
			 * We need to store the ph1, but it might have
			 * disapeared when xauth_reply is called, so
			 * store the index instead.
			 */
			xra->index = iph1->index;
			xra->port = port;
			xra->id = id;
			xra->res = res;
			sched_schedule(&xra->sc, throttle_delay,
				       xauth_reply_stub);
		} else {
			return xauth_reply(iph1, port, id, res);
		}
	}

	return 0;
}

void 
xauth_reply_stub(sc)
	struct sched *sc;
{
	struct xauth_reply_arg *xra = container_of(sc, struct xauth_reply_arg, sc);
	struct ph1handle *iph1;

	if ((iph1 = getph1byindex(&xra->index)) != NULL)
		(void)xauth_reply(iph1, xra->port, xra->id, xra->res);
	else
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Delayed Xauth reply: phase 1 no longer exists.\n"); 

	racoon_free(xra);
}

int
xauth_reply(iph1, port, id, res)
	struct ph1handle *iph1;
	int port;
	int id;
{
	struct xauth_state *xst = &iph1->mode_cfg->xauth;
	char *usr = xst->authdata.generic.usr;

	if (res != 0) {
		if (port != -1)
			isakmp_cfg_putport(iph1, port);

		plog(LLV_INFO, LOCATION, NULL, 
		    "login failed for user \"%s\"\n", usr);
		
		xauth_sendstatus(iph1, XAUTH_STATUS_FAIL, id);
		xst->status = XAUTHST_NOTYET;

		/* Delete Phase 1 SA */
		if (iph1->status >= PHASE1ST_ESTABLISHED)
			isakmp_info_send_d1(iph1);
		remph1(iph1);
		delph1(iph1);

		return -1;
	}

	xst->status = XAUTHST_OK;
	plog(LLV_INFO, LOCATION, NULL, 
	    "login succeeded for user \"%s\"\n", usr);

	xauth_sendstatus(iph1, XAUTH_STATUS_OK, id);

	return 0;
}

void
xauth_sendstatus(iph1, status, id)
	struct ph1handle *iph1;
	int status;
	int id;
{
	vchar_t *buffer;
	struct isakmp_pl_attr *attr;
	struct isakmp_data *stattr;
	size_t tlen;

	tlen = sizeof(*attr) +
	       + sizeof(*stattr); 
	
	if ((buffer = vmalloc(tlen)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, "Cannot allocate buffer\n");
		return;
	}
	
	attr = (struct isakmp_pl_attr *)buffer->v;
	memset(attr, 0, tlen);

	attr->h.len = htons(tlen);
	attr->type = ISAKMP_CFG_SET;
	attr->id = htons(id);

	stattr = (struct isakmp_data *)(attr + 1);
	stattr->type = htons(XAUTH_STATUS | ISAKMP_GEN_TV);
	stattr->lorv = htons(status);

	isakmp_cfg_send(iph1, buffer, 
	    ISAKMP_NPTYPE_ATTR, ISAKMP_FLAG_E, 1);
	
	vfree(buffer);

	return;	
}

#ifdef HAVE_LIBRADIUS
int
xauth_radius_init_conf(int free)
{
	/* free radius config resources */
	if (free) {
		int i;
		for (i = 0; i < xauth_rad_config.auth_server_count; i++) {
			vfree(xauth_rad_config.auth_server_list[i].host);
			vfree(xauth_rad_config.auth_server_list[i].secret);
		}
		for (i = 0; i < xauth_rad_config.acct_server_count; i++) {
			vfree(xauth_rad_config.acct_server_list[i].host);
			vfree(xauth_rad_config.acct_server_list[i].secret);
		}
		if (radius_auth_state != NULL)
			rad_close(radius_auth_state);
		if (radius_acct_state != NULL)
			rad_close(radius_acct_state);
	}

	/* initialize radius config */
	memset(&xauth_rad_config, 0, sizeof(xauth_rad_config));
	return 0;
}

int
xauth_radius_init(void)
{
	/* For first time use, initialize Radius */
	if ((isakmp_cfg_config.authsource == ISAKMP_CFG_AUTH_RADIUS) &&
	    (radius_auth_state == NULL)) {
		if ((radius_auth_state = rad_auth_open()) == NULL) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Cannot init libradius\n");
			return -1;
		}

		int auth_count = xauth_rad_config.auth_server_count;
		int auth_added = 0;
		if (auth_count) {
			int i;
			for (i = 0; i < auth_count; i++) {
				if(!rad_add_server(
					radius_auth_state,
					xauth_rad_config.auth_server_list[i].host->v,
					xauth_rad_config.auth_server_list[i].port,
					xauth_rad_config.auth_server_list[i].secret->v,
					xauth_rad_config.timeout,
					xauth_rad_config.retries ))
					auth_added++;
				else
					plog(LLV_WARNING, LOCATION, NULL,
						"could not add radius auth server %s\n",
						xauth_rad_config.auth_server_list[i].host->v);
			}
		}

		if (!auth_added) {
			if (rad_config(radius_auth_state, NULL) != 0) {
				plog(LLV_ERROR, LOCATION, NULL, 
				    "Cannot open librarius config file: %s\n", 
				    rad_strerror(radius_auth_state));
				rad_close(radius_auth_state);
				radius_auth_state = NULL;
				return -1;
			}
		}
	}

	if ((isakmp_cfg_config.accounting == ISAKMP_CFG_ACCT_RADIUS) &&
	    (radius_acct_state == NULL)) {
		if ((radius_acct_state = rad_acct_open()) == NULL) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Cannot init libradius\n");
			return -1;
		}

		int acct_count = xauth_rad_config.acct_server_count;
		int acct_added = 0;
		if (acct_count) {
			int i;
			for (i = 0; i < acct_count; i++) {
				if(!rad_add_server(
					radius_acct_state,
					xauth_rad_config.acct_server_list[i].host->v,
					xauth_rad_config.acct_server_list[i].port,
					xauth_rad_config.acct_server_list[i].secret->v,
					xauth_rad_config.timeout,
					xauth_rad_config.retries ))
					acct_added++;
				else
					plog(LLV_WARNING, LOCATION, NULL,
						"could not add radius account server %s\n",
						xauth_rad_config.acct_server_list[i].host->v);
			}
		}

		if (!acct_added) {
			if (rad_config(radius_acct_state, NULL) != 0) {
				plog(LLV_ERROR, LOCATION, NULL, 
				    "Cannot open librarius config file: %s\n", 
				    rad_strerror(radius_acct_state));
				rad_close(radius_acct_state);
				radius_acct_state = NULL;
				return -1;
			}
		}
	}

	return 0;
}

int
xauth_login_radius(iph1, usr, pwd)
	struct ph1handle *iph1;
	char *usr;
	char *pwd;
{
	int res;
	const void *data;
	size_t len;
	int type;

	if (rad_create_request(radius_auth_state, RAD_ACCESS_REQUEST) != 0) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "rad_create_request failed: %s\n", 
		    rad_strerror(radius_auth_state));
		return -1;
	}
	
	if (rad_put_string(radius_auth_state, RAD_USER_NAME, usr) != 0) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "rad_put_string failed: %s\n", 
		    rad_strerror(radius_auth_state));
		return -1;
	}

	if (rad_put_string(radius_auth_state, RAD_USER_PASSWORD, pwd) != 0) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "rad_put_string failed: %s\n", 
		    rad_strerror(radius_auth_state));
		return -1;
	}

	if (isakmp_cfg_radius_common(radius_auth_state, iph1->mode_cfg->port) != 0)
		return -1;

	switch (res = rad_send_request(radius_auth_state)) {
	case RAD_ACCESS_ACCEPT:
		while ((type = rad_get_attr(radius_auth_state, &data, &len)) != 0) {
			switch (type) {
			case RAD_FRAMED_IP_ADDRESS:
				iph1->mode_cfg->addr4 = rad_cvt_addr(data);
				iph1->mode_cfg->flags 
				    |= ISAKMP_CFG_ADDR4_EXTERN;
				break;

			case RAD_FRAMED_IP_NETMASK:
				iph1->mode_cfg->mask4 = rad_cvt_addr(data);
				iph1->mode_cfg->flags 
				    |= ISAKMP_CFG_MASK4_EXTERN;
				break;

			default:
				plog(LLV_INFO, LOCATION, NULL,
				    "Unexpected attribute: %d\n", type);
				break;
			}
		}

		return 0;
		break;

	case RAD_ACCESS_REJECT:
		return -1;
		break;

	case -1:
		plog(LLV_ERROR, LOCATION, NULL, 
		    "rad_send_request failed: %s\n", 
		    rad_strerror(radius_auth_state));
		return -1;
		break;
	default:
		plog(LLV_ERROR, LOCATION, NULL, 
		    "rad_send_request returned %d\n", res);
		return -1;
		break;
	}

	return -1;
}
#endif

#ifdef HAVE_LIBPAM
static int 
PAM_conv(msg_count, msg, rsp, dontcare)
	int msg_count;
	const struct pam_message **msg;
	struct pam_response **rsp;
	void *dontcare;
{
	int i;
	int replies = 0;
	struct pam_response *reply = NULL;

	if ((reply = racoon_malloc(sizeof(*reply) * msg_count)) == NULL) 
		return PAM_CONV_ERR;
	bzero(reply, sizeof(*reply) * msg_count);

	for (i = 0; i < msg_count; i++) {
		switch (msg[i]->msg_style) {
		case PAM_PROMPT_ECHO_ON:
			/* Send the username, libpam frees resp */
			reply[i].resp_retcode = PAM_SUCCESS;
			if ((reply[i].resp = strdup(PAM_usr)) == NULL) {
				plog(LLV_ERROR, LOCATION, 
				    NULL, "strdup failed\n");
				exit(1);
			}
			break;

		case PAM_PROMPT_ECHO_OFF:
			/* Send the password, libpam frees resp */
			reply[i].resp_retcode = PAM_SUCCESS;
			if ((reply[i].resp = strdup(PAM_pwd)) == NULL) {
				plog(LLV_ERROR, LOCATION, 
				    NULL, "strdup failed\n");
				exit(1);
			}
			break;

		case PAM_TEXT_INFO:
		case PAM_ERROR_MSG:
			reply[i].resp_retcode = PAM_SUCCESS;
			reply[i].resp = NULL;
			break;

		default:
			if (reply != NULL)
				racoon_free(reply);
			return PAM_CONV_ERR;
			break;
		}
	}

	if (reply != NULL)
		*rsp = reply;

	return PAM_SUCCESS;
}

int
xauth_login_pam(port, raddr, usr, pwd)
	int port;
	struct sockaddr *raddr;
	char *usr;
	char *pwd;
{
	int error;
	int res;
	const void *data;
	size_t len;
	int type;
	char *remote = NULL;
	pam_handle_t *pam = NULL;

	if (isakmp_cfg_config.port_pool == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "isakmp_cfg_config.port_pool == NULL\n");
		return -1;
	}

	if ((error = pam_start("racoon", usr, 
	    &PAM_chat, &isakmp_cfg_config.port_pool[port].pam)) != 0) {
		if (isakmp_cfg_config.port_pool[port].pam == NULL) {
			plog(LLV_ERROR, LOCATION, NULL, "pam_start failed\n");
			return -1;
		} else {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "pam_start failed: %s\n", 
			    pam_strerror(isakmp_cfg_config.port_pool[port].pam,
			    error));
			goto out;
		}
	}
	pam = isakmp_cfg_config.port_pool[port].pam;

	if ((remote = strdup(saddrwop2str(raddr))) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "cannot allocate memory: %s\n", strerror(errno)); 
		goto out;
	}

	if ((error = pam_set_item(pam, PAM_RHOST, remote)) != 0) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "pam_set_item failed: %s\n", 
		    pam_strerror(pam, error));
		goto out;
	}

	if ((error = pam_set_item(pam, PAM_RUSER, usr)) != 0) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "pam_set_item failed: %s\n", 
		    pam_strerror(pam, error));
		goto out;
	}

	PAM_usr = usr;
	PAM_pwd = pwd;
	error = pam_authenticate(pam, 0);
	PAM_usr = NULL;
	PAM_pwd = NULL;
	if (error != 0) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "pam_authenticate failed: %s\n", 
		    pam_strerror(pam, error));
		goto out;
	}

	if ((error = pam_acct_mgmt(pam, 0)) != 0) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "pam_acct_mgmt failed: %s\n", 
		    pam_strerror(pam, error));
		goto out;
	}

	if ((error = pam_setcred(pam, 0)) != 0) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "pam_setcred failed: %s\n", 
		    pam_strerror(pam, error));
		goto out;
	}

	if (remote != NULL)
		free(remote);

	return 0;

out:
	pam_end(pam, error);
	isakmp_cfg_config.port_pool[port].pam = NULL;
	if (remote != NULL)
		free(remote);
	return -1;
}
#endif

#ifdef HAVE_LIBLDAP
int 
xauth_ldap_init_conf(void)
{
	int tmplen;
	int error = -1;

	xauth_ldap_config.pver = 3;
	xauth_ldap_config.host = NULL;
	xauth_ldap_config.port = LDAP_PORT;
	xauth_ldap_config.base = NULL;
	xauth_ldap_config.subtree = 0;
	xauth_ldap_config.bind_dn = NULL;
	xauth_ldap_config.bind_pw = NULL;
	xauth_ldap_config.auth_type = LDAP_AUTH_SIMPLE;
	xauth_ldap_config.attr_user = NULL;
	xauth_ldap_config.attr_addr = NULL;
	xauth_ldap_config.attr_mask = NULL;
	xauth_ldap_config.attr_group = NULL;
	xauth_ldap_config.attr_member = NULL;

	/* set default host */
	tmplen = strlen(LDAP_DFLT_HOST);
	xauth_ldap_config.host = vmalloc(tmplen);
	if (xauth_ldap_config.host == NULL)
		goto out;
	memcpy(xauth_ldap_config.host->v, LDAP_DFLT_HOST, tmplen);

	/* set default user naming attribute */
	tmplen = strlen(LDAP_DFLT_USER);
	xauth_ldap_config.attr_user = vmalloc(tmplen);
	if (xauth_ldap_config.attr_user == NULL)
		goto out;	
	memcpy(xauth_ldap_config.attr_user->v, LDAP_DFLT_USER, tmplen);

	/* set default address attribute */
	tmplen = strlen(LDAP_DFLT_ADDR);
	xauth_ldap_config.attr_addr = vmalloc(tmplen);
	if (xauth_ldap_config.attr_addr == NULL)
		goto out;
	memcpy(xauth_ldap_config.attr_addr->v, LDAP_DFLT_ADDR, tmplen);

	/* set default netmask attribute */
	tmplen = strlen(LDAP_DFLT_MASK);
	xauth_ldap_config.attr_mask = vmalloc(tmplen);
	if (xauth_ldap_config.attr_mask == NULL)
		goto out;
	memcpy(xauth_ldap_config.attr_mask->v, LDAP_DFLT_MASK, tmplen);

	/* set default group naming attribute */
	tmplen = strlen(LDAP_DFLT_GROUP);
	xauth_ldap_config.attr_group = vmalloc(tmplen);
	if (xauth_ldap_config.attr_group == NULL)
		goto out;
	memcpy(xauth_ldap_config.attr_group->v, LDAP_DFLT_GROUP, tmplen);

	/* set default member attribute */
	tmplen = strlen(LDAP_DFLT_MEMBER);
	xauth_ldap_config.attr_member = vmalloc(tmplen);
	if (xauth_ldap_config.attr_member == NULL)
		goto out;
	memcpy(xauth_ldap_config.attr_member->v, LDAP_DFLT_MEMBER, tmplen);

	error = 0;
out:
	if (error != 0)
		plog(LLV_ERROR, LOCATION, NULL, "cannot allocate memory\n");

	return error;
}

int
xauth_login_ldap(iph1, usr, pwd)
	struct ph1handle *iph1;
	char *usr;
	char *pwd;
{
	int rtn = -1;
	int res = -1;
	LDAP *ld = NULL;
	LDAPMessage *lr = NULL;
	LDAPMessage *le = NULL;
	struct berval cred;
	struct berval **bv = NULL;
	struct timeval timeout;
	char *init = NULL;
	char *filter = NULL;
	char *atlist[3];
	char *basedn = NULL;
	char *userdn = NULL;
	int tmplen = 0;
	int ecount = 0;
	int scope = LDAP_SCOPE_ONE;

	atlist[0] = NULL;
	atlist[1] = NULL;
	atlist[2] = NULL;

	/* build our initialization url */
	tmplen = strlen("ldap://:") + 17;
	tmplen += strlen(xauth_ldap_config.host->v);
	init = racoon_malloc(tmplen);
	if (init == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"unable to alloc ldap init url\n");
		goto ldap_end;
	}
	sprintf(init,"ldap://%s:%d",
		xauth_ldap_config.host->v,
		xauth_ldap_config.port );

	/* initialize the ldap handle */
	res = ldap_initialize(&ld, init);
	if (res != LDAP_SUCCESS) {
		plog(LLV_ERROR, LOCATION, NULL,
			"ldap_initialize failed: %s\n",
			ldap_err2string(res));
		goto ldap_end;
	}

	/* initialize the protocol version */
	ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION,
		&xauth_ldap_config.pver);

	/*
	 * attempt to bind to the ldap server.
         * default to anonymous bind unless a
	 * user dn and password has been
	 * specified in our configuration
         */
	if ((xauth_ldap_config.bind_dn != NULL)&&
	    (xauth_ldap_config.bind_pw != NULL))
	{
		cred.bv_val = xauth_ldap_config.bind_pw->v;
		cred.bv_len = strlen( cred.bv_val );
		res = ldap_sasl_bind_s(ld,
			xauth_ldap_config.bind_dn->v, NULL, &cred,
			NULL, NULL, NULL);
	}
	else
	{
		res = ldap_sasl_bind_s(ld,
			NULL, NULL, NULL,
			NULL, NULL, NULL);
	}
	
	if (res!=LDAP_SUCCESS) {
		plog(LLV_ERROR, LOCATION, NULL,
			"ldap_sasl_bind_s (search) failed: %s\n",
			ldap_err2string(res));
		goto ldap_end;
	}

	/* build an ldap user search filter */
	tmplen = strlen(xauth_ldap_config.attr_user->v);
	tmplen += 1;
	tmplen += strlen(usr);
	tmplen += 1;
	filter = racoon_malloc(tmplen);
	if (filter == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"unable to alloc ldap search filter buffer\n");
		goto ldap_end;
	}
	sprintf(filter, "%s=%s",
		xauth_ldap_config.attr_user->v, usr);

	/* build our return attribute list */
	tmplen = strlen(xauth_ldap_config.attr_addr->v) + 1;
	atlist[0] = racoon_malloc(tmplen);
	tmplen = strlen(xauth_ldap_config.attr_mask->v) + 1;
	atlist[1] = racoon_malloc(tmplen);
	if ((atlist[0] == NULL)||(atlist[1] == NULL)) {
		plog(LLV_ERROR, LOCATION, NULL,
			"unable to alloc ldap attrib list buffer\n");
		goto ldap_end;
	}
	strcpy(atlist[0],xauth_ldap_config.attr_addr->v);
	strcpy(atlist[1],xauth_ldap_config.attr_mask->v);

	/* attempt to locate the user dn */
	if (xauth_ldap_config.base != NULL)
		basedn = xauth_ldap_config.base->v;
	if (xauth_ldap_config.subtree)
		scope = LDAP_SCOPE_SUBTREE;
	timeout.tv_sec = 15;
	timeout.tv_usec = 0;
	res = ldap_search_ext_s(ld, basedn, scope,
		filter, atlist, 0, NULL, NULL,
		&timeout, 2, &lr);
	if (res != LDAP_SUCCESS) {
		plog(LLV_ERROR, LOCATION, NULL,
			"ldap_search_ext_s failed: %s\n",
			ldap_err2string(res));
		goto ldap_end;
	}

	/* check the number of ldap entries returned */
	ecount = ldap_count_entries(ld, lr);
	if (ecount < 1) {
		plog(LLV_WARNING, LOCATION, NULL, 
			"no ldap results for filter \'%s\'\n", 
			 filter);
		goto ldap_end;
	}
	if (ecount > 1) {
		plog(LLV_WARNING, LOCATION, NULL, 
			"multiple (%i) ldap results for filter \'%s\'\n", 
			ecount, filter);
	}

	/* obtain the dn from the first result */
	le = ldap_first_entry(ld, lr);
	if (le == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"ldap_first_entry failed: invalid entry returned\n");
		goto ldap_end;
	}
	userdn = ldap_get_dn(ld, le);
	if (userdn == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"ldap_get_dn failed: invalid string returned\n");
		goto ldap_end;
	}

	/* cache the user dn in the xauth state */
	iph1->mode_cfg->xauth.udn = racoon_malloc(strlen(userdn)+1);
	strcpy(iph1->mode_cfg->xauth.udn,userdn);

	/* retrieve modecfg address */
	bv = ldap_get_values_len(ld, le, xauth_ldap_config.attr_addr->v);
	if (bv != NULL)	{
		char tmpaddr[16];
		/* sanity check for address value */
		if ((bv[0]->bv_len < 7)||(bv[0]->bv_len > 15)) {
			plog(LLV_DEBUG, LOCATION, NULL,
				"ldap returned invalid modecfg address\n");
			ldap_value_free_len(bv);
			goto ldap_end;
		}
		memcpy(tmpaddr,bv[0]->bv_val,bv[0]->bv_len);
		tmpaddr[bv[0]->bv_len]=0;
		iph1->mode_cfg->addr4.s_addr = inet_addr(tmpaddr);
		iph1->mode_cfg->flags |= ISAKMP_CFG_ADDR4_EXTERN;
		plog(LLV_INFO, LOCATION, NULL,
			"ldap returned modecfg address %s\n", tmpaddr);
		ldap_value_free_len(bv);
	}

	/* retrieve modecfg netmask */
	bv = ldap_get_values_len(ld, le, xauth_ldap_config.attr_mask->v);
	if (bv != NULL)	{
		char tmpmask[16];
		/* sanity check for netmask value */
		if ((bv[0]->bv_len < 7)||(bv[0]->bv_len > 15)) {
			plog(LLV_DEBUG, LOCATION, NULL,
				"ldap returned invalid modecfg netmask\n");
			ldap_value_free_len(bv);
			goto ldap_end;
		}
		memcpy(tmpmask,bv[0]->bv_val,bv[0]->bv_len);
		tmpmask[bv[0]->bv_len]=0;
		iph1->mode_cfg->mask4.s_addr = inet_addr(tmpmask);
		iph1->mode_cfg->flags |= ISAKMP_CFG_MASK4_EXTERN;
		plog(LLV_INFO, LOCATION, NULL,
			"ldap returned modecfg netmask %s\n", tmpmask);
		ldap_value_free_len(bv);
	}

	/*
	 * finally, use the dn and the xauth
	 * password to check the users given
	 * credentials by attempting to bind
	 * to the ldap server
	 */
	plog(LLV_INFO, LOCATION, NULL,
		"attempting ldap bind for dn \'%s\'\n", userdn);
	cred.bv_val = pwd;
	cred.bv_len = strlen( cred.bv_val );
	res = ldap_sasl_bind_s(ld,
		userdn, NULL, &cred,
		NULL, NULL, NULL);
        if(res==LDAP_SUCCESS)
		rtn = 0;

ldap_end:

	/* free ldap resources */
	if (userdn != NULL)
		ldap_memfree(userdn);
	if (atlist[0] != NULL)
		racoon_free(atlist[0]);
	if (atlist[1] != NULL)
		racoon_free(atlist[1]);
	if (filter != NULL)
		racoon_free(filter);
	if (lr != NULL)
		ldap_msgfree(lr);
	if (init != NULL)
		racoon_free(init);

	ldap_unbind_ext_s(ld, NULL, NULL);

	return rtn;
}

int
xauth_group_ldap(udn, grp)
	char * udn;
	char * grp;
{
	int rtn = -1;
	int res = -1;
	LDAP *ld = NULL;
	LDAPMessage *lr = NULL;
	LDAPMessage *le = NULL;
	struct berval cred;
	struct timeval timeout;
	char *init = NULL;
	char *filter = NULL;
	char *basedn = NULL;
	char *groupdn = NULL;
	int tmplen = 0;
	int ecount = 0;
	int scope = LDAP_SCOPE_ONE;

	/* build our initialization url */
	tmplen = strlen("ldap://:") + 17;
	tmplen += strlen(xauth_ldap_config.host->v);
	init = racoon_malloc(tmplen);
	if (init == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"unable to alloc ldap init url\n");
		goto ldap_group_end;
	}
	sprintf(init,"ldap://%s:%d",
		xauth_ldap_config.host->v,
		xauth_ldap_config.port );

	/* initialize the ldap handle */
	res = ldap_initialize(&ld, init);
	if (res != LDAP_SUCCESS) {
		plog(LLV_ERROR, LOCATION, NULL,
			"ldap_initialize failed: %s\n",
			ldap_err2string(res));
		goto ldap_group_end;
	}

	/* initialize the protocol version */
	ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION,
		&xauth_ldap_config.pver);

	/*
	 * attempt to bind to the ldap server.
         * default to anonymous bind unless a
	 * user dn and password has been
	 * specified in our configuration
         */
	if ((xauth_ldap_config.bind_dn != NULL)&&
	    (xauth_ldap_config.bind_pw != NULL))
	{
		cred.bv_val = xauth_ldap_config.bind_pw->v;
		cred.bv_len = strlen( cred.bv_val );
		res = ldap_sasl_bind_s(ld,
			xauth_ldap_config.bind_dn->v, NULL, &cred,
			NULL, NULL, NULL);
	}
	else
	{
		res = ldap_sasl_bind_s(ld,
			NULL, NULL, NULL,
			NULL, NULL, NULL);
	}

	if (res!=LDAP_SUCCESS) {
		plog(LLV_ERROR, LOCATION, NULL,
			"ldap_sasl_bind_s (search) failed: %s\n",
			ldap_err2string(res));
		goto ldap_group_end;
	}

	/* build an ldap group search filter */
	tmplen = strlen("(&(=)(=))") + 1;
	tmplen += strlen(xauth_ldap_config.attr_group->v);
	tmplen += strlen(grp);
	tmplen += strlen(xauth_ldap_config.attr_member->v);
	tmplen += strlen(udn);
	filter = racoon_malloc(tmplen);
	if (filter == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"unable to alloc ldap search filter buffer\n");
		goto ldap_group_end;
	}
	sprintf(filter, "(&(%s=%s)(%s=%s))",
		xauth_ldap_config.attr_group->v, grp,
		xauth_ldap_config.attr_member->v, udn);

	/* attempt to locate the group dn */
	if (xauth_ldap_config.base != NULL)
		basedn = xauth_ldap_config.base->v;
	if (xauth_ldap_config.subtree)
		scope = LDAP_SCOPE_SUBTREE;
	timeout.tv_sec = 15;
	timeout.tv_usec = 0;
	res = ldap_search_ext_s(ld, basedn, scope,
		filter, NULL, 0, NULL, NULL,
		&timeout, 2, &lr);
	if (res != LDAP_SUCCESS) {
		plog(LLV_ERROR, LOCATION, NULL,
			"ldap_search_ext_s failed: %s\n",
			ldap_err2string(res));
		goto ldap_group_end;
	}

	/* check the number of ldap entries returned */
	ecount = ldap_count_entries(ld, lr);
	if (ecount < 1) {
		plog(LLV_WARNING, LOCATION, NULL, 
			"no ldap results for filter \'%s\'\n", 
			 filter);
		goto ldap_group_end;
	}

	/* success */
	rtn = 0;

	/* obtain the dn from the first result */
	le = ldap_first_entry(ld, lr);
	if (le == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"ldap_first_entry failed: invalid entry returned\n");
		goto ldap_group_end;
	}
	groupdn = ldap_get_dn(ld, le);
	if (groupdn == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"ldap_get_dn failed: invalid string returned\n");
		goto ldap_group_end;
	}

	plog(LLV_INFO, LOCATION, NULL,
		"ldap membership group returned \'%s\'\n", groupdn);
ldap_group_end:

	/* free ldap resources */
	if (groupdn != NULL)
		ldap_memfree(groupdn);
	if (filter != NULL)
		racoon_free(filter);
	if (lr != NULL)
		ldap_msgfree(lr);
	if (init != NULL)
		racoon_free(init);

	ldap_unbind_ext_s(ld, NULL, NULL);

	return rtn;
}

#endif

int
xauth_login_system(usr, pwd)
	char *usr;
	char *pwd;
{
	struct passwd *pw;
	char *cryptpwd;
	char *syscryptpwd;
#ifdef HAVE_SHADOW_H
	struct spwd *spw;

	if ((spw = getspnam(usr)) == NULL)
		return -1;

	syscryptpwd = spw->sp_pwdp;
#endif

	if ((pw = getpwnam(usr)) == NULL)
		return -1;

#ifndef HAVE_SHADOW_H
	syscryptpwd = pw->pw_passwd;
#endif

	/* No root login. Ever. */
	if (pw->pw_uid == 0)
		return -1;

	if ((cryptpwd = crypt(pwd, syscryptpwd)) == NULL)
		return -1;

	if (strcmp(cryptpwd, syscryptpwd) == 0)
		return 0;

	return -1;
}

int
xauth_group_system(usr, grp)
	char * usr;
	char * grp;
{
	struct group * gr;
	char * member;
	int index = 0;

	gr = getgrnam(grp);
	if (gr == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"the system group name \'%s\' is unknown\n",
			grp);
		return -1;
	}

	while ((member = gr->gr_mem[index++])!=NULL) {
		if (!strcmp(member,usr)) {
			plog(LLV_INFO, LOCATION, NULL,
		                "membership validated\n");
			return 0;
		}
	}

	return -1;
}

int 
xauth_check(iph1)
	struct ph1handle *iph1;
{
	struct xauth_state *xst = &iph1->mode_cfg->xauth;

	/* 
 	 * Only the server side (edge device) really check for Xauth 
	 * status. It does it if the chose authmethod is using Xauth.
	 * On the client side (roadwarrior), we don't check anything.
	 */
	switch (iph1->approval->authmethod) {
	case OAKLEY_ATTR_AUTH_METHOD_HYBRID_RSA_R:
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSASIG_R:
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_PSKEY_R:
	/* The following are not yet implemented */
	case OAKLEY_ATTR_AUTH_METHOD_HYBRID_DSS_R:
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_DSSSIG_R:
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAENC_R:
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAREV_R:
		if ((iph1->mode_cfg->flags & ISAKMP_CFG_VENDORID_XAUTH) == 0) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "Hybrid auth negotiated but peer did not "
			    "announced as Xauth capable\n");
			return -1;
		}

		if (xst->status != XAUTHST_OK) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "Hybrid auth negotiated but peer did not "
			    "succeed Xauth exchange\n");
			return -1;
		}

		return 0;
		break;
	default:
		return 0;
		break;
	}

	return 0;
}

int
group_check(iph1, grp_list, grp_count)
	struct ph1handle *iph1;
	char **grp_list;
	int grp_count;
{
	int res = -1;
	int grp_index = 0;
	char * usr = NULL;

	/* check for presence of modecfg data */

	if(iph1->mode_cfg == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"xauth group specified but modecfg not found\n");
		return res;
	}

	/* loop through our group list */

	for(; grp_index < grp_count; grp_index++) {

		/* check for presence of xauth data */

		usr = iph1->mode_cfg->xauth.authdata.generic.usr;

		if(usr == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"xauth group specified but xauth not found\n");
			return res;
		}

		/* call appropriate group validation funtion */

		switch (isakmp_cfg_config.groupsource) {

			case ISAKMP_CFG_GROUP_SYSTEM:
				res = xauth_group_system(
					usr,
					grp_list[grp_index]);
				break;

#ifdef HAVE_LIBLDAP
			case ISAKMP_CFG_GROUP_LDAP:
				res = xauth_group_ldap(
					iph1->mode_cfg->xauth.udn,
					grp_list[grp_index]);
				break;
#endif

			default:
				/* we should never get here */
				plog(LLV_ERROR, LOCATION, NULL,
				    "Unknown group auth source\n");
				break;
		}

		if( !res ) {
			plog(LLV_INFO, LOCATION, NULL,
				"user \"%s\" is a member of group \"%s\"\n",
				usr,
				grp_list[grp_index]);
			break;
		} else {
			plog(LLV_INFO, LOCATION, NULL,
				"user \"%s\" is not a member of group \"%s\"\n",
				usr,
				grp_list[grp_index]);
		}
	}

	return res;
}

vchar_t *
isakmp_xauth_req(iph1, attr)
	struct ph1handle *iph1;
	struct isakmp_data *attr;
{
	int type;
	size_t dlen = 0;
	int ashort = 0;
	int value = 0;
	vchar_t *buffer = NULL;
	char *mraw = NULL, *mdata;
	char *data;
	vchar_t *usr = NULL;
	vchar_t *pwd = NULL;
	size_t skip = 0;
	int freepwd = 0;

	if ((iph1->mode_cfg->flags & ISAKMP_CFG_VENDORID_XAUTH) == 0) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Xauth mode config request but peer "
		    "did not declare itself as Xauth capable\n");
		return NULL;
	}

	type = ntohs(attr->type) & ~ISAKMP_GEN_MASK;

	/* Sanity checks */
	switch(type) {
	case XAUTH_TYPE:
		if ((ntohs(attr->type) & ISAKMP_GEN_TV) == 0) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Unexpected long XAUTH_TYPE attribute\n");
			return NULL;
		}
		if (ntohs(attr->lorv) != XAUTH_TYPE_GENERIC) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Unsupported Xauth authentication %d\n", 
			    ntohs(attr->lorv));
			return NULL;
		}
		ashort = 1;
		dlen = 0;
		value = XAUTH_TYPE_GENERIC;
		break;

	case XAUTH_USER_NAME:
		if (!iph1->rmconf->xauth || !iph1->rmconf->xauth->login) {
			plog(LLV_ERROR, LOCATION, NULL, "Xauth performed "
			    "with no login supplied\n");
			return NULL;
		}

		dlen = iph1->rmconf->xauth->login->l - 1;
		iph1->rmconf->xauth->state |= XAUTH_SENT_USERNAME;
		break;

	case XAUTH_USER_PASSWORD:
		if (!iph1->rmconf->xauth || !iph1->rmconf->xauth->login)
			return NULL;

		skip = sizeof(struct ipsecdoi_id_b);
		usr = vmalloc(iph1->rmconf->xauth->login->l - 1 + skip);
		if (usr == NULL) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Cannot allocate memory\n");
			return NULL;
		}
		memset(usr->v, 0, skip);
		memcpy(usr->v + skip, 
		    iph1->rmconf->xauth->login->v, 
		    iph1->rmconf->xauth->login->l - 1);

		if (iph1->rmconf->xauth->pass) {
			/* A key given through racoonctl */
			pwd = iph1->rmconf->xauth->pass;
		} else {
			if ((pwd = getpskbyname(usr)) == NULL) {
				plog(LLV_ERROR, LOCATION, NULL, 
				    "No password was found for login %s\n", 
				    iph1->rmconf->xauth->login->v);
				vfree(usr);
				return NULL;
			}
			/* We have to free it before returning */
			freepwd = 1;
		}
		vfree(usr);

		iph1->rmconf->xauth->state |= XAUTH_SENT_PASSWORD;
		dlen = pwd->l;

		break;
	case XAUTH_MESSAGE:
		if ((ntohs(attr->type) & ISAKMP_GEN_TV) == 0) {
			dlen = ntohs(attr->lorv);
			if (dlen > 0) {
				mraw = (char*)(attr + 1);
				mdata = binsanitize(mraw, dlen);
				if (mdata == NULL) {
					plog(LLV_ERROR, LOCATION, iph1->remote,
					    "Cannot allocate memory\n");
					return NULL;
				}
				plog(LLV_NOTIFY,LOCATION, iph1->remote,
					"XAUTH Message: '%s'.\n",
					mdata);
				racoon_free(mdata);
			}
		}
		return NULL;
	default:
		plog(LLV_WARNING, LOCATION, NULL,
		    "Ignored attribute %s\n", s_isakmp_cfg_type(type));
		return NULL;
		break;
	}

	if ((buffer = vmalloc(sizeof(*attr) + dlen)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "Cannot allocate memory\n");
		goto out;
	}

	attr = (struct isakmp_data *)buffer->v;
	if (ashort) {
		attr->type = htons(type | ISAKMP_GEN_TV);
		attr->lorv = htons(value);
		goto out;
	}

	attr->type = htons(type | ISAKMP_GEN_TLV);
	attr->lorv = htons(dlen);
	data = (char *)(attr + 1);

	switch(type) {
	case XAUTH_USER_NAME:
		/* 
		 * iph1->rmconf->xauth->login->v is valid, 
		 * we just checked it in the previous switch case 
		 */
		memcpy(data, iph1->rmconf->xauth->login->v, dlen);
		break;
	case XAUTH_USER_PASSWORD:
		memcpy(data, pwd->v, dlen);
		break;
	default:
		break;
	}

out:
	if (freepwd)
		vfree(pwd);

	return buffer;
}

vchar_t *
isakmp_xauth_set(iph1, attr)
	struct ph1handle *iph1;
	struct isakmp_data *attr;
{
	int type;
	vchar_t *buffer = NULL;
	char *data;
	struct xauth_state *xst;
	size_t dlen = 0;
	char* mraw = NULL, *mdata;

	if ((iph1->mode_cfg->flags & ISAKMP_CFG_VENDORID_XAUTH) == 0) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Xauth mode config set but peer "
		    "did not declare itself as Xauth capable\n");
		return NULL;
	}

	type = ntohs(attr->type) & ~ISAKMP_GEN_MASK;

	switch(type) {
	case XAUTH_STATUS:
		/* 
		 * We should only receive ISAKMP mode_cfg SET XAUTH_STATUS
		 * when running as a client (initiator).
		 */
		xst = &iph1->mode_cfg->xauth;
		switch (iph1->approval->authmethod) {
		case OAKLEY_ATTR_AUTH_METHOD_HYBRID_RSA_I:
		case OAKLEY_ATTR_AUTH_METHOD_XAUTH_PSKEY_I:
		case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSASIG_I:
		/* Not implemented ... */
		case OAKLEY_ATTR_AUTH_METHOD_HYBRID_DSS_I:
		case OAKLEY_ATTR_AUTH_METHOD_XAUTH_DSSSIG_I:
		case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAENC_I:
		case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAREV_I:
			break;
		default:
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Unexpected XAUTH_STATUS_OK\n");
			return NULL;
			break;
		}

		/* If we got a failure, delete iph1 */
		if (ntohs(attr->lorv) != XAUTH_STATUS_OK) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Xauth authentication failed\n");

			evt_phase1(iph1, EVT_PHASE1_XAUTH_FAILED, NULL);

			iph1->mode_cfg->flags |= ISAKMP_CFG_DELETE_PH1;
		} else {
			evt_phase1(iph1, EVT_PHASE1_XAUTH_SUCCESS, NULL);
		}


		/* We acknowledge it */
		break;
	case XAUTH_MESSAGE:
		if ((ntohs(attr->type) & ISAKMP_GEN_TV) == 0) {
			dlen = ntohs(attr->lorv);
			if (dlen > 0) {
				mraw = (char*)(attr + 1);
				mdata = binsanitize(mraw, dlen);
				if (mdata == NULL) {
					plog(LLV_ERROR, LOCATION, iph1->remote,
					    "Cannot allocate memory\n");
					return NULL;
				}
				plog(LLV_NOTIFY,LOCATION, iph1->remote,
					"XAUTH Message: '%s'.\n",
					mdata);
				racoon_free(mdata);
			}
		}

	default:
		plog(LLV_WARNING, LOCATION, NULL,
		    "Ignored attribute %s\n", s_isakmp_cfg_type(type));
		return NULL;
		break;
	}

	if ((buffer = vmalloc(sizeof(*attr))) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "Cannot allocate memory\n");
		return NULL;
	}

	attr = (struct isakmp_data *)buffer->v;
	attr->type = htons(type | ISAKMP_GEN_TV);
	attr->lorv = htons(0);

	return buffer;
}


void 
xauth_rmstate(xst)
	struct xauth_state *xst;
{
	switch (xst->authtype) {
	case XAUTH_TYPE_GENERIC:
		if (xst->authdata.generic.usr)
			racoon_free(xst->authdata.generic.usr);

		if (xst->authdata.generic.pwd)
			racoon_free(xst->authdata.generic.pwd);

		break;

	case XAUTH_TYPE_CHAP:
	case XAUTH_TYPE_OTP:
	case XAUTH_TYPE_SKEY:
		plog(LLV_WARNING, LOCATION, NULL, 
		    "Unsupported authtype %d\n", xst->authtype);
		break;

	default:
		plog(LLV_WARNING, LOCATION, NULL, 
		    "Unexpected authtype %d\n", xst->authtype);
		break;
	}

#ifdef HAVE_LIBLDAP
	if (xst->udn != NULL)
		racoon_free(xst->udn);
#endif
	return;
}

int
xauth_rmconf_used(xauth_rmconf)
	struct xauth_rmconf **xauth_rmconf;
{
	if (*xauth_rmconf == NULL) {
		*xauth_rmconf = racoon_malloc(sizeof(**xauth_rmconf));
		if (*xauth_rmconf == NULL) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "xauth_rmconf_used: malloc failed\n");
			return -1;
		}

		(*xauth_rmconf)->login = NULL;
		(*xauth_rmconf)->pass = NULL;
		(*xauth_rmconf)->state = 0;
	}

	return 0;
}

void 
xauth_rmconf_delete(xauth_rmconf)
	struct xauth_rmconf **xauth_rmconf;
{
	if (*xauth_rmconf != NULL) { 
		if ((*xauth_rmconf)->login != NULL)
			vfree((*xauth_rmconf)->login);
		if ((*xauth_rmconf)->pass != NULL)
			vfree((*xauth_rmconf)->pass);

		racoon_free(*xauth_rmconf);
		*xauth_rmconf = NULL;
	}

	return;
}

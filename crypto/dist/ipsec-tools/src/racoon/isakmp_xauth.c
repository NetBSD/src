/*	$NetBSD: isakmp_xauth.c,v 1.4 2005/08/20 00:57:06 manu Exp $	*/

/* Id: isakmp_xauth.c,v 1.17.2.5 2005/05/20 07:31:09 manubsd Exp */

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif
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
#endif

#ifdef HAVE_LIBPAM
#include <security/pam_appl.h>

static char *PAM_usr = NULL;
static char *PAM_pwd = NULL;
static int PAM_conv(int, const struct pam_message **, 
    struct pam_response **, void *);
static struct pam_conv PAM_chat = { &PAM_conv, NULL };
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
	if (iph1->status != PHASE1ST_ESTABLISHED) {
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

void 
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
		return;
	}

	if (xst->status != XAUTHST_REQSENT) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Xauth reply while Xauth state is %d\n", xst->status);
		return;
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
			return;
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
			return;
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
		default:
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Unexpected authentication source\n");
			res = -1;
			break;
		}

		/*
		 * On failure, throttle the connexion for the remote host
		 * in order to make password attacks more difficult.
		 */
		throttle_delay = throttle_host(iph1->remote, res) - time(NULL);
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

			if ((xra = racoon_malloc(sizeof(*xra))) == NULL) {
				plog(LLV_ERROR, LOCATION, NULL, 
				    "malloc failed, bypass throttling\n");
				xauth_reply(iph1, port, id, res);
				return;
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
			sched_new(throttle_delay, xauth_reply_stub, xra);
		} else {
			xauth_reply(iph1, port, id, res);
		}
	}

	return;
}

void 
xauth_reply_stub(args)
	void *args;
{
	struct xauth_reply_arg *xra = (struct xauth_reply_arg *)args;
	struct ph1handle *iph1;

	if ((iph1 = getph1byindex(&xra->index)) != NULL)
		xauth_reply(iph1, xra->port, xra->id, xra->res);
	else
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Delayed Xauth reply: phase 1 no longer exists.\n"); 

	racoon_free(xra);
	return;
}

void 
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
		if (iph1->status == PHASE1ST_ESTABLISHED)
			isakmp_info_send_d1(iph1);
		remph1(iph1);
		delph1(iph1);

		return;
	}

	xst->status = XAUTHST_OK;
	plog(LLV_INFO, LOCATION, NULL, 
	    "login succeeded for user \"%s\"\n", usr);

	xauth_sendstatus(iph1, XAUTH_STATUS_OK, id);

	return;
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

		if (rad_config(radius_auth_state, NULL) != 0) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Cannot open librarius config file: %s\n", 
			    rad_strerror(radius_auth_state));
			rad_close(radius_auth_state);
			radius_auth_state = NULL;
			return -1;
		}
	}

	if ((isakmp_cfg_config.accounting == ISAKMP_CFG_ACCT_RADIUS) &&
	    (radius_acct_state == NULL)) {
		if ((radius_acct_state = rad_auth_open()) == NULL) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Cannot init libradius\n");
			return -1;
		}

		if (rad_config(radius_acct_state, NULL) != 0) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Cannot open librarius config file: %s\n", 
			    rad_strerror(radius_acct_state));
			rad_close(radius_acct_state);
			radius_acct_state = NULL;
			return -1;
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
				    |= ISAKMP_CFG_ADDR4_RADIUS;
				break;

			case RAD_FRAMED_IP_NETMASK:
				iph1->mode_cfg->mask4 = rad_cvt_addr(data);
				iph1->mode_cfg->flags 
				    |= ISAKMP_CFG_MASK4_RADIUS;
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
			reply[i].resp = strdup(PAM_usr);
			break;

		case PAM_PROMPT_ECHO_OFF:
			/* Send the password, libpam frees resp */
			reply[i].resp_retcode = PAM_SUCCESS;
			reply[i].resp = strdup(PAM_pwd);
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
	char *remote;
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

	return 0;

out:
	pam_end(pam, error);
	isakmp_cfg_config.port_pool[port].pam = NULL;
	return -1;
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
xauth_check(iph1)
	struct ph1handle *iph1;
{
	struct xauth_state *xst = &iph1->mode_cfg->xauth;

	/* If we don't use Xauth, then we pass */
	switch (iph1->approval->authmethod) {
	case OAKLEY_ATTR_AUTH_METHOD_HYBRID_RSA_I:
	case OAKLEY_ATTR_AUTH_METHOD_HYBRID_DSS_I:
	/* The following are not yet implemented */
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_PSKEY_I:
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_DSSSIG_I:
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSASIG_I:
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAENC_I:
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAREV_I:
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
		if (iph1->rmconf->idvtype != IDTYPE_LOGIN) {
			plog(LLV_ERROR, LOCATION, NULL, "Xauth performed "
			    "while identifier is not a login\n");
			return NULL;
		}

		if (iph1->rmconf->idv == NULL) {
			plog(LLV_ERROR, LOCATION, NULL, "Xauth performed "
			    "with no login supplied\n");
			return NULL;
		}

		dlen = iph1->rmconf->idv->l;
		break;

	case XAUTH_USER_PASSWORD:
		if (iph1->rmconf->idvtype != IDTYPE_LOGIN) 
			return NULL;

		if (iph1->rmconf->idv == NULL)
			return NULL;

		skip = sizeof(struct ipsecdoi_id_b);
		if ((usr = vmalloc(iph1->rmconf->idv->l + skip)) == NULL) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Cannot allocate memory\n");
			return NULL;
		}

		memset(usr->v, 0, skip);
		memcpy(usr->v + skip, 
		    iph1->rmconf->idv->v, 
		    iph1->rmconf->idv->l);

		if (iph1->rmconf->key) {
			/* A key given through racoonctl */
			pwd = iph1->rmconf->key;
		} else {
			if ((pwd = getpskbyname(usr)) == NULL) {
				plog(LLV_ERROR, LOCATION, NULL, 
				    "No password was found for login %s\n", 
				    iph1->rmconf->idv->v);
				vfree(usr);
				return NULL;
			}
			/* We have to free it before returning */
			freepwd = 1;
		}
		vfree(usr);

		dlen = pwd->l;

		break;

	default:
		plog(LLV_WARNING, LOCATION, NULL,
		    "Ignored attribute %d\n", type);
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
		memcpy(data, iph1->rmconf->idv->v, dlen);
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

	if ((iph1->mode_cfg->flags & ISAKMP_CFG_VENDORID_XAUTH) == 0) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Xauth mode config set but peer "
		    "did not declare itself as Xauth capable\n");
		return NULL;
	}

	type = ntohs(attr->type) & ~ISAKMP_GEN_MASK;

	switch(type) {
	case XAUTH_STATUS:
		/* If we got a failure, delete iph1 */
		if (ntohs(attr->lorv) != XAUTH_STATUS_OK) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Xauth authentication failed\n");

			EVT_PUSH(iph1->local, iph1->remote, 
			    EVTT_XAUTH_FAILED, NULL);

			iph1->mode_cfg->flags |= ISAKMP_CFG_DELETE_PH1;
		} else {
			EVT_PUSH(iph1->local, 
			    iph1->remote, EVTT_XAUTH_SUCCESS, NULL);
		}


		/* We acknowledge it */
		break;
	default:
		plog(LLV_WARNING, LOCATION, NULL,
		    "Ignored attribute %d\n", type);
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

	return;
}


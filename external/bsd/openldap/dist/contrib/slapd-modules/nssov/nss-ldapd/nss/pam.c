/*	$NetBSD: pam.c,v 1.1.1.1 2010/03/08 02:14:15 lukem Exp $	*/

/*
   pam.c - pam module functions

   Copyright (C) 2009 Howard Chu

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA
*/

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

#include "prototypes.h"
#include "common.h"
#include "compat/attrs.h"

#ifndef HAVE_PAM_PAM_MODULES_H
#include <security/pam_modules.h>
#else
#include <pam/pam_modules.h>
#endif

#define	CONST_ARG	const

#define IGNORE_UNKNOWN	1
#define IGNORE_UNAVAIL	2

#define	PLD_CTX	"PAM_LDAPD_CTX"

#define	NSS2PAM_RC(rc,ignore,ok)	\
	switch(rc) { \
	case NSS_STATUS_SUCCESS: \
		rc = ok; break; \
	case NSS_STATUS_UNAVAIL: \
		rc = (ignore & IGNORE_UNAVAIL) ? PAM_IGNORE : PAM_AUTHINFO_UNAVAIL; \
		break; \
	case NSS_STATUS_NOTFOUND: \
		rc = (ignore & IGNORE_UNKNOWN) ? PAM_IGNORE: PAM_USER_UNKNOWN; \
		break; \
	default: \
		rc = PAM_SYSTEM_ERR; break; \
	}

typedef struct pld_ctx {
	char *user;
	char *dn;
	char *tmpluser;
	char *authzmsg;
	char *oldpw;
	int authok;
	int authz;
	int sessid;
	char buf[1024];
} pld_ctx;

static int nslcd2pam_rc(int rc)
{
#define	map(i)	case NSLCD_##i : rc = i; break
	switch(rc) {
		map(PAM_SUCCESS);
		map(PAM_PERM_DENIED);
		map(PAM_AUTH_ERR);
		map(PAM_CRED_INSUFFICIENT);
		map(PAM_AUTHINFO_UNAVAIL);
		map(PAM_USER_UNKNOWN);
		map(PAM_MAXTRIES);
		map(PAM_NEW_AUTHTOK_REQD);
		map(PAM_ACCT_EXPIRED);
		map(PAM_SESSION_ERR);
		map(PAM_AUTHTOK_DISABLE_AGING);
		map(PAM_IGNORE);
		map(PAM_ABORT);
	}
	return rc;
}

static void pam_clr_ctx(
	pld_ctx *ctx)
{
	if (ctx->user) {
		free(ctx->user);
		ctx->user = NULL;
	}
	if (ctx->oldpw) {
		memset(ctx->oldpw,0,strlen(ctx->oldpw));
		free(ctx->oldpw);
		ctx->oldpw = NULL;
	}
	ctx->dn = NULL;
	ctx->tmpluser = NULL;
	ctx->authzmsg = NULL;
	ctx->authok = 0;
	ctx->authz = 0;
}

static void pam_del_ctx(
	pam_handle_t *pamh, void *data, int err)
{
	pld_ctx *ctx = data;
	pam_clr_ctx(ctx);
	free(ctx);
}

static int pam_get_ctx(
	pam_handle_t *pamh, const char *user, pld_ctx **pctx)
{
	pld_ctx *ctx = NULL;
	int rc;

	if (pam_get_data(pamh, PLD_CTX, (CONST_ARG void **)&ctx) == PAM_SUCCESS) {
		if (ctx->user && strcmp(ctx->user, user)) {
			pam_clr_ctx(ctx);
		}
		rc = PAM_SUCCESS;
	}
	if (!ctx) {
		ctx = calloc(1, sizeof(*ctx));
		if (!ctx)
			return PAM_BUF_ERR;
		rc = pam_set_data(pamh, PLD_CTX, ctx, pam_del_ctx);
		if (rc != PAM_SUCCESS)
			pam_del_ctx(pamh, ctx, 0);
	}
	if (rc == PAM_SUCCESS)
		*pctx = ctx;
	return rc;
}

static int pam_get_authtok(
	pam_handle_t *pamh, int flags, char *prompt1, char *prompt2, char **pwd)
{
	int rc;
	char *p;
	struct pam_message msg[1], *pmsg[1];
	struct pam_response *resp;
	struct pam_conv *conv;

	*pwd = NULL;

	rc = pam_get_item (pamh, PAM_CONV, (CONST_ARG void **) &conv);
	if (rc == PAM_SUCCESS) {
		pmsg[0] = &msg[0];
		msg[0].msg_style = PAM_PROMPT_ECHO_OFF;
		msg[0].msg = prompt1;
		resp = NULL;
		rc = conv->conv (1,
			 (CONST_ARG struct pam_message **) pmsg,
			 &resp, conv->appdata_ptr);
	} else {
		return rc;
	}

	if (resp != NULL) {
		if ((flags & PAM_DISALLOW_NULL_AUTHTOK) && resp[0].resp == NULL)
		{
			free (resp);
			return PAM_AUTH_ERR;
		}

		p = resp[0].resp;
		resp[0].resp = NULL;
		free (resp);
	} else {
		return PAM_CONV_ERR;
	}

	if (prompt2) {
		msg[0].msg = prompt2;
		resp = NULL;
		rc = conv->conv (1,
			 (CONST_ARG struct pam_message **) pmsg,
			 &resp, conv->appdata_ptr);
		if (resp && resp[0].resp && !strcmp(resp[0].resp, p))
			rc = PAM_SUCCESS;
		else
			rc = PAM_AUTHTOK_RECOVERY_ERR;
		if (resp) {
			if (resp[0].resp) {
				(void) memset(resp[0].resp, 0, strlen(resp[0].resp));
				free(resp[0].resp);
			}
			free(resp);
		}
	}

	if (rc == PAM_SUCCESS)
		*pwd = p;
	else if (p) {
		memset(p, 0, strlen(p));
		free(p);
	}

	return rc;
}

static enum nss_status pam_read_authc(
	TFILE *fp,pld_ctx *ctx,int *errnop)
{
	char *buffer = ctx->buf, *user;
	size_t buflen = sizeof(ctx->buf);
	size_t bufptr = 0;
	int32_t tmpint32;

	READ_STRING_BUF(fp,user);
	READ_STRING_BUF(fp,ctx->dn);
	READ_INT32(fp,ctx->authok);
	READ_INT32(fp,ctx->authz);
	READ_STRING_BUF(fp,ctx->authzmsg);
	ctx->authok = nslcd2pam_rc(ctx->authok);
	ctx->authz = nslcd2pam_rc(ctx->authz);
	return NSS_STATUS_SUCCESS;
}

static enum nss_status pam_do_authc(
	pld_ctx *ctx, const char *user, const char *svc,const char *pwd,int *errnop)
{
	NSS_BYGEN(NSLCD_ACTION_PAM_AUTHC,
		WRITE_STRING(fp,user);
		WRITE_STRING(fp,"" /* DN */);
		WRITE_STRING(fp,svc);
		WRITE_STRING(fp,pwd),
		pam_read_authc(fp,ctx,errnop));
}

#define	USE_FIRST	1
#define	TRY_FIRST	2
#define	USE_TOKEN	4

int pam_sm_authenticate(
	pam_handle_t *pamh, int flags, int argc, const char **argv)
{
	int err, rc;
	const char *username, *svc;
	char *p = NULL;
	int first_pass = 0, ignore_flags = 0;
	int i;
	pld_ctx *ctx;

	for (i = 0; i < argc; i++) {
		if (!strcmp (argv[i], "use_first_pass"))
			first_pass |= USE_FIRST;
		else if (!strcmp (argv[i], "try_first_pass"))
			first_pass |= TRY_FIRST;
		else if (!strcmp (argv[i], "ignore_unknown_user"))
			ignore_flags |= IGNORE_UNKNOWN;
		else if (!strcmp (argv[i], "ignore_authinfo_unavail"))
			ignore_flags |= IGNORE_UNAVAIL;
		else if (!strcmp (argv[i], "no_warn"))
			;
		else if (!strcmp (argv[i], "debug"))
			;
		else
			syslog (LOG_ERR, "illegal option %s", argv[i]);
	}

	rc = pam_get_user (pamh, (CONST_ARG char **) &username, NULL);
	if (rc != PAM_SUCCESS)
		return rc;

	rc = pam_get_ctx(pamh, username, &ctx);
	if (rc != PAM_SUCCESS)
		return rc;

	rc = pam_get_item (pamh, PAM_SERVICE, (CONST_ARG void **) &svc);
	if (rc != PAM_SUCCESS)
		return rc;

	for (i=0;i<2;i++) {
		if (!first_pass) {
			rc = pam_get_authtok(pamh, flags, i ? "LDAP Password: " :
				"Password: ", NULL, &p);
			i = 2;
			if (rc == PAM_SUCCESS) {
				pam_set_item(pamh, PAM_AUTHTOK, p);
				memset(p, 0, strlen(p));
				free(p);
			} else {
				break;
			}
		}
		rc = pam_get_item (pamh, PAM_AUTHTOK, (CONST_ARG void **) &p);
		if (rc == PAM_SUCCESS) {
			rc = pam_do_authc(ctx, username, svc, p, &err);
			NSS2PAM_RC(rc, ignore_flags, ctx->authok);
		}
		if (rc == PAM_SUCCESS || (first_pass & USE_FIRST)) {
			break;
		}
		first_pass = 0;
	}

	if (rc == PAM_SUCCESS) {
		ctx->user = strdup(username);
		if (ctx->authz == PAM_NEW_AUTHTOK_REQD)
			ctx->oldpw = strdup(p);
	}

	return rc;
}

int pam_sm_setcred(
	pam_handle_t *pamh, int flags, int argc, const char **argv)
{
	return PAM_SUCCESS;
}

static int
pam_warn(
	struct pam_conv *aconv, const char *message, int style, int no_warn)
{
  struct pam_message msg, *pmsg;
  struct pam_response *resp;

  if (no_warn)
    return PAM_SUCCESS;

  pmsg = &msg;

  msg.msg_style = style;
  msg.msg = (char *) message;
  resp = NULL;

  return aconv->conv (1,
		      (CONST_ARG struct pam_message **) &pmsg,
		      &resp, aconv->appdata_ptr);
}

static enum nss_status pam_read_authz(
	TFILE *fp,pld_ctx *ctx,int *errnop)
{
	char *buffer = ctx->buf;
	size_t buflen = sizeof(ctx->buf);
	size_t bufptr = 0;
	int32_t tmpint32;

	READ_STRING_BUF(fp,ctx->tmpluser);
	READ_STRING_BUF(fp,ctx->dn);
	READ_INT32(fp,ctx->authz);
	READ_STRING_BUF(fp,ctx->authzmsg);
	ctx->authz = nslcd2pam_rc(ctx->authz);
	return NSS_STATUS_SUCCESS;
}

static enum nss_status pam_do_authz(
	pld_ctx *ctx, const char *svc, const char *ruser, const char *rhost,
	const char *tty, int *errnop)
{
	NSS_BYGEN(NSLCD_ACTION_PAM_AUTHZ,
		WRITE_STRING(fp,ctx->user);
		WRITE_STRING(fp,ctx->dn);
		WRITE_STRING(fp,svc);
		WRITE_STRING(fp,ruser);
		WRITE_STRING(fp,rhost);
		WRITE_STRING(fp,tty),
		pam_read_authz(fp,ctx,errnop));
}

int pam_sm_acct_mgmt(
	pam_handle_t *pamh, int flags, int argc, const char **argv)
{
	int rc, err;
	const char *username, *svc, *ruser, *rhost, *tty;
	int no_warn = 0, ignore_flags = 0;
	int i;
	struct pam_conv *appconv;
	pld_ctx *ctx = NULL, ctx2;

	for (i = 0; i < argc; i++)
	{
		if (!strcmp (argv[i], "use_first_pass"))
			;
		else if (!strcmp (argv[i], "try_first_pass"))
			;
		else if (!strcmp (argv[i], "no_warn"))
			no_warn = 1;
		else if (!strcmp (argv[i], "ignore_unknown_user"))
			ignore_flags |= IGNORE_UNKNOWN;
		else if (!strcmp (argv[i], "ignore_authinfo_unavail"))
			ignore_flags |= IGNORE_UNAVAIL;
		else if (!strcmp (argv[i], "debug"))
			;
		else
			syslog (LOG_ERR, "illegal option %s", argv[i]);
	}

	if (flags & PAM_SILENT)
		no_warn = 1;

	rc = pam_get_item (pamh, PAM_CONV, (CONST_ARG void **) &appconv);
	if (rc != PAM_SUCCESS)
		return rc;

	rc = pam_get_user (pamh, (CONST_ARG char **) &username, NULL);
	if (rc != PAM_SUCCESS)
		return rc;

	if (username == NULL)
		return PAM_USER_UNKNOWN;

	rc = pam_get_ctx(pamh, username, &ctx);
	if (rc != PAM_SUCCESS)
		return rc;

	rc = pam_get_item (pamh, PAM_SERVICE, (CONST_ARG void **) &svc);
	if (rc != PAM_SUCCESS)
		return rc;

	rc = pam_get_item (pamh, PAM_RUSER, (CONST_ARG void **) &ruser);
	if (rc != PAM_SUCCESS)
		return rc;

	rc = pam_get_item (pamh, PAM_RHOST, (CONST_ARG void **) &rhost);
	if (rc != PAM_SUCCESS)
		return rc;

	rc = pam_get_item (pamh, PAM_TTY, (CONST_ARG void **) &tty);
	if (rc != PAM_SUCCESS)
		return rc;

	ctx2.dn = ctx->dn;
	ctx2.user = ctx->user;
	rc = pam_do_authz(&ctx2, svc, ruser, rhost, tty, &err);
	NSS2PAM_RC(rc, ignore_flags, PAM_SUCCESS);
	if (rc != PAM_SUCCESS) {
		if (rc != PAM_IGNORE)
			pam_warn(appconv, "LDAP authorization failed", PAM_ERROR_MSG, no_warn);
	} else {
		rc = ctx2.authz;
		if (ctx2.authzmsg && ctx2.authzmsg[0])
			pam_warn(appconv, ctx2.authzmsg, PAM_TEXT_INFO, no_warn);
		if (ctx2.authz == PAM_SUCCESS) {
			rc = ctx->authz;
			if (ctx->authzmsg && ctx->authzmsg[0])
				pam_warn(appconv, ctx->authzmsg, PAM_TEXT_INFO, no_warn);
		}
	}
	if ( rc == PAM_SUCCESS && ctx->tmpluser && ctx->tmpluser[0] ) {
		rc = pam_set_item(pamh, PAM_USER, ctx->tmpluser);
	}
	return rc;
}

static enum nss_status pam_read_sess(
	TFILE *fp,pld_ctx *ctx,int *errnop)
{
	int tmpint32;
	READ_INT32(fp,ctx->sessid);
	return NSS_STATUS_SUCCESS;
}

static enum nss_status pam_do_sess(
	pam_handle_t *pamh,pld_ctx *ctx,int action,int *errnop)
{
	const char *svc = NULL, *tty = NULL, *rhost = NULL, *ruser = NULL;
	
	pam_get_item (pamh, PAM_SERVICE, (CONST_ARG void **) &svc);
	pam_get_item (pamh, PAM_TTY, (CONST_ARG void **) &tty);
	pam_get_item (pamh, PAM_RHOST, (CONST_ARG void **) &rhost);
	pam_get_item (pamh, PAM_RUSER, (CONST_ARG void **) &ruser);

	{
	NSS_BYGEN(action,
		WRITE_STRING(fp,ctx->user);
		WRITE_STRING(fp,ctx->dn);
		WRITE_STRING(fp,svc);
		WRITE_STRING(fp,tty);
		WRITE_STRING(fp,rhost);
		WRITE_STRING(fp,ruser);
		WRITE_INT32(fp,ctx->sessid),
		pam_read_sess(fp,ctx,errnop));
	}
}

static int pam_sm_session(
	pam_handle_t *pamh, int flags, int argc, const char **argv,
	int action, int *no_warn)
{
	int rc, err;
	const char *username;
	int ignore_flags = 0;
	int i, success = PAM_SUCCESS;
	pld_ctx *ctx = NULL;

	for (i = 0; i < argc; i++)
	{
		if (!strcmp (argv[i], "use_first_pass"))
			;
		else if (!strcmp (argv[i], "try_first_pass"))
			;
		else if (!strcmp (argv[i], "no_warn"))
			*no_warn = 1;
		else if (!strcmp (argv[i], "ignore_unknown_user"))
			ignore_flags |= IGNORE_UNKNOWN;
		else if (!strcmp (argv[i], "ignore_authinfo_unavail"))
			ignore_flags |= IGNORE_UNAVAIL;
		else if (!strcmp (argv[i], "debug"))
			;
		else
			syslog (LOG_ERR, "illegal option %s", argv[i]);
	}

	if (flags & PAM_SILENT)
		*no_warn = 1;

	rc = pam_get_user (pamh, (CONST_ARG char **) &username, NULL);
	if (rc != PAM_SUCCESS)
		return rc;

	if (username == NULL)
		return PAM_USER_UNKNOWN;

	rc = pam_get_ctx(pamh, username, &ctx);
	if (rc != PAM_SUCCESS)
		return rc;

	rc = pam_do_sess(pamh, ctx, action, &err);
	NSS2PAM_RC(rc, ignore_flags, PAM_SUCCESS);
	return rc;
}

int pam_sm_open_session(
	pam_handle_t *pamh, int flags, int argc, const char **argv)
{
	int rc, no_warn = 0;
	struct pam_conv *appconv;

	rc = pam_get_item (pamh, PAM_CONV, (CONST_ARG void **) &appconv);
	if (rc != PAM_SUCCESS)
		return rc;

	rc = pam_sm_session(pamh,flags,argc,argv,NSLCD_ACTION_PAM_SESS_O,&no_warn);
	if (rc != PAM_SUCCESS && rc != PAM_IGNORE)
		pam_warn(appconv, "LDAP open_session failed", PAM_ERROR_MSG, no_warn);
	return rc;
}

int pam_sm_close_session(
	pam_handle_t *pamh, int flags, int argc, const char **argv)
{
	int rc, no_warn = 0;;
	struct pam_conv *appconv;

	rc = pam_get_item (pamh, PAM_CONV, (CONST_ARG void **) &appconv);
	if (rc != PAM_SUCCESS)
		return rc;

	rc = pam_sm_session(pamh,flags,argc,argv,NSLCD_ACTION_PAM_SESS_C,&no_warn);
	if (rc != PAM_SUCCESS && rc != PAM_IGNORE)
		pam_warn(appconv, "LDAP close_session failed", PAM_ERROR_MSG, no_warn);
	return rc;
}

static enum nss_status pam_read_pwmod(
	TFILE *fp,pld_ctx *ctx,int *errnop)
{
	char *buffer = ctx->buf, *user;
	size_t buflen = sizeof(ctx->buf);
	size_t bufptr = 0;
	int32_t tmpint32;

	READ_STRING_BUF(fp,user);
	READ_STRING_BUF(fp,ctx->dn);
	READ_INT32(fp,ctx->authz);
	READ_STRING_BUF(fp,ctx->authzmsg);
	ctx->authz = nslcd2pam_rc(ctx->authz);
	return NSS_STATUS_SUCCESS;
}

static enum nss_status pam_do_pwmod(
	pld_ctx *ctx, const char *user, const char *svc,
	const char *oldpw, const char *newpw, int *errnop)
{
	NSS_BYGEN(NSLCD_ACTION_PAM_PWMOD,
		WRITE_STRING(fp,user);
		WRITE_STRING(fp,ctx->dn);
		WRITE_STRING(fp,svc);
		WRITE_STRING(fp,oldpw);
		WRITE_STRING(fp,newpw),
		pam_read_pwmod(fp,ctx,errnop));
}

int pam_sm_chauthtok(
	pam_handle_t *pamh, int flags, int argc, const char **argv)
{
	int rc, err;
	const char *username, *p = NULL, *q = NULL, *svc;
	int first_pass = 0, no_warn = 0, ignore_flags = 0;
	int i, success = PAM_SUCCESS;
	struct pam_conv *appconv;
	pld_ctx *ctx = NULL;

	for (i = 0; i < argc; i++)
	{
		if (!strcmp (argv[i], "use_first_pass"))
			first_pass |= USE_FIRST;
		else if (!strcmp (argv[i], "try_first_pass"))
			first_pass |= TRY_FIRST;
		else if (!strcmp (argv[i], "use_authtok"))
			first_pass |= USE_TOKEN;
		else if (!strcmp (argv[i], "no_warn"))
			no_warn = 1;
		else if (!strcmp (argv[i], "ignore_unknown_user"))
			ignore_flags |= IGNORE_UNKNOWN;
		else if (!strcmp (argv[i], "ignore_authinfo_unavail"))
			ignore_flags |= IGNORE_UNAVAIL;
		else if (!strcmp (argv[i], "debug"))
			;
		else
			syslog (LOG_ERR, "illegal option %s", argv[i]);
	}

	if (flags & PAM_SILENT)
		no_warn = 1;

	rc = pam_get_item (pamh, PAM_CONV, (CONST_ARG void **) &appconv);
	if (rc != PAM_SUCCESS)
		return rc;

	rc = pam_get_user (pamh, (CONST_ARG char **) &username, NULL);
	if (rc != PAM_SUCCESS)
		return rc;

	if (username == NULL)
		return PAM_USER_UNKNOWN;

	rc = pam_get_ctx(pamh, username, &ctx);
	if (rc != PAM_SUCCESS)
		return rc;

	rc = pam_get_item (pamh, PAM_SERVICE, (CONST_ARG void **) &svc);
	if (rc != PAM_SUCCESS)
		return rc;

	if (flags & PAM_PRELIM_CHECK) {
		if (getuid()) {
			if (!first_pass) {
				rc = pam_get_authtok(pamh, flags, "(current) LDAP Password: ",
					NULL, &p);
				if (rc == PAM_SUCCESS) {
					pam_set_item(pamh, PAM_OLDAUTHTOK, p);
					memset(p, 0, strlen(p));
					free(p);
				}
			}
			rc = pam_get_item(pamh, PAM_OLDAUTHTOK, &p);
			if (rc) return rc;
		} else {
			rc = PAM_SUCCESS;
		}
		if (!ctx->dn) {
			rc = pam_do_pwmod(ctx, username, svc, p, NULL, &err);
			NSS2PAM_RC(rc, ignore_flags, PAM_SUCCESS);
		}
		return rc;
	}

	rc = pam_get_item(pamh, PAM_OLDAUTHTOK, &p);
	if (rc) return rc;

	if (!p)
		p = ctx->oldpw;

	if (first_pass) {
		rc = pam_get_item(pamh, PAM_AUTHTOK, &q);
		if ((rc != PAM_SUCCESS || !q) && (first_pass & (USE_FIRST|USE_TOKEN))) {
			if (rc == PAM_SUCCESS)
				rc = PAM_AUTHTOK_RECOVERY_ERR;
			return rc;
		}
	}
	if (!q) {
		rc = pam_get_authtok(pamh, flags, "Enter new LDAP Password: ",
			"Retype new LDAP Password: ", &q);
		if (rc == PAM_SUCCESS) {
			pam_set_item(pamh, PAM_AUTHTOK, q);
			memset(q, 0, strlen(q));
			free(q);
			rc = pam_get_item(pamh, PAM_AUTHTOK, &q);
		}
		if (rc != PAM_SUCCESS)
			return rc;
	}
	rc = pam_do_pwmod(ctx, username, svc, p, q, &err);
	p = NULL; q = NULL;
	NSS2PAM_RC(rc, ignore_flags, PAM_SUCCESS);
	if (rc == PAM_SUCCESS) {
		rc = ctx->authz;
		if (rc != PAM_SUCCESS)
			pam_warn(appconv, ctx->authzmsg, PAM_ERROR_MSG, no_warn);
	} else if (rc != PAM_IGNORE)
		pam_warn(appconv, "LDAP pwmod failed", PAM_ERROR_MSG, no_warn);
	return rc;
}

#ifdef PAM_STATIC
struct pam_module _modstruct = {
	"pam_ldapd",
	pam_sm_authenticate,
	pam_sm_setcred,
	pam_sm_acct_mgmt,
	pam_sm_open_session,
	pam_sm_close_session,
	pam_sm_chauthtok
};
#endif /* PAM_STATIC */

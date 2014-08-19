/*	$NetBSD: auth.h,v 1.5.8.2 2014/08/19 23:45:24 tls Exp $	*/
/* $OpenBSD: auth.h,v 1.76 2013/07/19 07:37:48 markus Exp $ */

/*
 * Copyright (c) 2000 Markus Friedl.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef AUTH_H
#define AUTH_H

#include <signal.h>

#include <openssl/rsa.h>

#ifdef HAVE_LOGIN_CAP
#include <login_cap.h>
#endif
#ifdef BSD_AUTH
#include <bsd_auth.h>
#endif
#ifdef KRB5
#include <krb5.h>
#endif

typedef struct Authctxt Authctxt;
typedef struct Authmethod Authmethod;
typedef struct KbdintDevice KbdintDevice;

struct Authctxt {
	sig_atomic_t	 success;
	int		 authenticated;	/* authenticated and alarms cancelled */
	int		 postponed;	/* authentication needs another step */
	int		 valid;		/* user exists and is allowed to login */
	int		 attempt;
	int		 failures;
	int		 server_caused_failure; 
	int		 force_pwchange;
	char		*user;		/* username sent by the client */
	char		*service;
	struct passwd	*pw;		/* set if 'valid' */
	char		*style;
	void		*kbdintctxt;
	char		*info;		/* Extra info for next auth_log */
	void		*jpake_ctx;
#ifdef BSD_AUTH
	auth_session_t	*as;
#endif
#ifdef KRB4
	char		*krb4_ticket_file;
#endif 
	char		**auth_methods;	/* modified from server config */
	u_int		 num_auth_methods;
#ifdef KRB5
	krb5_context	 krb5_ctx;
	krb5_auth_context krb5_auth_ctx;
	krb5_ccache	 krb5_fwd_ccache;
	krb5_principal	 krb5_user;
	char		*krb5_ticket_file;
#endif
	void		*methoddata;
};
/*
 * Every authentication method has to handle authentication requests for
 * non-existing users, or for users that are not allowed to login. In this
 * case 'valid' is set to 0, but 'user' points to the username requested by
 * the client.
 */

#ifdef USE_PAM
#include "auth-pam.h"
#endif

struct Authmethod {
	const char	*name;
	int	(*userauth)(Authctxt *authctxt);
	int	*enabled;
};

/*
 * Keyboard interactive device:
 * init_ctx	returns: non NULL upon success
 * query	returns: 0 - success, otherwise failure
 * respond	returns: 0 - success, 1 - need further interaction,
 *		otherwise - failure
 */
struct KbdintDevice
{
	const char *name;
	void*	(*init_ctx)(Authctxt*);
	int	(*query)(void *ctx, char **name, char **infotxt,
		    u_int *numprompts, char ***prompts, u_int **echo_on);
	int	(*respond)(void *ctx, u_int numresp, char **responses);
	void	(*free_ctx)(void *ctx);
};

void 	 disable_forwarding(void);
int      auth_rhosts(struct passwd *, const char *);
int
auth_rhosts2(struct passwd *, const char *, const char *, const char *);

int	 auth_rhosts_rsa(Authctxt *, char *, Key *);
int      auth_password(Authctxt *, const char *);
int      auth_rsa(Authctxt *, BIGNUM *);
int      auth_rsa_challenge_dialog(Key *);
BIGNUM	*auth_rsa_generate_challenge(Key *);
int	 auth_rsa_verify_response(Key *, BIGNUM *, u_char[]);
int	 auth_rsa_key_allowed(struct passwd *, BIGNUM *, Key **);

int	 auth_rhosts_rsa_key_allowed(struct passwd *, char *, char *, Key *);
int	 hostbased_key_allowed(struct passwd *, const char *, char *, Key *);
int	 user_key_allowed(struct passwd *, Key *);
void	 pubkey_auth_info(Authctxt *, const Key *, const char *, ...)
	    __attribute__((__format__ (printf, 3, 4)));

#ifdef KRB4
#include <krb.h>
int     auth_krb4(Authctxt *, KTEXT, char **, KTEXT);
int	auth_krb4_password(Authctxt *, const char *);
void    krb4_cleanup_proc(void *);

#ifdef AFS
#include <kafs.h>
int     auth_krb4_tgt(Authctxt *, const char *);
int     auth_afs_token(Authctxt *, const char *);
#endif /* AFS */

#endif /* KRB4 */

struct stat;
int	 auth_secure_path(const char *, struct stat *, const char *, uid_t,
    char *, size_t);

#ifdef KRB5
int	auth_krb5(Authctxt *authctxt, krb5_data *auth, char **client, krb5_data *);
int	auth_krb5_tgt(Authctxt *authctxt, krb5_data *tgt);
int	auth_krb5_password(Authctxt *authctxt, const char *password);
void	krb5_cleanup_proc(Authctxt *authctxt);
#endif /* KRB5 */

void	do_authentication(Authctxt *);
void	do_authentication2(Authctxt *);

void	auth_info(Authctxt *authctxt, const char *, ...)
	    __attribute__((__format__ (printf, 2, 3)))
	    __attribute__((__nonnull__ (2)));
void	auth_log(Authctxt *, int, int, const char *, const char *);
void	userauth_finish(Authctxt *, int, const char *, const char *);
int	auth_root_allowed(const char *);

char	*auth2_read_banner(void);
int	 auth2_methods_valid(const char *, int);
int	 auth2_update_methods_lists(Authctxt *, const char *, const char *);
int	 auth2_setup_methods_lists(Authctxt *);
int	 auth2_method_allowed(Authctxt *, const char *, const char *);

void	privsep_challenge_enable(void);

int	auth2_challenge(Authctxt *, char *);
void	auth2_challenge_stop(Authctxt *);
int	bsdauth_query(void *, char **, char **, u_int *, char ***, u_int **);
int	bsdauth_respond(void *, u_int, char **);
int	skey_query(void *, char **, char **, u_int *, char ***, u_int **);
int	skey_respond(void *, u_int, char **);

void	auth2_jpake_get_pwdata(Authctxt *, BIGNUM **, char **, char **);
void	auth2_jpake_stop(Authctxt *);

int	allowed_user(struct passwd *);
struct passwd * getpwnamallow(const char *user);

char	*get_challenge(Authctxt *);
int	verify_response(Authctxt *, const char *);

char	*expand_authorized_keys(const char *, struct passwd *pw);
char	*authorized_principals_file(struct passwd *);

FILE	*auth_openkeyfile(const char *, struct passwd *, int);
FILE	*auth_openprincipals(const char *, struct passwd *, int);
int	 auth_key_is_revoked(Key *);

HostStatus
check_key_in_hostfiles(struct passwd *, Key *, const char *,
    const char *, const char *);

/* hostkey handling */
Key	*get_hostkey_by_index(int);
Key	*get_hostkey_public_by_index(int);
Key	*get_hostkey_public_by_type(int);
Key	*get_hostkey_private_by_type(int);
int	 get_hostkey_index(Key *);
int	 ssh1_session_key(BIGNUM *);
void	 sshd_hostkey_sign(Key *, Key *, u_char **, u_int *, u_char *, u_int);

/* debug messages during authentication */
void	 auth_debug_add(const char *fmt,...) __attribute__((format(printf, 1, 2)));
void	 auth_debug_send(void);
void	 auth_debug_reset(void);

struct passwd *fakepw(void);

#define AUTH_FAIL_MSG "Too many authentication failures for %.100s"

#define SKEY_PROMPT "\nS/Key Password: "

#if defined(KRB5) && !defined(HEIMDAL)
#include <krb5.h>
krb5_error_code ssh_krb5_cc_gen(krb5_context, krb5_ccache *);
#endif
#endif

/*	$NetBSD: auth1.c,v 1.12 2015/07/03 00:59:59 christos Exp $	*/
/* $OpenBSD: auth1.c,v 1.82 2014/07/15 15:54:14 millert Exp $ */
/*
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 */

#include "includes.h"
__RCSID("$NetBSD: auth1.c,v 1.12 2015/07/03 00:59:59 christos Exp $");
#include <sys/types.h>
#include <sys/queue.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

#include "xmalloc.h"
#include "rsa.h"
#include "ssh1.h"
#include "packet.h"
#include "buffer.h"
#include "log.h"
#include "misc.h"
#include "servconf.h"
#include "compat.h"
#include "key.h"
#include "hostfile.h"
#include "auth.h"
#include "channels.h"
#include "session.h"
#include "uidswap.h"
#ifdef GSSAPI
#include "ssh-gss.h"
#endif
#include "monitor_wrap.h"
#include "buffer.h"
#include "pfilter.h"

/* import */
extern ServerOptions options;
extern Buffer loginmsg;

static int auth1_process_password(Authctxt *);
static int auth1_process_rsa(Authctxt *);
static int auth1_process_rhosts_rsa(Authctxt *);
static int auth1_process_tis_challenge(Authctxt *);
static int auth1_process_tis_response(Authctxt *);
#if defined(KRB4) || defined(KRB5)
static int auth1_process_kerberos(Authctxt *);
#endif

struct AuthMethod1 {
	int type;
	const char *name;
	int *enabled;
	int (*method)(Authctxt *);
};

const struct AuthMethod1 auth1_methods[] = {
	{
		SSH_CMSG_AUTH_PASSWORD, "password",
		&options.password_authentication, auth1_process_password
	},
	{
		SSH_CMSG_AUTH_RSA, "rsa",
		&options.rsa_authentication, auth1_process_rsa
	},
	{
		SSH_CMSG_AUTH_RHOSTS_RSA, "rhosts-rsa",
		&options.rhosts_rsa_authentication, auth1_process_rhosts_rsa
	},
	{
		SSH_CMSG_AUTH_TIS, "challenge-response",
		&options.challenge_response_authentication,
		auth1_process_tis_challenge
	},
	{
		SSH_CMSG_AUTH_TIS_RESPONSE, "challenge-response",
		&options.challenge_response_authentication,
		auth1_process_tis_response
	},
#if defined(KRB4) || defined(KRB5)
	{
		SSH_CMSG_AUTH_KERBEROS, "kerberos",
		&options.kerberos_authentication,
		auth1_process_kerberos
	},
#endif /* KRB4 || KRB5 */
	{ -1, NULL, NULL, NULL}
};

static const struct AuthMethod1
*lookup_authmethod1(int type)
{
	int i;

	for (i = 0; auth1_methods[i].name != NULL; i++)
		if (auth1_methods[i].type == type)
			return (&(auth1_methods[i]));

	return (NULL);
}

static const char *
get_authname(int type)
{
	const struct AuthMethod1 *a;
	static char buf[64];

	if ((a = lookup_authmethod1(type)) != NULL)
		return (a->name);
	snprintf(buf, sizeof(buf), "bad-auth-msg-%d", type);
	return (buf);
}

/*ARGSUSED*/
static int
auth1_process_password(Authctxt *authctxt)
{
	int authenticated = 0;
	char *password;
	u_int dlen;

	/*
	 * Read user password.  It is in plain text, but was
	 * transmitted over the encrypted channel so it is
	 * not visible to an outside observer.
	 */
	password = packet_get_string(&dlen);
	packet_check_eom();

	/* Try authentication with the password. */
	authenticated = PRIVSEP(auth_password(authctxt, password));

	explicit_bzero(password, dlen);
	free(password);

	return (authenticated);
}

#if defined(KRB4) || defined(KRB5)
static int
auth1_process_kerberos(Authctxt *authctxt)
{
	int authenticated = 0;
	u_int dlen;
	char *client_user;
	char *kdata = packet_get_string(&dlen);
	packet_check_eom();

	if (kdata[0] == 4) { /* KRB_PROT_VERSION */
#ifdef KRB4
		KTEXT_ST tkt, reply;
		tkt.length = dlen;
		if (tkt.length < MAX_KTXT_LEN)
			memcpy(tkt.dat, kdata, tkt.length);

		if (PRIVSEP(auth_krb4(authctxt, &tkt, &client_user, &reply))) {
			authenticated = 1;

			packet_start(SSH_SMSG_AUTH_KERBEROS_RESPONSE);
			packet_put_string((char *)
			    reply.dat, reply.length);
			packet_send();
			packet_write_wait();

			free(client_user);
		}
#endif /* KRB4 */
	} else {
#ifdef KRB5
		krb5_data tkt, reply;
		tkt.length = dlen;
		tkt.data = kdata;

		if (PRIVSEP(auth_krb5(authctxt, &tkt, &client_user, &reply))) {
			authenticated = 1;

			/* Send response to client */
			packet_start(SSH_SMSG_AUTH_KERBEROS_RESPONSE);
			packet_put_string((char *)reply.data, reply.length);
			packet_send();
			packet_write_wait();

			if (reply.length)
				free(reply.data);
			free(client_user);
		}
#endif /* KRB5 */
	}
	free(kdata);
	return authenticated;
}
#endif /* KRB4 || KRB5 */

/*ARGSUSED*/
static int
auth1_process_rhosts_rsa(Authctxt *authctxt)
{
	int keybits, authenticated = 0;
	u_int bits;
	char *client_user;
	Key *client_host_key;
	u_int ulen;

	/*
	 * Get client user name.  Note that we just have to
	 * trust the client; root on the client machine can
	 * claim to be any user.
	 */
	client_user = packet_get_cstring(&ulen);

	/* Get the client host key. */
	client_host_key = key_new(KEY_RSA1);
	bits = packet_get_int();
	packet_get_bignum(client_host_key->rsa->e);
	packet_get_bignum(client_host_key->rsa->n);

	keybits = BN_num_bits(client_host_key->rsa->n);
	if (keybits < 0 || bits != (u_int)keybits) {
		verbose("Warning: keysize mismatch for client_host_key: "
		    "actual %d, announced %d",
		    BN_num_bits(client_host_key->rsa->n), bits);
	}
	packet_check_eom();

	authenticated = auth_rhosts_rsa(authctxt, client_user,
	    client_host_key);
	key_free(client_host_key);

	auth_info(authctxt, "ruser %.100s", client_user);
	free(client_user);

	return (authenticated);
}

/*ARGSUSED*/
static int
auth1_process_rsa(Authctxt *authctxt)
{
	int authenticated = 0;
	BIGNUM *n;

	/* RSA authentication requested. */
	if ((n = BN_new()) == NULL)
		fatal("do_authloop: BN_new failed");
	packet_get_bignum(n);
	packet_check_eom();
	authenticated = auth_rsa(authctxt, n);
	BN_clear_free(n);

	return (authenticated);
}

/*ARGSUSED*/
static int
auth1_process_tis_challenge(Authctxt *authctxt)
{
	char *challenge;

	if ((challenge = get_challenge(authctxt)) == NULL)
		return (0);

	debug("sending challenge '%s'", challenge);
	packet_start(SSH_SMSG_AUTH_TIS_CHALLENGE);
	packet_put_cstring(challenge);
	free(challenge);
	packet_send();
	packet_write_wait();

	return (-1);
}

/*ARGSUSED*/
static int
auth1_process_tis_response(Authctxt *authctxt)
{
	int authenticated = 0;
	char *response;
	u_int dlen;

	response = packet_get_string(&dlen);
	packet_check_eom();
	authenticated = verify_response(authctxt, response);
	explicit_bzero(response, dlen);
	free(response);

	return (authenticated);
}

/*
 * read packets, try to authenticate the user and
 * return only if authentication is successful
 */
static void
do_authloop(Authctxt *authctxt)
{
	int authenticated = 0;
	int type = 0;
	const struct AuthMethod1 *meth;

	debug("Attempting authentication for %s%.100s.",
	    authctxt->valid ? "" : "invalid user ", authctxt->user);

	/* If the user has no password, accept authentication immediately. */
	if (options.permit_empty_passwd && options.password_authentication &&
#if defined(KRB4) || defined(KRB5)
	    (!options.kerberos_authentication || options.kerberos_or_local_passwd) &&
#endif
	    PRIVSEP(auth_password(authctxt, __UNCONST("")))) {
#ifdef USE_PAM
 		if (options.use_pam && PRIVSEP(do_pam_account()))
#endif
		{
			auth_log(authctxt, 1, 0, "without authentication",
			    NULL);
			return;
		}
		return;
	}

	/* Indicate that authentication is needed. */
	packet_start(SSH_SMSG_FAILURE);
	packet_send();
	packet_write_wait();

	for (;;) {
		/* default to fail */
		authenticated = 0;


		/* Get a packet from the client. */
		type = packet_read();
		if (authctxt->failures >= options.max_authtries)
			goto skip;
		if ((meth = lookup_authmethod1(type)) == NULL) {
			logit("Unknown message during authentication: "
			    "type %d", type);
			goto skip;
		}

		if (!*(meth->enabled)) {
			verbose("%s authentication disabled.", meth->name);
			goto skip;
		}

		authenticated = meth->method(authctxt);
		if (authenticated == -1)
			continue; /* "postponed" */

#ifdef BSD_AUTH
		if (authctxt->as) {
			auth_close(authctxt->as);
			authctxt->as = NULL;
		}
#endif
		if (!authctxt->valid && authenticated)
			fatal("INTERNAL ERROR: authenticated invalid user %s",
			    authctxt->user);

		/* Special handling for root */
		if (authenticated && authctxt->pw->pw_uid == 0 &&
		    !auth_root_allowed(meth->name))
			authenticated = 0;

#ifdef USE_PAM
		if (options.use_pam && authenticated &&
		    !PRIVSEP(do_pam_account())) {
			char *msg;
			size_t len;

			error("Access denied for user %s by PAM account "
			    "configuration", authctxt->user);
			len = buffer_len(&loginmsg);
			buffer_append(&loginmsg, "\0", 1);
			msg = (char *)buffer_ptr(&loginmsg);
			/* strip trailing newlines */
			if (len > 0)
				while (len > 0 && msg[--len] == '\n')
					msg[len] = '\0';
			else
				msg = __UNCONST("Access denied.");
			packet_disconnect("%s", msg);
		}
#endif

 skip:
		/* Log before sending the reply */
		auth_log(authctxt, authenticated, 0, get_authname(type), NULL);

		if (authenticated)
			return;

		if (++authctxt->failures >= options.max_authtries)
			auth_maxtries_exceeded(authctxt);

		packet_start(SSH_SMSG_FAILURE);
		packet_send();
		packet_write_wait();
	}
}

/*
 * Performs authentication of an incoming connection.  Session key has already
 * been exchanged and encryption is enabled.
 */
void
do_authentication(Authctxt *authctxt)
{
	u_int ulen;
	char *user, *style = NULL;

	/* Get the name of the user that we wish to log in as. */
	packet_read_expect(SSH_CMSG_USER);

	/* Get the user name. */
	user = packet_get_cstring(&ulen);
	packet_check_eom();

	if ((style = strchr(user, ':')) != NULL)
		*style++ = '\0';

	authctxt->user = user;
	authctxt->style = style;

	/* Verify that the user is a valid user. */
	if ((authctxt->pw = PRIVSEP(getpwnamallow(user))) != NULL)
		authctxt->valid = 1;
	else {
		debug("do_authentication: invalid user %s", user);
		authctxt->pw = fakepw();
		pfilter_notify(1);
	}

	/* Configuration may have changed as a result of Match */
	if (options.num_auth_methods != 0)
		fatal("AuthenticationMethods is not supported with SSH "
		    "protocol 1");

	setproctitle("%s%s", authctxt->valid ? user : "unknown",
	    use_privsep ? " [net]" : "");

#ifdef USE_PAM
	if (options.use_pam)
		PRIVSEP(start_pam(authctxt));
#endif

	/*
	 * If we are not running as root, the user must have the same uid as
	 * the server.
	 */
	if (!use_privsep && getuid() != 0 && authctxt->pw &&
	    authctxt->pw->pw_uid != getuid())
		packet_disconnect("Cannot change user when server not running as root.");

	/*
	 * Loop until the user has been authenticated or the connection is
	 * closed, do_authloop() returns only if authentication is successful
	 */
	do_authloop(authctxt);

	/* The user has been authenticated and accepted. */
	packet_start(SSH_SMSG_SUCCESS);
	packet_send();
	packet_write_wait();
}

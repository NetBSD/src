/*	$NetBSD: k5login.c,v 1.9 1999/12/05 23:39:11 aidan Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)klogin.c	5.11 (Berkeley) 7/12/92";
#endif
__RCSID("$NetBSD: k5login.c,v 1.9 1999/12/05 23:39:11 aidan Exp $");
#endif /* not lint */

#ifdef KERBEROS5
#include <sys/param.h>
#include <sys/syslog.h>
#include <krb5/krb5.h>
#include <kerberosIV/com_err.h>
#include <pwd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define KRB5_DEFAULT_OPTIONS 0
#define KRB5_DEFAULT_LIFE 60*60*10 /* 10 hours */

krb5_data tgtname = {
    0,
    KRB5_TGS_NAME_SIZE,
    KRB5_TGS_NAME
};

krb5_context kcontext;

int notickets;
char *krbtkfile_env;
extern char *tty;

static char tkt_location[MAXPATHLEN];
static krb5_creds forw_creds;
int have_forward;
static krb5_principal me, server;
int use_krb5;

int k5_read_creds __P((char *));
int k5_write_creds __P((void));
int klogin __P((struct passwd *, char *, char *, char *));
void kdestroy __P((void));

/*
 * Attempt to read forwarded kerberos creds
 *
 * return 0 on success (forwarded creds in memory)
 *        1 if no forwarded creds.
 */
int
k5_read_creds(username)
	char *username;
{
	krb5_error_code code;
	krb5_creds mcreds;
	krb5_ccache ccache;

	if (! use_krb5)
		return(1);

	have_forward = 0;
	memset((char*) &mcreds, 0, sizeof(forw_creds));
	memset((char*) &forw_creds, 0, sizeof(forw_creds));

	code = krb5_cc_default(kcontext, &ccache);
	if (code) {
		com_err("login", code, "while getting default ccache");
		return(1);
	}

	code = krb5_parse_name(kcontext, username, &me);
	if (code) {
		com_err("login", code, "when parsing name %s", username);
		return(1);
	}

	mcreds.client = me;
	code = krb5_build_principal_ext(kcontext, &mcreds.server,
					krb5_princ_realm(kcontext, me)->length,
					krb5_princ_realm(kcontext, me)->data,
					tgtname.length, tgtname.data,
					krb5_princ_realm(kcontext, me)->length,
					krb5_princ_realm(kcontext, me)->data,
					0);
	if (code) {
		com_err("login", code, "while building server name");
		goto nuke_ccache;
	}

	code = krb5_cc_retrieve_cred(kcontext, ccache, 0,
				       &mcreds, &forw_creds);
	if (code) {
		com_err("login", code, "while retrieving V5 initial ticket for copy");
		goto nuke_ccache;
	}
	have_forward = 1;

	strcpy(tkt_location, getenv("KRB5CCNAME"));
	krbtkfile_env = tkt_location;
	notickets = 0;

nuke_ccache:
	krb5_cc_destroy(kcontext, ccache);
	return(!have_forward);
}

int
k5_write_creds()
{
	krb5_error_code code;
	krb5_ccache ccache;

	if (! use_krb5)
		return(1);
	if (!have_forward)
		return(1);
	code = krb5_cc_default(kcontext, &ccache);
	if (code) {
		com_err("login", code, "while getting default ccache");
		return(1);
	}

	code = krb5_cc_initialize(kcontext, ccache, me);
	if (code) {
		com_err("login", code, "while re-initializing V5 ccache as user");
		goto nuke_ccache_contents;
	}

	code = krb5_cc_store_cred(kcontext, ccache, &forw_creds);
	if (code) {
		com_err("login", code, "while re-storing V5 ccache as user");
		goto nuke_ccache_contents;
	}

nuke_ccache_contents:
	krb5_free_cred_contents(kcontext, &forw_creds);
	return(code != 0);
}

/*
 * Attempt to log the user in using Kerberos authentication
 *
 * return 0 on success (will be logged in)
 *	  1 if Kerberos failed (try local password in login)
 */
int
klogin(pw, instance, localhost, password)
	struct passwd *pw;
	char *instance, *localhost, *password;
{
        krb5_error_code kerror;
	krb5_address **my_addresses;
	krb5_creds my_creds;
	krb5_timestamp now;
	krb5_ccache ccache = NULL;
	long lifetime = KRB5_DEFAULT_LIFE;
	int options = KRB5_DEFAULT_OPTIONS;
	char *realm, *client_name;
	char *principal;

	/*
	 * Root logins don't use Kerberos.
	 * If we have a realm, try getting a ticket-granting ticket
	 * and using it to authenticate.  Otherwise, return
	 * failure so that we can try the normal passwd file
	 * for a password.  If that's ok, log the user in
	 * without issuing any tickets.
	 */
	if (! use_krb5 ||
	    strcmp(pw->pw_name, "root") == 0 ||
	    krb5_get_default_realm(kcontext, &realm))
		return (1);

	/*
	 * get TGT for local realm
	 * tickets are stored in a file named TKT_ROOT plus uid
	 * except for user.root tickets.
	 */

	if (strcmp(instance, "root") != 0)
	    (void)snprintf(tkt_location, sizeof tkt_location,
		"FILE:/tmp/krb5cc_%d.%s", pw->pw_uid, tty);
	else
	    (void)snprintf(tkt_location, sizeof tkt_location,
		"FILE:/tmp/krb5cc_root_%d.%s", pw->pw_uid, tty);
	krbtkfile_env = tkt_location;

	principal = (char *)malloc(strlen(pw->pw_name)+strlen(instance)+2);
	strcpy(principal, pw->pw_name);	/* XXX strcpy is safe */
	if (strlen(instance)) {
	    strcat(principal, "/");		/* XXX strcat is safe */
	    strcat(principal, instance);	/* XXX strcat is safe */
	}
	
	if ((kerror = krb5_cc_resolve(kcontext, tkt_location, &ccache)) != 0) {
	    syslog(LOG_NOTICE, "warning: %s while getting default ccache",
		error_message(kerror));
	    return(1);
	}

	if ((kerror = krb5_parse_name(kcontext, principal, &me)) != 0) {
	    syslog(LOG_NOTICE, "warning: %s when parsing name %s",
		error_message(kerror), principal);
	    return(1);
	}
    
	if ((kerror = krb5_unparse_name(kcontext, me, &client_name)) != 0) {
	    syslog(LOG_NOTICE, "warning: %s when unparsing name %s",
		error_message(kerror), principal);
	    return(1);
	}

	kerror = krb5_cc_initialize(kcontext, ccache, me);
	if (kerror != 0) {
	    syslog(LOG_NOTICE, "%s when initializing cache %s",
		error_message(kerror), tkt_location);
	    return(1);
	}

	memset((char *)&my_creds, 0, sizeof(my_creds));
    
	my_creds.client = me;

	if ((kerror = krb5_build_principal_ext(kcontext,
					&server,
					krb5_princ_realm(kcontext, me)->length,
					krb5_princ_realm(kcontext, me)->data,
					tgtname.length, tgtname.data,
					krb5_princ_realm(kcontext, me)->length,
					krb5_princ_realm(kcontext, me)->data,
					0)) != 0) {
	    syslog(LOG_NOTICE, "%s while building server name",
		error_message(kerror));
	    return(1);
	}

	my_creds.server = server;

	kerror = krb5_os_localaddr(kcontext, &my_addresses);
	if (kerror != 0) {
	    syslog(LOG_NOTICE, "%s when getting my address",
		error_message(kerror));
	    return(1);
	}

	if ((kerror = krb5_timeofday(kcontext, &now)) != 0) {
	    syslog(LOG_NOTICE, "%s while getting time of day",
		error_message(kerror));
	    return(1);
	}
	my_creds.times.starttime = 0;	/* start timer when request
					   gets to KDC */
	my_creds.times.endtime = now + lifetime;
	my_creds.times.renew_till = 0;

	kerror = krb5_get_in_tkt_with_password(kcontext, options,
					       my_addresses,
					       NULL,
					       NULL,
					       password,
					       ccache,
					       &my_creds, 0);

	krb5_free_principal(kcontext, server);
	krb5_free_addresses(kcontext, my_addresses);

	if (chown(&tkt_location[5], pw->pw_uid, pw->pw_gid) < 0)
		syslog(LOG_ERR, "chown tkfile (%s): %m", &tkt_location[5]);

	if (kerror) {
	    if (kerror == KRB5KRB_AP_ERR_BAD_INTEGRITY)
		printf("%s: Kerberos Password incorrect\n", principal);
	    else
		printf("%s while getting initial credentials\n", error_message(kerror));

	    return(1);
	}

	/* Success */
	notickets = 0;
	return(0);
}

/*
 * Remove any credentials
 */
void
kdestroy()
{
        krb5_error_code code;
	krb5_ccache ccache = NULL;

	if (! use_krb5 || krbtkfile_env == NULL)
	    return;

	code = krb5_cc_resolve(kcontext, krbtkfile_env, &ccache);
	if (!code) {
	    code = krb5_cc_destroy(kcontext, ccache);
	    if (!code) {
		krb5_cc_close(kcontext, ccache);
	    }
	}
}
#endif

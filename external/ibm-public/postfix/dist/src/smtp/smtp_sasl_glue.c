/*	$NetBSD: smtp_sasl_glue.c,v 1.1.1.1.2.2 2009/09/15 06:03:33 snj Exp $	*/

/*++
/* NAME
/*	smtp_sasl_glue 3
/* SUMMARY
/*	Postfix SASL interface for SMTP client
/* SYNOPSIS
/*	#include smtp_sasl.h
/*
/*	void	smtp_sasl_initialize()
/*
/*	void	smtp_sasl_connect(session)
/*	SMTP_SESSION *session;
/*
/*	void	smtp_sasl_start(session, sasl_opts_name, sasl_opts_val)
/*	SMTP_SESSION *session;
/*
/*	int     smtp_sasl_passwd_lookup(session)
/*	SMTP_SESSION *session;
/*
/*	int	smtp_sasl_authenticate(session, why)
/*	SMTP_SESSION *session;
/*	DSN_BUF *why;
/*
/*	void	smtp_sasl_cleanup(session)
/*	SMTP_SESSION *session;
/*
/*	void	smtp_sasl_passivate(session, buf)
/*	SMTP_SESSION *session;
/*	VSTRING	*buf;
/*
/*	int	smtp_sasl_activate(session, buf)
/*	SMTP_SESSION *session;
/*	char	*buf;
/* DESCRIPTION
/*	smtp_sasl_initialize() initializes the SASL library. This
/*	routine must be called once at process startup, before any
/*	chroot operations.
/*
/*	smtp_sasl_connect() performs per-session initialization. This
/*	routine must be called once at the start of each connection.
/*
/*	smtp_sasl_start() performs per-session initialization. This
/*	routine must be called once per session before doing any SASL
/*	authentication. The sasl_opts_name and sasl_opts_val parameters are
/*	the postfix configuration parameters setting the security
/*	policy of the SASL authentication.
/*
/*	smtp_sasl_passwd_lookup() looks up the username/password
/*	for the current SMTP server. The result is zero in case
/*	of failure.
/*
/*	smtp_sasl_authenticate() implements the SASL authentication
/*	dialog. The result is < 0 in case of protocol failure, zero in
/*	case of unsuccessful authentication, > 0 in case of success.
/*	The why argument is updated with a reason for failure.
/*	This routine must be called only when smtp_sasl_passwd_lookup()
/*	succeeds.
/*
/*	smtp_sasl_cleanup() cleans up. It must be called at the
/*	end of every SMTP session that uses SASL authentication.
/*	This routine is a noop for non-SASL sessions.
/*
/*	smtp_sasl_passivate() appends flattened SASL attributes to the
/*	specified buffer. The SASL attributes are not destroyed.
/*
/*	smtp_sasl_activate() restores SASL attributes from the
/*	specified buffer. The buffer is modified. A result < 0
/*	means there was an error.
/*
/*	Arguments:
/* .IP session
/*	Session context.
/* .IP mech_list
/*	String of SASL mechanisms (separated by blanks)
/* DIAGNOSTICS
/*	All errors are fatal.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Original author:
/*	Till Franke
/*	SuSE Rhein/Main AG
/*	65760 Eschborn, Germany
/*
/*	Adopted by:
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <stdlib.h>
#include <string.h>

 /*
  * Utility library
  */
#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <split_at.h>

 /*
  * Global library
  */
#include <mail_params.h>
#include <string_list.h>
#include <maps.h>
#include <mail_addr_find.h>

 /*
  * XSASL library.
  */
#include <xsasl.h>

 /*
  * Application-specific
  */
#include "smtp.h"
#include "smtp_sasl.h"
#include "smtp_sasl_auth_cache.h"

#ifdef USE_SASL_AUTH

 /*
  * Per-host login/password information.
  */
static MAPS *smtp_sasl_passwd_map;

 /*
  * Supported SASL mechanisms.
  */
STRING_LIST *smtp_sasl_mechs;

 /*
  * SASL implementation handle.
  */
static XSASL_CLIENT_IMPL *smtp_sasl_impl;

 /*
  * The 535 SASL authentication failure cache.
  */
#ifdef HAVE_SASL_AUTH_CACHE
static SMTP_SASL_AUTH_CACHE *smtp_sasl_auth_cache;

#endif

/* smtp_sasl_passwd_lookup - password lookup routine */

int     smtp_sasl_passwd_lookup(SMTP_SESSION *session)
{
    const char *myname = "smtp_sasl_passwd_lookup";
    SMTP_STATE *state = session->state;
    const char *value;
    char   *passwd;

    /*
     * Sanity check.
     */
    if (smtp_sasl_passwd_map == 0)
	msg_panic("%s: passwd map not initialized", myname);

    /*
     * Look up the per-server password information. Try the hostname first,
     * then try the destination.
     * 
     * XXX Instead of using nexthop (the intended destination) we use dest
     * (either the intended destination, or a fall-back destination).
     * 
     * XXX SASL authentication currently depends on the host/domain but not on
     * the TCP port. If the port is not :25, we should append it to the table
     * lookup key. Code for this was briefly introduced into 2.2 snapshots,
     * but didn't canonicalize the TCP port, and did not append the port to
     * the MX hostname.
     */
    if (((state->misc_flags & SMTP_MISC_FLAG_USE_LMTP) == 0
	 && var_smtp_sender_auth && state->request->sender[0]
	 && (value = mail_addr_find(smtp_sasl_passwd_map,
				 state->request->sender, (char **) 0)) != 0)
	|| (value = maps_find(smtp_sasl_passwd_map, session->host, 0)) != 0
      || (value = maps_find(smtp_sasl_passwd_map, session->dest, 0)) != 0) {
	if (session->sasl_username)
	    myfree(session->sasl_username);
	session->sasl_username = mystrdup(value);
	passwd = split_at(session->sasl_username, ':');
	if (session->sasl_passwd)
	    myfree(session->sasl_passwd);
	session->sasl_passwd = mystrdup(passwd ? passwd : "");
	if (msg_verbose)
	    msg_info("%s: host `%s' user `%s' pass `%s'",
		     myname, session->host,
		     session->sasl_username, session->sasl_passwd);
	return (1);
    } else {
	if (msg_verbose)
	    msg_info("%s: no auth info found (sender=`%s', host=`%s')",
		     myname, state->request->sender, session->host);
	return (0);
    }
}

/* smtp_sasl_initialize - per-process initialization (pre jail) */

void    smtp_sasl_initialize(void)
{

    /*
     * Sanity check.
     */
    if (smtp_sasl_passwd_map || smtp_sasl_impl)
	msg_panic("smtp_sasl_initialize: repeated call");
    if (*var_smtp_sasl_passwd == 0)
	msg_fatal("specify a password table via the `%s' configuration parameter",
		  VAR_SMTP_SASL_PASSWD);

    /*
     * Open the per-host password table and initialize the SASL library. Use
     * shared locks for reading, just in case someone updates the table.
     */
    smtp_sasl_passwd_map = maps_create("smtp_sasl_passwd",
				       var_smtp_sasl_passwd,
				       DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX);
    if ((smtp_sasl_impl = xsasl_client_init(var_smtp_sasl_type,
					    var_smtp_sasl_path)) == 0)
	msg_fatal("SASL library initialization");

    /*
     * Initialize optional supported mechanism matchlist
     */
    if (*var_smtp_sasl_mechs)
	smtp_sasl_mechs = string_list_init(MATCH_FLAG_NONE,
					   var_smtp_sasl_mechs);

    /*
     * Initialize the 535 SASL authentication failure cache.
     */
    if (*var_smtp_sasl_auth_cache_name) {
#ifdef HAVE_SASL_AUTH_CACHE
	smtp_sasl_auth_cache =
	    smtp_sasl_auth_cache_init(var_smtp_sasl_auth_cache_name,
				      var_smtp_sasl_auth_cache_time);
#else
	msg_warn("not compiled with TLS support -- "
		 "ignoring the " VAR_SMTP_SASL_AUTH_CACHE_NAME " setting");
#endif
    }
}

/* smtp_sasl_connect - per-session client initialization */

void    smtp_sasl_connect(SMTP_SESSION *session)
{

    /*
     * This initialization happens whenever we instantiate an SMTP session
     * object. We don't instantiate a SASL client until we actually need one.
     */
    session->sasl_mechanism_list = 0;
    session->sasl_username = 0;
    session->sasl_passwd = 0;
    session->sasl_client = 0;
    session->sasl_reply = 0;
}

/* smtp_sasl_start - per-session SASL initialization */

void    smtp_sasl_start(SMTP_SESSION *session, const char *sasl_opts_name,
			        const char *sasl_opts_val)
{
    XSASL_CLIENT_CREATE_ARGS create_args;

    if (msg_verbose)
	msg_info("starting new SASL client");
    if ((session->sasl_client =
	 XSASL_CLIENT_CREATE(smtp_sasl_impl, &create_args,
			     stream = session->stream,
			     service = var_procname,
			     server_name = session->host,
			     security_options = sasl_opts_val)) == 0)
	msg_fatal("SASL per-connection initialization failed");
    session->sasl_reply = vstring_alloc(20);
}

/* smtp_sasl_authenticate - run authentication protocol */

int     smtp_sasl_authenticate(SMTP_SESSION *session, DSN_BUF *why)
{
    const char *myname = "smtp_sasl_authenticate";
    SMTP_RESP *resp;
    const char *mechanism;
    int     result;
    char   *line;
    int     steps = 0;

    /*
     * Sanity check.
     */
    if (session->sasl_mechanism_list == 0)
	msg_panic("%s: no mechanism list", myname);

    if (msg_verbose)
	msg_info("%s: %s: SASL mechanisms %s",
		 myname, session->namaddrport, session->sasl_mechanism_list);

    /*
     * Avoid repeated login failures after a recent 535 error.
     */
#ifdef HAVE_SASL_AUTH_CACHE
    if (smtp_sasl_auth_cache
	&& smtp_sasl_auth_cache_find(smtp_sasl_auth_cache, session)) {
	char   *resp_dsn = smtp_sasl_auth_cache_dsn(smtp_sasl_auth_cache);
	char   *resp_str = smtp_sasl_auth_cache_text(smtp_sasl_auth_cache);

	if (var_smtp_sasl_auth_soft_bounce && resp_dsn[0] == '5')
	    resp_dsn[0] = '4';
	dsb_update(why, resp_dsn, DSB_DEF_ACTION, DSB_MTYPE_DNS,
		   session->host, var_procname, resp_str,
		   "SASL [CACHED] authentication failed; server %s said: %s",
		   session->host, resp_str);
	return (0);
    }
#endif

    /*
     * Start the client side authentication protocol.
     */
    result = xsasl_client_first(session->sasl_client,
				session->sasl_mechanism_list,
				session->sasl_username,
				session->sasl_passwd,
				&mechanism, session->sasl_reply);
    if (result != XSASL_AUTH_OK) {
	dsb_update(why, "4.7.0", DSB_DEF_ACTION, DSB_SKIP_RMTA,
		   DSB_DTYPE_SASL, STR(session->sasl_reply),
		   "SASL authentication failed; "
		   "cannot authenticate to server %s: %s",
		   session->namaddr, STR(session->sasl_reply));
	return (-1);
    }

    /*
     * Send the AUTH command and the optional initial client response.
     * sasl_encode64() produces four bytes for each complete or incomplete
     * triple of input bytes. Allocate an extra byte for string termination.
     */
    if (LEN(session->sasl_reply) > 0) {
	smtp_chat_cmd(session, "AUTH %s %s", mechanism,
		      STR(session->sasl_reply));
    } else {
	smtp_chat_cmd(session, "AUTH %s", mechanism);
    }

    /*
     * Step through the authentication protocol until the server tells us
     * that we are done.
     */
    while ((resp = smtp_chat_resp(session))->code / 100 == 3) {

	/*
	 * Sanity check.
	 */
	if (++steps > 100) {
	    dsb_simple(why, "4.3.0", "SASL authentication failed; "
		       "authentication protocol loop with server %s",
		       session->namaddr);
	    return (-1);
	}

	/*
	 * Process a server challenge.
	 */
	line = resp->str;
	(void) mystrtok(&line, "- \t\n");	/* skip over result code */
	result = xsasl_client_next(session->sasl_client, line,
				   session->sasl_reply);
	if (result != XSASL_AUTH_OK) {
	    dsb_update(why, "4.7.0", DSB_DEF_ACTION,	/* Fix 200512 */
		    DSB_SKIP_RMTA, DSB_DTYPE_SASL, STR(session->sasl_reply),
		       "SASL authentication failed; "
		       "cannot authenticate to server %s: %s",
		       session->namaddr, STR(session->sasl_reply));
	    return (-1);			/* Fix 200512 */
	}

	/*
	 * Send a client response.
	 */
	smtp_chat_cmd(session, "%s", STR(session->sasl_reply));
    }

    /*
     * We completed the authentication protocol.
     */
    if (resp->code / 100 != 2) {
#ifdef HAVE_SASL_AUTH_CACHE
	/* Update the 535 authentication failure cache. */
	if (smtp_sasl_auth_cache && resp->code == 535)
	    smtp_sasl_auth_cache_store(smtp_sasl_auth_cache, session, resp);
#endif
	if (var_smtp_sasl_auth_soft_bounce && resp->code / 100 == 5)
	    STR(resp->dsn_buf)[0] = '4';
	dsb_update(why, resp->dsn, DSB_DEF_ACTION,
		   DSB_MTYPE_DNS, session->host,
		   var_procname, resp->str,
		   "SASL authentication failed; server %s said: %s",
		   session->namaddr, resp->str);
	return (0);
    }
    return (1);
}

/* smtp_sasl_cleanup - per-session cleanup */

void    smtp_sasl_cleanup(SMTP_SESSION *session)
{
    if (session->sasl_username) {
	myfree(session->sasl_username);
	session->sasl_username = 0;
    }
    if (session->sasl_passwd) {
	myfree(session->sasl_passwd);
	session->sasl_passwd = 0;
    }
    if (session->sasl_mechanism_list) {
	/* allocated in smtp_sasl_helo_auth */
	myfree(session->sasl_mechanism_list);
	session->sasl_mechanism_list = 0;
    }
    if (session->sasl_client) {
	if (msg_verbose)
	    msg_info("disposing SASL state information");
	xsasl_client_free(session->sasl_client);
	session->sasl_client = 0;
    }
    if (session->sasl_reply) {
	vstring_free(session->sasl_reply);
	session->sasl_reply = 0;
    }
}

/* smtp_sasl_passivate - append serialized SASL attributes */

void    smtp_sasl_passivate(SMTP_SESSION *session, VSTRING *buf)
{
}

/* smtp_sasl_activate - de-serialize SASL attributes */

int     smtp_sasl_activate(SMTP_SESSION *session, char *buf)
{
    return (0);
}

#endif

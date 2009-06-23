/*	$NetBSD: xsasl_cyrus_server.c,v 1.1.1.1 2009/06/23 10:09:02 tron Exp $	*/

/*++
/* NAME
/*	xsasl_cyrus_server 3
/* SUMMARY
/*	Cyrus SASL server-side plug-in
/* SYNOPSIS
/*	#include <xsasl_cyrus_server.h>
/*
/*	XSASL_SERVER_IMPL *xsasl_cyrus_server_init(server_type, path_info)
/*	const char *server_type;
/*	const char *path_info;
/* DESCRIPTION
/*	This module implements the Cyrus SASL server-side authentication
/*	plug-in.
/*
/*	xsasl_cyrus_server_init() initializes the Cyrus SASL library and
/*	returns an implementation handle that can be used to generate
/*	SASL server instances.
/*
/*	Arguments:
/* .IP server_type
/*	The server type (cyrus). This argument is ignored, but it
/*	could be used when one implementation provides multiple
/*	variants.
/* .IP path_info
/*	The base name of the SASL server configuration file (example:
/*	smtpd becomes /usr/lib/sasl2/smtpd.conf).
/* DIAGNOSTICS
/*	Fatal: out of memory.
/*
/*	Panic: interface violation.
/*
/*	Other: the routines log a warning and return an error result
/*	as specified in xsasl_server(3).
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Initial implementation by:
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

/* System library. */

#include <sys_defs.h>
#include <stdlib.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <name_mask.h>
#include <stringops.h>

/* Global library. */

#include <mail_params.h>

/* Application-specific. */

#include <xsasl.h>
#include <xsasl_cyrus.h>
#include <xsasl_cyrus_common.h>

#if defined(USE_SASL_AUTH) && defined(USE_CYRUS_SASL)

#include <sasl.h>
#include <saslutil.h>

/*
 * Silly little macros.
 */
#define STR(s)	vstring_str(s)

 /*
  * Macros to handle API differences between SASLv1 and SASLv2. Specifics:
  * 
  * The SASL_LOG_* constants were renamed in SASLv2.
  * 
  * SASLv2's sasl_server_new takes two new parameters to specify local and
  * remote IP addresses for auth mechs that use them.
  * 
  * SASLv2's sasl_server_start and sasl_server_step no longer have the errstr
  * parameter.
  * 
  * SASLv2's sasl_decode64 function takes an extra parameter for the length of
  * the output buffer.
  * 
  * The other major change is that SASLv2 now takes more responsibility for
  * deallocating memory that it allocates internally.  Thus, some of the
  * function parameters are now 'const', to make sure we don't try to free
  * them too.  This is dealt with in the code later on.
  */

#if SASL_VERSION_MAJOR < 2
/* SASL version 1.x */
#define SASL_SERVER_NEW(srv, fqdn, rlm, lport, rport, cb, secflags, pconn) \
	sasl_server_new(srv, fqdn, rlm, cb, secflags, pconn)
#define SASL_SERVER_START(conn, mech, clin, clinlen, srvout, srvoutlen, err) \
	sasl_server_start(conn, mech, clin, clinlen, srvout, srvoutlen, err)
#define SASL_SERVER_STEP(conn, clin, clinlen, srvout, srvoutlen, err) \
	sasl_server_step(conn, clin, clinlen, srvout, srvoutlen, err)
#define SASL_DECODE64(in, inlen, out, outmaxlen, outlen) \
	sasl_decode64(in, inlen, out, outlen)
typedef char *MECHANISM_TYPE;
typedef unsigned MECHANISM_COUNT_TYPE;
typedef char *SERVEROUT_TYPE;
typedef void *VOID_SERVEROUT_TYPE;

#endif

#if SASL_VERSION_MAJOR >= 2
/* SASL version > 2.x */
#define SASL_SERVER_NEW(srv, fqdn, rlm, lport, rport, cb, secflags, pconn) \
	sasl_server_new(srv, fqdn, rlm, lport, rport, cb, secflags, pconn)
#define SASL_SERVER_START(conn, mech, clin, clinlen, srvout, srvoutlen, err) \
	sasl_server_start(conn, mech, clin, clinlen, srvout, srvoutlen)
#define SASL_SERVER_STEP(conn, clin, clinlen, srvout, srvoutlen, err) \
	sasl_server_step(conn, clin, clinlen, srvout, srvoutlen)
#define SASL_DECODE64(in, inlen, out, outmaxlen, outlen) \
	sasl_decode64(in, inlen, out, outmaxlen, outlen)
typedef const char *MECHANISM_TYPE;
typedef int MECHANISM_COUNT_TYPE;
typedef const char *SERVEROUT_TYPE;
typedef const void *VOID_SERVEROUT_TYPE;

#endif

 /*
  * The XSASL_CYRUS_SERVER object is derived from the generic XSASL_SERVER
  * object.
  */
typedef struct {
    XSASL_SERVER xsasl;			/* generic members, must be first */
    VSTREAM *stream;			/* client-server connection */
    sasl_conn_t *sasl_conn;		/* SASL context */
    VSTRING *decoded;			/* decoded challenge or response */
    char   *username;			/* authenticated user */
    char   *mechanism_list;		/* applicable mechanisms */
} XSASL_CYRUS_SERVER;

 /*
  * Forward declarations.
  */
static void xsasl_cyrus_server_done(XSASL_SERVER_IMPL *);
static XSASL_SERVER *xsasl_cyrus_server_create(XSASL_SERVER_IMPL *,
					        XSASL_SERVER_CREATE_ARGS *);
static void xsasl_cyrus_server_free(XSASL_SERVER *);
static int xsasl_cyrus_server_first(XSASL_SERVER *, const char *,
				            const char *, VSTRING *);
static int xsasl_cyrus_server_next(XSASL_SERVER *, const char *, VSTRING *);
static int xsasl_cyrus_server_set_security(XSASL_SERVER *, const char *);
static const char *xsasl_cyrus_server_get_mechanism_list(XSASL_SERVER *);
static const char *xsasl_cyrus_server_get_username(XSASL_SERVER *);

 /*
  * SASL callback interface structure. These call-backs have no per-session
  * context.
  */
#define NO_CALLBACK_CONTEXT	0

static sasl_callback_t callbacks[] = {
    {SASL_CB_LOG, &xsasl_cyrus_log, NO_CALLBACK_CONTEXT},
    {SASL_CB_LIST_END, 0, 0}
};

/* xsasl_cyrus_server_init - create implementation handle */

XSASL_SERVER_IMPL *xsasl_cyrus_server_init(const char *unused_server_type,
					           const char *path_info)
{
    const char *myname = "xsasl_cyrus_server_init";
    XSASL_SERVER_IMPL *xp;
    int     sasl_status;

#if SASL_VERSION_MAJOR >= 2 && (SASL_VERSION_MINOR >= 2 \
    || (SASL_VERSION_MINOR == 1 && SASL_VERSION_STEP >= 19))
    int     sasl_major;
    int     sasl_minor;
    int     sasl_step;

    /*
     * DLL hell guard.
     */
    sasl_version_info((const char **) 0, (const char **) 0,
		      &sasl_major, &sasl_minor,
		      &sasl_step, (int *) 0);
    if (sasl_major != SASL_VERSION_MAJOR
#if 0
	|| sasl_minor != SASL_VERSION_MINOR
	|| sasl_step != SASL_VERSION_STEP
#endif
	) {
	msg_warn("incorrect SASL library version. "
	      "Postfix was built with include files from version %d.%d.%d, "
		 "but the run-time library version is %d.%d.%d",
		 SASL_VERSION_MAJOR, SASL_VERSION_MINOR, SASL_VERSION_STEP,
		 sasl_major, sasl_minor, sasl_step);
	return (0);
    }
#endif

    if (*var_cyrus_conf_path) {
#ifdef SASL_PATH_TYPE_CONFIG			/* Cyrus SASL 2.1.22 */
	if (sasl_set_path(SASL_PATH_TYPE_CONFIG,
			  var_cyrus_conf_path) != SASL_OK)
	    msg_warn("failed to set Cyrus SASL configuration path: \"%s\"",
		     var_cyrus_conf_path);
#else
	msg_warn("%s is not empty, but setting the Cyrus SASL configuration "
		 "path is not supported with SASL library version %d.%d.%d",
		 VAR_CYRUS_CONF_PATH, SASL_VERSION_MAJOR,
		 SASL_VERSION_MINOR, SASL_VERSION_STEP);
#endif
    }

    /*
     * Initialize the library: load SASL plug-in routines, etc.
     */
    if (msg_verbose)
	msg_info("%s: SASL config file is %s.conf", myname, path_info);
    if ((sasl_status = sasl_server_init(callbacks, path_info)) != SASL_OK) {
	msg_warn("SASL per-process initialization failed: %s",
		 xsasl_cyrus_strerror(sasl_status));
	return (0);
    }

    /*
     * Return a generic XSASL_SERVER_IMPL object. We don't need to extend it
     * with our own methods or data.
     */
    xp = (XSASL_SERVER_IMPL *) mymalloc(sizeof(*xp));
    xp->create = xsasl_cyrus_server_create;
    xp->done = xsasl_cyrus_server_done;
    return (xp);
}

/* xsasl_cyrus_server_done - dispose of implementation */

static void xsasl_cyrus_server_done(XSASL_SERVER_IMPL *impl)
{
    myfree((char *) impl);
    sasl_done();
}

/* xsasl_cyrus_server_create - create server instance */

static XSASL_SERVER *xsasl_cyrus_server_create(XSASL_SERVER_IMPL *unused_impl,
				             XSASL_SERVER_CREATE_ARGS *args)
{
    const char *myname = "xsasl_cyrus_server_create";
    char   *server_address;
    char   *client_address;
    sasl_conn_t *sasl_conn = 0;
    XSASL_CYRUS_SERVER *server = 0;
    int     sasl_status;

    if (msg_verbose)
	msg_info("%s: SASL service=%s, realm=%s",
		 myname, args->service, args->user_realm ?
		 args->user_realm : "(null)");

    /*
     * The optimizer will eliminate code duplication and/or dead code.
     */
#define XSASL_CYRUS_SERVER_CREATE_ERROR_RETURN(x) \
    do { \
	if (server) { \
	    xsasl_cyrus_server_free(&server->xsasl); \
	} else { \
	    if (sasl_conn) \
		sasl_dispose(&sasl_conn); \
	} \
	return (x); \
    } while (0)

    /*
     * Set up a new server context.
     */
#define NO_SECURITY_LAYERS	(0)
#define NO_SESSION_CALLBACKS	((sasl_callback_t *) 0)
#define NO_AUTH_REALM		((char *) 0)

#if SASL_VERSION_MAJOR >= 2 && defined(USE_SASL_IP_AUTH)

    /*
     * Get IP addresses of local and remote endpoints for SASL.
     */
#error "USE_SASL_IP_AUTH is not implemented"

#else

    /*
     * Don't give any IP address information to SASL.  SASLv1 doesn't use it,
     * and in SASLv2 this will disable any mechanisms that do.
     */
    server_address = 0;
    client_address = 0;
#endif

    if ((sasl_status =
	 SASL_SERVER_NEW(args->service, var_myhostname,
			 args->user_realm ? args->user_realm : NO_AUTH_REALM,
			 server_address, client_address,
			 NO_SESSION_CALLBACKS, NO_SECURITY_LAYERS,
			 &sasl_conn)) != SASL_OK) {
	msg_warn("SASL per-connection server initialization: %s",
		 xsasl_cyrus_strerror(sasl_status));
	XSASL_CYRUS_SERVER_CREATE_ERROR_RETURN(0);
    }

    /*
     * Extend the XSASL_SERVER object with our own data. We use long-lived
     * conversion buffers rather than local variables to avoid memory leaks
     * in case of read/write timeout or I/O error.
     */
    server = (XSASL_CYRUS_SERVER *) mymalloc(sizeof(*server));
    server->xsasl.free = xsasl_cyrus_server_free;
    server->xsasl.first = xsasl_cyrus_server_first;
    server->xsasl.next = xsasl_cyrus_server_next;
    server->xsasl.get_mechanism_list = xsasl_cyrus_server_get_mechanism_list;
    server->xsasl.get_username = xsasl_cyrus_server_get_username;
    server->stream = args->stream;
    server->sasl_conn = sasl_conn;
    server->decoded = vstring_alloc(20);
    server->username = 0;
    server->mechanism_list = 0;

    if (xsasl_cyrus_server_set_security(&server->xsasl, args->security_options)
	!= XSASL_AUTH_OK)
	XSASL_CYRUS_SERVER_CREATE_ERROR_RETURN(0);

    return (&server->xsasl);
}

/* xsasl_cyrus_server_set_security - set security properties */

static int xsasl_cyrus_server_set_security(XSASL_SERVER *xp,
					           const char *sasl_opts_val)
{
    XSASL_CYRUS_SERVER *server = (XSASL_CYRUS_SERVER *) xp;
    sasl_security_properties_t sec_props;
    int     sasl_status;

    /*
     * Security options. Some information can be found in the sasl.h include
     * file.
     */
    memset(&sec_props, 0, sizeof(sec_props));
    sec_props.min_ssf = 0;
    sec_props.max_ssf = 0;			/* don't allow real SASL
						 * security layer */
    if (*sasl_opts_val == 0) {
	sec_props.security_flags = 0;
    } else {
	sec_props.security_flags =
	    xsasl_cyrus_security_parse_opts(sasl_opts_val);
	if (sec_props.security_flags == 0) {
	    msg_warn("bad per-session SASL security properties");
	    return (XSASL_AUTH_FAIL);
	}
    }
    sec_props.maxbufsize = 0;
    sec_props.property_names = 0;
    sec_props.property_values = 0;

    if ((sasl_status = sasl_setprop(server->sasl_conn, SASL_SEC_PROPS,
				    &sec_props)) != SASL_OK) {
	msg_warn("SASL per-connection security setup; %s",
		 xsasl_cyrus_strerror(sasl_status));
	return (XSASL_AUTH_FAIL);
    }
    return (XSASL_AUTH_OK);
}

/* xsasl_cyrus_server_get_mechanism_list - get available mechanisms */

static const char *xsasl_cyrus_server_get_mechanism_list(XSASL_SERVER *xp)
{
    const char *myname = "xsasl_cyrus_server_get_mechanism_list";
    XSASL_CYRUS_SERVER *server = (XSASL_CYRUS_SERVER *) xp;
    MECHANISM_TYPE mechanism_list;
    MECHANISM_COUNT_TYPE mechanism_count;
    int     sasl_status;

    /*
     * Get the list of authentication mechanisms.
     */
#define UNSUPPORTED_USER	((char *) 0)
#define IGNORE_MECHANISM_LEN	((unsigned *) 0)

    if ((sasl_status = sasl_listmech(server->sasl_conn, UNSUPPORTED_USER,
				     "", " ", "",
				     &mechanism_list,
				     IGNORE_MECHANISM_LEN,
				     &mechanism_count)) != SASL_OK) {
	msg_warn("%s: %s", myname, xsasl_cyrus_strerror(sasl_status));
	return (0);
    }
    if (mechanism_count <= 0) {
	msg_warn("%s: no applicable SASL mechanisms", myname);
	return (0);
    }
    server->mechanism_list = mystrdup(mechanism_list);
#if SASL_VERSION_MAJOR < 2
    /* SASL version 1 doesn't free memory that it allocates. */
    free(mechanism_list);
#endif
    return (server->mechanism_list);
}

/* xsasl_cyrus_server_free - destroy server instance */

static void xsasl_cyrus_server_free(XSASL_SERVER *xp)
{
    XSASL_CYRUS_SERVER *server = (XSASL_CYRUS_SERVER *) xp;

    sasl_dispose(&server->sasl_conn);
    vstring_free(server->decoded);
    if (server->username)
	myfree(server->username);
    if (server->mechanism_list)
	myfree(server->mechanism_list);
    myfree((char *) server);
}

/* xsasl_cyrus_server_auth_response - encode server first/next response */

static int xsasl_cyrus_server_auth_response(int sasl_status,
					            SERVEROUT_TYPE serverout,
					            unsigned serveroutlen,
					            VSTRING *reply)
{
    const char *myname = "xsasl_cyrus_server_auth_response";
    unsigned enc_length;
    unsigned enc_length_out;

    /*
     * Encode the server first/next non-error response; otherwise return the
     * unencoded error text that corresponds to the SASL error status.
     * 
     * Regarding the hairy expression below: output from sasl_encode64() comes
     * in multiples of four bytes for each triple of input bytes, plus four
     * bytes for any incomplete last triple, plus one byte for the null
     * terminator.
     */
    if (sasl_status == SASL_OK) {
	vstring_strcpy(reply, "");
	return (XSASL_AUTH_DONE);
    } else if (sasl_status == SASL_CONTINUE) {
	if (msg_verbose)
	    msg_info("%s: uncoded server challenge: %.*s",
		     myname, (int) serveroutlen, serverout);
	enc_length = ((serveroutlen + 2) / 3) * 4 + 1;
	VSTRING_RESET(reply);			/* Fix 200512 */
	VSTRING_SPACE(reply, enc_length);
	if ((sasl_status = sasl_encode64(serverout, serveroutlen,
					 STR(reply), vstring_avail(reply),
					 &enc_length_out)) != SASL_OK)
	    msg_panic("%s: sasl_encode64 botch: %s",
		      myname, xsasl_cyrus_strerror(sasl_status));
	return (XSASL_AUTH_MORE);
    } else {
	if (sasl_status == SASL_NOUSER)		/* privacy */
	    sasl_status = SASL_BADAUTH;
	vstring_strcpy(reply, xsasl_cyrus_strerror(sasl_status));
	return (XSASL_AUTH_FAIL);
    }
}

/* xsasl_cyrus_server_first - per-session authentication */

int     xsasl_cyrus_server_first(XSASL_SERVER *xp, const char *sasl_method,
			          const char *init_response, VSTRING *reply)
{
    const char *myname = "xsasl_cyrus_server_first";
    XSASL_CYRUS_SERVER *server = (XSASL_CYRUS_SERVER *) xp;
    char   *dec_buffer;
    unsigned dec_length;
    unsigned reply_len;
    unsigned serveroutlen;
    int     sasl_status;
    SERVEROUT_TYPE serverout = 0;
    int     xsasl_status;

#if SASL_VERSION_MAJOR < 2
    const char *errstr = 0;

#endif

#define IFELSE(e1,e2,e3) ((e1) ? (e2) : (e3))

    if (msg_verbose)
	msg_info("%s: sasl_method %s%s%s", myname, sasl_method,
		 IFELSE(init_response, ", init_response ", ""),
		 IFELSE(init_response, init_response, ""));

    /*
     * SASL authentication protocol start-up. Process any initial client
     * response that was sent along in the AUTH command.
     */
    if (init_response) {
	reply_len = strlen(init_response);
	VSTRING_RESET(server->decoded);		/* Fix 200512 */
	VSTRING_SPACE(server->decoded, reply_len);
	if ((sasl_status = SASL_DECODE64(init_response, reply_len,
					 dec_buffer = STR(server->decoded),
					 vstring_avail(server->decoded),
					 &dec_length)) != SASL_OK) {
	    vstring_strcpy(reply, xsasl_cyrus_strerror(sasl_status));
	    return (XSASL_AUTH_FORM);
	}
	if (msg_verbose)
	    msg_info("%s: decoded initial response %s", myname, dec_buffer);
    } else {
	dec_buffer = 0;
	dec_length = 0;
    }
    sasl_status = SASL_SERVER_START(server->sasl_conn, sasl_method, dec_buffer,
				    dec_length, &serverout,
				    &serveroutlen, &errstr);
    xsasl_status = xsasl_cyrus_server_auth_response(sasl_status, serverout,
						    serveroutlen, reply);
#if SASL_VERSION_MAJOR < 2
    /* SASL version 1 doesn't free memory that it allocates. */
    free(serverout);
#endif
    return (xsasl_status);
}

/* xsasl_cyrus_server_next - continue authentication */

static int xsasl_cyrus_server_next(XSASL_SERVER *xp, const char *request,
				           VSTRING *reply)
{
    const char *myname = "xsasl_cyrus_server_next";
    XSASL_CYRUS_SERVER *server = (XSASL_CYRUS_SERVER *) xp;
    unsigned dec_length;
    unsigned request_len;
    unsigned serveroutlen;
    int     sasl_status;
    SERVEROUT_TYPE serverout = 0;
    int     xsasl_status;

#if SASL_VERSION_MAJOR < 2
    const char *errstr = 0;

#endif

    request_len = strlen(request);
    VSTRING_RESET(server->decoded);		/* Fix 200512 */
    VSTRING_SPACE(server->decoded, request_len);
    if ((sasl_status = SASL_DECODE64(request, request_len,
				     STR(server->decoded),
				     vstring_avail(server->decoded),
				     &dec_length)) != SASL_OK) {
	vstring_strcpy(reply, xsasl_cyrus_strerror(sasl_status));
	return (XSASL_AUTH_FORM);
    }
    if (msg_verbose)
	msg_info("%s: decoded response: %.*s",
		 myname, (int) dec_length, STR(server->decoded));
    sasl_status = SASL_SERVER_STEP(server->sasl_conn, STR(server->decoded),
				   dec_length, &serverout,
				   &serveroutlen, &errstr);
    xsasl_status = xsasl_cyrus_server_auth_response(sasl_status, serverout,
						    serveroutlen, reply);
#if SASL_VERSION_MAJOR < 2
    /* SASL version 1 doesn't free memory that it allocates. */
    free(serverout);
#endif
    return (xsasl_status);
}

/* xsasl_cyrus_server_get_username - get authenticated username */

static const char *xsasl_cyrus_server_get_username(XSASL_SERVER *xp)
{
    const char *myname = "xsasl_cyrus_server_get_username";
    XSASL_CYRUS_SERVER *server = (XSASL_CYRUS_SERVER *) xp;
    VOID_SERVEROUT_TYPE serverout = 0;
    int     sasl_status;

    /*
     * XXX Do not free(serverout).
     */
    sasl_status = sasl_getprop(server->sasl_conn, SASL_USERNAME, &serverout);
    if (sasl_status != SASL_OK || serverout == 0) {
	msg_warn("%s: sasl_getprop SASL_USERNAME botch: %s",
		 myname, xsasl_cyrus_strerror(sasl_status));
	return (0);
    }
    if (server->username)
	myfree(server->username);
    server->username = mystrdup(serverout);
    printable(server->username, '?');
    return (server->username);
}

#endif

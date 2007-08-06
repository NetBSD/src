/*	$NetBSD: xsasl_cyrus_client.c,v 1.1.1.1.6.2 2007/08/06 11:06:30 ghen Exp $	*/

/*++
/* NAME
/*	xsasl_cyrus_client 3
/* SUMMARY
/*	Cyrus SASL client-side plug-in
/* SYNOPSIS
/*	#include <xsasl_cyrus_client.h>
/*
/*	XSASL_CLIENT_IMPL *xsasl_cyrus_client_init(client_type, path_info)
/*	const char *client_type;
/* DESCRIPTION
/*	This module implements the Cyrus SASL client-side authentication
/*	plug-in.
/*
/*	xsasl_cyrus_client_init() initializes the Cyrus SASL library and
/*	returns an implementation handle that can be used to generate
/*	SASL client instances.
/*
/*	Arguments:
/* .IP client_type
/*	The plug-in SASL client type (cyrus). This argument is
/*	ignored, but it could be used when one implementation
/*	provides multiple variants.
/* .IP path_info
/*	Implementation-specific information to specify the location
/*	of a configuration file, rendez-vous point, etc. This
/*	information is ignored by the Cyrus SASL client plug-in.
/* DIAGNOSTICS
/*	Fatal: out of memory.
/*
/*	Panic: interface violation.
/*
/*	Other: the routines log a warning and return an error result
/*	as specified in xsasl_client(3).
/* SEE ALSO
/*	xsasl_client(3) Client API
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

 /*
  * Global library
  */
#include <mail_params.h>

 /*
  * Application-specific
  */
#include <xsasl.h>
#include <xsasl_cyrus.h>
#include <xsasl_cyrus_common.h>

#if defined(USE_SASL_AUTH) && defined(USE_CYRUS_SASL)

#include <sasl.h>
#include <saslutil.h>

/*
 * Silly little macros.
 */
#define STR(s)  vstring_str(s)

 /*
  * Macros to handle API differences between SASLv1 and SASLv2. Specifics:
  * 
  * The SASL_LOG_* constants were renamed in SASLv2.
  * 
  * SASLv2's sasl_client_new takes two new parameters to specify local and
  * remote IP addresses for auth mechs that use them.
  * 
  * SASLv2's sasl_client_start function no longer takes the secret parameter.
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
#define SASL_CLIENT_NEW(srv, fqdn, lport, rport, prompt, secflags, pconn) \
	sasl_client_new(srv, fqdn, prompt, secflags, pconn)
#define SASL_CLIENT_START(conn, mechlst, secret, prompt, clout, cllen, mech) \
	sasl_client_start(conn, mechlst, secret, prompt, clout, cllen, mech)
#define SASL_DECODE64(in, inlen, out, outmaxlen, outlen) \
	sasl_decode64(in, inlen, out, outlen)
typedef char *CLIENTOUT_TYPE;

#endif

#if SASL_VERSION_MAJOR >= 2
/* SASL version > 2.x */
#define SASL_CLIENT_NEW(srv, fqdn, lport, rport, prompt, secflags, pconn) \
	sasl_client_new(srv, fqdn, lport, rport, prompt, secflags, pconn)
#define SASL_CLIENT_START(conn, mechlst, secret, prompt, clout, cllen, mech) \
	sasl_client_start(conn, mechlst, prompt, clout, cllen, mech)
#define SASL_DECODE64(in, inlen, out, outmaxlen, outlen) \
	sasl_decode64(in, inlen, out, outmaxlen, outlen)
typedef const char *CLIENTOUT_TYPE;

#endif

 /*
  * The XSASL_CYRUS_CLIENT object is derived from the generic XSASL_CLIENT
  * object.
  */
typedef struct {
    XSASL_CLIENT xsasl;			/* generic members, must be first */
    VSTREAM *stream;			/* client-server connection */
    sasl_conn_t *sasl_conn;		/* SASL context */
    VSTRING *decoded;			/* decoded server challenge */
    sasl_callback_t *callbacks;		/* user/password lookup */
    char   *username;
    char   *password;
} XSASL_CYRUS_CLIENT;

 /*
  * Forward declarations.
  */
static void xsasl_cyrus_client_done(XSASL_CLIENT_IMPL *);
static XSASL_CLIENT *xsasl_cyrus_client_create(XSASL_CLIENT_IMPL *,
					               VSTREAM *,
					               const char *,
					               const char *,
					               const char *);
static int xsasl_cyrus_client_set_security(XSASL_CLIENT *, const char *);
static int xsasl_cyrus_client_first(XSASL_CLIENT *, const char *, const char *,
			            const char *, const char **, VSTRING *);
static int xsasl_cyrus_client_next(XSASL_CLIENT *, const char *, VSTRING *);
static void xsasl_cyrus_client_free(XSASL_CLIENT *);

/* xsasl_cyrus_client_get_user - username lookup call-back routine */

static int xsasl_cyrus_client_get_user(void *context, int unused_id,
				               const char **result,
				               unsigned *len)
{
    const char *myname = "xsasl_cyrus_client_get_user";
    XSASL_CYRUS_CLIENT *client = (XSASL_CYRUS_CLIENT *) context;

    if (msg_verbose)
	msg_info("%s: %s", myname, client->username);

    /*
     * Sanity check.
     */
    if (client->password == 0)
	msg_panic("%s: no username looked up", myname);

    *result = client->username;
    if (len)
	*len = strlen(client->username);
    return (SASL_OK);
}

/* xsasl_cyrus_client_get_passwd - password lookup call-back routine */

static int xsasl_cyrus_client_get_passwd(sasl_conn_t *conn, void *context,
				            int id, sasl_secret_t **psecret)
{
    const char *myname = "xsasl_cyrus_client_get_passwd";
    XSASL_CYRUS_CLIENT *client = (XSASL_CYRUS_CLIENT *) context;
    int     len;

    if (msg_verbose)
	msg_info("%s: %s", myname, client->password);

    /*
     * Sanity check.
     */
    if (!conn || !psecret || id != SASL_CB_PASS)
	return (SASL_BADPARAM);
    if (client->password == 0)
	msg_panic("%s: no password looked up", myname);

    /*
     * Convert the password into a counted string.
     */
    len = strlen(client->password);
    if ((*psecret = (sasl_secret_t *) malloc(sizeof(sasl_secret_t) + len)) == 0)
	return (SASL_NOMEM);
    (*psecret)->len = len;
    memcpy((*psecret)->data, client->password, len + 1);

    return (SASL_OK);
}

/* xsasl_cyrus_client_init - initialize Cyrus SASL library */

XSASL_CLIENT_IMPL *xsasl_cyrus_client_init(const char *unused_client_type,
					       const char *unused_path_info)
{
    XSASL_CLIENT_IMPL *xp;
    int     sasl_status;

    /*
     * Global callbacks. These have no per-session context.
     */
    static sasl_callback_t callbacks[] = {
	{SASL_CB_LOG, &xsasl_cyrus_log, 0},
	{SASL_CB_LIST_END, 0, 0}
    };

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

    /*
     * Initialize the SASL library.
     */
    if ((sasl_status = sasl_client_init(callbacks)) != SASL_OK) {
	msg_warn("SASL library initialization error: %s",
		 xsasl_cyrus_strerror(sasl_status));
	return (0);
    }

    /*
     * Return a generic XSASL_CLIENT_IMPL object. We don't need to extend it
     * with our own methods or data.
     */
    xp = (XSASL_CLIENT_IMPL *) mymalloc(sizeof(*xp));
    xp->create = xsasl_cyrus_client_create;
    xp->done = xsasl_cyrus_client_done;
    return (xp);
}

/* xsasl_cyrus_client_done - dispose of implementation */

static void xsasl_cyrus_client_done(XSASL_CLIENT_IMPL *impl)
{
    myfree((char *) impl);
    sasl_done();
}

/* xsasl_cyrus_client_create - per-session SASL initialization */

XSASL_CLIENT *xsasl_cyrus_client_create(XSASL_CLIENT_IMPL *unused_impl,
					        VSTREAM *stream,
					        const char *service,
					        const char *server,
					        const char *sec_props)
{
    XSASL_CYRUS_CLIENT *client = 0;
    static sasl_callback_t callbacks[] = {
	{SASL_CB_USER, &xsasl_cyrus_client_get_user, 0},
	{SASL_CB_AUTHNAME, &xsasl_cyrus_client_get_user, 0},
	{SASL_CB_PASS, &xsasl_cyrus_client_get_passwd, 0},
	{SASL_CB_LIST_END, 0, 0}
    };
    sasl_conn_t *sasl_conn = 0;
    sasl_callback_t *custom_callbacks = 0;
    sasl_callback_t *cp;
    int     sasl_status;

    /*
     * The optimizer will eliminate code duplication and/or dead code.
     */
#define XSASL_CYRUS_CLIENT_CREATE_ERROR_RETURN(x) \
    do { \
	if (client) { \
	    xsasl_cyrus_client_free(&client->xsasl); \
	} else { \
	    if (custom_callbacks) \
		myfree((char *) custom_callbacks); \
	    if (sasl_conn) \
		sasl_dispose(&sasl_conn); \
	} \
	return (x); \
    } while (0)

    /*
     * Per-session initialization. Provide each session with its own callback
     * context.
     */
#define NULL_SECFLAGS		0

    custom_callbacks = (sasl_callback_t *) mymalloc(sizeof(callbacks));
    memcpy((char *) custom_callbacks, callbacks, sizeof(callbacks));

#define NULL_SERVER_ADDR	((char *) 0)
#define NULL_CLIENT_ADDR	((char *) 0)

    if ((sasl_status = SASL_CLIENT_NEW(service, server,
				       NULL_CLIENT_ADDR, NULL_SERVER_ADDR,
				 var_cyrus_sasl_authzid ? custom_callbacks :
				       custom_callbacks + 1, NULL_SECFLAGS,
				       &sasl_conn)) != SASL_OK) {
	msg_warn("per-session SASL client initialization: %s",
		 xsasl_cyrus_strerror(sasl_status));
	XSASL_CYRUS_CLIENT_CREATE_ERROR_RETURN(0);
    }

    /*
     * Extend XSASL_CLIENT_IMPL object with our own state. We use long-lived
     * conversion buffers rather than local variables to avoid memory leaks
     * in case of read/write timeout or I/O error.
     * 
     * XXX If we enable SASL encryption, there needs to be a way to inform the
     * application, so that they can turn off connection caching, refuse
     * STARTTLS, etc.
     */
    client = (XSASL_CYRUS_CLIENT *) mymalloc(sizeof(*client));
    client->xsasl.free = xsasl_cyrus_client_free;
    client->xsasl.first = xsasl_cyrus_client_first;
    client->xsasl.next = xsasl_cyrus_client_next;
    client->stream = stream;
    client->sasl_conn = sasl_conn;
    client->callbacks = custom_callbacks;
    client->decoded = vstring_alloc(20);
    client->username = 0;
    client->password = 0;

    for (cp = custom_callbacks; cp->id != SASL_CB_LIST_END; cp++)
	cp->context = (void *) client;

    if (xsasl_cyrus_client_set_security(&client->xsasl, sec_props)
	!= XSASL_AUTH_OK)
	XSASL_CYRUS_CLIENT_CREATE_ERROR_RETURN(0);

    return (&client->xsasl);
}

/* xsasl_cyrus_client_set_security - set security properties */

static int xsasl_cyrus_client_set_security(XSASL_CLIENT *xp,
					           const char *sasl_opts_val)
{
    XSASL_CYRUS_CLIENT *client = (XSASL_CYRUS_CLIENT *) xp;
    sasl_security_properties_t sec_props;
    int     sasl_status;

    /*
     * Per-session security properties. XXX This routine is not sufficiently
     * documented. What is the purpose of all this?
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
    if ((sasl_status = sasl_setprop(client->sasl_conn, SASL_SEC_PROPS,
				    &sec_props)) != SASL_OK) {
	msg_warn("set per-session SASL security properties: %s",
		 xsasl_cyrus_strerror(sasl_status));
	return (XSASL_AUTH_FAIL);
    }
    return (XSASL_AUTH_OK);
}

/* xsasl_cyrus_client_first - run authentication protocol */

static int xsasl_cyrus_client_first(XSASL_CLIENT *xp,
				            const char *mechanism_list,
				            const char *username,
				            const char *password,
				            const char **mechanism,
				            VSTRING *init_resp)
{
    const char *myname = "xsasl_cyrus_client_first";
    XSASL_CYRUS_CLIENT *client = (XSASL_CYRUS_CLIENT *) xp;
    unsigned enc_length;
    unsigned enc_length_out;
    CLIENTOUT_TYPE clientout;
    unsigned clientoutlen;
    int     sasl_status;

#define NO_SASL_SECRET		0
#define NO_SASL_INTERACTION	0

    /*
     * Save the username and password for the call-backs.
     */
    if (client->username)
	myfree(client->username);
    client->username = mystrdup(username);
    if (client->password)
	myfree(client->password);
    client->password = mystrdup(password);

    /*
     * Start the client side authentication protocol.
     */
    sasl_status = SASL_CLIENT_START((sasl_conn_t *) client->sasl_conn,
				    mechanism_list,
				    NO_SASL_SECRET, NO_SASL_INTERACTION,
				    &clientout, &clientoutlen, mechanism);
    if (sasl_status != SASL_OK && sasl_status != SASL_CONTINUE) {
	vstring_strcpy(init_resp, xsasl_cyrus_strerror(sasl_status));
	return (XSASL_AUTH_FAIL);
    }

    /*
     * Generate the AUTH command and the optional initial client response.
     * sasl_encode64() produces four bytes for each complete or incomplete
     * triple of input bytes. Allocate an extra byte for string termination.
     */
#define ENCODE64_LENGTH(n)	((((n) + 2) / 3) * 4)

    if (clientoutlen > 0) {
	if (msg_verbose) {
	    escape(client->decoded, clientout, clientoutlen);
	    msg_info("%s: uncoded initial reply: %s",
		     myname, STR(client->decoded));
	}
	enc_length = ENCODE64_LENGTH(clientoutlen) + 1;
	VSTRING_RESET(init_resp);		/* Fix 200512 */
	VSTRING_SPACE(init_resp, enc_length);
	if ((sasl_status = sasl_encode64(clientout, clientoutlen,
					 STR(init_resp),
					 vstring_avail(init_resp),
					 &enc_length_out)) != SASL_OK)
	    msg_panic("%s: sasl_encode64 botch: %s",
		      myname, xsasl_cyrus_strerror(sasl_status));
	VSTRING_AT_OFFSET(init_resp, enc_length_out);	/* XXX */
#if SASL_VERSION_MAJOR < 2
	/* SASL version 1 doesn't free memory that it allocates. */
	free(clientout);
#endif
    } else {
	vstring_strcpy(init_resp, "");
    }
    return (XSASL_AUTH_OK);
}

/* xsasl_cyrus_client_next - continue authentication */

static int xsasl_cyrus_client_next(XSASL_CLIENT *xp, const char *server_reply,
				           VSTRING *client_reply)
{
    const char *myname = "xsasl_cyrus_client_next";
    XSASL_CYRUS_CLIENT *client = (XSASL_CYRUS_CLIENT *) xp;
    unsigned enc_length;
    unsigned enc_length_out;
    CLIENTOUT_TYPE clientout;
    unsigned clientoutlen;
    unsigned serverinlen;
    int     sasl_status;

    /*
     * Process a server challenge.
     */
    serverinlen = strlen(server_reply);
    VSTRING_RESET(client->decoded);		/* Fix 200512 */
    VSTRING_SPACE(client->decoded, serverinlen);
    if ((sasl_status = SASL_DECODE64(server_reply, serverinlen,
				     STR(client->decoded),
				     vstring_avail(client->decoded),
				     &enc_length)) != SASL_OK) {
	vstring_strcpy(client_reply, xsasl_cyrus_strerror(sasl_status));
	return (XSASL_AUTH_FORM);
    }
    if (msg_verbose)
	msg_info("%s: decoded challenge: %.*s",
		 myname, (int) enc_length, STR(client->decoded));
    sasl_status = sasl_client_step(client->sasl_conn, STR(client->decoded),
				   enc_length, NO_SASL_INTERACTION,
				   &clientout, &clientoutlen);
    if (sasl_status != SASL_OK && sasl_status != SASL_CONTINUE) {
	vstring_strcpy(client_reply, xsasl_cyrus_strerror(sasl_status));
	return (XSASL_AUTH_FAIL);
    }

    /*
     * Send a client response.
     */
    if (clientoutlen > 0) {
	if (msg_verbose)
	    msg_info("%s: uncoded client response %.*s",
		     myname, (int) clientoutlen, clientout);
	enc_length = ENCODE64_LENGTH(clientoutlen) + 1;
	VSTRING_RESET(client_reply);		/* Fix 200512 */
	VSTRING_SPACE(client_reply, enc_length);
	if ((sasl_status = sasl_encode64(clientout, clientoutlen,
					 STR(client_reply),
					 vstring_avail(client_reply),
					 &enc_length_out)) != SASL_OK)
	    msg_panic("%s: sasl_encode64 botch: %s",
		      myname, xsasl_cyrus_strerror(sasl_status));
#if SASL_VERSION_MAJOR < 2
	/* SASL version 1 doesn't free memory that it allocates. */
	free(clientout);
#endif
    } else {
	/* XXX Can't happen. */
	vstring_strcpy(client_reply, "");
    }
    return (XSASL_AUTH_OK);
}

/* xsasl_cyrus_client_free - per-session cleanup */

void    xsasl_cyrus_client_free(XSASL_CLIENT *xp)
{
    XSASL_CYRUS_CLIENT *client = (XSASL_CYRUS_CLIENT *) xp;

    if (client->username)
	myfree(client->username);
    if (client->password)
	myfree(client->password);
    if (client->sasl_conn)
	sasl_dispose(&client->sasl_conn);
    myfree((char *) client->callbacks);
    vstring_free(client->decoded);
    myfree((char *) client);
}

#endif

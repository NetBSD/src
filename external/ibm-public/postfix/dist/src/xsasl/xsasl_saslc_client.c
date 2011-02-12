/*	$NetBSD: xsasl_saslc_client.c,v 1.1 2011/02/12 19:07:09 christos Exp $	*/

/*++
/* NAME
/*	xsasl_saslc_client 3
/* SUMMARY
/*	saslc SASL client-side plug-in
/* SYNOPSIS
/*	#include <xsasl_saslc_client.h>
/*
/*	XSASL_CLIENT_IMPL *xsasl_saslc_client_init(client_type, path_info)
/*	const char *client_type;
/* DESCRIPTION
/*	This module implements the saslc SASL client-side authentication
/*	plug-in.
/*
/*	xsasl_saslc_client_init() initializes the saslc SASL library and
/*	returns an implementation handle that can be used to generate
/*	SASL client instances.
/*
/*	Arguments:
/* .IP client_type
/*	The plug-in SASL client type (saslc). This argument is
/*	ignored, but it could be used when one implementation
/*	provides multiple variants.
/* .IP path_info
/*	Implementation-specific information to specify the location
/*	of a configuration file, rendez-vous point, etc. This
/*	information is ignored by the saslc SASL client plug-in.
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

#if defined(USE_SASL_AUTH) && defined(USE_SASLC_SASL)

 /*
  * System headers.
  */
#include <errno.h>
#include <saslc.h>
#include <stdlib.h>
#include <string.h>

#include "sys_defs.h"

 /*
  * Utility library
  */
#include "msg.h"
#include "mymalloc.h"
#include "stringops.h"

 /*
  * Global library
  */
#include "mail_params.h"

 /*
  * Application-specific
  */
#include "xsasl.h"
#include "xsasl_saslc.h"


#define XSASL_SASLC_APPNAME	"postfix"  /* The config files are in
					      /etc/saslc.d/<appname>/ */
typedef struct {
	XSASL_CLIENT_IMPL xsasl;	/* generic members, must be first */
	saslc_t *saslc;			/* saslc context */
} XSASL_SASLC_CLIENT_IMPL;

typedef struct {
	XSASL_CLIENT xsasl;		/* generic members, must be first */
	saslc_t *saslc;			/* saslc context */
	saslc_sess_t *sess;		/* session context */
	const char *service;		/* service (smtp) */
	const char *hostname;		/* server host name */
	const char *sec_opts;		/* security options */
} XSASL_SASLC_CLIENT;

static XSASL_CLIENT *xsasl_saslc_client_create(XSASL_CLIENT_IMPL *,
    XSASL_CLIENT_CREATE_ARGS *);
static int xsasl_saslc_client_first(XSASL_CLIENT *, const char *,
    const char *, const char *, const char **, VSTRING *);
static int xsasl_saslc_client_next(XSASL_CLIENT *, const char *,
    VSTRING *);
static void xsasl_saslc_client_done(XSASL_CLIENT_IMPL *);
static void xsasl_saslc_client_free(XSASL_CLIENT *);

static void
setprop(saslc_sess_t *sess, int overwrite, const char *key, const char *value)
{

	if (overwrite != 0 ||
	    saslc_sess_getprop(sess, key) == NULL)
		saslc_sess_setprop(sess, key, value);
}

/*
 * Run authentication protocol: first step.
 */
static int
xsasl_saslc_client_first(
	XSASL_CLIENT *xp,
	const char *mechanism_list,
	const char *username,
	const char *password,
	const char **mechanism,
	VSTRING *init_resp)
{
	XSASL_SASLC_CLIENT *client = (XSASL_SASLC_CLIENT *)xp;
	const char *mech;
	void *out;
	size_t outlen;
	int rv;

	if (msg_verbose) {
		msg_info("%s: mechanism_list='%s'", __func__, mechanism_list);
		msg_info("%s: username='%s'", __func__, username);
/*		msg_info("%s: password='%s'", __func__, password); */
	}
	client->sess = saslc_sess_init(client->saslc, mechanism_list,
					client->sec_opts);
	if (client->sess == NULL) {
		msg_info("%s: saslc_sess_init failed", __func__);
		return XSASL_AUTH_FAIL;
	}
	mech = saslc_sess_getmech(client->sess);
	if (mechanism)
		*mechanism = mech;
	if (msg_verbose)
		msg_info("%s: mechanism='%s'", __func__, mech);

	setprop(client->sess, 0, SASLC_PROP_AUTHCID,  username);
	setprop(client->sess, 1, SASLC_PROP_PASSWD,   password);
	setprop(client->sess, 1, SASLC_PROP_SERVICE,  client->service);
	setprop(client->sess, 1, SASLC_PROP_HOSTNAME, client->hostname);
	setprop(client->sess, 1, SASLC_PROP_BASE64IO, "true");
	setprop(client->sess, 0, SASLC_PROP_QOPMASK,  "auth");

	if ((rv = saslc_sess_cont(client->sess, NULL, 0, &out, &outlen))
	    == -1) {
		msg_info("%s: saslc_sess_encode='%s'", __func__,
		    saslc_sess_strerror(client->sess));
		return XSASL_AUTH_FAIL;
	}
	vstring_strcpy(init_resp, outlen ? out : "");
	if (msg_verbose)
		msg_info("%s: client_reply='%s'", __func__, outlen ? out : "");

	if (outlen > 0)
		memset(out, 0, outlen);		/* XXX: silly? */
	if (out != NULL)
		free (out);

	return XSASL_AUTH_OK;
}

/*
 * Continue authentication.
 */
static int
xsasl_saslc_client_next(XSASL_CLIENT *xp, const char *server_reply,
    VSTRING *client_reply)
{
	XSASL_SASLC_CLIENT *client;
	void *out;
	size_t outlen;

	client = (XSASL_SASLC_CLIENT *)xp;

	if (msg_verbose)
		msg_info("%s: server_reply='%s'", __func__, server_reply);

	if (saslc_sess_cont(client->sess, server_reply, strlen(server_reply),
	    &out, &outlen) == -1) {
		msg_info("%s: saslc_sess_encode='%s'", __func__,
		    saslc_sess_strerror(client->sess));
		return XSASL_AUTH_FAIL;
	}
	vstring_strcpy(client_reply, outlen ? out : "");
	if (msg_verbose)
		msg_info("%s: client_reply='%s'", __func__,
		    outlen ? out : "");

	if (outlen > 0)
		memset(out, 0, outlen);		/* XXX: silly? */
	if (out != NULL)
		free (out);

	return XSASL_AUTH_OK;
}

/*
 * Per-session cleanup.
 */
void
xsasl_saslc_client_free(XSASL_CLIENT *xp)
{
	XSASL_SASLC_CLIENT *client;

	client = (XSASL_SASLC_CLIENT *)xp;
	if (client->sess)
		saslc_sess_end(client->sess);
	myfree((char *)client);
}

/*
 * Per-session SASL initialization.
 */
XSASL_CLIENT *
xsasl_saslc_client_create(XSASL_CLIENT_IMPL *impl,
    XSASL_CLIENT_CREATE_ARGS *args)
{
	XSASL_SASLC_CLIENT_IMPL *xp;
	XSASL_SASLC_CLIENT *client;

	xp = (XSASL_SASLC_CLIENT_IMPL *)impl;
	if (msg_verbose) {
		msg_info("%s: service='%s'", __func__, args->service);
		msg_info("%s: server_name='%s'", __func__, args->server_name);
		msg_info("%s: security_options='%s'", __func__,
		    args->security_options);
	}

	/* NB: mymalloc never returns NULL, it calls _exit(3) instead */
	client = (XSASL_SASLC_CLIENT *)mymalloc(sizeof(*client));

	client->xsasl.free  = xsasl_saslc_client_free;
	client->xsasl.first = xsasl_saslc_client_first;
	client->xsasl.next  = xsasl_saslc_client_next;

	client->saslc = xp->saslc;

	/* XXX: should these be strdup()ed? */
	client->service  = args->service;
	client->hostname = args->server_name;
	client->sec_opts = args->security_options;

	return &client->xsasl;
}

/*
 * Dispose of implementation.
 */
static void
xsasl_saslc_client_done(XSASL_CLIENT_IMPL *impl)
{
	XSASL_SASLC_CLIENT_IMPL *xp;

	xp = (XSASL_SASLC_CLIENT_IMPL *)impl;
	if (xp->saslc) {
		saslc_end(xp->saslc);
		xp->saslc = NULL;  /* XXX: unnecessary as freeing impl */
	}
	myfree((char *)impl);
}

/*
 * Initialize saslc SASL library.
 */
XSASL_CLIENT_IMPL *
xsasl_saslc_client_init(const char *client_type, const char *path_info)
{
	XSASL_SASLC_CLIENT_IMPL *xp;

	/* XXX: This should be unnecessary! */
	if (strcmp(client_type, XSASL_TYPE_SASLC) != 0) {
		msg_info("%s: invalid client_type: '%s'", __func__,
		    client_type);
		return NULL;
	}
	if (msg_verbose) {
		msg_info("%s: client_type='%s'", __func__, client_type);
		msg_info("%s: path_info='%s'",   __func__, path_info);
	}

	/* NB: mymalloc() never returns NULL, it calls _exit(3) instead */
	xp = (XSASL_SASLC_CLIENT_IMPL *)mymalloc(sizeof(*xp));
	xp->xsasl.create = xsasl_saslc_client_create;
	xp->xsasl.done = xsasl_saslc_client_done;

	/* NB: msg_fatal() exits the program immediately after printing */
	if ((xp->saslc = saslc_alloc()) == NULL)
		msg_fatal("%s: saslc_alloc failed: %s", __func__,
		    strerror(errno));

	if (saslc_init(xp->saslc, XSASL_SASLC_APPNAME, path_info) == -1)
		msg_fatal("%s: saslc_init failed: %s", __func__,
		    saslc_strerror(xp->saslc));

	return &xp->xsasl;
}

#endif /* defined(USE_SASL_AUTH) && defined(USE_SASLC_SASL) */

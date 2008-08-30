#ifndef _XSASL_H_INCLUDED_
#define _XSASL_H_INCLUDED_

/*++
/* NAME
/*	xsasl 3h
/* SUMMARY
/*	Postfix SASL plug-in interface
/* SYNOPSIS
/*	#include <xsasl.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <argv.h>
#include <vstream.h>
#include <vstring.h>

 /*
  * Generic server object. Specific instances extend this with their own
  * private data.
  */
typedef struct XSASL_SERVER {
    void    (*free) (struct XSASL_SERVER *);
    int     (*first) (struct XSASL_SERVER *, const char *, const char *, VSTRING *);
    int     (*next) (struct XSASL_SERVER *, const char *, VSTRING *);
    const char *(*get_mechanism_list) (struct XSASL_SERVER *);
    const char *(*get_username) (struct XSASL_SERVER *);
} XSASL_SERVER;

#define xsasl_server_free(server) (server)->free(server)
#define xsasl_server_first(server, method, init_resp, reply) \
	(server)->first((server), (method), (init_resp), (reply))
#define xsasl_server_next(server, request, reply) \
	(server)->next((server), (request), (reply))
#define xsasl_server_get_mechanism_list(server) \
	(server)->get_mechanism_list((server))
#define xsasl_server_get_username(server) \
	(server)->get_username((server))

 /*
  * Generic server implementation. Specific instances extend this with their
  * own private data.
  */
typedef struct XSASL_SERVER_IMPL {
    XSASL_SERVER *(*create) (struct XSASL_SERVER_IMPL *, VSTREAM *, const char *, const char *, const char *);
    void    (*done) (struct XSASL_SERVER_IMPL *);
} XSASL_SERVER_IMPL;

extern XSASL_SERVER_IMPL *xsasl_server_init(const char *, const char *);
extern ARGV *xsasl_server_types(void);

#define xsasl_server_create(impl, stream, service, realm, sec_props) \
	(impl)->create((impl), (stream), (service), (realm), (sec_props))
#define xsasl_server_done(impl) (impl)->done((impl));

 /*
  * Generic client object. Specific instances extend this with their own
  * private data.
  */
typedef struct XSASL_CLIENT {
    void    (*free) (struct XSASL_CLIENT *);
    int     (*first) (struct XSASL_CLIENT *, const char *, const char *, const char *, const char **, VSTRING *);
    int     (*next) (struct XSASL_CLIENT *, const char *, VSTRING *);
} XSASL_CLIENT;

#define xsasl_client_free(client) (client)->free(client)
#define xsasl_client_first(client, server, method, user, pass, init_resp) \
	(client)->first((client), (server), (method), (user), (pass), (init_resp))
#define xsasl_client_next(client, request, reply) \
	(client)->next((client), (request), (reply))
#define xsasl_client_set_password(client, user, pass) \
	(client)->set_password((client), (user), (pass))

 /*
  * Generic client implementation. Specific instances extend this with their
  * own private data.
  */
typedef struct XSASL_CLIENT_IMPL {
    XSASL_CLIENT *(*create) (struct XSASL_CLIENT_IMPL *, VSTREAM *, const char *, const char *, const char *);
    void    (*done) (struct XSASL_CLIENT_IMPL *);
} XSASL_CLIENT_IMPL;

extern XSASL_CLIENT_IMPL *xsasl_client_init(const char *, const char *);
extern ARGV *xsasl_client_types(void);

#define xsasl_client_create(impl, stream, service, server, sec_props) \
	(impl)->create((impl), (stream), (service), (server), (sec_props))
#define xsasl_client_done(impl) (impl)->done((impl));

 /*
  * Status codes.
  */
#define XSASL_AUTH_OK	1		/* Success */
#define XSASL_AUTH_MORE	2		/* Need another c/s protocol exchange */
#define XSASL_AUTH_DONE	3		/* Authentication completed */
#define XSASL_AUTH_FORM	4		/* Cannot decode response */
#define XSASL_AUTH_FAIL	5		/* Error */

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

#endif

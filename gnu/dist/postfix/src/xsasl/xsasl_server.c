/*++
/* NAME
/*	xsasl-server 3
/* SUMMARY
/*	Postfix SASL server plug-in interface
/* SYNOPSIS
/*	#include <xsasl.h>
/*
/*	XSASL_SERVER_IMPL *xsasl_server_init(server_type, path_info)
/*	const char *server_type;
/*	const char *path_info;
/*
/*	void	xsasl_server_done(implementation)
/*	XSASL_SERVER_IMPL *implementation;
/*
/*	ARGV	*xsasl_server_types()
/*
/*	XSASL_SERVER *xsasl_server_create(implementation, stream, service,
/*						user_realm, security_options)
/*	XSASL_SERVER_IMPL *implementation;
/*	const char *service;
/*	VSTREAM *stream;
/*	const char *user_realm;
/*	const char *security_options;
/*
/*	void xsasl_server_free(server)
/*	XSASL_SERVER *server;
/*
/*	int	xsasl_server_first(server, auth_method, init_resp, server_reply)
/*	XSASL_SERVER *server;
/*	const char *auth_method;
/*	const char *init_resp;
/*	VSTRING *server_reply;
/*
/*	int	xsasl_server_next(server, client_request, server_reply)
/*	XSASL_SERVER *server;
/*	const char *client_request;
/*	VSTRING *server_reply;
/*
/*	const char *xsasl_server_get_mechanism_list(server)
/*	XSASL_SERVER *server;
/*
/*	const char *xsasl_server_get_username(server)
/*	XSASL_SERVER *server;
/* DESCRIPTION
/*	The XSASL_SERVER abstraction implements a generic interface
/*	to one or more SASL authentication implementations.
/*
/*	xsasl_server_init() is called once during process initialization.
/*	It selects a SASL implementation by name, specifies the
/*	location of a configuration file or rendez-vous point, and
/*	returns an implementation handle that can be used to generate
/*	SASL server instances. This function is typically used to
/*	initialize the underlying implementation.
/*
/*	xsasl_server_done() disposes of an implementation handle,
/*	and allows the underlying implementation to release resources.
/*
/*	xsasl_server_types() lists the available implementation types.
/*	The result should be destroyed by the caller.
/*
/*	xsasl_server_create() is called at the start of an SMTP
/*	session. It generates a Postfix SASL plug-in server instance
/*	for the specified service and authentication realm, and
/*	with the specified security properties. Specify a null
/*	pointer when no realm should be used. The stream handle is
/*	stored so that encryption can be turned on after successful
/*	negotiations.
/*
/*	xsasl_server_free() is called at the end of an SMTP session.
/*	It destroys a SASL server instance, and disables further
/*	read/write operations if encryption was turned on.
/*
/*	xsasl_server_first() produces the server reponse for the
/*	client AUTH command. The client input are an authentication
/*	method, and an optional initial response or null pointer.
/*	The initial response and server non-error replies are BASE64
/*	encoded.  Server error replies are 7-bit ASCII text without
/*	control characters, without BASE64 encoding, and without
/*	SMTP reply code or enhanced status code.
/*
/*	The result is one of the following:
/* .IP XSASL_AUTH_MORE
/*	More client input is needed. The server reply specifies
/*	what.
/* .IP XSASL_AUTH_DONE
/*	Authentication completed successfully.
/* .IP XSASL_AUTH_FORM
/*	The client input is incorrectly formatted. The server error
/*	reply explains why.
/* .IP XSASL_AUTH_FAIL
/*	Authentication failed. The server error reply explains why.
/* .PP
/*	xsasl_server_next() supports the subsequent stages of the
/*	client-server AUTH protocol. Both the client input and
/*	server non-error responses are BASE64 encoded.  See
/*	xsasl_server_first() for other details.
/*
/*	xsasl_server_get_mechanism_list() returns the authentication
/*	mechanisms that match the security properties, as a white-space
/*	separated list. This is meant to be used in the SMTP EHLO
/*	reply.
/*
/*	xsasl_server_get_username() returns the stored username
/*	after successful authentication.
/*
/*	Arguments:
/* .IP auth_method
/*	AUTH command authentication method.
/* .IP init_resp
/*	AUTH command initial response or null pointer.
/* .IP implementation
/*	Implementation handle that was obtained with xsasl_server_init().
/* .IP path_info
/*	The value of the smtpd_sasl_path parameter or equivalent.
/*	This specifies the implementation-dependent location of a
/*	configuration file, rendez-vous point, etc., and is passed
/*	unchanged to the plug-in.
/* .IP security_options
/*	The value of the smtpd_security_options parameter or
/*	equivalent. This is passed unchanged to the plug-in.
/* .IP server
/*	SASL plug-in server handle.
/* .IP server_reply
/*	BASE64 encoded server non-error reply (without SMTP reply
/*	code or enhanced status code), or ASCII error description.
/* .IP server_type
/*	The name of a Postfix SASL server plug_in implementation.
/* .IP server_types
/*	Null-terminated array of strings with SASL server plug-in
/*	implementation names.
/* .IP service
/*	The service that is implemented by the local server, typically
/*	"smtp" or "lmtp".
/* .IP stream
/*	The connection between client and server.  When SASL
/*	encryption is negotiated, the plug-in will transparently
/*	intercept the socket read/write operations.
/* .IP user_realm
/*	Authentication domain or null pointer.
/* SECURITY
/* .ad
/* .fi
/*	The caller does not sanitize client input. It is the
/*	responsibility of the underlying SASL server implementation
/*	to produce 7-bit ASCII without control characters as server
/*	non-error and error replies, and as the result from
/*	xsasl_server_method() and xsasl_server_username().
/* DIAGNOSTICS
/*	In case of failure, xsasl_server_init(), xsasl_server_create(),
/*	xsasl_server_get_mechanism_list() and xsasl_server_get_username()
/*	log a warning and return a null pointer.
/*
/*	Functions that normally return XSASL_AUTH_OK will log a warning
/*	and return an appropriate result value.
/*
/*	Fatal errors: out of memory.
/*
/*	Panic: interface violations.
/* SEE ALSO
/*	cyrus_security(3) Cyrus SASL security features
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this
/*	software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>

/* SASL implementations. */

#include <xsasl.h>
#include <xsasl_cyrus.h>
#include <xsasl_dovecot.h>

 /*
  * Lookup table for available SASL server implementations.
  */
typedef struct {
    char   *server_type;
    struct XSASL_SERVER_IMPL *(*server_init) (const char *, const char *);
} XSASL_SERVER_IMPL_INFO;

static const XSASL_SERVER_IMPL_INFO server_impl_info[] = {
#ifdef XSASL_TYPE_CYRUS
    {XSASL_TYPE_CYRUS, xsasl_cyrus_server_init},
#endif
#ifdef XSASL_TYPE_DOVECOT
    {XSASL_TYPE_DOVECOT, xsasl_dovecot_server_init},
#endif
    {0, 0}
};

/* xsasl_server_init - look up server implementation by name */

XSASL_SERVER_IMPL *xsasl_server_init(const char *server_type,
				             const char *path_info)
{
    const XSASL_SERVER_IMPL_INFO *xp;

    for (xp = server_impl_info; xp->server_type; xp++)
	if (strcmp(server_type, xp->server_type) == 0)
	    return (xp->server_init(server_type, path_info));
    msg_warn("unsupported SASL server implementation: %s", server_type);
    return (0);
}

/* xsasl_server_types - report available implementation types */

ARGV   *xsasl_server_types(void)
{
    const XSASL_SERVER_IMPL_INFO *xp;
    ARGV   *argv = argv_alloc(1);

    for (xp = server_impl_info; xp->server_type; xp++)
	argv_add(argv, xp->server_type, ARGV_END);
    return (argv);
}

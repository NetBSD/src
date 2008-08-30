/*++
/* NAME
/*	xsasl_client 3
/* SUMMARY
/*	Postfix SASL client plug-in interface
/* SYNOPSIS
/*	#include <xsasl.h>
/*
/*	XSASL_CLIENT_IMPL *xsasl_client_init(client_type, path_info)
/*	const char *client_type;
/*	const char *path_info;
/*
/*	void	xsasl_client_done(implementation)
/*	XSASL_CLIENT_IMPL *implementation;
/*
/*	ARGV	*xsasl_client_types()
/*
/*	XSASL_CLIENT *xsasl_client_create(implementation, stream, service, 
/*					server_name, security_properties)
/*	XSASL_CLIENT_IMPL *implementation;
/*	VSTREAM	*stream;
/*	const char *service;
/*	const char *server_name;
/*	const char *security_properties;
/*
/*	void	xsasl_client_free(client)
/*	XSASL_CLIENT *client;
/*
/*	int	xsasl_client_first(client, stream, mech_list, username,
/*					password, auth_method, init_resp)
/*	XSASL_CLIENT *client;
/*	const char *mech_list;
/*	const char *username;
/*	const char *password;
/*	const char **auth_method;
/*	VSTRING *init_resp;
/*
/*	int	xsasl_client_next(client, server_reply, client_reply)
/*	XSASL_CLIENT *client;
/*	const char *server_reply;
/*	VSTRING *client_reply;
/* DESCRIPTION
/*	The XSASL_CLIENT abstraction implements a generic interface
/*	to one or more SASL authentication implementations.
/*
/*	xsasl_client_init() is called once during process initialization.
/*	It selects a SASL implementation by name, specifies the
/*	location of a configuration file or rendez-vous point, and
/*	returns an implementation handle that can be used to generate
/*	SASL client instances. This function is typically used to
/*	initialize the underlying implementation.
/*
/*	xsasl_client_done() disposes of an implementation handle,
/*	and allows the underlying implementation to release resources.
/*
/*	xsasl_client_types() lists the available implementation types.
/*	The result should be destroyed by the caller.
/*
/*	xsasl_client_create() is called at the start of an SMTP
/*	session. It generates a Postfix SASL plug-in client instance
/*	for the specified service and server name, with the specified
/*	security properties. The stream handle is stored so that
/*	encryption can be turned on after successful negotiations.
/*
/*	xsasl_client_free() is called at the end of an SMTP session.
/*	It destroys a SASL client instance, and disables further
/*	read/write operations if encryption was turned on.
/*
/*	xsasl_client_first() produces the client input for the AUTH
/*	command. The input is an authentication method list from
/*	an EHLO response, a username and a password. On return, the
/*	method argument specifies the authentication method; storage
/*	space is owned by the underlying implementation.  The initial
/*	response and client non-error replies are BASE64 encoded.
/*	Client error replies are 7-bit ASCII text without control
/*	characters, and without BASE64 encoding. They are meant for
/*	the local application, not for transmission to the server.
/*	The client may negotiate encryption of the client-server
/*	connection.
/*
/*	The result is one of the following:
/* .IP XSASL_AUTH_OK
/*	Success.
/* .IP XSASL_AUTH_FORM
/*	The server reply is incorrectly formatted. The client error
/*	reply explains why.
/* .IP XSASL_AUTH_FAIL
/*	Other error. The client error reply explains why.
/* .PP
/*	xsasl_client_next() supports the subsequent stages of the
/*	AUTH protocol. Both the client reply and client non-error
/*	responses are BASE64 encoded.  See xsasl_client_first() for
/*	other details.
/*
/*	Arguments:
/* .IP client
/*	SASL plug-in client handle.
/* .IP client_reply
/*	BASE64 encoded non-error client reply, or ASCII error
/*	description for the user.
/* .IP client_type
/*	The name of a Postfix SASL client plug_in implementation.
/* .IP client_types
/*	Null-terminated array of strings with SASL client plug-in
/*	implementation names.
/* .IP init_resp
/*	The AUTH command initial response.
/* .IP implementation
/*	Implementation handle that was obtained with xsasl_client_init().
/* .IP mech_list
/*	List of SASL mechanisms as announced by the server.
/* .IP auth_method
/*	The AUTH command authentication method.
/* .IP password
/*	Information from the Postfix SASL password file or equivalent.
/* .IP path_info
/*	The value of the smtp_sasl_path parameter or equivalent.
/*	This specifies the implementation-dependent location of a
/*	configuration file, rendez-vous point, etc., and is passed
/*	unchanged to the plug-in.
/* .IP security_options
/*	The value of the smtp_sasl_security_options parameter or
/*	equivalent. This is passed unchanged to the plug-in.
/* .IP server_name
/*	The remote server fully qualified hostname.
/* .IP server_reply
/*	BASE64 encoded server reply without SMTP reply code or
/*	enhanced status code.
/* .IP service
/*	The service that is implemented by the local client (typically,
/*	"lmtp" or "smtp").
/* .IP stream
/*	The connection between client and server.
/*	When SASL encryption is negotiated, the plug-in will
/*	transparently intercept the socket read/write operations.
/* .IP username
/*	Information from the Postfix SASL password file.
/* SECURITY
/* .ad
/* .fi
/*	The caller does not sanitize the server reply. It is the
/*	responsibility of the underlying SASL client implementation
/*	to produce 7-bit ASCII without control characters as client
/*	non-error and error replies.
/* DIAGNOSTICS
/*	In case of error, xsasl_client_init() and xsasl_client_create()
/*	log a warning and return a null pointer.
/*
/*	Functions that normally return XSASL_AUTH_OK will log a warning
/*	and return an appropriate result value.
/*
/*	Panic: interface violation.
/*
/*	Fatal errors: out of memory.
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

 /*
  * Lookup table for available SASL client implementations.
  */
typedef struct {
    char   *client_type;
    struct XSASL_CLIENT_IMPL *(*client_init) (const char *, const char *);
} XSASL_CLIENT_IMPL_INFO;

static const XSASL_CLIENT_IMPL_INFO client_impl_info[] = {
#ifdef XSASL_TYPE_CYRUS
    XSASL_TYPE_CYRUS, xsasl_cyrus_client_init,
#endif
    0,
};

/* xsasl_client_init - look up client implementation by name */

XSASL_CLIENT_IMPL *xsasl_client_init(const char *client_type,
				             const char *path_info)
{
    const XSASL_CLIENT_IMPL_INFO *xp;

    for (xp = client_impl_info; xp->client_type; xp++)
	if (strcmp(client_type, xp->client_type) == 0)
	    return (xp->client_init(client_type, path_info));
    msg_warn("unsupported SASL client implementation: %s", client_type);
    return (0);
}

/* xsasl_client_types - report available implementation types */

ARGV   *xsasl_client_types(void)
{
    const XSASL_CLIENT_IMPL_INFO *xp;
    ARGV   *argv = argv_alloc(1);

    for (xp = client_impl_info; xp->client_type; xp++)
	argv_add(argv, xp->client_type, ARGV_END);
    return (argv);
}

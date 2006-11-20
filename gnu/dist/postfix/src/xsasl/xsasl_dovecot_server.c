/*	$NetBSD: xsasl_dovecot_server.c,v 1.1.1.1.4.2 2006/11/20 13:31:05 tron Exp $	*/

/*++
/* NAME
/*	xsasl_dovecot_server 3
/* SUMMARY
/*	Dovecot SASL server-side plug-in
/* SYNOPSIS
/*	XSASL_SERVER_IMPL *xsasl_dovecot_server_init(server_type, appl_name)
/*	const char *server_type;
/*	const char *appl_name;
/* DESCRIPTION
/*	This module implements the Dovecot SASL server-side authentication
/*	plug-in.
/*
/* .IP server_type
/*	The plug-in type that was specified to xsasl_server_init().
/*	The argument is ignored, because the Dovecot plug-in
/*	implements only one plug-in type.
/* .IP path_info
/*	The location of the Dovecot authentication server's UNIX-domain
/*	socket. Note: the Dovecot plug-in uses late binding, therefore
/*	all connect operations are done with Postfix privileges.
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
/*	Timo Sirainen
/*	Procontrol
/*	Finland
/*
/*	Adopted by:
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <connect.h>
#include <split_at.h>
#include <stringops.h>
#include <vstream.h>
#include <vstring_vstream.h>

/* Global library. */

#include <mail_params.h>

/* Application-specific. */

#include <xsasl.h>
#include <xsasl_dovecot.h>

#ifdef USE_SASL_AUTH

/* Major version changes are not backwards compatible,
   minor version numbers can be ignored. */
#define AUTH_PROTOCOL_MAJOR_VERSION 1
#define AUTH_PROTOCOL_MINOR_VERSION 0

 /*
  * Encorce read/write time limits, so that we can produce accurate
  * diagnostics instead of getting killed by the watchdog timer.
  */
#define AUTH_TIMEOUT	10

 /*
  * Class variables.
  */
typedef struct {
    XSASL_SERVER_IMPL xsasl;
    VSTREAM *sasl_stream;
    char   *socket_path;
    char   *mechanism_list;		/* applicable mechanisms */
    unsigned int request_id_counter;
} XSASL_DOVECOT_SERVER_IMPL;

 /*
  * The XSASL_DOVECOT_SERVER object is derived from the generic XSASL_SERVER
  * object.
  */
typedef struct {
    XSASL_SERVER xsasl;			/* generic members, must be first */
    XSASL_DOVECOT_SERVER_IMPL *impl;
    unsigned int last_request_id;
    char   *service;
    char   *username;			/* authenticated user */
    VSTRING *sasl_line;
} XSASL_DOVECOT_SERVER;

 /*
  * Forward declarations.
  */
static void xsasl_dovecot_server_done(XSASL_SERVER_IMPL *);
static XSASL_SERVER *xsasl_dovecot_server_create(XSASL_SERVER_IMPL *,
						         VSTREAM *,
						         const char *,
						         const char *,
						         const char *);
static void xsasl_dovecot_server_free(XSASL_SERVER *);
static int xsasl_dovecot_server_first(XSASL_SERVER *, const char *,
				              const char *, VSTRING *);
static int xsasl_dovecot_server_next(XSASL_SERVER *, const char *, VSTRING *);
static const char *xsasl_dovecot_server_get_mechanism_list(XSASL_SERVER *);
static const char *xsasl_dovecot_server_get_username(XSASL_SERVER *);

/* xsasl_dovecot_server_connect - initial auth server handshake */
 
static int xsasl_dovecot_server_connect(XSASL_DOVECOT_SERVER_IMPL *xp)
{
    const char *myname = "xsasl_dovecot_server_connect";
    VSTRING *line_str, *mechanisms_str;
    VSTREAM *sasl_stream;
    char   *line, *cmd, *mech_name;
    unsigned int major_version, minor_version;
    int     fd, success;

    if (msg_verbose)
	msg_info("%s: Connecting", myname);

    if ((fd = unix_connect(xp->socket_path, BLOCKING, AUTH_TIMEOUT)) < 0) {
	msg_warn("SASL: Connect to %s failed: %m", xp->socket_path);
	return (-1);
    }
    sasl_stream = vstream_fdopen(fd, O_RDWR);
    vstream_control(sasl_stream,
		    VSTREAM_CTL_PATH, xp->socket_path,
		    VSTREAM_CTL_TIMEOUT, AUTH_TIMEOUT,
		    VSTREAM_CTL_END);

    vstream_fprintf(sasl_stream,
		    "VERSION\t%u\t%u\n"
		    "CPID\t%u\n",
		    AUTH_PROTOCOL_MAJOR_VERSION,
		    AUTH_PROTOCOL_MINOR_VERSION,
		    (unsigned int) getpid());
    if (vstream_fflush(sasl_stream) == VSTREAM_EOF) {
	msg_warn("SASL: Couldn't send handshake: %m");
	return (-1);
    }
    success = 0;
    line_str = vstring_alloc(256);
    mechanisms_str = vstring_alloc(128);
    while (vstring_get_nonl(line_str, sasl_stream) != VSTREAM_EOF) {
	line = vstring_str(line_str);

	if (msg_verbose)
	    msg_info("%s: auth reply: %s", myname, line);

	cmd = line;
	line = split_at(line, '\t');

	if (strcmp(cmd, "VERSION") == 0) {
	    if (sscanf(line, "%u\t%u", &major_version, &minor_version) != 2) {
		msg_warn("SASL: Protocol version error");
		break;
	    }
	    if (major_version != AUTH_PROTOCOL_MAJOR_VERSION) {
		/* Major version is different from ours. */
		msg_warn("SASL: Protocol version mismatch (%d vs. %d)",
			 major_version, AUTH_PROTOCOL_MAJOR_VERSION);
		break;
	    }
	} else if (strcmp(cmd, "MECH") == 0 && line != NULL) {
	    mech_name = line;
	    line = split_at(line, '\t');

	    if (VSTRING_LEN(mechanisms_str) > 0)
		VSTRING_ADDCH(mechanisms_str, ' ');
	    vstring_strcat(mechanisms_str, mech_name);
	} else if (strcmp(cmd, "DONE") == 0) {
	    /* Handshake finished. */
	    success = 1;
	    break;
	} else {
	    /* ignore any unknown commands */
	}
    }
    vstring_free(line_str);

    if (!success) {
	/* handshake failed */
	vstring_free(mechanisms_str);
	(void) vstream_fclose(sasl_stream);
	return (-1);
    }
    xp->sasl_stream = sasl_stream;
    xp->mechanism_list =
	translit(vstring_export(mechanisms_str), "\t", " ");
    if (msg_verbose)
	msg_info("%s: Mechanisms: %s", myname, xp->mechanism_list);
    return (0);
}

/* xsasl_dovecot_server_disconnect - dispose of server connection state */

static void xsasl_dovecot_server_disconnect(XSASL_DOVECOT_SERVER_IMPL *xp)
{
    if (xp->sasl_stream) {
	(void) vstream_fclose(xp->sasl_stream);
	xp->sasl_stream = 0;
    }
    if (xp->mechanism_list) {
	myfree(xp->mechanism_list);
	xp->mechanism_list = 0;
    }
}

/* xsasl_dovecot_server_init - create implementation handle */

XSASL_SERVER_IMPL *xsasl_dovecot_server_init(const char *unused_server_type,
					             const char *path_info)
{
    XSASL_DOVECOT_SERVER_IMPL *xp;

    xp = (XSASL_DOVECOT_SERVER_IMPL *) mymalloc(sizeof(*xp));
    xp->xsasl.create = xsasl_dovecot_server_create;
    xp->xsasl.done = xsasl_dovecot_server_done;
    xp->socket_path = mystrdup(path_info);
    xp->sasl_stream = 0;
    xp->mechanism_list = 0;
    xp->request_id_counter = 0;
    return (&xp->xsasl);
}

/* xsasl_dovecot_server_done - dispose of implementation */

static void xsasl_dovecot_server_done(XSASL_SERVER_IMPL *impl)
{
    XSASL_DOVECOT_SERVER_IMPL *xp = (XSASL_DOVECOT_SERVER_IMPL *) impl;

    xsasl_dovecot_server_disconnect(xp);
    myfree(xp->socket_path);
    myfree((char *) impl);
}

/* xsasl_dovecot_server_create - create server instance */

static XSASL_SERVER *xsasl_dovecot_server_create(XSASL_SERVER_IMPL *impl,
					             VSTREAM *unused_stream,
						         const char *service,
						         const char *realm,
				               const char *unused_sec_props)
{
    const char *myname = "xsasl_dovecot_server_create";
    XSASL_DOVECOT_SERVER *server;

    if (msg_verbose)
	msg_info("%s: SASL service=%s, realm=%s",
		 myname, service, realm ? realm : "(null)");

    /*
     * Extend the XSASL_SERVER_IMPL object with our own data. We use
     * long-lived conversion buffers rather than local variables to avoid
     * memory leaks in case of read/write timeout or I/O error.
     */
    server = (XSASL_DOVECOT_SERVER *) mymalloc(sizeof(*server));
    server->xsasl.free = xsasl_dovecot_server_free;
    server->xsasl.first = xsasl_dovecot_server_first;
    server->xsasl.next = xsasl_dovecot_server_next;
    server->xsasl.get_mechanism_list = xsasl_dovecot_server_get_mechanism_list;
    server->xsasl.get_username = xsasl_dovecot_server_get_username;
    server->impl = (XSASL_DOVECOT_SERVER_IMPL *) impl;
    server->sasl_line = vstring_alloc(256);
    server->username = 0;
    server->service = mystrdup(service);
    server->last_request_id = 0;

    return (&server->xsasl);
}

/* xsasl_dovecot_server_get_mechanism_list - get available mechanisms */

static const char *xsasl_dovecot_server_get_mechanism_list(XSASL_SERVER *xp)
{
    XSASL_DOVECOT_SERVER *server = (XSASL_DOVECOT_SERVER *) xp;

    if (!server->impl->sasl_stream) {
	if (xsasl_dovecot_server_connect(server->impl) < 0)
	    return (0);
    }
    return (server->impl->mechanism_list);
}

/* xsasl_dovecot_server_free - destroy server instance */

static void xsasl_dovecot_server_free(XSASL_SERVER *xp)
{
    XSASL_DOVECOT_SERVER *server = (XSASL_DOVECOT_SERVER *) xp;

    vstring_free(server->sasl_line);
    if (server->username)
	myfree(server->username);
    myfree(server->service);
    myfree((char *) server);
}

/* xsasl_dovecot_server_auth_response - encode server first/next response */

static int xsasl_dovecot_parse_reply(XSASL_DOVECOT_SERVER *server, char **line)
{
    char   *id;

    if (*line == NULL) {
	msg_warn("SASL: Protocol error");
	return -1;
    }
    id = *line;
    *line = split_at(*line, '\t');

    if (strtoul(id, NULL, 0) != server->last_request_id) {
	/* reply to another request, shouldn't really happen.. */
	return -1;
    }
    return 0;
}

static void xsasl_dovecot_parse_reply_args(XSASL_DOVECOT_SERVER *server,
					         char *line, VSTRING *reply,
					           int success)
{
    char   *next;

    if (server->username) {
	myfree(server->username);
	server->username = 0;
    }

    /*
     * Note: TAB is part of the Dovecot protocol and must not appear in
     * legitimate Dovecot usernames, otherwise the protocol would break.
     */
    for (; line != NULL; line = next) {
	next = split_at(line, '\t');
	if (strncmp(line, "user=", 5) == 0) {
	    server->username = mystrdup(line + 5);
	    printable(server->username, '?');
	} else if (strncmp(line, "reason=", 7) == 0) {
	    if (!success) {
		printable(line + 7, '?');
		vstring_strcpy(reply, line + 7);
	    }
	}
    }
}

/* xsasl_dovecot_handle_reply - receive and process auth reply */

static int xsasl_dovecot_handle_reply(XSASL_DOVECOT_SERVER *server,
				              VSTRING *reply)
{
    const char *myname = "xsasl_dovecot_handle_reply";
    char   *line, *cmd;

    while (vstring_get_nonl(server->sasl_line,
			    server->impl->sasl_stream) != VSTREAM_EOF) {
	line = vstring_str(server->sasl_line);

	if (msg_verbose)
	    msg_info("%s: auth reply: %s", myname, line);

	cmd = line;
	line = split_at(line, '\t');

	if (strcmp(cmd, "OK") == 0) {
	    if (xsasl_dovecot_parse_reply(server, &line) == 0) {
		/* authentication successful */
		xsasl_dovecot_parse_reply_args(server, line, reply, 1);
		return XSASL_AUTH_DONE;
	    }
	} else if (strcmp(cmd, "CONT") == 0) {
	    if (xsasl_dovecot_parse_reply(server, &line) == 0) {
		vstring_strcpy(reply, line);
		return XSASL_AUTH_MORE;
	    }
	} else if (strcmp(cmd, "FAIL") == 0) {
	    if (xsasl_dovecot_parse_reply(server, &line) == 0) {
		/* authentication failure */
		xsasl_dovecot_parse_reply_args(server, line, reply, 0);
		return XSASL_AUTH_FAIL;
	    }
	} else {
	    /* ignore */
	}
    }

    vstring_strcpy(reply, "Connection lost to authentication server");
    return XSASL_AUTH_FAIL;
}

/* is_valid_base64 - input sanitized */

static int is_valid_base64(const char *data)
{

    /*
     * XXX Maybe use ISALNUM() (isascii && isalnum, i.e. locale independent).
     */
    for (; *data != '\0'; data++) {
	if (!((*data >= '0' && *data <= '9') ||
	      (*data >= 'a' && *data <= 'z') ||
	      (*data >= 'A' && *data <= 'Z') ||
	      *data == '+' || *data == '/' || *data == '='))
	    return 0;
    }
    return 1;
}

/* xsasl_dovecot_server_first - per-session authentication */

int     xsasl_dovecot_server_first(XSASL_SERVER *xp, const char *sasl_method,
			          const char *init_response, VSTRING *reply)
{
    const char *myname = "xsasl_dovecot_server_first";
    XSASL_DOVECOT_SERVER *server = (XSASL_DOVECOT_SERVER *) xp;
    int     i;

#define IFELSE(e1,e2,e3) ((e1) ? (e2) : (e3))

    if (msg_verbose)
	msg_info("%s: sasl_method %s%s%s", myname, sasl_method,
		 IFELSE(init_response, ", init_response ", ""),
		 IFELSE(init_response, init_response, ""));

    if (init_response)
	if (!is_valid_base64(init_response)) {
	    vstring_strcpy(reply, "Invalid base64 data in initial response");
	    return XSASL_AUTH_FAIL;
	}
    for (i = 0; i < 2; i++) {
	if (!server->impl->sasl_stream) {
	    if (xsasl_dovecot_server_connect(server->impl) < 0)
		return (0);
	}
	/* send the request */
	server->last_request_id = ++server->impl->request_id_counter;
	vstream_fprintf(server->impl->sasl_stream,
			"AUTH\t%u\t%s\tservice=%s",
			server->last_request_id, sasl_method,
			server->service);
	if (init_response) {

	    /*
	     * initial response is already base64 encoded, so we can send it
	     * directly.
	     */
	    vstream_fprintf(server->impl->sasl_stream,
			    "\tresp=%s", init_response);
	}
	VSTREAM_PUTC('\n', server->impl->sasl_stream);

	if (vstream_fflush(server->impl->sasl_stream) != VSTREAM_EOF)
	    break;

	if (i == 1) {
	    vstring_strcpy(reply, "Can't connect to authentication server");
	    return XSASL_AUTH_FAIL;
	}

	/*
	 * Reconnect and try again.
	 */
	xsasl_dovecot_server_disconnect(server->impl);
    }

    return xsasl_dovecot_handle_reply(server, reply);
}

/* xsasl_dovecot_server_next - continue authentication */

static int xsasl_dovecot_server_next(XSASL_SERVER *xp, const char *request,
				             VSTRING *reply)
{
    XSASL_DOVECOT_SERVER *server = (XSASL_DOVECOT_SERVER *) xp;

    if (!is_valid_base64(request)) {
	vstring_strcpy(reply, "Invalid base64 data in continued response");
	return XSASL_AUTH_FAIL;
    }
    vstream_fprintf(server->impl->sasl_stream,
		    "CONT\t%u\t%s\n", server->last_request_id, request);
    if (vstream_fflush(server->impl->sasl_stream) == VSTREAM_EOF) {
	vstring_strcpy(reply, "Connection lost to authentication server");
	return XSASL_AUTH_FAIL;
    }
    return xsasl_dovecot_handle_reply(server, reply);
}

/* xsasl_dovecot_server_get_username - get authenticated username */

static const char *xsasl_dovecot_server_get_username(XSASL_SERVER *xp)
{
    XSASL_DOVECOT_SERVER *server = (XSASL_DOVECOT_SERVER *) xp;

    return (server->username);
}

#endif

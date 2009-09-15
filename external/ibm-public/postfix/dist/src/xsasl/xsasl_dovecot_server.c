/*	$NetBSD: xsasl_dovecot_server.c,v 1.1.1.1.2.2 2009/09/15 06:04:13 snj Exp $	*/

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

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <connect.h>
#include <split_at.h>
#include <stringops.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <name_mask.h>
#include <argv.h>
#include <myaddrinfo.h>

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
  * Security property bitmasks.
  */
#define SEC_PROPS_NOPLAINTEXT	(1 << 0)
#define SEC_PROPS_NOACTIVE	(1 << 1)
#define SEC_PROPS_NODICTIONARY	(1 << 2)
#define SEC_PROPS_NOANONYMOUS	(1 << 3)
#define SEC_PROPS_FWD_SECRECY	(1 << 4)
#define SEC_PROPS_MUTUAL_AUTH	(1 << 5)
#define SEC_PROPS_PRIVATE	(1 << 6)

#define SEC_PROPS_POS_MASK	(SEC_PROPS_MUTUAL_AUTH | SEC_PROPS_FWD_SECRECY)
#define SEC_PROPS_NEG_MASK	(SEC_PROPS_NOPLAINTEXT | SEC_PROPS_NOACTIVE | \
				SEC_PROPS_NODICTIONARY | SEC_PROPS_NOANONYMOUS)

 /*
  * Security properties as specified in the Postfix main.cf file.
  */
static const NAME_MASK xsasl_dovecot_conf_sec_props[] = {
    "noplaintext", SEC_PROPS_NOPLAINTEXT,
    "noactive", SEC_PROPS_NOACTIVE,
    "nodictionary", SEC_PROPS_NODICTIONARY,
    "noanonymous", SEC_PROPS_NOANONYMOUS,
    "forward_secrecy", SEC_PROPS_FWD_SECRECY,
    "mutual_auth", SEC_PROPS_MUTUAL_AUTH,
    0, 0,
};

 /*
  * Security properties as specified in the Dovecot protocol. See
  * http://wiki.dovecot.org/Authentication_Protocol.
  */
static const NAME_MASK xsasl_dovecot_serv_sec_props[] = {
    "plaintext", SEC_PROPS_NOPLAINTEXT,
    "active", SEC_PROPS_NOACTIVE,
    "dictionary", SEC_PROPS_NODICTIONARY,
    "anonymous", SEC_PROPS_NOANONYMOUS,
    "forward-secrecy", SEC_PROPS_FWD_SECRECY,
    "mutual-auth", SEC_PROPS_MUTUAL_AUTH,
    "private", SEC_PROPS_PRIVATE,
    0, 0,
};

 /*
  * Class variables.
  */
typedef struct XSASL_DCSRV_MECH {
    char   *mech_name;			/* mechanism name */
    int     sec_props;			/* mechanism properties */
    struct XSASL_DCSRV_MECH *next;
} XSASL_DCSRV_MECH;

typedef struct {
    XSASL_SERVER_IMPL xsasl;
    VSTREAM *sasl_stream;
    char   *socket_path;
    XSASL_DCSRV_MECH *mechanism_list;	/* unfiltered mechanism list */
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
    unsigned int sec_props;		/* Postfix mechanism filter */
    int     tls_flag;			/* TLS enabled in this session */
    char   *mechanism_list;		/* filtered mechanism list */
    ARGV   *mechanism_argv;		/* ditto */
    char   *client_addr;		/* remote IP address */
    char   *server_addr;		/* remote IP address */
} XSASL_DOVECOT_SERVER;

 /*
  * Forward declarations.
  */
static void xsasl_dovecot_server_done(XSASL_SERVER_IMPL *);
static XSASL_SERVER *xsasl_dovecot_server_create(XSASL_SERVER_IMPL *,
					        XSASL_SERVER_CREATE_ARGS *);
static void xsasl_dovecot_server_free(XSASL_SERVER *);
static int xsasl_dovecot_server_first(XSASL_SERVER *, const char *,
				              const char *, VSTRING *);
static int xsasl_dovecot_server_next(XSASL_SERVER *, const char *, VSTRING *);
static const char *xsasl_dovecot_server_get_mechanism_list(XSASL_SERVER *);
static const char *xsasl_dovecot_server_get_username(XSASL_SERVER *);

/* xsasl_dovecot_server_mech_append - append server mechanism entry */

static void xsasl_dovecot_server_mech_append(XSASL_DCSRV_MECH **mech_list,
			               const char *mech_name, int sec_props)
{
    XSASL_DCSRV_MECH **mpp;
    XSASL_DCSRV_MECH *mp;

    for (mpp = mech_list; *mpp != 0; mpp = &mpp[0]->next)
	 /* void */ ;

    mp = (XSASL_DCSRV_MECH *) mymalloc(sizeof(*mp));
    mp->mech_name = mystrdup(mech_name);
    mp->sec_props = sec_props;
    mp->next = 0;
    *mpp = mp;
}

/* xsasl_dovecot_server_mech_free - destroy server mechanism list */

static void xsasl_dovecot_server_mech_free(XSASL_DCSRV_MECH *mech_list)
{
    XSASL_DCSRV_MECH *mp;
    XSASL_DCSRV_MECH *next;

    for (mp = mech_list; mp != 0; mp = next) {
	myfree(mp->mech_name);
	next = mp->next;
	myfree((char *) mp);
    }
}

/* xsasl_dovecot_server_mech_filter - filter server mechanism list */

static char *xsasl_dovecot_server_mech_filter(ARGV *mechanism_argv,
				           XSASL_DCSRV_MECH *mechanism_list,
					            unsigned int conf_props)
{
    const char *myname = "xsasl_dovecot_server_mech_filter";
    unsigned int pos_conf_props = (conf_props & SEC_PROPS_POS_MASK);
    unsigned int neg_conf_props = (conf_props & SEC_PROPS_NEG_MASK);
    VSTRING *mechanisms_str = vstring_alloc(10);
    XSASL_DCSRV_MECH *mp;

    /*
     * Match Postfix properties against Dovecot server properties.
     */
    for (mp = mechanism_list; mp != 0; mp = mp->next) {
	if ((mp->sec_props & pos_conf_props) == pos_conf_props
	    && (mp->sec_props & neg_conf_props) == 0) {
	    if (VSTRING_LEN(mechanisms_str) > 0)
		VSTRING_ADDCH(mechanisms_str, ' ');
	    vstring_strcat(mechanisms_str, mp->mech_name);
	    argv_add(mechanism_argv, mp->mech_name, (char *) 0);
	    if (msg_verbose)
		msg_info("%s: keep mechanism: %s", myname, mp->mech_name);
	} else {
	    if (msg_verbose)
		msg_info("%s: skip mechanism: %s", myname, mp->mech_name);
	}
    }
    return (vstring_export(mechanisms_str));
}

/* xsasl_dovecot_server_connect - initial auth server handshake */

static int xsasl_dovecot_server_connect(XSASL_DOVECOT_SERVER_IMPL *xp)
{
    const char *myname = "xsasl_dovecot_server_connect";
    VSTRING *line_str;
    VSTREAM *sasl_stream;
    char   *line, *cmd, *mech_name;
    unsigned int major_version, minor_version;
    int     fd, success;
    int     sec_props;
    const char *path;

    if (msg_verbose)
	msg_info("%s: Connecting", myname);

    /*
     * Not documented, but necessary for testing.
     */
    path = xp->socket_path;
    if (strncmp(path, "inet:", 5) == 0) {
	fd = inet_connect(path + 5, BLOCKING, AUTH_TIMEOUT);
    } else {
	if (strncmp(path, "unix:", 5) == 0)
	    path += 5;
	fd = unix_connect(path, BLOCKING, AUTH_TIMEOUT);
    }
    if (fd < 0) {
	msg_warn("SASL: Connect to %s failed: %m", xp->socket_path);
	return (-1);
    }
    sasl_stream = vstream_fdopen(fd, O_RDWR);
    vstream_control(sasl_stream,
		    VSTREAM_CTL_PATH, xp->socket_path,
		    VSTREAM_CTL_TIMEOUT, AUTH_TIMEOUT,
		    VSTREAM_CTL_END);

    /* XXX Encapsulate for logging. */
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
    /* XXX Encapsulate for logging. */
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
	    if (line != 0) {
		sec_props =
		    name_mask_delim_opt(myname,
					xsasl_dovecot_serv_sec_props,
					line, "\t", NAME_MASK_ANY_CASE);
		if ((sec_props & SEC_PROPS_PRIVATE) != 0)
		    continue;
	    } else
		sec_props = 0;
	    xsasl_dovecot_server_mech_append(&xp->mechanism_list, mech_name,
					     sec_props);
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
	(void) vstream_fclose(sasl_stream);
	return (-1);
    }
    xp->sasl_stream = sasl_stream;
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
	xsasl_dovecot_server_mech_free(xp->mechanism_list);
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
				             XSASL_SERVER_CREATE_ARGS *args)
{
    const char *myname = "xsasl_dovecot_server_create";
    XSASL_DOVECOT_SERVER *server;
    struct sockaddr_storage ss;
    struct sockaddr *sa = (struct sockaddr *) & ss;
    SOCKADDR_SIZE salen;
    MAI_HOSTADDR_STR server_addr;

    if (msg_verbose)
	msg_info("%s: SASL service=%s, realm=%s",
		 myname, args->service, args->user_realm ?
		 args->user_realm : "(null)");

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
    server->service = mystrdup(args->service);
    server->last_request_id = 0;
    server->mechanism_list = 0;
    server->mechanism_argv = 0;
    server->tls_flag = args->tls_flag;
    server->sec_props =
	name_mask_opt(myname, xsasl_dovecot_conf_sec_props,
		      args->security_options,
		      NAME_MASK_ANY_CASE | NAME_MASK_FATAL);
    server->client_addr = mystrdup(args->client_addr);

    /*
     * XXX Temporary code until smtpd_peer.c is updated.
     */
    if (args->server_addr && *args->server_addr) {
	server->server_addr = mystrdup(args->server_addr);
    } else {
	salen = sizeof(ss);
	if (getsockname(vstream_fileno(args->stream), sa, &salen) < 0
	    || sockaddr_to_hostaddr(sa, salen, &server_addr, 0, 0) != 0)
	    server_addr.buf[0] = 0;
	server->server_addr = mystrdup(server_addr.buf);
    }

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
    if (server->mechanism_list == 0) {
	server->mechanism_argv = argv_alloc(2);
	server->mechanism_list =
	    xsasl_dovecot_server_mech_filter(server->mechanism_argv,
					     server->impl->mechanism_list,
					     server->sec_props);
    }
    return (server->mechanism_list[0] ? server->mechanism_list : 0);
}

/* xsasl_dovecot_server_free - destroy server instance */

static void xsasl_dovecot_server_free(XSASL_SERVER *xp)
{
    XSASL_DOVECOT_SERVER *server = (XSASL_DOVECOT_SERVER *) xp;

    vstring_free(server->sasl_line);
    if (server->username)
	myfree(server->username);
    if (server->mechanism_list) {
	myfree(server->mechanism_list);
	argv_free(server->mechanism_argv);
    }
    myfree(server->service);
    myfree(server->server_addr);
    myfree(server->client_addr);
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

    /* XXX Encapsulate for logging. */
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
    char  **cpp;

#define IFELSE(e1,e2,e3) ((e1) ? (e2) : (e3))

    if (msg_verbose)
	msg_info("%s: sasl_method %s%s%s", myname, sasl_method,
		 IFELSE(init_response, ", init_response ", ""),
		 IFELSE(init_response, init_response, ""));

    if (server->mechanism_argv == 0)
	msg_panic("%s: no mechanism list", myname);

    for (cpp = server->mechanism_argv->argv; /* see below */ ; cpp++) {
	if (*cpp == 0) {
	    vstring_strcpy(reply, "Invalid authentication mechanism");
	    return XSASL_AUTH_FAIL;
	}
	if (strcasecmp(sasl_method, *cpp) == 0)
	    break;
    }
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
	/* XXX Encapsulate for logging. */
	vstream_fprintf(server->impl->sasl_stream,
			"AUTH\t%u\t%s\tservice=%s\tnologin\tlip=%s\trip=%s",
			server->last_request_id, sasl_method,
			server->service, server->server_addr,
			server->client_addr);
	if (server->tls_flag)
	    /* XXX Encapsulate for logging. */
	    vstream_fputs("\tsecured", server->impl->sasl_stream);
	if (init_response) {

	    /*
	     * initial response is already base64 encoded, so we can send it
	     * directly.
	     */
	    /* XXX Encapsulate for logging. */
	    vstream_fprintf(server->impl->sasl_stream,
			    "\tresp=%s", init_response);
	}
	/* XXX Encapsulate for logging. */
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
    /* XXX Encapsulate for logging. */
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

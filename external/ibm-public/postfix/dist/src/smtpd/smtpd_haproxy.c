/*	$NetBSD: smtpd_haproxy.c,v 1.1.1.1.8.2 2014/08/19 23:59:44 tls Exp $	*/

/*++
/* NAME
/*	smtpd_haproxy 3
/* SUMMARY
/*	Postfix SMTP server haproxy adapter
/* SYNOPSIS
/*	#include "smtpd.h"
/*
/*	int	smtpd_peer_from_haproxy(state)
/*	SMTPD_STATE *state;
/* DESCRIPTION
/*	smtpd_peer_from_haproxy() receives endpoint address and
/*	port information via the haproxy protocol.
/*
/*	The following summarizes what the Postfix SMTP server expects
/*	from an up-stream proxy adapter.
/* .IP \(bu
/*	Validate protocol, address and port syntax. Permit only
/*	protocols that are configured with the main.cf:inet_protocols
/*	setting.
/* .IP \(bu
/*	Convert IPv4-in-IPv6 address syntax to IPv4 syntax when
/*	both IPv6 and IPv4 support are enabled with main.cf:inet_protocols.
/* .IP \(bu
/*	Update the following session context fields: addr, port,
/*	rfc_addr, addr_family, dest_addr. The addr_family field
/*	applies to the client address.
/* .IP \(bu
/*	Dynamically allocate storage for string information with
/*	mystrdup(). In case of error, leave unassigned string fields
/*	at their initial zero value.
/* .IP \(bu
/*	Log a clear warning message that explains why a request
/*	fails.
/* .IP \(bu
/*	Never talk to the remote SMTP client.
/* .PP
/*	Arguments:
/* .IP state
/*	Session context.
/* DIAGNOSTICS
/*	Warnings: I/O errors, malformed haproxy line.
/*
/*	The result value is 0 in case of success, -1 in case of
/*	error.
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

/* System library. */

#include <sys_defs.h>
#include <sys/socket.h>

/* Utility library. */

#include <msg.h>
#include <myaddrinfo.h>
#include <mymalloc.h>
#include <stringops.h>

/* Global library. */

#include <smtp_stream.h>
#include <mail_params.h>
#include <valid_mailhost_addr.h>
#include <haproxy_srvr.h>

/* Application-specific. */

#include <smtpd.h>

/* SLMs. */

#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

/* smtpd_peer_from_haproxy - initialize peer information from haproxy */

int     smtpd_peer_from_haproxy(SMTPD_STATE *state)
{
    const char *myname = "smtpd_peer_from_haproxy";
    MAI_HOSTADDR_STR smtp_client_addr;
    MAI_SERVPORT_STR smtp_client_port;
    MAI_HOSTADDR_STR smtp_server_addr;
    MAI_SERVPORT_STR smtp_server_port;
    const char *proxy_err;
    int     io_err;
    VSTRING *escape_buf;

    /*
     * Note: the haproxy_srvr_parse() routine performs address protocol
     * checks, address and port syntax checks, and converts IPv4-in-IPv6
     * address string syntax (:ffff::1.2.3.4) to IPv4 syntax where permitted
     * by the main.cf:inet_protocols setting, but logs no warnings.
     */
#define ENABLE_DEADLINE	1

    smtp_stream_setup(state->client, var_smtpd_uproxy_tmout, ENABLE_DEADLINE);
    switch (io_err = vstream_setjmp(state->client)) {
    default:
	msg_panic("%s: unhandled I/O error %d", myname, io_err);
    case SMTP_ERR_EOF:
	msg_warn("haproxy read: unexpected EOF");
	return (-1);
    case SMTP_ERR_TIME:
	msg_warn("haproxy read: timeout error");
	return (-1);
    case 0:
	if (smtp_get(state->buffer, state->client, HAPROXY_MAX_LEN,
		     SMTP_GET_FLAG_NONE) != '\n') {
	    msg_warn("haproxy read: line > %d characters", HAPROXY_MAX_LEN);
	    return (-1);
	}
	if ((proxy_err = haproxy_srvr_parse(STR(state->buffer),
				       &smtp_client_addr, &smtp_client_port,
			      &smtp_server_addr, &smtp_server_port)) != 0) {
	    escape_buf = vstring_alloc(HAPROXY_MAX_LEN + 2);
	    escape(escape_buf, STR(state->buffer), LEN(state->buffer));
	    msg_warn("haproxy read: %s: %s", proxy_err, STR(escape_buf));
	    vstring_free(escape_buf);
	    return (-1);
	}
	state->addr = mystrdup(smtp_client_addr.buf);
	if (strrchr(state->addr, ':') != 0) {
	    state->rfc_addr = concatenate(IPV6_COL, state->addr, (char *) 0);
	    state->addr_family = AF_INET6;
	} else {
	    state->rfc_addr = mystrdup(state->addr);
	    state->addr_family = AF_INET;
	}
	state->port = mystrdup(smtp_client_port.buf);

	/*
	 * Avoid surprises in the Dovecot authentication server.
	 */
	state->dest_addr = mystrdup(smtp_server_addr.buf);
	return (0);
    }
}

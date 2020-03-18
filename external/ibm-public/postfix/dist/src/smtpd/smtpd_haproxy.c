/*	$NetBSD: smtpd_haproxy.c,v 1.3 2020/03/18 19:05:20 christos Exp $	*/

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
/*	Call smtpd_peer_from_default() if the up-stream proxy
/*	indicates that the connection is not proxied. In that case,
/*	a proxy adapter MUST NOT update any STATE fields: the
/*	smtpd_peer_from_default() function will do that instead.
/* .IP \(bu
/*	Validate protocol, address and port syntax. Permit only
/*	protocols that are configured with the main.cf:inet_protocols
/*	setting.
/* .IP \(bu
/*	Convert IPv4-in-IPv6 address syntax to IPv4 syntax when
/*	both IPv6 and IPv4 support are enabled with main.cf:inet_protocols.
/* .IP \(bu
/*	Update the following session context fields: addr, port,
/*	rfc_addr, addr_family, dest_addr, dest_port. The addr_family
/*	field applies to the client address.
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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/socket.h>

/* Utility library. */

#include <msg.h>
#include <myaddrinfo.h>
#include <mymalloc.h>
#include <stringops.h>
#include <iostuff.h>

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
    MAI_HOSTADDR_STR smtp_client_addr;
    MAI_SERVPORT_STR smtp_client_port;
    MAI_HOSTADDR_STR smtp_server_addr;
    MAI_SERVPORT_STR smtp_server_port;
    int     non_proxy = 0;

    if (read_wait(vstream_fileno(state->client), var_smtpd_uproxy_tmout) < 0) {
	msg_warn("haproxy read: timeout error");
	return (-1);
    }
    if (haproxy_srvr_receive(vstream_fileno(state->client), &non_proxy,
			     &smtp_client_addr, &smtp_client_port,
			     &smtp_server_addr, &smtp_server_port) < 0) {
	return (-1);
    }
    if (non_proxy) {
	smtpd_peer_from_default(state);
	return (0);
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
     * The Dovecot authentication server needs the server IP address.
     */
    state->dest_addr = mystrdup(smtp_server_addr.buf);
    state->dest_port = mystrdup(smtp_server_port.buf);
    return (0);
}

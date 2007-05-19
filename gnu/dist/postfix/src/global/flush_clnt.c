/*	$NetBSD: flush_clnt.c,v 1.1.1.7 2007/05/19 16:28:12 heas Exp $	*/

/*++
/* NAME
/*	flush_clnt 3
/* SUMMARY
/*	fast flush cache manager client interface
/* SYNOPSIS
/*	#include <flush_clnt.h>
/*
/*	void	flush_init()
/*
/*	int	flush_add(site, queue_id)
/*	const char *site;
/*	const char *queue_id;
/*
/*	int	flush_send_site(site)
/*	const char *site;
/*
/*	int	flush_send_file(queue_id)
/*	const char *queue_id;
/*
/*	int	flush_refresh()
/*
/*	int	flush_purge()
/* DESCRIPTION
/*	The following routines operate through the "fast flush" service.
/*	This service maintains a cache of what mail is queued. The cache
/*	is maintained for eligible destinations. A destination is the
/*	right-hand side of a user@domain email address.
/*
/*	flush_init() initializes. It must be called before dropping
/*	privileges in a daemon process.
/*
/*	flush_add() informs the "fast flush" cache manager that mail is
/*	queued for the specified site with the specified queue ID.
/*
/*	flush_send_site() requests delivery of all mail that is queued for
/*	the specified destination.
/*
/*	flush_send_file() requests delivery of mail with the specified
/*	queue ID.
/*
/*	flush_refresh() requests the "fast flush" cache manager to refresh
/*	cached information that was not used for some configurable amount
/*	time.
/*
/*	flush_purge() requests the "fast flush" cache manager to refresh
/*	all cached information. This is incredibly expensive, and is not
/*	recommended.
/* DIAGNOSTICS
/*	The result codes and their meanings are (see flush_clnt(5h)):
/* .IP MAIL_FLUSH_OK
/*	The request completed successfully (in case of requests that
/*	complete in the background: the request was accepted by the server).
/* .IP MAIL_FLUSH_FAIL
/*	The request failed (the request could not be sent to the server,
/*	or the server reported failure).
/* .IP MAIL_FLUSH_BAD
/*	The "fast flush" server rejected the request (invalid request
/*	parameter).
/* .IP MAIL_FLUSH_DENY
/*	The specified domain is not eligible for "fast flush" service,
/*	or the "fast flush" service is disabled.
/* SEE ALSO
/*	flush(8) Postfix fast flush cache manager
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

#include "sys_defs.h"
#include <unistd.h>
#include <stdarg.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>

/* Global library. */

#include <mail_proto.h>
#include <mail_flush.h>
#include <mail_params.h>
#include <domain_list.h>
#include <match_parent_style.h>
#include <flush_clnt.h>

/* Application-specific. */

#define STR(x)	vstring_str(x)

static DOMAIN_LIST *flush_domains;

/* flush_init - initialize */

void    flush_init(void)
{
    flush_domains = domain_list_init(match_parent_style(VAR_FFLUSH_DOMAINS),
				     var_fflush_domains);
}

/* flush_purge - house keeping */

int     flush_purge(void)
{
    const char *myname = "flush_purge";
    int     status;

    if (msg_verbose)
	msg_info("%s", myname);

    /*
     * Don't bother the server if the service is turned off.
     */
    if (*var_fflush_domains == 0)
	status = FLUSH_STAT_DENY;
    else
	status = mail_command_client(MAIL_CLASS_PUBLIC, var_flush_service,
			      ATTR_TYPE_STR, MAIL_ATTR_REQ, FLUSH_REQ_PURGE,
				     ATTR_TYPE_END);

    if (msg_verbose)
	msg_info("%s: status %d", myname, status);

    return (status);
}

/* flush_refresh - house keeping */

int     flush_refresh(void)
{
    const char *myname = "flush_refresh";
    int     status;

    if (msg_verbose)
	msg_info("%s", myname);

    /*
     * Don't bother the server if the service is turned off.
     */
    if (*var_fflush_domains == 0)
	status = FLUSH_STAT_DENY;
    else
	status = mail_command_client(MAIL_CLASS_PUBLIC, var_flush_service,
			    ATTR_TYPE_STR, MAIL_ATTR_REQ, FLUSH_REQ_REFRESH,
				     ATTR_TYPE_END);

    if (msg_verbose)
	msg_info("%s: status %d", myname, status);

    return (status);
}

/* flush_send_site - deliver mail queued for site */

int     flush_send_site(const char *site)
{
    const char *myname = "flush_send_site";
    int     status;

    if (msg_verbose)
	msg_info("%s: site %s", myname, site);

    /*
     * Don't bother the server if the service is turned off, or if the site
     * is not eligible.
     */
    if (flush_domains == 0)
	msg_panic("missing flush client initialization");
    if (domain_list_match(flush_domains, site) == 0)
	status = FLUSH_STAT_DENY;
    else
	status = mail_command_client(MAIL_CLASS_PUBLIC, var_flush_service,
			  ATTR_TYPE_STR, MAIL_ATTR_REQ, FLUSH_REQ_SEND_SITE,
				     ATTR_TYPE_STR, MAIL_ATTR_SITE, site,
				     ATTR_TYPE_END);

    if (msg_verbose)
	msg_info("%s: site %s status %d", myname, site, status);

    return (status);
}

/* flush_send_file - deliver specific message */

int     flush_send_file(const char *queue_id)
{
    const char *myname = "flush_send_file";
    int     status;

    if (msg_verbose)
	msg_info("%s: queue_id %s", myname, queue_id);

    /*
     * Require that the service is turned on.
     */
    status = mail_command_client(MAIL_CLASS_PUBLIC, var_flush_service,
			  ATTR_TYPE_STR, MAIL_ATTR_REQ, FLUSH_REQ_SEND_FILE,
				 ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, queue_id,
				 ATTR_TYPE_END);

    if (msg_verbose)
	msg_info("%s: queue_id %s status %d", myname, queue_id, status);

    return (status);
}

/* flush_add - inform "fast flush" cache manager */

int     flush_add(const char *site, const char *queue_id)
{
    const char *myname = "flush_add";
    int     status;

    if (msg_verbose)
	msg_info("%s: site %s id %s", myname, site, queue_id);

    /*
     * Don't bother the server if the service is turned off, or if the site
     * is not eligible.
     */
    if (flush_domains == 0)
	msg_panic("missing flush client initialization");
    if (domain_list_match(flush_domains, site) == 0)
	status = FLUSH_STAT_DENY;
    else
	status = mail_command_client(MAIL_CLASS_PUBLIC, var_flush_service,
				ATTR_TYPE_STR, MAIL_ATTR_REQ, FLUSH_REQ_ADD,
				     ATTR_TYPE_STR, MAIL_ATTR_SITE, site,
				 ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, queue_id,
				     ATTR_TYPE_END);

    if (msg_verbose)
	msg_info("%s: site %s id %s status %d", myname, site, queue_id,
		 status);

    return (status);
}

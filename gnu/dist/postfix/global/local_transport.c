/*++
/* NAME
/*	local_transport 3
/* SUMMARY
/*	determine if transport delivers locally
/* SYNOPSIS
/*	#include <local_transport.h>
/*
/*	const char *get_def_local_transport()
/*
/*	int	match_def_local_transport(transport)
/*	const char *transport;
/*
/*	int	match_any_local_transport(transport)
/*	const char *transport;
/* DESCRIPTION
/*	This module uses the information kept in the "local_transports"
/*	configuration parameter, which lists the name of the default
/*	local transport, followed by the names of zero or more other
/*	transports that deliver locally.
/*
/*	get_def_local_transport() returns the name of the default local
/*	transport, that is, the first transport name specified with
/*	the "local_transports" configuration parameter.
/*
/*	match_def_local_transport() determines if the named transport is
/*	identical to the default local transport.
/*
/*	match_any_local_transport() determines if the named transport is
/*	listed in the "local_transports" configuration parameter.
/* SEE ALSO
/*	resolve_local(3), see if address resolves locally
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
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <string_list.h>

/* Global library. */

#include <mail_params.h>
#include <local_transport.h>

/* Application-specific */

static STRING_LIST *local_transport_list;
static char *local_transport_name;

/* local_transport_init - initialize lookup table */

static void local_transport_init(void)
{
    char   *myname = "local_transport_init";

    /*
     * Sanity check.
     */
    if (local_transport_list || local_transport_name)
	msg_panic("local_transport_init: duplicate initialization");

    /*
     * Initialize.
     */
    local_transport_list = string_list_init(var_local_transports);
    local_transport_name = mystrndup(var_local_transports,
				 strcspn(var_local_transports, ", \t\r\n"));

    /*
     * Sanity check.
     */
    if (!match_any_local_transport(local_transport_name)
	|| !match_def_local_transport(local_transport_name))
	msg_panic("%s: unable to intialize", myname);
}

/* get_def_local_transport - determine default local transport */

const char *get_def_local_transport(void)
{

    /*
     * Initialize on the fly.
     */
    if (local_transport_name == 0)
	local_transport_init();

    /*
     * Return the first transport listed.
     */
    return (local_transport_name);
}

/* match_def_local_transport - match against default local transport */

int     match_def_local_transport(const char *transport)
{

    /*
     * Initialize on the fly.
     */
    if (local_transport_list == 0)
	local_transport_init();

    /*
     * Compare the transport against the list of transports that are listed
     * as delivering locally.
     */
    return (strcmp(transport, local_transport_name) == 0);
}

/* match_any_local_transport - match against list of local transports */

int     match_any_local_transport(const char *transport)
{

    /*
     * Initialize on the fly.
     */
    if (local_transport_list == 0)
	local_transport_init();

    /*
     * Compare the transport against the list of transports that are listed
     * as delivering locally.
     */
    return (string_list_match(local_transport_list, transport));
}

#ifdef TEST

#include <vstream.h>
#include <mail_conf.h>

int     main(int argc, char **argv)
{
    if (argc != 2)
	msg_fatal("usage: %s transport", argv[0]);
    mail_conf_read();
    vstream_printf("%s\n", match_any_local_transport(argv[1]) ? "yes" : "no");
    vstream_fflush(VSTREAM_OUT);
}

#endif

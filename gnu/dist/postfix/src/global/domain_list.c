/*++
/* NAME
/*	domain_list 3
/* SUMMARY
/*	match a host or domain name against a pattern list
/* SYNOPSIS
/*	#include <domain_list.h>
/*
/*	DOMAIN_LIST *domain_list_init(pattern_list)
/*	const char *pattern_list;
/*
/*	int	domain_list_match(list, name)
/*	DOMAIN_LIST *list;
/*	const char *name;
/*
/*	void domain_list_free(list)
/*	DOMAIN_LIST *list;
/* DESCRIPTION
/*	This module implements tests for list membership of a host or
/*	domain name.
/*
/*	Patterns are separated by whitespace and/or commas. A pattern
/*	is either a string, a file name (in which case the contents
/*	of the file are substituted for the file name) or a type:name
/*	lookup table specification.
/*
/*	A host name matches a domain list when its name appears in the
/*	list of domain patterns, or when any of its parent domains appears
/*	in the list of domain patterns. The matching process is case
/*	insensitive. In order to reverse the result, precede a non-file
/*	name pattern with an exclamation point (!).
/*
/*	domain_list_init() performs initializations. The argument is a
/*	list of domain patterns, or the name of a file containing domain
/*	patterns.
/*
/*	domain_list_match() matches the specified host or domain name
/*	against the specified pattern list.
/*
/*	domain_list_free() releases storage allocated by domain_list_init().
/* DIAGNOSTICS
/*	Fatal error: unable to open or read a domain_list file; invalid
/*	domain_list pattern.
/* SEE ALSO
/*	match_list(3) generic list matching
/*	match_ops(3) match hosts by name or by address
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

/* Utility library. */

#include <match_list.h>
#include <match_ops.h>

/* Global library. */

#include "domain_list.h"

/* domain_list_init - initialize domain list */

DOMAIN_LIST *domain_list_init(const char *patterns)
{
    return (match_list_init(patterns, 1, match_hostname));
}

/* domain_list_match - match host against domain list */

int     domain_list_match(DOMAIN_LIST *list, const char *name)
{
    return (match_list_match(list, name));
}

/* domain_list_free - release storage */

void    domain_list_free(DOMAIN_LIST *list)
{
    match_list_free(list);
}

#ifdef TEST

#include <msg.h>
#include <stdlib.h>
#include <vstream.h>
#include <msg_vstream.h>

static void usage(char *progname)
{
    msg_fatal("usage: %s [-v] patterns hostname", progname);
}

main(int argc, char **argv)
{
    DOMAIN_LIST *list;
    char   *host;
    int     ch;

    msg_vstream_init(argv[0], VSTREAM_ERR);

    while ((ch = GETOPT(argc, argv, "v")) > 0) {
	switch (ch) {
	case 'v':
	    msg_verbose++;
	    break;
	default:
	    usage(argv[0]);
	}
    }
    if (argc != optind + 2)
	usage(argv[0]);
    list = domain_list_init(argv[optind]);
    host = argv[optind + 1];
    vstream_printf("%s: %s\n", host, domain_list_match(list, host) ?
		   "YES" : "NO");
    vstream_fflush(VSTREAM_OUT);
    domain_list_free(list);
}

#endif

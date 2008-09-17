/*++
/* NAME
/*	namadr_list 3
/* SUMMARY
/*	name/address list membership
/* SYNOPSIS
/*	#include <namadr_list.h>
/*
/*	NAMADR_LIST *namadr_list_init(flags, pattern_list)
/*	int	flags;
/*	const char *pattern_list;
/*
/*	int	namadr_list_match(list, name, addr)
/*	NAMADR_LIST *list;
/*	const char *name;
/*	const char *addr;
/*
/*	void	namadr_list_free(list)
/*	NAMADR_LIST *list;
/* DESCRIPTION
/*	This is a convenience wrapper around the match_list module.
/*
/*	This module implements tests for list membership of a
/*	hostname or network address.
/*
/*	A list pattern specifies a host name, a domain name,
/*	an internet address, or a network/mask pattern, where the
/*	mask specifies the number of bits in the network part.
/*	When a pattern specifies a file name, its contents are
/*	substituted for the file name; when a pattern is a
/*	type:name table specification, table lookup is used
/*	instead.
/*	Patterns are separated by whitespace and/or commas. In
/*	order to reverse the result, precede a pattern with an
/*	exclamation point (!).
/*
/*	A host matches a list when its name or address matches
/*	a pattern, or when any of its parent domains matches a
/*	pattern. The matching process is case insensitive.
/*
/*	namadr_list_init() performs initializations. The first
/*	argument is the bit-wise OR of zero or more of the
/*	following:
/* .RS
/* .IP MATCH_FLAG_PARENT
/*	The hostname pattern foo.com matches itself and any name below
/*	the domain foo.com. If this flag is cleared, foo.com matches itself
/*	only, and .foo.com matches any name below the domain foo.com.
/* .RE
/*	Specify MATCH_FLAG_NONE to request none of the above.
/*	The second argument is a list of patterns, or the absolute
/*	pathname of a file with patterns.
/*
/*	namadr_list_match() matches the specified host name and
/*	address against the specified list of patterns.
/*
/*	namadr_list_free() releases storage allocated by namadr_list_init().
/* DIAGNOSTICS
/*	Fatal errors: unable to open or read a pattern file; invalid
/*	pattern. Panic: interface violations.
/* SEE ALSO
/*	match_list(3) generic list matching
/*	match_ops(3) match host by name or by address
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

/* Global library. */

#include "namadr_list.h"

#ifdef TEST

#include <msg.h>
#include <stdlib.h>
#include <unistd.h>
#include <vstream.h>
#include <msg_vstream.h>

static void usage(char *progname)
{
    msg_fatal("usage: %s [-v] pattern_list hostname address", progname);
}

int     main(int argc, char **argv)
{
    NAMADR_LIST *list;
    char   *host;
    char   *addr;
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
    if (argc != optind + 3)
	usage(argv[0]);
    list = namadr_list_init(MATCH_FLAG_PARENT, argv[optind]);
    host = argv[optind + 1];
    addr = argv[optind + 2];
    vstream_printf("%s/%s: %s\n", host, addr,
		   namadr_list_match(list, host, addr) ?
		   "YES" : "NO");
    vstream_fflush(VSTREAM_OUT);
    namadr_list_free(list);
    return (0);
}

#endif
